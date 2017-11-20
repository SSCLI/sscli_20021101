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
** Header: COMSynchronizable.cpp
**
**                                             
**
** Purpose: Native methods on System.SynchronizableObject
**          and its subclasses.
**
** Date:  April 1, 1998
** 
===========================================================*/

#include "common.h"

#include <object.h>
#include "threads.h"
#include "excep.h"
#include "vars.hpp"
#include "field.h"
#include "security.h"
#include "comsynchronizable.h"
#include "dbginterface.h"
#include "comdelegate.h"
#include "reflectwrap.h"
#include "remoting.h"
#include "eeconfig.h"
#include "threads.inl"

MethodTable* ThreadNative::m_MT = NULL;
MethodDesc* ThreadNative::m_SetPrincipalMethod;


// The two threads need to communicate some information.  Any object references must
// be declared to GC.
struct SharedState
{
    OBJECTHANDLE    m_Threadable;
    Thread         *m_Internal;
    OBJECTHANDLE    m_Principal;

    SharedState(OBJECTREF threadable, Thread *internal, OBJECTREF principal)
    {
        m_Threadable = internal->GetKickOffDomain()->CreateHandle(NULL);
        StoreObjectInHandle(m_Threadable, threadable);

        m_Internal = internal;

        m_Principal = internal->GetKickOffDomain()->CreateHandle(NULL);
        StoreObjectInHandle(m_Principal, principal);
    }

    ~SharedState()
    {
        DestroyHandle(m_Threadable);
        DestroyHandle(m_Principal);
    }

    static SharedState  *MakeSharedState(OBJECTREF threadable, Thread *internal, OBJECTREF principal);
};


// For the following helpers, we make no attempt to synchronize.  The app developer
// is responsible for managing his own race conditions.
//
// Note: if the internal Thread is NULL, this implies that the exposed object has
//       finalized and then been resurrected.
static inline BOOL ThreadNotStarted(Thread *t)
{
    return (t && t->IsUnstarted() && (t->GetThreadHandle() == INVALID_HANDLE_VALUE));
}

static inline BOOL ThreadIsRunning(Thread *t)
{
    return (t && (t->GetThreadHandle() != INVALID_HANDLE_VALUE));
}

static inline BOOL ThreadIsDead(Thread *t)
{
    return (t == 0 || t->IsDead());
}


// Map our exposed notion of thread priorities into the enumeration that NT uses.
static INT32 MapToNTPriority(INT32 ours)
{
    THROWSCOMPLUSEXCEPTION();

    INT32   NTPriority = 0;

    switch (ours)
    {
    case ThreadNative::PRIORITY_LOWEST:
        NTPriority = THREAD_PRIORITY_LOWEST;
        break;

    case ThreadNative::PRIORITY_BELOW_NORMAL:
        NTPriority = THREAD_PRIORITY_BELOW_NORMAL;
        break;

    case ThreadNative::PRIORITY_NORMAL:
        NTPriority = THREAD_PRIORITY_NORMAL;
        break;

    case ThreadNative::PRIORITY_ABOVE_NORMAL:
        NTPriority = THREAD_PRIORITY_ABOVE_NORMAL;
        break;

    case ThreadNative::PRIORITY_HIGHEST:
        NTPriority = THREAD_PRIORITY_HIGHEST;
        break;

    default:
        COMPlusThrow(kArgumentOutOfRangeException, L"Argument_InvalidFlag");
    }
    return NTPriority;
}


// Retrieve the handle from the internal thread.
HANDLE ThreadBaseObject::GetHandle()
{
    Thread  *thread = GetInternal();

    return (thread
            ? thread->GetThreadHandle()
            : INVALID_HANDLE_VALUE);
}

void ThreadNative::KickOffThread_Worker(LPVOID ptr)
{
    KickOffThread_Args *args = (KickOffThread_Args *) ptr;
    args->retVal = 0;

    // we are saving the delagate and result primarily for debugging
    struct _gc {
        OBJECTREF orPrincipal;
        OBJECTREF orDelegate;
        OBJECTREF orResult;
    } gc;
    ZeroMemory(&gc, sizeof(gc));

    GCPROTECT_BEGIN(gc);
    gc.orPrincipal = ObjectFromHandle(args->share->m_Principal);
    // Push the initial security principal object (if any) onto the
    // managed thread.
    if (gc.orPrincipal != NULL) {
        ARG_SLOT argsToSetPrincipal[2];
        argsToSetPrincipal[0] = ObjToArgSlot(args->pThread->GetExposedObject());
        argsToSetPrincipal[1] = ObjToArgSlot(gc.orPrincipal);
        m_SetPrincipalMethod->Call(argsToSetPrincipal, METHOD__THREAD__SET_PRINCIPAL_INTERNAL);
    }

    gc.orDelegate = ObjectFromHandle(args->share->m_Threadable);

    // We cannot call the Delegate Invoke method directly from ECall.  The
    //  stub has not been created for non multicast delegates.  Instead, we
    //  will invoke the Method on the OR stored in the delegate directly.
    // If there are changes to the signature of the ThreadStart delegate
    //  this code will need to change.  I've noted this in the Thread start
    //  class.
    ARG_SLOT arg[1];

    delete args->share;
    args->share = 0;

    MethodDesc *pMeth = ((DelegateEEClass*)( gc.orDelegate->GetClass() ))->m_pInvokeMethod;
    _ASSERTE(pMeth);
    arg[0] = ObjToArgSlot(gc.orDelegate);
    pMeth->Call(arg);
    GCPROTECT_END();
}

// Helper to avoid two COMPLUS_TRY/COMPLUS_CATCH blocks in one function
static void PulseAllHelper(Thread* pThread)
{
    COMPLUS_TRYEX(pThread)
    {
        _ASSERTE(pThread->PreemptiveGCDisabled());

        // GetExposedObject() will either throw, or we have a valid object.  Note
        // that we re-acquire it each time, since it may move during calls.
        pThread->GetExposedObject()->EnterObjMonitor();
        pThread->GetExposedObject()->PulseAll();
        pThread->GetExposedObject()->LeaveObjMonitor();
    }
    COMPLUS_CATCH
    {
        _ASSERTE(FALSE);
        // just keep going...
    }
    COMPLUS_END_CATCH
}

// When an exposed thread is started by Win32, this is where it starts.
ULONG __stdcall ThreadNative::KickOffThread(void *pass)
{
    ULONG retVal = 0;
    // Before we do anything else, get Setup so that we have a real thread.

    KickOffThread_Args args;
    // don't have a separate var becuase this can be updated in the worker
    args.share = (SharedState *) pass;
    Thread      *thread = args.share->m_Internal;
    args.pThread = thread;

    _ASSERTE(thread != NULL);

    // We have a sticky problem here.
    //
    // Under some circumstances, the context of 'this' doesn't match the context
    // of the thread.  Today this can only happen if the thread is marked for an
    // STA.  If so, the delegate that is stored in the object may not be directly
    // suitable for invocation.  Instead, we need to call through a proxy so that
    // the correct context transitions occur.
    //
    // All the changes occur inside HasStarted(), which will switch this thread
    // over to a brand new STA as necessary.  We have to notice this happening, so
    // we can adjust the delegate we are going to invoke on.

    BOOL         ok = thread->HasStarted();

    _ASSERTE(GetThread() == thread);        // Now that it's started
    _ASSERTE(ObjectFromHandle(args.share->m_Threadable) != NULL);

    // Note that we aren't reporting errors if the thread fails to start up properly.
    // The best we can do is quietly clean up our mess.  But the most likely reason
    // for (!ok) is that someone called Thread.Abort() on us before we got a call to
    // Thread.Start().
    if (ok)
    {
        COMPLUS_TRYEX(thread)
        {
			PAL_TRY
			{
				AppDomain *pKickOffDomain = thread->GetKickOffDomain();
				// should always have a kickoff domain - a thread should never start in a domain that is unloaded
				// because otherwise it would have been collected because nobody can hold a reference to thread object
				// in a domain that has been unloaded. But it is possible that we started the unload, in which 
				// case this thread wouldn't be allowed in or would be punted anyway.
				if (! pKickOffDomain)
					// this doesn't actually do much, since it will be caught below, but it might leave a trace
					// as to why the thread didn't start
					COMPlusThrow(kAppDomainUnloadedException);
				if (pKickOffDomain != thread->GetDomain())
				{
					thread->DoADCallBack(pKickOffDomain->GetDefaultContext(), ThreadNative::KickOffThread_Worker, &args);
					retVal = args.retVal;
				}
				else
				{
					KickOffThread_Worker(&args);
					retVal = args.retVal;
				}
			}
			PAL_EXCEPT_FILTER(ThreadBaseExceptionFilter, (PVOID)ManagedThread)
			{                        
				_ASSERTE(!"ThreadBaseExceptionFilter returned EXECUTE_HANDLER.");
			}
			PAL_ENDTRY
        }
        COMPLUS_CATCH
        {
            LOG((LF_EH, LL_INFO100, "ThreadNative::KickOffThread caught exception\n"));
            // fall through to thread destruction...
        }
        COMPLUS_END_CATCH
    }

    thread->ResetStopRequest();     // pointless, now

    PulseAllHelper(thread);

    if (args.share)
        delete args.share;

    thread->EnablePreemptiveGC();
    DestroyThread(thread);

    return retVal;
}


FCIMPL3(void, ThreadNative::Start, ThreadBaseObject* pThisUNSAFE, Object* pPrincipalUNSAFE, StackCrawlMark* pStackMark)
{
    THROWSCOMPLUSEXCEPTION();

    HELPER_METHOD_FRAME_BEGIN_NOPOLL();

    StartInner(pThisUNSAFE, pPrincipalUNSAFE, pStackMark);

    HELPER_METHOD_FRAME_END_POLL();
}
FCIMPLEND

// Start up a thread, which by now should be in the ThreadStore's Unstarted list.
void ThreadNative::StartInner(ThreadBaseObject* pThisUNSAFE, Object* pPrincipalUNSAFE, StackCrawlMark* pStackMark)
{
    THROWSCOMPLUSEXCEPTION();

    struct _gc
    {
        OBJECTREF       pPrincipal;
        THREADBASEREF   pThis;
    } gc;

    gc.pPrincipal  = (OBJECTREF) pPrincipalUNSAFE;
    gc.pThis       = (THREADBASEREF) pThisUNSAFE;

    GCPROTECT_BEGIN(gc);

    if (gc.pThis == NULL)
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    Thread        *pCurThread = GetThread();
    Thread        *pNewThread = gc.pThis->GetInternal();

    SharedState     *share = NULL;

    EE_TRY_FOR_FINALLY
    {
        HANDLE        h;
        DWORD         newThreadId;

        _ASSERTE(pCurThread != NULL);           // Current thread wandered in!

        gc.pThis->EnterObjMonitor();
    
        // Is the thread already started?  You can't restart a thread.
        if (ThreadIsRunning(pNewThread) || ThreadIsDead(pNewThread))
            COMPlusThrow(kThreadStateException, IDS_EE_THREADSTART_STATE);

        // Carry over the state used by security to the new thread
        pNewThread->CarryOverSecurityInfo(pCurThread);

        // Generate code-access security stack to carry over to thread.

		CompressedStack* pCodeAccessStack = Security::GetDelayedCompressedStack();

		_ASSERTE (pCodeAccessStack != NULL || Security::IsSecurityOff());

		// Compressed stack might be null if security is off.
		if (pCodeAccessStack != NULL)
		{
			// Add permission stack object to thread.
			pNewThread->SetDelayedInheritedSecurityStack(pCodeAccessStack);

			// We need to release the compressed stack now since setting it will increment it
			pCodeAccessStack->Release();
		}

        OBJECTREF   threadable = gc.pThis->GetDelegate();
        gc.pThis->SetDelegate(NULL);

        // This can never happen, because we construct it with a valid one and then
        // we never let you change it (because SetStart is private).
        _ASSERTE(threadable != NULL);

        // Allocate this away from our stack, so we can unwind without affecting
        // KickOffThread.  It is inside a GCFrame, so we can enable GC now.
        share = SharedState::MakeSharedState(threadable, pNewThread, gc.pPrincipal);
        if (share == NULL)
            COMPlusThrowOM();

        // As soon as we create the new thread, it is eligible for suspension, etc.
        // So it gets transitioned to cooperative mode before this call returns to
        // us.  It is our duty to start it running immediately, so that GC isn't blocked.

        h = pNewThread->CreateNewThread(0 /*stackSize override*/,
                                        KickOffThread, share, &newThreadId);

        _ASSERTE(h != NULL);        // CreateNewThread returns INVALID_HANDLE_VALUE for failure
        if (h == INVALID_HANDLE_VALUE)
            COMPlusThrowOM();

        _ASSERTE(pNewThread->GetThreadHandle() == h);

        // After we have established the thread handle, we can check m_Priority.
        // This ordering is required to eliminate the race condition on setting the
        // priority of a thread just as it starts up.
        ::SetThreadPriority(h, MapToNTPriority(gc.pThis->m_Priority));

        // Before we do the resume, we need to take note of the new ThreadId.  This
        // is necessary because -- before the thread starts executing at KickofThread --
        // it may perform some DllMain DLL_THREAD_ATTACH notifications.  These could
        // call into managed code.  During the consequent SetupThread, we need to
        // perform the Thread::HasStarted call instead of going through the normal
        // 'new thread' pathway.
        _ASSERTE(pNewThread->GetThreadId() == 0);
        _ASSERTE(newThreadId != 0);

        pNewThread->SetThreadId(newThreadId);

        share = NULL;       // we have handed off ownership of the shared struct



        FastInterlockOr((ULONG *) &pNewThread->m_State, Thread::TS_LegalToJoin);

#ifdef _DEBUG
        DWORD   ret =
#endif
        ::ResumeThread(h);


        _ASSERTE(ret == 1);
    }
    EE_FINALLY
    {
        if (share != NULL)
            delete share;
        gc.pThis->LeaveObjMonitor();
    } EE_END_FINALLY;

    GCPROTECT_END();
}

FCIMPL1(void, ThreadNative::Abort, ThreadBaseObject* pThis)
{
    THROWSCOMPLUSEXCEPTION();
	if (pThis == NULL)
        FCThrowVoid(kNullReferenceException);

    THREADBASEREF thisRef(pThis);
    HELPER_METHOD_FRAME_BEGIN_NOPOLL();
    Thread *thread = thisRef->GetInternal();
    if (thread == NULL)
        COMPlusThrow(kThreadStateException, IDS_EE_THREAD_CANNOT_GET);
    thread->UserAbort(thisRef);

    HELPER_METHOD_FRAME_END_POLL();
}
FCIMPLEND

FCIMPL1(void, ThreadNative::ResetAbort, ThreadBaseObject* pThis)
{
	_ASSERTE(pThis);
	VALIDATEOBJECTREF(pThis);
    THROWSCOMPLUSEXCEPTION();
    Thread *thread = pThis->GetInternal();
    HELPER_METHOD_FRAME_BEGIN_NOPOLL();
    if (thread == NULL)
        COMPlusThrow(kThreadStateException, IDS_EE_THREAD_CANNOT_GET);
    thread->UserResetAbort();
    HELPER_METHOD_FRAME_END_POLL();
}
FCIMPLEND

// You can only suspend a running thread.
FCIMPL1(void, ThreadNative::Suspend, ThreadBaseObject* pThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();
    if (pThisUNSAFE == NULL)
        FCThrowResVoid(kNullReferenceException, L"NullReference_This");

    Thread  *thread = pThisUNSAFE->GetInternal();

    HELPER_METHOD_FRAME_BEGIN_0();

    if (!ThreadIsRunning(thread))
        COMPlusThrow(kThreadStateException, IDS_EE_THREAD_SUSPEND_NON_RUNNING);

    thread->UserSuspendThread();

    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

// You can only resume a thread that is in the user-suspended state.  (This puts a large
// burden on the app developer, but we want him to be thinking carefully about race
// conditions.  Precise errors give him a hope of sorting out his logic).
FCIMPL1(void, ThreadNative::Resume, ThreadBaseObject* pThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();
    if (pThisUNSAFE == NULL)
        FCThrowResVoid(kNullReferenceException, L"NullReference_This");

    Thread  *thread = pThisUNSAFE->GetInternal();

    HELPER_METHOD_FRAME_BEGIN_0();

    // UserResumeThread() will return 0 if there isn't a user suspension for us to
    // clear.
    if (!ThreadIsRunning(thread))
        COMPlusThrow(kThreadStateException, IDS_EE_THREAD_RESUME_NON_RUNNING);
        
    if (thread->UserResumeThread() == 0)
        COMPlusThrow(kThreadStateException, IDS_EE_THREAD_RESUME_NON_USER_SUSPEND);

    HELPER_METHOD_FRAME_END();
}
FCIMPLEND


// Note that you can manipulate the priority of a thread that hasn't started yet,
// or one that is running.  But you get an exception if you manipulate the priority
// of a thread that has died.
FCIMPL1(INT32, ThreadNative::GetPriority, ThreadBaseObject* pThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();
    if (pThisUNSAFE==NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    // validate the handle
    if (ThreadIsDead(pThisUNSAFE->GetInternal()))
        FCThrowEx(kThreadStateException, IDS_EE_THREAD_DEAD_PRIORITY, NULL, NULL, NULL);

    return pThisUNSAFE->m_Priority;
}
FCIMPLEND

FCIMPL2(void, ThreadNative::SetPriority, ThreadBaseObject* pThisUNSAFE, INT32 iPriority)
{
    int     priority;
    Thread *thread;

    THREADBASEREF  pThis = (THREADBASEREF) pThisUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_1(pThis);

    THROWSCOMPLUSEXCEPTION();
    if (pThis==NULL)
    {
        COMPlusThrow(kNullReferenceException, L"NullReference_This");
    }

    // translate the priority (validating as well)
    priority = MapToNTPriority(iPriority);  // can throw; needs a frame
    
    // validate the thread
    thread = pThis->GetInternal();

    if (ThreadIsDead(thread))
    {
        COMPlusThrow(kThreadStateException, IDS_EE_THREAD_DEAD_PRIORITY, NULL, NULL, NULL);
    }

    // Eliminate the race condition by establishing m_Priority before we check for if
    // the thread is running.  See ThreadNative::Start() for the other half.
    pThis->m_Priority = iPriority;

    HANDLE  h = thread->GetThreadHandle();

    if (h != INVALID_HANDLE_VALUE)
        ::SetThreadPriority(h, priority);

    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

// This service can be called on unstarted and dead threads.  For unstarted ones, the
// next wait will be interrupted.  For dead ones, this service quietly does nothing.
FCIMPL1(void, ThreadNative::Interrupt, ThreadBaseObject* pThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();
    if (pThisUNSAFE==NULL)
        FCThrowResVoid(kNullReferenceException, L"NullReference_This");

    Thread  *thread = pThisUNSAFE->GetInternal();

    if (thread == 0)
        FCThrowExVoid(kThreadStateException, IDS_EE_THREAD_CANNOT_GET, NULL, NULL, NULL);

    HELPER_METHOD_FRAME_BEGIN_0();

    thread->UserInterrupt();

    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

FCIMPL1(INT32, ThreadNative::IsAlive, ThreadBaseObject* pThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();
    if (pThisUNSAFE==NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    Thread  *thread = pThisUNSAFE->GetInternal();

    if (thread == 0)
        FCThrowEx(kThreadStateException, IDS_EE_THREAD_CANNOT_GET, NULL, NULL, NULL);

    return ThreadIsRunning(thread);
}
FCIMPLEND

FCIMPL1(void, ThreadNative::Join, ThreadBaseObject* pThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();
    if (pThisUNSAFE == NULL)
        FCThrowResVoid(kNullReferenceException, L"NullReference_This");

    THREADBASEREF pThis = (THREADBASEREF) pThisUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_1(pThis);

    DoJoin(pThis, INFINITE_TIMEOUT);

    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

FCIMPL2(INT32, ThreadNative::JoinTimeout, ThreadBaseObject* pThisUNSAFE, INT32 Timeout)
{
    THROWSCOMPLUSEXCEPTION();
    if (pThisUNSAFE==NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    // validate the timeout
    if ((Timeout < 0) && (Timeout != INFINITE_TIMEOUT))
        FCThrowArgumentOutOfRange(L"millisecondsTimeout", L"ArgumentOutOfRange_NeedNonNegOrNegative1");

    INT32           iRetVal = 0;
    THREADBASEREF   pThis   = (THREADBASEREF) pThisUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_1(pThis);

    iRetVal = DoJoin(pThis, Timeout);

    HELPER_METHOD_FRAME_END();

    return iRetVal;
}
FCIMPLEND

FCIMPL1(void, ThreadNative::Sleep, INT32 iTime)
{
    THROWSCOMPLUSEXCEPTION();

    // validate the sleep time
    if ((iTime < 0) && (iTime != INFINITE_TIMEOUT))
        FCThrowArgumentOutOfRangeVoid(L"millisecondsTimeout", L"ArgumentOutOfRange_NeedNonNegOrNegative1");

    HELPER_METHOD_FRAME_BEGIN_0();

    GetThread()->UserSleep(iTime);

    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

FCIMPL0(Object*, ThreadNative::GetCurrentThread)
{
    THROWSCOMPLUSEXCEPTION();

    Thread      *pCurThread = GetThread();
    OBJECTREF   refRetVal  = NULL;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refRetVal);
    _ASSERTE(pCurThread->PreemptiveGCDisabled());

    refRetVal = pCurThread->GetExposedObject();

    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refRetVal);
}
FCIMPLEND

LPVOID F_CALL_CONV ThreadNative::FastGetCurrentThread()
{
    Thread *pThread;
    OBJECTHANDLE ExposedObject;
   
    pThread = GetThread();
    if (!pThread) {
	return NULL;
    }
    ExposedObject = pThread->m_ExposedObject;
    if (ExposedObject) {
        return *(LPVOID *)ExposedObject;
    }
    return NULL;
}


FCIMPL2(void, ThreadNative::SetStart, ThreadBaseObject* pThisUNSAFE, Object* pDelegateUNSAFE)
    {
    THREADBASEREF   pThis       = (THREADBASEREF) pThisUNSAFE;
    OBJECTREF       pDelegate   = (OBJECTREF    ) pDelegateUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_2(pThis, pDelegate);

    _ASSERTE(pThis != NULL);
    _ASSERTE(pDelegate != NULL); // Thread's constructor validates this

    if (pThis->m_InternalThread == NULL)
    {
        // if we don't have an internal Thread object associated with this exposed object,
        // now is our first opportunity to create one.
        Thread      *unstarted = SetupUnstartedThread();

        pThis->SetInternal(unstarted);
        unstarted->SetExposedObject(pThis);
    }

#ifdef APPDOMAIN_STATE
	// make sure that we have set the kickoff ID correctly if this is the unloadthread worker.
	FieldDesc *pFD = COMDelegate::GetOR();
    OBJECTREF target = NULL;
	GCPROTECT_BEGIN(target);

    pFD->GetInstanceField(pDelegate, &target);
	if (target != NULL && target->GetMethodTable() == g_Mscorlib.GetClass(CLASS__UNLOAD_THREAD_WORKER))
		_ASSERTE_ALL_BUILDS(pThis->m_InternalThread->GetKickOffDomain() == SystemDomain::System()->DefaultDomain());

	GCPROTECT_END();

#endif

    // save off the delegate
    pThis->SetDelegate(pDelegate);

    HELPER_METHOD_FRAME_END();
}
FCIMPLEND


// Set whether or not this is a background thread.
FCIMPL2(void, ThreadNative::SetBackground, ThreadBaseObject* pThisUNSAFE, BYTE isBackground)
{
    THROWSCOMPLUSEXCEPTION();
    if (pThisUNSAFE==NULL)
        FCThrowResVoid(kNullReferenceException, L"NullReference_This");

    // validate the thread
    Thread  *thread = pThisUNSAFE->GetInternal();

    if (ThreadIsDead(thread))
        FCThrowExVoid(kThreadStateException, IDS_EE_THREAD_DEAD_PRIORITY, NULL, NULL, NULL);

    HELPER_METHOD_FRAME_BEGIN_0();

    thread->SetBackground(isBackground);

    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

// Return whether or not this is a background thread.
FCIMPL1(INT32, ThreadNative::IsBackground, ThreadBaseObject* pThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();
    if (pThisUNSAFE==NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    // validate the thread
    Thread  *thread = pThisUNSAFE->GetInternal();

    if (ThreadIsDead(thread))
        FCThrowEx(kThreadStateException, IDS_EE_THREAD_DEAD_PRIORITY, NULL, NULL, NULL);

    // booleanize
    return !!thread->IsBackground();
}
FCIMPLEND


// Deliver the state of the thread as a consistent set of bits.
// This copied in VM\EEDbgInterfaceImpl.h's
//     CorDebugUserState GetUserState( Thread *pThread )
// , so propogate changes to both functions
FCIMPL1(INT32, ThreadNative::GetThreadState, ThreadBaseObject* pThisUNSAFE)
{
    INT32               res = 0;
    Thread::ThreadState state;

    THROWSCOMPLUSEXCEPTION();
    if (pThisUNSAFE==NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    // validate the thread.  Failure here implies that the thread was finalized
    // and then resurrected.
    Thread  *thread = pThisUNSAFE->GetInternal();

    if (!thread)
        FCThrowEx(kThreadStateException, IDS_EE_THREAD_CANNOT_GET, NULL, NULL, NULL);

    HELPER_METHOD_FRAME_BEGIN_RET_0();

    // grab a snapshot
    state = thread->GetSnapshotState();

    if (state & Thread::TS_Background)
        res |= ThreadBackground;

    if (state & Thread::TS_Unstarted)
        res |= ThreadUnstarted;

    // Don't report a StopRequested if the thread has actually stopped.
    if (state & Thread::TS_Dead)
    {
        if (state & Thread::TS_AbortRequested)
            res |= ThreadAborted;
        else
            res |= ThreadStopped;
    }
    else
    {
        if (state & Thread::TS_AbortRequested)
            res |= ThreadAbortRequested;
        else
        if (state & Thread::TS_UserStopRequested)
            res |= ThreadStopRequested;
    }

    if (state & Thread::TS_Interruptible)
        res |= ThreadWaitSleepJoin;

    // Don't report a SuspendRequested if the thread has actually Suspended.
    if ((state & Thread::TS_UserSuspendPending) &&
        (state & Thread::TS_SyncSuspended)
       )
    {
        res |= ThreadSuspended;
    }
    else
    if (state & Thread::TS_UserSuspendPending)
    {
        res |= ThreadSuspendRequested;
    }

    HELPER_METHOD_FRAME_END();

    return res;
}
FCIMPLEND


// Wait for the thread to die
BOOL ThreadNative::DoJoin(THREADBASEREF DyingThread, INT32 timeout)
{
    _ASSERTE(DyingThread != NULL);

    HANDLE   h;
    DWORD    dwTimeOut32;
    DWORD    rv = 0;
    Thread  *DyingInternal;

    THROWSCOMPLUSEXCEPTION();

    if (timeout < 0 && timeout != INFINITE_TIMEOUT)
        COMPlusThrow(kArgumentOutOfRangeException, L"ArgumentOutOfRange_NeedNonNegOrNegative1");

    DyingInternal = DyingThread->GetInternal();

    // Validate the handle.  It's valid to Join a thread that's not running -- so
    // long as it was once started.
    if (DyingInternal == 0 ||
        !(DyingInternal->m_State & Thread::TS_LegalToJoin))
    {
        COMPlusThrow(kThreadStateException, IDS_EE_THREAD_NOTSTARTED);
    }

    // Don't grab the handle until we know it has started, to eliminate the race
    // condition.
    h = DyingInternal->GetThreadHandle();

    if (ThreadIsDead(DyingInternal) || h == INVALID_HANDLE_VALUE)
        return TRUE;

    dwTimeOut32 = (timeout == INFINITE_TIMEOUT
                   ? INFINITE
                   : (DWORD) timeout);

    // There is a race here.  DyingThread is going to close its thread handle.
    // If we grab the handle and then DyingThread closes it, we will wait forever
    // in DoAppropriateWait.
    int RefCount = DyingInternal->IncExternalCount();
    h = DyingInternal->GetThreadHandle();
    if (RefCount == 1)
    {
        // !!! We resurrect the Thread Object.
        // !!! We will keep the Thread ref count to be 1 so that we will not try
        // !!! to destroy the Thread Object again.
        // !!! Do not call DecExternalCount here!
        _ASSERTE (h == INVALID_HANDLE_VALUE);
        return TRUE;
    }
    if (h == INVALID_HANDLE_VALUE)
    {
        DyingInternal->DecExternalCount(FALSE);
        return TRUE;
    }

    Thread *pCurThread = GetThread();
    pCurThread->EnablePreemptiveGC();

    COMPLUS_TRY {
    rv = pCurThread->DoAppropriateWait(1, &h, TRUE/*waitAll*/, dwTimeOut32,
                                       TRUE/*alertable*/);

    pCurThread->DisablePreemptiveGC();

    }
    COMPLUS_FINALLY {
    DyingInternal->DecExternalCount(FALSE);
    } COMPLUS_END_FINALLY
    return (rv == WAIT_OBJECT_0);
}


// There are two reasons for its existence.
// First: SharedState cannot be stack allocated because it will be passed 
// between two threads.  One is free to return, before the other consumes it.
//
// Second, it's not possible to do a C++ 'new' in the same method as a COM catch/try.
// That's because they each use different try/fail (C++ vs. SEH).  So move it down
// here where hopefully it will not be inlined.
SharedState *SharedState::MakeSharedState(OBJECTREF threadable, Thread *internal, OBJECTREF principal)
{
    return new SharedState(threadable, internal, principal);
}


// We don't get a constructor for ThreadBaseObject, so we rely on the fact that this
// method is only called once, out of SetStart.  Since SetStart is private/native
// and only called from the constructor, we'll only get called here once to set it
// up and once (with NULL) to tear it down.  The 'null' can only come from Finalize
// because the constructor throws if it doesn't get a valid delegate.
void ThreadBaseObject::SetDelegate(OBJECTREF delegate)
{
#ifdef APPDOMAIN_STATE
	if (delegate != NULL)
	{
		AppDomain *pDomain = delegate->GetAppDomain();
		Thread *pThread = GetInternal();
		AppDomain *kickoffDomain = pThread->GetKickOffDomain();
		_ASSERTE_ALL_BUILDS(! pDomain || pDomain == kickoffDomain);
		_ASSERTE_ALL_BUILDS(kickoffDomain == GetThread()->GetDomain());
	}
#endif

    SetObjectReferenceUnchecked( (OBJECTREF *)&m_Delegate, delegate );

    // If the delegate is being set then initialize the other data members.
    if (m_Delegate != NULL)
    {
        // Initialize the thread priority to normal.
        m_Priority = ThreadNative::PRIORITY_NORMAL;
    }
}


// If the exposed object is created after-the-fact, for an existing thread, we call
// InitExisting on it.  This is the other "construction", as opposed to SetDelegate.
void ThreadBaseObject::InitExisting()
{
    switch (::GetThreadPriority(GetHandle()))
    {
    case THREAD_PRIORITY_LOWEST:
    case THREAD_PRIORITY_IDLE:
        m_Priority = ThreadNative::PRIORITY_LOWEST;
        break;

    case THREAD_PRIORITY_BELOW_NORMAL:
        m_Priority = ThreadNative::PRIORITY_BELOW_NORMAL;
        break;

    case THREAD_PRIORITY_NORMAL:
        m_Priority = ThreadNative::PRIORITY_NORMAL;
        break;

    case THREAD_PRIORITY_ABOVE_NORMAL:
        m_Priority = ThreadNative::PRIORITY_ABOVE_NORMAL;
        break;

    case THREAD_PRIORITY_HIGHEST:
    case THREAD_PRIORITY_TIME_CRITICAL:
        m_Priority = ThreadNative::PRIORITY_HIGHEST;
        break;

    case THREAD_PRIORITY_ERROR_RETURN:
    default:
        _ASSERTE(FALSE);
        m_Priority = ThreadNative::PRIORITY_NORMAL;
        break;
    }

}

            
FCIMPL1(void, ThreadNative::Finalize, ThreadBaseObject* pThisUNSAFE)
{

    THREADBASEREF   refThis = (THREADBASEREF)ObjectToOBJECTREF(pThisUNSAFE);

    HELPER_METHOD_FRAME_BEGIN_1(refThis);

    _ASSERTE(refThis != NULL);

    Thread*         thread  = refThis->GetInternal();

    // Prevent multiple calls to Finalize
    // Objects can be resurrected after being finalized.  However, there is no
    // race condition here.  We always check whether an exposed thread object is
    // still attached to the internal Thread object, before proceeding.
    if (thread)
    {
        refThis->SetDelegate(NULL);

    // During process shutdown, we finalize even reachable objects.  But if we break
    // the link between the System.Thread and the internal Thread object, the runtime
    // may not work correctly.  In particular, we won't be able to transition between
    // contexts and domains to finalize other objects.  Since the runtime doesn't
    // require that Threads finalize during shutdown, we need to disable this.  If
    // we wait until phase 2 of shutdown finalization (when the EE is suspended and
    // will never resume) then we can simply skip the side effects of Thread
    // finalization.
    if ((g_fEEShutDown & ShutDown_Finalize2) == 0)
    {
        if (GetThread() != thread)
        {
                refThis->SetInternal(NULL);
        }

        thread->RemoveAllDomainLocalStores();

        // we no longer need to keep the thread object alive.
        if (thread)
            {
            thread->DecExternalCount(FALSE);
    }
}
    }
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND


FCIMPL0(Object*, ThreadNative::GetDomainLocalStore)
{
    OBJECTREF refRetVal = NULL;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refRetVal);

    Thread* thread = GetThread();
    
    if (thread && thread->m_pDLSHash && thread->GetDomain())
    {
        HashDatum Data;

        Thread *pCurThread = GetThread();
        BOOL toggleGC = pCurThread->PreemptiveGCDisabled();
        
        if (toggleGC)
            pCurThread->EnablePreemptiveGC();
        ThreadStore::LockDLSHash();
        if (toggleGC)
            pCurThread->DisablePreemptiveGC();

        if (thread->m_pDLSHash->GetValue(thread->GetDomain()->GetId(), &Data))
        {
            LocalDataStore *pLDS = (LocalDataStore *) Data;
            refRetVal = (OBJECTREF) pLDS->GetRawExposedObject();
            _ASSERTE(refRetVal != NULL);
        }
        ThreadStore::UnlockDLSHash();
    }

    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refRetVal);
}
FCIMPLEND

FCIMPL1(void, ThreadNative::SetDomainLocalStore, Object* pLocalDataStoreUNSAFE)
{
    LOCALDATASTOREREF refLocalDataStore = (LOCALDATASTOREREF)ObjectToOBJECTREF(pLocalDataStoreUNSAFE);
    HELPER_METHOD_FRAME_BEGIN_1(refLocalDataStore);

    THROWSCOMPLUSEXCEPTION();
    Thread* thread = GetThread();
    
    if (thread && thread->GetDomain())
    {
        Thread *pCurThread = GetThread();
        BOOL toggleGC = pCurThread->PreemptiveGCDisabled();
        
        if (toggleGC)
            pCurThread->EnablePreemptiveGC();
        ThreadStore::LockDLSHash();
        if (toggleGC)
            pCurThread->DisablePreemptiveGC();

        if (!thread->m_pDLSHash)
        {
            thread->m_pDLSHash = new EEIntHashTable();
            if (!thread->m_pDLSHash)
            {
                ThreadStore::UnlockDLSHash();
                COMPlusThrowOM();
            }
            LockOwner lock = {g_pThreadStore->GetDLSHashCrst(),IsOwnerOfCrst};
            thread->m_pDLSHash->Init(3,&lock);
        }

        LocalDataStore* pLDS = refLocalDataStore->GetLocalDataStore();
        if (!pLDS)
        {
            pLDS = new LocalDataStore();
            if (!pLDS)
            {
                ThreadStore::UnlockDLSHash();
                COMPlusThrowOM();
            }

            StoreFirstObjectInHandle(pLDS->m_ExposedTypeObject, refLocalDataStore);
            refLocalDataStore->SetLocalDataStore(pLDS);
        }

        thread->m_pDLSHash->InsertValue(thread->GetDomain()->GetId(), (HashDatum) pLDS);
        ThreadStore::UnlockDLSHash();
    }

    HELPER_METHOD_FRAME_END();
}
FCIMPLEND


FCIMPL0(Object*, ThreadNative::GetDomain)
{
    APPDOMAINREF refRetVal = NULL;

    Thread* thread = GetThread();

    if ((thread) && (thread->GetDomain()))
    {
        HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refRetVal);
        refRetVal = (APPDOMAINREF) thread->GetDomain()->GetExposedObject();
        HELPER_METHOD_FRAME_END();
    }

    return OBJECTREFToObject(refRetVal);
}
FCIMPLEND

LPVOID F_CALL_CONV ThreadNative::FastGetDomain()
{
    Thread *pThread;
    AppDomain *pDomain;
    OBJECTHANDLE ExposedObject;
   
    pThread = GetThread();
    if (!pThread) {
	return NULL;
    }
    pDomain = pThread->GetDomain();
    if (!pDomain) {
	return NULL;
    }
    ExposedObject = pDomain->m_ExposedObject;
    if (ExposedObject) {
        return *(LPVOID *)ExposedObject;
    }
    return NULL;    
}


// This is just a helper method that lets BCL get to the managed context
// from the contextID.
FCIMPL1(Object*, ThreadNative::GetContextFromContextID, LPVOID ContextID)
{   
    OBJECTREF   rv   = NULL;
    Context*    pCtx = (Context *) ContextID;
    // Get the managed context backing this unmanaged context
    rv = pCtx->GetExposedObjectRaw();

    // This assert maintains the following invariant:
    // Only default unmanaged contexts can have a null managed context
    // (All non-deafult contexts are created as managed contexts first, and then
    // hooked to the unmanaged context)
    _ASSERTE((rv != NULL) || (pCtx->GetDomain()->GetDefaultContext() == pCtx));

    return OBJECTREFToObject(rv);
}
FCIMPLEND

FCIMPL5(BOOL, ThreadNative::EnterContextFromContextID, ThreadBaseObject* refThis, ContextBaseObject* refContext, LPVOID contextID, INT32 appDomainId, ContextTransitionFrame* pFrame)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(refThis != NULL);
	VALIDATEOBJECTREF(refThis);
    Thread *pThread = refThis->GetInternal();   
    Context *pCtx = (Context *)contextID;


    _ASSERTE(pCtx && (refContext == NULL || pCtx->GetExposedObjectRaw() == NULL || 
             ObjectToOBJECTREF(refContext) == pCtx->GetExposedObjectRaw()));

    // set vptr for frame
    *(void**)(pFrame) = ContextTransitionFrame::GetMethodFrameVPtr();

    // If we have a non-zero appDomain index, this is a x-domain call
    // We must verify that the AppDomain is not unloaded
    if (appDomainId != 0)
    {
        //
        // NOTE: there is a potential race between the time we retrieve the app domain pointer,
        // and the time which this thread enters the domain.
        // 
        // To solve the race, we rely on the fact that there is a thread sync 
        // between releasing an app domain's handle, and destroying the app domain.  Thus
        // it is important that we not go into preemptive gc mode in that window.
        // 

        AppDomain* pAppDomain = SystemDomain::GetAppDomainAtId(appDomainId);

        if (pAppDomain == NULL && pThread == GCHeap::GetFinalizerThread())
            // check if the appdomain being unloaded has this ID so finalizer
            // thread might be allowed in
        {
            AppDomain *pUnloadingDomain = SystemDomain::System()->AppDomainBeingUnloaded();
            if (pUnloadingDomain && pUnloadingDomain->GetId())
                pAppDomain = pUnloadingDomain;
        }

        if (pAppDomain == NULL || !pAppDomain->CanThreadEnter(pThread))
            FCThrowRes(kAppDomainUnloadedException, L"Remoting_AppDomainUnloaded");
    }
    
    // Verify that the Context is valid. 
    if ( !Context::ValidateContext(pCtx) )
        FCThrowRes(kRemotingException, L"Remoting_InvalidContext");
    
    LOG((LF_APPDOMAIN, LL_INFO1000, "ThreadNative::EnterContextFromContextID: %p, %p pushing frame %p\n", refContext, pCtx, pFrame));
    // install our frame. We have to put it here before we put the helper frame on
    pFrame->Push();
        ;
    // Set the VM conext

    HELPER_METHOD_FRAME_BEGIN_RET_NOPOLL();
    pThread->EnterContextRestricted(pCtx, pFrame, FALSE);
    HELPER_METHOD_FRAME_END_POLL();
    return TRUE;
}
FCIMPLEND

FCIMPL2(BOOL, ThreadNative::ReturnToContextFromContextID, ThreadBaseObject* refThis, ContextTransitionFrame* pFrame)
{
    _ASSERTE(refThis != NULL);
	VALIDATEOBJECTREF(refThis);
    Thread *pThread = refThis->GetInternal();

    HELPER_METHOD_FRAME_BEGIN_RET_NOPOLL();
    // Reset the VM context
    pThread->ReturnToContext(pFrame, FALSE);
    HELPER_METHOD_FRAME_END_POLL();

#ifdef _DEBUG
    Context *pCtx = pFrame->GetReturnContext();
#endif

    LOG((LF_APPDOMAIN, LL_INFO1000, "ThreadNative::ReturnIntoContextFromContextID: %p popping frame %p\n", pCtx, pFrame));
    _ASSERTE(pThread->GetFrame() == pFrame);
    pFrame->Pop();
    _ASSERTE(Context::ValidateContext(pCtx));

    return TRUE;
}
FCIMPLEND

FCIMPL1(void, ThreadNative::InformThreadNameChange, ThreadBaseObject* thread)
{
	VALIDATEOBJECTREF(thread);
#ifdef DEBUGGING_SUPPORTED
    Thread *pThread = thread->GetInternal();
    HELPER_METHOD_FRAME_BEGIN_NOPOLL();
    _ASSERTE(NULL != g_pDebugInterface);
    g_pDebugInterface->NameChangeEvent (NULL, pThread);
    HELPER_METHOD_FRAME_END_POLL();
#endif // DEBUGGING_SUPPORTED
}
FCIMPLEND

FCIMPL2(BOOL, ThreadNative::IsRunningInDomain, ThreadBaseObject* thread, int domainId)
{
	VALIDATEOBJECTREF(thread);
    Thread *pThread = thread->GetInternal();
    Frame *pFrame;
    HELPER_METHOD_FRAME_BEGIN_RET_NOPOLL();
    AppDomain *pDomain = SystemDomain::GetAppDomainAtId(domainId);
    pFrame = pThread->IsRunningIn(pDomain, NULL);
    HELPER_METHOD_FRAME_END_POLL();
    return pFrame != NULL;
}
FCIMPLEND

FCIMPL1(BOOL, ThreadNative::IsThreadpoolThread, ThreadBaseObject* thread)
{
    Thread *pThread = thread->GetInternal();
	return pThread->IsThreadPoolThread() != FALSE;
}
FCIMPLEND


FCIMPL1(void, ThreadNative::SpinWait, int iterations)
{
	for(int i = 0; i < iterations; i++) 
		pause();
}
FCIMPLEND


