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
// File: Prestub.cpp
//
// ===========================================================================
// This file contains the implementation for creating and using prestubs
// ===========================================================================
//

#include "common.h"
#include "vars.hpp"
#include "security.h"
#include "eeconfig.h"
#include "ndirect.h"
#include "comdelegate.h"
#include "remoting.h"
#include "dbginterface.h"


#include "listlock.inl"

//==========================================================================
// This function is logically part of PreStubWorker(). The only reason
// it's broken out into a separate function is that StubLinker has a destructor
// and thus, we must put an inner COMPLUS_TRY clause to trap any
// COM+ exceptions that would otherwise bypass the StubLinker destructor.
// Because COMPLUS_TRY is based on SEH, VC won't let us use it in the
// same function that declares the StubLinker object.
//==========================================================================
Stub *MakeSecurityWorker(CPUSTUBLINKER *psl,
                         MethodDesc *pMD,
                         DWORD dwDeclFlags,
                         Stub* pRealStub,
                         LPVOID pRealAddr,
                         OBJECTREF *pThrowable)
{
    Stub *pStub = NULL;

    LOG((LF_CORDB, LL_INFO10000, "Real Stub 0x%x\n", pRealStub));
    
    COMPLUS_TRY
    {
        pStub = Security::CreateStub(psl,
                                     pMD,
                                     dwDeclFlags,
                                     pRealStub,
                                     pRealAddr);
    }
    COMPLUS_CATCH
    {
        UpdateThrowable(pThrowable);
        pStub = NULL;
    }
    COMPLUS_END_CATCH

    return pStub;

}

//==========================================================================
// This is another function to help the PreStubWorker().  I have broken this
// out since only in the backpatch case do we need this method!
//==========================================================================
OBJECTREF GetActiveObject(PrestubMethodFrame *pPFrame)
{
    THROWSCOMPLUSEXCEPTION();

#if _DEBUG 

    // This check is expensive (it accesses metadata), so only do it in a checked build 

    BYTE callingConvention = MetaSig::GetCallingConvention(pPFrame->GetModule(),pPFrame->GetFunction()->GetSig());

    if (!isCallConv(callingConvention, IMAGE_CEE_CS_CALLCONV_DEFAULT) &&
        !isCallConv(callingConvention, IMAGE_CEE_CS_CALLCONV_VARARG))
    {
        _ASSERTE(!"Unimplemented calling convention.");
        FATAL_EE_ERROR();
        return NULL;
    }
#endif

    // Now return the this pointer!
    return pPFrame->GetThis();
}

static void DoBackpatch(MethodDesc *pMD, Stub *pstub, MethodTable *pDispatchingMT)
{
    _ASSERTE(!pMD->IsAbstract());

    // don't backpatch COMDelegate::DelegateConstruct FCall
    if (pMD->IsECall() && pMD->GetClass()->IsAnyDelegateClass())
        return;

    // don't want to update for EditAndContinue on the jit pass through a prestub becuase
    // could be calling from ResumeInUpdatedFunction which calls PrestubWorker directly
    // so wouldn't have a current object to work from. So this would get updated to the
    // updateable stub on the next call through.
    if (pMD->IsVtableMethod() &&
        !pMD->GetClass()->IsValueClass() &&
        !pMD->GetModule()->IsEditAndContinue() && 
        pDispatchingMT)
    {
        // Try patching up and down the hierarchy.  If this fails (e.g.
        // because of app domain unloading) then fall back on the tired old
        // single slot patch.
        if (!EEClass::PatchAggressively(pMD, (SLOT)pstub))
            {
            if ((pDispatchingMT->GetVtable())[(pMD)->GetSlot()] == (SLOT)pMD->GetPreStubAddr())
                (pDispatchingMT->GetVtable())[(pMD)->GetSlot()] = (SLOT)pstub;
        }
    }

    // Always patch the entry of the class identified by the method desc.
    // This may have already happened, but it's not worth checking.
    (pMD->GetClass()->GetMethodTable()->GetVtable())[pMD->GetSlot()] = (SLOT)pstub;
}

// Helper function to avoid two COMPLUS_TRY/COMPLUS_CATCH blocks in one function
static Stub* JITFunctionHelper(MethodDesc *pMD, COR_ILMETHOD_DECODER* header, BOOL *fEJit, 
    DeadlockAwareLockedListElement * pEntry, OBJECTREF *pThrowable)
{
    Stub *pstub = NULL;

    COMPLUS_TRY 
    {
        pstub = JITFunction(pMD, header, fEJit);
    }
    COMPLUS_CATCH
    {
        // Don't forget the case where we aborted our jit because of a deadlock cycle that
        // another function broke by jitting our function
        if (!pMD->IsJitted())
        {
            *pThrowable = GETTHROWABLE();
            pEntry->m_hrResultCode = E_FAIL;
        }
    }
    COMPLUS_END_CATCH

    return pstub;
}

//==========================================================================
// This function is logically part of PreStubWorker(). The only reason
// it's broken out into a separate function is that StubLinker has a destructor
// and thus, we must put an inner COMPLUS_TRY clause to trap any
// COM+ exceptions that would otherwise bypass the StubLinker destructor.
// Because COMPLUS_TRY is based on SEH, VC won't let us use it in the
// same function that declares the StubLinker object.
//==========================================================================
Stub *MakeJitWorker(MethodDesc *pMD, COR_ILMETHOD_DECODER* ILHeader, BOOL fIntercepted, BOOL fGenerateUpdateableStub, MethodTable *pDispatchingMT, OBJECTREF *pThrowable)
{
    // ********************************************************************
    //                  README!!
    // ********************************************************************
    
    // This helper method is assumed to be thread safe!
    // If multiple threads get in here for the same pMD ALL of them
    // MUST return the SAME value for pstub.
    
    // ********************************************************************
    //                  End README!
    // ********************************************************************
   

    Stub *pstub = NULL; // CHANGE, VC6.0
    BOOL fisEJitted = FALSE;
    // complus to com calls don't really have a method desc
    _ASSERTE(!pMD->IsComPlusCall());

    // REVIEW: this fires on fstChk during profiler checkin BVTs(appdomain.exe)
    // investigate!
    // _ASSERTE(!pMD->IsPrejitted());

    _ASSERTE(pMD->GetModule());
    _ASSERTE(pMD->GetModule()->GetClassLoader());
    Assembly* pAssembly = pMD->GetModule()->GetAssembly();
    _ASSERTE(pAssembly);

    COMPLUS_TRY
    {
        if (pMD->IsIL())
        {
            ListLock *                       pJitLock = NULL;
            BOOL                             bJitLockHeld = FALSE;
            DeadlockAwareLockedListElement * pEntry = NULL;
            BOOL                             bEntryLockSucceed = FALSE;
            BOOL                             fSuccess = FALSE;



            pJitLock = pAssembly->GetJitLock();

            // Enter the global lock which protects the list of all functions being JITd
            pJitLock->Enter();
            bJitLockHeld = TRUE;

            // It is possible that another thread stepped in before we entered the global lock for the first time.
            if (pMD->IsJitted())
            {
                // We came in to jit but someone beat us so return the jitted method!
                pJitLock->Leave();
                bJitLockHeld = FALSE;

                pstub = (Stub*)pMD->GetAddrofJittedCode();
                COMPLUS_LEAVE;
            }

            EE_TRY_FOR_FINALLY 
            {
                pEntry = (DeadlockAwareLockedListElement *) pJitLock->Find(pMD);

                // The function is not currently being jitted.
                if (pEntry == NULL)
                {
                    // Did not find an entry for this function, so create one
                    pEntry = new DeadlockAwareLockedListElement();
                    if (pEntry == NULL)
                    {
                        pJitLock->Leave();
                        bJitLockHeld = FALSE;

                        COMPlusThrowOM();
                    }

                    pEntry->AddEntryToList(pJitLock, pMD);
                    pEntry->m_hrResultCode = S_FALSE;

                    // Take the entries lock.  This should always succeed since we're holding the global lock.
                    bEntryLockSucceed = pEntry->DeadlockAwareEnter();
                    _ASSERTE(bEntryLockSucceed); 

                    pMD->GetModule()->LogMethodLoad(pMD);

                    // Leave global lock
                    pJitLock->Leave();
                    bJitLockHeld = FALSE;
                }
                else 
                {
                    // Someone else was JITing the function

                    // Refcount ourselves as waiting for it
                    pEntry->m_dwRefCount++;

                    // Leave global lock
                    pJitLock->Leave();
                    bJitLockHeld = FALSE;

                    bEntryLockSucceed = pEntry->DeadlockAwareEnter();
                    if (!bEntryLockSucceed)
                    {
                        //
                        // Taking this lock would cause a deadlock (presumably because we
                        // are involved in a class constructor circular dependency.)  For
                        // instance, another thread may be waiting to run the class constructor
                        // that we are jitting, but is currently jitting this function.
                        // 
                        // To remedy this, we want to go ahead and do the jitting anyway.
                        // The other threads contending for the lock will then notice that
                        // the jit finished while they were running class constructors, and abort their
                        // current jit effort.
                        //
                        // Anyway I guess we don't have to do anything special right here since we 
                        // can check pMD->IsJitted() to detect this case later.
                        //
                    }
                }

                // It is possible that another thread stepped in before we entered the global lock for the first time.
                if (!pMD->IsJitted())
                {

#ifdef PROFILING_SUPPORTED
                    // If profiling, need to give a chance for a tool to examine and modify
                    // the IL before it gets to the JIT.  This allows one to add probe calls for
                    // things like code coverage, performance, or whatever.
                    if (CORProfilerTrackJITInfo())
                    {
                        g_profControlBlock.pProfInterface->JITCompilationStarted((ThreadID) GetThread(),
                                                                                 (FunctionID) pMD,
                                                                                 TRUE);

                        // The profiler may have changed the code on the callback.  Need to
                        // pick up the new code.  Note that you have to be fully trusted in
                        // this mode and the code will not be verified.
                        COR_ILMETHOD *pilHeader = pMD->GetILHeader();
                        new (ILHeader) COR_ILMETHOD_DECODER(pilHeader, pMD->GetMDImport());
                    }
#endif // PROFILING_SUPPORTED

                    pstub = JITFunctionHelper(pMD, ILHeader, &fisEJitted, pEntry, pThrowable);
                }

                if (pstub)
                {
                    if (fGenerateUpdateableStub)
                    {
                        if (UpdateableMethodStubManager::CheckIsStub(pMD->GetAddrofCode(), NULL))
                            pstub = UpdateableMethodStubManager::UpdateStub((Stub*)pMD->GetAddrofJittedCode(), (BYTE*)pstub);
                        else
                            pstub = UpdateableMethodStubManager::GenerateStub((BYTE*)pstub);
                    }


                    pMD->SetAddrofCode((BYTE*)pstub);

                    pEntry->m_hrResultCode = S_OK;

#ifdef PROFILING_SUPPORTED
                    // Notify the profiler that JIT completed.  Must do this after the
                    // address has been set.
                    if (CORProfilerTrackJITInfo())
                    {
                        g_profControlBlock.pProfInterface->
                        JITCompilationFinished((ThreadID) GetThread(), (FunctionID) pMD,
                                               pEntry->m_hrResultCode, !fisEJitted);
                    }
#endif // PROFILING_SUPPORTED
                }
                else if (pMD->IsJitted())
                {
                    // We came in to jit but someone beat us so return the 
                    // jitted method! 

                    // We *must* use GetAddrofJittedCode here ... 
                    // if we use the pMD->GetUnsafeAddrofCode() version, 
                    // in a race condition 2 threads can come out of this 
                    // function with different return values
                    // (eg. if ENC is ON for the assembly etc)
                    pstub = (Stub*)pMD->GetAddrofJittedCode();
                }

                fSuccess = (pstub != NULL);

            }
            EE_FINALLY 
            {
                // Now decrement refcount
                if (!bJitLockHeld) {
                    pJitLock->Enter();
                    bJitLockHeld = TRUE;
                }

                // Release the method's JIT lock, if we were able to obtain it in the first place.
                if (bEntryLockSucceed) {
                    pEntry->DeadlockAwareLeave();
                    bEntryLockSucceed = FALSE;
                }

                // If we are the last waiter, delete the entry
                if (pEntry && --pEntry->m_dwRefCount == 0)
                {
                    // Unlink item from list - in reality, anyone can do this, it doesn't have to be the last waiter.
                    pJitLock->Unlink(pEntry);

                    pEntry->Destroy();
                    delete(pEntry);
                }

                pJitLock->Leave();
                bJitLockHeld = FALSE;
            }
            EE_END_FINALLY

            if (fSuccess == FALSE && !*pThrowable)
            {
                FATAL_EE_ERROR();
            }

            // if this is a method of any sort then we want to backpatch the vtable this came from
            if (pstub && !fIntercepted)
            {
                DoBackpatch(pMD, pstub, pDispatchingMT);
            }
        }
        else
        {
            if (!((pMD->IsECall()) || (pMD->IsNDirect())))
                // This is a method type we don't handle yet
                FATAL_EE_ERROR();
        }
    }
    COMPLUS_CATCH
    {
        *pThrowable = GETTHROWABLE();
        pstub = NULL;
    }
    COMPLUS_END_CATCH

    return pstub; // CHANGE, VC6.0
}

//==========================================================================
// This function is logically part of PreStubWorker(). The only reason
// it's broken out into a separate function is that StubLinker has a destructor
// and thus, we must put an inner COMPLUS_TRY clause to trap any
// COM+ exceptions that would otherwise bypass the StubLinker destructor.
// Because COMPLUS_TRY is based on SEH, VC won't let us use it in the
// same function that declares the StubLinker object.
//==========================================================================
Stub *MakeStubWorker(MethodDesc *pMD, CPUSTUBLINKER *psl, OBJECTREF *pThrowable)
{

    // Note: this should be kept idempotent ... in the sense that
    // if multiple threads get in here for the same pMD 
    // it should not matter whose stub finally gets used. This applies
    // to all the helper functions this calls!

    Stub *pstub = NULL;  // CHANGE, VC6.0

    COMPLUS_TRY
    {
        if (pMD->IsIL())
        {
            _ASSERTE(!"Could not JIT method");
            FATAL_EE_ERROR();
            pstub = 0;
        }
        else if (pMD->IsECall())
        {

            LOG((LF_LOADER, LL_INFO1000, "Made ECALL stub for method '%s.%s'\n",
                pMD->GetClass()->m_szDebugClassName,
                pMD->GetName()));
            pstub = ECall::GetECallMethodStub(psl, (ECallMethodDesc*)pMD);
        }
        else if (pMD->IsNDirect())
        {
            LOG((LF_LOADER, LL_INFO1000, "Made NDirect stub for method '%s.%s'\n",
                pMD->GetClass()->m_szDebugClassName,
                pMD->GetName()));
            pstub = NDirect::GetNDirectMethodStub(psl, (NDirectMethodDesc*)pMD);
        }
        else if (pMD->IsEEImpl())
        {
            LOG((LF_LOADER, LL_INFO1000, "Made EEImpl stub for method '%s'\n",
                pMD->GetName()));
            _ASSERTE(pMD->GetClass()->IsAnyDelegateClass());
            pstub = COMDelegate::GetInvokeMethodStub(psl, (EEImplMethodDesc*)pMD);
        }
        else
        {
            // This is a method type we don't handle yet
            FATAL_EE_ERROR();
        }

    }
    COMPLUS_CATCH
    {
        UpdateThrowable(pThrowable);
        pstub = NULL;
    }
    COMPLUS_END_CATCH
    return pstub;  // CHANGE, VC6.0
}

// helper to replace the prestub with a more appropriate stub
void InterLockedReplacePrestub(MethodDesc* pMD, Stub* pStub)
{
    _ASSERTE(pMD != NULL);
    // At this point, we've either thrown an exception or we have a stub.
    _ASSERTE(pStub != NULL);

    // Now, try to replace ThePreStub with the stub. We have to be careful
    // here because it's possible for two threads to be running the
    // prestub simultaneously. We use InterlockCompareExchange to ensure
    // that we don't replace a previously replaced stub.

    const BYTE* entry = pStub->GetEntryPoint();
    const BYTE* expected = ThePreStub()->GetEntryPoint();

    if (setCallAddrInterlocked(pMD, entry, expected) != expected)
    {
        {
            // 
            // Somebody else beat us there -- throw away our stub. :-(
            //

            pStub->DecRef();
        }
    }
}

/* Make a stub that for a value class method that expects a BOXed this poitner */

// CTS: BIG hole if pMD is a method impl that has implemented more then one method
// on this value class!!!
Stub *MakeUnboxStubWorker(MethodDesc *pMD, CPUSTUBLINKER *psl, OBJECTREF *pThrowable)
{
    // Note: this should be kept idempotent ... in the sense that
    // if multiple threads get in here for the same pMD 
    // it should not matter whose stuff finally gets used.

    Stub *pstub = NULL;

    COMPLUS_TRY
    {
        MethodDesc *pUnboxedMD = pMD->GetClass()->GetMethodDescForUnboxingValueClassMethod(pMD);

        _ASSERTE(pUnboxedMD != 0 && pUnboxedMD != pMD);

        psl->EmitUnboxMethodStub(pUnboxedMD);
        pstub = psl->Link(pMD->GetClass()->GetClassLoader()->GetStubHeap());
    }
    COMPLUS_CATCH
    {
        UpdateThrowable(pThrowable);
        pstub = NULL;
    }
    COMPLUS_END_CATCH

    return pstub;
}

//=============================================================================
// This function generates the real code for a method and installs it into
// the methoddesc. Usually ***BUT NOT ALWAYS***, this function runs only once
// per methoddesc. In addition to installing the new code, this function
// returns a pointer to the new code for the prestub's convenience.
//=============================================================================
extern "C" const BYTE * __stdcall PreStubWorker(PrestubMethodFrame *pPFrame)
{
    MethodDesc *pMD = pPFrame->GetFunction();
    MethodTable *pDispatchingMT = NULL;

    if (pMD->IsVtableMethod() && !pMD->GetClass()->IsValueClass())
    {
        OBJECTREF curobj = GetActiveObject(pPFrame);
        if (curobj != 0)
            pDispatchingMT = curobj->GetMethodTable();
    }

    return pMD->DoPrestub(pDispatchingMT);
}

// Separated out the body of PreStubWorker for the case where we don't have a frame
const BYTE * MethodDesc::DoPrestub(MethodTable *pDispatchingMT)
{
    BOOL bBashCall = FALSE;         // convert MD's CALL Prestub to JMP Code?
    BOOL bIsCode = FALSE;           // pStub is pointer to code, not to a Stub
    DWORD dwSecurityFlags = 0;
    BOOL   fRemotingIntercepted = 0;
    THROWSCOMPLUSEXCEPTION();
    OBJECTREF     throwable = NULL;
    BOOL fMustReturnPreStubCallAddr = FALSE;

    Stub *pStub = NULL;

    // Make sure the class is restored
    MethodTable *pMT = GetMethodTable();
    Module* pModule = GetModule();
    pMT->CheckRestore();
    
    // We better be in cooperative mode
    _ASSERTE(GetThread()->PreemptiveGCDisabled());
#ifdef _DEBUG  
    static unsigned ctr = 0;
    ctr++;

    if (g_pConfig->ShouldPrestubHalt(this))
    {
        _ASSERTE(!"PreStubHalt");
    }
    LOG((LF_CLASSLOADER, LL_INFO10000, "In PreStubWorker for %s::%s\n", 
                m_pszDebugClassName, m_pszDebugMethodName));

    // Useful to test GC with the prestub on the call stack
    if (g_pConfig->ShouldPrestubGC(this))
    {
        GCInterface::CollectGeneration(-1);
    }
#endif


#ifdef STRESS_HEAP
        // Force a GC on every jit if the stress level is high enough
    if (g_pConfig->GetGCStressLevel() != 0
#ifdef _DEBUG
        && !g_pConfig->FastGCStressLevel()
#endif
        )
        g_pGCHeap->StressHeap();
#endif


    /**************************   SECURITY   *************************/
    //--------------------------------------------------------------------

    // If the function desc is an method then check to see if it has security
    // properties. If it does then set the flag so the Native addr's get set
    // correctly in the Jitted case and a Security Interceptor is created.

    if(Security::IsSecurityOn())
        dwSecurityFlags = GetSecurityFlags();

    // check if remoting needs to intercept this call
    fRemotingIntercepted = IsRemotingIntercepted();

    /**************************   BACKPATCHING   *************************/
    // See if the addr of code is the pre-stub && that the method has been jitted
    if ((GetUnsafeAddrofCode() != GetCallablePreStubAddr()) && (IsIL() || MustBeFCall()))
    {
        LOG((LF_CLASSLOADER, LL_INFO10000, "    In PreStubWorker, method already jitted, backpatching call point\n"));

        // Can we backpatch?
        // Only back packpatch here if it is a virtual call.
        if (pDispatchingMT != NULL)
        {
            // if it is not the pre stub then we are calling on this method 
            // from a vtable that hasn't been fixed up yet although we have 
            // already jitted the method - we need to backpatch

            // We should not get here for methods where the slot in
            // the method table that the MethodDesc is defined on has not
            // been backpatched.
            //
            // Actually, we cannot assert the following because of race conditions.
            // MakeJitWorker will actually do the SetAddrOfCode before it does the
            // VTable patching, so there's a small window where we might notice the
            // following is violated.

            // If we have backpatched the main slot for this method.  If not,
            // do so, if so, backpatch duplicates.
            
            if ((pDispatchingMT->GetVtable())[GetSlot()] == (SLOT)GetPreStubAddr())
            {
                (pDispatchingMT->GetVtable())[GetSlot()] = (SLOT)GetAddrofJittedCode();
            }                
            else
            {
                // The location of the duplicate slots is not guaranteed to be after the 
                // slot number containing method desc if the method desc is a method impl
                // Use the information stored with the method impl to finish the backpatch
                if(IsMethodImpl()) 
                {
                    MethodImpl* pImpl = MethodImpl::GetMethodImplData(this);
                    DWORD numslots = pImpl->GetSize();
                    DWORD* slots = pImpl->GetSlots();
                    for(DWORD sl = 0; sl < numslots; sl++) 
                    {
                        if ((pDispatchingMT->GetVtable())[slots[sl]] == (SLOT)GetPreStubAddr())
                        {
                            (pDispatchingMT->GetVtable())[slots[sl]] = (SLOT)GetAddrofJittedCode();
                        }
                    }
                }
                else 
                {
                    // We have a slot in the vtable that seems to already been backpatched or
                    // is not pointing at the this method's methoddesc.
                    // We must have called through a duplicate slot
                    // Walk the vtable looking for the current methoddesc
                    // if we find it  - backpatch it!
                    int numslots = (pDispatchingMT->GetClass())->GetNumVtableSlots();
                    for( int dupslot = 0 ;dupslot < numslots ; dupslot++ )
                    {
                        if ((pDispatchingMT->GetVtable())[dupslot] == (SLOT)GetPreStubAddr())
                        {
                            (pDispatchingMT->GetVtable())[dupslot] = (SLOT)GetAddrofJittedCode();
                        }
                    }
                }
            }
        
        }

        const BYTE *pbDest = GetAddrofJittedCode();
        
        return pbDest;
    }

    // 
    // Make sure .cctor has been run
    //
    GCPROTECT_BEGIN (throwable);
    if (pMT->CheckRunClassInit(&throwable) == FALSE)
        COMPlusThrow(throwable);
    
    /**************************   CODE CREATION  *************************/
    if (IsUnboxingStub()) 
    {
        CPUSTUBLINKER sl;
        pStub = MakeUnboxStubWorker(this, &sl, &throwable);
        bBashCall = TRUE;
    }
    else if (IsIL()) 
    {

        //
        // If not, try to jit it
        //

        if (pStub == NULL)
        {
            // Get the information on the method
            BOOL fMustFreeIL = FALSE;
            COR_ILMETHOD* ilHeader = GetILHeader();
			bool verify = !Security::LazyCanSkipVerification(pModule);
            COR_ILMETHOD_DECODER header(ilHeader, pModule->GetMDImport(), verify);
			if(verify && header.Code)
			{
				IMAGE_DATA_DIRECTORY dir;
				dir.VirtualAddress = VAL32(GetRVA());
				dir.Size = VAL32(header.GetCodeSize() + (header.EH ? header.EH->DataSize() : 0));
				if (pModule->IsPEFile() &&
                    (FAILED(pModule->GetPEFile()->VerifyDirectory(&dir,IMAGE_SCN_MEM_WRITE))))
                        header.Code = 0;
			}
            BAD_FORMAT_ASSERT(header.Code != 0);
            if (header.Code == 0)
                COMPlusThrowHR(COR_E_BADIMAGEFORMAT);



            // JIT it
            if (g_pConfig->ShouldJitMethod(this) || g_pConfig->ShouldEJitMethod(this))
            {
                LOG((LF_CLASSLOADER, LL_INFO10000, 
                     "    In PreStubWorker, calling MakeJitWorker\n"));
    
                // MakeJit worker uses a combination of the security flag, 
                // Edit and continue flag and RemotingIntercepted flag to 
                // determine whether or not to set the return address
                // (i.e. to do backpatching).
                
                // For Edit&Continue scenario ... (i.e. pMD belongs to a module
                // which was built for Edit&Continue .. this is kind of default
                // in debug builds) this function will return a stub that has 
                // already wrapped the actual native code 
                // (in which case m_dwCodeOrIL also represents the updateable 
                // EnC stub)
                pStub = MakeJitWorker(this,
                                      &header,
                                      (dwSecurityFlags != 0) || 
                                          fRemotingIntercepted ||
                                          this->IsEnCMethod(),
                                      pModule->SupportsUpdateableMethods(),
                                      pDispatchingMT,
                                      &throwable);
                                      
                // Security and/or Remoting may want to build stubs that hold 
                // the actual Jitted stub ... eventually we do an 
                // InterlockedExchange the code at GetPreStubAddr() with a 
                // call to the 'final' (outermost) stub. 
                // The call to MakeJitWorker above had better not return 
                // the same value as GetPreStubAddr() .. or else we will 
                // end up with code with an infinite loop! (hence this assert)
                // Note: pStub may be NULL if an exception happened during JIT
                
                _ASSERTE(pStub==NULL ||
                        !IsJitted()  ||
                        (IsJitted() && (((BYTE*)pStub) != GetLocationOfPreStub()))
                        );
                        
                if (!IsJitted())
                {
                    // In the rare case where a profiler causes the function to
                    // be un-jitted in the JitCompilationFinished notification
                    // we should not wrap pStub with the remoting stub ... 
                    // Since in such a case the above call will return 
                    // a mini-stub that does a "jmp GetPreStubAddr()" ... if
                    // remoting builds a stub around that we will have 
                    // the same infinite loop problem
                    fRemotingIntercepted = FALSE;

                    // REVIEW: what about security stubs?
                }
                
                bBashCall = bIsCode = TRUE;
            }

            if (fMustFreeIL)
                delete (BYTE*) header.Code;

            // We have no backup plan, if jitting fails, we are toast
        }
    }
    else    //!IsUnBoxingStub() && !IsIL() case
    {
        if (IsECall()) 
            pStub = (Stub*) FindImplForMethod(this);         // See if it is an FCALL
       

        if (pStub != 0)
        {
            _ASSERTE(IsECall() || !(GetClass()->IsAnyDelegateClass()));
            if (!fRemotingIntercepted && !(GetClass()->IsAnyDelegateClass()))
            {
                // backpatch the main slot.  
                pMT->GetVtable()[GetSlot()] = (SLOT) pStub;
            }
            bBashCall = bIsCode = TRUE;
        }
        else 
        {
            // do all the other stubs. 
            if (IsNDirect() && (!pModule->GetSecurityDescriptor()->CanCallUnmanagedCode(&throwable)))
                COMPlusThrow(throwable);
            CPUSTUBLINKER sl;
            pStub = MakeStubWorker(this, &sl, &throwable);
            fMustReturnPreStubCallAddr = TRUE;
        }
    }

    /**************************   CLEANUP / POSTJIT *************************/
    if (!pStub)
        COMPlusThrow(throwable);

    
    // Lets check to see if we need declarative security on this stub, If we have
    // security checks on this method or class then we need to add an intermediate
    // stub that performs declarative checks prior to calling the real stub.
    if(dwSecurityFlags != 0) {
        CPUSTUBLINKER sl;

        LOG((LF_CORDB, LL_INFO10000,
             "::PSW: Placing security interceptor before real stub 0x%08x\n",
             pStub));

        Stub *pCurrentStub = pStub;
        if(bIsCode)
            pStub = MakeSecurityWorker(&sl, this, dwSecurityFlags, NULL, (LPVOID) pStub, &throwable);
        else
            pStub = MakeSecurityWorker(&sl, this, dwSecurityFlags, pStub, (LPVOID) pStub->GetEntryPoint(), &throwable);
        if (!pStub)
        {
            // If there's no throwable, it's just MakeSecurityWorker telling us
            // (in the case where we're wrapping jitted code) that there was no
            // need to create an interceptor after all.
            if (throwable == NULL)
            {
                _ASSERTE(bIsCode);
                pStub = pCurrentStub;
            }
            else
            COMPlusThrow(throwable);
        }

        LOG((LF_CORDB, LL_INFO10000,
             "::PSW security interceptor stub 0x%08x\n",pStub));            
       
        // Check if a security interceptor was indeed created
        if (pCurrentStub != pStub)
        {
            bBashCall = bIsCode = FALSE;
        }
        else
            // We already marked the method as intercepted speculatively, back
            // out from that decision. Any caller that saw the intermediate
            // value will just go through a harmless extra level of indirection.
            SetIntercepted(FALSE);
    }

    // check for MarshalByRef scenarios ... we need to intercept
    // Non-virtual calls on MarshalByRef types
    if (fRemotingIntercepted)
    {   
        Stub* pCurrentStub = pStub;
        // find the actual address to jump to
        LPVOID pvAddrOfCode = (bIsCode) ? (LPVOID)pStub : (LPVOID)pStub->GetEntryPoint();
        Stub* pInnerStub  = (bIsCode) ? NULL : pStub;
        
        // let us setup a remoting stub to intercept all the calls
        pStub = CRemotingServices::GetStubForNonVirtualMethod(this, pvAddrOfCode, pInnerStub); // throws

        if (pCurrentStub != pStub)
        {
            bBashCall = bIsCode = FALSE;
            fMustReturnPreStubCallAddr = TRUE;
        }
    }

    //************************  BACKPATCH THE PRESTUB CALL AREA ************
    if (!bBashCall)
    {
        // Function was something other than an IL or an FCall.
        // Replace "call prestub" with "call realstub"

#ifdef DEBUGGING_SUPPORTED
        //
        // Tell the debugger that the function is now ready to run.
        //
        if ((g_pDebugInterface != NULL) && (IsIL()))
            g_pDebugInterface->FunctionStubInitialized(this, (const BYTE *)pStub);
#endif // DEBUGGING_SUPPORTED

        LOG((LF_CORDB, LL_EVERYTHING,
             "Backpatching prestub call to 0x%08x for %s::%s\n", pStub,
             (m_pszDebugClassName!=NULL)?(m_pszDebugClassName):("<Global Namespace>"),
             m_pszDebugMethodName));
        
        InterLockedReplacePrestub(this,pStub);
    }
    else
    {
        BYTE* codeAddr = 0;

        if (IsUnboxingStub())
        {
            codeAddr = (BYTE*)pStub->GetEntryPoint();
        }
        else if (IsJitted())
        {
            // The profiler can cause the IL function to become unjitted again:
            // The above test checks to see if that happened. The control flow
            // paths probably need to be rethought here...
            codeAddr = (BYTE*)GetAddrofJittedCode();
        }

        if (codeAddr != 0)
        {
            // Function was an IL or an FCall.
            // Replace "call prestub" with "jmp code"

            InterLockedShortcircuitPrestub(this,codeAddr);
        }
    }

    GCPROTECT_END();

    if (fMustReturnPreStubCallAddr)
    {
        return GetCallablePreStubAddr();
    }
    else
    {
        return GetUnsafeAddrofCode();
    }
}

//==========================================================================
// The following code manages the PreStub. All method stubs initially
// use the prestub. Note that method's do not IncRef the prestub as they
// do their regular stubs. This PreStub is permanent.
//==========================================================================
static Stub *g_preStub = NULL;
static Stub *g_UMThunkPreStub = NULL;

//-----------------------------------------------------------
// Stub manager for the prestub.  Although there is just one, it has
// unique behavior so it gets its own stub manager.
//-----------------------------------------------------------

class ThePreStubManager : public StubManager
{
  public:
    ThePreStubManager(const BYTE *address) : m_prestubAddress(address) {}

    BOOL CheckIsStub(const BYTE *stubStartAddress)
    {
        return stubStartAddress == m_prestubAddress;
    }

    BOOL DoTraceStub(const BYTE *stubStartAddress, TraceDestination *trace)
    {
        //
        // We cannot tell where the stub will end up
        // until after the prestub worker has been run.
        //

        Stub *stub = Stub::RecoverStub((const BYTE *)stubStartAddress);

        trace->type = TRACE_FRAME_PUSH;
        trace->address = ((const BYTE*) stubStartAddress) + stub->GetPatchOffset();

        return TRUE;
    }
    MethodDesc *Entry2MethodDesc(const BYTE *stubStartAddress, MethodTable *pMT) {return NULL;}
    const BYTE *m_prestubAddress;

    static ThePreStubManager *g_pManager;

    static BOOL Init()
    {
        //
        // Add the prestub manager
        //

        g_pManager = new ThePreStubManager((const BYTE *) g_preStub->GetEntryPoint());
        if (g_pManager == NULL)
            return FALSE;

        StubManager::AddStubManager(g_pManager);

        return TRUE;
    }

};

ThePreStubManager *ThePreStubManager::g_pManager = NULL;

//-----------------------------------------------------------
// Initialize the prestub.
//-----------------------------------------------------------
BOOL InitPreStubManager()
{
    BOOL ret = FALSE;

    g_preStub = GeneratePrestub();
    g_UMThunkPreStub = GenerateUMThunkPrestub();

    ret = ThePreStubManager::Init();

    return ret;
}


//-----------------------------------------------------------
// Destroy the prestub.
//-----------------------------------------------------------


//-----------------------------------------------------------
// Access the prestub (NO incref.)
//-----------------------------------------------------------
Stub *ThePreStub()
{
    return g_preStub;
}

Stub *TheUMThunkPreStub()
{
    return g_UMThunkPreStub;
}

void CallDefaultConstructor(OBJECTREF ref)
{
    THROWSCOMPLUSEXCEPTION();

    MethodTable *pMT = ref->GetTrueMethodTable();

    if (!pMT->HasDefaultConstructor())
    {
        MAKE_WIDEPTR_FROMUTF8(wzMethodName, COR_CTOR_METHOD_NAME);
        COMPlusThrowNonLocalized(kMissingMethodException, wzMethodName);
    }

    MethodDesc *pMD = pMT->GetDefaultConstructor();

    static MetaSig *sig = NULL;
    if (sig == NULL)
    {
        // Allocate a metasig to use for all default constructors.
        void *tempSpace = SystemDomain::Loader()->GetHighFrequencyHeap()->AllocMem(sizeof(MetaSig));
        sig = new (tempSpace) MetaSig(gsig_IM_RetVoid.GetBinarySig(), SystemDomain::SystemModule());
    }

    ARG_SLOT arg = ObjToArgSlot(ref);

    pMD->Call(&arg, sig);
}

ARG_SLOT CallConstructor(LPHARDCODEDMETASIG szMetaSig, const ARG_SLOT *pArgs)
{
    THROWSCOMPLUSEXCEPTION();

    OBJECTREF pThis = ArgSlotToObj(pArgs[0]);

    _ASSERTE(pThis != 0 && "about to call a null pointer, guess what's going to happen next");

    MethodDesc *pMD = pThis->GetTrueClass()->FindMethod(COR_CTOR_METHOD_NAME, szMetaSig);
    if (!pMD)
    {
        MAKE_WIDEPTR_FROMUTF8(wzMethodName, COR_CTOR_METHOD_NAME);
        COMPlusThrowNonLocalized(kMissingMethodException, wzMethodName);
    }
    return pMD->Call(pArgs);
}


