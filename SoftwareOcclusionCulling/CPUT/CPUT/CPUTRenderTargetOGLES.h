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
#ifndef _CPUTRENDERTARGETOGLES_H
#define _CPUTRENDERTARGETOGLES_H

#include "CPUT.h"

// OpenGL|ES types
#include <GLES2/gl2.h>

// gltodo:  this file is FULL of DX-isms, will need extensive rework
class CPUTMaterial;
class CPUTRenderParameters;
class CPUTTexture;
class CPUTRenderTargetDepth;
class CPUTSprite;

class CPUTRenderTargetColor
{
public:
    static GLuint                  GetActiveRenderTargetView() { return sActiveRenderTargetView; }
    static void                    SetActiveRenderTargetView(GLuint View) { sActiveRenderTargetView = View; }
    static void                    SetActiveWidthHeight( UINT width, UINT height ) {sCurrentWidth = width; sCurrentHeight=height; }
    static UINT                    GetActiveWidth()  {return sCurrentWidth; }
    static UINT                    GetActiveHeight() {return sCurrentHeight; }

protected:
    static UINT                    sCurrentWidth;
    static UINT                    sCurrentHeight;
    static GLuint                   sActiveRenderTargetView;

    cString                        mName;
    UINT                           mWidth;
    UINT                           mHeight;
    bool                           mRenderTargetSet;
                                  
    GLuint                         mColorFormat;
    //DXGI_FORMAT                    mColorFormat;
    UINT                           mMultiSampleCount;
                                  
    CPUTTexture                   *mpColorTexture;
    GLuint                         mColorTextureOGLES;    
    GLuint                         mColorResourceView;
    GLuint                         mColorRenderTargetView;
    //ID3D11Texture2D               *mpColorTextureDX;
    //ID3D11ShaderResourceView      *mpColorResourceView;
    //ID3D11RenderTargetView        *mpColorRenderTargetView;

    CPUTTexture                   *mpColorTextureMSAA;
    GLuint                        mColorTextureDXMSAA;
    //ID3D11Texture2D               *mpColorTextureDXMSAA;
    //ID3D11ShaderResourceView      *mpColorResourceViewMSAA;

    UINT                           mSavedWidth;
    UINT                           mSavedHeight;
    GLuint                        mSavedColorRenderTargetView;
    GLuint                        mSavedDepthStencilView;
    //ID3D11RenderTargetView        *mpSavedColorRenderTargetView;
    //ID3D11DepthStencilView        *mpSavedDepthStencilView;

public:
    CPUTRenderTargetColor() :
        mWidth(0),
        mHeight(0),
        mRenderTargetSet(false),
        mColorFormat(0), //DXGI_FORMAT_UNKNOWN),
        mMultiSampleCount(1),
        mpColorTexture(NULL),
        mColorTextureOGLES(0),
        mColorTextureDXMSAA(0),
        mpColorTextureMSAA(NULL),
        mColorResourceView(0),
        //mColorResourceViewMSAA(0),
        mColorRenderTargetView(0),
        mSavedWidth(0),
        mSavedHeight(0),
        mSavedColorRenderTargetView(0),
        mSavedDepthStencilView(0)
    {
    }

    ~CPUTRenderTargetColor();

    CPUTResult CreateRenderTarget(
        cString     textureName,
        UINT        width,
        UINT        height,
        GLuint colorFormat,
        UINT        multiSampleCount = 1,
        bool        recreate = false
    );

    CPUTResult RecreateRenderTarget(
        UINT        width,
        UINT        height,
        GLuint      colorFormat=0, //= DXGI_FORMAT_UNKNOWN,
        UINT        multiSampleCount = 1
    );

    void SetRenderTarget(
        CPUTRenderParameters &renderParams,
        CPUTRenderTargetDepth *pDepthBuffer = NULL,
        GLuint renderTargetIndex=0,
        const float *pClearColor=NULL,
        bool  clear = false,
        float zClearVal = 0.0f
    );

    void RestoreRenderTarget( CPUTRenderParameters &renderParams );
    void Resolve( CPUTRenderParameters &renderParams );

    CPUTTexture              *GetColorTexture()      { return mpColorTexture; }
    GLuint GetColorResourceView() { return mColorResourceView; }
    UINT                      GetWidth()             { return mWidth; }
    UINT                      GetHeight()            { return mHeight; }
    GLuint               GetColorFormat()       { return mColorFormat; }
};

//--------------------------------------------------------------------------------------
class CPUTRenderTargetDepth
{
public:
    static GLuint GetActiveDepthStencilView() { return sActiveDepthStencilView; }
    static void                    SetActiveDepthStencilView(GLuint View) { sActiveDepthStencilView = View; }
    static void                    SetActiveWidthHeight( UINT width, UINT height ) {sCurrentWidth = width; sCurrentHeight=height; }
    static UINT                    GetActiveWidth()  {return sCurrentWidth; }
    static UINT                    GetActiveHeight() {return sCurrentHeight; }

protected:
    static UINT                    sCurrentWidth;
    static UINT                    sCurrentHeight;
    static GLuint sActiveDepthStencilView;

    cString                        mName;
    UINT                           mWidth;
    UINT                           mHeight;
    bool                           mRenderTargetSet;
                                  
    GLuint                    mDepthFormat;
    UINT                           mMultiSampleCount;
                                                                 
    CPUTTexture                   *mpDepthTexture;
    GLuint               mDepthTextureDX;
    GLuint      mDepthResourceView;
    GLuint        mDepthStencilView;
    //ID3D11Texture2D               *mpDepthTextureDX;
    //ID3D11ShaderResourceView      *mpDepthResourceView;
    //ID3D11DepthStencilView        *mpDepthStencilView;
                                  
    UINT                           mSavedWidth;
    UINT                           mSavedHeight;
    GLuint        mSavedColorRenderTargetView;
    GLuint        mSavedDepthStencilView;
    //ID3D11RenderTargetView        *mpSavedColorRenderTargetView;
    //ID3D11DepthStencilView        *mpSavedDepthStencilView;
                                  
public:
    CPUTRenderTargetDepth() :
        mWidth(0),
        mHeight(0),
        mRenderTargetSet(false),
        mDepthFormat(0), //DXGI_FORMAT_UNKNOWN),
        mMultiSampleCount(1),
        mpDepthTexture(NULL),
        mDepthTextureDX(0),
        mDepthResourceView(0),
        mDepthStencilView(0),
        mSavedWidth(0),
        mSavedHeight(0),
        mSavedColorRenderTargetView(0),
        mSavedDepthStencilView(0)
    {
    }
    ~CPUTRenderTargetDepth();

    CPUTResult CreateRenderTarget(
        cString     textureName,
        UINT        width,
        UINT        height,
        GLuint depthFormat,
        UINT        multiSampleCount = 1,
        bool        recreate = false
    );

    CPUTResult RecreateRenderTarget(
        UINT        width,
        UINT        height,
        GLuint depthFormat = 0, //DXGI_FORMAT_UNKNOWN,
        UINT        multiSampleCount = 1
    );

    void SetRenderTarget(
        CPUTRenderParameters &renderParams,
        DWORD renderTargetIndex = 0,
        float zClearVal = 0.0f,
        bool  clear = false
    );

    void RestoreRenderTarget( CPUTRenderParameters &renderParams );

    GLuint   GetDepthBufferView()   { return mDepthStencilView; }
    GLuint GetDepthResourceView() { return mDepthResourceView; }
    UINT                      GetWidth()             { return mWidth; }
    UINT                      GetHeight()            { return mHeight; }
};

#endif // _CPUTRENDERTARGETOGLES_H
