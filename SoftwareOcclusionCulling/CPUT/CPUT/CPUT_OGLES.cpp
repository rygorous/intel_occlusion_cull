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
#include "CPUT_OGLES.h"
//#include "CPUTRenderStateBlockDX11.h"

// static initializers
//ID3D11Device* CPUT_DX11::m_pD3dDevice = NULL;
CPUT_OGLES *gpSample;

void checkGlError(const char* op)
{
    for (GLint error = glGetError(); error; error = glGetError())
    {
    	char buffer[CPUT_MAX_STRING_LENGTH];
    	sprintf(buffer, "glError! Op=%s  error code:%x", op, error);
        exit(1);
    }
}

///
//  Extensions - define here 
PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR;
PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR;
PFNEGLPOSTSUBBUFFERNVPROC eglPostSubBufferNV;

PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES;

PFNGLDELETEFENCESNVPROC glDeleteFencesNV;
PFNGLGENFENCESNVPROC glGenFencesNV;
PFNGLGETFENCEIVNVPROC glGetFenceivNV;
PFNGLISFENCENVPROC glIsFenceNV;
PFNGLFINISHFENCENVPROC glFinishFenceNV;
PFNGLSETFENCENVPROC glSetFenceNV;
PFNGLTESTFENCENVPROC glTestFenceNV;



CPUT_OGLES::CPUT_OGLES():m_pWindow(NULL),
    m_eglDisplay(NULL),
    m_eglContext(NULL),
    m_eglSurface(NULL),
    m_bShutdown(false)
{
    m_pTimer = (CPUTTimer*) new CPUTTimerWin();
    gpSample = this;
}


// Destructor
//-----------------------------------------------------------------------------
CPUT_OGLES::~CPUT_OGLES()
{
 //   // all previous shutdown tasks should have happened in CPUTShutdown()

 //   // We created the default renderstate block, we release it.
 //   CPUTRenderStateBlock::GetDefaultRenderStateBlock()->Release();

 //   // destroy the window
 //   if(m_pWindow)
 //   {
 //       delete m_pWindow;
 //       m_pWindow = NULL;
 //   }

	//SAFE_DELETE(m_pTimer);
}

// initialize the CPUT system
//-----------------------------------------------------------------------------
CPUTResult CPUT_OGLES::CPUTInitialize(const cString pCPUTResourcePath)
{
    // set where CPUT will look for it's button images, fonts, etc
    return SetCPUTResourceDirectory(pCPUTResourcePath);
}


// Set where CPUT will look for it's button images, fonts, etc
//-----------------------------------------------------------------------------
CPUTResult CPUT_OGLES::SetCPUTResourceDirectory(const cString ResourceDirectory)
{
    // check to see if the specified directory is valid
    CPUTResult result = CPUT_SUCCESS;

    // resolve the directory to a full path
    cString fullPath;
    CPUTOSServices *pServices = CPUTOSServices::GetOSServices();
    result = pServices->ResolveAbsolutePathAndFilename(ResourceDirectory, &fullPath);
    if(CPUTFAILED(result))
    {
        return result;
    }

    // check existence of directory
    result = pServices->DoesDirectoryExist(fullPath);
    if(CPUTFAILED(result))
    {
        return result;
    }

    // set the resource directory (absolute path)
    m_ResourceDirectory = fullPath;

    // tell the gui system where to look for it's resources
    // todo: do we want to force a flush/reload of all resources (i.e. change control graphics)
    result = CPUTGuiControllerOGLES::GetController()->SetResourceDirectory(ResourceDirectory);

    return result;
}

// Handle keyboard events
//-----------------------------------------------------------------------------
CPUTEventHandledCode CPUT_OGLES::CPUTHandleKeyboardEvent(CPUTKey key)
{
    // dispatch event to GUI to handle GUI triggers (if any)
    CPUTEventHandledCode handleCode = CPUTGuiControllerOGLES::GetController()->HandleKeyboardEvent(key);

    // dispatch event to users HandleMouseEvent() method
    HEAPCHECK;
    handleCode = HandleKeyboardEvent(key);
    HEAPCHECK;

    return handleCode;
}

// Handle mouse events
//-----------------------------------------------------------------------------
CPUTEventHandledCode CPUT_OGLES::CPUTHandleMouseEvent(int x, int y, int wheel, CPUTMouseState state)
{
    // dispatch event to GUI to handle GUI triggers (if any)
    CPUTEventHandledCode handleCode = CPUTGuiControllerOGLES::GetController()->HandleMouseEvent(x,y,wheel,state);

    // dispatch event to users HandleMouseEvent() method if it wasn't consumed by the GUI
    if(CPUT_EVENT_HANDLED != handleCode)
    {
        HEAPCHECK;
        handleCode = HandleMouseEvent(x,y,wheel,state);
        HEAPCHECK;
    }

    return handleCode;
}


// Call appropriate OS create window call
//-----------------------------------------------------------------------------
CPUTResult CPUT_OGLES::MakeWindow(const cString WindowTitle, int windowWidth, int windowHeight, int windowX, int windowY)
{
    CPUTResult result;

    HEAPCHECK;

    // if we have a window, destroy it
    if(m_pWindow)
    {
        delete m_pWindow;
        m_pWindow = NULL;
    }

    HEAPCHECK;

    // create the OS window
    m_pWindow = new CPUTWindowWin();

    result = m_pWindow->Create((CPUT*)this, WindowTitle, windowWidth, windowHeight, windowX, windowY);

    HEAPCHECK;

    return result;
}

// Create the OpenGL|ES context
//-----------------------------------------------------------------------------
CPUTResult CPUT_OGLES::CreateGLESContext(CPUTContextCreation ContextParams )
{
    HWND hWnd = m_pWindow->GetHWnd();

   if ( !CreateEGLContext ( hWnd,
                            &m_eglDisplay,
                            &m_eglContext,
                            &m_eglSurface,
                            ContextParams.configAttribList,
                            ContextParams.surfaceAttribList ) )
   {
       ASSERT(0, _L("GL|ES context creation error"));
   }
   
    return CPUT_SUCCESS;
}


// CreateEGLContext()
//
//    Creates an EGL rendering context and all associated elements
//
EGLBoolean CPUT_OGLES::CreateEGLContext ( EGLNativeWindowType hWnd, EGLDisplay* eglDisplay,
                              EGLContext* eglContext, EGLSurface* eglSurface,
                              EGLint* configAttribList, EGLint* surfaceAttribList)
{
   EGLint numConfigs;
   EGLint majorVersion;
   EGLint minorVersion;
   EGLDisplay display;
   EGLContext context;
   EGLSurface surface;
   EGLConfig config;
   EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE, EGL_NONE };
   
   checkGlError("CreateEGLContext #1");
   // Get Display
   display = eglGetDisplay(GetDC(hWnd));
   if ( display == EGL_NO_DISPLAY )
   {
      return EGL_FALSE;
   }
   checkGlError("CreateEGLContext #2");

   // Initialize EGL
   if ( !eglInitialize(display, &majorVersion, &minorVersion) )
   {
      return EGL_FALSE;
   }
   checkGlError("CreateEGLContext #3");

   // Bind to extensions
   eglCreateImageKHR = (PFNEGLCREATEIMAGEKHRPROC) eglGetProcAddress("eglCreateImageKHR");
   eglDestroyImageKHR = (PFNEGLDESTROYIMAGEKHRPROC) eglGetProcAddress("eglDestroyImageKHR");
   
   eglPostSubBufferNV = (PFNEGLPOSTSUBBUFFERNVPROC) eglGetProcAddress("eglPostSubBufferNV");

   glEGLImageTargetTexture2DOES = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC) eglGetProcAddress("glEGLImageTargetTexture2DOES");
   
   glDeleteFencesNV = (PFNGLDELETEFENCESNVPROC) eglGetProcAddress("glDeleteFencesNV");
   glGenFencesNV = (PFNGLGENFENCESNVPROC) eglGetProcAddress("glGenFencesNV");
   glGetFenceivNV = (PFNGLGETFENCEIVNVPROC) eglGetProcAddress("glGetFenceivNV");
   glIsFenceNV = (PFNGLISFENCENVPROC) eglGetProcAddress("glIsFenceNV");
   glFinishFenceNV = (PFNGLFINISHFENCENVPROC) eglGetProcAddress("glFinishFenceNV");
   glSetFenceNV = (PFNGLSETFENCENVPROC) eglGetProcAddress("glSetFenceNV");
   glTestFenceNV = (PFNGLTESTFENCENVPROC) eglGetProcAddress("glTestFenceNV");
   checkGlError("CreateEGLContext #4");

   // Get configs
   if ( !eglGetConfigs(display, NULL, 0, &numConfigs) )
   {
      return EGL_FALSE;
   }
   checkGlError("CreateEGLContext #5");


   // Choose config
   if ( !eglChooseConfig(display, configAttribList, &config, 1, &numConfigs) )
   {
      return EGL_FALSE;
   }
   checkGlError("CreateEGLContext #6");

   // Create a surface
   surface = eglCreateWindowSurface(display, config, (EGLNativeWindowType)hWnd, surfaceAttribList);
   if ( surface == EGL_NO_SURFACE )
   {
      return EGL_FALSE;
   }
   checkGlError("CreateEGLContext #7");

   // Create a GL context
   context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs );
   if ( context == EGL_NO_CONTEXT )
   {
      return EGL_FALSE;
   }   
   checkGlError("CreateEGLContext #8");

   // Make the context current
   if ( !eglMakeCurrent(display, surface, surface, context) )
   {
      return EGL_FALSE;
   }   
   checkGlError("CreateEGLContext #9");

   *eglDisplay = display;
   *eglSurface = surface;
   *eglContext = context;
   return EGL_TRUE;
} 



// Destroy the OpenGL|ES context
//-----------------------------------------------------------------------------
CPUTResult CPUT_OGLES::DestroyGLESContext()
{
    return CPUT_SUCCESS;
}
/*
// Return the current GUI controller
//-----------------------------------------------------------------------------
CPUTGuiControllerOGLES* CPUT_DX11::CPUTGetGuiController()
{
    return CPUTGuiControllerOGLES::GetController();
}



// Create a DX11 context
//-----------------------------------------------------------------------------
CPUTResult CPUT_DX11::CreateDXContext(CPUTContextCreation ContextParams )
{

    HRESULT hr = S_OK;
    CPUTResult result = CPUT_SUCCESS;

    // window params
    RECT rc;
    HWND hWnd = m_pWindow->GetHWnd();
    GetClientRect( hWnd, &rc );
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    // set up DirectX creation parameters
    m_driverType = D3D_DRIVER_TYPE_NULL;
    m_featureLevel = D3D_FEATURE_LEVEL_11_0;
    m_pD3dDevice = NULL;
    m_pContext = NULL;
    m_pSwapChain = NULL;
    m_pRenderTargetView = NULL;
    UINT createDeviceFlags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = ARRAYSIZE( driverTypes );

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    UINT numFeatureLevels = ARRAYSIZE( featureLevels );

    // swap chain information
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory( &sd, sizeof( sd ) );
    sd.BufferCount = ContextParams.swapChainBufferCount;
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;

    m_SwapChainFormat = ContextParams.swapChainFormat;
    sd.BufferDesc.Format = ContextParams.swapChainFormat; //SWAP_CHAIN_FORMAT; //DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = ContextParams.refreshRate;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = ContextParams.swapChainUsage; //DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = ContextParams.swapChainBufferCount;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;

    // walk devices and create device and swap chain on best matching piece of hardware
    for( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
    {
        m_driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDeviceAndSwapChain(
            NULL,
            m_driverType,
            NULL,
            createDeviceFlags,
            featureLevels,
            numFeatureLevels,
            D3D11_SDK_VERSION,
            &sd,
            &m_pSwapChain,
            &m_pD3dDevice,
            &m_featureLevel,
            &m_pContext
        );
        if( SUCCEEDED( hr ) )
        {
            break;
        }
    }
    ASSERT( SUCCEEDED(hr), _L("Failed creating device and swap chain.") );

    // call the DeviceCreated callback/backbuffer/etc creation
    result = CreateContext();

    CPUTRenderStateBlock *pBlock = new CPUTRenderStateBlockDX11();
    pBlock->CreateNativeResources();
    CPUTRenderStateBlock::SetDefaultRenderStateBlock( pBlock );

    return result;
}

// Return the active D3D device used to create the context
//-----------------------------------------------------------------------------
ID3D11Device* CPUT_DX11::GetDevice()
{
    return m_pD3dDevice;
}

// Default creation routine for making the back/stencil buffers
//-----------------------------------------------------------------------------
CPUTResult CPUT_DX11::CreateContext()
{
    HRESULT hr;
    CPUTResult result;
    RECT rc;
    HWND hWnd = m_pWindow->GetHWnd();

    GetClientRect( hWnd, &rc );
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    // Create a render target view
    ID3D11Texture2D* pBackBuffer = NULL;
    hr = m_pSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&pBackBuffer );
    ASSERT( SUCCEEDED(hr), _L("Failed getting back buffer.") );

    hr = m_pD3dDevice->CreateRenderTargetView( pBackBuffer, NULL, &m_pRenderTargetView );
    pBackBuffer->Release();
    ASSERT( SUCCEEDED(hr), _L("Failed creating render target view.") );
    CPUTSetDebugName( m_pRenderTargetView, _L("BackBufferView") );

    // create depth/stencil buffer
    result = CreateAndBindDepthBuffer(width, height);
    ASSERT( SUCCEEDED(hr), _L("Failed creating and binding depth buffer.") );

    // Setup the viewport
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    m_pContext->RSSetViewports( 1, &vp );

    return CPUT_SUCCESS;
}

// destroy the DX context and release all resources
//-----------------------------------------------------------------------------
CPUTResult CPUT_DX11::DestroyDXContext()
{
    if (m_pContext) {
        m_pContext->ClearState();
        m_pContext->Flush();
    }

    SAFE_RELEASE( m_pRenderTargetView );
    SAFE_RELEASE( m_pDepthStencilBuffer );
    SAFE_RELEASE( m_pDepthStencilState );
    SAFE_RELEASE( m_pDepthStencilView );
    SAFE_RELEASE( m_pContext );
    SAFE_RELEASE( m_pD3dDevice );
    SAFE_RELEASE( m_pSwapChain );

    return CPUT_SUCCESS;
}

// Toggle the fullscreen mode
// This routine keeps the current desktop resolution.  DougB suggested allowing
// one to go fullscreen in a different resolution
//-----------------------------------------------------------------------------
CPUTResult CPUT_DX11::CPUTToggleFullScreenMode()
{
    m_bFullscreenMode = !m_bFullscreenMode;
    HRESULT hr = m_pSwapChain->SetFullscreenState(m_bFullscreenMode, NULL);
    ASSERT( SUCCEEDED(hr), _L("Failed toggling full screen mode.") );

    // trigger resize event
    int x,y,width,height;
    CPUTOSServices::GetOSServices()->GetClientDimensions(&x, &y, &width, &height);
    ResizeWindow(width,height);

    // trigger a fullscreen mode change call if the sample has decided to handle the mode change
    ChangeFullscreenMode( m_bFullscreenMode );

    return CPUT_SUCCESS;
}

// Create the depth buffer
//-----------------------------------------------------------------------------
CPUTResult CPUT_DX11::CreateAndBindDepthBuffer(int width, int height)
{
    HRESULT hr;

    // Clamp to minimum size of 1x1 pixel
    width  = max( width, 1 );
    height = max( height, 1 );

    // ---- DEPTH BUFFER ---
    // 1. Initialize the description of the depth buffer.
    D3D11_TEXTURE2D_DESC depthBufferDesc;
    ZeroMemory(&depthBufferDesc, sizeof(depthBufferDesc));

    // Set up the description of the depth buffer.
    depthBufferDesc.Width = width;
    depthBufferDesc.Height = height;
    depthBufferDesc.MipLevels = 1;
    depthBufferDesc.ArraySize = 1;
    depthBufferDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthBufferDesc.SampleDesc.Count = 1;
    depthBufferDesc.SampleDesc.Quality = 0;
    depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthBufferDesc.CPUAccessFlags = 0;
    depthBufferDesc.MiscFlags = 0;

    // Create the texture for the depth buffer using the filled out description.
    hr = m_pD3dDevice->CreateTexture2D(&depthBufferDesc, NULL, &m_pDepthStencilBuffer);
    ASSERT( SUCCEEDED(hr), _L("Failed to create texture.") );
    CPUTSetDebugName( m_pDepthStencilBuffer, _L("DepthBufferView") );

    // 2. Initialize the description of the stencil state.
    D3D11_DEPTH_STENCIL_DESC depthStencilDesc;	
    ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));

    // Set up the description of the stencil state.
    depthStencilDesc.DepthEnable = true;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;

    depthStencilDesc.StencilEnable = true;
    depthStencilDesc.StencilReadMask = 0xFF;
    depthStencilDesc.StencilWriteMask = 0xFF;

    // Stencil operations if pixel is front-facing.
    depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
    depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

    // Stencil operations if pixel is back-facing.
    depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
    depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

    // Create the depth stencil state.
    hr = m_pD3dDevice->CreateDepthStencilState(&depthStencilDesc, &m_pDepthStencilState);
    ASSERT( SUCCEEDED(hr), _L("Failed to create depth-stencil state.") );

    // Set the depth stencil state.
    D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
    ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));

    m_pContext->OMSetDepthStencilState(m_pDepthStencilState, 1);

    // Initialize the depth stencil view.
    ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));

    // Set up the depth stencil view description.
    depthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    depthStencilViewDesc.Texture2D.MipSlice = 0;

    // Create the depth stencil view.
    hr = m_pD3dDevice->CreateDepthStencilView(m_pDepthStencilBuffer, &depthStencilViewDesc, &m_pDepthStencilView);
    ASSERT( SUCCEEDED(hr), _L("Failed to create depth-stencil view.") );
    CPUTSetDebugName( m_pDepthStencilView, _L("DepthStencilView") );

    // Bind the render target view and depth stencil buffer to the output render pipeline.
    m_pContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_pDepthStencilView);
    CPUTRenderTargetColor::SetActiveRenderTargetView( m_pRenderTargetView );
    CPUTRenderTargetDepth::SetActiveDepthStencilView( m_pDepthStencilView );

    return CPUT_SUCCESS;
}

// incoming resize event to be handled and translated
//-----------------------------------------------------------------------------
void CPUT_DX11::ResizeWindow(UINT width, UINT height)
{
    CPUTResult result;
    CPUT::ResizeWindow( width, height );

    // Call the sample's clean up code if present.
    ReleaseSwapChain();

    // handle the internals of a resize
    int windowWidth, windowHeight;
    CPUTOSServices* pServices = CPUTOSServices::GetOSServices();
    pServices->GetClientDimensions( &windowWidth, &windowHeight);

    SAFE_RELEASE(m_pRenderTargetView);

    // resize the swap chain
    m_pSwapChain->ResizeBuffers(1, windowWidth, windowHeight, m_SwapChainFormat, 0);

    // re-create the render-target view
    ID3D11Texture2D* pSwapChainBuffer = NULL;
    if(FAILED(m_pSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D), (LPVOID*) (&pSwapChainBuffer))))
    {
        // resize failed - likely driver ran out of memory
        ASSERT(0,_L(""));
        return;
    }

    if(FAILED(m_pD3dDevice->CreateRenderTargetView( pSwapChainBuffer, NULL, &m_pRenderTargetView)))
    {
        // resize failed - likely driver ran out of memory
        ASSERT(0,_L(""));
    }

    // release the temporary swap chain buffer
    pSwapChainBuffer->Release();
    pSwapChainBuffer = NULL;

    // release the old depth buffer objects
    SAFE_RELEASE(m_pDepthStencilBuffer);
    SAFE_RELEASE(m_pDepthStencilState);
    SAFE_RELEASE(m_pDepthStencilView);

    result = CreateAndBindDepthBuffer(windowWidth, windowHeight);
    if(CPUTFAILED(result))
    {
        // depth buffer creation error
        ASSERT(0,_L(""));
    }

    // set the viewport
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT) windowWidth;
    vp.Height = (FLOAT)windowHeight;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    m_pContext->RSSetViewports( 1, &vp );

    // trigger the GUI manager to resize
    CPUTGuiControllerOGLES::GetController()->Resize();
}

// 'soft' resize - just stretch-blit
//-----------------------------------------------------------------------------
void CPUT_DX11::ResizeWindowSoft(UINT width, UINT height)
{
    UNREFERENCED_PARAMETER(width);
    UNREFERENCED_PARAMETER(height);
    // trigger the GUI manager to resize
    CPUTGuiControllerOGLES::GetController()->Resize();

    InnerExecutionLoop();
}
*/
// Call the user's Render() callback (if it exists)
//-----------------------------------------------------------------------------
void CPUT_OGLES::InnerExecutionLoop()
{
#ifdef CPUT_GPA_INSTRUMENTATION
    D3DPERF_BeginEvent(D3DCOLOR(0xff0000), L"CPUT User's Render() ");
#endif
    if(!m_bShutdown)
    {
		double elapsedTime = m_pTimer->GetElapsedTime();
		Update(elapsedTime);
        Present(); // Note: Presenting immediately before Rendering minimizes CPU stalls (i.e., execute Update() before Present() stalls)
        Render(elapsedTime);
    }
    else
    {
        Present(); // Need to present, or will leak all references held by previous Render()!
        ShutdownAndDestroy();
    }

#ifdef CPUT_GPA_INSTRUMENTATION
    D3DPERF_EndEvent();
#endif
}
/*

// draw all the GUI controls
//-----------------------------------------------------------------------------
void CPUT_DX11::CPUTDrawGUI()
{
#ifdef CPUT_GPA_INSTRUMENTATION
    D3DPERF_BeginEvent(D3DCOLOR(0xff0000), L"CPUT Draw GUI");
#endif

    // draw all the Gui controls
    HEAPCHECK;
        CPUTGuiControllerOGLES::GetController()->Draw(m_pContext);
    HEAPCHECK;

#ifdef CPUT_GPA_INSTRUMENTATION
        D3DPERF_EndEvent();
#endif
}
*/
// Create a window context
//-----------------------------------------------------------------------------
CPUTResult CPUT_OGLES::CPUTCreateWindowAndContext(const cString WindowTitle, CPUTWindowCreationParams windowParams)
{
    CPUTResult result = CPUT_SUCCESS;

    HEAPCHECK;

	// create the window
    result = MakeWindow(WindowTitle, windowParams.windowWidth, windowParams.windowHeight, windowParams.windowPositionX, windowParams.windowPositionY);
    if(CPUTFAILED(result))
    {
        return result;
    }
    checkGlError("CPUTCreateWindowAndContext #1");
    HEAPCHECK;
    
    // create the DX context
    result = CreateGLESContext(windowParams.deviceParams);
    if(CPUTFAILED(result))
    {
        return result;
    }
    checkGlError("CPUTCreateWindowAndContext #2");
    // call user's ::Create() function
        HEAPCHECK;
    Create();
    HEAPCHECK;
    checkGlError("CPUTCreateWindowAndContext #3");
    /*
    HEAPCHECK;
#define ENABLE_GUI
#ifdef ENABLE_GUI
    // initialize the gui controller 
    // Use the ResourceDirectory that was given during the Initialize() function
    // to locate the GUI+font resources
    CPUTGuiControllerOGLES* pGUIController = CPUTGuiControllerOGLES::GetController();
    cString ResourceDirectory = GetCPUTResourceDirectory();
    result = pGUIController->Initialize(m_pContext, ResourceDirectory);
    if(CPUTFAILED(result))
    {
        return result;
    }
    // register the callback object for GUI events as our sample
    CPUTGuiControllerOGLES::GetController()->SetCallback(this);
#endif
    // trigger a post-create user callback event
    HEAPCHECK;
    Create();
    HEAPCHECK;

	//
	// Start the timer after everything is initialized and assets have been loaded
	//
	m_pTimer->StartTimer();

    // if someone triggers the shutdown routine in on-create, exit
    if(m_bShutdown)
    {
        return result;
    }

    // does user want to start in fullscreen mode?
    if(true == windowParams.startFullscreen)
    {
        result = CPUTToggleFullScreenMode();
        if(CPUTFAILED(result))
        {
            return result;
        }
    }

    // fill first frame with clear values so render order later is ok
    const float srgbClearColor[] = { 0.0993f, 0.0993f, 0.0993f, 1.0f };
    m_pContext->ClearRenderTargetView( m_pRenderTargetView, srgbClearColor );
    m_pContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 0.0f, 0);

    // trigger a 'resize' event
    int x,y,width,height;
    CPUTOSServices::GetOSServices()->GetClientDimensions(&x, &y, &width, &height);
    ResizeWindow(width,height);
    */
    return result;
}
/*
// Pop up a message box with specified title/text
//-----------------------------------------------------------------------------
void CPUT_DX11::CPUTMessageBox(const cString DialogBoxTitle, const cString DialogMessage)
{
    CPUTOSServices::GetOSServices()->OpenMessageBox(DialogBoxTitle.c_str(), DialogMessage.c_str());
}
*/
// start main message loop
//-----------------------------------------------------------------------------
int CPUT_OGLES::CPUTMessageLoop()
{
//#ifdef CPUT_GPA_INSTRUMENTATION
//    D3DPERF_BeginEvent(D3DCOLOR(0xff0000), L"CPUTMessageLoop");
//#endif

    return m_pWindow->StartMessageLoop();

//#ifdef CPUT_GPA_INSTRUMENTATION
//    D3DPERF_EndEvent();
//#endif
}
/*
// Window is closing. Shut the system to shut down now, not later.
//-----------------------------------------------------------------------------
void CPUT_DX11::DeviceShutdown()
{
    if(false == m_bShutdown)
    {
        m_bShutdown = true;
        ShutdownAndDestroy();
    }
}
*/
// Shutdown the CPUT system
// Destroy all 'global' resource handling objects, all asset handlers,
// the DX context, and everything EXCEPT the window
//-----------------------------------------------------------------------------
void CPUT_OGLES::Shutdown()
{
    // release the lock on the mouse (if there was one)
    CPUTOSServices::GetOSServices()->ReleaseMouse();
    m_bShutdown = true;
}
/*
// Frees all resources and removes all assets from asset library
//-----------------------------------------------------------------------------
void CPUT_DX11::RestartCPUT()
{
    //
    // Clear out all CPUT resources
    //
    CPUTVertexLayoutCacheDX11::GetVertexLayoutCache()->ClearLayoutCache();
    CPUTAssetLibrary::GetAssetLibrary()->ReleaseAllLibraryLists();
	CPUTGuiControllerOGLES::GetController()->DeleteAllControls();
	CPUTGuiControllerOGLES::GetController()->ReleaseResources();

    //
    // Clear out all DX resources and contexts
    //
    DestroyDXContext();

    //
    // Signal the window to close
    //
    m_pWindow->Destroy();
	
	//
	// Clear out the timer
	//
	m_pTimer->StopTimer();
	m_pTimer->ResetTimer();
    
    HEAPCHECK;
}
*/
// Actually destroy all 'global' resource handling objects, all asset handlers,
// the DX context, and everything EXCEPT the window
//-----------------------------------------------------------------------------
void CPUT_OGLES::ShutdownAndDestroy()
{
    // make sure no more rendering can happen
    m_bShutdown = true;

    // call the user's OnShutdown code
    Shutdown();
    /*
    CPUTVertexLayoutCacheDX11::DeleteVertexLayoutCache();
    CPUTAssetLibraryDX11::DeleteAssetLibrary();
    CPUTGuiControllerOGLES::DeleteController();
    
// #ifdef _DEBUG
#if 0
    ID3D11Debug* pDebug;
    m_pD3dDevice->QueryInterface(IID_ID3D11Debug, (VOID**)(&pDebug));
    if( pDebug )
    {
        pDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
        pDebug->Release();
    }
#endif
    DestroyDXContext();
    CPUTOSServices::DeleteOSServices();
    */
    // Tell the window layer to post a close-window message to OS
    // and stop the message pump
    m_pWindow->Destroy();

    HEAPCHECK;
}
/*
//-----------------------------------------------------------------------------
void CPUTSetDebugName( void *pResource, cString name )
{
#ifdef _DEBUG
    char pCharString[CPUT_MAX_STRING_LENGTH];
    const wchar_t *pWideString = name.c_str();
    UINT ii;
    UINT length = min( (UINT)name.length(), (CPUT_MAX_STRING_LENGTH-1));
    for(ii=0; ii<length; ii++)
    {
        pCharString[ii] = (char)pWideString[ii];
    }
    pCharString[ii] = 0; // Force NULL termination
    ((ID3D11DeviceChild*)pResource)->SetPrivateData( WKPDID_D3DDebugObjectName, (UINT)name.length(), pCharString );
#endif // _DEBUG
}
*/