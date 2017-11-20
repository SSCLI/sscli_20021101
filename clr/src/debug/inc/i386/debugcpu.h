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
typedef BYTE INSTRUCTION_TYPE;

#define MAX_INSTRUCTION_LENGTH 4+2+1+1+4+4

#define CORDbg_BREAK_INSTRUCTION_SIZE 1
#define CORDbg_BREAK_INSTRUCTION (BYTE)0xCC

inline void CORDbgAdjustPCForBreakInstruction(CONTEXT* pContext)
{
    pContext->Eip -= CORDbg_BREAK_INSTRUCTION_SIZE;
}

//
// inline function to access/modify the CONTEXT
//
inline LPVOID CORDbgGetIP(CONTEXT *context) {
    return (LPVOID)(size_t)(context->Eip);
}

inline void CORDbgSetIP(CONTEXT *context, LPVOID eip) {
    context->Eip = (UINT32)(size_t)eip;
}

inline LPVOID CORDbgGetSP(CONTEXT *context) {
    return (LPVOID)(size_t)(context->Esp);
}

inline void CORDbgSetSP(CONTEXT *context, LPVOID esp) {
    context->Esp = (UINT32)(size_t)esp;
}

inline void CORDbgSetFP(CONTEXT *context, LPVOID ebp) {
    context->Ebp = (UINT32)(size_t)ebp;
}
inline LPVOID CORDbgGetFP(CONTEXT* context)
{
    return (LPVOID)(UINT_PTR)context->Ebp;
}

#endif // DEBUG_CPU_H
