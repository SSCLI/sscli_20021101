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
#include "scavenger.h"
#include "asmstrm.h"
#include "fusionheap.h"
#include "cache.h"
#include "naming.h"
#include "util.h"
#include "transprt.h"
#include "cacheutils.h"
#include "enum.h"
#include "list.h"
#include "lock.h"

// global crit-sec for init dbs (reuse, defined at dllmain.cpp)
extern CRITICAL_SECTION g_csInitClb;

DWORD g_ScavengingThreadId=0;
HMODULE g_hFusionMod=0;

#define REG_VAL_FUSION_DOWNLOAD_CACHE_QUOTA_IN_KB           TEXT("DownloadCacheQuotaInKB")
DWORD g_DownloadCacheQuotaInKB;

#define REG_VAL_FUSION_DOWNLOAD_CACHE_USAGE     TEXT("DownloadCacheSize")


HRESULT ScavengeDownloadCache();


class CScavengerNode
{
public:

    CScavengerNode();
    ~CScavengerNode();
    static LONG Compare(CScavengerNode *, CScavengerNode *);
    LPWSTR _pwzManifestPath;
    FILETIME _ftLastAccess;
    FILETIME _ftCreation;
    DWORD _dwAsmSize;
};

CScavengerNode::CScavengerNode()
{
    _pwzManifestPath=NULL;
    _dwAsmSize = 0;
    memset( &_ftLastAccess, 0, sizeof(FILETIME));
    memset( &_ftCreation,   0, sizeof(FILETIME));

}

CScavengerNode::~CScavengerNode()
{
    SAFEDELETEARRAY(_pwzManifestPath);
}

LONG CScavengerNode::Compare(CScavengerNode *pItem1, CScavengerNode *pItem2)
{
    return CompareFileTime( &(pItem1->_ftLastAccess), &(pItem2->_ftLastAccess));
}

//------------------- Cache Scavenging APIs ------------- --------------------



STDAPI CreateScavenger(IUnknown **ppAsmScavenger)
{
    HRESULT                       hr = S_OK;
    IAssemblyScavenger           *pScavenger = NULL;

    if (!ppAsmScavenger) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    pScavenger = NEW(CScavenger);
    if (!pScavenger) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    *ppAsmScavenger = pScavenger;
    (*ppAsmScavenger)->AddRef();

Exit:
    SAFERELEASE(pScavenger);

    return hr;
}

CScavenger::CScavenger()
{
    _cRef = 1;
}

CScavenger::~CScavenger()
{

}

HRESULT GetDownloadUsage(DWORD *pdwDownloadUsageInKB)
{
    HRESULT                         hr=S_OK;
    DWORD                           dwDownloadUsage=0;
    WCHAR szBuf[20];
    LPWSTR endPtr;

    if (!PAL_FetchConfigurationString(FALSE, REG_VAL_FUSION_DOWNLOAD_CACHE_USAGE, szBuf, ARRAYSIZE(szBuf)))
    {
        hr = FusionpHresultFromLastError();
        goto exit;
    }    

    dwDownloadUsage = wcstol(szBuf, &endPtr, 10);

    if (endPtr == szBuf)
    {
        hr = E_FAIL;
        goto exit;
    }

    if(pdwDownloadUsageInKB)
        *pdwDownloadUsageInKB = dwDownloadUsage;

exit:
    return hr;
}

HRESULT SetDownLoadUsage(   /* [in] */ BOOL  bUpdate,
                            /* [in] */ int   dwDownloadUsage)
{
    HRESULT                         hr=S_OK;
    DWORD                           dwCurrDownloadCacheSize=0;
    WCHAR szBuf[20];

    if(bUpdate)
    {
        hr = GetDownloadUsage(&dwCurrDownloadCacheSize);
        dwDownloadUsage += dwCurrDownloadCacheSize;
    }

    if(dwDownloadUsage < 0)
        dwDownloadUsage = 0;

    _snwprintf(szBuf, ARRAYSIZE(szBuf), L"%d", dwDownloadUsage);

    if (!PAL_SetConfigurationString(FALSE, REG_VAL_FUSION_DOWNLOAD_CACHE_USAGE, szBuf))
    {
        hr = FusionpHresultFromLastError();
    }

    return hr;
}

DWORD GetDownloadTarget()
{
    HRESULT hr;
    DWORD dwCurrUsage = 0;

    hr = GetDownloadUsage(&dwCurrUsage);

    if(dwCurrUsage > g_DownloadCacheQuotaInKB)
        return g_DownloadCacheQuotaInKB/2 + 1;
    else
        return 0;

}

HRESULT GetScavengerQuotasFromReg(DWORD *pdwZapQuotaInGAC,
                                  DWORD *pdwDownloadQuotaAdmin,
                                  DWORD *pdwDownloadQuotaUser)
{
    HRESULT                         hr=S_OK;
    DWORD                           dwDownloadLMQuota=0;
    DWORD                           dwDownloadCUQuota=0;
    WCHAR szBuf[20];
    LPWSTR endPtr;

    if (PAL_FetchConfigurationString(TRUE, REG_VAL_FUSION_DOWNLOAD_CACHE_QUOTA_IN_KB, szBuf, ARRAYSIZE(szBuf)))
    {
        dwDownloadLMQuota = wcstol(szBuf, &endPtr, 10);
    }    
    
    if (PAL_FetchConfigurationString(FALSE, REG_VAL_FUSION_DOWNLOAD_CACHE_QUOTA_IN_KB, szBuf, ARRAYSIZE(szBuf)))
    {
        dwDownloadCUQuota = wcstol(szBuf, &endPtr, 10);
    }

    if(!dwDownloadLMQuota)
    {
        g_DownloadCacheQuotaInKB = 50000; // default Download Cache Quota
    }
    else
    {
        g_DownloadCacheQuotaInKB = dwDownloadLMQuota;
    }

    if(dwDownloadCUQuota)
    {
        g_DownloadCacheQuotaInKB = min(dwDownloadCUQuota, g_DownloadCacheQuotaInKB);
    }

    if(pdwZapQuotaInGAC)
         *pdwZapQuotaInGAC = 0;

    if(pdwDownloadQuotaAdmin)
         *pdwDownloadQuotaAdmin = g_DownloadCacheQuotaInKB;

    if(pdwDownloadQuotaUser)
         *pdwDownloadQuotaUser = g_DownloadCacheQuotaInKB;

    return hr;
}

HRESULT  CScavenger::GetCurrentCacheUsage( /* [in] */ DWORD *pdwZapUsage,
                                           /* [in] */ DWORD *pdwDownloadUsage)
{
    if(pdwZapUsage)
        *pdwZapUsage = 0;

    return GetDownloadUsage(pdwDownloadUsage);
}

HRESULT CScavenger::GetCacheDiskQuotas( /* [out] */ DWORD *pdwZapQuotaInGAC,
                                                /* [out] */ DWORD *pdwDownloadQuotaAdmin,
                                                /* [out] */ DWORD *pdwDownloadQuotaUser)
{
    return GetScavengerQuotasFromReg(pdwZapQuotaInGAC, pdwDownloadQuotaAdmin, pdwDownloadQuotaUser);
}

HRESULT CScavenger::SetCacheDiskQuotas(
                            /* [in] */ DWORD dwZapQuotaInGAC,
                            /* [in] */ DWORD dwDownloadQuotaAdmin,
                            /* [in] */ DWORD dwDownloadQuotaUser)
{
    HRESULT                         hr=S_OK;
    DWORD                           dwDownloadQuota=0;
    WCHAR                           szBuf[20];
    BOOL                            bPerMachine = FALSE;

    if(g_CurrUserPermissions == READ_ONLY)
    {
        bPerMachine = FALSE;
        dwDownloadQuota = dwDownloadQuotaUser;
    }
    else
    {
        bPerMachine = TRUE;
        dwDownloadQuota = dwDownloadQuotaAdmin;
    }

    _snwprintf(szBuf, ARRAYSIZE(szBuf), L"%d", dwDownloadQuota);

    if (!PAL_SetConfigurationString(bPerMachine, REG_VAL_FUSION_DOWNLOAD_CACHE_USAGE, szBuf))
    {
        hr = FusionpHresultFromLastError();
    }

    GetScavengerQuotasFromReg(NULL, NULL, NULL);

    return hr;
}

// ---------------------------------------------------------------------------
// CScavenger::ScavengeAssemblyCache
// Flush private cache and if required, scavenge private cache based on LRU.
//---------------------------------------------------------------------------
HRESULT CScavenger::ScavengeAssemblyCache()
{
    return DoScavengingIfRequired( TRUE );
}


HRESULT MoveAllFilesFromDir(LPWSTR pszSrcDirPath, LPWSTR pszDestDirPath)
{
    HRESULT hr = S_OK;
    TCHAR szDestFilePath[MAX_PATH+1];
    TCHAR szBuf[MAX_PATH+1];
    HANDLE hf = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA fd;
    StrCpy(szBuf, pszSrcDirPath);
    StrCat(szBuf, TEXT("\\*"));

    if ((hf = FindFirstFile(szBuf, &fd)) == INVALID_HANDLE_VALUE) 
    {
        hr = FusionpHresultFromLastError();
        goto exit;
    }

    StrCpy(szBuf, pszSrcDirPath);

    do
    {
        if ( (StrCmp(fd.cFileName, TEXT(".")) == 0) || 
             (StrCmp(fd.cFileName, TEXT("..")) == 0))
            continue;

        wnsprintf(szBuf, MAX_PATH-1, TEXT("%s\\%s"), pszSrcDirPath, fd.cFileName);
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) 
        {
            wnsprintf(szDestFilePath, MAX_PATH-1, TEXT("%s\\%s"), pszDestDirPath, fd.cFileName);
            if(!MoveFile( szBuf, szDestFilePath))
            {
                hr = FusionpHresultFromLastError();
#ifdef DEBUG
                WCHAR szMsgBuf[MAX_PATH*2 + 1];
                wnsprintf( szMsgBuf, MAX_PATH*2, L"   MoveFile(%s, %s) FAILED with  <%x> \r\n",
                                              szBuf, szDestFilePath, hr );
                WriteToLogFile(szMsgBuf);
#endif  // DEBUG

                if( (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) )  
                       || (hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND)) )
                {
                    hr = S_OK;
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
        }

    } while (FindNextFile(hf, &fd));

    if ((hr == S_OK) && (GetLastError() != ERROR_NO_MORE_FILES))
    {
        hr = FusionpHresultFromLastError();
        goto exit;
    }

    if (hf != INVALID_HANDLE_VALUE)
    {
        FindClose(hf);
        hf = INVALID_HANDLE_VALUE;
    }

    // after moving all files attempt to remove the source dir
    if (!RemoveDirectory(pszSrcDirPath)) 
    {
        hr = FusionpHresultFromLastError();
#ifdef DEBUG
                WCHAR szMsgBuf[MAX_PATH*2 + 1];
                wnsprintf( szMsgBuf, MAX_PATH*2, L"     RemoveDirectory(%s) FAILED in MoveAllFilesFromDir with  <%x> \r\n",
                                              pszSrcDirPath, hr );
                WriteToLogFile(szMsgBuf);
#endif  // DEBUG

        if( (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) )  
               || (hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND)) )
        {
            hr = S_OK;
        }
    }

exit :

    if (hf != INVALID_HANDLE_VALUE)
    {
        FindClose(hf);
        hf = INVALID_HANDLE_VALUE;
    }

    return hr;
}

HRESULT DeleteAssemblyFiles(DWORD dwCacheFlags, LPCWSTR pszCustomPath, LPWSTR pszManFilePath)
{
    HRESULT hr = S_OK;
    LPTSTR pszTemp=NULL;
    LPWSTR pszManifestPath=pszManFilePath;
    TCHAR szPendDelDirPath[MAX_PATH+1];
    TCHAR szAsmDirPath[MAX_PATH+1];
#define TEMP_PEND_DIR 10

    DWORD dwLen = 0;

    if(!pszManifestPath)
    {
        hr = E_FAIL;
        goto exit;
    }

    dwLen = lstrlen(pszManifestPath);
    ASSERT(dwLen <= MAX_PATH);

    lstrcpy(szAsmDirPath, pszManifestPath);

    pszTemp = PathFindFileName(szAsmDirPath);

    if(pszTemp > szAsmDirPath)
    {
        *(pszTemp-1) = L'\0';
    }

    dwLen = MAX_PATH;
    hr = GetPendingDeletePath( pszCustomPath, dwCacheFlags, szPendDelDirPath, &dwLen);
    if (FAILED(hr)) {
        goto exit;
    }

    if(lstrlen(szPendDelDirPath) + TEMP_PEND_DIR  > MAX_PATH)
    {
        hr = HRESULT_FROM_WIN32(FUSION_E_INVALID_NAME);
        goto exit;
    }

    GetRandomFileName( szPendDelDirPath, TEMP_PEND_DIR);

    if(!MoveFile( szAsmDirPath, szPendDelDirPath))
    {
        hr = FusionpHresultFromLastError();
#ifdef DEBUG
                WCHAR szMsgBuf[MAX_PATH*2 + 1];
                wnsprintf( szMsgBuf, MAX_PATH*2, L"     MoveFile((%s, %s)) FAILED in DeleteAssemblyFiles with  <%x> \r\n",
                                              szAsmDirPath, szPendDelDirPath, hr );
                WriteToLogFile(szMsgBuf);
#endif  // DEBUG

        if( (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) )  
               || (hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND)) )
        {
            hr = S_OK;
            goto exit;
        }
    }
    else
    {
        hr = RemoveDirectoryAndChildren(szPendDelDirPath);
        hr = S_OK; // don't worry about passing back error here. its already in pend-del dir.
        goto exit;
    }

    // looks like there are some in-use files here.
    // move all the asm files to pend del dir.
    if(!CreateDirectory(szPendDelDirPath, NULL))
    {
        hr = FusionpHresultFromLastError();
        goto exit;
    }

    hr = MoveAllFilesFromDir(szAsmDirPath, szPendDelDirPath);

    if(hr == S_OK)
    {
        // assembly deleted successfully delete/pend all temp files.
        hr = RemoveDirectoryAndChildren(szPendDelDirPath);
        hr = S_OK; // don't worry about passing back error here. its already in pend-del dir.
    }
    else
    {
        // could not delete assembly; restore all files back to original state.
#ifdef DEBUG
        HRESULT hrTemp =
#endif
        MoveAllFilesFromDir(szPendDelDirPath, szAsmDirPath);
#ifdef DEBUG
                WCHAR szMsgBuf[MAX_PATH*2 + 1];
                wnsprintf( szMsgBuf, MAX_PATH*2, L"     Restored all files back to <%s> hr = <%x> hrTemp = <%x> \r\n",
                                              szAsmDirPath, hr, hrTemp );
                WriteToLogFile(szMsgBuf);
#endif  // DEBUG

        hr = HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION);
    }

exit :

    if(hr == S_OK)
    {
        pszTemp = PathFindFileName(szAsmDirPath);

        if(pszTemp > szAsmDirPath)
        {
            *(pszTemp-1) = L'\0';
        }

        // now that we have two levels of dirs...try to remove parent dir also
        // this succeeds only if it is empty, don't worry about return value.
        RemoveDirectory(szAsmDirPath);
    }

    return hr;
}

// ---------------------------------------------------------------------------
// CScavenger::DeleteAssembly
//         Deletes the given TransCache entry after deleting bits.
// ---------------------------------------------------------------------------
HRESULT
CScavenger::DeleteAssembly( DWORD dwCacheFlags, LPCWSTR pszCustomPath, LPWSTR pszManFilePath, BOOL bForceDelete)
{
    HRESULT hr = S_OK;
    LPTSTR pszTemp=NULL;
    TCHAR szPendDelDirPath[MAX_PATH+1];
    TCHAR szAsmDirPath[MAX_PATH+1];
#define TEMP_PEND_DIR 10
    DWORD dwLen = 0;
    LPWSTR pszManifestPath=pszManFilePath;

    ASSERT( pszManFilePath);

    if(bForceDelete)
        return DeleteAssemblyFiles( dwCacheFlags, pszCustomPath, pszManFilePath);

    dwLen = lstrlen(pszManifestPath);
    ASSERT(dwLen <= MAX_PATH);

    DWORD dwAttrib;

    if((dwAttrib = GetFileAttributes(pszManifestPath)) == (DWORD) -1)
    {
        if(bForceDelete)
        {
            return DeleteAssemblyFiles( dwCacheFlags, pszCustomPath, pszManFilePath);
        }

        hr = S_FALSE;
        goto exit;
    }

    StrCpy(szAsmDirPath, pszManifestPath);

    if(!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
    {
        // looks manifestFilePath is passed in. knock-off the filename.
        pszTemp = PathFindFileName(szAsmDirPath);

        if(pszTemp > szAsmDirPath)
        {
            *(pszTemp-1) = L'\0';
        }
    }

    dwLen = MAX_PATH;
    hr = GetPendingDeletePath( pszCustomPath, dwCacheFlags, szPendDelDirPath, &dwLen);
    if (FAILED(hr)) {
        goto exit;
    }

    if(lstrlen(szPendDelDirPath) + TEMP_PEND_DIR  > MAX_PATH)
    {
        hr = HRESULT_FROM_WIN32(FUSION_E_INVALID_NAME);
        goto exit;
    }

    GetRandomFileName( szPendDelDirPath, TEMP_PEND_DIR);


    if(!MoveFile( szAsmDirPath, szPendDelDirPath))
    {
        hr = FusionpHresultFromLastError();
        /*
#ifdef DEBUG
                WCHAR szMsgBuf[MAX_PATH*2 + 1];
                wnsprintf( szMsgBuf, MAX_PATH*2, L"     MoveFile((%s, %s)) FAILED in DeleteAssembly with  <%x> \r\n",
                                              szAsmDirPath, szPendDelDirPath, hr );
                WriteToLogFile(szMsgBuf);
#endif  // DEBUG
        */
        if( (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) )  
               || (hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND)) )
        {
            hr = S_OK;
            goto exit;
        }
    }
    else
    {
        hr = RemoveDirectoryAndChildren(szPendDelDirPath);
        hr = S_OK; // don't worry about passing back error here. its already in pend-del dir.
        goto exit;
    }

exit :
    if( (hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED)) ||
           (hr == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION)) )
    {
        // We cannot delete this as someone else has locked it...
        hr = HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION);
    }

    if(hr == S_OK)
    {
        pszTemp = PathFindFileName(szAsmDirPath);

        if(pszTemp > szAsmDirPath)
        {
            *(pszTemp-1) = L'\0';
            RemoveDirectory(szAsmDirPath);
        }
    }

    return hr;
}

// ---------------------------------------------------------------------------
// CScavenger::NukeDowloadedCache()
// scavenging interface
//---------------------------------------------------------------------------
HRESULT CScavenger::NukeDownloadedCache()
{
    HRESULT hr=S_OK;
    WCHAR szCachePath[MAX_PATH+1];
    DWORD dwLen=MAX_PATH;

    if(FAILED(hr = GetCachePath(ASM_CACHE_DOWNLOAD, szCachePath, &dwLen)))
        goto exit;

    // remove the complete downloaded dir. tree
    hr = RemoveDirectoryAndChildren(szCachePath);

    if( (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) )  
           || (hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND)) )
    {
        hr = S_OK;
    }

    if(SUCCEEDED(hr))
    {
        SetDownLoadUsage( FALSE, 0);
    }

exit :
    return hr;
}

//
// IUnknown boilerplate...
//

STDMETHODIMP
CScavenger::QueryInterface(REFIID riid, void** ppvObj)
{
    if (   IsEqualIID(riid, IID_IUnknown)
                || IsEqualIID(riid, IID_IAssemblyScavenger)
       )
    {
        *ppvObj = static_cast<IAssemblyScavenger*> (this);
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
CScavenger::AddRef()
{
    return InterlockedIncrement (&_cRef);
}

STDMETHODIMP_(ULONG)
CScavenger::Release()
{
    ULONG lRet = InterlockedDecrement (&_cRef);
    if (!lRet)
        delete this;
    return lRet;
}

STDAPI NukeDownloadedCache()
{
    return CScavenger::NukeDownloadedCache();
}


HRESULT DeleteAssemblyBits(LPCTSTR pszManifestPath)
{
    HRESULT hr = S_OK;
    TCHAR szPath[MAX_PATH+1];
    DWORD           dwLen = 0;

    if(!pszManifestPath)
        goto exit;

    dwLen = lstrlen(pszManifestPath);
    ASSERT(dwLen <= MAX_PATH);

    lstrcpy(szPath, pszManifestPath);

    // making c:\foo\a.dll -> c:\foo for RemoveDirectoryAndChd()
    while( szPath[dwLen] != '\\' && dwLen > 0 )
        dwLen--;

    if( szPath[dwLen] == '\\')
        szPath[dwLen] = '\0';

    //  remove the disk file
    hr = RemoveDirectoryAndChildren(szPath);

    if( (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) )  
            || (hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND)) )
    {
        // file is not there, this not an error.
        hr = S_OK;
        goto exit;
    }

    if(hr == S_OK)
    {
        // making c:\foo\a.dll -> c:\foo for RemoveDirectory();
        while( szPath[dwLen] != '\\' && dwLen > 0 )
            dwLen--;

        if( szPath[dwLen] == '\\')
            szPath[dwLen] = '\0';

        // now that we have two levels of dirs...try to remove parent dir also
        // this succeeds only if it is empty, don't worry about return value.
        RemoveDirectory(szPath);
    }
#ifdef DEBUG
    else
    {
        WCHAR szMsgBuf[MAX_PATH*2 + 1];
        wnsprintf( szMsgBuf, MAX_PATH*2, L"     DeleteAssemblyBits:  RemoveDirectoryAndChildren FAILED for <%s> hr = <%x> \r\n",
                                      szPath, hr );
        WriteToLogFile(szMsgBuf);
    }
#endif  // DEBUG

exit :
    return hr;
}


HRESULT StartScavenging(LPVOID pSynchronous)
{
    HRESULT            hr = S_OK;
    CCriticalSection   cs(&g_csInitClb);

    hr = ScavengeDownloadCache();

    if(FAILED(hr))
        goto exit;


exit:

    HRESULT hrRet = cs.Lock();
    if (FAILED(hrRet)) {
        return hrRet;
    }

    g_ScavengingThreadId=0;

    cs.Unlock();

    if (!pSynchronous)
        FreeLibraryAndExitThread( g_hFusionMod, hr);

    return S_OK;
}


HRESULT CreateScavengerThread(BOOL bSynchronous)
{
    HRESULT hr=S_OK;
    HRESULT hrCS=S_OK;
    HANDLE hThread=0;
    DWORD dwThreadId=0;
    DWORD Error = 0;
    CCriticalSection cs(&g_csInitClb);

    hrCS = cs.Lock();
    if (FAILED(hrCS)) {
        return hrCS;
    }

    if(g_ScavengingThreadId)
    {
        /*                                                                                   
               */
        hr = S_FALSE;
        goto exit;
    }

    if( bSynchronous )
    {
        g_ScavengingThreadId = GetCurrentThreadId();
        cs.Unlock();
        hr = StartScavenging( (LPVOID)TRUE );
        
        hrCS = cs.Lock();
        if (FAILED(hrCS)) {
            return hrCS;
        }

        g_ScavengingThreadId = 0;
    }
    else
    {
        g_hFusionMod = LoadLibrary(g_FusionDllPath);

        hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) StartScavenging, (LPVOID)FALSE,  0, &dwThreadId);

        if (hThread == NULL)
        {
            Error = GetLastError();
            FreeLibrary(g_hFusionMod);
        }
        else
        {
            g_ScavengingThreadId = dwThreadId;
        }
    }

exit :

    cs.Unlock();

    if(hThread)
        CloseHandle(hThread);

    return hr;
}


HRESULT DoScavengingIfRequired(BOOL bSynchronous)
{
    HRESULT hr = S_OK;

    if(FAILED(hr = CreateCacheMutex()))
    {
        goto exit;
    }

    if(GetDownloadTarget())
        hr = CreateScavengerThread(bSynchronous);

exit:

    return hr;
}

HRESULT FlushOldAssembly(LPCWSTR pszCustomPath, LPWSTR pszAsmDirPath, LPWSTR pszManifestFileName, BOOL bForceDelete)
{
    HRESULT hr = S_OK;
    LPTSTR pszTemp=NULL, pszAsmDirName=NULL;
    TCHAR szParentDirPath[MAX_PATH+1];
    TCHAR szBuf[MAX_PATH+1];
    HANDLE hf = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA fd;

    ASSERT(pszAsmDirPath);

    lstrcpy(szParentDirPath, pszAsmDirPath);

    pszTemp = PathFindFileName(szParentDirPath);

    if(pszTemp > szParentDirPath)
    {
        *(pszTemp-1) = L'\0';
    }
    else
    {
        hr = E_UNEXPECTED;
        goto exit;
    }

    pszAsmDirName = pszTemp;
    StrCpy(szBuf, szParentDirPath);
    StrCat(szBuf, TEXT("\\*"));

    if ((hf = FindFirstFile(szBuf, &fd)) == INVALID_HANDLE_VALUE) 
    {
        hr = FusionpHresultFromLastError();
        goto exit;
    }

    do
    {
        if ( (StrCmp(fd.cFileName, TEXT(".")) == 0) || 
             (StrCmp(fd.cFileName, TEXT("..")) == 0))
            continue;

        if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) 
        {
            if(StrCmp(fd.cFileName, pszAsmDirName))
            {
                wnsprintf(szBuf, MAX_PATH, L"%s\\%s\\%s", szParentDirPath, fd.cFileName, pszManifestFileName);
                hr = CScavenger::DeleteAssembly(ASM_CACHE_DOWNLOAD, pszCustomPath, szBuf, bForceDelete);
#ifdef DEBUG
                if(hr != S_OK)
                {
                    WCHAR szMsgBuf[MAX_PATH*2 + 1];
                    wnsprintf( szMsgBuf, MAX_PATH*2, L"         Flushing <%s> returned  hr = <%x> \r\n",
                                                  szBuf, hr );
                    WriteToLogFile(szMsgBuf);
                }
#endif  // DEBUG

                if(hr != S_OK)
                    goto exit;
            }
        }
        else
        {
        }

    } while (FindNextFile(hf, &fd));

    if((hr == S_OK) && (GetLastError() != ERROR_NO_MORE_FILES))
    {
        hr = FusionpHresultFromLastError();
        goto exit;
    }

exit :

    if(hf != INVALID_HANDLE_VALUE)
    {
        FindClose(hf);
        hf = INVALID_HANDLE_VALUE;
    }

    return hr;
}

HRESULT CleanupTempDir(DWORD dwCacheFlags, LPCWSTR pszCustomPath)
{
    HRESULT hr = S_OK;
    TCHAR szPendDelDirPath[MAX_PATH+1];
    TCHAR szBuf[MAX_PATH+1];
    HANDLE hf = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA fd;

    DWORD dwLen = MAX_PATH;

    hr = GetPendingDeletePath( pszCustomPath, dwCacheFlags, szPendDelDirPath, &dwLen);
    if (FAILED(hr)) {
        goto exit;
    }

    StrCpy(szBuf, szPendDelDirPath);
    StrCat(szBuf, TEXT("\\*"));

    if((hf = FindFirstFile(szBuf, &fd)) == INVALID_HANDLE_VALUE) 
    {
        hr = FusionpHresultFromLastError();
        goto exit;
    }

    do
    {
        if ( (StrCmp(fd.cFileName, TEXT(".")) == 0) || 
             (StrCmp(fd.cFileName, TEXT("..")) == 0))
            continue;

        wnsprintf(szBuf, MAX_PATH-1, TEXT("%s\\%s"), szPendDelDirPath, fd.cFileName);

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            hr = RemoveDirectoryAndChildren(szBuf);
        }
        else
        {
            if(!DeleteFile(szBuf))
                hr = FusionpHresultFromLastError();

        }

    } while (FindNextFile(hf, &fd));

    if((hr == S_OK) && (GetLastError() != ERROR_NO_MORE_FILES))
    {
        hr = FusionpHresultFromLastError();
        goto exit;
    }

exit :

    if(hf != INVALID_HANDLE_VALUE)
    {
        FindClose(hf);
        hf = INVALID_HANDLE_VALUE;
    }

    return hr;
}

HRESULT GetCurrTime(FILETIME *pftCurrTime, DWORD dwSeconds)
{
    FILETIME ftCurrTime;
    ULARGE_INTEGER uliTime;

    HRESULT hr = S_OK;
    TCHAR szTempFilePath[MAX_PATH+1];
    HANDLE hFile = INVALID_HANDLE_VALUE;
#define TEMP_FILE_LEN (15)

    memset(pftCurrTime, 0, sizeof(ULARGE_INTEGER));

    DWORD dwLen = MAX_PATH;
    hr = GetPendingDeletePath( NULL, ASM_CACHE_DOWNLOAD, szTempFilePath, &dwLen);
    if (FAILED(hr)) {
        goto exit;
    }

    dwLen = MAX_PATH - dwLen;

    dwLen = TEMP_FILE_LEN;
    if((dwLen + lstrlenW(szTempFilePath) + 1)> MAX_PATH)
    {
        hr = E_FAIL;
        goto exit;
    }

    GetRandomFileName(szTempFilePath, dwLen);

    hFile = CreateFile(szTempFilePath, GENERIC_WRITE, 0 /* no sharing */,
                     NULL, CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else if(!GetFileTime(hFile, &ftCurrTime, NULL, NULL))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

exit:

    if(FAILED(hr))
    {
        GetSystemTimeAsFileTime(&ftCurrTime);
        hr = S_OK;
    }

    memcpy( &uliTime, &ftCurrTime, sizeof(ULARGE_INTEGER));

    uliTime.QuadPart -= dwSeconds * 10000000;  // 1 second = 10 ** 7 units in SystemTime.
    memcpy(pftCurrTime, &uliTime, sizeof(ULARGE_INTEGER));

    if(hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
        DeleteFile(szTempFilePath);
    }

    return hr;
}

HRESULT EnumFileStore(DWORD dwFlags, DWORD *pdwSizeOfCacheInKB, List<CScavengerNode *> **ppNodeList )
{
    HRESULT         hr = NOERROR;
    CEnumCache*     pEnumR = NULL;
    CTransCache*    pTCQry = NULL;
    CTransCache*    pTCOut= NULL;
    DWORD           dwTotalKBSize = 0;
    DWORD           dwAsmSize=0;
    List<CScavengerNode *>   *pNodeList=NEW(List<CScavengerNode *>);
    CScavengerNode  *pNode;

    ASSERT(ppNodeList);

    *ppNodeList = NULL;

    hr = CTransCache::Create(&pTCQry, dwFlags);
    if( hr != S_OK )
        goto exit;

    pEnumR = NEW(CEnumCache(TRUE, NULL));
    if(!pEnumR)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    hr = pEnumR->Init(pTCQry, 0);
    if (FAILED(hr))
       goto exit;

    while( NOERROR == hr )
    {
        // create temp object
        hr = CTransCache::Create(&pTCOut, dwFlags);

        if( hr != S_OK)
            break;

        hr = pEnumR->GetNextRecord(pTCOut);

        if( S_OK == hr )
        {
            pNode = NEW(CScavengerNode);
            if(!pNode)
            {
                hr = E_OUTOFMEMORY;
                goto exit;
            }

            hr = GetAssemblyKBSize(pTCOut->_pInfo->pwzPath, &dwAsmSize, 
                        &(pNode->_ftLastAccess), &(pNode->_ftCreation));
            dwTotalKBSize += dwAsmSize;
            // build ascending order list.
            pNode->_dwAsmSize = dwAsmSize;
            pNode->_pwzManifestPath = pTCOut->_pInfo->pwzPath;
            pTCOut->_pInfo->pwzPath = NULL;
            pNodeList->AddSorted(pNode, (LPVOID) CScavengerNode::Compare);
            // cleanup
            SAFERELEASE(pTCOut);
        } // nextRecord

    } // while

    if(pdwSizeOfCacheInKB)
        *pdwSizeOfCacheInKB = dwTotalKBSize;

    *ppNodeList = pNodeList;

exit:

    if(!(*ppNodeList))
    {
        SAFEDELETE(pNodeList); // this should call RemoveAll();
    }

    SAFEDELETE(pEnumR);
    SAFERELEASE(pTCOut);
    SAFERELEASE(pTCQry);

    return hr;
}

HRESULT ScavengeDownloadCache()
{
    HRESULT hr = S_OK;
    DWORD dwCacheSizeInKB=0;
    DWORD dwFreedInKB=0;
    List<CScavengerNode *>   *pNodeList=NULL;
    LISTNODE    pAsmList=NULL;
    int iAsmCount=0,i=0;
    CScavengerNode  *pTargetAsm;
    DWORD dwScavengeTo = 0;
    DWORD dwCurrentUsage = 0;
    CMutex  cCacheMutex(g_hCacheMutex);

    hr = CleanupTempDir(ASM_CACHE_DOWNLOAD, NULL);

    hr = EnumFileStore( TRANSPORT_CACHE_SIMPLENAME_IDX, &dwCacheSizeInKB,  &pNodeList);

    if(FAILED(hr = cCacheMutex.Lock()))
        goto exit;

    if(FAILED(hr = SetDownLoadUsage(FALSE, dwCacheSizeInKB)))
        goto exit;

    if(FAILED(hr = cCacheMutex.Unlock()))
    {
        goto exit;
    }

    pAsmList  = pNodeList->GetHeadPosition();
    iAsmCount = pNodeList->GetCount();

    if(!(dwScavengeTo = GetDownloadTarget()))
        goto exit;

    for(i=0; i<iAsmCount; i++)
    {
        if(FAILED(hr = GetDownloadUsage(&dwCurrentUsage)))
            goto exit;

        if(dwCurrentUsage <= dwScavengeTo)
            break;

        pTargetAsm = pNodeList->GetNext(pAsmList); // Element from list;

        if(FAILED(hr = cCacheMutex.Lock()))
            goto exit;

        hr = CScavenger::DeleteAssembly(ASM_CACHE_DOWNLOAD, NULL, pTargetAsm->_pwzManifestPath, FALSE);


        if(SUCCEEDED(hr))
        {
            dwFreedInKB += pTargetAsm->_dwAsmSize;
            SetDownLoadUsage(TRUE, - (int)pTargetAsm->_dwAsmSize);
        }

        if(FAILED(hr = cCacheMutex.Unlock()))
        {
            goto exit;
        }

    }

exit:
    // destroy list.
    if(pNodeList)
    {
        pAsmList  = pNodeList->GetHeadPosition();
        iAsmCount = pNodeList->GetCount();

        for(i=0; i<iAsmCount; i++)
        {
            pTargetAsm = pNodeList->GetNext(pAsmList); // Element from list;
            SAFEDELETE(pTargetAsm);
        }
        pNodeList->RemoveAll();
        SAFEDELETE(pNodeList); // this should call RemoveAll
    }

    return hr;
}

#ifdef DEBUG

HANDLE g_hLogFile;

HRESULT OpenLogFile()
{
#define REG_VAL_FUSION_LOG_FILE    TEXT("FusionLogFile")

    HRESULT hr=E_FAIL;
    HANDLE  hFile = NULL;
    WCHAR       szBuf[MAX_PATH+1];
    if (PAL_FetchConfigurationString(TRUE, REG_VAL_FUSION_LOG_FILE, szBuf, MAX_PATH - 50))
        hr = S_OK;

    if(hr == S_OK)
    {
        wnsprintf(szBuf, MAX_PATH, L"%s_%d.txt", szBuf, GetCurrentProcessId());
        hFile = CreateFile(szBuf, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL,
                             CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) 
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto exit;
        }

        g_hLogFile = hFile;
    }

exit:

    return hr;
}

HRESULT WriteToLogFile( LPWSTR pszMsg)
{
    if(g_hLogFile)
    {
        DWORD cbWritten=0, cbMsgSize = lstrlenW(pszMsg) * sizeof(WCHAR);

        WriteFile(g_hLogFile, pszMsg, cbMsgSize, &cbWritten, NULL);
    }

    return S_OK;
}
#endif

#ifdef DEBUG
    // szMsgBuf[MAX_PATH*2 + 1];
    // wnsprintf( szMsgBuf, MAX_PATH*2, L" msg here " );
    // WriteToLogFile(szMsgBuf);
#endif  // DEBUG
