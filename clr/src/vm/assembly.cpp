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
/*============================================================
**
** Header: Assembly.cpp
**
** Purpose: Implements assembly (loader domain) architecture
**
** Date:  Dec 1, 1998
**
===========================================================*/

#include "common.h"

#include <stdlib.h>

#include "assembly.hpp"
#include "appdomain.hpp"
#include "security.h"
#include "comclass.h"
#include "comstring.h"
#include "comreflectioncommon.h"
#include "assemblysink.h"
#include "perfcounters.h"
#include "assemblyname.hpp"
#include "fusion.h"
#include "eeprofinterfaces.h"
#include "reflectclasswriter.h" 
#include "comdynamic.h"


#include "reflectwrap.h"
#include "commember.h"
#include "eeconfig.h"
#include "strongname.h"

#include "ceefilegenwriter.h"
#include "assemblynative.hpp"
#include "timeline.h"


#include "appdomainnative.hpp"
#include "remoting.h"
#include "safegetfilesize.h"
#include "customattribute.h"

// Define these macro's to do strict validation for jit lock and class init entry leaks. 
// This defines determine if the asserts that verify for these leaks are defined or not. 
// These asserts can sometimes go off even if no entries have been leaked so this defines
// should be used with caution.
//
// If we are inside a .cctor when the application shut's down then the class init lock's
// head will be set and this will cause the assert to go off.
//
// If we are jitting a method when the application shut's down then the jit lock's head
// will be set causing the assert to go off.

//#define STRICT_JITLOCK_ENTRY_LEAK_DETECTION
//#define STRICT_CLSINITLOCK_ENTRY_LEAK_DETECTION

BOOL VerifyAllGlobalFunctions(Module *pModule);
BOOL VerifyAllMethodsForClass(Module *pModule, mdTypeDef cl, ClassLoader *pClassLoader);

Assembly::Assembly() :
    m_Context(NULL),
    m_FreeFlag(0),
    m_ulHashAlgId(0),
    m_pbHashValue(NULL),
    m_pwCodeBase(NULL),
    m_pAllowedFiles(NULL),
    m_pDomain(NULL),
    m_pClassLoader(NULL),
    m_pDynamicCode(NULL),
    m_tEntryModule(mdFileNil),
    m_pEntryPoint(NULL),
    m_pManifest(NULL),
    m_pManifestFile(NULL),
    m_kManifest(mdTokenNil),
    m_pManifestImport(NULL),
    m_pManifestAssemblyImport(NULL),
    m_pOnDiskManifest(NULL),
    m_pFusionAssembly(NULL),
    m_pFusionAssemblyName(NULL),
    m_dwCodeBase(0),
    m_pwsFullName(NULL),
    m_dwFlags(0),
    m_psName(NULL),
    m_pbPublicKey(NULL),
    m_cbPublicKey(0),
    m_isDynamic(false),
    m_pSharedSecurityDesc(NULL),
    m_fIsShared(FALSE),
    m_pSharingProperties(NULL),
    m_debuggerFlags(DACF_NONE),
    m_pbRefedPublicKeyToken(NULL),
    m_cbRefedPublicKeyToken(0),
    m_fTerminated(FALSE),
    m_fAllowUntrustedCaller(FALSE),
    m_fCheckedForAllowUntrustedCaller(FALSE)
{
}

Assembly::~Assembly() 
{
    Terminate();

    if (m_psName && (m_FreeFlag & FREE_NAME))
        delete[] m_psName;
    if (m_pbPublicKey && (m_FreeFlag & FREE_PUBLIC_KEY))
        delete[] m_pbPublicKey;
    if (m_pbStrongNameKeyPair && (m_FreeFlag & FREE_KEY_PAIR))
        delete[] m_pbStrongNameKeyPair;
    if (m_pwStrongNameKeyContainer && (m_FreeFlag & FREE_KEY_CONTAINER))
        delete[] m_pwStrongNameKeyContainer;
    if (m_Context) {
        if (m_Context->szLocale && (m_FreeFlag & FREE_LOCALE))
            delete[] m_Context->szLocale;

        delete m_Context;
    }

    if (m_pbHashValue)
        delete[] m_pbHashValue;

    if (m_pbRefedPublicKeyToken)
        StrongNameFreeBuffer(m_pbRefedPublicKeyToken);


    if (m_pAllowedFiles)
    {
        delete(m_pAllowedFiles);
    }

    if (m_pSharingProperties)
    {

        PEFileBinding *p = m_pSharingProperties->pDependencies;
        PEFileBinding *pEnd = p + m_pSharingProperties->cDependencies;
        while (p < pEnd)
        {
            if (p->pPEFile != NULL)
                delete p->pPEFile;
            p++;
        }
        
        delete m_pSharingProperties->pDependencies;
        delete m_pSharingProperties;
    }

    if (IsDynamic())
    {
        if (m_pOnDiskManifest)
        {
            // free the on disk manifest if it is not freed yet. Normally it is freed when we finish saving.
            // However, we can encouter error case such that user is aborting
            //
            if (m_pOnDiskManifest && m_fEmbeddedManifest == false)
                m_pOnDiskManifest->Destruct();
            m_pOnDiskManifest = NULL;
        }
    }

    if (m_pManifestFile && (m_FreeFlag & FREE_PEFILE))
        delete m_pManifestFile;

    ReleaseFusionInterfaces();

}


HRESULT Assembly::Init(BOOL isDynamic)
{
    if (GetDomain() == SharedDomain::GetDomain())
    {
        SetShared();
        m_ExposedObjectIndex = SharedDomain::GetDomain()->AllocateSharedClassIndices(1);
        // Expand the current appdomain's DLS to cover at least the index we
        // just allocated, since security might try to access this slot before
        // we add any shared class indices.
        HRESULT hr;
        IfFailRet(GetAppDomain()->GetDomainLocalBlock()->SafeEnsureIndex(m_ExposedObjectIndex));
    }
    else
        m_ExposedObject = GetDomain()->CreateHandle(NULL);

    m_pClassLoader = new (nothrow) ClassLoader();
    if(!m_pClassLoader) return E_OUTOFMEMORY;
    
    m_pClassLoader->SetAssembly(this);
    m_isDynamic = isDynamic;
    if(!m_pClassLoader->Init()) {
        _ASSERTE(!"Unable to initialize the class loader");
        return E_OUTOFMEMORY;
    }

    m_pSharedSecurityDesc = SharedSecDescHelper::Allocate(this);
    if (!m_pSharedSecurityDesc) return E_OUTOFMEMORY;

    m_pAllowedFiles = new (nothrow) EEUtf8StringHashTable();
    if (!m_pAllowedFiles)
        return E_OUTOFMEMORY;

    COUNTER_ONLY(GetPrivatePerfCounters().m_Loading.cAssemblies++);
    COUNTER_ONLY(GetGlobalPerfCounters().m_Loading.cAssemblies++);

    return S_OK;
}

void Assembly::Terminate( BOOL signalProfiler )
{
    if (this->m_fTerminated)
        return;

#ifdef PROFILING_SUPPORTED
    // Signal profile if present.
    if (signalProfiler && CORProfilerTrackAssemblyLoads())
        g_profControlBlock.pProfInterface->AssemblyUnloadStarted((ThreadID) GetThread(), (AssemblyID) this);
#endif // PROFILING_SUPPORTED
    
    delete m_pSharedSecurityDesc;
    m_pSharedSecurityDesc = NULL;

    if(m_pClassLoader != NULL)
    {
        delete m_pClassLoader;
        m_pClassLoader = NULL;
    }

    // Release the dynamic code module
    if(m_pDynamicCode) 
    {
       m_pDynamicCode->Release();
       m_pDynamicCode = NULL;
    }

    if (m_pManifestImport)
    {
        m_pManifestImport->Release();
        m_pManifestImport=NULL;
    }

    if (m_pManifestAssemblyImport)
    {
        m_pManifestAssemblyImport->Release();
        m_pManifestAssemblyImport=NULL;
    }

        
    if(m_pwCodeBase) 
    {
        delete m_pwCodeBase;
        m_pwCodeBase = NULL;
        m_dwCodeBase = 0;
    }

    COUNTER_ONLY(GetPrivatePerfCounters().m_Loading.cAssemblies--);
    COUNTER_ONLY(GetGlobalPerfCounters().m_Loading.cAssemblies--);

    
#ifdef PROFILING_SUPPORTED
    // Always signal the profiler.
    if (signalProfiler && CORProfilerTrackAssemblyLoads())
        g_profControlBlock.pProfInterface->AssemblyUnloadFinished((ThreadID) GetThread(), (AssemblyID) this, S_OK);
#endif // PROFILING_SUPPORTED

    this->m_fTerminated = TRUE;
    return;  // makes the compiler happy when PROFILING_SUPPORTED isn't defined
}  

void Assembly::ReleaseFusionInterfaces()
{
    if(SystemDomain::BeforeFusionShutdown()) {


        if (m_pFusionAssembly) {
            m_pFusionAssembly->Release();
            m_pFusionAssembly = NULL;
        }
           
        if(m_pFusionAssemblyName) {
            m_pFusionAssemblyName->Release();
            m_pFusionAssemblyName = NULL;
        }
    }

}// ReleaseFusionInterfaces


void Assembly::AllocateExposedObjectHandle(AppDomain *pDomain)
{
    _ASSERTE(pDomain);

    DomainLocalBlock *pLocalBlock = pDomain->GetDomainLocalBlock();
        
    if (pLocalBlock->GetSlot(m_ExposedObjectIndex) == NULL)
        pLocalBlock->SetSlot(m_ExposedObjectIndex, pDomain->CreateHandle(NULL));
}


OBJECTREF Assembly::GetRawExposedObject(AppDomain *pDomain)
{
    OBJECTHANDLE hObject;
    
    //
    // Figure out which handle to use.
    //

    if (IsShared())
    {
        if (pDomain == NULL)
            pDomain = GetAppDomain();

        DomainLocalBlock *pLocalBlock = pDomain->GetDomainLocalBlock();
        
        hObject = (OBJECTHANDLE) pLocalBlock->GetSlot(m_ExposedObjectIndex);
        if (hObject == NULL)
            return NULL;
    }
    else
        hObject = m_ExposedObject;

    //
    // Now get the object from the handle.
    //

    return ObjectFromHandle(hObject);
}

OBJECTREF Assembly::GetExposedObject(AppDomain *pDomain)
{
    THROWSCOMPLUSEXCEPTION();

    OBJECTHANDLE hObject;
    
    //
    // Figure out which handle to use.
    //

    if (IsShared())
    {
        if (pDomain == NULL)
            pDomain = GetAppDomain();

        DomainLocalBlock *pLocalBlock = pDomain->GetDomainLocalBlock();
        
        //
        // Create the handle if necessary.  There should be no race
        // possible here, because we force create the handle before
        // we publish the assembly to other threads.  This is really
        // only here for the case when we need the handle earlier than
        // that (e.g. security)
        //

        hObject = (OBJECTHANDLE) pLocalBlock->GetSlot(m_ExposedObjectIndex);
        if (hObject == NULL)
        {
            hObject = pDomain->CreateHandle(NULL);
            pLocalBlock->SetSlot(m_ExposedObjectIndex, hObject);
        }
    }
    else
        hObject = m_ExposedObject;

    //
    // Now get the object from the handle.
    //

    OBJECTREF ref = ObjectFromHandle(hObject);
    if (ref == NULL)
    {
        MethodTable *pMT;
        if (IsDynamic())
            pMT = g_Mscorlib.GetClass(CLASS__ASSEMBLY_BUILDER);
        else
            pMT = g_Mscorlib.GetClass(CLASS__ASSEMBLY);

        // Create the assembly object
        ASSEMBLYREF obj = (ASSEMBLYREF) AllocateObject(pMT);

        if(obj == NULL)
            COMPlusThrowOM();

        obj->SetAssembly(this);

        StoreFirstObjectInHandle(hObject, (OBJECTREF) obj);

        return (OBJECTREF) obj;
    }

    return ref;
}

ListLock*  Assembly::GetClassInitLock()
{
    return m_pDomain->GetClassInitLock();
}

ListLock* Assembly::GetJitLock()
{
    // Use the same lock as class inits, so we can detect cycles between the two.
    return m_pDomain->GetClassInitLock();
}
    
LoaderHeap* Assembly::GetLowFrequencyHeap()
{
    return m_pDomain->GetLowFrequencyHeap();
}

LoaderHeap* Assembly::GetHighFrequencyHeap()
{
    return m_pDomain->GetHighFrequencyHeap();
}

LoaderHeap* Assembly::GetStubHeap()
{
    return m_pDomain->GetStubHeap();
}

BaseDomain* Assembly::GetDomain()
{
    _ASSERTE(m_pDomain);
    return static_cast<BaseDomain*>(m_pDomain);
}

TypeHandle Assembly::LoadTypeHandle(NameHandle* pName, OBJECTREF *pThrowable,
                                    BOOL dontLoadInMemoryType /*=TRUE*/)
{
    return m_pClassLoader->LoadTypeHandle(pName, pThrowable, dontLoadInMemoryType);
}

HRESULT Assembly::SetParent(BaseDomain* pParent)
{
    m_pDomain = pParent;
    return S_OK;
}

HRESULT Assembly::AddManifestMetadata(PEFile* pFile)
{
    _ASSERTE(pFile);
    
    m_pManifestImport = pFile->GetMDImport();
    m_pManifestImport->AddRef();
    
    if (FAILED(m_pManifestImport->GetAssemblyFromScope(&m_kManifest)))
        return COR_E_ASSEMBLYEXPECTED;

    m_Context = new (nothrow) AssemblyMetaDataInternal;
    TESTANDRETURNMEMORY(m_Context);

    ZeroMemory(m_Context, sizeof(AssemblyMetaDataInternal));
        
    if (m_psName && (m_FreeFlag & FREE_NAME)) {
        delete[] m_psName;
        m_FreeFlag ^= FREE_NAME;
    }
    m_pManifestImport->GetAssemblyProps(m_kManifest,
                                        (const void**) &m_pbPublicKey,
                                        &m_cbPublicKey,
                                        &m_ulHashAlgId,
                                        &m_psName,
                                        m_Context,
                                        &m_dwFlags);

    m_pManifestFile = pFile;
    return S_OK;
}

HRESULT Assembly::AddManifest(PEFile* pFile,
                              IAssembly* pIAssembly,
                              BOOL fProfile)
{
    HRESULT hr;

    //
    // Make sure we don't do this more than once.
    //
    if (m_pManifestImport)
        return S_OK;

#ifdef PROFILING_SUPPORTED
    // Signal profile if present.
    if (CORProfilerTrackAssemblyLoads() && fProfile)
        g_profControlBlock.pProfInterface->AssemblyLoadStarted((ThreadID) GetThread(), (AssemblyID) this);
#endif // PROFILING_SUPPORTED

    if (FAILED(hr = AddManifestMetadata(pFile)))
        return hr;

    LOG((LF_CLASSLOADER, 
         LL_INFO10, 
         "Added manifest: \"%s\".\n", 
         m_psName));

    // We have the manifest in this scope so save it off
    if(m_pManifest) {
        STRESS_ASSERT(0);
        return COR_E_BADIMAGEFORMAT; 
    }

    if(pIAssembly == NULL && SystemDomain::GetStrongAssemblyStatus()) {
        if(!m_cbPublicKey) 
            return COR_E_TYPELOAD;
    }

    if(pIAssembly) {
        hr = SetFusionAssembly(pIAssembly);
        if(FAILED(hr)) return hr;
    }

    hr = CacheManifestExportedTypes();
    if(FAILED(hr)) return hr;
    hr = CacheManifestFiles();
    if(FAILED(hr)) return hr;
   
    // If the module containing the manifest has a token in the File Reference table
    // then that file will contain the real entry point for the assembly. When we
    // go to execute it will be necessary to get that module and find the entry
    // token stored the that modules header. If this module contains code as well
    // as the manifest then it may also contain the token pointing to the entry 
    // location. This case is handled by Assembly::AddModule().
    // InMemory modules do not have a header so we ignore them.
    if (!m_pEntryPoint) {
        mdToken tkEntry = VAL32(pFile->GetCORHeader()->EntryPointToken);
        if (TypeFromToken(tkEntry) == mdtFile)
            m_tEntryModule = tkEntry;
    }

    return hr;
}


// Returns;
// S_OK : if it was able to load the module
// S_FALSE : if the file reference is to a resource file and cannot be loaded
// S_FALSE but module set: module already set in file rid map
// otherwise it returns errors.
HRESULT Assembly::FindInternalModule(mdFile kFile,
                                     mdToken  mdTokenNotToLoad,
                                     Module** ppModule,
                                     OBJECTREF* pThrowable)
{
    HRESULT hr = S_OK;

    Module* pModule = m_pManifest->LookupFile(kFile);
    if(!pModule) {
        if (mdTokenNotToLoad != tdAllTypes)
            hr = LoadInternalModule(kFile, m_pManifestImport, 
                                    &pModule, pThrowable);
        else
            return E_FAIL;
    }

    if(SUCCEEDED(hr) && ppModule)
        *ppModule = pModule;

    return hr;
}

// Returns;
// S_OK : if it was able to load the module
// S_FALSE : if the file reference is to a resource file and cannot be loaded
// S_FALSE but module set: module already set in file rid map
// otherwise it returns errors.
HRESULT Assembly::LoadInternalModule(mdFile kFile, IMDInternalImport *pImport,
                                     Module** ppModule, OBJECTREF* pThrowable)
{
    HRESULT hr = S_OK;
    LPCSTR psModuleName = NULL;
    const BYTE* pHash;
    DWORD dwFlags = 0;
    ULONG dwHashLength = 0;

    pImport->GetFileProps(kFile,
                          &psModuleName,
                          (const void**) &pHash,
                          &dwHashLength,
                          &dwFlags);

    if(IsFfContainsMetaData(dwFlags)) {
        WCHAR pPath[MAX_PATH];
        hr = LoadInternalModule(psModuleName,
                                kFile,
                                m_ulHashAlgId,
                                pHash,
                                dwHashLength,
                                dwFlags,
                                pPath,
                                MAX_PATH,
                                ppModule,
                                pThrowable);
    }
    else
        hr = S_FALSE;

    return hr;

}

HRESULT Assembly::CopyCodeBase(LPCWSTR pCodeBase)
{
    if(m_pwCodeBase) {
        delete m_pwCodeBase;
        m_pwCodeBase = NULL;
        m_dwCodeBase = 0;
    }

    if(pCodeBase) {
        DWORD lgth = (DWORD)(wcslen(pCodeBase) + 1);
        m_pwCodeBase = new (nothrow) WCHAR[lgth];
        if(!m_pwCodeBase) return E_OUTOFMEMORY;

        memcpy(m_pwCodeBase, pCodeBase, lgth*sizeof(WCHAR));
        m_dwCodeBase = lgth-1;
    }
    return S_OK;
}


HRESULT Assembly::FindCodeBase(WCHAR* pCodeBase, DWORD* pdwCodeBase, BOOL fCopiedName)
{
    Module *pModule = m_pManifest;

    HRESULT hr = S_OK;
    if(pModule == NULL) 
        pModule = GetLoader()->m_pHeadModule;
    
    if(m_pManifestFile)
        return FindAssemblyCodeBase(pCodeBase, pdwCodeBase, (fCopiedName && GetDomain()->IsShadowCopyOn()));
    else if (pModule && pModule->IsPEFile())
        return pModule->GetPEFile()->FindCodeBase(pCodeBase, pdwCodeBase);
    else {
        if (*pdwCodeBase > 0)
            *pCodeBase = 0;
        else 
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);

        *pdwCodeBase = 1;
    }

    return hr;
}

HRESULT Assembly::GetCodeBase(LPWSTR *pwCodeBase, DWORD* pdwCodeBase)
{
    // If the assembly does not have a code base then build one
    if(m_pwCodeBase == NULL) {
        WCHAR* pCodeBase = NULL;
        DWORD  dwCodeBase = 0;

        HRESULT hr = FindCodeBase(pCodeBase, &dwCodeBase, FALSE);
        if(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
            pCodeBase = (WCHAR*) _alloca(dwCodeBase*sizeof(WCHAR));
            hr = FindCodeBase(pCodeBase, &dwCodeBase, FALSE);
        }
        if(FAILED(hr)) return hr;
        
        hr = CopyCodeBase(pCodeBase);
        if(FAILED(hr)) return hr;
    }
    
    if(pwCodeBase)
        *pwCodeBase = m_pwCodeBase;
    if(pdwCodeBase)
        *pdwCodeBase = m_dwCodeBase;
    
    return S_OK;
}

HRESULT Assembly::FindAssemblyCodeBase(WCHAR* pCodeBase, 
                                       DWORD* pdwCodeBase, 
                                       BOOL fCopiedName)
{
    HRESULT hr = S_OK;
    if(m_pFusionAssembly) {
        if(!fCopiedName) {
            IAssemblyName *pNameDef;
            DWORD dwSize = *pdwCodeBase;
            hr = m_pFusionAssembly->GetAssemblyNameDef(&pNameDef);
            if (SUCCEEDED(hr)) {
                hr = pNameDef->GetProperty(ASM_NAME_CODEBASE_URL, pCodeBase, &dwSize);
                pNameDef->Release();
                
                // If we end up with no codebase then use the file name instead. 
                if((SUCCEEDED(hr) || HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) && dwSize == 0)
                    fCopiedName  = true;
                else
                    *pdwCodeBase = dwSize;
                
#ifdef _DEBUG
                if(SUCCEEDED(hr)) 
                    LOG((LF_CLASSLOADER, LL_INFO10, "Found Fusion CodeBase: \"%ws\".\n", pCodeBase));
#endif
                
            }
        }
        
        if(fCopiedName)
            return PEFile::FindCodeBase(m_pManifestFile->GetFileName(), pCodeBase, pdwCodeBase);

        return hr;
    }
    else 
        return m_pManifestFile->FindCodeBase(pCodeBase, pdwCodeBase);
}

HRESULT Assembly::SetFusionAssembly(IAssembly *pFusionAssembly)
{
    HRESULT hr = NOERROR;
    // The following is in it's own scope so that gcc will not emit
    // a invalid warning
    {
        TIMELINE_AUTO(LOADER, "SetFusionAssembly");

        _ASSERTE(pFusionAssembly != NULL);

        if (m_pFusionAssembly) {
            m_pFusionAssemblyName->Release();
            m_pFusionAssembly->Release();
        }

        pFusionAssembly->AddRef();

        m_pFusionAssembly = pFusionAssembly;

        IfFailRet(pFusionAssembly->GetAssemblyNameDef(&m_pFusionAssemblyName));
    }

    return hr;
}

AssemblySecurityDescriptor *Assembly::GetSecurityDescriptor(AppDomain *pDomain)
{
    AssemblySecurityDescriptor *pSecDesc;

    if (pDomain == NULL)
        pDomain = GetAppDomain();

    pSecDesc = m_pSharedSecurityDesc->FindSecDesc(pDomain);

    // If we didn't find a security descriptor for this appdomain context we
    // need to create one now.
    if (pSecDesc == NULL) {

        pSecDesc = AssemSecDescHelper::Allocate(pDomain);
        if (pSecDesc == NULL)
            return NULL;

        pSecDesc = pSecDesc->Init(this);
    }

    return pSecDesc;
}

HRESULT Assembly::LoadInternalModule(LPCSTR    pName,
                                     mdFile    kFile,
                                     DWORD     dwHashAlgorithm,
                                     const     BYTE*   pbHash,
                                     DWORD     cbHash,
                                     DWORD     flags,
                                     WCHAR*    pPath,
                                     DWORD     dwPath,
                                     Module    **ppModule,
                                     OBJECTREF *pThrowable)
{
    if(!pName || !*pName) return E_POINTER;
    _ASSERTE(m_pManifest);  // We need to have a manifest before we can load a module

    MAKE_WIDEPTR_FROMUTF8(pwName,pName);

    // Verification of the name
    if (wcschr(pwName, L'\\') ||
        wcschr(pwName, L'/')  ||
        wcschr(pwName, L':')  ||
        (RunningOnWin95() && ContainsUnmappableANSIChars(pwName))) {
        BAD_FORMAT_ASSERT(!"NULL Or Bad File Name");
        return COR_E_BADIMAGEFORMAT;
    }

    // load the module
    PEFile *pFile = NULL;

    HRESULT hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    if(GetFusionAssembly())
        hr = GetFileFromFusion(pwName,
                               pPath,
                               dwPath);

    if(FAILED(hr) && HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME) != hr) {

        // Only check when we actually have a manifest file path.
        if (!(m_pManifest->GetFileName() && *(m_pManifest->GetFileName())))
            goto ErrExit;

        wcscpy(pPath, m_pManifest->GetFileName());

        LOG((LF_CLASSLOADER, 
             LL_INFO10, 
             "Retrieving internal module for: \"%S\".\n", 
             pPath));

        // Save this point
        WCHAR* tail = wcsrchr(pPath, '\\');
#ifdef PLATFORM_UNIX
        WCHAR *tail2 = wcsrchr(pPath, '/');
        if (tail2 > tail) {
	  tail = tail2;
	}
#else
        // Add the directory divider
        _ASSERTE(*tail == '\\');
#endif
        tail++;
        
        // Get the name and remove any slash
        WCHAR *pwChar = wcsrchr(pwName, '\\');
#ifdef PLATFORM_UNIX
	WCHAR *pwChar2 = wcsrchr(pwName, '/');
	if (pwChar2 > pwChar) {
	  pwChar = pwChar2;
	}
#endif
        if (pwChar)
            pwChar++;
        else
            pwChar = pwName;
        
        if ((DWORD) ((tail - pPath) + wcslen(pwChar)) >= dwPath)
            goto ErrExit;

        // Add the name of the module
        wcscpy(tail, pwChar);
    }

    hr = VerifyInternalModuleHash(pPath,
                                  dwHashAlgorithm,
                                  pbHash,
                                  cbHash,
                                  pThrowable);
    // Don't load this file
    if (!kFile)
        goto ErrExit;

    if (SUCCEEDED(hr))
        // Modules may come from fusion but we don't care
        hr = SystemDomain::LoadFile(pPath, 
                                    this, 
                                    kFile,
                                    FALSE, 
                                    NULL,
                                    NULL, // code base is determined by the assembly not the module
                                    NULL,
                                    &pFile, 
                                    IsFfContainsNoMetaData(flags));

    if(hr == S_OK) {
        hr = LoadFoundInternalModule(pFile,
                                     kFile,
                                     IsFfContainsNoMetaData(flags),
                                     ppModule,
                                     pThrowable);
    }
    else if (!ModuleFound(hr)) {
        // Module not found
        Module* pModule = RaiseModuleResolveEvent(pName, pThrowable);

        if (pModule &&
            (pModule == m_pManifest->LookupFile(kFile))) {
            hr = S_OK;
            if(ppModule) *ppModule = pModule;
        }
    }

 ErrExit:
    if (FAILED(hr))
        PostFileLoadException(pName, FALSE,NULL, hr, pThrowable);

    return hr;
}

HRESULT Assembly::VerifyInternalModuleHash(WCHAR*      pPath,
                                           DWORD       dwHashAlgorithm,
                                           const BYTE* pbHash,
                                           DWORD       cbHash,
                                           OBJECTREF*  pThrowable)
{
    HRESULT hr = S_OK;
    if ( !m_pManifestFile->HashesVerified() &&
         (m_cbPublicKey ||
          m_pManifest->GetSecurityDescriptor()->IsSigned()) ) {

        if (!pbHash)
            return COR_E_MODULE_HASH_CHECK_FAILED;

        // The hash was originally done on the entire file, as a data file.
        // When we normally load this file at runtime, we may not load the
        // entire file, and it's not loaded as a data file, anyway.  So,
        // unfortunately, we must reload, or fail the hash verification.
        PBYTE pbBuffer;
        DWORD dwResultLen;
        
        hr = ReadFileIntoMemory(pPath, &pbBuffer, &dwResultLen);
        if (FAILED(hr))
            return hr;
        hr = VerifyHash(pbBuffer,
                        dwResultLen,
                        dwHashAlgorithm,
                        pbHash,
                        cbHash);
        delete[] pbBuffer;
    }

    return hr;
}

HRESULT Assembly::LoadFoundInternalModule(PEFile    *pFile,
                                          mdFile    kFile,
                                          BOOL      fResource,
                                          Module    **ppModule,
                                          OBJECTREF *pThrowable)
{
    if (!pFile)
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

    BaseDomain* pDomain = GetDomain();
    _ASSERTE(pDomain);

    HRESULT hr = S_OK;
    Module *pModule = NULL;
    pDomain->EnterLoadLock();
    EE_TRY_FOR_FINALLY {
    // Check if this module has already been added
    Module* pRidModule = m_pManifest->LookupFile(kFile);
    if (pRidModule) {
        delete pFile;
        pModule = pRidModule;
        hr = S_FALSE;
        EE_LEAVE;
    }
    
    pModule = pDomain->FindModule(pFile->GetBase());
            
    // Make sure it is not us.
    if (pModule) {
        if (pModule->GetAssembly() != this)
            pModule = NULL;
        else
            delete pFile;
    }

    if (!pModule) {
        if (fResource)
            hr = Module::CreateResource(pFile, &pModule);
        else  {
	        hr = Module::Create(pFile, NULL, &pModule,
                                    CORDebuggerEnCMode(GetDebuggerInfoBits()));    
        }
    }

    if(SUCCEEDED(hr)) {
        if (fResource) {
            DWORD dwModuleIndex;
            // If InsertModule fails, it has already been added.
            hr = m_pClassLoader->InsertModule(pModule, kFile, &dwModuleIndex);
            if (hr == S_OK)
                pModule->SetContainer(this, dwModuleIndex, kFile, true, pThrowable);
        }
        else
            hr = AddModule(pModule, kFile, FALSE, pThrowable);
    }

    if(SUCCEEDED(hr)) {
        hr = S_OK;
    }

    }
    EE_FINALLY {
        pDomain->LeaveLoadLock();
    } EE_END_FINALLY;
    
    if(SUCCEEDED(hr)) {
        if(ppModule) *ppModule = pModule;
    }

    return hr;
}

#define ConvertAssemblyRefToContext(pContext, pI)                                             \
    pContext.usMajorVersion = pI.usMajorVersion;                                              \
    pContext.usMinorVersion = pI.usMinorVersion;                                              \
    pContext.usBuildNumber = pI.usBuildNumber;                                                \
    pContext.usRevisionNumber = pI.usRevisionNumber;                                          \
    if(pI.szLocale) {                                                                         \
        MAKE_WIDEPTR_FROMUTF8(pLocale, pI.szLocale);                                          \
        pContext.szLocale = pLocale;                                                          \
        pContext.cbLocale = wcslen(pLocale) + 1;                                              \
    } else {                                                                                  \
        pContext.szLocale = NULL;                                                             \
        pContext.cbLocale = 0;                                                                \
    }


// Returns S_OK
//         
HRESULT Assembly::FindExternalAssembly(Module* pTokenModule,
                                       mdAssemblyRef kAssemblyRef,
                                       IMDInternalImport *pImport, 
                                       mdToken mdTokenNotToLoad,
                                       Assembly** ppAssembly,
                                       OBJECTREF* pThrowable)
{
    HRESULT hr = S_OK;
    Assembly* pFoundAssembly = pTokenModule->LookupAssemblyRef(kAssemblyRef);
    if(!pFoundAssembly) {
        // Get the referencing assembly. This is used 
        // as a hint to find the location of the other assembly
        Assembly* pAssembly = pTokenModule->GetAssembly();

        // we do not care about individual tokens since this is out of scope.
        // When mdTokenNotToLoad is set to a single typedef it stops recursive
        // loads and this can not happen with external references.
        if (mdTokenNotToLoad != tdAllTypes) {

            hr = LoadExternalAssembly(kAssemblyRef,
                                      pImport,
                                      pAssembly,
                                      &pFoundAssembly,
                                      pThrowable);
            if(SUCCEEDED(hr)) {
                if(!pTokenModule->StoreAssemblyRef(kAssemblyRef, pFoundAssembly))
                    hr = E_OUTOFMEMORY;
            }
        }
        else
            return E_FAIL;
    }     

    if(SUCCEEDED(hr) && ppAssembly)
        *ppAssembly = pFoundAssembly;

    return hr;
}

// Returns S_OK
//         
HRESULT Assembly::LoadExternalAssembly(mdAssemblyRef      kAssemblyRef, 
                                       IMDInternalImport* pImport, 
                                       Assembly*          pAssembly,
                                       Assembly**         ppAssembly,
                                       OBJECTREF*         pThrowable)
{
    AssemblySpec spec;
    HRESULT hr;

    if (FAILED(hr = spec.InitializeSpec(kAssemblyRef, pImport, pAssembly)))
        return hr;

    //Strong assemblies should not be able to reference simple assemblies.
    if (pAssembly->m_cbPublicKey &&
        (!spec.IsStronglyNamed())) {
        MAKE_UTF8PTR_FROMWIDE(szName,
                              spec.GetName() ? L"" : spec.GetCodeBase()->m_pszCodeBase);

        LOG((LF_CLASSLOADER, LL_ERROR, "Could not load assembly '%s' because it was not strongly-named\n", spec.GetName() ? spec.GetName() : szName));
        
        PostFileLoadException(spec.GetName() ? spec.GetName() : szName, 
                              FALSE,NULL, FUSION_E_PRIVATE_ASM_DISALLOWED, pThrowable);
        return FUSION_E_PRIVATE_ASM_DISALLOWED;
    }

    return spec.LoadAssembly(ppAssembly, pThrowable);
}


/* static */
HRESULT Assembly::VerifyHash(PBYTE pbBuffer,
                             DWORD dwBufferLen,
                             ALG_ID iHashAlg,
                             const BYTE* pbGivenValue,
                             DWORD cbGivenValue)
{
    PBYTE pbCurrentValue = NULL;
    DWORD cbCurrentValue;

    // If hash is not provided, then don't verify it!
    if (cbGivenValue == 0)
        return NOERROR;

    HRESULT hr = GetHash(pbBuffer,
                         dwBufferLen,
                         iHashAlg,
                         &pbCurrentValue,
                         &cbCurrentValue);
    if (FAILED(hr))
        return hr;

    if (cbCurrentValue != cbGivenValue)
        hr = COR_E_MODULE_HASH_CHECK_FAILED;
    else {
        if (memcmp(pbCurrentValue, pbGivenValue, cbCurrentValue))
            hr = COR_E_MODULE_HASH_CHECK_FAILED;
        else
            hr = S_OK;
    }

    delete[] pbCurrentValue;
    return hr;
}

/* static */
// You should delete[] pbCurrentValue when you're through with it, if GetHash
// returns S_OK
HRESULT Assembly::GetHash(WCHAR*    strFullFileName,
                          ALG_ID    iHashAlg,
                          BYTE**    pbCurrentValue, // should be NULL
                          DWORD*    cbCurrentValue)
{
    HRESULT     hr;
    PBYTE       pbBuffer = 0;
    DWORD       dwBufLen = 0;;

    IfFailGo(ReadFileIntoMemory(strFullFileName, &pbBuffer, &dwBufLen));
    hr = GetHash(pbBuffer, dwBufLen, iHashAlg, pbCurrentValue, cbCurrentValue);
ErrExit:
    if (pbBuffer)
        delete[] pbBuffer;
    return S_OK;
}

/* static */
// You should delete[] ppbBuffer when you're through with it, if ReadFileIntoMemory
// returns S_OK
HRESULT Assembly::ReadFileIntoMemory(LPCWSTR    strFullFileName,
                                     BYTE**     ppbBuffer,
                                     DWORD*     pdwBufLen)
{
    // The hash needs to be done on the entire file, as a data file.
    HANDLE hFile = WszCreateFile(strFullFileName,
                                 GENERIC_READ,
                                 FILE_SHARE_READ,
                                 NULL,
                                 OPEN_EXISTING,
                                 FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                                 NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return HRESULT_FROM_WIN32(GetLastError());

    DWORD dwFileLen = SafeGetFileSize(hFile, 0);
    if (dwFileLen == 0xffffffff)
    {
        CloseHandle(hFile);
        return HRESULT_FROM_WIN32(GetLastError());
    }
    _ASSERTE(ppbBuffer);
    *ppbBuffer = new (nothrow) BYTE[dwFileLen];
    TESTANDRETURNMEMORY(*ppbBuffer);
      
    if ((SetFilePointer(hFile, 0, NULL, FILE_BEGIN) == 0xFFFFFFFF) ||
        (!ReadFile(hFile, *ppbBuffer, dwFileLen, pdwBufLen, NULL))) {
        CloseHandle(hFile);
        delete[] *ppbBuffer;
        *ppbBuffer = 0;
        return HRESULT_FROM_WIN32(GetLastError());
    }
    _ASSERTE(dwFileLen == *pdwBufLen);
    CloseHandle(hFile);
    return S_OK;
}   // Assembly::ReadFileIntoMemory

/* static */
// You should delete[] pbCurrentValue when you're through with it, if GetHash
// returns S_OK
HRESULT Assembly::GetHash(PBYTE pbBuffer,
                          DWORD dwBufferLen,
                          ALG_ID iHashAlg,
                          BYTE** pbCurrentValue,  // should be NULL
                          DWORD *cbCurrentValue)
{
    HRESULT    hr;
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    DWORD      dwCount = sizeof(DWORD);

    if (*pbCurrentValue)
        return E_POINTER;


    // No need to late bind this stuff, all these crypto API entry points happen
    // to live in ADVAPI32.

    if ((!WszCryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) ||
        (!CryptCreateHash(hProv, iHashAlg, 0, 0, &hHash)) ||
        (!CryptHashData(hHash, pbBuffer, dwBufferLen, 0)) ||
        (!CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE *) cbCurrentValue, 
                            &dwCount, 0))) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }

    *pbCurrentValue = new (nothrow) BYTE[*cbCurrentValue];
    if (!(*pbCurrentValue)) {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    if(!CryptGetHashParam(hHash, HP_HASHVAL, *pbCurrentValue, cbCurrentValue, 0)) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        delete[] *pbCurrentValue;
        *pbCurrentValue = 0;
        goto exit;
    }

    hr = S_OK;

 exit:
    if (hHash)
        CryptDestroyHash(hHash);
    if (hProv)
        CryptReleaseContext(hProv, 0);

    return hr;
}

HRESULT Assembly::FindModuleByExportedType(mdExportedType mdType, 
                                           mdToken tokenNotToLoad,
                                           mdTypeDef mdNested, 
                                           Module** ppModule,
                                           mdTypeDef* pCL, 
                                           OBJECTREF *pThrowable)
{
    HRESULT hr;
    mdToken mdLinkRef;
    mdToken mdBinding;
    LPCSTR pszNameSpace;
    LPCSTR pszName;

    m_pManifestImport->GetExportedTypeProps(mdType,
                                            &pszNameSpace,
                                            &pszName,
                                            &mdLinkRef, //impl
                                            &mdBinding, // Hint
                                            NULL); // dwflags

    switch(TypeFromToken(mdLinkRef)) {
    case mdtAssemblyRef:
        Assembly *pFoundAssembly;
        hr = FindExternalAssembly(m_pManifest,
                                  mdLinkRef,
                                  m_pManifestImport,
                                  tokenNotToLoad,
                                  &pFoundAssembly,
                                  pThrowable);
        if (SUCCEEDED(hr)) {
            *pCL = mdTypeDefNil;  // We don't trust the mdBinding token
            *ppModule = pFoundAssembly->m_pManifest;
        }
        break;

    case mdtFile:

        if (mdLinkRef == mdFileNil) {
            *ppModule = m_pManifest;  // type is in same file as manifest
            hr = S_OK;
        }
        else
            hr = FindInternalModule(mdLinkRef,
                                    tokenNotToLoad,
                                    ppModule,
                                    pThrowable);

        // We may not want to trust this TypeDef token, since it
        // was saved in a scope other than the one it was defined in
        if(SUCCEEDED(hr)) {
            if (mdNested == mdTypeDefNil)
                *pCL = mdBinding;
            else
                *pCL = mdNested;
        }
        break;

    case mdtExportedType:
        // Only override the nested type token if it hasn't been set yet.
        if (mdNested != mdTypeDefNil)
            mdBinding = mdNested;
        return FindModuleByExportedType(mdLinkRef, tokenNotToLoad, mdBinding,
                                        ppModule, pCL, pThrowable);

    default:
        STRESS_ASSERT(0);
        hr = COR_E_BADIMAGEFORMAT;
        BAD_FORMAT_ASSERT(!"Invalid token type");
    }

    return hr;
}

// Returns CLDB_S_NULL if nil-scoped token
HRESULT Assembly::FindAssemblyByTypeRef(NameHandle* pName,
                                        Assembly** ppAssembly,
                                        OBJECTREF *pThrowable)
{
    HRESULT hr = E_FAIL;
    _ASSERTE(pName);
    _ASSERTE(pName->GetTypeModule());
    _ASSERTE(pName->GetTypeToken());
    _ASSERTE(ppAssembly);

    IMDInternalImport *pImport = pName->GetTypeModule()->GetMDImport();
    mdTypeRef tkType = pName->GetTypeToken();
    _ASSERTE(TypeFromToken(tkType) == mdtTypeRef);

    // If nested, get top level encloser's impl
    do {
        tkType = pImport->GetResolutionScopeOfTypeRef(tkType);
        if (IsNilToken(tkType)) {
            *ppAssembly = this;
            return CLDB_S_NULL;  // nil-scope TR okay if there's an ExportedType
        }
    } while (TypeFromToken(tkType) == mdtTypeRef);

    switch (TypeFromToken(tkType)) {
    case mdtModule:
        *ppAssembly = pName->GetTypeModule()->GetAssembly();
        return S_OK;

    case mdtModuleRef:
        Module *pModule;
        if (SUCCEEDED(hr = FindModuleByModuleRef(pImport,
                                                 tkType,
                                                 pName->GetTokenNotToLoad(),
                                                 &pModule,
                                                 pThrowable)))
            *ppAssembly = pModule->GetAssembly();
        break;
        
    case mdtAssemblyRef:
        return FindExternalAssembly(pName->GetTypeModule(),
                                  tkType, 
                                  pImport, 
                                  pName->GetTokenNotToLoad(),
                                  ppAssembly, 
                                  pThrowable);

    default:
        // null token okay if there's an ExportedType
        if (IsNilToken(tkType)) {
            *ppAssembly = this;
            return CLDB_S_NULL;
        }

        _ASSERTE(!"Invalid token type");
    }

    return hr;
}

HRESULT Assembly::FindModuleByModuleRef(IMDInternalImport *pImport,
                                        mdModuleRef tkMR,
                                        mdToken tokenNotToLoad,
                                        Module** ppModule,
                                        OBJECTREF *pThrowable)
{
    // get the ModuleRef, match it by name to a File
    LPCSTR pszModuleName;
    HashDatum datum;
    Module *pFoundModule;
    HRESULT hr = COR_E_DLLNOTFOUND;

    if(pImport->IsValidToken(tkMR))
    {
        pImport->GetModuleRefProps(tkMR, &pszModuleName);

        if (m_pAllowedFiles->GetValue(pszModuleName, &datum)) {
            if (datum) { // internal module
                hr = FindInternalModule((mdFile)(size_t)datum,
                                        tokenNotToLoad,
                                        &pFoundModule,
                                        pThrowable);
                if (SUCCEEDED(hr) && pFoundModule)
                    *ppModule = pFoundModule;
            }
            else { // manifest file
                *ppModule = m_pManifest;
                hr = S_OK;
            }
        }
        else
            hr = COR_E_UNAUTHORIZEDACCESS;
    }
    return hr;
}

// Determines if the module contains an assembly.
/* static */
HRESULT Assembly::CheckFileForAssembly(PEFile* pFile)
{
    mdAssembly kManifest;
    HRESULT hr;
    
    IMDInternalImport *pMDImport = pFile->GetMDImport(&hr);
    if (!pMDImport)
        return hr;

    if(pFile->GetMDImport()->GetAssemblyFromScope(&kManifest) == S_OK) 
        return S_OK;

    return COR_E_ASSEMBLYEXPECTED;
}

BOOL Assembly::IsAssembly()
{
    return (mdTokenNil != m_kManifest);
}

HRESULT Assembly::CacheManifestExportedTypes()
{
    _ASSERTE(IsAssembly());
    _ASSERTE(m_pManifestImport);

    HRESULT hr;
    HENUMInternal phEnum;
    mdToken mdExportedType;
    mdToken mdImpl;
    LPCSTR pszName;
    LPCSTR pszNameSpace;
        
    hr = m_pManifestImport->EnumInit(mdtExportedType,
                                     mdTokenNil,
                                     &phEnum);

    if (SUCCEEDED(hr)) {
        m_pClassLoader->LockAvailableClasses();

        for(int i = 0; m_pManifestImport->EnumNext(&phEnum, &mdExportedType); i++) {
            if(TypeFromToken(mdExportedType) == mdtExportedType) {
                m_pManifestImport->GetExportedTypeProps(mdExportedType,
                                                        &pszNameSpace,
                                                        &pszName,
                                                        &mdImpl,
                                                        NULL,   // type def
                                                        NULL); // flags
                IfFailGo(m_pClassLoader->AddExportedTypeHaveLock(pszNameSpace, pszName, mdExportedType, m_pManifestImport, mdImpl));
            }
        }
        
    ErrExit:
        m_pClassLoader->UnlockAvailableClasses();
        m_pManifestImport->EnumClose(&phEnum);
    }

    return hr;
}                      
                      

HRESULT Assembly::CacheManifestFiles()
{
    HENUMInternal phEnum;
    mdToken tkFile;
    LPCSTR pszFileName;
    int i;

    HRESULT hr = m_pManifestImport->EnumInit(mdtFile,
                                             mdTokenNil,
                                             &phEnum);

    if (SUCCEEDED(hr)) {
        DWORD dwCount = m_pManifestImport->EnumGetCount(&phEnum);
        if (!m_pAllowedFiles->Init(dwCount ? dwCount+1 : 1, NULL))
            IfFailGo(E_OUTOFMEMORY);
    
        for(i = 0; m_pManifestImport->EnumNext(&phEnum, &tkFile); i++) {

            if(TypeFromToken(tkFile) == mdtFile) {
                m_pManifestImport->GetFileProps(tkFile,
                                                &pszFileName,
                                                NULL,  // hash
                                                NULL,  // hash len
                                                NULL); // flags

                // Add each internal module
                if (!m_pAllowedFiles->InsertValue(pszFileName, (HashDatum)(size_t)tkFile, FALSE))
                    IfFailGo(E_OUTOFMEMORY);
            }
        }

        // Add the manifest file
        m_pManifestImport->GetScopeProps(&pszFileName, NULL);
        if (!m_pAllowedFiles->InsertValue(pszFileName, NULL, FALSE))
            hr = E_OUTOFMEMORY;

    ErrExit:
        m_pManifestImport->EnumClose(&phEnum);

    }

    return hr;
}                      

HRESULT Assembly::GetModuleFromFilename(LPCWSTR wszModuleFilename,
                                        Module **ppModule)
{
    _ASSERTE(wszModuleFilename && ppModule);

    // Sensible default value
    *ppModule = NULL;

    // Let's split up the module name in case the caller provided a full
    // path, which makes no sense in the case of a manifest
    // The file name could be fully qualified, so need to chop it apart.
    {
        WCHAR *wszName = (WCHAR *)_alloca(MAX_PATH * sizeof(WCHAR));
        WCHAR wszExt[MAX_PATH];
        SplitPath(wszModuleFilename, NULL, NULL, wszName, wszExt);

        // Concat the extension
        wcscat(wszName, wszExt);

        // Replace the pointer provided with our own
        wszModuleFilename = wszName;
    }

    // First, check the manifest's file, since the manifest does not enumerate
    // itself
    {
        PEFile *pManFile = GetManifestFile();
        if (!pManFile)
            return (COR_E_NOTSUPPORTED); // Fails for dynamic modules

        // The file name could be fully qualified, so need to chop it apart.
        WCHAR wszName[MAX_PATH];
        WCHAR wszExt[MAX_PATH];
        SplitPath(pManFile->GetFileName(), NULL, NULL, wszName, wszExt);

        // Concat the extension
        wcscat(wszName, wszExt);

        // Compare the two names
        if (_wcsicmp(wszName, wszModuleFilename) == 0)
        {
            // So the manifest is also the module, so set it
            *ppModule = GetManifestModule();
            _ASSERTE(*ppModule);

            return (S_OK);
        }
    }

    // Next, have a look in the cache of files associated with the manifest
    {
        // This stuff is all done with UTF8 strings, so we need to
        // convert the wide string passed in.
        MAKE_UTF8PTR_FROMWIDE(szModuleFilename, wszModuleFilename);
        _ASSERTE(szModuleFilename);

        mdToken tkFile;
#ifdef _DEBUG
        tkFile = mdFileNil;
#endif //_DEBUG

        // Do a lookup in the hash
        BOOL fRes = m_pAllowedFiles->GetValue(szModuleFilename, (HashDatum *) &tkFile);

        // If we successfully found a match
        if (fRes)
        {
#ifdef _DEBUG
            _ASSERTE(tkFile != mdFileNil);
#endif //_DEBUG

            OBJECTREF objThrow = NULL;
            GCPROTECT_BEGIN(objThrow);

            // Try to get the Module for this filename
            HRESULT hr = FindInternalModule(tkFile, mdTokenNil, ppModule, &objThrow);

            // If an exception was thrown, convert it to an hr
            if (FAILED(hr) && objThrow != NULL)
                hr = SecurityHelper::MapToHR(objThrow);

            GCPROTECT_END();

            // Found it, so quit the search
            _ASSERTE(*ppModule);
            return (S_OK);
        }
    }

    return (E_FAIL);
}


HRESULT Assembly::AddModule(Module* module, mdFile kFile, BOOL fNeedSecurity, OBJECTREF *pThrowable)
{
    HRESULT hr = S_OK;

    BEGINCANNOTTHROWCOMPLUSEXCEPTION();
    _ASSERTE(m_pClassLoader);
    _ASSERTE(module);


    COMPLUS_TRY
    {
        DWORD index;
        // If InsertModule returns S_FALSE, it has already been added.
        hr = m_pClassLoader->InsertModule(module, kFile, &index);
        if (hr != S_OK) {
            if (hr == S_FALSE)
                hr = S_OK;
            COMPLUS_LEAVE;
        }

#ifdef PROFILING_SUPPORTED
        // Signal profile if present, but only in legacy mode
        if (CORProfilerTrackAssemblyLoads() && m_pManifest == NULL)
            g_profControlBlock.pProfInterface->AssemblyLoadStarted((ThreadID) GetThread(), (AssemblyID) this);
#endif // PROFILING_SUPPORTED
        
    
        AssemblySecurityDescriptor *pSec = GetSecurityDescriptor();
        _ASSERTE(pSec);

        if(fNeedSecurity)
            pSec->SetSecurity(module->IsSystem() ? true : false);

        if( module->IsPEFile() ) {
            // Check to see if the entry token stored in the header is a token for a MethodDef.
            // If it is then this is the entry point that is called. We don't want to do this if
            // the module is InMemory because it will not have a header.
            if (!m_pEntryPoint) {
                mdToken tkEntry = VAL32(module->GetCORHeader()->EntryPointToken);
                if (TypeFromToken(tkEntry) == mdtMethodDef)
                    m_pEntryPoint = module;
            }
        }

        TIMELINE_START(LOADER, ("EarlyResolve"));

        // If explicit permission requests were made we should
        // resolve policy now in case we can't grant the minimal
        // required permissions.
        if (fNeedSecurity
            && Security::IsSecurityOn()
            && !module->IsSystem()) {
            hr = Security::EarlyResolve(this, pSec, pThrowable);
            if (FAILED(hr)) COMPLUS_LEAVE;
        }

        TIMELINE_END(LOADER, ("EarlyResolve"));

        // Set up the module side caching of meta data, stubs, etc.
        hr = module->SetContainer(this, index, kFile, false, pThrowable);
        if(FAILED(hr)) COMPLUS_LEAVE;

    #ifdef PROFILING_SUPPORTED
        // Signal the profiler that the assembly is loaded.  This is done only in legacy mode,
        // since it is signalled in LoadManifest method in non-legacy mode.  This is ok, since
        // legacy mode only has one module per assembly and this will thus only be called once.
        if (CORProfilerTrackAssemblyLoads() && m_pManifest == NULL)
            g_profControlBlock.pProfInterface->AssemblyLoadFinished((ThreadID) GetThread(), (AssemblyID) this, hr);
    #endif // PROFILING_SUPPORTED
    
    #ifdef DEBUGGING_SUPPORTED
        // Modules take the DebuggerAssemblyControlFlags down from its
        // parent Assembly initially.
        module->SetDebuggerInfoBits(GetDebuggerInfoBits());

        LOG((LF_CORDB, LL_INFO10, "Module %S: bits=0x%x\n",
             module->GetFileName(),
             module->GetDebuggerInfoBits()));
    #endif // DEBUGGING_SUPPORTED

    #ifdef _DEBUG
        // Force the CodeBase to be found in debug mode.
        LPWSTR pName;
        DWORD  dwLength;
        hr = GetCodeBase(&pName, &dwLength);
    #endif

    } 
    COMPLUS_CATCH 
    {
        OBJECTREF Throwable = NULL;
        GCPROTECT_BEGIN( Throwable );
        Throwable = GETTHROWABLE();
        hr = SecurityHelper::MapToHR(Throwable);
        if (pThrowable != NULL) 
            *pThrowable = Throwable;
        GCPROTECT_END();
    } 
    COMPLUS_END_CATCH

    ENDCANNOTTHROWCOMPLUSEXCEPTION();
    return hr;
}

    
// Returns security information for the assembly based on the codebase
HRESULT Assembly::GetSecurityIdentity(LPWSTR *ppCodebase, DWORD *pdwZone, BYTE *pbUniqueID, DWORD *pcbUniqueID)
{
    HRESULT hr = S_OK;

    DWORD dwCodebase = 0;
    
    BEGIN_ENSURE_PREEMPTIVE_GC();
    
    hr = GetCodeBase(ppCodebase, &dwCodebase);
    if(SUCCEEDED(hr) && dwCodebase) {
        *pdwZone = Security::QuickGetZone( *ppCodebase );

    }


    END_ENSURE_PREEMPTIVE_GC();

    return hr;
}

Module* Assembly::FindAssembly(BYTE *pBase)
{
    
    if(m_pManifest && m_pManifest->GetILBase() == pBase)
        return m_pManifest;
    else
        return NULL;
}
       
Module* Assembly::FindModule(BYTE *pBase)
{
    _ASSERTE(m_pClassLoader);
    
    if(m_pManifest && m_pManifest->GetILBase() == pBase)
        return m_pManifest;

    
    Module* pModule = m_pClassLoader->m_pHeadModule;
    while (pModule)
    {
        if (pModule->GetILBase() == pBase)
            break;
        pModule = pModule->GetNextModule();
    }
    return pModule;
}


TypeHandle Assembly::LookupTypeHandle(NameHandle* pName, 
                                      OBJECTREF* pThrowable)
{
    return m_pClassLoader->LookupTypeHandle(pName, pThrowable);
}


TypeHandle Assembly::GetInternalType(NameHandle* typeName, BOOL bThrowOnError,
                                     OBJECTREF *pThrowable)
{
    THROWSCOMPLUSEXCEPTION();

    TypeHandle typeHnd;       
    HENUMInternal phEnum;
    HRESULT hr;
    //_ASSERTE(pThrowableAvailable(pThrowable));

    // load a file that hasn't been loaded, then check if the type's there
    if (FAILED(hr = m_pManifestImport->EnumInit(mdtFile,
                                                mdTokenNil,
                                                &phEnum))) {
        if (bThrowOnError) {
            DEBUG_SAFE_TO_THROW_IN_THIS_BLOCK;
            COMPlusThrowHR(hr);
        }
        return typeHnd;
    }

    bool FileSkipped = false;
    mdToken mdFile;
    while (m_pManifestImport->EnumNext(&phEnum, &mdFile)) {
        if (m_pManifest->LookupFile(mdFile))
            FileSkipped = true;
        else{
            if (FAILED(LoadInternalModule(mdFile,
                                          m_pManifestImport,
                                          NULL, // ppModule
                                          pThrowable))) {
                m_pManifestImport->EnumClose(&phEnum);
                //if (pThrowableAvailable(pThrowable) && bThrowOnError) {
                //DEBUG_SAFE_TO_THROW_IN_THIS_BLOCK;
                    COMPlusThrow(*pThrowable);
                    //}
                return typeHnd;
            }

            typeHnd = FindNestedTypeHandle(typeName, pThrowable);
            if (! (typeHnd.IsNull() && (*pThrowable == NULL)) )
                goto exit;
            
            FileSkipped = false;
        }
    }

    // check the available type table again, just in case it
    // was another thread that had added the skipped file
    if (FileSkipped)
        typeHnd = FindNestedTypeHandle(typeName, pThrowable);

 exit:
    m_pManifestImport->EnumClose(&phEnum);
    return typeHnd;
}


TypeHandle Assembly::FindNestedTypeHandle(NameHandle* typeName,
                                          OBJECTREF *pThrowable)
{
    //_ASSERTE(pThrowableAvailable(pThrowable));

    // Reset pThrowable to NULL before we do the look up.
    *pThrowable = NULL;

    TypeHandle typeHnd = LookupTypeHandle(typeName, pThrowable);

    if ((*pThrowable == NULL) && typeHnd.IsNull()) {
        char *plus;
        LPCSTR szCurrent = typeName->GetName();
        NameHandle nestedTypeName(*typeName);
        nestedTypeName.SetTypeToken(m_pManifest, mdtBaseType);

        // Find top-level type, then nested type beneath it, then nested type
        // beneath that...
        while ((plus = (char*) FindNestedSeparator(szCurrent)) != NULL) {
            *plus = '\0';

            typeHnd = LookupTypeHandle(&nestedTypeName, pThrowable);
            *plus = NESTED_SEPARATOR_CHAR;

            if (typeHnd.IsNull())
                return typeHnd;

            szCurrent = plus+1;
            nestedTypeName.SetName(NULL, szCurrent);
        }

        // Now find the nested type we really want
        if (szCurrent != typeName->GetName())
            typeHnd = LookupTypeHandle(&nestedTypeName, pThrowable);
    }

    return typeHnd;
}


// foo+bar means nested type "bar" with enclosing type "foo"
// foo\+bar means non-nested type with name "foo+bar"
// Returns pointer to first '+' that separates an enclosing and nested type.
/* static */
LPCSTR Assembly::FindNestedSeparator(LPCSTR szClassName)
{
    char *plus;
    char *slash;
    BOOL fEvenSlashCount;

    // If name begins with '+', encloser can't have "" name, so not nested
    if ( (plus = strchr(szClassName, NESTED_SEPARATOR_CHAR)) != NULL &&
         (plus != szClassName) ) {

        // ignore +'s with odd # of preceding backslashes
        while ((slash = plus) != NULL) {
            fEvenSlashCount = TRUE;

            while ( (--slash >= szClassName) &&
                    (*slash == BACKSLASH_CHAR) )
                fEvenSlashCount = !fEvenSlashCount;

            if (fEvenSlashCount) // '+' without matching backslash
                return plus;

            plus = strchr(plus+1, NESTED_SEPARATOR_CHAR);
        }
    }

    return NULL;
}


CorModule* Assembly::SetDynamicModule(CorModule* module, BOOL fNeedSecurity)
{
    _ASSERTE(m_pDynamicCode == NULL);  // For now we can not handle multiple dynamic modules
    CorModule* current = m_pDynamicCode;
    m_pDynamicCode = module;

    Module* pMod = m_pDynamicCode->GetModule();
    _ASSERTE(pMod && "Dynamic module must have a Module assocaited with it");
    AddModule(pMod, mdFileNil, fNeedSecurity);
    
    return current;
}

HRESULT Assembly::ExecuteMainMethod(PTRARRAYREF *stringArgs)
{
    HRESULT     hr;
    if (FAILED(hr = GetEntryPoint(&m_pEntryPoint)))
        return hr;
    return GetLoader()->ExecuteMainMethod(m_pEntryPoint, stringArgs);
}

HRESULT Assembly::GetEntryPoint(Module **ppModule)
{
    HRESULT     hr = S_OK;

    _ASSERTE(ppModule);

    if(!m_pEntryPoint) {
        if ((TypeFromToken(m_tEntryModule) == mdtFile) &&
            (m_tEntryModule != mdFileNil))
        {
            BEGIN_ENSURE_COOPERATIVE_GC();
            OBJECTREF throwable = NULL;
            GCPROTECT_BEGIN (throwable);
            
            Thread* pThread=GetThread();
            pThread->SetRedirectingEntryPoint();

            if (FAILED(hr = FindInternalModule(m_tEntryModule, 
                                               tdNoTypes,
                                               &m_pEntryPoint, 
                                               &throwable)))
            {
                if (throwable != NULL)
                    DefaultCatchHandler(&throwable);
                else
                    GetManifestModule()->DisplayFileLoadError(hr);
            }

            pThread->ResetRedirectingEntryPoint();

            GCPROTECT_END ();
            END_ENSURE_COOPERATIVE_GC();

            if (FAILED(hr))
                return hr;
        }

        if(m_pEntryPoint == NULL) 
            return E_FAIL;
    }
    *ppModule = m_pEntryPoint;
    return hr;
}

/* static */
BOOL Assembly::ModuleFound(HRESULT hr)
{
    switch (hr) {
    case HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND):
    case HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND):
    case HRESULT_FROM_WIN32(ERROR_INVALID_NAME):
    case HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND):
    case HRESULT_FROM_WIN32(ERROR_BAD_NET_NAME):
    case HRESULT_FROM_WIN32(ERROR_BAD_NETPATH):
    case HRESULT_FROM_WIN32(ERROR_NOT_READY):
    case HRESULT_FROM_WIN32(ERROR_WRONG_TARGET_NAME):
        return FALSE;
    default:
        return TRUE;
    }
}


HRESULT Assembly::GetFileFromFusion(LPWSTR      pwModuleName,
                                    WCHAR*      szPath,
                                    DWORD       dwPath)
{

    if(pwModuleName == NULL || *pwModuleName == L'\0') 
        return HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
       
    AppDomain* pAppDomain = SystemDomain::GetCurrentDomain();
    _ASSERTE(pAppDomain);
    if(!m_pManifestFile)
        return COR_E_FILENOTFOUND;
    
    _ASSERTE(GetFusionAssembly());

    IAssemblyModuleImport* pImport = NULL;
    HRESULT hr = GetFusionAssembly()->GetModuleByName(pwModuleName, &pImport);
    if(FAILED(hr)) 
        goto exit;

    if(pImport->IsAvailable())
        hr = pImport->GetModulePath(szPath,
                                    &dwPath);
    else {
        _ASSERTE(pAppDomain && "All assemblies must be associated with a domain");
        
        IApplicationContext *pFusionContext = pAppDomain->GetFusionContext();
        _ASSERTE(pFusionContext);
        
        AssemblySink* pSink = NULL;
        pSink = pAppDomain->GetAssemblySink();
        if(!pSink) {
            hr = E_OUTOFMEMORY;
            goto exit;
        }
        
        IAssemblyModuleImport* pResult = NULL;
        hr = FusionBind::RemoteLoadModule(pFusionContext, 
                                          pImport, 
                                          pSink,
                                          &pResult);
        pSink->Release();
        if(FAILED(hr)) 
            goto exit;
        
        _ASSERTE(pResult);
        hr = pResult->GetModulePath(szPath,
                                    &dwPath);
        pResult->Release();
    }

 exit:
    if(pImport) 
        pImport->Release();

    return hr;
}

Module* Assembly::RaiseModuleResolveEvent(LPCSTR szName, OBJECTREF *pThrowable)
{
    Module* pModule = NULL;

    BEGIN_ENSURE_COOPERATIVE_GC();

    COMPLUS_TRY {
        MethodDesc *pMD = g_Mscorlib.GetMethod(METHOD__ASSEMBLY__ON_MODULE_RESOLVE);

        struct _gc {
            OBJECTREF AssemblyRef;
            STRINGREF str;
        } gc;
        ZeroMemory(&gc, sizeof(gc));
            
        GCPROTECT_BEGIN(gc);
        if ((gc.AssemblyRef = GetRawExposedObject()) != NULL) {
            gc.str = COMString::NewString(szName);
            ARG_SLOT args[2] = {
                ObjToArgSlot(gc.AssemblyRef),
                ObjToArgSlot(gc.str)
            };
            REFLECTMODULEBASEREF ResultingModuleRef = 
                (REFLECTMODULEBASEREF) ArgSlotToObj(pMD->Call(args, METHOD__ASSEMBLY__ON_MODULE_RESOLVE));
            if (ResultingModuleRef != NULL)
                pModule = (Module*) ResultingModuleRef->GetData();
        }
        GCPROTECT_END();
    }
    COMPLUS_CATCH {
        if (pThrowable) *pThrowable = GETTHROWABLE();
    } COMPLUS_END_CATCH
          
    END_ENSURE_COOPERATIVE_GC();

    return pModule;
}

/*
  // The enum for dwLocation from managed code:
    public enum ResourceLocation
    {
        Embedded = 1,
        ContainedInAnotherAssembly = 2,
        ContainedInManifestFile = 4
    }
*/
HRESULT Assembly::GetResource(LPCSTR szName, HANDLE *hFile, DWORD *cbResource,
                              PBYTE *pbInMemoryResource, Assembly** pAssemblyRef,
                              LPCSTR *szFileName, DWORD *dwLocation, 
                              StackCrawlMark *pStackMark, BOOL fSkipSecurityCheck)
{
    mdToken            mdLinkRef;
    DWORD              dwResourceFlags;
    DWORD              dwOffset;
    mdManifestResource mdResource;
    HRESULT            hr;
    Assembly*          pAssembly = NULL;

    _ASSERTE(m_pManifestImport || "This assert normally means that mscorlib didn't build properly. Try doing a clean build in the BCL directory and verify the build log to ensure that it built cleanly.");
    if (!m_pManifestImport)
        return E_FAIL;

    if (SUCCEEDED(hr = m_pManifestImport->FindManifestResourceByName(szName,
                                                                     &mdResource)))
        pAssembly = this;
    else {
        AppDomain* pDomain = SystemDomain::GetCurrentDomain();
        _ASSERTE(pDomain);

        if (((BaseDomain*)pDomain) == SystemDomain::System())
            return E_FAIL;

        OBJECTREF Throwable = NULL;
        GCPROTECT_BEGIN(Throwable);
        pAssembly = pDomain->RaiseResourceResolveEvent(szName, &Throwable);
        if (!pAssembly) {
            if (Throwable != NULL)
                hr = SecurityHelper::MapToHR(Throwable);
        }
        GCPROTECT_END();
        if (!pAssembly)
            return hr;
      
        if (FAILED(hr = pAssembly->m_pManifestImport->FindManifestResourceByName(szName,
                                                                                 &mdResource)))
            return hr;
        if (dwLocation) {
            if (pAssemblyRef)
                *pAssemblyRef = pAssembly;
            
            *dwLocation = *dwLocation | 2; // ResourceLocation.containedInAnotherAssembly
        }
    }

    pAssembly->m_pManifestImport->GetManifestResourceProps(mdResource,
                                                           NULL, //&szName,
                                                           &mdLinkRef,
                                                           &dwOffset,
                                                           &dwResourceFlags);

    switch(TypeFromToken(mdLinkRef)) {
    case mdtAssemblyRef:
        Assembly *pFoundAssembly;
        if (FAILED(hr = pAssembly->FindExternalAssembly(m_pManifest,
                                                        mdLinkRef,
                                                        m_pManifestImport,
                                                        tdNoTypes,
                                                        &pFoundAssembly,
                                                        NULL)))
            return hr;

        if (dwLocation) {
            if (pAssemblyRef)
                *pAssemblyRef = pFoundAssembly;

            *dwLocation = *dwLocation | 2; // ResourceLocation.containedInAnotherAssembly
        }

        return pFoundAssembly->GetResource(szName,
                                           hFile,
                                           cbResource,
                                           pbInMemoryResource,
                                           pAssemblyRef,
                                           szFileName,
                                           dwLocation,
                                           pStackMark,
                                           fSkipSecurityCheck);

    case mdtFile:
        if (mdLinkRef == mdFileNil) {
            // The resource is embedded in the manifest file

            if (!IsMrPublic(dwResourceFlags) && pStackMark && !fSkipSecurityCheck) {
                Assembly *pCallersAssembly = SystemDomain::GetCallersAssembly(pStackMark);
                if (! ((!pCallersAssembly) || // full trust for interop
                       (pCallersAssembly == pAssembly) ||
                       (AssemblyNative::HaveReflectionPermission(FALSE))) )
                return CLDB_E_RECORD_NOTFOUND;
            }

            if (dwLocation) {
                *dwLocation = *dwLocation | 5; // ResourceLocation.embedded |
                                               // ResourceLocation.containedInManifestFile
                return S_OK;
            }

            return GetEmbeddedResource(pAssembly->m_pManifest, dwOffset, hFile,
                                       cbResource, pbInMemoryResource);
        }

        // The resource is either linked or embedded in a non-manifest-containing file
        return pAssembly->GetResourceFromFile(mdLinkRef, szName, hFile, cbResource,
                                              pbInMemoryResource, szFileName,
                                              dwLocation, IsMrPublic(dwResourceFlags), 
                                              pStackMark, fSkipSecurityCheck);

    default:
        STRESS_ASSERT(0);
        BAD_FORMAT_ASSERT(!"Invalid token saved in ManifestResource");
        return COR_E_BADIMAGEFORMAT;
    }
}


/* static */
HRESULT Assembly::GetEmbeddedResource(Module *pModule, DWORD dwOffset, HANDLE *hFile,
                                      DWORD *cbResource, PBYTE *pbInMemoryResource)
{   
    DWORD dwResourceSize;

    PEFile *pFile = pModule->GetPEFile();
    if (!pFile)
        return COR_E_NOTSUPPORTED;

    IMAGE_COR20_HEADER *Header = pFile->GetCORHeader();

    if (dwOffset > VAL32(Header->Resources.Size) - sizeof(DWORD))
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

    HANDLE hTempFile;
    LPCWSTR wszPath = pFile->GetFileName();
    if (hFile && wszPath && *wszPath) {
        hTempFile = VMWszCreateFile(wszPath,
                                    GENERIC_READ,
                                    FILE_SHARE_READ,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                                    NULL);
        if (hTempFile == INVALID_HANDLE_VALUE)
            return HRESULT_FROM_WIN32(GetLastError());
    }
    else {
        if (hFile)
            *hFile = INVALID_HANDLE_VALUE;

        DWORD *pdwSize;

        // base + RVA to resource blob + offset to this resource
        pdwSize = (DWORD*) (pFile->RVAToPointer(VAL32(Header->Resources.VirtualAddress)) + dwOffset);

        dwResourceSize = GET_UNALIGNED_VAL32(pdwSize);

        if (dwResourceSize > VAL32(Header->Resources.Size) - dwOffset - sizeof(DWORD))
            return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

        *pbInMemoryResource = (PBYTE) pdwSize + sizeof(DWORD);
        *cbResource = dwResourceSize;
        return S_OK;
    }


    
    // The manifest file is mapped as an exe, with an RVA for the VA.
    // dwOffset is from the RVA saved in Manifest.VA
    DWORD dwResourceOffset;
    DWORD dwFileLen = SafeGetFileSize(hTempFile, 0);
    if (dwFileLen == 0xFFFFFFFF) {
        HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
        CloseHandle(hTempFile);
        return hr;
    }
    if (0 == (dwResourceOffset = Cor_RtlImageRvaToOffset(pFile->GetNTHeader(),
                                                         VAL32(Header->Resources.VirtualAddress),
                                                         dwFileLen))) {
        CloseHandle(hTempFile);
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
    }

    if (SetFilePointer(hTempFile,
                       dwResourceOffset + dwOffset,
                       NULL,
                       FILE_BEGIN) == 0xFFFFFFFF) {
        CloseHandle(hTempFile);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    DWORD lgth;

    if (!ReadFile(hTempFile, &dwResourceSize, sizeof(DWORD), &lgth, NULL)) {
        CloseHandle(hTempFile);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    dwResourceSize = VAL32(dwResourceSize);
    if (dwResourceSize > VAL32(Header->Resources.Size) - dwOffset - sizeof(DWORD)) {
        CloseHandle(hTempFile);
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
    }

    *cbResource = dwResourceSize;
    *hFile = hTempFile;
    return S_OK;
}


HRESULT Assembly::GetResourceFromFile(mdFile mdResFile, LPCSTR szResName, HANDLE *hFile,
                                      DWORD *cbResource, PBYTE *pbInMemoryResource,
                                      LPCSTR *szFileName, DWORD *dwLocation,
                                      BOOL fIsPublic, StackCrawlMark *pStackMark,
                                      BOOL fSkipSecurityCheck)
{
    const char *szName;
    DWORD      cbHash;
    PBYTE      pbHash;
    HRESULT    hr;
    DWORD      dwFlags;
    Module     *pModule = NULL;
    DWORD      dwOffset = 0;

    m_pManifestImport->GetFileProps(mdResFile,
                                    &szName,
                                    (const void **) &pbHash,
                                    &cbHash,
                                    &dwFlags);

    if (IsFfContainsMetaData(dwFlags)) {
        // The resource is embedded in a non-manifest-containing file.
        mdManifestResource mdResource;
        mdToken mdLinkRef;
        DWORD dwResourceFlags;

        if (FAILED(hr = FindInternalModule(mdResFile,
                                           tdNoTypes,
                                           &pModule,
                                           NULL)))
            return hr;

        if (FAILED(hr = pModule->GetMDImport()->FindManifestResourceByName(szResName,
                                                                           &mdResource)))
            return hr;

        pModule->GetMDImport()->GetManifestResourceProps(mdResource,
                                                         NULL, //&szName,
                                                         &mdLinkRef,
                                                         &dwOffset,
                                                         &dwResourceFlags);
        _ASSERTE(mdLinkRef == mdFileNil);
        if (mdLinkRef != mdFileNil) {
            STRESS_ASSERT(0);
            BAD_FORMAT_ASSERT(!"Can't get LinkRef");
            return COR_E_BADIMAGEFORMAT;
        }
        fIsPublic = IsMrPublic(dwResourceFlags);
    }

    if (!fIsPublic && pStackMark && !fSkipSecurityCheck) {
        Assembly *pCallersAssembly = SystemDomain::GetCallersAssembly(pStackMark);
        if (! ((!pCallersAssembly) || // full trust for interop
               (pCallersAssembly == this) ||
               (AssemblyNative::HaveReflectionPermission(FALSE))) )
            return CLDB_E_RECORD_NOTFOUND;
    }

    if (IsFfContainsMetaData(dwFlags)) {
        if (dwLocation) {
            *dwLocation = *dwLocation | 1; // ResourceLocation.embedded
            *szFileName = szName;
            return S_OK;
        }

        return GetEmbeddedResource(pModule, dwOffset, hFile, cbResource,
                                   pbInMemoryResource);
    }

    // The resource is linked (it's in its own file)
    if (szFileName) {
        *szFileName = szName;
        return S_OK;        
    }

    if (hFile == NULL) {
        hr = FindInternalModule(mdResFile, 
                                tdNoTypes,
                                &pModule, 
                                NULL);
        if(hr == S_FALSE) { // resource file not yet loaded
            WCHAR pPath[MAX_PATH];
            hr = LoadInternalModule(szName,
                                    mdResFile,
                                    m_ulHashAlgId,
                                    pbHash,
                                    cbHash,
                                    dwFlags,
                                    pPath,
                                    MAX_PATH,
                                    &pModule,
                                    NULL);
        }

        if (FAILED(hr))
            return hr;
    
        *pbInMemoryResource = pModule->GetPEFile()->GetBase();
        *cbResource = pModule->GetPEFile()->GetUnmappedFileLength();
        return S_OK;
    }
    else {
        LPCWSTR pFileName = m_pManifest->GetFileName();
        WCHAR wszPath[MAX_PATH];
        DWORD lgth = (DWORD)wcslen(pFileName);

        if (lgth) {
            wcscpy(wszPath, pFileName);
            
            wchar_t* tail = wszPath+lgth; // go to one past the last character

            while(--tail != wszPath && *tail != L'\\' && *tail != L'/');
            // Add the directory divider
            if(*tail == L'\\' || *tail == L'/') tail++;
        WszMultiByteToWideChar(CP_UTF8, 0, szName, -1, tail, (int) (MAX_PATH - (tail - wszPath)));
        }
        else
            WszMultiByteToWideChar(CP_UTF8, 0, szName, -1, wszPath, MAX_PATH);
        
        HANDLE hTempFile = VMWszCreateFile(wszPath,
                                           GENERIC_READ,
                                           FILE_SHARE_READ,
                                           NULL,
                                           OPEN_EXISTING,
                                           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                                           NULL);
        if (hTempFile == INVALID_HANDLE_VALUE)
            return HRESULT_FROM_WIN32(GetLastError());
        
        DWORD dwFileLen = SafeGetFileSize(hTempFile, 0);
        if (dwFileLen == 0xFFFFFFFF) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto exit;
        }
        
	
        if (!m_pManifestFile->HashesVerified() &&
            (m_cbPublicKey ||
             m_pManifest->GetSecurityDescriptor()->IsSigned())) {
	
            if (!pbHash)
                return COR_E_MODULE_HASH_CHECK_FAILED;

            PBYTE pbResourceBlob = new (nothrow) BYTE[dwFileLen];
            if (!pbResourceBlob) {
                hr = E_OUTOFMEMORY;
                goto exit;
            }


            DWORD dwBytesRead;
            if (!ReadFile(hTempFile, pbResourceBlob, dwFileLen, &dwBytesRead, NULL)) {
                delete[] pbResourceBlob;
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto exit;
            }

            if (FAILED(hr = VerifyHash(pbResourceBlob,
                                       dwBytesRead,
                                       m_ulHashAlgId,
                                       pbHash,
                                       cbHash))) {
	      delete[] pbResourceBlob;
                goto exit;
            }

            delete[] pbResourceBlob;
            
            if (SetFilePointer(hTempFile, 0, NULL, FILE_BEGIN) == 0xFFFFFFFF) {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto exit;
            }
        }

        *cbResource = dwFileLen;
        *hFile = hTempFile;
        return S_OK;;
        
    exit:
        CloseHandle(hTempFile);
        return hr;
    }
}


//***********************************************************
// Add a typedef to the runtime TypeDef table of this assembly
//***********************************************************
void Assembly::AddType(
    Module          *pModule,
    mdTypeDef       cl)
{
    if (pModule->GetAssembly() != this)
    {
        // you cannot add a typedef outside of the assembly to the typedef table
        _ASSERTE(!"Bad usage!");
    }
    m_pClassLoader->AddAvailableClassDontHaveLock(pModule, pModule->GetClassLoaderIndex(), cl);
}



//***********************************************************
//
// get the IMetaDataAssemblyEmit for the on disk manifest.
// Note that the pointer returned is AddRefed. It is the caller's
// responsibility to release the reference.
//
//***********************************************************
IMetaDataAssemblyEmit *Assembly::GetOnDiskMDAssemblyEmitter()
{
    IMetaDataAssemblyEmit *pAssemEmitter = NULL;
    IMetaDataEmit   *pEmitter;
    HRESULT         hr;
    RefClassWriter  *pRCW;   

    _ASSERTE(m_pOnDiskManifest);

    pRCW = m_pOnDiskManifest->GetClassWriter(); 
    _ASSERTE(pRCW);

    // If the RefClassWriter has a on disk emitter, then use it rather than the in-memory emitter.
    pEmitter = pRCW->GetOnDiskEmitter();
        
    if (pEmitter == NULL)
        pEmitter = m_pOnDiskManifest->GetEmitter();

    _ASSERTE(pEmitter);

    IfFailGo( pEmitter->QueryInterface(IID_IMetaDataAssemblyEmit, (void**) &pAssemEmitter) );
    if (pAssemEmitter == NULL)
    {
        // the manifest is not writable
        _ASSERTE(!"Bad usage!");
    }
ErrExit:
    return pAssemEmitter;
}


//***********************************************************
//
// prepare saving manifest to disk.
// We will create a CorModule to store the on disk manifest.
// This CorModule will be destroyed/released when we are done emitting.
//
//***********************************************************
void Assembly::PrepareSavingManifest(ReflectionModule *pAssemblyModule)
{
    THROWSCOMPLUSEXCEPTION();

    HRESULT         hr = NOERROR;
    CorModule       *pWrite;
    IMetaDataAssemblyEmit *pAssemEmitter = NULL;
    LPWSTR          wszName = NULL;
    ASSEMBLYMETADATA assemData;
    int             len;
    CQuickBytes     qb;

    if (pAssemblyModule)
    {
        // embedded assembly
        m_pOnDiskManifest = pAssemblyModule;
        m_fEmbeddedManifest = true;
    }
    else
    {
        m_fEmbeddedManifest = false;

        pWrite = allocateReflectionModule();  
        if (!pWrite) 
            IfFailGo(E_OUTOFMEMORY);
    
        // intiailize the dynamic module
        hr = pWrite->Initialize(CORMODULE_NEW, IID_ICeeGen, IID_IMetaDataEmit);
        if (FAILED(hr)) 
            IfFailGo(E_OUTOFMEMORY);

        m_pOnDiskManifest = pWrite->GetReflectionModule();

        // make the On-Disk manifest module remember which assembly it belongs to.
        m_pOnDiskManifest->SetAssembly(this);   
        
        pWrite->Release();
    }

    pAssemEmitter = GetOnDiskMDAssemblyEmitter();

    // convert the manifest name to unicode
    _ASSERTE(m_psName);

    len = (int)strlen(m_psName);
    // Don't allocate asm name on stack since names may be very long.
    wszName = (LPWSTR) qb.Alloc((len + 1) * sizeof(WCHAR));
    len = WszMultiByteToWideChar(CP_UTF8, 0, m_psName, len+1, wszName, len+1);  

    memset(&assemData, 0, sizeof(ASSEMBLYMETADATA));

    // propagating version information 
    assemData.usMajorVersion = m_Context->usMajorVersion;
    assemData.usMinorVersion = m_Context->usMinorVersion;
    assemData.usBuildNumber = m_Context->usBuildNumber;
    assemData.usRevisionNumber = m_Context->usRevisionNumber;
    if (m_Context->szLocale)
    {
        assemData.cbLocale = (ULONG)(strlen(m_Context->szLocale) + 1);
        MAKE_WIDEPTR_FROMUTF8(wzLocale, m_Context->szLocale);
        assemData.szLocale = wzLocale;
    }


    hr = pAssemEmitter->DefineAssembly(
        m_pbPublicKey,          // [IN] Public key of the assembly.
        m_cbPublicKey,          // [IN] Count of bytes in the public key.
        m_ulHashAlgId,          // [IN] Hash Algorithm.
        wszName,                // [IN] Name of the assembly.
        &assemData,             // [IN] Assembly MetaData.
        m_dwFlags,              // [IN] Flags.
        &m_tkOnDiskManifest); // [OUT] Returned Assembly token.

ErrExit:
    if (pAssemEmitter)
        pAssemEmitter->Release();

    if (FAILED(hr))
    {
        _ASSERTE(!"Failed in prepare to save manifest!");
        FATAL_EE_ERROR();
    }
}   // Assembly::PrepareSavingManifest


//***********************************************************
//
// add a file name to the file list of this assembly. On disk only.
//
//***********************************************************
mdFile Assembly::AddFileList(LPWSTR wszFileName)
{
    THROWSCOMPLUSEXCEPTION();

    IMetaDataAssemblyEmit *pAssemEmitter = GetOnDiskMDAssemblyEmitter();
    HRESULT         hr = NOERROR;
    mdFile          fl;

    // Define File.
    IfFailGo( pAssemEmitter->DefineFile(   
        wszFileName,                // [IN] Name of the file.
        0,                          // [IN] Hash Blob.
        0,                          // [IN] Count of bytes in the Hash Blob.
        0,                          // [IN] Flags.
        &fl) );                     // [OUT] Returned File token.

ErrExit:
    if (pAssemEmitter)
        pAssemEmitter->Release();

    if (FAILED(hr))
        COMPlusThrowHR(hr);

    return fl;
}   // Assembly::AddFileList


//***********************************************************
//
// Set the hash value on a file table entry.
//
//***********************************************************
void Assembly::SetHashValue(mdFile tkFile, LPWSTR wszFullFileName)
{
    THROWSCOMPLUSEXCEPTION();

    IMetaDataAssemblyEmit *pAssemEmitter = GetOnDiskMDAssemblyEmitter();
    HRESULT         hr = NOERROR;
    BYTE            *pbHashValue = 0;
    DWORD           cbHashValue = 0;

    // Get the hash value.
    IfFailGo(GetHash(wszFullFileName, m_ulHashAlgId, &pbHashValue, &cbHashValue));

    // Set the hash blob.
    IfFailGo( pAssemEmitter->SetFileProps(
        tkFile,                 // [IN] File Token.
        pbHashValue,            // [IN] Hash Blob.
        cbHashValue,            // [IN] Count of bytes in the Hash Blob.
        (DWORD) -1));           // [IN] Flags.

    ErrExit:
    if (pAssemEmitter)
        pAssemEmitter->Release();

    if (pbHashValue)
        delete[] pbHashValue;

    if (FAILED(hr))
        COMPlusThrowHR(hr);
}   // Assembly::SetHashValue


//***********************************************************
// Add an assembly to the assemblyref list. pAssemEmitter specifies where 
// the AssemblyRef is emitted to.
//***********************************************************
mdAssemblyRef Assembly::AddAssemblyRef(Assembly *refedAssembly, IMetaDataAssemblyEmit *pAssemEmitter)
{
    THROWSCOMPLUSEXCEPTION();

    HRESULT         hr = NOERROR;
    mdAssemblyRef   ar = 0;
    BYTE           *pbPublicKeyToken = NULL;
    DWORD           cbPublicKeyToken = 0;

    if (pAssemEmitter) // We release at the end of this function
        pAssemEmitter->AddRef();
    else
        pAssemEmitter = GetOnDiskMDAssemblyEmitter();

    wchar_t wszLocale[MAX_PATH];

    _ASSERTE(refedAssembly->m_psName);
    int len = (int)strlen(refedAssembly->m_psName); 
    // Don't allocate asm name on stack since names may be very long.
    CQuickBytes qb;
    wchar_t *wszName = (LPWSTR) qb.Alloc((len + 1) * sizeof(WCHAR));
    if (wszName == NULL)
        IfFailGo(E_OUTOFMEMORY);
    len = WszMultiByteToWideChar(CP_UTF8, 0, refedAssembly->m_psName, len+1, wszName, len+1);  
    if (len == 0)
    {
        IfFailGo( HRESULT_FROM_WIN32(GetLastError()) );
    }

    ASSEMBLYMETADATA AMD;
    if (refedAssembly->m_Context) {
        AMD.usMajorVersion = refedAssembly->m_Context->usMajorVersion;
        AMD.usMinorVersion = refedAssembly->m_Context->usMinorVersion;
        AMD.usBuildNumber = refedAssembly->m_Context->usBuildNumber;
        AMD.usRevisionNumber = refedAssembly->m_Context->usRevisionNumber;

        if (refedAssembly->m_Context->szLocale) {
            AMD.cbLocale = (ULONG)strlen(refedAssembly->m_Context->szLocale) + 1;
            Wsz_mbstowcs(wszLocale, refedAssembly->m_Context->szLocale, MAX_PATH);
            AMD.szLocale = wszLocale;
        }
        else {
            AMD.cbLocale = 1;
            wszLocale[0] = L'\0';
            AMD.szLocale = wszLocale;
        }
            
        AMD.rProcessor = NULL;
        AMD.ulProcessor = 0;
        AMD.rOS = NULL;
        AMD.ulOS = 0;
    }
    else
        ZeroMemory(&AMD, (sizeof(ASSEMBLYMETADATA)));


    pbPublicKeyToken = refedAssembly->m_pbPublicKey;
    cbPublicKeyToken = refedAssembly->m_cbPublicKey;

    if (cbPublicKeyToken) {
        if (refedAssembly->m_cbRefedPublicKeyToken == 0)
        {
            // Compress public into a token (truncated hashed version).
            // Need to switch into GC pre-emptive mode for this call since
            // it might perform a load library (don't need to bother for
            // further StrongName calls since all library loading will be finished).
            Thread *pThread = GetThread();
            pThread->EnablePreemptiveGC();
            if (!StrongNameTokenFromPublicKey(refedAssembly->m_pbPublicKey,
                                              refedAssembly->m_cbPublicKey,
                                              &pbPublicKeyToken,
                                              &cbPublicKeyToken)) {
                hr = StrongNameErrorInfo();
                pThread->DisablePreemptiveGC();
                goto ErrExit;
            }

            // Cache the public key token for the referenced assembly so that we
            // won't recalculate if we reference to this assembly again.
            refedAssembly->m_pbRefedPublicKeyToken = pbPublicKeyToken;
            refedAssembly->m_cbRefedPublicKeyToken = cbPublicKeyToken;
            pThread->DisablePreemptiveGC();
        }
        else
        {
            // We have calculated the public key token for this referenced
            // assembly before. Just use it.
            pbPublicKeyToken = refedAssembly->m_pbRefedPublicKeyToken;
            cbPublicKeyToken = refedAssembly->m_cbRefedPublicKeyToken;
        }
    }

    IfFailGo( pAssemEmitter->DefineAssemblyRef(pbPublicKeyToken,
                                               cbPublicKeyToken,
                                               wszName,
                                               &AMD,
                                               NULL,
                                               0,
                                               refedAssembly->m_dwFlags & ~afPublicKey,
                                               &ar) );
    
ErrExit:
    if (pAssemEmitter)
        pAssemEmitter->Release();

    if (FAILED(hr)) {
        _ASSERTE(!"Failed in DefineAssemblyRef to save to disk!");
        FATAL_EE_ERROR();
    }
    return ar;
}   // Assembly::AddAssemblyRef


//***********************************************************
// Initialize an AssemblySpec from the Assembly data.
//***********************************************************
HRESULT Assembly::GetAssemblySpec(AssemblySpec *pSpec)
{
    HRESULT     hr;
    
    BYTE *pbPublicKeyToken = NULL;
    DWORD cbPublicKeyToken = 0;

    if (m_cbPublicKey) {
        // Compress public key into a token (truncated hashed version).
        // Need to switch into GC pre-emptive mode for this call since
        // it might perform a load library (don't need to bother for
        // further StrongName calls since all library loading will be finished).
        Thread *pThread = GetThread();
        BOOLEAN bGCWasDisabled = pThread && pThread->PreemptiveGCDisabled();
        if (bGCWasDisabled)
            pThread->EnablePreemptiveGC();
        if (!StrongNameTokenFromPublicKey(m_pbPublicKey,
                                          m_cbPublicKey,
                                          &pbPublicKeyToken,
                                          &cbPublicKeyToken)) {
            if (bGCWasDisabled)
                pThread->DisablePreemptiveGC();
            IfFailGo(StrongNameErrorInfo());
        }
        if (bGCWasDisabled)
            pThread->DisablePreemptiveGC();
    }

    IfFailGo(pSpec->Init(m_psName, 
                         m_Context, 
                         pbPublicKeyToken, cbPublicKeyToken, 
                         m_dwFlags & ~afPublicKey));

    pSpec->CloneFields(pSpec->PUBLIC_KEY_OR_TOKEN_OWNED);
        
ErrExit:

    if (pbPublicKeyToken)
        StrongNameFreeBuffer(pbPublicKeyToken);

    return hr;        
} // HRESULT Assembly::GetAssemblySpec()


//***********************************************************
// add a Type name to COMType table. On disk only.
//***********************************************************
mdExportedType Assembly::AddExportedType(LPWSTR wszExportedType, mdToken tkImpl, mdToken tkTypeDef, CorTypeAttr flags)
{
    THROWSCOMPLUSEXCEPTION();

    IMetaDataAssemblyEmit *pAssemEmitter = GetOnDiskMDAssemblyEmitter();
    HRESULT         hr = NOERROR;
    mdExportedType       ct;
    mdTypeDef       tkType = tkTypeDef;

    if (RidFromToken(tkTypeDef) == 0)
        tkType = mdTypeDefNil;

    IfFailGo( pAssemEmitter->DefineExportedType(   
        wszExportedType,            // [IN] Name of the COMType.
        tkImpl,                     // [IN] mdFile or mdAssemblyRef that provides the ExportedType.
        tkType,                     // [IN] TypeDef token within the file.
        flags,                      // [IN] Flags.
        &ct) );                     // [OUT] Returned ExportedType token.

ErrExit:
    if (pAssemEmitter)
        pAssemEmitter->Release();

    if (FAILED(hr))
    {
        FATAL_EE_ERROR();
    }
    return ct;
}   // Assembly::AddExportedType



//***********************************************************
// add an entry to ManifestResource table for a stand alone managed resource. On disk only.
//***********************************************************
void Assembly::AddStandAloneResource(LPWSTR wszName, LPWSTR wszDescription, LPWSTR wszMimeType, LPWSTR wszFileName, LPWSTR wszFullFileName, int iAttribute)
{
    THROWSCOMPLUSEXCEPTION();

    IMetaDataAssemblyEmit *pAssemEmitter = GetOnDiskMDAssemblyEmitter();
    HRESULT         hr = NOERROR;
    mdFile          tkFile;
    mdManifestResource mr;
    BYTE            *pbHashValue = 0;
    DWORD           cbHashValue = 0;

    // Get the hash value;
    if (m_ulHashAlgId)
        IfFailGo(GetHash(wszFullFileName, m_ulHashAlgId, &pbHashValue, &cbHashValue));

    IfFailGo( pAssemEmitter->DefineFile(
        wszFileName,            // [IN] Name of the file.
        pbHashValue,            // [IN] Hash Blob.
        cbHashValue,            // [IN] Count of bytes in the Hash Blob.
        ffContainsNoMetaData,   // [IN] Flags.
        &tkFile) );             // [OUT] Returned File token.


    IfFailGo( pAssemEmitter->DefineManifestResource(     
        wszName,                // [IN] Name of the resource.
        tkFile,                 // [IN] mdFile or mdAssemblyRef that provides the resource.
        0,                      // [IN] Offset to the beginning of the resource within the file.
        iAttribute,             // [IN] Flags.
        &mr) );                 // [OUT] Returned ManifestResource token.

ErrExit:
    if (pAssemEmitter)
        pAssemEmitter->Release();

    if (pbHashValue)
        delete[] pbHashValue;

    if (FAILED(hr))
    {
        COMPlusThrowHR(hr);
    }
}   // Assembly::AddStandAloneResource


//***********************************************************
// Save security permission requests.
//***********************************************************
void Assembly::SavePermissionRequests(U1ARRAYREF orRequired,
                                      U1ARRAYREF orOptional,
                                      U1ARRAYREF orRefused)
{
    THROWSCOMPLUSEXCEPTION();

    HRESULT              hr = S_OK;
    IMetaDataEmitHelper *pEmitHelper = NULL;
    IMetaDataAssemblyEmit *pAssemEmitter = GetOnDiskMDAssemblyEmitter();

    _ASSERTE( pAssemEmitter );
    IfFailGo( pAssemEmitter->QueryInterface(IID_IMetaDataEmitHelper, (void**)&pEmitHelper) );

    if (orRequired != NULL)
        IfFailGo(pEmitHelper->AddDeclarativeSecurityHelper(m_tkOnDiskManifest,
                                                           dclRequestMinimum,
                                                           orRequired->GetDataPtr(),
                                                           orRequired->GetNumComponents(),
                                                           NULL));

    if (orOptional != NULL)
        IfFailGo(pEmitHelper->AddDeclarativeSecurityHelper(m_tkOnDiskManifest,
                                                           dclRequestOptional,
                                                           orOptional->GetDataPtr(),
                                                           orOptional->GetNumComponents(),
                                                           NULL));

    if (orRefused != NULL)
        IfFailGo(pEmitHelper->AddDeclarativeSecurityHelper(m_tkOnDiskManifest,
                                                           dclRequestRefuse,
                                                           orRefused->GetDataPtr(),
                                                           orRefused->GetNumComponents(),
                                                           NULL));

 ErrExit:
    if (pEmitHelper)
        pEmitHelper->Release();
    if (pAssemEmitter)
        pAssemEmitter->Release();
    if (FAILED(hr))
        FATAL_EE_ERROR();
}


//***********************************************************
// Allocate space for a strong name signature in the manifest
//***********************************************************
HRESULT Assembly::AllocateStrongNameSignature(ICeeFileGen  *pCeeFileGen,
                                              HCEEFILE      ceeFile)
{
    HRESULT     hr;
    HCEESECTION TData;
    DWORD       dwDataOffset;
    DWORD       dwDataLength;
    DWORD       dwDataRVA;
    VOID       *pvBuffer;

    // Calling strong name routines for the first time can cause a load library,
    // potentially leaving us with a deadlock if we're in cooperative GC mode.
    // Switch to pre-emptive mode and touch a harmless strong name routine to
    // get any load library calls out of the way without having to switch modes
    // continuously through this routine (and the two support routines that
    // follow).
    Thread *pThread = GetThread();
    if (pThread->PreemptiveGCDisabled()) {
        pThread->EnablePreemptiveGC();
        StrongNameErrorInfo();
        pThread->DisablePreemptiveGC();
    }

    // Determine size of signature blob.
    if (!StrongNameSignatureSize(m_pbPublicKey, m_cbPublicKey, &dwDataLength)) {
        hr = StrongNameErrorInfo();
        return hr;
    }

    // Allocate space for the signature in the text section and update the COM+
    // header to point to the space.
    IfFailRet(pCeeFileGen->GetIlSection(ceeFile, &TData));
    IfFailRet(pCeeFileGen->GetSectionDataLen(TData, &dwDataOffset));
    IfFailRet(pCeeFileGen->GetSectionBlock(TData, dwDataLength, 4, &pvBuffer));
    IfFailRet(pCeeFileGen->GetMethodRVA(ceeFile, dwDataOffset, &dwDataRVA));
    IfFailRet(pCeeFileGen->SetStrongNameEntry(ceeFile, dwDataLength, dwDataRVA));

    return S_OK;
}


//***********************************************************
// Strong name sign a manifest already persisted to disk
//***********************************************************
HRESULT Assembly::SignWithStrongName(LPWSTR wszFileName)
{
    HRESULT hr = S_OK;

    // If we're going to do a full signing we have a key pair either
    // in a key container or provided directly in a byte array.

    switch (m_eStrongNameLevel) {
    case SN_FULL_KEYPAIR_IN_ARRAY:
        if (!StrongNameSignatureGeneration(wszFileName, NULL, m_pbStrongNameKeyPair, m_cbStrongNameKeyPair, NULL, NULL))
            hr = StrongNameErrorInfo();
        break;

    case SN_FULL_KEYPAIR_IN_CONTAINER:
        if (!StrongNameSignatureGeneration(wszFileName, m_pwStrongNameKeyContainer, NULL, 0, NULL, NULL))
            hr = StrongNameErrorInfo();
        break;

    default:
        break;
    }

    return hr;
}


//***********************************************************
// save the manifest to disk!
//***********************************************************
void Assembly::SaveManifestToDisk(LPWSTR wszFileName, int entrypoint, int fileKind)
{
    THROWSCOMPLUSEXCEPTION();

    HRESULT         hr = NOERROR;
    HCEEFILE        ceeFile = NULL;
    ICeeFileGen     *pCeeFileGen = NULL;
    RefClassWriter  *pRCW;   
    IMetaDataEmit   *pEmitter;

    _ASSERTE( m_fEmbeddedManifest == false );

    pRCW = m_pOnDiskManifest->GetClassWriter(); 
    _ASSERTE(pRCW);

    IfFailGo( pRCW->EnsureCeeFileGenCreated() );

    pCeeFileGen = pRCW->GetCeeFileGen();
    ceeFile = pRCW->GetHCEEFILE();
    _ASSERTE(ceeFile && pCeeFileGen);

    //Emit the MetaData 
    pEmitter = m_pOnDiskManifest->GetClassWriter()->GetEmitter();
    IfFailGo( pCeeFileGen->EmitMetaDataEx(ceeFile, pEmitter) );

    // Allocate space for a strong name signature if a public key was supplied
    // (this doesn't strong name the assembly, but it makes it possible to do so
    // as a post processing step).
    if (m_cbPublicKey)
        IfFailGo(AllocateStrongNameSignature(pCeeFileGen, ceeFile));

    IfFailGo( pCeeFileGen->SetOutputFileName(ceeFile, wszFileName) );

    // the entryPoint for an assembly is a tkFile token if exist.
    if (RidFromToken(entrypoint) != mdTokenNil)
        IfFailGo( pCeeFileGen->SetEntryPoint(ceeFile, entrypoint) );
    if (fileKind == Dll) 
    {
        pCeeFileGen->SetDllSwitch(ceeFile, true);
    } 
    else 
    {
        // should have a valid entry point for applications
        if (fileKind == WindowApplication)
        {
            IfFailGo( pCeeFileGen->SetSubsystem(ceeFile, IMAGE_SUBSYSTEM_WINDOWS_GUI, CEE_IMAGE_SUBSYSTEM_MAJOR_VERSION, CEE_IMAGE_SUBSYSTEM_MINOR_VERSION) );
        }
        else
        {
            _ASSERTE(fileKind == ConsoleApplication);
            IfFailGo( pCeeFileGen->SetSubsystem(ceeFile, IMAGE_SUBSYSTEM_WINDOWS_CUI, CEE_IMAGE_SUBSYSTEM_MAJOR_VERSION, CEE_IMAGE_SUBSYSTEM_MINOR_VERSION) );
        }

    }

    //Generate the CeeFile
    IfFailGo(pCeeFileGen->GenerateCeeFile(ceeFile) );

    // Strong name sign the resulting assembly if required.
    if (m_cbPublicKey)
        IfFailGo(SignWithStrongName(wszFileName));

    // now release the m_pOnDiskManifest
ErrExit:
    pRCW->DestroyCeeFileGen();

    m_pOnDiskManifest->Destruct();
    m_pOnDiskManifest = NULL;

    if (FAILED(hr)) 
    {
        if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
        {
            SCODE       scode = HRESULT_CODE(hr);
            WCHAR       wzErrorInfo[MAX_PATH];
            WszFormatMessage(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                0, 
                hr,
                0,
                wzErrorInfo,
                MAX_PATH,
                0);
            if (IsWin32IOError(scode))
            {
                COMPlusThrowHR(COR_E_IO, wzErrorInfo);
            }
            else
            {
                COMPlusThrowHR(hr, wzErrorInfo);
            }
        }
        if (hr == CEE_E_CVTRES_NOT_FOUND)
            COMPlusThrow(kIOException, L"Argument_cvtres_NotFound");
        COMPlusThrowHR(hr);
    }
}   // Assembly::SaveManifestToDisk

//***********************************************************
// Adding a module with file name wszFileName into the file list
//***********************************************************
void Assembly::AddFileToInMemoryFileList(LPWSTR wszFileName, Module *pModule)
{
    THROWSCOMPLUSEXCEPTION();

    IMetaDataAssemblyEmit   *pAssemEmitter = NULL;
    IMetaDataEmit           *pEmitter;
    mdFile                  tkFile;
    LPCSTR                  szFileName;
    HRESULT                 hr;

    pEmitter = m_pManifest->GetEmitter();
    _ASSERTE(pEmitter);

    IfFailGo( pEmitter->QueryInterface(IID_IMetaDataAssemblyEmit, (void**) &pAssemEmitter) );
    if (pAssemEmitter == NULL)
    {
        // the manifest is not writable
        goto ErrExit;
    }

    // Define an entry in the in-memory file list for this module
    IfFailGo( pAssemEmitter->DefineFile(        
        wszFileName,                // [IN] Name of the file.
        NULL,                       // [IN] Hash Blob.
        0,                          // [IN] Count of bytes in the Hash Blob.
        ffContainsMetaData,         // [IN] Flags.
        &tkFile) );                 // [OUT] Returned File token.

    m_pManifest->GetMDImport()->GetFileProps(tkFile, &szFileName, NULL, NULL, NULL);
    
    // insert the value into manifest's look up table.
    if (!m_pAllowedFiles->InsertValue(szFileName, (HashDatum)(size_t)tkFile, FALSE))
        IfFailGo(E_OUTOFMEMORY);

    // Now make file token associate with the loaded module
    if (!m_pManifest->StoreFile(tkFile, pModule))
        IfFailGo(E_OUTOFMEMORY);

ErrExit:
    if (pAssemEmitter)
        pAssemEmitter->Release();

    if (FAILED(hr))
    {
        _ASSERTE(!"Failed in saving manifest to disk!");
        if (hr == E_OUTOFMEMORY)
            COMPlusThrowOM();
        else
            FATAL_EE_ERROR();
    }

}   // AddFileToInMemoryFileList

//***********************************************************
// Define an assembly ref. The referenced assembly is a writable version.
// It is passed in pAsmRefEmit. pAsmEmit is the manifest to be updated to
// contained the assemblyRef.
//***********************************************************
HRESULT Assembly::DefineAssemblyRef(IMetaDataAssemblyEmit* pAsmEmit,
                                    IMetaDataEmit* pAsmRefEmit,
                                    mdAssemblyRef* mdAssemblyRef)
{
    PBYTE pbMetaData;
    DWORD cbMetaData;

    HRESULT hr = pAsmRefEmit->GetSaveSize(cssAccurate, &cbMetaData);
    if (!cbMetaData)
        return hr;

    pbMetaData = new (nothrow) BYTE[cbMetaData];
    if (!pbMetaData)
        return E_OUTOFMEMORY;

    IStream *pStream;
    if (FAILED(hr = CInMemoryStream::CreateStreamOnMemory(pbMetaData, cbMetaData, &pStream))) {
        delete[] pbMetaData;
        return hr;
    }
        
    hr = pAsmRefEmit->SaveToStream(pStream, 0);
    pStream->Release();

    if (SUCCEEDED(hr))
        hr = DefineAssemblyRef(pAsmEmit, pbMetaData, cbMetaData, mdAssemblyRef);

    delete[] pbMetaData;
    return hr;
}


//***********************************************************
// Define an assembly ref. The referenced assembly is a readonly version.
// It is passed in pbMetaData and cbMetaData. pAsmEmit is the manifest to be updated to
// contained the assemblyRef.
//***********************************************************
HRESULT Assembly::DefineAssemblyRef(IMetaDataAssemblyEmit* pAsmEmit,
                                    PBYTE pbMetaData,
                                    DWORD cbMetaData,
                                    mdAssemblyRef* mdAssemblyRef)
{
    PBYTE pbHashValue = NULL;
    DWORD cbHashValue = 0;
    WCHAR wszLocale[MAX_PATH];
    PBYTE pbPublicKeyToken = NULL;
    DWORD cbPublicKeyToken = 0;

    HRESULT hr = GetHash(pbMetaData,
                         cbMetaData,
                         m_ulHashAlgId,
                         &pbHashValue,
                         &cbHashValue);
    if (FAILED(hr))
        return hr;

    _ASSERTE(m_psName);
    // Don't allocate asm name on stack since names may be very long.
    int len = (int)strlen(m_psName); 
    CQuickBytes qb;
    wchar_t *wszName = (LPWSTR) qb.Alloc((len + 1) * sizeof(WCHAR));
    if (wszName == NULL)
        return E_OUTOFMEMORY;
    WszMultiByteToWideChar(CP_UTF8, 0, m_psName, len+1, wszName, len+1);

    ASSEMBLYMETADATA AMD;
    if (m_Context) {
        AMD.usMajorVersion = m_Context->usMajorVersion;
        AMD.usMinorVersion = m_Context->usMinorVersion;
        AMD.usBuildNumber = m_Context->usBuildNumber;
        AMD.usRevisionNumber = m_Context->usRevisionNumber;

        if (m_Context->szLocale) {
            AMD.cbLocale = (ULONG)(strlen(m_Context->szLocale) + 1);
            Wsz_mbstowcs(wszLocale, m_Context->szLocale, MAX_PATH);
            AMD.szLocale = wszLocale;
        }
        else {
            AMD.cbLocale = 1;
            wszLocale[0] = L'\0';
            AMD.szLocale = wszLocale;
        }

        AMD.rProcessor = NULL;
        AMD.ulProcessor = 0;
        AMD.rOS = NULL;
        AMD.ulOS = 0;
    }
    else
        ZeroMemory(&AMD, sizeof(ASSEMBLYMETADATA));

    pbPublicKeyToken = m_pbPublicKey;
    cbPublicKeyToken = m_cbPublicKey;

    if (cbPublicKeyToken) {
        // Compress public key into a token (truncated hashed version).
        // Need to switch into GC pre-emptive mode for this call since
        // it might perform a load library (don't need to bother for
        // further StrongName calls since all library loading will be finished).
        Thread *pThread = GetThread();
        BOOLEAN bGCWasDisabled = pThread->PreemptiveGCDisabled();
        if (bGCWasDisabled)
            pThread->EnablePreemptiveGC();
        if (!StrongNameTokenFromPublicKey(m_pbPublicKey,
                                          m_cbPublicKey,
                                          &pbPublicKeyToken,
                                          &cbPublicKeyToken)) {
            delete [] pbHashValue;
            if (bGCWasDisabled)
                pThread->DisablePreemptiveGC();
            return StrongNameErrorInfo();
        }
        if (bGCWasDisabled)
            pThread->DisablePreemptiveGC();
    }

    hr = pAsmEmit->DefineAssemblyRef(pbPublicKeyToken,
                                     cbPublicKeyToken,
                                     wszName,
                                     &AMD,
                                     pbHashValue,
                                     cbHashValue,
                                     m_dwFlags & ~afPublicKey,
                                     mdAssemblyRef);

    if (pbPublicKeyToken != m_pbPublicKey)
        StrongNameFreeBuffer(pbPublicKeyToken);

    delete[] pbHashValue;
    return hr;
}

HRESULT Assembly::VerifyModule(Module* pModule)
{
    // Get a count of all the classdefs and enumerate them.
    HENUMInternal   hEnum;
    mdTypeDef td;

    HRESULT hr = pModule->GetMDImport()->EnumTypeDefInit(&hEnum);
    if(SUCCEEDED(hr))
    {
        // First verify all global functions - if there are any
        if (!VerifyAllGlobalFunctions(pModule))
            hr = E_FAIL;
        
        while (pModule->GetMDImport()->EnumTypeDefNext(&hEnum, &td))
        {
            if (!VerifyAllMethodsForClass(pModule, td, pModule->GetClassLoader()))
            {
                pModule->GetMDImport()->EnumTypeDefClose(&hEnum);
                hr = E_FAIL;
            }
        }
    }
    return hr;
}

HRESULT Assembly::VerifyAssembly()
{
     HRESULT hr1;
     HRESULT hr = S_OK;
    _ASSERTE(IsAssembly());
    _ASSERTE(m_pManifestImport);

    // Verify the module containing the manifest. There is no
    // FileRefence so will no show up in the list.
    hr1 = VerifyModule(m_pManifest);

    if (FAILED(hr1))
        hr = hr1;

    HENUMInternal phEnum;
    hr1 = m_pManifestImport->EnumInit(mdtFile,
                                     mdTokenNil,
                                     &phEnum);
    if (FAILED(hr1)) {
        hr = hr1;
    }
    else {
        mdToken mdFile;
        
        for(int i = 0; m_pManifestImport->EnumNext(&phEnum, &mdFile); i++) {
            Module* pModule;
            hr1 = FindInternalModule(mdFile, 
                                     tdNoTypes,
                                     &pModule, 
                                     NULL);
            if (FAILED(hr1)) {
                hr = hr1;
            }
            else if(hr == S_FALSE) {
                // do nothing for resource files
            }
            else {
                hr1 = VerifyModule(pModule);
                if(FAILED(hr1)) hr = hr1;
            }
        }
    }

    return hr;
}

// Return the full name of the assembly
HRESULT Assembly::GetFullName(LPCWSTR *pwsFullName)
{
    HRESULT hr = S_OK;

    _ASSERTE(pwsFullName);

    if(m_pwsFullName == NULL) {

        BOOL fReleaseIAN = FALSE;
        IAssemblyName* pFusionAssemblyName = GetFusionAssemblyName();

        // With dynamic assemblies, m_pManifestFile would be NULL.
        // GetFusionAssemblyName() could also return NULL for byte[] assemblies.
        if(!pFusionAssemblyName) {
            // Don't allocate asm name on stack since names may be very long.
            int len = (int)strlen(m_psName); 
            CQuickBytes qb;
            wchar_t *wszName = (LPWSTR) qb.Alloc((len + 1) * sizeof(WCHAR));
            if (wszName == NULL)
                return E_OUTOFMEMORY;
            WszMultiByteToWideChar(CP_UTF8, 0, m_psName, len+1, wszName, len+1);

            if (FAILED(hr = Assembly::SetFusionAssemblyName(wszName,
                                                            m_dwFlags,
                                                            m_Context,
                                                            m_pbPublicKey,
                                                            m_cbPublicKey,
                                                            &pFusionAssemblyName)))
                return hr;

            fReleaseIAN = TRUE;
        }

        DWORD cb = 0;
        pFusionAssemblyName->GetDisplayName(NULL, &cb, 0);
        if(cb) {
            LoaderHeap* pHeap = GetDomain()->GetLowFrequencyHeap();
            _ASSERTE(pHeap);
            m_pwsFullName = (LPWSTR) pHeap->AllocMem(cb * sizeof(WCHAR));
            if (m_pwsFullName) {
                if (FAILED(pFusionAssemblyName->GetDisplayName(m_pwsFullName, &cb, 0))) {
                    // if we fail then just null out the name
                    ZeroMemory(m_pwsFullName, cb * sizeof(WCHAR));
                }
            }
        }

        if (fReleaseIAN)
            pFusionAssemblyName->Release();

    }

    *pwsFullName = m_pwsFullName;

    return hr;
}

/* static */
HRESULT Assembly::SetFusionAssemblyName(LPCWSTR pSimpleName,
                                        DWORD dwFlags,
                                        AssemblyMetaDataInternal *pContext,
                                        PBYTE  pbPublicKeyOrToken,
                                        DWORD  cbPublicKeyOrToken,
                                        IAssemblyName **ppFusionAssemblyName)
{
    HRESULT hr = S_OK;

    IAssemblyName* pFusionAssemblyName = NULL;

    if (SUCCEEDED(hr = CreateAssemblyNameObject(&pFusionAssemblyName, pSimpleName, 0, NULL))) {

        if ((pContext->usMajorVersion != (USHORT) -1) &&
            FAILED(hr = pFusionAssemblyName->SetProperty(ASM_NAME_MAJOR_VERSION, &pContext->usMajorVersion, sizeof(USHORT))))
            goto exit;
            
        if ((pContext->usMinorVersion != (USHORT) -1) &&
            FAILED(hr = pFusionAssemblyName->SetProperty(ASM_NAME_MINOR_VERSION, &pContext->usMinorVersion, sizeof(USHORT))))
            goto exit;

        if ((pContext->usBuildNumber != (USHORT) -1) &&
            FAILED(hr = pFusionAssemblyName->SetProperty(ASM_NAME_BUILD_NUMBER, &pContext->usBuildNumber, sizeof(USHORT))))
            goto exit;

        if ((pContext->usRevisionNumber != (USHORT) -1) &&
            FAILED(hr = pFusionAssemblyName->SetProperty(ASM_NAME_REVISION_NUMBER, &pContext->usRevisionNumber, sizeof(USHORT))))
            goto exit;


        if (pContext->szLocale) {
            MAKE_WIDEPTR_FROMUTF8(pwLocale,pContext->szLocale);
            
            if (FAILED(hr = pFusionAssemblyName->SetProperty(ASM_NAME_CULTURE, pwLocale, (DWORD)(sizeof(WCHAR) * (wcslen(pwLocale)+1)))))
                goto exit;
        }

        if (pbPublicKeyOrToken) {
            if (cbPublicKeyOrToken && IsAfPublicKey(dwFlags))
                hr = pFusionAssemblyName->SetProperty(ASM_NAME_PUBLIC_KEY, pbPublicKeyOrToken, cbPublicKeyOrToken);
            else if (cbPublicKeyOrToken && IsAfPublicKeyToken(dwFlags))
                hr = pFusionAssemblyName->SetProperty(ASM_NAME_PUBLIC_KEY_TOKEN, pbPublicKeyOrToken, cbPublicKeyOrToken);
            else
                hr = pFusionAssemblyName->SetProperty(ASM_NAME_NULL_PUBLIC_KEY, NULL, 0);

            if (FAILED(hr))
                goto exit;
        }


        *ppFusionAssemblyName = pFusionAssemblyName;
        return S_OK;

    exit:
        pFusionAssemblyName->Release();
    }

    return hr;
}

IMetaDataAssemblyImport* Assembly::GetManifestAssemblyImport()
{
    if (!m_pManifestAssemblyImport)
    {
        // Make sure internal MD is in RW format.
        if (SUCCEEDED(Module::ConvertMDInternalToReadWrite(&m_pManifestImport)))
            GetMetaDataPublicInterfaceFromInternal((void*)m_pManifestImport, 
                                                   IID_IMetaDataAssemblyImport, 
                                                   (void **)&m_pManifestAssemblyImport);
    }

    return m_pManifestAssemblyImport;
}

// Return the friendly name of the assembly.  In legacy mode, the friendly
// name is the filename of the module containing the manifest.
HRESULT Assembly::GetName(LPCUTF8 *pszName)
{

    // This should only occur in the legacy case, in which case the name is set
    // to the filename of the module that contains the manifest
    if (!m_psName) {
        // Since there's only one module per assembly in legacy mode, this is ok.
        if (m_pClassLoader == NULL || m_pClassLoader->m_pHeadModule == NULL)
            return (E_FAIL);

        DWORD dwLength;
        m_pClassLoader->m_pHeadModule->GetFileName(NULL, 0, &dwLength);

        if (dwLength == 0) {
            // Make sure string is unique by incorporating this pointer value into string.
            HRESULT hr;
            WCHAR   wszTemplate[30];
            IfFailRet(LoadStringRC(IDS_EE_NAME_UNKNOWN_UNQ,
                                   wszTemplate,
                                   sizeof(wszTemplate)/sizeof(wszTemplate[0]),
                                   FALSE));

            wchar_t wszUniq[20];
            swprintf(wszUniq, L"%#8x", this);
            LPWSTR wszMessage = NULL;
            LPCWSTR wszArgs[3] = {wszUniq, NULL, NULL};
            DWORD res = WszFormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY|FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                         wszTemplate,
                                         0,
                                         0,
                                         (LPWSTR) &wszMessage,
                                         0,
                                         (va_list*) wszArgs);
            m_psName = new (nothrow) char[30];
            if (! (m_psName && res) ) {
                LocalFree(wszMessage);
                return E_OUTOFMEMORY;
            }

            dwLength = Wsz_wcstombs((LPSTR) m_psName, wszMessage, 30);
            LocalFree(wszMessage);
        }
        else {
            m_psName = new (nothrow) char[++dwLength];
            if (!m_psName)
                return E_OUTOFMEMORY;
        m_pClassLoader->m_pHeadModule->GetFileName((LPUTF8) m_psName, 
                                                       dwLength, &dwLength);
        }

        _ASSERTE(dwLength);
        m_FreeFlag |= FREE_NAME;
    }

    *pszName = m_psName;

    return S_OK;
}

struct LoadAssemblyHelperCallback_Args {
    LPCWSTR wszAssembly;
    Assembly **ppAssembly;
    HRESULT hr;
};

static void LoadAssemblyHelperCallback(LPVOID ptr)
{
    LoadAssemblyHelperCallback_Args *args = (LoadAssemblyHelperCallback_Args *) ptr;
    AppDomain *pDomain = GetThread()->GetDomain();
    args->hr = pDomain->LoadAssemblyHelper(args->wszAssembly,
                                           NULL,
                                           args->ppAssembly,
                                           NULL);
}

static HRESULT LoadAssemblyHelper(Thread* pThread, AppDomain *pDomain, LPCWSTR wszAssembly, Assembly **ppAssembly)
{
    HRESULT hr = S_OK;

    COMPLUS_TRY {
        LoadAssemblyHelperCallback_Args args;
        args.wszAssembly = wszAssembly;
        args.ppAssembly = ppAssembly;
        pThread->DoADCallBack(pDomain->GetDefaultContext(), LoadAssemblyHelperCallback, &args);
        hr = args.hr;
    }
    COMPLUS_CATCH {
        hr = SecurityHelper::MapToHR(GETTHROWABLE());
    } COMPLUS_END_CATCH;

    return hr;
}

HRESULT STDMETHODCALLTYPE
GetAssembliesByName(LPCWSTR  szAppBase,
                    LPCWSTR  szPrivateBin,
                    LPCWSTR  szAssemblyName,
                    IUnknown *ppIUnk[],
                    ULONG    cMax,
                    ULONG    *pcAssemblies)
{
    HRESULT hr = S_OK;

    if (g_fEEInit) {
        // Cannot call this during EE startup
        return MSEE_E_ASSEMBLYLOADINPROGRESS;
    }

    BEGINCANNOTTHROWCOMPLUSEXCEPTION();

	Thread *pThread = NULL;
    AppDomain *pDomain = NULL;

    if (!(szAssemblyName && ppIUnk && pcAssemblies))
        IfFailGo(E_POINTER);

    pThread = GetThread();
    if (!pThread)
        IfFailGo(E_UNEXPECTED);

    if (SetupThread() == NULL)
        IfFailGo(E_OUTOFMEMORY);

    if(szAppBase || szPrivateBin) {

    BOOL fGCEnabled;
    fGCEnabled = !pThread->PreemptiveGCDisabled();
        if (fGCEnabled)
            pThread->DisablePreemptiveGC();

        COMPLUS_TRY {
            MethodDesc *pMD = g_Mscorlib.GetMethod(METHOD__APP_DOMAIN__CREATE_DOMAINEX);
            struct _gc {
                STRINGREF pFriendlyName;
                STRINGREF pAppBase;
                STRINGREF pPrivateBin;
            } gc;
            ZeroMemory(&gc, sizeof(gc));
            
            GCPROTECT_BEGIN(gc);
            gc.pFriendlyName = COMString::NewString(L"GetAssembliesByName");
            if(szAppBase)
                gc.pAppBase = COMString::NewString(szAppBase);
            if(szPrivateBin)
                gc.pPrivateBin = COMString::NewString(szPrivateBin);
            
            ARG_SLOT args[5] = {
                ObjToArgSlot(gc.pFriendlyName),
                NULL,
                ObjToArgSlot(gc.pAppBase),
                ObjToArgSlot(gc.pPrivateBin),
                0,
            };
            APPDOMAINREF pDom = (APPDOMAINREF) ArgSlotToObj(pMD->Call(args, METHOD__APP_DOMAIN__CREATE_DOMAINEX));
            if (pDom == NULL)
                hr = E_FAIL;
            else {
                Context *pContext = CRemotingServices::GetServerContextForProxy((OBJECTREF) pDom);
                _ASSERTE(pContext);
                pDomain = pContext->GetDomain();
            }

            GCPROTECT_END();
        }
        COMPLUS_CATCH {
            hr = SecurityHelper::MapToHR(GETTHROWABLE());
        } COMPLUS_END_CATCH;
          
    
        if (fGCEnabled)
            pThread->EnablePreemptiveGC();
    }
    else
        pDomain = SystemDomain::System()->DefaultDomain();

    Assembly *pFoundAssembly;
    if (SUCCEEDED(hr)) {

        if (pDomain == pThread->GetDomain())
            hr = pDomain->LoadAssemblyHelper(szAssemblyName,
                                             NULL,
                                             &pFoundAssembly,
                                             NULL);
        else {
            hr = LoadAssemblyHelper(pThread, pDomain, szAssemblyName, &pFoundAssembly);
        }

        if (SUCCEEDED(hr)) {
            if (cMax < 1)
                hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            else {
                ppIUnk[0] = (IUnknown *)pFoundAssembly->GetManifestAssemblyImport();
                ppIUnk[0]->AddRef();
            }
            *pcAssemblies = 1;
        }
    }

 ErrExit:
    ENDCANNOTTHROWCOMPLUSEXCEPTION();
    return hr;
}// Used by the IMetadata API's to access an assemblies metadata. 

void PrintVerificationException(OBJECTREF pThrowable, mdTypeDef cl, MethodDesc *pMD)
{    
    _ASSERTE(pThrowable);

    // Get and display class/method info!
    GCPROTECT_BEGIN(pThrowable);

    CQuickWSTRBase message;
    message.Init();

    COMPLUS_TRY 
    {
        // Defined and set to null
        LPCUTF8 pszClassname = NULL;
        LPCUTF8 pszNamespace = NULL;
        LPCUTF8 pszMethodname = NULL;

        GetExceptionMessage(pThrowable, &message);
        
        // Display class and Method info
        if (cl != COR_GLOBAL_PARENT_TOKEN)
        {
            pMD->GetMDImport()->GetNameOfTypeDef(cl, &pszClassname, &pszNamespace);
            if (*pszNamespace)
            {
                PrintToStdOutA(pszNamespace);
                PrintToStdOutA("\\");
            }
            if (pszClassname)
            {
                PrintToStdOutA(pszClassname);
                PrintToStdOutA("::");
            }
        }
        else
        {
            PrintToStdOutA("Global Function :");
        }

        pszMethodname = pMD->GetMDImport()->GetNameOfMethodDef(pMD->GetMemberDef());    

        PrintToStdOutA(pszMethodname);
        PrintToStdOutA("() - ");
        if (message.Size() > 0)
            PrintToStdOutW(message.Ptr());
        PrintToStdOutA("\n\n");
    }
    COMPLUS_CATCH 
    {
        _ASSERTE(!"We threw an exception during verification.  Investigation needed");
    }
    COMPLUS_END_CATCH

    message.Destroy();

    FlushLogging();     // Flush any logging output
    GCPROTECT_END();
}

BOOL VerifyAllMethodsForClass(Module *pModule, mdTypeDef cl, ClassLoader *pClassLoader)
{
    BOOL retval = TRUE;
    int  j;
    EEClass *pClass;
     
    // In the case of COR_GLOBAL_PARENT_TOKEN (i.e. global functions), it is guaranteed
    // that the module has a method table or our caller will have skipped this step.
    NameHandle name(pModule, cl);
    pClass = (cl == COR_GLOBAL_PARENT_TOKEN
              ? pModule->GetMethodTable()->GetClass()
              : (pClassLoader->LoadTypeHandle(&name)).GetClass());

    if (pClass == NULL)
        return FALSE;

    g_fVerifierOff = false;

    // Verify all methods in class - excluding inherited methods
    if (pClass->GetParentClass() == NULL)
        j = 0;
    else
        j = pClass->GetParentClass()->GetNumVtableSlots();

    while (j < pClass->GetNumMethodSlots())
    {
        MethodDesc *pMD = pClass->GetUnknownMethodDescForSlot(j);   
        if (pMD == NULL)
        {
            j++;
            continue;
        }

        if (pMD->IsIL() && !pMD->IsAbstract() && !pMD->IsUnboxingStub())
        {

            COMPLUS_TRY
            {
                COR_ILMETHOD_DECODER ILHeader(pMD->GetILHeader(), pMD->GetMDImport()); 
                if (FAILED(pMD->Verify(&ILHeader, TRUE, TRUE)))
                {
                    // Get and display class/method info!
                    retval = FALSE;
                }
            }
            COMPLUS_CATCH
            {
                PrintVerificationException(GETTHROWABLE(), cl, pMD);
                retval = FALSE;
            } COMPLUS_END_CATCH
        }

        j++;
    }
    return retval;
}

// Helper function to verify the global functions
BOOL VerifyAllGlobalFunctions(Module *pModule)
{
    // Is there anything worth verifying?
    if (pModule->GetMethodTable())
    {
        if (!VerifyAllMethodsForClass(pModule, COR_GLOBAL_PARENT_TOKEN,
                                      pModule->GetClassLoader()))
        {
            return FALSE;
        }
    }
    return TRUE;
}

#ifdef DEBUGGING_SUPPORTED
BOOL Assembly::NotifyDebuggerAttach(AppDomain *pDomain, int flags, BOOL attaching)
{
    BOOL result = FALSE;

    if (!attaching && !pDomain->IsDebuggerAttached())
        return FALSE;

    if (flags & ATTACH_ASSEMBLY_LOAD)
    {
        g_pDebugInterface->LoadAssembly(pDomain, this, FALSE, attaching);
        result = TRUE;
    }

    ClassLoader* pLoader = GetLoader();
    if (pLoader)
    {
        for (Module *pModule = pLoader->m_pHeadModule;
             pModule;
             pModule = pModule->GetNextModule())
        {
            result = pModule->NotifyDebuggerAttach(pDomain, flags, attaching) || result;
        }
    }


    return result;
}

void Assembly::NotifyDebuggerDetach(AppDomain *pDomain)
{

    if (!pDomain->IsDebuggerAttached())
        return;

    ClassLoader* pLoader = GetLoader();
    if (pLoader)
    {
        for (Module *pModule = pLoader->m_pHeadModule;
             pModule;
             pModule = pModule->GetNextModule())
        {
            pModule->NotifyDebuggerDetach(pDomain);
        }
    }

    g_pDebugInterface->UnloadAssembly(pDomain, this);

}
#endif // DEBUGGING_SUPPORTED

static BOOL CompareBases(UPTR u1, UPTR u2)
{
    BYTE *b1 = (BYTE *) (u1 << 1);
    BYTE *b2 = (BYTE *) u2;

    return b1 == b2;
}

HRESULT Assembly::ComputeBindingDependenciesClosure(PEFileBinding **ppDeps,
                                                    DWORD *pcDeps, 
                                                    BOOL suppressLoads)
{
    HRESULT hr = S_OK;

    AppDomain *pAppDomain = SystemDomain::GetCurrentDomain();

    AssemblySpecHash specHash;
    PtrHashMap manifestHash;
    manifestHash.Init(CompareBases, FALSE, NULL);

    DWORD depsEnd = 0;
    DWORD depsMax = 10;
    PEFileBinding *deps = (PEFileBinding *) 
      _alloca(depsMax * sizeof(PEFileBinding));

    DWORD peFileVisited = 0;
    DWORD peFileEnd = 0;
    DWORD peFileMax = 10;
    PEFile **peFiles = (PEFile **)
        _alloca(peFileMax * sizeof(PEFile *));

    peFiles[peFileEnd++] = this->GetManifestFile();

    
    BEGIN_ENSURE_COOPERATIVE_GC();
    while (peFileVisited < peFileEnd)
    {
        PEFile *pCurrentPEFile = peFiles[peFileVisited++];

        LOG((LF_CODESHARING, 
             LL_INFO100, 
             "Considering dependencies of pefile \"%S\".\n",
             pCurrentPEFile->GetFileName())); 

        IMDInternalImport *pImport = pCurrentPEFile->GetMDImport();
        HENUMInternal e;

        //
        // Enumerate all assembly refs in the manifest
        //

        pImport->EnumInit(mdtAssemblyRef, mdTokenNil, &e);

        mdAssemblyRef ar;
        while (pImport->EnumNext(&e, &ar))
        {
            AssemblySpec spec;
            spec.InitializeSpec(ar, pImport, NULL);

            // 
            // We never care about mscorlib as a dependency, since it
            // cannot be versioned.
            //

            if (spec.IsMscorlib())
                continue;


            if (!specHash.Store(&spec))
            {
                //
                // If we haven't seen this ref yet, add it to the list.
                //

                LOG((LF_CODESHARING, 
                     LL_INFO1000, 
                     "Testing dependency \"%s\" (hash %x).\n",
                     spec.GetName(), spec.Hash())); 

                PEFile *pDepFile = NULL;
                IAssembly* pIAssembly = NULL;
                OBJECTREF throwable = NULL;
                GCPROTECT_BEGIN(throwable);
                if (suppressLoads)
                {
                    //
                    // Try to find the spec in our cache; if it's not there, just skip it. 
                    // (Note that there can't be any transitive dependencies if it hasn't ever
                    // been loaded.)
                    //

                    hr = pAppDomain->LookupAssemblySpec(&spec, &pDepFile, NULL, &throwable);
                    if (hr == S_FALSE)
                    {
                        Assembly *pAssembly = pAppDomain->FindCachedAssembly(&spec);
                        if (pAssembly)
                            PEFile::Clone(pAssembly->GetManifestFile(), &pDepFile);
                        else 
                            continue;
                    }
                }
                else
                {
                    Assembly *pDynamicAssembly = NULL;
                    hr = pAppDomain->BindAssemblySpec(&spec, &pDepFile, &pIAssembly, &pDynamicAssembly, NULL, &throwable);

                    if (pDynamicAssembly)
                        continue;

                    //
                    // Store the binding in the cache, as we will probably
                    // try it again later.  Keep ownership of pDepFile,
                    // though.
                    //
                    
                    if (FAILED(hr))
                    {
                        pAppDomain->StoreBindAssemblySpecError(&spec, hr, &throwable);
                    }
                    else
                    {
                        pAppDomain->StoreBindAssemblySpecResult(&spec, pDepFile, pIAssembly);
                        pIAssembly->Release();
                    }
                }

                if (depsEnd == depsMax)
                {
                    //
                    // We need a bigger array...
                    //

                    depsMax *= 2;
                    PEFileBinding *newDeps = (PEFileBinding *) 
                        _alloca(depsMax * sizeof(PEFileBinding));
                    memcpy(newDeps, deps, depsEnd * sizeof(PEFileBinding));
                    deps = newDeps;
                }

                PEFileBinding *d = &deps[depsEnd++];
                
                d->pImport = pImport;
                d->assemblyRef = ar;
                d->pPEFile = pDepFile;

                GCPROTECT_END();

                //
                // See if we've examined this binding's manifest yet.  (Note
                // that we can have different specs which bind to the same
                // manifest.)
                //

                if (pDepFile != NULL)
                {
                    BYTE *base = pDepFile->GetBase();
                
                    if (manifestHash.LookupValue((UPTR)base, base) == (BYTE*) INVALIDENTRY)
                    {
                        manifestHash.InsertValue((UPTR)base, base);
                    
                        if (peFileEnd == peFileMax)
                        {
                            //
                            // We need a bigger array...
                            //

                            peFileMax *= 2;
                            PEFile **newPeFiles = (PEFile **) 
                              _alloca(peFileMax * sizeof(PEFile **));
                            memcpy(newPeFiles, peFiles, 
                                   peFileEnd * sizeof(PEFile **));
                            peFiles = newPeFiles;
                        }
                    
                        peFiles[peFileEnd++] = pDepFile;
                    }
                }
            }
            else
            {
                LOG((LF_CODESHARING, 
                     LL_INFO10000, 
                     "Already tested spec \"%s\" (hash %d).\n",
                     spec.GetName(), spec.Hash())); 
            }
        }
        pImport->EnumClose(&e);
    }
    END_ENSURE_COOPERATIVE_GC();

    //
    // Copy dependencies into an array we can return.
    //

    *pcDeps = depsEnd;
    *ppDeps = new (nothrow) PEFileBinding [ depsEnd ];
    if (!*ppDeps)
        return E_OUTOFMEMORY;

    memcpy(*ppDeps, deps, depsEnd * sizeof(PEFileBinding));
    return S_OK;
}

HRESULT Assembly::SetSharingProperties(PEFileBinding *pDependencies, DWORD cDependencies)
{
    _ASSERTE(m_pSharingProperties == NULL);

    PEFileSharingProperties *p = new (nothrow) PEFileSharingProperties;
    if (p == NULL)
        return E_OUTOFMEMORY;

    p->pDependencies = pDependencies;
    p->cDependencies = cDependencies;
    p->shareCount = 0;

    m_pSharingProperties = p;

    return S_OK;
}

HRESULT Assembly::GetSharingProperties(PEFileBinding **ppDependencies, DWORD *pcDependencies)
{
    if (m_pSharingProperties == NULL)
    {
        *ppDependencies = NULL;
        *pcDependencies = 0;
        return S_FALSE;
    }
    else
    {

        *ppDependencies = m_pSharingProperties->pDependencies;
        *pcDependencies = m_pSharingProperties->cDependencies;
        return S_OK;
    }
}

void Assembly::IncrementShareCount()
{
    FastInterlockIncrement(&m_pSharingProperties->shareCount);
}

void Assembly::DecrementShareCount()
{
    FastInterlockDecrement(&m_pSharingProperties->shareCount);
}

HRESULT Assembly::CanLoadInDomain(AppDomain *pAppDomain)
{
    HRESULT hr;

    // Only make sense for shared assemblies
    _ASSERTE(IsShared());

    // If we're already loaded, then of course we can load.
    if (pAppDomain->FindAssembly(this->GetManifestFile()->GetBase()) != NULL)
        return S_OK;

    // First, try to bind the assembly's spec in the given app domain.  This 
    // will discover the case where the assembly can never even be bound in the
    // domain.

    PEFile *pFile;
    IAssembly *pIAssembly;
    Assembly *pDynamicAssembly;

    AssemblySpec spec(pAppDomain);
    GetAssemblySpec(&spec);


    hr = pAppDomain->BindAssemblySpec(&spec, &pFile, &pIAssembly, &pDynamicAssembly, NULL, NULL);

    if (FAILED(hr))
    {
        // If we failed, the assembly cannot be shared in the domain.
        // Cache the error so we don't contradict this in the future.


        pAppDomain->StoreBindAssemblySpecError(&spec, hr, NULL);
        hr = S_FALSE;
    }
    else
    {
        // See if the assembly metadata bound to the expected assembly.

        pAppDomain->StoreBindAssemblySpecResult(&spec, pFile, pIAssembly);

        if (GetManifestFile()->GetBase() != pFile->GetBase())
            hr = S_FALSE;
        else
            hr = CanShare(pAppDomain, NULL, FALSE);

        delete pFile;
        if (pIAssembly)
            pIAssembly->Release();
    }

    return hr;
}

HRESULT Assembly::CanShare(AppDomain *pAppDomain, OBJECTREF *pThrowable, BOOL suppressLoads)
{
    HRESULT hr = S_OK;

    LOG((LF_CODESHARING, 
         LL_INFO100, 
         "Checking if we can share: \"%s\" in domain 0x%x.\n", 
         m_psName, pAppDomain));

    //
    // If we weren't able to compute sharing properties, err on the side of caution
    // & just fail.
    //

    if (m_pSharingProperties == NULL)
        return E_FAIL;

    //
    // Note that this routine has the side effect of loading a bunch of assemblies
    // into the domain.  However, because we visit the dependencies in sorted order, 
    // we at least can be sure that we won't load any assemblies which aren't part
    // of the dependencies of the assembly.  
    //
    // Also, since we will be sharing a different version of the assembly anyway, it 
    // won't matter because we will soon be loading all dependencies anyway. 
    //
    
    PEFileBinding *deps = m_pSharingProperties->pDependencies;
    PEFileBinding *depsEnd = deps + m_pSharingProperties->cDependencies;

    while (deps < depsEnd)
    {
        AssemblySpec spec;

        spec.InitializeSpec(deps->assemblyRef, deps->pImport, NULL);

        BOOL fail = FALSE;
        PEFile *pFile;
        Assembly *pDynamicAssembly = NULL;
        IAssembly* pIAssembly = NULL;

        if (suppressLoads)
        {
            //
            // Try to find the spec in our cache
            //

            if (pAppDomain->LookupAssemblySpec(&spec, &pFile, &pIAssembly, pThrowable) != S_OK
                || (pThrowable != NULL && *pThrowable != NULL))
                hr = E_FAIL;
        }
        else
        {
            hr = pAppDomain->BindAssemblySpec(&spec, &pFile, &pIAssembly, &pDynamicAssembly, NULL, pThrowable);
        }

        if ((FAILED(hr) != 0) // did we just fail
            != 
            (deps->pPEFile == NULL)) // did the cached binding fail
        {
            // Our error status doesn't match the error status of the dependency

            LOG((LF_CODESHARING, 
                 LL_INFO100, 
                 "%s %x binding dependent spec \"%S\": .\n", 
                 FAILED(hr) ? "Unexpected error" : "Didn't get expected error",
                 hr, spec.GetName()));

            fail = TRUE;
        } 

        if (pDynamicAssembly)
            fail = TRUE;

        if (!fail)
        {
            if (FAILED(hr))
            {
                // Expected error - match OK.  Make sure we store the error so it will
                // never succeed (which could lead to a binding mismatch between sharing 
                // domains)

                if (!suppressLoads)
                    pAppDomain->StoreBindAssemblySpecError(&spec, hr, pThrowable);
            }
            else
            {
                BYTE *pBoundBase = pFile->GetBase();
#ifdef LOGGING
                LPCWSTR pBoundName = pFile->GetFileName();
#endif

                //
                // Even if these files don't match up with the shared assembly
                // we are testing for, it is likely that they will be used by whatever 
                // version of this assembly we end up using.  Therefore, we will
                // prime our cache with the bindings so the files don't get unloaded
                // & reloaded
                //

                if (!suppressLoads)
                    pAppDomain->StoreBindAssemblySpecResult(&spec, pFile, pIAssembly);

                delete pFile;
                if(pIAssembly)
                    pIAssembly->Release();

                if (pBoundBase != deps->pPEFile->GetBase())
                {
                    LOG((LF_CODESHARING,
                         LL_INFO100, 
                         "We can't share it, \"%S\" binds to \"%S\" in the current domain: .\n", 
                         spec.GetName(), pBoundName));

                    fail = TRUE;
                }
            }
        }

        if (fail)
        {
            hr = S_FALSE;
            break;
        }

        deps++;
    }

    LOG((LF_CODESHARING, LL_INFO100, "We can %sshare it.\n", hr == S_OK ? "" : "not ")); 

    return hr;
}

#define DE_CUSTOM_VALUE_NAMESPACE        "System.Diagnostics"
#define DE_DEBUGGABLE_ATTRIBUTE_NAME     "DebuggableAttribute"

#define DE_INI_FILE_SECTION_NAME          ".NET Framework Debugging Control"
#define DE_INI_FILE_KEY_TRACK_INFO        "GenerateTrackingInfo"
#define DE_INI_FILE_KEY_ALLOW_JIT_OPTS    "AllowOptimize"

DWORD Assembly::ComputeDebuggingConfig()
{
#ifdef DEBUGGING_SUPPORTED
    bool fTrackJITInfo = false;
    bool fAllowJITOpts = true;
    bool fEnC = false;
    DWORD dacfFlags = DACF_NONE;
    bool fOverride = false;
    bool fHasBits;

    if ((fHasBits = GetDebuggingOverrides(&fTrackJITInfo, &fAllowJITOpts, &fOverride, &fEnC)) == false)
        fHasBits = GetDebuggingCustomAttributes(&fTrackJITInfo, &fAllowJITOpts, &fEnC);

    if (fHasBits)
    {
        if (fTrackJITInfo)
            dacfFlags |= DACF_TRACK_JIT_INFO;
        
        if (fAllowJITOpts)
            dacfFlags |= DACF_ALLOW_JIT_OPTS;

        if (fOverride)
            dacfFlags |= DACF_USER_OVERRIDE;

        if (fEnC)
            dacfFlags |= DACF_ENC_ENABLED;
    }
        
    return dacfFlags;
#else // !DEBUGGING_SUPPORTED
    return 0;
#endif // !DEBUGGING_SUPPORTED
}
        
void Assembly::SetupDebuggingConfig(void)
{
#ifdef DEBUGGING_SUPPORTED
    DWORD dacfFlags = ComputeDebuggingConfig();

    // If this process was launched by a debugger, then prevent optimized code from being produced
    if (CORLaunchedByDebugger())
    {
        dacfFlags &= ~DACF_ALLOW_JIT_OPTS;
        dacfFlags |= DACF_ENC_ENABLED;
    }

    SetDebuggerInfoBits((DebuggerAssemblyControlFlags)dacfFlags);

    LOG((LF_CORDB, LL_INFO10, "Assembly %S: bits=0x%x\n", GetManifestFile()->GetFileName(), GetDebuggerInfoBits()));
#endif // DEBUGGING_SUPPORTED
}

// The format for the (temporary) .INI file is:

// [.NET Framework Debugging Control]
// GenerateTrackingInfo=<n> where n is 0 or 1
// AllowOptimize=<n> where n is 0 or 1

// Where neither x nor y equal INVALID_INI_INT:
#define INVALID_INI_INT (0xFFFF)

bool Assembly::GetDebuggingOverrides(bool *pfTrackJITInfo, bool *pfAllowJITOpts, bool *pfOverride, bool *pfEnC)
{
#ifdef DEBUGGING_SUPPORTED
    _ASSERTE(pfTrackJITInfo);
    _ASSERTE(pfAllowJITOpts);
    _ASSERTE(pfEnC);

    WCHAR ConfigString[16];
    BOOL b;

    b = PAL_FetchConfigurationStringW(TRUE,
	    			      L"GenerateTrackingInfo",
				      ConfigString,
				      sizeof(ConfigString)/sizeof(WCHAR));
    if (b && _wtoi(ConfigString)) {
	*pfTrackJITInfo = true;
    } else {
	*pfTrackJITInfo = false;
    }

    b = PAL_FetchConfigurationStringW(TRUE,
	    			      L"AllowOptimize",
				      ConfigString,
				      sizeof(ConfigString)/sizeof(WCHAR));
    // Note: for V1, EnC mode is tied to whether or not optimizations are enabled.
    if (b) {
        if (_wtoi(ConfigString)) {
	    *pfAllowJITOpts = true;
            (*pfEnC) = false;
        } else {
	    *pfAllowJITOpts = false;
            (*pfEnC) = false;
	}
        *pfOverride = true;
        return true;
    }

    return false;


#else  // !DEBUGGING_SUPPORTED
    return false;
#endif // !DEBUGGING_SUPPORTED
}


// For right now, we only check to see if the DebuggableAttribute is present - later may add fields/properties to the
// attributes.
bool Assembly::GetDebuggingCustomAttributes(bool *pfTrackJITInfo, bool *pfAllowJITOpts, bool *pfEnC)
{
    _ASSERTE(pfTrackJITInfo);
    _ASSERTE(pfAllowJITOpts);
    _ASSERTE(pfEnC);

    bool fHasBits = false;

    ULONG size;
    BYTE *blob;
    mdModule mdMod;
    mdMod = GetManifestImport()->GetModuleFromScope();
    mdAssembly asTK = TokenFromRid(mdtAssembly, 1);
        
    HRESULT hr = S_OK;    
    hr = GetManifestImport()->GetCustomAttributeByName(asTK,
                                                       DE_CUSTOM_VALUE_NAMESPACE
                                                       NAMESPACE_SEPARATOR_STR
                                                       DE_DEBUGGABLE_ATTRIBUTE_NAME,
                                                       (const void**)&blob,
                                                       &size);

    // If there is no custom value, then there is no entrypoint defined.
    if (!(FAILED(hr) || hr == S_FALSE))
    {
        // We're expecting a 6 byte blob:
        //
        // 1, 0, enable tracking, disable opts, 0, 0
        if (size == 6)
        {
            _ASSERTE((blob[0] == 1) && (blob[1] == 0));
            
            (*pfTrackJITInfo) = (blob[2] != 0);
            (*pfAllowJITOpts) = (blob[3] == 0);

            // Note: for V1, EnC mode is tied to whether or not optimizations are enabled.
            (*pfEnC) = (blob[3] != 0);
            
            // We only say that we have bits if we're told to track.
            fHasBits = *pfTrackJITInfo;

            LOG((LF_CORDB, LL_INFO10, "Assembly %S: has %s=%d,%d\n",
                 GetManifestFile()->GetFileName(),
                 DE_DEBUGGABLE_ATTRIBUTE_NAME,
                 blob[2], blob[3]));
        }
    }

    return fHasBits;
}

BOOL Assembly::AllowUntrustedCaller()
{
    if (!m_fCheckedForAllowUntrustedCaller)
    {
        CheckAllowUntrustedCaller();
        m_fCheckedForAllowUntrustedCaller = TRUE;
    }

    return m_fAllowUntrustedCaller;
}


void Assembly::CheckAllowUntrustedCaller()
{
    SharedSecurityDescriptor* pSecDesc = this->GetSharedSecurityDescriptor();

    if (pSecDesc->IsSystem() || !this->IsStrongNamed())
    {
        m_fAllowUntrustedCaller = TRUE;
        return;
    }

    Module* pModule;
    mdToken token;
    TypeHandle attributeClass;

    pModule = this->GetSecurityModule();

    IMDInternalImport *mdImport = this->GetManifestImport();
    if (mdImport)
        mdImport->GetAssemblyFromScope(&token);
    else
        return;

    BEGIN_ENSURE_COOPERATIVE_GC();

    COMPLUS_TRY
    {
        MethodTable* pMethodTable = g_Mscorlib.GetClass( CLASS__ALLOW_PARTIALLY_TRUSTED_CALLER );
        _ASSERTE( pMethodTable != NULL && "System.Security.AllowPartiallyTrustedCallersAttribute must be defined in mscorlib" );
        attributeClass = TypeHandle( pMethodTable );

        m_fAllowUntrustedCaller = COMCustomAttribute::IsDefined( pModule,
                                                                 token,
                                                                 attributeClass,
                                                                 FALSE );
    }
    COMPLUS_CATCH
    {
        m_fAllowUntrustedCaller = FALSE;
    }
    COMPLUS_END_CATCH

    END_ENSURE_COOPERATIVE_GC();
}


// -- eof --


