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
// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
// ===========================================================================

#if !defined(AFX_STDAFX_H__1FB1B429_B183_11D2_88B7_00C04F990355__INCLUDED_)
#define AFX_STDAFX_H__1FB1B429_B183_11D2_88B7_00C04F990355__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define _WIN32_DCOM

//#define TRACKMEM

#ifdef DEBUG
#ifndef TRACKMEM
#define TRACKMEM
#endif
#endif
// Reference additional headers your program requires here
#include <rotor_pal.h>
#include <rotor_palrt.h>
#include <rotor_wrapper.h>

#include <vsassert.h>
#include <vsmem.h>

#define ASSERT(x) VSASSERT(x, "")

#undef VSAlloc
#undef VSAllocZero
#undef VSRealloc
#undef VSFree
#undef VSSize

#ifndef DEBUG

#ifndef TRACKMEM
#define VSAlloc(cb)       HeapAlloc(GetProcessHeap(), 0, cb)
#define VSAllocZero(cb)   HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cb)
#define VSRealloc(pv, cb) HeapReAlloc(GetProcessHeap(), 0, pv, cb)
#define VSFree(pv)        HeapFree(GetProcessHeap(), 0, pv)
#define VSSize(pv)        HeapSize(GetProcessHeap(), 0, pv)
#else // trackmem && !debug

extern size_t memCur, memMax;
inline void * VSAlloc(size_t cb) {
    memCur += cb;
    if (memCur > memMax)
        memMax = memCur;
    return HeapAlloc(GetProcessHeap(), 0, cb);
}
inline void * VSAllocZero(size_t cb) {
    memCur += cb;
    if (memCur > memMax)
        memMax = memCur;
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cb);
}
inline void * VSRealloc(void *pv, size_t cb) {
    memCur += cb - HeapSize(GetProcessHeap(), 0, pv);
    if (memCur > memMax)
        memMax = memCur;
    return HeapReAlloc(GetProcessHeap(), 0, pv, cb);
}
inline void VSFree(void *pv) {
    memCur -= HeapSize(GetProcessHeap(), 0, pv);
    HeapFree(GetProcessHeap(), 0, pv);
}
#define VSSize(pv)        HeapSize(GetProcessHeap(), 0, pv)

#endif // TRACKMEM

#else // DEBUG

extern size_t memCur, memMax;
inline void * VSAlloc(size_t cb) {
    memCur += cb;
    if (memCur > memMax)
        memMax = memCur;
    return VsDebAlloc(0, cb);
}
inline void * VSAllocZero(size_t cb) {
    memCur += cb;
    if (memCur > memMax)
        memMax = memCur;
    return VsDebAlloc(HEAP_ZERO_MEMORY, cb);
}
inline void * VSRealloc(void *pv, size_t cb) {
    memCur += cb - VsDebSize(pv);
    if (memCur > memMax)
        memMax = memCur;
    return VsDebRealloc(pv, 0, cb);
}
inline void VSFree(void *pv) {
    memCur -= VsDebSize(pv);
    VsDebFree(pv);
}

#define VSSize(pv)        VsDebSize(pv)

#endif //DEBUG

// Define lengthof macro - length of an array.
#define lengthof(a) (sizeof(a) / sizeof((a)[0]))


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.



#endif // !defined(AFX_STDAFX_H__1FB1B429_B183_11D2_88B7_00C04F990355__INCLUDED_)
