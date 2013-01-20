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

#include "CPUTTextureOGLES.h"



//-----------------------------------------------------------------------------
CPUTTexture *CPUTTextureOGLES::CreateTexture( const cString &name, const cString &absolutePathAndFilename, bool loadAsSRGB )
{
    /*
    // TODO:  Delegate to derived class.  We don't currently have CPUTTextureDX11
    ID3D11ShaderResourceView* pShaderResourceView = NULL;
    ID3D11Device *pD3dDevice= CPUT_DX11::GetDevice();
    CPUTResult result = CreateNativeTexture( pD3dDevice, absolutePathAndFilename, &pShaderResourceView, loadAsSRGB );
    ASSERT( CPUTSUCCESS(result), _L("Error loading texture: '")+absolutePathAndFilename );

    CPUTTextureDX11 *pNewTexture = new CPUTTextureDX11();
    pNewTexture->m_Name = name;
    pNewTexture->SetShaderResourceView( pShaderResourceView );
    pShaderResourceView->Release();

    CPUTAssetLibrary::GetAssetLibrary()->AddTexture( absolutePathAndFilename, pNewTexture);
    
    return pNewTexture;
    */
    return NULL;
}