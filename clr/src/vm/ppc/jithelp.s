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

// This contains JITinterface routines that are 100% PowerPC assembly

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


// A handy macro for declaring a public function
// The first argument is the function name
.macro ASMFUNC
  .align 2
  .globl _$0
_$0:
.endmacro

	// The stack on entry:
	// sp + 16 + 24 -- Arguments
	// sp + 12 + 24 -- sizeofCallerSpillArea (not used)
	// sp + 8  + 24 -- sizeofTargetArguments
	// sp + 4  + 24 -- flags (not used)
	// sp + 0  + 24 -- target address
	 
ASMFUNC JIT_TailCall
	#define LINKAGE_AREA 24
	#define NumGPRegisters 8
	#define TargetSizeOff 8
	#define TargetArgsOff 16
	#define TargetAddrOff 0
	
        // sp  + 16 - arguments to target function
        // r30  + 24 - spill area of the callee
        // r12 - amount left to copy
        lwz   r12, TargetSizeOff(r1)     // r12 = sizeofTargetArguments
	cmpwi r12, NumGPRegisters+1      // Skip first 8 words of arguments
        blt   LArgCopyDone
	addi  r12, r12, -NumGPRegisters  
	mtctr r12                           // initalize the counter register
	addi  t0, sp, TargetArgsOff + NumGPRegisters*4  // pointer to arguments
	addi  t1, r30, LINKAGE_AREA + NumGPRegisters*4   // pointer to spill area
LArgCopyLoop:
        lwz   r0, 0(t0)
        addi  t0, t0, 4
        stw   r0, 0(t1)
        addi  t1, t1, 4
        bdnz  LArgCopyLoop
LArgCopyDone:
	
        // Move the address of the target function into the counter register
        lwz   r12, TargetAddrOff(sp)
        mtctr r12
	
        // Restore the state of the processor
        lwz   r29, -8(r30) // callee saved register
        lwz   r0,   8(r30) // restore the link register
        mtlr  r0
        or    sp, r30, r30 // sp = fp
        lwz   r30, -4(sp)  // fp = old_fp
 
        // Branch to target 
        bctr

	#undef LINKAGE_AREA
	#undef NumGPRegisters
	#undef TargetSizeOff
	#undef TargetArgsOff
	#undef TargetAddrOff




