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

#include "TransformedAABBoxSSE.h"

static const UINT sBBIndexList[AABB_INDICES] =
{
	// index for top 
	1, 3, 2,
	0, 3, 1,

	// index for bottom
	5, 7, 4,
	6, 7, 5,

	// index for left
	1, 7, 6,
	2, 7, 1,

	// index for right
	3, 5, 4,
	0, 5, 3,

	// index for back
	2, 4, 7,
	3, 4, 2,

	// index for front
	0, 6, 5,
	1, 6, 0,
};

// 0 = use min corner, 1 = use max corner
static const UINT sBBxInd[AABB_VERTICES] = { 1, 0, 0, 1, 1, 1, 0, 0 };
static const UINT sBByInd[AABB_VERTICES] = { 1, 1, 1, 1, 0, 0, 0, 0 };
static const UINT sBBzInd[AABB_VERTICES] = { 1, 1, 0, 0, 0, 1, 1, 0 };

//--------------------------------------------------------------------------
// Get the bounding box center and half vector
// Create the vertex and index list for the triangles that make up the bounding box
//--------------------------------------------------------------------------
void TransformedAABBoxSSE::CreateAABBVertexIndexList(CPUTModelDX11 *pModel)
{
	mWorldMatrix = *pModel->GetWorldMatrix();

	pModel->GetBoundsObjectSpace(&mBBCenter, &mBBHalf);
	mRadiusSq = mBBHalf.lengthSq();
}

//----------------------------------------------------------------------------
// Determine if the occluddee size is too small and if so avoid drawing it
//----------------------------------------------------------------------------
bool TransformedAABBoxSSE::IsTooSmall(const BoxTestSetup &setup, __m128 cumulativeMatrix[4])
{
	__m128 worldMatrix[4];
	worldMatrix[0] = _mm_loadu_ps(mWorldMatrix.r0.f);
	worldMatrix[1] = _mm_loadu_ps(mWorldMatrix.r1.f);
	worldMatrix[2] = _mm_loadu_ps(mWorldMatrix.r2.f);
	worldMatrix[3] = _mm_loadu_ps(mWorldMatrix.r3.f);
	MatrixMultiply(worldMatrix, setup.mViewProjViewport, cumulativeMatrix);

	float w = mBBCenter.x * cumulativeMatrix[0].m128_f32[3] +
		mBBCenter.y * cumulativeMatrix[1].m128_f32[3] +
		mBBCenter.z * cumulativeMatrix[2].m128_f32[3] +
		cumulativeMatrix[3].m128_f32[3];

	if( w > 1.0f )
	{
		return mRadiusSq < w * setup.radiusThreshold;
	}

	return false;
}

//----------------------------------------------------------------
// Trasforms the AABB vertices to screen space once every frame
//----------------------------------------------------------------
bool TransformedAABBoxSSE::TransformAABBox(__m128 xformedPos[], const __m128 cumulativeMatrix[4])
{
	// w ends up being garbage, but it doesn't matter - we ignore it anyway.
	__m128 vCenter = _mm_loadu_ps(&mBBCenter.x);
	__m128 vHalf   = _mm_loadu_ps(&mBBHalf.x);

	__m128 vMin    = _mm_sub_ps(vCenter, vHalf);
	__m128 vMax    = _mm_add_ps(vCenter, vHalf);

	// transforms
	__m128 xRow[2], yRow[2], zRow[2];
	xRow[0] = _mm_shuffle_ps(vMin, vMin, 0x00) * cumulativeMatrix[0];
	xRow[1] = _mm_shuffle_ps(vMax, vMax, 0x00) * cumulativeMatrix[0];
	yRow[0] = _mm_shuffle_ps(vMin, vMin, 0x55) * cumulativeMatrix[1];
	yRow[1] = _mm_shuffle_ps(vMax, vMax, 0x55) * cumulativeMatrix[1];
	zRow[0] = _mm_shuffle_ps(vMin, vMin, 0xaa) * cumulativeMatrix[2];
	zRow[1] = _mm_shuffle_ps(vMax, vMax, 0xaa) * cumulativeMatrix[2];

	__m128 zAllIn = _mm_castsi128_ps(_mm_set1_epi32(~0));

	for(UINT i = 0; i < AABB_VERTICES; i++)
	{
		// Transform the vertex
		__m128 vert = cumulativeMatrix[3];
		vert += xRow[sBBxInd[i]];
		vert += yRow[sBByInd[i]];
		vert += zRow[sBBzInd[i]];

		// We have inverted z; z is in front of near plane iff z <= w.
		__m128 vertZ = _mm_shuffle_ps(vert, vert, 0xaa); // vert.zzzz
		__m128 vertW = _mm_shuffle_ps(vert, vert, 0xff); // vert.wwww
		__m128 zIn = _mm_cmple_ps(vertZ, vertW);
		zAllIn = _mm_and_ps(zAllIn, zIn);

		// project
		xformedPos[i] = _mm_div_ps(vert, vertW);
	}

	// return true if and only if none of the verts are z-clipped
	return _mm_movemask_ps(zAllIn) == 0xf;
}

void TransformedAABBoxSSE::Gather(vFloat4 pOut[3], UINT triId, const __m128 xformedPos[])
{
	for(int i = 0; i < 3; i++)
	{
		UINT ind0 = sBBIndexList[triId*3 + i + 0];
		UINT ind1 = sBBIndexList[triId*3 + i + 3];
		UINT ind2 = sBBIndexList[triId*3 + i + 6];
		UINT ind3 = sBBIndexList[triId*3 + i + 9];

		__m128 v0 = xformedPos[ind0];	// x0 y0 z0 w0
		__m128 v1 = xformedPos[ind1];	// x1 y1 z1 w1
		__m128 v2 = xformedPos[ind2];	// x2 y2 z2 w2
		__m128 v3 = xformedPos[ind3];	// x3 y3 z3 w3
		_MM_TRANSPOSE4_PS(v0, v1, v2, v3);
		pOut[i].X = VecF32(v0);
		pOut[i].Y = VecF32(v1);
		pOut[i].Z = VecF32(v2);
		pOut[i].W = VecF32(v3);
	}
}

//-----------------------------------------------------------------------------------------
// Rasterize the occludee AABB and depth test it against the CPU rasterized depth buffer
// If any of the rasterized AABB pixels passes the depth test exit early and mark the occludee
// as visible. If all rasterized AABB pixels are occluded then the occludee is culled
//-----------------------------------------------------------------------------------------
bool TransformedAABBoxSSE::RasterizeAndDepthTestAABBox(UINT *pRenderTargetPixels, const __m128 pXformedPos[])
{
	// Set DAZ and FZ MXCSR bits to flush denormals to zero (i.e., make it faster)
	// Denormal are zero (DAZ) is bit 6 and Flush to zero (FZ) is bit 15. 
	// so to enable the two to have to set bits 6 and 15 which 1000 0000 0100 0000 = 0x8040
	_mm_setcsr( _mm_getcsr() | 0x8040 );

	VecS32 colOffset(0, 1, 0, 1);
	VecS32 rowOffset(0, 0, 1, 1);

	float* pDepthBuffer = (float*)pRenderTargetPixels; 
	
	// Rasterize the AABB triangles 4 at a time
	for(UINT i = 0; i < AABB_TRIANGLES; i += SSE)
	{
		vFloat4 xformedPos[3];
		Gather(xformedPos, i, pXformedPos);

		// use fixed-point only for X and Y.
		VecS32 fixX[3], fixY[3];
		for(int i = 0; i < 3; i++)
		{
			fixX[i] = ftoi_round(xformedPos[i].X);
			fixY[i] = ftoi_round(xformedPos[i].Y);
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

		// Z setup
		VecF32 Z[3];
		Z[0] = xformedPos[0].Z;
		Z[1] = (xformedPos[1].Z - Z[0]) * oneOverTriArea;
		Z[2] = (xformedPos[2].Z - Z[0]) * oneOverTriArea;

		// Use bounding box traversal strategy to determine which pixels to rasterize 
		VecS32 startX = vmax(vmin(vmin(fixX[0], fixX[1]), fixX[2]), VecS32(0)) & VecS32(~1);
		VecS32 endX   = vmin(vmax(vmax(fixX[0], fixX[1]), fixX[2]), VecS32(SCREENW - 1));

		VecS32 startY = vmax(vmin(vmin(fixY[0], fixY[1]), fixY[2]), VecS32(0)) & VecS32(~1);
		VecS32 endY   = vmin(vmax(vmax(fixY[0], fixY[1]), fixY[2]), VecS32(SCREENH - 1));

		// Now we have 4 triangles set up.  Rasterize them each individually.
        for(int lane=0; lane < SSE; lane++)
        {
			// Skip triangle if area is zero 
			if(triArea.lane[lane] <= 0)
			{
				continue;
			}

			// Extract this triangle's properties from the SIMD versions
            VecF32 zz[3];
			for(int vv = 0; vv < 3; vv++)
			{
				zz[vv] = VecF32(Z[vv].lane[lane]);
			}

			int startXx = startX.lane[lane];
			int endXx	= endX.lane[lane];
			int startYy = startY.lane[lane];
			int endYy	= endY.lane[lane];
		
			VecS32 aa0(A0.lane[lane]);
			VecS32 aa1(A1.lane[lane]);
			VecS32 aa2(A2.lane[lane]);

			VecS32 bb0(B0.lane[lane]);
			VecS32 bb1(B1.lane[lane]);
			VecS32 bb2(B2.lane[lane]);

			VecS32 aa0Inc = shiftl<1>(aa0);
			VecS32 aa1Inc = shiftl<1>(aa1);
			VecS32 aa2Inc = shiftl<1>(aa2);

			VecS32 bb0Inc = shiftl<1>(bb0);
			VecS32 bb1Inc = shiftl<1>(bb1);
			VecS32 bb2Inc = shiftl<1>(bb2);

			// Traverse pixels in 2x2 blocks and store 2x2 pixel quad depths contiguously in memory ==> 2*X
			// This method provides better performance
			int rowIdx = (startYy * SCREENW + 2 * startXx);

			VecS32 col = VecS32(startXx) + colOffset;
			VecS32 row = VecS32(startYy) + rowOffset;

			VecS32 sum0Row = aa0 * col + bb0 * row + VecS32(C0.lane[lane]);
			VecS32 sum1Row = aa1 * col + bb1 * row + VecS32(C1.lane[lane]);
			VecS32 sum2Row = aa2 * col + bb2 * row + VecS32(C2.lane[lane]);

			VecF32 zx = itof(aa1Inc) * zz[1] + itof(aa2Inc) * zz[2];

			// Incrementally compute Fab(x, y) for all the pixels inside the bounding box formed by (startX, endX) and (startY, endY)
			for(int r = startYy; r <= endYy; r += 2,
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
				VecF32 depth = zz[0] + itof(beta) * zz[1] + itof(gama) * zz[2];
				VecS32 anyOut = VecS32::zero();

				for(int c = startXx; c <= endXx; c += 2,
												 idx += 4,
												 alpha += aa0Inc,
												 beta  += aa1Inc,
												 gama  += aa2Inc,
												 depth += zx)
				{
					//Test Pixel inside triangle
					VecS32 mask = alpha | beta | gama;
					
					VecF32 previousDepthValue = VecF32::load(&pDepthBuffer[idx]);
					VecF32 depthMask = cmpge(depth, previousDepthValue);
					VecS32 finalMask = andnot(mask, float2bits(depthMask));
					anyOut |= finalMask;
				}//for each column

				if(!_mm_testz_si128(anyOut.simd, _mm_set1_epi32(0x80000000)))
				{
					return true; //early exit
				}
			}// for each row
		}// for each triangle
	}// for each set of SIMD# triangles

	return false;
}