/*=============================================================
**
** Source:
**
** Purpose: Positive test the FreeLibrary API.
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
**============================================================*/
#include <palsuite.h>

/*Define platform specific information*/
#if WIN32
    char    LibraryName[] = "dlltest";
#elif defined(__APPLE__)
    char    LibraryName[] = "dlltest.dylib";
#else
    char    LibraryName[] = "dlltest.so";
#endif

BOOL  PALAPI TestDll(HMODULE, int);

int __cdecl main(int argc, char *argv[])
{
    HANDLE hLib;

    /* Initialize the PAL. */
    if ((PAL_Initialize(argc, argv)) != 0)
    {
        return (FAIL);
    }
    
    /*Load library (DLL). */
    hLib = LoadLibrary(LibraryName);
    if(hLib == NULL)
    {
        Fail("ERROR:%u:Unable to load library %s\n", 
             GetLastError(), 
             LibraryName);
    }

    /* Test access to DLL. */
    if(!TestDll(hLib, PASS))
        {
        Trace("ERROR: TestDll function returned FALSE "
             "expected TRUE\n.");
        FreeLibrary(hLib);
        Fail("");
    }

    /* Call the FreeLibrary API. */ 
    if (!FreeLibrary(hLib))
    {
        Fail("ERROR:%u: Unable to free library \"%s\"\n", 
             GetLastError(),
             LibraryName);
    }

    /* Test access to the free'd DLL. */
    if(!TestDll(hLib, FAIL))
    {
        Fail("ERROR: TestDll function returned FALSE "
            "expected TRUE\n.");
    }

    PAL_Terminate();
    return PASS;

}


BOOL PALAPI TestDll(HMODULE hLib, int testResult)
{
    int     RetVal;
#if WIN32
    char    FunctName[] = "_DllTest@0";
#else
    char    FunctName[] = "DllTest";
#endif
    FARPROC DllAddr;    

    /* Attempt to grab the proc address of the dll function.
     * This one should succeed.*/
    if(testResult == PASS)
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
    if(testResult == FAIL)
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
