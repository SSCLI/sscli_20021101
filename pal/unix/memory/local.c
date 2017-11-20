/*++


 Copyright (c) 2002 Microsoft Corporation.  All rights reserved.

 The use and distribution terms for this software are contained in the file
 named license.txt, which can be found in the root of this distribution.
 By using this software in any fashion, you are agreeing to be bound by the
 terms of this license.

 You must not remove this notice, or any other, from this software.


Module Name:

    local.c

Abstract:

    Implementation of local memory management functions.

Revision History:

--*/

#include "pal/palinternal.h"
#include "pal/dbgmsg.h"


SET_DEFAULT_DEBUG_CHANNEL(MEM);



/*++
Function:
  LocalAlloc

See MSDN doc.
--*/
HLOCAL
PALAPI
LocalAlloc(
	   IN UINT uFlags,
	   IN SIZE_T uBytes)
{
    LPVOID lpRetVal = NULL;
    ENTRY("LocalAlloc (uFlags=%#x, uBytes=%u)\n", uFlags, uBytes);

    if(uFlags !=0)
    {
        ASSERT("Invalid parameter uFlags=0x%x\n", uFlags);
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    lpRetVal = HeapAlloc( GetProcessHeap(), uFlags, uBytes );

done:
    LOGEXIT( "LocalAlloc returning %p.\n", lpRetVal );
    return (HLOCAL) lpRetVal;
}


/*++
Function:
  LocalFree

See MSDN doc.
--*/
HLOCAL
PALAPI
LocalFree(
	  IN HLOCAL hMem)
{
    BOOL bRetVal = FALSE;
    ENTRY("LocalFree (hmem=%p)\n", hMem);

    if ( hMem )
    {
        bRetVal = HeapFree( GetProcessHeap(), 0, hMem );
    }
    else
    {
        bRetVal = TRUE;
    }
    
    LOGEXIT( "LocalFree returning %p.\n", bRetVal == TRUE ? (HLOCAL)NULL : hMem );
    return bRetVal == TRUE ? (HLOCAL)NULL : hMem;
}
