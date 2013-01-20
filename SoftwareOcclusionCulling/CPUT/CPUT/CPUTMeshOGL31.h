#ifndef __CPUTMESHOGL31_H__
#define __CPUTMESHOGL31_H__
#include <stdio.h>
#include "CPUTMeshBase.h"
#include "CPUTMeshStreamUniformGL31.h"

#include <gl/glew.h>

namespace CPUTGL31
{
    class CPUTMesh:public CPUTMeshBase
    {
    public:
        CPUTMesh();
        ~CPUTMesh();

        CPUTResult Register(void** ppLayout);
        CPUTResult UnRegister();

        void Draw();

    private:
        GLuint  m_VertexArrayObjectID;
        GLuint* m_pVertexBufferObjectIDs;
        GLuint m_NumberOfVerticies;

        // helper functions    
        GLenum ConvertCPUTFormatToGL(CPUT_DATA_FORMAT_TYPE cputType);
    };
}

#endif//#ifndef __CPUTMESHOGL31_H__