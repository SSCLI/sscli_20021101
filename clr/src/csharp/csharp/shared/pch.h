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
// File: pch.h
//
// include all headers that are part of the PCH file
// ===========================================================================
#ifndef __pch_h__
#define __pch_h__

#define _WIN32_WINNT 0x0400
#define _ATL_APARTMENT_THREADED

#include "rotor_palrt.h"
#include "rotor_wrapper.h"
#include "vsmem.h"

#define ASSERT(a) _ASSERTE(a)

#define DLLEXPORT __declspec(dllexport)

#define STACK_ALLOC(t,n) (t*)_alloca ((n)*sizeof(t))
#define USE_RWLOCKS

inline HRESULT AllocateBSTR (PCWSTR pszText, BSTR *pbstrOut)
{
    *pbstrOut = SysAllocString (pszText);
    return *pbstrOut == NULL ? E_OUTOFMEMORY : S_OK;
}

#endif // __pch_h__

