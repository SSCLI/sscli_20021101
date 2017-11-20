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
** File:    remotingx86.cpp
**       
** Purpose: Defines various remoting related functions for the x86 architecture
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


// External variables
extern DWORD g_dwNonVirtualThunkRemotingLabelOffset;
extern DWORD g_dwNonVirtualThunkReCheckLabelOffset;

extern "C" size_t g_dwTPStubAddr;
extern "C" size_t g_dwOOContextAddr;

size_t g_dwTPStubAddr;
size_t g_dwOOContextAddr;

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
    THROWSCOMPLUSEXCEPTION();

    // The caller of this stub uses __cdecl instead of __fastcall so the argument needs
    // to be read of the stack
    // Note that the MethodDesc is currently on the top of the stack so the Arg1 is shifted by one slot 
    static const BYTE initThis[] = { 0x8b, 0x4C, 0x24, 0x08 }; // mov ecx, [esp+8]
    psl->EmitBytes(initThis, sizeof(initThis));

    // call CRemotingServices::DispatchInterfaceCall
    // NOTE: We pop 0 bytes of stack even though the size of the arguments is
    // 4 bytes because the MethodDesc argument gets pushed for "free" via the 
    // call instruction placed just before the start of the MethodDesc. 
    // See the class MethodDesc for more details.
    psl->X86EmitCall(psl->NewExternalCodeLabel((LPVOID) CRemotingServices__DispatchInterfaceCall), 0);

    // never returns
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
    BYTE *pCode = startaddr;
    _ASSERTE(NULL != s_pTPStub);

    // 0000   68 67 45 23 01     PUSH dwSlot
    // 0005   E9 ?? ?? ?? ??     JMP  s_pTPStub+1
    *pCode++ = 0x68;
    *((DWORD *) pCode) = dwSlot;
    pCode += sizeof(DWORD);
    *pCode++ = 0xE9;
    // self-relative call, based on the start of the next instruction.
    *((LONG *) pCode) = (LONG)(((size_t) s_pTPStub->GetEntryPoint()) - (size_t) (pCode + sizeof(LONG)));

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
       
    if (!fThrow)
    {

        COMPLUS_TRY
        {           
            // The thunk has not been created yet. Go ahead and create it.    
            LPVOID pvStub = (LPVOID)s_pTPStub->GetEntryPoint();

            // Generate label where a null reference exception will be thrown
            CodeLabel *pJmpAddrLabel = psl->NewCodeLabel();
            // Generate label where remoting code will execute
            CodeLabel *pRemotingLabel = psl->NewCodeLabel();

            // if this == NULL throw NullReferenceException
            // test ecx, ecx
            psl->X86EmitR2ROp(0x85, kECX, kECX);

            // je ExceptionLabel
            psl->X86EmitCondJump(pJmpAddrLabel, X86CondCode::kJE);


            // Emit a label here for the debugger. A breakpoint will
            // be set at the next instruction and the debugger will
            // call CNonVirtualThunkMgr::TraceManager when the
            // breakpoint is hit with the thread's context.
            CodeLabel *pRecheckLabel = psl->NewCodeLabel();
            psl->EmitLabel(pRecheckLabel);
        
            // If this.MethodTable != TPMethodTable then do RemotingCall
            // mov eax, [ecx]
            psl->X86EmitIndexRegLoad(kEAX, kECX, 0);
    
            // cmp eax, CTPMethodTable::s_pThunkTable
            psl->Emit8(0x3D);
            psl->Emit32((DWORD)(size_t)GetMethodTable());
    
            // jne pJmpAddrLabel
            // marshalbyref case
            psl->X86EmitCondJump(pJmpAddrLabel, X86CondCode::kJNE);

            // Transparent proxy case
            EmitCallToStub(psl, pRemotingLabel);

            // Exception handling and non-remoting share the 
            // same codepath
            psl->EmitLabel(pJmpAddrLabel);

            if (pInnerStub == NULL)
            {
                // pop the method desc
                psl->X86EmitPopReg(kEAX);
                // jump to the address
                psl->X86EmitNearJump(psl->NewExternalCodeLabel(pvAddrOfCode));
            }
            else
            {
                // jump to the address
                psl->X86EmitNearJump(psl->NewExternalCodeLabel(pvAddrOfCode));
            }
            
            psl->EmitLabel(pRemotingLabel);
                  
            // the MethodDesc is already on top of the stack.  goto TPStub
            // jmp TPStub
            psl->X86EmitNearJump(psl->NewExternalCodeLabel(pvStub));

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
    LOCKCOUNTINCL("GetOrCreateNonVirtualThunk in remotingx86.cpp");
    EnterCriticalSection(&s_TPMethodTableCrst);

    COMPLUS_TRY
    {
        // Check to make sure that no other thread has 
        // created the thunk
        _ASSERTE(NULL != s_pThunkHashTable);
    
        s_pThunkHashTable->GetValue(pMD, (HashDatum *)&pvThunk);
    
        if((NULL == pvThunk) && !fThrow)
        {
            // The thunk has not been created yet. Go ahead and create it.    
            EEClass* pClass = pMD->GetClass();                
            // Compute the address of the slot         
            LPVOID pvSlot = (LPVOID)pClass->GetMethodSlot(pMD);
            LPVOID pvStub = (LPVOID)s_pTPStub->GetEntryPoint();
    
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
            // test ecx, ecx
            psl->X86EmitR2ROp(0x85, kECX, kECX);
    
            // je ExceptionLabel
            psl->X86EmitCondJump(pExceptionLabel, X86CondCode::kJE);
    
            // Generate label where remoting code will execute
            CodeLabel *pRemotingLabel = psl->NewCodeLabel();
    
            // Emit a label here for the debugger. A breakpoint will
            // be set at the next instruction and the debugger will
            // call CNonVirtualThunkMgr::TraceManager when the
            // breakpoint is hit with the thread's context.
            CodeLabel *pRecheckLabel = psl->NewCodeLabel();
            psl->EmitLabel(pRecheckLabel);
            
            // If this.MethodTable == TPMethodTable then do RemotingCall
            // mov eax, [ecx]
            psl->X86EmitIndexRegLoad(kEAX, kECX, 0);
        
            // cmp eax, CTPMethodTable::s_pThunkTable
            psl->Emit8(0x3D);
            psl->Emit32((DWORD)(size_t)GetMethodTable());
        
            // je RemotingLabel
            psl->X86EmitCondJump(pRemotingLabel, X86CondCode::kJE);
    
            // Exception handling and non-remoting share the 
            // same codepath
            psl->EmitLabel(pExceptionLabel);
    
            // Non-RemotingCode
            // Jump to the vtable slot of the method
            // jmp [slot]
            psl->Emit8(0xff);
            psl->Emit8(0x25);
            psl->Emit32((DWORD)(size_t)pvSlot);            

            // Remoting code. Note: CNonVirtualThunkMgr::TraceManager
            // relies on this label being right after the jmp [slot]
            // instruction above. If you move this label, update
            // CNonVirtualThunkMgr::DoTraceStub.
            psl->EmitLabel(pRemotingLabel);
    
            // Save the MethodDesc and goto TPStub
            // push MethodDesc

            psl->X86EmitPushImm32((DWORD)(size_t)pMD);

            // jmp TPStub
            psl->X86EmitNearJump(psl->NewExternalCodeLabel(pvStub));
    
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
    LOCKCOUNTDECL("GetOrCreateNonVirtualThunk in remotingx86.cpp");
    
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
                        
            CodeLabel *ConvMD = pStubLinker->NewCodeLabel();
            CodeLabel *UseCode = pStubLinker->NewCodeLabel();
            CodeLabel *OOContext = pStubLinker->NewCodeLabel();

            // before we setup a frame check if the method is being executed 
            // in the same context in which the server was created, if true,
            // we do not set up a frame and instead jump directly to the code address.            
            EmitCallToStub(pStubLinker, OOContext);

            // The contexts match. Jump to the real address and start executing...
            EmitJumpToAddressCode(pStubLinker, ConvMD, UseCode);

            // label: OOContext
            pStubLinker->EmitLabel(OOContext);
            
			// CONTEXT MISMATCH CASE, call out to the real proxy to
			// dispatch

            // Setup the frame
            EmitSetupFrameCode(pStubLinker);

            // Finally, create the stub
            s_pTPStub = pStubLinker->Link();

            g_dwTPStubAddr = (size_t)s_pTPStub->GetEntryPoint();

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
//  Method:     CTPMethodTable::EmitCallToStub   private
//
//  Synopsis:   Emits code to call a stub defined on the proxy. 
//              The result of the call dictates whether the call should be executed in the callers 
//              context or not.
//+----------------------------------------------------------------------------
VOID CTPMethodTable::EmitCallToStub(CPUSTUBLINKER* pStubLinker, CodeLabel* pCtxMismatch)
{       

    // Move into eax the stub data and call the stub
    // mov eax, [ecx + TP_OFFSET_STUBDATA]
    pStubLinker->X86EmitIndexRegLoad(kEAX, kECX, TP_OFFSET_STUBDATA);

    //call [ecx + TP_OFFSET_STUB]
    BYTE callStub[] = {0xff, 0x51, (BYTE)TP_OFFSET_STUB};
    pStubLinker->EmitBytes(callStub, sizeof(callStub));

    // test eax,eax
    pStubLinker->Emit16(0xc085);
    // jnz CtxMismatch
    pStubLinker->X86EmitCondJump(pCtxMismatch, X86CondCode::kJNZ);
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

    // mov eax, [esp]
    BYTE loadSlotOrMD[] = {0x8B, 0x44, 0x24, 0x00};
    pStubLinker->EmitBytes(loadSlotOrMD, sizeof(loadSlotOrMD));

    // test eax, 0xffff0000
    BYTE testForSlot[] = { 0xA9, 0x00, 0x00, 0xFF, 0xFF };
    pStubLinker->EmitBytes(testForSlot, sizeof(testForSlot));

    // jnz ConvMD
    pStubLinker->X86EmitCondJump(ConvMD, X86CondCode::kJNZ);
    
    // if (!([esp] & 0xffff0000))
    // {
    
        // ** code addr from slot case **
    
        // mov eax, [ecx + TPMethodTable::GetOffsetOfMT()]
        pStubLinker->X86EmitIndexRegLoad(kEAX, kECX, TP_OFFSET_MT);

        // push ebx
        pStubLinker->X86EmitPushReg(kEBX);

        // mov ebx, [esp + 4]
        BYTE loadSlot[] = {0x8B, 0x5C, 0x24, 0x04};
        pStubLinker->EmitBytes(loadSlot, sizeof(loadSlot));

        // mov eax,[eax + ebx*4 + MethodTable::GetOffsetOfVtable()]
        BYTE getCodePtr[]  = {0x8B, 0x84, 0x98, 0x00, 0x00, 0x00, 0x00};
        *((DWORD *)(getCodePtr+3)) = MethodTable::GetOffsetOfVtable();
        pStubLinker->EmitBytes(getCodePtr, sizeof(getCodePtr));

        // pop ebx
        pStubLinker->X86EmitPopReg(kEBX);

        // lea esp, [esp+4]
        BYTE popNULL[] = { 0x8D, 0x64, 0x24, 0x04};
        pStubLinker->EmitBytes(popNULL, sizeof(popNULL));

        // jmp eax
        BYTE jumpToRegister[] = {0xff, 0xe0};
        pStubLinker->EmitBytes(jumpToRegister, sizeof(jumpToRegister));
    
    // }
    // else
    // {
        // ** code addr from MethodDesc case **

        pStubLinker->EmitLabel(ConvMD);                
        
        // sub eax, METHOD_CALL_PRESTUB_SIZE
        pStubLinker->X86EmitSubReg(kEAX, METHOD_CALL_PRESTUB_SIZE);
                
        // lea esp, [esp+4]
        pStubLinker->EmitBytes(popNULL, sizeof(popNULL));

        // jmp eax
        pStubLinker->EmitBytes(jumpToRegister, sizeof(jumpToRegister));

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
        pStubLinker->EmitMethodStubProlog(TPMethodFrame::GetMethodFrameVPtr());

        // Complete the setup of the frame by calling PreCall
        
        // push esi (push new frame as ARG)
        pStubLinker->X86EmitPushReg(kESI); 

        // pop 4 bytes or args on return from call
        pStubLinker->X86EmitCall(pStubLinker->NewExternalCodeLabel((LPVOID) PreCall), 4);

        ////////////////END SETUP FRAME////////////////////////////////                
        
        // Debugger patch location
        // NOTE: This MUST follow the call to emit the "PreCall" label
        // since after PreCall we know how to help the debugger in 
        // finding the actual destination address of the call.
        // See CVirtualThunkMgr::DoTraceStub.
        pStubLinker->EmitPatchLabel();

        // Call
        pStubLinker->X86EmitSubEsp(sizeof(INT64));
        pStubLinker->Emit8(0x54);          // push esp (push return value as ARG)
        pStubLinker->X86EmitPushReg(kEBX); // push ebx (push current thread as ARG)
        pStubLinker->X86EmitPushReg(kESI); // push esi (push new frame as ARG)
#ifdef _DEBUG
        // push IMM32
        pStubLinker->Emit8(0x68);
        pStubLinker->EmitPtr((LPVOID) OnCall);
        // in CE pop 12 bytes or args on return from call
            pStubLinker->X86EmitCall(pStubLinker->NewExternalCodeLabel((LPVOID) WrapCall), 12);
#else // !_DEBUG
        // in CE pop 12 bytes or args on return from call
        pStubLinker->X86EmitCall(pStubLinker->NewExternalCodeLabel((LPVOID) OnCall), 12);
#endif // _DEBUG

        // Tear down frame
        pStubLinker->X86EmitAddEsp(sizeof(INT64));
        pStubLinker->EmitMethodStubEpilog(-1, kNoTripStubStyle);
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
        LPBYTE pbAddr = NULL;
        LONG destAddress = 0;
        if(stubStartAddress == pThunk)
        {

            // Extract the long which gives the self relative address
            // of the destination
            pbAddr = pThunk + sizeof(BYTE) + sizeof(DWORD) + sizeof(BYTE);
            destAddress = *(LONG *)pbAddr;

            // Calculate the absolute address by adding the offset of the next 
            // instruction after the call instruction
            destAddress += (LONG)(size_t)(pbAddr + sizeof(LONG));

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

    // Future:: Try using the rangelist. This may be a problem if the code is not at least 6 bytes long
    const BYTE *bCode = startaddr + 6;
    return (startaddr &&
            (startaddr[0] == 0x68) &&
            (startaddr[5] == 0xe9) &&
            (*((LONG *) bCode) == (LONG)((LONG_PTR)CTPMethodTable::GetTPStub()->GetEntryPoint()) - (LONG_PTR)(bCode + sizeof(LONG))));
}

//+----------------------------------------------------------------------------
//
//  Method:     CVirtualThunkMgr::GetMethodDescByASM   public
//
//  Synopsis:   Parses MethodDesc out of assembly code
//+----------------------------------------------------------------------------
MethodDesc *CVirtualThunkMgr::GetMethodDescByASM(const BYTE *startaddr, MethodTable *pMT)
{
    return pMT->GetClass()->GetMethodDescForSlot(*((DWORD *) (startaddr + 1)));
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
    
    // Does this.MethodTable ([ecx]) == CTPMethodTable::GetMethodTableAddr()?
    DWORD pThis = pContext->Ecx;

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
        DWORD stubStartAddress = pContext->Eip -
            g_dwNonVirtualThunkReCheckLabelOffset;
        
        // Extract the long which gives the address of the destination
        BYTE* pbAddr = (BYTE *)(size_t)(stubStartAddress +
                                g_dwNonVirtualThunkRemotingLabelOffset -
                                sizeof(DWORD));

        // Since we do an indirect jump we have to dereference it twice
        LONG destAddress = **(LONG **)pbAddr;

        // Ask the stub manager to trace the destination address
        bRet = StubManager::TraceStub((BYTE *)(size_t)destAddress, trace);
    }

    // While we may have made it this far, further tracing may reveal
    // that the debugger can't continue on. Therefore, since there is
    // no frame currently pushed, we need to tell the debugger where
    // we're returning to just in case it hits such a situtation.  We
    // know that the return address is on the top of the thread's
    // stack.
    *pRetAddr = *((BYTE**)(size_t)(pContext->Esp));
    
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
    // FUTURE:: Try using rangelist, this may be a problem if the code is not long enough
    return  (startaddr &&
             startaddr[0] == 0x85 && 
             startaddr[1] == 0xc9 && 
             startaddr[2] == 0x74 && 
             (*((DWORD *)(startaddr + 7)) == (DWORD)(size_t)CTPMethodTable::GetMethodTable()));
}

//+----------------------------------------------------------------------------
//
//  Method:     CNonVirtualThunkMgr::GetMethodDescByASM   public
//
//  Synopsis:   Parses MethodDesc out of assembly code
//+----------------------------------------------------------------------------
MethodDesc *CNonVirtualThunkMgr::GetMethodDescByASM(const BYTE *startaddr)
{
    return *((MethodDesc **) (startaddr + 20));
}


