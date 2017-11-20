/*=====================================================================
**
** Source:  FindFirstFileW.c
**
** Purpose: Tests the PAL implementation of the FindFirstFileW function.
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


const char* szNoFileName =          "333asdf.x77t";
const char* szFindName =            "test01.txt";
const char* szFindNameWldCard_01 =  "test0?.txt";
const char* szFindNameWldCard_02 =  "*.txt";
const char* szDirName =             "test_dir";
const char* szDirNameSlash =        "test_dir\\";
const char* szDirNameWldCard_01 =   "?est_dir";
const char* szDirNameWldCard_02 =   "test_*";



int __cdecl main(int argc, char *argv[])
{
    WIN32_FIND_DATAW findFileData;
    HANDLE hFind = NULL;
    FILE *pFile = NULL;
    BOOL bRc = FALSE;
    WCHAR* pTemp = NULL;


    if (0 != PAL_Initialize(argc,argv))
    {
        return FAIL;
    }


    //
    // find a file that doesn't exist
    //
    pTemp = convert((LPSTR)szNoFileName);
    hFind = FindFirstFileW(pTemp, &findFileData);
    free(pTemp);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        Fail ("FindFirstFileW: ERROR -> Found invalid NULL file\n");
    }


    //
    // find a file that exists
    //
    pFile = fopen(szFindName, "w");
    if (pFile == NULL)
    {
        Fail("FindFirstFileW: ERROR -> Unable to create a test file\n");
    }
    else
    {
        fclose(pFile);
    }
    pTemp = convert((LPSTR)szFindName);
    hFind = FindFirstFileW(pTemp, &findFileData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        free(pTemp);
        Fail ("FindFirstFileW: ERROR -> Unable to find \"%s\"\n", szFindName);
    }
    else
    {
        // validate we found the correct file
        if (wcscmp(pTemp, findFileData.cFileName) != 0)
        {
            free(pTemp);
            Fail ("FindFirstFileW: ERROR -> Found the wrong file\n");
        }
    }
    free(pTemp);

    //
    // find a directory that exists
    //
    pTemp = convert((LPSTR)szDirName);
    bRc = CreateDirectoryW(pTemp, NULL);
    if (bRc == FALSE)
    {
        Fail("FindFirstFileW: ERROR -> Failed to create the directory \"%s\"\n",
        szDirName);
    }

    hFind = FindFirstFileW(pTemp, &findFileData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        free(pTemp);
        Fail("FindFirstFileW: ERROR. Unable to find \"%s\"\n", szDirName);
    }
    else
    {
        // validate we found the correct directory
        if (wcscmp(pTemp, findFileData.cFileName) != 0)
        {
            free(pTemp);
            Fail("FindFirstFileW: ERROR -> Found the wrong directory\n");
        }
    }
    free(pTemp);

    //
    // find a directory using a trailing '\' on the directory name: should fail
    //
    pTemp = convert((LPSTR)szDirNameSlash);
    hFind = FindFirstFileW(pTemp, &findFileData);
    free(pTemp);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        Fail("FindFirstFileW: ERROR -> Able to find \"%s\": trailing "
            "slash should have failed.\n", 
            szDirNameSlash);
    }

    // find a file using wild cards
    pTemp = convert((LPSTR)szFindNameWldCard_01);
    hFind = FindFirstFileW(pTemp, &findFileData);
    free(pTemp);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        Fail("FindFirstFileW: ERROR -> Unable to find \"%s\"\n", 
            szFindNameWldCard_01);
    }

    pTemp = convert((LPSTR)szFindNameWldCard_02);
    hFind = FindFirstFileW(pTemp, &findFileData);
    free(pTemp);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        Fail("FindFirstFileW: ERROR -> Unable to find \"%s\"\n", 
            szFindNameWldCard_02);
    }


    //
    // find a directory using wild cards
    //

    pTemp = convert((LPSTR)szDirNameWldCard_01);
    hFind = FindFirstFileW(pTemp, &findFileData);
    free(pTemp);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        Fail("FindFirstFileW: ERROR -> Unable to find \"%s\"\n", 
            szDirNameWldCard_01);
    }

    pTemp = convert((LPSTR)szDirNameWldCard_02);
    hFind = FindFirstFileW(pTemp, &findFileData);
    free(pTemp);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        Fail("FindFirstFileW: ERROR -> Unable to find \"%s\"\n", 
            szDirNameWldCard_02);
    }

    pTemp = convert((LPSTR)szDirName);
    RemoveDirectoryW(pTemp);
    free(pTemp);
    DeleteFileA(szFindName);
    PAL_Terminate();  

    return PASS;
}
