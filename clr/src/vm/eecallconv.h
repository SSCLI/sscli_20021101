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

//===================================================================
// EECALLCONV.H
//
//  This file can be included multiple times and is useful when you
//  need to write code that depends on details of the calling convention.
//
//  To learn how many registers are reserved for arguments, use the macro
//  NUM_ARGUMENT_REGISTERS
//
//  To enumerate the registers from the first (param #1 or "this") to last,
//
//      #define DEFINE_ARGUMENT_REGISTER(regname)  <expression using regname>
//      #include "eecallconv.h"
//
//  To enumerate the registers in reverse order
//
//      #define DEFINE_ARGUMENT_REGISTER_BACKWARD(regname)  <expression using regname>
//      #include "eecallconv.h"
//
//
//===================================================================


#if defined(_X86_)

#ifndef NUM_ARGUMENT_REGISTERS
#define NUM_ARGUMENT_REGISTERS 2
#endif // !NUM_ARGUMENT_REGISTERS


//--------------------------------------------------------------------
// This defines one register guaranteed not to hold an argument.
//--------------------------------------------------------------------
#ifndef SCRATCH_REGISTER
#define SCRATCH_REGISTER EAX
#endif // !SCRATCH_REGISTER

#ifndef SCRATCH_REGISTER_X86REG
#define SCRATCH_REGISTER_X86REG kEAX
#endif // !SCRATCH_REGISTER_X86REG


#ifndef DEFINE_ARGUMENT_REGISTER
#ifndef DEFINE_ARGUMENT_REGISTER_BACKWARD
#ifndef DEFINE_ARGUMENT_REGISTER_BACKWARD_WITH_OFFSET
#ifndef DEFINE_ARGUMENT_REGISTER_NOTHING    // used to pick up NUM_ARGUMENT_REGISTERS above
#error  "You didn't pick any choices. Check your spelling."
#endif // !DEFINE_ARGUMENT_REGISTER_NOTHING
#endif // !DEFINE_ARGUMENT_REGISTER_BACKWARD_WITH_OFFSET
#endif // !DEFINE_ARGUMENT_REGISTER_BACKWARD
#endif // !DEFINE_ARGUMENT_REGISTER


//-----------------------------------------------------------------------
// Location of "this" argument.
//-----------------------------------------------------------------------
#ifndef THIS_REG
#define THIS_REG        ECX
#endif // !THIS_REG

#ifndef THIS_kREG
#define THIS_kREG       kECX
#endif // !THIS_kREG


#define ARGUMENT_REG1   ECX
#define ARGUMENT_REG2   EDX

//-----------------------------------------------------------------------
// List of registers in forward order. MUST KEEP ALL LISTS IN SYNC.
//-----------------------------------------------------------------------

#ifndef DEFINE_ARGUMENT_REGISTER
#define DEFINE_ARGUMENT_REGISTER(regname)
#endif // !DEFINE_ARGUMENT_REGISTER

DEFINE_ARGUMENT_REGISTER(  ECX  )
DEFINE_ARGUMENT_REGISTER(  EDX  )

#undef DEFINE_ARGUMENT_REGISTER



//-----------------------------------------------------------------------
// List of registers in backward order. MUST KEEP ALL LISTS IN SYNC.
//-----------------------------------------------------------------------

#ifndef DEFINE_ARGUMENT_REGISTER_BACKWARD
#define DEFINE_ARGUMENT_REGISTER_BACKWARD(regname)
#endif // !DEFINE_ARGUMENT_REGISTER_BACKWARD


DEFINE_ARGUMENT_REGISTER_BACKWARD(EDX)
DEFINE_ARGUMENT_REGISTER_BACKWARD(ECX)


#undef DEFINE_ARGUMENT_REGISTER_BACKWARD



//-----------------------------------------------------------------------
// List of registers in backward order with offsets. MUST KEEP ALL LISTS IN SYNC.
//-----------------------------------------------------------------------

#ifndef DEFINE_ARGUMENT_REGISTER_BACKWARD_WITH_OFFSET
#define DEFINE_ARGUMENT_REGISTER_BACKWARD_WITH_OFFSET(regname,ofs)
#endif // !DEFINE_ARGUMENT_REGISTER_BACKWARD_WITH_OFFSET


DEFINE_ARGUMENT_REGISTER_BACKWARD_WITH_OFFSET(EDX,0)
DEFINE_ARGUMENT_REGISTER_BACKWARD_WITH_OFFSET(ECX,4)


#undef DEFINE_ARGUMENT_REGISTER_BACKWARD_WITH_OFFSET

#elif defined(_PPC_)

#ifndef NUM_ARGUMENT_REGISTERS
#define NUM_ARGUMENT_REGISTERS 8
#endif // !NUM_ARGUMENT_REGISTERS

#ifndef NUM_FLOAT_ARGUMENT_REGISTERS
#define NUM_FLOAT_ARGUMENT_REGISTERS 13
#endif // !NUM_FLOAT_ARGUMENT_REGISTERS

#ifndef NUM_CALLEESAVED_REGISTERS
#define NUM_CALLEESAVED_REGISTERS 19
#endif // !NUM_CALLEESAVED_REGISTERS

#ifndef NUM_FLOAT_CALLEESAVED_REGISTERS
#define NUM_FLOAT_CALLEESAVED_REGISTERS 18
#endif // !NUM_FLOAT_CALLEESAVED_REGISTERS

#else

PORTABILITY_WARNING("Platform calling convention not defined")

#ifndef NUM_ARGUMENT_REGISTERS
#define NUM_ARGUMENT_REGISTERS 1
#endif // !NUM_ARGUMENT_REGISTERS

#endif
