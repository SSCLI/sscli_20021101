/*=====================================================================
** 
** Source:  test1.c (FreeLibraryAndExitThread)
**
** Purpose: Tests the PAL implementation of the FreeLibraryAndExitThread
**          function. FreeLibraryAndExitThread when run will exit the 
**          process that it is called within, therefore we create a
**          thread to run the API. Then test for the existance of the
**          thread and access to the library.
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

/*Define platform specific information*/
#if WIN32
    char    LibraryName[] = "dlltest";
#elif defined(__APPLE__)
    char    LibraryName[] = "dlltest.dylib";
#else
    char    LibraryName[] = "dlltest.so";
#endif

BOOL  PALAPI StartThreadTest();
DWORD PALAPI CreateTestThread(LPVOID);
BOOL  PALAPI TestDll(HMODULE, int);

int __cdecl main(int argc, char* argv[])
{
    /*Initialize the PAL*/
    if ((PAL_Initialize(argc, argv)) != 0)
    {
        return (FAIL);
    }

    if (!StartThreadTest())
    {
        Fail("ERROR: FreeLibraryAndExitThread test failed.\n");
    }

    /*Terminate the PAL*/
    PAL_Terminate();
    return PASS;

}


BOOL  PALAPI StartThreadTest()
{
    HMODULE hLib;
    HANDLE  hThread;  
    DWORD   dwThreadId;
    LPTHREAD_START_ROUTINE lpStartAddress =  &CreateTestThread;
    LPVOID lpParameter = lpStartAddress;

    /*Load library (DLL).*/
    hLib = LoadLibrary(LibraryName);
    if(hLib == NULL)
    {
        Trace("ERROR: Unable to load library %s\n", LibraryName);
        
        return (FALSE);
    }

    /*Start the test thread*/
    hThread = CreateThread(NULL, 
                            (DWORD)0,
                            lpParameter,
                            hLib,
                            (DWORD)NULL,
                            &dwThreadId);
    if(hThread == NULL)
    {
        Trace("ERROR:%u: Unable to create thread.\n",
                GetLastError());

        FreeLibrary(hLib);
        return (FALSE);
    }

    /*Wait on thread.*/
    if(((WaitForSingleObject(hThread, 100)) != WAIT_OBJECT_0) &&
        ((WaitForSingleObject(hThread, 100)) != WAIT_TIMEOUT))
    {
        Trace("ERROR:%u: hThread=0x%4.4lx not exited by "
            "FreeLibraryAndExitThread\n",
            GetLastError(),  
            hThread);

        FreeLibrary(hLib);
        CloseHandle(hThread);
        return (FALSE);
    }
            
    /*Test access to DLL.*/
    if(!TestDll(hLib, 0))
    {
        Trace("ERROR: TestDll function returned FALSE "
            "expected TRUE\n.");
        
        FreeLibrary(hLib);
        CloseHandle(hThread);
        return (FALSE);
    }

    /*Clean-up thread.*/
    CloseHandle(hThread);

    return (TRUE);
}

BOOL PALAPI TestDll(HMODULE hLib, int testResult)
{
    int     RetVal;
    char    FunctName[] = "DllTest";
    FARPROC DllAddr;    

    /* Attempt to grab the proc address of the dll function.
     * This one should succeed.*/
    if(testResult == 1)
    {
        DllAddr = GetProcAddress(hLib, FunctName);
        if(DllAddr == NULL)
        {
            Trace("ERROR: Unable to load function \"%s\" library \"%s\"\n", 
                    FunctName,
                    LibraryName);
            return (FALSE);
        }
        /* Run the function in the DLL, 
         * to ensure that the DLL was loaded properly.*/
        RetVal = DllAddr();
        if (RetVal != 1)
        {
            Trace("ERROR: Unable to receive correct information from DLL! "
                ":expected \"1\", returned \"%d\"\n",
                RetVal);
            return (FALSE);
        }
    }

    /* Attempt to grab the proc address of the dll function.
     * This one should fail.*/
    if(testResult == 0)
    {
        DllAddr = GetProcAddress(hLib, FunctName);
        if(DllAddr != NULL)
        {
            Trace("ERROR: Able to load function \"%s\" from free'd"
                " library \"%s\"\n", 
                FunctName, 
                LibraryName);
            return (FALSE);
        }
    }
    return (TRUE);
}

DWORD PALAPI CreateTestThread(LPVOID lpParam)
{
    /* Test access to DLL.*/
    TestDll(lpParam, 1);

    /*Free library and exit thread.*/
    FreeLibraryAndExitThread(lpParam, (DWORD)0);

    /* NOT REACHED */

    /*Infinite loop, we should not get here.*/
    while(1);

    return (DWORD)0;
}
