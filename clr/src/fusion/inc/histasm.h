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
#ifndef __HISTASM_H_INCLUDED__
#define __HISTASM_H_INCLUDED__

#include "histinfo.h"

class CIniReader;

class CHistoryAssembly : public IHistoryAssembly {
    public:
        CHistoryAssembly();
        virtual ~CHistoryAssembly();

        static HRESULT Create(CIniReader* pIniReader, LPCWSTR pwzActivationDate,
                              LPCWSTR wzAsmName, LPCWSTR wzPublicKeyToken,
                              LPCWSTR wzCulture, LPCWSTR wzVerRef,
                              CHistoryAssembly **ppHistAsm);
        
        // IUnknown methods

        STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
        STDMETHODIMP_(ULONG) AddRef();
        STDMETHODIMP_(ULONG) Release();

        // IHistoryAssembly methods

        STDMETHODIMP GetAssemblyName(LPWSTR wzAsmName, DWORD *pdwSize);
        STDMETHODIMP GetPublicKeyToken(LPWSTR wzPublicKeyToken, DWORD *pdwSize);
        STDMETHODIMP GetCulture(LPWSTR wzCulture, DWORD *pdwSize);
        STDMETHODIMP GetReferenceVersion(LPWSTR wzVerRef, DWORD *pdwSize);
        STDMETHODIMP GetActivationDate(LPWSTR wzActivationDate, DWORD *pdwSize);

        STDMETHODIMP GetAppCfgVersion(LPWSTR pwzVerAppCfg, DWORD *pdwSize);
        STDMETHODIMP GetPublisherCfgVersion(LPWSTR pwzVerPublisherCfg, DWORD *pdwSize);
        STDMETHODIMP GetAdminCfgVersion(LPWSTR pwzAdminCfg, DWORD *pdwSize);

    private:
        HRESULT Init(CIniReader* pIniReader, LPCWSTR pwzActivationDate, LPCWSTR pwzAsmName,
                     LPCWSTR pwzPublicKeyToken, LPCWSTR pwzCulture, LPCWSTR pwzVerRef);

    private:
        DWORD                                  _dwSig;
        LONG                                   _cRef;
        LPWSTR                                 _pwzActivationDate;
        LPWSTR                                 _pwzAsmName;
        LPWSTR                                 _pwzPublicKeyToken;
        LPWSTR                                 _pwzCulture;
        LPWSTR                                 _pwzVerReference;
        AsmBindHistoryInfo                     _bindInfo;
};

#endif

