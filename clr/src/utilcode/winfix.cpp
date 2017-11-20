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
//*****************************************************************************
// WinWrap.cpp
//
// This file contains wrapper functions for Win32 API's that take strings.
// Support on each platform works as follows:
//      OS          Behavior
//      ---------   -------------------------------------------------------
//      NT          Fully supports both W and A funtions.
//      Win 9x      Supports on A functions, stubs out the W functions but
//                      then fails silently on you with no warning.
//      CE          Only has the W entry points.
//
// COM+ internally uses UNICODE as the internal state and string format.  This
// file will undef the mapping macros so that one cannot mistakingly call a
// method that isn't going to work.  Instead, you have to call the correct
// wrapper API.
//
//*****************************************************************************

#include "stdafx.h"                     // Precompiled header key.
#include "winwrap.h"                    // Header for macros and functions.
#include "utilcode.h"
#include "version/__file__.ver"


ULONG DBCS_MAXWID=0;

int UseUnicodeAPI()
{
    return TRUE;
}

BOOL OnUnicodeSystem()
{
    if (DBCS_MAXWID == 0)
    {
        CPINFO  cpInfo;

        if (GetCPInfo(CP_ACP, &cpInfo))
            DBCS_MAXWID = cpInfo.MaxCharSize;
        else
            DBCS_MAXWID = 2;        
    }

    return TRUE;
}



/////////////////////////////////////////////////////////////////////////
//
// WARNING: below is a very large #ifdef that groups together all the 
//          wrappers that are X86-only.  They all mirror some function 
//          that is known to be available on the non-X86 win32 platforms
//          in only the Unicode variants.  
//
/////////////////////////////////////////////////////////////////////////
#ifdef PLATFORM_WIN32
#endif // PLATFORM_WIN32

//////////////////////////////////////////////
//
// END OF X86-ONLY wrappers
//
//////////////////////////////////////////////

static void xtow (
        unsigned long val,
        LPWSTR buf,
        unsigned radix,
        int is_neg
        )
{
        WCHAR *p;               /* pointer to traverse string */
        WCHAR *firstdig;        /* pointer to first digit */
        WCHAR temp;             /* temp char */
        unsigned digval;        /* value of digit */

        p = buf;

        if (is_neg) {
            /* negative, so output '-' and negate */
            *p++ = (WCHAR) '-';
            val = (unsigned long)(-(long)val);
        }

        firstdig = p;           /* save pointer to first digit */

        do {
            digval = (unsigned) (val % radix);
            val /= radix;       /* get next digit */

            /* convert to text and store */
            if (digval > 9)
                *p++ = (WCHAR) (digval - 10 + 'A');  /* a letter */
            else
                *p++ = (WCHAR) (digval + '0');       /* a digit */
        } while (val > 0);

        /* We now have the digit of the number in the buffer, but in reverse
           order.  Thus we reverse them now. */

        *p-- = 0;               /* terminate string; p points to last digit */

        do {
            temp = *p;
            *p = *firstdig;
            *firstdig = temp;   /* swap *p and *firstdig */
            --p;
            ++firstdig;         /* advance to next two digits */
        } while (firstdig < p); /* repeat until halfway */
}

LPWSTR
Wszltow(
    LONG val,
    LPWSTR buf,
    int radix
    )
{
    xtow((unsigned long)val, buf, radix, (radix == 10 && val < 0));
    return buf;
}

LPWSTR
Wszultow(
    ULONG val,
    LPWSTR buf,
    int radix
    )
{
    xtow(val, buf, radix, 0);
    return buf;
}


//-----------------------------------------------------------------------------
// WszConvertToUnicode
//
// @func Convert a string from Ansi to Unicode
//
// @devnote cbIn can be -1 for Null Terminated string
//
// @rdesc HResult indicating status of Conversion
//      @flag S_OK | Converted to Ansi
//      @flag S_FALSE | Truncation occurred
//      @flag E_OUTOFMEMORY | Allocation problem.
//-----------------------------------------------------------------------------------
HRESULT WszConvertToUnicode
    (
    LPCSTR          szIn,       //@parm IN | Ansi String
    LONG            cbIn,       //@parm IN | Length of Ansi String in bytest
    LPWSTR*         lpwszOut,   //@parm INOUT | Unicode Buffer
    ULONG*          lpcchOut,   //@parm INOUT | Length of Unicode String in characters -- including '\0'
    BOOL            fAlloc      //@parm IN | Alloc memory or not
    )
{
    ULONG       cchOut;
    ULONG       cbOutJunk = 0;
//  ULONG       cchIn = szIn ? strlen(szIn) + 1 : 0;
            
//  _ASSERTE(lpwszOut);

    if (!(lpcchOut))
        lpcchOut = &cbOutJunk;

    if ((szIn == NULL) || (cbIn == 0))
    {
        *lpwszOut = NULL;
        if( lpcchOut )
            *lpcchOut = 0;
        return S_OK;
    }

    // Allocate memory if requested.   Note that we allocate as
    // much space as in the unicode buffer, since all of the input
    // characters could be double byte...
    if (fAlloc)
    {
        // Determine the number of characters needed 
        cchOut = (MultiByteToWideChar(CP_ACP,
                                0,                              
                                szIn,
                                cbIn,
                                NULL,
                                0));

        // _ASSERTE( cchOut != 0 );
        *lpwszOut = (LPWSTR) new WCHAR[cchOut];
        *lpcchOut = cchOut;     // Includes '\0'.

        if (!(*lpwszOut))
        {
//          TRACE("WszConvertToUnicode failed to allocate memory");
            return E_OUTOFMEMORY;
        }

    } 

    if( !(*lpwszOut) )
        return S_OK;
//  _ASSERTE(*lpwszOut);

    cchOut = (MultiByteToWideChar(CP_ACP,
                                0,                              
                                szIn,
                                cbIn,
                                *lpwszOut,
                                *lpcchOut));

    if (cchOut)
    {
        *lpcchOut = cchOut;
        return S_OK;
    }


//  _ASSERTE(*lpwszOut);
    if( fAlloc )
    {
        delete[] *lpwszOut;
        *lpwszOut = NULL;
    }
/*
    switch (GetLastError())
    {
        case    ERROR_NO_UNICODE_TRANSLATION:
        {
            OutputDebugString(TEXT("ODBC: no unicode translation for installer string"));
            return E_FAIL;
        }

        default:


        {
            _ASSERTE("Unexpected unicode error code from GetLastError" == NULL);
            return E_FAIL
        }
    }
*/
    return E_FAIL; // NOTREACHED
}


//-----------------------------------------------------------------------------
// WszConvertToAnsi
//
// @func Convert a string from Unicode to Ansi
//
// @rdesc HResult indicating status of Conversion
//      @flag S_OK | Converted to Ansi
//      @flag S_FALSE | Truncation occurred
//      @flag E_OUTOFMEMORY | Allocation problem.
//-----------------------------------------------------------------------------------
HRESULT WszConvertToAnsi
    (
    LPCWSTR         szIn,       //@parm IN | Unicode string
    LPSTR*          lpszOut,    //@parm INOUT | Pointer for buffer for ansi string
    ULONG           cbOutMax,   //@parm IN | Max string length in bytes
    ULONG*          lpcbOut,    //@parm INOUT | Count of bytes for return buffer
    BOOL            fAlloc      //@parm IN | Alloc memory or not
    )
{
    ULONG           cchInActual;
    ULONG           cbOutJunk;
    ULONG           cchIn = szIn ? lstrlenW (szIn) + 1 : 0;

    if (!(lpcbOut))
        lpcbOut = &cbOutJunk;

    if ((szIn == NULL) || (cchIn == 0))
    {
        *lpszOut = NULL;
        *lpcbOut = 0;
        return S_OK;
    }

    // Allocate memory if requested.   Note that we allocate as
    // much space as in the unicode buffer, since all of the input
    // characters could be double byte...
    cchInActual = cchIn;
    if (fAlloc)
    {
        cbOutMax = (WideCharToMultiByte(CP_ACP,
                                    0,                              
                                    szIn,
                                    cchInActual,
                                    NULL,
                                    0,
                                    NULL,
                                    FALSE));

        *lpszOut = (LPSTR) new CHAR[cbOutMax];

        if (!(*lpszOut))
        {
//          TRACE("WszConvertToAnsi failed to allocate memory");
            return E_OUTOFMEMORY;
        }

    } 

    if (!(*lpszOut))
        return S_OK;

//  _ASSERTE(*lpszOut);

    *lpcbOut = (WideCharToMultiByte(CP_ACP,
                                    0,                              
                                    szIn,
                                    cchInActual,
                                    *lpszOut,
                                    cbOutMax,
                                    NULL,
                                    FALSE));

    // Overflow on unicode conversion
    if (*lpcbOut > cbOutMax)
    {
        // If we had data truncation before, we have to guess
        // how big the string could be.   Guess large.
        if (cchIn > cbOutMax)
            *lpcbOut = cchIn * DBCS_MAXWID;

        return S_FALSE;
    }

    // handle external (driver-done) truncation
    if (cchIn > cbOutMax)
        *lpcbOut = cchIn * DBCS_MAXWID;
//  _ASSERTE(*lpcbOut);

    return S_OK;
}



DWORD
WszGetWorkingSet()
{
    DWORD dwMemUsage = 0;


    return dwMemUsage;
}
