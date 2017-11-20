; ==++==
; 
;   
;    Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
;   
;    The use and distribution terms for this software are contained in the file
;    named license.txt, which can be found in the root of this distribution.
;    By using this software in any fashion, you are agreeing to be bound by the
;    terms of this license.
;   
;    You must not remove this notice, or any other, from this software.
;   
; 
; ==--==
;
;  *** NOTE:  If you make changes to this file, propagate the changes to
;             asmhelpers.s in this directory                            

	.586
	.model	flat

include asmconstants.inc

	option	casemap:none
	.code

ifdef _DEBUG
 DEBUG_OR_CUST_CHECKED equ 1
else
 ifdef CUSTOMER_CHECKED_BUILD
  DEBUG_OR_CUST_CHECKED equ 1
 else
  DEBUG_OR_CUST_CHECKED equ 0
 endif
endif

EXTERN @LinkFrameAndThrow@4:PROC	; this is fastcall, with one arg in ECX
ifdef _DEBUG
EXTERN _PerformExitFrameChecks@0:PROC
EXTERN _HelperMethodFrameConfirmState@20:PROC
endif
EXTERN _StubRareEnableWorker@4:PROC
EXTERN _StubRareDisableHRWorker@8:PROC
EXTERN _StubRareDisableTHROWWorker@8:PROC
EXTERN _StubRareDisableRETURNWorker@8:PROC
EXTERN __imp__TlsGetValue@4:DWORD
TlsGetValue PROTO stdcall
EXTERN _OnStubObjectWorker:PROC
EXTERN _OnStubInteriorPointerWorker:PROC
ifdef _DEBUG
EXTERN _OnStubScalarWorker:PROC
endif
EXTERN _CommonTripThread:PROC
EXTERN _COMPlusEndCatch@12:PROC
EXTERN @COMPlusCheckForAbort@12:PROC	; this is fastcall, with args in ecx/edx and one on the stack
EXTERN __alloca_probe:PROC
EXTERN _NDirectGenericStubComputeFrameSize@12:PROC
EXTERN _NDirectGenericStubBuildArguments@28:PROC
if DEBUG_OR_CUST_CHECKED
EXTERN _NDirectGenericStubPostCall@36:PROC
else
EXTERN _NDirectGenericStubPostCall@28:PROC
endif
EXTERN _UMThunkStubRareDisableWorker@12:PROC

EXTERN _iSpinLock:DWORD
EXTERN _hlpFuncTable:DWORD
EXTERN _gThreadTLSIndex:DWORD
EXTERN _gAppDomainTLSIndex:DWORD
EXTERN _g_dwOOContextAddr:DWORD
EXTERN _g_dwTPStubAddr:DWORD
EXTERN _GetThread:DWORD

EXTERN _InprocEnter:PROC
EXTERN _InprocLeave:PROC
EXTERN _InprocTailcall:PROC

EXTERN _RejitCalleeMethod:PROC
EXTERN _RejitCallerMethod:PROC
	
FASTCALL_FUNC macro FuncName,cbArgs
FuncNameReal EQU @&FuncName&@&cbArgs
FuncNameReal proc public
endm

FASTCALL_ENDFUNC macro
FuncNameReal endp
endm

;
; This macro verifies that the pointer held in reg is 4-byte aligned
; and breaks into the debugger if the condition is not met
;
ifdef _DEBUG
_ASSERT_ALIGNED_4_X86 macro reg
    test reg, 3
    jz @f
    int 3
@@:
endm

_ASSERT_ALIGNED_8_X86 macro reg
    test reg, 7
    jz @f
    int 3
@@:
endm

else

_ASSERT_ALIGNED_4_X86 macro reg
endm

_ASSERT_ALIGNED_8_X86 macro reg
endm

endif

; Uses eax, ebx registers
_UP_SPINLOCK_ENTER macro X
    push ebx
    mov  ebx, 1
@@:
    xor  eax, eax
    cmpxchg X, ebx
    jnz  @B
endm

_UP_SPINLOCK_EXIT macro X
    mov X, 0
    pop ebx
endm

_MP_SPINLOCK_ENTER macro X
    push ebx
    mov  ebx, 1
@@:
    xor  eax, eax
    lock cmpxchg X, ebx
    jnz  @B
endm

_MP_SPINLOCK_EXIT macro X
    mov  ebx, -1
    lock xadd X, ebx
    pop  ebx
endm



ResetCurrentContext PROC stdcall public
        LOCAL ctrlWord:WORD

        ; Clear the direction flag (used for rep instructions)
	cld

	fnstcw ctrlWord
        fninit			; reset FPU
        and ctrlWord, 0f00h     ; preserve precision and rounding control
        or  ctrlWord, 007fh     ; mask all exceptions
        fldcw ctrlWord          ; preserve precision control
        RET
ResetCurrentContext ENDP

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
NakedThrowHelper PROC stdcall public
	; Erect a faulting Method Frame.  Layout is as follows ...	
        mov edx, esp
        push ebp                ; ebp
        push ebx                ; ebx
        push esi                ; esi
        push edi                ; edi
        push edx                ; original esp
        push ecx                ; m_ReturnAddress (i.e. original IP)
        sub  esp,12             ; m_Datum (trash)
                                ; Next (filled in by LinkFrameAndThrow)
                                ; FaultingExceptionFrame VFP (ditto)

        mov ecx, esp
        call @LinkFrameAndThrow@4
        RET
NakedThrowHelper ENDP


_ResumeAtJitEHHelper@4 PROC public 
        mov     edx, [esp+4]	 ; edx = pContext (EHContext*)

        mov     eax, [edx+EHContext_Eax]
        mov     ebx, [edx+EHContext_Ebx]
        mov     esi, [edx+EHContext_Esi]
        mov     edi, [edx+EHContext_Edi]
        mov     ebp, [edx+EHContext_Ebp]
	mov     esp, [edx+EHContext_Esp]

        jmp     dword ptr [edx+EHContext_Eip]
_ResumeAtJitEHHelper@4 ENDP

; int __stdcall CallJitEHFilterHelper(size_t *pShadowSP, EHContext *pContext);
;   on entry, only the pContext->Esp, Ebx, Esi, Edi, Ebp, and Eip are initialized
_CallJitEHFilterHelper@8 PROC public
        push    ebp
        mov     ebp, esp
        push    ebx
        push    esi
        push    edi

        pShadowSP equ [ebp+8]
        pContext  equ [ebp+12]

        mov     eax, pShadowSP      ; Write esp-4 to the shadowSP slot
        test    eax, eax
        jz      DONE_SHADOWSP_FILTER
        mov     ebx, esp
        sub     ebx, 4
        or      ebx, SHADOW_SP_IN_FILTER_ASM
        mov     [eax], ebx
    DONE_SHADOWSP_FILTER:

        mov     edx, [pContext]
        mov     eax, [edx+EHContext_Eax]
        mov     ebx, [edx+EHContext_Ebx]
        mov     esi, [edx+EHContext_Esi]
        mov     edi, [edx+EHContext_Edi]
        mov     ebp, [edx+EHContext_Ebp]

        call    dword ptr [edx+EHContext_Eip]
ifdef _DEBUG
        nop  ; Indicate that it is OK to call managed code directly from here
endif
        pop     edi
        pop     esi
        pop     ebx
        pop     ebp ; don't use 'leave' here, as ebp as been trashed
        retn    8        
_CallJitEHFilterHelper@8 ENDP


; void __stdcall CallJITEHFinallyHelper(size_t *pShadowSP, EHContext *pContext);
;   on entry, only the pContext->Esp, Ebx, Esi, Edi, Ebp, and Eip are initialized
_CallJitEHFinallyHelper@8 PROC public
        push    ebp
        mov     ebp, esp
        push    ebx
        push    esi
        push    edi

        pShadowSP equ [ebp+8]
        pContext  equ [ebp+12]

        mov     eax, pShadowSP      ; Write esp-4 to the shadowSP slot
        test    eax, eax
        jz      DONE_SHADOWSP_FINALLY
        mov     ebx, esp
        sub     ebx, 4
        mov     [eax], ebx
    DONE_SHADOWSP_FINALLY:

        mov     edx, [pContext]
        mov     eax, [edx+EHContext_Eax]
        mov     ebx, [edx+EHContext_Ebx]
        mov     esi, [edx+EHContext_Esi]
        mov     edi, [edx+EHContext_Edi]
        mov     ebp, [edx+EHContext_Ebp]
        call    dword ptr [edx+EHContext_Eip]
ifdef _DEBUG
        nop  ; Indicate that it is OK to call managed code directly from here
endif
        pop     edi
        pop     esi
        pop     ebx
        pop     ebp ; don't use 'leave' here, as ebp as been trashed
        retn    8        
_CallJitEHFinallyHelper@8 ENDP


GetSpecificCpuTypeAsm PROC stdcall public
	push    ebx         ; ebx is trashed by the cpuid calls

	; See if the chip supports CPUID
	pushfd
	pop	ecx	    ; Get the EFLAGS
	mov	eax, ecx    ; Save for later testing
	xor	ecx, 200000h ; Invert the ID bit.
	push	ecx
	popfd		    ; Save the updated flags.
	pushfd
	pop     ecx	    ; Retrive the updated flags
	xor	ecx, eax    ; Test if it actually changed (bit set means yes)
	push	eax
	popfd		    ; Restore the flags

	test    ecx, 200000h
	jz	Assume486

	xor	eax, eax
	cpuid

	test	eax, eax
	jz	Assume486   ; brif CPUID1 not allowed

	mov	eax, 1
	cpuid
	shr	eax, 8
	and	eax, 0fh    ; filter out family

        shl     edx, 16	    ; or in cpu features in upper 16 bits
        or      eax, edx
	jmp	CpuTypeDone

Assume486:
	mov     eax, 4	    ; report 486
CpuTypeDone:
	pop	ebx
	retn
GetSpecificCpuTypeAsm ENDP
        
	


; Atomic bit manipulations, with and without the lock prefix.  We initialize
; all consumers to go through the appropriate service at startup.

FASTCALL_FUNC OrMaskUP,8
	_ASSERT_ALIGNED_4_X86 ecx
	or	dword ptr [ecx], edx
        ret
FASTCALL_ENDFUNC OrMaskUP

FASTCALL_FUNC AndMaskUP,8
	_ASSERT_ALIGNED_4_X86 ecx
	and	dword ptr [ecx], edx
        ret
FASTCALL_ENDFUNC AndMaskUP

FASTCALL_FUNC ExchangeUP,8
        _ASSERT_ALIGNED_4_X86 ecx
	mov	eax, [ecx]	; attempted comparand
retry:
	cmpxchg [ecx], edx
	jne	retry1		; predicted NOT taken
	retn
retry1:
	jmp	retry
FASTCALL_ENDFUNC ExchangeUP

FASTCALL_FUNC ExchangeAddUP,8
	_ASSERT_ALIGNED_4_X86 ecx
	xadd	[ecx], edx	; Add Value to Target
	mov	eax, edx
	retn
FASTCALL_ENDFUNC ExchangeAddUP

FASTCALL_FUNC CompareExchangeUP,12
	_ASSERT_ALIGNED_4_X86 ecx
	mov	eax, [esp+4]	; Comparand
	cmpxchg [ecx], edx
	retn	4		; result in EAX
FASTCALL_ENDFUNC CompareExchangeUP

FASTCALL_FUNC IncrementUP,4
	_ASSERT_ALIGNED_4_X86 ecx
	mov	eax, 1
	xadd	[ecx], eax
	inc	eax		; return prior value, plus 1 we added
	retn
FASTCALL_ENDFUNC IncrementUP

FASTCALL_FUNC IncrementLongUP,4
        _ASSERT_ALIGNED_4_X86 ecx

	_UP_SPINLOCK_ENTER _iSpinLock

	add	dword ptr [ecx], 1
	adc	dword ptr [ecx+4], 0
	mov	eax, [ecx]
	mov	edx, [ecx+4]

	_UP_SPINLOCK_EXIT _iSpinLock
	retn
FASTCALL_ENDFUNC IncrementLongUP

FASTCALL_FUNC DecrementUP,4
        _ASSERT_ALIGNED_4_X86 ecx
	mov	eax, -1
	xadd	[ecx], eax
	dec	eax		; return prior value, less 1 we removed
	retn
FASTCALL_ENDFUNC DecrementUP

FASTCALL_FUNC DecrementLongUP,4
        _ASSERT_ALIGNED_4_X86 ecx

	_UP_SPINLOCK_ENTER _iSpinLock

	sub	dword ptr [ecx], 1
	sbb	dword ptr [ecx+4], 0
	mov	eax, [ecx]
	mov	edx, [ecx+4]

	_UP_SPINLOCK_EXIT _iSpinLock
	retn
FASTCALL_ENDFUNC DecrementLongUP

FASTCALL_FUNC OrMaskMP,8
        _ASSERT_ALIGNED_4_X86 ecx
  lock  or	dword ptr [ecx], edx
	retn
FASTCALL_ENDFUNC OrMaskMP

FASTCALL_FUNC AndMaskMP,8
        _ASSERT_ALIGNED_4_X86 ecx
  lock  and	dword ptr [ecx], edx
	retn
FASTCALL_ENDFUNC AndMaskMP

FASTCALL_FUNC ExchangeMP,8
	_ASSERT_ALIGNED_4_X86 ecx
	mov	eax, [ecx]	; attempted comparand
retryMP:
  lock  cmpxchg	[ecx], edx
	jne	retry1MP	; predicted NOT taken
	retn
retry1MP:
	jmp	retryMP
FASTCALL_ENDFUNC ExchangeMP

FASTCALL_FUNC CompareExchangeMP,12
        _ASSERT_ALIGNED_4_X86 ecx
	mov	eax, [esp+4]	; Comparand
  lock  cmpxchg [ecx], edx
	retn	4		; result in EAX
FASTCALL_ENDFUNC CompareExchangeMP

FASTCALL_FUNC ExchangeAddMP,8
        _ASSERT_ALIGNED_4_X86 ecx
  lock  xadd	[ecx], edx	; Add Value to Target
	mov	eax, edx	; move result
	retn
FASTCALL_ENDFUNC ExchangeAddMP

FASTCALL_FUNC IncrementMP,4
	_ASSERT_ALIGNED_4_X86 ecx
	mov	eax, 1
  lock  xadd	[ecx], eax
	inc	eax		; return prior value, plus 1 we added
	retn
FASTCALL_ENDFUNC IncrementMP

FASTCALL_FUNC IncrementLongMP8b,4

	_ASSERT_ALIGNED_4_X86 ecx 

	push	esi
	push	ebx
	mov	esi, ecx

	xor	edx, edx
	xor	eax, eax
	xor	ecx, ecx
	mov	ebx, 1

  lock  cmpxchg8b qword ptr [esi]
	jz	done

preempted:
	mov	ecx, edx
	mov	ebx, eax
	add	ebx, 1
	adc	ecx, 0

  lock  cmpxchg8b qword ptr [esi]
	jnz	preempted

done:
	mov	edx, ecx
	mov	eax, ebx

	pop	ebx
	pop	esi
	retn
FASTCALL_ENDFUNC IncrementLongMP8b

FASTCALL_FUNC IncrementLongMP,4
        _ASSERT_ALIGNED_4_X86 ecx

        _MP_SPINLOCK_ENTER _iSpinLock

        add	dword ptr [ecx], 1
	adc	dword ptr [ecx+4], 0
	mov	eax, [ecx]
	mov	edx, [ecx+4]

	_MP_SPINLOCK_EXIT _iSpinLock
	retn
FASTCALL_ENDFUNC IncrementLongMP

FASTCALL_FUNC DecrementMP,4
	_ASSERT_ALIGNED_4_X86 ecx
	mov	eax, -1
  lock  xadd	[ecx], eax
	dec	eax		; return priory value, less 1 we removed
	retn
FASTCALL_ENDFUNC DecrementMP

FASTCALL_FUNC DecrementLongMP8b,4

	_ASSERT_ALIGNED_4_X86 ecx

	push	esi
	push	ebx
	mov	esi, ecx

	xor	edx, edx
	xor	eax, eax
	mov	ecx, 0ffffffffh
	mov	ebx, 0ffffffffh

  lock  cmpxchg8b qword ptr [esi]
	jz	donedec

preempteddec:
	mov	ecx, edx
	mov	ebx, eax
	sub	ebx, 1
	sbb	ecx, 0

  lock  cmpxchg8b qword ptr [esi]
	jnz	preempteddec

donedec:
	mov	edx, ecx
	mov	eax, ebx

	pop	ebx
	pop	esi
	retn
FASTCALL_ENDFUNC IncrementLongMP8b

FASTCALL_FUNC DecrementLongMP,4
        _ASSERT_ALIGNED_4_X86 ecx

        _MP_SPINLOCK_ENTER _iSpinLock

        sub	dword ptr [ecx], 1
	sbb	dword ptr [ecx+4], 0
	mov	eax, [ecx]
	mov	edx, [ecx+4]

	_MP_SPINLOCK_EXIT _iSpinLock
	retn
FASTCALL_ENDFUNC DecrementLongMP

ifdef _DEBUG
;-------------------------------------------------------------------------
; This is a special purpose function used only by the stubs.
; IT TRASHES ESI SO IT CANNOT SAFELY BE CALLED FROM C.
;
; Whenever a DEBUG stub wants to call an external function,
; it should go through WrapCall. This is because VC's stack
; tracing expects return addresses to point to code sections.
;
; WrapCall uses ESI to keep track of the original return address.
; ESI is the register used to point to the current Frame by the stub
; prolog. It is not currently needed in the epilog, so we chose
; to sacrifice that one for WrapCall's use.
;-------------------------------------------------------------------------
; VOID WrapCall(LPVOID pFunc)
_WrapCall@4 PROC public
	 pop	      esi
	 pop	      eax
	 call	      eax
	 push	      esi
	 mov	      esi, 0cccccccch
	 retn
_WrapCall@4 ENDP


; void Frame::CheckExitFrameDebuggerCalls(void)
; NOTE: this may be called with cdecl, stdcall, or __thiscall, but not __fastcall
CheckExitFrameDebuggerCalls PROC stdcall public
	push    ecx	    ; preserve ecx, in case this is a __thiscall call
	call	_PerformExitFrameChecks@0
	pop	ecx
	jmp	eax
CheckExitFrameDebuggerCalls ENDP
endif


;-----------------------------------------------------------------------
; The out-of-line portion of the code to enable preemptive GC.
; After the work is done, the code jumps back to the "pRejoinPoint"
; which should be emitted right after the inline part is generated.
;
; Assumptions:
;      ebx = Thread
; Preserves
;      all registers except ecx.
;
;-----------------------------------------------------------------------
_StubRareEnable proc public
	push	eax
	push	edx

	push	ebx
	call	_StubRareEnableWorker@4

	pop	edx
	pop	eax
	retn
_StubRareEnable ENDP

_StubRareDisableHR proc public
        push    edx

	push    esi	; Frame
	push    ebx	; Thread
	call    _StubRareDisableHRWorker@8

	pop	edx
	retn
_StubRareDisableHR ENDP

_StubRareDisableTHROW proc public
	push	eax
	push	edx

	push	esi	; Frame
	push	ebx	; Thread
	call	_StubRareDisableTHROWWorker@8

	pop	edx
	pop	eax
	retn
_StubRareDisableTHROW endp

_StubRareDisableRETURN proc public
	push	eax
	push	edx
	
	push	esi	; Frame
	push	ebx	; Thread
	call	_StubRareDisableRETURNWorker@8

	pop	edx
	pop	eax
	retn
_StubRareDisableRETURN endp


InternalExceptionWorker proc public
	pop	edx		; recover RETADDR
	add	esp, eax	; release caller's args
        push    ecx		; push the exception number as the param to JIT_InternalThrow
	push	edx		; restore RETADDR
        jmp     dword ptr [_hlpFuncTable + CORINFO_HELP_INTERNALTHROW_ASM * VMHELPDEF_Size + VMHELPDEF_pfnHelper]
InternalExceptionWorker endp


; EAX -> number of caller arg bytes on the stack that we must remove before going
; to the throw helper, which assumes the stack is clean.
_ArrayOpStubNullException proc public
	add	esp, ARRAYOPLOCSIZEFORPOP
	mov	ecx, CORINFO_NullReferenceException_ASM
	jmp	InternalExceptionWorker
_ArrayOpStubNullException endp

; EAX -> number of caller arg bytes on the stack that we must remove before going
; to the throw helper, which assumes the stack is clean.
_ArrayOpStubRangeException proc public
	add	esp, ARRAYOPLOCSIZEFORPOP
	mov	ecx, CORINFO_IndexOutOfRangeException_ASM
	jmp	InternalExceptionWorker
_ArrayOpStubRangeException endp

; EAX -> number of caller arg bytes on the stack that we must remove before going
; to the throw helper, which assumes the stack is clean.
_ArrayOpStubTypeMismatchException proc public
	add	esp, ARRAYOPLOCSIZEFORPOP
	mov	ecx, CORINFO_ArrayTypeMismatchException_ASM
	jmp	InternalExceptionWorker
_ArrayOpStubTypeMismatchException endp

; EAX -> number of caller arg bytes on the stack that we must remove before going
; to the throw helper, which assumes the stack is clean.
_ThrowRankExceptionStub proc public
	pop	edx		; throw away methoddesc
	mov	ecx, CORINFO_RankException_ASM
	jmp	InternalExceptionWorker
_ThrowRankExceptionStub endp

;------------------------------------------------------------------------------
; This helper routine enregisters the appropriate arguments and makes the 
; actual call.
;------------------------------------------------------------------------------
;ARG_SLOT
;#ifdef _DEBUG
;      CallDescrWorkerInternal
;#else
;      CallDescrWorker
;#endif
;                     (LPVOID                   pSrcEnd,
;                      UINT32                   numStackSlots,
;                      const ArgumentRegisters *pArgumentRegisters,
;                      LPVOID                   pTarget
;                     )
ifdef _DEBUG
CallDescrWorkerInternal PROC stdcall public,
                         pSrcEnd: DWORD,
			 numStackSlots: DWORD,
			 pArgumentRegisters: DWORD,
			 pTarget: DWORD
else
CallDescrWorker         PROC stdcall public,
                         pSrcEnd: DWORD,
			 numStackSlots: DWORD,
			 pArgumentRegisters: DWORD,
			 pTarget: DWORD
endif

        mov     eax, pSrcEnd		; copy the stack
	mov	ecx, numStackSlots
	test	ecx, ecx
	jz	donestack
	sub	eax, 4
	push	dword ptr [eax]
	dec     ecx
	jz	donestack
	sub	eax, 4
	push	dword ptr [eax]
	dec	ecx
	jz	donestack
stackloop:
	sub	eax, 4
	push	dword ptr [eax]
	dec	ecx
	jnz	stackloop
donestack:

	; The first two argument were passed in pSrcEnd array and not
	; in pArgumentRegisters, so they were pushed onto the stack by 
	; the earlier loop
	call	pTarget
ifdef _DEBUG
        nop	; This is a tag that we use in an assert.  Fcalls expect to
		; be called from Jitted code or from certain blessed call sites like
		; this one.  (See HelperMethodFrame::InsureInit)
endif
	ret
ifdef _DEBUG
CallDescrWorkerInternal endp
else
CallDescrWorker endp
endif

ifdef _DEBUG
; int __fastcall HelperMethodFrameRestoreState(HelperMethodFrame*, struct MachState *)
FASTCALL_FUNC HelperMethodFrameRestoreState,8
    mov		eax, edx	; eax = MachState*
else
; int __fastcall HelperMethodFrameRestoreState(struct MachState *)
FASTCALL_FUNC HelperMethodFrameRestoreState,4
    mov		eax, ecx	; eax = MachState*
endif
    ; restore the registers from the m_MachState stucture.  Note that
    ; we only do this for register that where not saved on the stack
    ; at the time the machine state snapshot was taken.

    cmp	        [eax+MachState__pRetAddr], 0

ifdef _DEBUG
    jnz		noConfirm
    push	ebp
    push	ebx
    push	edi
    push	esi
    push	ecx	; HelperFrame*
    call	_HelperMethodFrameConfirmState@20
    ; on return, eax = MachState*
    cmp	        [eax+MachState__pRetAddr], 0
noConfirm:
endif

    jz		doRet

    lea		edx, [eax+MachState__esi]	; Did we have to spill ESI
    cmp		[eax+MachState__pEsi], edx
    jnz		SkipESI
    mov	        esi, [edx]			; Then restore it
SkipESI:

    lea		edx, [eax+MachState__edi]	; Did we have to spill EDI
    cmp		[eax+MachState__pEdi], edx
    jnz		SkipEDI
    mov	        edi, [edx]			; Then restore it
SkipEDI:

    lea		edx, [eax+MachState__ebx]	; Did we have to spill EBX
    cmp		[eax+MachState__pEbx], edx
    jnz		SkipEBX
    mov	        ebx, [edx]			; Then restore it
SkipEBX:

    lea		edx, [eax+MachState__ebp]	; Did we have to spill EBP
    cmp		[eax+MachState__pEbp], edx
    jnz		SkipEBP
    mov	        ebp, [edx]			; Then restore it
SkipEBP:

doRet:
    xor         eax, eax
    retn
FASTCALL_ENDFUNC HelperMethodFrameRestoreState


; void __stdcall InterlockedCompareExchange8b(UINT64 *pLocation,
;					      UINT64 *pValue,
;					      UINT64 *pComparand);
;
InterlockedCompareExchange8b PROC stdcall public USES ebx esi,
			     pLocation :DWORD,
			     pValue :DWORD,
			     pComparand :DWORD

    mov    edx, pComparand
    mov    eax, [edx]
    mov	   edx, [edx+4]

    mov    ecx, pValue
    mov	   ebx, [ecx]
    mov	   ecx, [ecx+4]

    mov	   esi, pLocation
    lock   cmpxchg8b qword ptr [esi]
    ret
InterlockedCompareExchange8b endp



;---------------------------------------------------------------------------
; Portable GetThread() function: used if no platform-specific optimizations apply.
; This is in assembly code because we count on edx not getting trashed on calls
; to this function.
;---------------------------------------------------------------------------
; Thread* __stdcall GetThreadGeneric(void);
GetThreadGeneric PROC stdcall public USES ecx edx esi

    mov		eax, dword ptr [_gThreadTLSIndex]

ifdef _DEBUG
    ; _ASSERTE(ThreadInited());
    cmp		eax, -1
    jnz		@F
    int         3    
@@:
endif

    push	eax
    call	dword ptr [__imp__TlsGetValue@4]
    ret
GetThreadGeneric ENDP

;---------------------------------------------------------------------------
; Portable GetAppdomain() function: used if no platform-specific optimizations apply.
; This is in assembly code because we count on edx not getting trashed on calls
; to this function.
;---------------------------------------------------------------------------
; Appdomain* __stdcall GetAppDomainGeneric(void);
GetAppDomainGeneric PROC stdcall public USES ecx edx esi,

ifdef _DEBUG
    ; _ASSERTE(ThreadInited());
    mov		eax, dword ptr [_gThreadTLSIndex]
    cmp		eax, -1
    jnz		@F
    int         3    
@@:
endif

    push	dword ptr [_gAppDomainTLSIndex]
    call	dword ptr [__imp__TlsGetValue@4]
    ret
GetAppDomainGeneric ENDP


; VOID __stdcall OnStubObjectTripThread(VOID)
; On entry, eax = an OBJECTREF to pass to OnStubObjectWorker
OnStubObjectTripThread PROC stdcall public
    push	eax		    ; pass the OBJECTREF
    call	_OnStubObjectWorker
    add		esp, 4	            ; __cdecl cleanup
    retn
OnStubObjectTripThread endp

; VOID __stdcall OnStubInteriorPointerTripThread(VOID)
; On entry, eax = an OBJECTREF to pass to OnStubInteriorPointerWorker
OnStubInteriorPointerTripThread PROC stdcall public
    push	eax		    ; pass the OBJECTREF
    call	_OnStubInteriorPointerWorker
    add		esp, 4	            ; __cdecl cleanup
    retn
OnStubInteriorPointerTripThread endp

; VOID __stdcall OnStubScalarTripThread(VOID)
; A stub is returning something other than an ObjectRef to its caller
;
; On entry, eax:edx = scalar to preserve
OnStubScalarTripThread PROC stdcall public uses eax edx
ifdef _DEBUG
    call        _OnStubScalarWorker
else
    call        _CommonTripThread
endif
    ret
OnStubScalarTripThread endp




; Note that the debugger skips this entirely when doing SetIP,
; since COMPlusCheckForAbort should always return 0.  Excep.cpp:LeaveCatch
; asserts that to be true.  If this ends up doing more work, then the
; debugger may need additional support.
; void __stdcall JIT_EndCatch();
JIT_EndCatch PROC stdcall public
    xor	    eax, eax
    push    eax		; pSEH
    push    eax		; pCtx
    push    eax		; pCurThread
    call    _COMPlusEndCatch@12	; returns old esp value in eax

    mov     ecx, [esp]  ; actual return address into jitted code
    mov     edx, eax    ; old esp value
    push    eax         ; save old esp
    push    ebp
    call    @COMPlusCheckForAbort@12  ; returns old esp value
    pop     ecx
    ; at this point, ecx   = old esp value 
    ;                [esp] = return address into jitted code
    ;                eax   = 0 (if no abort), address to jump to otherwise
    test    eax, eax
    jz      NormalEndCatch
    mov     esp, ecx
    jmp     eax

NormalEndCatch:
    pop     eax         ; Move the returnAddress into ecx
    mov     esp, ecx    ; Reset esp to the old value
    jmp     eax         ; Resume after the "endcatch"
JIT_EndCatch ENDP


;---------------------------------------------------------
; Performs an N/Direct call. This is a generic version
; that can handly any N/Direct call but is not as fast
; as more specialized versions.
;---------------------------------------------------------
;
; INT64 __stdcall NDirectGenericStubWorker(Thread *pThread,
;                                          NDirectMethodFrame *pFrame);
NDirectGenericStubWorker proc stdcall public,
			 pThread : DWORD,
                         pFrame  : DWORD
    local pHeader : DWORD
    local pvfn    : DWORD
    local pLocals : DWORD
    local pMLCode : DWORD

if DEBUG_OR_CUST_CHECKED
    local PreESP  : DWORD
    local PostESP : DWORD
.erre NDirectGenericWorkerFrameSize_DEBUG eq 40, "asmconstants.h is incorrect"
else
.erre NDirectGenericWorkerFrameSize eq 32, "asmconstants.h is incorrect"
endif

    lea     eax, pHeader
    push    eax		; &pHeader
    push    pFrame
    push    pThread
    call    _NDirectGenericStubComputeFrameSize@12
    ; eax = cbAlloc (rounded up to next DWORD)

    call    __alloca_probe ; __alloca, where the amt to allocate is in eax

    push    esp		; pAlloc
    lea     eax, pvfn
    push    eax         ; &pvfn
    lea     eax, pLocals
    push    eax         ; &pLocals
    lea     eax, pMLCode
    push    eax         ; &pMLCode
    push    pHeader
    push    pFrame
    push    pThread
    call    _NDirectGenericStubBuildArguments@28
    ; pvfn is now the function to invoke, pLocals points to the locals, and
    ; eax is zero or more MHLF_ constants.

    test    eax, MLHF_THISCALL_ASM
    jz	    doit
    test    eax, MLHF_THISCALLHIDDENARG_ASM
    jz	    regularthiscall

    ; this call, with hidden arg
    pop	    eax		; pop the first argument, shrinking the alloca space
    pop	    ecx		; pop the second argument
    push    eax
    jmp     doit

regularthiscall:
    pop     ecx		; pop the first argument, shrinking the alloca space

doit:
if DEBUG_OR_CUST_CHECKED
    mov     [PreESP], esp
endif
    call    dword ptr [pvfn]

; this label is used by the debugger, and must immediately follow the 'call'.
public _NDirectGenericReturnFromCall@0
_NDirectGenericReturnFromCall@0:

if DEBUG_OR_CUST_CHECKED
    mov     [PostESP], esp

    push    PostESP
    push    PreESP
    push    edx	        ; push high dword of the return value
    push    eax		; push low  dword of the return value
    push    pLocals
    push    pMLCode
    push    pHeader
    push    pFrame
    push    pThread
    call    _NDirectGenericStubPostCall@36
else ; retail and not customer_checked
    push    edx	        ; push high dword of the return value
    push    eax		; push low  dword of the return value
    push    pLocals
    push    pMLCode
    push    pHeader
    push    pFrame
    push    pThread
    call    _NDirectGenericStubPostCall@28
endif
    ; return value is in edx:eax
    ret
NDirectGenericStubWorker endp




; LPVOID __stdcall CTPMethodTable__CallTargetHelper2(
;     const void *pTarget,
;     LPVOID pvFirst,
;     LPVOID pvSecond)	
CTPMethodTable__CallTargetHelper2 proc stdcall public,
	                          pTarget : DWORD,
                                  pvFirst : DWORD,
	                          pvSecond : DWORD
    push    pvSecond
    push    pvFirst
    call    pTarget
ifdef _DEBUG
    nop				; Mark this as a special call site that can
	                        ; directly call unmanaged code
endif
    ret
CTPMethodTable__CallTargetHelper2 endp

; LPVOID __stdcall CTPMethodTable__CallTargetHelper3(
;     const void *pTarget,
;     LPVOID pvFirst,
;     LPVOID pvSecond,
;     LPVOID pvThird)
CTPMethodTable__CallTargetHelper3 proc stdcall public,
                                  pTarget : DWORD,
                                  pvFirst : DWORD,
                                  pvSecond : DWORD,
                                  pvThird : DWORD
    push    pvThird
    push    pvSecond
    push    pvFirst
    call    pTarget
ifdef _DEBUG
    nop                         ; Mark this as a special call site that can
                                ; directly call unmanaged code
endif
    ret
CTPMethodTable__CallTargetHelper3 endp


; void __stdcall setFPReturn(int fpSize, INT64 retVal)
_setFPReturn@12 proc public
    mov     eax, [esp+4]
    cmp     eax, 4
    jz      setFPReturn4

    cmp     eax, 8
    jnz     setFPReturnNot8
    fld     qword ptr [esp+8]
setFPReturnNot8:
    retn    12

setFPReturn4:
    fld     dword ptr [esp+8]
    retn    12
_setFPReturn@12 endp

; void __stdcall getFPReturn(int fpSize, INT64 *pretVal)
_getFPReturn@8 proc public
   mov     ecx, [esp+4]
   mov     eax, [esp+8]
   cmp     ecx, 4
   jz      getFPReturn4

   cmp     ecx, 8
   jnz     getFPReturnNot8
   fstp    qword ptr [eax]
getFPReturnNot8:
   retn    8

getFPReturn4:
   fstp    dword ptr [eax]
   retn    8
_getFPReturn@8 endp

; void *__stdcall UM2MThunk_WrapperHelper(void *pThunkArgs,
;                                         int argLen,
;                                         void *pAddr,
;                                         UMEntryThunk *pEntryThunk,
;                                         Thread *pThread)
UM2MThunk_WrapperHelper proc stdcall public,
                        pThunkArgs : DWORD,
                        argLen : DWORD,
                        pAddr : DWORD,
                        pEntryThunk : DWORD,
                        pThread : DWORD

    mov     ecx, argLen
    test    ecx, ecx
    jz      UM2MThunk_ArgsComplete ; brif no args

    ; copy the args to the stack, they will be released when we return from
    ; the thunk because we are coming from unmanaged to managed, any
    ; objectref args must have already been pinned so don't have to worry
    ; about them moving in the copy we make.
    _ASSERT_ALIGNED_4_X86 ecx
    mov     eax, pThunkArgs
UM2MThunk_CopyArg:
    push    dword ptr [eax+ecx-4]          ; push the argument on the stack
    sub     ecx, 4               ; move to the next argument
    jnz     UM2MThunk_CopyArg

UM2MThunk_ArgsComplete:
    mov     eax, pEntryThunk
    mov     ecx, pThread
    call    pAddr

    ret
UM2MThunk_WrapperHelper endp

; VOID __cdecl UMThunkStubRareDisable()
_UMThunkStubRareDisable proc public
    push    eax
    push    ecx

    push    esi          ; Push frame
    push    eax          ; Push the UMEntryThunk
    push    ecx          ; Push thread
    call    _UMThunkStubRareDisableWorker@12

    pop     ecx
    pop     eax
    retn
_UMThunkStubRareDisable endp


;+----------------------------------------------------------------------------
;
;  Method:     CRemotingServices::CheckForContextMatch   public
;
;  Synopsis:   This code generates a check to see if the current context and
;              the context of the proxy match.
;+----------------------------------------------------------------------------
; void __stdcall CRemotingServices__CheckForContextMatch(void)
_CRemotingServices__CheckForContextMatch@0 proc public
    push    ebx                  ; spill ebx
    mov     ebx, [eax+4]         ; Get the internal context id by unboxing
                                 ; the stub data
    call    [_GetThread]         ; Get the current thread, assumes that the
                                 ; registers are preserved
    mov     eax, [eax+Thread_m_Context] ; Get the current context from the
                                 ; thread
    sub     eax, ebx             ; Get the pointer to the context from the
                                 ; proxy and compare with the current context
    pop     ebx                  ; restore the value of ebx
    retn
_CRemotingServices__CheckForContextMatch@0 endp

;+----------------------------------------------------------------------------
;
;  Method:     CRemotingServices::DispatchInterfaceCall   public
;
;  Synopsis:
;              Push that method desc on the stack and jump to the
;              transparent proxy stub to execute the call.
;              WARNING!! This MethodDesc is not the methoddesc in the vtable
;              of the object instead it is the methoddesc in the vtable of
;              the interface class. Since we use the MethodDesc only to probe
;              the stack via the signature of the method call we are safe.
;              If we want to get any object vtable/class specific
;              information this is not safe.
;
;+----------------------------------------------------------------------------
; void __stdcall CRemotingServices__DispatchInterfaceCall(MethodDesc *pMD)
_CRemotingServices__DispatchInterfaceCall@4 proc public
    ; NOTE: At this point the stack looks like
    ;
    ; esp--->  return addr of stub
    ;          saved MethodDesc of Interface method
    ;          return addr of calling function
    ;
    mov      eax, [ecx + TP_OFFSET_STUBDATA]
    call    [ecx + TP_OFFSET_STUB]
ifdef _DEBUG
    nop     ; Mark this as a special call site that can directly
            ; call managed code
endif
    test    eax, eax
    jnz     CtxMismatch

CtxMatch:
    ; in current context, so resolve MethodDesc to real slot no
    push    ebx                                  ; spill ebx

    mov     eax, [esp + 8]                       ; eax <-- MethodDesc
    movsx   ebx, byte ptr [eax + MD_IndexOffset_ASM] ; get MethodTable from pMD
    mov     eax, [eax + ebx*MethodDesc__ALIGNMENT + MD_SkewOffset_ASM]
    mov     eax, [eax+MethodTable_m_pEEClass]    ; get EEClass from pMT

    mov     ebx, [eax+EEClass_m_dwInterfaceId]   ; get the interface id from
                                                 ; the EEClass
    mov     eax, [ecx + TP_OFFSET_MT]            ; get the *real* MethodTable

    mov     eax, [eax+MethodTable_m_pInterfaceVTableMap] ; get interface map
    mov     eax, [eax + ebx*4]                   ; offset map by interface id
    mov     ebx, [esp + 8]                       ; get MethodDesc
    mov     bx,  [ebx+MethodDesc_m_wSlotNumber]
    and     ebx, 0ffffh
    mov     eax, [eax + ebx*4]                   ; get jump addr

    pop     ebx                                  ; restore ebx

    add     esp, 8                               ; pop off Method desc and
                                                 ; and stub's return addr
    jmp     eax

CtxMismatch:                                     ; Jump to TPStub

    add     esp, 4                               ; pop ret addr of stub so that the
                                                 ; stack and registers are now
                                                 ; setup exactly like they
                                                 ; were at the callsite

    jmp     [_g_dwOOContextAddr]                 ; jump to OOContext label in
                                                 ; TPStub
_CRemotingServices__DispatchInterfaceCall@4 endp


;+----------------------------------------------------------------------------
;
;  Method:     CRemotingServices::CallFieldGetter   private
;
;  Synopsis:   Calls the field getter function (Object::__FieldGetter) in
;              managed code by setting up the stack and calling the target
;
;+----------------------------------------------------------------------------
; void __stdcall CRemotingServices__CallFieldGetter(
;    MethodDesc *pMD,
;    LPVOID pThis,
;    LPVOID pFirst,
;    LPVOID pSecond,
;    LPVOID pThird)
CRemotingServices__CallFieldGetter proc stdcall public,
                                   pMD : DWORD,
                                   pThis : DWORD,
                                   pFirst : DWORD,
                                   pSecond : DWORD,
                                   pThird : DWORD

    mov     ecx, [pThis]        ; enregister pThis, the 'this' pointer
    mov     edx, [pFirst]       ; enregister pFirst, the first argument

    push    [pThird]            ; push the third argument on the stack
    push    [pSecond]           ; push the second argument on the stack

    lea     eax, retAddrFieldGetter
    push    eax

    push    [pMD]               ; push the MethodDesc of object::__FieldGetter
    jmp     [_g_dwTPStubAddr]   ; jump to the TP stub

retAddrFieldGetter:
    ret
CRemotingServices__CallFieldGetter endp


;+----------------------------------------------------------------------------
;
;  Method:     CRemotingServices::CallFieldSetter   private
;
;  Synopsis:   Calls the field setter function (Object::__FieldSetter) in
;              managed code by setting up the stack and calling the target
;
;+----------------------------------------------------------------------------
; void __stdcall CRemotingServices__CallFieldSetter(
;    MethodDesc *pMD,
;    LPVOID pThis,
;    LPVOID pFirst,
;    LPVOID pSecond,
;    LPVOID pThird)
CRemotingServices__CallFieldSetter proc stdcall public,
                                   pMD : DWORD,
                                   pThis : DWORD,
                                   pFirst : DWORD,
                                   pSecond : DWORD,
                                   pThird : DWORD

    mov     ecx, [pThis]        ; enregister pThis, the 'this' pointer
    mov     edx, [pFirst]       ; enregister the first argument

    push    [pThird]            ; push the object (third arg) on the stack
    push    [pSecond]           ; push the field name (second arg)

    lea     eax, retAddrFieldSetter ; push the return address
    push    eax

    push    [pMD]               ; push the MethodDesc of Object::__FieldSetter
    jmp     [_g_dwTPStubAddr]   ; jump to the TP Stub

retAddrFieldSetter:
    ret
CRemotingServices__CallFieldSetter endp


;+----------------------------------------------------------------------------
;
;  Method:     CTPMethodTable::GenericCheckForContextMatch private
;
;  Synopsis:   Calls the stub in the TP & returns TRUE if the contexts
;              match, FALSE otherwise.
;
;  Note:       1. Called during FieldSet/Get, used for proxy extensibility
;+----------------------------------------------------------------------------
; BOOL __stdcall CTPMethodTable__GenericCheckForContextMatch(Object* orTP)
CTPMethodTable__GenericCheckForContextMatch proc stdcall public uses ecx,
                                            tp : DWORD

    mov     ecx, [tp]
    mov     eax, [ecx+TP_OFFSET_STUBDATA]
    call    [ecx + TP_OFFSET_STUB]
ifdef _DEBUG
    nop     ; Mark this as a special call site that can directly
            ; call managed code
endif
    test    eax, eax
    mov     eax, 0
    setz    al
    ; NOTE: In the CheckForXXXMatch stubs (for URT ctx/ Ole32 ctx) eax is
    ; non-zero if contexts *do not* match & zero if they do.
    ret
CTPMethodTable__GenericCheckForContextMatch endp


; void __stdcall JIT_ProfilerStub(FunctionID ProfilingHandle)
_JIT_ProfilerStub@4 proc public
    ; this function must preserve all registers, including scratch
    retn    4
_JIT_ProfilerStub@4 endp


;
; Used to get the current instruction pointer value
;
; LPVOID __stdcall GetIP(void);
_GetIP@0 proc public
    mov     eax, [esp]
    retn
_GetIP@0 endp

; LPVOID __stdcall GetSP(void);
_GetSP@0 proc public 
    mov     eax, esp
    retn
_GetSP@0 endp

; void __stdcall InprocEnterNaked(FunctionID functionId);
_InprocEnterNaked@4 proc public
    push    ebp
    mov     ebp, esp
    push    dword ptr [ebp+8]
    call    _InprocEnter
    add     esp, 4
    pop     ebp
    retn    4
_InprocEnterNaked@4 endp

; void __stdcall InprocLeaveNaked(FunctionID functionId);
_InprocLeaveNaked@4 proc public
    push    eax
    push    ecx
    push    edx
    push    dword ptr [esp+16]
    call    _InprocLeave
    add     esp, 4
    pop     edx
    pop     ecx
    pop     eax
    retn    4
_InprocLeaveNaked@4 endp


; void __stdcall InprocTailcallNaked(FunctionID functionId);
_InprocTailcallNaked@4 proc public
    push    eax
    push    ecx
    push    edx
    push    dword ptr [esp+16]
    call    _InprocTailcall
    add     esp, 4
    pop     edx
    pop     ecx
    pop     eax
    retn    4
_InprocTailcallNaked@4 endp

; void __cdecl CallThunk()
_CallThunk proc public
    lea     eax, [esp+4]; get address of return address into the caller of the THUNK. 
                        ; This well be used as the return address in the machine state

    mov     ecx, [esp+8]
    mov     edx, [esp+12]
	
    push    ecx         ; pass arg1    ; Need to be in this order for ArgIterator

    push    edx         ; pass arg2    (save the value)

    ; From here we are making a MachState structure.  
    push    eax         ; address of return address of whoever called the thunk
    push    0CCCCCCCCh  ; The ESP after we return.  We dont know this
                        ; since this is a shared thunk and we dont know
                        ; how many arguments to pop.  Put an illegal value
                        ; here to insure we don't use it 
    push    ebp 
    push    esp         ; pEbp
    push    ebx 
    push    esp         ; pEbx
    push    esi 
    push    esp         ; pEsi
    push    edi 
    push    esp         ; pEdi

    push    [eax-4]	; Make a copy of the caller's address to prevent it from being overwritten by the 
	                ; compiler optimizations
    lea     eax, [esp+44]; Pointer to the struct containing the enregisted arguments 
    push    eax

    lea     eax, [esp+8]; Pointer to the MachineState structure 
    push    eax
	
    call    _RejitCalleeMethod 

    add     esp, 12    ; Pop the arguments to RejitCalleeMethod of the stack
    mov     edi, [esp+4]; restore regs
    mov     esi, [esp+12]
    mov     ebx, [esp+20]
    mov     ebp, [esp+28]
    add     esp, 40     ; Pop off sizeof(MachineState)

    pop     edx         ; restore arg2
    pop     ecx         ; restore arg1

	
    lea     esp, [esp+4]; pop thunk return address
    jmp     eax
_CallThunk endp

; void __cdecl RejitThunk()
_RejitThunk proc public 
    mov     ecx, esp    ; save pointer to return address for MachState. From a stack
                        ; crawling point of view we have not really 'returned' from
                        ; the method whose return cause this code to be invoked 

    add     esp, -8     ; save the floating point return value 
    fstp    qword ptr [esp]
	
    push    edx         ; pass upper bytes of return value (if it is a long)

    push    eax         ; pass return value (so we can protect it

    ; From here we are making a MachState structure.  
    push    ecx         ; return address from a stack crawling point of view
    add     ecx, 4
    push    ecx         ; The ESP after we return.  (we poped the return value)
    push    ebp 
    push    esp         ; pEbp
    push    ebx 
    push    esp         ; pEbx
    push    esi 
    push    esp         ; pEsi
    push    edi 
    push    esp         ; pEdi

    push    [ecx-4]	; Make a copy of the thunk address to prevent from being overwritten by the 
	                ; compiler optimizations
    lea     eax, [esp + 44] ;  Pointer the saved return value
    push    eax

    lea     eax, [esp+8] ; Pointer to the MachState structure
    push    eax
	
    call    _RejitCallerMethod 
    mov     ecx, eax    ; save location to return to

    add     esp, 12	; erase the argument to the RejitCallerMethod
	 
    mov     edi, [esp+4]; restore regs
    mov     esi, [esp+12]
    mov     ebx, [esp+20]
    mov     ebp, [esp+28]
    add     esp, 40     ; Pop off sizeof(MachineState)

    pop     eax         ; restore return value
    pop     edx         ; restore upper bytes of return value (if it is a long)
    fld     qword ptr [esp] ; restore the floating point return value   
    add     esp, 8      
    lea     esp, [esp+4]; pop thunk return address (now we have actually returned!)
    jmp     ecx
_RejitThunk endp

    end
