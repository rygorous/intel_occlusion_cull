#include "CPUTCheckboxOGL31.h"

namespace CPUTGL31
{
    // initialize statics
    GLuint CPUTCheckbox::m_shaderProgram = 0;
    GLint CPUTCheckbox::m_ModelViewMatrixID = 0;
    GLint CPUTCheckbox::m_textureUniformLocation = 0;
    GLuint CPUTCheckbox::m_pCheckboxTextureList[CPUT_NUM_IMAGES_IN_CHECKBOX_SET] = { 0,0,0 };
    CPUT_SIZE CPUTCheckbox::m_pCheckboxTextureSizeList[CPUT_NUM_IMAGES_IN_CHECKBOX_SET] = { {0,0},{0,0},{0,0} };


    // constructor
    //------------------------------------------------------------------------------
    CPUTCheckbox::CPUTCheckbox(const cString ControlText, CPUTControlID id):m_pCheckboxText(NULL),
        m_VertexArrayObjectID(0)
    {
        m_pVertexBufferObjectIDList[0] = 0;
        m_pVertexBufferObjectIDList[1] = 0;

        RegisterInstanceResources();

        m_controlID = id;

        SetText(ControlText);
    }

    // destructor
    //------------------------------------------------------------------------------
    CPUTCheckbox::~CPUTCheckbox()
    {
        UnRegisterInstanceResources();
    }


    // load the images needed to draw the checkbox states
    //------------------------------------------------------------------------------
    CPUTResult CPUTCheckbox::RegisterStaticResources(GLuint m_shaderID, GLuint modelViewMatrixID)
    {
        const int NumberOfImages = CPUT_NUM_IMAGES_IN_CHECKBOX_SET;

        // store all required resource ID's
        m_ModelViewMatrixID = modelViewMatrixID;

        // load all the images associated with checkboxes
        cString pImageFilenames[NumberOfImages] = 
        {        
            _L(".//controls//checkbox//checkbox.png"),
            _L(".//controls//checkbox//checkbox-checked.png"),
            _L(".//controls//checkbox//checkbox-disabled.png"),
        };

        // load each of the images needed for the control
        CPUTResult result;
        CPUTTextureLoader* pTextureLoader = CPUTTextureLoader::GetTextureLoader();
        for(int i=0; i<NumberOfImages; i++)
        {
            result = pTextureLoader->LoadAndRegisterTexture(pImageFilenames[i], m_pCheckboxTextureList[i], (GLuint&)m_pCheckboxTextureSizeList[i].width, (GLuint&)m_pCheckboxTextureSizeList[i].height, true, CPUT_PATH_SEARCH_RESOURCE_DIRECTORY);
            if(CPUTFAILED(result))
            {
                return result;
            }
        }

        return CPUT_SUCCESS;
    }

    //------------------------------------------------------------------------------
    CPUTResult CPUTCheckbox::UnRegisterStaticResources()
    {
        const int NumberOfImages = CPUT_NUM_IMAGES_IN_CHECKBOX_SET;

        // unregister+delete the textures
        glDeleteTextures( NumberOfImages, m_pCheckboxTextureList );


        return CPUT_SUCCESS;
    }

    //------------------------------------------------------------------------------
    CPUTResult CPUTCheckbox::RegisterInstanceResources()
    {
        CPUTResult result;

        // todo: this assumes that the quad holding the checkbox icon is the same dimension for active/pressed/inactive
        result = RegisterQuad(m_pCheckboxTextureSizeList[0].width, m_pCheckboxTextureSizeList[0].height, m_VertexArrayObjectID, m_pVertexBufferObjectIDList);

        return CPUT_SUCCESS;
    }

    //------------------------------------------------------------------------------
    CPUTResult CPUTCheckbox::UnRegisterInstanceResources()
    {
        if(NULL!=m_pCheckboxText)
        {
            delete m_pCheckboxText;
            m_pCheckboxText = NULL;
        }

        // delete the VAO
        glDeleteVertexArrays(1, &m_VertexArrayObjectID);
        m_VertexArrayObjectID = 0;

        // delete the VBO's
        glDeleteBuffers(2, m_pVertexBufferObjectIDList);
        m_pVertexBufferObjectIDList[0]=0; m_pVertexBufferObjectIDList[1]=0;


        return CPUT_SUCCESS;
    }


    //CPUTEventHandler
    // Handle keyboard events - none for this control
    //--------------------------------------------------------------------------------
    CPUTEventHandledCode CPUTCheckbox::OnKeyboardEvent(CPUTKey key)
    {
        return CPUT_EVENT_UNHANDLED;
    }

    // Handle mouse events
    //-----------------------------------------------------------------------------
    CPUTEventHandledCode CPUTCheckbox::OnMouseEvent(int x, int y, int wheel, CPUTMouseState state)
    {
        if( ContainsPoint(x,y) )
        {
            if(state & CPUT_MOUSE_LEFT_DOWN ) // clicked
            {
                // checkbox clicked!
                // #1 - change state
                if(CPUT_CONTROL_UNCHECKED == m_eCheckboxState)
                    m_eCheckboxState = CPUT_CONTROL_CHECKED;
                else if(CPUT_CONTROL_CHECKED == m_eCheckboxState)
                    m_eCheckboxState = CPUT_CONTROL_UNCHECKED;

                // #2 - trigger the user's callback
                m_pCallbackHandler->OnCallbackEvent(1, m_controlID, (CPUTControl*) this);
            }
            return CPUT_EVENT_HANDLED;
        }
        return CPUT_EVENT_UNHANDLED;
    }

    //CPUTControl    
    // set the upper-left position of the checkbox control (screen space coords)
    //-----------------------------------------------------------------------------
    void CPUTCheckbox::SetPosition(int x, int y)
    {
        m_ControlDimensions.x = x;
        m_ControlDimensions.y = y;

        // move the static text along with the bitmap graphic
        int textX, textY;
        GetTextPosition(textX, textY);
        m_pCheckboxText->SetPosition(textX, textY);
    }

    //-----------------------------------------------------------------------------
    void CPUTCheckbox::GetDimensions(int& width, int& height)
    {
        CalculateBounds(width, height);
    }

    // Does the x,y coordinate fall in the control's region?
    //-----------------------------------------------------------------------------
    bool CPUTCheckbox::ContainsPoint(int x, int y)
    {
        int width, height;
        CalculateBounds(width, height);
        if( (x>m_ControlDimensions.x) && (y>m_ControlDimensions.y) &&
            (x< (m_ControlDimensions.x+width)) && (y< (m_ControlDimensions.y+height))
            )
        {
            return true;
        }

        return false;
    }

    // calculate the bounding rectangle
    //-----------------------------------------------------------------------------
    void CPUTCheckbox::CalculateBounds(int& width, int& height)
    {
        int textX, textY;
        int textWidth, textHeight;

        GetTextPosition(textX, textY);

        m_pCheckboxText->GetDimensions(textWidth, textHeight);

        width = (textX - m_ControlDimensions.x ) + textWidth;
        height = textHeight;

        if(m_pCheckboxTextureSizeList[0].height > textHeight)
            height = m_pCheckboxTextureSizeList[0].height;
    }

    // set the text on the control
    //--------------------------------------------------------------------------------
    void CPUTCheckbox::SetText(const cString String)
    {
        // create the static text object if it doesn't exist
        if(NULL == m_pCheckboxText)
        {
            m_pCheckboxText = new CPUTStatic();
        }

        // set the Static control's text
        m_pCheckboxText->SetText(String);


        // move the text to the right spot
        int x,y;
        GetTextPosition(x,y);
        m_pCheckboxText->SetPosition(x, y);
    }



    // helper function used to help determine where to place the text in relation
    // to the graphic
    //--------------------------------------------------------------------------------
    void CPUTCheckbox::GetTextPosition(int& x, int& y)
    {
        // get the dimensions of the string in pixels
        CPUT_RECT TextRect;
        m_pCheckboxText->GetDimensions(TextRect.width, TextRect.height);

        // calculate a good spot for the text to be in relation to the checkbox bitmap
        x = m_ControlDimensions.x + m_pCheckboxTextureSizeList[0].width + 5; // move right far enough not to overlap the bitmap    
        y = m_ControlDimensions.y + m_pCheckboxTextureSizeList[0].height - TextRect.height;  // try to center text top-to-bottom
    }


    // draw yourself
    //--------------------------------------------------------------------------------
    void CPUTCheckbox::Draw(HDC hdc) 
    {
        // if control is set as non-visible, don't draw
        if(!m_ControlVisible)
            return;

        //bind the texture
        glBindTexture(GL_TEXTURE_2D, m_pCheckboxTextureList[m_eCheckboxState]); 

        // bind the quad to draw on
        glBindVertexArray(m_VertexArrayObjectID);

        // set the control location
        float location[] = { 1.0f, 0.0f, 0.0f, (float)m_ControlDimensions.x, 
            0.0f, 1.0f, 0.0f, (float)m_ControlDimensions.y, 
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f};      

        glUniformMatrix4fv(m_ModelViewMatrixID, 1, GL_TRUE, location);  // GL_TRUE means transpose

        glDrawArrays(GL_TRIANGLES, 0, 6);

        if(m_pCheckboxText)
        {
            m_pCheckboxText->Draw(hdc);
        }

    }



    // register a quad for drawing on and return the id of the object
    //--------------------------------------------------------------------------------
    CPUTResult CPUTCheckbox::RegisterQuad(int width, int height, GLuint& objectID, GLuint* pVertexBufferObjectIDList)
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
            Point3(0.0f, 0.0f, 1.0f),                
            Point3(imageWidth, 0.0f, 1.0f),
            Point3(0.0f, imageHeight, 1.0f),

            Point3(imageWidth, imageHeight, 1.0f),
            Point3(0.0f, imageHeight, 1.0f),
            Point3(imageWidth, 0.0f, 1.0f),
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



        glBindVertexArray(0);

        return result;
    }

}