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
// EEConfigFactory.h -
//
// Parses XML files and adding runtime entries to the EEConfig list
//
//

#ifndef EECONFIGFACTORY_H
#define EECONFIGFACTORY_H

#include <xmlparser.h>
#include <objbase.h>
#include "unknwn.h"
#include "../xmlparser/_reference.h"
#include "../xmlparser/_unknown.h"
#include "eehash.h"

#define CONFIG_KEY_SIZE 128

class EEConfigFactory : public _unknown<IXMLNodeFactory, &IID_IXMLNodeFactory>
{

public:
    EEConfigFactory(EEUnicodeStringHashTable* pTable, LPCWSTR);
    ~EEConfigFactory();
    HRESULT STDMETHODCALLTYPE NotifyEvent( 
            /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
            /* [in] */ XML_NODEFACTORY_EVENT iEvt);

    HRESULT STDMETHODCALLTYPE BeginChildren( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ XML_NODE_INFO* __RPC_FAR pNodeInfo);

    HRESULT STDMETHODCALLTYPE EndChildren( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ BOOL fEmptyNode,
        /* [in] */ XML_NODE_INFO* __RPC_FAR pNodeInfo);
    
    HRESULT STDMETHODCALLTYPE Error( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ HRESULT hrErrorCode,
        /* [in] */ USHORT cNumRecs,
        /* [in] */ XML_NODE_INFO* __RPC_FAR * __RPC_FAR apNodeInfo)
    {
      /* 
         UNUSED(pSource);
         UNUSED(hrErrorCode);
         UNUSED(cNumRecs);
         UNUSED(apNodeInfo);
      */
        return hrErrorCode;
    }
    
    HRESULT STDMETHODCALLTYPE CreateNode( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ PVOID pNodeParent,
        /* [in] */ USHORT cNumRecs,
        /* [in] */ XML_NODE_INFO* __RPC_FAR * __RPC_FAR apNodeInfo);

private:

    HRESULT GrowKey(DWORD dwSize)
    {
        if(dwSize > m_dwSize) {
            DeleteKey();
            m_pLastKey = new WCHAR[dwSize];
            if(m_pLastKey == NULL) return E_OUTOFMEMORY;
            m_dwSize = dwSize;
        }
        return S_OK;
    }

    void ClearKey()
    {
        m_dwLastKey = 0;
    }

    void DeleteKey()
    {
        if(m_pLastKey != NULL && m_pLastKey != m_pBuffer)
            delete [] m_pLastKey;
        m_dwSize = 0;
        m_dwLastKey = 0;
    }

    HRESULT CopyToKey(LPWSTR pString, DWORD dwString)
    {
        dwString++; // add in the null
        HRESULT hr = GrowKey(dwString);
        if(FAILED(hr)) return hr;
        wcsncpy(m_pLastKey, pString, dwString);
        m_dwLastKey = dwString;
        return S_OK;
    }
        
    HRESULT CopyVersion(LPCWSTR version, DWORD dwVersion);

    EEUnicodeStringHashTable* m_pTable;
    BOOL    m_fRuntime;
    BOOL    m_fVersionedRuntime;
    BOOL    m_fDeveloperSettings;

    LPCWSTR m_pVersion;
    LPWSTR  m_pLastKey;
    DWORD   m_dwLastKey;

    WCHAR   m_pBuffer[CONFIG_KEY_SIZE];
    DWORD   m_dwSize;

    DWORD   m_dwDepth;
};

#endif
