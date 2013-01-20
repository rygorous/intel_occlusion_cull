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
#ifndef _CPUTVERTEXSHADEROGLES_H
#define _CPUTVERTEXSHADEROGLES_H

#include "CPUT.h"
#include "CPUTRefCount.h"

#include <GLES2/gl2.h>

class CPUTVertexShaderOGLES : public CPUTRefCount
{
protected:
    GLuint      m_VertexShader;
    //ID3D11VertexShader *m_pVertexShader;
    //ID3DBlob           *m_pBlob;

    // Destructor is not public.  Must release instead of delete.
    
    // gltodo: what do we do about glPrograms linking against this shader?  We need to invalidate them as well!
    ~CPUTVertexShaderOGLES(){ glDeleteShader(m_VertexShader); }

public:
    static CPUTVertexShaderOGLES *CreateVertexShader( cString &name, cString absolutePathAndFilename );

    CPUTVertexShaderOGLES() : m_VertexShader(0) {}
    CPUTVertexShaderOGLES(/*ID3D11VertexShader *pD3D11VertexShader, ID3DBlob *pBlob*/ GLuint VertexShader) : m_VertexShader(VertexShader) {}
    GLuint GetNativeVertexShader() { return m_VertexShader; }
};

#endif //_CPUTVERTEXSHADEROGLES_H
