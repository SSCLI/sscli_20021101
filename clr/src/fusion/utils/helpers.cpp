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
#include "helpers.h"
#include "list.h"
#include "policy.h"
#include "naming.h"
#include "appctx.h"
#include "actasm.h"
#include "util.h"
#include "cacheutils.h"
#include "util.h"
#include "lock.h"
#include "history.h"


#define MAX_DRIVE_ROOT_LEN                     4

extern CRITICAL_SECTION g_csDownload;
extern CRITICAL_SECTION g_csInitClb;

extern HMODULE g_hMSCorEE;

extern PFNSTRONGNAMETOKENFROMPUBLICKEY      g_pfnStrongNameTokenFromPublicKey;
extern PFNSTRONGNAMEERRORINFO               g_pfnStrongNameErrorInfo;
extern PFNSTRONGNAMEFREEBUFFER              g_pfnStrongNameFreeBuffer;
extern PFNSTRONGNAMESIGNATUREVERIFICATION   g_pfnStrongNameSignatureVerification;
extern pfnGetAssemblyMDImport               g_pfnGetAssemblyMDImport;
extern COINITIALIZECOR                      g_pfnCoInitializeCor;
extern pfnGetXMLObject                      g_pfnGetXMLObject;

COINITIALIZECOR g_pfnCoInitializeCor = NULL;
pfnGetCORVersion g_pfnGetCORVersion = NULL;
PFNGETCORSYSTEMDIRECTORY g_pfnGetCorSystemDirectory = NULL;



HRESULT AppCtxGetWrapper(IApplicationContext *pAppCtx, LPWSTR wzTag,
                         WCHAR **ppwzValue)
{
    HRESULT                               hr = S_OK;
    WCHAR                                *wzBuf = NULL;
    DWORD                                 cbBuf;

    if (!pAppCtx || !wzTag || !ppwzValue) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    cbBuf = 0;
    hr = pAppCtx->Get(wzTag, wzBuf, &cbBuf, 0);

    if (hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND)) {
        hr = S_FALSE;
        *ppwzValue = NULL;
        goto Exit;
    }

    ASSERT(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));

    wzBuf = NEW(WCHAR[cbBuf]);
    if (!wzBuf) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    hr = pAppCtx->Get(wzTag, wzBuf, &cbBuf, 0);

    if (FAILED(hr)) {
        *ppwzValue = NULL;
        delete [] wzBuf;
    }
    else {
        *ppwzValue = wzBuf;
    }

Exit:
    return hr;
}

// ---------------------------------------------------------------------------
// NameObjGetWrapper
// ---------------------------------------------------------------------------
HRESULT NameObjGetWrapper(IAssemblyName *pName, DWORD nIdx, 
    LPBYTE *ppbBuf, LPDWORD pcbBuf)
{
    HRESULT hr = S_OK;
    
    LPBYTE pbAlloc;
    DWORD cbAlloc;

    // Get property size
    hr = pName->GetProperty(nIdx, NULL, &(cbAlloc = 0));
    if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
    {
        // Property is set; alloc buf
        pbAlloc = NEW(BYTE[cbAlloc]);
        if (!pbAlloc)
        {
            hr = E_OUTOFMEMORY;                
            goto exit;
        }

        // Get the property
        if (FAILED(hr = pName->GetProperty(nIdx, pbAlloc, &cbAlloc)))
            goto exit;
            
        *ppbBuf = pbAlloc;
        *pcbBuf = cbAlloc;
    }
    else
    {
        // If property unset, hr should be S_OK
        if (hr != S_OK)
            goto exit;

        // Success, returning 0 bytes, ensure buf is null.    
        *ppbBuf = NULL;
    }

    
exit:
    return hr;
}


HRESULT GetFileLastModified(LPCWSTR pwzFileName, FILETIME *pftLastModified)
{
    HRESULT                                hr = S_OK;
    HANDLE                                 hFile = INVALID_HANDLE_VALUE;

    if (!pwzFileName || !pftLastModified) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    hFile = CreateFileW(pwzFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        hr = FusionpHresultFromLastError();
        goto Exit;
    }

    if (!GetFileTime(hFile, NULL, NULL, pftLastModified)) {
        hr = FusionpHresultFromLastError();
    }

Exit:
    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
    }

    return hr;
}


HRESULT SetAppCfgFilePath(IApplicationContext *pAppCtx, LPCWSTR wzFilePath)
{
    HRESULT                              hr = S_OK;
    CApplicationContext                 *pCAppCtx = static_cast<CApplicationContext *>(pAppCtx); // dynamic_cast

    ASSERT(pCAppCtx);

    hr = pCAppCtx->Lock();
    if (FAILED(hr)) {
        goto Exit;
    }

    if (!wzFilePath || !pAppCtx) {
        ASSERT(0);
        pCAppCtx->Unlock();
        hr = E_INVALIDARG;
        goto Exit;
    }

    hr = pAppCtx->Set(ACTAG_APP_CFG_LOCAL_FILEPATH, (void *)wzFilePath,
                      (sizeof(WCHAR) * (lstrlenW(wzFilePath) + 1)), 0);
                      
    pCAppCtx->Unlock();

Exit:
    return hr;
}

HRESULT MakeUniqueTempDirectory(LPCWSTR wzTempDir, LPWSTR wzUniqueTempDir,
                                DWORD dwLen)
{
    int                           n = 1;
    HRESULT                       hr = S_OK;
    CCriticalSection              cs(&g_csInitClb);

    ASSERT(wzTempDir && wzUniqueTempDir);

    //execute entire function under critical section
    hr = cs.Lock();
    if (FAILED(hr)) {
        goto Exit;
    }

    do {

        if (n > 100)    // avoid infinite loop!
            break;

        wnsprintfW(wzUniqueTempDir, dwLen, L"%ws%ws%d.tmp", wzTempDir, L"Fusion", n++);


    } while (GetFileAttributesW(wzUniqueTempDir) != (DWORD) -1);

    if (!CreateDirectoryW(wzUniqueTempDir, NULL)) {
        hr = FusionpHresultFromLastError();
    }

    lstrcatW(wzUniqueTempDir, L"\\");

    cs.Unlock();

Exit:
    return hr;
}

HRESULT RemoveDirectoryAndChildren(LPWSTR szDir)
{
    HRESULT hr = S_OK;
    HANDLE hf = INVALID_HANDLE_VALUE;
    TCHAR szBuf[MAX_PATH];
    WIN32_FIND_DATA fd;
    LPWSTR wzCanonicalized=NULL;
    WCHAR wzPath[MAX_PATH];
    DWORD dwSize;

    if (!szDir || !lstrlenW(szDir)) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    wzPath[0] = L'\0';

    wzCanonicalized = NEW(WCHAR[MAX_URL_LENGTH+1]);
    if (!wzCanonicalized)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    dwSize = MAX_URL_LENGTH;
    hr = UrlCanonicalizeW(szDir, wzCanonicalized, &dwSize, 0);
    if (FAILED(hr)) {
        goto Exit;
    }

    dwSize = MAX_PATH;
    hr = PathCreateFromUrlW(wzCanonicalized, wzPath, &dwSize, 0);
    if (FAILED(hr)) {
        goto Exit;
    }

    // Cannot delete root. Path must have greater length than "x:\"
    if (lstrlenW(wzPath) < 4) {
        ASSERT(0);
        hr = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
        goto Exit;
    }

    if (RemoveDirectory(wzPath)) {
        goto Exit;
    }

    // ha! we have a case where the directory is probbaly not empty

    StrCpy(szBuf, wzPath);
    StrCat(szBuf, TEXT("\\*"));

    if ((hf = FindFirstFile(szBuf, &fd)) == INVALID_HANDLE_VALUE) {
        hr = FusionpHresultFromLastError();
        goto Exit;
    }

    do {

        if ( (StrCmp(fd.cFileName, TEXT(".")) == 0) || 
             (StrCmp(fd.cFileName, TEXT("..")) == 0))
            continue;

        wnsprintf(szBuf, MAX_PATH-1, TEXT("%s\\%s"), wzPath, fd.cFileName);
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

            SetFileAttributes(szBuf, 
                FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_NORMAL);

            if (FAILED((hr=RemoveDirectoryAndChildren(szBuf)))) {
                goto Exit;
            }

        } else {

            SetFileAttributes(szBuf, FILE_ATTRIBUTE_NORMAL);
            if (!DeleteFile(szBuf)) {
                hr = FusionpHresultFromLastError();
#ifdef DEBUG
                if((hr != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
                        && (hr != HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND)))
                {
                WCHAR szMsgBuf[MAX_PATH*2 + 1];
                wnsprintf( szMsgBuf, MAX_PATH*2, L"     RemoveDirectoryAndChildren: DeleteFile(<%s>) failed hr = <%x>\r\n",
                                              szBuf, hr);
                WriteToLogFile(szMsgBuf);
                }
#endif  // DEBUG
                goto Exit;
            }
        }


    } while (FindNextFile(hf, &fd));


    if (GetLastError() != ERROR_NO_MORE_FILES) {

        hr = FusionpHresultFromLastError();
        goto Exit;
    }

    if (hf != INVALID_HANDLE_VALUE) {
        FindClose(hf);
        hf = INVALID_HANDLE_VALUE;
    }

    // here if all subdirs/children removed
    /// re-attempt to remove the main dir
    if (!RemoveDirectory(wzPath)) {
        hr = FusionpHresultFromLastError();
#ifdef DEBUG
                WCHAR szMsgBuf[MAX_PATH*2 + 1];
                wnsprintf( szMsgBuf, MAX_PATH*2, L"     RemoveDirectoryAndChildren: RemoveDirectory(<%s>) failed hr = <%x>\r\n",
                                              wzPath, hr);
                WriteToLogFile(szMsgBuf);
#endif  // DEBUG

        goto Exit;
    }

Exit:
    if(hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED))
        hr = HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION);

    if (hf != INVALID_HANDLE_VALUE)
        FindClose(hf);

    SAFEDELETEARRAY(wzCanonicalized);
    return hr;
}

HRESULT GetPDBName(LPWSTR wzFileName, LPWSTR wzPDBName, DWORD *pdwSize)
{
    LPWSTR                           wzExt = NULL;
    
    ASSERT(wzFileName && wzPDBName && pdwSize);


    lstrcpyW(wzPDBName, wzFileName);
    wzExt = PathFindExtension(wzPDBName);

    lstrcpyW(wzExt, L".PDB");

    return S_OK;
}

STDAPI CopyPDBs(IAssembly *pAsm)
{
    HRESULT                                       hr = S_OK;
    IAssemblyName                                *pName = NULL;
    IAssemblyModuleImport                        *pModImport = NULL;
    DWORD                                         dwSize;
    WCHAR                                         wzAsmCachePath[MAX_PATH];
    WCHAR                                         wzFileName[MAX_PATH];
    WCHAR                                         wzSourcePath[MAX_PATH];
    WCHAR                                         wzPDBName[MAX_PATH];
    WCHAR                                         wzPDBSourcePath[MAX_PATH];
    WCHAR                                         wzPDBTargetPath[MAX_PATH];
    WCHAR                                         wzModPath[MAX_PATH];
    LPWSTR                                        wzCodebase=NULL;
    LPWSTR                                        wzModName = NULL;
    DWORD                                         dwIdx = 0;
    LPWSTR                                        wzTmp = NULL;

    if (!pAsm) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    if (pAsm->GetAssemblyLocation(NULL) == E_NOTIMPL) {
        // This is a registered "known assembly" (ie. the process EXE).
        // We don't copy PDBs for the process EXE because it's never
        // shadow copied.

        hr = S_FALSE;
        goto Exit;
    }

    // Find the source location. Make sure this is a file:// URL (ie. we
    // don't support retrieving the PDB over http://).

    hr = pAsm->GetAssemblyNameDef(&pName);
    if (FAILED(hr)) {
        goto Exit;
    }

    wzCodebase = NEW(WCHAR[MAX_URL_LENGTH+1]);
    if (!wzCodebase)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    dwSize = MAX_URL_LENGTH * sizeof(WCHAR);
    hr = pName->GetProperty(ASM_NAME_CODEBASE_URL, (void *)wzCodebase, &dwSize);
    if (FAILED(hr)) {
        goto Exit;
    }

    if (!UrlIsW(wzCodebase, URLIS_FILEURL)) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    dwSize = MAX_PATH;
    hr = PathCreateFromUrlWrap(wzCodebase, wzSourcePath, &dwSize, 0);
    if (FAILED(hr)) {
        goto Exit;
    }

    wzTmp = PathFindFileName(wzSourcePath);
    ASSERT(wzTmp > (LPWSTR)wzSourcePath);
    *wzTmp = L'\0';
        
   // Find the target location in the cache.
   
    dwSize = MAX_PATH;
    hr = pAsm->GetManifestModulePath(wzAsmCachePath, &dwSize);
    if (FAILED(hr)) {
        goto Exit;
    }

    wzTmp = PathFindFileName(wzAsmCachePath);
    ASSERT(wzTmp > (LPWSTR)wzAsmCachePath);

    StrCpy(wzFileName, wzTmp);
    *wzTmp = L'\0';


    // Copy the manifest PDB.

    // Hack for now
    dwSize = MAX_PATH;
    hr = GetPDBName(wzFileName, wzPDBName, &dwSize);
    if (FAILED(hr)) {
        goto Exit;
    }

    wnsprintfW(wzPDBSourcePath, MAX_PATH, L"%ws%ws", wzSourcePath, wzPDBName);
    wnsprintf(wzPDBTargetPath, MAX_PATH, L"%ws%ws", wzAsmCachePath, wzPDBName);

    if (GetFileAttributes(wzPDBTargetPath) == (DWORD) -1 && lstrcmpiW(wzPDBSourcePath, wzPDBTargetPath)) {
        CopyFile(wzPDBSourcePath, wzPDBTargetPath, TRUE);
    }

    // Copy the module PDBs.

    dwIdx = 0;
    while (SUCCEEDED(hr)) {
        hr = pAsm->GetNextAssemblyModule(dwIdx++, &pModImport);

        if (SUCCEEDED(hr)) {
            if (pModImport->IsAvailable()) {
                dwSize = MAX_PATH;
                hr = pModImport->GetModulePath(wzModPath, &dwSize);
                if (FAILED(hr)) {
                    SAFERELEASE(pModImport);
                    goto Exit;
                }

                wzModName = PathFindFileName(wzModPath);
                ASSERT(wzModName);

                dwSize = MAX_PATH;
                hr = GetPDBName(wzModName, wzPDBName, &dwSize);
                if (FAILED(hr)) {
                    SAFERELEASE(pModImport);
                    goto Exit;
                }

                wnsprintfW(wzPDBSourcePath, MAX_PATH, L"%ws%ws", wzSourcePath,
                           wzPDBName);
                wnsprintfW(wzPDBTargetPath, MAX_PATH, L"%ws%ws", wzAsmCachePath,
                           wzPDBName);

                if (GetFileAttributes(wzPDBTargetPath) == (DWORD) -1 && lstrcmpiW(wzPDBSourcePath, wzPDBTargetPath)) {
                    CopyFile(wzPDBSourcePath, wzPDBTargetPath, TRUE);
                }
            }

            SAFERELEASE(pModImport);
        }
    }

    // Copy complete. Return success.

    if (hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS)) {
        hr = S_OK;
    }

Exit:
    SAFERELEASE(pName);
    SAFEDELETEARRAY(wzCodebase);
    return hr;
}

// ---------------------------------------------------------------------------
// GetCorSystemDirectory
// ---------------------------------------------------------------------------
BOOL GetCorSystemDirectory(LPWSTR szCorSystemDir)
{
    HRESULT                         hr = S_OK;
    BOOL                            fRet = FALSE;
    DWORD                           ccPath = MAX_PATH;

    hr = InitializeEEShim();
    if (FAILED(hr)) {
        goto Exit;
    }

    hr = g_pfnGetCorSystemDirectory(szCorSystemDir, MAX_PATH, &ccPath);
    if (FAILED(hr)) {
        goto Exit;
    }

    fRet = TRUE;

Exit:
    return fRet;
}

// ---------------------------------------------------------------------------
// VerifySignature
// StronNameSignatureVerificationEx call behavior from RudiM 
// and associated fusion actions
//
// Note: fForceVerify assumed FALSE for all of the below:
//
// 1) successful verification of signed assembly
//    Returns TRUE, *pfWasVerified == TRUE.
//    Fusion action: Allow cache commit.
//
// 2) unsuccessful verification of fully signed assembly
//    Returns FALSE, StrongNameErrorInfo() returns NTE_BAD_SIGNATURE (probably), 
//    *pfWasVerified == Undefined.
//    Fusion action: Fail cache commit.
//
// 3) successful verification of delay signed assembly
//    Returns TRUE, *pfWasVerified == FALSE.
//    Fusion action: Allow cache commit, mark entry so that signature 
//    verification is performed on retrieval.
//
// 4) unsuccessful verification of delay signed assembly
//    (Assuming fForceVerify == FALSE): returns FALSE, StrongNameErrorInfo() 
//    some error code other than NTE_BAD_SIGNATURE, *pfWasVerified == Undefined.
// ---------------------------------------------------------------------------
BOOL VerifySignature(LPWSTR szFilePath, LPBOOL pfWasVerified, DWORD dwFlags)
{    
    HRESULT                         hr = S_OK;
    DWORD                           dwFlagsOut = 0;
    BOOL                            fRet = FALSE;

    // Initialize crypto if necessary. 

    hr = InitializeEEShim();
    if (FAILED(hr)) {
        goto exit;
    }

    // Verify the signature
    if (!g_pfnStrongNameSignatureVerification(szFilePath, dwFlags, &dwFlagsOut)) {
        goto exit;
    }

    if (pfWasVerified) {
        *pfWasVerified = ((dwFlagsOut & SN_OUTFLAG_WAS_VERIFIED) != 0);
    }

    fRet = TRUE;

exit:

    return fRet;
}

// ---------------------------------------------------------------------------
// CreateFilePathHierarchy
// ---------------------------------------------------------------------------
HRESULT CreateFilePathHierarchy( LPCOLESTR pszName )
{
    HRESULT hr=S_OK;
    LPTSTR pszFileName;
    TCHAR szPath[MAX_PATH];

    // Assert (pszPath ) ;
    StrCpy (szPath, pszName);

    pszFileName = PathFindFileName ( szPath );

    if ( pszFileName <= szPath )
        return E_INVALIDARG; // Send some error 

    *(pszFileName-1) = 0;

    DWORD dw = GetFileAttributes( szPath );
    if ( dw != (DWORD) -1 )
        return S_OK;
    
    hr = FusionpHresultFromLastError();

    switch (hr)
    {
        case HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND):
            {
                hr =  CreateFilePathHierarchy(szPath);
                if (hr != S_OK)
                    return hr;
            }

        case HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND):
            {
                if ( CreateDirectory( szPath, NULL ) )
                    return S_OK;
                else
                {
                    hr = FusionpHresultFromLastError();
                    if(hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS))
                        hr = S_OK;
                    else
                        return hr;
                }
            }

        default:
            return hr;
    }
}

// ---------------------------------------------------------------------------
// Helper function to generate random name.
DWORD GetRandomName (LPTSTR szDirName, DWORD dwLen)
{
    static unsigned Counter;

    for (DWORD i = 0; i < dwLen; i++)
    {
        DWORD dwRand = (GetTickCount() + Counter++);
        BYTE bRand = (BYTE) (dwRand % 36);

        // 10 digits + 26 letters
        if (bRand < 10)
            *szDirName++ = TEXT('0') + bRand;
        else
            *szDirName++ = TEXT('A') + bRand - 10;
    }

    *szDirName = 0;

    return dwLen; // returns length not including null
}

HRESULT GetRandomFileName(LPTSTR pszPath, DWORD dwFileName)
{
    HRESULT hr=S_OK;
    LPTSTR  pszFileName=NULL;
    DWORD dwPathLen = 0;
    DWORD dwErr=0;

    ASSERT(pszPath);
    //ASSERT(IsPathRelative(pszPath))

    StrCat (pszPath, TEXT("\\") );
    dwPathLen = lstrlen(pszPath);

    ASSERT( (dwPathLen + dwFileName) < MAX_PATH);

    pszFileName = pszPath + dwPathLen;

    // Loop until we get a unique file name.
    while (1)
    {
        GetRandomName (pszFileName, dwFileName);
        if (GetFileAttributes(pszPath) != (DWORD) -1)
                    continue;

        dwErr = GetLastError();                
        if (dwErr == ERROR_FILE_NOT_FOUND)
        {
            hr = S_OK;
            break;
        }

        if (dwErr == ERROR_PATH_NOT_FOUND)
        {
            if(FAILED(hr = CreateFilePathHierarchy(pszPath)))
                break;
            else
                continue;
        }

        hr = HRESULT_FROM_WIN32(dwErr);
        break;
    }

    return hr;

}

// ---------------------------------------------------------------------------
// Creates a new Dir for assemblies
HRESULT CreateDirectoryForAssembly
   (IN DWORD dwDirSize, IN OUT LPTSTR pszPath, IN OUT LPDWORD pcwPath)
{
    HRESULT hr=S_OK;
    DWORD dwErr;
    DWORD cszStore;
    LPTSTR pszDir=NULL;

    // Check output buffer can contain a full path.
    ASSERT (!pcwPath || *pcwPath >= MAX_PATH);

    if (!pszPath)
    {
        *pcwPath = MAX_PATH;
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        goto done;
    }


    cszStore = lstrlen (pszPath);
    pszDir = pszPath + cszStore;

    // Loop until we create a unique dir.
    while (1)
    {
        GetRandomName (pszDir, dwDirSize);

        hr = CreateFilePathHierarchy(pszPath);
        if(hr != S_OK)
            goto done;

        if (CreateDirectory (pszPath, NULL))
            break;
        dwErr = GetLastError();
        if (dwErr == ERROR_ALREADY_EXISTS)
            continue;
        hr = HRESULT_FROM_WIN32(dwErr);
        goto done;
    }

    *pcwPath = cszStore + dwDirSize + 1;
    hr = S_OK;

done:
    return hr;
}

HRESULT VersionFromString(LPCWSTR wzVersionIn, WORD *pwVerMajor, WORD *pwVerMinor,
                          WORD *pwVerBld, WORD *pwVerRev)
{
    HRESULT                                  hr = S_OK;
    LPWSTR                                   wzVersion = NULL;
    WCHAR                                   *pchStart = NULL;
    WCHAR                                   *pch = NULL;
    WORD                                    *pawVersions[4] = {pwVerMajor, pwVerMinor, pwVerBld, pwVerRev};
    int                                      i;

    if (!wzVersionIn || !pwVerMajor || !pwVerMinor || !pwVerRev || !pwVerBld) {
        hr = E_INVALIDARG;
        goto Exit;
    }                          

    wzVersion = WSTRDupDynamic(wzVersionIn);
    if (!wzVersion) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    pchStart = wzVersion;
    pch = wzVersion;

    *pwVerMajor = 0;
    *pwVerMinor = 0;
    *pwVerRev = 0;
    *pwVerBld = 0;

    for (i = 0; i < 4; i++) {

        while (*pch && *pch != L'.') {
            pch++;
        }
    
        if (i < 3) {
            if (!*pch) {
                // Badly formatted string
                hr = E_UNEXPECTED;
                goto Exit;
            }

            *pch++ = L'\0';
        }
    
        *(pawVersions[i]) = (WORD)StrToIntW(pchStart);
        pchStart = pch;
    }

Exit:
    SAFEDELETEARRAY(wzVersion);

    return hr;
}

HRESULT FusionGetUserFolderPath(LPWSTR pszPath)
{
    if (!PAL_GetUserConfigurationDirectory(pszPath, MAX_PATH)) {
        DWORD dwError = GetLastError();
        return HRESULT_FROM_WIN32(dwError);
    }

    return S_OK;
}


DWORD HashString(LPCWSTR wzKey, DWORD dwHashSize, BOOL bCaseSensitive)
{
    DWORD                                 dwHash = 0;
    DWORD                                 dwLen;
    DWORD                                 i;

    ASSERT(wzKey);

    dwLen = lstrlenW(wzKey);
    for (i = 0; i < dwLen; i++) {
        if (bCaseSensitive) {
            dwHash = (dwHash * 65599) + (DWORD)wzKey[i];
        }
        else {
            dwHash = (dwHash * 65599) + (DWORD)TOLOWER(wzKey[i]);
        }
    }

    dwHash %= dwHashSize;

    return dwHash;
}


HRESULT ExtractXMLAttribute(LPWSTR *ppwzValue, XML_NODE_INFO **aNodeInfo,
                            USHORT *pCurIdx, USHORT cNumRecs)
{
    HRESULT                                  hr = S_OK;
    LPWSTR                                   pwzCurBuf = NULL;

    ASSERT(ppwzValue && aNodeInfo && pCurIdx && cNumRecs);

    // There shouldn't really be a previous value, but clear just to be safe.

    SAFEDELETEARRAY(*ppwzValue);

    (*pCurIdx)++;
    while (*pCurIdx < cNumRecs) {
        
        if (aNodeInfo[*pCurIdx]->dwType == XML_PCDATA ||
            aNodeInfo[*pCurIdx]->dwType == XML_ENTITYREF) {

            hr = AppendString(&pwzCurBuf, aNodeInfo[*pCurIdx]->pwcText,
                              aNodeInfo[*pCurIdx]->ulLen);
            if (FAILED(hr)) {
                goto Exit;
            }
        }
        else {
            // Reached end of data
            break;
        }

        (*pCurIdx)++;
    }

    if (!pwzCurBuf || !lstrlenW(pwzCurBuf)) {
        *ppwzValue = NULL;

        goto Exit;
    }

    *ppwzValue = WSTRDupDynamic(pwzCurBuf);
    if (!*ppwzValue) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

Exit:
    SAFEDELETEARRAY(pwzCurBuf);

    return hr;
}

HRESULT AppendString(LPWSTR *ppwzHead, LPCWSTR pwzTail, DWORD dwLen)
{
    HRESULT                                    hr = S_OK;
    LPWSTR                                     pwzBuf = NULL;
    DWORD                                      dwLenBuf;
    
    ASSERT(ppwzHead && pwzTail);

    if (!*ppwzHead) {
        *ppwzHead = NEW(WCHAR[dwLen + 1]);

        if (!*ppwzHead) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
 
        // StrCpyN takes length in chars *including* NULL char

        StrCpyNW(*ppwzHead, pwzTail, dwLen + 1);
    }
    else {
        dwLenBuf = lstrlenW(*ppwzHead) + dwLen + 1;

        pwzBuf = NEW(WCHAR[dwLenBuf]);
        if (!pwzBuf) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        StrCpyW(pwzBuf, *ppwzHead);
        StrNCatW(pwzBuf, pwzTail, dwLen + 1);

        SAFEDELETEARRAY(*ppwzHead);

        *ppwzHead = pwzBuf;
    }

Exit:
    return hr;
}


LPWSTR GetNextDelimitedString(LPWSTR *ppwzList, WCHAR wcDelimiter)
{
    LPWSTR                         wzCurString = NULL;
    LPWSTR                         wzPos = NULL;

    if (!ppwzList) {
        goto Exit;
    }

    wzCurString = *ppwzList;
    wzPos = *ppwzList;

    while (*wzPos && *wzPos != wcDelimiter) {
        wzPos++;
    }

    if (*wzPos == wcDelimiter) {
        // Found a delimiter
        *wzPos = L'\0';
        *ppwzList = (wzPos + 1);
    }
    else {
        // End of string
        *ppwzList = NULL;
    }

Exit:
    return wzCurString;
}

HRESULT FusionpHresultFromLastError()
{
    HRESULT hr = S_OK;
    DWORD dwLastError = GetLastError();
    if (dwLastError != NO_ERROR)
    {
        hr = HRESULT_FROM_WIN32(dwLastError);
    }
    else
    {
        hr = E_FAIL;
    }
    return hr;
}

HRESULT GetCORVersion(LPWSTR pbuffer, DWORD *dwLength)
{
    HRESULT                             hr = S_OK;

    if (!dwLength) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    hr = InitializeEEShim();
    if (FAILED(hr)) {
        goto Exit;
    }

    hr = (*g_pfnGetCORVersion)(pbuffer, *dwLength, dwLength);
    if (FAILED(hr)) {
        goto Exit;
    }

Exit:
    return hr;
}

HRESULT InitializeEEShim()
{
    HRESULT                              hr = S_OK;
    HMODULE                              hMod;
    HMODULE                              hSnMod;
    CCriticalSection                     cs(&g_csInitClb);

    if (!g_hMSCorEE) {
        hr = cs.Lock();
        if (FAILED(hr)) {
            goto Exit;
        }

        if (!g_hMSCorEE) {
            hMod = LoadLibrary(MSCOREE_SHIM_W);
            if (!hMod) {
                hr = HRESULT_FROM_WIN32(GetLastError());
                cs.Unlock();
                goto Exit;
            }

#if PLATFORM_UNIX
            hSnMod = LoadLibrary(MAKEDLLNAME(L"mscorsn"));
            if (!hSnMod) {
                hr = HRESULT_FROM_WIN32(GetLastError());
                cs.Unlock();
                goto Exit;
            }
#else   // PLATFORM_UNIX
            hSnMod = hMod;
#endif  // PLATFORM_UNIX

            g_pfnGetCorSystemDirectory = (PFNGETCORSYSTEMDIRECTORY)GetProcAddress(hMod, "GetCORSystemDirectory");
            g_pfnGetCORVersion = (pfnGetCORVersion)GetProcAddress(hMod, "GetCORVersion");
            g_pfnStrongNameTokenFromPublicKey = (PFNSTRONGNAMETOKENFROMPUBLICKEY)GetProcAddress(hSnMod, "StrongNameTokenFromPublicKey");
            g_pfnStrongNameErrorInfo = (PFNSTRONGNAMEERRORINFO)GetProcAddress(hSnMod, "StrongNameErrorInfo");
            g_pfnStrongNameFreeBuffer = (PFNSTRONGNAMEFREEBUFFER)GetProcAddress(hSnMod, "StrongNameFreeBuffer");
            g_pfnStrongNameSignatureVerification = (PFNSTRONGNAMESIGNATUREVERIFICATION)GetProcAddress(hSnMod, "StrongNameSignatureVerification");
            g_pfnGetAssemblyMDImport = (pfnGetAssemblyMDImport)GetProcAddress(hMod, "GetAssemblyMDImport");
            g_pfnCoInitializeCor = (COINITIALIZECOR) GetProcAddress(hMod, "CoInitializeCor");
            g_pfnGetXMLObject = (pfnGetXMLObject)GetProcAddress(hMod, "GetXMLObject");

            if (!g_pfnGetCorSystemDirectory || !g_pfnGetCORVersion || !g_pfnStrongNameTokenFromPublicKey ||
                !g_pfnStrongNameErrorInfo || !g_pfnStrongNameFreeBuffer || !g_pfnStrongNameSignatureVerification ||
                !g_pfnGetAssemblyMDImport || !g_pfnCoInitializeCor || !g_pfnGetXMLObject) {

                hr = HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
                cs.Unlock();
                goto Exit;
            }

            hr = (*g_pfnCoInitializeCor)(COINITCOR_DEFAULT);
            if (FAILED(hr)) {
                cs.Unlock();
                goto Exit;
            }

            // Interlocked exchange guarantees memory barrier
            
            InterlockedExchangePointer((void **)&g_hMSCorEE, hMod);
        }

        cs.Unlock();
    }

Exit:
    return hr;
}







DWORD GetFileSizeInKB(DWORD dwFileSizeLow, DWORD dwFileSizeHigh)
{
    const ULONG dwKBMask = (1023); // 1024-1
    ULONG   dwFileSizeInKB = dwFileSizeLow >> 10 ; // strip of 10 LSB bits to convert from bytes to KB.

    if(dwKBMask & dwFileSizeLow)
        dwFileSizeInKB++; // Round up to the next KB.

    if(dwFileSizeHigh)
        dwFileSizeInKB += (dwFileSizeHigh * (1 << 22) );

    return dwFileSizeInKB;
}



HRESULT GetManifestFileLock( LPWSTR pszFilename, HANDLE *phFile)
{
    HRESULT                                hr = S_OK;
    HANDLE                                 hFile = INVALID_HANDLE_VALUE;
    DWORD                                  dwShareMode = FILE_SHARE_READ;

    ASSERT(pszFilename);

        dwShareMode |= FILE_SHARE_DELETE;        

    hFile = CreateFile(pszFilename, GENERIC_READ, dwShareMode, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                       NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }

    if(phFile)
    {
        *phFile = hFile;
        hFile = INVALID_HANDLE_VALUE;
    }

exit:

    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
    }

    return hr;

}

DWORD PAL_IniReadStringEx(HINI hIni, LPCWSTR lpAppName, LPCWSTR lpKeyName,
                                 LPWSTR *ppwzReturnedString)
{
    DWORD                                        dwRet;
    LPWSTR                                       pwzBuf = NULL;
    int                                          iSizeCur = INI_READ_BUFFER_SIZE;


    for (;;) {
        pwzBuf = NEW(WCHAR[iSizeCur]);
        if (!pwzBuf) {
            dwRet = 0;
            *ppwzReturnedString = NULL;
            goto Exit;
        }
    
        dwRet = PAL_IniReadString(hIni, lpAppName, lpKeyName, 
                                        pwzBuf, iSizeCur);
        if (lpAppName && lpKeyName && dwRet == (DWORD) (iSizeCur - 1)) {
            SAFEDELETEARRAY(pwzBuf);
            iSizeCur += INI_READ_BUFFER_SIZE;
        }
        else if ((!lpAppName || !lpKeyName) && dwRet == (DWORD) (iSizeCur - 2)) {
            SAFEDELETEARRAY(pwzBuf);
            iSizeCur += INI_READ_BUFFER_SIZE;
        }
        else {
            break;
        }
    }

    *ppwzReturnedString = pwzBuf;

Exit:
    return dwRet;
}

HRESULT UpdatePublisherPolicyTimeStampFile(IAssemblyName *pName)
{
    HRESULT                                 hr = S_OK;
    DWORD                                   dwSize;
    HANDLE                                  hFile = INVALID_HANDLE_VALUE;
    WCHAR                                   wzTimeStampFile[MAX_PATH + 1];
    WCHAR                                   wzAsmName[MAX_PATH];

    ASSERT(pName);

    // If the name of the assembly begins with "policy." then update
    // the publisher policy timestamp file.

    wzAsmName[0] = L'\0';

    dwSize = MAX_PATH;
    hr = pName->GetProperty(ASM_NAME_NAME, wzAsmName, &dwSize);
    if (FAILED(hr)) {
        goto Exit;
    }

    if (StrCmpNI(wzAsmName, POLICY_ASSEMBLY_PREFIX, lstrlenW(POLICY_ASSEMBLY_PREFIX))) {
        // No work needs to be done

        goto Exit;
    }

    // Touch the file

    dwSize = MAX_PATH;
    hr = GetCachePath(ASM_CACHE_GAC, wzTimeStampFile, &dwSize);
    if (lstrlenW(wzTimeStampFile) + lstrlenW(FILENAME_PUBLISHER_PCY_TIMESTAMP) + 1 >= MAX_PATH) {
        hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
        goto Exit;
    }
        
    PathRemoveBackslash(wzTimeStampFile);
    lstrcatW(wzTimeStampFile, FILENAME_PUBLISHER_PCY_TIMESTAMP);

    hFile = CreateFileW(wzTimeStampFile, GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile == INVALID_HANDLE_VALUE) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

Exit:
    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
    }
    
    return hr;
} 

void FusionFormatGUID(GUID guid, LPWSTR pszBuf, DWORD cchSize)
{

    ASSERT(pszBuf && cchSize);

    wnsprintf(pszBuf,  cchSize, L"{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
            guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1],
            guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
}

BOOL PathIsRelativeWrap(LPCWSTR pwzPath)
{
    BOOL                             bRet = FALSE;
    
    ASSERT(pwzPath);

    if (pwzPath[0] == L'\\' || pwzPath[0] == L'/') {
        goto Exit;
    }

    if (PathIsURLW(pwzPath)) {
        goto Exit;
    }

    bRet = PathIsRelativeW(pwzPath);

Exit:
    return bRet;
}

//
// URL Combine madness from shlwapi:
//
//   \\server\share\ + Hello%23 = file://server/share/Hello%23 (left unescaped)
//   d:\a b\         + bin      = file://a%20b/bin
//
        
HRESULT UrlCombineUnescape(LPCWSTR pszBase, LPCWSTR pszRelative, LPWSTR pszCombined, LPDWORD pcchCombined, DWORD dwFlags)
{
    HRESULT                                   hr = S_OK;
    DWORD                                     dwSize;
    LPWSTR                                    pwzCombined = NULL;
    LPWSTR                                    pwzFileCombined = NULL;

    pwzCombined = NEW(WCHAR[MAX_URL_LENGTH]);
    if (!pwzCombined) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }
    // If we're just combining an absolute file path to an relative file
    // path, do this by concatenating the strings, and canonicalizing it.
    // This avoids UrlCombine randomness where you could end up with
    // a partially escaped (and partially unescaped) resulting URL!

    if (!PathIsURLW(pszBase) && PathIsRelativeWrap(pszRelative)) {
        pwzFileCombined = NEW(WCHAR[MAX_URL_LENGTH]);
        if (!pwzFileCombined) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        wnsprintfW(pwzFileCombined, MAX_URL_LENGTH, L"%ws%ws", pszBase, pszRelative);

        hr = UrlCanonicalizeUnescape(pwzFileCombined, pszCombined, pcchCombined, 0);
        goto Exit;
    }
    else {
        dwSize = MAX_URL_LENGTH;
        hr = UrlCombineW(pszBase, pszRelative, pwzCombined, &dwSize, dwFlags);
        if (FAILED(hr)) {
            goto Exit;
        }
    }

    // Don't unescape if the relative part was already an URL because
    // URLs wouldn't have been escaped during the UrlCombined.

    if (UrlIsW(pwzCombined, URLIS_FILEURL)) {
        hr = UrlUnescapeW(pwzCombined, pszCombined, pcchCombined, 0);
        if (FAILED(hr)) {
            goto Exit;
        }
    }
    else {
        if (*pcchCombined >= dwSize) {
            lstrcpyW(pszCombined, pwzCombined);
        }

        *pcchCombined = dwSize;
    }

Exit:
    SAFEDELETEARRAY(pwzCombined);
    SAFEDELETEARRAY(pwzFileCombined);

    return hr;
}

HRESULT UrlCanonicalizeUnescape(LPCWSTR pszUrl, LPWSTR pszCanonicalized, LPDWORD pcchCanonicalized, DWORD dwFlags)
{
    HRESULT                                   hr = S_OK;
    DWORD                                     dwSize;
    WCHAR                                     wzCanonical[MAX_URL_LENGTH];

    if (UrlIsW(pszUrl, URLIS_FILEURL) || !PathIsURLW(pszUrl)) {
        dwSize = MAX_URL_LENGTH;
        hr = UrlCanonicalizeW(pszUrl, wzCanonical, &dwSize, dwFlags);
        if (FAILED(hr)) {
            goto Exit;
        }

        hr = UrlUnescapeW(wzCanonical, pszCanonicalized, pcchCanonicalized, 0);
        if (FAILED(hr)) {
            goto Exit;
        }
    }
    else {
        hr = UrlCanonicalizeW(pszUrl, pszCanonicalized, pcchCanonicalized, dwFlags /*| URL_ESCAPE_PERCENT*/);
    }

Exit:
    return hr;
}


HRESULT PathCreateFromUrlWrap(LPCWSTR pszUrl, LPWSTR pszPath, LPDWORD pcchPath, DWORD dwFlags)
{
    HRESULT                                     hr = S_OK;
    DWORD                                       dw;
    WCHAR                                       wzEscaped[MAX_URL_LENGTH];

    if (!UrlIsW(pszUrl, URLIS_FILEURL)) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    dw = MAX_URL_LENGTH;
    hr = UrlEscapeW(pszUrl, wzEscaped, &dw, URL_ESCAPE_PERCENT);
    if (FAILED(hr)) {
        goto Exit;
    }

    hr = PathCreateFromUrlW(wzEscaped, pszPath, pcchPath, dwFlags);
    if (FAILED(hr)) {
        goto Exit;
    }

Exit:
    return hr;
}

#define FILE_URL_PREFIX              L"file://"

LPWSTR StripFilePrefix(LPWSTR pwzURL)
{
    LPWSTR                         szCodebase = pwzURL;

    ASSERT(pwzURL);

    if (!StrCmpNIW(szCodebase, FILE_URL_PREFIX, lstrlenW(FILE_URL_PREFIX))) {
        szCodebase += lstrlenW(FILE_URL_PREFIX);

        if (*(szCodebase + 1) == L':') {
            
            goto Exit;
        }

        if (*szCodebase == L'/') {
            szCodebase++;
        }
        else {
            /* UNC Path, go back two characters to preserve \\ */

            szCodebase -= 2;

            LPWSTR    pwzTmp = szCodebase;

            while (*pwzTmp) {
                if (*pwzTmp == L'/') {
                    *pwzTmp = L'\\';
                }

                pwzTmp++;
            }
        }
    }

Exit:
    return szCodebase;
}

    
