#include "CPUT_OGL31.h"

// context creation/destruction routines
//-----------------------------------------------------------------------------
CPUTResult CPUT_OGL31::CPUTInitialize(const cString ResourceDirectory)
{
    // set where CPUT will look for it's button images, fonts, etc
    return SetCPUTResourceDirectory(ResourceDirectory);
}

// Set where CPUT will look for it's button images, fonts, etc
//-----------------------------------------------------------------------------
CPUTResult CPUT_OGL31::SetCPUTResourceDirectory(const cString ResourceDirectory)
{
    // check to see if the specified directory is valid
    CPUTResult result = CPUT_SUCCESS;

    // resolve the directory to a full path
    cString fullPath;
    CPUTOSServices *pServices = CPUTOSServices::GetOSServices();
    result = pServices->ResolveAbsolutePathAndFilename(ResourceDirectory, &fullPath);
    if(CPUTFAILED(result))
    {
        return result;
    }

    // check existence of directory
    result = pServices->DoesDirectoryExist(fullPath);
    if(CPUTFAILED(result))
    {
        return result;
    }

    // set the resource directory (absolute path)
    m_ResourceDirectory = fullPath;

    // tell the gui system where to look for it's resources
    // todo: do we want to force a flush/reload of all resources (i.e. change control graphics)
    result = CPUTGuiControllerDX11::GetController()->SetResourceDirectory(ResourceDirectory);

    return result;
}
