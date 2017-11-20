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
// File: VsMem.H
//
// Common memory allocation routines
// ===========================================================================

#ifndef _INC_VSMEM_H
#define _INC_VSMEM_H

#include "vsassert.h"

#undef VSAlloc
#undef VSAllocZero
#undef VSRealloc
#undef VSReallocZero
#undef VSFree
#undef VSSize

#define VSAlloc(cb)             HeapAlloc(GetProcessHeap(), 0, cb)
#define VSAllocZero(cb)         HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cb)
#define VSRealloc(pv, cb)       HeapReAlloc(GetProcessHeap(), 0, pv, cb)
#define VSFree(pv)              HeapFree(GetProcessHeap(), 0, pv)
#define VSSize(pv)              HeapSize(GetProcessHeap(), 0, pv)

#define VSHeapDestroy(heap, fLeakCheck) HeapDestroy(heap)
#define VSHeapAlloc(heap, cb)           HeapAlloc(heap, 0, cb)
#define VSHeapAllocZero(heap, cb)       HeapAlloc(heap, HEAP_ZERO_MEMORY, cb)
#define VSHeapRealloc(heap, pv, cb)     HeapReAlloc(heap, 0, pv, cb)
#define VSHeapReallocZero(heap, pv, cb) HeapReAlloc(heap, HEAP_ZERO_MEMORY, pv, cb)
#define VSHeapFree(heap, pv)            HeapFree(heap, 0, pv)
#define VSHeapSize(heap, pv)            HeapSize(heap, 0, pv)

// 
// Rotor doesn't support HEAP_ZERO_MEMORY so add extra parameter (cbOld) with the original
// size and memset the new memory.
inline void *VSReallocZero(void *pv, size_t cb, size_t cbOld)
{
  BYTE *tmpAlloc = (BYTE *)HeapReAlloc(GetProcessHeap(), 0, pv, cb);
  if (tmpAlloc)
  {
      memset(tmpAlloc+cbOld, 0, cb-cbOld);
  }
  return tmpAlloc;
}

#endif // _INC_VSMEM_H
