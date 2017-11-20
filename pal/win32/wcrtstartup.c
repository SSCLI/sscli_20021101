/*++


 Copyright (c) 2002 Microsoft Corporation.  All rights reserved.

 The use and distribution terms for this software are contained in the file
 named license.txt, which can be found in the root of this distribution.
 By using this software in any fashion, you are agreeing to be bound by the
 terms of this license.

 You must not remove this notice, or any other, from this software.


Module Name:

    wcrtstartup.c

Abstract:

    Light weight version of linked-in MSVCCRT startup
    for Unicode command line applications

--*/

#undef PAL_IMPLEMENTATION
#include "win32pal.h"

int _dowildcard;

int __cdecl wmain(int, wchar_t **, wchar_t **);

struct _startupinfo
{
    int newmode;
};

struct _wmainargs
{
    int argc;
    wchar_t ** argv;
    wchar_t ** envp;
};

__declspec(dllimport) int __cdecl __wgetmainargs(int *pargc, wchar_t ***pargv, wchar_t ***penvp,
    int dowildcard, struct _startupinfo * startinfo);

static void* PALAPI run_wmain(struct _wmainargs *args)
{
    return (void*)wmain(args->argc, args->argv, args->envp);
}

int __cdecl wmainCRTStartup()
{
    int mainret;
    struct _startupinfo startupinfo;
    struct _wmainargs wmainargs;

    // initialize the PAL
    PAL_Initialize(NULL, NULL);

    startupinfo.newmode = 0;

    __wgetmainargs(&wmainargs.argc, &wmainargs.argv, &wmainargs.envp, _dowildcard, &startupinfo);

    // call the user supplied main
    mainret = (int)PAL_LocalFrame((PTHREAD_START_ROUTINE)run_wmain, &wmainargs);

    // shutdown the PAL
    PAL_Terminate();

    ExitProcess(mainret);

    //Never reach this point
    return mainret;
}

int __cdecl wmainCRTStartupWildcard()
{
    _dowildcard = 1;
    return wmainCRTStartup();
}
