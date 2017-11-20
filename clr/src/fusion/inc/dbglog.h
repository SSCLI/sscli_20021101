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
#ifndef __DBGLOG_H_INCLUDED__
#define __DBGLOG_H_INCLUDED__

#include "list.h"

// Logging constants and globals


#define REG_VAL_FUSION_LOG_PATH              TEXT("LogPath")
#define REG_VAL_FUSION_LOG_DISABLE           TEXT("DisableLog")
#define REG_VAL_FUSION_LOG_LEVEL             TEXT("LoggingLevel")
#define REG_VAL_FUSION_LOG_FORCE             TEXT("ForceLog")
#define REG_VAL_FUSION_LOG_FAILURES          TEXT("LogFailures")
#define REG_VAL_FUSION_LOG_RESOURCE_BINDS    TEXT("LogResourceBinds")

extern DWORD g_dwDisableLog;
extern DWORD g_dwLogLevel;
extern DWORD g_dwForceLog;

// Debug Output macros (for easy compile-time disable of logging)


#define DEBUGOUT(pdbglog, dwLvl, pszLogMsg)  if (!g_dwDisableLog && pdbglog) { pdbglog->DebugOut(dwLvl, pszLogMsg); }
#define DEBUGOUT1(pdbglog, dwLvl, pszLogMsg, param1) if (!g_dwDisableLog && pdbglog) { pdbglog->DebugOut(dwLvl, pszLogMsg, param1); }
#define DEBUGOUT2(pdbglog, dwLvl, pszLogMsg, param1, param2) if (!g_dwDisableLog && pdbglog) { pdbglog->DebugOut(dwLvl, pszLogMsg, param1, param2); }
#define DEBUGOUT3(pdbglog, dwLvl, pszLogMsg, param1, param2, param3) if (!g_dwDisableLog && pdbglog) { pdbglog->DebugOut(dwLvl, pszLogMsg, param1, param2, param3); }
#define DEBUGOUT4(pdbglog, dwLvl, pszLogMsg, param1, param2, param3, param4) if (!g_dwDisableLog && pdbglog) { pdbglog->DebugOut(dwLvl, pszLogMsg, param1, param2, param3, param4); }
#define DEBUGOUT5(pdbglog, dwLvl, pszLogMsg, param1, param2, param3, param4, param5) if (!g_dwDisableLog && pdbglog) { pdbglog->DebugOut(dwLvl, pszLogMsg, param1, param2, param3, param4, param5); }

#define DUMPDEBUGLOG(pdbglog, dwLvl, hr) if (!g_dwDisableLog && pdbglog) { pdbglog->DumpDebugLog(dwLvl, hr); }


#define MAX_DBG_STR_LEN                 1024
#define MAX_DATE_LEN                    128
#define DEBUG_LOG_HTML_START            L"<html><pre>\n"
#define DEBUG_LOG_HTML_META_LANGUAGE    L"<meta http-equiv=\"Content-Type\" content=\"charset=unicode-1-1-utf-8\">"
#define DEBUG_LOG_HTML_END              L"\n</pre></html>"
#define DEBUG_LOG_NEW_LINE              L"\n"

#define PAD_DIGITS_FOR_STRING(x) (((x) > 9) ? TEXT("") : TEXT("0"))

#undef SAFEDELETE
#define SAFEDELETE(p) if ((p) != NULL) { FUSION_DELETE_SINGLETON((p)); (p) = NULL; };

#undef SAFEDELETEARRAY
#define SAFEDELETEARRAY(p) if ((p) != NULL) { FUSION_DELETE_ARRAY((p)); (p) = NULL; };

class CDebugLogElement {
    public:
        CDebugLogElement(DWORD dwDetailLvl);
        virtual ~CDebugLogElement();

        static HRESULT Create(DWORD dwDetailLvl, LPCWSTR pwzMsg,
                              CDebugLogElement **ppLogElem);
        HRESULT Init(LPCWSTR pwzMsg);


        HRESULT Dump(HANDLE hFile);

    public:
        WCHAR                               *_pszMsg;
        DWORD                                _dwDetailLvl;
};

class CDebugLog : public IFusionBindLog {
    public:
        CDebugLog();
        virtual ~CDebugLog();

        static HRESULT Create(IApplicationContext *pAppCtx, LPCWSTR pwzAsmName,
                              CDebugLog **ppdl);

        // IUnknown methods
        
        STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
        STDMETHODIMP_(ULONG) AddRef();
        STDMETHODIMP_(ULONG) Release();

        // IFusionBindLog methods

        STDMETHODIMP GetResultCode();
        STDMETHODIMP GetBindLog(DWORD dwDetailLevel, LPWSTR pwzDebugLog,
                                DWORD *pcbDebugLog);

        // CDebugLog functions
        
        HRESULT SetAsmName(LPCWSTR pwzAsmName);
        HRESULT SetResultCode(HRESULT hr);
        HRESULT DebugOut(DWORD dwDetailLvl, DWORD dwResId, ...);
        HRESULT LogMessage(DWORD dwDetailLvl, LPCWSTR wzDebugStr, BOOL bPrepend);
        HRESULT DumpDebugLog(DWORD dwDetailLvl, HRESULT hrLog);
        
    private:
        HRESULT CreateLogFile(HANDLE *phFile, LPCWSTR wzFileName,
                              LPCWSTR wzEXEName, HRESULT hrLog);
        HRESULT CloseLogFile(HANDLE *phFile);
        HRESULT Init(IApplicationContext *pAppCtx, LPCWSTR pwzAsmName);

    private:
        List<CDebugLogElement *>                   _listDbgMsg;
        HRESULT                                    _hrResult;
        LONG                                       _cRef;
        DWORD                                      _dwNumEntries;
        LPWSTR                                     _pwzAsmName;
        WCHAR                                      _szLogPath[MAX_PATH];
        LPWSTR                                     _wzEXEName;
        BOOL                                       _bWroteDetails;
};

#endif

