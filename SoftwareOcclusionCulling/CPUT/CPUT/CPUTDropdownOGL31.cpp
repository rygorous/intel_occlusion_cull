#include "CPUTDropdownOGL31.h"

namespace CPUTGL31
{
    // static initializers
    GLuint CPUTDropdown::m_ModelViewMatrixID = 0;
    CPUT_SIZE CPUTDropdown::m_pImageSizeList[] = { {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, };
    GLuint CPUTDropdown::m_pActiveTextures[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, };

    CPUT_SIZE CPUTDropdown::m_pDisabledImageSizeList[] = { {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0},{0,0}, {0,0}, {0,0}, {0,0}, {0,0},  };
    GLuint CPUTDropdown::m_pDisabledTextures[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, };

    // constructor
    //-----------------------------------------------------------------------------
    CPUTDropdown::CPUTDropdown(const cString ControlText, CPUTControlID id):m_VertexStride(0),
        m_VertexOffset(0),
        m_NumberOfSelectableItems(0),
        m_SelectableItemBufferSize(0),
        m_ppListOfSelectableItems(NULL)
    {
        // clear the button selected area rect
        m_ButtonRect.x=0; m_ButtonRect.y=0; m_ButtonRect.width=0; m_ButtonRect.height=0;
        m_TrayDimensions.x=0; m_TrayDimensions.y=0; m_TrayDimensions.width=0; m_TrayDimensions.height=0;

        // clear out the quads
        for(int i=0; i<CPUT_NUM_QUADS_IN_DROPDOWN_ARRAY; i++)
        {
            m_pQuadVertexBuffers[i] = NULL;
        }

        // set the string to display with the slider
        AddSelectionItem(ControlText, true);

        RegisterInstanceResources();

    }


    // destructor
    //-----------------------------------------------------------------------------
    CPUTDropdown::~CPUTDropdown()
    {
        UnRegisterInstanceResources();
    }

    //
    //-----------------------------------------------------------------------------
    CPUTResult CPUTDropdown::AddSelectionItem(const cString Item, bool bIsSelected)
    {
        // create a static item to hold the text
        CPUTStatic* pNewItem = new CPUTStatic;
        pNewItem->SetText( Item, 0.875f);

        // insert it into the list of strings
        if(NULL == m_ppListOfSelectableItems)
        {
            // allocate a list if none exists, default to 5 items
            m_SelectableItemBufferSize = 5;
            m_ppListOfSelectableItems = new CPUTStatic*[m_SelectableItemBufferSize];     
        }

        if(m_NumberOfSelectableItems == m_SelectableItemBufferSize)
        {
            //resize the buffer by 10 more items
            m_SelectableItemBufferSize+=10;
            CPUTStatic** ppTempList = new CPUTStatic*[m_SelectableItemBufferSize];

            // move
            for(int i=0; i<m_NumberOfSelectableItems; i++)
            {
                ppTempList[i] = m_ppListOfSelectableItems[i];
            }
            // delete the old list allocation
            delete [] m_ppListOfSelectableItems;

            m_ppListOfSelectableItems = ppTempList;
        }

        // store the new item
        m_NumberOfSelectableItems++;
        m_ppListOfSelectableItems[m_NumberOfSelectableItems-1] = pNewItem;

        // is it to be the currently selected item?
        if(bIsSelected)
        {
            m_pSelectedControlText = pNewItem;
        }

        UnRegisterAllQuads();
        RegisterInstanceResources();

        return CPUT_SUCCESS;
    }

    // Return the number of items in the dropdown list
    //-----------------------------------------------------------------------------
    void CPUTDropdown::SetSelectedItem(const unsigned int index)
    {
        if((index>0) && (index<=m_NumberOfSelectableItems))
        {
            m_pSelectedControlText = m_ppListOfSelectableItems[index-1];
        }
    }

    // Return the index of the currently selected item
    //-----------------------------------------------------------------------------
    void CPUTDropdown::GetSelectedItem(unsigned int& index)
    {
        index = -1;
    }


    // Delete an item at a specific index
    //-----------------------------------------------------------------------------
    void CPUTDropdown::DeleteSelectionItem(const unsigned int index)
    {
        if(index>m_NumberOfSelectableItems)
            return; //CPUT_WARNING_OUT_OF_RANGE;

        // dealloc the CPUTStatic for this item
        delete m_ppListOfSelectableItems[index];

        // reorder the list
        for(int i=(index-1); i<m_NumberOfSelectableItems; i++)
        {        
            m_ppListOfSelectableItems[i] = m_ppListOfSelectableItems[i+1];
        }
        m_NumberOfSelectableItems--;
    }

    // Return the number of items in the dropdown list
    //-----------------------------------------------------------------------------
    void CPUTDropdown::NumberOfSelectableItems(unsigned int& count)
    {
        count = -1;
    }

    // Delete an item with string
    // Only deletes the first occurance of the string, you'd need to call multiple
    // times to delete each occurance
    //-----------------------------------------------------------------------------
    void CPUTDropdown::DeleteSelectionItem(const cString string)
    {
        /*
        cString itemText;

        for(unsigned int i=0; i<m_pListOfSelectableItems.size(); i++)
        {
        m_pListOfSelectableItems[i]->GetString(itemText);
        if(0==itemText.compare(string))
        {
        DeleteSelectionItem(i+1);
        break;
        }            
        }
        */
    }


    // 
    //-----------------------------------------------------------------------------
    bool CPUTDropdown::ContainsPoint(int x, int y)
    {
        // clicking the readout box
        CPUT_RECT inner, outer;
        CalculateReadoutRect(inner, outer);
        if( (x>outer.x) &&
            (x<outer.x+outer.width) &&
            (y>outer.y) &&
            (y<outer.y+outer.height) )
            return true;

        if(CPUT_CONTROL_ACTIVE == m_ControlState)
        {
            // button?
            if( (x< m_ButtonRect.x) ||
                (x> m_ButtonRect.x+m_ButtonRect.width) ||
                (y< m_ButtonRect.y) ||
                (y> m_ButtonRect.y+m_ButtonRect.height) )
                return false;
            return true;


        }
        if(CPUT_CONTROL_PRESSED == m_ControlState)
        {
            // button?
            if( (x> m_ButtonRect.x) &&
                (x< m_ButtonRect.x+m_ButtonRect.width) &&
                (y> m_ButtonRect.y) &&
                (y< m_ButtonRect.y+m_ButtonRect.height) )
                return true;

            // is it in the tray?
            CPUT_RECT inner, outer;
            CalculateTrayRect(inner, outer);

            if( (x>outer.x) &&
                (x<outer.x+outer.width) &&
                (y>outer.y) &&
                (y<outer.y+outer.height) )
                return true; 
        }
        return false;           
    }

    // 
    //-----------------------------------------------------------------------------
    void CPUTDropdown::CalculateReadoutTextPosition(int& x, int& y)
    {
        CPUT_RECT inner, outer;
        CalculateReadoutRect(inner, outer);

        CPUT_RECT textDimensions;
        m_pSelectedControlText->GetDimensions(textDimensions.width,textDimensions.height);

        x = inner.x;
        y = (int) (inner.y + inner.height/2.0f - textDimensions.height/2.0f + CPUT_DROPDOWN_TEXT_PADDING/2.0f);

    }

    // 
    //-----------------------------------------------------------------------------
    void CPUTDropdown::CalculateMaxItemSize(int& width, int& height)
    {
        width=0; height=0;

        for(int i=0; i<m_NumberOfSelectableItems; i++)
        {
            CPUT_RECT rect;
            m_ppListOfSelectableItems[i]->GetDimensions(rect.width, rect.height);
            if(width < rect.width)
                width = rect.width;
            if(height < rect.height)
                height = rect.height;
        }    
    }

    // 
    //-----------------------------------------------------------------------------
    void CPUTDropdown::CalculateTrayRect(CPUT_RECT& inner, CPUT_RECT& outer)
    {
        int maxItemWidth, maxItemHeight;
        CalculateMaxItemSize(maxItemWidth, maxItemHeight);

        CPUT_RECT innerReadout, outerReadout;
        CalculateReadoutRect(innerReadout, outerReadout);

        // locations
        outer.x = outerReadout.x + 5;
        outer.y = outerReadout.y + outerReadout.height;
        inner.x = outer.x + m_pImageSizeList[CLeftMid].width;
        inner.y = outer.y + CPUT_DROPDOWN_TEXT_PADDING;

        // dimension
        inner.width = maxItemWidth + 2*CPUT_DROPDOWN_TEXT_PADDING;
        inner.height = (m_NumberOfSelectableItems*(maxItemHeight+CPUT_DROPDOWN_TEXT_PADDING));

        outer.width = inner.width + m_pImageSizeList[CLeftMid].width + m_pImageSizeList[CRightMid].width;
        outer.height = inner.height + m_pImageSizeList[CLeftBot].height;
    }

    // 
    //-----------------------------------------------------------------------------
    void CPUTDropdown::CalculateReadoutRect(CPUT_RECT& inner, CPUT_RECT& outer)
    {
        bool ButtonIsBiggerThanText = false;

        // calculate outer dimensions
        outer.x = m_ControlDimensions.x;
        outer.y = m_ControlDimensions.y;
        int maxItemSizeWidth, maxItemSizeHeight;

        CalculateMaxItemSize(maxItemSizeWidth, maxItemSizeHeight);
        if( (maxItemSizeHeight+2*CPUT_DROPDOWN_TEXT_PADDING) < m_pImageSizeList[CButtonUp].height)
        {
            ButtonIsBiggerThanText = true;
        }

        outer.width = m_pImageSizeList[CLeftTop].width + CPUT_DROPDOWN_TEXT_PADDING + maxItemSizeWidth + CPUT_DROPDOWN_TEXT_PADDING + m_pImageSizeList[CButtonUp].width + m_pImageSizeList[CRightTop].width;
        if(ButtonIsBiggerThanText)
        {
            outer.height = m_pImageSizeList[CLeftTop].height + m_pImageSizeList[CButtonUp].width + m_pImageSizeList[CLeftBot].height;
        }
        else
        {
            outer.height = m_pImageSizeList[CLeftTop].height + CPUT_DROPDOWN_TEXT_PADDING + maxItemSizeHeight + CPUT_DROPDOWN_TEXT_PADDING + m_pImageSizeList[CLeftBot].height;
        }

        // calculate the inner dimensions
        inner.x = m_ControlDimensions.x + m_pImageSizeList[CLeftTop].width + CPUT_DROPDOWN_TEXT_PADDING;
        inner.y = m_ControlDimensions.y + m_pImageSizeList[CLeftTop].height;

        if(ButtonIsBiggerThanText)
        {
            inner.width = CPUT_DROPDOWN_TEXT_PADDING + maxItemSizeWidth + CPUT_DROPDOWN_TEXT_PADDING + m_pImageSizeList[CButtonUp].width;
            inner.height = m_pImageSizeList[CButtonUp].height;
        }
        else
        {
            inner.width = CPUT_DROPDOWN_TEXT_PADDING + maxItemSizeWidth + CPUT_DROPDOWN_TEXT_PADDING + m_pImageSizeList[CButtonUp].width;
            inner.height = maxItemSizeHeight + 2*CPUT_DROPDOWN_TEXT_PADDING;
        }
    }

    // todo: deprecated?
    //-----------------------------------------------------------------------------
    void CPUTDropdown::CalculateButtonRect(CPUT_RECT& button)
    {    
        CPUT_RECT inner, outer;
        CalculateReadoutRect(inner, outer);

        button.x =  (inner.x + inner.width) - m_pImageSizeList[CButtonUp].width - CPUT_DROPDOWN_TEXT_PADDING;
        button.y =  m_ControlDimensions.y + m_pImageSizeList[CLeftTop].height;

        button.width = m_pImageSizeList[CButtonUp].width;
        button.height = m_pImageSizeList[CButtonUp].height;

        m_ButtonRect.x = button.x;
        m_ButtonRect.y = button.y;
        m_ButtonRect.width = button.width;
        m_ButtonRect.height = button.height;
    }

    // Set the enabled/disabled state of this control
    //-----------------------------------------------------------------------------
    void CPUTDropdown::SetEnable(bool in_bEnabled) 
    {
        if(in_bEnabled)
            m_ControlState = CPUT_CONTROL_ACTIVE;
        else
            m_ControlState = CPUT_CONTROL_INACTIVE;
    } 

    // Get enabled/disabled state of this control
    //-----------------------------------------------------------------------------
    bool CPUTDropdown::IsEnabled() 
    {
        if(CPUT_CONTROL_INACTIVE == m_ControlState)
            return false;

        return true;
    }

    //
    //-----------------------------------------------------------------------------
    CPUTEventHandledCode CPUTDropdown::OnMouseEvent(int x, int y, CPUTMouseState state)
    {
        static int stat = 1;

        if( ContainsPoint(x,y) )
        {        
            if(state & CPUT_MOUSE_LEFT_DOWN ) // clicked
            {
                if( CPUT_CONTROL_ACTIVE == m_ControlState)
                {
                    m_ControlState = CPUT_CONTROL_PRESSED;
                }
                else
                {

                    // close the tray
                    m_ControlState = CPUT_CONTROL_ACTIVE;
                }

                /*
                // checkbox clicked!
                // #1 - change state
                if(CPUT_CONTROL_UNCHECKED == m_eCheckboxState)
                m_eCheckboxState = CPUT_CONTROL_CHECKED;
                else if(CPUT_CONTROL_CHECKED == m_eCheckboxState)

                m_eCheckboxState = CPUT_CONTROL_UNCHECKED;

                // #2 - trigger the user's callback
                m_pCallbackHandler->OnCallbackEvent(1, m_controlID, (CPUTControl*) this);
                */
            }
            else
            {
                if(CPUT_CONTROL_PRESSED == m_ControlState)
                {
                    // if the control is pressed, figure out where in the tray they clicked,
                    // and make that the active item
                    CPUT_RECT inner, outer;
                    CalculateTrayRect(inner, outer);
                    if( (x>outer.x) && (x<outer.x+outer.width) &&
                        (y>inner.y) && (y<inner.y+inner.height))
                    {
                        // figure out which one was selected
                        int itemWidth, itemHeight;
                        CalculateMaxItemSize(itemWidth, itemHeight);
                        int itemSelect = (int)( (y - inner.y)  / (float)itemHeight);
                        SetSelectedItem(itemSelect+1);
                    }

                    // trigger the user's callback
                    m_pCallbackHandler->OnCallbackEvent(1, m_controlID, (CPUTControl*) this);
                }
            }
            return CPUT_EVENT_HANDLED;
        }
        return CPUT_EVENT_UNHANDLED; 
    }



    // 
    //-----------------------------------------------------------------------------
    CPUTResult CPUTDropdown::RegisterStaticResources(GLuint m_shaderID, GLuint modelViewMatrixID)
    {
        CPUTResult result;

        // store the needed resource ID's
        m_ModelViewMatrixID = modelViewMatrixID;


        // get relevant GUI shader program variables
        m_ModelViewMatrixID = glGetUniformLocation(m_shaderID, "ModelViewMatrix");


        // load the textures!
        const int numberOfImages = CPUT_NUM_IMAGES_IN_DROPDOWN_ARRAY;
        cString pImageFilenames[numberOfImages] = 
        {                
            _L(".//controls//dropdown//dropdown-lt.png"),
            _L(".//controls//dropdown//dropdown-lm.png"),
            _L(".//controls//dropdown//dropdown-lb.png"),
            _L(".//controls//dropdown//dropdown-mt.png"),
            _L(".//controls//dropdown//dropdown-mm.png"),
            _L(".//controls//dropdown//dropdown-mb.png"),
            _L(".//controls//dropdown//dropdown-rt.png"),
            _L(".//controls//dropdown//dropdown-rm.png"),
            _L(".//controls//dropdown//dropdown-rb.png"),
            _L(".//controls//dropdown//dropdown-button.png"),
            _L(".//controls//dropdown//dropdown-button-down.png"),
            _L(".//controls//dropdown//dropdown-highlight.png"),
        };

        // load each of the images needed for the control
        CPUTTextureLoader* pTextureLoader = CPUTTextureLoader::GetTextureLoader();
        for(int i=0; i<numberOfImages; i++)
        {
            result = pTextureLoader->LoadAndRegisterTexture(pImageFilenames[i], m_pActiveTextures[i], (GLuint&)m_pImageSizeList[i].width, (GLuint&)m_pImageSizeList[i].height, true, CPUT_PATH_SEARCH_RESOURCE_DIRECTORY);
            if(CPUTFAILED(result))
            {
                return result;
            }
        }


        //todo: load the 'disabled' control images


        return CPUT_SUCCESS;
    }

    // Release all statically register resources
    //-----------------------------------------------------------------------------
    CPUTResult CPUTDropdown::UnRegisterStaticResources()
    {
        // delete the static textures
        glDeleteTextures(CPUT_NUM_IMAGES_IN_DROPDOWN_ARRAY, m_pActiveTextures);
        glDeleteTextures(CPUT_NUM_IMAGES_IN_DROPDOWN_ARRAY, m_pDisabledTextures);


        return CPUT_SUCCESS;
    }

    // 
    //-----------------------------------------------------------------------------
    CPUTResult CPUTDropdown::RegisterInstanceResources()
    {
        CPUTResult result;

        // button
        result = RegisterQuad(m_pImageSizeList[CButtonUp].width, m_pImageSizeList[CButtonUp].height, 0.875f, m_pQuadVertexBuffers[CButtonUp], &m_pQuadVertexBufferVBO[CButtonUp]);    
        if(CPUTFAILED(result))
            return result;
        result = RegisterQuad(m_pImageSizeList[CButtonDown].width, m_pImageSizeList[CButtonDown].height, 0.875f, m_pQuadVertexBuffers[CButtonDown], &m_pQuadVertexBufferVBO[CButtonDown]);    
        if(CPUTFAILED(result))
            return result;


        /*
        CPUT_RECT rect;
        CalculateInnerDimensions(rect.width, rect.height);
        */

        CPUT_RECT inner, outer;
        CalculateReadoutRect(inner, outer);

        // control pieces
        result = RegisterQuad(m_pImageSizeList[CLeftTop].width, m_pImageSizeList[CLeftTop].height, 0.86f, m_pQuadVertexBuffers[CLeftTop], &m_pQuadVertexBufferVBO[CLeftTop]);    
        if(CPUTFAILED(result))
            return result;
        result = RegisterQuad(m_pImageSizeList[CLeftMid].width, inner.height, 0.86f, m_pQuadVertexBuffers[CLeftMid], &m_pQuadVertexBufferVBO[CLeftMid]);    
        if(CPUTFAILED(result))
            return result;
        result = RegisterQuad(m_pImageSizeList[CLeftBot].width, m_pImageSizeList[CLeftBot].height, 0.86f, m_pQuadVertexBuffers[CLeftBot], &m_pQuadVertexBufferVBO[CLeftBot]);    
        if(CPUTFAILED(result))
            return result;
        result = RegisterQuad(inner.width, m_pImageSizeList[CMidTop].height, 0.86f, m_pQuadVertexBuffers[CMidTop], &m_pQuadVertexBufferVBO[CMidTop]);    
        if(CPUTFAILED(result))
            return result;
        result = RegisterQuad(inner.width, inner.height, 0.86f, m_pQuadVertexBuffers[CMidMid], &m_pQuadVertexBufferVBO[CMidMid]);    
        if(CPUTFAILED(result))
            return result;
        result = RegisterQuad(inner.width, m_pImageSizeList[CMidBot].height, 0.86f, m_pQuadVertexBuffers[CMidBot], &m_pQuadVertexBufferVBO[CMidBot]);    
        if(CPUTFAILED(result))
            return result;
        result = RegisterQuad(m_pImageSizeList[CRightTop].width, m_pImageSizeList[CRightTop].height, 0.86f, m_pQuadVertexBuffers[CRightTop], &m_pQuadVertexBufferVBO[CRightTop]);    
        if(CPUTFAILED(result))
            return result;
        result = RegisterQuad(m_pImageSizeList[CRightMid].width, inner.height, 0.86f, m_pQuadVertexBuffers[CRightMid], &m_pQuadVertexBufferVBO[CRightMid]);    
        if(CPUTFAILED(result))
            return result;
        result = RegisterQuad(m_pImageSizeList[CRightBot].width, m_pImageSizeList[CRightBot].height, 0.86f, m_pQuadVertexBuffers[CRightBot], &m_pQuadVertexBufferVBO[CRightBot]);    
        if(CPUTFAILED(result))
            return result;


        // the active state 'tray'
        CalculateMaxItemSize(m_TrayDimensions.width, m_TrayDimensions.height);
        m_TrayDimensions.width += 2*CPUT_DROPDOWN_TEXT_PADDING;
        m_TrayDimensions.height += ((m_NumberOfSelectableItems-1)*(m_TrayDimensions.height+CPUT_DROPDOWN_TEXT_PADDING));

        CalculateTrayRect(inner, outer);
        m_TrayDimensions.width = inner.width;
        m_TrayDimensions.height = inner.height;


        result = RegisterQuad(m_pImageSizeList[CLeftMid].width, m_TrayDimensions.height, 0.86f, m_pQuadVertexBuffers[CTrayLM], &m_pQuadVertexBufferVBO[CTrayLM]);    
        if(CPUTFAILED(result))
            return result;
        result = RegisterQuad(m_pImageSizeList[CLeftBot].width, m_pImageSizeList[CLeftBot].height, 0.86f, m_pQuadVertexBuffers[CTrayLB], &m_pQuadVertexBufferVBO[CTrayLB]);    
        if(CPUTFAILED(result))
            return result;
        result = RegisterQuad(m_TrayDimensions.width, m_TrayDimensions.height, 0.86f, m_pQuadVertexBuffers[CTrayMM], &m_pQuadVertexBufferVBO[CTrayMM]);    
        if(CPUTFAILED(result))
            return result;
        result = RegisterQuad(m_TrayDimensions.width, m_pImageSizeList[CMidBot].height, 0.86f, m_pQuadVertexBuffers[CTrayMB], &m_pQuadVertexBufferVBO[CTrayMB]);    
        if(CPUTFAILED(result))
            return result;
        result = RegisterQuad(m_pImageSizeList[CRightMid].width, m_TrayDimensions.height, 0.86f, m_pQuadVertexBuffers[CTrayRM], &m_pQuadVertexBufferVBO[CTrayRM]);    
        if(CPUTFAILED(result))
            return result;
        result = RegisterQuad(m_pImageSizeList[CRightBot].width, m_pImageSizeList[CRightBot].height, 0.86f, m_pQuadVertexBuffers[CTrayRB], &m_pQuadVertexBufferVBO[CTrayRB]);    
        if(CPUTFAILED(result))
            return result;




        // tray highlight
        int MaxSizeItemWidth, MaxSizeItemHeight;
        CalculateMaxItemSize(MaxSizeItemWidth, MaxSizeItemHeight);
        result = RegisterQuad(MaxSizeItemWidth + 2*CPUT_DROPDOWN_TEXT_PADDING, MaxSizeItemHeight + CPUT_DROPDOWN_TEXT_PADDING, 0.880f, m_pQuadVertexBuffers[CTrayHighlight], &m_pQuadVertexBufferVBO[CTrayHighlight]);    
        if(CPUTFAILED(result))
            return result;
        return CPUT_SUCCESS;
    }

    // Release all instance resources (vertex buffer quads/etc)
    //-----------------------------------------------------------------------------
    CPUTResult CPUTDropdown::UnRegisterInstanceResources()
    {
        // clear the string items
        if(m_ppListOfSelectableItems)
        {
            for(int i=0; i<m_NumberOfSelectableItems; i++)
            {
                delete m_ppListOfSelectableItems[i];
                m_ppListOfSelectableItems[i]=NULL;
            }
            delete [] m_ppListOfSelectableItems;
            m_ppListOfSelectableItems = NULL;
        }

        UnRegisterAllQuads();

        return CPUT_SUCCESS;
    }

    // delete all the drawing quads
    //-----------------------------------------------------------------------------
    void CPUTDropdown::UnRegisterAllQuads()
    {
        // release all the quads we registered
        // clear out the quads
        glDeleteVertexArrays(CPUT_NUM_QUADS_IN_DROPDOWN_ARRAY, m_pQuadVertexBuffers);    
        glDeleteBuffers(((CPUT_NUM_QUADS_IN_DROPDOWN_ARRAY)*2), m_pQuadVertexBufferVBO);    
        for(int i=0; i<CPUT_NUM_QUADS_IN_DROPDOWN_ARRAY;i++)
        {
            m_pQuadVertexBuffers[i]=0;
            m_pQuadVertexBufferVBO[i]=0;
        }
    }

    // 
    //-----------------------------------------------------------------------------
    void CPUTDropdown::SetPosition(int x, int y)
    {
        m_ControlDimensions.x = x;
        m_ControlDimensions.y = y;
        if(m_pSelectedControlText)
        {
            int a,b;
            CalculateReadoutTextPosition(a,b);   
            m_pSelectedControlText->SetPosition(a,b);
        }
    }

    // 
    //-----------------------------------------------------------------------------
    void CPUTDropdown::GetDimensions(int& width, int& height)
    {
        CPUT_RECT inner, outer;
        CalculateReadoutRect(inner, outer);
        width = outer.width;
        height = outer.height;
    }

    // draw the control
    //--------------------------------------------------------------------------------
    void CPUTDropdown::Draw(HDC hdc)
    {
        // if control is in 'invisible' state, don't draw it
        if(!m_ControlVisible)
            return;

        float moveRight=0.0f;
        float moveDown=0.0f;
        float tx,ty;

        CPUT_RECT inner, outer;
        CalculateReadoutRect(inner, outer);

        // calculate all the locations we'll need for drawing the 
        for(int i=0; i<CPUT_NUM_QUADS_IN_CLOSED_DROPDOWN; i++)
        { 
            //bind the texture
            glBindTexture(GL_TEXTURE_2D, m_pActiveTextures[i]);
            // bind the quad to draw on
            glBindVertexArray(m_pQuadVertexBuffers[i]);

            //left/right spacing
            if((3==i))
            {
                moveDown=0;
                moveRight+=m_pImageSizeList[i-1].width;
            }
            if(6==i)
            { 
                moveDown=0;
                moveRight = moveRight + inner.width;
            }

            tx = m_ControlDimensions.x + moveRight;        
            ty = m_ControlDimensions.y + moveDown; 

            if( (1==i) || (4==i) || (7==i))
            {
                moveDown+=inner.height;
            }
            else
            {
                moveDown+=m_pImageSizeList[i].height;
            }

            // update translation matrix to draw at the right spot               
            // set the control location
            float location[] = { 1.0f, 0.0f, 0.0f, (float)tx, 
                0.0f, 1.0f, 0.0f, (float)ty, 
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f};
            glUniformMatrix4fv(m_ModelViewMatrixID, 1, GL_TRUE, location);  // GL_TRUE means transpose
            glDrawArrays(GL_TRIANGLES, 0, 6);       
        }

        // draw the button   
        {
            CPUT_RECT rect;
            CalculateButtonRect(rect);

            //bind the texture
            if(CPUT_CONTROL_PRESSED == m_ControlState)
            {
                glBindTexture(GL_TEXTURE_2D, m_pActiveTextures[CButtonDown]);
            }
            else
            {
                glBindTexture(GL_TEXTURE_2D, m_pActiveTextures[CButtonUp]);
            }
            // bind the quad to draw on
            glBindVertexArray(m_pQuadVertexBuffers[CButtonUp]);

            // set the control location
            float location[] = { 1.0f, 0.0f, 0.0f, (float)rect.x, 
                0.0f, 1.0f, 0.0f, (float)rect.y, 
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f};
            glUniformMatrix4fv(m_ModelViewMatrixID, 1, GL_TRUE, location);  // GL_TRUE means transpose
            glDrawArrays(GL_TRIANGLES, 0, 6);  
        }

        // draw the tray
        if(CPUT_CONTROL_PRESSED == m_ControlState)
        {
            CalculateTrayRect(inner, outer);

            float tx, ty;
            tx = (float)outer.x;
            ty = (float)outer.y;

            glBindVertexArray(m_pQuadVertexBuffers[CTrayLM]);
            glBindTexture(GL_TEXTURE_2D, m_pActiveTextures[CLeftMid]);
            float location[] = { 1.0f, 0.0f, 0.0f, (float)tx, 
                0.0f, 1.0f, 0.0f, (float)ty, 
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f};
            glUniformMatrix4fv(m_ModelViewMatrixID, 1, GL_TRUE, location);  // GL_TRUE means transpose
            glDrawArrays(GL_TRIANGLES, 0, 6);  

            {
                ty+=inner.height;
                glBindVertexArray(m_pQuadVertexBuffers[CTrayLB]);
                glBindTexture(GL_TEXTURE_2D, m_pActiveTextures[CLeftBot]);
                float location[] = { 1.0f, 0.0f, 0.0f, (float)tx, 
                    0.0f, 1.0f, 0.0f, (float)ty, 
                    0.0f, 0.0f, 1.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f};
                glUniformMatrix4fv(m_ModelViewMatrixID, 1, GL_TRUE, location);  // GL_TRUE means transpose
                glDrawArrays(GL_TRIANGLES, 0, 6); 
            }

            {
                tx+=m_pImageSizeList[CLeftMid].width;
                ty = (float)outer.y;
                glBindVertexArray(m_pQuadVertexBuffers[CTrayMM]);
                glBindTexture(GL_TEXTURE_2D, m_pActiveTextures[CMidMid]);
                float location[] = { 1.0f, 0.0f, 0.0f, (float)tx, 
                    0.0f, 1.0f, 0.0f, (float)ty, 
                    0.0f, 0.0f, 1.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f};
                glUniformMatrix4fv(m_ModelViewMatrixID, 1, GL_TRUE, location);  // GL_TRUE means transpose
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }

            {
                ty+=inner.height;
                glBindVertexArray(m_pQuadVertexBuffers[CTrayMB]);
                glBindTexture(GL_TEXTURE_2D, m_pActiveTextures[CMidBot]);
                float location[] = { 1.0f, 0.0f, 0.0f, (float)tx, 
                    0.0f, 1.0f, 0.0f, (float)ty, 
                    0.0f, 0.0f, 1.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f};
                glUniformMatrix4fv(m_ModelViewMatrixID, 1, GL_TRUE, location);  // GL_TRUE means transpose
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }

            {
                tx+=m_TrayDimensions.width;
                ty = (float)outer.y;
                glBindVertexArray(m_pQuadVertexBuffers[CTrayRM]);
                glBindTexture(GL_TEXTURE_2D, m_pActiveTextures[CRightMid]);
                float location[] = { 1.0f, 0.0f, 0.0f, (float)tx, 
                    0.0f, 1.0f, 0.0f, (float)ty, 
                    0.0f, 0.0f, 1.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f};
                glUniformMatrix4fv(m_ModelViewMatrixID, 1, GL_TRUE, location);  // GL_TRUE means transpose
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }

            {
                ty += m_TrayDimensions.height;
                glBindVertexArray(m_pQuadVertexBuffers[CTrayRB]);
                glBindTexture(GL_TEXTURE_2D, m_pActiveTextures[CRightBot]);
                float location[] = { 1.0f, 0.0f, 0.0f, (float)tx, 
                    0.0f, 1.0f, 0.0f, (float)ty, 
                    0.0f, 0.0f, 1.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f};
                glUniformMatrix4fv(m_ModelViewMatrixID, 1, GL_TRUE, location);  // GL_TRUE means transpose
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }


            int sx = (inner.x + m_pImageSizeList[CLeftMid].width);
            int sy = (inner.y + CPUT_DROPDOWN_TEXT_PADDING);
            for(int i=0;i<m_NumberOfSelectableItems;i++)
            {
                m_ppListOfSelectableItems[i]->SetPosition(sx, sy);
                m_ppListOfSelectableItems[i]->Draw( hdc );

                if( m_pSelectedControlText == m_ppListOfSelectableItems[i])
                {
                    float rx = (float)(sx - CPUT_DROPDOWN_TEXT_PADDING);
                    float ry = (float)(sy - CPUT_DROPDOWN_TEXT_PADDING);
                    glBindVertexArray(m_pQuadVertexBuffers[CTrayHighlight]);
                    glBindTexture(GL_TEXTURE_2D, m_pActiveTextures[11]);  // tray highlight texture
                    float location[] = { 1.0f, 0.0f, 0.0f, (float)rx, 
                        0.0f, 1.0f, 0.0f, (float)ry, 
                        0.0f, 0.0f, 1.0f, 0.0f,
                        0.0f, 0.0f, 0.0f, 1.0f};
                    glUniformMatrix4fv(m_ModelViewMatrixID, 1, GL_TRUE, location);  // GL_TRUE means transpose
                    glDrawArrays(GL_TRIANGLES, 0, 6);

                }

                CPUT_RECT rect;
                m_ppListOfSelectableItems[i]->GetDimensions(rect.width, rect.height);
                sy+=rect.height + CPUT_DROPDOWN_TEXT_PADDING;

            }
        }


        // draw the selected item in the readout display
        if(m_pSelectedControlText)
        {
            int x,y;
            CalculateReadoutTextPosition(x,y);   
            m_pSelectedControlText->SetPosition(x,y);        
            m_pSelectedControlText->Draw( hdc );
        }

    }

    // Register button quads for drawing on
    //--------------------------------------------------------------------------------
    CPUTResult CPUTDropdown::RegisterQuad(int width, int height, float depth, GLuint& objectID, GLuint* pVertexBufferObjectIDList)
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