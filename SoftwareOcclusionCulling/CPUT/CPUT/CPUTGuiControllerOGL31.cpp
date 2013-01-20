#include "CPUTGuiControllerOGL31.h"

namespace CPUTGL31
{

    CPUTGuiController* CPUTGuiController::m_guiController = NULL;

    // chained constructor
    //--------------------------------------------------------------------------------
    CPUTGuiController::CPUTGuiController():CPUTGuiControllerBase(), 
        m_bInitialized(false),
        m_shaderProgram(0),
        m_ProjectionMatrixID(0),
        m_ModelViewMatrixID(0)
    {    

    }

    // destructor
    //--------------------------------------------------------------------------------
    CPUTGuiController::~CPUTGuiController()
    {
        // todo: unregister all static control resources
        CPUTButton::UnRegisterStaticResources();

        // walk list of controls and delete them all
        DeleteAllControls();

        // delete the shader
        glDeleteProgram(m_shaderProgram);    
    }

    // singleton getter
    //--------------------------------------------------------------------------------
    CPUTGuiController* CPUTGuiController::GetController() 
    {            
        if(NULL==m_guiController)
            m_guiController = new CPUTGuiController;
        return m_guiController;
    }

    // Initialize the GUI controller and all it's static resources
    //--------------------------------------------------------------------------------
    CPUTResult CPUTGuiController::Initialize()
    {
        cString VertexShader = _L(".//shaders//GUIShaderGL.vs");
        cString PixelShader = _L(".//shaders//GUIShaderGL.ps");
        cString DefaultFontFile = _L(".//fonts//font12b");

        return RegisterGUIResources(VertexShader, PixelShader, DefaultFontFile);   
    }


    // Get the GUI shader's program handle
    //--------------------------------------------------------------------------------
    CPUTResult CPUTGuiController::GetShaderProgram(GLuint& programID)
    {
        if(!m_bInitialized)
        {
            return CPUT_ERROR_COMPONENT_NOT_INITIALIZED;
        }
        programID = m_shaderProgram;

        m_bInitialized = true;

        return CPUT_SUCCESS;
    }

    // Control creation

    // Create a button control and add it to the GUI layout controller
    //--------------------------------------------------------------------------------
    CPUTResult CPUTGuiController::CreateButton(const cString ButtonText, CPUTControlID controlID, CPUTControlID panelID, CPUTButton** ppButton)
    {
        if(NULL==ppButton)
            return CPUT_ERROR_INVALID_PARAMETER;

        // create the control
        *ppButton = new CPUTButton(ButtonText, controlID);
        if(NULL==*ppButton)
            return CPUT_GUI_CANNOT_CREATE_CONTROL;

        // add control to the gui manager
        return this->AddControl(*ppButton, panelID);
    }

    // Create a slider control and add it to the GUI layout controller
    //--------------------------------------------------------------------------------
    CPUTResult CPUTGuiController::CreateSlider(const cString SliderText, CPUTControlID controlID, CPUTControlID panelID, CPUTSlider** ppSlider)
    {
        if(NULL==ppSlider)
            return CPUT_ERROR_INVALID_PARAMETER;

        // create the control
        *ppSlider = new CPUTSlider( SliderText, controlID);
        if(NULL==*ppSlider)
            return CPUT_GUI_CANNOT_CREATE_CONTROL;

        // add control to the gui manager
        return this->AddControl(*ppSlider, panelID);
    }

    // Create a checkbox control and add it to the GUI layout controller
    //--------------------------------------------------------------------------------
    CPUTResult CPUTGuiController::CreateCheckbox(const cString CheckboxText, CPUTControlID controlID, CPUTControlID panelID, CPUTCheckbox** ppChecbox)
    {
        if(NULL==ppChecbox)
            return CPUT_ERROR_INVALID_PARAMETER;

        // create the control
        *ppChecbox = new CPUTCheckbox( CheckboxText, controlID);
        if(NULL==*ppChecbox)
            return CPUT_GUI_CANNOT_CREATE_CONTROL;

        // add control to the gui manager
        return this->AddControl(*ppChecbox, panelID);
    }

    // Create a dropdown control and add it to the GUI layout controller
    //--------------------------------------------------------------------------------
    CPUTResult CPUTGuiController::CreateDropdown(const cString SelectionText, CPUTControlID controlID, CPUTControlID panelID, CPUTDropdown** ppDropdown)
    {
        if(NULL==ppDropdown)
            return CPUT_ERROR_INVALID_PARAMETER;

        // create the control
        *ppDropdown = new CPUTDropdown(SelectionText, controlID);
        if(NULL==*ppDropdown)
            return CPUT_GUI_CANNOT_CREATE_CONTROL;

        // add control to the gui manager
        return this->AddControl(*ppDropdown, panelID);
    }

    // Create a text item (static text)
    //--------------------------------------------------------------------------------
    CPUTResult CPUTGuiController::CreateStatic(const cString Text, CPUTControlID panelID, CPUTControlID controlID, CPUTStatic** ppStatic)
    {
        if(NULL==ppStatic)
            return CPUT_ERROR_INVALID_PARAMETER;

        // create the control
        CPUTStatic* pStatic=NULL;  
        pStatic = new CPUTStatic(Text, controlID);
        if(NULL==pStatic)
            return CPUT_GUI_CANNOT_CREATE_CONTROL;
        if(NULL != ppStatic)
            *ppStatic = pStatic;

        // add control to the gui manager
        return this->AddControl(pStatic, panelID);    
    }


    // Deletes a control from the GUI manager
    // Will delete all instances of the control no matter which panel(s) it is in and then
    // deallocates the memory for the control
    //--------------------------------------------------------------------------------
    CPUTResult CPUTGuiController::DeleteControl( CPUTControlID controlID)
    {
        // look thruogh all the panels and delete the item with this controlID
        // for each panel
        std::vector <CPUTControl*> pDeleteList;

        for(unsigned int i=0; i<m_ControlPanelIDList.size(); i++)
        {
            // walk list of controls
            for(unsigned int j=0; j<m_ControlPanelIDList[i]->m_ControlList.size(); j++)
            {
                if(controlID == m_ControlPanelIDList[i]->m_ControlList[j]->GetControlID())
                {
                    // found an instance of the control we wish to delete
                    // see if it's in the list already
                    bool bFound = false;
                    for(unsigned int x=0; x<pDeleteList.size(); x++)
                    {
                        if( m_ControlPanelIDList[i]->m_ControlList[j] ==  pDeleteList[x] )
                        {
                            bFound = true;
                            break;
                        }
                    }

                    if(!bFound)
                    {
                        // store for deleting
                        pDeleteList.push_back( m_ControlPanelIDList[i]->m_ControlList[j] );
                    }

                    // remove the control from the container list
                    m_ControlPanelIDList[i]->m_ControlList.erase( m_ControlPanelIDList[i]->m_ControlList.begin() + j );
                }
            }
        }

        // delete the control(s) we found with this id
        for(unsigned int i=0; i<pDeleteList.size(); i++)
            delete pDeleteList[i];

        // force a resize event to recalculate new control locations now that some might have been deleted
        this->Resize();

        // don't worry about cleaning up std::vector list itself, it'll do so when it falls out of scope
        return CPUT_SUCCESS;
    }

    // Walk the list of controls and issue a draw call to each one
    //--------------------------------------------------------------------------------
    void CPUTGuiController::Draw(HDC hdc)
    {
        HEAPCHECKVOID;

        if( 0 != GetNumberOfControlsInPanel())
            SetGUIDrawingState();
        else
            return;

        // walk the list of controls and issue a draw command!
        int i=0;
        CPUTOSServices* pServices;
        while(i<GetNumberOfControlsInPanel())
        {
            CPUTControl* control = m_ControlPanelIDList[m_ActiveControlPanelSlotID]->m_ControlList[i];

            switch(m_ControlPanelIDList[m_ActiveControlPanelSlotID]->m_ControlList[i]->GetType())
            {
            case CPUT_BUTTON:            
                ((CPUTButton*)control)->Draw(hdc);
                break;

            case CPUT_CHECKBOX:            
                ((CPUTCheckbox*)control)->Draw(hdc);
                break;

            case CPUT_DROPDOWN:
                ((CPUTDropdown*)control)->Draw(hdc);
                break;

            case CPUT_SLIDER:
                ((CPUTSlider*)control)->Draw(hdc);
                break;

            case CPUT_STATIC:
                ((CPUTStatic*)control)->Draw(hdc);
                break;
            default:
                // Error: Unknown control - Add handler for new control type if you want it rendered
                pServices = CPUTOSServices::GetOSServices();
                pServices->Assert(0);            
            }
            i++;
        }      

        // restore the drawing state
        ClearGUIDrawingState();

        HEAPCHECKVOID;
    }


    // Register all the global resources needed for drawing GUI elements
    //--------------------------------------------------------------------------------
    CPUTResult CPUTGuiController::RegisterGUIResources(cString VertexShader, cString PixelShader, cString DefaultFontFilename)
    {
        // register the shaders
        // register the simplified pixel+vertex shader for drawing buttons
        CPUTResult result;
        CPUTShaderLoader* pShaderLoader = CPUTShaderLoader::GetShaderLoader();
        GLuint vertexShaderID = 0;
        GLuint pixelShaderID = 0;

        result = pShaderLoader->CompileShaderFromFile(VertexShader, CPUT_VERTEX_SHADER, vertexShaderID, CPUT_PATH_SEARCH_RESOURCE_DIRECTORY);
        if(CPUTFAILED(result))
        {
            return result;
        }

        result = pShaderLoader->CompileShaderFromFile(PixelShader, CPUT_PIXEL_SHADER, pixelShaderID, CPUT_PATH_SEARCH_RESOURCE_DIRECTORY);
        if(CPUTFAILED(result))
        {
            return result;
        }

        GLint compiledVertex, compiledPixel;
        glGetShaderiv(vertexShaderID, GL_COMPILE_STATUS, &compiledVertex);
        glGetShaderiv(pixelShaderID, GL_COMPILE_STATUS, &compiledPixel);

        if( (0==vertexShaderID) || (0==pixelShaderID) || (!compiledVertex) || (!compiledPixel) )
        {
            return CPUT_SHADER_REGISTRATION_ERROR;
        }

        // create a new shader program, and attach the shaders
        m_shaderProgram = glCreateProgram();

        // link up the variables with shader inputs/outputs
        glBindAttribLocation(m_shaderProgram, 0, "in_Position");
        glBindAttribLocation(m_shaderProgram, 1, "in_UV");

        // attach the shaders to a program
        glAttachShader(m_shaderProgram, vertexShaderID);
        glAttachShader(m_shaderProgram, pixelShaderID);



        // link and test results
        glLinkProgram(m_shaderProgram);
        cString Message;
        result = pShaderLoader->CheckShaderCompileLog(m_shaderProgram, Message );
        if( CPUTFAILED(result) )
        {
            // todo: report an error
            //CPUTMessageBox("Error during shader link step", message, CPUT_MB_ERROR);
            return result;
        }

        // bind the program so you get the right id's for the uniformLocation calls
        glUseProgram(m_shaderProgram);


        // set the perspective projection matrix
        glUseProgram(m_shaderProgram);
        m_ProjectionMatrixID = glGetUniformLocation(m_shaderProgram, "ProjectionMatrix");
        m_ModelViewMatrixID = glGetUniformLocation(m_shaderProgram, "ModelViewMatrix");    

        // get screen dimensions
        int width,height;
        CPUTOSServices* pServices = CPUTOSServices::GetOSServices();    
        pServices->GetClientDimensions( width, height);

        // orthographic projection
        float projectionMatrix[4*4];
        BuildOrthoProjMat(projectionMatrix, 0, (float)width, (float)height, 0, -1.0f, 1.0f);

        // button location in window coordinates
        float identity[] = { 1.0f, 0.0f, 0.0f, 0.0f, 
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f};

        glUniformMatrix4fv(m_ProjectionMatrixID, 1, GL_TRUE, projectionMatrix );  // GL_TRUE means transpose  
        glUniformMatrix4fv(m_ModelViewMatrixID, 1, GL_TRUE, identity);  // GL_TRUE means transpose    



        // Now that all that's taken care of, register each control's graphics/etc
        // so that we only do that once instead of having tons of them laying around
        // per-object
        result = CPUTStatic::RegisterStaticResources( m_shaderProgram, m_ModelViewMatrixID, DefaultFontFilename );
        if(CPUTFAILED(result))
        {
            if(CPUT_ERROR_FILE_NOT_FOUND == result)
                return CPUT_ERROR_INITIALIZATION_GUI_CONTROL_TEXTURES_NOT_FOUND;
            else
                return result;
        }
        result = CPUTButton::RegisterStaticResources( m_shaderProgram, m_ModelViewMatrixID );
        if(CPUTFAILED(result))
        {
            if(CPUT_ERROR_FILE_NOT_FOUND == result)
                return CPUT_ERROR_INITIALIZATION_GUI_CONTROL_TEXTURES_NOT_FOUND;
            else
                return result;
        }
        result = CPUTCheckbox::RegisterStaticResources( m_shaderProgram, m_ModelViewMatrixID );
        if(CPUTFAILED(result))
        {
            if(CPUT_ERROR_FILE_NOT_FOUND == result)
                return CPUT_ERROR_INITIALIZATION_GUI_CONTROL_TEXTURES_NOT_FOUND;
            else
                return result;
        }
        result = CPUTSlider::RegisterStaticResources( m_shaderProgram, m_ModelViewMatrixID );
        if(CPUTFAILED(result))
        {
            if(CPUT_ERROR_FILE_NOT_FOUND == result)
                return CPUT_ERROR_INITIALIZATION_GUI_CONTROL_TEXTURES_NOT_FOUND;
            else
                return result;
        }
        result = CPUTDropdown::RegisterStaticResources( m_shaderProgram, m_ModelViewMatrixID );
        if(CPUTFAILED(result))
        {
            if(CPUT_ERROR_FILE_NOT_FOUND == result)
                return CPUT_ERROR_INITIALIZATION_GUI_CONTROL_TEXTURES_NOT_FOUND;
            else
                return result;
        }


        // all done!
        m_bInitialized = true;

        // unbind the shader program
        glUseProgram(0);

        return CPUT_SUCCESS;
    }


    // OpenGL orthographic matrix calculator
    // http://www.songho.ca/opengl/gl_projectionmatrix.html
    // or refer to docs on glOrtho()
    //-----------------------------------------------------------------------------
    void CPUTGuiController::BuildOrthoProjMat(float *m, float left, float right, float bottom, float top, float znear, float zfar)
    {
        float tx = (right+left) / (right-left);
        float ty = (top+bottom) / (top-bottom);
        float tz = (zfar+znear)/(zfar-znear);

        // equation same as in the OpenGL glOrtho() 
        m[0] = 2.0f/(right-left);     m[1] = 0;   m[2]  = 0;  m[3] = -1.0f*tx;
        m[4] = 0;     m[5] = 2.0f/(top-bottom);   m[6]  = 0;  m[7] = -1.0f*ty;
        m[8] = 0;     m[9] = 0;   m[10] = -2.0f/(zfar-znear);  m[11] = -1.0f*tz;
        m[12] = 0;     m[13] = 0;   m[14] =0;  m[15] = 1.0f;
    }


    //  Set the state for drawing GUI controls
    //-----------------------------------------------------------------------------
    void CPUTGuiController::SetGUIDrawingState()
    {
        // set shader to the GUI element shader
        glUseProgram(m_shaderProgram);

        // lights off

        // disable depth test
        glEnable(GL_DEPTH_TEST);

        // set fill mode
        glGetIntegerv(GL_POLYGON_MODE, m_FillMode);
        glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

        // set blend function

        // enable alpha blending
        glEnable(GL_BLEND);
        glAlphaFunc(GL_GREATER, 0.1f);
        glEnable(GL_ALPHA_TEST);    
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


        // enable textures    
        // get the uniform variable location in the shader for the diffuse texture
        GLint m_textureUniformLocation = glGetUniformLocation(m_shaderProgram, "diffuseTexture");

        //Active bound texture is unit 0
        glUniform1i(m_textureUniformLocation, 0);

        // turn on texture unit 0 (if it wasn't already)
        glActiveTexture(GL_TEXTURE0);

        // Set up the orthographic projection
        // see item #13 here for ortho setup tips: http://www.opengl.org/resources/features/KilgardTechniques/oglpitfall/
        int windowWidth, windowHeight;
        CPUTOSServices* pServices = CPUTOSServices::GetOSServices();
        pServices->GetClientDimensions( windowWidth, windowHeight);   

        float projectionMatrix[16];
        BuildOrthoProjMat(projectionMatrix, 0, (float)windowWidth, (float)windowHeight, 0, -1.0f, 1.0f);
        glUniformMatrix4fv(m_ProjectionMatrixID, 1, GL_TRUE, projectionMatrix );  // GL_TRUE means transpose  

    }

    //  Restore previous state from drawing GUI controls
    //-----------------------------------------------------------------------------
    void CPUTGuiController::ClearGUIDrawingState()
    {
        glUseProgram(0);
        glBindVertexArray(0);

        glEnable(GL_DEPTH_TEST);

        glPolygonMode( GL_FRONT_AND_BACK, m_FillMode[0] );
    }

}