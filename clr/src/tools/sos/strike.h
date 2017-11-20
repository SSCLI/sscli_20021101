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
#ifndef __strike_h__
#define __strike_h__

#ifndef PLATFORM_UNIX
#pragma warning(disable:4245)   // signed/unsigned mismatch
#pragma warning(disable:4100)   // unreferenced formal parameter
#pragma warning(disable:4201)   // nonstandard extension used : nameless struct/union
#pragma warning(disable:4127)   // conditional expression is constant

#include <wchar.h>
//#include <heap.h>
//#include <ntsdexts.h>
#include <windows.h>
#else
#include <gdbwrap.h>
#endif


//#define NOEXTAPI
#define KDEXT_64BIT
#include <wdbgexts.h>
#undef DECLARE_API

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <malloc.h>
#include <stddef.h>

#ifndef PLATFORM_UNIX
#include <basetsd.h> 
#endif 

#define  CORHANDLE_MASK 0x1

// C_ASSERT() can be used to perform many compile-time assertions:
#define C_ASSERT(e) typedef char __C_ASSERT__[(e)?1:-1]

#include "exts.h"

extern BOOL CallStatus;

// Function Prototypes (implemented in strike.cpp; needed by SonOfStrike.cpp)
DECLARE_API(DumpStack);
DECLARE_API(SyncBlk);
DECLARE_API(RWLock);
DECLARE_API(DumpObj);
DECLARE_API(DumpDomain);
DECLARE_API(EEVersion);
DECLARE_API(EEDLLPath);

#endif // __strike_h__

