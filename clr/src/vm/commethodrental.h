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
////////////////////////////////////////////////////////////////////////////////
// Date: May 6, 1999
////////////////////////////////////////////////////////////////////////////////

#ifndef _COMMETHODRENTAL_H_
#define _COMMETHODRENTAL_H_

#include "excep.h"
#include "reflectwrap.h"
#include "comreflectioncommon.h"
#include "fcall.h"

// COMMethodRental
// This class implements SwapMethodBody for our MethodRenting story
class COMMethodRental
{
public:

    // COMMethodRental    
    //struct _SwapMethodBodyArgs { 
    //    DECLARE_ECALL_PTR_ARG(StackCrawlMark*, stackMark);
    //    DECLARE_ECALL_I4_ARG(INT32, flags);
    //    DECLARE_ECALL_I4_ARG(INT32, iSize);
    //    DECLARE_ECALL_I4_ARG(LPVOID, rgMethod);
    //    DECLARE_ECALL_I4_ARG(INT32, tkMethod);
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, cls);
    //};
    
    // COMMethodRental.SwapMethodBody -- this function will swap an existing method body with
    // a new method body
    //
    static FCDECL6(void, SwapMethodBody, ReflectClassBaseObject* clsUNSAFE, INT32 tkMethod, LPVOID rgMethod, INT32 iSize, INT32 flags, StackCrawlMark* stackMark);
};

#endif //_COMMETHODRENTAL_H_
