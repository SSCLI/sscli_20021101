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
#ifndef __INIREAD_H_INCLUDED__
#define __INIREAD_H_INCLUDED__

#include "histinfo.h"

class CIniReader : public IHistoryReader {
    public:
        CIniReader();
        virtual ~CIniReader();

        HRESULT Init(HINI hIni, BOOL fOwned);
        
        // IUnknown methods

        STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
        STDMETHODIMP_(ULONG) AddRef();
        STDMETHODIMP_(ULONG) Release();

        // IHistoryReader methods

        STDMETHODIMP GetFilePath(LPWSTR wzFilePath, DWORD *pdwSize);
        STDMETHODIMP GetApplicationName(LPWSTR wzAppName, DWORD *pdwSize);
        STDMETHODIMP GetEXEModulePath(LPWSTR wzExePath, DWORD *pdwSize);
        
        STDMETHODIMP GetNumActivations(DWORD *pdwNumActivations);
        STDMETHODIMP GetActivationDate(DWORD dwIdx, FILETIME *pftDate);

        STDMETHODIMP GetRunTimeVersion(FILETIME *pftActivationDate,
                                  LPWSTR wzRunTimeVersion, DWORD *pdwSize);
        STDMETHODIMP GetNumAssemblies(FILETIME *pftActivationDate, DWORD *pdwNumAsms);
        STDMETHODIMP GetHistoryAssembly(FILETIME *pftActivationDate, DWORD dwIdx,
                                        IHistoryAssembly **ppHistAsm);

    public: 
        // Other (non IHistoryReader) methods

        STDMETHODIMP GetAssemblyInfo(LPCWSTR wzActivationDate,
                                LPCWSTR wzAssemblyName,
                                LPCWSTR wzPublicKeyToken,
                                LPCWSTR wzCulture,
                                LPCWSTR wzVerReference,
                                AsmBindHistoryInfo *pBindInfo);
        STDMETHODIMP_(BOOL) DoesExist(IHistoryAssembly *pHistAsm);


    private:
        STDMETHODIMP ExtractAssemblyInfo(LPWSTR wzAsmStr, LPWSTR *ppwzAsmName,
                                    LPWSTR *ppwzAsmPublicKeyToken,
                                    LPWSTR *ppwzAsmCulture,
                                    LPWSTR *ppwzAsmVerRef);
    private:
        DWORD                                  _dwSig;
        LPWSTR                                 _wzFilePath;
        LONG                                   _cRef;

        HINI                                   _hIni;
        BOOL                                   _fIniOwned;
};

#endif

