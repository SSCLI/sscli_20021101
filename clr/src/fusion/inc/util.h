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
#ifndef UTIL_H
#define UTIL_H

#include "fusionheap.h"
#include "shlwapi.h"

#if !defined(NUMBER_OF)
#define NUMBER_OF(a) (sizeof(a)/sizeof((a)[0]))
#endif

inline
WCHAR*
WSTRDupDynamic(LPCWSTR pwszSrc)
{
    LPWSTR pwszDest = NULL;
    if (pwszSrc != NULL)
    {
        const DWORD dwLen = lstrlenW(pwszSrc) + 1;
        pwszDest = FUSION_NEW_ARRAY(WCHAR, dwLen);

        if( pwszDest )
            memcpy(pwszDest, pwszSrc, dwLen * sizeof(WCHAR));
    }

    return pwszDest;
}


#if defined(UNICODE)
#define TSTRDupDynamic WSTRDupDynamic
#else
#define TSTRDupDynamic STRDupDynamic
#endif

inline
LPBYTE
MemDupDynamic(const BYTE *pSrc, DWORD cb)
{
    ASSERT(cb);
    LPBYTE  pDest = NULL;

    pDest = FUSION_NEW_ARRAY(BYTE, cb);
    if(pDest)
        memcpy(pDest, pSrc, cb);

    return pDest;
}





#endif  // UTIL_H
