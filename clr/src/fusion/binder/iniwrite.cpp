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
#include "iniwrite.h"
#include "history.h"
#include "util.h"
#include "helpers.h"

extern DWORD g_dwMaxAppHistory;

CIniWriter::CIniWriter()
: _pwzFileName(NULL)
{
    _dwSig = 0x57494e49; /* 'WINI' */
    _hIni = NULL;
}

CIniWriter::~CIniWriter()
{
    if (_hIni != NULL)
        PAL_IniClose(_hIni);

    SAFEDELETEARRAY(_pwzFileName);
}

HRESULT CIniWriter::Init(LPCWSTR pwzFileName)
{
    HRESULT                                     hr = S_OK;

    if (!pwzFileName) {
        hr = E_INVALIDARG;
        goto Exit;
    }
    
    _pwzFileName = WSTRDupDynamic(pwzFileName);
    if (!_pwzFileName) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    if ((_hIni = PAL_IniCreate()) == NULL)
    {
        hr = FusionpHresultFromLastError();
        goto Exit;
    }

Exit:
    return hr;
}

#define _ReadStringEx(section, key, value) PAL_IniReadStringEx(_hIni, section, key, value)
#define _WriteString(section, key, value) PAL_IniWriteString(_hIni, section, key, value)

HRESULT CIniWriter::AddSnapShot(LPCWSTR wzActivationDate, LPCWSTR wzURTVersion)
{
    HRESULT                                     hr = S_OK;
    LPWSTR                                      wzBuffer = NULL;
    LPWSTR                                      wzTagOld = NULL;
    LPWSTR                                      wzTagNew = NULL;
    DWORD                                       dwRet;
    DWORD                                       dwNumSnapShots;
    DWORD                                       idx;
    BOOL                                        bRet;

    if (!wzActivationDate) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    wzTagOld = NEW(WCHAR[MAX_INI_TAG_LENGTH]);
    if (!wzTagOld) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    wzTagNew = NEW(WCHAR[MAX_INI_TAG_LENGTH]);
    if (!wzTagNew) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    dwRet = _ReadStringEx(HISTORY_SECTION_HEADER, HEADER_DATA_NUM_SECTIONS, &wzBuffer);
    if (!wzBuffer) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    dwNumSnapShots = StrToIntW(wzBuffer);

    for (idx = dwNumSnapShots; idx >= 1; idx--) {
        wnsprintfW(wzTagOld, MAX_INI_TAG_LENGTH, L"%ws_%d", FUSION_SNAPSHOT_PREFIX, idx);
        wnsprintfW(wzTagNew, MAX_INI_TAG_LENGTH, L"%ws_%d", FUSION_SNAPSHOT_PREFIX, idx + 1);

        SAFEDELETEARRAY(wzBuffer);
        dwRet = _ReadStringEx(HISTORY_SECTION_HEADER, wzTagOld, &wzBuffer);
        if (!wzBuffer) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        if (!lstrcmpiW(wzBuffer, DEFAULT_INI_VALUE)) {
            // Expected old value to exist
            hr = E_UNEXPECTED;
        }

        if (idx >= g_dwMaxAppHistory) {
            // Delete this section.
            hr = DeleteSnapShot(wzBuffer);
            if (FAILED(hr)) {
                goto Exit;
            }

            // Delete the entry from the header
            bRet = _WriteString(HISTORY_SECTION_HEADER, wzTagOld, NULL);
            if (!bRet) {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto Exit;
            }

        }
        else {
            bRet = _WriteString(HISTORY_SECTION_HEADER, wzTagNew, wzBuffer);
            if (!bRet) {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto Exit;
            }

        }
    }                                              


    // Update number of entries

    if (dwNumSnapShots < g_dwMaxAppHistory) {
        dwNumSnapShots++;
    }
    else {
        dwNumSnapShots = g_dwMaxAppHistory;
    }

    wnsprintfW(wzTagNew, MAX_INI_TAG_LENGTH, L"%d", dwNumSnapShots);
    bRet = _WriteString(HISTORY_SECTION_HEADER, HEADER_DATA_NUM_SECTIONS, wzTagNew);
    if (!bRet) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    // Write new MRU snapshot in the header.
    
    wnsprintfW(wzTagNew, MAX_INI_TAG_LENGTH, L"%ws_%d", FUSION_SNAPSHOT_PREFIX, 1);

    bRet = _WriteString(HISTORY_SECTION_HEADER, wzTagNew, wzActivationDate);
    if (!bRet) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    // Create the new section

    bRet = _WriteString(wzActivationDate, SNAPSHOT_DATA_URT_VERSION, wzURTVersion);
    if (!bRet) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

Exit:
    SAFEDELETEARRAY(wzBuffer);
    SAFEDELETEARRAY(wzTagOld);
    SAFEDELETEARRAY(wzTagNew);

    return hr;
}

HRESULT CIniWriter::DeleteSnapShot(LPCWSTR wzActivationDate)
{
    HRESULT                               hr = S_OK;
    LPWSTR                                wzBigBuffer = NULL;
    LPWSTR                                wzSection = NULL;
    DWORD                                 dwRet;
    BOOL                                  bRet;
    LPWSTR                                wzCurStr;

    dwRet = _ReadStringEx(wzActivationDate, NULL, &wzBigBuffer);
    if (!wzBigBuffer) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    wzCurStr = wzBigBuffer;
    while (lstrlen(wzCurStr)) {

        SAFEDELETEARRAY(wzSection);
        dwRet = _ReadStringEx(wzActivationDate, wzCurStr, &wzSection);
        if (!wzSection) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        // Delete the dependent section
        bRet = _WriteString(wzSection, NULL, NULL);
        if (!bRet) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }

        // Iterate

        wzCurStr += (lstrlenW(wzCurStr) + 1);
    }

    // Delete the section itself

    bRet = _WriteString(wzActivationDate, NULL, NULL);
    if (!bRet) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

Exit:
    SAFEDELETEARRAY(wzBigBuffer);
    SAFEDELETEARRAY(wzSection);

    return hr;
}
        
HRESULT CIniWriter::AddAssembly(LPCWSTR wzActivationDate, AsmBindHistoryInfo *pHistInfo)
{
    HRESULT                                  hr = S_OK;
    BOOL                                     bRet;
    LPWSTR                                   wzAsmTag = NULL;
    LPWSTR                                   wzSectionName = NULL;

    if (!wzActivationDate || !pHistInfo) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    wzAsmTag = NEW(WCHAR[MAX_INI_TAG_LENGTH]);
    if (!wzAsmTag) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    wzSectionName = NEW(WCHAR[MAX_INI_TAG_LENGTH]);
    if (!wzSectionName) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    wnsprintfW(wzAsmTag, MAX_INI_TAG_LENGTH, L"%ws%wc%ws%wc%ws%wc%ws", pHistInfo->wzAsmName,
               HISTORY_DELIMITER_CHAR, pHistInfo->wzPublicKeyToken,
               HISTORY_DELIMITER_CHAR, pHistInfo->wzCulture,
               HISTORY_DELIMITER_CHAR, pHistInfo->wzVerReference);
    wnsprintfW(wzSectionName, MAX_INI_TAG_LENGTH, L"%ws%wc%ws", wzActivationDate,
               HISTORY_DELIMITER_CHAR, wzAsmTag);

    // Create the section link to the asm version information
    
    bRet = _WriteString(wzActivationDate, wzAsmTag, wzSectionName);
    if (!bRet) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    // Record version information in the assembly section

    bRet = _WriteString(wzSectionName, ASSEMBLY_DATA_VER_REFERENCE, pHistInfo->wzVerReference);
    if (!bRet) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    bRet = _WriteString(wzSectionName, ASSEMBLY_DATA_VER_APP_CFG, pHistInfo->wzVerAppCfg);
    if (!bRet) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    bRet = _WriteString(wzSectionName, ASSEMBLY_DATA_VER_PUBLISHER_CFG, pHistInfo->wzVerPublisherCfg);
    if (!bRet) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }
    
    bRet = _WriteString(wzSectionName, ASSEMBLY_DATA_VER_ADMIN_CFG, pHistInfo->wzVerAdminCfg);
    if (!bRet) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }
                                                                                                               
Exit:
    SAFEDELETEARRAY(wzSectionName);
    SAFEDELETEARRAY(wzAsmTag);

    return hr;
}

HRESULT CIniWriter::InsertHeaderData(LPCWSTR wzExePath, LPCWSTR wzAppName)
{
    HRESULT                                     hr = S_OK;
    BOOL                                        bRet;

    if (!wzExePath || !wzAppName) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    bRet = _WriteString(HISTORY_SECTION_HEADER, HEADER_DATA_EXE_PATH, wzExePath);
    if (!bRet) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    bRet = _WriteString(HISTORY_SECTION_HEADER, HEADER_DATA_APP_NAME, wzAppName);
    if (!bRet) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

Exit:
    return hr;
}


HINI CIniWriter::GetHIni()
{
    return _hIni;
}

HRESULT CIniWriter::Load()
{
    HRESULT                                     hr = S_OK;

    if (!PAL_IniLoad(_hIni, _pwzFileName))
    {
        hr = FusionpHresultFromLastError();
        goto Exit;
    }

Exit:
    return hr;
}

HRESULT CIniWriter::Save(BOOL fForce)
{
    HRESULT                                     hr = S_OK;

    if (!PAL_IniSave(_hIni, _pwzFileName, fForce))
    {
        hr = FusionpHresultFromLastError();
        goto Exit;
    }

Exit:
    return hr;
}

