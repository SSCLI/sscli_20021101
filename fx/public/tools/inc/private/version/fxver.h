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
/**
 * Version strings for Frameworks.
 * 
 */

//
// Insert just the #defines in winver.h, so that the
// C# compiler can include this file after macro preprocessing.
//

#ifdef __cplusplus
#pragma once
#else
#define RC_INVOKED 1
#endif

#include <winver.h>

//
// Include the definitions for rmj, rmm, rup, rpt
//

#include <product_version.h>

/*
 * Product version and name.
 */


#define VER_PRODUCTNAME_STR      "Microsoft (R) .NET Framework"


/*
 * File version, names, description.
 */

// COMPONENT_VER_INTERNALNAME_STR is passed in by the build environment.
#ifndef COMPONENT_VER_INTERNALNAME_STR
#define COMPONENT_VER_INTERNALNAME_STR     UNKNOWN_FILE
#endif

#define VER_INTERNALNAME_STR        QUOTE_MACRO(COMPONENT_VER_INTERNALNAME_STR)
#define VER_ORIGINALFILENAME_STR    QUOTE_MACRO(COMPONENT_VER_INTERNALNAME_STR)

// FX_VER_FILEDESCRIPTION_STR is defined in RC files that include fxver.h

#ifdef FX_VER_FILEDESCRIPTION_STR
#undef FX_VER_FILEDESCRIPTION_STR
#endif

#ifndef FX_VER_FILEDESCRIPTION_STR
#define FX_VER_FILEDESCRIPTION_STR  QUOTE_MACRO(FX_VER_INTERNALNAME_STR)
#endif

#define VER_FILEDESCRIPTION_STR     FX_VER_FILEDESCRIPTION_STR

#ifndef FX_VER_FILEVERSION_STR
#define FX_VER_FILEVERSION_STR      VER_PRODUCTVERSION_STR
#endif

#define VER_FILEVERSION_STR         FX_VER_FILEVERSION_STR

#ifndef FX_VER_FILEVERSION
#define FX_VER_FILEVERSION          VER_PRODUCTVERSION
#endif

#define VER_FILEVERSION             FX_VER_FILEVERSION

//URTVFT passed in by the build environment.
#ifndef URTVFT
#define URTVFT VFT_UNKNOWN
#endif

#define VER_FILETYPE                URTVFT
#define VER_FILESUBTYPE             VFT2_UNKNOWN

/* default is nodebug */
#if DBG
#define VER_DEBUG                   VS_FF_DEBUG
#else
#define VER_DEBUG                   0
#endif

// DEBUG flag is set for debug build, not set for retail build
// #if defined(DEBUG) || defined(_DEBUG)
// #define VER_DEBUG        VS_FF_DEBUG
// #else  // DEBUG
// #define VER_DEBUG        0
// #endif // DEBUG


/* default is prerelease */
#define VER_PRERELEASE              0

// PRERELEASE flag is always set unless building SHIP version
// #ifndef _SHIP
// #define VER_PRERELEASE   VS_FF_PRERELEASE
// #else
// #define VER_PRERELEASE   0
// #endif // _SHIP

#define VER_PRIVATE                 VS_FF_PRIVATEBUILD

// PRIVATEBUILD flag is set if not built by build lab
// #ifndef _VSBUILD
// #define VER_PRIVATEBUILD VS_FF_PRIVATEBUILD
// #else  // _VSBUILD
// #define VER_PRIVATEBUILD 0
// #endif // _VSBUILD


#define VER_FILEFLAGSMASK           VS_FFI_FILEFLAGSMASK
#define VER_FILEFLAGS               (VER_PRERELEASE|VER_DEBUG|VER_PRIVATE)
#define VER_FILEOS                  VOS__WINDOWS32

#define VER_COMPANYNAME_STR         "Microsoft Corporation"

#ifndef VER_LEGALCOPYRIGHT_YEARS
#define VER_LEGALCOPYRIGHT_YEARS    "1998-2002"
#endif

#ifndef VER_LEGALCOPYRIGHT_STR
#if CSC_INVOKED
#define VER_LEGALCOPYRIGHT_STR      "Copyright (C) Microsoft Corporation " + VER_LEGALCOPYRIGHT_YEARS + ". All rights reserved."
#else
#define VER_LEGALCOPYRIGHT_STR      "Copyright (C) Microsoft Corporation " VER_LEGALCOPYRIGHT_YEARS ". All rights reserved."
#endif
#endif

#ifndef VER_LEGALTRADEMARKS_STR
#define VER_LEGALTRADEMARKS_STR     "Microsoft and Windows are either registered trademarks or trademarks of Microsoft Corporation in the U.S. and/or other countries."
#endif


#define EXPORT_TAG


#ifdef VER_LANGNEUTRAL
#define VER_VERSION_UNICODE_LANG  "000004B0" /* LANG_NEUTRAL/SUBLANG_NEUTRAL, Unicode CP */
#define VER_VERSION_ANSI_LANG     "000004E4" /* LANG_NEUTRAL/SUBLANG_NEUTRAL, Ansi CP */
#define VER_VERSION_TRANSLATION   0x0000, 0x04B0
#else
#define VER_VERSION_UNICODE_LANG  "040904B0" /* LANG_ENGLISH/SUBLANG_ENGLISH_US, Unicode CP */
#define VER_VERSION_ANSI_LANG     "040904E4" /* LANG_ENGLISH/SUBLANG_ENGLISH_US, Ansi CP */
#define VER_VERSION_TRANSLATION   0x0409, 0x04B0
#endif

#if CSC_INVOKED
#define VER_COMMENTS_STR        "Microsoft .NET Framework build environement is " + QUOTE_MACRO(URTBLDENV_FRIENDLY) + \
                                ". SafeSync counter=" + QUOTE_MACRO(FX_BRANCH_SYNC_COUNTER_VALUE)
#else
#define VER_COMMENTS_STR        "Microsoft .NET Framework build environement is " QUOTE_MACRO(URTBLDENV_FRIENDLY) \
                                ". SafeSync counter=" QUOTE_MACRO(FX_BRANCH_SYNC_COUNTER_VALUE)
#endif

#define VER_PRIVATEBUILD_STR    QUOTE_MACRO(FX_VER_PRIVATEBUILD_STR)


