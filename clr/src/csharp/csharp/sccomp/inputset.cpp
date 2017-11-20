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
// File: inputset.cpp
//
// ===========================================================================

#include "stdafx.h"
#include "inputset.h"
#include "controller.h"
#include "namemgr.h"

////////////////////////////////////////////////////////////////////////////////
// CInputSet::CInputSet

CInputSet::CInputSet () :
    m_pController(NULL),
    m_tableSources(NULL),
    m_pSrcHead(NULL),
    m_tableResources(NULL),
    m_fDLL(FALSE),
    m_fNoOutput(FALSE),
    m_fWinApp(FALSE),
    m_fAssemble(TRUE),
    m_dwImageBase(0),
    m_dwFileAlign(0),
    m_pNext(NULL)
{
}

////////////////////////////////////////////////////////////////////////////////
// CInputSet::~CInputSet

CInputSet::~CInputSet ()
{
    if (m_pController != NULL)
        m_pController->Release();
}

////////////////////////////////////////////////////////////////////////////////
// CInputSet::Initialize

HRESULT CInputSet::Initialize (CController *pController)
{
    m_pController = pController;
    pController->AddRef();      // Keep a ref on the controller...

    m_tableSources.SetNameTable (pController->GetNameMgr());
    m_tableResources.SetNameTable (pController->GetNameMgr());
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// CInputSet::GetCompiler

STDMETHODIMP CInputSet::GetCompiler (ICSCompiler **ppCompiler)
{
    ASSERT (m_pController != NULL);

    *ppCompiler = m_pController;
    m_pController->AddRef();
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// CInputSet::AddSourceFile

STDMETHODIMP CInputSet::AddSourceFile (PCWSTR pszFileName, FILETIME *pFT)
{
    HRESULT hr;
    DWORD_PTR dwCookie;

    if (wcslen(pszFileName) >= MAX_PATH)
        return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);

    // See if this source file is already there
    if (FAILED (hr = m_tableSources.FindOrAddNoCase (pszFileName, &dwCookie)))
        return hr;

    if (hr == S_OK)
    {
        // Was already there.  Update the file time if we were given one.
        if (pFT != NULL)
        {
            CSourceFileInfo *pSrc = m_tableSources.GetData (dwCookie);
            pSrc->m_ft = *pFT;
        }

        return S_FALSE;
    }

    // Create a new source info object to store in the table...
    CSourceFileInfo *pSrcInfo = new CSourceFileInfo();
    if (pSrcInfo == NULL)
        return E_OUTOFMEMORY;
    if (FAILED (hr = m_tableSources.AddName (pszFileName, &pSrcInfo->m_pName)))
        return hr;
    if (pFT != NULL)
        pSrcInfo->m_ft = *pFT;
    m_tableSources.SetData (dwCookie, pSrcInfo);

    // Add it into the linked list to preserve ordering
    pSrcInfo->m_pNext = m_pSrcHead;
    m_pSrcHead = pSrcInfo;

    // If we are generating an output (as far as we know) and don't have a name
    // for it established yet, do so now.
    if (!m_fNoOutput && (m_sbstrOutputName == NULL))
    {
        if (m_fDLL)
        {
            PWSTR   psz = (PWSTR)_alloca ((wcslen (pszFileName) + 11) * sizeof (WCHAR));
            PWSTR   pszExt;
            PWSTR   pszDir;

            wcscpy (psz, pszFileName);

            // Find the extension start. Last '.', as long as that isn't followed
            // by a slash or backslash.
            pszExt = wcsrchr (psz, L'.');
            if (pszExt == NULL || wcschr (pszExt, L'/') != NULL || wcschr (pszExt, L'\\') != NULL)
            {
                // No current extension; put new extension at end.
                pszExt = psz + wcslen (psz);
            }

            // Place the new extension.
            wcscpy (pszExt, m_fAssemble ? L".dll" : L".netmodule");

            // Now chop of the directory name
            pszDir = wcsrchr (psz, L'\\');
            if (pszDir != NULL) {
                psz = pszDir + 1;
            }
            pszDir = wcsrchr (psz, L'/');
            if (pszDir != NULL) {
                psz = pszDir + 1;
            }

            m_sbstrOutputName.Attach (SysAllocString (psz));
            if (m_sbstrOutputName == NULL)
                return E_OUTOFMEMORY;
        }
        else
        {
            m_sbstrOutputName.Attach (SysAllocString (L"?"));
            if (m_sbstrOutputName == NULL)
                return E_OUTOFMEMORY;
        }
    }

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// CInputSet::RemoveSourceFile

STDMETHODIMP CInputSet::RemoveSourceFile (PCWSTR pszFileName)
{
    DWORD_PTR dwCookie = 0;
    // See if this source file is already there
    if (FAILED (m_tableSources.FindNoCase (pszFileName, &dwCookie)))
        return S_FALSE;

    ASSERT(m_pSrcHead != NULL);
    CSourceFileInfo *pSrc = m_tableSources.GetData (dwCookie);
    
#ifdef _DEBUG
    bool bFound = false;
#endif // _DEBUG
    for (CSourceFileInfo **ppCurrent = &m_pSrcHead;
         *ppCurrent != NULL; ppCurrent = &(*ppCurrent)->m_pNext) {
        if (*ppCurrent == pSrc) {
            // Remove it from the list
            *ppCurrent = pSrc->m_pNext;
            pSrc->m_pNext = NULL;
#ifdef _DEBUG
            bFound = true;
#endif // _DEBUG
            break;
        }
    }
    ASSERT(bFound == true);

    m_tableSources.Remove (dwCookie);
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// CInputSet::CopySources

void CInputSet::CopySources(CSourceFileInfo **ppSrcFiles)
{
    CSourceFileInfo* pCurrent = m_pSrcHead;
    long i = m_tableSources.Count();
    // Put the sources in the array backwards because
    // Th list has them linked backwards
    while (i > 0 && pCurrent != NULL)
        pCurrent = (ppSrcFiles[--i] = pCurrent)->m_pNext;
    ASSERT(pCurrent == NULL && i == 0);
}

////////////////////////////////////////////////////////////////////////////////
// CInputSet::RemoveAllSourceFiles

STDMETHODIMP CInputSet::RemoveAllSourceFiles ()
{
    m_tableSources.RemoveAll ();
    m_pSrcHead = NULL;
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// CInputSet::AddResourceFile

STDMETHODIMP CInputSet::AddResourceFile (PCWSTR pszFileName, PCWSTR pszIdent, BOOL bEmbed, BOOL bVis)
{
    LPWSTR  szResource;
    HRESULT hr;
    NAME    *pResource;
    unsigned chFileName = (unsigned)wcslen(pszFileName);
    unsigned chIdent = (unsigned)wcslen(pszIdent);

    if (chFileName >= MAX_PATH)
        return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
    
    szResource = (LPWSTR)_alloca(sizeof(WCHAR) * (chFileName + chIdent + 25));
    swprintf(szResource, L"%.10u%.10u%s%s%c%c", chFileName, chIdent, pszFileName, pszIdent, bEmbed ? L'T' : L'F', bVis ? L'T' : L'F');
    if (SUCCEEDED (hr = m_tableResources.AddName (szResource, &pResource)))
        hr = m_tableResources.Add ( pszIdent, pResource);

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
// CInputSet::RemoveResourceFile

STDMETHODIMP CInputSet::RemoveResourceFile (PCWSTR pszFileName, PCWSTR pszIdent, BOOL bEmbed, BOOL bVis)
{
    return m_tableResources.Remove (pszIdent);
}


////////////////////////////////////////////////////////////////////////////////
// CInputSet::SetOutputFileName

STDMETHODIMP CInputSet::SetOutputFileName (PCWSTR pszFileName)
{
    if (pszFileName != NULL && wcslen(pszFileName) >= MAX_PATH)
        return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
    
    m_sbstrOutputName.Empty();
    if (pszFileName == NULL || pszFileName[0] == 0)
    {
        m_fNoOutput = TRUE;
        return S_OK;
    }

    m_fNoOutput = FALSE;
    m_sbstrOutputName.Attach (SysAllocString (pszFileName));
    return m_sbstrOutputName == NULL ? E_OUTOFMEMORY : S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// CInputSet::SetOutputFileType

STDMETHODIMP CInputSet::SetOutputFileType (DWORD dwFileType)
{
    switch(dwFileType) {
    case OUTPUT_CONSOLE:
        m_fAssemble = TRUE;
        m_fDLL = FALSE;
        m_fWinApp = FALSE;
        break;
    case OUTPUT_WINDOWS:
        m_fAssemble = TRUE;
        m_fDLL = FALSE;
        m_fWinApp = TRUE;
        break;
    case OUTPUT_LIBRARY:
        m_fAssemble = TRUE;
        m_fDLL = TRUE;
        m_fWinApp = FALSE;
        break;
    case OUTPUT_MODULE:
        m_fAssemble = FALSE;
        m_fDLL = TRUE;
        m_fWinApp = FALSE;
        break;
    default:
        return E_INVALIDARG;
    }
    if (m_fDLL == TRUE && m_sbstrMainClass != NULL)
        return S_FALSE;
    return S_OK;
}


////////////////////////////////////////////////////////////////////////////////
// CInputSet::SetImageBase

STDMETHODIMP CInputSet::SetImageBase (DWORD dwImageBase)
{
    m_dwImageBase = dwImageBase;
    return S_OK;
}


////////////////////////////////////////////////////////////////////////////////
// CInputSet::SetFileAlignment

STDMETHODIMP CInputSet::SetFileAlignment (DWORD dwAlign)
{
    DWORD temp = (dwAlign >> 9);
    if (dwAlign < 0x0000200 || dwAlign > 0x0002000)
        return E_INVALIDARG;
    while ((temp & 0x0001) == 0)
        temp >>= 1;
    if (temp != 1)
        return E_INVALIDARG;
    m_dwFileAlign = dwAlign;
    return S_OK;
}


////////////////////////////////////////////////////////////////////////////////
// CInputSet::SetMainClass

STDMETHODIMP CInputSet::SetMainClass (LPCWSTR pszFQClassName)
{
    m_sbstrMainClass.Empty();
    if (pszFQClassName == NULL)
        return S_OK;

    if (m_fDLL)
        return E_INVALIDARG;

    m_sbstrMainClass.Attach (SysAllocString (pszFQClassName));
    return m_sbstrMainClass == NULL ? E_OUTOFMEMORY : S_OK;
}

