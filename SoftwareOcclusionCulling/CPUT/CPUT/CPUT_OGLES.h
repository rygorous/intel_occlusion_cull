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
#ifndef __CPUT_OGLES_H__
#define __CPUT_OGLES_H__

#include <stdio.h>

// include base headers we'll need
#include "CPUTWindowWin.h"
#include "CPUT.h"
#include "CPUTMath.h"
#include "CPUTEventHandler.h"
#include "CPUTGuiControllerOGLES.h"

///
//  Includes
// Be sure to link in the libraries: libEGL.lib, libGLESv2.lib
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>


//// CPUT objects
//#include "CPUTMeshDX11.h"
#include "CPUTModelOGLES.h"
//#include "CPUTAssetSetDX11.h"
#include "CPUTAssetLibraryOGLES.h"
#include "CPUTPixelShaderOGLES.h"
//#include "CPUTCamera.h"
//#include "CPUTLight.h"
//#include "CPUTMaterialDX11.h"
//
//// include all DX11 headers needed
//#include <d3d11.h>
//#include <d3DX11.h>
//#include <D3DX11async.h>    // for D3DX11CompileFromFile()
//#include <D3DCompiler.h>    // for D3DReflect() / D3DX11Refection - IMPORTANT NOTE: include directories MUST list DX SDK include path BEFORE
//// Windows include paths or you'll get compile errors with D3DShader.h

/// esCreateWindow flag - RGB color buffer
#define ES_WINDOW_RGB           0
/// esCreateWindow flag - ALPHA color buffer
#define ES_WINDOW_ALPHA         1 
/// esCreateWindow flag - depth buffer
#define ES_WINDOW_DEPTH         2 
/// esCreateWindow flag - stencil buffer
#define ES_WINDOW_STENCIL       4
/// esCreateWindow flag - multi-sample buffer
#define ES_WINDOW_MULTISAMPLE   8
/// esCreateWindow flag - EGL_POST_SUB_BUFFER_NV supported.
#define ES_WINDOW_POST_SUB_BUFFER_SUPPORTED 16


///
//  Extensions
//
extern PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR;
extern PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR;

extern PFNEGLPOSTSUBBUFFERNVPROC eglPostSubBufferNV;

extern PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES;

extern PFNGLDELETEFENCESNVPROC glDeleteFencesNV;
extern PFNGLGENFENCESNVPROC glGenFencesNV;
extern PFNGLGETFENCEIVNVPROC glGetFenceivNV;
extern PFNGLISFENCENVPROC glIsFenceNV;
extern PFNGLFINISHFENCENVPROC glFinishFenceNV;
extern PFNGLSETFENCENVPROC glSetFenceNV;
extern PFNGLTESTFENCENVPROC glTestFenceNV;



// context creation parameters
struct CPUTContextCreation
{
//    int refreshRate;
//    int swapChainBufferCount;
//    DXGI_FORMAT swapChainFormat;
//    DXGI_USAGE swapChainUsage;
   EGLint* configAttribList;
   EGLint* surfaceAttribList;
};

//// window creation parameters
// window creation parameters
struct CPUTWindowCreationParams
{
    bool startFullscreen;
    int windowWidth;
    int windowHeight;
    int windowPositionX;
    int windowPositionY;
    CPUTContextCreation deviceParams;
};
//
//// Types of message boxes you can create
//enum CPUT_MESSAGE_BOX_TYPE
//{
//    CPUT_MB_OK = MB_OK | MB_ICONINFORMATION,
//    CPUT_MB_ERROR = MB_OK | MB_ICONEXCLAMATION,
//    CPUT_MB_WARNING = MB_OK | MB_ICONWARNING
//};

extern void checkGlError(const char* op);

// DirectX 11 CPUT layer
//-----------------------------------------------------------------------------
class CPUT_OGLES;
extern CPUT_OGLES *gpSample;

class CPUT_OGLES:public CPUT
{    
protected:  
    // EGL display
   EGLDisplay  m_eglDisplay;      
   // EGL context
   EGLContext  m_eglContext;
   // EGL surface
   EGLSurface  m_eglSurface;

   CPUT_OGLES();

    /*
public:
    static ID3D11Device *GetDevice();
    */
protected:
    CPUTWindowWin           *m_pWindow;  
    bool                     m_bShutdown;
    bool                     m_bFullscreenMode;
    
    cString                  m_ResourceDirectory;
    /*
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
    */
public:
    virtual ~CPUT_OGLES();

    // context creation/destruction routines
    CPUTResult CPUTInitialize(const cString ResourceDirectory);
    CPUTResult SetCPUTResourceDirectory(const cString ResourceDirectory);
    cString GetCPUTResourceDirectory() { return m_ResourceDirectory; }
    
    //D3D_FEATURE_LEVEL GetFeatureLevel() { return m_featureLevel; }

    int CPUTMessageLoop();
    CPUTResult CPUTCreateWindowAndContext(const cString WindowTitle, CPUTWindowCreationParams windowParams);
    /*
    // CPUT interfaces
    virtual void ResizeWindow(UINT width, UINT height);
    virtual void ResizeWindowSoft(UINT width, UINT height);
    void DeviceShutdown();
	void RestartCPUT();
    */
    // side-channel calls for window layer callbacks
    void InnerExecutionLoop();
    
    // events
    virtual void Update(double deltaSeconds) {}
    virtual void Present() { eglSwapBuffers ( m_eglDisplay, m_eglSurface ); }
    virtual void Render(double deltaSeconds) = 0;
    virtual void Create()=0;
    virtual void Shutdown();
    virtual void ChangeFullscreenMode(bool bFullscreen) {UNREFERENCED_PARAMETER(bFullscreen);}
    virtual void ReleaseSwapChain() {}
    virtual void ResizeWindow(UINT width, UINT height){UNREFERENCED_PARAMETER(width);UNREFERENCED_PARAMETER(height);}
    //virtual CPUTResult CreateContext();
    /*
    // GUI
    void CPUTDrawGUI();
    */
    // Event Handling
    CPUTEventHandledCode CPUTHandleKeyboardEvent(CPUTKey key);
    CPUTEventHandledCode CPUTHandleMouseEvent(int x, int y, int wheel, CPUTMouseState state);
    /*
    // Sample developer utility functions
    CPUTResult CPUTToggleFullScreenMode();
    CPUTGuiControllerDX11* CPUTGetGuiController();

    // Message boxes
    void CPUTMessageBox(const cString DialogBoxTitle, const cString DialogMessage);
    */
protected:
    // private helper functions
    void ShutdownAndDestroy();
    
    virtual CPUTResult CreateGLESContext(CPUTContextCreation ContextParams);   // allow user to override DirectX context creation    
    EGLBoolean CreateEGLContext ( EGLNativeWindowType hWnd, EGLDisplay* eglDisplay,
                              EGLContext* eglContext, EGLSurface* eglSurface,
                              EGLint* configAttribList, EGLint* surfaceAttribList);
    virtual CPUTResult DestroyGLESContext();  // allow user to override DirectX context destruction
    
    CPUTResult         MakeWindow(const cString WindowTitle, int windowWidth, int windowHeight, int windowX, int windowY);
    /*
    CPUTResult         CreateAndBindDepthBuffer(int width, int height);
    
    // TODO: Put this somewhere else
    bool               FindMatchingInputSlot(const char* pInputSlotName, const ID3DBlob* pVertexShaderBlob);
    */
};

#endif //#ifndef __CPUT_OGLES_H__
