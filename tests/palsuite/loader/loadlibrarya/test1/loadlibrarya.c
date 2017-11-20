/*=============================================================
**
** Source:  loadlibrarya.c
**
** Purpose: Positive test the LoadLibrary API.
**          Call LoadLibrary to map a module into the calling 
**          process address space(DLL file)
**
** 
**  Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
** 
**  The use and distribution terms for this software are contained in the file
**  named license.txt, which can be found in the root of this distribution.
**  By using this software in any fashion, you are agreeing to be bound by the
**  terms of this license.
** 
**  You must not remove this notice, or any other, from this software.
** 
**
**============================================================*/
#include <palsuite.h>


int __cdecl main(int argc, char *argv[])
{
    HMODULE ModuleHandle;
    char ModuleName[256] = "";
    int err;

    /* Initialize the PAL environment */
    err = PAL_Initialize(argc, argv);
    if(0 != err)
    {
        ExitProcess(FAIL);
    }

#if WIN32
    sprintf(ModuleName,"%s","rotor_pal.dll");
#elif __APPLE__
    // Darwin/Mac OS X
    sprintf(ModuleName,"%s","librotor_pal.dylib");
#else
    /* Under FreeBSD */
    sprintf(ModuleName,"%s","librotor_pal.so");
#endif
    /* load a module */
    ModuleHandle = LoadLibrary(ModuleName);
    if(!ModuleHandle)
    {
        Fail("Failed to call LoadLibrary API!\n");
    }

    /* decrement the reference count of the loaded dll */
    err = FreeLibrary(ModuleHandle);
    if(0 == err)
    {
        Fail("\nFailed to all FreeLibrary API!\n");
    }

    PAL_Terminate();
    return PASS;
}
