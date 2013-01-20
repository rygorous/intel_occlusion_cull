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

#include "DepthBufferRasterizerScalarMT.h"

DepthBufferRasterizerScalarMT::DepthBufferRasterizerScalarMT()
	: DepthBufferRasterizerScalar()
{
	int size = SCREENH_IN_TILES * SCREENW_IN_TILES *  NUM_XFORMVERTS_TASKS;
	mpBin = new UINT[size * MAX_TRIS_IN_BIN_MT];
	mpBinModel = new USHORT[size * MAX_TRIS_IN_BIN_MT];
	mpBinMesh = new USHORT[size * MAX_TRIS_IN_BIN_MT];
	mpNumTrisInBin = new USHORT[size];
}

DepthBufferRasterizerScalarMT::~DepthBufferRasterizerScalarMT()
{
	SAFE_DELETE_ARRAY(mpBin);
	SAFE_DELETE_ARRAY(mpBinModel);
	SAFE_DELETE_ARRAY(mpBinMesh);
	SAFE_DELETE_ARRAY(mpNumTrisInBin);
}

//-------------------------------------------------------------------------------
// Create tasks to determine if the occluder model is within the viewing frustum 
//-------------------------------------------------------------------------------
void DepthBufferRasterizerScalarMT::IsVisible(CPUTCamera* pCamera)
{
	mpCamera = pCamera;

	gTaskMgr.CreateTaskSet(&DepthBufferRasterizerScalarMT::IsVisible, this, mNumModels1, NULL, 0, "Is Visible", &mIsVisible);
	// Wait for the task set
	gTaskMgr.WaitForSet(mIsVisible);
	// Release the task set
	gTaskMgr.ReleaseHandle(mIsVisible);
	mIsVisible = TASKSETHANDLE_INVALID;
}

void DepthBufferRasterizerScalarMT::IsVisible(VOID *taskData, INT context, UINT taskId, UINT taskCount)
{
	DepthBufferRasterizerScalarMT *pSOCScalar =  (DepthBufferRasterizerScalarMT*)taskData;
	pSOCScalar->IsVisible(taskId, taskCount);
}

//------------------------------------------------------------
// * Determine if the occluder model is inside view frustum
//------------------------------------------------------------
void DepthBufferRasterizerScalarMT::IsVisible(UINT taskId, UINT taskCount)
{
	mpTransformedModels1[taskId].IsVisible(mpCamera);
}

//------------------------------------------------------------------------------
// Create NUM_XFORMVERTS_TASKS to:
// * Transform the occluder models on the CPU
// * Bin the occluder triangles into tiles that the frame buffer is divided into
// * Rasterize the occluder triangles to the CPU depth buffer
//-------------------------------------------------------------------------------
void DepthBufferRasterizerScalarMT::TransformModelsAndRasterizeToDepthBuffer()
{
	mRasterizeTimer.StartTimer();
		
	gTaskMgr.CreateTaskSet(&DepthBufferRasterizerScalarMT::TransformMeshes, this, NUM_XFORMVERTS_TASKS, NULL, 0, "Xform Vertices", &mXformMesh);

	gTaskMgr.CreateTaskSet(&DepthBufferRasterizerScalarMT::BinTransformedMeshes, this, NUM_XFORMVERTS_TASKS, &mXformMesh, 1, "Bin Meshes", &mBinMesh);
	
	gTaskMgr.CreateTaskSet(&DepthBufferRasterizerScalarMT::RasterizeBinnedTrianglesToDepthBuffer, this, NUM_TILES, &mBinMesh, 1, "Raster Tris to DB", &mRasterize);	

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

void DepthBufferRasterizerScalarMT::TransformMeshes(VOID* taskData, INT context, UINT taskId, UINT taskCount)
{
	DepthBufferRasterizerScalarMT *pSOCScalar =  (DepthBufferRasterizerScalarMT*)taskData;
	pSOCScalar->TransformMeshes(taskId, taskCount);
}

//------------------------------------------------------------------------------------------------------------
// This function combines the vertices of all the occluder models in the scene and processes the models/meshes 
// that contain the task's triangle range. It trsanform the occluder vertices once every frame
//------------------------------------------------------------------------------------------------------------
void DepthBufferRasterizerScalarMT::TransformMeshes(UINT taskId, UINT taskCount)
{
	UINT verticesPerTask  = mNumVertices1/taskCount;
	verticesPerTask		  = (mNumVertices1 % taskCount) > 0 ? verticesPerTask + 1 : verticesPerTask;
	UINT startIndex		  = taskId * verticesPerTask;
	
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

void DepthBufferRasterizerScalarMT::BinTransformedMeshes(VOID* taskData, INT context, UINT taskId, UINT taskCount)
{
	DepthBufferRasterizerScalarMT *pDBR =  (DepthBufferRasterizerScalarMT*)taskData;
	pDBR->BinTransformedMeshes(taskId, taskCount);
}

//--------------------------------------------------------------------------------------
// This function combines the triangles of all the occluder models in the scene and processes 
// the models/meshes that contain the task's triangle range. It bins the occluder triangles 
// into tiles once every frame
//--------------------------------------------------------------------------------------
void DepthBufferRasterizerScalarMT::BinTransformedMeshes(UINT taskId, UINT taskCount)
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
	//trianglesPerTask      += (trianglesPerTask % taskCount) != 0 ? trianglesPerTask + 1 : trianglesPerTask;
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


void DepthBufferRasterizerScalarMT::RasterizeBinnedTrianglesToDepthBuffer(VOID* taskData, INT context, UINT taskId, UINT taskCount)
{
	DepthBufferRasterizerScalarMT *pDBR =  (DepthBufferRasterizerScalarMT*)taskData;
	pDBR->RasterizeBinnedTrianglesToDepthBuffer(taskId, taskCount);
}

//-------------------------------------------------------------------------------
// For each tile go through all the bins and process all the triangles in it.
// Rasterize each triangle to the CPU depth buffer. 
//-------------------------------------------------------------------------------
void DepthBufferRasterizerScalarMT::RasterizeBinnedTrianglesToDepthBuffer(UINT taskId, UINT taskCount)
{
	int fxptZero = 0;
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

	float4 xformedPos[3];
	bool done = false;
	bool allBinsEmpty = true;
	mNumRasterizedTris[taskId] = numTrisInBin;

	
	while(!done)
	{
		// Loop through all the bins and process the binned traingles
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
		mpTransformedModels1[modelId].Gather((float*)xformedPos, meshId, triIdx, 0);
		allBinsEmpty = false;
		
		++binIndex;
		--numTrisInBin;
		
		done = bin >= NUM_XFORMVERTS_TASKS;
		
		if(allBinsEmpty)
		{
			return;
		}

		// use fixed-point only for X and Y.  Avoid work for Z and W.
        int4 xformedPosFxPt[3] = {
            int4(xformedPos[0]),
            int4(xformedPos[1]),
            int4(xformedPos[2])
        };

		// Fab(x, y) =     Ax       +       By     +      C              = 0
		// Fab(x, y) = (ya - yb)x   +   (xb - xa)y + (xa * yb - xb * ya) = 0
		// Compute A = (ya - yb) for the 3 line segments that make up each triangle
		int A0 = xformedPosFxPt[1].y - xformedPosFxPt[2].y;
		int A1 = xformedPosFxPt[2].y - xformedPosFxPt[0].y;
		int A2 = xformedPosFxPt[0].y - xformedPosFxPt[1].y;

		// Compute B = (xb - xa) for the 3 line segments that make up each triangle
		int B0 = xformedPosFxPt[2].x - xformedPosFxPt[1].x;
		int B1 = xformedPosFxPt[0].x - xformedPosFxPt[2].x;
		int B2 = xformedPosFxPt[1].x - xformedPosFxPt[0].x;

		// Compute C = (xa * yb - xb * ya) for the 3 line segments that make up each triangle
		int C0 = xformedPosFxPt[1].x * xformedPosFxPt[2].y - xformedPosFxPt[2].x * xformedPosFxPt[1].y;
		int C1 = xformedPosFxPt[2].x * xformedPosFxPt[0].y - xformedPosFxPt[0].x * xformedPosFxPt[2].y;
		int C2 = xformedPosFxPt[0].x * xformedPosFxPt[1].y - xformedPosFxPt[1].x * xformedPosFxPt[0].y;

		// Compute triangle area
		int triArea = A0 * xformedPosFxPt[0].x + B0 * xformedPosFxPt[0].y + C0;
		float oneOverTriArea = (1.0f/float(triArea));

		// Use bounding box traversal strategy to determine which pixels to rasterize 
		int startX = max(min(min(xformedPosFxPt[0].x, xformedPosFxPt[1].x), xformedPosFxPt[2].x), tileStartX) & int(0xFFFFFFFE);
		int endX   = min(max(max(xformedPosFxPt[0].x, xformedPosFxPt[1].x), xformedPosFxPt[2].x) + 1, tileEndX);

		int startY = max(min(min(xformedPosFxPt[0].y, xformedPosFxPt[1].y), xformedPosFxPt[2].y), tileStartY) & int(0xFFFFFFFE);
		int endY   = min(max(max(xformedPosFxPt[0].y, xformedPosFxPt[1].y), xformedPosFxPt[2].y) + 1, tileEndY);

		// Extract this triangle's properties from the SIMD versions
        float zz[3], oneOverW[3];
		for(int vv = 0; vv < 3; vv++)
		{
			zz[vv] = xformedPos[vv].z;
			oneOverW[vv] = xformedPos[vv].w;
		}

		zz[0] *= oneOverTriArea;
		zz[1] *= oneOverTriArea;
		zz[2] *= oneOverTriArea;
			
		int rowIdx = (startY * SCREENW + startX);
		int col = startX;
		int row = startY;
		
		// Incrementally compute Fab(x, y) for all the pixels inside the bounding box formed by (startX, endX) and (startY, endY) 
		int alpha0 = (A0 * col) + (B0 * row) + C0;
		int beta0 = (A1 * col) + (B1 * row) + C1;
		int gama0 = (A2 * col) + (B2 * row) + C2;
				
		for(int r = startY; r < endY; r++,
									  row++,
									  rowIdx = rowIdx + SCREENW,
									  alpha0 += B0,
									  beta0 += B1,
									  gama0 += B2)									 
		{
			// Compute barycentric coordinates 
			int idx = rowIdx;
			int alpha = alpha0;
			int beta = beta0;
			int gama = gama0;
			
			for(int c = startX; c < endX; c++,
   										  idx++,
										  alpha = alpha + A0,
										  beta  = beta  + A1,
										  gama  = gama  + A2)
			{
				//Test Pixel inside triangle
				int mask = fxptZero < (alpha | beta | gama) ? 1 : 0;
					
				// Early out if all of this quad's pixels are outside the triangle.
				if((mask & mask) == 0)
				{
					continue;
				}

				// Compute barycentric-interpolated depth
			    float depth = (alpha * zz[0]) + (beta * zz[1]) + (gama * zz[2]);
				float previousDepthValue = pDepthBuffer[idx];
				
				int depthMask = (depth >= previousDepthValue) ? 1 : 0;
				int finalMask = mask & depthMask;

				depth = finalMask == 1 ? depth : previousDepthValue;
				pDepthBuffer[idx] = depth;
			}//for each column											
		}// for each row
	}// for each triangle
}