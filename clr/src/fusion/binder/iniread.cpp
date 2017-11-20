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
#include "history.h"
#include "iniread.h"
#include "iniwrite.h"
#include "util.h"
#include "helpers.h"

extern DWORD g_dwMaxAppHistory;

CIniReader::CIniReader()
: _cRef(1)
{
    _dwSig = 0x52494e49; /* 'RINI' */

    _hIni = NULL;
    _fIniOwned = FALSE;
}

CIniReader::~CIniReader()
{
    if (_fIniOwned && (_hIni != NULL)) {
        PAL_IniClose(_hIni);
    }
}

HRESULT CIniReader::Init(HINI hIni, BOOL fOwned)
{
    HRESULT                                     hr = S_OK;

    if (!hIni) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    _hIni = hIni;
    _fIniOwned = fOwned;

Exit:
    return hr;
}

HRESULT CIniReader::QueryInterface(REFIID riid, void **ppv)
{
    HRESULT                                    hr = S_OK;

    *ppv = NULL;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IHistoryReader)) {
        *ppv = static_cast<IHistoryReader *>(this);
    }
    else {
        hr = E_NOINTERFACE;
    }

    if (*ppv) {
        AddRef();
    }

    return hr;
}

STDMETHODIMP_(ULONG) CIniReader::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CIniReader::Release()
{
    ULONG                    ulRef = InterlockedDecrement(&_cRef);

    if (!ulRef) {
        delete this;
    }

    return ulRef;
}

#define _ReadStringEx(section, key, value) PAL_IniReadStringEx(_hIni, section, key, value)

HRESULT CIniReader::GetAssemblyInfo(LPCWSTR wzActivationDate,
                                    LPCWSTR wzAssemblyName,
                                    LPCWSTR wzPublicKeyToken,
                                    LPCWSTR wzCulture,
                                    LPCWSTR wzVerReference,
                                    AsmBindHistoryInfo *pBindInfo)
{
    HRESULT                                 hr = S_OK;
    LPWSTR                                  wzBuffer = NULL;
    LPWSTR                                  wzTag = NULL;
    LPWSTR                                  wzActivationTag = NULL;
    LPWSTR                                  wzKey = NULL;
    LPWSTR                                  wzPropBuf = NULL;
    DWORD                                   dwRet;
    BOOL                                    bFound;
    DWORD                                   i;

    if (!wzActivationDate || !pBindInfo || !wzAssemblyName || !wzPublicKeyToken || !wzCulture || !wzVerReference) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    wzTag = NEW(WCHAR[MAX_INI_TAG_LENGTH]);
    if (!wzTag) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    wzKey = NEW(WCHAR[MAX_INI_TAG_LENGTH]);
    if (!wzKey) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    memset(pBindInfo, 0, sizeof(AsmBindHistoryInfo));

    bFound = FALSE;

    for (i = 1; i <= g_dwMaxAppHistory; i++) {
        wnsprintfW(wzTag, MAX_INI_TAG_LENGTH, L"%ws_%d", FUSION_SNAPSHOT_PREFIX, i);

        dwRet = _ReadStringEx(HISTORY_SECTION_HEADER, wzTag, &wzActivationTag);
        if (!wzActivationTag) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        if (!lstrcmpiW(wzActivationTag, DEFAULT_INI_VALUE)) {
            hr = E_UNEXPECTED;
            goto Exit;
        }

        if (!lstrcmpiW(wzActivationTag, wzActivationDate)) {
            bFound = TRUE;
            break;
        }
    }

    if (bFound) {
        // Found activation history. Lookup assembly section.

        wnsprintfW(wzKey, MAX_INI_TAG_LENGTH, L"%ws%wc%ws%wc%ws%wc%ws", wzAssemblyName,
                   HISTORY_DELIMITER_CHAR, wzPublicKeyToken,
                   HISTORY_DELIMITER_CHAR, wzCulture,
                   HISTORY_DELIMITER_CHAR, wzVerReference);

        dwRet = _ReadStringEx(wzActivationTag, wzKey, &wzBuffer);
        if (!wzBuffer) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        if (!lstrcmpiW(wzBuffer, DEFAULT_INI_VALUE)) {
            // Did not find assembly

            hr = S_FALSE;
            goto Exit;
        }

        // Populate the bind history information

        wnsprintf(pBindInfo->wzAsmName, MAX_INI_TAG_LENGTH, L"%ws", wzAssemblyName);
        wnsprintf(pBindInfo->wzPublicKeyToken, MAX_PUBLIC_KEY_TOKEN_LEN, L"%ws", wzPublicKeyToken);

        SAFEDELETEARRAY(wzPropBuf);
        dwRet = _ReadStringEx(wzBuffer, ASSEMBLY_DATA_VER_REFERENCE, &wzPropBuf);
        if (!wzPropBuf) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
        wnsprintf(pBindInfo->wzVerReference, MAX_VERSION_DISPLAY_SIZE, L"%ws", wzPropBuf);

        SAFEDELETEARRAY(wzPropBuf);
        dwRet = _ReadStringEx(wzBuffer, ASSEMBLY_DATA_VER_APP_CFG, &wzPropBuf);
        if (!wzPropBuf) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
        wnsprintf(pBindInfo->wzVerAppCfg, MAX_VERSION_DISPLAY_SIZE, L"%ws", wzPropBuf);

        SAFEDELETEARRAY(wzPropBuf);
        dwRet = _ReadStringEx(wzBuffer, ASSEMBLY_DATA_VER_PUBLISHER_CFG, &wzPropBuf);
        if (!wzPropBuf) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
        wnsprintf(pBindInfo->wzVerPublisherCfg, MAX_VERSION_DISPLAY_SIZE, L"%ws", wzPropBuf);

        SAFEDELETEARRAY(wzPropBuf);
        dwRet = _ReadStringEx(wzBuffer, ASSEMBLY_DATA_VER_ADMIN_CFG, &wzPropBuf);
        if (!wzPropBuf) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
        wnsprintf(pBindInfo->wzVerAdminCfg, MAX_VERSION_DISPLAY_SIZE, L"%ws", wzPropBuf);
    }
    else {
        // Assembly not found

        hr = S_FALSE;
        goto Exit;
    }

Exit:
    SAFEDELETEARRAY(wzBuffer);
    SAFEDELETEARRAY(wzActivationTag);
    SAFEDELETEARRAY(wzTag);
    SAFEDELETEARRAY(wzKey);
    SAFEDELETEARRAY(wzPropBuf);
    
    return hr;
}

HRESULT CIniReader::GetFilePath(LPWSTR wzFilePath, DWORD *pdwSize)
{
    HRESULT                                        hr = S_OK;
    DWORD                                          dwLen;

    if (!pdwSize) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    ASSERT(_wzFilePath);
    dwLen = lstrlenW(_wzFilePath) + 1;

    if (!wzFilePath || *pdwSize < dwLen) {
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        *pdwSize = dwLen;
        goto Exit;
    }

    lstrcpyW(wzFilePath, _wzFilePath);
    *pdwSize = dwLen;

Exit:
    return hr;
}

HRESULT CIniReader::GetNumActivations(DWORD *pdwNumActivations)
{
    HRESULT                                     hr = S_OK;
    DWORD                                       dwRet;
    LPWSTR                                      wzBuffer = NULL;

    if (!pdwNumActivations) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    dwRet = _ReadStringEx(HISTORY_SECTION_HEADER, HEADER_DATA_NUM_SECTIONS, &wzBuffer);
    if (!wzBuffer) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    if (!lstrcmpiW(wzBuffer, DEFAULT_INI_VALUE)) {
        *pdwNumActivations = 0;
    }
    else {
        *pdwNumActivations = StrToIntW(wzBuffer);
    }

Exit:
    SAFEDELETEARRAY(wzBuffer);

    return hr;
}

HRESULT CIniReader::GetActivationDate(DWORD dwIdx, FILETIME *pftDate)
{
    HRESULT                                       hr = S_OK;
    DWORD                                         dwRet;
    LPWSTR                                        wzPos;
    LPWSTR                                        wzTag = NULL;
    LPWSTR                                        wzBuffer = NULL;

    if (!pftDate) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    wzTag = NEW(WCHAR[MAX_INI_TAG_LENGTH]);
    if (!wzTag) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    wnsprintfW(wzTag, MAX_INI_TAG_LENGTH, L"%ws_%d", FUSION_SNAPSHOT_PREFIX, dwIdx);

    dwRet = _ReadStringEx(HISTORY_SECTION_HEADER, wzTag, &wzBuffer);
    if (!wzBuffer) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    wzPos = wzBuffer;
    while (*wzPos) {
        if (*wzPos == L'.') {
            break;
        }

        wzPos++;
    }

    if (!*wzPos) {
        // Malformed date string
        hr = E_UNEXPECTED;
        goto Exit;
    }

    *wzPos = L'\0';
    wzPos++;

    pftDate->dwHighDateTime = StrToIntW(wzBuffer);
    pftDate->dwLowDateTime = StrToIntW(wzPos);

Exit:
    SAFEDELETEARRAY(wzTag);
    SAFEDELETEARRAY(wzBuffer);

    return hr;
}
    
HRESULT CIniReader::GetRunTimeVersion(FILETIME *pftActivationDate,
                                      LPWSTR wzRunTimeVersion, DWORD *pdwSize)
{
    HRESULT                                        hr = S_OK;
    DWORD                                          dwLen;
    DWORD                                          dwRet;
    LPWSTR                                         wzTag = NULL;
    LPWSTR                                         wzBuffer = NULL;

    if (!pftActivationDate || !pdwSize) {
        hr = E_INVALIDARG;
        goto Exit;
    }
    
    wzTag = NEW(WCHAR[MAX_INI_TAG_LENGTH]);
    if (!wzTag) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    wnsprintfW(wzTag, MAX_INI_TAG_LENGTH, L"%u.%u", pftActivationDate->dwHighDateTime,
               pftActivationDate->dwLowDateTime);

    dwRet = _ReadStringEx(wzTag, SNAPSHOT_DATA_URT_VERSION, &wzBuffer);
    if (!wzBuffer) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    if (!lstrcmpiW(wzBuffer, DEFAULT_INI_VALUE)) {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        goto Exit;
    }

    dwLen = lstrlenW(wzBuffer) + 1;

    if (!wzRunTimeVersion || *pdwSize < dwLen) {
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        *pdwSize = dwLen;
        goto Exit;
    }

    lstrcpyW(wzRunTimeVersion, wzBuffer);
    *pdwSize = dwLen;
    
Exit:
    SAFEDELETEARRAY(wzTag);
    SAFEDELETEARRAY(wzBuffer);

    return hr;
}


HRESULT CIniReader::GetApplicationName(LPWSTR wzAppName, DWORD *pdwSize)
{
    HRESULT                                   hr = S_OK;
    LPWSTR                                    wzBuffer = NULL;
    DWORD                                     dwRet;
    DWORD                                     dwLen;
    
    if (!pdwSize) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    dwRet = _ReadStringEx(HISTORY_SECTION_HEADER, HEADER_DATA_APP_NAME, &wzBuffer);
    if (!wzBuffer) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    if (!lstrcmpiW(wzBuffer, DEFAULT_INI_VALUE)) {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        goto Exit;
    }

    dwLen = lstrlenW(wzBuffer) + 1;

    if (!wzAppName || *pdwSize < dwLen) {
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        *pdwSize = dwLen;
        goto Exit;
    }

    lstrcpyW(wzAppName, wzBuffer);
    *pdwSize = dwLen;

Exit:
    SAFEDELETEARRAY(wzBuffer);

    return hr;
}
    
HRESULT CIniReader::GetEXEModulePath(LPWSTR wzExePath, DWORD *pdwSize)
{
    HRESULT                                   hr = S_OK;
    LPWSTR                                    wzBuffer = NULL;
    DWORD                                     dwRet;
    DWORD                                     dwLen;
    

    if (!pdwSize) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    dwRet = _ReadStringEx(HISTORY_SECTION_HEADER, HEADER_DATA_EXE_PATH, &wzBuffer);
    if (!wzBuffer) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    if (!lstrcmpiW(wzBuffer, DEFAULT_INI_VALUE)) {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        goto Exit;
    }

    dwLen = lstrlenW(wzBuffer) + 1;

    if (!wzExePath || *pdwSize < dwLen) {
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        *pdwSize = dwLen;
        goto Exit;
    }

    lstrcpyW(wzExePath, wzBuffer);
    *pdwSize = dwLen;

Exit:
    SAFEDELETEARRAY(wzBuffer);

    return hr;
}

HRESULT CIniReader::GetNumAssemblies(FILETIME *pftActivationDate,
                                     DWORD *pdwNumAsms)
{
    HRESULT                                        hr = S_OK;
    LPWSTR                                         wzTag = NULL;
    LPWSTR                                         wzBuffer = NULL;
    LPWSTR                                         wzCurStr;
    DWORD                                          dwRet;
    DWORD                                          dwCurIdx;

    if (!pftActivationDate || !pdwNumAsms) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    wzTag = NEW(WCHAR[MAX_INI_TAG_LENGTH]);
    if (!wzTag) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    *pdwNumAsms = 0;

    wnsprintfW(wzTag, MAX_INI_TAG_LENGTH, L"%u.%u", pftActivationDate->dwHighDateTime,
               pftActivationDate->dwLowDateTime);
               
    dwRet = _ReadStringEx(wzTag, NULL, &wzBuffer);
    if (!wzBuffer) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    if (!lstrcmpiW(wzBuffer, DEFAULT_INI_VALUE)) {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        goto Exit;
    }

    wzCurStr = wzBuffer;
    dwCurIdx = 0;

    while (lstrlenW(wzCurStr)) {
        if (!lstrcmpiW(wzCurStr, SNAPSHOT_DATA_URT_VERSION)) {
            // Iterate over this (it's not an assembly).
            wzCurStr += (lstrlenW(wzCurStr) + 1);
            continue;
        }

        dwCurIdx++;
        wzCurStr += (lstrlenW(wzCurStr) + 1);
    }

    *pdwNumAsms = dwCurIdx;

Exit:
    SAFEDELETEARRAY(wzTag);
    SAFEDELETEARRAY(wzBuffer);

    return hr;
}



HRESULT CIniReader::GetHistoryAssembly(FILETIME *pftActivationDate, DWORD dwIdx,
                                       IHistoryAssembly **ppHistAsm)
{
    HRESULT                                        hr = S_OK;
    DWORD                                          dwRet;
    DWORD                                          dwCurIdx;
    LPWSTR                                         wzCurStr;
    LPWSTR                                         wzAsmName = NULL;
    LPWSTR                                         wzAsmPublicKeyToken = NULL;
    LPWSTR                                         wzAsmCulture = NULL;
    LPWSTR                                         wzVerRef = NULL;
    LPWSTR                                         wzTag = NULL;
    LPWSTR                                         wzBuffer = NULL;
    BOOL                                           bFound = FALSE;

    if (!pftActivationDate) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    wzTag = NEW(WCHAR[MAX_INI_TAG_LENGTH]);
    if (!wzTag) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    wnsprintfW(wzTag, MAX_INI_TAG_LENGTH, L"%u.%u", pftActivationDate->dwHighDateTime,
               pftActivationDate->dwLowDateTime);

    dwRet = _ReadStringEx(wzTag, NULL, &wzBuffer);
    if (!wzBuffer) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    if (!lstrcmpiW(wzBuffer, DEFAULT_INI_VALUE)) {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        goto Exit;
    }

    wzCurStr = wzBuffer;
    dwCurIdx = 1;

    while (lstrlenW(wzCurStr)) {
        if (!lstrcmpiW(wzCurStr, SNAPSHOT_DATA_URT_VERSION)) {
            // Iterate over this (it's not an assembly).
            wzCurStr += (lstrlenW(wzCurStr) + 1);
            continue;
        }

        if (dwCurIdx == dwIdx) {
            // wzCurStr is of the form: [AsmName].[PublicKeyToken].[Culture].[VerRef]
    
            hr = ExtractAssemblyInfo(wzCurStr, &wzAsmName, &wzAsmPublicKeyToken, &wzAsmCulture, &wzVerRef);
            if (FAILED(hr)) {
                goto Exit;
            }

            bFound = TRUE;
            break;
        }

        // Iterate

        dwCurIdx++;
        wzCurStr += (lstrlenW(wzCurStr) + 1);
    }

    if (!bFound) {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        goto Exit;
    }

    hr = LookupHistoryAssemblyInternal(this, wzTag, wzAsmName, wzAsmPublicKeyToken,
                                       wzAsmCulture, wzVerRef, ppHistAsm);
    if (FAILED(hr)) {
        goto Exit;
    }

Exit:
    SAFEDELETEARRAY(wzTag);
    SAFEDELETEARRAY(wzBuffer);

    return hr;
}


HRESULT CIniReader::ExtractAssemblyInfo(LPWSTR wzAsmStr, LPWSTR *ppwzAsmName,
                                        LPWSTR *ppwzAsmPublicKeyToken,
                                        LPWSTR *ppwzAsmCulture,
                                        LPWSTR *ppwzAsmVerRef)
{
    HRESULT                                        hr = S_OK;
    LPWSTR                                         wzPos;
    
    ASSERT(wzAsmStr && ppwzAsmName && ppwzAsmPublicKeyToken && ppwzAsmCulture && ppwzAsmVerRef);
        // Assembly Name

    *ppwzAsmName = wzAsmStr;
    wzPos = wzAsmStr;

    while (*wzPos) {
        if (*wzPos == HISTORY_DELIMITER_CHAR) {
            break;
        }

        wzPos++;
    }

    if (!wzPos) {
        // malformed string
        hr = E_UNEXPECTED;
        goto Exit;
    }

    *wzPos = L'\0';
    wzPos++;

    // Public Key Token

    *ppwzAsmPublicKeyToken = wzPos;
    
    while (*wzPos) {
        if (*wzPos == HISTORY_DELIMITER_CHAR) {
            break;
        }

        wzPos++;
    }

    if (!wzPos) {
        // malformed string
        hr = E_UNEXPECTED;
        goto Exit;
    }

    *wzPos = L'\0';
    wzPos++;

    // Culture

    *ppwzAsmCulture = wzPos;

    while (*wzPos) {
        if (*wzPos == HISTORY_DELIMITER_CHAR) {
            break;
        }

        wzPos++;
    }

    if (!wzPos) {
        //malformed string
        hr = E_UNEXPECTED;
        goto Exit;
    }

    *wzPos = L'\0';
    wzPos++;

    // Reference Version

    *ppwzAsmVerRef = wzPos;

    if (!*wzPos) {
        // malformed string
        hr = E_UNEXPECTED;
        goto Exit;
    }


Exit:
    if (FAILED(hr)) {
        *ppwzAsmName = NULL;
        *ppwzAsmPublicKeyToken = NULL;
        *ppwzAsmVerRef = NULL;
    }
    return hr;
}


STDMETHODIMP_(BOOL) CIniReader::DoesExist(IHistoryAssembly *pHistAsm)
{
    HRESULT                                     hr = S_OK;
    LPWSTR                                      wzBuffer = NULL;
    WCHAR                                       wzAsmTag[MAX_INI_TAG_LENGTH];
    WCHAR                                       wzActivationDate[MAX_INI_TAG_LENGTH];
    WCHAR                                       wzAsmName[MAX_INI_TAG_LENGTH];
    WCHAR                                       wzAsmPublicKeyToken[MAX_INI_TAG_LENGTH];
    WCHAR                                       wzAsmCulture[MAX_INI_TAG_LENGTH];
    WCHAR                                       wzVerRef[MAX_INI_TAG_LENGTH];
    BOOL                                        bExists = FALSE;
    DWORD                                       dwSize;
    DWORD                                       dwRet;

    if (!pHistAsm) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    dwSize = MAX_INI_TAG_LENGTH;
    hr = pHistAsm->GetActivationDate(wzActivationDate, &dwSize);
    if (FAILED(hr)) {
        goto Exit;
    }

    dwSize = MAX_INI_TAG_LENGTH;
    hr = pHistAsm->GetAssemblyName(wzAsmName, &dwSize);
    if (FAILED(hr)) {
        goto Exit;
    }

    dwSize = MAX_INI_TAG_LENGTH;
    hr = pHistAsm->GetPublicKeyToken(wzAsmPublicKeyToken, &dwSize);
    if (FAILED(hr)) {
        goto Exit;
    }

    dwSize = MAX_INI_TAG_LENGTH;
    hr = pHistAsm->GetCulture(wzAsmCulture, &dwSize);
    if (FAILED(hr)) {
        goto Exit;
    }

    dwSize = MAX_INI_TAG_LENGTH;
    hr = pHistAsm->GetReferenceVersion(wzVerRef, &dwSize);
    if (FAILED(hr)) {
        goto Exit;
    }

    wnsprintfW(wzAsmTag, MAX_INI_TAG_LENGTH, L"%ws%wc%ws%wc%ws%wc%ws", wzAsmName,
               HISTORY_DELIMITER_CHAR, wzAsmPublicKeyToken,
               HISTORY_DELIMITER_CHAR, wzAsmCulture,
               HISTORY_DELIMITER_CHAR, wzVerRef);

    dwRet = _ReadStringEx(wzActivationDate, wzAsmTag, &wzBuffer);
    if (!wzBuffer) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    if (lstrcmpiW(DEFAULT_INI_VALUE, wzBuffer)) {
        bExists = TRUE;
    }
    
Exit:
    SAFEDELETEARRAY(wzBuffer);

    return bExists;
}
    

STDAPI CreateHistoryReader(LPCWSTR wzFilePath, IHistoryReader **ppHistoryReader)
{
    HRESULT                                 hr = S_OK;
    CIniReader                             *pHistoryReader = NULL;
    HINI                                    hIni = NULL;

    if (!wzFilePath || !ppHistoryReader) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    pHistoryReader = NEW(CIniReader);
    if (!pHistoryReader) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }


    if ((hIni = PAL_IniCreate()) == NULL)
    {
        hr = FusionpHresultFromLastError();
        goto Exit;
    }

    if (!PAL_IniLoad(hIni, wzFilePath))
    {
        hr = FusionpHresultFromLastError();
        goto Exit;
    }

    hr = pHistoryReader->Init(hIni, TRUE);
    if (FAILED(hr)) {
        goto Exit;
    }
    hIni = NULL; // pHistoryReader took ownership


    *ppHistoryReader = pHistoryReader;
    (*ppHistoryReader)->AddRef();

Exit:
    if (hIni != NULL)
        PAL_IniClose(hIni);

    SAFERELEASE(pHistoryReader);

    return hr;
}

