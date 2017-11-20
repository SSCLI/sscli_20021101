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
// regutil.cpp
//
// This module contains a set of functions that can be used to access the
// registry.
//
//*****************************************************************************


#include "stdafx.h"
#include "utilcode.h"
#include "mscoree.h"

//*****************************************************************************
// Reads from the environment setting
//*****************************************************************************
static LPWSTR EnvGetString(LPCWSTR name, BOOL fPrependCOMPLUS)
{
    WCHAR buff[64];
    if(wcslen(name) > (size_t)(64 - 1 - (fPrependCOMPLUS ? 8 : 0))) 
        return(0);

    if (fPrependCOMPLUS)
        wcscpy(buff, L"COMPlus_");
    else
        *buff = 0;

    wcscat(buff, name);

    int len = WszGetEnvironmentVariable(buff, 0, 0);
    if (len == 0)
        return(0);
    LPWSTR ret = new WCHAR [len];
    WszGetEnvironmentVariable(buff, ret, len);
    return(ret);
}

//*****************************************************************************
// Reads a DWORD from the COR configuration according to the level specified
// Returns back defValue if the key cannot be found
//*****************************************************************************
DWORD REGUTIL::GetConfigDWORD(LPCWSTR name, DWORD defValue, CORConfigLevel dwFlags, BOOL fPrependCOMPLUS)
{
    DWORD rtn;
    WCHAR val[16];

    OnUnicodeSystem();

    if (dwFlags & COR_CONFIG_ENV)
    {
        WCHAR buff[64];
        _snwprintf(buff, 64, L"%s%s", fPrependCOMPLUS ? L"COMPlus_" : L"", name);

        // don't allocate memory here - this is used to initialize memory checking for FEATURE_PAL
        if (WszGetEnvironmentVariable(buff, val, sizeof(val) / sizeof(WCHAR)))
        {
            LPWSTR endPtr;
            rtn = wcstoul(val, &endPtr, 16);         // treat it has hex
            if (endPtr != val)                      // success
                return(rtn);
        }
    }

    if (dwFlags & COR_CONFIG_USER)
    {
        if (PAL_FetchConfigurationString(FALSE, name, val, sizeof(val) / sizeof(WCHAR)))
        {
            LPWSTR endPtr;
            rtn = wcstoul(val, &endPtr, 16);         // treat it has hex
            if (endPtr != val)                      // success
                return(rtn);
        }
    }

    if (dwFlags & COR_CONFIG_MACHINE)
    {
        if (PAL_FetchConfigurationString(TRUE, name, val, sizeof(val) / sizeof(WCHAR)))
        {
            LPWSTR endPtr;
            rtn = wcstoul(val, &endPtr, 16);         // treat it has hex
            if (endPtr != val)                      // success
                return(rtn);
        }
    }

    return(defValue);
}


//*****************************************************************************
// Reads a string from the COR configuration according to the level specified
// The caller is responsible for deallocating the returned string
//*****************************************************************************
LPWSTR REGUTIL::GetConfigString(LPCWSTR name, BOOL fPrependCOMPLUS, CORConfigLevel level)
{
    LPWSTR ret = NULL;

    if (level & COR_CONFIG_ENV)
    {
        ret = EnvGetString(name, fPrependCOMPLUS);  // try getting it from the environement first
        if (ret != 0) {
            if (*ret != 0) 
                return(ret);
            delete [] ret;
            ret = NULL;
        }
    }

    if (level & COR_CONFIG_USER)
    {

        // Allocate the return value
        ret = new WCHAR [_MAX_PATH];
        if (!ret)
            goto LExit;

        if (PAL_FetchConfigurationString(TRUE, name, ret, _MAX_PATH))
            goto LExit;
        
        // If it's not there, free the ret
        delete [] ret;
        ret = NULL;
    }

    if (level & COR_CONFIG_MACHINE)
    {
        // Allocate the return value
        ret = new WCHAR [_MAX_PATH];
        if (!ret)
            goto LExit;

        if (PAL_FetchConfigurationString(FALSE, name, ret, _MAX_PATH))
            goto LExit;

        // If it's not there, free the ret
        delete [] ret;
        ret = NULL;
    }

LExit:

    return(ret);
}

void REGUTIL::FreeConfigString(LPWSTR str)
{
    delete [] str;
}

//*****************************************************************************
// Reads a BIT flag from the COR configuration according to the level specified
// Returns back defValue if the key cannot be found
//*****************************************************************************
DWORD REGUTIL::GetConfigFlag(LPCWSTR name, DWORD bitToSet, BOOL defValue)
{
    return(GetConfigDWORD(name, defValue) != 0 ? bitToSet : 0);
}


