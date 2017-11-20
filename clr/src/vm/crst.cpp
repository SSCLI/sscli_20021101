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
/*  CRST.CPP
 *
 */

#include "common.h"

#include "crst.h"
#include "log.h"

//-----------------------------------------------------------------
// Acquire the lock.
//-----------------------------------------------------------------
void CrstBase::Enter()
{
#ifdef _DEBUG
    PreEnter ();
#endif

    _ASSERTE(IsSafeToTake() || g_fEEShutDown);

#ifdef _DEBUG
    char buffer[100];
    sprintf(buffer, "Enter in crst.h - %s", m_tag);
    CRSTBLOCKCOUNTINCL();
    LOCKCOUNTINCL(buffer);
#endif

    EnterCriticalSection(&m_criticalsection);

    CRSTELOCKCOUNTINCL();

#ifdef _DEBUG
    m_holderthreadid = GetCurrentThreadId();
    m_entercount++;
    PostEnter ();
#endif
}


void CrstBase::IncThreadLockCount ()
{
    INCTHREADLOCKCOUNT();
}

#ifdef _DEBUG
void CrstBase::PreEnter()
{
    if (g_pThreadStore->IsCrstForThreadStore(this))
        return;
    
    Thread* pThread = GetThread();
    if (pThread && m_heldInSuspension && pThread->PreemptiveGCDisabled())
        _ASSERTE (!"Deallock situation 1: lock may be held during GC, but not entered in PreemptiveGC mode");
}

void CrstBase::PostEnter()
{
    if (g_pThreadStore->IsCrstForThreadStore(this))
        return;
    
    Thread* pThread = GetThread();
    if (pThread)
    {
        if (!m_heldInSuspension)
            m_ulReadyForSuspensionCount =
                pThread->GetReadyForSuspensionCount();
        if (!m_enterInCoopGCMode)
            m_enterInCoopGCMode = pThread->PreemptiveGCDisabled();
    }
}

void CrstBase::PreLeave()
{
    if (g_pThreadStore->IsCrstForThreadStore(this))
        return;
    
    Thread* pThread = GetThread();
    if (pThread)
    {
        if (!m_heldInSuspension &&
            m_ulReadyForSuspensionCount !=
            pThread->GetReadyForSuspensionCount())
        {
            m_heldInSuspension = TRUE;
        }
        if (m_heldInSuspension && m_enterInCoopGCMode)
        {
            // The GC thread calls into the handle table to scan handles.  Sometimes
            // the GC thread is a random application thread that is provoking a GC.
            // Sometimes the GC thread is a secret GC thread (server or concurrent).
            // This can happen if a DllMain notification executes managed code, as in
            // an IJW scenario.  In the case of the secret thread, we will take this
            // lock in preemptive mode.
            //
            // Normally this would be a dangerous combination.  But, in the case of
            // this particular Crst, we only ever take the critical section on a thread
            // which is identified as the GC thread and which is therefore not subject
            // to GC suspensions.
            //
            // The easiest way to handle this is to weaken the assert for this precise
            // case.  The alternative would be to have a notion of locks that are
            // only ever taken by threads identified as the GC thread.
            if (m_crstlevel != CrstHandleTable ||
                pThread != g_pGCHeap->GetGCThread())
            {
                _ASSERTE (!"Deadlock situation 2: lock may be held during GC, but were not entered in PreemptiveGC mode earlier");
            }
        }
    }
}

CrstBase* CrstBase::m_pDummyHeadCrst;
CrstBase  CrstBase::m_DummyHeadCrst;

void CrstBase::DebugInit(LPCSTR szTag, CrstLevel crstlevel, BOOL fAllowReentrancy, BOOL fAllowSameLevel)
{
    _ASSERTE((sizeof(CrstBase) == sizeof(Crst)) && "m_DummyHeadCrst will not be large enough");

    if(szTag) {
        int lgth = (int)strlen(szTag) + 1;
        lgth = lgth >  (int) (sizeof(m_tag)/sizeof(m_tag[0]) ? sizeof(m_tag)/sizeof(m_tag[0]) : lgth);
        memcpy(m_tag, szTag, lgth);

        // Null terminate the string in case it got truncated
        m_tag[lgth-1] = 0;
    }
    
    m_crstlevel        = crstlevel;
    m_holderthreadid   = 0;
    m_entercount       = 0;
    m_flags = 0;
    if(fAllowReentrancy) m_flags |= CRST_REENTRANCY;
    if(fAllowSameLevel)  m_flags |= CRST_SAMELEVEL;
    
    
    LOG((LF_SYNC, INFO3, "ConstructCrst with this:0x%x\n", this));
    if (this == &m_DummyHeadCrst) {
        // Set list to empty.
        m_next = m_prev = this;
    } else {
        // Link this crst into the global list.
        if(m_pDummyHeadCrst) {
        LOCKCOUNTINCL("DebugInit in crst.cpp");								\
            EnterCriticalSection(&(m_pDummyHeadCrst->m_criticalsection));
            m_next = m_pDummyHeadCrst;
            m_prev = m_pDummyHeadCrst->m_prev;
            m_prev->m_next = this;
            m_pDummyHeadCrst->m_prev = this;
            LeaveCriticalSection(&(m_pDummyHeadCrst->m_criticalsection));
        LOCKCOUNTDECL("DebugInit in crst.cpp");								\
        }
        _ASSERTE(this != m_next);
        _ASSERTE(this != m_prev);
    }

    m_heldInSuspension = FALSE;
    m_enterInCoopGCMode = FALSE;
}

void CrstBase::DebugDestroy()
{
    FillMemory(&m_criticalsection, sizeof(m_criticalsection), 0xcc);
    m_holderthreadid = 0xcccccccc;
    m_entercount     = 0xcccccccc;
    
    if (this == &m_DummyHeadCrst) {
        
        // m_DummyHeadCrst dies when global destructors are called.
        // It should be the last one to go.
        for (CrstBase *pcrst = m_pDummyHeadCrst->m_next;
             pcrst != m_pDummyHeadCrst;
             pcrst = pcrst->m_next) {
            // TEXT and not L"..." as the JIT uses this file and it is still ASCII
            DbgWriteEx(TEXT("WARNING: Crst \"%hs\" at 0x%lx was leaked.\n"),
                       pcrst->m_tag,
                       (size_t)pcrst);
        }
    } else {
        
        if(m_pDummyHeadCrst) {
        // Unlink from global crst list.
        LOCKCOUNTINCL("DebugDestroy in crst.cpp");								\
            EnterCriticalSection(&(m_pDummyHeadCrst->m_criticalsection));
            m_next->m_prev = m_prev;
            m_prev->m_next = m_next;
            m_next = (CrstBase*)POISONC;
            m_prev = (CrstBase*)POISONC;
            LeaveCriticalSection(&(m_pDummyHeadCrst->m_criticalsection));
        LOCKCOUNTDECL("DebugDestroy in crst.cpp");								\
        }
    }
}
    
//-----------------------------------------------------------------
// Check if attempting to take the lock would violate level order.
//-----------------------------------------------------------------
BOOL CrstBase::IsSafeToTake()
{
    // If mscoree.dll is being detached
    if (g_fProcessDetach)
        return TRUE;

    DWORD threadId = GetCurrentThreadId();

    if (m_holderthreadid == threadId)
    {
        // If we already hold it, we can't violate level order.
        // Check if client wanted to allow reentrancy.
        if ((m_flags & CRST_REENTRANCY) == 0)
        {
            LOG((LF_SYNC, INFO3, "Crst Reentrancy violation on %s\n", m_tag));
        }
        return ((m_flags & CRST_REENTRANCY) != 0);
    }


    // See if the current thread already owns a lower or sibling lock.
    BOOL fSafe = TRUE;
    if(m_pDummyHeadCrst) {
    LOCKCOUNTINCL("IsSafeToTake in crst.cpp");								\
        EnterCriticalSection(&(m_pDummyHeadCrst->m_criticalsection));
        CrstBase *pcrst = m_pDummyHeadCrst->m_next;
        while (pcrst != m_pDummyHeadCrst)
        {
            if (pcrst->m_holderthreadid == threadId && 
                (pcrst->m_crstlevel < m_crstlevel || 
                (pcrst->m_crstlevel == m_crstlevel && (m_flags & CRST_SAMELEVEL) == 0)))
            {
                fSafe = FALSE;  //no need to search any longer.
                LOG((LF_SYNC, INFO3, "Crst Level violation between %s and %s\n",
                    m_tag, pcrst->m_tag));
                break;
            }
            pcrst = pcrst->m_next;
        }
        LeaveCriticalSection(&(m_pDummyHeadCrst->m_criticalsection));
    LOCKCOUNTDECL("IsSafeToTake in crst.cpp");								\
    }
    return fSafe;
}

#endif

