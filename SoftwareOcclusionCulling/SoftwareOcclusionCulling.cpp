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
#include "SoftwareOcclusionCulling.h"
#include "CPUTRenderTarget.h"
#include "CPUTTextureDX11.h"

#include <numeric>

#define BENCHMARK
#define BENCHMARK_VTUNE

const UINT SHADOW_WIDTH_HEIGHT = 256;

// set file to open
cString g_OpenFilePath;
cString g_OpenShaderPath;
cString g_OpenFileName;

extern float3 gLightDir;
extern char *gpDefaultShaderSource;

float gFarClipDistance = 2000.0f;

#ifdef BENCHMARK

#ifdef BENCHMARK_VTUNE
#include "ittnotify.h"
#ifndef _M_X64
#pragma comment(lib, "libittnotify.lib")
#else
#pragma comment(lib, "libittnotify64.lib")
#endif
#endif

static void dprintf(const char *fmt, ...)
{
	char buf[2048];

	va_list arg;
	va_start(arg, fmt);
	vsprintf_s(buf, fmt, arg);
	va_end(arg);

	OutputDebugStringA(buf);
}

// Run statistics
class RunStatistics
{
	std::vector<float> values;

public:
	void record(float val)
	{
		values.push_back(val);
	}

	void summarize()
	{
		size_t count = values.size();
		if (count < 2)
			return;

		// Print min/max and different percentiles
		std::sort(values.begin(), values.end());

		static const char *statname[5] = { "min", "25th", "med", "75th", "max" };
		for (int i=0; i < 5; i++)
			dprintf("  %s=%.3fms", statname[i], values[i * (count - 1) / 4]);

		// Mean and standard deviation
		double mean = std::accumulate(values.begin(), values.end(), 0.0) / count;
		auto accum_var = [mean](double sum, double x) -> double { double d = x - mean; return sum + d*d; };
		double varsum = std::accumulate(values.begin(), values.end(), 0.0, accum_var);
		double sdev = sqrt(varsum / (count - 1.0));

		dprintf("\n  mean=%.3fms sdev=%.3fms\n", mean, sdev);
	}
};

static RunStatistics g_renderTime, g_testTime;

#endif

// Handle OnCreation events
//-----------------------------------------------------------------------------
void MySample::Create()
{    
    CPUTAssetLibrary *pAssetLibrary = CPUTAssetLibrary::GetAssetLibrary();

    gLightDir.normalize();

    // TODO: read from cmd line, using these as defaults
    //pAssetLibrary->SetMediaDirectoryName(    _L("Media"));

    CPUTGuiControllerDX11 *pGUI = CPUTGetGuiController();

    // create some controls
	CPUTButton     *pButton = NULL;
    pGUI->CreateButton(_L("Fullscreen"), ID_FULLSCREEN_BUTTON, ID_MAIN_PANEL, &pButton);
	pGUI->CreateDropdown( L"Rasterizer Technique: SCALAR", ID_RASTERIZE_TYPE, ID_MAIN_PANEL, &mpTypeDropDown);
    mpTypeDropDown->AddSelectionItem( L"Rasterizer Technique: SSE" );
   	mpTypeDropDown->SetSelectedItem(mSOCType + 1);
   
	wchar_t string[CPUT_MAX_STRING_LENGTH];
    pGUI->CreateText(    _L("Occluders                                              \t"), ID_OCCLUDERS, ID_MAIN_PANEL, &mpOccludersText);
	
	swprintf_s(&string[0], CPUT_MAX_STRING_LENGTH, _L("\tNumber of Models: \t%d"), mNumOccluders);
	pGUI->CreateText(string, ID_NUM_OCCLUDERS, ID_MAIN_PANEL, &mpNumOccludersText);

	swprintf_s(&string[0], CPUT_MAX_STRING_LENGTH, _L("\tDepth rasterized models: %d"), mNumOccludersR2DB);
	pGUI->CreateText(string, ID_NUM_OCCLUDERS_RASTERIZED_TO_DB, ID_MAIN_PANEL, &mpOccludersR2DBText);

	swprintf_s(&string[0], CPUT_MAX_STRING_LENGTH, _L("\tNumber of tris: \t\t%f"), mNumOccluderTris);
	pGUI->CreateText(string, ID_NUM_OCCLUDER_TRIS, ID_MAIN_PANEL, &mpOccluderTrisText);

	swprintf_s(&string[0], CPUT_MAX_STRING_LENGTH, _L("\tDepth rasterized tris: \t%f"), mNumOccluderRasterizedTris);
	pGUI->CreateText(string, ID_NUM_OCCLUDER_RASTERIZED_TRIS, ID_MAIN_PANEL, &mpOccluderRasterizedTrisText);

	swprintf_s(&string[0], CPUT_MAX_STRING_LENGTH, _L("\tDepth raterizer time: \t%f"), mRasterizeTime);
	pGUI->CreateText(string, ID_RASTERIZE_TIME, ID_MAIN_PANEL, &mpRasterizeTimeText);

	swprintf_s(&string[0], CPUT_MAX_STRING_LENGTH, _L("Occluder Size Threshold: %0.4f"), mOccluderSizeThreshold);
	pGUI->CreateSlider(string, ID_OCCLUDER_SIZE, ID_MAIN_PANEL, &mpOccluderSizeSlider);
	mpOccluderSizeSlider->SetScale(0, 5.0, 51);
	mpOccluderSizeSlider->SetValue(mOccluderSizeThreshold);
	mpOccluderSizeSlider->SetTickDrawing(false);
    
	pGUI->CreateText(_L("Occludees                                              \t"), ID_OCCLUDEES, ID_MAIN_PANEL, &mpOccludeesText);

	swprintf_s(&string[0], CPUT_MAX_STRING_LENGTH, _L("\tNumber of Models: \t%d"), mNumOccludees);
	pGUI->CreateText(string, ID_NUM_OCCLUDEES, ID_MAIN_PANEL, &mpNumOccludeesText);

	swprintf_s(&string[0], CPUT_MAX_STRING_LENGTH, _L("\tModels culled: \t%d"), mNumCulled);
	pGUI->CreateText(string, ID_NUM_CULLED, ID_MAIN_PANEL, &mpCulledText);

	swprintf_s(&string[0], CPUT_MAX_STRING_LENGTH, _L("\tModels visible: \t%d"), mNumVisible);
	pGUI->CreateText(string, ID_NUM_VISIBLE, ID_MAIN_PANEL, &mpVisibleText);

	swprintf_s(&string[0], CPUT_MAX_STRING_LENGTH, _L("\tNumber of tris: \t%d"), (int)mNumOccludeeTris);
	pGUI->CreateText(string, ID_NUM_OCCLUDEE_TRIS, ID_MAIN_PANEL, &mpOccludeeTrisText);

	swprintf_s(&string[0], CPUT_MAX_STRING_LENGTH, _L("\tCulled Triangles: \t%d"), (int)mNumOccludeeCulledTris);
	pGUI->CreateText(string, ID_NUM_OCCLUDEE_CULLED_TRIS, ID_MAIN_PANEL, &mpCulledTrisText);

	swprintf_s(&string[0], CPUT_MAX_STRING_LENGTH, _L("\tVisible Triangles: \t%d"), (int)mNumOccludeeVisibleTris);
	pGUI->CreateText(string, ID_NUM_OCCLUDEE_VISIBLE_TRIS, ID_MAIN_PANEL, &mpVisibleTrisText);

	swprintf_s(&string[0], CPUT_MAX_STRING_LENGTH, _L("\tVisible Triangles: \t%0.2f ms"), mDepthTestTime);
	pGUI->CreateText(string, ID_DEPTHTEST_TIME, ID_MAIN_PANEL, &mpDepthTestTimeText);

	swprintf_s(&string[0], CPUT_MAX_STRING_LENGTH, _L("Occludee Size Threshold: %0.4f"), mOccludeeSizeThreshold);
	pGUI->CreateSlider(string, ID_OCCLUDEE_SIZE, ID_MAIN_PANEL, &mpOccludeeSizeSlider);
	mpOccludeeSizeSlider->SetScale(0, 0.1f, 41);
	mpOccludeeSizeSlider->SetValue(mOccludeeSizeThreshold);
	mpOccludeeSizeSlider->SetTickDrawing(false);

	pGUI->CreateCheckbox(_L("Depth Test Culling"),  ID_ENABLE_CULLING, ID_MAIN_PANEL, &mpCullingCheckBox);
	pGUI->CreateCheckbox(_L("Frustum Culling"),  ID_ENABLE_FCULLING, ID_MAIN_PANEL, &mpFCullingCheckBox);
	pGUI->CreateCheckbox(_L("View Depth Buffer"),  ID_DEPTH_BUFFER_VISIBLE, ID_MAIN_PANEL, &mpDBCheckBox);
	pGUI->CreateCheckbox(_L("View Bounding Box"),  ID_BOUNDING_BOX_VISIBLE, ID_MAIN_PANEL, &mpBBCheckBox);
	pGUI->CreateCheckbox(_L("Multi Tasking"), ID_ENABLE_TASKS, ID_MAIN_PANEL, &mpTasksCheckBox);
	pGUI->CreateCheckbox(_L("Vsync"), ID_VSYNC_ON_OFF, ID_MAIN_PANEL, &mpVsyncCheckBox);

	swprintf_s(&string[0], CPUT_MAX_STRING_LENGTH, _L("Number of draw calls: \t%d"), mNumDrawCalls);
	pGUI->CreateText(string, ID_NUM_DRAW_CALLS, ID_MAIN_PANEL, &mpDrawCallsText),
	
	swprintf_s(&string[0], CPUT_MAX_STRING_LENGTH, _L("Depth Test Tasks: \t\t%d"), mNumDepthTestTasks);
    pGUI->CreateSlider(string, ID_DEPTH_TEST_TASKS, ID_MAIN_PANEL, &mpDepthTestTaskSlider);
	mpDepthTestTaskSlider->SetScale(1, 50, 11);
	mpDepthTestTaskSlider->SetValue((float)mNumDepthTestTasks);
	mpDepthTestTaskSlider->SetTickDrawing(false);
	mpAABB->SetDepthTestTasks(mNumDepthTestTasks);

    //
    // Create Static text
    //
    pGUI->CreateText( _L("F1 for Help"), ID_IGNORE_CONTROL_ID, ID_SECONDARY_PANEL);
    pGUI->CreateText( _L("[Escape] to quit application"), ID_IGNORE_CONTROL_ID, ID_SECONDARY_PANEL);
    pGUI->CreateText( _L("A,S,D,F - move camera position"), ID_IGNORE_CONTROL_ID, ID_SECONDARY_PANEL);
    pGUI->CreateText( _L("Q - camera position up"), ID_IGNORE_CONTROL_ID, ID_SECONDARY_PANEL);
    pGUI->CreateText( _L("E - camera position down"), ID_IGNORE_CONTROL_ID, ID_SECONDARY_PANEL);
    pGUI->CreateText( _L("mouse + right click - camera look location"), ID_IGNORE_CONTROL_ID, ID_SECONDARY_PANEL);
	pGUI->CreateText( _L("size thresholds : computed using screen space metris"), ID_IGNORE_CONTROL_ID, ID_SECONDARY_PANEL);

    pGUI->SetActivePanel(ID_MAIN_PANEL);
    pGUI->DrawFPS(true);

    // Add our programatic (and global) material parameters
    CPUTMaterial::mGlobalProperties.AddValue( _L("cbPerFrameValues"), _L("$cbPerFrameValues") );
    CPUTMaterial::mGlobalProperties.AddValue( _L("cbPerModelValues"), _L("#cbPerModelValues") );
    CPUTMaterial::mGlobalProperties.AddValue( _L("_Shadow"), _L("$shadow_depth") );

	// Creating a render target to view the CPU rasterized depth buffer
	mpCPUDepthBuf = new unsigned char[SCREENW*SCREENH*4];

	CD3D11_TEXTURE2D_DESC cpuRenderTargetDesc(
            DXGI_FORMAT_R8G8B8A8_UNORM,
            SCREENW * 2, // TODO: round up to full tile sizes
            SCREENH / 2,
            1, // Array Size
            1, // MIP Levels
			D3D11_BIND_SHADER_RESOURCE,
            D3D11_USAGE_DEFAULT,
			0
        );
	HRESULT hr = mpD3dDevice->CreateTexture2D(&cpuRenderTargetDesc, NULL, &mpCPURenderTarget);
	ASSERT(SUCCEEDED(hr), _L("Failed creating render target."));

	hr = mpD3dDevice->CreateShaderResourceView(mpCPURenderTarget, NULL, &mpCPUSRV);
	ASSERT(SUCCEEDED(hr), _L("Failed creating shader resource view."));

	// Corresponding texture object
	CPUTTextureDX11 *pDummyTex = new CPUTTextureDX11;
	pDummyTex->SetTextureAndShaderResourceView(mpCPURenderTarget, mpCPUSRV);
	pAssetLibrary->AddTexture( _L("$depthbuf_tex"), pDummyTex );
	SAFE_RELEASE(pDummyTex);

    // Create default shaders
    CPUTPixelShaderDX11  *pPS       = CPUTPixelShaderDX11::CreatePixelShaderFromMemory(            _L("$DefaultShader"), CPUT_DX11::mpD3dDevice,          _L("PSMain"), _L("ps_4_0"), gpDefaultShaderSource );
    CPUTPixelShaderDX11  *pPSNoTex  = CPUTPixelShaderDX11::CreatePixelShaderFromMemory(   _L("$DefaultShaderNoTexture"), CPUT_DX11::mpD3dDevice, _L("PSMainNoTexture"), _L("ps_4_0"), gpDefaultShaderSource );
    CPUTVertexShaderDX11 *pVS       = CPUTVertexShaderDX11::CreateVertexShaderFromMemory(          _L("$DefaultShader"), CPUT_DX11::mpD3dDevice,          _L("VSMain"), _L("vs_4_0"), gpDefaultShaderSource );
    CPUTVertexShaderDX11 *pVSNoTex  = CPUTVertexShaderDX11::CreateVertexShaderFromMemory( _L("$DefaultShaderNoTexture"), CPUT_DX11::mpD3dDevice, _L("VSMainNoTexture"), _L("vs_4_0"), gpDefaultShaderSource );

    // We just want to create them, which adds them to the library.  We don't need them any more so release them, leaving refCount at 1 (only library owns a ref)
    SAFE_RELEASE(pPS);
    SAFE_RELEASE(pPSNoTex);
    SAFE_RELEASE(pVS);
    SAFE_RELEASE(pVSNoTex);

    // load shadow casting material+sprite object
    cString ExecutableDirectory;
    CPUTOSServices::GetOSServices()->GetExecutableDirectory(&ExecutableDirectory);
    pAssetLibrary->SetMediaDirectoryName(  ExecutableDirectory+_L("Media\\"));

    mpShadowRenderTarget = new CPUTRenderTargetDepth();
    mpShadowRenderTarget->CreateRenderTarget( cString(_L("$shadow_depth")), SHADOW_WIDTH_HEIGHT, SHADOW_WIDTH_HEIGHT, DXGI_FORMAT_D32_FLOAT );

    mpDebugSprite = new CPUTSprite();
    mpDebugSprite->CreateSprite( -1.0f, -1.0f, 0.5f, 0.5f, _L("Sprite") );

	int width, height;
    CPUTOSServices::GetOSServices()->GetClientDimensions(&width, &height);

	// Depth buffer visualization material
	mpShowDepthBufMtrl = (CPUTMaterialDX11*)CPUTAssetLibraryDX11::GetAssetLibrary()->GetMaterial( _L("showDepthBuf") );

    // Call ResizeWindow() because it creates some resources that our blur material needs (e.g., the back buffer)
    ResizeWindow(width, height);

    CPUTRenderStateBlockDX11 *pBlock = new CPUTRenderStateBlockDX11();
    CPUTRenderStateDX11 *pStates = pBlock->GetState();

    // Override default sampler desc for our default shadowing sampler
    pStates->SamplerDesc[1].Filter         = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
    pStates->SamplerDesc[1].AddressU       = D3D11_TEXTURE_ADDRESS_BORDER;
    pStates->SamplerDesc[1].AddressV       = D3D11_TEXTURE_ADDRESS_BORDER;
    pStates->SamplerDesc[1].ComparisonFunc = D3D11_COMPARISON_GREATER;
    pBlock->CreateNativeResources();
    CPUTAssetLibrary::GetAssetLibrary()->AddRenderStateBlock( _L("$DefaultRenderStates"), pBlock );
    pBlock->Release(); // We're done with it.  The library owns it now.

    //
    // Load .set files to load the castle scene
	//
    pAssetLibrary->SetMediaDirectoryName(_L("Media\\Castle\\"));

#ifdef DEBUG
    mpAssetSetDBR[0] = pAssetLibrary->GetAssetSet(_L("castleDebug"));
	ASSERT(mpAssetSetDBR[0], _L("Failed loading castle."));

	mpAssetSetDBR[1] = pAssetLibrary->GetAssetSet(_L("groundDebug"));
	ASSERT(mpAssetSetDBR[1], _L("Failed loading ground."));

	mpAssetSetDBR[2] = pAssetLibrary->GetAssetSet(_L("stoneWallsDebug"));
	ASSERT(mpAssetSetDBR[2], _L("Failed loading stoneWalls."));

	mpAssetSetAABB[0] = pAssetLibrary->GetAssetSet(_L("marketStallsDebug"));
	ASSERT(mpAssetSetAABB, _L("Failed loading marketStalls"));
#else
    mpAssetSetDBR[0] = pAssetLibrary->GetAssetSet(_L("castle"));
	ASSERT(mpAssetSetDBR[0], _L("Failed loading castle."));

	mpAssetSetDBR[1] = pAssetLibrary->GetAssetSet(_L("ground"));
	ASSERT(mpAssetSetDBR[1], _L("Failed loading ground."));

	mpAssetSetDBR[2] = pAssetLibrary->GetAssetSet(_L("stoneWalls"));
	ASSERT(mpAssetSetDBR[2], _L("Failed loading stoneWalls."));

	mpAssetSetAABB[0] = pAssetLibrary->GetAssetSet(_L("marketStalls"));
	ASSERT(mpAssetSetAABB, _L("Failed loading marketStalls"));
#endif

	mpAssetSetSky = pAssetLibrary->GetAssetSet(_L("sky"));
	ASSERT(mpAssetSetSky, _L("Failed loading sky"));

	// For every occluder model in the sene create a place holder 
	// for the CPU transformed vertices of the model.   
	mpDBR->CreateTransformedModels(mpAssetSetDBR, OCCLUDER_SETS);
	// Get number of occluders in the scene
	mNumOccluders = mpDBR->GetNumOccluders();
	// Get number of occluder triangles in the scene 
	mNumOccluderTris = mpDBR->GetNumTriangles();


	// For every occludee model in the scene create a place holder
	// for the triangles that make up the model axis aligned bounding box
	mpAssetSetAABB[1] = mpAssetSetDBR[0];
	mpAssetSetAABB[2] = mpAssetSetDBR[1];
	mpAssetSetAABB[3] = mpAssetSetDBR[2];

	mpAABB->CreateTransformedAABBoxes(mpAssetSetAABB, OCCLUDEE_SETS);
	// Get number of occludees in the scene
	mNumOccludees = mpAABB->GetNumOccludees();
	// Get number of occluddee triangles in the scene
	mNumOccludeeTris = mpAABB->GetNumTriangles();
	
	swprintf_s(&string[0], CPUT_MAX_STRING_LENGTH, _L("\tNumber of Models: \t%d"), mNumOccluders);
	mpNumOccludersText->SetText(string);

	swprintf_s(&string[0], CPUT_MAX_STRING_LENGTH, _L("\tNumber of tris: \t\t%d"), mNumOccluderTris);
	mpOccluderTrisText->SetText(string);

	swprintf_s(&string[0], CPUT_MAX_STRING_LENGTH, _L("\tNumber of Model: \t%d"), mNumOccludees);
	mpNumOccludeesText->SetText(string);

	swprintf_s(&string[0], CPUT_MAX_STRING_LENGTH, _L("\tNumber of tris: \t\t%d"), mNumOccludeeTris);
	mpOccludeeTrisText->SetText(string);

	CPUTCheckboxState state;
	if(mEnableCulling)
	{
		state = CPUT_CHECKBOX_CHECKED;
	}
	else 
	{
		state = CPUT_CHECKBOX_UNCHECKED;
	}
	mpCullingCheckBox->SetCheckboxState(state);

	if(mEnableFCulling)
	{
		state = CPUT_CHECKBOX_CHECKED;
	}
	else 
	{
		state = CPUT_CHECKBOX_UNCHECKED;
	}
	mpFCullingCheckBox->SetCheckboxState(state);

	if(mViewDepthBuffer)
	{
		state = CPUT_CHECKBOX_CHECKED;
	}
	else 
	{
		state = CPUT_CHECKBOX_UNCHECKED;
	}
	mpDBCheckBox->SetCheckboxState(state);

	if(mEnableTasks)
	{
		state = CPUT_CHECKBOX_CHECKED;
	}
	else 
	{
		state = CPUT_CHECKBOX_UNCHECKED;
	}
	mpTasksCheckBox->SetCheckboxState(state);

	if(mSyncInterval)
	{
		state = CPUT_CHECKBOX_CHECKED;
	}
	else
	{
		state = CPUT_CHECKBOX_UNCHECKED;
	}
	mpVsyncCheckBox->SetCheckboxState(state);

	// Setting occluder size threshold in DepthBufferRasterizer
	mpDBR->SetOccluderSizeThreshold(mOccluderSizeThreshold);
	// Setting occludee size threshold in AABBoxRasterizer
	mpAABB->SetOccludeeSizeThreshold(mOccludeeSizeThreshold);
	
    //
	// If no cameras were created from the model sets then create a default simple camera
	// and add it to the camera array.
	//
    if( mpAssetSetDBR[0] && mpAssetSetDBR[0]->GetCameraCount() )
    {
        mpCamera = mpAssetSetDBR[0]->GetFirstCamera();
        mpCamera->AddRef(); 
    } else
    {
        mpCamera = new CPUTCamera();
        CPUTAssetLibraryDX11::GetAssetLibrary()->AddCamera( _L("SampleStart Camera"), mpCamera );

        mpCamera->SetPosition( 0.0f, 0.0f, 5.0f );
        // Set the projection matrix for all of the cameras to match our window.
        // TODO: this should really be a viewport matrix.  Otherwise, all cameras will have the same FOV and aspect ratio, etc instead of just viewport dimensions.
        mpCamera->SetAspectRatio(((float)width)/((float)height));
    }
    mpCamera->SetFov(XMConvertToRadians(60.0f)); // TODO: Fix converter's FOV bug (Maya generates cameras for which fbx reports garbage for fov)
    mpCamera->SetFarPlaneDistance(gFarClipDistance);
    mpCamera->Update();

    // Set up the shadow camera (a camera that sees what the light sees)
    float3 lookAtPoint(0.0f, 0.0f, 0.0f);
    float3 half(1.0f, 1.0f, 1.0f);
    if( mpAssetSetDBR[0] )
    {
        mpAssetSetDBR[0]->GetBoundingBox( &lookAtPoint, &half );
    }
    float length = half.length();

    mpShadowCamera = new CPUTCamera();
    mpShadowCamera->SetFov(XMConvertToRadians(45));
    mpShadowCamera->SetAspectRatio(1.0f);
    float fov = mpShadowCamera->GetFov();
    float tanHalfFov = tanf(fov * 0.5f);
    float cameraDistance = length/tanHalfFov;
    float nearDistance = cameraDistance * 0.1f;
    mpShadowCamera->SetNearPlaneDistance(nearDistance);
    mpShadowCamera->SetFarPlaneDistance(2.0f * cameraDistance);
    CPUTAssetLibraryDX11::GetAssetLibrary()->AddCamera( _L("ShadowCamera"), mpShadowCamera );
    float3 shadowCameraPosition = lookAtPoint - gLightDir * cameraDistance;
    mpShadowCamera->SetPosition( shadowCameraPosition );
    mpShadowCamera->LookAt( lookAtPoint.x, lookAtPoint.y, lookAtPoint.z );
    mpShadowCamera->Update();

    mpCameraController = new CPUTCameraControllerFPS();
    mpCameraController->SetCamera(mpCamera);
    mpCameraController->SetLookSpeed(0.004f);
    mpCameraController->SetMoveSpeed(2.5f);
}

//-----------------------------------------------------------------------------
void MySample::Update(double deltaSeconds)
{
    mpCameraController->Update((float)deltaSeconds);
}

// Handle keyboard events
//-----------------------------------------------------------------------------
CPUTEventHandledCode MySample::HandleKeyboardEvent(CPUTKey key)
{
    static bool panelToggle = false;
    CPUTEventHandledCode    handled = CPUT_EVENT_UNHANDLED;
    cString fileName;
    CPUTGuiControllerDX11*  pGUI = CPUTGetGuiController();

    switch(key)
    {
    case KEY_F1:
        panelToggle = !panelToggle;
        if(panelToggle)
        {
            pGUI->SetActivePanel(ID_SECONDARY_PANEL);
        }
        else
        {
            pGUI->SetActivePanel(ID_MAIN_PANEL);
        }
        handled = CPUT_EVENT_HANDLED;
        break;
    case KEY_L:
        {
            static int cameraObjectIndex = 0;
            CPUTRenderNode *pCameraList[] = { mpCamera, mpShadowCamera };
            cameraObjectIndex = (++cameraObjectIndex) % (sizeof(pCameraList)/sizeof(*pCameraList));
            CPUTRenderNode *pCamera = pCameraList[cameraObjectIndex];
            mpCameraController->SetCamera( pCamera );
        }
        handled = CPUT_EVENT_HANDLED;
        break;
    case KEY_ESCAPE:
        handled = CPUT_EVENT_HANDLED;
        Shutdown();
        break;
    }

    // pass it to the camera controller
    if(handled == CPUT_EVENT_UNHANDLED)
    {
        handled = mpCameraController->HandleKeyboardEvent(key);
    }
    return handled;
}


// Handle mouse events
//-----------------------------------------------------------------------------
CPUTEventHandledCode MySample::HandleMouseEvent(int x, int y, int wheel, CPUTMouseState state)
{
    if( mpCameraController )
    {
        return mpCameraController->HandleMouseEvent(x, y, wheel, state);
    }
    return CPUT_EVENT_UNHANDLED;
}

// Handle any control callback events
//-----------------------------------------------------------------------------
void MySample::HandleCallbackEvent( CPUTEventID Event, CPUTControlID ControlID, CPUTControl *pControl )
{
    UNREFERENCED_PARAMETER(Event);
    UNREFERENCED_PARAMETER(pControl);
    cString SelectedItem;
    
    switch(ControlID)
    {
	case ID_FULLSCREEN_BUTTON:
	{
        CPUTToggleFullScreenMode();
        break;
	}
	case ID_RASTERIZE_TYPE:
	{
		SAFE_DELETE_ARRAY(mpDBR);
		SAFE_DELETE_ARRAY(mpAABB);
		
		UINT selectedItem;
        mpTypeDropDown->GetSelectedItem(selectedItem);

		if(selectedItem - 1 == 0)
		{
			mSOCType = SCALAR_TYPE;
			if(!mEnableTasks)
			{
				mpDBRScalarST = new DepthBufferRasterizerScalarST;
				mpDBR = mpDBRScalarST;

				mpAABBScalarST = new AABBoxRasterizerScalarST;
				mpAABB = mpAABBScalarST;
			}
			else
			{
				mpDBRScalarMT = new DepthBufferRasterizerScalarMT;
				mpDBR = mpDBRScalarMT;

				mpAABBScalarMT = new AABBoxRasterizerScalarMT;
				mpAABB = mpAABBScalarMT;
			}
		}
		else if(selectedItem - 2 == 0)
		{
			mSOCType = SSE_TYPE;
			if(!mEnableTasks)
			{
				mpDBRSSEST = new DepthBufferRasterizerSSEST;
				mpDBR = mpDBRSSEST;

				mpAABBSSEST = new AABBoxRasterizerSSEST;
				mpAABB = mpAABBSSEST;
			}
			else
			{
				mpDBRSSEMT = new DepthBufferRasterizerSSEMT;
				mpDBR = mpDBRSSEMT;

				mpAABBSSEMT = new AABBoxRasterizerSSEMT;
				mpAABB = mpAABBSSEMT;
			}

		}
		mpDBR->CreateTransformedModels(mpAssetSetDBR, OCCLUDER_SETS);		
		mpDBR->SetOccluderSizeThreshold(mOccluderSizeThreshold);
		mpAABB->CreateTransformedAABBoxes(mpAssetSetAABB, OCCLUDEE_SETS);
		mpAABB->SetDepthTestTasks(mNumDepthTestTasks);
		mpAABB->SetOccludeeSizeThreshold(mOccludeeSizeThreshold);

		break;
	}
	case ID_DEPTH_BUFFER_VISIBLE:
	{
		CPUTCheckboxState state = mpDBCheckBox->GetCheckboxState();
		if(state == CPUT_CHECKBOX_CHECKED)
		{
			mViewDepthBuffer = true;
			HRESULT hr = mpSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&mpBackBuffer);
			ASSERT(SUCCEEDED(hr), _L("Failed gettting back buffer"));
	
			hr = mpD3dDevice->CreateRenderTargetView(mpBackBuffer, NULL, &mpRTView);
			ASSERT( SUCCEEDED(hr), _L("Failed creating render target view."));
		}
		else 
		{
			mpBackBuffer->Release();
			mpRTView->Release();
			mViewDepthBuffer = false;
		}
		break;
	}
	case ID_BOUNDING_BOX_VISIBLE:
	{
		CPUTCheckboxState state = mpBBCheckBox->GetCheckboxState();
		if(state == CPUT_CHECKBOX_CHECKED)
		{
			mViewBoundingBox = true;
		}
		else 
		{
			mViewBoundingBox = false;
		}
		break;
	}
	case ID_ENABLE_TASKS:
	{
		SAFE_DELETE_ARRAY(mpDBR);
		SAFE_DELETE_ARRAY(mpAABB);

		CPUTCheckboxState state = mpTasksCheckBox->GetCheckboxState();
		if(state == CPUT_CHECKBOX_CHECKED)
		{
			mEnableTasks = true;
			mpDepthTestTaskSlider->SetVisibility(true);

			wchar_t string[CPUT_MAX_STRING_LENGTH];
			swprintf_s(&string[0], CPUT_MAX_STRING_LENGTH, _L("Depth Test Task: \t%d"), mNumDepthTestTasks);
			mpDepthTestTaskSlider->SetText(string);
			
			if(mSOCType == SCALAR_TYPE)
			{
				mpDBRScalarMT = new DepthBufferRasterizerScalarMT;
				mpDBR = mpDBRScalarMT;

				mpAABBScalarMT = new AABBoxRasterizerScalarMT;
				mpAABB = mpAABBScalarMT;
			}
			else if(mSOCType == SSE_TYPE)
			{
				mpDBRSSEMT = new DepthBufferRasterizerSSEMT;
				mpDBR = mpDBRSSEMT;

				mpAABBSSEMT = new AABBoxRasterizerSSEMT;
				mpAABB = mpAABBSSEMT;
			}
			mpAABB->SetDepthTestTasks(mNumDepthTestTasks);
		}
		else
		{
			mEnableTasks = false;
			mpDepthTestTaskSlider->SetVisibility(false);
			if(mSOCType == SCALAR_TYPE)
			{
				mpDBRScalarST = new DepthBufferRasterizerScalarST;
				mpDBR = mpDBRScalarST;

				mpAABBScalarST = new AABBoxRasterizerScalarST;
				mpAABB = mpAABBScalarST;
			}
			else if(mSOCType == SSE_TYPE)
			{
				mpDBRSSEST = new DepthBufferRasterizerSSEST;
				mpDBR = mpDBRSSEST;

				mpAABBSSEST = new AABBoxRasterizerSSEST;
				mpAABB = mpAABBSSEST;
			}
		}
		mpDBR->CreateTransformedModels(mpAssetSetDBR, OCCLUDER_SETS);		
		mpDBR->SetOccluderSizeThreshold(mOccluderSizeThreshold);
		mpAABB->CreateTransformedAABBoxes(mpAssetSetAABB, OCCLUDEE_SETS);
		mpAABB->SetOccludeeSizeThreshold(mOccludeeSizeThreshold);
		break;
	}
	case ID_OCCLUDER_SIZE:
	{
		float occluderSize;
		mpOccluderSizeSlider->GetValue(occluderSize);
		mOccluderSizeThreshold = occluderSize;

		wchar_t string[CPUT_MAX_STRING_LENGTH];
		swprintf_s(&string[0], CPUT_MAX_STRING_LENGTH, _L("Occluder Size Threshold: %0.4f"), mOccluderSizeThreshold);
		mpOccluderSizeSlider->SetText(string);
		mpDBR->SetOccluderSizeThreshold(mOccluderSizeThreshold);
		break;
	}
	case ID_OCCLUDEE_SIZE:
	{
		float occludeeSize;
		mpOccludeeSizeSlider->GetValue(occludeeSize);
		mOccludeeSizeThreshold = occludeeSize;

		wchar_t string[CPUT_MAX_STRING_LENGTH];
		swprintf_s(&string[0], CPUT_MAX_STRING_LENGTH, _L("Occludee Size Threshold: %0.4f"), mOccludeeSizeThreshold);
		mpOccludeeSizeSlider->SetText(string);
		mpAABB->SetOccludeeSizeThreshold(mOccludeeSizeThreshold);
		break;
	}
	case ID_DEPTH_TEST_TASKS:
	{
		float numDepthTestTasks;
		mpDepthTestTaskSlider->GetValue(numDepthTestTasks);
		mNumDepthTestTasks = (UINT)numDepthTestTasks;

		wchar_t string[CPUT_MAX_STRING_LENGTH];
		swprintf_s(&string[0], CPUT_MAX_STRING_LENGTH, _L("Depth Test Task: \t\t%d"), mNumDepthTestTasks);
		mpDepthTestTaskSlider->SetText(string);
		mpAABB->SetDepthTestTasks(mNumDepthTestTasks);
		break;
	}

	case ID_ENABLE_CULLING:
	{
		CPUTCheckboxState state = mpCullingCheckBox->GetCheckboxState();
		if(state)
		{
			mEnableCulling = true;
		}
		else
		{
			mEnableCulling = false;
			//TODO: clear to zero in threads instead of here 
			memset(mpCPUDepthBuf, 0, SCREENW * SCREENH * 4);

			mpOccludersR2DBText->SetText(         _L("\tDepth rasterized models: 0"));
			mpOccluderRasterizedTrisText->SetText(_L("\tDepth rasterized tris: \t0"));
			mpRasterizeTimeText->SetText(         _L("\tDepth rasterizer time: \t0 ms"));

			mpCulledText->SetText(       _L("\tModels culled: \t\t0"));
			mpVisibleText->SetText(      _L("\tModels visible: \t\t0"));
			mpCulledTrisText->SetText(   _L("\tCulled tris: \t\t0"));
			mpVisibleTrisText->SetText(   _L("\tVisible tris: \t\t0"));
			mpDepthTestTimeText->SetText(_L("\tDepth test time: \t0 ms"));
		}
		break;
	}
	case ID_ENABLE_FCULLING:
	{
		CPUTCheckboxState state = mpFCullingCheckBox->GetCheckboxState();
		if(state)
		{
			mEnableFCulling = true;
		}
		else
		{
			mEnableFCulling = false;
			mpDBR->ResetInsideFrustum();
			mpAABB->ResetInsideFrustum();
		}	
		break;
	}
	case ID_VSYNC_ON_OFF:
	{
		CPUTCheckboxState state = mpVsyncCheckBox->GetCheckboxState();
		if(state)
		{
			mSyncInterval = 1;
		}
		else
		{
			mSyncInterval = 0;
		}
		break;
	}
    default:
        break;
    }
}

// Handle resize events
//-----------------------------------------------------------------------------
void MySample::ResizeWindow(UINT width, UINT height)
{
    CPUTAssetLibrary *pAssetLibrary = CPUTAssetLibrary::GetAssetLibrary();

    // Before we can resize the swap chain, we must release any references to it.
    // We could have a "AssetLibrary::ReleaseSwapChainResources(), or similar.  But,
    // Generic "release all" works, is simpler to implement/maintain, and is not performance critical.
    pAssetLibrary->ReleaseTexturesAndBuffers();

    // Resize the CPUT-provided render target
    CPUT_DX11::ResizeWindow( width, height );

    // Resize any application-specific render targets here
    if( mpCamera ) mpCamera->SetAspectRatio(((float)width)/((float)height));

    pAssetLibrary->RebindTexturesAndBuffers();
}

#include "xnamath.h"
static ID3D11UnorderedAccessView *gpNullUAVs[CPUT_MATERIAL_MAX_TEXTURE_SLOTS] = {0};
//-----------------------------------------------------------------------------
void MySample::Render(double deltaSeconds)
{
    CPUTRenderParametersDX renderParams(mpContext);

	// If mViewBoundingBox is enabled then draw the axis aligned bounding box 
	// for all the model in the scene. FYI This will affect frame rate.
	if(mViewBoundingBox)
	{
		renderParams.mShowBoundingBoxes = true;
	}
	else
	{
		renderParams.mShowBoundingBoxes = false;
	}	

    gLightDir = mpShadowCamera->GetLook();

    // Clear back buffer
    const float clearColor[] = { 0.0993f, 0.0993f, 0.0993f, 1.0f };
    mpContext->ClearRenderTargetView( mpBackBufferRTV,  clearColor );
    mpContext->ClearDepthStencilView( mpDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 0.0f, 0);

	mpCamera->SetNearPlaneDistance(1.0f);
	mpCamera->SetFarPlaneDistance(gFarClipDistance);
	mpCamera->Update();

	// Set the camera transforms so that the occluders can be transformed
	mpDBR->SetViewProj(mpCamera->GetViewMatrix(), (float4x4*)mpCamera->GetProjectionMatrix());

	// If view frustum culling is enabled then determine which occluders and occludees are 
	// inside the view frustum and run the software occlusion culling on only the those models
	if(mEnableFCulling)
	{
		renderParams.mpCamera = mpCamera;
		mpDBR->IsVisible(mpCamera);
		mpAABB->IsInsideViewFrustum(mpCamera);
	}

	// if software occlusion culling is enabled
	if(mEnableCulling)
	{
		// Clear the depth buffer
		mpCPURenderTargetPixels = (UINT*)mpCPUDepthBuf;
		memset(mpCPURenderTargetPixels, 0, SCREENW * SCREENH * 4);
		mpDBR->SetCPURenderTargetPixels(mpCPURenderTargetPixels);
		// Transform the occluder models and rasterize them to the depth buffer
		mpDBR->TransformModelsAndRasterizeToDepthBuffer();
	
		// Set the camera transforms so that the occludee abix aligned bounding boxes (AABB) can be transformed
		mpAABB->SetViewProjMatrix(mpCamera->GetViewMatrix(), (float4x4*)mpCamera->GetProjectionMatrix());
		mpAABB->SetCPURenderTargetPixels(mpCPURenderTargetPixels);
		// Transform the occludee AABB, rasterize and depth test to determine is occludee is visible or occluded 
		mpAABB->TransformAABBoxAndDepthTest();
	}
	else
	{
		// Set the camera transforms so that the occludee abix aligned bounding boxes (AABB) can be transformed
		mpAABB->SetViewProjMatrix(mpCamera->GetViewMatrix(), (float4x4*)mpCamera->GetProjectionMatrix());
	}

	// If mViewDepthBuffer is enabled then blit the CPU rasterized depth buffer to the frame buffer
	if(mViewDepthBuffer)
	{
		// Update the GPU-side depth buffer
		mpContext->UpdateSubresource(mpCPURenderTarget, 0, NULL, mpCPUDepthBuf, 2 * SCREENW * 4, 0);
		mpShowDepthBufMtrl->SetRenderStates(renderParams);
		mpContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		mpContext->Draw(3, 0);
	}
	// else render the (frustum culled) occluders and only the visible occludees
	else
	{
		CPUTMeshDX11::ResetDrawCallCount();

		if(mpAssetSetSky) { mpAssetSetSky->RenderRecursive(renderParams); }
		
		if(mpAssetSetAABB) 
		{
			// if occlusion culling is enabled then render only the visible occludees in the scene
			if(mEnableCulling)
			{
				mpAABB->RenderVisible(mpAssetSetAABB, renderParams, OCCLUDEE_SETS); 
			}
			// else render all the (25,000) occludee models in the scene
			else
			{
				mpAABB->Render(mpAssetSetAABB, renderParams, OCCLUDEE_SETS);
			}
		}
		mNumDrawCalls = CPUTMeshDX11::GetDrawCallCount();
	}

	wchar_t string[CPUT_MAX_STRING_LENGTH];
	if(mEnableCulling)
	{
		mNumOccludersR2DB = mpDBR->GetNumOccludersR2DB();
		mNumOccluderRasterizedTris = mpDBR->GetNumRasterizedTriangles();
		mRasterizeTime = mpDBR->GetRasterizeTime();
		
		mNumCulled = mpAABB->GetNumCulled();
		mNumVisible = mNumOccludees - mNumCulled;
		mNumOccludeeCulledTris = mpAABB->GetNumCulledTriangles();
		mNumOccludeeVisibleTris = mNumOccludeeTris - mNumOccludeeCulledTris;
		mDepthTestTime = mpAABB->GetDepthTestTime();
		
		swprintf_s(&string[0], CPUT_MAX_STRING_LENGTH, _L("\tDepth rasterized models: %d"), mNumOccludersR2DB);
		mpOccludersR2DBText->SetText(string);

		swprintf_s(&string[0], CPUT_MAX_STRING_LENGTH, _L("\tDepth rasterized tris: \t%d"), (int)mNumOccluderRasterizedTris);
		mpOccluderRasterizedTrisText->SetText(string);

		swprintf_s(&string[0], CPUT_MAX_STRING_LENGTH, _L("\tDepth rasterizer time: \t%0.2f ms"), mRasterizeTime * 1000.0f);
		mpRasterizeTimeText->SetText(string);


		swprintf_s(&string[0], CPUT_MAX_STRING_LENGTH, _L("\tModels culled: \t\t%d"), mNumCulled);
		mpCulledText->SetText(string);

		swprintf_s(&string[0], CPUT_MAX_STRING_LENGTH, _L("\tModels visible: \t\t%d"), mNumVisible);
		mpVisibleText->SetText(string);

		swprintf_s(&string[0], CPUT_MAX_STRING_LENGTH, _L("\tCulled tris: \t\t%d"), (int)mNumOccludeeCulledTris);
		mpCulledTrisText->SetText(string);

		swprintf_s(&string[0], CPUT_MAX_STRING_LENGTH, _L("\tVisible tris: \t\t%d"), (int)mNumOccludeeVisibleTris);
		mpVisibleTrisText->SetText(string);

		swprintf_s(&string[0], CPUT_MAX_STRING_LENGTH, _L("\tDepth test time: \t%0.2f ms"), mDepthTestTime * 1000.0f);
		mpDepthTestTimeText->SetText(string);		
	}

	swprintf_s(&string[0], CPUT_MAX_STRING_LENGTH, _L("Number of draw calls: \t\t %d"), mNumDrawCalls);
	mpDrawCallsText->SetText(string);

#ifdef BENCHMARK
	// Accumulate statistics over a number of frames
	static const int initialDelay = 60; // give it a few frames to settle
	static const int sampleLen = 600;

	static int frameCount = 0;
	int frame = frameCount++;

#ifdef BENCHMARK_VTUNE
	if (frame == initialDelay)
		__itt_resume();
#endif

	if (frame >= initialDelay)
	{
		g_renderTime.record((float) (mRasterizeTime * 1000.0f));
		g_testTime.record((float) (mDepthTestTime * 1000.0f));

		if (frame >= initialDelay + sampleLen)
		{
#ifdef BENCHMARK_VTUNE
			__itt_pause();
#endif

			dprintf("Render time:\n");
			g_renderTime.summarize();
			dprintf("Test time:\n");
			g_testTime.summarize();

			exit(1);
		}
	}
#endif
	
    CPUTDrawGUI();
}


char *gpDefaultShaderSource =  "\n\
// ********************************************************************************************************\n\
struct VS_INPUT\n\
{\n\
    float3 Pos      : POSITION; // Projected position\n\
    float3 Norm     : NORMAL;\n\
    float2 Uv       : TEXCOORD0;\n\
};\n\
struct PS_INPUT\n\
{\n\
    float4 Pos      : SV_POSITION;\n\
    float3 Norm     : NORMAL;\n\
    float2 Uv       : TEXCOORD0;\n\
    float4 LightUv  : TEXCOORD1;\n\
    float3 Position : TEXCOORD2; // Object space position \n\
};\n\
// ********************************************************************************************************\n\
    Texture2D    TEXTURE0 : register( t0 );\n\
    SamplerState SAMPLER0 : register( s0 );\n\
// ********************************************************************************************************\n\
cbuffer cbPerModelValues\n\
{\n\
    row_major float4x4 World : WORLD;\n\
    row_major float4x4 WorldViewProjection : WORLDVIEWPROJECTION;\n\
    row_major float4x4 InverseWorld : INVERSEWORLD;\n\
              float4   LightDirection;\n\
              float4   EyePosition;\n\
    row_major float4x4 LightWorldViewProjection;\n\
};\n\
// ********************************************************************************************************\n\
// TODO: Note: nothing sets these values yet\n\
cbuffer cbPerFrameValues\n\
{\n\
    row_major float4x4  View;\n\
    row_major float4x4  Projection;\n\
};\n\
// ********************************************************************************************************\n\
PS_INPUT VSMain( VS_INPUT input )\n\
{\n\
    PS_INPUT output = (PS_INPUT)0;\n\
    output.Pos      = mul( float4( input.Pos, 1.0f), WorldViewProjection );\n\
    output.Position = mul( float4( input.Pos, 1.0f), World ).xyz;\n\
    // TODO: transform the light into object space instead of the normal into world space\n\
    output.Norm = mul( input.Norm, (float3x3)World );\n\
    output.Uv   = float2(input.Uv.x, input.Uv.y);\n\
    output.LightUv   = mul( float4( input.Pos, 1.0f), LightWorldViewProjection );\n\
    return output;\n\
}\n\
// ********************************************************************************************************\n\
float4 PSMain( PS_INPUT input ) : SV_Target\n\
{\n\
    float3  lightUv = input.LightUv.xyz / input.LightUv.w;\n\
    lightUv.xy = lightUv.xy * 0.5f + 0.5f; // TODO: Move scale and offset to matrix.\n\
    lightUv.y  = 1.0f - lightUv.y;\n\
    float3 normal         = normalize(input.Norm);\n\
    float  nDotL          = saturate( dot( normal, -LightDirection ) );\n\
    float3 eyeDirection   = normalize(input.Position - EyePosition);\n\
    float3 reflection     = reflect( eyeDirection, normal );\n\
    float  rDotL          = saturate(dot( reflection, -LightDirection ));\n\
    float3 specular       = pow(rDotL, 16.0f);\n\
    float4 diffuseTexture = TEXTURE0.Sample( SAMPLER0, input.Uv );\n\
    float ambient = 0.05;\n\
    float3 result = (nDotL+ambient) * diffuseTexture + specular;\n\
    return float4( result, 1.0f );\n\
}\n\
\n\
// ********************************************************************************************************\n\
struct VS_INPUT_NO_TEX\n\
{\n\
    float3 Pos      : POSITION; // Projected position\n\
    float3 Norm     : NORMAL;\n\
};\n\
struct PS_INPUT_NO_TEX\n\
{\n\
    float4 Pos      : SV_POSITION;\n\
    float3 Norm     : NORMAL;\n\
    float4 LightUv  : TEXCOORD1;\n\
    float3 Position : TEXCOORD0; // Object space position \n\
};\n\
// ********************************************************************************************************\n\
PS_INPUT_NO_TEX VSMainNoTexture( VS_INPUT_NO_TEX input )\n\
{\n\
    PS_INPUT_NO_TEX output = (PS_INPUT_NO_TEX)0;\n\
    output.Pos      = mul( float4( input.Pos, 1.0f), WorldViewProjection );\n\
    output.Position = mul( float4( input.Pos, 1.0f), World ).xyz;\n\
    // TODO: transform the light into object space instead of the normal into world space\n\
    output.Norm = mul( input.Norm, (float3x3)World );\n\
    output.LightUv   = mul( float4( input.Pos, 1.0f), LightWorldViewProjection );\n\
    return output;\n\
}\n\
// ********************************************************************************************************\n\
float4 PSMainNoTexture( PS_INPUT_NO_TEX input ) : SV_Target\n\
{\n\
    float3 lightUv = input.LightUv.xyz / input.LightUv.w;\n\
    float2 uv = lightUv.xy * 0.5f + 0.5f;\n\
    float3 eyeDirection = normalize(input.Position - EyePosition.xyz);\n\
    float3 normal       = normalize(input.Norm);\n\
    float  nDotL = saturate( dot( normal, -normalize(LightDirection.xyz) ) );\n\
    float3 reflection   = reflect( eyeDirection, normal );\n\
    float  rDotL        = saturate(dot( reflection, -LightDirection.xyz ));\n\
    float  specular     = 0.2f * pow( rDotL, 4.0f );\n\
    return float4( (nDotL + specular).xxx, 1.0f);\n\
}\n\
";


