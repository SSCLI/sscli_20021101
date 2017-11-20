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
#include "xmlparser.h"
#include "list.h"
#include "cfgdata.h"
#include "nodefact.h"
#include "util.h"
#include "dbglog.h"
#include "policy.h"
#include "helpers.h"
#include "naming.h"

CNodeFactory::CNodeFactory(CDebugLog *pdbglog)
: _cRef(1)
, _dwState(PSTATE_LOOKUP_CONFIGURATION)
, _dwCurDepth(0)
, _bGlobalSafeMode(FALSE)
, _pwzPrivatePath(NULL)
, _pdbglog(pdbglog)
, _pAsmInfo(NULL)
{
    _dwSig = 0x54434146; /* 'TCAF' */

    if (_pdbglog) {
        _pdbglog->AddRef();
    }
}

CNodeFactory::~CNodeFactory()
{
    LISTNODE                           pos = NULL;
    CAsmBindingInfo                   *pAsmInfo = NULL;
    CQualifyAssembly                  *pqa = NULL;

    SAFEDELETEARRAY(_pwzPrivatePath);
    SAFERELEASE(_pdbglog);

    pos = _listAsmInfo.GetHeadPosition();
    while (pos) {
        pAsmInfo = _listAsmInfo.GetNext(pos);
        SAFEDELETE(pAsmInfo);
    }

    _listAsmInfo.RemoveAll();

    pos = _listQualifyAssembly.GetHeadPosition();
    while (pos) {
        pqa = _listQualifyAssembly.GetNext(pos);
        SAFEDELETE(pqa);
    }

    _listQualifyAssembly.RemoveAll();

    SAFEDELETE(_pAsmInfo);
}

HRESULT CNodeFactory::QueryInterface(REFIID riid, void **ppv)
{
    HRESULT                                    hr = S_OK;

    *ppv = NULL;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IXMLNodeFactory)) {
        *ppv = static_cast<IXMLNodeFactory *>(this);
    }
    else {
        hr = E_NOINTERFACE;
    }

    if (*ppv) {
        AddRef();
    }

    return hr;
}

STDMETHODIMP_(ULONG) CNodeFactory::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CNodeFactory::Release()
{
    ULONG                    ulRef = InterlockedDecrement(&_cRef);

    if (!ulRef) {
        delete this;
    }

    return ulRef;
}


HRESULT CNodeFactory::NotifyEvent(IXMLNodeSource *pSource,
                                  XML_NODEFACTORY_EVENT iEvt)
{
    return S_OK;
}

HRESULT CNodeFactory::BeginChildren(IXMLNodeSource *pSource,
                                    XML_NODE_INFO *pNodeInfo)
{
    return S_OK;
}

HRESULT CNodeFactory::EndChildren(IXMLNodeSource *pSource, BOOL fEmpty,
                                  XML_NODE_INFO *pNodeInfo)
{
    if (pNodeInfo->dwType == XML_ELEMENT) {
        _dwCurDepth--;
    }

    _nsmgr.OnEndChildren();

    // Unwind state

    if (_dwState == PSTATE_CONFIGURATION && _dwCurDepth < XML_CONFIGURATION_DEPTH) {
        _dwState = PSTATE_LOOKUP_CONFIGURATION;
    }
    else if (_dwState == PSTATE_RUNTIME && _dwCurDepth < XML_RUNTIME_DEPTH) {
        _dwState = PSTATE_CONFIGURATION;
    }
    else if (_dwState == PSTATE_ASSEMBLYBINDING && _dwCurDepth < XML_ASSEMBLYBINDING_DEPTH) {
        _dwState = PSTATE_RUNTIME;
    }
    else if (_dwState == PSTATE_DEPENDENTASSEMBLY && _dwCurDepth < XML_DEPENDENTASSEMBLY_DEPTH) {
        _dwState = PSTATE_ASSEMBLYBINDING;

        ASSERT(_pAsmInfo);

        // Add assembly information if valid

        if (_pAsmInfo->_pwzName) {
            if (!_pAsmInfo->_pwzPublicKeyToken && (_pAsmInfo->_listBindingRedir).GetCount()) {
                LISTNODE                              pos;
                CBindingRedir                        *pRedir;

                DEBUGOUT(_pdbglog, 1, ID_FUSLOG_XML_PRIVATE_ASM_REDIRECT);
                
                pos = (_pAsmInfo->_listBindingRedir).GetHeadPosition();
                while (pos) {
                    pRedir = (_pAsmInfo->_listBindingRedir).GetNext(pos);
                    SAFEDELETE(pRedir);
                }
                
                (_pAsmInfo->_listBindingRedir).RemoveAll();
            }

            _listAsmInfo.AddTail(_pAsmInfo);
            _pAsmInfo = NULL;
        }
        else {
            SAFEDELETE(_pAsmInfo);
        }
    }

    return S_OK;
}

HRESULT CNodeFactory::Error(IXMLNodeSource *pSource, HRESULT hrErrorCode,
                            USHORT cNumRecs, XML_NODE_INFO __RPC_FAR **aNodeInfo)
{
    DEBUGOUT1(_pdbglog, 1, ID_FUSLOG_XML_PARSE_ERROR_CODE, hrErrorCode);

    _nsmgr.Error(hrErrorCode);

    return S_OK;
}

HRESULT CNodeFactory::CreateNode(IXMLNodeSource __RPC_FAR *pSource,
                                 PVOID pNodeParent, USHORT cNumRecs,
                                 XML_NODE_INFO __RPC_FAR **aNodeInfo)
{
    HRESULT                                 hr = S_OK;
    LPWSTR                                  pwzElementNameNS = NULL;
    CCodebaseHint                          *pCodebaseInfo = NULL;
    CBindingRedir                          *pRedirInfo = NULL;

    hr = _nsmgr.OnCreateNode(pSource, pNodeParent, cNumRecs, aNodeInfo);
    if (FAILED(hr)) {
        goto Exit;
    }

    if (aNodeInfo[0]->dwType == XML_ELEMENT) {
        _dwCurDepth++;

        hr = ApplyNamespace(aNodeInfo[0], &pwzElementNameNS, XMLNS_FLAGS_APPLY_DEFAULT_NAMESPACE);
        if (FAILED(hr)) {
            goto Exit;
        }

        switch (_dwState) {
            case PSTATE_LOOKUP_CONFIGURATION:
                // Still looking for <configuration> tag

                if ((_dwCurDepth == XML_CONFIGURATION_DEPTH) &&
                    (!lstrcmpW(pwzElementNameNS, POLICY_TAG_CONFIGURATION))) {

                    _dwState = PSTATE_CONFIGURATION;
                }

                break;

           case PSTATE_CONFIGURATION:
                // In <configuration> tag, and looking for <runtime> tag

                if ((_dwCurDepth == XML_RUNTIME_DEPTH) &&
                    (!lstrcmpW(pwzElementNameNS, POLICY_TAG_RUNTIME))) {

                    _dwState = PSTATE_RUNTIME;
                }

                break;

            case PSTATE_RUNTIME:
                 // In <runtime> tag, and looking for <assemblyBinding> tag

                 if ((_dwCurDepth == XML_ASSEMBLYBINDING_DEPTH) &&
                     (!lstrcmpW(pwzElementNameNS, POLICY_TAG_ASSEMBLYBINDING))) {

                     _dwState = PSTATE_ASSEMBLYBINDING;
                 }

                 break;

             case PSTATE_ASSEMBLYBINDING:
                 // In <assemblyBinding> tag.

                 if ((_dwCurDepth == XML_PROBING_DEPTH) &&
                     (!lstrcmpW(pwzElementNameNS, POLICY_TAG_PROBING))) {

                     hr = ProcessProbingTag(aNodeInfo, cNumRecs);
                     if (FAILED(hr)) {
                         goto Exit;
                     }
                 }
                 else if ((_dwCurDepth == XML_QUALIFYASSEMBLY_DEPTH) &&
                          (!lstrcmpW(pwzElementNameNS, POLICY_TAG_QUALIFYASSEMBLY))) {
                     hr = ProcessQualifyAssemblyTag(aNodeInfo, cNumRecs);
                     if (FAILED(hr)) {
                         goto Exit;
                     }
                 }
                 else if ((_dwCurDepth == XML_GLOBAL_PUBLISHERPOLICY_DEPTH) &&
                          (!lstrcmpW(pwzElementNameNS, POLICY_TAG_PUBLISHERPOLICY))) {

                     hr = ProcessPublisherPolicyTag(aNodeInfo, cNumRecs, TRUE);
                     if (FAILED(hr)) {
                         goto Exit;
                     }
                 }
                 else if ((_dwCurDepth == XML_DEPENDENTASSEMBLY_DEPTH) &&
                          (!lstrcmpW(pwzElementNameNS, POLICY_TAG_DEPENDENTASSEMBLY))) {

                     // Create new assembly info struct

                     ASSERT(!_pAsmInfo);

                     _pAsmInfo = NEW(CAsmBindingInfo);
                     if (!_pAsmInfo) {
                         hr = E_OUTOFMEMORY;
                         goto Exit;
                     }

                     // Transition state

                     _dwState = PSTATE_DEPENDENTASSEMBLY;
                 }

                 break;

             case PSTATE_DEPENDENTASSEMBLY:
                 // In <dependentAssembly> tag.

                 if ((_dwCurDepth == XML_ASSEMBLYIDENTITY_DEPTH) &&
                     (!lstrcmpW(pwzElementNameNS, POLICY_TAG_ASSEMBLYIDENTITY))) {

                     hr = ProcessAssemblyIdentityTag(aNodeInfo, cNumRecs);
                     if (hr == HRESULT_FROM_WIN32(ERROR_TAG_NOT_PRESENT)) {
                         DEBUGOUT(_pdbglog, 1, ID_FUSLOG_XML_ASSEMBLYIDENTITY_MISSING_NAME);
                         hr = S_OK;
                     }
                     else if (FAILED(hr)) {
                         goto Exit;
                     }
                 }
                 else if ((_dwCurDepth == XML_BINDINGREDIRECT_DEPTH) &&
                          (!lstrcmpW(pwzElementNameNS, POLICY_TAG_BINDINGREDIRECT))) {

                     pRedirInfo = NEW(CBindingRedir);
                     if (!pRedirInfo) {
                         hr = E_OUTOFMEMORY;
                         goto Exit;
                     }
                    
                     hr = ProcessBindingRedirectTag(aNodeInfo, cNumRecs, pRedirInfo);
                     if (hr == S_OK) {
                         (_pAsmInfo->_listBindingRedir).AddTail(pRedirInfo);
                         pRedirInfo = NULL;
                     }
                     else if (hr == HRESULT_FROM_WIN32(ERROR_TAG_NOT_PRESENT)) {
                         SAFEDELETE(pRedirInfo);
                         DEBUGOUT(_pdbglog, 1, ID_FUSLOG_XML_BINDINGREDIRECT_INSUFFICIENT_DATA);
                         hr = S_OK;
                     }
                     else if (FAILED(hr)) {
                         SAFEDELETE(pRedirInfo);
                         goto Exit;
                     }
                 }
                 else if ((_dwCurDepth == XML_CODEBASE_DEPTH) &&
                          (!lstrcmpW(pwzElementNameNS, POLICY_TAG_CODEBASE))) {

                     pCodebaseInfo = NEW(CCodebaseHint);
                     if (!pCodebaseInfo) {
                         hr = E_OUTOFMEMORY;
                         goto Exit;
                     }

                     hr = ProcessCodebaseTag(aNodeInfo, cNumRecs, pCodebaseInfo);
                     if (hr == S_OK) {
                         (_pAsmInfo->_listCodebase).AddTail(pCodebaseInfo);
                         pCodebaseInfo = NULL;
                     }
                     else if (hr == HRESULT_FROM_WIN32(ERROR_TAG_NOT_PRESENT)) {
                         DEBUGOUT(_pdbglog, 1, ID_FUSLOG_XML_CODEBASE_HREF_MISSING);
                         SAFEDELETE(pCodebaseInfo);
                         hr = S_OK;
                     }
                     else if (FAILED(hr)) {
                         SAFEDELETE(pCodebaseInfo);
                         goto Exit;
                     }
                 }
                 else if ((_dwCurDepth == XML_PUBLISHERPOLICY_DEPTH) &&
                          (!lstrcmpW(pwzElementNameNS, POLICY_TAG_PUBLISHERPOLICY))) {

                     hr = ProcessPublisherPolicyTag(aNodeInfo, cNumRecs, FALSE);
                     if (FAILED(hr)) {
                         goto Exit;
                     }
                 }

                 break;

           default:
                // Unknown state!

                ASSERT(0);

                break;
        }
    }

Exit:
    SAFEDELETEARRAY(pwzElementNameNS);

    return hr;
}

HRESULT CNodeFactory::ProcessProbingTag(XML_NODE_INFO **aNodeInfo, USHORT cNumRecs)
{
    HRESULT                                            hr = S_OK;
    USHORT                                             idx = 1;
    LPWSTR                                             pwzAttributeNS = NULL;

    ASSERT(aNodeInfo && cNumRecs);

    while (idx < cNumRecs) {
        if (aNodeInfo[idx]->dwType == XML_ATTRIBUTE) {
            // Found an attribute. Find out which one, and extract the data.
            // Node: ::ExtractXMLAttribute increments idx.

            hr = ApplyNamespace(aNodeInfo[idx], &pwzAttributeNS, 0);
            if (FAILED(hr)) {
                goto Exit;
            }

            if (!lstrcmpW(pwzAttributeNS, XML_ATTRIBUTE_PRIVATEPATH)) {
                SAFEDELETEARRAY(pwzAttributeNS);
                
                if (_pwzPrivatePath) {
                    DEBUGOUT(_pdbglog, 1, ID_FUSLOG_PRIVATE_PATH_DUPLICATE);
                    idx++;
                }
                else {
                    hr = ::ExtractXMLAttribute(&_pwzPrivatePath, aNodeInfo, &idx, cNumRecs);
                    if (FAILED(hr)) {
                        goto Exit;
                    }
                }
            }
            else {
                SAFEDELETEARRAY(pwzAttributeNS);
                idx++;
            }
        }
        else {
            idx++;
        }
    }

Exit:
    return hr;
}

HRESULT CNodeFactory::ProcessQualifyAssemblyTag(XML_NODE_INFO **aNodeInfo, USHORT cNumRecs)
{
    HRESULT                                            hr = S_OK;
    USHORT                                             idx = 1;
    LPWSTR                                             pwzAttributeNS = NULL;
    LPWSTR                                             pwzPartialName = NULL;
    LPWSTR                                             pwzFullName = NULL;
    CQualifyAssembly                                  *pqa = NULL;
    IAssemblyName                                     *pNameFull = NULL;
    IAssemblyName                                     *pNamePartial = NULL;
    IAssemblyName                                     *pNameQualified = NULL;
    LPWSTR                              wzCanonicalDisplayName=NULL;

    ASSERT(aNodeInfo && cNumRecs);

    while (idx < cNumRecs) {
        if (aNodeInfo[idx]->dwType == XML_ATTRIBUTE) {
            // Found an attribute. Find out which one, and extract the data.
            // Node: ::ExtractXMLAttribute increments idx.

            hr = ApplyNamespace(aNodeInfo[idx], &pwzAttributeNS, 0);
            if (FAILED(hr)) {
                goto Exit;
            }

            if (!lstrcmpW(pwzAttributeNS, XML_ATTRIBUTE_PARTIALNAME)) {
                SAFEDELETEARRAY(pwzAttributeNS);

                if (pwzPartialName) {
                    // Ignore duplicate attribute
                    idx++;
                }
                else {
                    hr = ::ExtractXMLAttribute(&pwzPartialName, aNodeInfo, &idx, cNumRecs);
                    if (FAILED(hr)) {
                        goto Exit;
                    }
                }
            }
            else if (!lstrcmpW(pwzAttributeNS, XML_ATTRIBUTE_FULLNAME)) {
                SAFEDELETEARRAY(pwzAttributeNS);

                if (pwzFullName) {
                    // Ignore duplicate attribute
                    idx++;
                }
                else {
                    hr = ::ExtractXMLAttribute(&pwzFullName, aNodeInfo, &idx, cNumRecs);
                    if (FAILED(hr)) {
                        goto Exit;
                    }
                }

            }
            else {
                SAFEDELETEARRAY(pwzAttributeNS);
                idx++;
            }
        }
        else {
            idx++;
        }
    }

    if (pwzPartialName && pwzFullName) {
        DWORD                               dwSize;
        DWORD                               adwProperties[] = { ASM_NAME_NAME,
                                                                ASM_NAME_MAJOR_VERSION,
                                                                ASM_NAME_MINOR_VERSION,
                                                                ASM_NAME_BUILD_NUMBER,
                                                                ASM_NAME_REVISION_NUMBER,
                                                                ASM_NAME_CULTURE,
                                                                ASM_NAME_PUBLIC_KEY_TOKEN };
        DWORD                               adwCmpFlags[] = { ASM_CMPF_NAME,
                                                              ASM_CMPF_MAJOR_VERSION,
                                                              ASM_CMPF_MINOR_VERSION,
                                                              ASM_CMPF_BUILD_NUMBER,
                                                              ASM_CMPF_REVISION_NUMBER,
                                                              ASM_CMPF_CULTURE,
                                                              ASM_CMPF_PUBLIC_KEY_TOKEN };
        DWORD                               dwNumProps = sizeof(adwProperties) / sizeof(adwProperties[0]);

        if (FAILED(CreateAssemblyNameObject(&pNameFull, pwzFullName,
                                            CANOF_PARSE_DISPLAY_NAME, 0))) {
            goto Exit;
        }

        if (FAILED(CreateAssemblyNameObject(&pNamePartial, pwzPartialName,
                                            CANOF_PARSE_DISPLAY_NAME, 0))) {
            goto Exit;
        }

        // Check validity of qualification

        if (CAssemblyName::IsPartial(pNameFull) || !CAssemblyName::IsPartial(pNamePartial)) {
            goto Exit;
        }

        if (FAILED(pNamePartial->Clone(&pNameQualified))) {
            goto Exit;
        }

        for (DWORD i = 0; i < dwNumProps; i++) {
            dwSize = 0;
            if (pNamePartial->GetProperty(adwProperties[i], NULL, &dwSize) != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
                // Partial on this field. Set pNameQualified's corresponding
                // property to whatever is in pNameFull.

                dwSize = 0;
                pNameFull->GetProperty(adwProperties[i], NULL, &dwSize);
                if (!dwSize) {
                    goto Exit;
                }
                else {
                    BYTE                       *pBuf;

                    pBuf = NEW(BYTE[dwSize]);
                    if (!pBuf) {
                        hr = E_OUTOFMEMORY;
                        goto Exit;
                    }

                    if (FAILED(pNameFull->GetProperty(adwProperties[i], pBuf, &dwSize))) {
                        SAFEDELETEARRAY(pBuf);
                        goto Exit;
                    }

                    if (FAILED(pNameQualified->SetProperty(adwProperties[i], pBuf, dwSize))) {
                        SAFEDELETEARRAY(pBuf);
                        goto Exit;
                    }

                    SAFEDELETEARRAY(pBuf);
                }
            }
            else {
                // Full-specified on this field. Make sure it matches the full ref specified.

                if (pNamePartial->IsEqual(pNameFull, adwCmpFlags[i]) != S_OK) {
                    goto Exit;
                }
            }
        }

        if (CAssemblyName::IsPartial(pNameQualified)) {
            goto Exit;
        }

        // Get canonical display name format

        wzCanonicalDisplayName = NEW(WCHAR[MAX_URL_LENGTH+1]);
        if (!wzCanonicalDisplayName)
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        dwSize = MAX_URL_LENGTH;
        if (FAILED(pNamePartial->GetDisplayName(wzCanonicalDisplayName, &dwSize, 0))) {
            goto Exit;
        }

        // Add qualified assembly entry to list

        pqa = new CQualifyAssembly;
        if (!pqa) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        pqa->_pwzPartialName = WSTRDupDynamic(wzCanonicalDisplayName);
        if (!pqa->_pwzPartialName) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        pqa->_pNameFull = pNameQualified;
        pNameQualified->AddRef();

        _listQualifyAssembly.AddTail(pqa);
    }

Exit:
    SAFEDELETEARRAY(pwzPartialName);
    SAFEDELETEARRAY(pwzFullName);

    SAFERELEASE(pNameFull);
    SAFERELEASE(pNamePartial);
    SAFERELEASE(pNameQualified);

    SAFEDELETEARRAY(wzCanonicalDisplayName);
    return hr;
}


HRESULT CNodeFactory::ProcessBindingRedirectTag(XML_NODE_INFO **aNodeInfo,
                                                USHORT cNumRecs,
                                                CBindingRedir *pRedir)
{
    HRESULT                                            hr = S_OK;
    LPWSTR                                             pwzAttributeNS = NULL;
    USHORT                                             idx = 1;

    ASSERT(aNodeInfo && cNumRecs && pRedir);
    ASSERT(!pRedir->_pwzVersionOld && !pRedir->_pwzVersionNew);

    while (idx < cNumRecs) {
        if (aNodeInfo[idx]->dwType == XML_ATTRIBUTE) {
            // Found an attribute. Find out which one, and extract the data.
            // Note: ::ExtractXMLAttribute increments idx.

            hr = ApplyNamespace(aNodeInfo[idx], &pwzAttributeNS, 0);
            if (FAILED(hr)) {
                goto Exit;
            }

            if (!lstrcmpW(pwzAttributeNS, XML_ATTRIBUTE_OLDVERSION)) {

                hr = ::ExtractXMLAttribute(&(pRedir->_pwzVersionOld), aNodeInfo, &idx, cNumRecs);
                if (FAILED(hr)) {
                    SAFEDELETEARRAY(pwzAttributeNS);
                    goto Exit;
                }
            }
            else if (!lstrcmpW(pwzAttributeNS, XML_ATTRIBUTE_NEWVERSION)) {

                hr = ::ExtractXMLAttribute(&(pRedir->_pwzVersionNew), aNodeInfo, &idx, cNumRecs);
                if (FAILED(hr)) {
                    SAFEDELETEARRAY(pwzAttributeNS);
                    goto Exit;
                }
            }
            else {
                idx++;
            }
            
            SAFEDELETEARRAY(pwzAttributeNS);
        }
        else {
            idx++;
        }
    }

    if (!pRedir->_pwzVersionOld || !pRedir->_pwzVersionNew) {
        // Data was incomplete. These are required fields.

        hr = HRESULT_FROM_WIN32(ERROR_TAG_NOT_PRESENT);
        goto Exit;
    }

Exit:
    return hr;    
}

HRESULT CNodeFactory::ProcessCodebaseTag(XML_NODE_INFO **aNodeInfo,
                                         USHORT cNumRecs,
                                         CCodebaseHint *pCB)
{
    HRESULT                                            hr = S_OK;
    LPWSTR                                             pwzAttributeNS = NULL;
    USHORT                                             idx = 1;

    ASSERT(aNodeInfo && cNumRecs && pCB);
    ASSERT(!pCB->_pwzVersion && !pCB->_pwzCodebase);

    while (idx < cNumRecs) {
        if (aNodeInfo[idx]->dwType == XML_ATTRIBUTE) {
            // Found an attribute. Find out which one, and extract the data.
            // Note: ::ExtractXMLAttribute increments idx.

            hr = ApplyNamespace(aNodeInfo[idx], &pwzAttributeNS, 0);
            if (FAILED(hr)) {
                goto Exit;
            }

            if (!lstrcmpW(pwzAttributeNS, XML_ATTRIBUTE_VERSION)) {

                hr = ::ExtractXMLAttribute(&(pCB->_pwzVersion), aNodeInfo, &idx, cNumRecs);
                if (FAILED(hr)) {
                    SAFEDELETEARRAY(pwzAttributeNS);
                    goto Exit;
                }
            }
            else if (!lstrcmpW(pwzAttributeNS, XML_ATTRIBUTE_HREF)) {

                hr = ::ExtractXMLAttribute(&(pCB->_pwzCodebase), aNodeInfo, &idx, cNumRecs);
                if (FAILED(hr)) {
                    SAFEDELETEARRAY(pwzAttributeNS);
                    goto Exit;
                }
            }
            else {
                idx++;
            }

            SAFEDELETEARRAY(pwzAttributeNS);
        }
        else {
            idx++;
        }
    }

    if (!pCB->_pwzCodebase) {
        // Data was incomplete. 

        hr = HRESULT_FROM_WIN32(ERROR_TAG_NOT_PRESENT);
        goto Exit;
    }

Exit:
    return hr;    
}

HRESULT CNodeFactory::ProcessPublisherPolicyTag(XML_NODE_INFO **aNodeInfo,
                                                USHORT cNumRecs,
                                                BOOL bGlobal)
{
    HRESULT                                            hr = S_OK;
    LPWSTR                                             pwzAttributeNS = NULL;
    LPWSTR                                             pwzTmp = NULL;
    USHORT                                             idx = 1;

    ASSERT(aNodeInfo && cNumRecs);

    while (idx < cNumRecs) {
        if (aNodeInfo[idx]->dwType == XML_ATTRIBUTE) {
            // Found an attribute. Find out which one, and extract the data.
            // Node: ::ExtractXMLAttribute increments idx.

            hr = ApplyNamespace(aNodeInfo[idx], &pwzAttributeNS, 0);
            if (FAILED(hr)) {
                goto Exit;
            }

            if (!lstrcmpW(pwzAttributeNS, XML_ATTRIBUTE_APPLY)) {
                hr = ::ExtractXMLAttribute(&pwzTmp, aNodeInfo, &idx, cNumRecs);
                if (FAILED(hr)) {
                    SAFEDELETEARRAY(pwzAttributeNS);
                    goto Exit;
                }

                if (!lstrcmpW(pwzTmp, L"no")) {
                    if (bGlobal) {
                        _bGlobalSafeMode = TRUE;
                    }
                    else {
                        _pAsmInfo->_bApplyPublisherPolicy = FALSE;
                    }
                }

                SAFEDELETEARRAY(pwzTmp);
            }
            else {
                idx++;
            }

            SAFEDELETEARRAY(pwzAttributeNS);
        }
        else {
            idx++;
        }
    }

Exit:
    return hr;
}

HRESULT CNodeFactory::ProcessAssemblyIdentityTag(XML_NODE_INFO **aNodeInfo,
                                                 USHORT cNumRecs)
{
    HRESULT                                            hr = S_OK;
    LPWSTR                                             pwzAttributeNS = NULL;
    USHORT                                             idx = 1;

    ASSERT(aNodeInfo && cNumRecs && _pAsmInfo);

    while (idx < cNumRecs) {
        if (aNodeInfo[idx]->dwType == XML_ATTRIBUTE) {
            // Found an attribute. Find out which one, and extract the data.
            // Note: ::ExtractXMLAttribute increments idx.

            hr = ApplyNamespace(aNodeInfo[idx], &pwzAttributeNS, 0);
            if (FAILED(hr)) {
                goto Exit;
            }

            if (!lstrcmpW(pwzAttributeNS, XML_ATTRIBUTE_NAME)) {

                if (!_pAsmInfo->_pwzName) {
                    hr = ::ExtractXMLAttribute(&(_pAsmInfo->_pwzName), aNodeInfo, &idx, cNumRecs);
                    if (FAILED(hr)) {
                        SAFEDELETEARRAY(pwzAttributeNS);
                        goto Exit;
                    }
                }
                else {
                    DEBUGOUT(_pdbglog, 1, ID_FUSLOG_XML_MULTIPLE_IDENTITIES);
                    hr = E_FAIL;
                    goto Exit;
                }
            }
            else if (!lstrcmpW(pwzAttributeNS, XML_ATTRIBUTE_PUBLICKEYTOKEN)) {
                if (!_pAsmInfo->_pwzPublicKeyToken) {
                    hr = ::ExtractXMLAttribute(&(_pAsmInfo->_pwzPublicKeyToken), aNodeInfo, &idx, cNumRecs);
                    if (FAILED(hr)) {
                        SAFEDELETEARRAY(pwzAttributeNS);
                        goto Exit;
                    }
                }
                else {
                    DEBUGOUT(_pdbglog, 1, ID_FUSLOG_XML_MULTIPLE_IDENTITIES);
                    hr = E_FAIL;
                    goto Exit;
                }

            }
            else if (!lstrcmpW(pwzAttributeNS, XML_ATTRIBUTE_CULTURE)) {
                if (!_pAsmInfo->_pwzCulture) {
                    hr = ::ExtractXMLAttribute(&(_pAsmInfo->_pwzCulture), aNodeInfo, &idx, cNumRecs);
                    if (FAILED(hr)) {
                        SAFEDELETEARRAY(pwzAttributeNS);
                        goto Exit;
                    }

                    // Change all synonyms for culture=neutral/"" to unset

                    if (!lstrlenW(_pAsmInfo->_pwzCulture) || !lstrcmpiW(_pAsmInfo->_pwzCulture, CFG_CULTURE_NEUTRAL)) {
                        SAFEDELETEARRAY(_pAsmInfo->_pwzCulture);
                    }
                }
                else {
                    DEBUGOUT(_pdbglog, 1, ID_FUSLOG_XML_MULTIPLE_IDENTITIES);
                    hr = E_FAIL;
                    goto Exit;
                }
            }
            else {
                idx++;
            }

            SAFEDELETEARRAY(pwzAttributeNS);
        }
        else {
            idx++;
        }
    }

    if (!_pAsmInfo->_pwzName) {

        // Data was incomplete. 

        hr = HRESULT_FROM_WIN32(ERROR_TAG_NOT_PRESENT);
        goto Exit;
    }

Exit:
    return hr;    
}

HRESULT CNodeFactory::GetSharedPath(LPWSTR *ppwzSharedPath)
{
    HRESULT                                     hr = S_OK;


    if (!ppwzSharedPath) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    *ppwzSharedPath = NULL;

    hr = S_FALSE;

Exit:
    return hr;
}

HRESULT CNodeFactory::GetPrivatePath(LPWSTR *ppwzPrivatePath)
{
    HRESULT                                     hr = S_OK;

    if (!ppwzPrivatePath) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    *ppwzPrivatePath = NULL;

    if (!_pwzPrivatePath) {
        hr = S_FALSE;
        goto Exit;
    }
    
    *ppwzPrivatePath = WSTRDupDynamic(_pwzPrivatePath);
    if (!*ppwzPrivatePath) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

Exit:
    return hr;
}

        
HRESULT CNodeFactory::GetPolicyVersion(LPCWSTR wzAssemblyName,
                                       LPCWSTR wzPublicKeyToken,
                                       LPCWSTR wzCulture,
                                       LPCWSTR wzVersionIn,
                                       LPWSTR *ppwzVersionOut)
{
    HRESULT                                hr = S_OK;
    LISTNODE                               pos = NULL;
    LISTNODE                               posVer = NULL;
    LPCWSTR                                pwzCultureFormatted = NULL;
    CBindingRedir                         *pRedir = NULL;
    CAsmBindingInfo                       *pAsmInfo = NULL;

    if (!wzAssemblyName || !wzPublicKeyToken || !wzVersionIn || !ppwzVersionOut) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    if (wzCulture && (!lstrcmpiW(wzCulture, CFG_CULTURE_NEUTRAL) || !lstrlenW(wzCulture))) {
        pwzCultureFormatted = NULL;
    }
    else {
        pwzCultureFormatted = wzCulture;
    }

    *ppwzVersionOut = NULL;

    pos = _listAsmInfo.GetHeadPosition();
    while (pos) {
        pAsmInfo = _listAsmInfo.GetNext(pos);
        ASSERT(pAsmInfo);

        if ((!lstrcmpiW(wzAssemblyName, pAsmInfo->_pwzName) &&
            (pAsmInfo->_pwzPublicKeyToken && !lstrcmpiW(wzPublicKeyToken, pAsmInfo->_pwzPublicKeyToken)) &&
            ((pwzCultureFormatted && pAsmInfo->_pwzCulture && !lstrcmpiW(pwzCultureFormatted, pAsmInfo->_pwzCulture)) ||
             (!pwzCultureFormatted && !pAsmInfo->_pwzCulture)))) {

            // Match found. 

            // Look for matching version.

            posVer = (pAsmInfo->_listBindingRedir).GetHeadPosition();
            while (posVer) {
                pRedir = (pAsmInfo->_listBindingRedir).GetNext(posVer);

                if (IsMatchingVersion(pRedir->_pwzVersionOld, wzVersionIn)) {

                    // Match found

                    *ppwzVersionOut = WSTRDupDynamic(pRedir->_pwzVersionNew);
                    if (!*ppwzVersionOut) {
                        hr = E_OUTOFMEMORY;
                        goto Exit;
                    }

                    goto Exit;
                }
            }

            // We could break out of the loop here, but this prevents
            // multiple matches (ie. XML had many identical <dependentAssembly>
            // tags...
        }
    }

    // If we got here, we didn't find a match. Input Version == Output Version

    *ppwzVersionOut = WSTRDupDynamic(wzVersionIn);
    if (!*ppwzVersionOut) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }
    
Exit:
    return hr;
}

HRESULT CNodeFactory::GetSafeMode(LPCWSTR wzAssemblyName, LPCWSTR wzPublicKeyToken,
                                  LPCWSTR wzCulture, LPCWSTR wzVersionIn,
                                  BOOL *pbSafeMode)
{
    HRESULT                                hr = S_OK;
    LISTNODE                               pos = NULL;
    LPCWSTR                                pwzCultureFormatted = NULL;
    CAsmBindingInfo                       *pAsmInfo = NULL;

    if (!wzAssemblyName || !wzPublicKeyToken || !wzVersionIn || !pbSafeMode) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    if (wzCulture && (!lstrcmpiW(wzCulture, CFG_CULTURE_NEUTRAL) || !lstrlenW(wzCulture))) {
        pwzCultureFormatted = NULL;
    }
    else {
        pwzCultureFormatted = wzCulture;
    }
    
    if (_bGlobalSafeMode) {
        // Global safe mode is set

        *pbSafeMode = TRUE;
        goto Exit;
    }

    *pbSafeMode = FALSE;

    // Look for per-assembly safemode. If safe mode is set in any matching
    // section, then safe mode is enabled.

    pos = _listAsmInfo.GetHeadPosition();
    while (pos) {
        pAsmInfo = _listAsmInfo.GetNext(pos);
        ASSERT(pAsmInfo);

        if ((!lstrcmpiW(wzAssemblyName, pAsmInfo->_pwzName) &&
            (pAsmInfo->_pwzPublicKeyToken && !lstrcmpiW(wzPublicKeyToken, pAsmInfo->_pwzPublicKeyToken)) &&
            ((pwzCultureFormatted && pAsmInfo->_pwzCulture && !lstrcmpiW(pwzCultureFormatted, pAsmInfo->_pwzCulture)) ||
             (!pwzCultureFormatted && !pAsmInfo->_pwzCulture)))) {

            // Match found. 

            if (!*pbSafeMode) {
                *pbSafeMode = (pAsmInfo->_bApplyPublisherPolicy == FALSE);
            }
        }
    }

Exit:
    return hr;
}

HRESULT CNodeFactory::GetCodebaseHint(LPCWSTR pwzAsmName, LPCWSTR pwzVersion,
                                      LPCWSTR pwzPublicKeyToken, LPCWSTR pwzCulture,
                                      LPCWSTR pwzAppBase, LPWSTR *ppwzCodebase)
{
    HRESULT                                   hr = S_OK;
    LISTNODE                                  pos = NULL;
    LISTNODE                                  posCB = NULL;
    LPCWSTR                                   pwzCultureFormatted;
    LPWSTR                                    wzCombined=NULL;
    DWORD                                     dwSize;
    CAsmBindingInfo                          *pAsmInfo = NULL;
    CCodebaseHint                            *pCodebase = NULL;

    if (!pwzAsmName || (pwzPublicKeyToken && !pwzVersion) || !ppwzCodebase || !pwzAppBase) {
        hr = E_INVALIDARG;
        goto Exit;
    }
    
    if (pwzCulture && (!lstrcmpiW(pwzCulture, CFG_CULTURE_NEUTRAL) || !lstrlenW(pwzCulture))) {
        pwzCultureFormatted = NULL;
    }
    else {
        pwzCultureFormatted = pwzCulture;
    }

    *ppwzCodebase = NULL;

    wzCombined = NEW(WCHAR[MAX_URL_LENGTH+1]);
    if (!wzCombined)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    pos = _listAsmInfo.GetHeadPosition();
    while (pos) {
        pAsmInfo = _listAsmInfo.GetNext(pos);
        ASSERT(pAsmInfo);

        if ((!lstrcmpiW(pwzAsmName, pAsmInfo->_pwzName)) &&
            ((pwzPublicKeyToken && pAsmInfo->_pwzPublicKeyToken && !lstrcmpiW(pwzPublicKeyToken, pAsmInfo->_pwzPublicKeyToken))
             || (!pwzPublicKeyToken && !pAsmInfo->_pwzPublicKeyToken)) &&
            ((pwzCultureFormatted && pAsmInfo->_pwzCulture && !lstrcmpiW(pwzCultureFormatted, pAsmInfo->_pwzCulture))
             || (!pwzCultureFormatted && !pAsmInfo->_pwzCulture))) {

             // Match found.

            posCB = (pAsmInfo->_listCodebase).GetHeadPosition();
            while (posCB) {
                pCodebase = (pAsmInfo->_listCodebase).GetNext(posCB);
                ASSERT(pCodebase);

                if (!pwzPublicKeyToken) {
                    // No PublicKeyToken, take the first codebase (version ignored).

                    dwSize = MAX_URL_LENGTH;
                    hr = UrlCombineUnescape(pwzAppBase, pCodebase->_pwzCodebase,
                                            wzCombined, &dwSize, 0);
                    if (FAILED(hr)) {
                        goto Exit;
                    }

                    *ppwzCodebase = WSTRDupDynamic(wzCombined);
                    if (!*ppwzCodebase) {
                        hr = E_OUTOFMEMORY;
                        goto Exit;
                    }

                    goto Exit;
                }
                else {
                    // Match version.

                    if (pCodebase->_pwzVersion && !lstrcmpiW(pwzVersion, pCodebase->_pwzVersion)) {
                        // Match found.

                        dwSize = MAX_URL_LENGTH;
                        hr = UrlCombineUnescape(pwzAppBase, pCodebase->_pwzCodebase,
                                                wzCombined, &dwSize, 0);
                        if (FAILED(hr)) {
                            goto Exit;
                        }                                                

                        *ppwzCodebase = WSTRDupDynamic(wzCombined);
                        if (!*ppwzCodebase) {
                            hr = E_OUTOFMEMORY;
                            goto Exit;
                        }

                        goto Exit;
                    }
                }
            }
        }
    }

    // Did not find codebase hint.

    hr = S_FALSE;

Exit:
    SAFEDELETEARRAY(wzCombined);
    return hr;
}

HRESULT CNodeFactory::ApplyNamespace(XML_NODE_INFO *pNodeInfo, LPWSTR *ppwzTokenNS,
                                     DWORD dwFlags) 
{
    HRESULT                                  hr = S_OK;
    LPWSTR                                   pwzToken = NULL;

    ASSERT(pNodeInfo && ppwzTokenNS);

    if (!StrCmpN(pNodeInfo->pwcText, XML_NAMESPACE_TAG, XML_NAMESPACE_TAG_LEN)) {
        hr = S_FALSE;
        goto Exit;
    }

    pwzToken = NEW(WCHAR[pNodeInfo->ulLen + 1]);
    if (!pwzToken) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    StrCpyNW(pwzToken, pNodeInfo->pwcText, pNodeInfo->ulLen + 1);

    hr = _nsmgr.Map(pwzToken, ppwzTokenNS, dwFlags);
    if (FAILED(hr)) {
        goto Exit;
    }

Exit:
    SAFEDELETEARRAY(pwzToken);

    return hr;
}

HRESULT CNodeFactory::QualifyAssembly(LPCWSTR pwzDisplayName, IAssemblyName **ppNameQualified, CDebugLog *pdbglog)
{
    HRESULT                                  hr = S_OK;
    LPWSTR                                   wzDispName=NULL;
    DWORD                                    dwSize;
    CQualifyAssembly                        *pqa;
    LISTNODE                                 pos;

    wzDispName = NEW(WCHAR[MAX_URL_LENGTH+1]);
    if (!wzDispName)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    pos = _listQualifyAssembly.GetHeadPosition();
    while (pos) {
        pqa = _listQualifyAssembly.GetNext(pos);
        if (!lstrcmpW(pwzDisplayName, pqa->_pwzPartialName)) {
            // Found match

            *ppNameQualified = pqa->_pNameFull;
            (*ppNameQualified)->AddRef();

            dwSize = MAX_URL_LENGTH;
            if ((*ppNameQualified)->GetDisplayName(wzDispName, &dwSize, 0) == S_OK) {
                DEBUGOUT1(pdbglog, 0, ID_FUSLOG_QUALIFIED_ASSEMBLY, wzDispName);
            }

            goto Exit;
        }
    }

    // No match found

    hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);

Exit:
    SAFEDELETEARRAY(wzDispName);
    return hr;
}

