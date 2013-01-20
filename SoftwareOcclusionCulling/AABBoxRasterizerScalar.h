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

#ifndef AABBOXRASTERIZERSCALAR_H
#define AABBOXRASTERIZERSCALAR_H

#include "AABBoxRasterizer.h"
#include "TransformedAABBoxScalar.h"

class AABBoxRasterizerScalar : public AABBoxRasterizer
{
	public:
		AABBoxRasterizerScalar();
		virtual ~AABBoxRasterizerScalar();
		void CreateTransformedAABBoxes(CPUTAssetSet **pAssetSet, UINT numAssetSets);
		
		void RenderVisible(CPUTAssetSet **pAssetSet,
						   CPUTRenderParametersDX &renderParams,
						   UINT numAssetSets);

		void Render(CPUTAssetSet **pAssetSet,
					CPUTRenderParametersDX &renderParams,
					UINT numAssetSets);

		inline void ResetInsideFrustum()
		{
			for(UINT i = 0; i < mNumModels; i++)
			{
				mpTransformedAABBox[i].SetInsideViewFrustum(true);
			}
		}

		void SetViewProjMatrix(float4x4 *viewMatrix, float4x4 *projMatrix);
		inline void SetCPURenderTargetPixels(UINT *pRenderTargetPixels){mpRenderTargetPixels = pRenderTargetPixels;}
		inline void SetDepthTestTasks(UINT numTasks){mNumDepthTestTasks = numTasks;}
		inline void SetOccludeeSizeThreshold(float occludeeSizeThreshold)
		{
			for(UINT i = 0; i < mNumModels; i++)
			{
				mpTransformedAABBox[i].SetOccludeeSizeThreshold(occludeeSizeThreshold);
			}
		}
		inline void SetCamera(CPUTCamera *pCamera) {mpCamera = pCamera;}	


		inline UINT GetNumOccludees() {return mNumModels;}
		inline UINT GetNumCulled() {return mNumCulled;}
		inline double GetDepthTestTime()
		{
			double averageTime = 0.0;
			for(UINT i = 0; i < AVG_COUNTER; i++)
			{
				averageTime += mDepthTestTime[i];
			}
			return averageTime / AVG_COUNTER;
		}

		inline UINT GetNumTriangles()
		{
			UINT numTris = 0;
			for(UINT i = 0; i < mNumModels; i++)
			{
				numTris += mpNumTriangles[i];
			}
			return numTris;
		}

		inline UINT GetNumCulledTriangles()
		{
			UINT numCulledTris = 0;
			for(UINT i = 0; i < mNumModels; i++)
			{
				numCulledTris += mpVisible[i] ? 0 : mpNumTriangles[i];
			}
			return numCulledTris;
		}

	protected:
		UINT mNumModels;
		TransformedAABBoxScalar *mpTransformedAABBox;
		UINT *mpNumTriangles;
		float4x4 *mViewMatrix;
		float4x4 *mProjMatrix;
		UINT *mpRenderTargetPixels;
		CPUTCamera *mpCamera;
		bool *mpVisible;
		UINT  mNumCulled;
		UINT mNumDepthTestTasks;
		float mOccludeeSizeThreshold;
		UINT mTimeCounter;

		double mDepthTestTime[AVG_COUNTER];
		CPUTTimerWin mDepthTestTimer;
};



#endif //AABBOXRASTERIZERSCALAR_H