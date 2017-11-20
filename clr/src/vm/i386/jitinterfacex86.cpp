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
// File: JITinterfaceX86.CPP
//
// ===========================================================================

// This contains JITinterface routines that are tailored for
// X86 platforms. Non-X86 versions of these can be found in
// JITinterfaceGen.cpp

#include "common.h"
#include "jitinterface.h"
#include "eeconfig.h"
#include "excep.h"
#include "comstring.h"
#include "comdelegate.h"
#include "remoting.h" // create context bound and remote class instances
#include "field.h"
#include "tls.h"
#include "ecall.h"
#include "asmconstants.h"

// Compile-time checks that the offsets declared in clr/src/vm/i386/asmconstants.h
// match the actual structure offsets.  If any of these lines generates a
// compile error:
//   An assertion failure results in error C2118: negative subscript.
//    or
//   size of array `__C_ASSERTxx__' is negative
// Then the offsets in asmconstants.h are out of sync with the C/C++ struct
C_ASSERT(offsetof(CONTEXT,Edi) == CONTEXT_Edi);
C_ASSERT(offsetof(CONTEXT,Esi) == CONTEXT_Esi);
C_ASSERT(offsetof(CONTEXT,Ebx) == CONTEXT_Ebx);
C_ASSERT(offsetof(CONTEXT,Edx) == CONTEXT_Edx);
C_ASSERT(offsetof(CONTEXT,Eax) == CONTEXT_Eax);
C_ASSERT(offsetof(CONTEXT,Ebp) == CONTEXT_Ebp);
C_ASSERT(offsetof(CONTEXT,Eip) == CONTEXT_Eip);
C_ASSERT(offsetof(CONTEXT,Esp) == CONTEXT_Esp);

C_ASSERT(offsetof(EHContext,Eax) == EHContext_Eax);
C_ASSERT(offsetof(EHContext,Ebx) == EHContext_Ebx);
C_ASSERT(offsetof(EHContext,Ecx) == EHContext_Ecx);
C_ASSERT(offsetof(EHContext,Edx) == EHContext_Edx);
C_ASSERT(offsetof(EHContext,Esi) == EHContext_Esi);
C_ASSERT(offsetof(EHContext,Edi) == EHContext_Edi);
C_ASSERT(offsetof(EHContext,Esp) == EHContext_Esp);
C_ASSERT(offsetof(EHContext,Eip) == EHContext_Eip);

C_ASSERT(sizeof(VMHELPDEF) == VMHELPDEF_Size);
C_ASSERT(offsetof(VMHELPDEF,pfnHelper) == VMHELPDEF_pfnHelper);

#ifdef _DEBUG
class CheckAsmOffsets {
CPP_ASSERT(1, offsetof(MachState, _pEdi) == MachState__pEdi);
CPP_ASSERT(2, offsetof(MachState, _edi) == MachState__edi);
CPP_ASSERT(3, offsetof(MachState, _pEsi) == MachState__pEsi);
CPP_ASSERT(4, offsetof(MachState, _esi) == MachState__esi);
CPP_ASSERT(5, offsetof(MachState, _pEbx) == MachState__pEbx);
CPP_ASSERT(6, offsetof(MachState, _ebx) == MachState__ebx);
CPP_ASSERT(7, offsetof(MachState, _esp) == MachState__esp);
CPP_ASSERT(8, offsetof(MachState, _pEbp) == MachState__pEbp);
CPP_ASSERT(9, offsetof(MachState, _ebp) == MachState__ebp);
CPP_ASSERT(10,offsetof(MachState, _pRetAddr) == MachState__pRetAddr);
CPP_ASSERT(11,offsetof(LazyMachState, captureEbp) == LazyMachState_captureEbp);
CPP_ASSERT(12,offsetof(LazyMachState, captureEsp) == LazyMachState_captureEsp);
CPP_ASSERT(13,offsetof(LazyMachState, captureEip) == LazyMachState_captureEip);
};
#endif //_DEBUG



// To test with MON_DEBUG off, comment out the following line. DO NOT simply define
// to be 0 as the checks are for #ifdef not #if 0. 
#ifdef _DEBUG
#define MON_DEBUG 1
#endif

// Macros to improve readability of code 
#define ARGUMENT3_OFFSET 12
#define IF_N_CDECL(n)  

class generation;
extern "C" generation generation_table[];


#ifdef _DEBUG
extern "C" void __stdcall WriteBarrierAssert(BYTE* ptr) 
{
    _ASSERTE((g_lowest_address <= ptr && ptr < g_highest_address) ||
         ((size_t)ptr < MAX_UNCHECKED_OFFSET_FOR_NULL_OBJECT));
}

BOOL SafeObjIsInstanceOf(Object *pElement, TypeHandle toTypeHnd) {
    BEGINFORBIDGC();
    BOOL ret = ObjIsInstanceOf(pElement, toTypeHnd);
    ENDFORBIDGC();
    return(ret);
}
#endif //_DEBUG


#ifdef MAXALLOC
extern "C" BOOL __stdcall CheckAllocRequest(size_t n)
{
    return GetGCAllocManager()->CheckRequest(n);
}

extern "C" void __stdcall UndoAllocRequest()
{
    GetGCAllocManager()->UndoRequest();
}
#endif // MAXALLOC

#if CHECK_APP_DOMAIN_LEAKS
void * __stdcall SetObjectAppDomain(Object *pObject)
{
    pObject->SetAppDomain();
    return pObject;
}
#endif


// These functions are just here so we can link - for x86 high perf helpers are generated at startup
// so we should never get here
Object* __fastcall JIT_TrialAllocSFastSP(MethodTable *mt)   // JITinterfaceX86.cpp/JITinterfaceGen.cpp
{
    _ASSERTE(!"JIT_TrialAllocSFastSP");
    return  NULL;
}

// These functions are just here so we can link - for x86 high perf helpers are generated at startup
// so we should never get here
Object* __fastcall JIT_TrialAllocSFastMP(MethodTable *mt)   // JITinterfaceX86.cpp/JITinterfaceGen.cpp
{
    _ASSERTE(!"JIT_TrialAllocSFastMP");
    return  NULL;
}

HCIMPL1(int, JIT_Dbl2IntOvf, double val)
{
#if defined(_MSC_VER)
    __asm fnclex
#else
    __asm__ __volatile("fnclex");
#endif
    __int64 ret = JIT_Dbl2Lng(val);

    if (ret != (__int32) ret)
        goto THROW;

    return (__int32) ret;

THROW:
    FCThrow(kOverflowException);
}
HCIMPLEND

HCIMPL1(INT64, JIT_Dbl2LngOvf, double val)
{
#if defined(_MSC_VER)
    __asm fnclex
#else
    __asm__ __volatile("fnclex");
#endif
    __int64 ret = JIT_Dbl2Lng(val);
#if defined(_MSC_VER)
    __asm {
        fnstsw  ax
        test    ax,01h
        jnz     THROW
    }
#else
    short temp;
    __asm__ __volatile("fnstsw  %ax");
    __asm__ __volatile("movw %%ax,%0" : "=rm" (temp));
    if (temp & 1)
	goto THROW;
#endif
    return ret;

THROW:
    FCThrow(kOverflowException);
    return(0);
}
HCIMPLEND

/*********************************************************************/
// This is called by the JIT after every instruction in fully interuptable
// code to make certain our GC tracking is OK
HCIMPL0(VOID, JIT_StressGC_NOP) {}
HCIMPLEND


HCIMPL0(VOID, JIT_StressGC)
{
#ifdef _DEBUG
        HELPER_METHOD_FRAME_BEGIN_0();    // Set up a frame
        g_pGCHeap->GarbageCollect();

        HELPER_METHOD_FRAME_END();

#endif // _DEBUG
}
HCIMPLEND


/*********************************************************************/
// Caller has to be an EBP frame, callee-saved registers (EDI, ESI, EBX) have
// to be saved on the stack just below the stack arguments,
// enregistered arguements are in correct registers, remaining args pushed
// on stack, followed by target address and count of stack arguments.
// So the stack will look like TailCallArgs
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4200 )  // zero-sized array
#endif
struct TailCallArgs
{
    DWORD       dwRetAddr;
    DWORD       dwTargetAddr;

    int         offsetCalleeSavedRegs   : 28;
    unsigned    ebpRelCalleeSavedRegs   : 1;
    unsigned    maskCalleeSavedRegs     : 3; // EBX, ESDI, EDI

    DWORD       nNewStackArgs;
    DWORD       nOldStackArgs;
    DWORD       newStackArgs[0];
    DWORD *     GetCalleeSavedRegs(DWORD * Ebp)
    {
        if (ebpRelCalleeSavedRegs)
            return (DWORD*)&Ebp[-offsetCalleeSavedRegs];
        else
            return (DWORD*)&newStackArgs[nNewStackArgs + offsetCalleeSavedRegs];
    }
};
#if defined(_MSC_VER)
#pragma warning(pop)
#endif


extern "C" DWORD * __stdcall JIT_TailCallHelper(MachState * machState,
                                                TailCallArgs * args,
                                                ArgumentRegisters * argRegs)
{
    Thread * pThread = GetThread();

    bool shouldTrip = pThread->CatchAtSafePoint() != 0;

#ifdef _DEBUG
    // Force a GC if the stress level is high enough. Doing it on every tailcall
    // will slow down things too much. So do only periodically
    static DWORD count = 0;
    count++;
    if ((count % 10)==0 && (g_pConfig->GetGCStressLevel() & EEConfig::GCSTRESS_TRANSITION))
        shouldTrip = true;
#endif // _DEBUG

    if (shouldTrip)
    {
        /* We will rendezvous with the EE. Set up frame to protect the arguments */

        MethodDesc * pMD = Entry2MethodDesc((BYTE *)(size_t)args->dwTargetAddr, NULL);

        // The return address is separated from the stack arguments by the
        // extra arguments passed to JIT_TailCall(). Put them together
        // while creating the helper frame. When done, we will undo this.

        DWORD oldArgs = args->nOldStackArgs;        // temp
        _ASSERTE(offsetof(TailCallArgs, nOldStackArgs) + sizeof(void*) ==
                 offsetof(TailCallArgs,newStackArgs));
        args->nOldStackArgs = args->dwRetAddr;      // move dwRetAddr near newStackArgs[]
        _ASSERTE(machState->pRetAddr() == (void**)(size_t)0xDDDDDDDD);
        machState->pRetAddr()  = (void **)&args->nOldStackArgs;

        HelperMethodFrame helperFrame(machState, pMD, argRegs);

#ifdef STRESS_HEAP
        if ((g_pConfig->GetGCStressLevel() & EEConfig::GCSTRESS_TRANSITION)
#ifdef _DEBUG
            && !g_pConfig->FastGCStressLevel()
#endif
            )
        {
            // GC stress
            g_pGCHeap->StressHeap();
        }
        else
#endif // STRESS_HEAP
        {
            // rendezvous with the EE
#ifdef _DEBUG
            BOOL GCOnTransition = FALSE;
            if (g_pConfig->FastGCStressLevel()) {
                GCOnTransition = GC_ON_TRANSITIONS (FALSE);
            }
#endif
            CommonTripThread();
#ifdef _DEBUG
            if (g_pConfig->FastGCStressLevel()) {
                GC_ON_TRANSITIONS (GCOnTransition);
            }
#endif
        }

        // Pop the frame

        helperFrame.Pop();

        // Undo move of dwRetAddr from close to newStackArgs[]

        args->dwRetAddr = args->nOldStackArgs;
        args->nOldStackArgs = oldArgs;
#ifdef _DEBUG
        machState->pRetAddr() = (void **)(size_t)0xDDDDDDDD;
#endif // _DEBUG

    }

    /* Now the return-address is unhijacked. More importantly, the EE cannot
       have the address of the return-address. So we can move it around. */

    // Make a copy of the callee saved registers and return address
    // as they may get whacked during sliding of the argument.
    DWORD *  Ebp = (DWORD*)*(machState->pEbp());
    DWORD * calleeSavedRegsBase = args->GetCalleeSavedRegs(Ebp);

#define UPDATE_REG(reg, mask) \
    if (args->maskCalleeSavedRegs & (mask)) { \
        *(DWORD *)(machState->p##reg()) = *calleeSavedRegsBase++; \
    }

    UPDATE_REG(Ebx, 0x4);
    UPDATE_REG(Esi, 0x2);
    UPDATE_REG(Edi, 0x1);
    *(DWORD *)(machState->pEbp()) = Ebp[0];

    DWORD retAddr               = Ebp[1];

    // Slide the arguments. The old and the new location may be overlapping,
    // so use memmove() instead of memcpy().

    // We don't want to slide the original function frame because the callee of
    // that function will clean it up
    _ASSERTE( args->nOldStackArgs >= args->nNewStackArgs );
    DWORD * argsBase = Ebp + 2;
    memmove(argsBase, args->newStackArgs, args->nNewStackArgs*sizeof(void*));

    // Write the original return address just below the arguments

    argsBase[-1] = retAddr;

    DWORD * newESP      = &argsBase[-1]; // this will be the esp when we jump to targetAddr

    // We will set esp to newESP_m1 before doing a "ret" to keep the call-ret count balanced
    DWORD * newESP_m1   = newESP - 1;
    *newESP_m1          = args->dwTargetAddr; // this value will be popped by the / "ret"

    return newESP_m1;
}


    // emit code that adds MIN_OBJECT_SIZE to reg if reg is unaligned thus making it aligned
void JIT_TrialAlloc::EmitAlignmentRoundup(CPUSTUBLINKER *psl, X86Reg testAlignReg, X86Reg adjReg, Flags flags)
{   
    _ASSERTE((MIN_OBJECT_SIZE & 7) == 4);   // want to change alignment

    CodeLabel *AlreadyAligned = psl->NewCodeLabel();

    // test reg, 7
    psl->Emit16(0xC0F7 | (testAlignReg << 8));
    psl->Emit32(0x7);

    // jz alreadyAligned
    if (flags & ALIGN8OBJ)
    {
        psl->X86EmitCondJump(AlreadyAligned, X86CondCode::kJNZ);
    }
    else
    {
        psl->X86EmitCondJump(AlreadyAligned, X86CondCode::kJZ);
    }

    psl->X86EmitAddReg(adjReg, MIN_OBJECT_SIZE);        
    // AlreadyAligned:
    psl->EmitLabel(AlreadyAligned);
}

    // if 'reg' is unaligned, then set the dummy object at EAX and increment EAX past 
    // the dummy object
void JIT_TrialAlloc::EmitDummyObject(CPUSTUBLINKER *psl, X86Reg alignTestReg, Flags flags)
{
    CodeLabel *AlreadyAligned = psl->NewCodeLabel();

    // test reg, 7
    psl->Emit16(0xC0F7 | (alignTestReg << 8));
    psl->Emit32(0x7);

    // jz alreadyAligned
    if (flags & ALIGN8OBJ)
    {
        psl->X86EmitCondJump(AlreadyAligned, X86CondCode::kJNZ);
    }
    else
    {
        psl->X86EmitCondJump(AlreadyAligned, X86CondCode::kJZ);
    }

    // Make the fake object
    // mov EDX, [g_pObjectClass]
    psl->Emit16(0x158B);
    psl->Emit32((int)(size_t)&g_pObjectClass);

    // mov [EAX], EDX
    psl->X86EmitOffsetModRM(0x89, kEDX, kEAX, 0);

#if CHECK_APP_DOMAIN_LEAKS 
    EmitSetAppDomain(psl);
#endif

    // add EAX, MIN_OBJECT_SIZE
    psl->X86EmitAddReg(kEAX, MIN_OBJECT_SIZE);
    
    // AlreadyAligned:
    psl->EmitLabel(AlreadyAligned);
}

void JIT_TrialAlloc::EmitCore(CPUSTUBLINKER *psl, CodeLabel *noLock, CodeLabel *noAlloc, Flags flags)
{

    if (flags & MP_ALLOCATOR)
    {
        // Upon entry here, ecx contains the method we are to try allocate memory for
        // Upon exit, eax contains the allocated memory, edx is trashed, and ecx undisturbed

#ifdef MAXALLOC
        if (flags & SIZE_IN_EAX)
        {
            // save size for later
            psl->X86EmitPushReg(kEAX);
        }
        else
        {
            // load size from method table
            psl->X86EmitIndexRegLoad(kEAX, kECX, offsetof(MethodTable, m_BaseSize));
        }

        // save regs
        psl->X86EmitPushRegs((1<<kECX));

        // CheckAllocRequest(size);
        psl->X86EmitPushReg(kEAX);
        psl->X86EmitCall(psl->NewExternalCodeLabel((LPVOID) CheckAllocRequest), 0);

        // test eax, eax
        psl->Emit16(0xc085);

        // restore regs
        psl->X86EmitPopRegs((1<<kECX));

        CodeLabel *AllocRequestOK = psl->NewCodeLabel();

        if (flags & SIZE_IN_EAX)
            psl->X86EmitPopReg(kEAX);

        // jnz             AllocRequestOK
        psl->X86EmitCondJump(AllocRequestOK, X86CondCode::kJNZ);

        if (flags & SIZE_IN_EAX)
            psl->X86EmitZeroOutReg(kEAX);

        // ret
        psl->X86EmitReturn(0);

        // AllocRequestOK:
        psl->EmitLabel(AllocRequestOK);
#endif // MAXALLOC

        if (flags & (ALIGN8 | SIZE_IN_EAX | ALIGN8OBJ)) 
        {
            if (flags & ALIGN8OBJ)
            {
                // mov             eax, [ecx]MethodTable.m_BaseSize
                psl->X86EmitIndexRegLoad(kEAX, kECX, offsetof(MethodTable, m_BaseSize));
            }

            psl->X86EmitPushReg(kEBX);  // we need a spare register
        }
        else
        {
            // mov             eax, [ecx]MethodTable.m_BaseSize
            psl->X86EmitIndexRegLoad(kEAX, kECX, offsetof(MethodTable, m_BaseSize));
        }

        assert( ((flags & ALIGN8)==0     ||  // EAX loaded by else statement
                 (flags & SIZE_IN_EAX)   ||  // EAX already comes filled out
                 (flags & ALIGN8OBJ)     )   // EAX loaded in the if (flags & ALIGN8OBJ) statement
                 && "EAX should contain size for allocation and it doesnt!!!");

        // Fetch current thread into EDX, preserving EAX and ECX
        psl->X86EmitTLSFetch(GetThreadTLSIndex(), kEDX, (1<<kEAX)|(1<<kECX));

        // Try the allocation.


        if (flags & (ALIGN8 | SIZE_IN_EAX | ALIGN8OBJ))
        {
            // MOV EBX, [edx]Thread.m_alloc_context.alloc_ptr
            psl->X86EmitOffsetModRM(0x8B, kEBX, kEDX, offsetof(Thread, m_alloc_context) + offsetof(alloc_context, alloc_ptr));
            // add EAX, EBX
            psl->Emit16(0xC303);
            if (flags & ALIGN8)
                EmitAlignmentRoundup(psl, kEBX, kEAX, flags);      // bump EAX up size by 12 if EBX unaligned (so that we are aligned)
        }
        else
        {
            // add             eax, [edx]Thread.m_alloc_context.alloc_ptr
            psl->X86EmitOffsetModRM(0x03, kEAX, kEDX, offsetof(Thread, m_alloc_context) + offsetof(alloc_context, alloc_ptr));
        }

        // cmp             eax, [edx]Thread.m_alloc_context.alloc_limit
        psl->X86EmitOffsetModRM(0x3b, kEAX, kEDX, offsetof(Thread, m_alloc_context) + offsetof(alloc_context, alloc_limit));

        // ja              noAlloc
        psl->X86EmitCondJump(noAlloc, X86CondCode::kJA);

        // Fill in the allocation and get out.

        // mov             [edx]Thread.m_alloc_context.alloc_ptr, eax
        psl->X86EmitIndexRegStore(kEDX, offsetof(Thread, m_alloc_context) + offsetof(alloc_context, alloc_ptr), kEAX);

        if (flags & (ALIGN8 | SIZE_IN_EAX | ALIGN8OBJ))
        {
            // mov EAX, EBX
            psl->Emit16(0xC38B);
            // pop EBX
            psl->X86EmitPopReg(kEBX);

            if (flags & ALIGN8)
                EmitDummyObject(psl, kEAX, flags);
        }
        else
        {
            // sub             eax, [ecx]MethodTable.m_BaseSize
            psl->X86EmitOffsetModRM(0x2b, kEAX, kECX, offsetof(MethodTable, m_BaseSize));
        }

        // mov             dword ptr [eax], ecx
        psl->X86EmitIndexRegStore(kEAX, 0, kECX);
    }
    else
    {
        // Take the GC lock (there is no lock prefix required - we will use JIT_TrialAllocSFastMP on an MP System).
        // inc             dword ptr [m_GCLock]
        psl->Emit16(0x05ff);
        psl->Emit32((int)(size_t)&m_GCLock);

        // jnz             NoLock
        psl->X86EmitCondJump(noLock, X86CondCode::kJNZ);

        if (flags & SIZE_IN_EAX)
        {
            // mov edx, eax
            psl->Emit16(0xd08b);
        }
        else
        {
            // mov             edx, [ecx]MethodTable.m_BaseSize
            psl->X86EmitIndexRegLoad(kEDX, kECX, offsetof(MethodTable, m_BaseSize));
        }

#ifdef MAXALLOC
        // save regs
        psl->X86EmitPushRegs((1<<kEDX)|(1<<kECX));

        // CheckAllocRequest(size);
        psl->X86EmitPushReg(kEDX);
        psl->X86EmitCall(psl->NewExternalCodeLabel((LPVOID) CheckAllocRequest), 0);

        // test eax, eax
        psl->Emit16(0xc085);

        // restore regs
        psl->X86EmitPopRegs((1<<kEDX)|(1<<kECX));

        CodeLabel *AllocRequestOK = psl->NewCodeLabel();

        // jnz             AllocRequestOK
        psl->X86EmitCondJump(AllocRequestOK, X86CondCode::kJNZ);

        // ret
        psl->X86EmitReturn(0);

        // AllocRequestOK:
        psl->EmitLabel(AllocRequestOK);
#endif // MAXALLOC

        // mov             eax, dword ptr [generation_table]
        psl->Emit8(0xA1);
        psl->Emit32((int)(size_t)&generation_table);

        // Try the allocation.
        // add             edx, eax
        psl->Emit16(0xd003);

        if (flags & (ALIGN8 | ALIGN8OBJ))
            EmitAlignmentRoundup(psl, kEAX, kEDX, flags);      // bump up EDX size by 12 if EAX unaligned (so that we are aligned)

        // cmp             edx, dword ptr [generation_table+4]
        psl->Emit16(0x153b);
        psl->Emit32((int)(size_t)&generation_table + 4);

        // ja              noAlloc
        psl->X86EmitCondJump(noAlloc, X86CondCode::kJA);

        // Fill in the allocation and get out.
        // mov             dword ptr [generation_table], edx
        psl->Emit16(0x1589);
        psl->Emit32((int)(size_t)&generation_table);

        if (flags & (ALIGN8 | ALIGN8OBJ))
            EmitDummyObject(psl, kEAX, flags);

        // mov             dword ptr [eax], ecx
        psl->X86EmitIndexRegStore(kEAX, 0, kECX);

        // mov             dword ptr [m_GCLock], 0FFFFFFFFh
        psl->Emit16(0x05C7);
        psl->Emit32((int)(size_t)&m_GCLock);
        psl->Emit32(0xFFFFFFFF);
    }


#ifdef  INCREMENTAL_MEMCLR
    _ASSERTE(!"NYI");
#endif // INCREMENTAL_MEMCLR
}

#if CHECK_APP_DOMAIN_LEAKS
void JIT_TrialAlloc::EmitSetAppDomain(CPUSTUBLINKER *psl)
{
    if (!g_pConfig->AppDomainLeaks())
        return;

    // At both entry & exit, eax contains the allocated object.
    // ecx is preserved, edx is not.

    //
    // Add in a call to SetAppDomain.  (Note that this
    // probably would have been easier to implement by just not using
    // the generated helpers in a checked build, but we'd lose code
    // coverage that way.)
    //

    // Save ECX over function call
    psl->X86EmitPushReg(kECX);

    // Push object as arg
    psl->X86EmitPushReg(kEAX);

    // SetObjectAppDomain pops its arg & returns object in EAX
    psl->X86EmitCall(psl->NewExternalCodeLabel((LPVOID) SetObjectAppDomain), 4);
    
    psl->X86EmitPopReg(kECX);
}

#endif


void JIT_TrialAlloc::EmitNoAllocCode(CPUSTUBLINKER *psl, Flags flags)
{
#ifdef MAXALLOC
    psl->X86EmitPushRegs((1<<kEAX)|(1<<kEDX)|(1<<kECX));
    // call            UndoAllocRequest
    psl->X86EmitCall(psl->NewExternalCodeLabel((LPVOID) UndoAllocRequest), 0);
    psl->X86EmitPopRegs((1<<kEAX)|(1<<kEDX)|(1<<kECX));
#endif // MAXALLOC
    if (flags & MP_ALLOCATOR)
    {
        if (flags & (ALIGN8|SIZE_IN_EAX))
            psl->X86EmitPopReg(kEBX);
    }
    else
    {
        // mov             dword ptr [m_GCLock], 0FFFFFFFFh
        psl->Emit16(0x05c7);
        psl->Emit32((int)(size_t)&m_GCLock);
        psl->Emit32(0xFFFFFFFF);
    }
}

void *JIT_TrialAlloc::GenAllocSFast(Flags flags)
{
    CPUSTUBLINKER sl;

    CodeLabel *noLock  = sl.NewCodeLabel();
    CodeLabel *noAlloc = sl.NewCodeLabel();

    // The caller of this stub uses __cdecl instead of __fastcall so the argument needs
    // to be read of the stack
    static const BYTE arbCode1[] = { 0x8b, 0x4C, 0x24, 0x04 }; // mov ecx, [esp+4]
    sl.EmitBytes(arbCode1, sizeof(arbCode1));

    // Emit the main body of the trial allocator, be it SP or MP
    EmitCore(&sl, noLock, noAlloc, flags);

#if CHECK_APP_DOMAIN_LEAKS
    EmitSetAppDomain(&sl);
#endif

    // Here we are at the end of the success case - just emit a ret
    sl.X86EmitReturn(0);

    // Come here in case of no space
    sl.EmitLabel(noAlloc);

    // Release the lock in the uniprocessor case
    EmitNoAllocCode(&sl, flags);

    // Come here in case of failure to get the lock
    sl.EmitLabel(noLock);

    // Jump to the framed helper
    sl.Emit16(0x25ff);
    sl.Emit32((int)(size_t)&hlpFuncTable[CORINFO_HELP_NEWFAST].pfnHelper);

    Stub *pStub = sl.Link(SystemDomain::System()->GetHighFrequencyHeap());

    return (void *)pStub->GetEntryPoint();
}


void *JIT_TrialAlloc::GenBox(Flags flags)
{
    CPUSTUBLINKER sl;

    CodeLabel *noLock  = sl.NewCodeLabel();
    CodeLabel *noAlloc = sl.NewCodeLabel();

    // Initialize the argument registers with arguments from the stack 
    static const BYTE arbCode1[] = { 0x8b, 0x4C, 0x24, 0x04,   // mov ecx, [esp+4]
                     0x8b, 0x54, 0x24, 0x08 }; // mov edx, [esp+8]
    sl.EmitBytes(arbCode1, sizeof(arbCode1));

    // Save address of value to be boxed
    sl.X86EmitPushReg(kEBX);
    sl.Emit16(0xda8b);

    // Check whether the class has not been initialized
    // test [ecx]MethodTable.m_wFlags,MethodTable::enum_flag_Unrestored
    sl.X86EmitOffsetModRM(0xf7, (X86Reg)0x0, kECX, offsetof(MethodTable, m_wFlags));
    sl.Emit32(MethodTable::enum_flag_Unrestored);

    // jne              noAlloc
    sl.X86EmitCondJump(noAlloc, X86CondCode::kJNE);

    // Emit the main body of the trial allocator
    EmitCore(&sl, noLock, noAlloc, flags);

#if CHECK_APP_DOMAIN_LEAKS
    EmitSetAppDomain(&sl);
#endif

    // Here we are at the end of the success case

    // Check whether the object contains pointers
    // test [ecx]MethodTable.m_wFlags,MethodTable::enum_flag_ContainsPointers
    sl.X86EmitOffsetModRM(0xf7, (X86Reg)0x0, kECX, offsetof(MethodTable, m_wFlags));
    sl.Emit32(MethodTable::enum_flag_ContainsPointers);

    CodeLabel *pointerLabel = sl.NewCodeLabel();

    // jne              pointerLabel
    sl.X86EmitCondJump(pointerLabel, X86CondCode::kJNE);

    // We have no pointers - emit a simple inline copy loop

    // mov             ecx, [ecx]MethodTable.m_BaseSize
    sl.X86EmitOffsetModRM(0x8b, kECX, kECX, offsetof(MethodTable, m_BaseSize));

    // sub ecx,12
    sl.X86EmitSubReg(kECX, 12);

    CodeLabel *loopLabel = sl.NewCodeLabel();

    sl.EmitLabel(loopLabel);

    // mov edx,[ebx+ecx]
    sl.X86EmitOp(0x8b, kEDX, kEBX, 0, kECX, 1);

    // mov [eax+ecx+4],edx
    sl.X86EmitOp(0x89, kEDX, kEAX, 4, kECX, 1);

    // sub ecx,4
    sl.X86EmitSubReg(kECX, 4);

    // jg loopLabel
    sl.X86EmitCondJump(loopLabel, X86CondCode::kJGE);

    sl.X86EmitPopReg(kEBX);

    sl.X86EmitReturn(0);

    // Arrive at this label if there are pointers in the object
    sl.EmitLabel(pointerLabel);

    // Do call to CopyValueClassUnchecked(object, data, pMT)

    // Pass pMT (still in ECX)
    sl.X86EmitPushReg(kECX);

    // Pass data (still in EBX)
    sl.X86EmitPushReg(kEBX);

    // Save the address of the object just allocated
    // mov ebx,eax
    sl.Emit16(0xD88B);

    // Pass address of first user byte in the newly allocated object
    sl.X86EmitAddReg(kEAX, 4);
    sl.X86EmitPushReg(kEAX);

    // call CopyValueClass
    sl.X86EmitCall(sl.NewExternalCodeLabel((LPVOID) CopyValueClassUnchecked), 12);

    // Restore the address of the newly allocated object and return it.
    // mov eax,ebx
    sl.Emit16(0xC38B);

    sl.X86EmitPopReg(kEBX);

    sl.X86EmitReturn(0);

    // Come here in case of no space
    sl.EmitLabel(noAlloc);

    // Release the lock in the uniprocessor case
    EmitNoAllocCode(&sl, flags);

    // Come here in case of failure to get the lock
    sl.EmitLabel(noLock);

    // Restore the address of the value to be boxed
    // mov edx,ebx
    sl.Emit16(0xD38B);

    // pop ebx
    sl.X86EmitPopReg(kEBX);

    // Jump to the slow version of JIT_Box
    sl.X86EmitNearJump(sl.NewExternalCodeLabel(hlpFuncTable[CORINFO_HELP_BOX].pfnHelper));

    Stub *pStub = sl.Link(SystemDomain::System()->GetHighFrequencyHeap());

    return (void *)pStub->GetEntryPoint();
}


Object* F_CALL_CONV UnframedAllocateObjectArray(MethodTable *ElementType, DWORD cElements)
{
    return OBJECTREFToObject( AllocateObjectArray(cElements, TypeHandle(ElementType), FALSE) );
}


Object* F_CALL_CONV UnframedAllocatePrimitiveArray(CorElementType type, DWORD cElements)
{
    return OBJECTREFToObject( AllocatePrimitiveArray(type, cElements, FALSE) );
}


void *JIT_TrialAlloc::GenAllocArray(Flags flags)
{
    CPUSTUBLINKER sl;

    CodeLabel *noLock  = sl.NewCodeLabel();
    CodeLabel *noAlloc = sl.NewCodeLabel();

    // Get the arguments from the stack into the registers
    static const BYTE getArgs[]  = { 0x8b, 0x4C, 0x24, 0x04,   // mov ecx, [esp+4]
                     0x8b, 0x54, 0x24, 0x08 }; // mov edx, [esp+8]
    sl.EmitBytes(getArgs, sizeof(getArgs));

    // We were passed a type descriptor in ECX, which contains the (shared)
    // array method table and the element type.

    // If this is the allocator for use from unmanaged code, ECX contains the
    // element type descriptor, or the CorElementType.

    // We need to save ECX for later

    // push ecx
    sl.X86EmitPushReg(kECX);

    // The element count is in EDX - we need to save it for later.

    // push edx
    sl.X86EmitPushReg(kEDX);

    if (flags & NO_FRAME)
    {
        if (flags & OBJ_ARRAY)
        {
            // mov ecx, [g_pPredefinedArrayTypes[ELEMENT_TYPE_OBJECT]]
            sl.Emit16(0x0d8b);
            sl.Emit32((int)(size_t)&g_pPredefinedArrayTypes[ELEMENT_TYPE_OBJECT]);
        }
        else
        {
            // mov ecx,[g_pPredefinedArrayTypes+ecx*4]
            sl.Emit8(0x8b);
            sl.Emit16(0x8d0c);
            sl.Emit32((int)(size_t)&g_pPredefinedArrayTypes);

            // test ecx,ecx
            sl.Emit16(0xc985);

            // je noLock
            sl.X86EmitCondJump(noLock, X86CondCode::kJZ);
        }

        // we need to load the true method table from the type desc
        sl.X86EmitIndexRegLoad(kECX, kECX, offsetof(ArrayTypeDesc,m_TemplateMT));
    }
    else
    {
        // we need to load the true method table from the type desc
        sl.X86EmitIndexRegLoad(kECX, kECX, offsetof(ArrayTypeDesc,m_TemplateMT)-2);
    }

    // Instead of doing elaborate overflow checks, we just limit the number of elements
    // to (LARGE_OBJECT_SIZE - 256)/LARGE_ELEMENT_SIZE or less. As the jit will not call
    // this fast helper for element sizes bigger than LARGE_ELEMENT_SIZE, this will
    // avoid avoid all overflow problems, as well as making sure big array objects are
    // correctly allocated in the big object heap.

    // cmp edx,(LARGE_OBJECT_SIZE - 256)/LARGE_ELEMENT_SIZE
    sl.Emit16(0xfa81);
    sl.Emit32((LARGE_OBJECT_SIZE - 256)/LARGE_ELEMENT_SIZE);

    // jae noLock - seems tempting to jump to noAlloc, but we haven't taken the lock yet
    sl.X86EmitCondJump(noLock, X86CondCode::kJAE);

    if (flags & OBJ_ARRAY)
    {
        // In this case we know the element size is sizeof(void *), or 4 for x86
        // This helps us in two ways - we can shift instead of multiplying, and
        // there's no need to align the size either

        _ASSERTE(sizeof(void *) == 4);

        // mov eax, [ecx]MethodTable.m_BaseSize
        sl.X86EmitIndexRegLoad(kEAX, kECX, offsetof(MethodTable, m_BaseSize));

        // lea eax, [eax+edx*4]
        sl.X86EmitOp(0x8d, kEAX, kEAX, 0, kEDX, 4);
    }
    else
    {
        // movzx eax, [ECX]MethodTable.m_wFlags /* component size */
        sl.Emit8(0x0f);
        sl.X86EmitOffsetModRM(0xb7, kEAX, kECX, offsetof(MethodTable, m_wFlags /* component size */));

        // mul eax, edx
        sl.Emit16(0xe2f7);

        // add eax, [ecx]MethodTable.m_BaseSize
        sl.X86EmitOffsetModRM(0x03, kEAX, kECX, offsetof(MethodTable, m_BaseSize));
    }

#if DATA_ALIGNMENT == 4
    if (flags & OBJ_ARRAY)
    {
        // No need for rounding in this case - element size is 4, and m_BaseSize is guaranteed
        // to be a multiple of 4.
    }
    else
#endif
    {
        // round the size to a multiple of 4

        // add eax, 3
        sl.X86EmitAddReg(kEAX, (DATA_ALIGNMENT-1));

        // and eax, ~3
        sl.Emit16(0xe083);
        sl.Emit8(~(DATA_ALIGNMENT-1));
    }

    flags = (Flags)(flags | SIZE_IN_EAX);

    // Emit the main body of the trial allocator, be it SP or MP
    EmitCore(&sl, noLock, noAlloc, flags);

    // Here we are at the end of the success case - store element count
    // and possibly the element type descriptor and return

    // pop edx - element count
    sl.X86EmitPopReg(kEDX);

    // pop ecx - array type descriptor
    sl.X86EmitPopReg(kECX);

    // mov             dword ptr [eax]ArrayBase.m_NumComponents, edx
    sl.X86EmitIndexRegStore(kEAX, offsetof(ArrayBase,m_NumComponents), kEDX);

    if (flags & OBJ_ARRAY)
    {
        // need to store the element type descriptor

        if ((flags & NO_FRAME) == 0)
        {
        // mov ecx, [ecx]ArrayTypeDescriptor.m_Arg
        sl.X86EmitIndexRegLoad(kECX, kECX, offsetof(ArrayTypeDesc,m_Arg)-2);
        }

        // mov [eax]PtrArray.m_ElementType, ecx
        sl.X86EmitIndexRegStore(kEAX, offsetof(PtrArray,m_ElementType), kECX);
    }

#if CHECK_APP_DOMAIN_LEAKS
    EmitSetAppDomain(&sl);
#endif

    // no stack parameters
    sl.X86EmitReturn(0);

    // Come here in case of no space
    sl.EmitLabel(noAlloc);

    // Release the lock in the uniprocessor case
    EmitNoAllocCode(&sl, flags);

    // Come here in case of failure to get the lock
    sl.EmitLabel(noLock);

    // pop edx - element count
    sl.X86EmitPopReg(kEDX);

    // pop ecx - array type descriptor
    sl.X86EmitPopReg(kECX);

    // No need to push ECX and EDX because they have been put onto the stack by the caller of this stub
    if (flags & NO_FRAME)
    {
        if (flags & OBJ_ARRAY)
        {
            // Jump to the unframed helper
            sl.X86EmitNearJump(sl.NewExternalCodeLabel((LPVOID) UnframedAllocateObjectArray));
        }
        else
        {
            // Jump to the unframed helper
            sl.X86EmitNearJump(sl.NewExternalCodeLabel((LPVOID) UnframedAllocatePrimitiveArray));
        }
    }
    else
    {
    // Jump to the framed helper
    sl.Emit16(0x25ff);
    sl.Emit32((int)(size_t)&hlpFuncTable[CORINFO_HELP_NEWARR_1_DIRECT].pfnHelper);
    }

    Stub *pStub = sl.Link(SystemDomain::System()->GetHighFrequencyHeap());

    return (void *)pStub->GetEntryPoint();
}



static StringObject* F_CALL_CONV UnframedAllocateString(DWORD stringLength)
{
    STRINGREF result;
    result = SlowAllocateString(stringLength+1);
    result->SetStringLength(stringLength);
    return((StringObject*) OBJECTREFToObject(result));
}



HCIMPL1(StringObject*, FramedAllocateString, DWORD stringLength)
    StringObject* result;
    HELPER_METHOD_FRAME_BEGIN_RET_0();    // Set up a frame
    result = UnframedAllocateString(stringLength);
    HELPER_METHOD_FRAME_END();
    return result;
HCIMPLEND


void *JIT_TrialAlloc::GenAllocString(Flags flags)
{
    CPUSTUBLINKER sl;

    CodeLabel *noLock  = sl.NewCodeLabel();
    CodeLabel *noAlloc = sl.NewCodeLabel();

    // The caller of this stub uses __cdecl instead of __fastcall so the argument needs
    // to be read of the stack
    static const BYTE getECX[] = { 0x8b, 0x4C, 0x24, 0x04 }; // mov ecx, [esp+4]
    sl.EmitBytes(getECX, sizeof(getECX));

    // We were passed the number of characters in ECX

    // push ecx
    sl.X86EmitPushReg(kECX);

    // mov eax, ecx
    sl.Emit16(0xc18b);

    // we need to load the method table for string from the global

    // mov ecx, [g_pStringMethodTable]
    sl.Emit16(0x0d8b);
    sl.Emit32((int)(size_t)&g_pStringClass);

    // Instead of doing elaborate overflow checks, we just limit the number of elements
    // to (LARGE_OBJECT_SIZE - 256)/sizeof(WCHAR) or less.
    // This will avoid avoid all overflow problems, as well as making sure
    // big string objects are correctly allocated in the big object heap.

    _ASSERTE(sizeof(WCHAR) == 2);

    // cmp edx,(LARGE_OBJECT_SIZE - 256)/sizeof(WCHAR)
    sl.Emit16(0xf881);
    sl.Emit32((LARGE_OBJECT_SIZE - 256)/sizeof(WCHAR));

    // jae noLock - seems tempting to jump to noAlloc, but we haven't taken the lock yet
    sl.X86EmitCondJump(noLock, X86CondCode::kJAE);

    // mov edx, [ecx]MethodTable.m_BaseSize
    sl.X86EmitIndexRegLoad(kEDX, kECX, offsetof(MethodTable, m_BaseSize));

    // Calculate the final size to allocate.
    // We need to calculate baseSize + cnt*2, then round that up by adding 3 and anding ~3.

    // lea eax, [edx+eax*2+5]
    sl.X86EmitOp(0x8d, kEAX, kEDX, 2 + (DATA_ALIGNMENT-1), kEAX, 2);

    // and eax, ~3
    sl.Emit16(0xe083);
    sl.Emit8(~(DATA_ALIGNMENT-1));

    flags = (Flags)(flags | SIZE_IN_EAX);

    // Emit the main body of the trial allocator, be it SP or MP
    EmitCore(&sl, noLock, noAlloc, flags);

    // Here we are at the end of the success case - store element count
    // and possibly the element type descriptor and return

#if CHECK_APP_DOMAIN_LEAKS
    EmitSetAppDomain(&sl);
#endif

    // pop ecx - element count
    sl.X86EmitPopReg(kECX);

    // mov             dword ptr [eax]ArrayBase.m_StringLength, ecx
    sl.X86EmitIndexRegStore(kEAX, offsetof(StringObject,m_StringLength), kECX);

    // inc ecx
    sl.Emit8(0x41);

    // mov             dword ptr [eax]ArrayBase.m_ArrayLength, ecx
    sl.X86EmitIndexRegStore(kEAX, offsetof(StringObject,m_ArrayLength), kECX);

    // no stack parameters
    sl.X86EmitReturn(0);

    // Come here in case of no space
    sl.EmitLabel(noAlloc);

    // Release the lock in the uniprocessor case
    EmitNoAllocCode(&sl, flags);

    // Come here in case of failure to get the lock
    sl.EmitLabel(noLock);

    // pop ecx - element count
    sl.X86EmitPopReg(kECX);

    // We do not have to push ECX onto the stack because it has been pushed
    // by the caller of this stub.

    if (flags & NO_FRAME)
    {
        // Jump to the unframed helper
        sl.X86EmitNearJump(sl.NewExternalCodeLabel((LPVOID) UnframedAllocateString));
    }
    else
    {
    // Jump to the framed helper
    sl.X86EmitNearJump(sl.NewExternalCodeLabel((LPVOID) FramedAllocateString));
    }

    Stub *pStub = sl.Link(SystemDomain::System()->GetHighFrequencyHeap());

    return (void *)pStub->GetEntryPoint();
}


FastStringAllocatorFuncPtr fastStringAllocator;

FastObjectArrayAllocatorFuncPtr fastObjectArrayAllocator;

FastPrimitiveArrayAllocatorFuncPtr fastPrimitiveArrayAllocator;


// Note that this helper cannot be used directly since it doesn't preserve EDX

HCIMPL1(static void*, JIT_GetSharedStaticBase, DWORD dwClassDomainID)

    THROWSCOMPLUSEXCEPTION();

    DomainLocalClass *pLocalClass;

    HELPER_METHOD_FRAME_BEGIN_RET_0();    // Set up a frame

    AppDomain *pDomain = SystemDomain::GetCurrentDomain();
    DomainLocalBlock *pBlock = pDomain->GetDomainLocalBlock();
    if (dwClassDomainID >= pBlock->GetClassCount()) {
        pBlock->EnsureIndex(SharedDomain::GetDomain()->GetMaxSharedClassIndex());
        _ASSERTE (dwClassDomainID < pBlock->GetClassCount());
    }
    
    MethodTable *pMT = SharedDomain::GetDomain()->FindIndexClass(dwClassDomainID);
    _ASSERTE(pMT != NULL);

    OBJECTREF throwable = NULL;
    GCPROTECT_BEGIN(throwable);
    if (!pMT->CheckRunClassInit(&throwable, &pLocalClass))
      COMPlusThrow(throwable);
    GCPROTECT_END();
    HELPER_METHOD_FRAME_END();

    return pLocalClass;

HCIMPLEND

// For this helper, ECX contains the class domain ID, and the 
// shared static base is returned in EAX.  EDX is preserved.

// "init" should be the address of a routine which takes an argument of
// the class domain ID, and returns the static base pointer

static void EmitFastGetSharedStaticBase(CPUSTUBLINKER *psl, CodeLabel *init)
{
    CodeLabel *DoInit = psl->NewCodeLabel();

    // The caller of this stub uses __cdecl instead of __fastcall so the argument needs
    // to be read from the stack
    static const BYTE getArg1[] = { 0x8b, 0x4C, 0x24, 0x04 }; // mov ecx, [esp+4]
    psl->EmitBytes(getArg1, sizeof(getArg1));

    // mov eax GetAppDomain()
    psl->X86EmitTLSFetch(GetAppDomainTLSIndex(), kEAX, (1<<kECX)|(1<<kEDX));

    // cmp ecx [eax->m_sDomainLocalBlock.m_cSlots]
    psl->X86EmitOffsetModRM(0x3b, kECX, kEAX, AppDomain::GetOffsetOfSlotsCount());
    
    // jb init
    psl->X86EmitCondJump(DoInit, X86CondCode::kJNB);

    // mov eax [eax->m_sDomainLocalBlock.m_pSlots]
    psl->X86EmitIndexRegLoad(kEAX, kEAX, (__int32) AppDomain::GetOffsetOfSlotsPointer());

    // mov eax [eax + ecx*4]
    psl->X86EmitOp(0x8b, kEAX, kEAX, 0, kECX, 4);

    // btr eax, INTIALIZED_FLAG_BIT
    static BYTE code[] = {0x0f, 0xba, 0xf0, DomainLocalBlock::INITIALIZED_FLAG_BIT};
    psl->EmitBytes(code, sizeof(code));

    // jnc init
    psl->X86EmitCondJump(DoInit, X86CondCode::kJNC);

    // ret
    psl->X86EmitReturn(0);

    // DoInit: 
    psl->EmitLabel(DoInit);
   
    // push edx (must be preserved)
    psl->X86EmitPushReg(kEDX);

    // Push copy of arg1 onto the stack for the init function
    psl->X86EmitPushReg(kECX);
    // call init
    psl->X86EmitCall(init, 0);
    // Pop the copy of arg1 off the stack
    psl->X86EmitPopReg(kECX);

    // pop edx  
    psl->X86EmitPopReg(kEDX);

    // ret
    psl->X86EmitReturn(0);
}

void *GenFastGetSharedStaticBase()
{
    CPUSTUBLINKER sl;

    CodeLabel *init = sl.NewExternalCodeLabel((LPVOID) JIT_GetSharedStaticBase);
    
    EmitFastGetSharedStaticBase(&sl, init);

    Stub *pStub = sl.Link(SystemDomain::System()->GetHighFrequencyHeap());

    return (void*) pStub->GetEntryPoint();
}


FCDECL1(Object*, JIT_NewFast, CORINFO_CLASS_HANDLE typeHnd_);

/*********************************************************************/
// Initialize the part of the JIT helpers that require very little of
// EE infrastructure to be in place.
/*********************************************************************/
BOOL InitJITHelpers1()
{

    // Init GetThread function
    _ASSERTE(GetThread != NULL);
    hlpFuncTable[CORINFO_HELP_GET_THREAD].pfnHelper = (void *) GetThread;

    _ASSERTE(hlpFuncTable[CORINFO_HELP_NEWSFAST].pfnHelper == JIT_NewFast);
    _ASSERTE(hlpFuncTable[CORINFO_HELP_NEWSFAST_ALIGN8].pfnHelper == JIT_NewFast);

    hlpFuncTable[CORINFO_HELP_NEWSFAST].pfnHelper = (LPVOID) JIT_TrialAllocSFastSP;
    hlpFuncTable[CORINFO_HELP_NEWSFAST_ALIGN8].pfnHelper = (LPVOID) JIT_TrialAllocSFastSP;

    _ASSERTE(g_SystemInfo.dwNumberOfProcessors != 0);


    _ASSERTE(hlpFuncTable[CORINFO_HELP_NEWSFAST].pfnHelper == (void *)JIT_TrialAllocSFastSP);
    _ASSERTE(hlpFuncTable[CORINFO_HELP_NEWSFAST_ALIGN8].pfnHelper == (void *)JIT_TrialAllocSFastSP);

    JIT_TrialAlloc::Flags flags = JIT_TrialAlloc::NORMAL;
    
    if (g_SystemInfo.dwNumberOfProcessors != 1)
        flags = JIT_TrialAlloc::MP_ALLOCATOR;


    BOOL fAllocatorsInitialized = FALSE;

    COMPLUS_TRY 
    {
#ifdef PROFILING_SUPPORTED
        if (!((CORProfilerTrackAllocationsEnabled()) || (LoggingOn(LF_GCALLOC, LL_INFO10))))
#else
        if (!LoggingOn(LF_GCALLOC, LL_INFO10))
#endif
        {
            // Replace the slow helpers with faster version
            hlpFuncTable[CORINFO_HELP_NEWSFAST].pfnHelper = JIT_TrialAlloc::GenAllocSFast(flags);
            hlpFuncTable[CORINFO_HELP_NEWSFAST_ALIGN8].pfnHelper = JIT_TrialAlloc::GenAllocSFast((JIT_TrialAlloc::Flags)(flags|JIT_TrialAlloc::ALIGN8 | JIT_TrialAlloc::ALIGN8OBJ));       
            hlpFuncTable[CORINFO_HELP_BOX].pfnHelper = JIT_TrialAlloc::GenBox(flags);
            hlpFuncTable[CORINFO_HELP_NEWARR_1_OBJ].pfnHelper = JIT_TrialAlloc::GenAllocArray((JIT_TrialAlloc::Flags)(flags|JIT_TrialAlloc::OBJ_ARRAY));
            hlpFuncTable[CORINFO_HELP_NEWARR_1_VC].pfnHelper = JIT_TrialAlloc::GenAllocArray(flags);

            fastObjectArrayAllocator = (FastObjectArrayAllocatorFuncPtr)JIT_TrialAlloc::GenAllocArray((JIT_TrialAlloc::Flags)(flags|JIT_TrialAlloc::NO_FRAME|JIT_TrialAlloc::OBJ_ARRAY));
            fastPrimitiveArrayAllocator = (FastPrimitiveArrayAllocatorFuncPtr)JIT_TrialAlloc::GenAllocArray((JIT_TrialAlloc::Flags)(flags|JIT_TrialAlloc::NO_FRAME));

            // If allocation logging is on, then we divert calls to FastAllocateString to an Ecall method, not this
            // generated method. Find this hack in Ecall::Init() in ecall.cpp. 
            (*FCallFastAllocateStringImpl) = (LPVOID) (FastStringAllocatorFuncPtr) JIT_TrialAlloc::GenAllocString(flags);

            // generate another allocator for use from unmanaged code (won't need a frame)
            fastStringAllocator = (FastStringAllocatorFuncPtr) JIT_TrialAlloc::GenAllocString((JIT_TrialAlloc::Flags)(flags|JIT_TrialAlloc::NO_FRAME));
                                                               //UnframedAllocateString;
            hlpFuncTable[CORINFO_HELP_GETSHAREDSTATICBASE].pfnHelper = GenFastGetSharedStaticBase();
        }
        else
        {
            // Replace the slow helpers with faster version
            hlpFuncTable[CORINFO_HELP_NEWSFAST].pfnHelper = hlpFuncTable[CORINFO_HELP_NEWFAST].pfnHelper;
            hlpFuncTable[CORINFO_HELP_NEWSFAST_ALIGN8].pfnHelper = hlpFuncTable[CORINFO_HELP_NEWFAST].pfnHelper;
            hlpFuncTable[CORINFO_HELP_NEWARR_1_OBJ].pfnHelper = hlpFuncTable[CORINFO_HELP_NEWARR_1_DIRECT].pfnHelper;
            hlpFuncTable[CORINFO_HELP_NEWARR_1_VC].pfnHelper = hlpFuncTable[CORINFO_HELP_NEWARR_1_DIRECT].pfnHelper;
            // hlpFuncTable[CORINFO_HELP_NEW_CROSSCONTEXT].pfnHelper = &JIT_NewCrossContextProfiler;

            fastObjectArrayAllocator = UnframedAllocateObjectArray;
            fastPrimitiveArrayAllocator = UnframedAllocatePrimitiveArray;

            // If allocation logging is on, then we divert calls to FastAllocateString to an Ecall method, not this
            // generated method. Find this hack in Ecall::Init() in ecall.cpp. 
            (*FCallFastAllocateStringImpl) = (LPVOID) (FastStringAllocatorFuncPtr)FramedAllocateString;

            // This allocator is used from unmanaged code
            fastStringAllocator = UnframedAllocateString;

            hlpFuncTable[CORINFO_HELP_GETSHAREDSTATICBASE].pfnHelper = (LPVOID) JIT_GetSharedStaticBase;
        }

        fAllocatorsInitialized = TRUE;
    }
    COMPLUS_CATCH 
    {
    }
    COMPLUS_END_CATCH

    if (!fAllocatorsInitialized) {
        return FALSE;
    }

    return TRUE;
}

/*********************************************************************/

