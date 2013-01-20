#include "CPUTSliderOGL31.h"

namespace CPUTGL31
{
    // static initializers
    CPUT_SIZE CPUTSlider::m_pImageSizeList[] = { {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, };
    GLuint CPUTSlider::m_pTextureResViews[] = { NULL, NULL, NULL, NULL, NULL, NULL };

    CPUT_SIZE CPUTSlider::m_pDisabledImageSizeList[] = { {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, };
    GLuint CPUTSlider::m_pDisabledTextureResViews[] = { NULL, NULL, NULL, NULL, NULL, NULL };

    GLint CPUTSlider::m_ModelViewMatrixID = 0;

    const int CTrayLeftCap = 0;
    const int CTray = 1;
    const int CTrayRightCap = 2;
    const int CTick = 3;
    const int CGrip = 4;
    const int CGripActive = 5;

    // constructor
    //--------------------------------------------------------------------------------
    CPUTSlider::CPUTSlider(const cString ControlText, CPUTControlID id):CPUTSliderBase(ControlText, id),
        m_pControlText(NULL),
        m_SliderNubTickLocation(0),
        m_SliderNubLocation(0.0f)
    { 
        // store the control id
        m_controlID = id;

        // default to an active control
        m_ControlState = CPUT_CONTROL_ACTIVE;

        // clear out the quads
        for(int i=0; i<CPUT_NUM_QUADS_IN_SLIDER_ARRAY;i++)
        {
            m_pQuadVertexBuffers[i]=0;
            m_pQuadVertexBufferObjectIDList[i]=0;
        }
        // if m_ControlDimensions are not 0, chain specifically to parent constructor

        // set the string to display with the slider
        m_pControlText = new CPUTStatic();
        m_pControlText->SetText(ControlText);

        RegisterInstanceResources();

    }

    // destructor
    //--------------------------------------------------------------------------------
    CPUTSlider::~CPUTSlider()
    {
        UnRegisterInstanceResources();
    }

    //
    //--------------------------------------------------------------------------------
    void CPUTSlider::SetPosition(int x, int y)
    {
        m_ControlDimensions.x = x;
        m_ControlDimensions.y = y;

        // text is always in the upper-left corner of the control
        m_pControlText->SetPosition(x,y);
    }

    // Register all resources shared by every slider control
    //--------------------------------------------------------------------------------
    CPUTResult CPUTSlider::RegisterStaticResources(GLuint m_shaderID, GLuint modelViewMatrixID)
    {
        // store parameters we need
        m_ModelViewMatrixID = modelViewMatrixID;

        // load the textures!
        const int numberOfImages = CPUT_NUM_IMAGES_IN_SLIDER_ARRAY;
        cString pImageFilenames[numberOfImages] =  
        {                
            _L(".//controls//slider//slider-traycapl.png"),
            _L(".//controls//slider//slider-tray.png"),
            _L(".//controls//slider//slider-traycapr.png"),
            _L(".//controls//slider//slider-tick.png"),
            _L(".//controls//slider//slider-grip.png"),
            _L(".//controls//slider//slider-gripa.png"),
        };

        // load each of the images needed for the control
        CPUTResult result;
        CPUTTextureLoader* pTextureLoader = CPUTTextureLoader::GetTextureLoader();
        for(int i=0; i<numberOfImages; i++)
        {
            result = pTextureLoader->LoadAndRegisterTexture(pImageFilenames[i], m_pTextureResViews[i], (GLuint&)m_pImageSizeList[i].width, (GLuint&)m_pImageSizeList[i].height, true, CPUT_PATH_SEARCH_RESOURCE_DIRECTORY);
            if(CPUTFAILED(result))
            {
                return result;
            }
        }

        // todo: do the same for greyed-out/inactive control images

        return CPUT_SUCCESS;
    }

    // Unregister all the shared slider controls resources
    // only call this when NO more slider controls are left
    //--------------------------------------------------------------------------------
    CPUTResult CPUTSlider::UnRegisterStaticResources()
    {    
        glDeleteTextures(CPUT_NUM_IMAGES_IN_SLIDER_ARRAY, m_pTextureResViews);

        for(int i=0;i<CPUT_NUM_IMAGES_IN_SLIDER_ARRAY;i++)
            m_pTextureResViews[i] = 0; 

        return CPUT_SUCCESS;
    }



    //
    //--------------------------------------------------------------------------------
    CPUTResult CPUTSlider::RegisterInstanceResources()
    {
        CPUTResult result;

        // register 3 quads for the tray    
        // left tray tab
        result = RegisterQuad(m_pImageSizeList[CTrayLeftCap].width, m_pImageSizeList[CTrayLeftCap].height, 0.4f, m_pQuadVertexBuffers[CTrayLeftCap], &m_pQuadVertexBufferObjectIDList[(CTrayLeftCap*2)]);
        //result = RegisterQuad(m_pImageSizeList[CTrayLeftCap].width, m_pImageSizeList[CTrayLeftCap].height, 1.0f, &m_pQuadVertexBuffers[CTrayLeftCap]);
        if(CPUTFAILED(result))
            return result;

        // middle tray    
        float TickIndent = m_pImageSizeList[CGrip].width / 2.0f;
        result = RegisterQuad((int)(CPUT_DEFAULT_TRAY_WIDTH+TickIndent/2.0f), m_pImageSizeList[CTray].height, 0.4f, m_pQuadVertexBuffers[CTray], &m_pQuadVertexBufferObjectIDList[(CTray*2)]);
        //result = RegisterQuad((int)(CPUT_DEFAULT_TRAY_WIDTH+TickIndent/2.0f), m_pImageSizeList[CTray].height, 1.0f, &m_pQuadVertexBuffers[CTray]);
        if(CPUTFAILED(result))
            return result;

        // right tray tab
        result = RegisterQuad(m_pImageSizeList[CTrayRightCap].width, m_pImageSizeList[CTrayRightCap].height, 0.4f, m_pQuadVertexBuffers[CTrayRightCap], &m_pQuadVertexBufferObjectIDList[(CTrayRightCap*2)]);
        //result = RegisterQuad(m_pImageSizeList[CTrayRightCap].width, m_pImageSizeList[CTrayRightCap].height, 1.0f, &m_pQuadVertexBuffers[CTrayRightCap]);
        if(CPUTFAILED(result))
            return result;



        // register 1 quad for the slider grip - make sure depth is 'higher' in z stack than slider tray/ticks/etc
        result = RegisterQuad(m_pImageSizeList[CGrip].width, m_pImageSizeList[CGrip].height, 0.5f, m_pQuadVertexBuffers[CGrip], &m_pQuadVertexBufferObjectIDList[(CGrip*2)]);
        //result = RegisterQuad(m_pImageSizeList[CGrip].width, m_pImageSizeList[CGrip].height, 0.5f, &m_pQuadVertexBuffers[CGrip]);
        if(CPUTFAILED(result))
            return result;



        // register 1 quad for the tickmarks
        result = RegisterQuad(m_pImageSizeList[CTick].width, m_pImageSizeList[CTick].height, 0.5f, m_pQuadVertexBuffers[CTick], &m_pQuadVertexBufferObjectIDList[(CTick*2)]);
        //result = RegisterQuad( m_pImageSizeList[CTick].width, m_pImageSizeList[CTick].height, 1.0f, &m_pQuadVertexBuffers[CTick]);
        if(CPUTFAILED(result))
            return result;

        return CPUT_SUCCESS;
    }

    //
    //--------------------------------------------------------------------------------
    CPUTResult CPUTSlider::UnRegisterInstanceResources()
    {
        // release all the quads we registered
        // clear out the quads
        glDeleteVertexArrays(CPUT_NUM_QUADS_IN_SLIDER_ARRAY, m_pQuadVertexBuffers);    
        glDeleteBuffers(((CPUT_NUM_QUADS_IN_SLIDER_ARRAY)*2), m_pQuadVertexBufferObjectIDList);    
        for(int i=0; i<CPUT_NUM_QUADS_IN_SLIDER_ARRAY;i++)
        {
            m_pQuadVertexBuffers[i]=0;
            m_pQuadVertexBufferObjectIDList[i]=0;
        }


        return CPUT_SUCCESS;
    }

    // Handle mouse events
    //--------------------------------------------------------------------------------
    CPUTEventHandledCode CPUTSlider::OnMouseEvent(int x, int y, CPUTMouseState state)
    {
        CPUTEventHandledCode handledCode = CPUT_EVENT_UNHANDLED;

        // if we're continuing to be pressed, move around with the mouse movement
        if( (CPUT_CONTROL_PRESSED == m_ControlState) && (CPUT_MOUSE_LEFT_DOWN == state))
        {
            m_SliderNubLocation = (float)x-m_ControlDimensions.x;

            // are they dragging off the left side?
            if(m_SliderNubLocation < 0.0f)
            {
                m_SliderNubLocation = 0.0f;
            }

            // are they dragging off the right side?
            float TrayIndent = m_pImageSizeList[CGrip].width / 2.0f;
            float ExtentX = (float)(TrayIndent + m_pImageSizeList[CTrayLeftCap].width + CPUT_DEFAULT_TRAY_WIDTH + m_pImageSizeList[CTrayRightCap].width - m_pImageSizeList[CGrip].width+2);

            if(m_SliderNubLocation > ExtentX)
            {
                m_SliderNubLocation = ExtentX;
            }
            handledCode = CPUT_EVENT_HANDLED;
        } 
        // did the slider just get released?
        if( (CPUT_CONTROL_PRESSED == m_ControlState) && (CPUT_MOUSE_LEFT_DOWN != state))
        {
            m_ControlState = CPUT_CONTROL_ACTIVE;
            SnapToNearestTick();
            handledCode = CPUT_EVENT_HANDLED;

            // trigger the user's callback
            m_pCallbackHandler->OnCallbackEvent(1, m_controlID, (CPUTControl*) this);
        }

        if(ContainsPoint(x,y))
        {
            if(CPUT_MOUSE_LEFT_DOWN == state)
            {
                if(PointingAtNub(x,y))
                {
                    m_ControlState = CPUT_CONTROL_PRESSED;
                }
            }
            else
            {
                m_ControlState = CPUT_CONTROL_ACTIVE;
            }
            handledCode = CPUT_EVENT_HANDLED;
        }

        return handledCode;
    }

    // Calculate important size info used by most of the system
    //--------------------------------------------------------------------------------
    void CPUTSlider::CalculateLocationGuides(LocationGuides& guides)
    {
        CPUT_RECT rect;
        m_pControlText->GetDimensions(rect.width, rect.height);
        guides.TickIndent = m_pImageSizeList[CGrip].width / 2.0f;
        guides.TextDownIndent = (float)(rect.height+2);
        guides.GripDownIndent = guides.TextDownIndent + m_pImageSizeList[CTick].height/2.0f;
        guides.TrayDownIndent =  guides.TextDownIndent +  (m_pImageSizeList[CGrip].height / 2.0f) + m_pImageSizeList[CTick].height/2.0f; 
        guides.TickSpacing = ( CPUT_DEFAULT_TRAY_WIDTH/(float)(m_SliderNumberOfSteps-1));
        guides.TotalWidth = guides.TickIndent + m_pImageSizeList[CTrayLeftCap].width + CPUT_DEFAULT_TRAY_WIDTH + m_pImageSizeList[CTrayRightCap].width;
        guides.TotalHeight = ( guides.TextDownIndent + guides.GripDownIndent + m_pImageSizeList[CGrip].height);
    }

    // return screen-space width/height of the control
    //--------------------------------------------------------------------------------
    void CPUTSlider::GetDimensions(int& width, int& height)
    {
        LocationGuides guides;
        CalculateLocationGuides(guides);

        width=(int)guides.TotalWidth;
        height=(int)guides.TotalHeight;
    }

    // return the x/y window position of the control
    //--------------------------------------------------------------------------------
    void CPUTSlider::GetPosition(int& x, int& y)
    {
        x = m_ControlDimensions.x;
        y = m_ControlDimensions.y;
    }

    //
    //--------------------------------------------------------------------------------
    bool CPUTSlider::ContainsPoint(int x, int y)
    {
        // calculate all the locations we'll need for drawing
        LocationGuides guides;
        CalculateLocationGuides(guides);

        if( (x > ( m_ControlDimensions.x + guides.TotalWidth )) || 
            (x < m_ControlDimensions.x) ||
            (y < m_ControlDimensions.y) ||
            (y > (m_ControlDimensions.y + guides.TotalHeight)) )
        {
            return false;
        }

        return true;
    }

    //
    //--------------------------------------------------------------------------------
    bool CPUTSlider::PointingAtNub(int x, int y)
    {
        // calculate all the locations we'll need for drawing
        LocationGuides guides;
        CalculateLocationGuides(guides);

        // locate the grabber coordinates
        float UpperLeftX=m_SliderNubLocation+m_ControlDimensions.x;
        float UpperLeftY=(float)(m_ControlDimensions.y + guides.GripDownIndent);
        float LowerRightX = UpperLeftX + m_pImageSizeList[CGrip].width;
        float LowerRightY = UpperLeftY + m_pImageSizeList[CGrip].height;

        if( (x>LowerRightX) || (x<UpperLeftX) ||
            (y<UpperLeftY) || (y>LowerRightY ) )
        {
            return false;
        }

        return true;
    }

    //
    //--------------------------------------------------------------------------------
    void CPUTSlider::SnapToNearestTick()
    {
        LocationGuides guides;
        CalculateLocationGuides(guides);


        float locationOnTray = m_SliderNubLocation;

        int index = (int) (locationOnTray/guides.TickSpacing);
        float remainder = locationOnTray - index*guides.TickSpacing;
        if(remainder > (guides.TickSpacing/2.0f))
        {
            // snap to next higher spot
            index++;        
        }

        // calculate the tick mark to align with
        m_SliderNubLocation = (float)(guides.TickIndent  + (index*guides.TickSpacing) - (m_pImageSizeList[CGrip].width/2.0f));

    }

    // Set the string that sits above the actual slider bar
    //------------------------------------------------------------------------------
    void CPUTSlider::SetString(const cString ControlText)
    {
        m_pControlText->SetText(ControlText);
    }


    //CPUTSliderBase

    // Set the Scale
    //--------------------------------------------------------------------------------
    CPUTResult CPUTSlider::SetScale(float StartValue, float EndValue, int NumberOfSteps)
    {
        if( StartValue >= EndValue )
            return CPUT_ERROR_INVALID_PARAMETER;

        m_SliderStartValue = StartValue;
        m_SliderEndValue = EndValue;
        m_SliderNumberOfSteps = NumberOfSteps+1;

        // to avoid the problem of changing the scale and having the gripper be
        // out of that range, setScale always sets the gripper to the start
        // value when re-ranging the control
        SetValue(StartValue);

        return CPUT_SUCCESS;
    }

    //
    //--------------------------------------------------------------------------------
    CPUTResult CPUTSlider::GetValue(float& fValue)
    {
        //SnapToNearestTick();

        LocationGuides guides;
        CalculateLocationGuides(guides);

        float locationOnTray = m_SliderNubLocation;

        int index = (int) (locationOnTray/guides.TickSpacing);
        float remainderPixels = locationOnTray - index*guides.TickSpacing;

        float remainderPercent = remainderPixels/guides.TickSpacing;
        float stepSizeValue = (m_SliderEndValue - m_SliderStartValue)/(m_SliderNumberOfSteps-1);
        float remainderValue = stepSizeValue * remainderPercent;

        fValue = m_SliderStartValue + ((m_SliderEndValue - m_SliderStartValue)/(m_SliderNumberOfSteps-1))*index + remainderValue;

        return CPUT_SUCCESS;
    }

    // Moves the slider gripper to the specified value on the slider
    //--------------------------------------------------------------------------------
    CPUTResult CPUTSlider::SetValue(float fValue)
    {
        if(fValue>m_SliderEndValue)
        {
            fValue = m_SliderEndValue;
        }
        else if(fValue<m_SliderStartValue)
        {
            fValue = m_SliderStartValue;
        }

        LocationGuides guides;
        CalculateLocationGuides(guides);

        //+ guides.TickIndent ) + (i*guides.TickSpacing);
        float percentValue = (fValue-m_SliderStartValue)/(m_SliderEndValue-m_SliderStartValue);
        float locationOnTray = guides.TotalWidth * percentValue; 

        int index = (int) (locationOnTray/guides.TickSpacing);
        float remainder = locationOnTray - index*guides.TickSpacing;
        if(remainder > (guides.TickSpacing/2.0f))
        {
            // snap to next higher spot
            index++;        
        }

        float stepSizeValue = (m_SliderEndValue - m_SliderStartValue)/(m_SliderNumberOfSteps-1);
        float remainderValue = (fValue-m_SliderStartValue) - index*stepSizeValue;

        float remainderPercent = remainderValue/stepSizeValue;
        float remainderPixels = remainderPercent*guides.TickSpacing;
        m_SliderNubLocation = (float)(guides.TickIndent  + (index*guides.TickSpacing) + remainderPixels - (m_pImageSizeList[CGrip].width/2.0f));



        return CPUT_SUCCESS;
    }


    // draw the control
    //--------------------------------------------------------------------------------
    void CPUTSlider::Draw(HDC hdc)
    {
        // if control is in 'invisible' state, don't draw it
        if(!m_ControlVisible)
            return;

        if(m_pControlText)
        {
            m_pControlText->Draw( hdc );
        }


        LocationGuides guides;
        CalculateLocationGuides(guides);


        // draw the left tray tab
        //bind the texture
        {
            glBindTexture(GL_TEXTURE_2D, m_pTextureResViews[CTrayLeftCap]);
            // bind the quad to draw on
            glBindVertexArray(m_pQuadVertexBuffers[CTrayLeftCap]);

            // set the control location
            float TrayX=(float)(m_ControlDimensions.x + guides.TickIndent/2.0f);
            float TrayY=(float)(m_ControlDimensions.y + guides.TrayDownIndent);
            float location[] = { 1.0f, 0.0f, 0.0f, (float)TrayX, 
                0.0f, 1.0f, 0.0f, (float)TrayY, 
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f};
            glUniformMatrix4fv(m_ModelViewMatrixID, 1, GL_TRUE, location);  // GL_TRUE means transpose
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }



        // draw the tray itself
        {
            //bind the texture
            glBindTexture(GL_TEXTURE_2D, m_pTextureResViews[CTray]);
            // bind the quad to draw on
            glBindVertexArray(m_pQuadVertexBuffers[CTray]);

            // set the control location
            float TrayX=(float)(m_ControlDimensions.x + guides.TickIndent/2.0f + m_pImageSizeList[CTrayLeftCap].width);
            float TrayY=(float)(m_ControlDimensions.y + guides.TrayDownIndent);
            float location[] = { 1.0f, 0.0f, 0.0f, (float)TrayX, 
                0.0f, 1.0f, 0.0f, (float)TrayY, 
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f};
            glUniformMatrix4fv(m_ModelViewMatrixID, 1, GL_TRUE, location);  // GL_TRUE means transpose
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }



        // draw the right tray tab    
        {
            //bind the texture
            glBindTexture(GL_TEXTURE_2D, m_pTextureResViews[CTrayRightCap]);
            // bind the quad to draw on
            glBindVertexArray(m_pQuadVertexBuffers[CTrayRightCap]);

            // set the control location
            float TrayX=(float)(m_ControlDimensions.x  + guides.TickIndent/2.0f + m_pImageSizeList[CTrayLeftCap].width + CPUT_DEFAULT_TRAY_WIDTH + guides.TickIndent/2.0f);
            float TrayY=(float)(m_ControlDimensions.y + guides.TrayDownIndent);
            float location[] = { 1.0f, 0.0f, 0.0f, (float)TrayX, 
                0.0f, 1.0f, 0.0f, (float)TrayY, 
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f};
            glUniformMatrix4fv(m_ModelViewMatrixID, 1, GL_TRUE, location);  // GL_TRUE means transpose
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }   

        // draw ticks
        for(int i=0; i<m_SliderNumberOfSteps; i++)
        {
            //bind the texture
            glBindTexture(GL_TEXTURE_2D, m_pTextureResViews[CTick]);
            // bind the quad to draw on
            glBindVertexArray(m_pQuadVertexBuffers[CTick]);

            // set the control location
            float TrayX=(float)(m_ControlDimensions.x  + guides.TickIndent ) + (i*guides.TickSpacing);
            float TrayY=(float)(m_ControlDimensions.y + guides.TextDownIndent);
            float location[] = { 1.0f, 0.0f, 0.0f, (float)TrayX, 
                0.0f, 1.0f, 0.0f, (float)TrayY, 
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f};
            glUniformMatrix4fv(m_ModelViewMatrixID, 1, GL_TRUE, location);  // GL_TRUE means transpose
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }


        // draw the slider nub 
        {
            //bind the texture
            if(CPUT_CONTROL_PRESSED == m_ControlState)
            {
                glBindTexture(GL_TEXTURE_2D, m_pTextureResViews[CGripActive]);
            }
            else
            {
                glBindTexture(GL_TEXTURE_2D, m_pTextureResViews[CGrip]);
            }
            // bind the quad to draw on
            glBindVertexArray(m_pQuadVertexBuffers[CGrip]);

            // set the control location
            float TrayX = m_ControlDimensions.x + m_SliderNubLocation;
            float TrayY=(float)(m_ControlDimensions.y + guides.GripDownIndent);
            float location[] = { 1.0f, 0.0f, 0.0f, (float)TrayX, 
                0.0f, 1.0f, 0.0f, (float)TrayY, 
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f};
            glUniformMatrix4fv(m_ModelViewMatrixID, 1, GL_TRUE, location);  // GL_TRUE means transpose
            glDrawArrays(GL_TRIANGLES, 0, 6); 
        }

    }



    // Register button quads for drawing on
    //--------------------------------------------------------------------------------
    CPUTResult CPUTSlider::RegisterQuad(int width, int height, float depth, GLuint& objectID, GLuint* pVertexBufferObjectIDList)
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



        glBindVertexArray(0);

        return result;
    }
}