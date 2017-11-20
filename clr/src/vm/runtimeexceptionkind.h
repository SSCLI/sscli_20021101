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
// RuntimeExceptionKind.h - Copyrioht (C) 2001 Microsoft Corporation
//

#ifndef __runtimeexceptionkind_h__
#define __runtimeexceptionkind_h__

//==========================================================================
// Identifies commonly-used exception classes for COMPlusThrowable().
//==========================================================================
enum RuntimeExceptionKind {
#define EXCEPTION_BEGIN_DEFINE(ns, reKind, hr) k##reKind,
#define EXCEPTION_ADD_HR(hr)
#define EXCEPTION_END_DEFINE()
#include "rexcep.h"
kLastException
#undef EXCEPTION_BEGIN_DEFINE
#undef EXCEPTION_ADD_HR
#undef EXCEPTION_END_DEFINE
};

#endif  // __runtimeexceptionkind_h__
