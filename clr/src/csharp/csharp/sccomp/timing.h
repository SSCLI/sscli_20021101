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
// File: timing.h
//
// Defined the timer functions, which allow reporting performance
// information for the compiler. This is sort of a built-in mini-profiler
// that is always available. This allows anyone to do a quick investigation of
// performance problems and try to pinpoint where things have changed.
// ===========================================================================

// Create the TIMERID enum. To add a new timerid, edit the timerids.h file.
#define TIMERID(id, text, subtotal) id,
enum TIMERID {
    #include "timerids.h"
    TIMERID_MAX
};
#undef TIMERID



// Although it is verboten in most parts of the compiler, we use static
// data in the timings module. This means that the timings will
// not be correct if multiple compiles happen at the same time. Since
// this is an internal debugging tool anyway, that's OK for this
// one specific thing. It's important to be static because we want this 
// to be very low overhead if it's not in use.


// Is timing on?
extern bool g_isTimingActive;

// The timer functions.
extern void ActivateTiming();
extern void FinishTiming();
extern void ReportTimes(FILE * outputFile);
extern void DoTimerStart(TIMERID timerId);
extern void DoTimerStop(TIMERID timerId);

// Begin timing something.
__forceinline void TimerStart(TIMERID timerId)
{
    if (g_isTimingActive)
        DoTimerStart(timerId);
}

// Finish timing something
__forceinline void TimerStop(TIMERID timerId)
{
    if (g_isTimingActive)
        DoTimerStop(timerId);
}

// class to time a block of code
class TIMERBLOCK
{
public:
    TIMERBLOCK(TIMERID timerId) : timerId(timerId) { TimerStart(timerId); }
    ~TIMERBLOCK() { TimerStop(timerId); }
private:
    TIMERID timerId;
};

#define TIMEBLOCK(timerId) TIMERBLOCK __timerId(timerId)
