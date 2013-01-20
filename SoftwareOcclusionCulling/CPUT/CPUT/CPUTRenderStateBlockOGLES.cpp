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
#include "CPUTRenderStateBlockOGLES.h"
//#include "CPUTRenderStateMapsOGLES.h"

//-----------------------------------------------------------------------------
CPUTResult CPUTRenderStateBlockOGLES::LoadRenderStateBlock(const cString& fileName)
{
    // TODO: If already loaded, then Release() all the old members
    /*
    // use the fileName for now, maybe we'll add names later?
    m_MaterialName = fileName;

    // Open/parse the file
    CPUTConfigFile file;
    CPUTResult result = file.LoadFile(fileName);
    ASSERT( !FAILED(result), _L("Failed loading file: '") + fileName + _L("'.") );

    // Note: We ignore "not found error" results for ReadProperties() calls.
    // These blocks are optional.
    UINT ii;
    for( ii=0; ii<8; ii++ )
    {
        wchar_t pBlockName[64];
        wsprintf( pBlockName, _L("RenderTargetBlendStateDX11_%d"), ii+1 );
        ReadProperties( file, cString(pBlockName), pRenderTargetBlendDescMap, &m_StateDesc.BlendDesc.RenderTarget[ii] );
    }
    ReadProperties( file, _L("BlendStateDX11"),        pBlendDescMap,        &m_StateDesc.BlendDesc );
    ReadProperties( file, _L("DepthStencilStateDX11"), pDepthStencilDescMap, &m_StateDesc.DepthStencilDesc);
    ReadProperties( file, _L("RasterizerStateDX11"),   pRasterizerDescMap,   &m_StateDesc.RasterizerDesc);

    m_NumSamplers = 0;
    for( ii=0; ii<D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT; ii++ )
    {
        // TODO: Use sampler names from .fx file.  Already did this for texture names.
        // The challenge is that the renderstate file is independent from the material (and the shaders).
        // Another feature is that the artists don't name the samplers (in the CPUTSL source).  Though, arbitrary .fx files can.
        // TODO: Add sampler-state properties to CPUTSL source (e.g., filter modes).  Then, have ShaderGenerator output a .rs file.
        wchar_t pBlockName[64];
        wsprintf( pBlockName, _L("SamplerDX11_%d"), ii+1 );
        CPUTResult result = ReadProperties( file, cString(pBlockName), pSamplerDescMap, &m_StateDesc.SamplerDesc[ii] );
        if( CPUT_SUCCESS != result )
        {
            break; // Reached last sampler spec
        }
        ++m_NumSamplers;
    }
    CreateNativeResources();
    */
    return CPUT_SUCCESS;
} // CPUTRenderStateBlockOGLES::LoadRenderStateBlock()

//-----------------------------------------------------------------------------
void CPUTRenderStateBlockOGLES::CreateNativeResources()
{
    //// Now, create the DX render state items
    //ID3D11Device *pDevice = CPUT_DX11::GetDevice();
    //HRESULT hr;
    //hr = pDevice->CreateBlendState( &m_StateDesc.BlendDesc, &m_pBlendState );
    //ASSERT( SUCCEEDED(hr), _L("Failed to create blend state.") );

    //hr = pDevice->CreateDepthStencilState( &m_StateDesc.DepthStencilDesc, &m_pDepthStencilState );
    //ASSERT( SUCCEEDED(hr), _L("Failed to create depth stencil state.") );

    //hr = pDevice->CreateRasterizerState( &m_StateDesc.RasterizerDesc, &m_pRasterizerState );
    //ASSERT( SUCCEEDED(hr), _L("Failed to create rasterizer state.") );

    //// TODO: how to map samplers to shaders?
    //// Each type can have different samplers assigned (VS, PS, GS, etc.)
    //// How does DX treat them?  16 unified?  or 16 each?
    //// For now, just read 16 samplers, and set to all stages

    //for( UINT ii=0; ii<m_NumSamplers; ii++ )
    //{
    //    hr = pDevice->CreateSamplerState( &m_StateDesc.SamplerDesc[ii], &m_pSamplerState[ii] );
    //    ASSERT( SUCCEEDED(hr), _L("Failed to create sampler state.") );
    //}
} // CPUTRenderStateBlockOGLES::CreateDXResources()

//-----------------------------------------------------------------------------
void CPUTRenderStateBlockOGLES::SetRenderStates( CPUTRenderParameters &renderParams )
{
    /*
    ID3D11DeviceContext *pContext = ((CPUTRenderParametersDX*)&renderParams)->m_pContext;

    pContext->OMSetBlendState( m_pBlendState, m_StateDesc.BlendFactor, m_StateDesc.SampleMask );
    pContext->OMSetDepthStencilState( m_pDepthStencilState, 0 ); // TODO: read stecil ref from config file
    pContext->RSSetState( m_pRasterizerState );
    pContext->PSSetSamplers( 0, m_NumSamplers, m_pSamplerState );
    pContext->VSSetSamplers( 0, m_NumSamplers, m_pSamplerState );
    pContext->GSSetSamplers( 0, m_NumSamplers, m_pSamplerState );
    */
} // CPUTRenderStateBlockOGLES::SetRenderState()