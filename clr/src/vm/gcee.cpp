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

#include "common.h"
#include "dbginterface.h"

#include "gcsmppriv.h"

#include "remoting.h"
#include "comsynchronizable.h"
#include "comsystem.h"

#include "syncclean.hpp"


// The contract between GC and the EE, for starting and finishing a GC is as follows:
//
//      LockThreadStore
//      SetGCInProgress
//      SuspendEE
//
//      ... perform the GC ...
//
//      SetGCDone
//      RestartEE
//      UnlockThreadStore
//
// Note that this is intentionally *not* symmetrical.  The EE will assert that the
// GC does most of this stuff in the correct sequence.

// sets up vars for GC

COUNTER_ONLY(PERF_COUNTER_TIMER_PRECISION g_TotalTimeInGC = 0);
COUNTER_ONLY(PERF_COUNTER_TIMER_PRECISION g_TotalTimeSinceLastGCEnd = 0);

void GCHeap::UpdatePreGCCounters()
{

#if defined(ENABLE_PERF_COUNTERS)
    size_t allocation_0 = 0;
    size_t allocation_3 = 0; 
    size_t large_obj_heap_size = 0;
    
// Publish perf stats
    g_TotalTimeInGC = GET_CYCLE_COUNT();

    gc_heap* hp = pGenGCHeap;
    allocation_0 = 
        dd_desired_allocation (hp->dynamic_data_of (0))-
        dd_new_allocation (hp->dynamic_data_of (0));
    allocation_3 = 
        dd_desired_allocation (hp->dynamic_data_of (max_generation+1))-
        dd_new_allocation (hp->dynamic_data_of (max_generation+1));
    large_obj_heap_size = hp->large_blocks_size;
        

    GetGlobalPerfCounters().m_GC.cbAlloc += allocation_0;
    GetPrivatePerfCounters().m_GC.cbAlloc += allocation_0;
        
    GetGlobalPerfCounters().m_GC.cbAlloc += allocation_3;
    GetPrivatePerfCounters().m_GC.cbAlloc += allocation_3;
    
    GetGlobalPerfCounters().m_GC.cbLargeAlloc += allocation_3;
    GetPrivatePerfCounters().m_GC.cbLargeAlloc += allocation_3;

    GetGlobalPerfCounters().m_GC.cLrgObjSize = large_obj_heap_size;
    GetPrivatePerfCounters().m_GC.cLrgObjSize = large_obj_heap_size;
    
    GetGlobalPerfCounters().m_GC.cPinnedObj = 0;
    GetPrivatePerfCounters().m_GC.cPinnedObj = 0;

    // The following two counters are not a part of the memory object
    // They are reset here due to the lack of a heartbeat mechanism in the CLR
    // We use GCs as a hearbeat, since if the app is not doing gc maybe the perf of it
    // is not interesting?
    GetPrivatePerfCounters().m_Jit.timeInJit = 0;
    GetGlobalPerfCounters().m_Jit.timeInJit = 0;
    GetPrivatePerfCounters().m_Jit.timeInJitBase = 1; // To avoid divide by zero
    GetGlobalPerfCounters().m_Jit.timeInJitBase = 1; // To avoid divide by zero
    GetGlobalPerfCounters().m_Security.timeRTchecks = 0;
    GetPrivatePerfCounters().m_Security.timeRTchecks = 0;
    GetGlobalPerfCounters().m_Security.timeRTchecksBase = 1; // To avoid divide by zero
    GetPrivatePerfCounters().m_Security.timeRTchecksBase = 1; // To avoid divide by zero

#endif //ENABLE_PERF_COUNTERS
}   

void GCHeap::UpdatePostGCCounters()
{

#if defined(ENABLE_PERF_COUNTERS)
    // Publish Perf Data

    int xGen;
    gc_heap* hp = pGenGCHeap;

//Generation 0 is empty (if there isn't demotion) so its size is 0
//It is more interesting to report the desired size before next collection.
    for (xGen = 0; xGen < MAX_TRACKED_GENS; xGen++)
    {
        size_t gensize = 0;

        gensize = ((xGen == 0) ? 
                   dd_desired_allocation (hp->dynamic_data_of (xGen)) :
                   hp->generation_size(xGen));    


        GetGlobalPerfCounters().m_GC.cGenHeapSize[xGen] = gensize;
        GetPrivatePerfCounters().m_GC.cGenHeapSize[xGen] = gensize;

        GetGlobalPerfCounters().m_GC.cGenCollections[xGen] =
            dd_collection_count (hp->dynamic_data_of (xGen));
        GetPrivatePerfCounters().m_GC.cGenCollections[xGen] =
            dd_collection_count (hp->dynamic_data_of (xGen));
        
    }

    for (xGen = 0; xGen <= (int)GcCondemnedGeneration; xGen++)
    {
        size_t promoted_mem = 0; 
        size_t promoted_finalization_mem = 0;
        promoted_mem =  dd_promoted_size (hp->dynamic_data_of (xGen));
        promoted_finalization_mem =  dd_freach_previous_promotion (hp->dynamic_data_of (xGen));
        if (xGen < (MAX_TRACKED_GENS - 1))
        {
            GetGlobalPerfCounters().m_GC.cbPromotedMem[xGen] = promoted_mem;
            GetPrivatePerfCounters().m_GC.cbPromotedMem[xGen] = promoted_mem;
            
            GetGlobalPerfCounters().m_GC.cbPromotedFinalizationMem[xGen] = promoted_finalization_mem;
            GetPrivatePerfCounters().m_GC.cbPromotedFinalizationMem[xGen] = promoted_finalization_mem;
        }
    }
    for (xGen = (int)GcCondemnedGeneration + 1 ; xGen < MAX_TRACKED_GENS-1; xGen++)
    {
        // Reset the promoted mem for generations higer than the condemned one.
        GetGlobalPerfCounters().m_GC.cbPromotedMem[xGen] = 0;
        GetPrivatePerfCounters().m_GC.cbPromotedMem[xGen] = 0;
        
        GetGlobalPerfCounters().m_GC.cbPromotedFinalizationMem[xGen] = 0;
        GetPrivatePerfCounters().m_GC.cbPromotedFinalizationMem[xGen] = 0;
    }

    
    //Committed memory 
    {
        size_t committed_mem = 0;
        size_t reserved_mem = 0;
            {
                heap_segment* seg = 
                    generation_start_segment (hp->generation_of (max_generation));
                while (seg)
                {
                    committed_mem += heap_segment_committed (seg) - 
                        heap_segment_mem (seg);
                    reserved_mem += heap_segment_reserved (seg) - 
                        heap_segment_mem (seg);
                    seg = heap_segment_next (seg);
                }
        }
        committed_mem += hp->large_blocks_size;
        reserved_mem += hp->large_blocks_size;

        GetGlobalPerfCounters().m_GC.cTotalCommittedBytes =
            committed_mem;
        GetPrivatePerfCounters().m_GC.cTotalCommittedBytes = 
            committed_mem;

        GetGlobalPerfCounters().m_GC.cTotalReservedBytes =
            reserved_mem;
        GetPrivatePerfCounters().m_GC.cTotalReservedBytes = 
            reserved_mem;

    }
            

    GetGlobalPerfCounters().m_GC.cLrgObjSize = hp->large_blocks_size;       
    GetPrivatePerfCounters().m_GC.cLrgObjSize = hp->large_blocks_size;      
    GetGlobalPerfCounters().m_GC.cSurviveFinalize =  GetFinalizablePromotedCount();

    GetPrivatePerfCounters().m_GC.cSurviveFinalize = GetFinalizablePromotedCount();
    
    // Compute Time in GC
    PERF_COUNTER_TIMER_PRECISION _currentPerfCounterTimer = GET_CYCLE_COUNT();
    
    g_TotalTimeInGC = _currentPerfCounterTimer - g_TotalTimeInGC;
    PERF_COUNTER_TIMER_PRECISION _timeInGCBase = (_currentPerfCounterTimer - g_TotalTimeSinceLastGCEnd);

    _ASSERTE (_timeInGCBase >= g_TotalTimeInGC);
    while (_timeInGCBase > UINT_MAX) 
    {
        _timeInGCBase = _timeInGCBase >> 8;
        g_TotalTimeInGC = g_TotalTimeInGC >> 8;
        _ASSERTE (_timeInGCBase >= g_TotalTimeInGC);
    }

    // Update Total Time    
    GetGlobalPerfCounters().m_GC.timeInGC = (DWORD)g_TotalTimeInGC;
    GetPrivatePerfCounters().m_GC.timeInGC = (DWORD)g_TotalTimeInGC;

    GetGlobalPerfCounters().m_GC.timeInGCBase = (DWORD)_timeInGCBase;
    GetPrivatePerfCounters().m_GC.timeInGCBase = (DWORD)_timeInGCBase;
    
    g_TotalTimeSinceLastGCEnd = _currentPerfCounterTimer;

#endif //ENABLE_PERF_COUNTERS
}

void ProfScanRootsHelper(Object*& object, ScanContext *pSC, DWORD dwFlags)
{
#ifdef GC_PROFILING
    Object *pObj = object;
#ifdef INTERIOR_POINTERS
    if (dwFlags & GC_CALL_INTERIOR)
    {
        BYTE *o = (BYTE*)object;
        gc_heap* hp = gc_heap::heap_of (o
#ifdef _DEBUG
                                        , !(dwFlags & GC_CALL_INTERIOR)
#endif //_DEBUG
                                       );

        if ((o < hp->gc_low) || (o >= hp->gc_high))
        {
            return;
        }
        pObj = (Object*) hp->find_object(o, hp->gc_low);
    }
#endif //INTERIOR_POINTERS
    ScanRootsHelper(pObj, pSC, dwFlags);
#endif // GC_PROFILING
}

void GCProfileWalkHeap()
{
#if defined (GC_PROFILING)
    if (CORProfilerTrackGC())
    {
        // Indicate that inproc debugging is permitted for the duration of the heap walk
        g_profControlBlock.inprocState = ProfControlBlock::INPROC_PERMITTED;

        ProfilingScanContext SC;

        // Ask the vm to go over all of the roots
        CNameSpace::GcScanRoots(&ProfScanRootsHelper, max_generation, max_generation, &SC);

        // The finalizer queue is also a source of roots
        pGenGCHeap->finalize_queue->GcScanRoots(&ScanRootsHelper, 0, &SC);


        // Handles are kept independent of wks/svr/concurrent builds
        CNameSpace::GcScanHandlesForProfiler(max_generation, &SC);

        // Indicate that root scanning is over, so we can flush the buffered roots
        // to the profiler
        g_profControlBlock.pProfInterface->EndRootReferences(&SC.pHeapId);

        gc_heap::walk_heap (&HeapWalkHelper, 0, max_generation, TRUE);

        // Indicate that inproc debugging is no longer permitted
        g_profControlBlock.inprocState = ProfControlBlock::INPROC_FORBIDDEN;
    }
#endif //GC_PROFILING
}

void GCHeap::RestartEE(BOOL bFinishedGC, BOOL SuspendSucceded)
{
    if (g_fSuspendOnShutdown) {
        // We are shutting down.  The finalizer thread has suspended EE.
        // There will only be one thread running inside EE: either the shutdown
        // thread or the finalizer thread.

#ifdef PROFILING_SUPPORTED
		g_profControlBlock.inprocState = ProfControlBlock::INPROC_PERMITTED;
#endif

        _ASSERTE (g_fEEShutDown);
        m_suspendReason = SUSPEND_FOR_SHUTDOWN;
        return;
    }

#ifdef TIME_CPAUSE
    printf ("Pause time: %d\n", GetCycleCount32() - cstart);
#endif //TIME_CPAUSE

    // SetGCDone();
    SyncClean::CleanUp();
    GcInProgress= FALSE;
    ThreadStore::TrapReturningThreads(FALSE);
    GcThread    = 0;
    SetEvent( WaitForGCEvent );
    _ASSERTE(ThreadStore::HoldingThreadStore());

    Thread::SysResumeFromGC(bFinishedGC, SuspendSucceded);
}

void GCHeap::SuspendEE(SUSPEND_REASON reason)
{
    BOOL gcOnTransitions;

#ifdef TIME_CPAUSE
    cstart = GetCycleCount32();
#endif //TIME_CPAUSE

    if (g_fSuspendOnShutdown) {
        // We are shutting down.  The finalizer thread has suspended EE.
        // There will only be one thread running inside EE: either the shutdown
        // thread or the finalizer thread.
#ifdef PROFILING_SUPPORTED
		if (reason == GCHeap::SUSPEND_FOR_GC || reason == GCHeap::SUSPEND_FOR_GC_PREP)
			g_profControlBlock.inprocState = ProfControlBlock::INPROC_FORBIDDEN;
#endif

        _ASSERTE (g_fEEShutDown);
        m_suspendReason = reason;
        return;
    }

    LOG((LF_SYNC, INFO3, "Suspending the runtime for reason %d\n", reason));

    // lock the thread store which could take us out of our must
    // complete
    // Need the thread store lock here.  We take this lock before the thread
    // lock to avoid a deadlock condition where another thread suspends this
    // thread while it holds the heap lock.  While the thread store lock is
    // held, threads cannot be suspended.
    gcOnTransitions = GC_ON_TRANSITIONS(FALSE);        // dont do GC for GCStress 3

    Thread* pCurThread = GetThread();

    // Set variable to indicate that this thread is preforming a true GC
    // This is needed to overcome deadlock in taking the ThreadStore lock
    if (reason == GCHeap::SUSPEND_FOR_GC || reason == GCHeap::SUSPEND_FOR_GC_PREP)
    {
        m_GCThreadAttemptingSuspend = pCurThread;

    }

retry_for_debugger:
    ThreadStore::LockThreadStore(reason);

    if (ThreadStore::s_hAbortEvt != NULL &&
        (reason == GCHeap::SUSPEND_FOR_GC || reason == GCHeap::SUSPEND_FOR_GC_PREP))
    {
        LOG((LF_SYNC, INFO3, "GC thread is backing out the suspend abort event.\n"));
        ThreadStore::s_hAbortEvt = NULL;

        LOG((LF_SYNC, INFO3, "GC thread is signalling the suspend abort event.\n"));
        SetEvent(ThreadStore::s_hAbortEvtCache);
    }

    // Set variable to indicate that this thread is attempting the suspend because it
    // needs to perform a GC and, as such, it holds GC locks.
    if (reason == GCHeap::SUSPEND_FOR_GC || reason == GCHeap::SUSPEND_FOR_GC_PREP)
    {
        m_GCThreadAttemptingSuspend = NULL;
    }

    {
        // suspend for GC, set in progress after suspending
        // threads which have no must complete
        ResetEvent( WaitForGCEvent );
        // SetGCInProgress();
        {
            GcThread = pCurThread;
            ThreadStore::TrapReturningThreads(TRUE);
            m_suspendReason = reason;

#ifdef PROFILING_SUPPORTED
			if (reason == GCHeap::SUSPEND_FOR_GC || reason == GCHeap::SUSPEND_FOR_GC_PREP)
				g_profControlBlock.inprocState = ProfControlBlock::INPROC_FORBIDDEN;
#endif // PROFILING_SUPPORTED

            GcInProgress= TRUE;
        }

        HRESULT hr;
        {
            _ASSERTE(ThreadStore::HoldingThreadStore() || g_fProcessDetach);
            hr = Thread::SysSuspendForGC(reason);
            ASSERT( hr == S_OK || hr == ERROR_TIMEOUT);
        }

        // If the debugging services are attached, then its possible
        // that there is a thread which appears to be stopped at a gc
        // safe point, but which really is not. If that is the case,
        // back off and try again.

        // If this is not the GC thread and another thread has triggered
        // a GC, then we may have bailed out of SysSuspendForGC, so we
        // must resume all of the threads and tell the GC that we are
        // at a safepoint - since this is the exact same behaviour
        // that the debugger needs, just use it's code.
        if ((hr == ERROR_TIMEOUT)
#ifdef DEBUGGING_SUPPORTED
             || (CORDebuggerAttached() && 
                 g_pDebugInterface->ThreadsAtUnsafePlaces())
#endif // DEBUGGING_SUPPORTED
            )
        {
            // In this case, the debugger has stopped at least one
            // thread at an unsafe place.  The debugger will usually
            // have already requested that we stop.  If not, it will 
            // either do so shortly -- or resume the thread that is
            // at the unsafe place.
            //
            // Either way, we have to wait for the debugger to decide
            // what it wants to do.
            //
            // Note: we've still got the more_space_lock lock held.

            LOG((LF_GCROOTS | LF_GC | LF_CORDB,
                 LL_INFO10,
                 "***** Giving up on current GC suspension due "
                 "to debugger or timeout *****\n"));            

            if (ThreadStore::s_hAbortEvtCache == NULL)
            {
                LOG((LF_SYNC, INFO3, "Creating suspend abort event.\n"));
                ThreadStore::s_hAbortEvtCache = CreateEvent(NULL, TRUE, FALSE, NULL);
            }

            LOG((LF_SYNC, INFO3, "Using suspend abort event.\n"));
            ThreadStore::s_hAbortEvt = ThreadStore::s_hAbortEvtCache;
            ResetEvent(ThreadStore::s_hAbortEvt);
            
            // Mark that we're done with the gc, just like at the
            // end of this method.
            RestartEE(FALSE, FALSE);            
            
            LOG((LF_GCROOTS | LF_GC | LF_CORDB,
                 LL_INFO10, "The EE is free now...\n"));
            
            // Check if we're ready to go to suspend.
            if (pCurThread && pCurThread->CatchAtSafePoint())
            {
                _ASSERTE(pCurThread->PreemptiveGCDisabled());
                pCurThread->PulseGCMode();  // Go suspend myself.
            }
            else
            {
                __SwitchToThread (0); // Wait a little while, before retrying.
            }

            goto retry_for_debugger;
        }
    }
    GC_ON_TRANSITIONS(gcOnTransitions);
}

void CallFinalizer(Object* obj)
{

    MethodTable     *pMT = obj->GetMethodTable();
    LOG((LF_GC, LL_INFO1000, "Finalizing " LOG_OBJECT_CLASS(obj)));

    _ASSERTE(GetThread()->PreemptiveGCDisabled());
    // if we don't have a class, we can't call the finalizer
    // if the object has been marked run as finalizer run don't call either
    if (pMT)
    {
        if (!((obj->GetHeader()->GetBits()) & BIT_SBLK_FINALIZER_RUN))
        {
            if (pMT->IsContextful())
            {
                Object *proxy = OBJECTREFToObject(CRemotingServices::GetProxyFromObject(ObjectToOBJECTREF(obj)));

                _ASSERTE(proxy && "finalizing an object that was never wrapped?????");                
                if (proxy == NULL)
                {
                    // Quite possibly the app abruptly shutdown while a proxy
                    // was being setup for a contextful object. We will skip
                    // finalizing this object.
                    _ASSERTE (g_fEEShutDown);
                    return;
                }
                else
                {
                    // This saves us from the situation where an object gets GC-ed 
                    // after its Context. 
                    Object* stub = (Object *)proxy->GetPtrOffset(CTPMethodTable::GetOffsetOfStubData());
                    Context *pServerCtx = (Context *) stub->UnBox();
                    // Check if the context is valid             
                    if (!Context::ValidateContext(pServerCtx))
                    {
                        // Since the server context is gone (GC-ed)
                        // we will associate the server with the default 
                        // context for a good faith attempt to run 
                        // the finalizer
                        // We want to do this only if we are using RemotingProxy
                        // and not for other types of proxies (eg. SvcCompPrxy)
                        OBJECTREF orRP = ObjectToOBJECTREF(CRemotingServices::GetRealProxy(proxy));
                        if(CTPMethodTable::IsInstanceOfRemotingProxy(
                            orRP->GetMethodTable()))
                        {
                        *((Context **)stub->UnBox()) = (Context*) GetThread()->GetContext();
                    }
                    }
                    // call Finalize on the proxy of the server object.
                    obj = proxy;
                }
            }           
            _ASSERTE(pMT->HasFinalizer());
            MethodTable::CallFinalizer(obj);
        }
        else
        {
            //reset the bit so the object can be put on the list 
            //with RegisterForFinalization
            obj->GetHeader()->ClrBit (BIT_SBLK_FINALIZER_RUN);
        }
    }
}

static char s_FinalizeObjectName[MAX_CLASSNAME_LENGTH+MAX_NAMESPACE_LENGTH+2];
static BOOL s_fSaveFinalizeObjectName = FALSE;

void  CallFinalizer(Thread* FinalizerThread, 
                    Object* fobj)
{
    if (s_fSaveFinalizeObjectName) {
        DefineFullyQualifiedNameForClass();
        LPCUTF8 name = GetFullyQualifiedNameForClass(fobj->GetClass());
        strcat (s_FinalizeObjectName, name);
    }
    CallFinalizer(fobj);
    if (s_fSaveFinalizeObjectName) {
        s_FinalizeObjectName[0] = '\0';
    }
    // we might want to do some extra work on the finalizer thread
    // check and do it
    if (FinalizerThread->HaveExtraWorkForFinalizer())
    {
        FinalizerThread->DoExtraWorkForFinalizer();
    }

    // if someone is trying to stop us, open the gates
    FinalizerThread->PulseGCMode();
}

struct FinalizeAllObjects_Args {
	OBJECTREF fobj;
    int bitToCheck;
};

static Object *FinalizeAllObjects(Object* fobj, int bitToCheck);

static void FinalizeAllObjects_Wrapper(LPVOID ptr)
{
    FinalizeAllObjects_Args *args = (FinalizeAllObjects_Args *) ptr;
    _ASSERTE(args->fobj);
    args->fobj = ObjectToOBJECTREF(FinalizeAllObjects(OBJECTREFToObject(args->fobj), args->bitToCheck));
}

static Object *FinalizeAllObjects(Object* fobj, int bitToCheck)
{
    bool fTerminate = false;

    if (fobj == NULL)
        fobj = GCHeap::GetNextFinalizableObject();

    // Finalize everyone
    while (fobj)
    {
        if (fobj->GetHeader()->GetBits() & bitToCheck)
        {
            fobj = GCHeap::GetNextFinalizableObject();
            continue;
        }

        COMPLUS_TRY 
        {
            Thread *pThread = GetThread();
            AppDomain* targetAppDomain = fobj->GetAppDomain();
            AppDomain* currentDomain = pThread->GetDomain();
            if (! targetAppDomain || ! targetAppDomain->CanThreadEnter(pThread))
            {
                // if can't get into domain to finalize it, then it must be agile so finalize in current domain
                targetAppDomain = currentDomain;
#if CHECK_APP_DOMAIN_LEAKS
                 // object must be agile if can't get into it's domain
                if (g_pConfig->AppDomainLeaks() && !fobj->SetAppDomainAgile(FALSE))   
                    _ASSERTE(!"Found non-agile GC object which should have been finalized during app domain unload.");
#endif
            }
            if (targetAppDomain == currentDomain)
            {
                CallFinalizer(fobj);
                fobj = GCHeap::GetNextFinalizableObject();
            } 
            else 
            {
                if (! targetAppDomain->GetDefaultContext())
                {
                    // can no longer enter domain becuase the handle containing the context has been
                    // nuked so just bail. Should only get this if are at the stage of nuking the
                    // handles in the domain if it's still open.
                    _ASSERTE(targetAppDomain->IsUnloading() && targetAppDomain->ShouldHaveRoots());
                    fobj = GCHeap::GetNextFinalizableObject();
                }
                else
                if (currentDomain != SystemDomain::System()->DefaultDomain())
                {
                    // this means we are in some other domain, so need to return back out through the DoADCallback
                    // and handle the object from there in another domain.
                    fTerminate = true;
                } 
                else
                {
                    // otherwise call back to ourselves to process as many as we can in that other domain
                    FinalizeAllObjects_Args args;
                    args.fobj = ObjectToOBJECTREF(fobj);
                    args.bitToCheck = bitToCheck;
                    GCPROTECT_BEGIN(args.fobj);
                    pThread->DoADCallBack(targetAppDomain->GetDefaultContext(), FinalizeAllObjects_Wrapper, &args);
                    // process the object we got back or be done if we got back null
                    fobj = OBJECTREFToObject(args.fobj);
                    GCPROTECT_END();
                }
            }
        }    
        COMPLUS_CATCH
        {
            // Should be an out of memory from Thread::EnterDomain.  Swallow,
            // no where to report this, and get the next object
            fobj = GCHeap::GetNextFinalizableObject();
        }
        COMPLUS_END_CATCH

        if (fTerminate)
            break;
    }
    return fobj;
}

void GCHeap::WaitUntilGCComplete()
{
    DWORD dwWaitResult = NOERROR;

    if (GcInProgress) {
        ASSERT( WaitForGCEvent );
        ASSERT( GetThread() != GcThread );

#ifdef DETECT_DEADLOCK
        // wait for GC to complete
BlockAgain:
        dwWaitResult = WaitForSingleObject( WaitForGCEvent,
                                            DETECT_DEADLOCK_TIMEOUT );

        if (dwWaitResult == WAIT_TIMEOUT) {
            //  Even in retail, stop in the debugger if available.  Ideally, the
            //  following would use DebugBreak, but debspew.h makes this a null
            //  macro in retail.  Note that in debug, we don't use the debspew.h
            //  macros because these take a critical section that may have been
            //  taken by a suspended thread.
            RetailDebugBreak();
            goto BlockAgain;
        }

#else  //DETECT_DEADLOCK

        
        if (g_fEEShutDown) {
            Thread *pThread = GetThread();
            if (pThread) {
                dwWaitResult = pThread->DoAppropriateAptStateWait(1, &WaitForGCEvent, FALSE, INFINITE, TRUE);
            } else {
                dwWaitResult = WaitForSingleObject( WaitForGCEvent, INFINITE );
            }

        }
        else
        {
            dwWaitResult = WaitForSingleObject( WaitForGCEvent, INFINITE );
        }
        
#endif //DETECT_DEADLOCK

    }
}



void WaitForFinalizerEvent (HANDLE event)
{
        WaitForSingleObject(event, INFINITE);
}





ULONG __stdcall GCHeap::FinalizerThreadStart(void *args)
{
    ASSERT(args == 0);
    ASSERT(GCHeap::hEventFinalizer);

    LOG((LF_GC, LL_INFO10, "Finalizer thread starting..."));

#ifdef _DEBUG
	AppDomain* defaultDomain = FinalizerThread->GetDomain();
#endif
    _ASSERTE(defaultDomain == SystemDomain::System()->DefaultDomain());

    BOOL    ok = FinalizerThread->HasStarted();

    _ASSERTE(ok);
    _ASSERTE(GetThread() == FinalizerThread);

    // finalizer should always park in default domain

    if (ok)
    {
        EE_TRY_FOR_FINALLY
        {
            FinalizerThread->SetBackground(TRUE);

            BOOL noUnloadedObjectsRegistered = FALSE;

            while (!fQuitFinalizer)
            {
                // Wait for work to do...

                _ASSERTE(FinalizerThread->PreemptiveGCDisabled());
#ifdef _DEBUG
                if (g_pConfig->FastGCStressLevel()) {
                    FinalizerThread->m_GCOnTransitionsOK = FALSE;
                }
#endif
                FinalizerThread->EnablePreemptiveGC();
#ifdef _DEBUG
                if (g_pConfig->FastGCStressLevel()) {
                    FinalizerThread->m_GCOnTransitionsOK = TRUE;
                }
#endif
                WaitForFinalizerEvent (GCHeap::hEventFinalizer);
                FinalizerThread->DisablePreemptiveGC();

#ifdef _DEBUG
                if (g_pConfig->GetGCStressLevel() > 1)
                {
                    int last_gc_count;
                    do {
                        last_gc_count = gc_count;
                        FinalizerThread->m_GCOnTransitionsOK = FALSE; 
                        FinalizerThread->EnablePreemptiveGC();
                        __SwitchToThread (0);
                        FinalizerThread->DisablePreemptiveGC();             
                            // If no GCs happended, then we assume we are quiescent
                        FinalizerThread->m_GCOnTransitionsOK = TRUE; 
                    } while (gc_count - last_gc_count > 0);
                }
#endif //_DEBUG
                
                // we might want to do some extra work on the finalizer thread
                // check and do it
                if (FinalizerThread->HaveExtraWorkForFinalizer())
                {
                    FinalizerThread->DoExtraWorkForFinalizer();
                }

                LOG((LF_GC, LL_INFO100, "***** Calling Finalizers\n"));
                FinalizeAllObjects(NULL, 0);
                _ASSERTE(FinalizerThread->GetDomain() == SystemDomain::System()->DefaultDomain());


                if (GCHeap::UnloadingAppDomain != NULL)
                {
                    // Now schedule any objects from an unloading app domain for finalization 
                    // on the next pass (even if they are reachable.)
                    // Note that it may take several passes to complete the unload, if new objects are created during
                    // finalization.

                    if (!FinalizeAppDomain(GCHeap::UnloadingAppDomain, GCHeap::fRunFinalizersOnUnload))
                    {
                        if (!noUnloadedObjectsRegistered)
                        {
                            //
                            // There is nothing left to schedule.  However, there are possibly still objects
                            // left in the finalization queue.  We might be done after the next pass, assuming
                            // we don't see any new finalizable objects in the domain.
                            noUnloadedObjectsRegistered = TRUE;
                        }
                        else
                        {
                            // We've had 2 passes seeing no objects - we're done.
                            GCHeap::UnloadingAppDomain = NULL;
                            noUnloadedObjectsRegistered = FALSE;
                        }
                    }
                    else
                        noUnloadedObjectsRegistered = FALSE;
                }

                // Anyone waiting to drain the Q can now wake up.  Note that there is a
                // race in that another thread starting a drain, as we leave a drain, may
                // consider itself satisfied by the drain that just completed.  This is
                // acceptable.
                SetEvent(GCHeap::hEventFinalizerDone);
            }
            
            // Tell shutdown thread we are done with finalizing dead objects.
            SetEvent (GCHeap::hEventFinalizerToShutDown);
            
            // Wait for shutdown thread to signal us.
            FinalizerThread->EnablePreemptiveGC();
            WaitForSingleObject(GCHeap::hEventShutDownToFinalizer, INFINITE);
            FinalizerThread->DisablePreemptiveGC();
            
            AppDomain::RaiseExitProcessEvent();

            SetEvent(GCHeap::hEventFinalizerToShutDown);
            
            // Phase 1 ends.
            // Now wait for Phase 2 signal.

            // Wait for shutdown thread to signal us.
            FinalizerThread->EnablePreemptiveGC();
            WaitForSingleObject(GCHeap::hEventShutDownToFinalizer, INFINITE);
            FinalizerThread->DisablePreemptiveGC();
            
            SetFinalizeQueueForShutdown (FALSE);
            
            // Finalize all registered objects during shutdown, even they are still reachable.
            // we have been asked to quit, so must be shutting down      
            _ASSERTE(g_fEEShutDown);
            _ASSERTE(FinalizerThread->PreemptiveGCDisabled());
            //
            //
            FinalizeAllObjects(NULL, BIT_SBLK_FINALIZER_RUN);
            _ASSERTE(FinalizerThread->GetDomain() == SystemDomain::System()->DefaultDomain());

            // we might want to do some extra work on the finalizer thread
            // check and do it
            if (FinalizerThread->HaveExtraWorkForFinalizer())
            {
                FinalizerThread->DoExtraWorkForFinalizer();
            }

            SetEvent(GCHeap::hEventFinalizerToShutDown);

            // Wait for shutdown thread to signal us.
            FinalizerThread->EnablePreemptiveGC();
            WaitForSingleObject(GCHeap::hEventShutDownToFinalizer, INFINITE);
            FinalizerThread->DisablePreemptiveGC();


            SetEvent(GCHeap::hEventFinalizerToShutDown);
        }
        EE_FINALLY
        {
            if (GOT_EXCEPTION())
                _ASSERTE(!"Exception in the finalizer thread!");
        }
        EE_END_FINALLY;
    }
    // finalizer should always park in default domain
    _ASSERTE(GetThread()->GetDomain() == SystemDomain::System()->DefaultDomain());

    LOG((LF_GC, LL_INFO10, "Finalizer thread done."));

    // Enable pre-emptive GC before we leave so that anybody trying to suspend
    // us will not end up waiting forever. Don't do a DestroyThread because this
    // will happen soon when we tear down the thread store.
    FinalizerThread->EnablePreemptiveGC();

    // We do not want to tear Finalizer thread,
    // since doing so will cause OLE32 to CoUninitalize.
    ::Sleep(INFINITE);
    
    return 0;
}

DWORD GCHeap::FinalizerThreadCreate()
{
    DWORD   dwRet = 0;
    HANDLE  h;
    DWORD   newThreadId;

    hEventFinalizerDone = CreateEvent(0, TRUE, FALSE, 0);
    if (hEventFinalizerDone)
    {
        hEventFinalizer = CreateEvent(0, FALSE, FALSE, 0);
        hEventFinalizerToShutDown = CreateEvent(0, FALSE, FALSE, 0);
        hEventShutDownToFinalizer = CreateEvent(0, FALSE, FALSE, 0);
        if (hEventFinalizer && hEventFinalizerToShutDown && hEventShutDownToFinalizer)
        {
            _ASSERTE(FinalizerThread == 0);
            FinalizerThread = SetupUnstartedThread();
            if (FinalizerThread == 0) {
                return 0;
            }

            // We don't want the thread block disappearing under us -- even if the
            // actual thread terminates.
            FinalizerThread->IncExternalCount();

            h = FinalizerThread->CreateNewThread(4096, &FinalizerThreadStart, NULL, &newThreadId);
            if (h)
            {
                ::SetThreadPriority(h, THREAD_PRIORITY_HIGHEST);

                // Before we do the resume, we need to take note of the new ThreadId.  This
                // is necessary because -- before the thread starts executing at KickofThread --
                // it may perform some DllMain DLL_THREAD_ATTACH notifications.  These could
                // call into managed code.  During the consequent SetupThread, we need to
                // perform the Thread::HasStarted call instead of going through the normal
                // 'new thread' pathway.
                _ASSERTE(FinalizerThread->GetThreadId() == 0);
                _ASSERTE(newThreadId != 0);

                FinalizerThread->SetThreadId(newThreadId);

                dwRet = ::ResumeThread(h);
                _ASSERTE(dwRet == 1);
            }
        }
    }
    return dwRet;
}

// Wait for the finalizer thread to complete one pass.
void GCHeap::FinalizerThreadWait()
{
    ASSERT(hEventFinalizerDone);
    ASSERT(hEventFinalizer);
    ASSERT(FinalizerThread);

    // Can't call this from within a finalized method.
    if (!IsCurrentThreadFinalizer())
    {

        Thread  *pCurThread = GetThread();
        BOOL     toggleGC = pCurThread->PreemptiveGCDisabled();

        if (toggleGC)
            pCurThread->EnablePreemptiveGC();

        ::ResetEvent(hEventFinalizerDone);
        ::SetEvent(hEventFinalizer);

        //----------------------------------------------------
        // Do appropriate wait and pump messages if necessary
        //----------------------------------------------------
        //WaitForSingleObject(hEventFinalizerDone, INFINITE);

        pCurThread->DoAppropriateAptStateWait(1, &hEventFinalizerDone, FALSE, INFINITE, TRUE);

        if (toggleGC)
            pCurThread->DisablePreemptiveGC();
    }
}


#ifdef _DEBUG
#define FINALIZER_WAIT_TIMEOUT 250
#else
#define FINALIZER_WAIT_TIMEOUT 200
#endif
#define FINALIZER_TOTAL_WAIT 2000

static BOOL s_fRaiseExitProcessEvent = FALSE;

BOOL GCHeap::FinalizerThreadWatchDog()
{
    
    Thread *pThread = GetThread();
    HANDLE      h = FinalizerThread->GetThreadHandle();

        // Do not wait for FinalizerThread if the current one is FinalizerThread.
        if (pThread == FinalizerThread)
            return TRUE;

        // If finalizer thread is gone, just return.
        if (h == INVALID_HANDLE_VALUE || WaitForSingleObject (h, 0) != WAIT_TIMEOUT)
            return TRUE;

	// *** This is the first call ShutDown -> Finalizer to Finilize dead objects ***
	if ((g_fEEShutDown & ShutDown_Finalize1) &&
        !(g_fEEShutDown & ShutDown_Finalize2)) {
        // Wait for the finalizer...
        LOG((LF_GC, LL_INFO10, "Signalling finalizer to quit..."));

        fQuitFinalizer = TRUE;
        ResetEvent(hEventFinalizerDone);
        SetEvent(hEventFinalizer);

        LOG((LF_GC, LL_INFO10, "Waiting for finalizer to quit..."));
        
        pThread->EnablePreemptiveGC();
        BOOL fTimeOut = FinalizerThreadWatchDogHelper();
        
        if (!fTimeOut) {
            SetEvent(hEventShutDownToFinalizer);

            // Wait for finalizer thread to finish raising ExitProcess Event.
            s_fRaiseExitProcessEvent = TRUE;
            fTimeOut = FinalizerThreadWatchDogHelper();
            if (fTimeOut) {
                s_fRaiseExitProcessEvent = FALSE;
            }
        }
        
        pThread->DisablePreemptiveGC();
        
        // Can not call ExitProcess here if we are in a hosting environment.
        // The host does not expect that we terminate the process.
        //if (fTimeOut)
        //{
            //::ExitProcess (GetLatchedExitCode());
        //}
        
        return !fTimeOut;
    }

	// *** This is the second call ShutDown -> Finalizer to ***
	// suspend the Runtime and Finilize live objects
    if ( g_fEEShutDown & ShutDown_Finalize2 &&
        !(g_fEEShutDown & ShutDown_COM) ) {
        
        _ASSERTE (g_fEEShutDown & ShutDown_Finalize1);
        SuspendEE(GCHeap::SUSPEND_FOR_SHUTDOWN);

        g_fSuspendOnShutdown = TRUE;
        
        GcThread = FinalizerThread;
        // !!! We will not resume EE from now on.  But we are setting the finslizer thread
        // !!! to be the thread that SuspendEE, so that it will be blocked.
        // !!! Before we wake up Finalizer thread, we need to enable preemptive gc on the
        // !!! finalizer thread.  Otherwise we may see a deadlock during debug test.
        pThread->EnablePreemptiveGC();
        
        g_fFinalizerRunOnShutDown = TRUE;
        
        // Wait for finalizer thread to finish finalizing all objects.
        SetEvent(GCHeap::hEventShutDownToFinalizer);
        BOOL fTimeOut = FinalizerThreadWatchDogHelper();

        if (!fTimeOut) {
            // We only switch back GcThread if we do not timeout.
            // We check these to decide if we want to enter EE when processing DLL_PROCESS_DETACH.
            GcThread = pThread;
            g_fFinalizerRunOnShutDown = FALSE;
        }
        
        // Can not call ExitProcess here if we are in a hosting environment.
        // The host does not expect that we terminate the process.
        //if (fTimeOut) {
        //    ::ExitProcess (GetLatchedExitCode());
        //}

        pThread->DisablePreemptiveGC();
        return !fTimeOut;
    }

    // *** This is the third call ShutDown -> Finalizer ***
    // to do additional cleanup
	if (g_fEEShutDown & ShutDown_COM) {
        _ASSERTE (g_fEEShutDown & (ShutDown_Finalize2 | ShutDown_Finalize1));

        GcThread = FinalizerThread;
        pThread->EnablePreemptiveGC();
        g_fFinalizerRunOnShutDown = TRUE;
        
		SetEvent(GCHeap::hEventShutDownToFinalizer);
        DWORD status = pThread->DoAppropriateAptStateWait(1, &hEventFinalizerToShutDown,
        	                                    FALSE, FINALIZER_WAIT_TIMEOUT,
        	                                    TRUE);
        
		BOOL fTimeOut = (status == WAIT_TIMEOUT) ? TRUE : FALSE;

		if (!fTimeOut) {
            GcThread = pThread;
            g_fFinalizerRunOnShutDown = FALSE;
        }
		pThread->DisablePreemptiveGC();

        return !fTimeOut;
	}

    _ASSERTE(!"Should never reach this point");
	return FALSE;
}

BOOL GCHeap::FinalizerThreadWatchDogHelper()
{
    Thread *pThread = GetThread();
    _ASSERTE (!pThread->PreemptiveGCDisabled());
    
    DWORD dwBeginTickCount = GetTickCount();
    
    size_t prevNumFinalizableObjects = GetNumberFinalizableObjects();
    size_t curNumFinalizableObjects;
    BOOL fTimeOut = FALSE;
    DWORD nTry = 0;
    DWORD maxTry = (DWORD)(FINALIZER_TOTAL_WAIT*1.0/FINALIZER_WAIT_TIMEOUT + 0.5);
    DWORD maxTotalWait = (s_fRaiseExitProcessEvent?3000:40000);
    BOOL bAlertable = TRUE; //(g_fEEShutDown & ShutDown_Finalize2) ? FALSE:TRUE;

    static DWORD dwBreakOnFinalizeTimeOut = (DWORD) -1;
    if (dwBreakOnFinalizeTimeOut == (DWORD) -1) {
    dwBreakOnFinalizeTimeOut = g_pConfig->GetConfigDWORD(L"BreakOnFinalizeTimeOut", 0);
    }

    DWORD dwTimeout = FINALIZER_WAIT_TIMEOUT;

    // This used to set the dwTimeout to infinite, but this can cause a hang when shutting down
    // if a finalizer tries to take a lock that another suspended managed thread already has.
    // This results in the hang because the other managed thread is never going to be resumed
    // because we're in shutdown.  So we make a compromise here - make the timeout for every
    // iteration 10 times longer and make the total wait infinite - so if things hang we will
    // eventually shutdown but we also give things a chance to finish if they're running slower
    // because of the profiler.
#ifdef PROFILING_SUPPORTED
    if (CORProfilerPresent())
    {
        dwTimeout *= 10;
        maxTotalWait = INFINITE;
    }
#endif // PROFILING_SUPPORTED

    while (1) {
        DWORD status = 0;
        COMPLUS_TRY
        {
            status = pThread->DoAppropriateAptStateWait(1,&hEventFinalizerToShutDown,FALSE, dwTimeout, bAlertable);
        }
        COMPLUS_CATCH
        {
            status = WAIT_TIMEOUT;
        }
        COMPLUS_END_CATCH

        if (status != WAIT_TIMEOUT) {
            break;
        }
        nTry ++;
        curNumFinalizableObjects = GetNumberFinalizableObjects();
        if ((prevNumFinalizableObjects <= curNumFinalizableObjects || s_fRaiseExitProcessEvent)
#ifdef _DEBUG
            && gc_heap::more_space_lock.lock == -1
#else
            && gc_heap::more_space_lock == -1
#endif
            && !(pThread->m_State & (Thread::TS_UserSuspendPending | Thread::TS_DebugSuspendPending))){
            if (nTry == maxTry) {
                if (!s_fRaiseExitProcessEvent) {
                LOG((LF_GC, LL_INFO10, "Finalizer took too long on one object.\n"));
                }
                else
                    LOG((LF_GC, LL_INFO10, "Finalizer took too long to process ExitProcess event.\n"));

                fTimeOut = TRUE;
                if (dwBreakOnFinalizeTimeOut != 2) {
                    break;
                }
            }
        }
        else
        {
            nTry = 0;
            prevNumFinalizableObjects = curNumFinalizableObjects;
        }
        DWORD dwCurTickCount = GetTickCount();
        if (pThread->m_State & (Thread::TS_UserSuspendPending | Thread::TS_DebugSuspendPending)) {
            dwBeginTickCount = dwCurTickCount;
        }
        if (dwCurTickCount - dwBeginTickCount >= maxTotalWait
            || (dwBeginTickCount > dwCurTickCount && dwBeginTickCount - dwCurTickCount <= (~0) - maxTotalWait)) {
            LOG((LF_GC, LL_INFO10, "Finalizer took too long on shutdown.\n"));
            fTimeOut = TRUE;
            if (dwBreakOnFinalizeTimeOut != 2) {
                break;
            }
        }
    }

    if (fTimeOut) 
    {
        if (dwBreakOnFinalizeTimeOut){
            DebugBreak();
        }
        if (!s_fRaiseExitProcessEvent && s_FinalizeObjectName[0] != '\0') {
            LOG((LF_GC, LL_INFO10, "Currently running finalizer on object of %s\n", 
                 s_FinalizeObjectName));
        }
    }
    return fTimeOut;
}


void gc_heap::user_thread_wait (HANDLE event)
{
    Thread* pCurThread = GetThread();
    BOOL mode = pCurThread->PreemptiveGCDisabled();
    if (mode)
    {
        pCurThread->EnablePreemptiveGC();
    }

    WaitForSingleObject(event, INFINITE);

    if (mode)
    {
        pCurThread->DisablePreemptiveGC();
    }
}



#ifdef _DEBUG

// Normally, any thread we operate on has a Thread block in its TLS.  But there are
// a few special threads we don't normally execute managed code on.
//
// There is a scenario where we run managed code on such a thread, which is when the
// DLL_THREAD_ATTACH notification of an (IJW?) module calls into managed code.  This
// is incredibly dangerous.  If a GC is provoked, the system may have trouble performing
// the GC because its threads aren't available yet.                                  
static DWORD SpecialEEThreads[10];
static LONG  cnt_SpecialEEThreads = 0;

void dbgOnly_IdentifySpecialEEThread()
{
    LONG  ourCount = FastInterlockIncrement(&cnt_SpecialEEThreads);

    _ASSERTE(ourCount < (LONG) NumItems(SpecialEEThreads));
    SpecialEEThreads[ourCount-1] = ::GetCurrentThreadId();
}

BOOL dbgOnly_IsSpecialEEThread()
{
    DWORD   ourId = ::GetCurrentThreadId();

    for (LONG i=0; i<cnt_SpecialEEThreads; i++)
        if (ourId == SpecialEEThreads[i])
            return TRUE;

    return FALSE;
}

#endif // _DEBUG


// This function sets the card table with the granularity of 1 byte, to avoid ghost updates
//    that could occur if multiple threads were trying to set different bits in the same card.
// The alternative would be to use FastInterlockedOr, but that is too slow.
void ErectWriteBarrier(OBJECTREF *dst, OBJECTREF ref)
{
    // if the dst is outside of the heap (unboxed value classes) then we
    //      simply exit
    if (((*(BYTE**)&dst) < g_lowest_address) || ((*(BYTE**)&dst) >= g_highest_address))
        return;
    
#ifdef _DEBUG
    updateGCShadow((Object**) dst, OBJECTREFToObject(ref));     // support debugging write barrier
#endif
    
    // Do the Write Barrier thing
    setCardTableEntryInterlocked(*(BYTE**)&dst, *(BYTE**)&ref);
}        


/* These helpers basically replicate the ErectWriteBarrier code inline */

extern "C" HCIMPL2(VOID, JIT_CheckedWriteBarrier, Object **dst, Object *ref)
{
    *dst = ref;

    // if the dst is outside of the heap (unboxed value classes) then we
    //      simply exit
    if (((*(BYTE**)&dst) < g_lowest_address) || ((*(BYTE**)&dst) >= g_highest_address))
        return;
    
#ifdef _DEBUG
    updateGCShadow(dst, ref);     // support debugging write barrier
#endif
    
    // Do the Write Barrier thing
    setCardTableEntryInterlocked(*(BYTE**)&dst, *(BYTE**)&ref);
}
HCIMPLEND

// Use this version when you know that the destination field location is in the gc heap

extern "C" HCIMPL2(VOID, JIT_WriteBarrier, Object **dst, Object *ref)
{
    *dst = ref;

#ifdef _DEBUG
    updateGCShadow(dst, ref);     // support debugging write barrier
#endif
    
    // Do the Write Barrier thing
    setCardTableEntryInterlocked(*(BYTE**)&dst, *(BYTE**)&ref);
}
HCIMPLEND


HCIMPL3(VOID, JIT_StructWriteBarrier, void *dest, void* src, CORINFO_CLASS_HANDLE typeHnd_)
{
    TypeHandle typeHnd(typeHnd_);
    MethodTable *pMT = typeHnd.AsMethodTable();
    pMT->CheckRestore();    
    CopyValueClassUnchecked(dest, src, pMT);
}
HCIMPLEND

