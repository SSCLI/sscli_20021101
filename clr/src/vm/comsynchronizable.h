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
** Header: COMSynchronizable.h
**       
**
** Purpose: Native methods on System.SynchronizableObject
**          and its subclasses.
**
** Date:  April 1, 1998
** 
===========================================================*/

#ifndef _COMSYNCHRONIZABLE_H
#define _COMSYNCHRONIZABLE_H

#include "field.h"          // For FieldDesc definition.

//
// Each function that we call through native only gets one argument,
// which is actually a pointer to its stack of arguments.  Our structs
// for accessing these are defined below.
//

struct SharedState;

class ThreadNative
{
friend class ThreadBaseObject;

public:

    enum
    {
        PRIORITY_LOWEST = 0,
        PRIORITY_BELOW_NORMAL = 1,
        PRIORITY_NORMAL = 2,
        PRIORITY_ABOVE_NORMAL = 3,
        PRIORITY_HIGHEST = 4,
    };

    enum
    {
        ThreadStopRequested = 1,
        ThreadSuspendRequested = 2,
        ThreadBackground = 4,
        ThreadUnstarted = 8,
        ThreadStopped = 16,
        ThreadWaitSleepJoin = 32,
        ThreadSuspended = 64,
        ThreadAbortRequested = 128,
        ThreadAborted = 256,
    };

    enum
    {
        ApartmentSTA = 0,
        ApartmentMTA = 1,
        ApartmentUnknown = 2
    };

    static LPVOID F_CALL_CONV FastGetCurrentThread();
    static LPVOID F_CALL_CONV FastGetDomain();
    
    static void StartInner(ThreadBaseObject* pThisUNSAFE, Object* pPrincipalUNSAFE, StackCrawlMark* pStackMark);

    static FCDECL1(void, Abort, ThreadBaseObject* pThis);
    static FCDECL1(void, ResetAbort, ThreadBaseObject* pThis);
    static FCDECL3(void,    Start,             ThreadBaseObject* pThisUNSAFE, Object* pPrincipalUNSAFE, StackCrawlMark* pStackMark);
    static FCDECL1(void,    Suspend,           ThreadBaseObject* pThisUNSAFE);
    static FCDECL1(void,    Resume,            ThreadBaseObject* pThisUNSAFE);
    static FCDECL1(INT32,   GetPriority,       ThreadBaseObject* pThisUNSAFE);
    static FCDECL2(void,    SetPriority,       ThreadBaseObject* pThisUNSAFE, INT32 iPriority);
    static FCDECL1(void,    Interrupt,         ThreadBaseObject* pThisUNSAFE);
    static FCDECL1(INT32,   IsAlive,           ThreadBaseObject* pThisUNSAFE);
    static FCDECL1(void,    Join,              ThreadBaseObject* pThisUNSAFE);
    static FCDECL2(INT32,   JoinTimeout,       ThreadBaseObject* pThisUNSAFE, INT32 Timeout);
    static FCDECL1(void,    Sleep,             INT32 iTime);
    static FCDECL2(void,    SetStart,          ThreadBaseObject* pThisUNSAFE, Object* pDelegateUNSAFE);
    static FCDECL2(void,    SetBackground,     ThreadBaseObject* pThisUNSAFE, BYTE isBackground);
    static FCDECL1(INT32,   IsBackground,      ThreadBaseObject* pThisUNSAFE);
    static FCDECL1(INT32,   GetThreadState,    ThreadBaseObject* pThisUNSAFE);
    static FCDECL1(INT32,   GetThreadContext,  ThreadBaseObject* pThisUNSAFE);
    static FCDECL0(Object*, GetDomain);
    static FCDECL1(Object*, GetContextFromContextID,        LPVOID ContextID);
    static FCDECL5(BOOL,    EnterContextFromContextID,      ThreadBaseObject* refThis, ContextBaseObject*, LPVOID contextID, INT32 appDomainIndex, ContextTransitionFrame* pFrame);
    static FCDECL2(BOOL,    ReturnToContextFromContextID,   ThreadBaseObject* refThis, ContextTransitionFrame* pFrame);
    static FCDECL1(void,    InformThreadNameChange,         ThreadBaseObject* thread);
    static FCDECL2(BOOL,    IsRunningInDomain,              ThreadBaseObject* thread, int domainId);
    static FCDECL1(void,    SpinWait,                       int iterations);
    static FCDECL0(Object*, GetCurrentThread);
    static FCDECL0(Object*, GetDomainLocalStore);
    static FCDECL1(void,    Finalize,                       ThreadBaseObject* pThis);
    static FCDECL1(void,    SetDomainLocalStore,            Object* pLocalDataStore);
    static FCDECL1(BOOL,    IsThreadpoolThread,             ThreadBaseObject* thread);
    
public:
    static MethodTable* m_MT;
    static MethodDesc*  m_SetPrincipalMethod;
    static void __stdcall InitThread()
    {
        THROWSCOMPLUSEXCEPTION();
        if (m_MT == NULL) {
            m_MT = g_pThreadClass = g_Mscorlib.GetClass(CLASS__THREAD);
            m_SetPrincipalMethod = g_Mscorlib.GetMethod(METHOD__THREAD__SET_PRINCIPAL_INTERNAL);
        }
    }

private:

    struct KickOffThread_Args {
        Thread *pThread;
        SharedState *share;
        ULONG retVal;
    };

    static void KickOffThread_Worker(LPVOID /* KickOffThread_Args* */);
    static ULONG __stdcall KickOffThread(void *pass);
    static BOOL DoJoin(THREADBASEREF DyingThread, INT32 timeout);
};


#endif // _COMSYNCHRONIZABLE_H

