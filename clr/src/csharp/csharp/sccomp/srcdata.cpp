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
// File: srcdata.cpp
//
// ===========================================================================

#include "stdafx.h"
#include "srcdata.h"
#include "srcmod.h"

/*
////////////////////////////////////////////////////////////////////////////////
// CSourceData::operator new

void *CSourceData::operator new (size_t size)
{
    return VSAlloc (size);
}

////////////////////////////////////////////////////////////////////////////////
// CSourceData::operator delete

void CSourceData::operator delete (void *pv)
{
    VSFree (pv);
}
*/

////////////////////////////////////////////////////////////////////////////////
// CSourceData::CSourceData

CSourceData::CSourceData (CSourceModule *pModule) :
    m_pModule(pModule),
    m_iRef(0)
{
    pModule->AddDataRef ();
    pModule->AddRef();
}

////////////////////////////////////////////////////////////////////////////////
// CSourceData::~CSourceData

CSourceData::~CSourceData ()
{
    ASSERT (m_pModule != NULL);
    m_pModule->ReleaseDataRef ();
    m_pModule->Release();
}

////////////////////////////////////////////////////////////////////////////////
// CSourceData::CreateInstance

HRESULT CSourceData::CreateInstance (CSourceModule *pModule, ICSSourceData **ppData)
{
    CSourceData *pNew = new CSourceData (pModule);

    if (pNew == NULL)
        return E_OUTOFMEMORY;

    *ppData = pNew;
    pNew->AddRef();
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceData::AddRef

STDMETHODIMP_(ULONG) CSourceData::AddRef ()
{
    return InterlockedIncrement ((LONG *)&m_iRef);
}

////////////////////////////////////////////////////////////////////////////////
// CSourceData::Release

STDMETHODIMP_(ULONG) CSourceData::Release ()
{
    ULONG   iNew = InterlockedDecrement ((LONG *)&m_iRef);
    if (iNew == 0)
        delete this;

    return iNew;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceData::QueryInterface

STDMETHODIMP CSourceData::QueryInterface (REFIID riid, void **ppObj)
{
    *ppObj = NULL;

    if (riid == IID_IUnknown || riid == IID_ICSSourceData)
    {
        *ppObj = (ICSSourceData *)this;
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceData::GetSourceModule

STDMETHODIMP CSourceData::GetSourceModule (ICSSourceModule **ppModule)
{
    *ppModule = m_pModule;
    m_pModule->AddRef();
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceData::GetLexResults

STDMETHODIMP CSourceData::GetLexResults (LEXDATA *pLexData)
{
    return m_pModule->GetLexResults (this, pLexData);
}

////////////////////////////////////////////////////////////////////////////////
// CSourceData::GetSingleTokenData

STDMETHODIMP CSourceData::GetSingleTokenData (long iToken, TOKENDATA *pTokenData)
{
    return m_pModule->GetSingleTokenData (this, iToken, pTokenData);
}

////////////////////////////////////////////////////////////////////////////////
// CSourceData::GetSingleTokenPos

STDMETHODIMP CSourceData::GetSingleTokenPos (long iToken, POSDATA *pposToken)
{
    return m_pModule->GetSingleTokenPos (this, iToken, pposToken);
}

////////////////////////////////////////////////////////////////////////////////
// CSourceData::GetTokenText

STDMETHODIMP CSourceData::GetTokenText (long iTokenId, PCWSTR *ppszText, long *piLength)
{
    return m_pModule->GetTokenText (iTokenId, ppszText, piLength);
}

////////////////////////////////////////////////////////////////////////////////
// CSourceData::IsInsideComment

STDMETHODIMP CSourceData::IsInsideComment (const POSDATA pos, BOOL *pfInComment)
{
    return m_pModule->IsInsideComment (this, pos, pfInComment);
}

////////////////////////////////////////////////////////////////////////////////
// CSourceData::ParseTopLevel

STDMETHODIMP CSourceData::ParseTopLevel (BASENODE **ppTree)
{
    return m_pModule->ParseTopLevel (this, ppTree);
}

////////////////////////////////////////////////////////////////////////////////
// CSourceData::GetErrors

STDMETHODIMP CSourceData::GetErrors (ERRORCATEGORY iCategory, ICSErrorContainer **ppErrors)
{
    return m_pModule->GetErrors (this, iCategory, ppErrors);
}

////////////////////////////////////////////////////////////////////////////////
// CSourceData::GetInteriorParseTree

STDMETHODIMP CSourceData::GetInteriorParseTree (BASENODE *pNode, ICSInteriorTree **ppTree)
{
    return m_pModule->GetInteriorParseTree (this, pNode, ppTree);
}

////////////////////////////////////////////////////////////////////////////////
// CSourceData::LookupNode

STDMETHODIMP CSourceData::LookupNode (NAME *pKey, long iOrdinal, BASENODE **ppNode, long *piGlobalOrdinal)
{
    return m_pModule->LookupNode (this, pKey, iOrdinal, ppNode, piGlobalOrdinal);
}

////////////////////////////////////////////////////////////////////////////////
// CSourceData::GetNodeKeyOrdinal

STDMETHODIMP CSourceData::GetNodeKeyOrdinal (BASENODE *pNode, NAME **ppKey, long *piKeyOrdinal)
{
    return m_pModule->GetNodeKeyOrdinal (this, pNode, ppKey, piKeyOrdinal);
}

////////////////////////////////////////////////////////////////////////////////
// CSourceData::GetGlobalKeyArray

STDMETHODIMP CSourceData::GetGlobalKeyArray (KEYEDNODE *pKeyedNodes, long iSize, long *piCopied)
{
    return m_pModule->GetGlobalKeyArray (this, pKeyedNodes, iSize, piCopied);
}

////////////////////////////////////////////////////////////////////////////////
// CSourceData::ParseForErrors

STDMETHODIMP CSourceData::ParseForErrors ()
{
    return m_pModule->ParseForErrors (this);
}

////////////////////////////////////////////////////////////////////////////////
// CSourceData::FindLeafNode

STDMETHODIMP CSourceData::FindLeafNode (const POSDATA pos, BASENODE **ppNode, ICSInteriorTree **ppTree)
{
    return m_pModule->FindLeafNode (this, pos, ppNode, ppTree);
}

////////////////////////////////////////////////////////////////////////////////
// CSourceData::GetExtent

STDMETHODIMP CSourceData::GetExtentEx (BASENODE *pNode, POSDATA *pposStart, POSDATA *pposEnd, ExtentFlags flags)
{
    return m_pModule->GetExtent (pNode, pposStart, pposEnd, flags);
}

////////////////////////////////////////////////////////////////////////////////
// CSourceData::GetTokenExtent

STDMETHODIMP CSourceData::GetTokenExtent (BASENODE *pNode, long *piFirstToken, long *piLastToken)
{
    return m_pModule->GetTokenExtent (pNode, piFirstToken, piLastToken);
}

////////////////////////////////////////////////////////////////////////////////
// CSourceData::GetDocComment

STDMETHODIMP CSourceData::GetDocComment (BASENODE *pNode, long *piComment, long *piCount)
{
    return m_pModule->GetDocComment (pNode, piComment, piCount);
}

////////////////////////////////////////////////////////////////////////////////
// CSourceData::GetDocCommentPos

STDMETHODIMP CSourceData::GetDocCommentPos (long iComment, POSDATA * pos)
{
    return m_pModule->GetDocCommentPos (iComment, pos);
}

////////////////////////////////////////////////////////////////////////////////
// CSourceData::IsUpToDate

STDMETHODIMP CSourceData::IsUpToDate (BOOL *pfTokenized, BOOL *pfTopParsed)
{
    return m_pModule->IsUpToDate (pfTokenized, pfTopParsed);
}

////////////////////////////////////////////////////////////////////////////////
// CSourceData::ForceUpdate

STDMETHODIMP CSourceData::ForceUpdate ()
{
    return m_pModule->ForceUpdate (this);
}
