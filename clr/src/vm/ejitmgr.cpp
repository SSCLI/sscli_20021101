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
/*++

Module Name: EjitMgr.cpp

Abstract: EconojitManager Implementation
--*/

#include "common.h"
#include "excep.h"
#include "stubmgr.h"
#include "gms.h"        
#include "frames.h"
#include "ejitmgr.h"
#include "jitinterface.h"
#include "dbginterface.h"
#include "fjit_eetwain.h"
#include "eeconfig.h"
#define USE_EH_DECODER
#include "ehencoder.cpp"
#undef USE_EH_DECODER
#include "jitperf.h"
#include "wsperf.h"
#include "perfcounters.h"
#include "perflog.h"

#define SUPPORT_CODE_PITCH_TRIGGER
#define SUPPORT_MAX_UNPITCHED_PER_THREAD

unsigned  EconoJitManager::m_cMethodsJitted;        // number of methods jitted since last pitch
unsigned  EconoJitManager::m_cCalleeRejits;         // number of callees jitted since last pitch
unsigned  EconoJitManager::m_cCallerRejits;         // number of callers jitted since last pitch
EconoJitManager::JittedMethodInfo**   EconoJitManager::m_PreserveCandidates;   // methods that are possible candidates for pitching
unsigned  EconoJitManager::m_MaxUnpitchedPerThread = DEFAULT_MAX_PRESERVES_PER_THREAD;   // maximum number of methods in each thread that will be pitched
unsigned  EconoJitManager::m_PreserveCandidates_size=0;  // current size of m_PreserveCandidates array

EconoJitManager::JittedMethodInfo** EconoJitManager::m_PreserveEhGcInfoList;
unsigned EconoJitManager::m_cPreserveEhGcInfoList=0;
unsigned EconoJitManager::m_PreserveEhGcInfoList_size=DEFAULT_PRESERVED_EHGCINFO_SIZE;
Crst*               EconoJitManager::m_pRejitCritSec;
BYTE                EconoJitManager::m_RejitCritSecInstance[sizeof(Crst)];
Crst*               EconoJitManager::m_pThunkCritSec;
BYTE                EconoJitManager::m_ThunkCritSecInstance[sizeof(Crst)];
EconoJitManager::ThunkBlock*         EconoJitManager::m_ThunkBlocks;
EconoJitManager::PitchedCodeThunk*   EconoJitManager::m_FreeThunkList;
unsigned            EconoJitManager::m_cThunksInCurrentBlock;        // total number of thunks in current block
BOOL                EconoJitManager::m_PitchOccurred = false;
EconoJitManager::TICKS               EconoJitManager::m_CumulativePitchOverhead=0;
EconoJitManager::TICKS               EconoJitManager::m_AveragePitchOverhead=MINIMUM_PITCH_OVERHEAD;
unsigned            EconoJitManager::m_cPitchCycles=0;      

EconoJitManager::JittedMethodInfoHdr* EconoJitManager::m_JittedMethodInfoHdr;
EconoJitManager::JittedMethodInfo*    EconoJitManager::m_JMIT_free;
EconoJitManager::Link*                EconoJitManager::m_JMIT_freelist;
EconoJitManager::PCToMDMap*           EconoJitManager::m_PcToMdMap;         
unsigned                              EconoJitManager::m_PcToMdMap_len = 0;
unsigned                              EconoJitManager::m_PcToMdMap_size = INITIAL_PC2MD_MAP_SIZE;
EconoJitManager::PC2MDBlock*          EconoJitManager::m_RecycledPC2MDMaps = NULL;
EconoJitManager::LargeEhGcInfoList*   EconoJitManager::m_LargeEhGcInfo = NULL;

HINSTANCE                             EconoJitManager::m_JITCompiler;
BYTE*                                 EconoJitManager::m_CodeHeap;
BYTE*     EconoJitManager::m_CodeHeapFree;
size_t    EconoJitManager::m_CodeHeapCommittedSize;
size_t    EconoJitManager::m_CodeHeapReservedSize;
size_t    EconoJitManager::m_CodeHeapReserveIncrement;
size_t    EconoJitManager::m_CodeHeapTargetSize;
EconoJitManager::EHGCBlockHdr*       EconoJitManager::m_EHGCHeap;
unsigned char*      EconoJitManager::m_EHGC_alloc_end;      // ptr to next free byte in current block
unsigned char*      EconoJitManager::m_EHGC_block_end;      // ptr to end of current block
Crst*               EconoJitManager::m_pHeapCritSec;
BYTE                EconoJitManager::m_HeapCritSecInstance[sizeof(Crst)];
EjitStubManager*    EconoJitManager::m_stubManager = NULL;
EconoJitManager::TICKS               EconoJitManager::m_EjitStartTime;

#ifdef _DEBUG
DWORD               EconoJitManager::m_RejitLock_Holder = 0;
DWORD               EconoJitManager::m_AllocLock_Holder = 0; 
#endif

//#define DEBUG_LOG(str,size)  printf("\n%s - %x B",str,size)

inline size_t EconoJitManager::minimum(size_t x, size_t y)
{
    return (x < y) ? x : y;
}
        
inline size_t     EconoJitManager::InitialCodeHeapSize()
{
  return minimum(g_pConfig->GetMaxCodeCacheSize(),
                 max(m_CodeHeapTargetSize, DEFAULT_CODE_HEAP_RESERVED_SIZE));
}

EconoJitManager::EconoJitManager()
{
    m_EjitStartTime = GET_TIMESTAMP();
    m_JittedMethodInfoHdr = NULL;
    m_PcToMdMap = new PCToMDMap[m_PcToMdMap_size];
    _ASSERTE(m_PcToMdMap);

    m_JMIT_free = NULL;
    m_JMIT_freelist = NULL;
    InitializeCodeHeap();
    growJittedMethodInfoTable();
    m_EHGCHeap = NULL;
    m_EHGC_alloc_end = 0;
    m_EHGC_block_end = 0;
    m_pHeapCritSec = new  (&m_HeapCritSecInstance) Crst("EJitHeapCrst",CrstSingleUseLock);
    
    m_next = NULL;
    m_MaxUnpitchedPerThread = g_pConfig->GetMaxUnpitchedPerThread();
    m_cThunksInCurrentBlock = 0;
    m_ThunkBlocks = NULL;
    m_FreeThunkList = NULL;

    m_pRejitCritSec = new  (&m_RejitCritSecInstance) Crst("EJitRejitCrst",CrstClassInit, TRUE, FALSE);
    m_pThunkCritSec = new  (&m_ThunkCritSecInstance) Crst("EJitThunkCrst",CrstClassInit);
  
    if ((m_PreserveCandidates = new pJittedMethodInfo[m_MaxUnpitchedPerThread]) != NULL)
    {
        m_PreserveCandidates_size = m_MaxUnpitchedPerThread;
        memset(m_PreserveCandidates,0,m_MaxUnpitchedPerThread*sizeof(void*));
    }

    // initialize buffer for collecting list of methodinfo's whose GC info is preserved during code pitch
    m_PreserveEhGcInfoList = new (throws) pJittedMethodInfo[DEFAULT_PRESERVED_EHGCINFO_SIZE];
    if (m_PreserveEhGcInfoList == NULL)
    {
        m_PreserveEhGcInfoList_size = 0;
    }

    m_stubManager = new (throws) EjitStubManager();
    StubManager::AddStubManager(m_stubManager);

}


EconoJitManager::~EconoJitManager()
{ 
    if (m_PcToMdMap)
        delete [] m_PcToMdMap;
    delete m_pHeapCritSec;
    delete m_pRejitCritSec;
    delete m_pThunkCritSec;
    if (m_PreserveCandidates)
        delete [] m_PreserveCandidates;
    delete m_stubManager;
    while (m_LargeEhGcInfo)
    {
        LargeEhGcInfoList* temp = m_LargeEhGcInfo;
        m_LargeEhGcInfo = m_LargeEhGcInfo->next;
        delete temp;
    }

#if defined(ENABLE_PERF_LOG) 
    double TickFrequency = (double) TICK_FREQUENCY();
    double totalExecTime = (double) (GET_TIMESTAMP() - m_EjitStartTime);
    
    PERFLOG((L"Total pitch overhead", ((double) m_CumulativePitchOverhead)/TickFrequency, SECONDS));
    PERFLOG((L"Total pitch cycles", m_cPitchCycles, CYCLES));
    PERFLOG((L"Avg. Pitch Overhead", ((double) m_AveragePitchOverhead)/TickFrequency, SECONDS));
    PERFLOG((L"Total Execution Time", totalExecTime/TickFrequency, SECONDS));

    PERFLOG((L"Total Code Heap Reserved", (UINT_PTR)(m_CodeHeapReservedSize/1024), KBYTES));
    PERFLOG((L"Total Code Heap Committed", (UINT_PTR)(m_CodeHeapCommittedSize/1024), KBYTES));
    PERFLOG((L"Total EhGc Heap", (UINT_PTR)(m_EHGCHeap != NULL ? m_EHGCHeap->blockSize/1024 : 0 ), KBYTES));
    
    JittedMethodInfoHdr* pJMIT = m_JittedMethodInfoHdr;
    unsigned jmitHeapSize = 0;
    while (pJMIT) {
        jmitHeapSize += JMIT_BLOCK_SIZE;
        pJMIT = pJMIT->next;
    }

    PERFLOG((L"Total JMIT Heap", jmitHeapSize/1024, KBYTES));

    unsigned thunkHeapSize = 0;

    ThunkBlock* thunkBlock = m_ThunkBlocks;
    while (thunkBlock) {
        thunkHeapSize += THUNK_BLOCK_SIZE;
        thunkBlock = thunkBlock->next;
    }

    PERFLOG((L"Total Thunk Heap", thunkHeapSize/1024, KBYTES));

    PERFLOG((L"Total Pc2MD map", m_PcToMdMap_size/1024, KBYTES));
    PERFLOG((L"Total EJIT working set",
        (UINT_PTR) ((m_CodeHeapCommittedSize+
                     (m_EHGCHeap != NULL ? m_EHGCHeap->blockSize : 0)+
                     jmitHeapSize+thunkHeapSize+
                     m_PcToMdMap_size)/1024), KBYTES));
#endif
     delete[] m_PreserveEhGcInfoList;

     ResetPc2MdMap();
}

void EconoJitManager::InitializeCodeHeap()
{    
    m_CodeHeapTargetSize = g_pConfig->GetTargetCodeCacheSize();
    _ASSERTE(m_CodeHeapTargetSize <= (unsigned) g_pConfig->GetMaxCodeCacheSize());
    size_t initialCodeHeapSize =InitialCodeHeapSize();

    m_CodeHeap = (BYTE*) VirtualAlloc(NULL,
                                      initialCodeHeapSize,
                                      MEM_RESERVE,
                                      PAGE_EXECUTE_READWRITE);
    if (m_CodeHeap)
    {
        //DEBUG_LOG("ALLOC(InitialCodeHeap",initialCodeHeapSize);
        ExecutionManager::AddRange((LPVOID)m_CodeHeap, (LPVOID)((size_t)m_CodeHeap + initialCodeHeapSize),this, NULL);
        m_CodeHeapReservedSize = initialCodeHeapSize;
    }
    else
    {
        m_CodeHeapReservedSize =  0;
    }
    m_CodeHeapReserveIncrement = initialCodeHeapSize;
    _ASSERTE(m_CodeHeapReserveIncrement != 0);
    m_CodeHeapFree = m_CodeHeap;
    m_CodeHeapCommittedSize = 0;
}

__inline MethodDesc* EconoJitManager::JitCode2MethodDesc(SLOT currentPC, IJitManager::ScanFlag scanFlag)
{
    return JitCode2MethodDescStatic(currentPC);
}

MethodDesc* EconoJitManager::JitCode2MethodDescStatic(SLOT currentPC)
{
    // First see if currentPC is the stub address
    JittedMethodInfoHdr* pJMIT = m_JittedMethodInfoHdr;
    while (pJMIT) {
        if ((currentPC >= (SLOT)pJMIT) && 
            (currentPC < ((SLOT)pJMIT + JMIT_BLOCK_SIZE)))
        {// found it
            _ASSERTE(offsetof(JittedMethodInfo,JmpInstruction) == 0);
            return JitMethodInfo2MethodDesc((JittedMethodInfo*) currentPC);
        }
        pJMIT = pJMIT->next;
    }

    _ASSERTE(m_PcToMdMap_len);

    signed low, mid, high;
    low = 0;
    high = (m_PcToMdMap_len/ sizeof(PCToMDMap)) - 1;
    while (low < high) {
        // loop invariant
        _ASSERTE((size_t)m_PcToMdMap[high].pCodeEnd >= (size_t)currentPC );

        mid = (low+high)/2;
        if ( (size_t)m_PcToMdMap[mid].pCodeEnd < (size_t)currentPC ) {
            low = mid+1;
        }
        else {
            high = mid;
        }
    }
    _ASSERTE((size_t)m_PcToMdMap[low].pCodeEnd >= (size_t)currentPC);

    return m_PcToMdMap[low].pMD;
}

EconoJitManager::JittedMethodInfo*  EconoJitManager::JitCode2MethodInfo(SLOT currentPC)
{
    MethodDesc* pMD = JitCode2MethodDescStatic((SLOT) currentPC);


    const BYTE* stubAddr = pMD->GetNativeAddrofCode();
#ifdef _DEBUG
    // sanity check the stub address
    JittedMethodInfoHdr* pJMIT = m_JittedMethodInfoHdr;
    BOOL found = false;
    while (pJMIT) {
        if (((size_t)stubAddr >= (size_t)pJMIT) && 
            ((size_t)stubAddr < ((size_t)pJMIT) + JMIT_BLOCK_SIZE))
        {
            found = true;
            break;
        }
        pJMIT = pJMIT->next;
    }
    _ASSERTE(found);
#endif
    _ASSERTE(offsetof(JittedMethodInfo,JmpInstruction) == 0);
    return (JittedMethodInfo*) stubAddr;
    
}



void  EconoJitManager::JitCode2MethodTokenAndOffsetStatic(SLOT currentPC, METHODTOKEN* pMethodToken, DWORD* pPCOffset)
{
    ThunkBlock*  pThunkBlock = m_ThunkBlocks; 
    while (pThunkBlock) 
    {
        _ASSERTE(pThunkBlock);
        if ((SLOT) pThunkBlock < currentPC && currentPC < ((SLOT)pThunkBlock+THUNK_BLOCK_SIZE))
        {
            // This is a thunk
            PitchedCodeThunk* pThunk = (PitchedCodeThunk*) ((size_t)currentPC & THUNK_BEGIN_MASK );
            *pMethodToken = (METHODTOKEN)pThunk->u.pMethodInfo;
            // Make sure this thunk hasn't been recycled
            _ASSERTE(pThunk->relReturnAddress != 0xffffffff );
            *pPCOffset = pThunk->relReturnAddress;
            return;
        }
        pThunkBlock=pThunkBlock->next;
    }  

    JittedMethodInfo* jittedMethodInfo = JitCode2MethodInfo(currentPC);
    _ASSERTE(jittedMethodInfo);
    *pMethodToken = (METHODTOKEN) jittedMethodInfo;
    if (IsInStub(currentPC, FALSE))
    {
        *pPCOffset = 0;
        return;
    }

    _ASSERTE(!(jittedMethodInfo->flags.JittedMethodPitched));   // error to call if method has been pitched

    EJCodeHeader* pCodeHeader = jittedMethodInfo->u1.pCodeHeader;
    _ASSERTE(pCodeHeader && (((size_t)pCodeHeader & 1) == 0));

    SLOT startAddress = (SLOT) (pCodeHeader+1);
    _ASSERTE(currentPC >= startAddress);
    *pPCOffset = (DWORD)(currentPC - startAddress);
}

void  EconoJitManager::JitCode2MethodTokenAndOffset(SLOT currentPC, METHODTOKEN* pMethodToken,DWORD* pPCOffset, ScanFlag scanFlag)
{
        JitCode2MethodTokenAndOffsetStatic(currentPC,pMethodToken,pPCOffset);
}

EconoJitManager::JittedMethodInfo*  EconoJitManager::JitCode2MethodTokenInEnCMode(SLOT currentPC)
{
    return NULL;
}

unsigned char* EconoJitManager::JitToken2StartAddress(METHODTOKEN MethodToken, ScanFlag scanFlag)
{
    return JitToken2StartAddressStatic(MethodToken);
}

unsigned char* EconoJitManager::JitToken2StartAddressStatic(METHODTOKEN MethodToken)
{
    JittedMethodInfo* jittedMethodInfo = (JittedMethodInfo*) MethodToken;  
    _ASSERTE(!(jittedMethodInfo->flags.JittedMethodPitched));   // error to call if method has been pitched

    EJCodeHeader* pCodeHeader = jittedMethodInfo->u1.pCodeHeader;
    _ASSERTE(pCodeHeader && ( ((size_t)pCodeHeader & 1) == 0));

    return (BYTE*) (pCodeHeader+1);
}

BOOL EconoJitManager::ProtectThunk( const BYTE * address, bool fProtect )
{
  // Find if this is a thunk
  ThunkBlock*  pThunkBlock = m_ThunkBlocks;
  while (pThunkBlock) 
  {
      _ASSERTE(pThunkBlock);
      if ((SLOT) pThunkBlock < address && address < ((SLOT)pThunkBlock+THUNK_BLOCK_SIZE))
      {
          // This is a thunk
          PitchedCodeThunk* pThunk = (PitchedCodeThunk*) ((size_t)address & THUNK_BEGIN_MASK);
          // Make sure this thunk hasn't been recycled
          _ASSERTE(pThunk->relReturnAddress != 0xffffffff );

          if (fProtect)
	  {
              // Protect the thunk by marking it as linked in the free list without actually adding it to the 
              // free list
              pThunk->LinkedInFreeList = true;
          }
          else
	  {
              // Reset the thunk flags into normal state
              pThunk->Busy = true;
              pThunk->LinkedInFreeList = false;
	  }

          // The thunk was found
          return true;
        }
        pThunkBlock=pThunkBlock->next;
  }  
  // The address didn't point to a thunk
  return false;
}

const BYTE* EconoJitManager::FollowStub(const BYTE* address)
{
#ifdef _X86_
    _ASSERTE(address[0] == CALL_OPCODE || address[0] == JMP_OPCODE || address[0] == BREAK_OPCODE);
    return (address+1 + (*(DWORD*) (address+1))+sizeof(void*)) ;
#elif defined(_PPC_) || defined(_SPARC_)
    return *(BYTE**)(address+16);
#else
    PORTABILITY_ASSERT("@NYI - EconoJitManager::FollowStub (EjitMgr.cpp)");
    return NULL;
#endif
}

BOOL EconoJitManager::IsInStub(const BYTE* address, BOOL fSearchThunks)
{
    JittedMethodInfoHdr* pJMIT = m_JittedMethodInfoHdr;
    while (pJMIT)
    {
        if ( ((BYTE*)pJMIT < address) && (address < ((BYTE*)pJMIT + JMIT_BLOCK_SIZE)) )
        {
            return true;       // the only way to be caught here is through an asynchronous exception
        }
        pJMIT = pJMIT->next;
    }

    if (fSearchThunks == TRUE )
    {
        ThunkBlock *ptb = m_ThunkBlocks;
        while( ptb!= NULL)
        {
            if(address>=(const BYTE*)ptb  && 
                address < ( ((const BYTE*)ptb)+THUNK_BLOCK_SIZE))
            {
                return true;
            }
            ptb = ptb->next;
        }
    }

    return false;
}

//
// Returns TRUE if the address is in a method that has been pitched.
//
BOOL EconoJitManager::IsCodePitched(const BYTE* address)
{
    METHODTOKEN methodToken;
    DWORD pcOffset;
    
    JitCode2MethodTokenAndOffsetStatic((SLOT)address,
                                       &methodToken,
                                       &pcOffset);

    _ASSERTE(methodToken != NULL);

    JittedMethodInfo *jmi = (JittedMethodInfo*)methodToken;

    return jmi->flags.JittedMethodPitched;
}


__inline MethodDesc* EconoJitManager::JitTokenToMethodDesc(METHODTOKEN MethodToken, ScanFlag scanFlag)
{
    return JitMethodInfo2MethodDesc((JittedMethodInfo*) MethodToken);
}

unsigned EconoJitManager::InitializeEHEnumeration(METHODTOKEN MethodToken, EH_CLAUSE_ENUMERATOR* pEnumState)
{
    JittedMethodInfo* jittedMethodInfo = (JittedMethodInfo*) MethodToken;
    if (jittedMethodInfo->flags.EHInfoExists == 0)      // if no eh with this method return 0
        return 0;
    _ASSERTE(jittedMethodInfo->flags.EHandGCInfoPitched == 0);
    
    BYTE* EhInfo = jittedMethodInfo->u2.pEhGcInfo;

    if ((size_t)EhInfo & 1)
        EhInfo = (BYTE*) ((size_t)EhInfo & ~1);       // lose the mark bit
    else // code has not been pitched, and it is guaranteed to not be pitched while we are here
    {
        EJCodeHeader* pCodeHeader = jittedMethodInfo->u1.pCodeHeader;
        _ASSERTE(pCodeHeader && (( (size_t)pCodeHeader & 1) == 0));
        EH_OR_GC_INFO* EhGcInfo = (EH_OR_GC_INFO*) (pCodeHeader-1);
        EhInfo =  (BYTE*) (EhGcInfo->EH);
    }
    unsigned retval;
    // 2 bytes are used to encode length of EHinfo
    *pEnumState = 2 + EHEncoder::decode(EhInfo+2,&retval);     // read number of bytes used by encoded eh info
    _ASSERTE(retval);

    return retval;
    
}

EE_ILEXCEPTION_CLAUSE*  EconoJitManager::GetNextEHClause(METHODTOKEN MethodToken,
                                       EH_CLAUSE_ENUMERATOR* pEnumState, 
                                       EE_ILEXCEPTION_CLAUSE* pEHclause) 
{

    JittedMethodInfo* jittedMethodInfo = (JittedMethodInfo*) MethodToken;
    _ASSERTE(jittedMethodInfo->flags.EHInfoExists != 0);

    BYTE* EhInfo = jittedMethodInfo->u2.pEhGcInfo;

    if ((size_t)EhInfo & 1)
    {
        EhInfo = (BYTE*) ((size_t)EhInfo & ~1);       // lose the mark bit
    }
    else    // code has not been pitched, and it is guaranteed to not be pitched while we are here
    {
        EJCodeHeader* pCodeHeader = jittedMethodInfo->u1.pCodeHeader;
        _ASSERTE(pCodeHeader && (( (size_t)pCodeHeader & 1) == 0));
        EH_OR_GC_INFO* EhGcInfo = (EH_OR_GC_INFO*) (pCodeHeader-1);
        EhInfo = (BYTE*) (EhGcInfo->EH);
    }
    _ASSERTE(EhInfo);
    unsigned cBytes;
    cBytes = EHEncoder::decode(EhInfo+(*pEnumState),(CORINFO_EH_CLAUSE *) pEHclause);
    (*pEnumState) += cBytes;
    return pEHclause;

}

void  EconoJitManager::ResolveEHClause(METHODTOKEN MethodToken,
                                       EH_CLAUSE_ENUMERATOR* pEnumState, 
                                       EE_ILEXCEPTION_CLAUSE* pEHclause)
{
    // Resolve to class if defined in an *already loaded* scope. Don't cache in ejit for now
    JittedMethodInfo* jittedMethodInfo = (JittedMethodInfo*) MethodToken;
    Module *pModule = JitMethodInfo2MethodDesc(jittedMethodInfo)->GetModule();
    
    m_pHeapCritSec->Enter();
    if (! HasCachedEEClass(pEHclause))
    {
        NameHandle name(pModule, (mdToken)pEHclause->ClassToken);
        name.SetTokenNotToLoad(tdAllTypes);
        TypeHandle typeHnd = pModule->GetClassLoader()->LoadTypeHandle(&name);
        if (!typeHnd.IsNull())
        {
            pEHclause->pEEClass = typeHnd.GetClass();
            SetHasCachedEEClass(pEHclause);
        }
    }
    m_pHeapCritSec->Leave();
}

void* EconoJitManager::GetGCInfo(METHODTOKEN methodToken)
{
    JittedMethodInfo* jittedMethodInfo = (JittedMethodInfo*) methodToken;
    _ASSERTE(jittedMethodInfo->flags.GCInfoExists != 0);        // for now, we always emit GC info

    // Unfortunately I cant assert the following because there is one case where a thread might be
    // stopped in the ejit stub and the method may have been pitched.
    //_ASSERTE(!jittedMethodInfo->flags.EHandGCInfoPitched);

    // The following is still safe, because GC cannot be asking for this information since it
    // first needs to stop us in a safe place. So presumably HandledJitCase is asking for this info
    // to pass to the code manager. The fjit code manager always returns false (even if the debugger is
    // attached it will return false, since the stub is not a sequence point).
    if (jittedMethodInfo->flags.EHandGCInfoPitched)
        return 0;

    BYTE* pEhGcInfo = jittedMethodInfo->u2.pEhGcInfo;
    if ((size_t)pEhGcInfo & 1)
        pEhGcInfo = (BYTE*) ((size_t)pEhGcInfo & ~1);       // lose the mark bit
    else    // code has not been pitched, and it is guaranteed to not be pitched while we are here
    {
        EJCodeHeader* pCodeHeader = jittedMethodInfo->u1.pCodeHeader;
        _ASSERTE(pCodeHeader && (((size_t)pCodeHeader & 1) == 0));
        pEhGcInfo = *(BYTE**) (pCodeHeader-1);
    }
    _ASSERTE(pEhGcInfo);

    if (jittedMethodInfo->flags.EHInfoExists)
    {
        short cEHbytes = *(short*) pEhGcInfo;
        return (GC_INFO*) (pEhGcInfo + cEHbytes);
    }
    else
        return (GC_INFO*) pEhGcInfo;

}

BOOL  EconoJitManager::LoadJIT(LPCWSTR wzJITdll)
{
    return IJitManager::LoadJIT(wzJITdll);
}

void EconoJitManager::RemoveJitData (METHODTOKEN token)
{
    _ASSERTE(!"NYI");

}

// Class is being unloaded, so remove any info for this method

void EconoJitManager::Unload(MethodDesc *pFD, UnloadStage unloadStage)
{
    JittedMethodInfo* jmi = (JittedMethodInfo*)  pFD->GetAddrofCode();
    if (!jmi->flags.JittedMethodPitched)
        PitchAllJittedMethods(m_CodeHeapCommittedSize,m_CodeHeapCommittedSize,TRUE,TRUE);
    
    // link the freed jmi to the freelist
    jmi->flags.JittedMethodPitched = 1;
    jmi->flags.MarkedForPitching = 0;
    jmi->flags.EHInfoExists = 0;
    jmi->flags.GCInfoExists = 0;
    jmi->flags.EHandGCInfoPitched = 0;
    jmi->flags.Unused = 0;

#ifdef _DEBUG
    jmi->EhGcInfo_len = 0;
    jmi->u1.pCodeHeader = NULL;
#endif
    jmi->u1.pMethodDescriptor = NULL;

    // the freelist is protected by the m_pHeapCritSec
    m_pHeapCritSec->Enter();
    ((Link*)jmi)->next = m_JMIT_freelist;
    m_JMIT_freelist = (Link*) jmi;
    m_pHeapCritSec->Leave();
}

EXTERN_C void __cdecl CallThunk();
EXTERN_C void __cdecl RejitThunk();



EXTERN_C void __stdcall InterlockedCompareExchange8b(UINT64 *pLocation, UINT64 *pValue, UINT64 *pComparand);

void  EconoJitManager::ResumeAtJitEH(CrawlFrame* pCf, EE_ILEXCEPTION_CLAUSE *EHClausePtr, DWORD nestingLevel, Thread *pThread, BOOL unwindStack)
{
    METHODTOKEN token = pCf->GetMethodToken();
    BYTE* startAddress;

    if (IsMethodPitched(token))
    {
        PREGDISPLAY pRD = pCf->GetRegisterSet();

        m_pThunkCritSec->Enter();
        // Get a new thunk
        PitchedCodeThunk* thunk = GetNewThunk();
        // Initialize the thunk
        thunk->Busy = true;
        thunk->LinkedInFreeList = false;
        thunk->retTypeProtect = MethodDesc::RETNONOBJ;
        thunk->relReturnAddress = (unsigned) EHClausePtr->HandlerStartPC;
        _ASSERTE(thunk->relReturnAddress != 0xffffffff );
        thunk->u.pMethodInfo = (JittedMethodInfo*) token;
        // Create a transfer stub to the rejit helper  
        CreateTransferStub( (BYTE *)&(thunk->CallInstruction[0]), true, (DWORD *)RejitThunk, RejitThunkType );
        // Create a machine state explicitly
        MachState ms( pRD, (void**) &thunk);
        m_pThunkCritSec->Leave();

        HelperMethodFrame HelperFrame(&ms, 0);
        OBJECTREF pExceptionObj = pThread->GetThrowable();
        GCPROTECT_BEGIN(pExceptionObj);
        startAddress =  RejitMethod((JittedMethodInfo*) token,0);
        _ASSERT(*(&pExceptionObj) == pThread->GetThrowable());
        GCPROTECT_END();
        HelperFrame.Pop();
    }
    else 
    {
        startAddress = JitToken2StartAddress(token);
    }
    ::ResumeAtJitEH(pCf,startAddress,EHClausePtr,nestingLevel,pThread, unwindStack);
}

int  EconoJitManager::CallJitEHFilter(CrawlFrame* pCf, EE_ILEXCEPTION_CLAUSE *EHClausePtr, DWORD nestingLevel, OBJECTREF thrownObj)
{
    METHODTOKEN token = pCf->GetMethodToken();
    BYTE* startAddress;

    if (IsMethodPitched(token))
    {
        PREGDISPLAY pRD = pCf->GetRegisterSet();
        MachState ms(pRD, (void**) pRD->pPC);
        HelperMethodFrame HelperFrame(&ms, 0);
        GCPROTECT_BEGIN(thrownObj);
        startAddress =  RejitMethod((JittedMethodInfo*) token,0);
        GCPROTECT_END();
        HelperFrame.Pop();
    }
    else 
    {
        startAddress = JitToken2StartAddress(token);
    }
    return ::CallJitEHFilter(pCf,startAddress,EHClausePtr,nestingLevel,thrownObj);

}

void   EconoJitManager::CallJitEHFinally(CrawlFrame* pCf, EE_ILEXCEPTION_CLAUSE *EHClausePtr, DWORD nestingLevel)
{

    METHODTOKEN token = pCf->GetMethodToken();
    BYTE* startAddress;

    if (IsMethodPitched(token))
    {
        PREGDISPLAY pRD = pCf->GetRegisterSet();

        m_pThunkCritSec->Enter();
        PitchedCodeThunk* thunk = GetNewThunk();
        thunk->Busy = true;
        thunk->LinkedInFreeList = false;
        thunk->retTypeProtect = MethodDesc::RETNONOBJ;
        thunk->relReturnAddress = (unsigned) EHClausePtr->HandlerStartPC;
        _ASSERTE(thunk->relReturnAddress != 0xffffffff );
        thunk->u.pMethodInfo = (JittedMethodInfo*) token;
        // Create a transfer stub to the rejit helper  
        CreateTransferStub( (BYTE *)&(thunk->CallInstruction[0]), true, (DWORD *)RejitThunk, RejitThunkType );
        // Create a machine state explicitly
        MachState ms( pRD, (void**) &thunk);
        m_pThunkCritSec->Leave();
        
        HelperMethodFrame HelperFrame(&ms, 0);
        startAddress =  RejitMethod((JittedMethodInfo*) token,0);
        HelperFrame.Pop();
    }
    else 
    {
        startAddress = JitToken2StartAddress(token);
    }
    ::CallJitEHFinally(pCf,startAddress,EHClausePtr,nestingLevel);
}




HRESULT   EconoJitManager::alloc(
                                 size_t code_len, 
                                 unsigned char** ppCode,        /* IN/OUT */  // this is sort of a hack, the ejit is using this parameter to pass 
                                                                              // the code buffer so it can copy the jitted code into the allocated block
                                                                              // this is needed in order to prevent a race
                                 size_t EHinfo_len, 
                                 unsigned char** ppEHinfo,      /* OUT */
                                 size_t GCinfo_len, 
                                 unsigned char** ppGCinfo  ,    /* OUT */  
                                 MethodDesc* pMethodDescriptor
                                 )
{
    unsigned codeHeaderSize = sizeof(EJCodeHeader*);

    JittedMethodInfo* existingJMI = NULL;  


    _ASSERTE(EHinfo_len + GCinfo_len);      // this may not be true if we optimize GCinfo 
    codeHeaderSize += sizeof(void*);

    // make sure we always begin at proper machine word boundary
    unsigned adjusted_code_len = (unsigned)(((code_len+sizeof(void*)-1)/sizeof(void*)) * sizeof(void*));

    m_pHeapCritSec->Enter();

    if ( OutOfCodeMemory(adjusted_code_len+codeHeaderSize) 
            || m_cMethodsJitted == g_pConfig->GetCodePitchTrigger()
       )
    {
        size_t totalMemNeeded = adjusted_code_len + codeHeaderSize + usedMemoryInCodeHeap();

        // if we haven't hit the initial code heap size target OR we have incurred a high pitch overhead,
        // then we will try to either increase our committed space and if necessary also increase our reserved space
        // The rationale is that until we reach our target code heap size, we do not want to consider code pitching at all.
        if (m_CodeHeapReservedSize <= InitialCodeHeapSize() || PitchOverhead() >= (unsigned)g_pConfig->GetMaxPitchOverhead())
        {
            while (totalMemNeeded <= m_CodeHeapReservedSize && totalMemNeeded > m_CodeHeapCommittedSize)
            {
                m_pHeapCritSec->Leave();
                if (!SetCodeHeapCommittedSize((unsigned int) min(totalMemNeeded+PAGE_SIZE,m_CodeHeapReservedSize))  )
                {
                    return (E_FAIL);
                }
                m_pHeapCritSec->Enter();
                totalMemNeeded = (unsigned)(adjusted_code_len + codeHeaderSize + usedMemoryInCodeHeap());
            }
            // at this point we either have enough committed memory or we have hit the reserved size
            if ((totalMemNeeded > m_CodeHeapReservedSize) && 
                (m_CodeHeapReservedSize < (unsigned) g_pConfig->GetMaxCodeCacheSize()) // we haven't hid the hard upper limit
                && (m_cMethodsJitted != (unsigned) g_pConfig->GetCodePitchTrigger()) // force pitch if trigger value reached, 
                && (PitchOverhead() >= (unsigned) g_pConfig->GetMaxPitchOverhead()))
            {   //so try growing the heap instead of pitching
                size_t delta = ((totalMemNeeded - m_CodeHeapReservedSize) + m_CodeHeapReserveIncrement -1);
                delta = (delta/m_CodeHeapReserveIncrement)*m_CodeHeapReserveIncrement;
                GrowCodeHeapReservedSpace((m_CodeHeapReservedSize + delta),(adjusted_code_len+codeHeaderSize));
                if (m_CodeHeapReserveIncrement < CODE_HEAP_RESERVED_INCREMENT_LIMIT)
                    m_CodeHeapReserveIncrement *= 2;
            }      
        }

        // At this point we have attempted to meet our memory needs without pitching code, if we did not succeed, there is 
        // no option but to pitch code
        while (OutOfCodeMemory(adjusted_code_len+codeHeaderSize) 
            || (m_cMethodsJitted == g_pConfig->GetCodePitchTrigger())
            )
        {
            m_pHeapCritSec->Leave();

            if (!PitchAllJittedMethods((unsigned)(adjusted_code_len+codeHeaderSize),(unsigned)(adjusted_code_len+codeHeaderSize),true,true))       // During pitching we also adjust the cache code size if too  
                return E_FAIL;     // few or too many methods have been jitted since the last pitch   

            m_pHeapCritSec->Enter();
        }
    }
#ifdef _DEBUG
    m_AllocLock_Holder = GetCurrentThreadId();  
#endif
    m_cMethodsJitted++;

    if (m_PitchOccurred)
    {   // this could be a rejit of a method, need to find out if we already have an entry in the jmi table
         existingJMI = MethodDesc2MethodInfo(pMethodDescriptor);
        _ASSERTE(!existingJMI || existingJMI->flags.JittedMethodPitched);
    }

    unsigned char* pCodeBlock = (unsigned char*) allocCodeBlock(adjusted_code_len+codeHeaderSize);
    if (!pCodeBlock) {
        m_pHeapCritSec->Leave();
        return (E_FAIL);
    }


    // make sure we always begin at proper machine word boundary
    size_t adjusted_EhGc_len = ((EHinfo_len + GCinfo_len+1)/2) * 2;
    unsigned char* pEhGcBlock;
    if (!existingJMI || existingJMI->flags.EHandGCInfoPitched)
    {
        pEhGcBlock = (unsigned char*) allocEHGCBlock(adjusted_EhGc_len);
        if (!pEhGcBlock) {
            freeCodeBlock(adjusted_code_len+codeHeaderSize);
            m_pHeapCritSec->Leave();
            return (E_FAIL);
        }
    }
    else // no need to allocate EHGCBlock
    {
        _ASSERTE(existingJMI->flags.JittedMethodPitched); // since it doesnt make sense to pitch EhGC and retain code
        pEhGcBlock = (BYTE*) ((size_t)existingJMI->u2.pEhGcInfo & ~1);
    }

    JIT_PERF_UPDATE_X86_CODE_SIZE((unsigned)(adjusted_code_len + codeHeaderSize + EHinfo_len + GCinfo_len));
    
    //if (EHinfo_len + GCinfo_len) is zero, the following can be optimized away
    * (void**)pCodeBlock = pEhGcBlock;
    pCodeBlock += sizeof(void*);

    *((MethodDesc**)pCodeBlock) = pMethodDescriptor;
    pCodeBlock += sizeof(void*);

    // it is important to do this copy here before we change the call to a jmp in the thunk
    // otherwise we'd have a race, where a thread might pick up the new address before this 
    // thread completes the copy.
    memcpy(pCodeBlock, *ppCode, code_len);

    FlushInstructionCache(GetCurrentProcess(), pCodeBlock, code_len);

    *ppCode = pCodeBlock;
    *ppEHinfo = pEhGcBlock;


    if (existingJMI)
    {
        // we have to be careful while updating the following since other
        // threads might be reading this concurrently. It is guaranteed that
        // no one is writing into it since we hold the alloc lock and the
        // thread store lock is not held by anyone. 

        EJCodeHeader* codeHdr = (EJCodeHeader*) (pCodeBlock - sizeof(void*));
        _ASSERTE(((size_t)codeHdr & 3) == 0);
        existingJMI->u1.pCodeHeader = codeHdr; // verify that this is atomic
        BYTE* codeEnd = adjusted_code_len + (BYTE*) (pCodeBlock);
        _ASSERTE(((size_t)codeEnd & 3) == 0);
        existingJMI->u2.pCodeEnd = codeEnd;     // verify that this is atomic
        AddPC2MDMap(pMethodDescriptor,codeEnd);
        existingJMI->flags.JittedMethodPitched = false;// this should not be read unless code pitching
        existingJMI->flags.EHandGCInfoPitched = false; // this is either already false, in which case the operation is safe
                                                       // or it was true => method not in any callstack => no one can be reading it
        BYTE * JmpStub = &(existingJMI->JmpInstruction[0]);
        SwapTransferStub( JmpStub, false, (DWORD *)pCodeBlock, DefaultStubType );
#ifdef _DEBUG
        m_AllocLock_Holder = 0;
#endif
        m_pHeapCritSec->Leave();
        return (HRESULT)(DWORD_PTR)(JmpStub);
    }

    // if got here, then this is a newly jitted method
    _ASSERTE(m_JMIT_size);
    JittedMethodInfo* newJmiEntry = GetNextJmiEntry();
    if (newJmiEntry == NULL)
      return E_FAIL;

    _ASSERTE(newJmiEntry->flags.JittedMethodPitched == 0); 
    if (EHinfo_len)
    {
        newJmiEntry->flags.EHInfoExists = 1;
    }
    if (GCinfo_len)
    {
        newJmiEntry->flags.GCInfoExists = 1;
    }
    _ASSERTE(newJmiEntry->flags.EHandGCInfoPitched == 0); 

    newJmiEntry->SetEhGcInfo_len((UINT)adjusted_EhGc_len,&m_LargeEhGcInfo);

    BYTE * JmpStub = &(newJmiEntry->JmpInstruction[0]);
    CreateTransferStub( JmpStub, false, (DWORD *)pCodeBlock, DefaultStubType);
 
    newJmiEntry->u1.pCodeHeader = (EJCodeHeader*) (pCodeBlock - sizeof(void*));
    newJmiEntry->u2.pCodeEnd = adjusted_code_len + (BYTE*) (pCodeBlock);
    _ASSERTE(((size_t)newJmiEntry->u2.pCodeEnd & 1) == 0);
    AddPC2MDMap(pMethodDescriptor,newJmiEntry->u2.pCodeEnd);

#ifdef _DEBUG
    m_AllocLock_Holder = 0;  
#endif
    m_pHeapCritSec->Leave();

    return (HRESULT)(DWORD_PTR)(JmpStub);
}

EconoJitManager::JittedMethodInfo* EconoJitManager::GetNextJmiEntry()
{
#ifdef _DEBUG
    m_pHeapCritSec->OwnedByCurrentThread();
#endif
    JittedMethodInfo* newEntry;

    if (m_JMIT_freelist)
    {
        newEntry = (JittedMethodInfo*) m_JMIT_freelist;
        // Reset the pitched flag
        newEntry->flags.JittedMethodPitched = 0;
        m_JMIT_freelist = m_JMIT_freelist->next;
    }
    else //free list is empty
    {
        if ((size_t)(m_JMIT_free+1) >= ((size_t)m_JittedMethodInfoHdr)+JMIT_BLOCK_SIZE)
        {
            if (!growJittedMethodInfoTable())
                return NULL;
        }
        newEntry = m_JMIT_free;
        m_JMIT_free++;
    }
    return newEntry;
}

// It is assumed that code pitching happens synchronously at GC safe
// points. Therefore, it is not necessary to protect the pitching mechanims
// for concurrent access to the code cache. If this assumption changes, the
// code has to be appropriately protected.
BOOL EconoJitManager::PitchAllJittedMethods(size_t   minSpaceRequired,size_t   minCommittedSpaceRequired, BOOL PitchEHInfo, BOOL PitchGCInfo)
{
    if (!g_pConfig->IsCodePitchEnabled())
    {
        return FALSE;
    }
    TICKS startPitchTime = GET_TIMESTAMP();       // this is needed to record the pitching overhead
    // For now piggyback on the GC's suspend EE mechanism
    GCHeap::SuspendEE(GCHeap::SUSPEND_FOR_CODE_PITCHING);

    // ASSERT: All threads are now suspended at GC safe points
    // NOTE: In general, it is not safe to pitch the code if the debugger
    // has the thread suspended while it is executing the code. But such 
    // a point is not GC safe as far as the Ejit is concerned. In other 
    // words whenever we are pitching, the method is guaranteed to be on the
    // call stack and not at a leaf. 
    
    MarkThunksForRelease();               // tentatively mark all thunks as free

    MarkHeapsForPitching();               // tentatively, mark every method to be pitched 
    StackWalkForCodePitching();           // at the end of this all references to the code heap have
                                          // been replaced by references to thunks, also some candidates methods for preserving have
                                          // been identified
    UnmarkPreservedCandidates(minSpaceRequired);   // Guarantees that the space recovered is greater than minSpaceRequired
                                                   // OR that all methods have been marked for pitching
    MoveAllPreservedEhGcInfo();
    PitchMarkedCode();
    MovePreservedMethods();

    //_ASSERTE((unsigned) m_cMethodsJitted == cMethodsMarked);         
    m_cMethodsJitted = 0;
    m_cCalleeRejits = 0;
    m_cCallerRejits = 0;

#if defined(ENABLE_PERF_COUNTERS)
    size_t iCodeSize;
    iCodeSize = GetCodeHeapSize() + GetEHGCHeapSize();
#endif // ENABLE_PERF_COUNTERS

    m_PitchOccurred = true;
    HRESULT ret = TRUE;
    if (minSpaceRequired  > m_CodeHeapReservedSize)
    {
        if (minSpaceRequired > g_pConfig->GetMaxCodeCacheSize())
            ret = FALSE;
        else
        {
            ReplaceCodeHeap(minSpaceRequired,minCommittedSpaceRequired);
            ret = m_CodeHeap ? TRUE : FALSE;
        }
    }
    else if (minCommittedSpaceRequired > m_CodeHeapCommittedSize)
    {
        if (!SetCodeHeapCommittedSize(minCommittedSpaceRequired))          
                ret = FALSE;
    }


#if defined(ENABLE_PERF_COUNTERS)
    JIT_PERF_UPDATE_X86_CODE_SIZE(GetCodeHeapSize() + GetEHGCHeapSize() - iCodeSize); // Would be a -ive number for successful pitch

    // Perf counters temporarily don't support pitched bytes since Ejit is not in the product.
//    COUNTER_ONLY(GetPrivatePerfCounters().m_Jit.cbPitched+= -(GetCodeHeapSize() + GetEHGCHeapSize() - iCodeSize));
//    COUNTER_ONLY(GetGlobalPerfCounters().m_Jit.cbPitched+= -(GetCodeHeapSize() + GetEHGCHeapSize() - iCodeSize));
#endif

    GarbageCollectUnusedThunks();
    
    if (!PitchEHInfo || !PitchGCInfo) 
    {
        _ASSERTE(!"NYI");
        /*
        for (unsigned i = 0; i < m_JMIT_len; i++) {
            if (!PitchEHInfo) PreserveEHInfo(i);
            if (!PitchGCInfo) PreserveGCInfo(i);
        }
        */
    }

#ifdef _DEBUG
    SetBreakpointsInUnusedHeap(); // this enables us to immediately catch an attempt to execute code that has been pitched
#endif 
    TICKS endPitchTime = GET_TIMESTAMP();
    AddPitchOverhead(endPitchTime-startPitchTime);    // no need to protect this for multi-threads since
                                                        // we still haven't restarted EE.
    GCHeap::RestartEE(FALSE, TRUE);
    return ret;
}

//*********************************************************************
//                          Private functions                         *
//*********************************************************************

inline EconoJitManager::JittedMethodInfo* EconoJitManager::Token2JittedMethodInfo(METHODTOKEN token)
{
#ifdef _DEBUG
    // check that this is a valid token
    JittedMethodInfo* pJMITstart = (JittedMethodInfo*) (m_JittedMethodInfoHdr+1);
    if (pJMITstart <= (JittedMethodInfo*) token && (JittedMethodInfo*)token < m_JMIT_free)
    {
        _ASSERTE( ((((size_t)token - (size_t)pJMITstart)/sizeof(JittedMethodInfo)) * sizeof(JittedMethodInfo))
                  + (size_t) pJMITstart == (size_t) token );
    }
    else
    {
        JittedMethodInfoHdr* pJMIT = (m_JittedMethodInfoHdr->next);
        while (pJMIT)
        {
            pJMITstart = (JittedMethodInfo*) (pJMIT+1);
            if ((size_t)pJMITstart  <=  (size_t)token  && 
                (size_t)token       <   ((size_t) pJMIT) + JMIT_BLOCK_SIZE)
            {
                _ASSERTE( ((((size_t) token - (size_t)pJMITstart)/sizeof(JittedMethodInfo)) * sizeof(JittedMethodInfo))
                  + (size_t) pJMITstart == (size_t) token );
                break;
            }
            pJMIT= pJMIT->next;
        }
    }
#endif
    return (JittedMethodInfo*) token;
    
}


inline MethodDesc* EconoJitManager::JitMethodInfo2MethodDesc(JittedMethodInfo* jmi)
{
    BYTE* u1 = (BYTE*) (jmi->u1.pMethodDescriptor);

    if ((size_t)u1 & 1) // method has been pitched
        return (MethodDesc*) ((size_t)u1 & ~1); // it is possible that someone by now has rejitted the method,
                                        // but we've already obtained the methoddesc!
    else // method has not been pitched
    {
        //_ASSERTE(jmi->flags.JittedMethodPitched == 0);  // unfortunately can't have this because there is a small
                                                          // window between the time jmi->u1 gets written and jit->flags gets
                                                          // updated in which this assert is not true.
        return ((EJCodeHeader*) u1)->pMethodDescriptor;
    }
}

inline BYTE* EconoJitManager::JitMethodInfo2EhGcInfo(JittedMethodInfo* jmi)
{
    BYTE* u2 = (BYTE*) (jmi->u2.pEhGcInfo);
    if ((size_t)u2 & 1) // method has been pitched
        return (BYTE*) ((size_t)u2 & ~1); // it is possible that someone by now has rejitted the method,
                                        // but we've already obtained the EhGc info!
    else // method has not been pitched
    {
        _ASSERTE(jmi->flags.JittedMethodPitched == 0);
        return *(BYTE**) (jmi->u1.pCodeHeader - 1);
    }
}


// This method is called while holding the alloc lock to determine if there already exists
// entry for a method. Is such an entry exists, then it must contain the methoddesc because
// the code has been pitched away. Also, we are guaranteed that no one is updating the JMI table.
EconoJitManager::JittedMethodInfo*   EconoJitManager::MethodDesc2MethodInfo(MethodDesc* pMethodDesc)
{
    pMethodDesc = (MethodDesc*) ((size_t)pMethodDesc | 1);   // if the entry exists it will have to be pitched, so the last bit will be set
    JittedMethodInfoHdr* pJMIT = m_JittedMethodInfoHdr;
    size_t limit = (size_t)m_JMIT_free;
    while (pJMIT) {
        JittedMethodInfo* pJMI = (JittedMethodInfo*) (pJMIT+1);
        while ((size_t)(pJMI+1) <= limit)
        {
            if ( (pJMI->u1.pMethodDescriptor) == pMethodDesc)
            {
                _ASSERTE(pJMI->flags.JittedMethodPitched);
                return pJMI;
            }
            pJMI++;
        }
        pJMIT = pJMIT->next;
        limit = ((size_t)pJMIT) + JMIT_BLOCK_SIZE;        // all other JMIT blocks are completely full
    }
    return NULL;
}

BOOL EconoJitManager::growJittedMethodInfoTable()
{
    //DEBUG_LOG("ALLOC(growJMIT)",PAGE_SIZE);
    JittedMethodInfoHdr* newJMITHdr = (JittedMethodInfoHdr*) VirtualAlloc(NULL,PAGE_SIZE,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE);
    if (!newJMITHdr)
        return FALSE;
    WS_PERF_SET_HEAP(ECONO_JIT_HEAP);
    WS_PERF_UPDATE("growJittedMethodInfoTable", PAGE_SIZE, newJMITHdr);

    // register this address range since the jump stubs are here
    if (!ExecutionManager::AddRange((LPVOID)newJMITHdr, (LPVOID)((size_t)newJMITHdr + PAGE_SIZE),this, NULL))
    {
        VirtualFree(newJMITHdr,PAGE_SIZE,MEM_DECOMMIT);
        VirtualFree(newJMITHdr,0,MEM_RELEASE);
        return FALSE;
    }

    newJMITHdr->next = m_JittedMethodInfoHdr;
    m_JittedMethodInfoHdr = (JittedMethodInfoHdr*) newJMITHdr;
    m_JMIT_free = (JittedMethodInfo*) (newJMITHdr+1);
    return true;
}

BOOL EconoJitManager::growPC2MDMap()
{
    unsigned newSize = m_PcToMdMap_size + INITIAL_PC2MD_MAP_SIZE;
    PCToMDMap* temp = new PCToMDMap[newSize];
    if (!temp)
    {
      return false;
    }
    _ASSERTE(m_PcToMdMap_len);
    memcpy((BYTE*)temp,(BYTE*)m_PcToMdMap,m_PcToMdMap_size);
    
    // cannot delete m_PcToMdMap since a thread might be using it, so just
    // collect it in a recycle list which will be cleared during the next pitch
    ((PC2MDBlock*)m_PcToMdMap)->next = m_RecycledPC2MDMaps;
    m_RecycledPC2MDMaps = (PC2MDBlock*)m_PcToMdMap;

    m_PcToMdMap = temp;
    m_PcToMdMap_size = newSize;
    return true;
}

BOOL  EconoJitManager::AddPC2MDMap(MethodDesc* pMD, BYTE* pCodeEnd)
{
    if ((m_PcToMdMap_len+sizeof(PCToMDMap) > m_PcToMdMap_size) && !growPC2MDMap())
        return false;
    _ASSERTE(m_PcToMdMap_len+sizeof(PCToMDMap) <= m_PcToMdMap_size);
    PCToMDMap* newMap = (PCToMDMap*) ((size_t)m_PcToMdMap+m_PcToMdMap_len);
    newMap->pMD = pMD;
    newMap->pCodeEnd = pCodeEnd;
    m_PcToMdMap_len += sizeof(PCToMDMap);
    return true;
}

#ifdef _DEBUG
void EconoJitManager::LogAction(MethodDesc* pMD, LPCUTF8 action, void* codeStart, void* codeEnd)
{
    LPCUTF8 cls  = pMD->GetClass() ? pMD->GetClass()->m_szDebugClassName
                                   : "GlobalFunction";
    LPCUTF8 name = pMD->GetName();

    LOG((LF_JIT,LL_INFO10,"%s", action));
    LOG((LF_JIT, LL_INFO10," method %s.%s [%x,%x]\n", cls, name,codeStart,codeEnd));
}
#endif

BYTE* __cdecl RejitCalleeMethod(struct MachState *  ms, void * args, BYTE * thunkReturnAddress)
{
    BYTE* retAddress;
    EconoJitManager::JittedMethodInfo* jmi =NULL;
    jmi = (EconoJitManager::JittedMethodInfo*)GetTransferStubStart(thunkReturnAddress, CallThunkType ); 
    MethodDesc* pMD = EconoJitManager::JitMethodInfo2MethodDesc(jmi);
#if defined(_PPC_)
    // We need to call a helper to push the enregistered arguments into their home locations
    MetaSig msig(pMD->GetSig(), pMD->GetModule());
    BOOL isStatic = !msig.HasThis();
    BYTE * frameBase = (BYTE *)((int)args - FramedMethodFrame::GetOffsetOfArgumentRegisters());
    UINT argSize = msig.SizeOfActualFixedArgStack(isStatic);
    FramedMethodFrame::RegisterWorker(frameBase, &msig, isStatic, argSize, false);
#endif
    // This creates a transition frame so stackwalk happens properly
    _ASSERTE(ms->isValid());       /* This is not a lazy machine state (_pRetAddr != 0) */
    HelperMethodFrame HelperFrame( ms, pMD, (ArgumentRegisters*)args);
    retAddress =  EconoJitManager::RejitMethod(jmi,0);
    HelperFrame.Pop();
#if defined(_PPC_)
    // We need to copy the enregistered arguments from the home location back into the array from which
    // the argument registers will be restored 
    FramedMethodFrame::RegisterWorker(frameBase, &msig, isStatic, argSize, true);
#endif
    return retAddress;
}



BYTE* __cdecl RejitCallerMethod(struct MachState * ms, void* retval, BYTE * thunkReturnAddress)
{
    BYTE* retAddress;
    _ASSERTE(ms->isValid());		/* This is not a lazy machine state (_pRetAddr != 0) */
    HelperMethodFrame HelperFrame(ms, 0);    // This creates a transition frame so stackwalk happens properly
    EconoJitManager::PitchedCodeThunk* pThunk = NULL;
    pThunk = (EconoJitManager::PitchedCodeThunk*)GetTransferStubStart(thunkReturnAddress, RejitThunkType);
    EconoJitManager::JittedMethodInfo* jmi = pThunk->u.pMethodInfo;
    unsigned returnOffset = pThunk->relReturnAddress;
    // Make sure that the thunk hasn't been recycled
    _ASSERTE( returnOffset != 0xffffffff );

    // cant protect retLow directly because GCPROTECT_END wacks it
    if (pThunk->retTypeProtect == MethodDesc::RETNONOBJ) 
    {
        retAddress = EconoJitManager::RejitMethod(jmi,returnOffset);
    }
    else if (pThunk->retTypeProtect == MethodDesc::RETOBJ)
    { 
        OBJECTREF objToProtect = ObjectToOBJECTREF((Object *)*(void **)retval);
        GCPROTECT_BEGIN(objToProtect)
        retAddress = EconoJitManager::RejitMethod(jmi,returnOffset);
        GCPROTECT_END();
        (*(void **)retval) = OBJECTREFToObject(objToProtect);
    }
    else
    {
        _ASSERTE(pThunk->retTypeProtect == MethodDesc::RETBYREF);
        LPVOID byrefToProtect = *(void **)retval;
        GCPROTECT_BEGININTERIOR(byrefToProtect)
        retAddress = EconoJitManager::RejitMethod(jmi,returnOffset);
        GCPROTECT_END();
        (*(void**)retval) = byrefToProtect;
    }
    HelperFrame.Pop();
    return retAddress;
}


BYTE* EconoJitManager::RejitMethod(JittedMethodInfo* pJMI, unsigned returnOffset)
{
    TICKS startRejitTime = GET_TIMESTAMP();
    EconoJitManager* jitMgr = (EconoJitManager*) ExecutionManager::GetJitForType(miManaged_IL_EJIT);
    Thread* thread = GetThread();

    BYTE* startAddress;

#ifdef STRESS_HEAP
        // for GCStress > 2, the EnablePremtiveGC does it for us
    if (g_pConfig->GetGCStressLevel() & EEConfig::GCSTRESS_TRANSITION)  
        g_pGCHeap->StressHeap();
#endif

        thread->EnablePreemptiveGC();
        m_pRejitCritSec->Enter();
#ifdef _DEBUG
        m_RejitLock_Holder = GetCurrentThreadId();  
#endif
        thread->DisablePreemptiveGC();

    if (pJMI->flags.JittedMethodPitched)
    {
        // we found the flag to be true, and we are holding the rejit lock, so it will remain true until
        // we rejit
        MethodDesc* ftn =  jitMgr->JitMethodInfo2MethodDesc(pJMI);
        COR_ILMETHOD_DECODER ILHeader(ftn->GetILHeader(), ftn->GetMDImport());
        BOOL ignored;
#ifdef _DEBUG

        LOG((LF_JIT, LL_INFO1000,returnOffset ? "REJIT caller: " : "REJIT callee" ));
#endif
        // This is commented out right now because I have to give a private
        // drop to test, since this breaks their tests.  I will uncomment it
        // when they can handle re-jit events.
#ifdef PROFILING_SUPPORTED
        if (CORProfilerTrackJITInfo())
            g_profControlBlock.pProfInterface->JITCompilationStarted(
                reinterpret_cast<ThreadID>(thread),
                reinterpret_cast<FunctionID>(ftn), TRUE);
#endif // PROFILING_SUPPORTED

#ifdef PROFILING_SUPPORTED
		Stub *pStub =
#endif
		::JITFunction(ftn, &ILHeader, &ignored);

#ifdef PROFILING_SUPPORTED
        if (CORProfilerTrackJITInfo())
            g_profControlBlock.pProfInterface->JITCompilationFinished(
                reinterpret_cast<ThreadID>(thread),
                reinterpret_cast<FunctionID>(ftn),
                (pStub != NULL ? S_OK : E_FAIL),
                FALSE);
#endif // PROFILING_SUPPORTED
    }
    // else,  someone beat us, the method has already been rejitted!
    // we are guaranteed that the method will not be pitched until we get out of here
    _ASSERTE( (((size_t)(pJMI->u1.pCodeHeader)) & 1) == 0);
    startAddress = (BYTE*) (pJMI->u1.pCodeHeader + 1);
#ifdef _DEBUG
    m_RejitLock_Holder = 0;  
#endif
    
    if (returnOffset)
        m_cCallerRejits++;      // this operation is protected by the Rejit crst
    else
        m_cCalleeRejits++;

    TICKS endRejitTime = GET_TIMESTAMP();
    AddRejitOverhead(endRejitTime-startRejitTime);    // this is protected by the RejitCritSec
                                                        
    m_pRejitCritSec->Leave();
   
    return (startAddress + returnOffset);
}

const BYTE *GetCallThunkAddress()
{
    return (const BYTE *)CallThunk;
}

const BYTE *GetRejitThunkAddress()
{
    return (const BYTE *)RejitThunk;
}

typedef struct 
{
    EconoJitManager*    jitMgr;
    BYTE                retTypeProtect;
    unsigned            threadIndex;
    unsigned            preserveCandidateIndex;
    unsigned            cThreads;
} StackWalkData;

inline CORINFO_MODULE_HANDLE GetScopeHandle(MethodDesc* method) {
    return(CORINFO_MODULE_HANDLE(method));
}


void CreateThunk_callback(IJitManager* ejitMgr,LPVOID* pHijackLocation, ICodeInfo *pCodeInfo )
{
    if (ExecutionManager::FindJitMan((SLOT)*pHijackLocation) == ejitMgr) 
    {
        METHODTOKEN methodToken = ((EECodeInfo*)pCodeInfo)->m_methodToken;
        if (((EconoJitManager*)ejitMgr)->IsThunk((BYTE*) *pHijackLocation))
            ((EconoJitManager*)ejitMgr)->SetThunkBusy((BYTE*)*pHijackLocation, methodToken);
        else
            ((EconoJitManager*)ejitMgr)->CreateThunk(pHijackLocation, MethodDesc::RETNONOBJ, methodToken);
    }
    else
        _ASSERTE("Internal error while code pitching");
}


StackWalkAction StackWalkCallback_CodePitch(CrawlFrame* pCF, void* data)
{
    StackWalkData* swd = (StackWalkData*)data;
    EconoJitManager* ejitMgr = swd->jitMgr;
    BYTE prevRetType = swd->retTypeProtect;
    // update return type
    _ASSERTE(pCF->ReturnsObject() < 256);
    swd->retTypeProtect = (BYTE) pCF->ReturnsObject();
  
    PREGDISPLAY pRD = pCF->GetRegisterSet();
    SLOT* pPC = pRD->pPC;
   
    
    if (ExecutionManager::FindJitMan(*pPC) != ejitMgr ||
        !pCF->IsFrameless() ) // the EE inserts frames for context transition between managed calls; these should be ignored.
    {
        return SWA_CONTINUE; 
    }

#ifdef _DEBUG
    ICodeManager* codeman = ejitMgr->GetCodeManager();
    _ASSERTE(codeman);
#endif

    METHODTOKEN methodToken = pCF->GetMethodToken();

#ifdef _DEBUG
    MethodDesc* pMD = ejitMgr->JitTokenToMethodDesc(methodToken);
    LPCUTF8 cls  = pMD->GetClass() ? pMD->GetClass()->m_szDebugClassName : "GlobalFunction";
    LPCUTF8 name = pMD->GetName();

    LOG((LF_JIT,LL_INFO10000,"SW callback for %s:%s\n", cls,name));
#endif

    // if this is already a thunk, simply mark thunk as busy
    if (ejitMgr->IsThunk((BYTE*) *pPC))
    {
        ejitMgr->SetThunkBusy((BYTE*) *pPC,methodToken);
    }
    else
    {
        ejitMgr->AddPreserveCandidate(swd->threadIndex,
                                      swd->cThreads,
                                      swd->preserveCandidateIndex,
                                      methodToken);
        swd->preserveCandidateIndex++;
        ejitMgr->CreateThunk((LPVOID*)pPC, prevRetType,methodToken);
    }
    // also create thunks for each return from finally's and filters
    // NOTE: if we ever make the regular jit pitchable, make the following a virtual method on ICodeManager
    EECodeInfo codeInfo(methodToken, ejitMgr);
    Fjit_EETwain::HijackHandlerReturns(pRD,ejitMgr->GetGCInfo(methodToken),&codeInfo, ejitMgr, CreateThunk_callback );
    return SWA_CONTINUE;
}


void EconoJitManager::StackWalkForCodePitching() 
{
    // for each thread call StackWalkThreadForCodePitching();
    Thread  *thread = NULL;
    StackWalkData stackWalkData;
    stackWalkData.jitMgr = this;
    stackWalkData.retTypeProtect = MethodDesc::RETNONOBJ;
    stackWalkData.threadIndex = 0;
    stackWalkData.preserveCandidateIndex = 0;
    unsigned cThreads = GetThreadCount();
    stackWalkData.cThreads = cThreads;

    if (cThreads*m_MaxUnpitchedPerThread > m_PreserveCandidates_size)
        growPreserveCandidateArray(cThreads*m_MaxUnpitchedPerThread);

    while ((thread = ThreadStore::GetThreadList(thread)) != NULL)
    {
        stackWalkData.retTypeProtect = MethodDesc::RETNONOBJ;
        thread->StackWalkFrames(StackWalkCallback_CodePitch, (void*) &stackWalkData);
        stackWalkData.threadIndex++;
    }
}


unsigned EconoJitManager::GetThreadCount() 
{
    unsigned n=0;
    Thread  *thread = NULL;
    while ((thread = ThreadStore::GetThreadList(thread)) != NULL)
        n++;
    return n;
}

void EconoJitManager::AddPreserveCandidate(unsigned threadIndex,
                                           unsigned cThreads,
                                           unsigned candidateIndex,
                                           METHODTOKEN methodToken)
{
    _ASSERTE(!((JittedMethodInfo*) methodToken)->flags.JittedMethodPitched);
    if (candidateIndex < m_MaxUnpitchedPerThread)
        m_PreserveCandidates[threadIndex +cThreads*candidateIndex] = (JittedMethodInfo*) methodToken;

}

void EconoJitManager::MarkHeapsForPitching()
{
    _ASSERTE(m_JittedMethodInfoHdr);
    JittedMethodInfoHdr* pJMIT_Hdr = m_JittedMethodInfoHdr;
    size_t limit = (size_t)m_JMIT_free;

    do 
    {
        JittedMethodInfo* jmit = (JittedMethodInfo*) (pJMIT_Hdr+1);
        while ((size_t)(jmit+1) <= limit)
        {
            if (!(jmit->flags.JittedMethodPitched) && !(jmit->flags.MarkedForPitching))
            {
                jmit->flags.MarkedForPitching = true;
                jmit->flags.EHandGCInfoPitched = true;
                CreateTransferStub( (BYTE *)&(jmit->JmpInstruction[0]), true, (DWORD *)CallThunk, DefaultStubType );
            }
            jmit++;

        }
        pJMIT_Hdr = pJMIT_Hdr->next;
        limit = ((size_t)pJMIT_Hdr) + JMIT_BLOCK_SIZE;        // all other JMIT blocks are completely full
    } while (pJMIT_Hdr);
}

void EconoJitManager::UnmarkPreservedCandidates(size_t minSpaceRequired)
{
    JittedMethodInfo** candidate = m_PreserveCandidates;
    unsigned cThreads = GetThreadCount();
    if (minSpaceRequired >= m_CodeHeapCommittedSize)
    {
        while (candidate < &(m_PreserveCandidates[cThreads*m_MaxUnpitchedPerThread]))
            *candidate++ = NULL;
        return;
    }
    unsigned totalSpacePreserved = 0;
    while (candidate < &(m_PreserveCandidates[cThreads*m_MaxUnpitchedPerThread]))
    {
        if (*candidate) 
            
        {
            JittedMethodInfo* jmi = *candidate;
            unsigned spaceRequiredByMethod = (unsigned)((size_t)jmi->u2.pCodeEnd - (size_t)jmi->u1.pCodeHeader);
            if (availableMemoryInCodeHeap() + (usedMemoryInCodeHeap() - totalSpacePreserved - spaceRequiredByMethod) 
                >= minSpaceRequired)
            {
                _ASSERTE(!(jmi->flags.JittedMethodPitched));
                jmi->flags.MarkedForPitching = false;
                // We'll fix the {CALL instruction to the thunk}, by changing it to
                // a {JMP instruction to the actual code}, in MovePreservedMethod
                totalSpacePreserved += spaceRequiredByMethod;
            }
            else // delete it from the list of preserved candidates
            {
                *candidate = NULL;
            }
        }
        candidate++;
    } // while
    _ASSERTE(availableMemoryInCodeHeap() + (usedMemoryInCodeHeap() - totalSpacePreserved) >= minSpaceRequired);
}


unsigned EconoJitManager::PitchMarkedCode()
{
    _ASSERTE(m_JittedMethodInfoHdr);
    JittedMethodInfoHdr* pJMIT_Hdr = m_JittedMethodInfoHdr;
    size_t limit = (size_t)m_JMIT_free;    // the first JMIT block may be partially full
    unsigned cMethodsPitched = 0;

    
    do 
    {
        JittedMethodInfo* jmit = (JittedMethodInfo*) (pJMIT_Hdr+1);
        while ((size_t)(jmit+1) <= limit)
        {
            if (jmit->flags.MarkedForPitching)
            {
#ifdef DEBUGGING_SUPPORTED
                MethodDesc *pMD = *(MethodDesc**)(jmit->u1.pCodeHeader);
                DWORD dwDebugBits = pMD->GetModule()->GetDebuggerInfoBits();

                //quick, tell the debugger before we pitch it!
                if (CORDebuggerTrackJITInfo(dwDebugBits))
                {
                    g_pDebugInterface->PitchCode(pMD, 
                            (const BYTE*)jmit->u1.pCodeHeader+sizeof(void*));
                }
#endif // DEBUGGING_SUPPORTED

#ifdef PROFILING_SUPPORTED
                // Notify the profiler that this function is being pitched
                if (CORProfilerTrackJITInfo())
                {
                    MethodDesc *pMD = *(MethodDesc**)(jmit->u1.pCodeHeader);

                    g_profControlBlock.pProfInterface->
                        JITFunctionPitched(reinterpret_cast<ThreadID>(GetThread()),
                                           reinterpret_cast<FunctionID>(pMD));
                }
#endif // PROFILING_SUPPORTED

                jmit->flags.JittedMethodPitched = true;
                jmit->flags.MarkedForPitching = false;
#ifdef _DEBUG
                LogAction((MethodDesc*)((size_t) (* (MethodDesc**) (jmit->u1.pCodeHeader))), 
                    "Pitched",
                    jmit->u1.pCodeHeader,
                    jmit->u2.pCodeEnd);
#endif
                // set last bit to indicate that code has been pitched
                jmit->u2.pEhGcInfo =  (BYTE*) ((size_t)(*(BYTE **) (jmit->u1.pCodeHeader - 1)) | 1);
                jmit->u1.pMethodDescriptor = (MethodDesc*) ((size_t)(* (MethodDesc**) (jmit->u1.pCodeHeader)) | 1);
                cMethodsPitched++;
            }

            jmit++;
        }
        pJMIT_Hdr = pJMIT_Hdr->next;
        limit = ((size_t)pJMIT_Hdr) + JMIT_BLOCK_SIZE;        // all other JMIT blocks are completely full
    } while (pJMIT_Hdr);
    ResetPc2MdMap();
    return cMethodsPitched;
}

int __cdecl EconoJitManager::compareJMIstart( const void *arg1, const void *arg2 )
{
    JittedMethodInfo* jmi1 = *(JittedMethodInfo**) arg1;
    JittedMethodInfo* jmi2 = *(JittedMethodInfo**) arg2;
    _ASSERTE(!(jmi1->flags.JittedMethodPitched) && !(jmi2->flags.JittedMethodPitched));
    return (jmi1->u1.pCodeHeader < jmi2->u1.pCodeHeader ? -1 : 
             (jmi1->u1.pCodeHeader == jmi2->u1.pCodeHeader ? 0 : 1));
}

void EconoJitManager::MovePreservedMethods()
{
    unsigned cThreads = GetThreadCount();
    unsigned maxCandidates = cThreads * m_MaxUnpitchedPerThread;
    unsigned numCandidates = CompressPreserveCandidateArray(maxCandidates);
    if (numCandidates > 1) 
    {
        qsort(m_PreserveCandidates,numCandidates,sizeof(void*),EconoJitManager::compareJMIstart);
#ifdef _DEBUG
        // verify that it is sorted correctly
        for (unsigned j=0;j<numCandidates-1;j++)
        {
            _ASSERTE(m_PreserveCandidates[j]->u1.pCodeHeader <= m_PreserveCandidates[j+1]->u1.pCodeHeader);
        }
#endif

    }

#ifdef _DEBUG
    if (m_MaxUnpitchedPerThread == 0)
        memset(m_CodeHeap,0xcc,m_CodeHeapFree-m_CodeHeap);
#endif

    _ASSERTE(m_CodeHeap);
    m_CodeHeapFree = m_CodeHeap;
    for (unsigned i=0;i<numCandidates; i++)
    {
        if (i == 0 || m_PreserveCandidates[i] != m_PreserveCandidates[i-1])
        {
            JittedMethodInfo* jmi = m_PreserveCandidates[i];
            if (!(jmi->flags.JittedMethodPitched))
            {
                MovePreservedMethod(jmi);
#ifdef _DEBUG
                LogAction((* (MethodDesc**) (jmi->u1.pCodeHeader)), 
                    "Preserved",
                    jmi->u1.pCodeHeader,
                    jmi->u2.pCodeEnd);
#endif
            }
        }
    }
    // Flush the range spanning all preserved methods.
    FlushInstructionCache(GetCurrentProcess(), m_CodeHeap, m_CodeHeapFree-m_CodeHeap);
    memset(m_PreserveCandidates,0,cThreads*m_MaxUnpitchedPerThread*sizeof(void*));

}

unsigned EconoJitManager::CompressPreserveCandidateArray(unsigned size)
{
    JittedMethodInfo** pFront, **pRear = m_PreserveCandidates;
    //_ASSERTE(size);
    unsigned cCompressedSize=0;

    while (cCompressedSize < size && (*pRear))
    {
        cCompressedSize++;
        pRear++;
    }
    if (cCompressedSize == size)
        return cCompressedSize;
    pFront=pRear;
    while (pFront < &(m_PreserveCandidates[size]))
    {
        if (*pFront)
        {
            *pRear++ = *pFront;
            cCompressedSize++;
        }
        pFront++;
    }
    return cCompressedSize;
}


void EconoJitManager::MovePreservedMethod(JittedMethodInfo* jmi)
{
    _ASSERTE(m_CodeHeapFree);
    BYTE* startPreserved = ((BYTE*)jmi->u1.pCodeHeader) - sizeof (void*); // to account for EhGc info
    size_t size =  (size_t)jmi->u2.pCodeEnd - (size_t)startPreserved;

    _ASSERTE(m_CodeHeapFree <= startPreserved);
    memcpy(m_CodeHeapFree,startPreserved, size);
    jmi->u1.pCodeHeader = (EJCodeHeader*) ((size_t) m_CodeHeapFree + sizeof(void*));
    jmi->u2.pCodeEnd = m_CodeHeapFree + size;

    // Compute the start of the method
    size_t pCodeBlock = (size_t)(jmi->u1.pCodeHeader) + sizeof(void*);
    // Change the call back to a jmp
    CreateTransferStub( (BYTE *)&(jmi->JmpInstruction[0]), false, (DWORD *)pCodeBlock, DefaultStubType );

    MethodDesc *pMD = JitMethodInfo2MethodDesc(jmi);
    AddPC2MDMap(pMD,jmi->u2.pCodeEnd);
    m_CodeHeapFree += size;
    _ASSERTE((size_t)m_CodeHeapFree - (size_t)m_CodeHeap <= m_CodeHeapCommittedSize);

#ifdef DEBUGGING_SUPPORTED
    // Tell the debugger that something's moved.
    DWORD dwDebugBits = pMD->GetModule()->GetDebuggerInfoBits();
    if (CORDebuggerTrackJITInfo(dwDebugBits))
        g_pDebugInterface->MovedCode(pMD, 
                                     (const BYTE*)(startPreserved +2*sizeof(void*)),
                                     (const BYTE*)pCodeBlock);
#endif // DEBUGGING_SUPPORTED
}


void EconoJitManager::MoveAllPreservedEhGcInfo()
{
    _ASSERTE(m_JittedMethodInfoHdr);
    JittedMethodInfoHdr* pJMIT_Hdr = m_JittedMethodInfoHdr;
    size_t limit = (size_t)m_JMIT_free;    // the first JMIT block may be partially full
    m_cPreserveEhGcInfoList = 0;
    do 
    {
        JittedMethodInfo* jmit = (JittedMethodInfo*) (pJMIT_Hdr+1);
        while ((size_t)(jmit+1) <= limit)
        {
            if (!jmit->flags.EHandGCInfoPitched)
            {
                AddToPreservedEhGcInfoList(jmit);
            }
            jmit++;
        }
        pJMIT_Hdr = pJMIT_Hdr->next;
        limit = ((size_t)pJMIT_Hdr) + JMIT_BLOCK_SIZE;        // all other JMIT blocks are completely full
    } while (pJMIT_Hdr);
    EHGCBlockHdr* oldHeap = m_EHGCHeap;
    ResetEHGCHeap();
    for (unsigned i=0; i<m_cPreserveEhGcInfoList; i++)
        MoveSinglePreservedEhGcInfo(m_PreserveEhGcInfoList[i]);
    CleanupLargeEhGcInfoList();
    // free all the unused heap space
    while (oldHeap)
    {
        EHGCBlockHdr* pHeap = oldHeap->next;
        size_t size = oldHeap->blockSize;
        VirtualFree(oldHeap,size,MEM_DECOMMIT);      
        VirtualFree(oldHeap,0,MEM_RELEASE);
        //DEBUG_LOG("FREE(EhGcHeaps)",size);
        oldHeap = pHeap;
    }
}


void EconoJitManager::MoveSinglePreservedEhGcInfo(JittedMethodInfo* jmi)
{
    unsigned char* pDestEhGcBlock = (unsigned char*) allocEHGCBlock(jmi->GetEhGcInfo_len(m_LargeEhGcInfo));
    BYTE* u2 = (BYTE*) (jmi->u2.pEhGcInfo);
    if ((size_t)u2 & 1) // method has been pitched
    {
        _ASSERTE(jmi->flags.JittedMethodPitched);
        memcpy(pDestEhGcBlock,
               (BYTE*) ((size_t)u2 & ~1),
               jmi->GetEhGcInfo_len(m_LargeEhGcInfo));
        jmi->u2.pEhGcInfo = (BYTE*) ((size_t) pDestEhGcBlock | 1);
    }

    else // method has not been pitched
    {
        _ASSERTE(jmi->flags.JittedMethodPitched == 0);
        memcpy(pDestEhGcBlock,
               *(BYTE**)(jmi->u1.pCodeHeader - 1),
               jmi->GetEhGcInfo_len(m_LargeEhGcInfo));
        *(BYTE**) (jmi->u1.pCodeHeader - 1) = pDestEhGcBlock;
    }

}

void EconoJitManager::CleanupLargeEhGcInfoList()
{
    LargeEhGcInfoList* p = (LargeEhGcInfoList*) &m_LargeEhGcInfo, *q = m_LargeEhGcInfo;
    _ASSERTE(p->next == q); 
    while (q)
    {
        if (q->jmi->flags.EHandGCInfoPitched)
        {
            LargeEhGcInfoList* temp = q;
            q = q->next; 
            delete temp;
        }
        else
        {
            p = q;
            q = q->next;
        }
    }
}

int __cdecl EconoJitManager::compareEhGcPtr( const void *arg1, const void *arg2 )
{
    JittedMethodInfo* jmi1 = *(JittedMethodInfo**) arg1;
    JittedMethodInfo* jmi2 = *(JittedMethodInfo**) arg2;
    size_t EhGc1 = (size_t)JitMethodInfo2EhGcInfo(jmi1);
    size_t EhGc2 = (size_t)JitMethodInfo2EhGcInfo(jmi2);
    return (EhGc1 < EhGc2 ? -1 :
                EhGc1 == EhGc2 ? 0: 1);
}
void EconoJitManager::AddToPreservedEhGcInfoList(JittedMethodInfo* jmi)
{
    _ASSERTE(m_cPreserveEhGcInfoList <= m_PreserveEhGcInfoList_size);
    if (m_cPreserveEhGcInfoList == m_PreserveEhGcInfoList_size)
    {
        growPreservedEhGcInfoList();
    }
    m_PreserveEhGcInfoList[m_cPreserveEhGcInfoList++] = jmi;
}

void EconoJitManager::growPreservedEhGcInfoList()
{
    _ASSERTE(m_cPreserveEhGcInfoList == m_PreserveEhGcInfoList_size);
    JittedMethodInfo** temp = m_PreserveEhGcInfoList;
    m_PreserveEhGcInfoList_size *= 2;
    m_PreserveEhGcInfoList = new (throws) pJittedMethodInfo[m_PreserveEhGcInfoList_size];
    _ASSERTE(m_PreserveEhGcInfoList != NULL);
    memcpy(m_PreserveEhGcInfoList,temp,m_cPreserveEhGcInfoList*sizeof(void*));
    delete[] temp;
}

#if defined(ENABLE_PERF_COUNTERS) 
size_t EconoJitManager::GetCodeHeapSize()
{
    return 0;
}
size_t EconoJitManager::GetEHGCHeapSize()
{
    size_t icurSum = 0;
    return icurSum;
}
#endif // ENABLE_PERF_COUNTERS

#ifdef _DEBUG
// Puts a breakpoint at every byte of unused heap.
// this enables us to immediately catch an attempt to execute code that has been pitched
void  EconoJitManager::SetBreakpointsInUnusedHeap()
{ 
    _ASSERTE(m_CodeHeap && m_CodeHeapFree);
    
    BYTE *start = (BYTE*)m_CodeHeapFree;
    BYTE *end = start + availableMemoryInCodeHeap();

    for (BYTE* p = start; p < end; p += sizeof(BREAK_OPCODE_TYPE)) {
        *(BREAK_OPCODE_TYPE*)p = BREAK_OPCODE;
    }
}

void  EconoJitManager::VerifyAllCodePitched()
{
    // Walk the jittedmethodinfo table and verify that all code is pitched
    JittedMethodInfoHdr* pJMIT_Hdr = m_JittedMethodInfoHdr;
    size_t limit = (size_t)m_JMIT_free;
    do 
    {
        JittedMethodInfo* jmit = (JittedMethodInfo*) (pJMIT_Hdr+1);
        while ((size_t)(jmit+1) <= limit)
        {
            _ASSERTE(jmit->flags.JittedMethodPitched);
            jmit++;

        }
        pJMIT_Hdr = pJMIT_Hdr->next;
        limit = ((size_t)pJMIT_Hdr) + JMIT_BLOCK_SIZE;        // all other JMIT blocks are completely full
    } while (pJMIT_Hdr);
}
#endif 

/***************************************************************************************************
                    Thunk Management
****************************************************************************************************/

void EconoJitManager::MarkThunksForRelease() 
{
    ThunkBlock* thunkBlock = m_ThunkBlocks;
    while (thunkBlock) 
    {
        PitchedCodeThunk* thunks = (PitchedCodeThunk*) ( thunkBlock+1 );
        for (unsigned i=0;i<THUNKS_PER_BLOCK;i++)
        {
            thunks[i].Busy = false;
        }
        thunkBlock = thunkBlock->next;          
    }
}


BOOL EconoJitManager::IsThunk(const BYTE* address)
{
    ThunkBlock*  pThunkBlock = m_ThunkBlocks;
    while (pThunkBlock) 
    {
        if ((BYTE*) pThunkBlock < address && address < ((BYTE*)pThunkBlock+THUNK_BLOCK_SIZE))
        {
            address = (BYTE*) ((size_t) address & THUNK_BEGIN_MASK);
            _ASSERTE(((PitchedCodeThunk*)address)->relReturnAddress != 0xffffffff);
            return true;
        }
        pThunkBlock=pThunkBlock->next;
    }  
    return false;
}

void  EconoJitManager::SetThunkBusy(BYTE* address, METHODTOKEN methodToken)
{
    PitchedCodeThunk* pThunk = (PitchedCodeThunk*)(((size_t) address) & THUNK_BEGIN_MASK);
    _ASSERTE(IsThunk((BYTE*) pThunk));
    pThunk->Busy = true;
    JittedMethodInfo* jmi = (JittedMethodInfo*) methodToken; 
    jmi->flags.EHandGCInfoPitched = false;     // we always preserve EhGcInfo for methods on the stack

}

EconoJitManager::PitchedCodeThunk* EconoJitManager::GetNewThunk()
{
    PitchedCodeThunk* newThunk;
    if (m_FreeThunkList) //  && 0 is TEMPORARY for debugging only! Remove this before checkin
    {
        newThunk = m_FreeThunkList;
        m_FreeThunkList = m_FreeThunkList->u.next;
    }
    else if (m_cThunksInCurrentBlock && (m_cThunksInCurrentBlock < THUNKS_PER_BLOCK))
    {
        newThunk = ((PitchedCodeThunk*)(m_ThunkBlocks+1)) + m_cThunksInCurrentBlock;
        m_cThunksInCurrentBlock++;
    }
    else { // this is the first thunk to be created 
        ThunkBlock* newThunkBlock = (ThunkBlock*) VirtualAlloc(NULL,
                                                 THUNK_BLOCK_SIZE,
                                                 MEM_RESERVE|MEM_COMMIT,
                                                 PAGE_EXECUTE_READWRITE);
        //DEBUG_LOG("ALLOC(GetNewThunk)",THUNK_BLOCK_SIZE);
        if (!newThunkBlock)
        {
            _ASSERTE(!"Out of memory!");
        }
        WS_PERF_SET_HEAP(ECONO_JIT_HEAP);
        WS_PERF_UPDATE("CreateThunk", THUNK_BLOCK_SIZE, newThunkBlock);
        
        if (!ExecutionManager::AddRange((LPVOID)newThunkBlock, 
                                        (LPVOID)((size_t)newThunkBlock + THUNK_BLOCK_SIZE),
                                        this, 
                                        NULL))
        {
            VirtualFree(newThunkBlock,THUNK_BLOCK_SIZE,MEM_DECOMMIT);
            VirtualFree(newThunkBlock,0,MEM_RELEASE);
            _ASSERTE(!"Error: AddRange");
        }

        newThunkBlock->next = m_ThunkBlocks;
        m_ThunkBlocks = newThunkBlock; 
        newThunk = (PitchedCodeThunk*)(newThunkBlock+1);
        m_cThunksInCurrentBlock = 1;
    }
    return newThunk;
}
    
// This is called while code pitching, so we are free to read and update jmi entries
void EconoJitManager::CreateThunk(LPVOID* pHijackLocation,BYTE retTypeProtect, METHODTOKEN methodToken)
{
    BYTE* returnAddress = (BYTE*) *pHijackLocation;
    JittedMethodInfo* jmi = (JittedMethodInfo*) methodToken; //JitCode2MethodInfo(returnAddress);
    _ASSERTE(jmi);


    PitchedCodeThunk* newThunk = GetNewThunk();

    newThunk->u.pMethodInfo = jmi;
    _ASSERTE(jmi->flags.JittedMethodPitched == 0);
    jmi->flags.EHandGCInfoPitched = false;     // we always preserve EhGcInfo for methods on the stack
    newThunk->relReturnAddress = (unsigned)((DWORD_PTR)returnAddress - ((DWORD_PTR)(jmi->u1.pCodeHeader + 1)));
    _ASSERTE(newThunk->relReturnAddress != 0xffffffff );
    newThunk->Busy = true;
    newThunk->LinkedInFreeList = false;
    newThunk->retTypeProtect = retTypeProtect;
    // Create a stub to the RejitThunk helper
    CreateTransferStub( (BYTE *)&(newThunk->CallInstruction[0]), true, (DWORD *)RejitThunk, RejitThunkType );
    // Wrote the address of this stub into the hijack location
    *pHijackLocation = (BYTE *) &(newThunk->CallInstruction[0]);
}



void EconoJitManager::GarbageCollectUnusedThunks()
{
    ThunkBlock* thunkBlock = m_ThunkBlocks;
    size_t limit = (size_t) (thunkBlock+1) + m_cThunksInCurrentBlock*sizeof(PitchedCodeThunk);
    while (thunkBlock)
    {
        PitchedCodeThunk* thunk = (PitchedCodeThunk*)(thunkBlock+1);
        while ((size_t) (thunk+1) <= limit)
        {
            if (!thunk->Busy && !thunk->LinkedInFreeList) 
            {
                thunk->u.next = m_FreeThunkList;
                thunk->LinkedInFreeList = true;
#ifdef _DEBUG
                thunk->relReturnAddress = 0xffffffff;
#endif
                m_FreeThunkList = thunk;
            }
            thunk++;
        }
        thunkBlock = thunkBlock->next;
        limit = (size_t)thunkBlock + THUNK_BLOCK_SIZE;
    }
}

void EconoJitManager::growPreserveCandidateArray(unsigned numberOfCandidates)
{
    if (m_PreserveCandidates)
        delete [] m_PreserveCandidates;
    m_PreserveCandidates = new pJittedMethodInfo[numberOfCandidates];
    if (m_PreserveCandidates)
        memset(m_PreserveCandidates,0,numberOfCandidates*sizeof(void*));
    m_PreserveCandidates_size = m_PreserveCandidates ? numberOfCandidates : 0;
    return;
}

/***************************************************************************************************
                    Code Memory Management
****************************************************************************************************/
// frees the old heap which is guaranteed to be empty and replaces it with a new one of specified size
void EconoJitManager::ReplaceCodeHeap(size_t newReservedSize,size_t newCommittedSize)
{
    _ASSERTE(m_CodeHeapReservedSize < newReservedSize);
    if (m_CodeHeap)
    {
        VirtualFree(m_CodeHeap,m_CodeHeapCommittedSize,MEM_DECOMMIT);
        VirtualFree(m_CodeHeap,0,MEM_RELEASE);
        //DEBUG_LOG("FREE(CodeHeap)",m_CodeHeapCommittedSize);
        ExecutionManager::DeleteRange(m_CodeHeap);
    }
    newReservedSize = RoundToPageSize(newReservedSize);
    newCommittedSize = RoundToPageSize(newCommittedSize);
    m_CodeHeap = (BYTE*)VirtualAlloc(NULL,newReservedSize,MEM_RESERVE,PAGE_EXECUTE_READWRITE);
    if (m_CodeHeap)
    {
        //DEBUG_LOG("ALLOC(ReplaceCodeHeap0)",newSize);
        ExecutionManager::AddRange((LPVOID)m_CodeHeap, (LPVOID)((size_t)m_CodeHeap + newReservedSize),this, NULL);
        m_CodeHeapFree = m_CodeHeap;
        m_CodeHeapReservedSize = newReservedSize;
        
        BYTE* additionalMemory = (BYTE*)VirtualAlloc(m_CodeHeap,
                                                     newCommittedSize,
                                                     MEM_COMMIT,
                                                     PAGE_EXECUTE_READWRITE);
        if (additionalMemory == NULL) 
        {
            m_CodeHeapCommittedSize = 0;
            return;
        }
        _ASSERTE(additionalMemory == m_CodeHeap);
        m_CodeHeapCommittedSize = newCommittedSize;
    }
    else
    {
        m_CodeHeapFree = NULL;
        m_CodeHeapCommittedSize = 0;
        m_CodeHeapReservedSize = 0;
    }
                                             
}

// newReservedSize is self-explanatory
// minCommittedSize is used if we fail to expand the reservedSize and have to pitch code
BOOL EconoJitManager::GrowCodeHeapReservedSpace(size_t newReservedSize,size_t minCommittedSize)
{
    _ASSERTE(newReservedSize > m_CodeHeapReservedSize);
    if (newReservedSize > g_pConfig->GetMaxCodeCacheSize())
        return false;

    // if got here, we either did not get any memory or got memory that was not contiguous
    // so we need to pitch code, and try again
    size_t minCommittedSizeReqd = max(minCommittedSize,m_CodeHeapCommittedSize);
    do
    {
        m_pHeapCritSec->Leave();
        BOOL result = PitchAllJittedMethods(newReservedSize,minCommittedSizeReqd,true,true);
        m_pHeapCritSec->Enter();
        if (result)
        {
            _ASSERTE(m_CodeHeap && m_CodeHeapReservedSize >= newReservedSize);
            return TRUE;
        }
        // else failed to grow to the new size, try something smaller
        newReservedSize -= MINIMUM_VIRTUAL_ALLOC_SIZE;
        if (minCommittedSizeReqd > newReservedSize)
            return FALSE;
    } while (newReservedSize > 0);

    return FALSE;
}

size_t EconoJitManager::RoundToPageSize(size_t size)
{
    size_t adjustedSize = (size + PAGE_SIZE-1)/PAGE_SIZE * PAGE_SIZE;
    if (adjustedSize < size)       // check for overflow
    {
        adjustedSize = (size/PAGE_SIZE)*PAGE_SIZE;
    }
    return adjustedSize;
}

// For now there is no pre-set limit on how big a request can be made so the function always returns true
BOOL EconoJitManager::SetCodeHeapCommittedSize(size_t size)    // sets code cache to the given size rounded to next page size
{
    _ASSERTE(IS_ALIGNED(m_CodeHeap,PAGE_SIZE));
    _ASSERTE(IS_ALIGNED(m_CodeHeapCommittedSize,PAGE_SIZE));

    size_t adjustedSize = RoundToPageSize(size);
    m_pHeapCritSec->Enter();
    if (adjustedSize <= m_CodeHeapCommittedSize)    // check again to make sure that no other
                                                    // thread beat us to this. 
    {
        m_pHeapCritSec->Leave();
        return true;
    }

    _ASSERTE(adjustedSize <= m_CodeHeapReservedSize && adjustedSize > m_CodeHeapCommittedSize);
    
    BYTE* additionalMemory = (BYTE*)VirtualAlloc(m_CodeHeap+m_CodeHeapCommittedSize,
                                                 adjustedSize - m_CodeHeapCommittedSize,
                                                 MEM_COMMIT,
                                                 PAGE_EXECUTE_READWRITE);
    if (additionalMemory == NULL) 
    {
        m_pHeapCritSec->Leave();
        return false;
    }
    _ASSERTE(additionalMemory == m_CodeHeap+m_CodeHeapCommittedSize);
    m_CodeHeapCommittedSize = adjustedSize;
    m_pHeapCritSec->Leave();
    return true;
   
}


unsigned char* EconoJitManager::allocCodeBlock(size_t blockSize)
{
    // Ensure minimal size
   /* if (blockSize < BBT)
        blockSize = BBT;
   */
    _ASSERTE(m_CodeHeap);
#ifdef DEBUGGING_SUPPORTED
    _ASSERTE(CORDebuggerAttached() || availableMemoryInCodeHeap() >= blockSize);
#else // !DEBUGGING_SUPPORTED
    _ASSERTE(availableMemoryInCodeHeap() >= blockSize);
#endif // !DEBUGGING_SUPPORTED

    if (availableMemoryInCodeHeap() < blockSize)
    {
        if (blockSize > m_CodeHeapReservedSize)
        {
            size_t adjustedSize = RoundToPageSize(blockSize); 
            if (!GrowCodeHeapReservedSpace(adjustedSize,adjustedSize))
                return NULL;
        }

        size_t additionalMemorySize = (((blockSize + PAGE_SIZE -1)/PAGE_SIZE) * PAGE_SIZE);
        BYTE* additionalMemory = (BYTE*)VirtualAlloc(m_CodeHeap+m_CodeHeapCommittedSize,
                                                 additionalMemorySize,
                                                 MEM_COMMIT,
                                                 PAGE_EXECUTE_READWRITE);
        if (additionalMemory == NULL) 
            return NULL;
        _ASSERTE(additionalMemory == m_CodeHeap+m_CodeHeapCommittedSize);
        m_CodeHeapCommittedSize += additionalMemorySize;
    }
    unsigned char* pAllocatedBlock = m_CodeHeapFree;
    m_CodeHeapFree += blockSize;
    return pAllocatedBlock;
}

// This should be called only to free the last allocated block within the 
// same sync block that made the allocation
void EconoJitManager::freeCodeBlock(size_t blockSize)
{
    _ASSERTE((size_t) (m_CodeHeapFree - m_CodeHeap) >= blockSize);
    m_CodeHeapFree -= blockSize;
}
/****************************************************************************************************/
//                      EHandGC Memory Management
/****************************************************************************************************/
BOOL EconoJitManager::NewEHGCBlock(size_t minsize)
{
    size_t allocsize = EHGC_BLOCK_SIZE;
    if (minsize + sizeof(EHGCBlockHdr) > allocsize) {
        allocsize = ((minsize + sizeof(EHGCBlockHdr) + EHGC_BLOCK_SIZE - 1)/EHGC_BLOCK_SIZE) * EHGC_BLOCK_SIZE;
    }

    EHGCBlockHdr* newBlock = (EHGCBlockHdr*)  VirtualAlloc(NULL,
                                                             allocsize,
                                                             MEM_RESERVE | MEM_COMMIT,
                                                             PAGE_READWRITE);
    if (!newBlock)
        return FALSE;
    //DEBUG_LOG("ALLOC(NewEHGCBlock",allocsize);
    WS_PERF_SET_HEAP(ECONO_JIT_HEAP);
    WS_PERF_UPDATE("NewEHGCBlock", allocsize, newBlock);
    
    newBlock->next = m_EHGCHeap;
    newBlock->blockSize = allocsize;
    m_EHGCHeap = newBlock;
    m_EHGC_block_end = ((unsigned char*) newBlock)+allocsize;
    newBlock++;
    m_EHGC_alloc_end = (unsigned char*) newBlock;
    return TRUE;

}

unsigned char* EconoJitManager::allocEHGCBlock(size_t blockSize)
{
    if ((!m_EHGCHeap || (availableEHGCMemory() < blockSize)) && !(NewEHGCBlock((unsigned)blockSize)))
        return NULL;
    
    _ASSERTE((unsigned) (m_EHGC_block_end - m_EHGC_alloc_end) >= blockSize);

    unsigned char* pAllocatedBlock = m_EHGC_alloc_end;
    m_EHGC_alloc_end += blockSize;

    return pAllocatedBlock;

}

void EconoJitManager::ResetEHGCHeap()
{
    size_t newEhGcBlockSize=0;
    while (m_EHGCHeap)
    {
        newEhGcBlockSize += (m_EHGCHeap->blockSize - sizeof(CodeBlockHdr));
        m_EHGCHeap = m_EHGCHeap->next;
    }
    if (!NewEHGCBlock(newEhGcBlockSize))
    {
        m_EHGCHeap = NULL;
        m_EHGC_alloc_end = 0;
        m_EHGC_block_end = 0;
    }
}




/*****************************************************************************************************
                        EjitStub Manager
******************************************************************************************************/
EjitStubManager::EjitStubManager()
{ }

EjitStubManager::~EjitStubManager()
{ }


