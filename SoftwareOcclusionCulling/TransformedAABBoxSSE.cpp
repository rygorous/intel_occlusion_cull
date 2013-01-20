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

TransformedAABBoxSSE::TransformedAABBoxSSE()
	: mpCPUTModel(NULL),
	  mVisible(NULL),
	  mInsideViewFrustum(true),
	  mOccludeeSizeThreshold(0.0f),
	  mTooSmall(false)
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

	// index for top 
	mBBIndexList[0]  = 1;
	mBBIndexList[1]  = 3;
	mBBIndexList[2]  = 2;
	mBBIndexList[3]  = 0;
	mBBIndexList[4]  = 3;
	mBBIndexList[5]  = 1;

	// index for bottom
	mBBIndexList[6]  = 5;
	mBBIndexList[7]  = 7;
	mBBIndexList[8]  = 4;
	mBBIndexList[9]  = 6;
	mBBIndexList[10] = 7;
	mBBIndexList[11] = 5;

	// index for left
	mBBIndexList[12] = 1;
	mBBIndexList[13] = 7;
	mBBIndexList[14] = 6;
	mBBIndexList[15] = 2;
	mBBIndexList[16] = 7;
	mBBIndexList[17] = 1;

	// index for right
	mBBIndexList[18] = 3;
	mBBIndexList[19] = 5;
	mBBIndexList[20] = 4;
	mBBIndexList[21] = 0;
	mBBIndexList[22] = 5;
	mBBIndexList[23] = 3;

	// index for back
	mBBIndexList[24] = 2;
	mBBIndexList[25] = 4;
	mBBIndexList[26] = 7;
	mBBIndexList[27] = 3;
	mBBIndexList[28] = 4;
	mBBIndexList[29] = 2;

	// index for front
	mBBIndexList[30] = 0;
	mBBIndexList[31] = 6;
	mBBIndexList[32] = 5;
	mBBIndexList[33] = 1;
	mBBIndexList[34] = 6;
	mBBIndexList[35] = 0;
}

//----------------------------------------------------------------
// Determine is model is inside view frustum
//----------------------------------------------------------------
void TransformedAABBoxSSE::IsInsideViewFrustum(CPUTCamera *pCamera)
{
	float3 mBBCenterWS;
	float3 mBBHalfWS;
	mpCPUTModel->GetBoundsWorldSpace(&mBBCenterWS, &mBBHalfWS);
	mInsideViewFrustum = pCamera->mFrustum.IsVisible(mBBCenterWS, mBBHalfWS);
}

//----------------------------------------------------------------------------
// Determine if the occluddee size is too small and if so avoid drawing it
//----------------------------------------------------------------------------
bool TransformedAABBoxSSE::IsTooSmall(__m128 *pViewMatrix, __m128 *pProjMatrix, CPUTCamera *pCamera)
{
	float radius = mBBHalf.lengthSq(); // Use length-squared to avoid sqrt().  Relative comparissons hold.
	float fov = pCamera->GetFov();
	float tanOfHalfFov = tanf(fov * 0.5f);
	mTooSmall = false;

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

		mTooSmall = r2DivW2DivTanFov < (mOccludeeSizeThreshold * mOccludeeSizeThreshold) ?  true : false;
	}
	else
	{
		mTooSmall = false;
	}
	return mTooSmall;
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
			UINT index = mBBIndexList[(triId * 3) + (lane * 3) + i];
			pOut[i].X.m128_f32[lane] = mpXformedPos[index].m128_f32[0];
			pOut[i].Y.m128_f32[lane] = mpXformedPos[index].m128_f32[1];
			pOut[i].Z.m128_f32[lane] = mpXformedPos[index].m128_f32[2];
			pOut[i].W.m128_f32[lane] = mpXformedPos[index].m128_f32[3];
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

	__m128i colOffset = _mm_set_epi32(0, 1, 0, 1);
	__m128i rowOffset = _mm_set_epi32(0, 0, 1, 1);

	__m128i fxptZero = _mm_setzero_si128();
	float* pDepthBuffer = (float*)pRenderTargetPixels; 
	
	// Rasterize the AABB triangles 4 at a time
	for(UINT i = 0; i < AABB_TRIANGLES; i += SSE)
	{
		vFloat4 xformedPos[3];
		Gather(xformedPos, i);

		// use fixed-point only for X and Y.  Avoid work for Z and W.
        vFxPt4 xFormedFxPtPos[3];
		for(int m = 0; m < 3; m++)
		{
			xFormedFxPtPos[m].X = _mm_cvtps_epi32(xformedPos[m].X);
			xFormedFxPtPos[m].Y = _mm_cvtps_epi32(xformedPos[m].Y);
			xFormedFxPtPos[m].Z = _mm_cvtps_epi32(xformedPos[m].Z);
			xFormedFxPtPos[m].W = _mm_cvtps_epi32(xformedPos[m].W);
		}

		// Fab(x, y) =     Ax       +       By     +      C              = 0
		// Fab(x, y) = (ya - yb)x   +   (xb - xa)y + (xa * yb - xb * ya) = 0
		// Compute A = (ya - yb) for the 3 line segments that make up each triangle
		__m128i A0 = _mm_sub_epi32(xFormedFxPtPos[1].Y, xFormedFxPtPos[2].Y);
		__m128i A1 = _mm_sub_epi32(xFormedFxPtPos[2].Y, xFormedFxPtPos[0].Y);
		__m128i A2 = _mm_sub_epi32(xFormedFxPtPos[0].Y, xFormedFxPtPos[1].Y);

		// Compute B = (xb - xa) for the 3 line segments that make up each triangle
		__m128i B0 = _mm_sub_epi32(xFormedFxPtPos[2].X, xFormedFxPtPos[1].X);
		__m128i B1 = _mm_sub_epi32(xFormedFxPtPos[0].X, xFormedFxPtPos[2].X);
		__m128i B2 = _mm_sub_epi32(xFormedFxPtPos[1].X, xFormedFxPtPos[0].X);

		// Compute C = (xa * yb - xb * ya) for the 3 line segments that make up each triangle
		__m128i C0 = _mm_sub_epi32(_mm_mullo_epi32(xFormedFxPtPos[1].X, xFormedFxPtPos[2].Y), _mm_mullo_epi32(xFormedFxPtPos[2].X, xFormedFxPtPos[1].Y));
		__m128i C1 = _mm_sub_epi32(_mm_mullo_epi32(xFormedFxPtPos[2].X, xFormedFxPtPos[0].Y), _mm_mullo_epi32(xFormedFxPtPos[0].X, xFormedFxPtPos[2].Y));
		__m128i C2 = _mm_sub_epi32(_mm_mullo_epi32(xFormedFxPtPos[0].X, xFormedFxPtPos[1].Y), _mm_mullo_epi32(xFormedFxPtPos[1].X, xFormedFxPtPos[0].Y));

		// Compute triangle area
		__m128i triArea = _mm_mullo_epi32(A0, xFormedFxPtPos[0].X);
		triArea = _mm_add_epi32(triArea, _mm_mullo_epi32(B0, xFormedFxPtPos[0].Y));
		triArea = _mm_add_epi32(triArea, C0);

		__m128 oneOverTriArea = _mm_div_ps(_mm_set1_ps(1.0f), _mm_cvtepi32_ps(triArea));

		// Use bounding box traversal strategy to determine which pixels to rasterize 
		__m128i startX = _mm_and_si128(Max(Min(Min(xFormedFxPtPos[0].X, xFormedFxPtPos[1].X), xFormedFxPtPos[2].X), _mm_set1_epi32(0)), _mm_set1_epi32(0xFFFFFFFE));
		__m128i endX   = Min(_mm_add_epi32(Max(Max(xFormedFxPtPos[0].X, xFormedFxPtPos[1].X), xFormedFxPtPos[2].X), _mm_set1_epi32(1)), _mm_set1_epi32(SCREENW));

		__m128i startY = _mm_and_si128(Max(Min(Min(xFormedFxPtPos[0].Y, xFormedFxPtPos[1].Y), xFormedFxPtPos[2].Y), _mm_set1_epi32(0)), _mm_set1_epi32(0xFFFFFFFE));
		__m128i endY   = Min(_mm_add_epi32(Max(Max(xFormedFxPtPos[0].Y, xFormedFxPtPos[1].Y), xFormedFxPtPos[2].Y), _mm_set1_epi32(1)), _mm_set1_epi32(SCREENH));

		for(int vv = 0; vv < 3; vv++) 
		{
            // If W (holding 1/w in our case) is not between 0 and 1,
            // then vertex is behind near clip plane (1.0 in our case.
            // If W < 1, then verify 1/W > 1 (for W>0), and 1/W < 0 (for W < 0).
		    __m128 nearClipMask0 = _mm_cmple_ps(xformedPos[vv].W, _mm_set1_ps(0.0f));
		    __m128 nearClipMask1 = _mm_cmpge_ps(xformedPos[vv].W, _mm_set1_ps(1.0f));
            __m128 nearClipMask  = _mm_or_ps(nearClipMask0, nearClipMask1);

			if(!_mm_test_all_zeros(*(__m128i*)&nearClipMask, *(__m128i*)&nearClipMask))
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
			if(triArea.m128i_i32[lane] <= 0)
			{
				continue;
			}

			// Extract this triangle's properties from the SIMD versions
            __m128 zz[3], oneOverW[3];
			for(int vv = 0; vv < 3; vv++)
			{
				zz[vv] = _mm_set1_ps(xformedPos[vv].Z.m128_f32[lane]);
				oneOverW[vv] = _mm_set1_ps(xformedPos[vv].W.m128_f32[lane]);
			}

			__m128 oneOverTotalArea = _mm_set1_ps(oneOverTriArea.m128_f32[lane]);
			zz[0] *= oneOverTotalArea;
			zz[1] *= oneOverTotalArea;
			zz[2] *= oneOverTotalArea;
			
			int startXx = startX.m128i_i32[lane];
			int endXx	= endX.m128i_i32[lane];
			int startYy = startY.m128i_i32[lane];
			int endYy	= endY.m128i_i32[lane];
		
			__m128i aa0 = _mm_set1_epi32(A0.m128i_i32[lane]);
			__m128i aa1 = _mm_set1_epi32(A1.m128i_i32[lane]);
			__m128i aa2 = _mm_set1_epi32(A2.m128i_i32[lane]);

			__m128i bb0 = _mm_set1_epi32(B0.m128i_i32[lane]);
			__m128i bb1 = _mm_set1_epi32(B1.m128i_i32[lane]);
			__m128i bb2 = _mm_set1_epi32(B2.m128i_i32[lane]);

			__m128i cc0 = _mm_set1_epi32(C0.m128i_i32[lane]);
			__m128i cc1 = _mm_set1_epi32(C1.m128i_i32[lane]);
			__m128i cc2 = _mm_set1_epi32(C2.m128i_i32[lane]);

			__m128i aa0Inc = _mm_slli_epi32(aa0, 1);
			__m128i aa1Inc = _mm_slli_epi32(aa1, 1);
			__m128i aa2Inc = _mm_slli_epi32(aa2, 1);

			__m128i row, col;

			int rowIdx;
			// To avoid this branching, choose one method to traverse and store the pixel depth
			if(gVisualizeDepthBuffer)
			{
				// Sequentially traverse and store pixel depths contiguously
				rowIdx = (startYy * SCREENW + startXx);
			}
			else
			{
				// Tranverse pixels in 2x2 blocks and store 2x2 pixel quad depths contiguously in memory ==> 2*X
				// This method provides better perfromance
				rowIdx = (startYy * SCREENW + 2 * startXx);
			}

			col = _mm_add_epi32(colOffset, _mm_set1_epi32(startXx));
			__m128i aa0Col = _mm_mullo_epi32(aa0, col);
			__m128i aa1Col = _mm_mullo_epi32(aa1, col);
			__m128i aa2Col = _mm_mullo_epi32(aa2, col);

			row = _mm_add_epi32(rowOffset, _mm_set1_epi32(startYy));
			__m128i bb0Row = _mm_add_epi32(_mm_mullo_epi32(bb0, row), cc0);
			__m128i bb1Row = _mm_add_epi32(_mm_mullo_epi32(bb1, row), cc1);
			__m128i bb2Row = _mm_add_epi32(_mm_mullo_epi32(bb2, row), cc2);

			__m128i bb0Inc = _mm_slli_epi32(bb0, 1);
			__m128i bb1Inc = _mm_slli_epi32(bb1, 1);
			__m128i bb2Inc = _mm_slli_epi32(bb2, 1);

			// Incrementally compute Fab(x, y) for all the pixels inside the bounding box formed by (startX, endX) and (startY, endY)
			for(int r = startYy; r < endYy; r += 2,
											row  = _mm_add_epi32(row, _mm_set1_epi32(2)),
											rowIdx = rowIdx + 2 * SCREENW,
											bb0Row = _mm_add_epi32(bb0Row, bb0Inc),
											bb1Row = _mm_add_epi32(bb1Row, bb1Inc),
											bb2Row = _mm_add_epi32(bb2Row, bb2Inc))
			{
				// Compute barycentric coordinates 
				int idx = rowIdx;
				__m128i alpha = _mm_add_epi32(aa0Col, bb0Row);
				__m128i beta = _mm_add_epi32(aa1Col, bb1Row);
				__m128i gama = _mm_add_epi32(aa2Col, bb2Row);

				int idxIncr;
				if(gVisualizeDepthBuffer)
				{ 
					idxIncr = 2;
				}
				else
				{
					idxIncr = 4;
				}

				for(int c = startXx; c < endXx; c += 2,
												idx = idx + idxIncr,
												alpha = _mm_add_epi32(alpha, aa0Inc),
												beta  = _mm_add_epi32(beta, aa1Inc),
												gama  = _mm_add_epi32(gama, aa2Inc))
				{
					//Test Pixel inside triangle
					__m128i mask = _mm_cmplt_epi32(fxptZero, _mm_or_si128(_mm_or_si128(alpha, beta), gama));
					
					// Early out if all of this quad's pixels are outside the triangle.
					if(_mm_test_all_zeros(mask, mask))
					{
						continue;
					}

					// Compute barycentric-interpolated depth
			        __m128 depth = _mm_mul_ps(_mm_cvtepi32_ps(alpha), zz[0]);
					depth = _mm_add_ps(depth, _mm_mul_ps(_mm_cvtepi32_ps(beta), zz[1]));
					depth = _mm_add_ps(depth, _mm_mul_ps(_mm_cvtepi32_ps(gama), zz[2]));

					__m128 previousDepthValue;
					if(gVisualizeDepthBuffer)
					{
						previousDepthValue = _mm_set_ps(pDepthBuffer[idx], pDepthBuffer[idx + 1], pDepthBuffer[idx + SCREENW], pDepthBuffer[idx + SCREENW + 1]);
					}
					else
					{
						previousDepthValue = *(__m128*)&pDepthBuffer[idx];
					}

					__m128 depthMask  = _mm_cmpge_ps( depth, previousDepthValue);
					__m128i finalMask = _mm_and_si128( mask, _mm_castps_si128(depthMask));
					if(!_mm_test_all_zeros(finalMask, finalMask))
					{
						*mVisible = true;
						return; //early exit
					}
				}//for each column											
			}// for each row
		}// for each triangle
	}// for each set of SIMD# triangles
}