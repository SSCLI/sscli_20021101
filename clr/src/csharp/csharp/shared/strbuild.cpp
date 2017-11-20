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
// File: strbuild.cpp
//
// ===========================================================================

#include "pch.h"
#include "strbuild.h"
#include "scciface.h"

////////////////////////////////////////////////////////////////////////////////
// CStringBuilder::CreateBSTR

HRESULT CStringBuilder::CreateBSTR (BSTR *pbstrOut)
{
    // Report any accumulation failure
    if (FAILED (m_hr))
        return m_hr;

    // Just allocate a BSTR out of our current contents.
    return AllocateBSTR (m_pszText, pbstrOut);
}

////////////////////////////////////////////////////////////////////////////////
// CStringBuilder::CreateName

HRESULT CStringBuilder::CreateName (ICSNameTable *pNameTable, NAME **ppName)
{
    // Report any accumulation failure
    if (FAILED (m_hr))
        return m_hr;

    // Create a name using the given name table
    return pNameTable->Add (m_pszText, ppName);
}


////////////////////////////////////////////////////////////////////////////////
// CStringBuilder::Append

void CStringBuilder::Append (PCWSTR pszText, size_t iLen)
{
    // Do nothing if we've already failed
    if (FAILED (m_hr))
        return;

    // If -1, use nul to determine length
    if (iLen == (size_t)-1)
        iLen = wcslen (pszText);

    // Determine new size and reallocate if necessary
    size_t      iReqSize = (m_iLen + iLen + m_iAllocSize) & (~(m_iAllocSize -1));

    if (m_pszText == m_szBuf)
    {
        if (iReqSize > (sizeof (m_szBuf) / sizeof (m_szBuf[0])))
        {
            // The size needed is bigger than our static buffer.  Time to allocate
            // the dynamic buffer for the first time
            m_pszText = (PWSTR)VSAlloc (iReqSize * sizeof (WCHAR));
            if (m_pszText == NULL)
            {
                m_hr = E_OUTOFMEMORY;
                m_pszText = m_szBuf;
                return;
            }

            // Copy the current text
            wcscpy (m_pszText, m_szBuf);

            // Keep our current pointer current...
            m_pszCur = m_pszText + m_iLen;
            ASSERT (m_pszCur[0] == 0);
        }
        else
        {
            // Do nothing -- the text will fit in our static buffer.
        }
    }
    else if (iReqSize != ((m_iLen + m_iAllocSize) & (~(m_iAllocSize - 1))))
    {
        // See if we need to increase our allocation size
        if (iReqSize >= 0x10000 && m_iAllocSize < 0x10000)
        {
            m_iAllocSize = 0x10000;
            iReqSize = ((m_iLen + iLen + m_iAllocSize) & (~(m_iAllocSize - 1)));
        }
        else if (iReqSize >= 0x1000 && m_iAllocSize < 0x1000)
        {
            m_iAllocSize = 0x1000;
            iReqSize = ((m_iLen + iLen + m_iAllocSize) & (~(m_iAllocSize - 1)));
        }

        // Need to grow
        PWSTR   pszNewText = (PWSTR)VSRealloc (m_pszText, iReqSize * sizeof (WCHAR));
        if (pszNewText == NULL)
        {
            m_hr = E_OUTOFMEMORY;
            return;
        }
        m_pszText = pszNewText;

        // Keep our current pointer current...
        m_pszCur = m_pszText + m_iLen;
        ASSERT (m_pszCur[0] == 0);
    }

    ASSERT (iReqSize > m_iLen + iLen);

    // Copy incoming text to end of buffer.
    wcsncpy (m_pszCur, pszText, iLen);
    m_iLen += iLen;
    m_pszCur += iLen;
    m_pszCur[0] = 0;
}

////////////////////////////////////////////////////////////////////////////////
// CStringBuilder::ToLower ()

HRESULT CStringBuilder::ToLower ()
{
    // In place converts to lower case
    for (size_t iPos = 0; iPos < m_iLen; iPos++)
        m_pszText[iPos] = towlower(m_pszText[iPos]);

    return NOERROR;
}
