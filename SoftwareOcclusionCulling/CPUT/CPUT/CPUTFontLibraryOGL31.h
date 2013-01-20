#ifndef __CPUTFONTLIBRARYOGL31_H__ 
#define __CPUTFONTLIBRARYOGL31_H__

#include <stdio.h>
#include "CPUTBase.h"
#include "CPUTFontLibraryBase.h"
#include "CPUTTextureLoaderOGL31.h"
#include <assert.h>

namespace CPUTGL31
{
    class CPUTFontLibrary:public CPUTFontLibraryBase
    {
    public:
        static CPUTFontLibrary* GetFontLibrary();
        ~CPUTFontLibrary();
        CPUTResult LoadFont(const cString Filename, CPUTFont& FontID);    

    private:    
        CPUTFontLibrary();
        static CPUTFontLibrary* m_pFontLibrary;
    };

}

#endif // #ifndef __CPUTFONTLIBRARYOGL31_H__
