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
#include <windows.h>
#include "fusionp.h"
#include "dbglog.h"
#include "helpers.h"
#include "util.h"
#include "lock.h"


extern HSATELLITE g_hSatelliteInst;

static HSATELLITE PAL_GetSatelliteResource()
{
    WCHAR szSatellitePath[MAX_PATH+1];
    WCHAR* pwzFileName;

    lstrcpyW(szSatellitePath, g_FusionDllPath);
    pwzFileName = PathFindFileName(szSatellitePath);
    ASSERT(pwzFileName);
    *pwzFileName = '\0';

    StrCatBuff(szSatellitePath, L"fusion.satellite", MAX_PATH);

    return PAL_LoadSatelliteResourceW(szSatellitePath);
}

int PAL_LoadString(
    UINT uID,
    LPWSTR lpBuffer,
    UINT nBufferMax)
{
    if (g_hSatelliteInst == NULL)
    {
        HSATELLITE hSatelliteInst;
        
        hSatelliteInst = PAL_GetSatelliteResource();

        if (!hSatelliteInst)
            return 0;

        if (InterlockedCompareExchangePointer((LPVOID*)&g_hSatelliteInst, hSatelliteInst, NULL) != NULL)
        {
            // somebody else was faster - free our instance
            PAL_FreeSatelliteResource(hSatelliteInst);
        }
    }

    return PAL_LoadSatelliteStringW(g_hSatelliteInst, uID, lpBuffer, nBufferMax);
}

#define _LoadString(uID, lpBuffer, cchBufferMax) PAL_LoadString(uID, lpBuffer, cchBufferMax)



extern WCHAR g_wzEXEPath[MAX_PATH+1];

extern CRITICAL_SECTION g_csBindLog;
extern DWORD g_dwForceLog;
extern DWORD g_dwLogFailures;


//
// CDebugLogElement Class
//

HRESULT CDebugLogElement::Create(DWORD dwDetailLvl, LPCWSTR pwzMsg,
                                 CDebugLogElement **ppLogElem)
{
    HRESULT                                  hr = S_OK;
    CDebugLogElement                        *pLogElem = NULL;

    if (!ppLogElem || !pwzMsg) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    *ppLogElem = NULL;

    pLogElem = FUSION_NEW_SINGLETON(CDebugLogElement(dwDetailLvl));
    if (!pLogElem) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    hr = pLogElem->Init(pwzMsg);
    if (FAILED(hr)) {
        SAFEDELETE(pLogElem);
        goto Exit;
    }

    *ppLogElem = pLogElem;

Exit:
    return hr;
}
                                 
CDebugLogElement::CDebugLogElement(DWORD dwDetailLvl)
: _pszMsg(NULL)
, _dwDetailLvl(dwDetailLvl)
{
}

CDebugLogElement::~CDebugLogElement()
{
    SAFEDELETEARRAY(_pszMsg);
}

HRESULT CDebugLogElement::Init(LPCWSTR pwzMsg)
{
    HRESULT                                    hr = S_OK;

    ASSERT(pwzMsg);

    _pszMsg = WSTRDupDynamic(pwzMsg);
    if (!_pszMsg) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }
    
Exit:
    return hr;
}

HRESULT CDebugLogElement::Dump(HANDLE hFile)
{
    HRESULT                                        hr = S_OK;
    DWORD                                          dwLen = 0;
    DWORD                                          dwWritten = 0;
    DWORD                                          dwSize = 0;
    DWORD                                          dwBufSize = 0;
    LPSTR                                          szBuf = NULL;
    BOOL                                           bRet;

    if (!hFile) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    dwSize = lstrlenW(_pszMsg) + 1;

    // Allocate for worst-case scenario where the UTF-8 size is 3 bytes
    // (ie. 1 byte greater than the 2-byte UNICODE char)./
    
    dwBufSize = dwSize * (sizeof(WCHAR) + 1);

    szBuf = NEW(CHAR[dwBufSize]);
    if (!szBuf) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }
    
    dwLen = WszWideCharToMultiByte(CP_UTF8, 0, _pszMsg, dwSize, szBuf, dwBufSize, NULL, NULL);
    if (!dwLen) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    bRet = WriteFile(hFile, szBuf, dwLen, &dwWritten, NULL);
    if (!bRet) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }
    
Exit:
    SAFEDELETEARRAY(szBuf);

    return hr;
}

//
// CDebugLog Class
//

HRESULT CDebugLog::Create(IApplicationContext *pAppCtx, LPCWSTR pwzAsmName,
                          CDebugLog **ppdl)
{
    HRESULT                                   hr = S_OK;
    CDebugLog                                *pdl = NULL;

    if (!pwzAsmName || !pAppCtx) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    *ppdl = NULL;

    pdl = NEW(CDebugLog);
    if (!pdl) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    hr = pdl->Init(pAppCtx, pwzAsmName);
    if (FAILED(hr)) {
        delete pdl;
        pdl = NULL;
        goto Exit;
    }

    *ppdl = pdl;

Exit:
    return hr;
}

CDebugLog::CDebugLog()
: _hrResult(S_OK)
, _cRef(1)
, _dwNumEntries(0)
, _pwzAsmName(NULL)
, _wzEXEName(NULL)
, _bWroteDetails(FALSE)
{
    _szLogPath[0] = L'\0';
}

CDebugLog::~CDebugLog()
{
    LISTNODE                                 pos = NULL;
    CDebugLogElement                        *pLogElem = NULL;

    pos = _listDbgMsg.GetHeadPosition();

    while (pos) {
        pLogElem = _listDbgMsg.GetNext(pos);
        SAFEDELETE(pLogElem);
    }

    _listDbgMsg.RemoveAll();

    SAFEDELETEARRAY(_pwzAsmName);
    SAFEDELETEARRAY(_wzEXEName);
}

HRESULT CDebugLog::Init(IApplicationContext *pAppCtx, LPCWSTR pwzAsmName)
{
    HRESULT                                  hr = S_OK;
    LPWSTR                                   wzAppName = NULL;

    if (!pwzAsmName) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    hr = SetAsmName(pwzAsmName);
    if (FAILED(hr)) {
        goto Exit;
    }
    
    // Get the executable name

    hr = ::AppCtxGetWrapper(pAppCtx, ACTAG_APP_NAME, &wzAppName);
    if (FAILED(hr)) {
        goto Exit;
    }

    if (wzAppName && lstrlenW(wzAppName)) {
        _wzEXEName = WSTRDupDynamic(wzAppName);
        if (!_wzEXEName) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
    }
    else {
        LPWSTR               wzFileName;

        // Didn't find EXE name in appctx. Use the .EXE name.

        wzFileName = PathFindFileName(g_wzEXEPath);
        ASSERT(wzFileName);

        _wzEXEName = WSTRDupDynamic(wzFileName);
        if (!_wzEXEName) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
    }

    // Log path
    
    if (!PAL_FetchConfigurationString(TRUE, REG_VAL_FUSION_LOG_PATH, _szLogPath, MAX_PATH))
    {
        // fallback to default
        if (!PAL_GetUserConfigurationDirectory(_szLogPath, MAX_PATH))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }

        StrCatBuff(_szLogPath, L"\\FusionLogs", MAX_PATH);
    }

    PathRemoveBackslashW(_szLogPath);

Exit:
    SAFEDELETEARRAY(wzAppName);

    return hr;
}

HRESULT CDebugLog::SetAsmName(LPCWSTR pwzAsmName)
{
    HRESULT                                  hr = S_OK;
    int                                      iLen;

    if (_pwzAsmName) {
        // You can only set the name once.
        hr = S_FALSE;
        goto Exit;
    }

    if (!pwzAsmName) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    iLen = lstrlenW(pwzAsmName) + 1;
    _pwzAsmName = NEW(WCHAR[iLen]);
    if (!_pwzAsmName) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    lstrcpyW(_pwzAsmName, pwzAsmName);

Exit:
    return hr;
}

//
// IUnknown
//

STDMETHODIMP CDebugLog::QueryInterface(REFIID riid, void **ppv)
{
    HRESULT                          hr = S_OK;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IFusionBindLog)) {
        *ppv = static_cast<IFusionBindLog *>(this);
    }
    else {
        *ppv = NULL;
        hr = E_NOINTERFACE;
    }

    if (*ppv) {
        AddRef();
    }

    return hr;
}


STDMETHODIMP_(ULONG) CDebugLog::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CDebugLog::Release()
{
    ULONG            ulRef;

    ulRef = InterlockedDecrement(&_cRef);
    
    if (ulRef == 0) {
        delete this;
    }

    return ulRef;
}

//
// IFusionBindLog
//

STDMETHODIMP CDebugLog::GetResultCode()
{
    return _hrResult;
}

STDMETHODIMP CDebugLog::GetBindLog(DWORD dwDetailLevel, LPWSTR pwzDebugLog,
                                   DWORD *pcbDebugLog)
{
    HRESULT                                  hr = S_OK;
    LISTNODE                                 pos = NULL;
    DWORD                                    dwCharsReqd;
    CDebugLogElement                        *pLogElem = NULL;

    if (!pcbDebugLog) {
        hr = E_INVALIDARG;
        goto Exit;
    }
    
    pos = _listDbgMsg.GetHeadPosition();
    if (!pos) {
        // No entries in debug log!
        hr = S_FALSE;
        goto Exit;
    }

    // Calculate total size (entries + new line chars + NULL)

    dwCharsReqd = 0;
    while (pos) {
        pLogElem = _listDbgMsg.GetNext(pos);
        ASSERT(pLogElem);

        if (pLogElem->_dwDetailLvl <= dwDetailLevel) {
            dwCharsReqd += lstrlenW(pLogElem->_pszMsg) * sizeof(WCHAR);
            dwCharsReqd += sizeof(L"\r\n");
        }
    }

    dwCharsReqd += 1; // NULL char

    if (!pwzDebugLog || *pcbDebugLog < dwCharsReqd) {
        *pcbDebugLog = dwCharsReqd;

        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        goto Exit;
    }

    *pwzDebugLog = L'\0';

    pos = _listDbgMsg.GetHeadPosition();
    while (pos) {
        pLogElem = _listDbgMsg.GetNext(pos);
        ASSERT(pLogElem);

        if (pLogElem->_dwDetailLvl <= dwDetailLevel) {
            StrCatW(pwzDebugLog, pLogElem->_pszMsg);
            StrCatW(pwzDebugLog, L"\r\n");
        }
    }

    ASSERT((DWORD)lstrlenW(pwzDebugLog) * sizeof(WCHAR) < dwCharsReqd);

Exit:
    return hr;
}                                    

//
// CDebugLog helpers
//

HRESULT CDebugLog::SetResultCode(HRESULT hr)
{
    _hrResult = hr;

    return S_OK;
}

HRESULT CDebugLog::DebugOut(DWORD dwDetailLvl, DWORD dwResId, ...)
{
    HRESULT                                  hr = S_OK;
    va_list                                  args;
    LPWSTR                                   wzFormatString = NULL;
    LPWSTR                                   wzDebugStr = NULL;

    wzFormatString = NEW(WCHAR[MAX_DBG_STR_LEN]);
    if (!wzFormatString) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    wzDebugStr = NEW(WCHAR[MAX_DBG_STR_LEN]);
    if (!wzDebugStr) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }
    
    wzFormatString[0] = L'\0';

    if (!_LoadString(dwResId, wzFormatString, MAX_DBG_STR_LEN)) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    va_start(args, dwResId);
    wvnsprintfW(wzDebugStr, MAX_DBG_STR_LEN, wzFormatString, args);
    va_end(args);

    hr = LogMessage(dwDetailLvl, wzDebugStr, FALSE);
    if (FAILED(hr)) {
        goto Exit;
    }

Exit:
    SAFEDELETEARRAY(wzDebugStr);
    SAFEDELETEARRAY(wzFormatString);

    return hr;
}

HRESULT CDebugLog::LogMessage(DWORD dwDetailLvl, LPCWSTR wzDebugStr, BOOL bPrepend)
{
    HRESULT                                  hr = S_OK;
    CDebugLogElement                        *pLogElem = NULL;
    
    hr = CDebugLogElement::Create(dwDetailLvl, wzDebugStr, &pLogElem);
    if (FAILED(hr)) {
        goto Exit;
    }

    _dwNumEntries += 1;

    if (bPrepend) {
        _listDbgMsg.AddHead(pLogElem);
    }
    else {
        _listDbgMsg.AddTail(pLogElem);
    }

Exit:
    return hr;    
}

HRESULT CDebugLog::DumpDebugLog(DWORD dwDetailLvl, HRESULT hrLog)
{
    HRESULT                                    hr = S_OK;
    HANDLE                                     hFile = INVALID_HANDLE_VALUE;
    LISTNODE                                   pos = NULL;
    LPWSTR                                     wzUrlName=NULL;
    CDebugLogElement                          *pLogElem = NULL;
    WCHAR                                      wzFileName[MAX_PATH];
    WCHAR                                      wzSiteName[MAX_PATH];
    WCHAR                                      wzAppLogDir[MAX_PATH];
    LPWSTR                                     wzResourceName = NULL;
    DWORD                                      dwBytes;
    DWORD                                      dwSize;
    CCriticalSection                           cs(&g_csBindLog);
    BOOL                                       bRet;

    if (!g_dwLogFailures && !g_dwForceLog) {
        return S_FALSE;
    }

    hr = cs.Lock();
    if (FAILED(hr)) {
        return hr;
    }

    pos = _listDbgMsg.GetHeadPosition();
    if (!pos) {
        hr = S_FALSE;
        goto Exit;
    }

    wzUrlName = NEW(WCHAR[MAX_URL_LENGTH+1]);
    if (!wzUrlName)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    // Build the log entry URL and Wininet cache file
    
    wnsprintfW(wzUrlName, MAX_URL_LENGTH, L"?FusionBindError!exe=%ws!name=%ws", _wzEXEName, _pwzAsmName);

    {
        wnsprintfW(wzAppLogDir, MAX_PATH, L"%ws\\%ws", _szLogPath, _wzEXEName);

        if (GetFileAttributes(wzAppLogDir) == (DWORD) -1) {
            bRet = CreateDirectory(wzAppLogDir, NULL);
            if (!bRet) {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto Exit;
            }
        }

        if (PathIsURLW(_pwzAsmName)) {
            // This was a where-ref bind. We can't spit out a filename w/
            // the URL of the bind because the URL has invalid filename chars.
            // The best we can do is show that it was a where-ref bind, and
            // give the filename, and maybe the site.

            dwSize = MAX_PATH;
            hr = UrlGetPartW(_pwzAsmName, wzSiteName, &dwSize, URL_PART_HOSTNAME, 0);
            if (FAILED(hr)) {
                goto Exit;
            }

            wzResourceName = PathFindFileName(_pwzAsmName);

            ASSERT(wzResourceName);

            if (!lstrlenW(wzSiteName)) {
                lstrcpyW(wzSiteName, L"LocalMachine");
            }

            wnsprintfW(wzFileName, MAX_PATH, L"%ws\\FusionBindError!exe=%ws!name=WhereRefBind!Host=(%ws)!FileName=(%ws).HTM",
                       wzAppLogDir, _wzEXEName, wzSiteName, wzResourceName);
        }
        else {
            wnsprintfW(wzFileName, MAX_PATH, L"%ws\\FusionBindError!exe=%ws!name=%ws.HTM", wzAppLogDir, _wzEXEName, _pwzAsmName);
        }
    }

    // Create the and write the log file

    hr = CreateLogFile(&hFile, wzFileName, _wzEXEName, hrLog);
    if (FAILED(hr)) {
        goto Exit;
    }

    pos = _listDbgMsg.GetHeadPosition();
    while (pos) {
        pLogElem = _listDbgMsg.GetNext(pos);
        ASSERT(pLogElem);

        if (pLogElem->_dwDetailLvl <= dwDetailLvl) {
            pLogElem->Dump(hFile);
            WriteFile(hFile, DEBUG_LOG_NEW_LINE, lstrlenW(DEBUG_LOG_NEW_LINE) * sizeof(WCHAR),
                      &dwBytes, NULL);
        }
    }

    // Close the log file and commit the wininet cache entry

    hr = CloseLogFile(&hFile);
    if (FAILED(hr)) {
        goto Exit;
    }


Exit:
    cs.Unlock();
    SAFEDELETEARRAY(wzUrlName);

    return hr;
}

HRESULT CDebugLog::CloseLogFile(HANDLE *phFile)
{
    HRESULT                               hr = S_OK;
    DWORD                                 dwBytes;

    if (!phFile) {
        hr = E_INVALIDARG;
        goto Exit;
    }
    
    WriteFile(*phFile, DEBUG_LOG_HTML_END, lstrlenW(DEBUG_LOG_HTML_END) * sizeof(WCHAR),
              &dwBytes, NULL);

    CloseHandle(*phFile);

    *phFile = INVALID_HANDLE_VALUE;

Exit:
    return hr;
}

HRESULT CDebugLog::CreateLogFile(HANDLE *phFile, LPCWSTR wzFileName,
                                 LPCWSTR wzEXEName, HRESULT hrLog)
{
    HRESULT                              hr = S_OK;
    SYSTEMTIME                           systime;
    LPWSTR                               pwzFormatMessage = NULL;
    DWORD                                dwFMResult = 0;
    LPWSTR                               wzBuffer = NULL;
    LPWSTR                               wzBuf = NULL;
    LPWSTR                               wzResultText = NULL;
    WCHAR                                wzDateBuffer[MAX_DATE_LEN];
    WCHAR                                wzTimeBuffer[MAX_DATE_LEN];

    if (!phFile || !wzFileName || !wzEXEName) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    wzBuffer = NEW(WCHAR[MAX_DBG_STR_LEN]);
    if (!wzBuffer) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    wzBuf = NEW(WCHAR[MAX_DBG_STR_LEN]);
    if (!wzBuffer) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    wzResultText = NEW(WCHAR[MAX_DBG_STR_LEN]);
    if (!wzResultText) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    *phFile = CreateFile(wzFileName, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                         CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (*phFile == INVALID_HANDLE_VALUE) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    if (!_bWroteDetails) {
    
        // Details
    
        if (!_LoadString(ID_FUSLOG_DETAILED_LOG, wzBuffer, MAX_DBG_STR_LEN)) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }
    
        LogMessage(0, wzBuffer, TRUE);
        
        // Executable path
    
        if (!_LoadString(ID_FUSLOG_EXECUTABLE, wzBuf, MAX_DBG_STR_LEN)) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }
    
        wnsprintfW(wzBuffer, MAX_DBG_STR_LEN, L"%ws %ws", wzBuf, g_wzEXEPath);
        LogMessage(0, wzBuffer, TRUE);
        
        // Fusion.dll path
        
        if (!_LoadString(ID_FUSLOG_FUSION_DLL_PATH, wzBuf, MAX_DBG_STR_LEN)) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }
    
        wnsprintfW(wzBuffer, MAX_DBG_STR_LEN, L"%ws %ws", wzBuf, g_FusionDllPath);
        LogMessage(0, wzBuffer, TRUE);
        
        // Bind result and FormatMessage text
        
        if (!_LoadString(ID_FUSLOG_BIND_RESULT_TEXT, wzResultText, MAX_DBG_STR_LEN)) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }
            
        dwFMResult = WszFormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                    FORMAT_MESSAGE_FROM_SYSTEM, 0, hrLog, 0,
                                    (LPWSTR)&pwzFormatMessage, 0, NULL);
        if (dwFMResult) {                               
            wnsprintfW(wzBuffer, MAX_DBG_STR_LEN, wzResultText, hrLog, pwzFormatMessage);
        }
        else {
            WCHAR                             wzNoDescription[MAX_DBG_STR_LEN];
    
            if (!_LoadString(ID_FUSLOG_NO_DESCRIPTION, wzNoDescription, MAX_DBG_STR_LEN)) {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto Exit;
            }
            
            wnsprintfW(wzBuffer, MAX_DBG_STR_LEN, wzResultText, hrLog, wzNoDescription);
        }
    
        LogMessage(0, wzBuffer, TRUE);
    
        // Success/fail
        
        if (SUCCEEDED(hrLog)) {
            if (!_LoadString(ID_FUSLOG_OPERATION_SUCCESSFUL, wzBuffer, MAX_DBG_STR_LEN)) {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto Exit;;
            }
    
            LogMessage(0, wzBuffer, TRUE);
        }
        else {
            if (!_LoadString(ID_FUSLOG_OPERATION_FAILED, wzBuffer, MAX_DBG_STR_LEN)) {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto Exit;;
            }
    
            LogMessage(0, wzBuffer, TRUE);
        }
    
        // Header text
    
        GetSystemTime(&systime);

        wsprintf(wzDateBuffer, L"%02d/%02d/%04d", systime.wMonth, systime.wDay, systime.wYear);
        wsprintf(wzTimeBuffer, L"%02d:%02d:%02d (UTC)", systime.wHour, systime.wMinute, systime.wSecond);
    
        if (!_LoadString(ID_FUSLOG_HEADER_TEXT, wzBuf, MAX_DBG_STR_LEN)) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }
    
        wnsprintfW(wzBuffer, MAX_DBG_STR_LEN, L"%ws (%ws @ %ws) ***\n", wzBuf, wzDateBuffer, wzTimeBuffer);
        LogMessage(0, wzBuffer, TRUE);
        
        // HTML start/end
    
        LogMessage(0, DEBUG_LOG_HTML_START, TRUE);
        LogMessage(0, DEBUG_LOG_HTML_META_LANGUAGE, TRUE);

        _bWroteDetails = TRUE;
    }
    
Exit:
    if (pwzFormatMessage) {
        LocalFree(pwzFormatMessage);
    }

    SAFEDELETEARRAY(wzBuffer);
    SAFEDELETEARRAY(wzBuf);
    SAFEDELETEARRAY(wzResultText);

    return hr;    
}

