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
// asmconstants.h - 
//
// This header defines field offsets and constants used by assembly code
// Be sure to rebuild clr/src/vm/ceemain.cpp after changing this file, to
// ensure that the constants match the expected C/C++ values

#ifndef _PPC_
#error this file should only be used on an PPC platform
#endif // _PPC_

// from clr/src/vm/remoting.h
#define TP_OFFSET_STUBDATA    0x8
#define TP_OFFSET_MT          0xc
#define TP_OFFSET_STUB        0x14

// from clr/src/vm/ppc/gmscpu.h
#define MachState__Regs     0
#define MachState__pRegs    (4*NUM_CALLEESAVED_REGISTERS)
#define MachState__cr       (8*NUM_CALLEESAVED_REGISTERS)
#define MachState__sp       (MachState__cr+4)
#define MachState__pRetAddr (MachState__sp+4)
#define MachState_size      (8*NUM_CALLEESAVED_REGISTERS + 12)
#define LazyMachState_captureSp     (MachState__pRetAddr+4)
#define LazyMachState_captureSp2    (LazyMachState_captureSp+4)
#define LazyMachState_capturePC     (LazyMachState_captureSp2+4)

// ArgumentRegisters from clr/src/vm/ppc/cgencpu.h
#define ArgumentRegisters_r 0
#define ArgumentRegisters_f 32
#define ArgumentRegisters_size (8*NUM_FLOAT_ARGUMENT_REGISTERS+4*NUM_ARGUMENT_REGISTERS)

// CalleeSavedRegisters from clr/src/vm/ppc/cgencpu.h
#define CalleeSavedRegisters_cr 0
#define CalleeSavedRegisters_r  4
#define CalleeSavedRegisters_f  (4*(NUM_CALLEESAVED_REGISTERS+1))
#define CalleeSavedRegisters_size (8*NUM_FLOAT_CALLEESAVED_REGISTERS+4*NUM_CALLEESAVED_REGISTERS+4)

// EHContext from clr/src/vm/ppc/cgencpu.h
#define EHContext_R     0
#define EHContext_F     (4*32)
#define EHContext_CR    (EHContext_F+8*NUM_FLOAT_CALLEESAVED_REGISTERS)

// FaultingExceptionFrame from clr/src/vm/frames.h
#define FaultingExceptionFrame_m_regs   16
#define FaultingExceptionFrame_m_Link   240

// The size of the overall stack frame for NDirectGenericStubWorker,
// including the parameters, return address, preserved registers, and
// local variables, but not including any __alloca'd memory.
// 
#define NDirectGenericWorkerFrameSize   48

// The size of the overall stack frame for PInvokeCalliStubWorker,
// including the parameters, return address, preserved registers, and
// local variables, but not including any __alloca'd memory.
// 
#define PInvokeCalliWorkerFrameSize     48

// ICodeManager::SHADOW_SP_IN_FILTER from clr/src/inc/eetwain.h
#define SHADOW_SP_IN_FILTER_ASM 0x1

// from clr/src/vm/threads.h
#define Thread_m_Context    0x2C

// from clr/src/vm/method.hpp
#define METHOD_CALL_PRESTUB_SIZE_ASM    20

#define MD_IndexOffset_ASM    (-22)
#define MD_SkewOffset_ASM     (-40)

#define MethodDesc__ALIGNMENT 8

#ifdef _DEBUG
#define MethodDesc_m_wSlotNumber 0x1c
#else
#define MethodDesc_m_wSlotNumber 0x04
#endif

// from clr/src/vm/class.h
#ifdef _DEBUG
#define EEClass_m_dwInterfaceId 0x1c
#else
#define EEClass_m_dwInterfaceId 0x18
#endif

#define MethodTable_m_pEEClass 8
#define MethodTable_m_pInterfaceVTableMap 12
