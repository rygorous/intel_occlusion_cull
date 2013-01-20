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

#include "AABBoxRasterizerSSEST.h"

AABBoxRasterizerSSEST::AABBoxRasterizerSSEST()
	: AABBoxRasterizerSSE()
{

}

AABBoxRasterizerSSEST::~AABBoxRasterizerSSEST()
{

}

//--------------------------------------------------------------------
// Dtermine if the occludee model AABox is within the viewing frustum 
//--------------------------------------------------------------------
void AABBoxRasterizerSSEST::IsInsideViewFrustum(CPUTCamera *pCamera)
{
	mpCamera = pCamera;
	for(UINT i = 0; i < mNumModels; i++)
	{
		mpTransformedAABBox[i].IsInsideViewFrustum(mpCamera);
	}
}

//------------------------------------------------------------------------------
// For each occludee model
// * Transform the AABBox to screen space
// * Rasterize the triangles that make up the AABBox
// * Depth test the raterized triangles against the CPU rasterized depth buffer
//-----------------------------------------------------------------------------
void AABBoxRasterizerSSEST::TransformAABBoxAndDepthTest()
{
	mDepthTestTimer.StartTimer();

	for(UINT i = 0; i < mNumModels; i++)
	{
		mpVisible[i] = false;
		mpTransformedAABBox[i].SetVisible(&mpVisible[i]);
	
		if(mpTransformedAABBox[i].IsInsideViewFrustum() && !mpTransformedAABBox[i].IsTooSmall(mViewMatrix, mProjMatrix, mpCamera))
		{
			mpTransformedAABBox[i].TransformAABBox();
			mpTransformedAABBox[i].RasterizeAndDepthTestAABBox(mpRenderTargetPixels);
		}		
	}
	mDepthTestTime[mTimeCounter++] = mDepthTestTimer.StopTimer();
	mTimeCounter = mTimeCounter >= AVG_COUNTER ? 0 : mTimeCounter;
}