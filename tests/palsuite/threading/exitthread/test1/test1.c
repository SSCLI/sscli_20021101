/*============================================================
**
** Source: test1.c 
**
** Purpose: Test for ExitThread.  Create a thread and then call
** exit thread within the threading function.  Ensure that it exits
** immediatly.
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

#include <palsuite.h>

DWORD dwExitThreadTestParameter = 0;

DWORD PALAPI ExitThreadTestThread( LPVOID lpParameter)
{
    DWORD dwRet = 0;

    /* Save parameter for test */
    dwExitThreadTestParameter = (DWORD)lpParameter;

    /* Call the ExitThread function */
    ExitThread(dwRet);

    /* If we didn't exit, get caught in this loop.  But, the 
       program will exit.
    */
    while (!dwRet)
    {
        Fail("ERROR: Entered an infinite loop because ExitThread "
               "failed to exit from the thread.  Forcing exit from "
               "the test now.");
    }

    return dwRet;
}

BOOL ExitThreadTest()
{
    BOOL bRet = FALSE;
    DWORD dwRet = 0;

    LPSECURITY_ATTRIBUTES lpThreadAttributes = NULL;
    DWORD dwStackSize = 0; 
    LPTHREAD_START_ROUTINE lpStartAddress =  &ExitThreadTestThread;
    LPVOID lpParameter = lpStartAddress;
    DWORD dwCreationFlags = 0;  //run immediately
    DWORD dwThreadId = 0;

    HANDLE hThread = 0;

    dwExitThreadTestParameter = 0;

    /* Create a Thread.  We'll need this to test that we're able
       to exit the thread.  
    */
    hThread = CreateThread( lpThreadAttributes, 
                            dwStackSize, lpStartAddress, lpParameter, 
                            dwCreationFlags, &dwThreadId ); 
        
    if (hThread != INVALID_HANDLE_VALUE)
    {
        dwRet = WaitForSingleObject(hThread,INFINITE);
            
        if (dwRet != WAIT_OBJECT_0)
        {
            Trace("ExitThreadTest:WaitForSingleObject failed "
                   "(%x)\n",GetLastError());
        }
        else
        {
            /* Check to ensure that the parameter set in the Thread
               function is correct.
            */
            if (dwExitThreadTestParameter != (DWORD)lpParameter)
            {
                Trace("ERROR: The paramater passed should have been "
                       "%d but turned up as %d.",
                       dwExitThreadTestParameter, lpParameter);
            }
            else
            {
                bRet = TRUE;
            }
        }
    }
    else
    {
        Trace("ExitThreadTest:CreateThread failed (%x)\n",GetLastError());
    }

    return bRet; 
}

int __cdecl main(int argc, char **argv)
{
    if(0 != (PAL_Initialize(argc, argv)))
    {
        return ( FAIL );
    }

    if(!ExitThreadTest())
    {
        Fail ("Test failed\n");
    }
    
    PAL_Terminate();
    return ( PASS );
}
