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

#include "HelpUI.h"

// Displays default help and key bindings
void RenderHelp( CDXUTTextHelper* pTxtHelper )
{	
    pTxtHelper->DrawTextLine( L"Controls:" );
	
    //Render shortcut keys.
    pTxtHelper->DrawTextLine( L"Fullscreen: F2" );
    pTxtHelper->DrawTextLine( L"Change device: F3" );
    pTxtHelper->DrawTextLine( L"Toggle warp: F4" );
    pTxtHelper->DrawTextLine( L"Toggle GUI: F10" );
    pTxtHelper->DrawTextLine( L"Toggle Demo Text: F11" );
    pTxtHelper->DrawTextLine( L"Rotate Camera: Left Mouse Button" );
    pTxtHelper->DrawTextLine( L"Rotate Light: Right Mouse Button" );
    pTxtHelper->DrawTextLine( L"Rotate Model: Middle Mouse Button" );
    pTxtHelper->DrawTextLine( L"Zoom Camera: Mouse Wheel Scroll" );
    pTxtHelper->DrawTextLine( L"Hide Help: F1" );								
    pTxtHelper->DrawTextLine( L"Quit: ESC" );	

    return;
}

