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
#include "CPUTPane.h"

// Constructor
//------------------------------------------------------------------------------
CPUTPane::CPUTPane()
{
    // initialize the state variables
    InitialStateSet();
}

// Constructor
//-----------------------------------------------------------------------------
CPUTPane::CPUTPane(const cString ControlText, CPUTControlID id)
{
    // initialize the state variables
    InitialStateSet();

    // save the control ID for callbacks
    m_controlID = id;
}

// Initial state of the control's member variables
//-----------------------------------------------------------------------------
void CPUTPane::InitialStateSet()
{
    m_controlType = CPUT_PANE;

    // dimensions
    m_PaneDimensions.x=0;
    m_PaneDimensions.y=0;
    m_PaneDimensions.width=0;
    m_PaneDimensions.height=0;
}

// Destructor
//------------------------------------------------------------------------------
CPUTPane::~CPUTPane()
{

}


// Return the upper-left screen coordinate location of this control
//--------------------------------------------------------------------------------
void CPUTPane::GetPosition(int& x, int& y)
{
    x = m_PaneDimensions.x;
    y = m_PaneDimensions.y;
}

// Return the width/height of the control
//--------------------------------------------------------------------------------
void CPUTPane::GetDimensions(int& width, int& height)
{
    width = m_PaneDimensions.width;
    height = m_PaneDimensions.height;
}

// Set's the panes enabled/disabled greyed/active state
//--------------------------------------------------------------------------------
void CPUTPane::SetEnable(bool bEnabled)
{
    if(false == bEnabled)
        m_ControlState = CPUT_CONTROL_INACTIVE;
    else
        m_ControlState = CPUT_CONTROL_ACTIVE;
}

// is this pane enabled?
//--------------------------------------------------------------------------------
bool CPUTPane::IsEnabled()
{
    if( CPUT_CONTROL_ACTIVE == m_ControlState )
        return true;

    return false;
}
