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
// File: Method.CPP
//
// ===========================================================================
// Method is the cache sensitive portion of EEClass (see class.h)
// ===========================================================================

#include "common.h"
#include "comvariant.h"
#include "remoting.h"
#include "security.h"
#include "verifier.hpp"
#include "wsperf.h"
#include "excep.h"
#include "dbginterface.h"
#include "ecall.h"
#include "eeconfig.h"
#include "mlinfo.h"
#include "ndirect.h"
#include "utsem.h"

//
// Note: no one is allowed to use this mask outside of method.hpp and method.cpp. Don't make this public! Keep this
// version in sync with the version in the top of method.hpp.
//
#define METHOD_IS_IL_FLAG   0xC0000000

// Verify that the structure sizes of our MethodDescs support proper
// aligning for atomic stub replacement.
//
C_ASSERT((sizeof(MethodDescChunk)       & (METHOD_ALIGN - 1)) == 0);
C_ASSERT((sizeof(MethodDesc)            & (METHOD_ALIGN - 1)) == 0);
C_ASSERT((sizeof(ECallMethodDesc)       & (METHOD_ALIGN - 1)) == 0);
C_ASSERT((sizeof(NDirectMethodDesc)     & (METHOD_ALIGN - 1)) == 0);
C_ASSERT((sizeof(EEImplMethodDesc)      & (METHOD_ALIGN - 1)) == 0); 
C_ASSERT((sizeof(ArrayECallMethodDesc)  & (METHOD_ALIGN - 1)) == 0);

const unsigned g_ClassificationSizeTable[16] = {
    sizeof(MethodDesc) + METHOD_PREPAD,             // mcIL
    sizeof(ECallMethodDesc) + METHOD_PREPAD,        // mcECall
    sizeof(NDirectMethodDesc) + METHOD_PREPAD,      // mcNDirect
    sizeof(EEImplMethodDesc) + METHOD_PREPAD,       // mcEEImpl
    sizeof(ArrayECallMethodDesc) + METHOD_PREPAD,   // mcArray
        0,
        0, 
        0, 

    sizeof(MI_MethodDesc) + METHOD_PREPAD,          // mcIL | mdcMethodImpl
    sizeof(MI_ECallMethodDesc) + METHOD_PREPAD,     // mcECall | mdcMethodImpl
    sizeof(MI_NDirectMethodDesc) + METHOD_PREPAD,   // mcNDirect | mdcMethodImpl
    sizeof(MI_EEImplMethodDesc) + METHOD_PREPAD,    // mcEEImpl | mdcMethodImpl
    sizeof(MI_ArrayECallMethodDesc) + METHOD_PREPAD,// mcArray | mdcMethodImpl
        0, 
        0,
        0,
};  


LPCUTF8 MethodDesc::GetName(USHORT slot)
{
    if (GetMethodTable()->IsArray())
    {
        // Array classes don't have metadata tokens
        return ((ArrayECallMethodDesc*) this)->m_pszArrayClassMethodName;
    }
    else
    {
        if(IsMethodImpl()) {
            MethodImpl* pImpl = MethodImpl::GetMethodImplData(this);
            MethodDesc* real = pImpl->FindMethodDesc(slot, this);
            if (real == this || real->IsInterface())
                return (GetMDImport()->GetNameOfMethodDef(GetMemberDef()));
            else 
                return real->GetName();
        }
        else 
            return (GetMDImport()->GetNameOfMethodDef(GetMemberDef()));
    }
}

LPCUTF8 MethodDesc::GetName()
{
    if (GetMethodTable()->IsArray())
    {
        // Array classes don't have metadata tokens
        return ((ArrayECallMethodDesc*) this)->m_pszArrayClassMethodName;
    }
    else
    {
        if(IsMethodImpl()) {
            MethodImpl* pImpl = MethodImpl::GetMethodImplData(this);
            MethodDesc* real = pImpl->FindMethodDesc(GetSlot(), this);
            if (real == this || real->IsInterface())
                return (GetMDImport()->GetNameOfMethodDef(GetMemberDef()));
            else
                return real->GetName();
        }
        else 
            return (GetMDImport()->GetNameOfMethodDef(GetMemberDef()));
    }
}

void MethodDesc::GetSig(PCCOR_SIGNATURE *ppSig, DWORD *pcSig)
{
    if (HasStoredSig())
    {
        StoredSigMethodDesc *pSMD = (StoredSigMethodDesc *) this;
        if (pSMD->m_pSig != NULL)
        {
            *ppSig = pSMD->m_pSig;
            *pcSig = pSMD->m_cSig;
            return;
        }
    }

    *ppSig = GetMDImport()->GetSigOfMethodDef(GetMemberDef(), pcSig);
}

PCCOR_SIGNATURE MethodDesc::GetSig()
{

    if (HasStoredSig())
    {
        StoredSigMethodDesc *pSMD = (StoredSigMethodDesc *) this;
        if (pSMD->m_pSig != 0)
            return pSMD->m_pSig;
    }

    ULONG cbsig;
    return GetMDImport()->GetSigOfMethodDef(GetMemberDef(), &cbsig);
}


Stub *MethodDesc::GetStub()
{
    if (isShortcircuitedStub(this))
        return NULL;

    const BYTE* addr = getStubAddr(this);


    return Stub::RecoverStub(addr);
}

void MethodDesc::destruct()
{
    Stub *pStub = GetStub();
    if (pStub != NULL && pStub != ThePreStub()) {
        pStub->DecRef();
    }

    if (IsNDirect()) 
    {
        MLHeader *pMLHeader = ((NDirectMethodDesc*)this)->GetMLHeader();
        if (pMLHeader != NULL
            && !GetModule()->IsPreloadedObject(pMLHeader)) 
        {
            Stub *pMLStub = Stub::RecoverStub((BYTE*)pMLHeader);
            pMLStub->DecRef();
        }
    }

    EEClass *pClass = GetClass();
    if(pClass->IsMarshaledByRef() || (pClass == g_pObjectClass->GetClass()))
    {
        // Destroy the thunk generated to intercept calls for remoting
        CRemotingServices::DestroyThunk(this);    
    }

    // unload the code
    if (!g_fProcessDetach && IsJitted()) 
    {
        //
        IJitManager * pJM = ExecutionManager::FindJitMan((SLOT)GetAddrofCode());
        if (pJM) {
            pJM->Unload(this, IJitManager::UnloadUnlink);
        }
    }
}

BOOL MethodDesc::InterlockedReplaceStub(Stub** ppStub, Stub *pNewStub)
{
    _ASSERTE(ppStub != NULL);
    _ASSERTE(((DWORD_PTR)ppStub&(sizeof(void*)-1)) == 0);

    Stub *pPrevStub = (Stub*)FastInterlockCompareExchangePointer((void**)ppStub, (void*) pNewStub,
                                                          NULL);

    // Return TRUE if we succeeded.
    return (pPrevStub == NULL);
}


HRESULT MethodDesc::Verify(COR_ILMETHOD_DECODER* ILHeader, 
                            BOOL fThrowException,
                            BOOL fForceVerify)
{
    _ASSERTE(!"EE Verification is disabled, should never get here");
    return E_FAIL;
}

DWORD MethodDesc::SetIntercepted(BOOL set)
{
    DWORD dwResult = IsIntercepted();
    DWORD dwMask = mdcIntercepted;

    // We need to make this operation atomic (multiple threads can play with the
    // flags field while we're calling SetIntercepted). But the flags field is a
    // word and we only have interlock operations over dwords. So we round down
    // the flags field address to the nearest aligned dword (along with the
    // intended bitfield mask). Note that we make the assumption that the flags
    // word is aligned itself, so we only have two possibilites: the field
    // already lies on a dword boundary or it's precisely one word out.
    DWORD* pdwFlags = (DWORD*)((ULONG_PTR)&m_wFlags - (offsetof(MethodDesc, m_wFlags) & 0x3));    
#ifdef BIGENDIAN
    if ((offsetof(MethodDesc, m_wFlags) & ~0x3) == 0) {
#else
    if ((offsetof(MethodDesc, m_wFlags) & ~0x3) != 0) {
#endif
        C_ASSERT(sizeof(m_wFlags) == 2);
        dwMask <<= 16;
    }

    if (set)
        FastInterlockOr(pdwFlags, dwMask);
    else
        FastInterlockAnd(pdwFlags, ~dwMask);

    return dwResult;
}


BOOL MethodDesc::IsVoid()
{
    MetaSig sig(GetSig(),GetModule());
    return ELEMENT_TYPE_VOID == sig.GetReturnType();
}


ULONG MethodDesc::GetRVA()
{
    // IL RVA stored in same field as code address, but high bit set to
    // discriminate.
    // Fetch a local copy to avoid concurrent update problems.
    DWORD_PTR CodeOrIL = m_CodeOrIL;

    if ( ((CodeOrIL & METHOD_IS_IL_FLAG) == METHOD_IS_IL_FLAG) // !IsJitted() inlined
        && !IsPrejitted())
    {
        return CodeOrIL & ~METHOD_IS_IL_FLAG;
    }

    if (GetMemberDef() & 0x00FFFFFF)
    {
        DWORD dwDescrOffset;
        DWORD dwImplFlags;
        GetMDImport()->GetMethodImplProps(GetMemberDef(), &dwDescrOffset, &dwImplFlags);
        BAD_FORMAT_ASSERT(IsMiIL(dwImplFlags) || IsMiOPTIL(dwImplFlags) || dwDescrOffset == 0);
        return dwDescrOffset;
    }

    return 0;
}

BOOL MethodDesc::IsVarArg()
{
    return MetaSig::IsVarArg(GetModule(), GetSig());
}


COR_ILMETHOD* MethodDesc::GetILHeader()
{
    Module *pModule;

    _ASSERTE( IsIL() );

    pModule = GetModule();

    _ASSERTE( IsIL() && GetRVA() != ~METHOD_IS_IL_FLAG);
    return (COR_ILMETHOD*) pModule->GetILCode((DWORD)GetRVA());
}


MethodDesc::RETURNTYPE MethodDesc::ReturnsObject(
#ifdef _DEBUG
    bool supportStringConstructors
#endif    
    )
{
    MetaSig sig(GetSig(),GetModule());
    switch (sig.GetReturnType())
    {
        case ELEMENT_TYPE_STRING:
        case ELEMENT_TYPE_CLASS:
        case ELEMENT_TYPE_SZARRAY:
        case ELEMENT_TYPE_ARRAY:
        case ELEMENT_TYPE_OBJECT:
        case ELEMENT_TYPE_VAR:
            return RETOBJ;

        //TYPEDBYREF is a structure.  A function of this return type returns
        // void.  We drop out of this switch and consider if we have a constructor.
        // Otherwise this function is going to return RETNONOBJ.
        //case ELEMENT_TYPE_TYPEDBYREF:   // TYPEDBYREF is just an OBJECT.
            
        case ELEMENT_TYPE_BYREF:
            return RETBYREF;
        default:
            break;
    }

    // String constructors return objects.  We should not have any ecall string
    // constructors, except when called from gc coverage codes (which is only
    // done under debug).  We will therefore optimize the retail version of this
    // method to not support string constructors.
#ifdef _DEBUG
    if (IsCtor() && GetClass()->HasVarSizedInstances())
    {
        _ASSERTE(supportStringConstructors);
        return RETOBJ;
    }
#endif

    return RETNONOBJ;
}



DWORD MethodDesc::GetAttrs()
{
    if (IsArray())
		return ((ArrayECallMethodDesc*) this)->m_wAttrs;

    return GetMDImport()->GetMethodDefProps(GetMemberDef());
}
DWORD MethodDesc::GetImplAttrs()
{
    ULONG RVA;
    DWORD props;
    GetMDImport()->GetMethodImplProps(GetMemberDef(), &RVA, &props);
    return props;
}


Module *MethodDesc::GetModule()
{
    MethodDescChunk *chunk = MethodDescChunk::RecoverChunk(this);

    return chunk->GetMethodTable()->GetModule();
}


DWORD MethodDesc::IsUnboxingStub()
{
    return (m_CodeOrIL == (DWORD_PTR) ~0 && GetClass()->IsValueClass());
}


MethodDescChunk *MethodDescChunk::CreateChunk(LoaderHeap *pHeap, DWORD methodDescCount, int flags, BYTE tokrange)
{
    _ASSERTE(methodDescCount <= GetMaxMethodDescs(flags));
    _ASSERTE(methodDescCount > 0);

    SIZE_T mdSize = g_ClassificationSizeTable[flags & (mdcClassification|mdcMethodImpl)];
    SIZE_T presize = sizeof(MethodDescChunk);

    MethodDescChunk *block = (MethodDescChunk *) 
      pHeap->AllocAlignedmem(presize + mdSize * methodDescCount, MethodDesc::ALIGNMENT);
    if (block == NULL)
        return NULL;

    block->m_count = (BYTE) (methodDescCount-1);
    block->m_kind = flags & (mdcClassification|mdcMethodImpl);
    block->m_tokrange = tokrange;

    WS_PERF_UPDATE_DETAIL("MethodDescChunk::CreateChunk", 
                          presize + mdSize * methodDescCount, block);

    return block;
}

void MethodDescChunk::SetMethodTable(MethodTable *pMT)
{
    _ASSERTE(m_methodTable == NULL);
    m_methodTable = pMT;
    pMT->GetClass()->AddChunk(this);
}


UINT MethodDesc::SizeOfVirtualFixedArgStack()
{
    return MetaSig::SizeOfVirtualFixedArgStack(GetModule(), GetSig(), IsStatic());
}

UINT MethodDesc::SizeOfActualFixedArgStack()
{
    return MetaSig::SizeOfActualFixedArgStack(GetModule(), GetSig(), IsStatic());
}

UINT MethodDesc::CbStackPop()
{
    return MetaSig::CbStackPop(GetModule(), GetSig(), IsStatic());
}


const BYTE* MethodDesc::GetCallTarget(const ARG_SLOT* pArguments)
{
    const BYTE* pTarget;

    if (!((DontVirtualize() || GetClass()->IsValueClass())))
    {
        pTarget = GetAddrofCode(ArgSlotToObj(pArguments[0]));
#ifdef DEBUG
        // For the cases where we ALWAYS want the Prestub, make certain that 
        // the address we find in the Vtable actually points at the prestub. 
        if (IsComPlusCall() || IsECall() || IsIntercepted() || IsRemotingIntercepted() || IsEnCMethod())
            _ASSERTE(getStubCallAddr(GetClass()->GetUnknownMethodDescForSlotAddress(pTarget)) == pTarget);
#endif
    }
    else 
    {
        pTarget = (IsComPlusCall() || IsECall() || IsIntercepted() || IsRemotingIntercepted() || IsEnCMethod()) ?
                            GetLocationOfPreStub() : 
                            GetAddrofCode();
     }
    
    return pTarget;
}

//--------------------------------------------------------------------
// Invoke a method. Arguments are packaged up in right->left order
// which each array element corresponding to one argument.
//
// Can throw a COM+ exception.
//--------------------------------------------------------------------
ARG_SLOT MethodDesc::CallTransparentProxy(const ARG_SLOT *pArguments)
{
    THROWSCOMPLUSEXCEPTION();
    
    MethodTable* pTPMT = CTPMethodTable::GetMethodTable();
    _ASSERTE(pTPMT != NULL);

#ifdef _DEBUG
    OBJECTREF oref = ArgSlotToObj(pArguments[0]);
    MethodTable* pMT = oref->GetMethodTable();
    _ASSERTE(pMT->IsTransparentProxyType());
#endif

    DWORD slot = GetSlot();

    // ensure the slot is within range
    _ASSERTE( slot <= CTPMethodTable::GetCommitedTPSlots());

    const BYTE* pTarget = (const BYTE*)pTPMT->GetVtable()[slot];
    
    return CallDescr(pTarget, GetModule(), GetSig(), IsStatic(), pArguments);
}

//--------------------------------------------------------------------
// Invoke a method. Arguments are packaged up in ARG SLOTS.
//
// Can throw a COM+ exception.
//--------------------------------------------------------------------
INT64 MethodDesc::CallDebugHelper(const ARG_SLOT *pArguments, MetaSig* sig)
{
    // You're not allowed to call a method directly on a MethodDesc that comes from an Interface. If you do, then you're
    // calling through the slot in the Interface's vtable instead of through the slot on the object's vtable.
    
    THROWSCOMPLUSEXCEPTION();

    // For member methods, use the instance to determine the correct (VTable)
    // address to call.
    // REVIEW: Should we just return GetPreStubAddr() always and let that do
    // the right thing ... instead of doing so many checks? Even if the code has
    // been jitted all we will do is an extra "jmp"

    const BYTE *pTarget;
    
	// If the method is virutal, do the virtual lookup. 
    if (!DontVirtualize() && !GetClass()->IsValueClass()) 
    {
        OBJECTREF thisPtr = ArgSlotToObj(pArguments[0]);
        _ASSERTE(thisPtr);
        pTarget = GetAddrofCode(thisPtr);

#ifdef DEBUG
        // For the cases where we ALWAYS want the Prestub, make certain that 
        // the address we find in the Vtable actually points at the prestub. 
        if (IsComPlusCall() || IsECall() || IsIntercepted() || IsRemotingIntercepted() || IsEnCMethod())
            _ASSERTE(getStubCallAddr(GetClass()->GetUnknownMethodDescForSlotAddress(pTarget)) == pTarget);
#endif
    }
    else 
    {
        if (IsComPlusCall() || IsECall() || IsIntercepted() || IsRemotingIntercepted() || IsEnCMethod())
            pTarget = GetPreStubAddr();
        else
            pTarget = GetAddrofCode();
    }

   if (pTarget == NULL)
        COMPlusThrow(kArgumentException, L"Argument_CORDBBadAbstract");

    return CallDescr(pTarget, GetModule(), sig, IsStatic(), pArguments);
}


ARG_SLOT MethodDesc::Call(const ARG_SLOT *pArguments)
{
    // You're not allowed to call a method directly on a MethodDesc that comes from an Interface. If you do, then you're
    // calling through the slot in the Interface's vtable instead of through the slot on the object's vtable.
    _ASSERTE(!IsComPlusCall());
    
    THROWSCOMPLUSEXCEPTION();

    return CallDescr(GetCallTarget(pArguments), GetModule(), GetSig(), IsStatic(), pArguments);
}

// NOTE: This variant exists so that we don't have to touch the metadata for the method being called.
ARG_SLOT MethodDesc::Call(const ARG_SLOT *pArguments, BinderMethodID sigID)
{
    // You're not allowed to call a method directly on a MethodDesc that comes from an Interface. If you do, then you're
    // calling through the slot in the Interface's vtable instead of through the slot on the object's vtable.
    _ASSERTE(!IsComPlusCall());
    
#ifdef _DEBUG
    PCCOR_SIGNATURE pSig;
    DWORD cSig;
    GetSig(&pSig, &cSig);
    
    _ASSERTE(MetaSig::CompareMethodSigs(g_Mscorlib.GetMethodSig(sigID)->GetBinarySig(), 
                                        g_Mscorlib.GetMethodSig(sigID)->GetBinarySigLength(), 
                                        SystemDomain::SystemModule(),
                                        pSig, cSig, GetModule()));
#endif
    
    THROWSCOMPLUSEXCEPTION();

    MetaSig sig(g_Mscorlib.GetMethodBinarySig(sigID), SystemDomain::SystemModule());

    return CallDescr(GetCallTarget(pArguments), GetModule(), &sig, IsStatic(), pArguments);
}

ARG_SLOT MethodDesc::Call(const ARG_SLOT *pArguments, MetaSig* sig)
{
    // You're not allowed to call a method directly on a MethodDesc that comes from an Interface. If you do, then you're
    // calling through the slot in the Interface's vtable instead of through the slot on the object's vtable.
    _ASSERTE(!IsComPlusCall());
    
    THROWSCOMPLUSEXCEPTION();

    
    return CallDescr(GetCallTarget(pArguments), GetModule(), sig, IsStatic(), pArguments);
}

// This is another unusual case. When calling a COM object using a MD the call needs to
// go through an interface MD. This method must be used to accomplish that.
ARG_SLOT MethodDesc::CallOnInterface(const ARG_SLOT *pArguments)
{   
    // This should only be used for ComPlusCalls.
    _ASSERTE(IsComPlusCall());

    THROWSCOMPLUSEXCEPTION();

    const BYTE *pTarget = GetLocationOfPreStub();
    return CallDescr(pTarget, GetModule(), GetSig(), IsStatic(), pArguments);
}

ARG_SLOT MethodDesc::CallOnInterface(const ARG_SLOT *pArguments, MetaSig *pSig)
{   
    // This should only be used for ComPlusCalls.
    _ASSERTE(IsComPlusCall());

    THROWSCOMPLUSEXCEPTION();

    const BYTE *pTarget = GetLocationOfPreStub();
    return CallDescr(pTarget, GetModule(), pSig, IsStatic(), pArguments);
}

ARG_SLOT MethodDesc::CallDescr(const BYTE *pTarget, Module *pModule, PCCOR_SIGNATURE pSig, BOOL fIsStatic, const ARG_SLOT *pArguments)
{
    MetaSig sig(pSig, pModule);
    return MethodDesc::CallDescr (pTarget, pModule, &sig, fIsStatic, pArguments);
}


ARG_SLOT MethodDesc::CallDescr(const BYTE *pTarget, Module *pModule, MetaSig* pMetaSigOrig, BOOL fIsStatic, const ARG_SLOT *pArguments)
{
    THROWSCOMPLUSEXCEPTION();

    // Make local copy as this function mutates iterator state.
    MetaSig msigCopy = pMetaSigOrig;
    MetaSig *pMetaSig = &msigCopy;

    _ASSERTE(GetAppDomain()->ShouldHaveCode());

#ifdef _DEBUG
    {
        // Check to see that any value type args have been restored.
        // This is because we may be calling a FramedMethodFrame which will use the sig
        // to trace the args, but if any are unloaded we will be stuck if a GC occurs.

        _ASSERTE(GetMethodTable()->IsRestored());
        CorElementType argType;
        while ((argType = pMetaSig->NextArg()) != ELEMENT_TYPE_END)
        {
            if (argType == ELEMENT_TYPE_VALUETYPE)
            {
                TypeHandle th = pMetaSig->GetTypeHandle(NULL, TRUE, TRUE);
                _ASSERTE(th.IsRestored());
            }
        }
        pMetaSig->Reset();
    }
#endif

    BYTE callingconvention = pMetaSig->GetCallingConvention();
    if (!isCallConv(callingconvention, IMAGE_CEE_CS_CALLCONV_DEFAULT))
    {
        _ASSERTE(!"This calling convention is not supported.");
        COMPlusThrow(kInvalidProgramException);
    }

#ifdef DEBUGGING_SUPPORTED
    if (CORDebuggerTraceCall())
        g_pDebugInterface->TraceCall(pTarget);
#endif // DEBUGGING_SUPPORTED

#if CHECK_APP_DOMAIN_LEAKS
    if (g_pConfig->AppDomainLeaks())
    {
        // See if we are in the correct domain to call on the object 
        if (!fIsStatic && !GetClass()->IsValueClass())
        {
            OBJECTREF pThis = ArgSlotToObj(pArguments[0]);
            if (!pThis->AssignAppDomain(GetAppDomain()))
                _ASSERTE(!"Attempt to call method on object in wrong domain");
        }
    }
#endif

    DWORD   arg = 0;

    UINT   nActualStackBytes = pMetaSig->SizeOfActualFixedArgStack(fIsStatic);

    // Create a fake FramedMethodFrame on the stack.
    LPBYTE pAlloc = (LPBYTE)_alloca(FramedMethodFrame::GetNegSpaceSize() + sizeof(FramedMethodFrame) + nActualStackBytes);

    LPBYTE pFrameBase = pAlloc + FramedMethodFrame::GetNegSpaceSize();

    ArgIterator argit(pFrameBase, pMetaSig, fIsStatic);

    if (!fIsStatic) {
        *((LPVOID*) argit.GetThisAddr()) = ArgSlotToPtr(pArguments[arg++]);
    }
    
    if (pMetaSig->HasRetBuffArg()) 
    {
        *((LPVOID*) argit.GetRetBuffArgAddr()) = ArgSlotToPtr(pArguments[arg++]);
#if defined(_PPC_) || defined(_SPARC_) // retbuf
        // the CallDescrWorker callsite for methods with return buffer is 
        //  different for RISC CPUs - we pass this information along by setting 
        //  the lowest bit in pTarget
        pTarget = (const BYTE *)((UINT_PTR)pTarget | 0x1);
#endif
    }




    BYTE   typ;
    UINT32 structSize;
    int    ofs;
    for( ; 0 != (ofs = argit.GetNextOffsetFaster(&typ, &structSize)); arg++ ) 
    {
        UINT32 stackSize = structSize;


#if CHECK_APP_DOMAIN_LEAKS
        // Make sure the arg is in the right app domain
        if (g_pConfig->AppDomainLeaks() && typ == ELEMENT_TYPE_CLASS) {
            OBJECTREF objRef = ArgSlotToObj(pArguments[arg]);
            if (!objRef->AssignAppDomain(GetAppDomain()))
                _ASSERTE(!"Attempt to pass object in wrong app domain to method");
        }
#endif

        PVOID pDest = pFrameBase + ofs;


        switch (stackSize) {
            case 1:
            case 2:
            case 4:
                *((INT32*)pDest) = (INT32)pArguments[arg];
                break;

            case 8:
                *((INT64*)pDest) = pArguments[arg];
                break;

            default:
                {
                    CopyMemory(pDest,
                        (stackSize>sizeof(ARG_SLOT)) ? 
                            (LPVOID)ArgSlotToPtr(pArguments[arg]) : (LPVOID)&pArguments[arg],
                        stackSize);
                }
        }
    }

#ifdef _PPC_
    FramedMethodFrame::Enregister(pFrameBase, pMetaSig, fIsStatic, nActualStackBytes);
#endif

    INT64 retval;

    INSTALL_COMPLUS_EXCEPTION_HANDLER();
    retval = CallDescrWorker(pFrameBase + sizeof(FramedMethodFrame) + nActualStackBytes,
                             nActualStackBytes / STACK_ELEM_SIZE,
#if defined(_X86_) || defined(_PPC_) // argregs
                             (ArgumentRegisters*)(pFrameBase + FramedMethodFrame::GetOffsetOfArgumentRegisters()),
#endif
                             (LPVOID)pTarget);
    UNINSTALL_COMPLUS_EXCEPTION_HANDLER();


    getFPReturn(pMetaSig->GetFPReturnSize(), &retval);

#ifdef BIGENDIAN
    if (!pMetaSig->Is64BitReturn()) 
        retval >>= 32;
#endif

    return (ARG_SLOT) retval;
}


/*******************************************************************/
/* convert arbitrary IP location in jitted code to a MethodDesc */

MethodDesc* IP2MethodDesc(const BYTE* IP) 
{
    IJitManager* jitMan = ExecutionManager::FindJitMan((SLOT)IP);
    if (jitMan == 0)
        return(0);
    return jitMan->JitCode2MethodDesc((SLOT)IP);
}

//
// convert an entry point into a method desc 
//

MethodDesc* Entry2MethodDesc(const BYTE* entryPoint, MethodTable *pMT) 
{
    MethodDesc* ret;

    MethodDesc* method = IP2MethodDesc(entryPoint);
    if (method)
        return method;

    method = StubManager::MethodDescFromEntry(entryPoint, pMT);
    if (method) {
        return method;
    }
    
    // Is it an FCALL? 
    ret = MapTargetBackToMethod(entryPoint);
    if (ret != 0) {
        _ASSERTE(ret->GetAddrofJittedCode() == entryPoint);
        return(ret);
    }
    
    // Its a stub
    ret = (MethodDesc*) (entryPoint + METHOD_CALL_PRESTUB_SIZE);
    _ASSERTE(ret->m_pDebugEEClass == ret->m_pDebugMethodTable->GetClass());
    
    return(ret);
}

BOOL MethodDesc::CouldBeFCall() {
    return IsECall();
}

//
// Returns true if we are still pointing at the prestub.
// Note that there are two cases:
// 1) Prejit:    point to the prestub jump stub
// 2) No-prejit: point directly to the prestub
// Consider looking for the "e9 offset" pattern instead of
// the call to GetPrestubJumpStub if we want to improve
// the performance of this method.
//

BOOL MethodDesc::PointAtPreStub()
{
    const BYTE *stubAddr = getStubAddr(this);

    return ((stubAddr == ThePreStub()->GetEntryPoint()) 
           );
}

DWORD MethodDesc::GetSecurityFlags()
{
    DWORD dwMethDeclFlags       = 0;
    DWORD dwMethNullDeclFlags   = 0;
    DWORD dwClassDeclFlags      = 0;
    DWORD dwClassNullDeclFlags  = 0;

    // We're supposed to be caching this bit - make sure it's right.
    _ASSERTE((IsMdHasSecurity(GetAttrs()) != 0) == HasSecurity());

    if (HasSecurity())
    {
#ifdef _DEBUG
        HRESULT hr =
#endif
        Security::GetDeclarationFlags(GetMDImport(),
                                                   GetMemberDef(),
                                                   &dwMethDeclFlags,
                                                   &dwMethNullDeclFlags);
        _ASSERTE(SUCCEEDED(hr));

        // We only care about runtime actions, here.
        // Don't add security interceptors for anything else!
        dwMethDeclFlags     &= DECLSEC_RUNTIME_ACTIONS;
        dwMethNullDeclFlags &= DECLSEC_RUNTIME_ACTIONS;
    }

    EEClass *pCl = GetClass();
    if (pCl)
    {
        PSecurityProperties pSecurityProperties = pCl->GetSecurityProperties();
        if (pSecurityProperties)
        {
            dwClassDeclFlags    = pSecurityProperties->GetRuntimeActions();
            dwClassNullDeclFlags= pSecurityProperties->GetNullRuntimeActions();
        }
    }

    // Build up a set of flags to indicate the actions, if any,
    // for which we will need to set up an interceptor.

    // Add up the total runtime declarative actions so far.
    DWORD dwSecurityFlags = dwMethDeclFlags | dwClassDeclFlags;

    // Add in a declarative demand for NDirect.
    // If this demand has been overridden by a declarative check
    // on a class or method, then the bit won't change. If it's
    // overridden by an empty check, then it will be reset by the
    // subtraction logic below.
    if (IsNDirect())
    {
        dwSecurityFlags |= DECLSEC_UNMNGD_ACCESS_DEMAND;
    }

    if (dwSecurityFlags)
    {
        // If we've found any declarative actions at this point,
        // try to subtract any actions that are empty.

            // Subtract out any empty declarative actions on the method.
        dwSecurityFlags &= ~dwMethNullDeclFlags;

        // Finally subtract out any empty declarative actions on the class,
        // but only those actions that are not also declared by the method.
        dwSecurityFlags &= ~(dwClassNullDeclFlags & ~dwMethDeclFlags);
    }

    return dwSecurityFlags;
}

DWORD MethodDesc::GetSecurityFlags(IMDInternalImport *pInternalImport, mdToken tkMethod, mdToken tkClass, DWORD *pdwClassDeclFlags, DWORD *pdwClassNullDeclFlags, DWORD *pdwMethDeclFlags, DWORD *pdwMethNullDeclFlags)
{
#ifdef _DEBUG
    HRESULT hr =
#endif
    Security::GetDeclarationFlags(pInternalImport,
                                               tkMethod,
                                               pdwMethDeclFlags,
                                               pdwMethNullDeclFlags);
    _ASSERTE(SUCCEEDED(hr));

    if (!IsNilToken(tkClass) && (*pdwClassDeclFlags == 0xffffffff || *pdwClassNullDeclFlags == 0xffffffff))
    {
#ifdef _DEBUG
        HRESULT hr =
#endif
        Security::GetDeclarationFlags(pInternalImport,
                                                   tkClass, 
                                                   pdwClassDeclFlags,
                                                   pdwClassNullDeclFlags);
        _ASSERTE(SUCCEEDED(hr));

    }

    // Build up a set of flags to indicate the actions, if any,
    // for which we will need to set up an interceptor.

    // Add up the total runtime declarative actions so far.
    DWORD dwSecurityFlags = *pdwMethDeclFlags | *pdwClassDeclFlags;

    // Add in a declarative demand for NDirect.
    // If this demand has been overridden by a declarative check
    // on a class or method, then the bit won't change. If it's
    // overridden by an empty check, then it will be reset by the
    // subtraction logic below.
    if (IsNDirect())
    {
        dwSecurityFlags |= DECLSEC_UNMNGD_ACCESS_DEMAND;
    }

    if (dwSecurityFlags)
    {
        // If we've found any declarative actions at this point,
        // try to subtract any actions that are empty.

            // Subtract out any empty declarative actions on the method.
        dwSecurityFlags &= ~*pdwMethNullDeclFlags;

        // Finally subtract out any empty declarative actions on the class,
        // but only those actions that are not also declared by the method.
        dwSecurityFlags &= ~(*pdwClassNullDeclFlags & ~*pdwMethDeclFlags);
    }

    return dwSecurityFlags;
}

MethodImpl *MethodDesc::GetMethodImpl()
{
    _ASSERTE(IsMethodImpl());

    switch (GetClassification())
    {
    case mcIL:
        return ((MI_MethodDesc*)this)->GetImplData();
    case mcECall:
        return ((MI_ECallMethodDesc*)this)->GetImplData();
    case mcNDirect:
        return ((MI_NDirectMethodDesc*)this)->GetImplData();
    case mcEEImpl:
        return ((MI_EEImplMethodDesc*)this)->GetImplData();
    case mcArray:
        return ((MI_ArrayECallMethodDesc*)this)->GetImplData();
    default:
        _ASSERTE(!"Unknown MD Kind");
        return NULL;
    }
}


const BYTE* MethodDesc::GetAddrOfCodeForLdFtn()
{
    if (IsRemotingIntercepted2())
        return *(BYTE**)CRemotingServices::GetNonVirtualThunkForVirtualMethod(this);
    else
        return GetUnsafeAddrofCode();
}


// Attempt to store a kNoMarsh or kYesMarsh in the marshcategory field.
// Due to the need to avoid races with the prestub, there is a
// small but nonzero chance that this routine may silently fail
// and leave the marshcategory as "unknown." This is ok since
// all it means is that the JIT may have to do repeat some work
// the next time it JIT's a callsite to this NDirect.
void NDirectMethodDesc::ProbabilisticallyUpdateMarshCategory(MarshCategory value)
{
    // We can only attempt to go from kUnknown to Yes or No, or from
    // Yes to Yes and No to No.
    _ASSERTE(value == kNoMarsh || value == kYesMarsh);
    _ASSERTE(GetMarshCategory() == kUnknown || GetMarshCategory() == value); 


    // Due to the potential race with the prestub flags stored in the same
    // byte, we'll use InterlockedCompareExchange to ensure we don't
    // disturb those bits. But since InterlockedCompareExhange only
    // works on ULONGs, we'll have to operate on the entire ULONG. Ugh.

    BYTE *pb = &ndirect.m_flags;
    UINT ofs=0;

    // Decrement back until we have a ULONG-aligned address (not
    // sure if this needed for VipInterlocked, but better safe...)
    while (  ((size_t)pb) & (sizeof(ULONG)-1) )
    {
        ofs++;
        pb--;
    }

    // Ensure we won't be reading or writing outside the bounds of the NDirectMethodDesc.
    _ASSERTE(pb >= (BYTE*)this);
    _ASSERTE((pb+sizeof(ULONG)) < (BYTE*)(this+1));

    // Snapshot the existing bits
    ULONG oldulong = *(ULONG*)pb;
    
    // Modify the marshcat (and ONLY the marshcat fields in the snapshot)
    ULONG newulong = oldulong;
    ((BYTE*)&newulong)[ofs] &= ~kMarshCategoryMask;
    ((BYTE*)&newulong)[ofs] |= (value << kMarshCategoryShift);

    // Now, slam all 32 bits back in atomically but only if no other threads
    // have changed those bits since our snapshot. If they have, we will
    // silently throw away the new bits and no update will occur. That's
    // ok because this function's contract says it can throw away the update.
    FastInterlockCompareExchange((LONG*)pb, (LONG)newulong, (LONG)oldulong);

}

BOOL NDirectMethodDesc::InterlockedReplaceMLHeader(MLHeader *pMLHeader, MLHeader *pOldMLHeader)
{
    _ASSERTE(IsNDirect());
    _ASSERTE(sizeof(LONG) == sizeof(Stub*));
    MLHeader *pPrevML = (MLHeader*)FastInterlockCompareExchangePointer( (void**)&ndirect.m_pMLHeader, 
                                                                 (void*)pMLHeader, (void*)pOldMLHeader );
    return pPrevML == pOldMLHeader;
}



