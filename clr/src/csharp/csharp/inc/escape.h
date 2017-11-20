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
// File: escape.h
//
// ===========================================================================

#ifndef _ESCAPE_H_
#define _ESCAPE_H_

////////////////////////////////////////////////////////////////////////////////
// ScanEscape
//
// Given a pointer to a backslash in a character stream, check for a unicode
// escape sequence.


inline HRESULT ScanEscape (PCWSTR start, PCWSTR& p, WCHAR& ch, BOOL fAdvance)
{
    ASSERT(p[0] == L'\\');

    if ((p[1] != L'u' && p[1] != L'U') &&
        (p < start + 6 || p[-6] != L'\\' || p[-5] != L'U'))
        return E_FAIL;
    PCWSTR  pchStart = p;

    if (p[1] == L'u')
    {
        // 4-digit Unicode
        ASSERT(*p == '\\');
        p++;
        if (*p == 'u')
            p++;
        WCHAR szEsc[5];
        for (int i = 0; i < 4; i++)
        {
            if (p[i] <= 'f' && isxdigit(p[i]))
                szEsc[i] = p[i];
            else
                goto Fail;
        }
        szEsc[4] = 0;

        unsigned long j = wcstoul(szEsc, NULL, 16);
        if (j != 0 || !wcscmp(szEsc, L"0000"))
        {
            ch = (WCHAR)j;
            p = fAdvance ? p+4 : pchStart;
            return NOERROR;
        }
    }
    else if (p[1] == L'U')
    {
        // 8-digit Unicode, first character
        ASSERT(*p == '\\');
        p++;
        if (*p == L'U')
            p++;
        WCHAR szEsc[9];
        for (int i = 0; i < 8; i++)
        {
            if (p[i] <= 'f' && isxdigit(p[i]))
                szEsc[i] = p[i];
            else
                goto Fail;
        }
        szEsc[8] = 0;

        unsigned long j = wcstoul(szEsc, NULL, 16);
        if (j != 0 || !wcscmp(szEsc, L"00000000") && j <= 0x0010FFFF)
        {
            if (j > 0xFFFF)
            {
                ch = (WCHAR)((j - 0x00010000) / 0x0400 + 0xD800);
                p = fAdvance ? p+4 : pchStart;
            }
            else
            {
                ch = (WCHAR)j;
                p = fAdvance ? p+8 : pchStart;
            }
            return NOERROR;
        }
    }
    else
    {
        // 8-digit Unicode, second character
        ASSERT(p >= start + 6 && p[-6] == L'\\' && p[-5] == L'U');
        WCHAR szEsc[9];
        for (int i = -4; i < 4; i++)
        {
            if (p[i] <= 'f' && isxdigit(p[i]))
                szEsc[i + 4] = p[i];
            else
                goto Fail;
        }
        szEsc[8] = 0;

        unsigned long j = wcstoul(szEsc, NULL, 16);
        if (j <= 0x0010FFFF && j > 0xFFFF)
        {
            ch = (WCHAR)((j - 0x00010000) % 0x0400 + 0xDC00);
            p = fAdvance ? p+4 : pchStart;
            return NOERROR;
        }
    }

Fail:
    p = pchStart;
    return E_FAIL;
}

////////////////////////////////////////////////////////////////////////////////
// NEXTCHAR, PEEKCHAR
//
// These macros make Unicode-escape aware scanning of raw text easy.

__forceinline HRESULT ScanEscapeInline(PCWSTR start, PCWSTR& p, WCHAR& ch, BOOL fAdvance)
{
    if (p[0] != L'\\') {
        return E_FAIL;
    } else {
        return ScanEscape(start, p, ch, fAdvance);
    }
}

#define NEXTCHAR(s,p,ch) ((ch) = (FAILED (ScanEscapeInline ((s), (p), (ch), TRUE))) ? *(p)++ : (ch))
#define PEEKCHAR(s,p,ch) ((ch) = (FAILED (ScanEscapeInline ((s), (p), (ch), FALSE))) ? *(p) : (ch))

// These are the non-unicode-escape-aware versions.  Use these to determine the
// performance penalties of being unicode-escape-aware...
//#define NEXTCHAR(p,ch) (ch = *p++)
//#define PEEKCHAR(p,ch) (ch = *p)

#endif  // _ESCAPE_H_
