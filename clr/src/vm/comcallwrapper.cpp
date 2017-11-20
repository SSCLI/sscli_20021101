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
//--------------------------------------------------------------------------
// wrapper.cpp
//
// Implementation for various Wrapper classes
//
//  COMWrapper      : COM callable wrappers for COM+ interfaces
//  ContextWrapper  : Wrappers that intercept cross context calls
//--------------------------------------------------------------------------

#include "common.h"
#include "comcallwrapper.h"
#include "remoting.h"


#include "olevariant.h"
#include "comstring.h"

AppDomain * ComCallWrapper::GetDomainSynchronized()
{
    return SystemDomain::System()->GetAppDomainAtId(m_dwDomainId);
}

STDMETHODIMP ComCallWrapper::QueryInterface(REFIID riid, void** ppv)
{
    if ((riid == IID_IUnknown) || (riid == IID_IManagedInstanceWrapper))
    {
        *ppv = (IManagedInstanceWrapper*)this;
    }
    else 
    {
        // If you are hitting this assert, you need change your code to 
        // use IManagedInstanceWrapper                                  probably
        _ASSERT(false);

        *ppv = NULL;
        return E_NOINTERFACE;
    }

    AddRef();   
    return S_OK;
}

STDMETHODIMP_(ULONG) ComCallWrapper::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}
 
STDMETHODIMP_(ULONG) ComCallWrapper::Release(void)
{
    ULONG cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0)
        delete this;
    return cRef;   
}

static MethodDesc* GetInvokeMemberMD()
{
    static MethodDesc* s_pInvokeMemberMD = NULL;

    // If we already have retrieved the specified MD then just return it.
    if (s_pInvokeMemberMD == NULL)
    {
        // The method desc has not been retrieved yet so find it.
        MethodDesc *pMD = g_Mscorlib.GetMethod(METHOD__CLASS__INVOKE_MEMBER);

        // Ensure that the value types in the signature are loaded.
        MetaSig::EnsureSigValueTypesLoaded(pMD->GetSig(), pMD->GetModule());

        // Cache the method desc.
        s_pInvokeMemberMD = pMD;
    }

    // Return the specified method desc.
    return s_pInvokeMemberMD;
}

static OBJECTREF GetReflectionObject(EEClass* pClass)
{
    return pClass->GetExposedClassObject();
}

static OBJECTHANDLE s_hndOleAutBinder;

static OBJECTREF GetOleAutBinder()
{
    THROWSCOMPLUSEXCEPTION();

    // If we have already create the instance of the OleAutBinder then simply return it.
    if (s_hndOleAutBinder)
        return ObjectFromHandle(s_hndOleAutBinder);

    MethodTable *pOleAutBinderClass = g_Mscorlib.GetClass(CLASS__OLE_AUT_BINDER);

    // Allocate an instance of the OleAutBinder class.
    OBJECTREF OleAutBinder = AllocateObject(pOleAutBinderClass);

    // Keep a handle to the OleAutBinder instance.
    s_hndOleAutBinder = CreateGlobalHandle(OleAutBinder);

    return OleAutBinder;
}

VOID ComCallWrapper::InvokeByNameCallback(LPVOID ptr)
{
    InvokeByNameArgs* args = (InvokeByNameArgs*)ptr;
    INT32 NumByrefArgs = 0;
    INT32 *aByrefArgMngVariantIndex = NULL;
    INT32 iArg;
    struct __gc {
        OBJECTREF Target;
        STRINGREF MemberName;
        PTRARRAYREF ParamArray;
        OBJECTREF TmpObj;
        OBJECTREF RetVal;
    } gc;
    ZeroMemory(&gc, sizeof(gc));

    GCPROTECT_BEGIN(gc);

    gc.MemberName = COMString::NewString(args->MemberName);

    gc.ParamArray = (PTRARRAYREF)AllocateObjectArray(args->ArgCount, g_pObjectClass);

    //
    // Fill in the arguments.
    //

    for (iArg = 0; iArg < args->ArgCount; iArg++)
    {
        // Convert the variant.
        VARIANT *pSrcOleVariant = &args->ArgList[iArg];
        OleVariant::MarshalObjectForOleVariant(pSrcOleVariant, &gc.TmpObj);
        gc.ParamArray->SetAt(iArg, gc.TmpObj);

        // If the argument is byref then add it to the array of byref arguments.
        if (V_VT(pSrcOleVariant) & VT_BYREF)
        {
            if (aByrefArgMngVariantIndex == NULL) {
                aByrefArgMngVariantIndex = (INT32 *)_alloca(sizeof(INT32) * args->ArgCount);
            }

            aByrefArgMngVariantIndex[NumByrefArgs] = iArg;
            NumByrefArgs++;
        }
    }

    gc.Target = ObjectFromHandle(args->pThis->m_hThis);

    //
    // Invoke using IReflect::InvokeMember
    //

    EEClass *pClass = gc.Target->GetClass();

    // Retrieve the method descriptor that will be called on.
    MethodDesc *pMD = GetInvokeMemberMD();

    // Prepare the arguments that will be passed to Invoke.
    ARG_SLOT Args[] = {
            ObjToArgSlot(GetReflectionObject(pClass)), // IReflect
            ObjToArgSlot(gc.MemberName),    // name
            (ARG_SLOT) args->BindingFlags,  // invokeAttr
            ObjToArgSlot(GetOleAutBinder()),// binder
            ObjToArgSlot(gc.Target),        // target
            ObjToArgSlot(gc.ParamArray),    // args
            ObjToArgSlot(NULL),             // modifiers
            ObjToArgSlot(NULL),             // culture
            ObjToArgSlot(NULL)              // namedParameters
    };

    // Do the actual method invocation.
    MetaSig metaSig(pMD->GetSig(),pMD->GetModule());
    gc.RetVal = ArgSlotToObj(pMD->Call(Args, &metaSig));

    //
    // Convert the return value and the byref arguments.
    //

    // Convert all the ByRef arguments back.
    for (iArg = 0; iArg < NumByrefArgs; iArg++)
    {
        INT32 i = aByrefArgMngVariantIndex[iArg];
        gc.TmpObj = gc.ParamArray->GetAt(i);
        OleVariant::MarshalOleRefVariantForObject(&gc.TmpObj, &args->ArgList[i]);
    }

    // Convert the return COM+ object to an OLE variant.
    if (args->pRetVal)
        OleVariant::MarshalOleVariantForObject(&gc.RetVal, args->pRetVal);

    GCPROTECT_END();
}

STDMETHODIMP ComCallWrapper::InvokeByName(LPCWSTR MemberName, INT32 BindingFlags, 
    INT32 ArgCount, VARIANT *ArgList, VARIANT *pRetVal)
{
    HRESULT hr = S_OK;
    InvokeByNameArgs args;    

    if (pRetVal)
        VariantClear(pRetVal);

    Thread* pThread = SetupThread();
    if (pThread == NULL)
        return E_OUTOFMEMORY;

    BEGINCANNOTTHROWCOMPLUSEXCEPTION();
    BEGIN_ENSURE_COOPERATIVE_GC();
    
    args.pThis = this;
    args.MemberName = MemberName;
    args.BindingFlags = BindingFlags;
    args.ArgCount = ArgCount;
    args.ArgList = ArgList;
    args.pRetVal = pRetVal;

    COMPLUS_TRY {

        // go through AppDomain callback if we are running in wrong AppDomain
        if (m_dwDomainId != pThread->GetDomain()->GetId()) {
            pThread->DoADCallBack(m_pContext, InvokeByNameCallback, &args);
        }
        else {
            InvokeByNameCallback(&args);
        }

    } COMPLUS_CATCH {
        hr = SetupErrorInfo(GETTHROWABLE());
    } COMPLUS_END_CATCH

    END_ENSURE_COOPERATIVE_GC();
    ENDCANNOTTHROWCOMPLUSEXCEPTION();

    return hr;
}

IUnknown* ComCallWrapper::GetComIPFromObjectRef(OBJECTREF* poref)
{
    THROWSCOMPLUSEXCEPTION();
    ComCallWrapper *pWrapper = new (throws) ComCallWrapper();
    pWrapper->Init(poref);
    return pWrapper;
}

OBJECTREF ComCallWrapper::GetObjectRefFromComIP(IUnknown* pUnk)
{
    ComCallWrapper *pWrapper;

    THROWSCOMPLUSEXCEPTION();
    _ASSERTE(pUnk != NULL);
    
    // check whether the IUnknown* is comming from our wrapper
    if (*(PVOID*)pUnk != GetComCallWrapperVPtr())
        COMPlusThrow(kInvalidCastException);

    pWrapper = GetWrapperFromIP(pUnk);      
    return ObjectFromHandle(pWrapper->m_hThis);
}



// if the object we are creating is a proxy to another appdomain, want to create the wrapper for the
// new object in the appdomain of the proxy target
Context* ComCallWrapper::GetExecutionContext(OBJECTREF pObj, OBJECTREF* pServer )
{
    Context *pContext = NULL;

    if (pObj->GetMethodTable()->IsTransparentProxyType()) 
        pContext = CRemotingServices::GetServerContextForProxy(pObj);

    if (pContext == NULL)
        pContext = GetAppDomain()->GetDefaultContext();

    return pContext;
}
