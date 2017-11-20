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
//  Helium library of Naming, Binding and Manifest Services
//

#include "fusionp.h"
#include "list.h"
#include "debmacro.h"
#include "helpers.h"
#include "dbglog.h"
#include "fusionheap.h"
#include "histinfo.h"
#include "cacheutils.h"
#include "actasm.h"



FusionTag(TagDll, "Fusion", "DllMain Log");


HRESULT GetScavengerQuotasFromReg(DWORD *pdwZapQuotaInGAC,
                                  DWORD *pdwDownloadQuotaAdmin,
                                  DWORD *pdwDownloadQuotaUser);

UserAccessMode g_GAC_AccessMode = READ_ONLY;

UserAccessMode g_DownloadCache_AccessMode = READ_WRITE;

UserAccessMode g_CurrUserPermissions = READ_ONLY;

WCHAR g_UserFusionCacheDir[MAX_PATH+1];
WCHAR g_szWindowsDir[MAX_PATH+1];

// Note: g_wzEXEPath is ...\clix.exe most of the time on Rotor. 
//  Use ACTAG_APP_NAME and ACTAG_APP_BASE_URL as appropriate
WCHAR g_wzEXEPath[MAX_PATH+1];

HINSTANCE g_hInst = NULL;
LONG      g_cRef=0;

WCHAR g_FusionDllPath[MAX_PATH+1];

HSATELLITE g_hSatelliteInst = NULL;

// Clean-up function
typedef void (__stdcall *PFNFUSIONSHUTDOWNCALLBACK)(void);
PFNFUSIONSHUTDOWNCALLBACK g_pfnShutdownCallback = NULL;

HMODULE g_hMSCorEE = NULL;


CRITICAL_SECTION g_csInitClb = {0};

// Downloader critical section
CRITICAL_SECTION g_csDownload; 


// Debug log
CRITICAL_SECTION g_csBindLog;

// Max app binding history snapshots
DWORD g_dwMaxAppHistory;



extern BOOL OnUnicodeSystem(void);

BOOL InitFusionCriticalSections();


DWORD g_dwDisableLog;
DWORD g_dwLogLevel;
DWORD g_dwForceLog;
DWORD g_dwLogFailures;
DWORD g_dwLogResourceBinds;


List<CAssemblyDownload *>               *g_pDownloadList;

static DWORD GetConfigDWORD(LPCWSTR wzName, DWORD dwDefault)
{
    WCHAR wzValue[16];
    DWORD dwValue;

    if (PAL_FetchConfigurationString(TRUE, wzName, wzValue, sizeof(wzValue) / sizeof(WCHAR)))
    {
        LPWSTR pEnd;
        dwValue = wcstol(wzValue, &pEnd, 16);   // treat it has hex
        if (pEnd != wzValue)                    // success
            return dwValue;
    }

    return dwDefault;
}

//----------------------------------------------------------------------------
extern "C"
BOOL WINAPI DllMain( HINSTANCE hInst, DWORD dwReason, LPVOID pvReserved )
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            OnUnicodeSystem();
#ifdef DEBUG
            OpenLogFile();
#endif

            g_hInst = hInst;

            PAL_GetPALDirectory(g_FusionDllPath, MAX_PATH);
            StrCatBuff(g_FusionDllPath, MAKEDLLNAME_W(L"fusion"), MAX_PATH);

            FusionLog(TagDll, NULL, "+FUSION DLL_PROCESS_ATTACH");
            if (!InitFusionCriticalSections()) {
                return FALSE;
            }

            DisableThreadLibraryCalls(hInst);


            SetCurrentUserPermissions();

            GetScavengerQuotasFromReg(NULL, NULL, NULL);

            if (!GetModuleFileNameW(NULL, g_wzEXEPath, MAX_PATH)) {
                lstrcpyW(g_wzEXEPath, L"Unknown");
            }
    
            FusionLog(TagDll, NULL, "-FUSION DLL_PROCESS_ATTACH");
           
#define _GetConfigDWORD(name, default) GetConfigDWORD(name, default)

            g_dwDisableLog = _GetConfigDWORD(REG_VAL_FUSION_LOG_DISABLE, 0);
            g_dwLogLevel = _GetConfigDWORD(REG_VAL_FUSION_LOG_LEVEL, 1);
            g_dwForceLog = _GetConfigDWORD(REG_VAL_FUSION_LOG_FORCE, 0);
            g_dwLogFailures = _GetConfigDWORD(REG_VAL_FUSION_LOG_FAILURES, 0);
            g_dwLogResourceBinds = _GetConfigDWORD(REG_VAL_FUSION_LOG_RESOURCE_BINDS, 0);
            g_dwMaxAppHistory = _GetConfigDWORD(REG_VAL_FUSION_MAX_APP_HISTORY, MAX_PERSISTED_ACTIVATIONS_DEFAULT);

            

            
            g_pDownloadList = new List<CAssemblyDownload *>;
            if (!g_pDownloadList) {
                return FALSE;
            }

            break;

        case DLL_PROCESS_DETACH:
             FusionLog(TagDll, NULL, "+FUSION DLL_PROCESS_DETACH");
             

             if (g_pfnShutdownCallback)
                 (*g_pfnShutdownCallback)();

             DeleteCriticalSection(&g_csInitClb);
             DeleteCriticalSection(&g_csDownload);
             DeleteCriticalSection(&g_csBindLog);

             SAFEDELETE(g_pDownloadList);

             if (g_hSatelliteInst != NULL)
                 PAL_FreeSatelliteResource(g_hSatelliteInst);

             FusionLog(TagDll, NULL, "-FUSION DLL_PROCESS_DETACH");

             // Let's see what's left allocated...
             break;

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            ASSERT(FALSE); // we disabled these, right?
            break;
    }
    return TRUE;
}

STDAPI_(BOOL) SetFusionShutdownCallback(PFNFUSIONSHUTDOWNCALLBACK pCallback)
{
    // The shutdown handshake is modified for FEATURE_PAL 
    // to avoid GetModuleHandle calls during shutdown:
    // mscoree registers a cleanup function during initialization 
    // Fusion uses the registered cleanup function without looking
    // it up via GetModuleHandle/GetProcAddress


    if ((pCallback != NULL) && (g_pfnShutdownCallback != NULL))
    {
        _ASSERTE(false);
        return FALSE;
    }

    g_pfnShutdownCallback = pCallback;
    return TRUE;
}

BOOL InitFusionCriticalSections()
{
    BOOL fRet = FALSE;

    PAL_TRY {
        InitializeCriticalSection(&g_csInitClb);

        // downloader init
        InitializeCriticalSection(&g_csDownload);

        InitializeCriticalSection(&g_csBindLog);

        fRet = TRUE;
    }
    PAL_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
    }
    PAL_ENDTRY

    return fRet;
}

//----------------------------------------------------------------------------

STDAPI DllRegisterServer(void)
{
   return TRUE;
}


STDAPI DllUnregisterServer(void)
{
    return TRUE;
}


// ----------------------------------------------------------------------------
// DllAddRef
// ----------------------------------------------------------------------------
ULONG DllAddRef(void)
{
    return (ULONG)InterlockedIncrement(&g_cRef);
}

// ----------------------------------------------------------------------------
// DllRelease
// ----------------------------------------------------------------------------
ULONG DllRelease(void)
{
    return (ULONG)InterlockedDecrement(&g_cRef);
}
