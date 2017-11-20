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
//  File:       RWLock.h
//
//  Contents:   Reader writer lock implementation that supports the
//              following features
//                  1. Cheap enough to be used in large numbers
//                     such as per object synchronization.
//                  2. Supports timeout. This is a valuable feature
//                     to detect deadlocks
//                  3. Supports caching of events. The allows
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
//  Classes:    CRWLock,
//              CStaticRWLock
//--------------------------------------------------------------------
#ifndef _RWLOCK_H_
#define _RWLOCK_H_
#include "common.h"
#include "threads.h"
#include "ecall.h"
#include <member-offset-info.h>

#ifdef _TESTINGRWLOCK
/***************************************************/
#define LF_SYNC         0x1
#define LL_WARNING      0x2
#define LL_INFO10       0x2
extern void DebugOutput(int expr, int value, char *string, ...);
extern void MyAssert(int expr, char *string, ...);
extern SYSTEM_INFO g_SystemInfo;
extern HANDLE g_hProcessHeap;
#define LOG(Arg)  DebugOutput Arg
#define _ASSERTE(expr) MyAssert((int)(expr), "Assert:%s, File:%s, Line:%-d\n",  #expr, __FILE__, __LINE__)
/**************************************************/
#endif

#define RWLOCK_STATISTICS     0   //                          Temporarily collect statistics

extern DWORD gdwDefaultTimeout;
extern DWORD gdwDefaultSpinCount;


//+-------------------------------------------------------------------
//
//  Struct:     LockCookie
//
//  Synopsis:   Lock cookies returned to the client
//
//+-------------------------------------------------------------------
typedef struct {
    DWORD dwFlags;
    DWORD dwWriterSeqNum;
    WORD wReaderLevel;
    WORD wWriterLevel;
    DWORD dwThreadID;
} LockCookie;

//+-------------------------------------------------------------------
//
//  Class:      CRWLock
//
//  Synopsis:   Class the implements the reader writer locks. 
//+-------------------------------------------------------------------
class CRWLock
{
  friend struct MEMBER_OFFSET_INFO(CRWLock);
public:
    // Constuctor
    CRWLock();

    // Cleanup
    void Cleanup();

    // Lock functions
    void AcquireReaderLock(DWORD dwDesiredTimeout = gdwDefaultTimeout);
    void AcquireWriterLock(DWORD dwDesiredTimeout = gdwDefaultTimeout);
    void ReleaseReaderLock();
    void ReleaseWriterLock();
    void RestoreLock(LockCookie *pLockCookie);
    BOOL IsReaderLockHeld();
    BOOL IsWriterLockHeld();
    DWORD GetWriterSeqNum();
    BOOL AnyWritersSince(DWORD dwSeqNum);

    // Statics that do the core work
    static FCDECL1 (void, StaticPrivateInitialize, CRWLock *pRWLock);
    static FCDECL2 (void, StaticAcquireReaderLockPublic, CRWLock *pRWLock, DWORD dwDesiredTimeout);
    static FCDECL2 (void, StaticAcquireWriterLockPublic, CRWLock *pRWLock, DWORD dwDesiredTimeout);
    static FCDECL1 (void, StaticReleaseReaderLockPublic, CRWLock *pRWLock);
    static FCDECL1 (void, StaticReleaseWriterLockPublic, CRWLock *pRWLock);
    static FCDECL2_INST_RET_VC (LockCookie, pLockCookie, StaticUpgradeToWriterLockPublic, CRWLock *pRWLock, DWORD dwDesiredTimeout);
    static FCDECL2 (void, StaticDowngradeFromWriterLock, CRWLock *pRWLock, LockCookie* pLockCookie);
    static FCDECL1_INST_RET_VC (LockCookie, pLockCookie, StaticReleaseLock, CRWLock *pRWLock);
    static FCDECL2 (void, StaticRestoreLockPublic, CRWLock *pRWLock, LockCookie* pLockCookie);
    static FCDECL1 (BOOL, StaticIsReaderLockHeld, CRWLock *pRWLock);
    static FCDECL1 (BOOL, StaticIsWriterLockHeld, CRWLock *pRWLock);
    static FCDECL1 (INT32, StaticGetWriterSeqNum, CRWLock *pRWLock);
    static FCDECL2 (INT32, StaticAnyWritersSince, CRWLock *pRWLock, DWORD dwSeqNum);
private:
    static FCDECL2 (void, StaticAcquireReaderLock, CRWLock **ppRWLock, DWORD dwDesiredTimeout);
    static FCDECL2 (void, StaticAcquireWriterLock, CRWLock **ppRWLock, DWORD dwDesiredTimeout);
    static FCDECL1 (void, StaticReleaseReaderLock, CRWLock **ppRWLock);
    static FCDECL1 (void, StaticReleaseWriterLock, CRWLock **ppRWLock);
    static FCDECL3 (void, StaticRecoverLock, CRWLock **ppRWLock, LockCookie *pLockCookie, DWORD dwFlags);
    static FCDECL2 (void, StaticRestoreLock, CRWLock **ppRWLock, LockCookie *pLockCookie);
    static FCDECL3 (void, StaticUpgradeToWriterLock, CRWLock **ppRWLock, LockCookie *pLockCookie, DWORD dwDesiredTimeout);
public:
    // Assert functions
#ifdef _DEBUG
    BOOL AssertWriterLockHeld();
    BOOL AssertWriterLockNotHeld();
    BOOL AssertReaderLockHeld();
    BOOL AssertReaderLockNotHeld();
    BOOL AssertReaderOrWriterLockHeld();
    void AssertHeld()                            { AssertWriterLockHeld(); }
    void AssertNotHeld()                         { AssertWriterLockNotHeld();
                                                   AssertReaderLockNotHeld(); }
#else
    void AssertWriterLockHeld()                  {  }
    void AssertWriterLockNotHeld()               {  }
    void AssertReaderLockHeld()                  {  }
    void AssertReaderLockNotHeld()               {  }
    void AssertReaderOrWriterLockHeld()          {  }
    void AssertHeld()                            {  }
    void AssertNotHeld()                         {  }
#endif

    // Helper functions
#ifdef RWLOCK_STATISTICS
    DWORD GetReaderEntryCount()                  { return(_dwReaderEntryCount); }
    DWORD GetReaderContentionCount()             { return(_dwReaderContentionCount); }
    DWORD GetWriterEntryCount()                  { return(_dwWriterEntryCount); }
    DWORD GetWriterContentionCount()             { return(_dwWriterContentionCount); }
#endif
    // Static functions
    static void *operator new(size_t size)       { return ::operator new(size); }
    static void ProcessInit();
    static void SetTimeout(DWORD dwTimeout)      { gdwDefaultTimeout = dwTimeout; }
    static DWORD GetTimeout()                    { return(gdwDefaultTimeout); }
    static void SetSpinCount(DWORD dwSpinCount)  { gdwDefaultSpinCount = g_SystemInfo.dwNumberOfProcessors > 1
                                                                         ? dwSpinCount
                                                                         : 0; }
    static DWORD GetSpinCount()                  { return(gdwDefaultSpinCount); }

private:
    // Private helpers
    static void ChainEntry(Thread *pThread, LockEntry *pLockEntry);
    LockEntry *GetLockEntry();
    LockEntry *FastGetOrCreateLockEntry();
    LockEntry *SlowGetOrCreateLockEntry(Thread *pThread);
    void FastRecycleLockEntry(LockEntry *pLockEntry);
    static void RecycleLockEntry(LockEntry *pLockEntry);

    HANDLE GetReaderEvent();
    HANDLE GetWriterEvent();
    void ReleaseEvents();

    static LONG RWInterlockedCompareExchange(LONG volatile *pvDestination,
                                              LONG dwExchange,
                                              LONG dwComperand);
    static void* RWInterlockedCompareExchangePointer(PVOID volatile *pvDestination,
                                                   PVOID pExchange,
                                                   PVOID pComparand);
    static LONG RWInterlockedExchangeAdd(LONG volatile *pvDestination, LONG dwAddState);
    static LONG RWInterlockedIncrement(LONG volatile *pdwState);
    static DWORD RWWaitForSingleObject(HANDLE event, DWORD dwTimeout);
    static void RWSetEvent(HANDLE event);
    static void RWResetEvent(HANDLE event);
    static void RWSleep(DWORD dwTime);

    // private new
    static void *operator new(size_t size, void *pv)   { return(pv); }

    // Private data
    void *_pMT;
    HANDLE _hWriterEvent;
    HANDLE _hReaderEvent;
    volatile LONG _dwState;
    LONG _dwULockID;
    LONG _dwLLockID;
    DWORD _dwWriterID;
    DWORD _dwWriterSeqNum;
    WORD _wFlags;
    WORD _wWriterLevel;
#ifdef RWLOCK_STATISTICS
    volatile LONG _dwReaderEntryCount;
    volatile LONG _dwReaderContentionCount;
    volatile LONG _dwWriterEntryCount;
    volatile LONG _dwWriterContentionCount;
    volatile LONG _dwEventsReleasedCount;
#endif

    // Static data
    static volatile LONG s_mostRecentULockID;
    static volatile LONG s_mostRecentLLockID;
#ifdef _TESTINGRWLOCK
    static CRITICAL_SECTION s_RWLockCrst;
#else
    static CrstStatic       s_RWLockCrst;
#endif
};

inline void CRWLock::AcquireReaderLock(DWORD dwDesiredTimeout)
{
    StaticAcquireReaderLockPublic(this, dwDesiredTimeout);
}
inline void CRWLock::AcquireWriterLock(DWORD dwDesiredTimeout)
{
    StaticAcquireWriterLockPublic(this, dwDesiredTimeout);
}
inline void CRWLock::ReleaseReaderLock()
{
    StaticReleaseReaderLockPublic(this);
}
inline void CRWLock::ReleaseWriterLock()
{
    StaticReleaseWriterLockPublic(this);
}
inline void CRWLock::RestoreLock(LockCookie *pLockCookie)
{
    StaticRestoreLockPublic(this, pLockCookie);
}
inline BOOL CRWLock::IsReaderLockHeld()
{
    return(StaticIsReaderLockHeld(this));
}
inline BOOL CRWLock::IsWriterLockHeld()
{
    return(StaticIsWriterLockHeld(this));
}
// The following are the inlined versions of the static 
// functions
inline DWORD CRWLock::GetWriterSeqNum()
{
    return(_dwWriterSeqNum);
}
inline BOOL CRWLock::AnyWritersSince(DWORD dwSeqNum)
{ 
    if(_dwWriterID == GetCurrentThreadId())
        ++dwSeqNum;

    return(_dwWriterSeqNum > dwSeqNum);
}

#endif // _RWLOCK_H_
