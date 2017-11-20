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
// File: factory.cpp
//
// ===========================================================================

#include "stdafx.h"
#include "factory.h"
#include "namemgr.h"
#include "controller.h"
#include "compiler.h"
#include "error.h"


////////////////////////////////////////////////////////////////////////////////
// CFactory::CFactory

CFactory::CFactory()
{
    // Make sure the message DLL is loaded at this time. If this fails,
    // we'll detect it in CreateCompiler and return an HRESULT.
    GetMessageDll();
}

////////////////////////////////////////////////////////////////////////////////
// CFactory::CreateNameTable

STDMETHODIMP CFactory::CreateNameTable (ICSNameTable **ppTable)
{
    NAMEMGR *pMgr = new NAMEMGR();
    if (pMgr == NULL)
        return E_OUTOFMEMORY;

    HRESULT     hr = pMgr->Init();

    if (FAILED (hr))
    {
        delete pMgr;
        return hr;
    }

    *ppTable = pMgr;
    pMgr->AddRef();

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// CFactory::CreateCompiler

STDMETHODIMP CFactory::CreateCompiler (DWORD dwFlags, ICSCompilerHost *pHost, ICSNameTable *pNameTable, ICSCompiler **ppCompiler)
{
    HRESULT                 hr;
    CComPtr<ICSNameTable>   spNameTable;

    if (!hModuleMessages)
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);      // We couldn't load the message DLL.

    // Create a name table if we weren't given one
    if (pNameTable == NULL)
    {
        if (FAILED (hr = CreateNameTable (&spNameTable)))
            return hr;
        pNameTable = spNameTable;
    }

    CComObject<CController>     *pObj;

    // Create a compiler controller object.  It's the one that exposes ICSCompiler,
    // and manages objects whose lifespan extend beyond a single compilation.
    if (SUCCEEDED (hr = CComObject<CController>::CreateInstance (&pObj)))
    {
        if (FAILED (hr = pObj->Initialize (dwFlags, pHost, pNameTable)) ||
            FAILED (hr = pObj->QueryInterface (IID_ICSCompiler, (void **)ppCompiler)))
        {
            delete pObj;
        }
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
// CFactory::CreateLexer

STDMETHODIMP CFactory::CreateLexer (ICSNameTable *pNameTable, ICSLexer **ppLexer)
{
    return CTextLexer::CreateInstance (pNameTable, ppLexer);
}

////////////////////////////////////////////////////////////////////////////////
// CollectFileVersionInfo

HRESULT CollectFileVersionInfo (HINSTANCE hInst, BSTR *pbstrFileName, BSTR *pbstrVersion)
{
    if (pbstrFileName != NULL)
        *pbstrFileName = NULL;

    if (pbstrVersion != NULL)
        *pbstrVersion = NULL;
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// CFactory::CheckVersion

STDMETHODIMP CFactory::CheckVersion (HINSTANCE hInstance, BSTR *pbstrError)
{
	_ASSERTE(!"NYI");
	return S_OK;
}

// Update the registry.
LPCOLESTR CFactory::pszRegRoot = NULL;

