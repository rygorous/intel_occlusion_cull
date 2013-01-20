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
#include "CPUTPaneDX11.h"

#include "CPUTButtonDX11.h"
#include "CPUTSliderDX11.h"
#include "CPUTDropdownDX11.h"
#include "CPUTCheckboxDX11.h"
#include "CPUTGuiControllerDX11.h"

// static initializers
bool CPUTPaneDX11::m_StaticRegistered = false;
ID3DBlob* CPUTPaneDX11::m_pVertexShaderBlob = NULL;
ID3D11Buffer* CPUTPaneDX11::m_pVSConstantBuffer = NULL;

// list of the resources and sizes
ID3D11ShaderResourceView* CPUTPaneDX11::m_pIdleTextureResView[CPUT_NUM_IMAGES_IN_PANE] = {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
CPUT_SIZE CPUTPaneDX11::m_pPaneIdleImageSizeList[CPUT_NUM_IMAGES_IN_PANE] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

int CPUTPaneDX11::m_SmallestLeftSizeIdle=0;
int CPUTPaneDX11::m_SmallestRightSizeIdle=0;
int CPUTPaneDX11::m_SmallestTopSizeIdle=0;
int CPUTPaneDX11::m_SmallestBottomSizeIdle=0;

// Constructor
//--------------------------------------------------------------------------------
CPUTPaneDX11::CPUTPaneDX11(ID3D11DeviceContext* pImmediateContext, const cString ControlText, CPUTControlID id):CPUTPane(ControlText, id),
    m_bMouseInside(false),
    m_bPaneDown(false),
    m_VertexStride(0),
    m_VertexOffset(0)
{
    memset((void*) &m_pPaneIdleSizeList, 0, sizeof(CPUT_SIZE) * CPUT_NUM_IMAGES_IN_PANE);

    for(int i=0; i<CPUT_NUM_IMAGES_IN_PANE; i++)
    {
        m_pIdlePaneVertexBuffers[i] = NULL;
    }
    RegisterInstanceResources(pImmediateContext);

    // resize this control to fix that string with padding
    SetDimensions(pImmediateContext, 0, 0);

    // set the default position
    SetPosition( 0, 0 );
}

// Destructor
//--------------------------------------------------------------------------------
CPUTPaneDX11::~CPUTPaneDX11()
{
    UnRegisterInstanceResources();
}

//CPUTEventHandler

// Handle keyboard events
//--------------------------------------------------------------------------------
CPUTEventHandledCode CPUTPaneDX11::HandleKeyboardEvent(CPUTKey key)
{
    UNREFERENCED_PARAMETER(key);
    return CPUT_EVENT_UNHANDLED;
}

// Handle mouse events
//--------------------------------------------------------------------------------
CPUTEventHandledCode CPUTPaneDX11::HandleMouseEvent(int x, int y, int wheel, CPUTMouseState state)
{
    // walk the list of controls on the screen and see if they are to handle any of these events
    CPUTEventHandledCode eventHandled;
    for(UINT ii=0; ii<m_pControls.size(); ii++)
    {
        eventHandled = m_pControls[ii]->HandleMouseEvent(x,y,wheel,state);

        // if the control says it handled this event, do not pass it through to underlying controls
        // this is important for things like dropdowns that could dynamically overlap other controls
        if( CPUT_EVENT_HANDLED == eventHandled)
            return eventHandled;
    }
    return CPUT_EVENT_PASSTHROUGH;
}


// Returns true if the x,y coordinate is inside the pane's control region
//--------------------------------------------------------------------------------
bool CPUTPaneDX11::ContainsPoint(int x, int y)
{
    if( (x>=m_PaneDimensions.x) && (x<=m_PaneDimensions.x+m_PaneDimensions.width))
    {
        if( (y>=m_PaneDimensions.y) && (y<=m_PaneDimensions.y+m_PaneDimensions.height))
        {
            return true;
        }
    }
    return false;
}

// Returns the x,y coordinate inside the pane area that should be 'safe' to draw on
//--------------------------------------------------------------------------------
void CPUTPaneDX11::GetInsetTextCoordinate(int &x, int &y)
{
    // get text size
    CPUT_RECT PaneTextDimensions;
    PaneTextDimensions.width=0;
    PaneTextDimensions.height=0;

    // calculate a good 'center' point
    x =(int) ( m_PaneDimensions.x + m_PaneDimensions.width/2.0f - PaneTextDimensions.width/2.0f);
    y =(int) ( m_PaneDimensions.y + m_PaneDimensions.height/2.0f - PaneTextDimensions.height/2.0f);
}

//
//--------------------------------------------------------------------------------
void CPUTPaneDX11::SetDimensions(ID3D11DeviceContext* pImmediateContext,int width, int height)
{
    // Zero out the size and location
    InitialStateSet();

    // get the dimensions of the string in pixels
    CPUT_RECT rect = {0,0,0,0};

    width = max(rect.width, width);
    height = max(rect.height, height);

    // resize this control to fix that string with padding
    Resize(pImmediateContext, width, height);

    // move the text to a nice inset location inside the 'safe' area
    // of the pane image
}

// Set the upper-left screen coordinate location of this control
//--------------------------------------------------------------------------------
void CPUTPaneDX11::SetPosition(int x, int y)
{
    // move the pane graphics
    m_PaneDimensions.x = x;
    m_PaneDimensions.y = y;

    int currentHeight = m_PaneDimensions.y+m_SmallestTopSizeIdle;
    for(UINT ii=0; ii<m_pControls.size(); ++ii)
    {
        int width;
        int height;
        m_pControls[ii]->SetPosition(m_PaneDimensions.x+m_SmallestLeftSizeIdle, currentHeight);
        m_pControls[ii]->GetDimensions(width, height);
        currentHeight += height;
    }
}

void CPUTPaneDX11::AddControl(ID3D11DeviceContext* pImmediateContext, CPUTControl* pControl)
{
    m_pControls.push_back(pControl);

    int currentWidth;
    int currentHeight;
    GetDimensions(currentWidth, currentHeight);

    int width=0;
    int height=0;
    for(UINT ii=0; ii<m_pControls.size(); ii++)
    {
        m_pControls[ii]->GetDimensions(width, height);
        if((width+m_SmallestLeftSizeIdle*2) > currentWidth)
        {
            currentWidth = (width+m_SmallestLeftSizeIdle*2);
        }
    }
    currentHeight += height;
    pControl->SetControlCallback(m_pCallbackHandler);

    SetDimensions(pImmediateContext, currentWidth, currentHeight);
}

// draw this control.
// As an optimization, all the state setting code is done elsewhere
// in the CPUTGuiController class before calling this draw function
// It relies on a special GUI shader + state, so don't just call this for fun. :)
//--------------------------------------------------------------------------------
void CPUTPaneDX11::Draw(ID3D11DeviceContext* pImmediateContext, ID3D11Buffer* pVSConstantBuffer, ID3D11VertexShader* pVertexShader, ID3D11PixelShader* pPixelShader)
{
    if(!m_ControlVisible)
        return;

    float tx,ty;
    float moveRight=0.0f;
    float moveDown=0.0f;

    GUIVSConstantBuffer ConstantBufferMatrices;

    // set up matrices to position pane on screen
    float znear = 0.1f;
    float zfar = 100.0f;
    int windowWidth, windowHeight;
    XMMATRIX m;

    CPUTOSServices* pServices = CPUTOSServices::GetOSServices();
    pServices->GetClientDimensions( &windowWidth, &windowHeight);
    m = XMMatrixOrthographicOffCenterLH(0, (float)windowWidth, (float)windowHeight, 0, znear, zfar);
    ConstantBufferMatrices.projection = XMMatrixTranspose( m );

    ID3D11Buffer** ppVertexBufferList=NULL;
    ID3D11ShaderResourceView** ppTextureResourceList=NULL;
    CPUT_SIZE* pImageDimensionList=NULL;

    // set up/down pane state based on mouse state
    if(CPUT_CONTROL_INACTIVE != m_ControlState)
    {
        m_ControlState = CPUT_CONTROL_ACTIVE;
    }
    m_ControlState = CPUT_CONTROL_ACTIVE;

    switch(m_ControlState)
    {
    case CPUT_CONTROL_ACTIVE:
        ppVertexBufferList = m_pIdlePaneVertexBuffers;
        ppTextureResourceList = m_pIdleTextureResView;
        pImageDimensionList = m_pPaneIdleSizeList;
        break;

    default:
        // error! unknown state
        ASSERT(0,_L(""));
    }



    for(int i=0; i<CPUT_NUM_IMAGES_IN_PANE; i++)
    {
        pImmediateContext->VSSetShader( pVertexShader, NULL, 0 );
        pImmediateContext->IASetVertexBuffers( 0, 1, &ppVertexBufferList[i], &m_VertexStride, &m_VertexOffset );
        //pImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

        if((3==i) || (6==i))
        {
            moveDown=0;
            moveRight+=pImageDimensionList[i-1].width;
        }

        tx = m_PaneDimensions.x + moveRight;
        ty = m_PaneDimensions.y + moveDown;

        moveDown+=pImageDimensionList[i].height;

        // update translation matrix to draw at the right spot
        m = XMMatrixTranslation(tx, ty, 0);
        ConstantBufferMatrices.model = XMMatrixTranspose( m );
        pImmediateContext->UpdateSubresource( pVSConstantBuffer, 0, NULL, &ConstantBufferMatrices, 0, 0 );
        pImmediateContext->VSSetConstantBuffers( 0, 1, &pVSConstantBuffer );

        pImmediateContext->PSSetShader( pPixelShader, NULL, 0 );
        pImmediateContext->PSSetShaderResources( 0, 1, &ppTextureResourceList[i] );

        pImmediateContext->Draw(6,0);
    }

    for(UINT ii=0; ii<m_pControls.size(); ++ii)
    {
        CPUTControl* control = m_pControls[ii];
        switch(control->GetType())
        {
        case CPUT_BUTTON:
            ((CPUTButtonDX11*)control)->Draw(pImmediateContext, pVSConstantBuffer, pVertexShader, pPixelShader);
            break;

        case CPUT_CHECKBOX:
            ((CPUTCheckboxDX11*)control)->Draw(pImmediateContext, pVSConstantBuffer, pVertexShader, pPixelShader);
            break;

        case CPUT_DROPDOWN:
            ((CPUTDropdownDX11*)control)->Draw(pImmediateContext, pVSConstantBuffer, pVertexShader, pPixelShader);
            break;

        case CPUT_SLIDER:
            ((CPUTSliderDX11*)control)->Draw(pImmediateContext, pVSConstantBuffer, pVertexShader, pPixelShader);
            break;

        case CPUT_STATIC:
            ((CPUTStaticDX11*)control)->Draw(pImmediateContext, pVSConstantBuffer, pVertexShader, pPixelShader);
            break;
        case CPUT_PANE:
            ((CPUTPaneDX11*)control)->Draw(pImmediateContext, pVSConstantBuffer, pVertexShader, pPixelShader);
            break;
        default:
            // Error: Unknown control - Add handler for new control type if you want it rendered
            ASSERT(0,_L(""));
            return;
        }
    }
}

// Allocates/registers resources used by all panes
//--------------------------------------------------------------------------------
CPUTResult CPUTPaneDX11::RegisterStaticResources(ID3D11Device* pD3dDevice)
{
    if(m_StaticRegistered)
        return CPUT_SUCCESS;

    CPUTResult result;

    // set the static member variables
    m_StaticRegistered = false;
    m_pVertexShaderBlob = NULL;
    m_pVSConstantBuffer = NULL;

    CPUT_SIZE size;
    size.height=0; size.width=0;

    for(int ii=0; ii<CPUT_NUM_IMAGES_IN_PANE; ii++)
    {
        m_pIdleTextureResView[ii] = NULL;
        m_pPaneIdleImageSizeList[ii] = size;
    }

    m_SmallestLeftSizeIdle=0;
    m_SmallestRightSizeIdle=0;
    m_SmallestTopSizeIdle=0;
    m_SmallestBottomSizeIdle=0;



    // load all the idle+active pane images
    const int numberOfImages = CPUT_NUM_IMAGES_IN_PANE;
    cString pImageFilenames[numberOfImages] =
    {
        _L(".//controls//pane//pane-lt.png"),
        _L(".//controls//pane//pane-lm.png"),
        _L(".//controls//pane//pane-lb.png"),

        _L(".//controls//pane//pane-mt.png"),
        _L(".//controls//pane//pane-mm.png"),
        _L(".//controls//pane//pane-mb.png"),

        _L(".//controls//pane//pane-rt.png"),
        _L(".//controls//pane//pane-rm.png"),
        _L(".//controls//pane//pane-rb.png"),
    };

    // Load the textures
    for(int i=0; i<numberOfImages; i++)
    {
        cString fileName;
        cString resourceDir;
        CPUTGuiControllerDX11::GetController()->GetResourceDirectory(resourceDir);
        fileName = resourceDir + pImageFilenames[i];
        result = CPUTGuiTextureLoader::LoadTexture(pD3dDevice, fileName, &m_pIdleTextureResView[i]);
        ASSERT( CPUTSUCCESS(result), _L("Error loading texture.") );

        // get the size of the image from the resource view
        ID3D11Resource* pResource = NULL;
        m_pIdleTextureResView[i]->GetResource(&pResource);
        if(pResource)
        {
            ID3D11Texture2D* pTexture = (ID3D11Texture2D*)pResource;
            D3D11_TEXTURE2D_DESC TextureDesc;
            pTexture->GetDesc(&TextureDesc);
            m_pPaneIdleImageSizeList[i].height = TextureDesc.Height;
            m_pPaneIdleImageSizeList[i].width = TextureDesc.Width;

            // calculate the 'border' sizes
            if(i<3)
            {
                if(m_SmallestLeftSizeIdle < m_pPaneIdleImageSizeList[i].width)
                {
                    m_SmallestLeftSizeIdle = m_pPaneIdleImageSizeList[i].width;
                }
            }

            if(i>5)
            {
                if(m_SmallestRightSizeIdle < m_pPaneIdleImageSizeList[i].width)
                {
                    m_SmallestRightSizeIdle = m_pPaneIdleImageSizeList[i].width;
                }
            }

            if( (0==i) || (3==i) || (6==i) )
            {
                if(m_SmallestTopSizeIdle < m_pPaneIdleImageSizeList[i].height)
                {
                    m_SmallestTopSizeIdle = m_pPaneIdleImageSizeList[i].height;
                }
            }
            if( (2==i) || (5==i) || (8==i) )
            {
                if(m_SmallestBottomSizeIdle < m_pPaneIdleImageSizeList[i].height)
                {
                    m_SmallestBottomSizeIdle = m_pPaneIdleImageSizeList[i].height;
                }
            }
            pResource->Release(); // release the resource handle
        }

    }

    m_StaticRegistered = true;

    return CPUT_SUCCESS;
}




// deletes all static resources alloated for panes
//--------------------------------------------------------------------------------
CPUTResult CPUTPaneDX11::UnRegisterStaticResources()
{
    //release all the static textures
    for(int ii=0; ii<CPUT_NUM_IMAGES_IN_PANE; ii++)
    {
        if(m_pIdleTextureResView[ii])
        {
            SAFE_RELEASE( m_pIdleTextureResView[ii] );
        }
    }

    return CPUT_SUCCESS;
}

// allocates all instance resources alloated for this pane
//--------------------------------------------------------------------------------
CPUTResult CPUTPaneDX11::RegisterInstanceResources(ID3D11DeviceContext* pImmediateContext)
{
    CPUTResult result;

    // register all 9 of the idle image quads
    for(int i=0; i<CPUT_NUM_IMAGES_IN_PANE; i++)
    {
        result = RegisterQuad(pImmediateContext, m_pPaneIdleImageSizeList[i].width, m_pPaneIdleImageSizeList[i].height, &m_pIdlePaneVertexBuffers[i]);
        if(CPUTFAILED(result))
            return result;
    }

    // store all the default pane component quad sizes in instance variables
    for(int i=0; i<CPUT_NUM_IMAGES_IN_PANE; i++)
    {
        m_pPaneIdleSizeList[i].height = m_pPaneIdleImageSizeList[i].height;
        m_pPaneIdleSizeList[i].width = m_pPaneIdleImageSizeList[i].width;
    }

    return CPUT_SUCCESS;
}

// deletes all instance resources alloated for this pane
//--------------------------------------------------------------------------------
CPUTResult CPUTPaneDX11::UnRegisterInstanceResources()
{
    CPUTResult result = CPUT_SUCCESS;

    // delete the vertex buffer objects
    for(int ii=0; ii<CPUT_NUM_IMAGES_IN_PANE; ii++)
    {
        SAFE_RELEASE(m_pIdlePaneVertexBuffers[ii]);
    }

    for(UINT ii=0; ii<m_pControls.size(); ++ii)
    {
        if(m_pControls[ii])
            delete m_pControls[ii];
    }
    m_pControls.clear();

    return result;
}

// delete the resizable pane quads, keeping the corner ones since they don't resize
//--------------------------------------------------------------------------------
CPUTResult CPUTPaneDX11::UnRegisterResizableInstanceQuads()
{
    CPUTResult result = CPUT_SUCCESS;
    UINT jj=0;

    // delete the vertex buffer objects
    for(int i=1; i<CPUT_NUM_IMAGES_IN_PANE; i++)
    {
        if( (2!=i) && (6!=i) && (8!=i))
        {
            if(NULL!=m_pIdlePaneVertexBuffers[i])
            {
                jj = m_pIdlePaneVertexBuffers[i]->Release();
                m_pIdlePaneVertexBuffers[i] = NULL;
            }
        }
    }

    return result;
}



// Resize the pane - this is a forcing function - really does force the resize
// DOES NOT move the inside text
//--------------------------------------------------------------------------------
CPUTResult CPUTPaneDX11::Resize(ID3D11DeviceContext* pImmediateContext, int width, int height)
{
    CPUTResult result;

    // verify that the new dimensions fit the minimal 'safe' dimensions needed to draw the pane
    // or ugly clipping will occur
    int safeWidth, safeHeight;

    switch(m_ControlState)
    {
    case CPUT_CONTROL_ACTIVE:
        safeWidth = m_PaneDimensions.x + m_SmallestLeftSizeIdle + m_SmallestRightSizeIdle + 1;
        safeHeight = m_PaneDimensions.y + m_SmallestTopSizeIdle + m_SmallestBottomSizeIdle + 1;
        break;
    default:
        ASSERT(0,_L("")); // todo: error! unknown state - using idle dimensions as a default
        safeWidth = m_PaneDimensions.x + m_SmallestLeftSizeIdle + m_SmallestRightSizeIdle + 1;
        safeHeight = m_PaneDimensions.y + m_SmallestTopSizeIdle + m_SmallestBottomSizeIdle + 1;
    }

    // if the user's dimensions are smaller than the smallest 'safe' dimensions of the pane,
    // use the safe ones instead.
    if(safeWidth > width)
        width = safeWidth;
    if(safeHeight > height)
        height = safeHeight;

    // add some padding for nicety
    width += CPUT_PANE_TEXT_BORDER_PADDING_X;
    height += CPUT_PANE_TEXT_BORDER_PADDING_Y;

    // test for rebuilding
    if( (m_PaneDimensions.width != width) || (m_PaneDimensions.height != height) )
    {
        // store the new dimensions
        m_PaneDimensions.width = width;
        m_PaneDimensions.height = height;

        // calculate the pieces we'll need to rebuild
        int middleWidth = width - m_SmallestLeftSizeIdle - m_SmallestRightSizeIdle;
        int middleHeight = height - m_SmallestTopSizeIdle - m_SmallestBottomSizeIdle;

        // delete the old pane quads for the middle sections
        result = UnRegisterResizableInstanceQuads();

        // create a new quads with the correct size
        // Idle pane quads
        result = RegisterQuad(pImmediateContext, m_pPaneIdleSizeList[1].width, middleHeight, &m_pIdlePaneVertexBuffers[1]);
        m_pPaneIdleSizeList[1].height = middleHeight;
        if(CPUTFAILED(result))
            return result;

        result = RegisterQuad(pImmediateContext, middleWidth, m_pPaneIdleSizeList[3].height, &m_pIdlePaneVertexBuffers[3]);
        m_pPaneIdleSizeList[3].width = middleWidth;
        if(CPUTFAILED(result))
            return result;
        result = RegisterQuad(pImmediateContext, middleWidth, middleHeight, &m_pIdlePaneVertexBuffers[4]);
        m_pPaneIdleSizeList[4].width = middleWidth;
        m_pPaneIdleSizeList[4].height = middleHeight;
        if(CPUTFAILED(result))
            return result;
        result = RegisterQuad(pImmediateContext, middleWidth, m_pPaneIdleSizeList[5].height, &m_pIdlePaneVertexBuffers[5]);
        m_pPaneIdleSizeList[5].width = middleWidth;
        if(CPUTFAILED(result))
            return result;

        result = RegisterQuad(pImmediateContext, m_pPaneIdleSizeList[7].width, middleHeight, &m_pIdlePaneVertexBuffers[7]);
        m_pPaneIdleSizeList[7].height = middleHeight;
        if(CPUTFAILED(result))
            return result;

    }

    return CPUT_SUCCESS;
}


// Register pane quads for drawing on
//--------------------------------------------------------------------------------
CPUTResult CPUTPaneDX11::RegisterQuad(ID3D11DeviceContext* pImmediateContext, int w, int h, ID3D11Buffer** ppVertexBuffer )
{
    HRESULT hr;

    // get the device
    ID3D11Device* pD3dDevice = NULL;
    pImmediateContext->GetDevice(&pD3dDevice);
    float width = (float) w;
    float height = (float) h;

    struct SimpleVertex
    {
        XMFLOAT3 Pos;
        XMFLOAT2 UV;
    };

    SimpleVertex vertices[] =
    {   // Position                       UV coord
        {XMFLOAT3( 0.0f, 0.0f, 1.1f),   XMFLOAT2(0.0f, 0.0f) }, //0
        {XMFLOAT3( width, 0.0f, 1.1f),  XMFLOAT2(1.0f, 0.0f) }, //2
        {XMFLOAT3( 0.0f, height, 1.1f), XMFLOAT2(0.0f, 1.0f) }, //1


        {XMFLOAT3( width, 0.0f, 1.1f),  XMFLOAT2(1.0f, 0.0f) },
        {XMFLOAT3( width, height, 1.1f),XMFLOAT2(1.0f, 1.0f) },
        {XMFLOAT3( 0.0f, height, 1.1f), XMFLOAT2(0.0f, 1.0f) },
    };

    // create the vertex buffer
    D3D11_BUFFER_DESC bd;
    ZeroMemory( &bd, sizeof(bd) );
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof( SimpleVertex ) * 6;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory( &InitData, sizeof(InitData) );
    InitData.pSysMem = vertices;
    hr = pD3dDevice->CreateBuffer( &bd, &InitData, ppVertexBuffer );
    ASSERT( SUCCEEDED(hr), _L("Error creating vertex buffer.") );

    // release the device pointer
    pD3dDevice->Release();

    // Set contents of the vertex buffer
    m_VertexStride = sizeof(SimpleVertex);
    m_VertexOffset = 0;

    return CPUT_SUCCESS;
}
