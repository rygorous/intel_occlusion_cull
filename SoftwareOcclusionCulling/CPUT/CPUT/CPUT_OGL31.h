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
#ifndef __CPUT_OGL31_H__
#define __CPUT_OGL31_H__

#include <stdio.h>

// include base headers we'll need
#include "CPUTWindowWin.h"
#include "CPUT.h"
#include "CPUTMath.h"
#include "CPUTEventHandler.h"
//#include "CPUTGuiControllerDX11.h"

// CPUT objects
#include "CPUTMeshDX11.h"
#include "CPUTModelDX11.h"
#include "CPUTAssetSetDX11.h"
#include "CPUTAssetLibraryDX11.h"
#include "CPUTCamera.h"
#include "CPUTLight.h"
//#include "CPUTMaterialDX11.h"

// include all OpenGL 3.1 headers needed
//#include "GL\gl.h"      // Widnows OpenGL32 
//#include "GL\glu.h"     // Widnows GLu32

#include "GL\glew.h"
#include "GL\wglew.h"

/*
// context creation parameters
struct CPUTContextCreation
{
    int refreshRate;
    int swapChainBufferCount;
    DXGI_FORMAT swapChainFormat;
    DXGI_USAGE swapChainUsage;
};

// window creation parameters
struct CPUTWindowCreationParams
{
    bool startFullscreen;
    int windowWidth;
    int windowHeight;
    CPUTContextCreation deviceParams;
};

// Types of message boxes you can create
enum CPUT_MESSAGE_BOX_TYPE
{
    CPUT_MB_OK = MB_OK | MB_ICONINFORMATION,
    CPUT_MB_ERROR = MB_OK | MB_ICONEXCLAMATION,
    CPUT_MB_WARNING = MB_OK | MB_ICONWARNING
};
*/

// DirectX 11 CPUT layer
//-----------------------------------------------------------------------------
class CPUT_OGL31;
//extern CPUT_DX11 *gpSample;

class CPUT_OGL31:public CPUT
{

protected:
    // OpenGL 3.1 context members
    HDC     m_hdc;
    HGLRC   m_hrc;

    cString m_ResourceDirectory;

public:
    CPUT_OGL31():m_hdc(0),
        m_hrc(0)
    {
        m_ResourceDirectory=_L("");
    }
    virtual ~CPUT_OGL31();

    // context creation/destruction routines
    CPUTResult CPUTInitialize(const cString ResourceDirectory);
    CPUTResult SetCPUTResourceDirectory(const cString ResourceDirectory);
    /*
protected:
    static ID3D11Device *m_pD3dDevice;

public:
    static ID3D11Device *GetDevice();

protected:
    CPUTWindowWin           *m_pWindow;
    bool                     m_bShutdown;
    bool                     m_bFullscreenMode;
    cString                  m_ResourceDirectory;

    D3D_DRIVER_TYPE          m_driverType;
    D3D_FEATURE_LEVEL        m_featureLevel;
    ID3D11DeviceContext     *m_pContext;
    IDXGISwapChain          *m_pSwapChain;
    ID3D11RenderTargetView  *m_pRenderTargetView;
    DXGI_FORMAT              m_SwapChainFormat;

    ID3D11Texture2D         *m_pDepthStencilBuffer;
    ID3D11DepthStencilState *m_pDepthStencilState;
    ID3D11DepthStencilView  *m_pDepthStencilView; // was in protected

public:
    CPUT_DX11():m_pWindow(NULL),
        m_pContext(NULL),
        m_pSwapChain(NULL),
        m_pRenderTargetView(NULL),
        m_pDepthStencilBuffer(NULL),
        m_pDepthStencilState(NULL),
        m_pDepthStencilView(NULL),
        m_SwapChainFormat(DXGI_FORMAT_UNKNOWN),
        m_bShutdown(false),
        m_bFullscreenMode(false)
    {
		m_pTimer = (CPUTTimer*) new CPUTTimerWin();
        gpSample = this;
    }
    virtual ~CPUT_DX11();

    // context creation/destruction routines
    CPUTResult CPUTInitialize(const cString ResourceDirectory);
    CPUTResult SetCPUTResourceDirectory(const cString ResourceDirectory);
    cString *GetCPUTResourceDirectory() { return &m_ResourceDirectory; }

    D3D_FEATURE_LEVEL GetFeatureLevel() { return m_featureLevel; }

    int CPUTMessageLoop();
    CPUTResult CPUTCreateWindowAndContext(const cString WindowTitle, CPUTWindowCreationParams windowParams);

    // CPUT interfaces
    virtual void ResizeWindow(UINT width, UINT height);
    virtual void ResizeWindowSoft(UINT width, UINT height);
    void DeviceShutdown();
	void RestartCPUT();

    // side-channel calls for window layer callbacks
    void InnerExecutionLoop();

    // events
    virtual void Update(double deltaSeconds) {}
    virtual void Present() { m_pSwapChain->Present( 0, 0 ); }
    virtual void Render(double deltaSeconds) = 0;
    virtual void Create()=0;
    virtual void Shutdown();
    virtual void ChangeFullscreenMode(bool bFullscreen) {UNREFERENCED_PARAMETER(bFullscreen);}
    virtual void ReleaseSwapChain() {}
    // virtual void ResizeWindow(UINT width, UINT height){UNREFERENCED_PARAMETER(width);UNREFERENCED_PARAMETER(height);}
    virtual CPUTResult CreateContext();

    // GUI
    void CPUTDrawGUI();

    // Event Handling
    CPUTEventHandledCode CPUTHandleKeyboardEvent(CPUTKey key);
    CPUTEventHandledCode CPUTHandleMouseEvent(int x, int y, int wheel, CPUTMouseState state);

    // Sample developer utility functions
    CPUTResult CPUTToggleFullScreenMode();
    CPUTGuiControllerDX11* CPUTGetGuiController();

    // Message boxes
    void CPUTMessageBox(const cString DialogBoxTitle, const cString DialogMessage);

protected:
    // private helper functions
    void ShutdownAndDestroy();
    virtual CPUTResult CreateDXContext(CPUTContextCreation ContextParams);   // allow user to override DirectX context creation
    virtual CPUTResult DestroyDXContext();  // allow user to override DirectX context destruction
    CPUTResult         MakeWindow(const cString WindowTitle, int windowWidth, int windowHeight);
    CPUTResult         CreateAndBindDepthBuffer(int width, int height);
    
    // TODO: Put this somewhere else
    bool FindMatchingInputSlot(const char* pInputSlotName, const ID3DBlob* pVertexShaderBlob);
    */
};

#endif //#ifndef __CPUT_OGL31_H__
