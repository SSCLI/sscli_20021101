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
// ===========================================================================
// File: cocreate.cpp
// 
// ===========================================================================
/*++

Abstract:

    Fake CoCreateInstance for PAL RT

Author:


Revision History:

--*/

#include "rotor_palrt.h"
#include "palclr.h"

#include "unknwn.h"

static const CLSID _CLSID_CorRuntimeHost = 
    { 0xCB2F6723, 0xAB3A, 0x11d2, {0x9C, 0x40, 0x00, 0xC0, 0x4F, 0xA3, 0x0A, 0x3E} };

static const CLSID _CLSID_CorMetaDataDispenser =
    { 0xE5CB7A31, 0x7512, 0x11D2, {0x89, 0xCE, 0x00, 0x80, 0xC7, 0x92, 0xE5, 0xD8} };

static const CLSID _CLSID_CorMetaDataDispenserRuntime =
    { 0x1EC2DE53, 0x75CC, 0x11D2, {0x97, 0x75, 0x00, 0xA0, 0xC9, 0xB4, 0xD5, 0x0C} };

static const CLSID _CLSID_CorSymWriter =
    { 0x108296C1, 0x281E, 0x11D3, {0xBD, 0x22, 0x00, 0x00, 0xF8, 0x08, 0x49, 0xBD} };

static const CLSID _CLSID_CorSymReader =
    { 0x108296C2, 0x281E, 0x11d3, {0xBD, 0x22, 0x00, 0x00, 0xF8, 0x08, 0x49, 0xBD} };

static const CLSID _CLSID_CorSymBinder =
    { 0xAA544D41, 0x28CB, 0x11d3, {0xBD, 0x22, 0x00, 0x00, 0xF8, 0x08, 0x49, 0xBD} };

static const CLSID _CLSID_CorDebug =
    { 0x6fef44d0, 0x39e7, 0x4c77, {0xbe, 0x8e, 0xc9, 0xf8, 0xcf, 0x98, 0x86, 0x30} };

static const CLSID _CLSID_CorpubPublish =
    { 0x047a9a40, 0x657e, 0x11d3, {0x8d, 0x5b, 0x00, 0x10, 0x4b, 0x35, 0xe7, 0xef} };

struct CoClass
{
    const CLSID *pClsid;
    LPCWSTR     dllName;
};

#define COCLASS(name, dll) { &_CLSID_##name, MAKEDLLNAME_W(dll) },

static const struct CoClass g_CoClasses[] =
{
    COCLASS(CorRuntimeHost,              L"sscoree")
    COCLASS(CorMetaDataDispenser,        L"sscoree")
    COCLASS(CorMetaDataDispenserRuntime, L"sscoree")
    COCLASS(CorSymWriter,                L"ildbsymbols")
    COCLASS(CorSymReader,                L"ildbsymbols")
    COCLASS(CorSymBinder,                L"ildbsymbols")
    COCLASS(CorDebug,                    L"mscordbi")
    COCLASS(CorpubPublish,               L"mscordbi")
};

typedef HRESULT __stdcall DLLGETCLASSOBJECT(REFCLSID rclsid,
                                            REFIID   riid,
                                            void   **ppv);

HRESULT PALAPI PAL_CoCreateInstance(REFCLSID   rclsid,
                             REFIID     riid,
                             void     **ppv)
{
    for (size_t i = 0; i < sizeof(g_CoClasses) / sizeof(g_CoClasses[0]); i++)
    {
        const CoClass *pCoClass = &g_CoClasses[i];

        if (*pCoClass->pClsid == rclsid)
        {
            HRESULT             hr;
            HINSTANCE           dll;
            DLLGETCLASSOBJECT   *dllGetClassObject;
            IClassFactory       *classFactory;
            WCHAR FullPath[_MAX_PATH];

            if (!PAL_GetPALDirectoryW(FullPath, _MAX_PATH)) {
                goto Win32Error;
            }
            wcsncat(FullPath, pCoClass->dllName, _MAX_PATH);
            
            dll = LoadLibraryW(FullPath);
            if (dll == NULL)
                goto Win32Error;

            dllGetClassObject = (DLLGETCLASSOBJECT*)GetProcAddress(dll, "DllGetClassObject");
            if (dllGetClassObject == NULL) {
                // The CLR shim exports a DllGetClassObject which in turn decides which DLL to load and
                // call DllGetClassObjectInternal on.  Without the shim, the PALRT must do the same
                // here.
                dllGetClassObject = (DLLGETCLASSOBJECT*)GetProcAddress(dll, "DllGetClassObjectInternal");
                if (dllGetClassObject == NULL) {
                    goto Win32Error;
                }
            }

            hr = (*dllGetClassObject)(rclsid, IID_IClassFactory, (void**)&classFactory);
            if (FAILED(hr))
                return hr;

            hr = classFactory->CreateInstance(NULL, riid, ppv);
            classFactory->Release();
            return hr;
        }
    }

    _ASSERTE(!"Unknown CLSID in PAL_CoCreateInstance");
    return CLASS_E_CLASSNOTAVAILABLE;

Win32Error:
    DWORD dwError = GetLastError();
    return HRESULT_FROM_WIN32(dwError);
}
