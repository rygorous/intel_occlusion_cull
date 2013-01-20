#include "CPUTButtonOGL31.h"

namespace CPUTGL31
{
    // static button pieces
    bool CPUTButton::m_Registered = false;
    GLuint CPUTButton::m_shaderProgram = 0;
    GLint CPUTButton::m_textureUniformLocation = 0;
    GLint CPUTButton::m_ModelViewMatrixID = 0;
    GLuint CPUTButton::m_pButtonIdleTextureList[CPUT_NUM_IMAGES_IN_BUTTON] = {0,0,0,0,0,0,0,0,0};
    CPUT_SIZE CPUTButton::m_pButtonIdleImageSizeList[CPUT_NUM_IMAGES_IN_BUTTON] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    GLuint CPUTButton::m_pButtonPressedTextureList[CPUT_NUM_IMAGES_IN_BUTTON] = {0,0,0,0,0,0,0,0,0};
    CPUT_SIZE CPUTButton::m_pButtonPressedImageSizeList[CPUT_NUM_IMAGES_IN_BUTTON] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

    int CPUTButton::m_SmallestLeftSizeIdle=0;
    int CPUTButton::m_SmallestRightSizeIdle=0;
    int CPUTButton::m_SmallestTopSizeIdle=0;
    int CPUTButton::m_SmallestBottomSizeIdle=0;

    int CPUTButton::m_SmallestLeftSizePressed=0;
    int CPUTButton::m_SmallestRightSizePressed=0;
    int CPUTButton::m_SmallestTopSizePressed=0;
    int CPUTButton::m_SmallestBottomSizePressed=0;


    // Constructor
    //--------------------------------------------------------------------------------
    CPUTButton::CPUTButton(const cString ControlText, CPUTControlID id):CPUTButtonBase(ControlText, id), 
        m_bMouseInside(false),
        m_bButtonDown(false),
        m_pButtonText(NULL)
    {
        CPUTResult result=CPUT_SUCCESS;

        // initialize dimensions
        m_ButtonDimensions.x=0;
        m_ButtonDimensions.y=0;
        m_ButtonDimensions.width=0;
        m_ButtonDimensions.height=0;

        // register per-button resources
        RegisterInstanceResources();      

        // set the text on the button and resize it accordingly
        SetText(ControlText);

        // set the default position
        SetPosition( 0, 0 ); 

    }

    // Destructor
    //--------------------------------------------------------------------------------
    CPUTButton::~CPUTButton()
    {
        // remove yourself from the managed list
        //    CPUTGuiControllerResource::GetController()->RemoveControl((CPUTControl*) this);

        // unregister instance data - vertex buffers/etc
        UnRegisterInstanceResources();

    }

    // Does the screen coordinate intersect with this button's rect
    //-----------------------------------------------------------------------------
    bool CPUTButton::ContainsPoint(int x, int y)
    {
        if( (x > m_ButtonDimensions.x) && (x < (m_ButtonDimensions.x+m_ButtonDimensions.width)))
        {
            if( (y > m_ButtonDimensions.y) && (y < (m_ButtonDimensions.y+m_ButtonDimensions.height)))
            {
                return true;
            }
        }

        return false;
    }

    // Handle keyboard events
    //--------------------------------------------------------------------------------
    CPUTEventHandledCode CPUTButton::OnKeyboardEvent(CPUTKey key)
    {
        return CPUT_EVENT_UNHANDLED;
    }

    // Initialize the OpenGL internals
    //--------------------------------------------------------------------------------
    CPUTEventHandledCode CPUTButton::OnMouseEvent(int x, int y, int wheel, CPUTMouseState state)
    {
        CPUTEventHandledCode handledCode = CPUT_EVENT_UNHANDLED;
        if(ContainsPoint(x,y))
        {
            if(state & CPUT_MOUSE_LEFT_DOWN)
            {
                m_bButtonDown = true;

                // clicked - trigger the user's callback
                m_pCallbackHandler->OnCallbackEvent(1, m_controlID, (CPUTControl*) this);

            }
            m_bMouseInside = true;
            handledCode = CPUT_EVENT_HANDLED;
        }
        else
        {
            m_bMouseInside = false;
        }

        if(!(state & CPUT_MOUSE_LEFT_DOWN))
        {
            m_bButtonDown = false;
        }
        return handledCode;
    }

    // Register all shared resources
    //--------------------------------------------------------------------------------
    CPUTResult CPUTButton::RegisterStaticResources(GLuint m_shaderID, GLuint modelViewMatrixID)
    {
        CPUTResult result;

        if(m_Registered)
            return CPUT_SUCCESS;

        // store shader ID and needed matrix ID's
        m_shaderProgram = m_shaderID;
        m_ModelViewMatrixID = glGetUniformLocation(m_shaderProgram, "ModelViewMatrix");

        m_textureUniformLocation = glGetUniformLocation(m_shaderProgram, "diffuseTexture");


        glUseProgram(m_shaderProgram);

        // load all the idle+active button images
        const int numberOfImages = CPUT_NUM_IMAGES_IN_BUTTON;
        cString pImageFilenames[numberOfImages] = 
        {        
            _L(".//controls//button//button-lt.png"),
            _L(".//controls//button//button-lm.png"),
            _L(".//controls//button//button-lb.png"),

            _L(".//controls//button//button-mt.png"),
            _L(".//controls//button//button-mm.png"),
            _L(".//controls//button//button-mb.png"),

            _L(".//controls//button//button-rt.png"),
            _L(".//controls//button//button-rm.png"),
            _L(".//controls//button//button-rb.png"),
        };

        // load each of the 9 quadrants of a button
        CPUTOSServices* pServices = CPUTOSServices::GetOSServices();

        // load each of the 9 quadrants of a button
        CPUTTextureLoader* pTextureLoader = CPUTTextureLoader::GetTextureLoader();
        for(int i=0; i<numberOfImages; i++)
        {
            result = pTextureLoader->LoadAndRegisterTexture(pImageFilenames[i], m_pButtonIdleTextureList[i], (GLuint&)m_pButtonIdleImageSizeList[i].width, (GLuint&)m_pButtonIdleImageSizeList[i].height, true, CPUT_PATH_SEARCH_RESOURCE_DIRECTORY);
            if(CPUTFAILED(result))
            {
                return result;
            }

            // calculate the 'border' sizes
            if(i<3)
            {
                if(m_SmallestLeftSizeIdle < m_pButtonIdleImageSizeList[i].width)
                {
                    m_SmallestLeftSizeIdle = m_pButtonIdleImageSizeList[i].width;
                }
            }

            if(i>5)
            {
                if(m_SmallestRightSizeIdle < m_pButtonIdleImageSizeList[i].width)
                {
                    m_SmallestRightSizeIdle = m_pButtonIdleImageSizeList[i].width;
                }
            }

            if( (0==i) || (3==i) || (6==i) )
            {
                if(m_SmallestTopSizeIdle < m_pButtonIdleImageSizeList[i].height)
                {
                    m_SmallestTopSizeIdle = m_pButtonIdleImageSizeList[i].height;
                }
            }
            if( (2==i) || (5==i) || (8==i) )
            {
                if(m_SmallestBottomSizeIdle < m_pButtonIdleImageSizeList[i].height)
                {
                    m_SmallestBottomSizeIdle = m_pButtonIdleImageSizeList[i].height;
                }
            }
        }


        // Load the 'pressed' button state images
        cString pPressedImageFilenames[numberOfImages] = 
        {
            _L(".//controls//button//button-pressed-lt.png"),
            _L(".//controls//button//button-pressed-lm.png"),
            _L(".//controls//button//button-pressed-lb.png"),

            _L(".//controls//button//button-pressed-mt.png"),
            _L(".//controls//button//button-pressed-mm.png"),
            _L(".//controls//button//button-pressed-mb.png"),

            _L(".//controls//button//button-pressed-rt.png"),
            _L(".//controls//button//button-pressed-rm.png"),
            _L(".//controls//button//button-pressed-rb.png"),
        };

        // load each of the 9 quadrants of button
        for(int i=0; i<numberOfImages; i++)
        {
            result = pTextureLoader->LoadAndRegisterTexture(pPressedImageFilenames[i], m_pButtonPressedTextureList[i], (GLuint&)m_pButtonPressedImageSizeList[i].width, (GLuint&)m_pButtonPressedImageSizeList[i].height, true, CPUT_PATH_SEARCH_RESOURCE_DIRECTORY);
            if(CPUTFAILED(result))
            {
                return result;
            }

            // calculate the 'border' sizes
            if(i<3)
            {
                if(m_SmallestLeftSizePressed < m_pButtonPressedImageSizeList[i].width)
                {
                    m_SmallestLeftSizePressed = m_pButtonPressedImageSizeList[i].width;
                }
            }

            if(i>5)
            {
                if(m_SmallestRightSizePressed < m_pButtonPressedImageSizeList[i].width)
                {
                    m_SmallestRightSizePressed = m_pButtonPressedImageSizeList[i].width;
                }
            }

            if( (0==i) || (3==i) || (6==i) )
            {
                if(m_SmallestTopSizePressed < m_pButtonPressedImageSizeList[i].height)
                {
                    m_SmallestTopSizePressed = m_pButtonPressedImageSizeList[i].height;
                }
            }
            if( (2==i) || (5==i) || (8==i) )
            {
                if(m_SmallestBottomSizePressed < m_pButtonPressedImageSizeList[i].height)
                {
                    m_SmallestBottomSizePressed = m_pButtonPressedImageSizeList[i].height;
                }
            }
        }

        m_Registered = true;

        return CPUT_SUCCESS;
    }

    // Destroy all shared resources
    //--------------------------------------------------------------------------------
    CPUTResult CPUTButton::UnRegisterStaticResources()
    {
        const int numberOfImages = CPUT_NUM_IMAGES_IN_BUTTON;

        // unregister+delete the textures
        glDeleteTextures( numberOfImages, m_pButtonIdleTextureList );
        glDeleteTextures( numberOfImages, m_pButtonPressedTextureList );

        memset(m_pButtonIdleTextureList, 0, numberOfImages*sizeof(GLuint));
        memset(m_pButtonPressedTextureList, 0, numberOfImages*sizeof(GLuint));

        return CPUT_SUCCESS;
    }

    // Initialize the button internals
    //--------------------------------------------------------------------------------
    CPUTResult CPUTButton::RegisterInstanceResources()
    {
        CPUTResult result = CPUT_SUCCESS;     

        glUseProgram(m_shaderProgram);


        // register all 9 of the idle image quads
        for(int i=0;i<CPUT_NUM_IMAGES_IN_BUTTON;i++)
        {   
            result = RegisterQuad(m_pButtonIdleImageSizeList[i].width, m_pButtonIdleImageSizeList[i].height, m_pIdleVertexArrayObjectIDs[i], &m_pIdleVertexBufferObjectIDs[i*2]);
        }

        // register all 9 of the pressed image quads
        for(int i=0;i<CPUT_NUM_IMAGES_IN_BUTTON;i++)
        {   
            result = RegisterQuad(m_pButtonPressedImageSizeList[i].width, m_pButtonPressedImageSizeList[i].height, m_pPressedVertexArrayObjectIDs[i], &m_pPressedVertexBufferObjectIDs[i*2]);
        }

        for(int i=0;i<CPUT_NUM_IMAGES_IN_BUTTON;i++)
        {
            m_pButtonIdleSizeList[i] = m_pButtonIdleImageSizeList[i];
            m_pButtonPressedSizeList[i] = m_pButtonPressedImageSizeList[i];
        }

        // todo: register all 9 of the inactive image quads

        return result;
    }

    // delete any per-instance resources for this button
    //--------------------------------------------------------------------------------
    CPUTResult CPUTButton::UnRegisterInstanceResources()
    {
        const int numberOfBuffers = CPUT_NUM_IMAGES_IN_BUTTON;

        if(m_pButtonText)
        {
            delete m_pButtonText;
            m_pButtonText = NULL;
        }

        // unregister+delete the vertex objects
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        // delete the VBO's for this control
        glDeleteBuffers((CPUT_NUM_IMAGES_IN_BUTTON*2), m_pIdleVertexBufferObjectIDs);    
        glDeleteBuffers((CPUT_NUM_IMAGES_IN_BUTTON*2), m_pPressedVertexBufferObjectIDs);

        // delete the VAO's for this control
        glDeleteVertexArrays(numberOfBuffers, m_pIdleVertexArrayObjectIDs);
        glDeleteVertexArrays(numberOfBuffers, m_pPressedVertexArrayObjectIDs);
        memset(m_pIdleVertexArrayObjectIDs, 0, numberOfBuffers*sizeof(GLuint));

        return CPUT_SUCCESS;
    }

    //
    //--------------------------------------------------------------------------------
    void CPUTButton::SetText(const cString String)
    {
        // create the static text object if it doesn't exist
        if(NULL == m_pButtonText)
        {
            m_pButtonText = new CPUTStatic();           
        }

        m_pButtonText->SetText( String );

        // get the dimensions of the string in pixels
        CPUT_RECT rect;
        m_pButtonText->GetDimensions(rect.width, rect.height);

        // resize this control to fix that string with padding
        Resize( rect.width, rect.height);

        // move the text to a nice inset location inside the 'safe' area
        // of the button image
        int x,y;
        GetInsetTextCoordinate(x, y);
        m_pButtonText->SetPosition(x, y);

    }

    // Returns the x,y coordinate inside the button area that should be 'safe' to draw on
    //--------------------------------------------------------------------------------
    void CPUTButton::GetInsetTextCoordinate(int &x, int &y)
    {

        // get text size
        CPUT_RECT ButtonTextDimensions;
        if(m_pButtonText)
        {
            m_pButtonText->GetDimensions(ButtonTextDimensions.width, ButtonTextDimensions.height);
        }
        else
        {
            ButtonTextDimensions.width=0;
            ButtonTextDimensions.height=0;
        }     

        // calculate a good 'center' point
        x =(int) ( m_ButtonDimensions.x + m_ButtonDimensions.width/2.0f - ButtonTextDimensions.width/2.0f);
        y =(int) ( m_ButtonDimensions.y + m_ButtonDimensions.height/2.0f - ButtonTextDimensions.height/2.0f); 
    }

    //
    //--------------------------------------------------------------------------------
    void CPUTButton::GetText(char** ppString)
    {
        // todo: return string?
    }

    //
    //--------------------------------------------------------------------------------
    void CPUTButton::SetPosition(int x, int y)
    {
        m_ButtonDimensions.x = x;
        m_ButtonDimensions.y = y;

        // move the static text (if any)
        if(m_pButtonText)
        {
            int insetX, insetY;
            GetInsetTextCoordinate(insetX, insetY);
            m_pButtonText->SetPosition(insetX, insetY);
        }
    }


    // Get the upper-left screen coordinate location
    //--------------------------------------------------------------------------------
    void CPUTButton::GetLocation(int& x, int& y)
    {
        x = m_ButtonDimensions.x;
        y = m_ButtonDimensions.y;
    }

    // Returns the x,y coordinate inside the button area that should be 'safe' to draw on
    //--------------------------------------------------------------------------------
    void CPUTButton::GetInsideTextCoordinate(int &x, int &y)
    {
        // get string size
        CPUT_RECT StringDimensions;
        m_pButtonText->GetDimensions(StringDimensions.width, StringDimensions.height); 

        x = m_ButtonDimensions.x + m_ButtonDimensions.width/2 - StringDimensions.width/2;
        y = m_ButtonDimensions.y +m_ButtonDimensions.height/2 - StringDimensions.height/2;


        // subtract that from middle of button in x direction
        //m_pIdleButtonVertexBuffers[
    }

    // Force resize of the button to given dimensions
    //--------------------------------------------------------------------------------
    CPUTResult CPUTButton::Resize(int width, int height)
    {
        CPUTResult result = CPUT_SUCCESS;

        int safeWidth, safeHeight;    

        switch(m_ControlState)
        {
        case CPUT_CONTROL_ACTIVE:
            safeWidth = m_ButtonDimensions.x + m_SmallestLeftSizeIdle + m_SmallestRightSizeIdle + 1; 
            safeHeight = m_ButtonDimensions.y + m_SmallestTopSizeIdle + m_SmallestBottomSizeIdle + 1;
            break;

        case CPUT_CONTROL_INACTIVE:
            assert(false); // todo: error! unknown state - using idle dimensions as a default
            safeWidth = m_ButtonDimensions.x + m_SmallestLeftSizeIdle + m_SmallestRightSizeIdle + 1; 
            safeHeight = m_ButtonDimensions.y + m_SmallestTopSizeIdle + m_SmallestBottomSizeIdle + 1;
            break;

        case CPUT_CONTROL_PRESSED:
            safeWidth = m_ButtonDimensions.x + m_SmallestLeftSizePressed + m_SmallestRightSizePressed + 1; 
            safeHeight = m_ButtonDimensions.y + m_SmallestTopSizePressed + m_SmallestBottomSizePressed + 1;
            break;

        default:
            assert(false); // todo: error! unknown state - using idle dimensions as a default
            safeWidth = m_ButtonDimensions.x + m_SmallestLeftSizeIdle + m_SmallestRightSizeIdle + 1; 
            safeHeight = m_ButtonDimensions.y + m_SmallestTopSizeIdle + m_SmallestBottomSizeIdle + 1;
        }

        // if the user's dimensions are smaller than the smallest 'safe' dimensions of the button,
        // use the safe ones instead.
        if(safeWidth > width)
            width = safeWidth;
        if(safeHeight > height)
            height = safeHeight;

        // add some padding for nicety
        width += CPUT_BUTTON_TEXT_BORDER_PADDING_X;
        height += CPUT_BUTTON_TEXT_BORDER_PADDING_Y;


        if( (m_ButtonDimensions.width != width) || (m_ButtonDimensions.height != height) )
        {
            // store the new dimensions
            m_ButtonDimensions.width = width;
            m_ButtonDimensions.height = height;

            // calculate the pieces we'll need to rebuild
            int middleWidth = width - m_SmallestLeftSizeIdle - m_SmallestRightSizeIdle;
            int middleHeight = height - m_SmallestTopSizeIdle - m_SmallestBottomSizeIdle;

            // create a new quads with the correct size
            // Idle button quads
            //glDeleteBuffers(2, m_pIdleVertexBufferObjectIDs[1*2]);
            //glDeleteVertexArrays(1, &m_pIdleVertexArrayObjectIDs[1]);
            result = RegisterQuad(m_pButtonIdleSizeList[1].width, middleHeight, m_pIdleVertexArrayObjectIDs[1], &m_pIdleVertexBufferObjectIDs[1*2]);
            m_pButtonIdleSizeList[1].height = middleHeight;
            if(CPUTFAILED(result))
                return result;

            result = RegisterQuad(middleWidth, m_pButtonIdleSizeList[3].height, m_pIdleVertexArrayObjectIDs[3], &m_pIdleVertexBufferObjectIDs[3*2]);
            m_pButtonIdleSizeList[3].width = middleWidth;
            if(CPUTFAILED(result))
                return result;
            result = RegisterQuad(middleWidth, middleHeight, m_pIdleVertexArrayObjectIDs[4], &m_pIdleVertexBufferObjectIDs[4*2]);
            m_pButtonIdleSizeList[4].width = middleWidth;
            m_pButtonIdleSizeList[4].height = middleHeight;
            if(CPUTFAILED(result))
                return result;
            result = RegisterQuad(middleWidth, m_pButtonIdleSizeList[5].height, m_pIdleVertexArrayObjectIDs[5], &m_pIdleVertexBufferObjectIDs[5*2]);
            m_pButtonIdleSizeList[5].width = middleWidth;
            if(CPUTFAILED(result))
                return result;

            result = RegisterQuad(m_pButtonIdleSizeList[7].width, middleHeight, m_pIdleVertexArrayObjectIDs[7], &m_pIdleVertexBufferObjectIDs[7*2]);
            m_pButtonIdleSizeList[7].height = middleHeight;
            if(CPUTFAILED(result))
                return result;


            // Pressed button quads        
            result = RegisterQuad(m_pButtonPressedSizeList[1].width, middleHeight, m_pPressedVertexArrayObjectIDs[1], &m_pPressedVertexBufferObjectIDs[1*2]);
            m_pButtonPressedSizeList[1].height = middleHeight;
            if(CPUTFAILED(result))
                return result;

            result = RegisterQuad(middleWidth, m_pButtonPressedSizeList[3].height, m_pPressedVertexArrayObjectIDs[3], &m_pPressedVertexBufferObjectIDs[3*2]);
            m_pButtonPressedSizeList[3].width = middleWidth;
            if(CPUTFAILED(result))
                return result;
            result = RegisterQuad(middleWidth, middleHeight, m_pPressedVertexArrayObjectIDs[4], &m_pPressedVertexBufferObjectIDs[4*2]);
            m_pButtonPressedSizeList[4].width = middleWidth;
            m_pButtonPressedSizeList[4].height = middleHeight;
            if(CPUTFAILED(result))
                return result;
            result = RegisterQuad(middleWidth, m_pButtonPressedSizeList[5].height, m_pPressedVertexArrayObjectIDs[5], &m_pPressedVertexBufferObjectIDs[5*2]);
            m_pButtonPressedSizeList[5].width = middleWidth;
            if(CPUTFAILED(result))
                return result;

            result = RegisterQuad(m_pButtonPressedSizeList[7].width, middleHeight, m_pPressedVertexArrayObjectIDs[7], &m_pPressedVertexBufferObjectIDs[7*2]);
            m_pButtonPressedSizeList[7].height = middleHeight;
            if(CPUTFAILED(result))
                return result;
        }


        return result;
    }

    // register one of the 9 quads and return the id of the object
    //--------------------------------------------------------------------------------
    CPUTResult CPUTButton::RegisterQuad(int width, int height, GLuint& objectID, GLuint* pVertexBufferObjectIDList)
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
            Point3(0.0f, 0.0f, 0.4f),                
            Point3(imageWidth, 0.0f, 0.4f),
            Point3(0.0f, imageHeight, 0.4f),

            Point3(imageWidth, imageHeight, 0.4f),
            Point3(0.0f, imageHeight, 0.4f),
            Point3(imageWidth, 0.0f, 0.4f),
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






    // draw yourself
    //--------------------------------------------------------------------------------
    void CPUTButton::Draw(HDC hdc) 
    {
        // if control is set as non-visible, don't draw
        if(!m_ControlVisible)
            return;

        // bind the geometry
        float tx,ty;
        float moveRight=0.0f;
        float moveDown=0.0f;

        GLuint* pVertexArrayObjectList=NULL;
        GLuint* pTextureList=NULL;
        CPUT_SIZE* pImageSizeList = NULL;

        // set up/down button state based on mouse state
        if(m_bButtonDown)
            m_ControlState = CPUT_CONTROL_PRESSED;
        else
            m_ControlState = CPUT_CONTROL_ACTIVE;


        switch(m_ControlState)
        {
        case CPUT_CONTROL_ACTIVE:
            pVertexArrayObjectList = m_pIdleVertexArrayObjectIDs;
            pTextureList = m_pButtonIdleTextureList;
            pImageSizeList = m_pButtonIdleSizeList;
            break;
        case CPUT_CONTROL_INACTIVE:
            break;
        case CPUT_CONTROL_PRESSED:
            pVertexArrayObjectList = m_pPressedVertexArrayObjectIDs;
            pTextureList = m_pButtonPressedTextureList;
            pImageSizeList = m_pButtonPressedSizeList; 
            break;

        default:
            ;// todo: error! unknown state
        }


        for(int i=0; i<9; i++) //CPUT_NUM_IMAGES_IN_BUTTON; i++)
        {
            //bind the texture
            glBindTexture(GL_TEXTURE_2D, pTextureList[i]); //m_pButtonIdleTextureList[i]);

            // bind the quadrant
            glBindVertexArray(pVertexArrayObjectList[i]); //m_pIdleVertexArrayObjectIDs[i]);



            if((3==i) || (6==i))
            {
                moveDown=0;
                moveRight+=pImageSizeList[i-1].width;
            }

            tx = m_ButtonDimensions.x + moveRight;        
            ty = m_ButtonDimensions.y + moveDown;            


            float location[] = { 1.0f, 0.0f, 0.0f, tx, 
                0.0f, 1.0f, 0.0f, ty, 
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f};      

            glUniformMatrix4fv(m_ModelViewMatrixID, 1, GL_TRUE, location); //model );  // GL_TRUE means transpose

            glDrawArrays(GL_TRIANGLES, 0, 6);

            moveDown = moveDown + pImageSizeList[i].height;
        }

        if(m_pButtonText)
        {
            m_pButtonText->Draw(hdc);
        }

    }

}
