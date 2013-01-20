#ifndef __CPUTMATERIALPHONGGL31_H__
#define __CPUTMATERIALPHONGGL31_H__

#include "CPUTMaterialGL31.h"
#include "CPUTTextureLoaderOGL31.h"
#include "CPUTServicesWin.h"
#include <memory.h>

namespace CPUTGL31
{
#define CPUT_PHONG_MATERIAL_PROPERTY_COUNT 18
    enum CPUT_PHONG_MATERIAL_PROPERTY
    {
        // color/factor pairs (3xF32, 1xF32) for now
        // as hard-coded in ron's CPRT defs
        CPUT_PHONG_MATERIAL_PROPERTY_DIFFUSE_COLOR=0,
        CPUT_PHONG_MATERIAL_PROPERTY_DIFFUSE_FACTOR=1,
        CPUT_PHONG_MATERIAL_PROPERTY_AMBIENT_COLOR=2,
        CPUT_PHONG_MATERIAL_PROPERTY_AMBIENT_FACTOR=3,
        CPUT_PHONG_MATERIAL_PROPERTY_SPECULAR_COLOR=4,
        CPUT_PHONG_MATERIAL_PROPERTY_SPECULAR_FACTOR=5,
        CPUT_PHONG_MATERIAL_PROPERTY_REFLECTIVE_COLOR=6,
        CPUT_PHONG_MATERIAL_PROPERTY_REFLECTIVE_FACTOR=7,
        CPUT_PHONG_MATERIAL_PROPERTY_EMISSIVE_COLOR=8,
        CPUT_PHONG_MATERIAL_PROPERTY_EMISSIVE_FACTOR=9,
        CPUT_PHONG_MATERIAL_PROPERTY_TRANSPARENT_COLOR=10,
        CPUT_PHONG_MATERIAL_PROPERTY_TRANSPARENT_FACTOR=11,
        CPUT_PHONG_MATERIAL_PROPERTY_DISPLACEMENT_COLOR=12,
        CPUT_PHONG_MATERIAL_PROPERTY_DISPLACEMENT_FACTOR=13,

        CPUT_PHONG_MATERIAL_PROPERTY_SHININESS_FACTOR=14,

        CPUT_PHONG_MATERIAL_PROPERTY_BUMP=15,      //3XF32 - what does this mean?
        CPUT_PHONG_MATERIAL_PROPERTY_BUMP_FACTOR=16,   //1XF32
        CPUT_PHONG_MATERIAL_PROPERTY_NORMAL_MAP=17,    //3xF32 - what does this mean?
    };

#define CPUT_PHONG_MATERIAL_TEXTURE_COUNT 4
    enum CPUT_PHONG_MATERIAL_TEXTURES
    {
        CPUT_PHONG_MATERIAL_PROPERTY_DIFFUSE_TEXTURE=0,
        CPUT_PHONG_MATERIAL_PROPERTY_AMBIENT_TEXTURE=1,
        CPUT_PHONG_MATERIAL_PROPERTY_SPECULAR_TEXTURE=2,
        CPUT_PHONG_MATERIAL_PROPERTY_NORMAL_TEXTURE=3,

    };



    class CPUTMaterialPhong:public CPUTMaterial
    {
    public:    
        CPUTMaterialPhong();
        ~CPUTMaterialPhong();

        // set/get particular properties
        CPUTResult SetProperty(const char* pName, CPUT_DATA_FORMAT_TYPE eFormatType, int FormatElementCount, const void* pData);
        CPUTResult SetProperty(CPUT_PHONG_MATERIAL_PROPERTY eProperty, CPUT_DATA_FORMAT_TYPE eFormatType, int FormatElementCount, const void* pData);
        CPUTResult GetProperty(const char* pName, CPUT_DATA_FORMAT_TYPE& eFormatType, int& FormatElementCount, void** ppData);
        CPUTResult GetProperty(CPUT_PHONG_MATERIAL_PROPERTY eProperty, CPUT_DATA_FORMAT_TYPE& eFormatType, int& FormatElementCount, void** ppData);

        // textures 
        CPUTResult SetTexture(const char* pTextureAttributeName, const char* pFilename);
        CPUTResult SetTexture(CPUT_PHONG_MATERIAL_TEXTURES eTexture, const char* pFilename);
        CPUTResult SetTexture(CPUT_PHONG_MATERIAL_TEXTURES eTexture, GLuint TextureID);
        CPUTResult GetTexture(CPUT_PHONG_MATERIAL_TEXTURES eTexture, GLuint& TextureID);

        // assign all these parameters to the default-supplied shader
        CPUTResult BindToDefaultShader( GLuint ShaderProgramID );



    private:
        void DeleteResources();
        int FindPropertyName(const char* pName);
        int FindTextureName(const char* pName);
        // todo: this is mirror of struct held somewhere else, find it and
        // combine them into a single header somewhere

        /*
        struct MaterialConstantBuffer
        {
        XMFLOAT4 ambientColor;       
        XMFLOAT4 diffuseColor;
        FLOAT ambientFactor;
        FLOAT diffuseFactor;
        FLOAT fill1;
        FLOAT fill2;
        };

        ID3D11ShaderResourceView* m_pTextureResViews[CPUT_PHONG_MATERIAL_TEXTURE_COUNT];

        ID3D11ShaderResourceView* m_pDiffuseTextureResView;
        ID3D11ShaderResourceView* m_pAmbientTextureResView;
        ID3D11ShaderResourceView* m_pSpecularTextureResView;
        ID3D11ShaderResourceView* m_pNormalMapTextureResView;
        */
        GLuint m_pTextureIDs[CPUT_PHONG_MATERIAL_TEXTURE_COUNT];
    };
}
#endif // #ifndef __CPUTMATERIALPHONGGL31_H__