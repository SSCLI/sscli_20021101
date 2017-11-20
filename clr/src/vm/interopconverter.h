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
#ifndef _H_INTEROPCONVERTER_
#define _H_INTEROPCONVERTER_

#include "debugmacros.h"

//
// THE FOLLOWING ARE THE MAIN APIS USED BY EVERY ONE TO CONVERT BETWEEN
// OBJECTREF AND COM IP


//--------------------------------------------------------
// Only IUnknown* is supported without FEATURE_COMINTEROP
//--------------------------------------------------------
IUnknown* __stdcall GetComIPFromObjectRef(OBJECTREF* poref);
OBJECTREF __stdcall GetObjectRefFromComIP(IUnknown* pUnk);
OBJECTREF __stdcall GetObjectRefFromComIP(IUnknown* pUnk, MethodTable* pMTClass);


#endif // #ifndef _H_INTEROPCONVERTER_

