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
#ifndef __PCYCACHE_H_INCLUDED__
#define __PCYCACHE_H_INCLUDED__

#include "list.h"
#include "histinfo.h"

#define POLICY_CACHE_SIZE                  255

class CPolicyMapping {
    public:
        CPolicyMapping(IAssemblyName *pNameSource, IAssemblyName *pNamePolicy,
                       AsmBindHistoryInfo *pBindHistory);
        virtual ~CPolicyMapping();

        static HRESULT Create(IAssemblyName *pNameRefSource,
                              IAssemblyName *pNameRefPolicy,
                              AsmBindHistoryInfo *pBindHistory,
                              CPolicyMapping **ppMapping);

    public:
        IAssemblyName                         *_pNameRefSource;
        IAssemblyName                         *_pNameRefPolicy;
        AsmBindHistoryInfo                     _bindHistory;
};

class CPolicyCache : public IUnknown {
    public:
        CPolicyCache();
        virtual ~CPolicyCache();
        
        static HRESULT Create(CPolicyCache **ppPolicyCache);

        // IUnknown methods

        STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
        STDMETHODIMP_(ULONG) AddRef();
        STDMETHODIMP_(ULONG) Release();

        // Helpers

        HRESULT InsertPolicy(IAssemblyName *pNameRefSource,
                             IAssemblyName *pNameRefPolicy,
                             AsmBindHistoryInfo *pBindHistory);

        HRESULT LookupPolicy(IAssemblyName *pNameRefSource,
                             IAssemblyName **ppNameRefPolicy,
                             AsmBindHistoryInfo *pBindHistory);

    private:
        HRESULT Init();

    private:
        LONG                                  _cRef;
        BOOL                                  _bInitialized;
        CRITICAL_SECTION                      _cs;
        List<CPolicyMapping *>                _listMappings[POLICY_CACHE_SIZE];

};


HRESULT PreparePolicyCache(IApplicationContext *pAppCtx, CPolicyCache **ppPolicyCache);

#endif

