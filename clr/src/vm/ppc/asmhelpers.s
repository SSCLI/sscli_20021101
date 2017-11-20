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


#include "eecallconv.h"
#include "asmconstants.h"

// mnemonics for the PowerPC registers
#define sp      r1
#define t0      r2   // This is TOC on Mac OS 9
#define a0      r3   // First arg register.  This is also the return-value.
#define a1      r4
#define a2      r5
#define a3      r6
#define a4      r7
#define a5      r8
#define a6      r9
#define a7      r10
#define t1      r11
// r12 is the branch-target
#define s17     r13  // Callee-preserved registers
#define s16     r14
#define s15     r15
#define s14     r16
#define s13     r17
#define s12     r18
#define s11     r19
#define s10     r20
#define s9      r21
#define s8      r22
#define s7      r23
#define s6      r24
#define s5      r25
#define s4      r26
#define s3      r28
#define s2      r29
#define s1      r30
#define s0      r31

#define roundup(n, roundto)  ((n)+roundto-((n) % roundto))

#define RED_ZONE 224
#define LINKAGE_AREA 24
// offset 0 in the linkage area is the caller's saved SP
// offset 4 is the saved CR
// offset 8 as the saved LR
// offsets 12, 16 and 20 are reserved.

// A handy macro for declaring a public function
// The first argument is the function name
.macro ASMFUNC
  .align 2
  .globl _$0
_$0:
.endmacro


// Do the equivalent of "extern int X;", where
// the first argument is the variable name.
// This must be invoked within a .data section.
.macro EXTERN_GLOBAL
    .non_lazy_symbol_pointer
    L_$0_non_lazy_ptr:
        .indirect_symbol _$0
	.long 0
.endmacro


// Enter a spinlock.
//  First argument is the spinlock variable's name
// On return, a7 must be preserved until the SPINLOCK_EXIT is complete.
.macro SPINLOCK_ENTER
    mflr r0	// save the return address in r0
    bl 0f
0:
    mflr t1     // move link register into t1
    mtlr r0     // restore the return address back into the lr
    addis a7, t1, ha16(L_$0_non_lazy_ptr-0b)
    lwz a7, lo16(L_$0_non_lazy_ptr-0b)(a7)
    // a7 now holds the address of the spinlock variable
    li t1, 1
1:    
    lwarx   t0, 0, a7  // load the current spinlock value
    cmpwi   t0, 0      // check if it is zero
    bne     1b         // retry if the value isn't zero - the lock is held
    stwcx.  t1, 0, a7  // store 1 to lock it
    bne     1b         // store failed - someone else beat us to it    
.endmacro

// Leave a spinlock.  It assumes a7 still points to the spinlock variable,
// and trashes t0 and t1
.macro SPINLOCK_EXIT
    li t1, 0
0:
    lwarx   t0, 0, a7
    stwcx.  t1, 0, a7
    bne 0b
.endmacro


.data
EXTERN_GLOBAL iSpinLock
EXTERN_GLOBAL g_dwTPStubAddr
EXTERN_GLOBAL g_dwOOContextAddr
EXTERN_GLOBAL GetThread


.text

// void [__cdecl|__stdcall] ResetCurrentContext(void);
//
// Restores the floating-point hardware to a known state, leaving
// precision and rounding unmodified.
//
ASMFUNC ResetCurrentContext
    stfd  f0, -8(sp)   // preserve the current value in f0
    mffs  f0            // move fpscr to the upper 32 bits of f0
    stfd  f0, -16(sp)   // store f0 to the stack
    lwz   t0, -12(sp)   // load one word from it... the fpscr value
    addis t1, 0, 0xc010 // clear all bits except 30/31 (RN, rounding control)
    and   t0, t0, t1    //  and 20 (reserved)
    stw   t0, -12(sp)   // store the new fpscr value to memory
    lfd   f0,-16(sp)    // load f0 with the update fpscr value
    mtfsf 7, f0         // move it to fpscr
    lfd   f0, -8(sp)    // restore f0
    blr
    
.macro CALLEESAVED_REGISTERS_GP
    $0 r13, (0*4+$1)($2)
    $0 r14, (1*4+$1)($2)
    $0 r15, (2*4+$1)($2)
    $0 r16, (3*4+$1)($2)
    $0 r17, (4*4+$1)($2)
    $0 r18, (5*4+$1)($2)
    $0 r19, (6*4+$1)($2)
    $0 r20, (7*4+$1)($2)
    $0 r21, (8*4+$1)($2)
    $0 r22, (9*4+$1)($2)
    $0 r23, (10*4+$1)($2)
    $0 r24, (11*4+$1)($2)
    $0 r25, (12*4+$1)($2)
    $0 r26, (13*4+$1)($2)
    $0 r27, (14*4+$1)($2)
    $0 r28, (15*4+$1)($2)
    $0 r29, (16*4+$1)($2)
    $0 r30, (17*4+$1)($2)
    $0 r31, (18*4+$1)($2)
.endmacro

.macro CALLEESAVED_REGISTERS_FP
    $0 f14, (0*8+$1)($2)
    $0 f15, (1*8+$1)($2)
    $0 f16, (2*8+$1)($2)
    $0 f17, (3*8+$1)($2)
    $0 f18, (4*8+$1)($2)
    $0 f19, (5*8+$1)($2)
    $0 f20, (6*8+$1)($2)
    $0 f21, (7*8+$1)($2)
    $0 f22, (8*8+$1)($2)
    $0 f23, (9*8+$1)($2)
    $0 f24, (10*8+$1)($2)
    $0 f25, (11*8+$1)($2)
    $0 f26, (12*8+$1)($2)
    $0 f27, (13*8+$1)($2)
    $0 f28, (14*8+$1)($2)
    $0 f29, (15*8+$1)($2)
    $0 f30, (16*8+$1)($2)
    $0 f31, (17*8+$1)($2)
.endmacro
   	
.macro ARGUMENT_REGISTERS_GP
    $0 r3,  (0*4+$1)($2)
    $0 r4,  (1*4+$1)($2)
    $0 r5,  (2*4+$1)($2)
    $0 r6,  (3*4+$1)($2)
    $0 r7,  (4*4+$1)($2)
    $0 r8,  (5*4+$1)($2)
    $0 r9,  (6*4+$1)($2)
    $0 r10, (7*4+$1)($2)
.endmacro
	
.macro ARGUMENT_REGISTERS_FP
    $0 f1,  (0*8+$1)($2)
    $0 f2,  (1*8+$1)($2)
    $0 f3,  (2*8+$1)($2)
    $0 f4,  (3*8+$1)($2)
    $0 f5,  (4*8+$1)($2)
    $0 f6,  (5*8+$1)($2)
    $0 f7,  (6*8+$1)($2)
    $0 f8,  (7*8+$1)($2)
    $0 f9,  (8*8+$1)($2)
    $0 f10, (9*8+$1)($2)
    $0 f11, (10*8+$1)($2)
    $0 f12, (11*8+$1)($2)
    $0 f13, (12*8+$1)($2)
.endmacro

/*
; This is a helper that we use to raise the correct managed exception with
; the necessary frame (after taking a fault in jitted code).
;
; Inputs:
;      all registers still have the value
;      they had at the time of the fault, except
;              Iar points to this function
;              r0 contains the original EIP
;
; What it does:
;      The exception to be thrown is stored in m_pLastThrownObjectHandle.
;      We push a FaultingExcepitonFrame on the stack, and then we call
;      complus throw.
;
*/
ASMFUNC NakedThrowHelper

        // skip the reserved red zone & create caller linkage area
        stwu sp, -roundup(LINKAGE_AREA+RED_ZONE, 16)(sp)
        
        // save the return address
        stw r0, 8(sp)
        
        // allocate the rest of FaultingExceptionFrame and callee linkage area
    #if (LINKAGE_AREA+8 + FaultingExceptionFrame_m_Link) % 16 != 0
    #error Stack frame not alligned
    #endif                         
        stwu sp, -(LINKAGE_AREA+8 + FaultingExceptionFrame_m_Link)(sp)       

        // save cr
        mfcr r0                             
        stw r0,(LINKAGE_AREA+8 + FaultingExceptionFrame_m_regs+CalleeSavedRegisters_cr)(sp)

        // save general regs
        stmw r13,(LINKAGE_AREA+8 + FaultingExceptionFrame_m_regs+CalleeSavedRegisters_r)(sp)

        // save float regs
        CALLEESAVED_REGISTERS_FP stfd,(LINKAGE_AREA+8 + FaultingExceptionFrame_m_regs+CalleeSavedRegisters_f),sp

        // a0 = pFrame
        la a0, (LINKAGE_AREA+8)(sp)

        bl _LinkFrameAndThrow
        
        // this should never return, but...
        lwz sp, 0(sp)
        lwz r0, 8(sp)
        lwz sp, 0(sp) // skip the reserved red zone
        mtlr r0
        blr


// void ResumeAtJitEHHelper(CONTEXT *pContext)
//
// Restores the CPU registers from the CONTEXT record, including
// EIP.
//
ASMFUNC ResumeAtJitEHHelper

        // load cr
        lwz r0,(EHContext_CR)(sp)
        mtcr r0

        // load float regs
        CALLEESAVED_REGISTERS_FP lfd,EHContext_F,a0
        
        // load general regs
        mr r2, a0
        lmw r3,(EHContext_R+3*4)(r2)
        lwz r2,(EHContext_R+4)(r2)
        
        // allocate linkage area
        addi r2,r2,-LINKAGE_AREA
        
        // align the stack
        clrrwi r2,r2,4

        // try to fill the linkage area with a somehow meaningful content
        stw r30,0(r2)

        // set the new sp
        mr sp,r2

        // make the jump
        mtctr r12
        bctr

// int __stdcall CallJitEHFilterHelper(size_t *pShadowSP, EHContext *pContext);
//   Restores only r3..r31 from the EHContext
//
ASMFUNC CallJitEHFilterHelper

        // prolog
        mflr r0
        stw r0, 8(sp)
        stwu sp, -roundup(LINKAGE_AREA + 4*NUM_CALLEESAVED_REGISTERS, 16)(sp)

        // Write sp to the shadowSP slot
        cmpwi a0, 0   
        beq LDONE_SHADOWSP_FILTER
        ori t0, sp, SHADOW_SP_IN_FILTER_ASM
        stw t0, 0(a0)
    LDONE_SHADOWSP_FILTER:

        // store callee saved regs
        stmw r13, (LINKAGE_AREA)(sp)

        // load general regs
        mr r2, a1
        lmw r3,(EHContext_R+3*4)(r2)
              
        // make the call
        mtctr r12
        bctrl

        // restore callee saved regs
        lmw r13, (LINKAGE_AREA)(sp)

        // epilog
        lwz sp, 0(sp)
        lwz r0, 8(sp)
        mtlr r0
        blr


// void __stdcall CallJitEHFinallyHelper(size_t *pShadowSP, EHContext *pContext);
//   Restores only r3..r31 from the EHContext
ASMFUNC CallJitEHFinallyHelper

        // prolog
        mflr r0
        stw r0, 8(sp)
        stwu sp, -roundup(LINKAGE_AREA + 4*NUM_CALLEESAVED_REGISTERS, 16)(sp)

        // Write sp to the shadowSP slot
        cmpwi a0, 0   
        beq LDONE_SHADOWSP_FINALLY
        stw sp, 0(a0)
    LDONE_SHADOWSP_FINALLY:

        // store callee saved regs
        stmw    r13, (LINKAGE_AREA)(sp)

        // load general regs
        mr r2, a1
        lmw r3,(EHContext_R+3*4)(r2)
       
        // make the call
        mtctr r12
        bctrl

        // restore callee saved regs
        lmw     r13, (LINKAGE_AREA)(sp)

        // epilog
        lwz sp, 0(sp)
        lwz r0, 8(sp)
        mtlr r0
        blr



// Atomic bit manipulations.  On x86 there are UP and MP versions,
// but for PowerPC, one version, generic (GN) handles both cases.

// void __stdcall OrMaskGN(DWORD volatile *p, const int msk)
ASMFUNC OrMaskGN
        lwarx   t0, 0, a0
        or      t0, t0, a1
        stwcx.  t0, 0, a0
        bne     _OrMaskGN
        sync
        blr

// void __stdcall AndMaskGN(DWORD volatile *p, const int msk)
ASMFUNC AndMaskGN
        lwarx   t0, 0, a0
        and     t0, t0, a1
        stwcx.  t0, 0, a0
        bne     _AndMaskGN
        sync
        blr

// LONG __stdcall ExchangeGN(LONG volatile *Target, LONG Value)
// Returns the previous value of *Target
ASMFUNC ExchangeGN
        lwarx   t0, 0, a0
        stwcx.  a1, 0, a0
        bne     _ExchangeGN
        sync
        mr      a0, t0
        blr


// LONG __stdcall ExchangeAddGN(LONG volatile *Target, LONG value)
// Returns the previous value of *Target
ASMFUNC ExchangeAddGN
        lwarx   t0, 0, a0
        add     t1, t0, a1
        stwcx.  t1, 0, a0
        bne     _ExchangeAddGN
        sync
        mr      a0, t0
        blr


// void *__stdcall CompareExchangeGN(PVOID volatile *Destination,
//                                  PVOID Exchange, PVOID Comparand)
ASMFUNC CompareExchangeGN
        lwarx   t0, 0, a0
        cmpw    t0, a2
        bne     LCompareExchangeGNMismatch
        stwcx.  a1, 0, a0
        bne     _CompareExchangeGN
LCompareExchangeGNMismatch:
        sync
        mr      a0, t0
        blr

// LONG __stdcall IncrementGN(LONG volatile *Target)
ASMFUNC IncrementGN
        lwarx   t0, 0, a0
        addi    t0, t0, 1
        stwcx.  t0, 0, a0
        bne     _IncrementGN
        sync
        mr      a0, t0
        blr

// UINT64 __stdcall IncrementLongGN(UINT64 volatile *Target)
// Note that the return value pointer is in a0, and the Target is in a1
ASMFUNC IncrementLongGN
        SPINLOCK_ENTER iSpinLock
        lwz t1, 4(a0)   // load the low word into t1
        lwz t0, 0(a0)   // load the high word into t0
        addic t1, t1, 1	// increment t1, and record carry in XER[CA]
        addze t0, t0    // add the value of XER[CA] (a bit) to t0
        stw t1, 4(a0)   // store the new low word back to Target
        stw t0, 0(a0)   // store the new high word back to Target
        mr  a1, t1      // return the low 4 bytes in a1
        mr  a0, t0      // return the high 4 bytes in a0
        SPINLOCK_EXIT
        sync
        blr

// LONG __stdcall DecrementGN(LONG volatile *Target)
ASMFUNC DecrementGN
        lwarx   t0, 0, a0
        addi    t0, t0, -1
        stwcx.  t0, 0, a0
        bne     _DecrementGN
        sync
        mr      a0, t0
        blr


// UINT64 __stdcall DecrementLongGN(UINT64 volatile *Target)
ASMFUNC DecrementLongGN
        SPINLOCK_ENTER iSpinLock
        lwz t1, 4(a0)   // load the low word into t1
        lwz t0, 0(a0)   // load the high word into t0
        addic t1, t1, -1// decrement t1, and record carry in XER[CA]
        addme t0, t0    // subtract the value of XER[CA] (a bit) from t0
        stw t1, 4(a0)   // store the new low word back to Target
        stw t0, 0(a0)   // store the new high word back to Target
        mr  a1, t1      // return the low 4 bytes in a1
        mr  a0, t0      // return the high 4 bytes in a0
        SPINLOCK_EXIT
        sync
        blr


#ifdef _DEBUG
// void Frame::CheckExitFrameDebuggerCalls(void)
ASMFUNC CheckExitFrameDebuggerCalls
        // Spill the argument and link registers to the stack
        mflr t0
        stw  t0, 8(sp)
        
        ARGUMENT_REGISTERS_GP stw,(-32),sp
        
        stwu sp, -(LINKAGE_AREA+32)(sp)

        // call void* PerformExitFrameChecks(void)
        bl _PerformExitFrameChecks
        mtctr a0 // move the return value into the link register

        // Restore the argument and link registers back
        lwz sp, 0(sp)
        lwz t0, 8(sp)
        mtlr t0
        
        ARGUMENT_REGISTERS_GP lwz,(-32),sp

        // jump to the returned function pointer
        bctr
#endif //_DEBUG

//-----------------------------------------------------------------------------
// This helper routine enregisters the appropriate arguments and makes the
// actual call.
//-----------------------------------------------------------------------------
//ARG_SLOT
//__stdcall
//#ifdef _DEBUG
//      CallDescrWorkerInternal
//#else
//      CallDescrWorker
//#endif
//                     (LPVOID                   pSrcEnd,
//                      UINT32                   numStackSlots,
//                      const ArgumentRegisters *pArgumentRegisters,
//                      LPVOID                   pTarget
//                     )
#ifdef _DEBUG
ASMFUNC CallDescrWorkerInternal
#else
ASMFUNC CallDescrWorker
#endif
        // prolog
        mflr r0
        stw r0, 8(sp)

        // allocate frame with size roundup(4*numStackSlots+LINKAGE_AREA,16)
        slwi t1, a1, 2 // t1 = size of arguments
        addi t0, t1, LINKAGE_AREA+15
        clrrwi t0, t0, 4
        neg t0, t0
        stwux sp, sp, t0

        // copy the stack over
        sub t0, a0, t1 // t0 = src
        addi t1, sp, LINKAGE_AREA // t1 = dst

        cmpwi a1, 0
        mtctr a1
        beq- LArgCopyDone
LArgCopyLoop:
        lwz a0, 0(t0)
        addi t0, t0, 4
        stw a0, 0(t1)
        addi t1, t1, 4
        bdnz LArgCopyLoop
LArgCopyDone:

        // t0 = pArgumentRegisters
        mr t0, a2

        // r12 = pTarget
        mr r12, a3

        // enregister float registers
        ARGUMENT_REGISTERS_FP lfd,ArgumentRegisters_f,t0
        // enregister integer registers
        ARGUMENT_REGISTERS_GP lwz,ArgumentRegisters_r,t0
   
        // make the call
        
        // The lowest bit indicates the type of callsite to use        
        andi. r0, r12, 1
        bne LReturnBuffer

        mtctr r12
        bctrl
        b LCallDone

LReturnBuffer:
        clrrwi r12, r12, 1
        mtctr r12
        bctrl
        oris r0, r0, 0
        
LCallDone:
        // epilog
        lwz sp, 0(sp)
        lwz r0, 8(sp)
        mtlr r0
        blr

#ifdef _DEBUG
// int __fastcall HelperMethodFrameRestoreState(HelperMethodFrame * pFrame, 
//                                              struct MachState * pState)
  #define pFrame a0
  #define pState a1
#else
// int __fastcall HelperMethodFrameRestoreState(struct MachState * pState)
  #define pState a0
#endif
ASMFUNC HelperMethodFrameRestoreState
        // restore the registers from the m_MachState stucture.  Note that
        // we only do this for register that were not saved on the stack
        // at the time the machine state snapshot was taken.

        lwz t0, MachState__pRetAddr(pState)
        cmpwi t0, 0

#ifdef _DEBUG
        bne+  LNoConfirm
        mflr r0
        stw r0, 8(sp)
        stwu sp, -(LINKAGE_AREA + 8 + 20 * 4)(sp) // build a stack frame

        stmw r13, (LINKAGE_AREA + 8)(sp)

        // a0 == pFrame (this was passed to HelperMethodFrameRestoreState
        la a1, (LINKAGE_AREA + 8)(sp)
        bl _HelperMethodFrameConfirmState

        // on return, r3 == MachState*, move back into pState
        mr pState,r3
        lwz t0, MachState__pRetAddr(pState)
        cmpwi t0, 0

        lwz sp, 0(sp)                      // restore the stack pointer
        lwz r0, 8(sp)                      // restore the link register
        mtlr r0
LNoConfirm:
#endif
        beq+ LDoRet

        // Reload any spilled registers
        addi t0, pState, MachState__Regs // t0=&pState->_Regs[0]
        addi t1, pState, MachState__pRegs// t1=&pState->_pRegs[0]

    // Reload a register if it was spilled:
    //  if (pState->_pRegs[rnum-13] == &pState->_Regs[rnum-13]) {
    //    Reload register rnum from pState->_Regs[rnum]
    //  }
    .macro ReloadRegister
        lwz  t0, MachState__pRegs+($0-13)*4(pState)
        addi t1, pState, MachState__Regs+$0-13
        cmpw  t0, t1
        bne+ 0f
        lwz  r$0, MachState__Regs+($0-13)*4(pState)
    0:
    .endmacro

        ReloadRegister 13
        ReloadRegister 14
        ReloadRegister 15
        ReloadRegister 16
        ReloadRegister 17
        ReloadRegister 18
        ReloadRegister 19
        ReloadRegister 20
        ReloadRegister 21
        ReloadRegister 22
        ReloadRegister 23
        ReloadRegister 24
        ReloadRegister 25
        ReloadRegister 26
        ReloadRegister 27
        ReloadRegister 28
        ReloadRegister 29
        ReloadRegister 30
        ReloadRegister 31

LDoRet:
        li r3, 0
        blr
#undef pFrame
#undef pState


// Note that the debugger skips this entirely when doing SetIP,
// since COMPlusCheckForAbort should always return 0.  Excep.cpp:LeaveCatch
// asserts that to be true.  If this ends up doing more work, then the
// debugger may need additional support.
// void __stdcall JIT_EndCatch()
ASMFUNC JIT_EndCatch

#define FRAME_SIZE roundup(LINKAGE_AREA + 4*3, 16)

    // prolog
    mflr r0
    stw r0, 8(sp)
    stwu sp, -FRAME_SIZE(sp)
    
    li a0,0             // pCurThread
    li a1,0             // pCtx
    li a2,0             // pSEH
    bl _COMPlusEndCatch  // returns old esp value in a0        

    stw a0, (FRAME_SIZE)(sp) // save old esp        
    
    mr a1,a0        // esp              
    mr a2,r30       // ebp    
    lwz a0, (FRAME_SIZE + 8)(sp) // return address
    bl _COMPlusCheckForAbort // returns old esp value

    // at this point, (FRAME_SIZE)(sp) = old esp value 
    //                (FRAME_SIZE + 8)(sp) = return address into jitted code
    //                a0 = 0 (if no abort), address to jump to otherwise
    
    cmpwi a0, 0
    beq LNormalEndCatch
    
    mr r12, a0    
    lwz sp, (FRAME_SIZE)(sp)
    
    // make the jump
    mtctr r12
    bctr

LNormalEndCatch:
    lwz r0, (FRAME_SIZE + 8)(sp)
    lwz sp, (FRAME_SIZE)(sp)
    
    // return as usual
    mtlr r0
    blr
    
#undef FRAME_SIZE    


//---------------------------------------------------------
// Performs an N/Direct call. This is a generic version
// that can handly any N/Direct call but is not as fast
// as more specialized versions.
//---------------------------------------------------------
//
// INT64 __stdcall NDirectGenericStubWorker(Thread *pThread,
//                                          NDirectMethodFrame *pFrame);
//
ASMFUNC NDirectGenericStubWorker

    // prolog
    mflr r0
    stw s0, -4(sp)
    stw r0, 8(sp)
    mr s0, sp

    #define LOCALS  (6*4) // local variables
    #define ARGS    (8*4) // space to pass arguments to worker methods

    // Locals
    #define pHeader (-LOCALS + 0)(s0)
    #define pvfn    (-LOCALS + 4)(s0)
    #define pLocals (-LOCALS + 8)(s0)
    #define pMLCode (-LOCALS + 12)(s0)
    // pad
    // saved s0     (-LOCALS + 28)(s0)

    // Parameters
    #define pThread (LINKAGE_AREA + 0)(s0)
    #define pFrame  (LINKAGE_AREA + 4)(s0)
    
#if NDirectGenericWorkerFrameSize != (LINKAGE_AREA + LOCALS)
 #error asmconstants.h is incorrect
#endif

#if (LINKAGE_AREA + LOCALS + ARGS) % 16 != 0
 #error Stack frame not alligned
#endif 

    stwu sp, -(LINKAGE_AREA + LOCALS + ARGS)(sp)

    stw a0, pThread
    stw a1, pFrame

    la a2, pHeader
    bl _NDirectGenericStubComputeFrameSize

    // a0 = cbAlloc

    // alloca
    addi a0, a0, 15 + 8 * 13 // align + float registers
    clrrwi a0, a0, 4
    neg a0, a0
    stwux s0, sp, a0

    lwz a0, pThread
    lwz a1, pFrame
    lwz a2, pHeader
    la a3, pMLCode
    la a4, pLocals
    la a5, pvfn
    addi a6, sp, LINKAGE_AREA + ARGS // pAlloc
    bl _NDirectGenericStubBuildArguments
    
#if ARGS % 16 != 0
 #error Stack frame not alligned
#endif 
        
    // free the space for worker arguments
    stwu s0, (ARGS)(sp)

    // pvfn is now the function to invoke, pLocals points to the locals, and
    // a0 is flags that are ignored on PPC
    lwz r12, pvfn

    // enregister float registers
    ARGUMENT_REGISTERS_FP lfd,-(LOCALS+8*13),s0
    // enregister integer registers
    ARGUMENT_REGISTERS_GP lwz,LINKAGE_AREA,sp

    // make the call
    mtctr r12
    bctrl

// this label is used by the debugger, and must immediately follow the 'call'.
LNDirectGenericReturnFromCall:

    // reclaim the space for worker arguments
    stwu s0, (-ARGS)(sp)

    // pass the return value
    mr a5, a0
    mr a6, a1

    lwz a0, pThread
    lwz a1, pFrame
    lwz a2, pHeader
    lwz a3, pMLCode
    lwz a4, pLocals
    bl _NDirectGenericStubPostCall
    // return value is in r3:r4

    // epilog
    lwz sp, 0(sp)
    lwz r0, 8(sp)
    lwz s0, -4(sp)
    mtlr r0
    blr

    #undef  pThread
    #undef  pFrame
    #undef  pHeader
    #undef  pvfn
    #undef  pLocals
    #undef  pMLCode
    
    #undef ARGS
    #undef LOCALS
    
.data
    .globl _NDirectGenericReturnFromCall
    .align 2
_NDirectGenericReturnFromCall:
    .long LNDirectGenericReturnFromCall
.text


//==========================================================================
//__declspec (naked) void CallThunk()
ASMFUNC CallThunk
    // Locals
    #define MachStateOff     (LINKAGE_AREA+12)
    #define ArgRegistersOff  -(16 + CalleeSavedRegisters_size + ArgumentRegisters_size)
    #define ThunkAddressOff  -8
    #define LocalFrameSize   roundup((LINKAGE_AREA + 12 + 16 + CalleeSavedRegisters_size + ArgumentRegisters_size + MachState_size), 16)
	
    mflr r0
    stw r0, 8(sp)
    stwu sp, -LocalFrameSize(sp)
    // Store the return address to the stub
    stw r11, ThunkAddressOff+LocalFrameSize(sp)
    addi t0, sp, ArgRegistersOff+LocalFrameSize
	
    // Create ArgumentRegisters structure to protect the arguments
    ARGUMENT_REGISTERS_FP stfd,ArgumentRegisters_f,t0
    ARGUMENT_REGISTERS_GP stw,ArgumentRegisters_r,t0

    // Generate the machine state structure at sp + 24
    // Generate an array of argument register values
    stmw  r13, MachStateOff(sp)

    // Generate an array of pointers to the argument register values
    CALLEESAVED_REGISTERS_GP la,MachStateOff,sp
    stmw  r13, (4*NUM_CALLEESAVED_REGISTERS + MachStateOff)(sp)
  
    addi  t0, sp, MachStateOff
    addi  t1, sp, 8+LocalFrameSize    // Pointer to the return address
    stw   t1, MachState__pRetAddr(t0)
    mfcr  t1                          // Current flags values
    stw   t1, MachState__cr(t0)
   

    // Generate the arguments for the call
    addi  a0, sp, MachStateOff                       // pointer to the machine state
    addi  a1, sp, ArgRegistersOff+LocalFrameSize     // pointer to the argument registers 
    lwz   a2, ThunkAddressOff+LocalFrameSize(sp)     // pointer to the thunk address
	
    // Call the helper to rejit the function
    bl _RejitCalleeMethod
	
    // Save the address of the rejited function
    mr r12, a0
	
    // Restore the state of the machine
	
    // Restore argument registers
    addi t0, sp, ArgRegistersOff+LocalFrameSize
    ARGUMENT_REGISTERS_FP lfd,ArgumentRegisters_f,t0
    ARGUMENT_REGISTERS_GP lwz,ArgumentRegisters_r,t0
   
    // Restore callee saved GP registers
    lmw r13, MachStateOff(sp)
   
    // Remove the scratch frame 
    lwz  sp,  0(sp)
    lwz  r11, 8(sp)
    mtlr r11
    lwz  r11, ThunkAddressOff(sp)  
    // Call the rejitted funtion
    mtctr r12
    bctr
    #undef LocalFrameSize
    #undef MachStateOff
    #undef ArgRegistersOff  
    #undef ThunkAddressOff  

//__declspec (naked) void RejitThunk()
ASMFUNC RejitThunk
    // Locals
    #define MachStateOff     (LINKAGE_AREA+12)
    #define RetValueOff      -24
    #define ThunkAddressOff  -8
    #define LocalFrameSize   roundup((LINKAGE_AREA + 24 + 12 + MachState_size), 16)
    mflr r0
    stw r0, 8(sp)
    stwu sp, -LocalFrameSize(sp)
    // Store the return address to the stub
    stw r11, ThunkAddressOff+LocalFrameSize(sp)

    // Save the return value
    stfd f1, LocalFrameSize+RetValueOff+8(sp)
    stw  r4, LocalFrameSize+RetValueOff+4(sp)
    stw  r3, LocalFrameSize+RetValueOff(sp)
    

    // Generate the machine state structure at sp + 24
    // Generate an array of argument register values
    stmw  r13, MachStateOff(sp)

    // Generate an array of pointers to the argument register values
    CALLEESAVED_REGISTERS_GP la,MachStateOff,sp
    stmw  r13, (4*NUM_CALLEESAVED_REGISTERS + MachStateOff)(sp)

    addi  t0, sp, MachStateOff
    addi  t1, sp, 8+LocalFrameSize    // Pointer to the return address
    stw   t1, MachState__pRetAddr(t0)
    mfcr  t1                          // Current flags values
    stw   t1, MachState__cr(t0)
   

    // Generate the arguments for the call
    addi  a0, sp, MachStateOff                         // pointer to the machine state
    addi  a1, sp, RetValueOff + LocalFrameSize         // pointer to the saved return value 
    lwz   a2, LocalFrameSize+ThunkAddressOff(sp)       // pointer to the thunk address
	  
    // Call the helper to rejit the function
    bl _RejitCallerMethod
	
    // Save the address of the rejited function
    mr r12, a0
	
    // Restore the state of the machine
   
    // Restore callee saved GP registers
    lmw r13, MachStateOff(sp)

    // Restore the return value
    lwz r3, LocalFrameSize+RetValueOff(sp)
    lwz r4, LocalFrameSize+RetValueOff+4(sp) 
    lfd f1, LocalFrameSize+RetValueOff+8(sp)
	
    // Remove the stach frame 
    lwz  sp,  0(sp)
    lwz  r11, 8(sp)
    mtlr r11
    lwz  r11, ThunkAddressOff(sp)  
    // Call the rejitted funtion
    mtctr r12
    bctr
    #undef MachStateOff
    #undef RetValueOff  
    #undef ThunkAddressOff  

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::CallFieldGetter   private
//
//  Synopsis:   Calls the field getter function (Object::__FieldGetter) in
//              managed code by setting up the stack and calling the target
//
//+----------------------------------------------------------------------------
// void __stdcall CRemotingServices__CallFieldGetter(
//    MethodDesc *pMD,
//    LPVOID pThis,
//    LPVOID pFirst,
//    LPVOID pSecond,
//    LPVOID pThird)
ASMFUNC CRemotingServices__CallFieldGetter

    // prolog
    mflr r0
    stw r0, 8(sp)
    stwu sp, -roundup(LINKAGE_AREA + 4*4, 16)(sp)

    addi r11, r3, -METHOD_CALL_PRESTUB_SIZE_ASM // setup the method desc ptr for the stub
    mr r3, r4 // pThis
    mr r4, r5 // pFirst
    mr r5, r6 // pSecond
    mr r6, r7 // pThird

    bl 0f
0:
    mflr r12    // move link register into r12
    addis r12, r12, ha16(L_g_dwTPStubAddr_non_lazy_ptr-0b)
    lwz r12, lo16(L_g_dwTPStubAddr_non_lazy_ptr-0b)(r12)
    lwz r12, 0(r12)

    // call the TP stub
    mtctr r12
    bctrl

    // epilog
    lwz sp, 0(sp)
    lwz r0, 8(sp)
    mtlr r0
    blr

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::CallFieldSetter   private
//
//  Synopsis:   Calls the field setter function (Object::__FieldSetter) in
//              managed code by setting up the stack and calling the target
//
//+----------------------------------------------------------------------------
// void __stdcall CRemotingServices__CallFieldSetter(
//    MethodDesc *pMD,
//    LPVOID pThis,
//    LPVOID pFirst,
//    LPVOID pSecond,
//    LPVOID pThird)
ASMFUNC CRemotingServices__CallFieldSetter
    // prolog
    mflr r0
    stw r0, 8(sp)
    stwu sp, -roundup(LINKAGE_AREA + 3*4, 16)(sp)

    addi r11, r3, -METHOD_CALL_PRESTUB_SIZE_ASM // setup the method desc ptr for the stub
    mr r3, r4 // pThis
    mr r4, r5 // pFirst
    mr r5, r6 // pSecond

    bl 0f
0:
    mflr r12    // move link register into r12
    addis r12, r12, ha16(L_g_dwTPStubAddr_non_lazy_ptr-0b)
    lwz r12, lo16(L_g_dwTPStubAddr_non_lazy_ptr-0b)(r12)
    lwz r12, 0(r12)

    // call the TP stub
    mtctr r12
    bctrl

    // epilog
    lwz sp, 0(sp)
    lwz r0, 8(sp)
    mtlr r0
    blr

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::CheckForContextMatch   public
//
//  Synopsis:   This code generates a check to see if the current context and
//              the context of the proxy match.
//+----------------------------------------------------------------------------
ASMFUNC CRemotingServices__CheckForContextMatch
    // kR0 = stub data
    // result will be in kR0
    // must preserve kR2..kR11

    // prolog
    mflr r12
    stw r12, 8(sp)
    stwu sp, -roundup(LINKAGE_AREA + 4*12, 16)(sp)

    stw r0, (LINKAGE_AREA + 0*4)(sp)

    stw r2, (LINKAGE_AREA + 2*4)(sp)
    
    ARGUMENT_REGISTERS_GP stw,(LINKAGE_AREA + 3*4),sp
    
    stw r11, (LINKAGE_AREA + 11*4)(sp)

    // GetThread won't hopefully destroy the floating point registers

    bl 0f
0:
    mflr r12    // move link register into r12
    addis r12, r12, ha16(L_GetThread_non_lazy_ptr-0b)
    lwz r12, lo16(L_GetThread_non_lazy_ptr-0b)(r12)
    lwz r12, 0(r12)

    // Get the current thread
    mtctr r12
    bctrl

    lwz r4, (LINKAGE_AREA + 0*4)(sp)    // Get the pointer to the context from the proxy

    lwz r3, Thread_m_Context(r3)        // Get the current context from the thread
    lwz r4, 4(r4)                       // Get the internal context id by unboxing the stub data

    sub r0, r3, r4

    lwz r2, (LINKAGE_AREA + 2*4)(sp)
    
    ARGUMENT_REGISTERS_GP lwz,(LINKAGE_AREA + 3*4),sp

    lwz r11, (LINKAGE_AREA + 11*4)(sp)

    // epilog
    lwz sp, 0(sp)
    lwz r12, 8(sp)
    mtlr r12
    blr

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::DispatchInterfaceCall   public
//
//+----------------------------------------------------------------------------
ASMFUNC CRemotingServices__DispatchInterfaceCall
    // NOTE: At this point r11 contains the MethodDesc* - METHOD_CALL_PRESTUB_SIZE_ASM
    
    mflr r12
    stw r12, 8(sp)
    stwu sp, -roundup(LINKAGE_AREA, 16)(sp)

    // get the this register: look at the callsite whether it contains
    // the magic instruction to determine the location of the this pointer

    lwz r0, 0(r12)
    
    // r12 = oris r0, r0, 0
    addis r12, 0, 0x6400
    cmpw r12, r0

    // r2 = this reg
    mr r2, r3
    bne+ 0f
    mr r2, r4
0:

    // Move into kR0 the stub data and call the stub
    lwz r12, TP_OFFSET_STUB(r2)
    lwz r0, TP_OFFSET_STUBDATA(r2)

    mtctr r12
    bctrl

    cmpwi r0,0

    // restore the stack frame
    lwz sp, 0(sp)
    lwz r0, 8(sp)

    bne LCtxMismatch

    // finish restoring the stack frame
    mtlr r0

    // in current context, so resolve MethodDesc to real slot no

    lbz r12, (METHOD_CALL_PRESTUB_SIZE_ASM + MD_IndexOffset_ASM)(r11) // get MethodTable from pMD
#if MethodDesc__ALIGNMENT != 8
#error This code assumes MethodDesc__ALIGNMENT == 8
#endif
    slwi r12, r12, 3
    add r12, r12, r11
    lwz r12, MD_SkewOffset_ASM(r12)
    lwz r2, TP_OFFSET_MT(r2)                    // get the *real* MethodTable
    lwz r12, MethodTable_m_pEEClass(r12)        // get EEClass from pMT
    lwz r2, MethodTable_m_pInterfaceVTableMap(r2) // get interface map    
    lwz r12, EEClass_m_dwInterfaceId(r12)       // get the interface id from the EEClass
    slwi r12, r12, 2                            // offset map by interface id
    add r2, r2, r12
    lhz r12, (METHOD_CALL_PRESTUB_SIZE_ASM + MethodDesc_m_wSlotNumber)(r11)
    slwi r12, r12, 2
    lwzx r12, r12, r2                           // get jump addr

    mtctr r12
    bctr

LCtxMismatch:                                   // Jump to TPStub
    // jump to OOContext label in TPStub
    bl 0f
0:
    mflr r12    // move link register into r12
    addis r12, r12, ha16(L_g_dwOOContextAddr_non_lazy_ptr-0b)
    lwz r12, lo16(L_g_dwOOContextAddr_non_lazy_ptr-0b)(r12)
    lwz r12, 0(r12)

    // finish restoring the stack frame
    mtlr r0

    mtctr r12
    bctr

// LPVOID __stdcall CTPMethodTable__CallTargetHelper2(
//     const void *pTarget,
//     LPVOID pvFirst,
//     LPVOID pvSecond)	
ASMFUNC CTPMethodTable__CallTargetHelper2
    // prolog
    mflr r0
    stw r0, 8(sp)
    stwu sp, -roundup(LINKAGE_AREA + 8, 16)(sp)
    
    mr r12, r3
    mr r3, r4
    mr r4, r5
    
    // make the call
    mtctr r12
    bctrl
    
    // epilog
    lwz sp, 0(sp)
    lwz r0, 8(sp)
    mtlr r0
    blr

// LPVOID __stdcall CTPMethodTable__CallTargetHelper3(
//     const void *pTarget,
//     LPVOID pvFirst,
//     LPVOID pvSecond,
//     LPVOID pvThird
ASMFUNC CTPMethodTable__CallTargetHelper3
    // prolog
    mflr r0
    stw r0, 8(sp)
    stwu sp, -roundup(LINKAGE_AREA + 12, 16)(sp)
    
    mr r12, r3
    mr r3, r4
    mr r4, r5
    mr r5, r6
    
    // make the call
    mtctr r12
    bctrl
    
    // epilog
    lwz sp, 0(sp)
    lwz r0, 8(sp)
    mtlr r0
    blr

//+----------------------------------------------------------------------------
//
//  Method:     CTPMethodTable::GenericCheckForContextMatch private
//
//  Synopsis:   Calls the stub in the TP & returns TRUE if the contexts
//              match, FALSE otherwise.
//
//  Note:       1. Called during FieldSet/Get, used for proxy extensibility
//+----------------------------------------------------------------------------
// BOOL __stdcall CTPMethodTable__GenericCheckForContextMatch(Object* orTP)
ASMFUNC CTPMethodTable__GenericCheckForContextMatch

    // prolog
    mflr r0
    stw r0, 8(sp)
    stwu sp, -roundup(LINKAGE_AREA, 16)(sp)

    lwz r12, TP_OFFSET_STUB(a0)
    lwz r0, TP_OFFSET_STUBDATA(a0)

    mtctr r12
    bctrl

    // NOTE: In the CheckForXXXMatch stubs (for URT ctx/ Ole32 ctx) eax is
    // non-zero if contexts *do not* match & zero if they do.
    subfic r3,r0,0
    adde r3,r3,r0

    // epilog
    lwz sp, 0(sp)
    lwz r0, 8(sp)
    mtlr r0
    blr

// void __stdcall getFPReturn(int fpSize, INT64 *pretVal)
//
// Retrieve a floating-point return value from the FPU, after a
// call to a function whose return type is float or double.  Note
// that the fpSize may not be 4 or 8, meaning that the floating-
// point state and the contents of pretVal must not be modified.
//
ASMFUNC getFPReturn
    cmpwi   a0, 4
    beq-    LGetFPReturn4
    cmpwi   a0, 8
    bne+    LGetFPReturnNot8
    stfd    f1, 0(a1)
LGetFPReturnNot8:
    blr
LGetFPReturn4:
    stfs    f1, 0(a1)
    blr

// void __stdcall setFPReturn(int fpSize, INT64 retVal)
//
// Load a floating-point return value into the FPU's top-of-stack,
// in preparation for a return from a function whose return type
// is float or double.  Note that fpSize may not be 4 or 8, meaning
// that the floating-point state must not be modified.
//
ASMFUNC setFPReturn
    stw     a1, (LINKAGE_AREA+1*4)(sp)
    stw     a2, (LINKAGE_AREA+2*4)(sp)   
    cmpwi   a0, 4
    beq-    LSetFPReturn4
    cmpwi   a0, 8
    bne+    LSetFPReturnNot8
    lfd     f1, (LINKAGE_AREA+1*4)(sp)
LSetFPReturnNot8:
    blr
LSetFPReturn4:
    lfs     f1, (LINKAGE_AREA+1*4)(sp)
    blr

// LPVOID __stdcall GetIP(void);
ASMFUNC GetIP
    mflr a0
    blr

// LPVOID __stdcall GetSP(void);
ASMFUNC GetSP
    mr a0, sp
    blr
