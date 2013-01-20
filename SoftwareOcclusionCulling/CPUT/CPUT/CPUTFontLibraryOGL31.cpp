#include "CPUTFontLibraryOGL31.h"
#include <malloc.h> // for heapcheck

namespace CPUTGL31
{

    CPUTFontLibrary* CPUTFontLibrary::m_pFontLibrary = NULL;

    // constructor
    //--------------------------------------------------------------------------------
    CPUTFontLibrary::CPUTFontLibrary()
    {

    }

    // destructor
    //--------------------------------------------------------------------------------
    CPUTFontLibrary::~CPUTFontLibrary()
    {

    }

    // Singleton resource retriever
    //--------------------------------------------------------------------------------
    CPUTFontLibrary* CPUTFontLibrary::GetFontLibrary()
    {
        if(m_pFontLibrary)
            return m_pFontLibrary;

        m_pFontLibrary = new CPUTFontLibrary();
        return m_pFontLibrary;
    }


    // Load font
    //--------------------------------------------------------------------------------
    CPUTResult CPUTFontLibrary::LoadFont(const cString Filename, CPUTFont& FontID)
    {
        CPUTResult result;
        result = IsFontAlreadyLoaded(Filename, FontID);
        if( 0!=FontID ) 
        {
            return CPUT_SUCCESS;
        }

        cString imageExtension = _L(".png");
        cString mapExtension = _L(".map");
        size_t found;

        // open the image file that contains the font   
        CPUTTextureLoader* pTextureLoader = CPUTTextureLoader::GetTextureLoader();

        // the filename will be the font name + png for the image
        cString PNGFilename(Filename);

        found = PNGFilename.find(imageExtension);
        if (found==cString::npos)
        {
            PNGFilename.append(imageExtension);
        }

        cString MapFilename(Filename);

        found = MapFilename.find(mapExtension);
        if (found==cString::npos)
        {
            MapFilename.append(mapExtension);
        }


        /*
        // build up the .png name
        size_t copiedCount=0;
        char pPNG[] = ".png";
        int FilenameLength = strlen(pFilename);
        int ExtLength = strlen(pPNG);
        int PNGFilenameLength = FilenameLength + ExtLength + 1;
        char* pPNGFilename = new char[PNGFilenameLength];
        memset((void*) pPNGFilename, 0, PNGFilenameLength);
        strncpy_s(pPNGFilename, PNGFilenameLength, pFilename, FilenameLength);
        strncat_s(pPNGFilename, PNGFilenameLength, pPNG, ExtLength);
        */

        // start setting up data structure
        CPUTLoadedFontItem* pLoadedFontItem = new CPUTLoadedFontItem;  
        pLoadedFontItem->Filename = PNGFilename;
        pLoadedFontItem->pNext = NULL;

        // load the texture file
        result = pTextureLoader->LoadTexture(PNGFilename, pLoadedFontItem->width, pLoadedFontItem->height, pLoadedFontItem->eFormat, &pLoadedFontItem->pData, CPUT_PATH_SEARCH_RESOURCE_DIRECTORY);
        if(CPUTFAILED(result))
            return result;

        HEAPCHECK;

        // open the glyph mapping file
        result = LoadGlyphMappingFile(MapFilename, *pLoadedFontItem);
        if(CPUTFAILED(result))
        {
            delete pLoadedFontItem;
            return result;
        }

        HEAPCHECK;

        // Extract the glyphs from the data file
        result = ExtractGlyphs(*pLoadedFontItem);
        if(CPUTFAILED(result))
        {
            return result;
        }

        HEAPCHECK;

        result = AddFont(pLoadedFontItem);
        if(CPUTFAILED(result))
        {
            return result;
        }

        HEAPCHECK;

        return CPUT_SUCCESS;
    }

}