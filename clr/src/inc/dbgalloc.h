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
#ifndef __DBGALLOC_H_INCLUDED
#define __DBGALLOC_H_INCLUDED

//
// DbgAlloc.h
//
//  Routines layered on top of allocation primitives to provide debugging
//  support.
//

#include "switches.h"


void * __stdcall DbgAlloc(size_t n, void **ppvCallstack);
void __stdcall DbgFree(void *b, void **ppvCallstack);
void __stdcall DbgAllocReport(char *pString = NULL, BOOL fDone = TRUE, BOOL fDoPrintf = TRUE);

#define CDA_DECL_CALLSTACK()
#define CDA_GET_CALLSTACK() NULL

// Routines to verify locks are being opened/closed ok
void DbgIncLock(char* info);
void DbgDecLock(char* info);
void DbgIncBCrstLock();
void DbgIncECrstLock();
void DbgIncBCrstUnLock();
void DbgIncECrstUnLock();


void LockLog(char*);


#ifdef _DEBUG
#define LOCKCOUNTINC(string)    LOCKCOUNTINCL("No info");
#define LOCKCOUNTDEC(string)    LOCKCOUNTDECL("No info");
#define LOCKCOUNTINCL(string)   { DbgIncLock(string); };
#define LOCKCOUNTDECL(string)	{ DbgDecLock(string); };

// Special Routines for CRST locks
#define CRSTBLOCKCOUNTINCL()   { DbgIncBCrstLock(); };
#define CRSTELOCKCOUNTINCL()   { DbgIncECrstLock(); };
#define CRSTBUNLOCKCOUNTINCL()   { DbgIncBCrstUnLock(); };
#define CRSTEUNLOCKCOUNTINCL()   { DbgIncECrstUnLock(); };


#define LOCKLOG(string)         { LockLog(string); };
#else
#define LOCKCOUNTINCL(string)
#define LOCKCOUNTDECL(string)
#define CRSTBLOCKCOUNTINCL()
#define CRSTELOCKCOUNTINCL()
#define CRSTBUNLOCKCOUNTINCL()
#define CRSTEUNLOCKCOUNTINCL()
#define LOCKCOUNTINC(string)
#define LOCKCOUNTDEC(string)
#define LOCKLOG(string)

#endif

#endif
