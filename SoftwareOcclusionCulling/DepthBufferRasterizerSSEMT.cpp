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

static const int BlockLog2 = 3;
static const int BlockSize = 1 << BlockLog2;

struct FourEdges
{
	__m128i stepX;
	__m128i stepY;
	__m128i offs;
	__m128i nmin;
	__m128i nmax;

	__forceinline void setup(const __m128i &x0, const __m128i &y0, const __m128i &x1, const __m128i &y1)
	{
		stepX = _mm_sub_epi32(y0, y1);
		stepY = _mm_sub_epi32(x1, x0);

		offs = _mm_sub_epi32(_mm_setzero_si128(), _mm_mullo_epi32(x0, stepX));
		offs = _mm_sub_epi32(offs, _mm_mullo_epi32(y0, stepY));

		// min/max corners
		__m128i zero = _mm_setzero_si128();
		nmin = _mm_add_epi32(_mm_min_epi32(stepX, zero), _mm_min_epi32(stepY, zero));
		nmax = _mm_add_epi32(_mm_max_epi32(stepX, zero), _mm_max_epi32(stepY, zero));

		nmin = _mm_mullo_epi32(nmin, _mm_set1_epi32(1 - BlockSize));
		nmax = _mm_mullo_epi32(nmax, _mm_set1_epi32(1 - BlockSize));
	}

	__forceinline __m128i getOffs(const __m128i &minX, const __m128i &minY) const
	{
		__m128i r = _mm_add_epi32(offs, _mm_mullo_epi32(minX, stepX));
		return _mm_add_epi32(r, _mm_mullo_epi32(minY, stepY));
	}
};

struct StepEdge
{
	__m128i offs, quadX, quadY, line;
};

struct BlockSetup
{
	StepEdge e[3];
	__m128 z[4];
	int pitch;

	__forceinline void setup(const FourEdges edge[], const __m128 Z[], int lane, int depthPitch)
	{
		__m128i colOffs = _mm_setr_epi32(0, 1, 0, 1);
		__m128i rowOffs = _mm_setr_epi32(0, 0, 1, 1);

		for (int i=0; i < 3; i++)
		{
			__m128i stepX = _mm_set1_epi32(edge[i].stepX.m128i_i32[lane]);
			__m128i stepY = _mm_set1_epi32(edge[i].stepY.m128i_i32[lane]);
			e[i].offs = _mm_add_epi32(_mm_mullo_epi32(colOffs, stepX), _mm_mullo_epi32(rowOffs, stepY));
			e[i].quadX = _mm_slli_epi32(stepX, 1);
			e[i].quadY = _mm_slli_epi32(stepY, 1);
			e[i].line = _mm_sub_epi32(e[i].quadY, _mm_slli_epi32(stepX, BlockLog2));
		}

		for (int i=0; i < 4; i++)
			z[i] = _mm_set1_ps(Z[i].m128_f32[lane]);
		z[3] = _mm_add_ps(z[3], z[3]);

		pitch = 2*depthPitch;
	}
};

static __forceinline __m128 BaryDepth(const BlockSetup &setup, const __m128i &beta, const __m128i &gama)
{
	__m128 depth = setup.z[0];
	depth = _mm_add_ps(depth, _mm_mul_ps(_mm_cvtepi32_ps(beta), setup.z[1]));
	depth = _mm_add_ps(depth, _mm_mul_ps(_mm_cvtepi32_ps(gama), setup.z[2]));
	return depth;
}

static __forceinline void DirectTri(float *pDepthRow, int e0, int e1, int e2, const BlockSetup &setup, int w, int h)
{
	__m128i alphRow = _mm_add_epi32(_mm_set1_epi32(e0), setup.e[0].offs);
	__m128i betaRow = _mm_add_epi32(_mm_set1_epi32(e1), setup.e[1].offs);
	__m128i gamaRow = _mm_add_epi32(_mm_set1_epi32(e2), setup.e[2].offs);

	for (int y=0; y < h; y += 2, pDepthRow += setup.pitch)
	{
		float *pDepthEnd = &pDepthRow[w * 2];

		__m128i alph = alphRow;
		__m128i beta = betaRow;
		__m128i gama = gamaRow;
		__m128 depth = BaryDepth(setup, beta, gama);

		for (float *pDepth = pDepthRow; pDepth < pDepthEnd; pDepth += 4, depth = _mm_add_ps(depth, setup.z[3]))
		{
			// Test pixel inside triangle
			__m128i mask = _mm_or_si128(_mm_or_si128(alph, beta), gama);
			alph = _mm_add_epi32(alph, setup.e[0].quadX);
			beta = _mm_add_epi32(beta, setup.e[1].quadX);
			gama = _mm_add_epi32(gama, setup.e[2].quadX);

			// Compute barycentric-interpolated depth
			__m128 prevDepth = _mm_load_ps(pDepth);
			__m128 depthMax  = _mm_max_ps(depth, prevDepth);
			__m128 oDepth = _mm_blendv_ps(depthMax, prevDepth, _mm_castsi128_ps(mask));
			_mm_store_ps(pDepth, oDepth);
		}

		alphRow = _mm_add_epi32(alphRow, setup.e[0].quadY);
		betaRow = _mm_add_epi32(betaRow, setup.e[1].quadY);
		gamaRow = _mm_add_epi32(gamaRow, setup.e[2].quadY);
	}
}

static __forceinline void PartialBlock(float *pDepth, __m128i e, BlockSetup &setup)
{
	__m128i alph = _mm_add_epi32(_mm_shuffle_epi32(e, 0x00), setup.e[0].offs);
	__m128i beta = _mm_add_epi32(_mm_shuffle_epi32(e, 0x55), setup.e[1].offs);
	__m128i gama = _mm_add_epi32(_mm_shuffle_epi32(e, 0xaa), setup.e[2].offs);

	for (int y=0; y < BlockSize; y += 2, pDepth += setup.pitch)
	{
		__m128 depth = BaryDepth(setup, beta, gama);

		for (int x=0; x < BlockSize; x += 2, depth = _mm_add_ps(depth, setup.z[3]))
		{
			// Test pixel inside triangle
			__m128i mask = _mm_or_si128(_mm_or_si128(alph, beta), gama);
			alph = _mm_add_epi32(alph, setup.e[0].quadX);
			beta = _mm_add_epi32(beta, setup.e[1].quadX);
			gama = _mm_add_epi32(gama, setup.e[2].quadX);

			//// Early out if all of this quad's pixels are outside the triangle
			//if(_mm_testc_si128(mask, _mm_set1_epi32(0x80000000)))
			//	continue;

			// Compute barycentric-interpolated depth
			__m128 prevDepth = _mm_load_ps(&pDepth[x*2]);
			__m128 depthMax  = _mm_max_ps(depth, prevDepth);
			__m128 oDepth = _mm_blendv_ps(depthMax, prevDepth, _mm_castsi128_ps(mask));
			_mm_store_ps(&pDepth[x*2], oDepth);
		}

		alph = _mm_add_epi32(alph, setup.e[0].line);
		beta = _mm_add_epi32(beta, setup.e[1].line);
		gama = _mm_add_epi32(gama, setup.e[2].line);
	}
}

static __forceinline void Transpose4x3(__m128i *o, const __m128i &i0, const __m128i &i1, const __m128i &i2)
{
	__m128i a0 = _mm_unpacklo_epi32(i0, i2); // i00 i20 i01 i21
	__m128i a1 = _mm_unpacklo_epi32(i1, i1); // i10 xxx i11 xxx
	__m128i a2 = _mm_unpackhi_epi32(i0, i2); // i02 i22 i03 i23
	__m128i a3 = _mm_unpackhi_epi32(i1, i1); // i12 xxx i13 xxx

	o[0] = _mm_unpacklo_epi32(a0, a1); // i00 i10 i20 i30
	o[1] = _mm_unpackhi_epi32(a0, a1); // i01 i11 i21 i31
	o[2] = _mm_unpacklo_epi32(a2, a3); // i02 i12 i22 i32
	o[3] = _mm_unpackhi_epi32(a2, a3); // i03 i13 i23 i33
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

	float* pDepthBuffer = (float*)mpRenderTargetPixels;

	static const int BLOCKW = (TILE_WIDTH_IN_PIXELS + BlockSize-1) / BlockSize;
	static const int BLOCKH = (TILE_HEIGHT_IN_PIXELS + BlockSize-1) / BlockSize;

	// Based on TaskId determine which tile to process
	UINT screenWidthInTiles = SCREENW/TILE_WIDTH_IN_PIXELS;
    UINT tileX = taskId % screenWidthInTiles;
    UINT tileY = taskId / screenWidthInTiles;

    int tileStartX = tileX * TILE_WIDTH_IN_PIXELS;
	int tileEndX   = min(tileStartX + TILE_WIDTH_IN_PIXELS - 1, SCREENW - 1);
	
	int tileStartY = tileY * TILE_HEIGHT_IN_PIXELS;
	int tileEndY   = min(tileStartY + TILE_HEIGHT_IN_PIXELS - 1, SCREENH - 1);

	if (tileEndX > SCREENW-1) tileEndX = SCREENW-1;
	if (tileEndY > SCREENH-1) tileEndY = SCREENH-1;

	UINT bin = 0;
	UINT binIndex = 0;
	UINT offset1 = YOFFSET1_MT * tileY + XOFFSET1_MT * tileX;
	UINT offset2 = YOFFSET2_MT * tileY + XOFFSET2_MT * tileX;
	UINT numTrisInBin = mpNumTrisInBin[offset1 + bin];

	__m128 vertBuf[4][3];
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

			const TransformedMeshSSE *pMesh = &mpTransformedModels1[modelId].mpMeshes[meshId];
			const __m128 *pVerts = pMesh->mpXformedPos;
			const UINT *pInds = pMesh->mpIndices + triIdx*3;
			for(UINT j=0; j < 3; j++)
				vertBuf[ii][j] = pVerts[pInds[j]];

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

		// transpose input vertex data
		// and generate fixed-point X/Y
		__m128i fixX[3], fixY[3];
		__m128 Z[5];
		for(int i = 0; i < 3; i++)
		{
			__m128 v0 = vertBuf[0][i];
			__m128 v1 = vertBuf[1][i];
			__m128 v2 = vertBuf[2][i];
			__m128 v3 = vertBuf[3][i];

			__m128 a0 = _mm_unpacklo_ps(v0, v2);	// v0x v2x v0y v2y
			__m128 a1 = _mm_unpacklo_ps(v1, v3);	// v1x v3x v1y v3y
			__m128 a2 = _mm_unpackhi_ps(v0, v2);	// v0z v2z v0w v2w
			__m128 a3 = _mm_unpackhi_ps(v1, v3);	// v1z v3z v1w v3w

			__m128 x = _mm_unpacklo_ps(a0, a1);
			__m128 y = _mm_unpackhi_ps(a0, a1);
			__m128 z = _mm_unpacklo_ps(a2, a3);

			fixX[i] = _mm_cvtps_epi32(x);
			fixY[i] = _mm_cvtps_epi32(y);
			Z[i]    = z;
		}

		// Edges
		FourEdges e[3];
		e[0].setup(fixX[1], fixY[1], fixX[2], fixY[2]);
		e[1].setup(fixX[2], fixY[2], fixX[0], fixY[0]);
		e[2].setup(fixX[0], fixY[0], fixX[1], fixY[1]);

		// Compute triangle area
		__m128i triArea = _mm_add_epi32(_mm_add_epi32(e[0].offs, e[1].offs), e[2].offs);
		__m128 oneOverTriArea = _mm_div_ps(_mm_set1_ps(1.0f), _mm_cvtepi32_ps(triArea));

		// Z setup
		Z[1] = _mm_mul_ps(_mm_sub_ps(Z[1], Z[0]), oneOverTriArea);
		Z[2] = _mm_mul_ps(_mm_sub_ps(Z[2], Z[0]), oneOverTriArea);
		Z[3] = _mm_add_ps(_mm_mul_ps(Z[1], _mm_cvtepi32_ps(e[1].stepX)), _mm_mul_ps(Z[2], _mm_cvtepi32_ps(e[2].stepX)));
		Z[4] = _mm_add_ps(_mm_mul_ps(Z[1], _mm_cvtepi32_ps(e[1].stepY)), _mm_mul_ps(Z[2], _mm_cvtepi32_ps(e[2].stepY)));

		__m128 zerof = _mm_setzero_ps();
		__m128 Znmin = _mm_mul_ps(_mm_add_ps(_mm_min_ps(Z[3], zerof), _mm_min_ps(Z[4], zerof)), _mm_set1_ps(BlockSize - 1.0f));
		__m128 Znmax = _mm_mul_ps(_mm_add_ps(_mm_max_ps(Z[3], zerof), _mm_max_ps(Z[4], zerof)), _mm_set1_ps(BlockSize - 1.0f));

		// Bounding rectangle
		__m128i minX = Max(Min(Min(fixX[0], fixX[1]), fixX[2]), _mm_set1_epi32(tileStartX));
		__m128i minY = Max(Min(Min(fixY[0], fixY[1]), fixY[2]), _mm_set1_epi32(tileStartY));
		__m128i maxX = Min(Max(Max(fixX[0], fixX[1]), fixX[2]), _mm_set1_epi32(tileEndX));
		__m128i maxY = Min(Max(Max(fixY[0], fixY[1]), fixY[2]), _mm_set1_epi32(tileEndY));

		// Start in corner of block
		__m128i minXSnap = _mm_and_si128(minX, _mm_set1_epi32(-BlockSize));
		__m128i minYSnap = _mm_and_si128(minY, _mm_set1_epi32(-BlockSize));
		minX = _mm_and_si128(minX, _mm_set1_epi32(-2));
		minY = _mm_and_si128(minY, _mm_set1_epi32(-2));

		// Sizes
		__m128i sizeX = _mm_sub_epi32(maxX, minX);
		__m128i sizeY = _mm_sub_epi32(maxY, minY);
		__m128i sizeMin = _mm_min_epi32(sizeX, sizeY);
		__m128i sizeBig = _mm_cmpgt_epi32(sizeMin, _mm_set1_epi32(2*BlockSize));

		// Edges again (batch-transpose!)
		__m128i edgenmin[4], edgenmax[4], edgeoffs[4], edgecbmax[4], edgestepx[4], edgestepy[4];

		if (!_mm_testc_si128(sizeBig, _mm_set1_epi32(-1))) // any small?
		{
			Transpose4x3(edgeoffs, e[0].getOffs(minX, minY), e[1].getOffs(minX, minY), e[2].getOffs(minX, minY));
		}

		if (!_mm_testz_si128(sizeBig, sizeBig)) // any big?
		{
			__m128i cb0 = _mm_sub_epi32(e[0].getOffs(minXSnap, minYSnap), e[0].nmax);
			__m128i cb1 = _mm_sub_epi32(e[1].getOffs(minXSnap, minYSnap), e[1].nmax);
			__m128i cb2 = _mm_sub_epi32(e[2].getOffs(minXSnap, minYSnap), e[2].nmax);
			Transpose4x3(edgenmin, e[0].nmin, e[1].nmin, e[2].nmin);
			Transpose4x3(edgenmax, e[0].nmax, e[1].nmax, e[2].nmax);
			Transpose4x3(edgecbmax, cb0, cb1, cb2);
			Transpose4x3(edgestepx, e[0].stepX, e[1].stepX, e[2].stepX);
			Transpose4x3(edgestepy, e[0].stepY, e[1].stepY, e[2].stepY);
		}

        // Now we have 4 triangles set up.  Rasterize them each individually.
        for(int lane=0; lane < numSimdTris; lane++)
        {
			// Extract this triangle's properties from the SIMD versions
			int lx0 = minX.m128i_i32[lane];
			int ly0 = minY.m128i_i32[lane];
			int lx1 = maxX.m128i_i32[lane];
			int ly1 = maxY.m128i_i32[lane];


			BlockSetup block;
			block.setup(e, Z, lane, mDepthPitch);

			if (!sizeBig.m128i_i32[lane])
			{
				const __m128i &offs = edgeoffs[lane];

				int rowIdx = ly0 * mDepthPitch + 2 * lx0;
				DirectTri(&pDepthBuffer[rowIdx], offs.m128i_i32[0], offs.m128i_i32[1], offs.m128i_i32[2], block, lx1 - lx0 + 1, ly1 - ly0 + 1);
			}
			else
			{
				lx0 = minXSnap.m128i_i32[lane];
				ly0 = minYSnap.m128i_i32[lane];

				// Prepare for block traversal
				__m128i sign = _mm_set1_epi32(0x80000000);
				__m128i enmin = edgenmin[lane];
				__m128i enmax = edgenmax[lane];
				__m128i endiff = _mm_sub_epi32(enmax, enmin);

				__m128i ex = _mm_slli_epi32(edgestepx[lane], BlockLog2);
				__m128i ey = _mm_slli_epi32(edgestepy[lane], BlockLog2);
				__m128i cbmaxr = edgecbmax[lane];
				float *pDepthRow = &pDepthBuffer[ly0 * mDepthPitch];

				// Loop through block rows
				for (int y0 = ly0; y0 <= ly1; y0 += BlockSize)
				{
					__m128i cbmax = cbmaxr;
					for (int x0 = lx0; x0 <= lx1; x0 += BlockSize, cbmax = _mm_add_epi32(cbmax, ex))
				__m128i alpha = _mm_sub_epi32(_mm_sub_epi32(sum, beta), gama);
					{
						if (!_mm_testz_si128(cbmax, sign))
							continue;

						// Okay, this block is in. Render it.
						float *pDepth = &pDepthRow[x0 * 2];
						__m128i cb = _mm_add_epi32(cbmax, enmax);

						PartialBlock(pDepth, cb, block);
					}

					cbmaxr = _mm_add_epi32(cbmaxr, ey);
					pDepthRow += BlockSize * mDepthPitch;
				}
			}
		}// for each triangle
	}// for each set of SIMD# triangles
}