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
; ***********************************************************************
; File: JIThelp.asm
;
; ***********************************************************************
;
;  *** NOTE:  If you make changes to this file, propagate the changes to
;             jithelp.s in this directory                            

; This contains JITinterface routines that are 100% x86 assembly

        .386
        OPTION  OLDSTRUCTS

        include asmconstants.inc
;
;

ARGUMENT_REG1           equ     ecx
ARGUMENT_REG2           equ     edx
JIT_Dbl2Lng                     TEXTEQU <_JIT_Dbl2Lng@8>
JIT_TailCall                    TEXTEQU <_JIT_TailCall@0>
JIT_TailCallHelper              TEXTEQU <_JIT_TailCallHelper@12>

EXTERN  JIT_TailCallHelper:PROC




_TEXT   SEGMENT PARA PUBLIC FLAT 'CODE'
        ASSUME  CS:FLAT, DS:FLAT, ES:FLAT, SS:FLAT, FS:NOTHING



;*********************************************************************/
; JIT_Dbl2Lng

;Purpose:
;   converts a double to a long truncating toward zero (C semantics)
;
;	uses stdcall calling conventions 
;
;   note that changing the rounding mode is very expensive.  This
;   routine basiclly does the truncation sematics without changing
;   the rounding mode, resulting in a win.
;
PUBLIC JIT_Dbl2Lng
JIT_Dbl2Lng PROC
	fld qword ptr[ESP+4]		; fetch arg
	lea ecx,[esp-8]
	sub esp,16 			; allocate frame
	and ecx,-8 			; align pointer on boundary of 8
	fld st(0)			; duplciate top of stack
	fistp qword ptr[ecx]		; leave arg on stack, also save in temp
	fild qword ptr[ecx]		; arg, round(arg) now on stack
	mov edx,[ecx+4] 		; high dword of integer
	mov eax,[ecx] 			; low dword of integer
	test eax,eax
	je integer_QNaN_or_zero

arg_is_not_integer_QNaN:
	fsubp st(1),st 			; TOS=d-round(d),
					; { st(1)=st(1)-st & pop ST }
	test edx,edx 			; what's sign of integer
	jns positive
					; number is negative
					; dead cycle
					; dead cycle
	fstp dword ptr[ecx]		; result of subtraction
	mov ecx,[ecx] 			; dword of difference(single precision)
	add esp,16
	xor ecx,80000000h
	add ecx,7fffffffh		; if difference>0 then increment integer
	adc eax,0 			; inc eax (add CARRY flag)
	adc edx,0			; propagate carry flag to upper bits
	ret 8

positive:
	fstp dword ptr[ecx]		;17-18 ; result of subtraction
	mov ecx,[ecx] 			; dword of difference (single precision)
	add esp,16
	add ecx,7fffffffh		; if difference<0 then decrement integer
	sbb eax,0 			; dec eax (subtract CARRY flag)
	sbb edx,0			; propagate carry flag to upper bits
	ret 8

integer_QNaN_or_zero:
	test edx,7fffffffh
	jnz arg_is_not_integer_QNaN
	fstp st(0)			;; pop round(arg)
	fstp st(0)			;; arg
	add esp,16
	ret 8
JIT_Dbl2Lng ENDP


PUBLIC JIT_TailCall
JIT_TailCall PROC
	mov     eax, esp    ; set eax = TailCallArgs*

; Create a MachState struct on the stack 

; return address is already on the stack, but is separated from stack 
; arguments by the extra arguments of JIT_TailCall. So we cant use it directly

        push    0DDDDDDDDh

; Esp on unwind. Not needed as we it is deduced from the target method

        push    0CCCCCCCCh
        push    ebp 
        push    esp         ; pEbp
        push    ebx 
        push    esp         ; pEbx
        push    esi 
        push    esp         ; pEsi
        push    edi 
        push    esp         ; pEdi
	mov     ebx, esp    ; set ebx = MachState*

        push    ecx         ; ArgumentRegisters
        push    edx

        push    esp         ; ArgumentRegisters*
        push    eax         ; TailCallArgs*
        push    ebx         ; MachState*

        ; JIT_TailCallHelper(MachState*, TailCallArgs*, ArgumentRegisters*)
        call    JIT_TailCallHelper  ; this is __stdcall
        ; on return, eax is the new esp value to use when returning

        ; Restore ArgumentRegisters
        pop     edx
        pop     ecx

        ; Restore MachState registers
        mov     [esp+MachState__esp], eax ; set MachState.esp=eax
        pop     eax         ; discard pEdi
        pop     edi         ; restore edi
        pop     eax         ; discard pEsi
        pop     esi         ; restore esi
        pop     eax         ; discard pEbx
        pop     ebx         ; restore ebx
        pop     eax         ; discard pEbp
        pop     ebp         ; restore ebp
        pop     esp         ; restore esp
        retn                ; Will branch to targetAddr.  This matches the
                            ; "call" done by JITted code, keeping the
                            ; call-ret count balanced.
JIT_TailCall ENDP


; float __stdcall JIT_FltRem(float divisor, float dividend)
_JIT_FltRem@8 proc public
        fld  dword ptr [esp+4]          ; divisor
        fld  dword ptr [esp+8]          ; dividend
fremloop:
        fprem
        fstsw   ax
        fwait
        sahf
        jp      fremloop        ; Continue while the FPU status bit C2 is set
        fxch    ; swap, so divisor is on top and result is in st(1)
        fstp    ST(0)           ; Pop the divisor from the FP stack
        retn    8               ; Return value is in st(0)
_JIT_FltRem@8 endp

; double __stdcall JIT_DblRem(double divisor, double dividend)
_JIT_DblRem@16 proc public
        fld  qword ptr [esp+4]          ; divisor
        fld  qword ptr [esp+12]         ; dividend
fremloopd:
        fprem
        fstsw   ax
        fwait
        sahf
        jp      fremloopd       ; Continue while the FPU status bit C2 is set
        fxch    ; swap, so divisor is on top and result is in st(1)
        fstp    ST(0)           ; Pop the divisor from the FP stack
        retn    16              ; Return value is in st(0)
_JIT_DblRem@16 endp

_TEXT   ENDS
        END

