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

#include "TransformedMeshSSE.h"

TransformedMeshSSE::TransformedMeshSSE()
	: mNumVertices(0),
	  mNumIndices(0),
	  mNumTriangles(0),
	  mpVertices(NULL),
	  mpIndices(NULL),
	  mpXformedPos(NULL),
	  mVertexStart(0)
{

}

TransformedMeshSSE::~TransformedMeshSSE()
{

}

void TransformedMeshSSE::Initialize(CPUTMeshDX11* pMesh)
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
void TransformedMeshSSE::TransformVertices(__m128 *cumulativeMatrix, 
										   UINT start, 
										   UINT end)
{
	UINT i;
	for(i = start; i <= end; i++)
	{
		mpXformedPos[i] = TransformCoords(&mpVertices[i].position, cumulativeMatrix);
		float oneOverW = 1.0f/max(mpXformedPos[i].m128_f32[3], 0.0000001f);
		mpXformedPos[i] = _mm_mul_ps(mpXformedPos[i], _mm_set1_ps(oneOverW));
		mpXformedPos[i].m128_f32[3] = oneOverW;
	}
}

void TransformedMeshSSE::Gather(vFloat4 pOut[3], UINT triId, UINT numLanes)
{
	for(UINT l = 0; l < numLanes; l++)
	{
		for(UINT i = 0; i < 3; i++)
		{
			UINT index = mpIndices[(triId * 3) + (l * 3) + i];
			pOut[i].X.lane[l] = mpXformedPos[index].m128_f32[0];
			pOut[i].Y.lane[l] = mpXformedPos[index].m128_f32[1];
			pOut[i].Z.lane[l] = mpXformedPos[index].m128_f32[2];
			pOut[i].W.lane[l] = mpXformedPos[index].m128_f32[3];
			
		}
	}
}

//--------------------------------------------------------------------------------
// Bin the screen space transformed triangles into tiles. For single threaded version
//--------------------------------------------------------------------------------
void TransformedMeshSSE::BinTransformedTrianglesST(UINT taskId,
												   UINT modelId,
												   UINT meshId,
												   UINT start,
												   UINT end,
												   UINT* pBin,
												   USHORT* pBinModel,
												   USHORT* pBinMesh,
												   USHORT* pNumTrisInBin)
{
	int numLanes = SSE;
	// working on 4 triangles at a time
	for(UINT index = start; index <= end; index += SSE)
	{
		if(index + SSE > end)
		{
			numLanes = end - index + 1;
		}
		
		// storing x,y,z,w for the 3 vertices of 4 triangles = 4*3*4 = 48
		vFloat4 xformedPos[3];		
		Gather(xformedPos, index, numLanes);
		
		// TODO: Maybe convert to Fixed pt and store it once so that dont have to convert to fixedPt again during rasterization
		vFxPt4 xFormedFxPtPos[3];
		for(int i = 0; i < 3; i++)
		{
			xFormedFxPtPos[i].X = ftoi_round(xformedPos[i].X);
			xFormedFxPtPos[i].Y = ftoi_round(xformedPos[i].Y);
			xFormedFxPtPos[i].Z = ftoi_round(xformedPos[i].Z);
			xFormedFxPtPos[i].W = ftoi_round(xformedPos[i].W);
		}

		// Compute triangle are
		VecS32 A0 = xFormedFxPtPos[1].Y - xFormedFxPtPos[2].Y;
		VecS32 B0 = xFormedFxPtPos[2].X - xFormedFxPtPos[1].X;
		VecS32 C0 = xFormedFxPtPos[1].X * xFormedFxPtPos[2].Y - xFormedFxPtPos[2].X * xFormedFxPtPos[1].Y;

		VecS32 triArea = A0 * xFormedFxPtPos[0].X;
		triArea += B0 * xFormedFxPtPos[0].Y;
		triArea += C0;

		VecF32 oneOverTriArea = VecF32(1.0f) / itof(triArea);

		// Find bounding box for screen space triangle in terms of pixels
		VecS32 vStartX = vmax(vmin(vmin(xFormedFxPtPos[0].X, xFormedFxPtPos[1].X), xFormedFxPtPos[2].X), VecS32(0));
		VecS32 vEndX   = vmin(vmax(vmax(xFormedFxPtPos[0].X, xFormedFxPtPos[1].X), xFormedFxPtPos[2].X) + VecS32(1), VecS32(SCREENW));

        VecS32 vStartY = vmax(vmin(vmin(xFormedFxPtPos[0].Y, xFormedFxPtPos[1].Y), xFormedFxPtPos[2].Y), VecS32(0));
        VecS32 vEndY   = vmin(vmax(vmax(xFormedFxPtPos[0].Y, xFormedFxPtPos[1].Y), xFormedFxPtPos[2].Y) + VecS32(1), VecS32(SCREENH));

		for(int i = 0; i < numLanes; i++)
		{
			// Skip triangle if area is zero 
			if(triArea.lane[i] <= 0) continue;
			
			float oneOverW[3];
			for(int j = 0; j < 3; j++)
			{
				oneOverW[j] = xformedPos[j].W.lane[i];
			}

			// Reject the triangle if any of its verts is behind the nearclip plane
			if(oneOverW[0] > 1.0f || oneOverW[1] > 1.0f || oneOverW[2] > 1.0f) continue;

			// Convert bounding box in terms of pixels to bounding box in terms of tiles
			int startX = max(vStartX.lane[i]/TILE_WIDTH_IN_PIXELS, 0);
			int endX   = min(vEndX.lane[i]/TILE_WIDTH_IN_PIXELS, SCREENW_IN_TILES-1);

			int startY = max(vStartY.lane[i]/TILE_HEIGHT_IN_PIXELS, 0);
			int endY   = min(vEndY.lane[i]/TILE_HEIGHT_IN_PIXELS, SCREENH_IN_TILES-1);

			// Add triangle to the tiles or bins that the bounding box covers
			int row, col;
			for(row = startY; row <= endY; row++)
			{
				int offset1 = YOFFSET1_ST * row;
				int offset2 = YOFFSET2_ST * row;
				for(col = startX; col <= endX; col++)
				{
					int idx1 = offset1 + (XOFFSET1_ST * col) + taskId;
					int idx2 = offset2 + (XOFFSET2_ST * col) + (taskId * MAX_TRIS_IN_BIN_ST) + pNumTrisInBin[idx1];
					pBin[idx2] = index + i;
					pBinModel[idx2] = modelId;
					pBinMesh[idx2] = meshId;
					pNumTrisInBin[idx1] += 1;
				}
			}
		}
	}
}

//--------------------------------------------------------------------------------
// Bin the screen space transformed triangles into tiles. For multi threaded version
//--------------------------------------------------------------------------------
void TransformedMeshSSE::BinTransformedTrianglesMT(UINT taskId,
												   UINT modelId,
												   UINT meshId,
												   UINT start,
												   UINT end,
												   UINT* pBin,
												   USHORT* pBinModel,
												   USHORT* pBinMesh,
												   USHORT* pNumTrisInBin)
{
	int numLanes = SSE;
	// working on 4 triangles at a time
	for(UINT index = start; index <= end; index += SSE)
	{
		if(index + SSE > end)
		{
			numLanes = end - index + 1;
		}
		
		// storing x,y,z,w for the 3 vertices of 4 triangles = 4*3*4 = 48
		vFloat4 xformedPos[3];		
		Gather(xformedPos, index, numLanes);
		
		// TODO: Maybe convert to Fixed pt and store it once so that dont have to convert to fixedPt again during rasterization
		vFxPt4 xFormedFxPtPos[3];
		for(int i = 0; i < 3; i++)
		{
			xFormedFxPtPos[i].X = ftoi_round(xformedPos[i].X);
			xFormedFxPtPos[i].Y = ftoi_round(xformedPos[i].Y);
			xFormedFxPtPos[i].Z = ftoi_round(xformedPos[i].Z);
			xFormedFxPtPos[i].W = ftoi_round(xformedPos[i].W);
		}

		// Compute triangle are
		VecS32 A0 = xFormedFxPtPos[1].Y - xFormedFxPtPos[2].Y;
		VecS32 B0 = xFormedFxPtPos[2].X - xFormedFxPtPos[1].X;
		VecS32 C0 = xFormedFxPtPos[1].X * xFormedFxPtPos[2].Y - xFormedFxPtPos[2].X * xFormedFxPtPos[1].Y;

		VecS32 triArea = A0 * xFormedFxPtPos[0].X;
		triArea += B0 * xFormedFxPtPos[0].Y;
		triArea += C0;

		VecF32 oneOverTriArea = VecF32(1.0f) / itof(triArea);
		
		// Find bounding box for screen space triangle in terms of pixels
		VecS32 vStartX = vmax(vmin(vmin(xFormedFxPtPos[0].X, xFormedFxPtPos[1].X), xFormedFxPtPos[2].X), VecS32(0));
		VecS32 vEndX   = vmin(vmax(vmax(xFormedFxPtPos[0].X, xFormedFxPtPos[1].X), xFormedFxPtPos[2].X) + VecS32(1), VecS32(SCREENW));

        VecS32 vStartY = vmax(vmin(vmin(xFormedFxPtPos[0].Y, xFormedFxPtPos[1].Y), xFormedFxPtPos[2].Y), VecS32(0));
        VecS32 vEndY   = vmin(vmax(vmax(xFormedFxPtPos[0].Y, xFormedFxPtPos[1].Y), xFormedFxPtPos[2].Y) + VecS32(1), VecS32(SCREENH));


		for(int i = 0; i < numLanes; i++)
		{
			// Skip triangle if area is zero 
			if(triArea.lane[i] <= 0) continue;
			
			float oneOverW[3];
			for(int j = 0; j < 3; j++)
			{
				oneOverW[j] = xformedPos[j].W.lane[i];
			}
			
			// Reject the triangle if any of its verts is behind the nearclip plane
			if(oneOverW[0] > 1.0f || oneOverW[1] > 1.0f || oneOverW[2] > 1.0f) continue;

			// Convert bounding box in terms of pixels to bounding box in terms of tiles
			int startX = max(vStartX.lane[i]/TILE_WIDTH_IN_PIXELS, 0);
			int endX   = min(vEndX.lane[i]/TILE_WIDTH_IN_PIXELS, SCREENW_IN_TILES-1);

			int startY = max(vStartY.lane[i]/TILE_HEIGHT_IN_PIXELS, 0);
			int endY   = min(vEndY.lane[i]/TILE_HEIGHT_IN_PIXELS, SCREENH_IN_TILES-1);

			// Add triangle to the tiles or bins that the bounding box covers
			int row, col;
			for(row = startY; row <= endY; row++)
			{
				int offset1 = YOFFSET1_MT * row;
				int offset2 = YOFFSET2_MT * row;
				for(col = startX; col <= endX; col++)
				{
					int idx1 = offset1 + (XOFFSET1_MT * col) + taskId;
					int idx2 = offset2 + (XOFFSET2_MT * col) + (taskId * MAX_TRIS_IN_BIN_MT) + pNumTrisInBin[idx1];
					pBin[idx2] = index + i;
					pBinModel[idx2] = modelId;
					pBinMesh[idx2] = meshId;
					pNumTrisInBin[idx1] += 1;
				}
			}
		}
	}
}


void TransformedMeshSSE::GetOneTriangleData(float* xformedPos, UINT triId, UINT lane)
{
	vFloat4* pOut = (vFloat4*) xformedPos;
	for(int i = 0; i < 3; i++)
	{
		UINT index = mpIndices[(triId * 3) + i];
		(pOut + i)->X.lane[lane] = mpXformedPos[index].m128_f32[0];
		(pOut + i)->Y.lane[lane] = mpXformedPos[index].m128_f32[1];
		(pOut + i)->Z.lane[lane] = mpXformedPos[index].m128_f32[2];
		(pOut + i)->W.lane[lane] = mpXformedPos[index].m128_f32[3];
	}
}