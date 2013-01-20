#include "CPUTStaticOGL31.h"

#include <malloc.h> // for HEAPCHECK

namespace CPUTGL31
{
    // statics initialization
    CPUTFont CPUTStatic::m_FontID = 0;
    GLint CPUTStatic::m_ModelViewMatrixID = 0;

    // constructor
    //--------------------------------------------------------------------------------
    CPUTStatic::CPUTStatic():m_VertexStride(0),
        m_VertexOffset(0),
        m_VertexBuffer(0),
        m_TextureID(0)
    {
        m_pVertexBufferVBO[0]=0; m_pVertexBufferVBO[1]=0;
        m_QuadSize.height=0; m_QuadSize.width=0;
        m_Position.x=0; m_Position.y=0;

    }

    // constructor that takes a string
    //--------------------------------------------------------------------------------
    CPUTStatic::CPUTStatic(const cString ControlText, CPUTControlID id):CPUTStaticBase(ControlText, id),
        m_VertexStride(0),
        m_VertexOffset(0),
        m_TextureID(0)
    {
        m_QuadSize.height=0; m_QuadSize.width=0;
        m_Position.x=0; m_Position.y=0;  

        SetText(ControlText);

        // store the control id
        m_controlID = id;
    }

    // destructor
    //--------------------------------------------------------------------------------
    CPUTStatic::~CPUTStatic()
    {
        ReleaseInstanceData();
    }

    // Release all instance data
    //--------------------------------------------------------------------------------
    void CPUTStatic::ReleaseInstanceData()
    {
        // delete the VAO
        glDeleteVertexArrays(1, &m_VertexBuffer);
        m_VertexBuffer = 0;

        // delete the VBO's
        glDeleteBuffers(2, m_pVertexBufferVBO);
        m_pVertexBufferVBO[0]=0; m_pVertexBufferVBO[1]=0;

        // delete texture
        glDeleteTextures(1, &m_TextureID);
        m_TextureID = 0;
    }


    // Register all static assets (used by all CPUTStatic objects)
    //--------------------------------------------------------------------------------
    CPUTResult CPUTStatic::RegisterStaticResources(GLuint m_shaderID, GLuint modelViewMatrixID, const cString DefaultFontFilename)
    {
        CPUTResult result;

        if( 0==m_shaderID )
            return CPUT_ERROR_INVALID_PARAMETER;

        // load the font for static text
        CPUTFontLibrary* pFontLibrary = CPUTFontLibrary::GetFontLibrary();
        result = pFontLibrary->LoadFont(DefaultFontFilename, m_FontID);

        return result;

        /*
        // store the needed resource ID's
        m_ModelViewMatrixID = modelViewMatrixID;

        // append the default resource directory
        //char pFontFile[] = ".//resources//font12b";

        // build up the resource path+filename
        int dirLength = strlen(pResourcePath);

        int resourceLength = strlen(pFontFile);

        int BufferSize = dirLength + resourceLength + 1;
        char* pFontPath = new char[BufferSize];

        strncpy_s(pFontPath, BufferSize, pResourcePath, _TRUNCATE);
        strncat_s(pFontPath, BufferSize, pFontFile, _TRUNCATE);


        // load the font for static text
        CPUTFontLibrary* pFontLibrary = CPUTFontLibrary::GetFontLibrary();
        result = pFontLibrary->LoadFont(pFontPath, m_FontID);


        // get relevant GUI shader program variables
        m_ModelViewMatrixID = glGetUniformLocation(m_shaderID, "ModelViewMatrix");

        // cleanup
        delete [] pFontPath;

        return result;
        */
    }

    //
    //--------------------------------------------------------------------------------
    CPUTResult CPUTStatic::UnRegisterStaticResources()
    {
        // todo: anything?
        return CPUT_SUCCESS;
    }

    //
    //--------------------------------------------------------------------------------
    void CPUTStatic::Draw(HDC hdc)
    {
        // if control is set as non-visible, don't draw
        if(!m_ControlVisible)
            return;

        //bind the texture
        glBindTexture(GL_TEXTURE_2D, m_TextureID); 

        // bind the quad
        glBindVertexArray(m_VertexBuffer);     

        float location[] = { 1.0f, 0.0f, 0.0f, (float)m_Position.x, 
            0.0f, 1.0f, 0.0f, (float)m_Position.y, 
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f};      

        glUniformMatrix4fv(m_ModelViewMatrixID, 1, GL_TRUE, location); // GL_TRUE means transpose

        glDrawArrays(GL_TRIANGLES, 0, 6);


    }

    // Register quad for drawing string on
    //--------------------------------------------------------------------------------
    CPUTResult CPUTStatic::SetText(const cString String, float depth)
    {

        CPUTResult result;
        // todo: remove all these heap-checks
        HEAPCHECK;

        // retrieve the font for rendering static text
        CPUTLoadedFontItem* pFont=NULL;
        CPUTFontLibrary* pFontLibrary = CPUTFontLibrary::GetFontLibrary();
        result = pFontLibrary->GetFont(m_FontID, &pFont);
        if(FAILED(result))
            return result;

        HEAPCHECK;

        // release the texture and quad the old text was drawn on (if it exists)
        ReleaseInstanceData();   

        HEAPCHECK;

        // now generate the texture from that string with each character copied into it
        void* pData = NULL;
        result = GenerateStringTexture(String, m_QuadSize, &pData);
        if(FAILED(result))
            return result;

        HEAPCHECK;

        result = RegisterTexture(m_QuadSize.width, m_QuadSize.height, pData);
        delete [] pData;
        if(FAILED(result))
            return result;

        HEAPCHECK;    

        // generate a quad that size for drawing on
        result = RegisterQuad( m_QuadSize.width, m_QuadSize.height, depth, m_VertexBuffer, m_pVertexBufferVBO);
        if(FAILED(result))
            return result;

        HEAPCHECK;

        return CPUT_SUCCESS;
    }


    // Make a string object for drawing
    //--------------------------------------------------------------------------------
    CPUTResult CPUTStatic::GenerateStringTexture(const cString String, CPUT_SIZE& m_TextureSize, void** ppData )
    {
        if(NULL==ppData )
            return CPUT_ERROR_INVALID_PARAMETER;

        CPUTResult result;

        // retrieve the font for rendering static text
        CPUTLoadedFontItem* pFont=NULL;
        CPUTFontLibrary* pFontLibrary = CPUTFontLibrary::GetFontLibrary();
        result = pFontLibrary->GetFont(m_FontID, &pFont);
        if(FAILED(result))
            return result;

        // calculate the size of the data block needed to hold the text    
        result = pFont->GetStringDimensions(String, m_TextureSize);    
        if(CPUTFAILED(result))
            return result;

        // create the texture to hold all the characters
        char* pBuffer = new char[ m_TextureSize.width * m_TextureSize.height * 4 ];  // rgba 8,8,8,8 format
        char* pInsertAt = pBuffer;

        // return the new texture for binding
        *ppData = pBuffer;

        HEAPCHECK;

        // walk list of characterrs,compositing them into a master buffer as you go
        int StringLength = (int) String.size();
        CPUT_SIZE SpaceSize;
        pFont->GetSpaceDimenions(SpaceSize);

        for(int i=0; i<StringLength; i++)
        {
            wchar_t c = String[i];
            int glyphIndex = 0;        

            // spaces are special blocks of alpha = 0 spaces
            if( ' ' == c)
            {            
                char* pBufferIndex = pInsertAt;           

                for(int y=0; y<SpaceSize.height; y++)
                {
                    for(int x=0; x<SpaceSize.width; x++)
                    {
                        //if(x<m_glyphSize[glyphIndex].width)
                        //{
                        // copy the glyph into the output buffer at the right location                
                        pBufferIndex[0] = 0x0;
                        pBufferIndex[1] = 0x0;
                        pBufferIndex[2] = 0x0;
                        pBufferIndex[3] = 0x0; //(char)255;
                        pBufferIndex+=4;               
                    }
                    // skip to next line minus size of character
                    pBufferIndex += (m_TextureSize.width - SpaceSize.width)*4;  

                }
                // step to next glyph position in output buffer
                pInsertAt += (SpaceSize.width * 4);               
            }
            else
            {
                // get the glyph from the character map
                char* pGlyphIndex=NULL;
                CPUT_SIZE GlyphSize;

                pFont->GetGlyphData(c, GlyphSize, (void**)&pGlyphIndex);

                char* pBufferIndex = pInsertAt;
                if(NULL == pGlyphIndex)
                {
                    return CPUT_ERROR_ERROR_LOADING_IMAGE;
                }

                for(int y=0; y<GlyphSize.height; y++)
                {               
                    for(int x=0; x<GlyphSize.width; x++)
                    {
                        // copy the glyph into the output buffer at the right location      

                        pBufferIndex[0] = pGlyphIndex[0];
                        pBufferIndex[1] = pGlyphIndex[1];
                        pBufferIndex[2] = pGlyphIndex[2];
                        pBufferIndex[3] = pGlyphIndex[3];

                        pGlyphIndex+=4; // the glyph is contiguous in memory, no need to skip lines
                        pBufferIndex+=4;               
                    }
                    // skip to next line minus size of character
                    pBufferIndex += (m_TextureSize.width - GlyphSize.width)*4;  

                }
                // step to next glyph position in output buffer
                pInsertAt += (GlyphSize.width * 4);       
            }
        }       

        return CPUT_SUCCESS;
    }



    // register a quad and return the id of the object
    //--------------------------------------------------------------------------------
    CPUTResult CPUTStatic::RegisterQuad(int width, int height, float depth, GLuint& objectID, GLuint* pVertexBufferObjectIDList)
    {
        CPUTResult result = CPUT_SUCCESS;

        // register the 9 quads we'll want to draw
        struct Point3 { float m[3];  Point3(float x, float y, float z) {m[0]=x;m[1]=y;m[2]=z;} };
        struct Point2 { float m[2];  Point2(float x, float y) {m[0]=x;m[1]=y;} };
        float scale = 1.0f;

        float imageHeight = (float) height;
        float imageWidth = (float) width;

        Point3 Pos[] = 
        {
            Point3(0.0f, 0.0f, depth),                
            Point3(imageWidth, 0.0f, depth),
            Point3(0.0f, imageHeight, depth),

            Point3(imageWidth, imageHeight, depth),
            Point3(0.0f, imageHeight, depth),
            Point3(imageWidth, 0.0f, depth),
        };

        Point2 UV[] =
        {
            Point2(0.0f, 0.0f),
            Point2(1.0f, 0.0f),
            Point2(0.0f, 1.0f), 

            Point2(1.0f, 1.0f),
            Point2(0.0f, 1.0f),
            Point2(1.0f, 0.0f),
        };

        // generate 1 VAO
        glGenVertexArrays(1, &objectID);

        // First VAO setup
        glBindVertexArray(objectID);

        // generate 3 'stream' slots
        //GLuint pVertexBufferObjectID[2];
        //m_pIdleVertexBufferObjectIDs[m_pIdleVertexBufferObjectIDCount];

        //glGenBuffers(2, &m_pIdleVertexBufferObjectIDs[m_pIdleVertexBufferObjectIDCount]);
        glGenBuffers(2, pVertexBufferObjectIDList);

        // slot #0 - 3-float positions
        glBindBuffer(GL_ARRAY_BUFFER, pVertexBufferObjectIDList[0]);
        glBufferData(GL_ARRAY_BUFFER, 6*3*sizeof(GLfloat), Pos, GL_STATIC_DRAW);
        glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0); 
        glEnableVertexAttribArray(0);

        // slot #1 - 2-float uv coordinates
        glBindBuffer(GL_ARRAY_BUFFER, pVertexBufferObjectIDList[1]);
        glBufferData(GL_ARRAY_BUFFER, 6*2*sizeof(GLfloat), UV, GL_STATIC_DRAW);
        glVertexAttribPointer((GLuint)1, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(1);


        /*
        // slot #0 - 3-float positions
        glBindBuffer(GL_ARRAY_BUFFER, m_pIdleVertexBufferObjectIDs[m_pIdleVertexBufferObjectIDCount]);
        glBufferData(GL_ARRAY_BUFFER, 6*3*sizeof(GLfloat), Pos, GL_STATIC_DRAW);
        glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0); 
        glEnableVertexAttribArray(0);

        // slot #1 - 2-float uv coordinates
        glBindBuffer(GL_ARRAY_BUFFER, m_pIdleVertexBufferObjectIDs[m_pIdleVertexBufferObjectIDCount+1]);
        glBufferData(GL_ARRAY_BUFFER, 6*2*sizeof(GLfloat), UV, GL_STATIC_DRAW);
        glVertexAttribPointer((GLuint)1, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(1);

        m_pIdleVertexBufferObjectIDCount+=2;
        */     
        glBindVertexArray(0);

        return result;
    }


    // Register texture with the string data in it
    //--------------------------------------------------------------------------------
    CPUTResult CPUTStatic::RegisterTexture( int width, int height, void* pData)
    {

        GLint iCurrentlyBoundTexture = 0;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &iCurrentlyBoundTexture);

        // found and loaded - register texture with GL
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        // while registering, save the previously bound texture
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &iCurrentlyBoundTexture);

        // generate a new GL textureID
        glGenTextures(1, &m_TextureID);

        // Set parameters
        glBindTexture(GL_TEXTURE_2D, m_TextureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

        // bind the data to the texture ID
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pData);

        // restore the previous texture binding (so the user is none the wiser)
        glBindTexture(GL_TEXTURE_2D, iCurrentlyBoundTexture);


        return CPUT_SUCCESS;
    }

}