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
#ifndef _CPUTRENDERSTATEBLOCKOGLES_H
#define _CPUTRENDERSTATEBLOCKOGLES_H

#include "CPUTRenderStateBlock.h"
#include "CPUTConfigBlock.h"


struct CPUTRenderStateMapEntry;
class CPUTRenderParameters;

//-----------------------------------------------------------------------------
// TODO: Move to own file
class CPUTRenderStatesOGLES
{
public:
    //D3D11_BLEND_DESC         BlendDesc;
    //D3D11_DEPTH_STENCIL_DESC DepthStencilDesc;
    //D3D11_RASTERIZER_DESC    RasterizerDesc;
    //D3D11_SAMPLER_DESC       SamplerDesc[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
    //float                    BlendFactor[4];
    //UINT                     SampleMask;

    CPUTRenderStatesOGLES() { SetDefaults(); }
    ~CPUTRenderStatesOGLES() {} // Destructor is not public.  Must release instead of delete.
    void SetDefaults();
};

//-----------------------------------------------------------------------------
class CPUTRenderStateBlockOGLES:public CPUTRenderStateBlock
{
protected:
    //// The state descriptor describes all of the states.
    //// We read it in when creating assets.  We keep it around in case we need to adjust and recreate.
    //CPUTRenderStatesDX11        m_StateDesc;

    //// Each of the native state objects.
    //ID3D11BlendState           *m_pBlendState;
    //ID3D11DepthStencilState    *m_pDepthStencilState;
    //ID3D11RasterizerState      *m_pRasterizerState;
    //ID3D11SamplerState         *m_pSamplerState[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
    UINT                        m_NumSamplers;

     // Destructor is not public.  Must release instead of delete.
    ~CPUTRenderStateBlockOGLES()
    {
        //SAFE_RELEASE( m_pBlendState );
        //SAFE_RELEASE( m_pDepthStencilState );
        //SAFE_RELEASE( m_pRasterizerState );
        //for( UINT ii=0; ii<D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT; ii++ )
        //{
        //    SAFE_RELEASE( m_pSamplerState[ii] );
        //}
    }

public:
    // constructor
    CPUTRenderStateBlockOGLES():
        //m_pBlendState(NULL),
        //m_pDepthStencilState(NULL),
        //m_pRasterizerState(NULL),
        m_NumSamplers(16) // gltodo: whoa!  Get GL|ES equivalent D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT) // Default to all.  Set to actual count when loading
    {
        //for( UINT ii=0; ii<D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT; ii++ )
        //{
        //    m_pSamplerState[ii] = NULL;
        //}
    }
    CPUTResult ReadProperties(
        CPUTConfigFile                &file,
        const cString                 &blockName,
        const CPUTRenderStateMapEntry *pMap,
        void                          *pDest
    );
    void               ReadValue( CPUTConfigEntry *pValue, const CPUTRenderStateMapEntry *pRenderStateList, void *pDest );
    virtual CPUTResult LoadRenderStateBlock(const cString& fileName);
    virtual void       CreateNativeResources();
    void               SetRenderStates(CPUTRenderParameters &renderParams);
};

//-----------------------------------------------------------------------------
enum CPUT_PARAM_TYPE{
    ePARAM_TYPE_TYPELESS,
    ePARAM_TYPE_INT,
    ePARAM_TYPE_UINT,
    ePARAM_TYPE_FLOAT,
    ePARAM_TYPE_BOOL,
    ePARAM_TYPE_SHORT,
    ePARAM_TYPE_CHAR,
    ePARAM_TYPE_UCHAR,
    ePARAM_TYPE_STRING, // Does string make sense?  Could copy it.
    ePARAM_TYPE_D3D11_BLEND,
    ePARAM_TYPE_D3D11_BLEND_OP,
    ePARAM_TYPE_DEPTH_WRITE_MASK,
    ePARAM_TYPE_D3D11_COMPARISON_FUNC,
    ePARAM_TYPE_D3D11_STENCIL_OP,
    ePARAM_TYPE_D3D11_FILL_MODE,
    ePARAM_TYPE_D3D11_CULL_MODE,
    ePARAM_TYPE_D3D11_FILTER,
    ePARAM_TYPE_D3D11_TEXTURE_ADDRESS_MODE
};

//-----------------------------------------------------------------------------
struct CPUTRenderStateMapEntry
{
    cString         name;
    CPUT_PARAM_TYPE type;
    UINT            offset;
};

//-----------------------------------------------------------------------------
class StringToIntMapEntry
{
public:
    cString mName;
    int     mValue;

    bool FindMapEntryByName( int *pValue, const cString name ) const{
        StringToIntMapEntry const *pstart = this;
        for( StringToIntMapEntry const *pEntry = pstart; pEntry->mName.length() > 0; pEntry++ ){
            if( 0 == _wcsicmp( pEntry->mName.data(), name.data() ) ){
                *pValue = pEntry->mValue;
                return true;
            }
        }
        return false;
    }
};

extern CPUTRenderStateBlockOGLES *g_pDefaultRenderStateBlock;

#endif // _CPUTRENDERSTATEBLOCKOGLES_H
