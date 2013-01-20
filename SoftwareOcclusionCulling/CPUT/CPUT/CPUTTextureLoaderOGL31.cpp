#include "CPUTTextureLoaderOGL31.h"
#include <malloc.h>


CPUTTextureLoader* CPUTTextureLoader::m_pTextureLoader = NULL;

// Constructor
//-----------------------------------------------------------------------------
CPUTTextureLoader::CPUTTextureLoader()
{
    m_LoadedTextures.clear();
}

// Destructor
//-----------------------------------------------------------------------------
CPUTTextureLoader::~CPUTTextureLoader()
{
    // walk the list of 'loaded textures' and release them/names
    for(unsigned int i=0; i<m_LoadedTextures.size(); i++)
    {
        // delete the objects in the node
        m_LoadedTextures[i]->Filename.clear();
        glDeleteTextures(1, &m_LoadedTextures[i]->TextureResourceID);
        m_LoadedTextures[i]->TextureResourceID = 0;
        

        // delete the node itself
        delete m_LoadedTextures[i];
    }
    m_LoadedTextures.clear();
}

//  Singleton Texture loader resource
//-----------------------------------------------------------------------------
CPUTTextureLoader* CPUTTextureLoader::GetTextureLoader() 
{            
    if(NULL==m_pTextureLoader)
        m_pTextureLoader = new CPUTTextureLoader;
    return m_pTextureLoader;
}


// Load a texture
//-----------------------------------------------------------------------------
CPUTResult CPUTTextureLoader::LoadTexture(const cString Filename, unsigned int& width, unsigned int& height, CPUT_TEXTURE_FORMAT& eFormat, void** ppImageData, CPUT_PATH_SEARCH_MODE eSearchmode)
{
    if( NULL==ppImageData )
        return CPUT_ERROR_INVALID_PARAMETER;

    // find the location of the file, taking into account the default locations/etc
    CPUTResult result;
    cString FoundPathAndFilename;
    CPUTOSServices* pServices = CPUTOSServices::GetOSServices();
    
    result = pServices->FindFileInStandardSearchDirectories(Filename, FoundPathAndFilename, eSearchmode);
    if(CPUTFAILED(result) )
    {    
        return result;
    }

    result = LoadPNG(FoundPathAndFilename, width, height, eFormat, ppImageData);    
    if(CPUTFAILED(result))
    {
        result = CPUTTextureLoaderBase::LoadRaw(FoundPathAndFilename, width, eFormat, ppImageData);
        if(CPUTSUCCESS(result))
        {
            height=1;
        }
    }


    return result;
}


// Load and register the texture with OpenGL
//-----------------------------------------------------------------------------
CPUTResult CPUTTextureLoader::LoadAndRegisterTexture(const cString Filename, GLuint& textureID, GLuint& width, GLuint& height, bool LoadAndRegister, CPUT_PATH_SEARCH_MODE eSearchmode)
{
    //GLuint textureID=0;    
    CPUTResult result;
    // TODO Optimization: check the repository of loaded textures to see if this has already been loaded
    
    unsigned int ImageHeight=0;
    unsigned int ImageWidth=0;

    CPUT_TEXTURE_FORMAT eFormat;
    void* pRawImageData = NULL;

    result = LoadTexture(Filename, ImageWidth, ImageHeight, eFormat, &pRawImageData, eSearchmode);
    if(CPUTFAILED(result))
    {
        // error!      
        height=0; width=0;
        return result;
    }

    if(true == LoadAndRegister)
    {
        GLint iCurrentlyBoundTexture = 0;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &iCurrentlyBoundTexture);

        // found and loaded - register texture with GL
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        // while registering, save the previously bound texture
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &iCurrentlyBoundTexture);

        // generate a new GL textureID
        glGenTextures(1, &textureID);

        // Set parameters
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

        unsigned int GLType = ConvertCPUTTypeToGLType(eFormat);

        // bind the data to the texture ID
        width = ImageWidth;
        height = ImageHeight;
        glTexImage2D(GL_TEXTURE_2D, 0, GLType, width, height, 0, GLType, GL_UNSIGNED_BYTE, pRawImageData);

        // restore the previous texture binding (so the user is none the wiser)
        glBindTexture(GL_TEXTURE_2D, iCurrentlyBoundTexture);

        // clean up that huge buffer holding the image data
        delete [] pRawImageData;
    }


    return CPUT_SUCCESS;
}

unsigned int CPUTTextureLoader::ConvertCPUTTypeToGLType(CPUT_TEXTURE_FORMAT eFormat)
{
    switch(eFormat)
    {
    case CPUT_TEXTURE_FORMAT_R32G32B32A32:
        return GL_RGBA;
    case CPUT_TEXTURE_FORMAT_R32G32B32:
        return GL_RGB;
    case CPUT_TEXTURE_FORMAT_R16G16B16A16:
        return GL_RGBA;
    case CPUT_TEXTURE_FORMAT_R16G16B16:
        return GL_RGB;
    case CPUT_TEXTURE_FORMAT_R8G8B8A8:
        return GL_RGBA;
    case CPUT_TEXTURE_FORMAT_R8G8B8:
        return GL_RGB;
    case CPUT_TEXTURE_FORMAT_R16A16:
        return GL_RGBA;
    case CPUT_TEXTURE_FORMAT_R16:
        return GL_RED;
    case CPUT_TEXTURE_FORMAT_R8A8:
        return GL_LUMINANCE_ALPHA;
    case CPUT_TEXTURE_FORMAT_R8:
        return GL_RED;

    default:
        // unknown type!
        CPUTOSServices::GetOSServices()->Assert(0);
    }
    return 0;
}

// Attempt to load file as PNG
//-----------------------------------------------------------------------------
CPUTResult CPUTTextureLoader::LoadPNG(const cString Filename, unsigned int& width, unsigned int& height, CPUT_TEXTURE_FORMAT& eFormat, void** ppData)
{
    if( NULL==ppData )
        return CPUT_ERROR_INVALID_PARAMETER;
    
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

    errno_t err = fopen_s(&pFile, Filename.c_str(), "rb");
    if (0 != err)
    {
        return CPUT_ERROR_FILE_NOT_FOUND;
    }
    fread(pngHeader, 1, pngHeaderSize, pFile);
    bool is_png = !png_sig_cmp(pngHeader, 0, pngHeaderSize);
    if (!is_png)
    {
        // error - this is not a recognized PNG file 
        fclose(pFile);
        return CPUT_ERROR_FILE_READ_ERROR;
    }

    // set up PNG load structures
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) 
    {
        fclose(pFile);
        return CPUT_ERROR_SUBSYSTEM_OUT_OF_MEMORY;   // out of memory
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) 
    {
        png_destroy_read_struct(&png_ptr, (png_infopp) NULL, (png_infopp) NULL);
        fclose(pFile);
        return CPUT_ERROR_SUBSYSTEM_OUT_OF_MEMORY;  // out of memory
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

    // return the size parameter
    if(PNG_COLOR_TYPE_GRAY == color_type)
    {
        // component
        if(8==bit_depth)
            eFormat = CPUT_TEXTURE_FORMAT_R8;
        if(16==bit_depth)
            eFormat = CPUT_TEXTURE_FORMAT_R16;
    }
    if(PNG_COLOR_TYPE_GRAY_ALPHA == color_type)
    {
        if(8==bit_depth)
            eFormat = CPUT_TEXTURE_FORMAT_R8A8;
        if(16==bit_depth)
            eFormat = CPUT_TEXTURE_FORMAT_R16A16;
    }
    if(PNG_COLOR_TYPE_RGB == color_type)
    {
        if(8==bit_depth)
            eFormat = CPUT_TEXTURE_FORMAT_R8G8B8;
        if(16==bit_depth)
            eFormat = CPUT_TEXTURE_FORMAT_R16G16B16;
    }

    if(PNG_COLOR_TYPE_RGB_ALPHA == color_type)
    {
        if(8==bit_depth)
            eFormat = CPUT_TEXTURE_FORMAT_R8G8B8A8;
        if(16==bit_depth)
            eFormat = CPUT_TEXTURE_FORMAT_R16G16B16A16;
    }
    if(PNG_COLOR_TYPE_PALETTE == color_type)
    {
        // todo: throw an assertion as most of the rest of the
        // system can't handle paletized formats yet
        eFormat = CPUT_TEXTURE_FORMAT_PALLETIZED;
    }


    // Update the png info struct.
    png_read_update_info(png_ptr, info_ptr);

    // Rowsize in bytes. 
    png_RowSizeInBytes = png_get_rowbytes(png_ptr, info_ptr);


    // Allocate the image_data buffer. 
    if ((image_data = ( char *) malloc(png_RowSizeInBytes * png_height))==NULL) 
    {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

        fclose(pFile);
        return CPUT_ERROR_SUBSYSTEM_OUT_OF_MEMORY;   // not enough memory to allocate image buffer
    }

    if ((row_pointers = (png_bytepp)malloc(png_height*sizeof(png_bytep))) == NULL) 
    {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        
        free(image_data);
        image_data = NULL;
        fclose(pFile);
        return CPUT_ERROR_SUBSYSTEM_OUT_OF_MEMORY;
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
    height = png_height;
    width = png_width;
    
    // cleanup
    delete [] pngHeader;
    free(row_pointers);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

    // close files 
    fclose(pFile);    
    
    return CPUT_SUCCESS;
}