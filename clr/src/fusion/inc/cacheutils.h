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
#ifndef _CACHEUTILS_H_
#define _CACHEUTILS_H_

#include "transprt.h"

HRESULT CheckAccessPermissions();

HRESULT SetCurrentUserPermissions();
BOOL IsGACWritable();
HRESULT GetAssemblyStagingPath(LPCTSTR pszCustomPath, DWORD dwCacheFlags,
                               BOOL bUser, LPTSTR pszPath, DWORD *pcchSize);

HRESULT CreateAssemblyDirPath( LPCTSTR pszCustomPath, DWORD dwInstaller, DWORD dwCacheFlags,
                               BOOL bUser, LPTSTR pszPath, DWORD *pcchSize);

HRESULT GetPendingDeletePath(LPCTSTR pszCustomPath, DWORD dwCacheFlags,
                               LPTSTR pszPath, DWORD *pcchSize);

HRESULT SetPerUserDownloadDir();

HRESULT GetGACDir(LPWSTR *pszGACDir);
HRESULT GetZapDir(LPWSTR *pszZapDir);
HRESULT GetDownloadDir(LPWSTR *pszDownLoadDir);
HRESULT SetDownLoadDir();

HRESULT GetAssemblyParentDir( CTransCache *pTransCache, LPWSTR pszParentDir);
HRESULT ParseDirName( CTransCache *pTransCache, LPWSTR pszParentDir, LPWSTR pszAsmDir);
HRESULT RetrieveFromFileStore( CTransCache *pTransCache );

HRESULT GetCacheDirsFromName(IAssemblyName *pName, 
    DWORD dwFlags, LPWSTR pszAsmTextName, LPWSTR pszSubDirName);

DWORD GetStringHash(LPCWSTR wzKey);
DWORD GetBlobHash(PBYTE pbKey, DWORD dwLen);

HRESULT StoreFusionInfo(IAssemblyName *pName, LPWSTR pszDir, DWORD *pdwFileSizeLow);
HRESULT GetFusionInfo(CTransCache *pTC, LPWSTR pszAsmDir);

HRESULT GetAssemblyKBSize(LPWSTR pszManifestPath, DWORD *pdwSizeinKB, LPFILETIME pftLastAccess, LPFILETIME pftCreation);

LPWSTR GetManifestFileNameFromURL(LPWSTR pszURL);

#endif // _CACHEUTILS_H_
