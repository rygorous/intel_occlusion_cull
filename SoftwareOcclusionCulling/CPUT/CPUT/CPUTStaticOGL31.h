#ifndef __CPUTSTATICOGL31_H__
#define __CPUTSTATICOGL31_H__

#include "CPUTBase.h"
#include "CPUTStaticBase.h"
#include "CPUTFontLibraryOGL31.h"

#include <gl\GL.h>

namespace CPUTGL31
{
    // DX11 Static text control
    //--------------------------------------------------------------------------------
    class CPUTStatic:public CPUTStaticBase
    {
    public:
        CPUTStatic();
        CPUTStatic(const cString String, CPUTControlID id);
        ~CPUTStatic();

        // CPUTControl
        void SetEnable(bool in_bEnabled) {;} // IMPL!
        bool IsEnabled() {return false;}// IMPL!
        bool ContainsPoint(int x, int y) {return false;} //IMPL!

        CPUTResult SetText(const cString String, float depth=0.5f);
        void Draw(HDC hdc);

        // Register assets
        static CPUTResult RegisterStaticResources(GLuint m_shaderID, GLuint modelViewMatrixID, const cString DefaultFontFilename);
        static CPUTResult UnRegisterStaticResources();


    private:
        // static variables
        static CPUTFont m_FontID;
        static GLint m_ModelViewMatrixID;
        // instance variables

        //CPUT_SIZE m_TextLocation;
        UINT m_VertexStride;
        UINT m_VertexOffset;
        GLuint m_pVertexBufferVBO[2];
        GLuint m_VertexBuffer; // vertex array object
        GLuint m_TextureID;    


        // helper functions
        void ReleaseInstanceData();
        CPUTResult GenerateStringTexture(const cString String, CPUT_SIZE& m_TextureSize, void** ppData);
        CPUTResult RegisterQuad(int width, int height, float depth, GLuint& objectID, GLuint* VertexBufferObjectIDList);
        CPUTResult RegisterTexture( int width, int height, void* pData);

    };
}

#endif //__CPUTSTATICOGL31_H__