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
//*****************************************************************************
// MSCoree.cpp
//*****************************************************************************
#include "stdafx.h"                     // Standard header.

#include <utilcode.h>                   // Utility helpers.
#include <posterror.h>                  // Error handlers
#define INIT_GUIDS  
#include <corpriv.h>
#include <winwrap.h>
#include <internaldebug.h>
#include <mscoree.h>
#include "shimload.h"


class Thread;
Thread* SetupThread();


// For free-threaded marshaling, we must not be spoofed by out-of-process marshal data.
// Only unmarshal data that comes from our own process.
extern BYTE         g_UnmarshalSecret[sizeof(GUID)];
extern bool         g_fInitedUnmarshalSecret;


// Locals.
BOOL STDMETHODCALLTYPE EEDllMain( // TRUE on success, FALSE on error.
                       HINSTANCE    hInst,                  // Instance handle of the loaded module.
                       DWORD        dwReason,               // Reason for loading.
                       LPVOID       lpReserved);                // Unused.


// Meta data startup/shutdown routines.
void  InitMd();
void  UninitMd();
STDAPI  MetaDataDllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv);
STDAPI  MetaDataDllRegisterServerEx(HINSTANCE);
STDAPI  MetaDataDllUnregisterServer();
STDAPI  GetMDInternalInterface(
    LPVOID      pData, 
    ULONG       cbData, 
    DWORD       flags,                  // [IN] MDInternal_OpenForRead or MDInternal_OpenForENC
    REFIID      riid,                   // [in] The interface desired.
    void        **ppIUnk);              // [out] Return interface on success.
STDAPI GetMDInternalInterfaceFromPublic(
    void        *pIUnkPublic,           // [IN] Given scope.
    REFIID      riid,                   // [in] The interface desired.
    void        **ppIUnkInternal);      // [out] Return interface on success.
STDAPI GetMDPublicInterfaceFromInternal(
    void        *pIUnkPublic,           // [IN] Given scope.
    REFIID      riid,                   // [in] The interface desired.
    void        **ppIUnkInternal);      // [out] Return interface on success.
STDAPI MDReOpenMetaDataWithMemory(
    void        *pImport,               // [IN] Given scope. public interfaces
    LPCVOID     pData,                  // [in] Location of scope data.
    ULONG       cbData);                // [in] Size of the data pointed to by pData.


extern "C" {



// Globals.
HINSTANCE       g_hThisInst;            // This library.
LONG            g_cCorInitCount = -1;   // Ref counting for init code.



//*****************************************************************************
// Handle lifetime of loaded library.
//*****************************************************************************
BOOL WINAPI DllMain(HANDLE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    static HMODULE hModPALRT = NULL;
    static HMODULE hModMSCorPE = NULL;
    static HMODULE hModFusion = NULL;
    static HMODULE hModMSCorSN = NULL;

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        {
            // Save the module handle.
            g_hThisInst = (HINSTANCE)hInstance;

            // Register libraries.
            if ((hModPALRT = PAL_RegisterLibrary(L"rotor_palrt")) == NULL ||
                (hModMSCorPE = PAL_RegisterLibrary(L"mscorpe")) == NULL ||
                (hModFusion = PAL_RegisterLibrary(L"fusion")) == NULL ||
                (hModMSCorSN = PAL_RegisterLibrary(L"mscorsn")) == NULL)
            {
                return FALSE;
            }
            
            // Init unicode wrappers.
            OnUnicodeSystem();

            if (!EEDllMain((HINSTANCE)hInstance, dwReason, NULL))
                return (FALSE);
    
            InitMd();

            // Debug cleanup code.
            _DbgInit((HINSTANCE)hInstance);
        }
        break;

    case DLL_PROCESS_DETACH:
        {
            if (lpReserved) {
                if (BeforeFusionShutdown()) {

                    ReleaseFusionInterfaces();

                }
            }

            EEDllMain((HINSTANCE)hInstance, dwReason, NULL);

            UninitMd();
            _DbgUninit();
        
            // Unregister libraries.
            PAL_UnregisterLibrary(hModMSCorSN);
            PAL_UnregisterLibrary(hModFusion);
            PAL_UnregisterLibrary(hModMSCorPE);
            PAL_UnregisterLibrary(hModPALRT);
        }
        break;

    case DLL_THREAD_DETACH:
        {
            EEDllMain((HINSTANCE)hInstance, dwReason, NULL);
        }
        break;
    }


    return (true);
}


} // extern "C"




HINSTANCE GetModuleInst()
{
    return (g_hThisInst);
}

// %%Globals: ----------------------------------------------------------------

// ---------------------------------------------------------------------------
// %%Function: DllGetClassObjectInternal  %%Owner: NatBro   %%Reviewed: 00/00/00
// 
// Parameters:
//  rclsid                  - reference to the CLSID of the object whose
//                            ClassObject is being requested
//  iid                     - reference to the IID of the interface on the
//                            ClassObject that the caller wants to communicate
//                            with
//  ppv                     - location to return reference to the interface
//                            specified by iid
// 
// Returns:
//  S_OK                    - if successful, valid interface returned in *ppv,
//                            otherwise *ppv is set to NULL and one of the
//                            following errors is returned:
//  E_NOINTERFACE           - ClassObject doesn't support requested interface
//  CLASS_E_CLASSNOTAVAILABLE - clsid does not correspond to a supported class
// 
// Description:
//  Returns a reference to the iid interface on the main COR ClassObject.
//  This function is one of the required by-name entry points for COM
// DLL's. Its purpose is to provide a ClassObject which by definition
// supports at least IClassFactory and can therefore create instances of
// objects of the given class.
// ---------------------------------------------------------------------------
STDAPI InternalDllGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    LPVOID FAR *ppv)
{
    HRESULT hr = CLASS_E_CLASSNOTAVAILABLE;

    if (rclsid == CLSID_CorMetaDataDispenser || rclsid == CLSID_CorMetaDataDispenserRuntime ||
             rclsid == CLSID_CorRuntimeHost)
    {
        hr = MetaDataDllGetClassObject(rclsid, riid, ppv);
    }

    return hr;
}  // InternalDllGetClassObject


STDAPI DllGetClassObjectInternal(
				 REFCLSID rclsid,
				 REFIID riid,
				 LPVOID FAR *ppv)
{
    // InternalDllGetClassObject exists to resolve an issue
    // on FreeBSD, where libsscoree.so's DllGetClassObject's
    // call to DllGetClassObjectInternal() was being bound to
    // the implementation in libmscordbi.so, not the one in
    // libsscoree.so.  The fix is to disambiguate the name.
    return InternalDllGetClassObject(rclsid, riid, ppv);
}

STDAPI DllGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    LPVOID FAR *ppv)
{
    return InternalDllGetClassObject(rclsid, riid, ppv);
}


// ---------------------------------------------------------------------------
// %%Function: MetaDataGetDispenser
// This function gets the Dispenser interface given the CLSID and REFIID.
// ---------------------------------------------------------------------------
STDAPI MetaDataGetDispenser(            // Return HRESULT
    REFCLSID    rclsid,                 // The class to desired.
    REFIID      riid,                   // Interface wanted on class factory.
    LPVOID FAR  *ppv)                   // Return interface pointer here.
{
    IClassFactory *pcf = NULL;
    HRESULT hr;

    hr = MetaDataDllGetClassObject(rclsid, IID_IClassFactory, (void **) &pcf);
    if (FAILED(hr)) 
        return (hr);

    hr = pcf->CreateInstance(NULL, riid, ppv);
    pcf->Release();

    return (hr);
}


// ---------------------------------------------------------------------------
// %%Function: GetMetaDataInternalInterface
// This function gets the IMDInternalImport given the metadata on memory.
// ---------------------------------------------------------------------------
STDAPI  GetMetaDataInternalInterface(
    LPVOID      pData,                  // [IN] in memory metadata section
    ULONG       cbData,                 // [IN] size of the metadata section
    DWORD       flags,                  // [IN] MDInternal_OpenForRead or MDInternal_OpenForENC
    REFIID      riid,                   // [IN] desired interface
    void        **ppv)                  // [OUT] returned interface
{
    return GetMDInternalInterface(pData, cbData, flags, riid, ppv);
}

// ---------------------------------------------------------------------------
// %%Function: GetMetaDataInternalInterfaceFromPublic
// This function gets the internal scopeless interface given the public
// scopeless interface.
// ---------------------------------------------------------------------------
STDAPI  GetMetaDataInternalInterfaceFromPublic(
    void        *pv,                    // [IN] Given interface.
    REFIID      riid,                   // [IN] desired interface
    void        **ppv)                  // [OUT] returned interface
{
    return GetMDInternalInterfaceFromPublic(pv, riid, ppv);
}

// ---------------------------------------------------------------------------
// %%Function: GetMetaDataPublicInterfaceFromInternal
// This function gets the public scopeless interface given the internal
// scopeless interface.
// ---------------------------------------------------------------------------
STDAPI  GetMetaDataPublicInterfaceFromInternal(
    void        *pv,                    // [IN] Given interface.
    REFIID      riid,                   // [IN] desired interface.
    void        **ppv)                  // [OUT] returned interface
{
    return GetMDPublicInterfaceFromInternal(pv, riid, ppv);
}


// ---------------------------------------------------------------------------
// %%Function: ReopenMetaDataWithMemory
// This function gets the public scopeless interface given the internal
// scopeless interface.
// ---------------------------------------------------------------------------
STDAPI ReOpenMetaDataWithMemory(
    void        *pUnk,                  // [IN] Given scope. public interfaces
    LPCVOID     pData,                  // [in] Location of scope data.
    ULONG       cbData)                 // [in] Size of the data pointed to by pData.
{
    return MDReOpenMetaDataWithMemory(pUnk, pData, cbData);
}

// ---------------------------------------------------------------------------
// %%Function: GetAssemblyMDImport
// This function gets the IMDAssemblyImport given the filename
// ---------------------------------------------------------------------------
STDAPI GetAssemblyMDImport(             // Return code.
    LPCWSTR     szFileName,             // [in] The scope to open.
    REFIID      riid,                   // [in] The interface desired.
    IUnknown    **ppIUnk)               // [out] Return interface on success.
{
    return GetAssemblyMDInternalImport(szFileName, riid, ppIUnk);
}

// ---------------------------------------------------------------------------
// %%Function: CoInitializeCor
// 
// Parameters:
//  fFlags                  - Initialization flags for the engine.  See the
//                              COINITICOR enumerator for valid values.
// 
// Returns:
//  S_OK                    - On success
// 
// Description:
//  Reserved to initialize the Cor runtime engine explicitly.  Right now most
//  work is actually done inside the DllMain.
// ---------------------------------------------------------------------------
STDAPI          CoInitializeCor(DWORD fFlags)
{
    InterlockedIncrement(&g_cCorInitCount);
    return (S_OK);
}


// ---------------------------------------------------------------------------
// %%Function: CoUninitializeCor
// 
// Parameters:
//  none
// 
// Returns:
//  Nothing
// 
// Description:
//  Must be called by client on shut down in order to free up the system.
// ---------------------------------------------------------------------------
STDAPI_(void)   CoUninitializeCor(void)
{
    // Last one out shuts off the lights.
    if (InterlockedDecrement(&g_cCorInitCount) < 0)
    {
        //
    }
}



#ifndef ROTOR_VERSION_NUMBER
// By using (L"v1.0.0"), this should allow the us to run on the desktop version as well
#define ROTOR_VERSION_NUMBER (L"v1.0.0")
#endif

// Note: These are shim-free versions of 
//      GetCORRequiredVersion
//      GetCORVersion
//      GetCORSystemDirectory
//      LoadLibraryShim
//  for FEATURE_PAL. The real shim-aware versions are in clr\src\dlls\shim.cpp

STDAPI GetCORRequiredVersion(LPWSTR pbuffer, DWORD cchBuffer, DWORD* dwlength)
{
    return GetCORVersion(pbuffer, cchBuffer, dwlength);
}

STDAPI GetCORVersion(LPWSTR pbuffer, 
                     DWORD cchBuffer,
                     DWORD* dwlength)
{
    HRESULT hr;
    DWORD lgth = wcslen(ROTOR_VERSION_NUMBER);
    if(lgth > cchBuffer)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }
    else
    {
        wcscpy(pbuffer, ROTOR_VERSION_NUMBER);
        hr = S_OK;
    }
    *dwlength = lgth;
    return hr;
}

STDAPI GetCORSystemDirectory(LPWSTR pbuffer, 
                             DWORD  cchBuffer,
                             DWORD* dwlength)
{
    if(dwlength == NULL)
        return E_POINTER;

    if (!PAL_GetPALDirectory(pbuffer, cchBuffer)) {
        DWORD dwError = GetLastError();
        return HRESULT_FROM_WIN32(dwError);
    }

    // Include the null terminator in the length
    *dwlength = wcslen(pbuffer)+1;

    return S_OK;
}

STDAPI LoadLibraryShim(LPCWSTR szDllName, LPCWSTR szVersion, LPVOID pvReserved, HMODULE *phModDll)
{
    if (szDllName == NULL)
        return E_POINTER;

    WCHAR szDllPath[_MAX_PATH];

    if (!PAL_GetPALDirectoryW(szDllPath, _MAX_PATH)) {
        goto Win32Error;
    }
    wcsncat(szDllPath, szDllName, _MAX_PATH);
    
    if ((*phModDll = LoadLibrary(szDllPath)) == NULL)
        goto Win32Error;

    return S_OK;

Win32Error:
    DWORD dwError = GetLastError();
    return HRESULT_FROM_WIN32(dwError);
}

// Note: These are shim-free versions of 
//      GetInternalSystemDirectory
//      SetInternalSystemDirectory
//  for FEATURE_PAL. The real shim-aware versions are in clr\src\shimload\delayload.cpp

static DWORD g_dwSystemDirectory = 0;
static WCHAR g_pSystemDirectory[_MAX_PATH + 1];

HRESULT GetInternalSystemDirectory(LPWSTR buffer, DWORD* pdwLength)
{
    if (g_dwSystemDirectory == 0)
        SetInternalSystemDirectory();

    //We need to say <= for the case where the two lengths are exactly equal
    //this will leave us insufficient space to append the null.  Since some
    //of the callers (see AppDomain.cpp) assume the null, it's safest to have
    //it, even on a counted string.
    if(*pdwLength <= g_dwSystemDirectory) {
        *pdwLength = g_dwSystemDirectory;
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    wcsncpy(buffer, g_pSystemDirectory, g_dwSystemDirectory);
    *pdwLength = g_dwSystemDirectory;
    return S_OK;
}

HRESULT SetInternalSystemDirectory()
{
    HRESULT hr = S_OK;

    if(g_dwSystemDirectory == 0) {
        DWORD len;
        
        hr = GetCORSystemDirectory(g_pSystemDirectory, _MAX_PATH+1, &len);

        if(FAILED(hr)) {
            g_pSystemDirectory[0] = L'\0';
            g_dwSystemDirectory = 1;
        }
        else{
            g_dwSystemDirectory = len;
        }
    }

    return hr;
}

