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
 * tpoolwrap.cpp
 *
 * Wrapper for all threadpool functions. 
 * 
*/

#include "common.h"
#include "eeconfig.h"
#include "win32threadpool.h"

typedef VOID (__stdcall *WAITORTIMERCALLBACK)(PVOID, BOOL);


//+------------------------------------------------------------------------
//
//  Define inline functions which call through the global functions. The
//  functions are defined from entries in tpoolfns.h.
//
//-------------------------------------------------------------------------

#define STRUCT_ENTRY(FnName, FnType, FnParamList, FnArgs)   \
        FnType COM##FnName FnParamList                      \
        {                                                   \
            return ThreadpoolMgr::FnName FnArgs;            \
        }                                                   \

#include "tpoolfnsp.h"

#undef STRUCT_ENTRY




