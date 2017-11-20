/*=============================================================================
**
** Source: test2.c (filemapping_memmgt\getprocaddress\test2)
**
** Purpose: This test tries to call GetProcAddress with
**          a NULL handle, with a NULL function name, with an empty 
**          function name, with an invalid name and with an 
**          invalid ordinal value.
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
**===========================================================================*/
#include <palsuite.h>

/**
 * main
 */
int __cdecl main(int argc, char *argv[])
{
    int err;
    HMODULE hModule;
    FARPROC procAddress;
    char lpModuleName[256]="";

    /* Initialize the PAL environment. */
    if(0 != PAL_Initialize(argc, argv))
    {
        return FAIL;
    }

#if WIN32
    sprintf(lpModuleName,"%s","testlib.dll");
#elif defined(__APPLE__)
    sprintf(lpModuleName,"%s","testlib.dylib");
#else
    sprintf(lpModuleName,"%s","testlib.so");
#endif

    /* load a module */
    hModule = LoadLibrary(lpModuleName);
    if(!hModule)
    {
        Fail("Unexpected error: "
             "LoadLibrary(%s) failed.\n",
             lpModuleName);
    }

    /*
     * Test 1
     *
     * Call GetProcAddress with a NULL handle
     */
    procAddress = GetProcAddress(NULL,"SimpleFunction");
    if(procAddress != NULL)
	{
        Trace("ERROR: GetProcAddress with a NULL handle "
              "returned a non-NULL value when it should have "
              "returned a NULL value with an error\n");

         /* Cleanup */
        err = FreeLibrary(hModule);
        if(0 == err)
	    {
            Fail("Unexpected error: Failed to FreeLibrary %s\n", 
                 lpModuleName);
	    }
        Fail("");
	}

    /**
     * Test 2
     *
     * Call GetProcAddress with a NULL function name
     */

    procAddress = GetProcAddress(hModule,NULL);
    if(procAddress != NULL)
	{
        Trace("ERROR: GetProcAddress with a NULL function name "
              "returned a non-NULL value when it should have "
              "returned a NULL value with an error\n");

         /* Cleanup */
        err = FreeLibrary(hModule);
        if(0 == err)
	    {
            Fail("Unexpected error: Failed to FreeLibrary %s\n", 
                 lpModuleName);
	    }
        Fail("");
	}

    /**
     * Test 3
     *
     * Call GetProcAddress with an empty function name string
     */

    procAddress = GetProcAddress(hModule,"");
    if(procAddress != NULL)
	{
        Trace("ERROR: GetProcAddress with an empty function name "
              "returned a non-NULL value when it should have "
              "returned a NULL value with an error\n");

         /* Cleanup */
        err = FreeLibrary(hModule);
        if(0 == err)
	    {
            Fail("Unexpected error: Failed to FreeLibrary %s\n", 
                 lpModuleName);
	    }
        Fail("");
	}

    /**
     * Test 4
     *
     * Call GetProcAddress with an invalid name
     */

    procAddress = GetProcAddress(hModule,"Simple Function");
    if(procAddress != NULL)
	{
        Trace("ERROR: GetProcAddress with an invalid function name "
              "returned a non-NULL value when it should have "
              "returned a NULL value with an error\n");

         /* Cleanup */
        err = FreeLibrary(hModule);
        if(0 == err)
	    {
            Fail("Unexpected error: Failed to FreeLibrary %s\n", 
                 lpModuleName);
	    }
        Fail("");
	}

    /* cleanup */
    err = FreeLibrary(hModule);
    if(0 == err)
	{
        Fail("Unexpected error: Failed to FreeLibrary %s\n", 
              lpModuleName);
	}

    PAL_Terminate();
    return PASS;
}
