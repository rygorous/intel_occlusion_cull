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

// File: ContactUI.h
// Displays the About contact box, with sample name, version number and ISN contact
//--------------------------------------------------------------------------------------

#ifndef __CONTACTUI_H
#define __CONTACTUI_H

#include <tchar.h>

#include "DXUT.h"
#include "SDKmisc.h"

#include "resource.h"

/// < summary >
///     Handles messages sent to dialog box.	
/// < /summary >
/// <param name="param1">
///		HWND - A handle to the dialog box. 
/// </param>
/// <param name="param2">
///		UINT - The message or dialog box ID.
/// </param>
/// <param name="param3">
///		WPARAM - Additional message-specific information. 
/// </param>
/// <param name="param4">
///		LPARAM - Additional message-specific information
/// </param>
BOOL CALLBACK   ShowMessage( HWND,
                             UINT,
                             WPARAM,
                             LPARAM );


/// < summary >
///     Handles messages sent to dialog box.	
/// < /summary >
/// <param name="param1">
///		TCHAR* - Pointer to string to hold version info 
/// </param>
/// <param name="param2">
///		UINT - Max buffer length of string
/// </param>
BOOL GetAppVersionString( TCHAR*,
                          UINT );

/// < summary >
///		Handles on click action on 'Contact Us' button.
///		When the 'Conatct Us' button is clicked, a dialog box pops up which displays
///		the link for the feedback emails and link to VCSE website.
/// < /summary >
void RenderContactBox();

#endif //__CONTACTUI_H

