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

void BoxTestSetup::Init(__m128 viewMatrix[4], __m128 projMatrix[4], CPUTCamera *pCamera, float occludeeSizeThreshold)
{
	__m128 viewport[4];
	viewport[0] = _mm_loadu_ps((float*)&viewportMatrix.r0);
	viewport[1] = _mm_loadu_ps((float*)&viewportMatrix.r1);
	viewport[2] = _mm_loadu_ps((float*)&viewportMatrix.r2);
	viewport[3] = _mm_loadu_ps((float*)&viewportMatrix.r3);

	HelperSSE::MatrixMultiply(viewMatrix, projMatrix, mViewProjViewport);
	HelperSSE::MatrixMultiply(mViewProjViewport, viewport, mViewProjViewport);

	float fov = pCamera->GetFov();
	float tanOfHalfFov = tanf(fov * 0.5f);
	radiusThreshold = occludeeSizeThreshold * occludeeSizeThreshold * tanOfHalfFov;
}

TransformedAABBoxSSE::TransformedAABBoxSSE()
	: mpCPUTModel(NULL)
{
}

TransformedAABBoxSSE::~TransformedAABBoxSSE()
{
}

//--------------------------------------------------------------------------
// Get the bounding box center and half vector
// Create the vertex and index list for the triangles that make up the bounding box
//--------------------------------------------------------------------------
void TransformedAABBoxSSE::CreateAABBVertexIndexList(CPUTModelDX11 *pModel)
{
	mpCPUTModel = pModel;

	float3 bbHalf;
	pModel->GetBoundsObjectSpace(&mBBCenter, &bbHalf);
	mRadiusSq = pModel->mBoundingBoxHalfObjectSpace.lengthSq();
}

//----------------------------------------------------------------------------
// Determines the cumulative transform matrix
//----------------------------------------------------------------------------
void TransformedAABBoxSSE::MakeCumulativeMatrix(__m128 cumulativeMatrix[4], const BoxTestSetup &setup)
{
	float* world = (float*)mpCPUTModel->GetWorldMatrix();

	__m128 worldMatrix[4];
	worldMatrix[0] = _mm_loadu_ps(world + 0);
	worldMatrix[1] = _mm_loadu_ps(world + 4);
	worldMatrix[2] = _mm_loadu_ps(world + 8);
	worldMatrix[3] = _mm_loadu_ps(world + 12);

	MatrixMultiply(worldMatrix, setup.mViewProjViewport, cumulativeMatrix);
}

//----------------------------------------------------------------------------
// Determine if the occluddee size is too small and if so avoid drawing it
//----------------------------------------------------------------------------
bool TransformedAABBoxSSE::IsTooSmall(const BoxTestSetup &setup, const __m128 cumulativeMatrix[4])
{
	__m128 center = _mm_set_ps(1.0f, mBBCenter.z, mBBCenter.y, mBBCenter.x);
	__m128 mBBCenterOSxForm = TransformCoords(&center, cumulativeMatrix);
    float w = mBBCenterOSxForm.m128_f32[3];
	if( w > 1.0f )
	{
		return mRadiusSq < w * setup.radiusThreshold;
	}
	return false;
}

//----------------------------------------------------------------
// Trasforms the AABB vertices to screen space once every frame
//----------------------------------------------------------------
void TransformedAABBoxSSE::TransformAABBox(__m128 *pXformedPos, const __m128 cumulativeMatrix[4])
{
	const float3 &bbCenter = mpCPUTModel->mBoundingBoxCenterObjectSpace;
	const float3 &bbHalf   = mpCPUTModel->mBoundingBoxHalfObjectSpace;

	static const __declspec(align(16)) unsigned int mixVert[8][4] = {
		//Top 4 vertices in BB
		{ ~0, ~0, ~0, 0 },
		{  0, ~0, ~0, 0 },
		{  0, ~0,  0, 0 },
		{ ~0, ~0,  0, 0 },
		// Bottom 4 vertices in BB
		{ ~0,  0,  0, 0 },
		{ ~0,  0, ~0, 0 },
		{  0,  0, ~0, 0 },
		{  0,  0,  0, 0 },
	};

	__m128 minVert = _mm_setr_ps(bbCenter.x - bbHalf.x, bbCenter.y - bbHalf.y, bbCenter.z - bbHalf.z, 1.0f);
	__m128 maxVert = _mm_setr_ps(bbCenter.x + bbHalf.x, bbCenter.y + bbHalf.y, bbCenter.z + bbHalf.z, 1.0f);

	for(UINT i = 0; i < AABB_VERTICES; i++)
	{
		__m128 mixConst = _mm_load_ps((const float *) mixVert[i]);
		__m128 vertex = _mm_blendv_ps(minVert, maxVert, mixConst);

		__m128 xformed = TransformCoords(&vertex, cumulativeMatrix);
		__m128 clampedW = _mm_max_ps(_mm_set1_ps(0.0000001f), _mm_shuffle_ps(xformed, xformed, 0xff));
		pXformedPos[i] = _mm_div_ps(xformed, clampedW);
	}
}

void TransformedAABBoxSSE::Gather(vFloat4 pOut[3], UINT triId, const __m128 *pXformedPos)
{
	for(int i = 0; i < 3; i++)
	{
		UINT i0 = sBBIndexList[triId * 3 + 0 + i];
		UINT i1 = sBBIndexList[triId * 3 + 3 + i];
		UINT i2 = sBBIndexList[triId * 3 + 6 + i];
		UINT i3 = sBBIndexList[triId * 3 + 9 + i];

		// Load
		__m128 a0 = pXformedPos[i0];
		__m128 a1 = pXformedPos[i1];
		__m128 a2 = pXformedPos[i2];
		__m128 a3 = pXformedPos[i3];

		// Pass 1
		__m128 b0 = _mm_unpacklo_ps(a0, a2); // a0x a2x a0y a2y
		__m128 b1 = _mm_unpacklo_ps(a1, a3); // a1x a3x a1y a3y
		__m128 b2 = _mm_unpackhi_ps(a0, a2); // a0z a2z a0w a2w
		__m128 b3 = _mm_unpackhi_ps(a1, a3); // a1z a3z a1w a3w

		// Pass 2
		pOut[i].X = _mm_unpacklo_ps(b0, b1); // a0x a1x a2x a3x
		pOut[i].Y = _mm_unpackhi_ps(b0, b1); // a0y a1y a2y a3y
		pOut[i].Z = _mm_unpacklo_ps(b2, b3); // a0z a1z a2z a3z
		pOut[i].W = _mm_unpackhi_ps(b2, b3); // a0w a1w a2a a3w
	}
}

//-----------------------------------------------------------------------------------------
// Rasterize the occludee AABB and depth test it against the CPU rasterized depth buffer
// If any of the rasterized AABB pixels passes the depth test exit early and mark the occludee
// as visible. If all rasterized AABB pixels are occluded then the occludee is culled
//-----------------------------------------------------------------------------------------
bool TransformedAABBoxSSE::RasterizeAndDepthTestAABBox(UINT *pRenderTargetPixels, const __m128 *pXformedPos)
{
	// Set DAZ and FZ MXCSR bits to flush denormals to zero (i.e., make it faster)
	// Denormal are zero (DAZ) is bit 6 and Flush to zero (FZ) is bit 15. 
	// so to enable the two to have to set bits 6 and 15 which 1000 0000 0100 0000 = 0x8040
	_mm_setcsr( _mm_getcsr() | 0x8040 );

	__m128i colOffset = _mm_setr_epi32(0, 1, 0, 1);
	__m128i rowOffset = _mm_setr_epi32(0, 0, 1, 1);

	float* pDepthBuffer = (float*)pRenderTargetPixels; 
	
	// Rasterize the AABB triangles 4 at a time
	for(UINT i = 0; i < AABB_TRIANGLES; i += SSE)
	{
		vFloat4 xformedPos[3];
		Gather(xformedPos, i, pXformedPos);

		// use fixed-point only for X and Y.  Avoid work for Z and W.
		__m128i fixX[3], fixY[3];
		for(int i = 0; i < 3; i++)
		{
			fixX[i] = _mm_cvtps_epi32(xformedPos[i].X);
			fixY[i] = _mm_cvtps_epi32(xformedPos[i].Y);
		}

		// Fab(x, y) =     Ax       +       By     +      C              = 0
		// Fab(x, y) = (ya - yb)x   +   (xb - xa)y + (xa * yb - xb * ya) = 0
		// Compute A = (ya - yb) for 2 of the 3 line segments that make up each triangle
		__m128i A1 = _mm_sub_epi32(fixY[2], fixY[0]);
		__m128i A2 = _mm_sub_epi32(fixY[0], fixY[1]);

		// Compute B = (xb - xa) for 2 of the 3 line segments that make up each triangle
		__m128i B1 = _mm_sub_epi32(fixX[0], fixX[2]);
		__m128i B2 = _mm_sub_epi32(fixX[1], fixX[0]);

		// Compute C = (xa * yb - xb * ya) for 2 of the 3 line segments that make up each triangle
		__m128i C1 = _mm_sub_epi32(_mm_mullo_epi32(fixX[2], fixY[0]), _mm_mullo_epi32(fixX[0], fixY[2]));
		__m128i C2 = _mm_sub_epi32(_mm_mullo_epi32(fixX[0], fixY[1]), _mm_mullo_epi32(fixX[1], fixY[0]));

		// Compute triangle area
		__m128i triArea = _mm_sub_epi32(_mm_mullo_epi32(B2, A1), _mm_mullo_epi32(B1, A2));
		__m128 oneOverTriArea = _mm_div_ps(_mm_set1_ps(1.0f), _mm_cvtepi32_ps(triArea));

		// Z setup
		__m128 Z[3];
		Z[0] = xformedPos[0].Z;
		Z[1] = _mm_mul_ps(_mm_sub_ps(xformedPos[1].Z, xformedPos[0].Z), oneOverTriArea);
		Z[2] = _mm_mul_ps(_mm_sub_ps(xformedPos[2].Z, xformedPos[0].Z), oneOverTriArea);

		// Use bounding box traversal strategy to determine which pixels to rasterize 
		__m128i startX = _mm_and_si128(Max(Min(Min(fixX[0], fixX[1]), fixX[2]), _mm_set1_epi32(0)), _mm_set1_epi32(0xFFFFFFFE));
		__m128i endX   = Min(Max(Max(fixX[0], fixX[1]), fixX[2]), _mm_set1_epi32(SCREENW-1));

		__m128i startY = _mm_and_si128(Max(Min(Min(fixY[0], fixY[1]), fixY[2]), _mm_set1_epi32(0)), _mm_set1_epi32(0xFFFFFFFE));
		__m128i endY   = Min(Max(Max(fixY[0], fixY[1]), fixY[2]), _mm_set1_epi32(SCREENH-1));

		// If Z is larger than 1, then the vertex is behind the near clip plane.
		__m128	v0near	= _mm_cmpnle_ps(xformedPos[0].Z, _mm_set1_ps(1.0f));
		__m128	v1near	= _mm_cmpnle_ps(xformedPos[1].Z, _mm_set1_ps(1.0f));
		__m128	v2near	= _mm_cmpnle_ps(xformedPos[2].Z, _mm_set1_ps(1.0f));

		// If, for any tri, any of the vertices are behind the near plane, bail
		__m128	vnear	= _mm_or_ps(_mm_or_ps(v0near, v1near), v2near);
		if (_mm_movemask_ps(vnear))
			return true;

		// Now we have 4 triangles set up.  Rasterize them each individually.
        for(int lane=0; lane < SSE; lane++)
        {
			// Skip triangle if area is zero 
			if(triArea.m128i_i32[lane] <= 0)
			{
				continue;
			}

			// Extract this triangle's properties from the SIMD versions
            __m128 zz[3];
			for(int vv = 0; vv < 3; vv++)
			{
				zz[vv] = _mm_set1_ps(Z[vv].m128_f32[lane]);
			}

			int startXx = startX.m128i_i32[lane];
			int endXx	= endX.m128i_i32[lane];
			int startYy = startY.m128i_i32[lane];
			int endYy	= endY.m128i_i32[lane];
			if (endXx < startXx || endYy < startYy)
				continue;
		
			__m128i sum = _mm_set1_epi32(triArea.m128i_i32[lane]);

			__m128i aa1 = _mm_set1_epi32(A1.m128i_i32[lane]);
			__m128i aa2 = _mm_set1_epi32(A2.m128i_i32[lane]);

			__m128i bb1 = _mm_set1_epi32(B1.m128i_i32[lane]);
			__m128i bb2 = _mm_set1_epi32(B2.m128i_i32[lane]);

			__m128i cc1 = _mm_set1_epi32(C1.m128i_i32[lane]);
			__m128i cc2 = _mm_set1_epi32(C2.m128i_i32[lane]);

			__m128i aa1Inc = _mm_slli_epi32(aa1, 1);
			__m128i aa2Inc = _mm_slli_epi32(aa2, 1);
			__m128i aa0Dec = _mm_add_epi32(aa1Inc, aa2Inc);

			__m128i row, col;

			// Traverse pixels in 2x2 blocks and store 2x2 pixel quad depths contiguously in memory ==> 2*X
			// This method provides better perfromance
			UINT ofs_x0 = EncodePosX(startXx);
			UINT ofs_x1 = EncodePosX(endXx);
			UINT ofs_y0 = EncodePosY(startYy);
			UINT ofs_y1 = EncodePosY(endYy);

			col = _mm_add_epi32(colOffset, _mm_set1_epi32(startXx));
			__m128i aa1Col = _mm_mullo_epi32(aa1, col);
			__m128i aa2Col = _mm_mullo_epi32(aa2, col);

			row = _mm_add_epi32(rowOffset, _mm_set1_epi32(startYy));
			__m128i bb1Row = _mm_add_epi32(_mm_mullo_epi32(bb1, row), cc1);
			__m128i bb2Row = _mm_add_epi32(_mm_mullo_epi32(bb2, row), cc2);

			__m128i bb1Inc = _mm_slli_epi32(bb1, 1);
			__m128i bb2Inc = _mm_slli_epi32(bb2, 1);

			// Incrementally compute Fab(x, y) for all the pixels inside the bounding box formed by (startX, endX) and (startY, endY)
			for(UINT ofs_y = ofs_y0; ofs_y <= ofs_y1; ofs_y = StepY2(ofs_y))
			{
				// Compute barycentric coordinates 
				float *pDepth = &pDepthBuffer[ofs_y];
				__m128i beta = _mm_add_epi32(aa1Col, bb1Row);
				__m128i gama = _mm_add_epi32(aa2Col, bb2Row);
				__m128i alpha = _mm_sub_epi32(_mm_sub_epi32(sum, beta), gama);

				bb1Row = _mm_add_epi32(bb1Row, bb1Inc);
				bb2Row = _mm_add_epi32(bb2Row, bb2Inc);

				for(UINT ofs_x = ofs_x0; ofs_x <= ofs_x1; ofs_x = StepX2(ofs_x))
				{
					// Compute barycentric-interpolated depth
			        __m128 depth = zz[0];
					depth = _mm_add_ps(depth, _mm_mul_ps(_mm_cvtepi32_ps(beta), zz[1]));
					depth = _mm_add_ps(depth, _mm_mul_ps(_mm_cvtepi32_ps(gama), zz[2]));

					//Test Pixel inside triangle
					__m128i mask = _mm_or_si128(_mm_or_si128(alpha, beta), gama);
					alpha = _mm_sub_epi32(alpha, aa0Dec);
					beta  = _mm_add_epi32(beta, aa1Inc);
					gama  = _mm_add_epi32(gama, aa2Inc);

					// Depth test
					__m128 previousDepthValue = _mm_load_ps(&pDepth[ofs_x]);
					__m128 depthMask  = _mm_cmpge_ps( depth, previousDepthValue);
					__m128i finalMask = _mm_andnot_si128( mask, _mm_castps_si128(depthMask));
					if(!_mm_test_all_zeros(finalMask, finalMask))
					{
						return true; // early exit
					}
				}//for each column											
			}// for each row
		}// for each triangle
	}// for each set of SIMD# triangles

	return false;
}