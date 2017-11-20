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
#include "fusionp.h"
#include "fuspriv.h"
#include "debmacro.h"
#include "cblist.h"

CCodebaseList::CCodebaseList()
: _cRef(1)
{
    _dwSig = 0x204c4243; /* ' LBC' */
}

CCodebaseList::~CCodebaseList()
{
    RemoveAll();
}

STDMETHODIMP_(ULONG) CCodebaseList::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CCodebaseList::Release()
{
    ULONG                    ulRef = InterlockedDecrement(&_cRef);

    if (!ulRef) {
        delete this;
    }
    
    return ulRef;
}

HRESULT CCodebaseList::QueryInterface(REFIID iid, void **ppvObj)
{
    HRESULT hr = NOERROR;
    *ppvObj = NULL;

    if (iid == IID_IUnknown  || iid == IID_ICodebaseList) {
        *ppvObj = static_cast<ICodebaseList *>(this);
    } 
    else {
        hr = E_NOINTERFACE;
    }

    if (*ppvObj) {
        AddRef();
    }

    return hr;
}    


HRESULT CCodebaseList::AddCodebase(LPCWSTR wzCodebase)
{
    HRESULT                            hr = S_OK;
    LPWSTR                             wzCodebaseCopy = NULL;
    DWORD                              dwLen;
    
    if (!wzCodebase) {
        hr = E_INVALIDARG;
        goto Exit;
    }
    

    dwLen = lstrlenW(wzCodebase) + 1;

    wzCodebaseCopy = NEW(WCHAR[dwLen]);
    if (!wzCodebaseCopy) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    lstrcpyW(wzCodebaseCopy, wzCodebase);
    
    _listCodebase.AddTail(wzCodebaseCopy);

Exit:
    return hr;
}

HRESULT CCodebaseList::RemoveCodebase(DWORD dwIndex)
{
    HRESULT                              hr = S_OK;
    LISTNODE                             pos = NULL;
    LISTNODE                             oldpos = NULL;
    LPWSTR                               wzCodebase = NULL;
    DWORD                                dwNumCodebases = 0;
    DWORD                                i;
    

    dwNumCodebases = _listCodebase.GetCount();

    if (dwIndex > dwNumCodebases) {
        hr = E_INVALIDARG;
        goto Exit;
    }
    
    if (!dwNumCodebases) {
        hr = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
        goto Exit;
    }
    
    pos = _listCodebase.GetHeadPosition();

    ASSERT(pos);

    for (i = 0; i < dwNumCodebases; i++) {
        oldpos = pos;
        wzCodebase = _listCodebase.GetNext(pos);
        ASSERT(wzCodebase);

        if (i == dwIndex) {
            delete [] wzCodebase;
            _listCodebase.RemoveAt(oldpos);
            break;
        }
    }
        
Exit:
    return hr;
}

HRESULT CCodebaseList::GetCodebase(DWORD dwIndex, LPWSTR wzCodebase,
                                   DWORD *pcbCodebase)
{
    HRESULT                              hr = E_UNEXPECTED;
    LISTNODE                             pos = NULL;
    LISTNODE                             oldpos = NULL;
    LPWSTR                               wzCurCodebase = NULL;
    DWORD                                dwNumCodebases;
    DWORD                                i;
    DWORD                                dwCodebaseLen;

    dwNumCodebases = _listCodebase.GetCount();

    if (dwIndex > dwNumCodebases || !pcbCodebase) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    if (!dwNumCodebases) {
        hr = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
        goto Exit;
    }

    pos = _listCodebase.GetHeadPosition();

    ASSERT(pos);

    for (i = 0; i < dwNumCodebases; i++) {
        oldpos = pos;
        wzCurCodebase = _listCodebase.GetNext(pos);
        ASSERT(wzCurCodebase);

        if (i == dwIndex) {
            dwCodebaseLen = lstrlenW(wzCurCodebase) + 1;

            if (!wzCodebase || (*pcbCodebase < dwCodebaseLen)) {
                hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            }
            else {
                lstrcpyW(wzCodebase, wzCurCodebase);
                hr = S_OK;
            }
            *pcbCodebase = dwCodebaseLen;
            break;
        }
    }

Exit:
    return hr;
}

HRESULT CCodebaseList::RemoveAll()
{
    LISTNODE                      pos = NULL;
    LPWSTR                        wzCodebase = NULL;

    pos = _listCodebase.GetHeadPosition();

    while (pos) {
        wzCodebase = _listCodebase.GetNext(pos);
        ASSERT(wzCodebase);

        delete [] wzCodebase;
    }

    _listCodebase.RemoveAll();

    return S_OK;
}

HRESULT CCodebaseList::GetCount(DWORD *pdwCount)
{
    HRESULT                           hr = S_OK;

    if (!pdwCount) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    *pdwCount = _listCodebase.GetCount();

Exit:
    return hr;
}

