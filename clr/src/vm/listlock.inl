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
// File: ListLock.inl
//
// ===========================================================================
// This file decribes the list lock and deadlock aware list lock functions
// that are inlined but can't go in the header.
// ===========================================================================
#ifndef LISTLOCK_INL
#define LISTLOCK_INL

#include "listlock.h"
#include "dbginterface.h"
// Must own the lock before calling this or is ok if the debugger has
// all threads stopped

inline LockedListElement *ListLock::Find(void *pData)
{
#ifdef DEBUGGING_SUPPORTED
    _ASSERTE(m_CriticalSection.OwnedByCurrentThread() || 
             CORDebuggerAttached() && g_pDebugInterface->IsStopped());
#else
    _ASSERTE(m_CriticalSection.OwnedByCurrentThread());
#endif // DEBUGGING_SUPPORTED

    LockedListElement *pSearch;

    for (pSearch = m_pHead; pSearch != NULL; pSearch = pSearch->m_pNext)
    {
        if (pSearch->m_pData == pData)
            return pSearch;
    }

    return NULL;
}


#endif // LISTLOCK_I
