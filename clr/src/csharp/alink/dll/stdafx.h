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
// File: stdafx.h :
//      include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently
//
// ===========================================================================

#if !defined(AFX_STDAFX_H__ADDBC8E1_02FF_4CB4_9306_3AD3DAE21F7F__INCLUDED_)
#define AFX_STDAFX_H__ADDBC8E1_02FF_4CB4_9306_3AD3DAE21F7F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_APARTMENT_THREADED


#include <rotor_pal.h>
#include <rotor_palrt.h>
#include <rotor_wrapper.h>
#include <atl.h>


#include <vsmem.h>
#include "palclr.h"

#define COR_ILEXCEPTION_OFFSETLEN_SUPPORTED 1
#define _META_DATA_NO_SCOPE_ 1
#include <cor.h>
#include <corsym.h>
#include <iceefilegen.h>
#include <strongname.h>

#include <unilib.h>

#ifdef _DEBUG
#define ASSERT(expr) VSASSERT((expr), "")
#define VERIFY(expr) VSVERIFY((expr), "")
#else

#define ASSERT(expr) do {} while(0)
#define VERIFY(expr) (void)(expr);
#endif

#ifndef DEBUG

#undef VSAlloc
#undef VSAllocZero
#undef VSRealloc
#undef VSFree
#undef VSSize

// NOTE:  These have changed to use the HeapAlloc family (as opposed to LocalAlloc family)
// because of HeapReAlloc's behavior (a block may move to satisfy a realloc request, as
// opposed to LocalReAlloc's behavior of simply failing).
#define VSAlloc(cb)             HeapAlloc(GetProcessHeap(), 0, cb)
#define VSAllocZero(cb)         HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cb)
#define VSRealloc(pv, cb)       HeapReAlloc(GetProcessHeap(), 0, pv, cb)
#define VSFree(pv)              HeapFree(GetProcessHeap(), 0, pv)
#define VSSize(pv)              HeapSize(GetProcessHeap(), 0, pv)

#endif //DEBUG

// Define lengthof macro - length of an array.
#define lengthof(a) (sizeof(a) / sizeof((a)[0]))
// Define a simple string allocator (copies the string too)
#define VSAllocStr(str) (wcscpy((WCHAR*)VSAlloc(sizeof(WCHAR)*(DWORD)(wcslen(str)+1)), str))

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__ADDBC8E1_02FF_4CB4_9306_3AD3DAE21F7F__INCLUDED)
