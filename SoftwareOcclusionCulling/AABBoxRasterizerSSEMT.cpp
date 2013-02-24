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

#include "AABBoxRasterizerSSEMT.h"
#include "HelperMT.h"

AABBoxRasterizerSSEMT::AABBoxRasterizerSSEMT()
	: AABBoxRasterizerSSE()
{

}

AABBoxRasterizerSSEMT::~AABBoxRasterizerSSEMT()
{

}

//--------------------------------------------------------------------
// Create mNumDepthTestTasks tasks to determine if the occludee model 
// AABox is within the viewing frustum 
//--------------------------------------------------------------------
void AABBoxRasterizerSSEMT::IsInsideViewFrustum(CPUTCamera *pCamera)
{
	mpCamera = pCamera;
	gTaskMgr.CreateTaskSet(&AABBoxRasterizerSSEMT::IsInsideViewFrustum, this, mNumDepthTestTasks, NULL, 0, "Xform Vertices", &mAABBoxInsideViewFrustum);
	// Wait for the task set
	gTaskMgr.WaitForSet(mAABBoxInsideViewFrustum);
	// Release the task set
	gTaskMgr.ReleaseHandle(mAABBoxInsideViewFrustum);
	mAABBoxInsideViewFrustum = TASKSETHANDLE_INVALID;
}

void AABBoxRasterizerSSEMT::IsInsideViewFrustum(VOID* taskData, INT context, UINT taskId, UINT taskCount)
{
	AABBoxRasterizerSSEMT *pAABB = (AABBoxRasterizerSSEMT*)taskData;
	pAABB->IsInsideViewFrustum(taskId, taskCount);
}

//-----------------------------------------------------------------------------
// * Determine the batch of occludee models each task should work on
// * For each model in the batch determine is the AABBox is inside view frustum
//-----------------------------------------------------------------------------
void AABBoxRasterizerSSEMT::IsInsideViewFrustum(UINT taskId, UINT taskCount)
{
	UINT start, end;
	GetWorkExtent(&start, &end, taskId, taskCount, mNumModels);

	CalcInsideFrustum(&mpCamera->mFrustum, start, end);
}

//-------------------------------------------------------------------------------
// Create mNumDepthTestTasks to tarnsform occludee AABBox, rasterize and depth test 
// to determine if occludee is visible or occluded
//-------------------------------------------------------------------------------
void AABBoxRasterizerSSEMT::TransformAABBoxAndDepthTest()
{
	mDepthTestTimer.StartTimer();

	gTaskMgr.CreateTaskSet(&AABBoxRasterizerSSEMT::TransformAABBoxAndDepthTest, this, mNumDepthTestTasks, NULL, 0, "Xform Vertices", &mAABBoxDepthTest);
	// Wait for the task set
	gTaskMgr.WaitForSet(mAABBoxDepthTest);
	// Release the task set
	gTaskMgr.ReleaseHandle(mAABBoxDepthTest);
	mAABBoxDepthTest = TASKSETHANDLE_INVALID;
	
	mDepthTestTime[mTimeCounter++] = mDepthTestTimer.StopTimer();
	mTimeCounter = mTimeCounter >= AVG_COUNTER ? 0 : mTimeCounter; 
}

//--------------------------------------------------------------------------------
// Determine the batch of occludee models each task should work on
// For each occludee model in the batch
// * Transform the AABBox to screen space
// * Rasterize the triangles that make up the AABBox
// * Depth test the raterized triangles against the CPU rasterized depth buffer
//--------------------------------------------------------------------------------
void AABBoxRasterizerSSEMT::TransformAABBoxAndDepthTest(UINT taskId)
{
	BoxTestSetup setup;
	setup.Init(mViewMatrix, mProjMatrix, mpCamera, mOccludeeSizeThreshold);

	__m128 xformedPos[AABB_VERTICES];
	__m128 cumulativeMatrix[4];

	static const UINT kChunkSize = 64;
	for(UINT base = taskId*kChunkSize; base < mNumModels; base += mNumDepthTestTasks * kChunkSize)
	{
		UINT end = min(base + kChunkSize, mNumModels);
		for(UINT i = base; i < end; i++)
		{
			mpVisible[i] = false;

			if(mpBBoxVisible[i] && !mpTransformedAABBox[i].IsTooSmall(setup, cumulativeMatrix))
			{
				if(mpTransformedAABBox[i].TransformAABBox(xformedPos, cumulativeMatrix))
					mpVisible[i] = mpTransformedAABBox[i].RasterizeAndDepthTestAABBox(mpRenderTargetPixels, xformedPos);
				else
					mpVisible[i] = true;
			}
		}
	}
}

void AABBoxRasterizerSSEMT::TransformAABBoxAndDepthTest(VOID* pTaskData, INT context, UINT taskId, UINT taskCount)
{
	AABBoxRasterizerSSEMT *pAabbox = (AABBoxRasterizerSSEMT*)pTaskData;
	pAabbox->TransformAABBoxAndDepthTest(taskId);
}