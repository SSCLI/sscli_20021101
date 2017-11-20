/*++


 Copyright (c) 2002 Microsoft Corporation.  All rights reserved.

 The use and distribution terms for this software are contained in the file
 named license.txt, which can be found in the root of this distribution.
 By using this software in any fashion, you are agreeing to be bound by the
 terms of this license.

 You must not remove this notice, or any other, from this software.


Module Name:

    include/pal/module.h

Abstract:
    Header file for modle management utilities.

--*/

#ifndef _PAL_MODULE_H_
#define _PAL_MODULE_H_

typedef struct _MODSTRUCT
{
    HMODULE self;         /* circular reference to this module */
    void *dl_handle;      /* handle returned by dlopen() */
    LPWSTR lib_name;      /* full path of module */
    LPWSTR short_name;    /* library name as given to LoadLibrary */
    INT refcount;         /* reference count */
                          /* -1 means infinite reference count - module is never released */
    BOOL ThreadLibCalls;  /* TRUE for DLL_THREAD_ATTACH/DETACH notifications 
                              enabled, FALSE if they are disabled */

    BOOL (* PALAPI pDllMain)(HINSTANCE,DWORD,LPVOID); /* entry point of module */

    /* reference to next and previous modules in list (in load order) */
    struct _MODSTRUCT *next;
    struct _MODSTRUCT *prev;
} MODSTRUCT;

extern MODSTRUCT pal_module;


/*++
Function :
    LoadInitializeModules

    Initialize the process-wide list of modules (2 initial modules : 1 for
    the executable and 1 for the PAL)

Parameters :
    LPWSTR exe_name : full path to executable

Return value :
    TRUE on success, FALSE on failure

Notes :
    the module manager takes ownership of the string
--*/
BOOL LOADInitializeModules(LPWSTR exe_name);

/*++
Function :
    LOADFreeModules

    Release all resources held by the module manager (including dlopen handles)

Parameters:
    BOOL bTerminateUnconditionally: If TRUE, this will avoid calling any DllMains

    (no return value)
--*/
void LOADFreeModules(BOOL bTerminateUnconditionally);

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
void LOADCallDllMain(DWORD fdwReason);

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
void LockModuleList();

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
void UnlockModuleList();


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
const char *LOADGetLibRotorPalSoFileName(void);

#endif /* _PAL_MODULE_H_ */

