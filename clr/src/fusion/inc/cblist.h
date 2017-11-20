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
#ifndef __CBLIST_H_INCLUDED__
#define __CBLIST_H_INCLUDED__

#include "list.h"

class CCodebaseList : public ICodebaseList
{
    public:
        CCodebaseList();
        virtual ~CCodebaseList();

        // IUnknown methods

        STDMETHODIMP QueryInterface(REFIID riid, void ** ppv);
        STDMETHODIMP_(ULONG) AddRef();
        STDMETHODIMP_(ULONG) Release();

        // ICodebaseList methods

        STDMETHODIMP AddCodebase(LPCWSTR wzCodebase);
        STDMETHODIMP RemoveCodebase(DWORD dwIndex);
        STDMETHODIMP GetCodebase(DWORD dwIndex, LPWSTR wzCodebase, DWORD *pcbCodebase);
        STDMETHODIMP GetCount(DWORD *pdwCount);
        STDMETHODIMP RemoveAll();

    private:
        DWORD                             _dwSig;
        LONG                              _cRef;
        List<LPWSTR>                      _listCodebase;
};

#endif  // __CBLIST_H_INCLUDED__
