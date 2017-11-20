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
** Header: COMThreadPool.cpp
**
**                                                  
**
** Purpose: Native methods on System.ThreadPool
**          and its inner classes
**
** Date:  August, 1999
** 
===========================================================*/

/********************************************************************************************************************/
#include "common.h"
#include "comdelegate.h"
#include "comthreadpool.h"
#include "win32threadpool.h"
#include "class.h"
#include "object.h"
#include "field.h"
#include "reflectwrap.h"
#include "excep.h"
#include "security.h"
#include "eeconfig.h"

/*****************************************************************************************************/
#ifdef _DEBUG
void LogCall(MethodDesc* pMD, LPCUTF8 api)
{
    LPCUTF8 cls  = pMD->GetClass() ? pMD->GetClass()->m_szDebugClassName
                                   : "GlobalFunction";
    LPCUTF8 name = pMD->GetName();

    LOG((LF_THREADPOOL,LL_INFO1000,"%s: ", api));
    LOG((LF_THREADPOOL, LL_INFO1000,
         " calling %s.%s\n", cls, name));
}
#else
#define LogCall(pMd,api) 
#endif

/*****************************************************************************************************/
DelegateInfo *DelegateInfo::MakeDelegateInfo(AppDomain *pAppDomain, 
                                             OBJECTREF delegate,                                              
                                             OBJECTREF state,
                                             OBJECTREF waitEvent,
                                             OBJECTREF registeredWaitHandle)
{
    THROWSCOMPLUSEXCEPTION();
    DelegateInfo* delegateInfo = (DelegateInfo*) ThreadpoolMgr::GetRecycledMemory(ThreadpoolMgr::MEMTYPE_DelegateInfo);
    _ASSERTE(delegateInfo);
    if (NULL == delegateInfo)
        COMPlusThrow(kOutOfMemoryException);
    delegateInfo->m_appDomainId = pAppDomain->GetId();

    delegateInfo->m_delegateHandle = pAppDomain->CreateHandle(delegate);
    delegateInfo->m_stateHandle = pAppDomain->CreateHandle(state);
    delegateInfo->m_eventHandle = pAppDomain->CreateHandle(waitEvent);
    delegateInfo->m_registeredWaitHandle = pAppDomain->CreateHandle(registeredWaitHandle);

    delegateInfo->m_compressedStack = NULL;
    delegateInfo->m_overridesCount = 0;
    delegateInfo->m_hasSecurityInfo = FALSE;

    return delegateInfo;
}

/*****************************************************************************************************/
FCIMPL2(VOID, ThreadPoolNative::CorGetMaxThreads,DWORD* workerThreads, DWORD* completionPortThreads)
{
    ThreadpoolMgr::GetMaxThreads(workerThreads,completionPortThreads);
    return;
}
FCIMPLEND

/*****************************************************************************************************/
FCIMPL2(VOID, ThreadPoolNative::CorGetAvailableThreads,DWORD* workerThreads, DWORD* completionPortThreads)
{
    ThreadpoolMgr::GetAvailableThreads(workerThreads,completionPortThreads);
    return;
}
FCIMPLEND

/*****************************************************************************************************/

struct RegisterWaitForSingleObjectCallback_Args
{
    DelegateInfo *delegateInfo;
    BOOL TimerOrWaitFired;
};

static void RegisterWaitForSingleObjectCallback_Worker(LPVOID ptr)
{
    RegisterWaitForSingleObjectCallback_Args *args = (RegisterWaitForSingleObjectCallback_Args *) ptr;
    Thread *pThread = GetThread();
    if ((args->delegateInfo)->m_hasSecurityInfo)
    {
        _ASSERTE( Security::IsSecurityOn() && "This block should only be called if security is on" );
        pThread->SetDelayedInheritedSecurityStack( (args->delegateInfo)->m_compressedStack );
        pThread->CarryOverSecurityInfo( (args->delegateInfo)->m_compressedStack->GetOverridesCount(), (args->delegateInfo)->m_compressedStack->GetAppDomainStack() );
        if (!(args->delegateInfo)->m_compressedStack->GetPLSOptimizationState())
            pThread->SetPLSOptimizationState( FALSE );
    }

    OBJECTREF orDelegate = ObjectFromHandle(((DelegateInfo*) args->delegateInfo)->m_delegateHandle);
    OBJECTREF orState = ObjectFromHandle(((DelegateInfo*) args->delegateInfo)->m_stateHandle);


    ARG_SLOT arg[3];

    MethodDesc *pMeth = ((DelegateEEClass*)(orDelegate->GetClass() ))->m_pInvokeMethod;
    _ASSERTE(pMeth);

    // Get the OR on which we are going to invoke the method and set it
    //  as the first parameter in arg above.
    unsigned short argIndex = 0;
    if (!pMeth->IsStatic())
        arg[argIndex++] = ObjToArgSlot(orDelegate);
    arg[argIndex++] = ObjToArgSlot(orState);
    arg[argIndex++] = (ARG_SLOT) args->TimerOrWaitFired;

    // Call the method...

    LogCall(pMeth,"RWSOCallback");

    pMeth->Call(arg);
}

static void RegisterWaitForSingleObjectCallback_Helper(Thread* pThread, AppDomain* appDomain, PVOID delegateInfo, BOOL TimerOrWaitFired)
{
    // dummy COMPLUS_TRY/COMPLUS_FINALLY to force unwind of frames
    COMPLUS_TRY {
        RegisterWaitForSingleObjectCallback_Args args = {(DelegateInfo*) delegateInfo, TimerOrWaitFired};
        if (pThread->GetDomain() != appDomain)
        {
            pThread->DoADCallBack(appDomain->GetDefaultContext(), RegisterWaitForSingleObjectCallback_Worker, &args);
        }
        else
            RegisterWaitForSingleObjectCallback_Worker(&args);
    }                    
    COMPLUS_FINALLY {
    } COMPLUS_END_FINALLY
}

VOID RegisterWaitForSingleObjectCallback(PVOID delegateInfo,  BOOL TimerOrWaitFired)
{
    Thread* pThread = SetupThreadPoolThread(WorkerThread);
    _ASSERTE(pThread != NULL);
    _ASSERTE(pThread == GetThread());

    // This thread should not have any locks held at entry point.
    _ASSERTE(pThread->m_dwLockCount == 0);
    
    _ASSERTE(delegateInfo != NULL);
    _ASSERTE(((DelegateInfo*) delegateInfo)->m_delegateHandle != NULL);

    BEGIN_COOPERATIVE_GC(pThread);

    //
    // NOTE: there is a potential race between the time we retrieve the app domain pointer,
    // and the time which this thread enters the domain.
    // 
    // To solve the race, we rely on the fact that there is a thread sync (via GC)
    // between releasing an app domain's handle, and destroying the app domain.  Thus
    // it is important that we not go into preemptive gc mode in that window.
    // 

    AppDomain* appDomain = SystemDomain::GetAppDomainAtId(((DelegateInfo*) delegateInfo)->m_appDomainId);
    if (appDomain != NULL)
    {
        COMPLUS_TRYEX(pThread)
        {
            PAL_TRY
            {
                PAL_TRY
                {
                    RegisterWaitForSingleObjectCallback_Helper(pThread, appDomain, delegateInfo, TimerOrWaitFired);
                }
                PAL_EXCEPT_FILTER_EX(Label1, ThreadBaseExceptionFilter, (PVOID)ThreadPoolThread)
                {
                    _ASSERTE(!"ThreadBaseExceptionFilter returned EXCEPITON_EXECUTE_HANDLER");
                }
                PAL_ENDTRY
            }
            PAL_FINALLY_EX(Label2)
            {
                pThread->SetDelayedInheritedSecurityStack( NULL );
                pThread->ResetSecurityInfo();
                pThread->SetPLSOptimizationState( TRUE );
            }
            PAL_ENDTRY
        }
        COMPLUS_CATCH
        {
            // quietly swallow the exception
            if (pThread->IsAbortRequested())
                pThread->UserResetAbort();
        }
        COMPLUS_END_CATCH
    }

    END_COOPERATIVE_GC(pThread);


    // We should have released all locks.
    _ASSERTE(g_fEEShutDown || pThread->m_dwLockCount == 0);
}

void ThreadPoolNative::Init()
{

}


FCIMPL8(LPVOID, ThreadPoolNative::CorRegisterWaitForSingleObject, Object* waitObjectUNSAFE, Object* delegateUNSAFE, Object* stateUNSAFE, INT64 timeout, BOOL executeOnlyOnce, Object* registeredWaitObjectUNSAFE, StackCrawlMark* stackMark, BOOL compressStack)
{
    HANDLE handle;
    struct _gc
    {
        OBJECTREF waitObject;
        OBJECTREF delegate;
        OBJECTREF state;
        OBJECTREF registeredWaitObject;
    } gc;
    gc.waitObject = (OBJECTREF) waitObjectUNSAFE;
    gc.delegate = (OBJECTREF) delegateUNSAFE;
    gc.state = (OBJECTREF) stateUNSAFE;
    gc.registeredWaitObject = (OBJECTREF) registeredWaitObjectUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_NOPOLL();
    GCPROTECT_BEGIN(gc);
    HELPER_METHOD_POLL();
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();

    if(gc.waitObject == NULL || gc.delegate == NULL)
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_Obj");

    _ASSERTE(gc.registeredWaitObject != NULL);

    ULONG flag = executeOnlyOnce ? WAIT_SINGLE_EXECUTION | WAIT_FREE_CONTEXT : WAIT_FREE_CONTEXT;

    HANDLE hWaitHandle = ((WAITHANDLEREF)(OBJECTREFToObject(gc.waitObject)))->GetWaitHandle();
    _ASSERTE(hWaitHandle);

    Thread* pCurThread = GetThread();
    _ASSERTE( pCurThread);

    AppDomain* appDomain = pCurThread->GetDomain();
    _ASSERTE(appDomain);

    DelegateInfo* delegateInfo = DelegateInfo::MakeDelegateInfo(appDomain,
                                                                gc.delegate,                                                                
                                                                gc.state, 
                                                                gc.waitObject,
                                                                gc.registeredWaitObject);

    if (Security::IsSecurityOn() && compressStack)
    {
        delegateInfo->SetThreadSecurityInfo( GetThread(), stackMark );
    }



    if (!(ThreadpoolMgr::RegisterWaitForSingleObject(&handle,
                                          hWaitHandle,
                                          (WAITORTIMERCALLBACK) RegisterWaitForSingleObjectCallback,
                                          (PVOID) delegateInfo,
                                          (ULONG) timeout,
                                          flag)))
   
    {
        _ASSERTE(GetLastError() != ERROR_CALL_NOT_IMPLEMENTED);
        delegateInfo->Release();
        ThreadpoolMgr::RecycleMemory((LPVOID*)delegateInfo, ThreadpoolMgr::MEMTYPE_DelegateInfo);

        COMPlusThrowWin32();
    }

    //-[autocvtepi]-------------------------------------------------------
    GCPROTECT_END();
    HELPER_METHOD_FRAME_END();
    return (LPVOID) handle;
}
FCIMPLEND


/********************************************************************************************************************/

static void QueueUserWorkItemCallback_Worker(PVOID delegateInfo)
{
    Thread *pThread = GetThread();

    // This thread should not have any locks held at entry point.
    _ASSERTE(pThread->m_dwLockCount == 0);

    if (((DelegateInfo*)delegateInfo)->m_hasSecurityInfo)
    {
        _ASSERTE( Security::IsSecurityOn() && "This block should only be called if security is on" );
        pThread->SetDelayedInheritedSecurityStack( ((DelegateInfo*)delegateInfo)->m_compressedStack );
        pThread->CarryOverSecurityInfo( ((DelegateInfo*) delegateInfo)->m_compressedStack->GetOverridesCount(), ((DelegateInfo*) delegateInfo)->m_compressedStack->GetAppDomainStack() );
        if (!((DelegateInfo*) delegateInfo)->m_compressedStack->GetPLSOptimizationState())
            pThread->SetPLSOptimizationState( FALSE );
    }

    OBJECTREF orDelegate = ObjectFromHandle(((DelegateInfo*) delegateInfo)->m_delegateHandle);
    OBJECTREF orState = ObjectFromHandle(((DelegateInfo*) delegateInfo)->m_stateHandle);

    ((DelegateInfo*)delegateInfo)->Release();
    ThreadpoolMgr::RecycleMemory((LPVOID*)delegateInfo, ThreadpoolMgr::MEMTYPE_DelegateInfo);

    ARG_SLOT arg[2];

    MethodDesc *pMeth = ((DelegateEEClass*)(orDelegate->GetClass() ))->m_pInvokeMethod;
    _ASSERTE(pMeth);

    // Get the OR on which we are going to invoke the method and set it
    //  as the first parameter in arg above.
    if (pMeth->IsStatic())
    {
        arg[0] = ObjToArgSlot(orState);
    }
    else
    {
        arg[0] = ObjToArgSlot(orDelegate);
        arg[1] = ObjToArgSlot(orState);
    }

    // Call the method...
    LogCall(pMeth,"QUWICallback");

    pMeth->Call(arg);
}

static void QueueUserWorkItemCallback_Helper(Thread* pThread, AppDomain* appDomain, PVOID delegateInfo)
{
    // dummy COMPLUS_TRY/COMPLUS_FINALLY to force unwind of frames
    COMPLUS_TRY {
        if (appDomain != pThread->GetDomain())
        {
            pThread->DoADCallBack(appDomain->GetDefaultContext(), QueueUserWorkItemCallback_Worker, delegateInfo);                
        }
        else
            QueueUserWorkItemCallback_Worker(delegateInfo);
    }                    
    COMPLUS_FINALLY {
    } COMPLUS_END_FINALLY
}

DWORD WINAPI  QueueUserWorkItemCallback(PVOID delegateInfo)
{
    Thread* pThread = SetupThreadPoolThread(WorkerThread);
    _ASSERTE(pThread != NULL);
    _ASSERTE(pThread == GetThread());
    _ASSERTE(delegateInfo != NULL);

    BEGIN_COOPERATIVE_GC(pThread);
            
    //
    // NOTE: there is a potential race between the time we retrieve the app domain pointer,
    // and the time which this thread enters the domain.
    // 
    // To solve the race, we rely on the fact that there is a thread sync (via GC)
    // between releasing an app domain's handle, and destroying the app domain.  Thus
    // it is important that we not go into preemptive gc mode in that window.
    // 

    AppDomain* appDomain = SystemDomain::GetAppDomainAtId(((DelegateInfo*) delegateInfo)->m_appDomainId);
    if (appDomain != NULL)
    {
        COMPLUS_TRYEX(pThread)
        {
            PAL_TRY
            {
                PAL_TRY
                {
                    QueueUserWorkItemCallback_Helper(pThread, appDomain, delegateInfo);
                } 
                PAL_EXCEPT_FILTER_EX(Label1, ThreadBaseExceptionFilter, (PVOID)ThreadPoolThread)
                {
                    _ASSERTE(!"ThreadBaseExceptionFilter returned EXCEPTION_EXECUTE_HANDLER");
                }
                PAL_ENDTRY
            }
            PAL_FINALLY_EX(Label2)
            {
                pThread->SetDelayedInheritedSecurityStack( NULL );
                pThread->ResetSecurityInfo();
                pThread->SetPLSOptimizationState( TRUE );
            }
            PAL_ENDTRY
        }
        COMPLUS_CATCH
        {
                // quietly swallow the exception
            if (pThread->IsAbortRequested())
                pThread->UserResetAbort();
        }
        COMPLUS_END_CATCH

    }    

    END_COOPERATIVE_GC(pThread);

    
    // We should have released all locks.
    _ASSERTE(g_fEEShutDown || pThread->m_dwLockCount == 0);

    return ERROR_SUCCESS;
}


FCIMPL4(VOID, ThreadPoolNative::CorQueueUserWorkItem, Object* delegateUNSAFE, Object* stateUNSAFE, StackCrawlMark* stackMark, BYTE compressStack)
{
    OBJECTREF delegate = (OBJECTREF) delegateUNSAFE;
    OBJECTREF state = (OBJECTREF) stateUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_2(delegate, state);
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();

    if (delegate == NULL)
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_Obj");

    Thread* pCurThread = GetThread();
    _ASSERTE( pCurThread);

    AppDomain* appDomain = pCurThread->GetDomain();
    _ASSERTE(appDomain);

    DelegateInfo* delegateInfo = DelegateInfo::MakeDelegateInfo(appDomain,
                                                                delegate,                                                                
                                                                state,
                                                                NULL,
                                                                NULL);

    if (Security::IsSecurityOn() && compressStack)
    {
        delegateInfo->SetThreadSecurityInfo( GetThread(), stackMark );
    }

    BOOL res = ThreadpoolMgr::QueueUserWorkItem(QueueUserWorkItemCallback,
                                      (PVOID) delegateInfo,
                                                0);
    if (!res)
    {
        if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
            COMPlusThrow(kNotSupportedException);
        else
            COMPlusThrowWin32();
    }

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND


/********************************************************************************************************************/

FCIMPL2(BOOL, ThreadPoolNative::CorUnregisterWait, LPVOID WaitHandle, LPVOID objectToNotify)
{
    BOOL retVal;
    HELPER_METHOD_FRAME_BEGIN_RET_0();
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();

    HANDLE hWait = (HANDLE) WaitHandle;  
    HANDLE hObjectToNotify = (HANDLE) objectToNotify;

    retVal = ThreadpoolMgr::UnregisterWaitEx(hWait, hObjectToNotify);


    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return retVal;
}
FCIMPLEND

/********************************************************************************************************************/
FCIMPL1(void, ThreadPoolNative::CorWaitHandleCleanupNative, LPVOID WaitHandle)
{
    HELPER_METHOD_FRAME_BEGIN_0();
    //-[autocvtpro]-------------------------------------------------------

    HANDLE hWait = (HANDLE)WaitHandle;
    ThreadpoolMgr::WaitHandleCleanup(hWait);

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND



/******************************************************************************************/
/*                                                                                        */
/*                              Timer Functions                                           */
/*                                                                                        */
/******************************************************************************************/
struct AddTimerCallback_Args
{
    PVOID delegateInfo;
    BOOL TimerOrWaitFired;
    BOOL setStack;
};

VOID WINAPI AddTimerCallbackEx(PVOID delegateInfo, BOOL TimerOrWaitFired, BOOL setStack);

void AddTimerCallback_Wrapper(LPVOID ptr)
{
    AddTimerCallback_Args *args = (AddTimerCallback_Args *) ptr;
    AddTimerCallbackEx(args->delegateInfo, args->TimerOrWaitFired, args->setStack );
}

VOID WINAPI AddTimerCallback(PVOID delegateInfo, BOOL TimerOrWaitFired)
{
    AddTimerCallbackEx( delegateInfo, TimerOrWaitFired, TRUE );
}

VOID WINAPI AddTimerCallbackEx(PVOID delegateInfo, BOOL TimerOrWaitFired, BOOL setStack)
{
    Thread* pThread = SetupThreadPoolThread(WorkerThread);
    _ASSERTE(pThread != NULL);
    _ASSERTE(pThread == GetThread());

    // This thread should not have any locks held at entry point.
    _ASSERTE(pThread->m_dwLockCount == 0);
    
    _ASSERTE(delegateInfo != NULL);
    _ASSERTE(((DelegateInfo*) delegateInfo)->m_delegateHandle != NULL);

    BEGIN_ENSURE_COOPERATIVE_GC(); 

    // NOTE: there is a potential race between the time we retrieve the app domain pointer,
    // and the time which this thread enters the domain.
    // 
    // To solve the race, we rely on the fact that there is a thread sync (via GC)
    // between releasing an app domain's handle, and destroying the app domain.  Thus
    // it is important that we not go into preemptive gc mode in that window.
    // 
    AppDomain *appDomain = SystemDomain::GetAppDomainAtId(((DelegateInfo*) delegateInfo)->m_appDomainId);
    if (appDomain != NULL)
    {
        COMPLUS_TRYEX(pThread)
        {
            PAL_TRY
            {
                PAL_TRY
                {

                    if (setStack && ((DelegateInfo*)delegateInfo)->m_hasSecurityInfo)
                    {
                        _ASSERTE( Security::IsSecurityOn() && "This block should only be called if security is on" );
                        pThread->SetDelayedInheritedSecurityStack( ((DelegateInfo*)delegateInfo)->m_compressedStack );
                        pThread->CarryOverSecurityInfo( ((DelegateInfo*) delegateInfo)->m_compressedStack->GetOverridesCount(), ((DelegateInfo*) delegateInfo)->m_compressedStack->GetAppDomainStack() );
                        if (!((DelegateInfo*) delegateInfo)->m_compressedStack->GetPLSOptimizationState())
                            pThread->SetPLSOptimizationState( FALSE );
                    }

                    if (appDomain != pThread->GetDomain())
                    {
                        AddTimerCallback_Args args = {delegateInfo, TimerOrWaitFired, FALSE};
                        pThread->DoADCallBack(appDomain->GetDefaultContext(), AddTimerCallback_Wrapper, &args);
                    }
                    else
                    {
                        OBJECTREF orDelegate = ObjectFromHandle(((DelegateInfo*) delegateInfo)->m_delegateHandle);
                        OBJECTREF orState = ObjectFromHandle(((DelegateInfo*) delegateInfo)->m_stateHandle);

                        ARG_SLOT arg[2];


                        MethodDesc *pMeth = ((DelegateEEClass*)( orDelegate->GetClass() ))->m_pInvokeMethod;
                        _ASSERTE(pMeth);
                        arg[0] = ObjToArgSlot(orDelegate);
                        arg[1] = ObjToArgSlot(orState);

                        // Call the method...
                        LogCall(pMeth,"TimerCallback");

                        pMeth->Call(arg);
                    }
                }
                PAL_EXCEPT_FILTER_EX(Label1, ThreadBaseExceptionFilter, (PVOID)ThreadPoolThread)
                {
                    _ASSERTE(!"ThreadBaseExceptionFilter returned EXCEPTION_EXECUTE_HANDLER");
                }
                PAL_ENDTRY
            }
            PAL_FINALLY_EX(Label2)
            {
                if (setStack)
                {
                    pThread->SetDelayedInheritedSecurityStack( NULL );
                    pThread->ResetSecurityInfo();
                    pThread->SetPLSOptimizationState( TRUE );
                }
            }
            PAL_ENDTRY
        }
        COMPLUS_CATCH
        {
            // quietly swallow the exception
            if (pThread->IsAbortRequested())
                pThread->UserResetAbort();
        }
        COMPLUS_END_CATCH
    }
    END_ENSURE_COOPERATIVE_GC(); 

    // We should have released all locks.
    _ASSERTE(g_fEEShutDown || pThread->m_dwLockCount == 0);

}

FCIMPL6(VOID, TimerNative::CorCreateTimer, TimerBaseNative* pThisUNSAFE, Object* delegateUNSAFE, Object* stateUNSAFE, INT32 dueTime, INT32 period, StackCrawlMark* stackMark)
{
    struct _gc
    {
        TIMERREF pThis;
        OBJECTREF delegate;
        OBJECTREF state;
    } gc;
    gc.pThis = (TIMERREF) pThisUNSAFE;
    gc.delegate = (OBJECTREF) delegateUNSAFE;
    gc.state = (OBJECTREF) stateUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_NOPOLL();
    GCPROTECT_BEGIN(gc);
    HELPER_METHOD_POLL();
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();

    Thread* pCurThread = GetThread();
    _ASSERTE( pCurThread);

    AppDomain* appDomain = pCurThread->GetDomain();
    _ASSERTE(appDomain);

    DelegateInfo* delegateInfo = DelegateInfo::MakeDelegateInfo(appDomain,
                                                                gc.delegate,                                                                
                                                                gc.state,
                                                                NULL,
                                                                NULL);
    
    if (Security::IsSecurityOn())
    {
        delegateInfo->SetThreadSecurityInfo( GetThread(), stackMark );
    }
   
    HANDLE hNewTimer;
    BOOL res = ThreadpoolMgr::CreateTimerQueueTimer(&hNewTimer,
                                     (WAITORTIMERCALLBACK) AddTimerCallback ,
                                     (PVOID) delegateInfo,
                                     (ULONG) dueTime,
                                     (ULONG) period,
                                     0
                                     );
    if (!res)
    {
        delegateInfo->Release();
        ThreadpoolMgr::RecycleMemory((LPVOID*)delegateInfo, ThreadpoolMgr::MEMTYPE_DelegateInfo);

        if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
            COMPlusThrow(kNotSupportedException);
        else
            COMPlusThrowWin32();
    }
    gc.pThis->SetDelegateInfo(delegateInfo);
    gc.pThis->SetTimerHandle(hNewTimer);
              
    //-[autocvtepi]-------------------------------------------------------
    GCPROTECT_END();
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

/******************************************************************************************/

struct TimerDeleteInfo
{
    DelegateInfo*  delegateInfo;
    HANDLE         waitObjectHandle;		// handle of the registered wait that needs to be deleted
    HANDLE         notifyHandle;
    HANDLE         surrogateEvent;

    TimerDeleteInfo(DelegateInfo* dI, HANDLE nh, HANDLE se)
    {
        delegateInfo = dI;
        waitObjectHandle = NULL;
        notifyHandle = nh;
        surrogateEvent = se;
    }

    ~TimerDeleteInfo()
    {
        CloseHandle(surrogateEvent);

        if (delegateInfo != NULL)
        {
            delegateInfo->Release();
            ThreadpoolMgr::RecycleMemory((LPVOID*)delegateInfo, ThreadpoolMgr::MEMTYPE_DelegateInfo);
        }
        ThreadpoolMgr::UnregisterWaitEx(waitObjectHandle,NULL);
    }
}; 

VOID WINAPI TimerNative::timerDeleteWorkItem(PVOID parameters, BOOL ignored /* since this is wait infinite*/)
{
    TimerDeleteInfo* timerDeleteInfo = (TimerDeleteInfo*) parameters;

    if (timerDeleteInfo->notifyHandle != NULL)
        SetEvent((HANDLE)timerDeleteInfo->notifyHandle);

    delete timerDeleteInfo;
}

FCIMPL2(BOOL, TimerNative::CorDeleteTimer, TimerBaseNative* pThisUNSAFE, LPVOID notifyObjectHandle)
{
    BOOL retVal = FALSE;
    TIMERREF pThis = (TIMERREF) pThisUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_1(pThis);
    //-[autocvtpro]-------------------------------------------------------


    THROWSCOMPLUSEXCEPTION();
    if (pThis == NULL)
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    LONG deleted = 0;
    BOOL res1,res2;
    DWORD errorCode = 0;
	HANDLE ev = NULL;
	Thread *pThread = NULL;
    BOOL bToggleGC = FALSE;

	HANDLE timerHandle = pThis->GetTimerHandle();
    if (timerHandle == NULL)        // this can happen if an exception is thrown in the timer constructor
    {                               // and the finalizer thread calls this through dispose
        goto lExit;
    }

    deleted = InterlockedExchange(pThis->GetAddressTimerDeleted(),TRUE);
    if (deleted)   // someone beat us to it
    {
        goto lExit;   // an application error, so return false.
    }

    ev = WszCreateEvent(NULL, // security attributes
                               TRUE, // manual event
                               FALSE, // initial state is not signalled
                               NULL); // no name
    _ASSERTE(ev);
    pThread = GetThread();

	if (pThread)
        bToggleGC = pThread->PreemptiveGCDisabled ();
    if (bToggleGC)
        pThread->EnablePreemptiveGC ();

    res1 = ThreadpoolMgr::DeleteTimerQueueTimer(timerHandle, ev);

    if (!res1)
        errorCode = ::GetLastError();   // capture the error code so we can throw the right exception

    if (bToggleGC)
        pThread->DisablePreemptiveGC ();

    // NOTE: We are assuming that the error code is benign and the timer is still going to get deleted...
    TimerDeleteInfo* timerDeleteInfo;
    timerDeleteInfo = new TimerDeleteInfo(pThis->GetDelegateInfo(),
                                          notifyObjectHandle,
                                          ev);
    _ASSERTE(timerDeleteInfo != NULL);

    res2 = ThreadpoolMgr::RegisterWaitForSingleObject(&(timerDeleteInfo->waitObjectHandle),
                                                      ev,
                                                      (WAITORTIMERCALLBACK)timerDeleteWorkItem,
                                                      (LPVOID) timerDeleteInfo,
                                                      INFINITE,
                                                      WAIT_SINGLE_EXECUTION);

    // .... however, we are still reporting the failure as an exception except for ERROR_IO_PENDING 
    if (!res1 && errorCode != ERROR_IO_PENDING)
    {
        ::SetLastError(errorCode);
        COMPlusThrowWin32();
    }
    else if (!res2)
    {
        COMPlusThrowWin32();
    }

    retVal = TRUE;

lExit: ;
    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return retVal;
}
FCIMPLEND



/******************************************************************************************/

FCIMPL3(BOOL, TimerNative::CorChangeTimer, TimerBaseNative* pThisUNSAFE, INT32 dueTime, INT32 period)
{
    BOOL retVal = FALSE;
    TIMERREF pThis = (TIMERREF) pThisUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_1(pThis);
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();

    BOOL status = FALSE;

    if (pThis->IsTimerDeleted())
    {
        goto lExit;
    }
    status = ThreadpoolMgr::ChangeTimerQueueTimer(
                                            pThis->GetTimerHandle(),
                                            (ULONG)dueTime,
                                            (ULONG)period);

    if (!status)
    {
        COMPlusThrowWin32();
    }

    retVal = TRUE;

lExit: ;
    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return retVal;
}
FCIMPLEND





