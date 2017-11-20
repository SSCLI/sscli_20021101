/*++


 Copyright (c) 2002 Microsoft Corporation.  All rights reserved.

 The use and distribution terms for this software are contained in the file
 named license.txt, which can be found in the root of this distribution.
 By using this software in any fashion, you are agreeing to be bound by the
 terms of this license.

 You must not remove this notice, or any other, from this software.


Module Name:

    module.c

Abstract:

    Implementation of module related functions in the Win32 API

--*/

#include "config.h"
#include "pal/palinternal.h"
#include "pal/dbgmsg.h"
#include "pal/module.h"
#include "pal/critsect.h"
#include "pal/thread.h"
#include "pal/file.h"
#include "pal/utils.h"

#include <sys/param.h>
#include <errno.h>
#include <string.h>
#if HAVE_DYLIBS
#include "dlcompat.h"
#else   // HAVE_DYLIBS
#include <dlfcn.h>
#endif  // HAVE_DYLIBS
#if HAVE_ALLOCA_H
#include <alloca.h>
#endif  // HAVE_ALLOCA_H

SET_DEFAULT_DEBUG_CHANNEL(LOADER);

/* macro definitions **********************************************************/

/* get the full name of a module if available, and the short name otherwise*/
#define MODNAME(x) ((x)->lib_name)?(x)->lib_name:(x)->short_name

/* static variables ***********************************************************/

/* critical section that regulates access to the module list */
CRITICAL_SECTION module_critsec;

MODSTRUCT exe_module; /* always the first, in the in-load-order list */
MODSTRUCT pal_module; /* always the second, in the in-load-order list */

/* static function declarations ***********************************************/

static BOOL LOADValidateModule(MODSTRUCT *module);
static LPWSTR LOADGetModuleFileName(MODSTRUCT *module);
static HMODULE LOADLoadLibrary(LPSTR ShortAsciiName);
static void LOAD_SEH_CallDllMain(MODSTRUCT *module);
static MODSTRUCT *LOADAllocModule(void *dl_handle, char *name, 
                                  BOOL name_is_long);

/* API function definitions ***************************************************/

/*++
Function:
  LoadLibraryA

See MSDN doc.
--*/
HMODULE
PALAPI
LoadLibraryA(
         IN LPCSTR lpLibFileName)
{
    LPSTR lpstr;
    HMODULE hModule = NULL;

    ENTRY("LoadLibraryA (lpLibFileName=%p (%s)) \n",
          (lpLibFileName)?lpLibFileName:"NULL",
          (lpLibFileName)?lpLibFileName:"NULL");

    if(NULL == lpLibFileName)
    {
        ERROR("lpLibFileName is NULL;Exit.\n");
        SetLastError(ERROR_MOD_NOT_FOUND);
        goto Done;
    }

    if(lpLibFileName[0]=='\0')
    {
        ERROR("can't load library with NULL file name...\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Done;
    }

    /* do the Dos/Unix conversion on our own copy of the name */
    lpstr = strdup(lpLibFileName);
    if(!lpstr)
    {
        ERROR("memory allocation failure!\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto Done;
    }
    FILEDosToUnixPathA(lpstr);

    hModule = LOADLoadLibrary(lpstr);

    free(lpstr);

    /* let LOADLoadLibrary call SetLastError */
 Done:
    LOGEXIT("LoadLibraryA returns HMODULE %p\n", hModule);
    return hModule;
}


/*++
Function:
  LoadLibraryW

See MSDN doc.
--*/
HMODULE
PALAPI
LoadLibraryW(
         IN LPCWSTR lpLibFileName)
{
    CHAR lpstr[MAX_PATH];
    INT name_length;
    HMODULE hModule = NULL;

    ENTRY("LoadLibraryW (lpLibFileName=%p (%S)) \n",
          lpLibFileName?lpLibFileName:W16_NULLSTRING,
          lpLibFileName?lpLibFileName:W16_NULLSTRING);

    if(NULL == lpLibFileName)
    {
        ERROR("lpLibFileName is NULL;Exit.\n");
        SetLastError(ERROR_MOD_NOT_FOUND);
        goto done;
    }

    if(lpLibFileName[0]==0)
    {
        ERROR("Can't load library with NULL file name...\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    /* do the Dos/Unix conversion on our own copy of the name */

    name_length = WideCharToMultiByte(CP_ACP, 0, lpLibFileName, -1, lpstr,
                                      MAX_PATH, NULL, NULL);
    if( name_length == 0 )
    {
        DWORD dwLastError = GetLastError();
        if( dwLastError == ERROR_INSUFFICIENT_BUFFER )
        {
            ERROR("lpLibFileName is larger than MAX_PATH (%d)!\n", MAX_PATH);
        }
        else
        {
            ASSERT("WideCharToMultiByte failure! error is %d\n", dwLastError);
        }
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    FILEDosToUnixPathA(lpstr);

    /* let LOADLoadLibrary call SetLastError in case of failure */
    hModule = LOADLoadLibrary(lpstr);

done:
    LOGEXIT("LoadLibraryW returns HMODULE %p\n", hModule);
    return hModule;
}


/*++
Function:
  GetProcAddress

See MSDN doc.
--*/
FARPROC
PALAPI
GetProcAddress(
           IN HMODULE hModule,
           IN LPCSTR lpProcName)
{
    MODSTRUCT *module;
    FARPROC ProcAddress = NULL;
    LPCSTR symbolName = lpProcName;

    ENTRY("GetProcAddress (hModule=%p, lpProcName=%p (%s))\n",
          hModule, lpProcName?lpProcName:"NULL", lpProcName?lpProcName:"NULL");

    LockModuleList();

    module = (MODSTRUCT *) hModule;

    /* parameter validation */

    if( (lpProcName == NULL) || (*lpProcName == '\0') )
    {
        TRACE("No function name given\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    if( !LOADValidateModule( module ) )
    {
        TRACE("Invalid module handle %p\n", hModule);
        SetLastError(ERROR_INVALID_HANDLE);
        goto done;
    }
    
    /* try to assert on attempt to locate symbol by ordinal */
    /* this can't be an exact test for HIWORD((DWORD)lpProcName) == 0
       because of the address range reserved for ordinals contain can
       be a valid string address on non-Windows systems
    */
    if( (DWORD)lpProcName < PAGE_SIZE )
    {
        ASSERT("Attempt to locate symbol by ordinal?!\n");
    }

    // Get the symbol's address.
    
    // If we're looking for a symbol inside the PAL, we try the PAL_ variant
    // first because otherwise we run the risk of having the non-PAL_
    // variant preferred over the PAL's implementation.
    if (module->dl_handle == pal_module.dl_handle)
    {
        LPSTR lpPALProcName = alloca(4 + strlen(lpProcName) + 1);
        strcpy(lpPALProcName, "PAL_");
        strcat(lpPALProcName, lpProcName);
        ProcAddress = (FARPROC) dlsym(module->dl_handle, lpPALProcName);
        symbolName = lpPALProcName;
    }

    // If we aren't looking inside the PAL or we didn't find a PAL_ variant
    // inside the PAL, fall back to a normal search.
    if (ProcAddress == NULL)
    {
        ProcAddress = (FARPROC) dlsym(module->dl_handle, lpProcName);
    }

    if (ProcAddress)
    {
        TRACE("Symbol %s found at address %p in module %p (named %S)\n",
              lpProcName, ProcAddress, module, MODNAME(module));

        /* if we don't know the module's full name yet, this is our chance to
           obtain it */
        if(!module->lib_name)
        {
            Dl_info info;

            if(!dladdr(ProcAddress, &info))
            {
                WARN("dladdr() call failed! dlerror says '%s'\n", dlerror());
            }
            else
            {
                module->lib_name = UTIL_MBToWC_Alloc(info.dli_fname, -1);
                if(NULL == module->lib_name)
                {
                    ERROR("MBToWC failure; can't save module's full name\n");
                }
                else
                {
                    TRACE("Saving full path of module %p as %s\n",
                          module, info.dli_fname);
                }
            }
        }
    }
    else
    {
        TRACE("Symbol %s not found in module %p (named %S)\n",
              lpProcName, module, MODNAME(module), dlerror());
        SetLastError(ERROR_PROC_NOT_FOUND);
    }
done:
    UnlockModuleList();
    LOGEXIT("GetProcAddress returns FARPROC %p\n", ProcAddress);
    return ProcAddress;
}


/*++
Function:
  FreeLibrary

See MSDN doc.
--*/
BOOL
PALAPI
FreeLibrary(
        IN OUT HMODULE hLibModule)
{
    MODSTRUCT *module;
    BOOL retval = FALSE;

    ENTRY("FreeLibrary (hLibModule=%p)\n", hLibModule);

    LockModuleList();

    module = (MODSTRUCT *) hLibModule;

    if (terminator)
    {
        /* PAL shutdown is in progress - ignore FreeLibrary calls */
        retval = TRUE;
        goto done;
    }

    if( !LOADValidateModule( module ) )
    {
        TRACE("Can't free invalid module handle %p\n", hLibModule);
        SetLastError(ERROR_INVALID_HANDLE);
        goto done;
    }

    if( module->refcount == -1 )
    {
        /* special module - never released */
        retval=TRUE;
        goto done;
    }

    module->refcount--;
    TRACE("Reference count for module %p (named %S) decreases to %d\n",
            module, MODNAME(module), module->refcount);

    if( module->refcount != 0 )
    {
        retval=TRUE;
        goto done;
    }

    /* Releasing the last reference : call dlclose(), remove module from the
       process-wide module list */

    TRACE("Reference count for module %p (named %S) now 0; destroying "
            "module structure.\n", module, MODNAME(module));

    /* unlink the module structure from the list */
    module->prev->next = module->next;
    module->next->prev = module->prev;

    /* remove the circular reference so that LOADValidateModule will fail */
    module->self=NULL;

    /* Call DllMain if the module contains one */
    if(module->pDllMain)
    {
        TRACE("Calling DllMain (%p) for module %S\n",
                module->pDllMain, 
                module->lib_name ? module->lib_name : W16_NULLSTRING);

/* reset ENTRY nesting level back to zero while inside the callback... */
#if !_NO_DEBUG_MESSAGES_
    {
        int old_level;
        old_level = DBG_change_entrylevel(0);
#endif /* !_NO_DEBUG_MESSAGES_ */
    
        module->pDllMain((HMODULE)module, DLL_PROCESS_DETACH, NULL);

/* ...and set nesting level back to what it was */
#if !_NO_DEBUG_MESSAGES_
        DBG_change_entrylevel(old_level);
    }
#endif /* !_NO_DEBUG_MESSAGES_ */
    }

    if(0!=dlclose(module->dl_handle))
    {
        /* report dlclose() failure, but proceed anyway. */
        WARN("dlclose() call failed! error message is \"%s\"\n", dlerror());
    }

    /* release all memory */
    free(module->lib_name);
    free(module->short_name);
    free(module);

    retval=TRUE;

done:
    UnlockModuleList();
    LOGEXIT("FreeLibrary returns BOOL %d\n", retval);
    return retval;
}


/*++
Function:
  FreeLibraryAndExitThread

See MSDN doc.

--*/
PALIMPORT
VOID
PALAPI
FreeLibraryAndExitThread(
             IN HMODULE hLibModule,
             IN DWORD dwExitCode)
{
    FreeLibrary(hLibModule);
    ExitThread(dwExitCode);
}


/*++
Function:
  GetModuleFileNameA

See MSDN doc.

Notes :
    because of limitations in the dlopen() mechanism, this will only return the
    full path name if a relative or absolute path was given to LoadLibrary, or
    if the module was used in a GetProcAddress call. otherwise, this will return
    the short name as given to LoadLibrary. The exception is if hModule is
    NULL : in this case, the full path of the executable is always returned.
--*/
DWORD
PALAPI
GetModuleFileNameA(
           IN HMODULE hModule,
           OUT LPSTR lpFileName,
           IN DWORD nSize)
{
    INT name_length;
    DWORD retval=0;
    LPWSTR wide_name = NULL;

    ENTRY("GetModuleFileNameA (hModule=%p, lpFileName=%p, nSize=%u)\n",
          hModule, lpFileName, nSize);

    LockModuleList();
    if(hModule && !LOADValidateModule((MODSTRUCT *)hModule))
    {
        TRACE("Can't find name for invalid module handle %p\n", hModule);
        SetLastError(ERROR_INVALID_HANDLE);
        goto done;
    }
    wide_name=LOADGetModuleFileName((MODSTRUCT *)hModule);

    if(!wide_name)
    {
        ASSERT("Can't find name for valid module handle %p\n", hModule);
        SetLastError(ERROR_INTERNAL_ERROR);
        goto done;
    }

    /* Convert module name to Ascii, place it in the supplied buffer */

    name_length = WideCharToMultiByte(CP_ACP, 0, wide_name, -1, lpFileName,
                                      nSize, NULL, NULL);
    if( name_length==0 )
    {
        TRACE("Buffer too small to copy module's file name.\n");
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        goto done;
    }

    TRACE("File name of module %p is %s\n", hModule, lpFileName);
    retval=name_length;
done:
    UnlockModuleList();
    LOGEXIT("GetModuleFileNameA returns DWORD %d\n", retval);
    return retval;
}


/*++
Function:
  GetModuleFileNameW

See MSDN doc.

Notes :
    because of limitations in the dlopen() mechanism, this will only return the
    full path name if a relative or absolute path was given to LoadLibrary, or
    if the module was used in a GetProcAddress call. otherwise, this will return
    the short name as given to LoadLibrary. The exception is if hModule is
    NULL : in this case, the full path of the executable is always returned.
--*/
DWORD
PALAPI
GetModuleFileNameW(
           IN HMODULE hModule,
           OUT LPWSTR lpFileName,
           IN DWORD nSize)
{
    INT name_length;
    DWORD retval=0;
    LPWSTR wide_name = NULL;

    ENTRY("GetModuleFileNameW (hModule=%p, lpFileName=%p, nSize=%u)\n",
          hModule, lpFileName, nSize);

    LockModuleList();

    if(hModule && !LOADValidateModule((MODSTRUCT *)hModule))
    {
        TRACE("Can't find name for invalid module handle %p\n", hModule);
        SetLastError(ERROR_INVALID_HANDLE);
        goto done;
    }
    wide_name=LOADGetModuleFileName((MODSTRUCT *)hModule);

    if(!wide_name)
    {
        ASSERT("Can't find name for valid module handle %p\n", hModule);
        SetLastError(ERROR_INTERNAL_ERROR);
        goto done;
    }

    /* Copy module name into supplied buffer */

    name_length = lstrlenW(wide_name);
    if(name_length>=nSize)
    {
        TRACE("Buffer too small to copy module's file name.\n");
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        goto done;
    }
    lstrcpyW(lpFileName,wide_name);

    TRACE("file name of module %p is %S\n", hModule, lpFileName);
    retval=name_length;
done:
    UnlockModuleList();
    LOGEXIT("GetModuleFileNameW returns DWORD %u\n", retval);
    return retval;
}

/*++
Function:
  PAL_RegisterLibraryW

  Same as LoadLibraryW, but with only the base name of
  the library instead of a full filename.
--*/

HMODULE
PALAPI
PAL_RegisterLibraryW(
         IN LPCWSTR lpLibFileName)
{
    HMODULE hModule;
    LPWSTR libFileName;
    static const WCHAR LIB_PREFIX[] = { 'l', 'i', 'b', '\0' };
#if __APPLE__
    static const WCHAR LIB_SUFFIX[] = { '.', 'd', 'y', 'l', 'i', 'b', '\0' };
#else   // !__APPLE__
    static const WCHAR LIB_SUFFIX[] = { '.', 's', 'o', '\0' };
#endif  // !__APPLE__
    static const int LIB_PREFIX_LENGTH = sizeof(LIB_PREFIX) / sizeof(WCHAR) - 1;
    static const int LIB_SUFFIX_LENGTH = sizeof(LIB_SUFFIX) / sizeof(WCHAR) - 1;

    ENTRY("PAL_RegisterLibraryW (lpLibFileName=%p (%S)) \n",
          lpLibFileName?lpLibFileName:W16_NULLSTRING,
          lpLibFileName?lpLibFileName:W16_NULLSTRING);

    libFileName = (LPWSTR) malloc((PAL_wcslen(lpLibFileName) +
                                LIB_PREFIX_LENGTH + LIB_SUFFIX_LENGTH + 1) *
                                sizeof(WCHAR));
    if (libFileName == NULL) {
        ERROR("malloc() failed\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        LOGEXIT("PAL_RegisterLibraryW returns HMODULE NULL\n");
        return NULL;
    }
    PAL_wcscpy(libFileName, LIB_PREFIX);
    PAL_wcscat(libFileName, lpLibFileName);
    PAL_wcscat(libFileName, LIB_SUFFIX);
    hModule = LoadLibraryW(libFileName);
    free(libFileName);

    LOGEXIT("PAL_RegisterLibraryW returns HMODULE %p\n", hModule);
    return hModule;
}


/*++
Function:
  PAL_UnregisterLibraryW

  Same as FreeLibrary.
--*/
BOOL
PALAPI
PAL_UnregisterLibraryW(
        IN OUT HMODULE hLibModule)
{
    BOOL retval;

    ENTRY("PAL_UnregisterLibraryW (hLibModule=%p)\n", hLibModule);

    retval = FreeLibrary(hLibModule);

    LOGEXIT("PAL_UnregisterLibraryW returns BOOL %d\n", retval);
    return retval;
}

/* Internal PAL functions *****************************************************/


/*++
    LOADGetLibRotorPalSoFileName

    Retrieve the full path of the librotor_pal.so being used.

Parameters:
    None

Return value:
    Pointer to string with the fullpath to the librotor_pal.so being
    used.

    NULL if error occured.

Notes: 
    The string returned by this function is owned by the OS.
    If you need to keep it, strdup() it, because it is unknown how long
    this ptr will point at the string you want (over the lifetime of
    the system running)  It is only safe to use it immediately after calling
    this function.
--*/
const char *LOADGetLibRotorPalSoFileName(void)
{
    Dl_info dl_info;

    /* Call dladdr to get the real name of the PAL library */
    if(!dladdr(&LOADGetLibRotorPalSoFileName,&dl_info))
    {
        /* If we get an error, return NULL */
        return (NULL);
    }
    else
    {
        /* All is good, return the filename */
        return (dl_info.dli_fname);
    }
}

/*++
Function :
    LOADInitializeModules

    Initialize the process-wide list of modules (2 initial modules : 1 for
    the executable and 1 for the PAL)

Parameters :
    LPWSTR exe_name : full path to executable

Return value:
    TRUE  if initialization succeedded
    FALSE otherwise

Notes :
    the module manager takes ownership of the exe_name string
--*/
BOOL LOADInitializeModules(LPWSTR exe_name)
{
    const char *librotor_fname = NULL;
    LPWSTR lpwstr;

    if(exe_module.prev)
    {
        ERROR("Module manager already initialized!\n");
        return FALSE;
    }

    if (0 != SYNCInitializeCriticalSection(&module_critsec))
    {
        ERROR("Module manager critical section initialization failed!\n")
        return FALSE;
    }

    /* initialize module for main executable */
    TRACE("Initializing module for main executable\n");
    exe_module.self=(HMODULE)&exe_module;
    exe_module.dl_handle=dlopen(NULL, RTLD_LAZY);
    if(!exe_module.dl_handle)
    {
        ASSERT("Main executable module will be broken : dlopen(NULL) failed. "
             "dlerror message is \"%s\" \n", dlerror());
    }
    exe_module.short_name = NULL;
    exe_module.lib_name = exe_name;
    exe_module.refcount=-1;
    exe_module.next=&pal_module;
    exe_module.prev=&pal_module;
    exe_module.pDllMain = NULL;
    exe_module.ThreadLibCalls = TRUE;
    
    TRACE("Initializing module for PAL library\n");
    pal_module.self=(HANDLE)&pal_module;

    /* Get the real name of the PAL library */
    librotor_fname = LOADGetLibRotorPalSoFileName();
    if(!librotor_fname)
    {
        ASSERT("PAL module will be broken : LOADGetLibRotorPalSoFileName() "
             "failed. dlerror message is \"%s\" \n", dlerror());
        pal_module.short_name=NULL;
        pal_module.lib_name=NULL;
        pal_module.dl_handle=NULL;
    } else
    {
        TRACE("PAL library is %s\n", librotor_fname);
        lpwstr = UTIL_MBToWC_Alloc(librotor_fname, -1);
        if(NULL == lpwstr)
        {
            ERROR("MBToWC failure, unable to save full name of PAL module\n");
        }
        
        pal_module.short_name=NULL;
        pal_module.lib_name=lpwstr;
        pal_module.dl_handle=dlopen(librotor_fname, RTLD_LAZY);
        
        if(!pal_module.dl_handle)
        {
            ASSERT("PAL module will be broken : dlopen(%s) failed. dlerror "
                 "message is \"%s\"\n ", librotor_fname, dlerror());
        }
    }
    pal_module.refcount=-1;
    pal_module.next=&exe_module;
    pal_module.prev=&exe_module;
    pal_module.pDllMain = NULL;
    pal_module.ThreadLibCalls = TRUE;

    TRACE("Module manager initialization complete.\n");
    return TRUE;
}

/*++
Function :
    LOADFreeModules

    Release all resources held by the module manager (including dlopen handles)

Parameters:
    BOOL bTerminateUnconditionally: If TRUE, this will avoid calling any DllMains

    (no return value)
--*/
void LOADFreeModules(BOOL bTerminateUnconditionally)
{
    MODSTRUCT *module;

    if(!exe_module.prev)
    {
        ERROR("Module manager not initialized!\n");
        return;
    }

    LockModuleList();

    /* Go through the list of modules, release any references we still hold.
       The list is traversed from newest module to oldest */
    do
    {
        module = exe_module.prev;

        // Call DllMain if the module contains one and if we're supposed
        // to call DllMains.
        if( !bTerminateUnconditionally && module->pDllMain )
        {
           /* Exception-safe call to DllMain */
           LOAD_SEH_CallDllMain( module );
        }

        /* Remove the current MODSTRUCT from the list, then free its memory */
        module->prev->next = module->next;
        module->next->prev = module->prev;
        module->self = NULL;

        dlclose( module->dl_handle );

        free( module->lib_name );
        module->lib_name = NULL;
        free( module->short_name );
        module->short_name = NULL;
        if (module != &exe_module && module != &pal_module)
        {
            free( module );
        }
    }
    while( module != &exe_module );

    /* Flag the module manager as uninitialized */
    exe_module.prev = NULL;

    TRACE("Module manager stopped.\n");

    UnlockModuleList();
    DeleteCriticalSection(&module_critsec);
}

/*++
Function :
    LOADCallDllMain

    Call DllMain for all modules (that have one) with the given "fwReason"

Parameters :
    DWORD fdwReason : second parameter to pass to DllMain

(no return value)

Notes :
    This is used to send DLL_THREAD_*TACH messages to modules
--*/
void LOADCallDllMain(DWORD fdwReason)
{
    MODSTRUCT *module;
    BOOL InLoadOrder = TRUE; /* true if in load order, false for reverse */
    THREAD *thread;
    
    thread = PROCGetCurrentThreadObject();
    if (thread != NULL && thread->isInternal)
    {
        return;
    }

    /* Validate fdwReason */
    switch(fdwReason)
    {
    case DLL_PROCESS_ATTACH: 
        ASSERT("got called with DLL_PROCESS_ATTACH parameter! Why?\n");
        break;
    case DLL_PROCESS_DETACH:
        ASSERT("got called with DLL_PROCESS_DETACH parameter! Why?\n");
        InLoadOrder = FALSE;
        break;
    case DLL_THREAD_ATTACH:
        TRACE("Calling DllMain(DLL_THREAD_ATTACH) on all known modules.\n");
        break;
    case DLL_THREAD_DETACH:
        TRACE("Calling DllMain(DLL_THREAD_DETACH) on all known modules.\n");
        InLoadOrder = FALSE;
        break;
    default:
        ASSERT("LOADCallDllMain called with unknown parameter %d!\n", fdwReason);
        return;
    }

    LockModuleList();

    module = &exe_module;
    do {
	if (!InLoadOrder)
	    module = module->prev;

        if (module->ThreadLibCalls)
	{
	    if(module->pDllMain)
	    {
		/* reset ENTRY nesting level back to zero while inside the callback... */
#if !_NO_DEBUG_MESSAGES_
		{
		int old_level;
		old_level = DBG_change_entrylevel(0);
#endif /* !_NO_DEBUG_MESSAGES_ */

		module->pDllMain((HMODULE) module, fdwReason, NULL);

		/* ...and set nesting level back to what it was */
#if !_NO_DEBUG_MESSAGES_
		DBG_change_entrylevel(old_level);
		}
#endif /* !_NO_DEBUG_MESSAGES_ */
	    }
	}

        if (InLoadOrder)
            module = module->next;
    } while (module != &exe_module);

    UnlockModuleList();
}


/*++
Function:
    DisableThreadLibraryCalls

See MSDN doc.
--*/
BOOL
PALAPI
DisableThreadLibraryCalls(
    IN HMODULE hLibModule)
{
    BOOL ret = FALSE;
    MODSTRUCT *module;
    ENTRY("DisableThreadLibraryCalls(hLibModule=%p)\n", hLibModule);

    if (terminator)
    {
        /* PAL shutdown in progress - ignore DisableThreadLibraryCalls */
        ret = TRUE;
        goto done_nolock;
    }

    LockModuleList();
    module = (MODSTRUCT *) hLibModule;

    if(!module || !LOADValidateModule(module))
    {
        // DisableThreadLibraryCalls() does nothing when given
        // an invalid module handle. This matches the Windows
        // behavior, though it is counter to MSDN.
        WARN("Invalid module handle %p\n", hLibModule);
        ret = TRUE;
        goto done;
    }

    module->ThreadLibCalls = FALSE;
    ret = TRUE;

done:
    UnlockModuleList();
done_nolock:
    LOGEXIT("DisableThreadLibraryCalls returns BOOL %d\n", ret);
    return ret;
}


/* Static function definitions ************************************************/

/*++
Function :
    LOADValidateModule

    Check whether the given MODSTRUCT pointer is valid

Parameters :
    MODSTRUCT *module : module to check

Return value :
    TRUE if module is valid, FALSE otherwise

--*/
static BOOL LOADValidateModule(MODSTRUCT *module)
{
    MODSTRUCT *modlist_enum;

    LockModuleList();

    modlist_enum=&exe_module;

    /* enumerate through the list of modules to make sure the given handle is
       really a module (HMODULEs are actually MODSTRUCT pointers) */
    do 
    {
        if(module==modlist_enum)
        {
            /* found it; check its integrity to be on the safe side */
            if(module->self!=module)
            {
                ERROR("Found corrupt module %p!\n",module);
                UnlockModuleList();
                return FALSE;
            }
            UnlockModuleList();
            TRACE("Module %p is valid (name : %S)\n", module,
                  MODNAME(module));
            return TRUE;
        }
        modlist_enum = modlist_enum->next;
    }
    while (modlist_enum != &exe_module);

    TRACE("Module %p is NOT valid.\n", module);
    UnlockModuleList();
    return FALSE;
}

/*++
Function :
    LOADGetModuleFileName [internal]

    Retrieve the module's full path if it is known, the short name given to
    LoadLibrary otherwise.

Parameters :
    MODSTRUCT *module : module to check

Return value :
    pointer to internal buffer with name of module (Unicode)

Notes :
    this function assumes that the module critical section is held, and that
    the module has already been validated.
--*/
static LPWSTR LOADGetModuleFileName(MODSTRUCT *module)
{
    LPWSTR module_name;
    /* special case : if module is NULL, we want the name of the executable */
    if(!module)
    {
        module_name = exe_module.lib_name;
        TRACE("Returning name of main executable\n");
        return module_name;
    }

    /* return "real" name of module if it is known. we have this if LoadLibrary
       was given an absolute or relative path; we can also determine it at the
       first GetProcAdress call. */
    if(module->lib_name)
    {
        TRACE("Returning full path name of module\n");
        return module->lib_name;
    }
    /* otherwise return the short name used in the dlopen() call */
    WARN("Don't know real name of module : returning short name of module\n");
    return module->short_name;
}

/*++
Function :
    LOADAllocModule

    Allocate and initialize a new MODSTRUCT structure

Parameters :
    void *dl_handle :   handle returned by dl_open, goes in MODSTRUCT::dl_handle
    
    char *name :        name of new module. after conversion to widechar, 
                        goes in MODSTRUCT::lib_name or MODSTRUCT::short_name
                        
    BOOL name_is_long : determines whether 'name' is the module's full path 
                        name of only its short name

Return value:
    a pointer to a new, initialized MODSTRUCT strucutre, or NULL on failure.
    
Notes :
    if 'name_is_long' is FALSE, 'name' is used to initialize 
    MODSTRUCT::short_name; otherwise, it is used to initialize 
    MODSTRUCT::lib_name. In either case, the other member is set to NULL
    In case of failure (in malloc or MBToWC), this function sets LastError.
--*/
static MODSTRUCT *LOADAllocModule(void *dl_handle, char *name, 
                                  BOOL name_is_long)
{   
    MODSTRUCT *module;
    LPWSTR wide_name;

    /* no match found : try to create a new module structure */
    module=(MODSTRUCT *) malloc(sizeof(MODSTRUCT));
    if(!module)
    {
        ERROR("malloc() failed! errno is %d (%s)\n", errno, strerror(errno));
        return NULL;
    }

    wide_name = UTIL_MBToWC_Alloc(name, -1);
    if(NULL == wide_name)
    {
        ERROR("couldn't convert name to a wide-character string\n");
        free(module);
        return NULL;
    }

    module->dl_handle = dl_handle;
#if HAVE_DYLIBS
    if (isdylib(module))
    {
        module->refcount = -1;
    }
    else
    {
        module->refcount = 1;
    }
#else   // HAVE_DYLIBS
    module->refcount = 1;
#endif  // HAVE_DYLIBS
    module->self = module;
    module->ThreadLibCalls = TRUE;
    module->next = NULL;
    module->prev = NULL;

    if(name_is_long)
    {
        module->lib_name = wide_name;
        module->short_name = NULL;
    }
    else
    {
        module->short_name = wide_name;
        module->lib_name = NULL;
    }

    return module;
}

/*++
Function :
    LOADLoadLibrary [internal]

    implementation of LoadLibrary (for use by the A/W variants)

Parameters :
    LPSTR ShortAsciiName : name of module as specified to LoadLibrary

Return value :
    handle to loaded module

--*/
static HMODULE LOADLoadLibrary(LPSTR ShortAsciiName)
{
    MODSTRUCT *module = NULL;
    CHAR LongAsciiName[MAXPATHLEN];
    void *dl_handle;
    DWORD dwerror;
    BOOL got_long_name = FALSE;

    LockModuleList();

    /* if a path is specified (relative or absolute), we can determine the full
       path and check if we already loaded that library */
    if(strchr(ShortAsciiName, '/'))
    {
        struct stat theStats;

        TRACE("Looking for module with specific path %s\n", ShortAsciiName);

        got_long_name = TRUE;

        if ( -1 == stat( ShortAsciiName, &theStats ) )
        {
            if ( errno == ENOENT )
            {
                TRACE("File %s doesn't exist.\n", ShortAsciiName);
                SetLastError(ERROR_MOD_NOT_FOUND);
                goto done;
            }
        }

        if ( !UTIL_IsExecuteBitsSet( &theStats ) )
        {
            TRACE("File %s isn't executable.\n", ShortAsciiName);
            SetLastError(ERROR_MOD_NOT_FOUND);
            goto done;
        }

        /* get canonical name of file */
        if(!realpath(ShortAsciiName, LongAsciiName))
        {
            ASSERT("realpath() failed! problem path is %s\n", LongAsciiName);
            SetLastError(ERROR_INTERNAL_ERROR);
            goto done;
        }

        TRACE("Found module; real path is %s\n", LongAsciiName);

        /* see if file can be dlopen()ed; this should work even if it's already
           loaded */
        dl_handle=dlopen(LongAsciiName, RTLD_LAZY);
        if(!dl_handle)
        {
            WARN("dlopen() failed; dlerror says '%s'\n", dlerror()); 
            SetLastError(ERROR_MOD_NOT_FOUND);
            goto done;
        }
        TRACE("dlopen() found module %s\n", LongAsciiName);
    }
    else /* no path information in short name : let dlopen() find it*/
    {
        CHAR TempAsciiName[MAXPATHLEN];
        LPWSTR ExeDir;

        TRACE("Looking for module %s in standard places\n", ShortAsciiName);

        /* first, try to load library from same directory as executable */
        TRACE("Looking for module in application directory\n");

        /* build full path from short name and application directory */
        ExeDir = pAppDir;

        if(0 == WideCharToMultiByte(CP_ACP, 0, ExeDir, -1, TempAsciiName,
                                    MAXPATHLEN-strlen(ShortAsciiName)-1,
                                    NULL, NULL))
        {
            DWORD dwLastError = GetLastError();
            if( dwLastError == ERROR_INSUFFICIENT_BUFFER )
            {
                ERROR("Can't fit application path and module name in "
                      "MAXPATHLEN (%d)!\n", MAXPATHLEN);
            }
            else
            {
                ASSERT("WideCharToMultiByte failure! error is %d\n", dwLastError);
            }
            SetLastError(ERROR_INTERNAL_ERROR);
            goto done;
        }

        /* PROCGetAppPath returns a path without trailing /, so we add one. */
        strcat(TempAsciiName, "/");
        strcat(TempAsciiName, ShortAsciiName);

        /* avoid code duplication : let LOADLoadLibrary try to load it. No risk
           of infinite recursion, since it isn't a short name anymore */
        module = LOADLoadLibrary(TempAsciiName);
        if(module)
        {
            /* Found it :we're done. */
            TRACE("Module %s found in same directory as executable\n",
                  ShortAsciiName);
            goto done;
        }

        /* if we get here, the module name contains no path information and the
           file isn't in the same directory as the executable*/
        TRACE("Module %s not found in application directory; letting dlopen() "
              "look for it\n", ShortAsciiName);

        /* see if file can be dlopen()ed; this should work even if it's already
           loaded */          
        dl_handle=dlopen(ShortAsciiName, RTLD_LAZY);
        if(!dl_handle)
        {
            WARN("dlopen() failed; dlerror says '%s'\n", dlerror());
            SetLastError(ERROR_MOD_NOT_FOUND);
            goto done;
        }
        TRACE("dlopen() found module %s\n", ShortAsciiName);
    }

    /* search module list for a match. if the module was already loaded, 
       dlopen() will have returned the same handle */
    module = &exe_module;
    do
    {
        if(dl_handle == module->dl_handle)
        {   
            /* found the handle. increment the refcount and return the 
               existing module structure */
            TRACE("Found matching module %p for module name %s\n",
                 module, ShortAsciiName);
            if (module->refcount != -1)
                module->refcount++;
            dlclose(dl_handle);
            goto done;
        }
        module = module->next;
    } while (module != &exe_module);

    TRACE("Module doesn't exist : creating it.\n");

    if(got_long_name)
    {
        module = LOADAllocModule(dl_handle, LongAsciiName, TRUE);
    }
    else
    {
        module = LOADAllocModule(dl_handle, ShortAsciiName, FALSE);
    }

    if(NULL == module)
    {
        ERROR("couldn't create new module\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        dlclose(dl_handle);
        goto done;
    }

    /* Add the new module on to the end of the list */
    module->prev = exe_module.prev;
    module->next = &exe_module;
    exe_module.prev->next = module;
    exe_module.prev = module;

    /* If we get here, then we have created a new module structure. We can now
       get the address of DllMain if the module contains one. We save
       the last error and restore it afterward, because our caller doesn't
       care about GetProcAddress failures. */
    dwerror = GetLastError();
    module->pDllMain = GetProcAddress((HMODULE)module, "DllMain");
    SetLastError(dwerror);

    /* If it did contain a DllMain, call it. */
    if(module->pDllMain)
    {
        DWORD dllmain_retval;

        TRACE("Calling DllMain (%p) for module %S\n", 
              module->pDllMain, 
              module->lib_name ? module->lib_name : W16_NULLSTRING);

/* reset ENTRY nesting level back to zero while inside the callback... */
#if !_NO_DEBUG_MESSAGES_
    {
        int old_level;
        old_level = DBG_change_entrylevel(0);
#endif /* !_NO_DEBUG_MESSAGES_ */

        dllmain_retval = module->pDllMain((HINSTANCE) module,
                                          DLL_PROCESS_ATTACH, NULL);

/* ...and set nesting level back to what it was */
#if !_NO_DEBUG_MESSAGES_
        DBG_change_entrylevel(old_level);
    }
#endif /* !_NO_DEBUG_MESSAGES_ */

        /* If DlMain(DLL_PROCESS_ATTACH) returns FALSE, we must immediately
           unload the module.*/
        if(FALSE == dllmain_retval)
        {
            TRACE("DllMain returned FALSE; unloading module.\n");
            module->pDllMain = NULL;
            FreeLibrary((HMODULE) module);
            ERROR("DllMain failed and returned NULL. \n");
            SetLastError(ERROR_DLL_INIT_FAILED);
            module = NULL;
        }
    }
    else
    {
        TRACE("Module does not caontian a DllMain function.\n");
    }

done:
    UnlockModuleList();
    return (HMODULE)module;
}

/*++
Function :
    LOAD_SEH_CallDllMain

    Exception-safe call to DllMain.

Parameters :
    MODSTRUCT *module : module whose DllMain must be called

(no return value)

Notes :
This function is called from LOADFreeModules. Since we get there from
PAL_Terminate, we can't let exceptions in DllMain go unhandled :
TerminateProcess would be called, and would have to abort uncleanly because
termination was already started. So we catch the exception and ignore it;
we're terminating anyway.
*/
static void LOAD_SEH_CallDllMain(MODSTRUCT *module)
{
/* reset ENTRY nesting level back to zero while inside the callback... */
#if !_NO_DEBUG_MESSAGES_
{
    int old_level;
    old_level = DBG_change_entrylevel(0);
#endif /* !_NO_DEBUG_MESSAGES_ */
    
    PAL_TRY
    {
        TRACE("Calling DllMain (%p) for module %S\n",
              module->pDllMain, 
              module->lib_name ? module->lib_name : W16_NULLSTRING);
        
        module->pDllMain((HMODULE)module, DLL_PROCESS_DETACH, NULL);
    }
    PAL_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        WARN("Call to DllMain (%p) got an unhandled exception; "
              "ignoring.\n", module->pDllMain);
    }
    PAL_ENDTRY

/* ...and set nesting level back to what it was */
#if !_NO_DEBUG_MESSAGES_
        DBG_change_entrylevel(old_level);
    }
#endif /* !_NO_DEBUG_MESSAGES_ */
}

/*++
Function:
  LockModuleList

Abstract
  Enter the critical section associated to the module list

Parameter
  void

Return
  void
--*/
void LockModuleList()
{
    SYNCEnterCriticalSection(&module_critsec, TRUE);
}

/*++
Function:
  UnlockModuleList

Abstract
  Leave the critical section associated to the module list

Parameter
  void

Return
  void
--*/
void UnlockModuleList()
{
    SYNCLeaveCriticalSection(&module_critsec, TRUE);
}

