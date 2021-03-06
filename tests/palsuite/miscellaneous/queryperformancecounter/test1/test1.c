/*============================================================
**
** Source: test1.c
**
** Purpose: Test for QueryPerformanceCounter function
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
**=========================================================*/

/* Depends on: QueryPerformanceFrequency. */

#include <palsuite.h>

/* Milliseconds of error which are acceptable Function execution time, etc.
   FreeBSD has a "standard" resolution of 50ms for waiting operations, so we
   must take that into account as well */
DWORD AcceptableTimeError = 15; 

int __cdecl main(int argc, char *argv[])
{

    int           i;
    int           NumIterations = 100;
    DWORD         AvgTimeDiff;
    DWORD         TimeDiff[100];
    DWORD         TotalTimeDiff = 0;
    DWORD         SleepInterval = 50;
    LARGE_INTEGER StartTime;
    LARGE_INTEGER EndTime;
    LARGE_INTEGER Freq;

    /* Initialize the PAL.
     */

    if(0 != (PAL_Initialize(argc, argv)))
    {
        return FAIL;
    }

    /* Get the frequency of the High-Performance Counter,
     * in order to convert counter time to milliseconds.
     */
    if (!QueryPerformanceFrequency(&Freq))
    {
        Fail("ERROR:%u:Unable to retrieve the frequency of the "
             "high-resolution performance counter.\n", 
             GetLastError());
    }

    /* Perform this set of sleep timings a number of times.
     */
    for(i=0; i < NumIterations; i++)
    {

        /* Get the current counter value.
        */ 
        if (!QueryPerformanceCounter(&StartTime))
        {
            Fail("ERROR:%u:Unable to retrieve the current value of the "
                "high-resolution performance counter.\n", 
                GetLastError());
        }

        /* Sleep a predetermined interval.
        */
        Sleep(SleepInterval);

        /* Get the new current counter value.
        */
        if (!QueryPerformanceCounter(&EndTime))
        {
            Fail("ERROR:%u:Unable to retrieve the current value of the "
                "high-resolution performance counter.\n", 
                GetLastError());
        }

        /* Determine elapsed time, in milliseconds. Compare the elapsed time
         * with the sleep interval, and add to counter.
         */
        TimeDiff[i] = (DWORD)(((EndTime.QuadPart - StartTime.QuadPart)*1000)/
                             (Freq.QuadPart));
        TotalTimeDiff += abs(TimeDiff[i] - SleepInterval);

    }

    /* Verify that the average of the difference between the performance 
     * counter and the sleep interval is within our acceptable range.
     */
    AvgTimeDiff = TotalTimeDiff / NumIterations;
    if (AvgTimeDiff > AcceptableTimeError)
    {
        Fail("ERROR:  average diff %u acceptable %u.\n",
            AvgTimeDiff,
            AcceptableTimeError);
    }

    /* Terminate the PAL.
     */  
    PAL_Terminate();
    return PASS;
}


