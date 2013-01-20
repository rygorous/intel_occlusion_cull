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

#include "TransformedAABBoxScalar.h"

TransformedAABBoxScalar::TransformedAABBoxScalar()
	: mpCPUTModel(NULL),
	  mVisible(NULL),
	  mInsideViewFrustum(true),
	  mOccludeeSizeThreshold(0.0f),
	  mTooSmall(false)
{

}

TransformedAABBoxScalar::~TransformedAABBoxScalar()
{

}

//--------------------------------------------------------------------------
// Get the bounding box center and half vector
// Create the vertex and index list for the triangles that make up the bounding box
//--------------------------------------------------------------------------
void TransformedAABBoxScalar::CreateAABBVertexIndexList(CPUTModelDX11 *pModel)
{
	mpCPUTModel = pModel;
	mWorldMatrix = *pModel->GetWorldMatrix();
	pModel->GetBoundsObjectSpace(&mBBCenter, &mBBHalf);
	
	float3 min = mBBCenter - mBBHalf;
	float3 max = mBBCenter + mBBHalf;

	//Top 4 vertices in BB
	mBBVertexList[0] = float4(max, 1.0f);
	mBBVertexList[1] = float4(min.x, max.y, max.z, 1.0f);
	mBBVertexList[2] = float4(min.x, max.y, min.z, 1.0f);
	mBBVertexList[3] = float4(max.x, max.y, min.z, 1.0f);
	// Bottom 4 vertices in BB
	mBBVertexList[4] = float4(max.x, min.y, min.z, 1.0f);
	mBBVertexList[5] = float4(max.x, min.y, max.z, 1.0f);
	mBBVertexList[6] = float4(min.x, min.y, max.z, 1.0f);
	mBBVertexList[7] = float4(min, 1.0f);

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
void TransformedAABBoxScalar::IsInsideViewFrustum(CPUTCamera *pCamera)
{
	float3 mBBCenterWS;
	float3 mBBHalfWS;
	mpCPUTModel->GetBoundsWorldSpace(&mBBCenterWS, &mBBHalfWS);
	mInsideViewFrustum = pCamera->mFrustum.IsVisible(mBBCenterWS, mBBHalfWS);
}

//----------------------------------------------------------------------------
// Determine if the occludee size is too small and if so avoid drawing it
//----------------------------------------------------------------------------
bool TransformedAABBoxScalar::IsTooSmall(float4x4 *pViewMatrix, float4x4 *pProjMatrix, CPUTCamera *pCamera)
{
	float radius = mBBHalf.lengthSq();
	float fov = pCamera->GetFov();
	float tanOfHalfFov = tanf(fov * 0.5f);
	mTooSmall = false;

	mCumulativeMatrix = mWorldMatrix * *pViewMatrix;
	mCumulativeMatrix = mCumulativeMatrix * *pProjMatrix;
	mCumulativeMatrix = mCumulativeMatrix * viewportMatrix;
	float4 mBBCenterOSxForm = TransformCoords(float4(mBBCenter, 1.0f), mCumulativeMatrix);

	float w = mBBCenterOSxForm.w;
	if( w > 1.0f )
	{
		float radiusDivW = radius / w;
		float r2DivW2DivTanFov = radiusDivW / tanOfHalfFov;
		mTooSmall = r2DivW2DivTanFov < (mOccludeeSizeThreshold*mOccludeeSizeThreshold) ?  true : false;
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
void TransformedAABBoxScalar::TransformAABBox()
{
	for(UINT i = 0; i < AABB_VERTICES; i++)
	{
		mXformedPos[i] = TransformCoords(mBBVertexList[i], mCumulativeMatrix);
		float oneOverW = 1.0f/max(mXformedPos[i].w, 0.0000001f);
		mXformedPos[i] = mXformedPos[i] * oneOverW;
		mXformedPos[i].w = oneOverW;
	}
}

void TransformedAABBoxScalar::Gather(float4 pOut[3], UINT triId)
{
	for(UINT i = 0; i < 3; i++)
	{
		UINT index = mBBIndexList[(triId * 3) + i];
		pOut[i] = mXformedPos[index];	
	}
}

//-----------------------------------------------------------------------------------------
// Rasterize the occludee AABB and depth test it against the CPU rasterized depth buffer
// If any of the rasterized AABB pixels passes the depth test exit early and mark the occludee
// as visible. If all rasterized AABB pixels are occluded then the occludee is culled
//-----------------------------------------------------------------------------------------
void TransformedAABBoxScalar::RasterizeAndDepthTestAABBox(UINT *mpRenderTargetPixels)
{
	int fxptZero = 0;
	float* pDepthBuffer = (float*)mpRenderTargetPixels; 
	
	// Rasterize the AABB triangles
	for(UINT i = 0; i < AABB_TRIANGLES; i++)
	{
		float4 xformedPos[3];
		Gather(xformedPos, i);

		// use fixed-point only for X and Y.  Avoid work for Z and W.
        int4 xformedFxPtPos[3] = {
            int4(xformedPos[0]),
            int4(xformedPos[1]),
            int4(xformedPos[2])
        };

		// Fab(x, y) =     Ax       +       By     +      C              = 0
		// Fab(x, y) = (ya - yb)x   +   (xb - xa)y + (xa * yb - xb * ya) = 0
		// Compute A = (ya - yb) for the 3 line segments that make up each triangle
		int A0 = xformedFxPtPos[1].y - xformedFxPtPos[2].y;
		int A1 = xformedFxPtPos[2].y - xformedFxPtPos[0].y;
		int A2 = xformedFxPtPos[0].y - xformedFxPtPos[1].y;

		// Compute B = (xb - xa) for the 3 line segments that make up each triangle
		int B0 = xformedFxPtPos[2].x - xformedFxPtPos[1].x;
		int B1 = xformedFxPtPos[0].x - xformedFxPtPos[2].x;
		int B2 = xformedFxPtPos[1].x - xformedFxPtPos[0].x;

		// Compute C = (xa * yb - xb * ya) for the 3 line segments that make up each triangle
		int C0 = xformedFxPtPos[1].x * xformedFxPtPos[2].y - xformedFxPtPos[2].x * xformedFxPtPos[1].y;
		int C1 = xformedFxPtPos[2].x * xformedFxPtPos[0].y - xformedFxPtPos[0].x * xformedFxPtPos[2].y;
		int C2 = xformedFxPtPos[0].x * xformedFxPtPos[1].y - xformedFxPtPos[1].x * xformedFxPtPos[0].y;

		// Compute triangle area
		int triArea = A0 * xformedFxPtPos[0].x + B0 * xformedFxPtPos[0].y + C0;
		float oneOverTriArea = (1.0f/float(triArea));

		// Use bounding box traversal strategy to determine which pixels to rasterize 
		int startX = max(min(min(xformedFxPtPos[0].x, xformedFxPtPos[1].x), xformedFxPtPos[2].x), 0) & int(0xFFFFFFFE);
		int endX   = min(max(max(xformedFxPtPos[0].x, xformedFxPtPos[1].x), xformedFxPtPos[2].x) + 1, SCREENW);

		int startY = max(min(min(xformedFxPtPos[0].y, xformedFxPtPos[1].y), xformedFxPtPos[2].y), 0) & int(0xFFFFFFFE);
		int endY   = min(max(max(xformedFxPtPos[0].y, xformedFxPtPos[1].y), xformedFxPtPos[2].y) + 1, SCREENH);

		float zz[3];
		for(UINT vv = 0; vv < 3; vv++)
		{
			// If W (holding 1/w in our case) is not between 0 and 1,
            // then vertex is behind near clip plane (1.0 in our case.
            // If W < 1, then verify 1/W > 1 (for W>0), and 1/W < 0 (for W < 0).
			if((xformedPos[vv].w <= 0.0f || xformedPos[vv].w >= 1.0f))
			{
				*mVisible = true;
                return;
			}
			zz[vv] = xformedPos[vv].z;
		}

		//Skip triangle if area is zero 
		if(triArea <= 0)
		{
			continue;
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

				if(finalMask == 1)
				{
					*mVisible = true;
					return; // early exit
				}				
			}//for each column											
		}// for each row
	}// for each triangle
}
