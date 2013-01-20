#include "CPUTMeshOGL31.h"


namespace CPUTGL31
{
    // Constructor
    //-----------------------------------------------------------------------------
    CPUTMesh::CPUTMesh():m_VertexArrayObjectID(0),
        m_NumberOfVerticies(0),
        m_pVertexBufferObjectIDs(NULL)
    {
    }

    // Destructor
    //-----------------------------------------------------------------------------
    CPUTMesh::~CPUTMesh()
    {
        // un-register the streams of this mesh from OpenGL
        UnRegister();

        if(m_pVertexBufferObjectIDs)
        {
            delete [] m_pVertexBufferObjectIDs;
            m_pVertexBufferObjectIDs = NULL;
        }    
    }

    // Unregister all registered meshes
    //-----------------------------------------------------------------------------
    CPUTResult CPUTMesh::UnRegister()
    {
        int NumberOfVertexStreamsInThisMesh = GetVertexStreamCount();
        int NumberOfTotalStreamsInThisMesh=NumberOfVertexStreamsInThisMesh;

        // if indexed, we need one additional vbo
        if(IsIndexedMesh())
            NumberOfTotalStreamsInThisMesh++;

        // delete the single VAO (vertex array object) that holds all the VBO's
        glDeleteVertexArrays(1, &m_VertexArrayObjectID);
        m_VertexArrayObjectID = 0;

        // delete the VBO's (vertex buffer objects) - one for each stream +1 for index (if it exists)
        if(m_pVertexBufferObjectIDs)
        {
            glDeleteBuffers(NumberOfTotalStreamsInThisMesh, m_pVertexBufferObjectIDs);
            for(int i=0; i<NumberOfTotalStreamsInThisMesh; i++)
            {
                m_pVertexBufferObjectIDs[i] = 0;
            }
        }
        return CPUT_SUCCESS;
    }

    // Register the buffer with OpenGL
    //-----------------------------------------------------------------------------
    CPUTResult CPUTMesh::Register(void** ppLayout)
    {
        CPUTResult result;
        result = UnRegister();
        if(CPUTFAILED(result))
            return result;

        int NumberOfVertexStreamsInThisMesh = GetVertexStreamCount();
        int NumberOfTotalStreamsInThisMesh=NumberOfVertexStreamsInThisMesh;

        // if indexed, we need one additional vbo
        if(IsIndexedMesh())
            NumberOfTotalStreamsInThisMesh++;

        // allocate the variables we'll need for this operation
        //m_ppDXBufferObjects = new ID3D11Buffer*[NumberOfVertexStreamsInThisMesh];
        //D3D11_INPUT_ELEMENT_DESC* layout = new D3D11_INPUT_ELEMENT_DESC[ NumberOfVertexStreamsInThisMesh ];
        //m_pStreamStrides = new UINT[NumberOfVertexStreamsInThisMesh];
        //m_pStreamOffsets = new UINT[NumberOfVertexStreamsInThisMesh];

        // generate 1 VAO (vertex array object) to hold all these streams
        glGenVertexArrays(1, &m_VertexArrayObjectID);

        // Bind VAO so you can 
        glBindVertexArray(m_VertexArrayObjectID);

        // generate 'stream' slots
        m_pVertexBufferObjectIDs = new GLuint[NumberOfTotalStreamsInThisMesh];   

        glGenBuffers(NumberOfTotalStreamsInThisMesh, m_pVertexBufferObjectIDs);


        for(int j=0; j<NumberOfVertexStreamsInThisMesh; j++)
        {
            // register each stream of each mesh, storing it's DX object pointer
            CPUTMeshStream* stream = GetVertexStream(j);

            if(CPUT_STREAM_ELEMENT_LAYOUT_UNIFORM == stream->GetStreamElementLayoutType())
            {
                // non-indexed
                CPUTMeshStreamUniform* uniformStream = (CPUTMeshStreamUniform*) stream;

                // deterimine the number of components per generic vertex attribute. Must be 1, 2, 3, or 4.  


                // bind this stream's data
                GLenum glDataType;
                void* pData = NULL;
                glDataType = ConvertCPUTFormatToGL(uniformStream->GetDataFormatElementType());
                glBindBuffer(GL_ARRAY_BUFFER, m_pVertexBufferObjectIDs[j]);
                uniformStream->Lock(&pData);
                glBufferData(GL_ARRAY_BUFFER, uniformStream->GetDataElementBlockSize() * uniformStream->GetNumberVerticies(), pData, GL_STATIC_DRAW);
                uniformStream->Unlock();
                glVertexAttribPointer((GLuint)j, uniformStream->GetNumberDataFormatElements(), glDataType, GL_FALSE, 0, 0); 
                glEnableVertexAttribArray(j); // slot number

                // Store the number of verticies
                // TODO: BUG - what if there diff number of elements/stream?
                m_NumberOfVerticies = uniformStream->GetNumberVerticies();

            } 
            else if(CPUT_STREAM_ELEMENT_LAYOUT_FLEXIBLE == stream->GetStreamElementLayoutType())
            {
                // TODO: fill this out with flexible streams
            }
            else
            {
                // TODO: ERROR
            }
        }

        // register any index stream (if there is one)
        if(IsIndexedMesh())
        {
            CPUTMeshStream* stream = GetIndexStream();

            if(CPUT_STREAM_ELEMENT_LAYOUT_UNIFORM == stream->GetStreamElementLayoutType())
            {
                void* pData = NULL;
                CPUTMeshStreamUniform* uniformStream = (CPUTMeshStreamUniform*) stream;

                // bind index stream and fill data
                // last m_pVertexBufferObjectID[] is always the Index buffer (if there is one)
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_pVertexBufferObjectIDs[NumberOfTotalStreamsInThisMesh-1]);
                uniformStream->Lock(&pData);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, uniformStream->GetDataElementBlockSize() * uniformStream->GetNumberVerticies(), pData, GL_STATIC_DRAW);
                uniformStream->Unlock();
                // this is an indexed mesh, so draw as such
                m_MeshDrawingType = CPUT_STREAM_TYPE_INDEX;
                m_NumberOfVerticies = uniformStream->GetNumberVerticies();
            }
        }

        // unbind what we were doing
        // todo: would be better to restore the user's state instead of just blast over it with 0
        glBindVertexArray(0);

        return CPUT_SUCCESS;
    }

    // convert from CPUT data type into the equivalent OpenGL type
    //-----------------------------------------------------------------------------
    GLenum CPUTMesh::ConvertCPUTFormatToGL(CPUT_DATA_FORMAT_TYPE cputType)
    {
        switch(cputType)
        {
        case CPUT_F32:
            return GL_FLOAT;

        case CPUT_U64:
        case CPUT_I64:
            return GL_DOUBLE;

        case CPUT_U32:
            return GL_UNSIGNED_INT;
        case CPUT_I32:
            return GL_INT;

        case CPUT_U16:  
            return GL_UNSIGNED_SHORT;
        case CPUT_I16:
            return GL_SHORT;

        case CPUT_U8:
            return GL_UNSIGNED_BYTE;
        case CPUT_I8:
            return GL_BYTE;
        }

        // not sure - default to OpenGL default
        return GL_FLOAT;
    }


    // OpenGL-specific draw function
    //-----------------------------------------------------------------------------
    void CPUTMesh::Draw()
    {
        // is there anything to draw?  Did you register() your buffers before
        // calling draw?
        if(0==m_VertexArrayObjectID)
            return;

        glBindVertexArray(m_VertexArrayObjectID);

        // draw
        if( CPUT_STREAM_TYPE_VERTEX == m_MeshDrawingType)
        {
            glDrawArrays(GL_TRIANGLES, 0, m_NumberOfVerticies);
        }
        else if( CPUT_STREAM_TYPE_INDEX == m_MeshDrawingType)
        {
            //glDrawArrays(GL_TRIANGLES, 0, m_NumberOfVerticies);
            glDrawElements(GL_TRIANGLES, m_NumberOfVerticies, GL_UNSIGNED_INT, 0);
        }
        glBindVertexArray(0);
    }

}