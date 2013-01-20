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

#ifndef AABBOXRASTERIZER_H
#define AABBOXRASTERIZER_H

#include "CPUT_DX11.h"
#include "TaskMgrTBB.h"

class AABBoxRasterizer
{
	public:
		AABBoxRasterizer();
		virtual ~AABBoxRasterizer();
		virtual void CreateTransformedAABBoxes(CPUTAssetSet **pAssetSet, UINT numAssetSets) = 0;
		virtual void TransformAABBoxAndDepthTest() = 0;
		virtual void RenderVisible(CPUTAssetSet **pAssetSet,
								   CPUTRenderParametersDX &renderParams,
								   UINT numAssetSets) = 0;
		virtual void Render(CPUTAssetSet **pAssetSet,
							CPUTRenderParametersDX &renderParams,
							UINT numAssetSets) = 0;

		virtual void ResetInsideFrustum() = 0; 
		virtual void IsInsideViewFrustum(CPUTCamera *pCamera) = 0;
		virtual void SetViewProjMatrix(float4x4 *viewMatrix, float4x4 *projMatrix) = 0;
		virtual void SetCPURenderTargetPixels(UINT *pRenderTargetPixels) = 0;
		virtual void SetDepthTestTasks(UINT numTasks) = 0;
		virtual void SetOccludeeSizeThreshold(float occludeeSizeThreshold) = 0;
		virtual void SetCamera(CPUTCamera *pCamera) = 0;

		virtual UINT GetNumOccludees() = 0;
		virtual UINT GetNumCulled() = 0;
		virtual double GetDepthTestTime() = 0;
		virtual UINT GetNumTriangles() = 0;
		virtual UINT GetNumCulledTriangles() = 0;

	protected:
		TASKSETHANDLE mAABBoxDepthTest;
		TASKSETHANDLE mAABBoxInsideViewFrustum;

};



#endif //AABBOXRASTERIZER_H