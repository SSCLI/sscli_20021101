/*=============================================================
**
** Source: getmodulefilenamew.c
**
** Purpose: Test the GetModuleFileNameW to retrive the specified module
**          full path and file name in UNICODE.
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
#define UNICODE
#include <palsuite.h>

#define MODULENAMEBUFFERSIZE 1024


int __cdecl main(int argc, char *argv[])
{
    HMODULE ModuleHandle;
    int err;
    WCHAR *lpModuleName;
    DWORD ModuleNameLength;
        WCHAR *ModuleFileNameBuf = malloc(MODULENAMEBUFFERSIZE*sizeof(WCHAR));
    char ModuleName[256]="";
    char* TempBuf = NULL;
    char* LastBuf = NULL;
    char Delimiter[4] = "";
    char NewModuleFileNameBuf[MODULENAMEBUFFERSIZE+200] = "";


    //Initialize the PAL environment
    err = PAL_Initialize(argc, argv);
    if(0 != err)
    {
        ExitProcess(FAIL);
    }

#if WIN32
    sprintf(ModuleName,"%s","rotor_pal.dll");
    sprintf(Delimiter,"%c",'\\');
#elif __APPLE__
    // Darwin/Mac OS X
    sprintf(ModuleName,"%s","librotor_pal.dylib");
    sprintf(Delimiter,"%c",'/');
#else
    //Under FreeBSD
    sprintf(ModuleName,"%s","librotor_pal.so");
    sprintf(Delimiter,"%c",'/');
#endif

    //convert a normal string to a wide one
    lpModuleName = convert(ModuleName);

    //load a module
        ModuleHandle = LoadLibrary(lpModuleName);

    //free the memory
    free(lpModuleName);

    if(!ModuleHandle)
    {
        Fail("Failed to call LoadLibrary API!\n");
    }


    //retrive the specified module full path and file name
    ModuleNameLength = GetModuleFileName(
                ModuleHandle,//specified module handle
                ModuleFileNameBuf,//buffer for module file name
                MODULENAMEBUFFERSIZE);



    //convert a wide full path name to a normal one
    strcpy(NewModuleFileNameBuf,convertC(ModuleFileNameBuf));

    //strip out all full path
    TempBuf = strtok(NewModuleFileNameBuf,Delimiter);
    LastBuf = TempBuf;
    while(NULL != TempBuf)
    {
        LastBuf = TempBuf;
        TempBuf = strtok(NULL,Delimiter);
    }


    //free the memory
    free(ModuleFileNameBuf);

    if(0 == ModuleNameLength || strcmp(ModuleName,LastBuf))
    {
        Trace("\nFailed to all GetModuleFileName API!\n");
        err = FreeLibrary(ModuleHandle);
        if(0 == err)
        {
            Fail("\nFailed to all FreeLibrary API!\n");
        }
        Fail("");
    }



    //decrement the reference count of the loaded dll
    err = FreeLibrary(ModuleHandle);
    if(0 == err)
    {
        Fail("\nFailed to all FreeLibrary API!\n");
    }

    PAL_Terminate();
    return PASS;
}

