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


#ifndef TRANSFORMEDAABBOXSCALAR_H
#define TRANSFORMEDAABBOXSCALAR_H

#include "CPUT_DX11.h"
#include "Constants.h"
#include "HelperScalar.h"

class TransformedAABBoxScalar : public HelperScalar
{
	public:
		TransformedAABBoxScalar();
		~TransformedAABBoxScalar();
		void CreateAABBVertexIndexList(CPUTModelDX11 *pModel);
		void IsInsideViewFrustum(CPUTCamera *pcamera);
		void TransformAABBox();
		void RasterizeAndDepthTestAABBox(UINT *mpRenderTargetPixels);

		bool IsTooSmall(float4x4 *pViewMatrix, float4x4 *pProjMatrix, CPUTCamera *pCamera);

		inline void SetInsideViewFrustum(bool insideVF){mInsideViewFrustum = insideVF;}
		inline bool IsInsideViewFrustum(){ return mInsideViewFrustum;}
		inline void SetVisible(bool *visible){mVisible = visible;}
		inline void SetOccludeeSizeThreshold(float occludeeSizeThreshold){mOccludeeSizeThreshold = occludeeSizeThreshold;}

	private:
		CPUTModelDX11 *mpCPUTModel;
		float4x4 mWorldMatrix;
		float3  mBBCenter;
		float3  mBBHalf;
		float4x4 mCumulativeMatrix;


		float4  mBBVertexList[AABB_VERTICES];
		float4  mXformedPos[AABB_VERTICES];

		UINT	mBBIndexList[AABB_INDICES];
		bool   *mVisible;
		bool	mInsideViewFrustum;
		float	mOccludeeSizeThreshold;
		bool    mTooSmall;

		void Gather(float4 pOut[3], UINT triId);
};


#endif // TRANSFORMEDAABBOXSCALAR_H