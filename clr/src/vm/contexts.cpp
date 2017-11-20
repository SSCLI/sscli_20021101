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
/*  Contexts.CPP:
 *
 *  Implementation for class Context
 */

#include "common.h"

#include "context.h"

#include "excep.h"
#include "field.h"
#include "remoting.h"
#include "perfcounters.h"
#include "specialstatics.h"


#define     NEW_CLS     1

#define IDS_CONTEXT_LOCK            "Context"           // Context lock                                      

BOOL Context::s_fInitializedContext;                    // Static fields inited?
CrstStatic Context::s_ContextCrst;                      // Lock for safe operations

MethodTable *Context::s_pContextMT;     // Method Table for the managed class
MethodDesc *Context::s_pDoCallBackFromEE;//Method Desc for requesting callbacks
MethodDesc *Context::s_pReserveSlot;    //Method Desc for reserving ctx static slots for objRef types

//Method Desc for managed Thread::Get_CurrentContext (property)
MethodDesc *Context::s_pThread_CurrentContext;

#define CONTEXT_SIGNATURE   (0x2b585443)    // CTX+
#define CONTEXT_DESTROYED   (0x2d585443)    // CTX-


Context::Context(AppDomain *pDomain)
{
    SetDomain(pDomain);
    m_Signature = CONTEXT_SIGNATURE;
    
    // This needs to be a LongWeakHandle since we want to be able
    // to run finalizers on Proxies while the Context itself 
    // unreachable. When running the finalizer we will have to 
    // transition into the context like a regular remote call.
    // If this is a short weak handle, it ceases being updated
    // as soon as the context is unreachable. By making it a strong
    // handle, it is updated till the context::finalize is run.

    m_ExposedObjectHandle = pDomain->CreateLongWeakHandle(NULL);

    // Set the pointers to the static data storage
    m_pUnsharedStaticData = NULL;
    m_pSharedStaticData = NULL;
    
    COUNTER_ONLY(GetPrivatePerfCounters().m_Context.cContexts++);
    COUNTER_ONLY(GetGlobalPerfCounters().m_Context.cContexts++);
}

Context::~Context()
{
    DestroyLongWeakHandle(m_ExposedObjectHandle);

    SetDomain(NULL);
    
    m_Signature = CONTEXT_DESTROYED;

    // Cleanup the static data storage
    if(m_pUnsharedStaticData)
    {
        for(WORD i = 0; i < m_pUnsharedStaticData->cElem; i++)
        {
            //delete (LPVOID)m_pUnsharedStaticData->dataPtr[i];
            HeapFree(g_hProcessHeap, 0, (LPVOID)m_pUnsharedStaticData->dataPtr[i]);
        }
        HeapFree(g_hProcessHeap, 0, m_pUnsharedStaticData);
        //delete m_pUnsharedStaticData;
        m_pUnsharedStaticData = NULL;
    }

    if(m_pSharedStaticData)
    {
        for(WORD i = 0; i < m_pSharedStaticData->cElem; i++)
        {
            HeapFree(g_hProcessHeap, 0, m_pSharedStaticData->dataPtr[i]);
            //delete (LPVOID)m_pSharedStaticData->dataPtr[i];
        }
        HeapFree(g_hProcessHeap, 0, m_pSharedStaticData);
        //delete m_pSharedStaticData; 
        m_pSharedStaticData = NULL;
    }
    
    COUNTER_ONLY(GetPrivatePerfCounters().m_Context.cContexts--);
    COUNTER_ONLY(GetGlobalPerfCounters().m_Context.cContexts--);
}

// static
Context* Context::CreateNewContext(AppDomain *pDomain)
{
    void *p = HeapAlloc(g_hProcessHeap, HEAP_ZERO_MEMORY, sizeof(Context));
    if (p == NULL) FailFast(GetThread(), FatalOutOfMemory);
    return new (p) Context(pDomain);
}

// static
BOOL Context::Initialize()
{
    s_fInitializedContext = FALSE;

    // Initialize the context critical section
    s_ContextCrst.Init(IDS_CONTEXT_LOCK,CrstRemoting, TRUE, FALSE);

    return TRUE;
}

//static
BOOL Context::ValidateContext(Context *pCtx)
{

    _ASSERTE(pCtx != NULL);
    BOOL bRet = FALSE;
    EE_TRY_FOR_FINALLY
    {
        if (pCtx->m_Signature == CONTEXT_SIGNATURE)
        {
            bRet = TRUE;
        }
    }
    EE_FINALLY
    {
        if (GOT_EXCEPTION()) 
        {
            // This is a bogus context!
            bRet = FALSE;
        }
    } EE_END_FINALLY;
    return bRet;
}

// static
Context *Context::SetupDefaultContext(AppDomain *pDomain)
{
    Context *pCtx = ::new Context(pDomain);
    _ASSERTE(pDomain != NULL);
    return pCtx;
}

void Context::CleanupDefaultContext(AppDomain *pDomain)
{
    delete pDomain->GetDefaultContext();
}

void Context::EnterLock()
{
    s_ContextCrst.Enter();
}

void Context::LeaveLock()
{
    s_ContextCrst.Leave();
}

// Query whether the exposed object exists
BOOL Context::IsExposedObjectSet()
{
    return (ObjectFromHandle(m_ExposedObjectHandle) != NULL) ;
}

//+----------------------------------------------------------------------------
//
//  Method:     Context::Cleanup    public
//
//  Synopsis:   Clean up the context related data structures
//
//
//  History:    02-Dec-99                       Created
//
//+----------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
//  Method:     Context::InitializeFields    private
//
//  Synopsis:   Extract the method descriptors and fields of Context class
//
//
//  History:    02-Dec-99                       Created
//
//+----------------------------------------------------------------------------
BOOL Context::InitializeFields()
{
    BOOL fReturn = TRUE;

    // Acquire the lock 
    Thread *t = GetThread();
    BOOL toggleGC = (t && t->PreemptiveGCDisabled());
    if (toggleGC)
        t->EnablePreemptiveGC();
    s_ContextCrst.Enter();
    if (toggleGC)
        t->DisablePreemptiveGC();

    if(!s_fInitializedContext)
    {
        s_pContextMT = g_Mscorlib.GetClass(CLASS__CONTEXT);

        // Cache the methodDesc for Context.DoCallBackFromEE
        s_pDoCallBackFromEE = g_Mscorlib.GetMethod(METHOD__CONTEXT__CALLBACK);

        s_pReserveSlot = g_Mscorlib.GetMethod(METHOD__CONTEXT__RESERVE_SLOT);

        // NOTE: CurrentContext is a static property on System.Threading.Thread
        s_pThread_CurrentContext = g_Mscorlib.GetMethod(METHOD__THREAD__GET_CURRENT_CONTEXT);
            
        // *********   NOTE   ************ 
        // This must always be the last statement in this block to prevent races
        // 
        s_fInitializedContext = TRUE;
        // ********* END NOTE ************        
    }

    // Leave the lock 
    LeaveLock();

    LOG((LF_REMOTING, LL_INFO10, "Context::InitializeFields returning %d\n", fReturn));
    return fReturn;
}



// This is called by the managed context constructor
FCIMPL2(void, Context::SetupInternalContext, ContextBaseObject* pThisUNSAFE, BYTE bDefault)
{
    CONTEXTBASEREF pThis = (CONTEXTBASEREF) pThisUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_1(pThis);
    //-[autocvtpro]-------------------------------------------------------


    _ASSERTE(pThis != NULL);
    _ASSERTE(pThis->m_internalContext == NULL);


    // Make sure we have initialized pMT, checked offsets etc.
    InitializeFields();
    Context *pCtx;
    if (bDefault)
    {
        // We have to hook this up with the internal default
        // context for the current appDomain
        pCtx = GetThread()->GetDomain()->GetDefaultContext();
    }
    else
    {
        // Create the unmanaged backing context object
        pCtx = Context::CreateNewContext(GetThread()->GetDomain());
    }


    // Set the managed & unmanaged objects to point at each other.
    pThis->SetInternalContext(pCtx);
    pCtx->SetExposedObject((OBJECTREF)pThis);


    // Set the AppDomain field in the Managed context object
    pThis->SetExposedDomain(
                        GetThread()->GetDomain()->GetExposedObject());

    COUNTER_ONLY(GetPrivatePerfCounters().m_Context.cContexts++);
    COUNTER_ONLY(GetGlobalPerfCounters().m_Context.cContexts++);

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

// This is called by the managed context finalizer
FCIMPL1(void, Context::CleanupInternalContext, ContextBaseObject* pThisUNSAFE)
{
    CONTEXTBASEREF pThis = (CONTEXTBASEREF) pThisUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_1(pThis);
    //-[autocvtpro]-------------------------------------------------------

    _ASSERTE(pThis != NULL);

    CONTEXTBASEREF refCtx = pThis;

    Context *pCtx = refCtx->m_internalContext;
    _ASSERTE(pCtx != NULL);
    if (ValidateContext(pCtx))
    {
        LOG((LF_APPDOMAIN, LL_INFO1000, "Context::CleanupInternalContext: %8.8x, %8.8x\n", OBJECTREFToObject(refCtx), pCtx));
        Context::FreeContext(pCtx);
    }
    COUNTER_ONLY(GetPrivatePerfCounters().m_Context.cContexts--);
    COUNTER_ONLY(GetGlobalPerfCounters().m_Context.cContexts--);

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND


OBJECTREF Context::GetExposedObject()
{
    THROWSCOMPLUSEXCEPTION();
    Thread *pCurThread = GetThread();


    _ASSERTE(pCurThread->PreemptiveGCDisabled());

    if (ObjectFromHandle(m_ExposedObjectHandle) == NULL)
    {
        // Ensure that we have inited the methodTable etc
        InitializeFields();


        // This call should fault in the managed context for the thread
        CONTEXTBASEREF ctx = (CONTEXTBASEREF) 
            ArgSlotToObj(
                MDofManagedThreadCurrentContext()->Call(
                    NULL,
                    METHOD__THREAD__GET_CURRENT_CONTEXT));


        GCPROTECT_BEGIN(ctx);

        // Take a lock to make sure that only one thread creates the object.
        pCurThread->EnablePreemptiveGC();
        // This locking may be too severe!
        EnterLock();       
        pCurThread->DisablePreemptiveGC();

        // Check to see if another thread has not already created the exposed object.
        if (ObjectFromHandle(m_ExposedObjectHandle) == NULL)
        {
            // Keep a weak reference to the exposed object.
            StoreObjectInHandle(m_ExposedObjectHandle, (OBJECTREF) ctx);
            
            ctx->SetInternalContext(this);
        }
        LeaveLock();
        GCPROTECT_END();
        
    }
    return ObjectFromHandle(m_ExposedObjectHandle);
}

// This will NOT create the exposed object if there isn't one!
OBJECTREF Context::GetExposedObjectRaw()
{
    return ObjectFromHandle(m_ExposedObjectHandle);
}

void Context::SetExposedObject(OBJECTREF exposed)
{
    _ASSERTE(exposed != NULL);
    _ASSERTE(ObjectFromHandle(m_ExposedObjectHandle) == NULL);
    StoreObjectInHandle(m_ExposedObjectHandle, exposed);
}

// This is called by EE to transition into a context(possibly in
// another appdomain) and execute the method Context::ExecuteCallBack
// with the private data provided to this method
void Context::RequestCallBack(Context* targetCtxID, void* privateData)
{
    THROWSCOMPLUSEXCEPTION(); 
    _ASSERTE(ValidateContext((Context*)targetCtxID));

    // Ensure that we have inited the methodTable, methodDesc-s etc
    InitializeFields(); 

    // Get the current context of the thread. This is assumed as
    // the context where the request originated
    Context *pCurrCtx = GetCurrentContext();

    // Check that the target context is not the same (presumably the caller has checked for it).
    _ASSERTE(pCurrCtx != targetCtxID);

    // Check if we might be going to a context in another appDomain.
    size_t targetDomainID = 0;
    AppDomain *pCurrDomain = pCurrCtx->GetDomain();
    AppDomain *pTargetDomain = ((Context*)targetCtxID)->GetDomain();
    
    if (pCurrDomain != pTargetDomain)
    {
        targetDomainID = pTargetDomain->GetId();
    }

    ARG_SLOT args[3];
    args[0] = PtrToArgSlot(targetCtxID);
    args[1] = PtrToArgSlot(privateData);
    args[2] = (ARG_SLOT) targetDomainID;
    
    // we need to be co-operative mode for jitting
    Thread* pCurThread = GetThread();
    _ASSERTE(pCurThread);
    BOOL cooperativeGCMode = pCurThread->PreemptiveGCDisabled();
    if (!cooperativeGCMode)
        pCurThread->DisablePreemptiveGC();

    (MDofDoCallBackFromEE()->Call(args, METHOD__CONTEXT__CALLBACK));

    if (!cooperativeGCMode)
        pCurThread->EnablePreemptiveGC();

}

/*** Definitions of callback executions for the various callbacks that are known to EE  ***/

// Callback for waits on waithandle
void Context::ExecuteWaitCallback(WaitArgs* waitArgs)
{

    Thread* pCurThread; 
    pCurThread = GetThread();
    _ASSERTE(pCurThread != NULL);
    // DoAppropriateWait switches to preemptive GC before entering the wait
    *(waitArgs->pResult) = pCurThread->DoAppropriateWait( waitArgs->numWaiters,
                                                          waitArgs->waitHandles,
                                                          waitArgs->waitAll,
                                                          waitArgs->millis,
                                                          waitArgs->alertable);
}

// Callback for monitor wait on objects
void Context::ExecuteMonitorWaitCallback(MonitorWaitArgs* waitArgs)
{
    Thread* pCurThread; 
    pCurThread = GetThread();
    _ASSERTE(pCurThread != NULL);
    BOOL toggleGC = pCurThread->PreemptiveGCDisabled();
    if (toggleGC)
        pCurThread->EnablePreemptiveGC();
    *(waitArgs->pResult) = pCurThread->Block(waitArgs->millis,
                                             waitArgs->syncState);
    if (toggleGC)
        pCurThread->DisablePreemptiveGC();
}

// This is where a call back request made by EE in Context::RequestCallBack
// actually gets "executed". 
// At this point we have done a real context transition from the threads
// context when RequestCallBack was called to the destination context.
FCIMPL1(void, Context::ExecuteCallBack, LPVOID privateData)
{
    HELPER_METHOD_FRAME_BEGIN_0();
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION(); 
    _ASSERTE(privateData);
    

    switch (((CallBackInfo*) privateData)->callbackId)
    {
        case Wait_callback:
            {
                WaitArgs* waitArgs; 
                waitArgs = (WaitArgs*) ((CallBackInfo*) privateData)->callbackData;
                ExecuteWaitCallback(waitArgs);
            }
            break;
        case MonitorWait_callback:
            {
                MonitorWaitArgs* waitArgs; 
                waitArgs = (MonitorWaitArgs*) ((CallBackInfo*) privateData)->callbackData;
                ExecuteMonitorWaitCallback(waitArgs);
            }
            break;
        case ADTransition_callback:
            {
                ADCallBackArgs* pCallArgs = (ADCallBackArgs*)(((CallBackInfo*) privateData)->callbackData); 
                pCallArgs->pTarget(pCallArgs->pArguments);
            }
            break;
        // Add other callback types here
        default:
            _ASSERTE(!"Invalid callback type");
            break;
    }
    // This is EE's entry point to do whatever it wanted to do in
    // the targetContext. This will return back into the managed 
    // world and transition back into the original context.

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

//+----------------------------------------------------------------------------
//
//  Method:     Context::GetStaticFieldAddress   private
//
//  Synopsis:   Get the address of the field relative to the current context.
//              If an address has not been assigned yet then create one.
//
//  History:    15-Feb-2000                       Created
//
//+----------------------------------------------------------------------------
LPVOID Context::GetStaticFieldAddress(FieldDesc *pFD)
{
    THROWSCOMPLUSEXCEPTION();

    BOOL fThrow = FALSE;
    LPVOID pvAddress = NULL;    
    Context *pCtx = NULL;    
    STATIC_DATA *pData;
    MethodTable *pMT = pFD->GetMethodTableOfEnclosingClass();
    BOOL fIsShared = pMT->IsShared();
    WORD wClassOffset = pMT->GetClass()->GetContextStaticOffset();
    WORD currElem = 0; 

    // NOTE: if you change this method, you must also change
    // GetStaticFieldAddrForDebugger below.

    // Retrieve the current context 
    pCtx = GetCurrentContext();
    _ASSERTE(NULL != pCtx);

    _ASSERTE(!s_ContextCrst.OwnedByCurrentThread());

    // Acquire the context lock before accessing the static data pointer
    Thread *t = GetThread();
    BOOL toggleGC = (t && t->PreemptiveGCDisabled());
    if (toggleGC)
        t->EnablePreemptiveGC();
    s_ContextCrst.Enter();
    if (toggleGC)
        t->DisablePreemptiveGC();

    if(!fIsShared)
    {
        pData = pCtx->m_pUnsharedStaticData;
    }
    else
    {
        pData = pCtx->m_pSharedStaticData;
    }
    
    if(NULL != pData)
    {
        currElem = pData->cElem;
    }

    // Check whether we have allocated space for storing a pointer to
    // this class' context static store    
    if(wClassOffset >= currElem)
    {
        // Allocate space for storing pointers 
        WORD wNewElem = (currElem == 0 ? 4 : currElem*2);
        // Ensure that we grow to a size larger than the index we intend to use
        while (wNewElem <= wClassOffset)
        {
            wNewElem = 2*wNewElem;
        }
        //STATIC_DATA *pNew = (STATIC_DATA *)new BYTE[sizeof(STATIC_DATA) + wNewElem*sizeof(LPVOID)]; 

        STATIC_DATA *pNew = (STATIC_DATA *)HeapAlloc(g_hProcessHeap, HEAP_ZERO_MEMORY, sizeof(BYTE)*(sizeof(STATIC_DATA) + wNewElem*sizeof(LPVOID)));

        if (pNew == NULL) FailFast(GetThread(), FatalOutOfMemory);

        pNew->cElem = wNewElem;     // Set the new count.
        if(NULL != pData)
        {
            // Copy the old data into the new data
            memcpy(&pNew->dataPtr[0], &pData->dataPtr[0], currElem*sizeof(LPVOID));
        }
        // Zero init any new elements.
        memset(&pNew->dataPtr[currElem], 0x00, (wNewElem - currElem)* sizeof(LPVOID));

        // Delete the old data
        //delete pData;
        HeapFree(g_hProcessHeap, 0, pData);

        // Update the locals
        pData = pNew;

        // Reset the pointers in the context object to point to the 
        // new memory
        if(!fIsShared)
        {
            pCtx->m_pUnsharedStaticData = pData;
        }
        else
        {
            pCtx->m_pSharedStaticData = pData;
        }            
    }
    
    // Check whether we have to allocate space for 
    // the context local statics of this class
    if(NULL == pData->dataPtr[wClassOffset])
    {
        // Allocate memory for context static fields
        //pData->dataPtr[wClassOffset] = (LPVOID)new BYTE[pMT->GetClass()->GetContextLocalStaticsSize()];
        pData->dataPtr[wClassOffset] = HeapAlloc(g_hProcessHeap, HEAP_ZERO_MEMORY, sizeof(BYTE)*(pMT->GetClass()->GetContextLocalStaticsSize()));
        if (pData->dataPtr[wClassOffset] == NULL) FailFast(GetThread(), FatalOutOfMemory);

        // Initialize the memory allocated for the fields
        memset(pData->dataPtr[wClassOffset], 0x00, pMT->GetClass()->GetContextLocalStaticsSize());
    }
    
    _ASSERTE(NULL != pData->dataPtr[wClassOffset]);
    // We have allocated static storage for this data
    // Just return the address by getting the offset into the data
    pvAddress = (LPVOID)((LPBYTE)pData->dataPtr[wClassOffset] + pFD->GetOffset());

    // For object and value class fields we have to allocate storage in the
    // __StaticContainer class in the managed heap
    if(pFD->IsObjRef() || pFD->IsByValue())
    {
        // Ensure that we have inited the methodDesc for ReserveSlot etc
        InitializeFields();
        // in this case *pvAddress == bucket|index
        int *pSlot = (int*)pvAddress;
        pvAddress = NULL;
        fThrow = GetStaticFieldAddressSpecial(pFD, pMT, pSlot, &pvAddress);

        if (pFD->IsByValue())
        {
            _ASSERTE(pvAddress != NULL);
            pvAddress = (*((OBJECTREF*)pvAddress))->GetData();
        }
        // ************************************************
        // ************** WARNING *************************
        // Do not provoke GC from here to the point JIT gets
        // pvAddress back
        // ************************************************
        _ASSERTE(*pSlot > 0);
    }
    
    s_ContextCrst.Leave();

    // Check if we have to throw an exception
    if(fThrow)
    {
        COMPlusThrowOM();
    }
    _ASSERTE(NULL != pvAddress);
    _ASSERTE(!s_ContextCrst.OwnedByCurrentThread());

    return pvAddress;
}



//+----------------------------------------------------------------------------
//       
//  Method:     Context::GetStaticFieldAddrForDebugger   private
//
//  Synopsis:   Get the address of the field relative to the context given a thread. 
//              If an address has not been assigned, return NULL.
//              No creating is allowed.
//
//  History:    04-May-2001                         Created
//
//+----------------------------------------------------------------------------
LPVOID Context::GetStaticFieldAddrForDebugger(Thread *pTH, FieldDesc *pFD)
{    
    LPVOID pvAddress = NULL;    
    Context *pCtx = NULL;    
    STATIC_DATA *pData;
    MethodTable *pMT = pFD->GetMethodTableOfEnclosingClass();
    BOOL fIsShared = pMT->IsShared();
    WORD wClassOffset = pMT->GetClass()->GetContextStaticOffset();
    WORD currElem = 0; 

    // Retrieve the context with a given thread 
    pCtx = pTH->GetContext();
    _ASSERTE(NULL != pCtx);

    if(!fIsShared)
    {
        pData = pCtx->m_pUnsharedStaticData;
    }
    else
    {
        pData = pCtx->m_pSharedStaticData;
    }
    
    if(NULL != pData)
    {
        currElem = pData->cElem;
    }

    // Check whether we have allocated space for storing a pointer to
    // this class' context static store    
    if(wClassOffset >= currElem || NULL == pData->dataPtr[wClassOffset])
    {
        return NULL;
    }
    
    _ASSERTE(NULL != pData->dataPtr[wClassOffset]);

    // We have allocated static storage for this data
    // Just return the address by getting the offset into the data
    pvAddress = (LPVOID)((LPBYTE)pData->dataPtr[wClassOffset] + pFD->GetOffset());

    if(pFD->IsObjRef() || pFD->IsByValue())
    {
        // If Context is not initialized, just return NULL.
        if (!s_fInitializedContext)
            return NULL;

        if (NULL == *(LPVOID *)pvAddress)
        {
            pvAddress = NULL;
            LOG((LF_SYNC, LL_ALWAYS, "dbgr: pvAddress = NULL"));
        }
        else
        {
            pvAddress = CalculateAddressForManagedStatic(*(int*)pvAddress, pCtx);
            LOG((LF_SYNC, LL_ALWAYS, "dbgr: pvAddress = %lx", pvAddress));
            if (pFD->IsByValue())
            {
                _ASSERTE(pvAddress != NULL);
                pvAddress = (*((OBJECTREF*)pvAddress))->GetData();
            }
        }
    }

    return pvAddress;
}

//+----------------------------------------------------------------------------
//
//  Method:     Context::AllocateStaticFieldObjRefPtrs   private
//
//  Synopsis:   Allocate an entry in the __StaticContainer class in the
//              managed heap for static objects and value classes
//
//  History:    28-Feb-2000                       Created
//
//+----------------------------------------------------------------------------
BOOL Context::AllocateStaticFieldObjRefPtrs(FieldDesc *pFD, MethodTable *pMT, LPVOID pvAddress)
{
    BOOL fThrow = FALSE;

    BEGINCANNOTTHROWCOMPLUSEXCEPTION();    

    // Retrieve the object ref pointers from the app domain.
    OBJECTREF *pObjRef = NULL;

    COMPLUS_TRY 
    {
        // Reserve some object ref pointers.
        GetAppDomain()->AllocateStaticFieldObjRefPtrs(1, &pObjRef);


        // to a boxed version of the value class.  This allows the standard GC
        // algorithm to take care of internal pointers in the value class.
        if (pFD->IsByValue())
        {
    
            // Extract the type of the field
            TypeHandle  th;        
            PCCOR_SIGNATURE pSig;
            DWORD       cSig;
            pFD->GetSig(&pSig, &cSig);
            FieldSig sig(pSig, pFD->GetModule());

            OBJECTREF throwable = NULL;
            GCPROTECT_BEGIN(throwable);
            th = sig.GetTypeHandle(&throwable);
            if (throwable != NULL)
                COMPlusThrow(throwable);
            GCPROTECT_END();

            OBJECTREF obj = AllocateObject(th.AsClass()->GetMethodTable());
            SetObjectReference( pObjRef, obj, GetAppDomain() );                      
        }

        *(ULONG_PTR *)pvAddress =  (ULONG_PTR)pObjRef;
    } 
    COMPLUS_CATCH
    {
        fThrow = TRUE;
    }            
    COMPLUS_END_CATCH

    ENDCANNOTTHROWCOMPLUSEXCEPTION();

    return fThrow;
}


// This is used for context relative statics that are object refs 
// These are stored in a structure in the managed context. The first
// time over an index and a bucket are determined and subsequently 
// remembered in the location for the field in the per-context-per-class
// data structure.
// Here we map back from the index to the address of the object ref.
LPVOID Context::CalculateAddressForManagedStatic(int slot, Context *pContext)
{
    BEGINFORBIDGC();
    OBJECTREF *pObjRef;
    int bucket = (slot>>16);
    int index = (0x0000ffff&slot);
    // Now determine the address of the static field 
    PTRARRAYREF bucketRef = NULL;

    if (pContext == NULL)
        pContext = GetCurrentContext();

    _ASSERTE(pContext);

    bucketRef = ((CONTEXTBASEREF)pContext->GetExposedObjectRaw())->GetContextStaticsHolder();
    // walk the chain to our bucket
    while (bucket--)
    {
        bucketRef = (PTRARRAYREF) bucketRef->GetAt(0);
    }
    // Index 0 is used to point to the next bucket!
    _ASSERTE(index > 0);
    pObjRef = ((OBJECTREF*)bucketRef->GetDataPtr())+index;
    ENDFORBIDGC();
    return (LPVOID) pObjRef;
}

//+----------------------------------------------------------------------------
//
//  Method:     Context::GetStaticFieldAddressSpecial private
//
//  Synopsis:   Allocate an entry in the __StaticContainer class in the
//              managed heap for static objects and value classes
//
//  History:    28-Feb-2000                       Created
//
//+----------------------------------------------------------------------------

// NOTE: At one point we used to allocate these in the long lived handle table
// which is per-appdomain. However, that causes them to get rooted and not 
// cleaned up until the appdomain gets unloaded. This is not very desirable 
// since a context static object may hold a reference to the context itself or
// to a proxy in the context causing a whole lot of garbage to float around.
// Now (2/13/01) these are allocated from a managed structure rooted in each
// managed context.
BOOL Context::GetStaticFieldAddressSpecial(
    FieldDesc *pFD, MethodTable *pMT, int *pSlot, LPVOID *ppvAddress)
{
    BOOL fThrow = FALSE;

    BEGINCANNOTTHROWCOMPLUSEXCEPTION();    

    COMPLUS_TRY 
    {
        OBJECTREF *pObjRef = NULL;
        BOOL bNewSlot = (*pSlot == 0);
        if (bNewSlot)
        {
            // ! this line will trigger a GC, don't move it down
            // ! without protecting the args[] and other OBJECTREFS
            MethodDesc * pMD = MDofReserveSlot();
            
            // We need to assign a location for this static field. 
            // Call the managed helper
            ARG_SLOT args[1] = {
                ObjToArgSlot(GetCurrentContext()->GetExposedObject())
            };
            
            // The managed ReserveSlot methods counts on this!
            _ASSERTE(s_ContextCrst.OwnedByCurrentThread());

            _ASSERTE(args[0] != 0);

            *pSlot = (int) pMD->Call(args, METHOD__CONTEXT__RESERVE_SLOT);
        
            _ASSERTE(*pSlot>0);
        

            // to a boxed version of the value class.This allows the standard GC
            // algorithm to take care of internal pointers in the value class.
            if (pFD->IsByValue())
            {
                // Extract the type of the field
                TypeHandle  th;        
                PCCOR_SIGNATURE pSig;
                DWORD       cSig;
                pFD->GetSig(&pSig, &cSig);
                FieldSig sig(pSig, pFD->GetModule());

                OBJECTREF throwable = NULL;
                GCPROTECT_BEGIN(throwable);
                th = sig.GetTypeHandle(&throwable);
                if (throwable != NULL)
                    COMPlusThrow(throwable);
                GCPROTECT_END();

                OBJECTREF obj = AllocateObject(th.AsClass()->GetMethodTable());
                pObjRef = (OBJECTREF*)CalculateAddressForManagedStatic(*pSlot);
                SetObjectReference( pObjRef, obj, GetAppDomain() );
            }
            else
            {
                pObjRef = (OBJECTREF*)CalculateAddressForManagedStatic(*pSlot);
            }
        }
        else
        {
            // If the field already has a location assigned we go through here
            pObjRef = (OBJECTREF*)CalculateAddressForManagedStatic(*pSlot);
        }
        *(ULONG_PTR *)ppvAddress =  (ULONG_PTR)pObjRef;
    } 
    COMPLUS_CATCH
    {
        fThrow = TRUE;
    }            
    COMPLUS_END_CATCH

    ENDCANNOTTHROWCOMPLUSEXCEPTION();

    return fThrow;
}

#ifdef ENABLE_PERF_COUNTERS

FCIMPL0(LPVOID, GetPrivateContextsPerfCountersEx)
    return (LPVOID)GetPrivateContextsPerfCounters();
}

FCIMPL0(LPVOID, GetGlobalContextsPerfCountersEx)
    return (LPVOID)GetGlobalContextsPerfCounters();
}

#else
FCIMPL0(LPVOID, GetPrivateContextsPerfCountersEx)
    return NULL;
}

FCIMPL0(LPVOID, GetGlobalContextsPerfCountersEx)
    return NULL;
}
#endif

