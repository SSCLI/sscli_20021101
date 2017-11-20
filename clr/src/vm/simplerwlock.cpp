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
#include "simplerwlock.hpp"

BOOL SimpleRWLock::TryEnterRead()
{
#ifdef _DEBUG
    if (m_bRequirePreemptiveGC)
        _ASSERTE(!GetThread() || !GetThread()->PreemptiveGCDisabled());
    else
        _ASSERTE(!GetThread() || GetThread()->PreemptiveGCDisabled());
#endif
    
    BOOL fGotIt = FALSE;
    if (m_RWLock != -1)
    {
        LOCK();
        if (m_RWLock != -1)
        {
            InterlockedIncrement (&m_RWLock);
            INCTHREADLOCKCOUNT();
            fGotIt = TRUE;
        }
        UNLOCK();
    }
    return fGotIt;
}

//=====================================================================        
void SimpleRWLock::EnterRead()
{
#ifdef _DEBUG
    if (m_bRequirePreemptiveGC)
        _ASSERTE(!GetThread() || !GetThread()->PreemptiveGCDisabled());
    else
        _ASSERTE(!GetThread() || GetThread()->PreemptiveGCDisabled());
#endif

    while (!TryEnterRead());
}

//=====================================================================        
BOOL SimpleRWLock::TryEnterWrite()
{
#ifdef _DEBUG
    if (m_bRequirePreemptiveGC)
        _ASSERTE(!GetThread() || !GetThread()->PreemptiveGCDisabled());
    else
        _ASSERTE(!GetThread() || GetThread()->PreemptiveGCDisabled());
#endif

    BOOL fGotIt = FALSE;
    if (m_RWLock == 0)
    {
        LOCK();
        if (m_RWLock == 0)
        {
            m_RWLock = -1;
            INCTHREADLOCKCOUNT();
            fGotIt = TRUE;
        }
        UNLOCK();
    }
    return fGotIt;
}

//=====================================================================        
void SimpleRWLock::EnterWrite()
{
#ifdef _DEBUG
    if (m_bRequirePreemptiveGC)
        _ASSERTE(!GetThread() || !GetThread()->PreemptiveGCDisabled());
    else
        _ASSERTE(!GetThread() || GetThread()->PreemptiveGCDisabled());
#endif

    while (!TryEnterWrite());
}

