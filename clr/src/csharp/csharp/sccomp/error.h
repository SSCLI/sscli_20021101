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
// File: error.h
//
// ===========================================================================

#ifndef __error_h__
#define __error_h__

#include "scciface.h"
#include "posdata.h"

class ERRLOC;

////////////////////////////////////////////////////////////////////////////////
// CError
//
// This object is the implementation of ISCError for all compiler errors,
// including lexer, parser, and compiler errors.

class CError :
    public CComObjectRootMT,
    public ICSError
{
private:
    enum { bufferSize = 4096 };

    struct LOCATION
    {
        BSTR    bstrFile;
        POSDATA posStart;
        POSDATA posEnd;
    };

    ERRORKIND       m_iKind;
    CComBSTR        m_sbstrText;
    short           m_iID;
    short           m_iLocations;
    LOCATION        *m_pLocations;
    LOCATION        *m_pMappedLocations;

public:
    BEGIN_COM_MAP(CError)
        COM_INTERFACE_ENTRY(ICSError)
    END_COM_MAP()

    CError ();
    ~CError();

    HRESULT     Initialize (ERRORKIND iKind, short iErrorID, long iResourceID, va_list args);
    HRESULT     AddLocation (PCWSTR pszFile, const POSDATA *pposStart, const POSDATA *pposEnd, PCWSTR pszMapFile, const POSDATA *pMapStart, const POSDATA *pMapEnd);
    ERRORKIND   Kind() { return m_iKind; }

#ifdef _DEBUG
    PCWSTR      GetText () { return m_sbstrText; }
#endif

    // ICSError
    STDMETHOD(GetErrorInfo)(long *piErrorID, ERRORKIND *pKind, PCWSTR *ppszText);
    STDMETHOD(GetLocationCount)(long *piCount);
    STDMETHOD(GetLocationAt)(long iIndex, PCWSTR *ppszFileName, POSDATA *pposStart, POSDATA *pposEnd);
    STDMETHOD(GetUnmappedLocationAt)(long iIndex, PCWSTR *ppszFileName, POSDATA *pposStart, POSDATA *pposEnd);

};

////////////////////////////////////////////////////////////////////////////////
// CErrorContainer
//
// This is a standard implementation of ICSErrorContainer.  It can either
// accumulate errors in order of addition, or maintain order by line number.
// It also has a range replacement mode, which allows new incoming errors to
// replace any existing ones (used by incremental tokenization).

class CErrorContainer :
    public CComObjectRootMT,
    public ICSErrorContainer
{
private:
    ERRORCATEGORY   m_iCategory;
    DWORD_PTR       m_dwID;
    ICSError        **m_ppErrors;
    long            m_iErrors;
    long            m_iWarnings;
    long            m_iCount;
    CErrorContainer *m_pReplacements;

public:
    BEGIN_COM_MAP(CErrorContainer)
        COM_INTERFACE_ENTRY(ICSErrorContainer)
    END_COM_MAP()

    CErrorContainer();
    ~CErrorContainer();

    static  HRESULT     CreateInstance (ERRORCATEGORY iCategory, DWORD_PTR dwID, CErrorContainer **ppContainer);

    HRESULT     Initialize (ERRORCATEGORY iCategory, DWORD_PTR dwID);
    HRESULT     Clone (CErrorContainer **ppContainer);
    HRESULT     AddError (ICSError *pError);
    HRESULT     BeginReplacement ();
    HRESULT     EndReplacement (const POSDATA &posStart, const POSDATA &posEnd);
    long        Count () { return m_iCount; }
    long        RefCount () { return (long)m_dwRef; }
    void        ReleaseAllErrors ();

    // ICSErrorContainer
    STDMETHOD(GetContainerInfo)(ERRORCATEGORY *pCategory, DWORD_PTR *pdwID);
    STDMETHOD(GetErrorCount)(long *piWarnings, long *piErrors, long *piFatals, long *piTotal);
    STDMETHOD(GetErrorAt)(long iIndex, ICSError **ppError);
};



// Global Utility functions.
extern bool IsValidWarningNumber(int id);
extern bool __cdecl LoadAndFormatMessage(int resid, LPWSTR buffer, int cchMax, ...);
#ifdef DEBUG
extern void CheckErrorMessageInfo(MEMHEAP * heap, bool dumpAll);
#endif //DEBUG

#endif // __error_h__

