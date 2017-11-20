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
// File: strbuild.h
//
// ===========================================================================

#ifndef _STRBUILD_H_
#define _STRBUILD_H_

#include "name.h"

interface ICSNameTable;

////////////////////////////////////////////////////////////////////////////////
// CStringBuilder
//
// This is a string buffer optimized for appending text onto the end in several
// small increments, potentially resulting in large strings.  The string buffer
// starts out as a static buffer, and grows as necessary to accomodate larger
// strings.  Note that the append function(s) don't return a result code; instead
// any failure is remembered, and returned when the string is eventually turned
// into a NAME or a BSTR.  Users can also check for successful operation by
// calling GetResultCode(), in the event that the resulting string is not 
// required in the form of either a BSTR or NAME.

class CStringBuilder
{
protected:
    // NOTE:  Keep this on a power-of-2 boundary!
    enum { ALLOC_SIZE = 256, ALLOC_MASK = ~(ALLOC_SIZE - 1) };

    WCHAR       m_szBuf[ALLOC_SIZE];    // Initial buffer, used for nearly all strings
    PWSTR       m_pszText;              // Buffer -- either points to m_szBuf, or an allocated block
    PWSTR       m_pszCur;               // Pointer to next append location
    size_t      m_iLen;                 // Length so far (allocation size = (m_iLen + ALLOC_SIZE) & ALLOC_MASK, if m_iLen >= ALLOCSIZE)
    size_t      m_iAllocSize;           // Variable allocation size
    HRESULT     m_hr;                   // Result code

public:
    CStringBuilder (PCWSTR pszText = NULL) : m_pszText(m_szBuf), m_pszCur(m_szBuf), m_iLen(0), m_iAllocSize(ALLOC_SIZE), m_hr(S_OK) { m_szBuf[0] = 0; if (pszText != NULL) Append (pszText); }
    ~CStringBuilder () { if (m_pszText != m_szBuf) VSFree (m_pszText); }

    // Create a BSTR out of the text in the buffer.
    HRESULT     CreateBSTR (BSTR *pbstrOut);

    // Create a NAME out of the text in the buffer
    HRESULT     CreateName (ICSNameTable *pNameTable, NAME **ppName);

    // Get the result code of all previous operations
    HRESULT     GetResultCode () { return m_hr; }

    // Convert the string to lower case
    HRESULT     ToLower ();

    // Append text
    void virtual Append (PCWSTR pszText, size_t iLen = (size_t)-1);

    // Get current length
    size_t      GetLength () { return m_iLen; }

    // Rewind
    void        Rewind (size_t iLen) 
    { 
        ASSERT (iLen <= m_iLen); 
        if (SUCCEEDED (m_hr)) 
        { 
            m_iLen = iLen; 
            m_pszCur = m_pszText + iLen;
            m_pszCur[0] = 0;
        }
    }

    // For ease of use...
    CStringBuilder  & operator += (PCWSTR pszText) { Append (pszText); return *this; }
    CStringBuilder  & operator = (PCWSTR pszText) { Rewind (0); Append (pszText); return *this; }
    operator PWSTR () const { return m_pszText; }
};

#endif  // _STRBUILD_H_
