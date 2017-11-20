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
// File: stdafx.h
//
//      include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently
// ===========================================================================

#if !defined(AFX_STDAFX_H__1FB1B422_B183_11D2_88B7_00C04F990355__INCLUDED_)
#define AFX_STDAFX_H__1FB1B422_B183_11D2_88B7_00C04F990355__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define _WIN32_DCOM

#define VOLATILEFIELDS 1   // Should we allow volatile modifier on fields.


// Insert your headers here
#include <windows.h>
#include <shlwapi.h> // For the PathIs* functions

#include "palclr.h"
#include <corhlpr.h>

#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <time.h>


// Ack!  Why does ATL use malloc in it's headers??!!
//
#undef malloc
#undef realloc
#undef free
#define malloc(a)     VSAlloc(a)
#define realloc(a, b) VSRealloc(a, b)
#define free(pv)      VSFree(pv)

#include "rotor_pal.h"
#include "rotor_palrt.h"
#include "rotor_wrapper.h"
#include "atl.h"
#include <vsmem.h>

#include <vsassert.h>

#define COR_ILEXCEPTION_OFFSETLEN_SUPPORTED 1
#define _META_DATA_NO_SCOPE_ 1
#include <cor.h>
#include <corsym.h>
#include <iceefilegen.h>
#include <strongname.h>
#include <unilib.h>
#include <alink.h>

// Define simple assert macro.
//
// This is ripped off from vs\src\common\inc\vsassert.h
//
#define EXCEPTION_CSHARP_ASSERT 0xE004BEBB

#ifdef DEBUG

#define ASSERT(fTest)                                      \
  do {                                                              \
    static BOOL fDisableThisAssert = FALSE;                         \
    if (!(fTest) && !fDisableThisAssert)                            \
      {                                                             \
      if(g_fStopOnVsAssert ||                                       \
        VsAssert("", #fTest, __FILE__, __LINE__, &fDisableThisAssert))\
        Int3;                                                       \
      }                                                             \
  } while (false)                                                   \

#else // DEBUG

#define ASSERT(fTest) do {} while (0)

#endif // DEBUG



// The benign assert, which does not produce errors or any output...
#ifdef DEBUG

#define BASSERT(fTest)                                      \
  do {                                                              \
    static BOOL fDisableThisAssert = FALSE;                         \
    if (!(fTest) && !fDisableThisAssert)                            \
      {                                                             \
      if(g_fStopOnVsAssert ||                                       \
        VsAssert("", #fTest, __FILE__, __LINE__, &fDisableThisAssert))\
        Int3;                                                       \
      }                                                             \
  } while (false)                                                   \

#else // DEBUG

#define BASSERT(fTest) do {} while (0)

#endif // DEBUG

// vsmem defines new to get leak checking. We don't use new in the compiler.
//#undef new

// This allows the compiler to create nodes
#define DECLARE_BASENODE_NEW

// Define lengthof macro - length of an array.
#define lengthof(a) (sizeof(a) / sizeof((a)[0]))

// Make sure people don't use malloc et. al. in the compiler.
#ifdef calloc
#  undef calloc
#endif
#ifdef malloc
#  undef malloc
#endif
#ifdef realloc
#  undef realloc
#endif
#ifdef free
#  undef free
#endif

#define calloc(num, size)     Dont_use_calloc_in_the_compiler;
#define malloc(size)          Dont_use_malloc_in_the_compiler;
#define realloc(pv, size)     Dont_use_realloc_in_the_compiler;
#define free(pv)              Dont_use_free_in_the_compiler;

// Always use the defines in vsmem for the PAL build
#include "vsmem.h"

#if defined(_MSC_VER)
#pragma warning(disable: 4200)
#endif  // defined(_MSC_VER)

// define this to output a list of unbound methods to stdout at the conclusion of the compilation
//#define USAGEHACK 1

// define this to track memory in retail
//#define TRACKMEM

// this defines memtrack in debug
#ifndef TRACKMEM
#ifdef DEBUG
#define TRACKMEM
#endif
#endif

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#define STACK_ALLOC(t,n) (t*)_alloca ((n)*sizeof(t))

////////////////////////////////////////////////////////////////////////////////
// CComObjectRootMT
//
// ATL didn't seem to have a way to do this explicitly...
//

#if defined(_MSC_VER)
namespace ATL {
#endif  // defined(_MSC_VER)
template <>
class CComObjectRootEx<CComMultiThreadModelNoCS> : public CComObjectRootBase
{
public:

	typedef CComMultiThreadModelNoCS _ThreadModel;

	ULONG InternalAddRef()
	{
		ATLASSERT(m_dwRef != -1L);
		return _ThreadModel::Increment(&m_dwRef);
	}
	ULONG InternalRelease()
	{
		return _ThreadModel::Decrement(&m_dwRef);
	}
	void Lock() {}
	void Unlock() {}
};
#if defined(_MSC_VER)
}
#endif  // defined(_MSC_VER)
typedef CComObjectRootEx<CComMultiThreadModelNoCS> CComObjectRootMT;

#endif // !defined(AFX_STDAFX_H__1FB1B422_B183_11D2_88B7_00C04F990355__INCLUDED_)
