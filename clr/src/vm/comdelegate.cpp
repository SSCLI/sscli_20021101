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
////////////////////////////////////////////////////////////////////////////////
// This module contains the implementation of the native methods for the
//  Delegate class.
//
// Date: June 1998
////////////////////////////////////////////////////////////////////////////////
#include "common.h"
#include "comdelegate.h"
#include "comclass.h"
#include "invokeutil.h"
#include "commember.h"
#include "excep.h"
#include "class.h"
#include "field.h"
#include "utsem.h"
#include "nexport.h"
#include "ndirect.h"
#include "remoting.h"

FieldDesc*  COMDelegate::m_pORField = 0;    // Object reference field...
FieldDesc*  COMDelegate::m_pFPField = 0;    // Function Pointer Address field...
FieldDesc*  COMDelegate::m_pFPAuxField = 0; // Aux Function Pointer field
FieldDesc*  COMDelegate::m_pPRField = 0;    // Prev delegate field (Multicast)
FieldDesc*  COMDelegate::m_pMethInfoField = 0;  // Method Info
FieldDesc*  COMDelegate::m_ppNextField = 0;  // pNext info
ShuffleThunkCache *COMDelegate::m_pShuffleThunkCache = NULL; 
ArgBasedStubCache *COMDelegate::m_pMulticastStubCache = NULL;

MethodTable* COMDelegate::s_pIAsyncResult = 0;
MethodTable* COMDelegate::s_pAsyncCallback = 0;

VOID GenerateShuffleArray(PCCOR_SIGNATURE pSig,
                          Module*         pModule,
                          ShuffleEntry   *pShuffleEntryArray);

class ShuffleThunkCache : public MLStubCache
{
    private:
        //---------------------------------------------------------
        // Compile a static delegate shufflethunk. Always returns
        // STANDALONE since we don't interpret these things.
        //---------------------------------------------------------
        virtual MLStubCompilationMode CompileMLStub(const BYTE *pRawMLStub,
                                                    StubLinker *pstublinker,
                                                    void *callerContext)
        {
            MLStubCompilationMode ret = INTERPRETED;
            COMPLUS_TRY
            {

                ((CPUSTUBLINKER*)pstublinker)->EmitShuffleThunk((ShuffleEntry*)pRawMLStub);
                ret = STANDALONE;
            }
            COMPLUS_CATCH
            {
                // In case of an error, we'll just leave the mode as "INTERPRETED."
                // and let the caller of Canonicalize() treat that as an error.
            }
            COMPLUS_END_CATCH
            return ret;
        }

        //---------------------------------------------------------
        // Tells the MLStubCache the length of a ShuffleEntryArray.
        //---------------------------------------------------------
        virtual UINT Length(const BYTE *pRawMLStub)
        {
            ShuffleEntry *pse = (ShuffleEntry*)pRawMLStub;
            while (pse->srcofs != pse->SENTINEL)
            {
                pse++;
            }
            return sizeof(ShuffleEntry) * (UINT)(1 + (pse - (ShuffleEntry*)pRawMLStub));
        }


};

// One time init.
BOOL COMDelegate::Init()
{
    if (NULL == (m_pShuffleThunkCache = new ShuffleThunkCache())) 
    { 
        return FALSE; 
    } 
    if (NULL == (m_pMulticastStubCache = new ArgBasedStubCache())) 
    { 
        return FALSE; 
    }

    return TRUE;
}

// Termination

Stub* COMDelegate::SetupShuffleThunk(DelegateEEClass *pDelCls)
{
    Stub* pShuffleThunk;

    MethodDesc *pInvokeMethod = pDelCls->m_pInvokeMethod;
    UINT allocsize = sizeof(ShuffleEntry) * (3+pInvokeMethod->SizeOfVirtualFixedArgStack()/STACK_ELEM_SIZE); 

#ifndef _DEBUG
    // This allocsize prediction is easy to break, so in retail, add
    // some fudge to be safe.
    allocsize += 3*sizeof(ShuffleEntry);
#endif

    ShuffleEntry *pShuffleEntryArray = (ShuffleEntry*)_alloca(allocsize);

#ifdef _DEBUG
    FillMemory(pShuffleEntryArray, allocsize, 0xcc);
#endif
    GenerateShuffleArray(pInvokeMethod->GetSig(), 
                            pInvokeMethod->GetModule(), 
                            pShuffleEntryArray);
    MLStubCache::MLStubCompilationMode mode;
    pShuffleThunk = m_pShuffleThunkCache->Canonicalize((const BYTE *)pShuffleEntryArray, &mode);
    if (!pShuffleThunk || mode != MLStubCache::STANDALONE) {
        COMPlusThrowOM();
    }

    if (FastInterlockCompareExchangePointer( (void*volatile*) &(pDelCls->m_pStaticShuffleThunk),
                                        pShuffleThunk,
                                        NULL ) != NULL) {
        pShuffleThunk->DecRef();
        pShuffleThunk = pDelCls->m_pStaticShuffleThunk;
    }

    return pShuffleThunk;
}

void COMDelegate::InitFields()
{
    if (m_pORField == NULL)
    {
        m_pORField = g_Mscorlib.GetField(FIELD__DELEGATE__TARGET);
        m_pFPField = g_Mscorlib.GetField(FIELD__DELEGATE__METHOD_PTR);
        m_pFPAuxField = g_Mscorlib.GetField(FIELD__DELEGATE__METHOD_PTR_AUX);
        m_pPRField = g_Mscorlib.GetField(FIELD__MULTICAST_DELEGATE__NEXT);
        m_ppNextField = g_Mscorlib.GetField(FIELD__MULTICAST_DELEGATE__NEXT);
        m_pMethInfoField = g_Mscorlib.GetField(FIELD__DELEGATE__METHOD);
    }
}

// InternalCreate
// Internal Create is called from the constructor.  It does the internal
//  initialization of the Delegate.
FCIMPL4(void, COMDelegate::InternalCreate, ReflectBaseObject* refThisUNSAFE, Object* targetUNSAFE, StringObject* methodNameUNSAFE, BYTE ignoreCase)
{
    struct _gc
    {
        REFLECTBASEREF refThis;
        OBJECTREF target;
        STRINGREF methodName;
    } gc;

    gc.refThis    = (REFLECTBASEREF) refThisUNSAFE;
    gc.target     = (OBJECTREF) targetUNSAFE;
    gc.methodName = (STRINGREF) methodNameUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_NOPOLL();
    GCPROTECT_BEGIN(gc);
    HELPER_METHOD_POLL();
    //-[autocvtpro]-------------------------------------------------------
    
    WCHAR* method;
    EEClass* pVMC;
    EEClass* pDelEEC;
    MethodDesc* pMeth;

    THROWSCOMPLUSEXCEPTION();
    COMClass::EnsureReflectionInitialized();
    InitFields();

    method = gc.methodName->GetBuffer();

    // get the signature of the 
    pDelEEC = gc.refThis->GetClass();
    MethodDesc* pInvokeMeth = FindDelegateInvokeMethod(pDelEEC);

    pVMC = gc.target->GetTrueClass();
    _ASSERTE(pVMC);

    // Convert the signature and find it for this object  
    // We don't throw exceptions from this block because of the CQuickBytes
    //  has a destructor.

    // Convert the method name to UTF8
    // Allocate a buffer twice the size of the length
    WCHAR* wzStr = gc.methodName->GetBuffer();
    int len = gc.methodName->GetStringLength();
    _ASSERTE(wzStr);
    _ASSERTE(len >= 0);

    int cStr = len * 2;
    LPUTF8 szNameStr = (LPUTF8) _alloca((cStr+1) * sizeof(char));
    cStr = WszWideCharToMultiByte(CP_UTF8, 0, wzStr, len, szNameStr, cStr-1, NULL, NULL);
    szNameStr[cStr] = 0;

    // Convert the signatures and find the method.
    PCCOR_SIGNATURE pSignature; // The signature of the found method
    DWORD cSignature;
	if(pInvokeMeth) {
        pInvokeMeth->GetSig(&pSignature,&cSignature);
        pMeth = pVMC->FindMethod(szNameStr, pSignature, cSignature,pInvokeMeth->GetModule(), NULL, !ignoreCase);
	}
	else
        pMeth = NULL;

    // The method wasn't found or is a static method we need to throw an exception
    if (!pMeth || pMeth->IsStatic())
        COMPlusThrow(kArgumentException,L"Arg_DlgtTargMeth");

    RefSecContext sCtx;
    sCtx.SetClassOfInstance(pVMC);
    InvokeUtil::CheckAccess(&sCtx,
                            pMeth->GetAttrs(),
                            pMeth->GetMethodTable(),
                            REFSEC_CHECK_MEMBERACCESS|REFSEC_THROW_MEMBERACCESS);
    InvokeUtil::CheckLinktimeDemand(&sCtx,
                                    pMeth,
                                    true);

    m_pORField->SetRefValue((OBJECTREF)gc.refThis, gc.target);

    if (pMeth->IsVirtual())
        m_pFPField->SetValuePtr((OBJECTREF)gc.refThis, (void*)pMeth->GetAddrofCode(gc.target));
    else
        m_pFPField->SetValuePtr((OBJECTREF)gc.refThis, (void*)pMeth->GetAddrofCodeNonVirtual()); 

    //-[autocvtepi]-------------------------------------------------------
    GCPROTECT_END();
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

// InternalCreateStatic
// Internal Create is called from the constructor. The method must
//  be a static method.
FCIMPL3(void, COMDelegate::InternalCreateStatic, ReflectBaseObject* refThisUNSAFE, ReflectClassBaseObject* targetUNSAFE, StringObject* methodNameUNSAFE)
{
    struct _gc
    {
        REFLECTBASEREF refThis;
        REFLECTCLASSBASEREF target;
        STRINGREF methodName;
    } gc;

    gc.refThis = (REFLECTBASEREF) refThisUNSAFE;
    gc.target = (REFLECTCLASSBASEREF) targetUNSAFE;
    gc.methodName = (STRINGREF) methodNameUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_NOPOLL();
    GCPROTECT_BEGIN(gc);
    HELPER_METHOD_POLL();
    //-[autocvtpro]-------------------------------------------------------

    WCHAR* method;
    EEClass* pDelEEC;
    EEClass* pEEC;

    THROWSCOMPLUSEXCEPTION();
    COMClass::EnsureReflectionInitialized();
    InitFields();

    method = gc.methodName->GetBuffer();


    // get the signature of the 
    pDelEEC = gc.refThis->GetClass();
    MethodDesc* pInvokeMeth = FindDelegateInvokeMethod(pDelEEC);
    _ASSERTE(pInvokeMeth);

    ReflectClass* pRC = (ReflectClass*) gc.target->GetData();
    _ASSERTE(pRC);

    // Convert the method name to UTF8
    // Allocate a buffer twice the size of the length
    WCHAR* wzStr = gc.methodName->GetBuffer();
    int len = gc.methodName->GetStringLength();
    _ASSERTE(wzStr);
    _ASSERTE(len >= 0);

    int cStr = len * 2;
    LPUTF8 szNameStr = (LPUTF8) _alloca((cStr+1) * sizeof(char));
    cStr = WszWideCharToMultiByte(CP_UTF8, 0, wzStr, len, szNameStr, cStr-1, NULL, NULL);
    szNameStr[cStr] = 0;

    // Convert the signatures and find the method.
    PCCOR_SIGNATURE pInvokeSignature; // The signature of the found method
    DWORD cSignature;
    pInvokeMeth->GetSig(&pInvokeSignature,&cSignature);

    // Invoke has the HASTHIS bit set, we have to unset it 
    PCOR_SIGNATURE pSignature = (PCOR_SIGNATURE) _alloca(cSignature);
    memcpy(pSignature, pInvokeSignature, cSignature);
    *pSignature &= ~IMAGE_CEE_CS_CALLCONV_HASTHIS;  // This is a static delegate, 


    pEEC = pRC->GetClass();
    MethodDesc* pMeth = pEEC->FindMethod(szNameStr, pSignature, cSignature, 
                                         pInvokeMeth->GetModule());
    if (!pMeth || !pMeth->IsStatic())
        COMPlusThrow(kArgumentException,L"Arg_DlgtTargMeth");

    RefSecContext sCtx;
    InvokeUtil::CheckAccess(&sCtx,
                            pMeth->GetAttrs(),
                            pMeth->GetMethodTable(),
                            REFSEC_CHECK_MEMBERACCESS|REFSEC_THROW_MEMBERACCESS);
    InvokeUtil::CheckLinktimeDemand(&sCtx,
                                    pMeth,
                                    true);

    m_pORField->SetRefValue((OBJECTREF)gc.refThis, (OBJECTREF)gc.refThis);
    m_pFPAuxField->SetValuePtr((OBJECTREF)gc.refThis, pMeth->GetPreStubAddr());


    DelegateEEClass *pDelCls = (DelegateEEClass*) pDelEEC;
    Stub *pShuffleThunk = pDelCls->m_pStaticShuffleThunk;
    if (!pShuffleThunk) {
        pShuffleThunk = SetupShuffleThunk(pDelCls);
    }


    m_pFPField->SetValuePtr((OBJECTREF)gc.refThis,(void*)(pShuffleThunk->GetEntryPoint()));

    //-[autocvtepi]-------------------------------------------------------
    GCPROTECT_END();
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND


// FindDelegateInvokeMethod
//
// Finds the compiler-generated "Invoke" method for delegates. pcls
// must be derived from the "Delegate" class.
//
MethodDesc * COMDelegate::FindDelegateInvokeMethod(EEClass *pcls)
{
    THROWSCOMPLUSEXCEPTION();

    if (!pcls->IsAnyDelegateClass())
        return NULL;

    DelegateEEClass *pDcls = (DelegateEEClass *) pcls;

    if (pDcls->m_pInvokeMethod == NULL) {
        COMPlusThrowNonLocalized(kMissingMethodException, L"Invoke");
        return NULL;
    }
    else
        return pDcls->m_pInvokeMethod;
}


// Marshals a delegate to a unmanaged callback.
LPVOID COMDelegate::ConvertToCallback(OBJECTREF pDelegate)
{
    THROWSCOMPLUSEXCEPTION();

    if (!pDelegate) {
        return NULL;
    } else {

        LPVOID pCode;
        GCPROTECT_BEGIN(pDelegate);

        _ASSERTE(4 == sizeof(UMEntryThunk*));

        UMEntryThunk *pUMEntryThunk = NULL;
        SyncBlock *pSyncBlock = pDelegate->GetSyncBlockSpecial();
        if (pSyncBlock != NULL)
            pUMEntryThunk = (UMEntryThunk*)pSyncBlock->GetUMEntryThunk();
        if (!pUMEntryThunk) {

            MethodDesc *pMeth = GetMethodDesc(pDelegate);

            DelegateEEClass *pcls = (DelegateEEClass*)(pDelegate->GetClass());
            UMThunkMarshInfo *pUMThunkMarshInfo = pcls->m_pUMThunkMarshInfo;
            MethodDesc *pInvokeMeth = FindDelegateInvokeMethod(pcls);

            if (!pUMThunkMarshInfo) {
                pUMThunkMarshInfo = new UMThunkMarshInfo();
                if (!pUMThunkMarshInfo) {
                    COMPlusThrowOM();
                }

                PCCOR_SIGNATURE pSig;
                DWORD cSig;
                pInvokeMeth->GetSig(&pSig, &cSig);

                CorPinvokeMap unmanagedCallConv = MetaSig::GetUnmanagedCallingConvention(pInvokeMeth->GetModule(), pSig, cSig);
                if (unmanagedCallConv == (CorPinvokeMap)0 || unmanagedCallConv == (CorPinvokeMap)pmCallConvWinapi)
                {
                    unmanagedCallConv = pInvokeMeth->IsVarArg() ? pmCallConvCdecl : pmCallConvStdcall;
                }

                pUMThunkMarshInfo->CompleteInit(pSig, cSig, pInvokeMeth->GetModule(), pInvokeMeth->IsStatic(), nltAnsi, unmanagedCallConv, pInvokeMeth->GetMemberDef());            


                if (FastInterlockCompareExchangePointer( (void*volatile*) &(pcls->m_pUMThunkMarshInfo),
                                                   pUMThunkMarshInfo,
                                                   NULL ) != NULL) {
                    delete pUMThunkMarshInfo;
                    pUMThunkMarshInfo = pcls->m_pUMThunkMarshInfo;
                }
            }

            _ASSERTE(pUMThunkMarshInfo != NULL);
            _ASSERTE(pUMThunkMarshInfo == pcls->m_pUMThunkMarshInfo);


            pUMEntryThunk = UMEntryThunk::CreateUMEntryThunk();
            if (!pUMEntryThunk) {
                COMPlusThrowOM();
            }
            OBJECTHANDLE objhnd = NULL;
            BOOL fSuccess = FALSE;
            EE_TRY_FOR_FINALLY {
                if (pInvokeMeth->GetClass()->IsSingleDelegateClass()) {

                    
                    // singlecast delegate: just go straight to target method
                    if (NULL == (objhnd = GetAppDomain()->CreateLongWeakHandle(m_pORField->GetRefValue(pDelegate)))) {
                        COMPlusThrowOM();
                    }
                    if (pMeth->IsIL() &&
                        !(pMeth->IsStatic()) &&
                        pMeth->IsJitted()) {
                        // MethodDesc is passed in for profiling to know the method desc of target
                        pUMEntryThunk->CompleteInit(NULL, objhnd,    
                                                    pUMThunkMarshInfo, pMeth,
                                                    GetAppDomain()->GetId());
                    } else {
                        // Pass in NULL as last parameter to indicate no associated MethodDesc
                        pUMEntryThunk->CompleteInit((const BYTE *)(m_pFPField->GetValuePtr(pDelegate)),
                                                    objhnd, pUMThunkMarshInfo, NULL,
                                                    GetAppDomain()->GetId());
                    }
                } else {
                    // multicast. go thru Invoke
                    if (NULL == (objhnd = GetAppDomain()->CreateLongWeakHandle(pDelegate))) {
                        COMPlusThrowOM();
                    }
                    // MethodDesc is passed in for profiling to know the method desc of target
                    pUMEntryThunk->CompleteInit((const BYTE *)(pInvokeMeth->GetPreStubAddr()), objhnd,
                                                pUMThunkMarshInfo, pInvokeMeth,
                                                GetAppDomain()->GetId());
                }
                                fSuccess = TRUE;
            } EE_FINALLY {
                if (!fSuccess && objhnd != NULL) {
                    DestroyLongWeakHandle(objhnd);
                }
            } EE_END_FINALLY
            
            if (!pSyncBlock->SetUMEntryThunk(pUMEntryThunk)) {
                UMEntryThunk::FreeUMEntryThunk(pUMEntryThunk);
                pUMEntryThunk = (UMEntryThunk*)pSyncBlock->GetUMEntryThunk();
            }

            _ASSERTE(pUMEntryThunk != NULL);
            _ASSERTE(pUMEntryThunk == (UMEntryThunk*)(UMEntryThunk*)pSyncBlock->GetUMEntryThunk()); 

        }
        pCode = (LPVOID)pUMEntryThunk->GetCode();
        GCPROTECT_END();
        return pCode;
    }
}


// Marshals an unmanaged callback to Delegate
OBJECTREF COMDelegate::ConvertToDelegate(LPVOID pCallback)
{
    THROWSCOMPLUSEXCEPTION();

    if (!pCallback) {
        return NULL;
    }

    UMEntryThunk *pUMEntryThunk = UMEntryThunk::Decode(pCallback);
    if (!pUMEntryThunk) {
        COMPlusThrow(kArgumentException, IDS_EE_NOTADELEGATE);
    }
    return ObjectFromHandle(pUMEntryThunk->GetObjectHandle());
}

// This is the single constructor for all Delegates.  The compiler
//  doesn't provide an implementation of the Delegate constructor.  We
//  provide that implementation through an ECall call to this method.
FCIMPL3(void, COMDelegate::DelegateConstruct, ReflectBaseObject* refThisUNSAFE, Object* targetUNSAFE, SLOT method)
{
    struct _gc
    {
        REFLECTBASEREF refThis;
        OBJECTREF target;
    } gc;

    gc.refThis = (REFLECTBASEREF) refThisUNSAFE;
    gc.target  = (OBJECTREF) targetUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_NOPOLL();
    GCPROTECT_BEGIN(gc);
    HELPER_METHOD_POLL();

    THROWSCOMPLUSEXCEPTION();

    InitFields();

    //                         programmers could feed garbage data to DelegateConstruct().
    // It's difficult to validate a method code pointer, but at least we'll
    // try to catch the easy garbage.
    _ASSERTE(isMemoryReadable(method,1));

    _ASSERTE(gc.refThis);
    _ASSERTE(method);
    
    MethodTable *pMT = NULL;
    MethodTable *pRealMT = NULL;

    if (gc.target != NULL)
    {
        pMT = gc.target->GetMethodTable();
        pRealMT = pMT->AdjustForThunking(gc.target);
    }

    MethodDesc *pMeth = Entry2MethodDesc((BYTE*)method, pRealMT);

    //
    // If target is a contextful class, then it must be a proxy
    //    
    _ASSERTE((NULL == pMT) || pMT->IsTransparentProxyType() || !pRealMT->IsContextful());

    EEClass* pDel = gc.refThis->GetClass();
    // Make sure we call the <cinit>
    OBJECTREF Throwable;
    if (!pDel->DoRunClassInit(&Throwable)) {
        COMPlusThrow(Throwable);
    }

    _ASSERTE(pMeth);

#ifdef _DEBUG
    // Assert that everything is OK...This is not some bogus
    //  address...Very unlikely that the code below would work
    //  for a random address in memory....
    MethodTable* p = pMeth->GetMethodTable();
    _ASSERTE(p);
    EEClass* cls = pMeth->GetClass();
    _ASSERTE(cls);
    _ASSERTE(cls == p->GetClass());
#endif

    // Static method.
    if (!gc.target || pMeth->IsStatic()) 
    {
        // if this is a not a static method throw...
        if (!pMeth->IsStatic())
            COMPlusThrow(kNullReferenceException,L"Arg_DlgtTargMeth");

        m_pORField->SetRefValue((OBJECTREF)gc.refThis, (OBJECTREF)gc.refThis);
        m_pFPAuxField->SetValuePtr((OBJECTREF)gc.refThis, pMeth->GetPreStubAddr());

        DelegateEEClass *pDelCls = (DelegateEEClass*)(gc.refThis->GetClass());
        Stub *pShuffleThunk = pDelCls->m_pStaticShuffleThunk;
        if (!pShuffleThunk) {
            pShuffleThunk = SetupShuffleThunk(pDelCls);
        }

        m_pFPField->SetValuePtr((OBJECTREF)gc.refThis, (void*)pShuffleThunk->GetEntryPoint());
    }
    else 
    {
        EEClass* pTarg = gc.target->GetClass();
        EEClass* pMethClass = pMeth->GetClass();

        if (!pMT->IsThunking())
        {
            if (pMethClass != pTarg) {
                //They cast to an interface before creating the delegate, so we now need 
                //to figure out where this actually lives before we continue.
                if (pMethClass->IsInterface())  {
                    // No need to resolve the interface based method desc to a class based
                    // one for COM objects because we invoke directly thru the interface MT.
                    if (!pTarg->GetMethodTable()->IsComObjectType())
                    {
                        DWORD cSig=1024;
                        PCCOR_SIGNATURE sig = (PCCOR_SIGNATURE)_alloca(cSig);
                        pMeth->GetSig(&sig, &cSig);
                        pMeth = pTarg->FindMethod(pMeth->GetName(),sig,cSig,pMeth->GetModule());
                    }
                }
            }

            // Use the Unboxing stub for value class methods, since the value
            // class is constructed using the boxed instance.
    
            if (pTarg->IsValueClass() && !pMeth->IsUnboxingStub())
            {
                // If these are Object/ValueType.ToString().. etc,
                // don't need an unboxing Stub.

                if ((pMethClass != g_pValueTypeClass->GetClass()) 
                    && (pMethClass != g_pObjectClass->GetClass()))
                {
                    pMeth = pTarg->GetUnboxingMethodDescForValueClassMethod(pMeth);


                    if (pMeth == NULL)
		      COMPlusThrow(kNotSupportedException, L"NotSupported_NonVirtualValueClassDelegates");
                }
            }

            // Set the target address of this subclass
            method = (BYTE *)(pMeth->GetUnsafeAddrofCode());
        }

        m_pORField->SetRefValue((OBJECTREF)gc.refThis, gc.target);
        m_pFPField->SetValuePtr((OBJECTREF)gc.refThis, method);
        }        
    GCPROTECT_END();
    HELPER_METHOD_FRAME_END();
    }
FCIMPLEND


// This method will validate that the target method for a delegate
//  and the delegate's invoke method have compatible signatures....
bool COMDelegate::ValidateDelegateTarget(MethodDesc* pMeth,EEClass* pDel)
{
    return true;
}
// GetMethodPtr
// Returns the FieldDesc* for the MethodPtr field
FieldDesc* COMDelegate::GetMethodPtr()
{
    if (!m_pFPField)
        InitFields();
    return m_pFPField;
}


MethodDesc *COMDelegate::GetMethodDesc(OBJECTREF orDelegate)
{
    // First, check for a static delegate
    void *code = (void *) m_pFPAuxField->GetValuePtr((OBJECTREF)orDelegate);
    if (code == NULL)
    {
        // Must be a normal delegate
        code = (void *) m_pFPField->GetValuePtr((OBJECTREF)orDelegate);

        // Weird case - need to check for a prejit vtable fixup stub.
        if (StubManager::IsStub((const BYTE *)code))
    {
            OBJECTREF orThis = m_pORField->GetRefValue(orDelegate);
            MethodDesc *pMD = StubManager::MethodDescFromEntry((const BYTE *) code, 
                                                               orThis->GetTrueMethodTable());
            if (pMD != NULL)
                return pMD;
    }
}

    return EEClass::GetUnknownMethodDescForSlotAddress((SLOT)code);
}

// GetMethodPtrAux
// Returns the FieldDesc* for the MethodPtrAux field
FieldDesc* COMDelegate::GetMethodPtrAux()
{
    if (!m_pFPAuxField)
        InitFields();

    _ASSERTE(m_pFPAuxField);
    return m_pFPAuxField;
}

// GetOR
// Returns the FieldDesc* for the Object reference field
FieldDesc* COMDelegate::GetOR()
{
    if (!m_pORField)
        InitFields();

    _ASSERTE(m_pORField);
    return m_pORField;
}

// GetpNext
// Returns the FieldDesc* for the pNext field
FieldDesc* COMDelegate::GetpNext()
{
    if (!m_ppNextField)
        InitFields();

    _ASSERTE(m_ppNextField);
    return m_ppNextField;
}

// Decides if pcls derives from Delegate.
BOOL COMDelegate::IsDelegate(EEClass *pcls)
{
    return pcls->IsAnyDelegateExact() || pcls->IsAnyDelegateClass();
}

VOID GenerateShuffleArray(PCCOR_SIGNATURE pSig,
                          Module*         pModule,
                          ShuffleEntry   *pShuffleEntryArray)
{
    THROWSCOMPLUSEXCEPTION();

#ifdef _X86_

    // Must create independent msigs to prevent the argiterators from
    // interfering with other.
    MetaSig msig1(pSig, pModule);
    MetaSig msig2(pSig, pModule);

    ArgIterator    aisrc(NULL, &msig1, FALSE);
    ArgIterator    aidst(NULL, &msig2, TRUE);

    UINT stacksizedelta = MetaSig::SizeOfActualFixedArgStack(pModule, pSig, FALSE) -
                          MetaSig::SizeOfActualFixedArgStack(pModule, pSig, TRUE);


    UINT srcregofs,dstregofs;
    INT  srcofs,   dstofs;
    UINT cbSize;
    BYTE typ;
    UINT NumRegsUsedDest = 0;

    if (msig1.HasRetBuffArg())
    {
        int offsetIntoArgumentRegisters;
        int numRegisterUsed = 1;
        // the first register is used for 'this'
        if (IsArgumentInRegister(&numRegisterUsed, ELEMENT_TYPE_PTR, sizeof(void*), FALSE,
                msig1.GetCallingConvention(), &offsetIntoArgumentRegisters))
                pShuffleEntryArray->srcofs = ShuffleEntry::REGMASK | offsetIntoArgumentRegisters;
        else
                _ASSERTE (!"ret buff arg has to be in a register");

        numRegisterUsed = 0;
        if (IsArgumentInRegister(&numRegisterUsed, ELEMENT_TYPE_PTR, sizeof(void*), FALSE,
                msig2.GetCallingConvention(), &offsetIntoArgumentRegisters))
                pShuffleEntryArray->dstofs = ShuffleEntry::REGMASK | offsetIntoArgumentRegisters;
        else
                _ASSERTE (!"ret buff arg has to be in a register");

        pShuffleEntryArray ++;
        NumRegsUsedDest++;
    }

    while (0 != (srcofs = aisrc.GetNextOffset(&typ, &cbSize, &srcregofs)) && NumRegsUsedDest < NUM_ARGUMENT_REGISTERS )
    {
        dstofs = aidst.GetNextOffset(&typ, &cbSize, &dstregofs) + stacksizedelta;

        cbSize = StackElemSize(cbSize);

        srcofs -= FramedMethodFrame::GetOffsetOfReturnAddress();
        dstofs -= FramedMethodFrame::GetOffsetOfReturnAddress();

        
        while (cbSize)
        {
            if (srcregofs == (UINT)(-1))
            {
                pShuffleEntryArray->srcofs = srcofs;
            }
            else
            {
                pShuffleEntryArray->srcofs = ShuffleEntry::REGMASK | srcregofs;
            }
            if (dstregofs == (UINT)(-1))
            {
                pShuffleEntryArray->dstofs = dstofs;
            }
            else
            {
                pShuffleEntryArray->dstofs = ShuffleEntry::REGMASK | dstregofs;

                NumRegsUsedDest++;
            }
            srcofs += STACK_ELEM_SIZE;
            dstofs += STACK_ELEM_SIZE;

            pShuffleEntryArray++;
            cbSize -= STACK_ELEM_SIZE;
        }
    }


    pShuffleEntryArray->srcofs = ShuffleEntry::SENTINEL;
    pShuffleEntryArray->dstofs = (UINT16)stacksizedelta;

#elif defined(_PPC_) || defined(_SPARC_)

    UINT cbSize;
    INT  ofs;
    BYTE typ;

    MetaSig msig(pSig, pModule);
    ArgIterator ai(NULL, &msig, FALSE);

    int numRegisterUsed = 0; // numRegisterUsed in destination
#if defined(_PPC_)
    int numFloatRegisterUsed = 0;
#endif

#if defined(_PPC_) // retbuf
    if (msig.HasRetBuffArg())
        numRegisterUsed++;
#endif

    // this pointer is special
    pShuffleEntryArray->srcofs = ShuffleEntry::REGMASK | (numRegisterUsed * STACK_ELEM_SIZE);
    pShuffleEntryArray->dstofs = (UINT16)-1;
    pShuffleEntryArray++;

    while (0 != (ofs = ai.GetNextOffset(&typ, &cbSize, NULL)))
    {
        ofs -= sizeof(FramedMethodFrame);

#if defined(_PPC_)
        // skip the enregistered float arguments
        if ((typ == ELEMENT_TYPE_R4) || (typ == ELEMENT_TYPE_R8)) {
            if (numFloatRegisterUsed  < NUM_FLOAT_ARGUMENT_REGISTERS) {
                numRegisterUsed += cbSize / STACK_ELEM_SIZE;
                numFloatRegisterUsed++;
                continue;
            }
        }
#endif
        cbSize = StackElemSize(cbSize);

        while (cbSize > 0) {
            pShuffleEntryArray->srcofs = ((numRegisterUsed+1) * STACK_ELEM_SIZE) |
                ((numRegisterUsed+1 < NUM_ARGUMENT_REGISTERS) ? ShuffleEntry::REGMASK : 0);

            pShuffleEntryArray->dstofs = (numRegisterUsed * STACK_ELEM_SIZE) |
                ((numRegisterUsed < NUM_ARGUMENT_REGISTERS) ? ShuffleEntry::REGMASK : 0);

            numRegisterUsed++;

            cbSize -= STACK_ELEM_SIZE;
            pShuffleEntryArray++;
        }
    }

    pShuffleEntryArray->srcofs = ShuffleEntry::SENTINEL;
    pShuffleEntryArray->dstofs = 0;

#else

    PORTABILITY_ASSERT("GenerateShuffleArray not implemented on this platform");

#endif
}



// Get the cpu stub for a delegate invoke.
Stub *COMDelegate::GetInvokeMethodStub(CPUSTUBLINKER *psl, EEImplMethodDesc* pMD)
{
    THROWSCOMPLUSEXCEPTION();

    DelegateEEClass *pClass = (DelegateEEClass*) pMD->GetClass();

    if (pMD == pClass->m_pInvokeMethod)
    {
        // Validate the invoke method, which at the moment just means checking the calling convention

        if (*pMD->GetSig() != (IMAGE_CEE_CS_CALLCONV_HASTHIS | IMAGE_CEE_CS_CALLCONV_DEFAULT))
            COMPlusThrow(kInvalidProgramException);

        // skip down to code for invoke
    }
    else if (pMD == pClass->m_pBeginInvokeMethod)
    {
        if (!InitializeRemoting())
        {
            _ASSERTE(!"Remoting initialization failure.");
        }
        if (!ValidateBeginInvoke(pClass))
            COMPlusThrow(kInvalidProgramException);

        Stub *ret = TheDelegateStub();
        ret->IncRef();
        return ret;
    }
    else if (pMD == pClass->m_pEndInvokeMethod)
    {
        if (!InitializeRemoting())
        {
            _ASSERTE(!"Remoting initialization failure.");
        }
        if (!ValidateEndInvoke(pClass))
            COMPlusThrow(kInvalidProgramException);
        Stub *ret = TheDelegateStub();
        ret->IncRef();
        return ret;
    }
    else
    {
        _ASSERTE(!"Bad Delegate layout");
        COMPlusThrow(kExecutionEngineException);
    }

    _ASSERTE(pMD->GetClass()->IsAnyDelegateClass());

    UINT numStackBytes = pMD->SizeOfActualFixedArgStack();
    _ASSERTE(!(numStackBytes & 3));

    UINT hash = numStackBytes;

#ifdef _PPC_
    MetaSig sig(pMD->GetSig(), pMD->GetModule());
    BOOL bHasReturnBuffer = sig.HasRetBuffArg();
    if (bHasReturnBuffer)
    {
        hash |= 2;
    }
#endif // _PPC_

    if (pMD->GetClass()->IsSingleDelegateClass())
    {
        hash |= 1;
    }

    Stub *pStub = m_pMulticastStubCache->GetStub(hash);
    if (pStub) {
        return pStub;

    } else {

        LOG((LF_CORDB,LL_INFO10000, "COMD::GIMS making a multicast delegate\n"));
        psl->EmitMulticastInvoke(numStackBytes, pMD->GetClass()->IsSingleDelegateClass()
#ifdef _PPC_
            , bHasReturnBuffer
#endif
            );

        UINT cbSize;
        Stub *pCandidate = psl->Link(SystemDomain::System()->GetStubHeap(), &cbSize, TRUE);

        Stub *pWinner = m_pMulticastStubCache->AttemptToSetStub(hash,pCandidate);
        pCandidate->DecRef();
        if (!pWinner) {
            COMPlusThrowOM();
        }
    
        LOG((LF_CORDB,LL_INFO10000, "Putting a MC stub at 0x%x (code:0x%x)\n",
            pWinner, (BYTE*)pWinner+sizeof(Stub)));

        return pWinner;
    }
}

FCIMPL1(Object*, COMDelegate::InternalAlloc, ReflectClassBaseObject* targetUNSAFE)
{
    OBJECTREF refRetVal = NULL;
    REFLECTCLASSBASEREF target = (REFLECTCLASSBASEREF) targetUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, target);
    //-[autocvtpro]-------------------------------------------------------

    ReflectClass* pRC = (ReflectClass*) target->GetData();
    _ASSERTE(pRC);
    EEClass* pEEC = pRC->GetClass();
    refRetVal = pEEC->GetMethodTable()->Allocate();

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refRetVal);
}
FCIMPLEND

// InternalCreateMethod
// This method will create initalize a delegate based upon a MethodInfo
//      for a static method.
FCIMPL3(void, COMDelegate::InternalCreateMethod, ReflectBaseObject* refThisUNSAFE, ReflectBaseObject* invokeMethUNSAFE, ReflectBaseObject* targetMethodUNSAFE)
{
    struct _gc
    {
        REFLECTBASEREF refThis;
        REFLECTBASEREF invokeMeth;
        REFLECTBASEREF targetMethod; 
    } gc;

    gc.refThis = (REFLECTBASEREF) refThisUNSAFE;
    gc.invokeMeth = (REFLECTBASEREF) invokeMethUNSAFE;
    gc.targetMethod = (REFLECTBASEREF) targetMethodUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_NOPOLL();
    GCPROTECT_BEGIN(gc);
    HELPER_METHOD_POLL();
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();
        // Intialize reflection
    COMClass::EnsureReflectionInitialized();
    InitFields();

    ReflectMethod* pRMTarget = (ReflectMethod*) gc.targetMethod->GetData();
    _ASSERTE(pRMTarget);
    MethodDesc* pTarget = pRMTarget->pMethod;
    PCCOR_SIGNATURE pTSig;
    DWORD TSigCnt;
    pTarget->GetSig(&pTSig,&TSigCnt);
    
    ReflectMethod* pRMInvoke = (ReflectMethod*) gc.invokeMeth->GetData();
    _ASSERTE(pRMInvoke);
    MethodDesc* pInvoke = pRMInvoke->pMethod;
    PCCOR_SIGNATURE pISig;
    DWORD ISigCnt;
    pInvoke->GetSig(&pISig,&ISigCnt);

    // the target method must be static, validated in Managed.
    _ASSERTE(pTarget->IsStatic());

    // Validate that the signature of the Invoke method and the 
    //      target method are exactly the same, except for the staticness.
    // (The Invoke method on Delegate is always non-static since it needs
    //  the this pointer for the Delegate (not the this pointer for the target), 
    //  so we must make the sig non-static before comparing sigs.)
    PCOR_SIGNATURE tmpSig = (PCOR_SIGNATURE) _alloca(ISigCnt);
    memcpy(tmpSig, pISig, ISigCnt);
    *((BYTE*)tmpSig) &= ~IMAGE_CEE_CS_CALLCONV_HASTHIS;

    if (MetaSig::CompareMethodSigs(pTSig,TSigCnt,pTarget->GetModule(),
        tmpSig,ISigCnt,pInvoke->GetModule()) == 0) {
        COMPlusThrow(kArgumentException,L"Arg_DlgtTargMeth");
    }

    EEClass* pDelEEC = gc.refThis->GetClass();

    RefSecContext sCtx;
    InvokeUtil::CheckAccess(&sCtx,
                            pTarget->GetAttrs(),
                            pTarget->GetMethodTable(),
                            REFSEC_CHECK_MEMBERACCESS|REFSEC_THROW_MEMBERACCESS);
    InvokeUtil::CheckLinktimeDemand(&sCtx,
                                    pTarget,
                                    true);
    GetMethodPtrAux();

    m_pORField->SetRefValue((OBJECTREF)gc.refThis, (OBJECTREF)gc.refThis);
    m_pFPAuxField->SetValuePtr((OBJECTREF)gc.refThis, pTarget->GetPreStubAddr());

    DelegateEEClass *pDelCls = (DelegateEEClass*) pDelEEC;
    Stub *pShuffleThunk = pDelCls->m_pStaticShuffleThunk;
    if (!pShuffleThunk) {
        pShuffleThunk = SetupShuffleThunk(pDelCls);
    }

    m_pFPField->SetValuePtr((OBJECTREF)gc.refThis, (void*)pShuffleThunk->GetEntryPoint());

    // Now set the MethodInfo in the delegate itself and we are done.    
    m_pMethInfoField->SetRefValue((OBJECTREF)gc.refThis, (OBJECTREF)gc.targetMethod);

    //-[autocvtepi]-------------------------------------------------------
    GCPROTECT_END();
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

// InternalGetMethodInfo
// This method will get the MethodInfo for a delegate
FCIMPL1(Object*, COMDelegate::InternalFindMethodInfo, ReflectBaseObject* refThisIn)
{
    MethodDesc*         pMD;
    ReflectMethod*      pRM;
    OBJECTREF           objMeth = NULL;

    struct _gc 
    {
        REFLECTCLASSBASEREF pRefClass;
        REFLECTBASEREF      refThis;
    } gc;
    ZeroMemory(&gc, sizeof(gc));

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, objMeth);
    GCPROTECT_BEGIN(gc);

    gc.refThis      = (REFLECTBASEREF)ObjectToOBJECTREF(refThisIn);
    pMD             = GetMethodDesc(gc.refThis);
    gc.pRefClass    = (REFLECTCLASSBASEREF) pMD->GetClass()->GetExposedClassObject();
    pRM             = ((ReflectClass*) gc.pRefClass->GetData())->FindReflectMethod(pMD);
    objMeth         = (OBJECTREF) pRM->GetMethodInfo((ReflectClass*) gc.pRefClass->GetData());

    GCPROTECT_END();
    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(objMeth);
}
FCIMPLEND

/*
    Does a static validation of parameters passed into a delegate constructor.

    Params:
    pFtn  : MethodDesc of the function pointer used to create the delegate
    pDlgt : The delegate type
    pInst : Type of the instance, from which pFtn is obtained. Ignored if pFtn 
            is static.

    Validates the following conditions:
    1.  If the function is not static, pInst should be equal to the type where 
        pFtn is defined or pInst should be a parent of pFtn's type.
    2.  The signature of the function should be compatible with the signature
        of the Invoke method of the delegate type.

 */
/* static */
BOOL COMDelegate::ValidateCtor(MethodDesc *pFtn, EEClass *pDlgt, EEClass *pInst)
{
    _ASSERTE(pFtn);
    _ASSERTE(pDlgt);

    /* Abstract is ok, since the only way to get a ftn of a an abstract method
       is via ldvirtftn, and we don't allow instantiation of abstract types.
       ldftn on abstract types is illegal.
    if (pFtn->IsAbstract())
        return FALSE;       // Cannot use an abstact method
    */
    if (!pFtn->IsStatic())
    {
        if (pInst == NULL)
            goto skip_inst_check;   // Instance missing, this will result in a 
                                    // NullReferenceException at runtime on Invoke().

        EEClass *pClsOfFtn = pFtn->GetClass();

        if (pClsOfFtn != pInst)
        {
            // If class of method is an interface, verify that
            // the interface is implemented by this instance.
            if (pClsOfFtn->IsInterface())
            {
                MethodTable *pMTOfFtn;
                InterfaceInfo_t *pImpl;

                pMTOfFtn = pClsOfFtn->GetMethodTable();
                pImpl = pInst->GetInterfaceMap();

                for (int i = 0; i < pInst->GetNumInterfaces(); i++)
                {
                    if (pImpl[i].m_pMethodTable == pMTOfFtn)
                        goto skip_inst_check;
                }
            }

            // Type of pFtn should be Equal or a parent type of pInst

            EEClass *pObj = pInst;

            do {
                pObj = pObj->GetParentClass();
            } while (pObj && (pObj != pClsOfFtn));

            if (pObj == NULL)
                return FALSE;   // Function pointer is not that of the instance
        }
    }

skip_inst_check:
    // Check the number and type of arguments

    MethodDesc *pDlgtInvoke;        // The Invoke() method of the delegate
    Module *pModDlgt, *pModFtn;     // Module where the signature is present
    PCCOR_SIGNATURE pSigDlgt, pSigFtn, pEndSigDlgt, pEndSigFtn; // Signature
    DWORD cSigDlgt, cSigFtn;        // Length of the signature
    DWORD nArgs;                    // Number of arguments

    pDlgtInvoke = COMDelegate::FindDelegateInvokeMethod(pDlgt);

    if (pDlgtInvoke->IsStatic())
        return FALSE;               // Invoke cannot be Static.

    pDlgtInvoke->GetSig(&pSigDlgt, &cSigDlgt);
    pFtn->GetSig(&pSigFtn, &cSigFtn);

    pModDlgt = pDlgtInvoke->GetModule();
    pModFtn = pFtn->GetModule();

    if ((*pSigDlgt & IMAGE_CEE_CS_CALLCONV_MASK) != 
        (*pSigFtn & IMAGE_CEE_CS_CALLCONV_MASK))
        return FALSE; // calling convention mismatch

    // The function pointer should never be a vararg
    if ((*pSigFtn & IMAGE_CEE_CS_CALLCONV_MASK) == IMAGE_CEE_CS_CALLCONV_VARARG)
        return FALSE; // Vararg function pointer

    // Check the number of arguments
    pSigDlgt++; pSigFtn++;

    pEndSigDlgt = pSigDlgt + cSigDlgt;
    pEndSigFtn = pSigFtn + cSigFtn;

    nArgs = CorSigUncompressData(pSigDlgt);

    if (CorSigUncompressData(pSigFtn) != nArgs)
        return FALSE;   // number of arguments don't match

    // do return type as well
    for (DWORD i = 0; i<=nArgs; i++)
    {
        if (MetaSig::CompareElementType(pSigDlgt, pSigFtn,
                pEndSigDlgt, pEndSigFtn, pModDlgt, pModFtn) == FALSE)
            return FALSE; // Argument types don't match
    }

    return TRUE;
}

BOOL COMDelegate::ValidateBeginInvoke(DelegateEEClass* pClass)
{
	_ASSERTE(pClass->m_pBeginInvokeMethod);
	if (pClass->m_pInvokeMethod == NULL) 
		return FALSE;

	MetaSig beginInvokeSig(pClass->m_pBeginInvokeMethod->GetSig(), pClass->GetModule());
	MetaSig invokeSig(pClass->m_pInvokeMethod->GetSig(), pClass->GetModule());

	if (beginInvokeSig.GetCallingConventionInfo() != (IMAGE_CEE_CS_CALLCONV_HASTHIS | IMAGE_CEE_CS_CALLCONV_DEFAULT))
		return FALSE;

	if (beginInvokeSig.NumFixedArgs() != invokeSig.NumFixedArgs() + 2)
		return FALSE;

	if (s_pIAsyncResult == 0) {
		s_pIAsyncResult = g_Mscorlib.FetchClass(CLASS__IASYNCRESULT);
		if (s_pIAsyncResult == 0)
			return FALSE;
	}

	OBJECTREF throwable = NULL; 
    bool result = FALSE;
    GCPROTECT_BEGIN(throwable);
	if (beginInvokeSig.GetRetTypeHandle(&throwable) != TypeHandle(s_pIAsyncResult) || throwable != NULL)
        goto exit;

	while(invokeSig.NextArg() != ELEMENT_TYPE_END) {
		beginInvokeSig.NextArg();
		if (beginInvokeSig.GetTypeHandle(&throwable) != invokeSig.GetTypeHandle(&throwable) || throwable != NULL)
            goto exit;
	}

	if (s_pAsyncCallback == 0) {
		s_pAsyncCallback = g_Mscorlib.FetchClass(CLASS__ASYNCCALLBACK);
		if (s_pAsyncCallback == 0)
            goto exit;
	}

	beginInvokeSig.NextArg();
	if (beginInvokeSig.GetTypeHandle(&throwable)!= TypeHandle(s_pAsyncCallback) || throwable != NULL)
        goto exit;

	beginInvokeSig.NextArg();
	if (beginInvokeSig.GetTypeHandle(&throwable)!= TypeHandle(g_pObjectClass) || throwable != NULL)
        goto exit;

	_ASSERTE(beginInvokeSig.NextArg() == ELEMENT_TYPE_END);

    result = TRUE;
exit:
    GCPROTECT_END();
    return result;
}

BOOL COMDelegate::ValidateEndInvoke(DelegateEEClass* pClass)
{
	_ASSERTE(pClass->m_pEndInvokeMethod);
	if (pClass->m_pInvokeMethod == NULL) 
		return FALSE;

	MetaSig endInvokeSig(pClass->m_pEndInvokeMethod->GetSig(), pClass->GetModule());
	MetaSig invokeSig(pClass->m_pInvokeMethod->GetSig(), pClass->GetModule());

	if (endInvokeSig.GetCallingConventionInfo() != (IMAGE_CEE_CS_CALLCONV_HASTHIS | IMAGE_CEE_CS_CALLCONV_DEFAULT))
		return FALSE;

	OBJECTREF throwable = NULL;
    bool result = FALSE;
    GCPROTECT_BEGIN(throwable);
	if (endInvokeSig.GetRetTypeHandle(&throwable) != invokeSig.GetRetTypeHandle(&throwable) || throwable != NULL)
        goto exit;

	CorElementType type;
	while((type = invokeSig.NextArg()) != ELEMENT_TYPE_END) {
		if (type == ELEMENT_TYPE_BYREF) {
			endInvokeSig.NextArg();
			if (endInvokeSig.GetTypeHandle(&throwable) != invokeSig.GetTypeHandle(&throwable) || throwable != NULL)
                goto exit;
		}
	}

	if (s_pIAsyncResult == 0) {
		s_pIAsyncResult = g_Mscorlib.FetchClass(CLASS__IASYNCRESULT);
		if (s_pIAsyncResult == 0)
            goto exit;
	}

	if (endInvokeSig.NextArg() == ELEMENT_TYPE_END)
        goto exit;

	if (endInvokeSig.GetTypeHandle(&throwable) != TypeHandle(s_pIAsyncResult) || throwable != NULL)
        goto exit;

	if (endInvokeSig.NextArg() != ELEMENT_TYPE_END)
        goto exit;

    result = TRUE;
exit:
    GCPROTECT_END();
    return result;
}
