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
// -*- C++ -*-
#ifndef _IFJITCOMPILER_H_
#define _IFJITCOMPILER_H_
#include "fjitencode.h"
/*****************************************************************************/

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                            FJit.h                                         XX
XX                                                                           XX
XX   The functionality needed for the FJIT DLL.                              XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/


class IFJitCompiler: public ICorJitCompiler {
public:
    virtual FJit_Encode* __stdcall getEncoder () = 0;
};

struct Fjit_hdrInfo
{
    size_t              methodSize;
    unsigned short      methodFrame;      /* includes all save regs and security obj, units are sizeof(void*) */
    unsigned short      methodArgsSize;   /* amount to pop off in epilog */
    unsigned short      methodJitGeneratedLocalsSize; /* number of jit generated locals in method */
    unsigned char       prologSize;
    unsigned char       epilogSize;
    bool                hasThis : 1;
    bool                hasRetBuff : 1;
    bool                savedIP : 1;
    bool		EnCMode : 1;	  /* has been compiled in EnC mode */
};

// Internal frame management
#define JIT_SIZE_EH_SLOT                     1
#define JIT_GENERATED_LOCAL_LOCALLOC_OFFSET  0  
#define JIT_GENERATED_LOCAL_NESTING_COUNTER (int)-1
#define JIT_GENERATED_LOCAL_FIRST_ESP       -2  // this - 2*[nestingcounter-1] = top of esp stack

#if defined(_X86_) 
#define MAX_ENREGISTERED 2
#endif

#ifdef _X86_
//describes the layout of the prolog frame in ascending address order
struct prolog_frame {
    unsigned nextFrame;
    unsigned returnAddress;
};

//describes the layout of the saved data in the prolog in ascending address order
struct prolog_data {
    unsigned security_obj;
    unsigned callee_saved_esi;
};

#define prolog_bias (0-sizeof(prolog_data))

/* tell where a register argument lives in the stack frame */
inline unsigned offsetOfRegister(unsigned regNum) {
    return(sizeof(prolog_frame) + (regNum)*sizeof(void*));
}

#define EnregReturnBuffer true      // Is 'return buffer' pointer eligble for being passed in a register
#define ReturnBufferFirst false     // The return buffer is passed after the this pointer   
#define RETURN_BUFF_OFFSET 0        // Only used if 'return buffer' is not enregistred
#define PASS_VALUETYPE_BYREF false  // If value types are passed as pointer to temporary copies
#define SIZE_LINKAGE_AREA  0        // There is not fixed callee cleaned linkage area

#define SIZE_STACK_SLOT      4

#endif //_X86_

#ifdef _PPC_

//describes the layout of the saved data in the prolog in ascending address order
struct prolog_data {
    unsigned security_obj;
    unsigned callee_saved_esi;
    unsigned nextFrame;
};

//describes the layout of the prolog frame in ascending address order
struct prolog_frame {
    unsigned callerSavedSP;
    unsigned returnAddress;
    unsigned calleeSavedCR;
    unsigned Reserved0;
    unsigned Reserved1;
    unsigned Reserved2;
};

#define prolog_bias         (0-sizeof(prolog_data))

/* tell where a register argument lives in the stack frame */
inline unsigned offsetOfRegister(unsigned regNum) {
    return(sizeof(prolog_frame) + regNum*sizeof(void*));
}

#define EnregReturnBuffer    true   // Is 'return buffer' pointer eligble for being passed in a register
#define ReturnBufferFirst    true   // The return buffer is passed before the this pointer  
#define RETURN_BUFF_OFFSET   0      // Only used if 'return buffer' is not enregistred
#define PASS_VALUETYPE_BYREF false  // If value types are passed as pointer to temporary copies
#define SIZE_LINKAGE_AREA    24        // There is fixed size callee cleaned linkage area

#define SIZE_STACK_SLOT      4

#endif //_PPC_



#endif //_IFJITCOMPILER_H_




