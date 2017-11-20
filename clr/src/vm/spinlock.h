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
//----------------------------------------------------------------------------
//	spinlock.h , defines the spin lock class and a profiler class
//  	
//----------------------------------------------------------------------------


//#ifndef _H_UTIL
//#error I am a part of util.hpp Please don't include me alone !
//#endif


#ifndef _H_SPINLOCK_
#define _H_SPINLOCK_

#include <stddef.h>

// Lock Types, used in profiling
//
enum LOCK_TYPE
{
	LOCK_STARTUP	 = 0,
	LOCK_THREAD_POOL  = 1,
	LOCK_PLUSWRAPPER_CACHE = 2,
	LOCK_COMWRAPPER_CACHE = 3,
	LOCK_HINTCACHE = 4,
    LOCK_COMIPLOOKUP = 5,
    LOCK_FCALL = 7,
	LOCK_COMCTXENTRYCACHE = 8,
    LOCK_REFLECTCACHE = 10,
	LOCK_TYPE_DEFAULT  = 11
};

//----------------------------------------------------------------------------
// class: Spinlock 
//
// PURPOSE:
//	 spinlock class that contains constructor and out of line spinloop.
//
//----------------------------------------------------------------------------
class SpinLock
{
protected:
    // m_lock has to be the fist data member in the class
    volatile LONG       m_lock;		// LONG used in interlocked exchange

#ifdef _DEBUG
    LOCK_TYPE           m_LockType;     // lock type to track statistics
    bool                m_fInitialized; // DEBUG check to verify initialized
    
    // Check for dead lock situation.
    bool                m_heldInSuspension; // may be held while the thread is
                                            // suspended.
    bool                m_enterInCoopGCMode;
    ULONG               m_ulReadyForSuspensionCount;
    DWORD               m_holdingThreadId;
#endif

public:
		//Init method, initialize lock and _DEBUG flags
	void Init(LOCK_TYPE type)
	{
		m_lock = 0;		

#ifdef _DEBUG
        m_LockType = type;
        m_heldInSuspension = false;
        m_enterInCoopGCMode = false;
        m_fInitialized = true; // DEBUG check for initialization
#endif
	}

	void GetLock () 	// Acquire lock, blocks, if unsuccessfull
	{
		_ASSERTE(m_fInitialized == true);

#ifdef _DEBUG
        dbg_PreEnterLock();
#endif

        LOCKCOUNTINCL("GetLock in spinlock.h");
		if (!GetLockNoWait ())
		{
			SpinToAcquire();
		}
#ifdef _DEBUG
        m_holdingThreadId = GetCurrentThreadId();
        dbg_EnterLock();
#endif
	}
	
	bool GetLockNoWait();	// Acquire lock, fail-fast 

	void FreeLock ();		// Release lock

	void SpinToAcquire (); // out of line call spins 

    //-----------------------------------------------------------------
    // Is the current thread the owner?
    //-----------------------------------------------------------------
#ifdef _DEBUG
    BOOL OwnedByCurrentThread()
    {
        return m_holdingThreadId == GetCurrentThreadId();
    }
#endif
    
private:
#ifdef _DEBUG
    void dbg_PreEnterLock();
    void dbg_EnterLock();
    void dbg_LeaveLock();
#endif

    // Helper functions to track lock count per thread.
    void IncThreadLockCount();
    void DecThreadLockCount();
};

//----------------------------------------------------------------------------
// SpinLock::GetLockNoWait   
// used interlocked exchange and fast lock acquire

inline bool
SpinLock::GetLockNoWait ()
{
	if (FastInterlockExchange (&m_lock, 1) == 0)
    {
        IncThreadLockCount();
        return 1;
    }
	return 0;
} // SpinLock::GetLockNoWait ()

//----------------------------------------------------------------------------
// SpinLock::FreeLock   
//  Release the spinlock
//
inline void
SpinLock::FreeLock ()
{
	_ASSERTE(m_fInitialized);

#ifdef _DEBUG
    _ASSERTE(OwnedByCurrentThread());
    m_holdingThreadId = (DWORD) -1;
    dbg_LeaveLock();
#endif

	// else uses interlocked exchange.
	//
	FastInterlockExchange (&m_lock, 0);

	LOCKCOUNTDECL("GetLock in spinlock.h");
    DecThreadLockCount();

} // SpinLock::FreeLock ()

__inline BOOL IsOwnerOfSpinLock (LPVOID lock)
{
#ifdef _DEBUG
    return ((SpinLock*)lock)->OwnedByCurrentThread();
#else
    // This function should not be called on free build.
    DebugBreak();
    return TRUE;
#endif
}

#ifdef _DEBUG
//----------------------------------------------------------------------------
// class SpinLockProfiler 
//  to track contention, useful for profiling
//
//----------------------------------------------------------------------------
class SpinLockProfiler
{
	// Pointer to spinlock names.
	//
	static ULONG	s_ulBackOffs;
	static ULONG	s_ulCollisons [LOCK_TYPE_DEFAULT + 1];
	static ULONG	s_ulSpins [LOCK_TYPE_DEFAULT + 1];

public:

	static void	InitStatics ()
	{
		s_ulBackOffs = 0;
		memset (s_ulCollisons, 0, sizeof (s_ulCollisons));
		memset (s_ulSpins, 0, sizeof (s_ulSpins));
	}

	static void	IncrementSpins (LOCK_TYPE type, ULONG value)
	{
        _ASSERTE(type <= LOCK_TYPE_DEFAULT);
		s_ulSpins [type] += value;
	}

	static void	IncrementCollisions (LOCK_TYPE type)
	{
		++s_ulCollisons [type];
	}

	static void IncrementBackoffs (ULONG value)
	{
		s_ulBackOffs += value;
	}

	static void DumpStatics()
	{
	}

};

#endif	// ifdef _DEBUG
#endif //  ifndef _H_SPINLOCK_
