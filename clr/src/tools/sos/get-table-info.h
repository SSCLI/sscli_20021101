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
/*
 */
#ifndef INC_GET_TABLE_INFO
#define INC_GET_TABLE_INFO

#include <stddef.h>
#ifndef PLATFORM_UNIX
#include <basetsd.h>
#endif

struct ClassDumpTable;

ClassDumpTable *GetClassDumpTable();
ULONG_PTR GetMemberInformation (size_t klass, size_t member);
SIZE_T GetClassSize (size_t klass);
ULONG_PTR GetEEJitManager ();
ULONG_PTR GetEconoJitManager ();
ULONG_PTR GetMNativeJitManager ();

#endif /* ndef INC_GET_TABLE_INFO */
