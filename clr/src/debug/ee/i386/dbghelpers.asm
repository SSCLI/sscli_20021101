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
;             dbghelpers.s in this directory                            

	.586
	.model	flat
        .code

        extern _FuncEvalHijackWorker@4:PROC

;
; This is the method that we hijack a thread running managed code. It calls
; FuncEvalHijackWorker, which actually performs the func eval, then jumps to 
; the patch address so we can complete the cleanup.
;
; Note: the parameter is passed in eax - see Debugger::FuncEvalSetup for
;       details
;
_FuncEvalHijack@0 proc public
        push eax       ; the ptr to the DebuggerEval
        call _FuncEvalHijackWorker@4
        jmp  eax       ; return is the patch addresss to jmp to
_FuncEvalHijack@0 endp

	end






