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
** Header:  FusionSink.hpp
**
** Purpose: Implements FusionSink
**
** Date:  Oct 26, 1998
**
===========================================================*/
#ifndef _FUSIONSINK_H
#define _FUSIONSINK_H

#include <fusion.h>
#include <fusionpriv.h>
#include "corhlpr.h"

class FusionSink : public IAssemblyBindSink
{
public:

    FusionSink() :
        m_punk(NULL),
        pFusionLog(NULL),
        m_cRef(1),
        m_hEvent(NULL)
    {
        Reset();
    }

    void Reset()
    {
        if(m_punk) {
            m_punk->Release();
            m_punk = NULL;
        }
        m_LastResult = S_OK;
    }

	~FusionSink()
    {
        if(m_hEvent)
            CloseHandle(m_hEvent);
    }

    HRESULT AssemblyResetEvent();
    HRESULT LastResult()
    {
        return m_LastResult;
    }

    STDMETHODIMP QueryInterface(REFIID riid, void **ppInterface);
    ULONG STDMETHODCALLTYPE AddRef(void); 
    ULONG STDMETHODCALLTYPE Release(void);
    
    STDMETHODIMP OnProgress(DWORD dwNotification,
                            HRESULT hrNotification,
                            LPCWSTR szNotification,
                            DWORD dwProgress,
                            DWORD dwProgressMax,
                            IUnknown* punk);

    // Wait on the event.
    virtual HRESULT Wait();

    IUnknown*       m_punk;      // Getting an assembly
    CQuickWSTR*     pFusionLog;

protected:
    HRESULT AssemblyCreateEvent();

    LONG        m_cRef;    // Ref count.
    HANDLE      m_hEvent;  // Event to block thread.
    HRESULT     m_LastResult; // Last notification result
};

EXTERN_GUID(IID_IAssemblyName, 0xcd193bc0, 0xb4bc, 0x11d2, 0x98, 0x33, 0x00, 0xc0, 0x4f, 0xc3, 0x1d, 0x2e);

EXTERN_GUID(IID_IAssembly, 0xff08d7d4, 0x04c2, 0x11d3, 0x94, 0xaa, 0x00, 0xc0, 0x4f, 0xc3, 0x08, 0xff);

EXTERN_GUID(IID_IAssemblyBindSink, 0xaf0bc960, 0x0b9a, 0x11d3, 0x95, 0xca, 0x00, 0xa0, 0x24, 0xa8, 0x5b, 0x51);

EXTERN_GUID(IID_IAssemblyModuleImport, 0xda0cd4b0, 0x1117, 0x11d3, 0x95, 0xca, 0x00, 0xa0, 0x24, 0xa8, 0x5b, 0x51);

EXTERN_GUID(IID_IAssemblyManifestImport, 0xde9a68ba, 0x0fa2, 0x11d3, 0x94, 0xaa, 0x00, 0xc0, 0x4f, 0xc3, 0x08, 0xff);

EXTERN_GUID(IID_IFusionBindLog, 0x67e9f87d, 0x8b8a, 0x4a90, 0x9d, 0x3e, 0x85, 0xed, 0x5b, 0x2d, 0xcc, 0x83);

#endif  // _FUSIONSINK_H
