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

#ifndef __CPUUSAGEUI_H
#define __CPUUSAGEUI_H

#include "DXUT.h"
#include "SDKMisc.h"

#include "CPUUsage.h"

class CPUUsageUI
{
public:
    //Initialize the constructor with the CPU count
    CPUUsageUI( unsigned int );
    ~CPUUsageUI();

    void RenderCPUUsage( CDXUTTextHelper*,
                         CPUUsage&,
                         int = 10,
                         int = 120 );

private:
    //Declare the default constructor as private
    CPUUsageUI();

    unsigned int m_uNumCounters;
    unsigned int m_uNumCPUs;
    double* m_pdCPUPercent;
};

#endif
