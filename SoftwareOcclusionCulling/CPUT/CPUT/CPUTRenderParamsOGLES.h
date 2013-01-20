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
#ifndef __CPUTRENDERPARAMSOGLES_H__
#define __CPUTRENDERPARAMSOGLES_H__

#include "CPUT.h"
#include "CPUTRenderParams.h"

class CPUTRenderParametersOGLES : public CPUTRenderParameters
{
public:
    //ID3D11DeviceContext *m_pContext;

public:
    CPUTRenderParametersOGLES( /*ID3D11DeviceContext *pContext,*/ bool drawModels=true, bool renderOnlyVisibleModels=true, bool showBoundingBoxes=false )
    {
        m_ShowBoundingBoxes       = showBoundingBoxes;
        m_DrawModels              = drawModels;
        m_RenderOnlyVisibleModels = renderOnlyVisibleModels;
    }
    ~CPUTRenderParametersOGLES(){}
};

#endif // #ifndef __CPUTRENDERPARAMSOGLES_H__