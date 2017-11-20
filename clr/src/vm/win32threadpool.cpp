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
/*++

Module Name:

    Win32ThreadPool.cpp

Abstract:

    This module implements Threadpool support using Win32 APIs


Revision History:
    December 1999 -                                           - Created

--*/

#include "common.h"
#include "log.h"
#include "win32threadpool.h"
#include "delegateinfo.h"
#include "eeconfig.h"
#include "dbginterface.h"
#include "utilcode.h"


#define SPIN_COUNT 4000

#define INVALID_HANDLE ((HANDLE) -1)
#define NEW_THREAD_THRESHOLD            7       // Number of requests outstanding before we start a new thread

LONG ThreadpoolMgr::Initialization=0;           // indicator of whether the threadpool is initialized.
int ThreadpoolMgr::NumWorkerThreads=0;          // total number of worker threads created
int ThreadpoolMgr::MinLimitTotalWorkerThreads;              // = MaxLimitCPThreadsPerCPU * number of CPUS
int ThreadpoolMgr::MaxLimitTotalWorkerThreads;              // = MaxLimitCPThreadsPerCPU * number of CPUS
int ThreadpoolMgr::NumRunningWorkerThreads=0;   // = NumberOfWorkerThreads - no. of blocked threads
int ThreadpoolMgr::NumIdleWorkerThreads=0;
int ThreadpoolMgr::NumQueuedWorkRequests=0;     // number of queued work requests
int ThreadpoolMgr::LastRecordedQueueLength;	    // captured by GateThread, used on Win9x to detect thread starvation 
unsigned int ThreadpoolMgr::LastDequeueTime;	// used to determine if work items are getting thread starved 
//unsigned int ThreadpoolMgr::LastCompletionTime;	// used to determine if io completions are getting thread starved 
BOOL ThreadpoolMgr::MonitorWorkRequestsQueue=0; // if 1, the gate thread monitors progress of WorkRequestQueue to prevent starvation due to blocked worker threads


WorkRequest* ThreadpoolMgr::WorkRequestHead=NULL;        // Head of work request queue
WorkRequest* ThreadpoolMgr::WorkRequestTail=NULL;        // Head of work request queue

//unsigned int ThreadpoolMgr::LastCpuSamplingTime=0;	//  last time cpu utilization was sampled by gate thread
unsigned int ThreadpoolMgr::LastWorkerThreadCreation=0;	//  last time a worker thread was created
unsigned int ThreadpoolMgr::LastCPThreadCreation=0;		//  last time a completion port thread was created
unsigned int ThreadpoolMgr::NumberOfProcessors; // = NumberOfWorkerThreads - no. of blocked threads


CRITICAL_SECTION ThreadpoolMgr::WorkerCriticalSection;
HANDLE ThreadpoolMgr::WorkRequestNotification;


CRITICAL_SECTION ThreadpoolMgr::WaitThreadsCriticalSection;
ThreadpoolMgr::LIST_ENTRY ThreadpoolMgr::WaitThreadsHead;

CRITICAL_SECTION ThreadpoolMgr::EventCacheCriticalSection;
ThreadpoolMgr::LIST_ENTRY ThreadpoolMgr::EventCache;                       // queue of cached events
DWORD ThreadpoolMgr::NumUnusedEvents=0;                                    // number of events in cache

CRITICAL_SECTION ThreadpoolMgr::TimerQueueCriticalSection;
ThreadpoolMgr::LIST_ENTRY ThreadpoolMgr::TimerQueue;                       // queue of timers
DWORD ThreadpoolMgr::NumTimers=0;                                          // number of timers in timer queue
HANDLE ThreadpoolMgr::TimerThread=NULL;
DWORD ThreadpoolMgr::LastTickCount;                                                                            

LONG  ThreadpoolMgr::GateThreadCreated=0;                   // Set to 1 after the thread is created
long  ThreadpoolMgr::cpuUtilization=0;

unsigned ThreadpoolMgr::MaxCachedRecyledLists=40;			// don't cache freed memory after this (40 is arbitrary)
ThreadpoolMgr::RecycledListInfo ThreadpoolMgr::RecycledList[ThreadpoolMgr::MEMTYPE_COUNT];




// Macros for inserting/deleting from doubly linked list

#define InitializeListHead(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = (ListHead))

//
// these are named the same as slightly different macros in the NT headers
//
#undef RemoveHeadList
#undef RemoveEntryList
#undef InsertTailList
#undef InsertHeadList

#define RemoveHeadList(ListHead,FirstEntry) \
    {\
    FirstEntry = (LIST_ENTRY*) (ListHead)->Flink;\
    ((LIST_ENTRY*)FirstEntry->Flink)->Blink = (ListHead);\
    (ListHead)->Flink = FirstEntry->Flink;\
    }

#define RemoveEntryList(Entry) {\
    LIST_ENTRY* _EX_Entry;\
        _EX_Entry = (Entry);\
        ((LIST_ENTRY*) _EX_Entry->Blink)->Flink = _EX_Entry->Flink;\
        ((LIST_ENTRY*) _EX_Entry->Flink)->Blink = _EX_Entry->Blink;\
    }

#define InsertTailList(ListHead,Entry) \
    (Entry)->Flink = (ListHead);\
    (Entry)->Blink = (ListHead)->Blink;\
    ((LIST_ENTRY*)(ListHead)->Blink)->Flink = (Entry);\
    (ListHead)->Blink = (Entry);

#define InsertHeadList(ListHead,Entry) {\
    LIST_ENTRY* _EX_Flink;\
    LIST_ENTRY* _EX_ListHead;\
    _EX_ListHead = (LIST_ENTRY*)(ListHead);\
    _EX_Flink = (LIST_ENTRY*) _EX_ListHead->Flink;\
    (Entry)->Flink = _EX_Flink;\
    (Entry)->Blink = _EX_ListHead;\
    _EX_Flink->Blink = (Entry);\
    _EX_ListHead->Flink = (Entry);\
    }

#define IsListEmpty(ListHead) \
    ((ListHead)->Flink == (ListHead))
/************************************************************************/
void ThreadpoolMgr::EnsureInitialized()
{
    if (IsInitialized())
        return;

    if (InterlockedCompareExchange(&Initialization, 1, 0) == 0)
    {
        Initialize();
        Initialization = -1;
    }
    else // someone has already begun initializing. 
    {
        // just wait until it finishes
        while (Initialization != -1)
            __SwitchToThread(0);
    }
}
//#define PRIVATE_BUILD 

void ThreadpoolMgr::Initialize()
{
    NumberOfProcessors = GetCurrentProcessCpuCount(); 
	InitPlatformVariables();

	{
		InitializeCriticalSection( &WorkerCriticalSection );
		InitializeCriticalSection( &WaitThreadsCriticalSection );
		InitializeCriticalSection( &EventCacheCriticalSection );
		InitializeCriticalSection( &TimerQueueCriticalSection );
	}

    // initialize WaitThreadsHead
    InitializeListHead(&WaitThreadsHead);

    // initialize EventCache
    InitializeListHead(&EventCache);

    // initialize TimerQueue
    InitializeListHead(&TimerQueue);

    WorkRequestNotification = WszCreateEvent(NULL, // security attributes
                                          TRUE, // manual reset
                                          FALSE, // initial state
                                          NULL);
    _ASSERTE(WorkRequestNotification != NULL);

    // initialize Worker thread settings
    MaxLimitTotalWorkerThreads = NumberOfProcessors*MaxLimitCPThreadsPerCPU;
#ifdef _DEBUG
    MaxLimitTotalWorkerThreads = EEConfig::GetConfigDWORD(L"MaxThreadpoolThreads",MaxLimitTotalWorkerThreads);
#endif

    MinLimitTotalWorkerThreads = NumberOfProcessors; // > 1 ? NumberOfProcessors : 2;
#ifdef PRIVATE_BUILD
    MinLimitTotalWorkerThreads = EEConfig::GetConfigDWORD(L"MinWorkerThreads", NumberOfProcessors);
#endif

    // initialize recyleLists
    for (unsigned i = 0; i < MEMTYPE_COUNT; i++)
    {
        RecycledList[i].root   = NULL;
        RecycledList[i].tag    = 0;
        RecycledList[i].count  = 0;
    }
    MaxCachedRecyledLists *= NumberOfProcessors;

}




void ThreadpoolMgr::InitPlatformVariables()
{

}

/************************************************************************/
BOOL ThreadpoolMgr::SetMaxThreads(DWORD MaxWorkerThreads, 
                                     DWORD MaxIOCompletionThreads)
{

    if (IsInitialized())
    {
        BOOL result = FALSE;
        
        if (MaxWorkerThreads >= (DWORD) NumWorkerThreads)
        {
            MaxLimitTotalWorkerThreads = MaxWorkerThreads;
            result = TRUE;
        }

        return result;
    }

    if (InterlockedCompareExchange(&Initialization, 1, 0) == 0)
	{
		Initialize();

        BOOL result = FALSE;

        if (MaxWorkerThreads >= (DWORD) MinLimitTotalWorkerThreads) 
        {
            MaxLimitTotalWorkerThreads = MaxWorkerThreads;
            result = TRUE;
        }

		Initialization = -1;

        return result;
	}
    else // someone else is initializing. Too late, return false
    {
        return FALSE;
    }

}

BOOL ThreadpoolMgr::GetMaxThreads(DWORD* MaxWorkerThreads, 
                                     DWORD* MaxIOCompletionThreads)
{
    if (IsInitialized())
    {
        *MaxWorkerThreads = MaxLimitTotalWorkerThreads;
        *MaxIOCompletionThreads = 0;
    }
    else
    {
        NumberOfProcessors = GetCurrentProcessCpuCount(); 
        *MaxWorkerThreads = NumberOfProcessors*MaxLimitCPThreadsPerCPU;
        *MaxIOCompletionThreads = 0;
    }
    return TRUE;
}
    
BOOL ThreadpoolMgr::GetAvailableThreads(DWORD* AvailableWorkerThreads, 
                                        DWORD* AvailableIOCompletionThreads)
{
    if (IsInitialized())
    {
        *AvailableWorkerThreads = (MaxLimitTotalWorkerThreads - NumWorkerThreads)  /*threads yet to be created */
                                   + NumIdleWorkerThreads;
        *AvailableIOCompletionThreads = 0;
    }
    else
    {
        GetMaxThreads(AvailableWorkerThreads,AvailableIOCompletionThreads);
    }
    return TRUE;
}


/************************************************************************/

BOOL ThreadpoolMgr::QueueUserWorkItem(LPTHREAD_START_ROUTINE Function, 
                                      PVOID Context,
                                      DWORD Flags)
{
    EnsureInitialized();

    BOOL status;


    LOCKCOUNTINCL("QueueUserWorkItem in win32ThreadPool.h");
    EnterCriticalSection (&WorkerCriticalSection) ;

    status = EnqueueWorkRequest(Function, Context);

    if (status)
    {
        _ASSERTE(NumQueuedWorkRequests > 0);

        // see if we need to grow the worker thread pool, but don't bother if GC is in progress
        if (ShouldGrowWorkerThreadPool() &&
            !(g_pGCHeap->IsGCInProgress()
#ifdef _DEBUG
#ifdef STRESS_HEAP
              && g_pConfig->GetGCStressLevel() == 0
#endif
#endif
              ))
        {
            status = CreateWorkerThread();
        }
        else
        // else we did not grow the worker pool, so make sure there is a gate thread
        // that monitors the WorkRequest queue and spawns new threads if no progress is
        // being made
        {
            if (!GateThreadCreated)
                CreateGateThread();
            MonitorWorkRequestsQueue = 1;
        }
    }


    LeaveCriticalSection (&WorkerCriticalSection) ;
    LOCKCOUNTDECL("QueueUserWorkItem in win32ThreadPool.h");

    return status;
}

//************************************************************************
BOOL ThreadpoolMgr::EnqueueWorkRequest(LPTHREAD_START_ROUTINE Function, 
                                       PVOID Context)
{
    WorkRequest* workRequest = MakeWorkRequest(Function, Context);
    if (workRequest == NULL)
        return FALSE;
	LOG((LF_THREADPOOL ,LL_INFO100 ,"Enqueue work request (Function= %x, Context = %x)\n", Function, Context));
    AppendWorkRequest(workRequest);
    SetEvent(WorkRequestNotification);
    return TRUE;
}

WorkRequest* ThreadpoolMgr::DequeueWorkRequest()
{
    WorkRequest* entry = RemoveWorkRequest();
    if (NumQueuedWorkRequests == 0)
        ResetEvent(WorkRequestNotification);
    if (entry)
    {
        LastDequeueTime = GetTickCount();
#ifdef _DEBUG
        LOG((LF_THREADPOOL ,LL_INFO100 ,"Dequeue work request (Function= %x, Context = %x)\n", entry->Function, entry->Context));
#endif
    }
    return entry;
}

void ThreadpoolMgr::ExecuteWorkRequest(WorkRequest* workRequest)
{
    LPTHREAD_START_ROUTINE wrFunction = workRequest->Function;
    LPVOID                 wrContext  = workRequest->Context;

    PAL_TRY
    {
        // First delete the workRequest then call the function to 
        // prevent leaks in apps that call functions that never return

        LOG((LF_THREADPOOL ,LL_INFO100 ,"Starting work request (Function= %x, Context = %x)\n", wrFunction, wrContext));

        RecycleMemory((LPVOID*)workRequest, MEMTYPE_WorkRequest); //delete workRequest;
        (wrFunction)(wrContext);

        LOG((LF_THREADPOOL ,LL_INFO100 ,"Finished work request (Function= %x, Context = %x)\n", wrFunction, wrContext));
    }
    PAL_EXCEPT_FILTER(DefaultCatchFilter, COMPLUS_EXCEPTION_EXECUTE_HANDLER)
    {
        LOG((LF_THREADPOOL ,LL_INFO100 ,"Unhandled exception from work request (Function= %x, Context = %x)\n", wrFunction, wrContext));
        //_ASSERTE(!"FALSE");
    }
    PAL_ENDTRY
}


// Remove a block from the appropriate recycleList and return.
// If recycleList is empty, fall back to new.
LPVOID ThreadpoolMgr::GetRecycledMemory(enum MemType memType)
{
    switch (memType)
    {
        case MEMTYPE_DelegateInfo: 
            return new DelegateInfo;
        case MEMTYPE_AsyncCallback:
            return new AsyncCallback;
        case MEMTYPE_WorkRequest:
            return new WorkRequest;
        default:
            _ASSERTE(!"Unknown Memtype");
            return 0;
	}
}

// Insert freed block in recycle list. If list is full, return to system heap
void ThreadpoolMgr::RecycleMemory(LPVOID* mem, enum MemType memType)
{

    switch (memType)
    {
        case MEMTYPE_DelegateInfo: 
            delete (DelegateInfo*) mem;
            break;
        case MEMTYPE_AsyncCallback:
            delete (AsyncCallback*) mem;
            break;
        case MEMTYPE_WorkRequest:
            delete (WorkRequest*) mem;
            break;
        default:
            _ASSERTE(!"Unknown Memtype");

    }
}

//************************************************************************


BOOL ThreadpoolMgr::ShouldGrowWorkerThreadPool()
{
    // we only want to grow the worker thread pool if there are less than n threads, where n= no. of processors
    // and more requests than the number of idle threads and GC is not in progress
    return (NumRunningWorkerThreads < MinLimitTotalWorkerThreads&& //(int) NumberOfProcessors &&
            NumIdleWorkerThreads < NumQueuedWorkRequests &&
            NumWorkerThreads < MaxLimitTotalWorkerThreads); 

}

/* If threadId is the id of a worker thread, reduce the number of running worker threads,
   grow the threadpool if necessary, and return TRUE. Otherwise do nothing and return false*/
BOOL  ThreadpoolMgr::ThreadAboutToBlock(Thread* pThread)
{
    BOOL isWorkerThread = pThread->IsWorkerThread();

    if (isWorkerThread)
    {
        LOCKCOUNTINCL("ThreadAboutToBlock in win32ThreadPool.h");                       \
		LOG((LF_THREADPOOL ,LL_INFO1000 ,"Thread about to block\n"));
    
        EnterCriticalSection (&WorkerCriticalSection) ;

        _ASSERTE(NumRunningWorkerThreads > 0);
        NumRunningWorkerThreads--;
        if (ShouldGrowWorkerThreadPool())
        {
            CreateWorkerThread();
        }
        LeaveCriticalSection(&WorkerCriticalSection) ;

        LOCKCOUNTDECL("ThreadAboutToBlock in win32ThreadPool.h");
    }
    
    return isWorkerThread;

}

/* Must be balanced by a previous call to ThreadAboutToBlock that returned TRUE*/
void ThreadpoolMgr::ThreadAboutToUnblock()
{
    LOCKCOUNTINCL("ThreadAboutToUnBlock in win32ThreadPool.h");  \
    EnterCriticalSection (&WorkerCriticalSection) ;
    _ASSERTE(NumRunningWorkerThreads < NumWorkerThreads);
    NumRunningWorkerThreads++;
    LeaveCriticalSection(&WorkerCriticalSection) ;
    LOCKCOUNTDECL("ThreadAboutToUnBlock in win32ThreadPool.h"); \
    LOG((LF_THREADPOOL ,LL_INFO1000 ,"Thread unblocked\n"));

}

#define THROTTLE_RATE  0.10 /* rate by which we increase the delay as number of threads increase */


// On Win9x, there are no apis to get cpu utilization, so we fall back on 
// other heuristics
void ThreadpoolMgr::GrowWorkerThreadPoolIfStarvationSimple()
{

	if (NumQueuedWorkRequests == 0 ||
        NumWorkerThreads == MaxLimitTotalWorkerThreads)
        return;

	int  lastQueueLength = LastRecordedQueueLength;
	LastRecordedQueueLength = NumQueuedWorkRequests;

	// else, we have queued work items, and we haven't
	// hit the upper limit on the number of worker threads

	// if the queue length has decreased since our last time
	// or not enough delay since we last created a worker thread
	if ((NumQueuedWorkRequests < lastQueueLength) ||
		!SufficientDelaySinceLastSample(LastWorkerThreadCreation,NumWorkerThreads, THROTTLE_RATE))
		return;


	LOCKCOUNTINCL("GrowWorkerThreadPoolIfStarvation in win32ThreadPool.h");						\
	EnterCriticalSection (&WorkerCriticalSection) ;
	if ((NumQueuedWorkRequests >= lastQueueLength) && (NumWorkerThreads < MaxLimitTotalWorkerThreads))
		CreateWorkerThread();
	LeaveCriticalSection(&WorkerCriticalSection) ;
	LOCKCOUNTDECL("GrowWorkerThreadPoolIfStarvation in win32ThreadPool.h");						\

}

HANDLE ThreadpoolMgr::CreateUnimpersonatedThread(LPTHREAD_START_ROUTINE lpStartAddress)
{
    DWORD threadId;


	return CreateThread(NULL,                // security descriptor
                                       0,                   // default stack size
                        lpStartAddress,   // 
                                       NULL,                //no arguments
                                       CREATE_SUSPENDED,    // start immediately
                                       &threadId);

}

BOOL ThreadpoolMgr::CreateWorkerThread()
{
    HANDLE threadHandle = CreateUnimpersonatedThread(WorkerThreadStart);

    if (threadHandle)
    {
		LastWorkerThreadCreation = GetTickCount();	// record this for use by logic to spawn additional threads

        _ASSERTE(NumWorkerThreads >= NumRunningWorkerThreads);
        NumRunningWorkerThreads++;
        NumWorkerThreads++;
        NumIdleWorkerThreads++;
		LOG((LF_THREADPOOL ,LL_INFO100 ,"Worker thread created (NumWorkerThreads=%d\n)",NumWorkerThreads));

#ifdef _DEBUG
        DWORD status =
#endif
        ResumeThread(threadHandle);
        _ASSERTE(status != (DWORD) (-1));
        CloseHandle(threadHandle);          // we don't need this anymore
    }
    // dont return failure if we have at least one running thread, since we can service the request 
    return (NumRunningWorkerThreads > 0);
}

#ifdef _MSC_VER
#pragma warning(disable:4702)       // Unreachable code
#endif
DWORD __stdcall ThreadpoolMgr::WorkerThreadStart(LPVOID lpArgs)
{
    #define IDLE_WORKER_TIMEOUT (40*1000) // milliseonds
    DWORD SleepTime = IDLE_WORKER_TIMEOUT;

	LOG((LF_THREADPOOL ,LL_INFO1000 ,"Worker thread started\n"));


    for (;;)
    {
        _ASSERTE(NumRunningWorkerThreads > 0);
        DWORD status = WaitForSingleObject(WorkRequestNotification,SleepTime);
        _ASSERTE(status == WAIT_TIMEOUT || status == WAIT_OBJECT_0);
        
        BOOL shouldTerminate = FALSE;

        if ( status == WAIT_TIMEOUT )
        {
        // The thread terminates if there are > 1 threads and the queue is small
        // OR if there is only 1 thread and there is no request pending
            if (NumWorkerThreads > 1)
            {
                ULONG Threshold = NEW_THREAD_THRESHOLD * (NumRunningWorkerThreads-1);


                if (NumQueuedWorkRequests < (int) Threshold)
                {
                    shouldTerminate = !IsIoPending(); // do not terminate if there is pending io on this thread
                }
                else
                {
                    SleepTime <<= 1 ;
                    SleepTime += 1000; // to prevent wraparound to 0
                }
            }   
            else // this is the only worker thread
            {
                if (NumQueuedWorkRequests == 0)
                {
                    // delay termination of last thread
                    if (SleepTime < 4*IDLE_WORKER_TIMEOUT) 
                    {
                        SleepTime <<= 1 ;
                        SleepTime += 1000; // to prevent wraparound to 0
                    }
                    else
                    {
                        shouldTerminate = !IsIoPending(); // do not terminate if there is pending io on this thread
                    }
                }
            }


            if (shouldTerminate)
            {   // recheck NumQueuedWorkRequest since a new one may have arrived while we are checking it
                LOCKCOUNTINCL("WorkerThreadStart in win32ThreadPool.h");                        \
                EnterCriticalSection (&WorkerCriticalSection) ;
                if (NumQueuedWorkRequests == 0)
                {   
                    // it really is zero, so terminate this thread
                    NumRunningWorkerThreads--;
                    NumWorkerThreads--;     // protected by WorkerCriticalSection
                    NumIdleWorkerThreads--; //   ditto
                    _ASSERTE(NumRunningWorkerThreads >= 0 && NumWorkerThreads >= 0 && NumIdleWorkerThreads >= 0);

                    LeaveCriticalSection(&WorkerCriticalSection);
                    LOCKCOUNTDECL("WorkerThreadStart in win32ThreadPool.h");                        \

					LOG((LF_THREADPOOL ,LL_INFO100 ,"Worker thread terminated (NumWorkerThreads=%d)\n",NumWorkerThreads));

                    ExitThread(0);
                }
                else
                {
                    LeaveCriticalSection (&WorkerCriticalSection) ;
                    LOCKCOUNTDECL("WorkerThreadStart in win32ThreadPool.h");                        \

                    continue;
                }
            }
        }
        else
        {
            // woke up because of a new work request arrival
            WorkRequest* workRequest;
            LOCKCOUNTINCL("WorkerThreadStart in win32ThreadPool.h");                        \
            EnterCriticalSection (&WorkerCriticalSection) ;

            if ( ( workRequest = DequeueWorkRequest() ) != NULL)
            {
                _ASSERTE(NumIdleWorkerThreads > 0);
                NumIdleWorkerThreads--; // we found work, decrease the number of idle threads
            }

            // the dequeue operation also resets the WorkRequestNotification event

            LeaveCriticalSection(&WorkerCriticalSection);
            LOCKCOUNTDECL("WorkerThreadStart in win32ThreadPool.h");                        \

            while (workRequest)
            {
                ExecuteWorkRequest(workRequest);

                LOCKCOUNTINCL("WorkerThreadStart in win32ThreadPool.h");                        \
                EnterCriticalSection (&WorkerCriticalSection) ;

                workRequest = DequeueWorkRequest();
                // the dequeue operation resets the WorkRequestNotification event

                if (workRequest == NULL)
                {
                    NumIdleWorkerThreads++; // no more work, increase the number of idle threads
                }

                LeaveCriticalSection(&WorkerCriticalSection);
                LOCKCOUNTDECL("WorkerThreadStart in win32ThreadPool.h");                        \

            } 

        }

    } // for(;;)

    return 0;

}
#ifdef _MSC_VER
#pragma warning(default:4702)
#endif

/************************************************************************/

BOOL ThreadpoolMgr::RegisterWaitForSingleObject(PHANDLE phNewWaitObject,
                                                HANDLE hWaitObject,
                                                WAITORTIMERCALLBACK Callback,
                                                PVOID Context,
                                                ULONG timeout,
                                                DWORD dwFlag )
{
    EnsureInitialized();

    LOCKCOUNTINCL("RegisterWaitForSingleObject in win32ThreadPool.h");                      \
    EnterCriticalSection(&WaitThreadsCriticalSection);

    ThreadCB* threadCB = FindWaitThread();
        
    LeaveCriticalSection(&WaitThreadsCriticalSection);
    LOCKCOUNTDECL("RegisterWaitForSingleObject in win32ThreadPool.h");                      \

    *phNewWaitObject = NULL;

    if (threadCB)
    {
        WaitInfo* waitInfo = new WaitInfo;
        
        if (waitInfo == NULL)
            return FALSE;

        waitInfo->waitHandle = hWaitObject;
        waitInfo->Callback = Callback;
        waitInfo->Context = Context;
        waitInfo->timeout = timeout;
        waitInfo->flag = dwFlag;
        waitInfo->threadCB = threadCB;
        waitInfo->state = 0;
        waitInfo->refCount = 1;     // safe to do this since no wait has yet been queued, so no other thread could be modifying this
        waitInfo->CompletionEvent = INVALID_HANDLE;
        waitInfo->PartialCompletionEvent = INVALID_HANDLE;

        waitInfo->timer.startTime = ::GetTickCount();
        waitInfo->timer.remainingTime = timeout;

        *phNewWaitObject = waitInfo;

        LOG((LF_THREADPOOL ,LL_INFO100 ,"Registering wait for handle %x, Callback=%x, Context=%x \n",
                                        hWaitObject, Callback, Context));

        QueueUserAPC((PAPCFUNC)InsertNewWaitForSelf, threadCB->threadHandle, (size_t) waitInfo);

        return TRUE;
    }

    return FALSE;
}


// Returns a wait thread that can accomodate another wait request. The 
// caller is responsible for synchronizing access to the WaitThreadsHead 
ThreadpoolMgr::ThreadCB* ThreadpoolMgr::FindWaitThread()
{
    do 
    {
        for (LIST_ENTRY* Node = (LIST_ENTRY*) WaitThreadsHead.Flink ; 
             Node != &WaitThreadsHead ; 
             Node = (LIST_ENTRY*)Node->Flink) 
        {
            _ASSERTE(offsetof(WaitThreadInfo,link) == 0);

            ThreadCB*  threadCB = ((WaitThreadInfo*) Node)->threadCB;
        
            if (threadCB->NumWaitHandles < MAX_WAITHANDLES)         // the test and
            {
                InterlockedIncrement(&threadCB->NumWaitHandles);        // increment are protected by WaitThreadsCriticalSection.
                                                                        // but there might be a concurrent decrement in DeactivateWait, hence the interlock
                return threadCB;
            }
        }

        // if reached here, there are no wait threads available, so need to create a new one
        if (!CreateWaitThread())
            return NULL;


        // Now loop back
    } while (TRUE);

}

BOOL ThreadpoolMgr::CreateWaitThread()
{
    DWORD threadId;

    WaitThreadInfo* waitThreadInfo = new WaitThreadInfo;
    if (waitThreadInfo == NULL)
        return FALSE;
        
    ThreadCB* threadCB = new ThreadCB;

    if (threadCB == NULL)
    {
        delete waitThreadInfo;
        return FALSE;
    }

    HANDLE threadHandle = CreateThread(NULL,                // security descriptor
                                       0,                   // default stack size
                                       WaitThreadStart,     // 
                                       (LPVOID) threadCB,   // thread control block is passed as argument
                                       CREATE_SUSPENDED,    // start immediately
                                       &threadId);

    if (threadHandle == NULL)
    {
        return FALSE;
    }

    threadCB->threadHandle = threadHandle;      
    threadCB->threadId = threadId;              // may be useful for debugging otherwise not used
    threadCB->NumWaitHandles = 0;
    threadCB->NumActiveWaits = 0;
    for (int i=0; i< MAX_WAITHANDLES; i++)
    {
        InitializeListHead(&(threadCB->waitPointer[i]));
    }

    waitThreadInfo->threadCB = threadCB;

    InsertHeadList(&WaitThreadsHead,&waitThreadInfo->link);

    DWORD status = ResumeThread(threadHandle);
    _ASSERTE(status != (DWORD) (-1));

	LOG((LF_THREADPOOL ,LL_INFO100 ,"Created wait thread \n"));

    return (status != (DWORD) (-1));

}

// Executed as an APC on a WaitThread. Add the wait specified in pArg to the list of objects it is waiting on
void ThreadpoolMgr::InsertNewWaitForSelf(WaitInfo* pArgs)
{
	WaitInfo* waitInfo = pArgs;

    // the following is safe since only this thread is allowed to change the state
    if (!(waitInfo->state & WAIT_DELETE))
    {
        waitInfo->state =  (WAIT_REGISTERED | WAIT_ACTIVE);
    }
    else 
    {
        // some thread unregistered the wait  
        DeleteWait(waitInfo);
        return;
    }

 
    ThreadCB* threadCB = waitInfo->threadCB;

    _ASSERTE(threadCB->NumActiveWaits < threadCB->NumWaitHandles);

    int index = FindWaitIndex(threadCB, waitInfo->waitHandle);
    _ASSERTE(index >= 0 && index <= threadCB->NumActiveWaits);

    if (index == threadCB->NumActiveWaits)
    {
        threadCB->waitHandle[threadCB->NumActiveWaits] = waitInfo->waitHandle;
        threadCB->NumActiveWaits++;
    }

    _ASSERTE(offsetof(WaitInfo, link) == 0);
    InsertTailList(&(threadCB->waitPointer[index]), (&waitInfo->link));
    
    return;
}

// returns the index of the entry that matches waitHandle or next free entry if not found
int ThreadpoolMgr::FindWaitIndex(const ThreadCB* threadCB, const HANDLE waitHandle)
{
    for (int i=0;i<threadCB->NumActiveWaits; i++)
        if (threadCB->waitHandle[i] == waitHandle)
            return i;

    // else not found
    return threadCB->NumActiveWaits;
}


// if no wraparound that the timer is expired if duetime is less than current time
// if wraparound occurred, then the timer expired if dueTime was greater than last time or dueTime is less equal to current time
#define TimeExpired(last,now,duetime) (last <= now ? \
                                       duetime <= now : \
                                       (duetime >= last || duetime <= now))

#define TimeInterval(end,start) ( end > start ? (end - start) : ((0xffffffff - start) + end + 1)   )

// Returns the minimum of the remaining time to reach a timeout among all the waits
DWORD ThreadpoolMgr::MinimumRemainingWait(LIST_ENTRY* waitInfo, unsigned int numWaits)
{
    unsigned int min = (unsigned int) -1;
    DWORD currentTime = ::GetTickCount();

    for (unsigned i=0; i < numWaits ; i++)
    {
        WaitInfo* waitInfoPtr = (WaitInfo*) (waitInfo[i].Flink);
        PVOID waitInfoHead = &(waitInfo[i]);
        do
        {
            if (waitInfoPtr->timeout != INFINITE)
            {
                // compute remaining time
                DWORD elapsedTime = TimeInterval(currentTime,waitInfoPtr->timer.startTime );

                __int64 remainingTime = (__int64) (waitInfoPtr->timeout) - (__int64) elapsedTime;

                // update remaining time
                waitInfoPtr->timer.remainingTime =  remainingTime > 0 ? (int) remainingTime : 0; 
                
                // ... and min
                if (waitInfoPtr->timer.remainingTime < min)
                    min = waitInfoPtr->timer.remainingTime;
            }

            waitInfoPtr = (WaitInfo*) (waitInfoPtr->link.Flink);

        } while ((PVOID) waitInfoPtr != waitInfoHead);

    } 
    return min;
}

#ifdef _MSC_VER
#pragma warning (disable : 4715)
#endif
DWORD __stdcall ThreadpoolMgr::WaitThreadStart(LPVOID lpArgs)
{
    ThreadCB* threadCB = (ThreadCB*) lpArgs;
    // wait threads never die. (Why?)
    for (;;) 
    {
        DWORD status;
        DWORD timeout = 0;

        if (threadCB->NumActiveWaits == 0)
        {
            status = SleepEx(INFINITE,TRUE);

            _ASSERTE(status == WAIT_IO_COMPLETION);
        }
        else
        {
            // compute minimum timeout. this call also updates the remainingTime field for each wait
            timeout = MinimumRemainingWait(threadCB->waitPointer,threadCB->NumActiveWaits);

            status = WaitForMultipleObjectsEx(  threadCB->NumActiveWaits,
                                                threadCB->waitHandle,
                                                FALSE,                      // waitall
                                                timeout,
                                                TRUE  );                    // alertable

            _ASSERTE( (status == WAIT_TIMEOUT) ||
                      (status == WAIT_IO_COMPLETION) ||
                      (status >= WAIT_OBJECT_0 && status < (DWORD)(WAIT_OBJECT_0 + threadCB->NumActiveWaits))  ||
                      (status == WAIT_FAILED));
        }

        if (status == WAIT_IO_COMPLETION)
            continue;

        if (status == WAIT_TIMEOUT)
        {
            for (int i=0; i< threadCB->NumActiveWaits; i++)
            {
                WaitInfo* waitInfo = (WaitInfo*) (threadCB->waitPointer[i]).Flink;
                PVOID waitInfoHead = &(threadCB->waitPointer[i]);
                    
                do 
                {
                    _ASSERTE(waitInfo->timer.remainingTime >= timeout);

                    WaitInfo* wTemp = (WaitInfo*) waitInfo->link.Flink;

                    if (waitInfo->timer.remainingTime == timeout)
                    {
                        ProcessWaitCompletion(waitInfo,i,TRUE); 
                    }

                    waitInfo = wTemp;

                } while ((PVOID) waitInfo != waitInfoHead);
            }
        }
        else if (status >= WAIT_OBJECT_0 && status < (DWORD)(WAIT_OBJECT_0 + threadCB->NumActiveWaits))
        {
            unsigned index = status - WAIT_OBJECT_0;
            WaitInfo* waitInfo = (WaitInfo*) (threadCB->waitPointer[index]).Flink;
            PVOID waitInfoHead = &(threadCB->waitPointer[index]);
			BOOL isAutoReset;
				isAutoReset = (WaitForSingleObject(threadCB->waitHandle[index],0) == WAIT_TIMEOUT);

            do 
            {
                WaitInfo* wTemp = (WaitInfo*) waitInfo->link.Flink;
                ProcessWaitCompletion(waitInfo,index,FALSE);

                waitInfo = wTemp;

            } while (((PVOID) waitInfo != waitInfoHead) && !isAutoReset);

			// If an app registers a recurring wait for an event that is always signalled (!), 
			// then no apc's will be executed since the thread never enters the alertable state.
			// This can be fixed by doing the following:
			//     SleepEx(0,TRUE);
			// However, it causes an unnecessary context switch. It is not worth penalizing well
			// behaved apps to protect poorly written apps. 
				

        }
        else
        {
            _ASSERTE(status == WAIT_FAILED);
            // wait failed: application error 
            // find out which wait handle caused the wait to fail
            for (int i = 0; i < threadCB->NumActiveWaits; i++)
            {
                DWORD subRet = WaitForSingleObject(threadCB->waitHandle[i], 0);

                if (subRet != WAIT_FAILED)
                    continue;

                // remove all waits associated with this wait handle

                WaitInfo* waitInfo = (WaitInfo*) (threadCB->waitPointer[i]).Flink;
                PVOID waitInfoHead = &(threadCB->waitPointer[i]);

                do
                {
                    WaitInfo* temp  = (WaitInfo*) waitInfo->link.Flink;

                    DeactivateNthWait(waitInfo,i);


		    // Note, we cannot cleanup here since there is no way to suppress finalization
		    // we will just leak, and rely on the finalizer to clean up the memory
                    //if (InterlockedDecrement(&waitInfo->refCount) == 0)
                    //    DeleteWait(waitInfo);


                    waitInfo = temp;

                } while ((PVOID) waitInfo != waitInfoHead);

                break;
            }
        }
    }
}
#ifdef _MSC_VER
#pragma warning (default : 4715)
#endif

void ThreadpoolMgr::ProcessWaitCompletion(WaitInfo* waitInfo,
                                          unsigned index,
                                          BOOL waitTimedOut
                                         )
{
    if ( waitInfo->flag & WAIT_SINGLE_EXECUTION) 
    {
        DeactivateNthWait (waitInfo,index) ;
    }
    else
    {   // reactivate wait by resetting timer
        waitInfo->timer.startTime = GetTickCount();
    }

    AsyncCallback* asyncCallback = MakeAsyncCallback();
    if (asyncCallback)
    {
        asyncCallback->wait = waitInfo;
        asyncCallback->waitTimedOut = waitTimedOut;

        InterlockedIncrement(&waitInfo->refCount);

        if (FALSE == QueueUserWorkItem(AsyncCallbackCompletion,asyncCallback,0))
        {
            RecycleMemory((LPVOID*)asyncCallback, MEMTYPE_AsyncCallback); //delete asyncCallback;

            if (InterlockedDecrement(&waitInfo->refCount) == 0)
                DeleteWait(waitInfo);

        }
    }
}


DWORD __stdcall ThreadpoolMgr::AsyncCallbackCompletion(PVOID pArgs)
{
    AsyncCallback* asyncCallback = (AsyncCallback*) pArgs;

    WaitInfo* waitInfo = asyncCallback->wait;

	LOG((LF_THREADPOOL ,LL_INFO100 ,"Doing callback, Function= %x, Context= %x, Timeout= %2d\n",
		waitInfo->Callback, waitInfo->Context,asyncCallback->waitTimedOut));

    ((WAITORTIMERCALLBACKFUNC) waitInfo->Callback) 
                                ( waitInfo->Context, asyncCallback->waitTimedOut);

    RecycleMemory((LPVOID*) asyncCallback, MEMTYPE_AsyncCallback); //delete asyncCallback;

	// if this was a single execution, we now need to stop rooting registeredWaitHandle  
	// in a GC handle. This will cause the finalizer to pick it up and call the cleanup
	// routine.
	if ( (waitInfo->flag & WAIT_SINGLE_EXECUTION)  && (waitInfo->flag & WAIT_FREE_CONTEXT))
	{

		DelegateInfo* pDelegate = (DelegateInfo*) waitInfo->Context;
		
		_ASSERTE(pDelegate->m_registeredWaitHandle);
		
		if (SystemDomain::GetAppDomainAtId(pDelegate->m_appDomainId))
			// if no domain then handle already gone or about to go.
			StoreObjectInHandle(pDelegate->m_registeredWaitHandle, NULL);
	}

    if (InterlockedDecrement(&waitInfo->refCount) == 0)
	{
		// the wait has been unregistered, so safe to delete it
        DeleteWait(waitInfo);
	}

    return 0; // ignored
}

void ThreadpoolMgr::DeactivateWait(WaitInfo* waitInfo)
{
    ThreadCB* threadCB = waitInfo->threadCB;
    DWORD endIndex = threadCB->NumActiveWaits-1;
    DWORD index;

    for (index = 0;  index <= endIndex; index++) 
    {
        LIST_ENTRY* head = &(threadCB->waitPointer[index]);
        LIST_ENTRY* current = head;
        do {
            if (current->Flink == (PVOID) waitInfo)
                goto FOUND;

            current = (LIST_ENTRY*) current->Flink;

        } while (current != head);
    }

FOUND:
    _ASSERTE(index <= endIndex);

    DeactivateNthWait(waitInfo, index);
}


void ThreadpoolMgr::DeactivateNthWait(WaitInfo* waitInfo, DWORD index)
{

    ThreadCB* threadCB = waitInfo->threadCB;

    if (waitInfo->link.Flink != waitInfo->link.Blink)
    {
        RemoveEntryList(&(waitInfo->link));
    }
    else
    {

        ULONG EndIndex = threadCB->NumActiveWaits -1;

        // Move the remaining ActiveWaitArray left.

        ShiftWaitArray( threadCB, index+1, index,EndIndex - index ) ;

        // repair the blink and flink of the first and last elements in the list
        for (unsigned int i = 0; i< EndIndex-index; i++)
        {
            WaitInfo* firstWaitInfo = (WaitInfo*) threadCB->waitPointer[index+i].Flink;
            WaitInfo* lastWaitInfo = (WaitInfo*) threadCB->waitPointer[index+i].Blink;
            firstWaitInfo->link.Blink =  &(threadCB->waitPointer[index+i]);
            lastWaitInfo->link.Flink =  &(threadCB->waitPointer[index+i]);
        }
        // initialize the entry just freed
        InitializeListHead(&(threadCB->waitPointer[EndIndex]));

        threadCB->NumActiveWaits-- ;
        InterlockedDecrement(&threadCB->NumWaitHandles);
    }

    waitInfo->state &= ~WAIT_ACTIVE ;

}

void ThreadpoolMgr::DeleteWait(WaitInfo* waitInfo)
{
    HANDLE hCompletionEvent = waitInfo->CompletionEvent;
    if(waitInfo->Context && (waitInfo->flag & WAIT_FREE_CONTEXT)) {
        DelegateInfo* pDelegate = (DelegateInfo*) waitInfo->Context;
        pDelegate->Release();
        RecycleMemory((LPVOID*)pDelegate, MEMTYPE_DelegateInfo); //delete pDelegate;
    }
    
    delete waitInfo;

    if (hCompletionEvent!= INVALID_HANDLE)
        SetEvent(hCompletionEvent);

}



/************************************************************************/

BOOL ThreadpoolMgr::UnregisterWaitEx(HANDLE hWaitObject,HANDLE Event)
{
    _ASSERTE(IsInitialized());              // cannot call unregister before first registering

    const BOOL NonBlocking = ( Event != (HANDLE) -1 ) ;
    const BOOL Blocking = (Event == (HANDLE) -1);
    WaitInfo* waitInfo = (WaitInfo*) hWaitObject;
    WaitEvent* CompletionEvent = NULL; 
    WaitEvent* PartialCompletionEvent = NULL; // used to wait until the wait has been deactivated

    if (!hWaitObject)
    {
        return FALSE;
    }

    // we do not allow callbacks to run in the wait thread, hence the assert
    _ASSERTE(GetCurrentThreadId() != waitInfo->threadCB->threadId);


    if (Blocking) 
    {
        // Get an event from the event cache
        CompletionEvent = GetWaitEvent() ;  // get event from the event cache

        if (!CompletionEvent) 
        {
            return FALSE ;
        }
    } 

    waitInfo->CompletionEvent = CompletionEvent
                                ? CompletionEvent->Handle
                                : Event ;

    if (NonBlocking)
    {
        // we still want to block until the wait has been deactivated
        PartialCompletionEvent = GetWaitEvent () ;
        if (!PartialCompletionEvent) 
        {
            return FALSE ;
        }
        waitInfo->PartialCompletionEvent = PartialCompletionEvent->Handle;
    }
    else
    {
        waitInfo->PartialCompletionEvent = INVALID_HANDLE;
    }

	LOG((LF_THREADPOOL ,LL_INFO1000 ,"Unregistering wait, waitHandle=%x, Context=%x\n",
			waitInfo->waitHandle, waitInfo->Context));


	BOOL status = QueueUserAPC((PAPCFUNC)DeregisterWait,
                               waitInfo->threadCB->threadHandle,
                               (size_t) waitInfo);

    if (status == 0)
    {
        if (CompletionEvent) FreeWaitEvent(CompletionEvent);
        if (PartialCompletionEvent) FreeWaitEvent(PartialCompletionEvent);
		return FALSE;
    }

    if (NonBlocking) 
    {
        WaitForSingleObject(PartialCompletionEvent->Handle, INFINITE ) ;
        FreeWaitEvent(PartialCompletionEvent);
    } 
    
    else        // i.e. blocking
    {
        _ASSERTE(CompletionEvent);
        WaitForSingleObject(CompletionEvent->Handle, INFINITE ) ;
        FreeWaitEvent(CompletionEvent);
    }
    return TRUE;
}


void ThreadpoolMgr::DeregisterWait(WaitInfo* pArgs)
{
	WaitInfo* waitInfo = pArgs;

    if ( ! (waitInfo->state & WAIT_REGISTERED) ) 
    {
        // set state to deleted, so that it does not get registered
        waitInfo->state |= WAIT_DELETE ;
        
        // since the wait has not even been registered, we dont need an interlock to decrease the RefCount
        waitInfo->refCount--;

        if ( waitInfo->PartialCompletionEvent != INVALID_HANDLE) 
        {
            SetEvent( waitInfo->PartialCompletionEvent ) ;
        }
        return;
    }

    if (waitInfo->state & WAIT_ACTIVE) 
    {
        DeactivateWait(waitInfo);
    }

    if ( waitInfo->PartialCompletionEvent != INVALID_HANDLE) 
    {
        SetEvent( waitInfo->PartialCompletionEvent ) ;
    }

    if (InterlockedDecrement(&waitInfo->refCount) == 0)
    {
        DeleteWait(waitInfo);
    }
    return;
}


/* This gets called in a finalizer thread ONLY IF an app does not deregister the 
   the wait. Note that just because the registeredWaitHandle is collected by GC
   does not mean it is safe to delete the wait. The refcount tells us when it is 
   safe.
*/
void ThreadpoolMgr::WaitHandleCleanup(HANDLE hWaitObject)
{
    WaitInfo* waitInfo = (WaitInfo*) hWaitObject;
    _ASSERTE(waitInfo->refCount > 0);

    QueueUserAPC((PAPCFUNC)DeregisterWait, 
                     waitInfo->threadCB->threadHandle,
					 (size_t) waitInfo);
}


ThreadpoolMgr::WaitEvent* ThreadpoolMgr::GetWaitEvent()
{
    WaitEvent* waitEvent;

    LOCKCOUNTINCL("GetWaitEvent in win32ThreadPool.h");                     \
    EnterCriticalSection(&EventCacheCriticalSection);

    if (!IsListEmpty (&EventCache)) 
    {
        LIST_ENTRY* FirstEntry;

        RemoveHeadList (&EventCache, FirstEntry);
        
        waitEvent = (WaitEvent*) FirstEntry ;

        NumUnusedEvents--;

        LeaveCriticalSection(&EventCacheCriticalSection);
        LOCKCOUNTDECL("GetWaitEvent in win32ThreadPool.h");                     \

    }
    else
    {
        LeaveCriticalSection(&EventCacheCriticalSection);
        LOCKCOUNTDECL("GetWaitEvent in win32ThreadPool.h");                     \

        waitEvent = new WaitEvent;
        
        if (waitEvent == NULL)
            return NULL;

        waitEvent->Handle = WszCreateEvent(NULL,TRUE,FALSE,NULL);

        if (waitEvent->Handle == NULL)
        {
            delete waitEvent;
            return NULL;
        }
    }
    return waitEvent;
}

void ThreadpoolMgr::FreeWaitEvent(WaitEvent* waitEvent)
{
    ResetEvent(waitEvent->Handle);
    LOCKCOUNTINCL("FreeWaitEvent in win32ThreadPool.h");                        \
    EnterCriticalSection(&EventCacheCriticalSection);

    if (NumUnusedEvents < MAX_CACHED_EVENTS)
    {
        InsertHeadList (&EventCache, &waitEvent->link) ;

        NumUnusedEvents++;

    }
    else
    {
        CloseHandle(waitEvent->Handle);
        delete waitEvent;
    }

    LeaveCriticalSection(&EventCacheCriticalSection);
    LOCKCOUNTDECL("FreeWaitEvent in win32ThreadPool.h");                        \

}

void ThreadpoolMgr::CleanupEventCache()
{
    for (LIST_ENTRY* Node = (LIST_ENTRY*) EventCache.Flink ; 
         Node != &EventCache ; 
         )
    {
        WaitEvent* waitEvent = (WaitEvent*) Node;
        CloseHandle(waitEvent->Handle);
        Node = (LIST_ENTRY*)Node->Flink;
        delete waitEvent;
    }
}

BOOL ThreadpoolMgr::CreateGateThread()
{
    DWORD threadId;

    if (FastInterlockCompareExchange(&GateThreadCreated, 1, 0) == 1)
    {
       return TRUE;
    }

    HANDLE threadHandle = CreateThread(NULL,                // security descriptor
                                       0,                   // default stack size
                                       GateThreadStart, // 
                                       NULL,                //no arguments
                                       CREATE_SUSPENDED,    // start immediately
                                       &threadId);

    if (threadHandle)
    {
#ifdef _DEBUG
        DWORD status =
#endif
        ResumeThread(threadHandle);
        _ASSERTE(status != (DWORD) -1);

        LOG((LF_THREADPOOL ,LL_INFO100 ,"Gate thread started\n"));

        CloseHandle(threadHandle);  //we don't need this anymore
        return TRUE;
    }

    return FALSE;
}



/************************************************************************/

BOOL ThreadpoolMgr::BindIoCompletionCallback(HANDLE FileHandle,
                                            LPOVERLAPPED_COMPLETION_ROUTINE Function,
                                            ULONG Flags )
{

    _ASSERTE(false);
    ::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);    
    return FALSE;

}



#define GATE_THREAD_DELAY 500 /*milliseconds*/


DWORD __stdcall ThreadpoolMgr::GateThreadStart(LPVOID lpArgs)
{


    for (;;)
    {
        Sleep(GATE_THREAD_DELAY);
		
#ifdef DEBUGGING_SUPPORTED
        // if we are stopped at a debug breakpoint, go back to sleep
        if (CORDebuggerAttached() && g_pDebugInterface->IsStopped())
            continue;
#endif // DEBUGGING_SUPPORTED


        if (MonitorWorkRequestsQueue)
        {
            GrowWorkerThreadPoolIfStarvationSimple();
        }

    }       // for (;;)
}

// called by logic to spawn a new completion port thread.
// return false if not enough time has elapsed since the last
// time we sampled the cpu utilization. 
BOOL ThreadpoolMgr::SufficientDelaySinceLastSample(unsigned int LastThreadCreationTime, 
												   unsigned NumThreads,	 // total number of threads of that type (worker or CP)
												   double    throttleRate // the delay is increased by this percentage for each extra thread
												   )
{

	unsigned dwCurrentTickCount =  GetTickCount();
	
	unsigned delaySinceLastThreadCreation = dwCurrentTickCount - LastThreadCreationTime;

	unsigned minWaitBetweenThreadCreation =  GATE_THREAD_DELAY;

	if (throttleRate > 0.0)
	{
		_ASSERTE(throttleRate <= 1.0);

		unsigned adjustedThreadCount = NumThreads > NumberOfProcessors ? (NumThreads - NumberOfProcessors) : 0;

		minWaitBetweenThreadCreation = (unsigned) (GATE_THREAD_DELAY * pow((1.0 + throttleRate),(double)adjustedThreadCount));
	}
	// the amount of time to wait should grow up as the number of threads is increased
    
	return (delaySinceLastThreadCreation > minWaitBetweenThreadCreation); 

}

/*
BOOL ThreadpoolMgr::SufficientDelaySinceLastCompletion()
{
    #define DEQUEUE_DELAY_THRESHOLD (GATE_THREAD_DELAY * 2)
	
	unsigned delay = GetTickCount() - LastCompletionTime;

	unsigned tooLong = NumCPThreads * DEQUEUE_DELAY_THRESHOLD; 

	return (delay > tooLong);

}
*/

// called by logic to spawn new worker threads, return true if it's been too long
// since the last dequeue operation - takes number of worker threads into account
// in deciding "too long"
BOOL ThreadpoolMgr::SufficientDelaySinceLastDequeue()
{
    #define DEQUEUE_DELAY_THRESHOLD (GATE_THREAD_DELAY * 2)
    
    unsigned delay = GetTickCount() - LastDequeueTime;

    unsigned tooLong = NumWorkerThreads * DEQUEUE_DELAY_THRESHOLD; 

    return (delay > tooLong);

}

#ifdef _MSC_VER
#pragma warning (default : 4715)
#endif

/************************************************************************/

BOOL ThreadpoolMgr::CreateTimerQueueTimer(PHANDLE phNewTimer,
                                          WAITORTIMERCALLBACK Callback,
                                          PVOID Parameter,
                                          DWORD DueTime,
                                          DWORD Period,
                                          ULONG Flag)
{
    EnsureInitialized();

    // For now we use just one timer thread. Consider using multiple timer threads if 
    // number of timers in the queue exceeds a certain threshold. The logic and code 
    // would be similar to the one for creating wait threads.
    if (NULL == TimerThread)
    {
        LOCKCOUNTINCL("CreateTimerQueueTimer in win32ThreadPool.h");                        \
        EnterCriticalSection(&TimerQueueCriticalSection);

        // check again
        if (NULL == TimerThread)
        {
            DWORD threadId;
            TimerThread = CreateThread(NULL,                // security descriptor
                                       0,                   // default stack size
                                       TimerThreadStart,        // 
                                       NULL,    // thread control block is passed as argument
                                       0,   
                                       &threadId);
            if (TimerThread == NULL)
            {
                LeaveCriticalSection(&TimerQueueCriticalSection);
                LOCKCOUNTDECL("CreateTimerQueueTimer in win32ThreadPool.h"); \
                return FALSE;
            }

            LOG((LF_THREADPOOL ,LL_INFO100 ,"Timer thread started\n"));
        }
        LeaveCriticalSection(&TimerQueueCriticalSection);
        LOCKCOUNTDECL("CreateTimerQueueTimer in win32ThreadPool.h");                        \

    }


    TimerInfo* timerInfo = new TimerInfo;
    *phNewTimer = (HANDLE) timerInfo;

    if (NULL == timerInfo)
        return FALSE;

    timerInfo->FiringTime = DueTime;
    timerInfo->Function = (LPVOID) Callback;
    timerInfo->Context = Parameter;
    timerInfo->Period = Period;
    timerInfo->state = 0;
    timerInfo->flag = Flag;

    BOOL status = QueueUserAPC((PAPCFUNC)InsertNewTimer,TimerThread,(size_t)timerInfo);
    if (FALSE == status)
        return FALSE;

    return TRUE;
}

#ifdef _MSC_VER
#pragma warning (disable : 4715)
#endif
DWORD __stdcall ThreadpoolMgr::TimerThreadStart(LPVOID args)
{
    _ASSERTE(NULL == args);
    
    // Timer threads never die

    LastTickCount = GetTickCount();


    for (;;)
    {
        DWORD timeout = FireTimers();

        LastTickCount = GetTickCount();

        SleepEx(timeout, TRUE);

        // the thread could wake up either because an APC completed or the sleep timeout
        // in both case, we need to sweep the timer queue, firing timers, and readjusting
        // the next firing time


    }
}
#ifdef _MSC_VER
#pragma warning (default : 4715)
#endif

// Executed as an APC in timer thread
void ThreadpoolMgr::InsertNewTimer(TimerInfo* pArg)
{
    _ASSERTE(pArg);
    TimerInfo * timerInfo = pArg;

    if (timerInfo->state & TIMER_DELETE)
    {   // timer was deleted before it could be registered
        DeleteTimer(timerInfo);
        return;
    }

    // set the firing time = current time + due time (note initially firing time = due time)
    DWORD currentTime = GetTickCount();
    if (timerInfo->FiringTime == (ULONG) -1)
    {
        timerInfo->state = TIMER_REGISTERED;
        timerInfo->refCount = 1;

    }
    else
    {
        timerInfo->FiringTime += currentTime;

        timerInfo->state = (TIMER_REGISTERED | TIMER_ACTIVE);
        timerInfo->refCount = 1;

        // insert the timer in the queue
        InsertTailList(&TimerQueue,(&timerInfo->link));
    }

    LOG((LF_THREADPOOL ,LL_INFO1000 ,"Timer created, period= %x, Function = %x\n",
                  timerInfo->Period, timerInfo->Function));

    return;
}


// executed by the Timer thread
// sweeps through the list of timers, readjusting the firing times, queueing APCs for
// those that have expired, and returns the next firing time interval
DWORD ThreadpoolMgr::FireTimers()
{

    DWORD currentTime = GetTickCount();

    DWORD nextFiringInterval = (DWORD) -1;

    for (LIST_ENTRY* node = (LIST_ENTRY*) TimerQueue.Flink;
         node != &TimerQueue;
        )
    {
        TimerInfo* timerInfo = (TimerInfo*) node;
        node = (LIST_ENTRY*) node->Flink;

        if (TimeExpired(LastTickCount, currentTime, timerInfo->FiringTime))
        {
            if (timerInfo->Period == 0 || timerInfo->Period == (ULONG) -1)
            {
                DeactivateTimer(timerInfo);
            }

            InterlockedIncrement(&timerInfo->refCount);

            QueueUserWorkItem(AsyncTimerCallbackCompletion,
                              timerInfo,
                              0 /* TimerInfo take care of deleting*/);
                
            timerInfo->FiringTime = currentTime+timerInfo->Period;

            if ((timerInfo->Period != 0) && (timerInfo->Period != (ULONG) -1) && (nextFiringInterval > timerInfo->Period))
                nextFiringInterval = timerInfo->Period;
        }

        else
        {
            DWORD firingInterval = TimeInterval(timerInfo->FiringTime,currentTime);
            if (firingInterval < nextFiringInterval)
                nextFiringInterval = firingInterval; 
        }
    }

    return nextFiringInterval;
}

DWORD __stdcall ThreadpoolMgr::AsyncTimerCallbackCompletion(PVOID pArgs)
{
    TimerInfo* timerInfo = (TimerInfo*) pArgs;
    ((WAITORTIMERCALLBACKFUNC) timerInfo->Function) (timerInfo->Context, TRUE) ;

    if (InterlockedDecrement(&timerInfo->refCount) == 0)
        DeleteTimer(timerInfo);

    return 0; /* ignored */
}


// removes the timer from the timer queue, thereby cancelling it
// there may still be pending callbacks that haven't completed
void ThreadpoolMgr::DeactivateTimer(TimerInfo* timerInfo)
{
    RemoveEntryList((LIST_ENTRY*) timerInfo);

    timerInfo->state = timerInfo->state & ~TIMER_ACTIVE;
}

void ThreadpoolMgr::DeleteTimer(TimerInfo* timerInfo)
{
    _ASSERTE((timerInfo->state & TIMER_ACTIVE) == 0);

    if (timerInfo->Context && (timerInfo->flag & WAIT_FREE_CONTEXT)) 
        delete (BYTE *) (timerInfo->Context);

    if (timerInfo->CompletionEvent != INVALID_HANDLE)
        SetEvent(timerInfo->CompletionEvent);

    delete timerInfo;
}

/************************************************************************/
BOOL ThreadpoolMgr::ChangeTimerQueueTimer(
                                        HANDLE Timer,
                                        ULONG DueTime,
                                        ULONG Period)
{
    THROWSCOMPLUSEXCEPTION();
    _ASSERTE(IsInitialized());
    _ASSERTE(Timer);                    // not possible to give invalid handle in managed code

    TimerUpdateInfo* updateInfo = new TimerUpdateInfo;
    if (NULL == updateInfo)
        COMPlusThrow(kOutOfMemoryException);

    updateInfo->Timer = (TimerInfo*) Timer;
    updateInfo->DueTime = DueTime;
    updateInfo->Period = Period;

    BOOL status = QueueUserAPC((PAPCFUNC)UpdateTimer,
                               TimerThread,
                               (size_t) updateInfo);

    return status;
}

void ThreadpoolMgr::UpdateTimer(TimerUpdateInfo* pArgs)
{
    TimerUpdateInfo* updateInfo = (TimerUpdateInfo*) pArgs;
    TimerInfo* timerInfo = updateInfo->Timer;

    timerInfo->Period = updateInfo->Period;

    if (updateInfo->DueTime == (ULONG) -1)
    {
        if (timerInfo->state & TIMER_ACTIVE)
        {
            DeactivateTimer(timerInfo);
        }
        // else, noop (the timer was already inactive)
        _ASSERTE((timerInfo->state & TIMER_ACTIVE) == 0);
        LOG((LF_THREADPOOL ,LL_INFO1000 ,"Timer inactive, period= %x, Function = %x\n",
                 timerInfo->Period, timerInfo->Function));
        
        delete updateInfo;
        return;
    }

    DWORD currentTime = GetTickCount();
    timerInfo->FiringTime = currentTime + updateInfo->DueTime;

    delete updateInfo;
    
    if (! (timerInfo->state & TIMER_ACTIVE))
    {
        // timer not active (probably a one shot timer that has expired), so activate it
        timerInfo->state |= TIMER_ACTIVE;
        _ASSERTE(timerInfo->refCount >= 1);
        // insert the timer in the queue
        InsertTailList(&TimerQueue,(&timerInfo->link));
        
    }

    LOG((LF_THREADPOOL ,LL_INFO1000 ,"Timer changed, period= %x, Function = %x\n",
             timerInfo->Period, timerInfo->Function));

    return;
}

/************************************************************************/
BOOL ThreadpoolMgr::DeleteTimerQueueTimer(
                                        HANDLE Timer,
                                        HANDLE Event)
{
    _ASSERTE(IsInitialized());          // cannot call delete before creating timer
    _ASSERTE(Timer);                    // not possible to give invalid handle in managed code

    WaitEvent* CompletionEvent = NULL; 

    if (Event == (HANDLE) -1)
    {
        CompletionEvent = GetWaitEvent();
    }

    TimerInfo* timerInfo = (TimerInfo*) Timer;

    timerInfo->CompletionEvent = CompletionEvent ? CompletionEvent->Handle : Event;

    BOOL status = QueueUserAPC((PAPCFUNC)DeregisterTimer,
                               TimerThread,
                               (size_t) timerInfo);

    if (FALSE == status)
    {
        if (CompletionEvent)
            FreeWaitEvent(CompletionEvent);
        return FALSE;
    }

    if (CompletionEvent)
    {
        WaitForSingleObject(CompletionEvent->Handle,INFINITE);
        FreeWaitEvent(CompletionEvent);
    }
    return status;
}

void ThreadpoolMgr::DeregisterTimer(TimerInfo* pArgs)
{

    TimerInfo* timerInfo = (TimerInfo*) pArgs;

    if (! (timerInfo->state & TIMER_REGISTERED) )
    {
        // set state to deleted, so that it does not get registered
        timerInfo->state |= WAIT_DELETE ;
        
        // since the timer has not even been registered, we dont need an interlock to decrease the RefCount
        timerInfo->refCount--;

        return;
    }

    if (timerInfo->state & WAIT_ACTIVE) 
    {
        DeactivateTimer(timerInfo);
    }

    LOG((LF_THREADPOOL ,LL_INFO1000 ,"Timer deregistered\n"));

    if (InterlockedDecrement(&timerInfo->refCount) == 0 ) 
    {
        DeleteTimer(timerInfo);
    }
    return;
}

void ThreadpoolMgr::CleanupTimerQueue()
{

}

