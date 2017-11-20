/*=============================================================
**
** Source:  loadlibrary.c (test 6)
**
** Purpose: Positive test the LoadLibrary API. Test will verify
**          that it is unable to load the library twice. Once by
**          using the full path name and secondly by using the 
**          short name.
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
    typedef int (*dllfunct)();
    char    LibraryName[] = "dlltest";
    #define GETATTACHCOUNTNAME "_GetAttachCount@0"
#else
#if defined(__APPLE__)
    char    LibraryName[] = "dlltest.dylib";
#else   // __APPLE__
    char    LibraryName[] = "dlltest.so";
#endif  // __APPLE__
    #define GETATTACHCOUNTNAME "GetAttachCount"
#endif

/* Helper function to test the loaded library.
 */
BOOL PALAPI TestDll(HMODULE hLib)
{
    int     RetVal;
    char    FunctName[] = GETATTACHCOUNTNAME;
    FARPROC DllFunc;  

    /* Access a function from the loaded library.
     */
    DllFunc = GetProcAddress(hLib, FunctName);
    if(DllFunc == NULL)
    {
        Trace("ERROR: Unable to load function \"%s\" from library \"%s\"\n",
              FunctName,
              LibraryName);
        return (FALSE);
    }

    /* Verify that the DLL_PROCESS_ATTACH is only 
     * accessed once.*/
    RetVal = DllFunc();
    if (RetVal != 1)
    {
        Trace("ERROR: Unable to receive correct information from DLL! "
              ":expected \"1\", returned \"%d\"\n",
              RetVal);
        return (FALSE);
    }

    return (TRUE);
}

int __cdecl main(int argc, char *argv[])
{
    HANDLE hFullLib;
    HANDLE hShortLib;
    int    iRetVal  = FAIL;
    DWORD  dwRetVal  = 0;
    LPSTR  fullPathPtr;
    char   fullPath[_MAX_DIR];

    /* Initialize the PAL. */
    if ((PAL_Initialize(argc, argv)) != 0)
    {
        return (FAIL);
    }

    /* Initalize the buffer.
     */
    memset(fullPath, 0, _MAX_DIR);

    /* Get the full path to the library (DLL).
     */
    dwRetVal = GetFullPathName( LibraryName,
                             _MAX_DIR,
                             fullPath,
                             &fullPathPtr);
    if (dwRetVal == 0)
    {
        Fail("ERROR:%u: Unable to get the full path to \"%s\".\n",
             GetLastError(),
             LibraryName);
    }

    /* Call Load library with the short name of
     * the dll.
     */
    hShortLib = LoadLibrary(LibraryName);
    if(hShortLib == NULL)
    {
        Fail("ERROR:%u:Unable to load library %s\n", 
             GetLastError(), 
             LibraryName);
    }
    
    /* Test the loaded library.
     */
    if (!TestDll(hShortLib))
    {
        iRetVal = FAIL;
        goto cleanUpOne;
    }

    /* Call Load library with the full name of
     * the dll.
     */
    hFullLib = LoadLibrary(fullPath);
    if(hFullLib == NULL)
    {
        Trace("ERROR:%u:Unable to load library %s\n", 
              GetLastError(), 
              fullPath);
        iRetVal = FAIL;
        goto cleanUpTwo;
    }

    /* Test the loaded library.
     */
    if (!TestDll(hFullLib))
    {
        iRetVal = FAIL;
        goto cleanUpTwo;
    }

    /* Test Succeeded.
     */
    iRetVal = PASS;

cleanUpTwo:

    /* Call the FreeLibrary API. 
     */ 
    if (!FreeLibrary(hFullLib))
    {
        Trace("ERROR:%u: Unable to free library \"%s\"\n", 
              GetLastError(),
              fullPath);
        iRetVal = FAIL;
    }

cleanUpOne:

    /* Call the FreeLibrary API. 
     */ 
    if (!FreeLibrary(hShortLib))
    {
        Trace("ERROR:%u: Unable to free library \"%s\"\n", 
              GetLastError(),
              LibraryName);
        iRetVal = FAIL;
    }

    /* Terminate the PAL.
     */
    PAL_Terminate();
    return iRetVal;

}
