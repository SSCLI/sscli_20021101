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
// ===========================================================================
// File: timing.cpp
//
// Defined the timer functions, which allow reporting performance
// information for the compiler. This is sort of a built-in mini-profiler
// that is always available. This allows anyone to do a quick investigation of
// performance problems and try to pinpoint where things have changed.
// ===========================================================================

#include "stdafx.h"

#include "compiler.h"

// Although it is verboten in most parts of the compiler, we use static
// data in the timings module. This means that the timings will
// not be correct if multiple compiles happen at the same time. Since
// this is an internal debugging tool anyway, that's OK for this
// one specific thing. It's important to be static because we want this 
// to be very low overhead if it's not in use.

bool g_isTimingActive = false;

LARGE_INTEGER g_qpcStartTime;  // In units returns by QueryPerformanceCounter
LARGE_INTEGER g_qpcStopTime;
        
__int64 g_startTime;            // In units returned by GetCurrentTimerTick
__int64 g_lastTime;
__int64 g_stopTime;
TIMERID g_timeridStack[1000];
int g_timeridStackPtr;

struct TIMERSECTIONINFO {
    LPSTR            name;
    int              subTotal;
};

struct TIMERSECTIONDATA {
    unsigned  totalCount;
    __int64   totalTime;
};

TIMERSECTIONDATA g_timerData[TIMERID_MAX];

#define TIMERID(id, text, subtotal) { text, subtotal} ,
const TIMERSECTIONINFO g_timerInfo[TIMERID_MAX] = {
    #include "timerids.h"
};
#undef TIMERID



// QueryPerformanceCounter is slow enough that is skews the
// timing somewhat. The rdtsc (read cycle counter) instruction
// is much better. We have to restrict execution to a single
// processor to make it reliable, though.

__forceinline __int64 GetCurrentTimerTick()
{
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return li.QuadPart;
}

// Initialize the counter, if needed.
void InitializeTimerTick()
{}



/* 
 * Start the timing and reset all timer counts.
 */
void ActivateTiming()
{

    for (TIMERID id = (TIMERID) 0; id < TIMERID_MAX; id = (TIMERID) (id + 1))
    {
        g_timerData[id].totalCount = 0;
        g_timerData[id].totalTime = 0;
    }

    InitializeTimerTick();
    g_stopTime = 0;
    g_timeridStackPtr = -1;
    g_isTimingActive = true;
    QueryPerformanceCounter(&g_qpcStartTime);
    g_lastTime = g_startTime = GetCurrentTimerTick();
}

/*
 * Stop the timing.
 */
void FinishTiming()
{
    g_stopTime = GetCurrentTimerTick();
    QueryPerformanceCounter(&g_qpcStopTime);
    g_isTimingActive = false;
}

/* 
 * Record the start of a new section of timing
 */
void DoTimerStart(TIMERID timerId)
{
    __int64 now = GetCurrentTimerTick();
    int stackPtr = g_timeridStackPtr;
    TIMERID oldId;

    // Record the amount of time so far in the containing section, if any.
    if (stackPtr >= 0) {
        oldId = g_timeridStack[stackPtr];
        g_timerData[oldId].totalTime += (now - g_lastTime);
    }

    // update the stack
    ++stackPtr;
    g_timeridStack[stackPtr] = timerId;
    g_timeridStackPtr = stackPtr;

    // update the count and remember when we started.
    ++g_timerData[timerId].totalCount;
    g_lastTime = now;
}


/* 
 * Record the end of a section of timing
 */
void DoTimerStop(TIMERID timerId)
{
    __int64 now = GetCurrentTimerTick();
    int stackPtr = g_timeridStackPtr;

    // Pop the stack. If the id doesn't match, pop until it does (exception thrown, maybe?)
    while (g_timeridStack[stackPtr] != timerId) {
        --stackPtr;
        ASSERT(stackPtr >= 0);
    }
    --stackPtr;

    // Record the amount of time so far in the current section, if any.
    g_timerData[timerId].totalTime += (now - g_lastTime);
    g_timeridStackPtr = stackPtr;

    // remember the new time
    g_lastTime = now;
}

/*
 * Print a report of the times spent in each timed section to a file.
 */
extern void ReportTimes(FILE * outputFile)
{
    double elapsedTime = (double)(g_stopTime - g_startTime);
    LARGE_INTEGER qpcFreq;
    QueryPerformanceFrequency(& qpcFreq);
    double elapsedTimeMsec = (double)(g_qpcStopTime.QuadPart - g_qpcStartTime.QuadPart) / (double) qpcFreq.QuadPart * 1000.0;
    __int64 subTotal;
    __int64 total;

    fprintf(outputFile, "All times are mutually exclusive. Total compile time: %.1f ms.\n", elapsedTimeMsec);
    fprintf(outputFile, "\n");

    subTotal = 0;
    total = 0;

    fprintf(outputFile, "%-50s  %10s  %7s\n", "Name of code section", "Hits", "  Time %");
    fprintf(outputFile, "==========================================================================\n");
    for (TIMERID id = (TIMERID)0; id < TIMERID_MAX; id = (TIMERID) (id + 1))
    {
        if (g_timerData[id].totalCount != 0) {
            fprintf(outputFile, "%-50s  %10d  %7.3f%%\n", g_timerInfo[id].name, 
                                                          g_timerData[id].totalCount, 
                                                          (double) g_timerData[id].totalTime / elapsedTime * 100.0);
        }

        subTotal += g_timerData[id].totalTime;
        total += g_timerData[id].totalTime;

        if (id + 1 >= TIMERID_MAX || g_timerInfo[id + 1].subTotal != g_timerInfo[id].subTotal) {
            // print subtotal
            if (subTotal > 0) {
                fprintf(outputFile, "--------------------------------------------------------------------------\n");
                fprintf(outputFile, "%-50s  %10s  %7.3f%%\n\n", "SUBTOTAL", "",
                                                                (double) subTotal / elapsedTime * 100.0);
            }

            subTotal = 0;
        }
    }

    // print total
    fprintf(outputFile, "--------------------------------------------------------------------------\n");
    fprintf(outputFile, "%-50s  %10s  %7.3f%%\n\n", "TOTAL OF TIMED SECTIONS", "",
                                                    (double) total / elapsedTime * 100.0);
}
