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
// File: opdef.h
//
// ===========================================================================

#ifndef BOOLOPTDEF
#error you must define BOOLOPTDEF macro before including OPTDEF.H!
#endif
#ifndef STROPTDEF
#error you must define STROPTDEF macro before including OPTDEF.H!
#endif

////////////////////////////////////////////////////////////////////////////////
// OPTIONS
//
//
//          ID                      DESC ID                 SHORT               LONG                    BOTH                        FLAGS (COF_BOOLEAN is implied correctly)

BOOLOPTDEF( WARNINGSAREERRORS,      WARNINGSAREERRORS,      NULL,               L"warnaserror",         L"warnaserror",             COF_GRP_ERRORS)
BOOLOPTDEF( NOSTDLIB,               NOSTDLIB,               NULL,               L"nostdlib",            L"nostdlib",                COF_GRP_ADVANCED)
BOOLOPTDEF( EMITDEBUGINFO,          EMITDEBUGINFO,          NULL,               L"debug",               L"debug",                   COF_GRP_CODE)
STROPTDEF(  DEBUGTYPE,              DEBUGTYPE,              NULL,               L"debug",               L"debug",                   COF_GRP_CODE | COF_ARG_DEBUGTYPE)
BOOLOPTDEF( OPTIMIZATIONS,          OPTIMIZATIONS,          L"o",               L"optimize",            L"o[ptimize]",              COF_GRP_CODE)
BOOLOPTDEF( INCBUILD,               INCBUILD,               L"incr",            L"incremental",         L"incr[emental]",           COF_GRP_CODE)
BOOLOPTDEF( CHECKED,                CHECKED,                NULL,               L"checked",             L"checked",                 COF_GRP_LANGUAGE)
BOOLOPTDEF( UNSAFE,                 UNSAFE,                 NULL,               L"unsafe",              L"unsafe",                  COF_GRP_LANGUAGE)

STROPTDEF(  WARNINGLEVEL,           WARNINGLEVEL,           L"w",               L"warn",                L"w[arn]",                  COF_HASDEFAULT | COF_GRP_ERRORS | COF_ARG_NUMBER)
STROPTDEF(  NOWARNLIST,             NOWARNLIST,             NULL,               L"nowarn",              L"nowarn",                  COF_GRP_ERRORS | COF_ARG_WARNLIST)
STROPTDEF(  CCSYMBOLS,              CCSYMBOLS,              L"d",               L"define",              L"d[efine]",                COF_GRP_OUTPUT | COF_ARG_SYMLIST)
STROPTDEF(  IMPORTS,                IMPORTS,                L"r",               L"reference",           L"r[eference]",             COF_GRP_INPUT | COF_ARG_FILELIST)
STROPTDEF(  INTERNALTESTS,          HIDDEN,                 NULL,               L"test",                L"test",                    COF_HIDDEN)
STROPTDEF(  MODULES,                MODULES,                NULL,               L"addmodule",           L"addmodule",               COF_GRP_INPUT | COF_ARG_FILELIST)
STROPTDEF(  LIBPATH,                LIBPATH,                NULL,               L"lib",                 L"lib",                     COF_GRP_ADVANCED | COF_ARG_FILELIST)

#undef BOOLOPTDEF
#undef STROPTDEF
