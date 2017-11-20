/*++


 Copyright (c) 2002 Microsoft Corporation.  All rights reserved.

 The use and distribution terms for this software are contained in the file
 named license.txt, which can be found in the root of this distribution.
 By using this software in any fashion, you are agreeing to be bound by the
 terms of this license.

 You must not remove this notice, or any other, from this software.


Module Name:

    thread.c

Abstract:

    Implementation of the threads/process functions in the C runtime library
    that are Windows specific.

--*/

#include "pal/palinternal.h"

#include "pal/dbgmsg.h"

SET_DEFAULT_DEBUG_CHANNEL(CRT);

void
PAL_exit(int status)
{
    ENTRY ("exit(status=%d)\n", status);

    /* should also clean up any resources allocated by pal/cruntime, if any */
    ExitProcess(status);

    LOGEXIT ("exit returns void");
}

int
PAL_atexit(void (__cdecl *function)(void))
{
    int ret;
    
    ENTRY ("atexit(function=%p)\n", function);
    ret = atexit(function);
    LOGEXIT ("atexit returns int %d", ret);
    return ret;
}
