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

#ifndef TRANSFORMEDMODELSSE_H
#define TRANSFORMEDMODELSSE_H

#include "CPUT_DX11.h"
#include "TransformedMeshSSE.h"
#include "HelperSSE.h"

class TransformedModelSSE : public HelperSSE
{
	public:
		TransformedModelSSE();
		~TransformedModelSSE();
		void CreateTransformedMeshes(CPUTModelDX11 *pModel);
		void IsVisible(CPUTCamera *pCamera);
		void TransformMeshes(__m128 *viewMatrix, 
					    	 __m128 *projMatrix,
							 UINT start, 
							 UINT end,
							 CPUTCamera *pCamera);

		void BinTransformedTrianglesST(UINT taskId,
									   UINT modelId,
									   UINT start,
									   UINT end,
									   UINT* pBin,
									   USHORT* pBinModel,
									   USHORT* pBinMesh,
									   USHORT* pNumTrisInBin);

		void BinTransformedTrianglesMT(UINT taskId,
									   UINT modelId,
									   UINT start,
									   UINT end,
									   UINT* pBin,
									   USHORT* pBinModel,
									   USHORT* pBinMesh,
									   USHORT* pNumTrisInBin);

		void Gather(float* xformedPos,
			        UINT meshId, 
					UINT triId, 
					UINT lane);

		inline UINT GetNumVertices()
		{
			UINT numVertices = 0;
			for(UINT i = 0; i < mNumMeshes; i++)
			{
				numVertices += mpMeshes[i].GetNumVertices();
			}
			return numVertices;
		}

		inline UINT GetNumTriangles()
		{
			UINT numTriangles = 0;
			for(UINT i = 0; i < mNumMeshes; i++)
			{
				numTriangles += mpMeshes[i].GetNumTriangles();
			}
			return numTriangles;
		}

		inline void SetXformedPos(__m128 *pXformedPos, UINT modelStart)
		{
			mpXformedPos = pXformedPos;

			mpMeshes[0].SetXformedPos(mpXformedPos);
			mpMeshes[0].SetVertexStart(modelStart);
			
			UINT numVertices = 0;
			numVertices += mpMeshes[0].GetNumVertices();
			
			for(UINT i = 1; i < mNumMeshes; i++)
			{
				mpMeshes[i].SetXformedPos((mpXformedPos + numVertices));
				mpMeshes[i].SetVertexStart(modelStart + numVertices);
				numVertices += mpMeshes[i].GetNumVertices(); 
			}
		}

		inline void SetOccluderSizeThreshold(float occluderSizeThreshold)
		{
			mOccluderSizeThreshold = occluderSizeThreshold;
		}

		inline void SetVisible(bool visible){mVisible = visible;}

		inline bool IsRasterized2DB()
		{
			return (mVisible && !mTooSmall);
		}

	private:
		CPUTModelDX11 *mpCPUTModel;
		UINT mNumMeshes;
		__m128 *mWorldMatrix;
		__m128 *mViewMatrix;
		__m128 *mProjMatrix;
		__m128 *mViewPortMatrix;
				
		float3 mBBCenterWS;
		float3 mBBHalfWS;
		bool mVisible;
		bool mTooSmall;
		float mOccluderSizeThreshold;

		float4 mBBCenterOS;
		float4 mBBHalfOS;
		TransformedMeshSSE *mpMeshes;
		__m128 *mpXformedPos;
};

#endif