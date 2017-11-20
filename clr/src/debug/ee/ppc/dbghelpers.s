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

// a handy macro for declaring a public function 
.macro ASMFUNC 
  .align 2 
  .globl _$0 
_$0: 
.endmacro 

#define roundup(n, roundto)  ((n)+roundto-((n) % roundto))

#define LINKAGE_AREA 24
#define RED_ZONE 224

#define sp r1
#define t0 r2

// ArgumentRegisters from clr/src/vm/ppc/cgencpu.h
#define ArgumentRegisters_r 0
#define ArgumentRegisters_f 32
#define ArgumentRegisters_size (8*NUM_FLOAT_ARGUMENT_REGISTERS+4*NUM_ARGUMENT_REGISTERS)

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

// This is the method that we hijack a thread running managed code. It calls
// FuncEvalHijackWorker, which actually performs the func eval, then jumps to 
// the patch address so we can complete the cleanup.
//
// Note: the parameter is passed in r11 - see Debugger::FuncEvalSetup for
//       details
//
ASMFUNC FuncEvalHijack
    // skip the reserved red zone & create caller linkage area
    stwu sp, -roundup(LINKAGE_AREA+RED_ZONE, 16)(sp)

    // spill the link registers to the stack
    mflr r0
    stw  r0, 8(sp)

    // allocate callee frame
    stwu sp, -(LINKAGE_AREA+8 + ArgumentRegisters_size)(sp)

    // Spill the argument registers to the stack
    ARGUMENT_REGISTERS_GP stw,(-ArgumentRegisters_size+ArgumentRegisters_r),sp
    ARGUMENT_REGISTERS_FP stfd,(-ArgumentRegisters_size+ArgumentRegisters_f),sp

    mr r3, r11
    bl _FuncEvalHijackWorker
    mr r11,r3

    // Reload the argument registers from the stack
    ARGUMENT_REGISTERS_GP lwz,(-ArgumentRegisters_size+ArgumentRegisters_r),sp
    ARGUMENT_REGISTERS_FP lfd,(-ArgumentRegisters_size+ArgumentRegisters_f),sp

    // Restore the stack
    lwz sp, 0(sp)

    // Restore the link register
    lwz r0, 8(sp)
    mtlr r0

    // Skip the red area
    lwz sp, 0(sp)

    // return value is the address to jump to
    mtctr r11
    bctr

