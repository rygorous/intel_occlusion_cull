#ifndef __WINDOWWINCPRTVIEWER_H__
#define __WINDOWWINCPRTVIEWER_H__

#include <windows.h>
#include <winuser.h> // for character codes
#include <cstringt.h> // for CString class
#include <atlstr.h> // CString class

#include "CPUTBase.h"
#include "CPUTServicesWin.h"
// all the code needed to create a Windows window

#define APPTITLE "CPUT Sample"

// forward declarations
//-----------------------------------------------------------------------------
class CPUTBase;

enum CPUT_MESSAGE_BOX_TYPE 
{
    CPUT_MB_OK = MB_OK | MB_ICONINFORMATION,
    CPUT_MB_ERROR = MB_OK | MB_ICONEXCLAMATION,
    CPUT_MB_WARNING = MB_OK | MB_ICONWARNING
};


// OS-specific window class
//-----------------------------------------------------------------------------
class WindowWinCPRTViewer
{
public:
    WindowWinCPRTViewer();
    ~WindowWinCPRTViewer();

    // CPUT callins
    CPUTResult Create(CPUTBase* cput, const char* windowTitle,int windowWidth, int windowHeight);
    int Destroy();
    int ReturnCode();    
    int MessageBox(const char* titleText, const char* message, CPUT_MESSAGE_BOX_TYPE eType);
    void GetWindowDimensions(int &x, int &y, int &width, int &height);
    void GetClientDimensions(int &width, int &height);
    int StartMessageLoop();

 //   void SetCallback(eWindowCallbacks callback, void* func);
    HWND GetHWnd() { return m_hWnd;};

private:
    HINSTANCE   m_hInst;					// current instance
    HWND        m_hWnd;
	HWND		parentHWnd;
    int         m_AppClosedReturnCode;
    char*       m_AppTitle;                 // title put at top of window
    static CPUTBase*   m_CPUT;

    bool        m_ResizeTriggered;
    static bool        m_ShutdownTriggered;

    // cput helper functions
    //static eKey ConvertKeyCode(WPARAM wParam, LPARAM lParam);

    // creation functions
    ATOM MyRegisterClass(HINSTANCE hInstance);
    BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, int windowWidth, int windowHeight);
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    //static eMouseState ConvertMouseState(WPARAM wParam);
};


#endif //#ifndef __WINDOWWINCPRTVIEWER_H__
