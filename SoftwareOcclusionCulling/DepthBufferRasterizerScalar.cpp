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
//-------------------------------------------------------------------------------------
#include "DepthBufferRasterizerScalar.h"

DepthBufferRasterizerScalar::DepthBufferRasterizerScalar()
	: DepthBufferRasterizer(),
	  mpTransformedModels1(NULL),
	  mNumModels1(0),
	  mpXformedPosOffset1(NULL),
	  mpStartV1(NULL),
	  mpStartT1(NULL),
	  mNumVertices1(0),
	  mNumTriangles1(0),
	  mpXformedPos1(NULL),
	  mpCamera(NULL),
	  mViewMatrix(NULL),
	  mProjMatrix(NULL),
	  mpRenderTargetPixels(NULL),
	  mNumRasterized(NULL),
	  mpBin(NULL),
	  mpBinModel(NULL),
	  mpBinMesh(NULL),
	  mpNumTrisInBin(NULL),
	  mTimeCounter(0)
{
	for(UINT i = 0; i < AVG_COUNTER; i++)
	{
		mRasterizeTime[i] = 0.0;
	}
}

DepthBufferRasterizerScalar::~DepthBufferRasterizerScalar()
{
	SAFE_DELETE_ARRAY(mpTransformedModels1);
	SAFE_DELETE_ARRAY(mpXformedPosOffset1);
	SAFE_DELETE_ARRAY(mpStartV1);
	SAFE_DELETE_ARRAY(mpStartT1)
	SAFE_DELETE_ARRAY(mpXformedPos1);
}

//--------------------------------------------------------------------
// * Go through the asset set and determine the model count in it
// * Create data structures for all the models in the asset set
// * For each model create the place holders for the transformed vertices
//--------------------------------------------------------------------
void DepthBufferRasterizerScalar::CreateTransformedModels(CPUTAssetSet **mpAssetSet, UINT numAssetSets)
{
	for(UINT assetId = 0; assetId < numAssetSets; assetId++)
	{
		for(UINT nodeId = 0; nodeId < mpAssetSet[assetId]->GetAssetCount(); nodeId++)
		{
			CPUTRenderNode* pRenderNode = NULL;
			CPUTResult result = mpAssetSet[assetId]->GetAssetByIndex(nodeId, &pRenderNode);
			ASSERT((CPUT_SUCCESS == result), _L ("Failed getting asset by index")); 
			if(pRenderNode->IsModel())
			{
				mNumModels1++;		
			}
			pRenderNode->Release();
		}	
	}

	mpTransformedModels1 = new TransformedModelScalar[mNumModels1];
	mpXformedPosOffset1 = new UINT[mNumModels1];
	mpStartV1 = new UINT[mNumModels1];
	mpStartT1 = new UINT[mNumModels1];

	mpStartV1[0] = mpStartT1[0] = 0;

	for(UINT assetId = 0, modelId = 0; assetId < numAssetSets; assetId++)
	{
		for(UINT nodeId = 0; nodeId< mpAssetSet[assetId]->GetAssetCount(); nodeId++)
		{
			CPUTRenderNode* pRenderNode = NULL;
			CPUTResult result = mpAssetSet[assetId]->GetAssetByIndex(nodeId, &pRenderNode);
			ASSERT((CPUT_SUCCESS == result), _L ("Failed getting asset by index")); 
			if(pRenderNode->IsModel())
			{
				CPUTModelDX11* model = (CPUTModelDX11*)pRenderNode;
				model = (CPUTModelDX11*)pRenderNode;
				mpTransformedModels1[modelId].CreateTransformedMeshes(model);
			
				mpXformedPosOffset1[modelId] = mpTransformedModels1[modelId].GetNumVertices();
	
				if(modelId > 0)
				{
					mpStartV1[modelId] = mNumVertices1;				
				}
				mNumVertices1 += mpTransformedModels1[modelId].GetNumVertices();
	
				if(modelId > 0)
				{
					mpStartT1[modelId] = mNumTriangles1;				
				}
				mNumTriangles1 += mpTransformedModels1[modelId].GetNumTriangles();
				modelId++;
			}
			pRenderNode->Release();
		}
	}

	mpStartV1[0] = 0;
	mpStartT1[0] = 0;
	
	//multiply by 4 for x, y, z, w
	mpXformedPos1 =  new float[mNumVertices1 * 4];
	for(UINT i = 0; i < mNumModels1; i++)
	{
		mpTransformedModels1[i].SetXformedPos((float4*)&mpXformedPos1[mpStartV1[i] * 4], mpStartV1[i]);
	}
}

void DepthBufferRasterizerScalar::SetViewProj(float4x4 *viewMatrix, float4x4 *projMatrix)
{
	mViewMatrix = viewMatrix;
	mProjMatrix = projMatrix;
}