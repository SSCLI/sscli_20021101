/*++


 Copyright (c) 2002 Microsoft Corporation.  All rights reserved.

 The use and distribution terms for this software are contained in the file
 named license.txt, which can be found in the root of this distribution.
 By using this software in any fashion, you are agreeing to be bound by the
 terms of this license.

 You must not remove this notice, or any other, from this software.


Module Name:

    heap.c

Abstract:

    Implementation of heap memory management functions.

Revision History:

--*/

#include "pal/palinternal.h"
#include "pal/dbgmsg.h"
#include <errno.h>


SET_DEFAULT_DEBUG_CHANNEL(MEM);


#define HEAP_MAGIC 0xEAFDC9BB
#define DUMMY_HEAP 0x01020304


/*++
Function:
  RtlMoveMemory

See MSDN doc.
--*/
VOID
PALAPI
RtlMoveMemory(
          IN PVOID Destination,
          IN CONST VOID *Source,
          IN SIZE_T Length)
{
    ENTRY("RtlMoveMemory(Destination:%p, Source:%p, Length:%d)\n", 
          Destination, Source, Length);
    
    memmove(Destination, Source, Length);
    
    LOGEXIT("RtlMoveMemory returning\n");
}

/*++
Function:
  GetProcessHeap

See MSDN doc.
--*/
HANDLE
PALAPI
GetProcessHeap(
	       VOID)
{
    HANDLE ret;

    ENTRY("GetProcessHeap()\n");

    ret = (HANDLE) DUMMY_HEAP;
  
    LOGEXIT("GetProcessHeap returning HANDLE %p\n", ret);
    return ret;
}

/*++
Function:
  HeapAlloc

Abstract
  Implemented as wrapper over malloc

See MSDN doc.
--*/
LPVOID
PALAPI
HeapAlloc(
	  IN HANDLE hHeap,
	  IN DWORD dwFlags,
	  IN SIZE_T dwBytes)
{
    BYTE *pMem;
    int nSize = 0;
    #if defined(_DEBUG)
        nSize =  max(sizeof(void*),sizeof(double));
    #endif

    ENTRY("HeapAlloc (hHeap=%p, dwFlags=%#x, dwBytes=%u)\n",
          hHeap, dwFlags, dwBytes);

    if (hHeap != (HANDLE) DUMMY_HEAP)
    {
        ERROR("Invalid heap handle\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        LOGEXIT("HeapAlloc returning NULL\n");
        return NULL;
    }

    if ((dwFlags != 0) && (dwFlags != HEAP_ZERO_MEMORY))
    {
        ASSERT("Invalid parameter dwFlags=%#x\n", dwFlags);
        SetLastError(ERROR_INVALID_PARAMETER);
        LOGEXIT("HeapAlloc returning NULL\n");
        return NULL;
    }

    pMem = (BYTE *) malloc(dwBytes+nSize);
    if (pMem == NULL)
    {
        ERROR("Not enough memory\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        LOGEXIT("HeapAlloc returning NULL\n");
        return NULL;
    }

    /* use a magic number, to know it has been allocated with HeapAlloc
       when doing HeapFree */
    /*we are doing this only for the debug build*/
    #if defined(_DEBUG)
        *((DWORD *) pMem) = HEAP_MAGIC;
    #endif

    /*If the Heap Zero memory flag is set initialize to zero*/
    if (dwFlags == HEAP_ZERO_MEMORY)
    {
        memset(pMem+nSize, 0, dwBytes);
    }
    LOGEXIT("HeapAlloc returning LPVOID %p\n", pMem+nSize);
    return (pMem + nSize);
}


/*++
Function:
  HeapFree

Abstract
  Implemented as wrapper over free

See MSDN doc.
--*/
BOOL
PALAPI
HeapFree(
	 IN HANDLE hHeap,
	 IN DWORD dwFlags,
	 IN LPVOID lpMem)
{
    int nSize =0;
    BOOL bRetVal = FALSE;
    #if defined(_DEBUG)
        nSize =  max(sizeof(void*),sizeof(double));
    #endif

    ENTRY("HeapFree (hHeap=%p, dwFalgs = %#x lpMem=%p)\n", 
          hHeap, dwFlags, lpMem);

    if (hHeap != (HANDLE) DUMMY_HEAP)
    {
        ERROR("Invalid heap handle\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    if (dwFlags != 0)
    {
        ASSERT("Invalid parameter dwFlags=%#x\n", dwFlags);
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    if ( !lpMem )
    {
        bRetVal = TRUE;
        goto done;
    }
    /*nSize + nMemAlloc is the size of Magic Number plus
     *size of the int to store value of Memory allocated */
    lpMem -= (nSize);
    
    /* check if it is valid pointer */
    if (IsBadReadPtr(lpMem, 1))
    {
        ERROR("Invalid Pointer (%p)\n", lpMem+nSize);
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    /* check if the memory has been allocated by HeapAlloc */
    /*we are doing this only for the debug build*/
    #if defined(_DEBUG)
        if (*((DWORD *) lpMem) != HEAP_MAGIC)
        {
            ERROR("Pointer hasn't been allocated with HeapAlloc (%p)\n", lpMem+nSize);
            SetLastError(ERROR_INVALID_PARAMETER);
            goto done;
        }
        *((DWORD *) lpMem) = 0;
    #endif

    bRetVal = TRUE;
    free (lpMem);

done:
    LOGEXIT( "HeapFree returning BOOL %d\n", bRetVal );
    return bRetVal;
}




/*++
Function:
  HeapReAlloc

Abstract
  Implemented as wrapper over realloc

See MSDN doc.
--*/
LPVOID
PALAPI
HeapReAlloc(
	  IN HANDLE hHeap,
	  IN DWORD dwFlags,
	  IN LPVOID lpmem,
	  IN SIZE_T dwBytes)
{
    BYTE *pMem = NULL;
    int nSize = 0;
    #if defined(_DEBUG)
        nSize =  max(sizeof(void*),sizeof(double));
    #endif

    ENTRY("HeapReAlloc (hHeap=%p, dwFlags=%#x, lpmem=%p, dwBytes=%u)\n",
          hHeap, dwFlags, lpmem, dwBytes);

    if (hHeap != GetProcessHeap())
    {
        ASSERT("Invalid heap handle\n");
        SetLastError(ERROR_INVALID_HANDLE);
        goto done;
    }

    if ((dwFlags != 0))
    {
        ASSERT("Invalid parameter dwFlags=%#x\n", dwFlags);
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    if (lpmem == NULL)
    {
        WARN("NULL memory pointer to realloc.Do not do anything\n");
        /* set LastError back to zero. this appears to be an undocumented
        behavior in Windows, in doesn't cost much to match it */
        SetLastError(0);
        goto done;
    }

   /*nSize + nMemAlloc is the size of Magic Number plus
     *size of the int to store value of Memory allocated */
   lpmem -= (nSize);

    /* check if it is valid pointer */
    if (IsBadReadPtr(lpmem, 1))
    {
        ERROR("Invalid Pointer (%p)\n", lpmem+nSize);
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    /* check if the memory has been allocated by HeapAlloc */
    /*we are doing this only for the debug build*/
    #if defined(_DEBUG)
        if (*((DWORD *) lpmem) != HEAP_MAGIC)
        {
            ERROR("Pointer hasn't been allocated with HeapAlloc (%p)\n", lpmem+nSize);
            SetLastError(ERROR_INVALID_PARAMETER);
            goto done;
        }
    #endif

   pMem = (BYTE *) realloc(lpmem,dwBytes+nSize);

   /*make sure the pointer is null and it is not because of
    * any reason other than ENOMEM*/
   if ((pMem == NULL) && (errno != 0))
   {
       ERROR("Not enough memory\n");
       SetLastError(ERROR_NOT_ENOUGH_MEMORY);
       goto done;
   }

     /* use a magic number, to know it has been allocated with HeapAlloc
       when doing HeapFree */
    /*we are doing this only for the debug build*/
    #if defined(_DEBUG)
        *((DWORD *) pMem) = HEAP_MAGIC;
    #endif

done:
    LOGEXIT("HeapReAlloc returns LPVOID %p\n", pMem ? (pMem+nSize) : pMem);
    return pMem ? (pMem+nSize) : pMem;
}

