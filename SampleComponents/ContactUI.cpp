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

// File: ContactUI.cpp
//--------------------------------------------------------------------------------------

#include "ContactUI.h"


// Handles messages from the dialog box
BOOL CALLBACK ShowMessage( HWND hwndDlg,
                           UINT message,
                           WPARAM wParam,
                           LPARAM lParam )
{
    switch( message )
    {
    case WM_COMMAND:
        {
            switch( LOWORD( wParam ) )
            {
            case IDOK:
                EndDialog( hwndDlg, wParam );
                return true;
            }
            break;
        }
        break;
					
    case WM_NOTIFY:
        {
            LPNMHDR pnmh = ( LPNMHDR )lParam;

            // If the notification came from the syslink control, execute the link.
            if( ( pnmh->idFrom == IDC_SYSLINKEMAIL ) || ( pnmh->idFrom == IDC_SYSLINKHOME ) )
            {
                if( ( pnmh->code == NM_CLICK ) || ( pnmh->code == NM_RETURN ) )
                {
                    PNMLINK link = ( PNMLINK )lParam;
                    ShellExecute( NULL, L"open", link->item.szUrl, NULL, NULL, SW_SHOWNORMAL );
                }
            }				
        }
        break;

    case WM_CLOSE:
        {
            EndDialog( hwndDlg, wParam );
            return true;
        }
        break;
    }
    return false;
}


// Retrieve string for name, author and version information
BOOL GetAppVersionString( TCHAR* lpBuffer,
                          UINT uBufferLength )
{
    HRSRC hResource = FindResource( NULL, MAKEINTRESOURCE( VS_VERSION_INFO ), RT_VERSION );
    HGLOBAL hResData = hResource ? LoadResource( NULL, hResource ) : NULL;
    LPVOID pData = hResData ? LockResource( hResData ) : NULL;
    if( !pData )
        return FALSE;
    UINT length;
    DWORD* pLangCodepage;
    if( !VerQueryValue( pData, _T( "\\VarFileInfo\\Translation" ), ( LPVOID* )&pLangCodepage, &length ) )
        return FALSE;
	
    TCHAR* strAuthorName;
    TCHAR* strProductName;
    TCHAR* strVersion;

    TCHAR query[MAX_PATH];
    _stprintf_s( query, _T( "\\StringFileInfo\\%04x%04x\\ProductName" ), LOWORD( *pLangCodepage ),
                 HIWORD( *pLangCodepage ) );
    if( !VerQueryValue( pData, query, ( LPVOID* )&strProductName, &length ) )
        return FALSE;
    _stprintf_s( query, _T( "\\StringFileInfo\\%04x%04x\\ProductVersion" ), LOWORD( *pLangCodepage ),
                 HIWORD( *pLangCodepage ) );
    if( !VerQueryValue( pData, query, ( LPVOID* )&strVersion, &length ) )
        return FALSE;
    _stprintf_s( query, _T( "\\StringFileInfo\\%04x%04x\\Comments" ), LOWORD( *pLangCodepage ),
                 HIWORD( *pLangCodepage ) );
    if( !VerQueryValue( pData, query, ( LPVOID* )&strAuthorName, &length ) )
        return FALSE;
    assert(uBufferLength>0);
    _stprintf_s( lpBuffer, uBufferLength, _T( "Name: %s\nAuthor: %s\nVersion: %s\n" ), strProductName, strAuthorName,
                 strVersion );
    return TRUE;
}


// Display sample/ contact information.
void RenderContactBox()
{	
    // Window handle of dialog box 
    HWND hwndGoto = NULL;

    // Obtain the sample version and other information
    WCHAR versionString[MAX_PATH];
	BOOL haveVersionString = GetAppVersionString( versionString, MAX_PATH );
    assert( haveVersionString );

    //Create Dialog Box
    hwndGoto = CreateDialog( NULL, MAKEINTRESOURCE( IDD_DIALOG1 ), NULL, ( DLGPROC )ShowMessage );
    SetDlgItemText( hwndGoto, IDC_TEXTSTRING, versionString ); // update the text 
    ShowWindow( hwndGoto, SW_SHOW ); // Show the dialog box

    return;
}


