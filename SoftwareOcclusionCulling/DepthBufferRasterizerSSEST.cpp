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
	
	BoxTestSetup setup;
	setup.Init(mViewMatrix, mProjMatrix, viewportMatrix, pCamera, mOccluderSizeThreshold);

	for(UINT i = 0; i < mNumModels1; i++)
	{
		mpTransformedModels1[i].IsVisible(setup);
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
        
        mpTransformedModels1[ss].TransformMeshes(0, thisSurfaceVertexCount - 1, mpCamera);
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

	VecS32 colOffset(0, 1, 0, 1);
	VecS32 rowOffset(0, 0, 1, 1);

	__m128i fxptZero = _mm_setzero_si128();
	float* pDepthBuffer = (float*)mpRenderTargetPixels; 

	// Based on TaskId determine which tile to process
	UINT screenWidthInTiles = SCREENW/TILE_WIDTH_IN_PIXELS;
    UINT tileX = tileId % screenWidthInTiles;
    UINT tileY = tileId / screenWidthInTiles;

    int tileStartX = tileX * TILE_WIDTH_IN_PIXELS;
	int tileEndX   = tileStartX + TILE_WIDTH_IN_PIXELS;
	
	int tileStartY = tileY * TILE_HEIGHT_IN_PIXELS;
	int tileEndY   = tileStartY + TILE_HEIGHT_IN_PIXELS;

	ClearDepthTile(tileStartX, tileStartY, tileEndX, tileEndY);

	UINT bin = 0;
	UINT binIndex = 0;
	UINT offset1 = YOFFSET1_ST * tileY + XOFFSET1_ST * tileX;
	UINT offset2 = YOFFSET2_ST * tileY + XOFFSET2_ST * tileX;
	UINT numTrisInBin = mpNumTrisInBin[offset1 + bin];

	__m128 gatherBuf[4][3];
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
			mpTransformedModels1[modelId].Gather(gatherBuf[ii], meshId, triIdx);
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

		// use fixed-point only for X and Y.
		VecS32 fixX[3], fixY[3];
		VecF32 Z[3];
		for(int i = 0; i < 3; i++)
		{
			// read 4 verts
			__m128 v0 = gatherBuf[0][i];
			__m128 v1 = gatherBuf[1][i];
			__m128 v2 = gatherBuf[2][i];
			__m128 v3 = gatherBuf[3][i];

			// transpose into SoA layout
			_MM_TRANSPOSE4_PS(v0, v1, v2, v3);

			fixX[i] = ftoi_round(VecF32(v0));
			fixY[i] = ftoi_round(VecF32(v1));
			Z[i] = VecF32(v2);
		}

		// Fab(x, y) =     Ax       +       By     +      C              = 0
		// Fab(x, y) = (ya - yb)x   +   (xb - xa)y + (xa * yb - xb * ya) = 0
		// Compute A = (ya - yb) for the 3 line segments that make up each triangle
		VecS32 A0 = fixY[1] - fixY[2];
		VecS32 A1 = fixY[2] - fixY[0];
		VecS32 A2 = fixY[0] - fixY[1];

		// Compute B = (xb - xa) for the 3 line segments that make up each triangle
		VecS32 B0 = fixX[2] - fixX[1];
		VecS32 B1 = fixX[0] - fixX[2];
		VecS32 B2 = fixX[1] - fixX[0];

		// Compute C = (xa * yb - xb * ya) for the 3 line segments that make up each triangle
		VecS32 C0 = fixX[1] * fixY[2] - fixX[2] * fixY[1];
		VecS32 C1 = fixX[2] * fixY[0] - fixX[0] * fixY[2];
		VecS32 C2 = fixX[0] * fixY[1] - fixX[1] * fixY[0];

		// Compute triangle area
		VecS32 triArea = B2 * A1 - B1 * A2;
		VecF32 oneOverTriArea = VecF32(1.0f) / itof(triArea);

		// Use bounding box traversal strategy to determine which pixels to rasterize 
		VecS32 startX = vmax(vmin(vmin(fixX[0], fixX[1]), fixX[2]), VecS32(tileStartX)) & VecS32(~1);
		VecS32 endX   = vmin(vmax(vmax(fixX[0], fixX[1]), fixX[2]) + VecS32(1), VecS32(tileEndX));

		VecS32 startY = vmax(vmin(vmin(fixY[0], fixY[1]), fixY[2]), VecS32(tileStartY)) & VecS32(~1);
		VecS32 endY   = vmin(vmax(vmax(fixY[0], fixY[1]), fixY[2]) + VecS32(1), VecS32(tileEndY));

		// Now we have 4 triangles set up.  Rasterize them each individually.
        for(int lane=0; lane < numSimdTris; lane++)
        {
			// Extract this triangle's properties from the SIMD versions
            VecF32 zz[3];
			for(int vv = 0; vv < 3; vv++)
			{
				zz[vv] = VecF32(Z[vv].lane[lane]);
			}

			VecF32 oneOverTotalArea(oneOverTriArea.lane[lane]);
			zz[1] = (zz[1] - zz[0]) * oneOverTotalArea;
			zz[2] = (zz[2] - zz[0]) * oneOverTotalArea;
			
			int startXx = startX.lane[lane];
			int endXx	= endX.lane[lane];
			int startYy = startY.lane[lane];
			int endYy	= endY.lane[lane];
		
			 // Incrementally compute Fab(x, y) for all the pixels inside the bounding box formed by (startX, endX) and (startY, endY) 
			VecS32 aa0(A0.lane[lane]);
			VecS32 aa1(A1.lane[lane]);
			VecS32 aa2(A2.lane[lane]);

			VecS32 bb0(B0.lane[lane]);
			VecS32 bb1(B1.lane[lane]);
			VecS32 bb2(B2.lane[lane]);

			VecS32 cc0(C0.lane[lane]);
			VecS32 cc1(C1.lane[lane]);
			VecS32 cc2(C2.lane[lane]);

			VecS32 aa0Inc = shiftl<1>(aa0);
			VecS32 aa1Inc = shiftl<1>(aa1);
			VecS32 aa2Inc = shiftl<1>(aa2);

			// Tranverse pixels in 2x2 blocks and store 2x2 pixel quad depthscontiguously in memory ==> 2*X
			// This method provides better perfromance
			int rowIdx = (startYy * SCREENW + 2 * startXx);

			VecS32 col = colOffset + VecS32(startXx);
			VecS32 aa0Col = aa0 * col;
			VecS32 aa1Col = aa1 * col;
			VecS32 aa2Col = aa2 * col;

			VecS32 row = rowOffset + VecS32(startYy);
			VecS32 bb0Row = bb0 * row + cc0;
			VecS32 bb1Row = bb1 * row + cc1;
			VecS32 bb2Row = bb2 * row + cc2;

			VecS32 sum0Row = aa0Col + bb0Row;
			VecS32 sum1Row = aa1Col + bb1Row;
			VecS32 sum2Row = aa2Col + bb2Row;

			VecS32 bb0Inc = shiftl<1>(bb0);
			VecS32 bb1Inc = shiftl<1>(bb1);
			VecS32 bb2Inc = shiftl<1>(bb2);

			for(int r = startYy; r < endYy; r += 2,
											rowIdx = rowIdx + 2 * SCREENW,
											sum0Row += bb0Inc,
											sum1Row += bb1Inc,
											sum2Row += bb2Inc)
			{
				// Compute barycentric coordinates 
				int idx = rowIdx;
				VecS32 alpha = sum0Row;
				VecS32 beta = sum1Row;
				VecS32 gama = sum2Row;

				for(int c = startXx; c < endXx; c += 2,
												idx += 4,
												alpha += aa0Inc,
												beta  += aa1Inc,
												gama  += aa2Inc)
				{
					//Test Pixel inside triangle
					VecS32 mask = alpha | beta | gama;
					
					// Early out if all of this quad's pixels are outside the triangle.
					if(is_all_negative(mask))
						continue;
					
					// Compute barycentric-interpolated depth
			        VecF32 depth = zz[0];
					depth += itof(beta) * zz[1];
					depth += itof(gama) * zz[2];

					VecF32 previousDepthValue = VecF32::load(&pDepthBuffer[idx]);
					VecF32 mergedDepth = vmax(depth, previousDepthValue);
					depth = select(mergedDepth, previousDepthValue, mask);
					depth.store(&pDepthBuffer[idx]);
				}//for each column											
			}// for each row
		}// for each triangle
	}// for each set of SIMD# triangles
}