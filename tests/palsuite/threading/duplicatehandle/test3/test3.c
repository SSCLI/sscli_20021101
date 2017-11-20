/*=====================================================================
**
** Source:    test3.c (DuplicateHandle)
**
** Purpose:   Tests the PAL implementation of the DuplicateHandle function.
**            This will test duplication of an OpenEvent handle. Test an 
**            event in a signaled state to wait, and then set the duplicate
**            to nonsignaled state and perform the wait again. The wait on
**            the event should fail. Test the duplication of closed and NULL
**            events, these should fail.  
**
** 
**  Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
** 
**  The use and distribution terms for this software are contained in the file
**  named license.txt, which can be found in the root of this distribution.
**  By using this software in any fashion, you are agreeing to be bound by the
**  terms of this license.
** 
**  You must not remove this notice, or any other, from this software.
** 
**
**===================================================================*/
#include <palsuite.h>

int __cdecl main(int argc, char **argv)
{
    HANDLE  hCreateEvent;
    HANDLE  hOpenEvent;
    HANDLE  hDupEvent;
    WCHAR   lpEventName[]={'E','v','e','n','t','\0'};

    /*Initialize the PAL.*/
    if ((PAL_Initialize(argc,argv)) != 0)
    {
        return (FAIL);
    }
    
    /*Create an Event, and set it in the signaled state.*/
    hCreateEvent = CreateEventW(0,
                                TRUE, 
                                TRUE, 
                                lpEventName);
    if (hCreateEvent == NULL)
    {
        Fail("ERROR: %u :unable to create event %s\n", 
              GetLastError(),
              lpEventName);
    }

    /*Open another handle to hCreateHandle with OpenEvent*/
    hOpenEvent = OpenEventW(EVENT_ALL_ACCESS,
                            TRUE,
                            lpEventName);
    if (hOpenEvent == NULL)
    {
        Trace("ERROR: %u :unable to create handle with OpenEvent to %s\n",
             GetLastError(),
             lpEventName);
        CloseHandle(hCreateEvent);
        Fail("");
    }

    /*Create a duplicate Event handle*/
    if (!(DuplicateHandle(GetCurrentProcess(), 
                          hOpenEvent,
                          GetCurrentProcess(),
                          &hDupEvent,
                          GENERIC_READ|GENERIC_WRITE,
                          FALSE, 
                          DUPLICATE_SAME_ACCESS)))
    {
        Trace("ERROR: %u :Fail to create the duplicate handle"
                " to hCreateEvent=0x%lx\n",
                GetLastError(),
                hCreateEvent);
        CloseHandle(hCreateEvent);
        CloseHandle(hOpenEvent);
        Fail("");
    }

    /*Perform wait on Event that is in signaled state*/
    if ((WaitForSingleObject(hOpenEvent, 1000)) != WAIT_OBJECT_0)
    {
        Trace("ERROR: %u :WaitForSignalObject on hOpenEvent=0x%lx set to "
                " signaled state failed\n",
                GetLastError(),
                hOpenEvent);
        CloseHandle(hCreateEvent);
        CloseHandle(hOpenEvent);
        CloseHandle(hDupEvent);
        Fail("");
    }

    /*Set the Duplicate Event handle to nonsignaled state*/
    if ((ResetEvent(hDupEvent)) == 0)
    {
        Trace("ERROR: %u: unable to reset hDupEvent=0x%lx\n", 
             GetLastError(),
             hDupEvent);
        CloseHandle(hCreateEvent);
        CloseHandle(hOpenEvent);
        CloseHandle(hDupEvent);
        Fail("");
    }

    /*Perform wait on Event that is in signaled state*/
    if ((WaitForSingleObject(hOpenEvent, 1000)) == WAIT_OBJECT_0)
    {
        Trace("ERROR: %u :WaitForSignalObject succeeded on hOpenEvent=0x%lx "
                " when Duplicate hDupEvent=0x%lx set to nonsignaled state"
                " succeeded\n",
                GetLastError(),
                hOpenEvent,
                hDupEvent);
        CloseHandle(hCreateEvent);
        CloseHandle(hOpenEvent);
        CloseHandle(hDupEvent);
        Fail("");
    }

    /*Close handles the events*/
    CloseHandle(hCreateEvent);
    CloseHandle(hOpenEvent);
    CloseHandle(hDupEvent);

    PAL_Terminate();
    return (PASS);
}
