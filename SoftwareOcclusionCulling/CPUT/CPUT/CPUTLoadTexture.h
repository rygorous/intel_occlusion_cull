#ifndef __CPUTTEXTURE_H__
#define __CPUTTEXTURE_H__
#include <stdio.h>
#include <string.h> // substr

#include "png.h"    // libPNG library

enum eCPUT_TEXTURE_FORMAT
{
    CPUT_TEXTURE_FORMAT_R32G32B32A32,
    CPUT_TEXTURE_FORMAT_R32G32B32,
    CPUT_TEXTURE_FORMAT_R8G8B8A8,
    CPUT_TEXTURE_FORMAT_R8G8B8,
};

// these need to be in the same order as g_CPUTKnownTextureTypes
enum eCPUT_TEXTURE_EXTENSION
{
    CPUT_TEXTURE_EXTENSION_PNG,
    CPUT_TEXTURE_EXTENSION_DDS,
    CPUT_TEXTURE_EXTENSION_UNKNOWN,
};

const char *g_CPUTKnownTextureTypes[]  = { "png", "dds", "dxt", "S3C", '\0' };
const char *g_CPUTKnownDirectories[] = { ".\\", "..\\", '\0'};

class CPUTLoadTexture
{
public:
    CPUTLoadTexture();
    bool LoadFile(char* filename, int* height, int* width, eCPUT_TEXTURE_FORMAT* eFormat, void** ppImageData);

private:
    bool FindFile(char* pFilename, char** ppPathAndFile);
    bool FindFileInStandardSearchDirectories(char* pFilename, char** ppPathAndFile);
    bool HasExtension(char* pFilename, char** ppExtension, eCPUT_TEXTURE_EXTENSION* eFileExtension);

    bool LoadPNG(char* pFilename, int* height, int* width, eCPUT_TEXTURE_FORMAT* eFormat, void** ppData);
    bool LoadRaw(char* pFilename, int* size, eCPUT_TEXTURE_FORMAT* eFormat, void** ppData);
};



// constructor
//-----------------------------------------------------------------------------
CPUTLoadTexture::CPUTLoadTexture()
{

}


// determine file format, and find it on the disc using standard path search
//-----------------------------------------------------------------------------
bool CPUTLoadTexture::LoadFile(char* pFilename, int* height, int* width, eCPUT_TEXTURE_FORMAT* eFormat, void** ppImageData)
{
    if( (NULL==pFilename) || (NULL==height) || (NULL==width) || (NULL==eFormat) || (NULL==ppImageData) )
        return false;

    // find the file (if you can)
    char* foundAtString=NULL;
    if(!FindFile(pFilename, &foundAtString))
    {
        return false;
    }

    // found a file with specified name 
    // try to load it using the known image formats we have

    // png
    if( LoadPNG(foundAtString, height, width, eFormat, ppImageData) )
    {
        return true;
    }

    // raw

    return true;
}


// Try to find a specified file
//-----------------------------------------------------------------------------
bool CPUTLoadTexture::FindFile(char* pFilename, char** ppPathAndFile)
{
    bool found = false;

    if((NULL == pFilename) || (NULL==ppPathAndFile))
        return false;

    // see if there is an extension on this file
    char* pExtension=NULL;
    eCPUT_TEXTURE_EXTENSION eFileExtension;
    if(!HasExtension(pFilename, &pExtension, &eFileExtension))
    {
        // alright, no extension, so we'll need to try each extension in each directory (sigh)

        // loop over known extensions
        int i=0;
        const char* extension = g_CPUTKnownTextureTypes[i];
        while('\0' != extension)
        {
            // attach extension
            int size = (strlen(pFilename)+strlen(extension)+1);
            char* pFilenameWithExtension = new char[ size ];
            strcat_s(pFilenameWithExtension, size, pFilename);      
            strcat_s(pFilenameWithExtension, size, extension);
            pFilenameWithExtension[0]='\0';

            // check for existence in directory tree
            found = FindFileInStandardSearchDirectories(pFilename, ppPathAndFile);
            delete [] pFilenameWithExtension;

            // found, return what you found
            if( found )
            {
                return found;
            }            

            i++;
            extension = g_CPUTKnownTextureTypes[i];
        }
        // search for filename in each directory
    }
    else
    {
        // ok, we have a valid file w/ it's extension
        // find it
        return FindFileInStandardSearchDirectories(pFilename, ppPathAndFile);
    }
    return false;
}

// Try to find a extension of file (if it has one)
//-----------------------------------------------------------------------------
bool CPUTLoadTexture::FindFileInStandardSearchDirectories(char* pFilename, char** ppPathAndFile)
{
    bool found = false;
    int filenameLength = strlen(pFilename);

    // is it right where they said it was?
    FILE* pFile = NULL;
    errno_t err = fopen_s(&pFile, pFilename, "r");
    if(0 == err)
    {
        // yep - return the correct filename/path in ppPathAndFile
        fclose(pFile);        
        *ppPathAndFile = new char[filenameLength+1];
        strcpy_s(*ppPathAndFile, filenameLength+1, pFilename);

        return true;
    }
    else
    {
        // now you have to search the standard list of directories
        int i=0;
        const char* pDirectoryPrefix = g_CPUTKnownDirectories[i];
        
        while('\0' != pDirectoryPrefix)
        {
            // prefix filename with path
            int size = filenameLength + strlen(pDirectoryPrefix) + 1;
            char* pFilenameWithDirectory = new char[size];
            pFilenameWithDirectory[0]='\0';
            strcat_s(pFilenameWithDirectory, size, pDirectoryPrefix);
            strcat_s(pFilenameWithDirectory, size, pFilename);

            // does file exist there?
            err = fopen_s(&pFile, pFilenameWithDirectory, "r");
            if(0 == err)
            {
                // yep - return the correct filename/path in *ppPathAndFile
                fclose(pFile);        
                *ppPathAndFile = pFilenameWithDirectory;
                return true;
            }

            i++;
            pDirectoryPrefix = g_CPUTKnownDirectories[i];
        } 

    }


    return false;
}

// Try to find a extension of file (if it has one)
//-----------------------------------------------------------------------------
bool CPUTLoadTexture::HasExtension(char* pFilename, char** ppExtension, eCPUT_TEXTURE_EXTENSION* eFileExtension)
{
    bool found = false;
    if((NULL==pFilename) || (NULL==ppExtension))
        return false;

    // compare against list of known extensions
    int filenameLen = (int) strlen(pFilename);
    int dotLocation = filenameLen-4;
    if( ('.'==pFilename[dotLocation]) ) 
    {        
        //strip off the extension
        char* pExtension = new char[4];
        strncpy_s(pExtension, 4, &pFilename[dotLocation+1], 3);
        *ppExtension = pExtension;
        found = true;
    }
    else 
    {
        dotLocation--;
        if( ('.'==pFilename[dotLocation]) ) 
        {
            //strip off the extension
            char* pExtension = new char[5];
            strncpy_s(pExtension, 5, &pFilename[dotLocation+1], 4);
            *ppExtension = pExtension;
            found = true;
        }
    }

    // compare extension string with known extensions
    int i=0;
    *eFileExtension = CPUT_TEXTURE_EXTENSION_UNKNOWN;

    while('\0' != g_CPUTKnownTextureTypes[i])
    {
        // convert incoming to lower case
        int size = strlen(*ppExtension)+1;
        char* pExtension = new char[size];
        strcpy_s(pExtension, size, *ppExtension);

        _strlwr_s(pExtension, size);

        // is it the same?
        if(0==strcmp(pExtension, g_CPUTKnownTextureTypes[i]))
        {
            // report and return
            *eFileExtension = (eCPUT_TEXTURE_EXTENSION)i;
            return found;
        }
        i++;
    }

    return found;
}

// RAW reading ala DXT/S3C
//-----------------------------------------------------------------------------
bool CPUTLoadTexture::LoadRaw(char* pFilename, int* size, eCPUT_TEXTURE_FORMAT* eFormat, void** ppData)
{

    return true;
}

// Attempt to load file as PNG
//-----------------------------------------------------------------------------
bool CPUTLoadTexture::LoadPNG(char* pFilename, int* height, int* width, eCPUT_TEXTURE_FORMAT* eFormat, void** ppData)
{
    if( (NULL==pFilename) || (NULL==height) || (NULL==width) || (NULL==eFormat) )
        return false;

    // call libPNG loader facility
    FILE *pFile = NULL;
    const int       pngHeaderSize = 8;
    png_bytep       pngHeader=(png_bytep) new png_byte[8];  // PNG signature (first 8 bytes)
    png_structp     png_ptr=NULL;                           // used internally by libpng 
    png_infop       info_ptr=NULL;                          // user requested transforms 
    
    png_uint_32     png_height=0;
    png_uint_32     png_width=0;
    int             bit_depth=0;
    int             color_type=0;
    png_uint_32     png_RowSizeInBytes=0;
    png_bytepp      row_pointers = NULL;            // row pointer used internally by libpng
    char            *image_data=NULL;               // stores the actual image data

    errno_t err = fopen_s(&pFile, pFilename, "rb");
    if (0 != err)
    {
        return false;
    }
    fread(pngHeader, 1, pngHeaderSize, pFile);
    bool is_png = !png_sig_cmp(pngHeader, 0, pngHeaderSize);
    if (!is_png)
    {
        // error - this is not a recognized PNG file 
        fclose(pFile);
        return false;
    }

    // set up PNG load structures
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) 
    {
        fclose(pFile);
        return false;   // out of memory
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) 
    {
        png_destroy_read_struct(&png_ptr, (png_infopp) NULL, (png_infopp) NULL);
        fclose(pFile);
        return false;  // out of memory
    }

    // Takes our file stream pointer (pFile) and
    // stores it in the png_ptr struct for internal libpng use.
    png_init_io(png_ptr, pFile);


    // lets libpng know that we already checked the 8 
    // signature bytes, so it should not expect to find 
    // them at the current file pointer location
    png_set_sig_bytes(png_ptr, pngHeaderSize);


    // Read the png image info block
    // Reads and processes not only the PNG file's IHDR chunk 
    // but also any other chunks up to the first IDAT 
    // (i.e., everything before the image data).
    //
    // gets the image width, height and format
    png_read_info(png_ptr, info_ptr);
    png_get_IHDR(png_ptr, info_ptr, &png_width, &png_height, &bit_depth, &color_type, NULL, NULL, NULL);

    // set up any image formatting parameters
    // 1. ensure 8-bit depth
    // 2. ensure it's RGB instead of grayscale or palettized color
    if (bit_depth > 8) 
    {
        png_set_strip_16(png_ptr);
    }
    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) 
    {
            png_set_gray_to_rgb(png_ptr);
    }
    if (color_type == PNG_COLOR_TYPE_PALETTE) 
    {
        png_set_palette_to_rgb(png_ptr);
    }

    // Update the png info struct.
    png_read_update_info(png_ptr, info_ptr);

    // Rowsize in bytes. 
    png_RowSizeInBytes = png_get_rowbytes(png_ptr, info_ptr);


    // Allocate the image_data buffer. */
    if ((image_data = ( char *) malloc(png_RowSizeInBytes * png_height))==NULL) 
    {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

        fclose(pFile);
        return false;   // not enough memory to allocate image buffer
    }

    if ((row_pointers = (png_bytepp)malloc(png_height*sizeof(png_bytep))) == NULL) 
    {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        
        free(image_data);
        image_data = NULL;
        fclose(pFile);
        return false;
    }

    // Set the individual row_pointers to point at the correct offsets 
    for(png_uint_32 i = 0;  i < png_height;  ++i)
    {
        row_pointers[i] = (png_byte*)image_data + i*png_RowSizeInBytes;        
    }


    // whew - NOW we can read the image
    png_read_image(png_ptr, row_pointers);
    

    // And we're done!  (png_read_end() can be omitted if no processing of
    // post-IDAT text/time/etc. is desired), but I do it anyway
    png_read_end(png_ptr, info_ptr);


    // update the user's variables
    *ppData = (void*)image_data;
    *height = png_height;
    *width = png_width;
    
    // cleanup
    delete [] pngHeader;
    free(row_pointers);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

    // close files 
    fclose(pFile);    

    return true;
}
#endif //__CPUTLOADTEXTURE_H__