/*=====================================================================
**
** Source:  WriteFile.c (test 1)
**
** Purpose: Tests the PAL implementation of the WriteFile function.
**          This test will attempt to write to a NULL handle and a
**          read-only file
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


const char* szStringTest = "The quick fox jumped over the lazy dog's back.";
const char* szReadOnlyFile = "ReadOnly.txt";


int __cdecl main(int argc, char *argv[])
{
    HANDLE hFile = NULL;
    DWORD dwByteCount = 0;
    DWORD dwBytesWritten;
    BOOL bRc = FALSE;


    if (0 != PAL_Initialize(argc,argv))
    {
        return FAIL;
    }

    //
    // Write to a NULL handle
    //

    bRc = WriteFile(hFile, szStringTest, 20, &dwBytesWritten, NULL);

    if (bRc == TRUE)
    {
        Fail("WriteFile: ERROR -> Able to write to a NULL handle\n");
    }


    //
    // Write to a file with read-only permissions
    //

    // create a file without write permissions
    hFile = CreateFile(szReadOnlyFile, 
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if(hFile == INVALID_HANDLE_VALUE)
    {
        dwByteCount = GetLastError();
        Fail("WriteFile: ERROR -> Unable to create file \"%s\".\n", 
            szReadOnlyFile);
    }
    
    if (!SetFileAttributes(szReadOnlyFile, FILE_ATTRIBUTE_READONLY))
    {
        Fail("WriteFile: ERROR -> Unable to make the file read-only.\n");
    }

    bRc = WriteFile(hFile, szStringTest, 20, &dwBytesWritten, NULL);
    if (bRc == TRUE)
    {
        Fail("WriteFile: ERROR -> Able to write to a read-only file.\n");
    }

    bRc = CloseHandle(hFile);
    if (bRc != TRUE)
    {
        Fail("WriteFile: ERROR -> Unable to close file \"%s\".\n", 
            szReadOnlyFile);
    }


    PAL_Terminate();
    return PASS;
}
