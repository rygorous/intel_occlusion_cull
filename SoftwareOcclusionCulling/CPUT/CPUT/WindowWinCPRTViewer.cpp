#include "WindowWinCPRTViewer.h"



CPUTBase* WindowWinCPRTViewer::m_CPUT=NULL;
bool WindowWinCPRTViewer::m_ShutdownTriggered=false;
static int childWindowCount = 0;

// constructor
//-----------------------------------------------------------------------------
WindowWinCPRTViewer::WindowWinCPRTViewer():		m_hInst(0),
												m_hWnd(0),
												parentHWnd(0),
												m_AppClosedReturnCode(0),
												m_AppTitle(NULL)    
{
    childWindowCount++;
}

// destructor
//-----------------------------------------------------------------------------
WindowWinCPRTViewer::~WindowWinCPRTViewer()
{
    if(m_AppTitle)
    {
        delete [] m_AppTitle;
        m_AppTitle = NULL;
    }
}


// convert OS specific key events to CPUT events
//-----------------------------------------------------------------------------
//eKey WindowWinCPRTViewer::ConvertKeyCode(WPARAM wParam, LPARAM lParam)
//{
//    switch(wParam)
//    {
//        case 'Q': 
//        case 'q':
//            return KEY_Q;       
//
//    }
//    return KEY_NONE;
//}


// create window (ignore accelerators at the moment)
//-----------------------------------------------------------------------------
CPUTResult WindowWinCPRTViewer::Create(CPUTBase* cput, const char* windowTitle, int windowWidth, int windowHeight)
{
    // get the hInstance of this executable
    m_hInst = GetModuleHandle(NULL);
    int       nCmdShow = 1;
    int       nReturn = 0;

	//This is admittedly a hack in order to avoid changing the basic interface for the class.  In order to get parent
	//handle, using FindWindow and hardcoding title/classname.
	parentHWnd = FindWindow("CPRTViewerWindowClass","CPRT Viewer");
	if(!parentHWnd)
		MessageBox("Couldn't find parent window", "When creating the current window, could not find the parent window 'CPRT Viewer'.", CPUT_MB_ERROR);

    if(NULL != windowTitle)
    {
        size_t length = strlen(windowTitle);
        size_t newlength = length+1;

        m_AppTitle = new char[newlength];
        strncpy_s( m_AppTitle, newlength, windowTitle, length);
		//m_AppTitle[length] = childWindowCount;
    }
    else
    {
        char title[] = "Sample";
        size_t length = strlen(title);
        size_t newlength = length+1;
        m_AppTitle = new char[newlength];
        sprintf_s( m_AppTitle, newlength, title, length);
    }

    WNDCLASS wc;
    LPCTSTR title = m_AppTitle;
	ATOM classID = GetClassInfo(m_hInst, title, &wc); 
    
    if(0==classID)
    {
        classID = MyRegisterClass(m_hInst);        
    }

	// Perform application initialization:
	if (!InitInstance (m_hInst, nCmdShow, windowWidth, windowHeight))
	{
		return false;
	}

// resource loading - do we want to support this?
//	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TEST));

    m_CPUT = cput;
    return CPUT_SUCCESS;
}



// destroy window
//-----------------------------------------------------------------------------
int WindowWinCPRTViewer::Destroy()
{    
    m_ShutdownTriggered = true;    
    PostQuitMessage( 0 );
    return true;
    //return true;
    //return PostMessage(m_hWnd, WM_QUIT, 0, 0);
}


// get window return code on close
//-----------------------------------------------------------------------------
int WindowWinCPRTViewer::ReturnCode()
{
    return m_AppClosedReturnCode;
}


// Pop up an OS messagebox
//-----------------------------------------------------------------------------
int WindowWinCPRTViewer::MessageBox(const char* titleText, const char* message, CPUT_MESSAGE_BOX_TYPE eType)
{
    return ::MessageBox(m_hWnd, message, titleText, eType); 
}

// Get the OS window dimensions
//-----------------------------------------------------------------------------
void WindowWinCPRTViewer::GetWindowDimensions(int &x, int &y, int &width, int &height)
{
    RECT windowRect;
    GetWindowRect(m_hWnd, &windowRect);

    x = windowRect.left;
    y = windowRect.top;

    width = windowRect.right - windowRect.left;
    height = windowRect.bottom - windowRect.top;
}

// Get the client area dimensions
//-----------------------------------------------------------------------------
void WindowWinCPRTViewer::GetClientDimensions(int &width, int &height)
{
    RECT windowRect;
    GetClientRect(m_hWnd, &windowRect);
    height = windowRect.bottom - windowRect.top;
    width = windowRect.right - windowRect.left;
}

//
//  FUNCTION: MyRegisterClass()
//  PURPOSE: Registers the window class.
//  COMMENTS:
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//-----------------------------------------------------------------------------
ATOM WindowWinCPRTViewer::MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

    LPCTSTR title = m_AppTitle; //L("Title");

    wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= NULL;
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= NULL; 
	wcex.lpszClassName	= "CPRTChildWindowClass";
	wcex.hIconSm		= NULL; 

	return RegisterClassEx(&wcex);
}


//
//   FUNCTION: InitInstance(HINSTANCE, int)
//   PURPOSE: Saves instance handle and creates main window
//   COMMENTS:
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//-----------------------------------------------------------------------------
BOOL WindowWinCPRTViewer::InitInstance(HINSTANCE hInstance, int nCmdShow, int windowWidth, int windowHeight)
{
   m_hInst = hInstance; // Store instance handle in our global variable

   LPCSTR lpTitle = m_AppTitle;

   if( (0==windowWidth) || (0==windowHeight) )
   {
       // zero sized windows means - you choose the size. :)
       CPUTOSServices* pServices = CPUTOSServices::GetOSServices();
       pServices->GetDesktopDimensions(windowWidth, windowHeight);
       
       // default window size will be 1/3 of the screen size
       windowWidth/=3;
       windowHeight/=3;
   }

   m_hWnd = CreateWindow("CPRTChildWindowClass", lpTitle, //APPTITLE, APPTITLE, //szWindowClass, szTitle, 
       WS_CHILDWINDOW | WS_VISIBLE,
       CW_USEDEFAULT, 
       CW_USEDEFAULT, 
       100, 
       100,
       parentHWnd, 
       NULL, 
       (HINSTANCE)GetWindowLongPtr(parentHWnd, GWLP_HINSTANCE), 
       NULL);

   if (!m_hWnd)
   {
      return FALSE;
   }

   ShowWindow(m_hWnd, nCmdShow);
   UpdateWindow(m_hWnd);

   CPUTOSServices* pServices = CPUTOSServices::GetOSServices();
   pServices->SethWnd(m_hWnd);

   return TRUE;
}


//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//  PURPOSE:  Processes messages for the main window.
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//-----------------------------------------------------------------------------
LRESULT CALLBACK WindowWinCPRTViewer::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

    int width, height;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

        return DefWindowProc(hWnd, message, wParam, lParam);
		break;

    case WM_CHAR: // WM_KEYDOWN: gives you EVERY key - including shifts/etc       
        
        if(m_CPUT)
        {
            /*eKey key = ConvertKeyCode(wParam, lParam);
            if(KEY_Q == key)
                m_ShutdownTriggered = true;                
            else
                m_CPUT->OnKeyboardEvent( key );*/
        }
        break;

    case WM_LBUTTONDBLCLK:
    case WM_MBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
        // todo: handle double-click
        break;

    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MOUSEMOVE:
        if(m_CPUT)
        {
            /*eMouseState state = ConvertMouseState(wParam);*/

            int xPos = LOWORD(lParam); 
            int yPos = HIWORD(lParam); 

            /*m_CPUT->OnMouseEvent(xPos, yPos, state);*/
        }
        break;

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
        // TODO: Add any drawing code here...
        EndPaint(hWnd, &ps);
        break;

    case WM_SIZING:
    case WM_MOVING:
    case WM_ERASEBKGND:
        // avoids flicker and awful expense of re-creating tons of contexts
        break;

    case WM_SIZE:        
        height =HIWORD(lParam);
        width = LOWORD(lParam);

        // update the system's size and make callback
        if(m_CPUT)
            m_CPUT->OnResizeEvent(width,height);
        
        break;

        /*
        // method used by freeglut
    case WM_EXITSIZEMOVE: 
        
        RECT winRect;
        GetWindowRect(hWnd, &winRect );
        x = winRect.left;
        y = winRect.top;
        height = winRect.bottom - winRect.top;
        width = winRect.right - winRect.left;

        
        SetWindowPos(hWnd, 
            HWND_TOP, 
            x,y, width, height,
            SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSENDCHANGING |
            SWP_NOZORDER
            );     

        m_CPUT->OnSize(width,height);
        break;
        */

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Convert mouse state to CPUT state
//-----------------------------------------------------------------------------
//eMouseState WindowWinCPRTViewer::ConvertMouseState(WPARAM wParam)
//{
//    eMouseState eState=CPUT_MOUSE_NONE;
//
//    if( wParam & MK_CONTROL)
//        eState = (eMouseState) (eState | (int)CPUT_MOUSE_CTRL_DOWN) ;
//    
//    if( wParam & MK_SHIFT)
//        eState = (eMouseState) (eState | (int)CPUT_MOUSE_SHIFT_DOWN) ;
//
//    if( wParam & MK_LBUTTON)
//        eState = (eMouseState) (eState | (int)CPUT_MOUSE_LEFT_DOWN) ;
//
//    if( wParam & MK_MBUTTON)
//        eState = (eMouseState) (eState | (int)CPUT_MOUSE_MIDDLE_DOWN) ;
//
//    if( wParam & MK_RBUTTON)
//        eState = (eMouseState) (eState | (int)CPUT_MOUSE_RIGHT_DOWN) ;
//
//    
//    return eState;
//}

// main message loop
//-----------------------------------------------------------------------------
int WindowWinCPRTViewer::StartMessageLoop()
{
    MSG msg;
    msg.message = WM_NULL;
	// Main message loop:
    while( !m_ShutdownTriggered ) 
    {       
        // Use PeekMessage() so we can use idle time to render the scene. 
        if( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) )
        {
            if(msg.message == WM_QUIT)
                break;
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
        else
        {       
            m_CPUT->CallRender();
        }
    }

    m_AppClosedReturnCode =  (int) msg.wParam;
	return (int) msg.wParam;
}