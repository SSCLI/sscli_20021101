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
//             gmsasm.asm in this directory                            

	.intel_syntax
	.arch i586

#include "asmconstants.h"

// int __stdcall LazyMachStateCaptureState(struct LazyMachState *pState);
.global LazyMachStateCaptureState
.func LazyMachStateCaptureState
LazyMachStateCaptureState:	
        mov %ecx, [%esp+4]	          //  get pState into ecx
        mov dword ptr [%ecx+MachState__pRetAddr], 0 // marks that this is not yet valid
        mov [%ecx+MachState__edi], %edi   // remember register values
	mov [%ecx+MachState__esi], %esi 
        mov [%ecx+MachState__ebx], %ebx
	mov [%ecx+LazyMachState_captureEbp], %ebp
	lea %eax, [%esp+4]	          // capture esp, but simulate the
	                                  // pop of the pState arg, to be
					  // compatible with the __fastcall
					  // Win32 version of this function
	mov [%ecx+LazyMachState_captureEsp], %eax

        mov %eax, [%esp]                  // capture return address
	mov [%ecx+LazyMachState_captureEip], %eax
	xor %eax, %eax
	ret 4
.endfunc

.end


