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
#include "CPUTConfigBlock.h"
#include "CPUTOSServicesWin.h"

CPUTConfigEntry  &CPUTConfigEntry::sNullConfigValue = CPUTConfigEntry(_L(""), _L(""));

//----------------------------------------------------------------
void RemoveWhitespace(cString &szString)
{
    // Remove leading whitespace
    size_t nFirstIndex = szString.find_first_not_of(_L(' '));
    if(nFirstIndex != cString::npos)
    {
        szString = szString.substr(nFirstIndex);
    }

    // Remove trailing newlines
    size_t nLastIndex = szString.find_last_not_of(_L('\n'));
    while(nLastIndex != szString.length()-1)
    {
        szString.erase(nLastIndex+1,1);
        nLastIndex = szString.find_last_not_of(_L('\n'));
    };
    // Tabs
    nLastIndex = szString.find_last_not_of(_L('\t'));
    while(nLastIndex != szString.length()-1)
    {
        szString.erase(nLastIndex+1,1);
        nLastIndex = szString.find_last_not_of(_L('\t'));
    };
    // Spaces
    nLastIndex = szString.find_last_not_of(_L(' '));
    while(nLastIndex != szString.length()-1)
    {
        szString.erase(nLastIndex+1,1);
        nLastIndex = szString.find_last_not_of(_L(' '));
    };
}

//----------------------------------------------------------------
CPUTResult ReadLine(cString &szString, FILE *pFile)
{
    // TODO: 128 chars is a narrow line.  Why the limit?
    // Is this not really reading a line, but instead just reading the next 128 chars to parse?
    TCHAR   szCurrLine[128] = {0};
    TCHAR *ret = fgetws(szCurrLine, 128, pFile);
    if(ret != szCurrLine)
    {
        if(!feof(pFile))
        {
            return CPUT_ERROR_FILE_ERROR;
        }
    }

    szString = szCurrLine;
    RemoveWhitespace(szString);

    // TODO: why are we checking feof twice in this loop?
    // And, why are we using an error code to signify done?
    // eof check should be performed outside ReadLine()
    if(feof(pFile))
    {
        return CPUT_ERROR_FILE_ERROR;
    }

    return CPUT_SUCCESS;
}

//----------------------------------------------------------------
void CPUTConfigEntry::ValueAsFloatArray(float *pFloats, int count)
{
    cString valueCopy = szValue;
    TCHAR *szOrigValue = (TCHAR*)valueCopy.c_str();

    TCHAR *szNewValue = NULL;
    TCHAR *szCurrValue = wcstok_s(szOrigValue, _L(" "), &szNewValue);
	for(int clear = 0; clear < count; clear++)
	{
		pFloats[clear] = 0.0f;
	}
    for(int ii=0;ii<count;++ii)
    {
		if(szCurrValue == NULL)
            return;
        swscanf_s(szCurrValue, _L("%f"), pFloats+ii);
        szCurrValue = wcstok_s(NULL, _L(" "), &szNewValue);

    }
}
//----------------------------------------------------------------
CPUTConfigBlock::CPUTConfigBlock()
    : mnValueCount(0)
{
}
//----------------------------------------------------------------
CPUTConfigBlock::~CPUTConfigBlock()
{
}
//----------------------------------------------------------------
const cString &CPUTConfigBlock::GetName(void)
{
    return mszName;
}
//----------------------------------------------------------------
int CPUTConfigBlock::GetNameValue(void)
{
    return mName.ValueAsInt();
}
//----------------------------------------------------------------
CPUTConfigEntry *CPUTConfigBlock::GetValue(int nValueIndex)
{
    if(nValueIndex < 0 || nValueIndex >= mnValueCount)
    {
        return NULL;
    }
    return &mpValues[nValueIndex];
}
//----------------------------------------------------------------
CPUTConfigEntry *CPUTConfigBlock::AddValue(const cString &szName, const cString &szValue )
{
    cString szNameLower = szName;
    std::transform(szNameLower.begin(), szNameLower.end(), szNameLower.begin(), ::tolower);

    cString szValueLower = szValue;
    std::transform(szValueLower.begin(), szValueLower.end(), szValueLower.begin(), ::tolower);

    // TODO: What should we do if it already exists?
    CPUTConfigEntry *pEntry = &mpValues[mnValueCount++];
    pEntry->szName  = szNameLower;
    pEntry->szValue = szValueLower;
    return pEntry;
}
//----------------------------------------------------------------
CPUTConfigEntry *CPUTConfigBlock::GetValueByName(const cString &szName)
{
    cString szString = szName;
    std::transform(szString.begin(), szString.end(), szString.begin(), ::tolower);

    for(int ii=0; ii<mnValueCount; ++ii)
    {
        if(mpValues[ii].szName.compare(szString) == 0)
        {
            return &mpValues[ii];
        }
    }

    // not found - return an 'empty' object to avoid crashes/extra error checking
    return &CPUTConfigEntry::sNullConfigValue;
}
//----------------------------------------------------------------
int CPUTConfigBlock::ValueCount(void)
{
    return mnValueCount;
}
//----------------------------------------------------------------
CPUTConfigFile::CPUTConfigFile()
    : mnBlockCount(0)
    , mpBlocks(NULL)
{
}
//----------------------------------------------------------------
CPUTConfigFile::~CPUTConfigFile()
{
    if(mpBlocks)
    {
        delete [] mpBlocks;
        mpBlocks = 0;
    }
    mnBlockCount = 0;
}
//----------------------------------------------------------------
CPUTResult CPUTConfigFile::LoadFile(const cString &szFilename)
{
    // Load the file
    cString             szCurrLine;
    CPUTConfigBlock    *pCurrBlock = NULL;
    FILE               *pFile = NULL;
    int                 nCurrBlock = 0;
    CPUTResult result = CPUTOSServices::GetOSServices()->OpenFile(szFilename, &pFile);
    if(CPUTFAILED(result))
    {
        return result;
    }

    /* count the number of blocks */
    while(1)
    {
        /* Find the block */
        // Read lines until a '[' is found
        CPUTResult readResult = ReadLine(szCurrLine, pFile);
        if(readResult != CPUT_SUCCESS)
            break;

        size_t nOpenBracketIndex    = szCurrLine.find_first_of(_L('['));
        size_t nCloseBracketIndex   = szCurrLine.find_last_of(_L(']'));
        if(nOpenBracketIndex != cString::npos && nCloseBracketIndex != cString::npos)
        {   // This line is a valid block header
            mnBlockCount++;
        }
    };
    /* Mtl files don't have headers, so we have
    to do some magic to support them */
    if(mnBlockCount == 0)
    {
        mnBlockCount   = 1;
    }

    fseek(pFile, 0, SEEK_SET);
    mpBlocks = new CPUTConfigBlock[mnBlockCount];
    pCurrBlock = mpBlocks;

    /* Find the first block first */
    while(1)
    {
        /* Find the block */
        // Read lines until a '[' is found
        CPUTResult readResult = ReadLine(szCurrLine, pFile);
        if(readResult != CPUT_SUCCESS && szCurrLine == _L(""))
        {
            fclose(pFile);
            return CPUT_SUCCESS;
        }

        size_t nOpenBracketIndex    = szCurrLine.find_first_of(_L('['));
        size_t nCloseBracketIndex   = szCurrLine.find_last_of(_L(']'));
        if(nOpenBracketIndex != cString::npos && nCloseBracketIndex != cString::npos)
        {   // This line is a valid block header
            pCurrBlock = mpBlocks + nCurrBlock++;
            szCurrLine.erase(nCloseBracketIndex,1);
            pCurrBlock->mszName = szCurrLine.c_str()+1;
            /*
            size_t nSpaceIndex = szCurrLine.find_first_of(_L(' '));
            cString szValue = szCurrLine.substr(nSpaceIndex+1); 
            cString szName = szCurrLine.erase(nSpaceIndex, 1024); 
            RemoveWhitespace(szValue);
            RemoveWhitespace(szName);
            pCurrBlock->mName.szName = szName;
            pCurrBlock->mName.szValue = szValue;
            */
            std::transform(pCurrBlock->mszName.begin(), pCurrBlock->mszName.end(), pCurrBlock->mszName.begin(), ::tolower);
        }
        else if(szCurrLine != _L(""))
        {   // It's a value
            if(pCurrBlock == NULL)
            {
                continue;
            }

            size_t  nEqualsIndex = szCurrLine.find_first_of(_L('='));
            if(nEqualsIndex == cString::npos)
            {
                bool dup = false;
                // No value, just a key, save it anyway
                for(int ii=0;ii<pCurrBlock->mnValueCount;++ii)
                {
                    if(!pCurrBlock->mpValues[ii].szName.compare(szCurrLine))
                    {
                        dup = true;
                        break;
                    }
                }
                if(!dup)
                {
                    pCurrBlock->mpValues[pCurrBlock->mnValueCount].szName = szCurrLine;
                    pCurrBlock->mnValueCount++;
                }
            }
            else
            {
                cString szValue = szCurrLine.substr(nEqualsIndex+1);
                cString szName = szCurrLine.erase(nEqualsIndex, 1024);
                RemoveWhitespace(szValue);
                RemoveWhitespace(szName);
                std::transform(szName.begin(), szName.end(), szName.begin(), ::tolower);

                bool dup = false;
                for(int ii=0;ii<pCurrBlock->mnValueCount;++ii)
                {
                    if(!pCurrBlock->mpValues[ii].szName.compare(szName))
                    {
                        dup = true;
                        break;
                    }
                }
                if(!dup)
                {
                    pCurrBlock->mpValues[pCurrBlock->mnValueCount].szValue = szValue;
                    pCurrBlock->mpValues[pCurrBlock->mnValueCount].szName = szName;
                    pCurrBlock->mnValueCount++;
                }
            }
        }
    };

    fclose(pFile);
    return CPUT_SUCCESS;
}

//----------------------------------------------------------------
CPUTConfigBlock *CPUTConfigFile::GetBlock(int nBlockIndex)
{
    if(nBlockIndex >= mnBlockCount || nBlockIndex < 0)
    {
        return NULL;
    }

    return &mpBlocks[nBlockIndex];
}

//----------------------------------------------------------------
CPUTConfigBlock *CPUTConfigFile::GetBlockByName(const cString &szBlockName)
{
    cString szString = szBlockName;
    std::transform(szString.begin(), szString.end(), szString.begin(), ::tolower);

    for(int ii=0; ii<mnBlockCount; ++ii)
    {
        if(mpBlocks[ii].mszName.compare(szString) == 0)
        {
            return &mpBlocks[ii];
        }
    }
    return NULL;
}

//----------------------------------------------------------------
int CPUTConfigFile::BlockCount(void)
{
    return mnBlockCount;
}
