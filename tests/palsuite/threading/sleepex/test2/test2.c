/*=====================================================================
**
** Source:  test2.c
**
** Purpose: Tests that a child thread in the middle of a SleepEx call will be
**          interrupted by QueueUserAPC if the alert flag was set.
**
** 
**  Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
** 
**  The use and distribution terms for this software are contained in the file
**  named license.txt, which can be found in the root of this distribution.
**  By using this software in any fashion, you are agreeing to be bound by the
**  terms of this license.
** 
**  You must not remove this notice, or any other, from this software.
** 
**
**===================================================================*/

#include <palsuite.h>

const int ChildThreadSleepTime = 2000;
const int InterruptTime = 1000; 
/* We need to keep in mind that BSD has a timer resolution of 10ms, so
   we need to adjust our delta to keep that in mind */
const int AcceptableDelta = 20;

void RunTest(BOOL AlertThread);
VOID PALAPI APCFunc(ULONG_PTR dwParam);
DWORD PALAPI SleeperProc(LPVOID lpParameter);

DWORD ThreadSleepDelta;

int __cdecl main( int argc, char **argv ) 
{
    if (0 != (PAL_Initialize(argc, argv)))
    {
        return FAIL;
    }

  
    /* 
     * Check that Queueing an APC in the middle of a sleep does interrupt
     * it, if it's in an alertable state.
     */
    RunTest(TRUE);
    if (abs(ThreadSleepDelta - InterruptTime) > AcceptableDelta)
    {
        Fail("Expected thread to sleep for %d ms (and get interrupted).\n"
            "Thread slept for %d ms! (Acceptable delta: %d)", 
            InterruptTime, ThreadSleepDelta, AcceptableDelta);
    }

    /* 
     * Check that Queueing an APC in the middle of a sleep does NOT interrupt 
     * it, if it is not in an alertable state.
     */
    RunTest(FALSE);
    if (abs(ThreadSleepDelta - ChildThreadSleepTime) > AcceptableDelta)
    {
        Fail("Expected thread to sleep for %d ms (and not be interrupted).\n"
            "Thread slept for %d ms! (Acceptable delta: %d)\n", 
            ChildThreadSleepTime, ThreadSleepDelta, AcceptableDelta);
    }


    PAL_Terminate();
    return PASS;
}

void RunTest(BOOL AlertThread)
{
    HANDLE hThread = 0;
    DWORD dwThreadId = 0;
    int ret;

    hThread = CreateThread( NULL, 
                            0, 
                            (LPTHREAD_START_ROUTINE)SleeperProc,
                            (LPVOID) AlertThread,
                            0,
                            &dwThreadId);

    if (hThread == NULL)
    {
        Fail("ERROR: Was not able to create the thread to test SleepEx!\n"
            "GetLastError returned %d\n", GetLastError());
    }

    if (SleepEx(InterruptTime, FALSE) != 0)
    {
        Fail("The creating thread did not sleep!\n");
    }

    ret = QueueUserAPC(APCFunc, hThread, 0);
    if (ret == 0)
    {
        Fail("QueueUserAPC failed! GetLastError returned %d\n", GetLastError());
    }

    ret = WaitForSingleObject(hThread, INFINITE);
    if (ret == WAIT_FAILED)
    {
        Fail("Unable to wait on child thread!\nGetLastError returned %d.", 
            GetLastError());
    }
}

/* Function doesn't do anything, just needed to interrupt SleepEx */
VOID PALAPI APCFunc(ULONG_PTR dwParam)
{

}

/* Entry Point for child thread. */
DWORD PALAPI SleeperProc(LPVOID lpParameter)
{
    DWORD OldTickCount;
    DWORD NewTickCount;
    BOOL Alertable;
    DWORD ret;

    Alertable = (BOOL) lpParameter;

    OldTickCount = GetTickCount();

    ret = SleepEx(ChildThreadSleepTime, Alertable);
    
    NewTickCount = GetTickCount();


    if (Alertable && ret != WAIT_IO_COMPLETION)
    {
        Fail("Expected the interrupted sleep to return WAIT_IO_COMPLETION.\n"
            "Got %d\n", ret);
    }
    else if (!Alertable && ret != 0)
    {
        Fail("Sleep did not timeout.  Expected return of 0, got %d.\n", ret);
    }


    /* 
    * Check for DWORD wraparound
    */
    if (OldTickCount>NewTickCount)
    {
        OldTickCount -= NewTickCount+1;
        NewTickCount  = 0xFFFFFFFF;
    }

    ThreadSleepDelta = NewTickCount - OldTickCount;

    return 0;
}
