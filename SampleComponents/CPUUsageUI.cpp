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

#include "CPUUsageUI.h"

/// < summary >
///	    CPUUSageUI class constructor
/// < /summary >
/// <param name="param1">
///		unsigned int - Number of Logical CPUs
/// </param>
CPUUsageUI::CPUUsageUI( unsigned int uNumCounters )
{
    //Update the CPU count 
    m_uNumCounters = uNumCounters;
    m_uNumCPUs = m_uNumCounters - 1;

    // If we are on a single CPU system without HT
    if( m_uNumCPUs <= 0 )
    {
        m_uNumCPUs = 1;
    }

    //Have we have already allocated the array to hold CPU stats?
    if( m_pdCPUPercent )
        delete [] m_pdCPUPercent;

    //Allocate array to hold the CPU stats
    m_pdCPUPercent = new double[m_uNumCounters];

    //Initialize the CPU stats array with 0
    for( unsigned int i = 0; i < m_uNumCounters; i++ )
    {
        m_pdCPUPercent[i] = 0.0f;
    }
}


/// < summary >
///	    CPUUSageUI class destructor
/// < /summary >
CPUUsageUI::~CPUUsageUI()
{
    //Delete the allocated CPU stats array
    if( m_pdCPUPercent )
        delete [] m_pdCPUPercent;
}


/// < summary >
///	    Update and render the CPU usage stats on the screen
/// < /summary >
/// <param name="param1">
///		CDXUTTextHelper* - Pointer to DXUTTextHelper to draw on screen
/// </param>
/// <param name="param2">
///		CPUUSage& - Reference to CPUUsage object to obtain the updated CPU stats array
/// </param>
void CPUUsageUI::RenderCPUUsage( CDXUTTextHelper* pTxtHelper,
                                 CPUUsage& aCPUUsage,
                                 int iX,
                                 int iY )
{
    WCHAR buffer[100];

    //Update the CPU stats and store in local array 
    aCPUUsage.UpdatePeriodicData();
    aCPUUsage.getCPUCounters( m_pdCPUPercent );

    //Print the total number of logical CPUs
    pTxtHelper->SetInsertionPos( iX, iY );
    swprintf_s( buffer, 100, L"Logical CPUs: %d", m_uNumCPUs );
    pTxtHelper->DrawFormattedTextLine( buffer );

    //Draw the per CPU usage on the screen
    for( unsigned int i = 0; i < m_uNumCPUs; i++ )
    {
        pTxtHelper->SetInsertionPos( iX, iY += 20 );
        swprintf_s( buffer, 100, L"CPU%d: %d", i, ( int )m_pdCPUPercent[i] );
        pTxtHelper->DrawFormattedTextLine( buffer );
    }	
}
