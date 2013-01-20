//============================================================================
// Name        : CPUT-linux.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>

#include <stdio.h>
#include <stdlib.h>


using namespace std;

#include <stdio.h>

// windows specific headers
//#include <windows.h>
//#define DIRECTX

#ifdef DIRECTX
#include "CPUT_DX11.h"
#elif OPENGL31_WIN
#include "CPUT_OGL31.h"
#else
#include "CPUT-OGL31X.h"
#endif

/*
//-----------------------------------------------------------------------------
void OnKeyboardEvent(eKey key)
{
    if(KEY_Q == key)
    {
        printf("Q pressed");
        CPUTShutdown();
    }
    else
        printf("Key pressed");
}
*/
#ifdef DIRECTX
// DirectX 11 render callback
//-----------------------------------------------------------------------------
void Render(ID3D11DeviceContext* pImmediateContext, IDXGISwapChain* pSwapChain, ID3D11RenderTargetView* pRenderTargetView)
{
    /*
    DXOBJ* model = CPUTLoadModel();


    meshStruct mesh = CPUTLoadMesh();

    meshStruct
    {
        Vec3 vert;
        Vec3 Normal;
        Vec3
    }
    */

    // blue for DirectX
    float ClearColor[4] = { 0.0f, 0.0f, 1.0f, 1.0f }; //red,green,blue,alpha
    pImmediateContext->ClearRenderTargetView( pRenderTargetView, ClearColor );

//    DXUTDrawGui();

    pSwapChain->Present( 0, 0 );
}

#elif OPENGL31_WIN
// OpenGL render callback
//-----------------------------------------------------------------------------
void Render(HDC hdc)
{
    glClearColor (0.5, 0.0, 0.5, 1.0);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    SwapBuffers(hdc);

}
#else

#endif


//-----------------------------------------------------------------------------
int  main( )
{
    CPUTInitialize();
/*
    // Set callbacks
    CPUTSetKeyboardCallBack(OnKeyboardEvent);
    CPUTSetRenderCallback(Render);


    // Create gui
//    CPUTCreateGroup(IDC_NAMEGROUP);
//    CPUTAddButton(x,y,"push me", IDC_PUSHME, IDC_NAMEGROUP);
*/
    // create the context
    CPUTCreateContext();


    // start the main loop
    CPUTStart();

    return CPUTReturnCode();
}

