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

#ifndef DEBUG_CPU_H
#define DEBUG_CPU_H

// INSTRUCTION_TYPE, the Minimum instruction size used to set traps (int3, etc)
typedef DWORD INSTRUCTION_TYPE;

#define MAX_INSTRUCTION_LENGTH	sizeof(DWORD)

#define CORDbg_BREAK_INSTRUCTION_SIZE sizeof(DWORD)
#define CORDbg_BREAK_INSTRUCTION (DWORD)0x7ef00008

inline void CORDbgAdjustPCForBreakInstruction(CONTEXT* pContext)
{
    // The instruction is already stopped at the correct place
    return;
}

//
// inline function to access/modify the CONTEXT
//
inline LPVOID CORDbgGetIP(CONTEXT *context) {
    return (LPVOID)(size_t)context->Iar;
}

inline void CORDbgSetIP(CONTEXT *context, LPVOID eip) {
    context->Iar = (ULONG)(size_t)eip;
}

inline LPVOID CORDbgGetSP(CONTEXT *context) {
    return (LPVOID)(size_t)context->Gpr1;
}

inline void CORDbgSetSP(CONTEXT *context, LPVOID esp) {
    context->Gpr1 = (ULONG)(size_t)esp;
}

inline LPVOID CORDbgGetFP(PCONTEXT context) {
    return (LPVOID)(UINT_PTR)context->Gpr30;
}

inline void CORDbgSetFP(PCONTEXT context, LPVOID ebp) {
    context->Gpr30 = (ULONG)(size_t)ebp;
}


#endif // DEBUG_CPU_H
