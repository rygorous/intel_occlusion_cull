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
#include "CPUTVertexLayoutCacheDX11.h"
#include "CPUTVertexShaderDX11.h"

extern const cString *gpDXGIFormatNames;

CPUTVertexLayoutCacheDX11* CPUTVertexLayoutCacheDX11::m_pVertexLayoutCache = NULL;

//-----------------------------------------------------------------------------
void CPUTVertexLayoutCacheDX11::ClearLayoutCache()
{
	// iterate over the entire map - and release each layout object
    std::map<cString, ID3D11InputLayout*>::iterator mapIterator;

    for(mapIterator = m_LayoutList.begin(); mapIterator != m_LayoutList.end(); mapIterator++)
    {
        mapIterator->first;
        mapIterator->second->Release();  // release the ID3D11InputLayout*
    }

    // clear the map list
    m_LayoutList.clear();
}

// singleton retriever
//-----------------------------------------------------------------------------
CPUTVertexLayoutCacheDX11* CPUTVertexLayoutCacheDX11::GetVertexLayoutCache()
{
    if(NULL == m_pVertexLayoutCache)
    {
        m_pVertexLayoutCache = new CPUTVertexLayoutCacheDX11();
    }
    return m_pVertexLayoutCache;
}

// singleton destroy routine
//-----------------------------------------------------------------------------
CPUTResult CPUTVertexLayoutCacheDX11::DeleteVertexLayoutCache()
{
    if(m_pVertexLayoutCache)
    {
        delete m_pVertexLayoutCache;
        m_pVertexLayoutCache = NULL;
    }
    return CPUT_SUCCESS;
}

// find existing, or create new, ID3D11InputLayout layout
//-----------------------------------------------------------------------------
CPUTResult CPUTVertexLayoutCacheDX11::GetLayout(
    ID3D11Device* pDevice,
    D3D11_INPUT_ELEMENT_DESC* pDXLayout,
    CPUTVertexShaderDX11 *pVertexShader,
    ID3D11InputLayout** ppVertexLayout
){
    // Generate the vertex layout pattern portion of the key
    cString layoutKey = GenerateLayoutKey(pDXLayout);

    // Append the vertex shader pointer to the key for layout<->vertex shader relationship
    cString address;
    address = ptoc(pVertexShader);
    layoutKey += address;

    // Do we already have one like this?
    if( m_LayoutList[layoutKey] )
    {
        *ppVertexLayout = m_LayoutList[layoutKey];
        return CPUT_SUCCESS;
    }
    // Not found, create a new ID3D11InputLayout object

    // How many elements are in the input layout?
    int numInputLayoutElements=0;
    while(NULL != pDXLayout[numInputLayoutElements].SemanticName)
    {
        numInputLayoutElements++;
    }
    // Create the input layout
    HRESULT hr;
    ID3DBlob *pBlob = pVertexShader->GetBlob();
    hr = pDevice->CreateInputLayout( pDXLayout, numInputLayoutElements, pBlob->GetBufferPointer(), pBlob->GetBufferSize(), ppVertexLayout );
    ASSERT( SUCCEEDED(hr), _L("Error creating input layout.") );

    // Store this layout object in our map
    m_LayoutList[layoutKey] = *ppVertexLayout;

    // Addref for storing it in our map as well as returning it (count should be = 2 at this point)
    (*ppVertexLayout)->AddRef();

    return CPUT_SUCCESS;
}

// Generate a string version of the vertex-buffer's layout.  Allows us to search, compare, etc...
//-----------------------------------------------------------------------------
cString CPUTVertexLayoutCacheDX11::GenerateLayoutKey(D3D11_INPUT_ELEMENT_DESC* pDXLayout)
{
    // TODO:  Duh!  We can simply memcmp the DX layouts == use the layout input description directly as the key.
    //        We just need to know how many elements, or NULL terminate it.
    //        Uses less memory, faster, etc...
    //        Duh!

    // TODO: Use shorter names, etc...
    ASSERT( (pDXLayout[0].Format>=0) && (pDXLayout[0].Format<=DXGI_FORMAT_BC7_UNORM_SRGB), _L("Invalid DXGI Format.") );
    // Start first layout entry and no comma.
    cString layoutKey = s2ws(pDXLayout[0].SemanticName) + _L(":") + gpDXGIFormatNames[pDXLayout[0].Format];
    for( int index=1; NULL != pDXLayout[index].SemanticName; index++ )
    {
        ASSERT( (pDXLayout[index].Format>=0) && (pDXLayout[index].Format<=DXGI_FORMAT_BC7_UNORM_SRGB), _L("Invalid DXGI Format.") );
        // Add a comma and the next layout entry
        layoutKey = layoutKey + _L(",") + s2ws(pDXLayout[index].SemanticName) + _L(":") + gpDXGIFormatNames[pDXLayout[index].Format];
    }
    return layoutKey;
}
