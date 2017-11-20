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
// EEConfig.H -
//
// Fetched configuration data from the registry (should we Jit, run GC checks ...)
//
//
#include "common.h"
#include <xmlparser.h>
#include <mscorcfg.h>
#include "eeconfig.h"
#include "method.hpp"
#include "wsperf.h"
#include "eeconfigfactory.h"
#include "fusionsetup.h"

#define DEFAULT_ZAP_SET L""

#define DEFAULT_APP_DOMAIN_LEAKS 0

/**************************************************************/
// Poor mans narrow
LPUTF8 NarrowWideChar(LPWSTR str) 
{
    if (str != 0) { 
        LPWSTR fromPtr = str;
        LPUTF8 toPtr = (LPUTF8) str;
        LPUTF8 result = toPtr;
        while(*fromPtr != 0)
            *toPtr++ = (char) *fromPtr++;
        *toPtr = 0;
        return result;
    }
    return NULL;
}

/**************************************************************/
// For in-place constructor
BYTE g_EEConfigMemory[sizeof(EEConfig)];

void *EEConfig::operator new(size_t size)
{
    WS_PERF_UPDATE("EEConfig", sizeof(EEConfig), g_EEConfigMemory);    
    return g_EEConfigMemory;
}

void EEConfig::operator delete(void *pMem)
{
}

/**************************************************************/
EEConfig::EEConfig()
{
    _ASSERTE(g_pConfig == 0);       // we only need one instance of this

    fInited = false;
    fEnableJit = 0;
    fEnableEJit = true;
    fEnableCodePitch = true;
   
    fCodePitchTrigger = DEFAULT_CODE_PITCH_TRIGGER;
    iMaxCodeCacheSize = INT_MAX;
    iMaxPitchOverhead = DEFAULT_MAX_PITCH_OVERHEAD;
    iTargetCodeCacheSize = DEFAULT_TARGET_CODE_CACHE_SIZE;
    iMaxUnpitchedPerThread = DEFAULT_MAX_UNPITCHED_PER_THREAD;

    iGCStress = 0;
    iGCHeapVerify = 0;          // Heap Verification OFF by default
    iGCtraceStart = INT_MAX; // Set to huge value so GCtrace is off by default
    iGCtraceEnd = INT_MAX;
    iGCprnLvl = DEFAULT_GC_PRN_LVL;
    iGCgen0size = 0;
    iGCSegmentSize = 0;
    iGCconcurrent = 0;
    iGCForceCompact = 0;
    fVerifierOff = false;
    m_fFreepZapSet = false;
    iJitOptimizeType = OPT_DEFAULT;
    fJitLooseExceptOrder = false;
    fFinalizeAllRegisteredObjects = false;
    iLogRemotingPerf = 0;
    
#ifdef _DEBUG
    fDebugInfo = false;
    fDebuggable = false;
    fRequireJit = true;
    fStressOn = false;
    pPrestubHalt = 0;
    pPrestubGC = 0;
    pszBreakOnClassLoad = 0;
    pszBreakOnClassBuild = 0;
    pszDumpOnClassLoad = 0;
    fVerifierBreakOnError  = false;
    pVerifierSkip          = NULL;
    pVerifierBreak         = NULL;
    iVerifierBreakOffset   = -1;
    iVerifierBreakPass     = -1;
    fVerifierBreakOffset   = false;
    fVerifierBreakPass     = false;
    fVerifierMsgMethodInfoOff = false;
    fDoAllowUntrustedCallerChecks = true;
    m_dwSecurityOptThreshold = 0;
    iPerfNumAllocsThreshold = 0;
    iPerfAllocsSizeThreshold = 0;
    pPerfTypesToLog = NULL;
    iFastGCStress = 0;
    fJitVerificationDisable = false;
#endif

#ifdef _X86_
    dwCpuFlag  = 0;
    dwCpuCapabilities = 0;
#endif

#ifdef STRESS_COMCALL
    dwStressComCall = 0; 
#endif

#ifdef _DEBUG
    fBuiltInLoader = false;
    m_fAssertOnBadImageFormat = true;
#endif

    fUseZaps = true;
    fRequireZaps = false;
    fVersionZapsByTimestamp = true;
    fLogMissingZaps = false;

    m_fDeveloperInstallation = false;

    pZapSet = DEFAULT_ZAP_SET;

    dwSharePolicy = BaseDomain::SHARE_POLICY_UNSPECIFIED;

    dwMonitorZapStartup = 0;
    dwMonitorZapExecution = 0;

    dwMetaDataPageNumber = 0xFFFFFFFF;
    fShowMetaDataAccess = 0;
    szMetaDataFileName = L"";

    fRecordLoadOrder = false;

    fAppDomainUnload = true;

#ifdef _DEBUG
    fAppDomainLeaks = DEFAULT_APP_DOMAIN_LEAKS;
#endif

    // interop logging
    m_pTraceIUnknown = NULL;
    m_TraceWrapper = 0;

    fContinueAfterFatalError = FALSE;

    SetupConfiguration();
}

/**************************************************************/
EEConfig::~EEConfig()
{

#ifdef _DEBUG
    if(g_pConfig) {
        LPWSTR setting = NULL;
        if(SUCCEEDED(g_pConfig->GetConfiguration(L"DumpConfiguration", CONFIG_SYSTEM, & setting)) &&
           setting != NULL) {
            ConfigList::ConfigIter iter(&m_Configuration);
            int count = 0;
            for(EEUnicodeStringHashTable* table = iter.Next();table; table = iter.Next()) {
                EEHashTableIteration tableIter;
                table->IterateStart(&tableIter);
                LOG((LF_ALWAYS, LL_ALWAYS, "\nConfiguration Table %d\n", count++));
                LOG((LF_ALWAYS, LL_ALWAYS, "*********************************\n"));
                while(table->IterateNext(&tableIter)) {
                    EEStringData* key = table->IterateGetKey(&tableIter);
                    LPCWSTR keyString = key->GetStringBuffer();
                    LPCWSTR data = (LPCWSTR) table->IterateGetValue(&tableIter);
                    LOG((LF_ALWAYS, LL_ALWAYS, "%S = %S\n", keyString, data));
                }
                LOG((LF_ALWAYS, LL_ALWAYS, "\n"));
            }
        }
    }
#endif

    if (m_fFreepZapSet)
        delete[] pZapSet;

#ifdef _DEBUG
    DestroyMethList(pPrestubHalt);
    DestroyMethList(pPrestubGC);
    delete [] pszBreakOnClassLoad;
    delete [] pszBreakOnClassBuild;
    delete [] pszDumpOnClassLoad;
    delete [] pszGcCoverageOnMethod;
#endif
#ifdef _DEBUG
    DestroyMethList(pVerifierSkip);
    DestroyMethList(pVerifierBreak);
    DestroyTypeList(pPerfTypesToLog);
#endif
}

/**************************************************************/

LPWSTR EEConfig::GetConfigString(LPWSTR name, BOOL fPrependCOMPLUS, ConfigSearch direction)
{
    LPWSTR pvalue = REGUTIL::GetConfigString(name, fPrependCOMPLUS); 
    if(pvalue == NULL && g_pConfig) {
        LPWSTR pResult;
        if(SUCCEEDED(g_pConfig->GetConfiguration(name, direction, &pResult)) && pResult != NULL) {
            pvalue = new WCHAR[wcslen(pResult) + 1];
            wcscpy(pvalue,pResult);
        }
    }
    return pvalue;
}

DWORD EEConfig::GetConfigDWORD(LPCWSTR name, DWORD defValue, DWORD level, BOOL fPrependCOMPLUS, ConfigSearch direction)
{    
    DWORD result = REGUTIL::GetConfigDWORD(name, defValue, (REGUTIL::CORConfigLevel)level, fPrependCOMPLUS); 
    if(result == defValue && g_pConfig) {
        LPWSTR pvalue;
        if(SUCCEEDED(g_pConfig->GetConfiguration(name, direction, &pvalue))) {
            if(pvalue) {
                MAKE_UTF8PTR_FROMWIDE(pstr, pvalue);
                result = atoi(pstr);
            }
        }
    }
    return result;
}

DWORD EEConfig::GetConfigDWORDInternal(LPWSTR name, DWORD defValue, DWORD level, BOOL fPrependCOMPLUS, ConfigSearch direction)
{    
    DWORD result = REGUTIL::GetConfigDWORD(name, defValue, (REGUTIL::CORConfigLevel)level, fPrependCOMPLUS); 
    if(result == defValue) {
        LPWSTR pvalue;
        if(SUCCEEDED(GetConfiguration(name, direction, &pvalue))) {
            if(pvalue) {
                MAKE_UTF8PTR_FROMWIDE(pstr, pvalue);
                result = atoi(pstr);
            }
        }
    }
    return result;
}

DWORD EEConfig::GetConfigFlag(LPWSTR name, DWORD bitToSet, bool defValue)
{ return REGUTIL::GetConfigFlag(name, bitToSet, defValue); }

/**************************************************************/

void EEConfig::sync()
{
    // Note the global variable is not updated directly by the GetRegKey function
    // so we only update it one (to avoid reentrancy windows)



    fEnableJit          = GetConfigDWORD(L"JitEnable", fEnableJit);
    // if JIT is disabled, by default don't use zaps either (since they were made by the JIT
    fUseZaps = (fEnableJit != 0);
    // Enable code pitching by default
    fEnableCodePitch    = (GetConfigDWORD(L"CodePitchEnable", fEnableCodePitch) != 0);

#ifdef _DEBUG
    iMaxCodeCacheSize = GetConfigDWORD(L"MaxCodeCacheSize",iMaxCodeCacheSize);
    iMaxCodeCacheSize = RoundToPageSize(iMaxCodeCacheSize);
    if (iMaxCodeCacheSize < MINIMUM_CODE_CACHE_SIZE)
        iMaxCodeCacheSize = MINIMUM_CODE_CACHE_SIZE;
    iMaxUnpitchedPerThread = GetConfigDWORD(L"MaxUnpitchedPerThread",iMaxUnpitchedPerThread);
    iTargetCodeCacheSize   = GetConfigDWORD(L"TargetCodeCacheSize",iTargetCodeCacheSize);
    if (!fEnableCodePitch) {
        iMaxPitchOverhead = 0;
        iTargetCodeCacheSize = 64 * 1024 * 1024;
        if (iMaxCodeCacheSize < iTargetCodeCacheSize) 
            iMaxCodeCacheSize = iTargetCodeCacheSize;
    }
    else
    {
        fCodePitchTrigger    = GetConfigDWORD(L"CodePitchTrigger", fCodePitchTrigger);
    }
    fEnableEJit         = (GetConfigDWORD(L"EJitEnable", fEnableEJit) != 0);

    iTargetCodeCacheSize = min(max(iTargetCodeCacheSize,MINIMUM_CODE_CACHE_SIZE), iMaxCodeCacheSize);
    iMaxPitchOverhead   = GetConfigDWORD(L"MaxPitchOverhead",iMaxPitchOverhead);
#endif

#ifdef STRESS_HEAP
    iGCStress           =  GetConfigDWORD(L"GCStress", iGCStress );
    if (iGCStress)
        g_IGCconcurrent = 0;        // by default GCStress turns off concurrent GC since it make objects move less

    iGCHeapVerify       =  GetConfigDWORD(L"HeapVerify", iGCHeapVerify);
#endif //STRESS_HEAP

#ifdef GC_SIZE
    if (!iGCSegmentSize) iGCSegmentSize =  GetConfigDWORD(L"GCSegmentSize", iGCSegmentSize);
    if (!iGCgen0size) iGCgen0size = GetConfigDWORD(L"GCgen0size"  , iGCgen0size);
#endif //GC_SIZE

#ifdef _DEBUG
    iGCtraceStart       =  GetConfigDWORD(L"GCtraceStart", iGCtraceStart);
    iGCtraceEnd         =  GetConfigDWORD(L"GCtraceEnd"  , iGCtraceEnd);
    iGCprnLvl           =  GetConfigDWORD(L"GCprnLvl"    , iGCprnLvl);
#endif

    iGCconcurrent       =  GetConfigDWORD(L"gcConcurrent"  , g_IGCconcurrent);
    iGCForceCompact     =  GetConfigDWORD(L"gcForceCompact"  , iGCForceCompact);

#ifdef _DEBUG
    iFastGCStress       = GetConfigDWORD(L"FastGCStress", iFastGCStress);
    pszGcCoverageOnMethod = NarrowWideChar(GetConfigString(L"GcCoverage"));
#endif
    
    m_dwStressLoadThreadCount = GetConfigDWORD(L"StressLoadThreadCount", m_dwStressLoadThreadCount);
#ifdef STRESS_THREAD
    dwStressThreadCount =  GetConfigDWORD(L"StressThreadCount", dwStressThreadCount);
#endif
    
#ifdef STRESS_COMCALL
    dwStressComCall = GetConfigDWORD(L"STRESSCOMCALL", dwStressComCall);
    if (dwStressComCall) {
        iGCForceCompact = 1;
    }
#endif
    
    fUseZaps            = !(GetConfigDWORD(L"ZapDisable", !fUseZaps) != 0);

    fRequireZaps        = (GetConfigDWORD(L"ZapRequire", fRequireZaps) != 0);
    fVersionZapsByTimestamp = (GetConfigDWORD(L"ZapsVersionByTimestamp", fVersionZapsByTimestamp) != 0);

    pZapSet             = GetConfigString(L"ZapSet");



    dwSharePolicy           = GetConfigDWORD(L"LoaderOptimization", dwSharePolicy);

    fRecordLoadOrder        = GetConfigDWORD(L"ZapRecordLoadOrder", fRecordLoadOrder) != 0;

    m_fFreepZapSet = true;
    
    if (pZapSet == NULL)
    {
        m_fFreepZapSet = false;
        pZapSet = L"";
    }
    if (wcslen(pZapSet) > 3)
    {
        _ASSERTE(!"Zap Set String must be less than 3 chars");
        delete[] pZapSet;
        m_fFreepZapSet = false;
        pZapSet = L"";
    }

    fShowMetaDataAccess     = GetConfigDWORD(L"ShowMetadataAccess", fShowMetaDataAccess);

    iLogRemotingPerf = GetConfigDWORD(L"LogRemotingPerf", 0);

#ifdef BREAK_ON_CLSLOAD
    pszBreakOnClassLoad = NarrowWideChar(GetConfigString(L"BreakOnClassLoad"));
#endif

#ifdef _DEBUG
    iJitOptimizeType      =  GetConfigDWORD(L"JitOptimizeType",     iJitOptimizeType);
    if (iJitOptimizeType > OPT_RANDOM)     iJitOptimizeType = OPT_DEFAULT;
    fJitLooseExceptOrder  = (GetConfigDWORD(L"JitLooseExceptOrder", fJitLooseExceptOrder) != 0);

    fDebugInfo          = (GetConfigDWORD(L"JitDebugInfo",       fDebugInfo)          != 0);
    fDebuggable         = (GetConfigDWORD(L"JitDebuggable",      fDebuggable)         != 0);
    fRequireJit         = (GetConfigDWORD(L"JitRequired",        fRequireJit)         != 0);
    fStressOn           = (GetConfigDWORD(L"StressOn",           fStressOn)           != 0);

    pPrestubHalt = ParseMethList(GetConfigString(L"PrestubHalt"));
    pPrestubGC = ParseMethList(GetConfigString(L"PrestubGC"));

    pszBreakOnClassBuild = NarrowWideChar(GetConfigString(L"BreakOnClassBuild"));
    pszDumpOnClassLoad = NarrowWideChar(GetConfigString(L"DumpOnClassLoad"));
    
    m_dwSecurityOptThreshold = GetConfigDWORD(L"SecurityOptThreshold", m_dwSecurityOptThreshold);

    fBuiltInLoader = (GetConfigDWORD(L"BuiltInLoader", fBuiltInLoader) != 0);

    m_fAssertOnBadImageFormat = (GetConfigDWORD(L"AssertOnBadImageFormat", m_fAssertOnBadImageFormat) != 0);
   
    fJitVerificationDisable = (GetConfigDWORD(L"JitVerificationDisable", fJitVerificationDisable)         != 0);
#endif


    fFinalizeAllRegisteredObjects  = (GetConfigDWORD(L"FinalizeAllRegisteredObjects", fFinalizeAllRegisteredObjects) != 0);

    if(g_pConfig) {
        LPWSTR result = NULL;
        if(SUCCEEDED(g_pConfig->GetConfiguration(L"developerInstallation", CONFIG_SYSTEM, &result)) && result)
        {
            if(_wcsicmp(result, L"true") == 0)
                m_fDeveloperInstallation = true;
        }
    }

#ifdef AD_NO_UNLOAD
    fAppDomainUnload = (GetConfigDWORD(L"AppDomainNoUnload", 0) == 0);
#endif

#ifdef BUILDENV_CHECKED
    fAppDomainLeaks = GetConfigDWORD(L"AppDomainAgilityChecked", DEFAULT_APP_DOMAIN_LEAKS) == 1;
#elif defined _DEBUG
    fAppDomainLeaks = GetConfigDWORD(L"AppDomainAgilityFastChecked", DEFAULT_APP_DOMAIN_LEAKS) == 1;
#endif

#ifdef _DEBUG
    fVerifierBreakOnError =  
        (GetConfigDWORD(L"VerBreakOnError", (DWORD) -1) == 1);

    pVerifierSkip        =  ParseMethList(GetConfigString(L"VerSkip"));
    pVerifierBreak       =  ParseMethList(GetConfigString(L"VerBreak"));
    iVerifierBreakOffset =  GetConfigDWORD(L"VerOffset", iVerifierBreakOffset);
    iVerifierBreakPass   =  GetConfigDWORD(L"VerPass",   iVerifierBreakPass);
    if (iVerifierBreakOffset != -1)
        fVerifierBreakOffset = true;
    if (iVerifierBreakPass != -1)
        fVerifierBreakPass = true;

    fVerifierMsgMethodInfoOff =  
        (GetConfigDWORD(L"VerMsgMethodInfoOff", (DWORD) -1) == 1);

    fDoAllowUntrustedCallerChecks =  
        (GetConfigDWORD(L"SupressAllowUntrustedCallerChecks", 0) != 1);

#endif
    fInited = true;

#ifdef _DEBUG
    fVerifierOff    = (GetConfigDWORD(L"VerifierOff", fVerifierOff) != 0);
#endif

    m_pTraceIUnknown = (IUnknown*)(DWORD_PTR)(GetConfigDWORD(L"TraceIUnknown", (DWORD)(DWORD_PTR)(m_pTraceIUnknown)));
    m_TraceWrapper = GetConfigDWORD(L"TraceWrap", m_TraceWrapper);

    // can't have both
    if (m_pTraceIUnknown != 0)
    {
        m_TraceWrapper = 0;
    }
    else
    if (m_TraceWrapper != 0)
    {
        m_pTraceIUnknown = (IUnknown*)-1;
    }

#ifdef _DEBUG

    pPerfTypesToLog = ParseTypeList(GetConfigString(L"PerfTypesToLog"));
    iPerfNumAllocsThreshold = GetConfigDWORD(L"PerfNumAllocsThreshold", 0x3FFFFFFF);
    iPerfAllocsSizeThreshold    = GetConfigDWORD(L"PerfAllocsSizeThreshold", 0x3FFFFFFF);

#endif //_DEBUG

    fContinueAfterFatalError = GetConfigDWORD(L"ContinueAfterFatalError", 0);
}

/**************************************************************/
HRESULT EEConfig::SetupConfiguration()
{
    // Get the system configuration file
    WCHAR systemDir[_MAX_PATH+9];
    DWORD dwSize = _MAX_PATH;
    static const WCHAR configFile[] = MACHINE_CONFIGURATION_FILE;
    WCHAR version[_MAX_PATH];
    DWORD dwVersion = _MAX_PATH;

    HRESULT hr;
    // Get the version location
    hr= GetCORVersion(version, _MAX_PATH, & dwVersion);
    if(FAILED(hr)) return hr;


    // See if the environment has specified an XML file
    LPWSTR file = REGUTIL::GetConfigString(L"CONFIG", TRUE);
    if(file != NULL) {
        hr = AppendConfigurationFile(file, version);
        delete [] file;
    }

    if (FAILED(hr)&&!(REGUTIL::GetConfigDWORD(L"NoGuiFromShim", FALSE)))
    {
        WCHAR* wszMessage=(WCHAR*)alloca(2*wcslen(file)+100);
        swprintf(wszMessage,L"Error parsing %s\nParser returned error 0x%08X",file,hr);
        WszMessageBoxInternal(NULL,wszMessage,L"Config parser error",MB_OK);
    }

    // Size includes the null terminator
    hr = GetInternalSystemDirectory(systemDir, &dwSize);
    if(SUCCEEDED(hr)) {
        DWORD configSize = sizeof(configFile) / sizeof(WCHAR) - 1;
        if(configSize + dwSize <= _MAX_PATH) {
            wcscat(systemDir, configFile);
            hr = AppendConfigurationFile(systemDir, version);
            if(hr == S_FALSE) {
                hr = GetInternalSystemDirectory(systemDir, &dwSize);
                wcscpy((systemDir+dwSize-1), configFile);
                hr = AppendConfigurationFile(systemDir, version);
            }               
            if (FAILED(hr)&&!(REGUTIL::GetConfigDWORD(L"NoGuiFromShim", FALSE)))
            {
                WCHAR* wszMessage=(WCHAR*)alloca(2*wcslen(systemDir)+100);
                swprintf(wszMessage,L"Error parsing %s\nParser returned error 0x%08X",systemDir,hr);
                WszMessageBoxInternal(NULL,wszMessage,L"Config parser error",MB_OK);
            }
        }
    }
        
#ifdef _DEBUG
    hr = S_OK;
    int len = WszGetModuleFileName(NULL, systemDir, _MAX_PATH); // get name of file used to create process
    if (len) {
        wcscat(systemDir, L".config\0");
        hr = AppendConfigurationFile(systemDir, version);
        if (FAILED(hr)&&!(REGUTIL::GetConfigDWORD(L"NoGuiFromShim", FALSE))&&
            GetConfigDWORDInternal(L"NotifyBadAppCfg",false))
        {
            
            WCHAR* wszMessage=(WCHAR*)alloca(2*wcslen(systemDir)+100);
            swprintf(wszMessage,L"Error parsing %s\nParser returned error 0x%08X",systemDir,hr);
            WszMessageBoxInternal(NULL,wszMessage,L"Config parser error",MB_OK);
        }

        hr=S_OK;
    }
#endif

    return hr;
}

HRESULT EEConfig::GetConfiguration(LPCWSTR pKey, ConfigSearch direction, LPWSTR* pValue)
{
    _ASSERTE(pValue);
    _ASSERTE(pKey);

    *pValue = NULL;
    ConfigList::ConfigIter iter(&m_Configuration);
    EEStringData sKey((DWORD)wcslen(pKey)+1,pKey);
    HashDatum datum;

    switch(direction) {
    case CONFIG_SYSTEM: {
        for(EEUnicodeStringHashTable* table = iter.Next();table; table = iter.Next()) {
            if(table->GetValue(&sKey, &datum)) {
                *pValue = (LPWSTR) datum;
                    return S_OK;
            }
        }
    }
    case CONFIG_APPLICATION: {
        for(EEUnicodeStringHashTable* table = iter.Previous();table != NULL; table = iter.Previous()) {
            if(table->GetValue(&sKey, &datum)) {
                *pValue = (LPWSTR) datum;
                return S_OK;
            }
        }
    }
    default:
        return E_FAIL;
    }
}        

LPCWSTR EEConfig::GetProcessBindingFile()
{
    return g_pszHostConfigFile;
}

/**************************************************************/
bool EEConfig::ShouldJitMethod(MethodDesc* pMethodInfo) const
{
    if (!fInited)
        const_cast<EEConfig*>(this)->sync();

      /* If Jit disabled check the Include list to see if we have to jit it conditionally */
    return fEnableJit != 0;
}

/**************************************************************/
unsigned EEConfig::RoundToPageSize(unsigned size)
{
    unsigned adjustedSize = (size + PAGE_SIZE-1)/PAGE_SIZE * PAGE_SIZE;
    if (adjustedSize < size)       // check for overflow
    {
        adjustedSize = (size/PAGE_SIZE)*PAGE_SIZE;
    }
    return adjustedSize;
}


HRESULT EEConfig::AppendConfigurationFile(LPCWSTR pszFileName, LPCWSTR version)
{
    EEUnicodeStringHashTable* pTable = m_Configuration.Append();
    if(pTable == NULL) return NULL;

    IXMLParser     *pIXMLParser = NULL;
    IStream        *pFile = NULL;
    EEConfigFactory *factory = NULL; 

    HRESULT hr = CreateConfigStream(pszFileName, &pFile);
    if(FAILED(hr)) goto Exit;
        
    hr = GetXMLObject(&pIXMLParser);
    if(FAILED(hr)) goto Exit;

    factory = new EEConfigFactory(pTable, version);
    if ( ! factory) { 

        hr = E_OUTOFMEMORY; 
        goto Exit; 
    }
    factory->AddRef(); // RefCount = 1 

    
    hr = pIXMLParser->SetInput(pFile); // filestream's RefCount=2
    if ( ! SUCCEEDED(hr)) 
        goto Exit;

    hr = pIXMLParser->SetFactory(factory); // factory's RefCount=2
    if ( ! SUCCEEDED(hr)) 
        goto Exit;

    hr = pIXMLParser->Run(-1);
    
Exit:  
    if (hr==(HRESULT) XML_E_MISSINGROOT)
        hr=S_OK;

    if (hr==HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        hr=S_FALSE;

    if (pIXMLParser) { 
        pIXMLParser->Release();
        pIXMLParser= NULL ; 
    }
    if ( factory) {
        factory->Release();
        factory=NULL;
    }
    if ( pFile) {
        pFile->Release();
        pFile=NULL;
    }
    return hr;
}


#ifdef _DEBUG

/**************************************************************/
// Ownership of the string buffer passes to ParseMethList
/* static */
MethodNamesList* EEConfig::ParseMethList(LPWSTR str) {
        // we are now done with the string passed in
    if (str == NULL)
        return NULL;

    MethodNamesList* ret = NULL;
    ret = new MethodNamesList(str);
    delete [] str;
    return ret;
}

/**************************************************************/
/* static */
void EEConfig::DestroyMethList(MethodNamesList* list) {
    if (list == 0)
        return;
    delete list;
}

/**************************************************************/
/* static */
bool EEConfig::IsInMethList(MethodNamesList* list, MethodDesc* pMD) {

    DefineFullyQualifiedNameForClass();
    if (list == 0)
        return(false);

    LPCUTF8 name = pMD->GetName();
    LPCUTF8 className = pMD->GetClass() ? GetFullyQualifiedNameForClass(pMD->GetClass()) : "";
    PCCOR_SIGNATURE sig = pMD->GetSig();

    return list->IsInList(name, className, sig);
}

TypeNamesList *EEConfig::ParseTypeList(LPWSTR str)
{
    if (str == NULL)
        return NULL;

    TypeNamesList *ret = NULL;

    ret = new TypeNamesList(str);
    delete [] str;

    return ret;
}

void EEConfig::DestroyTypeList(TypeNamesList* list) {
    if (list == 0)
        return;
    delete list;
}

TypeNamesList::TypeNamesList(LPWSTR str)
{
    pNames = NULL;

    LPWSTR currentType = str;
    int length = 0;
    bool typeFound = false;

    for (; *str != '\0'; str++)
    {
        switch(*str)
        {
        case ' ':
            {
                if (!typeFound)
                    break;

                TypeName *tn = new TypeName();
                tn->typeName = new char[length + 1];
                MAKE_UTF8PTR_FROMWIDE(temp, currentType);
                memcpy(tn->typeName, temp, length * sizeof(char));
                tn->typeName[length] = '\0';

                tn->next = pNames;
                pNames = tn;

                typeFound = false;
                length = 0;

                break;
            }

        default:
            if (!typeFound)
                currentType = str;

            typeFound = true;
            length++;
            break;
        }
    }

    if (typeFound)
    {
        TypeName *tn = new TypeName();
        tn->typeName = new char[length + 1];
        MAKE_UTF8PTR_FROMWIDE(temp, currentType);
        memcpy(tn->typeName, temp, length * sizeof(char));
        tn->typeName[length] = '\0';

        tn->next = pNames;
        pNames = tn;
    }
}

TypeNamesList::~TypeNamesList()
{

    while (pNames)
    {
        delete [] pNames->typeName;

        TypeName *tmp = pNames;
        pNames = pNames->next;

        delete tmp;
    }
}

bool TypeNamesList::IsInList(LPCUTF8 typeName)
{

    TypeName *tnTemp = pNames;
    while (tnTemp)
    {
        if (strstr(typeName, tnTemp->typeName) != typeName)
            tnTemp = tnTemp->next;
        else
            return true;
    }

    return false;
}

#endif // _DEBUG


