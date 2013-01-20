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
#ifndef __CPUTMODELOGLES_H__
#define __CPUTMODELOGLES_H__

#include "CPUTModel.h"
#include "CPUT_OGLES.h"

class CPUTMeshOGLES;
class CPUTRenderParametersOGLES;
class CPUTMaterialOGLES;

//--------------------------------------------------------------------------------------
struct CPUTModelConstantBuffer
{
    // see p199 in yellow book
 //   XMMATRIX  World;
	//XMMATRIX  WorldViewProjection;
 //   XMMATRIX  InverseWorld;
 //   XMVECTOR  LightDirection;
 //   XMVECTOR  EyePosition;
 //   XMMATRIX  LightWorldViewProjection;
 //   XMMATRIX  ViewProjection;
 //   XMVECTOR  BoundingBoxCenterWorldSpace;
 //   XMVECTOR  BoundingBoxHalfWorldSpace;
 //   XMVECTOR  BoundingBoxCenterObjectSpace;
 //   XMVECTOR  BoundingBoxHalfObjectSpace;
};

//--------------------------------------------------------------------------------------
class CPUTModelOGLES : public CPUTModel
{
protected:
    GLuint m_ModelConstantBuffer;

    // Destructor is not public.  Must release instead of delete.
    ~CPUTModelOGLES(){ /*gltodo: delete uniform block &m_ModelConstantBuffer */ }

public:
    CPUTModelOGLES() : m_ModelConstantBuffer(0) {}

    CPUTMeshOGLES *GetMesh(const UINT index) const;
    CPUTResult    LoadModel(CPUTConfigBlock *pBlock, int *pParentID);
    CPUTResult    CloneModel(CPUTConfigBlock *pBlock, int *pParentID, CPUTModel *pMasterModel);
    void          SetRenderStates(CPUTRenderParameters &renderParams);
    void          Render(CPUTRenderParameters &renderParams, CPUTMaterial *pOverrideMaterial=NULL);
    void          SetMaterial(UINT ii, CPUTMaterial *pMaterial);
    void          DrawBoundingBox(CPUTRenderParameters &renderParams);
};


#endif // __CPUTMODELOGLES_H__