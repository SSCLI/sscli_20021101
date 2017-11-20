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
/******************************************************************************
	FILE : UTSEM.CPP

	Purpose: Part of the utilities library for the VIPER project

	Abstract : Implements the UTSemReadWrite class.
-------------------------------------------------------------------------------
Revision History:

*******************************************************************************/
#include "stdafx.h"


#include <utsem.h>



/******************************************************************************
Function : UTSemExclusive::UTSemExclusive

Abstract: Constructor.
******************************************************************************/
CSemExclusive::CSemExclusive (DWORD ulcSpinCount)
{
    ::InitializeCriticalSection (&m_csx);

	DEBUG_STMT(m_iLocks = 0);
}


/******************************************************************************
Definitions of the bit fields in UTSemReadWrite::m_dwFlag:

Warning: The code assume that READER_MASK is in the low-order bits of the DWORD.
******************************************************************************/

const ULONG READERS_MASK      = 0x000003FF;	// field that counts number of readers
const ULONG READERS_INCR      = 0x00000001;	// amount to add to increment number of readers

// The following field is 2 bits long to make it easier to catch errors.
// (If the number of writers ever exceeds 1, we've got problems.)
const ULONG WRITERS_MASK      = 0x00000C00;	// field that counts number of writers
const ULONG WRITERS_INCR      = 0x00000400;	// amount to add to increment number of writers

const ULONG READWAITERS_MASK  = 0x003FF000;	// field that counts number of threads waiting to read
const ULONG READWAITERS_INCR  = 0x00001000;	// amount to add to increment number of read waiters

const ULONG WRITEWAITERS_MASK = 0xFFC00000;	// field that counts number of threads waiting to write
const ULONG WRITEWAITERS_INCR = 0x00400000;	// amoun to add to increment number of write waiters

/******************************************************************************
Function : UTSemReadWrite::UTSemReadWrite

Abstract: Constructor.
******************************************************************************/
UTSemReadWrite::UTSemReadWrite (DWORD ulcSpinCount,
		LPCSTR szSemaphoreName, LPCSTR szEventName)
{
	static BOOL fInitialized = FALSE;
	static DWORD maskMultiProcessor;	// 0 => uniprocessor, all 1 bits => multiprocessor

	if (!fInitialized)
	{
		SYSTEM_INFO SysInfo;

		GetSystemInfo (&SysInfo);
		if (SysInfo.dwNumberOfProcessors > 1)
			maskMultiProcessor = 0xFFFFFFFF;
		else
			maskMultiProcessor = 0;

		fInitialized = TRUE;
	}


	m_ulcSpinCount = ulcSpinCount & maskMultiProcessor;
	m_dwFlag = 0;
	m_hReadWaiterSemaphore = NULL;
	m_hWriteWaiterEvent = NULL;
	m_szSemaphoreName = szSemaphoreName;
	m_szEventName = szEventName;
}


/******************************************************************************
Function : UTSemReadWrite::~UTSemReadWrite

Abstract: Destructor
******************************************************************************/
UTSemReadWrite::~UTSemReadWrite ()
{
	_ASSERTE (m_dwFlag == 0 && "Destroying a UTSemReadWrite while in use");

	if (m_hReadWaiterSemaphore != NULL)
		CloseHandle (m_hReadWaiterSemaphore);

	if (m_hWriteWaiterEvent != NULL)
		CloseHandle (m_hWriteWaiterEvent);
}


/******************************************************************************
Function : UTSemReadWrite::LockRead

Abstract: Obtain a shared lock
******************************************************************************/
void UTSemReadWrite::LockRead ()
{
	ULONG dwFlag;
	ULONG ulcLoopCount = 0;


	for (;;)
	{
		dwFlag = m_dwFlag;

		if (dwFlag < READERS_MASK)
		{
			if (dwFlag == (ULONG) VipInterlockedCompareExchange (&m_dwFlag, dwFlag + READERS_INCR, dwFlag))
				break;
		}

		else if ((dwFlag & READERS_MASK) == READERS_MASK)
			Sleep(1000);

		else if ((dwFlag & READWAITERS_MASK) == READWAITERS_MASK)
			Sleep(1000);

		else if (ulcLoopCount++ < m_ulcSpinCount)
			/* nothing */ ;

		else
		{
			if (dwFlag == (ULONG) VipInterlockedCompareExchange (&m_dwFlag, dwFlag + READWAITERS_INCR, dwFlag))
			{
				WaitForSingleObject (GetReadWaiterSemaphore(), INFINITE);
				break;
			}
		}
	}

	_ASSERTE ((m_dwFlag & READERS_MASK) != 0 && "reader count is zero after acquiring read lock");
	_ASSERTE ((m_dwFlag & WRITERS_MASK) == 0 && "writer count is nonzero after acquiring write lock");
}



/******************************************************************************
Function : UTSemReadWrite::LockWrite

Abstract: Obtain an exclusive lock
******************************************************************************/
void UTSemReadWrite::LockWrite ()
{
	ULONG dwFlag;
	ULONG ulcLoopCount = 0;


	for (;;)
	{
		dwFlag = m_dwFlag;

		if (dwFlag == 0)
		{
			if (dwFlag == (ULONG) VipInterlockedCompareExchange (&m_dwFlag, WRITERS_INCR, dwFlag))
				break;
		}

		else if ((dwFlag & WRITEWAITERS_MASK) == WRITEWAITERS_MASK)
			Sleep(1000);

		else if (ulcLoopCount++ < m_ulcSpinCount)
			/*nothing*/ ;

		else
		{
			if (dwFlag == (ULONG) VipInterlockedCompareExchange (&m_dwFlag, dwFlag + WRITEWAITERS_INCR, dwFlag))
			{
				WaitForSingleObject (GetWriteWaiterEvent(), INFINITE);
				break;
			}
		}

	}

	_ASSERTE ((m_dwFlag & READERS_MASK) == 0 && "reader count is nonzero after acquiring write lock");
	_ASSERTE ((m_dwFlag & WRITERS_MASK) == WRITERS_INCR && "writer count is not 1 after acquiring write lock");
}



/******************************************************************************
Function : UTSemReadWrite::UnlockRead

Abstract: Release a shared lock
******************************************************************************/
void UTSemReadWrite::UnlockRead ()
{
	ULONG dwFlag;


	_ASSERTE ((m_dwFlag & READERS_MASK) != 0 && "reader count is zero before releasing read lock");
	_ASSERTE ((m_dwFlag & WRITERS_MASK) == 0 && "writer count is nonzero before releasing read lock");

	for (;;)
	{
		dwFlag = m_dwFlag;

		if (dwFlag == READERS_INCR)
		{		// we're the last reader, and nobody is waiting
			if (dwFlag == (ULONG) VipInterlockedCompareExchange (&m_dwFlag, 0, dwFlag))
				break;
		}

		else if ((dwFlag & READERS_MASK) > READERS_INCR)
		{		// we're not the last reader
			if (dwFlag == (ULONG) VipInterlockedCompareExchange (&m_dwFlag, dwFlag - READERS_INCR, dwFlag))
				break;
		}

		else
		{
			// here, there should be exactly 1 reader (us), and at least one waiting writer.
			_ASSERTE ((dwFlag & READERS_MASK) == READERS_INCR && "UnlockRead consistency error 1");
			_ASSERTE ((dwFlag & WRITEWAITERS_MASK) != 0 && "UnlockRead consistency error 2");

			// one or more writers is waiting, do one of them next
			// (remove a reader (us), remove a write waiter, add a writer
			if (dwFlag == (ULONG) VipInterlockedCompareExchange (&m_dwFlag,
					dwFlag - READERS_INCR - WRITEWAITERS_INCR + WRITERS_INCR, dwFlag))
			{
				SetEvent (GetWriteWaiterEvent());
				break;
			}
		}
	}
}


/******************************************************************************
Function : UTSemReadWrite::UnlockWrite

Abstract: Release an exclusive lock
******************************************************************************/
void UTSemReadWrite::UnlockWrite ()
{
	ULONG dwFlag;
	ULONG count;


	_ASSERTE ((m_dwFlag & READERS_MASK) == 0 && "reader count is nonzero before releasing write lock");
	_ASSERTE ((m_dwFlag & WRITERS_MASK) == WRITERS_INCR && "writer count is not 1 before releasing write lock");


	for (;;)
	{
		dwFlag = m_dwFlag;

		if (dwFlag == WRITERS_INCR)
		{		// nobody is waiting
			if (dwFlag == (ULONG) VipInterlockedCompareExchange (&m_dwFlag, 0, dwFlag))
				break;
		}

		else if ((dwFlag & READWAITERS_MASK) != 0)
		{		// one or more readers are waiting, do them all next
			count = (dwFlag & READWAITERS_MASK) / READWAITERS_INCR;
			// remove a writer (us), remove all read waiters, turn them into readers
			if (dwFlag == (ULONG) VipInterlockedCompareExchange (&m_dwFlag,
					dwFlag - WRITERS_INCR - count * READWAITERS_INCR + count * READERS_INCR, dwFlag))
			{
				ReleaseSemaphore (GetReadWaiterSemaphore(), count, NULL);
				break;
			}
		}

		else
		{		// one or more writers is waiting, do one of them next
			_ASSERTE ((dwFlag & WRITEWAITERS_MASK) != 0 && "UnlockWrite consistency error");
				// (remove a writer (us), remove a write waiter, add a writer
			if (dwFlag == (ULONG) VipInterlockedCompareExchange (&m_dwFlag, dwFlag - WRITEWAITERS_INCR, dwFlag))
			{
				SetEvent (GetWriteWaiterEvent());
				break;
			}
		}
	}
}

/******************************************************************************
Function : UTSemReadWrite::GetReadWaiterSemaphore

Abstract: Return the semaphore to use for read waiters
******************************************************************************/
HANDLE UTSemReadWrite::GetReadWaiterSemaphore()
{
	HANDLE h;

	if (m_hReadWaiterSemaphore == NULL)
	{
		h = CreateSemaphoreA (NULL, 0, MAXLONG, m_szSemaphoreName);
		_ASSERTE (h != NULL && "GetReadWaiterSemaphore can't CreateSemaphore");
		if (NULL != VipInterlockedCompareExchange (&m_hReadWaiterSemaphore, h, NULL))
			CloseHandle (h);
	}

	return m_hReadWaiterSemaphore;
}


/******************************************************************************
Function : UTSemReadWrite::GetWriteWaiterEvent

Abstract: Return the semaphore to use for write waiters
******************************************************************************/
HANDLE UTSemReadWrite::GetWriteWaiterEvent()
{
	HANDLE h;

	if (m_hWriteWaiterEvent == NULL)
	{
		h = CreateEventA (NULL, FALSE, FALSE, m_szEventName);	// auto-reset event
		_ASSERTE (h != NULL && "GetWriteWaiterEvent can't CreateEvent");
		if (NULL != VipInterlockedCompareExchange (&m_hWriteWaiterEvent, h, NULL))
			CloseHandle (h);
	}

	return m_hWriteWaiterEvent;
}
