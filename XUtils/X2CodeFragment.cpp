#include <stdlib.h>
#include <stdio.h>
#include "X2CodeFragment.h"
#include "X2RtcCheck.h"

#if _WIN32
    // Win32 includes here
#elif __MacOSX__
    #include <CoreFoundation/CFString.h>
    #include <CoreFoundation/CFBundle.h>
#else
#include <dlfcn.h>
#endif

X2CodeFragment::X2CodeFragment(const char* inPath)
: fFragmentP(NULL)
{
#if defined(HPUX) || defined(HPUX10)
    shl_t handle;
    fFragmentP = shl_load(inPath, BIND_IMMEDIATE|BIND_VERBOSE|BIND_NOSTART, 0L);
#elif defined(OSF1) ||\
    (defined(__FreeBSD_version) && (__FreeBSD_version >= 220000))
    fFragmentP = dlopen((char *)inPath, RTLD_NOW | RTLD_GLOBAL);
#elif defined(__FreeBSD__)
    fFragmentP = dlopen(inPath, RTLD_NOW);
#elif defined(__sgi__) 
    fFragmentP = dlopen(inPath, RTLD_NOW); // not sure this should be either RTLD_NOW or RTLD_LAZY
#elif defined(_WIN32)
    fFragmentP = ::LoadLibraryA(inPath);
#elif defined(__MacOSX__)
    CFStringRef theString = CFStringCreateWithCString( kCFAllocatorDefault, inPath, kCFStringEncodingASCII);

        //
        // In MacOSX, our "fragments" are CF bundles, which are really
        // directories, so our paths are paths to a directory
        CFURLRef    bundleURL = CFURLCreateWithFileSystemPath(  kCFAllocatorDefault,
                                                            theString,
                                                            kCFURLPOSIXPathStyle,
                                                            true);

    //
    // I figure CF is safe about having NULL passed
    // into its functions (if fBundle failed to get created).
    // So, I won't worry about error checking myself
    fFragmentP = CFBundleCreate( kCFAllocatorDefault, bundleURL );
    Boolean success = false;
    if (fFragmentP != NULL)
        success = CFBundleLoadExecutable( fFragmentP );
    if (!success && fFragmentP != NULL)
    {
        CFRelease( fFragmentP );
        fFragmentP = NULL;
    }
    
    CFRelease(bundleURL);
    CFRelease(theString);
    
#else
    fFragmentP = dlopen(inPath, RTLD_NOW | RTLD_GLOBAL);
    //fprintf (stderr, "%s\n", dlerror());

#endif
}

X2CodeFragment::~X2CodeFragment()
{
    if (fFragmentP == NULL)
        return;
        
#if defined(HPUX) || defined(HPUX10)
    shl_unload((shl_t)fFragmentP);
#elif defined(_WIN32)
    BOOL theErr = ::FreeLibrary(fFragmentP);
    X2RTC_CHECK(theErr);
#elif defined(__MacOSX__)
    CFBundleUnloadExecutable( fFragmentP );
    CFRelease( fFragmentP );
#else
    dlclose(fFragmentP);
#endif
}

void*   X2CodeFragment::GetSymbol(const char* inSymbolName)
{
    if (fFragmentP == NULL)
        return NULL;
        
#if defined(HPUX) || defined(HPUX10)
    void *symaddr = NULL;
    int status;

    errno = 0;
    status = shl_findsym((shl_t *)&fFragmentP, symname, TYPE_PROCEDURE, &symaddr);
    if (status == -1 && errno == 0) /* try TYPE_DATA instead */
        status = shl_findsym((shl_t *)&fFragmentP, inSymbolName, TYPE_DATA, &symaddr);
    return (status == -1 ? NULL : symaddr);
#elif defined(DLSYM_NEEDS_UNDERSCORE)
    char *symbol = (char*)malloc(sizeof(char)*(strlen(inSymbolName)+2));
    void *retval;
    qtss_sprintf(symbol, "_%s", inSymbolName);
    retval = dlsym(fFragmentP, symbol);
    free(symbol);
    return retval;
#elif defined(_WIN32)
    return ::GetProcAddress(fFragmentP, inSymbolName);
#elif defined(__MacOSX__)
    CFStringRef theString = CFStringCreateWithCString( kCFAllocatorDefault, inSymbolName, kCFStringEncodingASCII);
    void* theSymbol = (void*)CFBundleGetFunctionPointerForName( fFragmentP, theString );
    CFRelease(theString);
    return theSymbol;
#else
    return dlsym(fFragmentP, inSymbolName);
#endif  
}
