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

#include "AABBoxRasterizerSSE.h"

AABBoxRasterizerSSE::AABBoxRasterizerSSE()
	: mNumModels(0),
	  mpTransformedAABBox(NULL),
	  mpNumTriangles(NULL),
	  mpRenderTargetPixels(NULL),
	  mpCamera(NULL),
	  mpVisible(NULL),
	  mNumCulled(0),
	  mNumDepthTestTasks(0),
	  mOccludeeSizeThreshold(0.0f),
	  mTimeCounter(0)
{
	mViewMatrix = (__m128*)_aligned_malloc(sizeof(float) * 4 * 4, 16);
	mProjMatrix = (__m128*)_aligned_malloc(sizeof(float) * 4 * 4, 16);

	for(UINT i = 0; i < AVG_COUNTER; i++)
	{
		mDepthTestTime[i] = 0.0;
	}
}

AABBoxRasterizerSSE::~AABBoxRasterizerSSE()
{
	_aligned_free(mViewMatrix);
	_aligned_free(mProjMatrix);
	SAFE_DELETE_ARRAY(mpVisible);
	SAFE_DELETE_ARRAY(mpTransformedAABBox);
	SAFE_DELETE_ARRAY(mpNumTriangles);
}

//--------------------------------------------------------------------
// * Go through the asset set and determine the model count in it
// * Create data structures aor all the models in the asset set
// * For each model create the axis aligned bounding box triangle 
//   vertex and index list
//--------------------------------------------------------------------
void AABBoxRasterizerSSE::CreateTransformedAABBoxes(CPUTAssetSet **pAssetSet, UINT numAssetSets)
{
	for(UINT assetId = 0;  assetId < numAssetSets; assetId++)
	{
		for(UINT nodeId = 0; nodeId < pAssetSet[assetId]->GetAssetCount(); nodeId++)
		{
			CPUTRenderNode* pRenderNode = NULL;
			CPUTResult result = pAssetSet[assetId]->GetAssetByIndex(nodeId, &pRenderNode);
			ASSERT((CPUT_SUCCESS == result), _L ("Failed getting asset by index")); 
			if(pRenderNode->IsModel())
			{
				mNumModels++;		
			}
			pRenderNode->Release();
		}
	}

	mpVisible = new bool[mNumModels];
	mpTransformedAABBox = new TransformedAABBoxSSE[mNumModels];
	mpNumTriangles = new UINT[mNumModels];
	
	for(UINT assetId = 0, modelId = 0; assetId < numAssetSets; assetId++)
	{
		for(UINT nodeId = 0; nodeId < pAssetSet[assetId]->GetAssetCount(); nodeId++)
		{
			CPUTRenderNode* pRenderNode = NULL;
			CPUTResult result = pAssetSet[assetId]->GetAssetByIndex(nodeId, &pRenderNode);
			ASSERT((CPUT_SUCCESS == result), _L ("Failed getting asset by index")); 
			if(pRenderNode->IsModel())
			{
				CPUTModelDX11 *pModel = (CPUTModelDX11*)pRenderNode;
				pModel = (CPUTModelDX11*)pRenderNode;
	
				mpTransformedAABBox[modelId].CreateAABBVertexIndexList(pModel);
				mpNumTriangles[modelId] = 0;
				for(int meshId = 0; meshId < pModel->GetMeshCount(); meshId++)
				{
					mpNumTriangles[modelId] += pModel->GetMesh(meshId)->GetTriangleCount();
				}
				modelId++;
			}
			pRenderNode->Release();
		}
	}
}

void AABBoxRasterizerSSE::SetViewProjMatrix(float4x4 *viewMatrix, float4x4 *projMatrix)
{
	mViewMatrix[0] = _mm_loadu_ps((float*)&viewMatrix->r0);
	mViewMatrix[1] = _mm_loadu_ps((float*)&viewMatrix->r1);
	mViewMatrix[2] = _mm_loadu_ps((float*)&viewMatrix->r2);
	mViewMatrix[3] = _mm_loadu_ps((float*)&viewMatrix->r3);

	mProjMatrix[0] = _mm_loadu_ps((float*)&projMatrix->r0);
	mProjMatrix[1] = _mm_loadu_ps((float*)&projMatrix->r1);
	mProjMatrix[2] = _mm_loadu_ps((float*)&projMatrix->r2);
	mProjMatrix[3] = _mm_loadu_ps((float*)&projMatrix->r3);
}

//------------------------------------------------------------------------
// Go through the list of models in the asset set and render only those 
// models that are marked as visible by the software occlusion culling test
//------------------------------------------------------------------------
void AABBoxRasterizerSSE::RenderVisible(CPUTAssetSet **pAssetSet,
										CPUTRenderParametersDX &renderParams,
										UINT numAssetSets)
{
	int count = 0;

	for(UINT assetId = 0, modelId = 0; assetId < numAssetSets; assetId++)
	{
		for(UINT nodeId = 0; nodeId < pAssetSet[assetId]->GetAssetCount(); nodeId++)
		{
			CPUTRenderNode* pRenderNode = NULL;
			CPUTResult result = pAssetSet[assetId]->GetAssetByIndex(nodeId, &pRenderNode);
			ASSERT((CPUT_SUCCESS == result), _L ("Failed getting asset by index")); 
			if(pRenderNode->IsModel())
			{
				if(mpVisible[modelId])
				{
					CPUTModelDX11* model = (CPUTModelDX11*)pRenderNode;
					model = (CPUTModelDX11*)pRenderNode;
					model->Render(renderParams);
					count++;
				}
				modelId++;			
			}
			pRenderNode->Release();
		}
	}
	mNumCulled =  mNumModels - count;
}

//------------------------------------------------------------------------
// Go through the list of models in the asset set and render only those 
// models that are not marked as too small by the software occlusion culling test
//------------------------------------------------------------------------
void AABBoxRasterizerSSE::Render(CPUTAssetSet **pAssetSet,
								 CPUTRenderParametersDX &renderParams,
								 UINT numAssetSets)
{
	int count = 0;

	for(UINT assetId = 0, modelId = 0; assetId < numAssetSets; assetId++)
	{
		for(UINT nodeId = 0; nodeId < pAssetSet[assetId]->GetAssetCount(); nodeId++)
		{
			CPUTRenderNode* pRenderNode = NULL;
			CPUTResult result = pAssetSet[assetId]->GetAssetByIndex(nodeId, &pRenderNode);
			ASSERT((CPUT_SUCCESS == result), _L ("Failed getting asset by index")); 
			if(pRenderNode->IsModel())
			{
				if(!mpTransformedAABBox[modelId].IsTooSmall(mViewMatrix, mProjMatrix, mpCamera))
				{
					CPUTModelDX11* model = (CPUTModelDX11*)pRenderNode;
					model = (CPUTModelDX11*)pRenderNode;
					model->Render(renderParams);
					count++;
				}
				modelId++;			
			}
			pRenderNode->Release();
		}
	}
	mNumCulled =  mNumModels - count;
}