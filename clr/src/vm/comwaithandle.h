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
** Header: COMWaitHandle.h
**       
**
** Purpose: Native methods on System.WaitHandle
**
** Date:  August, 1999
** 
===========================================================*/

#ifndef _COM_WAITABLE_HANDLE_H
#define _COM_WAITABLE_HANDLE_H


class WaitHandleNative
{
    // The following should match the definition in the classlib (managed) file
private:

    //struct WaitOneArgs
    //{
    //    DECLARE_ECALL_I4_ARG(INT32 /*bool*/, exitContext);
    //    DECLARE_ECALL_I4_ARG(INT32, timeout);
    //    DECLARE_ECALL_I4_ARG(LPVOID, handle);
    //};

    //struct WaitMultipleArgs
    //{
    //    DECLARE_ECALL_I4_ARG(INT32, waitForAll);
    //    DECLARE_ECALL_I4_ARG(INT32, exitContext);
    //    DECLARE_ECALL_I4_ARG(INT32, timeout);
    //    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, waitObjects);
    //};

public:
    static FCDECL3(BOOL, CorWaitOneNative, LPVOID handle, INT32 timeout, BYTE exitContext);
    static FCDECL4(INT32, CorWaitMultipleNative, Object* waitObjectsUNSAFE, INT32 timeout, BYTE exitContext, BYTE waitForAll);
};
#endif
