/*=====================================================================
**
** Source:  test5.c 
**
** Purpose: Tests the PAL implementation of the SetEndOfFile function.
**          Test attempts to read the number of characters up to
**          the EOF pointer, which was specified.
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

int __cdecl main(int argc, char *argv[])
{
    HANDLE  hFile = NULL;
    DWORD   dwBytesWritten;
    DWORD   retCode;
    BOOL    bRc = FALSE;
    char    szBuffer[256];
    DWORD   dwBytesRead     = 0;
    char    testFile[]      = "testfile.tmp";
    char    testString[]    = "watch what happens";
    LONG shiftAmount = 10;

    /* Initialize the PAL.
     */
    if (0 != PAL_Initialize(argc,argv))
    {
        return FAIL;
    }

     /* Initialize the buffer.
     */
    memset(szBuffer, 0, 256);

    /* Create a file to test with.
    */
    hFile = CreateFile(testFile, 
                       GENERIC_WRITE|GENERIC_READ,
                       FILE_SHARE_WRITE|FILE_SHARE_READ,
                       NULL,
                       CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);

    if(hFile == INVALID_HANDLE_VALUE)
    {
        Fail("ERROR:%u: Unable to create file \"%s\".\n",
             GetLastError(),
             testFile);
    }

    /* Write to the File handle.
    */ 
    bRc = WriteFile(hFile,
                    testString,
                    strlen(testString),
                    &dwBytesWritten,
                    NULL);

    if (bRc == FALSE)
    {
        Trace("ERROR:%u: Unable to write to file handle "
                "hFile=0x%lx\n",
                GetLastError(),
                hFile);
        if (!CloseHandle(hFile))
        {
            Fail("ERROR:%u%: Unable to close handle 0x%lx.\n",
                 GetLastError(),
                 hFile);
        }
        Fail("");
    }

    /* Set the file pointer to shiftAmount bytes from the front of the file
    */
    retCode = SetFilePointer(hFile, shiftAmount, NULL, FILE_BEGIN);
    if(retCode == INVALID_SET_FILE_POINTER)
    {
        Trace("ERROR:%u: Unable to set the file pointer to %d\n",
            GetLastError(),
            shiftAmount);
        if (!CloseHandle(hFile))
        {
            Fail("ERROR:%u%: Unable to close handle 0x%lx.\n",
                 GetLastError(),
                 hFile);
        }
        Fail("");
    }

    /* set the end of file pointer to 'shiftAmount' */
    bRc = SetEndOfFile(hFile);
    if (bRc == FALSE)
    {
        Trace("ERROR:%u: Unable to set file pointer of file handle 0x%lx.\n",
              GetLastError(),
              hFile);
        if (!CloseHandle(hFile))
        {
            Fail("ERROR:%u%: Unable to close handle 0x%lx.\n",
                 GetLastError(),
                 hFile);
        }
        Fail("");
    }

    /* Set the file pointer to 10 bytes from the front of the file
    */
    retCode = SetFilePointer(hFile, (LONG)NULL, NULL, FILE_BEGIN);
    if(retCode == INVALID_SET_FILE_POINTER)
    {
        Trace("ERROR:%u: Unable to set the file pointer to %d\n",
            GetLastError(),
            FILE_BEGIN);
        if (!CloseHandle(hFile))
        {
            Fail("ERROR:%u%: Unable to close handle 0x%lx.\n",
                 GetLastError(),
                 hFile);
        }
        Fail("");
    }

    /* Attempt to read the entire string, 'testString' from a file
    * that has it's end of pointer set at shiftAmount;
    */
    bRc = ReadFile(hFile,
                   szBuffer,
                   strlen(testString),
                   &dwBytesRead,
                   NULL);

    if (bRc == FALSE)
    {
        Trace("ERROR:%u: Unable to read from file handle 0x%lx.\n",
              GetLastError(),
              hFile);
        if (!CloseHandle(hFile))
        {
            Fail("ERROR:%u%: Unable to close handle 0x%lx.\n",
                 GetLastError(),
                 hFile);
        }
        Fail("");
    }

    /* Confirm the number of bytes read with that requested.
    */
    if (dwBytesRead != shiftAmount)
    {
        Trace("ERROR: The number of bytes read \"%d\" is not equal to the "
              "number that should have been written \"%d\".\n",
              dwBytesRead,
              shiftAmount);
        if (!CloseHandle(hFile))
        {
            Fail("ERROR:%u%: Unable to close handle 0x%lx.\n",
                 GetLastError(),
                 hFile);
        }
        Fail("");
    }

    bRc = CloseHandle(hFile);
    if(!bRc)
    {
        Fail("ERROR:%u CloseHandle failed to close the handle\n",
             GetLastError());
    }
 
    /* Terminate the PAL.
     */
    PAL_Terminate();
    return PASS;
}
            
