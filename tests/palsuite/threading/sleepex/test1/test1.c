/*=====================================================================
**
** Source:  test1.c
**
** Purpose: Tests that SleepEx correctly sleeps for a given amount of time,
**          regardless of the alertable flag.
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

typedef struct
{
    DWORD SleepTime;
    BOOL Alertable;
} testCase;

testCase testCases[] =
{
    {0, FALSE},
    {50, FALSE},
    {100, FALSE},
    {500, FALSE},
    {1000, FALSE},

    {0, TRUE},
    {50, TRUE},
    {100, TRUE},
    {500, TRUE},
    {1000, TRUE},
};

/* Milliseconds of error which are acceptable Function execution time, etc.
   FreeBSD has a "standard" resolution of 10ms for waiting operations, so we 
   must take that into account as well */
DWORD AcceptableTimeError = 15; 

int __cdecl main( int argc, char **argv ) 
{
    DWORD OldTickCount;
    DWORD NewTickCount;
    DWORD MaxDelta;
    DWORD TimeDelta;
    DWORD i;

    if (0 != (PAL_Initialize(argc, argv)))
    {
        return FAIL;
    }

    for (i = 0; i<sizeof(testCases) / sizeof(testCases[0]); i++)
    {
        OldTickCount = GetTickCount();

        SleepEx(testCases[i].SleepTime, testCases[i].Alertable);

        NewTickCount = GetTickCount();

        /* 
         * Check for DWORD wraparound
         */
        if (OldTickCount>NewTickCount)
        {
            OldTickCount -= NewTickCount+1;
            NewTickCount  = 0xFFFFFFFF;
        }

        TimeDelta = NewTickCount - OldTickCount;

        /* For longer intervals use a 10 percent tolerance */
        if ((testCases[i].SleepTime * 0.1) > AcceptableTimeError)
        {
            MaxDelta = testCases[i].SleepTime + (DWORD)(testCases[i].SleepTime * 0.1);
        }
        else
        {
            MaxDelta = testCases[i].SleepTime + AcceptableTimeError;
        }

        if (TimeDelta < testCases[i].SleepTime || TimeDelta > MaxDelta)
        {
            Fail("The sleep function slept for %d ms when it should have "
             "slept for %d ms\n", TimeDelta, testCases[i].SleepTime);
       }
    }

    PAL_Terminate();
    return PASS;
}
