/*============================================================
**
** Source : test.c
**
** Purpose: Test for FormatMessageW() function
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
**=========================================================*/


#define UNICODE
#include <palsuite.h>


int __cdecl main(int argc, char *argv[]) {


    LPWSTR OutBuffer;
    int ReturnResult;

    /*
     * Initialize the PAL and return FAILURE if this fails
     */

    if(0 != (PAL_Initialize(argc, argv)))
    {
        return FAIL;
    }


    /* This is testing the use of FROM_SYSTEM.  We can't check to ensure
       the error message it extracts is correct, only that it does place some
       information into the buffer when it is called.
    */
  
    ReturnResult = FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_ALLOCATE_BUFFER,  /* source and processing options */
        NULL,                            /* message source */
        GetLastError(),                  /* message identifier */
        0,                               /* language identifier */
        (LPWSTR)&OutBuffer,              /* message buffer */
        0,                               /* maximum size of message buffer */
        NULL                            /* array of message inserts */
        );
  
    if(ReturnResult == 0) 
    {
        Fail("ERROR: The return value was 0, which indicates failure. The "
             "function failed when trying to Format a FROM_SYSTEM message.");
    }
  
    if(wcslen(OutBuffer) <= 0) 
    {
        Fail("ERROR: There are no characters in the buffer, and when the "
             "FORMAT_MESSAGE_FROM_SYSTEM flag is used with GetLastError(), "
             "something should be put into the buffer.");
    }
  
    LocalFree(OutBuffer);
  
    PAL_Terminate();
    return PASS;
 
}


