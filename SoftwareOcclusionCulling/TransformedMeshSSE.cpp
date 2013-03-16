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
#include "DepthBufferRasterizerSSE.h"

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
	mNumVertices = pMesh->GetDepthVertexCount();
	mNumIndices  = pMesh->GetIndexCount();
	mNumTriangles = pMesh->GetTriangleCount();
	mpVertices   = pMesh->GetDepthVertices();
	mpIndices    = pMesh->GetDepthIndices();
}

//-------------------------------------------------------------------
// Trasforms the occluder vertices to screen space once every frame
//-------------------------------------------------------------------
void TransformedMeshSSE::TransformVertices(__m128 *cumulativeMatrix, 
										   UINT start, 
										   UINT end)
{
	__m128 row0 = cumulativeMatrix[0];
	__m128 row1 = cumulativeMatrix[1];
	__m128 row2 = cumulativeMatrix[2];
	__m128 row3 = cumulativeMatrix[3];
	Vertex * const inPos = mpVertices;
	__m128 * __restrict outPos = mpXformedPos;

	for(UINT i = start; i <= end; i++)
	{
		__m128 xform = row3;
		xform += row0 * _mm_load1_ps(&inPos[i].pos.x);
		xform += row1 * _mm_load1_ps(&inPos[i].pos.y);
		xform += row2 * _mm_load1_ps(&inPos[i].pos.z);

		__m128 vertZ = _mm_shuffle_ps(xform, xform, 0xaa);
		__m128 vertW = _mm_shuffle_ps(xform, xform, 0xff);
		__m128 projected = _mm_div_ps(xform, vertW);

		// set to all-0 if near-clipped
		__m128 mNoNearClip = _mm_cmple_ps(vertZ, vertW);
		outPos[i] = _mm_and_ps(projected, mNoNearClip);
	}
}

void TransformedMeshSSE::Gather(vFloat4 pOut[3], UINT triId, UINT numLanes)
{
	const UINT *pInd0 = &mpIndices[triId * 3];
	const UINT *pInd1 = pInd0 + (numLanes > 1 ? 3 : 0);
	const UINT *pInd2 = pInd0 + (numLanes > 2 ? 6 : 0);
	const UINT *pInd3 = pInd0 + (numLanes > 3 ? 9 : 0);

	for(UINT i = 0; i < 3; i++)
	{
		__m128 v0 = mpXformedPos[pInd0[i]];	// x0 y0 z0 w0
		__m128 v1 = mpXformedPos[pInd1[i]];	// x1 y1 z1 w1
		__m128 v2 = mpXformedPos[pInd2[i]];	// x2 y2 z2 w2
		__m128 v3 = mpXformedPos[pInd3[i]];	// x3 y3 z3 w3
		_MM_TRANSPOSE4_PS(v0, v1, v2, v3);
		pOut[i].X = VecF32(v0);
		pOut[i].Y = VecF32(v1);
		pOut[i].Z = VecF32(v2);
		pOut[i].W = VecF32(v3);
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
												   BinTriangle* pBin,
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
		
		VecS32 fixX[3], fixY[3];
		VecS32 vXY[3];
		VecF32 vZ[3];
		for(int i = 0; i < 3; i++)
		{
			fixX[i] = ftoi_round(xformedPos[i].X);
			fixY[i] = ftoi_round(xformedPos[i].Y);

			__m128i inter0 = _mm_unpacklo_epi32(fixX[i].simd, fixY[i].simd);
			__m128i inter1 = _mm_unpackhi_epi32(fixX[i].simd, fixY[i].simd);

			vXY[i] = VecS32(_mm_packs_epi32(inter0, inter1));
			vZ[i] = xformedPos[i].Z;
		}

		// Compute triangle area
		VecS32 triArea = (fixX[1] - fixX[0]) * (fixY[2] - fixY[0]) - (fixX[0] - fixX[2]) * (fixY[0] - fixY[1]);
		VecF32 oneOverTriArea = VecF32(1.0f) / itof(triArea);

		// Z setup
		vZ[1] = (vZ[1] - vZ[0]) * oneOverTriArea;
		vZ[2] = (vZ[2] - vZ[0]) * oneOverTriArea;

		// Find bounding box for screen space triangle in terms of pixels
		VecS32 vStartX = vmax(vmin(vmin(fixX[0], fixX[1]), fixX[2]), VecS32(0));
		VecS32 vEndX   = vmin(vmax(vmax(fixX[0], fixX[1]), fixX[2]), VecS32(SCREENW - 1));

        VecS32 vStartY = vmax(vmin(vmin(fixY[0], fixY[1]), fixY[2]), VecS32(0));
        VecS32 vEndY   = vmin(vmax(vmax(fixY[0], fixY[1]), fixY[2]), VecS32(SCREENH - 1));

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
			if(oneOverW[0] == 0.0f || oneOverW[1] == 0.0f || oneOverW[2] == 0.0f) continue;

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

					BinTriangle *pTri = pBin + idx2;
					pTri->vert[0].xy = vXY[0].lane[i];
					pTri->vert[1].xy = vXY[1].lane[i];
					pTri->vert[2].xy = vXY[2].lane[i];
					pTri->Z[0] = vZ[0].lane[i];
					pTri->Z[1] = vZ[1].lane[i];
					pTri->Z[2] = vZ[2].lane[i];

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
												   BinTriangle* pBin,
												   USHORT* pNumTrisInBin)
{
	int numLanes = SSE;
	int laneMask = (1 << numLanes) - 1;

	// working on 4 triangles at a time
	for(UINT index = start; index <= end; index += SSE)
	{
		if(index + SSE > end)
		{
			numLanes = end - index + 1;
			laneMask = (1 << numLanes) - 1;
		}
		
		// storing x,y,z,w for the 3 vertices of 4 triangles = 4*3*4 = 48
		vFloat4 xformedPos[3];		
		Gather(xformedPos, index, numLanes);
		
		VecS32 fixX[3], fixY[3];
		VecS32 vXY[3];
		VecF32 vZ[3];
		for(int i = 0; i < 3; i++)
		{
			fixX[i] = ftoi_round(xformedPos[i].X);
			fixY[i] = ftoi_round(xformedPos[i].Y);

			__m128i inter0 = _mm_unpacklo_epi32(fixX[i].simd, fixY[i].simd);
			__m128i inter1 = _mm_unpackhi_epi32(fixX[i].simd, fixY[i].simd);

			vXY[i] = VecS32(_mm_packs_epi32(inter0, inter1));
			vZ[i] = xformedPos[i].Z;
		}

		// Compute triangle area
		VecS32 triArea = (fixX[1] - fixX[0]) * (fixY[2] - fixY[0]) - (fixX[0] - fixX[2]) * (fixY[0] - fixY[1]);
		VecF32 oneOverTriArea = VecF32(1.0f) / itof(triArea);

		// Z setup
		vZ[1] = (vZ[1] - vZ[0]) * oneOverTriArea;
		vZ[2] = (vZ[2] - vZ[0]) * oneOverTriArea;

		// Find bounding box for screen space triangle in terms of pixels
		VecS32 vStartX = vmax(vmin(vmin(fixX[0], fixX[1]), fixX[2]), VecS32(0));
		VecS32 vEndX   = vmin(vmax(vmax(fixX[0], fixX[1]), fixX[2]), VecS32(SCREENW - 1));

        VecS32 vStartY = vmax(vmin(vmin(fixY[0], fixY[1]), fixY[2]), VecS32(0));
        VecS32 vEndY   = vmin(vmax(vmax(fixY[0], fixY[1]), fixY[2]), VecS32(SCREENH - 1));

		// Figure out which lanes are active
		VecS32 mFront = cmpgt(triArea, VecS32::zero());
		VecS32 mNonemptyX = cmpgt(vEndX, vStartX);
		VecS32 mNonemptyY = cmpgt(vEndY, vStartY);
		VecF32 mAccept1 = bits2float(mFront & mNonemptyX & mNonemptyY);

		// All verts must be inside the near clip volume
		VecF32 mW0 = cmpgt(xformedPos[0].W, VecF32::zero());
		VecF32 mW1 = cmpgt(xformedPos[1].W, VecF32::zero());
		VecF32 mW2 = cmpgt(xformedPos[2].W, VecF32::zero());

		VecF32 mAccept = and(and(mAccept1, mW0), and(mW1, mW2));
		unsigned int triMask = _mm_movemask_ps(mAccept.simd) & laneMask;

		while(triMask)
		{
			int i = FindClearLSB(&triMask);

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
					int idx1 = offset1 + (XOFFSET1_MT * col) + (TOFFSET1_MT * taskId);
					int idx2 = offset2 + (XOFFSET2_MT * col) + (taskId * MAX_TRIS_IN_BIN_MT) + pNumTrisInBin[idx1];

					BinTriangle *pTri = pBin + idx2;
					pTri->vert[0].xy = vXY[0].lane[i];
					pTri->vert[1].xy = vXY[1].lane[i];
					pTri->vert[2].xy = vXY[2].lane[i];
					pTri->Z[0] = vZ[0].lane[i];
					pTri->Z[1] = vZ[1].lane[i];
					pTri->Z[2] = vZ[2].lane[i];

					pNumTrisInBin[idx1] += 1;
				}
			}
		}
	}
}
