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
// File: alinklib.cpp
//
// Implementation of DLL Exports.
// ===========================================================================

// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f alinkps.mk in the project directory.

#include "stdafx.h"
#include "asmlink.h"

static HINSTANCE hModuleMessages = NULL;


#include "rotor_pal.h"
// name of the message DLL.
#define MESSAGE_DLL L"alink.satellite"


// List of fallback langids we try (in order) if we can't find the messages DLL by normal means.
// These are just a bunch of common languages -- not necessarily all languages we localize to.
// This list should never be used in usual course of things -- it's just an "emergency" fallback.
const LANGID g_fallbackLangs[] = {
    MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
    MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
    MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_UK),
    MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_AUS),
    MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_CAN),
//    MAKELANGID(LANG_CHINESE, SUBLANG_DEFAULT), // Same as below
    MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL),
    MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED),
    MAKELANGID(LANG_FRENCH, SUBLANG_DEFAULT),
    MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH),
    MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH_CANADIAN),
//    MAKELANGID(LANG_GERMAN, SUBLANG_DEFAULT), // Same as below
    MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN),
    MAKELANGID(LANG_GREEK, SUBLANG_DEFAULT),
    MAKELANGID(LANG_HEBREW, SUBLANG_DEFAULT),
//    MAKELANGID(LANG_ITALIAN, SUBLANG_DEFAULT), // Same as below
    MAKELANGID(LANG_ITALIAN, SUBLANG_ITALIAN),
    MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT),
//    MAKELANGID(LANG_KOREAN, SUBLANG_DEFAULT), // Same as below
    MAKELANGID(LANG_KOREAN, SUBLANG_KOREAN),
//    MAKELANGID(LANG_PORTUGUESE, SUBLANG_DEFAULT), // Same as below
    MAKELANGID(LANG_PORTUGUESE, SUBLANG_PORTUGUESE),
    MAKELANGID(LANG_PORTUGUESE, SUBLANG_PORTUGUESE_BRAZILIAN),
    MAKELANGID(LANG_RUSSIAN, SUBLANG_DEFAULT),
//    MAKELANGID(LANG_SPANISH, SUBLANG_DEFAULT), // Same as below
    MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH),
    MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH_MEXICAN),
    MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH_MODERN),
};


STDAPI DllCanUnloadNow(void) { return E_UNEXPECTED; }
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv) { return E_UNEXPECTED; }
STDAPI DllRegisterServer(void) { return E_UNEXPECTED; }
STDAPI DllUnregisterServer(void) { return E_UNEXPECTED; }

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/) 
{ 
    static HMODULE hMod = NULL;
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        hMod = PAL_RegisterLibraryW(L"sscoree");
        if (hMod == NULL)
        {
            return FALSE;
        }
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        if (hMod)
        {
            PAL_UnregisterLibraryW(hMod);
        }

        if (hModuleMessages)
        {
            PAL_FreeSatelliteResource(hModuleMessages);
            hModuleMessages = NULL;
        }
    }
    return TRUE; 
}


HRESULT WINAPI CreateALink(REFIID riid, IUnknown** ppInterface)
{
    HRESULT                 hr;
    CComObject<CAsmLink>    *pObj;

    if (SUCCEEDED (hr = CComObject<CAsmLink>::CreateInstance (&pObj)))
    {
        if (FAILED (hr = pObj->QueryInterface (riid, (void **)ppInterface)))
        {
            delete pObj;
        }
    }

    return hr;
}

// Try to load a message DLL from a location, in a subdirectory of mine given by the LANGID
// passed in (or the same directory if -1). 
static HINSTANCE FindMessageDll(LANGID langid)
{

    WCHAR path[MAX_PATH];
    WCHAR * pEnd;

    if (!PAL_GetPALDirectoryW(path, lengthof(path)))
        return 0;

    pEnd = path + wcslen(path);

    // Append language ID
    if (langid != (LANGID)-1) {
        pEnd += _snwprintf(pEnd, lengthof(path) - (pEnd - path) - 1, L"%d\\", langid);
    }

    // Append message DLL name.
    if ((int) lengthof(MESSAGE_DLL) + pEnd - path > (int) lengthof(path) - 1)
        return 0;
    wcscpy(pEnd, MESSAGE_DLL);

    // Try to load it.
    return PAL_LoadSatelliteResourceW(path);

}

////////////////////////////////////////////////////////////////////////////////
// GetMesageDll -- find and load the message DLL. Returns 0 if the message
// DLL could not be located or loaded. The message DLL should be either in a 
// subdirectory whose name is a language id, or in the current directory. We have
// a complex set of rules to figure out which language ids to try.
HINSTANCE WINAPI GetALinkMessageDll ()
{
    LANGID langid;

    if (! hModuleMessages) {
        HINSTANCE hModuleMessagesLocal = 0;


        // Next try current locale.
        if (!hModuleMessagesLocal) {
            langid = LANGIDFROMLCID(GetThreadLocale());
            hModuleMessagesLocal = FindMessageDll(langid);
            if (!hModuleMessagesLocal)
                hModuleMessagesLocal = FindMessageDll(MAKELANGID(PRIMARYLANGID(langid), SUBLANG_DEFAULT));
        }

        // Next try user and system locale.
        if (!hModuleMessagesLocal) {
            langid = GetUserDefaultLangID();
            hModuleMessagesLocal = FindMessageDll(langid);
        }
        if (!hModuleMessagesLocal) {
            langid = GetSystemDefaultLangID();
            hModuleMessagesLocal = FindMessageDll(langid);
        }

        // Next try current directory.
        if (!hModuleMessagesLocal) 
            hModuleMessagesLocal = FindMessageDll((LANGID)-1);

        // Try a fall-back list of locales.
        if (!hModuleMessagesLocal) {
            for (unsigned int i = 0; i < lengthof(g_fallbackLangs); ++i) {
                langid = g_fallbackLangs[i];
                hModuleMessagesLocal = FindMessageDll(langid);
                if (hModuleMessagesLocal)
                    break;
            }
        }

        if (hModuleMessagesLocal)
        {
            if (InterlockedCompareExchangePointer( (void**)&hModuleMessages, hModuleMessagesLocal, NULL)) {
                ASSERT(hModuleMessages != hModuleMessagesLocal);
                PAL_FreeSatelliteResource(hModuleMessagesLocal);
            } else
                ASSERT(hModuleMessagesLocal == hModuleMessages);
        }

    }
    return hModuleMessages;
}
