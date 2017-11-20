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
** Header: COMEvent.cpp
**
**                                                  
**
** Purpose: Native methods on System.ManualResetEvent and System.AutoResetEvent
**
** Date:  August, 1999
**
===========================================================*/
#include "common.h"
#include "object.h"
#include "field.h"
#include "reflectwrap.h"
#include "excep.h"
#include "comevent.h"

FCIMPL1(HANDLE, ManualResetEventNative::CorCreateManualResetEvent, BYTE initialState)
{
    THROWSCOMPLUSEXCEPTION();

    HANDLE eventHandle =  WszCreateEvent(NULL, // security attributes
                                         TRUE, // manual event
                                         initialState,
                                         NULL); // no name
    if (eventHandle == NULL)
    {
        HELPER_METHOD_FRAME_BEGIN_RET_0();
        COMPlusThrowWin32();
        HELPER_METHOD_FRAME_END();
    }
    return eventHandle;
}
FCIMPLEND

FCIMPL1(BOOL, ManualResetEventNative::CorSetEvent, LPVOID eventHandle)
{
    _ASSERTE(eventHandle);
    return  SetEvent((HANDLE)eventHandle);
}
FCIMPLEND

FCIMPL1(BOOL, ManualResetEventNative::CorResetEvent, LPVOID eventHandle)
{
    _ASSERTE(eventHandle);
    return  ResetEvent((HANDLE)eventHandle);
}
FCIMPLEND

/***************************************************************************************/
FCIMPL1(HANDLE, AutoResetEventNative::CorCreateAutoResetEvent, BYTE initialState)
{
    THROWSCOMPLUSEXCEPTION();

    HANDLE eventHandle =  WszCreateEvent(NULL, // security attributes
                                         FALSE, // manual event
                                         initialState,
                                         NULL); // no name
    if (eventHandle == NULL)
    {
        HELPER_METHOD_FRAME_BEGIN_RET_0();
        COMPlusThrowWin32();
        HELPER_METHOD_FRAME_END();
    }
    return eventHandle;
}
FCIMPLEND

FCIMPL1(BOOL, AutoResetEventNative::CorSetEvent, LPVOID eventHandle)
{
    _ASSERTE(eventHandle);
    return  SetEvent((HANDLE)eventHandle);
}
FCIMPLEND

FCIMPL1(BOOL, AutoResetEventNative::CorResetEvent, LPVOID eventHandle)
{
    _ASSERTE(eventHandle);
    return  ResetEvent((HANDLE)eventHandle);
}
FCIMPLEND


