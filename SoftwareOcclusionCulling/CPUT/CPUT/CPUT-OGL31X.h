#ifndef cput_h
#define cput_h

#include <stdio.h>

// include all OpenGL headers needed
#include "GL//glew.h"
#include "GL//glxew.h"


// include project headers we'll need
#include "WindowX.h"

//namespace CPUT {

//Callback defs
//-----------------------------------------------------------------------------
/*
typedef void (*LPCPUTCALLBACK_ONKEY)( eKey key );
typedef void (*LPCPUTCALLBACK_RENDER)( HDC hdc );
*/

// OpenGL 3.1
class CPUT_Internals
{
public:
    CPUT_Internals();
    WindowX* m_window;

    // user's sample callbacks
/*    LPCPUTCALLBACK_RENDER   UserRender;

    HDC     m_hdc;
    HGLRC   m_hrc;
*/

    GLXContext 		m_glxContext;
};

// initialize
CPUT_Internals::CPUT_Internals():m_glxContext(0)
{
}

// ENUMS
//-----------------------------------------------------------------------------
enum CPUTButtonStatus
{
    CPUT_MOUSE_NO_BUTTON    = 0,
    CPUT_MOUSE_LEFT_BUTTON  = 2^0,
    CPUT_MOUSE_MIDDLE_BUTTON  = 2^1,
    CPUT_MOUSE_RIGHT_BUTTON  = 2^2,

};


// GLOBAL VARIABLES
//-----------------------------------------------------------------------------
CPUT_Internals* g_Internals = NULL;



//-----------------------------------------------------------------------------
void CPUTInitialize()
{
    if(g_Internals)
    {
        // destroy window and all data

        // delete object
        delete g_Internals;
    }
    g_Internals = new CPUT_Internals();
}



//-----------------------------------------------------------------------------
void CPUTCreateWindow()
{
    // create the OS window
    g_Internals->m_window = new WindowX();
    g_Internals->m_window->Create();
}


//-----------------------------------------------------------------------------
int CPUTCreateGL31Context()
{
    GLenum err = glewInit();
	if (GLEW_OK != err)
	{
        printf("GLEW is not initialized!\n");
	}

	const int attribs[] =
	{
		GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
		GLX_CONTEXT_MINOR_VERSION_ARB, 1,
		GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
		0
	};

    if(glxewIsSupported("GLX_ARB_create_context") == 1)
    {
    	/*
    	GLXContext share_context = NULL; // not sharing contexts for now
    	bool direct = true; // try to directly render to/access the rendering device (not possible on non-local X-servers, hw context limits, no hw rasterizer for driver)

    	g_Internals->m_glxContext = glXCreateContextAttribsARB(g_Internals->m_window->GetDisplay(), *g_Internals->m_window->GetGLXFBConfig(), share_context, direct, attribs);

    	GLXDrawable	 glxDrawable;
		glXMakeCurrent(g_Internals->m_window->GetDisplay(), glxDrawable, g_Internals->m_glxContext);

		// delete the old, 2.0 context
		glXDestroyContext(g_Internals->m_window->GetDisplay() ,g_Internals->m_window->GetContext());

		// update the Xwindow class with the new 3.1 context
		g_Internals->m_window->SetContext( g_Internals->m_glxContext );

		// make the 3.1 context current
		glXMakeCurrent(g_Internals->m_window->GetDisplay(), g_Internals->m_window->GetWindow(), g_Internals->m_glxContext);
*/
        GLenum err = glGetError();
        if(err)
        {

        }

	}
	else
	{	//It's not possible to make a GL 3.x context. Use the old style context (GL 2.1 and before)
		//g_Internals->m_hrc = tempContext;
        printf("Not possible to make a GL 3.x context, defaulting to OpenGL 2.1\n");
	}
/*
	//Checking GL version
	const GLubyte* GLVersionString = glGetString(GL_VERSION);

	//Or better yet, use the GL3 way to get the version number
	int OpenGLVersion[2];
	glGetIntegerv(GL_MAJOR_VERSION, &OpenGLVersion[0]);
	glGetIntegerv(GL_MINOR_VERSION, &OpenGLVersion[1]);

	//if (!g_Internals->m_hrc)
      //  return S_FALSE;
*/
    return true;
}

//-----------------------------------------------------------------------------
void CPUTRender()
{
    // start timer

    // call user drawing routine
    //if(g_Internals->UserRender)
    //    g_Internals->UserRender(g_Internals->m_hdc);


    // end frame timer


}

//-----------------------------------------------------------------------------
void CPUTCreateContext()
{
    // create the window
    CPUTCreateWindow();

    // create the DX context
    CPUTCreateGL31Context();
}





//-----------------------------------------------------------------------------
void CPUTStart()
{
    g_Internals->m_window->StartMessageLoop();
}
/*
//-----------------------------------------------------------------------------
void CPUTShutdown()
{
    // make sure no more rendering can happen
    g_Internals->m_window->SetCallback( OnRender, NULL );

    // shut down the OGL context
    wglMakeCurrent(NULL, NULL);
    if(g_Internals->m_hrc)
    {
        wglDeleteContext(g_Internals->m_hrc);
        g_Internals->m_hrc = NULL;
    }

    // shut down window
    g_Internals->m_window->Destroy();
}
*/
//-----------------------------------------------------------------------------
int CPUTReturnCode()
{
    return g_Internals->m_window->ReturnCode();
}

/*
// CALLBACKS
//-----------------------------------------------------------------------------
void CPUTSetKeyboardCallBack(LPCPUTCALLBACK_ONKEY func)
{
    g_Internals->m_window->SetCallback( OnKey, func );
};

void CPUTSetMouseCallBack()
{

};

void CPUTSetRenderCallback(LPCPUTCALLBACK_RENDER func)
{
    // register CPUTRender with window
    g_Internals->m_window->SetCallback( OnRender, CPUTRender );

    // set user callback
    g_Internals->UserRender = func;
};
void CPUTSetResizeCallback() {};
void CPUTSetCreateCallback() {};


*/
//}; // end namespace CPUT
#endif
