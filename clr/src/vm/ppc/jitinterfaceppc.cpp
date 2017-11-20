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
// ===========================================================================
// File: JITinterfacePPC.CPP
//
// ===========================================================================

// This contains JITinterface routines that are tailored for
// PPC platforms.

#include "common.h"
#include "jitinterface.h"
#include "eeconfig.h"
#include "excep.h"
#include "comstring.h"
#include "comdelegate.h"
#include "remoting.h" // create context bound and remote class instances
#include "field.h"
#include "tls.h"
#include "ecall.h"
#include "asmconstants.h"

// Compile-time checks that the offsets declared in clr/src/vm/ppc/asmconstants.h
// match the actual structure offsets.  If any of these lines generates a
// compile error:
//   An assertion failure results in error C2118: negative subscript.
//    or
//   size of array `__C_ASSERTxx__' is negative
// Then the offsets in asmconstants.h are out of sync with the C/C++ struct
C_ASSERT(offsetof(ArgumentRegisters, r) == ArgumentRegisters_r);
C_ASSERT(offsetof(ArgumentRegisters, f) == ArgumentRegisters_f);
C_ASSERT(sizeof(ArgumentRegisters) == ArgumentRegisters_size);

C_ASSERT(offsetof(CalleeSavedRegisters, cr) == CalleeSavedRegisters_cr);
C_ASSERT(offsetof(CalleeSavedRegisters, r) == CalleeSavedRegisters_r);
C_ASSERT(offsetof(CalleeSavedRegisters, f) == CalleeSavedRegisters_f);
C_ASSERT(sizeof(CalleeSavedRegisters) == CalleeSavedRegisters_size);

C_ASSERT(offsetof(EHContext,R) == EHContext_R);
C_ASSERT(offsetof(EHContext,F) == EHContext_F);
C_ASSERT(offsetof(EHContext,CR) == EHContext_CR);

class CheckAsmOffsets {
    CPP_ASSERT(1, offsetof(MachState, _Regs) == MachState__Regs);
    CPP_ASSERT(2, offsetof(MachState, _pRegs) == MachState__pRegs);
    CPP_ASSERT(3, offsetof(MachState, _sp) == MachState__sp);
    CPP_ASSERT(4, offsetof(MachState, _pRetAddr) == MachState__pRetAddr);

    CPP_ASSERT(5, offsetof(LazyMachState, captureSp) == LazyMachState_captureSp);
    CPP_ASSERT(6, offsetof(LazyMachState, captureSp2) == LazyMachState_captureSp2);
    CPP_ASSERT(7, offsetof(LazyMachState, capturePC) == LazyMachState_capturePC);

    CPP_ASSERT(7, offsetof(FaultingExceptionFrame, m_regs) == FaultingExceptionFrame_m_regs);
    CPP_ASSERT(8, offsetof(FaultingExceptionFrame, m_Link) == FaultingExceptionFrame_m_Link);
};

C_ASSERT(sizeof(MachState) == MachState_size);


/*********************************************************************/
// This is called by the JIT after every instruction in fully interuptable
// code to make certain our GC tracking is OK
HCIMPL0(VOID, JIT_StressGC_NOP) {}
HCIMPLEND


HCIMPL0(VOID, JIT_StressGC)
{
#ifdef _DEBUG
    HELPER_METHOD_FRAME_BEGIN_0();    // Set up a frame
    g_pGCHeap->GarbageCollect();
    HELPER_METHOD_FRAME_END();
#endif // _DEBUG
}
HCIMPLEND


/*********************************************************************/
// Initialize the part of the JIT helpers that require very little of
// EE infrastructure to be in place.
/*********************************************************************/
BOOL InitJITHelpers1()
{
    // Init GetThread function
    _ASSERTE(GetThread != NULL);
    hlpFuncTable[CORINFO_HELP_GET_THREAD].pfnHelper = (void *) GetThread;

    return TRUE;
}

/*********************************************************************/
// When a GC happens, the upper and lower bounds of the ephemeral
// generation change.  This routine updates the WriteBarrier thunks
// with the new values.
void StompWriteBarrierEphemeral() 
{
    // No smart write barriers on PPC
}

/*********************************************************************/
// When the GC heap grows, the ephemeral generation may no longer
// be after the older generations.  If this happens, we need to switch
// to the PostGrow thunk that checks both upper and lower bounds.
// regardless we need to update the thunk with the
// card_table - lowest_address.
void StompWriteBarrierResize(BOOL bReqUpperBoundsCheck)
{
    // No smart write barriers on PPC
}
