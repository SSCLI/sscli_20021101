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

/*===========================================================================
**
** File:    remoting.cpp
**        
** Purpose: Defines various remoting related objects such as
**          proxies
**
** Date:    Feb 16, 1999
**
=============================================================================*/
#include "common.h"
#include "excep.h"
#include "comstring.h"
#include "comdelegate.h"
#include "remoting.h"
#include "reflectwrap.h"
#include "field.h"
#include "siginfo.hpp"
#include "comclass.h"
#include "stackbuildersink.h"
#include "eehash.h"
#include "wsperf.h"
#include "profilepriv.h"
#include "message.h"
#include "eeconfig.h"
#include "comcallwrapper.h"
#include "interopconverter.h"
#include "asmconstants.h"

// Macros

#define IDS_REMOTING_LOCK           "Remoting Services" // Remoting services lock
#define IDS_TPMETHODTABLE_LOCK      "TP Method Table"   // Transparent Proxy Method table



// These hold label offsets into non-virtual thunks. They are used by
// CNonVirtualThunkMgr::DoTraceStub and ::TraceManager to help the
// debugger figure out where the thunk is going to go.
DWORD g_dwNonVirtualThunkRemotingLabelOffset = 0;
DWORD g_dwNonVirtualThunkReCheckLabelOffset = 0;

// Statics

MethodTable *CRemotingServices::s_pMarshalByRefObjectClass;    
MethodTable *CRemotingServices::s_pServerIdentityClass;
MethodTable *CRemotingServices::s_pProxyAttributeClass;
MethodTable *CRemotingServices::s_pContextClass;

MethodDesc *CRemotingServices::s_pRPPrivateInvoke;
MethodDesc *CRemotingServices::s_pRPInvokeStatic;
MethodDesc *CRemotingServices::s_pWrapMethodDesc;
MethodDesc *CRemotingServices::s_pIsCurrentContextOK;
MethodDesc *CRemotingServices::s_pCheckCast;
MethodDesc *CRemotingServices::s_pFieldSetterDesc;
MethodDesc *CRemotingServices::s_pFieldGetterDesc;
MethodDesc *CRemotingServices::s_pGetTypeDesc;
MethodDesc *CRemotingServices::s_pProxyForDomainDesc;
MethodDesc *CRemotingServices::s_pServerContextForProxyDesc;
MethodDesc *CRemotingServices::s_pServerDomainIdForProxyDesc;


DWORD CRemotingServices::s_dwTPOffset;
DWORD CRemotingServices::s_dwServerOffsetInRealProxy;
DWORD CRemotingServices::s_dwIdOffset;
DWORD CRemotingServices::s_dwFlagsOffset;
DWORD CRemotingServices::s_dwServerCtxOffset;
DWORD CRemotingServices::s_dwTPOrObjOffsetInIdentity;
DWORD CRemotingServices::s_dwMBRIDOffset;
CrstStatic CRemotingServices::s_RemotingCrst;
BOOL CRemotingServices::s_fInitializedRemoting;


#ifdef REMOTING_PERF
HANDLE CRemotingServices::s_hTimingData = NULL;
#endif

// CTPMethodTable Statics
DWORD CTPMethodTable::s_cRefs;
DWORD CTPMethodTable::s_dwCommitedTPSlots;
DWORD CTPMethodTable::s_dwReservedTPSlots;
MethodTable *CTPMethodTable::s_pThunkTable;
EEClass *CTPMethodTable::s_pTransparentProxyClass;
DWORD CTPMethodTable::s_dwGCInfoBytes;
DWORD CTPMethodTable::s_dwMTDataSlots;
DWORD CTPMethodTable::s_dwRPOffset;
DWORD CTPMethodTable::s_dwMTOffset;
DWORD CTPMethodTable::s_dwItfMTOffset;
DWORD CTPMethodTable::s_dwStubOffset;
DWORD CTPMethodTable::s_dwStubDataOffset;
DWORD CTPMethodTable::s_dwMaxSlots;
MethodTable *CTPMethodTable::s_pTPMT;
MethodTable *CTPMethodTable::s_pRemotingProxyClass;
Stub *CTPMethodTable::s_pTPStub;
Stub *CTPMethodTable::s_pDelegateStub;
CRITICAL_SECTION CTPMethodTable::s_TPMethodTableCrst;
EEThunkHashTable *CTPMethodTable::s_pThunkHashTable;
BOOL CTPMethodTable::s_fInitializedTPTable;

// CVirtualThunks statics
CVirtualThunks *CVirtualThunks::s_pVirtualThunks;

// CVirtualThunkMgr statics                                                     
CVirtualThunkMgr *CVirtualThunkMgr::s_pVirtualThunkMgr;

// CNonVirtualThunk statics
CNonVirtualThunk *CNonVirtualThunk::s_pNonVirtualThunks;

// CNonVirtualThunkMgr statics
CNonVirtualThunkMgr *CNonVirtualThunkMgr::s_pNonVirtualThunkMgr;


//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::Initialize    public
//
//  Synopsis:   Initialized remoting state
//+----------------------------------------------------------------------------
BOOL CRemotingServices::Initialize()
{
    s_pRPPrivateInvoke = NULL;
    s_pRPInvokeStatic = NULL;
    s_dwTPOffset = NULL;
    s_fInitializedRemoting = FALSE;

    // Initialize the remoting services critical section
    s_RemotingCrst.Init(IDS_REMOTING_LOCK,CrstRemoting,TRUE,FALSE);

    return CTPMethodTable::Initialize();
}

#ifdef REMOTING_PERF
//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::LogRemotingStage    public
//
//  Synopsis:   Records timing data for a particular stage in a call
//
//+----------------------------------------------------------------------------
FCIMPL1(VOID, CRemotingServices::LogRemotingStage, INT32 stage)

    LogRemotingStageInner(stage);

FCIMPLEND

VOID CRemotingServices::LogRemotingStageInner(INT32 stage)
{
    if (s_hTimingData == NULL)
        return;

    struct timingData td;
    LARGE_INTEGER   cycles;
    DWORD written = 0;

    td.threadId = GetCurrentThreadId();
    td.stage = stage;
    QueryPerformanceCounter(&cycles);
    td.cycleCount = cycles.QuadPart;

#ifdef _DEBUG
    BOOL result =
#endif
    WriteFile(s_hTimingData, &td, 
                            sizeof(struct timingData), &written, NULL);
    _ASSERTE(result && written == sizeof(struct timingData));
}

void CRemotingServices::OpenLogFile()
{
    if (g_pConfig->LogRemotingPerf() > 0)
    {
        WCHAR path[MAX_PATH];
        DWORD len = WszGetModuleFileName(NULL, &path[0], MAX_PATH);
        if (len) {
            WCHAR *period = wcsrchr(&path[0], '.');
            wcscpy(period + 1, L"dat");
            s_hTimingData = WszCreateFile(&path[0], GENERIC_WRITE, FILE_SHARE_READ, NULL, 
                                          OPEN_ALWAYS, 
                                          FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                                          NULL);
            _ASSERTE(s_hTimingData != INVALID_HANDLE_VALUE);
        }
    }
}

void CRemotingServices::CloseLogFile()
{
    if (s_hTimingData)
    {
        LogRemotingStageInner(TIMING_DATA_EOF);
        CloseHandle(s_hTimingData);
        s_hTimingData = NULL;
    }

}
#endif // REMOTING_PERF

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::CleanUp    public
//
//  Synopsis:   Cleansup remoting state
//+----------------------------------------------------------------------------


//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::IsTransparentProxy    public
//
//  Synopsis:   Check whether the supplied object is proxy or not. This 
//              represents the overloaded method that takes an object.
//              
//+----------------------------------------------------------------------------
FCIMPL1(INT32, CRemotingServices::IsTransparentProxy, Object* orTP)
{
    INT32 fIsTPMT = FALSE;

    if(orTP != NULL)
    {
        // Check if the supplied object has transparent proxy method table
        MethodTable *pMT = orTP->GetMethodTable();
        fIsTPMT = pMT->IsTransparentProxyType() ? TRUE : FALSE;
    }

    LOG((LF_REMOTING, LL_INFO1000, "!IsTransparentProxy(0x%x) returning %s",
         orTP, fIsTPMT ? "TRUE" : "FALSE"));

    return(fIsTPMT);
}
FCIMPLEND

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::IsTransparentProxyEx    public
//
//  Synopsis:   Check whether the supplied object is proxy or not. This 
//              represents the overloaded method which takes a contextbound 
//              object
//              
//+----------------------------------------------------------------------------
FCIMPL1(INT32, CRemotingServices::IsTransparentProxyEx, Object* orTP)
{
    INT32 fIsTPMT = FALSE;

    if(orTP != NULL)
    {
        // Check if the supplied object has transparent proxy method table
        MethodTable *pMT = orTP->GetMethodTable();
        fIsTPMT = pMT->IsTransparentProxyType() ? TRUE : FALSE;
    }

    LOG((LF_REMOTING, LL_INFO1000, "!IsTransparentProxyEx(0x%x) returning %s",
         orTP, fIsTPMT ? "TRUE" : "FALSE"));

    return(fIsTPMT);
}
FCIMPLEND

// Called from RemotingServices::ConfigureRemoting to remember that
// a config file has been parsed.
FCIMPL0(VOID, CRemotingServices::SetRemotingConfiguredFlag)
    // Mark a flag for the current appDomain to remember the fact
    // that ConfigureRemoting has been called.
    GetThread()->GetDomain()->SetRemotingConfigured();
FCIMPLEND


//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::GetRealProxy    public
//
//  Synopsis:   Returns the real proxy backing the transparent
//              proxy
//+----------------------------------------------------------------------------
FCIMPL1(Object*, CRemotingServices::GetRealProxy, Object* objTP)
{   
    // Check if the supplied object has transparent proxy method table
    Object* rv = NULL;
    if ((NULL != objTP) && IsTransparentProxy(objTP))
    {
        // RemotingServices should have already been initialized by now
        _ASSERTE(s_fInitializedRemoting);
        rv = OBJECTREFToObject(CTPMethodTable::GetRP(OBJECTREF(objTP)));
    }

    LOG((LF_REMOTING, LL_INFO100, "!GetRealProxy(0x%x) returning 0x%x\n", objTP, rv));
    return(rv);
}
FCIMPLEND

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::CreateTransparentProxy    public
//
//  Synopsis:   Creates a new transparent proxy for the supplied real
//              proxy
//+----------------------------------------------------------------------------
FCIMPL4(Object*, CRemotingServices::CreateTransparentProxy, Object* orRPUNSAFE, ReflectClassBaseObject* pClassToProxyUNSAFE, LPVOID pStub, Object* orStubDataUNSAFE)
{
    OBJECTREF pTP = NULL;
    struct _gc
    {
        OBJECTREF orRP;
        REFLECTCLASSBASEREF pClassToProxy;
        OBJECTREF orStubData;
    } gc;
    gc.orRP = (OBJECTREF) orRPUNSAFE;
    gc.pClassToProxy = (REFLECTCLASSBASEREF) pClassToProxyUNSAFE;
    gc.orStubData = (OBJECTREF) orStubDataUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_NOPOLL(Frame::FRAME_ATTR_RETURNOBJ);
    GCPROTECT_BEGIN(gc);
    HELPER_METHOD_POLL();
    //-[autocvtpro]-------------------------------------------------------

    // Sanity check
    THROWSCOMPLUSEXCEPTION();

    // Ensure that the fields of remoting service have been initialized
    // This is separated from the initialization of the remoting services
    if (!s_fInitializedRemoting)
    {
        if (!InitializeFields())
        {
            _ASSERTE(!"Initialization Failed");
            FATAL_EE_ERROR();
        }
    }
    // Check if the supplied object has a transparent proxy already
    if (((DWORD_PTR)gc.orRP->GetPtrOffset(s_dwTPOffset)) != NULL)
        COMPlusThrow(kArgumentException, L"Remoting_TP_NonNull");

    // Create a tranparent proxy that behaves as an object of the desired class
    ReflectClass *pRefClass = (ReflectClass *) gc.pClassToProxy->GetData();
    EEClass *pEEClass = pRefClass->GetClass();
    pTP = CTPMethodTable::CreateTPOfClassForRP(pEEClass, gc.orRP);
    
    // Set the stub pointer
    pTP->SetOffsetPtr(CTPMethodTable::GetOffsetOfStub(), pStub);

    // Set the stub data
    pTP->SetOffsetObjectRef(CTPMethodTable::GetOffsetOfStubData(), (size_t)OBJECTREFToObject(gc.orStubData));

    COUNTER_ONLY(GetPrivatePerfCounters().m_Context.cProxies++);
    COUNTER_ONLY(GetGlobalPerfCounters().m_Context.cProxies++);
    
    LOG((LF_REMOTING, LL_INFO100, "CreateTransparentProxy returning 0x%x\n", OBJECTREFToObject(pTP)));

    //-[autocvtepi]-------------------------------------------------------
    GCPROTECT_END();
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(pTP);
}
FCIMPLEND

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::_InitializeRemoting    private
//
//  Synopsis:   Initialize the static fields of CTPMethodTable class
//
//+----------------------------------------------------------------------------

BOOL CRemotingServices::_InitializeRemoting()
{
    BOOL fReturn = TRUE;
    if (!CRemotingServices::s_fInitializedRemoting)
    {
        fReturn = CRemotingServices::InitializeFields();
        if (fReturn && !CTPMethodTable::s_fInitializedTPTable)
        {
            fReturn = CTPMethodTable::InitializeFields();
        }
    }
    return fReturn;
}

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::InitializeFields    private
//
//  Synopsis:   Initialize the static fields of CRemotingServices class
//
//+----------------------------------------------------------------------------
BOOL CRemotingServices::InitializeFields()
{
    BOOL fReturn = TRUE;

    // Acquire the remoting lock before initializing fields
    Thread *t = GetThread();
    BOOL toggleGC = (t && t->PreemptiveGCDisabled());
    if (toggleGC)
        t->EnablePreemptiveGC();
    s_RemotingCrst.Enter();
    if (toggleGC)
        t->DisablePreemptiveGC();

    // Make sure that no other thread has initialized the fields
    if (!s_fInitializedRemoting)
    {
        if(!InitActivationServicesClass())
        {
            goto ErrExit;
        }
        
        if(!InitRealProxyClass())
        {
            goto ErrExit;
        }

        if(!InitRemotingProxyClass())
        {
            goto ErrExit;
        }

        if(!InitIdentityClass())
        {
            goto ErrExit;
        }
        
        if(!InitServerIdentityClass())
        {
            goto ErrExit;
        }

        if(!InitContextBoundObjectClass())
        {
            goto ErrExit;        
        }

        if(!InitContextClass())
        {
            goto ErrExit;
        }

        if(!InitMarshalByRefObjectClass())
        {
            goto ErrExit;
        }

        if(!InitRemotingServicesClass())
        {
            goto ErrExit;
        }

        if(!InitProxyAttributeClass())
        {
            goto ErrExit;
        }

        if(!InitObjectClass())
        {
            goto ErrExit;
        }


        // *********   NOTE   ************ 
        // This must always be the last statement in this block to prevent races
        // 
        s_fInitializedRemoting = TRUE;
        // ********* END NOTE ************        
    }

ErrExit:
    // Leave the remoting lock
    s_RemotingCrst.Leave();

    LOG((LF_REMOTING, LL_INFO10, "InitializeFields returning %d\n", fReturn));
    return fReturn;
}

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::InitActivationServicesClass    private
//
//  Synopsis:   Extract the method descriptors and fields of ActivationServices class
//
//+----------------------------------------------------------------------------
BOOL CRemotingServices::InitActivationServicesClass()
{
    BOOL fReturn = TRUE;

    s_pIsCurrentContextOK = g_Mscorlib.GetMethod(METHOD__ACTIVATION_SERVICES__IS_CURRENT_CONTEXT_OK);

    LOG((LF_REMOTING, LL_INFO10, "InitRealProxyClass returning %d\n", fReturn));
    return fReturn;
}

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::InitRealProxyClass    private
//
//  Synopsis:   Extract the method descriptors and fields of Real Proxy class
//
//+----------------------------------------------------------------------------
BOOL CRemotingServices::InitRealProxyClass()
{
    BOOL fReturn = TRUE;

    // Now store the methoddesc of the PrivateInvoke method on the RealProxy class
    s_pRPPrivateInvoke = g_Mscorlib.GetMethod(METHOD__REAL_PROXY__PRIVATE_INVOKE);

        // Now find the offset to the _tp field inside the RealProxy class
    s_dwTPOffset = g_Mscorlib.GetFieldOffset(FIELD__REAL_PROXY__TP);
            _ASSERTE(s_dwTPOffset == 0);

        // Now find the offset to the _identity field inside the 
        // RealProxy  class
    s_dwIdOffset = g_Mscorlib.GetFieldOffset(FIELD__REAL_PROXY__IDENTITY);

    s_dwServerOffsetInRealProxy = 
            g_Mscorlib.GetFieldOffset(FIELD__REAL_PROXY__SERVER);

    LOG((LF_REMOTING, LL_INFO10, "InitRealProxyClass returning %d\n", fReturn));
    return fReturn;
}

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::InitRemotingProxyClass    private
//
//  Synopsis:   Extract the method descriptors and fields of RemotingProxy class
//
//+----------------------------------------------------------------------------
BOOL CRemotingServices::InitRemotingProxyClass()
{
    BOOL fReturn = TRUE;

    s_dwFlagsOffset = g_Mscorlib.GetFieldOffset(FIELD__REMOTING_PROXY__FLAGS);
    s_pRPInvokeStatic = g_Mscorlib.GetMethod(METHOD__REMOTING_PROXY__INVOKE);

    // Note: We cannot do this inside TPMethodTable::InitializeFields ..
    // that causes recursions if in some situation only the latter is called
    // If you do this you will see Asserts when running any process under CorDbg
    // This is because jitting of NV methods on MBR objects calls 
    // InitializeFields and when actually doing that we should not need to
    // JIT another NV method on some MBR object.
    CTPMethodTable::s_pRemotingProxyClass = g_Mscorlib.GetClass(CLASS__REMOTING_PROXY);
    _ASSERTE(CTPMethodTable::s_pRemotingProxyClass);

    LOG((LF_REMOTING, LL_INFO10, "InitRemotingProxyClass returning %d\n", fReturn));
    return fReturn;
}

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::InitServerIdentityClass    private
//
//  Synopsis:   Extract the method descriptors and fields of ServerIdentity class
//
//+----------------------------------------------------------------------------
BOOL CRemotingServices::InitServerIdentityClass()
{
    BOOL fReturn = TRUE;

    s_pServerIdentityClass = g_Mscorlib.GetClass(CLASS__SERVER_IDENTITY);

    s_dwServerCtxOffset = g_Mscorlib.GetFieldOffset(FIELD__SERVER_IDENTITY__SERVER_CONTEXT);

    LOG((LF_REMOTING, LL_INFO10, "InitServerIdentityClass returning %d\n", fReturn));
    return fReturn;
}

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::InitIdentityClass    private
//
//  Synopsis:   Extract the method descriptors and fields of Identity class
//
//+----------------------------------------------------------------------------
BOOL CRemotingServices::InitIdentityClass()
{
    BOOL fReturn = TRUE;

    s_dwTPOrObjOffsetInIdentity = g_Mscorlib.GetFieldOffset(FIELD__IDENTITY__TP_OR_OBJECT);

    LOG((LF_REMOTING, LL_INFO10, "InitIdentityClass returning %d\n", fReturn));
    return fReturn;
}

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::InitContextBoundObjectClass    private
//
//  Synopsis:   Extract the method descriptors and fields of ContextBoundObject class
//
//+----------------------------------------------------------------------------
BOOL CRemotingServices::InitContextBoundObjectClass()
{
    BOOL fReturn = TRUE;

    LOG((LF_REMOTING, LL_INFO10, "InitContextBoundObjectClass returning %d\n", fReturn));
    return fReturn;
}


//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::InitContextClass    private
//
//  Synopsis:   Extract the method descriptors and fields of Contexts class
//
//+----------------------------------------------------------------------------
BOOL CRemotingServices::InitContextClass()
{
    BOOL fReturn = TRUE;
    // Note reliance on LoadClass being an idempotent operation

    s_pContextClass = g_Mscorlib.GetClass(CLASS__CONTEXT);

    LOG((LF_REMOTING, LL_INFO10, "InitContextClass returning %d\n", fReturn));
    return fReturn;
}

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::InitMarshalByRefObjectClass    private
//
//  Synopsis:   Extract the method descriptors and fields of MarshalByRefObject class
//
//+----------------------------------------------------------------------------
BOOL CRemotingServices::InitMarshalByRefObjectClass()
{
    BOOL fReturn = TRUE;

    s_pMarshalByRefObjectClass = g_Mscorlib.GetClass(CLASS__MARSHAL_BY_REF_OBJECT);
    s_dwMBRIDOffset = g_Mscorlib.GetFieldOffset(FIELD__MARSHAL_BY_REF_OBJECT__IDENTITY);

    LOG((LF_REMOTING, LL_INFO10, "InitMarshalByRefObjectClass returning %d\n", fReturn));
    return fReturn;
}

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::InitRemotingServicesClass    private
//
//  Synopsis:   Extract the method descriptors and fields of RemotingServices class
//
//+----------------------------------------------------------------------------
BOOL CRemotingServices::InitRemotingServicesClass()
{
    BOOL fReturn = TRUE;

    s_pCheckCast = g_Mscorlib.GetMethod(METHOD__REMOTING_SERVICES__CHECK_CAST);

        // Need these to call wrap/unwrap from the VM (message.cpp).
        // Also used by JIT helpers to wrap/unwrap
    s_pWrapMethodDesc = g_Mscorlib.GetMethod(METHOD__REMOTING_SERVICES__WRAP);
    s_pProxyForDomainDesc = g_Mscorlib.GetMethod(METHOD__REMOTING_SERVICES__CREATE_PROXY_FOR_DOMAIN);
    s_pServerContextForProxyDesc = g_Mscorlib.GetMethod(METHOD__REMOTING_SERVICES__GET_SERVER_CONTEXT_FOR_PROXY);
    s_pServerDomainIdForProxyDesc = g_Mscorlib.GetMethod(METHOD__REMOTING_SERVICES__GET_SERVER_DOMAIN_ID_FOR_PROXY);
    s_pGetTypeDesc = g_Mscorlib.GetMethod(METHOD__REMOTING_SERVICES__GET_TYPE);


    LOG((LF_REMOTING, LL_INFO10, "InitRemotingServicesClass returning %d\n", fReturn));
    return fReturn;
}




//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::InitProxyAttributeClass    private
//
//  Synopsis:   Cache the ProxyAttribute class method table
//
//+----------------------------------------------------------------------------
BOOL CRemotingServices::InitProxyAttributeClass()
{
    if (s_pProxyAttributeClass == NULL)
    {
        s_pProxyAttributeClass = g_Mscorlib.GetClass(CLASS__PROXY_ATTRIBUTE);
    }
    return s_pProxyAttributeClass != NULL;
}

MethodTable *CRemotingServices::GetProxyAttributeClass()
{
    InitProxyAttributeClass();
    _ASSERTE(s_pProxyAttributeClass != NULL);
    return s_pProxyAttributeClass;
}


//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::InitObjectClass    private
//
//  Synopsis:   Extract the method descriptors and fields of Object class
//
//+----------------------------------------------------------------------------
BOOL CRemotingServices::InitObjectClass()
{
    BOOL fReturn = TRUE;

    s_pFieldSetterDesc = g_Mscorlib.GetMethod(METHOD__OBJECT__FIELD_SETTER);
    s_pFieldGetterDesc = g_Mscorlib.GetMethod(METHOD__OBJECT__FIELD_GETTER);

    LOG((LF_REMOTING, LL_INFO10, "InitObjectClass returning %d\n", fReturn));
    return fReturn;
}


//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::GetInternalHashCode    public
//
//  Synopsis:   Returns sync block index for use in hash tables
//              
//+----------------------------------------------------------------------------
FCIMPL1(INT32, CRemotingServices::GetInternalHashCode, Object* orObjUNSAFE)
{
    DWORD idx;
    OBJECTREF orObj = (OBJECTREF) orObjUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_1(orObj);
    //-[autocvtpro]-------------------------------------------------------

    idx = (orObj)->GetSyncBlockIndex();    // succeeds or throws

    _ASSERTE(idx != 0);

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return (INT32) idx;
}
FCIMPLEND

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::IsRemoteActivationRequired    public
//
//  Synopsis:   Determines whether we can activate in the current context.  
//              If not, then we will end up creating the object in a different 
//              context/appdomain/process/machine and so on...
//              This is used to provide the appropriate activator to JIT
//              (if we return true here ... JIT_NewCrossContext will get called
//              when the "new" executes)
//
//
//  Note:       Called by getNewHelper
//+----------------------------------------------------------------------------
BOOL CRemotingServices::IsRemoteActivationRequired(EEClass* pClass)
{   
    BEGINFORBIDGC();

    _ASSERTE(!pClass->IsThunking());
    
    BOOL fRequiresNewContext = pClass->IsMarshaledByRef();

    // Contextful classes imply marshal by ref but not vice versa
    _ASSERTE(!fRequiresNewContext || 
             !(pClass->IsContextful() && !pClass->IsMarshaledByRef()));

    LOG((LF_REMOTING, LL_INFO1000, "IsRemoteActivationRequired returning %d\n", fRequiresNewContext));

    ENDFORBIDGC();

    return fRequiresNewContext; 
}

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::RequiresManagedActivation    private
//
//  Synopsis:   Determine if a config file has been parsed or if there
//              are any attributes on the class that would require us
//              to go into the managed activation codepath.
//              
//
//  Note:       Called by CreateProxyOrObject (JIT_NewCrossContext)
//+----------------------------------------------------------------------------
ManagedActivationType __stdcall CRemotingServices::RequiresManagedActivation(EEClass* pClass)
{
    if (!pClass->IsMarshaledByRef())
        return NoManagedActivation;

    BEGINFORBIDGC();
   
    
	ManagedActivationType bManaged = NoManagedActivation;
    if (pClass->IsConfigChecked())
    {
        // We have done work to figure this out in the past ... 
        // use the cached result
        bManaged = pClass->IsRemoteActivated() ? ManagedActivation : NoManagedActivation;
    }
    else if (pClass->IsContextful() || pClass->HasRemotingProxyAttribute()) 
    {
        // Contextful and classes that have a remoting proxy attribute 
        // (whether they are MarshalByRef or ContextFul) always take the slow 
        // path of managed activation
        bManaged = ManagedActivation;
    }
    else
    {
        // If we have parsed a config file that might have configured
        // this Type to be activated remotely 
        if (GetAppDomain()->IsRemotingConfigured())
        {
            bManaged = ManagedActivation;
            // We will remember if the activation is actually going
            // remote based on if the managed call to IsContextOK returned us
            // a proxy or not
        }

        if (bManaged == NoManagedActivation)
        {
            pClass->SetConfigChecked();
        }
    }

    ENDFORBIDGC();

    return bManaged;
}

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::CreateProxyOrObject    public
//
//  Synopsis:   Determine if the current context is appropriate
//              for activation. If the current context is OK then it creates 
//              an object else it creates a proxy.
//              
//
//  Note:       Called by JIT_NewCrossContext 
//+----------------------------------------------------------------------------
OBJECTREF CRemotingServices::CreateProxyOrObject(MethodTable* pMT, BOOL fIsCom)
{   
    _ASSERTE(!pMT->IsThunking());

    THROWSCOMPLUSEXCEPTION();

    EEClass* pClass = pMT->GetClass();

    // By the time we reach here, we have alread checked that the class requires
    // managed activation. This check is made either through the JIT_NewCrossContext helper
    // or Activator.CreateInstance codepath.
    _ASSERTE(RequiresManagedActivation(pClass) || IsRemoteActivationRequired(pClass));

    if(!s_fInitializedRemoting)
    {        
        if(!InitializeFields())
        {
            // Fatal Error if initialization returns false
            // We can throw exceptions here because a helper frame is setup
            // by the caller
            _ASSERTE(!"Initialization Failed");
            FATAL_EE_ERROR();
        }
    }

        Object *pServer = NULL;
        // Get the address of IsCurrentContextOK in managed code
        void* pTarget = NULL;

        {
            pTarget = (void *)CRemotingServices::MDofIsCurrentContextOK()->GetMethodEntryPoint();
        }

        // Arrays are not created by JIT_NewCrossContext
        _ASSERTE(!pClass->IsArrayClass());

        // Get the type seen by reflection
        REFLECTCLASSBASEREF reflectType = (REFLECTCLASSBASEREF) pClass->GetExposedClassObject();
        LPVOID pvType = NULL;
        *(REFLECTCLASSBASEREF *)&pvType = reflectType;

        // This will return either an uninitialized object or a proxy
        pServer = (Object *)CTPMethodTable::CallTarget(pTarget, pvType, NULL);

        if (!pClass->IsContextful() 
            && !pClass->GetMethodTable()->IsComObjectType())
        {   
            // Cache the result of the activation attempt ... 
            // if a strictly MBR class is not configured for remote 
            // activation we will not go 
            // through this slow path next time! 
            // (see RequiresManagedActivation)
            if (IsTransparentProxy(pServer))
            {
                // Set the flag that this class is remote activate
                // which means activation will go to managed code.
                pClass->SetRemoteActivated();
            }
            else
            {
                // Set only the flag that no managed checks are required
                // for this class next time.
                pClass->SetConfigChecked();
            }
        }

        LOG((LF_REMOTING, LL_INFO1000, "CreateProxyOrObject returning 0x%0x\n", pServer));
        if (pClass->IsContextful())
        {
            COUNTER_ONLY(GetPrivatePerfCounters().m_Context.cObjAlloc++);
            COUNTER_ONLY(GetGlobalPerfCounters().m_Context.cObjAlloc++);
        }
        return ObjectToOBJECTREF(pServer);
}

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::AllocateUninitializedObject    public
//
//  Synopsis:   Allocates an uninitialized object of the given type
//
//+----------------------------------------------------------------------------
FCIMPL1(Object*, CRemotingServices::AllocateUninitializedObject, ReflectClassBaseObject* pClassOfObjectUNSAFE)
{   
    OBJECTREF newobj = NULL;
    REFLECTCLASSBASEREF pClassOfObject = (REFLECTCLASSBASEREF) pClassOfObjectUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, pClassOfObject);
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();

    ReflectClass *pRefClass = (ReflectClass *) pClassOfObject->GetData();
    EEClass *pEEClass = pRefClass->GetClass();

    // Make sure that this private allocator function is used by remoting 
    // only for marshalbyref objects
    _ASSERTE(!pEEClass->IsContextful() || pEEClass->IsMarshaledByRef());


    // if this is an abstract class then we will
    //  fail this
    if (pEEClass->IsAbstract())
    {
        COMPlusThrow(kMemberAccessException,L"Acc_CreateAbst");
    }

    if (pEEClass->IsInterface())
    {
        COMPlusThrow(kMemberAccessException,L"Acc_CreateInterface");
    }

    newobj = AllocateObject(pEEClass->GetMethodTable());

    LOG((LF_REMOTING, LL_INFO1000, "AllocateUninitializedObject returning 0x%0x\n", OBJECTREFToObject(newobj)));

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(newobj);
}
FCIMPLEND


//+----------------------------------------------------------------------------
//
//  Method:     VOID RemotingServices::CallDefaultCtor(callDefaultCtorArgs* pArgs)
//  Synopsis:   call default ctor
//+----------------------------------------------------------------------------
FCIMPL1(VOID, CRemotingServices::CallDefaultCtor, Object* orefUNSAFE)
{
    OBJECTREF oref = (OBJECTREF) orefUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_1(oref);
    //-[autocvtpro]-------------------------------------------------------

    CallDefaultConstructor(oref);

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::AllocateInitializedObject    public
//
//  Synopsis:   Allocates an uninitialized object of the given type
//
//+----------------------------------------------------------------------------
FCIMPL1(Object*, CRemotingServices::AllocateInitializedObject, ReflectClassBaseObject* pClassOfObjectUNSAFE)
{   
    OBJECTREF newobj = NULL;
    REFLECTCLASSBASEREF pClassOfObject = (REFLECTCLASSBASEREF) pClassOfObjectUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, pClassOfObject);
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();

    ReflectClass *pRefClass = (ReflectClass *) pClassOfObject->GetData();
    EEClass *pEEClass = pRefClass->GetClass();

    // Make sure that this private allocator function is used by remoting 
    // only for marshalbyref objects
    _ASSERTE(!pEEClass->IsContextful() || pEEClass->IsMarshaledByRef());

    newobj = AllocateObject(pEEClass->GetMethodTable());

    CallDefaultConstructor(newobj);

    LOG((LF_REMOTING, LL_INFO1000, "AllocateInitializedObject returning 0x%0x\n", OBJECTREFToObject(newobj)));

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(newobj);
}
FCIMPLEND
//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::GetStubForNonVirtualMethod   public
//
//  Synopsis:   Get a stub for a non virtual method. 
//
//+----------------------------------------------------------------------------
Stub* CRemotingServices::GetStubForNonVirtualMethod(MethodDesc* pMD, LPVOID pvAddrOfCode, Stub* pInnerStub)
{
    THROWSCOMPLUSEXCEPTION();
    
    CPUSTUBLINKER sl;
    Stub* pStub = CTPMethodTable::CreateStubForNonVirtualMethod(pMD, &sl, pvAddrOfCode, pInnerStub);
    return pStub;
}

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::GetNonVirtualThunkForVirtualMethod   public
//
//  Synopsis:   Get a thunk for a non virtual method. 
//
//+----------------------------------------------------------------------------
LPVOID CRemotingServices::GetNonVirtualThunkForVirtualMethod(MethodDesc* pMD)
{
    THROWSCOMPLUSEXCEPTION();
    
    CPUSTUBLINKER sl;
    return CTPMethodTable::GetOrCreateNonVirtualThunkForVirtualMethod(pMD, &sl);
} 

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::DestroyThunk   public
//
//  Synopsis:   Destroy the thunk for the non virtual method. 
//
//+----------------------------------------------------------------------------
void CRemotingServices::DestroyThunk(MethodDesc* pMD)
{
    // Delegate to a helper routine
    CTPMethodTable::DestroyThunk(pMD);
} 

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::CheckCast   public
//
//  Synopsis:   Checks either 
//              (1) If the object type supports the given interface OR
//              (2) If the given type is present in the hierarchy of the 
//              object type
//+----------------------------------------------------------------------------
BOOL CRemotingServices::CheckCast(OBJECTREF orTP, EEClass* pObjClass, 
                                  EEClass *pClass)
{
    BEGINFORBIDGC();

    BOOL fCastOK = FALSE;

    _ASSERTE((NULL != pClass) && (NULL != pObjClass));
    // Object class can never be an interface. We use a separate cached
    // entry for storing interfaces that the proxy supports.
    _ASSERTE(!pObjClass->IsInterface());
    

    // (1) We are trying to cast to an interface 
    if(pClass->IsInterface())
    {
        // Do a quick check for interface cast by comparing it against the
        // cached entry
        MethodTable *pItfMT = (MethodTable *)orTP->GetPtrOffset(CTPMethodTable::GetOffsetOfInterfaceMT());
        if(NULL != pItfMT)
        {
            if(pItfMT == pClass->GetMethodTable())
            {
                fCastOK = TRUE;
            }
            else
            {
                fCastOK = pItfMT->GetClass()->StaticSupportsInterface(pClass->GetMethodTable());
            }
        }

        if(!fCastOK)
        {
            fCastOK = pObjClass->StaticSupportsInterface(pClass->GetMethodTable());
        }
        
    }
    // (2) Everything else...
    else
    {
        // Walk up the class hierarchy and find a matching class
        while (pObjClass != pClass)
        {
            if (pObjClass == NULL)
            {
                // Oh-oh, the cast did not succeed. Maybe we have to refine
                // the proxy to match the clients view
                break;
            }            

            // Continue searching
            pObjClass = pObjClass->GetParentClass();
        }

        if(pObjClass == pClass)
        {
            fCastOK = TRUE;
        }
    }

    ENDFORBIDGC();

    return fCastOK;
}

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::CheckCast   public
//
//  Synopsis:   Refine the type hierarchy that the proxy represents to match
//              the client view. If the client is trying to cast the proxy
//              to a type not supported by the server object then we 
//              return NULL
//
//+----------------------------------------------------------------------------
BOOL CRemotingServices::CheckCast(OBJECTREF orTP, EEClass *pClass)
{
    BEGINFORBIDGC();

    MethodTable *pMT = orTP->GetMethodTable();

    // Make sure that we have a transparent proxy
    _ASSERTE(pMT->IsTransparentProxyType());

    pMT = pMT->AdjustForThunking(orTP);
    EEClass *pObjClass = pMT->GetClass();

    // Do a cast check without taking a lock
    BOOL fCastOK = CheckCast(orTP, pObjClass, pClass);

    ENDFORBIDGC();

    if(!fCastOK)
    {
        // Cast on arrays work via ComplexArrayStoreCheck. We should not
        // reach here for such cases.
        _ASSERTE(!pClass->IsArrayClass());

        // We reach here only if any of the types in the current type hierarchy
        // represented by the proxy does not match the given type.     
        // Call a helper routine in managed RemotingServices to find out 
        // whether the server object supports the given type
        const void* pTarget = (const void *)MDofCheckCast()->GetMethodEntryPoint();
        fCastOK = CTPMethodTable::CheckCast(pTarget, orTP, pClass);
    }

    LOG((LF_REMOTING, LL_INFO100, "CheckCast returning %s for object 0x%x and class 0x%x \n", (fCastOK ? "TRUE" : "FALSE")));

    return (fCastOK);
}

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::NativeCheckCast    public
//
//  Synopsis:   Does a CheckCast to force the expansion of the MethodTable for 
//              the object (possibly a proxy) contained in pvObj.  Returns True if
//              the object is not a proxy or if it can be cast to the specified
//              type.
//+----------------------------------------------------------------------------
FCIMPL2(Object*, CRemotingServices::NativeCheckCast, Object* pObj, ReflectClassBaseObject* pType) 
{
    _ASSERTE(pObj != NULL);
    _ASSERTE(pType != NULL);

    OBJECTREF orObj(pObj);
    REFLECTCLASSBASEREF typeObj(pType);

    //Get the EEClass of the object which we have and the class to which we're widening.
    ReflectClass *pRC = (ReflectClass *)typeObj->GetData();
    EEClass *pEEC = pRC->GetClass();
    EEClass *pEECOfObj = orObj->GetClass();

    //Always initialize retval
    // If it's thunking, check what we actually have.
    if (pEECOfObj->IsThunking()) {
        HELPER_METHOD_FRAME_BEGIN_RET_1(orObj);
        if (!CRemotingServices::CheckCast(orObj, pEEC))
            orObj = 0;
        HELPER_METHOD_FRAME_END();
    } 
    return OBJECTREFToObject(orObj);
}
HCIMPLEND

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::FieldAccessor   public
//
//  Synopsis:   Sets/Gets the value of the field given an instance or a proxy
//+----------------------------------------------------------------------------
void CRemotingServices::FieldAccessor(FieldDesc* pFD, OBJECTREF o, LPVOID pVal,
                                      BOOL fIsGetter)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(o->IsThunking() || o->GetClass()->IsMarshaledByRef());

    EEClass *pClass = o->GetClass();
    CorElementType fieldType = pFD->GetFieldType();
    UINT cbSize = GetSizeForCorElementType(fieldType);
    BOOL fIsGCRef = pFD->IsObjRef();
    BOOL fIsByValue = pFD->IsByValue();
    TypeHandle  th;
    EEClass *fldClass = NULL;

    //
    // We mustn't try to get the type handle of a field
    // unless it's a proxy or value type.  Otherwise
    // there is an obscure case where we may cause
    // class loading to happen which is forbidden here.
    // (e.g. getting or setting a null field with a type which
    // hasn't been restored yet.)
    //

    if(!pClass->IsMarshaledByRef() || fIsByValue)
    {
        // Extract the type of the field
        PCCOR_SIGNATURE pSig;
        DWORD       cSig;
        pFD->GetSig(&pSig, &cSig);
        FieldSig sig(pSig, pFD->GetModule());

        OBJECTREF throwable = NULL;
        GCPROTECT_BEGIN(throwable);
        GCPROTECT_BEGIN(o);
        GCPROTECT_BEGININTERIOR(pVal);
        th = sig.GetTypeHandle(&throwable);
        if (throwable != NULL)
            COMPlusThrow(throwable);
        GCPROTECT_END();
        GCPROTECT_END();
        GCPROTECT_END();
        // Extract the field class for unshared method tables only
        if(th.IsUnsharedMT())
        {
            fldClass = th.AsClass();
        }
    }

    if(pClass->IsMarshaledByRef())
    {
        BEGINFORBIDGC();

        _ASSERTE(!o->IsThunking());
    
        // This is a reference to a real object. Get/Set the field value
        // and return
        LPVOID pFieldAddress = pFD->GetAddress((LPVOID)OBJECTREFToObject(o));
        LPVOID pDest = (fIsGetter ? pVal : pFieldAddress);
        LPVOID pSrc  = (fIsGetter ? pFieldAddress : pVal);
        if(fIsGCRef && !fIsGetter)
        {
            SetObjectReference((OBJECTREF*)pDest, ObjectToOBJECTREF(*(Object **)pSrc), o->GetAppDomain());
        }
        else if(fIsByValue) 
        {
            CopyValueClass(pDest, pSrc, th.AsMethodTable(), o->GetAppDomain());
        }
        else
        {    
            CopyDestToSrc(pDest, pSrc, cbSize);
        }

        ENDFORBIDGC();
    }
    else
    {
        // This is a reference to a proxy. Get the real class of the instance.
        pClass = pFD->GetMethodTableOfEnclosingClass()->GetClass();
#ifdef _DEBUG
        EEClass *pCheckClass = CTPMethodTable::GetClassBeingProxied(o);
    
        while (pCheckClass != pClass) 
        {
            pCheckClass = pCheckClass->GetParentClass();
            _ASSERTE(pCheckClass);
        }
#endif

        // Call the managed code to start the field access call
        CallFieldAccessor(pFD, o, pVal, fIsGetter, fIsByValue, fIsGCRef, pClass, 
                          fldClass, fieldType, cbSize);        
    }
}

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::CopyDestToSrc   private
//
//  Synopsis:   Copies the specified number of bytes from the src to dest
//
//+----------------------------------------------------------------------------
VOID CRemotingServices::CopyDestToSrc(LPVOID pDest, LPVOID pSrc, UINT cbSize)
{
    BEGINFORBIDGC();

    switch (cbSize)
    {
        case 1:
            *(INT8*)pDest = *(INT8*)pSrc;
            break;
    
        case 2:
            *(INT16*)pDest = *(INT16*)pSrc;
            break;
    
        case 4:
            *(INT32*)pDest = *(INT32*)pSrc;
            break;
    
        case 8:
            *(INT64*)pDest = *(INT64*)pSrc;
            break;
    
        default:
            _ASSERTE(!"Invalid size");
            break;
    }

    ENDFORBIDGC();
}

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::CallFieldAccessor   private
//
//  Synopsis:   Sets up the arguments and calls RealProxy::FieldAccessor
//
//+----------------------------------------------------------------------------
VOID CRemotingServices::CallFieldAccessor(FieldDesc* pFD,
                                          OBJECTREF o, 
                                          VOID* pVal, 
                                          BOOL fIsGetter, 
                                          BOOL fIsByValue, 
                                          BOOL fIsGCRef,
                                          EEClass *pClass, 
                                          EEClass *fldClass, 
                                          CorElementType fieldType, 
                                          UINT cbSize)
{
    THROWSCOMPLUSEXCEPTION();

    //****************************WARNING******************************
    // GC Protect all non-primitive variables
    //*****************************************************************
    
    FieldArgs fieldArgs;
    // Initialize the member variables because GCPROTECT_BEGIN expects 
    // valid contents. NULL is a valid value
    fieldArgs.obj = NULL;
    fieldArgs.val = NULL;
    fieldArgs.typeName = NULL;
    fieldArgs.fieldName = NULL;    

    GCPROTECT_BEGIN(fieldArgs); // fieldArgs

    fieldArgs.obj = o;

    // protect the field value if it is a gc-ref type
    if(fIsGCRef)
    {
        fieldArgs.val = ObjectToOBJECTREF(*(Object **)pVal);
    }

    // Set up the arguments
    
    // Argument 1: String typeName
    // Argument 2: String fieldName
    // Get the type name and field name strings
    GetTypeAndFieldName(&fieldArgs, pFD); 
    
    // Argument 3: Object val
    OBJECTREF val = NULL;
    if(!fIsGetter)
    {
        // If we are setting a field value then we create a variant data 
        // structure to hold the field value        
        // Extract the field from the gc protected structure if it is an object
        // else use the value passed to the function
        LPVOID pvFieldVal = (fIsGCRef ? (LPVOID)&(fieldArgs.val) : pVal);
        OBJECTREF *lpVal = &val;
        GetObjectFromStack(lpVal, pvFieldVal, fieldType, fldClass); 
    }
        
    // Get the method descriptor of the call
    MethodDesc *pMD = (fIsGetter ? MDofFieldGetter() : MDofFieldSetter());
            
    // Call the field accessor function 
    //////////////////////////////// GETTER ///////////////////////////////////
    if(fIsGetter)
    {       
        // Set up the return value
        OBJECTREF oRet = NULL;

        GCPROTECT_BEGIN (oRet);
        CRemotingServices__CallFieldGetter(
                        pMD, 
                        (LPVOID)OBJECTREFToObject(fieldArgs.obj),
                        (LPVOID)OBJECTREFToObject(fieldArgs.typeName),
                        (LPVOID)&oRet,
                        (LPVOID)OBJECTREFToObject(fieldArgs.fieldName));

        // If we are getting a field value then extract the field value
        // based on the type of the field    
        if(fIsGCRef)
        {
            // Do a check cast to ensure that the field type and the 
            // return value are compatible
            OBJECTREF orRet = oRet;
            OBJECTREF orSaved = orRet;
            if(IsTransparentProxy(OBJECTREFToObject(orRet)))
            {
                GCPROTECT_BEGIN(orRet);

                if(!CheckCast(orRet, fldClass))
                {
                    COMPlusThrow(kInvalidCastException, L"Arg_ObjObj");
                }

                orSaved = orRet;

                GCPROTECT_END();
            }

            *(OBJECTREF *)pVal = orSaved;
        }
        else if (fIsByValue) 
        {       
            // Copy from the source to the destination
            if (oRet != NULL)
            {
                CopyValueClass(pVal, oRet->UnBox(), fldClass->GetMethodTable(), fieldArgs.obj->GetAppDomain());
            }
        }
        else
        {
            if (oRet != NULL)
            {                
                CopyDestToSrc(pVal, oRet->UnBox(), cbSize);
            }
        }    
        GCPROTECT_END ();
    }
    ///////////////////////// SETTER //////////////////////////////////////////
    else
    {    
        CRemotingServices__CallFieldSetter(
                        pMD, 
                        (LPVOID)OBJECTREFToObject(fieldArgs.obj), 
                        (LPVOID)OBJECTREFToObject(fieldArgs.typeName), 
                        (LPVOID)OBJECTREFToObject(val), 
                        (LPVOID)OBJECTREFToObject(fieldArgs.fieldName));
    }

    GCPROTECT_END(); // fieldArgs
}
  
//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::GetTypeAndFieldName   private
//
//  Synopsis:   Get the type name and field name of the 
//
//+----------------------------------------------------------------------------
VOID CRemotingServices::GetTypeAndFieldName(FieldArgs *pArgs, FieldDesc *pFD)                                   
{
    THROWSCOMPLUSEXCEPTION();

    DWORD          i = 0;
    REFLECTBASEREF refField = NULL;
    ReflectFieldList* pFields = NULL;
    ReflectClass *pRC = NULL;
    LPCUTF8 pszFieldName = pFD->GetName();
    LPWSTR  szName = NULL;

    // Protect the reflection info
    REFLECTCLASSBASEREF reflectType = (REFLECTCLASSBASEREF)pFD->GetEnclosingClass()->GetExposedClassObject();
        
    pRC = (ReflectClass *)(reflectType->GetData());
    // This call can cause a GC!
    pFields = pRC->GetFields();    

    for(i=0;i<pFields->dwFields;i++) 
    {
        // Check for access to non-publics

        // Get the FieldDesc and match names
        if (MatchField(pFields->fields[i].pField, pszFieldName)) 
        {
            // Found the first field that matches, so return it
            // This call can cause a GC!
            refField = pFields->fields[i].GetFieldInfo(pRC);

            break;
        }
    }

    if(refField == NULL)
    {
        // Throw an exception
        COMPlusThrow(kMissingFieldException, L"Arg_MissingFieldException");
    }

    // Extract the type name and field name string
    DefineFullyQualifiedNameForClassW();
    szName = GetFullyQualifiedNameForClassW(pFD->GetEnclosingClass());    
    pArgs->typeName = COMString::NewString(szName);
    pArgs->fieldName = COMString::NewString(pszFieldName);
}

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::MatchField   private
//
//  Synopsis:   Find out whether the given field name is the same as the name
//              of the field descriptor field name.
//
//+----------------------------------------------------------------------------
BOOL CRemotingServices::MatchField(FieldDesc* pCurField, LPCUTF8 szFieldName)
{
    BEGINFORBIDGC();

    _ASSERTE(pCurField);

    // Get the name of the field
    LPCUTF8 pwzCurFieldName = pCurField->GetName();
    
    ENDFORBIDGC();

    return strcmp(pwzCurFieldName, szFieldName) == 0;
}

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::Unwrap   public
//
//  Synopsis:   Unwrap a proxy to return the underlying object
//              
//
//+----------------------------------------------------------------------------
FCIMPL1(Object*, CRemotingServices::Unwrap, Object* pvTP)
{
    return pvTP;
}
FCIMPLEND    

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::AlwaysUnwrap   public
//
//  Synopsis:   Unwrap a proxy to return the underlying object
//              
//
//+----------------------------------------------------------------------------
FCIMPL1(Object*, CRemotingServices::AlwaysUnwrap, Object* obj)
{
    VALIDATEOBJECTREF(obj);
    
    //**********WARNING************************************************
    // Do not throw exceptions or provoke GC without setting up a frame    
    //
    //*****************************************************************                 
    if(IsTransparentProxy(obj))
        obj = OBJECTREFToObject(GetObjectFromProxy(OBJECTREF(obj), TRUE));

    return obj;
}
FCIMPLEND    

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::Wrap   public
//
//  Synopsis:   Wrap a contextful object to create a proxy
//              Delegates to a helper method to do the actual work
//
//+----------------------------------------------------------------------------
OBJECTREF CRemotingServices::Wrap(OBJECTREF obj)
{
    THROWSCOMPLUSEXCEPTION();

    // Basic sanity check
    VALIDATEOBJECTREF(obj);

    // ******************* WARNING ********************************************
    // Do not throw any exceptions or provoke GC without setting up a frame.
    // At present its the callers responsibility to setup a frame that can 
    // handle exceptions.
    // ************************************************************************    
    OBJECTREF orProxy = obj;
    if(obj != NULL && (obj->GetMethodTable()->IsContextful()))       
    {
        if(!IsTransparentProxy(OBJECTREFToObject(obj)))
        {
            // See if we can extract the proxy from the object
            orProxy = GetProxyFromObject(obj);
            if(orProxy == NULL)
            {
                // ask the remoting services to wrap the object
                orProxy = CRemotingServices::WrapHelper(obj);

                // Check to make sure that everything went fine
                if(orProxy == NULL)
                {
                    // The frame should have been setup by now
                    FATAL_EE_ERROR();
                }                 
            }
        }
    }

    return orProxy;
}

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::WrapHelper   public
//
//  Synopsis:   Wrap an object to return a proxy. This function assumes that 
//              a fcall frame is already setup.
//              Called by JIT_Wrap & Wrap
//+----------------------------------------------------------------------------
OBJECTREF CRemotingServices::WrapHelper(OBJECTREF obj)
{
    // Basic sanity check
    VALIDATEOBJECTREF(obj);

    // Default return value indicates an error
    OBJECTREF newobj = NULL;
    const void *pTarget = NULL;

    _ASSERTE((obj != NULL) && 
                (!IsTransparentProxy(OBJECTREFToObject(obj))) &&
                obj->GetMethodTable()->IsContextful());
    if (InitializeRemoting())
    {
        // Get the address of wrap in managed code        
        pTarget = (const void *)CRemotingServices::MDofWrap()->GetMethodEntryPoint();
        _ASSERTE(pTarget);
    
        // call the managed method to wrap
        newobj = ObjectToOBJECTREF( (Object *)CTPMethodTable::CallTarget(pTarget,
                                                (LPVOID)OBJECTREFToObject(obj),
                                                NULL));    
    }
    
    return newobj;
}

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::GetProxyFromObject   public
//
//  Synopsis:   Extract the proxy from the field in the 
//              ContextBoundObject class
//              
//+----------------------------------------------------------------------------
OBJECTREF CRemotingServices::GetProxyFromObject(OBJECTREF obj)
{
    BEGINFORBIDGC();

    // Basic sanity check
    VALIDATEOBJECTREF(obj);

    // We can derive a proxy for contextful types only.
    _ASSERTE(obj->GetMethodTable()->IsContextful());

    OBJECTREF srvID = (OBJECTREF)(Object*)obj->GetPtrOffset(s_dwMBRIDOffset);
    OBJECTREF orProxy = NULL;
    
    if (srvID != NULL)
    {
        orProxy = (OBJECTREF)(Object*)srvID->GetPtrOffset(s_dwTPOrObjOffsetInIdentity);
    }

    // This should either be null or a proxy type
    _ASSERTE((orProxy == NULL) || 
             IsTransparentProxy(OBJECTREFToObject(orProxy)));

    ENDFORBIDGC();

    return orProxy;
}

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::IsProxyToRemoteObject   public
//
//  Synopsis:   Check if the proxy is to a remote object
//              (1) TRUE : if object is non local (ie outside this PROCESS) otherwise
//              (2) FALSE 
//+----------------------------------------------------------------------------
BOOL CRemotingServices::IsProxyToRemoteObject(OBJECTREF obj)
{
    // Basic sanity check
    VALIDATEOBJECTREF(obj);

    BOOL fRemote = TRUE;

    // If remoting is not initialzed, for now let us
    // just return FALSE
    if(!s_fInitializedRemoting)
        return FALSE;

    BEGINFORBIDGC();

    _ASSERTE(obj != NULL);

    if(!obj->GetMethodTable()->IsTransparentProxyType())
    {       
        fRemote = FALSE;    
    }
    ENDFORBIDGC();
    
    // so it is a transparent proxy
    if (fRemote != FALSE)
    {       
        AppDomain *pDomain = GetServerDomainForProxy(obj);
        if(pDomain != NULL)
        {
            fRemote = FALSE;
        }
    }    

    return fRemote;
}

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::GetObjectFromProxy   public
//
//  Synopsis:   Extract the object given a proxy. 
//              fMatchContexts if
//              (1) TRUE It matches the current context with the server context
//              and if they match then returns the object else the proxy
//              (2) FALSE returns the object without matching the contexts.
//              WARNING!! This should be used by code which is context-aware.
//              
//              
//+----------------------------------------------------------------------------
OBJECTREF CRemotingServices::GetObjectFromProxy(OBJECTREF obj, 
                                                BOOL fMatchContexts)
{
    BEGINFORBIDGC();

    // Basic sanity check
    VALIDATEOBJECTREF(obj);

    // Make sure that remoting is initialized
    ASSERT(s_fInitializedRemoting);

    // Make sure that we are given a proxy
    ASSERT(IsTransparentProxy(OBJECTREFToObject(obj)));

    OBJECTREF oref = NULL;
    if (CTPMethodTable__GenericCheckForContextMatch(OBJECTREFToObject(obj)))
    {
        OBJECTREF objRef = ObjectToOBJECTREF(GetRealProxy(OBJECTREFToObject(obj)));
        oref = (OBJECTREF)(Object*)objRef->GetPtrOffset(s_dwServerOffsetInRealProxy);
        if (oref != NULL)
        {
            obj = oref; 
        }
    }

    ENDFORBIDGC();

    return obj;
}

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::GetServerContext   public
//
//  Synopsis:   Gets the context of the object. If the object is a proxy 
//              extract the context from the identity else the current context
//              is the context of the object.
//              
//+----------------------------------------------------------------------------
OBJECTREF CRemotingServices::GetServerContext(OBJECTREF obj)
{
    BEGINFORBIDGC();

    // Basic sanity check
    VALIDATEOBJECTREF(obj);

    OBJECTREF serverCtx = NULL;

    if(IsTransparentProxy(OBJECTREFToObject(obj)))
    {
        OBJECTREF id = GetServerIdentityFromProxy(obj);
        if(id != NULL)
        {
            // We can extract the server context only for proxies of objects
            // that were born in this app domain. 
            serverCtx  = (OBJECTREF)(Object*)id->GetPtrOffset(s_dwServerCtxOffset);
            _ASSERTE(IsInstanceOfContext(serverCtx->GetMethodTable()));
        }
    }
    else
    {
        // Current context is the server context
        serverCtx = GetCurrentContext()->GetExposedObject();
    }

    ENDFORBIDGC();

    return serverCtx;
}

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::GetServerIdentityFromProxy   private
//
//  Synopsis:   Gets the server identity (if one exists) from a proxy
//              
//              
//              
//+----------------------------------------------------------------------------
OBJECTREF CRemotingServices::GetServerIdentityFromProxy(OBJECTREF obj)
{
    BEGINFORBIDGC();

    // Make sure that we are given a proxy
    ASSERT(IsTransparentProxy(OBJECTREFToObject(obj)));

    // Extract the real proxy underlying the transparent proxy
    OBJECTREF pObj = ObjectToOBJECTREF(GetRealProxy(OBJECTREFToObject(obj)));

    OBJECTREF id = NULL;
        
    // Extract the identity object
    pObj = (OBJECTREF)(Object*)pObj->GetPtrOffset(s_dwIdOffset);

    // Extract the _identity from the real proxy only if it is an instance of 
    // remoting proxy
    if((pObj != NULL) && IsInstanceOfServerIdentity(pObj->GetMethodTable()))
    {
        id = pObj;
    }

    ENDFORBIDGC();

    return id;
}

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::GetServerDomainForProxy public
//
//  Synopsis:   Returns the AppDomain corresponding to the server
//              if the proxy and the server are in the same process.
//              
//+----------------------------------------------------------------------------
AppDomain *CRemotingServices::GetServerDomainForProxy(OBJECTREF proxy)
{
    // call the managed method 
    Context *pContext = (Context *)GetServerContextForProxy(proxy);
    if (pContext)
    {
        return pContext->GetDomain();
    }
    else
    {
        return NULL;
    }
}

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::GetServerDomainIdForProxy public
//
//  Synopsis:   Returns the AppDomain ID corresponding to the server
//              if the proxy and the server are in the same process.
//              Returns 0 if it cannot determine.
//              
//+----------------------------------------------------------------------------
int CRemotingServices::GetServerDomainIdForProxy(OBJECTREF proxy)
{
    _ASSERTE(IsTransparentProxy(OBJECTREFToObject(proxy)));

    TRIGGERSGC();

    // Get the address of GetDomainIdForProxy in managed code
    const void *pTarget = (const void *)
    CRemotingServices::MDofGetServerDomainIdForProxy()->GetMethodEntryPoint();
    _ASSERTE(pTarget);

    // This will just read the appDomain ID from the marshaled data
    // for the proxy. It returns 0 if the proxy is to a server in another
    // process. It may also return 0 if it cannot determine the server
    // domain ID (eg. for Well Known Object proxies).

    // call the managed method
    return (int)(INT_PTR)CTPMethodTable::CallTarget(
                pTarget,
                (LPVOID)OBJECTREFToObject(proxy),
                NULL);
}


//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::GetServerContextForProxy public
//
//  Synopsis:   Returns the AppDomain corresponding to the server
//              if the proxy and the server are in the same process.
//              
//+----------------------------------------------------------------------------
Context *CRemotingServices::GetServerContextForProxy(OBJECTREF proxy)
{
    _ASSERTE(IsTransparentProxy(OBJECTREFToObject(proxy)));
    

    TRIGGERSGC();

    // Get the address of GetAppDomainForProxy in managed code        
    const void *pTarget = (const void *)
    CRemotingServices::MDofGetServerContextForProxy()->GetMethodEntryPoint();
    _ASSERTE(pTarget);
    
    // This will return the correct VM Context object for the server if 
    // the proxy is true cross domain proxy to a server in another domain 
    // in the same process. The managed method will Assert if called on a proxy
    // which is either half-built or does not have an ObjRef ... which may
    // happen for eg. if the proxy and the server are in the same appdomain.

    // we return NULL if the server object for the proxy is in another 
    // process or if the appDomain for the server is invalid or if we cannot
    // determine the context (eg. well known object proxies).

    // call the managed method 
    return (Context *)CTPMethodTable::CallTarget(
                            pTarget,
                            (LPVOID)OBJECTREFToObject(proxy),
                            NULL);    
}


// To get the ExportedType for a nested class, we must first get
// the ExportedTypes for all of its enclosers.
HRESULT NestedExportedTypeHelper(
    IMDInternalImport *pTDImport, mdTypeDef mdCurrent, 
    IMDInternalImport *pCTImport, mdExportedType *mct)
{
    mdTypeDef mdEnclosing;
    LPCSTR szcNameSpace;
    LPCSTR szcName;
    HRESULT hr;

    pTDImport->GetNameOfTypeDef(mdCurrent, &szcName, &szcNameSpace);
    if (SUCCEEDED(pTDImport->GetNestedClassProps(mdCurrent, &mdEnclosing))) {
        hr = NestedExportedTypeHelper(pTDImport, mdEnclosing, pCTImport, mct);
        if (FAILED(hr)) return hr;

        return pCTImport->FindExportedTypeByName(szcNameSpace, szcName, *mct, mct);
    }

    return pCTImport->FindExportedTypeByName(szcNameSpace, szcName, mdTokenNil, mct);
}


//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::GetExecutionLocation   private
//
//  Synopsis:   Finds the execution location for a given class from the 
//              manifest
//              
//              
//+----------------------------------------------------------------------------
HRESULT CRemotingServices::GetExecutionLocation(EEClass *pClass, LPCSTR pszLoc)
{
    // Init the out params
    pszLoc = NULL;

    _ASSERTE(!"No longer implemented");

        return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::CreateProxyForDomain   public
//
//  Synopsis:   Create a proxy for the app domain object by calling marshal
//              inside the newly created domain and unmarshaling in the old
//              domain
//              
//+----------------------------------------------------------------------------
OBJECTREF CRemotingServices::CreateProxyForDomain(AppDomain* pDomain)
{
    THROWSCOMPLUSEXCEPTION();

    // Ensure that the fields of remoting service have been initialized
    // This is separated from the initialization of the remoting services
    if (!s_fInitializedRemoting)
    {
        if (!InitializeFields())
        {
            _ASSERTE(!"Initialization Failed");
            FATAL_EE_ERROR();
    }

    }

    const void *pTarget = (const void *)MDOfCreateProxyForDomain()->GetMethodEntryPoint();

    // Call the managed method which will marshal and unmarshal the 
    // appdomain object to create the proxy

    // We pass the ContextID of the default context of the new appDomain
    // object. This helps the boot-strapping! (i.e. entering the new domain
    // to marshal itself out).

    Object *proxy = (Object *)CTPMethodTable::CallTarget(
                                    pTarget, 
                                    (LPVOID)(DWORD_PTR)pDomain->GetId(),
                                    (LPVOID)pDomain->GetDefaultContext());
    return ObjectToOBJECTREF(proxy);
}

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::GetClass   public
//
//  Synopsis:   Extract the true class of the object whose proxy is given.
//              
//              
//+----------------------------------------------------------------------------
REFLECTCLASSBASEREF CRemotingServices::GetClass(OBJECTREF pThis)
{
    THROWSCOMPLUSEXCEPTION();

    REFLECTCLASSBASEREF refClass = NULL;
    EEClass *pClass = NULL;

    GCPROTECT_BEGIN(pThis);

    TRIGGERSGC();

    // For proxies to objects in the same appdomain, we always know the
    // correct type
    if(GetServerIdentityFromProxy(pThis) != NULL)
    {
        pClass = pThis->GetTrueMethodTable()->GetClass();
    }
    // For everything else either we have refined the proxy to its correct type
    // or we have to consult the objref to get the true type
    else
    {   const void *pTarget = (const void *)CRemotingServices::MDofGetType()->GetMethodEntryPoint();

        refClass = (REFLECTCLASSBASEREF)(ObjectToOBJECTREF((Object *)CTPMethodTable::CallTarget(pTarget, (LPVOID)OBJECTREFToObject(pThis), NULL)));
        if(refClass == NULL)
        {
            // There was no objref associated with the proxy or it is a proxy
            // that we do not understand. 
            // In this case, we return the class that is stored in the proxy
            pClass = pThis->GetTrueMethodTable()->GetClass();
        }

        _ASSERTE(refClass != NULL || pClass != NULL);

        // Refine the proxy to the class just retrieved
        if(refClass != NULL)
        {
            if(!CTPMethodTable::RefineProxy(pThis, 
                                            ((ReflectClass *)refClass->GetData())->GetClass()))
            {
                // Throw an exception to indicate that we failed to expand the 
                // method table to the given size.
                FATAL_EE_ERROR();
            }
        }
    }    

    if (refClass == NULL)
        refClass = (REFLECTCLASSBASEREF)pClass->GetExposedClassObject();

    GCPROTECT_END();

    _ASSERTE(refClass != NULL);
    return refClass;
}

//+----------------------------------------------------------------------------
//
//  Method:     CRealProxy::SetStubData   public
//
//  Synopsis:   Set the stub data in the transparent proxy
//+----------------------------------------------------------------------------
FCIMPL2(VOID, CRealProxy::SetStubData, LPVOID pvRP, LPVOID pvStubData)
{
    BOOL fThrow = FALSE;
    OBJECTREF orRP = ObjectToOBJECTREF((Object *)pvRP);    
    
    if(orRP != NULL)
    {
        OBJECTREF orTP = ObjectToOBJECTREF((Object *)orRP->GetPtrOffset(CRemotingServices::GetTPOffset()));
        if(orTP != NULL)
        {
            orTP->SetOffsetObjectRef(
                    CTPMethodTable::GetOffsetOfStubData(), 
                    (size_t)pvStubData);
        }
        else
        {
            fThrow = TRUE;
        }
    }
    else
    {
        fThrow = TRUE;
    }
    
    if(fThrow)
    {
        FCThrowVoid(kArgumentNullException);
    }
}
FCIMPLEND

//+----------------------------------------------------------------------------
//
//  Method:     CRealProxy::GetStubData   public
//
//  Synopsis:   Get the stub data in the transparent proxy
//+----------------------------------------------------------------------------
FCIMPL1(LPVOID, CRealProxy::GetStubData, LPVOID pvRP)
{
    BOOL fThrow = FALSE;
    OBJECTREF orRP = ObjectToOBJECTREF((Object *)pvRP);    
    LPVOID pvRet = NULL;

    if(orRP != NULL)
    {
    OBJECTREF orTP = ObjectToOBJECTREF((Object *)orRP->GetPtrOffset(CRemotingServices::GetTPOffset()));
        if(orTP != NULL)
        {
            pvRet = (LPVOID)orTP->GetPtrOffset(CTPMethodTable::GetOffsetOfStubData());
        }
        else
        {
            fThrow = TRUE;
        }
    }
    else
    {
        fThrow = TRUE;
    }
    
    if(fThrow)
    {
        FCThrow(kArgumentNullException);
    }

    return pvRet;
}
FCIMPLEND

//+----------------------------------------------------------------------------
//
//  Method:     CRealProxy::GetDefaultStub   public
//
//  Synopsis:   Get the default stub implemented by us which matches contexts
//+----------------------------------------------------------------------------
FCIMPL0(LPVOID, CRealProxy::GetDefaultStub)
{

    return (LPVOID)CRemotingServices__CheckForContextMatch;
}
FCIMPLEND

//+----------------------------------------------------------------------------
//
//  Method:     CRealProxy::GetStub   public
//
//  Synopsis:   Get the stub pointer in the transparent proxy 
//+----------------------------------------------------------------------------
FCIMPL1(ULONG_PTR, CRealProxy::GetStub, LPVOID pvRP)
{
    ULONG_PTR stub = 0;
    OBJECTREF orRP = ObjectToOBJECTREF((Object *)pvRP);    
    OBJECTREF orTP = ObjectToOBJECTREF((Object *)orRP->GetPtrOffset(CRemotingServices::GetTPOffset()));
            
    stub = (ULONG_PTR)orTP->GetPtrOffset(CTPMethodTable::GetOffsetOfStub()); 

    return stub;
}
FCIMPLEND

//+----------------------------------------------------------------------------
//
//  Method:     CRealProxy::GetProxiedType   public
//
//  Synopsis:   Get the type that is represented by the transparent proxy 
//+----------------------------------------------------------------------------
FCIMPL1(Object*, CRealProxy::GetProxiedType, Object* orRPUNSAFE)
{
    REFLECTCLASSBASEREF refClass = NULL;
    OBJECTREF orRP = (OBJECTREF) orRPUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, orRP);
    //-[autocvtpro]-------------------------------------------------------

    OBJECTREF orTP = ObjectToOBJECTREF((Object *)orRP->GetPtrOffset(CRemotingServices::GetTPOffset()));
    
    refClass = CRemotingServices::GetClass(orTP);
    
    _ASSERTE(refClass != NULL);

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refClass);
}
FCIMPLEND

//+----------------------------------------------------------------------------
//
//  Method:     CTPMethodTable::Initialize   public
//
//  Synopsis:   Initialized data structures needed for managing tranparent
//              proxies
//+----------------------------------------------------------------------------
BOOL CTPMethodTable::Initialize()
{
    // Init    
    s_cRefs = 0;
    s_dwCommitedTPSlots = 0;
    s_dwReservedTPSlots = 0;
    s_pThunkTable = NULL;
    s_pTransparentProxyClass = NULL;
    s_dwGCInfoBytes = 0;
    s_dwMTDataSlots = 0;
    s_dwRPOffset = 0;
    s_dwMTOffset = 0;
    s_dwItfMTOffset = 0;
    s_dwStubOffset = 0;
    s_dwStubDataOffset = 0;
    s_dwMaxSlots = 0;
    s_pTPMT = NULL;    
    s_pTPStub = NULL;    
    s_pDelegateStub = NULL;    
    s_fInitializedTPTable = FALSE;

    // Initialize the thunks
    CVirtualThunks::Initialize();
    CNonVirtualThunk::Initialize();
    
    InitializeCriticalSection(&s_TPMethodTableCrst);
    
    return TRUE;
}


//+----------------------------------------------------------------------------
//
//  Method:     CTPMethodTable::Cleanup   public
//
//  Synopsis:   Cleansup data structures used for managing tranparent
//              proxies
//+----------------------------------------------------------------------------
void CTPMethodTable::Cleanup()
{
    // Replace the transparent proxy method table with the true TP method
    // table so that the transparent proxy class can be unloaded properly
    if(s_pTransparentProxyClass && s_pTPMT)
    {
        s_pTransparentProxyClass->SetMethodTableForTransparentProxy(s_pTPMT);
    }

    // Reclaim memory used by transparent proxies
    if(s_pThunkTable)
    {
        DestroyThunkTable();
    }

    // Reclaim memory used by thunks
    CVirtualThunks *pNextVirtualThunk = CVirtualThunks::GetVirtualThunks();
    CVirtualThunks *pCurrentVirtualThunk = NULL;
    while (pNextVirtualThunk)
    {
        pCurrentVirtualThunk = pNextVirtualThunk;
        pNextVirtualThunk = pNextVirtualThunk->_pNext;
        CVirtualThunks::DestroyVirtualThunk(pCurrentVirtualThunk);
    }

    // Uninit
    if (s_pTPStub)
        s_pTPStub->DecRef();

    if (s_pDelegateStub)
        s_pDelegateStub->DecRef();

    // Clean up the stub managers which aid in debugging
    CVirtualThunkMgr::Cleanup();

    CNonVirtualThunkMgr::Cleanup();

    DeleteCriticalSection(&s_TPMethodTableCrst);

    // Delete the hash table used to store the thunks
    if(s_pThunkHashTable)
    {
        // We need to empty out the hash table of all the thunks we've store in it
        EmptyThunkHashTable();
        delete s_pThunkHashTable;
        s_pThunkHashTable = NULL;
    }
    
    return;
}

//+----------------------------------------------------------------------------
//
//  Method:     CTPMethodTable::EmptyThunkHashTable private
//
//  Synopsis:   Frees all the Thunks that are stored in the hash table
//+----------------------------------------------------------------------------
void CTPMethodTable::EmptyThunkHashTable()
{
    EEHashTableIteration Itr;
    LPVOID pvCode = NULL;

    s_pThunkHashTable->IterateStart(&Itr);

    while(s_pThunkHashTable->IterateNext(&Itr))
    {
        pvCode = s_pThunkHashTable->IterateGetValue(&Itr);  

        if(NULL != pvCode)
            delete CNonVirtualThunk::AddrToThunk(pvCode);
    }
}// CTPMethodTable::EmptyThunkHashTable


//+----------------------------------------------------------------------------
//
//  Method:     CTPMethodTable::InitThunkHashTable private
//
//  Synopsis:   Inits the hashtable used to store Thunks
//+----------------------------------------------------------------------------
void CTPMethodTable::InitThunkHashTable()
{
    s_pThunkHashTable = new EEThunkHashTable();
}// CTPMethodTable:InitThunkHashTable


//+----------------------------------------------------------------------------
//
//  Method:     CTPMethodTable::InitializeFields    private
//
//  Synopsis:   Initialize the static fields of CTPMethodTable class
//              and the thunk manager classes
//
//+----------------------------------------------------------------------------
BOOL CTPMethodTable::InitializeFields()
{
    BOOL bRet = TRUE;

    EE_TRY_FOR_FINALLY
    {
        // Acquire the lock
        LOCKCOUNTINCL("InitializeFields in remoting.cpp");
        EnterCriticalSection(&s_TPMethodTableCrst);
        if(!s_fInitializedTPTable)
        {
            // Load Tranparent proxy class
            s_pTransparentProxyClass = g_Mscorlib.GetClass(CLASS__TRANSPARENT_PROXY)->GetClass();

            s_pTPMT = s_pTransparentProxyClass->GetMethodTable();
            s_pTPMT->SetTransparentProxyType();
            
            // Obtain size of GCInfo stored above the method table
            CGCDesc *pGCDesc = CGCDesc::GetCGCDescFromMT(s_pTPMT);
            BYTE *pGCTop = (BYTE *) pGCDesc->GetLowestSeries();
            s_dwGCInfoBytes = (DWORD)(((BYTE *) s_pTPMT) - pGCTop);
            _ASSERTE((s_dwGCInfoBytes & 3) == 0);
            
            // Obtain the number of bytes to be copied for creating the TP
            // method tables containing thunks
            _ASSERTE(((s_dwGCInfoBytes + MethodTable::GetOffsetOfVtable()) & (sizeof(SLOT)-1)) == 0);
            s_dwMTDataSlots = ((s_dwGCInfoBytes + MethodTable::GetOffsetOfVtable()) / sizeof(SLOT));
            
            // We rely on the number of interfaces implemented by the
            // Transparent proxy being 0, so that InterfaceInvoke hints
            // fail and trap to InnerFailStub which also fails and
            // in turn traps to FailStubWorker. In FailStubWorker, we
            // determine the class being proxied and return correct slot.
            _ASSERTE(s_pTPMT->m_wNumInterface == 0);
        
            // Calculate offsets to various fields defined by the
            // __Transparent proxy class
            s_dwRPOffset        = g_Mscorlib.GetFieldOffset(FIELD__TRANSPARENT_PROXY__RP);
            s_dwMTOffset        = g_Mscorlib.GetFieldOffset(FIELD__TRANSPARENT_PROXY__MT);
            s_dwItfMTOffset     = g_Mscorlib.GetFieldOffset(FIELD__TRANSPARENT_PROXY__INTERFACE_MT);
            s_dwStubOffset      = g_Mscorlib.GetFieldOffset(FIELD__TRANSPARENT_PROXY__STUB);
            s_dwStubDataOffset  = g_Mscorlib.GetFieldOffset(FIELD__TRANSPARENT_PROXY__STUB_DATA);
    
            _ASSERTE(s_dwStubDataOffset == (TP_OFFSET_STUBDATA  - sizeof(MethodTable*)));
            _ASSERTE(s_dwMTOffset       == (TP_OFFSET_MT        - sizeof(MethodTable*)));
            _ASSERTE(s_dwStubOffset     == (TP_OFFSET_STUB      - sizeof(MethodTable*)));

            // Create the one and only transparent proxy stub
            s_pTPStub = CreateTPStub();
            _ASSERTE(s_pTPStub);
            if(!s_pTPStub)
            {
                bRet = FALSE;
            }

            // Create the one and only delegate stub
            s_pDelegateStub = CreateDelegateStub();
            _ASSERTE(s_pDelegateStub);
            if(!s_pDelegateStub)
            {
                bRet = FALSE;
            }

            if(bRet)
            {
                _ASSERTE(NULL == s_pThunkHashTable);
                InitThunkHashTable();
                if(NULL != s_pThunkHashTable)
                {
                    LockOwner lock = {&s_TPMethodTableCrst,IsOwnerOfOSCrst};
                    bRet = s_pThunkHashTable->Init(23,&lock);
                }
                else
                {
                    bRet = FALSE;
                }                
            }
    
                // Set the largest possible vtable size 64K
                s_dwMaxSlots = 64*1024;
    
            if(bRet)
            {
                // Create the global thunk table and set the cycle between
                // the transparent proxy class and the global thunk table
                bRet = CreateTPMethodTable();
    
                // Either we successfully initialized the method table or
                // the return value is false
                _ASSERTE(!bRet || s_pThunkTable);
            }
    
            // NOTE: This must always be the last statement in this block
            // to prevent races
            // Load Tranparent proxy class
            s_fInitializedTPTable = TRUE;
        }
    }
    EE_FINALLY
    {
        LeaveCriticalSection(&s_TPMethodTableCrst);
        LOCKCOUNTDECL("InitializeFields in remoting.cpp");
        // Leave the lock
    }EE_END_FINALLY;
    
    // Make sure that the field has been set (done at the end of a 
    // successful initialization)
    _ASSERTE(!bRet || s_fInitializedTPTable);
    
    return bRet;
}

//+----------------------------------------------------------------------------
//
//  Method:     CTPMethodTable::GetRP       public
//
//  Synopsis:   Get the real proxy backing the transparent proxy
//
//+----------------------------------------------------------------------------
OBJECTREF CTPMethodTable::GetRP(OBJECTREF orTP) 
{
    //THROWSCOMPLUSEXCEPTION();

    if(!s_fInitializedTPTable)
    {
        if(!InitializeFields())
        {
        }
    }

    _ASSERTE(s_fInitializedTPTable);
    return (OBJECTREF)(Object*)orTP->GetPtrOffset(s_dwRPOffset);
}

//+----------------------------------------------------------------------------
//
//  Method:     CTPMethodTable::CreateTPMethodTable   private
//
//  Synopsis:   (1) Reserves a transparent proxy method table that is large 
//              enough to support the largest vtable
//              (2) Commits memory for the GC info of the global thunk table and
//              sets the cycle between the transparent proxy class and the 
//              globale thunk table.
//+----------------------------------------------------------------------------
BOOL CTPMethodTable::CreateTPMethodTable()
{
    // Allocate virtual memory that is big enough to hold a method table
    // of the maximum possible size
    DWORD dwReserveSize = ((DWORD)((s_dwMTDataSlots * sizeof(SLOT)) +
                                   (s_dwMaxSlots    * sizeof(SLOT)) +
                                    g_SystemInfo.dwPageSize)) & ~(g_SystemInfo.dwPageSize - 1);
    void *pAlloc = ::VirtualAlloc(0, dwReserveSize,
                                  MEM_RESERVE | MEM_TOP_DOWN,
                                  PAGE_EXECUTE_READWRITE);
    
    if (pAlloc)
    {
        BOOL bFailed = TRUE;
        // Compute reserved slots
        DWORD dwReservedSlots = dwReserveSize - (s_dwMTDataSlots * sizeof(SLOT));
        _ASSERTE((dwReservedSlots & (sizeof(SLOT)-1)) == 0);
        dwReservedSlots = dwReservedSlots / sizeof(SLOT);

        // Make sure that we have not created the one and only
        // transparent proxy method table before
        _ASSERTE(NULL == s_pThunkTable);


        WS_PERF_SET_HEAP(REMOTING_HEAP);

        // Commit the required amount of memory
        DWORD dwCommitSize = (s_dwMTDataSlots) * sizeof(SLOT);
        if (::VirtualAlloc(pAlloc, dwCommitSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE))
        {
            WS_PERF_UPDATE("CreateTPMethodTable", dwCommitSize, pAlloc);

            // Copy the fixed portion from the true TP Method Table
            memcpy(pAlloc,MTToAlloc(s_pTPMT, s_dwGCInfoBytes),
                   (s_dwMTDataSlots * sizeof(SLOT)));

            // Initialize the transparent proxy method table
            InitThunkTable(
                        0, 
                        dwReservedSlots, 
                        AllocToMT((BYTE *) pAlloc, s_dwGCInfoBytes));

            // At this point the transparent proxy class points to the
            // the true TP Method Table and not the transparent 
            // proxy method table. We do not use the true method table
            // any more. Instead we use the transparent proxy method table
            // for allocating transparent proxies. So, we have to make the
            // transparent proxy class point to the one and only transparent 
            // proxy method table
            //MethodTable *pMethodTable = s_pTransparentProxyClass->GetMethodTable();
            CTPMethodTable::s_pTransparentProxyClass->SetMethodTableForTransparentProxy(s_pThunkTable);

            // Allocate the slots of the Object class method table because
            // we can reflect on the __Transparent proxy class even though 
            // we never intend to use remoting.
            _ASSERTE(NULL != g_pObjectClass);
            _ASSERTE(0 == GetCommitedTPSlots());
            if(ExtendCommitedSlots(g_pObjectClass->GetTotalSlots()))
            {
                bFailed = FALSE;

                // We override the slots allocated for the methods
                // on System.Object class with the slots in 
                // __TransparentProxy class. This gives us the desired behavior 
                // of not intercepting methods on System.Object and executing
                // them locally.
                /*SLOT *pThunkVtable = s_pThunkTable->GetVtable();
                SLOT *pClassVtable = pMethodTable->GetVtable();
                for(unsigned i = 0; i < g_pObjectClass->GetTotalSlots(); i++)
                {
                    pThunkVtable[i] = pClassVtable[i];
                }*/
            }
        }
        else{
            VirtualFree(pAlloc, 0, MEM_RELEASE);
        }
        
        if(bFailed)
        {
            DestroyThunkTable();
        }        
    }

    // Note that the thunk table is set to null on any failure path
    // via DestroyThunkTable
    return (s_pThunkTable == NULL ? FALSE : TRUE);
}

//+----------------------------------------------------------------------------
//
//  Method:     CTPMethodTable::ExtendCommitedSlots   private
//
//  Synopsis:   Extends the commited slots of transparent proxy method table to
//              the desired number
//+----------------------------------------------------------------------------
BOOL CTPMethodTable::ExtendCommitedSlots(DWORD dwSlots)
{
    // Sanity checks
    _ASSERTE(s_dwCommitedTPSlots <= dwSlots);
    _ASSERTE(dwSlots <= s_dwReservedTPSlots);
    _ASSERTE((CVirtualThunks::GetVirtualThunks() == NULL) || 
                (s_dwCommitedTPSlots == CVirtualThunks::GetVirtualThunks()->_dwCurrentThunk));
    // Either we have initialized everything or we are asked to allocate
    // some slots during initialization
    _ASSERTE(s_fInitializedTPTable || (0 == s_dwCommitedTPSlots));

    // Commit memory for TPMethodTable
    WS_PERF_SET_HEAP(REMOTING_HEAP);

    BOOL bAlloc = FALSE;
    void *pAlloc = MTToAlloc(s_pThunkTable, s_dwGCInfoBytes);
    DWORD dwCommitSize = (s_dwMTDataSlots + dwSlots) * sizeof(SLOT);
    if (::VirtualAlloc(pAlloc, dwCommitSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE))
    {
        WS_PERF_UPDATE("ExtendCommittedSlots", dwCommitSize, pAlloc);

        bAlloc = AllocateThunks(dwSlots, dwCommitSize);
        if (!bAlloc)
            VirtualFree(pAlloc, dwCommitSize, MEM_DECOMMIT);
    }

    return bAlloc;
}

//+----------------------------------------------------------------------------
//
//  Method:     CTPMethodTable::AllocateThunks   private
//
//  Synopsis:   Allocates the desired number of thunks for virtual methods
//+----------------------------------------------------------------------------
BOOL CTPMethodTable::AllocateThunks(DWORD dwSlots, DWORD dwCommitSize)
{
    // Check for existing thunks
    DWORD dwCommitThunks = 0;
    DWORD dwAllocThunks = dwSlots;
    void **pVTable = (void **) s_pThunkTable->GetVtable();
    CVirtualThunks* pThunks = CVirtualThunks::GetVirtualThunks();
    if (pThunks)
    {
        // Compute the sizes of memory to be commited and allocated
        BOOL fCommit;
        if (dwSlots < pThunks->_dwReservedThunks)
        {
            fCommit = TRUE;
            dwCommitThunks = dwSlots;
            dwAllocThunks = 0;
        } 
        else
        {
            fCommit = (pThunks->_dwCurrentThunk != pThunks->_dwReservedThunks);
            dwCommitThunks = pThunks->_dwReservedThunks;
            dwAllocThunks = dwSlots - pThunks->_dwReservedThunks;
        }

        // Commit memory if needed
        if (fCommit)
        {
            WS_PERF_SET_HEAP(REMOTING_HEAP);
            DWORD dwCommitSize = (sizeof(CVirtualThunks) - ConstVirtualThunkSize) +
                                 ((dwCommitThunks - pThunks->_dwStartThunk) * ConstVirtualThunkSize);
            if (!::VirtualAlloc(pThunks, dwCommitSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE))
                return(NULL);
            WS_PERF_UPDATE("ExtendCommittedSlots", dwCommitSize, pThunks);

            // Generate thunks that push slot number and jump to TP stub
            DWORD dwStartSlot = pThunks->_dwStartThunk;
            DWORD dwCurrentSlot = pThunks->_dwCurrentThunk;
            while (dwCurrentSlot < dwCommitThunks)
            {
                pVTable[dwCurrentSlot] = &pThunks->ThunkCode[dwCurrentSlot-dwStartSlot];
                CreateThunkForVirtualMethod(dwCurrentSlot, (BYTE *) pVTable[dwCurrentSlot]);
                ++dwCurrentSlot;
            }
            FlushInstructionCache(GetCurrentProcess(), 
                                  &pThunks->ThunkCode[pThunks->_dwCurrentThunk-dwStartSlot], 
                                  (dwCommitThunks-pThunks->_dwCurrentThunk)*ConstVirtualThunkSize);
            s_dwCommitedTPSlots = dwCommitThunks;
            pThunks->_dwCurrentThunk = dwCommitThunks;
        }
    }


    // Allocate memory if necessary
    if (dwAllocThunks)
    {
        DWORD dwReserveSize = ((sizeof(CVirtualThunks) - ConstVirtualThunkSize) +
                               ((dwAllocThunks << 1) * ConstVirtualThunkSize) +
                               g_SystemInfo.dwPageSize) & ~((size_t) g_SystemInfo.dwPageSize - 1);
        void *pAlloc = ::VirtualAlloc(0, dwReserveSize,
                                      MEM_RESERVE | MEM_TOP_DOWN,
                                      PAGE_EXECUTE_READWRITE);
        if (pAlloc)
        {
            WS_PERF_SET_HEAP(REMOTING_HEAP);
            // Commit the required amount of memory
            DWORD dwCommitSize = (sizeof(CVirtualThunks) - ConstVirtualThunkSize) +
                                 (dwAllocThunks * ConstVirtualThunkSize);
            if (::VirtualAlloc(pAlloc, dwCommitSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE))
            {
                WS_PERF_UPDATE("AllocateThunks", dwCommitSize, pAlloc);
                ((CVirtualThunks *) pAlloc)->_pNext = pThunks;
                pThunks = CVirtualThunks::SetVirtualThunks((CVirtualThunks *) pAlloc);
                pThunks->_dwReservedThunks = (dwReserveSize -
                                                  (sizeof(CVirtualThunks) - ConstVirtualThunkSize)) /
                                                 ConstVirtualThunkSize;
                pThunks->_dwStartThunk = dwCommitThunks;
                pThunks->_dwCurrentThunk = dwCommitThunks;

                // Generate thunks that push slot number and jump to TP stub
                DWORD dwStartSlot = pThunks->_dwStartThunk;
                DWORD dwCurrentSlot = pThunks->_dwCurrentThunk;
                while (dwCurrentSlot < dwSlots)
                {
                    pVTable[dwCurrentSlot] = &pThunks->ThunkCode[dwCurrentSlot-dwStartSlot];
                    CreateThunkForVirtualMethod(dwCurrentSlot, (BYTE *) pVTable[dwCurrentSlot]);
                    ++dwCurrentSlot;
                }
                FlushInstructionCache(GetCurrentProcess(), 
                                      &pThunks->ThunkCode[pThunks->_dwCurrentThunk-dwStartSlot], 
                                      (dwSlots-pThunks->_dwCurrentThunk)*ConstVirtualThunkSize);
                s_dwCommitedTPSlots = dwSlots;
                pThunks->_dwCurrentThunk = dwSlots;
            } else
            {
                ::VirtualFree(pAlloc, 0, MEM_RELEASE);
                return FALSE;
            }
        } else
        {
            return FALSE;
        }
    }

    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CTPMethodTable::CreateTPForRP   private
//
//  Synopsis:   Creates a transparent proxy that behaves as an object of the
//              supplied class
//+----------------------------------------------------------------------------
OBJECTREF CTPMethodTable::CreateTPOfClassForRP(EEClass *pClass, OBJECTREF pRP)
{
    // Sanity check
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(pClass);

    OBJECTREF pTP = NULL;
    BOOL fAddRef = TRUE;
    RuntimeExceptionKind reException = kLastException;
    BOOL fAlloc = FALSE;

    if(!s_fInitializedTPTable)
    {
        if(!InitializeFields())
        {
            // Set the exception kind to RuntimeException
            reException = kExecutionEngineException;
        }
        else
        {
            fAddRef = FALSE;
        }
    }

    // Proceed only if we have successfully initialized the fields
    if(s_fInitializedTPTable)
    {
        // Get the size of the VTable for the class to proxy
        DWORD dwSlots = pClass->GetNumVtableSlots();
        if (dwSlots == 0)
            dwSlots = 1;
        
        // The global thunk table must have been initialized
        _ASSERTE(s_pThunkTable != NULL);

        // Check for the need to extend existing TP method table
        if (dwSlots > GetCommitedTPSlots())
        {            
            // Acquire the lock
            LOCKCOUNTINC("CreateTPOfClassForRP in remoting.cpp");
            EnterCriticalSection(&s_TPMethodTableCrst);
            if (dwSlots > GetCommitedTPSlots())
            {
                fAlloc = ExtendCommitedSlots(dwSlots);
            }
            else
            {
                // The existing method table is sufficient for us
                fAlloc = TRUE;
            }
            LeaveCriticalSection(&s_TPMethodTableCrst);
            LOCKCOUNTDECL("CreateTPOfClassForRP in remoting.cpp");
        }
        else
        {
            // The existing method table is sufficient for us
            fAlloc = TRUE;
        }
    }

    // Check for failure to create TP Method table of desired size
    if (fAlloc)
    {
        reException = kLastException;

        // GC protect the reference to real proxy
        GCPROTECT_BEGIN(pRP);

        // Create a TP Object
        pTP = FastAllocateObject(GetMethodTable());
        if (pTP != NULL)
        {
            // Sanity check
            //_ASSERTE((s_dwRPOffset == 0) && (s_dwStubDataOffset == 0x4) &&    // these offsets aren't portable 
            //         (s_dwMTOffset == 8) && (s_dwItfMTOffset == 0xc) &&       // however, they are _ASSERTE'ed in 
            //         (s_dwStubOffset == 0x10));                               // CTPMethodTable::InitializeFields

            // Create the cycle between TP and RP
            pRP->SetOffsetObjectRef(CRemotingServices::GetTPOffset(), (size_t)OBJECTREFToObject(pTP));

            // Make the TP behave as an object of supplied class
            pTP->SetOffsetObjectRef(s_dwRPOffset, (size_t) OBJECTREFToObject(pRP));
            
            // If we are creating a proxy for an interface then the class
            // is the object class else it is the class supplied
            if(pClass->IsInterface())
            {
                _ASSERTE(NULL != g_pObjectClass);

                //pTP->SetOffset32(s_dwMTOffset, (DWORD)g_pObjectClass);
                pTP->SetOffsetPtr(s_dwMTOffset, CRemotingServices::GetMarshalByRefClass());
                // Set the cached interface method table to the given interface
                // method table
                pTP->SetOffsetPtr(s_dwItfMTOffset, pClass->GetMethodTable()); 
            }
            else
            {
                pTP->SetOffsetPtr(s_dwMTOffset, pClass->GetMethodTable()); 
            }
            

            // Addref the TP Method Table if necessary
            if (fAddRef)
                AddRef();
        } 
        else
        {
            reException = kOutOfMemoryException;
        }

        GCPROTECT_END();
    }

    // Throw if necessary
    if (reException != kLastException)
    {
        COMPlusThrow(reException);
    }

    return(pTP);
}

//+----------------------------------------------------------------------------
//
//  Method:     CTPMethodTable::PreCall   private
//
//  Synopsis:   This function replaces the slot number with the function
//              descriptor thus completely setting up the frame
//              
//+----------------------------------------------------------------------------
void __stdcall CTPMethodTable::PreCall(TPMethodFrame* pFrame)
{
    BEGINFORBIDGC();

    _ASSERTE(s_fInitializedTPTable);


#ifdef _PPC_
    // On PPC, we can't call GetThis() because of the RegisterHelper has not
    // been called yet. "this" is either r3 or r4 depending on whether the method
    // has return buffer. The methods with return buffer are marked
    // with "oris r0, r0, 0" (=0x64000000) on the callsite
    PPCReg regThis = (*(DWORD*)pFrame->GetReturnAddress() == 0x64000000) ? kR4 : kR3;

    ArgumentRegisters* pRegArgs = (ArgumentRegisters*)
        ((BYTE*)pFrame + FramedMethodFrame::GetOffsetOfArgumentRegisters());

    OBJECTREF pTP = *(OBJECTREF*)(&pRegArgs->r[regThis - kR3]);
#else
    // The frame is not completly setup at this point.
    // Do not throw exceptions or provoke GC
    OBJECTREF pTP = pFrame->GetThis();
#endif
    MethodTable* pMT = (MethodTable*)(pTP->GetPtrOffset(s_dwMTOffset)); 
    _ASSERTE(pMT);
    DWORD dwSlot = (DWORD) pFrame->GetSlotNumber();

    // For virtual calls the slot number is pushed but for 
    // non virtual calls/interface invoke the method descriptor is already 
    // pushed
    if((DWORD) -1 != dwSlot)
    {
        // Replace the slot number with the method descriptor on the stack
        MethodDesc *pMD = pMT->GetClass()->GetMethodDescForSlot(dwSlot);
        pFrame->SetFunction(pMD);
    }

#ifdef _PPC_
    FramedMethodFrame::RegisterHelper(pFrame, FALSE);
#endif

    ENDFORBIDGC();
}

PCCOR_SIGNATURE InitMessageData(messageData *msgData, FramedMethodFrame *pFrame, Module **ppModule)
{
    msgData->pFrame = pFrame;
    msgData->iFlags = 0;

    MethodDesc *pMD = pFrame->GetFunction();
    EEClass* cls = pMD->GetClass();
    if (cls->IsAnyDelegateClass())
    {
        DelegateEEClass* delegateCls = (DelegateEEClass*) cls;

        _ASSERTE(pFrame->GetThis()->GetClass()->IsAnyDelegateClass());
        msgData->pDelegateMD = pMD;
        msgData->pMethodDesc = COMDelegate::GetMethodDesc(pFrame->GetThis());
        _ASSERTE(msgData->pMethodDesc != NULL);

        if (pMD == delegateCls->m_pBeginInvokeMethod)
        {
            msgData->iFlags |= MSGFLG_BEGININVOKE;
        }
        else
        {
            _ASSERTE(pMD == delegateCls->m_pEndInvokeMethod);
            msgData->iFlags |= MSGFLG_ENDINVOKE;
        }
    }
    else
    {
        msgData->pDelegateMD = NULL;
        msgData->pMethodDesc = pMD;
    }
    if (msgData->pMethodDesc->IsOneWay())
    {
        msgData->iFlags |= MSGFLG_ONEWAY;
    }

    if (msgData->pMethodDesc->IsCtor())
    {
        msgData->iFlags |= MSGFLG_CTOR;
    }

    PCCOR_SIGNATURE pSig;
    DWORD cSig;
    Module *pModule;

    if (msgData->pDelegateMD)
    {
        msgData->pDelegateMD->GetSig(&pSig, &cSig);
        pModule = msgData->pDelegateMD->GetModule();
    }
    else if (msgData->pMethodDesc->IsVarArg()) 
    {
        VASigCookie *pVACookie = pFrame->GetVASigCookie();
        pSig = pVACookie->mdVASig;
        pModule = pVACookie->pModule;
    }
    else 
    {
        msgData->pMethodDesc->GetSig(&pSig, &cSig);
        pModule = msgData->pMethodDesc->GetModule();
    }

    _ASSERTE(ppModule);
    *ppModule = pModule;
    return pSig;
}

//+----------------------------------------------------------------------------
//
//  Method:     CTPMethodTable::OnCall   private
//
//  Synopsis:   This function gets control in two situations
//              (1) When a call is made on the transparent proxy it delegates to              
//              PrivateInvoke method on the real proxy
//              (2) When a call is made on the constructor it again delegates to the 
//              PrivateInvoke method on the real proxy.
//              
//+----------------------------------------------------------------------------
ARG_SLOT __stdcall CTPMethodTable::OnCall(TPMethodFrame *pFrame, Thread *pThrd, ARG_SLOT *pReturn)
{
    _ASSERTE(s_fInitializedTPTable);


    _ASSERTE(pReturn == (ARG_SLOT*)pFrame->GetReturnValuePtr());

    *pReturn = 0;

    // The frame should be completely setup at this point    

#ifdef REMOTING_PERF
    CRemotingServices::LogRemotingStageInner(CLIENT_MSG_GEN);
#endif

    // We can handle exception and GC promotion from this point
    THROWSCOMPLUSEXCEPTION();

    messageData msgData;
    PCCOR_SIGNATURE pSig = NULL;
    Module *pModule = NULL;
    pSig = InitMessageData(&msgData, pFrame, &pModule);

    _ASSERTE(pSig && pModule);

    // Allocate metasig on the stack
    MetaSig mSig(pSig, pModule);
    msgData.pSig = &mSig; 

    MethodDesc *pMD = pFrame->GetFunction();    
    if (pMD->GetClass()->IsMultiDelegateClass())
    {
        // check that there is only one target
        if (COMDelegate::GetpNext()->GetValue32(pFrame->GetThis()) != NULL)
        {
            COMPlusThrow(kArgumentException, L"Remoting_Delegate_TooManyTargets");
        }
    }

#ifdef PROFILING_SUPPORTED
    // If profiling is active, notify it that remoting stuff is kicking in
    if (CORProfilerTrackRemoting())
        g_profControlBlock.pProfInterface->RemotingClientInvocationStarted(
            reinterpret_cast<ThreadID>(pThrd));
#endif // PROFILING_SUPPORTED

    OBJECTREF pThisPointer = NULL;

#ifdef PROFILING_SUPPORTED
    GCPROTECT_BEGIN(pThisPointer);
#endif // PROFILING_SUPPORTED

    if (pMD->GetClass()->IsAnyDelegateClass())
    {
        // this is an async call
        _ASSERTE(pFrame->GetThis()->GetClass()->IsAnyDelegateClass());

        COMDelegate::GetOR()->GetInstanceField(pFrame->GetThis(),&pThisPointer);
    }
    else
    {
        pThisPointer = pFrame->GetThis();
    }

#ifdef PROFILING_SUPPORTED
    GCPROTECT_END();
#endif // PROFILING_SUPPORTED


    OBJECTREF firstParameter;
    const void *pTarget = NULL;
    size_t callType = CALLTYPE_INVALIDCALL;
    // We are invoking either the constructor or a method on the object
    if(pMD->IsCtor())
    {
        // Get the address of PrivateInvoke in managed code
        pTarget = (const void *)CRemotingServices::MDofPrivateInvoke()->GetMethodEntryPoint();
        _ASSERTE(pTarget);
        _ASSERTE(IsTPMethodTable(pThisPointer->GetMethodTable()));
        firstParameter = (OBJECTREF)(Object*)(DWORD_PTR)pThisPointer->GetPtrOffset(s_dwRPOffset);

        // Set a field to indicate that it is a constructor call
        callType = CALLTYPE_CONSTRUCTORCALL;
    }
    else
    {
        // Set a field to indicate that it is a method call
        callType = CALLTYPE_METHODCALL;

        if (IsTPMethodTable(pThisPointer->GetMethodTable()))
        {

            _ASSERTE(pReturn == (void *) (((BYTE *)pFrame) - pFrame->GetNegSpaceSize() - sizeof(INT64)));

            // Extract the real proxy underlying the transparent proxy
            firstParameter = (OBJECTREF)(Object*)pThisPointer->GetPtrOffset(s_dwRPOffset);

            // Get the address of PrivateInvoke in managed code
            pTarget = (const void *)CRemotingServices::MDofPrivateInvoke()->GetMethodEntryPoint();
            _ASSERTE(pTarget);
        }
        else 
        {
            // must be async if this is not a TP 
            _ASSERTE(pMD->GetClass()->IsAnyDelegateClass());
            firstParameter = NULL;
            
            // Get the address of PrivateInvoke in managed code
            pTarget = (const void *)CRemotingServices::MDofInvokeStatic()->GetMethodEntryPoint();
            _ASSERTE(pTarget);
        }

        
        // Go ahead and call PrivateInvoke on Real proxy. There is no need to 
        // catch exceptions thrown by it
        // See RealProxy.cs
    }

    _ASSERTE(pTarget);
    
    // Call the appropriate target
    CallTarget(pTarget, (LPVOID)OBJECTREFToObject(firstParameter), (LPVOID)&msgData, (LPVOID)callType);

    // Check for the need to trip thread
    if (pThrd->CatchAtSafePoint())
    {
        // There is no need to GC protect the return object as
        // TPFrame is GC protecting it
        CommonTripThread();
    }

    // floating point return values go in different registers.
    // check that here.
    CorElementType typ = msgData.pSig->GetReturnType();
    if (typ == ELEMENT_TYPE_R4)
    {
        setFPReturn(4, *pReturn);
    }
    else if (typ == ELEMENT_TYPE_R8)
    {
        setFPReturn(8, *pReturn);
    }

#ifdef PROFILING_SUPPORTED
    // If profiling is active, tell profiler we've made the call, received the
    // return value, done any processing necessary, and now remoting is done.
    if (CORProfilerTrackRemoting())
        g_profControlBlock.pProfInterface->RemotingClientInvocationFinished(
            reinterpret_cast<ThreadID>(pThrd));
#endif // PROFILING_SUPPORTED

    // Set the number of bytes to pop
    pFrame->SetFunction((void *)(size_t)pMD->CbStackPop());

#ifdef REMOTING_PERF
    CRemotingServices::LogRemotingStageInner(CLIENT_END_CALL);
#endif

    ARG_SLOT    retval = *pReturn;


    return retval;
}

//+----------------------------------------------------------------------------
//
//  Method:     CTPMethodTable::CheckCast   private
//
//  Synopsis:   Call the managed checkcast method to determine whether the 
//              server type can be cast to the given type
//              
//              
//+----------------------------------------------------------------------------
BOOL CTPMethodTable::CheckCast(const void* pTarget, OBJECTREF orTP, EEClass *pClass)
{
    THROWSCOMPLUSEXCEPTION();
    REFLECTCLASSBASEREF reflectType = NULL;
    LPVOID pvType = NULL;    
    BOOL fCastOK = FALSE;
    
    typedef struct _GCStruct
    {
        OBJECTREF orTP;
        OBJECTREF orRP;
    } GCStruct;

    GCStruct gcValues;
    gcValues.orTP = orTP;
    gcValues.orRP = GetRP(orTP);

    GCPROTECT_BEGIN (gcValues);
    COMPLUS_TRY
    {        
        reflectType = (REFLECTCLASSBASEREF) pClass->GetExposedClassObject();
        *(REFLECTCLASSBASEREF *)&pvType = reflectType;

        fCastOK = (BOOL)(INT_PTR)CallTarget(pTarget, 
                                   (LPVOID)OBJECTREFToObject(gcValues.orRP),
                                   pvType);    
    }
    COMPLUS_CATCH
    {
        fCastOK = FALSE;
        COMPlusRareRethrow();
    }
    COMPLUS_END_CATCH


    if(fCastOK)
    {
        _ASSERTE(s_fInitializedTPTable);

        // The cast succeeded. Replace the current type in the proxy
        // with the given type. 

        // Acquire the lock
        LOCKCOUNTINCL("CheckCast in remoting.cpp");
        EnterCriticalSection(&s_TPMethodTableCrst);

        MethodTable *pCurrent = (MethodTable *) gcValues.orTP->GetPtrOffset(s_dwMTOffset);
        
        
        if(pClass->IsInterface())
        {
            // We replace the cached interface method table with the interface
            // method table that we are trying to cast to. This will ensure that
            // casts to this interface, which are likely to happen, will succeed.
            gcValues.orTP->SetOffsetPtr(s_dwItfMTOffset, pClass->GetMethodTable());
        }
        else
        {
            BOOL fDerivedClass = FALSE;
            // Check whether this class derives from the current class
            fDerivedClass = CRemotingServices::CheckCast(gcValues.orTP, pClass,
                                                         pCurrent->GetClass());
            // We replace the current method table only if we cast to a more 
            // derived class
            if(fDerivedClass)
            {
                // Set the method table in the proxy to the given method table
                fCastOK = RefineProxy(gcValues.orTP, pClass);
            }
        }
                
        // Release the lock
        LeaveCriticalSection(&s_TPMethodTableCrst);
                LOCKCOUNTDECL("CheckCast in remoting.cpp");
    }

    GCPROTECT_END();
    return fCastOK;
}

//+----------------------------------------------------------------------------
//
//  Method:     CTPMethodTable::RefineProxy   public
//
//  Synopsis:   Set the method table in the proxy to the given class' method table.
//              Additionally, expand the TP method table to the required number of slots.
//              
//+----------------------------------------------------------------------------
BOOL CTPMethodTable::RefineProxy(OBJECTREF orTP, EEClass *pClass)
{
    _ASSERTE((orTP != NULL) && (pClass != NULL));

    BOOL fExpanded = TRUE;

    // Do the expansion only if necessary
    MethodTable *pMT = pClass->GetMethodTable();
    if (pMT != (MethodTable *)orTP->GetPtrOffset(s_dwMTOffset))
    {
        orTP->SetOffsetPtr(s_dwMTOffset, pMT); 
    
        // Extend the vtable if necessary
        DWORD dwSlots = pClass->GetNumVtableSlots();
        if (dwSlots == 0)
            dwSlots = 1;
    
        if((dwSlots > GetCommitedTPSlots()) && !ExtendCommitedSlots(dwSlots))
        {
            // We failed to extend the committed slots. Indicate a failure
            // by setting the flag to false
            fExpanded = FALSE;
        }
    }

    return fExpanded;
}

//+----------------------------------------------------------------------------
//
//  Method:     CTPMethodTable::IsTPMethodTable   private
//
//  Synopsis:   Returns TRUE if the supplied method table is the one and only TP Method
//              Table
//+----------------------------------------------------------------------------
INT32 CTPMethodTable::IsTPMethodTable(MethodTable *pMT)
{    
    if (GetMethodTable() == pMT)
    {
        return(TRUE);        
    }
    else
    {
        return(FALSE);
    }

}

//+----------------------------------------------------------------------------
//
//  Method:     CTPMethodTable::DestroyThunk   public
//
//  Synopsis:   Destroy the thunk for the non virtual method. 
//
//+----------------------------------------------------------------------------
void CTPMethodTable::DestroyThunk(MethodDesc* pMD)
{
    if(s_pThunkHashTable)
    {
        EnterCriticalSection (&s_TPMethodTableCrst);

        LPVOID pvCode = NULL;
        s_pThunkHashTable->GetValue(pMD, (HashDatum *)&pvCode);
        CNonVirtualThunk *pThunk = NULL;
        if(NULL != pvCode)
        {
            pThunk = CNonVirtualThunk::AddrToThunk(pvCode);
            delete pThunk;
            s_pThunkHashTable->DeleteValue(pMD);
        }

        LeaveCriticalSection (&s_TPMethodTableCrst);
    }
} 


//+----------------------------------------------------------------------------
//
//  Method:     CTPMethodTable::CallTarget   private
//
//  Synopsis:   Calls the target method on the given object
//+----------------------------------------------------------------------------
LPVOID __stdcall CTPMethodTable::CallTarget (const void *pTarget,
                                            LPVOID pvFirst,
                                            LPVOID pvSecond)
{
#ifdef _DEBUG
    Thread* curThread = GetThread();
    
    Object* ObjRefTable[OBJREF_TABSIZE];
    if (curThread)
        memcpy(ObjRefTable, curThread->dangerousObjRefs,
               sizeof(curThread->dangerousObjRefs));
    
    if (curThread)
        curThread->SetReadyForSuspension ();

    _ASSERTE(curThread->PreemptiveGCDisabled());  // Jitted code expects to be in cooperative mode
#endif

    LPVOID ret;
    INSTALL_COMPLUS_EXCEPTION_HANDLER();
    ret = CTPMethodTable__CallTargetHelper2(pTarget, pvFirst, pvSecond);
    UNINSTALL_COMPLUS_EXCEPTION_HANDLER();
    
#ifdef _DEBUG
    // Restore dangerousObjRefs when we return back to EE after call
    if (curThread)
        memcpy(curThread->dangerousObjRefs, ObjRefTable,
               sizeof(curThread->dangerousObjRefs));

    TRIGGERSGC ();

    ENABLESTRESSHEAP ();
#endif

    return ret;
}

LPVOID __stdcall CTPMethodTable::CallTarget (const void *pTarget,
                                            LPVOID pvFirst,
                                            LPVOID pvSecond,
                                            LPVOID pvThird)
{
#ifdef _DEBUG
    Thread* curThread = GetThread();
    
    Object* ObjRefTable[OBJREF_TABSIZE];
    if (curThread)
        memcpy(ObjRefTable, curThread->dangerousObjRefs,
               sizeof(curThread->dangerousObjRefs));
    
    if (curThread)
        curThread->SetReadyForSuspension ();

    _ASSERTE(curThread->PreemptiveGCDisabled());  // Jitted code expects to be in cooperative mode
#endif

    LPVOID ret;
    INSTALL_COMPLUS_EXCEPTION_HANDLER();
    ret = CTPMethodTable__CallTargetHelper3(pTarget, pvFirst, pvSecond, pvThird);
    UNINSTALL_COMPLUS_EXCEPTION_HANDLER();
    
#ifdef _DEBUG
    // Restore dangerousObjRefs when we return back to EE after call
    if (curThread)
        memcpy(curThread->dangerousObjRefs, ObjRefTable,
               sizeof(curThread->dangerousObjRefs));

    TRIGGERSGC ();

    ENABLESTRESSHEAP ();
#endif
    
    return ret;
}


//+----------------------------------------------------------------------------
//
//  Method:     CNonVirtualThunk::SetNextThunk   public
//
//  Synopsis:   Creates a thunk for the given address and adds it to the global
//              list
//+----------------------------------------------------------------------------
CNonVirtualThunk* CNonVirtualThunk::SetNonVirtualThunks(const BYTE* pbCode)
{    
    THROWSCOMPLUSEXCEPTION();

    CNonVirtualThunk *pThunk = new CNonVirtualThunk(pbCode);            
    if(NULL == pThunk)
    {
        COMPlusThrowOM();
    }

    // Put the generated thunk in a global list
    // Note: this is called when a NV thunk is being created ..
    // The TPMethodTable critsec is held at this point
    pThunk->SetNextThunk();

    // Set up the stub manager if necessary
    CNonVirtualThunkMgr::InitNonVirtualThunkManager();

    return pThunk;
}

//+----------------------------------------------------------------------------
//
//  Method:     CNonVirtualThunk::~CNonVirtualThunk   public
//
//  Synopsis:   Deletes the thunk from the global list of thunks
//              
//+----------------------------------------------------------------------------
CNonVirtualThunk::~CNonVirtualThunk()
{
    _ASSERTE(NULL != s_pNonVirtualThunks);

    CNonVirtualThunk* pCurr = s_pNonVirtualThunks;
    CNonVirtualThunk* pPrev = NULL;
    BOOL found = FALSE;

    // Note: This is called with the TPMethodTable critsec held
    while(!found && (NULL != pCurr))
    {
        if(pCurr == this)
        {
            found = TRUE;
            // Unlink from the chain 
            if(NULL != pPrev)
            {                    
                pPrev->_pNext = pCurr->_pNext;
            }
            else
            {
               // First entry needs to be deleted
                s_pNonVirtualThunks = pCurr->_pNext;
            }
        }
        pPrev = pCurr;
        pCurr = pCurr->_pNext;
    }

    _ASSERTE(found);
}

//+----------------------------------------------------------------------------
//
//  Method:     CVirtualThunkMgr::InitVirtualThunkManager   public
//
//  Synopsis:   Adds the stub manager to aid debugger in stepping into calls
//              
//+----------------------------------------------------------------------------
void CVirtualThunkMgr::InitVirtualThunkManager(const BYTE* stubAddress)
{    
    THROWSCOMPLUSEXCEPTION();

    // This is function is already threadsafe since this method is called from within a 
    // critical section                               
    if(NULL == s_pVirtualThunkMgr)
    {
        // Add the stub manager for vtable calls
        s_pVirtualThunkMgr =  new CVirtualThunkMgr(stubAddress);
        if (s_pVirtualThunkMgr == NULL)
        {
            COMPlusThrowOM();
        }
    
        StubManager::AddStubManager(s_pVirtualThunkMgr);
    }

}

//+----------------------------------------------------------------------------
//
//  Method:     CVirtualThunkMgr::Cleanup   public
//
//  Synopsis:   Removes the stub manager that aids the debugger 
//              
//+----------------------------------------------------------------------------
void CVirtualThunkMgr::Cleanup()
{
    if(s_pVirtualThunkMgr)
    {
        delete s_pVirtualThunkMgr;
        s_pVirtualThunkMgr = NULL;
    }
}

//+----------------------------------------------------------------------------
//
//  Method:     CVirtualThunkMgr::CheckIsStub   public
//
//  Synopsis:   Returns TRUE if the given address is the starting address of
//              the transparent proxy stub
//+----------------------------------------------------------------------------
BOOL CVirtualThunkMgr::CheckIsStub(const BYTE *stubStartAddress)
{
    BOOL bIsStub = FALSE;

    if(NULL != FindThunk(stubStartAddress))
    {
        bIsStub = TRUE;       
    }

    return bIsStub;
}

//+----------------------------------------------------------------------------
//
//  Method:     CVirtualThunkMgr::Entry2MethodDesc   public
//
//  Synopsis:   Convert a starting address to a MethodDesc
//+----------------------------------------------------------------------------
MethodDesc *CVirtualThunkMgr::Entry2MethodDesc(const BYTE *StubStartAddress, MethodTable *pMT)
{
    if (pMT && IsThunkByASM(StubStartAddress) && CheckIsStub(StubStartAddress))
    {
        return GetMethodDescByASM(StubStartAddress, pMT);
    }
    else
    {
        return NULL;
    }
}


//+----------------------------------------------------------------------------
//
//  Method:     CVirtualThunkMgr::FindThunk   private
//
//  Synopsis:   Finds a thunk that matches the given starting address
//+----------------------------------------------------------------------------
LPBYTE CVirtualThunkMgr::FindThunk(const BYTE *stubStartAddress)
{
    CVirtualThunks* pThunks = CVirtualThunks::GetVirtualThunks();
    LPBYTE pThunkAddr = NULL;

    while(NULL != pThunks)
    {
        DWORD dwStartSlot = pThunks->_dwStartThunk;
        DWORD dwCurrSlot = pThunks->_dwStartThunk;
        DWORD dwMaxSlot = pThunks->_dwCurrentThunk;        
        while (dwCurrSlot < dwMaxSlot)
        {
            LPBYTE pStartAddr =  pThunks->ThunkCode[dwCurrSlot-dwStartSlot].pCode;
            if((stubStartAddress >= pStartAddr) &&
               (stubStartAddress <  (pStartAddr + ConstVirtualThunkSize)))
            {
                pThunkAddr = pStartAddr;
                break;
            }            
            ++dwCurrSlot;
        }

        pThunks = pThunks->GetNextThunk();            
     }

     return pThunkAddr;
}

//+----------------------------------------------------------------------------
//
//  Method:     CNonVirtualThunkMgr::InitNonVirtualThunkManager   public
//
//  Synopsis:   Adds the stub manager to aid debugger in stepping into calls
//              
//+----------------------------------------------------------------------------
void CNonVirtualThunkMgr::InitNonVirtualThunkManager()
{   
    THROWSCOMPLUSEXCEPTION();

    // This function is already thread safe since this method is called from within a 
    // critical section
    if(NULL == s_pNonVirtualThunkMgr)
    {
        // Add the stub manager for non vtable calls
        s_pNonVirtualThunkMgr = new CNonVirtualThunkMgr();
        if (s_pNonVirtualThunkMgr == NULL)
        {
            COMPlusThrowOM();
        }
        
        StubManager::AddStubManager(s_pNonVirtualThunkMgr);
    }
}

//+----------------------------------------------------------------------------
//
//  Method:     CNonVirtualThunkMgr::Cleanup   public
//
//  Synopsis:   Removes the stub manager that aids the debugger 
//              
//+----------------------------------------------------------------------------
void CNonVirtualThunkMgr::Cleanup()
{
    if(s_pNonVirtualThunkMgr)
    {
        delete s_pNonVirtualThunkMgr;
        s_pNonVirtualThunkMgr = NULL;
    }
}

//+----------------------------------------------------------------------------
//
//  Method:     CNonVirtualThunkMgr::CheckIsStub   public
//
//  Synopsis:   Returns TRUE if the given address is the starting address of
//              one of our thunks
//+----------------------------------------------------------------------------
BOOL CNonVirtualThunkMgr::CheckIsStub(const BYTE *stubStartAddress)
{
    BOOL bIsStub = FALSE;

    if(NULL != FindThunk(stubStartAddress))
    {
        bIsStub = TRUE;       
    }

    return bIsStub;
}

//+----------------------------------------------------------------------------
//
//  Method:     CNonVirtualThunkMgr::Entry2MethodDesc   public
//
//  Synopsis:   Convert a starting address to a MethodDesc
//+----------------------------------------------------------------------------
MethodDesc *CNonVirtualThunkMgr::Entry2MethodDesc(const BYTE *StubStartAddress, MethodTable *pMT)
{
    if (IsThunkByASM(StubStartAddress) && CheckIsStub(StubStartAddress))
    {
        return GetMethodDescByASM(StubStartAddress);
    }
    else
    {
        return NULL;
    }
}

//+----------------------------------------------------------------------------
//
//  Method:     CNonVirtualThunkMgr::FindThunk   private
//
//  Synopsis:   Finds a thunk that matches the given starting address
//+----------------------------------------------------------------------------
CNonVirtualThunk* CNonVirtualThunkMgr::FindThunk(const BYTE *stubStartAddress)
{
    CNonVirtualThunk* pThunk = CNonVirtualThunk::GetNonVirtualThunks();

     while(NULL != pThunk)
     {
        if(stubStartAddress == pThunk->GetThunkCode())           
        {
            break;
        }
        pThunk = pThunk->GetNextThunk();            
     }

     return pThunk;
}


//+----------------------------------------------------------------------------
//+- HRESULT MethodDescDispatchHelper(MethodDesc* pMD, ARG_SLOT[] args, ARG_SLOT *pret)
//+----------------------------------------------------------------------------
HRESULT MethodDescDispatchHelper(MethodDesc* pMD, BinderMethodID sigID, ARG_SLOT args[], ARG_SLOT *pret)
{
    _ASSERTE(pMD != NULL);
    _ASSERTE(pret != NULL);
    _ASSERTE(args != NULL);

    // Setup the thread object.
    Thread *pThread = SetupThread();

    // SetupThread will return NULL if memory is exhausted
    // or if there is some initialization problem
    if (!pThread)
        return E_FAIL;
    
    BOOL fGCDisabled = pThread->PreemptiveGCDisabled();
    if (!fGCDisabled)
    {
        pThread->DisablePreemptiveGC();
    }

    HRESULT hr = S_OK;

    COMPLUS_TRY
    {
        *pret = pMD->Call(args, sigID);
    }
    COMPLUS_CATCH
    {
        hr = SetupErrorInfo(GETTHROWABLE());
    }
    COMPLUS_END_CATCH

    if (!fGCDisabled)
    {
        pThread->EnablePreemptiveGC();
    }
    return hr;
}


