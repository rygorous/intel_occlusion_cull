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
#include "DepthBufferRasterizerSSE.h"

DepthBufferRasterizerSSE::DepthBufferRasterizerSSE()
	: DepthBufferRasterizer(),
	  mpTransformedModels1(NULL),
	  mNumModels1(0),
	  mpXformedPosOffset1(NULL),
	  mpStartV1(NULL),
	  mpStartT1(NULL),
	  mpModelIndexA(NULL),
	  mNumVertices1(0),
	  mNumTriangles1(0),
	  mpXformedPos1(NULL),
	  mpCamera(NULL),
	  mpRenderTargetPixels(NULL),
	  mNumRasterized(NULL),
	  mpBin(NULL),
	  mpNumTrisInBin(NULL),
	  mTimeCounter(0),
	  mpSummaryBuffer(NULL)
{
	mViewMatrix = (__m128*)_aligned_malloc(sizeof(float) * 4 * 4, 16);
	mProjMatrix = (__m128*)_aligned_malloc(sizeof(float) * 4 * 4, 16);
	mpSummaryBuffer = (float*)_aligned_malloc(sizeof(float) * (SCREENW/8) * (SCREENH/8), 64);

	for(UINT i = 0; i < AVG_COUNTER; i++)
	{
		mRasterizeTime[i] = 0.0;
	}
}

DepthBufferRasterizerSSE::~DepthBufferRasterizerSSE()
{
	SAFE_DELETE_ARRAY(mpTransformedModels1);
	SAFE_DELETE_ARRAY(mpXformedPosOffset1);
	SAFE_DELETE_ARRAY(mpStartV1);
	SAFE_DELETE_ARRAY(mpStartT1)
	SAFE_DELETE_ARRAY(mpModelIndexA);
	_aligned_free(mpXformedPos1);
	_aligned_free(mViewMatrix);
	_aligned_free(mProjMatrix);
	_aligned_free(mpSummaryBuffer);
}

//--------------------------------------------------------------------
// * Go through the asset set and determine the model count in it
// * Create data structures for all the models in the asset set
// * For each model create the place holders for the transformed vertices
//--------------------------------------------------------------------
void DepthBufferRasterizerSSE::CreateTransformedModels(CPUTAssetSet **mpAssetSet, UINT numAssetSets)
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

	mpTransformedModels1 = new TransformedModelSSE[mNumModels1];
	mpXformedPosOffset1 = new UINT[mNumModels1];
	mpStartV1 = new UINT[mNumModels1 + 1];
	mpStartT1 = new UINT[mNumModels1 + 1];

	mpModelIndexA = new UINT[mNumModels1];

	UINT modelId = 0;
	
	for(UINT assetId = 0; assetId < numAssetSets; assetId++)
	{
		for(UINT nodeId = 0; nodeId < mpAssetSet[assetId]->GetAssetCount(); nodeId++)
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

				mpStartV1[modelId] = mNumVertices1;
				mNumVertices1 += mpTransformedModels1[modelId].GetNumVertices();

				mpStartT1[modelId] = mNumTriangles1;
				mNumTriangles1 += mpTransformedModels1[modelId].GetNumTriangles();
				modelId++;
			}
			pRenderNode->Release();
		}
	}

	mpStartV1[modelId] = mNumVertices1;
	mpStartT1[modelId] = mNumTriangles1;
		
	//for x, y, z, w
	mpXformedPos1 = (__m128*)_aligned_malloc(sizeof(float) * 4 * mNumVertices1, 16);
	for(UINT i = 0; i < mNumModels1; i++)
	{
		mpTransformedModels1[i].SetXformedPos(&mpXformedPos1[mpStartV1[i]], mpStartV1[i]);
	}
}

//--------------------------------------------------------------------
// Clear depth buffer for a tile
//--------------------------------------------------------------------
void DepthBufferRasterizerSSE::ClearDepthTile(int startX, int startY, int endX, int endY)
{
	assert(startX % 2 == 0 && startY % 2 == 0);
	assert(endX % 2 == 0 && endY % 2 == 0);

	float* pDepthBuffer = (float*)mpRenderTargetPixels;
	int width = endX - startX;

	// Note we need to account for tiling pattern here
	for(int r = startY; r < endY; r += 2)
	{
		int rowIdx = r * SCREENW + 2 * startX;
		memset(&pDepthBuffer[rowIdx], 0, sizeof(float) * 2 * width);
	}
}

//--------------------------------------------------------------------
// Summarize the depth buffer for a tile
//--------------------------------------------------------------------
void DepthBufferRasterizerSSE::SummarizeDepthTile(int startX, int startY, int endX, int endY)
{
	assert(startX % 8 == 0 && startY % 8 == 0);
	assert(endX % 8 == 0 && endY % 8 == 0);

	const float* pDepthBuffer = (const float*)mpRenderTargetPixels;
	int x0s = startX / 8;
	int y0s = startY / 8;
	int x1s = endX / 8;
	int y1s = endY / 8;

	for(int yt = y0s; yt < y1s; yt++)
	{
		const float *srcRow = pDepthBuffer + (yt * 8) * SCREENW;
		float *dstRow = mpSummaryBuffer + yt * (SCREENW/8);

		for(int xt = x0s; xt <= x1s; xt++)
		{
			const float *src = srcRow + (xt * 8) * 2;

			static const int ofsTab[8] = 
			{
				0*SCREENW + 0*2, 0*SCREENW + 4*2,
				2*SCREENW + 0*2, 2*SCREENW + 4*2,
				4*SCREENW + 0*2, 4*SCREENW + 4*2,
				6*SCREENW + 0*2, 6*SCREENW + 4*2
			};
			__m128 min0 = _mm_set1_ps(1.0f);
			__m128 min1 = _mm_set1_ps(1.0f);

			for(int i = 0; i < 8; i++)
			{
				const float *srcQuad = src + ofsTab[i];
				min0 = _mm_min_ps(min0, _mm_load_ps(srcQuad + 0*2));
				min1 = _mm_min_ps(min1, _mm_load_ps(srcQuad + 2*2));
			}

			// merge
			min0 = _mm_min_ps(min0, min1);
			min0 = _mm_min_ps(min0, _mm_shuffle_ps(min0, min0, 0x4e)); // .zwxy
			min0 = _mm_min_ps(min0, _mm_shuffle_ps(min0, min0, 0xb1)); // .yxwz
			_mm_store_ss(&dstRow[xt], min0);
		}
	}
}

void DepthBufferRasterizerSSE::SetViewProj(float4x4 *viewMatrix, float4x4 *projMatrix)
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