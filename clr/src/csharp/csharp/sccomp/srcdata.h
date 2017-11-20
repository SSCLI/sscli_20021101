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
// File: srcdata.h
//
// ===========================================================================

#ifndef __srcdata_h__
#define __srcdata_h__

#include "scciface.h"
#include "tokinfo.h"
#include "alloc.h"
#include "nodes.h"

class CSourceModule;

////////////////////////////////////////////////////////////////////////////////
// CSourceData

class CSourceData : public ICSSourceData
{
private:
    CSourceModule       *m_pModule;         // NOTE:  This is a ref'd pointer!
    long                m_iRef;

    CSourceData (CSourceModule *pModule);
    ~CSourceData ();

public:
    //void    *operator new (size_t size);
    //void    operator delete (void *pv);

    static  HRESULT CreateInstance (CSourceModule *pModule, ICSSourceData **ppData);

    CSourceModule   *GetModule() { return m_pModule; }

    // IUnknown
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    STDMETHOD(QueryInterface)(REFIID riid, void **ppObj);

    // ICSSourceData
    STDMETHOD(GetSourceModule)(ICSSourceModule **ppModule);
    STDMETHOD(GetLexResults)(LEXDATA *pLexData);
    STDMETHOD(GetSingleTokenPos)(long iToken, POSDATA *pposToken);
    STDMETHOD(GetSingleTokenData)(long iToken, TOKENDATA *pTokenData);
    STDMETHOD(GetTokenText)(long iTokenId, PCWSTR *ppszText, long *piLength);
    STDMETHOD(IsInsideComment)(const POSDATA pos, BOOL *pfInComment);
    STDMETHOD(ParseTopLevel)(BASENODE **ppTree);
    STDMETHOD(GetErrors)(ERRORCATEGORY iCategory, ICSErrorContainer **ppErrors);
    STDMETHOD(GetInteriorParseTree)(BASENODE *pNode, ICSInteriorTree **ppTree);
    STDMETHOD(LookupNode)(NAME *pKey, long iOrdinal, BASENODE **ppNode, long *piGlobalOrdinal);
    STDMETHOD(GetNodeKeyOrdinal)(BASENODE *pNode, NAME **ppKey, long *piKeyOrdinal);
    STDMETHOD(GetGlobalKeyArray)(KEYEDNODE *pKeyedNodes, long iSize, long *piCopied);
    STDMETHOD(ParseForErrors)();
    STDMETHOD(FindLeafNode)(const POSDATA pos, BASENODE **ppNode, ICSInteriorTree **ppTree);
    STDMETHOD(GetExtent)(BASENODE *pNode, POSDATA *pposStart, POSDATA *pposEnd) { return GetExtentEx( pNode, pposStart, pposEnd, EF_ALL); };
    STDMETHOD(GetTokenExtent)(BASENODE *pNode, long *piFirstToken, long *piLastToken);
    STDMETHOD(GetDocComment)(BASENODE *pNode, long *piComment, long *piCount);
    STDMETHOD(GetDocCommentPos)(long iComment, POSDATA *loc);
    STDMETHOD(IsUpToDate)(BOOL *pfTokenized, BOOL *pfTopParsed);
    STDMETHOD(ForceUpdate)();
    STDMETHOD(GetExtentEx)(BASENODE *pNode, POSDATA *pposStart, POSDATA *pposEnd, ExtentFlags flags);
};

#endif //__srcdata_h__

