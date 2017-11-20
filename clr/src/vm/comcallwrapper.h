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
/*============================================================
**
** Header:  Com Callable wrapper  classes
**
**                                            
===========================================================*/

#ifndef _COMCALLWRAPPER_H
#define _COMCALLWRAPPER_H


#include "corffi.h"

class ComCallWrapper : public IManagedInstanceWrapper
{
protected:
    LONG            m_cRef;
    OBJECTHANDLE    m_hThis;
    DWORD           m_dwDomainId;
    Context*        m_pContext;

    inline static PVOID GetComCallWrapperVPtr()
    {
        ComCallWrapper boilerplate;
        return (PVOID&)boilerplate;
    }

public:
    ComCallWrapper()
    {
        m_cRef = 1;
        m_hThis = NULL;
        m_dwDomainId = 0;
        m_pContext = NULL;
    }

    ~ComCallWrapper()
    {
        // NICE: A bad things will happen if the ComCallWrapper is called or
        // destroyed after the unload of AppDomain it belongs to.
        // It might be ok: AppDomain unloading is pretty much like DLL unloading,
        // and nobody is surprised that you crash if you call an object
        // in unloaded DLL ... but it would be nice to handle this case gracefully.
        if (m_hThis) {
            DestroyHandle(m_hThis);
        }
    }

    void Init(OBJECTREF* poref)
    {
        Context *pContext = GetExecutionContext(*poref, NULL);
        AppDomain *pDomain = pContext->GetDomain();

        m_hThis = pDomain->CreateHandle(NULL);
        StoreObjectInHandle(m_hThis, *poref);

        m_dwDomainId = pDomain->GetId();
        m_pContext = pContext;
    }

    // IUnknown interface
    STDMETHODIMP QueryInterface(REFIID riid, void **ppInterface);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    // IManagedInstanceWrapper interface implementation
    STDMETHODIMP InvokeByName(LPCWSTR MemberName, INT32 BindingFlags, 
        INT32 ArgCount, VARIANT *ArgList, VARIANT *pRetVal);

    struct InvokeByNameArgs {
        ComCallWrapper* pThis;
        LPCWSTR MemberName;
        INT32 BindingFlags;
        INT32 ArgCount;
        VARIANT *ArgList;
        VARIANT *pRetVal;
    };

    static VOID InvokeByNameCallback(LPVOID ptr);

    // helper to determine the app domain of a created object
    static Context* GetExecutionContext(OBJECTREF pObj, OBJECTREF *pServer);

    // Get wrapper from IP
    inline static ComCallWrapper* GetWrapperFromIP(IUnknown* pUnk)
    {
        _ASSERTE(pUnk != NULL);
        return (ComCallWrapper*)pUnk;
    }

    AppDomain *GetDomainSynchronized();

    static IUnknown* GetComIPFromObjectRef(OBJECTREF* poref);
    static OBJECTREF GetObjectRefFromComIP(IUnknown* pUnk);
};


#endif // _COMCALLWRAPPER_H
