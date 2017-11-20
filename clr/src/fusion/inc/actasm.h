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
#ifndef __ACTASM_H_
#define __ACTASM_H_

#include "list.h"
#include "helpers.h"

#define DEPENDENCY_HASH_TABLE_SIZE                     127

class CActivatedAssembly {
    public:
        CActivatedAssembly(IAssembly *pAsm, IAssemblyName *pName);
        ~CActivatedAssembly();

    public:
        IAssembly                            *_pAsm;
        IAssemblyName                        *_pName;
};

class CLoadContext : public IFusionLoadContext {
    public:
        CLoadContext(LOADCTX_TYPE ctxType);
        ~CLoadContext();

        static HRESULT Create(CLoadContext **ppLoadContext, LOADCTX_TYPE ctxType);

        // IUnknown

        STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
        STDMETHODIMP_(ULONG) AddRef();
        STDMETHODIMP_(ULONG) Release();

        // IFusionLoadContext

        STDMETHODIMP_(LOADCTX_TYPE) GetContextType();

        // Other methods

        HRESULT CheckActivated(IAssemblyName *pName, IAssembly **ppAsm);
        HRESULT AddActivation(IAssembly *pAsm, IAssembly **ppAsmActivated);
        HRESULT RemoveActivation(IAssembly *pAsm);
        STDMETHODIMP Lock();
        STDMETHODIMP Unlock();

    private:
        HRESULT Init();

    private:
        LOADCTX_TYPE                          _ctxType;
        CRITICAL_SECTION                      _cs;
        LONG                                  _cRef;
        List<CActivatedAssembly *>            _hashDependencies[DEPENDENCY_HASH_TABLE_SIZE];
};

#endif  // __ACTASM_H_
