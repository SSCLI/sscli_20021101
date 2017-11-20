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
//+-------------------------------------------------------------------
//
//  File:       RWLock.cpp
//
//  Contents:   Reader writer lock implementation that supports the
//              following features
//                  1. Cheap enough to be used in large numbers
//                     such as per object synchronization.
//                  2. Supports timeout. This is a valuable feature
//                     to detect deadlocks
//                  3. Supports caching of events. This allows
//                     the events to be moved from least contentious
//                     regions to the most contentious regions.
//                     In other words, the number of events needed by
//                     Reader-Writer lockls is bounded by the number
//                     of threads in the process.
//                  4. Supports nested locks by readers and writers
//                  5. Supports spin counts for avoiding context switches
//                     on  multi processor machines.
//                  6. Supports functionality for upgrading to a writer
//                     lock with a return argument that indicates
//                     intermediate writes. Downgrading from a writer
//                     lock restores the state of the lock.
//                  7. Supports functionality to Release Lock for calling
//                     app code. RestoreLock restores the lock state and
//                     indicates intermediate writes.
//                  8. Recovers from most common failures such as creation of
//                     events. In other words, the lock mainitains consistent
//                     internal state and remains usable
//
//
//  Classes:    CRWLock
//--------------------------------------------------------------------
#include "common.h"
#include "rwlock.h"


// Reader increment
#define READER                 0x00000001
// Max number of readers
#define READERS_MASK           0x000003FF
// Reader being signaled
#define READER_SIGNALED        0x00000400
// Writer being signaled
#define WRITER_SIGNALED        0x00000800
#define WRITER                 0x00001000
// Waiting reader increment
#define WAITING_READER         0x00002000
// Note size of waiting readers must be less
// than or equal to size of readers
#define WAITING_READERS_MASK   0x007FE000
#define WAITING_READERS_SHIFT  13
// Waiting writer increment
#define WAITING_WRITER         0x00800000
// Max number of waiting writers
#define WAITING_WRITERS_MASK   0xFF800000
// Events are being cached
#define CACHING_EVENTS         (READER_SIGNALED | WRITER_SIGNALED)

// Cookie flags
#define UPGRADE_COOKIE         0x02000
#define RELEASE_COOKIE         0x04000
#define COOKIE_NONE            0x10000
#define COOKIE_WRITER          0x20000
#define COOKIE_READER          0x40000
#define INVALID_COOKIE         (~(UPGRADE_COOKIE | RELEASE_COOKIE |            \
                                  COOKIE_NONE | COOKIE_WRITER | COOKIE_READER))

// globals
volatile LONG CRWLock::s_mostRecentLLockID = 0;
volatile LONG CRWLock::s_mostRecentULockID = -1;
#ifdef _TESTINGRWLOCK
CRITICAL_SECTION CRWLock::s_RWLockCrst;
SYSTEM_INFO g_SystemInfo;
HANDLE g_hProcessHeap;
#else // !_TESTINGRWLOCK
CrstStatic CRWLock::s_RWLockCrst;
#endif // _TESTINGRWLOCK

// Default values
#ifdef _DEBUG
DWORD gdwDefaultTimeout = 120000;
#else //!_DEBUG
DWORD gdwDefaultTimeout = INFINITE;
#endif //_DEBUG
const DWORD gdwReasonableTimeout = 120000;
DWORD gdwDefaultSpinCount = 0;
BOOL fBreakOnErrors = FALSE; // Temporarily break on errors

#define HEAP_SERIALIZE                   0
#define RWLOCK_RECOVERY_FAILURE          (0xC0000227L)

// Helpers class that tracks the lifetime of a frame
#ifdef _TESTINGRWLOCK

#define COR_E_THREADINTERRUPTED          WAIT_IO_COMPLETION
#define VALIDATE(pRWLock)
#define COMPlusThrowWin32(dwStatus, args) RaiseException(dwStatus, EXCEPTION_NONCONTINUABLE, 0, NULL)

#define FastInterlockExchangeAdd InterlockedExchangeAdd

inline LONG FastInterlockCompareExchange(LONG* pvDestination, LONG dwExchange, LONG dwComperand)
{
    return(InterlockedCompareExchange(pvDestination, dwExchange, dwComperand));
}

#else //!_TESTINGRWLOCK

// Catch GC holes
#if _DEBUG
#define VALIDATE(pRWLock)                ((Object *) (pRWLock))->Validate();
#else // !_DEBUG
#define VALIDATE(pRWLock)
#endif // _DEBUG

#endif //_TESTINGRWLOCK                                               

//+-------------------------------------------------------------------
//
//  Method:     CRWLock::ProcessInit     public
//
//  Synopsis:   Reads default values from registry and intializes 
//              process wide data structures
//+-------------------------------------------------------------------
void CRWLock::ProcessInit()
{
#ifdef _TESTINGRWLOCK
    // Obtain process heap
    g_hProcessHeap = GetProcessHeap();

    // Obtain number of processors on the system
    GetSystemInfo(&g_SystemInfo);
#endif //_TESTINGRWLOCK

    _ASSERTE(g_SystemInfo.dwNumberOfProcessors != 0);
    gdwDefaultSpinCount = (g_SystemInfo.dwNumberOfProcessors != 1) ? 500 : 0;


    // Initialize the critical section used by the lock
#ifdef _TESTINGRWLOCK
    InitializeCriticalSection(&s_RWLockCrst);
#else
    s_RWLockCrst.Init("RWLock", CrstDummy);
#endif
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::ProcessCleanup     public
//
//  Synopsis:   Cleansup process wide data structures
//+-------------------------------------------------------------------

//+-------------------------------------------------------------------
//
//  Method:     CRWLock::CRWLock     public
//
//  Synopsis:   Constructor
//+-------------------------------------------------------------------
CRWLock::CRWLock()
:   _hWriterEvent(NULL),
    _hReaderEvent(NULL),
    _dwState(0),
    _dwWriterID(0),
    _dwWriterSeqNum(1),
    _wFlags(0),
    _wWriterLevel(0)
#ifdef RWLOCK_STATISTICS
    ,
    _dwReaderEntryCount(0),
    _dwReaderContentionCount(0),
    _dwWriterEntryCount(0),
    _dwWriterContentionCount(0),
    _dwEventsReleasedCount(0)
#endif
{
    LONG dwKnownLLockID;
    LONG dwULockID = s_mostRecentULockID;
    LONG dwLLockID = s_mostRecentLLockID;
    do
    {
        dwKnownLLockID = dwLLockID;
        if(dwKnownLLockID != 0)
        {
            dwLLockID = RWInterlockedCompareExchange(&s_mostRecentLLockID, dwKnownLLockID+1, dwKnownLLockID);
        }
        else
        {
#ifdef _TESTINGRWLOCK
            LOCKCOUNTINCL("CRWLock in rwlock.cpp");
            EnterCriticalSection(&s_RWLockCrst);
#else
            s_RWLockCrst.Enter();
#endif
            
            if(s_mostRecentLLockID == 0)
            {
                dwULockID = ++s_mostRecentULockID;
                dwLLockID = s_mostRecentLLockID++;
                dwKnownLLockID = dwLLockID;
            }
            else
            {
                dwULockID = s_mostRecentULockID;
                dwLLockID = s_mostRecentLLockID;
            }

#ifdef _TESTINGRWLOCK
            LeaveCriticalSection(&s_RWLockCrst);
            LOCKCOUNTDECL("CRWLock in rwlock.cpp");
#else
            s_RWLockCrst.Leave();
#endif
        }
    } while(dwKnownLLockID != dwLLockID);

    _dwLLockID = ++dwLLockID;
    _dwULockID = dwULockID;

    _ASSERTE(_dwLLockID > 0);
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::Cleanup    public
//
//  Synopsis:   Cleansup state
//+-------------------------------------------------------------------
void CRWLock::Cleanup()
{
    // Sanity checks
    _ASSERTE(_dwState == 0);
    _ASSERTE(_dwWriterID == 0);
    _ASSERTE(_wWriterLevel == 0);

    if(_hWriterEvent)
        CloseHandle(_hWriterEvent);
    if(_hReaderEvent)
        CloseHandle(_hReaderEvent);

    return;
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::ChainEntry     private
//
//  Synopsis:   Chains the given lock entry into the chain
//+-------------------------------------------------------------------
inline void CRWLock::ChainEntry(Thread *pThread, LockEntry *pLockEntry)
{
    LockEntry *pHeadEntry = pThread->m_pHead;
    pLockEntry->pNext = pHeadEntry;
    pLockEntry->pPrev = pHeadEntry->pPrev;
    pLockEntry->pPrev->pNext = pLockEntry;
    pHeadEntry->pPrev = pLockEntry;

    return;
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::GetLockEntry     private
//
//  Synopsis:   Gets lock entry from TLS
//+-------------------------------------------------------------------
inline LockEntry *CRWLock::GetLockEntry()
{
    LockEntry *pHeadEntry = GetThread()->m_pHead;
    LockEntry *pLockEntry = pHeadEntry;
    do
    {
        if((pLockEntry->dwLLockID == _dwLLockID) && (pLockEntry->dwULockID == _dwULockID))
            return(pLockEntry);
        pLockEntry = pLockEntry->pNext;
    } while(pLockEntry != pHeadEntry);

    return(NULL);
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::FastGetOrCreateLockEntry     private
//
//  Synopsis:   The fast path for getting a lock entry from TLS
//+-------------------------------------------------------------------
inline LockEntry *CRWLock::FastGetOrCreateLockEntry()
{
    Thread *pThread = GetThread();
    _ASSERTE(pThread);
    LockEntry *pLockEntry = pThread->m_pHead;
    if(pLockEntry->dwLLockID == 0)
    {
        _ASSERTE(pLockEntry->wReaderLevel == 0);
        pLockEntry->dwLLockID = _dwLLockID;
        pLockEntry->dwULockID = _dwULockID;
        return(pLockEntry);
    }
    else if((pLockEntry->dwLLockID == _dwLLockID) && (pLockEntry->dwULockID == _dwULockID))
    {
        _ASSERTE(pLockEntry->wReaderLevel);
        return(pLockEntry);
    }

    return(SlowGetOrCreateLockEntry(pThread));
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::SlowGetorCreateLockEntry     private
//
//  Synopsis:   The slow path for getting a lock entry from TLS
//+-------------------------------------------------------------------
LockEntry *CRWLock::SlowGetOrCreateLockEntry(Thread *pThread)
{
    LockEntry *pFreeEntry = NULL;
    LockEntry *pHeadEntry = pThread->m_pHead;

    // Search for an empty entry or an entry belonging to this lock
    LockEntry *pLockEntry = pHeadEntry->pNext;
    while(pLockEntry != pHeadEntry)
    {
         if(pLockEntry->dwLLockID && 
            ((pLockEntry->dwLLockID != _dwLLockID) || (pLockEntry->dwULockID != _dwULockID)))
         {
             // Move to the next entry
             pLockEntry = pLockEntry->pNext;
         }
         else
         {
             // Prepare to move it to the head
             pFreeEntry = pLockEntry;
             pLockEntry->pPrev->pNext = pLockEntry->pNext;
             pLockEntry->pNext->pPrev = pLockEntry->pPrev;

             break;
         }
    }

    if(pFreeEntry == NULL)
    {
        pFreeEntry = (LockEntry *) HeapAlloc(g_hProcessHeap, HEAP_SERIALIZE, sizeof(LockEntry));
        if (pFreeEntry == NULL) FailFast(GetThread(), FatalOutOfMemory);
        pFreeEntry->wReaderLevel = 0;
    }

    if(pFreeEntry)
    {
        _ASSERTE((pFreeEntry->dwLLockID != 0) || (pFreeEntry->wReaderLevel == 0));
        _ASSERTE((pFreeEntry->wReaderLevel == 0) || 
                 ((pFreeEntry->dwLLockID == _dwLLockID) && (pFreeEntry->dwULockID == _dwULockID)));

        // Chain back the entry
        ChainEntry(pThread, pFreeEntry);

        // Move this entry to the head
        pThread->m_pHead = pFreeEntry;

        // Mark the entry as belonging to this lock
        pFreeEntry->dwLLockID = _dwLLockID;
        pFreeEntry->dwULockID = _dwULockID;
    }

    return pFreeEntry;
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::FastRecycleLockEntry     private
//
//  Synopsis:   Fast path for recycling the lock entry that is used
//              when the thread is the next few instructions is going
//              to call FastGetOrCreateLockEntry again
//+-------------------------------------------------------------------
inline void CRWLock::FastRecycleLockEntry(LockEntry *pLockEntry)
{
    // Sanity checks
    _ASSERTE(pLockEntry->wReaderLevel == 0);
    _ASSERTE((pLockEntry->dwLLockID == _dwLLockID) && (pLockEntry->dwULockID == _dwULockID));
    _ASSERTE(pLockEntry == GetThread()->m_pHead);

    pLockEntry->dwLLockID = 0;
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::RecycleLockEntry     private
//
//  Synopsis:   Fast path for recycling the lock entry
//+-------------------------------------------------------------------
inline void CRWLock::RecycleLockEntry(LockEntry *pLockEntry)
{
    // Sanity check
    _ASSERTE(pLockEntry->wReaderLevel == 0);

    // Move the entry to tail
    Thread *pThread = GetThread();
    LockEntry *pHeadEntry = pThread->m_pHead;
    if(pLockEntry == pHeadEntry)
    {
        pThread->m_pHead = pHeadEntry->pNext;
    }
    else if(pLockEntry->pNext->dwLLockID)
    {
        // Prepare to move the entry to tail
        pLockEntry->pPrev->pNext = pLockEntry->pNext;
        pLockEntry->pNext->pPrev = pLockEntry->pPrev;

        // Chain back the entry
        ChainEntry(pThread, pLockEntry);
    }

    // The entry does not belong to this lock anymore
    pLockEntry->dwLLockID = 0;
    return;
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::StaticIsWriterLockHeld    public
//
//  Synopsis:   Return TRUE if writer lock is held
//+-------------------------------------------------------------------
FCIMPL1(BOOL, CRWLock::StaticIsWriterLockHeld, CRWLock *pRWLock)
{
    THROWSCOMPLUSEXCEPTION();

    if (pRWLock == NULL)
    {
        FCThrow(kNullReferenceException);
    }

    if(pRWLock->_dwWriterID == GetCurrentThreadId())
        return(TRUE);

    return(FALSE);
}
FCIMPLEND


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::StaticIsReaderLockHeld    public
//
//  Synopsis:   Return TRUE if reader lock is held
//+-------------------------------------------------------------------
FCIMPL1(BOOL, CRWLock::StaticIsReaderLockHeld, CRWLock *pRWLock)
{
    THROWSCOMPLUSEXCEPTION();

    if (pRWLock == NULL)
    {
        FCThrow(kNullReferenceException);
    }
    
    LockEntry *pLockEntry = pRWLock->GetLockEntry();
    if(pLockEntry)
    {
        _ASSERTE(pLockEntry->wReaderLevel);
        return(TRUE);
    }

    return(FALSE);
}
FCIMPLEND


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::AssertWriterLockHeld    public
//
//  Synopsis:   Asserts that writer lock is held
//+-------------------------------------------------------------------
#ifdef _DEBUG
BOOL CRWLock::AssertWriterLockHeld()
{
    if(_dwWriterID == GetCurrentThreadId())
        return(TRUE);

    _ASSERTE(!"Writer lock not held by the current thread");
    return(FALSE);
}
#endif


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::AssertWriterLockNotHeld    public
//
//  Synopsis:   Asserts that writer lock is not held
//+-------------------------------------------------------------------
#ifdef _DEBUG
BOOL CRWLock::AssertWriterLockNotHeld()
{
    if(_dwWriterID != GetCurrentThreadId())
        return(TRUE);

    _ASSERTE(!"Writer lock held by the current thread");
    return(FALSE);
}
#endif


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::AssertReaderLockHeld    public
//
//  Synopsis:   Asserts that reader lock is held
//+-------------------------------------------------------------------
#ifdef _DEBUG
BOOL CRWLock::AssertReaderLockHeld()
{
    LockEntry *pLockEntry = GetLockEntry();
    if(pLockEntry)
    {
        _ASSERTE(pLockEntry->wReaderLevel);
        return(TRUE);
    }

    _ASSERTE(!"Reader lock not held by the current thread");
    return(FALSE);
}
#endif


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::AssertReaderLockNotHeld    public
//
//  Synopsis:   Asserts that writer lock is not held
//+-------------------------------------------------------------------
#ifdef _DEBUG
BOOL CRWLock::AssertReaderLockNotHeld()
{
    LockEntry *pLockEntry = GetLockEntry();
    if(pLockEntry == NULL)
        return(TRUE);

    _ASSERTE(pLockEntry->wReaderLevel);
    _ASSERTE(!"Reader lock held by the current thread");

    return(FALSE);
}
#endif


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::AssertReaderOrWriterLockHeld   public
//
//  Synopsis:   Asserts that writer lock is not held
//+-------------------------------------------------------------------
#ifdef _DEBUG
BOOL CRWLock::AssertReaderOrWriterLockHeld()
{
    if(_dwWriterID == GetCurrentThreadId())
    {
        return(TRUE);
    }
    else
    {
        LockEntry *pLockEntry = GetLockEntry();
        if(pLockEntry)
        {
            _ASSERTE(pLockEntry->wReaderLevel);
            return(TRUE);
        }
    }

    _ASSERTE(!"Neither Reader nor Writer lock held");
    return(FALSE);
}
#endif


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::RWSetEvent    private
//
//  Synopsis:   Helper function for setting an event
//+-------------------------------------------------------------------
inline void CRWLock::RWSetEvent(HANDLE event)
{
    THROWSCOMPLUSEXCEPTION();
    if(!SetEvent(event))
    {
        _ASSERTE(!"SetEvent failed");
        if(fBreakOnErrors)
            DebugBreak();
        COMPlusThrowWin32(E_UNEXPECTED, NULL);        
    }
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::RWResetEvent    private
//
//  Synopsis:   Helper function for resetting an event
//+-------------------------------------------------------------------
inline void CRWLock::RWResetEvent(HANDLE event)
{
    THROWSCOMPLUSEXCEPTION();
    if(!ResetEvent(event))
    {
        _ASSERTE(!"ResetEvent failed");
        if(fBreakOnErrors)
            DebugBreak();
        COMPlusThrowWin32(E_UNEXPECTED, NULL);        
    }
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::RWWaitForSingleObject    public
//
//  Synopsis:   Helper function for waiting on an event
//+-------------------------------------------------------------------
inline DWORD CRWLock::RWWaitForSingleObject(HANDLE event, DWORD dwTimeout)
{
#ifdef _TESTINGRWLOCK
    return(WaitForSingleObjectEx(event, dwTimeout, TRUE));
#else
    return(GetThread()->DoAppropriateWaitWorker(1, &event, TRUE, dwTimeout, TRUE));
#endif
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::RWSleep    public
//
//  Synopsis:   Helper function for calling Sleep
//+-------------------------------------------------------------------
inline void CRWLock::RWSleep(DWORD dwTime)
{
    SleepEx(dwTime, TRUE);
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::RWInterlockedCompareExchange    public
//
//  Synopsis:   Helper function for calling intelockedCompareExchange
//+-------------------------------------------------------------------
inline LONG CRWLock::RWInterlockedCompareExchange(LONG volatile *pvDestination,
                                                   LONG dwExchange,
                                                   LONG dwComparand)
{
    return  FastInterlockCompareExchange(pvDestination, 
                                         dwExchange, 
                                         dwComparand);
}

inline void* CRWLock::RWInterlockedCompareExchangePointer(PVOID volatile *pvDestination,
                                                   void* pExchange,
                                                   void* pComparand)
{
    return  FastInterlockCompareExchangePointer(pvDestination, 
                                            pExchange, 
                                            pComparand);
}

//+-------------------------------------------------------------------
//
//  Method:     CRWLock::RWInterlockedExchangeAdd    public
//
//  Synopsis:   Helper function for adding state
//+-------------------------------------------------------------------
inline LONG CRWLock::RWInterlockedExchangeAdd(LONG volatile *pvDestination,
                                               LONG dwAddToState)
{
    return FastInterlockExchangeAdd(pvDestination, dwAddToState);
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::RWInterlockedIncrement    public
//
//  Synopsis:   Helper function for incrementing a pointer
//+-------------------------------------------------------------------
inline LONG CRWLock::RWInterlockedIncrement(LONG volatile *pdwState)
{
    return FastInterlockIncrement(pdwState);
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::ReleaseEvents    public
//
//  Synopsis:   Helper function for caching events
//+-------------------------------------------------------------------
void CRWLock::ReleaseEvents()
{
    // Ensure that reader and writers have been stalled
    _ASSERTE((_dwState & CACHING_EVENTS) == CACHING_EVENTS);

    // Save writer event
    HANDLE hWriterEvent = _hWriterEvent;
    _hWriterEvent = NULL;

    // Save reader event
    HANDLE hReaderEvent = _hReaderEvent;
    _hReaderEvent = NULL;

    // Allow readers and writers to continue
    RWInterlockedExchangeAdd(&_dwState, -(CACHING_EVENTS));

    // Cache events
    if(hWriterEvent)
    {
        LOG((LF_SYNC, LL_INFO10, "Releasing writer event\n"));
        CloseHandle(hWriterEvent);
    }
    if(hReaderEvent)
    {
        LOG((LF_SYNC, LL_INFO10, "Releasing reader event\n"));
        CloseHandle(hReaderEvent);
    }
#ifdef RWLOCK_STATISTICS
    RWInterlockedIncrement(&_dwEventsReleasedCount);
#endif

    return;
}

//+-------------------------------------------------------------------
//
//  Method:     CRWLock::GetWriterEvent    public
//
//  Synopsis:   Helper function for obtaining a auto reset event used
//              for serializing writers. It utilizes event cache
//+-------------------------------------------------------------------
HANDLE CRWLock::GetWriterEvent()
{
    if(_hWriterEvent == NULL)
    {
        HANDLE hWriterEvent = ::WszCreateEvent(NULL, FALSE, FALSE, NULL);
        if(hWriterEvent)
        {
            if(RWInterlockedCompareExchangePointer((PVOID volatile *) &_hWriterEvent,
                                            hWriterEvent,        
                                            NULL))
            {
                CloseHandle(hWriterEvent);
            }
        }
    }

    return(_hWriterEvent);
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::GetReaderEvent    public
//
//  Synopsis:   Helper function for obtaining a manula reset event used
//              by readers to wait when a writer holds the lock.
//              It utilizes event cache
//+-------------------------------------------------------------------
HANDLE CRWLock::GetReaderEvent()
{
    if(_hReaderEvent == NULL)
    {
        HANDLE hReaderEvent = ::WszCreateEvent(NULL, TRUE, FALSE, NULL);
        if(hReaderEvent)
        {
            if(RWInterlockedCompareExchangePointer((PVOID volatile *) &_hReaderEvent,
                                            hReaderEvent,
                                            NULL))
            {
                CloseHandle(hReaderEvent);
            }
        }
    }

    return(_hReaderEvent);
}

//+-------------------------------------------------------------------
//
//  Method:     CRWLock::StaticRecoverLock    public
//
//  Synopsis:   Helper function to restore the lock to 
//              the original state
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
void F_CALL_CONV CRWLock::StaticRecoverLock(
    CRWLock **ppRWLock, 
    LockCookie *pLockCookie,
    DWORD dwFlags)
{
    THROWSCOMPLUSEXCEPTION();

    DWORD dwTimeout = (gdwDefaultTimeout > gdwReasonableTimeout)
                        ? gdwDefaultTimeout
                        : gdwReasonableTimeout;

    Thread *pThread = GetThread();
    _ASSERTE (pThread);

    COMPLUS_TRYEX(pThread)
    {
        // Check if the thread was a writer
        if(dwFlags & COOKIE_WRITER)
        {
            // Acquire writer lock
            StaticAcquireWriterLock(ppRWLock, dwTimeout);
            pThread->m_dwLockCount -= (*ppRWLock)->_wWriterLevel;
            _ASSERTE (pThread->m_dwLockCount >= 0);
            (*ppRWLock)->_wWriterLevel = pLockCookie->wWriterLevel;
            pThread->m_dwLockCount += (*ppRWLock)->_wWriterLevel;
        }
        // Check if the thread was a reader
        else if(dwFlags & COOKIE_READER)
        {
            StaticAcquireReaderLock(ppRWLock, dwTimeout);
            LockEntry *pLockEntry = (*ppRWLock)->GetLockEntry();
            _ASSERTE(pLockEntry);
            pThread->m_dwLockCount -= pLockEntry->wReaderLevel;
            _ASSERTE (pThread->m_dwLockCount >= 0);
            pLockEntry->wReaderLevel = pLockCookie->wReaderLevel;
            pThread->m_dwLockCount += pLockEntry->wReaderLevel;
        }
    }
    COMPLUS_CATCH
    {
        _ASSERTE(!"Failed to restore to a reader");
        COMPlusThrowWin32(RWLOCK_RECOVERY_FAILURE, NULL);
    }
    COMPLUS_END_CATCH
}

//+-------------------------------------------------------------------
//
//  Method:     CRWLock::StaticAcquireReaderLockPublic    public
//
//  Synopsis:   Public access to StaticAcquireReaderLock
//
//+-------------------------------------------------------------------
FCIMPL2(void, CRWLock::StaticAcquireReaderLockPublic, CRWLock *pRWLockUNSAFE, DWORD dwDesiredTimeout)
{
    THROWSCOMPLUSEXCEPTION();

    if (pRWLockUNSAFE == NULL)
    {
        FCThrowVoid(kNullReferenceException);
    }

    OBJECTREF pRWLock = ObjectToOBJECTREF((Object*)pRWLockUNSAFE);
    HELPER_METHOD_FRAME_BEGIN_1(pRWLock);

    StaticAcquireReaderLock((CRWLock**)&pRWLock, dwDesiredTimeout);

    HELPER_METHOD_FRAME_END();
}
FCIMPLEND


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::StaticAcquireReaderLock    private
//
//  Synopsis:   Makes the thread a reader. Supports nested reader locks.
//+-------------------------------------------------------------------

void F_CALL_CONV CRWLock::StaticAcquireReaderLock(
    CRWLock **ppRWLock, 
    DWORD dwDesiredTimeout)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(ppRWLock);
	_ASSERTE(*ppRWLock);

    LockEntry *pLockEntry = (*ppRWLock)->FastGetOrCreateLockEntry();
    if (pLockEntry == NULL)
    {
        COMPlusThrowWin32(STATUS_NO_MEMORY, NULL);        
    }
    
    DWORD dwStatus = WAIT_OBJECT_0;
    // Check for the fast path
    if(RWInterlockedCompareExchange(&(*ppRWLock)->_dwState, READER, 0) == 0)
    {
        _ASSERTE(pLockEntry->wReaderLevel == 0);
    }
    // Check for nested reader
    else if(pLockEntry->wReaderLevel != 0)
    {
        _ASSERTE((*ppRWLock)->_dwState & READERS_MASK);
        ++pLockEntry->wReaderLevel;
        INCTHREADLOCKCOUNT();
        return;
    }
    // Check if the thread already has writer lock
    else if((*ppRWLock)->_dwWriterID == GetCurrentThreadId())
    {
        StaticAcquireWriterLock(ppRWLock, dwDesiredTimeout);
        (*ppRWLock)->FastRecycleLockEntry(pLockEntry);
        return;
    }
    else
    {
        DWORD dwSpinCount;
        DWORD dwCurrentState, dwKnownState;
        
        // Initialize
        dwSpinCount = 0;
        dwCurrentState = (*ppRWLock)->_dwState;
        do
        {
            dwKnownState = dwCurrentState;

            // Reader need not wait if there are only readers and no writer
            if((dwKnownState < READERS_MASK) ||
                (((dwKnownState & READER_SIGNALED) && ((dwKnownState & WRITER) == 0)) &&
                 (((dwKnownState & READERS_MASK) +
                   ((dwKnownState & WAITING_READERS_MASK) >> WAITING_READERS_SHIFT)) <=
                  (READERS_MASK - 2))))
            {
                // Add to readers
                dwCurrentState = RWInterlockedCompareExchange(&(*ppRWLock)->_dwState,
                                                              (dwKnownState + READER),
                                                              dwKnownState);
                if(dwCurrentState == dwKnownState)
                {
                    // One more reader
                    break;
                }
            }
            // Check for too many Readers or waiting readers or signaling in progress
            else if(((dwKnownState & READERS_MASK) == READERS_MASK) ||
                    ((dwKnownState & WAITING_READERS_MASK) == WAITING_READERS_MASK) ||
                    ((dwKnownState & CACHING_EVENTS) == READER_SIGNALED))
            {
                //  Sleep
                GetThread()->UserSleep(1000);
                
                // Update to latest state
                dwSpinCount = 0;
                dwCurrentState = (*ppRWLock)->_dwState;
            }
            // Check if events are being cached
            else if((dwKnownState & CACHING_EVENTS) == CACHING_EVENTS)
            {
                if(++dwSpinCount > gdwDefaultSpinCount)
                {
                    RWSleep(1);
                    dwSpinCount = 0;
                }
                dwCurrentState = (*ppRWLock)->_dwState;
            }
            // Check spin count
            else if(++dwSpinCount <= gdwDefaultSpinCount)
            {
                dwCurrentState = (*ppRWLock)->_dwState;
            }
            else
            {
                // Add to waiting readers
                dwCurrentState = RWInterlockedCompareExchange(&(*ppRWLock)->_dwState,
                                                              (dwKnownState + WAITING_READER),
                                                              dwKnownState);
                if(dwCurrentState == dwKnownState)
                {
                    HANDLE hReaderEvent;
                    DWORD dwModifyState;

                    // One more waiting reader
#ifdef RWLOCK_STATISTICS
                    RWInterlockedIncrement(&(*ppRWLock)->_dwReaderContentionCount);
#endif
                    
                    hReaderEvent = (*ppRWLock)->GetReaderEvent();
                    if(hReaderEvent)
                    {
                        dwStatus = RWWaitForSingleObject(hReaderEvent, dwDesiredTimeout);
                        VALIDATE(*ppRWLock);
                    }
                    else
                    {
                        LOG((LF_SYNC, LL_WARNING,
                            "AcquireReaderLock failed to create reader "
                            "event for RWLock 0x%x\n", *ppRWLock));
                        dwStatus = GetLastError();
                    }

                    if(dwStatus == WAIT_OBJECT_0)
                    {
                        _ASSERTE((*ppRWLock)->_dwState & READER_SIGNALED);
                        _ASSERTE(((*ppRWLock)->_dwState & READERS_MASK) < READERS_MASK);
                        dwModifyState = READER - WAITING_READER;
                    }
                    else
                    {
                        dwModifyState = (DWORD) -WAITING_READER;
                        if(dwStatus == WAIT_TIMEOUT)
                        {
                            LOG((LF_SYNC, LL_WARNING,
                                "Timed out trying to acquire reader lock "
                                "for RWLock 0x%x\n", *ppRWLock));
                            dwStatus = ERROR_TIMEOUT;
                        }
                        else if(dwStatus == WAIT_IO_COMPLETION)
                        {
                            LOG((LF_SYNC, LL_WARNING,
                                "Thread interrupted while trying to acquire reader lock "
                                "for RWLock 0x%x\n", *ppRWLock));
                            dwStatus = COR_E_THREADINTERRUPTED;
                        }
                        else
                        {
                            dwStatus = GetLastError();
                            LOG((LF_SYNC, LL_WARNING,
                                "WaitForSingleObject on Event 0x%x failed for "
                                "RWLock 0x%x with status code 0x%x\n",
                                hReaderEvent, *ppRWLock, dwStatus));
                        }
                    }

                    // One less waiting reader and he may have become a reader
                    dwKnownState = RWInterlockedExchangeAdd(&(*ppRWLock)->_dwState, dwModifyState);

                    // Check for last signaled waiting reader
                    if(dwStatus == WAIT_OBJECT_0)
                    {
                        _ASSERTE(dwKnownState & READER_SIGNALED);
                        _ASSERTE((dwKnownState & READERS_MASK) < READERS_MASK);
                        if((dwKnownState & WAITING_READERS_MASK) == WAITING_READER)
                        {
                            // Reset the event and lower reader signaled flag
                            RWResetEvent(hReaderEvent);
                            RWInterlockedExchangeAdd(&(*ppRWLock)->_dwState, -READER_SIGNALED);
                        }
                    }
                    else
                    {
                        if(((dwKnownState & WAITING_READERS_MASK) == WAITING_READER) &&
                           (dwKnownState & READER_SIGNALED))
                        {
                            if(hReaderEvent == NULL)
                                hReaderEvent = (*ppRWLock)->GetReaderEvent();
                            _ASSERTE(hReaderEvent);

                            // Ensure the event is signalled before resetting it.
#ifdef _DEBUG
                            DWORD dwTemp =
#endif
                            WaitForSingleObject(hReaderEvent, INFINITE);
                            _ASSERTE(dwTemp == WAIT_OBJECT_0);
                            _ASSERTE(((*ppRWLock)->_dwState & READERS_MASK) < READERS_MASK);
                            
                            // Reset the event and lower reader signaled flag
                            RWResetEvent(hReaderEvent);
                            RWInterlockedExchangeAdd(&(*ppRWLock)->_dwState, (READER - READER_SIGNALED));

                            // Honor the orginal status
                            pLockEntry->wReaderLevel = 1;
                            INCTHREADLOCKCOUNT();
                            StaticReleaseReaderLock(ppRWLock);
                        }
                        else
                        {
                            (*ppRWLock)->FastRecycleLockEntry(pLockEntry);
                        }
                        
                        _ASSERTE((pLockEntry == NULL) ||
                                 ((pLockEntry->dwLLockID == 0) &&
                                  (pLockEntry->wReaderLevel == 0)));
                        if(fBreakOnErrors)
                        {
                            _ASSERTE(!"Failed to acquire reader lock");
                            DebugBreak();
                        }
                        
                        // Prepare the frame for throwing an exception
                        COMPlusThrowWin32(dwStatus, NULL);
                    }

                    // Sanity check
                    _ASSERTE(dwStatus == WAIT_OBJECT_0);
                    break;                        
                }
            }
            pause();        // Indicate to the processor that we are spining
        } while(TRUE);
    }

    // Success
    _ASSERTE(dwStatus == WAIT_OBJECT_0);
    _ASSERTE(((*ppRWLock)->_dwState & WRITER) == 0);
    _ASSERTE((*ppRWLock)->_dwState & READERS_MASK);
    _ASSERTE(pLockEntry->wReaderLevel == 0);
    pLockEntry->wReaderLevel = 1;
    INCTHREADLOCKCOUNT();
#ifdef RWLOCK_STATISTICS
    RWInterlockedIncrement(&(*ppRWLock)->_dwReaderEntryCount);
#endif
    return;
}
// FCIMPLEND


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::StaticAcquireWriterLockPublic    public
//
//  Synopsis:   Public access to StaticAcquireWriterLock
//
//+-------------------------------------------------------------------
FCIMPL2(void, CRWLock::StaticAcquireWriterLockPublic, CRWLock *pRWLockUNSAFE, DWORD dwDesiredTimeout)
{
    THROWSCOMPLUSEXCEPTION();

    if (pRWLockUNSAFE == NULL)
    {
        FCThrowVoid(kNullReferenceException);
    }

    OBJECTREF pRWLock = ObjectToOBJECTREF((Object*)pRWLockUNSAFE);
    HELPER_METHOD_FRAME_BEGIN_1(pRWLock);

    StaticAcquireWriterLock((CRWLock**)&pRWLock, dwDesiredTimeout);

    HELPER_METHOD_FRAME_END();
}
FCIMPLEND


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::StaticAcquireWriterLock    private
//
//  Synopsis:   Makes the thread a writer. Supports nested writer
//              locks
//+-------------------------------------------------------------------

void F_CALL_CONV CRWLock::StaticAcquireWriterLock(
    CRWLock **ppRWLock, 
    DWORD dwDesiredTimeout)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(ppRWLock);
    _ASSERTE(*ppRWLock);

    // Declare locals needed for setting up frame
    DWORD dwThreadID = GetCurrentThreadId();
    DWORD dwStatus;

    // Check for the fast path
    if(RWInterlockedCompareExchange(&(*ppRWLock)->_dwState, WRITER, 0) == 0)
    {
        _ASSERTE(((*ppRWLock)->_dwState & READERS_MASK) == 0);
    }
    // Check if the thread already has writer lock
    else if((*ppRWLock)->_dwWriterID == dwThreadID)
    {
        ++(*ppRWLock)->_wWriterLevel;
        INCTHREADLOCKCOUNT();
        return;
    }
    else
    {
        DWORD dwCurrentState, dwKnownState;
        DWORD dwSpinCount;

        // Initialize
        dwSpinCount = 0;
        dwCurrentState = (*ppRWLock)->_dwState;
        do
        {
            dwKnownState = dwCurrentState;

            // Writer need not wait if there are no readers and writer
            if((dwKnownState == 0) || (dwKnownState == CACHING_EVENTS))
            {
                // Can be a writer
                dwCurrentState = RWInterlockedCompareExchange(&(*ppRWLock)->_dwState,
                                                              (dwKnownState + WRITER),
                                                              dwKnownState);
                if(dwCurrentState == dwKnownState)
                {
                    // Only writer
                    break;
                }
            }
            // Check for too many waiting writers
            else if(((dwKnownState & WAITING_WRITERS_MASK) == WAITING_WRITERS_MASK))
            {
                // Sleep
                GetThread()->UserSleep(1000);
                
                // Update to latest state
                dwSpinCount = 0;
                dwCurrentState = (*ppRWLock)->_dwState;
            }
            // Check if events are being cached
            else if((dwKnownState & CACHING_EVENTS) == CACHING_EVENTS)
            {
                if(++dwSpinCount > gdwDefaultSpinCount)
                {
                    RWSleep(1);
                    dwSpinCount = 0;
                }
                dwCurrentState = (*ppRWLock)->_dwState;
            }
            // Check spin count
            else if(++dwSpinCount <= gdwDefaultSpinCount)
            {
                dwCurrentState = (*ppRWLock)->_dwState;
            }
            else
            {
                // Add to waiting writers
                dwCurrentState = RWInterlockedCompareExchange(&(*ppRWLock)->_dwState,
                                                              (dwKnownState + WAITING_WRITER),
                                                              dwKnownState);
                if(dwCurrentState == dwKnownState)
                {
                    HANDLE hWriterEvent;
                    DWORD dwModifyState;

                    // One more waiting writer
#ifdef RWLOCK_STATISTICS
                    RWInterlockedIncrement(&(*ppRWLock)->_dwWriterContentionCount);
#endif
                    hWriterEvent = (*ppRWLock)->GetWriterEvent();
                    if(hWriterEvent)
                    {
                        dwStatus = RWWaitForSingleObject(hWriterEvent, dwDesiredTimeout);
                        VALIDATE(*ppRWLock);
                    }
                    else
                    {
                        LOG((LF_SYNC, LL_WARNING,
                            "AcquireWriterLock failed to create writer "
                            "event for RWLock 0x%x\n", *ppRWLock));
                        dwStatus = WAIT_FAILED;
                    }

                    if(dwStatus == WAIT_OBJECT_0)
                    {
                        _ASSERTE((*ppRWLock)->_dwState & WRITER_SIGNALED);
                        dwModifyState = WRITER - WAITING_WRITER - WRITER_SIGNALED;
                    }
                    else
                    {
                        dwModifyState = (DWORD) -WAITING_WRITER;
                        if(dwStatus == WAIT_TIMEOUT)
                        {
                            LOG((LF_SYNC, LL_WARNING,
                                "Timed out trying to acquire writer "
                                "lock for RWLock 0x%x\n", *ppRWLock));
                            dwStatus = ERROR_TIMEOUT;
                        }
                        else if(dwStatus == WAIT_IO_COMPLETION)
                        {
                            LOG((LF_SYNC, LL_WARNING,
                                "Thread interrupted while trying to acquire writer lock "
                                "for RWLock 0x%x\n", *ppRWLock));
                            dwStatus = COR_E_THREADINTERRUPTED;
                        }
                        else
                        {
                            dwStatus = GetLastError();
                            LOG((LF_SYNC, LL_WARNING,
                                "WaitForSingleObject on Event 0x%x failed for "
                                "RWLock 0x%x with status code 0x%x",
                                hWriterEvent, *ppRWLock, dwStatus));
                        }
                    }

                    // One less waiting writer and he may have become a writer
                    dwKnownState = RWInterlockedExchangeAdd(&(*ppRWLock)->_dwState, dwModifyState);

                    // Check for last timing out signaled waiting writer
                    if(dwStatus == WAIT_OBJECT_0)
                    {
                        // Common case
                    }
                    else
                    {
                        if((dwKnownState & WRITER_SIGNALED) &&
                           ((dwKnownState & WAITING_WRITERS_MASK) == WAITING_WRITER))
                        {
                            if(hWriterEvent == NULL)
                                hWriterEvent = (*ppRWLock)->GetWriterEvent();
                            _ASSERTE(hWriterEvent);
                            do
                            {
                                dwKnownState = (*ppRWLock)->_dwState;
                                if((dwKnownState & WRITER_SIGNALED) &&
                                   ((dwKnownState & WAITING_WRITERS_MASK) == 0))
                                {
                                    DWORD dwTemp = WaitForSingleObject(hWriterEvent, 10);
                                    if(dwTemp == WAIT_OBJECT_0)
                                    {
                                        dwKnownState = RWInterlockedExchangeAdd(&(*ppRWLock)->_dwState, (WRITER - WRITER_SIGNALED));
                                        _ASSERTE(dwKnownState & WRITER_SIGNALED);
                                        _ASSERTE((dwKnownState & WRITER) == 0);

                                        // Honor the orginal status
                                        (*ppRWLock)->_dwWriterID = dwThreadID;
                                        Thread *pThread = GetThread();
                                        _ASSERTE (pThread);
                                        pThread->m_dwLockCount -= (*ppRWLock)->_wWriterLevel - 1;
                                        _ASSERTE (pThread->m_dwLockCount >= 0);
                                        (*ppRWLock)->_wWriterLevel = 1;
                                        StaticReleaseWriterLock(ppRWLock);
                                        break;
                                    }
                                    // else continue;
                                }
                                else
                                    break;
                            }while(TRUE);
                        }

                        if(fBreakOnErrors)
                        {
                            _ASSERTE(!"Failed to acquire writer lock");
                            DebugBreak();
                        }
                        
                        // Prepare the frame for throwing an exception
                        COMPlusThrowWin32(dwStatus, NULL);
                    }

                    // Sanity check
                    _ASSERTE(dwStatus == WAIT_OBJECT_0);
                    break;
                }
            }
            pause();		// indicate to the processor that we are spinning 
        } while(TRUE);
    }

    // Success
    _ASSERTE((*ppRWLock)->_dwState & WRITER);
    _ASSERTE(((*ppRWLock)->_dwState & READERS_MASK) == 0);
    _ASSERTE((*ppRWLock)->_dwWriterID == 0);

    // Save threadid of the writer
    (*ppRWLock)->_dwWriterID = dwThreadID;
    (*ppRWLock)->_wWriterLevel = 1;
    INCTHREADLOCKCOUNT();
    ++(*ppRWLock)->_dwWriterSeqNum;
#ifdef RWLOCK_STATISTICS
    ++(*ppRWLock)->_dwWriterEntryCount;
#endif
    return;
}
// FCIMPLEND


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::StaticReleaseWriterLockPublic    public
//
//  Synopsis:   Public access to StaticReleaseWriterLock
//
//+-------------------------------------------------------------------
FCIMPL1(void, CRWLock::StaticReleaseWriterLockPublic, CRWLock *pRWLockUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    if (pRWLockUNSAFE == NULL)
    {
        FCThrowVoid(kNullReferenceException);
    }

    OBJECTREF pRWLock = ObjectToOBJECTREF((Object*)pRWLockUNSAFE);
    HELPER_METHOD_FRAME_BEGIN_1(pRWLock);

    StaticReleaseWriterLock((CRWLock**)&pRWLock);

    HELPER_METHOD_FRAME_END();
}
FCIMPLEND


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::StaticReleaseWriterLock    private
//
//  Synopsis:   Removes the thread as a writer if not a nested
//              call to release the lock
//+-------------------------------------------------------------------
void F_CALL_CONV CRWLock::StaticReleaseWriterLock(
    CRWLock **ppRWLock)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(ppRWLock);
    _ASSERTE(*ppRWLock);

    DWORD dwThreadID = GetCurrentThreadId();

    // Check validity of caller
    if((*ppRWLock)->_dwWriterID == dwThreadID)
    {
        DECTHREADLOCKCOUNT();
        // Check for nested release
        if(--(*ppRWLock)->_wWriterLevel == 0)
        {
            DWORD dwCurrentState, dwKnownState, dwModifyState;
            BOOL fCacheEvents;
            HANDLE hReaderEvent = NULL, hWriterEvent = NULL;

            // Not a writer any more
            (*ppRWLock)->_dwWriterID = 0;
            dwCurrentState = (*ppRWLock)->_dwState;
            do
            {
                dwKnownState = dwCurrentState;
                dwModifyState = (DWORD) -WRITER;
                fCacheEvents = FALSE;
                if(dwKnownState & WAITING_READERS_MASK)
                {
                    hReaderEvent = (*ppRWLock)->GetReaderEvent();
                    if(hReaderEvent == NULL)
                    {
                        LOG((LF_SYNC, LL_WARNING,
                            "ReleaseWriterLock failed to create "
                            "reader event for RWLock 0x%x\n", *ppRWLock));
                        RWSleep(100);
                        dwCurrentState = (*ppRWLock)->_dwState;
                        dwKnownState = 0;
                        _ASSERTE(dwCurrentState != dwKnownState);
                        continue;
                    }
                    dwModifyState += READER_SIGNALED;
                }
                else if(dwKnownState & WAITING_WRITERS_MASK)
                {
                    hWriterEvent = (*ppRWLock)->GetWriterEvent();
                    if(hWriterEvent == NULL)
                    {
                        LOG((LF_SYNC, LL_WARNING,
                            "ReleaseWriterLock failed to create "
                            "writer event for RWLock 0x%x\n", *ppRWLock));
                        RWSleep(100);
                        dwCurrentState = (*ppRWLock)->_dwState;
                        dwKnownState = 0;
                        _ASSERTE(dwCurrentState != dwKnownState);
                        continue;
                    }
                    dwModifyState += WRITER_SIGNALED;
                }
                else if(((*ppRWLock)->_hReaderEvent || (*ppRWLock)->_hWriterEvent) &&
                        (dwKnownState == WRITER))
                {
                    fCacheEvents = TRUE;
                    dwModifyState += CACHING_EVENTS;
                }

                // Sanity checks
                _ASSERTE((dwKnownState & READERS_MASK) == 0);

                dwCurrentState = RWInterlockedCompareExchange(&(*ppRWLock)->_dwState,
                                                              (dwKnownState + dwModifyState),
                                                              dwKnownState);
            } while(dwCurrentState != dwKnownState);

            // Check for waiting readers
            if(dwKnownState & WAITING_READERS_MASK)
            {
                _ASSERTE((*ppRWLock)->_dwState & READER_SIGNALED);
                _ASSERTE(hReaderEvent);
                RWSetEvent(hReaderEvent);
            }
            // Check for waiting writers
            else if(dwKnownState & WAITING_WRITERS_MASK)
            {
                _ASSERTE((*ppRWLock)->_dwState & WRITER_SIGNALED);
                _ASSERTE(hWriterEvent);
                RWSetEvent(hWriterEvent);
            }
            // Check for the need to release events
            else if(fCacheEvents)
            {
                (*ppRWLock)->ReleaseEvents();
            }
        }
    }
    else
    {
        if(fBreakOnErrors)
        {
            _ASSERTE(!"Attempt to release writer lock on a wrong thread");
            DebugBreak();
        }
        COMPlusThrowWin32(ERROR_NOT_OWNER, NULL);
    }

    return;
}
// FCIMPLEND


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::StaticReleaseReaderLockPublic    public
//
//  Synopsis:   Public access to StaticReleaseReaderLock
//
//+-------------------------------------------------------------------
FCIMPL1(void, CRWLock::StaticReleaseReaderLockPublic, CRWLock *pRWLockUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    if (pRWLockUNSAFE == NULL)
    {
        FCThrowVoid(kNullReferenceException);
    }

    OBJECTREF pRWLock = ObjectToOBJECTREF((Object*)pRWLockUNSAFE);
    HELPER_METHOD_FRAME_BEGIN_1(pRWLock);

    StaticReleaseReaderLock((CRWLock**)&pRWLock);

    HELPER_METHOD_FRAME_END();
}
FCIMPLEND


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::StaticReleaseReaderLock    private
//
//  Synopsis:   Removes the thread as a reader
//+-------------------------------------------------------------------

void F_CALL_CONV CRWLock::StaticReleaseReaderLock(
    CRWLock **ppRWLock)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(ppRWLock);
    _ASSERTE(*ppRWLock);

    // Check if the thread has writer lock
    if((*ppRWLock)->_dwWriterID == GetCurrentThreadId())
    {
        StaticReleaseWriterLock(ppRWLock);
    }
    else
    {
        LockEntry *pLockEntry = (*ppRWLock)->GetLockEntry();
        if(pLockEntry)
        {
            --pLockEntry->wReaderLevel;
            DECTHREADLOCKCOUNT();
            if(pLockEntry->wReaderLevel == 0)
            {
                DWORD dwCurrentState, dwKnownState, dwModifyState;
                BOOL fLastReader, fCacheEvents = FALSE;
                HANDLE hReaderEvent = NULL, hWriterEvent = NULL;

                // Sanity checks
                _ASSERTE(((*ppRWLock)->_dwState & WRITER) == 0);
                _ASSERTE((*ppRWLock)->_dwState & READERS_MASK);

                // Not a reader any more
                dwCurrentState = (*ppRWLock)->_dwState;
                do
                {
                    dwKnownState = dwCurrentState;
                    dwModifyState = (DWORD) -READER;
                    if((dwKnownState & (READERS_MASK | READER_SIGNALED)) == READER)
                    {
                        fLastReader = TRUE;
                        fCacheEvents = FALSE;
                        if(dwKnownState & WAITING_WRITERS_MASK)
                        {
                            hWriterEvent = (*ppRWLock)->GetWriterEvent();
                            if(hWriterEvent == NULL)
                            {
                                LOG((LF_SYNC, LL_WARNING,
                                    "ReleaseReaderLock failed to create "
                                    "writer event for RWLock 0x%x\n", *ppRWLock));
                                RWSleep(100);
                                dwCurrentState = (*ppRWLock)->_dwState;
                                dwKnownState = 0;
                                _ASSERTE(dwCurrentState != dwKnownState);
                                continue;
                            }
                            dwModifyState += WRITER_SIGNALED;
                        }
                        else if(dwKnownState & WAITING_READERS_MASK)
                        {
                            hReaderEvent = (*ppRWLock)->GetReaderEvent();
                            if(hReaderEvent == NULL)
                            {
                                LOG((LF_SYNC, LL_WARNING,
                                    "ReleaseReaderLock failed to create "
                                    "reader event\n", *ppRWLock));
                                RWSleep(100);
                                dwCurrentState = (*ppRWLock)->_dwState;
                                dwKnownState = 0;
                                _ASSERTE(dwCurrentState != dwKnownState);
                                continue;
                            }
                            dwModifyState += READER_SIGNALED;
                        }
                        else if(((*ppRWLock)->_hReaderEvent || (*ppRWLock)->_hWriterEvent) &&
                                (dwKnownState == READER))
                        {
                            fCacheEvents = TRUE;
                            dwModifyState += CACHING_EVENTS;
                        }
                    }
                    else
                    {
                        fLastReader = FALSE;
                    }

                    // Sanity checks
                    _ASSERTE((dwKnownState & WRITER) == 0);
                    _ASSERTE(dwKnownState & READERS_MASK);

                    dwCurrentState = RWInterlockedCompareExchange(&(*ppRWLock)->_dwState,
                                                                  (dwKnownState + dwModifyState),
                                                                  dwKnownState);
                } while(dwCurrentState != dwKnownState);

                // Check for last reader
                if(fLastReader)
                {
                    // Check for waiting writers
                    if(dwKnownState & WAITING_WRITERS_MASK)
                    {
                        _ASSERTE((*ppRWLock)->_dwState & WRITER_SIGNALED);
                        _ASSERTE(hWriterEvent);
                        RWSetEvent(hWriterEvent);
                    }
                    // Check for waiting readers
                    else if(dwKnownState & WAITING_READERS_MASK)
                    {
                        _ASSERTE((*ppRWLock)->_dwState & READER_SIGNALED);
                        _ASSERTE(hReaderEvent);
                        RWSetEvent(hReaderEvent);
                    }
                    // Check for the need to release events
                    else if(fCacheEvents)
                    {
                        (*ppRWLock)->ReleaseEvents();
                    }
                }

                // Recycle lock entry
                RecycleLockEntry(pLockEntry);
            }
        }
        else
        {
            if(fBreakOnErrors)
            {
                _ASSERTE(!"Attempt to release reader lock on a wrong thread");
                DebugBreak();
            }
            COMPlusThrowWin32(ERROR_NOT_OWNER, NULL);
        }
    }

    return;
}
// FCIMPLEND


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::StaticUpgradeToWriterLockPublic    public
//
//  Synopsis:   Public Access to StaticUpgradeToWriterLockPublic
//
//+-------------------------------------------------------------------
FCIMPL2_INST_RET_VC(LockCookie, pLockCookie, CRWLock::StaticUpgradeToWriterLockPublic, CRWLock *pRWLockUNSAFE, DWORD dwDesiredTimeout)
{
    THROWSCOMPLUSEXCEPTION();

    if (pRWLockUNSAFE == NULL)
    {
        FCThrowRetVC(kNullReferenceException);
    }

    OBJECTREF pRWLock = ObjectToOBJECTREF((Object*)pRWLockUNSAFE);

    HELPER_METHOD_FRAME_BEGIN_RET_VC_1(pRWLock);

    StaticUpgradeToWriterLock((CRWLock**)&pRWLock, pLockCookie, dwDesiredTimeout);

    HELPER_METHOD_FRAME_END();
}
FCIMPLEND_RET_VC

//+-------------------------------------------------------------------
//
//  Method:     CRWLock::StaticUpgradeToWriterLock    Private
//
//  Synopsis:   Upgrades to a writer lock. It returns a BOOL that
//              indicates intervening writes.
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------

void CRWLock::StaticUpgradeToWriterLock(
    CRWLock **ppRWLock, 
    LockCookie *pLockCookie, 
    DWORD dwDesiredTimeout)

{

    THROWSCOMPLUSEXCEPTION();
    DWORD dwThreadID = GetCurrentThreadId();

    // Check if the thread is already a writer
    if((*ppRWLock)->_dwWriterID == dwThreadID)
    {
        // Update cookie state
        pLockCookie->dwFlags = UPGRADE_COOKIE | COOKIE_WRITER;
        pLockCookie->wWriterLevel = (*ppRWLock)->_wWriterLevel;

        // Acquire the writer lock again
        StaticAcquireWriterLock(ppRWLock, dwDesiredTimeout);
    }
    else
    {
        BOOL fAcquireWriterLock;
        LockEntry *pLockEntry = (*ppRWLock)->GetLockEntry();
        if(pLockEntry == NULL)
        {
            fAcquireWriterLock = TRUE;
            pLockCookie->dwFlags = UPGRADE_COOKIE | COOKIE_NONE;
        }
        else
        {
            // Sanity check
            _ASSERTE((*ppRWLock)->_dwState & READERS_MASK);
            _ASSERTE(pLockEntry->wReaderLevel);

            // Save lock state in the cookie
            pLockCookie->dwFlags = UPGRADE_COOKIE | COOKIE_READER;
            pLockCookie->wReaderLevel = pLockEntry->wReaderLevel;
            pLockCookie->dwWriterSeqNum = (*ppRWLock)->_dwWriterSeqNum;

            // If there is only one reader, try to convert reader to a writer
            DWORD dwKnownState = RWInterlockedCompareExchange(&(*ppRWLock)->_dwState,
                                                              WRITER,
                                                              READER);
            if(dwKnownState == READER)
            {
                // Thread is no longer a reader
                Thread* pThread = GetThread();
                _ASSERTE (pThread);
                pThread->m_dwLockCount -= pLockEntry->wReaderLevel;
                _ASSERTE (pThread->m_dwLockCount >= 0);
                pLockEntry->wReaderLevel = 0;
                RecycleLockEntry(pLockEntry);

                // Thread is a writer
                (*ppRWLock)->_dwWriterID = dwThreadID;
                (*ppRWLock)->_wWriterLevel = 1;
                INCTHREADLOCKCOUNT();
                ++(*ppRWLock)->_dwWriterSeqNum;
                fAcquireWriterLock = FALSE;

                // No intevening writes
#if RWLOCK_STATISTICS
                ++(*ppRWLock)->_dwWriterEntryCount;
#endif
            }
            else
            {
                // Release the reader lock
                Thread *pThread = GetThread();
                _ASSERTE (pThread);
                pThread->m_dwLockCount -= (pLockEntry->wReaderLevel - 1);
                _ASSERTE (pThread->m_dwLockCount >= 0);
                pLockEntry->wReaderLevel = 1;
                StaticReleaseReaderLock(ppRWLock);
                fAcquireWriterLock = TRUE;
            }
        }

        // Check for the need to acquire the writer lock
        if(fAcquireWriterLock)
        {

            // Declare and Setup the frame as we are aware of the contention
            // on the lock and the thread will most probably block
            // to acquire writer lock

            COMPLUS_TRY
            {
                StaticAcquireWriterLock(ppRWLock, dwDesiredTimeout);
            }
            COMPLUS_CATCH
            {
                // Invalidate cookie
                DWORD dwFlags = pLockCookie->dwFlags; 
                pLockCookie->dwFlags = INVALID_COOKIE;

                StaticRecoverLock(ppRWLock, pLockCookie, dwFlags & COOKIE_READER);
                COMPlusRareRethrow();
            }
            COMPLUS_END_CATCH
        }
    }


    // Update the validation fields of the cookie 
    pLockCookie->dwThreadID = dwThreadID;

    return;
}

//+-------------------------------------------------------------------
//
//  Method:     CRWLock::StaticDowngradeFromWriterLock   public
//
//  Synopsis:   Downgrades from a writer lock.
//+-------------------------------------------------------------------

inline CRWLock* GetLock(OBJECTREF orLock)
{
    return (CRWLock*)OBJECTREFToObject(orLock);
}

FCIMPL2(void, CRWLock::StaticDowngradeFromWriterLock, CRWLock *pRWLockUNSAFE, LockCookie* pLockCookie)
{
    THROWSCOMPLUSEXCEPTION();

    DWORD dwThreadID = GetCurrentThreadId();

    if (pRWLockUNSAFE == NULL)
    {
        FCThrowVoid(kNullReferenceException);
    }

    OBJECTREF pRWLock = ObjectToOBJECTREF((Object*)pRWLockUNSAFE);
    HELPER_METHOD_FRAME_BEGIN_1(pRWLock);

    if (GetLock(pRWLock)->_dwWriterID != dwThreadID)
    {
        COMPlusThrowWin32(ERROR_NOT_OWNER, NULL);
    }

    // Validate cookie
    DWORD dwStatus;
    if(((pLockCookie->dwFlags & INVALID_COOKIE) == 0) && 
       (pLockCookie->dwThreadID == dwThreadID))
    {
        DWORD dwFlags = pLockCookie->dwFlags;
        pLockCookie->dwFlags = INVALID_COOKIE;
        
        // Check if the thread was a reader
        if(dwFlags & COOKIE_READER)
        {
            // Sanity checks
            _ASSERTE(GetLock(pRWLock)->_wWriterLevel == 1);
    
            LockEntry *pLockEntry = GetLock(pRWLock)->FastGetOrCreateLockEntry();
            if(pLockEntry)
            {
                DWORD dwCurrentState, dwKnownState, dwModifyState;
                HANDLE hReaderEvent = NULL;
    
                // Downgrade to a reader
                GetLock(pRWLock)->_dwWriterID = 0;
                GetLock(pRWLock)->_wWriterLevel = 0;
                DECTHREADLOCKCOUNT ();
                dwCurrentState = GetLock(pRWLock)->_dwState;
                do
                {
                    dwKnownState = dwCurrentState;
                    dwModifyState = READER - WRITER;
                    if(dwKnownState & WAITING_READERS_MASK)
                    {
                        hReaderEvent = GetLock(pRWLock)->GetReaderEvent();
                        if(hReaderEvent == NULL)
                        {
                            LOG((LF_SYNC, LL_WARNING,
                                "DowngradeFromWriterLock failed to create "
                                "reader event for RWLock 0x%x\n", GetLock(pRWLock)));
                            RWSleep(100);
                            dwCurrentState = GetLock(pRWLock)->_dwState;
                            dwKnownState = 0;
                            _ASSERTE(dwCurrentState != dwKnownState);
                            continue;
                        }
                        dwModifyState += READER_SIGNALED;
                    }
    
                    // Sanity checks
                    _ASSERTE((dwKnownState & READERS_MASK) == 0);
    
                    dwCurrentState = RWInterlockedCompareExchange(&GetLock(pRWLock)->_dwState,
                                                                  (dwKnownState + dwModifyState),
                                                                  dwKnownState);
                } while(dwCurrentState != dwKnownState);
    
                // Check for waiting readers
                if(dwKnownState & WAITING_READERS_MASK)
                {
                    _ASSERTE(GetLock(pRWLock)->_dwState & READER_SIGNALED);
                    _ASSERTE(hReaderEvent);
                    RWSetEvent(hReaderEvent);
                }
    
                // Restore reader nesting level
                Thread *pThread = GetThread();
                _ASSERTE (pThread);
                pThread->m_dwLockCount -= pLockEntry->wReaderLevel;
                _ASSERTE (pThread->m_dwLockCount >= 0);
                pLockEntry->wReaderLevel = pLockCookie->wReaderLevel;
                pThread->m_dwLockCount += pLockEntry->wReaderLevel;
    #ifdef RWLOCK_STATISTICS
                RWInterlockedIncrement(&GetLock(pRWLock)->_dwReaderEntryCount);
    #endif
            }
            else
            {
                _ASSERTE(!"Failed to restore the thread as a reader");
                dwStatus = RWLOCK_RECOVERY_FAILURE;
                goto ThrowException;
            }
        }
        else if(dwFlags & (COOKIE_WRITER | COOKIE_NONE))
        {
            // Release the writer lock
            StaticReleaseWriterLock((CRWLock**)&pRWLock);
            _ASSERTE((GetLock(pRWLock)->_dwWriterID != GetCurrentThreadId()) ||
                     (dwFlags & COOKIE_WRITER));
        }
    }
    else
    {
        dwStatus = E_INVALIDARG;
ThrowException:        
        COMPlusThrowWin32(dwStatus, NULL);
    }

    HELPER_METHOD_FRAME_END();

    // Update the validation fields of the cookie 
    pLockCookie->dwThreadID = dwThreadID;
    
}
FCIMPLEND


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::StaticReleaseLock    public
//
//  Synopsis:   Releases the lock held by the current thread
//+-------------------------------------------------------------------

FCIMPL1_INST_RET_VC(LockCookie, pLockCookie, CRWLock::StaticReleaseLock, CRWLock *pRWLockUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    DWORD dwThreadID = GetCurrentThreadId();

    if (pRWLockUNSAFE == NULL)
    {
        FCThrowRetVC(kNullReferenceException);
    }

    OBJECTREF pRWLock = ObjectToOBJECTREF((Object*)pRWLockUNSAFE);

    HELPER_METHOD_FRAME_BEGIN_RET_VC_1(pRWLock);

    // Check if the thread is a writer
    if(GetLock(pRWLock)->_dwWriterID == dwThreadID)
    {
        // Save lock state in the cookie
        pLockCookie->dwFlags = RELEASE_COOKIE | COOKIE_WRITER;
        pLockCookie->dwWriterSeqNum = GetLock(pRWLock)->_dwWriterSeqNum;
        pLockCookie->wWriterLevel = GetLock(pRWLock)->_wWriterLevel;

        // Release the writer lock
        Thread *pThread = GetThread();
        _ASSERTE (pThread);
        pThread->m_dwLockCount -= (GetLock(pRWLock)->_wWriterLevel - 1);
        _ASSERTE (pThread->m_dwLockCount >= 0);
        GetLock(pRWLock)->_wWriterLevel = 1;
        StaticReleaseWriterLock((CRWLock**)&pRWLock);
    }
    else
    {
        LockEntry *pLockEntry = GetLock(pRWLock)->GetLockEntry();
        if(pLockEntry)
        {
            // Sanity check
            _ASSERTE(GetLock(pRWLock)->_dwState & READERS_MASK);
            _ASSERTE(pLockEntry->wReaderLevel);

            // Save lock state in the cookie
            pLockCookie->dwFlags = RELEASE_COOKIE | COOKIE_READER;
            pLockCookie->wReaderLevel = pLockEntry->wReaderLevel;
            pLockCookie->dwWriterSeqNum = GetLock(pRWLock)->_dwWriterSeqNum;

            // Release the reader lock
            Thread *pThread = GetThread();
            _ASSERTE (pThread);
            pThread->m_dwLockCount -= (pLockEntry->wReaderLevel - 1);
            _ASSERTE (pThread->m_dwLockCount >= 0);
            pLockEntry->wReaderLevel = 1;
            StaticReleaseReaderLock((CRWLock**)&pRWLock);
        }
        else
        {
            pLockCookie->dwFlags = RELEASE_COOKIE | COOKIE_NONE;
        }
    }

    HELPER_METHOD_FRAME_END();

    // Update the validation fields of the cookie 
    pLockCookie->dwThreadID = dwThreadID;
    
}
FCIMPLEND_RET_VC


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::StaticRestoreLockPublic    public
//
//  Synopsis:   Public Access to StaticRestoreLock
//
//+-------------------------------------------------------------------

FCIMPL2(void, CRWLock::StaticRestoreLockPublic, CRWLock *pRWLockUNSAFE, LockCookie* pLockCookie)
{
    THROWSCOMPLUSEXCEPTION();

    if (pRWLockUNSAFE == NULL)
    {
        FCThrowVoid(kNullReferenceException);
    }

    OBJECTREF pRWLock = ObjectToOBJECTREF((Object*)pRWLockUNSAFE);
    HELPER_METHOD_FRAME_BEGIN_1(pRWLock);

    StaticRestoreLock((CRWLock**)&pRWLock, pLockCookie);
    
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

//+-------------------------------------------------------------------
//
//  Method:     CRWLock::StaticRestoreLock  Private
//
//  Synopsis:   Restore the lock held by the current thread
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------

void F_CALL_CONV CRWLock::StaticRestoreLock(
    CRWLock **ppRWLock, 
    LockCookie *pLockCookie)
{

    THROWSCOMPLUSEXCEPTION();
    // Validate cookie
    DWORD dwThreadID = GetCurrentThreadId();
    DWORD dwFlags = pLockCookie->dwFlags;
    if(pLockCookie->dwThreadID == dwThreadID)
    {
        // Assert that the thread does not hold reader or writer lock
        _ASSERTE(((*ppRWLock)->_dwWriterID != dwThreadID) && ((*ppRWLock)->GetLockEntry() == NULL));
    
        // Check for the no contention case
        pLockCookie->dwFlags = INVALID_COOKIE;
        if(dwFlags & COOKIE_WRITER)
        {
            if(RWInterlockedCompareExchange(&(*ppRWLock)->_dwState, WRITER, 0) == 0)
            {
                // Restore writer nesting level
                (*ppRWLock)->_dwWriterID = dwThreadID;
                Thread *pThread = GetThread();
                _ASSERTE (pThread);
                pThread->m_dwLockCount -= (*ppRWLock)->_wWriterLevel;
                _ASSERTE (pThread->m_dwLockCount >= 0);
                (*ppRWLock)->_wWriterLevel = pLockCookie->wWriterLevel;
                pThread->m_dwLockCount += (*ppRWLock)->_wWriterLevel;
                ++(*ppRWLock)->_dwWriterSeqNum;
#ifdef RWLOCK_STATISTICS
                ++(*ppRWLock)->_dwWriterEntryCount;
#endif
                goto LNormalReturn;
            }
        }
        else if(dwFlags & COOKIE_READER)
        {
            LockEntry *pLockEntry = (*ppRWLock)->FastGetOrCreateLockEntry();
            if(pLockEntry)
            {
                // This thread should not already be a reader
                // else bad things can happen
                _ASSERTE(pLockEntry->wReaderLevel == 0);
                DWORD dwKnownState = (*ppRWLock)->_dwState;
                if(dwKnownState < READERS_MASK)
                {
                    DWORD dwCurrentState = RWInterlockedCompareExchange(&(*ppRWLock)->_dwState,
                                                                        (dwKnownState + READER),
                                                                        dwKnownState);
                    if(dwCurrentState == dwKnownState)
                    {
                        // Restore reader nesting level
                        Thread *pThread = GetThread();
                        _ASSERTE (pThread);
                        pThread->m_dwLockCount -= pLockEntry->wReaderLevel;
                        _ASSERTE (pThread->m_dwLockCount >= 0);
                        pLockEntry->wReaderLevel = pLockCookie->wReaderLevel;
                        pThread->m_dwLockCount += pLockEntry->wReaderLevel;
#ifdef RWLOCK_STATISTICS
                        RWInterlockedIncrement(&(*ppRWLock)->_dwReaderEntryCount);
#endif
                        goto LNormalReturn;
                    }
                }
    
                // Recycle the lock entry for the slow case
                (*ppRWLock)->FastRecycleLockEntry(pLockEntry);
            }
            else
            {
                // Ignore the error and try again below. May be thread will luck
                // out the second time
            }
        }
        else if(dwFlags & COOKIE_NONE) 
        {
            goto LNormalReturn;
        }

        // Declare and Setup the frame as we are aware of the contention
        // on the lock and the thread will most probably block
        // to acquire lock below
ThrowException:        
        if((dwFlags & INVALID_COOKIE) == 0)
        {
            StaticRecoverLock(ppRWLock, pLockCookie, dwFlags);
        }
        else
        {
            COMPlusThrowWin32(E_INVALIDARG, NULL);
        }

        goto LNormalReturn;
    }
    else
    {
        dwFlags = INVALID_COOKIE;
        goto ThrowException;
    }

LNormalReturn:
    return;
}


//+-------------------------------------------------------------------
//
//  Class:      CRWLock::StaticPrivateInitialize
//
//  Synopsis:   Initialize lock
//+-------------------------------------------------------------------
FCIMPL1(void, CRWLock::StaticPrivateInitialize, CRWLock *pRWLock)
{
    // Run the constructor on the GC allocated space
#ifdef _DEBUG
    CRWLock *pTemp =
#endif
    new (pRWLock) CRWLock();
    _ASSERTE(pTemp == pRWLock);

    // Catch GC holes
    VALIDATE(pRWLock);

    return;
}
FCIMPLEND

//+-------------------------------------------------------------------
//
//  Class:      CRWLock::StaticGetWriterSeqNum
//
//  Synopsis:   Returns the current sequence number
//+-------------------------------------------------------------------
FCIMPL1(INT32, CRWLock::StaticGetWriterSeqNum, CRWLock *pRWLock)
{
    THROWSCOMPLUSEXCEPTION();

    if (pRWLock == NULL)
    {
        FCThrow(kNullReferenceException);
    }

    return(pRWLock->_dwWriterSeqNum);
}    
FCIMPLEND


//+-------------------------------------------------------------------
//
//  Class:      CRWLock::StaticAnyWritersSince
//
//  Synopsis:   Returns TRUE if there were writers since the given
//              sequence number
//+-------------------------------------------------------------------
FCIMPL2(INT32, CRWLock::StaticAnyWritersSince, CRWLock *pRWLock, DWORD dwSeqNum)
{
    THROWSCOMPLUSEXCEPTION();

    if (pRWLock == NULL)
    {
        FCThrow(kNullReferenceException);
    }
    

    if(pRWLock->_dwWriterID == GetCurrentThreadId())
        ++dwSeqNum;

    return(pRWLock->_dwWriterSeqNum > dwSeqNum);
}
FCIMPLEND
