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

#ifndef _X86_
#error this file should only be used on an X86 platform
#endif // _X86_

// from clr/src/vm/remoting.h
#define TP_OFFSET_STUBDATA    0x8
#define TP_OFFSET_MT          0xc
#define TP_OFFSET_STUB        0x14

// CONTEXT from rotor_pal.h
#define CONTEXT_Edi 0x9c
#define CONTEXT_Esi 0xa0
#define CONTEXT_Ebx 0xa4
#define CONTEXT_Edx 0xa8
#define CONTEXT_Eax 0xb0
#define CONTEXT_Ebp 0xb4
#define CONTEXT_Eip 0xb8
#define CONTEXT_Esp 0xc4

// EHContext from clr/src/vm/i386/cgencpu.h
#define EHContext_Eax 0x00
#define EHContext_Ebx 0x04
#define EHContext_Ecx 0x08
#define EHContext_Edx 0x0c
#define EHContext_Esi 0x10
#define EHContext_Edi 0x14
#define EHContext_Ebp 0x18
#define EHContext_Esp 0x1c
#define EHContext_Eip 0x20

// from clr/src/fjit/helperframe.h
#define MachState__pEdi           0
#define MachState__edi            4
#define MachState__pEsi           8
#define MachState__esi            12
#define MachState__pEbx           16
#define MachState__ebx            20
#define MachState__pEbp           24
#define MachState__ebp            28
#define MachState__esp            32
#define MachState__pRetAddr       36
#define LazyMachState_captureEbp  40
#define LazyMachState_captureEsp  44
#define LazyMachState_captureEip  48

// The size of the overall stack frame for NDirectGenericStubWorker,
// including the parameters, return address, preserved registers, and
// local variables, but not including any __alloca'd memory.
// 
// The size counts two arguments, the return value, and one preserved register
// for a total of 16 bytes, plus 16 bytes of locals
#define NDirectGenericWorkerFrameSize (16+16)

// in _DEBUG or CUSTOMER_CHECKED_BUILDS, there are 8 more bytes of locals
// there is no '#if' here - the generate asmconstants.inc should remain
// identical regardless of which defines are set, to avoid problems if
// more than one build flavor co-exist in one source tree.
#define NDirectGenericWorkerFrameSize_DEBUG (NDirectGenericWorkerFrameSize+8)


// ICodeManager::SHADOW_SP_IN_FILTER from clr/src/inc/eetwain.h
#define SHADOW_SP_IN_FILTER_ASM 0x1

//#define ARRAYOPACCUMOFS     0
//#define ARRAYOPMULTOFS      4
//#define ARRAYOPMETHODDESC   8
#define ARRAYOPLOCSIZE      12
// ARRAYOPACCUMOFS and ARRAYOPMULTOFS should have been popped by our callers.
#define ARRAYOPLOCSIZEFORPOP (ARRAYOPLOCSIZE-8)

// from clr/src/inc/corinfo.h
 #define CORINFO_HELP_INTERNALTHROW_ASM 0x2f
#define CORINFO_NullReferenceException_ASM 0
#define CORINFO_IndexOutOfRangeException_ASM 3
#define CORINFO_ArrayTypeMismatchException_ASM 6
#define CORINFO_RankException_ASM 7

// from clr/src/vm/jitinterface.h
#ifdef _DEBUG
  #define VMHELPDEF_pfnHelper 4
  #define VMHELPDEF_Size      0x10
#else
  #define VMHELPDEF_pfnHelper 0
  #define VMHELPDEF_Size      0xc
#endif

// from clr/src/vm/ml.h
#define MLHF_THISCALL_ASM          0x0100
#define MLHF_THISCALLHIDDENARG_ASM 0x0200

// from clr/src/vm/threads.h
#define Thread_m_Context    0x2C

// from clr/src/vm/method.hpp
#define MD_IndexOffset_ASM    (-6)

#define MD_SkewOffset_ASM     (-24)

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

