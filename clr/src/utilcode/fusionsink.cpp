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
** Header:  FusionSink.cpp
**
** Purpose: Implements FusionSink, event objects that block 
**          the current thread waiting for an asynchronous load
**          of an assembly to succeed. 
**
** Date:  June 16, 1999
**
===========================================================*/

#include "stdafx.h"
#include <stdlib.h>
#include "utilcode.h"
#include "fusionsink.h"

STDMETHODIMP FusionSink::QueryInterface(REFIID riid, void** ppv)
{
    *ppv = NULL;

    if (riid == IID_IUnknown)
        *ppv = (IUnknown*)this;
    else if (riid == IID_IAssemblyBindSink)   
        *ppv = (IAssemblyBindSink*)this;
    if (*ppv == NULL)
        return E_NOINTERFACE;
    AddRef();   
    return S_OK;
}

STDMETHODIMP FusionSink::OnProgress(DWORD dwNotification,
                                    HRESULT hrNotification,
                                    LPCWSTR szNotification,
                                    DWORD dwProgress,
                                    DWORD dwProgressMax,
                                    IUnknown* punk)
{
    HRESULT hr = S_OK;
    switch(dwNotification)
    {
    case ASM_NOTIFICATION_DONE:
        m_LastResult = hrNotification;
        if(punk && SUCCEEDED(hrNotification)) {
            hr = punk->QueryInterface(IID_IUnknown,
                                      (void**) &m_punk);
        }
        SetEvent(m_hEvent);
        break;
#ifdef _DEBUG
    case ASM_NOTIFICATION_ATTEMPT_NEXT_CODEBASE:
//          if(szNotification)
//              printf("Fusion attempted to load: \"%ws\".\n", 
//                     szNotification);
        break;
#endif
	case ASM_NOTIFICATION_BIND_LOG:
		IFusionBindLog* pLog;
		pLog=NULL;
		PAL_TRY
		{
			if (pFusionLog&&punk)
			{
				hr=punk->QueryInterface(IID_IFusionBindLog,(LPVOID*)&pLog);
				if (SUCCEEDED(hr))
				{
					DWORD dwSize=0;
					hr = pLog->GetBindLog(0,NULL,&dwSize);
					if (hr==HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
					{
						DWORD dwOldLen = (DWORD)pFusionLog->iSize;
						if(SUCCEEDED(pFusionLog->ReSize(dwOldLen+dwSize)))
							pLog->GetBindLog(0,pFusionLog->Ptr()+dwOldLen,&dwSize);
					}
				}
			}
		}
		PAL_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
		}
		PAL_ENDTRY

		if(pLog)
			pLog->Release();
		break;
    default:
        break;
    }
    
    return hr;
}

ULONG FusionSink::AddRef()
{
    return (InterlockedIncrement(&m_cRef));
}

ULONG FusionSink::Release()
{
    ULONG   cRef = InterlockedDecrement(&m_cRef);
    if (!cRef) {
        Reset();
        delete this;
    }
    return (cRef);
}

HRESULT FusionSink::AssemblyResetEvent()
{
    HRESULT hr = AssemblyCreateEvent();
    if(FAILED(hr)) return hr;

    if(!ResetEvent(m_hEvent))
        hr = HRESULT_FROM_WIN32(GetLastError());

    return hr;
}

HRESULT FusionSink::AssemblyCreateEvent()
{
    HRESULT hr = S_OK;
    if(m_hEvent == NULL) {
        // Initialize the event to require manual reset
        // and to initially signaled.
        m_hEvent = WszCreateEvent(NULL, TRUE, TRUE, NULL);
        if(m_hEvent == NULL) {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    return hr;
}

HRESULT FusionSink::Wait()
{
    HRESULT hr = S_OK;
    DWORD   dwReturn;
    dwReturn = WaitForSingleObject(m_hEvent, INFINITE);

    return hr;
}

