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
#ifndef __HELPERS_H_INCLUDED__
#define __HELPERS_H_INCLUDED__

#include "xmlparser.h"

#define FROMHEX(a) ((a)>=L'a' ? a - L'a' + 10 : a - L'0')
#define TOLOWER(a) (((a) >= L'A' && (a) <= L'Z') ? (L'a' + (a - L'A')) : (a))


#define MAX_URL_LENGTH                     2084 // same as INTERNET_MAX_URL_LENGTH

HRESULT AppCtxGetWrapper(IApplicationContext *pAppCtx, LPWSTR wzTag,
                         WCHAR **ppwzValue);
HRESULT NameObjGetWrapper(IAssemblyName *pName, DWORD nIdx, 
                          LPBYTE *ppbBuf, LPDWORD pcbBuf);
HRESULT GetFileLastModified(LPCWSTR pwzFileName, FILETIME *pftLastModified);
HRESULT SetAppCfgFilePath(IApplicationContext *pAppCtx, LPCWSTR wzFilePath);

HRESULT MakeUniqueTempDirectory(LPCWSTR wzTempDir, LPWSTR wzUniqueTempDir,
                                DWORD dwLen);
HRESULT CreateFilePathHierarchy( LPCOLESTR pszName );
DWORD GetRandomName (LPTSTR szDirName, DWORD dwLen);
HRESULT CreateDirectoryForAssembly
   (IN DWORD dwDirSize, IN OUT LPTSTR pszPath, IN OUT LPDWORD pcwPath);
HRESULT RemoveDirectoryAndChildren(LPWSTR szDir);
STDAPI CopyPDBs(IAssembly *pAsm);
HRESULT VersionFromString(LPCWSTR wzVersion, WORD *pwVerMajor, WORD *pwVerMinor,
                          WORD *pwVerBld, WORD *pwVerRev);

BOOL VerifySignature(LPWSTR szFilePath, LPBOOL pfWasVerified, DWORD dwFlags);
HRESULT FusionGetUserFolderPath(LPWSTR pszPath);
BOOL GetCorSystemDirectory(LPWSTR szCorSystemDir);
DWORD HashString(LPCWSTR wzKey, DWORD dwHashSize, BOOL bCaseSensitive = TRUE);
HRESULT ExtractXMLAttribute(LPWSTR *ppwzValue, XML_NODE_INFO **aNodeInfo,
                            USHORT *pCurIdx, USHORT cNumRecs);
HRESULT AppendString(LPWSTR *ppwzHead, LPCWSTR pwzTail, DWORD dwLen);


LPWSTR GetNextDelimitedString(LPWSTR *ppwzList, WCHAR wcDelimiter);
HRESULT GetCORVersion(LPWSTR pbuffer, DWORD *dwLength);
HRESULT InitializeEEShim();

HRESULT FusionpHresultFromLastError();
HRESULT GetRandomFileName(LPTSTR pszPath, DWORD dwFileName);

HRESULT GetManifestFileLock( LPWSTR pszManifestFile, HANDLE *phFile);

DWORD PAL_IniReadStringEx(HINI hIni, LPCWSTR lpAppName, LPCWSTR lpKeyName,
                                 LPWSTR *ppwzReturnedString);

HRESULT UpdatePublisherPolicyTimeStampFile(IAssemblyName *pName);

void FusionFormatGUID(GUID guid, LPWSTR pszBuf, DWORD cchSize);

HRESULT UrlCanonicalizeUnescape(LPCWSTR pszUrl, LPWSTR pszCanonicalized, LPDWORD pcchCanonicalized, DWORD dwFlags);
HRESULT UrlCombineUnescape(LPCWSTR pszBase, LPCWSTR pszRelative, LPWSTR pszCombined, LPDWORD pcchCombined, DWORD dwFlags);
HRESULT PathCreateFromUrlWrap(LPCWSTR pszUrl, LPWSTR pszPath, LPDWORD pcchPath, DWORD dwFlags);
LPWSTR StripFilePrefix(LPWSTR pwzURL);


inline BOOL IsHosted() { return FALSE; }




#endif

