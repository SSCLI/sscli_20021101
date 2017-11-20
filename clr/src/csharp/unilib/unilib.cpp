// ==++==
//
//   
//    Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
//   
//    The use and distribution terms for this software are contained in the file
//    named license.txt, which can be found in the root of this distribution.
//    By using this software in any fashion, you are agreeing to be bound by the
//    terms of this license.
//   
//    You must not remove this notice, or any other, from this software.
//   
//
// ==--==
// ===========================================================================
// File: unilib.cpp
//
// This file contains various UNICODE api functions
// ===========================================================================

//
//  Include Files.
//

#include "unilib.h"


//-----------------------------------------------------------------------------
EXTERN_C PWSTR WINAPI ToLowerCase (PCWSTR pSrc, PWSTR pDst, size_t cch)
{
    if (!pSrc)
    {
        *pDst = 0;
        return pDst;
    }
    WCHAR ch;
    if ((size_t) -1 == cch)
    {
        while ((ch = *pSrc++))
            *pDst++ = towlower(ch);
        *pDst = 0;
        return pDst;
    }
    if (cch)
    {
        for (; cch && (ch = *pSrc++); cch--)
            *pDst++ = towlower(ch);
        if (cch) // if room, terminate
            *pDst = 0;
    }
    return pDst;
}

inline PWSTR WINAPI NormalizeFileSlashes (PWSTR szFile)
{
    const WCHAR BS = '\\';
    const WCHAR FS = '/';
    PWSTR pchCopy;
    PWSTR pch = szFile;

    if (FS == *pch) *pch = BS;
    pch++;
    pchCopy = pch;

    WCHAR ch;
    while ((ch = *pch))
    {
        if ((BS == ch) || (FS == ch))
        {
            *pchCopy++ = BS;
            pch++;
            while ((BS == (ch = *pch)) || (FS == ch))
                pch++;
        }
        else
        {
            *pchCopy++ = ch;
            pch++;
        }
    }
    *pchCopy = 0;
    return pchCopy;
}

inline PWSTR WINAPI CopyCat (PWSTR dst, PCWSTR src)
{
    while ((*dst++ = *src++))
        ;
    return --dst; 
}

EXTERN_C void WINAPI W_GetActualFileCase(PCWSTR pszName, PWSTR psz)
{
    WIN32_FIND_DATAW fd;
    HANDLE           hFind;
    WCHAR            szName[MAX_PATH], sz[MAX_PATH], *pchScan, *pchSection, *pchBuild, *pchOut;
    WCHAR            *pchScanPrevious, *pchOutPrevious;

    CopyCat(szName, pszName);
    NormalizeFileSlashes(szName);
    pchScan = szName;
    pchOut  = psz;
    pchSection = sz;
    pchBuild = pchSection;
    if (*pchScan == L'\\')
    {
        *pchOut++ = *pchBuild++ = *pchScan++;
#if !PLATFORM_UNIX
        if (*pchScan == L'\\')
        {
            // It's a UNC name.
            pchOut--;
            *pchBuild++ = *pchScan++;
            while (*pchScan && *pchScan != L'\\')
                *pchBuild++ = *pchScan++;
            *pchBuild++ = *pchScan++;
            while (*pchScan && *pchScan != L'\\')
                *pchBuild++ = *pchScan++;
            if (*pchScan == L'\\')
                *pchBuild++ = *pchScan++;
            *pchBuild = 0;
            pchOut = CopyCat(pchOut, pchSection);
            pchSection = pchBuild;
        }
#endif  // !PLATFORM_UNIX
    }
    else if (*(pchScan+1) == L':')
    {
        *pchBuild++ = towupper(*pchScan++);
        *pchBuild++ = *pchScan++;
        if (*pchScan == L'\\')
            *pchBuild++ = *pchScan++;
        *pchBuild = 0;
        pchOut = CopyCat(pchOut, pchSection);
        pchSection = pchBuild;
    }
    for (;;)
    {
        pchScanPrevious = pchScan;
        while (*pchScan && *pchScan != L'\\')
            *pchBuild++ = *pchScan++;
        *pchBuild = 0;

        if ((wcsncmp(pchScanPrevious, L"..", 2) == 0) && pchOut > psz && *(pchOut-1) == L'\\')
        {
            // Remove the .. from the name
            pchBuild -= 2;
            *pchBuild = 0;
            // Increase past the '\'
            pchScan++;

            // Remove the last directory
            pchOut--;
            while (pchOut > psz && *(pchOut-1) != L'\\')
            {
                pchOut--;
            }
        }
        else
        {
            hFind = FindFirstFileW(sz, &fd);
            if (INVALID_HANDLE_VALUE == hFind)
            {
                // copy the rest of the path verbatim.
                pchOut = CopyCat(pchOut, pchSection);
                CopyCat(pchOut, pchScan);
                return;
            }
            ::FindClose(hFind);

            pchOutPrevious = pchOut;
            pchOut = CopyCat(pchOut, fd.cFileName);
            if (0 == *pchScan)
                return;

            pchScan++;
            *pchBuild++ = *pchOut++ = L'\\';
            pchSection = pchBuild;
        }
    }
}

// Note this doesn't set the errno if there's an error but no one seemed to be using it.
EXTERN_C int WINAPI W_Access(PCWSTR pPathName, int mode)
{
    // Only support checking for read access
    _ASSERTE(mode == 0x4);
    HANDLE hFile;
    hFile = CreateFileW(pPathName,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        // Not Errno not set
        CloseHandle (hFile);
        return 0;
    }
    return -1;
}
