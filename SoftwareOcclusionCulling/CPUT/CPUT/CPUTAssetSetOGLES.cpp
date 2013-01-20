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
#include "CPUTAssetSetOGLES.h"

#include "CPUTModelOGLES.h"
#include "CPUTAssetLibraryOGLES.h"
#include "CPUTCamera.h"
#include "CPUTLight.h"

//-----------------------------------------------------------------------------
CPUTAssetSetOGLES::~CPUTAssetSetOGLES()
{
    // Release all the elements in the asset list.  Note that we don't
    // recursively delete the hierarchy here.
    // We release the entries here because this class is where we add them.
    // TODO: Howevere, all derivations will have this, so perhaps it should go in the base.
    for( UINT ii=0; ii<m_AssetCount; ii++ )
    {
        SAFE_RELEASE( m_ppAssetList[ii] );
    }
    SAFE_DELETE_ARRAY(m_ppAssetList);
}

//-----------------------------------------------------------------------------
CPUTResult CPUTAssetSetOGLES::LoadAssetSet(cString name)
{
    CPUTResult result = CPUT_SUCCESS;

    // if not found, load the set file
    CPUTConfigFile ConfigFile;
    result = ConfigFile.LoadFile(name);
    ASSERT( CPUTSUCCESS(result), _L("Failed loading set file '") + name + _L("'.") );

    m_AssetCount = ConfigFile.BlockCount() + 1; // Add one for the implied root node
    m_ppAssetList = new CPUTRenderNode*[m_AssetCount];
    m_ppAssetList[0] = m_pRootNode;
    m_pRootNode->AddRef();

    CPUTAssetLibraryOGLES *pAssetLibrary = (CPUTAssetLibraryOGLES*)CPUTAssetLibrary::GetAssetLibrary();

    for(UINT ii=0; ii<m_AssetCount-1; ii++) // Note: -1 because we added one for the root node (we don't load it)
    {
        CPUTConfigBlock *pBlock = ConfigFile.GetBlock(ii);
        int assetIndex = pBlock->GetNameValue();
        cString nodeType = pBlock->GetValueByName(_L("type"))->ValueAsString();
        CPUTRenderNode *pParentNode = NULL;

        // TODO: use Get*() instead of Load*() ?

        cString name = pBlock->GetValueByName(_L("name"))->ValueAsString();

        int parentIndex;
        CPUTRenderNode *pNode;
        if(0==nodeType.compare(_L("null")))
        {
            pNode  = pNode = new CPUTNullNode();
            result = ((CPUTNullNode*)pNode)->LoadNullNode(pBlock, &parentIndex);
            pAssetLibrary->AddNullNode(name, (CPUTNullNode*)pNode);
            // Add this null's name to our prefix
            // Append this null's name to our parent's prefix
            pParentNode = m_ppAssetList[parentIndex+1];
            pNode->SetParent( pParentNode );
            pParentNode->AddChild( pNode );
            cString &parentPrefix = pParentNode->GetPrefix();
            pNode->SetPrefix( parentPrefix + _L(".") + name );
        }
        else if(0==nodeType.compare(_L("model")))
        {
            CPUTConfigEntry *pValue = pBlock->GetValueByName( _L("instance") );
            CPUTModelOGLES *pModel;
            if( pValue == &CPUTConfigEntry::sNullConfigValue )
            {
                // Not found.  So, not an instance.
                pModel = new CPUTModelOGLES();
                pModel->LoadModel(pBlock, &parentIndex);
            }
            else
            {
                int instance = pValue->ValueAsInt();
                pModel = new CPUTModelOGLES();
                pModel->CloneModel(pBlock, &parentIndex, (CPUTModel*)m_ppAssetList[instance+1]);
            }
            pParentNode = m_ppAssetList[parentIndex+1];
            pModel->SetParent( pParentNode );
            pParentNode->AddChild( pModel );
            cString &parentPrefix = pParentNode->GetPrefix();
            pModel->SetPrefix( parentPrefix );
            pAssetLibrary->AddModel(parentPrefix + name, pModel);

            pModel->UpdateBoundsWorldSpace();

#ifdef SUPPORT_DRAWING_BOUNDING_BOXES
            // Create a mesh for rendering the bounding box
            // TODO: There is definitely a better way to do this.  But, want to see the bounding boxes!
            pModel->CreateBoundingBoxMesh();
#endif
            pNode = pModel;
        }
        else if(0==nodeType.compare(_L("light")))
        {
            pNode = new CPUTLight();
            ((CPUTLight*)pNode)->LoadLight(pBlock, &parentIndex);
            pParentNode = m_ppAssetList[parentIndex+1]; // +1 because we added a root node to the start
            pNode->SetParent( pParentNode );
            pParentNode->AddChild( pNode );
            cString &parentPrefix = pParentNode->GetPrefix();
            pNode->SetPrefix( parentPrefix );
            pAssetLibrary->AddLight(parentPrefix + name, (CPUTLight*)pNode);
        }
        else if(0==nodeType.compare(_L("camera")))
        {
            pNode = new CPUTCamera();
            ((CPUTCamera*)pNode)->LoadCamera(pBlock, &parentIndex);
            pParentNode = m_ppAssetList[parentIndex+1]; // +1 because we added a root node to the start
            pNode->SetParent( pParentNode );
            pParentNode->AddChild( pNode );
            cString &parentPrefix = pParentNode->GetPrefix();
            pNode->SetPrefix( parentPrefix );
            pAssetLibrary->AddCamera(parentPrefix + name, (CPUTCamera*)pNode);
            if( !m_pFirstCamera ) { m_pFirstCamera = (CPUTCamera*)pNode; m_pFirstCamera->AddRef();}
            ++m_CameraCount;
        }
        else
        {
            ASSERT(0,_L("Unsupported node type '") + nodeType + _L("'."));
        }

        // Add the node to our asset list (i.e., the linear list, not the hierarchical)
        m_ppAssetList[ii+1] = pNode;
        // Don't AddRef.Creating it set the refcount to 1.  We add it to the list, and then we're done with it.
        // Net effect is 0 (+1 to add to list, and -1 because we're done with it)
        // pNode->AddRef();
    }
    return result;
}

//-----------------------------------------------------------------------------
CPUTAssetSet *CPUTAssetSetOGLES::CreateAssetSet( const cString &name, const cString &absolutePathAndFilename )
{
    CPUTAssetLibraryOGLES *pAssetLibrary = ((CPUTAssetLibraryOGLES*)CPUTAssetLibrary::GetAssetLibrary());

    // Create the root node.
    CPUTNullNode *pRootNode = new CPUTNullNode();
    pRootNode->SetName(_L("_CPUTAssetSetRootNode_"));

    // Create the asset set, set its root, and load it
    CPUTAssetSet   *pNewAssetSet = new CPUTAssetSetOGLES();
    pNewAssetSet->SetRoot( pRootNode );
    pAssetLibrary->AddNullNode( name + _L("_Root"), pRootNode );

    pNewAssetSet->LoadAssetSet(absolutePathAndFilename);
    pAssetLibrary->AddAssetSet(name, pNewAssetSet);

    return pNewAssetSet;
}

