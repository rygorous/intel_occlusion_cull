#include <stdio.h>

#include <X11/X.h> 		// add -L/usr/X11R6/lib -lX11 to linker path+libs
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/gl.h> 		// add -lGL -lGLU to libs
#include <GL/glx.h>
#include <GL/glu.h>

// enums - todo - put all enums in a 'definitions' file universal to all platforms
enum eKey
{
    KEY_NONE,
    KEY_A,
    KEY_B,
    KEY_C,
    KEY_Q,

};

enum eWindowCallbacks
{
    OnKey,
    OnMouse,
    OnRender,
};

class WindowX
{
public:
	WindowX();
    int Create();
    int Destroy();
    int ReturnCode();

    int StartMessageLoop();

    Display* GetDisplay() { return dpy; }
    GLXFBConfig* GetGLXFBConfig() { return fbConfig; }
    GLXContext GetContext() { return glc; }
    Window GetWindow() { return win; }
    void SetContext(GLXContext newContext) { glc=newContext; }

private:
    Display                 *dpy;
    Window                  root;
    GLXFBConfig          	*fbConfig;
    XVisualInfo             *vi;
    Colormap                cmap;
    XSetWindowAttributes    swa;
    Window                  win;
    GLXContext              glc;
    XWindowAttributes       gwa;
    XEvent                  xev;

};

WindowX::WindowX()
{
}

int WindowX::Create()
{
	dpy = XOpenDisplay(NULL);

	if(dpy == NULL)
	{
		printf("\n\tcannot connect to X server\n\n");
		exit(0);
	}




	// get the 'desktop'
	root = DefaultRootWindow(dpy);

	/*
	// Request a suitable framebuffer configuration
	static int attributeList[] = { GLX_RENDER_TYPE, GLX_RGBA_BIT, GLX_DOUBLEBUFFER, GLX_RED_SIZE, 1, GLX_GREEN_SIZE, 1, GLX_BLUE_SIZE, 1, None };
	static int doubleBufferAttributes[] = {
			//			GLX_X_RENDERABLE    , True,
			GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
			GLX_RENDER_TYPE     , GLX_RGBA_BIT,
			//GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
			GLX_RED_SIZE        , 1,
			GLX_GREEN_SIZE      , 1,
			GLX_BLUE_SIZE       , 1,
			//GLX_ALPHA_SIZE      , 8,
			//GLX_DEPTH_SIZE      , 24,
			//GLX_STENCIL_SIZE    , 8,
			GLX_DOUBLEBUFFER    , GL_TRUE,

			None
	};

	int glx_major, glx_minor;

	if ( !glXQueryVersion( dpy, &glx_major, &glx_minor ) || ( ( glx_major == 1 ) && ( glx_minor < 3 ) ) || ( glx_major < 1 ) )
	{
		printf( "Invalid GLX version" );
		exit(1);
	}

*/
	/*
	// using desired settings, choose color depth, depth buffer and/or double buffer, stencil buffer etc
	GLint att[5] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
	vi = glXChooseVisual(dpy, 0, att);

	if(vi == NULL)
	{
		printf("\n\tno appropriate visual found\n\n");
		exit(0);
	}
	else
	{
		printf("\n\tvisual %p selected\n", (void *)vi->visualid);
	} // %p creates hexadecimal output like in glxinfo




	int fbCount=0;
	int screen_num = DefaultScreen(dpy);
	 fbConfig = glXChooseFBConfig(dpy, screen_num, doubleBufferAttributes, &fbCount);

    //fbConfig = glXChooseFBConfig( dpy, vi->screen, attributeList, &fbCount );

    if ( fbConfig == NULL )
    {
    	// no double buffered configs available - try single buffering
    	printf("Double-buffering NOT present on this OpenGL device - resorting to single-buffering\n.");
    	int singleBufferAttributess[] = {
    	    GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
    	    GLX_RENDER_TYPE,   GLX_RGBA_BIT,
    	    GLX_RED_SIZE,      1,   // Request a single buffered color buffer
    	    GLX_GREEN_SIZE,    1,   // with the maximum number of color bits
    	    GLX_BLUE_SIZE,     1,   // for each component
    	    None
    	};


    	fbConfig = glXChooseFBConfig( dpy, DefaultScreen(dpy), singleBufferAttributess, &fbCount );
    }


    // Create an X colormap and window with a visual matching the first
    // returned framebuffer config
    vi = glXGetVisualFromFBConfig( dpy, fbConfig[0] );

    swa.border_pixel = 0;
    swa.event_mask = StructureNotifyMask;
    swa.colormap = XCreateColormap( dpy, RootWindow(dpy, vi->screen), vi->visual, AllocNone );


*/
	// this is the version from the OpenGL site, but doesn't use a GLXFBConfig which is needed for 3.1 support
	// using desired settings, choose color depth, depth buffer and/or double buffer, stencil buffer etc
	GLint att[5] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
	vi = glXChooseVisual(dpy, 0, att);

	if(vi == NULL) {
		printf("\n\tno appropriate visual found\n\n");
		exit(0); }
	else {
		printf("\n\tvisual %p selected\n", (void *)vi->visualid); } // %p creates hexadecimal output like in glxinfo


	// create a color map for the window
	cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);

    swa.border_pixel = 0;
	swa.colormap = cmap;
	swa.event_mask = ExposureMask | KeyPressMask;


	// create the window
	win = XCreateWindow(dpy, root, 0, 0, 600, 600, 0, vi->depth, InputOutput, vi->visual, CWColormap | CWEventMask, &swa);

	// map/name app
	XMapWindow(dpy, win);
	XStoreName(dpy, win, "VERY SIMPLE APPLICATION");


	glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
	glXMakeCurrent(dpy, win, glc);

	//glEnable(GL_DEPTH_TEST);

	return 0;
}

int WindowX::StartMessageLoop()
{
	while(1) {
		XNextEvent(dpy, &xev);

		if(xev.type == Expose)
		{
			XGetWindowAttributes(dpy, win, &gwa);
			glViewport(0, 0, gwa.width, gwa.height);

			glClearColor(1.0, 0.0, 0.0, 1.0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glXSwapBuffers(dpy, win);
		}

		else if(xev.type == KeyPress)
		{
			glXMakeCurrent(dpy, None, NULL);
			glXDestroyContext(dpy, glc);
			XDestroyWindow(dpy, win);
			XCloseDisplay(dpy);
			exit(0);
		}
	} /* this closes while(1) { */
	return 1;
}

int WindowX::ReturnCode()
{
	return 0;
}
