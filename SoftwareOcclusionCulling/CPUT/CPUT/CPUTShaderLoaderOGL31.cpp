#include "CPUTShaderLoaderOGL31.h"
#include <memory.h> // for memset()

namespace CPUTGL31
{
    CPUTShaderLoader* CPUTShaderLoader::m_pShaderLoader = NULL;

    //  Static resource retriever
    //-----------------------------------------------------------------------------
    CPUTShaderLoader* CPUTShaderLoader::GetShaderLoader() 
    {            
        if(NULL==m_pShaderLoader)
            m_pShaderLoader = new CPUTShaderLoader;
        return m_pShaderLoader;
    }

    //  Initialize the shader loader system
    //-----------------------------------------------------------------------------
    CPUTResult CPUTShaderLoader::Initialize()
    {
        return CPUT_SUCCESS;
    }

    //  compile a shader from a file
    //-----------------------------------------------------------------------------
    CPUTResult CPUTShaderLoader::CompileShaderFromFile(const cString Filename, CPUT_SHADER_TYPE shadertype, GLuint& shaderID, CPUT_PATH_SEARCH_MODE eSearchmode )
    {
        shaderID = 0;
        CPUTResult result = CPUT_SUCCESS;

        CPUTOSServices* pServices = CPUTOSServices::GetOSServices();

        // find the file in the standard/specified search directory
        cString FoundFileAndPath;
        cString ErrorMessage;
        result = pServices->FindFileInStandardSearchDirectories(Filename, FoundFileAndPath, eSearchmode);
        if(CPUTFAILED(result))
            return result;

        // load the shader file into a string
        char* pFileContents = NULL;
        result = LoadStringTermintatedFile(FoundFileAndPath, (void**)&pFileContents);

        if(CPUTFAILED(result))
        {
            return result;
        }

        // we have text, give it a grind through the compiler
        GLuint shaderType=0;
        switch(shadertype)
        {
        case CPUT_VERTEX_SHADER:
            shaderID = glCreateShader(GL_VERTEX_SHADER);
            break;
            //case CPUT_GEOMETRY_SHADER:
            //  shaderID = glCreateShader(GL_GEOMETRY_SHADER);
            //break;
        case CPUT_PIXEL_SHADER:
            shaderID = glCreateShader(GL_FRAGMENT_SHADER);
            break;
        default:
            ErrorMessage = _L("Unknown shader type attempted to be registered.\n Shader file: ");
            ErrorMessage.append(Filename);
            ErrorMessage.append(_L(" not loaded."));
            //m_window->MessageBox("Shader Load Error", message, CPUT_MB_ERROR);
        }

        // set the shader source code
        glShaderSource(shaderID, 1, (const GLchar**)&pFileContents, NULL);

        // compile
        glCompileShader(shaderID);

        // error check
        cString message;
        if( CheckShaderCompileLog(shaderID, message) )
        {
            //m_window->MessageBox("Shader Load Error", message, CPUT_MB_ERROR);
            // error - return 0 for shader
            glDeleteShader(shaderID);
            shaderID=0;
        }

        // delete the program string
        delete [] pFileContents;
        pFileContents = NULL;

        return CPUT_SUCCESS;
    }


    // load a file and return it's contents
    //-----------------------------------------------------------------------------
    CPUTResult CPUTShaderLoader::LoadStringTermintatedFile(const cString Filename, void** ppData)
    {
        if(NULL == ppData)
            return CPUT_ERROR_INVALID_PARAMETER;

        FILE* pFile = NULL;
        errno_t err = fopen_s(&pFile, Filename.c_str(), "r");
        if(0 != err)
        {
            // error opening file
            //        char message[250];
            //        sprintf_s(message, 250, "Could not find file: %s\n", filename);
            //        MessageBox("Error: Load file", message, CPUT_MB_ERROR);
            return CPUT_ERROR_FILE_NOT_FOUND;
        }


        // file open - read contents
        // get file size
        err = fseek( pFile, 0, SEEK_END );
        if(0!=err)
        {
            //        char message[250];
            //        sprintf_s(message, 250, "Error getting file size: %s\n", filename);
            //        MessageBox("Error: Load file", message, CPUT_MB_ERROR);
            fclose(pFile);
            return CPUT_ERROR_FILE_READ_ERROR;
        }

        long endPos = ftell( pFile );

        fseek( pFile, 0, SEEK_SET );
        if(0!=err)
        {
            //        char message[250];
            //        sprintf_s(message, 250, "Error getting file size: %s\n", filename);
            //        MessageBox("Error: Load file", message, CPUT_MB_ERROR);
            fclose(pFile);
            return CPUT_ERROR_FILE_READ_ERROR;
        }

        // allocate buffer
        char* pContents = new char[endPos+1];

        // read the contents!
        size_t actuallyRead = fread(pContents, sizeof(char), endPos, pFile);
        pContents[actuallyRead] = '\0'; // always null terminate on your own - not gauranteed

        // close file
        if(pFile)
        {
            err = fclose(pFile);
            if(0 != err)
            {
                // file did not close properly!
                //           char message[250];
                //           sprintf_s(message, 250, "File did not close properly: %s\n", filename);
                //           MessageBox("Error: Load file", message, CPUT_MB_ERROR);
                delete [] pContents;
                return CPUT_ERROR_FILE_CLOSE_ERROR;
            }
        }

        // set the return buffer
        *ppData = pContents;

        return CPUT_SUCCESS;
    }

    // Create a window context
    //-----------------------------------------------------------------------------
    CPUTResult CPUTShaderLoader::CheckShaderCompileLog(GLuint shaderID, cString& Message)
    {
        CPUTResult result = CPUT_SUCCESS;

        GLsizei infoLength = 0;
        GLint shaderType = 0;

        // Check the Shader Info Log length.  If it's > 1, then there is something 
        // in the log that needs printing and an error almost certainly occured
        glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &infoLength);

        if(infoLength > 1) 
        {        
            char* pText=NULL;
            char* pShaderType = new char[50];
            GLchar* pInfo=NULL;
            int    length = 0;
            int actual = 0;

            // Determine which type of shader generated the message
            glGetShaderiv(shaderID, GL_SHADER_TYPE, &shaderType);

            switch(shaderType) 
            {
            case GL_VERTEX_SHADER:
                sprintf_s(pShaderType, 50, "vertex");
                break;
            case GL_FRAGMENT_SHADER:
                sprintf_s(pShaderType, 50, "fragment");
                break;
            case GL_GEOMETRY_SHADER_EXT:
                sprintf_s(pShaderType, 50, "geometry");
                break;
            default:
                sprintf_s(pShaderType, 50, "programmable");
            }

            // Allocate enough space for holding the message stored in the error log
            length = infoLength+1;
            pInfo = new GLchar[length];

            // Hold the extra 'error during compilation' text we're about to add to start of message
            length += (150 + 1);     
            pText = new char[length];

            // Extract the message from OpenGL's services and display it in a message box
            if(pText && pInfo)
            {
                memset(pText, 0, length);

                // Construct the error message
                glGetShaderInfoLog( shaderID, infoLength, &actual, pInfo);
                pInfo[infoLength] = '\0';

                Message = _L("Error compiling the ");
                cString convertString = pShaderType;
                Message.append(convertString);
                Message.append(_L(" shader:\n"));
                convertString = pInfo;
                Message.append( convertString );
                //sprintf_s(message, length, "Error compiling the %s shader:\n%s", pShaderType, pInfo);

                //m_window->MessageBox("OpenGL Shader Compile Error", message, CPUT_MB_ERROR);
                //delete [] message;

                /*
                // work-around for cards that report 'error - no error', discover the card, then uncomment rest to avoid it
                // LAME - the beta OpenGL Larrabee driver always returns 'No errors.' in the message
                // for compiling the shader
                if(0!=strcmp(pInfo, "No errors."))
                {
                sprintf_s(pText, (length-1), "Error during the %s shader compilation:\n%s", pShaderType, pInfo); 

                COSServices* pOSServices=COSServices::GetOSServices();
                if(pOSServices)
                {
                pOSServices->SGLMessageBox(pText, "StumpGL: Shader Compile Error", SGL_MESSAGE_BOX_OK);
                }
                } 
                else
                {
                bLarrabeeBetaDriverFalseError = true;
                }
                */

                // Free the resources, nulling the variables for safety
                delete [] pInfo;
                pInfo = NULL;

                delete [] pText;
                pText = NULL;

                delete [] pShaderType;
                pShaderType = NULL;

                result = CPUT_SHADER_COMPILE_ERROR;
            }
        }
        return result;
    }
}