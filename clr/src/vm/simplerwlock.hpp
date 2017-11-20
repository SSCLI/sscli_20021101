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
#ifndef _SimpleRWLock_hpp_
#define _SimpleRWLock_hpp_

class SimpleRWLock
{
protected:
    SimpleRWLock (BOOL bRequirePreemptiveGC)
        : m_bRequirePreemptiveGC (bRequirePreemptiveGC)
    {
        m_lock.Init(LOCK_REFLECTCACHE);	
        m_RWLock = 0;
        m_dwRWLockVersion = 0;
    }
    
	// Lock and Unlock, use a very fast lock like a spin lock
    void LOCK()
    {
		m_lock.GetLock();
    }

    void UNLOCK()
    {
		m_lock.FreeLock();
    }

    // Acquire the reader lock.
    BOOL TryEnterRead();
    void EnterRead();

    // Acquire the writer lock.
    BOOL TryEnterWrite();
    void EnterWrite();

    // Leave the reader lock.
    void LeaveRead()
    {
        _ASSERTE(m_RWLock > 0);
        InterlockedDecrement(&m_RWLock);
        DECTHREADLOCKCOUNT();
    }

    // Leave the writer lock.
    void LeaveWrite()
    {
        _ASSERTE(m_RWLock == -1);
        m_dwRWLockVersion++;
        m_RWLock = 0;
        DECTHREADLOCKCOUNT();
    }

#ifdef _DEBUG
    BOOL LockTaken ()
    {
        return m_RWLock != 0;
    }

    BOOL IsReaderLock ()
    {
        return m_RWLock > 0;
    }

    BOOL IsWriterLock ()
    {
        return m_RWLock < 0;
    }
    
#endif
    
private:
	// spin lock for fast synchronization	
	SpinLock            m_lock;

    // lock used for R/W synchronization
    LONG                m_RWLock;     

    // A version number associated with the reader writer lock.
    LONG                m_dwRWLockVersion;

    // Does this lock require to be taken in PreemptiveGC mode?
    const BOOL          m_bRequirePreemptiveGC;
};

#endif
