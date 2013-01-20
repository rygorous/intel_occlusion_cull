#include <stdio.h>

//#define DIRECTX

#ifdef DIRECTX
    #include "CPUT_DX11.h"
    #include <D3D11.h> // for D3D11_BUFFER_DESC
    #include <xnamath.h> // for XMFLOAT
    
#else
    #include "CPUT_OGL31.h"
#include <math.h> // for tan/cos/sin/etc
#endif

#ifdef DIRECTX
struct WorldViewProjectionConstantBuffer
{
    XMMATRIX world;
    XMMATRIX view;
    XMMATRIX projection;
};


// DirectX 11
//-----------------------------------------------------------------------------
class MySample:public CPUT_DX11
{
public:
    MySample();
    void OnKeyboardEvent(eKey key);
    //void OnResize(int width, int height);

    void OnCreate(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pImmediateContext, IDXGISwapChain* pSwapChain);
    void Render(ID3D11DeviceContext* pImmediateContext, IDXGISwapChain* pSwapChain, ID3D11RenderTargetView* pRenderTargetView);    
    void OnShutdown();

private:
    ID3D11VertexShader* m_vertexShader;
    ID3D11PixelShader* m_pixelShader;
    
    ID3D11InputLayout* m_pVertexLayout;
    ID3D11Buffer*      m_pVertexBuffer;

    ID3D11Buffer*      m_pBuffers[3];
    
    CPUTModel*          m_model;

    ID3D11ShaderResourceView* m_pTextureResView;
    ID3D11SamplerState*      m_pSamplerStateLinear;


    ID3D11Buffer*                       m_pCBWorldMatrix; 
    ID3D11Buffer*                       m_pCBViewMatrix; 
    


};
#else

// global matricies
float g_projectionMatrix[] = { 1.0f, 0.0f, 0.0f, 0.0f, 
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f};
float g_modelViewMatrix[] = { 1.0f, 0.0f, 0.0f, 0.0f, 
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f};
const float PI=3.14159f;
const float PI_OVER_360 = PI/360.0f;

// OpenGL 3.1
//-----------------------------------------------------------------------------
class MySample:public CPUT_OGL31
{
public:
    MySample();
    void OnKeyboardEvent(eKey key);
    void OnCreate(HDC hdc);
    void Render(HDC hdc);
    void OnShutdown();
    void OnSize(int width, int height);

    void BuildPerspProjMat(float *m, float fov, float aspect, float znear, float zfar);
    void glhLookAtf2( float *matrix, float *eyePosition3D, float *center3D, float *upVector3D );

private:
    GLuint  m_shaderProgram;
    GLuint  m_vertexShader;
    GLuint  m_pixelShader;

	GLuint m_VertexArrayObjectID;			// vertex array object, one for each drawn object
	GLuint m_VertexBufferObjectID[3];		// two VBOs: pos+color

    // textures
    GLint m_textureUniformLocation;
    GLuint m_textureID;

    GLuint m_ModelViewMatrixID;
    GLuint m_ProjectionMatrixID;

    // models
    CPUTModel* m_model;
};
#endif




// Handle keyboard events
//-----------------------------------------------------------------------------
void MySample::OnKeyboardEvent(eKey key)
{
    if(KEY_Q == key)
    {
        printf("Q pressed");
        Shutdown();
    }
    else
        printf("Key pressed");
}





#ifdef DIRECTX

// Constructor
//-----------------------------------------------------------------------------
MySample::MySample():m_vertexShader(NULL),
    m_pixelShader(NULL),
    m_pVertexLayout(NULL),
    m_pVertexBuffer(NULL),
    m_pSamplerStateLinear(NULL),
    m_pTextureResView(NULL),
    m_model(NULL)
{

}

//
//// Handle resize events
////-----------------------------------------------------------------------------
//void MySample::OnResize(int width, int height)
//{
//
//}


// Handle OnCreation events
//-----------------------------------------------------------------------------
void MySample::OnCreate(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pImmediateContext, IDXGISwapChain* pSwapChain)
{
    HRESULT result;

    // add a control
    CPUTControlID ID_PUSHME = 101;
    CPUTButton* button = new CPUTButton("Push me!", ID_PUSHME);


    // load the shaders
    ID3D10Blob* pVertexShaderBlob = NULL;
    m_vertexShader = CPUTCompileVertexShaderFromFile("..\\shaders\\vertexShaderDX11.vs", "VS", "vs_4_0", &pVertexShaderBlob );
    m_pixelShader = CPUTCompilePixelShaderFromFile("..\\shaders\\pixelShaderDX11.ps", "PS", "ps_4_0" );
    

    m_model = CPUTGetModel("hard-coded-for-now.model", pVertexShaderBlob);


    // constant buffers for world/view/projection camera control
    D3D11_BUFFER_DESC bd;
    ZeroMemory( &bd, sizeof(bd) );
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(WorldViewProjectionConstantBuffer);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    result = pd3dDevice->CreateBuffer( &bd, NULL, &m_pCBWorldMatrix );
    if( FAILED( result ) )
        return;
    

    
    // Load the Texture
    m_pTextureResView = CPUTGetTexture("..\\media\\testPattern_01_diff.png");
    //result = D3DX11CreateShaderResourceViewFromFile( pd3dDevice, "testPattern_01_diff.png", NULL, NULL, &m_pTextureResView, NULL );
    if( NULL == m_pTextureResView )
        return;

    // Create the sample state
    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory( &sampDesc, sizeof(sampDesc) );
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    result = pd3dDevice->CreateSamplerState( &sampDesc, &m_pSamplerStateLinear );
    if( FAILED( result ) )
        return;



    // create and set the blend state - std non-premultplied alpha blending
    ID3D11BlendState* pBlendState = NULL;

    D3D11_BLEND_DESC BlendStateDesc;
    ZeroMemory(&BlendStateDesc, sizeof(D3D11_BLEND_DESC));

    BlendStateDesc.AlphaToCoverageEnable=true;     // creates 'hashed' shading, ideally you'd turn this off and sort your polys
    BlendStateDesc.IndependentBlendEnable = false;  // use this blend mode for all render target layers

    BlendStateDesc.RenderTarget[0].BlendEnable = true;
    BlendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    BlendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    BlendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    BlendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
    BlendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    BlendStateDesc.RenderTarget[0].BlendOpAlpha =D3D11_BLEND_OP_ADD;
    BlendStateDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    
    pd3dDevice->CreateBlendState(&BlendStateDesc, &pBlendState);

     float blendFactor[] = {1.0f, 1.0f, 1.0f, 1.0f};
     UINT sampleMask   = 0xffffffff;
     pImmediateContext->OMSetBlendState(pBlendState, blendFactor, sampleMask);


}

// DirectX 11 render callback
//-----------------------------------------------------------------------------
void MySample::Render(ID3D11DeviceContext* pImmediateContext, IDXGISwapChain* pSwapChain, ID3D11RenderTargetView* pRenderTargetView)
{
    static float rotation = 90.0f;

    // clear color/back buffer = blue for DirectX
    float ClearColor[4] = { 0.0f, 0.0f, 1.0f, 1.0f }; //red,green,blue,alpha
    pImmediateContext->ClearRenderTargetView( pRenderTargetView, ClearColor );

    // clear the depth buffer
    pImmediateContext->ClearDepthStencilView(m_depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);


	
    
    // set up the view matricies
    const float PI=3.1415f;    
    WorldViewProjectionConstantBuffer cbWoldViewProjection; 

    // Initialize the view matrix
    XMVECTOR EyePosition = XMVectorSet(0.0f, -2.0f, -1.0f, 0.0f );
    XMVECTOR LookAt = XMVectorSet( 0.0f,-1.0f, 0.0f, 0.0f );
    XMVECTOR Up = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );
    
    XMMATRIX view = XMMatrixLookAtLH( EyePosition, LookAt, Up );
    cbWoldViewProjection.view = XMMatrixTranspose( view );

    // Initialize the projection matrix
    int x,y,width,height;
    CPUTGetWindowDimensions(x,y,width,height);
    float aspectRatio = ((float)width)/((float)height);
    XMMATRIX projection = XMMatrixPerspectiveFovLH( PI/4.0f, aspectRatio, 0.01f, 1000.0f );    
    cbWoldViewProjection.projection = XMMatrixTranspose( projection );
    //cbWoldViewProjection.projection = XMMatrixIdentity();


    // set up the model/world matrix   
    XMMATRIX world = XMMatrixIdentity(); 
    XMMATRIX translate = XMMatrixTranslation(0,0,1); //XMMatrixIdentity();   
    XMMATRIX model( m_model->GetTransform() );

    rotation += (float)PI * 0.0001f;
    if(rotation > 360.0f)
    {
        rotation -= 360.0f;
    }
    XMMATRIX rotY = XMMatrixRotationY(rotation);
    world = rotY * translate * model * world ;
    cbWoldViewProjection.world = XMMatrixTranspose( world );

        
    // update the constant-buffer matricies
    pImmediateContext->UpdateSubresource( m_pCBWorldMatrix, 0, NULL, &cbWoldViewProjection, 0, 0 );
    pImmediateContext->VSSetConstantBuffers( 0, 1, &m_pCBWorldMatrix );


    for(int i=0; i<m_model->GetNumberOfMeshes(); i++)
    {   
        // get each mesh of the model and it's material
        CPUTMesh* mesh = m_model->GetMesh(i);
        CPUTMaterial* material = mesh->GetMaterial();

        // set the vertex shader (stored in CPUTMaterial?)
        pImmediateContext->VSSetShader( m_vertexShader, NULL, 0 );

        // Set the shader 
        pImmediateContext->PSSetShader( m_pixelShader, NULL, 0 );


        // Set the sampler (i.e. texture) (stored in CPUTMaterial)
        pImmediateContext->PSSetShaderResources( 0, 1, &m_pTextureResView );
        pImmediateContext->PSSetSamplers( 0, 1, &m_pSamplerStateLinear );

        // draw each mesh!          
        mesh->Draw(pImmediateContext);

    }

    // draw all the gui items
    CPUTDrawGUI();

    // done - show them your hard work!
    pSwapChain->Present( 0, 0 );
}

// Handle the shutdown event - clean up everything you created
//-----------------------------------------------------------------------------
void MySample::OnShutdown()
{
    if( m_vertexShader ) m_vertexShader->Release();
    if( m_pixelShader ) m_pixelShader->Release();

    if( m_pVertexLayout ) m_pVertexLayout->Release();
    if( m_pVertexBuffer ) m_pVertexBuffer->Release();
        
}

#else

// Constructor
//-----------------------------------------------------------------------------
MySample::MySample():m_shaderProgram(0), 
    m_vertexShader(0), 
    m_pixelShader(0),
    m_model(NULL)
{
}


// Handle OnCreation events
//-----------------------------------------------------------------------------
void MySample::OnCreate(HDC hdc)
{
    // Create a control
    CPUTControlID ID_PUSHME = 101;
    CPUTButton* button = new CPUTButton("Push me!", ID_PUSHME);



    // Load shaders
    m_vertexShader = CPUTCompileShaderFromFile("..\\shaders\\vertexShaderGL.vs", CPUT_VERTEX_SHADER );
    m_pixelShader = CPUTCompileShaderFromFile("..\\shaders\\pixelShaderGL.ps", CPUT_PIXEL_SHADER );

    GLint compiledVertex, compiledPixel;
    glGetShaderiv(m_vertexShader, GL_COMPILE_STATUS, &compiledVertex);
    glGetShaderiv(m_pixelShader, GL_COMPILE_STATUS, &compiledPixel);

    if( (0==m_vertexShader) || (0==m_pixelShader) || (!compiledVertex) || (!compiledPixel) )
    {
        CPUTMessageBox("Error", "Error loading shaders", CPUT_MB_ERROR);
    }
    
    // create a new shader program, and attach the shaders
    m_shaderProgram = glCreateProgram();

    // link up the variables with shader inputs/outputs
    glBindAttribLocation(m_shaderProgram, 0, "in_Position");
    glBindAttribLocation(m_shaderProgram, 1, "in_Color");
    glBindAttribLocation(m_shaderProgram, 2, "in_UV");

    // attach the shaders to a program
    glAttachShader(m_shaderProgram, m_vertexShader);
    glAttachShader(m_shaderProgram, m_pixelShader);
    
    // link and test results
    glLinkProgram(m_shaderProgram);
    char* message = NULL;
    if( !CPUTCheckProgramCompileLog(m_shaderProgram, &message ) )
    {
        CPUTMessageBox("Error during shader link step", message, CPUT_MB_ERROR);
        delete [] message;
    }
    
    // bind the program
    glUseProgram(m_shaderProgram);



    // load model
    m_model = CPUTGetModel("hard-coded-for-now.model");




    // load a texture
    m_textureID = CPUTGetTexture("..\\media\\testPattern_01_diff.png");

    // bind it
    glBindTexture(GL_TEXTURE_2D, m_textureID);

    // get the uniform variable location in the shader for the diffuse texture
    m_textureUniformLocation = glGetUniformLocation(m_shaderProgram, "diffuseTexture");
     
    //Bind to tex unit 0
    glUniform1i(m_textureUniformLocation, 0);

    // turn on texture unit 0
    glActiveTexture(GL_TEXTURE0);

    // bind texture to texture unit 0
    glBindTexture(GL_TEXTURE_2D, m_textureID);




    // Set up model/view/projection matricies
    int x,y,width,height;
    CPUTGetWindowDimensions(x,y, width, height);
    
    float fov=60.0f; // in degrees
    float aspect= (float)width/height; //1.3333f;
    float znear=1.0f;
    float zfar=1000.0f;
    BuildPerspProjMat(g_projectionMatrix, fov, aspect, znear, zfar);

    m_ProjectionMatrixID = glGetUniformLocation(m_shaderProgram, "ProjectionMatrix");
    m_ModelViewMatrixID = glGetUniformLocation(m_shaderProgram, "ModelViewMatrix");

    glUniformMatrix4fv(m_ProjectionMatrixID, 1, GL_TRUE, g_projectionMatrix );  // GL_TRUE means transpose
    glUniformMatrix4fv(m_ModelViewMatrixID, 1, GL_TRUE, g_modelViewMatrix );    // GL_TRUE means transpose
    
      
    // enable alpha blending
    glEnable(GL_BLEND);
    glAlphaFunc(GL_GREATER, 0.1f);
    glEnable(GL_ALPHA_TEST);    
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // enable depth testing - todo: move this to OGL initialization code
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    //glEnable(GL_CULL_FACE); 

}

// handle re-size events
//-----------------------------------------------------------------------------
void MySample::OnSize(int width, int height) 
{
    glViewport(0, 0, width, height);

    // Set up model/view/projection matricies
   
    float fov=60.0f; // in degrees
    float aspect= (float)width/height; //1.3333f;
    float znear=1.0f;
    float zfar=1000.0f;
    BuildPerspProjMat(g_projectionMatrix, fov, aspect, znear, zfar);

    glUniformMatrix4fv(m_ProjectionMatrixID, 1, GL_TRUE, g_projectionMatrix );  // GL_TRUE means transpose
}

// OpenGL projection matrix calculator
//-----------------------------------------------------------------------------
void MySample::BuildPerspProjMat(float *m, float fov, float aspect, float znear, float zfar)
{
  float xymax = znear * tan(fov * PI_OVER_360);
  float ymin = -xymax;
  float xmin = -xymax;

  float width = xymax - xmin;
  float height = xymax - ymin;

  float depth = zfar - znear;
  float q = -(zfar + znear) / depth;
  float qn = -2 * (zfar * znear) / depth;

  float w = 2 * znear / width;
  w = w / aspect;
  float h = 2 * znear / height;

  m[0] = w;     m[4] = 0;   m[8]  = 0;  m[12] = 0;
  m[1] = 0;     m[5] = h;   m[9]  = 0;  m[13] = 0;
  m[2] = 0;     m[6] = 0;   m[10] = q;  m[14] = qn;
  m[3] = 0;     m[7] = 0;   m[11] =-1;  m[15] = 0;
}

// OpenGL look-at
//-----------------------------------------------------------------------------
void MySample::glhLookAtf2( float *matrix, float *eyePosition3D, float *center3D, float *upVector3D )
{/*
   float forward[3], side[3], up[3];
   float matrix2[16], resultMatrix[16];
   //------------------
   forward[0] = center3D[0] - eyePosition3D[0];
   forward[1] = center3D[1] - eyePosition3D[1];
   forward[2] = center3D[2] - eyePosition3D[2];
   NormalizeVector(forward);
   //------------------
   //Side = forward x up
   ComputeNormalOfPlane(side, forward, upVector3D);
   NormalizeVector(side);
   //------------------
   //Recompute up as: up = side x forward
   ComputeNormalOfPlane(up, side, forward);
   //------------------
   matrix2[0] = side[0];
   matrix2[4] = side[1];
   matrix2[8] = side[2];
   matrix2[12] = 0.0;
   //------------------
   matrix2[1] = up[0];
   matrix2[5] = up[1];
   matrix2[9] = up[2];
   matrix2[13] = 0.0;
   //------------------
   matrix2[2] = -forward[0];
   matrix2[6] = -forward[1];
   matrix2[10] = -forward[2];
   matrix2[14] = 0.0;
   //------------------
   matrix2[3] = matrix2[7] = matrix2[11] = 0.0;
   matrix2[15] = 1.0;
   //------------------
   MultiplyMatrices4by4OpenGL_FLOAT(resultMatrix, matrix, matrix2);
   glhTranslatef2(resultMatrix,
                  -eyePosition3D[0], -eyePosition3D[1], -eyePosition3D[2]);
   //------------------
   memcpy(matrix, resultMatrix, 16*sizeof(float));*/
}


// OpenGL render callback
//-----------------------------------------------------------------------------
void MySample::Render(HDC hdc)
{
    // red for OpenGL
    glClearColor (1.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    // use the shader we put together in the OnCreate function
    glUseProgram(m_shaderProgram);
    
    
    // set up the rotation matrix for the world-model matrix
    static float theta = 0.0f;
    theta += (float)PI * 0.01f;
    if(theta > 2*PI)
        theta -= 2*PI;
    float rotationMatrix[] = { cos(theta),  0.0f,   sin(theta),   0.0f, 
                        0.0f,       1.0f,   0.0f,         -0.7f,
                       -sin(theta), 0.0f,   cos(theta),  -1.5f,
                        0.0f,       0.0f,   0.0f,         1.0f};

    glUniformMatrix4fv(m_ModelViewMatrixID, 1, GL_TRUE, rotationMatrix); //g_modelViewMatrix );


    
    // set the shader program (vertex+pixel+geom) for this model
    glUseProgram(m_shaderProgram);

    // draw each of the mesh's for this model
    for(int i=0; i<m_model->GetNumberOfMeshes(); i++)
    {   
        // get each mesh of the model and it's material
        CPUTMesh* mesh = m_model->GetMesh(i);
        CPUTMaterial* material = mesh->GetMaterial();

        // Set any material state here 
        // ..textures..VS's, PS's, etc
        material;
        glBindTexture(GL_TEXTURE_2D, m_textureID);

        // draw each mesh!          
        mesh->Draw();

    }

    glBindVertexArray(0);

    // draw second cube below and with different rotation
    float theta2 = theta;
    theta2+= (float)PI/4.0f;
    if(theta2 > 2*PI)
        theta2 -= 2*PI;
    float rotationMatrix2[] = { cos(theta2),  0.0f,   sin(theta2),   0.0f, 
                        0.0f,       1.0f,   0.0f,         0.7f,
                       -sin(theta2), 0.0f,   cos(theta2),  -1.5f,
                        0.0f,       0.0f,   0.0f,         1.0f};

    glUniformMatrix4fv(m_ModelViewMatrixID, 1, GL_TRUE, rotationMatrix2); //g_modelViewMatrix );
    for(int i=0; i<m_model->GetNumberOfMeshes(); i++)
    {   
        // get each mesh of the model and it's material
        CPUTMesh* mesh = m_model->GetMesh(i);
        CPUTMaterial* material = mesh->GetMaterial();

        // Set any material state here 
        // ..textures..VS's, PS's, etc
        material;
        glBindTexture(GL_TEXTURE_2D, m_textureID);

        // draw each mesh!          
        mesh->Draw();

    }
    

    // draw all the gui items
    CPUTDrawGUI();    

    // done - show them your hard work!
    glFlush();
    SwapBuffers(hdc);
}


// OpenGL render callback
//-----------------------------------------------------------------------------
void MySample::OnShutdown()
{
    // unbind any VBO's
    glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // delete the VBO's
	glDeleteBuffers(3, m_VertexBufferObjectID);

    // unbind and delete the VAO
    glBindVertexArray(0);
	glDeleteVertexArrays(1, &m_VertexArrayObjectID);

    // detatch and delete the shaders
    glDetachShader(m_shaderProgram, m_vertexShader);
    glDetachShader(m_shaderProgram, m_pixelShader);

    glDeleteShader(m_vertexShader);
    glDeleteShader(m_pixelShader);

    // delete the shader program
    glDeleteProgram(m_shaderProgram);

    // delete the texture(s)
    glDeleteTextures(1, &m_textureID);

}

#endif



// Entrypoint for your sample
//-----------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    bool success = true;
    MySample* sample = new MySample();


    sample->Initialize();
#ifdef DIRECTX
    success = sample->CreateContext("CPUTWindow DirectX 11");
#else
    success = sample->CreateContext("CPUTWindow OpenGL 3.1");
#endif

    if(true == success)
    {
        sample->Start();
    }

    return sample->ReturnCode();
}










