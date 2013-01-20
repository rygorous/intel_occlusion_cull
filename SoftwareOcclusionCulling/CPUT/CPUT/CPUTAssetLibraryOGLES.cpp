//--------------------------------------------------------------------------------------
// Copyright 2011 Intel Corporation
// All Rights Reserved
//
// Permission is granted to use, copy, distribute and prepare derivative works of this
// software for any purpose and without fee, provided, that the above copyright notice
// and this statement appear in all copies.  Intel makes no representations about the
// suitability of this software for any purpose.  THIS SOFTWARE IS PROVIDED "AS IS."
// INTEL SPECIFICALLY DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, AND ALL LIABILITY,
// INCLUDING CONSEQUENTIAL AND OTHER INDIRECT DAMAGES, FOR THE USE OF THIS SOFTWARE,
// INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PROPRIETARY RIGHTS, AND INCLUDING THE
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  Intel does not
// assume any responsibility for any errors which may appear in this software nor any
// responsibility to update it.
//--------------------------------------------------------------------------------------
#include "D3dx9tex.h"  // super-annoying - must be first or you get new() operator overloading errors during compile b/c of D3DXGetImageInfoFromFile() function

#include "CPUTAssetLibraryOGLES.h"

// define the objects we'll need
#include "CPUTModelOGLES.h"
//#include "CPUTMaterialOGLES.h"
#include "CPUTTextureOGLES.h"
#include "CPUTRenderStateBlockOGLES.h"
#include "CPUTLight.h"
#include "CPUTCamera.h"

#include "CPUTPixelShaderOGLES.h"

CPUTAssetLibrary   *CPUTAssetLibrary::m_pAssetLibrary = new CPUTAssetLibraryOGLES();
CPUTAssetListEntry *CPUTAssetLibraryOGLES::m_pPixelShaderList    = NULL;
CPUTAssetListEntry *CPUTAssetLibraryOGLES::m_pVertexShaderList   = NULL;
CPUTAssetListEntry *CPUTAssetLibraryOGLES::m_pGeometryShaderList = NULL;
CPUTAssetListEntry *CPUTAssetLibraryOGLES::m_pHullShaderList = NULL;
CPUTAssetListEntry *CPUTAssetLibraryOGLES::m_pDomainShaderList = NULL;

// TODO: Change OS Services to a flat list of CPUT* functions.  Avoid calls all over the place like:
// CPUTOSServices::GetOSServices();

// Deletes and properly releases all asset library lists that contain
// unwrapped IUnknown DirectX objects.
//-----------------------------------------------------------------------------
void CPUTAssetLibraryOGLES::ReleaseAllLibraryLists()
{
    // TODO: we really need to wrap the DX assets so we don't need to distinguish their IUnknown type.
    SAFE_RELEASE_LIST(m_pPixelShaderList);
    SAFE_RELEASE_LIST(m_pVertexShaderList);
    SAFE_RELEASE_LIST(m_pGeometryShaderList);
    SAFE_RELEASE_LIST(m_pHullShaderList);
    SAFE_RELEASE_LIST(m_pDomainShaderList);

    // Call base class implementation to clean up the non-DX object lists
    return CPUTAssetLibrary::ReleaseAllLibraryLists();
}

// Erase the specified list, Release()-ing underlying objects
//-----------------------------------------------------------------------------
void CPUTAssetLibraryOGLES::ReleaseIunknownList( CPUTAssetListEntry *pList )
{
    CPUTAssetListEntry* pNode = pList;
    CPUTAssetListEntry* pOldNode = NULL;

    while( NULL!=pNode )
    {
        // release the object using the DirectX IUnknown interface
        ((IUnknown*)(pNode->pData))->Release();
        pOldNode = pNode;
        pNode = pNode->pNext;
        delete pOldNode;
    }
    HEAPCHECK;
}

// Retrieve specified pixel shader
// TODO: Delegate creation to asset (AssetLibrary shouldn't know the nitty-gritty details of how to create an asset)
//-----------------------------------------------------------------------------
CPUTResult CPUTAssetLibraryOGLES::GetPixelShader(
    const cString        &name,
    const cString        &shaderMain,
    const cString        &shaderProfile,
    CPUTPixelShaderOGLES **ppPixelShader,
    bool                  nameIsFullPathAndFilename
)
{
    CPUTResult result = CPUT_SUCCESS;
    
    // Resolve name to absolute path before searching
    cString absolutePathAndFilename;
    CPUTOSServices* pServices = CPUTOSServices::GetOSServices();
    pServices->ResolveAbsolutePathAndFilename( nameIsFullPathAndFilename? name : (m_ShaderDirectoryName + name), &absolutePathAndFilename);

    // see if the shader is already in the library
    void *pShader = FindPixelShader(absolutePathAndFilename);
    if(NULL!=pShader)
    {
        *ppPixelShader = (CPUTPixelShaderOGLES*) pShader;
        (*ppPixelShader)->AddRef();
        return result;
    }

    // load the pixel shader as null-terminated char* string
    void* pShaderString=NULL;
    result = LoadShaderFileString(absolutePathAndFilename, &pShaderString);
    ASSERT( CPUTSUCCESS(result), _L("Error loading pixel shader: ")+name );

    // compile the pixel/fragment shader
    GLuint NewPixelShaderID = CompileShader(GL_FRAGMENT_SHADER, pShaderString);
    ASSERT( (0!=NewPixelShaderID), _L("Error compiling pixel shader: "+name) );

    // delete the shader's string now that it's no longer needed
    delete [] pShaderString;
    
    // store this new shader
    CPUTPixelShaderOGLES *pNewCPUTPixelShader = new CPUTPixelShaderOGLES( NewPixelShaderID );

    // add shader to library
    AddPixelShader(absolutePathAndFilename, pNewCPUTPixelShader);
    
    // return the shader 
    *ppPixelShader = pNewCPUTPixelShader;

    return result;
}

// Retrieve specified vertex shader
//-----------------------------------------------------------------------------
CPUTResult CPUTAssetLibraryOGLES::GetVertexShader(
    const cString         &name,
    const cString          &shaderMain,
    const cString          &shaderProfile,
    CPUTVertexShaderOGLES **ppVertexShader,
    bool                   nameIsFullPathAndFilename
)
{
    CPUTResult result = CPUT_SUCCESS;
    /*
    // Resolve name to absolute path before searching
    cString absolutePathAndFilename;
    CPUTOSServices* pServices = CPUTOSServices::GetOSServices();
    pServices->ResolveAbsolutePathAndFilename( nameIsFullPathAndFilename? name : (m_ShaderDirectoryName + name), &absolutePathAndFilename);

    // see if the shader is already in the library
    void *pShader = FindVertexShader(absolutePathAndFilename);
    if(NULL!=pShader)
    {
        *ppVertexShader = (CPUTVertexShaderDX11*) pShader;
        (*ppVertexShader)->AddRef();
        return result;
    }

    // TODO: Move to CPUTVertexShader class
    // compile the shader blob
    ID3DBlob           *pCompiledBlob = NULL;
    ID3D11VertexShader *pNewVertexShader = NULL;
    result = CompileShaderFromFile(absolutePathAndFilename, shaderMain, shaderProfile, &pCompiledBlob);
    ASSERT(CPUTSUCCESS(result), _L("Error compiling vertex shader:\n\n") );

    // Create the vertex shader with DirectX
    HRESULT hr = pD3dDevice->CreateVertexShader( pCompiledBlob->GetBufferPointer(), pCompiledBlob->GetBufferSize(), NULL, &pNewVertexShader );
    ASSERT(SUCCEEDED(result), _L("Error creating vertex shader '")+name+_L("'") );
    // cString DebugName = _L("CPUTAssetLibraryOGLES::GetVertexShader ")+name;
    // CPUTSetDebugName(pNewVertexShader, DebugName);

    CPUTVertexShaderDX11 *pNewCPUTVertexShader = new CPUTVertexShaderDX11(pNewVertexShader, pCompiledBlob);

    // add shader to library
    AddVertexShader(absolutePathAndFilename, pNewCPUTVertexShader);
    // pNewCPUTVertexShader->Release(); // We've added it to the library, so release our reference

    // return the newly created shader and blob (if they asked for it)
    *ppVertexShader = pNewCPUTVertexShader;
    */
    return result;
}

// Retrieve specified geometry shader
//-----------------------------------------------------------------------------
CPUTResult CPUTAssetLibraryOGLES::GetGeometryShader(
    const cString           &name,
    const cString           &shaderMain,
    const cString           &shaderProfile,
    CPUTGeometryShaderOGLES **ppGeometryShader,
    bool                     nameIsFullPathAndFilename
    )
{
    CPUTResult result = CPUT_SUCCESS;
    /*
    // Resolve name to absolute path before searching
    cString absolutePathAndFilename;
    CPUTOSServices* pServices = CPUTOSServices::GetOSServices();
    pServices->ResolveAbsolutePathAndFilename( nameIsFullPathAndFilename? name : (m_ShaderDirectoryName + name), &absolutePathAndFilename);

    // see if the shader is already in the library
    void *pShader = FindGeometryShader(absolutePathAndFilename);
    if(NULL!=pShader)
    {
        *ppGeometryShader = (CPUTGeometryShaderDX11*) pShader;
        (*ppGeometryShader)->AddRef();
        return result;
    }

    // compile the shader blob
    ID3DBlob* pCompiledBlob;
    ID3D11GeometryShader *pNewGeometryShader = NULL;

    result = CompileShaderFromFile(absolutePathAndFilename, shaderMain, shaderProfile, &pCompiledBlob);
    ASSERT(CPUTSUCCESS(result), _L("Error compiling geometry shader:\n\n") );

    // Create the geometry shader with DirectX
    HRESULT hr = pD3dDevice->CreateGeometryShader( pCompiledBlob->GetBufferPointer(), pCompiledBlob->GetBufferSize(), NULL, &pNewGeometryShader );
    ASSERT(SUCCEEDED(result), _L("Error creating geometry shader '")+name+_L("'") );

    CPUTGeometryShaderDX11 *pNewCPUTGeometryShader = new CPUTGeometryShaderDX11(pNewGeometryShader, pCompiledBlob);

    // add shader to library
    AddGeometryShader(absolutePathAndFilename, pNewCPUTGeometryShader);
    // pNewCPUTGeometryShader->Release(); // We've added it to the library, so release our reference
    */
    return CPUT_SUCCESS;
}

// Retrieve specified hull shader
//-----------------------------------------------------------------------------
CPUTResult CPUTAssetLibraryOGLES::GetHullShader(
    const cString           &name,
    const cString           &shaderMain,
    const cString           &shaderProfile,
    CPUTHullShaderOGLES **ppHullShader,
    bool                     nameIsFullPathAndFilename
    )
{
    CPUTResult result = CPUT_SUCCESS;
    /*
    // Resolve name to absolute path before searching
    cString absolutePathAndFilename;
    CPUTOSServices* pServices = CPUTOSServices::GetOSServices();
    pServices->ResolveAbsolutePathAndFilename( nameIsFullPathAndFilename? name : (m_ShaderDirectoryName + name), &absolutePathAndFilename);

    // see if the shader is already in the library
    void *pShader = FindHullShader(absolutePathAndFilename);
    if(NULL!=pShader)
    {
        *ppHullShader = (CPUTHullShaderDX11*) pShader;
        (*ppHullShader)->AddRef();
        return result;
    }

    // compile the shader blob
    ID3DBlob* pCompiledBlob;
    ID3D11HullShader *pNewHullShader = NULL;

    result = CompileShaderFromFile(absolutePathAndFilename, shaderMain, shaderProfile, &pCompiledBlob);
    ASSERT(CPUTSUCCESS(result), _L("Error compiling hull shader:\n\n") );

    // Create the geometry shader with DirectX
    HRESULT hr = pD3dDevice->CreateHullShader( pCompiledBlob->GetBufferPointer(), pCompiledBlob->GetBufferSize(), NULL, &pNewHullShader );
    ASSERT(SUCCEEDED(result), _L("Error creating hull shader '")+name+_L("'") );

    CPUTHullShaderDX11 *pNewCPUTHullShader = new CPUTHullShaderDX11(pNewHullShader, pCompiledBlob);

    // add shader to library
    AddHullShader(absolutePathAndFilename, pNewCPUTHullShader);
    // pNewCPUTHullShader->Release(); // We've added it to the library, so release our reference
    */
    return CPUT_SUCCESS;
}

// Retrieve specified domain shader
//-----------------------------------------------------------------------------
CPUTResult CPUTAssetLibraryOGLES::GetDomainShader(
    const cString           &name,
    const cString           &shaderMain,
    const cString           &shaderProfile,
    CPUTDomainShaderOGLES **ppDomainShader,
    bool                     nameIsFullPathAndFilename
    )
{
    CPUTResult result = CPUT_SUCCESS;
    /*
    // Resolve name to absolute path before searching
    cString absolutePathAndFilename;
    CPUTOSServices* pServices = CPUTOSServices::GetOSServices();
    pServices->ResolveAbsolutePathAndFilename( nameIsFullPathAndFilename? name : (m_ShaderDirectoryName + name), &absolutePathAndFilename);

    // see if the shader is already in the library
    void *pShader = FindDomainShader(absolutePathAndFilename);
    if(NULL!=pShader)
    {
        *ppDomainShader = (CPUTDomainShaderDX11*) pShader;
        (*ppDomainShader)->AddRef();
        return result;
    }

    // compile the shader blob
    ID3DBlob* pCompiledBlob;
    ID3D11DomainShader *pNewDomainShader = NULL;

    result = CompileShaderFromFile(absolutePathAndFilename, shaderMain, shaderProfile, &pCompiledBlob);
    ASSERT(CPUTSUCCESS(result), _L("Error compiling domain shader:\n\n") );

    // Create the geometry shader with DirectX
    HRESULT hr = pD3dDevice->CreateDomainShader( pCompiledBlob->GetBufferPointer(), pCompiledBlob->GetBufferSize(), NULL, &pNewDomainShader );
    ASSERT(SUCCEEDED(result), _L("Error creating domain shader '")+name+_L("'") );

    CPUTDomainShaderDX11 *pNewCPUTDomainShader = new CPUTDomainShaderDX11(pNewDomainShader, pCompiledBlob);

    // add shader to library
    AddDomainShader(absolutePathAndFilename, pNewCPUTDomainShader);
    // pNewCPUTDomainShader->Release(); // We've added it to the library, so release our reference
    */
    return CPUT_SUCCESS;
}


// Use DX11 compile from file method to do all the heavy lifting
//-----------------------------------------------------------------------------
CPUTResult CPUTAssetLibraryOGLES::CompileShaderFromFile(
    const cString  &fileName,
    const cString  &shaderMain,
    const cString  &shaderProfile
)
{
    CPUTResult result = CPUT_SUCCESS;
    /*
    char pShaderMainAsChar[128];
    char pShaderProfileAsChar[128];
    ASSERT( shaderMain.length()     < 128, _L("Shader main name '")    + shaderMain    + _L("' longer than 128 chars.") );
    ASSERT( shaderProfile.length()  < 128, _L("Shader profile name '") + shaderProfile + _L("' longer than 128 chars.") );
    size_t count;
    wcstombs_s( &count, pShaderMainAsChar,    shaderMain.c_str(),    128 );
    wcstombs_s( &count, pShaderProfileAsChar, shaderProfile.c_str(), 128 );

    // use DirectX to compile the shader file
    ID3DBlob* pErrorBlob = NULL;
    D3D10_SHADER_MACRO pShaderMacros[2] = { "_CPUT", "1", NULL, NULL };
    HRESULT hr = D3DX11CompileFromFile(
        fileName.c_str(),     // fileName
        pShaderMacros,        // macro define's
        NULL,                 // includes
        pShaderMainAsChar,    // main function name
        pShaderProfileAsChar, // shader profile/feature level
        0,                    // flags 1
        0,                    // flags 2
        NULL,                 // threaded load? (no for right now)
        ppBlob,               // blob data with compiled code
        &pErrorBlob,          // any compile errors stored here
        NULL
    );
    ASSERT( SUCCEEDED(hr), _L("Error compiling shader '") + fileName + _L("'.\n") + (pErrorBlob ? s2ws((char*)pErrorBlob->GetBufferPointer()) : _L("no error message") ) );
    if(pErrorBlob)
    {
        pErrorBlob->Release();
    }
    */
    return result;
}

// load a file and return it's contents
//-----------------------------------------------------------------------------
CPUTResult CPUTAssetLibraryOGLES::LoadShaderFileString(const cString Filename, void** ppData)
{
    if(NULL == ppData)
    {
        ASSERT( ppData, _L("Invalid data pointer for storing shader string"));
        return CPUT_ERROR_INVALID_PARAMETER;
    }

    FILE* pFile = NULL;
#if defined (UNICODE) || defined(_UNICODE)
    errno_t err = _wfopen_s(&pFile, Filename.c_str(), _L("r"));
#else
    errno_t err = fopen_s(&pFile, Filename.c_str(), _L("r"));
#endif
    ASSERT( (0 == err), _L("Error loading shader file: ")+Filename);
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
    ASSERT( (0 == err), _L("Error reading contents of shader file: ")+Filename);
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
    ASSERT( (0 == err), _L("Error getting size of shader file: ")+Filename);
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
    pContents[actuallyRead] = '\0'; // always null terminate on your own - not guaranteed by fread()

    // close file
    err = fclose(pFile);
    ASSERT( (0 == err), _L("Error closing shader file: ")+Filename);

    
    if(pFile)
    {   
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

// Compile an OpenGL|ES shader and assert errors (if any)
//-----------------------------------------------------------------------------
GLuint CPUTAssetLibraryOGLES::CompileShader( GLenum ShaderType, void* pShaderString )
{
    GLuint shader;
    GLint compiled;

    ASSERT( (pShaderString!=NULL), _L("Invalid shader string pointer"));

    // Create the shader object
    shader = glCreateShader ( ShaderType );
    ASSERT( 0!=shader, _L("glCreateShader failed"));

    // Load the shader source
    glShaderSource ( shader, 1, (const GLchar**) &pShaderString, NULL );

    // Compile the shader
    glCompileShader ( shader );

    // Check the compile status
    glGetShaderiv ( shader, GL_COMPILE_STATUS, &compiled );

    if ( !compiled ) 
    {
        GLint infoLen = 0;

        glGetShaderiv ( shader, GL_INFO_LOG_LENGTH, &infoLen );

        if ( infoLen > 1 )
        {
            char* infoLog = (char*) malloc (sizeof(char) * infoLen );

            glGetShaderInfoLog ( shader, infoLen, NULL, infoLog );
            cString ErrorMessage = s2ws(infoLog);

            //esLogMessage ( "Error compiling shader:\n%s\n", infoLog );
            free ( infoLog );
            ASSERT(0, _L("ERROR compiling OGLES shader: ")+ErrorMessage);
            
        }
        glDeleteShader ( shader );
        return 0;
    }

    return shader;
}