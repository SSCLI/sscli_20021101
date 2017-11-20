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

Module Name:

    gc.h

--*/

#ifndef __GC_H
#define __GC_H

#ifdef PROFILING_SUPPORTED
#define GC_PROFILING       //Turn on profiling
#endif // PROFILING_SUPPORTED

/*
 * Promotion Function Prototypes
 */
typedef void enum_func (Object*);


/* forward declerations */
class gc_heap;
class CFinalize;
class CObjectHeader;
class Object;


/* misc defines */
#define LARGE_OBJECT_SIZE   85000

extern "C" BYTE* g_lowest_address;
extern "C" BYTE* g_highest_address;
extern "C" DWORD* g_card_table;

#ifdef _DEBUG
#define  _LOGALLOC
#endif

inline void initGCShadow() {}
inline void deleteGCShadow() {}
inline void updateGCShadow(Object** ptr, Object* val) {}
inline void checkGCWriteBarrier() {}


void setCardTableEntryInterlocked(BYTE* location, BYTE* ref);

//server specific settings. 



#define PER_HEAP static



#define PER_HEAP_ISOLATED static


extern "C" BYTE* g_ephemeral_low;
extern "C" BYTE* g_ephemeral_high;


/*
 * Ephemeral Garbage Collected Heap Interface
 */

struct alloc_context 
{
    BYTE*          alloc_ptr;
    BYTE*          alloc_limit;
    __int64         alloc_bytes; //Number of bytes allocated by this context

    void init()
    {
        alloc_ptr = 0;
        alloc_limit = 0;
        alloc_bytes = 0;
    }
};

struct ScanContext
{
    Thread* thread_under_crawl;
    int thread_number;
    BOOL promotion; //TRUE: Promotion, FALSE: Relocation.
    BOOL concurrent; //TRUE: concurrent scanning 
#if CHECK_APP_DOMAIN_LEAKS
    AppDomain *pCurrentDomain;
#endif
    
    ScanContext()
    {
        thread_under_crawl = 0;
        thread_number = -1;
        promotion = FALSE;
        concurrent = FALSE;
    }
};

#ifdef GC_PROFILING

struct ProfilingScanContext : ScanContext
{
    void *pHeapId;

    ProfilingScanContext() : ScanContext()
    {
        pHeapId = NULL;
    }
};

typedef BOOL (* walk_fn)(Object*, void*);
void walk_object (Object* obj, walk_fn fn, void* context);

#endif //GC_PROFILING

//dynamic data interface
struct gc_counters
{
    size_t current_size;
    size_t promoted_size;
    size_t collection_count;
};

//constants for the flags parameter to the gc call back

#define GC_CALL_INTERIOR            0x1
#define GC_CALL_PINNED              0x2
#define GC_CALL_CHECK_APP_DOMAIN    0x4

//flags for GCHeap::Alloc(...)
#define GC_ALLOC_FINALIZE 0x1
#define GC_ALLOC_CONTAINS_REF 0x2


class GCHeap
{
    friend HRESULT InitializeMiniDumpBlock();
    friend struct MEMBER_OFFSET_INFO(GCHeap);

protected:

    #define pGenGCHeap ((gc_heap*)0)
    
    friend class CFinalize;
    friend class gc_heap;
    friend void EnterAllocLock();
    friend void LeaveAllocLock();
    friend void STDMETHODCALLTYPE EEShutDown(BOOL fIsDllUnloading);
    friend void ProfScanRootsHelper(Object*& object, ScanContext *pSC, DWORD dwFlags);
    friend void GCProfileWalkHeap();

    //In order to keep gc.cpp cleaner, ugly EE specific code is relegated to methods. 
    PER_HEAP_ISOLATED   void UpdatePreGCCounters();
    PER_HEAP_ISOLATED   void UpdatePostGCCounters();

public:
    GCHeap(){};
    ~GCHeap(){};

    /* BaseGCHeap Methods*/
    PER_HEAP_ISOLATED   HRESULT Shutdown ();

    PER_HEAP_ISOLATED   size_t  GetTotalBytesInUse ();

    PER_HEAP_ISOLATED   BOOL    IsGCInProgress ()    
    { return GcInProgress; }

    PER_HEAP    Thread* GetGCThread ()       
    { return GcThread; };

    PER_HEAP    Thread* GetGCThreadAttemptingSuspend()
    {
        return m_GCThreadAttemptingSuspend;
    }

    PER_HEAP_ISOLATED   void    WaitUntilGCComplete ();

    PER_HEAP            HRESULT Initialize ();

    //flags can be GC_ALLOC_CONTAINS_REF GC_ALLOC_FINALIZE
    PER_HEAP_ISOLATED Object*  Alloc (DWORD size, DWORD flags);
    PER_HEAP_ISOLATED Object*  AllocLHeap (DWORD size, DWORD flags);
    
    PER_HEAP_ISOLATED Object* Alloc (alloc_context* acontext, 
                                         DWORD size, DWORD flags);

    PER_HEAP_ISOLATED void FixAllocContext (alloc_context* acontext,
                                            BOOL lockp, void* arg);

    
    static BOOL IsLargeObject(MethodTable *mt);

    static BOOL IsObjectInFixedHeap(Object *pObj);

    PER_HEAP_ISOLATED       HRESULT GarbageCollect (int generation = -1, 
                                        BOOL collectClasses=FALSE);

    // Drain the queue of objects waiting to be finalized.
    PER_HEAP_ISOLATED    void    FinalizerThreadWait();

    ////
    // GC callback functions
    // Check if an argument is promoted (ONLY CALL DURING
    // THE PROMOTIONSGRANTED CALLBACK.)
    PER_HEAP_ISOLATED    BOOL    IsPromoted (Object *object, 
                                             ScanContext* sc);

    // promote an object
    PER_HEAP_ISOLATED    void    Promote (Object*& object, 
                                          ScanContext* sc,
                                          DWORD flags=0);

    // Find the relocation address for an object
    PER_HEAP_ISOLATED    void    Relocate (Object*& object,
                                           ScanContext* sc, 
                                           DWORD flags=0);


    PER_HEAP            HRESULT Init (size_t heapSize);

    //Register an object for finalization
    PER_HEAP_ISOLATED    void    RegisterForFinalization (int gen, Object* obj); 
    
    //Unregister an object for finalization
    PER_HEAP_ISOLATED    void    SetFinalizationRun (Object* obj); 
    
    //returns the generation number of an object (not valid during relocation)
    PER_HEAP_ISOLATED    unsigned WhichGeneration (Object* object);
    // returns TRUE is the object is ephemeral 
    PER_HEAP_ISOLATED    BOOL    IsEphemeral (Object* object);
#ifdef VERIFY_HEAP
    PER_HEAP_ISOLATED    BOOL    IsHeapPointer (void* object, BOOL small_heap_only = FALSE);
#endif //_DEBUG

    PER_HEAP    size_t  ApproxTotalBytesInUse(BOOL small_heap_only = FALSE);
    PER_HEAP    size_t  ApproxFreeBytes();
    

    static      BOOL    HandlePageFault(void*);//TRUE handled, FALSE propagate

    PER_HEAP_ISOLATED   unsigned GetCondemnedGeneration()
    { return GcCondemnedGeneration;}


    PER_HEAP_ISOLATED     unsigned GetMaxGeneration();
 
    //suspend all threads

    typedef enum
    {
        SUSPEND_OTHER                   = 0,
        SUSPEND_FOR_GC                  = 1,
        SUSPEND_FOR_APPDOMAIN_SHUTDOWN  = 2,
        SUSPEND_FOR_CODE_PITCHING       = 3,
        SUSPEND_FOR_SHUTDOWN            = 4,
        SUSPEND_FOR_DEBUGGER            = 5,
        SUSPEND_FOR_INPROC_DEBUGGER     = 6,
        SUSPEND_FOR_GC_PREP             = 7
    } SUSPEND_REASON;

    PER_HEAP_ISOLATED void SuspendEE(SUSPEND_REASON reason);

    PER_HEAP_ISOLATED void RestartEE(BOOL bFinishedGC, BOOL SuspendSucceded); //resume threads. 

    PER_HEAP_ISOLATED inline SUSPEND_REASON GetSuspendReason()
    { return (m_suspendReason); }

    PER_HEAP_ISOLATED inline void SetSuspendReason(SUSPEND_REASON suspendReason)
    { m_suspendReason = suspendReason; }

    PER_HEAP_ISOLATED  Thread* GetFinalizerThread();

        //  Returns TRUE if the current thread is the finalizer thread.
    PER_HEAP_ISOLATED   BOOL    IsCurrentThreadFinalizer();

    // allow finalizer thread to run
    PER_HEAP_ISOLATED    void    EnableFinalization( void );

    // Start unloading app domain
    PER_HEAP_ISOLATED   void    UnloadAppDomain( AppDomain *pDomain, BOOL fRunFinalizers ) 
      { UnloadingAppDomain = pDomain; fRunFinalizersOnUnload = fRunFinalizers; }

    // Return current unloading app domain (NULL when unload is finished.)
    PER_HEAP_ISOLATED   AppDomain*  GetUnloadingAppDomain() { return UnloadingAppDomain; }

    // Lock for allocation Public because of the 
    // fast allocation helper

    PER_HEAP_ISOLATED unsigned GetGcCount() { return GcCount; }

    PER_HEAP_ISOLATED HRESULT GetGcCounters(int gen, gc_counters* counters);

    static BOOL IsValidSegmentSize(size_t cbSize);

    static BOOL IsValidGen0MaxSize(size_t cbSize);

    static size_t GetValidSegmentSize();

    static size_t GetValidGen0MaxSize(size_t seg_size);

    PER_HEAP_ISOLATED void SetReservedVMLimit (size_t vmlimit);

    PER_HEAP_ISOLATED Object* GetNextFinalizableObject();
    PER_HEAP_ISOLATED size_t GetNumberFinalizableObjects();
    PER_HEAP_ISOLATED size_t GetFinalizablePromotedCount();
    PER_HEAP_ISOLATED BOOL FinalizeAppDomain(AppDomain *pDomain, BOOL fRunFinalizers);
    PER_HEAP_ISOLATED void SetFinalizeQueueForShutdown(BOOL fHasLock);

protected:

    // Lock for finalization
    PER_HEAP_ISOLATED   
        volatile        LONG    m_GCFLock;

    PER_HEAP_ISOLATED   BOOL    GcCollectClasses;
    PER_HEAP_ISOLATED
        volatile        BOOL    GcInProgress;       // used for syncing w/GC
    PER_HEAP_ISOLATED
              SUSPEND_REASON    m_suspendReason;    // This contains the reason
                                                    // that the runtime was suspended
    PER_HEAP_ISOLATED   Thread* GcThread;           // thread running GC
    PER_HEAP_ISOLATED   Thread* m_GCThreadAttemptingSuspend;
    PER_HEAP_ISOLATED   unsigned GcCount;
    PER_HEAP_ISOLATED   unsigned GcCondemnedGeneration;

    
    // Use only for GC tracing.
    PER_HEAP    unsigned long GcDuration;



    // Interface with gc_heap
    PER_HEAP_ISOLATED   void    GarbageCollectGeneration (unsigned int gen=0, 
                                                  BOOL collectClasses = FALSE);
    // Finalizer thread stuff.


    
    PER_HEAP_ISOLATED   BOOL    FinalizerThreadWatchDog();
    PER_HEAP_ISOLATED   BOOL    FinalizerThreadWatchDogHelper();
    PER_HEAP_ISOLATED   DWORD   FinalizerThreadCreate();
    PER_HEAP_ISOLATED   ULONG   __stdcall FinalizerThreadStart(void *args);
    PER_HEAP_ISOLATED   HANDLE  WaitForGCEvent;     // used for syncing w/GC
    PER_HEAP_ISOLATED   HANDLE  hEventFinalizer;
    PER_HEAP_ISOLATED   HANDLE  hEventFinalizerDone;
    PER_HEAP_ISOLATED   HANDLE  hEventFinalizerToShutDown;
    PER_HEAP_ISOLATED   HANDLE  hEventShutDownToFinalizer;
    PER_HEAP_ISOLATED   BOOL    fQuitFinalizer;
    PER_HEAP_ISOLATED   Thread *FinalizerThread;
    PER_HEAP_ISOLATED   AppDomain *UnloadingAppDomain;
    PER_HEAP_ISOLATED   BOOL    fRunFinalizersOnUnload;

    PER_HEAP_ISOLATED    CFinalize* m_Finalize;

    PER_HEAP_ISOLATED   gc_heap* Getgc_heap();

#ifdef STRESS_HEAP 
public:
    PER_HEAP_ISOLATED   void    StressHeap(alloc_context * acontext = 0);
protected:

    // handles to hold the string objects that will force GC movement
    enum { NUM_HEAP_STRESS_OBJS = 8 };
    PER_HEAP OBJECTHANDLE m_StressObjs[NUM_HEAP_STRESS_OBJS];
    PER_HEAP int m_CurStressObj;
#endif  // STRESS_HEAP 


};

    extern volatile LONG m_GCLock;

void SetCardsAfterBulkCopy( Object**, size_t );



//#define TOUCH_ALL_PINNED_OBJECTS  // Force interop to touch all pages straddled by pinned objects.


// Go through and touch (read) each page straddled by a memory block.
void TouchPages(LPVOID pStart, UINT cb);

#ifdef VERIFY_HEAP
void    ValidateObjectMember (Object *obj);
#endif

#endif // __GC_H
