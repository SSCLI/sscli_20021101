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
//*****************************************************************************
// XMLReader.cpp
// 
//*****************************************************************************
//
// Lite weight xmlreader  
//

#include "stdafx.h"
#include <mscoree.h>
#include <xmlparser.hpp>
#include <objbase.h>
#include <mscorcfg.h>
#include "xmlreader.h"

#define ISWHITE(ch) ((ch) >= 0x09 && (ch) <= 0x0D || (ch) == 0x20)

class ShimFactory : public _unknown<IXMLNodeFactory, &IID_IXMLNodeFactory>
{

public:
    ShimFactory();
    ~ShimFactory();
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
    
    WCHAR* ReleaseVersion()
    {
        WCHAR* version = pVersion;
        pVersion = NULL;
        return version;
    }

    WCHAR* ReleaseImageVersion()
    {
        WCHAR* version = pImageVersion;
        pImageVersion = NULL;
        return version;
    }

    BOOL IsSafeMode(){
        return bIsSafeMode;
    }

    HRESULT STDMETHODCALLTYPE CreateNode( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ PVOID pNodeParent,
        /* [in] */ USHORT cNumRecs,
        /* [in] */ XML_NODE_INFO* __RPC_FAR * __RPC_FAR apNodeInfo);

private:
    HRESULT CopyVersion(LPCWSTR version, DWORD dwVersion);
    HRESULT CopyImageVersion(LPCWSTR version, DWORD dwVersion);
    HRESULT GetSafeMode(LPCWSTR version);

    WCHAR* pVersion;
    WCHAR* pImageVersion;
    BOOL   bIsSafeMode;
    BOOL   fConfiguration;
    BOOL   fStartup;
    BOOL   fRequiredVersion;
    DWORD  nLevel;
};

ShimFactory::ShimFactory() 
{
    pVersion = NULL;
    pImageVersion = NULL;
    bIsSafeMode = FALSE;
    fConfiguration=FALSE;
    fStartup = FALSE;
    fRequiredVersion = FALSE;
    nLevel=0;
}

ShimFactory::~ShimFactory() 
{
    if(pVersion != NULL) {
        delete [] pVersion;
    }
    if(pImageVersion != NULL) {
        delete [] pImageVersion;
    }
}

HRESULT ShimFactory::CopyVersion(LPCWSTR version, DWORD dwVersion)
{
    if (pVersion)
        delete[] pVersion;
    pVersion = new WCHAR[dwVersion+1];
    if(pVersion == NULL) return E_OUTOFMEMORY;
    wcsncpy(pVersion, version, dwVersion);
    pVersion[dwVersion]=L'\0';
    return S_OK;
}

HRESULT ShimFactory::CopyImageVersion(LPCWSTR imageVersion, DWORD dwImageVersion)
{
    if (pImageVersion)
        delete[] pImageVersion;
    pImageVersion = new WCHAR[dwImageVersion+1];
    if(pImageVersion == NULL) return E_OUTOFMEMORY;
    wcsncpy(pImageVersion, imageVersion, dwImageVersion);
    pImageVersion[dwImageVersion]=L'\0';
    return S_OK;
}


HRESULT ShimFactory::GetSafeMode(LPCWSTR val)
{
    if (wcscmp(val, L"true") == 0)
        bIsSafeMode = TRUE;
    else 
        bIsSafeMode = FALSE;
    return S_OK;
}


HRESULT STDMETHODCALLTYPE ShimFactory::NotifyEvent( 
            /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
            /* [in] */ XML_NODEFACTORY_EVENT iEvt)
{

    UNUSED(pSource);
    UNUSED(iEvt);
    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ShimFactory::BeginChildren( 
    /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
    /* [in] */ XML_NODE_INFO __RPC_FAR *pNodeInfo)
{
    UNUSED(pSource);
    UNUSED(pNodeInfo); 

    if (nLevel==1&&wcscmp(pNodeInfo->pwcText, L"configuration") == 0) 
        fConfiguration = TRUE;
    if (nLevel==2&&fConfiguration&&wcscmp(pNodeInfo->pwcText, L"startup") == 0) 
        fStartup = TRUE;

    return S_OK;

}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ShimFactory::EndChildren( 
    /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
    /* [in] */ BOOL fEmptyNode,
    /* [in] */ XML_NODE_INFO __RPC_FAR *pNodeInfo)
{
    UNUSED(pSource);
    UNUSED(fEmptyNode);
    UNUSED(pNodeInfo);
    if (pNodeInfo->dwType == XML_ELEMENT)
        nLevel--;
    if ( fEmptyNode ) { 

    }
    else {
        if (fStartup && wcscmp(pNodeInfo->pwcText, L"startup") == 0) {
            fStartup = FALSE;
            fRequiredVersion = FALSE;
            fConfiguration=FALSE;
            return STARTUP_FOUND;
        }
    }
    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE ShimFactory::CreateNode( 
    /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
    /* [in] */ PVOID pNode,
    /* [in] */ USHORT cNumRecs,
    /* [in] */ XML_NODE_INFO* __RPC_FAR * __RPC_FAR apNodeInfo)
{
    if (apNodeInfo[0]->dwType == XML_ELEMENT)  
        nLevel++;

    if(fStartup == FALSE)
        return S_OK;

    HRESULT hr = S_OK;
    WCHAR  wstr[128]; 
    DWORD  dwSize = 128;
    WCHAR* pString = wstr;
    DWORD  i; 
    DWORD  fAttribute=0;
    const int fVersion = 1;
    const int fImageVersion = 2;
    const int fSafeMode = 3;

    UNUSED(pSource);
    UNUSED(pNode);
    UNUSED(apNodeInfo);
    UNUSED(cNumRecs);

    for( i = 0; i < cNumRecs; i++) { 
        if(apNodeInfo[i]->dwType == XML_ELEMENT ||
           apNodeInfo[i]->dwType == XML_ATTRIBUTE ||
           apNodeInfo[i]->dwType == XML_PCDATA) {

            DWORD lgth = apNodeInfo[i]->ulLen;
            WCHAR *ptr = (WCHAR*) apNodeInfo[i]->pwcText;
            // Trim the value
            for(;*ptr && ISWHITE(*ptr); ptr++, lgth--);
            for(lgth--; lgth > 0 && ISWHITE(ptr[lgth]); lgth--);
            lgth++;

            if ( (lgth+1) > dwSize ) {
                dwSize = lgth+1;
                pString = (WCHAR*) alloca(sizeof(WCHAR) * dwSize);
            }
            
            wcsncpy(pString, ptr, lgth);
            pString[lgth] = L'\0';
            
            switch(apNodeInfo[i]->dwType) {
            case XML_ELEMENT : 
                if(wcscmp(pString, L"requiredRuntime") == 0)
                    fRequiredVersion = TRUE;
                else 
                    fRequiredVersion = FALSE;
                break ;     
            case XML_ATTRIBUTE : 
                fAttribute=0;
                if(fRequiredVersion && wcscmp(pString, L"version") == 0)
                {
                    fAttribute = fVersion;
                    break;
                }

                if(fRequiredVersion && wcscmp(pString, L"imageVersion") == 0)
                {
                    fAttribute = fImageVersion;
                    break;
                }

                if(fRequiredVersion && wcscmp(pString, L"safemode") == 0)
                {
                    fAttribute=fSafeMode;
                    break;
                }

                break;
            case XML_PCDATA:
                if((fRequiredVersion && cNumRecs == 1) || fAttribute == (DWORD) fVersion)
                    hr = CopyVersion(pString, lgth);
                else {
                    switch(fAttribute) {
                    case fSafeMode:
                        hr = GetSafeMode(pString);
                        break;
                    case fImageVersion:
                        hr = CopyImageVersion(pString, lgth);
                        break;
                    default:
                        break;
                    }
                }
                break ;     
            default: 
                ;
            } // end of switch
        }
    }

    return hr;  
}

LPWSTR GetShimInfoFromConfigFile(LPCWSTR name, LPCWSTR wszFileName){
    _ASSERTE(name);
    return NULL; 
}

STDAPI GetXMLElement(LPCWSTR wszFileName, LPCWSTR pwszTag) {
    return E_FAIL;
}

STDAPI GetXMLElementAttribute(LPCWSTR pwszAttributeName, LPWSTR pbuffer, DWORD cchBuffer, DWORD* dwLen){
    return E_FAIL;
}

STDAPI GetXMLObject(IXMLParser **ppv)
{

    if(ppv == NULL) return E_POINTER;
    IXMLParser *pParser = new XMLParser();
    if(pParser == NULL) return E_OUTOFMEMORY;

    pParser->AddRef();
    *ppv = pParser;
    return S_OK;
}


HRESULT XMLGetVersionFromStream(IStream* pStream, 
                                DWORD reserved, LPWSTR* pVersion, LPWSTR* pImageVersion, 
                                LPWSTR* pBuildFlavor, BOOL* bSafeMode)
{
    if(pVersion == NULL) return E_POINTER;

    HRESULT        hr = S_OK;  
    IXMLParser     *pIXMLParser = NULL;
    ShimFactory    *factory = NULL; 

    hr = GetXMLObject(&pIXMLParser);
    if(FAILED(hr)) goto Exit;

    factory = new ShimFactory();
    if ( ! factory) { 

        hr = E_OUTOFMEMORY; 
        goto Exit; 
    }
    factory->AddRef(); // RefCount = 1 

    
    hr = pIXMLParser->SetInput(pStream); // filestream's +1
    if ( ! SUCCEEDED(hr)) 
        goto Exit;

    hr = pIXMLParser->SetFactory(factory); // factory's RefCount=2
    if ( ! SUCCEEDED(hr)) 
        goto Exit;

    hr = pIXMLParser->Run(-1);
    if(SUCCEEDED(hr) || hr == STARTUP_FOUND) {
        if (pVersion)
            *pVersion  = factory->ReleaseVersion();
        if (bSafeMode)
            *bSafeMode = factory->IsSafeMode();
        if (pBuildFlavor)
            *pBuildFlavor = NULL;
        if(pImageVersion)
            *pImageVersion = factory->ReleaseImageVersion();

        hr = S_OK;
    }
Exit:  

    if (hr == (HRESULT) XML_E_MISSINGROOT || hr == (HRESULT) E_PENDING)
        hr=S_OK;

    if (pIXMLParser) { 
        pIXMLParser->Release();
        pIXMLParser= NULL ; 
    }
    if ( factory) {
        factory->Release();
        factory=NULL;
    }
    return hr;  
}


HRESULT XMLGetVersion(PCWSTR filename, LPWSTR* pVersion, LPWSTR* pImageVersion, LPWSTR* pBuildFlavor, BOOL* bSafeMode)
{
    if(pVersion == NULL) return E_POINTER;

    HRESULT        hr = S_OK;  
    IStream        *pFile = NULL;

    hr = CreateConfigStream(filename, &pFile);
    if(FAILED(hr)) goto Exit;

    hr = XMLGetVersionFromStream(pFile, 0, pVersion, pImageVersion, pBuildFlavor, bSafeMode);
Exit:  
    if ( pFile) {
        pFile->Release();
        pFile=NULL;
    }

    return hr;  
}

