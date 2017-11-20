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
#include "histnode.h"
#include "util.h"

CHistoryInfoNode::CHistoryInfoNode()
{
    _dwSig = 0x444f4e48;    /* 'DONH' */
    memset(&_bindHistoryInfo, 0, sizeof(_bindHistoryInfo));
}

CHistoryInfoNode::~CHistoryInfoNode()
{
}

HRESULT CHistoryInfoNode::Init(AsmBindHistoryInfo *pHistInfo)
{
    HRESULT                                      hr = S_OK;
    
    if (!pHistInfo) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    lstrcpyW(_bindHistoryInfo.wzAsmName, pHistInfo->wzAsmName);
    lstrcpyW(_bindHistoryInfo.wzPublicKeyToken, pHistInfo->wzPublicKeyToken);
    lstrcpyW(_bindHistoryInfo.wzCulture, pHistInfo->wzCulture);
    lstrcpyW(_bindHistoryInfo.wzVerReference, pHistInfo->wzVerReference);
    lstrcpyW(_bindHistoryInfo.wzVerAppCfg, pHistInfo->wzVerAppCfg);
    lstrcpyW(_bindHistoryInfo.wzVerPublisherCfg, pHistInfo->wzVerPublisherCfg);
    lstrcpyW(_bindHistoryInfo.wzVerAdminCfg, pHistInfo->wzVerAdminCfg);

Exit:
    return hr;
}

HRESULT CHistoryInfoNode::Create(AsmBindHistoryInfo *pHistInfo, CHistoryInfoNode **pphn)
{
    HRESULT                                     hr = S_OK;
    CHistoryInfoNode                               *pHList = NULL;
   
    if (!pHistInfo || !pphn) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    *pphn = NULL;

    pHList = NEW(CHistoryInfoNode);
    if (!pHList) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    hr = pHList->Init(pHistInfo);
    if (FAILED(hr)) {
        SAFEDELETE(pHList);
        goto Exit;
    }

    *pphn = pHList;

Exit:
    return hr;
}
    

