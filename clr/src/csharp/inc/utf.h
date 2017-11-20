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
// File: utf.h
//
// ===========================================================================

#ifndef __UTF_H__
#define __UTF_H__

//
//  Constant Declarations.
//

#define NULL_TERMINATED_MODE          (-1L)

inline int UTF8ToUnicode(
        LPCSTR lpSrcStr,
        int cchSrc,
        LPWSTR lpDestStr,
        int cchDest)

{
     return MultiByteToWideChar(CP_UTF8, 0, lpSrcStr, cchSrc, lpDestStr, cchDest);
}

inline int WINAPI UnicodeToUTF8(PCWSTR pUni,
                                int * pcchUni,
                                PSTR pUTF8,
                                int cbUTF)
{
    int cchSrc = *pcchUni;
    if (cchSrc == -1)
    {
        *pcchUni = (int)wcslen(pUni);
    }
    return WideCharToMultiByte(CP_UTF8, 0, pUni, cchSrc, pUTF8, cbUTF, NULL, false);
}

inline int WINAPI UnicodeLengthOfUTF8 (PCSTR pUTF8, int cbUTF8)
{
    return MultiByteToWideChar(CP_UTF8, 0, pUTF8, cbUTF8, NULL, 0);
}

inline int WINAPI UTF8LengthOfUnicode(PCWSTR pUTF16, int cchUTF16)
{
    return WideCharToMultiByte(CP_UTF8, 0, pUTF16, cchUTF16, NULL, 0, 0, 0);
}

#endif //__UTF_H__

