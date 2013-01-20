#include "CPUTMaterialPhongGL31.h"


namespace CPUTGL31
{

    static char l_pPhongMaterialAttributeNames[CPUT_PHONG_MATERIAL_PROPERTY_COUNT][256] = 
    {
        // list MUST match the enum order in CPUT_PHONG_MATERIAL_PROPERTY
        "DiffuseColor", 
        "DiffuseFactor",
        "AmbientColor",
        "AmbientFactor",
        "SpecularColor",
        "SpecularFactor",
        "ReflectionColor",
        "ReflectionFactor",
        "EmissiveColor",
        "EmissiveFactor",
        "TransparentColor",
        "TransparencyFactor",
        "DisplacementColor",
        "DisplacementFactor",
        "Shininess",
        "Bump",
        "BumpFactor",
        "NormalMap",
    };

    static char l_pPhongMaterialTextureNames[CPUT_PHONG_MATERIAL_TEXTURE_COUNT][256] = 
    {
        // list MUST match the enum order in CPUT_PHONG_MATERIAL_TEXTURES
        "DiffuseColor",
        "AmbientColor",
        "SpecularColor",
        "NormalMap",   
    };

    // constructor
    //-----------------------------------------------------------------------------
    CPUTMaterialPhong::CPUTMaterialPhong()
    {
        // identify ourselves as a phong material
        SetMaterialType(CPUT_MATERIAL_PHONG);


        // add all the default Phong material parameters to the internal list
        // so we know they'll be there when queried/set/get later
        float pData[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        for(int i=0; i<CPUT_PHONG_MATERIAL_PROPERTY_COUNT; i++)
        {
            // set up with default values for now
            AddMaterialProperty(l_pPhongMaterialAttributeNames[i], CPUT_F32, 1, pData);
        }

        // set all texture pointers to NULL
        for(int i=0; i<CPUT_PHONG_MATERIAL_TEXTURE_COUNT; i++)
        {
            m_pTextureIDs[i]=0;
        }
    }

    // destructor
    //-----------------------------------------------------------------------------
    CPUTMaterialPhong::~CPUTMaterialPhong()
    {
        DeleteResources();

        // chain destructor
        CPUTMaterial::~CPUTMaterial();
    }

    // delete all allocated resources
    //-----------------------------------------------------------------------------
    void CPUTMaterialPhong::DeleteResources()
    {
        for(int i=0; i<CPUT_PHONG_MATERIAL_TEXTURE_COUNT; i++)
        {
            if(m_pTextureIDs[i])
            {
                // delete any loaded textures
                // todo: is this right?  what if they're shared?
                glDeleteTextures(1, &m_pTextureIDs[i]);            
                m_pTextureIDs[i]=0;
            }
        }
    }

    // sets the shader parameters for this Phong shader on what would be
    // the 'default' shader - i.e it doesn't work with any shader you cooked up 
    // on your own :)
    //-----------------------------------------------------------------------------
    CPUTResult CPUTMaterialPhong::BindToDefaultShader(GLuint ShaderProgramID)
    {
        // TODO: flesh this bad boy out with all the other parameters

        // bind all the material properties


        // bind all the textures
        // http://www.opengl.org/wiki/GLSL_Samplers#Binding_textures_to_samplers
        //glBindTexture(GL_TEXTURE_2D, m_pTextureIDs[CPUT_PHONG_MATERIAL_PROPERTY_DIFFUSE_TEXTURE]);

        // bind the current program
        glUseProgram(ShaderProgramID);

        // query the shader for each of these texture varable id's
        GLint diffuseImageLoc = glGetUniformLocation(ShaderProgramID, "diffuseTexture");
        GLint normalImageLoc = glGetUniformLocation(ShaderProgramID, "normalTexture");
        GLint specularImageLoc = glGetUniformLocation(ShaderProgramID, "specularTexture");
        GLint notfoundImageLoc = glGetUniformLocation(ShaderProgramID, "notfound");

        // assign texture units to the various input texture variable id's    
        glUniform1i(diffuseImageLoc, 0); //Texture unit 0 is for diffuse texture
        glUniform1i(normalImageLoc, 1); //Texture unit 1 is for normal maps.


        // turn on the shader units for each slot
        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_2D, m_pTextureIDs[CPUT_PHONG_MATERIAL_PROPERTY_DIFFUSE_TEXTURE]);
        glActiveTexture(GL_TEXTURE0 + 1);
        glBindTexture(GL_TEXTURE_2D, m_pTextureIDs[CPUT_PHONG_MATERIAL_PROPERTY_NORMAL_TEXTURE]);



        /*
        CPUTResult result;

        // update the material parameters
        MaterialConstantBuffer cbMaterial;

        CPUT_DATA_FORMAT_TYPE eFormatType;
        int FormatElementCount;
        float* pData = NULL;

        // ambient terms
        result = GetProperty(CPUT_PHONG_MATERIAL_PROPERTY_AMBIENT_COLOR, eFormatType, FormatElementCount, (void**)&pData);
        cbMaterial.ambientColor = XMFLOAT4(pData[0], pData[1], pData[2], 1.0f);
        result = GetProperty(CPUT_PHONG_MATERIAL_PROPERTY_AMBIENT_FACTOR, eFormatType, FormatElementCount, (void**)&pData);
        cbMaterial.ambientFactor = pData[0];

        // diffuse terms
        result = GetProperty(CPUT_PHONG_MATERIAL_PROPERTY_DIFFUSE_COLOR, eFormatType, FormatElementCount, (void**)&pData);
        cbMaterial.diffuseColor = XMFLOAT4(pData[0], pData[1], pData[2], 1.0f);
        result = GetProperty(CPUT_PHONG_MATERIAL_PROPERTY_DIFFUSE_FACTOR, eFormatType, FormatElementCount, (void**)&pData);
        cbMaterial.diffuseFactor = pData[0]; //1.0f;

        // fill terms to hit 32-bit boundary
        cbMaterial.fill1 = 1.0f;
        cbMaterial.fill2 = 1.0f;

        // set the constant buffer
        pImmediateContext->UpdateSubresource( pMaterialConstBuffer, 0, NULL, &cbMaterial, 0, 0 );
        pImmediateContext->PSSetConstantBuffers( 0, 1, &pMaterialConstBuffer );

        // set the textures
        if(m_pTextureResViews[CPUT_PHONG_MATERIAL_PROPERTY_DIFFUSE_TEXTURE])
        pImmediateContext->PSSetShaderResources( 0, 1, &m_pTextureResViews[CPUT_PHONG_MATERIAL_PROPERTY_DIFFUSE_TEXTURE] );
        if(m_pTextureResViews[CPUT_PHONG_MATERIAL_PROPERTY_NORMAL_TEXTURE])
        pImmediateContext->PSSetShaderResources( 1, 1, &m_pTextureResViews[CPUT_PHONG_MATERIAL_PROPERTY_NORMAL_TEXTURE] );
        */
        return CPUT_SUCCESS;
    }


    // set particular properties
    //-----------------------------------------------------------------------------
    CPUTResult CPUTMaterialPhong::SetProperty(const char* pName, CPUT_DATA_FORMAT_TYPE eFormatType, int FormatElementCount, const void* pData)
    {
        if(NULL==pName)
            return CPUT_ERROR_INVALID_PARAMETER;

        int index = FindPropertyName(pName);
        if(-1 == index)
        {
            return CPUT_ERROR_NOT_FOUND;
        }
        return SetProperty((CPUT_PHONG_MATERIAL_PROPERTY)index, eFormatType, FormatElementCount, pData);
    }

    // set particular properties
    //-----------------------------------------------------------------------------
    CPUTResult CPUTMaterialPhong::SetProperty(CPUT_PHONG_MATERIAL_PROPERTY eProperty, CPUT_DATA_FORMAT_TYPE eFormatType, int FormatElementCount, const void* pData)
    {
        return SetMaterialProperty(eProperty, eFormatType, FormatElementCount, pData);

    }

    // get particular properties
    //-----------------------------------------------------------------------------
    CPUTResult CPUTMaterialPhong::GetProperty(const char* pName, CPUT_DATA_FORMAT_TYPE& eFormatType, int& FormatElementCount, void** ppData)
    {
        if(NULL==pName)
            return CPUT_ERROR_INVALID_PARAMETER;

        int index = FindPropertyName(pName);
        return GetProperty((CPUT_PHONG_MATERIAL_PROPERTY)index, eFormatType, FormatElementCount, ppData);
    }

    // get particular properties
    //-----------------------------------------------------------------------------
    CPUTResult CPUTMaterialPhong::GetProperty(CPUT_PHONG_MATERIAL_PROPERTY eProperty, CPUT_DATA_FORMAT_TYPE& eFormatType, int& FormatElementCount, void** ppData)
    {    
        return GetMaterialProperty(eProperty, eFormatType, FormatElementCount, ppData);
    }



    // find a property name (char* to enum conversion)
    //-----------------------------------------------------------------------------
    int CPUTMaterialPhong::FindPropertyName(const char* pName)
    {
        for(int i=0; i<CPUT_PHONG_MATERIAL_PROPERTY_COUNT; i++)
        {
            if(0==strcmp(pName, l_pPhongMaterialAttributeNames[i]))
            {
                return i;
            }
        }
        return -1;
    }




    // textures
    // Set texture by attribute and file names
    //-----------------------------------------------------------------------------
    CPUTResult CPUTMaterialPhong::SetTexture(const char* pTextureAttributeName, const char* pFilename)
    {
        int index = FindTextureName(pTextureAttributeName);
        if(-1 == index)
        {
            return CPUT_ERROR_NOT_FOUND;
        }
        return SetTexture((CPUT_PHONG_MATERIAL_TEXTURES)index, pFilename);
    }

    // Set texture by attribute type and file name
    //-----------------------------------------------------------------------------
    CPUTResult CPUTMaterialPhong::SetTexture(CPUT_PHONG_MATERIAL_TEXTURES eTexture, const char* pFilename)
    {
        if(eTexture>=CPUT_PHONG_MATERIAL_TEXTURE_COUNT)
            return CPUT_ERROR_INVALID_PARAMETER;

        if(m_pTextureIDs[eTexture])
        {
            // delete any loaded textures
            // todo: is this right?  what if they're shared?
            // directx is refcounted, but we are not - refcount?
            glDeleteTextures(1, &m_pTextureIDs[eTexture]);            
            m_pTextureIDs[eTexture]=0;
        }

        GLuint width, height;
        return CPUTTextureLoader::GetTextureLoader()->LoadAndRegisterTexture( pFilename, m_pTextureIDs[eTexture], width, height, true, CPUT_PATH_SEARCH_NONE);        
    }

    // Set texture by attribute type and resource type
    //-----------------------------------------------------------------------------
    CPUTResult CPUTMaterialPhong::SetTexture(CPUT_PHONG_MATERIAL_TEXTURES eTexture, GLuint TextureID)
    {
        if(eTexture>=CPUT_PHONG_MATERIAL_TEXTURE_COUNT)
            return CPUT_ERROR_INVALID_PARAMETER;

        if(m_pTextureIDs[eTexture])
        {
            // delete any loaded textures
            // todo: is this right?  what if they're shared?
            // directx is refcounted, but we are not - refcount?
            glDeleteTextures(1, &m_pTextureIDs[eTexture]);            
            m_pTextureIDs[eTexture]=0;
        }

        m_pTextureIDs[eTexture] = TextureID;

        return CPUT_SUCCESS;
    }

    // Set texture by attribute type and file name
    //-----------------------------------------------------------------------------
    CPUTResult CPUTMaterialPhong::GetTexture(CPUT_PHONG_MATERIAL_TEXTURES eTexture, GLuint& TextureID)
    {
        if(eTexture>=CPUT_PHONG_MATERIAL_TEXTURE_COUNT)
            return CPUT_ERROR_INVALID_PARAMETER;

        TextureID = m_pTextureIDs[eTexture];
        return CPUT_SUCCESS;
    }

    // find a texture name (char* to enum conversion)
    //-----------------------------------------------------------------------------
    int CPUTMaterialPhong::FindTextureName(const char* pName)
    {
        for(int i=0; i<CPUT_PHONG_MATERIAL_TEXTURE_COUNT; i++)
        {
            if(0==strcmp(pName, l_pPhongMaterialTextureNames[i]))
            {
                return i;
            }
        }
        return -1;
    }

}


