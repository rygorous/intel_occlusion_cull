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
#ifndef __CPUTMATERIALOGLES_H__
#define __CPUTMATERIALOGLES_H__

#include <GLES2/gl2.h>

#include "CPUTMaterial.h"
#include "CPUTAssetLibraryOGLES.h"
#include "CPUTTextureOGLES.h"
#include "CPUTVertexShaderOGLES.h"
#include "CPUTPixelShaderOGLES.h"
//#include "CPUTGeometryShaderOGLES.h"
//#include "CPUTHullShaderOGLES.h"
//#include "CPUTDomainShaderOGLES.h"

class CPUTMaterialOGLES:public CPUTMaterial
{
protected:
    // TODO: move texture to base class.  All APIs have textures.
    GLuint *m_pBindTexture[CPUT_MATERIAL_MAX_TEXTURE_SLOTS];
    CPUTPixelShaderOGLES      *m_pPixelShader;
    CPUTVertexShaderOGLES     *m_pVertexShader;
    CPUTGeometryShaderOGLES   *m_pGeometryShader;
    CPUTHullShaderOGLES       *m_pHullShader;
    CPUTDomainShaderOGLES     *m_pDomainShader;

    UINT                      m_TextureParameterCount;
    UINT                      m_SamplerParameterCount;
    cString                  *m_pTextureParameterName;
    UINT                     *m_pTextureParameterBindPoint;
    cString                  *m_pSamplerParameterName;
    UINT                     *m_pSamplerParameterBindPoint;

    ~CPUTMaterialOGLES();  // Destructor is not public.  Must release instead of delete.

public:
    CPUTMaterialOGLES();

    CPUTResult    LoadMaterial(const cString& fileName);
    void          RebindTextures();
    CPUTVertexShaderOGLES *GetVertexShader() { return m_pVertexShader; }

    // Tells material to set the current render state to match the properties, textures,
    //  shaders, state, etc that this material represents
    virtual void SetRenderStates( CPUTRenderParameters &renderParams );
};

#endif // #ifndef __CPUTMATERIALOGLES_H__
