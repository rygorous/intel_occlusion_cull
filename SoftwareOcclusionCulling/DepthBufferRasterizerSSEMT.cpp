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

#include "DepthBufferRasterizerSSEMT.h"

DepthBufferRasterizerSSEMT::DepthBufferRasterizerSSEMT()
	: DepthBufferRasterizerSSE()
{
	int size = SCREENH_IN_TILES * SCREENW_IN_TILES *  NUM_XFORMVERTS_TASKS;
	mpBin = new UINT[size * MAX_TRIS_IN_BIN_MT];
	mpBinModel = new USHORT[size * MAX_TRIS_IN_BIN_MT];
	mpBinMesh = new USHORT[size * MAX_TRIS_IN_BIN_MT];
	mpNumTrisInBin = new USHORT[size];
}

DepthBufferRasterizerSSEMT::~DepthBufferRasterizerSSEMT()
{
	SAFE_DELETE_ARRAY(mpBin);
	SAFE_DELETE_ARRAY(mpBinModel);
	SAFE_DELETE_ARRAY(mpBinMesh);
	SAFE_DELETE_ARRAY(mpNumTrisInBin);
}

//-------------------------------------------------------------------------------
// Create tasks to determine if the occluder model is within the viewing frustum 
//-------------------------------------------------------------------------------
void DepthBufferRasterizerSSEMT::IsVisible(CPUTCamera* pCamera)
{
	mpCamera = pCamera;
	
	gTaskMgr.CreateTaskSet(&DepthBufferRasterizerSSEMT::IsVisible, this, mNumModels1, NULL, 0, "Is Visible", &mIsVisible);
	// Wait for the task set
	gTaskMgr.WaitForSet(mIsVisible);
	// Release the task set
	gTaskMgr.ReleaseHandle(mIsVisible);
	mIsVisible = TASKSETHANDLE_INVALID;
	
}

void DepthBufferRasterizerSSEMT::IsVisible(VOID *taskData, INT context, UINT taskId, UINT taskCount)
{
	DepthBufferRasterizerSSEMT *pSOCSSE =  (DepthBufferRasterizerSSEMT*)taskData;
	pSOCSSE->IsVisible(taskId, taskCount);
}

//------------------------------------------------------------
// * Determine if the occluder model is inside view frustum
//------------------------------------------------------------
void DepthBufferRasterizerSSEMT::IsVisible(UINT taskId, UINT taskCount)
{
	mpTransformedModels1[taskId].IsVisible(mpCamera);
}

//------------------------------------------------------------------------------
// Create NUM_XFORMVERTS_TASKS to:
// * Transform the occluder models on the CPU
// * Bin the occluder triangles into tiles that the frame buffer is divided into
// * Rasterize the occluder triangles to the CPU depth buffer
//-------------------------------------------------------------------------------
void DepthBufferRasterizerSSEMT::TransformModelsAndRasterizeToDepthBuffer()
{
	mRasterizeTimer.StartTimer();
	
	gTaskMgr.CreateTaskSet(&DepthBufferRasterizerSSEMT::TransformMeshes, this, NUM_XFORMVERTS_TASKS, NULL, 0, "Xform Vertices", &mXformMesh);

	gTaskMgr.CreateTaskSet(&DepthBufferRasterizerSSEMT::BinTransformedMeshes, this, NUM_XFORMVERTS_TASKS, &mXformMesh, 1, "Bin Meshes", &mBinMesh);
	
	gTaskMgr.CreateTaskSet(&DepthBufferRasterizerSSEMT::RasterizeBinnedTrianglesToDepthBuffer, this, NUM_TILES, &mBinMesh, 1, "Raster Tris to DB", &mRasterize);	

	// Wait for the task set
	gTaskMgr.WaitForSet(mRasterize);
	// Release the task set
	gTaskMgr.ReleaseHandle(mXformMesh);
	gTaskMgr.ReleaseHandle(mBinMesh);
	gTaskMgr.ReleaseHandle(mRasterize);
	mXformMesh = mBinMesh = mRasterize = TASKSETHANDLE_INVALID;

	mRasterizeTime[mTimeCounter++] = mRasterizeTimer.StopTimer();
	mTimeCounter = mTimeCounter >= AVG_COUNTER ? 0 : mTimeCounter;

	mNumRasterized = 0;
	for(UINT i = 0; i < mNumModels1; i++)
	{
		mNumRasterized += mpTransformedModels1[i].IsRasterized2DB() ? 1 : 0;
	}
}

void DepthBufferRasterizerSSEMT::TransformMeshes(VOID* taskData, INT context, UINT taskId, UINT taskCount)
{
	DepthBufferRasterizerSSEMT *pSOCSSE =  (DepthBufferRasterizerSSEMT*)taskData;
	pSOCSSE->TransformMeshes(taskId, taskCount);
}

//------------------------------------------------------------------------------------------------------------
// This function combines the vertices of all the occluder models in the scene and processes the models/meshes 
// that contain the task's triangle range. It trsanform the occluder vertices once every frame
//------------------------------------------------------------------------------------------------------------
void DepthBufferRasterizerSSEMT::TransformMeshes(UINT taskId, UINT taskCount)
{
	UINT verticesPerTask  = mNumVertices1/taskCount;
	verticesPerTask		  = (mNumVertices1 % taskCount) > 0 ? verticesPerTask + 1 : verticesPerTask;
	UINT startIndex		  = taskId * verticesPerTask;
	//UINT endIndex		  = taskId == NUM_XFORMVERTS_TASKS - 1 ? mNumVertices1 : startIndex + verticesPerTask;

	UINT remainingVerticesPerTask = verticesPerTask;

	// Now, process all of the surfaces that contain this task's triangle range.
	UINT runningVertexCount = 0;
	for(UINT ss = 0; ss < mNumModels1; ss++)
    {
		UINT thisSurfaceVertexCount = mpTransformedModels1[ss].GetNumVertices();
        
        UINT newRunningVertexCount = runningVertexCount + thisSurfaceVertexCount;
        if( newRunningVertexCount < startIndex )
        {
            // We haven't reached the first surface in our range yet.  Skip to the next surface.
            runningVertexCount = newRunningVertexCount;
            continue;
        }

        // If we got this far, then we need to process this surface.
        UINT thisSurfaceStartIndex = max( 0, (int)startIndex - (int)runningVertexCount );
        UINT thisSurfaceEndIndex   = min( thisSurfaceStartIndex + remainingVerticesPerTask, thisSurfaceVertexCount) - 1;

		mpTransformedModels1[ss].TransformMeshes(mViewMatrix, mProjMatrix, thisSurfaceStartIndex, thisSurfaceEndIndex, mpCamera);

		remainingVerticesPerTask -= (thisSurfaceEndIndex + 1 - thisSurfaceStartIndex);
        if( remainingVerticesPerTask <= 0 ) break;

		runningVertexCount = newRunningVertexCount;
    }
}

void DepthBufferRasterizerSSEMT::BinTransformedMeshes(VOID* taskData, INT context, UINT taskId, UINT taskCount)
{
	DepthBufferRasterizerSSEMT* sample =  (DepthBufferRasterizerSSEMT*)taskData;
	sample->BinTransformedMeshes(taskId, taskCount);
}

//--------------------------------------------------------------------------------------
// This function combines the triangles of all the occluder models in the scene and processes 
// the models/meshes that contain the task's triangle range. It bins the occluder triangles 
// into tiles once every frame
//--------------------------------------------------------------------------------------
void DepthBufferRasterizerSSEMT::BinTransformedMeshes(UINT taskId, UINT taskCount)
{
	// Reset the bin count.  Note the data layout makes this traversal a bit awkward.
    // We can't just use memset() because the last array index isn't what's varying.
    // However, this should make the real use of this structure go faster.
	for(UINT yy = 0; yy < SCREENH_IN_TILES; yy++)
    {
		UINT offset = YOFFSET1_MT * yy;
        for(UINT xx = 0; xx < SCREENW_IN_TILES; xx++)
        {
			UINT index = offset + (XOFFSET1_MT * xx) + taskId;
            mpNumTrisInBin[index] = 0;
	    }
    }

	// Making sure that the #of Tris in each task (except the last one) is a multiple of 4 
	UINT trianglesPerTask  = (mNumTriangles1 + taskCount - 1)/taskCount;
	trianglesPerTask      += (trianglesPerTask % SSE) != 0 ? SSE - (trianglesPerTask % SSE) : 0;
	
	UINT startIndex		   = taskId * trianglesPerTask;
	
	UINT remainingTrianglesPerTask = trianglesPerTask;

	// Now, process all of the surfaces that contain this task's triangle range.
	UINT runningTriangleCount = 0;
	for(UINT ss = 0; ss < mNumModels1; ss++)
    {
		UINT thisSurfaceTriangleCount = mpTransformedModels1[ss].GetNumTriangles();
        
        UINT newRunningTriangleCount = runningTriangleCount + thisSurfaceTriangleCount;
        if( newRunningTriangleCount < startIndex )
        {
            // We haven't reached the first surface in our range yet.  Skip to the next surface.
            runningTriangleCount = newRunningTriangleCount;
            continue;
        }

        // If we got this far, then we need to process this surface.
        UINT thisSurfaceStartIndex = max( 0, (int)startIndex - (int)runningTriangleCount );
        UINT thisSurfaceEndIndex   = min( thisSurfaceStartIndex + remainingTrianglesPerTask, thisSurfaceTriangleCount) - 1;

       	mpTransformedModels1[ss].BinTransformedTrianglesMT(taskId, ss, thisSurfaceStartIndex, thisSurfaceEndIndex, mpBin, mpBinModel, mpBinMesh, mpNumTrisInBin);

		remainingTrianglesPerTask -= ( thisSurfaceEndIndex + 1 - thisSurfaceStartIndex);
        if( remainingTrianglesPerTask <= 0 ) break;
				
		runningTriangleCount = newRunningTriangleCount;
    }
}

void DepthBufferRasterizerSSEMT::RasterizeBinnedTrianglesToDepthBuffer(VOID* taskData, INT context, UINT taskId, UINT taskCount)
{
	DepthBufferRasterizerSSEMT* sample =  (DepthBufferRasterizerSSEMT*)taskData;
	sample->RasterizeBinnedTrianglesToDepthBuffer(taskId, taskCount);
}

//-------------------------------------------------------------------------------
// For each tile go through all the bins and process all the triangles in it.
// Rasterize each triangle to the CPU depth buffer. 
//-------------------------------------------------------------------------------
void DepthBufferRasterizerSSEMT::RasterizeBinnedTrianglesToDepthBuffer(UINT taskId, UINT taskCount)
{
	// Set DAZ and FZ MXCSR bits to flush denormals to zero (i.e., make it faster)
	// Denormal are zero (DAZ) is bit 6 and Flush to zero (FZ) is bit 15. 
	// so to enable the two to have to set bits 6 and 15 which 1000 0000 0100 0000 = 0x8040
	_mm_setcsr( _mm_getcsr() | 0x8040 );

	__m128i colOffset = _mm_set_epi32(0, 1, 0, 1);
	__m128i rowOffset = _mm_set_epi32(0, 0, 1, 1);

	__m128i fxptZero = _mm_setzero_si128();
	float* pDepthBuffer = (float*)mpRenderTargetPixels; 

	// Based on TaskId determine which tile to process
	UINT screenWidthInTiles = SCREENW/TILE_WIDTH_IN_PIXELS;
    UINT tileX = taskId % screenWidthInTiles;
    UINT tileY = taskId / screenWidthInTiles;

    int tileStartX = tileX * TILE_WIDTH_IN_PIXELS;
	int tileEndX   = tileStartX + TILE_WIDTH_IN_PIXELS;
	
	int tileStartY = tileY * TILE_HEIGHT_IN_PIXELS;
	int tileEndY   = tileStartY + TILE_HEIGHT_IN_PIXELS;

	UINT bin = 0;
	UINT binIndex = 0;
	UINT offset1 = YOFFSET1_MT * tileY + XOFFSET1_MT * tileX;
	UINT offset2 = YOFFSET2_MT * tileY + XOFFSET2_MT * tileX;
	UINT numTrisInBin = mpNumTrisInBin[offset1 + bin];

	vFloat4 xformedPos[3];
	bool done = false;
	bool allBinsEmpty = true;
	mNumRasterizedTris[taskId] = numTrisInBin;
	while(!done)
	{
		// Loop through all the bins and process the 4 binned traingles at a time
		UINT ii;
		int numSimdTris = 0;
		for(ii = 0; ii < SSE; ii++)
		{
			while(numTrisInBin <= 0)
			{
				 // This bin is empty.  Move to next bin.
				if(++bin >= NUM_XFORMVERTS_TASKS)
				{
					break;
				}
				numTrisInBin = mpNumTrisInBin[offset1 + bin];
				mNumRasterizedTris[taskId] += numTrisInBin;
				binIndex = 0;
			}
			if(!numTrisInBin)
			{
				 break; // No more tris in the bins
			}
			USHORT modelId = mpBinModel[offset2 + bin * MAX_TRIS_IN_BIN_MT + binIndex];
			USHORT meshId = mpBinMesh[offset2 + bin * MAX_TRIS_IN_BIN_MT + binIndex];
			UINT triIdx = mpBin[offset2 + bin * MAX_TRIS_IN_BIN_MT + binIndex];
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
        vFxPt4 xFormedFxPtPos[3];
		for(int i = 0; i < 3; i++)
		{
			xFormedFxPtPos[i].X = _mm_cvtps_epi32(xformedvPos[i].X);
			xFormedFxPtPos[i].Y = _mm_cvtps_epi32(xformedvPos[i].Y);
			xFormedFxPtPos[i].Z = _mm_cvtps_epi32(xformedvPos[i].Z);
			xFormedFxPtPos[i].W = _mm_cvtps_epi32(xformedvPos[i].W);
		}

		// Fab(x, y) =     Ax       +       By     +      C              = 0
		// Fab(x, y) = (ya - yb)x   +   (xb - xa)y + (xa * yb - xb * ya) = 0
		// Compute A = (ya - yb) for the 3 line segments that make up each triangle
		__m128i A0 = _mm_sub_epi32(xFormedFxPtPos[1].Y, xFormedFxPtPos[2].Y);
		__m128i A1 = _mm_sub_epi32(xFormedFxPtPos[2].Y, xFormedFxPtPos[0].Y);
		__m128i A2 = _mm_sub_epi32(xFormedFxPtPos[0].Y, xFormedFxPtPos[1].Y);

		// Compute B = (xb - xa) for the 3 line segments that make up each triangle
		__m128i B0 = _mm_sub_epi32(xFormedFxPtPos[2].X, xFormedFxPtPos[1].X);
		__m128i B1 = _mm_sub_epi32(xFormedFxPtPos[0].X, xFormedFxPtPos[2].X);
		__m128i B2 = _mm_sub_epi32(xFormedFxPtPos[1].X, xFormedFxPtPos[0].X);

		// Compute C = (xa * yb - xb * ya) for the 3 line segments that make up each triangle
		__m128i C0 = _mm_sub_epi32(_mm_mullo_epi32(xFormedFxPtPos[1].X, xFormedFxPtPos[2].Y), _mm_mullo_epi32(xFormedFxPtPos[2].X, xFormedFxPtPos[1].Y));
		__m128i C1 = _mm_sub_epi32(_mm_mullo_epi32(xFormedFxPtPos[2].X, xFormedFxPtPos[0].Y), _mm_mullo_epi32(xFormedFxPtPos[0].X, xFormedFxPtPos[2].Y));
		__m128i C2 = _mm_sub_epi32(_mm_mullo_epi32(xFormedFxPtPos[0].X, xFormedFxPtPos[1].Y), _mm_mullo_epi32(xFormedFxPtPos[1].X, xFormedFxPtPos[0].Y));

		// Compute triangle area
		__m128i triArea = _mm_mullo_epi32(A0, xFormedFxPtPos[0].X);
		triArea = _mm_add_epi32(triArea, _mm_mullo_epi32(B0, xFormedFxPtPos[0].Y));
		triArea = _mm_add_epi32(triArea, C0);

		__m128 oneOverTriArea = _mm_div_ps(_mm_set1_ps(1.0f), _mm_cvtepi32_ps(triArea));

		// Use bounding box traversal strategy to determine which pixels to rasterize 
		__m128i startX = _mm_and_si128(Max(Min(Min(xFormedFxPtPos[0].X, xFormedFxPtPos[1].X), xFormedFxPtPos[2].X), _mm_set1_epi32(tileStartX)), _mm_set1_epi32(0xFFFFFFFE));
		__m128i endX   = Min(_mm_add_epi32(Max(Max(xFormedFxPtPos[0].X, xFormedFxPtPos[1].X), xFormedFxPtPos[2].X), _mm_set1_epi32(1)), _mm_set1_epi32(tileEndX));

		__m128i startY = _mm_and_si128(Max(Min(Min(xFormedFxPtPos[0].Y, xFormedFxPtPos[1].Y), xFormedFxPtPos[2].Y), _mm_set1_epi32(tileStartY)), _mm_set1_epi32(0xFFFFFFFE));
		__m128i endY   = Min(_mm_add_epi32(Max(Max(xFormedFxPtPos[0].Y, xFormedFxPtPos[1].Y), xFormedFxPtPos[2].Y), _mm_set1_epi32(1)), _mm_set1_epi32(tileEndY));

        // Now we have 4 triangles set up.  Rasterize them each individually.
        for(int lane=0; lane < numSimdTris; lane++)
        {
			// Extract this triangle's properties from the SIMD versions
            __m128 zz[3], oneOverW[3];
			for(int vv = 0; vv < 3; vv++)
			{
				zz[vv] = _mm_set1_ps(xformedvPos[vv].Z.m128_f32[lane]);
				oneOverW[vv] = _mm_set1_ps(xformedvPos[vv].W.m128_f32[lane]);
			}

			__m128 oneOverTotalArea = _mm_set1_ps(oneOverTriArea.m128_f32[lane]);
			zz[0] *= oneOverTotalArea;
			zz[1] *= oneOverTotalArea;
			zz[2] *= oneOverTotalArea;
			
			int startXx = startX.m128i_i32[lane];
			int endXx	= endX.m128i_i32[lane];
			int startYy = startY.m128i_i32[lane];
			int endYy	= endY.m128i_i32[lane];
		
			 // Incrementally compute Fab(x, y) for all the pixels inside the bounding box formed by (startX, endX) and (startY, endY) 
			__m128i aa0 = _mm_set1_epi32(A0.m128i_i32[lane]);
			__m128i aa1 = _mm_set1_epi32(A1.m128i_i32[lane]);
			__m128i aa2 = _mm_set1_epi32(A2.m128i_i32[lane]);

			__m128i bb0 = _mm_set1_epi32(B0.m128i_i32[lane]);
			__m128i bb1 = _mm_set1_epi32(B1.m128i_i32[lane]);
			__m128i bb2 = _mm_set1_epi32(B2.m128i_i32[lane]);

			__m128i cc0 = _mm_set1_epi32(C0.m128i_i32[lane]);
			__m128i cc1 = _mm_set1_epi32(C1.m128i_i32[lane]);
			__m128i cc2 = _mm_set1_epi32(C2.m128i_i32[lane]);

			__m128i aa0Inc = _mm_slli_epi32(aa0, 1);
			__m128i aa1Inc = _mm_slli_epi32(aa1, 1);
			__m128i aa2Inc = _mm_slli_epi32(aa2, 1);

			__m128i row, col;

			int rowIdx;
			// To avoid this branching, choose one method to traverse and store the pixel depth
			if(gVisualizeDepthBuffer)
			{
				// Sequentially traverse and store pixel depths contiguously
				rowIdx = (startYy * SCREENW + startXx);
			}
			else
			{
				// Tranverse pixels in 2x2 blocks and store 2x2 pixel quad depthscontiguously in memory ==> 2*X
				// This method provides better perfromance
				rowIdx = (startYy * SCREENW + 2 * startXx);
			}

			col = _mm_add_epi32(colOffset, _mm_set1_epi32(startXx));
			__m128i aa0Col = _mm_mullo_epi32(aa0, col);
			__m128i aa1Col = _mm_mullo_epi32(aa1, col);
			__m128i aa2Col = _mm_mullo_epi32(aa2, col);

			row = _mm_add_epi32(rowOffset, _mm_set1_epi32(startYy));
			__m128i bb0Row = _mm_add_epi32(_mm_mullo_epi32(bb0, row), cc0);
			__m128i bb1Row = _mm_add_epi32(_mm_mullo_epi32(bb1, row), cc1);
			__m128i bb2Row = _mm_add_epi32(_mm_mullo_epi32(bb2, row), cc2);

			__m128i bb0Inc = _mm_slli_epi32(bb0, 1);
			__m128i bb1Inc = _mm_slli_epi32(bb1, 1);
			__m128i bb2Inc = _mm_slli_epi32(bb2, 1);

			for(int r = startYy; r < endYy; r += 2,
											row  = _mm_add_epi32(row, _mm_set1_epi32(2)),
											rowIdx = rowIdx + 2 * SCREENW,
											bb0Row = _mm_add_epi32(bb0Row, bb0Inc),
											bb1Row = _mm_add_epi32(bb1Row, bb1Inc),
											bb2Row = _mm_add_epi32(bb2Row, bb2Inc))
			{
				// Compute barycentric coordinates 
				int idx = rowIdx;
				__m128i alpha = _mm_add_epi32(aa0Col, bb0Row);
				__m128i beta = _mm_add_epi32(aa1Col, bb1Row);
				__m128i gama = _mm_add_epi32(aa2Col, bb2Row);

				int idxIncr;
				if(gVisualizeDepthBuffer)
				{
					idxIncr = 2;
				}
				else
				{
					idxIncr = 4;
				}
				for(int c = startXx; c < endXx; c += 2,
												idx = idx + idxIncr,
												alpha = _mm_add_epi32(alpha, aa0Inc),
												beta  = _mm_add_epi32(beta, aa1Inc),
												gama  = _mm_add_epi32(gama, aa2Inc))
				{
					//Test Pixel inside triangle
					__m128i mask = _mm_cmplt_epi32(fxptZero, _mm_or_si128(_mm_or_si128(alpha, beta), gama));
					
					// Early out if all of this quad's pixels are outside the triangle.
					if(_mm_test_all_zeros(mask, mask))
					{
						continue;
					}
					
					// Compute barycentric-interpolated depth
			        __m128 depth = _mm_mul_ps(_mm_cvtepi32_ps(alpha), zz[0]);
					depth = _mm_add_ps(depth, _mm_mul_ps(_mm_cvtepi32_ps(beta), zz[1]));
					depth = _mm_add_ps(depth, _mm_mul_ps(_mm_cvtepi32_ps(gama), zz[2]));

					__m128 previousDepthValue;
					if(gVisualizeDepthBuffer)
					{
						previousDepthValue = _mm_set_ps(pDepthBuffer[idx], pDepthBuffer[idx + 1], pDepthBuffer[idx + SCREENW], pDepthBuffer[idx + SCREENW + 1]);
					}
					else
					{
						previousDepthValue = *(__m128*)&pDepthBuffer[idx];
					}

					__m128 depthMask = _mm_cmpge_ps(depth, previousDepthValue);
					__m128i finalMask = _mm_and_si128(mask, _mm_castps_si128(depthMask));
										

					if(gVisualizeDepthBuffer)
					{
						if(finalMask.m128i_i32[3]) pDepthBuffer[idx] = depth.m128_f32[3];
						if(finalMask.m128i_i32[2]) pDepthBuffer[idx + 1] = depth.m128_f32[2];
						if(finalMask.m128i_i32[1]) pDepthBuffer[idx + SCREENW] = depth.m128_f32[1];
						if(finalMask.m128i_i32[0]) pDepthBuffer[idx + SCREENW + 1] = depth.m128_f32[0];
					}
					else
					{
						depth = _mm_blendv_ps(previousDepthValue, depth, _mm_castsi128_ps(finalMask));
						_mm_store_ps(&pDepthBuffer[idx], depth);
					}
				}//for each column											
			}// for each row
		}// for each triangle
	}// for each set of SIMD# triangles
}