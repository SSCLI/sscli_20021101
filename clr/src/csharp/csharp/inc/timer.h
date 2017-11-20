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
// File: timer.h
//
// This is a poor-man's profiling timer.  In the debug build, you can use the
// STARTTIMER(name)/STOPTIMER() macros around a body of code you'd like to
// "profile".  When the STOPTIMER macro is hit, you'll see a debug output
// message like this:
//
// TIMER:  Took <time> to <name>
//
// You can also nest these.
//
// Note that these are also enabled/disabled by a debug switch (<timergroup>/Timers).
// <timergroup> is a string that must be defined in ONE source file in your
// project before including this file (i.e. #define DEFINE_TIMER_SWITCH "MyDllName")
//
// There is also a simpler set of macros to use, DECLARETIMER(name) and ELAPSEDTIME(name).
// These, under DEBUG, simply declare a DWORD to store the current time (DECLARETIMER)
// and return the current time delta (ELAPSEDTIME).  The usage pattern for this is:
//  
//  DECLARE_TIMER(dwTime);
//  ...(do stuff)
//  DBGOUT (("Took %d ms to %s", ELAPSEDTIME(dwTime), "do stuff"));
//  RESET_TIMER(dwTime);
//  ...(do more stuff)
//  DBGOUT (("Took %d ms to %s", ELAPSEDTIME(dwTime), "do more stuff"));
//
// These aren't under a switch -- only the "debug spew" switch.
// ===========================================================================

#ifndef _TIMER_H_
#define _TIMER_H_

#include "locks.h"

inline void do_nothing() {}

#ifdef DEBUG

// Each STARTTIMER/STOPTIMER adds one of these to a stack
struct DBGTIMERNODE
{
    DBGTIMERNODE   *pPrev;         // Previous timer node
    DWORD       dwTime;
    PCSTR       pszName;        // Name of this timer
};

// This is the timer manager
class CTimerStack
{
public:
    static  DBGTIMERNODE   *m_pStack;
    static  CTinyLock   m_lock;

    static  void    Start (PCSTR pszName);
    static  void    Stop ();
};

#ifdef DEFINE_TIMER_SWITCH

VSDEFINE_SWITCH(gfTimers, DEFINE_TIMER_SWITCH, "Timers");
DBGTIMERNODE   *CTimerStack::m_pStack = NULL;
CTinyLock   CTimerStack::m_lock;

void CTimerStack::Start (PCSTR pszName)
{
    CTinyGate   gate (&m_lock);

    DBGTIMERNODE   *pNew = (DBGTIMERNODE *)VSAlloc (sizeof (DBGTIMERNODE));
    if (pNew != NULL)
    {
        pNew->pPrev = m_pStack;
        pNew->dwTime = GetCurrentTime ();
        pNew->pszName = pszName;
        m_pStack = pNew;
    }
}

void CTimerStack::Stop ()
{
    if (m_pStack != NULL)
    {
        DBGTIMERNODE   *p = m_pStack;
        m_pStack = m_pStack->pPrev;
        DBGOUT (("TIMER:  Took %d ms to %s", GetCurrentTime() - p->dwTime, p->pszName));
        VSFree (p);
    }
    else
    {
        DBGOUT (("TIMER:  Mismatched STOPTIMER; no current timer set!"));
    }
}

#else

VSEXTERN_SWITCH(gfTimers);

#endif

#define STARTTIMER(n) if (VSFSWITCH(gfTimers)) { CTimerStack::Start (n); }
#define STOPTIMER() if (VSFSWITCH(gfTimers)) { CTimerStack::Stop (); }

#define DECLARE_TIMER(n) DWORD n = GetCurrentTime()
#define RESET_TIMER(n) n = GetCurrentTime()
#define ELAPSEDTIME(n) (GetCurrentTime() - n)

#else

// Retail builds these go away completely
#define STARTTIMER(n)
#define STOPTIMER()

#define DECLARE_TIMER(n) do_nothing()
#define RESET_TIMER(n) do_nothing()
#define ELAPSEDTIME(n) 0

#endif



#define STARTCAP() do_nothing()
#define STOPCAP() do_nothing()
#define SUSPENDCAP() do_nothing()
#define RESUMECAP() do_nothing()


#endif  // _TIMER_H_
