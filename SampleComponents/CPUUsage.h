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

//---------------------------------------------------------------------------------------

#ifndef __CPUUSAGE_H
#define __CPUUSAGE_H

#include <vector>
#include <windows.h>

#include "pdh.h"
#include "pdhmsg.h"
#include "tchar.h"

class CPUUsage
{
public:
    CPUUsage( void );
    ~CPUUsage();
	
    void UpdatePeriodicData();

    unsigned int getCPUCount()
    {
        return m_CPUCount;
    }
    unsigned int getNumCounters()
    {
        return m_numCounters;
    }
    void getCPUCounters( double* CPUPercent )
    {
        if( CPUPercent != NULL )
        {
            for( unsigned int i = 0; i < m_numCounters; i++ )
            {
                CPUPercent[i] = m_CPUPercentCounters[i];
            }
        }
        return;
    }

private:
    unsigned int m_CPUCount;
    unsigned int m_numCounters;
    double* m_CPUPercentCounters;
    float m_secondsPerUpdate;

    LARGE_INTEGER m_LastUpdateTick;
    LARGE_INTEGER m_Frequency;

    std::vector <void*> m_vecProcessorCounters;

    // Index of the performance object called "Processor" in English.
    // From registry, in HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\PerfLib\<your language ID>.
    static const int m_processorObjectIndex = 238;
	
    // Take a guess at how long the name could be in all languages of the "Processor" counter object.
    static const int m_processorObjectNameMaxSize = 128;
};

#endif
