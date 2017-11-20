/*=====================================================================
**
** Source:  GetTempFileNameA.c (test 3)
**
** Purpose: Tests the PAL implementation of the GetTempFileNameA function.
**          Checks the file attributes and ensures that getting a file name,
**          deleting the file and getting another doesn't produce the same 
**          as the just deleted file. Also checks the file size is 0.
**
** Depends on:
**          GetFileAttributesA
**          CloseHandle
**          DeleteFileA
**          CreateFileA
**          GetFileSize
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
    const UINT uUnique = 0;
    UINT uiError;
    const char* szDot = {"."};
    char szReturnedName[MAX_PATH];
    char szReturnedName_02[MAX_PATH];
    DWORD dwFileSize = 0;
    HANDLE hFile;

    if (0 != PAL_Initialize(argc, argv))
    {
        return FAIL;
    }


    /* valid path with null prefix */
    uiError = GetTempFileNameA(szDot, NULL, uUnique, szReturnedName);
    if (uiError == 0)
    {
        Fail("GetTempFileNameA: ERROR -> Call failed with a valid path "
            "with the error code: %u.\n", 
            GetLastError());
    }

    /* verify temp file was created */
    if (GetFileAttributesA(szReturnedName) == -1) 
    {
        Fail("GetTempFileNameA: ERROR -> GetFileAttributes failed on the "
            "returned temp file \"%s\" with error code: %u.\n", 
            szReturnedName,
            GetLastError());
    }

    /* 
    ** verify that the file size is 0 bytes
    */

    hFile = CreateFileA(szReturnedName,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        Trace("GetTempFileNameA: ERROR -> CreateFileA failed to open"
            " the created temp file with error code: %u.\n", 
            GetLastError());
        if (!DeleteFileA(szReturnedName))
        {
            Trace("GetTempFileNameA: ERROR -> DeleteFileA failed to delete"
                " the created temp file with error code: %u.\n", 
                GetLastError());
        }
        Fail("");
    }

    if ((dwFileSize = GetFileSize(hFile, NULL)) != (DWORD)0)
    {
        Trace("GetTempFileNameA: ERROR -> GetFileSize returned %u whereas"
            "it should have returned 0.\n", 
            dwFileSize);
        if (!CloseHandle(hFile))
        {
            Trace("GetTempFileNameA: ERROR -> CloseHandle failed. "
                "GetLastError returned: %u.\n", 
                GetLastError());
        }
        if (!DeleteFileA(szReturnedName))
        {
            Trace("GetTempFileNameA: ERROR -> DeleteFileA failed to delete"
                " the created temp file with error code: %u.\n", 
                GetLastError());
        }
        Fail("");
    }


    if (!CloseHandle(hFile))
    {
        Fail("GetTempFileNameA: ERROR -> CloseHandle failed. "
            "GetLastError returned: %u.\n", 
            GetLastError());
    }

    if (DeleteFileA(szReturnedName) != TRUE)
    {
        Fail("GetTempFileNameA: ERROR -> DeleteFileA failed to delete"
            " the created temp file with error code: %u.\n", 
            GetLastError());
    }

    /* get another and make sure it's not the same as the last */
    uiError = GetTempFileNameA(szDot, NULL, uUnique, szReturnedName_02);
    if (uiError == 0)
    {
        Fail("GetTempFileNameA: ERROR -> Call failed with a valid path "
            "with the error code: %u.\n", 
            GetLastError());
    }

    /* did we get different names? */
    if (strcmp(szReturnedName, szReturnedName_02) == 0)
    {
        Trace("GetTempFileNameA: ERROR -> The first call returned \"%s\". "
            "The second call returned \"%s\" and the two should not be"
            " the same.\n",
            szReturnedName,
            szReturnedName_02);
        if (!DeleteFileA(szReturnedName_02))
        {
            Trace("GetTempFileNameA: ERROR -> DeleteFileA failed to delete"
                " the created temp file with error code: %u.\n", 
                GetLastError());
        }
        Fail("");
    }

    /* clean up */
    if (!DeleteFileA(szReturnedName_02))
    {
        Fail("GetTempFileNameA: ERROR -> DeleteFileA failed to delete"
            " the created temp file with error code: %u.\n", 
            GetLastError());
    }


    PAL_Terminate();
    return PASS;
}
