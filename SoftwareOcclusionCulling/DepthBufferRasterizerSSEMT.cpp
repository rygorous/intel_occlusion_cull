//--------------------------------------------------------------------------------------
// Copyright 2013 Intel Corporation
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
#include"HelperMT.h"

DepthBufferRasterizerSSEMT::DepthBufferRasterizerSSEMT()
	: DepthBufferRasterizerSSE()
{
	int size = SCREENH_IN_TILES * SCREENW_IN_TILES *  NUM_XFORMVERTS_TASKS;
	mpBin[0] = new UINT[size * MAX_TRIS_IN_BIN_MT];
	mpBinModel[0] = new USHORT[size * MAX_TRIS_IN_BIN_MT];
	mpBinMesh[0] = new USHORT[size * MAX_TRIS_IN_BIN_MT];
	mpNumTrisInBin[0] = (USHORT*)_aligned_malloc(size * sizeof(USHORT), 64);

	mpBin[1] = new UINT[size * MAX_TRIS_IN_BIN_MT];
	mpBinModel[1] = new USHORT[size * MAX_TRIS_IN_BIN_MT];
	mpBinMesh[1] = new USHORT[size * MAX_TRIS_IN_BIN_MT];
	mpNumTrisInBin[1] = (USHORT*)_aligned_malloc(size * sizeof(USHORT), 64);
}

DepthBufferRasterizerSSEMT::~DepthBufferRasterizerSSEMT()
{
	SAFE_DELETE_ARRAY(mpBin[0]);
	SAFE_DELETE_ARRAY(mpBinModel[0]);
	SAFE_DELETE_ARRAY(mpBinMesh[0]);
	_aligned_free(mpNumTrisInBin[0]);

	SAFE_DELETE_ARRAY(mpBin[1]);
	SAFE_DELETE_ARRAY(mpBinModel[1]);
	SAFE_DELETE_ARRAY(mpBinMesh[1]);
	_aligned_free(mpNumTrisInBin[1]);
}

void DepthBufferRasterizerSSEMT::InsideViewFrustum(VOID *taskData, INT context, UINT taskId, UINT taskCount)
{
	PerTaskData *pTaskData = (PerTaskData*)taskData;
	pTaskData->pDBR->InsideViewFrustum(taskId, taskCount, pTaskData->idx);
}

//------------------------------------------------------------
// * Determine if the occluder model is inside view frustum
//------------------------------------------------------------
void DepthBufferRasterizerSSEMT::InsideViewFrustum(UINT taskId, UINT taskCount, UINT idx)
{
	UINT start, end;
	GetWorkExtent(&start, &end, taskId, taskCount, mNumModels1);

 	BoxTestSetupSSE setup;
    setup.Init(mpViewMatrix[idx], mpProjMatrix[idx], viewportMatrix, mpCamera[idx], mOccluderSizeThreshold); 

	for(UINT i = start; i < end; i++)
	{
		mpTransformedModels1[i].InsideViewFrustum(setup,  idx);
	}
}

void DepthBufferRasterizerSSEMT::TooSmall(VOID *taskData, INT context, UINT taskId, UINT taskCount)
{
	PerTaskData *pTaskData = (PerTaskData*)taskData;
	pTaskData->pDBR->TooSmall(taskId, taskCount, pTaskData->idx);
}

//------------------------------------------------------------
// * Determine if the occluder model is too small in screen space
//------------------------------------------------------------
void DepthBufferRasterizerSSEMT::TooSmall(UINT taskId, UINT taskCount, UINT idx)
{
	UINT start, end;
	GetWorkExtent(&start, &end, taskId, taskCount, mNumModels1);

 	BoxTestSetupSSE setup;
    setup.Init(mpViewMatrix[idx], mpProjMatrix[idx], viewportMatrix, mpCamera[idx], mOccluderSizeThreshold); 

	for(UINT i = start; i < end; i++)
	{
		mpTransformedModels1[i].TooSmall(setup,  idx);
	}
}

void DepthBufferRasterizerSSEMT::ActiveModels(VOID* taskData, INT context, UINT taskId, UINT taskCount)
{
	PerTaskData *pTaskData = (PerTaskData*)taskData;
	pTaskData->pDBR->ActiveModels(taskId, pTaskData->idx);
}

void DepthBufferRasterizerSSEMT::ActiveModels(UINT taskId, UINT idx)
{
	ResetActive(idx);
	for (UINT i = 0; i < mNumModels1; i++)
	{
		if(mpTransformedModels1[i].IsRasterized2DB(idx))
		{
			Activate(i, idx);
		}
	}
}

//------------------------------------------------------------------------------
// Create tasks to determine if the occluder model is within the viewing frustum 
// Create NUM_XFORMVERTS_TASKS to:
// * Transform the occluder models on the CPU
// * Bin the occluder triangles into tiles that the frame buffer is divided into
// * Rasterize the occluder triangles to the CPU depth buffer
//-------------------------------------------------------------------------------
void DepthBufferRasterizerSSEMT::TransformModelsAndRasterizeToDepthBuffer(CPUTCamera *pCamera, UINT idx)
{
	static const unsigned int kNumOccluderVisTasks = 32;

	mTaskData[idx].idx = idx;
	mTaskData[idx].pDBR = this;

	QueryPerformanceCounter(&mStartTime[idx]);
	mpCamera[idx] = pCamera;
	
	if(mEnableFCulling)
	{
		gTaskMgr.CreateTaskSet(&DepthBufferRasterizerSSEMT::InsideViewFrustum, &mTaskData[idx], kNumOccluderVisTasks, NULL, 0, "Is Visible", &gInsideViewFrustum[idx]);
		
		gTaskMgr.CreateTaskSet(&DepthBufferRasterizerSSEMT::ActiveModels, &mTaskData[idx], 1, &gInsideViewFrustum[idx], 1, "IsActive", &gActiveModels[idx]);
		
		gTaskMgr.CreateTaskSet(&DepthBufferRasterizerSSEMT::TransformMeshes, &mTaskData[idx], NUM_XFORMVERTS_TASKS, &gActiveModels[idx], 1, "Xform Vertices", &gXformMesh[idx]);
	}
	else
	{
		gTaskMgr.CreateTaskSet(&DepthBufferRasterizerSSEMT::TooSmall, &mTaskData[idx], kNumOccluderVisTasks, NULL, 0, "TooSmall", &gTooSmall[idx]);
	
		gTaskMgr.CreateTaskSet(&DepthBufferRasterizerSSEMT::ActiveModels, &mTaskData[idx], 1, &gTooSmall[idx], 1, "IsActive", &gActiveModels[idx]);

		gTaskMgr.CreateTaskSet(&DepthBufferRasterizerSSEMT::TransformMeshes, &mTaskData[idx], NUM_XFORMVERTS_TASKS, &gActiveModels[idx], 1, "Xform Vertices", &gXformMesh[idx]);
	}

	gTaskMgr.CreateTaskSet(&DepthBufferRasterizerSSEMT::BinTransformedMeshes, &mTaskData[idx], NUM_XFORMVERTS_TASKS, &gXformMesh[idx], 1, "Bin Meshes", &gBinMesh[idx]);

	gTaskMgr.CreateTaskSet(&DepthBufferRasterizerSSEMT::SortBins, &mTaskData[idx], 1, &gBinMesh[idx], 1, "BinSort", &gSortBins[idx]);
	
	gTaskMgr.CreateTaskSet(&DepthBufferRasterizerSSEMT::RasterizeBinnedTrianglesToDepthBuffer, &mTaskData[idx], NUM_TILES, &gSortBins[idx], 1, "Raster Tris to DB", &gRasterize[idx]);
}

void DepthBufferRasterizerSSEMT::TransformMeshes(VOID* taskData, INT context, UINT taskId, UINT taskCount)
{
	PerTaskData *pTaskData = (PerTaskData*)taskData;
	pTaskData->pDBR->TransformMeshes(taskId, taskCount, pTaskData->idx);
}

//------------------------------------------------------------------------------------------------------------
// This function combines the vertices of all the occluder models in the scene and processes the models/meshes 
// that contain the task's triangle range. It trsanform the occluder vertices once every frame
//------------------------------------------------------------------------------------------------------------
void DepthBufferRasterizerSSEMT::TransformMeshes(UINT taskId, UINT taskCount, UINT idx)
{
	UINT verticesPerTask  = mNumVerticesA[idx]/taskCount;
	verticesPerTask		  = (mNumVerticesA[idx] % taskCount) > 0 ? verticesPerTask + 1 : verticesPerTask;
	UINT startIndex		  = taskId * verticesPerTask;

	UINT remainingVerticesPerTask = verticesPerTask;

	// Now, process all of the surfaces that contain this task's triangle range.
	UINT runningVertexCount = 0;
	for(UINT active = 0; active < mNumModelsA[idx]; active++)
    {
		UINT ss = mpModelIndexA[idx][active];
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

		mpTransformedModels1[ss].TransformMeshes(thisSurfaceStartIndex, thisSurfaceEndIndex, mpCamera[idx], idx);

		remainingVerticesPerTask -= (thisSurfaceEndIndex + 1 - thisSurfaceStartIndex);
        if( remainingVerticesPerTask <= 0 ) break;

		runningVertexCount = newRunningVertexCount;
    }
}

void DepthBufferRasterizerSSEMT::BinTransformedMeshes(VOID* taskData, INT context, UINT taskId, UINT taskCount)
{
	PerTaskData *pTaskData = (PerTaskData*)taskData;
	pTaskData->pDBR->BinTransformedMeshes(taskId, taskCount, pTaskData->idx);
}

//--------------------------------------------------------------------------------------
// This function combines the triangles of all the occluder models in the scene and processes 
// the models/meshes that contain the task's triangle range. It bins the occluder triangles 
// into tiles once every frame
//--------------------------------------------------------------------------------------
void DepthBufferRasterizerSSEMT::BinTransformedMeshes(UINT taskId, UINT taskCount, UINT idx)
{
	// Reset the bin count.  Note the data layout makes this traversal a bit awkward.
    // We can't just use memset() because the last array index isn't what's varying.
    // However, this should make the real use of this structure go faster.
	for(UINT yy = 0; yy < SCREENH_IN_TILES; yy++)
    {
		UINT offset = YOFFSET1_MT * yy;
        for(UINT xx = 0; xx < SCREENW_IN_TILES; xx++)
        {
			UINT index = offset + (XOFFSET1_MT * xx) + (TOFFSET1_MT * taskId);
            mpNumTrisInBin[idx][index] = 0;
	    }
    }

	// Making sure that the #of Tris in each task (except the last one) is a multiple of 4 
	UINT trianglesPerTask  = (mNumTrianglesA[idx] + taskCount - 1)/taskCount;
	trianglesPerTask      += (trianglesPerTask % SSE) != 0 ? SSE - (trianglesPerTask % SSE) : 0;
	
	UINT startIndex		   = taskId * trianglesPerTask;
	
	UINT remainingTrianglesPerTask = trianglesPerTask;

	// Now, process all of the surfaces that contain this task's triangle range.
	UINT runningTriangleCount = 0;
	for(UINT active = 0; active < mNumModelsA[idx]; active++)
    {
		UINT ss = mpModelIndexA[idx][active];
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

       	mpTransformedModels1[ss].BinTransformedTrianglesMT(taskId, ss, thisSurfaceStartIndex, thisSurfaceEndIndex, mpBin[idx], mpBinModel[idx], mpBinMesh[idx], mpNumTrisInBin[idx], idx);

		remainingTrianglesPerTask -= ( thisSurfaceEndIndex + 1 - thisSurfaceStartIndex);
        if( remainingTrianglesPerTask <= 0 ) break;
				
		runningTriangleCount = newRunningTriangleCount;
    }
}

void DepthBufferRasterizerSSEMT::RasterizeBinnedTrianglesToDepthBuffer(VOID* taskData, INT context, UINT taskId, UINT taskCount)
{
	PerTaskData *pTaskData = (PerTaskData*)taskData;
	pTaskData->pDBR->RasterizeBinnedTrianglesToDepthBuffer(taskId, pTaskData->idx);
}

//--------------------------------------------------------------------------------------
// This function sorts the tiles in order of decreasing number of triangles; since the
// scheduler starts tasks roughly in order, the idea is to put the "fat tiles" first
// and leave the small jobs for last. This is to avoid the pathological case where a
// relatively big tile gets picked up late (as the other worker threads are about to
// finish) and rendering effectively completes single-threaded.
//--------------------------------------------------------------------------------------
void DepthBufferRasterizerSSEMT::SortBins(VOID* taskData, INT context, UINT taskId, UINT taskCount)
{
	PerTaskData *pTaskData = (PerTaskData*)taskData;

	// Initialize sequence in sequential order and compute total number of triangles
	// in the bins for each tile
	UINT tileTotalTris[NUM_TILES];
	for(UINT tile = 0; tile < NUM_TILES; tile++)
	{
		pTaskData->pDBR->mTileSequence[pTaskData->idx][tile] = tile;

		UINT numTris = 0;
		for (UINT bin = 0; bin < NUM_XFORMVERTS_TASKS; bin++)
		{
			numTris += pTaskData->pDBR->mpNumTrisInBin[pTaskData->idx][tile * NUM_XFORMVERTS_TASKS + bin];
		}
		tileTotalTris[tile] = numTris;
	}

	// Sort tiles by number of triangles, decreasing.
	std::sort(pTaskData->pDBR->mTileSequence[pTaskData->idx], pTaskData->pDBR->mTileSequence[pTaskData->idx] + NUM_TILES,
		[&](const UINT a, const UINT b){ return tileTotalTris[a] > tileTotalTris[b]; });
}


//-------------------------------------------------------------------------------
// Hierarchical rasterizer helpers
//-------------------------------------------------------------------------------

static const int BlockLog2 = 3;
static const int BlockSize = 1 << BlockLog2;

struct FourEdges
{
	__m128i stepX;
	__m128i stepY;
	__m128i offs;
	__m128i nmax;

	__forceinline void Setup(const __m128i &x0, const __m128i &y0, const __m128i &x1, const __m128i &y1, const __m128i &minX, const __m128i &minY)
	{
		stepX = _mm_sub_epi32(y0, y1);
		stepY = _mm_sub_epi32(x1, x0);
		offs = _mm_add_epi32(_mm_mullo_epi32(_mm_sub_epi32(minX, x0), stepX), _mm_mullo_epi32(_mm_sub_epi32(minY, y0), stepY));

		// max corner
		__m128i zero = _mm_setzero_si128();
		nmax = _mm_add_epi32(_mm_max_epi32(stepX, zero), _mm_max_epi32(stepY, zero));
		nmax = _mm_mullo_epi32(nmax, _mm_set1_epi32(1 - BlockSize));
		offs = _mm_sub_epi32(offs, nmax);
	}
};

struct StepEdge
{
	__m128i offs, quadX, quadY, line;
};

struct BlockSetup
{
	StepEdge e[3];
	__m128 z[5];

	__forceinline void Setup(const FourEdges edge[], const __m128 Z[], int lane)
	{
		__m128i colOffs = _mm_setr_epi32(0, 1, 0, 1);
		__m128i rowOffs = _mm_setr_epi32(0, 0, 1, 1);

		for(int i=0; i < 3; i++)
		{
			__m128i stepX = _mm_set1_epi32(edge[i].stepX.m128i_i32[lane]);
			__m128i stepY = _mm_set1_epi32(edge[i].stepY.m128i_i32[lane]);
			__m128i nmax = _mm_set1_epi32(edge[i].nmax.m128i_i32[lane]);
			e[i].offs = _mm_add_epi32(_mm_mullo_epi32(colOffs, stepX), _mm_mullo_epi32(rowOffs, stepY));
			e[i].offs = _mm_add_epi32(e[i].offs, nmax);
			e[i].quadX = _mm_slli_epi32(stepX, 1);
			e[i].quadY = _mm_slli_epi32(stepY, 1);
			e[i].line = _mm_sub_epi32(e[i].quadY, _mm_slli_epi32(stepX, BlockLog2));
		}

		for(int i=0; i < 4; i++)
			z[i] = _mm_set1_ps(Z[i].m128_f32[lane]);

		z[4] = _mm_add_ps(_mm_mul_ps(z[1], _mm_cvtepi32_ps(e[1].quadY)), _mm_mul_ps(z[2], _mm_cvtepi32_ps(e[2].quadY)));
	}
};

static __forceinline __m128 BaryDepth(const BlockSetup &setup, const __m128i &beta, const __m128i &gama)
{
	__m128 depth = setup.z[0];
	depth = _mm_add_ps(depth, _mm_mul_ps(_mm_cvtepi32_ps(beta), setup.z[1]));
	depth = _mm_add_ps(depth, _mm_mul_ps(_mm_cvtepi32_ps(gama), setup.z[2]));
	return depth;
}

static __forceinline void PartialBlock(float * __restrict pDepth, __m128i e, const BlockSetup &setup, int nrows)
{
	__m128i alph = _mm_add_epi32(_mm_shuffle_epi32(e, 0x00), setup.e[0].offs);
	__m128i beta = _mm_add_epi32(_mm_shuffle_epi32(e, 0x55), setup.e[1].offs);
	__m128i gama = _mm_add_epi32(_mm_shuffle_epi32(e, 0xaa), setup.e[2].offs);
	__m128 depthRow = BaryDepth(setup, beta, gama);

	if (nrows > BlockSize)
		nrows = BlockSize;

	while(nrows > 0)
	{
		__m128 depth = depthRow;

		for (int index=0; index < BlockSize*2; index += 2*2)
		{
			// Test pixel inside triangle
			__m128i mask = _mm_or_si128(_mm_or_si128(alph, beta), gama);

			// Compute barycentric-interpolated depth
			__m128 previousDepthValue = _mm_load_ps(&pDepth[index]);
			__m128 mergedDepth = _mm_max_ps(depth, previousDepthValue);
			__m128 finaldepth = _mm_blendv_ps(mergedDepth, previousDepthValue, _mm_castsi128_ps(mask));
			_mm_store_ps(&pDepth[index], finaldepth);

			// Increment
			alph = _mm_add_epi32(alph, setup.e[0].quadX);
			beta = _mm_add_epi32(beta, setup.e[1].quadX);
			gama = _mm_add_epi32(gama, setup.e[2].quadX);
			depth = _mm_add_ps(depth, setup.z[3]);
		}

		alph = _mm_add_epi32(alph, setup.e[0].line);
		beta = _mm_add_epi32(beta, setup.e[1].line);
		gama = _mm_add_epi32(gama, setup.e[2].line);
		depthRow = _mm_add_ps(depthRow, setup.z[4]);

		nrows -= 2;
		pDepth += SCREENW*2;
	}
}

//-------------------------------------------------------------------------------
// For each tile go through all the bins and process all the triangles in it.
// Rasterize each triangle to the CPU depth buffer. 
//-------------------------------------------------------------------------------
void DepthBufferRasterizerSSEMT::RasterizeBinnedTrianglesToDepthBuffer(UINT rawTaskId, UINT idx)
{
	UINT taskId = mTileSequence[idx][rawTaskId];
	// Set DAZ and FZ MXCSR bits to flush denormals to zero (i.e., make it faster)
	// Denormal are zero (DAZ) is bit 6 and Flush to zero (FZ) is bit 15. 
	// so to enable the two to have to set bits 6 and 15 which 1000 0000 0100 0000 = 0x8040
	_mm_setcsr( _mm_getcsr() | 0x8040 );

	__m128i colOffset = _mm_setr_epi32(0, 1, 0, 1);
	__m128i rowOffset = _mm_setr_epi32(0, 0, 1, 1);

	__m128i fxptZero = _mm_setzero_si128();
	float* pDepthBuffer = (float*)mpRenderTargetPixels[idx]; 

	// Based on TaskId determine which tile to process
	UINT screenWidthInTiles = SCREENW/TILE_WIDTH_IN_PIXELS;
    UINT tileX = taskId % screenWidthInTiles;
    UINT tileY = taskId / screenWidthInTiles;

    int tileStartX = tileX * TILE_WIDTH_IN_PIXELS;
	int tileEndX   = min(tileStartX + TILE_WIDTH_IN_PIXELS - 1, SCREENW - 1);
	
	int tileStartY = tileY * TILE_HEIGHT_IN_PIXELS;
	int tileEndY   = min(tileStartY + TILE_HEIGHT_IN_PIXELS - 1, SCREENH - 1);

	ClearDepthTile(tileStartX, tileStartY, tileEndX + 1, tileEndY + 1, idx);

	UINT bin = 0;
	UINT binIndex = 0;
	UINT offset1 = YOFFSET1_MT * tileY + XOFFSET1_MT * tileX;
	UINT offset2 = YOFFSET2_MT * tileY + XOFFSET2_MT * tileX;
	UINT numTrisInBin = mpNumTrisInBin[idx][offset1 + TOFFSET1_MT * bin];

	__m128 gatherBuf[4][3];
	bool done = false;
	bool allBinsEmpty = true;
	mNumRasterizedTris[idx][taskId] = numTrisInBin;
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
				numTrisInBin = mpNumTrisInBin[idx][offset1 + TOFFSET1_MT * bin];
				mNumRasterizedTris[idx][taskId] += numTrisInBin;
				binIndex = 0;
			}
			if(!numTrisInBin)
			{
				 break; // No more tris in the bins
			}
			USHORT modelId = mpBinModel[idx][offset2 + bin * MAX_TRIS_IN_BIN_MT + binIndex];
			USHORT meshId = mpBinMesh[idx][offset2 + bin * MAX_TRIS_IN_BIN_MT + binIndex];
			UINT triIdx = mpBin[idx][offset2 + bin * MAX_TRIS_IN_BIN_MT + binIndex];
			mpTransformedModels1[modelId].Gather(gatherBuf[ii], meshId, triIdx, idx);
			allBinsEmpty = false;
			numSimdTris++; 

			++binIndex;
			--numTrisInBin;
		}
		done = bin >= NUM_XFORMVERTS_TASKS;
		
		if(allBinsEmpty)
		{
			QueryPerformanceCounter(&mStopTime[idx][taskId]);
			return;
		}

		// use fixed-point only for X and Y.  Avoid work for Z and W.
        __m128i fxPtX[3], fxPtY[3];
		__m128 Z[4];
		for(int i = 0; i < 3; i++)
		{
			// read 4 verts
			__m128 v0 = gatherBuf[0][i];
			__m128 v1 = gatherBuf[1][i];
			__m128 v2 = gatherBuf[2][i];
			__m128 v3 = gatherBuf[3][i];

  			// transpose into SoA layout
			_MM_TRANSPOSE4_PS(v0, v1, v2, v3);
			fxPtX[i] = _mm_cvtps_epi32(v0);
			fxPtY[i] = _mm_cvtps_epi32(v1);
			Z[i] = v2;
		}

		// Use bounding box traversal strategy to determine which pixels to rasterize 
		__m128i minX = Max(Min(Min(fxPtX[0], fxPtX[1]), fxPtX[2]), _mm_set1_epi32(tileStartX));
		__m128i minY = Max(Min(Min(fxPtY[0], fxPtY[1]), fxPtY[2]), _mm_set1_epi32(tileStartY));
		__m128i maxX = Min(Max(Max(fxPtX[0], fxPtX[1]), fxPtX[2]), _mm_set1_epi32(tileEndX));
		__m128i maxY = Min(Max(Max(fxPtY[0], fxPtY[1]), fxPtY[2]), _mm_set1_epi32(tileEndY));

		// Align minX/Y corner on block boundary
		// Regular tris only need to be 2x2 aligned, but if we hit tile edge, need to go 8x8 aligned.
		__m128i hitsMax = _mm_or_si128(_mm_cmpeq_epi32(maxX, _mm_set1_epi32(tileEndX)), _mm_cmpeq_epi32(maxY, _mm_set1_epi32(tileEndY)));
		__m128i coordAlign = _mm_xor_si128(_mm_set1_epi32(-2), _mm_and_si128(hitsMax, _mm_set1_epi32(-2 ^ -8)));
		minX = _mm_and_si128(minX, coordAlign);
		minY = _mm_and_si128(minY, coordAlign);

		// Edges
		FourEdges e[3];
		e[0].Setup(fxPtX[1], fxPtY[1], fxPtX[2], fxPtY[2], minX, minY);
		e[1].Setup(fxPtX[2], fxPtY[2], fxPtX[0], fxPtY[0], minX, minY);
		e[2].Setup(fxPtX[0], fxPtY[0], fxPtX[1], fxPtY[1], minX, minY);

		// Compute triangle area
		__m128i triArea = _mm_mullo_epi32(e[2].stepY, e[1].stepX);
		triArea = _mm_sub_epi32(triArea, _mm_mullo_epi32(e[1].stepY, e[2].stepX));
		__m128 oneOverTriArea = _mm_div_ps(_mm_set1_ps(1.0f), _mm_cvtepi32_ps(triArea));

		// Z setup
		Z[1] = _mm_mul_ps(_mm_sub_ps(Z[1], Z[0]), oneOverTriArea);
		Z[2] = _mm_mul_ps(_mm_sub_ps(Z[2], Z[0]), oneOverTriArea);

		Z[3] = _mm_mul_ps(_mm_cvtepi32_ps(e[1].stepX), Z[1]);
		Z[3] = _mm_add_ps(Z[3], _mm_mul_ps(_mm_cvtepi32_ps(e[2].stepX), Z[2]));
		Z[3] = _mm_add_ps(Z[3], Z[3]); // *2 (quad size in pixels)

        // Now we have 4 triangles set up.  Rasterize them each individually.
        for(int lane=0; lane < numSimdTris; lane++)
        {
			// Extract this triangle's properties from the SIMD versions
			int lx0 = minX.m128i_i32[lane];
			int ly0 = minY.m128i_i32[lane];
			int lx1 = maxX.m128i_i32[lane];
			int ly1 = maxY.m128i_i32[lane];
			float *pDepthRow = &pDepthBuffer[ly0 * SCREENW];

			BlockSetup block;
			block.Setup(e, Z, lane);

			// Prepare for block traversal
			__m128i sign = _mm_set1_epi32(0x80000000);
			__m128i eoffs = _mm_setr_epi32(e[0].offs.m128i_i32[lane], e[1].offs.m128i_i32[lane], e[2].offs.m128i_i32[lane], e[1].offs.m128i_i32[lane]);
			__m128i estepx = _mm_setr_epi32(e[0].stepX.m128i_i32[lane], e[1].stepX.m128i_i32[lane], e[2].stepX.m128i_i32[lane], e[1].stepX.m128i_i32[lane]);
			__m128i estepy = _mm_setr_epi32(e[0].stepY.m128i_i32[lane], e[1].stepY.m128i_i32[lane], e[2].stepY.m128i_i32[lane], e[1].stepY.m128i_i32[lane]);
			__m128i ex = _mm_slli_epi32(estepx, BlockLog2);
			__m128i ey = _mm_slli_epi32(estepy, BlockLog2);
			__m128i cbmaxr = eoffs;

			// Loop through block rows
			for(int y0=ly0; y0 <= ly1; y0 += BlockSize)
			{
				__m128i cbmax = cbmaxr;
				for(int x0=lx0; x0 <= lx1; x0 += BlockSize, cbmax = _mm_add_epi32(cbmax, ex))
				{
					if(!_mm_testz_si128(sign, cbmax))
						continue;

					// Okay, this block is in. Render it.
					PartialBlock(&pDepthRow[x0 * 2], cbmax, block, ly1 - y0 + 1);
				}

				cbmaxr = _mm_add_epi32(cbmaxr, ey);
				pDepthRow += BlockSize * SCREENW;
			}
		}// for each triangle
	}// for each set of SIMD# triangles	
	SummarizeDepthTile(tileStartX, tileStartY, tileEndX + 1, tileEndY + 1, idx);
	QueryPerformanceCounter(&mStopTime[idx][taskId]);
}

void DepthBufferRasterizerSSEMT::ComputeR2DBTime(UINT idx)
{
	LARGE_INTEGER stopTime = mStopTime[idx][0];
	for(UINT i = 0; i < NUM_TILES; i++)
	{
		stopTime = stopTime.QuadPart < mStopTime[idx][i].QuadPart ? mStopTime[idx][i] : stopTime;
	}

	mRasterizeTime[mTimeCounter++] = ((double)(stopTime.QuadPart - mStartTime[idx].QuadPart))/((double)glFrequency.QuadPart);
	mTimeCounter = mTimeCounter >= AVG_COUNTER ? 0 : mTimeCounter;	
}