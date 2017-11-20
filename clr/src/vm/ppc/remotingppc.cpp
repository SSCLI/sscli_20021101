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
** File:    remotingppc.cpp
**       
** Purpose: Defines various remoting related functions for the ppc architecture
**
** Date:    Oct 12, 1999
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
#include "wsperf.h"
#include "threads.h"
#include "method.hpp"
#include "asmconstants.h"

#include "interoputil.h"


extern "C" size_t g_dwTPStubAddr; // This is the external entrypoint - expects MethodDesc
extern "C" size_t g_dwOOContextAddr;

size_t g_dwTPStubAddr;
size_t g_dwOOContextAddr;

// External variables
extern DWORD g_dwNonVirtualThunkRemotingLabelOffset;
extern DWORD g_dwNonVirtualThunkReCheckLabelOffset;

extern "C" void __stdcall CRemotingServices__DispatchInterfaceCall(MethodDesc *pMD);

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::GenerateCheckForProxy   public
//
//  Synopsis:   This code generates a check to see if the "this" pointer
//              is a proxy. If so, the interface invoke is handled via
//              the CRemotingServices::DispatchInterfaceCall else we 
//              delegate to the old path
//+----------------------------------------------------------------------------
void CRemotingServices::GenerateCheckForProxy(CPUSTUBLINKER* psl)
{
    // This must be the remoting proxy
    psl->PPCEmitLoadImm(kR12, (__int32)(size_t)CRemotingServices__DispatchInterfaceCall);

    // mtctr r12, bctr
    psl->PPCEmitBranchR12();
}


//+----------------------------------------------------------------------------
//
//  Method:     CTPMethodTable::CreateThunkForVirtualMethod   private
//
//  Synopsis:   Creates the thunk that pushes the supplied slot number and jumps
//              to TP Stub
//+----------------------------------------------------------------------------
void CTPMethodTable::CreateThunkForVirtualMethod(DWORD dwSlot, BYTE *startaddr)
{
    DWORD *pCode = (DWORD*)startaddr;

    _ASSERTE(NULL != s_pTPStub);
    UINT32 addr = (UINT32)(UINT_PTR)s_pTPStub->GetEntryPoint();

    // addis r12, 0, (addr>>16)
    *pCode++ = 0x3D800000 | (addr >> 16);
    // ori r12,r12,addr
    *pCode++ = 0x618C0000 | (UINT16)addr;

    // The high 16 bits of r11 can contain a bogus sign extension because of this
    _ASSERTE(s_dwMaxSlots <= 65536);
    _ASSERTE(dwSlot < 65536);

    // li r11, value
    *pCode++ = 0x39600000 | dwSlot;

    // mtctr r12
    *pCode++ = 0x7D8903A6;

    // bctr
    *pCode++ = 0x4E800420;

    _ASSERTE(CVirtualThunkMgr::IsThunkByASM(startaddr));
}




//+----------------------------------------------------------------------------
//
//  Method:     CTPMethodTable::CreateStubForNonVirtualMethod   public
//
//  Synopsis:   Create a stub for a non virtual method
//+----------------------------------------------------------------------------

Stub* CTPMethodTable::CreateStubForNonVirtualMethod(MethodDesc* pMD, CPUSTUBLINKER* psl, 
                                            LPVOID pvAddrOfCode, Stub* pInnerStub)
{
    // Sanity check
    THROWSCOMPLUSEXCEPTION();

    RuntimeExceptionKind reException = kLastException;
    BOOL fThrow = FALSE;
    Stub *pStub = NULL;    

    // we need a hash table only for virtual methods
    _ASSERTE(!pMD->IsVirtual());

    if(!s_fInitializedTPTable)
    {
        if(!InitializeFields())
        {
            reException = kExecutionEngineException;
            fThrow = TRUE;
        }
    }

    MetaSig msig(pMD);
    PPCReg regThis = msig.HasRetBuffArg() ? kR4 : kR3;
       
    if (!fThrow)
    {

        COMPLUS_TRY
        {           
            // The thunk has not been created yet. Go ahead and create it.    

            // Generate label where a null reference exception will be thrown
            CodeLabel *pJmpAddrLabel = psl->NewCodeLabel();
            // Generate label where remoting code will execute
            CodeLabel *pRemotingLabel = psl->NewCodeLabel();

            // if this == NULL throw NullReferenceException
            psl->PPCEmitCmpwi(regThis, 0);
            psl->PPCEmitBranch(pJmpAddrLabel, PPCCondCode::kEQ);

            // Emit a label here for the debugger. A breakpoint will
            // be set at the next instruction and the debugger will
            // call CNonVirtualThunkMgr::TraceManager when the
            // breakpoint is hit with the thread's context.
            CodeLabel *pRecheckLabel = psl->NewCodeLabel();
            psl->EmitLabel(pRecheckLabel);
        
            // If this.MethodTable != TPMethodTable then do RemotingCall
            psl->PPCEmitLwz(kR12, regThis);
            psl->PPCEmitLoadImm(kR0, (__int32)(size_t)GetMethodTable());
            psl->PPCEmitCmpw(kR0, kR12);

            // marshalbyref case
            psl->PPCEmitBranch(pJmpAddrLabel, PPCCondCode::kNE);

            // erect stack frame before making the call

            // mflr r0
            psl->Emit32(0x7C0802A6);
            // stw r0, 8(r1)
            psl->PPCEmitStw(kR0, kR1, 8);
            // stwu r1, -local_space(r1)
            psl->PPCEmitRegRegDisp16(0x94000000, kR1, kR1, -AlignStack(sizeof(LinkageArea)));

            // EmitCallToStub(psl, pRemotingLabel)
            // Move into r0 the stub data and call the stub
            psl->PPCEmitLwz(kR12, regThis, TP_OFFSET_STUB);
            psl->PPCEmitLwz(kR0, regThis, TP_OFFSET_STUBDATA);

            // mtctr r12, bctrl
            psl->PPCEmitCallR12();

            // cmpwi r0,r0
            psl->PPCEmitCmpwi(kR0, 0);

            // restore the stack frame

            // lwz r1, 0(r1)
            psl->PPCEmitLwz(kR1, kR1, 0);
            // lwz r0, 8(r1)
            psl->PPCEmitLwz(kR0, kR1, 8);
            // mtlr r0
            psl->Emit32(0x7C0803A6);

            // bne CtxMismatch
            psl->PPCEmitBranch(pRemotingLabel, PPCCondCode::kNE);

            // Exception handling and non-remoting share the 
            // same codepath
            psl->EmitLabel(pJmpAddrLabel);
            psl->PPCEmitLoadImm(kR12, (__int32)(size_t)pvAddrOfCode);

            // mtctr r12, bctr
            psl->PPCEmitBranchR12();
            
            psl->EmitLabel(pRemotingLabel);
                  
            // the MethodDesc should be still in R11.  goto TPStub
            // jmp TPStub
            psl->PPCEmitLoadImm(kR12, (__int32)(size_t)g_dwTPStubAddr);
            psl->PPCEmitBranchR12();

            // Link and produce the stub
            pStub = psl->LinkInterceptor(pMD->GetClass()->GetDomain()->GetStubHeap(),
                                           pInnerStub, pvAddrOfCode);        
        }
        COMPLUS_CATCH
        {
            reException = kOutOfMemoryException;
            fThrow = TRUE;
        }                       
        COMPLUS_END_CATCH
    }
    
    // Check for the need to throw exceptions
    if(fThrow)
    {
        COMPlusThrow(reException);
    }
    
    _ASSERTE(NULL != pStub);
    return pStub;
}

//+----------------------------------------------------------------------------
//
//  Synopsis:   Find an existing thunk or create a new one for the given 
//              method descriptor. NOTE: This is used for the methods that do 
//              not go through the vtable such as constructors, private and 
//              final methods.
//+----------------------------------------------------------------------------
LPVOID CTPMethodTable::GetOrCreateNonVirtualThunkForVirtualMethod(MethodDesc* pMD, CPUSTUBLINKER* psl)
{
    // Sanity check
    THROWSCOMPLUSEXCEPTION();

    RuntimeExceptionKind reException = kLastException;
    BOOL fThrow = FALSE;

    Stub *pStub = NULL;
    LPVOID *pvThunk = NULL;

    if(!s_fInitializedTPTable)
    {
        if(!InitializeFields())
        {
            reException = kExecutionEngineException;
            fThrow = TRUE;
        }
    }

    // Create the thunk in a thread safe manner
    LOCKCOUNTINCL("GetOrCreateNonVirtualThunk in remotingppc.cpp");
    EnterCriticalSection(&s_TPMethodTableCrst);

    COMPLUS_TRY
    {
        // Check to make sure that no other thread has 
        // created the thunk
        _ASSERTE(NULL != s_pThunkHashTable);
    
        s_pThunkHashTable->GetValue(pMD, (HashDatum *)&pvThunk);
    
        if((NULL == pvThunk) && !fThrow)
        {
            MetaSig msig(pMD);
            PPCReg regThis = msig.HasRetBuffArg() ? kR4 : kR3;

            // The thunk has not been created yet. Go ahead and create it.    
            EEClass* pClass = pMD->GetClass();                
            // Compute the address of the slot         
            LPVOID pvSlot = (LPVOID)pClass->GetMethodSlot(pMD);
    
            // Generate label where a null reference exception will be thrown
            CodeLabel *pExceptionLabel = psl->NewCodeLabel();

            //  !!! WARNING WARNING WARNING WARNING WARNING !!!
            //
            //  DO NOT CHANGE this code without changing the thunk recognition
            //  code in CNonVirtualThunkMgr::IsThunkByASM 
            //  & CNonVirtualThunkMgr::GetMethodDescByASM
            //
            //  !!! WARNING WARNING WARNING WARNING WARNING !!!
            
            // if this == NULL throw NullReferenceException
            psl->PPCEmitCmpwi(regThis, 0);
            psl->PPCEmitBranch(pExceptionLabel, PPCCondCode::kEQ);
    
            // Generate label where remoting code will execute
            CodeLabel *pRemotingLabel = psl->NewCodeLabel();
    
            // Emit a label here for the debugger. A breakpoint will
            // be set at the next instruction and the debugger will
            // call CNonVirtualThunkMgr::TraceManager when the
            // breakpoint is hit with the thread's context.
            CodeLabel *pRecheckLabel = psl->NewCodeLabel();
            psl->EmitLabel(pRecheckLabel);

            // If this.MethodTable != TPMethodTable then do RemotingCall
            psl->PPCEmitLwz(kR12, regThis);
            psl->PPCEmitLoadImmNonoptimized(kR0, (__int32)(size_t)GetMethodTable());
            psl->PPCEmitCmpw(kR0, kR12);

            psl->PPCEmitBranch(pRemotingLabel, PPCCondCode::kEQ);
    
            // Exception handling and non-remoting share the 
            // same codepath
            psl->EmitLabel(pExceptionLabel);
    
            // Non-RemotingCode
            // Jump to the vtable slot of the method
            psl->PPCEmitLoadImmNonoptimized(kR12, (DWORD)(size_t)pvSlot);
            psl->PPCEmitLwz(kR12, kR12);
            psl->PPCEmitBranchR12();

            // Remoting code. Note: CNonVirtualThunkMgr::TraceManager
            // relies on the arrangement of instructions around this label.
            // If you change any instructions after the pvSlot loadimm
            // above and before the MethodDesc loadimm below, update
            // CNonVirtualThunkMgr::DoTraceStub and TraceManager.
            psl->EmitLabel(pRemotingLabel);
    
            // Save the MethodDesc and goto TPStub
            // push MethodDesc
            psl->PPCEmitLoadImmNonoptimized(kR12, (DWORD)(size_t)g_dwTPStubAddr);
            psl->PPCEmitLoadImmNonoptimized(kR11, ((DWORD)(size_t)pMD) - METHOD_CALL_PRESTUB_SIZE);
            psl->PPCEmitBranchR12();
    
            // Link and produce the stub
            // FUTURE: Do we have to provide the loader heap ?
            pStub = psl->Link(SystemDomain::System()->GetHighFrequencyHeap());
    
            // Grab the offset of the RemotingLabel and RecheckLabel
            // for use in CNonVirtualThunkMgr::DoTraceStub and
            // TraceManager.
            g_dwNonVirtualThunkRemotingLabelOffset =
                psl->GetLabelOffset(pRemotingLabel);
            g_dwNonVirtualThunkReCheckLabelOffset =
                psl->GetLabelOffset(pRecheckLabel);
    
            // Set the generated thunk once and for all..            
            CNonVirtualThunk *pThunk = CNonVirtualThunk::SetNonVirtualThunks(pStub->GetEntryPoint());
    
            // Remember the thunk address in a hash table 
            // so that we dont generate it again
            pvThunk = pThunk->GetAddrOfCode();
            s_pThunkHashTable->InsertValue(pMD, (HashDatum)pvThunk);

            _ASSERTE(CNonVirtualThunkMgr::IsThunkByASM((BYTE*)*pvThunk));
            _ASSERTE(CNonVirtualThunkMgr::GetMethodDescByASM((BYTE*)*pvThunk) == pMD);
        }
    }
    COMPLUS_CATCH
    {
        reException = kOutOfMemoryException;
        fThrow = TRUE;
    }
    COMPLUS_END_CATCH

    // Leave the lock
    LeaveCriticalSection(&s_TPMethodTableCrst);    
    LOCKCOUNTDECL("GetOrCreateNonVirtualThunk in remotingppc.cpp");
    
    // Check for the need to throw exceptions
    if(fThrow)
    {
        COMPlusThrow(reException);
    }
    
    _ASSERTE(NULL != pvThunk);
    return pvThunk;
}


CPUSTUBLINKER *CTPMethodTable::NewStubLinker()
{
    return new CPUSTUBLINKER();
}

//+----------------------------------------------------------------------------
//
//  Method:     CTPMethodTable::CreateTPStub   private
//
//  Synopsis:   Creates the stub that sets up a CtxCrossingFrame and forwards the
//              call to
//+----------------------------------------------------------------------------

Stub *CTPMethodTable::CreateTPStub()
{
    THROWSCOMPLUSEXCEPTION();

    CPUSTUBLINKER *pStubLinker = NULL;

    EE_TRY_FOR_FINALLY
    {
        // Note: We are already inside a criticalsection

        if (s_pTPStub == NULL)
        {
            pStubLinker = CTPMethodTable::NewStubLinker();
            if (!pStubLinker)
                COMPlusThrowOM();

            CodeLabel *OOExternal = pStubLinker->NewCodeLabel();
            CodeLabel *ThisReg = pStubLinker->NewCodeLabel();
            CodeLabel *ConvMD = pStubLinker->NewCodeLabel();
            CodeLabel *UseCode = pStubLinker->NewCodeLabel();
            CodeLabel *OOContext = pStubLinker->NewCodeLabel();

            // The slot # is sign extended - get rid of the sign extension

            // clrlwi r11, r11, 16 == rlwinm r11, r11, 0, 16, 31
            pStubLinker->Emit32(0x556B043E);

            pStubLinker->EmitLabel(OOExternal);

            // EmitCallToStub(pStubLinker, OOContext);

            // erect stack frame before making the call

            // mflr r12
            pStubLinker->Emit32(0x7D8802A6);
            // stw r12, 8(r1)
            pStubLinker->PPCEmitStw(kR12, kR1, 8);
            // stwu r1, -local_space(r1)
            pStubLinker->PPCEmitRegRegDisp16(0x94000000, kR1, kR1, -AlignStack(sizeof(LinkageArea)));

            // get the this register: look at the callsite whether it contains
            // the magic instruction to determine the location of the this pointer

            // lwz r0, 0(r12)
            pStubLinker->PPCEmitLwz(kR0, kR12);

            // kR12 = oris r0, r0, 0
            pStubLinker->PPCEmitLoadImm(kR12, 0x64000000);
            pStubLinker->PPCEmitCmpw(kR0, kR12);

            // kR2 = this reg
            pStubLinker->PPCEmitMr(kR2, kR3);
            pStubLinker->PPCEmitBranch(ThisReg, PPCCondCode::kNE);
            pStubLinker->PPCEmitMr(kR2, kR4);
            pStubLinker->EmitLabel(ThisReg);

            // Move into kR0 the stub data and call the stub
            pStubLinker->PPCEmitLwz(kR12, kR2, TP_OFFSET_STUB);
            pStubLinker->PPCEmitLwz(kR0, kR2, TP_OFFSET_STUBDATA);

            // mtctr r12, bctrl
            pStubLinker->PPCEmitCallR12();

            // cmpwi r0,0
            pStubLinker->PPCEmitCmpwi(kR0, 0);

            // restore the stack frame

            // lwz r1, 0(r1)
            pStubLinker->PPCEmitLwz(kR1, kR1, 0);
            // lwz r0, 8(r1)
            pStubLinker->PPCEmitLwz(kR0, kR1, 8);
            // mtlr r0
            pStubLinker->Emit32(0x7C0803A6);

            // bne CtxMismatch
            pStubLinker->PPCEmitBranch(OOContext, PPCCondCode::kNE);

            // The contexts match. Jump to the real address and start executing.
            EmitJumpToAddressCode(pStubLinker, ConvMD, UseCode);

            // label: OOContext
            pStubLinker->EmitLabel(OOContext);

            // CONTEXT MISMATCH CASE, call out to the real proxy to
            // dispatch

            // Setup the frame
            EmitSetupFrameCode(pStubLinker);

            // Finally, create the stub
            s_pTPStub = pStubLinker->Link();

            g_dwTPStubAddr = (size_t)(s_pTPStub->GetEntryPoint() +
                                        pStubLinker->GetLabelOffset(OOExternal));

            // Set the address of Out Of Context case.
            // This address is used by other stubs like interface
            // invoke to jump straight to RealProxy::PrivateInvoke
            // because they have already determined that contexts 
            // don't match.
            g_dwOOContextAddr = (size_t)(s_pTPStub->GetEntryPoint() + 
                                        pStubLinker->GetLabelOffset(OOContext));
        }

        if(NULL != s_pTPStub)
        {
            // Initialize the stub manager which will aid the debugger in finding
            // the actual address of a call made through the vtable
            // Note: This function can throw, but we are guarded by a try..finally
            CVirtualThunkMgr::InitVirtualThunkManager((const BYTE *) s_pTPStub->GetEntryPoint());
    
        }        
    }
    EE_FINALLY
    {
        // Cleanup
        if (pStubLinker)
            delete pStubLinker;
    }EE_END_FINALLY;

        
    return(s_pTPStub);
}

//+----------------------------------------------------------------------------
//
//  Method:     CTPMethodTable::CreateDelegateStub   private
//
//  Synopsis:   Creates the stub that sets up a CtxCrossingFrame and forwards the
//              call to PreCall
//+----------------------------------------------------------------------------
Stub *CTPMethodTable::CreateDelegateStub()
{
    THROWSCOMPLUSEXCEPTION();

    CPUSTUBLINKER *pStubLinker = NULL;

    EE_TRY_FOR_FINALLY
    {
        // Note: We are inside a critical section

        if (s_pDelegateStub == NULL)
        {
            pStubLinker = NewStubLinker();

	        if (!pStubLinker)
            {
                COMPlusThrowOM();
            }

            // Setup the frame
            EmitSetupFrameCode(pStubLinker);

            s_pDelegateStub = pStubLinker->Link();
        }
    }
    EE_FINALLY
    {
        // Cleanup
        if (pStubLinker)
            delete pStubLinker;
    }EE_END_FINALLY;

        
    return(s_pDelegateStub);
}

//+----------------------------------------------------------------------------
//
//  Method:     CTPMethodTable::EmitJumpToAddressCode   private
//
//  Synopsis:   Emits the code to extract the address from the slot or the method 
//              descriptor and jump to it.
//+----------------------------------------------------------------------------
VOID CTPMethodTable::EmitJumpToAddressCode(CPUSTUBLINKER* pStubLinker, CodeLabel* ConvMD, 
                                           CodeLabel* UseCode)
{
    // UseCode:
    pStubLinker->EmitLabel(UseCode);

    // andis. r12,r11,0xFFFF0000
    pStubLinker->Emit32(0x756CFFFF);

    // bne ConvMD
    pStubLinker->PPCEmitBranch(ConvMD, PPCCondCode::kNE);
    
    // if (!(r11 & 0xffff0000))
    // {
    
        // ** code addr from slot case **
    
        // lwz r2, (kR2)(TPMethodTable::GetOffsetOfMT())
        pStubLinker->PPCEmitLwz(kR2, kR2, TP_OFFSET_MT);

        // slwi r11, r11, 2 == rlwinm r11, r11, 2, 0, 29
        pStubLinker->Emit32(0x556B103A);

        // add r12, r2, r11
        pStubLinker->PPCEmitRegRegReg(0x7C000214, kR12, kR2, kR11);

        // lwz r12,[r12 + MethodTable::GetOffsetOfVtable()]
        pStubLinker->PPCEmitLwz(kR12, kR12, MethodTable::GetOffsetOfVtable());

        // mtctr r12, bctr
        pStubLinker->PPCEmitBranchR12();
    
    // }
    // else
    // {
        // ** code addr from MethodDesc case **

        pStubLinker->EmitLabel(ConvMD);                
        
        // mr r12, r11
        pStubLinker->PPCEmitMr(kR12, kR11);

        // mtctr r12, bctr
        pStubLinker->PPCEmitBranchR12();

    // }
}

//+----------------------------------------------------------------------------
//
//  Method:     CTPMethodTable::EmitSetupFrameCode   private
//
//  Synopsis:   Emits the code to setup a frame and call to PreCall method
//              call to PreCall
//+----------------------------------------------------------------------------
VOID CTPMethodTable::EmitSetupFrameCode(CPUSTUBLINKER *pStubLinker)
{
    ////////////////START SETUP FRAME//////////////////////////////
    // Setup frame (partial)

    // get the real address of the methoddesc
    pStubLinker->PPCEmitAddi(kR11, kR11, METHOD_CALL_PRESTUB_SIZE);

    pStubLinker->EmitMethodStubProlog(TPMethodFrame::GetMethodFrameVPtr(), FALSE);

    //  StubStackFrame::params are used for 3 params and 1 INT64 local var

    // Complete the setup of the frame by calling PreCall

    // call PreCall with the frame as an argument
    pStubLinker->PPCEmitAddi(kR3, kR1, offsetof(StubStackFrame, methodframe));  
    pStubLinker->PPCEmitCall(pStubLinker->NewExternalCodeLabel((LPVOID) PreCall));

    ////////////////END SETUP FRAME////////////////////////////////                
    
    // Debugger patch location
    // NOTE: This MUST follow the call to emit the "PreCall" label
    // since after PreCall we know how to help the debugger in 
    // finding the actual destination address of the call.
    // See CVirtualThunkMgr::DoTraceStub.
    pStubLinker->EmitPatchLabel();

    // Call
    pStubLinker->PPCEmitAddi(kR3, kR1, offsetof(StubStackFrame, methodframe)); // new frame as ARG
    pStubLinker->PPCEmitMr(kR4, kR31); // current thread as ARG
    pStubLinker->PPCEmitAddi(kR5, kR1, offsetof(StubStackFrame, argregs) - sizeof(INT64)); // return value as ARG
    pStubLinker->PPCEmitCall(pStubLinker->NewExternalCodeLabel((LPVOID) OnCall));

    // Epilog
    pStubLinker->EmitMethodStubEpilog(kNoTripStubStyle);
}

//+----------------------------------------------------------------------------
// Helper functions for IsThunkByASM and GetMethodDescByASM
//+----------------------------------------------------------------------------
static inline BOOL PPCIsLoadImmNonoptimized(DWORD *pCode, PPCReg r) {
    return ((pCode[0] >> 16) == (UINT)(0x3C00 | (r << 5))) // addis r, 0, (addr>>16)
        && ((pCode[1] >> 16) == (UINT)(0x6000 | (r << 5) | r)); // ori r,r,addr
}

static inline INT PPCCrackLoadImmNonoptimized(DWORD *pCode) {
    return (pCode[0] << 16) + (UINT16)pCode[1];
}

//+----------------------------------------------------------------------------
//
//  Method:     CVirtualThunkMgr::DoTraceStub   public
//
//  Synopsis:   Traces the stub given the starting address
//+----------------------------------------------------------------------------
BOOL CVirtualThunkMgr::DoTraceStub(const BYTE *stubStartAddress, TraceDestination *trace)
{
    BOOL bIsStub = FALSE;

    // Find a thunk whose code address matching the starting address
    LPBYTE pThunk = FindThunk(stubStartAddress);
    if(NULL != pThunk)
    {
        LONG destAddress = 0;
        if(stubStartAddress == pThunk)
        {
            // The first two instructions contain the address
            destAddress = PPCCrackLoadImmNonoptimized((DWORD*)stubStartAddress);
        }

        // We cannot tell where the stub will end up until OnCall is reached.
        // So we tell the debugger to run till OnCall is reached and then 
        // come back and ask us again for the actual destination address of 
        // the call
    
        Stub *stub = Stub::RecoverStub((BYTE *)(size_t)destAddress);
    
        trace->type = TRACE_FRAME_PUSH;
        trace->address = stub->GetEntryPoint() + stub->GetPatchOffset();
        bIsStub = TRUE;
    }

    return bIsStub;
}

//+----------------------------------------------------------------------------
//
//  Method:     CVirtualThunkMgr::IsThunkByASM  public
//
//  Synopsis:   Check assembly to see if this one of our thunks
//+----------------------------------------------------------------------------
BOOL CVirtualThunkMgr::IsThunkByASM(const BYTE *startaddr)
{
    // This is decoding code generated in CTPMethodTable::CreateThunkForVirtualMethod

    DWORD *pCode = (DWORD*)startaddr;

    // Future:: Try using the rangelist. This may be a problem if the code is not at least 20 bytes long
    return (startaddr &&
        PPCIsLoadImmNonoptimized(pCode+0, kR12) &&
        (PPCCrackLoadImmNonoptimized(pCode+0) == 
            (INT)(size_t)CTPMethodTable::GetTPStub()->GetEntryPoint()) &&
        ((pCode[2] >> 16) == 0x3960) && // li r11, value
        (pCode[3] == 0x7D8903A6) && // mtctr r12
        (pCode[4] == 0x4E800420)); // bctr
}

//+----------------------------------------------------------------------------
//
//  Method:     CVirtualThunkMgr::GetMethodDescByASM   public
//
//  Synopsis:   Parses MethodDesc out of assembly code
//+----------------------------------------------------------------------------
MethodDesc *CVirtualThunkMgr::GetMethodDescByASM(const BYTE *startaddr, MethodTable *pMT)
{
    // This is decoding code generated in CTPMethodTable::CreateThunkForVirtualMethod

    DWORD *pCode = (DWORD*)startaddr;

    return pMT->GetClass()->GetMethodDescForSlot((UINT16)pCode[2]);
}


//+----------------------------------------------------------------------------
//
//  Method:     CNonVirtualThunkMgr::TraceManager   public
//
//  Synopsis:   Traces the stub given the current context
//+----------------------------------------------------------------------------
BOOL CNonVirtualThunkMgr::TraceManager(Thread *thread,
                                       TraceDestination *trace,
                                       CONTEXT *pContext,
                                       BYTE **pRetAddr)
{
    BOOL bRet = FALSE;
    
    // We need to find the methodDesc first
    DWORD stubStartAddress = pContext->Iar -
            g_dwNonVirtualThunkReCheckLabelOffset;

    DWORD* remotingLabelAddr = (DWORD *)(size_t)(stubStartAddress +
                                g_dwNonVirtualThunkRemotingLabelOffset);

    MethodDesc *pMD =
        (MethodDesc *)(size_t)(PPCCrackLoadImmNonoptimized(remotingLabelAddr + 2) + METHOD_CALL_PRESTUB_SIZE);

    _ASSERTE(pMD != NULL);

    MetaSig msig(pMD);
    DWORD pThis = msig.HasRetBuffArg() ? pContext->Gpr4 : pContext->Gpr3;

    // Does this.MethodTable == CTPMethodTable::GetMethodTableAddr()?
    if ((pThis != NULL) &&
        (*(DWORD*)(size_t)pThis == (DWORD)(size_t)CTPMethodTable::GetMethodTableAddr()))
    {
        //

        bRet = FALSE;
    }
    else
    {
        // No proxy in the way, so figure out where we're really going
        // to and let the stub manager try to pickup the trace from
        // there.
        
        // Find the address of the loadimm instructions that have the slot
        DWORD* pvSlotAddr = remotingLabelAddr - 5;

        // Extract the immediate value
        DWORD pvSlot = PPCCrackLoadImmNonoptimized(pvSlotAddr);

        // Dereference it to get the address
        BYTE *destAddress = *(BYTE **)(size_t)pvSlot;

        // Ask the stub manager to trace the destination address
        bRet = StubManager::TraceStub(destAddress, trace);
    }

    // While we may have made it this far, further tracing may reveal
    // that the debugger can't continue on. Therefore, since there is
    // no frame currently pushed, we need to tell the debugger where
    // we're returning to just in case it hits such a situation.  We
    // know that the return address is in the link register
    *pRetAddr = (BYTE*)(size_t)(pContext->Lr);
    
    return bRet;
}

//+----------------------------------------------------------------------------
//
//  Method:     CNonVirtualThunkMgr::DoTraceStub   public
//
//  Synopsis:   Traces the stub given the starting address
//+----------------------------------------------------------------------------
BOOL CNonVirtualThunkMgr::DoTraceStub(const BYTE *stubStartAddress,
                                      TraceDestination *trace)
{
    BOOL bRet = FALSE;

    CNonVirtualThunk* pThunk = FindThunk(stubStartAddress);
    
    if(NULL != pThunk)
    {
        // We can either jump to 
        // (1) a slot in the transparent proxy table (UNMANAGED)
        // (2) a slot in the non virtual part of the vtable
        // ... so, we need to return TRACE_MGR_PUSH with the address
        // at which we want to be called back with the thread's context
        // so we can figure out which way we're gonna go.
        if(stubStartAddress == pThunk->GetThunkCode())
        {
            trace->type = TRACE_MGR_PUSH;
            trace->stubManager = this; // Must pass this stub manager!
            trace->address = (BYTE*)(stubStartAddress +
                                     g_dwNonVirtualThunkReCheckLabelOffset);
            bRet = TRUE;
        }
    }

    return bRet;
}

//+----------------------------------------------------------------------------
//
//  Method:     CNonVirtualThunkMgr::IsThunkByASM  public
//
//  Synopsis:   Check assembly to see if this one of our thunks
//+----------------------------------------------------------------------------
BOOL CNonVirtualThunkMgr::IsThunkByASM(const BYTE *startaddr)
{
    DWORD *pCode = (DWORD*)startaddr;

    // This is decoding code generated in CTPMethodTable::GetOrCreateNonVirtualThunkForVirtualMethod

    // FUTURE:: Try using rangelist, this may be a problem if the code is not long enough
    return  (startaddr &&
        ((pCode[0] == 0x2c030000) || (pCode[0] == 0x2c040000)) && // cmpwi regThis, 0
        (pCode[1] == 0x41820018) && // beq pExceptionLabel
        ((pCode[2] == 0x81830000) || (pCode[2] == 0x81840000)) && // lwz r12,regThis
        PPCIsLoadImmNonoptimized(pCode+3, kR0) &&
        (PPCCrackLoadImmNonoptimized(pCode+3) == 
            (INT)(size_t)CTPMethodTable::GetMethodTable()) &&
        (pCode[5] == 0x7c006000) && // cmpw r0,r12
        (pCode[6] == 0x41820018) && // beq pRemotingLabel
        PPCIsLoadImmNonoptimized(pCode+7, kR12) &&
        (pCode[9] == 0x818c0000) && // lwz r12,0(r12)
        (pCode[10] == 0x7d8903a6) && // mtctr r12
        (pCode[11] == 0x4e800420) && // bctr
        PPCIsLoadImmNonoptimized(pCode+12, kR12) &&
        (PPCCrackLoadImmNonoptimized(pCode+12) == 
            (INT)(size_t)g_dwTPStubAddr) &&
        PPCIsLoadImmNonoptimized(pCode+14, kR11) &&
        (pCode[16] == 0x7d8903a6) && // mtctr r12
        (pCode[17] == 0x4e800420)); // bctr
}

//+----------------------------------------------------------------------------
//
//  Method:     CNonVirtualThunkMgr::GetMethodDescByASM   public
//
//  Synopsis:   Parses MethodDesc out of assembly code
//+----------------------------------------------------------------------------
MethodDesc *CNonVirtualThunkMgr::GetMethodDescByASM(const BYTE *startaddr)
{
    DWORD *pCode = (DWORD*)startaddr;

    return (MethodDesc *)(size_t)
        (METHOD_CALL_PRESTUB_SIZE + PPCCrackLoadImmNonoptimized(pCode+14));
}
