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
		__m128 xformed = TransformCoords(&mpVertices[i].position, cumulativeMatrix);
		__m128 clampedW = _mm_max_ps(_mm_set1_ps(0.0000001f), _mm_shuffle_ps(xformed, xformed, 0xff));
		__m128 oneOverW = _mm_div_ps(_mm_set1_ps(1.0f), clampedW);
		__m128 xformWith1 = _mm_insert_ps(xformed, _mm_set1_ps(1.0f), 0x30);
		mpXformedPos[i] = _mm_mul_ps(xformWith1, oneOverW);
	}
}

void TransformedMeshSSE::Gather(vFloat4 pOut[3], UINT triId, UINT numLanes)
{
	for(UINT l = 0; l < numLanes; l++)
	{
		for(UINT i = 0; i < 3; i++)
		{
			UINT index = mpIndices[(triId * 3) + (l * 3) + i];
			pOut[i].X.m128_f32[l] = mpXformedPos[index].m128_f32[0];
			pOut[i].Y.m128_f32[l] = mpXformedPos[index].m128_f32[1];
			pOut[i].Z.m128_f32[l] = mpXformedPos[index].m128_f32[2];
			pOut[i].W.m128_f32[l] = mpXformedPos[index].m128_f32[3];
			
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
			xFormedFxPtPos[i].X = _mm_cvtps_epi32(xformedPos[i].X);
			xFormedFxPtPos[i].Y = _mm_cvtps_epi32(xformedPos[i].Y);
			xFormedFxPtPos[i].Z = _mm_cvtps_epi32(xformedPos[i].Z);
			xFormedFxPtPos[i].W = _mm_cvtps_epi32(xformedPos[i].W);
		}

		// Compute triangle area
		__m128i A1 = _mm_sub_epi32(xFormedFxPtPos[2].Y, xFormedFxPtPos[0].Y);
		__m128i A2 = _mm_sub_epi32(xFormedFxPtPos[0].Y, xFormedFxPtPos[1].Y);
		__m128i B1 = _mm_sub_epi32(xFormedFxPtPos[0].X, xFormedFxPtPos[2].X);
		__m128i B2 = _mm_sub_epi32(xFormedFxPtPos[1].X, xFormedFxPtPos[0].X);
		__m128i triArea = _mm_sub_epi32(_mm_mullo_epi32(B2, A1), _mm_mullo_epi32(B1, A2));
		__m128 oneOverTriArea = _mm_div_ps(_mm_set1_ps(1.0f), _mm_cvtepi32_ps(triArea));

		// Find bounding box for screen space triangle in terms of pixels
		__m128i vStartX = Max(Min(Min(xFormedFxPtPos[0].X, xFormedFxPtPos[1].X), xFormedFxPtPos[2].X), _mm_set1_epi32(0));
		__m128i vEndX   = Min(Max(Max(xFormedFxPtPos[0].X, xFormedFxPtPos[1].X), xFormedFxPtPos[2].X), _mm_set1_epi32(SCREENW-1));

        __m128i vStartY = Max(Min(Min(xFormedFxPtPos[0].Y, xFormedFxPtPos[1].Y), xFormedFxPtPos[2].Y), _mm_set1_epi32(0));
        __m128i vEndY   = Min(Max(Max(xFormedFxPtPos[0].Y, xFormedFxPtPos[1].Y), xFormedFxPtPos[2].Y), _mm_set1_epi32(SCREENH-1));


		for(int i = 0; i < numLanes; i++)
		{
			// Skip triangle if area is zero 
			if(triArea.m128i_i32[i] <= 0) continue;
			if(vEndX.m128i_i32[i] < vStartX.m128i_i32[i] || vEndY.m128i_i32[i] < vStartY.m128i_i32[i]) continue;
			
			float oneOverW[3];
			for(int j = 0; j < 3; j++)
			{
				oneOverW[j] = xformedPos[j].W.m128_f32[i];
			}

			// Reject the triangle if any of its verts is behind the nearclip plane
			if(oneOverW[0] > 1.0f || oneOverW[1] > 1.0f || oneOverW[2] > 1.0f) continue;

			// Convert bounding box in terms of pixels to bounding box in terms of tiles
			int startX = max(vStartX.m128i_i32[i]/TILE_WIDTH_IN_PIXELS, 0);
			int endX   = min(vEndX.m128i_i32[i]/TILE_WIDTH_IN_PIXELS, SCREENW_IN_TILES-1);

			int startY = max(vStartY.m128i_i32[i]/TILE_HEIGHT_IN_PIXELS, 0);
			int endY   = min(vEndY.m128i_i32[i]/TILE_HEIGHT_IN_PIXELS, SCREENH_IN_TILES-1);

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
		__m128i fixX[3], fixY[3];
		for(int i = 0; i < 3; i++)
		{
			fixX[i] = _mm_cvtps_epi32(xformedPos[i].X);
			fixY[i] = _mm_cvtps_epi32(xformedPos[i].Y);
		}

		// Compute triangle area
		__m128i A1 = _mm_sub_epi32(fixY[2], fixY[0]);
		__m128i A2 = _mm_sub_epi32(fixY[0], fixY[1]);
		__m128i B1 = _mm_sub_epi32(fixX[0], fixX[2]);
		__m128i B2 = _mm_sub_epi32(fixX[1], fixX[0]);
		__m128i triArea = _mm_sub_epi32(_mm_mullo_epi32(B2, A1), _mm_mullo_epi32(B1, A2));
		__m128 oneOverTriArea = _mm_div_ps(_mm_set1_ps(1.0f), _mm_cvtepi32_ps(triArea));
		
		// Find bounding box for screen space triangle in terms of pixels
		__m128i vStartX = Max(Min(Min(fixX[0], fixX[1]), fixX[2]), _mm_set1_epi32(0));
		__m128i vEndX   = Min(Max(Max(fixX[0], fixX[1]), fixX[2]), _mm_set1_epi32(SCREENW - 1));

        __m128i vStartY = Max(Min(Min(fixY[0], fixY[1]), fixY[2]), _mm_set1_epi32(0));
        __m128i vEndY   = Min(Max(Max(fixY[0], fixY[1]), fixY[2]), _mm_set1_epi32(SCREENH - 1));

		// Skip triangle if:
		// 1. triArea <= 0  <=> 1 > triArea
		// 2. Bounding box intersection with screen is empty
		// 3. Any of the verts are behind near clip plane (oneOverW > 1.0)
		__m128i culled	= _mm_cmpgt_epi32(_mm_set1_epi32(1), triArea);
		__m128i emptyX	= _mm_cmpgt_epi32(vStartX, vEndX);
		__m128i emptyY	= _mm_cmpgt_epi32(vStartY, vEndY);
		__m128  maxW	= _mm_max_ps(_mm_max_ps(xformedPos[0].W, xformedPos[1].W), xformedPos[2].W);
		__m128  nearClip = _mm_cmplt_ps(_mm_set1_ps(1.0f), maxW);

		__m128  skipTri	= _mm_or_ps(_mm_castsi128_ps(_mm_or_si128(emptyX, emptyY)), _mm_or_ps(_mm_castsi128_ps(culled), nearClip));
		int skipMask	= _mm_movemask_ps(skipTri);

		for(int i = 0; i < numLanes; i++, skipMask >>= 1)
		{
			// Should we skip the tri?
			if(skipMask & 1) continue;

			// Convert bounding box in terms of pixels to bounding box in terms of tiles
			int startX = max(vStartX.m128i_i32[i]/TILE_WIDTH_IN_PIXELS, 0);
			int endX   = min(vEndX.m128i_i32[i]/TILE_WIDTH_IN_PIXELS, SCREENW_IN_TILES-1);

			int startY = max(vStartY.m128i_i32[i]/TILE_HEIGHT_IN_PIXELS, 0);
			int endY   = min(vEndY.m128i_i32[i]/TILE_HEIGHT_IN_PIXELS, SCREENH_IN_TILES-1);

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
		(pOut + i)->X.m128_f32[lane] = mpXformedPos[index].m128_f32[0];
		(pOut + i)->Y.m128_f32[lane] = mpXformedPos[index].m128_f32[1];
		(pOut + i)->Z.m128_f32[lane] = mpXformedPos[index].m128_f32[2];
		(pOut + i)->W.m128_f32[lane] = mpXformedPos[index].m128_f32[3];
	}
}