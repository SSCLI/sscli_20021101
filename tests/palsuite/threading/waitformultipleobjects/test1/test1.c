/*============================================================
**
** Source: test1.c 
**
** Purpose: Test for WaitForMultipleObjects. Call the function
** on an array of 4 events, and ensure that it returns correct
** results when we do so.
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

/* Number of events in array */
#define MAX_EVENTS      4

BOOL WaitForMultipleObjectsTest()
{
    BOOL bRet = TRUE;
    DWORD dwRet = 0;

    DWORD i = 0, j = 0;

    LPSECURITY_ATTRIBUTES lpEventAttributes = NULL;
    BOOL bManualReset = TRUE; 
    BOOL bInitialState = TRUE;
    LPTSTR lpName[MAX_EVENTS];

    HANDLE hEvent[MAX_EVENTS];

    /* Run through this for loop and create 4 events */ 
    for (i = 0; i < MAX_EVENTS; i++)
    {
        lpName[i] = (TCHAR*)malloc(MAX_PATH);
        sprintf(lpName[i],"Event #%d",i);

        hEvent[i] = CreateEvent( lpEventAttributes, 
                                 bManualReset, bInitialState, lpName[i]);  

        if (hEvent[i] == INVALID_HANDLE_VALUE)
        {
            Trace("WaitForMultipleObjectsTest:CreateEvent "
                   "%s failed (%x)\n",lpName[i],GetLastError());
            bRet = FALSE;
            break;
        }
        
        /* Set the current event */
        bRet = SetEvent(hEvent[i]);

        if (!bRet)
        {
            Trace("WaitForMultipleObjectsTest:SetEvent %s "
                   "failed (%x)\n",lpName[i],GetLastError());
            bRet = FALSE;
            break;
        }
        
        /* Ensure that this returns the correct value */
        dwRet = WaitForSingleObject(hEvent[i],0);

        if (dwRet != WAIT_OBJECT_0)
        {
            Trace("WaitForMultipleObjectsTest:WaitForSingleObject "
                   "%s failed (%x)\n",lpName[i],GetLastError());
            bRet = FALSE;
            break;
        }

        /* Reset the event, and again ensure that the return value of
           WaitForSingle is correct.
        */
        bRet = ResetEvent(hEvent[i]);

        if (!bRet)
        {
            Trace("WaitForMultipleObjectsTest:ResetEvent %s "
                   "failed (%x)\n",lpName[i],GetLastError());
            bRet = FALSE;
            break;
        }
        
        dwRet = WaitForSingleObject(hEvent[i],0);

        if (dwRet != WAIT_TIMEOUT)
        {
            Trace("WaitForMultipleObjectsTest:WaitForSingleObject "
                   "%s failed (%x)\n",lpName[i],GetLastError());
            bRet = FALSE;
            break;
        }
    }
    
    /* 
     * If the first section of the test passed, move on to the
     * second. 
    */

    if (bRet)
    {
        BOOL bWaitAll = TRUE;
        DWORD nCount = MAX_EVENTS;
        CONST HANDLE *lpHandles = &hEvent[0]; 

        /* Call WaitForMultipleOjbects on all the events, the return
           should be WAIT_TIMEOUT
        */
        dwRet = WaitForMultipleObjects( nCount, 
                                        lpHandles, 
                                        bWaitAll, 
                                        0);

        if (dwRet != WAIT_TIMEOUT)
        {
            Trace("WaitForMultipleObjectsTest:WaitForMultipleObjects "
                   "%s failed (%x)\n",lpName[0],GetLastError());
        }
        else
        {
            /* Step through each event and one at a time, set the
               currect test, while reseting all the other tests
            */
            
            for (i = 0; i < MAX_EVENTS; i++)
            {
                for (j = 0; j < MAX_EVENTS; j++)
                {
                    if (j == i)
                    {
                        
                        bRet = SetEvent(hEvent[j]);
                        
                        if (!bRet)
                        {
                            Trace("WaitForMultipleObjectsTest:SetEvent "
                                   "%s failed (%x)\n",
                                   lpName[j],GetLastError());
                            break;
                        }
                    }
                    else
                    {
                        bRet = ResetEvent(hEvent[j]);
                        
                        if (!bRet)
                        {
                            Trace("WaitForMultipleObjectsTest:ResetEvent "
                                   "%s failed (%x)\n",
                                   lpName[j],GetLastError());
                        }
                    }
                }
                
                bWaitAll = FALSE;

                /* Check that WaitFor returns WAIT_OBJECT + i */ 
                dwRet = WaitForMultipleObjects( nCount, 
                                                lpHandles, bWaitAll, 0);
                
                if (dwRet != WAIT_OBJECT_0+i)
                {
                    Trace("WaitForMultipleObjectsTest:WaitForMultipleObjects"
                           " %s failed (%x)\n",lpName[0],GetLastError());
                    bRet = FALSE;
                    break;
                }
            }
        }
        
        for (i = 0; i < MAX_EVENTS; i++)
        {
            bRet = CloseHandle(hEvent[i]);
            
            if (!bRet)
            {
                Trace("WaitForMultipleObjectsTest:CloseHandle %s "
                       "failed (%x)\n",lpName[i],GetLastError());
            }
            
            free((void*)lpName[i]);
        }
    }
    
    return bRet;
}


int __cdecl main(int argc, char **argv)
{
    
    if(0 != (PAL_Initialize(argc, argv)))
    {
        return ( FAIL );
    }
    
    if(!WaitForMultipleObjectsTest())
    {
        Fail ("Test failed\n");
    }
    PAL_Terminate();
    return ( PASS );

}
