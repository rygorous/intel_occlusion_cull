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

TransformedAABBoxSSE::TransformedAABBoxSSE()
	: mpCPUTModel(NULL),
	  mVisible(NULL),
	  mOccludeeSizeThreshold(0.0f)
{
	mWorldMatrix = (__m128*)_aligned_malloc(sizeof(float) * 4 * 4, 16);
	mpBBVertexList = (__m128*)_aligned_malloc(sizeof(float) * 4 * AABB_VERTICES, 16);
	mpXformedPos =  (__m128*)_aligned_malloc(sizeof(float) * 4 * AABB_VERTICES, 16);
	mViewPortMatrix = (__m128*)_aligned_malloc(sizeof(float) * 4 * 4, 16);
	mCumulativeMatrix = (__m128*)_aligned_malloc(sizeof(float) * 4 * 4, 16); 

	mViewPortMatrix[0] = _mm_loadu_ps((float*)&viewportMatrix.r0);
	mViewPortMatrix[1] = _mm_loadu_ps((float*)&viewportMatrix.r1);
	mViewPortMatrix[2] = _mm_loadu_ps((float*)&viewportMatrix.r2);
	mViewPortMatrix[3] = _mm_loadu_ps((float*)&viewportMatrix.r3);
}

TransformedAABBoxSSE::~TransformedAABBoxSSE()
{
	_aligned_free(mWorldMatrix);
	_aligned_free(mpBBVertexList);
	_aligned_free(mpXformedPos);
	_aligned_free(mViewPortMatrix);
	_aligned_free(mCumulativeMatrix);
}

//--------------------------------------------------------------------------
// Get the bounding box center and half vector
// Create the vertex and index list for the triangles that make up the bounding box
//--------------------------------------------------------------------------
void TransformedAABBoxSSE::CreateAABBVertexIndexList(CPUTModelDX11 *pModel)
{
	mpCPUTModel = pModel;
	float* world = (float*)pModel->GetWorldMatrix();

	mWorldMatrix[0] = _mm_loadu_ps(world + 0);
	mWorldMatrix[1] = _mm_loadu_ps(world + 4);
	mWorldMatrix[2] = _mm_loadu_ps(world + 8);
	mWorldMatrix[3] = _mm_loadu_ps(world + 12);

	pModel->GetBoundsObjectSpace(&mBBCenter, &mBBHalf);

	float3 min = mBBCenter - mBBHalf;
	float3 max = mBBCenter + mBBHalf;
	
	//Top 4 vertices in BB
	mpBBVertexList[0] = _mm_set_ps(1.0f, max.z, max.y, max.x);
	mpBBVertexList[1] = _mm_set_ps(1.0f, max.z, max.y, min.x); 
	mpBBVertexList[2] = _mm_set_ps(1.0f, min.z, max.y, min.x);
	mpBBVertexList[3] = _mm_set_ps(1.0f, min.z, max.y, max.x);
	// Bottom 4 vertices in BB
	mpBBVertexList[4] = _mm_set_ps(1.0f, min.z, min.y, max.x);
	mpBBVertexList[5] = _mm_set_ps(1.0f, max.z, min.y, max.x);
	mpBBVertexList[6] = _mm_set_ps(1.0f, max.z, min.y, min.x);
	mpBBVertexList[7] = _mm_set_ps(1.0f, min.z, min.y, min.x);
}

//----------------------------------------------------------------------------
// Determine if the occluddee size is too small and if so avoid drawing it
//----------------------------------------------------------------------------
bool TransformedAABBoxSSE::IsTooSmall(__m128 *pViewMatrix, __m128 *pProjMatrix, CPUTCamera *pCamera)
{
	float radius = mBBHalf.lengthSq(); // Use length-squared to avoid sqrt().  Relative comparissons hold.
	float fov = pCamera->GetFov();
	float tanOfHalfFov = tanf(fov * 0.5f);

	MatrixMultiply(mWorldMatrix, pViewMatrix, mCumulativeMatrix);
	MatrixMultiply(mCumulativeMatrix, pProjMatrix, mCumulativeMatrix);
	MatrixMultiply(mCumulativeMatrix, mViewPortMatrix, mCumulativeMatrix);

	__m128 center = _mm_set_ps(1.0f, mBBCenter.z, mBBCenter.y, mBBCenter.x);
	__m128 mBBCenterOSxForm = TransformCoords(&center, mCumulativeMatrix);
    float w = mBBCenterOSxForm.m128_f32[3];
	if( w > 1.0f )
	{
		float radiusDivW = radius / w;
		float r2DivW2DivTanFov = radiusDivW / tanOfHalfFov;

		return r2DivW2DivTanFov < (mOccludeeSizeThreshold * mOccludeeSizeThreshold) ?  true : false;
	}

	return false;
}

//----------------------------------------------------------------
// Trasforms the AABB vertices to screen space once every frame
//----------------------------------------------------------------
void TransformedAABBoxSSE::TransformAABBox()
{
	for(UINT i = 0; i < AABB_VERTICES; i++)
	{
		mpXformedPos[i] = TransformCoords(&mpBBVertexList[i], mCumulativeMatrix);
		float oneOverW = 1.0f/max(mpXformedPos[i].m128_f32[3], 0.0000001f);
		mpXformedPos[i] = mpXformedPos[i] * oneOverW;
		mpXformedPos[i].m128_f32[3] = oneOverW;
	}
}

void TransformedAABBoxSSE::Gather(vFloat4 pOut[3], UINT triId)
{
	for(int lane = 0; lane < SSE; lane++)
	{
		for(int i = 0; i < 3; i++)
		{
			UINT index = sBBIndexList[(triId * 3) + (lane * 3) + i];
			pOut[i].X.lane[lane] = mpXformedPos[index].m128_f32[0];
			pOut[i].Y.lane[lane] = mpXformedPos[index].m128_f32[1];
			pOut[i].Z.lane[lane] = mpXformedPos[index].m128_f32[2];
			pOut[i].W.lane[lane] = mpXformedPos[index].m128_f32[3];
		}
	}
}

//-----------------------------------------------------------------------------------------
// Rasterize the occludee AABB and depth test it against the CPU rasterized depth buffer
// If any of the rasterized AABB pixels passes the depth test exit early and mark the occludee
// as visible. If all rasterized AABB pixels are occluded then the occludee is culled
//-----------------------------------------------------------------------------------------
void TransformedAABBoxSSE::RasterizeAndDepthTestAABBox(UINT *pRenderTargetPixels)
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
		Gather(xformedPos, i);

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

		for(int vv = 0; vv < 3; vv++) 
		{
            // If W (holding 1/w in our case) is not between 0 and 1,
            // then vertex is behind near clip plane (1.0 in our case.
            // If W < 1, then verify 1/W > 1 (for W>0), and 1/W < 0 (for W < 0).
		    VecF32 nearClipMask0 = cmple(xformedPos[vv].W, VecF32(0.0f));
		    VecF32 nearClipMask1 = cmpge(xformedPos[vv].W, VecF32(1.0f));
			VecS32 nearClipMask = float2bits(or(nearClipMask0, nearClipMask1));

			if(!is_all_zeros(nearClipMask))
			{
                // All four vertices are behind the near plane (we're processing four triangles at a time w/ SSE)
                *mVisible = true;
                return;
			}
		}

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
					*mVisible = true;
					return; //early exit
				}
			}// for each row
		}// for each triangle
	}// for each set of SIMD# triangles
}