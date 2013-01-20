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
#ifndef __CPUTASSETLIBRARYOGLES_H__
#define __CPUTASSETLIBRARYOGLES_H__

#include "CPUTAssetLibrary.h"
#include "CPUTConfigBlock.h"

#include <GLES2/gl2.h>

class CPUTAssetSet;
class CPUTMaterial;
class CPUTModel;
class CPUTNullNode;
class CPUTCamera;
class CPUTRenderStateBlock;
class CPUTLight;
class CPUTTexture;
class CPUTAssetSetOGLES;
class CPUTVertexShaderOGLES;
class CPUTPixelShaderOGLES;
class CPUTGeometryShaderOGLES;
class CPUTHullShaderOGLES;
class CPUTDomainShaderOGLES;

/*
//-----------------------------------------------------------------------------
struct CPUTSRGBLoadFlags
{
    bool bInterpretInputasSRGB;
    bool bWritetoSRGBOutput;
};
*/
//-----------------------------------------------------------------------------
class CPUTAssetLibraryOGLES:public CPUTAssetLibrary
{
protected:
    static CPUTAssetListEntry  *m_pPixelShaderList;
    static CPUTAssetListEntry  *m_pVertexShaderList;
    static CPUTAssetListEntry  *m_pGeometryShaderList;
    static CPUTAssetListEntry  *m_pHullShaderList;
    static CPUTAssetListEntry  *m_pDomainShaderList;

public:
    CPUTAssetLibraryOGLES(){}
    virtual ~CPUTAssetLibraryOGLES()
    {
        ReleaseAllLibraryLists();
    }

    virtual void ReleaseAllLibraryLists();
    void ReleaseIunknownList( CPUTAssetListEntry* pList );

    void AddPixelShader(    const cString& name, CPUTPixelShaderOGLES    *pShader) { AddAsset( name, pShader, &m_pPixelShaderList ); }
    void AddVertexShader(   const cString& name, CPUTVertexShaderOGLES   *pShader) { AddAsset( name, pShader, &m_pVertexShaderList ); }
    void AddGeometryShader( const cString& name, CPUTGeometryShaderOGLES *pShader) { AddAsset( name, pShader, &m_pGeometryShaderList ); }
    void AddHullShader(     const cString& name, CPUTHullShaderOGLES     *pShader) { AddAsset( name, pShader, &m_pHullShaderList ); }
    void AddDomainShader(   const cString& name, CPUTDomainShaderOGLES   *pShader) { AddAsset( name, pShader, &m_pDomainShaderList ); }
    
    CPUTPixelShaderOGLES    *FindPixelShader(    const cString &name ) { return    (CPUTPixelShaderOGLES*)FindAsset( name, m_pPixelShaderList ); }
    CPUTVertexShaderOGLES   *FindVertexShader(   const cString &name ) { return   (CPUTVertexShaderOGLES*)FindAsset( name, m_pVertexShaderList ); }
    CPUTGeometryShaderOGLES *FindGeometryShader( const cString &name ) { return (CPUTGeometryShaderOGLES*)FindAsset( name, m_pGeometryShaderList ); }
    CPUTHullShaderOGLES     *FindHullShader( const cString &name )     { return     (CPUTHullShaderOGLES*)FindAsset( name, m_pHullShaderList ); }
    CPUTDomainShaderOGLES   *FindDomainShader( const cString &name )   { return   (CPUTDomainShaderOGLES*)FindAsset( name, m_pDomainShaderList ); }

    // shaders - vertex, pixel
    CPUTResult GetPixelShader(     const cString& name, const cString &shaderMain, const cString &shaderProfile, CPUTPixelShaderOGLES**    ppShader, bool nameIsFullPathAndFilename=false);
    CPUTResult GetVertexShader(    const cString& name, const cString &shaderMain, const cString &shaderProfile, CPUTVertexShaderOGLES**   ppShader, bool nameIsFullPathAndFilename=false);
    CPUTResult GetGeometryShader(  const cString& name, const cString &shaderMain, const cString &shaderProfile, CPUTGeometryShaderOGLES** ppShader, bool nameIsFullPathAndFilename=false);
    CPUTResult GetHullShader(      const cString& name, const cString &shaderMain, const cString &shaderProfile, CPUTHullShaderOGLES**     ppShader, bool nameIsFullPathAndFilename=false);
    CPUTResult GetDomainShader(    const cString& name, const cString &shaderMain, const cString &shaderProfile, CPUTDomainShaderOGLES**   ppShader, bool nameIsFullPathAndFilename=false);
 
protected:
    // helper functions
    CPUTResult CompileShaderFromFile(const cString &fileName, const cString &shaderMain, const cString &shaderProfile);
    CPUTResult LoadShaderFileString(const cString Filename, void** ppData);
    GLuint CompileShader( GLenum type, void *pShaderString );
};


#endif // #ifndef __CPUTASSETLIBRARYOGLES_H__
