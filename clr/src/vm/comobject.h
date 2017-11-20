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
/*============================================================
**
** Header: COMObject.h
**       
**
** Purpose: Native methods on System.Object
**
** Date:  March 27, 1998
** 
===========================================================*/

#ifndef _COMOBJECT_H
#define _COMOBJECT_H

#include "fcall.h"


//
// Each function that we call through native only gets one argument,
// which is actually a pointer to it's stack of arguments.  Our structs
// for accessing these are defined below.
//

class ObjectNative
{
//#pragma pack(push, 4)
    //struct NoArgs
    //{
    //    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, m_pThis);
    //};

    //struct WaitTimeoutArgs
    //{
    //    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, m_pThis);
    //    DECLARE_ECALL_I4_ARG(INT32, m_Timeout);
    //    DECLARE_ECALL_I4_ARG(INT32, m_exitContext);
    //};
//#pragma pack(pop)

public:

    // This method will return a Class object for the object
    //  iff the Class object has already been created.
    //  If the Class object doesn't exist then you must call the GetClass() method.
    static FCDECL1(Object*, GetObjectValue, Object* vThisRef);
    static FCDECL1(Object*, GetExistingClass, Object* vThisRef);
    static FCDECL1(INT32, GetHashCode, Object* vThisRef);
    static FCDECL2(BOOL, Equals, Object *pThisRef, Object *pCompareRef);
    static FCDECL1(Object*, Clone, Object* m_pThis);
    static FCDECL1(ReflectClassBaseObject*, GetClass, Object* pThis);
    static FCDECL3(INT32, WaitTimeout, INT32 exitContext, INT32 Timeout, Object* pThisUNSAFE);
    static FCDECL1(void, Pulse, Object* pThisUNSAFE);
    static FCDECL1(void, PulseAll, Object* pThisUNSAFE);
    static FCDECL1(LPVOID, FastGetClass, Object* vThisRef);
};

#endif
