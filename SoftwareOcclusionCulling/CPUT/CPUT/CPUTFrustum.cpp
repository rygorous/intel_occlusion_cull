//--------------------------------------------------------------------------------------
// Copyright 2012 Intel Corporation
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
//--------------------------------------------------------------------------------------
#include "CPUTFrustum.h"
#include "CPUTCamera.h"

//-----------------------------------------------
void CPUTFrustum::InitializeFrustum( CPUTCamera *pCamera )
{
    InitializeFrustum(
        pCamera->GetNearPlaneDistance(),
        pCamera->GetFarPlaneDistance(),
        pCamera->GetAspectRatio(),
        pCamera->GetFov(),
        pCamera->GetPosition(),
        pCamera->GetLook(),
        pCamera->GetUp()
    );
}

float gScale2=1.0f;

//-----------------------------------------------
void CPUTFrustum::InitializeFrustum
(
    float nearClipDistance,
    float farClipDistance,
    float aspectRatio,
    float fov,
    const float3 &position,
    const float3 &look,
    const float3 &up
)
{
    // ******************************
    // This function computes the position of each of the frustum's eight points.
    // It also computes the normal of each of the frustum's six planes.
    // ******************************

    mNumFrustumVisibleModels = 0;
    mNumFrustumCulledModels  = 0;

    // We have the camera's up and look, but we also need right.
    float3 right = cross3( up, look );

    // Compute the position of the center of the near and far clip planes.
    float3 nearCenter = position + look * nearClipDistance;
    float3 farCenter  = position + look * farClipDistance;

    // Compute the width and height of the near and far clip planes
    float tanHalfFov = gScale2 * tanf(0.5f*fov);
    float halfNearWidth  = nearClipDistance * tanHalfFov;
    float halfNearHeight = halfNearWidth / aspectRatio;
    
    float halfFarWidth   = farClipDistance * tanHalfFov;
    float halfFarHeight  = halfFarWidth / aspectRatio;
    
    // Create two vectors each for the near and far clip planes.
    // These are the scaled up and right vectors.
    float3 upNear      = up    * halfNearHeight;
    float3 rightNear   = right * halfNearWidth;
    float3 upFar       = up    * halfFarHeight;
    float3 rightFar    = right * halfFarWidth;

    // Use the center positions and the up and right vectors
    // to compute the positions for the near and far clip plane vertices (four each)
    mpPosition[0] = nearCenter + upNear - rightNear; // near top left
    mpPosition[1] = nearCenter + upNear + rightNear; // near top right
    mpPosition[2] = nearCenter - upNear + rightNear; // near bottom right
    mpPosition[3] = nearCenter - upNear - rightNear; // near bottom left
    mpPosition[4] = farCenter  + upFar  - rightFar;  // far top left
    mpPosition[5] = farCenter  + upFar  + rightFar;  // far top right
    mpPosition[6] = farCenter  - upFar  + rightFar;  // far bottom right
    mpPosition[7] = farCenter  - upFar  - rightFar;  // far bottom left

    // Compute some of the frustum's edge vectors.  We will cross these
    // to get the normals for each of the six planes.
    float3 nearTop     = mpPosition[1] - mpPosition[0];
    float3 nearLeft    = mpPosition[3] - mpPosition[0];
    float3 topLeft     = mpPosition[4] - mpPosition[0];
    float3 bottomRight = mpPosition[2] - mpPosition[6];
    float3 farRight    = mpPosition[5] - mpPosition[6];
    float3 farBottom   = mpPosition[7] - mpPosition[6];

    mpNormal[0] = cross3(nearTop,     nearLeft).normalize();    // near clip plane
    mpNormal[1] = cross3(nearLeft,    topLeft).normalize();     // left
    mpNormal[2] = cross3(topLeft,     nearTop).normalize();     // top
    mpNormal[3] = cross3(farBottom,   bottomRight).normalize(); // bottom
    mpNormal[4] = cross3(bottomRight, farRight).normalize();    // right
    mpNormal[5] = cross3(farRight,    farBottom).normalize();   // far clip plane

	for (int i=0; i < 6; i++)
	{
		mPlaneX[i] = mpNormal[i].x;
		mPlaneY[i] = mpNormal[i].y;
		mPlaneZ[i] = mpNormal[i].z;
		mPlaneW[i] = -dot3(mpNormal[i], mpPosition[(i < 3) ? 0 : 6]);
	}

	for (int i=6; i < 8; i++)
	{
		mPlaneX[i] = 0;
		mPlaneY[i] = 0;
		mPlaneZ[i] = 0;
		mPlaneW[i] = -1.0f;
	}
}

//-----------------------------------------------
bool CPUTFrustum::IsVisible(
    const float3 &center,
    const float3 &half
){
	UINT ii;

	__m128 centerX	= _mm_set1_ps( center.x );
	__m128 centerY	= _mm_set1_ps( center.y );
	__m128 centerZ	= _mm_set1_ps( center.z );
	__m128 halfX	= _mm_set1_ps( half.x );
	__m128 halfY	= _mm_set1_ps( half.y );
	__m128 halfZ	= _mm_set1_ps( half.z );

	__m128 signMask	= _mm_castsi128_ps( _mm_set1_epi32( 0x80000000 ) );

	// Test the bounding box against 4 planes at a time
	for( ii=0; ii<2; ii++ )
	{
		__m128 planesX	= _mm_loadu_ps( &mPlaneX[ii * 4] );
		__m128 planesY	= _mm_loadu_ps( &mPlaneY[ii * 4] );
		__m128 planesZ	= _mm_loadu_ps( &mPlaneZ[ii * 4] );
		__m128 planesW	= _mm_loadu_ps( &mPlaneW[ii * 4] );

		// Sign for half[XYZ] so that dot product with plane normal would be maximal
		__m128 halfXSgn	= _mm_xor_ps( halfX, _mm_and_ps( planesX, signMask ) );
		__m128 halfYSgn = _mm_xor_ps( halfY, _mm_and_ps( planesY, signMask ) );
		__m128 halfZSgn = _mm_xor_ps( halfZ, _mm_and_ps( planesZ, signMask ) );

		// Bounding box corner to test (min corner)
		__m128 cornerX	= _mm_sub_ps( centerX, halfXSgn );
		__m128 cornerY	= _mm_sub_ps( centerY, halfYSgn );
		__m128 cornerZ	= _mm_sub_ps( centerZ, halfZSgn );

		// Dot product
		__m128 dot = planesW;
		dot = _mm_add_ps( dot, _mm_mul_ps( cornerX, planesX ) );
		dot = _mm_add_ps( dot, _mm_mul_ps( cornerY, planesY ) );
		dot = _mm_add_ps( dot, _mm_mul_ps( cornerZ, planesZ ) );

		// If not all negative, at least one plane rejected the box completely
		if ( !_mm_testc_si128( _mm_castps_si128( dot ), _mm_castps_si128( signMask ) ) )
		{
			mNumFrustumCulledModels++;
			return false;
		}
	}

    // Tested box against all six planes and none of the planes
    // had the full box outside.
    mNumFrustumVisibleModels++;
    return true;
}

