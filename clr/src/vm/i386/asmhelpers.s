// ==++==
// 
//  
//   Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
//  
//   The use and distribution terms for this software are contained in the file
//   named license.txt, which can be found in the root of this distribution.
//   By using this software in any fashion, you are agreeing to be bound by the
//   terms of this license.
//  
//   You must not remove this notice, or any other, from this software.
//  
// 
// ==--== 
//
//  *** NOTE:  If you make changes to this file, propagate the changes to
//             asmhelpers.asm in this directory                            

	.intel_syntax
	.arch i586

#include "asmconstants.h"



    .extern iSpinLock
    .extern g_dwOOContextAddr
    .extern g_dwTPStubAddr

// a handy macro for declaring a function
#define ASMFUNC(n)	          \
	.global n		; \
	.func n			; \
n:				;

#ifdef _DEBUG

// ensure that a register contains a 4-byte-aligned pointer.  If it doesn't,
// then execute a hard-coded int3 to report the assertion failure.
#define _ASSERT_ALIGNED_4_X86(reg) \
	test %reg, 3		; \
	jz .+3		        ; \
	int3			; 

// ensure that a register contains an 8-byte-aligned pointer.  If it doesn't,
// then execute a hard-coded int3 to report the assertion failure.
#define _ASSERT_ALIGNED_8_X86(reg) \
	test %reg, 7		; \
	jz .+3			; \
	int3			; 

#else
#define _ASSERT_ALIGNED_4_X86(reg)
#define _ASSERT_ALIGNED_8_X86(reg)
#endif
	

// Uniproc spinlocks
#define _UP_SPINLOCK_ENTER(X, n)  \
	push %ebx		; \
	mov  %ebx, 1		; \
lspin##n:			; \
	xor %eax, %eax		; \
	cmpxchg [X], %ebx	; \
	jnz lspin##n		;

#define _UP_SPINLOCK_EXIT(X)      \
	mov dword ptr [X], 0	; \
	pop %ebx		;

// Multi-proc spinlocks
#define _MP_SPINLOCK_ENTER(X, n)  \
	push %ebx		; \
	mov  %ebx, 1		; \
lspin##n:			; \
	xor %eax, %eax		; \
  lock; cmpxchg [X], %ebx       ; \
	jnz lspin##n		;

#define _MP_SPINLOCK_EXIT(X)      \
	mov %ebx, -1		; \
  lock; xadd X, %ebx            ; \
	pop %ebx


// void [__cdecl|__stdcall] ResetCurrentContext(void);
//
// Restores the floating-point hardware to a known state, leaving
// precision and rounding unmodified.
//
.global ResetCurrentContext
.func ResetCurrentContext
ResetCurrentContext:	
	push %ebp
	mov  %ebp, %esp
	sub  %esp, 4
	#define ctrlWord word ptr [%ebp-4]
	
        // Clear the direction flag (used for rep instructions)
	cld

	fnstcw ctrlWord
        fninit			// reset FPU
        and ctrlWord, 0x0f00    // preserve precision and rounding control
        or  word ptr ctrlWord, 0x007f    // mask all exceptions
        fldcw ctrlWord          // preserve precision control

	leave
	ret
	#undef ctrlWord
.endfunc

	
/*
; This is a helper that we use to raise the correct managed exception with
; the necessary frame (after taking a fault in jitted code).
;
; Inputs:
;      all registers still have the value
;      they had at the time of the fault, except
;              EIP points to this function
;              ECX contains the original EIP
;
; What it does:
;      The exception to be thrown is stored in m_pLastThrownObjectHandle.
;      We push a FaultingExcepitonFrame on the stack, and then we call
;      complus throw.
;
*/
.global NakedThrowHelper
.func NakedThrowHelper
NakedThrowHelper:
	// Erect a faulting Method Frame.  Layout is as follows ...	
        mov %edx, %esp
        push %ebp               // ebp
        push %ebx               // ebx
        push %esi               // esi
        push %edi               // edi
        push %edx               // original esp
        push %ecx               // m_ReturnAddress (i.e. original IP)
        sub  %esp,12            // m_Datum (trash)
                                // Next (filled in by LinkFrameAndThrow)
                                // FaultingExceptionFrame VFP (ditto)

        mov %ecx, %esp
        push %esp               // push cdecl argument
        call LinkFrameAndThrow
        pop  %eax               // clean up after cdecl call

	pop %ebp
	ret 4
.endfunc



// void ResumeAtJitEHHelper(CONTEXT *pContext)
//
// Restores the CPU registers from the CONTEXT record, including
// EIP.
//
.global ResumeAtJitEHHelper
.func ResumeAtJitEHHelper
ResumeAtJitEHHelper:
        mov     %edx, [%esp+4]	 // edx = pContext (EHContext*)

        mov     %eax, [%edx+EHContext_Eax]
        mov     %ebx, [%edx+EHContext_Ebx]
        mov     %esi, [%edx+EHContext_Esi]
        mov     %edi, [%edx+EHContext_Edi]
        mov     %ebp, [%edx+EHContext_Ebp]
        mov     %esp, [%edx+EHContext_Esp]

        jmp     dword ptr [%edx+EHContext_Eip]
.endfunc

// int __stdcall CallJitEHFilterHelper(size_t *pShadowSP, EHContext *pContext);
//   On entry, only the pContext->Esp, Ebx, Esi, Edi, Ebp, and Eip are 
//   initialized.
//
.global CallJitEHFilterHelper
.func CallJitEHFilterHelper
CallJitEHFilterHelper:
        push    %ebp
        mov     %ebp, %esp
        push    %ebx
        push    %esi
        push    %edi

	#define pShadowSP [%ebp+8]
	#define pContext  [%ebp+12]

        mov     %eax, pShadowSP      // Write esp-4 to the shadowSP slot
        test    %eax, %eax
        jz      DONE_SHADOWSP_FILTER
        mov     %ebx, %esp
        sub     %ebx, 4
        or      %ebx, SHADOW_SP_IN_FILTER_ASM
        mov     [%eax], %ebx
    DONE_SHADOWSP_FILTER:

        mov     %edx, [pContext]
        mov     %eax, [%edx+EHContext_Eax]
        mov     %ebx, [%edx+EHContext_Ebx]
        mov     %esi, [%edx+EHContext_Esi]
        mov     %edi, [%edx+EHContext_Edi]
        mov     %ebp, [%edx+EHContext_Ebp]

        call    dword ptr [%edx+EHContext_Eip]
#ifdef _DEBUG
        nop  // Indicate that it is OK to call managed code directly from here
#endif
        pop     %edi
        pop     %esi
        pop     %ebx
        pop     %ebp // don't use 'leave' here, as ebp as been trashed
        ret     8
	#undef pShadowSP
	#undef pContext
.endfunc


// void __stdcall CallJitEHFinallyHelper(size_t *pShadowSP, EHContext *pContext);
//   on entry, only the pContext->Esp, Ebx, Esi, Edi, Ebp, and Eip are initialized
.global CallJitEHFinallyHelper
.func CallJitEHFinallyHelper
CallJitEHFinallyHelper:
        push    %ebp
        mov     %ebp, %esp
        push    %ebx
        push    %esi
        push    %edi

	#define pShadowSP [%ebp+8]
	#define pContext  [%ebp+12]

        mov     %eax, pShadowSP      // Write esp-4 to the shadowSP slot
        test    %eax, %eax
        jz      DONE_SHADOWSP_FINALLY
        mov     %ebx, %esp
        sub     %ebx, 4
        mov     [%eax], %ebx
    DONE_SHADOWSP_FINALLY:

        mov     %edx, [pContext]
        mov     %eax, [%edx+EHContext_Eax]
        mov     %ebx, [%edx+EHContext_Ebx]
        mov     %esi, [%edx+EHContext_Esi]
        mov     %edi, [%edx+EHContext_Edi]
        mov     %ebp, [%edx+EHContext_Ebp]
        call    dword ptr [%edx+EHContext_Eip]
#ifdef _DEBUG
        nop  // Indicate that it is OK to call managed code directly from here
#endif
        pop     %edi
        pop     %esi
        pop     %ebx
        pop     %ebp // don't use 'leave' here, as ebp as been trashed
        ret     8
	#undef pShadowSP
	#undef pContext
.endfunc


// DWORD [__cdecl||__stdcall] GetCpecificCpuTypeAsm(void);
//
// Determines the type of x86 processor
// 
ASMFUNC(GetSpecificCpuTypeAsm)
	push    %ebx        // ebx is trashed by the cpuid calls

	// See if the chip supports CPUID
	pushfd
	pop	%ecx	    // Get the EFLAGS
	mov	%eax, %ecx  // Save for later testing
	xor	%ecx, 0x200000 // Invert the ID bit.
	push	%ecx
	popfd		    // Save the updated flags.
	pushfd
	pop     %ecx	    // Retrive the updated flags
	xor	%ecx, %eax  // Test if it actually changed (bit set means yes)
	push	%eax
	popfd		    // Restore the flags

	test    %ecx, 0x200000
	jz	Assume486

	xor	%eax, %eax
	cpuid

	test	%eax, %eax
	jz	Assume486   // brif CPUID1 not allowed

	mov	%eax, 1
	cpuid
	shr	%eax, 8
	and	%eax, 0x0f  // filter out family

        shl     %eax, 16    // or in cpu features in upper 16 bits
	and     %edx, 0xffff0000
        or      %eax, %edx
	jmp	CpuTypeDone

Assume486:
	mov     %eax, 4	    // report 486
CpuTypeDone:
	pop	%ebx
	ret
.endfunc


// Atomic bit manipulations, with and without the lock prefix.  We initialize
// all consumers to go through the appropriate service at startup.

// void __stdcall OrMaskUP(DWORD volatile *p, const int msk)
ASMFUNC(OrMaskUP)
	mov	%ecx, [%esp+4]	// p
	mov	%edx, [%esp+8]	// msk
	_ASSERT_ALIGNED_4_X86(ecx)
	or	[%ecx], %edx
        ret	8
.endfunc

// void __stdcall AndMaskUP(DWORD volatile *p, const int msk)
ASMFUNC(AndMaskUP)
	mov	%ecx, [%esp+4]	// p
	mov	%edx, [%esp+8]	// msk
	_ASSERT_ALIGNED_4_X86(ecx)
	and	[%ecx], %edx
	ret	8
.endfunc

// LONG __stdcall ExchangeUP(LONG volatile *Target, LONG Value)
ASMFUNC(ExchangeUP)
	mov	%ecx, [%esp+4]	// Target
	mov	%edx, [%esp+8]	// Value
	mov	%eax, [%ecx]	// attempted comparant
	_ASSERT_ALIGNED_4_X86(ecx)
LRetryExchange:
	cmpxchg [%ecx], %edx
	jne	LRetryExchange1
	ret	8
LRetryExchange1:
	jmp	LRetryExchange
.endfunc

// LONG __stdcall ExchangeAddUP(LONG volatile *Target, LONG value)
ASMFUNC(ExchangeAddUP)
	mov	%ecx, [%esp+4]	// Target
	mov	%eax, [%esp+8]	// Value
	_ASSERT_ALIGNED_4_X86(ecx)
	xadd	[%ecx], %eax	// Add Value to Target
	ret	8		// result in EAX
.endfunc

// void *__stdcall CompareExchangeUP(PVOID volatile *Destination, 
//                                  PVOID Exchange, PVOID Comparand)
ASMFUNC(CompareExchangeUP)
	mov	%ecx, [%esp+4]	// Destintation
	mov	%edx, [%esp+8]	// Exchange
	mov	%eax, [%esp+12]	// Comparand
	_ASSERT_ALIGNED_4_X86(ecx)
	cmpxchg [%ecx], %edx
	ret	12		// result in EAX
.endfunc

// LONG __stdcall IncrementUP(LONG volatile *Target)
ASMFUNC(IncrementUP)
	mov	%ecx, [%esp+4]
	mov	%eax, 1
	_ASSERT_ALIGNED_4_X86(ecx)
	xadd	[%ecx], %eax
	inc	%eax		// return prior value, plus 1 we added
	ret	4
.endfunc

// UINT64 __stdcall IncrementLongUP(UINT64 volatile *Target)
ASMFUNC(IncrementLongUP)

	mov	%ecx, [%esp+4]	// Target 
	_ASSERT_ALIGNED_4_X86(ecx)
	
	_UP_SPINLOCK_ENTER(iSpinLock,IncrementLongUP)

	add	[%ecx], 1
	adc	[%ecx+4], 0
	mov	%eax, [%ecx]
	mov	%edx, [%ecx+4]

	_UP_SPINLOCK_EXIT(iSpinLock)

	ret	4
.endfunc

// LONG __stdcall DecrementUP(LONG volatile *Target)
ASMFUNC(DecrementUP)
	mov	%ecx, [%esp+4]
	_ASSERT_ALIGNED_4_X86(ecx)
	mov	%eax, -1
	xadd	[%ecx], %eax
	dec	%eax		// return prior value, less 1 we removed
	ret	4
.endfunc

// UINT64 __stdcall DecrementLongUP(UINT64 volatile *Target)
ASMFUNC(DecrementLongUP)

	mov	%ecx, [%esp+4]	// Target 
	_ASSERT_ALIGNED_4_X86(ecx)
	
	_UP_SPINLOCK_ENTER(iSpinLock, DecrementLongUP)

	sub	[%ecx], 1
	sbb	[%ecx+4], 0
	mov	%eax, [%ecx]
	mov	%edx, [%ecx+4]

	_UP_SPINLOCK_EXIT(iSpinLock)

	ret	4
.endfunc



// void __stdcall OrMaskMP(DWORD volatile *p, const int msk)
ASMFUNC(OrMaskMP)
	mov	%ecx, [%esp+4]	// p
	mov	%edx, [%esp+8]	// msk
	_ASSERT_ALIGNED_4_X86(ecx)
  lock; or [%ecx], %edx
	ret	8
.endfunc

// void __stdcall AndMaskMP(DWORD volatile *p, const int msk)
ASMFUNC(AndMaskMP)
	mov	%ecx, [%esp+4]	// p
	mov	%edx, [%esp+8]	// msk
	_ASSERT_ALIGNED_4_X86(ecx)
  lock; and [%ecx], %edx
	ret	8
.endfunc
	
// LONG __stdcall ExchangeMP(LONG volatile *Target, LONG Value)
ASMFUNC(ExchangeMP)
        mov     %ecx, [%esp+4]  // Target
        mov     %edx, [%esp+8]  // Value
        mov     %eax, [%ecx]    // attempted comparant
        _ASSERT_ALIGNED_4_X86(ecx)
LRetryExchangeMP:
  lock; cmpxchg [%ecx], %edx
        jne     LRetryExchangeMP1
        ret	8
LRetryExchangeMP1:
        jmp     LRetryExchangeMP
.endfunc

// void * __stdcall CompareExchangeMP(PVOID volatile *Destination,
//                                    PVOID Exchange, PVOID Comparand)
ASMFUNC(CompareExchangeMP)
        mov     %ecx, [%esp+4]  // Destintation
        mov     %edx, [%esp+8]  // Exchange
        mov     %eax, [%esp+12] // Comparand
        _ASSERT_ALIGNED_4_X86(ecx)
  lock; cmpxchg [%ecx], %edx
        ret     12              // result in EAX
.endfunc

// LONG __stdcall ExchangeAddMP(LONG volatile *Target, LONG value)
ASMFUNC(ExchangeAddMP)
        mov     %ecx, [%esp+4]  // Target
        mov     %eax, [%esp+8]  // Value
        _ASSERT_ALIGNED_4_X86(ecx)
  lock; xadd    [%ecx], %eax    // Add Value to Target
        ret     8               // result in EAX
.endfunc
	
// LONG __stdcall IncrementMP(LONG volatile *Target)
ASMFUNC(IncrementMP)
        mov     %ecx, [%esp+4]
        mov     %eax, 1
        _ASSERT_ALIGNED_4_X86(ecx)
  lock; xadd    [%ecx], %eax
        inc     %eax            // return prior value, plus 1 we added
        ret	4
.endfunc

// UINT64 __stdcall IncrementLongMP8b(UINT64 volatile *Target)
ASMFUNC(IncrementLongMP8b)
	push	%esi
	push	%ebx
	mov	%esi, [%esp+12]	// Target

	
	_ASSERT_ALIGNED_4_X86(esi)

	xor	%edx, %edx
	xor	%eax, %eax
	xor	%ecx, %ecx
	mov	%ebx, 1

  lock; cmpxchg8b [%esi]
	jz LDoneInc8b

LPreemptedInc8b:
	mov	%ecx, %edx
	mov	%ebx, %eax
	add	%ebx, 1
	adc	%ecx, 0

  lock; cmpxchg8b [%esi]
	jnz	LPreemptedInc8b

LDoneInc8b:
	mov	%edx, %ecx
	mov	%eax, %ebx

	pop	%ebx
	pop	%esi
	ret	4
.endfunc

// UINT64 __stdcall IncrementLongMP(UINT64 volatile *Target)
ASMFUNC(IncrementLongMP)
	mov	%ecx, [%esp+4]

	_ASSERT_ALIGNED_4_X86(ecx)

	_MP_SPINLOCK_ENTER(iSpinLock, IncrementLongMP)

	add	[%ecx], 1
	adc	[%ecx+4], 0
	mov	%eax, [%ecx]
	mov	%edx, [%ecx+4]

	_MP_SPINLOCK_EXIT(iSpinLock)

	ret	4
.endfunc
	
// LONG __stdcall DecrementMP(LONG volatile *Target)
ASMFUNC(DecrementMP)
        mov     %ecx, [%esp+4]
        _ASSERT_ALIGNED_4_X86(ecx)
        mov     %eax, -1
  lock; xadd    [%ecx], %eax
        dec     %eax            // return prior value, less 1 we removed
        ret	4
.endfunc

// UINT64 __stdcall DecrementLongMP8b(UINT64 volatile *Target)
ASMFUNC(DecrementLongMP8b)
	push	%esi
	push	%ebx
	
	mov	%esi, [%esp+12]	// Target

	_ASSERT_ALIGNED_4_X86(esi)

	xor	%ebx, %ebx
	xor	%eax, %eax
	mov	%ecx, 0xffffffff
	mov	%ebx, %ecx

  lock; cmpxchg8b [%esi]
	jz	LDoneDec8b

LPreemptedDec8b:
	mov	%ecx, %edx
	mov	%ebx, %eax
	sub	%ebx, 1
	sbb	%ecx, 0

  lock; cmpxchg8b [%esi]
	jnz	LPreemptedDec8b

LDoneDec8b:
	mov	%edx, %ecx
	mov	%eax, %ebx

	pop	%ebx
	pop	%esi
	ret	4
.endfunc

// UINT64 __stdcall DecrementLongMP(UINT64 volatile *Target)
ASMFUNC(DecrementLongMP)
	mov	%ecx, [%esp+4]
	_ASSERT_ALIGNED_4_X86(ecx)

	_MP_SPINLOCK_ENTER(iSpinLock, DecrementLongMP)

	sub	[%ecx], 1
	sbb	[%ecx+4], 0
	mov	%eax, [%ecx]
	mov	%edx, [%ecx+4]

	_MP_SPINLOCK_EXIT(iSpinLock)

	ret	4
.endfunc


#ifdef _DEBUG
//-------------------------------------------------------------------------
// This is a special purpose function used only by the stubs.
// IT TRASHES ESI SO IT CANNOT SAFELY BE CALLED FROM C.
//
// Whenever a DEBUG stub wants to call an external function,
// it should go through WrapCall. This is because VC's stack
// tracing expects return addresses to point to code sections.
//
// WrapCall uses ESI to keep track of the original return address.
// ESI is the register used to point to the current Frame by the stub
// prolog. It is not currently needed in the epilog, so we chose
// to sacrifice that one for WrapCall's use.
//-------------------------------------------------------------------------
// VOID __stdcall WrapCall(LPVOID pFunc)
ASMFUNC(WrapCall)
	pop	%esi		// pop off return address
	pop	%eax		// pop off function to call
	call    %eax		// call it
	push	%esi
	mov	%esi, 0xcccccccc
	ret
.endfunc

// void Frame::CheckExitFrameDebuggerCalls(void)
// NOTE: this may be called with cdecl, or stdcall
ASMFUNC(CheckExitFrameDebuggerCalls)
	call	PerformExitFrameChecks
	jmp	%eax
.endfunc
#endif



//-----------------------------------------------------------------------
// The out-of-line portion of the code to enable preemptive GC.
// After the work is done, the code jumps back to the "pRejoinPoint"
// which should be emitted right after the inline part is generated.
//
// Assumptions:
//      ebx = Thread
// Preserves
//      all registers except ecx.
//
//-----------------------------------------------------------------------
ASMFUNC(StubRareEnable)
	push	%eax
	push	%edx

	push	%ebx
	call	StubRareEnableWorker

	pop	%edx
	pop	%eax
	ret
.endfunc

ASMFUNC(StubRareDisableHR)
        push    %edx

	push    %esi	// Frame
	push    %ebx	// Thread
	call    StubRareDisableHRWorker

	pop	%edx
	ret
.endfunc

ASMFUNC(StubRareDisableTHROW)
	push	%eax
	push	%edx

	push	%esi	// Frame
	push	%ebx	// Thread
	call	StubRareDisableTHROWWorker

	pop	%edx
	pop	%eax
	ret
.endfunc

ASMFUNC(StubRareDisableRETURN)
	push	%eax
	push	%edx
	
	push	%esi	// Frame
	push	%ebx	// Thread
	call	StubRareDisableRETURNWorker

	pop	%edx
	pop	%eax
	ret
.endfunc

// EAX -> number of caller args bytes on the stack that we must remove before going
// to the throw helper, which assumes the stack is clean.
// ECX -> exception number to throw
ASMFUNC(InternalExceptionWorker)
	pop	%edx		// recover RETADDR
	add	%esp, %eax	// release caller's args
        push    %ecx            // push the exception number as the param to JIT_InternalThrow
	push	%edx		// restore RETADDR
        jmp     dword ptr [hlpFuncTable + CORINFO_HELP_INTERNALTHROW_ASM * VMHELPDEF_Size + VMHELPDEF_pfnHelper]
.endfunc

// EAX -> number of caller arg bytes on the stack that we must remove before going
// to the throw helper, which assumes the stack is clean.
ASMFUNC(ArrayOpStubNullException)
	add	%esp, ARRAYOPLOCSIZEFORPOP
	mov	%ecx, CORINFO_NullReferenceException_ASM
	jmp	InternalExceptionWorker
.endfunc

// EAX -> number of caller arg bytes on the stack that we must remove before going
// to the throw helper, which assumes the stack is clean.
ASMFUNC(ArrayOpStubRangeException)
	add	%esp, ARRAYOPLOCSIZEFORPOP
	mov	%ecx, CORINFO_IndexOutOfRangeException_ASM
	jmp	InternalExceptionWorker
.endfunc

// EAX -> number of caller arg bytes on the stack that we must remove before going
// to the throw helper, which assumes the stack is clean.
ASMFUNC(ArrayOpStubTypeMismatchException)
	add	%esp, ARRAYOPLOCSIZEFORPOP
	mov	%ecx, CORINFO_ArrayTypeMismatchException_ASM
	jmp	InternalExceptionWorker
.endfunc

// EAX -> number of caller arg bytes on the stack that we must remove before going
// to the throw helper, which assumes the stack is clean.
ASMFUNC(ThrowRankExceptionStub)
	pop	%edx		// throw away methoddesc
	mov	%ecx, CORINFO_RankException_ASM
	jmp	InternalExceptionWorker
.endfunc


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
ASMFUNC(CallDescrWorkerInternal)
#else
ASMFUNC(CallDescrWorker)
#endif
	push	%ebp
	mov	%ebp, %esp
	#define pSrcEnd		[%ebp+8]
	#define numStackSlots	[%ebp+12]
	#define pArgumentRegisters [%ebp+16]
	#define pTarget		[%ebp+20]

        mov     %eax, pSrcEnd		// copy the stack
	mov	%ecx, numStackSlots
	test	%ecx, %ecx
	jz	donestack
	sub	%eax, 4
	push	dword ptr [%eax]
	dec     %ecx
	jz	donestack
	sub	%eax, 4
	push	dword ptr [%eax]
	dec	%ecx
	jz	donestack
stackloop:
	sub	%eax, 4
	push	dword ptr [%eax]
	dec	%ecx
	jnz	stackloop
donestack:

	// The first two argument were passed in pSrcEnd array and not
	// in pArgumentRegisters, so they were pushed onto the stack by 
	// the earlier loop

	call	pTarget
#ifdef _DEBUG
        nop	// This is a tag that we use in an assert.  Fcalls expect to
		// be called from Jitted code or from certain blessed call sites like
		// this one.  (See HelperMethodFrame::InsureInit)
#endif
	leave
	ret     16
        #undef pSrcEnd
        #undef numStackSlots
        #undef pArgumentRegisters
        #undef pTarget
.endfunc


ASMFUNC(HelperMethodFrameRestoreState)
#ifdef _DEBUG
// int __fastcall HelperMethodFrameRestoreState(HelperMethodFrame*, struct MachState *)
    mov		%eax, [%esp+8]	// MachState*
#else
// int __fastcall HelperMethodFrameRestoreState(struct MachState *)
    mov         %eax, [%esp+4]  // MachState*
#endif
    // restore the registers from the m_MachState stucture.  Note that
    // we only do this for register that where not saved on the stack
    // at the time the machine state snapshot was taken.

    cmp	        [%eax+MachState__pRetAddr], 0

#ifdef _DEBUG
    jnz		noConfirm
    mov		%ecx, [%esp+4]	// HelperMethodFrame*
    push	%ebp
    push	%ebx
    push	%edi
    push	%esi
    push	%ecx	// HelperFrame*
    call	HelperMethodFrameConfirmState
    // on return, eax = MachState*
    cmp	        [%eax+MachState__pRetAddr], 0
noConfirm:
#endif

    jz		doRet

    lea		%edx, [%eax+MachState__esi]	// Did we have to spill ESI
    cmp		[%eax+MachState__pEsi], %edx
    jnz		SkipESI
    mov	        %esi, [%edx]			// Then restore it
SkipESI:

    lea		%edx, [%eax+MachState__edi]	// Did we have to spill EDI
    cmp		[%eax+MachState__pEdi], %edx
    jnz		SkipEDI
    mov	        %edi, [%edx]			// Then restore it
SkipEDI:

    lea		%edx, [%eax+MachState__ebx]	// Did we have to spill EBX
    cmp		[%eax+MachState__pEbx], %edx
    jnz		SkipEBX
    mov	        %ebx, [%edx]			// Then restore it
SkipEBX:

    lea		%edx, [%eax+MachState__ebp]	// Did we have to spill EBP
    cmp		[%eax+MachState__pEbp], %edx
    jnz		SkipEBP
    mov	        %ebp, [%edx]			// Then restore it
SkipEBP:

doRet:
    xor         %eax, %eax
#ifdef _DEBUG
    ret         8
#else
    ret         4
#endif
.endfunc


//  void __stdcall InterlockedCompareExchange8b(UINT64 *pLocation,
//					        UINT64 *pValue,
//					        UINT64 *pComparand);
//
ASMFUNC(InterlockedCompareExchange8b)
    push    %ebp
    mov	    %ebp, %esp
    push    %esi
    push    %ebx
	
    #define pLocation  [%ebp+8]
    #define pValue     [%ebp+12]
    #define pComparand [%ebp+16]

    mov    %edx, pComparand
    mov    %eax, [%edx]
    mov	   %edx, [%edx+4]

    mov    %ecx, pValue
    mov	   %ebx, [%ecx]
    mov	   %ecx, [%ecx+4]

    mov	   %esi, pLocation
lock; cmpxchg8b [%esi]

    pop    %ebx
    pop    %esi
    leave
    ret    12
    #undef pLocation
    #undef pValue
    #undef pComparand
.endfunc


//---------------------------------------------------------------------------
// Portable GetThread() function: used if no platform-specific optimizations apply.
// This is in assembly code because we count on edx not getting trashed on calls
// to this function.
//---------------------------------------------------------------------------
// Thread* __stdcall GetThreadGeneric(void);
ASMFUNC(GetThreadGeneric)
    push	%ebp
    mov		%ebp, %esp
    push	%ecx
    push	%edx
    push	%esi

    mov		%eax, dword ptr [gThreadTLSIndex]

#ifdef _DEBUG
    // _ASSERTE(ThreadInited());
    cmp		%eax, -1
    jnz		ThreadInited
    int3    
ThreadInited:
#endif

    push	%eax
    call	TlsGetValue

    pop		%esi
    pop		%edx
    pop		%ecx
    leave
    ret
.endfunc

//---------------------------------------------------------------------------
// Portable GetAppDomain() function: used if no platform-specific optimizations apply.
// This is in assembly code because we count on edx not getting trashed on calls
// to this function.
//---------------------------------------------------------------------------
// Appdomain* __stdcall GetAppDomainGeneric(void);
ASMFUNC(GetAppDomainGeneric)
    push	%ebp
    mov		%ebp, %esp
    push	%ecx
    push	%edx
    push	%esi

#ifdef _DEBUG
    // _ASSERTE(ThreadInited());
    mov		%eax, dword ptr [gAppDomainTLSIndex]
    cmp		%eax, -1
    jnz		ThreadInited2
    int3    
ThreadInited2:
#endif

    push	dword ptr [gAppDomainTLSIndex]
    call	TlsGetValue

    pop		%esi
    pop		%edx
    pop		%ecx
    leave
    ret
.endfunc


// VOID __stdcall OnStubObjectTripThread(VOID)
// On entry, eax = an OBJECTREF to pass to OnStubObjectWorker
ASMFUNC(OnStubObjectTripThread)
    push	%eax		    // pass the OBJECTREF
    call	OnStubObjectWorker
    add		%esp, 4	            // __cdecl cleanup
    ret
.endfunc

// VOID __stdcall OnStubObjectTripThread(VOID)
// On entry, eax = an OBJECTREF to pass to OnStubInteriorPointerWorker
ASMFUNC(OnStubInteriorPointerTripThread)
    push	%eax		    // pass the OBJECTREF
    call	OnStubInteriorPointerWorker
    add		%esp, 4	            // __cdecl cleanup
    ret
.endfunc


// VOID __stdcall OnStubScalarTripThread(VOID)
// A stub is returning something other than an ObjectRef to its caller
//
// On entry, eax:edx = scalar to preserve
ASMFUNC(OnStubScalarTripThread)
    push	%eax
    push	%edx

#ifdef _DEBUG
    call        OnStubScalarWorker
#else
    call        CommonTripThread
#endif

    pop         %edx
    pop		%eax
    ret
.endfunc



// Note that the debugger skips this entirely when doing SetIP,
// since COMPlusCheckForAbort should always return 0.  Excep.cpp:LeaveCatch
// asserts that to be true.  If this ends up doing more work, then the
// debugger may need additional support.
// void __stdcall JIT_EndCatch()//
ASMFUNC(JIT_EndCatch)
    xor	    %eax, %eax
    push    %eax		// pSEH
    push    %eax		// pCtx
    push    %eax		// pCurThread
    call    COMPlusEndCatch	// returns old esp value in eax

    mov     %ecx, [%esp]	// actual return address into jitted code
    mov     %edx, %eax		// old esp value
    push    %eax		// save old esp
    push    %ebp
    
    push    %ecx
    push    %edx
    call    COMPlusCheckForAbort  // __stdcall, returns old esp value

    pop     %ecx
    // at this point, ecx   = old esp value 
    //                [esp] = return address into jitted code
    //                eax   = 0 (if no abort), address to jump to otherwise
    test    %eax, %eax
    jz      NormalEndCatch
    mov     %esp, %ecx
    jmp     %eax

NormalEndCatch:
    pop     %eax                // Move the returnAddress into ecx
    mov     %esp, %ecx		// Reset esp to the old value
    jmp     %eax		// Resume after the "endcatch"
.endfunc



//---------------------------------------------------------
// Performs an N/Direct call. This is a generic version
// that can handly any N/Direct call but is not as fast
// as more specialized versions.
//---------------------------------------------------------
//
// INT64 __stdcall NDirectGenericStubWorker(Thread *pThread,
//                                          NDirectMethodFrame *pFrame);
//
ASMFUNC(NDirectGenericStubWorker)
    push    %ebp
    mov	    %ebp, %esp
#if defined(_DEBUG) || defined(CUSTOMER_CHECKED_BUILD)
    sub     %esp, 24
#else
    sub     %esp, 16
#endif

    // Parameters
    #define pThread [%ebp+8]
    #define pFrame  [%ebp+12]
    // Locals
    #define pHeader [%ebp-4]
    #define pvfn    [%ebp-8]
    #define pLocals [%ebp-12]
    #define pMLCode [%ebp-16]

#if defined(_DEBUG) || defined(CUSTOMER_CHECKED_BUILD)
    #define PreESP  [%ebp-20]
    #define PostESP [%ebp-24]
 #if NDirectGenericWorkerFrameSize_DEBUG != 40
  #error asmconstants.h is incorrect
 #endif
#else
 #if NDirectGenericWorkerFrameSize != 32
  #error asmconstants.h is incorrect
 #endif
#endif

    lea     %eax, pHeader
    push    %eax	 // &pHeader
    push    pFrame
    push    pThread
    call    NDirectGenericStubComputeFrameSize
    // eax = cbAlloc (rounded up to next DWORD)

    add     %eax, 3      // _alloca
    and     %eax, -4
    sub     %esp, %eax

    push    %esp	 // pAlloc
    lea     %eax, pvfn
    push    %eax         // &pvfn
    lea     %eax, pLocals
    push    %eax         // &pLocals
    lea     %eax, pMLCode
    push    %eax         // &pMLCode
    push    pHeader
    push    pFrame
    push    pThread
    call    NDirectGenericStubBuildArguments
    // pvfn is now the function to invoke, pLocals points to the locals, and
    // eax is zero or more MHLF_ constants.

    test    %eax, MLHF_THISCALL_ASM
    jz	    doit
    test    %eax, MLHF_THISCALLHIDDENARG_ASM
    jz	    regularthiscall

    // this call, with hidden arg
    pop	    %eax	// pop the first argument, shrinking the alloca space
    pop	    %ecx	// pop the second argument
    push    %eax
    jmp     doit

regularthiscall:
    pop     %ecx	// pop the first argument, shrinking the alloca space

doit:
#if defined(_DEBUG) || defined(CUSTOMER_CHECKED_BUILD)
    mov     [PreESP], %esp
#endif
    call    dword ptr [pvfn]

// this label is used by the debugger, and must immediately follow the 'call'.
.global NDirectGenericReturnFromCall
NDirectGenericReturnFromCall:

#if defined(_DEBUG) || defined(CUSTOMER_CHECKED_BUILD)
    mov     [PostESP], %esp

    push    PostESP
    push    PreESP
#endif
    push    %edx	        // push high dword of the return value
    push    %eax		// push low  dword of the return value
    push    pLocals
    push    pMLCode
    push    pHeader
    push    pFrame
    push    pThread
    call    NDirectGenericStubPostCall
    // return value is in edx:eax

    leave
    ret     8
    #undef  pThread
    #undef  pFrame
    #undef  pHeader
    #undef  pvfn
    #undef  pLocals
    #undef  pMLCode
    #undef  PreESP
    #undef  PostESP
.endfunc	
	


// LPVOID __stdcall CTPMethodTable__CallTargetHelper2(
//     const void *pTarget,
//     LPVOID pvFirst,
//     LPVOID pvSecond)	
ASMFUNC(CTPMethodTable__CallTargetHelper2)
    push    %ebp
    mov	    %ebp, %esp

    #define pTarget  [%ebp+8]
    #define pvFirst  [%ebp+12]
    #define pvSecond [%ebp+16]

    push    pvSecond
    push    pvFirst
    call    pTarget
#ifdef _DEBUG
    nop     // Mark this as a special call site that can directly 
            // call managed code
#endif
    mov     %esp, %ebp
    pop     %ebp
    ret     0xc
    #undef  pTarget
    #undef  pvFirst
    #undef  pvSecond
.endfunc

// LPVOID __stdcall CTPMethodTable__CallTargetHelper3(
//     const void *pTarget,
//     LPVOID pvFirst,
//     LPVOID pvSecond,
//     LPVOID pvThird
ASMFUNC(CTPMethodTable__CallTargetHelper3)
    push    %ebp
    mov     %ebp, %esp

    #define pTarget  [%ebp+8]
    #define pvFirst  [%ebp+12]
    #define pvSecond [%ebp+16]
    #define pvThird  [%ebp+20]

    push    pvThird
    push    pvSecond
    push    pvFirst
    call    pTarget
#ifdef _DEBUG
    nop     // Mark this as a special call site that can directly 
            // call managed code
#endif
    mov     %esp, %ebp
    pop     %ebp
    ret     0x10
    #undef  pTarget
    #undef  pvFirst
    #undef  pvSecond
    #undef  pvThird
.endfunc

// void __stdcall setFPReturn(int fpSize, INT64 retVal)
//
// Load a floating-point return value into the FPU's top-of-stack,
// in preparation for a return from a function whose return type
// is float or double.  Note that fpSize may not be 4 or 8, meaning
// that the floating-point state must not be modified.
//
ASMFUNC(setFPReturn)
    mov     %eax, [%esp+4]
    cmp     %eax, 4
    jz      setFPReturn4

    cmp     %eax, 8
    jnz     setFPReturnNot8
    fld     qword ptr [%esp+8]
setFPReturnNot8:
    ret     12

setFPReturn4:
    fld     dword ptr [%esp+8]
    ret     12
.endfunc

// void __stdcall getFPReturn(int fpSize, INT64 *pretVal)
//
// Retrieve a floating-point return value from the FPU, after a
// call to a function whose return type is float or double.  Note
// that the fpSize may not be 4 or 8, meaning that the floating-
// point state and the contents of pretVal must not be modified.
//
ASMFUNC(getFPReturn)
   mov      %ecx, [%esp+4]
   mov      %eax, [%esp+8]
   cmp      %ecx, 4
   jz       getFPReturn4

   cmp      %ecx, 8
   jnz      getFPReturnNot8
   fstp     qword ptr [%eax]
getFPReturnNot8:
   ret      8

getFPReturn4:
   fstp     dword ptr [%eax]
   ret      8
.endfunc

// void *__stdcall UM2MThunk_WrapperHelper(void *pThunkArgs, 
//                                         int argLen, 
//                                         void *pAddr, 
//                                         UMEntryThunk *pEntryThunk,
//                                         Thread *pThread)
//
ASMFUNC(UM2MThunk_WrapperHelper)
    push    %ebp
    mov     %ebp, %esp
    #define pThunkArgs [%ebp+8]
    #define argLen     [%ebp+12]
    #define pAddr      [%ebp+16]
    #define pEntryThunk [%ebp+20]
    #define pThread    [%ebp+24]
    mov     %ecx, [argLen]
    test    %ecx, %ecx
    jz      UM2MThunk_ArgsComplete // brif no args

    // copy the args to the stack, they will be released when we return from 
    // the thunk because we are coming from unmanaged to managed, any 
    // objectref args must have already been pinned so don't have to worry 
    // about them moving in the copy we make.
    _ASSERT_ALIGNED_4_X86(ecx)
    mov     %eax, [pThunkArgs]
UM2MThunk_CopyArg:
    push    [%eax+%ecx-4]        // push the argument on the stack
    sub     %ecx, 4              // move to the next argument
    jnz     UM2MThunk_CopyArg

UM2MThunk_ArgsComplete:
    mov     %eax, [pEntryThunk]
    mov     %ecx, [pThread]
    call    [pAddr]  

    // restore the stack after the call
    #undef  pThunkArgs
    #undef  argLen
    #undef  pAddr
    #undef  pEntryThunk
    #undef  pThread
    mov     %esp, %ebp
    pop     %ebp
    ret     20
.endfunc

// VOID __cdecl UMThunkStubRareDisable()
ASMFUNC(UMThunkStubRareDisable)
    push    %eax
    push    %ecx

    push    %esi	 // Push frame
    push    %eax         // Push the UMEntryThunk
    push    %ecx         // Push thread
    call    UMThunkStubRareDisableWorker // __stdcall

    pop     %ecx
    pop     %eax
    ret
.endfunc

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::CheckForContextMatch   public
//
//  Synopsis:   This code generates a check to see if the current context and
//              the context of the proxy match.
//+----------------------------------------------------------------------------
// void __stdcall CRemotingServices__CheckForContextMatch(void)
ASMFUNC(CRemotingServices__CheckForContextMatch)
    push    %ebx		 // spill ebx
    mov     %ebx, [%eax+4]       // Get the internal context id by unboxing
                                 // the stub data
    call    [GetThread]          // Get the current thread, assumes that the
                                 // registers are preserved
    mov     %eax, [%eax+Thread_m_Context] // Get the current context from the
                                 // thread
    sub     %eax, %ebx           // Get the pointer to the context from the
                                 // proxy and compare with the current context
    pop     %ebx                 // restore the value of ebx
    ret
.endfunc

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::DispatchInterfaceCall   public
//
//  Synopsis:
//              Push that method desc on the stack and jump to the
//              transparent proxy stub to execute the call.
//              WARNING!! This MethodDesc is not the methoddesc in the vtable
//              of the object instead it is the methoddesc in the vtable of
//              the interface class. Since we use the MethodDesc only to probe
//              the stack via the signature of the method call we are safe.
//              If we want to get any object vtable/class specific
//              information this is not safe.
//
//+----------------------------------------------------------------------------
// void __stdcall CRemotingServices__DispatchInterfaceCall(MethodDesc *pMD)
ASMFUNC(CRemotingServices__DispatchInterfaceCall)
    // NOTE: At this point the stack looks like
    //
    // esp--->  return addr of stub
    //          saved MethodDesc of Interface method
    //          return addr of calling function
    //
    mov     %eax, [%ecx + TP_OFFSET_STUBDATA]
    call    [%ecx + TP_OFFSET_STUB]
#ifdef _DEBUG
    nop     // Mark this as a special call site that can directly
            // call managed code
#endif
    test    %eax, %eax
    jnz     CtxMismatch

CtxMatch:
    // in current context, so resolve MethodDesc to real slot no
    push    %ebx                                 // spill ebx

    mov     %eax, [%esp + 8]                     // eax <-- MethodDesc
    movsx   %ebx, byte ptr [%eax + MD_IndexOffset_ASM] // get MethodTable from pMD
    mov     %eax, [%eax + %ebx*MethodDesc__ALIGNMENT + MD_SkewOffset_ASM]
    mov     %eax, [%eax]MethodTable_m_pEEClass   // get EEClass from pMT

    mov     %ebx, [%eax]EEClass_m_dwInterfaceId  // get the interface id from
                                                 // the EEClass
    mov     %eax, [%ecx + TP_OFFSET_MT]          // get the *real* MethodTable

    mov     %eax, [%eax]MethodTable_m_pInterfaceVTableMap  // get interface map
    mov     %eax, [%eax + %ebx*4]                // offset map by interface id
    mov     %ebx, [%esp + 8]                     // get MethodDesc
    mov     %bx,  [%ebx]MethodDesc_m_wSlotNumber
    and     %ebx, 0xffff
    mov     %eax, [%eax + %ebx*4]                // get jump addr

    pop     %ebx                                 // restore ebx

    add     %esp, 0x8                            // pop off Method desc and
                                                 // and stub's return addr
    jmp     %eax

CtxMismatch:                                     // Jump to TPStub

    add     %esp, 0x4                            // pop ret addr of stub, so that the 
                                                 // stack and registers are now
                                                 // setup exactly like they 
                                                 // were at the callsite

    jmp     [g_dwOOContextAddr]                  // jump to OOContext label in
                                                 // TPStub
.endfunc


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
ASMFUNC(CRemotingServices__CallFieldGetter)
    push    %ebp
    mov     %ebp, %esp
    #define pMD      [%ebp+8]
    #define pThis    [%ebp+12]
    #define pFirst   [%ebp+16]
    #define pSecond  [%ebp+20]
    #define pThird   [%ebp+24]

    mov     %ecx, [pThis]       // enregister pThis, the 'this' pointer
    mov     %edx, [pFirst]      // enregister pFirst, the first argument

    push    [pThird]            // push the third argument on the stack
    push    [pSecond]           // push the second argument on the stack

    lea     %eax, retAddrFieldGetter
    push    %eax

    push    [pMD]               // push the MethodDesc of object::__FieldGetter
    jmp     [g_dwTPStubAddr]    // jump to the TP stub

retAddrFieldGetter:
    #undef  pMD
    #undef  pThis
    #undef  pFirst
    #undef  pSecond
    #undef  pThird
    mov     %esp, %ebp
    pop     %ebp
    ret     0x14
.endfunc



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
ASMFUNC(CRemotingServices__CallFieldSetter)
    push    %ebp
    mov     %ebp, %esp
    #define pMD      [%ebp+8]
    #define pThis    [%ebp+12]
    #define pFirst   [%ebp+16]
    #define pSecond  [%ebp+20]
    #define pThird   [%ebp+24]

    mov     %ecx, [pThis]       // enregister pThis, the 'this' pointer
    mov     %edx, [pFirst]      // enregister the first argument

    push    [pThird]            // push the object (third arg) on the stack
    push    [pSecond]           // push the field name (second arg)
   
    lea     %eax, retAddrFieldSetter // push the return address
    push    %eax

    push    [pMD]              // push the MethodDesc of Object::__FieldSetter
    jmp     [g_dwTPStubAddr]   // jump to the TP Stub

retAddrFieldSetter:
    #undef  pMD
    #undef  pThis
    #undef  pFirst
    #undef  pSecond
    #undef  pThird
    mov     %esp, %ebp
    pop     %ebp
    ret     0x14
.endfunc


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
ASMFUNC(CTPMethodTable__GenericCheckForContextMatch)
    push    %ebp
    mov     %ebp, %esp
    #define tp    [%ebp+8]

    push    %ecx
    mov     %ecx, tp
    mov     %eax, [%ecx+TP_OFFSET_STUBDATA]
    call    [%ecx + TP_OFFSET_STUB]
#ifdef _DEBUG
    nop     // Mark this as a special call site that can directly
            // call managed code
#endif
    test    %eax, %eax
    mov     %eax, 0
    setz    %al
    // NOTE: In the CheckForXXXMatch stubs (for URT ctx/ Ole32 ctx) eax is
    // non-zero if contexts *do not* match & zero if they do.
    pop     %ecx
    #undef  tp
    mov     %esp, %ebp
    pop     %ebp
    ret     0x4
.endfunc


// void __stdcall JIT_ProfilerStub(UINT_PTR ProfilingHandle)
ASMFUNC(JIT_ProfilerStub)
    // this function must preserve all registers, including scratch
    ret     4
.endfunc

//
// Used to get the current instruction pointer value
//
// LPVOID __stdcall GetIP(void);
ASMFUNC(GetIP)
    mov     %eax, [%esp]
    ret
.endfunc

// LPVOID __stdcall GetSP(void);
ASMFUNC(GetSP)
    mov     %eax, %esp
    ret
.endfunc

	
// void __stdcall InprocEnterNaked(FunctionID functionId);
ASMFUNC(InprocEnterNaked)
    push    %ebp
    mov     %ebp, %esp
    push    dword ptr [%ebp+8]
    call    InprocEnter
    add     %esp, 4
    pop     %ebp
    ret     4
.endfunc

// void __stdcall InprocLeaveNaked(FunctionID functionId);
ASMFUNC(InprocLeaveNaked)
    push    %eax
    push    %ecx
    push    %edx
    push    dword ptr [%esp+16]
    call    InprocLeave
    add     %esp, 4
    pop     %edx
    pop     %ecx
    pop     %eax
    ret     4
.endfunc


// void __stdcall InprocTailcallNaked(FunctionID functionId);
ASMFUNC(InprocTailcallNaked)
    push    %eax
    push    %ecx
    push    %edx
    push    dword ptr [%esp+16]
    call    InprocTailcall
    add     %esp, 4
    pop     %edx
    pop     %ecx
    pop     %eax
    ret     4
.endfunc


//__declspec (naked) void CallThunk()
ASMFUNC(CallThunk)
    lea     %eax, [%esp+4]// get address of return address into the caller of the THUNK. 
                        // This well be used as the return address in the machine state

    mov     %ecx, [%esp+8]
    mov     %edx, [%esp+12]

		
    push    %ecx         // pass arg1    // Need to be in this order for ArgIterator

    push    %edx         // pass arg2    (save the value)

    // From here we are making a MachState structure.  
    push    %eax         // address of return address of whoever called the thunk
    push    0xCCCCCCCC  // The ESP after we return.  We dont know this
                        // since this is a shared thunk and we dont know
                        // how many arguments to pop.  Put an illegal value
                        // here to insure we don't use it 
    push    %ebp 
    push    %esp         // pEbp
    push    %ebx 
    push    %esp         // pEbx
    push    %esi 
    push    %esp         // pEsi
    push    %edi 
    push    %esp         // pEdi
  
    push    [%eax-4]	 // Make a copy of the caller's address to prevent it from being overwritten by the 
	                 // compiler optimizations
    lea     %eax, [%esp+44]// Pointer to the struct containing the enregisted arguments 
    push    %eax

    lea     %eax, [%esp+8] // Pointer to the MachineState structure 
    push    %eax
	  
    call    RejitCalleeMethod

    add     %esp, 12    // Pop the arguments to RejitCalleeMethod of the stack

    mov     %edi, [%esp+4]// restore regs
    mov     %esi, [%esp+12]
    mov     %ebx, [%esp+20]
    mov     %ebp, [%esp+28]
    add     %esp, 40     // Pop off sizeof(MachineState)

    pop     %edx         // restore arg2
    pop     %ecx         // restore arg1
    lea     %esp, [%esp+4]// pop thunk return address
    jmp     %eax
.endfunc

//__declspec (naked) void RejitThunk()
ASMFUNC(RejitThunk)
    mov     %ecx, %esp   // save pointer to return address for MachState. From a stack
                         // crawling point of view we have not really 'returned' from
                         // the method whose return cause this code to be invoked 

    add     %esp, -8     // store the floating point return value
    fstp    qword ptr [%esp]
    push    %edx         // pass upper bytes of return value (if it is a long)

    push    %eax         // pass return value (so we can protect it

    // From here we are making a MachState structure.  
    push    %ecx         // return address from a stack crawling point of view
    add     %ecx, 4
    push    %ecx         // The ESP after we return.  (we poped the return value)
    push    %ebp 
    push    %esp         // pEbp
    push    %ebx 
    push    %esp         // pEbx
    push    %esi 
    push    %esp         // pEsi
    push    %edi 
    push    %esp         // pEdi

    push    [%ecx-4]	 // Make a copy of the thunk address to prevent from being overwritten by the 
	                 // compiler optimizations
    lea     %eax, [%esp + 44] //  Pointer the saved return value
    push    %eax

    lea     %eax, [%esp+8] // Pointer to the MachState structure
    push    %eax
	
    call    RejitCallerMethod
    add     %esp, 12	  // erase the argument to the RejitCallerMethod
    mov     %ecx, %eax    // save location to return to

    mov     %edi, [%esp+4]// restore regs
    mov     %esi, [%esp+12]
    mov     %ebx, [%esp+20]
    mov     %ebp, [%esp+28]
    add     %esp, 40     // Pop off sizeof(MachineState)

    pop     %eax         // restore return value
    pop     %edx         // restore upper bytes of return value (if it is a long)
    fld     qword ptr [%esp] // restore the floating point return value
    add     %esp, 8     
    lea     %esp, [%esp+4]// pop thunk return address (now we have actually returned!)
    jmp     %ecx
.endfunc

	.end
