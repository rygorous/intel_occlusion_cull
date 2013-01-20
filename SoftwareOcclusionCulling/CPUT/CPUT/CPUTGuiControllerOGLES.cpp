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
#include "CPUTGuiControllerOGLES.h"
#include "CPUT_OGLES.h" // for CPUTSetRasterizerState()
#include "CPUTTextureOGLES.h"
#include "CPUTFontOGLES.h"


CPUTGuiControllerOGLES* CPUTGuiControllerOGLES::m_guiController = NULL;


// chained constructor
//--------------------------------------------------------------------------------
CPUTGuiControllerOGLES::CPUTGuiControllerOGLES():CPUTGuiController(),
    m_pGUIVertexShader(NULL),
    m_pGUIPixelShader(NULL),
    m_ConstantBufferVS(0),
    m_VertexLayout(0),

    // texture atlas+uber buffer
    m_pControlTextureAtlas(NULL),
    //m_pControlTextureAtlasView(NULL),
    m_UberBuffer(0),
    m_pMirrorBuffer(NULL),
    m_UberBufferIndex(0),
    m_UberBufferMax(CPUT_GUI_BUFFER_SIZE),
    m_pFont(NULL),
    m_pTextTextureAtlas(NULL),
    m_TextTextureAtlasView(0),
    m_TextUberBuffer(0),
    m_pTextMirrorBuffer(NULL),
    m_TextUberBufferIndex(0),

    m_FocusedControlBufferIndex(0),
    m_pFocusedControlBuffer(NULL),
    m_FocusedControlTextBufferIndex(0),
    m_pFocusedControlTextBuffer(NULL),
    m_pGUIRenderStateBlock(NULL),
    
    m_bDrawFPS(false),
    m_pFPSCounter(NULL),
    m_pFPSMirrorBuffer(NULL),
    m_FPSBufferIndex(0),
    m_FPSDirectXBuffer(0),
    m_pFPSTimer(NULL)
{
    m_pMirrorBuffer = new CPUTGUIVertex[CPUT_GUI_BUFFER_SIZE];
    m_pTextMirrorBuffer = new  CPUTGUIVertex[CPUT_GUI_BUFFER_STRING_SIZE];

    m_pFocusedControlMirrorBuffer = new CPUTGUIVertex[CPUT_GUI_BUFFER_SIZE];
    m_pFocusedControlTextMirrorBuffer = new CPUTGUIVertex[CPUT_GUI_BUFFER_STRING_SIZE];
    
    m_pFPSMirrorBuffer = new CPUTGUIVertex[CPUT_GUI_BUFFER_STRING_SIZE];
    m_pFPSTimer = new CPUTTimerWin();
    
#ifdef SAVE_RESTORE_DSHSGS_SHADER_STATE
    m_pGeometryShaderState=NULL;
    m_pGeometryShaderClassInstances=NULL;
    m_GeometryShaderNumClassInstances=0;

    m_pHullShaderState=NULL;
    m_pHullShaderClassInstances=NULL;
    UINT m_HullShaderNumClassInstance=0;

    m_pDomainShaderState=NULL;
    m_pDomainShaderClassIntances=NULL;
    UINT m_DomainShaderNumClassInstances=0;
#endif
}

// destructor
//--------------------------------------------------------------------------------
CPUTGuiControllerOGLES::~CPUTGuiControllerOGLES()
{
    // delete all the controls under you
	ReleaseResources();
    DeleteAllControls();

    // FPS counter
    SAFE_DELETE(m_pFPSCounter);
    SAFE_DELETE(m_pFPSTimer);

    // delete arrays
    SAFE_DELETE_ARRAY(m_pTextMirrorBuffer);
    SAFE_DELETE_ARRAY(m_pMirrorBuffer);
    SAFE_DELETE_ARRAY(m_pFocusedControlMirrorBuffer);
    SAFE_DELETE_ARRAY(m_pFocusedControlTextMirrorBuffer);
    SAFE_DELETE_ARRAY(m_pFPSMirrorBuffer);
}

// static getter
//--------------------------------------------------------------------------------
CPUTGuiControllerOGLES* CPUTGuiControllerOGLES::GetController()
{
    if(NULL==m_guiController)
        m_guiController = new CPUTGuiControllerOGLES();
    return m_guiController;
}

// Delete the controller
//--------------------------------------------------------------------------------
CPUTResult CPUTGuiControllerOGLES::DeleteController()
{
    SAFE_DELETE(m_guiController);

    return CPUT_SUCCESS;
}

//--------------------------------------------------------------------------------
CPUTResult CPUTGuiControllerOGLES::ReleaseResources()
{
//	//Release all allocated resources
//    SAFE_RELEASE(m_pGUIVertexShader);
//    SAFE_RELEASE(m_pGUIPixelShader);
//    SAFE_RELEASE(m_pVertexLayout);
//    SAFE_RELEASE(m_pConstantBufferVS);
//
//    // release the texture atlas+buffers
//    SAFE_RELEASE(m_pControlTextureAtlasView);
//    SAFE_RELEASE(m_pControlTextureAtlas);
//    SAFE_RELEASE(m_pUberBuffer);
//    
////	SAFE_RELEASE(m_pFont->m_pTextAtlas);
//    SAFE_RELEASE(m_pFont);
//    SAFE_RELEASE(m_pTextTextureAtlasView);
//    SAFE_RELEASE(m_pTextTextureAtlas);    
//    SAFE_RELEASE(m_pTextUberBuffer);
//    
//    SAFE_RELEASE(m_pFocusedControlBuffer);
//    SAFE_RELEASE(m_pFocusedControlTextBuffer);    
//
//    SAFE_RELEASE(m_pGUIRenderStateBlock);
//
//    SAFE_RELEASE(m_pFPSDirectXBuffer);
//
//    // tell all controls to unregister all their static resources
//    CPUTText::UnRegisterStaticResources();
//    CPUTButton::UnRegisterStaticResources();
//    CPUTCheckbox::UnRegisterStaticResources();
//    CPUTSlider::UnRegisterStaticResources();
//    CPUTDropdown::UnRegisterStaticResources();
//    //CPUTPane::UnRegisterStaticResources();

	return CPUT_SUCCESS;
}

