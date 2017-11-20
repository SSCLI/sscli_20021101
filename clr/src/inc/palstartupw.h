// ==++==
// 
//   
//    Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
//   
//    The use and distribution terms for this software are contained in the file
//    named license.txt, which can be found in the root of this distribution.
//    By using this software in any fashion, you are agreeing to be bound by the
//    terms of this license.
//   
//    You must not remove this notice, or any other, from this software.
//   
// 
// ==--==
// ===========================================================================
// File: palstartupw.h
//
// An implementation of startup code for Rotor's Unix PAL.  This file should
// be included by any file in a PAL application that defines wmain.
// palstartup.h is the ANSI version of this file.
// ===========================================================================

#ifndef __PALSTARTUPW_H__
#define __PALSTARTUPW_H__

#if PLATFORM_UNIX

#include <rotor_pal.h>

#ifdef __cplusplus
extern "C"
#endif
int __cdecl PAL_startup_wmain(int argc, WCHAR **argv);

#ifdef __cplusplus
extern "C"
#endif
int __cdecl main(int argc, char **argv) {
    WCHAR **wargv;
    int i;
    
    if (PAL_Initialize(argc, argv)) {
        return 1;
    }
    // PAL_Terminate is a stdcall function, but it takes no parameters
    // so the difference doesn't matter.
    atexit((void (__cdecl *)(void)) PAL_Terminate);
    
    wargv = (WCHAR **) alloca(sizeof(wargv[0]) * (argc + 1));
    for(i = 0; i < argc; i++) {
        int len;
        
        len = MultiByteToWideChar(CP_ACP, 0, argv[i], -1, NULL, 0);
        wargv[i] = (WCHAR *) alloca(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, argv[i], -1, wargv[i], len);
    }
    wargv[argc] = NULL;
    exit(PAL_startup_wmain(argc, wargv));
    return 0;   // Quiet a compiler warning
}

#define wmain   PAL_startup_wmain

#endif  // PLATFORM_UNIX

#endif  // __PALSTARTUPW_H__
