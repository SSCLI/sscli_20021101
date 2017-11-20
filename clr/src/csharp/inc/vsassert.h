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
// File: vsassert.h
//
// ===========================================================================

#ifndef __VSASSERT_H__
#define __VSASSERT_H__

#define VsDebugInitialize()
#define VsDebugTerminate()

#define VSASSERT(a, b) _ASSERTE(a)
#define VsAssert(a, b, c, d, e, f) _ASSERTE(a)
#define VSFAIL(a) _ASSERTE(FALSE)

#define VSDEFINE_SWITCH(a, b, c) 
#define VSFSWITCH(a) false
#ifdef _DEBUG
#define VSVERIFY(fTest, szMsg) VSASSERT((fTest), (szMsg))
#else
#define VSVERIFY(fTest, szMsg) (void)(fTest);
#endif

#endif // __VSASSERT_H__
