/*++


 Copyright (c) 2002 Microsoft Corporation.  All rights reserved.

 The use and distribution terms for this software are contained in the file
 named license.txt, which can be found in the root of this distribution.
 By using this software in any fashion, you are agreeing to be bound by the
 terms of this license.

 You must not remove this notice, or any other, from this software.


Module Name:

    localstorage.c

Abstract:

    Implementation of Thread local storage functions.

--*/

#include "pal/palinternal.h"

#include <pthread.h>

#include "pal/dbgmsg.h"
#include "pal/thread.h"
#include "process.h"
#include "pal/misc.h"


SET_DEFAULT_DEBUG_CHANNEL(THREAD);

/* This tracks the slots that are used for TlsAlloc. Its size in bits
   must be the same as TLS_SLOT_SIZE in pal/thread.h. Since this is
   static, it is initialized to 0, which is what we want. */
static unsigned __int64 sTlsSlotFields;

/*++
Function:
  TlsAlloc

See MSDN doc.
--*/
DWORD
PALAPI
TlsAlloc(
	 VOID)
{
    DWORD dwIndex;
    unsigned int i;
    THREADLIST *threadList;

    ENTRY("TlsAlloc()\n");

    /* Yes, this could be ever so slightly improved. It's not
       likely to be called enough to matter, though, so we won't
       optimize here until or unless we need to. */
    PROCProcessLock();
    for(i = 0; i < sizeof(sTlsSlotFields) * 8; i++)
    {
        if ((sTlsSlotFields & ((unsigned __int64) 1 << i)) == 0)
        {
            sTlsSlotFields |= ((unsigned __int64) 1 << i);
            break;
        }
    }
    if (i == sizeof(sTlsSlotFields) * 8)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        dwIndex = TLS_OUT_OF_INDEXES;
    }
    else
    {
        dwIndex = i;

        /* Initialize all threads' values to zero for this index. */
        for(threadList = pGThreadList; 
            threadList != NULL; threadList = threadList->next)
        {
            threadList->lpThread->tlsSlots[dwIndex] = 0;
        }
    }
    PROCProcessUnlock();

    LOGEXIT("TlsAlloc returns DWORD %u\n", dwIndex);
    return dwIndex;
}


/*++
Function:
  TlsGetValue

See MSDN doc.
--*/
LPVOID
PALAPI
TlsGetValue(
	    IN DWORD dwTlsIndex)
{
    THREAD *thread;

    if (dwTlsIndex == (DWORD) -1 || dwTlsIndex >= TLS_SLOT_SIZE)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    thread = PROCGetCurrentThreadObject();
    if (thread == NULL)
    {
        ASSERT("Unable to access the current thread\n");
        SetLastError(ERROR_INVALID_DATA);
        return 0;
    }

    /* From MSDN : "The TlsGetValue function calls SetLastError to clear a
       thread's last error when it succeeds." */
    thread->lastError = NO_ERROR;

    return thread->tlsSlots[dwTlsIndex];
}


/*++
Function:
  TlsSetValue

See MSDN doc.
--*/
BOOL
PALAPI
TlsSetValue(
	    IN DWORD dwTlsIndex,
	    IN LPVOID lpTlsValue)
{
    THREAD *thread;
    BOOL bRet = FALSE;

    ENTRY("TlsSetValue(dwTlsIndex=%u, lpTlsValue=%p)\n", dwTlsIndex, lpTlsValue);

    if (dwTlsIndex == (DWORD) -1 || dwTlsIndex >= TLS_SLOT_SIZE)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto EXIT;
    }

    thread = PROCGetCurrentThreadObject();
    if (thread == NULL)
    {
        ASSERT("Unable to access the current thread\n");
        goto EXIT;
    }

    thread->tlsSlots[dwTlsIndex] = lpTlsValue;
    bRet = TRUE;
EXIT:
    LOGEXIT("TlsSetValue returns BOOL %d\n", bRet);
    return bRet;
}


/*++
Function:
  TlsFree

See MSDN doc.
--*/
BOOL
PALAPI
TlsFree(
	IN DWORD dwTlsIndex)
{
    ENTRY("TlsFree(dwTlsIndex=%u)\n", dwTlsIndex);

    if (dwTlsIndex == (DWORD) -1 || dwTlsIndex >= TLS_SLOT_SIZE)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        LOGEXIT("TlsFree returns BOOL FALSE\n");
        return FALSE;
    }

    PROCProcessLock();
    sTlsSlotFields &= ~((unsigned __int64) 1 << dwTlsIndex);
    PROCProcessUnlock();

    LOGEXIT("TlsFree returns BOOL TRUE\n");
    return TRUE;
}


/* Internal function definitions **********************************************/

/*++
Function:
    TlsInternalGetValue

    Do the work of TlsGetValue, but without calling SetLastError(NO_ERROR)
    This is so that PAL functions can call it without changing LastError

Parameters :
    DWORD dwTlsIndex : TLS index to query
    LPVOID *pValue : pointer where the requested value will be placed

Return value :
    TRUE if function succeeds, FALSE otherwise.

--*/
BOOL TlsInternalGetValue(DWORD dwTlsIndex, LPVOID *pValue)
{
    THREAD *thread;

    if (dwTlsIndex == (DWORD) -1 || dwTlsIndex >= TLS_SLOT_SIZE)
    {
        ASSERT("Invalid index (%d)\n", dwTlsIndex);
        return FALSE;
    }

    thread = PROCGetCurrentThreadObject();
    if (thread == NULL)
    {
        ASSERT("Unable to access the current thread\n");
        return FALSE;
    }

    *pValue = thread->tlsSlots[dwTlsIndex];
    return TRUE;
}

