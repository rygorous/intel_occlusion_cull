#ifndef __CPUTSHADERLOADEROGL31_H__
#define __CPUTSHADERLOADEROGL31_H__

#include "CPUTBase.h"
#include "CPUTServicesWin.h"

#include <string.h> // strnlen_s
#include <gl/glew.h>

namespace CPUTGL31
{
    class CPUTShaderLoader
    {
    public:
        static CPUTShaderLoader* CPUTShaderLoader::GetShaderLoader();
        CPUTResult CompileShaderFromFile(const cString Filename, CPUT_SHADER_TYPE shadertype, GLuint& ShaderID, CPUT_PATH_SEARCH_MODE eSearchmode=CPUT_PATH_SEARCH_MEDIA_DIRECTORY );
        CPUTResult CheckShaderCompileLog(GLuint shaderID, cString& Message);
        CPUTResult Initialize();

    private:
        static CPUTShaderLoader* m_pShaderLoader;
        CPUTResult LoadStringTermintatedFile(const cString Filename, void** ppData);

    };
}
#endif // #ifndef __CPUTSHADERLOADEROGL31_H__