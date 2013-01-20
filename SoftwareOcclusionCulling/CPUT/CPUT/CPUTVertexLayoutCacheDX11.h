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
#ifndef __CPUTVERTEXLAYOUTCACHERDX11_H__
#define __CPUTVERTEXLAYOUTCACHERDX11_H__

#include "CPUTVertexLayoutCache.h"
#include "CPUTOSServicesWin.h"
#include "CPUTVertexShaderDX11.h"
#include <D3D11.h> // D3D11_INPUT_ELEMENT_DESC
#include <map>

class CPUTVertexLayoutCacheDX11:public CPUTVertexLayoutCache
{
public:
    ~CPUTVertexLayoutCacheDX11()
    {
        ClearLayoutCache();
    }
    static CPUTVertexLayoutCacheDX11 *GetVertexLayoutCache();
    static CPUTResult DeleteVertexLayoutCache();
    CPUTResult GetLayout(ID3D11Device *pDevice, D3D11_INPUT_ELEMENT_DESC *pDXLayout, CPUTVertexShaderDX11 *pVertexShader, ID3D11InputLayout **ppVertexLayout);
	void ClearLayoutCache();

private:
    // singleton
    CPUTVertexLayoutCacheDX11() { m_LayoutList.clear(); }

    // convert the D3D11_INPUT_ELEMENT_DESC to string key
    cString GenerateLayoutKey(D3D11_INPUT_ELEMENT_DESC *pDXLayout);

    CPUTResult VerifyLayoutCompatibility(D3D11_INPUT_ELEMENT_DESC *pDXLayout, ID3DBlob *pVertexShaderBlob);

    static CPUTVertexLayoutCacheDX11 *m_pVertexLayoutCache;
    std::map<cString, ID3D11InputLayout*> m_LayoutList;
};

#endif //#define __CPUTVERTEXLAYOUTCACHERDX11_H__