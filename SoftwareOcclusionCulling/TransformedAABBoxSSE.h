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

enum PreTestResult
{
	ePT_INVISIBLE,
	ePT_VISIBLE,
	ePT_UNSURE,
};

class TransformedAABBoxSSE : public HelperSSE
{
	public:
		void CreateAABBVertexIndexList(CPUTModelDX11 *pModel);
		void TransformAABBoxAndDepthTest();

		bool IsTooSmall(const BoxTestSetup &setup, __m128 cumulativeMatrix[4]);

		PreTestResult TransformAndPreTestAABBox(__m128 xformedPos[], const __m128 cumulativeMatrix[4]);

		bool RasterizeAndDepthTestAABBox(UINT *pRenderTargetPixels, const __m128 xformedPos[]);

	private:
		float4x4 mWorldMatrix;

		float3 mBBCenter;
		float3 mBBHalf;
		float  mRadiusSq;

		void Gather(vFloat4 pOut[3], UINT triId, const __m128 xformedPos[]);
};


#endif // TRANSFORMEDAABBOXSSE_H