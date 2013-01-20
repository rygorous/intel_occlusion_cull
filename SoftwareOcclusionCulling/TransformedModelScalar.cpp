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
#include "TransformedModelScalar.h"

TransformedModelScalar::TransformedModelScalar()
	: mpCPUTModel(NULL),
	  mNumMeshes(0),
	  mVisible(false),
	  mTooSmall(false),
	  mOccluderSizeThreshold(0.0),
	  mpMeshes(NULL),
	  mpXformedPos(NULL)
{

}

TransformedModelScalar::~TransformedModelScalar()
{
	SAFE_DELETE_ARRAY(mpMeshes);
}

//--------------------------------------------------------------------
// Create place holder for the transformed meshes for each model
//---------------------------------------------------------------------
void TransformedModelScalar::CreateTransformedMeshes(CPUTModelDX11 *pModel)
{
	mpCPUTModel = pModel;
	mNumMeshes = pModel->GetMeshCount();
	mWorldMatrix = *pModel->GetWorldMatrix();
	
	float3 center, half;
	pModel->GetBoundsObjectSpace(&center, &half);

	mBBCenterOS = float4(center, 1.0f);
	mBBHalfOS = float4(half, 0.0f); 

	mpMeshes = new TransformedMeshScalar[mNumMeshes];

	for(UINT i = 0; i < mNumMeshes; i++)
	{
		CPUTMeshDX11* pMesh = (CPUTMeshDX11*)pModel->GetMesh(i);
		mpMeshes[i].Initialize(pMesh);
	}
}

//------------------------------------------------------------------
// Determine is the occluder model is inside view frustum
//------------------------------------------------------------------
void TransformedModelScalar::IsVisible(CPUTCamera* pCamera)
{
	mpCPUTModel->GetBoundsWorldSpace(&mBBCenterWS, &mBBHalfWS);
	mVisible = pCamera->mFrustum.IsVisible(mBBCenterWS, mBBHalfWS);
}

//---------------------------------------------------------------------------------------------------
// Determine if the occluder size is sufficiently large enough to occlude other object sin the scene
// If so transform the occluder to screen space so that it can be rasterized to the cPU depth buffer
//---------------------------------------------------------------------------------------------------
void TransformedModelScalar::TransformMeshes(float4x4 *viewMatrix, 
										     float4x4 *projMatrix,
										     UINT start, 
										     UINT end,
											 CPUTCamera* pCamera)
{
	if(mVisible)
	{
		float radius = float3(mBBHalfOS.x, mBBHalfOS.y, mBBHalfOS.z).lengthSq();
		float fov = pCamera->GetFov();
		float tanOfHalfFov = tanf(fov * 0.5f);

		float4x4 cumulativeMatrix = mWorldMatrix * *viewMatrix;
		cumulativeMatrix = cumulativeMatrix * *projMatrix;
		cumulativeMatrix = cumulativeMatrix * viewportMatrix;
		
		float4 mBBCenterOSxForm = TransformCoords(mBBCenterOS,cumulativeMatrix);
	
		if(mBBCenterOSxForm.w > 1.0f)
		{
			float radiusDivW = radius / mBBCenterOSxForm.w;
			float r2DivW2DivTanFov = radiusDivW / tanOfHalfFov;
		
			mTooSmall = r2DivW2DivTanFov < (mOccluderSizeThreshold * mOccluderSizeThreshold) ?  true : false;
		}
		else
		{
			// BB center is behind the near clip plane, making screen-space radius meaningless.
            // Assume visible.  This should be a safe assumption, as the frustum test says the bbox is visible.
            mTooSmall = false;
		}

		if(!mTooSmall)
		{
			UINT totalNumVertices = 0;
			for(UINT meshId = 0; meshId < mNumMeshes; meshId++)
			{		
				totalNumVertices +=  mpMeshes[meshId].GetNumVertices();
				if(totalNumVertices < start)
				{
					continue;
				}
				mpMeshes[meshId].TransformVertices(cumulativeMatrix, start, end);
			}
		}
	}
}

//------------------------------------------------------------------------------------
// If the occluder is sufficiently large enough to occlude other objects in the scene 
// bin the triangles that make up the occluder into tiles to speed up rateraization
// Single threaded version
//------------------------------------------------------------------------------------
void TransformedModelScalar::BinTransformedTrianglesST(UINT taskId,
													   UINT modelId,
													   UINT start,
													   UINT end,
													   UINT* pBin,
													   USHORT* pBinModel,
													   USHORT* pBinMesh,
													   USHORT* pNumTrisInBin)
{
	if(mVisible && !mTooSmall)
	{
		UINT totalNumTris = 0;
		for(UINT meshId = 0; meshId < mNumMeshes; meshId++)
		{
			totalNumTris += mpMeshes[meshId].GetNumTriangles();
			if(totalNumTris < start)
			{
				continue;
			}
			mpMeshes[meshId].BinTransformedTrianglesST(taskId, modelId, meshId, start, end, pBin, pBinModel, pBinMesh, pNumTrisInBin);
		}
	}
}

//------------------------------------------------------------------------------------
// If the occluder is sufficiently large enough to occlude other objects in the scene 
// bin the triangles that make up the occluder into tiles to speed up rateraization
// Multi threaded version
//------------------------------------------------------------------------------------
void TransformedModelScalar::BinTransformedTrianglesMT(UINT taskId,
													   UINT modelId,
													   UINT start,
													   UINT end,
													   UINT* pBin,
													   USHORT* pBinModel,
													   USHORT* pBinMesh,
													   USHORT* pNumTrisInBin)
{
	if(mVisible && !mTooSmall)
	{
		UINT totalNumTris = 0;
		for(UINT meshId = 0; meshId < mNumMeshes; meshId++)
		{
			totalNumTris += mpMeshes[meshId].GetNumTriangles();
			if(totalNumTris < start)
			{
				continue;
			}
			mpMeshes[meshId].BinTransformedTrianglesMT(taskId, modelId, meshId, start, end, pBin, pBinModel, pBinMesh, pNumTrisInBin);
		}
	}
}

void TransformedModelScalar::Gather(float* xformedPos,
									UINT meshId, 
									UINT triId, 
									UINT lane)
{
	mpMeshes[meshId].GetOneTriangleData(xformedPos, triId, lane);
}