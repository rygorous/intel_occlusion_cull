#ifndef __CPUTTEXTURELOADEROGL31_H__
#define __CPUTTEXTURELOADEROGL31_H__

#include "CPUTTextureLoaderBase.h"
#include "CPUTTextureLoaderOGL31.h"
#include "CPUTServicesWin.h"

#include "png.h"    // libPNG library

#include <gl\glew.h>
#include <vector> // for std::vector


class CPUTTextureLoader:public CPUTTextureLoaderBase
{
public:
    static CPUTTextureLoader* GetTextureLoader();
    static CPUTResult DeleteTextureLoader(); 

    CPUTResult LoadTexture(const cString Filename, unsigned int& width, unsigned int& height, CPUT_TEXTURE_FORMAT& eFormat, void** ppImageData, CPUT_PATH_SEARCH_MODE eSearchmode=CPUT_PATH_SEARCH_MEDIA_DIRECTORY);
    CPUTResult LoadAndRegisterTexture(const cString Filename, GLuint& textureID, GLuint& width, GLuint& height, bool LoadAndRegister=true, CPUT_PATH_SEARCH_MODE eSearchmode=CPUT_PATH_SEARCH_MEDIA_DIRECTORY);

    // This will remove a texture from the 'shared' list (if it had been registered with that list) 
    // allowing you to delete the ID3D11ShaderResourceView    
    CPUTResult RemoveTextureFromShareList(GLuint textureID);    
    CPUTResult RemoveTextureFromShareList(const cString Filename);

private:
    CPUTTextureLoader(); // singleton constructor 
    ~CPUTTextureLoader();

    static CPUTTextureLoader* m_pTextureLoader;
    struct TextureInfoNode
    {
        cString Filename;
        GLuint TextureResourceID;
    };
    std::vector<TextureInfoNode*> m_LoadedTextures;

    unsigned int ConvertCPUTTypeToGLType(CPUT_TEXTURE_FORMAT eFormat);
    CPUTResult LoadPNG(const cString Filename, unsigned int& width, unsigned int& height, CPUT_TEXTURE_FORMAT& eFormat, void** ppData);
};



#endif //#ifndef __CPUTTEXTURELOADEROGL31_H__