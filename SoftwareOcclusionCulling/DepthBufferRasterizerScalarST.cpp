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

#include "DepthBufferRasterizerScalarST.h"

DepthBufferRasterizerScalarST::DepthBufferRasterizerScalarST()
	: DepthBufferRasterizerScalar()
{
	int size = SCREENH_IN_TILES * SCREENW_IN_TILES;
	mpBin = new UINT[size * MAX_TRIS_IN_BIN_ST];
	mpBinModel = new USHORT[size * MAX_TRIS_IN_BIN_ST];
	mpBinMesh = new USHORT[size * MAX_TRIS_IN_BIN_ST];
	mpNumTrisInBin = new USHORT[size];
}

DepthBufferRasterizerScalarST::~DepthBufferRasterizerScalarST()
{
	SAFE_DELETE_ARRAY(mpBin);
	SAFE_DELETE_ARRAY(mpBinModel);
	SAFE_DELETE_ARRAY(mpBinMesh);
	SAFE_DELETE_ARRAY(mpNumTrisInBin);
}

//------------------------------------------------------------
// * Determine if the occludee model is inside view frustum
//------------------------------------------------------------
void DepthBufferRasterizerScalarST::IsVisible(CPUTCamera* pCamera)
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
void DepthBufferRasterizerScalarST::TransformModelsAndRasterizeToDepthBuffer()
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
void DepthBufferRasterizerScalarST::TransformMeshes()
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
void DepthBufferRasterizerScalarST::BinTransformedMeshes()
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
void DepthBufferRasterizerScalarST::RasterizeBinnedTrianglesToDepthBuffer(UINT tileId)
{
	int fxptZero = 0;
	float* pDepthBuffer = (float*)mpRenderTargetPixels; 

	// Based on TaskId determine which tile to process
	UINT screenWidthInTiles = SCREENW/TILE_WIDTH_IN_PIXELS;
    UINT tileX = tileId % screenWidthInTiles;
    UINT tileY = tileId / screenWidthInTiles;

    int tileStartX = tileX * TILE_WIDTH_IN_PIXELS;
	int tileEndX   = tileStartX + TILE_WIDTH_IN_PIXELS;
	
	int tileStartY = tileY * TILE_HEIGHT_IN_PIXELS;
	int tileEndY   = tileStartY + TILE_HEIGHT_IN_PIXELS;

	UINT bin = 0;
	UINT binIndex = 0;
	UINT offset1 = YOFFSET1_ST * tileY + XOFFSET1_ST * tileX;
	UINT offset2 = YOFFSET2_ST * tileY + XOFFSET2_ST * tileX;
	UINT numTrisInBin = mpNumTrisInBin[offset1 + bin];

	float4 xformedPos[3];
	bool done = false;
	bool allBinsEmpty = true;
	mNumRasterizedTris[tileId] = numTrisInBin;

	while(!done)
	{
		// Loop through all the bins and process the binned traingles
		while(numTrisInBin <= 0)
		{
			// This bin is empty.  Move to next bin.
			if(++bin >= 1)
			{
				break;
			}
			numTrisInBin = mpNumTrisInBin[offset1 + bin];
			mNumRasterizedTris[tileId] += numTrisInBin;
			binIndex = 0; // Slightly inefficient.  We set it every time through this loop.  Could do only once.
		}
		if(!numTrisInBin)
		{
			break; // No more tris in the bins
		}
		USHORT modelId = mpBinModel[offset2 + bin * MAX_TRIS_IN_BIN_ST + binIndex];
		USHORT meshId = mpBinMesh[offset2 + bin * MAX_TRIS_IN_BIN_ST + binIndex];
		UINT triIdx = mpBin[offset2 + bin * MAX_TRIS_IN_BIN_ST + binIndex];
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
		
		int alpha0 = (A0 * col) + (B0 * row) + C0;
		int beta0 = (A1 * col) + (B1 * row) + C1;
		int gama0 = (A2 * col) + (B2 * row) + C2;

		// Incrementally compute Fab(x, y) for all the pixels inside the bounding box formed by (startX, endX) and (startY, endY)
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