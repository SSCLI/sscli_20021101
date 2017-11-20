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
/* ----------------------------------------------------------------------------





---------------------------------------------------------------------------- */
#ifndef __UTSEM_H__
#define __UTSEM_H__


// -------------------------------------------------------------
//              INCLUDES
// -------------------------------------------------------------
#include "utilcode.h"


// -------------------------------------------------------------
//              CONSTANTS AND TYPES
// -------------------------------------------------------------
typedef enum {SLC_WRITE, SLC_READWRITE, SLC_READWRITEPROMOTE}
             SYNCH_LOCK_CAPS;

typedef enum {SLT_READ, SLT_READPROMOTE, SLT_WRITE}
             SYNCH_LOCK_TYPE;

const int NUM_SYNCH_LOCK_TYPES = SLT_WRITE + 1;



// -------------------------------------------------------------
//              FORWARDS
// -------------------------------------------------------------
class CSemExclusive;
class CSemExclusiveSL;
class CLock;
class UTGuard;
class UTSemReadWrite;
class UTSemRWMgrRead;
class UTSemRWMgrWrite;


// -------------------------------------------------------------
//                  GLOBAL HELPER FUNCTIONS
// -------------------------------------------------------------


/* ----------------------------------------------------------------------------
@class CSemExclusive:

    An instance of this class represents an exclusive lock. If one thread calls
    Lock(), it will wait until any other thread that has called Lock() calls
    Unlock().

    A thread MAY call Lock multiple times.  It needs to call Unlock a matching
    number of times before the object is available to other threads.

---------------------------------------------------------------------------- */
class CSemExclusive
{
public:
    // Parameter to constructor is the maximum number of times to spin when
    // attempting to acquire the lock. The thread will sleep if the lock does
    // not become available in this period.
    CSemExclusive (DWORD ulcSpinCount = 0);
    ~CSemExclusive (void)           { DeleteCriticalSection (&m_csx); }
    void Lock (void)                { EnterCriticalSection (&m_csx); _ASSERTE(++m_iLocks > 0);}
    void UnLock (void)              { _ASSERTE(--m_iLocks >= 0);  LeaveCriticalSection (&m_csx); }

#ifdef _DEBUG
    int IsLocked()                  { return (m_iLocks > 0); }
#endif

private:
    CRITICAL_SECTION m_csx;
#ifdef _DEBUG
    int             m_iLocks;           // Count of locks.
#endif
};  //end class CSemExclusive





/* ----------------------------------------------------------------------------
@class CSemExclusiveSL:

    A subclass of CSemExclusive with a different default constructor.
    This subclass is appropriate for locks that are:
    *   Frequently Lock'd and Unlock'd, and
    *   Are held for very brief intervals.

---------------------------------------------------------------------------- */
class CSemExclusiveSL : public CSemExclusive
{
public:
    CSemExclusiveSL (DWORD ulcSpinCount = 400) : CSemExclusive (ulcSpinCount) {}
};


/* ----------------------------------------------------------------------------
@class CLock

    An instance of this class represents the holding of a lock on an instance
    of CSemExclusive. When this object is destroyed, the read lock will be
    automatically released.

---------------------------------------------------------------------------- */

class CLock
{
public:
    CLock (CSemExclusive* val) : m_locked(TRUE), m_pSem(val) {m_pSem->Lock();}
    CLock (CSemExclusive& val) : m_locked(TRUE), m_pSem(&val) {m_pSem->Lock();}
    CLock (CSemExclusive* val,BOOL fInitiallyLocked) : m_locked(fInitiallyLocked), m_pSem(val) {if (fInitiallyLocked) m_pSem->Lock();}
    ~CLock () { if (m_locked) m_pSem->UnLock(); }
    void Unlock () {m_pSem->UnLock(); m_locked = FALSE;}
    void Lock () {m_pSem->Lock(); m_locked = TRUE;}

private:
    BOOL m_locked;
    CSemExclusive* m_pSem;
};


/* ----------------------------------------------------------------------------
Convenience #define's for use with CSemExclusive and CLock

  These #define's assume that your object has a member named 'm_semCritical'.

  LOCK()        // locks m_semCritical; released at end of function if not sooner
  LOCKON(x)     // locks 'x', which is a CSemExclusive
  UNLOCK()      // undo effect of LOCK(), LOCKON(x), or RELOCK()
  RELOCK()      // re-locks the object that UNLOCK() unlocks
  READYLOCK()   // allows use of RELOCK() and UNLOCK() to lock 'm_semCritical', but
                // does not grab the lock at this time.
  READYLOCKON(x)// allows use of RELOCK() and UNLOCK() to lock 'x', but does not
                // grab the lock at this time.

---------------------------------------------------------------------------- */



#define AUTO_CRIT_LOCK(plck) CLock __sLock(plck)
#define CRIT_LOCK(plck) plck->Lock()
#define CRIT_UNLOCK(plck) plck->Unlock()




/* ----------------------------------------------------------------------------
@function VipInterlockedCompareExchange:
    This function is exactly equivalent to the system-defined InterlockedCompareExchange,
    except that it is inline, and therefore works on Win95.

    It does the following atomically:
    {
        temp = *Destination;
        if (*Destination == Comperand)
            *Destination = Exchange;
        return temp;
    }

    This function occufrs in three variations, for parameters of type VOID*,
    LONG, and ULONG.

---------------------------------------------------------------------------- */


inline LONG VipInterlockedCompareExchange(ULONG volatile * Destination, ULONG Exchange, ULONG Comparand)
{
    return InterlockedCompareExchange((LPLONG)Destination, (LONG)Exchange, (LONG)Comparand);
}

inline LONG VipInterlockedCompareExchange(LONG volatile * Destination, LONG Exchange, LONG Comparand)
{
    return InterlockedCompareExchange((LPLONG)Destination, Exchange, Comparand);
}

inline PVOID VipInterlockedCompareExchange(PVOID volatile * Destination, PVOID Exchange, PVOID Comparand)
{
    return InterlockedCompareExchangePointer((PVOID*)Destination, Exchange, Comparand);
}


/* ----------------------------------------------------------------------------
@class  UTGuard
    This object represents a guard that can be acquired or released. The 
    advantage with useing this instead of a critical section is that this
    is non blocking. If AcquireGuard fails, it will return false and will not
    block.

---------------------------------------------------------------------------- */
class UTGuard
{
private:
    LONG            m_lVal;

public:
    //@cmember Constructor
    UTGuard (void)                      { m_lVal = 0; }
    //@cmember Destructor
    ~UTGuard (void)                     {}

    //@cmember  Acquires the guard
    BOOL            AcquireGuard (void) { return 0 == InterlockedExchange (&m_lVal, 1); }
    //@cmember  Releases the guard
    void            ReleaseGuard (void) { m_lVal = 0; }
    
    //@cmember  Initializes the Guard
    void            Init (void)         { m_lVal = 0; }
} ; //End class UTGuard



/* ----------------------------------------------------------------------------
@class UTSemReadWrite

    An instance of class UTSemReadWrite provides multi-read XOR single-write
    (a.k.a. shared vs. exclusive) lock capabilities, with protection against
    writer starvation.

    A thread MUST NOT call any of the Lock methods if it already holds a Lock.
    (Doing so may result in a deadlock.)

---------------------------------------------------------------------------- */
class UTSemReadWrite
{
public:
    UTSemReadWrite(DWORD ulcSpinCount = 0,
            LPCSTR szSemaphoreName = NULL, LPCSTR szEventName = NULL); // Constructor
	~UTSemReadWrite(void);                  // Destructor

    // This implementation supports Read and Write locks
    SYNCH_LOCK_CAPS GetCaps(void)   { return SLC_READWRITE; };

    void LockRead(void);                    // Lock the object for reading
    void LockWrite(void);                   // Lock the object for writing
    void UnlockRead(void);                  // Unlock the object for reading
    void UnlockWrite(void);                 // Unlock the object for writing

    // This object is valid if it was initialized
    BOOL IsValid(void)              { return TRUE; }

    BOOL Lock(SYNCH_LOCK_TYPE t)            // Lock the object, mode specified by parameter
    {
        if (t == SLT_READ)
        {
            LockRead();
            return TRUE;
        }

        if (t == SLT_WRITE)
        {
            LockWrite();
            return TRUE;
        }
        return FALSE;
    }

    BOOL UnLock(SYNCH_LOCK_TYPE t)          // Unlock the object, mode specified by parameter
    {
        if (t == SLT_READ)
        {
            UnlockRead();
            return TRUE;
        }
        if (t == SLT_WRITE)
        {
            UnlockWrite();
            return TRUE;
        }
        return FALSE;
    }

private:
    virtual HANDLE GetReadWaiterSemaphore(void); // return Read Waiter Semaphore handle, creating if necessary
    virtual HANDLE GetWriteWaiterEvent (void); // return Write Waiter Event handle, creating if necessary

    DWORD m_ulcSpinCount;                   // spin counter
    volatile ULONG m_dwFlag;                // internal state, see implementation
    HANDLE m_hReadWaiterSemaphore;          // semaphore for awakening read waiters
    HANDLE m_hWriteWaiterEvent;             // event for awakening write waiters
    LPCSTR m_szSemaphoreName;               // For cross process handle creation.
    LPCSTR m_szEventName;                   // For cross process handle creation.
};



/* ----------------------------------------------------------------------------
@class UTSemRWMgrRead

    An instance of this class represents the holding of a read lock on an instance
    of UTSemReadWrite. When this object is destroyed, the read lock will be
    automatically released.

---------------------------------------------------------------------------- */

class UTSemRWMgrRead
{
public:
    UTSemRWMgrRead (UTSemReadWrite* pSemRW) { m_pSemRW = pSemRW; pSemRW->LockRead(); };
    ~UTSemRWMgrRead () { m_pSemRW->UnlockRead(); }

private:
    UTSemReadWrite* m_pSemRW;
};


/* ----------------------------------------------------------------------------
@class UTSemRWMgrWrite

    An instance of this class represents the holding of a write lock on an instance
    of UTSemReadWrite. When this object is destroyed, the write lock will be
    automatically released.

---------------------------------------------------------------------------- */

class UTSemRWMgrWrite
{
public:
    inline UTSemRWMgrWrite (UTSemReadWrite* pSemRW) { m_pSemRW = pSemRW; pSemRW->LockWrite(); };
    inline ~UTSemRWMgrWrite () { m_pSemRW->UnlockWrite(); }

private:
    UTSemReadWrite* m_pSemRW;
};


#endif // __UTSEM_H__
