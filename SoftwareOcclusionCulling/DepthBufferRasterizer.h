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
//--------------------------------------------------------------------------------------
#ifndef DEPTHBUFFERRASTERIZER_H
#define DEPTHBUFFERRASTERIZER_H

#include "CPUT_DX11.h"
#include "TaskMgrTBB.h"


class DepthBufferRasterizer
{
	public:
		DepthBufferRasterizer();
		virtual ~DepthBufferRasterizer();
		virtual void CreateTransformedModels(CPUTAssetSet **pAssetSet, UINT numAssetSets) = 0;
		virtual void TransformModelsAndRasterizeToDepthBuffer() = 0;

		virtual void ResetInsideFrustum() = 0;
		virtual void IsVisible(CPUTCamera *pCamera) = 0;
		virtual void SetViewProj(float4x4 *viewMatrix, float4x4 *projMatrix) = 0;
		virtual void SetCPURenderTargetPixels(UINT *pRenderTargetPixels) = 0;
		virtual void SetOccluderSizeThreshold(float occluderSizeThreshold) = 0;
		virtual inline void SetCamera(CPUTCamera *pCamera) = 0;

		virtual UINT GetNumOccluders() = 0;
		virtual UINT GetNumOccludersR2DB() = 0;
		virtual double GetRasterizeTime() = 0;
		virtual UINT GetNumTriangles() = 0;
		virtual UINT GetNumRasterizedTriangles() = 0;

	protected:
		TASKSETHANDLE mIsVisible;
		TASKSETHANDLE mXformMesh;
		TASKSETHANDLE mBinMesh;
		TASKSETHANDLE mRasterize;
};


#endif //DEPTHBUFFERRASTERIZER