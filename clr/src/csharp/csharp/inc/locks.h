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
// File: locks.h
//
// ===========================================================================

#ifndef _LOCKS_H_
#define _LOCKS_H_



////////////////////////////////////////////////////////////////////////////////
// Snooze
//
// This is a friendly way to give up a time slice.  It uses Sleep(1), which
// allows lower-priority threads to execute, and then calls PeekMessage, which
// allows sent messages to be processed by this thread (allowing COM/RPC messages
// to go through, etc.)

inline void Snooze ()
{
    Sleep(1);
}


#ifdef USE_RWLOCKS

class CLockBase;

////////////////////////////////////////////////////////////////////////////////
// CLockState
//
// This class holds lock state for a given thread

class CLockState
{
private:
    long        m_iCount;           // Number of locks held
    CLockBase   *m_rgpLocks[6];     // First 6 locks go here
    CLockBase   **m_ppMoreLocks;    // Next m_iCount - 6 locks go here

    CLockBase   *GetLock (long i) { return i < 6 ? m_rgpLocks[i] : m_ppMoreLocks[i-6]; }

public:
    CLockState() : m_iCount(0), m_ppMoreLocks(NULL) { ZeroMemory (m_rgpLocks, sizeof (m_rgpLocks)); }
    ~CLockState() { if (m_ppMoreLocks != NULL) free (m_ppMoreLocks); }

    ////////////////////////////////////////////////////////////////////////////
    // CLockState::Lock -- add the given lock to the list of held locks

    void        Lock (CLockBase *pLock)
    {
        if (m_iCount < 6)
        {
            // Lock will fit in fixed area...
            m_rgpLocks[m_iCount++] = pLock;
            return;
        }

        // Allocate space for locks on 8-lock boundary
        if (m_ppMoreLocks == NULL)
        {
            m_ppMoreLocks = (CLockBase **)malloc (((m_iCount + 2) & ~7) * sizeof (CLockBase *));
        }
        else
        {
            m_ppMoreLocks = (CLockBase **)realloc (m_ppMoreLocks, ((m_iCount + 2) & ~7) * sizeof (CLockBase *));
        }
        if (m_ppMoreLocks == NULL)
        {
            VSFAIL ("LOCK STATE OUT OF MEMORY!  BADNESS!");
            return;
        }

        m_ppMoreLocks[m_iCount-6] = pLock;
        m_iCount++;
    }

    ////////////////////////////////////////////////////////////////////////////
    // CLockState::Unlock -- remove the given lock from the list of held locks

    void        Unlock (CLockBase *pLock)
    {
        long        i, iFixed = min (m_iCount, 6);

        // Search the fixed area first...
        for (i=0; i<iFixed; i++)
        {
            if (m_rgpLocks[i] == pLock)
            {
                if (i < m_iCount - 1)
                    m_rgpLocks[i] = GetLock (m_iCount - 1);
                m_iCount--;
                return;
            }
        }

        DBGOUT (("LOCK:  Searching dynamic section of lock state..."));
        for (i=0; i<m_iCount-6; i++)
        {
            if (m_ppMoreLocks[i] == pLock)
            {
                if (i < m_iCount - 7)
                    m_ppMoreLocks[i] = m_ppMoreLocks[m_iCount - 7];
                m_iCount--;
                return;
            }
        }

        VSFAIL ("LOCK STATE:  Lock not found in lock state!");
    }

    ////////////////////////////////////////////////////////////////////////////
    // CLockState::IsLocked -- returns TRUE if the given lock is in the held list

    BOOL        IsLocked (CLockBase *pLock)
    {
        long        i, iFixed = min (m_iCount, 6);

        // Search the fixed area first...
        for (i=0; i<iFixed; i++)
            if (m_rgpLocks[i] == pLock)
                return TRUE;

#ifdef _DEBUG
        if (m_iCount > 6)
            DBGOUT (("LOCK:  Searching dynamic section of lock state..."));
#endif
        for (i=0; i<m_iCount-6; i++)
            if (m_ppMoreLocks[i] == pLock)
                return TRUE;

        return FALSE;
    }
};

////////////////////////////////////////////////////////////////////////////////
// LOCKKEY -- values returned by CLockBase::AcquireLock() and
// CLockBase::LockedByMe() to indicate lock states.  (Note that ACQUIRED_WRITE
// will never be returned by LockedByMe(), because write locks are also read
// locks.)

enum LOCKKEY
{
    ACQUIRED_NOTHING = 0,       // NOTE:  Must be zero!  Allows boolean check for LockedByMe()...
    ACQUIRED_READ,
    ACQUIRED_WRITE,
    ACQUIRED_READWRITE
};

////////////////////////////////////////////////////////////////////////////////
// CLockBase

class CLockBase
{
private:
    static      DWORD       m_dwSlot;                   // Where lock states are stored

    volatile    long        m_iLock;                    // Lock busy flag
                unsigned    m_fWrite:1;                 // Locked for write
                unsigned    m_iReadThreads:31;          // Number of reader threads

    static  CLockState  *GetLockState ();
    static  void        FreeLockState ();

public:
    CLockBase() : m_iLock(0), m_fWrite(FALSE), m_iReadThreads(0) {}

    static  void        DllMain (DWORD dwReason);

    LOCKKEY AcquireLock (BOOL fWrite);
    void    ReleaseLock (LOCKKEY key);

    ////////////////////////////////////////////////////////////////////////////
    // CLockBase::LockedByMe -- tests the lock to see if this thread currently
    // has it locked.

    LOCKKEY LockedByMe ()
    {
        CLockState  *pState = GetLockState();
        BOOL        fIsLocked = pState ? pState->IsLocked (this) : FALSE;

        if (!fIsLocked)
            return ACQUIRED_NOTHING;        // Not locked

        if (m_fWrite)
            return ACQUIRED_READWRITE;      // We're writing

        return ACQUIRED_READ;               // We must be just reading
    }
};

#endif // USE_RWLOCKS

////////////////////////////////////////////////////////////////////////////////
// CTinyLock
//
// Smallest possible implementation of an exclusive lock.

class CTinyLock
{
private:
    volatile    DWORD       m_dwOwner;          // Owning thread (non-zero means locked)

    void        Spin (long *piSpinCount) 
    { 
        if ((*piSpinCount)-- < 0) 
        { 
            InterlockedIncrement (&m_iSleeps); 
            Snooze(); 
            *piSpinCount = m_iSpinStart; 
        } 
        else if (*piSpinCount == (m_iSpinStart - 1)) 
        { 
            InterlockedIncrement (&m_iSpins); 
        }
    }

public:
    static  LONG        m_iSpins;
    static  LONG        m_iSleeps;
    static  LONG        m_iSpinStart;

public:
    CTinyLock() : m_dwOwner(0) 
    {
        if (m_iSpinStart == -1)
        {
            SYSTEM_INFO si;
            GetSystemInfo (&si);
            if (si.dwNumberOfProcessors > 1)
                m_iSpinStart = 4000;            // Hard-coded spin count...
            else
                m_iSpinStart = 0;
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    // CTinyLock::Acquire -- lock this object, returning TRUE if this is the
    // first lock, FALSE otherwise.  If TRUE is returned, Release() must be
    // called to unlock the object.

    BOOL    Acquire ()
    {
        long    iSpin = m_iSpinStart;
        DWORD   dwOld, dwTid = GetCurrentThreadId();

        while (TRUE)
        {
            dwOld = (DWORD)InterlockedCompareExchange ((LONG*)&m_dwOwner, (LONG)dwTid, 0);
            if (dwOld == 0 || dwOld == dwTid)
                break;
            Spin (&iSpin);
        }

        return dwOld == 0;
    }

    ////////////////////////////////////////////////////////////////////////////
    // CTinyLock::AttemptAcquire -- try to lock this object, returning TRUE if
    // successful, FALSE if not (either because it is locked by another thread,
    // OR if locked by this thread).  Never blocks.

    BOOL    AttemptAcquire ()
    {
        DWORD   dwOld, dwTid = GetCurrentThreadId();
        dwOld = (DWORD)InterlockedCompareExchange ((LONG*)&m_dwOwner, (LONG)dwTid, 0);
        return dwOld == 0;
    }

    ////////////////////////////////////////////////////////////////////////////
    // CTinyLock::Release -- release the lock held by this thread.  This should
    // NOT be called if Acquire() returned FALSE, indicating that this thread
    // already had this object locked.

    void    Release ()
    {
        ASSERT (m_dwOwner == GetCurrentThreadId());
        m_dwOwner = 0;
    }

    ////////////////////////////////////////////////////////////////////////////
    // CTinyLock::LockedByMe -- tests the lock to see if this thread currently
    // has it locked.

    BOOL    LockedByMe () { return m_dwOwner == GetCurrentThreadId(); }

    ////////////////////////////////////////////////////////////////////////////
    // CTinyLock::IsLocked -- tests the lock to see if it is locked or not (by
    // any thread)

    BOOL    IsLocked () { return m_dwOwner != 0; }
};



////////////////////////////////////////////////////////////////////////////////
// Gates -- these objects handle locking/unlocking the lock object provided,
// during their own construction/destruction, making scope-locking very simple --
// just create one on the stack.
////////////////////////////////////////////////////////////////////////////////

#ifdef USE_RWLOCKS

////////////////////////////////////////////////////////////////////////////////
// CGate -- handles scope-locking for any CLockBase derivation

class CGate
{
private:
    CLockBase   *m_pLock;
    LOCKKEY     m_key;

public:
    CGate (CLockBase *p, BOOL fWrite = FALSE) : m_pLock(p) { m_key = (m_pLock == NULL) ? ACQUIRED_NOTHING : m_pLock->AcquireLock (fWrite); }
    ~CGate () { Release (); }
    void    Detach () { m_pLock = NULL; }
    void    Release () { if (m_pLock != NULL) { m_pLock->ReleaseLock (m_key); m_pLock = NULL; } }
    void    RevertToRead () { if (m_pLock != NULL && m_key == ACQUIRED_READWRITE) { m_pLock->ReleaseLock (ACQUIRED_WRITE); m_key = ACQUIRED_READ; } }
    void    Acquire (CLockBase *p, BOOL fWrite = FALSE) { ASSERT (m_pLock == NULL); m_pLock = p; m_key = p->AcquireLock (fWrite); }
};

#endif // USE_RWLOCKS

////////////////////////////////////////////////////////////////////////////////
// CTinyGate -- handles scope-locking for a CTinyLock object

class CTinyGate
{
private:
    CTinyLock   *m_pLock;
    BOOL        m_fLocked;

public:
    CTinyGate (CTinyLock *p) : m_pLock(p) { m_fLocked = m_pLock->Acquire (); }
    ~CTinyGate () { Release (); }
    void    Detach () { m_fLocked = FALSE; }
    void    Release () { if (m_fLocked) { m_pLock->Release (); m_fLocked = FALSE; } }
    void    Acquire () { ASSERT (!m_fLocked); m_fLocked = m_pLock->Acquire (); }
};

////////////////////////////////////////////////////////////////////////////////
// CTinyTryGate -- handles scope-lock attempts for a CTinyLock object.

class CTinyTryGate
{
private:
    CTinyLock   *m_pLock;
    BOOL        m_fLocked;

public:
    CTinyTryGate (CTinyLock *p) : m_pLock(p) { m_fLocked = m_pLock->AttemptAcquire (); }
    ~CTinyTryGate () { Release (); }
    void    Detach () { m_fLocked = FALSE; }
    void    Release () { if (m_fLocked) { m_pLock->Release (); m_fLocked = FALSE; } }
    BOOL    AttemptAcquire () { ASSERT (!m_fLocked); m_fLocked = m_pLock->AttemptAcquire (); return m_fLocked; }
    BOOL    Acquired () { return m_fLocked; }
};

#endif  // _LOCKS_H_
