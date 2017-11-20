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
#ifndef __POLICY_H_INCLUDED__
#define __POLICY_H_INCLUDED__


#include "histinfo.h"

#define POLICY_ASSEMBLY_PREFIX                   L"policy."

class CDebugLog;
class CApplicationContext;
class CNodeFactory;


HRESULT GetPolicyVersion(LPCWSTR wzCfgFile, LPCWSTR wzAssemblyName,
                         LPCWSTR wzPublicKeyToken, LPCWSTR wzVersionIn,
                         LPWSTR *ppwzVersionOut, BOOL *pbSafeMode,
                         CDebugLog *pdbglog);

HRESULT PrepQueryMatchData(IAssemblyName *pName, LPWSTR *ppwzAsmName, LPWSTR *ppwzAsmVersion,
                           LPWSTR *ppwzPublicKeyToken, LPWSTR *ppwzLanguage);

HRESULT GetPublisherPolicyFilePath(LPCWSTR pwzAsmName, LPCWSTR pwzPublicKeyToken,
                                   LPCWSTR pwzCulture, WORD wVerMajor,
                                   WORD wVerMinor, LPWSTR *ppwzPublisherCfg);

HRESULT GetCodebaseHint(LPCWSTR wzCfgFilePath, LPCWSTR wzAsmName,
                        LPCWSTR wzVersion, LPCWSTR wzPublicKeyToken,
                        LPWSTR *ppwzCodebase, CDebugLog *pdbglog);

HRESULT GetGlobalSafeMode(LPCWSTR wzCfgFilePath, BOOL *pbSafeMode);

HRESULT ParseXML(CNodeFactory **ppNodeFactory, LPCWSTR wzFileName, CDebugLog *pdbglog);
HRESULT ApplyPolicy(LPCWSTR wzHostCfg, LPCWSTR wzAppCfg, IAssemblyName *pNameSource, IAssemblyName **ppNamePolicy,
                    LPWSTR *ppwzPolicyCodebase, IApplicationContext *pAppCtx,
                    AsmBindHistoryInfo *pHistInfo, BOOL bDisallowApplyPublisherPolicy,
                    CDebugLog *pdbglog);
HRESULT ReadConfigSettings(IApplicationContext *pAppCtx, LPCWSTR wzAppCfg, CDebugLog *pdbglog);
HRESULT GetVersionFromString(LPCWSTR wzStr, ULONGLONG *pullVer);
BOOL IsMatchingVersion(LPCWSTR wzVerCfg, LPCWSTR wzVerSource);

#endif 

