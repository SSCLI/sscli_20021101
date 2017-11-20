/*++


 Copyright (c) 2002 Microsoft Corporation.  All rights reserved.

 The use and distribution terms for this software are contained in the file
 named license.txt, which can be found in the root of this distribution.
 By using this software in any fashion, you are agreeing to be bound by the
 terms of this license.

 You must not remove this notice, or any other, from this software.


Module Name:

    crtstartup.c

Abstract:

    Light weight version of linked-in MSVCCRT startup
    for command line applications

--*/

#undef PAL_IMPLEMENTATION
#include "win32pal.h"

int _dowildcard;

int __cdecl main(int, char **, char **);

struct _startupinfo
{
    int newmode;
};

struct _mainargs
{
    int argc;
    char ** argv;
    char ** envp;
};

__declspec(dllimport) int __cdecl __getmainargs(int *pargc, char ***pargv, char ***penvp, 
    int dowildcard, struct _startupinfo * startinfo);

static void* PALAPI run_main(struct _mainargs *args)
{
    return (void*)main(args->argc, args->argv, args->envp);
}

int __cdecl mainCRTStartup()
{
    int mainret;
    struct _startupinfo startupinfo;
    struct _mainargs mainargs;

    // initialize the PAL
    PAL_Initialize(NULL, NULL);

    startupinfo.newmode = 0;

    __getmainargs(&mainargs.argc, &mainargs.argv, &mainargs.envp, _dowildcard, &startupinfo);

    // call the user supplied main
    mainret = (int)PAL_LocalFrame((PTHREAD_START_ROUTINE)run_main, &mainargs);

    // shutdown the PAL
    PAL_Terminate();

    ExitProcess(mainret);

    //Never reach this point
    return mainret;
}

int __cdecl mainCRTStartupWildcard()
{
    _dowildcard = 1;
    return mainCRTStartup();
}
