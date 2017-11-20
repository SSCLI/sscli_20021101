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
#include <winbase.h>
#include <winerror.h>
#include <shlwapi.h>
#include "fusionp.h"
#include "naming.h"
#include "debmacro.h"
#include "appctx.h"
#include "list.h"
#include "actasm.h"
#include "policy.h"
#include "fusionheap.h"
#include "helpers.h"
#include "history.h"
#include "asm.h"
#include "nodefact.h"
#include "lock.h"


extern CRITICAL_SECTION g_csDownload;
extern CRITICAL_SECTION g_csInitClb;


// ---------------------------------------------------------------------------
// CApplicationContext::CreateEntry
// 
// Private func; Allocates and copies data input.
// ---------------------------------------------------------------------------
HRESULT CApplicationContext::CreateEntry(LPTSTR szName, LPVOID pvValue, 
    DWORD cbValue, DWORD dwFlags, Entry** ppEntry)
{
    HRESULT hr = S_OK;
    DWORD cbName;

    // Allocate the entry.
    Entry *pEntry = NEW(Entry);
    if (!pEntry)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    
    // Copy name.
    cbName = (lstrlen(szName) + 1) * sizeof(TCHAR);
    pEntry->_szName = NEW(TCHAR[cbName]);
    if (!pEntry->_szName)
    {
        SAFEDELETE(pEntry);
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    memcpy(pEntry->_szName, szName, cbName);

    // Allocate and copy data, don't free existing data.
    hr = CopyData(pEntry, pvValue, cbValue, dwFlags, FALSE);
    if (FAILED(hr)) {
        SAFEDELETE(pEntry);
        goto exit;
    }
    
    *ppEntry = pEntry;

exit:
    return hr;
}

CApplicationContext::Entry::Entry()
: _szName(NULL)
, _pbValue(NULL)
, _cbValue(0)
, _dwFlags(0)
{
    _dwSig = 0x59544e45; /* 'YTNE' */
}

// ---------------------------------------------------------------------------
// CApplicationContext::Entry dtor
// ---------------------------------------------------------------------------
CApplicationContext::Entry::~Entry()
{
    if (_dwFlags & APP_CTX_FLAGS_INTERFACE)
        ((IUnknown*) _pbValue)->Release();       
    else
        SAFEDELETEARRAY(_pbValue);

    SAFEDELETEARRAY(_szName);
}



// ---------------------------------------------------------------------------
// CApplicationContext::CopyData
//
// Private func; used to create and update entries.
// ---------------------------------------------------------------------------
HRESULT CApplicationContext::CopyData(Entry *pEntry, LPVOID pvValue, 
    DWORD cbValue, DWORD dwFlags, BOOL fFree)
{
    HRESULT hr = S_OK;

    if (fFree)
    {
        // Cleanup if pre-existing.
        if (pEntry->_dwFlags & APP_CTX_FLAGS_INTERFACE)
            ((IUnknown*) pEntry->_pbValue)->Release();       
        else
            SAFEDELETEARRAY(pEntry->_pbValue);
    }

    // Input is straight blob.
    if (!(dwFlags & APP_CTX_FLAGS_INTERFACE))
    {
        pEntry->_pbValue = NEW(BYTE[cbValue]);
        if (!pEntry->_pbValue)
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }
        memcpy(pEntry->_pbValue, pvValue, cbValue);    
        pEntry->_cbValue = cbValue;
    }
    // Input is Interface ptr.
    else
    {
        pEntry->_pbValue = (LPBYTE) pvValue;
        pEntry->_cbValue = sizeof(IUnknown*);
        ((IUnknown*) pEntry->_pbValue)->AddRef();
    }

    pEntry->_dwFlags = dwFlags;

exit:
    return hr;
}


// ---------------------------------------------------------------------------
// CreateApplicationContext
// ---------------------------------------------------------------------------
STDAPI
CreateApplicationContext(
    IAssemblyName *pName,
    LPAPPLICATIONCONTEXT *ppCtx)
{
    HRESULT hr;
    CApplicationContext *pCtx = NULL;

    if (!ppCtx)
    {
        hr = E_INVALIDARG;
        goto exit;
    }
 
    pCtx = NEW(CApplicationContext);
    if (!pCtx)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    hr = pCtx->Init(pName);

    if (FAILED(hr)) 
    {
        SAFERELEASE(pCtx);
        goto exit;
    }
    
    *ppCtx = pCtx;

exit:
    return hr;
}

// ---------------------------------------------------------------------------
// CApplicationContext ctor
// ---------------------------------------------------------------------------
CApplicationContext::CApplicationContext()
{
    _dwSig = 0x58544341; /* 'XTCA' */
    memset(&_List, 0, sizeof(SERIALIZED_LIST));
    _pName = NULL;
    _cRef = 0;
    _bInitialized = FALSE;
}

// ---------------------------------------------------------------------------
// CApplicationContext dtor
// ---------------------------------------------------------------------------
CApplicationContext::~CApplicationContext()
{
    HRESULT                               hr;
    DWORD                                 dwSize;
    HANDLE                                hFile = NULL;
    CRITICAL_SECTION                     *pcs = NULL;
    CBindHistory                         *pBindHistory = NULL;

    Entry *pEntry = NULL;

    if (_bInitialized) {
        DeleteCriticalSection(&_cs);
    }

        
    // Release the config downloader crit sect

    dwSize = sizeof(CRITICAL_SECTION *);
    hr = Get(ACTAG_APP_CFG_DOWNLOAD_CS, &pcs, &dwSize, 0);
    if (hr == S_OK) {
        hr = Set(ACTAG_APP_CFG_DOWNLOAD_CS, NULL, 0, 0);
        ASSERT(hr == S_OK);

        DeleteCriticalSection(pcs);
        SAFEDELETE(pcs);
    }

    // Release lock on app.cfg

    dwSize = sizeof(HANDLE);
    hr = Get(ACTAG_APP_CFG_FILE_HANDLE, (void *)&hFile, &dwSize, 0);
    if (hr == S_OK && hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
    }


    // Delete bind history object

    dwSize = sizeof(CBindHistory *);
    hr = Get(ACTAG_APP_BIND_HISTORY, (void *)&pBindHistory, &dwSize, 0);
    if (hr == S_OK) {
        SAFEDELETE(pBindHistory);
    }

    // Release associated IAssemblyName*
    SAFERELEASE(_pName);

    // Destruct list, destruct entries.
    while (_List.ElementCount)
    {
        pEntry = (Entry*) HeadOfSerializedList(&_List);    
        RemoveFromSerializedList(&_List, pEntry);
        delete pEntry;
    }

    // Free up list resources.
    TerminateSerializedList(&_List);
    
}

// ---------------------------------------------------------------------------
// CApplicationContext::Init
// ---------------------------------------------------------------------------
HRESULT CApplicationContext::Init(LPASSEMBLYNAME pName)
{
    HRESULT                                      hr = S_OK;
    CLoadContext                                *pLoadCtxDefault = NULL;
    CLoadContext                                *pLoadCtxLoadFrom = NULL;

    PAL_TRY {
        InitializeCriticalSection(&_cs);
        _bInitialized = TRUE;
    }
    PAL_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
        hr = E_OUTOFMEMORY;
    }
    PAL_ENDTRY

    if (FAILED(hr)) {
        goto Exit;
    }

    // Init list. 
    InitializeSerializedList(&_List);

    // Set name if any.
    _pName = pName;

    if (_pName) {
        _pName->AddRef();
    }

    hr = CLoadContext::Create(&pLoadCtxDefault, LOADCTX_TYPE_DEFAULT);
    if (FAILED(hr)) {
        goto Exit;
    }

    hr = CLoadContext::Create(&pLoadCtxLoadFrom, LOADCTX_TYPE_LOADFROM);
    if (FAILED(hr)) {
        goto Exit;
    }

    hr = Set(ACTAG_LOAD_CONTEXT_DEFAULT, pLoadCtxDefault, sizeof(pLoadCtxDefault), APP_CTX_FLAGS_INTERFACE);
    if (FAILED(hr)) {
        goto Exit;
    }

    hr = Set(ACTAG_LOAD_CONTEXT_LOADFROM, pLoadCtxLoadFrom, sizeof(pLoadCtxLoadFrom), APP_CTX_FLAGS_INTERFACE);
    if (FAILED(hr)) {
        goto Exit;
    }

    // Success

    _cRef = 1;
    
Exit:
    SAFERELEASE(pLoadCtxDefault);
    SAFERELEASE(pLoadCtxLoadFrom);

    return hr;
}


// ---------------------------------------------------------------------------
// CApplicationContext::SetContextNameObject
// ---------------------------------------------------------------------------
STDMETHODIMP
CApplicationContext::SetContextNameObject(LPASSEMBLYNAME pName)
{
    // Free existing name if any
    SAFERELEASE(_pName);

    // Set name.
    _pName = pName;
    if (_pName)
        _pName->AddRef();
    return S_OK;
}

// ---------------------------------------------------------------------------
// CApplicationContext::GetContextNameObject
// ---------------------------------------------------------------------------
STDMETHODIMP
CApplicationContext::GetContextNameObject(LPASSEMBLYNAME *ppName)
{
    if (!ppName)
        return E_INVALIDARG;

    *ppName = _pName;

    if (*ppName)
        (*ppName)->AddRef();

    return S_OK;
}

// ---------------------------------------------------------------------------
// CApplicationContext::Set
// ---------------------------------------------------------------------------
STDMETHODIMP
CApplicationContext::Set(LPCOLESTR szName, LPVOID pvValue, 
    DWORD cbValue, DWORD dwFlags)
{
    HRESULT                                     hr = S_OK;
    BOOL                                        fUpdate = FALSE;
    Entry                                      *pEntry;
    LONG                                        i;
    
    if (!szName) {
        hr = E_INVALIDARG;
        goto exit;
    }


    // If a setting the app name or cache base (ie. anything that affects the
    // cache directory), then make sure the any previously cached CCache
    // objects are released (ie. so they get rebuilt with the right new
    // location on next use.

    if (!lstrcmpiW(ACTAG_APP_NAME, szName) || !lstrcmpiW(ACTAG_APP_CACHE_BASE, szName)) {
        hr = Set(ACTAG_APP_CACHE, 0, 0, 0);
        if (FAILED(hr)) {
            goto exit;
        }
    }
    
    // If interface ptr, byte count is optional.
    if (!cbValue && (dwFlags & APP_CTX_FLAGS_INTERFACE))
        cbValue = sizeof(IUnknown*);
        
    // Grab crit sect.
    LockSerializedList(&_List);
    PAL_TRY {
        // Validate input.
        if (!pvValue && cbValue)
        {
            ASSERT(FALSE);
            hr = E_INVALIDARG;
            PAL_LEAVE;
        }
    
        // If not empty, check for pre-existing entry.
        if (!IsSerializedListEmpty(&_List))
        {
            pEntry = (Entry*) HeadOfSerializedList(&_List);
            for (i = 0; i < _List.ElementCount; i++)
            {
                if (!StrCmp(pEntry->_szName, szName))
                {
                    // Found identically named entry.
                    fUpdate = TRUE;
                    break;
                }
                pEntry = (Entry*) pEntry->Flink;
            } 
        }
        
        // If updating a current entry.
        if (fUpdate)
        {
            if (cbValue)
            {
                // Copy data over, freeing previous.
                if (FAILED(hr = CopyData(pEntry, pvValue, cbValue, dwFlags, TRUE)))
                    PAL_LEAVE;
            }
            else
            {
                // 0 byte count means remove entry.
                
                RemoveFromSerializedList(&_List, pEntry);
                delete pEntry;
                hr = S_OK;
            }
        }
        // otherwise allocate a new entry.
        else
        {
            if (cbValue) 
            {
                // Create new and push onto list.
                if (FAILED(hr = CreateEntry((LPOLESTR) szName, pvValue, 
                        cbValue, dwFlags, &pEntry)))
                    PAL_LEAVE;
                InsertAtHeadOfSerializedList(&_List, pEntry);
            }
            else
            {
                // Trying to create a new entry, but no byte count.
                hr = S_FALSE;
                PAL_LEAVE;
            }  
        }
    }
    PAL_FINALLY {
        // Release crit sect.
        UnlockSerializedList(&_List);
    }
    PAL_ENDTRY
        
exit:

    return hr;
}

// ---------------------------------------------------------------------------
// CApplicationContext::Get
// ---------------------------------------------------------------------------
STDMETHODIMP
CApplicationContext::Get(LPCOLESTR szName, LPVOID pvValue, 
    LPDWORD pcbValue, DWORD dwFlags)
{
    HRESULT hr = S_OK;
    Entry *pEntry = NULL;
    BOOL fFound = FALSE;
    LONG i;
    
    // Validate input.
    if (!szName || !pcbValue || (!pvValue && *pcbValue))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // perfperf - readers locking out readers.
    LockSerializedList(&_List);
    PAL_TRY {
    
        if (IsSerializedListEmpty(&_List))
        {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        }
    
        if( hr == S_OK )
        {
        
            pEntry = (Entry*) HeadOfSerializedList(&_List);
            for (i = 0; i < _List.ElementCount; i++)
            {
                if (!StrCmp(pEntry->_szName, szName))
                {
                    fFound = TRUE;
                    break;
                }
                pEntry = (Entry*) pEntry->Flink;
            }                        
        }
    
        // Entry found.
        if (fFound)
        {
            // Insufficient buffer case.
            if (*pcbValue < pEntry->_cbValue)
            {        
                *pcbValue = pEntry->_cbValue;
                hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
                PAL_LEAVE;
            }
            
            // If interface pointer addref and hand out.
            if (pEntry->_dwFlags & APP_CTX_FLAGS_INTERFACE)
            {
                *((IUnknown**) pvValue) = (IUnknown*) pEntry->_pbValue;
                ((IUnknown*) pEntry->_pbValue)->AddRef();
            }
            // Otherwise just copy blob.
            else    
                memcpy(pvValue, pEntry->_pbValue, pEntry->_cbValue);
    
            // Indicate byte count.
            *pcbValue = pEntry->_cbValue;
            hr = S_OK;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        }
    
    }
    PAL_FINALLY {
        UnlockSerializedList(&_List);
    }
    PAL_ENDTRY

exit:
    return hr;
}


// IUnknown methods

// ---------------------------------------------------------------------------
// CApplicationContext::AddRef
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CApplicationContext::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

// ---------------------------------------------------------------------------
// CApplicationContext::Release
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CApplicationContext::Release()
{
    if (InterlockedDecrement(&_cRef) == 0) {
        delete this;
        return 0;
    }

    return _cRef;
}

// ---------------------------------------------------------------------------
// CApplicationContext::QueryInterface
// ---------------------------------------------------------------------------
STDMETHODIMP
CApplicationContext::QueryInterface(REFIID riid, void** ppv)
{
    *ppv = NULL;

    if (riid==IID_IApplicationContext || riid == IID_IUnknown)
        *ppv = (IApplicationContext *) this;

    if (*ppv == NULL)
        return E_NOINTERFACE;

    AddRef();

    return S_OK;

} 

STDMETHODIMP CApplicationContext::GetDynamicDirectory(LPWSTR wzDynamicDir,
                                                      DWORD *pdwSize)
{
    HRESULT                                 hr = S_OK;
    LPWSTR                                  wzDynamicBase = NULL;
    LPWSTR                                  wzAppName = NULL;
    LPWSTR                                  wzAppCtxDynamicDir = NULL;
    WCHAR                                   wzDir[MAX_PATH];
    DWORD                                   dwLen;

    wzDir[0] = L'\0';

    if (!pdwSize) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    // Check if the dynamic directory has already been set.

    hr = ::AppCtxGetWrapper(this, ACTAG_APP_DYNAMIC_DIRECTORY, &wzAppCtxDynamicDir);
    if (FAILED(hr)) {
        goto Exit;
    }

    if (wzAppCtxDynamicDir) {
        ASSERT(lstrlenW(wzAppCtxDynamicDir));

        dwLen = lstrlenW(wzAppCtxDynamicDir) + 1;
        if (!wzDynamicDir || *pdwSize < dwLen) {
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            *pdwSize = dwLen;
            goto Exit;
        }

        *pdwSize = dwLen;

        lstrcpyW(wzDynamicDir, wzAppCtxDynamicDir);
        goto Exit;
    }

    // Dynamic directory not set. Calculate it.

    hr = ::AppCtxGetWrapper(this, ACTAG_APP_DYNAMIC_BASE, &wzDynamicBase);
    if (FAILED(hr)) {
        goto Exit;
    }

    hr = ::AppCtxGetWrapper(this, ACTAG_APP_NAME, &wzAppName);
    if (FAILED(hr)) {
        goto Exit;
    }

    if (!wzAppName || !wzDynamicBase) {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        goto Exit;
    }

    PathRemoveBackslashW(wzDynamicBase);

    wnsprintfW(wzDir, MAX_PATH, L"%ws\\%ws", wzDynamicBase, wzAppName);
    dwLen = lstrlenW(wzDir) + 1;


    if (!wzDynamicDir || *pdwSize < dwLen) {
        *pdwSize = dwLen;
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        goto Exit;
    }

    *pdwSize = dwLen;
    lstrcpyW(wzDynamicDir, wzDir);

    // Cache this for future use.

    Set(ACTAG_APP_DYNAMIC_DIRECTORY, wzDynamicDir, dwLen * sizeof(WCHAR), 0);

Exit:
    SAFEDELETEARRAY(wzDynamicBase);
    SAFEDELETEARRAY(wzAppName);
    SAFEDELETEARRAY(wzAppCtxDynamicDir);

    return hr;
}

STDMETHODIMP CApplicationContext::GetAppCacheDirectory(LPWSTR wzCacheDir,
                                                       DWORD *pdwSize)
{
    HRESULT                                 hr = S_OK;
    LPWSTR                                  wzCacheBase = NULL;
    LPWSTR                                  wzAppName = NULL;
    LPWSTR                                  wzAppCtxCacheDir = NULL;
    WCHAR                                   wzDir[MAX_PATH];
    DWORD                                   dwLen;

    wzDir[0] = L'\0';

    if (!pdwSize) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    // Check if the cache directory has already been set.

    hr = ::AppCtxGetWrapper(this, ACTAG_APP_CACHE_DIRECTORY, &wzAppCtxCacheDir);
    if (FAILED(hr)) {
        goto Exit;
    }

    if (wzAppCtxCacheDir) {
        ASSERT(lstrlenW(wzAppCtxCacheDir));

        dwLen = lstrlenW(wzAppCtxCacheDir) + 1;
        if (!wzCacheDir || *pdwSize < dwLen) {
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            *pdwSize = dwLen;
            goto Exit;
        }

        *pdwSize = dwLen;

        lstrcpyW(wzCacheDir, wzAppCtxCacheDir);
        goto Exit;
    }


    // Always recalculate cache directory, so it can be changed on-demand

    hr = ::AppCtxGetWrapper(this, ACTAG_APP_CACHE_BASE, &wzCacheBase);
    if (FAILED(hr)) {
        goto Exit;
    }

    hr = ::AppCtxGetWrapper(this, ACTAG_APP_NAME, &wzAppName);
    if (FAILED(hr)) {
        goto Exit;
    }

    if (!wzAppName || !wzCacheBase) {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        goto Exit;
    }

    PathRemoveBackslashW(wzCacheBase);

    wnsprintfW(wzDir, MAX_PATH, L"%ws\\%ws", wzCacheBase, wzAppName);
    dwLen = lstrlenW(wzDir) + 1;

    if (!wzCacheDir || *pdwSize < dwLen) {
        *pdwSize = dwLen;
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        goto Exit;
    }

    *pdwSize = dwLen;
    lstrcpyW(wzCacheDir, wzDir);

    // Cache this for future use.

    Set(ACTAG_APP_CACHE_DIRECTORY, wzCacheDir, dwLen * sizeof(WCHAR), 0);

Exit:
    SAFEDELETEARRAY(wzCacheBase);
    SAFEDELETEARRAY(wzAppName);
    SAFEDELETEARRAY(wzAppCtxCacheDir);

    return hr;
}

//
// RegisterKnownAssembly
//
// Params:
//
// [in]  pName      : IAssemblyName describing the known assembly
// [in]  pwzAsmURL  : Full URL to assembly described by pName
// [out] ppAsmOut   : Output IAssembly to be passed to BindToObject
//

STDMETHODIMP CApplicationContext::RegisterKnownAssembly(IAssemblyName *pName,
                                                        LPCWSTR pwzAsmURL,
                                                        IAssembly **ppAsmOut)
{
    HRESULT                               hr = S_OK;
    DWORD                                 dwSize = 0;
    CLoadContext                         *pLoadContext = NULL;
    CAssembly                            *pAsm = NULL;
    IAssembly                            *pAsmActivated = NULL;
    WCHAR                                 wzLocalPath[MAX_PATH];
    LPWSTR                                wzURLCanonicalized=NULL;
    CCriticalSection                      cs(&_cs);

    if (!pName || !pwzAsmURL || !ppAsmOut) {
        hr = E_INVALIDARG;
        return hr;
    }

    if (CAssemblyName::IsPartial(pName)) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    hr = cs.Lock();
    if (FAILED(hr)) {
        return hr;
    }
    
    dwSize = sizeof(pLoadContext);
    hr = Get(ACTAG_LOAD_CONTEXT_DEFAULT, &pLoadContext, &dwSize, APP_CTX_FLAGS_INTERFACE);
    if (FAILED(hr) || !pLoadContext) {
        hr = E_UNEXPECTED;
        goto Exit;
    }

    // See if we know about this assembly already

    hr = pLoadContext->CheckActivated(pName, ppAsmOut);
    if (hr == S_OK) {
        // Something with this name is already registered!

        hr = HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
        goto Exit;
    }

    // Create the activated assembly node, and load context.

    wzURLCanonicalized = NEW(WCHAR[MAX_URL_LENGTH+1]);
    if (!wzURLCanonicalized)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    dwSize = MAX_URL_LENGTH;
    hr = UrlCanonicalizeUnescape(pwzAsmURL, wzURLCanonicalized, &dwSize, 0);
    if (FAILED(hr)) {
        goto Exit;
    }

    // Create a bogus CAssembly just to hold the load context. Mark all
    // methods as disabled.

    pAsm = NEW(CAssembly);
    if (!pAsm) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    dwSize = MAX_PATH;
    hr = PathCreateFromUrlWrap(wzURLCanonicalized, wzLocalPath, &dwSize, 0);
    if (FAILED(hr)) {
        goto Exit;
    }

    hr = pAsm->InitDisabled(pName, wzLocalPath);
    if (FAILED(hr)) {
        goto Exit;
    }

    // Now add ourselves to the default load context

    hr = pLoadContext->AddActivation(pAsm, &pAsmActivated);
    if (FAILED(hr)) {
        goto Exit;
    }
    else if (hr == S_FALSE) {
        SAFERELEASE(pAsmActivated);
        SAFERELEASE(pAsm);
        hr = HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
        goto Exit;
    }

    // Hand out IAssembly

    (*ppAsmOut) = pAsm;
    (*ppAsmOut)->AddRef();

Exit:
    cs.Unlock();

    SAFEDELETEARRAY(wzURLCanonicalized);
    SAFERELEASE(pLoadContext);
    SAFERELEASE(pAsm);

    return hr;
}

//
// PrefetchAppConfigFile
//

STDMETHODIMP CApplicationContext::PrefetchAppConfigFile()
{
    _ASSERTE(false);
    return E_NOTIMPL;
}

STDMETHODIMP CApplicationContext::SxsActivateContext(ULONG_PTR *lpCookie)
{
    *lpCookie = NULL;
    return S_OK;
}

STDMETHODIMP CApplicationContext::SxsDeactivateContext(ULONG_PTR ulCookie)
{
    return S_OK;
}


HRESULT CApplicationContext::Lock()
{
    HRESULT                                      hr = S_OK;

    PAL_TRY {
        EnterCriticalSection(&_cs);
    }
    PAL_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        hr = E_OUTOFMEMORY;
    }
    PAL_ENDTRY

    return hr;
}

HRESULT CApplicationContext::Unlock()
{
    LeaveCriticalSection(&_cs);

    return S_OK;
}

