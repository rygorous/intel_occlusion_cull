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
#ifndef __CPUTGUICONTROLLERDX11_H__
#define __CPUTGUICONTROLLERDX11_H__

#include "CPUTGuiController.h"
#include "CPUTTimerWin.h"

#include "CPUTButton.h"
#include "CPUTText.h"
#include "CPUTCheckbox.h"
#include "CPUTSlider.h"
#include "CPUTDropdown.h"
#include "CPUTVertexShaderOGLES.h"
#include "CPUTPixelShaderOGLES.h"
#include "CPUTRenderStateBlockOGLES.h"

//#define SAVE_RESTORE_DS_HS_GS_SHADER_STATE

// forward declarations
class CPUT_OGLES;
class CPUTButton;
class CPUTSlider;
class CPUTCheckbox;
class CPUTDropdown;
class CPUTText;
class CPUTTextureOGLES;
class CPUTFontOGLES;

const unsigned int CPUT_GUI_BUFFER_SIZE = 5000;         // size (in number of verticies) for all GUI control graphics
const unsigned int CPUT_GUI_BUFFER_STRING_SIZE = 5000;  // size (in number of verticies) for all GUI string graphics
const unsigned int CPUT_GUI_VERTEX_BUFFER_SIZE = 5000;
const CPUTControlID ID_CPUT_GUI_FPS_COUNTER = 4000000201;        // pick very random number for FPS counter string ID

// gltodo: remove extra ones of these we don't need
// gl includes
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

// the GUI controller class that dispatches the rendering calls to all the buttons
//--------------------------------------------------------------------------------
class CPUTGuiControllerOGLES:public CPUTGuiController
{ 
    
    struct GUIConstantBufferVS
    {
        // gltodo: get constant buffer working on gui shader
        //XMMATRIX Projection;
        //XMMATRIX Model;
    };
    

public:
    static CPUTGuiControllerOGLES* GetController();
    static CPUTResult DeleteController();

    // initialization
    CPUTResult Initialize(/*ID3D11DeviceContext* pImmediateContext,*/ cString& ResourceDirectory);
	CPUTResult ReleaseResources();
	

    // Control creation/deletion 'helpers'
    CPUTResult CreateButton(const cString pButtonText, CPUTControlID controlID, CPUTControlID panelID, CPUTButton** ppButton=NULL);
    CPUTResult CreateSlider(const cString pSliderText, CPUTControlID controlID, CPUTControlID panelID, CPUTSlider** ppSlider=NULL);
    CPUTResult CreateCheckbox(const cString pCheckboxText, CPUTControlID controlID, CPUTControlID panelID, CPUTCheckbox** ppCheckbox=NULL);
    CPUTResult CreateDropdown(const cString pSelectionText, CPUTControlID controlID, CPUTControlID panelID, CPUTDropdown** ppDropdown=NULL);
    CPUTResult CreateText(const cString Text,  CPUTControlID controlID, CPUTControlID panelID, CPUTText** ppStatic=NULL);    
    CPUTResult DeleteControl(CPUTControlID controlID);

    // draw routines    
    void Draw(/*ID3D11DeviceContext* pImmediateContext*/);
    void DrawFPS(bool drawfps);

private:
    static CPUTGuiControllerOGLES* m_guiController; // singleton object

    // DirectX state objects for GUI drawing
    CPUTVertexShaderOGLES* m_pGUIVertexShader;
    CPUTPixelShaderOGLES*  m_pGUIPixelShader;
    GLuint                 m_VertexLayout;
    GLuint                 m_ConstantBufferVS;
    //ID3D11InputLayout*    m_pVertexLayout;
    //ID3D11Buffer*         m_pConstantBufferVS;
    GUIConstantBufferVS   m_ModelViewMatrices;

    // Texture atlas
    CPUTTextureOGLES*           m_pControlTextureAtlas;
    GLuint                      m_ControlTextureAtlasID;
    GLuint                      m_UberBuffer;
    CPUTGUIVertex*              m_pMirrorBuffer;           
    //ID3D11ShaderResourceView*   m_pControlTextureAtlasView;
    //ID3D11Buffer*               m_pUberBuffer;
    UINT                        m_UberBufferIndex;
    UINT                        m_UberBufferMax;
    
    // Font atlas
    CPUTFontOGLES*               m_pFont;
    CPUTTextureOGLES*            m_pTextTextureAtlas;
    GLuint                      m_TextTextureAtlasView;
    GLuint                      m_TextUberBuffer;
    CPUTGUIVertex*              m_pTextMirrorBuffer;
    //ID3D11ShaderResourceView*   m_pTextTextureAtlasView;
    //ID3D11Buffer*               m_pTextUberBuffer;
    UINT                        m_TextUberBufferIndex;

    // Focused control buffers
    CPUTGUIVertex*              m_pFocusedControlMirrorBuffer;
    UINT                        m_FocusedControlBufferIndex;
    GLuint*                 m_pFocusedControlBuffer;
    CPUTGUIVertex*              m_pFocusedControlTextMirrorBuffer;
    UINT                        m_FocusedControlTextBufferIndex;
    GLuint*               m_pFocusedControlTextBuffer;

    //ID3D11Buffer*               m_pFocusedControlBuffer;
    //ID3D11Buffer*               m_pFocusedControlTextBuffer;



    // FPS
    bool                        m_bDrawFPS;
    CPUTText*                   m_pFPSCounter;
    // FPS control buffers
    CPUTGUIVertex*              m_pFPSMirrorBuffer;
    UINT                        m_FPSBufferIndex;
    GLuint                      m_FPSDirectXBuffer;
    //ID3D11Buffer*               m_pFPSDirectXBuffer;
    CPUTTimerWin*               m_pFPSTimer;

    // render state
    CPUTRenderStateBlockOGLES*   m_pGUIRenderStateBlock;
    CPUTResult UpdateUberBuffers(/*ID3D11DeviceContext* pImmediateContext */);


    // members for saving render state before/after drawing gui
    // gltodo: topology?
    //D3D11_PRIMITIVE_TOPOLOGY    m_Topology;

    // helper functions
    CPUTGuiControllerOGLES();    // singleton
    ~CPUTGuiControllerOGLES();
    CPUTResult RegisterGUIResources(/*ID3D11DeviceContext* pImmediateContext,*/ cString VertexShaderFilename, cString RenderStateFile, cString PixelShaderFilename, cString DefaultFontFilename, cString ControlTextureAtlas);
    void SetGUIDrawingState(/*ID3D11DeviceContext* pImmediateContext*/);
    void ClearGUIDrawingState(/*ID3D11DeviceContext* pImmediateContext*/);
};




#endif // #ifndef __CPUTGUICONTROLLERDX11_H__
