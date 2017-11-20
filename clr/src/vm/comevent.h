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
** Header: COMEvent.h
**        
**
** Purpose: Native methods on System.ManualResetEvent and System.AutoResetEvent
**
** Date:  August, 1999
** 
===========================================================*/

#ifndef _COMEVENT_H
#define _COMEVENT_H
#include "comwaithandle.h"

class ManualResetEventNative;
class AutoResetEventNative;

typedef ManualResetEventNative* MANUAL_RESET_EVENT_REF;

class ManualResetEventNative :public WaitHandleNative
{
    //struct CreateEventArgs
    //{
    //    DECLARE_ECALL_I4_ARG(INT32, initialState);
    //};
    //struct SetEventArgs
    //{
    //    DECLARE_ECALL_I4_ARG(LPVOID, eventHandle);
    //};

public:
    static FCDECL1(HANDLE, CorCreateManualResetEvent, BYTE initialState);
    static FCDECL1(BOOL, CorSetEvent, LPVOID eventHandle);
    static FCDECL1(BOOL, CorResetEvent, LPVOID eventHandle);
};

typedef AutoResetEventNative* AUTO_RESET_EVENT_REF;

class AutoResetEventNative : public WaitHandleNative
{
    //struct CreateEventArgs
    //{
    //    DECLARE_ECALL_I4_ARG(INT32, initialState);
    //};
    //struct SetEventArgs
    //{
    //    DECLARE_ECALL_I4_ARG(LPVOID, eventHandle);
    //};

public:
    static FCDECL1(HANDLE, CorCreateAutoResetEvent, BYTE initialState);
    static FCDECL1(BOOL, CorSetEvent, LPVOID eventHandle);
    static FCDECL1(BOOL, CorResetEvent, LPVOID eventHandle);
};

#endif
