//--------------------------------------------------------------------------------------
// Copyright 2011 Intel Corporation
// All Rights Reserved
//
// Permission is granted to use, copy, distribute and prepare derivative works of this
// software for any purpose and without fee, provided, that the above copyright notice
// and this statement appear in all copies.  Intel makes no representations about the
// suitability of this software for any purpose.  THIS SOFTWARE IS PROVIDED "AS IS."
// INTEL SPECIFICALLY DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, AND ALL LIABILITY,
// INCLUDING CONSEQUENTIAL AND OTHER INDIRECT DAMAGES, FOR THE USE OF THIS SOFTWARE,
// INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PROPRIETARY RIGHTS, AND INCLUDING THE
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  Intel does not
// assume any responsibility for any errors which may appear in this software nor any
// responsibility to update it.
//
//--------------------------------------------------------------------------------------

#include "DepthBufferRasterizerSSEST.h"

DepthBufferRasterizerSSEST::DepthBufferRasterizerSSEST()
	: DepthBufferRasterizerSSE()
{
	int size = SCREENH_IN_TILES * SCREENW_IN_TILES; 
	mpBin = new UINT[size * MAX_TRIS_IN_BIN_ST];
	mpBinModel = new USHORT[size * MAX_TRIS_IN_BIN_ST];
	mpBinMesh = new USHORT[size * MAX_TRIS_IN_BIN_ST];
	mpNumTrisInBin = new USHORT[size];
}

DepthBufferRasterizerSSEST::~DepthBufferRasterizerSSEST()
{
	SAFE_DELETE_ARRAY(mpBin);
	SAFE_DELETE_ARRAY(mpBinModel);
	SAFE_DELETE_ARRAY(mpBinMesh);
	SAFE_DELETE_ARRAY(mpNumTrisInBin);
}

//------------------------------------------------------------
// * Determine if the occludee model is inside view frustum
//------------------------------------------------------------
void DepthBufferRasterizerSSEST::IsVisible(CPUTCamera* pCamera)
{
	mpCamera = pCamera;
	
	for(UINT i = 0; i < mNumModels1; i++)
	{
		mpTransformedModels1[i].IsVisible(mpCamera);
	}
}

//------------------------------------------------------------------------------
// * Transform the occluder models on the CPU
// * Bin the occluder triangles into tiles that the frame buffer is divided into
// * Rasterize the occluder triangles to the CPU depth buffer
//-------------------------------------------------------------------------------
void DepthBufferRasterizerSSEST::TransformModelsAndRasterizeToDepthBuffer()
{
	mRasterizeTimer.StartTimer();
		
	TransformMeshes();
	BinTransformedMeshes();
	for(UINT i = 0; i < NUM_TILES; i++)
	{
		RasterizeBinnedTrianglesToDepthBuffer(i);
	}

	mRasterizeTime[mTimeCounter++] = mRasterizeTimer.StopTimer();
	mTimeCounter = mTimeCounter >= AVG_COUNTER ? 0 : mTimeCounter;

	mNumRasterized = 0;
	for(UINT i = 0; i < mNumModels1; i++)
	{
		mNumRasterized += mpTransformedModels1[i].IsRasterized2DB() ? 1 : 0;
	}
}

//-------------------------------------------------------------------
// Trasforms the occluder vertices to screen space once every frame
//-------------------------------------------------------------------
void DepthBufferRasterizerSSEST::TransformMeshes()
{
	for(UINT ss = 0; ss < mNumModels1; ss++)
    {
		UINT thisSurfaceVertexCount = mpTransformedModels1[ss].GetNumVertices();
        
        mpTransformedModels1[ss].TransformMeshes(mViewMatrix, mProjMatrix, 0, thisSurfaceVertexCount - 1, mpCamera);
    }
}

//-------------------------------------------------
// Bins the transformed triangles into tiles
//-------------------------------------------------
void DepthBufferRasterizerSSEST::BinTransformedMeshes()
{
	// Reset the bin count.  Note the data layout makes this traversal a bit awkward.
    // We can't just use memset() because the last array index isn't what's varying.
    // However, this should make the real use of this structure go faster.
	for(UINT yy = 0; yy < SCREENH_IN_TILES; yy++)
    {
		UINT offset = YOFFSET1_ST * yy;
        for(UINT xx = 0; xx < SCREENW_IN_TILES; xx++)
        {
			UINT index = offset + (XOFFSET1_ST * xx);
            mpNumTrisInBin[index] = 0;
	    }
    }

	// Now, process all of the surfaces that contain this task's triangle range.
	for(UINT ss = 0; ss < mNumModels1; ss++)
    {
		UINT thisSurfaceTriangleCount = mpTransformedModels1[ss].GetNumTriangles();
        
		mpTransformedModels1[ss].BinTransformedTrianglesST(0, ss, 0, thisSurfaceTriangleCount - 1, mpBin, mpBinModel, mpBinMesh, mpNumTrisInBin);
	}
}

//-------------------------------------------------------------------------------
// For each tile go through all the bins and process all the triangles in it.
// Rasterize each triangle to the CPU depth buffer. 
//-------------------------------------------------------------------------------
void DepthBufferRasterizerSSEST::RasterizeBinnedTrianglesToDepthBuffer(UINT tileId)
{
	// Set DAZ and FZ MXCSR bits to flush denormals to zero (i.e., make it faster)
	_mm_setcsr( _mm_getcsr() | 0x8040 );

	__m128i colOffset = _mm_setr_epi32(0, 1, 0, 1);
	__m128i rowOffset = _mm_setr_epi32(0, 0, 1, 1);

	float* pDepthBuffer = (float*)mpRenderTargetPixels; 

	// Based on TaskId determine which tile to process
	UINT screenWidthInTiles = SCREENW/TILE_WIDTH_IN_PIXELS;
    UINT tileX = tileId % screenWidthInTiles;
    UINT tileY = tileId / screenWidthInTiles;

    int tileStartX = tileX * TILE_WIDTH_IN_PIXELS;
	int tileEndX   = min(tileStartX + TILE_WIDTH_IN_PIXELS - 1, SCREENW - 1);
	
	int tileStartY = tileY * TILE_HEIGHT_IN_PIXELS;
	int tileEndY   = min(tileStartY + TILE_HEIGHT_IN_PIXELS - 1, SCREENH - 1);

	UINT bin = 0;
	UINT binIndex = 0;
	UINT offset1 = YOFFSET1_ST * tileY + XOFFSET1_ST * tileX;
	UINT offset2 = YOFFSET2_ST * tileY + XOFFSET2_ST * tileX;
	UINT numTrisInBin = mpNumTrisInBin[offset1 + bin];

	vFloat4 xformedPos[3];
	bool done = false;
	bool allBinsEmpty = true;
	mNumRasterizedTris[tileId] = numTrisInBin;
	while(!done)
	{
		// Loop through all the bins and process 4 binned traingles at a time
		UINT ii;
		int numSimdTris = 0;
		for(ii = 0; ii < SSE; ii++)
		{
			while(numTrisInBin <= 0)
			{
				 // This bin is empty.  Move to next bin.
				if(++bin >= 1)
				{
					break;
				}
				numTrisInBin = mpNumTrisInBin[offset1 + bin];
				mNumRasterizedTris[tileId] += numTrisInBin;
				binIndex = 0;
			}
			if(!numTrisInBin)
			{
				break; // No more tris in the bins
			}
			USHORT modelId = mpBinModel[offset2 + bin * MAX_TRIS_IN_BIN_ST + binIndex];
			USHORT meshId = mpBinMesh[offset2 + bin * MAX_TRIS_IN_BIN_ST + binIndex];
			UINT triIdx = mpBin[offset2 + bin * MAX_TRIS_IN_BIN_ST + binIndex];
			mpTransformedModels1[modelId].Gather((float*)&xformedPos, meshId, triIdx, ii);
			allBinsEmpty = false;
			numSimdTris++; 

			++binIndex;
			--numTrisInBin;
		}
		done = bin >= NUM_XFORMVERTS_TASKS;
		
		if(allBinsEmpty)
		{
			return;
		}

		vFloat4* xformedvPos = (vFloat4*)&xformedPos;

		// use fixed-point only for X and Y.  Avoid work for Z and W.
		__m128i fixX[3], fixY[3];
		for(int i = 0; i < 3; i++)
		{
			fixX[i] = _mm_cvtps_epi32(xformedvPos[i].X);
			fixY[i] = _mm_cvtps_epi32(xformedvPos[i].Y);
		}

		// Fab(x, y) =     Ax       +       By     +      C              = 0
		// Fab(x, y) = (ya - yb)x   +   (xb - xa)y + (xa * yb - xb * ya) = 0
		// Compute A = (ya - yb) for 2 of the 3 line segments that make up each triangle
		__m128i A1 = _mm_sub_epi32(fixY[2], fixY[0]);
		__m128i A2 = _mm_sub_epi32(fixY[0], fixY[1]);

		// Compute B = (xb - xa) for 2 of the 3 line segments that make up each triangle
		__m128i B1 = _mm_sub_epi32(fixX[0], fixX[2]);
		__m128i B2 = _mm_sub_epi32(fixX[1], fixX[0]);

		// Compute C = (xa * yb - xb * ya) for 2 of the 3 line segments that make up each triangle
		__m128i C1 = _mm_sub_epi32(_mm_mullo_epi32(fixX[2], fixY[0]), _mm_mullo_epi32(fixX[0], fixY[2]));
		__m128i C2 = _mm_sub_epi32(_mm_mullo_epi32(fixX[0], fixY[1]), _mm_mullo_epi32(fixX[1], fixY[0]));

		// Compute triangle area
		__m128i triArea = _mm_sub_epi32(_mm_mullo_epi32(B2, A1), _mm_mullo_epi32(B1, A2));
		__m128 oneOverTriArea = _mm_div_ps(_mm_set1_ps(1.0f), _mm_cvtepi32_ps(triArea));

		// Z setup
		__m128 Z[3];
		Z[0] = xformedvPos[0].Z;
		Z[1] = _mm_mul_ps(_mm_sub_ps(xformedvPos[1].Z, xformedvPos[0].Z), oneOverTriArea);
		Z[2] = _mm_mul_ps(_mm_sub_ps(xformedvPos[2].Z, xformedvPos[0].Z), oneOverTriArea);

		// When we interpolate, beta and gama have already been advanced
		// by one block, so compensate here.
		Z[0] = _mm_sub_ps(Z[0], _mm_mul_ps(_mm_cvtepi32_ps(_mm_slli_epi32(A1, 1)), Z[1]));
		Z[0] = _mm_sub_ps(Z[0], _mm_mul_ps(_mm_cvtepi32_ps(_mm_slli_epi32(A2, 1)), Z[2]));

		// Use bounding box traversal strategy to determine which pixels to rasterize 
		__m128i startX = _mm_and_si128(Max(Min(Min(fixX[0], fixX[1]), fixX[2]), _mm_set1_epi32(tileStartX)), _mm_set1_epi32(0xFFFFFFFE));
		__m128i endX   = Min(Max(Max(fixX[0], fixX[1]), fixX[2]), _mm_set1_epi32(tileEndX));

		__m128i startY = _mm_and_si128(Max(Min(Min(fixY[0], fixY[1]), fixY[2]), _mm_set1_epi32(tileStartY)), _mm_set1_epi32(0xFFFFFFFE));
		__m128i endY   = Min(Max(Max(fixY[0], fixY[1]), fixY[2]), _mm_set1_epi32(tileEndY));

		// Now we have 4 triangles set up.  Rasterize them each individually.
        for(int lane=0; lane < numSimdTris; lane++)
        {
			// Extract this triangle's properties from the SIMD versions
            __m128 zz[3];
			for(int vv = 0; vv < 3; vv++)
			{
				zz[vv] = _mm_set1_ps(Z[vv].m128_f32[lane]);
			}

			int startXx = startX.m128i_i32[lane];
			int endXx	= endX.m128i_i32[lane];
			int startYy = startY.m128i_i32[lane];
			int endYy	= endY.m128i_i32[lane];
		
			__m128i sum = _mm_set1_epi32(triArea.m128i_i32[lane]);

			__m128i aa1 = _mm_set1_epi32(A1.m128i_i32[lane]);
			__m128i aa2 = _mm_set1_epi32(A2.m128i_i32[lane]);

			__m128i bb1 = _mm_set1_epi32(B1.m128i_i32[lane]);
			__m128i bb2 = _mm_set1_epi32(B2.m128i_i32[lane]);

			__m128i cc1 = _mm_set1_epi32(C1.m128i_i32[lane]);
			__m128i cc2 = _mm_set1_epi32(C2.m128i_i32[lane]);

			__m128i aa1Inc = _mm_slli_epi32(aa1, 1);
			__m128i aa2Inc = _mm_slli_epi32(aa2, 1);
			__m128i aa0Dec = _mm_add_epi32(aa1Inc, aa2Inc);

			__m128i row, col;

			// Traverse pixels in 2x2 blocks and store 2x2 pixel quad depths contiguously in memory ==> 2*X
			// This method provides better perfromance
			UINT ofs_x0 = EncodePosX(startXx);
			UINT ofs_x1 = EncodePosX(endXx);
			UINT ofs_y0 = EncodePosY(startYy);
			UINT ofs_y1 = EncodePosY(endYy);

			col = _mm_add_epi32(colOffset, _mm_set1_epi32(startXx));
			__m128i aa1Col = _mm_mullo_epi32(aa1, col);
			__m128i aa2Col = _mm_mullo_epi32(aa2, col);

			row = _mm_add_epi32(rowOffset, _mm_set1_epi32(startYy));
			__m128i bb1Row = _mm_add_epi32(_mm_mullo_epi32(bb1, row), cc1);
			__m128i bb2Row = _mm_add_epi32(_mm_mullo_epi32(bb2, row), cc2);

			__m128i bb1Inc = _mm_slli_epi32(bb1, 1);
			__m128i bb2Inc = _mm_slli_epi32(bb2, 1);

			// Incrementally compute Fab(x, y) for all the pixels inside the bounding box formed by (startX, endX) and (startY, endY)
			for(UINT ofs_y = ofs_y0; ofs_y <= ofs_y1; ofs_y = StepY2(ofs_y))
			{
				// Compute barycentric coordinates 
				float *pDepth = &pDepthBuffer[ofs_y];
				__m128i beta = _mm_add_epi32(aa1Col, bb1Row);
				__m128i gama = _mm_add_epi32(aa2Col, bb2Row);
				__m128i alpha = _mm_sub_epi32(_mm_sub_epi32(sum, beta), gama);

				bb1Row = _mm_add_epi32(bb1Row, bb1Inc);
				bb2Row = _mm_add_epi32(bb2Row, bb2Inc);

				for(UINT ofs_x = ofs_x0; ofs_x <= ofs_x1; ofs_x = StepX2(ofs_x))
				{
					//Test Pixel inside triangle
					__m128i mask = _mm_or_si128(_mm_or_si128(alpha, beta), gama);
					alpha = _mm_sub_epi32(alpha, aa0Dec);
					beta  = _mm_add_epi32(beta, aa1Inc);
					gama  = _mm_add_epi32(gama, aa2Inc);
					
					// Compute barycentric-interpolated depth
			        __m128 depth = zz[0];
					depth = _mm_add_ps(depth, _mm_mul_ps(_mm_cvtepi32_ps(beta), zz[1]));
					depth = _mm_add_ps(depth, _mm_mul_ps(_mm_cvtepi32_ps(gama), zz[2]));
					
					// Update depth buffer
					__m128 previousDepthValue = _mm_load_ps(&pDepth[ofs_x]);
					depth = _mm_max_ps(depth, previousDepthValue);
					depth = _mm_blendv_ps(depth, previousDepthValue, _mm_castsi128_ps(mask));
					_mm_store_ps(&pDepth[ofs_x], depth);
				}//for each column											
			}// for each row
		}// for each triangle
	}// for each set of SIMD# triangles
}