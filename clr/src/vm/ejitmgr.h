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

Module Name: EjitMgr.h

Abstract: Defines the EconojitManager Interface 

--*/


#ifndef H_EJITMGR
#define H_EJITMGR

#include <member-offset-info.h>

class EjitStubManager;

EXTERN_C BYTE* __cdecl RejitCalleeMethod(struct MachState * ms, void* args, BYTE* thunkReturnAddr);
EXTERN_C BYTE* __cdecl RejitCallerMethod(struct MachState * ms, void* retval, BYTE* thunkReturnAddr);
EXTERN_C void __stdcall InterlockedCompareExchange8b(UINT64 *pLocation, UINT64 *pValue, UINT64 *pComparand);

enum TransferStubType {
  DefaultStubType, // 0 -- JMI that points at either PreStub or jitted method address
  RejitThunkType,  // 1 -- Thunk pointing at the RejitThunk helper
  CallThunkType,   // 2 -- JMI pointing at the CallThunk helper
  ThunkStubType,   // 3
};
// Per platform functions for JMI and thunk stub manipulation
#ifdef _X86_

#define BREAK_OPCODE 0xCC
#define BREAK_OPCODE_TYPE BYTE

#define JMP_OPCODE   0xE9
#define CALL_OPCODE  0xE8
#define JMI_STUB_SIZE 5
#define THUNK_STUB_SIZE 5
#define THUNK_BEGIN_MASK 0xfffffff0
#define THUNK_ALIGNMENT  16

inline void CreateTransferStub( BYTE * StubStart, BOOL CallStub, DWORD * address, TransferStubType type )
{
    StubStart[0] = CallStub ? CALL_OPCODE : JMP_OPCODE;
    *((unsigned*) (StubStart+1)) = (unsigned) ((size_t)address - (size_t)(StubStart+1) - sizeof(void*));
}

inline void SwapTransferStub( BYTE * StubStart, BOOL CallStub, DWORD * address, TransferStubType type )
{
    UINT64 m64 = *(UINT64*)StubStart;
    UINT64 newInstruction = m64;
    *((BYTE*)(&newInstruction)) = CallStub ? CALL_OPCODE : JMP_OPCODE;
    *((unsigned*)((BYTE*)(&newInstruction)+1)) = (unsigned)((size_t)address - (size_t)(StubStart+1) - sizeof(void*));
    InterlockedCompareExchange8b( (UINT64 *)StubStart, &newInstruction, &m64 );
}

inline BYTE* ReadTransferStub( BYTE * StubStart, TransferStubType type )
{
  _ASSERTE( StubStart[0] == CALL_OPCODE ||  StubStart[0] == JMP_OPCODE);
  return (StubStart + 5 + *((DWORD*) (StubStart+1)));
}

inline BYTE* GetTransferStubStart( BYTE * RetAddr, TransferStubType type )
{
  BYTE * StubStart = RetAddr-5;
  _ASSERTE( StubStart[0] == CALL_OPCODE ||  StubStart[0] == JMP_OPCODE);
  return StubStart;
}

#elif defined(_PPC_) //!_X86_ && !_PPC_

#define BREAK_OPCODE  0x7ef00008
#define BREAK_OPCODE_TYPE DWORD

#define JMI_STUB_SIZE 20
#define THUNK_STUB_SIZE (36+7)  // 1 is pad to align the thunk on 32 byte boundary
#define THUNK_BEGIN_MASK 0xffffffc0
#define THUNK_ALIGNMENT  64

inline void CreateTransferStub( BYTE * StubStart, BOOL CallStub, DWORD * address, TransferStubType type )
{
    DWORD * StubStartD = (DWORD *)StubStart;
    // Add an extra logic upfront to handle returns from unmanaged functions
    if ( type == RejitThunkType )
    {
      StubStartD[0]    = 0x7C0802A6; StubStartD++;  // mflr r0
      StubStartD[0]    = 0x48000005; StubStartD++;  // bl +4
      StubStartD[0]    = 0x7D8802A6; StubStartD++;  // mflr r12
      StubStartD[0]    = 0x7C0803A6; StubStartD++;  // mtlr r0
    }    

    StubStartD[0]    = 0x7D8B6378;  // mr r11, r12
    if ( type == RejitThunkType )
      StubStartD[1]    = 0x818C0018;  // lwz r12, 24(r12)
    else
      StubStartD[1]    = 0x818C0010;  // lwz r12, 16(r12)
    StubStartD[2]    = 0x7D8903A6;  // mtctr r12
    StubStartD[3]    = 0x4E800420;  // bctr
    StubStartD[4]    = (DWORD)(size_t)address;
    FlushInstructionCache(GetCurrentProcess(), StubStart, 16);
}

inline void SwapTransferStub( BYTE * StubStart, BOOL CallStub, DWORD * address, TransferStubType type )
{
    //Make sure that the pointer actually points at a stub 
    _ASSERTE( (*(DWORD *)StubStart) == 0x7D8B6378 && 
              (type != RejitThunkType || (size_t)StubStart & THUNK_BEGIN_MASK == (size_t)StubStart));
    DWORD * StubStartD = ((DWORD *)StubStart)+4;
     // Skip the extra logic in front  
    if ( type == RejitThunkType )
      StubStartD += 4;
    // Atomically exchange the old address with new
    InterlockedExchangePointer((volatile PVOID *)StubStartD, (PVOID)address);
}

inline BYTE* ReadTransferStub( BYTE * StubStart, TransferStubType type )
{
  _ASSERTE(type != RejitThunkType || (size_t)StubStart & THUNK_BEGIN_MASK == (size_t)StubStart);
  DWORD * StubStartD = (DWORD *)StubStart;
  if ( type == RejitThunkType )
      StubStartD += 4;
  _ASSERTE( StubStartD[0] == 0x7D8B6378 && StubStartD[1] == 0x818C0010 &&
	    StubStartD[2] == 0x7D8903A6 && StubStartD[3] == 0x4E800420);
  return ((BYTE *)StubStartD[4]);
}

inline BYTE* GetTransferStubStart( BYTE * RetAddr, TransferStubType type )
{
  DWORD * StubStartD = ((DWORD *)RetAddr);

  // Align the rejit thunks
  if ( type == RejitThunkType )
     StubStartD = ((DWORD *)((size_t)StubStartD & THUNK_BEGIN_MASK));

  _ASSERTE( StubStartD[0 + (type == RejitThunkType? 4 :0)] == 0x7D8B6378 && 
	    StubStartD[3 + (type == RejitThunkType? 4 :0)] == 0x4E800420);
  return ((BYTE *)StubStartD);
}


#else //!_X86_ && !_PPC_ && !_SPARC_
    PORTABILITY_WARNING("Missing a valid definition for JittedMethodInfo.");
    
// This is simply filler to allow new platforms to compile. It will not
// work on any new platform!
#define BREAK_OPCODE 1
#define BREAK_TYPE BYTE

#define JMI_STUB_SIZE 1
#define THUNK_STUB_SIZE 1
#define THUNK_BEGIN_MASK 0xffffffff
#define THUNK_ALIGNMENT  1

inline void CreateTransferStub( BYTE * StubStart, BOOL CallStub, DWORD address )
{
  PORTABILITY_ASSERT(!"NYI - CreateTransferStub");
}

inline void SwapTransferStub( BYTE * StubStart, BOOL CallStub, DWORD address )
{
  PORTABILITY_ASSERT(!"NYI - SwapTransferStub");
}

inline BYTE* ReadTransferStub( BYTE * StubStart )
{
  PORTABILITY_ASSERT(!"NYI - ReadTransferStub");
  return NULL;
}

inline BYTE* GetTransferStubStart( BYTE * RetAddr )
{
  PORTABILITY_ASSERT(!"NYI - GetTransferStubStart");
  return NULL;
}
#endif // Various architectures


class EconoJitManager  :public IJitManager
{
        friend class EjitStubManager;
        friend struct MEMBER_OFFSET_INFO(EconoJitManager);
public:
    EconoJitManager();
    ~EconoJitManager();

    virtual void            JitCode2MethodTokenAndOffset(SLOT currentPC, METHODTOKEN* pMethodToken, DWORD* pPCOffset, ScanFlag scanFlag=ScanReaderLock);
    virtual MethodDesc*     JitCode2MethodDesc(SLOT currentPC, ScanFlag scanFlag=ScanReaderLock);
    static  void            JitCode2MethodTokenAndOffsetStatic(SLOT currentPC, METHODTOKEN* pMethodToken, DWORD* pPCOffset);
    static  void            JitCode2Offset(SLOT currentPC, DWORD* pPCOffset);
    static  BYTE*           JitToken2StartAddressStatic(METHODTOKEN MethodToken);
    virtual BYTE*           JitToken2StartAddress(METHODTOKEN MethodToken, ScanFlag scanFlag=ScanReaderLock);
    MethodDesc*             JitTokenToMethodDesc(METHODTOKEN MethodToken, ScanFlag scanFlag=ScanReaderLock);
    virtual unsigned        InitializeEHEnumeration(METHODTOKEN MethodToken, EH_CLAUSE_ENUMERATOR* pEnumState);
    virtual EE_ILEXCEPTION_CLAUSE*          GetNextEHClause(METHODTOKEN MethodToken,
                                            EH_CLAUSE_ENUMERATOR* pEnumState, 
                                            EE_ILEXCEPTION_CLAUSE* pEHclause); 
    virtual void            ResolveEHClause(METHODTOKEN MethodToken,
                                            EH_CLAUSE_ENUMERATOR* pEnumState, 
                                            EE_ILEXCEPTION_CLAUSE* pEHClauseOut);
    void*                   GetGCInfo(METHODTOKEN methodToken);
    virtual void            RemoveJitData(METHODTOKEN methodToken);

    virtual void            Unload(MethodDesc *pFD, UnloadStage);
    virtual void            Unload(AppDomain *pDomain, UnloadStage) { }
    virtual BOOL            LoadJIT(LPCWSTR wzJITdll);
    virtual void            ResumeAtJitEH   (CrawlFrame* pCf, EE_ILEXCEPTION_CLAUSE *EHClausePtr, DWORD nestingLevel, Thread *pThread, BOOL unwindStack);
    virtual int             CallJitEHFilter (CrawlFrame* pCf, EE_ILEXCEPTION_CLAUSE *EHClausePtr, DWORD nestingLevel, OBJECTREF thrownObj);
    virtual void            CallJitEHFinally(CrawlFrame* pCf, EE_ILEXCEPTION_CLAUSE *EHClausePtr, DWORD nestingLevel);

    HRESULT                 alloc(size_t code_len, unsigned char** pCode,
                                            size_t EHinfo_len, unsigned char** pEHinfo,
                                            size_t GCinfo_len, unsigned char** pGCinfo,
                                            MethodDesc* pMethodDescriptor);
    
    BOOL SupportsPitching(void)
    {
        return TRUE;
    }
    
    BOOL IsMethodInfoValid(METHODTOKEN methodToken)
    {
        return (BOOL) !(((JittedMethodInfo*)methodToken)->flags.EHandGCInfoPitched);
    }

    // Protect the thunk from being GC even if it is not found during the stackwalk
    virtual BOOL ProtectThunk( const BYTE * address, bool fProtect );
  
    inline virtual BOOL    IsStub(const BYTE* address)
    {
        // It's important to keep in mind that IsStub is used by the EJitMgr
        // and thus isn't concerned with thunks.  As opposed to CheckIsStub
        // on the EjitStubManager, which (being used by the debugger) is 
        // concerned with thunks.
        return IsInStub(address, FALSE);
    }

    virtual const BYTE*     FollowStub(const BYTE* address);

    // The following three should eventually be removed
    inline CodeHeader*         allocCode(MethodDesc* pFD, size_t numBytes)
    {
        _ASSERTE(!"NYI - should not get here!");
        return NULL;
    }

    inline BYTE*               allocGCInfo(CodeHeader* pCodeHeader, DWORD numBytes)
    {
        _ASSERTE(!"NYI - should not get here!");
        return NULL;
    }

    inline EE_ILEXCEPTION*     allocEHInfo(CodeHeader* pCodeHeader, unsigned numClauses)
    {
        _ASSERTE(!"NYI - should not get here!");
        return NULL;
    }

    BOOL	            PitchAllJittedMethods(size_t   minSpaceRequired,size_t   minCommittedSpaceRequired,BOOL PitchEHInfo, BOOL PitchGCInfo);
    // The following are called by the stackwalker callback
    void                AddPreserveCandidate(unsigned threadIndex,unsigned cThreads,unsigned candidateIndex,METHODTOKEN methodToken);
    void                CreateThunk(LPVOID* pHijackLocation,BYTE retTypeProtect, METHODTOKEN methodToken );
    static BOOL         IsThunk(const BYTE* address);
    static void         SetThunkBusy(BYTE* address, METHODTOKEN methodToken);
    friend BYTE*        __cdecl RejitCalleeMethod(struct MachState * ms, void* args, BYTE* thunkReturnAddr);
    friend BYTE*        __cdecl RejitCallerMethod(struct MachState * ms, void* retval, BYTE* thunkReturnAddr);
    static BOOL         IsInStub(const BYTE* address, BOOL fSearchThunks);
    static BOOL         IsCodePitched(const BYTE* address);
    inline virtual BYTE* GetNativeEntry(BYTE* startAddress)
    {
      return ReadTransferStub(startAddress, DefaultStubType);
    }


protected:
    typedef BYTE            GC_INFO;
    typedef EE_ILEXCEPTION  EH_INFO;
    typedef struct {
        EH_INFO*         phdrJitEHInfo;
        GC_INFO*         phdrJitGCInfo;
    } EH_AND_GC_INFO;

    typedef union {
        EH_AND_GC_INFO*   EHAndGC;
        EH_INFO*   EH;
        GC_INFO*   GC;
    } EH_OR_GC_INFO;

    typedef struct {
        MethodDesc *    pMethodDescriptor;
    } EJCodeHeader;

	typedef struct _link {			// a generic structure to implement singly linked list
		struct _link* next;
	} Link;

    struct JittedMethodInfo;
    // structure to create linked list of (jmi,EhGcInfo_len) pairs for very large methods.
    struct LargeEhGcInfoList {
        LargeEhGcInfoList *         next;
        JittedMethodInfo*           jmi;
        unsigned                    length;

        LargeEhGcInfoList(JittedMethodInfo* j, unsigned int l)
        {
            jmi = j;
            length = l;
        }
    };

    struct JittedMethodInfo {
        BYTE      JmpInstruction[JMI_STUB_SIZE]  ;        // this is the start address that is exposed to the EE so it can
                                                // patch all the vtables, etc. It contains a jump to the real start.
        struct {
            __int8 JittedMethodPitched: 1 ;   // if 1, the jitted method has been pitched
            __int8 MarkedForPitching  : 1 ;   // if 1, the jitted method is scheduled to be pitched, but has not been pitched yet
            __int8 EHInfoExists       : 1 ;   // if 0, no exception info in this method 
            __int8 GCInfoExists       : 1 ;   // if 0, no gc info in this method
            __int8 EHandGCInfoPitched : 1 ;   // (if at least one of EHInfo or GCInfo exists) if 1, the info has been pitched
            __int8 Unused             : 3 ;
        } flags;
        unsigned short EhGcInfo_len;
        union {
            MethodDesc* pMethodDescriptor;      // If code pitched
            EJCodeHeader* pCodeHeader;            // If not pitched : points to code header which points to the methoddesc. Code begins after the code header
        } u1;
        union {
            BYTE*       pEhGcInfo;        // If code pitched: points to beginning of EH/GC info
            BYTE*       pCodeEnd;               // If not pitched : points to end of jitted code for this method.
        } u2;
      
        void SetEhGcInfo_len(unsigned int len, LargeEhGcInfoList** ppEhGcInfoList) 
        {
            //_ASSERTE(adjusted_EhGc_len < 0xffff);
            if (len < 0xffff)
            {
                EhGcInfo_len = (unsigned short)len;
            }
            else
            {
                EhGcInfo_len = 0xffff;
                LargeEhGcInfoList* elem = new LargeEhGcInfoList(this,len);
                _ASSERTE(elem != NULL);
                elem->next = *ppEhGcInfoList;
                *ppEhGcInfoList = elem;
            }
        }

        unsigned GetEhGcInfo_len(LargeEhGcInfoList* pEhGcInfoList)
        {
            if (EhGcInfo_len < 0xffff)
                return EhGcInfo_len;
            LargeEhGcInfoList* ptr = pEhGcInfoList;
            while (ptr) 
            {
                if (ptr->jmi == this)
                    return ptr->length;
                ptr = ptr->next;
            }
            _ASSERTE(!"Error: EhGc length info corrupted");
            return 0;
        }
    };


    typedef struct {
        MethodDesc*     pMD;
        BYTE*           pCodeEnd;
    } PCToMDMap;

    typedef struct PC2MDBlock_tag {
        struct PC2MDBlock_tag *next;
    } PC2MDBlock;

#define JMIT_BLOCK_SIZE PAGE_SIZE           // size of individual blocks of JMITs that are chained together                     
    typedef struct JittedMethodInfoHdr_tag {
        struct JittedMethodInfoHdr_tag* next;          // ptr to next block  
    } JittedMethodInfoHdr; 

    typedef struct CodeBlock_tag {
        unsigned char*      startFree;       // beginning of free space in code region
        unsigned char*      end;             // end of code region
    } CodeBlockHdr; 

    typedef struct EHGCBlock_tag {
        struct EHGCBlock_tag* next;          // ptr to next EHGC block  
        size_t blockSize;
    } EHGCBlockHdr;
     
    typedef EHGCBlockHdr HeapList;

    typedef struct Thunk_tag {
        BYTE                CallInstruction[THUNK_STUB_SIZE]; // ***IMPORTANT: This should be first field. space to insert a call instruction here
        bool                Busy;               // used to mark thunks is use, is cleared before each stackwalk
        bool                LinkedInFreeList;   // set when thunks are threaded into a free list, to avoid being linked
                                                // twice during thunk garbage collection
        BYTE                retTypeProtect;     // if true, then return value must be protected
        union {
            JittedMethodInfo*   pMethodInfo;    
            struct Thunk_tag*   next;           // used to create linked list of free thunks
        } u;
        unsigned            relReturnAddress;
    }  PitchedCodeThunk;

        private:

#define CODE_BLOCK_SIZE PAGE_SIZE
#define EHGC_BLOCK_SIZE PAGE_SIZE
#define INITIAL_PC2MD_MAP_SIZE ((EHGC_BLOCK_SIZE/sizeof(JittedMethodInfo)) * sizeof(PCToMDMap))
#define THUNK_BLOCK_SIZE PAGE_SIZE
#define THUNKS_PER_BLOCK  ((PAGE_SIZE - sizeof(unsigned) - sizeof(void*))/ sizeof(PitchedCodeThunk))
#define DEFAULT_CODE_HEAP_RESERVED_SIZE 0x10000         
#define MINIMUM_VIRTUAL_ALLOC_SIZE 0x10000 // 64K
#define CODE_HEAP_RESERVED_INCREMENT_LIMIT  0x10000      // we double the reserved code heap size until we hit this limit
                                                         // after which we increase in this amount each time
#define DEFAULT_MAX_PRESERVES_PER_THREAD 10
#define DEFAULT_PRESERVED_EHGCINFO_SIZE 10

#define TARGET_MIN_JITS_BETWEEN_PITCHES 500 
#define MINIMUM_PITCH_OVERHEAD  10         // in milliseconds

    typedef struct ThunkBlockTag{
        struct ThunkBlockTag*   next;
        // This is to ensure that all thunks start at 16 byte boundaries
        size_t               Fillers[THUNK_ALIGNMENT / sizeof(size_t) - 1];   
    } ThunkBlock;

private:
    // JittedMethodInfo is kept as a linked list of 1 page blocks
    // Within each block there is a table of jittedMethodInfo structs. 
    typedef JittedMethodInfo* pJittedMethodInfo;
    static JittedMethodInfoHdr* m_JittedMethodInfoHdr;
    static JittedMethodInfo*    m_JMIT_free;         // points at start of next available entry
	static Link*				m_JMIT_freelist;	 // pointer to head of list of freed entries
    static PCToMDMap*           m_PcToMdMap;         
    static unsigned             m_PcToMdMap_len;
    static unsigned             m_PcToMdMap_size;
    static PC2MDBlock*          m_RecycledPC2MDMaps;  // a linked list of PC2MDMaps that are freed at pitch cycles

#define  m_JMIT_size  (PAGE_SIZE-sizeof(JittedMethodInfoHdr))/sizeof(JittedMethodInfo)
    static MethodDesc*          JitCode2MethodDescStatic(SLOT currentPC);
    
    static JittedMethodInfo*    JitCode2MethodInfo(SLOT currentPC);
    static MethodDesc*          JitMethodInfo2MethodDesc(JittedMethodInfo* jittedMethodInfo);
    JittedMethodInfo*           Token2JittedMethodInfo(METHODTOKEN methodToken);
    static BYTE*                JitMethodInfo2EhGcInfo(JittedMethodInfo* jmi);
    JittedMethodInfo*           MethodDesc2MethodInfo(MethodDesc* pMethodDesc);
	static JittedMethodInfo*    JitCode2MethodTokenInEnCMode(SLOT currentPC);

    static HINSTANCE           m_JITCompiler;
    static BYTE*               m_CodeHeap;
    static BYTE*               m_CodeHeapFree;           // beginning of free space in code heap
    static size_t              m_CodeHeapTargetSize;     // this size is increased as needed until the global max is reached
    static size_t              m_CodeHeapCommittedSize;  // this is a number between 0 and m_CodeHeapReservedSize
    static size_t              m_CodeHeapReservedSize;
    static size_t              m_CodeHeapReserveIncrement;
 
    static EHGCBlockHdr*       m_EHGCHeap;
    static unsigned char*      m_EHGC_alloc_end;      // ptr to next free byte in current block
    static unsigned char*      m_EHGC_block_end;      // ptr to end of current block

    static Crst*               m_pHeapCritSec;
    static BYTE                m_HeapCritSecInstance[sizeof(Crst)];
    static EjitStubManager*    m_stubManager;
    static Crst*               m_pRejitCritSec;
    static BYTE                m_RejitCritSecInstance[sizeof(Crst)];
    static Crst*               m_pThunkCritSec;          // used to synchronize concurrent creation of thunks
    static BYTE                m_ThunkCritSecInstance[sizeof(Crst)];
    static ThunkBlock*         m_ThunkBlocks;
    static PitchedCodeThunk*   m_FreeThunkList;
    static unsigned            m_cThunksInCurrentBlock;         // total number of thunks in current block
    static unsigned            m_cMethodsJitted;                // number of methods jitted since last pitch
    static unsigned            m_cCalleeRejits;                 // number of callees rejitted since last pitch
    static unsigned            m_cCallerRejits;                 // number of callers rejitted since last pitch
    static JittedMethodInfo**  m_PreserveCandidates;            // methods that are possible candidates for pitching
    static unsigned            m_PreserveCandidates_size;       // current size of m_PreserveCandidates array
    static JittedMethodInfo**  m_PreserveEhGcInfoList;          // list of EhGc info that needs to be preserved during pitching
    static unsigned            m_cPreserveEhGcInfoList;         // count of members in m_PreserveEhGcInfoList
    static unsigned            m_PreserveEhGcInfoList_size;     // current size of m_PreserveEhGcInfoList
    static unsigned            m_MaxUnpitchedPerThread;         // maximum number of methods in each thread that will be pitched
    static LargeEhGcInfoList*  m_LargeEhGcInfo;                 // linked list of structures that encode EhGcInfo length >= 64K
    
    // Clock ticks measurement for code pitch heuristics
#ifdef _X86_
    typedef __int64 TICKS;
    static TICKS GET_TIMESTAMP()
    {
        LARGE_INTEGER lpPerfCtr;
#ifdef _DEBUG
        BOOL exists =
#endif
		QueryPerformanceCounter(&lpPerfCtr);
        _ASSERTE(exists);
        return lpPerfCtr.QuadPart;
    }
    static TICKS TICK_FREQUENCY()
    {
        LARGE_INTEGER lpPerfCtr;
#ifdef _DEBUG
        BOOL exists =
#endif
		QueryPerformanceFrequency(&lpPerfCtr);
        _ASSERTE(exists);
        return lpPerfCtr.QuadPart;
    }
#else // defined(_X86_)
    typedef unsigned TICKS;
    static TICKS GET_TIMESTAMP()
    {
        return GetTickCount();
    }
    static TICKS TICK_FREQUENCY()
    {
        return 1000;
    }
#endif // defined(_X86_)

    static TICKS               m_CumulativePitchOverhead;       // measures the total overhead due to pitching and rejitting
    static TICKS               m_AveragePitchOverhead;
    static unsigned            m_cPitchCycles;
    static TICKS               m_EjitStartTime;

#ifdef _DEBUG
    static DWORD               m_RejitLock_Holder;  
    static DWORD               m_AllocLock_Holder; 
#endif
    inline BOOL IsMethodPitched(METHODTOKEN token)
    {
        return ((JittedMethodInfo*)token)->flags.JittedMethodPitched;
    }
    // Memory management support 
    static BOOL                 m_PitchOccurred;                // initially false, set to true after the first pitch
    unsigned char*              allocCodeBlock(size_t blockSize);
    void                        freeCodeBlock(size_t blockSize);


	size_t						minimum(size_t x, size_t y);

    size_t						InitialCodeHeapSize();
    void                        ReplaceCodeHeap(size_t newReservedSize,size_t newCommittedSize);
    size_t                      RoundToPageSize(size_t size);
    BOOL                        GrowCodeHeapReservedSpace(size_t newReservedSize,size_t minCommittedSize);
    BOOL                        SetCodeHeapCommittedSize(size_t size);


    inline size_t     usedMemoryInCodeHeap()
    {
        return (m_CodeHeapFree - m_CodeHeap);
    }
    inline size_t     availableMemoryInCodeHeap()
    {
        return (m_CodeHeapCommittedSize - usedMemoryInCodeHeap());
    }

    inline BOOL OutOfCodeMemory(size_t newRequest)
    {
      /*if ( newRequest > availableMemoryInCodeHeap() )
	printf( "Requested %d, Avail %d Committed %d Used %d\n", newRequest, availableMemoryInCodeHeap(), m_CodeHeapCommittedSize, usedMemoryInCodeHeap() );*/
        return (newRequest > availableMemoryInCodeHeap());
    }

    BOOL                NewEHGCBlock(size_t minsize);
    unsigned char*      allocEHGCBlock(size_t blockSize);
    
    inline size_t availableEHGCMemory()
    {
        return (m_EHGC_block_end - m_EHGC_alloc_end);
    }

    void                ResetEHGCHeap();
    void                InitializeCodeHeap();
    static BYTE*        RejitMethod(JittedMethodInfo* pJMI, unsigned returnOffset);
    void                StackWalkForCodePitching();
    unsigned            GetThreadCount();
    void                MarkHeapsForPitching();
    void                UnmarkPreservedCandidates(size_t minSpaceRequired);
    unsigned            PitchMarkedCode();          // returns number of methods pitched
    void                MovePreservedMethods();
    unsigned            CompressPreserveCandidateArray(unsigned size);
    void                MovePreservedMethod(JittedMethodInfo* jmi);
    static int __cdecl  compareJMIstart( const void *arg1, const void *arg2 );

    void                MoveAllPreservedEhGcInfo();
    void                MoveSinglePreservedEhGcInfo(JittedMethodInfo* jmi);
    void                AddToPreservedEhGcInfoList(JittedMethodInfo* jmi);
    static int __cdecl  compareEhGcPtr( const void *arg1, const void *arg2 );
    void                growPreservedEhGcInfoList();
    void static         CleanupLargeEhGcInfoList();

    __inline void AddPitchOverhead(TICKS time)
    {
        // this is always called within a single user critical section  
#ifndef _X86_
      //_ASSERTE(!"NYI");
        if (time == 0) 
            time = 1;       // make sure we allocate at least a millisecond for each pitch overhead
#endif
        m_CumulativePitchOverhead += time;
        m_cPitchCycles++;
        m_AveragePitchOverhead = (m_AveragePitchOverhead + m_CumulativePitchOverhead)/(m_cPitchCycles+1);
        if (m_AveragePitchOverhead == 0)
            m_AveragePitchOverhead = 1;
    }
    __inline static void AddRejitOverhead(TICKS time)
    {
#ifndef _X86_
      //_ASSERTE(!"NYI");
        if (time == 0) 
            time = 1;       // make sure we allocate at least a millisecond for each pitch overhead
#endif
        // this is always called within a single user critical section        
        m_CumulativePitchOverhead += time;
    }

    __inline TICKS EconoJitManager::PitchOverhead()
    {
        // this is always called within a single user critical section  
        TICKS totalExecTime = GET_TIMESTAMP()-m_EjitStartTime;
        if (totalExecTime > m_CumulativePitchOverhead)      // this can only be possible if we don't have a high resolution ctr
            totalExecTime -= m_CumulativePitchOverhead;
        return ((m_CumulativePitchOverhead+m_AveragePitchOverhead)*100)/(totalExecTime);
    }
    PitchedCodeThunk*   GetNewThunk();
    void                MarkThunksForRelease(); 
    void                GarbageCollectUnusedThunks();
    void                growPreserveCandidateArray(unsigned numberOfCandidates);
    JittedMethodInfo*   GetNextJmiEntry();
    BOOL                growJittedMethodInfoTable();
    BOOL                growPC2MDMap();
    BOOL                AddPC2MDMap(MethodDesc* pMD, BYTE* pCodeEnd);


    void __inline ResetPc2MdMap()
    {
        m_PcToMdMap_len = 0;
        // delete each element in m_RecycledPC2MDMaps
        while (m_RecycledPC2MDMaps)
        {
            PCToMDMap* temp = (PCToMDMap*) m_RecycledPC2MDMaps;
            m_RecycledPC2MDMaps = m_RecycledPC2MDMaps->next;
            delete[] temp;
        }
        
    }

#ifdef _DEBUG
    void                SetBreakpointsInUnusedHeap();
    void                VerifyAllCodePitched();
    void                LogAction(MethodDesc* pMD, LPCUTF8 action,void* codeStart ,void* codeEnd);

#endif

#if defined(ENABLE_PERF_COUNTERS)
    size_t                 GetCodeHeapSize();
    size_t                 GetEHGCHeapSize();
#endif // ENABLE_PERF_COUNTERS
};

const BYTE *GetCallThunkAddress();
const BYTE *GetRejitThunkAddress();

class EjitStubManager :public StubManager
{
public:
    EjitStubManager();
    ~EjitStubManager();
protected:

        // It's important to keep in mind that CheckIsStub
        // which (being used by the debugger) is 
        // concerned with thunks.  As opposed to IsStub, which 
        // is used by the EconoJitManager and thus isn't concerned with thunks.  
    __inline BOOL CheckIsStub(const BYTE *stubStartAddress)
    {
        LOG((LF_CORDB, LL_INFO1000000,
            "EjitStubManager::CheckIsStub stubStartAddress = 0x%x, return 0x%x\n", stubStartAddress, EconoJitManager::IsInStub(stubStartAddress, TRUE)));

        return EconoJitManager::IsInStub(stubStartAddress, TRUE);
    }


    __inline virtual BOOL DoTraceStub(const BYTE *stubStartAddress, 
                                      TraceDestination *trace)
    {
        trace->type = TRACE_MANAGED;
#ifdef _X86_
        trace->address = stubStartAddress + 5 +
            *((DWORD*) (stubStartAddress+1));
#elif defined(_PPC_) || defined(_SPARC_)
        trace->address = *(BYTE**)(stubStartAddress+16);
#else
        PORTABILITY_ASSERT("DoTraceStub not implemented on this platform\n");
#endif
        if (trace->address == GetCallThunkAddress() )
        {
            MethodDesc* pMD = EconoJitManager::
                JitMethodInfo2MethodDesc(
                   (EconoJitManager::JittedMethodInfo *)
                                stubStartAddress);
            trace->type = TRACE_UNJITTED_METHOD;
            trace->address = (const BYTE*)pMD;
        }

        if ( trace->address == GetRejitThunkAddress())
        {
            _ASSERTE( offsetof( EconoJitManager::PitchedCodeThunk,CallInstruction)==0);
        
            EconoJitManager::PitchedCodeThunk *pct =
                                (EconoJitManager::PitchedCodeThunk *)stubStartAddress;
                                
            EconoJitManager::JittedMethodInfo *jmi = 
                    (EconoJitManager::JittedMethodInfo *)pct->u.pMethodInfo;

            _ASSERTE( jmi != NULL );

            if (jmi->flags.JittedMethodPitched)
            {
                trace->type = TRACE_UNJITTED_METHOD;
                trace->address = (const BYTE*)EconoJitManager
                        ::JitMethodInfo2MethodDesc(jmi);
            }
            else
            {
                trace->type = TRACE_MANAGED;
                trace->address = ((const BYTE*)jmi->u1.pCodeHeader) +
                        sizeof( MethodDesc *);
            }
        }
        return true;
    }

    MethodDesc *Entry2MethodDesc(const BYTE *StubStartAddress, MethodTable *pMT)  {return NULL;}
};

#endif
 
