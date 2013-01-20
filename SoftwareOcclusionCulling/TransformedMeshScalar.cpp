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

#include "TransformedMeshScalar.h"

TransformedMeshScalar::TransformedMeshScalar()
	: mNumVertices(0),
	  mNumIndices(0),
	  mNumTriangles(0),
	  mpVertices(NULL),
	  mpIndices(NULL),
	  mpXformedPos(NULL),
	  mVertexStart(0)
{

}

TransformedMeshScalar::~TransformedMeshScalar()
{

}

void TransformedMeshScalar::Initialize(CPUTMeshDX11* pMesh)
{
	mNumVertices = pMesh->GetVertexCount();
	mNumIndices  = pMesh->GetIndexCount();
	mNumTriangles = pMesh->GetTriangleCount();
	mpVertices   = pMesh->GetVertices();
	mpIndices    = pMesh->GetIndices();
}

//-------------------------------------------------------------------
// Trasforms the occluder vertices to screen space once every frame
//-------------------------------------------------------------------
void TransformedMeshScalar::TransformVertices(const float4x4& cumulativeMatrix, 
										      UINT start, 
										      UINT end)
{
	for(UINT i = start; i <= end; i++)
	{
		mpXformedPos[i] = TransformCoords(mpVertices[i].pos, cumulativeMatrix);
		float oneOverW = 1.0f/max(mpXformedPos[i].w, 0.0000001f);
		mpXformedPos[i] = mpXformedPos[i] * oneOverW;
		mpXformedPos[i].w = oneOverW;
	}
}

void TransformedMeshScalar::Gather(float4 pOut[3], UINT triId)
{
	for(UINT i = 0; i < 3; i++)
	{
		UINT index = mpIndices[(triId * 3) + i];
		pOut[i] = mpXformedPos[index];	
	}
}

//--------------------------------------------------------------------------------
// Bin the screen space transformed triangles into tiles. For single threaded version
//--------------------------------------------------------------------------------
void TransformedMeshScalar::BinTransformedTrianglesST(UINT taskId,
													  UINT modelId,
													  UINT meshId,
													  UINT start,
													  UINT end,
													  UINT* pBin,
												      USHORT* pBinModel,
													  USHORT* pBinMesh,
													  USHORT* pNumTrisInBin)
{
	// working on one triangle at a time
	for(UINT index = start; index <= end; index++)
	{
		float4 xformedPos[3];
		Gather(xformedPos, index);
			
		// TODO: Maybe convert to Fixed pt and store it once so that dont have to convert to fixedPt again during rasterization
		int4 xFormedFxPtPos[3] = {
			int4(xformedPos[0]),
			int4(xformedPos[1]),
			int4(xformedPos[2]),
		};

		// Compute triangle are
		int A0 = xFormedFxPtPos[1].y - xFormedFxPtPos[2].y;
		int B0 = xFormedFxPtPos[2].x - xFormedFxPtPos[1].x;
		int C0 = (xFormedFxPtPos[1].x * xFormedFxPtPos[2].y) - (xFormedFxPtPos[2].x * xFormedFxPtPos[1].y);
		int triArea = A0 * xFormedFxPtPos[0].x + B0 * xFormedFxPtPos[0].y + C0;
		
		// Find bounding box for screen space triangle in terms of pixels
		int startX = max(min(min(xFormedFxPtPos[0].x, xFormedFxPtPos[1].x), xFormedFxPtPos[2].x), 0); 
        int endX   = min(max(max(xFormedFxPtPos[0].x, xFormedFxPtPos[1].x), xFormedFxPtPos[2].x) + 1, SCREENW);

        int startY = max(min(min(xFormedFxPtPos[0].y, xFormedFxPtPos[1].y), xFormedFxPtPos[2].y), 0 );
        int endY   = min(max(max(xFormedFxPtPos[0].y, xFormedFxPtPos[1].y), xFormedFxPtPos[2].y) + 1, SCREENH);

		// Skip triangle if area is zero 
		if(triArea <= 0) continue;
			
		float oneOverW[3];
		for(int j = 0; j < 3; j++)
		{
			oneOverW[j] = xformedPos[j].w;
		}

		// Reject the triangle if any of its verts is behind the nearclip plane
		if(oneOverW[0] > 1.0f || oneOverW[1] > 1.0f || oneOverW[2] > 1.0f) continue;

		// Convert bounding box in terms of pixels to bounding box in terms of tiles
		int startXx = max(startX/TILE_WIDTH_IN_PIXELS, 0);
		int endXx   = min(endX/TILE_WIDTH_IN_PIXELS, SCREENW_IN_TILES-1);

		int startYy = max(startY/TILE_HEIGHT_IN_PIXELS, 0);
		int endYy   = min(endY/TILE_HEIGHT_IN_PIXELS, SCREENH_IN_TILES-1);

		// Add triangle to the tiles or bins that the bounding box covers
		int row, col;
		for(row = startYy; row <= endYy; row++)
		{
			int offset1 = YOFFSET1_ST * row;
			int offset2 = YOFFSET2_ST * row;
			for(col = startXx; col <= endXx; col++)
			{
				int idx1 = offset1 + (XOFFSET1_ST * col) + taskId;
				int idx2 = offset2 + (XOFFSET2_ST * col) + (taskId * MAX_TRIS_IN_BIN_ST) + pNumTrisInBin[idx1];
				pBin[idx2] = index;
				pBinModel[idx2] = modelId;
				pBinMesh[idx2] = meshId;
				pNumTrisInBin[idx1] += 1;
			}
		}
	}
}

//--------------------------------------------------------------------------------
// Bin the screen space transformed triangles into tiles. For multi threaded version
//--------------------------------------------------------------------------------
void TransformedMeshScalar::BinTransformedTrianglesMT(UINT taskId,
													  UINT modelId,
													  UINT meshId,
													  UINT start,
													  UINT end,
													  UINT* pBin,
												      USHORT* pBinModel,
													  USHORT* pBinMesh,
													  USHORT* pNumTrisInBin)
{
	// working on 4 triangles at a time
	for(UINT index = start; index <= end; index++)
	{
		float4 xformedPos[3];
		Gather(xformedPos, index);
			
		// TODO: Maybe convert to Fixed pt and store it once so that dont have to convert to fixedPt again during rasterization
		int4 xFormedFxPtPos[3] = {
			int4(xformedPos[0]),
			int4(xformedPos[1]),
			int4(xformedPos[2]),
		};

		// Compute triangle are
		int A0 = xFormedFxPtPos[1].y - xFormedFxPtPos[2].y;
		int B0 = xFormedFxPtPos[2].x - xFormedFxPtPos[1].x;
		int C0 = (xFormedFxPtPos[1].x * xFormedFxPtPos[2].y) - (xFormedFxPtPos[2].x * xFormedFxPtPos[1].y);
		int triArea = A0 * xFormedFxPtPos[0].x + B0 * xFormedFxPtPos[0].y + C0;
		
		// Find bounding box for screen space triangle in terms of pixels
		int startX = max(min(min(xFormedFxPtPos[0].x, xFormedFxPtPos[1].x), xFormedFxPtPos[2].x), 0);
        int endX   = min(max(max(xFormedFxPtPos[0].x, xFormedFxPtPos[1].x), xFormedFxPtPos[2].x) + 1, SCREENW);

        int startY = max(min(min(xFormedFxPtPos[0].y, xFormedFxPtPos[1].y), xFormedFxPtPos[2].y), 0 );
        int endY   = min(max(max(xFormedFxPtPos[0].y, xFormedFxPtPos[1].y), xFormedFxPtPos[2].y) + 1, SCREENH);

		// Skip triangle if area is zero 
		if(triArea <= 0) continue;
			
		float oneOverW[3];
		for(int j = 0; j < 3; j++)
		{
			oneOverW[j] = xformedPos[j].w;
		}

		// Reject the triangle if any of its verts is behind the nearclip plane
		if(oneOverW[0] > 1.0f || oneOverW[1] > 1.0f || oneOverW[2] > 1.0f) continue;

		// Convert bounding box in terms of pixels to bounding box in terms of tiles
		int startXx = max(startX/TILE_WIDTH_IN_PIXELS, 0);
		int endXx   = min(endX/TILE_WIDTH_IN_PIXELS, SCREENW_IN_TILES-1);

		int startYy = max(startY/TILE_HEIGHT_IN_PIXELS, 0);
		int endYy   = min(endY/TILE_HEIGHT_IN_PIXELS, SCREENH_IN_TILES-1);

		// Add triangle to the tiles or bins that the bounding box covers
		int row, col;
		for(row = startYy; row <= endYy; row++)
		{
			int offset1 = YOFFSET1_MT * row;
			int offset2 = YOFFSET2_MT * row;
			for(col = startXx; col <= endXx; col++)
			{
				int idx1 = offset1 + (XOFFSET1_MT * col) + taskId;
				int idx2 = offset2 + (XOFFSET2_MT * col) + (taskId * MAX_TRIS_IN_BIN_MT) + pNumTrisInBin[idx1];
				pBin[idx2] = index;
				pBinModel[idx2] = modelId;
				pBinMesh[idx2] = meshId;
				pNumTrisInBin[idx1] += 1;
			}
		}
	}
}

void TransformedMeshScalar::GetOneTriangleData(float* xformedPos, UINT triId, UINT lane)
{
	float4* pOut = (float4*) xformedPos;
	for(int i = 0; i < 3; i++)
	{
		UINT index = mpIndices[(triId * 3) + i];
		(pOut + i)->x = mpXformedPos[index].x;
		(pOut + i)->y = mpXformedPos[index].y;
		(pOut + i)->z = mpXformedPos[index].z;
		(pOut + i)->w = mpXformedPos[index].w;
	}
}