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
#include "asmcache.h"
#include "asmitem.h"
#include "naming.h"
#include "debmacro.h"
#include "appctx.h"
#include "helpers.h"
#include "asm.h"
#include "asmimprt.h"
#include "policy.h"
#include "dbglog.h"
#include "scavenger.h"
#include "util.h"
#include "cache.h"
#include "cacheutils.h"


extern BOOL g_bRunningOnNT;

// ---------------------------------------------------------------------------
// ValidateAssembly
// ---------------------------------------------------------------------------
HRESULT ValidateAssembly(LPCTSTR pszManifestFilePath, IAssemblyName *pName)
{
    HRESULT                    hr = S_OK;
    BYTE                       abCurHash[MAX_HASH_LEN];
    BYTE                       abFileHash[MAX_HASH_LEN];
    DWORD                      cbModHash;
    DWORD                      cbFileHash;
    DWORD                      dwAlgId;
    WCHAR                      wzDir[MAX_PATH+1];
    LPWSTR                     pwzTmp = NULL;
    WCHAR                      wzModName[MAX_PATH+1];
    WCHAR                      wzModPath[MAX_PATH+1];
    DWORD                      idx = 0;
    DWORD                      cbLen=0;
    IAssemblyManifestImport   *pManifestImport=NULL;
    IAssemblyModuleImport     *pCurModImport = NULL;


    hr = GetFileAttributes(pszManifestFilePath);
    if (hr == -1) 
    {
        hr = FusionpHresultFromLastError();
        goto exit;
    }

    if (FAILED(hr = CreateAssemblyManifestImport((LPTSTR)pszManifestFilePath, &pManifestImport))) 
    {
        goto exit;
    }

    // Integrity checking
    // Walk all modules to make sure they are there (and are valid)

    lstrcpyW(wzDir, pszManifestFilePath);
    pwzTmp = PathFindFileName(wzDir);
    *pwzTmp = L'\0';

    while (SUCCEEDED(hr = pManifestImport->GetNextAssemblyModule(idx++, &pCurModImport)))
    {
        cbLen = MAX_PATH;
        if (FAILED(hr = pCurModImport->GetModuleName(wzModName, &cbLen)))
            goto exit;

        wnsprintfW(wzModPath, MAX_PATH, L"%s%s", wzDir, wzModName);
        hr = GetFileAttributes(wzModPath);
        if (hr == -1)
        {
                hr = FusionpHresultFromLastError();
                goto exit;
        }

        // Get the hash of this module from manifest
        if(FAILED(hr = pCurModImport->GetHashAlgId(&dwAlgId)))
            goto exit;

        cbModHash = MAX_HASH_LEN; 
        if(FAILED(hr = pCurModImport->GetHashValue(abCurHash, &cbModHash)))
            goto exit;

        cbFileHash = MAX_HASH_LEN;
        if(FAILED(hr = GetHash(wzModPath, (ALG_ID)dwAlgId, abFileHash, &cbFileHash)))
            goto exit;

        if ((cbModHash != cbFileHash) || !CompareHashs(cbModHash, abCurHash, abFileHash)) 
        {
            hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
            goto exit;
        }

        SAFERELEASE(pCurModImport);
    }

    if (hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS)) 
    {
        hr = S_OK;
    }

exit:

    SAFERELEASE(pManifestImport);
    SAFERELEASE(pCurModImport);

    return hr;
}


// ---------------------------------------------------------------------------
// CopyAssemblyFile
// ---------------------------------------------------------------------------
HRESULT CopyAssemblyFile
    (IAssemblyCacheItem *pasm, LPCOLESTR pszSrcFile, DWORD dwFormat)
{
    HRESULT hr;
    IStream* pstm    = NULL;
    HANDLE hf        = INVALID_HANDLE_VALUE;
    LPBYTE pBuf      = NULL;
    DWORD cbBuf      = 0x4000;
    DWORD cbRootPath = 0;
    TCHAR *pszName   = NULL;
    
    // Find root path length
    pszName = PathFindFileName(pszSrcFile);

    cbRootPath = (DWORD) (pszName - pszSrcFile);
    ASSERT(cbRootPath < MAX_PATH);
    
    hr = pasm->CreateStream (0, pszSrcFile+cbRootPath, 
        dwFormat, 0, &pstm, NULL);

    if (hr != S_OK)
        goto exit;

    pBuf = NEW(BYTE[cbBuf]);
    if (!pBuf)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    
    DWORD dwWritten, cbRead;
    hf = CreateFile (pszSrcFile, GENERIC_READ, FILE_SHARE_READ,
        NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hf == INVALID_HANDLE_VALUE)
    {
        hr = FusionpHresultFromLastError();
        goto exit;
    }

    while (ReadFile (hf, pBuf, cbBuf, &cbRead, NULL) && cbRead)
    {
        hr = pstm->Write (pBuf, cbRead, &dwWritten);
        if (hr != S_OK)
            goto exit;
    }

    hr = pstm->Commit(0);
    if (hr != S_OK)
        goto exit;

exit:

    SAFERELEASE(pstm);
    SAFEDELETEARRAY(pBuf);

    if (hf != INVALID_HANDLE_VALUE)
    {
        CloseHandle (hf);
    }
    return hr;
}





/*--------------------- CAssemblyCache defines -----------------------------*/


CAssemblyCache::CAssemblyCache()
{
    _dwSig = 0x434d5341; /* 'CMSA' */
    _cRef = 1;
}

CAssemblyCache::~CAssemblyCache()
{

}


STDMETHODIMP CAssemblyCache::InstallAssembly( // if you use this, fusion will do the streaming & commit.
        /* [in] */ DWORD dwFlags,
        /* [in] */ LPCWSTR pszManifestFilePath, 
        /* [in] */ LPCFUSION_INSTALL_REFERENCE pRefData)
{
    HRESULT                            hr=S_OK;
    LPWSTR                             szFullCodebase=NULL;
    LPWSTR                             szFullManifestFilePath=NULL;
    DWORD                              dwLen=0;
    DWORD                              dwIdx = 0;
    WCHAR                              wzDir[MAX_PATH+1];
    WCHAR                              wzModPath[MAX_PATH+1];
    WCHAR                              wzModName[MAX_PATH+1];
    LPWSTR                             pwzTmp = NULL;

    IAssemblyManifestImport           *pManifestImport=NULL;
    IAssemblyModuleImport             *pModImport = NULL;
    CAssemblyCacheItem                *pAsmItem    = NULL;
    FILETIME                          ftLastModTime;

    if(!IsGACWritable())
    {
        return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    }

    /*
    if(!pRefData)
    {
        hr = E_INVALIDARG;
        goto exit;
    }
    */

    DWORD dwCommitFlags=0;

    hr = GetFileAttributes(pszManifestFilePath);
    if (hr == -1) {
        hr = FusionpHresultFromLastError();
        goto exit;
    }

    if(FAILED(hr = GetFileLastModified((LPWSTR) pszManifestFilePath, &ftLastModTime)))
        goto exit;

    szFullCodebase = NEW(WCHAR[MAX_URL_LENGTH*2+2]);
    if (!szFullCodebase)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    szFullManifestFilePath = szFullCodebase + MAX_URL_LENGTH +1;

    if (PathIsRelative(pszManifestFilePath)
#if !PLATFORM_UNIX
          || ((PathGetDriveNumber(pszManifestFilePath) == -1) && !PathIsUNC(pszManifestFilePath))
#endif
       )
    {
        // szPath is relative! Combine this with the CWD
        // Canonicalize codebase with CWD if needed.
        TCHAR szCurrentDir[MAX_PATH+1];

        if (!GetCurrentDirectory(MAX_PATH, szCurrentDir)) {
            hr = FusionpHresultFromLastError();
            goto exit;
        }

        if(!IsPathSeparator(szCurrentDir[lstrlenW(szCurrentDir)-1]))
        {
            // Add trailing backslash
            PathAddBackslash(szCurrentDir);
        }

        dwLen = MAX_URL_LENGTH;
        hr = UrlCombineUnescape(szCurrentDir, pszManifestFilePath, szFullCodebase, &dwLen, 0);
        if (FAILED(hr)) {
            goto exit;
        }

        if(lstrlen(szCurrentDir) + lstrlen(pszManifestFilePath) > MAX_URL_LENGTH)
        {
            hr = E_FAIL;
            goto exit;
        }
        
        if(!PathCombine(szFullManifestFilePath, szCurrentDir, pszManifestFilePath))
        {
            hr = FUSION_E_INVALID_NAME;
            goto exit;
        }

    }
    else 
    {
        dwLen = MAX_URL_LENGTH;
        hr = UrlCanonicalizeUnescape(pszManifestFilePath, szFullCodebase, &dwLen, 0);
        if (FAILED(hr)) {
            goto exit;
        }

        StrCpy(szFullManifestFilePath, pszManifestFilePath);
    }

    if (FAILED(hr = CreateAssemblyManifestImport((LPTSTR)szFullManifestFilePath, &pManifestImport))) 
    {
        goto exit;
    }

    lstrcpyW(wzDir, szFullManifestFilePath);
    pwzTmp = PathFindFileName(wzDir);
    *pwzTmp = L'\0';


    // Create the assembly cache item.
    if (FAILED(hr = CAssemblyCacheItem::Create(NULL, NULL, (LPTSTR) szFullCodebase, 
        &ftLastModTime, ASM_CACHE_GAC, pManifestImport, NULL,
        (IAssemblyCacheItem**) &pAsmItem)))
        goto exit;    


    // Copy to cache.
    if (FAILED(hr = CopyAssemblyFile (pAsmItem, szFullManifestFilePath, 
        STREAM_FORMAT_MANIFEST)))
        goto exit;

    while (SUCCEEDED(hr = pManifestImport->GetNextAssemblyModule(dwIdx++, &pModImport))) 
    {
        dwLen = MAX_PATH;
        hr = pModImport->GetModuleName(wzModName, &dwLen);

        if (FAILED(hr))
        {
                goto exit;
        }

        wnsprintfW(wzModPath, MAX_PATH, L"%s%s", wzDir, wzModName);
        hr = GetFileAttributes(wzModPath);
        if (hr == -1)
        {
                hr = FUSION_E_ASM_MODULE_MISSING;
                goto exit;
        }

        // Copy to cache.
        if (FAILED(hr = CopyAssemblyFile (pAsmItem, wzModPath, 0)))
            goto exit;

        SAFERELEASE(pModImport);
    }

    // don't enforce this flag for now. i.e always replace bits.
    // if(dwFlags & IASSEMBLYCACHE_INSTALL_FLAG_REFRESH)
    {
        dwCommitFlags |= IASSEMBLYCACHEITEM_COMMIT_FLAG_REFRESH; 
    }

    if(dwFlags & IASSEMBLYCACHE_INSTALL_FLAG_FORCE_REFRESH)
    {
        dwCommitFlags |= IASSEMBLYCACHEITEM_COMMIT_FLAG_FORCE_REFRESH; 
    }

    //  Do a force install. This will delete the existing entry(if any)
    if (FAILED(hr = pAsmItem->Commit(dwCommitFlags, NULL)))
    {
        goto exit;        
    }


    CleanupTempDir(ASM_CACHE_GAC, NULL);

exit:

    SAFERELEASE(pAsmItem);
    SAFERELEASE(pModImport);
    SAFERELEASE(pManifestImport);
    SAFEDELETEARRAY(szFullCodebase);
    return hr;
}

STDMETHODIMP CAssemblyCache::UninstallAssembly(
        /* [in] */ DWORD dwFlags,
        /* [in] */ LPCWSTR pszAssemblyName, 
        /* [in] */ LPCFUSION_INSTALL_REFERENCE pRefData, 
        /* [out, optional] */ ULONG *pulDisposition)
{
    HRESULT hr=S_OK;
    IAssemblyName *pName = NULL;
    CTransCache *pTransCache = NULL;
    CCache *pCache = NULL;
    DWORD dwCacheFlags;
    BOOL bHasActiveRefs = FALSE;
    BOOL bRefNotFound = FALSE;

    if(!IsGACWritable())
    {
        return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    }

    if(!pszAssemblyName)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    if (FAILED(hr = CCache::Create(&pCache, NULL)))
        goto exit;

    if (FAILED(hr = CreateAssemblyNameObject(&pName, pszAssemblyName, CANOF_PARSE_DISPLAY_NAME, 0)))
        goto exit;

    dwCacheFlags = CCache::IsCustom(pName) ? ASM_CACHE_ZAP : ASM_CACHE_GAC;

    hr = pCache->RetrieveTransCacheEntry(pName, dwCacheFlags, &pTransCache);

        if ((hr != S_OK) && (hr != DB_S_FOUND))
            goto exit;


    hr = CScavenger::DeleteAssembly(pTransCache->GetCacheType(), NULL,
                                    pTransCache->_pInfo->pwzPath, FALSE);

    if(FAILED(hr))
        goto exit;

    if (SUCCEEDED(hr) && dwCacheFlags == ASM_CACHE_GAC) {
        // If we uninstalled a policy assembly, touch the last modified
        // time of the policy timestamp file.
        UpdatePublisherPolicyTimeStampFile(pName);
    }

    CleanupTempDir(pTransCache->GetCacheType(), NULL);

exit:

    if(pulDisposition)
    {
        *pulDisposition = 0;
        if(bRefNotFound)
        {
            *pulDisposition |= IASSEMBLYCACHE_UNINSTALL_DISPOSITION_REFERENCE_NOT_FOUND;
            hr = S_FALSE;
        }
        else if(bHasActiveRefs)
        {
            *pulDisposition |= IASSEMBLYCACHE_UNINSTALL_DISPOSITION_HAS_INSTALL_REFERENCES;
            hr = S_FALSE;
        }
        else if(hr == S_OK)
        {
            *pulDisposition |= IASSEMBLYCACHE_UNINSTALL_DISPOSITION_UNINSTALLED;
        }
        else if(hr == S_FALSE)
        {
            *pulDisposition |= IASSEMBLYCACHE_UNINSTALL_DISPOSITION_ALREADY_UNINSTALLED;
        }
        else if(hr == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION))
        {
            *pulDisposition |= IASSEMBLYCACHE_UNINSTALL_DISPOSITION_STILL_IN_USE;
        }
    }

    SAFERELEASE(pTransCache);
    SAFERELEASE(pCache);
    SAFERELEASE(pName);
    return hr;
}

STDMETHODIMP CAssemblyCache::QueryAssemblyInfo(
        /* [in] */ DWORD dwFlags,
        /* [in] */ LPCWSTR pszAssemblyName,
        /* [in, out] */ ASSEMBLY_INFO *pAsmInfo)
{
    HRESULT hr = S_OK;
    LPTSTR  pszPath=NULL;
    DWORD   cbPath=0;
    IAssemblyName *pName = NULL;

    CTransCache *pTransCache = NULL;
    CCache *pCache = NULL;

        if (FAILED(hr = CreateAssemblyNameObject(&pName, pszAssemblyName, CANOF_PARSE_DISPLAY_NAME, 0)))
                goto exit;

    if (FAILED(hr = CCache::Create(&pCache, NULL)))
            goto exit;

    hr = pCache->RetrieveTransCacheEntry(pName, CCache::IsCustom(pName) ? ASM_CACHE_ZAP : ASM_CACHE_GAC, &pTransCache);
    if((hr != S_OK) && (hr != DB_S_FOUND) )
        goto exit;

    pszPath = pTransCache->_pInfo->pwzPath;

    // Check Asm hash
    if ( dwFlags & QUERYASMINFO_FLAG_VALIDATE)
        hr = ValidateAssembly(pszPath, pName);

    if(pAsmInfo && SUCCEEDED(hr))
    {
        LPWSTR szPath = pAsmInfo->pszCurrentAssemblyPathBuf;

       // if requested return the assembly path in cache.
        cbPath = lstrlen(pszPath);

        if(szPath && (pAsmInfo->cchBuf > cbPath))
        {
                StrCpy(szPath, pszPath );
        }
        else
        {
                hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }

        pAsmInfo->cchBuf =  cbPath+1;

        pAsmInfo->dwAssemblyFlags = ASSEMBLYINFO_FLAG_INSTALLED;

        if(dwFlags & QUERYASMINFO_FLAG_GETSIZE)
        {
            hr = GetAssemblyKBSize(pTransCache->_pInfo->pwzPath, &(pTransCache->_pInfo->dwKBSize), NULL, NULL);

            pAsmInfo->uliAssemblySizeInKB.QuadPart = pTransCache->_pInfo->dwKBSize;
        }
    }

exit:

    if (hr == DB_S_NOTFOUND) {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    if(hr == DB_S_FOUND)
        hr = S_OK;

    SAFERELEASE(pTransCache);
    SAFERELEASE(pCache);
    SAFERELEASE(pName);
    
    return hr;
}

STDMETHODIMP   CAssemblyCache::CreateAssemblyCacheItem(
        /* [in] */ DWORD dwFlags,
        /* [in] */ PVOID pvReserved,
        /* [out] */ IAssemblyCacheItem **ppAsmItem,
        /* [in, optional] */ LPCWSTR pszAssemblyName)  // uncanonicalized, comma separted name=value pairs.
{

    if(!ppAsmItem)
        return E_INVALIDARG;

    return CAssemblyCacheItem::Create(NULL, NULL, NULL, NULL, ASM_CACHE_GAC, NULL, pszAssemblyName, ppAsmItem);

}


STDMETHODIMP  CAssemblyCache::CreateAssemblyScavenger(
        /* [out] */ IUnknown **ppAsmScavenger )
{

    if(!ppAsmScavenger)
        return E_INVALIDARG;

    return CreateScavenger( ppAsmScavenger );
}

//
// IUnknown boilerplate...
//

STDMETHODIMP
CAssemblyCache::QueryInterface(REFIID riid, void** ppvObj)
{
    if (   IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, IID_IAssemblyCache)
       )
    {
        *ppvObj = static_cast<IAssemblyCache*> (this);
        AddRef();
        return S_OK;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(ULONG)
CAssemblyCache::AddRef()
{
    return InterlockedIncrement (&_cRef);
}

STDMETHODIMP_(ULONG)
CAssemblyCache::Release()
{
    ULONG lRet = InterlockedDecrement (&_cRef);
    if (!lRet)
        delete this;
    return lRet;
}


STDAPI CreateAssemblyCache(IAssemblyCache **ppAsmCache,
                           DWORD dwReserved)
{
    HRESULT                       hr = S_OK;

    if (!ppAsmCache) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    *ppAsmCache = NEW(CAssemblyCache);

    if (!ppAsmCache) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

Exit:
    return hr;
}    
