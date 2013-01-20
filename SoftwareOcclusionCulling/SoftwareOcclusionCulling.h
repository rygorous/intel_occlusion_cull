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
#ifndef __CPUT_SAMPLESTARTDX11_H__
#define __CPUT_SAMPLESTARTDX11_H__

#include <d3dx9math.h>
#include <stdio.h>
#include "CPUT_DX11.h"
#include <D3D11.h>
#include <xnamath.h>
#include <time.h>
#include "CPUTSprite.h"

#include "DepthBufferRasterizerScalarST.h"
#include "DepthBufferRasterizerScalarMT.h"

#include "DepthBufferRasterizerSSEST.h"
#include "DepthBufferRasterizerSSEMT.h"

#include "AABBoxRasterizerScalarST.h"
#include "AABBoxRasterizerScalarMT.h"

#include "AABBoxRasterizerSSEST.h"
#include "AABBoxRasterizerSSEMT.h"

#include "TaskMgrTBB.h"

enum SOC_TYPE
{
	SCALAR_TYPE,
	SSE_TYPE,
};

//-----------------------------------------------------------------------------
class MySample : public CPUT_DX11
{
private:
    float                  mfElapsedTime;
   
    CPUTCameraController  *mpCameraController;
    CPUTSprite            *mpDebugSprite;

    CPUTAssetSet          *mpShadowCameraSet;
    CPUTRenderTargetDepth *mpShadowRenderTarget;

    CPUTText              *mpFPSCounter;
	CPUTDropdown		  *mpTypeDropDown;

	CPUTText			  *mpOccludersText;
	CPUTText			  *mpNumOccludersText;
	CPUTText			  *mpOccludersR2DBText;
	CPUTText			  *mpOccluderTrisText;
	CPUTText			  *mpOccluderRasterizedTrisText;
	CPUTText		      *mpRasterizeTimeText;
	CPUTSlider			  *mpOccluderSizeSlider;

	CPUTText			  *mpOccludeesText;
	CPUTText			  *mpNumOccludeesText;
	CPUTText			  *mpCulledText;
	CPUTText			  *mpVisibleText;
	CPUTText			  *mpOccludeeTrisText;
	CPUTText			  *mpCulledTrisText;
	CPUTText			  *mpVisibleTrisText;
	CPUTText			  *mpDepthTestTimeText;
	CPUTSlider			  *mpOccludeeSizeSlider;

	CPUTCheckbox		  *mpCullingCheckBox;
	CPUTCheckbox		  *mpFCullingCheckBox;
	CPUTCheckbox		  *mpDBCheckBox;
	CPUTCheckbox		  *mpBBCheckBox;
	CPUTCheckbox		  *mpTasksCheckBox;
	CPUTCheckbox		  *mpVsyncCheckBox;

	CPUTText		      *mpDrawCallsText;
	CPUTSlider			  *mpDepthTestTaskSlider;

	CPUTAssetSet	      *mpAssetSetDBR[OCCLUDER_SETS];
	CPUTAssetSet		  *mpAssetSetAABB[OCCLUDEE_SETS];
	CPUTAssetSet		  *mpAssetSetSky;

	ID3D11Texture2D         *mpCPURenderTarget;
	ID3D11Texture2D         *mpBackBuffer;
	ID3D11RenderTargetView  *mpRTView;

	SOC_TYPE mSOCType;
	DepthBufferRasterizer	 		*mpDBR;
	DepthBufferRasterizerScalarST	*mpDBRScalarST;
	DepthBufferRasterizerScalarMT	*mpDBRScalarMT;
	DepthBufferRasterizerSSEST		*mpDBRSSEST;
	DepthBufferRasterizerSSEMT		*mpDBRSSEMT;

	AABBoxRasterizer				*mpAABB;
	AABBoxRasterizerScalarST		*mpAABBScalarST;
	AABBoxRasterizerScalarMT		*mpAABBScalarMT;
	AABBoxRasterizerSSEST			*mpAABBSSEST;
	AABBoxRasterizerSSEMT			*mpAABBSSEMT;

	UINT				mNumOccluders;
	UINT				mNumOccludersR2DB;
	UINT				mNumOccluderTris;
	UINT    			mNumOccluderRasterizedTris;
	double				mRasterizeTime;
	float				mOccluderSizeThreshold;
	
	UINT				mNumOccludees;
	UINT				mNumCulled;
	UINT				mNumVisible;
	UINT	   			mNumOccludeeTris;
	UINT    			mNumOccludeeCulledTris;
	UINT    			mNumOccludeeVisibleTris;
	double				mDepthTestTime;
	float				mOccludeeSizeThreshold;

	bool				mEnableCulling;
	bool				mEnableFCulling;
	bool				mViewDepthBuffer;
	bool				mViewBoundingBox;
	bool				mEnableTasks;

	UINT				mNumDrawCalls;
	UINT				mNumDepthTestTasks;

public:
    MySample() :
        mpCameraController(NULL),
        mpDebugSprite(NULL),
        mpShadowCameraSet(NULL),
        mpShadowRenderTarget(NULL),
        mpFPSCounter(NULL),
		mpTypeDropDown(NULL),
		mpOccludersText(NULL),
		mpNumOccludersText(NULL),
		mpOccludersR2DBText(NULL),
		mpOccluderTrisText(NULL),
		mpOccluderRasterizedTrisText(NULL),
		mpRasterizeTimeText(NULL),
		mpOccluderSizeSlider(NULL),
		mpOccludeesText(NULL),
		mpNumOccludeesText(NULL),
		mpCulledText(NULL),
		mpVisibleText(NULL),
		mpOccludeeTrisText(NULL),
		mpCulledTrisText(NULL),
		mpVisibleTrisText(NULL),
		mpDepthTestTimeText(NULL),
		mpOccludeeSizeSlider(NULL),
		mpCullingCheckBox(NULL),
		mpFCullingCheckBox(NULL),
		mpDBCheckBox(NULL),
		mpBBCheckBox(NULL),
		mpTasksCheckBox(NULL),
		mpVsyncCheckBox(NULL),
		mpDrawCallsText(NULL),
		mpDepthTestTaskSlider(NULL),
		mpCPURenderTarget(NULL),
		mpBackBuffer(NULL),
		mpRTView(NULL),
		mSOCType(SSE_TYPE),
		mNumOccluders(0),
		mNumOccludersR2DB(0),
		mNumOccluderTris(0),
		mNumOccluderRasterizedTris(0),
		mRasterizeTime(0.0),
		mOccluderSizeThreshold(1.5f),
		mNumCulled(0),
		mNumVisible(0),
		mNumOccludeeTris(0),
		mNumOccludeeCulledTris(0),
		mNumOccludeeVisibleTris(0),
		mDepthTestTime(0.0),
		mOccludeeSizeThreshold(0.01f),
		mEnableCulling(true),
		mEnableFCulling(true),
		mViewDepthBuffer(false),
		mViewBoundingBox(false),
		mEnableTasks(true),
		mNumDrawCalls(0),
		mNumDepthTestTasks(20)
    {
		for(UINT i = 0; i < OCCLUDER_SETS; i++)
		{
			mpAssetSetDBR[i] = NULL;
		}

		for(UINT i = 0; i < OCCLUDEE_SETS; i++)
		{
			mpAssetSetAABB[i] = NULL;
		}

		mpAssetSetSky = NULL;

		if((mSOCType == SCALAR_TYPE) && !mEnableTasks)
		{
			mpDBRScalarST = new DepthBufferRasterizerScalarST;
			mpDBR = mpDBRScalarST;

			mpAABBScalarST = new AABBoxRasterizerScalarST;
			mpAABB = mpAABBScalarST;
		}
		else if((mSOCType == SCALAR_TYPE) && mEnableTasks)
		{
			mpDBRScalarMT = new DepthBufferRasterizerScalarMT;
			mpDBR = mpDBRScalarMT;

			mpAABBScalarMT = new AABBoxRasterizerScalarMT;
			mpAABB = mpAABBScalarMT;
		}
		else if((mSOCType == SSE_TYPE) && !mEnableTasks)
		{
			mpDBRSSEST = new DepthBufferRasterizerSSEST;
			mpDBR = mpDBRSSEST;

			mpAABBSSEST = new AABBoxRasterizerSSEST;
			mpAABB = mpAABBSSEST;
		}
		else if((mSOCType == SSE_TYPE) && mEnableTasks)
		{
			mpDBRSSEMT = new DepthBufferRasterizerSSEMT;
			mpDBR = mpDBRSSEMT;

			mpAABBSSEMT = new AABBoxRasterizerSSEMT;
			mpAABB = mpAABBSSEMT;
		}	
	}
    virtual ~MySample()
    {
        // Note: these two are defined in the base.  We release them because we addref them.
        SAFE_RELEASE(mpCamera);
        SAFE_RELEASE(mpShadowCamera);

		SAFE_RELEASE(mpCPURenderTarget);
		if(mViewDepthBuffer)
		{
			SAFE_RELEASE(mpBackBuffer);
			SAFE_RELEASE(mpRTView);
		}

		SAFE_DELETE(mpDBR);
		SAFE_DELETE(mpAABB);

		for(UINT i = 0; i < OCCLUDER_SETS; i++)
		{
			SAFE_RELEASE(mpAssetSetDBR[i]);
		}
		SAFE_RELEASE(mpAssetSetAABB[0]);
		SAFE_RELEASE(mpAssetSetSky);

        SAFE_DELETE( mpCameraController );
        SAFE_DELETE( mpDebugSprite);
        SAFE_RELEASE(mpShadowCameraSet);
        SAFE_DELETE( mpShadowRenderTarget );

        CPUTModel::ReleaseStaticResources();
    }

	UINT			*mpCPURenderTargetPixels;

    virtual CPUTEventHandledCode HandleKeyboardEvent(CPUTKey key);
    virtual CPUTEventHandledCode HandleMouseEvent(int x, int y, int wheel, CPUTMouseState state);
    virtual void                 HandleCallbackEvent( CPUTEventID Event, CPUTControlID ControlID, CPUTControl *pControl );

    virtual void Create();
    virtual void Render(double deltaSeconds);
    virtual void Update(double deltaSeconds);
    virtual void ResizeWindow(UINT width, UINT height);

	// define some controls
	static const CPUTControlID ID_MAIN_PANEL = 10;
	static const CPUTControlID ID_SECONDARY_PANEL = 20;
	static const CPUTControlID ID_FULLSCREEN_BUTTON = 100;
	static const CPUTControlID ID_NEXTMODEL_BUTTON = 200;
	static const CPUTControlID ID_TEST_CONTROL = 1000;
	static const CPUTControlID ID_IGNORE_CONTROL_ID = -1;

	static const CPUTControlID ID_RASTERIZE_TYPE = 1100;

	static const CPUTControlID ID_OCCLUDERS = 1200;
	static const CPUTControlID ID_NUM_OCCLUDERS = 1300;
	static const CPUTControlID ID_NUM_OCCLUDERS_RASTERIZED_TO_DB = 1400;
	static const CPUTControlID ID_NUM_OCCLUDER_TRIS = 1500;
	static const CPUTControlID ID_NUM_OCCLUDER_RASTERIZED_TRIS = 1600;
	static const CPUTControlID ID_RASTERIZE_TIME = 1700;
	static const CPUTControlID ID_OCCLUDER_SIZE = 1800;

	static const CPUTControlID ID_OCCLUDEES = 1900;
	static const CPUTControlID ID_NUM_OCCLUDEES = 2000;
	static const CPUTControlID ID_NUM_CULLED = 2100;
	static const CPUTControlID ID_NUM_VISIBLE = 2150;
	static const CPUTControlID ID_NUM_OCCLUDEE_TRIS = 2200;
	static const CPUTControlID ID_NUM_OCCLUDEE_CULLED_TRIS = 2300;
	static const CPUTControlID ID_NUM_OCCLUDEE_VISIBLE_TRIS = 2350;
	static const CPUTControlID ID_DEPTHTEST_TIME = 2400;
	static const CPUTControlID ID_OCCLUDEE_SIZE = 2500;

	static const CPUTControlID ID_ENABLE_CULLING = 2600;
	static const CPUTControlID ID_ENABLE_FCULLING = 2700;
	static const CPUTControlID ID_DEPTH_BUFFER_VISIBLE = 2800;
	static const CPUTControlID ID_BOUNDING_BOX_VISIBLE = 2900;
	static const CPUTControlID ID_ENABLE_TASKS = 3000;
	static const CPUTControlID ID_NUM_DRAW_CALLS = 3100;
	static const CPUTControlID ID_DEPTH_TEST_TASKS = 3200;
	static const CPUTControlID ID_VSYNC_ON_OFF = 3300;
};
#endif // __CPUT_SAMPLESTARTDX11_H__
