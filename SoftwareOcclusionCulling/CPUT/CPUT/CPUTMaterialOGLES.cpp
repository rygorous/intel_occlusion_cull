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
#include "CPUTMaterialOGLES.h"
#include "CPUT_OGLES.h"
#include "CPUTRenderStateBlockOGLES.h"
//#include "D3DCompiler.h"

// Constructor
//-----------------------------------------------------------------------------
CPUTMaterialOGLES::CPUTMaterialOGLES() :
    m_pPixelShader(NULL),
    m_pVertexShader(NULL),
    m_pGeometryShader(NULL),
    m_pHullShader(NULL),
    m_pDomainShader(NULL),
    m_TextureParameterCount(0),
    m_SamplerParameterCount(0),
    m_pTextureParameterName(NULL),
    m_pTextureParameterBindPoint(NULL),
    m_pSamplerParameterName(NULL),
    m_pSamplerParameterBindPoint(NULL)
{
    // initialize texture slot list to null
    for(int ii=0; ii<CPUT_MATERIAL_MAX_TEXTURE_SLOTS; ii++)
    {
        m_pBindTexture[ii] = 0;
    }
}

    // Destructor
//-----------------------------------------------------------------------------
CPUTMaterialOGLES::~CPUTMaterialOGLES()
{
    //// release textures
    //for(int ii=0; ii<CPUT_MATERIAL_MAX_TEXTURE_SLOTS; ii++)
    //{
    //    SAFE_RELEASE( m_pTexture[ii] );
    //}
    //SAFE_DELETE_ARRAY(m_pTextureParameterName);
    //SAFE_DELETE_ARRAY(m_pTextureParameterBindPoint);
    //SAFE_DELETE_ARRAY(m_pSamplerParameterName);
    //SAFE_DELETE_ARRAY(m_pSamplerParameterBindPoint);

    //// reset count to 0
    //m_TextureCount = 0;

    //// release any shaders
    //SAFE_RELEASE(m_pPixelShader);
    //SAFE_RELEASE(m_pVertexShader);
    //SAFE_RELEASE(m_pGeometryShader);
    //SAFE_RELEASE(m_pHullShader);
    //SAFE_RELEASE(m_pDomainShader);
    //SAFE_RELEASE(m_pRenderStateBlock);

    CPUTMaterial::~CPUTMaterial();
}

//-----------------------------------------------------------------------------
void CPUTMaterialOGLES::SetRenderStates(CPUTRenderParameters &renderParams)
{
    //ID3D11DeviceContext *pContext = ((CPUTRenderParametersDX*)&renderParams)->m_pContext;
    //pContext->VSSetShader( m_pVertexShader   ? m_pVertexShader->GetNativeVertexShader()    : NULL, NULL, 0 );
    //pContext->PSSetShader( m_pPixelShader    ? m_pPixelShader->GetNativePixelShader()      : NULL, NULL, 0 );
    //pContext->GSSetShader( m_pGeometryShader ? m_pGeometryShader->GetNativeGeometryShader(): NULL, NULL, 0 );
    //pContext->HSSetShader( m_pHullShader     ? m_pHullShader->GetNativeHullShader()        : NULL, NULL, 0 );
    //pContext->DSSetShader( m_pDomainShader   ? m_pDomainShader->GetNativeDomainShader()    : NULL, NULL, 0 );
    //// TODO: set other shaders (HULL, Domain, etc.)

    //pContext->PSSetShaderResources( 0, CPUT_MATERIAL_MAX_TEXTURE_SLOTS, m_ppBindTexture );

    //if( m_pRenderStateBlock )
    //{
    //    // We know we have a DX11 class.  Does this correctly bypass the virtual?
    //    // Should we move it to the DX11 class.
    //    ((CPUTRenderStateBlockDX11*)m_pRenderStateBlock)->SetRenderStates(renderParams);
    //}
    //else
    //{
    //    CPUTRenderStateBlock::GetDefaultRenderStateBlock()->SetRenderStates(renderParams);
    //}
}


//-----------------------------------------------------------------------------
CPUTResult CPUTMaterialOGLES::LoadMaterial(const cString& fileName)
{
    CPUTResult result = CPUT_SUCCESS;

  //  // use the fileName for now, maybe we'll add names later?
  //  m_MaterialName = fileName;

  //  // Open/parse the file
  //  CPUTConfigFile file;
  //  result = file.LoadFile(fileName);
  //  if(CPUTFAILED(result))
  //  {
  //      return result;
  //  }

  //  // Make a local copy of all the parameters
  //  m_Properties = *file.GetBlock(0);

  //  // get necessary device and AssetLibrary pointers
  //  ID3D11Device* pD3dDevice = CPUT_DX11::GetDevice();
  //  CPUTAssetLibraryDX11* pAssetLibrary = (CPUTAssetLibraryDX11*)CPUTAssetLibrary::GetAssetLibrary();

  //  // see if there are any pixel/vertex/geo shaders to load
  //  CPUTConfigEntry *pValue, *pEntryPointName, *pProfileName;
  //  pValue   = m_Properties.GetValueByName(_L("VertexShaderFile"));
  //  if( pValue->IsValid() )
  //  {
  //      pEntryPointName = m_Properties.GetValueByName(_L("VertexShaderMain"));
  //      pProfileName    = m_Properties.GetValueByName(_L("VertexShaderProfile"));
  //      pAssetLibrary->GetVertexShader(pValue->ValueAsString(), pD3dDevice, pEntryPointName->ValueAsString(), pProfileName->ValueAsString(), &m_pVertexShader );
  //  }

  //  // load and store the pixel shader if it was specified
  //  pValue  = m_Properties.GetValueByName(_L("PixelShaderFile"));
  //  D3D11_SHADER_INPUT_BIND_DESC desc;

  //  if( pValue->IsValid() )
  //  {
  //      pEntryPointName = m_Properties.GetValueByName(_L("PixelShaderMain"));
  //      pProfileName    = m_Properties.GetValueByName(_L("PixelShaderProfile"));
  //      pAssetLibrary->GetPixelShader(pValue->ValueAsString(), pD3dDevice, pEntryPointName->ValueAsString(), pProfileName->ValueAsString(), &m_pPixelShader);

  //      // ***************************
  //      // Use shader reflection to get texture and sampler names.  We use them later to bind .mtl texture-specification to shader parameters/variables.
  //      // ***************************
  //      ID3DBlob *pPixelShaderBlob = m_pPixelShader->GetBlob();
  //      ID3D11ShaderReflection* pReflector = NULL; 
  //      D3DReflect( pPixelShaderBlob->GetBufferPointer(), pPixelShaderBlob->GetBufferSize(), IID_ID3D11ShaderReflection, (void**)&pReflector);
  //      // Walk through the shader input bind descriptors.  Find the samplers and textures.
  //      int ii=0;
		//HRESULT hr = pReflector->GetResourceBindingDesc( ii++, &desc );
		//while( SUCCEEDED(hr) )
		//{
		//	if( desc.Type == D3D_SIT_TEXTURE ) m_TextureParameterCount++;
		//	if( desc.Type == D3D_SIT_SAMPLER ) m_SamplerParameterCount++;

		//	hr = pReflector->GetResourceBindingDesc( ii++, &desc );
		//}
  //      m_pTextureParameterName      = new cString[m_TextureParameterCount];
  //      m_pTextureParameterBindPoint = new UINT[m_TextureParameterCount];
  //      m_pSamplerParameterName      = new cString[m_SamplerParameterCount];
  //      m_pSamplerParameterBindPoint = new UINT[m_SamplerParameterCount];
  //      
  //      // Start over.  This time, copy the names.
  //      ii=0;
  //      UINT textureIndex = 0;
  //      UINT samplerIndex = 0;
		//hr = pReflector->GetResourceBindingDesc( ii++, &desc );
  //      while( SUCCEEDED(hr) )
  //      {
  //          if( desc.Type == D3D_SIT_TEXTURE )
  //          {
  //              m_pTextureParameterName[textureIndex] = cString(s2ws(desc.Name));
  //              m_pTextureParameterBindPoint[textureIndex] = desc.BindPoint;
  //              textureIndex++;
  //          }
  //          if( desc.Type == D3D_SIT_SAMPLER )
  //          {
  //              m_pSamplerParameterName[samplerIndex] = cString(s2ws(desc.Name));
  //              m_pSamplerParameterBindPoint[samplerIndex] = desc.BindPoint;
  //              samplerIndex++;
  //          }
		//	hr = pReflector->GetResourceBindingDesc( ii++, &desc );
  //      }
  //  }

  //  // load and store the geometry shader if it was specified
  //  pValue = m_Properties.GetValueByName(_L("GeometryShaderFile"));
  //  if( pValue->IsValid() )
  //  {
  //      pEntryPointName = m_Properties.GetValueByName(_L("GeometryShaderMain"));
  //      pProfileName = m_Properties.GetValueByName(_L("GeometryShaderProfile"));
  //      pAssetLibrary->GetGeometryShader(pValue->ValueAsString(), pD3dDevice, pEntryPointName->ValueAsString(), pProfileName->ValueAsString(), &m_pGeometryShader);
  //  }

  //  // load and store the hull shader if it was specified
  //  pValue = m_Properties.GetValueByName(_L("HullShaderFile"));
  //  if( pValue->IsValid() )
  //  {
  //      pEntryPointName = m_Properties.GetValueByName(_L("HullShaderMain"));
  //      pProfileName = m_Properties.GetValueByName(_L("HullShaderProfile"));
  //      pAssetLibrary->GetHullShader(pValue->ValueAsString(), pD3dDevice, pEntryPointName->ValueAsString(), pProfileName->ValueAsString(), &m_pHullShader);
  //  }

  //  // load and store the domain shader if it was specified
  //  pValue = m_Properties.GetValueByName(_L("DomainShaderFile"));
  //  if( pValue->IsValid() )
  //  {
  //      pEntryPointName = m_Properties.GetValueByName(_L("DomainShaderMain"));
  //      pProfileName = m_Properties.GetValueByName(_L("DomainShaderProfile"));
  //      pAssetLibrary->GetDomainShader(pValue->ValueAsString(), pD3dDevice, pEntryPointName->ValueAsString(), pProfileName->ValueAsString(), &m_pDomainShader);
  //  }

  //  // load and store the render state file if it was specified
  //  pValue = m_Properties.GetValueByName(_L("RenderStateFile"));
  //  if( pValue->IsValid() )
  //  {
  //      m_pRenderStateBlock = pAssetLibrary->GetRenderStateBlock(pValue->ValueAsString());
  //  }

  //  // extract out the textures and map them to their slots in the order they appeared in the .mtl file
  //  cString tagName;
  //  bool done = false;
  //  // char pNumber[CPUT_MAX_DIGIT_STRING_LENGTH];

  //  pValue = m_Properties.GetValueByName(_L("ReceiveShadows"));
  //  if( pValue->IsValid() )
  //  {
  //      m_ReceiveShadows = pValue->ValueAsBool();
  //  } else
  //  {
  //      m_ReceiveShadows = true;
  //  }
  //  bool appendShadowTexture = m_ReceiveShadows;
  //  // loop over all the textures and load/get them
  //  for(m_TextureCount=0; m_TextureCount < m_TextureParameterCount; m_TextureCount++)
  //  {
  //      cString textureName;

  //      // see if it exists
  //      tagName = m_pTextureParameterName[m_TextureCount];
  //      if( tagName == L"_Shadow" )
  //      {
  //          // _Shadow is a programatic texture.  It isn't specified in the .mtl file.
  //          textureName = L"$shadow_depth";
  //      }
  //      else
  //      {
  //          pValue = m_Properties.GetValueByName(tagName);
  //          if(!pValue->IsValid())
  //          {
  //              ASSERT( 0, L"Can't find texture '" + tagName + L"'." ); //  TODO: fix message
  //          } else
  //          {
  //              textureName = pValue->ValueAsString();
  //              if( 0 == textureName.length() )
  //              {
  //                  // Texture name not specified.  Load default.dds instead
  //                  textureName = _L("default.dds");
  //              }
  //          }
  //      }
  //      // Get the sRGB flag (default to true)
  //      bool loadAsSRGB = true;
  //      cString SRGBName = tagName+_L("sRGB");
  //      CPUTConfigEntry* pSRGBValue = m_Properties.GetValueByName(SRGBName);
  //      if(pSRGBValue->IsValid())
  //      {
  //          loadAsSRGB = pSRGBValue->ValueAsBool();
  //      }
  //      // load/get the texture
  //      m_pTexture[m_TextureCount] = pAssetLibrary->GetTexture( textureName, false, loadAsSRGB );
  //      if(!m_pTexture[m_TextureCount])
  //      {
  //          // We do not fail load but drop a console error when we attempt to load a texture that
  //          // is not there and attempt to keep going
  //          cString message = _L("CPUTMaterialDX11::LoadMaterial() - Material '");
  //          message = message + m_MaterialName + _L("' Could not load texture ");
  //          message+=pValue->ValueAsString()+_L(".  Continuing - but texture is missing.\n\n");
  //          TRACE(message.c_str());
  //          result = CPUT_SUCCESS;
  //      }
  //      // The shader file (e.g. .fx) can specify the texture bind point (e.g., t0).  Those specifications 
  //      // might not be contiguous, and there might be gaps (bind points without assigned textures)
  //      // TODO: Warn about missing bind points?
  //      UINT bindPoint = m_pTextureParameterBindPoint[m_TextureCount]; 
  //      m_ppBindTexture[bindPoint] = ((CPUTTextureDX11*)m_pTexture[m_TextureCount])->GetShaderResourceView();
  //  }
    return result;
}

//-----------------------------------------------------------------------------
void CPUTMaterialOGLES::RebindTextures()
{
    //for( UINT ii=0; ii<m_TextureCount; ii++ )
    //{
    //    UINT bindPoint = m_pTextureParameterBindPoint[ii];
    //    m_ppBindTexture[bindPoint] = ((CPUTTextureDX11*)m_pTexture[ii])->GetShaderResourceView();
    //}
}