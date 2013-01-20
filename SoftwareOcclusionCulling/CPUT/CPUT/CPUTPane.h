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
#ifndef __CPUTPANE_H__
#define __CPUTPANE_H__



#include "CPUTControl.h"
#include <vector>

// default padding between controls
#define CPUT_PANE_TEXT_BORDER_PADDING_X 0
#define CPUT_PANE_TEXT_BORDER_PADDING_Y 0

// Pane base - common functionality for the control
//-----------------------------------------------------------------------------
class CPUTPane : public CPUTControl
{
public:
    // constructors
    CPUTPane();
    CPUTPane(CPUTPane& copy); // don't allow copy construction
    CPUTPane(const cString ControlText, CPUTControlID id);
    virtual ~CPUTPane();

    // CPUTControl
    virtual void GetPosition(int& x, int& y);
    virtual void GetDimensions(int& width, int& height);
    virtual void SetEnable(bool bEnabled);
    virtual bool IsEnabled();

protected:
    std::vector<CPUTControl*>   m_pControls;
    CPUT_RECT m_PaneDimensions;
    //CPUTGUIControlState m_PaneState;

    // helper functions
    void InitialStateSet();
};

#endif //#ifndef __CPUTPANE_H__