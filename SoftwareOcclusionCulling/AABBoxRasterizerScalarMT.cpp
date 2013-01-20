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

#include "AABBoxRasterizerScalarMT.h"

AABBoxRasterizerScalarMT::AABBoxRasterizerScalarMT()
	: AABBoxRasterizerScalar()
{

}

AABBoxRasterizerScalarMT::~AABBoxRasterizerScalarMT()
{

}

//--------------------------------------------------------------------
// Create mNumDepthTestTasks tasks to determine if the occludee model 
// AABox is within the viewing frustum 
//--------------------------------------------------------------------
void AABBoxRasterizerScalarMT::IsInsideViewFrustum(CPUTCamera *pCamera)
{
	mpCamera = pCamera;
	
	gTaskMgr.CreateTaskSet(&AABBoxRasterizerScalarMT::IsInsideViewFrustum, this, mNumDepthTestTasks, NULL, 0, "Xform Vertices", &mAABBoxInsideViewFrustum);
	// Wait for the task set
	gTaskMgr.WaitForSet(mAABBoxInsideViewFrustum);
	// Release the task set
	gTaskMgr.ReleaseHandle(mAABBoxInsideViewFrustum);
	mAABBoxInsideViewFrustum = TASKSETHANDLE_INVALID;
}	

void AABBoxRasterizerScalarMT::IsInsideViewFrustum(VOID* taskData, INT context, UINT taskId, UINT taskCount)
{
	AABBoxRasterizerScalarMT *pAABB = (AABBoxRasterizerScalarMT*)taskData;
	pAABB->IsInsideViewFrustum(taskId, taskCount);
}

//-----------------------------------------------------------------------------
// * Determine the batch of occludee models each task should work on
// * For each model in the batch determine is the AABBox is inside view frustum
//-----------------------------------------------------------------------------
void AABBoxRasterizerScalarMT::IsInsideViewFrustum(UINT taskId, UINT taskCount)
{
	UINT numRemainingModels = mNumModels % taskCount;

	UINT numModelsPerTask1 = mNumModels / taskCount + 1;
	UINT numModelsPerTask2 = mNumModels / taskCount;

	UINT start, end;

	if(taskId < numRemainingModels)
	{
		start = taskId * numModelsPerTask1;
		end   = start +  numModelsPerTask1;
	}
	else
	{
		start = (numRemainingModels * numModelsPerTask1) + ((taskId - numRemainingModels) * numModelsPerTask2);
		end   = start +  numModelsPerTask2;
	}
	
	for(UINT i = start; i < end; i++)
	{
		mpTransformedAABBox[i].IsInsideViewFrustum(mpCamera);
	}
}

//-------------------------------------------------------------------------------
// Create mNumDepthTestTasks to tarnsform occludee AABBox, rasterize and depth test 
// to determine if occludee is visible or occluded
//-------------------------------------------------------------------------------
void AABBoxRasterizerScalarMT::TransformAABBoxAndDepthTest()
{
	mDepthTestTimer.StartTimer();

	gTaskMgr.CreateTaskSet(&AABBoxRasterizerScalarMT::TransformAABBoxAndDepthTest, this, mNumDepthTestTasks, NULL, 0, "Xform Vertices", &mAABBoxDepthTest);
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
void AABBoxRasterizerScalarMT::TransformAABBoxAndDepthTest(UINT taskId)
{
	UINT numRemainingModels = mNumModels % mNumDepthTestTasks;

	UINT numModelsPerTask1 = mNumModels / mNumDepthTestTasks + 1;
	UINT numModelsPerTask2 = mNumModels / mNumDepthTestTasks;

	UINT start, end;
	if(taskId < numRemainingModels)
	{
		start = taskId * numModelsPerTask1;
		end   = start +  numModelsPerTask1;
	}
	else
	{
		start = (numRemainingModels * numModelsPerTask1) + ((taskId - numRemainingModels) * numModelsPerTask2);
		end   = start +  numModelsPerTask2;
	}

	for(UINT i = start; i < end; i++)
	{
		mpVisible[i] = false;
		mpTransformedAABBox[i].SetVisible(&mpVisible[i]);
		
		if(mpTransformedAABBox[i].IsInsideViewFrustum() && !mpTransformedAABBox[i].IsTooSmall(mViewMatrix, mProjMatrix, mpCamera))
		{
			mpTransformedAABBox[i].TransformAABBox();
			mpTransformedAABBox[i].RasterizeAndDepthTestAABBox(mpRenderTargetPixels);
		}
	}
}

void AABBoxRasterizerScalarMT::TransformAABBoxAndDepthTest(VOID* pTaskData, INT context, UINT taskId, UINT taskCount)
{
	AABBoxRasterizerScalarMT *pAabbox = (AABBoxRasterizerScalarMT*)pTaskData;
	pAabbox->TransformAABBoxAndDepthTest(taskId);
}



