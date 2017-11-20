/*++


 Copyright (c) 2002 Microsoft Corporation.  All rights reserved.

 The use and distribution terms for this software are contained in the file
 named license.txt, which can be found in the root of this distribution.
 By using this software in any fashion, you are agreeing to be bound by the
 terms of this license.

 You must not remove this notice, or any other, from this software.


Module Name:

    wait.c

Abstract:

    Implementation of waiting functions as described in
    the WIN32 API

Revision History:

--*/
#include "pal/palinternal.h"
#include "pal/dbgmsg.h"
#include "pal/thread.h"
#include "pal/handle.h"
#include "pal/mutex.h"
#include "pal/critsect.h"
#include "event.h"
#include "semaphore.h"
#include "../thread/process.h"
                 
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#if HAVE_POLL
#include <poll.h>
#else
#include "pal/fakepoll.h"
#endif  // HAVE_POLL
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#if HAVE_STROPTS_H
#include <stropts.h>
#endif  // HAVE_STROPTS_H
#if HAVE_BROKEN_FIFO_SELECT
#include <sys/stat.h>
#endif  /* HAVE_BROKEN_FIFO_SELECT */

SET_DEFAULT_DEBUG_CHANNEL(SYNC);

/*global variables for interaction with WFMO worker thread. worker thread is 
used when WFMO is called with multiple handles, one of which is a process. It 
is guaranteed that this will never occur multiple times concurrently in a 
process (that is, 2 threads will never call WFMO this way at the same time), 
so using global (static) variables is perfectly safe. this guarantee 
also allows us to use a single, global worker thread instead of having to 
create one every time.
keep_going : if FALSE, worker thread breaks out of main loop 
worker_handle :  handle of worker thread (from CreateThread)
worker_event :   handle of event used to wake up worker thread
standby_event :  handle of event used to indicate that the worker thread is 
                 going back in standby mode
process_event :  handle of event used by worker thread to indicate that the 
                 process is signaled
process_handle : handle of process on which worker thread must wait
worker_timeout : timeout to be used by worker thread (from WFMO parameter list)
*/
static int keep_going;
static HANDLE worker_handle;
static HANDLE worker_event;
static HANDLE standby_event;
static HANDLE process_event;
static HANDLE process_handle;
static DWORD worker_timeout;


/****************** static functions ***************************/
static WAITON_CODE WaitOn(DWORD type, HANDLE handle, SHMPTR wait_state);
static int StopWaitingOnObjects(HOBJSTRUCT **hObjs, CONST HANDLE *pHandles, 
                                int Count, int process_index);
static BOOL StopWaiting(DWORD type, HANDLE handle);
static DWORD ThreadWait(DWORD MilliSeconds, BOOL bAlertable, 
                        DWORD *pWaitState);
static int PollBlockingPipe(DWORD Milliseconds, int blockingPipe, 
                            DWORD *pWaitState);
static void WFMO_update_timeout(DWORD *old_time, DWORD *timeout);
static DWORD WFMO_WaitForAllObjects(int nCount, CONST HANDLE *lpHandles, 
                                   HOBJSTRUCT **hObjs, DWORD dwMilliseconds);
static DWORD WFMO_WaitForProcess(HANDLE hProcess, DWORD dwMilliseconds, 
                                 BOOL interruptible);
static DWORD_PTR PALAPI WFMO_workerthread(LPVOID param);

/*++
Function:
  WaitForSingleObject

See MSDN doc.
--*/
DWORD
PALAPI
WaitForSingleObject(
            IN HANDLE hHandle,
            IN DWORD dwMilliseconds)
{
    DWORD ret;

    ENTRY("WaitForSingleObject(hHandle=%p, dwMilliseconds=%u)\n",
          hHandle, dwMilliseconds);

    ret = WaitForMultipleObjectsEx(1, &hHandle, FALSE, dwMilliseconds,
                                   FALSE);

    LOGEXIT("WaitForSingleObject returns DWORD %u\n", ret);
    return ret;
}


/*++
Function:
  WaitForMultipleObjects

See MSDN doc.

--*/
DWORD
PALAPI
WaitForMultipleObjects(
               IN DWORD nCount,
               IN CONST HANDLE *lpHandles,
               IN BOOL bWaitAll,
               IN DWORD dwMilliseconds)
{
    DWORD ret;

    ENTRY("WaitForMultipleObjects(nCount=%d, lpHandles=%p,"
          " bWaitAll=%d, dwMilliseconds=%u)\n",
          nCount, lpHandles, bWaitAll, dwMilliseconds);

    ret = WaitForMultipleObjectsEx(nCount, lpHandles, bWaitAll, 
                                   dwMilliseconds, FALSE);

    LOGEXIT("WaitForMultipleObject returns DWORD %u\n", ret);
    return ret;
}

/*++
Function:
  WaitForMultipleObjectsEx

All the other waiting functions eventually call this function. 

How thread blocking is achieved:
There's a pipe associated with each Rotor thread. When the current thread
calls WaitForMultipleObjectsEx() and there's no object signaled, the thread
will try to read on the pipe associated to the thread. This will make the 
thread block until another thread writes in that pipe.

For each blocking objects (mutex, event, thread, semaphore) there's a list
of waiting threads. When an object is signaled, if there's a thread waiting 
for this object, the thread is removed from the waiting list, the thread pipe
is retreived, and then a DWORD is written into the pipe to wake up the 
thread.

See MSDN doc for info about this function.
--*/
DWORD
PALAPI
WaitForMultipleObjectsEx(
             IN DWORD nCount,
             IN CONST HANDLE *lpHandles,
             IN BOOL bWaitAll,
             IN DWORD dwMilliseconds,
             IN BOOL bAlertable)
{
    HANDLE      hHandles[MAXIMUM_WAIT_OBJECTS];
    HOBJSTRUCT *hObjs[MAXIMUM_WAIT_OBJECTS];
    int i;
    int handles_locked = 0;
    DWORD retValue = WAIT_FAILED;
    THREAD *pThread = NULL;
    HANDLE hThread = NULL;
    int NumAPCCalled = 0;
    int process_index=-1; /* index of process handle to wait on, if any */


    ENTRY("WaitForMultipleObjectsEx(nCount=%d, lpHandles=%p, bWaitAll=%d, "
          "dwMilliseconds=%u, bAlertable=%d)\n",
          nCount, lpHandles, bWaitAll, dwMilliseconds, bAlertable);

    if (nCount >= MAXIMUM_WAIT_OBJECTS)
    {
        ERROR("nCount is greater than MAXIMUM_WAIT_OBJECTS\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        goto WaitFMOExit;
    }

    /* Validate and lock all handles */
    /* Only mutexes, semaphores, events, processes and threads handles may be 
       passed to this function */
    for (i = 0; i < nCount; i++)
    {
        /* create a local copy of lpHandles - bad caller may change lpHandles while we are working with it */
        hHandles[i] = lpHandles[i];

        hObjs[i] = HMGRLockHandle(hHandles[i]);

        if (hObjs[i] == NULL)
        {
            ERROR("Invalid handle[%d]\n", i);
            SetLastError(ERROR_INVALID_PARAMETER);
            /* cleanup code will unlock all handles until hObjs[i-1], since it 
               stops on the first NULL (and we just got hObjs[i]==NULL) */
            goto WaitFMOExit;
        }

        handles_locked++;

        if ((hObjs[i]->type != HOBJ_PROCESS)  &&
            (hObjs[i]->type != HOBJ_EVENT)  &&
            (hObjs[i]->type != HOBJ_SEMAPHORE)  &&
            (hObjs[i]->type != HOBJ_MUTEX)  &&
            (hObjs[i]->type != HOBJ_THREAD))
        {
            ASSERT("Handle %p (index %d) has invalid type %#x\n", 
                   hHandles[i], i, hObjs[i]->type);
            SetLastError(ERROR_INVALID_HANDLE);
            goto WaitFMOExit;
        }

        /* if handle is a process handle, remember its index, it may need 
           special treatment */
        if(HOBJ_PROCESS == hObjs[i]->type)
        {
            if(-1 == process_index)
            {
                process_index = i;
            }
            else if(!bWaitAll)
            {
                /* without bWaitAll, there must never be more than 1 process 
                   handle in the array */
                ASSERT("found more than 1 process handle in the array!\n");
            }
        }
    }

    /* If we are alertable, then we need to execute any queued APC calls */
    if (bAlertable)
    {
        NumAPCCalled = THREADCallThreadAPCs();
        if (NumAPCCalled == -1)
        {
            ERROR("Failed in calling APCs for the current thread\n");
            goto WaitFMOExit;
        }
        else if (NumAPCCalled > 0)
        {
            retValue = STATUS_USER_APC;
            goto WaitFMOExit;
        }
    }

    if (bWaitAll && (handles_locked != 1))
    {
        TRACE("bWaitAll is TRUE : waiting for all specified objects.\n");
        retValue = WFMO_WaitForAllObjects(handles_locked, hHandles, hObjs, 
                                          dwMilliseconds);

        if(retValue !=WAIT_TIMEOUT)
        {
            TRACE("All specified objects have been signalled.\n");
        }
        else
        {           
            TRACE("Timeout value reached while waiting for all specified "
                  "objects.\n");
        }
    }
    else  /* bWaitAll == FALSE */
    if(-1 != process_index && 1 == handles_locked && !bAlertable)
    {
        /* only one handle to wait for, and it's a process handle : no need to 
           use the worker thread mechanism. Note that WFMO_WaitForProcess can't 
           support alertable waits, so if bAlertable is TRUE, we also want to 
           use the worker thread */
        /* using the worker thread for this still requires the guarantee that 
           this operation will never be attempted more than once at the same 
           time... if we can't get this guarantee, we can't use the worker 
           thread; this means we can't support bAlertable when waiting only on
           a process handle */
        retValue = WFMO_WaitForProcess(hHandles[0], dwMilliseconds, FALSE);
    }
    else /* no process handle, or more than 1 handle, or alertable wait */
    {
        DWORD WakeUpSignal;
        SHMPTR shmThreadWaitState;
        DWORD *pThreadWaitState;
        DWORD waitState;
        WAITON_CODE ret = 0;

        if ((hThread = PROCGetRealCurrentThread()) == INVALID_HANDLE_VALUE)
        {
            ASSERT("Unable to get the real current thread handle\n");
            goto WaitFMOExit;
        }

        pThread = (THREAD *)HMGRLockHandle2(hThread, HOBJ_THREAD);
        if(NULL == pThread)
        {
            ASSERT("Unable to lock thread handle %p!\n", hThread);
            goto WaitFMOExit;
        }
        
        /* if there's a process handle, get ready to use the worker thread */
        if( -1 != process_index)
        {
            TRACE("got a process handle, need to use the WFMO worker thread\n");

            if(NULL == worker_handle)
            {
                /*first-time initialization : create worker thread and event */
                DWORD tid;

                TRACE("creating WFMO worker thread and related events\n");

                /* reset flag used to stop worker thread */
                keep_going = TRUE;

                /* reset process handle to avoid asserting below */
                process_handle = NULL;

                /* create worker thread's event, which we'll use to wake it up 
                  when we want it to wait on a process, and to tell it to stop 
                  waiting on a process if a different object gets signaled */
                worker_event = CreateEventW(NULL, FALSE, FALSE, NULL);
                if(NULL == worker_event)
                {
                    ERROR("couldn't create event used to signal WFMO worker "
                          "thread; error is %d\n", GetLastError());
                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    goto WaitFMOExit;
                }

                /* create event used to indicate that the worker thread is 
                   going back to standby mode */
                standby_event = CreateEventW(NULL, FALSE, FALSE, NULL);
                if(NULL == standby_event)
                {
                    ERROR("couldn't create event used to signal WFMO worker "
                          "thread; error is %d\n", GetLastError());
                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    CloseHandle(worker_event);
                    goto WaitFMOExit;
                }

                /* create event used by worker thread to signal us when process
                   has terminated */
                process_event = CreateEventW(NULL, FALSE, FALSE, NULL);
                if(NULL == process_event)
                {
                    ERROR("couldn't create event for use by WFMO worker thread; "
                          "error is %d\n", GetLastError());
                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    CloseHandle(standby_event);
                    CloseHandle(worker_event);
                    goto WaitFMOExit;
                }

                /* create the worker thread itself */
                worker_handle = CreateThread(NULL, 0, &WFMO_workerthread, NULL,
                                            0, &tid);
                if(NULL == worker_handle)
                {
                    ERROR("couldn't create WFMO worker thread; error is %d\n",
                          GetLastError());
                    CloseHandle(standby_event);
                    CloseHandle(worker_event);
                    CloseHandle(process_event);
                    SetLastError(ERROR_OUTOFMEMORY);
                    goto WaitFMOExit;
                }
            }

            /* tell worker thread which process to wait on */
            if(NULL != process_handle)
            {
                ASSERT("trying to use the WFMO worker thread to wait on "
                       "process %#x, but it's already busy waiting on %#x\n",
                       hHandles[process_index], process_handle);
            }
            process_handle = hHandles[process_index];

            /* tell worker thread how long to wait for the process to die */
            worker_timeout = dwMilliseconds;
        }                  
        
        shmThreadWaitState = pThread->waitAwakened;
        pThreadWaitState = SHMPTR_TO_PTR(shmThreadWaitState);
        waitState = bAlertable?TWS_ALERTABLE:TWS_WAITING;
        waitState = InterlockedCompareExchange(pThreadWaitState, waitState, 
                                               TWS_ACTIVE);
        if(TWS_EARLYDEATH == waitState)
        {
            /* process is terminating, this thread will soon be suspended 
               (by SuspendOtherThreads). */
            WARN("thread is about to get suspended by TerminateProcess\n");
        }
        else if(TWS_ACTIVE != waitState)
        {
            ASSERT("unexpected thread wait state %d\n", waitState);
        }

        /* check if there's any signaled object, add this thread to 
           non-signaled objects' waiting list */
        for (i = 0; i < handles_locked; i++)
        {
            if(i == process_index)
            {
                /* reached the index of the process handle. tell the worker 
                  thread to start waiting, and wait on our event instead of the 
                  process; the worker thread will signal process_event when the 
                  process is signaled */
                /* note : we call WaitOn before SetEvent; this allows the worker
                   thread to use process_event as a toggle (Set it, then 
                   immediately Reset it) */
                ret = WaitOn(HOBJ_EVENT, process_event, 
                             shmThreadWaitState);
                TRACE("now waking up WFMO worker thread\n");
                SetEvent(worker_event);
            }
            else
            {
                ret = WaitOn(hObjs[i]->type, *(hHandles+i), 
                             shmThreadWaitState);
            }                                        

            if (ret == WOC_ERROR)
            {
                ERROR("WaitOn() reported an error for object %p\n",
                      *(hHandles+i));
                /* an error occurred, SetLastError has already been called */
                StopWaitingOnObjects(hObjs, hHandles, i, process_index);
                goto WaitFMOExit;
            }

            if (WOC_SIGNALED == ret || WOC_ABANDONED == ret) 
            {
                /* one object was signaled, and we don't 
                    need to wait on every object, so we're done waiting */
                if(WOC_SIGNALED == ret)
                {
                    retValue = WAIT_OBJECT_0 + i;
                }
                else
                {
                    retValue = WAIT_ABANDONED_0 + i;
                }

                StopWaitingOnObjects(hObjs, hHandles, i, process_index);
                goto WaitFMOExit;
            }
            else if(WOC_INTERUPTED == ret)
            {
                /* ThreadWait shouldn't wait, since there should already be a 
                   wakeup code in the pipe. but just in case something went 
                   wrong, let's make the call non-blocking and catch the 
                   unexpected timeout */
                dwMilliseconds = 0;
            }
        }

        HMGRUnlockHandle(hThread, &pThread->objHeader);
        pThread = NULL;

        /* wait until one object is being signaled. */
        WakeUpSignal = ThreadWait( dwMilliseconds, bAlertable, 
                                   pThreadWaitState );

        /* one object has been signaled or the timeout has been reached, 
           remove the current thread from the waiting thread list 
           of every objects. The object which doesn't contained the 
           current thread in its waiting thread list is the object 
           that has been signaled */
        retValue = StopWaitingOnObjects(hObjs, hHandles, handles_locked,process_index);

        /* If we get this special code, that means we were unblocked
           by a queued user APC, and we were alertable... so we need
           to return here */
        if (WakeUpSignal == WUTC_APC_QUEUED)
        {
            retValue = STATUS_USER_APC;
        }
        else
        {
            if (retValue == -1)
            {
                /* if we don't found any object signaled, it means
                   the thread has been woke up by the timeout */
                retValue = WAIT_TIMEOUT;
            }
            else
            {
                if ( WakeUpSignal == WUTC_ABANDONED )
                {
                    retValue += WAIT_ABANDONED_0;
                }
                else
                {
                    retValue += WAIT_OBJECT_0;
                }
            }
        }
    }
   

WaitFMOExit:
    /* unlock thread handle if not done yet */
    if (NULL != pThread)
    {
        HMGRUnlockHandle(hThread, &pThread->objHeader);
    }         

    /* unlock all locked handles; stop on first NULL pointer */
    for(i=0;i<handles_locked;i++)
    {
        HMGRUnlockHandle(hHandles[i],hObjs[i]);
    }

    LOGEXIT("WaitForMultipleObjectsEx returns DWORD %u\n", retValue);
    return retValue;
}


/*++
Function:
  Sleep

See MSDN doc.
--*/
VOID
PALAPI
Sleep(
      IN DWORD dwMilliseconds)
{
    ENTRY("Sleep(dwMilliseconds=%u)\n", dwMilliseconds);

    WaitForMultipleObjectsEx(0, NULL, FALSE, dwMilliseconds, FALSE);

    LOGEXIT("Sleep returns VOID\n");
}


/*++
Function:
  SleepEx

See MSDN doc.
--*/
DWORD
PALAPI
SleepEx(
    IN DWORD dwMilliseconds,
    IN BOOL bAlertable)
{
    DWORD ret;

    ENTRY("SleepEx(dwMilliseconds=%u, bAlertable=%d)\n", dwMilliseconds, bAlertable);

    ret = WaitForMultipleObjectsEx(0, NULL, FALSE, dwMilliseconds, bAlertable);

    if (ret == WAIT_TIMEOUT)
    {
        ret = 0;
    }

    LOGEXIT("SleepEx returns DWORD %u\n", ret);

    return ret;
}

/*++
Function:
    WaitOn

    Check if the object is signaled. If not signaled the current thread
    will be placed in the waiting list for this object.

    SetLastError() is already called when exiting this function with an error.

Parameters:
    DWORD type  : object type
    HANDLE handle : object handle
    SHMPTR wait_state : shared memory pointer to waiting thread's wait state

Return value:
    -1: an error occurred, SetLastError is called in this function.
    0: the event is signaled.
    1: if the event is not signaled.
    2: the object is a mutex which was abandoned
--*/
static 
WAITON_CODE
WaitOn(
       HOBJTYPE type,
       HANDLE handle,
       SHMPTR wait_state)
{
    WAITON_CODE ret = WOC_ERROR;

    switch (type)
    {
    case HOBJ_PROCESS:
        ASSERT("Shouldn't have gotten here : waiting on processes doesn't use "
              "the pipe mechanism.\n");
        SetLastError(ERROR_INTERNAL_ERROR);
        break;
    case HOBJ_EVENT:
        ret = EventWaitOn(handle, wait_state);
        break;
    case HOBJ_SEMAPHORE:
        ret = SemaphoreWaitOn(handle, wait_state);
        break;
    case HOBJ_THREAD:
        ret = ThreadWaitOn(handle, wait_state);
        break;
    case HOBJ_MUTEX:
        ret = MutexWaitOn(handle, wait_state);
        break;
    default:
        SetLastError(ERROR_INVALID_HANDLE);
        ASSERT("Unsupported handle type\n");
    }

    return ret;
}

/*++
Function:
    StopWaitingOnObjects

    Remove the current thread from the waiting thread list of all
    objects passed in parameter (handle).

    It also clears the current thread pipe, so the pipe will be
    ready fo the next WaitForMultipleObjectsEx call.

Parameters:
    HOBJSTRUCT **hObjs : array of locked object structures
    HANDLE *pHandles : array of object handle
    int Count : number of objects
    int process_index : index of process handle in the array (or -1 if none)

Return value:
    the position of the last object where the current thread was not
    in the waiting thread list.
    -1 if all objects have the current thread in the waiting list.
--*/
static 
int
StopWaitingOnObjects(
            HOBJSTRUCT **hObjs,
            CONST HANDLE *pHandles,
            int Count,
            int process_index)
{
    int i;
    int retValue = -1;
    int pipe;
    struct pollfd fds;
    DWORD WakeupCode;
    int ret;

    for (i = 0; i < Count; i++)
    {
        /* remove the current thread from the waiting list of object i */
        if(i == process_index)
        {
            /* this is the process handle; stop waiting on the event instead of 
               on the process */
            ret = StopWaiting(HOBJ_EVENT, process_event);

            TRACE("telling WFMO worker thread to stop waiting on process\n");

            /* tell worker thread to stop waiting (if it is waiting; if not, it 
               will tell the worker thread that we've reached this function) */
            SetEvent(worker_event);
        }
        else
        {
            ret = StopWaiting(hObjs[i]->type, pHandles[i]);
        }
        if (ret == 0)
        {
            retValue = i;
        }

        /* even if StopWaiting returns an error(-1), we should try to 
           stop waiting on all the handles. It is possible the handle has
           been deleted while waiting for it */
    }

    /* We have to make sure the thread pipe is empty.
       It is possible than more than one thread have 
       tried to wake up this thread */
        
    pipe = THREADGetPipe();

    fds.fd = pipe;
    fds.events = POLLIN;
    fds.revents = 0;


    /* loop until there's no data in the pipe */
    while (TRUE)
    {
#if HAVE_BROKEN_FIFO_SELECT
        struct stat stat;
#endif  /* HAVE_BROKEN_FIFO_SELECT */
        ret = poll(&fds, 1, 0);

        if (ret == -1)
        {
            if(EINTR == errno)
            {
                TRACE("poll() failed with EINTR; re-polling.\n");
                continue;
            }
            ASSERT("poll() failed; errno is %d (%s)\n", errno, strerror(errno));
            break;
        }

        if (ret == 0)
        {
            /* timeout reach, there's no data */
            break;
        }

#if HAVE_BROKEN_FIFO_SELECT
        if (fstat(pipe, &stat) == -1)
        {
            ASSERT("fstat failed on the thread pipe\n");
        }
        else
        {
            if (stat.st_size == 0)
            {
                /* No data on the pipe */
                break;
            }
        }
#endif  /* HAVE_BROKEN_FIFO_SELECT */
        /* there's at least one DWORD in the pipe, clear it */
        TRACE("Clean up thread pipe\n");
        if ( read( pipe, &WakeupCode, sizeof( DWORD ) ) != sizeof( DWORD ) )
        {
            ASSERT("Unable to clear the thread pipe\n");
        }
    }

    /* if there was a process handle, we need to reset the process_handle 
       variable to NULL now that we're done with it */
    if( process_index >=0 )
    {
        /* if the worker thread was activated, it may still be using the 
           process handle; we need to wait for it to go back to standby mode */
        if( process_index < Count )
        {
            TRACE("waiting for WFMO worker thread to go in standby\n");
            WaitForSingleObject(standby_event, INFINITE);
            TRACE("standby_event got signaled, WFMO worker thread is done\n");
        }

        process_handle = NULL;
    }

    return retValue;
}

/*++
Function:
    StopWaiting

    Remove the current thread from the waiting thread list of the
    object passed in parameter (handle).

Parameters:
    DWORD type  : object type
    HANDLE handle : object handle

Return value:
    -1: an error occurred
    0: if the thread wasn't found in the list.
    1: if the thread was removed from the waiting list.
--*/
static 
int
StopWaiting(
            DWORD type,
            HANDLE handle)
{
    int ret = -1;

    switch (type)
    {
    case HOBJ_PROCESS:
        ASSERT("Shouldn't have gotten here : waiting on processes doesn't use "
              "the pipe mechanism.\n");
        break;
    case HOBJ_EVENT:
        ret = EventRemoveWaitingThread(handle);
        break;
    case HOBJ_SEMAPHORE:
        ret = SemaphoreRemoveWaitingThread(handle);
        break;
    case HOBJ_THREAD:
        ret = ThreadRemoveWaitingThread(handle);
        break;
    case HOBJ_MUTEX:
        ret = MutexRemoveWaitingThread(handle);
        break;
    default:
        ASSERT("Unsupported handle type\n");
    }

    return ret;
}

/*++
Function:
    PollBlockingPipe

    Helper function for ThreadWait.  Blocks the thread's blocking pipe
    until something is written to it.

Parameters:
    DWORD Milliseconds:  timeout value
    int blockingPipe  :  Thread's blocking pipe

Return value:
     1 - Poll executed properly, socket is ready for reading
     0 - Poll executed properly, wait timed out
    -1 - Poll had an error
--*/
static
int PollBlockingPipe(DWORD Milliseconds, int blockingPipe, DWORD *pWaitState)
{
    struct pollfd Poll;
    int poll_timeout;
    int poll_retval;
    DWORD old_time;
    DWORD old_waitstate;

    old_time = GetTickCount();

    /* repeat until we're done. there are 2 reasons to poll more than once :
       -timeout value is > INT_MAX; since poll() takes an int instead of a 
        DWORD, we may need to call poll multiple times to use up the whole 
        timeout value.
       -poll() fails with EINTR (interrupted by signal) : when this happens, 
        the proper action is to poll again 
     */
    while(1)
    {
        /* determine poll() timeout value */
        if(INFINITE == Milliseconds)
        {
            poll_timeout = INFTIM;
        }
        else if(INT_MAX < Milliseconds)
        {
            poll_timeout = INT_MAX;
        }
        else
        {
            poll_timeout = Milliseconds;
        }
        
        Poll.fd = blockingPipe;
        Poll.events = POLLIN;
        Poll.revents = 0;
        poll_retval = poll( &Poll, 1, poll_timeout );

        /* check result of poll(), take proper action (break o loop) */
        if(0 == poll_retval)
        {
            /* wait timed out; see if we should wait again */

            if(0 == poll_timeout)
            {
                /* non-blocking : we're done */
                break;
            }
            if(Milliseconds == poll_timeout)
            {
                /* delay was < INT_MAX, no need to block again */
                break;
            }
            /* remove elapsed time from timeout */
            WFMO_update_timeout(&old_time, &Milliseconds);
        }
        else if(-1 == poll_retval)
        {
            if(EINTR != errno)
            {
                /* unexpected error */
                ASSERT("poll() failed with %d (%s)\n", errno, strerror(errno));
                break;
            }
            TRACE("poll() failed with EINTR; re-polling\n");

            /* update the timeout if necessary */
            if(0 != Milliseconds && INFINITE != Milliseconds)
            {
                WFMO_update_timeout(&old_time, &Milliseconds);
            }
        }
        else
        {
#if HAVE_BROKEN_FIFO_SELECT
            struct stat stat;
            if (fstat(blockingPipe, &stat) == -1)
            {
                ASSERT("fstat failed on the blocking pipe\n");
            }
            else
            {
                if (stat.st_size == 0)
                {
                    /* No data on the pipe */
                    continue;
                }
            }
#endif  /* HAVE_BROKEN_FIFO_SELECT */
            /* no error, no timeout : socket is ready. we're done. */
            break;
        }                                                        
    }

    if(0 == poll_retval)
    {
        /* timeout reached. set wait state back to 'active */

        old_waitstate = InterlockedCompareExchange(pWaitState, TWS_ACTIVE, 
                                                   TWS_WAITING);
        if(TWS_ALERTABLE == old_waitstate)
        {
            /* thread is alertable instead of just waiting. let's try again */
            old_waitstate = InterlockedCompareExchange(pWaitState, TWS_ACTIVE, 
                                                       TWS_ALERTABLE);
        }
        if(TWS_ACTIVE == old_waitstate)
        {
            /* oops, we were already 'active'; someone decided to wake us up 
               sometime between the moment poll() timed out and here. Report a 
               signal instead of a timeout */
            poll_retval = 1;
        }
    }

    return poll_retval;
}


/*++
Function:
    ThreadWait

    Block the current thread

Parameters:
    DWORD Milliseconds:  timeout value
    BOOL  bAlertable:    Determines whether to call the APCs or not

Return value:
    WAKEUPTHREAD_CODE - The wakeup code sent by WakeupThread over the pipe, 
        
--*/
static 
WAKEUPTHREAD_CODE
ThreadWait( DWORD Milliseconds, BOOL bAlertable, DWORD *pWaitState )
{
    int pipe;
    WAKEUPTHREAD_CODE WakeupCode=0;
    int ret;
    int NumAPCCalled=0;
    BOOL ExitBlockLoop=FALSE;
    DWORD old_time;
    

    TRACE("Current Thread is blocking\n");
    pipe = THREADGetPipe();

    /* Save current time, so that we can know how much time has elapsed 
       (only needs to be done once, gets updated in WFMO_update_timeout() ) */
    old_time = GetTickCount(); 
        
    TRACE("Starting to wait, at time %u\n", old_time);

    ret = PollBlockingPipe(Milliseconds, pipe, pWaitState);

    if (ret == -1)
    {
        ASSERT("PollBlockingPipe() failed! errno is %d (%s)\n",
              errno, strerror(errno));
        return WUTC_NONE;
    }

    if (ret != 0)
    {
        TRACE("Read thread pipe\n");
        /* there's at least one DWORD in the pipe, clear it */
        if ( read(pipe,&WakeupCode,sizeof WakeupCode) != sizeof WakeupCode )
        {
            ASSERT("Unable to clear the thread pipe\n");
            return WUTC_NONE;
        }

        if(WUTC_NONE>= WakeupCode || WUTC_LAST<= WakeupCode)
        {
            ASSERT("got unknown wakeup code %d from the pipe!\n",
                   WakeupCode);
            return WUTC_NONE;
        }

        /* If we receive WUTC_APC_QUEUED as a wakeup code, that means 
           that QueueUserAPC() was called APC(es) were queued on our 
           thread.  QueueUserAPC() unblocks us by writing this code 
           into our pipe after queuing the APC. 
        */
        if (WUTC_APC_QUEUED == WakeupCode)
        {
            if(!bAlertable)
            {
                /* WUTC_APC_QUEUED shouldn't be sent if we aren't in an 
                   alertable wait */
                ASSERT("Got woken up by QueueUserAPC, but we're not "
                       "alertable!\n");
                return WUTC_NONE;
            }

            NumAPCCalled = THREADCallThreadAPCs();
            
            /* If NumAPCCalled == 0, that means no APCs were queued.
               This means that the APCs were handled by WFMO before a block 
               was necessary.  (ie. QueueUserAPC was called BEFORE WFMO.)  
               We still need to block in this case, so we will loop back
               and do another poll/read.  */
            if (NumAPCCalled == -1)
            {
                ASSERT("Failed in calling APCs for the current thread\n");
                return WUTC_NONE;
            }
            else if (NumAPCCalled > 0)
            {
                TRACE("%d APCs called; done waiting\n", NumAPCCalled);
                ExitBlockLoop = TRUE;
            }
            else
            {
                /* since we only get WUTC_APC_QUEUED from QueueUserAPC, there 
                   has to be at least 1 APC queued */
                ASSERT("got woken up by QueueUserAPC, but there were no APCs "
                       "queued!\n");
            }      
        }
    }
    
    TRACE("Current Thread is unblocked\n");
    return (WakeupCode);
}





/*++
Function:
    WakeUpThread

    Unblock a waiting thread

Parameters:
    DWORD ThreadId    Thread to wake up
    DWORD ProcessId   Process of the thread to wake up
    int   ThreadPipe  Blocking pipe associated with the thread to wake up.
    WAKEUPTHREAD_CODE WakeUpCode Code to use when waking up the thread

Return value:
    VOID
--*/
VOID
WakeUpThread( DWORD ThreadId,
              DWORD ProcessId,
              int   ThreadPipe,
              WAKEUPTHREAD_CODE WakeUpCode )
{
    BOOL closePipe = FALSE;

    if(WUTC_NONE>= WakeUpCode || WUTC_LAST<=WakeUpCode)
    {
        ASSERT("Got unknown wakeup code %d\n", WakeUpCode);
        return;
    }

    if (GetCurrentProcessId() != ProcessId)
    {
        /* need to open the pipe, the pipe fd received in parameter
           is only valid in the context of the' ProcessId' process */

        char PipeFilename[MAX_PATH];
        
        if(FALSE == THREADGetThreadPipeName(PipeFilename, MAX_PATH, ProcessId, 
                                            ThreadId))
        {
            /* if this fails createBlockingPipe() should also have failed, 
               the thread shouldn't even exist */
            ASSERT("couldn't retrieve threadpipe's name!\n");
            return;
        }

        ThreadPipe = open(PipeFilename, O_WRONLY);

        if (ThreadPipe == -1)
        {
            ASSERT("Unable to open the thread pipe to wake up the thread\n");
            return;
        }

        closePipe = TRUE;
    }

    /* unblock the thread */
    if ( write(ThreadPipe,&WakeUpCode,sizeof WakeUpCode) != sizeof WakeUpCode )
    {
        ASSERT("Unable to write in the pipe to wakeup the thread (errno=%d)\n",
              errno);
        return;
    }

    if ( closePipe )
    {
        /* close the opened pipe */
        close(ThreadPipe);
    }
}

/*++
Function:
    WFMO_update_timeout

    Substract elapsed time from a timeout value

Parameters:
    DWORD *old_time : pointer to previous system time
    DWORD *timeout : pointer to timeout value relative.

--*/
static void WFMO_update_timeout(DWORD *old_time, DWORD *timeout)
{
    DWORD new_time;
    DWORD delta_time;
    new_time = GetTickCount();
    
    /* check for wrap around */
    if(new_time<*old_time)
    {
        TRACE("GetTickCount wrapped around!\n")
        delta_time = new_time + (UINT_MAX - *old_time);
    }
    else
    {
        delta_time = new_time - *old_time;
    }
    *old_time = new_time;
    if(*timeout > delta_time)
    {
        *timeout -= delta_time;
    }
    else
    {
        *timeout = 0;
    }
    TRACE("%d milliseconds left on timeout\n", *timeout);
}

/*++
Function:
    WFMO_WaitForAllObjects

    Implementation of bWaitAll support for WaitForMultipleObjects

Parameters:
    int nCount, CONST HANDLE *lpHandles, DWORD dwMilliseconds :
        (see WaitForMultipleObjects)
    HOBJSTRUCT **hObjs : locked object structures
    
Return value :
    valid WaitForMultipleObjects return value                      
--*/
static DWORD WFMO_WaitForAllObjects(int nCount, CONST HANDLE *lpHandles, 
                              HOBJSTRUCT **hObjs, DWORD dwMilliseconds)
{
    int i;
    int ret;
    int resetable_count = 0;
    int blocking_object;
    DWORD old_time;
    HANDLE resetables[MAXIMUM_WAIT_OBJECTS];
    HOBJSTRUCT *resetable_objs[MAXIMUM_WAIT_OBJECTS];
    BOOL was_abandoned[MAXIMUM_WAIT_OBJECTS];
    BOOL got_abandoned;

    /* Save current time, so that we can know when the timeout is elapsed */
    old_time = GetTickCount(); 
    TRACE("Starting to wait, at time %u\n", old_time);

    /* Step 1 : wait for processes and threads. Once those are signalled, we 
      don't need to "unsignal" them and wait on them again if other objects 
      aren't signalled, and there's no point in waiting on the others as long 
      as processes and threads aren't signalled, so this minimizes the number 
      of calls to WaitForSingleObjects in the while() loop of step 2 */
    for(i=0;i<nCount;i++)
    {
        if( HOBJ_PROCESS == hObjs[i]->type ||
            HOBJ_THREAD  == hObjs[i]->type )
        {
            TRACE("Object #%d (%p) is a process or a thread; waiting for "
                  "its termination\n", i, lpHandles[i]);
            ret = WaitForSingleObject(lpHandles[i], dwMilliseconds);
            if(WAIT_TIMEOUT == ret)
            {
                TRACE("Timeout reached while waiting for object %p!\n", 
                      lpHandles[i]);
                return WAIT_TIMEOUT;
            }
            TRACE("Object %p has been signalled.\n", lpHandles[i]);
        }
        else
        {
            TRACE("Object #%d (%p) is resetable object #%d; adding it to "
                  "the list\n", i, lpHandles[i], resetable_count);

            /* Build arrays of "resetable" objects (mutexes, events and 
              semaphores) for use below. */
            resetables[resetable_count] = lpHandles[i];
            resetable_objs[resetable_count] = hObjs[i];

            resetable_count++;
        }

        /* If we have a timeout value (not infinite), remove from it the time 
           we spent in this pass, and see if we reached zero */
        if( 0 != dwMilliseconds && INFINITE != dwMilliseconds )
        {
            WFMO_update_timeout(&old_time, &dwMilliseconds);
            if(0 == dwMilliseconds)
            {
                TRACE("Timeout reached while waiting for processes & threads!");
                return WAIT_TIMEOUT;
            }
        }
    }

    /* If only process and thread objects were given, we can stop here */
    if( 0 == resetable_count)
    {
        TRACE("All process and thread objects have terminated, no resetable "
              "objects specified : we're done waiting.\n");
        return WAIT_OBJECT_0;
    }

    TRACE("All process and thread objects have terminated; now waiting on "
          "%d resetable objects\n", resetable_count);

    /* Step 2 : We now wait for one of the resetable objects, using what's 
       left of the timeout value. Once the object is signalled, we'll see if 
       all other objects are signalled by waiting on each with a timeout of 0 
       (nonblocking). If an object isn't signalled, we have to re-signal all 
       objects for which we got a signal, and wait again. */
    blocking_object = 0;
    while(1)
    {
        TRACE("Waiting for object %p for %u milliseconds\n", 
              resetables[blocking_object], dwMilliseconds);
        ret = WaitForSingleObject( resetables[blocking_object], dwMilliseconds);
        if(WAIT_TIMEOUT == ret)
        {
            /* timed out during wait : we're done */
            TRACE("Timeout reached while waiting for object %08x!\n",
                  blocking_object);
            return WAIT_TIMEOUT;
        }
        got_abandoned = was_abandoned[blocking_object] = WAIT_ABANDONED==ret;
        
        TRACE("Now trying non-blocking wait on all remaining objects...\n");
        for(i=0; i<resetable_count;i++)
        {
            /* skip the first object we waited on, it's already signalled */
            if(i == blocking_object)
            {
                continue;
            }
            /* non-blocking wait on all others, to see if they're currently 
               signalled */
            ret = WaitForSingleObject(resetables[i], 0);

            /* timeout means an object wasn't signalled : break out of the loop, 
               we have to start over */
            if(WAIT_TIMEOUT == ret)
            {
                TRACE("Object %p isn't signalled, restarting blocking wait\n", 
                      resetables[i]);
                break;
            }
            
            was_abandoned[i] = WAIT_ABANDONED == ret;
            if(was_abandoned[i])
            {
                got_abandoned = TRUE;
            }
        }

        /* if the for loop completed normally (no break), all object were 
           signalled. we're done. */
        if(i == resetable_count)
        {
            TRACE("All objects have been signalled, we're done waiting.\n");
            if(got_abandoned)
            {
                return WAIT_ABANDONED;
            }
            return WAIT_OBJECT_0;
        }

        /* save the object that wasn't signalled. we'll wait on that one in the 
           next pass, since we know that one isn't signalled yet. (if we always 
           waited on the first object, we'd end up busy waiting) */
        blocking_object = i;
        TRACE("Object %08x wasn't signalled; we'll start the next pass by "
              "waiting on it.\n", blocking_object);

        /* give up ownership or re-signal all events we succesfully waited on, 
           to allow other threads to un-block while we're waiting for all 
           obects to get signalled */
        TRACE("Failed to acquire all objects : now releasing all those we "
              "acquired\n");
        for(; i>=0;i--)
        {
            switch(resetable_objs[i]->type)
            {
            case HOBJ_MUTEX:
                TRACE("Releasing Mutex object %p\n", resetables[i]);
                if(was_abandoned[i])
                {
                    /* re-flag the mutex as abandoned if tha's how it was 
                       before we acquired it */
                    MutexReleaseMutex((MUTEX_HANDLE_OBJECT *)resetable_objs[i],
                                      WUTC_ABANDONED, FALSE);
                }
                else
                {
                    ReleaseMutex(resetables[i]);
                }
                break;
            case HOBJ_SEMAPHORE:
                TRACE("Releasing Semaphore object %p\n", resetables[i]);
                ReleaseSemaphore(resetables[i], 1, NULL);
                break;
            case HOBJ_EVENT:
                /* note : for manual-reset events, this isn't really necessary, 
                   but it doesn't hurt and avoids the need to determine the 
                   type of event */
                TRACE("Re-setting Event object %p\n", resetables[i]);
                SetEvent(resetables[i]);
                break;
            default:
                break;
            }
        }
        /* If we have a timeout value (not infinite), remove from it the time 
           we spent in this pass, and see if we reached zero */
        if( 0 != dwMilliseconds && INFINITE != dwMilliseconds )
        {
            WFMO_update_timeout(&old_time, &dwMilliseconds);
            if(0 == dwMilliseconds)
            {
                /* reached zero : time is up, we're done.*/
                TRACE("Timeout reached while releasing resetable objects!\n");
                return WAIT_TIMEOUT;
            }
        }
    }
}

#define PROCWAIT_DELAY 250 /* 1/4 second delay between waitpid() calls */
/*++
Function:
    WFMO_WaitForProcess

    Implementation of HPROCESS support for WaitForMultipleObjects

Parameters:
    HANDLE hProcess : process on which to wait
    DWORD dwMilliseconds : timeout value
    BOOL interruptible : whether the caller is the worker thread (see notes)
    
Return value :
    appropriate return value for WaitForMultipleObjects
    Exception : WAIT_ABANDONED is used to report that the wait was interrupted 
    because the worker thread's event got signaled. This doesn't match the 
    semantics of WAIT_ABANDONED, but this is strictly internal, so there's no 
    conflict.
                      
Notes : 
In order to support waiting on non-PAL processes, we must use kill() instead 
of the pipe mechanism used for other handle types. This imposes one restriction:
it is not possible to wait on a process at the same time as other handle types. 
To work around this, a worker thread can be used; the worker thread waits on 
the process, and signals an event (which does use the pipe mechanism) when the 
process is signalled. This is not necessary if bWaitAll was TRUE (in this case 
it is possible to wait on process handles before waiting on other handle types)
or if the process handle is the only handle to wait on (in which case this 
function can be called directly)
-if 'interruptible' is true, the call comes from the worker thread. instead of 
Sleep()ing between waitpid calls, we need to use WFSO on the worker event; this
is so that WFMO can notify us if we should stop waiting (because another object 
got signaled)
--*/
static DWORD WFMO_WaitForProcess(HANDLE hProcess, DWORD dwMilliseconds, 
                                 BOOL interruptible)
{
    DWORD exit_code;
    DWORD dwRet;
    DWORD time_to_wait;
    BOOL gotExitCode;

    TRACE("Waiting on process handle %p\n", hProcess);
    
    gotExitCode = GetExitCodeProcess(hProcess,&exit_code);
    if (gotExitCode && STILL_ACTIVE != exit_code)
    {
        // We already know the process has terminated; nothing to do.
        return WAIT_OBJECT_0;
    }               

    // Loop until the process is found dead, the timeout elapses, or - for 
    // interruptible waits - the worker thread event is signaled.
    while(1)
    {
        gotExitCode = GetExitCodeProcess(hProcess, &exit_code);
        if (gotExitCode && exit_code != STILL_ACTIVE)
        {
            dwRet = WAIT_OBJECT_0;
            break;
        }
        
        /* if we've reached the end of the timeout, stop trying */
        if( dwMilliseconds == 0 )
        {
            dwRet = WAIT_TIMEOUT;
            break;
        }

        TRACE("Process still alive, timeout not reached (%d milliseconds left);"
              "sleeping a little.\n", dwMilliseconds);
        
        /* decrement the timeout if it's not infinite */
        if(INFINITE != dwMilliseconds) 
        {
            if(PROCWAIT_DELAY<=dwMilliseconds)
            {
                time_to_wait = PROCWAIT_DELAY;
                dwMilliseconds -= PROCWAIT_DELAY;
            }
            else
            {
                time_to_wait = dwMilliseconds;
                dwMilliseconds = 0;
            }
        }
        else
        {
            time_to_wait = PROCWAIT_DELAY;
        }

        if(interruptible) 
        {
            DWORD wfso_retval;
            /* this is the worker thread : we need to check if the worker event 
               is signaled; this would mean WFMO got woken up by someone else, 
               and we should stop waiting. using WFSO, we combine the sleep and 
               the event check */

            TRACE("blocking on worker_event for %d milliseconds\n", 
                  time_to_wait);
            wfso_retval = WaitForSingleObject(worker_event, time_to_wait);
            if(WAIT_TIMEOUT != wfso_retval)
            {
                /* no timeout : the event got signaled. this means WFMO is 
                  already unblocked, and we can stop waiting on the process */
                /* since this function is used to wait on a process and not a 
                   mutex, it should never return WAIT_ABANDONED. that means we 
                   can use it as a special value for our own purposes */
                dwRet = WAIT_ABANDONED;
                TRACE("worker_event got signaled; no need to wait on process "
                      "anymore\n");
                break;
            }
        }
        else
        {
            /* not worker thread : sleep a little before the next kill() */
            Sleep(time_to_wait);
        }
    }
    return dwRet;
}

/*++
Function:
  WFMOStopWorkerThread
  
  Terminate the WFMO worker thread (if it's active)

(no parameters, no return value)
--*/
void WFMOStopWorkerThread(void)
{
    TRACE("now terminating WFMO worker thread\n");

    /* set a flag to tell the worker thread to stop */
    keep_going=FALSE;

    /* no need to do anything if the worker thread was never started */
    if(worker_handle)
    {
        /* unblock worker if it is blocked, so it can notice keepgoing==FALSE */
        SetEvent(worker_event);

        /* wait for worker to terminate : we don't want to close its handles 
           while it's still using them */
        WaitForSingleObject(worker_handle,INFINITE); 

        TRACE("WFMO worker thread has terminated\n");

        /* get rid of these handles, we don't need them anymore */
        CloseHandle(standby_event); 
        CloseHandle(worker_event); 
        CloseHandle(worker_handle);
        CloseHandle(process_handle);

        /* indicate that there is no active worker thread */
        worker_handle = NULL; 
    }
}

/*++
Function:
  WFMO_workerthread
  
  Thread routine of WFMO worker thread, used to wait on process handles 
  concurrently with other handle types

(parameter is unused, return value always 0)
--*/
static DWORD_PTR PALAPI WFMO_workerthread(LPVOID param)
{
    DWORD ret;

    TRACE("WFMO worker thread launched!\n");

    /* we need to check for "keep_going" *after* getting signaled, since 
       WFMOStopWorkerThread signals us to tell us to stop. this means the WFSO
       call needs to go at the end of the loop, so we need to call it before 
       the lo   op for the first time */
    WaitForSingleObject(worker_event, INFINITE);
    while(keep_going)
    {
        TRACE("worker_event got signaled! now waiting on process %#x\n", 
              process_handle);
        
        /* we have a process to wait on! do so now. */
        ret = WFMO_WaitForProcess(process_handle, worker_timeout, TRUE);
        switch(ret)
        {
        case WAIT_OBJECT_0:
            TRACE("process has terminated; now signaling process_event\n");

            /* process terminated. signal the waiting thread. note that the 
               waiting thread may *already* be awake; if it is, we have to 
               Reset the event, since we don't want the next wait on it to 
               wake up spuriously. We can "toggle" the event (Set/Reset) to do 
               this, because by the time we get here, we know the waiting 
               thread has added itself on the event's waiting list; this means 
               SetEvent will instantly wake it up and get reset if the thread 
               is still waiting */
            SetEvent(process_event);
            ResetEvent(process_event);
            break;
        case WAIT_TIMEOUT:
            TRACE("WaitForProcess timed out! now waiting for WFMOEx to time "
                  "out too\n");
            
            /* timed out. wait for the waiting thread to time out too and tell 
               us to stop waiting (in StopWaitingOnObjects) */
            break;
        case WAIT_ABANDONED:
            /* some other event got triggered. we're done for today */
            /* note : WAIT_ABANDONED isn't really meant for this, but 
               WaitForProcess can't normally return it (it only gets returned 
               for waits on mutexes), so we can do whatever we want with it */
            /* note also that in this case there's no need to wait on 
               worker_event, since we got here precisely because the waiting 
               thread has already signaled it and woke us up */
            TRACE("worker_event got signaled while waiitng on the process\n");
            break;
        default:
            ASSERT("got unexpected value %d from WFMOWaitForProcess\n",ret);
        }                 
        /* now we need to tell the waiter that we're going back in standby. We 
        can't use the process_event for this : this would require the waiting 
        thread to wait on it a second time, which would introduce a race 
        condition with the Set/Reset toggle above. So we use a 3rd event */
        SetEvent(standby_event);

        /* wait for WFMO to have work for us again */
        WaitForSingleObject(worker_event, INFINITE);
    }   
    TRACE("keep_going is FALSE! now terminating WFMO worker thread\n");
    /* don't call ExitThread, we don't want to call DllMain */
    TerminateCurrentThread(0);
    return 0;
}




