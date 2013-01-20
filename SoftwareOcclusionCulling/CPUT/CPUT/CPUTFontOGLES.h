// INTEL SPECIFICALLY DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, AND ALL LIABILITY,
// INCLUDING CONSEQUENTIAL AND OTHER INDIRECT DAMAGES, FOR THE USE OF THIS SOFTWARE,
// INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PROPRIETARY RIGHTS, AND INCLUDING THE
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  Intel does not
// assume any responsibility for any errors which may appear in this software nor any
// responsibility to update it.
//--------------------------------------------------------------------------------------
#ifndef __CPUTFONTOGLES_H__
#define __CPUTFONTOGLES_H__

#include "CPUT.h"
#include "CPUTRefCount.h"
#include "CPUTFont.h"

// gl includes
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

class CPUTTextureOGLES;

class CPUTFontOGLES : public CPUTFont
{
protected:
    ~CPUTFontOGLES(); // Destructor is not public.  Must release instead of delete.

public:
    CPUTFontOGLES();
    static CPUTFont *CreateFont( cString FontName, cString AbsolutePathAndFilename);

    CPUTTextureOGLES* GetAtlasTexture();
    GLuint GetAtlasTextureResourceView();
    //ID3D11ShaderResourceView* GetAtlasTextureResourceView();


private:
    CPUTTextureOGLES*            m_pTextAtlas;
    GLuint                       m_TextAtlasResource;
    //ID3D11ShaderResourceView*   m_pTextAtlasView;
    
    CPUTResult LoadGlyphMappingFile(const cString fileName);
};

#endif // #ifndef __CPUTFONTOGLES_H__