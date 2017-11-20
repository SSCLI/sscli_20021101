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
// File: shlwapi.h
//
// Header for ported shlwapi stuff
// ===========================================================================

#ifndef SHLWAPIP_H_INCLUDED
#define SHLWAPIP_H_INCLUDED

#define UNICODE 1

#include "rotor_palrt.h"

#define ARRAYSIZE(x)    (sizeof(x)/sizeof(x[0]))
#define SIZECHARS(sz)   (sizeof(sz)/sizeof(sz[0]))

#define SIZEOF(x)       sizeof(x)
#define PRIVATE
#define PUBLIC
#define ASSERT          _ASSERTE
#define AssertMsg(f,m)  _ASSERTE(f)
#define RIP(f)          _ASSERTE(f)
#define RIPMSG(f,m)     _ASSERTE(f)

#define IsFlagSet(obj, f)           (BOOL)(((obj) & (f)) == (f))
#define IsFlagClear(obj, f)         (BOOL)(((obj) & (f)) != (f))

#define InRange(id, idFirst, idLast)      ((UINT)((id)-(idFirst)) <= (UINT)((idLast)-(idFirst)))

#define CbFromCch(cch)              ((cch)*sizeof(TCHAR))

#define IS_VALID_READ_BUFFER(p, t, n)   (p != NULL)
#define IS_VALID_WRITE_BUFFER(p, t, n)  (p != NULL)

#define IS_VALID_READ_PTR(p, t)         IS_VALID_READ_BUFFER(p, t, 1)
#define IS_VALID_WRITE_PTR(p, t)        IS_VALID_WRITE_BUFFER(p, t, 1)

#define IS_VALID_STRING_PTR(p, c)       (p != NULL)
#define IS_VALID_STRING_PTRW(p, c)      (p != NULL)

#define CharLowerW  _wcslwr

inline int StrCmpNCW(LPCWSTR pch1, LPCWSTR pch2, int n)
{
    if (n == 0)
        return 0;

    while (--n && *pch1 && *pch1 == *pch2)
    {
        pch1++;
        pch2++;
    }

    return *pch1 - *pch2;
}

typedef struct tagPARSEDURLW {
    DWORD     cbSize;
    // Pointers into the buffer that was provided to ParseURL
    LPCWSTR   pszProtocol;
    UINT      cchProtocol;
    LPCWSTR   pszSuffix;
    UINT      cchSuffix;
    UINT      nScheme;            // One of URL_SCHEME_*
    } PARSEDURLW, * PPARSEDURLW;

typedef enum {
    URL_SCHEME_INVALID     = -1,
    URL_SCHEME_UNKNOWN     =  0,
    URL_SCHEME_FTP = 1,
    URL_SCHEME_HTTP = 2,
    URL_SCHEME_FILE = 9,
    URL_SCHEME_HTTPS = 11,
} URL_SCHEME;

#define URL_ESCAPE_UNSAFE               0x20000000
#define URL_DONT_ESCAPE_EXTRA_INFO      0x02000000
#define URL_ESCAPE_SPACES_ONLY          0x04000000
#define URL_DONT_SIMPLIFY               0x08000000
#define URL_UNESCAPE_INPLACE            0x00100000
#define URL_ESCAPE_PERCENT              0x00001000
#define URL_ESCAPE_SEGMENT_ONLY         0x00002000  // Treat the entire URL param as one URL segment.

#endif  // ! SHLWAPIP_H_INCLUDED
