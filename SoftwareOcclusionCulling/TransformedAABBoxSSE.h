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


#ifndef TRANSFORMEDAABBOXSSE_H
#define TRANSFORMEDAABBOXSSE_H

#include "CPUT_DX11.h"
#include "Constants.h"
#include "HelperSSE.h"

struct BoxTestSetup : public HelperSSE
{
	__m128 mViewProjViewport[4];
	float radiusThreshold;

	void Init(__m128 viewMatrix[4], __m128 projMatrix[4], CPUTCamera *pCamera, float occludeeSizeThreshold);
};

class TransformedAABBoxSSE : public HelperSSE
{
	public:
		TransformedAABBoxSSE();
		~TransformedAABBoxSSE();
		void CreateAABBVertexIndexList(CPUTModelDX11 *pModel);
		bool IsInsideViewFrustum(CPUTCamera *pCamera);
		void TransformAABBoxAndDepthTest();

		void MakeCumulativeMatrix(__m128 cumulativeMatrix[4], const BoxTestSetup &setup);
		bool IsTooSmall(const BoxTestSetup &setup, const __m128 cumulativeMatrix[4]);
		void TransformAABBox(__m128 *pXformedPos, const __m128 cumulativeMatrix[4]);

		bool RasterizeAndDepthTestAABBox(UINT *pRenderTargetPixels, const __m128 *pXformedPos); // returns if visible

	private:
		CPUTModelDX11 *mpCPUTModel;

		float3 mBBCenter;
		float  mRadiusSq;

		void Gather(vFloat4 pOut[3], UINT triId, const __m128 *pXformedPos);
};


#endif // TRANSFORMEDAABBOXSSE_H