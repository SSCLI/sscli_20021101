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
// CGENPPC.CPP -
//
// Various helper routines for generating ppc assembly code.
//
//

// Precompiled Header
#include "common.h"

#include "field.h"
#include "frames.h"
#include "stublink.h"
#include "cgensys.h"
#include "tls.h"
#include "frames.h"
#include "excep.h"
#include "ndirect.h"
#include "log.h"
#include "security.h"
#include "comdelegate.h"
#include "array.h"
#include "jitinterface.h"
#include "codeman.h"
#include "ejitmgr.h"
#include "remoting.h"
#include "dbginterface.h"
#include "eeprofinterfaces.h"
#include "eeconfig.h"
#include "comobject.h"
#include "asmconstants.h"

// Prevent multiple threads from simultaneous interlocked in/decrementing
extern "C" {
  UINT	iSpinLock = 0;
}

DWORD __stdcall GetSpecificCpuType()
{
    return 0;   // just indicate this is a non-x86 cpu
}

//-----------------------------------------------------------------------
// InstructionFormat for call.
//-----------------------------------------------------------------------
class PPCCall : public InstructionFormat
{
    public:
        PPCCall(UINT allowedSizes) : InstructionFormat(allowedSizes)
        {}

        virtual UINT GetSizeOfInstruction(UINT refsize, UINT variationCode)
        {
            return 16;
        }

        virtual VOID EmitInstruction(UINT refsize, __int64 fixedUpReference, BYTE *pOutBuffer, UINT variationCode, BYTE *pDataBuffer)
        {
            // addis r12, 0, (value>>16)
            *(__int32*)(pOutBuffer+0) = 0x3D800000 | (UINT16)(fixedUpReference >> 16);

            // ori r12,r12,value
            *(__int32*)(pOutBuffer+4) = 0x618C0000 | (UINT16)(fixedUpReference);

            // mtctr r12
            *(__int32*)(pOutBuffer+8) = 0x7D8903A6;

            // bctrl
            *(__int32*)(pOutBuffer+12) = 0x4E800421;
        }
};

//-----------------------------------------------------------------------
// InstructionFormat for branch.
//-----------------------------------------------------------------------
class PPCBranch : public InstructionFormat
{
    public:
        PPCBranch(UINT allowedSizes) : InstructionFormat(allowedSizes)
        {}

        virtual UINT GetSizeOfInstruction(UINT refsize, UINT variationCode)
        {
            return (refsize == InstructionFormat::k16) ? 4 : 8;
        }

        virtual VOID EmitInstruction(UINT refsize, __int64 fixedUpReference, BYTE *pOutBuffer, UINT variationCode, BYTE *pDataBuffer)
        {
            // bcx ...
            _ASSERTE((fixedUpReference & 3) == 0);
            if (FitsInI2(fixedUpReference))
                *(__int32*)pOutBuffer = 0x40000000 | variationCode | (UINT16)fixedUpReference;
            else {
                // Stublinker calls CanReach to make sure this doesn't happen
                _ASSERTE(FitsInI2(fixedUpReference >> 10));

                if (variationCode == PPCCondCode::kAlways)
                    *(__int32*)pOutBuffer = 0x60000000; // nop
                else
                    *(__int32*)pOutBuffer = 0x40000000 | (variationCode ^ 0x1000000) | 8; // if (!cond) skip the following instruction

                *(__int32*)(pOutBuffer + 4) = 0x48000000 | ((unsigned)fixedUpReference & 0x03ffffff);
            }
        }

        virtual UINT GetHotSpotOffset(UINT refsize, UINT variationCode)
        {
            // On PPC, offset is measured from address of branch instruction
            // Adjust if refsize is 26 bits and we're emitting two instructions
            return (refsize == InstructionFormat::k16) ? 0 : 4;
        }
};

static BYTE gPPCCall[sizeof(PPCCall)];
static BYTE gPPCBranch[sizeof(PPCCall)];

/* static */ BOOL StubLinkerCPU::Init()
{
    new (gPPCCall) PPCCall(InstructionFormat::k32);
    new (gPPCBranch) PPCBranch(InstructionFormat::k16 | InstructionFormat::k26);
    return TRUE;
}

//---------------------------------------------------------------
// helper for emitting instructions with reg, reg, disp16 format
//---------------------------------------------------------------
VOID StubLinkerCPU::PPCEmitRegRegDisp16(UINT32 code, PPCReg r1, PPCReg r2, int disp16) {
    _ASSERTE((code & 0xFC000000) == code);
    if (!FitsInI2(disp16))
        COMPlusThrow(kTypeLoadException, IDS_EE_SIGTOOCOMPLEX);
    Emit32(code | (r1 << 21) | (r2 << 16) | (UINT16)disp16);
}

//---------------------------------------------------------------
// helper for emitting instructions with reg, reg, reg format
//---------------------------------------------------------------
VOID StubLinkerCPU::PPCEmitRegRegReg(UINT32 code, PPCReg r1, PPCReg r2, PPCReg r3) {
    _ASSERTE((code & 0xFC0003FF) == code);
    Emit32(code | (r1 << 21) | (r2 << 16) | (r3 << 11));
}

//---------------------------------------------------------------
// emit: rD = *(rA+d)
//---------------------------------------------------------------
VOID StubLinkerCPU::PPCEmitLwz(PPCReg rD, PPCReg rA, int d /*=0*/) {
    PPCEmitRegRegDisp16(0x80000000, rD, rA, d);
}

//---------------------------------------------------------------
// emit: rD = rA+d
//---------------------------------------------------------------
VOID StubLinkerCPU::PPCEmitAddi(PPCReg rD, PPCReg rA, int d /*=0*/) {
    _ASSERTE(rA != kR0); // addi uses the value 0, not the contents of GPR0, if rA = 0
    PPCEmitRegRegDisp16(0x38000000, rD, rA, d);
}

//---------------------------------------------------------------
// emit: cr = (rA == d)
//---------------------------------------------------------------
VOID StubLinkerCPU::PPCEmitCmpwi(PPCReg rA, int d /* = 0*/) {
    PPCEmitRegRegDisp16(0x2C000000, (PPCReg)0, rA, d);
}

//---------------------------------------------------------------
// emit: cr = (rA == rB)
//---------------------------------------------------------------
VOID StubLinkerCPU::PPCEmitCmpw(PPCReg rA, PPCReg rB) {
    PPCEmitRegRegReg(0x7C000000, (PPCReg)0, rA, rB);
}

//---------------------------------------------------------------
// emit: *(rA+d) = rS
//---------------------------------------------------------------
VOID StubLinkerCPU::PPCEmitStw(PPCReg rS, PPCReg rA, int d /*=0*/) {
    PPCEmitRegRegDisp16(0x90000000, rS, rA, d);
}

//---------------------------------------------------------------
// emit: rA = rS
//---------------------------------------------------------------
VOID StubLinkerCPU::PPCEmitMr(PPCReg rA, PPCReg rS) {
    PPCEmitRegRegReg(0x7C000378, rS, rA, rS);
}

//---------------------------------------------------------------
// emit: rD = value
//---------------------------------------------------------------
VOID StubLinkerCPU::PPCEmitLoadImm(PPCReg rD, INT value)
{
    if (FitsInI2(value)) {
        // addi rD, 0, value
        PPCEmitRegRegDisp16(0x38000000, rD, (PPCReg)0, value);
    }
    else
    {
        // addis rD, 0, (value>>16)
        PPCEmitRegRegDisp16(0x3C000000, rD, (PPCReg)0, value>>16);

        if ((INT16)value != 0) {
            // ori r,r,value
            PPCEmitRegRegDisp16(0x60000000, rD, rD, (INT16)value);
        }
    }
}

//---------------------------------------------------------------
// emit: rD = value
// special flavor of LoadImm for stubs that have to have predictable layout
//---------------------------------------------------------------
VOID StubLinkerCPU::PPCEmitLoadImmNonoptimized(PPCReg rD, INT value)
{
    // addis rD, 0, (value>>16)
    PPCEmitRegRegDisp16(0x3C000000, rD, (PPCReg)0, value>>16);

    // ori r,r,value
    PPCEmitRegRegDisp16(0x60000000, rD, rD, (INT16)value);
}

//---------------------------------------------------------------
// emit: call
//---------------------------------------------------------------
VOID StubLinkerCPU::PPCEmitCall(CodeLabel *target)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(!target->m_fAbsolute);
    target->m_fAbsolute = TRUE;

    EmitLabelRef(target, reinterpret_cast<PPCCall&>(gPPCCall), 0);
}

//---------------------------------------------------------------
// emit: call
//---------------------------------------------------------------
VOID StubLinkerCPU::PPCEmitBranch(CodeLabel *target, PPCCondCode::cc cond /*= PPCCondCode::kAlways*/)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(!target->m_fAbsolute);
    _ASSERTE(!target->m_fExternal);

    EmitLabelRef(target, reinterpret_cast<PPCBranch&>(gPPCBranch), cond);
}

//---------------------------------------------------------------
// emit: indirect branch
//---------------------------------------------------------------
VOID StubLinkerCPU::PPCEmitBranchR12()
{
    // mtctr r12
    Emit32(0x7D8903A6);
    // bctr
    Emit32(0x4E800420);
}

//---------------------------------------------------------------
// emit: indirect call
//---------------------------------------------------------------
VOID StubLinkerCPU::PPCEmitCallR12()
{
    // mtctr r12
    Emit32(0x7D8903A6);
    // bctr
    Emit32(0x4E800421);
}

//---------------------------------------------------------------
// emit: rD = GetThread()
//---------------------------------------------------------------
VOID StubLinkerCPU::PPCEmitCurrentThreadFetch(PPCReg rD)
{
    PPCEmitCall(NewExternalCodeLabel((LPVOID) GetThread));

    if (rD != kR3) {
        PPCEmitMr(rD, kR3);
    }
}

//---------------------------------------------------------------
// emit: *(ArgumentRegisters*)(basereg + ofs) = <machine state>
//---------------------------------------------------------------
VOID StubLinkerCPU::EmitArgumentRegsSave(PPCReg basereg, int ofs)
{
    int i;

    for (i = 0; i < NUM_ARGUMENT_REGISTERS; i++) {
        PPCEmitStw((PPCReg)(kR3 + i), basereg,
            ofs + offsetof(ArgumentRegisters, r[i]));
    }

    for (i = 0; i < NUM_FLOAT_ARGUMENT_REGISTERS; i++) {
        // stfd
        PPCEmitRegRegDisp16(0xD8000000, (PPCReg)(1 /*FPR1*/ + i), basereg,
            ofs + offsetof(ArgumentRegisters, f[i]));
    }
}

//---------------------------------------------------------------
// emit: <machine state> = *(ArgumentRegisters*)(basereg + ofs)
//---------------------------------------------------------------
VOID StubLinkerCPU::EmitArgumentRegsRestore(PPCReg basereg, int ofs)
{
    int i;

    for (i = 0; i < NUM_ARGUMENT_REGISTERS; i++) {
        PPCEmitLwz((PPCReg)(kR3 + i), basereg,
            ofs + offsetof(ArgumentRegisters, r[i]));
    }

    for (i = 0; i < NUM_FLOAT_ARGUMENT_REGISTERS; i++) {
        // lfd
        PPCEmitRegRegDisp16(0xC8000000, (PPCReg)(1 /*FPR1*/ + i), basereg,
            ofs + offsetof(ArgumentRegisters, f[i]));
    }
}


//---------------------------------------------------------------
// emit: *(CalleeSavedRegisters*)(basereg + ofs) = <machine state>
//---------------------------------------------------------------
VOID StubLinkerCPU::EmitCalleeSavedRegsSave(PPCReg basereg, int ofs)
{
    int i;

    // mfcr r0
    Emit32(0x7C000026);
    // stw r0, d(basereg)
    PPCEmitStw(kR0, kR1, ofs + offsetof(CalleeSavedRegisters, cr));

    // stmw r13, d(basereg)
    PPCEmitRegRegDisp16(0xBC000000, kR13, basereg,
        ofs + offsetof(CalleeSavedRegisters, r[0]));

    for (i = 0; i < NUM_FLOAT_CALLEESAVED_REGISTERS; i++) {
        // stfd
        PPCEmitRegRegDisp16(0xD8000000, (PPCReg)(14 /*FPR14*/ + i), basereg,
            ofs + offsetof(CalleeSavedRegisters, f[i]));
    }
}

//---------------------------------------------------------------
// emit: <machine state> = *(CalleeSavedRegisters*)(basereg + ofs)
//---------------------------------------------------------------
VOID StubLinkerCPU::EmitCalleeSavedRegsRestore(PPCReg basereg, int ofs)
{
    int i;

    // lwz r0, d(basereg)
    PPCEmitLwz(kR0, kR1, ofs + offsetof(CalleeSavedRegisters, cr));
    // mtcr r0
    Emit32(0x7C0FF120);

    // lmw r13, d(basereg)
    PPCEmitRegRegDisp16(0xB8000000, kR13, basereg,
        ofs + offsetof(CalleeSavedRegisters, r[0]));

    for (i = 0; i < NUM_FLOAT_CALLEESAVED_REGISTERS; i++) {
        // lfd
        PPCEmitRegRegDisp16(0xC8000000, (PPCReg)(14 /*FPR14*/ + i), basereg,
            ofs + offsetof(CalleeSavedRegisters, f[i]));
    }
}

//---------------------------------------------------------------
// Helper for enregistering/deregistering arguments
//---------------------------------------------------------------
void FramedMethodFrame::RegisterHelper(FramedMethodFrame* pThis, BOOL fEnregister)
{
    PCCOR_SIGNATURE pCallSig;
    MethodDesc   *pFunction;

    // This code - including the somehow fragile
    // returning on NULL - came from FramedMethodFrame::PromoteCallerStackWorker

    // We're going to have to look at the signature to determine
    // which arguments are in registers
    pFunction = pThis->GetFunction();
    if (! pFunction)
        return;

    // Now get the signature...
    pCallSig = pFunction->GetSig();
    if (! pCallSig)
        return;

    MetaSig msig(pCallSig,pFunction->GetModule());
    MetaSig* pSig = &msig;

#ifdef _DEBUG
    PVOID pRetAddr = pThis->GetReturnAddress();
    if (pRetAddr != 0) {
        // Verify that all callsites that are passing return buffer
        // have the "oris r0, r0, 0" marker
        _ASSERTE((*(DWORD*)pRetAddr == 0x64000000) == !!pSig->HasRetBuffArg());
    }
#endif

    BOOL fIsStatic = !pSig->HasThis();

    RegisterWorker((LPBYTE)pThis, pSig, fIsStatic,
        pSig->SizeOfActualFixedArgStack(fIsStatic), fEnregister);
}

//---------------------------------------------------------------
// Helper for enregistering/deregistering arguments
//---------------------------------------------------------------
/* static */ void FramedMethodFrame::RegisterWorker(BYTE* pFrameBase, MetaSig* pSig, BOOL fIsStatic,
    UINT nActualStackBytes, BOOL fEnregister)
{
    _ASSERT(nActualStackBytes == pSig->SizeOfActualFixedArgStack(fIsStatic));

    BYTE* pStackArgs = pFrameBase + sizeof(FramedMethodFrame);
    ArgumentRegisters* pRegArgs = (ArgumentRegisters*)(pFrameBase + GetOffsetOfArgumentRegisters());

#ifdef _DEBUG
    if (fEnregister) {
        memset(pRegArgs, 0xCC, sizeof(ArgumentRegisters));
    }
#endif

    if (pSig->GetCallingConvention() == IMAGE_CEE_CS_CALLCONV_VARARG)
        return;

    UINT nRegisterBytes = min(nActualStackBytes, sizeof(INT32) * NUM_ARGUMENT_REGISTERS);

    if (fEnregister) {
        memcpy(&pRegArgs->r[0], pStackArgs, nRegisterBytes);
    }
    else {
        memcpy(pStackArgs, &pRegArgs->r[0], nRegisterBytes);
    }

    ArgIterator argit(pFrameBase, pSig, fIsStatic);

    BYTE   typ;
    UINT32 structSize;
    int    ofs;

    int    floatArg = 0;

    for( ; 0 != (ofs = argit.GetNextOffsetFaster(&typ, &structSize)); )
    {
        switch (typ)
        {
        case ELEMENT_TYPE_R4:
            if (fEnregister)
                pRegArgs->f[floatArg] = (DOUBLE)*(FLOAT*)(pFrameBase + ofs);
            else
                *(FLOAT*)(pFrameBase + ofs) = (FLOAT)pRegArgs->f[floatArg];
            break;

        case ELEMENT_TYPE_R8:
            if (fEnregister)
                pRegArgs->f[floatArg] = *(DOUBLE*)(pFrameBase + ofs);
            else
                *(DOUBLE*)(pFrameBase + ofs) = pRegArgs->f[floatArg];
            break;

        default:
            continue;
        }

        if (++floatArg >= NUM_FLOAT_ARGUMENT_REGISTERS)
            break;
    }

    // avoid setting float registers to bogus values
    if (fEnregister && (floatArg < NUM_FLOAT_ARGUMENT_REGISTERS)) {
        memset(&pRegArgs->f[floatArg], 0, NUM_FLOAT_ARGUMENT_REGISTERS - floatArg);
    }

#ifdef _DEBUG
    if (!fEnregister) {
        memset(pRegArgs, 0xCC, sizeof(ArgumentRegisters));
    }
#endif
}

/*
    This method is dependent on the StubProlog, therefore it's implementation
    is done right next to it.
*/
void FramedMethodFrame::UpdateRegDisplay(const PREGDISPLAY pRD)
{
    CalleeSavedRegisters* regs = GetCalleeSavedRegisters();
    int i;

    // reset pContext; it's only valid for active (top-most) frame
    pRD->pContext = NULL;

    pRD->CR  = regs->cr;

    for (i = 0; i < NUM_CALLEESAVED_REGISTERS; i++) {
        pRD->pR[i] = (DWORD*) &(regs->r[i]);
    }

    for (i = 0; i < NUM_FLOAT_CALLEESAVED_REGISTERS; i++) {
        pRD->pF[i] = (DOUBLE*) &(regs->f[i]);
    }

    pRD->pPC  = (SLOT*)GetReturnAddressPtr();
    pRD->SP   = (DWORD)(size_t)&m_Link;
}

void HelperMethodFrame::UpdateRegDisplay(const PREGDISPLAY pRD)
{
    _ASSERTE(m_MachState->isValid());               // InsureInit has been called

    // reset pContext; it's only valid for active (top-most) frame
    pRD->pContext = NULL;

    for (int i=0; i<NUM_CALLEESAVED_REGISTERS; ++i) {
        pRD->pR[i] = (DWORD*)m_MachState->_pRegs[i];
    }

    // Strictly speaking, we should be restoring the floating point registers.
    // Fortunately, we don't have to do this because of:

    // - there is always at FramedMethodFrame on the interop boundary that will
    // eventually restore the registers when comming from foreign code. Notice that
    // this is important only if somebody decides to mix unmanaged floating point
    // exceptions with interop of PAL SEH
    // - float JIT helpers do not use helper frames

    pRD->CR = m_MachState->_cr;

    pRD->pPC = (SLOT*)m_MachState->pRetAddr();
    pRD->SP = (DWORD)(size_t)m_MachState->sp();
}

void FaultingExceptionFrame::UpdateRegDisplay(const PREGDISPLAY pRD)
{
    // NOTE: There's a version in tools\sos\tst-stackwalk.cpp that
    // should be kept up to date
    CalleeSavedRegisters* regs = GetCalleeSavedRegisters();
    int i;

    pRD->CR  = regs->cr;

    for (i = 0; i < NUM_CALLEESAVED_REGISTERS; i++) {
        pRD->pR[i] = (DWORD*) &(regs->r[i]);
    }

    for (i = 0; i < NUM_FLOAT_CALLEESAVED_REGISTERS; i++) {
        pRD->pF[i] = (DOUBLE*) &(regs->f[i]);
    }

    pRD->pPC  = (SLOT*) GetReturnAddressPtr();
    pRD->SP   = (DWORD)(size_t)m_Link.SavedSP;
}

void UnmanagedToManagedFrame::UpdateRegDisplay(const PREGDISPLAY pRD)
{
    // NOTE: There's a version in tools\sos\tst-stackwalk.cpp that
    // should be kept up to date
    CalleeSavedRegisters *regs = (CalleeSavedRegisters*)((BYTE*)this - sizeof(CalleeSavedRegisters));
    int i;

    // reset pContext; it's only valid for active (top-most) frame

    pRD->pContext = NULL;

    pRD->CR  = regs->cr;

    for (i = 0; i < NUM_CALLEESAVED_REGISTERS; i++) {
        pRD->pR[i] = (DWORD*) &(regs->r[i]);
    }

    for (i = 0; i < NUM_FLOAT_CALLEESAVED_REGISTERS; i++) {
        pRD->pF[i] = (DOUBLE*) &(regs->f[i]);
    }

    pRD->pPC  = (SLOT*)GetReturnAddressPtr();;
    pRD->SP   = (DWORD)(size_t)(&m_Link);
}

extern "C" Thread* __stdcall CreateThreadBlock()
{
    return SetupThread();
}

extern "C" VOID __stdcall StubEnableWorker(Thread *pThread)
{
    pThread->m_fPreemptiveGCDisabled = 0;

    if (!pThread->CatchAtSafePoint())
        return;

    pThread->RareEnablePreemptiveGC();
}


// I would prefer to define a unique HRESULT in our own facility, but we aren't
// supposed to create new HRESULTs this close to ship
#define E_PROCESS_SHUTDOWN_REENTRY    HRESULT_FROM_WIN32(ERROR_PROCESS_ABORTED)


// Disable when calling into managed code from a place that fails via Exceptions
extern "C" VOID __stdcall StubDisableTHROWWorker(Thread *pThread, Frame *pFrame)
{
    pThread->m_fPreemptiveGCDisabled = 1;

    if (!g_TrapReturningThreads) 
        return;

    // WARNING!!!!
    // when we start executing here, we are actually in cooperative mode.  But we
    // haven't synchronized with the barrier to reentry yet.  So we are in a highly
    // dangerous mode.  If we call managed code, we will potentially be active in
    // the GC heap, even as GC's are occuring!

    // Do not add THROWSCOMPLUSEXCEPTION() here.  We haven't set up SEH.  We rely
    // on HandleThreadAbort and COMPlusThrowBoot dealing with this situation properly.

#ifdef DEBUGGING_SUPPORTED
    // If the debugger is attached, we use this opprotunity to see if
    // we're disabling preemptive GC on the way into the runtime from
    // unmanaged code. We end up here because
    // Increment/DecrementTraceCallCount() will bump
    // g_TrapReturningThreads for us.
    if (CORDebuggerTraceCall())
        g_pDebugInterface->PossibleTraceCall(NULL, pFrame);
#endif // DEBUGGING_SUPPORTED

    // Check for ShutDown scenario.  This happens only when we have initiated shutdown
    // and someone is trying to call in after the CLR is suspended.  In that case, we
    // must either raise an unmanaged exception or return an HRESULT, depending on the
    // expectations of our caller.
    if (!CanRunManagedCode())
    {
        // DO NOT IMPROVE THIS EXCEPTION!  It cannot be a managed exception.  It
        // cannot be a real exception object because we cannot execute any managed
        // code here.
        pThread->m_fPreemptiveGCDisabled = 0;
        COMPlusThrowBoot(E_PROCESS_SHUTDOWN_REENTRY);
    }

    // We must do the following in this order, because otherwise we would be constructing
    // the exception for the abort without synchronizing with the GC.  Also, we have no
    // CLR SEH set up, despite the fact that we may throw a ThreadAbortException.
    pThread->RareDisablePreemptiveGC();
    pThread->HandleThreadAbort();
}

//========================================================================
//  void StubLinkerCPU::EmitComMethodStubProlog()
//  Prolog for entering managed code from COM
//  pushes the appropriate frame ptr
//  sets up a thread and returns a label that needs to be emitted by the caller
void StubLinkerCPU::EmitComMethodStubProlog(LPVOID pFrameVptr,
                                            LPVOID pSEHHandler,
                                            BOOL bShouldProfile)
{
    // r11 = UMEntryThunk.m_code

    // get the address of the UMEntryThunk
    PPCEmitAddi(kR11, kR11, -UMEntryThunk::GetOffsetOfCode());

    // mflr r0
    Emit32(0x7C0802A6);

    // stw r0, 8(r1)
    PPCEmitStw(kR0, kR1, 8);
    // stwu r1, -local_space(r1)
    INT frameSize = sizeof(UMStubStackFrame)-sizeof(LinkageArea);
    _ASSERTE(AlignStack(frameSize) == frameSize);
    PPCEmitRegRegDisp16(0x94000000, kR1, kR1, -frameSize);

    // store the datum in the frame
    PPCEmitStw(kR11, kR1,
        offsetof(UMStubStackFrame, umframe) + UnmanagedToManagedFrame::GetOffsetOfDatum());

    // store all arguments registers
    EmitArgumentRegsSave(kR1, offsetof(UMStubStackFrame, argregs));
    // store all callee saved registers
    EmitCalleeSavedRegsSave(kR1, offsetof(UMStubStackFrame, savedregs));

    // setup the vtbl
    PPCEmitLoadImm(kR3, (INT)(size_t)pFrameVptr);
    PPCEmitStw(kR3, kR1, offsetof(UMStubStackFrame, umframe));
    
    // clear auxilary information
    PPCEmitLoadImm(kR0, 0);

    PPCEmitStw(kR0, kR1, offsetof(UMStubStackFrame, info.m_List));
    PPCEmitStw(kR0, kR1, offsetof(UMStubStackFrame, info.m_pReturnContext));
    PPCEmitStw(kR0, kR1, offsetof(UMStubStackFrame, info.m_pArgs));
    PPCEmitStw(kR0, kR1, offsetof(UMStubStackFrame, info.m_fGCEnabled));
    PPCEmitStw(kR0, kR1, offsetof(UMStubStackFrame, info.m____NOLONGERUSED____));

    // Emit Setup thread
    PPCEmitCall(NewExternalCodeLabel((LPVOID) CreateThreadBlock));
    PPCEmitMr(kR31, kR3);

    //-----------------------------------------------------------------------
    // Generate the inline part of disabling preemptive GC.  It is critical
    // that this part happen before we link in the frame.  That's because
    // we won't be able to unlink the frame from preemptive mode.  And during
    // shutdown, we cannot switch to cooperative mode under some circumstances
    //-----------------------------------------------------------------------

    // r3 = pThread

    // r4 = pFrame
    PPCEmitAddi(kR4, kR1, offsetof(UMStubStackFrame, umframe));

    // call VOID StubDisableTHROWWorker(Thread *pThread, Frame *pFrame)
    PPCEmitCall(NewExternalCodeLabel((LPVOID) StubDisableTHROWWorker));

    // kR3 = new frame
    PPCEmitAddi(kR3, kR1, offsetof(UMStubStackFrame, umframe));
    // kR30 = previous frame
    PPCEmitLwz(kR30, kR31, Thread::GetOffsetOfCurrentFrame());
    // frame.m_pNext = previous frame
    PPCEmitStw(kR30, kR1, offsetof(UMStubStackFrame, umframe) + Frame::GetOffsetOfNextLink());
    // thread.m_pFrame = frame
    PPCEmitStw(kR3, kR31, Thread::GetOffsetOfCurrentFrame());

#if _DEBUG
    // call LogTransition
    PPCEmitAddi(kR3, kR1, offsetof(UMStubStackFrame, umframe));
    PPCEmitCall(NewExternalCodeLabel((LPVOID) Frame::LogTransition));
#endif

    if (pSEHHandler)
    {
        // setup the SEH handler
        PPCEmitLoadImm(kR4, (INT32)(size_t)pSEHHandler);
        PPCEmitStw(kR4, kR1, offsetof(UMStubStackFrame, exReg.Handler));

        PPCEmitAddi(kR3, kR1, offsetof(UMStubStackFrame, exReg));
        PPCEmitStw(kR3, kR1, offsetof(UMStubStackFrame, exReg.pvFilterParameter));

        PPCEmitLoadImm(kR4, PAL_EXCEPTION_FLAGS_UNWINDONLY);
        PPCEmitStw(kR4, kR1, offsetof(UMStubStackFrame, exReg.dwFlags));

        // call PAL_TryHelper
        PPCEmitCall(NewExternalCodeLabel((LPVOID) PAL_TryHelper));
    }

#ifdef PROFILING_SUPPORTED
    // If profiling is active, emit code to notify profiler of transition
    // Must do this before preemptive GC is disabled, so no problem if the
    // profiler blocks.
    if (CORProfilerTrackTransitions() && bShouldProfile)
    {
        EmitProfilerComCallProlog(pFrameVptr);
    }
#endif // PROFILING_SUPPORTED
}


//========================================================================
//  Epilog for stubs that enter managed code from COM
//
void StubLinkerCPU::EmitComMethodStubEpilog(LPVOID pFrameVptr,                                           
                                            LPVOID pSEHHandler,
                                            BOOL bShouldProfile,
                                            BOOL bReloop)
{
    // r31 is pThread
    // r30 is previous frame

    if (!bReloop) {
        // store the return value in argregs during epilog
        PPCEmitStw(kR3, kR1, offsetof(UMStubStackFrame, argregs.r[kR3 - kR3]));
        PPCEmitStw(kR4, kR1, offsetof(UMStubStackFrame, argregs.r[kR4 - kR3]));
        
        // stfd f1
        PPCEmitRegRegDisp16(0xD8000000, (PPCReg)1, kR1, offsetof(UMStubStackFrame, argregs.f[0]));
    }

    if (pSEHHandler) {

        // unlink SEH
        PPCEmitAddi(kR3, kR1, offsetof(UMStubStackFrame, exReg));
        PPCEmitLoadImm(kR4, 0);

        // call PAL_EndTryHelper
        PPCEmitCall(NewExternalCodeLabel((LPVOID) PAL_EndTryHelper));
    }

    // restore previous frame
    PPCEmitStw(kR30, kR31, Thread::GetOffsetOfCurrentFrame());

    //-----------------------------------------------------------------------
    // Generate the inline part of disabling preemptive GC
    //-----------------------------------------------------------------------
    // r3 = pThread
    PPCEmitMr(kR3, kR31);

    // call VOID StubEnableWorker(Thread *pThread)
    PPCEmitCall(NewExternalCodeLabel((LPVOID) StubEnableWorker));

#ifdef PROFILING_SUPPORTED
    // If profiling is active, emit code to notify profiler of transition
    if (CORProfilerTrackTransitions() && bShouldProfile)
    {
        EmitProfilerComCallEpilog(pFrameVptr);
    }
#endif // PROFILING_SUPPORTED

    EmitCalleeSavedRegsRestore(kR1, offsetof(UMStubStackFrame, savedregs));    

    if (bReloop) {
        EmitArgumentRegsRestore(kR1, offsetof(UMStubStackFrame, argregs));

        // get the reloop address
        PPCEmitLwz(kR12, kR1, offsetof(UMStubStackFrame, umframe) + UnmanagedToManagedFrame::GetOffsetOfDatum());
        PPCEmitAddi(kR12, kR12, UMEntryThunk::GetOffsetOfCode());

        // restore the stack
        // lwz r1, 0(r1)
        PPCEmitLwz(kR1, kR1, 0);

        // restore the link register
        // lwz r0, 8(r1)
        PPCEmitLwz(kR0, kR1, 8);

        // reloop
        PPCEmitBranchR12();
    }
    else {
        // restore the return value
        PPCEmitLwz(kR3, kR1, offsetof(UMStubStackFrame, argregs.r[kR3 - kR3]));
        PPCEmitLwz(kR4, kR1, offsetof(UMStubStackFrame, argregs.r[kR4 - kR3]));

        // lfd f1
        PPCEmitRegRegDisp16(0xC8000000, (PPCReg)1, kR1, offsetof(UMStubStackFrame, argregs.f[0]));

        // restore the stack
        // lwz r1, 0(r1)
        PPCEmitLwz(kR1, kR1, 0);

        // restore the link register
        // lwz r0, 8(r1)
        PPCEmitLwz(kR0, kR1, 8);
        // mtlr r0
        Emit32(0x7C0803A6);
        
        // blr
        Emit32(0x4E800020);
    }
}

/*
    If you make any changes to the prolog instruction sequence, be sure
    to update UpdateRegdisplay, too!!  This service should only be called from
    within the runtime.  It should not be called for any unmanaged -> managed calls in.
*/
VOID StubLinkerCPU::EmitMethodStubProlog(LPVOID pFrameVptr, BOOL fDeregister /*=TRUE*/)
{
    THROWSCOMPLUSEXCEPTION();

    // lr = original return address
    // r11 = prestub instruction address

    if (fDeregister) {
        // get the real address of the methoddesc
        PPCEmitAddi(kR11, kR11, METHOD_CALL_PRESTUB_SIZE);
    }

    // mflr r0
    Emit32(0x7C0802A6);

    // stw r0, 8(r1)
    PPCEmitStw(kR0, kR1, 8);
    // stwu r1, -local_space(r1)
    INT frameSize = sizeof(StubStackFrame)-sizeof(LinkageArea);
    _ASSERTE(AlignStack(frameSize) == frameSize);
    PPCEmitRegRegDisp16(0x94000000, kR1, kR1, -frameSize);

    // store the datum in the frame
    PPCEmitStw(kR11, kR1,
        offsetof(StubStackFrame, methodframe) + FramedMethodFrame::GetOffsetOfMethod());

    // store all arguments registers
    EmitArgumentRegsSave(kR1, offsetof(StubStackFrame, argregs));
    // store all callee saved registers
    EmitCalleeSavedRegsSave(kR1, offsetof(StubStackFrame, savedregs));

    // setup the vtbl
    PPCEmitLoadImm(kR3, (INT)(size_t)pFrameVptr);
    PPCEmitStw(kR3, kR1, offsetof(StubStackFrame, methodframe));

    // kR31 <-- GetThread()
    PPCEmitCurrentThreadFetch(kR31);

    if (fDeregister) {
        // spill all the registers onto the stack
        // kR3 = new frame
        PPCEmitAddi(kR3, kR1, offsetof(StubStackFrame, methodframe));
        PPCEmitLoadImm(kR4, 0); // deregister
        PPCEmitCall(NewExternalCodeLabel((LPVOID) FramedMethodFrame::RegisterHelper));
    }

#if _DEBUG
    // call ObjectRefFlush
    PPCEmitMr(kR3, kR31);
    PPCEmitCall(NewExternalCodeLabel((LPVOID) Thread::ObjectRefFlush));
#endif

    // kR3 = new frame
    PPCEmitAddi(kR3, kR1, offsetof(StubStackFrame, methodframe));
    // kR30 = previous frame
    PPCEmitLwz(kR30, kR31, Thread::GetOffsetOfCurrentFrame());
    // frame.m_pNext = previous frame
    PPCEmitStw(kR30, kR1, offsetof(StubStackFrame, methodframe) + Frame::GetOffsetOfNextLink());
    // thread.m_pFrame = frame
    PPCEmitStw(kR3, kR31, Thread::GetOffsetOfCurrentFrame());

#if _DEBUG
    // call LogTransition
    PPCEmitAddi(kR3, kR1, offsetof(StubStackFrame, methodframe));
    PPCEmitCall(NewExternalCodeLabel((LPVOID) Frame::LogTransition));
#endif

    // OK for the debugger to examine the new frame now
    // (Note that if it's not OK yet for some stub, another patch label
    // can be emitted later which will override this one.)
    EmitPatchLabel();

    // r31 is pThread
    // r30 is previous frame
}

VOID StubLinkerCPU::EmitMethodStubEpilog(StubStyle style)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(style == kNoTripStubStyle ||
             style == kInterceptorStubStyle);        // the only ones this code knows about.

    // restore previous frame
    PPCEmitStw(kR30, kR31, Thread::GetOffsetOfCurrentFrame());

    // restore callee saved registers
    EmitCalleeSavedRegsRestore(kR1, offsetof(StubStackFrame, savedregs));

    // restore the stack
    // lwz r1, 0(r1)
    PPCEmitLwz(kR1, kR1, 0);

    // restore the link register
    // lwz r0, 8(r1)
    PPCEmitLwz(kR0, kR1, 8);
    // mtlr r0
    Emit32(0x7C0803A6);

    if(style != kInterceptorStubStyle) {
        // blr
        Emit32(0x4E800020);
    }
}

// This method unboxes the THIS pointer and then calls pRealMD
VOID StubLinkerCPU::EmitUnboxMethodStub(MethodDesc* pUnboxMD)
{
    // unboxing a value class simply means adding 4 to the THIS pointer
    MetaSig msig(pUnboxMD);

    PPCReg regThis = msig.HasRetBuffArg() ? kR4 : kR3;

    PPCEmitAddi(regThis, regThis, sizeof(void*));

    // If it is an ECall, m_CodeOrIL does not reflect the correct address to
    // call to (which is an ECall stub).  Rather, it reflects the actual ECall
    // implementation.  Naturally, ECalls must always hit the stub first.
    // Along the same lines, perhaps the method isn't JITted yet.  The easiest
    // way to handle all this is to simply dispatch through the top of the MD.
    PPCEmitLoadImm(kR12, (__int32)(size_t)getStubCallAddr(pUnboxMD));

    // mtctr r12, bctr
    PPCEmitBranchR12();
}

//
// SecurityWrapper
//
// Wraps a real stub do some security work before the stub and clean up after. Before the
// real stub is called a security frame is added and declarative checks are performed. The
// Frame is added so declarative asserts and denies can be maintained for the duration of the
// real call. At the end the frame is simply removed and the wrapper cleans up the stack. The
// registers are restored.
//
VOID StubLinkerCPU::EmitSecurityWrapperStub(UINT numStackBytes, MethodDesc* pMD, BOOL fToStub, LPVOID pRealStub, DeclActionInfo *pActions)
{
    THROWSCOMPLUSEXCEPTION();

    EmitMethodStubProlog(InterceptorFrame::GetMethodFrameVPtr());

    // StubStackFrame::params are used for 3 params and 1 local var

    // call DoDeclaritiveSecurity(MethodDesc*, DeclActionInfo*, InterceptorFrame*);
    PPCEmitLoadImm(kR3, (UINT)(size_t)pMD);
    PPCEmitLoadImm(kR4, (UINT)(size_t)pActions);
    PPCEmitAddi(kR5, kR1, offsetof(StubStackFrame, methodframe)); // pFrame
    PPCEmitCall(NewExternalCodeLabel((LPVOID) DoDeclarativeSecurity));

    // refill the registers
    PPCEmitAddi(kR3, kR1, offsetof(StubStackFrame, methodframe));
    PPCEmitLoadImm(kR4, 1); // enregister
    PPCEmitCall(NewExternalCodeLabel((LPVOID) FramedMethodFrame::RegisterHelper));

    if (fToStub) {
        // restore the hidden stub argument
        PPCEmitLwz(kR11, kR1,
            offsetof(StubStackFrame, methodframe) + FramedMethodFrame::GetOffsetOfMethod());

        PPCEmitAddi(kR11, kR11, -METHOD_CALL_PRESTUB_SIZE);
    }

    UINT numStackBytesAligned = AlignStack(numStackBytes);

    // allocate the stack
    PPCEmitLwz(kR2, kR1, 0);
    // stwu r2, -numStackBytesAligned(r1)
    PPCEmitRegRegDisp16(0x94000000, kR2, kR1, -numStackBytesAligned);

    // copy the stack over (including the enregistered area to support the vararg calling convention)
    int i;
    for (i = 0; i < (int)numStackBytes; i += STACK_ELEM_SIZE) {
        PPCEmitLwz(kR2, kR1, numStackBytesAligned + sizeof(StubStackFrame) + i);
        PPCEmitStw(kR2, kR1, sizeof(LinkageArea) + i);
    }

    // restore the argument registers
    EmitArgumentRegsRestore(kR1, offsetof(StubStackFrame, argregs) + numStackBytesAligned);

    // call to the real stub
    PPCEmitLoadImm(kR12, (__int32)(size_t)pRealStub);
    // mtctr r12, bctrl
    PPCEmitCallR12();

    MetaSig msig(pMD);
    MetaSig* pSig = &msig;

    if (pSig->HasRetBuffArg()) {
        // oris r0, r0, 0
        Emit32(0x64000000);
    }

    // Restore the stack
    PPCEmitLwz(kR2, kR1, 0);
    // stwu r2, numStackBytesAligned(r1)
    PPCEmitRegRegDisp16(0x94000000, kR2, kR1, numStackBytesAligned);

    // Epilog
    EmitMethodStubEpilog(kNoTripStubStyle);
}

//
// Security Filter, if no security frame is required because there are no declarative asserts or denies
// then the arguments do not have to be copied. This interceptor creates a temporary Security frame
// calls the declarative security return, cleans up the stack to the same state when the inteceptor
// was called and jumps into the real routine.
//
VOID StubLinkerCPU::EmitSecurityInterceptorStub(MethodDesc* pMD, BOOL fToStub, LPVOID pRealStub, DeclActionInfo *pActions)
{
    THROWSCOMPLUSEXCEPTION();

    EmitMethodStubProlog(InterceptorFrame::GetMethodFrameVPtr());

    //  StubStackFrame::params are used for 3 params and 1 local var

    // call DoDeclaritiveSecurity(MethodDesc*, DeclActionInfo*, InterceptorFrame*);
    PPCEmitLoadImm(kR3, (UINT)(size_t)pMD);
    PPCEmitLoadImm(kR4, (UINT)(size_t)pActions);
    PPCEmitAddi(kR5, kR1, offsetof(StubStackFrame, methodframe)); // pFrame
    PPCEmitCall(NewExternalCodeLabel((LPVOID) DoDeclarativeSecurity));

    // refill the registers
    PPCEmitAddi(kR3, kR1, offsetof(StubStackFrame, methodframe));
    PPCEmitLoadImm(kR4, 1); // enregister
    PPCEmitCall(NewExternalCodeLabel((LPVOID) FramedMethodFrame::RegisterHelper));

    // restore argument registers
    EmitArgumentRegsRestore(kR1, offsetof(StubStackFrame, argregs));

    if (fToStub) {
        // restore the hidden stub argument
        PPCEmitLwz(kR11, kR1,
            offsetof(StubStackFrame, methodframe) + FramedMethodFrame::GetOffsetOfMethod());

        PPCEmitAddi(kR11, kR11, -METHOD_CALL_PRESTUB_SIZE);
    }

    EmitMethodStubEpilog(kInterceptorStubStyle);

    // jump to the real stub
    PPCEmitLoadImm(kR12, (__int32)(size_t)pRealStub);
    // mtctr r12, bctr
    PPCEmitBranchR12();
}

#ifdef _DEBUG
//-------------------------------------------------------------------------
// This is a function which checks the debugger stub tracing capability
// when calling out to unmanaged code.
//
// IF YOU SEE STRANGE ERRORS CAUSED BY THIS CODE, it probably means that
// you have changed some exit stub logic, and not updated the corresponding
// debugger helper routines.  The debugger helper routines need to be able
// to determine
//
//      (a) the unmanaged address called by the stub
//      (b) the return address which the unmanaged code will return to
//      (c) the size of the stack pushed by the stub
//
// This information is necessary to allow the COM+ debugger to hand off
// control properly to an unmanaged debugger & manage the boundary between
// managed & unmanaged code.
//
// These are in XXXFrame::GetUnmanagedCallSite. (Usually
// with some help from the stub linker for generated stubs.)
//-------------------------------------------------------------------------

extern "C" void * __stdcall PerformExitFrameChecks()
{
    Thread *thread = GetThread();
    Frame *frame = thread->GetFrame();

    void *ip, *returnIP, *returnSP;
    frame->GetUnmanagedCallSite(&ip, &returnIP, &returnSP);

    _ASSERTE((LPVOID)(size_t)(((LinkageArea*)returnSP)->SavedLR) == returnIP);

    return ip;
}

#endif  // _DEBUG

//===========================================================================
// create prestub
Stub * GeneratePrestub()
{
    CPUSTUBLINKER sl;
    CPUSTUBLINKER *psl = &sl;

    // the prolog
    psl->EmitMethodStubProlog(PrestubMethodFrame::GetMethodFrameVPtr());

    // r31 is pThread
    // r30 is previous frame

    //  StubStackFrame::params are used for 2 params

    // call PreStubWorker with the frame as an argument
    psl->PPCEmitAddi(kR3, kR1, offsetof(StubStackFrame, methodframe));
    psl->PPCEmitCall(psl->NewExternalCodeLabel((LPVOID) PreStubWorker));

    // r3 now contains replacement stub. PreStubWorker will never return
    // NULL (it throws an exception if stub creation fails.)

    // Debugger patch location
    psl->EmitPatchLabel();

    // restore previous frame
    psl->PPCEmitStw(kR30, kR31, Thread::GetOffsetOfCurrentFrame());

    // we'll jump to this address at the end of the function
    psl->PPCEmitMr(kR30, kR3);

    // refill the registers
    psl->PPCEmitAddi(kR3, kR1, offsetof(StubStackFrame, methodframe));
    psl->PPCEmitLoadImm(kR4, 1); // enregister
    psl->PPCEmitCall(psl->NewExternalCodeLabel((LPVOID) FramedMethodFrame::RegisterHelper));

    // mr r12, r30
    psl->PPCEmitMr(kR12, kR30);

    // restore all registers
    psl->EmitCalleeSavedRegsRestore(kR1, offsetof(StubStackFrame, savedregs));
    psl->EmitArgumentRegsRestore(kR1, offsetof(StubStackFrame, argregs));

    // restore the stack
    // lwz r1, 0(r1)
    psl->PPCEmitLwz(kR1, kR1, 0);

    // restore the link register
    // lwz r0, 8(r1)
    psl->PPCEmitLwz(kR0, kR1, 8);
    // mtlr r0
    psl->Emit32(0x7C0803A6);

    // mtctr r12, bctr
    psl->PPCEmitBranchR12();

    return psl->Link();
}

//-------------------------------------------------------------------------
// One-time creation of special prestub to initialize UMEntryThunks.
//-------------------------------------------------------------------------
Stub *GenerateUMThunkPrestub()
{
    CPUSTUBLINKER sl;
    CPUSTUBLINKER *psl = &sl;

    // r11 = UMEntryThunk.m_code

    // emit the initial prolog
    psl->EmitComMethodStubProlog(UMThkCallFrame::GetUMThkCallFrameVPtr(),
                               (LPVOID) UMThunkPrestubHandler, FALSE /*Don't profile*/);

    // r31 is pThread
    // r30 is previous frame

    // r3 = thunk
    psl->PPCEmitLwz(kR3, kR1, offsetof(UMStubStackFrame, umframe) + UMThkCallFrame::GetOffsetOfUMEntryThunk());

    // call UMEntryThunk::DoRuntimeInit
    psl->PPCEmitCall(psl->NewExternalCodeLabel((LPVOID)UMEntryThunk::DoRunTimeInit));

    psl->EmitComMethodStubEpilog(UMThkCallFrame::GetUMThkCallFrameVPtr(),
                               (LPVOID) UMThunkPrestubHandler, FALSE /*Don't profile*/, TRUE /* reloop */);

    return psl->Link();
}

/*static*/ void NDirect::CreateGenericNDirectStubSys(CPUSTUBLINKER *psl, UINT numStackBytes)
{
    // the prolog
    psl->EmitMethodStubProlog(NDirectMethodFrameGeneric::GetMethodFrameVPtr());

    // r31 is pThread
    // r30 is previous frame

    //  StubStackFrame::params are used for 2 params and 1 local var

    // CleanupWorkList
    psl->PPCEmitLoadImm(kR0, 0);
    psl->PPCEmitStw(kR0, kR1, offsetof(StubStackFrame, argregs) - sizeof(LPVOID));

    psl->PPCEmitMr(kR3, kR31); // current thread as ARG
    psl->PPCEmitAddi(kR4, kR1, offsetof(StubStackFrame, methodframe)); // new frame as ARG
    psl->PPCEmitCall(psl->NewExternalCodeLabel((LPVOID) NDirectGenericStubWorker));

    // the epilog
    psl->EmitMethodStubEpilog(kNoTripStubStyle);
}

/*static*/ Stub* CreateGenericNExportStub(UINT cbRetPop, 
    BOOL fHashThisCallAdjustment, BOOL fHashThisCallHiddenArg)
{
    CPUSTUBLINKER sl;
    CPUSTUBLINKER *psl = &sl;

    // r11 = UMEntryThunk.m_code

    // emit the initial prolog
    psl->EmitComMethodStubProlog(UMThkCallFrame::GetUMThkCallFrameVPtr(), 
                                    (LPVOID) UMThunkPrestubHandler, TRUE /*Profile*/);

    // r31 is pThread
    // r30 is previous frame

    // r3 = thread
    psl->PPCEmitMr(kR3, kR31);
    // r4 = pFrame  
    psl->PPCEmitAddi(kR4, kR1, offsetof(UMStubStackFrame, umframe));

    // call the worker
    psl->PPCEmitCall(psl->NewExternalCodeLabel((LPVOID) DoUMThunkCall));

    psl->EmitComMethodStubEpilog(UMThkCallFrame::GetUMThkCallFrameVPtr(), 
                                    (LPVOID) UMThunkPrestubHandler, TRUE /*Profile*/, FALSE);

    return psl->Link();
}

// Here's the support for the interlocked operations.  The external view of
// them is declared in util.hpp.

// These are implemented in assembly code, asmhelpers.s
extern "C" {
    void __fastcall OrMaskGN(DWORD volatile *Target, const int Bits);
    void __fastcall AndMaskGN(DWORD volatile *Target, const int Bits);
    LONG __fastcall ExchangeGN(LONG volatile *Target, LONG Value);
    PVOID __fastcall CompareExchangeGN(PVOID volatile *Target, PVOID Exchange, PVOID Comperand);
    LONG __fastcall ExchangeAddGN(LONG volatile *Target, LONG Value);
    LONG __fastcall IncrementGN(LONG volatile *Target);
    LONG __fastcall DecrementGN(LONG volatile *Target);
    UINT64 __fastcall IncrementLongGN(UINT64 volatile *Target);
    UINT64 __fastcall DecrementLongGN(UINT64 volatile *Target);
}

BitFieldOps FastInterlockOr = OrMaskGN;
BitFieldOps FastInterlockAnd = AndMaskGN;

XchgOps         FastInterlockExchange = ExchangeGN;
XchgOpsPtr      FastInterlockExchangePointer = (XchgOpsPtr)ExchangeGN;
CmpXchgOps      FastInterlockCompareExchange = (CmpXchgOps)CompareExchangeGN;
CmpXchgOpsPtr   FastInterlockCompareExchangePointer = CompareExchangeGN;
XchngAddOps     FastInterlockExchangeAdd = ExchangeAddGN;

IncDecOps   FastInterlockIncrement = IncrementGN;
IncDecOps   FastInterlockDecrement = DecrementGN;
IncDecLongOps    FastInterlockIncrementLong = IncrementLongGN;
IncDecLongOps	FastInterlockDecrementLong = DecrementLongGN;

// Adjust the generic interlocked operations for any platform specific ones we
// might have.
void InitFastInterlockOps()
{
    // Nothing to do for PowerPC
}


//---------------------------------------------------------
// Handles system specfic portion of fully optimized NDirect stub creation
//
// Results:
//     TRUE     - was able to create a standalone asm stub (generated into
//                psl)
//     FALSE    - decided not to create a standalone asm stub due to
//                to the method's complexity. Stublinker remains empty!
//
//     COM+ exception - error - don't trust state of stublinker.
//---------------------------------------------------------
/*static*/ BOOL NDirect::CreateStandaloneNDirectStubSys(const MLHeader *pheader, CPUSTUBLINKER *psl, BOOL fDoComInterop)
{
    // sorry, no standalone stubs on PPC
    return FALSE;
}

//---------------------------------------------------------
// Creates the slim NDirect stub.
//---------------------------------------------------------
/* static */
Stub* NDirect::CreateSlimNDirectStub(StubLinker *pstublinker, NDirectMethodDesc *pMD, UINT numStackBytes)
{
    // sorry, no slim stubs on PPC
    return NULL;
}

VOID __cdecl PopSEHRecords(LPVOID pTargetSP)
{
    PPAL_EXCEPTION_REGISTRATION pReg = PAL_GetBottommostRegistration();

    while ((PVOID)pReg < pTargetSP) {
        pReg = pReg->Next;
    }

    PAL_SetBottommostRegistration(pReg);
}

// This is implemented differently from the PopSEHRecords b/c it's called
// in the context of the DebuggerRCThread                                  
VOID PopSEHRecords(LPVOID pTargetSP, CONTEXT *pCtx, void *pSEH)
{
#ifdef _DEBUG
    LOG((LF_CORDB,LL_INFO1000, "\nPrintSEHRecords:\n"));

    EXCEPTION_REGISTRATION_RECORD *pEHR = (EXCEPTION_REGISTRATION_RECORD *)(size_t)*(DWORD *)pSEH;

    // check that all the eh frames are all greater than the current stack value. If not, the
    // stack has been updated somehow w/o unwinding the SEH chain.
    while (pEHR != NULL && pEHR != EXCEPTION_CHAIN_END)
    {
        LOG((LF_EH, LL_INFO1000000, "\t%08x: next:%08x handler:%x\n", pEHR, pEHR->Next, pEHR->Handler));
        pEHR = pEHR->Next;
    }
#endif

    DWORD dwCur = *(DWORD*)pSEH; // 'EAX' in the original routine
    DWORD dwPrev = (DWORD)(size_t)pSEH;

    while (dwCur < (DWORD)(size_t)pTargetSP)
    {
        // Watch for the OS handler
        // for nested exceptions, or any C++ handlers for destructors in our call
        // stack, or anything else.
        if (dwCur < (DWORD)(size_t)GetSP(pCtx))
            dwPrev = dwCur;

        dwCur = *(DWORD *)(size_t)dwCur;

        LOG((LF_CORDB,LL_INFO10000, "dwCur: 0x%x dwPrev:0x%x pTargetSP:0x%x\n",
            dwCur, dwPrev, pTargetSP));
    }

    *(DWORD *)(size_t)dwPrev = dwCur;

#ifdef _DEBUG
    pEHR = (EXCEPTION_REGISTRATION_RECORD *)(size_t)*(DWORD *)pSEH;
    // check that all the eh frames are all greater than the current stack value. If not, the
    // stack has been updated somehow w/o unwinding the SEH chain.

    LOG((LF_CORDB,LL_INFO1000, "\nPopSEHRecords:\n"));
    while (pEHR != NULL && pEHR != (void *)-1)
    {
        LOG((LF_EH, LL_INFO1000000, "\t%08x: next:%08x handler:%x\n", pEHR, pEHR->Next, pEHR->Handler));
        pEHR = pEHR->Next;
    }
#endif
}

//////////////////////////////////////////////////////////////////////////////
//
// JITInterface
//
//////////////////////////////////////////////////////////////////////////////

PPCReg GetPPCArgumentRegisterFromOffset(size_t ofs)
{
    return (PPCReg)(kR3 + (ofs / STACK_ELEM_SIZE));
}

//===========================================================================
// Emits code to adjust for a static delegate target.
VOID StubLinkerCPU::EmitShuffleThunk(ShuffleEntry *pShuffleEntryArray)
{
    // shuffle the arguments.
    ShuffleEntry *pWalk = pShuffleEntryArray;
    while (pWalk->srcofs != ShuffleEntry::SENTINEL) {
        if (pWalk->srcofs & ShuffleEntry::REGMASK) {
            if (pWalk->dstofs == (UINT16)-1) {
                // -1 is special - it is this pointer that will get converted into
                // the destination address
                PPCEmitLwz(kR12, GetPPCArgumentRegisterFromOffset(pWalk->srcofs & ShuffleEntry::OFSMASK),
                    Object::GetOffsetOfFirstField() + COMDelegate::m_pFPAuxField->GetOffset());
            }
            else {
                // there should be no register->memory moves because of the number of arguments is shrinking
                _ASSERT(pWalk->dstofs & ShuffleEntry::REGMASK);

                PPCEmitMr(GetPPCArgumentRegisterFromOffset(pWalk->dstofs & ShuffleEntry::OFSMASK),
                    GetPPCArgumentRegisterFromOffset(pWalk->srcofs & ShuffleEntry::OFSMASK));
            }
        } else {
            if (pWalk->dstofs & ShuffleEntry::REGMASK) {
                PPCEmitLwz(GetPPCArgumentRegisterFromOffset(pWalk->dstofs & ShuffleEntry::OFSMASK),
                    kR1, pWalk->srcofs + sizeof(LinkageArea));
            }
            else {
                PPCEmitLwz(kR0, kR1, pWalk->srcofs + sizeof(LinkageArea));
                PPCEmitStw(kR0, kR1, pWalk->dstofs + sizeof(LinkageArea));
            }
        }
        pWalk++;
    }

    // mtctr r12, bctr
    PPCEmitBranchR12();
}

//===========================================================================
// Emits code for MulticastDelegate.Invoke()
VOID StubLinkerCPU::EmitMulticastInvoke(UINT numStackBytes, BOOL fSingleCast, BOOL fHasReturnBuffer)
{
    THROWSCOMPLUSEXCEPTION();

    CodeLabel *pNullLabel = NewCodeLabel();
    CodeLabel *pMultiCaseLabel = NULL;

    PPCReg regThis = fHasReturnBuffer ? kR4 : kR3;

    PPCEmitCmpwi(regThis, 0);
    PPCEmitBranch(pNullLabel, PPCCondCode::kEQ);

    // StubLinkStubManager::TraceManager depends on the third intruction being a lwz r, thisReg imm
    // Make sure that's always the case, or change SLSM::TM
    if (!fSingleCast) {
        pMultiCaseLabel = NewCodeLabel();

        _ASSERTE(COMDelegate::m_pPRField);

        // multiple subscribers?
        PPCEmitLwz(kR2, regThis, Object::GetOffsetOfFirstField() + COMDelegate::m_pPRField->GetOffset());
        PPCEmitCmpwi(kR2, 0);
        PPCEmitBranch(pMultiCaseLabel, PPCCondCode::kNE);
    }

    // Only one subscriber. Do the simple jump.
    _ASSERTE(COMDelegate::m_pFPField);
    _ASSERTE(COMDelegate::m_pORField);

    // save target stub in register
    PPCEmitLwz(kR12, regThis, Object::GetOffsetOfFirstField() + COMDelegate::m_pFPField->GetOffset());
    // replace "this" pointer
    PPCEmitLwz(regThis, regThis, Object::GetOffsetOfFirstField() + COMDelegate::m_pORField->GetOffset());

    // mtctr r12, bctr
    PPCEmitBranchR12();

    if (!fSingleCast) {

        // The multiple subscriber case. Must create a frame to protect arguments during iteration.
        EmitLabel(pMultiCaseLabel);

        // Push a MulticastFrame on the stack.
        EmitMethodStubProlog(MulticastFrame::GetMethodFrameVPtr());

        // compute # of delegates to invoke
        PPCEmitLoadImm(kR29, 0);
        PPCEmitLwz(kR2, kR1, sizeof(StubStackFrame) + 4 * (regThis - kR3));

        CodeLabel *pLoop1Label = NewCodeLabel();
        CodeLabel *pDone1Label = NewCodeLabel();

        EmitLabel(pLoop1Label);
        // kR2 = kR2._pPrev
        PPCEmitLwz(kR2, kR2, Object::GetOffsetOfFirstField() + COMDelegate::m_pPRField->GetOffset());
        // if (kR2 == NULL) break;
        PPCEmitCmpwi(kR2, 0);
        PPCEmitBranch(pDone1Label, PPCCondCode::kEQ);
        // kR29++; goto pLoop1Label;
        PPCEmitAddi(kR29, kR29, 1);
        PPCEmitBranch(pLoop1Label);
        EmitLabel(pDone1Label);

        // kR29 = (# of delegates to invoke) - 1

        CodeLabel *pInvokeLoop = NewCodeLabel();
        EmitLabel(pInvokeLoop);

        // Walk down the list from the promoted head pointer each time.
        PPCEmitLwz(kR28, kR1, sizeof(StubStackFrame) + 4 * (regThis - kR3));
        PPCEmitMr(kR2, kR29);

        CodeLabel *pLoop2Label = NewCodeLabel();
        CodeLabel *pDone2Label = NewCodeLabel();

        EmitLabel(pLoop2Label);
        // if (kR2 == NULL) break;
        PPCEmitCmpwi(kR2, 0);
        PPCEmitBranch(pDone2Label, PPCCondCode::kEQ);
        // kR28 = kR28._pPrev
        PPCEmitLwz(kR28, kR28, Object::GetOffsetOfFirstField() + COMDelegate::m_pPRField->GetOffset());
        // kR2--; goto pLoop2Label;
        PPCEmitAddi(kR2, kR2, -1);
        PPCEmitBranch(pLoop2Label);
        EmitLabel(pDone2Label);

        // refill the registers
        PPCEmitAddi(kR3, kR1, offsetof(StubStackFrame, methodframe));
        PPCEmitLoadImm(kR4, 1); // enregister
        PPCEmitCall(NewExternalCodeLabel((LPVOID) FramedMethodFrame::RegisterHelper));

        UINT numStackBytesAligned = AlignStack(numStackBytes);

        // allocate the stack
        PPCEmitLwz(kR2, kR1, 0);
        // stwu r2, -numStackBytesAligned(r1)
        PPCEmitRegRegDisp16(0x94000000, kR2, kR1, -numStackBytesAligned);

        // copy the stack over (including the enregistered area to support the vararg calling convention)
        int i;        
        for (i = 0; i < (int)numStackBytes; i += STACK_ELEM_SIZE) {
            PPCEmitLwz(kR2, kR1, numStackBytesAligned + sizeof(StubStackFrame) + i);
            PPCEmitStw(kR2, kR1, sizeof(LinkageArea) + i);
        }

        // restore the argument registers
        EmitArgumentRegsRestore(kR1, offsetof(StubStackFrame, argregs) + numStackBytesAligned);

        // save target stub in register
        PPCEmitLwz(kR12, kR28, Object::GetOffsetOfFirstField() + COMDelegate::m_pFPField->GetOffset());
        // replace "this" pointer
        PPCEmitLwz(regThis, kR28, Object::GetOffsetOfFirstField() + COMDelegate::m_pORField->GetOffset());

        // mtctr r12, bctrl
        PPCEmitCallR12();

        if (fHasReturnBuffer) {
            // oris r0, r0, 0
            Emit32(0x64000000);
        }

        // Restore the stack
        PPCEmitLwz(kR2, kR1, 0);
        // stwu r2, numStackBytesAligned(r1)
        PPCEmitRegRegDisp16(0x94000000, kR2, kR1, numStackBytesAligned);

        // The debugger may need to stop here, so grab the offset of this code.
        EmitDebuggerIntermediateLabel();

        PPCEmitCmpwi(kR29, 0);
        PPCEmitAddi(kR29, kR29, -1);
        PPCEmitBranch(pInvokeLoop, PPCCondCode::kNE);

        // Epilog
        EmitMethodStubEpilog(kNoTripStubStyle);
    }

    // We're going to be clever, in that we're going to record the offset of the last instruction,
    // and the diff between this and the call behind us
    EmitPatchLabel();


    // Do a null throw
    EmitLabel(pNullLabel);

    // mflr r0
    Emit32(0x7C0802A6);

    // stw r0, 8(r1)
    PPCEmitStw(kR0, kR1, 8);
    // stwu r1, -local_space(r1)
    PPCEmitRegRegDisp16(0x94000000, kR1, kR1, -(sizeof(LinkageArea)+4*6));

    PPCEmitLoadImm(kR3, 0);
    PPCEmitLoadImm(kR4, kNullReferenceException);
    PPCEmitLoadImm(kR5, 0);
    PPCEmitLoadImm(kR6, 0);
    PPCEmitLoadImm(kR7, 0);
    PPCEmitLoadImm(kR8, 0);

    PPCEmitCall(NewExternalCodeLabel((LPVOID) __FCThrow));

    // lwz r1, 0(r1)
    PPCEmitLwz(kR1, kR1, 0);

    // restore the link register
    // lwz r0, 8(r1)
    PPCEmitLwz(kR0, kR1, 8);
    // mtlr r0
    Emit32(0x7C0803A6);

    // blr
    Emit32(0x4E800020);
}

//===========================================================================
// This routine is called if the Array store needs a frame constructed
// in order to do the array check.  It should only be called from
// the array store check helpers.

HCIMPL2(BOOL, ArrayStoreCheck, Object** pElement, PtrArray** pArray)
    BOOL ret;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_2(Frame::FRAME_ATTR_CAPTURE_DEPTH_2 | Frame::FRAME_ATTR_EXACT_DEPTH, *pElement, *pArray);

#ifdef STRESS_HEAP
    // Force a GC on every jit if the stress level is high enough
    if (g_pConfig->GetGCStressLevel() != 0
#ifdef _DEBUG
        && !g_pConfig->FastGCStressLevel()
#endif
        )
        g_pGCHeap->StressHeap();
#endif

#if CHECK_APP_DOMAIN_LEAKS
    if (g_pConfig->AppDomainLeaks())
      (*pElement)->AssignAppDomain((*pArray)->GetAppDomain());
#endif
    
    ret = ObjIsInstanceOf(*pElement, (*pArray)->GetElementTypeHandle());
    
    HELPER_METHOD_FRAME_END();
    return(ret);
HCIMPLEND

//===========================================================================
// Helper for EmitArrayOpStub - almost identical to CopyValueClassUnchecked
void ArrayOpCopyValue(void* dest, void* src, UINT size, CGCDesc* map)
{
    // Copy the bulk of the data, and any non-GC refs. 
    switch (size)
    {
    case 1:
        *(UINT8*)dest = *(UINT8*)src;
        break;
    case 2:
        *(UINT16*)dest = *(UINT16*)src;
        break;
    case 4:
        *(UINT32*)dest = *(UINT32*)src;
        break;
    case 8:
        *(UINT64*)dest = *(UINT64*)src;
        break;
    default:
        memcpyNoGCRefs(dest, src, size);
        break;
    }

    // Tell the GC about any copies.  
    if (map)
    {   
        CGCDescSeries* cur = map->GetHighestSeries();
        CGCDescSeries* last = map->GetLowestSeries();
        _ASSERTE(cur >= last);
        do                                                                  
        {   
            // offset to embedded references in this series must be
            // adjusted by the VTable pointer, when in the unboxed state.
            size_t offset = cur->GetSeriesOffset() - sizeof(void*);
            OBJECTREF* srcPtr = (OBJECTREF*)(((BYTE*) src) + offset);
            OBJECTREF* destPtr = (OBJECTREF*)(((BYTE*) dest) + offset);
            OBJECTREF* srcPtrStop = (OBJECTREF*)((BYTE*) srcPtr + cur->GetSeriesSize() + size);         
            while (srcPtr < srcPtrStop)                                         
            {   
                SetObjectReferenceUnchecked(destPtr, ObjectToOBJECTREF(*(Object**)srcPtr));
                srcPtr++;
                destPtr++;
            }                                                               
            cur--;                                                              
        } while (cur >= last);                                              
    }
}

//===========================================================================
// Helper for EmitArrayOpStub
PPCReg StubLinkerCPU::ArrayOpEnregisterFromOffset(UINT offset, PPCReg regScratch)
{
    if (offset < sizeof(FramedMethodFrame) + NUM_ARGUMENT_REGISTERS * sizeof(INT32)) {
        return GetPPCArgumentRegisterFromOffset(offset - sizeof(FramedMethodFrame));
    }

    PPCEmitLwz(regScratch, kR1, offset - (sizeof(FramedMethodFrame) - sizeof(LinkageArea)));
    return regScratch;
}

//===========================================================================
// Emits code to do an array operation.
VOID StubLinkerCPU::EmitArrayOpStub(const ArrayOpScript* pArrayOpScript)
{
    THROWSCOMPLUSEXCEPTION();

    const PPCReg kTotalReg    = kR31;
    const PPCReg kFactorReg   = kR30;

    // Working registers:
    PPCReg arrayRefReg = (pArrayOpScript->m_flags & pArrayOpScript->HASRETVALBUFFER) ? kR4 : kR3;       

    CodeLabel *Epilog = NewCodeLabel();
    CodeLabel *Inner_nullexception = NewCodeLabel();
    CodeLabel *Inner_rangeexception = NewCodeLabel();
    CodeLabel *Inner_typeMismatchexception = 0;

    INT calleesaved = 2 * sizeof(INT);

    // save kR30 and kR31 in red area
    // stmw r30, d(basereg)
    PPCEmitRegRegDisp16(0xBC000000, kR30, kR1, -8);

    PPCEmitCmpwi(arrayRefReg, 0);
    PPCEmitBranch(Inner_nullexception, PPCCondCode::kEQ);

    // Do Type Check if needed
    if (pArrayOpScript->m_flags & ArrayOpScript::NEEDSTYPECHECK) {

        // throw exception if failed
        Inner_typeMismatchexception = NewCodeLabel();
        if (pArrayOpScript->m_op == ArrayOpScript::STORE) {           

            // Get the value to be stored.
            PPCReg valueReg = ArrayOpEnregisterFromOffset(pArrayOpScript->m_fValLoc, kR2);

            PPCEmitCmpwi(valueReg, 0);
            CodeLabel *CheckPassed = NewCodeLabel();
            PPCEmitBranch(CheckPassed, PPCCondCode::kEQ);    // storing NULL is OK

            PPCEmitLwz(kR2, valueReg);
            PPCEmitLwz(kR11, arrayRefReg, offsetof(PtrArray, m_ElementType));
            PPCEmitCmpw(kR2, kR11);
            PPCEmitBranch(CheckPassed, PPCCondCode::kEQ);    // Exact match is OK

            PPCEmitLoadImm(kR2, (DWORD)(size_t) &g_pObjectClass);
            PPCEmitCmpw(kR2, kR11);
            PPCEmitBranch(CheckPassed, PPCCondCode::kEQ);    // Assigning to array of object is OK

            struct ArrayStoreCheckFrame {
                LinkageArea link;
                INT32 params[2];
                ArgumentRegisters argregs;
            };

            // mflr r0
            Emit32(0x7C0802A6);
            // stw r0, 8(r1)
            PPCEmitStw(kR0, kR1, 8);
            // stwu r1, -local_space(r1)
            INT frameSize = sizeof(ArrayStoreCheckFrame) + calleesaved;
            _ASSERTE(AlignStack(frameSize) == frameSize);
            PPCEmitRegRegDisp16(0x94000000, kR1, kR1, -frameSize);

            // store all arg registers
            EmitArgumentRegsSave(kR1, offsetof(ArrayStoreCheckFrame, argregs));

            // get address of value to store
            PPCEmitAddi(kR3, kR1, (valueReg == kR2) ? 
                (frameSize + pArrayOpScript->m_fValLoc - (sizeof(FramedMethodFrame) - sizeof(LinkageArea))) : 
                offsetof(ArrayStoreCheckFrame, argregs.r[valueReg - kR3]));

            // get address of 'this'
            PPCEmitAddi(kR4, kR1, offsetof(ArrayStoreCheckFrame, argregs.r[arrayRefReg - kR3]));

            // call BOOL ArrayStoreCheck(Object** pElement, PtrArray** pArray)
            PPCEmitCall(NewExternalCodeLabel((LPVOID) ArrayStoreCheck));

            // save the return value
            PPCEmitMr(kR2, kR3);

            // restore all arg registers
            EmitArgumentRegsRestore(kR1, offsetof(ArrayStoreCheckFrame, argregs));

            // lwz r1, 0(r1)
            PPCEmitLwz(kR1, kR1, 0);
            // lwz r0, 8(r1)
            PPCEmitLwz(kR0, kR1, 8);
            // mtlr r0
            Emit32(0x7C0803A6);

            PPCEmitCmpw(kR2, kR2);
            PPCEmitBranch(Epilog, PPCCondCode::kNE);    // This branch never taken, but epilog walker uses it

            PPCEmitCmpwi(kR2, 0);
            PPCEmitBranch(Inner_typeMismatchexception, PPCCondCode::kEQ);

            EmitLabel(CheckPassed);
        }
        else if (pArrayOpScript->m_op == ArrayOpScript::LOADADDR) {
                       
            // Load up the hidden type parameter into 'regType'
            PPCReg regType = ArrayOpEnregisterFromOffset(pArrayOpScript->m_typeParamOffs, kR2);

            // regType holds the typeHandle for the ARRAY.  This must be a ArrayTypeDesc*, so
            // mask off the low two bits to get the TypeDesc*

            // rlwinm r2,regType,0,0,29
            PPCEmitRegRegDisp16(0x54000000, regType, kR2, 0x003a);

            // Get the parameter of the parameterize type
            PPCEmitLwz(kR2, kR2, offsetof(ParamTypeDesc, m_Arg));

            // Compare this against the element type of the array.
            PPCEmitLwz(kR11, arrayRefReg, offsetof(PtrArray, m_ElementType));
            PPCEmitCmpw(kR2, kR11);

            // Throw error if not equal
            PPCEmitBranch(Inner_typeMismatchexception, PPCCondCode::kNE);
        }
    }

    CodeLabel* DoneCheckLabel = 0;
    if (pArrayOpScript->m_rank == 1 && pArrayOpScript->m_fHasLowerBounds) {
        DoneCheckLabel = NewCodeLabel();
        CodeLabel* NotSZArrayLabel = NewCodeLabel();

        // for rank1 arrays, we might actually have two different layouts depending on
        // if we are ELEMENT_TYPE_ARRAY or ELEMENT_TYPE_SZARRAY.

        // lwz kR2, 0(arrayRefReg)
        PPCEmitLwz(kR2, arrayRefReg); // kR2 holds the method table

        // lbzu kR2, kR2.m_NormType
        PPCEmitRegRegDisp16(0x8C000000, kR2, kR2, offsetof(MethodTable, m_NormType));
        PPCEmitCmpwi(kR2, ELEMENT_TYPE_SZARRAY);

        // bne NotSZArrayLabel
        PPCEmitBranch(NotSZArrayLabel, PPCCondCode::kNE);

        // The index will be always in register in this case
        const ArrayOpIndexSpec *pai = pArrayOpScript->GetArrayOpIndexSpecs();
        PPCReg idxReg = ArrayOpEnregisterFromOffset(pai->m_idxloc, kR2);

        // lwz kR11, arrayRefReg.LENGTH
        PPCEmitLwz(kR11, arrayRefReg, ArrayBase::GetOffsetOfNumComponents());
        // cmpl idxReg, kR11
        PPCEmitRegRegReg(0x7C000040, (PPCReg)0, idxReg, kR11);

        // bge Inner_rangeexception
        PPCEmitBranch(Inner_rangeexception, PPCCondCode::kGE);

        PPCEmitMr(kTotalReg, idxReg);

        // addi arrayRefReg, arrayRefReg, -8 // 8 is accounts for the Lower bound and Dim count in the ARRAY
        PPCEmitAddi(arrayRefReg, arrayRefReg, -8);

        PPCEmitBranch(DoneCheckLabel);
        EmitLabel(NotSZArrayLabel);
    }

    {
        // For each index, range-check and mix into accumulated total.
        UINT idx = pArrayOpScript->m_rank;
        BOOL firstTime = TRUE;
        while (idx--) {
            const ArrayOpIndexSpec *pai = pArrayOpScript->GetArrayOpIndexSpecs() + idx;

            //Load the passed-in index
            PPCReg idxReg = ArrayOpEnregisterFromOffset(pai->m_idxloc, kR2);

            if (pArrayOpScript->m_fHasLowerBounds) {
                // lwz kR11, LOWERBOUND
                PPCEmitLwz(kR11, arrayRefReg, pai->m_lboundofs);
                // subf idxReg, kR11, idxReg
                PPCEmitRegRegReg(0x7C000050, idxReg, kR11, idxReg);
            }

            // lwz kR11, LENGTH
            PPCEmitLwz(kR11, arrayRefReg, pai->m_lengthofs); 
            // cmpl idxReg, kR11
            PPCEmitRegRegReg(0x7C000040, (PPCReg)0, idxReg, kR11);
   
            // bge Inner_rangeexception
            PPCEmitBranch(Inner_rangeexception, PPCCondCode::kGE);
    
            if (!firstTime) {  //Can skip the first time since FACTOR==1
                // mullw idxReg, idxReg, kFactorReg
                PPCEmitRegRegReg(0x7C0001D6, idxReg, idxReg, kFactorReg);
            }

            // kTotalReg += idxReg
            if (firstTime) {
                // First time, we must zero-init TOTAL. Since
                // zero-initing and then adding is just equivalent to a
                // "mr", emit a "mr"
                // mr kTotalReg, idxReg
                PPCEmitMr(kTotalReg, idxReg);
            } else {
                // add kTotalReg, idxReg
                PPCEmitRegRegReg(0x7C000214, kTotalReg, kTotalReg, idxReg);
            }

            // kTotalReg *= [arrayRefReg + LENGTH]
            if (idx != 0) {  // No need to update FACTOR on the last iteration
                //  since we won't use it again

                if (firstTime) {
                    // must init FACTOR to 1 first: hence,
                    // the "mullw" becomes a "lwz"
                    // lwz kFactorReg, [arrayRefReg + LENGTH]
                    PPCEmitLwz(kFactorReg, arrayRefReg, pai->m_lengthofs);
                } else {
                    // lwz kR11, [arrayRefReg + LENGTH]
                    PPCEmitLwz(kR11, arrayRefReg, pai->m_lengthofs);
                    // mullw kFactorReg, kFactorReg, kR11
                    PPCEmitRegRegReg(0x7C0001D6, kFactorReg, kFactorReg, kR11);
                }
            }

            firstTime = FALSE;
        }
    }

    if (DoneCheckLabel != 0)
        EmitLabel(DoneCheckLabel);

    UINT32 elemScale = pArrayOpScript->m_elemsize;
    if (elemScale != 1) {
        // kR2 = elemScale
        PPCEmitLoadImm(kR2, elemScale);
        // mullw kTotalReg, kTotalReg, kR2
        PPCEmitRegRegReg(0x7C0001D6, kTotalReg, kTotalReg, kR2);
    }    

    PPCReg elemBaseReg = kTotalReg;
    // add elemBaseReg, arrayRefReg, kTotalReg
    PPCEmitRegRegReg(0x7C000214, elemBaseReg, arrayRefReg, kTotalReg);

    // Now, do the operation:

    UINT32 elemOfs = pArrayOpScript->m_ofsoffirst;

    switch (pArrayOpScript->m_op) {
        case pArrayOpScript->LOADADDR:
            // add kR3, elemBaseReg, elemOfs
            PPCEmitAddi(kR3, elemBaseReg, elemOfs);
            break;

        case pArrayOpScript->LOAD:
            if (pArrayOpScript->m_flags & pArrayOpScript->HASRETVALBUFFER)
            {
                // first two arguments for ArrayOpCopyValue

                // dest - already in R3
                _ASSERTE(GetPPCArgumentRegisterFromOffset(
                    pArrayOpScript->m_fRetBufLoc - sizeof(FramedMethodFrame)) == kR3);

                // src
                PPCEmitAddi(kR4, elemBaseReg, elemOfs);

            COPY_VALUE_CLASS:
                // erect frame and call ArrayOpCopyValue

                // mflr r0
                Emit32(0x7C0802A6);
                // stw r0, 8(r1)
                PPCEmitStw(kR0, kR1, 8);
                // stwu r1, -local_space(r1)
                INT frameSize = AlignStack(sizeof(LinkageArea) + 4 * sizeof(INT) + calleesaved);
                PPCEmitRegRegDisp16(0x94000000, kR1, kR1, -frameSize);

                // size
                PPCEmitLoadImm(kR5, pArrayOpScript->m_elemsize);
                
                // map
                PPCEmitLoadImm(kR6, (INT)(size_t)pArrayOpScript->m_gcDesc);

                // call ArrayOpCopyValue(void* dest, void* src, UINT size, CGCDesc* map)
                PPCEmitCall(NewExternalCodeLabel((LPVOID)ArrayOpCopyValue));

                // lwz r1, 0(r1)
                PPCEmitLwz(kR1, kR1, 0);
                // lwz r0, 8(r1)
                PPCEmitLwz(kR0, kR1, 8);
                // mtlr r0
                Emit32(0x7C0803A6);                
            }
            else
            {
                switch (pArrayOpScript->m_elemsize) {
                    case 1:
                        // lbz r3, ELEMADDR
                        PPCEmitRegRegDisp16(0x88000000, kR3, elemBaseReg, elemOfs);
                        if (pArrayOpScript->m_signed) {
                            // extsb r3,r3
                            PPCEmitRegRegDisp16(0x7C000000, kR3, kR3, 0x0774);
                        }
                        break;

                    case 2:
                        // lh[a|z] r3, ELEMADDR
                        PPCEmitRegRegDisp16(pArrayOpScript->m_signed ? 0xA8000000 : 0xA0000000,
                            kR3, elemBaseReg, elemOfs);
                        break;

                    case 4:
                        if (pArrayOpScript->m_flags & pArrayOpScript->ISFPUTYPE) {
                            // lfs f1, ELEMADDR
                            PPCEmitRegRegDisp16(0xC0000000, (PPCReg)1, elemBaseReg, elemOfs);
                        } else {
                            // lwz r3, ELEMADDR
                            PPCEmitLwz(kR3, elemBaseReg, elemOfs);
                        }
                        break;

                    case 8:
                        if (pArrayOpScript->m_flags & pArrayOpScript->ISFPUTYPE) {
                            // lfd f1, ELEMADDR
                            PPCEmitRegRegDisp16(0xC8000000, (PPCReg)1, elemBaseReg, elemOfs);
                        } else {
                            // lwz r3, ELEMADDR
                            PPCEmitLwz(kR3, elemBaseReg, elemOfs);
                            // lwz r4, ELEMADDR+4
                            PPCEmitLwz(kR4, elemBaseReg, elemOfs + 4);
                        }
                        break;


                    default:
                        _ASSERTE(0);
                }
            }
            break;

        case pArrayOpScript->STORE:
            {
            PPCReg srcReg = ArrayOpEnregisterFromOffset(pArrayOpScript->m_fValLoc, kR2);

            switch (pArrayOpScript->m_elemsize) {

                case 1:
                    // stb srcReg, ELEMADDR
                    PPCEmitRegRegDisp16(0x98000000, srcReg, elemBaseReg, elemOfs);
                    break;
                case 2:
                    // sth srcReg, ELEMADDR
                    PPCEmitRegRegDisp16(0xB0000000, srcReg, elemBaseReg, elemOfs);
                    break;
                case 4:
                    if (pArrayOpScript->m_flags & pArrayOpScript->NEEDSWRITEBARRIER) {
                        // mflr r0
                        Emit32(0x7C0802A6);
                        // stw r0, 8(r1)
                        PPCEmitStw(kR0, kR1, 8);
                        // stwu r1, -local_space(r1)
                        INT frameSize = AlignStack(sizeof(LinkageArea) + 2 * sizeof(INT) + calleesaved);
                        PPCEmitRegRegDisp16(0x94000000, kR1, kR1, -frameSize);

                        // dst
                        PPCEmitAddi(kR3, elemBaseReg, elemOfs);
                        // src
                        PPCEmitMr(kR4, srcReg);

                        // call JIT_WriteBarrier(OBJECTREF *dst, OBJECTREF ref)
                        PPCEmitCall(NewExternalCodeLabel((LPVOID)JIT_WriteBarrier));

                        // lwz r1, 0(r1)
                        PPCEmitLwz(kR1, kR1, 0);
                        // lwz r0, 8(r1)
                        PPCEmitLwz(kR0, kR1, 8);
                        // mtlr r0
                        Emit32(0x7C0803A6);
                    }
                    else
                    if (pArrayOpScript->m_flags & pArrayOpScript->ISFPUTYPE) {
                        // stfs f1, ELEMADDR
                        PPCEmitRegRegDisp16(0xD0000000, (PPCReg)1, elemBaseReg, elemOfs);
                    }
                    else {
                        // stw srcReg, ELEMADDR
                        PPCEmitStw(srcReg, elemBaseReg, elemOfs);
                    }
                    break;

                case 8:
                    if (!pArrayOpScript->m_gcDesc) {
                        if (pArrayOpScript->m_flags & pArrayOpScript->ISFPUTYPE) {
                            // stfd f1, ELEMADDR
                            PPCEmitRegRegDisp16(0xD8000000, (PPCReg)1, elemBaseReg, elemOfs);
                        }
                        else {
                            // stw srcReg, ELEMADDR
                            PPCEmitStw(srcReg, elemBaseReg, elemOfs);

                            srcReg = ArrayOpEnregisterFromOffset(pArrayOpScript->m_fValLoc + 4, kR2);

                            // stw srcReg, ELEMADDR+4
                            PPCEmitStw(srcReg, elemBaseReg, elemOfs + 4);
                        }
                        break;
                    }
                        // FALL THROUGH
                default:
                    {
                    // deregister the value
                    UINT ofsend = min(pArrayOpScript->m_fValLoc + pArrayOpScript->m_elemsize,
                        sizeof(FramedMethodFrame) + NUM_ARGUMENT_REGISTERS * sizeof(INT32));

                    for (UINT ofs = pArrayOpScript->m_fValLoc; ofs < ofsend; ofs += 4) {
                        PPCReg regVal = GetPPCArgumentRegisterFromOffset(ofs - sizeof(FramedMethodFrame));
                        PPCEmitStw(regVal, kR1, ofs - (sizeof(FramedMethodFrame) - sizeof(LinkageArea)));
                    }

                    // first two arguments for ArrayOpCopyValue

                    // dest
                    PPCEmitAddi(kR3, elemBaseReg, elemOfs);

                    // src
                    PPCEmitAddi(kR4, kR1, 
                        pArrayOpScript->m_fValLoc - (sizeof(FramedMethodFrame) - sizeof(LinkageArea)));

                    goto COPY_VALUE_CLASS;
                    }
            }
            }
            break;

        default:
            _ASSERTE(0);
    }

    EmitLabel(Epilog);

    // Restore the callee-saved registers
    // lmw r30, d(basereg)
    PPCEmitRegRegDisp16(0xB8000000, kR30, kR1, -8);

    // blr
    Emit32(0x4E800020);

   // Exception points must clean up the stack for all those extra args:
    EmitLabel(Inner_nullexception);
    PPCEmitLoadImm(kR3, CORINFO_NullReferenceException);

    CodeLabel* ExceptionWorkerLabel = NewCodeLabel();
    EmitLabel(ExceptionWorkerLabel);

    // Restore the callee-saved registers
    // lmw r30, d(basereg)
    PPCEmitRegRegDisp16(0xB8000000, kR30, kR1, -8);

    // jump to JIT_InternalThrow
    PPCEmitLoadImm(kR12, (__int32)(size_t)hlpFuncTable[CORINFO_HELP_INTERNALTHROW].pfnHelper);
    PPCEmitBranchR12();

    EmitLabel(Inner_rangeexception);
    PPCEmitLoadImm(kR3, CORINFO_IndexOutOfRangeException);
    PPCEmitBranch(ExceptionWorkerLabel);

    if (pArrayOpScript->m_flags & pArrayOpScript->NEEDSTYPECHECK) {
        EmitLabel(Inner_typeMismatchexception);
        PPCEmitLoadImm(kR3, CORINFO_ArrayTypeMismatchException);
        PPCEmitBranch(ExceptionWorkerLabel);
    }
}

//===========================================================================
// Emits code to throw a rank exception
VOID StubLinkerCPU::EmitRankExceptionThrowStub(UINT cbFixedArgs)
{
    PPCEmitLoadImm(kR3, CORINFO_RankException);

    // jump to JIT_InternalThrow
    PPCEmitLoadImm(kR12, (__int32)(size_t)hlpFuncTable[CORINFO_HELP_INTERNALTHROW].pfnHelper);
    PPCEmitBranchR12();
}

//===========================================================================
// Emits code to break into debugger
VOID StubLinkerCPU::EmitDebugBreak()
{
    // trap 23, r16, r0
    Emit32(0x7ef00008);
}

VOID StubLinkerCPU::EmitProfilerComCallProlog(PVOID pFrameVptr)
{
#ifdef PROFILING_SUPPORTED
    if (pFrameVptr == UMThkCallFrame::GetUMThkCallFrameVPtr())
    {
        // the ComMethodProlog already saved registers
        
        // Load methoddesc* into r3
        PPCEmitLwz(kR3, kR1, offsetof(UMStubStackFrame, umframe) + UMThkCallFrame::GetOffsetOfDatum());
        PPCEmitLwz(kR3, kR3, UMEntryThunk::GetOffsetOfMethodDesc());
        // Load reason into r4
        PPCEmitLoadImm(kR4, COR_PRF_TRANSITION_CALL);
        // Make the call
        PPCEmitCall(NewExternalCodeLabel((LPVOID)ProfilerUnmanagedToManagedTransitionMD));
    } else {
        _ASSERTE(!"Unrecognized vtble passed to EmitComMethodStubProlog with profiling turned on.");
    }
#endif
}

VOID StubLinkerCPU::EmitProfilerComCallEpilog(PVOID pFrameVptr)
{
#ifdef PROFILING_SUPPORTED
    if (pFrameVptr == UMThkCallFrame::GetUMThkCallFrameVPtr())
    {
        // the ComMethodProlog already saved registers
        
        // Load methoddesc* into r3
        PPCEmitLwz(kR3, kR1, offsetof(UMStubStackFrame, umframe) + UMThkCallFrame::GetOffsetOfDatum());
        PPCEmitLwz(kR3, kR3, UMEntryThunk::GetOffsetOfMethodDesc());
        // Load reason into r4
        PPCEmitLoadImm(kR4, COR_PRF_TRANSITION_RETURN);
        // Make the call
        PPCEmitCall(NewExternalCodeLabel((LPVOID)ProfilerManagedToUnmanagedTransitionMD));
    } else {
        _ASSERTE(!"Unrecognized vtble passed to EmitComMethodStubProlog with profiling turned on.");
    }
#endif
}

void UMEntryThunkCode::Encode(BYTE* pTargetCode, void* pvSecretParam)
{
    m_code[0] = 0x7D8B6378; // mr r11, r12
    m_code[1] = 0x818C0010; // lwz r12, 16(r12)
    m_code[2] = 0x7D8903A6; // mtctr r12
    m_code[3] = 0x4E800420; // bctr
    m_execstub = pTargetCode; // <actual code address>
    FlushInstructionCache(GetCurrentProcess(), m_code, 16);
}

UMEntryThunk* UMEntryThunk::Decode(LPVOID pCallback)
{
    if ( ((((size_t)pCallback) & 3) != 0) ||
        (((DWORD*)pCallback)[0] != 0x7D8B6378) || // mr r11, r12
        (((DWORD*)pCallback)[1] != 0x818C0010) || // lwz r12, 16(r12)
        (((DWORD*)pCallback)[2] != 0x7D8903A6) || // mtctr r12
        (((DWORD*)pCallback)[3] != 0x4E800420) ) // bctr
    {
        return NULL;
    }
    return (UMEntryThunk*)((BYTE*)pCallback - UMEntryThunk::GetOffsetOfCode());
}
