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
#include "CPUTModelOGLES.h"
#include "CPUTMaterialOGLES.h"
#include "CPUTRenderParamsOGLES.h"
//#include "CPUTFrustum.h"

// Render - render this model (only)
//-----------------------------------------------------------------------------
void CPUTModelOGLES::Render(CPUTRenderParameters &renderParams, CPUTMaterial *pOverrideMaterial)
{
//    CPUTRenderParametersDX *pParams = (CPUTRenderParametersDX*)&renderParams;
//    CPUTCamera             *pCamera = pParams->m_pCamera;
//
//#ifdef SUPPORT_DRAWING_BOUNDING_BOXES 
//    if( renderParams.m_ShowBoundingBoxes && (!pCamera || pCamera->m_Frustum.IsVisible( m_BoundingBoxCenterWorldSpace, m_BoundingBoxHalfWorldSpace )))
//    {
//        DrawBoundingBox( renderParams );
//    }
//#endif
//    if( !renderParams.m_DrawModels ) { return; }
//
//    // TODO: add world-space bounding box to model so we don't need to do that work every frame
//    if( !pParams->m_RenderOnlyVisibleModels || !pCamera || pCamera->m_Frustum.IsVisible( m_BoundingBoxCenterWorldSpace, m_BoundingBoxHalfWorldSpace ) )
//    {
//        SetRenderStates(renderParams);
//
//        // loop over all meshes in this model and draw them
//        for(UINT ii=0; ii<m_MeshCount; ii++)
//        {
//            CPUTMaterialDX11 *pMaterial = (CPUTMaterialDX11*)(pOverrideMaterial ? pOverrideMaterial : m_pMaterial[ii]);
//            pMaterial->SetRenderStates(renderParams);
//
//            // Potentially need to use a different vertex-layout object!
//            CPUTVertexShaderDX11 *pVertexShader = pMaterial->GetVertexShader();
//            ((CPUTMeshDX11*)m_pMesh[ii])->Draw(renderParams, pVertexShader);
//        }
//    }
}

// Load the set file definition of this object
// 1. Parse the block of name/parent/transform info for model block
// 2. Load the model's binary payload (i.e., the meshes)
// 3. Assert the # of meshes matches # of materials
// 4. Load each mesh's material
//-----------------------------------------------------------------------------
CPUTResult CPUTModelOGLES::LoadModel(CPUTConfigBlock *pBlock, int *pParentID)
{
    CPUTResult result = CPUT_SUCCESS;

 //   // set the null/group node name
 //   m_Name = pBlock->GetValueByName(_L("name"))->ValueAsString();
 //   m_Name = m_Name + _L(".mdl");


 //   // resolve the full path name
 //   cString modelLocation;
 //   cString resolvedPathAndFile;
 //   modelLocation = ((CPUTAssetLibraryDX11*)CPUTAssetLibrary::GetAssetLibrary())->GetModelDirectory();
 //   modelLocation = modelLocation+m_Name;
 //   CPUTOSServices::GetOSServices()->ResolveAbsolutePathAndFilename(modelLocation, &resolvedPathAndFile);	

 //   // Get the parent ID.  Note: the caller will use this to set the parent.
 //   *pParentID = pBlock->GetValueByName(_L("parent"))->ValueAsInt();

 //   LoadParentMatrixFromParameterBlock( pBlock );

 //   // Get the bounding box information
	//float3 center(0.0f), half(0.0f);
 //   pBlock->GetValueByName(_L("BoundingBoxCenter"))->ValueAsFloatArray(center.f, 3);
	//pBlock->GetValueByName(_L("BoundingBoxHalf"))->ValueAsFloatArray(half.f, 3);
 //   m_BoundingBoxCenterObjectSpace = center;
 //   m_BoundingBoxHalfObjectSpace   = half;
	//
 //   // Load materials
 //   // the # of meshes in the binary file better match the number of meshes in the .set file definition
 //   m_MeshCount = pBlock->GetValueByName(_L("meshcount"))->ValueAsInt();
 //   m_pMesh     = new CPUTMesh*[m_MeshCount];
 //   m_pMaterial = new CPUTMaterial*[m_MeshCount];
 //   memset( m_pMaterial, 0, m_MeshCount * sizeof(CPUTMaterial*) );
 //   
 //   // get the material names, load them, and match them up with each mesh
 //   cString materialName;
 //   char pNumber[4];
 //   cString materialValueName;
 //   CPUTAssetLibraryDX11* pAssetLibrary = (CPUTAssetLibraryDX11*)CPUTAssetLibrary::GetAssetLibrary();

 //   for(UINT ii=0; ii<m_MeshCount; ii++)
 //   {
 //       m_pMesh[ii] = new CPUTMeshDX11();

 //       // get the right material number ('material0', 'material1', 'material2', etc)
 //       materialValueName = _L("material");
 //       _itoa_s(ii, pNumber, 4, 10);
 //       materialValueName.append(s2ws(pNumber));
 //       materialName = pBlock->GetValueByName(materialValueName)->ValueAsString();
 //       // materialName = _L("teapot_lambert");

 //       // attempt to load/retrieve the material
 //       CPUTMaterialDX11* pMaterial=NULL;

 //       // Get/load material for this mesh
 //       pMaterial = (CPUTMaterialDX11*)pAssetLibrary->GetMaterial(materialName);
 //       ASSERT( pMaterial, _L("Couldn't find material.") );

 //       // set the material on this mesh
 //       // TODO: Model owns the materials.  That allows different models to share meshes (aka instancing) that have different materials
 //       SetMaterial(ii, pMaterial);

 //       // Release the extra refcount we're holding from the GetMaterial operation earlier
 //       // now the asset library, and this model have the only refcounts on that material
 //       pMaterial->Release();
 //   }
 //   
 //   // trigger a binary load of the model file
 //   // TODO: Change to use GetModel()
 //   result = LoadModelPayload(resolvedPathAndFile);
 //   ASSERT( CPUTSUCCESS(result), _L("Failed loading model") );

 //   for(UINT ii=0; ii<m_MeshCount; ii++)
 //   {
 //       CPUTMeshDX11 *pMesh = (CPUTMeshDX11*)(m_pMesh[ii]);
 //       if(pMesh->GetIndexCount())
 //       {
 //           D3D11_INPUT_ELEMENT_DESC *pDesc = pMesh->GetLayoutDescription();
 //           ((CPUTMeshDX11*)(m_pMesh[ii]))->BindVertexShaderLayout(m_pMaterial[ii]);
 //       }
 //   }

 //   // Create the model matrix constant buffer.  TODO: Make sure we have one per model instance, and not just one per master.
 //   HRESULT hr;
 //   D3D11_BUFFER_DESC bd = {0};
 //   bd.ByteWidth = sizeof(CPUTModelConstantBuffer);
 //   bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
 //   bd.Usage = D3D11_USAGE_DYNAMIC;
 //   bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
 //   hr = (CPUT_DX11::GetDevice())->CreateBuffer( &bd, NULL, &m_pModelConstantBuffer );
 //   ASSERT( !FAILED( hr ), _L("Error creating constant buffer.") );
 //   CPUTSetDebugName( m_pModelConstantBuffer, _L("Model Constant buffer") );

    return result;
}

// Load the set file definition of this object
// 1. Parse the block of name/parent/transform info for model block
// 2. fire off a binary load of the model object
// 3. When binary file is loaded, model will consist of it's meshes
// 4. compare the # of meshes with # of materials - they should match
// 5. Parse and fire off a load event for each material
// 6. Match up the loaded material object with it's mesh
//-----------------------------------------------------------------------------
CPUTResult CPUTModelOGLES::CloneModel(CPUTConfigBlock *pBlock, int *pParentID, CPUTModel *pMasterModel)
{
    CPUTResult result = CPUT_SUCCESS;

 //   // set the null/group node name
 //   m_Name = pBlock->GetValueByName(_L("name"))->ValueAsString();
 //   m_Name = m_Name + _L(".mdl");


 //   // resolve the full path name
 //   cString modelLocation;
 //   cString resolvedPathAndFile;
 //   modelLocation = ((CPUTAssetLibraryDX11*)CPUTAssetLibrary::GetAssetLibrary())->GetModelDirectory();
 //   modelLocation = modelLocation+m_Name;
 //   CPUTOSServices::GetOSServices()->ResolveAbsolutePathAndFilename(modelLocation, &resolvedPathAndFile);	

 //   // Get the parent ID.  Note: the caller will use this to set the parent.
 //   *pParentID = pBlock->GetValueByName(_L("parent"))->ValueAsInt();

 //   LoadParentMatrixFromParameterBlock( pBlock );

 //   // Get the bounding box information
	//float3 center(0.0f), half(0.0f);
 //   pBlock->GetValueByName(_L("BoundingBoxCenter"))->ValueAsFloatArray(center.f, 3);
	//pBlock->GetValueByName(_L("BoundingBoxHalf"))->ValueAsFloatArray(half.f, 3);
 //   m_BoundingBoxCenterObjectSpace = center;
 //   m_BoundingBoxHalfObjectSpace   = half;
	//
 //   // Load materials
 //   // the # of meshes in the binary file better match the number of meshes in the .set file definition
 //   m_MeshCount = pBlock->GetValueByName(_L("meshcount"))->ValueAsInt();
 //   m_pMesh     = new CPUTMesh*[m_MeshCount];
 //   m_pMaterial = new CPUTMaterial*[m_MeshCount];
 //   memset( m_pMaterial, 0, m_MeshCount * sizeof(CPUTMaterial*) );
 //   
 //   // get the material names, load them, and match them up with each mesh
 //   cString materialName;
 //   char pNumber[4];
 //   cString materialValueName;
 //   CPUTAssetLibraryDX11* pAssetLibrary = (CPUTAssetLibraryDX11*)CPUTAssetLibrary::GetAssetLibrary();

 //   // Point to the master model's mesh(es)
 //   CPUTModelDX11 *pMasterModelDX = (CPUTModelDX11*)pMasterModel;
 //   m_MeshCount = pMasterModelDX->m_MeshCount;

 //   for(UINT ii=0; ii<m_MeshCount; ii++)
 //   {
 //       // Reference the maste model's mesh.  Don't create a new one.
 //       m_pMesh[ii] = pMasterModelDX->m_pMesh[ii];
 //       m_pMesh[ii]->AddRef();

 //       // get the right material number ('material0', 'material1', 'material2', etc)
 //       materialValueName = _L("material");
 //       _itoa_s(ii, pNumber, 4, 10);
 //       materialValueName.append(s2ws(pNumber));
 //       materialName = pBlock->GetValueByName(materialValueName)->ValueAsString();
 //       // materialName = _L("teapot_lambert");

 //       // Get/load material for this mesh
 //       CPUTMaterialDX11 *pMaterial = (CPUTMaterialDX11*)pAssetLibrary->GetMaterial(materialName);
 //       ASSERT( pMaterial, _L("Couldn't find material.") );

 //       // set the material on this mesh
 //       // TODO: Model owns the materials.  That allows different models to share meshes (aka instancing) that have different materials
 //       SetMaterial(ii, pMaterial);

 //       // Release the extra refcount we're holding from the GetMaterial operation earlier
 //       // now the asset library, and this model have the only refcounts on that material
 //       pMaterial->Release();
 //   }

 //   for(UINT ii=0; ii<m_MeshCount; ii++)
 //   {
 //       CPUTMeshDX11 *pMesh = (CPUTMeshDX11*)(m_pMesh[ii]);
 //       if(pMesh->GetIndexCount())
 //       {
 //           D3D11_INPUT_ELEMENT_DESC *pDesc = pMesh->GetLayoutDescription();
 //           pMesh->BindVertexShaderLayout(m_pMaterial[ii]);
 //       }
 //   }

 //   // Create the model matrix constant buffer.  TODO: Make sure we have one per model instance, and not just one per master.
 //   HRESULT hr;
 //   D3D11_BUFFER_DESC bd = {0};
 //   bd.ByteWidth = sizeof(CPUTModelConstantBuffer);
 //   bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
 //   bd.Usage = D3D11_USAGE_DYNAMIC;
 //   bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
 //   hr = (CPUT_DX11::GetDevice())->CreateBuffer( &bd, NULL, &m_pModelConstantBuffer );
 //   ASSERT( !FAILED( hr ), _L("Error creating constant buffer.") );
 //   CPUTSetDebugName( m_pModelConstantBuffer, _L("Constant buffer") );

    return result;
}

// Set the material associated with this mesh and create/re-use a
//-----------------------------------------------------------------------------
void CPUTModelOGLES::SetMaterial(UINT ii, CPUTMaterial *pMaterial)
{
    CPUTModel::SetMaterial(ii, pMaterial);

    //// Can't bind the layout if we haven't loaded the mesh yet.
    //CPUTMeshDX11 *pMesh = (CPUTMeshDX11*)m_pMesh[ii];
    //D3D11_INPUT_ELEMENT_DESC *pDesc = pMesh->GetLayoutDescription();
    //if( pDesc )
    //{
    //    pMesh->BindVertexShaderLayout(pMaterial);
    //}
}