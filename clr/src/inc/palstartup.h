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
// File: palstartup.h
//
// An implementation of startup code for Rotor's Unix PAL.  This file should
// be included by any file in a PAL application that defines main.
// palstartupw.h is the Unicode version of this file.
// ===========================================================================

#ifndef __PALSTARTUP_H__
#define __PALSTARTUP_H__

#if PLATFORM_UNIX

#include <rotor_pal.h>

#ifdef __cplusplus
extern "C"
#endif
int __cdecl PAL_startup_main(int argc, char **argv);

#ifdef __cplusplus
extern "C"
#endif
int __cdecl main(int argc, char **argv) {
    if (PAL_Initialize(argc, argv)) {
        return 1;
    }
    // PAL_Terminate is a stdcall function, but it takes no parameters
    // so the difference doesn't matter.
    atexit((void (__cdecl *)(void)) PAL_Terminate);
    exit(PAL_startup_main(argc, argv));
    return 0;   // Quiet a compiler warning
}

#define main    PAL_startup_main

#endif  // PLATFORM_UNIX

#endif  // __PALSTARTUP_H__
