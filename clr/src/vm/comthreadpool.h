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
** Header: COMThreadPool.h
**       
**
** Purpose: Native methods on System.ThreadPool
**          and its inner classes
**
** Date:  August, 1999
**
===========================================================*/

#ifndef _COMTHREADPOOL_H
#define _COMTHREADPOOL_H

#include <member-offset-info.h>

typedef VOID (__stdcall *WAITORTIMERCALLBACK)(PVOID, BOOL);

#define STRUCT_ENTRY(FnName, FnType, FnParamList, FnArgs)   \
    extern FnType COM##FnName FnParamList ;                 
    
#include "tpoolfnsp.h"
#include "delegateinfo.h"
#undef STRUCT_ENTRY

class ThreadPoolNative
{
//#pragma pack(push,4)
//    struct RegisterWaitForSingleObjectsArgs
//    {
//        DECLARE_ECALL_I4_ARG(BOOL, compressStack);
//        DECLARE_ECALL_PTR_ARG(StackCrawlMark*, stackMark);
//        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, registeredWaitObject);
//        DECLARE_ECALL_I4_ARG(BOOL, executeOnlyOnce);
//        DECLARE_ECALL_I8_ARG(INT64, timeout);
//        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, state);
//        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, delegate);
//        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, waitObject);
//
//    };
//#pragma pack(pop)

    //struct UnregisterWaitArgs
    //{
    //    DECLARE_ECALL_PTR_ARG(LPVOID, objectToNotify);
    //    DECLARE_ECALL_PTR_ARG(LPVOID, WaitHandle);
    //};

    //struct WaitHandleCleanupArgs
    //{
    //    DECLARE_ECALL_PTR_ARG(LPVOID, WaitHandle);
    //};

    //struct QueueUserWorkItemArgs
    //{
    //    DECLARE_ECALL_I4_ARG(BOOL, compressStack);
    //    DECLARE_ECALL_PTR_ARG(StackCrawlMark*, stackMark);
    //    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, state);
    //    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, delegate);
    //};

    //struct BindIOCompletionCallbackArgs
    //{
    //    DECLARE_ECALL_I4_ARG(HANDLE, fileHandle);
    //};

public:

    static void Init();

    static FCDECL2(VOID, CorGetMaxThreads, DWORD* workerThreads, DWORD* completionPortThreads);
    static FCDECL2(VOID, CorGetAvailableThreads, DWORD* workerThreads, DWORD* completionPortThreads);
    static FCDECL8(LPVOID, CorRegisterWaitForSingleObject, Object* waitObjectUNSAFE, Object* delegateUNSAFE, Object* stateUNSAFE, INT64 timeout, BOOL executeOnlyOnce, Object* registeredWaitObjectUNSAFE, StackCrawlMark* stackMark, BOOL compressStack);
    static FCDECL4(VOID, CorQueueUserWorkItem, Object* delegateUNSAFE, Object* stateUNSAFE, StackCrawlMark* stackMark, BYTE compressStack);
    static FCDECL2(BOOL, CorUnregisterWait, LPVOID WaitHandle, LPVOID objectToNotify);
    static FCDECL1(void, CorWaitHandleCleanupNative, LPVOID WaitHandle);
};

class TimerBaseNative;
typedef TimerBaseNative* TIMERREF;

class TimerBaseNative : public MarshalByRefObjectBaseObject 
{
    friend class TimerNative;
    friend class ThreadPoolNative;

    LPVOID          timerHandle;
    DelegateInfo*   delegateInfo;
    LONG            timerDeleted;

    __inline LPVOID         GetTimerHandle() {return timerHandle;}
    __inline void           SetTimerHandle(LPVOID handle) {timerHandle = handle;}
    __inline PLONG          GetAddressTimerDeleted() { return &timerDeleted;}
    __inline BOOL           IsTimerDeleted() { return timerDeleted;}

public:
    __inline DelegateInfo*  GetDelegateInfo() { return delegateInfo;}
    __inline void           SetDelegateInfo(DelegateInfo* delegate) { delegateInfo = delegate;}
};
class TimerNative
{
    friend struct MEMBER_OFFSET_INFO(TimerNative);
private:
    //struct ChangeTimerArgs
    //{
    //    DECLARE_ECALL_OBJECTREF_ARG(TIMERREF, pThis);
    //    DECLARE_ECALL_I4_ARG(INT32, period);
    //    DECLARE_ECALL_I4_ARG(INT32, dueTime);
    //};
    //struct DeleteTimerArgs
    //{
    //    DECLARE_ECALL_OBJECTREF_ARG(TIMERREF, pThis);
    //    DECLARE_ECALL_PTR_ARG(LPVOID, notifyObjectHandle);    
    //};
    //struct AddTimerArgs
    //{
    //    DECLARE_ECALL_OBJECTREF_ARG(TIMERREF, pThis);
    //    DECLARE_ECALL_PTR_ARG(StackCrawlMark*, stackMark);
    //    DECLARE_ECALL_I4_ARG(INT32, period);
    //    DECLARE_ECALL_I4_ARG(INT32, dueTime);
    //    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, state);
    //    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, delegate);
    //};
    //struct NoArgs
    //{
    //    DECLARE_ECALL_OBJECTREF_ARG(TIMERREF, pThis);
    //};
 
    static VOID WINAPI TimerNative::timerDeleteWorkItem(PVOID,BOOL);
    public:
    static FCDECL6(VOID, CorCreateTimer, TimerBaseNative* pThisUNSAFE, Object* delegateUNSAFE, Object* stateUNSAFE, INT32 dueTime, INT32 period, StackCrawlMark* stackMark);
    static FCDECL3(BOOL, CorChangeTimer, TimerBaseNative* pThisUNSAFE, INT32 dueTime, INT32 period);
    static FCDECL2(BOOL, CorDeleteTimer, TimerBaseNative* pThisUNSAFE, LPVOID notifyObjectHandle);
};



#endif
