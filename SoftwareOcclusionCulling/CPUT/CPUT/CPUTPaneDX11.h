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
#ifndef __CPUTPANEDX11_H__
#define __CPUTPANEDX11_H__



// include CPUT components used by this control
#include "CPUTPane.h"
#include "CPUTGuiControllerDX11.h"
#include "CPUTStaticDX11.h"
#include "CPUTGuiHelperDX11.h"

// include all DX11 headers needed
#include <d3d11.h>
#include <d3DX11.h>
#include <xnamath.h> // for XMMATRIX objects


#define CPUT_NUM_IMAGES_IN_PANE 9

// DX11 pane control
//--------------------------------------------------------------------------------
class CPUTPaneDX11:public CPUTPane
{
public:
    // Construction
    CPUTPaneDX11(ID3D11DeviceContext* pImmediateContext, const cString ControlText, CPUTControlID id);
    virtual ~CPUTPaneDX11();

    //CPUTEventHandler
    virtual CPUTEventHandledCode HandleKeyboardEvent(CPUTKey key);
    virtual CPUTEventHandledCode HandleMouseEvent(int x, int y, int wheel, CPUTMouseState state);

    //CPUTControl
    virtual bool ContainsPoint(int x, int y);
    virtual void SetPosition(int x, int y);
    virtual void SetDimensions(ID3D11DeviceContext* pImmediateContext, int width, int height);
    virtual void AddControl(ID3D11DeviceContext* pImmediateContext, CPUTControl* pControl);

    // Register assets
    static CPUTResult RegisterStaticResources(ID3D11Device* pD3dDevice);
    static CPUTResult UnRegisterStaticResources();

    CPUTResult RegisterInstanceResources(ID3D11DeviceContext* pImmediateContext);

    CPUTResult UnRegisterInstanceResources();

    // CPUTPaneDX11
    virtual void Draw(ID3D11DeviceContext* pImmediateContext, ID3D11Buffer* pVSConstantBuffer, ID3D11VertexShader* vertexShader, ID3D11PixelShader* pixelShader);

private:
    struct GUIVSConstantBuffer
    {
        XMMATRIX projection;
        XMMATRIX model;
    };

    // Static resources
    static bool m_StaticRegistered;
    static ID3DBlob* m_pVertexShaderBlob;
    static ID3D11Buffer* m_pVSConstantBuffer;

    static ID3D11ShaderResourceView* m_pIdleTextureResView[CPUT_NUM_IMAGES_IN_PANE];

    static CPUT_SIZE m_pPaneIdleImageSizeList[CPUT_NUM_IMAGES_IN_PANE];

    static int m_SmallestLeftSizeIdle;
    static int m_SmallestRightSizeIdle;
    static int m_SmallestTopSizeIdle;
    static int m_SmallestBottomSizeIdle;

    // control state
    bool m_bMouseInside;
    bool m_bPaneDown;

    // information
    UINT m_VertexStride;
    UINT m_VertexOffset;
    CPUT_SIZE m_pPaneIdleSizeList[CPUT_NUM_IMAGES_IN_PANE];

    ID3D11Buffer* m_pIdlePaneVertexBuffers[CPUT_NUM_IMAGES_IN_PANE];

    // helper functions
    CPUTResult UnRegisterResizableInstanceQuads();
    CPUTResult RegisterQuad(ID3D11DeviceContext* pImmediateContext, int width, int height, ID3D11Buffer** ppVertexBuffer);
    CPUTResult Resize(ID3D11DeviceContext* pImmediateContext, int width, int height);
    void GetInsetTextCoordinate(int &x, int &y);
};

#endif // #ifndef __CPUTPANEDX11_H__
