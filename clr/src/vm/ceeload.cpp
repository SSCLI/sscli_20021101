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
// File: CEELOAD.CPP
// 

// CEELOAD reads in the PE file format using LoadLibrary
// ===========================================================================

#include "common.h"

#include "ceeload.h"
#include "hash.h"
#include "vars.hpp"
#include "reflectclasswriter.h"
#include "method.hpp"
#include "stublink.h"
#include "security.h"
#include "cgensys.h"
#include "excep.h"
#include "comclass.h"
#include "dbginterface.h"
#include "ndirect.h"
#include "wsperf.h"
#include "eeprofinterfaces.h"
#include "perfcounters.h"
#include "cormap.hpp"
#include "encee.h"
#include "jitinterface.h"
#include "reflectutil.h"
#include "eeconfig.h"
#include "peverifier.h"
#include "utsem.h"
#include "nexport.h"


#include "perflog.h"
#include "timeline.h"

#define FORMAT_MESSAGE_LENGTH       1024
#define ERROR_LENGTH                512

HRESULT STDMETHODCALLTYPE CreateICeeGen(REFIID riid, void **pCeeGen);



// {F5398690-98FE-11d2-9C56-00A0C9B7CC45}
const GUID IID_ICorReflectionModule =
{ 0xf5398690, 0x98fe, 0x11d2, { 0x9c, 0x56, 0x0, 0xa0, 0xc9, 0xb7, 0xcc, 0x45 } };




// ===========================================================================
// Module
// ===========================================================================

//
// RuntimeInit initializes only fields which are not persisted in preload files
//

HRESULT Module::RuntimeInit()
{
#ifdef PROFILING_SUPPORTED
    // If profiling, then send the pModule event so load time may be measured.
    if (CORProfilerTrackModuleLoads())
        g_profControlBlock.pProfInterface->ModuleLoadStarted((ThreadID) GetThread(), (ModuleID) this);
#endif // PROFILING_SUPPORTED

    m_pCrst = new (&m_CrstInstance) Crst("ModuleCrst", CrstModule);

    m_pLookupTableCrst = new (&m_LookupTableCrstInstance) Crst("ModuleLookupTableCrst", CrstModuleLookupTable);

#ifdef PROFILING_SUPPORTED
    // Profiler enabled, and re-jits requested?
    if (CORProfilerAllowRejit())
    {
        m_dwFlags |= SUPPORTS_UPDATEABLE_METHODS;
    }
    else
#endif // PROFILING_SUPPORTED
    {
        m_dwFlags &= ~SUPPORTS_UPDATEABLE_METHODS;
    }

#ifdef _DEBUG
    m_fForceVerify = FALSE;
#endif


    return S_OK;
}

//
// Init initializes all fields of a Module
//

HRESULT Module::Init(BYTE *ilBaseAddress)
{
    m_ilBase                    = ilBaseAddress;
    m_zapFile                   = NULL;

    // This is now NULL'd in the module's constructor so it can be set
    // before Init is called
    // m_file                      = NULL;

    m_pMDImport                 = NULL;
    m_pEmitter                  = NULL;
    m_pImporter                 = NULL;
    m_pHelper                   = NULL;
    m_pDispenser                = NULL;

    m_pDllMain                  = NULL;
    m_dwFlags                   = 0;
    m_pVASigCookieBlock         = NULL;
    m_pAssembly                 = NULL;
    m_moduleRef                 = mdFileNil;
    m_dwModuleIndex             = 0;
    m_pCrst                     = NULL;
    m_pChunks                   = NULL;
    m_pMethodTable              = NULL;
    m_pISymUnmanagedReader      = NULL;
    m_pNextModule               = NULL;
    m_dwBaseClassIndex          = 0;
    m_pPreloadRangeStart        = NULL;
    m_pPreloadRangeEnd          = NULL;
    m_pThunkTable               = NULL;
    m_ExposedModuleObject       = NULL;
    m_pLookupTableHeap          = NULL;
    m_pLookupTableCrst          = NULL;
    
    m_pIStreamSym               = NULL;
    
    // Set up tables
    ZeroMemory(&m_TypeDefToMethodTableMap, sizeof(LookupMap));
    m_dwTypeDefMapBlockSize = 0;
    ZeroMemory(&m_TypeRefToMethodTableMap, sizeof(LookupMap));
    m_dwTypeRefMapBlockSize = 0;
    ZeroMemory(&m_MethodDefToDescMap, sizeof(LookupMap));
    m_dwMethodDefMapBlockSize = 0;
    ZeroMemory(&m_FieldDefToDescMap, sizeof(LookupMap));
    m_dwFieldDefMapBlockSize = 0;
    ZeroMemory(&m_MemberRefToDescMap, sizeof(LookupMap));
    m_dwMemberRefMapBlockSize = 0;
    ZeroMemory(&m_FileReferencesMap, sizeof(LookupMap));
    m_dwFileReferencesMapBlockSize = 0;
    ZeroMemory(&m_AssemblyReferencesMap, sizeof(LookupMap));
    m_dwAssemblyReferencesMapBlockSize = 0;

    m_pBinder                   = NULL;



    m_compiledMethodRecord      = NULL;
    m_loadedClassRecord         = NULL;
    
    // Remaining inits
    return RuntimeInit();
}

HRESULT Module::Init(PEFile *pFile, PEFile *pZapFile, BOOL preload)
{
    HRESULT             hr;

    m_file = pFile;

    if (preload)
        IfFailRet(Module::RuntimeInit());
    else
        IfFailRet(Module::Init(pFile->GetBase()));

    m_zapFile = pZapFile;

    IMDInternalImport *pImport = NULL;
        pImport = pFile->GetMDImport(&hr);

    if (pImport == NULL) 
    {
        if (FAILED(hr))
            return hr;
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
    }

    pImport->AddRef();
    SetMDImport(pImport);

    LOG((LF_CLASSLOADER, LL_INFO10, "Loaded pModule: \"%ws\".\n", GetFileName()));

    if (!preload)
        IfFailRet(AllocateMaps());



    return S_OK;
}

HRESULT Module::CreateResource(PEFile *file, Module **ppModule)
{
    Module *pModule = new (nothrow) Module();
    if (pModule == NULL)
        return E_OUTOFMEMORY;

    pModule->m_file = file;

    HRESULT hr;
    IfFailRet(pModule->Init(file->GetBase()));

    pModule->SetPEFile();
    pModule->SetResource();
    *ppModule = pModule;
    return S_OK;
}

HRESULT Module::Create(PEFile *file, Module **ppModule, BOOL isEnC)
{
    HRESULT hr = S_OK;

    IfFailRet(VerifyFile(file, FALSE));

    //
    // Enable the zap monitor if appropriate
    //


    Module *pModule;

        pModule = new Module();
    
    if (pModule == NULL)
        hr = E_OUTOFMEMORY;

    IfFailGo(pModule->Init(file, NULL, false));

    pModule->SetPEFile();



    *ppModule = pModule;

ErrExit:
#ifdef PROFILING_SUPPORTED
    // When profiling, let the profiler know we're done.
    if (CORProfilerTrackModuleLoads())
        g_profControlBlock.pProfInterface->ModuleLoadFinished((ThreadID) GetThread(), (ModuleID) pModule, hr);
#endif // PROFILILNG_SUPPORTED

    return hr;
}

void Module::Unload()
{
    // Unload the EEClass*'s filled out in the TypeDefToEEClass map
    LookupMap *pMap;
    DWORD       dwMinIndex = 0;
    MethodTable *pMT;

    // Go through each linked block
    for (pMap = &m_TypeDefToMethodTableMap; pMap != NULL && pMap->pTable; pMap = pMap->pNext)
    {
        DWORD i;
        DWORD dwIterCount = pMap->dwMaxIndex - dwMinIndex;
        void **pRealTableStart = &pMap->pTable[dwMinIndex];
        
        for (i = 0; i < dwIterCount; i++)
        {
            pMT = (MethodTable *) (pRealTableStart[i]);
            
            if (pMT != NULL)
            {
                pMT->GetClass()->Unload();
            }
        }
        
        dwMinIndex = pMap->dwMaxIndex;
    }
}

void Module::UnlinkClasses(AppDomain *pDomain)
{
    // Unlink the EEClass*'s filled out in the TypeDefToEEClass map
    LookupMap *pMap;
    DWORD       dwMinIndex = 0;
    MethodTable *pMT;

    // Go through each linked block
    for (pMap = &m_TypeDefToMethodTableMap; pMap != NULL && pMap->pTable; pMap = pMap->pNext)
    {
        DWORD i;
        DWORD dwIterCount = pMap->dwMaxIndex - dwMinIndex;
        void **pRealTableStart = &pMap->pTable[dwMinIndex];
        
        for (i = 0; i < dwIterCount; i++)
        {
            pMT = (MethodTable *) (pRealTableStart[i]);
            
            if (pMT != NULL)
                pDomain->UnlinkClass(pMT->GetClass());
        }
        
        dwMinIndex = pMap->dwMaxIndex;
    }
}

//
// Destructor for Module
//

void Module::Destruct()
{
#ifdef PROFILING_SUPPORTED
    if (CORProfilerTrackModuleLoads())
        g_profControlBlock.pProfInterface->ModuleUnloadStarted((ThreadID) GetThread(), (ModuleID) this);
#endif // PROFILING_SUPPORTED

    LOG((LF_EEMEM, INFO3, 
         "Deleting module %x\n"
         "m_pLookupTableHeap:    %10d bytes\n",
         this,
         (m_pLookupTableHeap ? m_pLookupTableHeap->GetSize() : (size_t) -1)));

    // Free classes in the class table
    FreeClassTables();

#ifdef DEBUGGING_SUPPORTED
    g_pDebugInterface->DestructModule(this);
#endif // DEBUGGING_SUPPORTED

    // when destructing a module - close the scope
    ReleaseMDInterfaces();

    ReleaseISymUnmanagedReader();

    if (m_pISymUnmanagedReaderLock)
    {
        DeleteCriticalSection(m_pISymUnmanagedReaderLock);
        delete m_pISymUnmanagedReaderLock;
        m_pISymUnmanagedReaderLock = NULL;
    }

   // Clean up sig cookies
    VASigCookieBlock    *pVASigCookieBlock = m_pVASigCookieBlock;
    while (pVASigCookieBlock)
    {
        VASigCookieBlock    *pNext = pVASigCookieBlock->m_Next;

        for (UINT i = 0; i < pVASigCookieBlock->m_numcookies; i++)
            pVASigCookieBlock->m_cookies[i].Destruct();

        delete pVASigCookieBlock;

        pVASigCookieBlock = pNext;
    }

    delete m_pCrst;


    if (m_pLookupTableHeap)
        delete m_pLookupTableHeap;

    delete m_pLookupTableCrst;


#ifdef PROFILING_SUPPORTED
    if (CORProfilerTrackModuleLoads())
        g_profControlBlock.pProfInterface->ModuleUnloadFinished((ThreadID) GetThread(), 
                                                                (ModuleID) this, S_OK);
#endif // PROFILING_SUPPORTED

    if (m_file)
        delete m_file;

    
    if (m_compiledMethodRecord)
        delete m_compiledMethodRecord;

    if (m_loadedClassRecord)
        delete m_loadedClassRecord;

    //
    // Warning - deleting the zap file will cause the module to be unmapped
    //
    IStream *pStream = GetInMemorySymbolStream();
    if(pStream != NULL)
    {
        pStream->Release();
        SetInMemorySymbolStream(NULL);
    }


    if (IsPreload())
    {
        if (m_zapFile)
            delete m_zapFile;
    }
    else
    {
        if (m_zapFile)
            delete m_zapFile;

        delete this;
    }
}

HRESULT Module::VerifyFile(PEFile *file, BOOL fZap)
{
    HRESULT hr;
    IMAGE_COR20_HEADER *pCORHeader = file->GetCORHeader();

    // If the file is COM+ 1.0, which by definition has nothing the runtime can
    // use, or if the file requires a newer version of this engine than us,
    // it cannot be run by this engine.

    if (pCORHeader == NULL
        || VAL16(pCORHeader->MajorRuntimeVersion) == 1
        || VAL16(pCORHeader->MajorRuntimeVersion) > COR_VERSION_MAJOR)
    {
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
    }

    //verify that COM+ specific parts of the PE file are valid
    IfFailRet(file->VerifyDirectory(&pCORHeader->MetaData,IMAGE_SCN_MEM_WRITE));
    IfFailRet(file->VerifyDirectory(&pCORHeader->CodeManagerTable,IMAGE_SCN_MEM_WRITE));
    IfFailRet(file->VerifyDirectory(&pCORHeader->VTableFixups,0));

    // Don't do this.  If set, the VA is only guaranteed to be a
    // valid, loaded RVA if this file contains a standalone manifest.
    // Non-standalone manifest files will have the VA set, but the
    // manifest is not in an NTSection, so verifyDirectory() will fail.
    // IfFailRet(file->VerifyDirectory(m_pcorheader->Manifest,0));
    
    IfFailRet(file->VerifyFlags(VAL32(pCORHeader->Flags), fZap));
    return S_OK;
}

HRESULT Module::SetContainer(Assembly *pAssembly, int moduleIndex, mdToken moduleRef,
                             BOOL fResource, OBJECTREF *pThrowable)
{
    HRESULT hr;

    m_pAssembly = pAssembly;
    m_moduleRef = moduleRef;
    m_dwModuleIndex = moduleIndex;

    if (m_pAssembly->IsShared())
    {
        //
        // Compute a base DLS index for classes
        //
        SIZE_T typeCount = m_pMDImport ? m_pMDImport->GetCountWithTokenKind(mdtTypeDef)+1 : 0;

        SharedDomain::GetDomain()->AllocateSharedClassIndices(this, typeCount+1);
        m_ExposedModuleObjectIndex = m_dwBaseClassIndex + typeCount;
    }

    if (fResource)
        return S_OK;

    if (!IsPreload()) 
    { 
        hr = BuildClassForModule(pThrowable);
        if (FAILED(hr))
            return(hr);
    }    


#ifdef DEBUGGING_SUPPORTED
    //
    // If we're debugging, let the debugger know that this module
    // is initialized and loaded now.
    //
    if (CORDebuggerAttached())
        NotifyDebuggerLoad();
#endif // DEBUGGING_SUPPORTED

#ifdef PROFILING_SUPPORTED
    if (CORProfilerTrackModuleLoads())
        g_profControlBlock.pProfInterface->ModuleAttachedToAssembly((ThreadID) GetThread(),
                    (ModuleID) this, (AssemblyID) m_pAssembly);
#endif // PROFILING_SUPPORTED


    return S_OK;
}

//
// AddChunk adds the given chunk to the module's chunk list.
//

void Module::AddChunk(MethodDescChunk *pChunk)
{
    pChunk->SetNextChunk(m_pChunks);
    m_pChunks = pChunk;
}

//
// AllocateMap allocates the RID maps based on the size of the current
// metadata (if any)
//

HRESULT Module::AllocateMaps()
{
    enum
    {
        TYPEDEF_MAP_INITIAL_SIZE = 5,
        TYPEREF_MAP_INITIAL_SIZE = 5,
        MEMBERREF_MAP_INITIAL_SIZE = 10,
        MEMBERDEF_MAP_INITIAL_SIZE = 10,
        FILEREFERENCES_MAP_INITIAL_SIZE = 5,
        ASSEMBLYREFERENCES_MAP_INITIAL_SIZE = 5,

        TYPEDEF_MAP_BLOCK_SIZE = 50,
        TYPEREF_MAP_BLOCK_SIZE = 50,
        MEMBERREF_MAP_BLOCK_SIZE = 50,
        MEMBERDEF_MAP_BLOCK_SIZE = 50,
        FILEREFERENCES_MAP_BLOCK_SIZE = 50,
        ASSEMBLYREFERENCES_MAP_BLOCK_SIZE = 50,
        DEFAULT_RESERVE_SIZE = 4096,
    };
    
    DWORD           dwTableAllocElements;
    DWORD           dwReserveSize;
    void            **pTable = NULL;
    HRESULT         hr;

    if (m_pMDImport == NULL)
    {
        // For dynamic modules, it is essential that we at least have a TypeDefToMethodTable
        // map with an initial block.  Otherwise, all the iterators will abort on an
        // initial empty table and we will e.g. corrupt the backpatching chains during
        // an appdomain unload.
        m_TypeDefToMethodTableMap.dwMaxIndex = TYPEDEF_MAP_INITIAL_SIZE;

        // The above is essential.  The following ones are precautionary.
        m_TypeRefToMethodTableMap.dwMaxIndex = TYPEREF_MAP_INITIAL_SIZE;
        m_MethodDefToDescMap.dwMaxIndex = MEMBERDEF_MAP_INITIAL_SIZE;
        m_FieldDefToDescMap.dwMaxIndex = MEMBERDEF_MAP_INITIAL_SIZE;
        m_MemberRefToDescMap.dwMaxIndex = MEMBERREF_MAP_BLOCK_SIZE;
        m_FileReferencesMap.dwMaxIndex = FILEREFERENCES_MAP_INITIAL_SIZE;
        m_AssemblyReferencesMap.dwMaxIndex = ASSEMBLYREFERENCES_MAP_INITIAL_SIZE;
    }
    else
    {
        HENUMInternal   hTypeDefEnum;

        IfFailRet(m_pMDImport->EnumTypeDefInit(&hTypeDefEnum));
        m_TypeDefToMethodTableMap.dwMaxIndex = m_pMDImport->EnumTypeDefGetCount(&hTypeDefEnum);
        m_pMDImport->EnumTypeDefClose(&hTypeDefEnum);

        if (m_TypeDefToMethodTableMap.dwMaxIndex >= MAX_CLASSES_PER_MODULE)
            return COR_E_TYPELOAD;

        // Metadata count is inclusive
        m_TypeDefToMethodTableMap.dwMaxIndex++;

        // Get # TypeRefs
        m_TypeRefToMethodTableMap.dwMaxIndex = m_pMDImport->GetCountWithTokenKind(mdtTypeRef)+1;

        // Get # MethodDefs
        m_MethodDefToDescMap.dwMaxIndex = m_pMDImport->GetCountWithTokenKind(mdtMethodDef)+1;

        // Get # FieldDefs
        m_FieldDefToDescMap.dwMaxIndex = m_pMDImport->GetCountWithTokenKind(mdtFieldDef)+1;

        // Get # MemberRefs
        m_MemberRefToDescMap.dwMaxIndex = m_pMDImport->GetCountWithTokenKind(mdtMemberRef)+1;

        // Get the number of AssemblyReferences and FileReferences in the map
        m_FileReferencesMap.dwMaxIndex = m_pMDImport->GetCountWithTokenKind(mdtFile)+1;
        m_AssemblyReferencesMap.dwMaxIndex = m_pMDImport->GetCountWithTokenKind(mdtAssemblyRef)+1;
    }
    
    // Use one allocation to allocate all the tables
    dwTableAllocElements = m_TypeDefToMethodTableMap.dwMaxIndex + 
      (m_TypeRefToMethodTableMap.dwMaxIndex + m_MemberRefToDescMap.dwMaxIndex + 
       m_MethodDefToDescMap.dwMaxIndex + m_FieldDefToDescMap.dwMaxIndex) +
      m_AssemblyReferencesMap.dwMaxIndex + m_FileReferencesMap.dwMaxIndex;

    dwReserveSize = dwTableAllocElements * sizeof(void*);

    if (m_pLookupTableHeap == NULL)
    {
        // Round to system page size
        dwReserveSize = (dwReserveSize + g_SystemInfo.dwPageSize - 1) & (~(g_SystemInfo.dwPageSize-1));

        m_pLookupTableHeap = new (&m_LookupTableHeapInstance) 
          LoaderHeap(dwReserveSize, RIDMAP_COMMIT_SIZE);
        if (m_pLookupTableHeap == NULL)
            return E_OUTOFMEMORY;
        WS_PERF_ADD_HEAP(LOOKUP_TABLE_HEAP, m_pLookupTableHeap);
        
    }

    if (dwTableAllocElements > 0)
    {
        WS_PERF_SET_HEAP(LOOKUP_TABLE_HEAP);    
        pTable = (void **) m_pLookupTableHeap->AllocMem(dwTableAllocElements * sizeof(void*));
        WS_PERF_UPDATE_DETAIL("LookupTableHeap", dwTableAllocElements * sizeof(void*), pTable);
        if (pTable == NULL)
            return E_OUTOFMEMORY;
    }

    // Don't need to memset, since AllocMem() uses VirtualAlloc(), which guarantees the memory is zero
    // This way we don't automatically touch all those pages
    // memset(pTable, 0, dwTableAllocElements * sizeof(void*));

    m_dwTypeDefMapBlockSize = TYPEDEF_MAP_BLOCK_SIZE;
    m_TypeDefToMethodTableMap.pdwBlockSize = &m_dwTypeDefMapBlockSize;
    m_TypeDefToMethodTableMap.pNext  = NULL;
    m_TypeDefToMethodTableMap.pTable = pTable;

    m_dwTypeRefMapBlockSize = TYPEREF_MAP_BLOCK_SIZE;
    m_TypeRefToMethodTableMap.pdwBlockSize = &m_dwTypeRefMapBlockSize;
    m_TypeRefToMethodTableMap.pNext  = NULL;
    m_TypeRefToMethodTableMap.pTable = &pTable[m_TypeDefToMethodTableMap.dwMaxIndex];

    m_dwMethodDefMapBlockSize = MEMBERDEF_MAP_BLOCK_SIZE;
    m_MethodDefToDescMap.pdwBlockSize = &m_dwMethodDefMapBlockSize;
    m_MethodDefToDescMap.pNext  = NULL;
    m_MethodDefToDescMap.pTable = &m_TypeRefToMethodTableMap.pTable[m_TypeRefToMethodTableMap.dwMaxIndex];

    m_dwFieldDefMapBlockSize = MEMBERDEF_MAP_BLOCK_SIZE;
    m_FieldDefToDescMap.pdwBlockSize = &m_dwFieldDefMapBlockSize;
    m_FieldDefToDescMap.pNext  = NULL;
    m_FieldDefToDescMap.pTable = &m_MethodDefToDescMap.pTable[m_MethodDefToDescMap.dwMaxIndex];

    m_dwMemberRefMapBlockSize = MEMBERREF_MAP_BLOCK_SIZE;
    m_MemberRefToDescMap.pdwBlockSize = &m_dwMemberRefMapBlockSize;
    m_MemberRefToDescMap.pNext  = NULL;
    m_MemberRefToDescMap.pTable = &m_FieldDefToDescMap.pTable[m_FieldDefToDescMap.dwMaxIndex];
    
    m_dwFileReferencesMapBlockSize = FILEREFERENCES_MAP_BLOCK_SIZE;
    m_FileReferencesMap.pdwBlockSize = &m_dwFileReferencesMapBlockSize;
    m_FileReferencesMap.pNext  = NULL;
    m_FileReferencesMap.pTable = &m_MemberRefToDescMap.pTable[m_MemberRefToDescMap.dwMaxIndex];
    
    m_dwAssemblyReferencesMapBlockSize = ASSEMBLYREFERENCES_MAP_BLOCK_SIZE;
    m_AssemblyReferencesMap.pdwBlockSize = &m_dwAssemblyReferencesMapBlockSize;
    m_AssemblyReferencesMap.pNext  = NULL;
    m_AssemblyReferencesMap.pTable = &m_FileReferencesMap.pTable[m_FileReferencesMap.dwMaxIndex];

    return S_OK;
}

//
// FreeClassTables frees the classes in the module
//

void Module::FreeClassTables()
{
    if (m_dwFlags & CLASSES_FREED)
        return;

    m_dwFlags |= CLASSES_FREED;

#if _DEBUG
    DebugLogRidMapOccupancy();
#endif

    // Free the EEClass*'s filled out in the TypeDefToEEClass map
    LookupMap *pMap;
    DWORD       dwMinIndex = 0;
    MethodTable *pMT;

    // Go through each linked block
    for (pMap = &m_TypeDefToMethodTableMap; pMap != NULL && pMap->pTable; pMap = pMap->pNext)
    {
        DWORD i;
        DWORD dwIterCount = pMap->dwMaxIndex - dwMinIndex;
        void **pRealTableStart = &pMap->pTable[dwMinIndex];
        
        for (i = 0; i < dwIterCount; i++)
        {
            pMT = (MethodTable *) (pRealTableStart[i]);
            
            if (pMT != NULL)
            {
                pMT->GetClass()->destruct();
                pRealTableStart[i] = NULL;
            }
        }
    
        dwMinIndex = pMap->dwMaxIndex;
    }
}

void Module::SetMDImport(IMDInternalImport *pImport)
{
    _ASSERTE(m_pImporter == NULL);
    _ASSERTE(m_pEmitter == NULL);
    _ASSERTE(m_pHelper == NULL);

    m_pMDImport = pImport;
}

void Module::SetEmit(IMetaDataEmit *pEmit)
{
    _ASSERTE(m_pMDImport == NULL);
    _ASSERTE(m_pImporter == NULL);
    _ASSERTE(m_pEmitter == NULL);
    _ASSERTE(m_pHelper == NULL);

    m_pEmitter = pEmit;

#ifdef _DEBUG
	HRESULT hr =
#endif
	GetMetaDataInternalInterfaceFromPublic((void*) pEmit, IID_IMDInternalImport,
                                                        (void **)&m_pMDImport);
    _ASSERTE(SUCCEEDED(hr) && m_pMDImport != NULL);
}

//
// ConvertMDInternalToReadWrite: 
// If a public metadata interface is needed, must convert to r/w format
//  first.  Note that the data is not made writeable, nor actually converted,
//  only the data structures pointing to the actual data change.  This is 
//  done because the public interfaces only understand the MDInternalRW
//  format (which understands both the optimized and un-optimized metadata).
//
HRESULT Module::ConvertMDInternalToReadWrite(IMDInternalImport **ppImport)
{ 
    HRESULT     hr=S_OK;                // A result.
    IMDInternalImport *pOld;            // Old (current RO) value of internal import.
    IMDInternalImport *pNew;            // New (RW) value of internal import.

    // Take a local copy of *ppImport.  This may be a pointer to an RO
    //  or to an RW MDInternalXX.
    pOld = *ppImport;

    // If an RO, convert to an RW, return S_OK.  If already RW, no conversion 
    //  needed, return S_FALSE.
    IfFailGo(ConvertMDInternalImport(pOld, &pNew));

    // If no conversion took place, don't change pointers.
    if (hr == S_FALSE)
    {
        hr = S_OK;
        goto ErrExit;
    }

    // Swap the pointers in a thread safe manner.  If the contents of *ppImport
    //  equals pOld then no other thread got here first, and the old contents are
    //  replaced with pNew.  The old contents are returned.
    if (FastInterlockCompareExchangePointer((void**)ppImport, pNew, pOld) == pOld)
    {   // Swapped -- get the metadata to hang onto the old Internal import.
        HRESULT hrTemp;
        hrTemp =(*ppImport)->SetUserContextData(pOld);
        _ASSERTE(hrTemp == S_OK);
    }
    else
    {   // Some other thread finished first.  Just free the results of this conversion.
        pNew->Release();
        _ASSERTE((*ppImport)->QueryInterface(IID_IMDInternalImportENC, (void**)&pOld) == S_OK);
        DEBUG_STMT(if (pOld) pOld->Release();)
    }

ErrExit:
    return hr;
} // HRESULT Module::ConvertMDInternalToReadWrite()




IMetaDataImport *Module::GetImporter()
{
    if (m_pImporter == NULL && m_pMDImport != NULL)
    {
        if (SUCCEEDED(ConvertMDInternalToReadWrite(&m_pMDImport)))
        {
            IMetaDataImport *pIMDImport = NULL;
            GetMetaDataPublicInterfaceFromInternal((void*)m_pMDImport, 
                                                   IID_IMetaDataImport,
                                                   (void **)&pIMDImport);

            // Do a safe pointer assignment.   If another thread beat us, release
            // the interface and use the first one that gets in.
            if (FastInterlockCompareExchangePointer((void **)&m_pImporter, pIMDImport, NULL))
                pIMDImport->Release();
        }
    }

    _ASSERTE((m_pImporter != NULL && m_pMDImport != NULL) ||
             (m_pImporter == NULL && m_pMDImport == NULL));

    return m_pImporter;
}

LPCWSTR Module::GetFileName()
{
    if (m_file == NULL)
        return L"";
    else
        return m_file->GetFileName();
}

// Note that the debugger relies on the file name being copied
// into buffer pointed to by name.
HRESULT Module::GetFileName(LPSTR name, DWORD max, DWORD *count)
{
    if (m_file != NULL)
        return m_file->GetFileName(name, max, count);

    *count = 0;
    return S_OK;
}

BOOL Module::IsSystem()
{
    Assembly *pAssembly = GetAssembly();
    if (pAssembly == NULL)
        return IsSystemFile();
    else
        return pAssembly->IsSystem();
}

IMetaDataEmit *Module::GetEmitter()
{
    if (m_pEmitter == NULL)
    {
        HRESULT hr;
        hr = GetImporter()->QueryInterface(IID_IMetaDataEmit, (void **)&m_pEmitter);
        _ASSERTE(m_pEmitter && SUCCEEDED(hr));
    }

    return m_pEmitter;
}

IMetaDataHelper *Module::GetHelper()
{
    if (m_pHelper == NULL)
    {
        HRESULT hr;
        hr = GetImporter()->QueryInterface(IID_IMetaDataHelper, (void **)&m_pHelper);
        _ASSERTE(m_pHelper && SUCCEEDED(hr));
    }
    _ASSERTE(m_pHelper != NULL);
    return m_pHelper;
}

IMetaDataDispenserEx *Module::GetDispenser()
{
    if (m_pDispenser == NULL)
    {
        // Get the Dispenser interface.
        MetaDataGetDispenser(CLSID_CorMetaDataDispenser,
                                          IID_IMetaDataDispenserEx, (void **)&m_pDispenser);
    }
    _ASSERTE(m_pDispenser != NULL);
    return m_pDispenser;
}

void Module::ReleaseMDInterfaces(BOOL forENC)
{
    if (!forENC) 
    {
        if (m_pMDImport)
        {
            m_pMDImport->Release();
            m_pMDImport = NULL;
        }
        if (m_pDispenser)
        {
            m_pDispenser->Release();
            m_pDispenser = NULL;
        }
    }

    if (m_pEmitter)
    {
        m_pEmitter->Release();
        m_pEmitter = NULL;
    }

    if (m_pImporter)
    {
        m_pImporter->Release();
        m_pImporter = NULL;
    }

    if (m_pHelper)
    {
        m_pHelper->Release();
        m_pHelper = NULL;
    }
}

ClassLoader *Module::GetClassLoader()
{
    _ASSERTE(m_pAssembly != NULL);
    return m_pAssembly->GetLoader();
}

BaseDomain *Module::GetDomain()
{
    _ASSERTE(m_pAssembly != NULL);
    return m_pAssembly->GetDomain();
}

AssemblySecurityDescriptor *Module::GetSecurityDescriptor()
{
    _ASSERTE(m_pAssembly != NULL);
    return m_pAssembly->GetSecurityDescriptor();
}

BOOL Module::IsSystemClasses()
{
    return GetSecurityDescriptor()->IsSystemClasses();
}

BOOL Module::IsFullyTrusted()
{
    return GetSecurityDescriptor()->IsFullyTrusted();
}

//
// We'll use this struct and global to keep a list of all
// ISymUnmanagedReaders and ISymUnmanagedWriters (or any IUnknown) so
// we can relelease them at the end.
//
struct IUnknownList
{
    IUnknownList   *next;
    HelpForInterfaceCleanup *cleanup;
    IUnknown       *pUnk;
};

static IUnknownList *g_IUnknownList = NULL;

/*static*/ HRESULT Module::TrackIUnknownForDelete(
                                 IUnknown *pUnk,
                                 IUnknown ***pppUnk,
                                 HelpForInterfaceCleanup *pCleanHelp)
{
    IUnknownList *pNew = new IUnknownList;

    if (pNew == NULL)
        return E_OUTOFMEMORY;

    pNew->pUnk = pUnk; // Ref count is 1
    pNew->cleanup = pCleanHelp;
    pNew->next = g_IUnknownList;
    g_IUnknownList = pNew;

    // Return the address of where we're keeping the IUnknown*, if
    // needed.
    if (pppUnk)
        *pppUnk = &(pNew->pUnk);

    return S_OK;
}

/*static*/ void Module::ReleaseAllIUnknowns(void)
{
    IUnknownList **ppElement = &g_IUnknownList;

    while (*ppElement)
    {
        IUnknownList *pTmp = *ppElement;

        // Release the IUnknown
        if (pTmp->pUnk != NULL)
            pTmp->pUnk->Release();
            
        if (pTmp->cleanup != NULL)
            delete pTmp->cleanup;

        *ppElement = pTmp->next;
        delete pTmp;
    }
}

void Module::ReleaseIUnknown(IUnknown *pUnk)
{
    IUnknownList **ppElement = &g_IUnknownList;

    while (*ppElement)
    {
        IUnknownList *pTmp = *ppElement;

        // Release the IUnknown
        if (pTmp->pUnk == pUnk)
        {
            // This doesn't have to be thread safe because only add to front of list and
            // only delete on unload or shutdown and can only be happening on one thread
            pTmp->pUnk->Release();
            if (pTmp->cleanup != NULL)
                delete pTmp->cleanup;
            *ppElement = pTmp->next;
            delete pTmp;
            break;
        }
        ppElement = &pTmp->next;
    }
    _ASSERTE(ppElement);    // if have a reader, should have found it in list
}

void Module::ReleaseIUnknown(IUnknown **ppUnk)
{
    IUnknownList **ppElement = &g_IUnknownList;

    while (*ppElement)
    {
        IUnknownList *pTmp = *ppElement;

        // Release the IUnknown
        if (&(pTmp->pUnk) == ppUnk)
        {
            // This doesn't have to be thread safe because only add to front of list and
            // only delete on unload or shutdown and can only be happening on one thread
            if (pTmp->pUnk)
            pTmp->pUnk->Release();
            if (pTmp->cleanup != NULL)
                delete pTmp->cleanup;
            *ppElement = pTmp->next;
            delete pTmp;
            break;
        }
        ppElement = &pTmp->next;
    }
    _ASSERTE(ppElement);    // if have a reader, should have found it in list
}

void Module::ReleaseISymUnmanagedReader(void)
{
    // This doesn't have to take the m_pISymUnmanagedReaderLock since
    // a module is destroyed only by one thread.
    if (m_pISymUnmanagedReader == NULL)
        return;
    Module::ReleaseIUnknown(m_pISymUnmanagedReader);
    m_pISymUnmanagedReader = NULL;
}

/*static*/ void Module::ReleaseMemoryForTracking()
{
    IUnknownList **ppElement = &g_IUnknownList;

    while (*ppElement)
    {
        IUnknownList *pTmp = *ppElement;

        *ppElement = pTmp->next;

        if (pTmp->cleanup != NULL)
        {
            (*(pTmp->cleanup->pFunction))(pTmp->cleanup->pData);

            delete pTmp->cleanup;
        }
        
        delete pTmp;
    }
}// ReleaseMemoryForTracking


//
// Module::FusionCopyPDBs asks Fusion to copy PDBs for a given
// assembly if they need to be copied. This is for the case where a PE
// file is shadow copied to the Fusion cache. Fusion needs to be told
// to take the time to copy the PDB, too.
//

STDAPI CopyPDBs(IAssembly *pAsm); // private fusion API

void Module::FusionCopyPDBs(LPCWSTR moduleName)
{
    Assembly *pAssembly = GetAssembly();

    // Just return if we've already done this for this Module's
    // Assembly.
    if ((pAssembly->GetDebuggerInfoBits() & DACF_PDBS_COPIED) ||
        (pAssembly->GetFusionAssembly() == NULL))
    {
        LOG((LF_CORDB, LL_INFO10,
             "Don't need to copy PDB's for module %S\n",
             moduleName));
        
        return;
    }

    LOG((LF_CORDB, LL_INFO10,
         "Attempting to copy PDB's for module %S\n", moduleName));


    {
#ifdef LOGGING
        HRESULT hr =
#endif  // LOGGING
        CopyPDBs(pAssembly->GetFusionAssembly());

        LOG((LF_CORDB, LL_INFO10,
             "Fusion.dll!CopyPDBs returned hr=0x%08x for module 0x%08x\n",
             hr, this));
    }


    // Remember that we've copied the PDBs for this assembly.
    pAssembly->SetDebuggerInfoBits(
            (DebuggerAssemblyControlFlags)(pAssembly->GetDebuggerInfoBits() |
                                           DACF_PDBS_COPIED));
}

//
// This will return a symbol reader for this module, if possible.
//
#if defined(ENABLE_PERF_LOG) && defined(DEBUGGING_SUPPORTED)
extern BOOL g_fDbgPerfOn;
extern __int64 g_symbolReadersCreated;
#endif

// This function will free the metadata interface if we are not
// able to free the ISymUnmanagedReader
static void __stdcall ReleaseImporterFromISymUnmanagedReader(void * pData)
{
    IMetaDataImport *md = (IMetaDataImport*)pData;

    // We need to release it twice
    md->Release();
    md->Release();
    
}// ReleaseImporterFromISymUnmanagedReader

ISymUnmanagedReader *Module::GetISymUnmanagedReader(void)
{
    // ReleaseAllIUnknowns() called during EEShutDown() will destroy
    // m_pISymUnmanagedReader. We cannot use it for stack-traces or anything
    if (g_fEEShutDown)
        return NULL;

    // If we haven't created the lock yet, do so lazily here
    if (m_pISymUnmanagedReaderLock == NULL)
    {
        // Allocate and initialize the critical section
        PCRITICAL_SECTION pCritSec = new CRITICAL_SECTION;
        _ASSERTE(pCritSec != NULL);

        if (pCritSec == NULL)
            return (NULL);

        InitializeCriticalSection(pCritSec);

        // Swap the pointers in a thread safe manner.
        if (InterlockedCompareExchangePointer((PVOID *)&m_pISymUnmanagedReaderLock, (PVOID)pCritSec, NULL) != NULL)
        {
            DeleteCriticalSection(pCritSec);
            delete pCritSec;
        }
    }

    // Take the lock for the m_pISymUnmanagedReader
    EnterCriticalSection(m_pISymUnmanagedReaderLock);

    HRESULT hr = S_OK;
    HelpForInterfaceCleanup* hlp = NULL; 
    ISymUnmanagedBinder *pBinder = NULL;
    UINT lastErrorMode = 0;

    // Name for the module
    LPCWSTR pName = NULL;

    // Check to see if this variable has already been set
    if (m_pISymUnmanagedReader != NULL)
        goto ErrExit;

    pName = GetFileName();

    if (pName[0] == L'\0')
    {
        hr = E_INVALIDARG;
        goto ErrExit;
    }

    // Call Fusion to ensure that any PDB's are shadow copied before
    // trying to get a synbol reader. This has to be done once per
    // Assembly.
    FusionCopyPDBs(pName);

    // Create a binder to find the reader.
    //
    IfFailGo(FakeCoCreateInstance(CLSID_CorSymBinder,
                                  IID_ISymUnmanagedBinder,
                                  (void**)&pBinder));

    LOG((LF_CORDB, LL_INFO10, "M::GISUR: Created binder\n"));

    // Note: we change the error mode here so we don't get any popups as the PDB symbol reader attempts to search the
    // hard disk for files.
    lastErrorMode = SetErrorMode(SEM_NOOPENFILEERRORBOX|SEM_FAILCRITICALERRORS);
    
    hr = pBinder->GetReaderForFile(GetImporter(),
                                       pName,
                                       NULL,
                                   &m_pISymUnmanagedReader);

    SetErrorMode(lastErrorMode);

    if (FAILED(hr))
        goto ErrExit;

    hlp = new HelpForInterfaceCleanup;
    hlp->pData = GetImporter();
    hlp->pFunction = ReleaseImporterFromISymUnmanagedReader;
    
    
    IfFailGo(Module::TrackIUnknownForDelete((IUnknown*)m_pISymUnmanagedReader,
                                            NULL,
                                            hlp));

    LOG((LF_CORDB, LL_INFO10, "M::GISUR: Loaded symbols for module %S\n",
         pName));

#if defined(ENABLE_PERF_LOG) && defined(DEBUGGING_SUPPORTED)
    if (g_fDbgPerfOn)
        g_symbolReadersCreated++;
#endif
    
ErrExit:
    if (pBinder != NULL)
        pBinder->Release();
    
    if (FAILED(hr))
    {
        LOG((LF_CORDB, LL_INFO10, "M::GISUR: Failed to load symbols for module %S, hr=0x%08x\n", pName, hr));
        if (m_pISymUnmanagedReader)
            m_pISymUnmanagedReader->Release();
        m_pISymUnmanagedReader = (ISymUnmanagedReader*)0x01; // Failed to load.
    }

    // Leave the lock
    LeaveCriticalSection(m_pISymUnmanagedReaderLock);
    
    // Make checks that don't have to be done under lock
    if (m_pISymUnmanagedReader == (ISymUnmanagedReader *)0x01)
        return (NULL);
    else
        return (m_pISymUnmanagedReader);
}

HRESULT Module::UpdateISymUnmanagedReader(IStream *pStream)
{
    HRESULT hr = S_OK;
    ISymUnmanagedBinder *pBinder = NULL;
    HelpForInterfaceCleanup* hlp = NULL; 


    // If we don't already have a reader, create one.
    if (m_pISymUnmanagedReader == NULL)
    {
        IfFailGo(FakeCoCreateInstance(CLSID_CorSymBinder,
                                      IID_ISymUnmanagedBinder,
                                      (void**)&pBinder));

        LOG((LF_CORDB, LL_INFO10, "M::UISUR: Created binder\n"));

        IfFailGo(pBinder->GetReaderFromStream(GetImporter(),
                                              pStream,
                                              &m_pISymUnmanagedReader));

        hlp = new HelpForInterfaceCleanup;
        hlp->pData = GetImporter();
        hlp->pFunction = ReleaseImporterFromISymUnmanagedReader;

        IfFailGo(Module::TrackIUnknownForDelete(
                                      (IUnknown*)m_pISymUnmanagedReader,
                                      NULL,
                                      hlp));
    
        LOG((LF_CORDB, LL_INFO10,
             "M::UISUR: Loaded symbols for module 0x%08x\n", this));
    }
    else if (m_pISymUnmanagedReader != (ISymUnmanagedReader*)0x01)
    {
        // We already have a reader, so just replace the symbols. We
        // replace instead of update because we are doing this only
        // for dynamic modules and the syms are cumulative.
        hr = m_pISymUnmanagedReader->ReplaceSymbolStore(NULL, pStream);
    
        LOG((LF_CORDB, LL_INFO10,
             "M::UISUR: Updated symbols for module 0x%08x\n", this));
    }
    else
        hr = E_INVALIDARG;
        
ErrExit:
    if (pBinder)
        pBinder->Release();
    
    if (FAILED(hr))
    {
        LOG((LF_CORDB, LL_INFO10,
             "M::GISUR: Failed to load symbols for module 0x%08x, hr=0x%08x\n",
             this, hr));

        if (m_pISymUnmanagedReader)
        {
            m_pISymUnmanagedReader->Release();
            m_pISymUnmanagedReader = NULL; // We'll try again next time...
        }
    }

    return hr;
}

// At this point, this is only called when we're creating an appdomain
// out of an array of bytes, so we'll keep the IStream that we create
// around in case the debugger attaches later (including detach & re-attach!)
HRESULT Module::SetSymbolBytes(BYTE *pbSyms, DWORD cbSyms)
{
    HRESULT hr = S_OK;
    HelpForInterfaceCleanup* hlp = NULL; 


    // Create a IStream from the memory for the syms.
    ISymUnmanagedBinder *pBinder = NULL;

    CGrowableStream *pStream = new CGrowableStream();
    if (pStream == NULL)
    {
        return E_OUTOFMEMORY;
    }

    pStream->AddRef(); // The Module will keep a copy for it's own use.
    
#ifdef LOGGING        
    LPCWSTR pName;
    pName = GetFileName();
#endif
        
    ULONG cbWritten;
    hr = HRESULT_FROM_WIN32(pStream->Write((const void *)pbSyms,
                   (ULONG)cbSyms,
                                           &cbWritten));
    if (FAILED(hr))
        return hr;
                   
    // Create a reader.
    IfFailGo(FakeCoCreateInstance(CLSID_CorSymBinder,
                                  IID_ISymUnmanagedBinder,
                                  (void**)&pBinder));

    LOG((LF_CORDB, LL_INFO10, "M::SSB: Created binder\n"));

    // The SymReader gets the other reference:
    IfFailGo(pBinder->GetReaderFromStream(GetImporter(),
                                          pStream,
                                          &m_pISymUnmanagedReader));
    hlp = new HelpForInterfaceCleanup;
    hlp->pData = GetImporter();
    hlp->pFunction = ReleaseImporterFromISymUnmanagedReader;
    
    IfFailGo(Module::TrackIUnknownForDelete(
                                     (IUnknown*)m_pISymUnmanagedReader,
                                     NULL,
                                     hlp));
    
    LOG((LF_CORDB, LL_INFO10,
         "M::SSB: Loaded symbols for module 0x%08x\n", this));

    LOG((LF_CORDB, LL_INFO10, "M::GISUR: Loaded symbols for module %S\n",
         pName));

    // Make sure to set the symbol stream on the module before
    // attempting to send UpdateModuleSyms messages up for it.
    SetInMemorySymbolStream(pStream);

#ifdef DEBUGGING_SUPPORTED
    // Tell the debugger that symbols have been loaded for this
    // module.  We iterate through all domains which contain this
    // module's assembly, and send a debugger notify for each one.
    if (CORDebuggerAttached())
    {
        AppDomainIterator i;
    
        while (i.Next())
        {
            AppDomain *pDomain = i.GetDomain();

            if (pDomain->IsDebuggerAttached() && (GetDomain() == SystemDomain::System() ||
                                                  pDomain->ContainsAssembly(m_pAssembly)))
                g_pDebugInterface->UpdateModuleSyms(this, pDomain, FALSE);
        }
    }
#endif // DEBUGGING_SUPPORTED
    
ErrExit:
    if (pBinder)
        pBinder->Release();
    
    if (FAILED(hr))
    {
        LOG((LF_CORDB, LL_INFO10,
             "M::GISUR: Failed to load symbols for module %S, hr=0x%08x\n",
             pName, hr));

        if (m_pISymUnmanagedReader != NULL)
        {
            m_pISymUnmanagedReader->Release();
            m_pISymUnmanagedReader = NULL; // We'll try again next time.
        }
    }

    return hr;
}

//---------------------------------------------------------------------------
// displays details about metadata error including module name, error 
// string corresponding to hr code, and a rich error posted by the
// metadata (if available).
//
//---------------------------------------------------------------------------
void Module::DisplayFileLoadError(HRESULT hrRpt)
{
    HRESULT     hr;
    LPCWSTR     rcMod;                              // Module path
    WCHAR       rcErr[ERROR_LENGTH];                // Error string for hr
    IErrorInfo  *pIErrInfo;                         // Error item 
    BSTR        sDesc = NULL;                       // MetaData error message
    WCHAR       rcTemplate[ERROR_LENGTH];       

    LPWSTR      rcFormattedErr = new (throws) WCHAR[FORMAT_MESSAGE_LENGTH];

    // Retrieve metadata rich error
    if (GetErrorInfo(0, &pIErrInfo) == S_OK) {
        pIErrInfo->GetDescription(&sDesc);
	pIErrInfo->Release();
    }

    // Get error message template
    hr = LoadStringRC(IDS_EE_METADATA_ERROR, rcTemplate, NumItems(rcTemplate), true);

    if (SUCCEEDED(hr)) {
        rcMod = GetFileName();

        // Print metadata rich error
        if (sDesc && sDesc[0])
        {
            _snwprintf(rcFormattedErr, FORMAT_MESSAGE_LENGTH, rcTemplate, rcMod, sDesc);
        }
        else if (HRESULT_FACILITY(hrRpt) == FACILITY_URT)
        {
            // If this is one of our errors, then grab the error from the rc file.
            hr = LoadStringRC(LOWORD(hrRpt), rcErr, NumItems(rcErr), true);
            if (hr == S_OK)
                // Retrieve error message string for inputted hr code
                _snwprintf(rcFormattedErr, FORMAT_MESSAGE_LENGTH, rcTemplate, rcMod, rcErr);
        } 
        else if (WszFormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                                  0, hrRpt, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                  rcErr, NumItems(rcErr), 0))
        {
            // Otherwise it isn't one of ours, so we need to see if the system can
            // find the text for it.
            hr = S_OK;
            
            // System messages contain a trailing \r\n, which we don't want normally.
            int iLen = lstrlenW(rcErr);
            if (iLen > 3 && rcErr[iLen - 2] == '\r' && rcErr[iLen - 1] == '\n')
                rcErr[iLen - 2] = '\0';
            _snwprintf(rcFormattedErr, FORMAT_MESSAGE_LENGTH, rcTemplate, rcMod, rcErr);
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            if (SUCCEEDED(hr))
                hr = E_FAIL;
        }
    }

    // If we failed to find the message anywhere, then issue a hard coded message.
    if (FAILED(hr))
    {
        swprintf(rcErr, L"CLR Internal error: 0x%08x", hrRpt);
        DEBUG_STMT(DbgWriteEx(rcErr));
    }

    DisplayError(hrRpt, rcFormattedErr);       
    delete rcFormattedErr;
    if (sDesc) {
	SysFreeString(sDesc);
    }
}
 

//==========================================================================
// If Module doesn't yet know the Exposed Module class that represents it via
// Reflection, acquire that class now.  Regardless, return it to the caller.
//==========================================================================
OBJECTREF Module::GetExposedModuleObject(AppDomain *pDomain)
{
    THROWSCOMPLUSEXCEPTION();

    //
    // Figure out which handle to use.
    //
    
    OBJECTHANDLE hObject;


    if (GetAssembly()->IsShared())
    {
        if (pDomain == NULL)
            pDomain = GetAppDomain();

        DomainLocalBlock *pLocalBlock = pDomain->GetDomainLocalBlock();
        
        hObject = (OBJECTHANDLE) pLocalBlock->GetSlot(m_ExposedModuleObjectIndex);
        if (hObject == NULL)
        {
            hObject = pDomain->CreateHandle(NULL);
            pLocalBlock->SetSlot(m_ExposedModuleObjectIndex, hObject);
        }
    }
    else
    {
        hObject = m_ExposedModuleObject;
        if (hObject == NULL)
        {
            hObject = GetDomain()->CreateHandle(NULL);
            m_ExposedModuleObject = hObject;
        }
    }

    if (ObjectFromHandle(hObject) == NULL)
    {
        // Make sure that reflection has been initialized
        COMClass::EnsureReflectionInitialized();

        REFLECTMODULEBASEREF  refClass = NULL;
        HRESULT         hr = COMClass::CreateClassObjFromModule(this, &refClass);

        // either we got a refClass or we got an error code:
        _ASSERTE(SUCCEEDED(hr) == (refClass != NULL));

        if (FAILED(hr))
            COMPlusThrowHR(hr);

        // The following will only update the handle if it is currently NULL.
        // In other words, first setter wins.  We don't have to do any
        // cleanup if our guy loses, since GC will collect it.
        StoreFirstObjectInHandle(hObject, (OBJECTREF) refClass);

        // either way, we must have a non-NULL value now (since nobody will
        // reset it back to NULL underneath us)
        _ASSERTE(ObjectFromHandle(hObject) != NULL);

    }
    return ObjectFromHandle(hObject);
}

//==========================================================================
// If Module doesn't yet know the Exposed Module class that represents it via
// Reflection, acquire that class now.  Regardless, return it to the caller.
//==========================================================================
OBJECTREF Module::GetExposedModuleBuilderObject(AppDomain *pDomain)
{
    THROWSCOMPLUSEXCEPTION();

    //
    // Figure out which handle to use.
    //

    OBJECTHANDLE hObject;


    if (GetAssembly()->IsShared())
    {
        if (pDomain == NULL)
            pDomain = GetAppDomain();

        DomainLocalBlock *pLocalBlock = pDomain->GetDomainLocalBlock();
        
        hObject = (OBJECTHANDLE) pLocalBlock->GetSlot(m_ExposedModuleObjectIndex);
        if (hObject == NULL)
        {
            hObject = pDomain->CreateHandle(NULL);
            pLocalBlock->SetSlot(m_ExposedModuleObjectIndex, hObject);
        }
    }
    else
    {
        hObject = m_ExposedModuleObject;
        if (hObject == NULL)
        {
            hObject = GetDomain()->CreateHandle(NULL);
            m_ExposedModuleObject = hObject;
        }
    }

    if (ObjectFromHandle(hObject) == NULL)
    {
        // Make sure that reflection has been initialized
        COMClass::EnsureReflectionInitialized();

        // load module builder if it is not loaded.
        if (g_pRefUtil->GetClass(RC_DynamicModule) == NULL)
        {
            // Question: do I need to worry about multi-threading? I think so...
            MethodTable *pMT = g_Mscorlib.GetClass(CLASS__MODULE_BUILDER);
            g_pRefUtil->SetClass(RC_DynamicModule, pMT);
            g_pRefUtil->SetTrueClass(RC_DynamicModule, pMT);
        }

        REFLECTMODULEBASEREF  refClass = NULL;
        HRESULT         hr = COMClass::CreateClassObjFromDynamicModule(this, &refClass);

        // either we got a refClass or we got an error code:
        _ASSERTE(SUCCEEDED(hr) == (refClass != NULL));

        if (FAILED(hr))
            COMPlusThrowHR(hr);

        // The following will only update the handle if it is currently NULL.
        // In other words, first setter wins.  We don't have to do any
        // cleanup if our guy loses, since GC will collect it.
        StoreFirstObjectInHandle(hObject, (OBJECTREF) refClass);

        // either way, we must have a non-NULL value now (since nobody will
        // reset it back to NULL underneath us)
        _ASSERTE(ObjectFromHandle(hObject) != NULL);

    }
    return ObjectFromHandle(hObject);
}


// Distinguish between the fake class associated with the module (global fields &
// functions) from normal classes.
BOOL Module::AddClass(mdTypeDef classdef)
{
    if (!RidFromToken(classdef))
    {
        OBJECTREF       pThrowable = 0;
        BOOL            result;

        GCPROTECT_BEGIN(pThrowable);
        result = SUCCEEDED(BuildClassForModule(&pThrowable));
        GCPROTECT_END();

        return result;
    }
    else
    {
        return SUCCEEDED(GetClassLoader()->AddAvailableClassDontHaveLock(this, m_dwModuleIndex, classdef));
    }
}

//---------------------------------------------------------------------------
// For the global class this builds the table of MethodDescs an adds the rids
// to the MethodDef map.
//---------------------------------------------------------------------------
HRESULT Module::BuildClassForModule(OBJECTREF *pThrowable)
{        
    EEClass        *pClass;
    HRESULT         hr;
    HENUMInternal   hEnum;
    DWORD           cFunctions, cFields;

    _ASSERTE(m_pMDImport != NULL);

    // Obtain count of global functions
    hr = m_pMDImport->EnumGlobalFunctionsInit(&hEnum);
    if (FAILED(hr))
    {
        _ASSERTE(!"Cannot count global functions");
        return hr;
    }
    cFunctions = m_pMDImport->EnumGetCount(&hEnum);
    m_pMDImport->EnumClose(&hEnum);

    // Obtain count of global fields
    hr = m_pMDImport->EnumGlobalFieldsInit(&hEnum);
    if (FAILED(hr))
    {
        _ASSERTE(!"Cannot count global fields");
        return hr;
    }
    cFields = m_pMDImport->EnumGetCount(&hEnum);
    m_pMDImport->EnumClose(&hEnum);

    // If we have any work to do...
    if (cFunctions > 0 || cFields > 0)
    {
        COUNTER_ONLY(size_t _HeapSize = 0);

        hr = GetClassLoader()->LoadTypeHandleFromToken(this,
                                                       COR_GLOBAL_PARENT_TOKEN,
                                                       &pClass,
                                                       pThrowable);
        if (SUCCEEDED(hr)) 
        {
#ifdef PROFILING_SUPPORTED
            // Record load of the class for the profiler, whether successful or not.
            if (CORProfilerTrackClasses())
            {
                g_profControlBlock.pProfInterface->ClassLoadStarted((ThreadID) GetThread(),
                                                                    (ClassID) TypeHandle(pClass).AsPtr());
            }
#endif //PROFILING_SUPPORTED

#ifdef PROFILING_SUPPORTED
            // Record load of the class for the profiler, whether successful or not.
            if (CORProfilerTrackClasses())
            {
                g_profControlBlock.pProfInterface->ClassLoadFinished((ThreadID) GetThread(),
                                                                     (ClassID) TypeHandle(pClass).AsPtr(),
                                                                     SUCCEEDED(hr) ? S_OK : hr);
            }
#endif //PROFILING_SUPPORTED
        }

#ifdef DEBUGGING_SUPPORTED
        //
        // If we're debugging, let the debugger know that this class
        // is initialized and loaded now.
        //
        if (CORDebuggerAttached())
            pClass->NotifyDebuggerLoad();
#endif // DEBUGGING_SUPPORTED
      
        if (FAILED(hr))
            return hr;

#ifdef ENABLE_PERF_COUNTERS
        GetGlobalPerfCounters().m_Loading.cClassesLoaded ++;
        GetPrivatePerfCounters().m_Loading.cClassesLoaded ++;

        _HeapSize = pClass->GetClassLoader()->GetHighFrequencyHeap()->GetSize();

        GetGlobalPerfCounters().m_Loading.cbLoaderHeapSize = _HeapSize;
        GetPrivatePerfCounters().m_Loading.cbLoaderHeapSize = _HeapSize;
#endif

        m_pMethodTable = pClass->GetMethodTable();
    }
    else
    {
        m_pMethodTable = NULL;
    }
        
    return hr;
}

//
// Virtual defaults
//

BYTE *Module::GetILCode(DWORD target) const
{
    return ResolveILRVA(target, FALSE);
}

OBJECTHANDLE Module::ResolveStringRef(DWORD Token, BaseDomain *pDomain) const
{
    _ASSERTE(TypeFromToken(Token) == mdtString);

    EEStringData strData;
    OBJECTHANDLE string = NULL;

    ULONG   cbString;
    BOOL    fIs80Plus;
    LPCWSTR pszString;

#ifndef BIGENDIAN
    pszString = m_pMDImport->GetUserString(Token, &cbString, &fIs80Plus);
    strData.SetStringBuffer(pszString);
#else // !BIGENDIAN
    pszString = m_pMDImport->GetUserStringRequiresSwap(Token, &cbString, &fIs80Plus);

    WCHAR * pszSwapped = new WCHAR[cbString];
    memcpy(pszSwapped, pszString, cbString*sizeof(WCHAR));
    SwapStringLength(pszSwapped, cbString);

    strData.SetStringBuffer(pszSwapped);

    EE_TRY_FOR_FINALLY {
#endif // !BIGENDIAN

    // MD and String look at this bit in opposite ways.  Here's where we'll do the conversion.
    // MD sets the bit to true if the string contains characters greater than 80.
    // String sets the bit to true if the string doesn't contain characters greater than 80.

    strData.SetCharCount(cbString);
    strData.SetIsOnlyLowChars(!fIs80Plus);

    // Retrieve the string from the AppDomain.

    BEGIN_ENSURE_COOPERATIVE_GC();

    string = (OBJECTHANDLE)pDomain->GetStringObjRefPtrFromUnicodeString(&strData);

    END_ENSURE_COOPERATIVE_GC();

#ifdef BIGENDIAN
    }
    EE_FINALLY {
        delete [] pszSwapped;
    }
    EE_END_FINALLY
#endif

    return string;
}

//
// Used by the verifier.  Returns whether this stringref is valid.  
//
BOOL Module::IsValidStringRef(DWORD token)
{
    if(TypeFromToken(token)==mdtString)
    {
        ULONG rid;
        if((rid = RidFromToken(token)) != 0)
        {
            if(m_pMDImport->IsValidToken(token)) return TRUE;
        }
    }
    return FALSE;
}

//
// Increase the size of one of the maps, such that it can handle a RID of at least "rid".
//
// This function must also check that another thread didn't already add a LookupMap capable
// of containing the same RID.
//
LookupMap *Module::IncMapSize(LookupMap *pMap, DWORD rid)
{
    LookupMap   *pPrev = NULL;
    DWORD       dwPrevMaxIndex = 0;

    m_pLookupTableCrst->Enter();

    // Check whether we can already handle this RID index
    do
    {
        if (rid < pMap->dwMaxIndex)
        {
            // Already there - some other thread must have added it
            m_pLookupTableCrst->Leave();
            return pMap;
        }

        dwPrevMaxIndex = pMap->dwMaxIndex;
        pPrev = pMap;
        pMap = pMap->pNext;
    } while (pMap != NULL);

    _ASSERTE(pPrev != NULL); // should never happen, because there's always at least one map

    DWORD dwMinNeeded = rid - dwPrevMaxIndex + 1; // Min # elements required for this chunk
    DWORD dwBlockSize = *pPrev->pdwBlockSize;   // Min # elements required by block size
    DWORD dwSizeToAllocate;                     // Actual number of elements we will allocate

    if (dwMinNeeded > dwBlockSize)
    {
        dwSizeToAllocate = dwMinNeeded;
    }
    else
    {
        dwSizeToAllocate = dwBlockSize;
        dwBlockSize <<= 1;                      // Increase block size
        *pPrev->pdwBlockSize = dwBlockSize;
    }

    if (m_pLookupTableHeap == NULL)
    {
        m_pLookupTableHeap = new (&m_LookupTableHeapInstance) 
          LoaderHeap(g_SystemInfo.dwPageSize, RIDMAP_COMMIT_SIZE);
        if (m_pLookupTableHeap == NULL)
        {
            m_pLookupTableCrst->Leave();
            return NULL;
        }
    }

    WS_PERF_SET_HEAP(LOOKUP_TABLE_HEAP);    
    LookupMap *pNewMap = (LookupMap *) m_pLookupTableHeap->AllocMem(sizeof(LookupMap) + dwSizeToAllocate*sizeof(void*));
    WS_PERF_UPDATE_DETAIL("LookupTableHeap", sizeof(LookupMap) + dwSizeToAllocate*sizeof(void*), pNewMap);
    if (pNewMap == NULL)
    {
        m_pLookupTableCrst->Leave();
        return NULL;
    }

    // NOTE: we don't need to zero fill the map since we VirtualAlloc()'d it

    pNewMap->pNext          = NULL;
    pNewMap->dwMaxIndex     = dwPrevMaxIndex + dwSizeToAllocate;
    pNewMap->pdwBlockSize   = pPrev->pdwBlockSize;

    // pTable is not a pointer to the beginning of the table.  Rather, anyone who uses Table can
    // simply index off their RID (as long as their RID is < dwMaxIndex, and they are not serviced
    // by a previous table for lower RIDs).
    pNewMap->pTable         = ((void **) (pNewMap + 1)) - dwPrevMaxIndex;

    // Link ourselves in
    pPrev->pNext            = pNewMap;

    m_pLookupTableCrst->Leave();
    return pNewMap;
}

BOOL Module::AddToRidMap(LookupMap *pMap, DWORD rid, void *pDatum)
{
    LookupMap *pMapStart = pMap;
    _ASSERTE(pMap != NULL);

    do
    {
        if (rid < pMap->dwMaxIndex)
        {
            pMap->pTable[rid] = pDatum;
            return TRUE;
        }

        pMap = pMap->pNext;
    } while (pMap != NULL);

    pMap = IncMapSize(pMapStart, rid);
    if (pMap == NULL)
        return NULL;

    pMap->pTable[rid] = pDatum;
    return TRUE;
}

void *Module::GetFromRidMap(LookupMap *pMap, DWORD rid)
{
    _ASSERTE(pMap != NULL);

    do
    {
        if (rid < pMap->dwMaxIndex)
        {
            if (pMap->pTable[rid] != NULL)
                return pMap->pTable[rid]; 
            else
                break;
        }

        pMap = pMap->pNext;
    } while (pMap != NULL);

    return NULL;
}

#ifdef _DEBUG
void Module::DebugGetRidMapOccupancy(LookupMap *pMap, DWORD *pdwOccupied, DWORD *pdwSize)
{
    DWORD       dwMinIndex = 0;

    *pdwOccupied = 0;
    *pdwSize     = 0;

    if(pMap == NULL) return;

    // Go through each linked block
    for (; pMap != NULL; pMap = pMap->pNext)
    {
        DWORD i;
        DWORD dwIterCount = pMap->dwMaxIndex - dwMinIndex;
        void **pRealTableStart = &pMap->pTable[dwMinIndex];

        for (i = 0; i < dwIterCount; i++)
        {
            if (pRealTableStart[i] != NULL)
                (*pdwOccupied)++;
        }

        (*pdwSize) += dwIterCount;

        dwMinIndex = pMap->dwMaxIndex;
    }
}

void Module::DebugLogRidMapOccupancy()
{
    DWORD dwOccupied1, dwSize1, dwPercent1;
    DWORD dwOccupied2, dwSize2, dwPercent2;
    DWORD dwOccupied3, dwSize3, dwPercent3;
    DWORD dwOccupied4, dwSize4, dwPercent4;
    DWORD dwOccupied5, dwSize5, dwPercent5;
    DWORD dwOccupied6, dwSize6, dwPercent6;
    DWORD dwOccupied7, dwSize7, dwPercent7;
    
    DebugGetRidMapOccupancy(&m_TypeDefToMethodTableMap, &dwOccupied1, &dwSize1);
    DebugGetRidMapOccupancy(&m_TypeRefToMethodTableMap, &dwOccupied2, &dwSize2);
    DebugGetRidMapOccupancy(&m_MethodDefToDescMap, &dwOccupied3, &dwSize3);
    DebugGetRidMapOccupancy(&m_FieldDefToDescMap, &dwOccupied4, &dwSize4);
    DebugGetRidMapOccupancy(&m_MemberRefToDescMap, &dwOccupied5, &dwSize5);
    DebugGetRidMapOccupancy(&m_FileReferencesMap, &dwOccupied6, &dwSize6);
    DebugGetRidMapOccupancy(&m_AssemblyReferencesMap, &dwOccupied7, &dwSize7);

    dwPercent1 = dwOccupied1 ? ((dwOccupied1 * 100) / dwSize1) : 0;
    dwPercent2 = dwOccupied2 ? ((dwOccupied2 * 100) / dwSize2) : 0;
    dwPercent3 = dwOccupied3 ? ((dwOccupied3 * 100) / dwSize3) : 0;
    dwPercent4 = dwOccupied4 ? ((dwOccupied4 * 100) / dwSize4) : 0;
    dwPercent5 = dwOccupied5 ? ((dwOccupied5 * 100) / dwSize5) : 0;
    dwPercent6 = dwOccupied6 ? ((dwOccupied6 * 100) / dwSize6) : 0;
    dwPercent7 = dwOccupied7 ? ((dwOccupied7 * 100) / dwSize7) : 0;

    LOG((
        LF_EEMEM, 
        INFO3, 
        "   Map occupancy:\n"
        "      TypeDefToEEClass map: %4d/%4d (%2d %%)\n"
        "      TypeRefToEEClass map: %4d/%4d (%2d %%)\n"
        "      MethodDefToDesc map:  %4d/%4d (%2d %%)\n"
        "      FieldDefToDesc map:  %4d/%4d (%2d %%)\n"
        "      MemberRefToDesc map:  %4d/%4d (%2d %%)\n"
        "      FileReferences map:  %4d/%4d (%2d %%)\n"
        "      AssemblyReferences map:  %4d/%4d (%2d %%)\n"
        ,
        dwOccupied1, dwSize1, dwPercent1,
        dwOccupied2, dwSize2, dwPercent2,
        dwOccupied3, dwSize3, dwPercent3,
        dwOccupied4, dwSize4, dwPercent4,
        dwOccupied5, dwSize5, dwPercent5,
        dwOccupied6, dwSize6, dwPercent6,
        dwOccupied7, dwSize7, dwPercent7

    ));
}
#endif




//
// FindFunction finds a MethodDesc for a global function methoddef or ref
//

MethodDesc *Module::FindFunction(mdToken pMethod)
{
    MethodDesc* pMethodDesc = NULL;

    BEGIN_ENSURE_COOPERATIVE_GC();
    OBJECTREF throwable = NULL;
    GCPROTECT_BEGIN(throwable);
        
    HRESULT hr = E_FAIL;
    
    COMPLUS_TRY
      {
        hr = EEClass::GetMethodDescFromMemberRef(this, pMethod, &pMethodDesc, &throwable);
      }
    COMPLUS_CATCH
      {
        throwable = GETTHROWABLE();
      }
    COMPLUS_END_CATCH

    if(FAILED(hr) || throwable != NULL) 
    {
        pMethodDesc = NULL;
#ifdef _DEBUG
        LPCUTF8 pszMethodName = CEEInfo::findNameOfToken(this, pMethod);
        LOG((LF_IJW, 
             LL_INFO10, "Failed to find Method: %s for Vtable Fixup\n", pszMethodName));
#endif
    }

    GCPROTECT_END();
    END_ENSURE_COOPERATIVE_GC();
        
    return pMethodDesc;
}

OBJECTREF Module::GetLinktimePermissions(mdToken token, OBJECTREF *prefNonCasDemands)
{
    OBJECTREF refCasDemands = NULL;
    SecurityHelper::GetDeclaredPermissions(
        GetMDImport(),
        token,
        dclLinktimeCheck,
        &refCasDemands);

    SecurityHelper::GetDeclaredPermissions(
        GetMDImport(),
        token,
        dclNonCasLinkDemand,
        prefNonCasDemands);

    return refCasDemands;
}

OBJECTREF Module::GetInheritancePermissions(mdToken token, OBJECTREF *prefNonCasDemands)
{
    OBJECTREF refCasDemands = NULL;
    SecurityHelper::GetDeclaredPermissions(
        GetMDImport(),
        token,
        dclInheritanceCheck,
        &refCasDemands);

    SecurityHelper::GetDeclaredPermissions(
        GetMDImport(),
        token,
        dclNonCasInheritance,
        prefNonCasDemands);

    return refCasDemands;
}

OBJECTREF Module::GetCasInheritancePermissions(mdToken token)
{
    OBJECTREF refCasDemands = NULL;
    SecurityHelper::GetDeclaredPermissions(
        GetMDImport(),
        token,
        dclInheritanceCheck,
        &refCasDemands);
    return refCasDemands;
}

OBJECTREF Module::GetNonCasInheritancePermissions(mdToken token)
{
    OBJECTREF refNonCasDemands = NULL;
    SecurityHelper::GetDeclaredPermissions(
        GetMDImport(),
        token,
        dclNonCasInheritance,
        &refNonCasDemands);
    return refNonCasDemands;
}

#ifdef DEBUGGING_SUPPORTED
void Module::NotifyDebuggerLoad()
{
    if (!CORDebuggerAttached())
        return;

    // This routine iterates through all domains which contain its 
    // assembly, and send a debugger notify in them.

    // Note that if this is the LoadModule of an assembly manifeset, we expect to 
    // NOT send the module load for our own app domain here - instead it 
    // will be sent from the NotifyDebuggerAttach call called from 
    // Assembly::NotifyDebuggerAttach.
    //

    AppDomainIterator i;
    
    while (i.Next())
    {
        AppDomain *pDomain = i.GetDomain();

        if (pDomain->IsDebuggerAttached() 
            && (GetDomain() == SystemDomain::System()
                || pDomain->ShouldContainAssembly(m_pAssembly, FALSE) == S_OK))
            NotifyDebuggerAttach(pDomain, ATTACH_ALL, FALSE);
    }
}

BOOL Module::NotifyDebuggerAttach(AppDomain *pDomain, int flags, BOOL attaching)
{
    if (!attaching && !pDomain->IsDebuggerAttached())
        return FALSE;

    // We don't notify the debugger about modules that don't contain any code.
    if (IsResource())
        return FALSE;
    
    BOOL result = FALSE;

    HRESULT hr = S_OK;

    LPCWSTR module = NULL;
    IMAGE_COR20_HEADER* pHeader = NULL;

    // Grab the file name
    module = GetFileName();

    // We need to check m_file directly because the debugger
    // can be called before the module is setup correctly
    if(m_file) 
        pHeader = m_file->GetCORHeader();
        
    if (FAILED(hr))
        return result;

    // Due to the wacky way in which modules/assemblies are loaded,
    // it is possible that a module is loaded before its corresponding
    // assembly has been loaded. We will defer sending the load
    // module event till after the assembly has been loaded and a 
    // load assembly event has been sent.
    if (GetClassLoader()->GetAssembly() != NULL)
    {
        if (flags & ATTACH_MODULE_LOAD)
        {
            g_pDebugInterface->LoadModule(this, 
                                          pHeader,
                                          GetILBase(),
                                          module, (DWORD)wcslen(module),
                                          GetAssembly(), pDomain,
                                          attaching,
                                          m_file ? CorMap::ImageType(m_file->GetCORModule()) : CorLoadUndefinedMap);

            result = TRUE;
        }
        
        if (flags & ATTACH_CLASS_LOAD)
        {
            LookupMap *pMap;
            DWORD       dwMinIndex = 0;

            // Go through each linked block
            for (pMap = &m_TypeDefToMethodTableMap; pMap != NULL;
                 pMap = pMap->pNext)
            {
                DWORD i;
                DWORD dwIterCount = pMap->dwMaxIndex - dwMinIndex;
                void **pRealTableStart = &pMap->pTable[dwMinIndex];

                for (i = 0; i < dwIterCount; i++)
                {
                    MethodTable *pMT = (MethodTable *) (pRealTableStart[i]);

                    if (pMT != NULL && pMT->GetClass()->IsRestored())
                    {
                        EEClass *pClass = pMT->GetClass();

                        result = pClass->NotifyDebuggerAttach(pDomain, attaching) || result;
                    }
                }

                dwMinIndex = pMap->dwMaxIndex;
            }
            
            // Send a load event for the module's method table, if necessary.

            if (GetMethodTable() != NULL)
            {
                MethodTable *pMT = GetMethodTable();

                if (pMT != NULL && pMT->GetClass()->IsRestored())
                    result = pMT->GetClass()->NotifyDebuggerAttach(pDomain, attaching) || result;
            }

        }
    }

    return result;
}

void Module::NotifyDebuggerDetach(AppDomain *pDomain)
{
    if (!pDomain->IsDebuggerAttached())
        return;
        
    // We don't notify the debugger about modules that don't contain any code.
    if (IsResource())
        return;
    
    LookupMap  *pMap;
    DWORD       dwMinIndex = 0;

    // Go through each linked block
    for (pMap = &m_TypeDefToMethodTableMap; pMap != NULL;
         pMap = pMap->pNext)
    {
        DWORD i;
        DWORD dwIterCount = pMap->dwMaxIndex - dwMinIndex;
        void **pRealTableStart = &pMap->pTable[dwMinIndex];

        for (i = 0; i < dwIterCount; i++)
        {
            MethodTable *pMT = (MethodTable *) (pRealTableStart[i]);

            if (pMT != NULL && pMT->GetClass()->IsRestored())
            {
                EEClass *pClass = pMT->GetClass();

                pClass->NotifyDebuggerDetach(pDomain);
            }
        }

        dwMinIndex = pMap->dwMaxIndex;
    }

    // Send a load event for the module's method table, if necessary.

    if (GetMethodTable() != NULL)
    {
        MethodTable *pMT = GetMethodTable();

        if (pMT != NULL && pMT->GetClass()->IsRestored())
            pMT->GetClass()->NotifyDebuggerDetach(pDomain);
    }
    
    g_pDebugInterface->UnloadModule(this, pDomain);
}
#endif // DEBUGGING_SUPPORTED


HRESULT Module::Create(PEFile *pFile, PEFile *pZapFile, Module **ppModule, BOOL isEnC)
{
     _ASSERTE(pZapFile == NULL);
     return Create(pFile, ppModule, isEnC);
}



void Module::LogClassLoad(EEClass *pClass)
{
    _ASSERTE(pClass->GetModule() == this);


}

void Module::LogMethodLoad(MethodDesc *pMethod)
{
    _ASSERTE(pMethod->GetModule() == this);


}


// ===========================================================================
// InMemoryModule
// ===========================================================================

InMemoryModule::InMemoryModule()
  : m_sdataSection(0),
    m_pCeeFileGen(NULL)
{
#ifdef _DEBUG
    HRESULT hr =
#endif
	Module::Init(0);
    _ASSERTE(SUCCEEDED(hr));
}

HRESULT InMemoryModule::Init(REFIID riidCeeGen)
{
    HRESULT hr;

    SetInMemory();

    IfFailGo(AllocateMaps());

    IMetaDataEmit *pEmit;
    IfFailGo(GetDispenser()->DefineScope(CLSID_CorMetaDataRuntime, 0, IID_IMetaDataEmit, (IUnknown **)&pEmit));

    SetEmit(pEmit);

    IfFailRet(CreateICeeGen(riidCeeGen, (void **)&m_pCeeFileGen));    

 ErrExit:
#ifdef PROFILING_SUPPORTED
    // If profiling, then send the pModule event so load time may be measured.
    if (CORProfilerTrackModuleLoads())
        g_profControlBlock.pProfInterface->ModuleLoadFinished((ThreadID) GetThread(), (ModuleID) this, hr);
#endif // PROFILING_SUPPORTED

    return S_OK;
}

void InMemoryModule::Destruct()
{
    if (m_pCeeFileGen)  
        m_pCeeFileGen->Release();   
    Module::Destruct();
}

REFIID InMemoryModule::ModuleType()
{
    return IID_ICorModule;  
}

BYTE* InMemoryModule::GetILCode(DWORD target) const // virtual
{
    BYTE* pByte = NULL;
    m_pCeeFileGen->GetMethodBuffer(target, &pByte); 
    return pByte;
}

BYTE* InMemoryModule::ResolveILRVA(DWORD target, BOOL hasRVA) const // virtual
{
    // This function should be call only if the target is a field or a field with RVA.
    BYTE* pByte = NULL;
    if (hasRVA == TRUE)
    {
        m_pCeeFileGen->ComputePointer(m_sdataSection, target, &pByte); 
        return pByte;
    }
    else
        return ((BYTE*) (target + ((Module *)this)->GetILBase()));
}


// ===========================================================================
// ReflectionModule
// ===========================================================================

HRESULT ReflectionModule::Init(REFIID riidCeeGen)
{
    HRESULT     hr = E_FAIL;    
    VARIANT     varOption;

    // Set the option on the dispenser turn on duplicate check for TypeDef and moduleRef
    V_VT(&varOption) = VT_UI4;
    V_I4(&varOption) = MDDupDefault | MDDupTypeDef | MDDupModuleRef | MDDupExportedType | MDDupAssemblyRef | MDDupPermission;
    hr = GetDispenser()->SetOption(MetaDataCheckDuplicatesFor, &varOption);
    _ASSERTE(SUCCEEDED(hr));

    // turn on the thread safety!
    V_I4(&varOption) = MDThreadSafetyOn;
    hr = GetDispenser()->SetOption(MetaDataThreadSafetyOptions, &varOption);
    _ASSERTE(SUCCEEDED(hr));

    // turn on the thread safety!
    V_I4(&varOption) = MDThreadSafetyOn;
    hr = GetDispenser()->SetOption(MetaDataThreadSafetyOptions, &varOption);
    _ASSERTE(SUCCEEDED(hr));

    IfFailRet(InMemoryModule::Init(riidCeeGen));

    m_pInMemoryWriter = new RefClassWriter();   
    if (!m_pInMemoryWriter)
        return E_OUTOFMEMORY; 

    m_pInMemoryWriter->Init(GetCeeGen(), GetEmitter()); 

    SetReflection();

    m_ppISymUnmanagedWriter = NULL;
    m_pFileName = NULL;

    return S_OK;  

}

void ReflectionModule::Destruct()
{
    delete m_pInMemoryWriter;   

    if (m_pFileName)
    {
        delete [] m_pFileName;
        m_pFileName = NULL;
    }

    if (m_ppISymUnmanagedWriter)
    {
        Module::ReleaseIUnknown((IUnknown**)m_ppISymUnmanagedWriter);
        m_ppISymUnmanagedWriter = NULL;
    }

    InMemoryModule::Destruct();
}

REFIID ReflectionModule::ModuleType()
{
    return IID_ICorReflectionModule;    
}

// ===========================================================================
// CorModule
// ===========================================================================

// instantiate an ICorModule interface
HRESULT STDMETHODCALLTYPE CreateICorModule(REFIID riid, void **pCorModule)
{
    if (! pCorModule)   
        return E_POINTER;   
    *pCorModule = NULL; 

    CorModule *pModule = new CorModule();   
    if (!pModule)   
        return E_OUTOFMEMORY;   

    InMemoryModule *pInMemoryModule = NULL; 

    if (riid == IID_ICorModule) {   
        pInMemoryModule = new InMemoryModule();   
    } else if (riid == IID_ICorReflectionModule) {  
        pInMemoryModule = new ReflectionModule(); 
    } else {    
        delete pModule; 
        return E_NOTIMPL;   
    }   

    if (!pInMemoryModule) { 
        delete pModule; 
        return E_OUTOFMEMORY;   
    }   
    pModule->SetModule(pInMemoryModule);    
    pModule->AddRef();  
    *pCorModule = pModule;  
    return S_OK;    
}

STDMETHODIMP CorModule::QueryInterface(REFIID riid, void** ppv)
{
    *ppv = NULL;

    if (riid == IID_IUnknown)
        *ppv = (IUnknown*)(ICeeGen*)this;
    else if (riid == IID_ICeeGen)   
        *ppv = (ICorModule*)this;
    if (*ppv == NULL)
        return E_NOINTERFACE;
    AddRef();   
    return S_OK;
}

STDMETHODIMP_(ULONG) CorModule::AddRef(void)
{
    return InterlockedIncrement(&m_cRefs);
}
 
STDMETHODIMP_(ULONG) CorModule::Release(void)
{
    if (InterlockedDecrement(&m_cRefs) == 0)    
        // the actual InMemoryModule will be deleted when EE goes through it's list 
        delete this;    
    return 1;   
}

CorModule::CorModule()
{
    m_pModule = NULL;   
    m_cRefs = 0;    
}

STDMETHODIMP CorModule::Initialize(DWORD flags, REFIID riidCeeGen, REFIID riidEmitter)
{
    return m_pModule->Init(riidCeeGen);
}

STDMETHODIMP CorModule::GetCeeGen(ICeeGen **pCeeGen)
{
    if (!pCeeGen)   
        return E_POINTER;   
    if (!m_pModule) 
        return E_FAIL;  
    *pCeeGen = m_pModule->GetCeeGen();  
    if (! *pCeeGen) 
        return E_FAIL;  
    (*pCeeGen)->AddRef();   
    return S_OK;    
}

STDMETHODIMP CorModule::GetMetaDataEmit(IMetaDataEmit **pEmitter)
{
    *pEmitter = 0;
    _ASSERTE(!"Can't get scopeless IMetaDataEmit from EE");
    return E_NOTIMPL;
}



// ===========================================================================
// VASigCookies
// ===========================================================================

//==========================================================================
// Enregisters a VASig. Returns NULL for failure (out of memory.)
//==========================================================================
VASigCookie *Module::GetVASigCookie(PCCOR_SIGNATURE pVASig, Module *pScopeModule)
{
    VASigCookieBlock *pBlock;
    VASigCookie      *pCookie;

    if (pScopeModule == NULL)
        pScopeModule = this;

    pCookie = NULL;

    // First, see if we already enregistered this sig.
    // Note that we're outside the lock here, so be a bit careful with our logic
    for (pBlock = m_pVASigCookieBlock; pBlock != NULL; pBlock = pBlock->m_Next)
    {
        for (UINT i = 0; i < pBlock->m_numcookies; i++)
        {
            if (pVASig == pBlock->m_cookies[i].mdVASig)
            {
                pCookie = &(pBlock->m_cookies[i]);
                break;
            }
        }
    }

    if (!pCookie)
    {
        // If not, time to make a new one.

        // Compute the size of args first, outside of the lock.

        DWORD sizeOfArgs = MetaSig::SizeOfActualFixedArgStack(pScopeModule, pVASig, 
                                              (*pVASig & IMAGE_CEE_CS_CALLCONV_HASTHIS)==0);


        // enable gc before taking lock

        BEGIN_ENSURE_PREEMPTIVE_GC();
        m_pCrst->Enter();
        END_ENSURE_PREEMPTIVE_GC();

        // Note that we were possibly racing to create the cookie, and another thread
        // may have already created it.  We could put another check 
        // here, but it's probably not worth the effort, so we'll just take an 
        // occasional duplicate cookie instead.

        // Is the first block in the list full?
        if (m_pVASigCookieBlock && m_pVASigCookieBlock->m_numcookies 
            < VASigCookieBlock::kVASigCookieBlockSize)
        {
            // Nope, reserve a new slot in the existing block.
            pCookie = &(m_pVASigCookieBlock->m_cookies[m_pVASigCookieBlock->m_numcookies]);
        }
        else
        {
            // Yes, create a new block.
            VASigCookieBlock *pNewBlock = new VASigCookieBlock();
            if (pNewBlock)
            {
                pNewBlock->m_Next = m_pVASigCookieBlock;
                pNewBlock->m_numcookies = 0;
                m_pVASigCookieBlock = pNewBlock;
                pCookie = &(pNewBlock->m_cookies[0]);
            }
        }

        // Now, fill in the new cookie (assuming we had enough memory to create one.)
        if (pCookie)
        {
            pCookie->mdVASig = pVASig;
            pCookie->pModule  = pScopeModule;
            pCookie->pNDirectMLStub = NULL;
            pCookie->sizeOfArgs = sizeOfArgs;
        }

        // Finally, now that it's safe for ansynchronous readers to see it, 
        // update the count.
        m_pVASigCookieBlock->m_numcookies++;

        m_pCrst->Leave();
    }

    return pCookie;
}


VOID VASigCookie::Destruct()
{
    if (pNDirectMLStub)
        pNDirectMLStub->DecRef();
}

// ===========================================================================
// LookupMap
// ===========================================================================

DWORD LookupMap::Find(void *pointer)
{
    LookupMap *map = this;
    DWORD index = 0;
    while (map != NULL)
    {
        void **p = map->pTable + index;
        void **pEnd = map->pTable + map->dwMaxIndex;
        while (p < pEnd)
        {
            if (*p == pointer)
                return (DWORD)(p - map->pTable);
            p++;
        }
        index = map->dwMaxIndex;
        map = map->pNext;
    }

    return 0;
}

// -------------------------------------------------------
// Stub manager for thunks.
//
// Note, the only reason we have this stub manager is so that we can recgonize UMEntryThunks for IsTransitionStub. If it
// turns out that having a full-blown stub manager for these things causes problems else where, then we can just attach
// a range list to the thunk heap and have IsTransitionStub check that after checking with the main stub manager.
// -------------------------------------------------------

ThunkHeapStubManager *ThunkHeapStubManager::g_pManager = NULL;

BOOL ThunkHeapStubManager::Init()
{
    g_pManager = new ThunkHeapStubManager();
    if (g_pManager == NULL)
        return FALSE;

    StubManager::AddStubManager(g_pManager);

    return TRUE;
}


BOOL ThunkHeapStubManager::CheckIsStub(const BYTE *stubStartAddress)
{
    LOG((LF_CORDB, LL_INFO1000000,
         "ThunkHeapStubManager::CheckIsStub stubStartAddress = 0x%x\n", stubStartAddress));

    // Its a stub if its in our heaps range.
    return m_rangeList.IsInRange(stubStartAddress);
}

BOOL ThunkHeapStubManager::DoTraceStub(const BYTE *stubStartAddress, 
                                       TraceDestination *trace)
{
    // We never trace through these stubs when stepping through managed code. The only reason we have this stub manager
    // is so that IsTransitionStub can recgonize UMEntryThunks.
    return FALSE;
}


