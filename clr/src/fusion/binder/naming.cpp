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
 //
//  Naming Services
//

#include <windows.h>
#include <winerror.h>
#include "naming.h"
#include "debmacro.h"
#include "asmcache.h"
#include "asm.h"
#include "asmimprt.h"
#include "fusionp.h"
#include "adl.h"
#include "cblist.h"
#include "helpers.h"
#include "appctx.h"
#include "actasm.h"
#include "parse.h"
#include "adlmgr.h"
#include "policy.h"
#include "dbglog.h"
#include "util.h"    
#include "pcycache.h"
#include "history.h"
#include "histinfo.h"
#include "cacheutils.h"
#include "lock.h"

#define VERSION_STRING_SEGMENTS                     4

PFNSTRONGNAMETOKENFROMPUBLICKEY      g_pfnStrongNameTokenFromPublicKey = NULL;
PFNSTRONGNAMEERRORINFO               g_pfnStrongNameErrorInfo = NULL;
PFNSTRONGNAMEFREEBUFFER              g_pfnStrongNameFreeBuffer = NULL;
PFNSTRONGNAMESIGNATUREVERIFICATION   g_pfnStrongNameSignatureVerification = NULL;

FusionTag(TagNaming, "Fusion", "Name Object");

extern WCHAR g_wszAdminCfg[];
extern CRITICAL_SECTION g_csInitClb;
extern WCHAR g_wzEXEPath[MAX_PATH+1];

#ifdef PLATFORM_UNIX
#define DEVPATH_DIR_DELIMITER                   L':'
#else
#define DEVPATH_DIR_DELIMITER                   L';'
#endif


// ---------------------------------------------------------------------------
// CPropertyArray ctor
// ---------------------------------------------------------------------------
CPropertyArray::CPropertyArray()
{
    _dwSig = 0x504f5250; /* 'PORP' */
    memset(&_rProp, 0, ASM_NAME_MAX_PARAMS * sizeof(Property));
}

// ---------------------------------------------------------------------------
// CPropertyArray dtor
// ---------------------------------------------------------------------------
CPropertyArray::~CPropertyArray()
{
    for (DWORD i = 0; i < ASM_NAME_MAX_PARAMS; i++)
    {
        if (_rProp[i].cb > sizeof(DWORD))
        {
            if (_rProp[i].pv != NULL)
            {
                FUSION_DELETE_ARRAY((LPBYTE) _rProp[i].pv);
                _rProp[i].pv = NULL;
            }
        }
    }
}


// ---------------------------------------------------------------------------
// CPropertyArray::Set
// ---------------------------------------------------------------------------
HRESULT CPropertyArray::Set(DWORD PropertyId, 
    LPVOID pvProperty, DWORD cbProperty)
{
    HRESULT hr = S_OK;
    Property *pItem = NULL;
        
    if (PropertyId >= ASM_NAME_MAX_PARAMS
        || (!pvProperty && cbProperty))
    {
        ASSERT(FALSE);
        hr = E_INVALIDARG;
        goto exit;
    }        

    pItem = &(_rProp[PropertyId]);

    if (!cbProperty && !pvProperty)
    {
        if (pItem->cb > sizeof(DWORD))
        {
            if (pItem->pv != NULL)
                FUSION_DELETE_ARRAY((LPBYTE) pItem->pv);
        }
        pItem->pv = NULL;
    }
    else if (cbProperty > sizeof(DWORD))
    {
        LPBYTE ptr = NEW(BYTE[cbProperty]);
        if (!ptr)
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

        if (pItem->cb > sizeof(DWORD))
            FUSION_DELETE_ARRAY((LPBYTE) pItem->pv);

        memcpy(ptr, pvProperty, cbProperty);        
        pItem->pv = ptr;
    }
    else
    {
        if (pItem->cb > sizeof(DWORD))
            FUSION_DELETE_ARRAY((LPBYTE) pItem->pv);

        memcpy(&(pItem->pv), pvProperty, cbProperty);
    }
    pItem->cb = cbProperty;

exit:
    return hr;
}     

// ---------------------------------------------------------------------------
// CPropertyArray::Get
// ---------------------------------------------------------------------------
HRESULT CPropertyArray::Get(DWORD PropertyId, 
    LPVOID pvProperty, LPDWORD pcbProperty)
{
    HRESULT hr = S_OK;
    Property *pItem;    

    ASSERT(pcbProperty);

    if (PropertyId >= ASM_NAME_MAX_PARAMS
        || (!pvProperty && *pcbProperty))
    {
        ASSERT(FALSE);
        hr = E_INVALIDARG;
        goto exit;
    }        

    pItem = &(_rProp[PropertyId]);

    if (pItem->cb > *pcbProperty)
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    else if (pItem->cb)
        memcpy(pvProperty, (pItem->cb > sizeof(DWORD) ? 
            pItem->pv : (LPBYTE) &(pItem->pv)), pItem->cb);

    *pcbProperty = pItem->cb;
        
exit:
    return hr;
}     

// ---------------------------------------------------------------------------
// CPropertyArray::operator []
// Wraps DWORD optimization test.
// ---------------------------------------------------------------------------
Property CPropertyArray::operator [] (DWORD PropertyId)
{
    Property prop;

    prop.pv = _rProp[PropertyId].cb > sizeof(DWORD) ?
        _rProp[PropertyId].pv : &(_rProp[PropertyId].pv);

    prop.cb = _rProp[PropertyId].cb;

    return prop;
}

// Creation funcs.


// ---------------------------------------------------------------------------
// CreateAssemblyNameObject
// ---------------------------------------------------------------------------
STDAPI
CreateAssemblyNameObject(
    LPASSEMBLYNAME    *ppAssemblyName,
    LPCOLESTR          szAssemblyName,
    DWORD              dwFlags,
    LPVOID             pvReserved)
{

    HRESULT hr = S_OK;
    CAssemblyName *pName = NULL;

    if (!ppAssemblyName)
    {
        hr = E_INVALIDARG;
        goto exit;
    }
    
    pName = NEW(CAssemblyName);
    if (!pName)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    if (dwFlags & CANOF_PARSE_DISPLAY_NAME)
    {
        hr = pName->Init(NULL, NULL);
        if (FAILED(hr)) {
            goto exit;
        }

        hr = pName->Parse((LPWSTR)szAssemblyName);
    }
    else
    {
        hr = pName->Init(szAssemblyName, NULL);

        if (dwFlags & CANOF_SET_DEFAULT_VALUES)
        {
            hr = pName->SetDefaults();
        }
    }

    if (FAILED(hr)) 
    {
        SAFERELEASE(pName);
        goto exit;
    }

    *ppAssemblyName = pName;

exit:

    return hr;
}

// ---------------------------------------------------------------------------
// CreateAssemblyNameObjectFromMetaData
// ---------------------------------------------------------------------------

STDAPI
CreateAssemblyNameObjectFromMetaData(
    LPASSEMBLYNAME    *ppAssemblyName,
    LPCOLESTR          szAssemblyName,
    ASSEMBLYMETADATA  *pamd,
    LPVOID             pvReserved)
{

    HRESULT hr = S_OK;
    CAssemblyName *pName = NULL;

    pName = NEW(CAssemblyName);
    if (!pName)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    hr = pName->Init(szAssemblyName, pamd);
        
    if (FAILED(hr)) 
    {
        SAFERELEASE(pName);
        goto exit;
    }

    *ppAssemblyName = pName;

exit:

    return hr;
}

// IUnknown methods

// ---------------------------------------------------------------------------
// CAssemblyName::AddRef
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CAssemblyName::AddRef()
{
    return InterlockedIncrement((LONG*) &_cRef);
}

// ---------------------------------------------------------------------------
// CAssemblyName::Release
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CAssemblyName::Release()
{
    if (InterlockedDecrement((LONG*) &_cRef) == 0) {
        delete this;
        return 0;
    }

    return _cRef;
}

// ---------------------------------------------------------------------------
// CAssemblyName::QueryInterface
// ---------------------------------------------------------------------------
STDMETHODIMP
CAssemblyName::QueryInterface(REFIID riid, void** ppv)
{
    if (   IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, IID_IAssemblyName)
       )
    {
        *ppv = static_cast<IAssemblyName*> (this);
        AddRef();
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}

// ---------------------------------------------------------------------------
// CAssemblyName::SetProperty
// ---------------------------------------------------------------------------
STDMETHODIMP
CAssemblyName::SetProperty(DWORD PropertyId, 
    LPVOID pvProperty, DWORD cbProperty)
{
    LPBYTE pbSN = NULL;
    DWORD  cbSN = 0;
    HRESULT hr = S_OK;
    CCriticalSection cs(&_cs);

    // Fail if finalized.
    if (_fIsFinalized)
    {
        hr = E_UNEXPECTED;
        ASSERT(FALSE);
        return hr;
    }

    hr = cs.Lock();
    if (FAILED(hr)) {
        return hr;
    }

    // Check if public key is being set and if so,
    // set the public key token if not already set.
    if (PropertyId == ASM_NAME_PUBLIC_KEY)
    {
        // If setting true public key, generate hash.
        if (pvProperty && cbProperty)
        {
            // Generate the public key token from the pk.
            if (FAILED(hr = GetPublicKeyTokenFromPKBlob((LPBYTE) pvProperty, cbProperty, &pbSN, &cbSN)))
                goto exit;

            // Set the public key token property.
            if (FAILED(hr = SetProperty(ASM_NAME_PUBLIC_KEY_TOKEN, pbSN, cbSN)))
                goto exit;        
        }
        // Otherwise expect call to reset property.
        else if (!cbProperty)
        {
            if (FAILED(hr = SetProperty(ASM_NAME_PUBLIC_KEY_TOKEN, pvProperty, cbProperty)))
                goto exit;
        }
            
    }
    // Setting NULL public key clears values in public key,
    // public key token and sets public key token flag.
    else if (PropertyId == ASM_NAME_NULL_PUBLIC_KEY)
    {
        pvProperty = NULL;
        cbProperty = 0;
        hr = SetProperty(ASM_NAME_NULL_PUBLIC_KEY_TOKEN, pvProperty, cbProperty);
        goto exit;
    }
    // Setting or clearing public key token.
    else if (PropertyId == ASM_NAME_PUBLIC_KEY_TOKEN)
    {
        if (pvProperty && cbProperty)
            _fPublicKeyToken = TRUE;
        else if (!cbProperty)
            _fPublicKeyToken = FALSE;
    }
    // Setting NULL public key token clears public key token and
    // sets public key token flag.
    else if (PropertyId == ASM_NAME_NULL_PUBLIC_KEY_TOKEN)
    {
        _fPublicKeyToken = TRUE;
        pvProperty = NULL;
        cbProperty = 0;
        PropertyId = ASM_NAME_PUBLIC_KEY_TOKEN;
    }
    else if (PropertyId == ASM_NAME_CUSTOM)
    {
        if (pvProperty && cbProperty)
            _fCustom = TRUE;
        else if (!cbProperty)
            _fCustom = FALSE;
    }
    else if (PropertyId == ASM_NAME_NULL_CUSTOM)
    {
        _fCustom = TRUE;
        pvProperty = NULL;
        cbProperty = 0;
        PropertyId = ASM_NAME_CUSTOM;
    }

    // Setting "neutral" as the culture is the same as "" culture (meaning
    // culture-invariant).
    else if (PropertyId == ASM_NAME_CULTURE) {
        if (pvProperty && !lstrcmpiW((LPWSTR)pvProperty, L"neutral")) {
            pvProperty = (void *)L"";
            cbProperty = sizeof(L"");
        }
    }

    // Set property on array.
    hr = _rProp.Set(PropertyId, pvProperty, cbProperty);

exit:
    // Free memory allocated by crypto wrapper.
    if (pbSN) {
        g_pfnStrongNameFreeBuffer(pbSN);
    }

    cs.Unlock();

    return hr;
}


// ---------------------------------------------------------------------------
// CAssemblyName::GetProperty
// ---------------------------------------------------------------------------
STDMETHODIMP
CAssemblyName::GetProperty(DWORD PropertyId, 
    LPVOID pvProperty, LPDWORD pcbProperty)
{
    HRESULT hr;
    CCriticalSection cs(&_cs);

    hr = cs.Lock();
    if (FAILED(hr)) {
        return hr;
    }

    // Retrieve the property.
    switch(PropertyId)
    {
        case ASM_NAME_NULL_PUBLIC_KEY_TOKEN:
        case ASM_NAME_NULL_PUBLIC_KEY:
        {
            hr = (_fPublicKeyToken && !_rProp[PropertyId].cb) ? S_OK : S_FALSE;
            break;
        }
        case ASM_NAME_NULL_CUSTOM:
        {
            hr = (_fCustom && !_rProp[PropertyId].cb) ? S_OK : S_FALSE;
            break;
        }
        default:        
        {
            hr = _rProp.Get(PropertyId, pvProperty, pcbProperty);
            break;
        }
    }

    cs.Unlock();
    return hr;
}

// ---------------------------------------------------------------------------
// CAssemblyName::GetName
// ---------------------------------------------------------------------------
STDMETHODIMP
CAssemblyName::GetName(
        /* [out][in] */ LPDWORD lpcwBuffer,
        /* [out] */     LPOLESTR pwzBuffer)
{
    DWORD cbBuffer = *lpcwBuffer * sizeof(TCHAR);
    HRESULT hr = GetProperty(ASM_NAME_NAME, pwzBuffer, &cbBuffer);
    *lpcwBuffer = cbBuffer / sizeof(TCHAR);
    return hr;
}


// ---------------------------------------------------------------------------
// CAssemblyName::GetVersion
// ---------------------------------------------------------------------------
STDMETHODIMP
CAssemblyName::GetVersion(
        /* [out] */ LPDWORD pdwVersionHi,
        /* [out] */ LPDWORD pdwVersionLow)
{
    DWORD cb = sizeof(WORD);
    WORD wVerMajor = 0, wVerMinor = 0, wRevNo = 0, wBldNo = 0;
    
    ASSERT(pdwVersionHi);
    ASSERT(pdwVersionLow);

    GetProperty(ASM_NAME_MAJOR_VERSION,   &wVerMajor, &(cb = sizeof(WORD)));
    GetProperty(ASM_NAME_MINOR_VERSION,   &wVerMinor, &(cb = sizeof(WORD)));
    GetProperty(ASM_NAME_REVISION_NUMBER, &wRevNo,    &(cb = sizeof(WORD)));
    GetProperty(ASM_NAME_BUILD_NUMBER,    &wBldNo,    &(cb = sizeof(WORD)));

    *pdwVersionHi  = MAKELONG(wVerMinor, wVerMajor);
    *pdwVersionLow = MAKELONG(wRevNo, wBldNo);

    return S_OK;
}


// ---------------------------------------------------------------------------
// CAssemblyName::IsEqual
// ---------------------------------------------------------------------------
STDMETHODIMP
CAssemblyName::IsEqual(LPASSEMBLYNAME pName, DWORD dwCmpFlags)
{
    return IsEqualLogging(pName, dwCmpFlags, NULL);
}

STDMETHODIMP
CAssemblyName::IsEqualLogging(LPASSEMBLYNAME pName, DWORD dwCmpFlags, CDebugLog *pdbglog)
{
    HRESULT hr = S_OK;
    TCHAR szName[MAX_CLASS_NAME], *pszName;
    WCHAR szCulture[MAX_PATH];
    DWORD cbName = MAX_CLASS_NAME, cbWord = sizeof(WORD);
    WORD wMajor = 0, wMinor = 0, wRev = 0, wBuild = 0;
    BYTE bPublicKeyToken[MAX_PUBLIC_KEY_TOKEN_LEN];
    DWORD dwPublicKeyToken = MAX_PUBLIC_KEY_TOKEN_LEN;
    DWORD cbCulture = MAX_PATH;
    LPBYTE pbPublicKeyToken, pbCustom = NULL;
    DWORD dwPartialCmpMask = 0;
    BOOL  fIsPartial = FALSE;
    
    const DWORD SIMPLE_VERSION_MASK =
        ASM_CMPF_MAJOR_VERSION
            | ASM_CMPF_MINOR_VERSION
            | ASM_CMPF_REVISION_NUMBER
            | ASM_CMPF_BUILD_NUMBER;

    Property propName       = _rProp[ASM_NAME_NAME];
    Property propPublicKeyToken = _rProp[ASM_NAME_PUBLIC_KEY_TOKEN];
    Property propMajor      = _rProp[ASM_NAME_MAJOR_VERSION];
    Property propMinor      = _rProp[ASM_NAME_MINOR_VERSION];
    Property propRev        = _rProp[ASM_NAME_REVISION_NUMBER];
    Property propBuild      = _rProp[ASM_NAME_BUILD_NUMBER];
    Property propCulture    = _rProp[ASM_NAME_CULTURE];
    Property propCustom     = _rProp[ASM_NAME_CUSTOM];

    // Get the ref partial comparison mask, if any.    
    fIsPartial = CAssemblyName::IsPartial(this, &dwPartialCmpMask);

    // If default semantics are requested.
    if (dwCmpFlags == ASM_CMPF_DEFAULT) 
    {
        // Set all comparison flags.
        dwCmpFlags = ASM_CMPF_ALL;

        // Otherwise, if ref is simple (possibly partial)
        // we mask off all version bits.
        if (!CCache::IsStronglyNamed(this)) {

            if (dwPartialCmpMask & ASM_CMPF_PUBLIC_KEY_TOKEN)
            {
                dwCmpFlags &= ~SIMPLE_VERSION_MASK;
            }
            // If neither of these two cases then public key token
            // is not set in ref , but def may be simple or strong.
            // The comparison mask is chosen based on def.
            else
            {
                if (!CCache::IsStronglyNamed(pName))
                    dwCmpFlags &= ~SIMPLE_VERSION_MASK;            
            }
        }
    }   

    // Mask off flags (either passed in or generated
    // by default flag with the comparison mask generated 
    // from the ref.
    if (fIsPartial)
        dwCmpFlags &= dwPartialCmpMask;

    
    // The individual name fields can now be compared..

    // Compare name

    if (dwCmpFlags & ASM_CMPF_NAME) {
        pszName = (LPTSTR) propName.pv;
            
        hr = pName->GetName(&cbName, szName);
    
        if (FAILED(hr)) {
            goto Exit;
        }

        cbName *= sizeof(TCHAR);

        if (cbName != propName.cb) {
            DEBUGOUT(pdbglog, 0, ID_FUSLOG_ISEQUAL_DIFF_NAME);
            hr = S_FALSE;
            goto Exit;
        }
    
        if (cbName && StrCmpI(pszName, szName)) {
            DEBUGOUT(pdbglog, 0, ID_FUSLOG_ISEQUAL_DIFF_NAME);
            hr = S_FALSE;
            goto Exit;
        }
    }

    // Compare version

    if (dwCmpFlags & ASM_CMPF_MAJOR_VERSION) {
        if (FAILED(hr = pName->GetProperty(ASM_NAME_MAJOR_VERSION, &wMajor, &cbWord)))
        {
            goto Exit;
        }
    
        if (*((LPWORD) propMajor.pv) != wMajor)
        {
            DEBUGOUT(pdbglog, 0, ID_FUSLOG_ISEQUAL_DIFF_VERSION_MAJOR);
            hr = S_FALSE;
            goto Exit;
        }
    }

    if (dwCmpFlags & ASM_CMPF_MINOR_VERSION) {
        if (FAILED(hr = pName->GetProperty(ASM_NAME_MINOR_VERSION, &wMinor, &cbWord)))
        {
            goto Exit;
        }
        if (*((LPWORD) propMinor.pv) != wMinor)
        {
            DEBUGOUT(pdbglog, 0, ID_FUSLOG_ISEQUAL_DIFF_VERSION_MINOR);
            hr = S_FALSE;
            goto Exit;
        }
    }

    if (dwCmpFlags & ASM_CMPF_REVISION_NUMBER) {
        if (FAILED(hr = pName->GetProperty(ASM_NAME_REVISION_NUMBER, &wRev, &cbWord)))
        {
            goto Exit;
        }
            
        if (*((LPWORD) propRev.pv) != wRev)
        {
            DEBUGOUT(pdbglog, 0, ID_FUSLOG_ISEQUAL_DIFF_VERSION_REVISION);
            hr = S_FALSE;
            goto Exit;
        }
    }

    if (dwCmpFlags & ASM_CMPF_BUILD_NUMBER) {
        if (FAILED(hr = pName->GetProperty(ASM_NAME_BUILD_NUMBER, &wBuild, &cbWord)))
        {
            goto Exit;
        }

        if (*((LPWORD) propBuild.pv) != wBuild)
        {
            DEBUGOUT(pdbglog, 0, ID_FUSLOG_ISEQUAL_DIFF_VERSION_BUILD);
            hr = S_FALSE;
            goto Exit;
        }
    }


    // Compare public key token

    if (dwCmpFlags & ASM_CMPF_PUBLIC_KEY_TOKEN) {
        pbPublicKeyToken = (LPBYTE) propPublicKeyToken.pv;

    
        hr = pName->GetProperty(ASM_NAME_PUBLIC_KEY_TOKEN, (void *)bPublicKeyToken,
                                &dwPublicKeyToken);
        if (FAILED(hr)) {
            goto Exit;
        }
    
        if (dwPublicKeyToken != propPublicKeyToken.cb) {
            DEBUGOUT(pdbglog, 0, ID_FUSLOG_ISEQUAL_DIFF_PUBLIC_KEY_TOKEN);
            hr = S_FALSE;
            goto Exit; 
        }

        if (memcmp((void *)bPublicKeyToken, pbPublicKeyToken, dwPublicKeyToken)) {
            DEBUGOUT(pdbglog, 0, ID_FUSLOG_ISEQUAL_DIFF_PUBLIC_KEY_TOKEN);
            hr = S_FALSE;
            goto Exit;
        }
    }

    // Compare Culture
    
    if (dwCmpFlags & ASM_CMPF_CULTURE)
    {
        LPWSTR szCultureThis = NULL;
        DWORD cbCultureThis = 0;
        
        if (FAILED(hr = pName->GetProperty(ASM_NAME_CULTURE, szCulture, &cbCulture)))
            goto Exit;
            
        cbCultureThis = propCulture.cb;
        szCultureThis = (WCHAR*) (propCulture.pv);

        if (!(cbCultureThis && cbCulture)
            || (cbCultureThis != cbCulture)
            || (StrCmpIW(szCultureThis, szCulture)))
        {
            DEBUGOUT(pdbglog, 0, ID_FUSLOG_ISEQUAL_DIFF_CULTURE);
            hr = S_FALSE;
            goto Exit;
        }
        
    }

    // Compare Custom attribute.

    if (dwCmpFlags & ASM_CMPF_CUSTOM) 
    {
        LPBYTE bCustom; bCustom = NULL;
        DWORD cbCustom; cbCustom = 0;
        
        pbCustom = (LPBYTE) propCustom.pv;

        hr = pName->GetProperty(ASM_NAME_CUSTOM, (void *)bCustom, &cbCustom);

        if (hr != S_OK && hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
            goto Exit;
    
        if (cbCustom)
        {
            bCustom = NEW(BYTE[cbCustom]);
            if (!bCustom)
            {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }

            if (FAILED(hr = pName->GetProperty(ASM_NAME_CUSTOM, (void *)bCustom, &cbCustom))) {
                SAFEDELETEARRAY(bCustom);
                goto Exit;
            }
        }
                
        PAL_TRY {
            if (cbCustom != propCustom.cb) {
                DEBUGOUT(pdbglog, 0, ID_FUSLOG_ISEQUAL_DIFF_CUSTOM);
                hr = S_FALSE;
                PAL_LEAVE; 
            }
    
            if (memcmp((void *)bCustom, pbCustom, cbCustom)) {
                DEBUGOUT(pdbglog, 0, ID_FUSLOG_ISEQUAL_DIFF_CUSTOM);
                hr = S_FALSE;
                PAL_LEAVE;
            }

            hr = S_OK;
        }
        PAL_FINALLY {
            SAFEDELETEARRAY(bCustom);
        }
        PAL_ENDTRY

        if (hr != S_OK)
            goto Exit;
    }


Exit:
    return hr;
}

// ---------------------------------------------------------------------------
// CAssemblyName constructor
// ---------------------------------------------------------------------------
CAssemblyName::CAssemblyName()
{
    _dwSig              = 0x454d414e; /* 'EMAN' */
    _fIsFinalized       = FALSE;
    _fPublicKeyToken    = FALSE;
    _fCustom            = TRUE;
    _cRef               = 0;
    _fCSInitialized     = FALSE;
}

// ---------------------------------------------------------------------------
// CAssemblyName destructor
// ---------------------------------------------------------------------------
CAssemblyName::~CAssemblyName()
{
    if (_fCSInitialized) {
        DeleteCriticalSection(&_cs);
    }
}

// ---------------------------------------------------------------------------
// CAssemblyName::Init
// ---------------------------------------------------------------------------

HRESULT
CAssemblyName::Init(LPCTSTR pszAssemblyName, ASSEMBLYMETADATA *pamd)
{
    HRESULT hr = S_OK;

    PAL_TRY {
        InitializeCriticalSection(&_cs);
        _fCSInitialized = TRUE;
    }
    PAL_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        hr = E_OUTOFMEMORY;
    }
    PAL_ENDTRY

    if (FAILED(hr)) {
        goto exit;
    }

    // Name
    if (pszAssemblyName) 
    {
        hr = SetProperty(ASM_NAME_NAME, (LPTSTR) pszAssemblyName, 
            (lstrlen(pszAssemblyName)+1) * sizeof(TCHAR));

        if (FAILED(hr))
            goto exit;
    }
           
    if (pamd) {
            // Major version
        if (FAILED(hr = SetProperty(ASM_NAME_MAJOR_VERSION,
                &pamd->usMajorVersion, sizeof(WORD)))
    
            // Minor version
            || FAILED(hr = SetProperty(ASM_NAME_MINOR_VERSION, 
                &pamd->usMinorVersion, sizeof(WORD)))
    
            // Revision number
            || FAILED(hr = SetProperty(ASM_NAME_REVISION_NUMBER, 
                &pamd->usRevisionNumber, sizeof(WORD)))
    
            // Build number
            || FAILED(hr = SetProperty(ASM_NAME_BUILD_NUMBER, 
                &pamd->usBuildNumber, sizeof(WORD)))
    
            // Culture
            || FAILED(hr = SetProperty(ASM_NAME_CULTURE,
                pamd->szLocale, pamd->cbLocale * sizeof(WCHAR)))
    

                )
            {
                goto exit;
            }
        
    }

exit:
    _cRef = 1;
    return hr;
}

HRESULT CAssemblyName::Clone(IAssemblyName **ppName)
{
    HRESULT         hr = S_OK;
    CAssemblyName*  pClone = NULL;
    DWORD           i = 0;
    LPVOID          pv = NULL;
    DWORD           dwSize = 0;

    if (!ppName) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    pClone = NEW(CAssemblyName);
    if( !pClone ) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    hr = pClone->Init(NULL, NULL);
    if (FAILED(hr)) {
        goto Exit;
    }

    for( i = 0; i < ASM_NAME_MAX_PARAMS; i ++)
    {
        // get size
        if( GetProperty(i, NULL, &dwSize) == 
                HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
        {
            // Allocate space 
            pv = NEW(BYTE[dwSize]);
            if (!pv )
                goto fail;

            // Retrieve 
            if (FAILED(GetProperty(i, pv, &dwSize)))
                goto fail;         

            // Set
            if (FAILED(pClone->SetProperty(i, pv, dwSize)))
                goto fail;
        }

        // reinit for next property
        FUSION_DELETE_ARRAY((LPBYTE) pv);
        pv = NULL;
        dwSize = 0;
    }
    
    pClone->_fPublicKeyToken = _fPublicKeyToken;
    pClone->_fCustom = _fCustom;
    
    // done
    goto Exit; 

fail:
    // if we failed for whatever reason
    FUSION_DELETE_ARRAY((LPBYTE) pv);

    if( pClone )
    {
        delete pClone;
        pClone = NULL;
    }

Exit:
    if (ppName) {
        *ppName = pClone;
        if (*ppName) {
            (*ppName)->AddRef();
        }
    }

    SAFERELEASE(pClone);

    return hr;
}

// ---------------------------------------------------------------------------
// CAssemblyName::BindToObject
// ---------------------------------------------------------------------------
STDMETHODIMP
CAssemblyName::BindToObject(
        /* in      */  REFIID               refIID,
        /* in      */  IUnknown            *pUnkBindSink,
        /* in      */  IUnknown            *pUnkAppCtx,
        /* in      */  LPCOLESTR            szCodebaseIn,
        /* in      */  LONGLONG             llFlags,
        /* in      */  LPVOID               pvReserved,
        /* in      */  DWORD                cbReserved,
        /*     out */  VOID               **ppv)

{
    HRESULT                                    hr = S_OK;
    LPWSTR                                     szCodebaseDupe = NULL;
    LPWSTR                                     szCodebase = NULL;
    ICodebaseList                             *pCodebaseList = NULL;
    DWORD                                      dwSize = 0;
    CDebugLog                                 *pdbglog = NULL;
    IAssemblyBindSink                         *pAsmBindSink = NULL;
    IApplicationContext                       *pAppCtx = NULL;
    CAsmDownloadMgr                           *pDLMgr = NULL;
    CAssemblyDownload                         *padl = NULL;
    CApplicationContext                       *pCAppCtx = NULL;
    LPWSTR                                     pwzAsmName;


    if (!ppv || !pUnkAppCtx) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    if (szCodebaseIn) {
        szCodebaseDupe = WSTRDupDynamic(szCodebaseIn);
        if (!szCodebaseDupe) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
    }

    hr = pUnkAppCtx->QueryInterface(IID_IApplicationContext, (void**)&pAppCtx);
    if (FAILED(hr)) {
        goto Exit;
    }

    pCAppCtx = static_cast<CApplicationContext *>(pAppCtx); // dynamic_cast
    if (!pCAppCtx) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    hr = pUnkBindSink->QueryInterface(IID_IAssemblyBindSink, (void **)&pAsmBindSink);
    if (FAILED(hr)) {
        goto Exit;
    }

    pwzAsmName = (LPWSTR)(_rProp[ASM_NAME_NAME].pv);
    if (pwzAsmName) {
        LPWSTR                    pwzPart = NULL;

        pwzPart = NEW(WCHAR[MAX_URL_LENGTH]);
        if (!pwzPart) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        if (lstrlenW(pwzAsmName) >= MAX_PATH) {
            // Name is too long.

            SAFEDELETEARRAY(pwzPart);
            hr = FUSION_E_INVALID_NAME;
            goto Exit;
        }

        dwSize = MAX_URL_LENGTH;
        hr = UrlGetPartW(pwzAsmName, pwzPart, &dwSize, URL_PART_SCHEME, 0);
        if (SUCCEEDED(hr) && lstrlenW(pwzPart)) {
            // The assembly name looks like a protocol (ie. it starts with
            // the form protocol:// ). Abort binds in this case.

            hr = FUSION_E_INVALID_NAME;
            SAFEDELETEARRAY(pwzPart);
            goto Exit;
        }

        SAFEDELETEARRAY(pwzPart);
    }

    if (szCodebaseDupe) {
        szCodebase = StripFilePrefix((LPWSTR)szCodebaseDupe);
    }

    {
        IAssemblyName      *pNameCalling = NULL;
        IAssembly          *pAsmCalling = NULL;
        LPWSTR              pwzCallingAsm = NULL;
        DWORD               dw;

        pwzCallingAsm = NEW(WCHAR[MAX_URL_LENGTH]);
        if (!pwzCallingAsm) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        lstrcpyW(pwzCallingAsm, L"(Unknown)");
    
        if (pvReserved && (llFlags & ASM_BINDF_PARENT_ASM_HINT)) {
            pAsmCalling = (IAssembly *)pvReserved;

            if (pAsmCalling->GetAssemblyNameDef(&pNameCalling) == S_OK) {
                dw = MAX_URL_LENGTH;
                pNameCalling->GetDisplayName(pwzCallingAsm, &dw, 0);
            }

            SAFERELEASE(pNameCalling);
        }

        CreateLogObject(&pdbglog, szCodebase, pAppCtx);
        DescribeBindInfo(pdbglog, pAppCtx, szCodebase, pwzCallingAsm);

        SAFEDELETEARRAY(pwzCallingAsm);
    }

    *ppv = NULL;

    // Handle dev-path special case

    if (!CCache::IsCustom(this)) {
        CAssembly            *pCAsmParent = NULL;

        if (pvReserved && (llFlags & ASM_BINDF_PARENT_ASM_HINT)) {
            IAssembly *pAsmParent = static_cast<IAssembly *>(pvReserved);
            pCAsmParent = static_cast<CAssembly *>(pAsmParent); // dynamic_cast

            ASSERT(pCAsmParent);
        }

        hr = ProcessDevPath(pAppCtx, ppv, pCAsmParent, pdbglog);
        if (hr == S_OK) {
            // Found match in dev path. Succeed immediately.
            ASSERT(ppv);
            goto Exit;
        }
    }
    else {
        DEBUGOUT(pdbglog, 1, ID_FUSLOG_DEVPATH_NO_PREJIT);
    }

    // Setup policy cache in appctx

    hr = PreparePolicyCache(pAppCtx, NULL);
    if (FAILED(hr)) {
        DEBUGOUT(pdbglog, 1, ID_FUSLOG_POLICY_CACHE_FAILURE);
    }


    // Create download objects for the real assembly download
    hr = CAsmDownloadMgr::Create(&pDLMgr, this, pAppCtx, pCodebaseList,
                                 (szCodebase) ? (szCodebase) : (NULL),
                                 pdbglog, pvReserved, llFlags);
    if (FAILED(hr)) {
        goto Exit;
    }
    
    hr = CAssemblyDownload::Create(&padl, pDLMgr, pDLMgr, pdbglog, llFlags);
    if (FAILED(hr)) {
        goto Exit;
    }

    // Download app.cfg if we don't already have it

    hr = pCAppCtx->Lock();
    if (FAILED(hr)) {
        goto Exit;
    }

    dwSize = 0;
    hr = pAppCtx->Get(ACTAG_APP_CFG_DOWNLOAD_ATTEMPTED, NULL, &dwSize, 0);
    if (hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND)) {
            hr = CCache::IsCustom(this) ? S_FALSE : 
                DownloadAppCfg(pAppCtx, padl, pAsmBindSink, pdbglog);
    }
    else {
        hr = S_OK;
    }

    pCAppCtx->Unlock();

    // If hr==S_OK, then we either had an app.cfg already, or
    // it was on the local hard disk.
    //
    // If hr==S_FALSE, then no app.cfg exists, continue regular download
    //
    // If hr==E_PENDING, then went async.

    if (SUCCEEDED(hr)) {
        hr = padl->PreDownload(FALSE, ppv);

        if (hr == S_OK) {
            hr = padl->AddClient(pAsmBindSink, TRUE);
            if (FAILED(hr)) {
                ASSERT(0);
                SAFERELEASE(pDLMgr);
                SAFERELEASE(padl);
                goto Exit;
            }

            hr = padl->KickOffDownload(TRUE);
        }
        else if (hr == S_FALSE) {
            // Completed synchronously
            hr = S_OK;
        }

    }

Exit:
    if (hr != E_PENDING && (g_dwForceLog || (FAILED(hr) && pDLMgr && pDLMgr->LogResult()))) {
        DUMPDEBUGLOG(pdbglog, g_dwLogLevel, hr);
    }

    SAFERELEASE(pDLMgr);
    SAFERELEASE(padl);
    SAFERELEASE(pdbglog);

    SAFERELEASE(pAsmBindSink);
    SAFERELEASE(pAppCtx);

    SAFEDELETEARRAY(szCodebaseDupe);

    return hr;
}

HRESULT CAssemblyName::CreateLogObject(CDebugLog **ppdbglog, LPCWSTR szCodebase,
                                       IApplicationContext *pAppCtx)
{
    HRESULT                                    hr = S_OK;
    LPWSTR                                     pwzAsmName = NULL;
    LPWSTR                                     wzBuf=NULL;
    DWORD                                      dwSize;
    
    dwSize = 0;
    hr = GetDisplayName(NULL, &dwSize, 0);
    if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
        pwzAsmName = NEW(WCHAR[dwSize]);
        if (!pwzAsmName) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
    
        hr = GetDisplayName(pwzAsmName, &dwSize, 0);
        if (FAILED(hr)) {
            goto Exit;
        }
    
    }
    else if (szCodebase) {
        // Can't get display name. Could be a where-ref bind. Use the URL.
        wzBuf = NEW(WCHAR[MAX_URL_LENGTH+1]);
        if (!wzBuf)
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        wzBuf[0] = L'\0';
        dwSize = MAX_URL_LENGTH;
        hr = UrlCanonicalizeUnescape(szCodebase, wzBuf, &dwSize, 0);
        if (SUCCEEDED(hr)) {
            pwzAsmName = NEW(WCHAR[dwSize + 1]);
            if (!pwzAsmName) {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }
            
            lstrcpyW(pwzAsmName, wzBuf);
            hr = S_OK;
        }
    }

    if (SUCCEEDED(hr)) {
        hr = CDebugLog::Create(pAppCtx, pwzAsmName, ppdbglog);
        if (FAILED(hr)) {
            goto Exit;
        }
    }

Exit:
    SAFEDELETEARRAY(pwzAsmName);
    SAFEDELETEARRAY(wzBuf);
    return hr;
}

HRESULT CAssemblyName::DescribeBindInfo(CDebugLog *pdbglog,
                                        IApplicationContext *pAppCtx,
                                        LPCWSTR wzCodebase,
                                        LPCWSTR pwzCallingAsm)
{
    HRESULT                                     hr = S_OK;
    DWORD                                       dwSize;
    LPWSTR                                      wzName = NULL;
    LPWSTR                                      wzAppName = NULL;
    LPWSTR                                      wzAppBase = NULL;
    LPWSTR                                      wzPrivatePath = NULL;
    LPWSTR                                      wzCacheBase = NULL;
    LPWSTR                                      wzDynamicBase = NULL;
    LPWSTR                                      wzDevPath = NULL;
    Property                                    propName = _rProp[ASM_NAME_NAME];

    if (!pdbglog || !pAppCtx) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    DEBUGOUT(pdbglog, 0, ID_FUSLOG_PREBIND_INFO_START);

    if (propName.cb) {
        // This is not a where-ref bind.
        dwSize = 0;
        GetDisplayName(NULL, &dwSize, 0);

        wzName = NEW(WCHAR[dwSize + 1]);
        if (!wzName) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        hr = GetDisplayName(wzName, &dwSize, 0);
        if (FAILED(hr)) {
            goto Exit;
        }

        DEBUGOUT2(pdbglog, 0, ID_FUSLOG_PREBIND_INFO_DISPLAY_NAME, wzName, (IsPartial(this, 0)) ? (L"Partial") : (L"Fully-specified"));
    }
    else {
        ASSERT(wzCodebase);

        DEBUGOUT1(pdbglog, 0, ID_FUSLOG_PREBIND_INFO_WHERE_REF, wzCodebase);
    }

    // appbase

    hr = ::AppCtxGetWrapper(pAppCtx, ACTAG_APP_BASE_URL, &wzAppBase);
    if (FAILED(hr)) {
        goto Exit;
    }

    if (!wzAppBase) {
        ASSERT(0);
        hr = E_UNEXPECTED;
        goto Exit;
    }

    DEBUGOUT1(pdbglog, 0, ID_FUSLOG_PREBIND_INFO_APPBASE, wzAppBase);

    // devpath

    hr = ::AppCtxGetWrapper(pAppCtx, ACTAG_DEV_PATH, &wzDevPath);
    if (FAILED(hr)) {
        goto Exit;
    }

    if (wzDevPath) {
        DEBUGOUT1(pdbglog, 0, ID_FUSLOG_PREBIND_INFO_DEVPATH, (wzDevPath) ? (wzDevPath) : (L"NULL"));
    }

    // private path

    hr = ::AppCtxGetWrapper(pAppCtx, ACTAG_APP_PRIVATE_BINPATH, &wzPrivatePath);
    if (FAILED(hr)) {
        goto Exit;
    }

    DEBUGOUT1(pdbglog, 0, ID_FUSLOG_PREBIND_INFO_PRIVATE_PATH, (wzPrivatePath) ? (wzPrivatePath) : (L"NULL"));

    // dynamic base

    hr = ::AppCtxGetWrapper(pAppCtx, ACTAG_APP_DYNAMIC_BASE, &wzDynamicBase);
    if (FAILED(hr)) {
        goto Exit;
    }

    DEBUGOUT1(pdbglog, 1, ID_FUSLOG_PREBIND_INFO_DYNAMIC_BASE, (wzDynamicBase) ? (wzDynamicBase) : (L"NULL"));

    // cache base

    hr = ::AppCtxGetWrapper(pAppCtx, ACTAG_APP_CACHE_BASE, &wzCacheBase);
    if (FAILED(hr)) {
        goto Exit;
    }

    DEBUGOUT1(pdbglog, 1, ID_FUSLOG_PREBIND_INFO_CACHE_BASE, (wzCacheBase) ? (wzCacheBase) : (L"NULL"));

    // App name

    hr = ::AppCtxGetWrapper(pAppCtx, ACTAG_APP_NAME, &wzAppName);
    if (FAILED(hr)) {
        goto Exit;
    }

    DEBUGOUT1(pdbglog, 1, ID_FUSLOG_PREBIND_INFO_APP_NAME, (wzAppName) ? (wzAppName) : (L"NULL"));

    // Calling assembly

    DEBUGOUT1(pdbglog, 0, ID_FUSLOG_CALLING_ASSEMBLY, ((pwzCallingAsm) ? (pwzCallingAsm) : L"(Unknown)"));

    // Output debug info trailer

    DEBUGOUT(pdbglog, 0, ID_FUSLOG_PREBIND_INFO_END);


Exit:
    SAFEDELETEARRAY(wzName);
    SAFEDELETEARRAY(wzAppBase);
    SAFEDELETEARRAY(wzPrivatePath);
    SAFEDELETEARRAY(wzDynamicBase);
    SAFEDELETEARRAY(wzCacheBase);
    SAFEDELETEARRAY(wzAppName);
    SAFEDELETEARRAY(wzDevPath);

    return hr;
}

// ---------------------------------------------------------------------------
// CAssemblyName::Finalize
// ---------------------------------------------------------------------------
STDMETHODIMP
CAssemblyName::Finalize()
{
    _fIsFinalized = TRUE;
    return S_OK;
}

// ---------------------------------------------------------------------------
// CAssemblyName::GetDisplayName
// ---------------------------------------------------------------------------
STDMETHODIMP
CAssemblyName::GetDisplayName(LPOLESTR szDisplayName, 
    LPDWORD pccDisplayName, DWORD dwDisplayFlags)
{
    HRESULT hr;

    WORD wVer;

    DWORD ccBuf, cbTmp, i, ccVersion, 
        cbCulture, ccCulture, ccProp;

    WCHAR *pszBuf = NULL, *pszName = NULL, *szProp = NULL,
        szVersion[MAX_VERSION_DISPLAY_SIZE + 1], *szCulture = NULL;

    LPBYTE pbProp = NULL;   

    if (!dwDisplayFlags)
        dwDisplayFlags = 
            ASM_DISPLAYF_VERSION | ASM_DISPLAYF_CULTURE | ASM_DISPLAYF_PUBLIC_KEY_TOKEN;

    // Reference internal name, strong name, public key and custom.
    Property propName   = _rProp[ASM_NAME_NAME];
    Property propSN     = _rProp[ASM_NAME_PUBLIC_KEY_TOKEN];
    Property propPK     = _rProp[ASM_NAME_PUBLIC_KEY];
    Property propCustom = _rProp[ASM_NAME_CUSTOM];

    // Validate input buffer.
    if (!pccDisplayName || (!szDisplayName && *pccDisplayName))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    pszBuf = szDisplayName;
    ccBuf = 0;

    // Name required
    if (!propName.cb)
    {
        hr = E_INVALIDARG;
        goto exit;
    }
    
    // Reference name.
    pszName = (LPWSTR) propName.pv;

    // Output name.
    if (FAILED(hr = CParseUtils::SetKey(pszBuf, &ccBuf, 
        pszName, *pccDisplayName, NULL)))
        goto exit;
    
    // Output version if extant.
    if (dwDisplayFlags & ASM_DISPLAYF_VERSION)
    {
        // Convert versions to a.b.c.d format.
        ccVersion = 0;
        for (i = 0; i < 4; i++)
        {
            // Get the version.
            if (FAILED(hr=GetProperty(ASM_NAME_MAJOR_VERSION + i, 
                &wVer, &(cbTmp = sizeof(WORD)))))
                goto exit;

            // No version -> we're done.
            if (!cbTmp)
                break;

            // Print to buf.        
            ccVersion += wnsprintf(szVersion + ccVersion, 
                MAX_VERSION_DISPLAY_SIZE - ccVersion + 1, L"%hu.", wVer);
        }
   
        // Output version.
        if (ccVersion)
        {
            // Remove last '.' printed in above loop.
            szVersion[ccVersion-1] = L'\0';
            if (FAILED(hr = CParseUtils::SetKeyValuePair(pszBuf, &ccBuf, L"Version",
                szVersion, *pccDisplayName, FLAG_DELIMIT)))
                goto exit;
        }
    }

    // Display culture
    if (dwDisplayFlags & ASM_DISPLAYF_CULTURE)
    {
        hr = GetProperty(ASM_NAME_CULTURE, NULL, &(cbCulture = 0));

        if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
        {
            ccCulture = cbCulture / sizeof(WCHAR);
            szCulture = NEW(WCHAR[ccCulture]);
            if (!szCulture)
            {
                hr = E_OUTOFMEMORY;
                goto exit;
            }

            // Retrieve the Culture.
            if (FAILED(hr = GetProperty(ASM_NAME_CULTURE, szCulture, &cbCulture)))
                goto exit;         

            // Output.
            if (FAILED(hr = CParseUtils::SetKeyValuePair(pszBuf, &ccBuf, L"Culture",
                (cbCulture && !*szCulture) ? L"neutral" : szCulture,
                *pccDisplayName, FLAG_DELIMIT)))
                goto exit;
        }
        else if (hr != S_OK) 
        {
            // Unexpected error.
            goto exit;
        }    
    }
    
    // Output public key and/or public key token
    for (i = 0; i < 2; i++)
    {
        if ((i == 0 && !(dwDisplayFlags & ASM_DISPLAYF_PUBLIC_KEY_TOKEN))
            || (i == 1 && !(dwDisplayFlags & ASM_DISPLAYF_PUBLIC_KEY)))
            continue;
            
        Property *pProp;
        pProp = (!i ? &propSN : &propPK);
    
        if (pProp->cb)
        {        
            // Reference the value
            pbProp = (LPBYTE) pProp->pv;

            // Encode to hex in unicode - string
            // is twice as long + null terminator.
            ccProp = 2 * pProp->cb; 
            szProp = NEW(WCHAR[ccProp+1]);
            if (!szProp)
            {
                hr = E_OUTOFMEMORY;
                goto exit;
            }

            // Convert to unicode.
            CParseUtils::BinToUnicodeHex(pbProp, pProp->cb, szProp);
            szProp[ccProp] = L'\0';
        
            // Output property.
            if (FAILED(hr = CParseUtils::SetKeyValuePair(pszBuf, &ccBuf, 
                (!i ? L"PublicKeyToken" : L"PublicKey") , szProp, *pccDisplayName, FLAG_DELIMIT)))
                goto exit;

            SAFEDELETEARRAY(szProp);
        }
        else if (_fPublicKeyToken)
        {
            // Output property.
            if (FAILED(hr = CParseUtils::SetKeyValuePair(pszBuf, &ccBuf, 
                (!i ? L"PublicKeyToken" : L"PublicKey") , L"null", *pccDisplayName, FLAG_DELIMIT)))
                goto exit;
        }
            
        SAFEDELETEARRAY(szProp);
    }

    // Output custom property.
    if (dwDisplayFlags & ASM_DISPLAYF_CUSTOM)
    {
        if (propCustom.cb)
        {
            // Reference the value
            pbProp = (LPBYTE) propCustom.pv;
    
            // Encode to hex in unicode - string
            // is twice as long + null terminator.
            ccProp = 2 * propCustom.cb; 
            szProp = NEW(WCHAR[ccProp+1]);
            if (!szProp)
            {
                hr = E_OUTOFMEMORY;
                goto exit;
            }
    
            // Convert to unicode.
            CParseUtils::BinToUnicodeHex(pbProp, propCustom.cb, szProp);
            szProp[ccProp] = L'\0';
        
            // Output property.
            if (FAILED(hr = CParseUtils::SetKeyValuePair(pszBuf, &ccBuf, 
                L"Custom", szProp, *pccDisplayName, FLAG_DELIMIT)))
                goto exit;

            SAFEDELETEARRAY(szProp);
        }
        else if (_fCustom)
        {
            // Output property.
            if (FAILED(hr = CParseUtils::SetKeyValuePair(pszBuf, &ccBuf, 
                L"Custom", L"null", *pccDisplayName, FLAG_DELIMIT)))
                goto exit;
        }
    }    
    
    // If we came in under buffer size null terminate.
    // otherwise ccBuf contains required buffer size.
    if (ccBuf < *pccDisplayName)
    {
        pszBuf[ccBuf] = L'\0';
        hr = S_OK;
    }
    // Indicate to caller.
    else
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        
    // In either case indicate required size.
    *pccDisplayName = ccBuf+1;

exit:

    SAFEDELETEARRAY(szProp);
    SAFEDELETEARRAY(szCulture);
        
    return hr;
}

// ---------------------- Private funcs --------------------------------------

//--------------------------------------------------------------------
// Parse
//   Parses ASSEMBLYMETADATA and text name from simple string rep.
//--------------------------------------------------------------------
HRESULT CAssemblyName::Parse(LPWSTR szDisplayName)
{
    HRESULT hr;
    
    WCHAR *szBuffer = NULL, *pszBuf, *pszToken, *pszKey, *pszValue, *pszProp,
        *pszName = NULL, *pszVersion = NULL, *pszPK = NULL, *pszSN = NULL,
        *pszCulture = NULL, *pszRef = NULL, *pszDef = NULL, *pszCustom = NULL;

    DWORD ccBuffer, ccKey, ccValue, ccBuf, ccToken, ccProp,
        ccName = 0, ccVersion = 0, ccPK = 0, ccSN = 0, 
        ccCulture = 0, ccRef = 0, ccDef = 0, ccCustom = 0;
        
    WORD wVer[4] = {0,0,0,0};

    DWORD i=0, nVers=0;
    
    DWORD cbProp;
    BYTE *pbProp = NULL;
    
    // Verify display name passed in.
    if (!szDisplayName)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    if (!*szDisplayName)
    {
        hr = FUSION_E_INVALID_NAME;
        goto exit;
    }
    
    // Make local copy for parsing.
    ccBuffer = lstrlen(szDisplayName) + 1;
    szBuffer = NEW(WCHAR[ccBuffer]);
    if (!szBuffer)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    memcpy(szBuffer, szDisplayName, ccBuffer * sizeof(WCHAR));

    // Begin parsing buffer.
    pszBuf = szBuffer;
    ccBuf = ccBuffer;
    
    // Get first comma or NULL delimited token.
    if (!CParseUtils::GetDelimitedToken(&pszBuf, &ccBuf, &pszToken, &ccToken, L','))
    {
        hr = FUSION_E_INVALID_NAME;
        goto exit;
    }
    
    // Point to name and 0 terminate.
    pszName = pszToken;
    ccName = ccToken;
    *(pszName + ccName) = L'\0';

    // Get additional params. 
    while (CParseUtils::GetDelimitedToken(&pszBuf, &ccBuf, &pszToken, &ccToken, L','))
    {
        // Parse key=vaue form
        if (CParseUtils::GetKeyValuePair(pszToken, ccToken, 
            &pszKey, &ccKey, &pszValue, &ccValue))
        {
            // Trim one pair of quotes.
            if (ccValue && (*pszValue == L'"'))
            {
                pszValue++;
                ccValue--;
            }
            if (ccValue && (*(pszValue + ccValue - 1) == L'"'))
                ccValue--;

            // Version
            if (ccValue && ccKey == (DWORD)lstrlenW(L"Version") &&
                !StrCmpNI(pszKey, L"Version", ccKey))
            {
                // Record version, 0 terminate.
                pszVersion = pszValue;
                ccVersion  = ccValue;
                *(pszVersion + ccVersion) = L'\0';
            }
            // Public key
            else if (ccValue && ccKey == (DWORD)lstrlenW(L"PublicKey") &&
                     !StrCmpNI(pszKey, L"PublicKey", ccKey))
            {
                // PK, 0 terminate.
                pszPK = pszValue;
                ccPK  = ccValue;
                if(ccPK % 2) 
                {
                    hr = FUSION_E_INVALID_NAME;
                    goto exit;
                }
                *(pszPK + ccPK) = L'\0';
            }
            // Ref flags
            else if (ccValue && ccKey == (DWORD)lstrlenW(L"fRef") &&
                     !StrCmpNI(pszKey, L"fRef", ccKey))
            {
                pszRef = pszValue;
                ccRef = ccValue;
                *(pszRef + ccRef) = L'\0';
            }
            // Def flags
            else if (ccValue && ccKey == (DWORD)lstrlenW(L"fDef") &&
                     !StrCmpNI(pszKey, L"fDef", ccKey))
            {
                pszDef = pszValue;
                ccDef = ccValue;
                *(pszDef + ccDef) = L'\0';
            }
            // Strong name (SN or OR)
            else if (ccValue && ccKey == (DWORD)lstrlenW(L"PublicKeyToken") &&
                     (!StrCmpNI(pszKey, L"PublicKeyToken", ccKey)))
            {
                // SN, 0 terminate.
                pszSN = pszValue;
                ccSN  = ccValue;
                if(ccSN % 2) 
                {
                    hr = FUSION_E_INVALID_NAME;
                    goto exit;
                }
                *(pszSN + ccSN) = L'\0';
            }
            // Culture
            else if (pszValue && ccKey == (DWORD)lstrlenW(L"Culture") &&
                     (!StrCmpNI(pszKey, L"Culture", ccKey)))
            {
                pszCulture = pszValue;
                ccCulture  = ccValue;
                *(pszCulture + ccCulture) = L'\0';
            }
            // Custom
            else if (ccValue && ccKey == (DWORD)lstrlenW(L"Custom") &&
                     !StrCmpNI(pszKey, L"Custom", ccKey))
            {
                // Custom, 0 terminate.
                pszCustom = pszValue;
                ccCustom  = ccValue;
                if (ccCustom % 2)
                {
                    hr = FUSION_E_INVALID_NAME;
                    goto exit;
                }                
                *(pszCustom + ccCustom) = L'\0';
            }
        }
    }

    // Parse major, minor, rev# and bld#
    // from version string if extant.
    if (pszVersion)
    {
        pszBuf = pszVersion;
        ccBuf  = ccVersion + 1;

        INT iVersion;
        while (CParseUtils::GetDelimitedToken(&pszBuf, &ccBuf, &pszToken, &ccToken, L'.'))
        {                    
            if (nVers < VERSION_STRING_SEGMENTS) {
                iVersion = StrToInt(pszToken);
                if (iVersion > 0xffff)
                {
                    hr = FUSION_E_INVALID_NAME;
                    goto exit;
                }
                wVer[nVers++] = (WORD)iVersion;
            }
        }            
    }
    
    // Set name.
    if (FAILED(hr = SetProperty(ASM_NAME_NAME, (LPWSTR) pszName, 
        (ccName + 1) * sizeof(WCHAR))))
        goto exit;
        
    // Set version info.
    for (i = 0; i < nVers; i++)
    {
        if (FAILED(hr = SetProperty(i + ASM_NAME_MAJOR_VERSION,
            &wVer[i], sizeof(WORD))))
            goto exit;
    }

    // Set public key and/or public key token
    for (i = 0; i < 2; i++)
    {
        pszProp = (i ? pszSN : pszPK);
        ccProp  = (i ? ccSN  : ccPK);

        if (pszProp)
        {
            // SN=NULL/PK=NULL sets null property.
            if ((ccProp == (sizeof("NULL") - 1)) 
                && !(StrCmpNIW(pszProp, L"NULL", sizeof("NULL") - 1)))
            {
                if (FAILED(hr = SetProperty(ASM_NAME_NULL_PUBLIC_KEY + i, NULL, 0)))
                    goto exit;
            }
            // Otherwise setting public key or public key token.
            else
            {
                cbProp = ccProp / 2;
                pbProp = NEW(BYTE[cbProp]);
                if (!pbProp)
                {
                    hr = E_OUTOFMEMORY;
                    goto exit;
                }                
                CParseUtils::UnicodeHexToBin(pszProp, ccProp, pbProp);
                if (FAILED(hr = SetProperty(ASM_NAME_PUBLIC_KEY + i, pbProp, cbProp)))
                    goto exit;
                SAFEDELETEARRAY(pbProp);
            }
        }
    }
    
    // Culture if any specified.
    if (pszCulture)
    {
        if (!ccCulture || !StrCmpIW(pszCulture, L"neutral"))
        {
            LPWSTR nullStr = L"\0";
            if (FAILED(hr = SetProperty(ASM_NAME_CULTURE, nullStr, sizeof(WCHAR))))
                goto exit;
        }
        else if (FAILED(hr = SetProperty(ASM_NAME_CULTURE, pszCulture,
            (ccCulture + 1) * sizeof(WCHAR))))
            goto exit;            
    }

    // Custom if any specified
    if (pszCustom)
    {
        // Custom=null sets null property
        if ((ccCustom == (sizeof("NULL") - 1)) 
            && !(StrCmpNIW(pszCustom, L"NULL", sizeof("NULL") - 1)))
        {
            if (FAILED(hr = SetProperty(ASM_NAME_NULL_CUSTOM, NULL, 0)))
                goto exit;
        }   
        // Otherwise wildcarding Custom, which is automatically set
        // in constructor to IL (_fCustom = TRUE)
        else if ((ccCustom == (sizeof("*") - 1)) 
            && !(StrCmpNIW(pszCustom, L"*", sizeof("*") - 1)))
        {
            if (FAILED(hr = SetProperty(ASM_NAME_CUSTOM, NULL, 0)))
                goto exit;
        }   
        // Otherwise setting custom blob.
        else
        {
            cbProp = ccCustom / 2;
            pbProp = NEW(BYTE[cbProp]);
            if (!pbProp)
            {
                hr = E_OUTOFMEMORY;
                goto exit;
            }                
            CParseUtils::UnicodeHexToBin(pszCustom, ccCustom, pbProp);
            if (FAILED(hr = SetProperty(ASM_NAME_CUSTOM, pbProp, cbProp)))
                goto exit;
        }
    }        


exit:
    
    SAFEDELETEARRAY(szBuffer);
    SAFEDELETEARRAY(pbProp);
    
    _cRef = 1;
        
    return hr;
}

HRESULT CAssemblyName::DownloadAppCfg(IApplicationContext *pAppCtx,
                                      CAssemblyDownload *padl,
                                      IAssemblyBindSink *pbindsink,
                                      CDebugLog *pdbglog)
{
    HRESULT                                hr = S_OK;
    DWORD                                  dwLen;
    DWORD                                  dwFileAttr;
    LPWSTR                                 wszURL=NULL;
    WCHAR                                  wszAppCfg[MAX_PATH];
    LPWSTR                                 wszAppBase=NULL;
    LPWSTR                                 wszAppBaseStr = NULL;
    LPWSTR                                 wszAppCfgFile = NULL;
    BOOL                                   bIsFileUrl = FALSE;

    if (!pAppCtx) {
        hr = E_INVALIDARG;
        goto Exit;
    }
    
    DEBUGOUT(pdbglog, 1, ID_FUSLOG_APP_CFG_DOWNLOAD);

    hr = ::AppCtxGetWrapper(pAppCtx, ACTAG_APP_BASE_URL, &wszAppBaseStr);
    if (FAILED(hr) || hr == S_FALSE) {
        goto Exit;
    }

    ASSERT(lstrlenW(wszAppBaseStr));

    if (!wszAppBaseStr || !lstrlenW(wszAppBaseStr)) {
        hr = E_UNEXPECTED;
        goto Exit;
    }

    wszAppBase = NEW(WCHAR[MAX_URL_LENGTH*2+2]);
    if (!wszAppBase)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    wszURL = wszAppBase + MAX_URL_LENGTH + 1;

    lstrcpyW(wszAppBase, wszAppBaseStr);

    dwLen = lstrlenW(wszAppBase) - 1;
    if (wszAppBase[dwLen] != L'/' && wszAppBase[dwLen] != L'\\') {
        if (dwLen + 1 >= MAX_URL_LENGTH) {
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            goto Exit;
        }
        StrCatW(wszAppBase, L"/");
    }

    hr = ::AppCtxGetWrapper(pAppCtx, ACTAG_APP_CONFIG_FILE, &wszAppCfgFile);
    if (FAILED(hr) || hr == S_FALSE) {
        goto Exit;
    }

    dwLen = MAX_URL_LENGTH;
    hr = UrlCombineUnescape(wszAppBase, wszAppCfgFile, wszURL, &dwLen, 0);
    if (hr == S_OK) {
        DEBUGOUT1(pdbglog, 1, ID_FUSLOG_APP_CFG_DOWNLOAD_LOCATION, wszURL);

        bIsFileUrl = UrlIsW(wszURL, URLIS_FILEURL);
        if (bIsFileUrl) {
            dwLen = MAX_PATH;
            hr = PathCreateFromUrlWrap(wszURL, wszAppCfg, &dwLen, 0);
            if (FAILED(hr)) {
                goto Exit;
            }

            // Indicate that we've already searched for app.cfg

            pAppCtx->Set(ACTAG_APP_CFG_DOWNLOAD_ATTEMPTED, (void *)L"", sizeof(L""), 0);

            dwFileAttr = GetFileAttributes(wszAppCfg);
            if (dwFileAttr == (DWORD) -1 || (dwFileAttr & FILE_ATTRIBUTE_DIRECTORY)) {
                // App.cfg does not exist (or user incorrectly specified a
                // directory!).
                PrepareBindHistory(pAppCtx);
                DEBUGOUT(pdbglog, 0, ID_FUSLOG_APP_CFG_NOT_EXIST);
                hr = S_FALSE;
                goto Exit;
            }
            else {
                // Add app.cfg filepath to appctx
                DEBUGOUT1(pdbglog, 0, ID_FUSLOG_APP_CFG_FOUND, wszAppCfg);
                hr = SetAppCfgFilePath(pAppCtx, wszAppCfg);
                ASSERT(hr == S_OK);
                PrepareBindHistory(pAppCtx);
            }
        }
        else {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
            goto Exit;
        }
    }

Exit:
    if (wszAppBase) {
        delete [] wszAppBaseStr;
    }

    if (wszAppCfg) {
        delete [] wszAppCfgFile;
    }

    SAFEDELETEARRAY(wszAppBase);
    return hr;
}


// ---------------------------------------------------------------------------
// CAssemblyName::GetPublicKeyTokenFromPKBlob
// ---------------------------------------------------------------------------
HRESULT CAssemblyName::GetPublicKeyTokenFromPKBlob(LPBYTE pbPublicKeyToken, DWORD cbPublicKeyToken,
    LPBYTE *ppbSN, LPDWORD pcbSN)
{    
    HRESULT hr = S_OK;

    hr = InitializeEEShim();
    if (FAILED(hr)) {
        goto Exit;
    }

    // Generate the hash of the public key.
    if (!g_pfnStrongNameTokenFromPublicKey(pbPublicKeyToken, cbPublicKeyToken, ppbSN, pcbSN))
    {
        hr = g_pfnStrongNameErrorInfo();
    }

Exit:
    return hr;
}

// ---------------------------------------------------------------------------
// CAssemblyName::GetPublicKeyToken
// ---------------------------------------------------------------------------
HRESULT CAssemblyName::GetPublicKeyToken(LPDWORD pcbBuf, LPBYTE pbBuf,
    BOOL fDisplay)
{    
    HRESULT hr;
    LPBYTE pbPublicKeyToken = NULL;
    DWORD cbPublicKeyToken = 0, cbRequired = 0;

    if (!pcbBuf) {
        hr = E_INVALIDARG;
        goto exit;
    }

    // If display format is not required then call 
    // GetProperty directly on the provided buffer.
    if (!fDisplay)
    {
        hr = GetProperty(ASM_NAME_PUBLIC_KEY_TOKEN, pbBuf, pcbBuf);
        goto exit;
    }

    // Otherwise, display format required.

    // Get the required public key token output buf size.
    hr = GetProperty(ASM_NAME_PUBLIC_KEY_TOKEN, NULL, &cbPublicKeyToken);

    // No public key token or unexpected error
    if (!cbPublicKeyToken || (hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)))
    {
        *pcbBuf = cbPublicKeyToken;
        goto exit;
    }

    // We will convert binary format to hex encoded unicode - 
    // Calculated actual output buffer size in bytes - 
    // one byte maps to two unicode chars = 4 bytes/byte + null term.
    cbRequired = (2 * cbPublicKeyToken + 1) * sizeof(WCHAR);

    // Inform client if insufficient buffer
    if (*pcbBuf < cbRequired)
    {
        *pcbBuf = cbRequired;
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        goto exit;
    }

    if (!pbBuf) {
        if (pcbBuf) {
            *pcbBuf = cbRequired;
        }

        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        goto exit;
    }

    // Allocate temp space for public key token
    pbPublicKeyToken = NEW(BYTE[cbPublicKeyToken]);
    if (!pbPublicKeyToken)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Get the public key token
    if (FAILED(hr = GetProperty(ASM_NAME_PUBLIC_KEY_TOKEN, pbPublicKeyToken, &cbPublicKeyToken)))
        goto exit;

    // Convert to unicode.
    CParseUtils::BinToUnicodeHex(pbPublicKeyToken, cbPublicKeyToken, (LPWSTR) pbBuf);

    *pcbBuf = cbRequired;

exit:
    SAFEDELETEARRAY(pbPublicKeyToken);
    return hr;
}

// ---------------------------------------------------------------------------
// CAssemblyName::SetDefaults
// ---------------------------------------------------------------------------
HRESULT CAssemblyName::SetDefaults()
{
    HRESULT                              hr = S_OK;


    return hr;
}

// ---------------------------------------------------------------------------
// CAssemblyName::IsPartial
// ---------------------------------------------------------------------------
BOOL CAssemblyName::IsPartial(IAssemblyName *pIName, LPDWORD pdwCmpMask)
{
    DWORD dwCmpMask = 0;
    BOOL fPartial    = FALSE;

    static const ASM_NAME rNameFlags[8] = {ASM_NAME_NAME, ASM_NAME_CULTURE,
        ASM_NAME_PUBLIC_KEY_TOKEN, ASM_NAME_MAJOR_VERSION, ASM_NAME_MINOR_VERSION,
        ASM_NAME_BUILD_NUMBER, ASM_NAME_REVISION_NUMBER, ASM_NAME_CUSTOM};

    static const ASM_CMP_FLAGS rCmpFlags[8] = {ASM_CMPF_NAME, ASM_CMPF_CULTURE,
        ASM_CMPF_PUBLIC_KEY_TOKEN, ASM_CMPF_MAJOR_VERSION, ASM_CMPF_MINOR_VERSION,
        ASM_CMPF_BUILD_NUMBER, ASM_CMPF_REVISION_NUMBER, ASM_CMPF_CUSTOM};

    CAssemblyName *pName = static_cast<CAssemblyName*> (pIName); // dynamic_cast
    ASSERT(pName);
    
    for (INT i = 0; i < 8; i++)
    {
        if (pName->_rProp[rNameFlags[i]].cb 
            || (rNameFlags[i] == ASM_NAME_PUBLIC_KEY_TOKEN
                && pName->_fPublicKeyToken)
            || (rNameFlags[i] == ASM_NAME_CUSTOM 
                && pName->_fCustom))
        {
            dwCmpMask |= rCmpFlags[i];            
        }
        else
        {
            fPartial = TRUE;
        }
    }

    if (pdwCmpMask)
        *pdwCmpMask = dwCmpMask;

    return fPartial;
}


// ---------------------------------------------------------------------------
// CAssemblyName::GetVersion
//---------------------------------------------------------------------------
ULONGLONG CAssemblyName::GetVersion(IAssemblyName *pName)
{
    ULONGLONG ullVer = 0;
    DWORD dwVerHigh = 0, dwVerLow = 0;
    pName->GetVersion(&dwVerHigh, &dwVerLow);
    ullVer = ((ULONGLONG) dwVerHigh) << sizeof(DWORD) * 8;
    ullVer |= (ULONGLONG) dwVerLow;
    return ullVer;
}

// ---------------------------------------------------------------------------
// CAssemblyName::ProcessDevPath
//---------------------------------------------------------------------------

HRESULT CAssemblyName::ProcessDevPath(IApplicationContext *pAppCtx, LPVOID *ppv,
                                      CAssembly *pCAsmParent, CDebugLog *pdbglog)
{
    HRESULT                                      hr = S_OK;
    WCHAR                                        wzAsmPath[MAX_PATH + 1];
    LPWSTR                                       pwzDevPath = NULL;
    LPWSTR                                       pwzDevPathBuf = NULL;
    LPWSTR                                       pwzCurPath = NULL;
    static const WCHAR                           pwzExtDLL[] = L".dll";
    static const WCHAR                           pwzExtEXE[] = L".exe";
    IAssemblyManifestImport                     *pManImport = NULL;
    IAssemblyName                               *pNameDef = NULL;
    IAssemblyName                               *pNameDevPath = NULL;
    IAssembly                                   *pAsmDevPath = NULL;
    IAssembly                                   *pAsmActivated = NULL;
    Property                                     propName = _rProp[ASM_NAME_NAME];
    LPWSTR                                       pwzAsmName = (LPWSTR)propName.pv;
    FILETIME                                     ftLastModified;
    LPWSTR                                       wzURL=NULL;
    BOOL                                         bWasVerified = FALSE;
    DWORD                                        dwVerifyFlags = SN_INFLAG_USER_ACCESS;
    DWORD                                        dwLen;
    CAssembly                                   *pCAsmDevPath = NULL;
    CLoadContext                                *pLoadContextParent = NULL;
    CLoadContext                                *pLoadCtxDefault = NULL;
    LPWSTR                                       wzProbingBase=NULL;
    DWORD                                        dwCmpFlags = ASM_CMPF_NAME |
                                                              ASM_CMPF_PUBLIC_KEY_TOKEN |
                                                              ASM_CMPF_CULTURE;

    DEBUGOUT(pdbglog, 1, ID_FUSLOG_PROCESS_DEVPATH);

    if (!pAppCtx || !ppv) {
        hr = E_INVALIDARG;
        goto Exit;
    }
    
    if (!pwzAsmName || !lstrlenW(pwzAsmName)) {
        // This is a where-ref bind (no name is set). Bypass dev path lookup.
        hr = S_FALSE;
        goto Exit;
    }

    hr = ::AppCtxGetWrapper(pAppCtx, ACTAG_DEV_PATH, &pwzDevPathBuf);
    if (hr != S_OK) {
        // Dev path is not set. Fall through to regular bind.
        DEBUGOUT(pdbglog, 1, ID_FUSLOG_DEVPATH_UNSET);
        hr = S_FALSE;
        goto Exit;
    }

    // Extract parent assembly information

    wzProbingBase = NEW(WCHAR[MAX_URL_LENGTH*2+2]);
    if (!wzProbingBase)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    wzURL = wzProbingBase + MAX_URL_LENGTH + 1;

    wzProbingBase[0] = L'\0';

    if (pCAsmParent) {
        // We have a parent assembly, so set the load context and
        // probing base to that of the parent.

        hr = pCAsmParent->GetLoadContext(&pLoadContextParent);
        if (FAILED(hr)) {
            ASSERT(0);
            goto Exit;
        }

        dwLen = MAX_URL_LENGTH;
        hr = pCAsmParent->GetProbingBase(wzProbingBase, &dwLen);
        if (FAILED(hr)) {
            ASSERT(0);
            goto Exit;
        }
    }

    // Look for assembly in devpath

    ASSERT(pwzDevPathBuf);

    pwzDevPath = pwzDevPathBuf;

    while (pwzDevPath) {
        pwzCurPath = ::GetNextDelimitedString(&pwzDevPath, DEVPATH_DIR_DELIMITER);

        PathRemoveBackslash(pwzCurPath);
        wnsprintfW(wzAsmPath, MAX_PATH, L"%ws\\%ws%ws", pwzCurPath, pwzAsmName, pwzExtDLL);

        if (GetFileAttributes(wzAsmPath) == (DWORD) -1) {
            // File does not exist, try next path

            DEBUGOUT1(pdbglog, 1, ID_FUSLOG_DEVPATH_PROBE_MISS, wzAsmPath);

            wnsprintfW(wzAsmPath, MAX_PATH, L"%ws\\%ws%ws", pwzCurPath, pwzAsmName, pwzExtEXE);

            if (GetFileAttributes(wzAsmPath) == (DWORD) -1) {

                DEBUGOUT1(pdbglog, 1, ID_FUSLOG_DEVPATH_PROBE_MISS, wzAsmPath);
                continue;
            }
        }
    
        // File exists. Crack the manifest and do ref/def matching
        
        hr = CreateAssemblyManifestImport(wzAsmPath, &pManImport);
        if (FAILED(hr)) {
            goto Exit;
        }
    
        // Get read-only name def from manifest.
        hr = pManImport->GetAssemblyNameDef(&pNameDef);
        if (FAILED(hr)) {
            goto Exit;
        }
    
        hr = IsEqualLogging(pNameDef, dwCmpFlags, pdbglog);
        if (hr != S_OK) {
            // Found a file, but it is the wrong one. Try next path.

            DEBUGOUT1(pdbglog, 1, ID_FUSLOG_DEVPATH_REF_DEF_MISMATCH, wzAsmPath);
            continue;
        }

        // Verify signature
        if (CCache::IsStronglyNamed(pNameDef))
        {
            if (!VerifySignature(wzAsmPath, &bWasVerified, dwVerifyFlags)) {
                hr = FUSION_E_SIGNATURE_CHECK_FAILED;
                goto Exit;
            }
        }

        // Matching assembly found in dev path. Create the IAssembly.
    
        dwLen = MAX_URL_LENGTH;
        hr = UrlCanonicalizeUnescape(wzAsmPath, wzURL, &dwLen, 0);
        if (FAILED(hr)) {
            goto Exit;
        }
    
        hr = GetFileLastModified(wzAsmPath, &ftLastModified);
        if (FAILED(hr)) {
            goto Exit;
        }

        hr = CreateAssemblyFromManifestImport(pManImport, wzURL, &ftLastModified,
                                              &pAsmDevPath);
        if (FAILED(hr)) {
            goto Exit;
        }

        // Check if the name def exists in the load context already.
        // If so, then use the already activated one, instead of the
        // devpath one.

        hr = pAsmDevPath->GetAssemblyNameDef(&pNameDevPath);
        if (FAILED(hr)) {
            goto Exit;
        }

        // Not found in parent load context. Continue to hand out this
        // assembly.

        // Set load context and probing base

        pCAsmDevPath = static_cast<CAssembly *>(pAsmDevPath); // dynamic_cast
        ASSERT(pCAsmDevPath);

        if (pLoadContextParent) {
            if (lstrlenW(wzProbingBase)) {
                
                pCAsmDevPath->SetProbingBase(wzProbingBase);
            }
    
            hr = pLoadContextParent->AddActivation(pAsmDevPath, &pAsmActivated);
            if (FAILED(hr)) {
                goto Exit;
            }
            else if (hr == S_FALSE) {
                *ppv = pAsmActivated;
                hr = S_OK;
                goto Exit;
            }
        }
        else {
            // We do not have a parent, so just set the load context to
            // the default load context.

            dwLen = sizeof(pLoadCtxDefault);
            hr = pAppCtx->Get(ACTAG_LOAD_CONTEXT_DEFAULT, &pLoadCtxDefault, &dwLen, APP_CTX_FLAGS_INTERFACE);
            if (FAILED(hr)) {
                ASSERT(0);
                return hr;
            }

            hr = pLoadCtxDefault->AddActivation(pAsmDevPath, &pAsmActivated);
            if (FAILED(hr)) {
                goto Exit;
            }
            else if (hr == S_FALSE) {
                *ppv = pAsmActivated;
                hr = S_OK;
                goto Exit;
            }
        }
    
        // Successful dev path lookup. AddRef out param and we're finished.
    
        *ppv = pAsmDevPath;
        pAsmDevPath->AddRef();

        DEBUGOUT1(pdbglog, 1, ID_FUSLOG_DEVPATH_FOUND, wzAsmPath);

        goto Exit;
    }

    // Didn't find the assembly. Fall through to regular bind.

    DEBUGOUT(pdbglog, 1, ID_FUSLOG_DEVPATH_NOT_FOUND);

    hr = S_FALSE;

Exit:
    SAFEDELETEARRAY(pwzDevPathBuf);

    SAFERELEASE(pManImport);
    SAFERELEASE(pAsmDevPath);
    SAFERELEASE(pNameDef);
    SAFERELEASE(pAsmActivated);
    SAFERELEASE(pNameDevPath);
    SAFERELEASE(pLoadContextParent);
    SAFERELEASE(pLoadCtxDefault);

    SAFEDELETEARRAY(wzProbingBase);

    return hr;
}                                      

//
// PreBindAssembly
//
// Returns:
//    FUSION_E_REF_DEF_MISMATCH: Policy will be applied to the pName, and
//                               will result in a different def.
//
//    S_OK: Policy will not affect the pName. 
//
//    S_FALSE: Policy will not affect the pName, but we do not know the MVID.
//

STDAPI PreBindAssembly(IApplicationContext *pAppCtx, IAssemblyName *pName,
                       IAssembly *pAsmParent, IAssemblyName **ppNamePostPolicy,
                       LPVOID pvReserved)
{
    HRESULT                               hr = S_OK;
    LPWSTR                                pwzDevPathBuf = NULL;
    LPWSTR                                pwzDevPath = NULL;
    LPWSTR                                pwzCurPath = NULL;
    LPWSTR                                pwzAppCfg = NULL;
    LPWSTR                                pwzHostCfg = NULL;
    LPWSTR                                pwzAsmName = NULL;
    WCHAR                                 wzAsmPath[MAX_PATH];
    DWORD                                 dwSize;
    DWORD                                 dwCmpMask;
    CPolicyCache                         *pPolicyCache = NULL;
    CCache                               *pCache = NULL;
    CLoadContext                         *pLoadContext = NULL;
    CTransCache                          *pTransCache = NULL;
    IAssemblyName                        *pNamePolicy = NULL;
    IAssemblyName                        *pNameRefPolicy = NULL;
    IAssembly                            *pAsm = NULL;
    AsmBindHistoryInfo                    bindHistory;
    static const WCHAR                    pwzExtDLL[] = L".dll";
    static const WCHAR                    pwzExtEXE[] = L".exe";

    if (!pAppCtx || !pName || !ppNamePostPolicy) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    *ppNamePostPolicy = NULL;

    memset(&bindHistory, 0, sizeof(AsmBindHistoryInfo));

    hr = CCache::Create(&pCache, pAppCtx);
    if (FAILED(hr)) {
        goto Exit;
    }

    // Name ref must be fully-specified

    if (CAssemblyName::IsPartial(pName, &dwCmpMask) || !CCache::IsStronglyNamed(pName)) {
        hr = E_INVALIDARG;
        goto Exit;
    }
    
    // Get assembly name

    dwSize = 0;
    pName->GetName(&dwSize, NULL);

    if (!dwSize) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    pwzAsmName = NEW(WCHAR[dwSize]);
    if (!pwzAsmName) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    hr = pName->GetName(&dwSize, pwzAsmName);
    if (FAILED(hr)) {
        goto Exit;
    }

    // Extract load context

    if (!pAsmParent) {
        dwSize = sizeof(pLoadContext);
        hr = pAppCtx->Get(ACTAG_LOAD_CONTEXT_DEFAULT, &pLoadContext, &dwSize, APP_CTX_FLAGS_INTERFACE);
        if (FAILED(hr) || !pLoadContext) {
            hr = E_UNEXPECTED;
            goto Exit;
        }
    }
    else {
        CAssembly *pCAsm = static_cast<CAssembly *>(pAsmParent); // dynamic_cast
        ASSERT(pCAsm);

        hr = pCAsm->GetLoadContext(&pLoadContext);
        if (FAILED(hr)) {
            hr = E_UNEXPECTED;
            goto Exit;
        }
    }


    // Check devpath

    hr = ::AppCtxGetWrapper(pAppCtx, ACTAG_DEV_PATH, &pwzDevPathBuf);
    if (hr == S_OK) {
        pwzDevPath = pwzDevPathBuf;
    
        while (pwzDevPath) {
            pwzCurPath = ::GetNextDelimitedString(&pwzDevPath, DEVPATH_DIR_DELIMITER);
    
            PathRemoveBackslash(pwzCurPath);
            wnsprintfW(wzAsmPath, MAX_PATH, L"%ws\\%ws%ws", pwzCurPath, pwzAsmName, pwzExtDLL);
    
            if (GetFileAttributes(wzAsmPath) != (DWORD) -1) {
                // DLL exists in DEVPATH. Fail.

                hr = FUSION_E_REF_DEF_MISMATCH;
                goto Exit;
            }

            wnsprintfW(wzAsmPath, MAX_PATH, L"%ws\\%ws%ws", pwzCurPath, pwzAsmName, pwzExtEXE);

            if (GetFileAttributes(wzAsmPath) != (DWORD) -1) {
                // EXE exists in DEVPATH. Fail.

                hr = FUSION_E_REF_DEF_MISMATCH;
                goto Exit;
            }
        }
    }

    // Check policy cache
    
    dwSize = sizeof(pPolicyCache);
    hr = pAppCtx->Get(ACTAG_APP_POLICY_CACHE, &pPolicyCache, &dwSize, APP_CTX_FLAGS_INTERFACE);
    if (hr == S_OK) {
        // Policy cache exists.

        hr = pPolicyCache->LookupPolicy(pName, &pNamePolicy, &bindHistory);
        if (FAILED(hr)) {
            goto Exit;
        }

        if (hr == S_OK) {
            hr = pNamePolicy->Clone(ppNamePostPolicy);
            if (FAILED(hr)) {
                goto Exit;
            }
    
            if (pName->IsEqual(pNamePolicy, ASM_CMPF_DEFAULT) != S_OK) {
                // Policy resulted in a different ref. Fail.
    
                hr = FUSION_E_REF_DEF_MISMATCH;
                goto Exit;
            }
    
            // Return MVID from activated assemblies list.

            hr = pLoadContext->CheckActivated(pNamePolicy, &pAsm);
            if (hr == S_OK) {
                IAssemblyName                  *pNameDef;

                hr = pAsm->GetAssemblyNameDef(&pNameDef);
                if (FAILED(hr)) {
                    goto Exit;
                }

                // Use post-policy def since it has the MVID.

                SAFERELEASE(*ppNamePostPolicy);

                hr = pNameDef->Clone(ppNamePostPolicy);
                if (FAILED(hr)) {
                    goto Exit;
                }

                SAFERELEASE(pNameDef);
                goto Exit;
            }

            // If hr==S_FALSE, then we hit in the policy cache, but did not
            // find a hit in the activated asm list. Check the GAC. If it
            // doesn't exist in the GAC, then we don't know the MVID.

            hr = pCache->RetrieveTransCacheEntry(pName, ASM_CACHE_GAC, &pTransCache);
            if (pTransCache) {
                TRANSCACHEINFO *pInfo = (TRANSCACHEINFO *)pTransCache->_pInfo;
        
                hr = GetFusionInfo(pTransCache, NULL);
                if (FAILED(hr)) {
                    goto Exit;
                }
        
                ASSERT(pInfo->blobMVID.cbSize == sizeof(GUID));
        
                // Hit in cache. Copy off the MVID.
        
                hr = (*ppNamePostPolicy)->SetProperty(ASM_NAME_MVID, (pInfo->blobMVID).pBlobData, sizeof(GUID));
                if (FAILED(hr)) {
                    goto Exit;
                }
                    
                hr = S_OK;
                goto Exit;
            }
            else {
                // Don't know the MVID, but policy guaranteed to not be applied.
                hr = S_FALSE;
                goto Exit;
            }
        }

        // Missed in policy cache. Fall through to apply policy.
    }
    else {
        // Policy cache didn't exist in this context.

        hr = PreparePolicyCache(pAppCtx, &pPolicyCache);
        if (FAILED(hr)) {
            goto Exit;
        }
    }

    // Apply Policy

    hr = ::AppCtxGetWrapper(pAppCtx, ACTAG_APP_CFG_LOCAL_FILEPATH, &pwzAppCfg);
    if (FAILED(hr)) {
        goto Exit;
    }

    hr = ::AppCtxGetWrapper(pAppCtx, ACTAG_HOST_CONFIG_FILE, &pwzHostCfg);
    if (FAILED(hr)) {
        goto Exit;
    }

    hr = ApplyPolicy(pwzHostCfg, pwzAppCfg, pName, &pNameRefPolicy, NULL,
                     pAppCtx, &bindHistory, FALSE, NULL);
    if (FAILED(hr)) {
        goto Exit;
    }

    hr = pNameRefPolicy->Clone(ppNamePostPolicy);
    if (FAILED(hr)) {
        goto Exit;
    }

    // Record resolution in policy cache

    hr = pPolicyCache->InsertPolicy(pName, pNameRefPolicy, &bindHistory);
    if (FAILED(hr)) {
        goto Exit;
    }

    if (pName->IsEqual(pNameRefPolicy, ASM_CMPF_DEFAULT) != S_OK) {
        // Policy resulted in a different ref. Fail.

        hr = FUSION_E_REF_DEF_MISMATCH;
        goto Exit;
    }

    // Try to find assembly in the GAC.
    
    hr = pCache->RetrieveTransCacheEntry(pName, ASM_CACHE_GAC, &pTransCache);
    if (pTransCache) {
        TRANSCACHEINFO *pInfo = (TRANSCACHEINFO *)pTransCache->_pInfo;

        hr = GetFusionInfo(pTransCache, NULL);
        if (FAILED(hr)) {
            goto Exit;
        }

        ASSERT(pInfo->blobMVID.cbSize == sizeof(GUID));

        // Hit in cache. Copy off the MVID.

        hr = (*ppNamePostPolicy)->SetProperty(ASM_NAME_MVID, (pInfo->blobMVID).pBlobData, sizeof(GUID));
        if (FAILED(hr)) {
            goto Exit;
        }

        // done

        goto Exit;
    }

    // Assembly not found in cache, but we can guarantee that there won't
    // be policy. We don't know the MVID, because we can only find this
    // assembly via probing.

    hr = S_FALSE;

Exit:
    SAFEDELETEARRAY(pwzDevPathBuf);
    SAFEDELETEARRAY(pwzAppCfg);
    SAFEDELETEARRAY(pwzHostCfg);
    SAFEDELETEARRAY(pwzAsmName);

    SAFERELEASE(pPolicyCache);
    SAFERELEASE(pNamePolicy);
    SAFERELEASE(pNameRefPolicy);
    SAFERELEASE(pCache);
    SAFERELEASE(pTransCache);
    SAFERELEASE(pLoadContext);
    SAFERELEASE(pAsm);

    return hr;
}

