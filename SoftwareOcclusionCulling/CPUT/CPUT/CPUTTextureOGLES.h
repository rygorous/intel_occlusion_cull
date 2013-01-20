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
#ifndef _CPUTTEXTUREOGLES_H
#define _CPUTTEXTUREOGLES_H

#include "CPUTTexture.h"
#include "CPUT_OGLES.h"


class CPUTTextureOGLES : public CPUTTexture
{
private:
    // resource view pointer
    GLuint m_ShaderResourceID;

    // Destructor is not public.  Must release instead of delete.
    ~CPUTTextureOGLES() { glDeleteTextures(1, &m_ShaderResourceID); }

public:
    static const cString &GetGLFormatString(/*DXGI_FORMAT Format*/);
    static CPUTResult     GetSRGBEquivalent(/*DXGI_FORMAT inFormat, DXGI_FORMAT& sRGBFormat*/);
    static bool           DoesExistEquivalentSRGBFormat(/*DXGI_FORMAT inFormat*/);
    static CPUTTexture   *CreateTexture( const cString &name, const cString &absolutePathAndFilename, bool loadAsSRGB );
    static CPUTResult     CreateNativeTexture(/*const ID3D11Device* pD3dDevice,*/ const cString& fileName, GLuint* pTextureResourceID, bool forceLoadAsSRGB);

    CPUTTextureOGLES(): m_ShaderResourceID(0){}
    CPUTTextureOGLES(cString &name): m_ShaderResourceID(0), CPUTTexture(name){}

    GLuint GetShaderResourceID()
    {
        return m_ShaderResourceID;
    }

    void SetShaderResourceID(GLuint ShaderResourceID)
    {
        // gltodo: do we addref/release/delete the replaced ID's?
        // i.e. glDeleteTextures(1, &m_ShaderResourceID);
        m_ShaderResourceID = ShaderResourceID;
    }
};

#endif //_CPUTTEXTUREOGLES_H

