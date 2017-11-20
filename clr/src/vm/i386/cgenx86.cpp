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
// CGENX86.CPP -
//
// Various helper routines for generating x86 assembly code.
//
//

// Precompiled Header
#include "common.h"

#include "field.h"
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


extern "C" {

    DWORD __stdcall GetSpecificCpuTypeAsm(void);
    VOID __cdecl StubRareEnable(Thread *pThread);
    HRESULT __cdecl StubRareDisableHR(Thread *pThread, Frame *pFrame);
    VOID __cdecl StubRareDisableRETURN(Thread *pThread, Frame *pFrame);
    VOID __cdecl StubRareDisableTHROW(Thread *pThread, Frame *pFrame);
    VOID __cdecl ArrayOpStubNullException(void);
    VOID __cdecl ArrayOpStubRangeException(void);
    VOID __cdecl ArrayOpStubTypeMismatchException(void);
    VOID __cdecl ThrowRankExceptionStub(void);

    void __stdcall InterlockedCompareExchange8b(UINT64 *pLocation, UINT64 *pValue, UINT64 *pComparand);

    // Prevent multiple threads from simultaneous interlocked in/decrementing
    UINT	iSpinLock = 0;
}

//-----------------------------------------------------------------------
// InstructionFormat for near Jump and short Jump
//-----------------------------------------------------------------------
class X86NearJump : public InstructionFormat
{
    public:
        X86NearJump(UINT allowedSizes) : InstructionFormat(allowedSizes)
        {}

        virtual UINT GetSizeOfInstruction(UINT refsize, UINT variationCode)
        {
            return (refsize == k8 ? 2 : 5);
        }

        virtual VOID EmitInstruction(UINT refsize, __int64 fixedUpReference, BYTE *pOutBuffer, UINT variationCode, BYTE *pDataBuffer)
        {
            if (refsize == k8) {
                pOutBuffer[0] = 0xeb;
                *((__int8*)(pOutBuffer+1)) = (__int8)fixedUpReference;
            } else {
                pOutBuffer[0] = 0xe9;
                *((__int32*)(pOutBuffer+1)) = (__int32)fixedUpReference;
            }
        }
};




//-----------------------------------------------------------------------
// InstructionFormat for conditional jump. Set the variationCode
// to members of X86CondCode.
//-----------------------------------------------------------------------
class X86CondJump : public InstructionFormat
{
    public:
        X86CondJump(UINT allowedSizes) : InstructionFormat(allowedSizes)
        {}

        virtual UINT GetSizeOfInstruction(UINT refsize, UINT variationCode)
        {
            return (refsize == k8 ? 2 : 6);
        }

        virtual VOID EmitInstruction(UINT refsize, __int64 fixedUpReference, BYTE *pOutBuffer, UINT variationCode, BYTE *pDataBuffer)
        {
            if (refsize == k8) {
                pOutBuffer[0] = 0x70|variationCode;
                *((__int8*)(pOutBuffer+1)) = (__int8)fixedUpReference;
            } else {
                pOutBuffer[0] = 0x0f;
                pOutBuffer[1] = 0x80|variationCode;
                *((__int32*)(pOutBuffer+2)) = (__int32)fixedUpReference;
            }
        }


};




//-----------------------------------------------------------------------
// InstructionFormat for near call.
//-----------------------------------------------------------------------
class X86Call : public InstructionFormat
{
    public:
        X86Call(UINT allowedSizes) : InstructionFormat(allowedSizes)
        {}

        virtual UINT GetSizeOfInstruction(UINT refsize, UINT variationCode)
        {
            return 5;
        }

        virtual VOID EmitInstruction(UINT refsize, __int64 fixedUpReference, BYTE *pOutBuffer, UINT variationCode, BYTE *pDataBuffer)
        {
            pOutBuffer[0] = 0xe8;
            *((__int32*)(1+pOutBuffer)) = (__int32)fixedUpReference;
        }


};

//-----------------------------------------------------------------------
// InstructionFormat for push imm32.
//-----------------------------------------------------------------------
class X86PushImm32 : public InstructionFormat
{
    public:
        X86PushImm32(UINT allowedSizes) : InstructionFormat(allowedSizes)
        {}

        virtual UINT GetSizeOfInstruction(UINT refsize, UINT variationCode)
        {
            return 5;
        }

        virtual VOID EmitInstruction(UINT refsize, __int64 fixedUpReference, BYTE *pOutBuffer, UINT variationCode, BYTE *pDataBuffer)
        {
            pOutBuffer[0] = 0x68;
            // only support absolute pushimm32 of the label address. The fixedUpReference is
            // the offset to the label from the current point, so add to get address
            *((__int32*)(1+pOutBuffer)) = (__int32)(fixedUpReference);
        }


};

static BYTE gX86NearJump[sizeof(X86NearJump)];
static BYTE gX86CondJump[sizeof(X86CondJump)];
static BYTE gX86Call[sizeof(X86Call)];
static BYTE gX86PushImm32[sizeof(X86PushImm32)];

/* static */ BOOL StubLinkerCPU::Init()
{
    new (gX86NearJump) X86NearJump( InstructionFormat::k8|InstructionFormat::k32);
    new (gX86CondJump) X86CondJump( InstructionFormat::k8|InstructionFormat::k32);
    new (gX86Call) X86Call(InstructionFormat::k32);
    new (gX86PushImm32) X86PushImm32(InstructionFormat::k32);
    return TRUE;
}

//---------------------------------------------------------------
// Emits:
//    PUSH <reg32>
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitPushReg(X86Reg reg)
{
    THROWSCOMPLUSEXCEPTION();
    Emit8(0x50+reg);
    Push(4);
}


//---------------------------------------------------------------
// Emits:
//    POP <reg32>
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitPopReg(X86Reg reg)
{
    THROWSCOMPLUSEXCEPTION();
    Emit8(0x58+reg);
    Pop(4);
}


//---------------------------------------------------------------
// Emits:
//    PUSH <imm32>
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitPushImm32(UINT32 value)
{
    THROWSCOMPLUSEXCEPTION();
    Emit8(0x68);
    Emit32(value);
    Push(4);
}

//---------------------------------------------------------------
// Emits:
//    PUSH <imm32>
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitPushImm32(CodeLabel &target)
{
    THROWSCOMPLUSEXCEPTION();
    EmitLabelRef(&target, reinterpret_cast<X86PushImm32&>(gX86PushImm32), 0);
}

//---------------------------------------------------------------
// Emits:
//    PUSH <imm8>
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitPushImm8(BYTE value)
{
    THROWSCOMPLUSEXCEPTION();
    Emit8(0x6a);
    Emit8(value);
    Push(4);
}


//---------------------------------------------------------------
// Emits:
//    XOR <reg32>,<reg32>
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitZeroOutReg(X86Reg reg)
{
    Emit8(0x33);
    Emit8( 0xc0 | (reg << 3) | reg );
}


//---------------------------------------------------------------
// Emits:
//    JMP <ofs8>   or
//    JMP <ofs32}
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitNearJump(CodeLabel *target)
{
    THROWSCOMPLUSEXCEPTION();
    EmitLabelRef(target, reinterpret_cast<X86NearJump&>(gX86NearJump), 0);
}


//---------------------------------------------------------------
// Emits:
//    Jcc <ofs8> or
//    Jcc <ofs32>
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitCondJump(CodeLabel *target, X86CondCode::cc condcode)
{
    THROWSCOMPLUSEXCEPTION();
    EmitLabelRef(target, reinterpret_cast<X86CondJump&>(gX86CondJump), condcode);
}


//---------------------------------------------------------------
// Returns the type of CPU (the value of x of x86)
// (Please note, that it returns 6 for P5II)
// Also note that the CPU features are returned in the upper 16 bits.
//---------------------------------------------------------------
DWORD __stdcall GetSpecificCpuType()
{
    static DWORD val = 0;

    if (val)
        return(val);

    val = GetSpecificCpuTypeAsm();
#ifdef _DEBUG
    const DWORD cpuDefault = 0xFFFFFFFF;
    static ConfigDWORD cpuFamily;
    DWORD configFamily = cpuFamily.val(L"CPUFamily", cpuDefault);
    if (configFamily != cpuDefault)
    {
        assert((configFamily & 0xF) == configFamily);
        val = (val & 0xFFFF0000) | configFamily;
    }

    static ConfigDWORD cpuFeatures;
    DWORD configFeatures = cpuFeatures.val(L"CPUFeatures", cpuDefault);
    if (configFeatures != cpuDefault)
    {
        assert((configFeatures & 0xFFFF) == configFeatures);
        val = (val & 0x0000FFFF) | (configFeatures << 16);
    }
#endif
    return val;
}


//---------------------------------------------------------------
// Emits:
//    call <ofs32>
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitCall(CodeLabel *target, int iArgBytes,
                                BOOL returnLabel)
{
    THROWSCOMPLUSEXCEPTION();
    EmitLabelRef(target, reinterpret_cast<X86Call&>(gX86Call), 0);
    if (returnLabel)
        EmitReturnLabel();

    INDEBUG(Emit8(0x90));       // Emit a nop after the call in debug so that
                                // we know that this is a call that can directly call 
                                // managed code

    Pop(iArgBytes);
}

//---------------------------------------------------------------
// Emits:
//    ret n
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitReturn(int iArgBytes)
{
    THROWSCOMPLUSEXCEPTION();

    if (iArgBytes == 0)
        Emit8(0xc3);
    else
    {
        Emit8(0xc2);
        Emit16(iArgBytes);
    }

    Pop(iArgBytes);
}


VOID StubLinkerCPU::X86EmitPushRegs(unsigned regSet)
{
    for (X86Reg r = kEAX; r <= kEDI; r = (X86Reg)(r+1))
        if (regSet & (1U<<r))
            X86EmitPushReg(r);
}


VOID StubLinkerCPU::X86EmitPopRegs(unsigned regSet)
{
    for (X86Reg r = kEDI; r >= kEAX; r = (X86Reg)(r-1))
        if (regSet & (1U<<r))
            X86EmitPopReg(r);
}


//---------------------------------------------------------------
// Emit code to store the current Thread structure in dstreg
// preservedRegSet is a set of registers to be preserved
// TRASHES  EAX, EDX, ECX unless they are in preservedRegSet.
// RESULTS  dstreg = current Thread
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitTLSFetch(DWORD idx, X86Reg dstreg, unsigned preservedRegSet)
{
    // It doesn't make sense to have the destination register be preserved
    _ASSERTE((preservedRegSet & (1<<dstreg)) == 0);

    THROWSCOMPLUSEXCEPTION();
    TLSACCESSMODE mode = GetTLSAccessMode(idx);

#ifdef _DEBUG
    {
        static BOOL f = TRUE;
        f = !f;
        if (f) {
           mode = TLSACCESS_GENERIC;
        }
    }
#endif

    switch (mode) {

        case TLSACCESS_GENERIC:

            X86EmitPushRegs(preservedRegSet & ((1<<kEAX)|(1<<kEDX)|(1<<kECX)));

            X86EmitPushImm32(idx);

            // call TLSGetValue
            X86EmitCall(NewExternalCodeLabel((LPVOID) TlsGetValue), 4);  // in CE pop 4 bytes or args after the call

            // mov dstreg, eax
            Emit8(0x89);
            Emit8(0xc0 + dstreg);

            X86EmitPopRegs(preservedRegSet & ((1<<kEAX)|(1<<kEDX)|(1<<kECX)));

            break;

        default:
            _ASSERTE(0);
    }

#ifdef _DEBUG
    // Trash caller saved regs that we were not told to preserve, and that aren't the dstreg.
    preservedRegSet |= 1<<dstreg;
    if (!(preservedRegSet & (1<<kEAX)))
        X86EmitDebugTrashReg(kEAX);
    if (!(preservedRegSet & (1<<kEDX)))
        X86EmitDebugTrashReg(kEDX);
    if (!(preservedRegSet & (1<<kECX)))
        X86EmitDebugTrashReg(kECX);
#endif
}

//---------------------------------------------------------------
// Emit code to store the current Thread structure in ebx.
// TRASHES  eax
// RESULTS  ebx = current Thread
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitCurrentThreadFetch()
{
    X86EmitTLSFetch(GetThreadTLSIndex(), kEBX, (1<<kEDX)|(1<<kECX));
}


// fwd decl
Thread* __stdcall CreateThreadBlock();

//---------------------------------------------------------------
// Emit code to store the setup current Thread structure in eax.
// TRASHES  eax,ecx&edx.
// RESULTS  ebx = current Thread
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitSetupThread()
{
    THROWSCOMPLUSEXCEPTION();
    DWORD idx = GetThreadTLSIndex();
    TLSACCESSMODE mode = GetTLSAccessMode(idx);
    CodeLabel *labelThreadSetup;


#ifdef _DEBUG
    {
        static BOOL f = TRUE;
        f = !f;
        if (f) {
           mode = TLSACCESS_GENERIC;
        }
    }
#endif

    switch (mode) {

        case TLSACCESS_GENERIC:
            X86EmitPushImm32(idx);
            // call TLSGetValue
            X86EmitCall(NewExternalCodeLabel((LPVOID) TlsGetValue), 4); // in CE pop 4 bytes or args after the call
            break;
        default:
            _ASSERTE(0);
    }

    // tst eax,eax
    static const BYTE code[] = {0x85, 0xc0};
    EmitBytes(code, sizeof(code));

    labelThreadSetup = NewCodeLabel();
    X86EmitCondJump(labelThreadSetup, X86CondCode::kJNZ);
    X86EmitCall(NewExternalCodeLabel((LPVOID) CreateThreadBlock), 0); // in CE pop no args to pop
    EmitLabel(labelThreadSetup);

#ifdef _DEBUG
    X86EmitDebugTrashReg(kECX);
    X86EmitDebugTrashReg(kEDX);
#endif

}

//---------------------------------------------------------------
// Emits:
//    mov <dstreg>, [<srcreg> + <ofs>]
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitIndexRegLoad(X86Reg dstreg,
                                        X86Reg srcreg,
                                        __int32 ofs)
{
    THROWSCOMPLUSEXCEPTION();
    X86EmitOffsetModRM(0x8b, dstreg, srcreg, ofs);
}


//---------------------------------------------------------------
// Emits:
//    mov [<dstreg> + <ofs>],<srcreg>
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitIndexRegStore(X86Reg dstreg,
                                         __int32 ofs,
                                         X86Reg srcreg)
{
    THROWSCOMPLUSEXCEPTION();
    X86EmitOffsetModRM(0x89, srcreg, dstreg, ofs);
}



//---------------------------------------------------------------
// Emits:
//    push dword ptr [<srcreg> + <ofs>]
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitIndexPush(X86Reg srcreg, __int32 ofs)
{
    THROWSCOMPLUSEXCEPTION();
    X86EmitOffsetModRM(0xff, (X86Reg)0x6, srcreg, ofs);
    Push(4);
}


//---------------------------------------------------------------
// Emits:
//    push dword ptr [<srcreg> + <ofs>]
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitSPIndexPush(__int8 ofs)
{
    THROWSCOMPLUSEXCEPTION();
    BYTE code[] = {0xff, 0x74, 0x24, ofs};
    EmitBytes(code, sizeof(code));
    Push(4);
}


//---------------------------------------------------------------
// Emits:
//    pop dword ptr [<srcreg> + <ofs>]
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitIndexPop(X86Reg srcreg, __int32 ofs)
{
    THROWSCOMPLUSEXCEPTION();
    X86EmitOffsetModRM(0x8f, (X86Reg)0x0, srcreg, ofs);
    Pop(4);
}



//---------------------------------------------------------------
// Emits:
//   sub esp, IMM
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitSubEsp(INT32 imm32)
{
    THROWSCOMPLUSEXCEPTION();
    if (imm32 < 0x1000-100) {
        // As long as the esp size is less than 1 page plus a small
        // safety fudge factor, we can just bump esp.
        X86EmitSubEspWorker(imm32);
    } else {
        // Otherwise, must touch at least one byte for each page.
        while (imm32 >= 0x1000) {

            X86EmitSubEspWorker(0x1000-4);
            X86EmitPushReg(kEAX);

            imm32 -= 0x1000;
        }
        if (imm32 < 500) {
            X86EmitSubEspWorker(imm32);
        } else {
            // If the remainder is large, touch the last byte - again,
            // as a fudge factor.
            X86EmitSubEspWorker(imm32-4);
            X86EmitPushReg(kEAX);
        }

    }

    Push(imm32);

}

//---------------------------------------------------------------
// Emits:
//   sub esp, IMM
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitSubEspWorker(INT32 imm32)
{
    THROWSCOMPLUSEXCEPTION();

    // On Win32, stacks must be faulted in one page at a time.
    _ASSERTE(imm32 < 0x1000);

    if (!imm32) {
        // nop
    } else if (FitsInI1(imm32)) {
        Emit16(0xec83);
        Emit8((INT8)imm32);
    } else {
        Emit16(0xec81);
        Emit32(imm32);
    }
}

//---------------------------------------------------------------
// Emits:
//   add esp, IMM
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitAddEsp(INT32 imm32)
{
    if (!imm32) {
        // nop
    } else if (FitsInI1(imm32)) {
        Emit16(0xc483);
        Emit8((INT8)imm32);
    } else {
        Emit16(0xc481);
        Emit32(imm32);
    }
    Pop(imm32);
}


VOID StubLinkerCPU::X86EmitAddReg(X86Reg reg, INT32 imm32)
{
    _ASSERTE((int) reg < 8);

    if (FitsInI1(imm32)) {
        Emit8(0x83);
        Emit8(0xC0 | reg);
        Emit8((INT8)imm32);
    } else {
        Emit8(0x81);
        Emit8(0xC0 | reg);
        Emit32(imm32);
    }
}


VOID StubLinkerCPU::X86EmitSubReg(X86Reg reg, INT32 imm32)
{
    _ASSERTE((int) reg < 8);

    if (FitsInI1(imm32)) {
        Emit8(0x83);
        Emit8(0xE8 | reg);
        Emit8((INT8)imm32);
    } else {
        Emit8(0x81);
        Emit8(0xE8 | reg);
        Emit32(imm32);
    }
}



//---------------------------------------------------------------
// Emits a MOD/RM for accessing a dword at [<indexreg> + ofs32]
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitOffsetModRM(BYTE opcode, X86Reg opcodereg, X86Reg indexreg, __int32 ofs)
{
    THROWSCOMPLUSEXCEPTION();
    BYTE code[6];
    code[0] = opcode;
    BYTE modrm = (opcodereg << 3) | indexreg;
    if (ofs == 0 && indexreg != kEBP) {
        code[1] = modrm;
        EmitBytes(code, 2);
    } else if (FitsInI1(ofs)) {
        code[1] = 0x40|modrm;
        code[2] = (BYTE)ofs;
        EmitBytes(code, 3);
    } else {
        code[1] = 0x80|modrm;
        *((__int32*)(2+code)) = ofs;
        EmitBytes(code, 6);
    }
}



//---------------------------------------------------------------
// Emits the most efficient form of the operation:
//
//    opcode   altreg, [basereg + scaledreg*scale + ofs]
//
// or
//
//    opcode   [basereg + scaledreg*scale + ofs], altreg
//
// (the opcode determines which comes first.)
//
//
// Limitations:
//
//    scale must be 0,1,2,4 or 8.
//    if scale == 0, scaledreg is ignored.
//    basereg and altreg may be equal to 4 (ESP) but scaledreg cannot
//    for some opcodes, "altreg" may actually select an operation
//      rather than a second register argument.
//    if basereg is EBP, scale must be 0.
//
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitOp(BYTE    opcode,
                              X86Reg  altreg,
                              X86Reg  basereg,
                              __int32 ofs /*=0*/,
                              X86Reg  scaledreg /*=0*/,
                              BYTE    scale /*=0*/)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(scale == 0 || scale == 1 || scale == 2 || scale == 4 || scale == 8);
    _ASSERTE(scaledreg != (X86Reg)4);
    _ASSERTE(!(basereg == kEBP && scale != 0));

    _ASSERTE( ((UINT)basereg)   < 8 );
    _ASSERTE( ((UINT)scaledreg) < 8 );
    _ASSERTE( ((UINT)altreg)    < 8 );


    BYTE modrmbyte = altreg << 3;
    BOOL fNeedSIB  = FALSE;
    BYTE SIBbyte = 0;
    BYTE ofssize;
    BYTE scaleselect= 0;

    if (ofs == 0 && basereg != kEBP) {
        ofssize = 0; // Don't change this constant!
    } else if (FitsInI1(ofs)) {
        ofssize = 1; // Don't change this constant!
    } else {
        ofssize = 2; // Don't change this constant!
    }

    switch (scale) {
        case 1: scaleselect = 0; break;
        case 2: scaleselect = 1; break;
        case 4: scaleselect = 2; break;
        case 8: scaleselect = 3; break;
    }

    if (scale == 0 && basereg != (X86Reg)4 /*ESP*/) {
        // [basereg + ofs]
        modrmbyte |= basereg | (ofssize << 6);
    } else if (scale == 0) {
        // [esp + ofs]
        _ASSERTE(basereg == (X86Reg)4);
        fNeedSIB = TRUE;
        SIBbyte  = 0044;

        modrmbyte |= 4 | (ofssize << 6);
    } else {

        //[basereg + scaledreg*scale + ofs]

        modrmbyte |= 0004 | (ofssize << 6);
        fNeedSIB = TRUE;
        SIBbyte = ( scaleselect << 6 ) | (scaledreg << 3) | (basereg);

    }

    //Some sanity checks:
    _ASSERTE(!(fNeedSIB && basereg == kEBP)); // EBP not valid as a SIB base register.
    _ASSERTE(!( (!fNeedSIB) && basereg == (X86Reg)4 )) ; // ESP addressing requires SIB byte

    Emit8(opcode);
    Emit8(modrmbyte);
    if (fNeedSIB) {
        Emit8(SIBbyte);
    }
    switch (ofssize) {
        case 0: break;
        case 1: Emit8( (__int8)ofs ); break;
        case 2: Emit32( ofs ); break;
        default: _ASSERTE(!"Can't get here.");
    }
}



// Emits
//
//    opcode altreg, modrmreg
//
// or
//
//    opcode modrmreg, altreg
//
// (the opcode determines which one comes first)
//
// For single-operand opcodes, "altreg" actually selects
// an operation rather than a register.

VOID StubLinkerCPU::X86EmitR2ROp(BYTE opcode, X86Reg altreg, X86Reg modrmreg)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE( ((UINT)altreg) < 8 );
    _ASSERTE( ((UINT)modrmreg) < 8 );

    Emit8(opcode);
    Emit8(0300 | (altreg << 3) | modrmreg);
}


//---------------------------------------------------------------
// Emit a MOD/RM + SIB for accessing a DWORD at [esp+ofs32]
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitEspOffset(BYTE opcode, X86Reg altreg, __int32 ofs)
{
    THROWSCOMPLUSEXCEPTION();

    BYTE code[7];

    code[0] = opcode;
    BYTE modrm = (altreg << 3) | 004;
    if (ofs == 0) {
        code[1] = modrm;
        code[2] = 0044;
        EmitBytes(code, 3);
    } else if (FitsInI1(ofs)) {
        code[1] = 0x40|modrm;
        code[2] = 0044;
        code[3] = (BYTE)ofs;
        EmitBytes(code, 4);
    } else {
        code[1] = 0x80|modrm;
        code[2] = 0044;
        *((__int32*)(3+code)) = ofs;
        EmitBytes(code, 7);
    }
}



/*
    This method is dependent on the StubProlog, therefore it's implementation
    is done right next to it.
*/
void FramedMethodFrame::UpdateRegDisplay(const PREGDISPLAY pRD)
{
    CalleeSavedRegisters* regs = GetCalleeSavedRegisters();
    MethodDesc * pFunc = GetFunction();


    // reset pContext; it's only valid for active (top-most) frame

    pRD->pContext = NULL;


    pRD->pEdi = (DWORD*) &regs->edi;
    pRD->pEsi = (DWORD*) &regs->esi;
    pRD->pEbx = (DWORD*) &regs->ebx;
    pRD->pEbp = (DWORD*) &regs->ebp;
    pRD->pPC  = (SLOT*) GetReturnAddressPtr();
    pRD->SP  = (DWORD)((size_t)pRD->pPC + sizeof(void*));



    if (pFunc)
    {
        pRD->SP += (DWORD) pFunc->CbStackPop();
    }

}

void HelperMethodFrame::UpdateRegDisplay(const PREGDISPLAY pRD)
{
        _ASSERTE(m_MachState->isValid());               // InsureInit has been called

    // reset pContext; it's only valid for active (top-most) frame
    pRD->pContext = NULL;

    pRD->pEdi = (DWORD*) m_MachState->pEdi();
    pRD->pEsi = (DWORD*) m_MachState->pEsi();
    pRD->pEbx = (DWORD*) m_MachState->pEbx();
    pRD->pEbp = (DWORD*) m_MachState->pEbp();
    pRD->pPC  = (SLOT*)  m_MachState->pRetAddr();
    pRD->SP   = (DWORD)(size_t)  m_MachState->esp();

        if (m_RegArgs == 0)
                return;

        // If we are promoting arguments, then we should do what the signature
        // tells us to do, instead of what the epilog tells us to do.
        // This is because the helper (and thus the epilog) may be shared and
        // can not pop off the correct number of arguments
	
    MethodDesc * pFunc = GetFunction();
    _ASSERTE(pFunc != 0);
    pRD->SP = (DWORD)(size_t)pRD->pPC + sizeof(void*) + pFunc->CbStackPop();
}


void FaultingExceptionFrame::UpdateRegDisplay(const PREGDISPLAY pRD)
{
    CalleeSavedRegisters* regs = GetCalleeSavedRegisters();

    // reset pContext; it's only valid for active (top-most) frame
    pRD->pContext = NULL;

    pRD->pEdi = (DWORD*) &regs->edi;
    pRD->pEsi = (DWORD*) &regs->esi;
    pRD->pEbx = (DWORD*) &regs->ebx;
    pRD->pEbp = (DWORD*) &regs->ebp;
    pRD->pPC  = (SLOT*) GetReturnAddressPtr();
    pRD->SP   = m_Esp;
}


//===========================================================================
// Emits code to capture the lasterror code.
VOID StubLinkerCPU::EmitSaveLastError()
{
    THROWSCOMPLUSEXCEPTION();

    // push eax (must save return value)
    X86EmitPushReg(kEAX);

    // call GetLastError
    X86EmitCall(NewExternalCodeLabel((LPVOID) GetLastError), 0);

    // mov [ebx + Thread.m_dwLastError], eax
    X86EmitIndexRegStore(kEBX, offsetof(Thread, m_dwLastError), kEAX);

    // pop eax (restore return value)
    X86EmitPopReg(kEAX);
}


void UnmanagedToManagedFrame::UpdateRegDisplay(const PREGDISPLAY pRD)
{
    DWORD *savedRegs = (DWORD *)((size_t)this - (sizeof(CalleeSavedRegisters)));

    // reset pContext; it's only valid for active (top-most) frame

    pRD->pContext = NULL;

    pRD->pEdi = savedRegs++;
    pRD->pEsi = savedRegs++;
    pRD->pEbx = savedRegs++;
    pRD->pEbp = savedRegs++;
    pRD->pPC  = (SLOT*)((BYTE*)this + GetOffsetOfReturnAddress());
    pRD->SP   = (DWORD)(size_t)pRD->pPC + sizeof(void*);

    pRD->SP += (DWORD) GetNumCallerStackBytes();
}



//========================================================================
//  void StubLinkerCPU::EmitSEHProlog(LPVOID pvFrameHandler)
//  Prolog for setting up SEH for stubs that enter managed code from unmanaged
//  assumptions: esi has the current frame pointer
void StubLinkerCPU::EmitSEHProlog(LPVOID pvFrameHandler)
{
    // allocate SEH frame
    X86EmitSubEsp(sizeof(EXCEPTION_REGISTRATION_RECORD));

    // mov eax,esp
    Emit16(0xc48b);

    // save registers
    X86EmitPushReg(kEDX);
    X86EmitPushReg(kECX);

    // mov [eax]EXCEPTION_REGISTRATION_RECORD.Handler, pvFrameHandler
    X86EmitOffsetModRM(0xc7, (X86Reg)0, kEAX, offsetof(EXCEPTION_REGISTRATION_RECORD, Handler));
    Emit32((INT32)(size_t)pvFrameHandler);

    // mov [eax]EXCEPTION_REGISTRATION_RECORD.pvFilterParameter, eax
    X86EmitOffsetModRM(0x89, kEAX, kEAX, offsetof(EXCEPTION_REGISTRATION_RECORD, pvFilterParameter));

    // mov [eax]EXCEPTION_REGISTRATION_RECORD.dwFlags, PAL_EXCEPTION_FLAGS_UNWINDONLY
    X86EmitOffsetModRM(0xc7, (X86Reg)0, kEAX, offsetof(EXCEPTION_REGISTRATION_RECORD, dwFlags));
    Emit32(PAL_EXCEPTION_FLAGS_UNWINDONLY);

    // call PAL_TryHelper
    X86EmitPushReg(kEAX);
    X86EmitCall(NewExternalCodeLabel((LPVOID) PAL_TryHelper), 4);

    // restore registers
    X86EmitPopReg(kECX);
    X86EmitPopReg(kEDX);
}

//===========================================================================
//  void StubLinkerCPU::EmitUnLinkSEH(unsigned offset)
//  negOffset is the offset from the current frame where the next exception record
//  pointer is stored in the stack
//  for e.g. COM to managed frames the pointer to next SEH record is in the stack
//          after the ComMethodFrame::NegSpaceSize() + 4 ( address of handler)
//
//  also assumes ESI is pointing to the current frame
void StubLinkerCPU::EmitUnLinkSEH(unsigned offset)
{
    // lea ecx,[esi+offset]  ;;pointer to the exception record
    X86EmitOffsetModRM(0x8d, kECX, kESI, offset);

    // save registers
    X86EmitPushReg(kEAX);
    X86EmitPushReg(kEDX);

    // call PAL_EndTryHelper(ecx, 0)
    X86EmitPushImm32(0);
    X86EmitPushReg(kECX);
    X86EmitCall(NewExternalCodeLabel((LPVOID) PAL_EndTryHelper), 8);
    
    // restore registers
    X86EmitPopReg(kEDX);
    X86EmitPopReg(kEAX);
}

//========================================================================
//  voidStubLinkerCPU::EmitComMethodStubProlog()
//  Prolog for entering managed code from COM
//  pushes the appropriate frame ptr
//  sets up a thread and returns a label that needs to be emitted by the caller
void StubLinkerCPU::EmitComMethodStubProlog(LPVOID pFrameVptr,
                                            CodeLabel** rgRareLabels,
                                            CodeLabel** rgRejoinLabels,
                                            LPVOID pSEHHandler,
                                            BOOL bShouldProfile)
{
    _ASSERTE(rgRareLabels != NULL);
    _ASSERTE(rgRareLabels[0] != NULL && rgRareLabels[1] != NULL && rgRareLabels[2] != NULL);
    _ASSERTE(rgRejoinLabels != NULL);
    _ASSERTE(rgRejoinLabels[0] != NULL && rgRejoinLabels[1] != NULL && rgRejoinLabels[2] != NULL);

    // push edx ;leave room for m_next (edx is an arbitrary choice)
    X86EmitPushReg(kEDX);

    // push IMM32 ; push Frame vptr
    X86EmitPushImm32((UINT)(size_t)pFrameVptr);

    // push ebp     ;; save callee-saved register
    // push ebx     ;; save callee-saved register
    // push esi     ;; save callee-saved register
    // push edi     ;; save callee-saved register
    X86EmitPushReg(kEBP);
    X86EmitPushReg(kEBX);
    X86EmitPushReg(kESI);
    X86EmitPushReg(kEDI);

    // lea esi, [esp+0x10]  ;; set ESI -> new frame
    static const BYTE code10[] = {0x8d, 0x74, 0x24, 0x10 };
    EmitBytes(code10 ,sizeof(code10));

#ifdef _DEBUG

    //======================================================================
    // Under DEBUG, set up just enough of a standard C frame so that
    // the VC debugger can stacktrace through stubs.
    //======================================================================


    //  mov eax, [esi+Frame.retaddr]
    static const BYTE code20[] = {0x8b, 0x44, 0x24};
    EmitBytes(code20, sizeof(code20));
    Emit8(UnmanagedToManagedFrame::GetOffsetOfReturnAddress());

    //  push eax        ;; push return address
    //  push ebp        ;; push previous ebp
    X86EmitPushReg(kEAX);
    X86EmitPushReg(kEBP);

    // mov ebp,esp
    Emit8(0x8b);
    Emit8(0xec);


#endif

    // Emit Setup thread
    X86EmitSetup(rgRareLabels[0]);  // rareLabel for rare setup
    EmitLabel(rgRejoinLabels[0]); // rejoin label for rare setup

    // push auxilary information

    // xor eax, eax
    X86EmitZeroOutReg(kEAX);

    // push eax ;push NULL for protected Marshalers
    X86EmitPushReg(kEAX);

    // push eax ;push null for GC flag
    X86EmitPushReg(kEAX);

    // push eax ;push null for ptr to args
    X86EmitPushReg(kEAX);

    // push eax ;push NULL for pReturnDomain
    X86EmitPushReg(kEAX);

    // push eax ;push NULL for CleanupWorkList->m_pnode
    X86EmitPushReg(kEAX);

    //-----------------------------------------------------------------------
    // Generate the inline part of disabling preemptive GC.  It is critical
    // that this part happen before we link in the frame.  That's because
    // we won't be able to unlink the frame from preemptive mode.  And during
    // shutdown, we cannot switch to cooperative mode under some circumstances
    //-----------------------------------------------------------------------
    EmitDisable(rgRareLabels[1]);        // rare disable gc
    EmitLabel(rgRejoinLabels[1]);        // rejoin for rare disable gc

     // mov edi,[ebx + Thread.GetFrame()]  ;; get previous frame
    X86EmitIndexRegLoad(kEDI, kEBX, Thread::GetOffsetOfCurrentFrame());

    // mov [esi + Frame.m_next], edi
    X86EmitIndexRegStore(kESI, Frame::GetOffsetOfNextLink(), kEDI);

    // mov [ebx + Thread.GetFrame()], esi
    X86EmitIndexRegStore(kEBX, Thread::GetOffsetOfCurrentFrame(), kESI);

#if _DEBUG
        // call LogTransition
    X86EmitPushReg(kESI);
    X86EmitCall(NewExternalCodeLabel((LPVOID) Frame::LogTransition), 4);
#endif

    if (pSEHHandler)
    {
        // fixup for ComToManagedExRecord::m_tct
        X86EmitSubEsp(sizeof(ThrowCallbackType));

        EmitSEHProlog(pSEHHandler);
    }

#ifdef PROFILING_SUPPORTED
    // If profiling is active, emit code to notify profiler of transition
    // Must do this before preemptive GC is disabled, so no problem if the
    // profiler blocks.
    if (CORProfilerTrackTransitions() && bShouldProfile)
    {
        EmitProfilerComCallProlog(pFrameVptr, /*Frame*/ kESI);
    }
#endif // PROFILING_SUPPORTED
}


//========================================================================
//  void StubLinkerCPU::EmitEnterManagedStubEpilog(unsigned numStackBytes,
//                      CodeLabel** rgRareLabels, CodeLabel** rgRejoinLabels)
//  Epilog for stubs that enter managed code from unmanaged
void StubLinkerCPU::EmitEnterManagedStubEpilog(LPVOID pFrameVptr, unsigned numStackBytes,
                        CodeLabel** rgRareLabel, CodeLabel** rgRejoinLabel,
                        BOOL bShouldProfile)
{
    _ASSERTE(rgRareLabel != NULL);
    _ASSERTE(rgRareLabel[0] != NULL && rgRareLabel[1] != NULL && rgRareLabel[2] != NULL);
    _ASSERTE(rgRejoinLabel != NULL);
    _ASSERTE(rgRejoinLabel[0] != NULL && rgRejoinLabel[1] != NULL && rgRejoinLabel[2] != NULL);

    // mov [ebx + Thread.GetFrame()], edi  ;; restore previous frame
    X86EmitIndexRegStore(kEBX, Thread::GetOffsetOfCurrentFrame(), kEDI);

    //-----------------------------------------------------------------------
    // Generate the inline part of disabling preemptive GC
    //-----------------------------------------------------------------------
    EmitEnable(rgRareLabel[2]); // rare gc
    EmitLabel(rgRejoinLabel[2]);        // rejoin for rare gc

#ifdef PROFILING_SUPPORTED
    // If profiling is active, emit code to notify profiler of transition
    if (CORProfilerTrackTransitions() && bShouldProfile)
    {
        EmitProfilerComCallEpilog(pFrameVptr, kESI);
    }
#endif // PROFILING_SUPPORTED

    #ifdef _DEBUG
        // add esp, SIZE VC5Frame     ;; pop the stacktrace info for VC5
        X86EmitAddEsp(sizeof(VC5Frame));
    #endif

    // pop edi        ; restore callee-saved registers
    // pop esi
    // pop ebx
    // pop ebp
    X86EmitPopReg(kEDI);
    X86EmitPopReg(kESI);
    X86EmitPopReg(kEBX);
    X86EmitPopReg(kEBP);

    // add esp,popstack     ; deallocate frame + MethodDesc
    unsigned popStack = sizeof(Frame) + sizeof(MethodDesc*);
    X86EmitAddEsp(popStack);

    //retn n
    X86EmitReturn(numStackBytes);

    //-----------------------------------------------------------------------
    // The out-of-line portion of enabling preemptive GC - rarely executed
    //-----------------------------------------------------------------------
    EmitLabel(rgRareLabel[2]);  // label for rare enable gc
    EmitRareEnable(rgRejoinLabel[2]); // emit rare enable gc

    //-----------------------------------------------------------------------
    // The out-of-line portion of disabling preemptive GC - rarely executed
    //-----------------------------------------------------------------------
    EmitLabel(rgRareLabel[1]);  // label for rare disable gc
    EmitRareDisable(rgRejoinLabel[1], /*bIsCallIn=*/TRUE); // emit rare disable gc

    //-----------------------------------------------------------------------
    // The out-of-line portion of setup thread - rarely executed
    //-----------------------------------------------------------------------
    EmitLabel(rgRareLabel[0]);  // label for rare setup thread
    EmitRareSetup(rgRejoinLabel[0]); // emit rare setup thread
}


//========================================================================
//  Epilog for stubs that enter managed code from COM
//
void StubLinkerCPU::EmitComMethodStubEpilog(LPVOID pFrameVptr,
                                            unsigned numStackBytes,
                                            CodeLabel** rgRareLabels,
                                            CodeLabel** rgRejoinLabels,
                                            LPVOID pSEHHandler,
                                            BOOL bShouldProfile)
{
    if (!pSEHHandler)
    {
        X86EmitAddEsp(sizeof(UnmanagedToManagedCallFrame::NegInfo));
    }
    else
    {
        // oh well, if we are using exceptions, unlink the SEH and
        // just reset the esp to where EnterManagedStubEpilog likes it to be

        // unlink SEH
        EmitUnLinkSEH(0-(UnmanagedToManagedCallFrame::GetNegSpaceSize()
                +sizeof(EXCEPTION_REGISTRATION_RECORD)
                // fixup for ComToManagedExRecord::m_tct
                +sizeof(ThrowCallbackType)
            ));

        // reset esp
        // lea esp,[esi - PLATFORM_FRAME_ALIGN(sizeof(CalleeSavedRegisters) + VC5FRAME_SIZE)]
        X86EmitOffsetModRM(0x8d, (X86Reg)4 /*kESP*/, kESI, 0-PLATFORM_FRAME_ALIGN(sizeof(CalleeSavedRegisters) + VC5FRAME_SIZE));
    }

    EmitEnterManagedStubEpilog(pFrameVptr, numStackBytes,
                              rgRareLabels, rgRejoinLabels, bShouldProfile);
}


/*
    If you make any changes to the prolog instruction sequence, be sure
    to update UpdateRegdisplay, too!!  This service should only be called from
    within the runtime.  It should not be called for any unmanaged -> managed calls in.
*/
VOID StubLinkerCPU::EmitMethodStubProlog(LPVOID pFrameVptr)
{
    THROWSCOMPLUSEXCEPTION();

    // push edx ;leave room for m_next (edx is an arbitrary choice)
    X86EmitPushReg(kEDX);

    // push Frame vptr
    X86EmitPushImm32((UINT)(size_t)pFrameVptr);

    // push ebp     ;; save callee-saved register
    // push ebx     ;; save callee-saved register
    // push esi     ;; save callee-saved register
    // push edi     ;; save callee-saved register
    X86EmitPushReg(kEBP);
    X86EmitPushReg(kEBX);
    X86EmitPushReg(kESI);
    X86EmitPushReg(kEDI);

    // lea esi, [esp+0x10]  ;; set ESI -> new frame
    static const BYTE code10[] = {0x8d, 0x74, 0x24, 0x10 };
    EmitBytes(code10 ,sizeof(code10));

#ifdef _DEBUG

    //======================================================================
    // Under DEBUG, set up just enough of a standard C frame so that
    // the VC debugger can stacktrace through stubs.
    //======================================================================

    //  push dword ptr [esi+Frame.retaddr]
    X86EmitIndexPush(kESI, FramedMethodFrame::GetOffsetOfReturnAddress());

    //  push ebp
    X86EmitPushReg(kEBP);

    // mov ebp,esp
    Emit8(0x8b);
    Emit8(0xec);


#endif


    // Push & initialize ArgumentRegisters
#define DEFINE_ARGUMENT_REGISTER(regname) X86EmitPushReg(k##regname);
#include "eecallconv.h"

    // ebx <-- GetThread()
    X86EmitCurrentThreadFetch();

#if _DEBUG
        // call ObjectRefFlush
    X86EmitPushReg(kEBX);
    X86EmitCall(NewExternalCodeLabel((LPVOID) Thread::ObjectRefFlush), 4);
#endif

    // mov edi,[ebx + Thread.GetFrame()]  ;; get previous frame
    X86EmitIndexRegLoad(kEDI, kEBX, Thread::GetOffsetOfCurrentFrame());

    // mov [esi + Frame.m_next], edi
    X86EmitIndexRegStore(kESI, Frame::GetOffsetOfNextLink(), kEDI);

    // mov [ebx + Thread.GetFrame()], esi
    X86EmitIndexRegStore(kEBX, Thread::GetOffsetOfCurrentFrame(), kESI);

#if _DEBUG
        // call LogTransition
    X86EmitPushReg(kESI);
    X86EmitCall(NewExternalCodeLabel((LPVOID) Frame::LogTransition), 4);
#endif

    // OK for the debugger to examine the new frame now
    // (Note that if it's not OK yet for some stub, another patch label
    // can be emitted later which will override this one.)
    EmitPatchLabel();
}

VOID StubLinkerCPU::EmitMethodStubEpilog(__int16 numArgBytes, StubStyle style,
                                         __int16 shadowStackArgBytes)
{
    THROWSCOMPLUSEXCEPTION();

    numArgBytes = 0;

    _ASSERTE(style == kNoTripStubStyle ||
             style == kInterceptorStubStyle);        // the only ones this code knows about.



    // mov [ebx + Thread.GetFrame()], edi  ;; restore previous frame
    X86EmitIndexRegStore(kEBX, Thread::GetOffsetOfCurrentFrame(), kEDI);

    X86EmitAddEsp(ARGUMENTREGISTERS_SIZE + shadowStackArgBytes);

#ifdef _DEBUG
    // add esp, SIZE VC5Frame     ;; pop the stacktrace info for VC5
    X86EmitAddEsp(sizeof(VC5Frame));
#endif



    // pop edi        ; restore callee-saved registers
    // pop esi
    // pop ebx
    // pop ebp
    X86EmitPopReg(kEDI);
    X86EmitPopReg(kESI);
    X86EmitPopReg(kEBX);
    X86EmitPopReg(kEBP);


    if (numArgBytes == -1) {
        // This stub is called for methods with varying numbers of bytes on the
        // stack.  The correct number to pop is expected to now be sitting on
        // the stack.
        //
        // shift the retaddr & stored EDX:EAX return value down on the stack
        // and then toss the variable number of args pushed by the caller.
        // Of course, the slide must occur backwards.

        // add     esp,8                ; deallocate frame
        // pop     ecx                  ; scratch register gets delta to pop
        // push    eax                  ; running out of registers!
        // mov     eax, [esp+4]         ; get retaddr
        // mov     [esp+ecx+4], eax     ; put it where it belongs
        // pop     eax                  ; restore retval
        // add     esp, ecx             ; pop all the args
        // ret

        X86EmitAddEsp(sizeof(Frame));

        X86EmitPopReg(kECX);
        X86EmitPushReg(kEAX);

        static const BYTE arbCode1[] = { 0x8b, 0x44, 0x24, 0x04, // mov eax, [esp+4]
                                   0x89, 0x44, 0x0c, 0x04, // mov [esp+ecx+4], eax
                                 };

        EmitBytes(arbCode1, sizeof(arbCode1));
        X86EmitPopReg(kEAX);

        static const BYTE arbCode2[] = { 
                                   0x03, 0xe1,             // add esp, ecx
                                   0xc3,                   // ret
                                 };

        EmitBytes(arbCode2, sizeof(arbCode2));
    }
    else {
        _ASSERTE(numArgBytes >= 0);

        // add esp,12     ; deallocate frame + MethodDesc
        X86EmitAddEsp(sizeof(Frame) + sizeof(MethodDesc*));

        if(style != kInterceptorStubStyle) {

            X86EmitReturn(numArgBytes);

        }
    }
}


VOID StubLinkerCPU::EmitRareSetup(CodeLabel *pRejoinPoint)
{
    THROWSCOMPLUSEXCEPTION();

    X86EmitCall(NewExternalCodeLabel((LPVOID) CreateThreadBlock), 0);

    // mov ebx,eax
     Emit16(0xc389);
    X86EmitNearJump(pRejoinPoint);
}

//---------------------------------------------------------------
// Emit code to store the setup current Thread structure in eax.
// TRASHES  eax,ecx&edx.
// RESULTS  ebx = current Thread
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitSetup(CodeLabel *pForwardRef)
{
    THROWSCOMPLUSEXCEPTION();
    DWORD idx = GetThreadTLSIndex();
    TLSACCESSMODE mode = GetTLSAccessMode(idx);


#ifdef _DEBUG
    {
        static BOOL f = TRUE;
        f = !f;
        if (f) {
           mode = TLSACCESS_GENERIC;
        }
    }
#endif

    switch (mode) {

        case TLSACCESS_GENERIC:
            X86EmitPushImm32(idx);

            // call TLSGetValue
            X86EmitCall(NewExternalCodeLabel((LPVOID) TlsGetValue), 4); // in CE pop 4 bytes or args after the call
            // mov ebx,eax
            Emit16(0xc389);
            break;
        default:
            _ASSERTE(0);
    }

  // cmp ebx, 0
   static const BYTE b[] = { 0x83, 0xFB, 0x0};

    EmitBytes(b, sizeof(b));

    // jz RarePath
    X86EmitCondJump(pForwardRef, X86CondCode::kJZ);

#ifdef _DEBUG
    X86EmitDebugTrashReg(kECX);
    X86EmitDebugTrashReg(kEDX);
#endif

}

// This method unboxes the THIS pointer and then calls pRealMD
VOID StubLinkerCPU::EmitUnboxMethodStub(MethodDesc* pUnboxMD)
{
    // unboxing a value class simply means adding 4 to the THIS pointer
    // Add 4 to Arg1 which is assumed to be at [esp+4]
    static const BYTE addEsp[] = { 0x83, 0x44, 0x24, 0x04, 0x04 };   // add dword ptr [esp+4], 4
    EmitBytes(addEsp, sizeof(addEsp));
    

    // If it is an ECall, m_CodeOrIL does not reflect the correct address to
    // call to (which is an ECall stub).  Rather, it reflects the actual ECall
    // implementation.  Naturally, ECalls must always hit the stub first.
    // Along the same lines, perhaps the method isn't JITted yet.  The easiest
    // way to handle all this is to simply dispatch through the top of the MD.

    Emit8(0xB8);                                        // MOV EAX, pre stub call addr
    Emit32((__int32)(size_t)getStubCallAddr(pUnboxMD));
    Emit16(0xE0FF);                                     // JMP EAX
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

    UINT32 negspacesize = InterceptorFrame::GetNegSpaceSize() -
                          FramedMethodFrame::GetNegSpaceSize();

    // make room for negspace fields of IFrame
    X86EmitSubEsp(negspacesize);

    // push arguments for DoDeclaritiveSecurity(MethodDesc*, DeclActionInfo*, InterceptorFrame*);
    X86EmitPushReg(kESI);            // push esi (push new frame as ARG)
    X86EmitPushImm32((UINT)(size_t)pActions);
    X86EmitPushImm32((UINT)(size_t)pMD);

#ifdef _DEBUG
    // push IMM32 ; push SecurityMethodStubWorker
    X86EmitPushImm32((UINT)(size_t)DoDeclarativeSecurity);

    X86EmitCall(NewExternalCodeLabel((LPVOID) WrapCall), 12); // in CE pop 4 bytes or args after the call
#else
    X86EmitCall(NewExternalCodeLabel((LPVOID) DoDeclarativeSecurity), 12); // in CE pop 4 bytes or args after the call
#endif

    // Copy the arguments, calculate the offset
    //     terminology:  opt.          - Optional
    //                   Sec. Desc.    - Security Descriptor
    //                   desc          - Descriptor
    //                   rtn           - Return
    //                   addr          - Address
    //
    //          method desc    <-- copied from below
    //         ------------
    //           rtn addr      <-- points back into the wrapper stub
    //         ------------
    //            copied       <-- copied from below
    //             args
    //         ------------   -|
    //          Sec. Desc      |
    //         ------------   -|
    //          Arg. Registers |
    //         ------------   -|
    //          S. Stack Ptr   |
    //         ------------   -|
    //             EBP (opt.)  |
    //             EAX (opt.)  |
    //         ------------    |- Security Negitive Space (for frame)
    //             EDI         |
    //             ESI         |
    //             EBX         |
    //             EBP         |
    //         ------------ -----                -
    //            vtable                         |
    //         ------------                      |
    //            next                           |-- Security Frame
    //         ------------   --                 |
    //          method desc    |                 |
    //         ------------    |                 |
    //           rtn addr <-|  |--Original Stack |
    //         ------------ |  |                 -
    //         | original | |  :
    //         |  args    | |
    //                      ------- Points to the real return addr.
    //
    //
    //

    // Offset from original args to new args. (see above). We are copying from
    // the bottom of the stack to top. The offset is calculated to repush the
    // the arguments on the stack.
    //
    // offset = Negitive Space + return addr + method des + next + vtable + size of args - 4
    // The 4 is subtracted because the esp is one slot below the start of the copied args.
    UINT32 offset = InterceptorFrame::GetNegSpaceSize() + sizeof(InterceptorFrame) - 4 + numStackBytes;

    // Convert bytes to number of slots
    int args  = numStackBytes >> 2;

    // Emit the required number of pushes to copy the arguments
    while(args) {
        X86EmitSPIndexPush(offset);
        args--;
    }

    // Add a jmp to the main call, this adds our current EIP+4 to the stack
    CodeLabel* mainCall;
    mainCall = NewCodeLabel();
    X86EmitCall(mainCall, 0);

    // Jump past the call into the real stub we have already been there
    // The return addr into the stub points to this jump statement
    //
    CodeLabel* continueCall;
    continueCall = NewCodeLabel();
    X86EmitNearJump(continueCall);

    // Main Call label attached to the real stubs call
    EmitLabel(mainCall);

    // push the address of the method descriptor for the interpreted case only
    //  push dword ptr [esp+offset] and add four bytes for that case
    if(fToStub) {
        X86EmitSPIndexPush(offset);
        offset += 4;
    }

    // Set up for arguments in stack, offset is 8 bytes below base of frame.
    // Call GetOffsetOfArgumentRegisters to move back from base of frame
    offset = offset - 8 + InterceptorFrame::GetOffsetOfArgumentRegisters();

    // Move to the last register in the space used for registers
    offset += NUM_ARGUMENT_REGISTERS * sizeof(UINT32) - 4;

    // Push the args on the stack, as esp is incremented and the
    // offset stays the same all the register values are pushed on
    // the correct registers
    for(int i = 0; i < NUM_ARGUMENT_REGISTERS; i++)
        X86EmitSPIndexPush(offset);

    // This generates the appropriate pops into the registers specified in eecallconv.h
#define DEFINE_ARGUMENT_REGISTER_BACKWARD(regname) X86EmitPopReg(k##regname);
#include "eecallconv.h"

    // Add a jump to the real stub, we will return to
    // the jump statement added  above
    X86EmitNearJump(NewExternalCodeLabel(pRealStub));

    // we will continue on past the real stub
    EmitLabel(continueCall);

    // Clear the arguments to the functions from the stack
    // This needs to be done here because the stub has pushed a copy of the
    // original arguments onto the stack.
    X86EmitAddEsp(numStackBytes);

    // deallocate negspace fields of IFrame
    X86EmitAddEsp(negspacesize);

    // Return poping of the same number of bytes that
    // the real stub would have popped.
    EmitMethodStubEpilog(numStackBytes, kNoTripStubStyle);
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

    UINT32 negspacesize = InterceptorFrame::GetNegSpaceSize() -
                          FramedMethodFrame::GetNegSpaceSize();

    // make room for negspace fields of IFrame
    X86EmitSubEsp(negspacesize);

    // push arguments for DoDeclaritiveSecurity(MethodDesc*, DeclActionInfo*, InterceptorFrame*);
    X86EmitPushReg(kESI);            // push esi (push new frame as ARG)
    X86EmitPushImm32((UINT)(size_t)pActions);
    X86EmitPushImm32((UINT)(size_t)pMD);

#ifdef _DEBUG
    // push IMM32 ; push SecurityMethodStubWorker
    X86EmitPushImm32((UINT)(size_t)DoDeclarativeSecurity);
    X86EmitCall(NewExternalCodeLabel((LPVOID) WrapCall), 12); // in CE pop 4 bytes or args after the call
#else
    X86EmitCall(NewExternalCodeLabel((LPVOID) DoDeclarativeSecurity), 12); // in CE pop 4 bytes or args after the call
#endif

    // Get the number arguments stored in registers. For now we are doing the
    // simple approach of saving off all the registers and then poping them off.
    // This needs to be cleaned up for the real stubs or when CallDescr is done
    // correctly


    // Clean up security frame, this rips off the MD and gets the real return addr at the top of the stack
    X86EmitAddEsp(negspacesize);
    EmitMethodStubEpilog(0, kInterceptorStubStyle);

    // Add the phoney return address back on the stack so the real
    // stub can ignore it also.
    if(fToStub)
        X86EmitSubEsp(4);

    // Add a jump to the real stub
    X86EmitNearJump(NewExternalCodeLabel(pRealStub));
}



#ifdef _DEBUG
//---------------------------------------------------------------
// Emits:
//     mov <reg32>,0xcccccccc
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitDebugTrashReg(X86Reg reg)
{
    THROWSCOMPLUSEXCEPTION();
    Emit8(0xb8|reg);
    Emit32(0xcccccccc);
}
#endif //_DEBUG



Thread* __stdcall CreateThreadBlock()
{
        Thread* pThread = SetupThread();
        if(pThread == NULL) return(pThread);

    return pThread;
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

    _ASSERTE(*(void**)returnSP == returnIP);

    return ip;
}

#endif  // _DEBUG

//===========================================================================
// create prestub
Stub * GeneratePrestub()
{
    CPUSTUBLINKER sl;
    CPUSTUBLINKER *psl = &sl;

    psl->EmitMethodStubProlog(PrestubMethodFrame::GetMethodFrameVPtr());

    // push the new frame as an argument and call PreStubWorker.
    psl->X86EmitPushReg(kESI);
    psl->X86EmitCall(psl->NewExternalCodeLabel((LPVOID) PreStubWorker), 4);

    // eax now contains replacement stub. PreStubWorker will never return
    // NULL (it throws an exception if stub creation fails.)

    // Debugger patch location
    psl->EmitPatchLabel();

    // mov [ebx + Thread.GetFrame()], edi  ;; restore previous frame
    psl->X86EmitIndexRegStore(kEBX, Thread::GetOffsetOfCurrentFrame(), kEDI);

    // Save the replacement stuff in the space that Frame.Next used to occupy
    psl->X86EmitIndexRegStore(kESI, sizeof(Frame) - sizeof(LPVOID), kEAX);

    // Pop ArgumentRegisters structure, while restoring the actual
    // machine registers.
    #define DEFINE_ARGUMENT_REGISTER_BACKWARD(regname) psl->X86EmitPopReg(k##regname);
    #include "eecallconv.h"

    // !!! From here on, mustn't trash eax, ecx or edx.

#ifdef _DEBUG
    // Deallocate VC stack trace info
    psl->X86EmitAddEsp(sizeof(VC5Frame));
#endif

    //--------------------------------------------------------------------------
    // Pop CalleeSavedRegisters structure, while restoring the actual machine registers.
    //--------------------------------------------------------------------------
    psl->X86EmitPopReg(kEDI);
    psl->X86EmitPopReg(kESI);
    psl->X86EmitPopReg(kEBX);
    psl->X86EmitPopReg(kEBP);

    //--------------------------------------------------------------------------
    //!!! From here on, can't trash ANY register other than esp & eip.
    //--------------------------------------------------------------------------

    // Pop off the Frame structure *except* for the "next" field
    // which has been overwritten with the new address to jump to.
    psl->X86EmitAddEsp(sizeof(Frame) - sizeof(LPVOID));

    // Pop off methodref - this allows us to remove pop ecx from the jitted code
    //   pop dword ptr [esp]
    psl->Emit8(0x8f);
    psl->Emit16(0x2404);

    // Now, jump to the new address.
    //    retn
    psl->Emit8(0xc3);

    return psl->Link();
}

//-------------------------------------------------------------------------
// One-time creation of special prestub to initialize UMEntryThunks.
//-------------------------------------------------------------------------
Stub *GenerateUMThunkPrestub()
{
    THROWSCOMPLUSEXCEPTION();

    CPUSTUBLINKER sl;

    CodeLabel* rgRareLabels[] = { sl.NewCodeLabel(),
                                  sl.NewCodeLabel(),
                                  sl.NewCodeLabel()
                                };


    CodeLabel* rgRejoinLabels[] = { sl.NewCodeLabel(),
                                    sl.NewCodeLabel(),
                                    sl.NewCodeLabel()
                                };


    CodeLabel *pWrapLabel = sl.NewCodeLabel();

    //    push eax   // push UMEntryThunk
    sl.X86EmitPushReg(kEAX);

    //    push ecx      (in case this is a _thiscall: need to protect this register)
    sl.X86EmitPushReg(kECX);

    // Wrap puts a fake return address and a duplicate copy
    // of pUMEntryThunk on the stack, then falls thru to the regular
    // stub prolog. The stub prolog is fooled into thinkin this duplicate
    // copy is the real return address and UMEntryThunk.
    //
    //    call wrap. 
    sl.X86EmitCall(pWrapLabel, 0);


    //    pop  ecx
    sl.X86EmitPopReg(kECX);

    // Now we've executed the prestub and fixed up UMEntryThunk. The
    // duplicate data has been popped off.
    //
    //    pop eax   // pop UMEntryThunk
    sl.X86EmitPopReg(kEAX);

    //    lea eax, [eax + UMEntryThunk.m_code]  // point to fixedup UMEntryThunk
    sl.X86EmitOp(0x8d, kEAX, kEAX, offsetof(UMEntryThunk, m_code) + UMEntryThunkCode::GetEntryPointOffset());

    //    jmp eax //reexecute!
    sl.X86EmitR2ROp(0xff, (X86Reg)4, kEAX);

    sl.EmitLabel(pWrapLabel);

    // Wrap:
    //
    //   push [esp+8]  //repush UMEntryThunk
    sl.X86EmitEspOffset(0xff, (X86Reg)6, 8);

    // emit the initial prolog
    sl.EmitComMethodStubProlog(UMThkCallFrame::GetUMThkCallFrameVPtr(), rgRareLabels, rgRejoinLabels,
                               (LPVOID) UMThunkPrestubHandler, FALSE /*Don't profile*/);

    // mov ecx, [esi+UMThkCallFrame.pUMEntryThunk]
    sl.X86EmitIndexRegLoad(kECX, kESI, UMThkCallFrame::GetOffsetOfUMEntryThunk());

    // The call conv is a __stdcall   
    sl.X86EmitPushReg(kECX);

    // call UMEntryThunk::RuntimeInit
    sl.X86EmitCall(sl.NewExternalCodeLabel((LPVOID)UMEntryThunk::DoRunTimeInit), 4);

    sl.EmitComMethodStubEpilog(UMThkCallFrame::GetUMThkCallFrameVPtr(), 0, rgRareLabels, rgRejoinLabels,
                               (LPVOID) UMThunkPrestubHandler, FALSE /*Don't profile*/);

    return sl.Link();
}

/*static*/ void NDirect::CreateGenericNDirectStubSys(CPUSTUBLINKER *psl, UINT numStackBytes)
{
    psl->EmitMethodStubProlog(NDirectMethodFrameGeneric::GetMethodFrameVPtr());

    _ASSERTE(sizeof(CleanupWorkList) == sizeof(LPVOID));

    // push 00000000    ;; pushes a CleanupWorkList.
    psl->X86EmitPushImm32(0);

    psl->X86EmitPushReg(kESI);       // push esi (push new frame as ARG)
    psl->X86EmitPushReg(kEBX);       // push ebx (push current thread as ARG)

#ifdef _DEBUG
    // push IMM32 ; push NDirectGenericStubWorker
    psl->X86EmitPushImm32((UINT)(size_t)NDirectGenericStubWorker);
    psl->X86EmitCall(psl->NewExternalCodeLabel((LPVOID) WrapCall), 8); // in CE pop 8 bytes or args on return from call
#else
    psl->X86EmitCall(psl->NewExternalCodeLabel((LPVOID) NDirectGenericStubWorker), 8); // in CE pop 8 bytes or args on return from call
#endif

    // Pop off cleanup worker
    psl->X86EmitAddEsp(sizeof(CleanupWorkList));

    psl->EmitMethodStubEpilog(numStackBytes, kNoTripStubStyle);
}


/*static*/ Stub* CreateGenericNExportStub(UINT cbRetPop, 
    BOOL fHashThisCallAdjustment, BOOL fHashThisCallHiddenArg)
{
    CPUSTUBLINKER sl;
    CPUSTUBLINKER *psl = &sl;
    
    CodeLabel* rgRareLabels[] = { psl->NewCodeLabel(),
                                    psl->NewCodeLabel(),
                                    psl->NewCodeLabel()
                                };


    CodeLabel* rgRejoinLabels[] = { psl->NewCodeLabel(),
                                    psl->NewCodeLabel(),
                                    psl->NewCodeLabel()
                                };

    if (fHashThisCallAdjustment) {
        if (fHashThisCallHiddenArg) { 
                    
            // pop off the return address
            psl->X86EmitPopReg(kEDX);

            // exchange ecx (this "this") with the hidden structure return buffer
            //  xchg ecx, [esp]
            psl->X86EmitOp(0x87, kECX, (X86Reg)4 /*ESP*/);

            // push ecx
            psl->X86EmitPushReg(kECX);

            // push edx
            psl->X86EmitPushReg(kEDX);
        }
        else
        {
            // pop off the return address
            psl->X86EmitPopReg(kEDX);

            // jam ecx (the "this" param onto stack. Now it looks like a normal stdcall.)
            psl->X86EmitPushReg(kECX);

            // repush
            psl->X86EmitPushReg(kEDX);
        }
    }

    // push UMEntryThunk
    psl->X86EmitPushReg(kEAX);

    // emit the initial prolog
    psl->EmitComMethodStubProlog(UMThkCallFrame::GetUMThkCallFrameVPtr(), rgRareLabels, rgRejoinLabels,
                                    (LPVOID) UMThunkPrestubHandler, TRUE /*Profile*/);

    psl->X86EmitPushReg(kESI);       // push frame as an ARG
    psl->X86EmitPushReg(kEBX);       // push ebx (push current thread as ARG)

    psl->X86EmitCall(psl->NewExternalCodeLabel((LPVOID) DoUMThunkCall), 8); // on CE pop 8 bytes or args on return

    psl->EmitComMethodStubEpilog(UMThkCallFrame::GetUMThkCallFrameVPtr(), cbRetPop, rgRareLabels,
                                    rgRejoinLabels, (LPVOID) UMThunkPrestubHandler, TRUE /*Profile*/);

    return psl->Link();
}


// Here's the support for the interlocked operations.  The external view of 
// them is declared in util.hpp.

// These are implemented in assembly code, jithelp.asm/jithelp.s
extern "C" {
    void __fastcall OrMaskUP(DWORD volatile *Target, const int Bits);
    void __fastcall OrMaskMP(DWORD volatile *Target, const int Bits);
    void __fastcall AndMaskUP(DWORD volatile *Target, const int Bits);
    void __fastcall AndMaskMP(DWORD volatile *Target, const int Bits);
    LONG __fastcall ExchangeUP(LONG volatile *Target, LONG Value);
    LONG __fastcall ExchangeMP(LONG volatile *Target, LONG Value);
    PVOID __fastcall CompareExchangeUP(PVOID volatile *Target, PVOID Exchange, PVOID Comperand);
    PVOID __fastcall CompareExchangeMP(PVOID volatile *Target, PVOID Exchange, PVOID Comperand);
    LONG __fastcall ExchangeAddUP(LONG volatile *Target, LONG Value);
    LONG __fastcall ExchangeAddMP(LONG volatile *Target, LONG Value);
    LONG __fastcall IncrementUP(LONG volatile *Target);
    LONG __fastcall IncrementMP(LONG volatile *Target);
    LONG __fastcall DecrementUP(LONG volatile *Target);
    LONG __fastcall DecrementMP(LONG volatile *Target);
    UINT64 __fastcall IncrementLongUP(UINT64 volatile *Target);
    UINT64 __fastcall IncrementLongMP(UINT64 volatile *Target);
    UINT64 __fastcall DecrementLongUP(UINT64 volatile *Target);
    UINT64 __fastcall DecrementLongMP(UINT64 volatile *Target);
    UINT64 __fastcall IncrementLongMP8b(UINT64 volatile *Target);
    UINT64 __fastcall DecrementLongMP8b(UINT64 volatile *Target);
}

BitFieldOps FastInterlockOr = OrMaskUP;
BitFieldOps FastInterlockAnd = AndMaskUP;

XchgOps         FastInterlockExchange = ExchangeUP;
XchgOpsPtr      FastInterlockExchangePointer = (XchgOpsPtr)ExchangeUP;
CmpXchgOps      FastInterlockCompareExchange = (CmpXchgOps)CompareExchangeUP;
CmpXchgOpsPtr   FastInterlockCompareExchangePointer = CompareExchangeUP;
XchngAddOps     FastInterlockExchangeAdd = ExchangeAddUP;

IncDecOps   FastInterlockIncrement = IncrementUP;
IncDecOps   FastInterlockDecrement = DecrementUP;
IncDecLongOps    FastInterlockIncrementLong = IncrementLongUP;
IncDecLongOps	FastInterlockDecrementLong = DecrementLongUP;

// Adjust the generic interlocked operations for any platform specific ones we
// might have.
void InitFastInterlockOps()
{
    _ASSERTE(g_SystemInfo.dwNumberOfProcessors != 0);

    if (g_SystemInfo.dwNumberOfProcessors != 1)
    {
        FastInterlockOr  = OrMaskMP;
        FastInterlockAnd = AndMaskMP;

        FastInterlockExchange = ExchangeMP;
        FastInterlockExchangePointer = (XchgOpsPtr)ExchangeMP;
        FastInterlockCompareExchange = (CmpXchgOps)CompareExchangeMP;
        FastInterlockCompareExchangePointer = CompareExchangeMP;
        FastInterlockExchangeAdd = ExchangeAddMP;

        FastInterlockIncrement = IncrementMP;
        FastInterlockDecrement = DecrementMP;

            FastInterlockIncrementLong = IncrementLongMP8b;
			FastInterlockDecrementLong = DecrementLongMP8b;
    }
}



//---------------------------------------------------------
// Handles Cleanup for a standalone stub that returns a gcref
//---------------------------------------------------------
LPVOID STDMETHODCALLTYPE DoCleanupWithGcProtection(CleanupWorkList *pCleanup, OBJECTREF oref)
{
    LPVOID pvret;
    GCPROTECT_BEGIN(oref);
    pCleanup->Cleanup(FALSE);
    *(OBJECTREF*)&pvret = oref;
    GCPROTECT_END();
    return pvret;
}


//---------------------------------------------------------
// Wrapper arround an instance call to ML_CSTR_C2N_SR::DoConversion(STRINGREF, CleanupWorkList*)
//---------------------------------------------------------
  
LPSTR STDMETHODCALLTYPE DoConversionWrapper_CSTR(ML_CSTR_C2N_SR * obj, StringObject * s, CleanupWorkList* l)
{
  return obj->DoConversion( ObjectToSTRINGREF(s), l );
}


//---------------------------------------------------------
//Wrapper around an instance call to VOID CleanupWorkList::Cleanup(BOOL fBecauseOfException)
//---------------------------------------------------------
VOID STDMETHODCALLTYPE CleanupWrapper( CleanupWorkList * obj, BOOL fBecauseOfException )
{
  obj->Cleanup(fBecauseOfException );
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
    THROWSCOMPLUSEXCEPTION();


    // Must first scan the ML stream to see if this method qualifies for
    // a standalone stub. Can't wait until we start generating because we're
    // supposed to leave psl empty if we return FALSE.
    if (0 != (pheader->m_Flags & ~(MLHF_SETLASTERROR|MLHF_THISCALL|MLHF_64BITMANAGEDRETVAL|MLHF_64BITUNMANAGEDRETVAL|MLHF_MANAGEDRETVAL_TYPECAT_MASK|MLHF_UNMANAGEDRETVAL_TYPECAT_MASK|MLHF_NATIVERESULT))) {
        return FALSE;
    }

    BOOL fNeedsCleanup = FALSE;
    const MLCode *pMLCode = pheader->GetMLCode();
    MLCode mlcode;
    while (ML_INTERRUPT != (mlcode = *(pMLCode++))) {
        switch (mlcode) {
            case ML_COPY4: //intentional fallthru
            case ML_COPY8: //intentional fallthru
            case ML_PINNEDUNISTR_C2N: //intentional fallthru
            case ML_BLITTABLELAYOUTCLASS_C2N:
            case ML_CBOOL_C2N:
            case ML_COPYPINNEDGCREF:
                break;
            case ML_BUMPSRC:
            case ML_PINNEDISOMORPHICARRAY_C2N_EXPRESS:
                pMLCode += 2;
                break;
            case ML_REFBLITTABLEVALUECLASS_C2N:
                pMLCode += 4;
                break;

            case ML_CSTR_C2N:
                break;


            case ML_PUSHRETVALBUFFER1: //fallthru
            case ML_PUSHRETVALBUFFER2: //fallthru
            case ML_PUSHRETVALBUFFER4:
                break;

            case ML_HANDLEREF_C2N:
                break;

            case ML_CREATE_MARSHALER_WSTR: //fallthru
            case ML_CREATE_MARSHALER_CSTR: //fallthru

                if (*pMLCode == ML_PRERETURN_C2N_RETVAL) {
                    pMLCode++;
                    break;
                } else {
                    return FALSE;
                }


            default:
                return FALSE;
        }

                if (gMLInfo[mlcode].m_frequiresCleanup)
                {
                        fNeedsCleanup = TRUE;
                }

    }

    if (*(pMLCode) == ML_THROWIFHRFAILED) {
        return FALSE;
    }



    if (*(pMLCode) == ML_SETSRCTOLOCAL) {
        pMLCode += 3;
    }

    mlcode = *(pMLCode++);
    if (!(mlcode == ML_END ||
         ( (mlcode == ML_RETURN_C2N_RETVAL && *(pMLCode+2) == ML_END) ) ||
         ( (mlcode == ML_COPY4 ||
            mlcode == ML_COPY8 ||
            mlcode == ML_COPYI1 ||
            mlcode == ML_COPYU1 ||
            mlcode == ML_COPYI2 ||
            mlcode == ML_COPYU2 ||
            mlcode == ML_COPYI4 ||
            mlcode == ML_COPYU4 ||
            mlcode == ML_CBOOL_N2C ||
            mlcode == ML_BOOL_N2C) && *(pMLCode) == ML_END))) {
        return FALSE;
    }


    //-----------------------------------------------------------------------
    // Qualification stage done. If we've gotten this far, we MUST return
    // TRUE or throw an exception.
    //-----------------------------------------------------------------------


    //-----------------------------------------------------------------------
    // Generate the standard prolog
    //-----------------------------------------------------------------------
        {
                psl->EmitMethodStubProlog(fNeedsCleanup ? NDirectMethodFrameStandaloneCleanup::GetMethodFrameVPtr() : NDirectMethodFrameStandalone::GetMethodFrameVPtr());
        }

        //------------------------------------------------------------------------
        // If needs cleanup, reserve space for cleamup pointer.
        //------------------------------------------------------------------------
        if (fNeedsCleanup)
        {
                psl->X86EmitPushImm32(0);
        }



    //-----------------------------------------------------------------------
    // Add space for locals
    //-----------------------------------------------------------------------
    psl->X86EmitSubEsp(pheader->m_cbLocals);

    if (fNeedsCleanup)
        {
                // push ebx // thread
                psl->X86EmitPushReg(kEBX);
                // push esi // frame
                psl->X86EmitPushReg(kESI);
                // call DoCheckPointForCleanup
                psl->X86EmitCall(psl->NewExternalCodeLabel((LPVOID) DoCheckPointForCleanup), 8);
        }

        INT32 locbase = 0-( (fNeedsCleanup ? NDirectMethodFrameEx::GetNegSpaceSize() : NDirectMethodFrame::GetNegSpaceSize()) + pheader->m_cbLocals + (fDoComInterop?4:0));

    INT32  locofs = locbase;
    UINT32 ofs;
    ofs = 0;
    // The ofs should not be adjusted for __cdecl because the ArgIterator already generates the appropriate offsets
                   


    //-----------------------------------------------------------------------
    // Generate code to marshal each parameter.
    //-----------------------------------------------------------------------
    pMLCode = pheader->GetMLCode();
    while (ML_INTERRUPT != (mlcode = *(pMLCode++))) {
        switch (mlcode) {
            case ML_COPY4:
            case ML_COPYPINNEDGCREF:
                psl->X86EmitIndexPush(kESI, ofs);
                ofs += 4;
                break;

            case ML_COPY8:
                psl->X86EmitIndexPush(kESI, ofs+4);
                psl->X86EmitIndexPush(kESI, ofs);
                ofs += 8;
                break;

            case ML_HANDLEREF_C2N:
                psl->X86EmitIndexPush(kESI, ofs+4);
                ofs += 8;
                break;

            case ML_CBOOL_C2N:
                {
                    //    mov eax,[esi+ofs+4]
                    psl->X86EmitIndexRegLoad(kEAX, kESI, ofs+4);
                    //    xor  ecx,ecx
                    //    test al,al
                    //    setne cl
                    static const BYTE code[] = {0x33,0xc9,0x84,0xc0,0x0f,0x95,0xc1};
                    psl->EmitBytes(code, sizeof(code));
                    //    push ecx
                    psl->X86EmitPushReg(kECX);
                    ofs += 4;
                }
                break;

            case ML_REFBLITTABLEVALUECLASS_C2N:
                {
                    // mov eax, [esi+ofs]
                    psl->X86EmitOp(0x8b, kEAX, kESI, ofs);

                    // push eax
                    psl->X86EmitPushReg(kEAX);

                    ofs += sizeof(LPVOID);

#ifdef TOUCH_ALL_PINNED_OBJECTS
                    // lea edx [eax+IMM32]
                    psl->X86EmitOp(0x8d, kEDX, kEAX, cbSize);
                    psl->EmitPageTouch(TRUE);
#endif
                }
                break;


            case ML_PINNEDUNISTR_C2N: {


                // mov eax, [esi+OFS]
                psl->X86EmitIndexRegLoad(kEAX, kESI, ofs);
                // test eax,eax
                psl->Emit16(0xc085);
                CodeLabel *plabel = psl->NewCodeLabel();
                // jz LABEL
                psl->X86EmitCondJump(plabel, X86CondCode::kJZ);
                // add eax, BUFOFS
                psl->X86EmitAddReg(kEAX, (UINT8)(StringObject::GetBufferOffset()));


#ifdef TOUCH_ALL_PINNED_OBJECTS
                // mov edx, eax
                psl->X86EmitR2ROp(0x8b, kEDX, kEAX);

                // mov ecx, dword ptr [eax - BUFOFS + STRINGLEN]
                psl->X86EmitOp(0x8b, kECX, kEAX, StringObject::GetStringLengthOffset_MaskOffHighBit() - StringObject::GetBufferOffset());

                // and ecx, 0x7fffffff
                psl->Emit16(0xe181);
                psl->Emit32(0x7fffffff);

                // lea edx, [eax + ecx*2 + 2]
                psl->X86EmitOp(0x8d, kEDX, kEAX, 2, kECX, 2);


                // touch all pages
                psl->EmitPageTouch(TRUE);

                // mov eax, [esi+OFS]
                psl->X86EmitIndexRegLoad(kEAX, kESI, ofs);

                // add eax, BUFOFS
                psl->X86EmitAddReg(kEAX, (UINT8)(StringObject::GetBufferOffset()));

#endif


                psl->EmitLabel(plabel);
                // push eax
                psl->X86EmitPushReg(kEAX);

                ofs += 4;
                }
                break;


            case ML_BLITTABLELAYOUTCLASS_C2N: {


                // mov eax, [esi+OFS]
                psl->X86EmitIndexRegLoad(kEAX, kESI, ofs);
                // test eax,eax
                psl->Emit16(0xc085);
                CodeLabel *plabel = psl->NewCodeLabel();
                psl->X86EmitCondJump(plabel, X86CondCode::kJZ);

#ifdef TOUCH_ALL_PINNED_OBJECTS
                // mov ecx, [eax]
                psl->X86EmitOp(0x8b, kECX, kEAX);
#endif



                // lea eax, [eax+DATAPTR]
                psl->X86EmitOp(0x8d, kEAX, kEAX, Object::GetOffsetOfFirstField());

#ifdef TOUCH_ALL_PINNED_OBJECTS
                // mov edx, eax
                psl->X86EmitR2ROp(0x8b, kEDX, kEAX);

                // add edx, dword ptr [ecx + MethodTable.cbNativeSize]
                psl->X86EmitOp(0x03, kEDX, kECX, MethodTable::GetOffsetOfNativeSize());

                // touch all pages
                psl->EmitPageTouch(TRUE);

                // mov eax, [esi+OFS]
                psl->X86EmitIndexRegLoad(kEAX, kESI, ofs);

                // lea eax, [eax+DATAPTR]
                psl->X86EmitOp(0x8d, kEAX, kEAX, Object::GetOffsetOfFirstField());
#endif



                // LABEL:
                psl->EmitLabel(plabel);
                psl->X86EmitPushReg(kEAX);


                ofs += 4;
            }
            break;

            case ML_CSTR_C2N:
            {
                    // push cleanup worklist
                    //   lea eax, [esi + NDirectMethodFrameEx.CleanupWorklist]
                    psl->X86EmitOp(0x8d, kEAX, kESI, NDirectMethodFrameEx::GetOffsetOfCleanupWorkList());
                    //   push eax
                    psl->X86EmitPushReg(kEAX);
                    //   push [esi + OFS]
                    psl->X86EmitIndexPush(kESI, ofs);

                    //   lea ecx, [esi + locofs]
                    psl->X86EmitOp(0x8d, kECX, kESI, locofs);
                    //   push ecx
                    psl->X86EmitPushReg(kECX); 
                   
                    //   call ML_CSTR_C2N_SR::DoConversion(STRINGREF, CleanupWorkList*) via a static wrapper
                    psl->X86EmitCall(psl->NewExternalCodeLabel((LPVOID*)DoConversionWrapper_CSTR), 12);

                    //   push eax
                    psl->X86EmitPushReg(kEAX);
                    ofs += 4;
                    locofs += sizeof(ML_CSTR_C2N_SR);
            }
            break;


            case ML_BUMPSRC:
                ofs += *( (INT16*)pMLCode );
                pMLCode += 2;
                break;

            case ML_PINNEDISOMORPHICARRAY_C2N_EXPRESS:
                {
                    UINT16 dataofs = *( (INT16*)pMLCode );
                    pMLCode += 2;
                    _ASSERTE(dataofs);
#ifdef TOUCH_ALL_PINNED_OBJECTS
                    _ASSERTE(!"Not supposed to be here.");
#endif


                    // mov eax,[esi+ofs]
                    psl->X86EmitIndexRegLoad(kEAX, kESI, ofs);
                    // test eax,eax
                    psl->Emit16(0xc085);
                    CodeLabel *plabel = psl->NewCodeLabel();
                    // jz LABEL
                    psl->X86EmitCondJump(plabel, X86CondCode::kJZ);
                    // lea eax, [eax + dataofs]
                    psl->X86EmitOp(0x8d, kEAX, kEAX, (UINT32)dataofs);
    
                    psl->EmitLabel(plabel);
                    // push eax
                    psl->X86EmitPushReg(kEAX);

    
                    ofs += 4;

                }
                break;


            case ML_PUSHRETVALBUFFER1: //fallthru
            case ML_PUSHRETVALBUFFER2: //fallthru
            case ML_PUSHRETVALBUFFER4:
                // lea eax, [esi+locofs]
                // mov [eax],0
                // push eax

                psl->X86EmitOffsetModRM(0x8d, kEAX, kESI, locofs);
                psl->X86EmitOffsetModRM(0xc7, (X86Reg)0, kEAX, 0);
                psl->Emit32(0);
                psl->X86EmitPushReg(kEAX);

                locofs += 4;
                break;

            case ML_CREATE_MARSHALER_WSTR:
            case ML_CREATE_MARSHALER_CSTR:
                _ASSERTE(*pMLCode == ML_PRERETURN_C2N_RETVAL);

                //  lea     eax, [esi+locofs]
                //  push    eax       ;; push plocalwalk
                //  lea     eax, [esi + Frame.CleanupWorkList]
                //  push    eax       ;; push CleanupWorkList
                //  push    esi       ;; push Frame
                //  call    DoMLCreateMarshaler?Str

                //  push    edx       ;; push garbage (this will be overwritten by actual argument)
                //  push    esp       ;; push address of the garbage we just pushed
                //  lea     eax, [esi+locofs]
                //  push    eax       ;; push marshaler
                //  call    DoMLPrereturnC2N

                psl->X86EmitOffsetModRM(0x8d, kEAX, kESI, locofs);
                psl->X86EmitPushReg(kEAX);

                psl->X86EmitOp(0x8d, kEAX, kESI, NDirectMethodFrameEx::GetOffsetOfCleanupWorkList());
                psl->X86EmitPushReg(kEAX);

                psl->X86EmitPushReg(kESI);
                switch (mlcode)
                {
                    case ML_CREATE_MARSHALER_WSTR:
                        psl->X86EmitCall(psl->NewExternalCodeLabel((LPVOID) DoMLCreateMarshalerWStr), 12);
                        break;
                    case ML_CREATE_MARSHALER_CSTR:
                        psl->X86EmitCall(psl->NewExternalCodeLabel((LPVOID) DoMLCreateMarshalerCStr), 12);
                        break;
                    default:
                        _ASSERTE(0);
                }
                psl->X86EmitPushReg(kEDX);
                psl->X86EmitPushReg((X86Reg)4 /*kESP*/);
                psl->X86EmitOffsetModRM(0x8d, kEAX, kESI, locofs);
                psl->X86EmitPushReg(kEAX);
                psl->X86EmitCall(psl->NewExternalCodeLabel((LPVOID) DoMLPrereturnC2N), 8);


                locofs += gMLInfo[mlcode].m_cbLocal;
                pMLCode++;

                break;


            default:
                _ASSERTE(0);
        }
    }
    UINT32 numStackBytes;
    numStackBytes = pheader->m_cbStackPop;

    _ASSERTE(FitsInI1(MDEnums::MD_IndexOffset));
    _ASSERTE(FitsInI1(MDEnums::MD_SkewOffset));

    INT thisOffset = 0-(FramedMethodFrame::GetNegSpaceSize() + 4);
    if (fNeedsCleanup)
    {
        thisOffset -= sizeof(CleanupWorkList);
    }



    CodeLabel *pRareEnable,  *pEnableRejoin;
    CodeLabel *pRareDisable, *pDisableRejoin;
    pRareEnable    = psl->NewCodeLabel();
    pEnableRejoin  = psl->NewCodeLabel();
    pRareDisable   = psl->NewCodeLabel();
    pDisableRejoin = psl->NewCodeLabel();

    //-----------------------------------------------------------------------
    // Generate the inline part of enabling preemptive GC
    //-----------------------------------------------------------------------
    psl->EmitEnable(pRareEnable);
    psl->EmitLabel(pEnableRejoin);

#ifdef PROFILING_SUPPORTED
    // Notify the profiler of a call out of managed code
    if (CORProfilerTrackTransitions())
    {
        // Save registers
        psl->X86EmitPushReg(kEAX);
        psl->X86EmitPushReg(kECX);
        psl->X86EmitPushReg(kEDX);

        psl->X86EmitPushImm32(COR_PRF_TRANSITION_CALL);     // Reason
        psl->X86EmitPushReg(kESI);                          // Frame*
        psl->X86EmitCall(psl->NewExternalCodeLabel((LPVOID)ProfilerManagedToUnmanagedTransition), 8);

        // Restore registers
        psl->X86EmitPopReg(kEDX);
        psl->X86EmitPopReg(kECX);
        psl->X86EmitPopReg(kEAX);
    }
#endif // PROFILING_SUPPORTED

    {
        //-----------------------------------------------------------------------
        // Invoke the DLL target.
        //-----------------------------------------------------------------------

        if (pheader->m_Flags & MLHF_THISCALL) {
            if (pheader->m_Flags & MLHF_THISCALLHIDDENARG)
            {
                // pop eax
                psl->X86EmitPopReg(kEAX);
                // pop ecx
                psl->X86EmitPopReg(kECX);
                // push eax
                psl->X86EmitPushReg(kEAX);
            }
            else
            {
                // pop ecx
                psl->X86EmitPopReg(kECX);
            }
        }

        //  mov eax, [CURFRAME.MethodDesc]
        psl->X86EmitIndexRegLoad(kEAX, kESI, FramedMethodFrame::GetOffsetOfMethod());

#if _DEBUG
        // Call through debugger logic to make sure it works
        psl->X86EmitCall(psl->NewExternalCodeLabel((LPVOID)CheckExitFrameDebuggerCalls), 0, TRUE);
#else
        //  call [eax + MethodDesc.NDirectTarget]
        psl->X86EmitOffsetModRM(0xff, (X86Reg)2, kEAX, NDirectMethodDesc::GetOffsetofNDirectTarget());
        psl->EmitReturnLabel();
#endif


        if (pheader->m_Flags & MLHF_SETLASTERROR) {
            psl->EmitSaveLastError();
        }
    }

#ifdef PROFILING_SUPPORTED
    // Notify the profiler of a return from a managed->unmanaged call
    if (CORProfilerTrackTransitions())
    {
        // Save registers
        psl->X86EmitPushReg(kEAX);
        psl->X86EmitPushReg(kECX);
        psl->X86EmitPushReg(kEDX);

        psl->X86EmitPushImm32(COR_PRF_TRANSITION_RETURN);   // Reason
        psl->X86EmitPushReg(kESI);                          // FrameID
        psl->X86EmitCall(psl->NewExternalCodeLabel((LPVOID)ProfilerUnmanagedToManagedTransition), 8);

        // Restore registers
        psl->X86EmitPopReg(kEDX);
        psl->X86EmitPopReg(kECX);
        psl->X86EmitPopReg(kEAX);
    }
#endif // PROFILING_SUPPORTED


    //-----------------------------------------------------------------------
    // Generate the inline part of disabling preemptive GC
    //-----------------------------------------------------------------------
    psl->EmitDisable(pRareDisable);
    psl->EmitLabel(pDisableRejoin);

    //-----------------------------------------------------------------------
    // Marshal the return value
    //-----------------------------------------------------------------------



    if (*pMLCode == ML_SETSRCTOLOCAL) {
        pMLCode++;
        UINT16 bufidx = *((UINT16*)(pMLCode));
        pMLCode += 2;
        // mov eax, [esi + locbase + bufidx]
        psl->X86EmitIndexRegLoad(kEAX, kESI, locbase+bufidx);

    }


    switch (mlcode = *(pMLCode++)) {
        case ML_BOOL_N2C: {
                //    xor  ecx,ecx
                //    test eax,eax
                //    setne cl
                //    mov  eax,ecx

                static const BYTE code[] = {0x33,0xc9,0x85,0xc0,0x0f,0x95,0xc1,0x8b,0xc1};
                psl->EmitBytes(code, sizeof(code));
            }
            break;

        case ML_CBOOL_N2C: {
                //    xor  ecx,ecx
                //    test al,al
                //    setne cl
                //    mov  eax,ecx
    
                static const BYTE code[] = {0x33,0xc9,0x84,0xc0,0x0f,0x95,0xc1,0x8b,0xc1};
                psl->EmitBytes(code, sizeof(code));
            }
            break;

        case ML_COPY4: //fallthru
        case ML_COPY8: //fallthru
        case ML_COPYI4: //fallthru
        case ML_COPYU4:
        case ML_END:
            //do nothing
            break;

        case ML_COPYU1:
            // movzx eax,al
            psl->Emit8(0x0f);
            psl->Emit16(0xc0b6);
            break;


        case ML_COPYI1:
            // movsx eax,al
            psl->Emit8(0x0f);
            psl->Emit16(0xc0be);
            break;

        case ML_COPYU2:
            // movzx eax,ax
            psl->Emit8(0x0f);
            psl->Emit16(0xc0b7);
            break;

        case ML_COPYI2:
            // movsx eax,ax
            psl->Emit8(0x0f);
            psl->Emit16(0xc0bf);
            break;


        case ML_RETURN_C2N_RETVAL:
            {
                UINT16 locidx = *((UINT16*)(pMLCode));
                pMLCode += 2;

                // lea        eax, [esi + locidx + locbase]
                // push       eax  //push marshaler
                // call       DoMLReturnC2NRetVal   ;; returns oref in eax
                //


                psl->X86EmitOffsetModRM(0x8d, kEAX, kESI, locbase+locidx);
                psl->X86EmitPushReg(kEAX);

                psl->X86EmitCall(psl->NewExternalCodeLabel((LPVOID) DoMLReturnC2NRetVal), 4);
            }
            break;


        default:
            _ASSERTE(!"Can't get here.");
    }

        if (fNeedsCleanup)
        {
                if ( pheader->GetManagedRetValTypeCat() == MLHF_TYPECAT_GCREF )
                {
                    // push eax
                    // lea  eax, [esi + Frame.CleanupWorkList]
                    // push eax
                    // call DoCleanupWithGcProtection
                    // ;; (possibly promoted) objref left in eax
                    psl->X86EmitPushReg(kEAX);
                    psl->X86EmitOp(0x8d, kEAX, kESI, NDirectMethodFrameEx::GetOffsetOfCleanupWorkList());
                    psl->X86EmitPushReg(kEAX);
                    psl->X86EmitCall(psl->NewExternalCodeLabel((LPVOID) DoCleanupWithGcProtection), 8);

                }
                else
                {

                    // Do cleanup
    
                    // push eax             //save EAX
                    psl->X86EmitPushReg(kEAX);
    
                    // push edx             //save EDX
                    psl->X86EmitPushReg(kEDX);
    
    
                    // push 0               // FALSE
                    psl->Emit8(0x68);
                    psl->Emit32(0);
    
                    // lea ecx, [esi + Frame.CleanupWorkList]
                    psl->X86EmitOp(0x8d, kECX, kESI, NDirectMethodFrameEx::GetOffsetOfCleanupWorkList());
                    // push ecx
                    psl->X86EmitPushReg(kECX);    

                    // call CleanupWorkList::Cleanup( bool ) via a static wrapper
                    psl->X86EmitCall(psl->NewExternalCodeLabel((LPVOID*)CleanupWrapper), 8);
   
                    // pop edx
                    psl->X86EmitPopReg(kEDX);
    
                    // pop eax
                    psl->X86EmitPopReg(kEAX);
                }
        }

    // must restore esp explicitly since we don't know whether the target
    // popped the args.
    // lea esp, [esi+xx]
    psl->X86EmitOffsetModRM(0x8d, (X86Reg)4 /*kESP*/, kESI, 0-FramedMethodFrame::GetNegSpaceSize());



    //-----------------------------------------------------------------------
    // Epilog
    //-----------------------------------------------------------------------
    psl->EmitMethodStubEpilog(numStackBytes, kNoTripStubStyle);

    //-----------------------------------------------------------------------
    // The out-of-line portion of enabling preemptive GC - rarely executed
    //-----------------------------------------------------------------------
    psl->EmitLabel(pRareEnable);
    psl->EmitRareEnable(pEnableRejoin);

    //-----------------------------------------------------------------------
    // The out-of-line portion of disabling preemptive GC - rarely executed
    //-----------------------------------------------------------------------
    psl->EmitLabel(pRareDisable);
    psl->EmitRareDisable(pDisableRejoin, /*bIsCallIn=*/FALSE);


    return TRUE;
}



extern "C" VOID __stdcall StubRareEnableWorker(Thread *pThread)
{
    //printf("RareEnable\n");
    pThread->RareEnablePreemptiveGC();
}


// I would prefer to define a unique HRESULT in our own facility, but we aren't
// supposed to create new HRESULTs this close to ship
#define E_PROCESS_SHUTDOWN_REENTRY    HRESULT_FROM_WIN32(ERROR_PROCESS_ABORTED)


// Disable when calling into managed code from a place that fails via HRESULT
extern "C" HRESULT __stdcall StubRareDisableHRWorker(Thread *pThread, Frame *pFrame)
{
    // WARNING!!!!
    // when we start executing here, we are actually in cooperative mode.  But we
    // haven't synchronized with the barrier to reentry yet.  So we are in a highly
    // dangerous mode.  If we call managed code, we will potentially be active in
    // the GC heap, even as GC's are occuring!

    // Do not add THROWSCOMPLUSEXCEPTION() here.  We haven't set up SEH.  We rely
    // on HandleThreadAbort dealing with this situation properly.

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
        return E_PROCESS_SHUTDOWN_REENTRY;
    }

    // We must do the following in this order, because otherwise we would be constructing
    // the exception for the abort without synchronizing with the GC.  Also, we have no
    // CLR SEH set up, despite the fact that we may throw a ThreadAbortException.
    pThread->RareDisablePreemptiveGC();
    pThread->HandleThreadAbort();
    return S_OK;
}


// Disable when calling into managed code from a place that fails via Exceptions
extern "C" VOID __stdcall StubRareDisableTHROWWorker(Thread *pThread, Frame *pFrame)
{
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

// Disable when calling from a place that is returning to managed code, not calling
// into it.
extern "C" VOID __stdcall StubRareDisableRETURNWorker(Thread *pThread, Frame *pFrame)
{
    // WARNING!!!!
    // when we start executing here, we are actually in cooperative mode.  But we
    // haven't synchronized with the barrier to reentry yet.  So we are in a highly
    // dangerous mode.  If we call managed code, we will potentially be active in
    // the GC heap, even as GC's are occuring!
    THROWSCOMPLUSEXCEPTION();

#ifdef DEBUGGING_SUPPORTED
    // If the debugger is attached, we use this opprotunity to see if
    // we're disabling preemptive GC on the way into the runtime from
    // unmanaged code. We end up here because
    // Increment/DecrementTraceCallCount() will bump
    // g_TrapReturningThreads for us.
    if (CORDebuggerTraceCall())
        g_pDebugInterface->PossibleTraceCall(NULL, pFrame);
#endif // DEBUGGING_SUPPORTED

    // Don't check for ShutDown scenario.  We are returning to managed code, not
    // calling into it.  The best we can do during shutdown is to deadlock and allow
    // the WatchDogThread to terminate the process on timeout.

    // We must do the following in this order, because otherwise we would be constructing
    // the exception for the abort without synchronizing with the GC.  Also, we have no
    // CLR SEH set up, despite the fact that we may throw a ThreadAbortException.
    pThread->RareDisablePreemptiveGC();
    pThread->HandleThreadAbort();
}

// Disable from a place that is calling into managed code via a UMEntryThunk.
extern "C" VOID __stdcall UMThunkStubRareDisableWorker(Thread *pThread, UMEntryThunk *pUMEntryThunk, Frame *pFrame)
{
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
        g_pDebugInterface->PossibleTraceCall(pUMEntryThunk, pFrame);
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


//-----------------------------------------------------------------------
// Generates the inline portion of the code to enable preemptive GC. Hopefully,
// the inline code is all that will execute most of the time. If this code
// path is entered at certain times, however, it will need to jump out to
// a separate out-of-line path which is more expensive. The "pForwardRef"
// label indicates the start of the out-of-line path.
//
// Assumptions:
//      ebx = Thread
// Preserves
//      all registers except ecx.
//
//-----------------------------------------------------------------------
VOID StubLinkerCPU::EmitEnable(CodeLabel *pForwardRef)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(4 == sizeof( ((Thread*)0)->m_State ));
    _ASSERTE(4 == sizeof( ((Thread*)0)->m_fPreemptiveGCDisabled ));


    // move byte ptr [ebx + Thread.m_fPreemptiveGCDisabled],0
    X86EmitOffsetModRM(0xc6, (X86Reg)0, kEBX, offsetof(Thread, m_fPreemptiveGCDisabled));
    Emit8(0);

    _ASSERTE(FitsInI1(Thread::TS_CatchAtSafePoint));

    // test byte ptr [ebx + Thread.m_State], TS_CatchAtSafePoint
    X86EmitOffsetModRM(0xf6, (X86Reg)0, kEBX, offsetof(Thread, m_State));
    Emit8(Thread::TS_CatchAtSafePoint);

    // jnz RarePath
    X86EmitCondJump(pForwardRef, X86CondCode::kJNZ);

#ifdef _DEBUG
    X86EmitDebugTrashReg(kECX);
#endif



}



//-----------------------------------------------------------------------
// Generates the out-of-line portion of the code to enable preemptive GC.
// After the work is done, the code jumps back to the "pRejoinPoint"
// which should be emitted right after the inline part is generated.
//
// Assumptions:
//      ebx = Thread
// Preserves
//      all registers except ecx.
//
//-----------------------------------------------------------------------
VOID StubLinkerCPU::EmitRareEnable(CodeLabel *pRejoinPoint)
{
    THROWSCOMPLUSEXCEPTION();

    X86EmitCall(NewExternalCodeLabel((LPVOID) StubRareEnable), 0);
#ifdef _DEBUG
    X86EmitDebugTrashReg(kECX);
#endif
    X86EmitNearJump(pRejoinPoint);


}




//-----------------------------------------------------------------------
// Generates the inline portion of the code to disable preemptive GC. Hopefully,
// the inline code is all that will execute most of the time. If this code
// path is entered at certain times, however, it will need to jump out to
// a separate out-of-line path which is more expensive. The "pForwardRef"
// label indicates the start of the out-of-line path.
//
// Assumptions:
//      ebx = Thread
// Preserves
//      all registers except ecx.
//
//-----------------------------------------------------------------------
VOID StubLinkerCPU::EmitDisable(CodeLabel *pForwardRef)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(4 == sizeof( ((Thread*)0)->m_fPreemptiveGCDisabled ));
    _ASSERTE(4 == sizeof(g_TrapReturningThreads));

    // move byte ptr [ebx + Thread.m_fPreemptiveGCDisabled],1
    X86EmitOffsetModRM(0xc6, (X86Reg)0, kEBX, offsetof(Thread, m_fPreemptiveGCDisabled));
    Emit8(1);

    // cmp dword ptr g_TrapReturningThreads, 0
    Emit16(0x3d83);
    EmitPtr(&g_TrapReturningThreads);
    Emit8(0);


    // jnz RarePath
    X86EmitCondJump(pForwardRef, X86CondCode::kJNZ);

#ifdef _DEBUG
    X86EmitDebugTrashReg(kECX);
#endif




}


//-----------------------------------------------------------------------
// Generates the out-of-line portion of the code to disable preemptive GC.
// After the work is done, the code jumps back to the "pRejoinPoint"
// which should be emitted right after the inline part is generated.  However,
// if we cannot execute managed code at this time, an exception is thrown
// which cannot be caught by managed code.
//
// Assumptions:
//      ebx = Thread
// Preserves
//      all registers except ecx, eax.
//
//-----------------------------------------------------------------------
VOID StubLinkerCPU::EmitRareDisable(CodeLabel *pRejoinPoint, BOOL bIsCallIn)
{
    THROWSCOMPLUSEXCEPTION();

    if (bIsCallIn)
        X86EmitCall(NewExternalCodeLabel((LPVOID) StubRareDisableTHROW), 0);
    else
        X86EmitCall(NewExternalCodeLabel((LPVOID) StubRareDisableRETURN), 0);

#ifdef _DEBUG
    X86EmitDebugTrashReg(kECX);
#endif
    X86EmitNearJump(pRejoinPoint);
}



//-----------------------------------------------------------------------
// Generates the out-of-line portion of the code to disable preemptive GC.
// After the work is done, the code normally jumps back to the "pRejoinPoint"
// which should be emitted right after the inline part is generated.  However,
// if we cannot execute managed code at this time, an HRESULT is returned
// via the ExitPoint.
//
// Assumptions:
//      ebx = Thread
// Preserves
//      all registers except ecx, eax.
//
//-----------------------------------------------------------------------
VOID StubLinkerCPU::EmitRareDisableHRESULT(CodeLabel *pRejoinPoint, CodeLabel *pExitPoint)
{
    THROWSCOMPLUSEXCEPTION();

    X86EmitCall(NewExternalCodeLabel((LPVOID) StubRareDisableHR), 0);

#ifdef _DEBUG
    X86EmitDebugTrashReg(kECX);
#endif

    // test eax,eax
    Emit16(0xc085);

    // JZ pRejoinPoint
    X86EmitCondJump(pRejoinPoint, X86CondCode::kJZ);

    X86EmitNearJump(pExitPoint);
}




//---------------------------------------------------------
// Performs a slim N/Direct call. This form can handle most
// common cases and is faster than the full generic version.
//---------------------------------------------------------

#define NDIRECT_SLIM_CBDSTMAX 32

struct NDirectSlimLocals
{
    Thread               *pThread;
    NDirectMethodFrameEx *pFrame;
    UINT32                savededi;

    NDirectMethodDesc    *pMD;
    const MLCode         *pMLCode;
    CleanupWorkList      *pCleanup;
    BYTE                 *pLocals;

    INT64                 nativeRetVal;
};

VOID __stdcall NDirectSlimStubWorker1(NDirectSlimLocals *pNSL)
{
    THROWSCOMPLUSEXCEPTION();

    pNSL->pMD                 = (NDirectMethodDesc*)(pNSL->pFrame->GetFunction());
    MLHeader *pheader         = pNSL->pMD->GetMLHeader();
    UINT32 cbLocals           = pheader->m_cbLocals;
    BYTE *pdst                = ((BYTE*)pNSL) - NDIRECT_SLIM_CBDSTMAX - cbLocals;
    pNSL->pLocals             = pdst + NDIRECT_SLIM_CBDSTMAX;
    VOID *psrc                = (VOID*)(pNSL->pFrame);
    pNSL->pCleanup            = pNSL->pFrame->GetCleanupWorkList();

    LOG((LF_STUBS, LL_INFO1000, "Calling NDirectSlimStubWorker1 %s::%s \n", pNSL->pMD->m_pszDebugClassName, pNSL->pMD->m_pszDebugMethodName));

    if (pNSL->pCleanup) {
        // Checkpoint the current thread's fast allocator (used for temporary
        // buffers over the call) and schedule a collapse back to the checkpoint in
        // the cleanup list. Note that if we need the allocator, it is
        // guaranteed that a cleanup list has been allocated.
        void *pCheckpoint = pNSL->pThread->m_MarshalAlloc.GetCheckpoint();
        pNSL->pCleanup->ScheduleFastFree(pCheckpoint);
        pNSL->pCleanup->IsVisibleToGc();
    }

#ifdef _DEBUG
    FillMemory(pdst, NDIRECT_SLIM_CBDSTMAX+cbLocals, 0xcc);
#endif

    pNSL->pMLCode = RunML(pheader->GetMLCode(),
                          psrc,
                          pdst + pheader->m_cbDstBuffer,
                          (UINT8*const)(pNSL->pLocals),
                          pNSL->pCleanup);

    pNSL->pThread->EnablePreemptiveGC();

#ifdef PROFILING_SUPPORTED
    // Notify the profiler of transitions out of the runtime
    if (CORProfilerTrackTransitions())
    {
        g_profControlBlock.pProfInterface->
            ManagedToUnmanagedTransition((FunctionID) pNSL->pMD,
                                               COR_PRF_TRANSITION_CALL);
    }
#endif // PROFILING_SUPPORTED
}


INT64 __stdcall NDirectSlimStubWorker2(const NDirectSlimLocals *pNSL)
{
    THROWSCOMPLUSEXCEPTION();

    LOG((LF_STUBS, LL_INFO1000, "Calling NDirectSlimStubWorker2 %s::%s \n", pNSL->pMD->m_pszDebugClassName, pNSL->pMD->m_pszDebugMethodName));

#ifdef PROFILING_SUPPORTED
    // Notify the profiler of transitions out of the runtime
    if (CORProfilerTrackTransitions())
    {
        g_profControlBlock.pProfInterface->
            UnmanagedToManagedTransition((FunctionID) pNSL->pMD,
                                               COR_PRF_TRANSITION_RETURN);
    }
#endif // PROFILING_SUPPORTED

    pNSL->pThread->DisablePreemptiveGC();
    pNSL->pThread->HandleThreadAbort();
    INT64 returnValue;



    RunML(pNSL->pMLCode,
          &(pNSL->nativeRetVal),
          ((BYTE*)&returnValue) + 4, // We don't slimstub 64-bit returns
          (UINT8*const)(pNSL->pLocals),
          pNSL->pFrame->GetCleanupWorkList());

    if (pNSL->pCleanup) {
        pNSL->pCleanup->Cleanup(FALSE);
    }

    return returnValue;
}


//---------------------------------------------------------
// Creates the slim NDirect stub.
//---------------------------------------------------------
/* static */
Stub* NDirect::CreateSlimNDirectStub(StubLinker *pstublinker, NDirectMethodDesc *pMD, UINT numStackBytes)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE (!pMD->IsVarArg());

    BOOL fSaveLastError = FALSE;

    // Putting this in a local block to prevent the code below from seeing
    // the header. Since we sharing stubs based on the return value, we can't
    // customize based on the header.
    {
        {
            MLHeader *pheader    = pMD->GetMLHeader();

            if ( !(((pheader->m_Flags & MLHF_MANAGEDRETVAL_TYPECAT_MASK) 
                    != MLHF_TYPECAT_GCREF) &&
                   0 == (pheader->m_Flags & ~(MLHF_SETLASTERROR)) &&
                   pheader->m_cbDstBuffer <= NDIRECT_SLIM_CBDSTMAX &&
                   pheader->m_cbLocals + pheader->m_cbDstBuffer   <= 0x1000-100) ) {
                return NULL;
            }

            if (pheader->m_Flags & MLHF_SETLASTERROR) {
                fSaveLastError = TRUE;
            }
        }
    }

    //printf("Generating slim.\n");


    UINT key           = numStackBytes << 1;
    if (fSaveLastError) {
        key |= 1;
    }
    Stub *pStub = m_pNDirectSlimStubCache->GetStub(key);
    if (pStub) {
        return pStub;
    } else {


        CPUSTUBLINKER *psl = (CPUSTUBLINKER*)pstublinker;

        psl->EmitMethodStubProlog(NDirectMethodFrameSlim::GetMethodFrameVPtr());

        // pushes a CleanupWorkList.
        psl->X86EmitPushImm32(0);

        // Reserve space for NDirectSlimLocals (note this actually reserves
        // more space than necessary.)
        psl->X86EmitSubEsp(sizeof(NDirectSlimLocals));

        // Allocate & initialize leading NDirectSlimLocals fields
        psl->X86EmitPushReg(kEDI); _ASSERTE(8==offsetof(NDirectSlimLocals, savededi));
        psl->X86EmitPushReg(kESI); _ASSERTE(4==offsetof(NDirectSlimLocals, pFrame));
        psl->X86EmitPushReg(kEBX); _ASSERTE(0==offsetof(NDirectSlimLocals, pThread));

        // Save pointer to NDirectSlimLocals in edi.
        // mov edi,esp
        psl->Emit16(0xfc8b);

        // Save space for destination & ML local buffer.
        //  mov edx, [CURFRAME.MethodDesc]
        psl->X86EmitIndexRegLoad(kEDX, kESI, FramedMethodFrame::GetOffsetOfMethod());

        //  mov ecx, [edx + NDirectMethodDesc.ndirect.m_pMLStub]
        psl->X86EmitIndexRegLoad(kECX, kEDX, NDirectMethodDesc::GetOffsetofMLHeaderField());

        _ASSERTE(2 == sizeof(((MLHeader*)0)->m_cbLocals));
        //  movzx eax, word ptr [ecx + Stub.m_cbLocals]
        psl->Emit8(0x0f);
        psl->X86EmitOffsetModRM(0xb7, kEAX, kECX, offsetof(MLHeader,m_cbLocals));

        //  add eax, NDIRECT_SLIM_CBDSTMAX
        psl->Emit8(0x05);
        psl->Emit32(NDIRECT_SLIM_CBDSTMAX);

        psl->Push(NDIRECT_SLIM_CBDSTMAX);

        //  sub esp, eax
        psl->Emit16(0xe02b);

        // Invoke the first worker, passing it the address of NDirectSlimLocals.
        // This will marshal the parameters into the dst buffer and enable gc.
        psl->X86EmitPushReg(kEDI);
        psl->X86EmitCall(psl->NewExternalCodeLabel((LPVOID) NDirectSlimStubWorker1), 4);

        // Invoke the DLL target.
        //  mov eax, [CURFRAME.MethodDesc]
        psl->X86EmitIndexRegLoad(kEAX, kESI, FramedMethodFrame::GetOffsetOfMethod());
#if _DEBUG
        // Call through debugger logic to make sure it works
        psl->X86EmitCall(psl->NewExternalCodeLabel((LPVOID)CheckExitFrameDebuggerCalls), 0, TRUE);
#else
        //  call [eax + MethodDesc.NDirectTarget]
        psl->X86EmitOffsetModRM(0xff, (X86Reg)2, kEAX, NDirectMethodDesc::GetOffsetofNDirectTarget());
        psl->EmitReturnLabel();
#endif

        // Emit our call site return label


        if (fSaveLastError) {
            psl->EmitSaveLastError();
        }



        // Save away the raw return value
        psl->X86EmitIndexRegStore(kEDI, offsetof(NDirectSlimLocals, nativeRetVal), kEAX);
        psl->X86EmitIndexRegStore(kEDI, offsetof(NDirectSlimLocals, nativeRetVal) + 4, kEDX);

        // Invoke the second worker, passing it the address of NDirectSlimLocals.
        // This will marshal the return value into eax, and redisable gc.
        psl->X86EmitPushReg(kEDI);
        psl->X86EmitCall(psl->NewExternalCodeLabel((LPVOID) NDirectSlimStubWorker2), 4);

        // DO NOT TRASH EAX FROM HERE OUT.

        // Restore edi.
        // mov edi, [edi + savededi]
        psl->X86EmitIndexRegLoad(kEDI, kEDI, offsetof(NDirectSlimLocals, savededi));

        // must restore esp explicitly since we don't know whether the target
        // popped the args.
        // lea esp, [esi+xx]
        psl->X86EmitOffsetModRM(0x8d, (X86Reg)4 /*kESP*/, kESI, 0-FramedMethodFrame::GetNegSpaceSize());


        // Tear down frame and exit.
        psl->EmitMethodStubEpilog(numStackBytes, kNoTripStubStyle);

        Stub *pCandidate = psl->Link(SystemDomain::System()->GetStubHeap());
        Stub *pWinner = m_pNDirectSlimStubCache->AttemptToSetStub(key,pCandidate);
        pCandidate->DecRef();
        if (!pWinner) {
            COMPlusThrowOM();
        }
        return pWinner;
    }

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

/*********************************************************************/



// Get X86Reg indexes of argument registers based on offset into ArgumentRegister
X86Reg GetX86ArgumentRegisterFromOffset(size_t ofs)
{
#define DEFINE_ARGUMENT_REGISTER(reg) if (ofs == offsetof(ArgumentRegisters, reg)) return k##reg;
#include "eecallconv.h"
    _ASSERTE(0);//Can't get here.
    return kEBP;
}

static VOID LoadArgIndex(StubLinkerCPU *psl, ShuffleEntry *pShuffleEntry, size_t argregofs, X86Reg reg, UINT espadjust)
{
    THROWSCOMPLUSEXCEPTION();
    argregofs |= ShuffleEntry::REGMASK;

    static const BYTE replaceArg1[] = { 0x89, 0x4C, 0x24, 0x08 }; // mov [esp+8], ecx
    static const BYTE replaceArg2[] = { 0x89, 0x54, 0x24, 0x0C }; // mov [esp+c], edx

    while (pShuffleEntry->srcofs != ShuffleEntry::SENTINEL) {
        if ( pShuffleEntry->dstofs == argregofs) {
            if (pShuffleEntry->srcofs & ShuffleEntry::REGMASK) {
                _ASSERT( reg == kECX &&
                      GetX86ArgumentRegisterFromOffset( pShuffleEntry->srcofs & ShuffleEntry::OFSMASK ) == kEDX );
                psl->X86EmitIndexRegLoad(reg, kEAX, 0xc );
                psl->EmitBytes(replaceArg1, sizeof(replaceArg1));
                 
            } else {

                psl->X86EmitIndexRegLoad(reg, kEAX, pShuffleEntry->srcofs+espadjust);

                _ASSERT( reg == kEDX );
                psl->EmitBytes(replaceArg2, sizeof(replaceArg2));
            }
            break;
        }
        pShuffleEntry++;
    }
}

//===========================================================================
// Emits code to adjust for a static delegate target.
#if defined(_DEBUG)
VOID DisassembleShuffleEntryArray(const ShuffleEntry *pWalk)
{
  //ShuffleEntry *pWalk = pShuffleEntryArray;
   while (pWalk->srcofs != ShuffleEntry::SENTINEL) {
     //if (!(pWalk->dstofs & ShuffleEntry::REGMASK)) {
            if (pWalk->srcofs & ShuffleEntry::REGMASK) {
	      printf( "REGMASK(srcofs): %d\n", pWalk->srcofs & ShuffleEntry::OFSMASK);     
            } else {
              printf("REGMASK(!srcofs,dstofs): %d + %d\n", pWalk->srcofs, pWalk->dstofs); 
            }
	    //}

        pWalk++;
    }
    _ASSERTE(pWalk->srcofs == ShuffleEntry::SENTINEL);
    printf("SENTINEL: StackDelta %d \n", pWalk->stacksizedelta);
  
}
#endif

VOID StubLinkerCPU::EmitShuffleThunk(ShuffleEntry *pShuffleEntryArray)
{
    THROWSCOMPLUSEXCEPTION();

    UINT espadjust = 4;

    //DisassembleShuffleEntryArray(pShuffleEntryArray);

    // save the real target on the stack (will jump to it later)
    // push [ecx + Delegate._methodptraux]
    X86EmitIndexPush(THIS_kREG, Object::GetOffsetOfFirstField() + COMDelegate::m_pFPAuxField->GetOffset());

    // mov SCRATCHREG,esp
    Emit8(0x8b);
    Emit8(0304 | (SCRATCH_REGISTER_X86REG << 3));

    // Load any enregistered arguments first. Order is important.
#define DEFINE_ARGUMENT_REGISTER(reg) LoadArgIndex(this, pShuffleEntryArray, offsetof(ArgumentRegisters, reg), k##reg, espadjust);
#include "eecallconv.h"

    // Now shift any nonenregistered arguments.
    ShuffleEntry *pWalk = pShuffleEntryArray;
    while (pWalk->srcofs != ShuffleEntry::SENTINEL) {
        if (!(pWalk->dstofs & ShuffleEntry::REGMASK)) {
            if (pWalk->srcofs & ShuffleEntry::REGMASK) {
                X86EmitPushReg( GetX86ArgumentRegisterFromOffset( pWalk->srcofs & ShuffleEntry::OFSMASK ) );
            } else {
                // If srcofs is zero we are attempting to move the return address
                X86EmitIndexPush(kEAX, pWalk->srcofs+espadjust);		
            }
        }

        pWalk++;
    }

    // Capture the stacksizedelta while we're at the end of the list.
    _ASSERTE(pWalk->srcofs == ShuffleEntry::SENTINEL);
    if (pWalk != pShuffleEntryArray) {
        do {
            pWalk--;
            if (!(pWalk->dstofs & ShuffleEntry::REGMASK)) {
                _ASSERT((pWalk->dstofs)>=4);
                X86EmitIndexPop(kEAX, pWalk->dstofs+espadjust-4); // We don't need to espadjust 
            }


        } while (pWalk != pShuffleEntryArray);
    }

    X86EmitPopReg(SCRATCH_REGISTER_X86REG);

    // Now jump to real target
    //   JMP SCRATCHREG
    Emit16(0xe0ff | (SCRATCH_REGISTER_X86REG<<8));
}

//===========================================================================
// Emits code for MulticastDelegate.Invoke()
VOID StubLinkerCPU::EmitMulticastInvoke(UINT numStackBytes, BOOL fSingleCast)
{
    THROWSCOMPLUSEXCEPTION();

    CodeLabel *pNullLabel = NewCodeLabel();
    CodeLabel *pMultiCaseLabel = NULL;

    _ASSERTE(THIS_kREG == kECX); //If this macro changes, have to change hardcoded emit below

    // The caller of this stub uses __cdecl instead of __fastcall so the argument needs
    // to be read of the stack
    // Note that the MethodDesc is currently on the top of the stack so the Arg1 is shifted by one slot 
    static const BYTE initThis[] = { 0x8b, 0x4C, 0x24, 0x08 }; // mov ecx, [esp+8]
    EmitBytes(initThis, sizeof(initThis));

    // cmp THISREG, 0
    Emit16(0xf983);
    Emit8(0);

    // jz null
    X86EmitCondJump(pNullLabel, X86CondCode::kJZ);

    if (!fSingleCast) {
        pMultiCaseLabel = NewCodeLabel();

        _ASSERTE(COMDelegate::m_pPRField);

        // cmp dword ptr [THISREG + Delegate.PR], 0  ; multiple subscribers?
        X86EmitOffsetModRM(0x81, (X86Reg)7, THIS_kREG, Object::GetOffsetOfFirstField() + COMDelegate::m_pPRField->GetOffset());
        Emit32(0);

        // jnz MultiCase
        X86EmitCondJump(pMultiCaseLabel, X86CondCode::kJNZ);
    }

    // Only one subscriber. Do the simple jump.
    _ASSERTE(COMDelegate::m_pFPField);
    _ASSERTE(COMDelegate::m_pORField);

    // mov SCRATCHREG, [THISREG + Delegate.FP]  ; Save target stub in register
    X86EmitIndexRegLoad(SCRATCH_REGISTER_X86REG, THIS_kREG, Object::GetOffsetOfFirstField() + COMDelegate::m_pFPField->GetOffset());

    // mov THISREG, [THISREG + Delegate.OR]  ; replace "this" pointer
    X86EmitIndexRegLoad(THIS_kREG, THIS_kREG, Object::GetOffsetOfFirstField() + COMDelegate::m_pORField->GetOffset());

    // discard unwanted MethodDesc
    X86EmitAddEsp(sizeof(MethodDesc*));

    // Replace the pointer to a Delegate with the THIS pointer
    static const BYTE replaceArg1[] = { 0x89, 0x4C, 0x24, 0x04 }; // mov [esp+4], ecx
        EmitBytes(replaceArg1, sizeof(replaceArg1));

    // jmp SCRATCHREG
    Emit16(0xe0ff | (SCRATCH_REGISTER_X86REG<<8));

    if (!fSingleCast) {
        // There is a dependency between this and the StubLinkStubManager - dont' change
        // this without fixing up that.     --MiPanitz

        // The multiple subscriber case. Must create a frame to protect arguments during iteration.
        EmitLabel(pMultiCaseLabel);
    
        // Push a MulticastFrame on the stack.
        EmitMethodStubProlog(MulticastFrame::GetMethodFrameVPtr());
    
        // push edi     ;; Save EDI (want to use it as loop index)
        X86EmitPushReg(kEDI);
    
        // push ebx     ;; Save EBX (want to use it as tmp)
        X86EmitPushReg(kEBX);
    
        // xor edi,edi  ;; Loop counter: EDI=0,1,2...
        X86EmitZeroOutReg(kEDI);
    
        CodeLabel *pInvokeRecurseLabel = NewCodeLabel();
    
    
        // call InvokeRecurse               ;; start the recursion rolling
        X86EmitCall(pInvokeRecurseLabel, 0);
    
        // pop ebx     ;; Restore ebx
        X86EmitPopReg(kEBX);
    
        // pop edi     ;; Restore edi
        X86EmitPopReg(kEDI);
    
    
        // Epilog
        EmitMethodStubEpilog(numStackBytes, kNoTripStubStyle);
    
        // Entry:
        //   EDI == distance from head of delegate list
        // INVOKERECURSE:
    
        EmitLabel(pInvokeRecurseLabel);
    
        // This is disgusting. We can't use the current delegate pointer itself
        // as the recursion variable because gc can move it during the recursive call.
        // So we use the index itself and walk down the list from the promoted
        // head pointer each time.
    
    
        // mov SCRATCHREG, [esi + this]     ;; get head of list delegate
        X86EmitIndexRegLoad(SCRATCH_REGISTER_X86REG, kESI, MulticastFrame::GetOffsetOfThis());
    
        // mov ebx, edi
        Emit16(0xdf8b);
        CodeLabel *pLoop1Label = NewCodeLabel();
        CodeLabel *pEndLoop1Label = NewCodeLabel();
    
        // LOOP1:
        EmitLabel(pLoop1Label);
    
        // cmp ebx,0
        Emit16(0xfb83); Emit8(0);
    
        // jz ENDLOOP1
        X86EmitCondJump(pEndLoop1Label, X86CondCode::kJZ);
    
        // mov SCRATCHREG, [SCRATCHREG+Delegate._prev]
        X86EmitIndexRegLoad(SCRATCH_REGISTER_X86REG, SCRATCH_REGISTER_X86REG, Object::GetOffsetOfFirstField() + COMDelegate::m_pPRField->GetOffset());
    
        // dec ebx
        Emit8(0x4b);
    
        // jmp LOOP1
        X86EmitNearJump(pLoop1Label);
    
        //ENDLOOP1:
        EmitLabel(pEndLoop1Label);
    
        //    cmp SCRATCHREG,0      ;;done?
        Emit8(0x81);
        Emit8(0xf8 | SCRATCH_REGISTER_X86REG);
        Emit32(0);
    
    
        CodeLabel *pDoneLabel = NewCodeLabel();
    
        //    jz  done
        X86EmitCondJump(pDoneLabel, X86CondCode::kJZ);
    
        //    inc edi
        Emit8(0x47);
    
        //    call INVOKERECURSE    ;; cast to the tail
        X86EmitCall(pInvokeRecurseLabel, 0);
    
        //    dec edi
        Emit8(0x4f);
    
        // Gotta go retrieve the current delegate again.
    
        // mov SCRATCHREG, [esi + this]     ;; get head of list delegate
        X86EmitIndexRegLoad(SCRATCH_REGISTER_X86REG, kESI, MulticastFrame::GetOffsetOfThis());
    
        // mov ebx, edi
        Emit16(0xdf8b);
        CodeLabel *pLoop2Label = NewCodeLabel();
        CodeLabel *pEndLoop2Label = NewCodeLabel();
    
        // Loop2:
        EmitLabel(pLoop2Label);
    
        // cmp ebx,0
        Emit16(0xfb83); Emit8(0);
    
        // jz ENDLoop2
        X86EmitCondJump(pEndLoop2Label, X86CondCode::kJZ);
    
        // mov SCRATCHREG, [SCRATCHREG+Delegate._prev]
        X86EmitIndexRegLoad(SCRATCH_REGISTER_X86REG, SCRATCH_REGISTER_X86REG, Object::GetOffsetOfFirstField() + COMDelegate::m_pPRField->GetOffset());
    
        // dec ebx
        Emit8(0x4b);
    
        // jmp Loop2
        X86EmitNearJump(pLoop2Label);
    
        //ENDLoop2:
        EmitLabel(pEndLoop2Label);
    
    
        //    ..repush & reenregister args..
        INT32 ofs = numStackBytes + MulticastFrame::GetOffsetOfArgs();
        while (ofs != MulticastFrame::GetOffsetOfArgs())
        {
            ofs -= 4;
            X86EmitIndexPush(kESI, ofs);
        }
        // All the arguments are on the stack but the ECX register still needs to be initialized
        // because it is used by the shuffle thunk
        //    mov THISREG, [SCRATCHREG+Delegate.object]  ;;replace "this" poiner
        X86EmitIndexRegLoad(THIS_kREG, SCRATCH_REGISTER_X86REG, Object::GetOffsetOfFirstField() + COMDelegate::m_pORField->GetOffset());
  
        // Replace the pointer to a Delegate with the THIS pointer
        static const BYTE replaceArg0[] = { 0x89, 0x4C, 0x24, 0x00 }; // mov [esp+4], ecx
           EmitBytes(replaceArg0, sizeof(replaceArg0));
        //    call [SCRATCHREG+Delegate.target] ;; call current subscriber
        X86EmitOffsetModRM(0xff, (X86Reg)2, SCRATCH_REGISTER_X86REG, Object::GetOffsetOfFirstField() + COMDelegate::m_pFPField->GetOffset());
        INDEBUG(Emit8(0x90));       // Emit a nop after the call in debug so that
                                    // we know that this is a call that can directly call 
                                    // managed code
        X86EmitAddEsp(numStackBytes);
        // The debugger may need to stop here, so grab the offset of this code.
        EmitDebuggerIntermediateLabel();
        //
        //
        //  done:
        EmitLabel(pDoneLabel);
    
        X86EmitReturn(0);


    }

    // We're going to be clever, in that we're going to record the offset of the last instruction,
    // and the diff between this and the call behind us
    EmitPatchLabel();

    // Do a null throw
    EmitLabel(pNullLabel);

    X86EmitZeroOutReg(kEAX);

    X86EmitPushReg(kEAX);
    X86EmitPushReg(kEAX);
    X86EmitPushReg(kEAX);
    X86EmitPushReg(kEAX);
    X86EmitPushImm32(kNullReferenceException);
    X86EmitPushReg(kEAX);

    X86EmitCall(NewExternalCodeLabel((LPVOID) __FCThrow), 6*4);

    X86EmitReturn(0);
}

//little helper to generate code to move nbytes bytes of non Ref memory
void generate_noref_copy (unsigned nbytes, StubLinkerCPU* sl)
{
        if ((nbytes & ~0xC) == 0)               // Is it 4, 8, or 12 ?
        {
                while (nbytes > 0)
                {
                        sl->Emit8(0xa5);        // movsd
                        nbytes -= 4;
                }
                return;
        }

    //copy the start before the first pointer site
    sl->Emit8(0xb8+kECX);
    if ((nbytes & 3) == 0)
    {               // move words
        sl->Emit32(nbytes / sizeof(void*)); // mov ECX, size / 4
        sl->Emit16(0xa5f3);                 // repe movsd
    }
    else
    {
        sl->Emit32(nbytes);     // mov ECX, size
        sl->Emit16(0xa4f3);     // repe movsb
    }
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
// Emits code to do an array operation.
VOID StubLinkerCPU::EmitArrayOpStub(const ArrayOpScript* pArrayOpScript)
{
    THROWSCOMPLUSEXCEPTION();

    const UINT  locsize     = ARRAYOPLOCSIZE;
    const UINT  ofsadjust   = locsize - FramedMethodFrame::GetOffsetOfReturnAddress();

    // Working registers:
    //  THIS_kREG     (points to managed array)
    //  edi == total  (accumulates unscaled offset)
    //  esi == factor (accumulates the slice factor)
    const X86Reg kArrayRefReg = THIS_kREG;
    const X86Reg kTotalReg    = kEDI;
    const X86Reg kFactorReg   = kESI;

    CodeLabel *Epilog = NewCodeLabel();
    CodeLabel *Inner_nullexception = NewCodeLabel();
    CodeLabel *Inner_rangeexception = NewCodeLabel();
    CodeLabel *Inner_typeMismatchexception = 0;

    // Preserve the callee-saved registers
    _ASSERTE(ARRAYOPLOCSIZE - sizeof(MethodDesc*) == 8);
    X86EmitPushReg(kTotalReg);
    X86EmitPushReg(kFactorReg);

    // While ECX and EDX will always contain correct values when this 
    // stub is called from jitted code. It is possible that GC will happen
    // between the call and the stub in the prestub, so registers have to be refreshed
    static const BYTE refreshArg1[] = { 0x8b, 0x4C, 0x24, 0x10 }; // mov ecx, [esp+0x10]
    static const BYTE refreshArg2[] = { 0x8b, 0x54, 0x24, 0x14 }; // mov edx, [esp+0x14]
    EmitBytes(refreshArg1, sizeof(refreshArg1));
    EmitBytes(refreshArg2, sizeof(refreshArg2));
    // Check for null.
    X86EmitR2ROp(0x85, kArrayRefReg, kArrayRefReg);                         //   TEST ECX, ECX
    X86EmitCondJump(Inner_nullexception, X86CondCode::kJZ);     //   jz  Inner_nullexception

    // Do Type Check if needed
    if (pArrayOpScript->m_flags & ArrayOpScript::NEEDSTYPECHECK) {
        // throw exception if failed
        Inner_typeMismatchexception = NewCodeLabel();
        if (pArrayOpScript->m_op == ArrayOpScript::STORE) {
                                // Get the value to be stored.
            X86EmitEspOffset(0x8b, kEAX, pArrayOpScript->m_fValLoc + ofsadjust);

            X86EmitR2ROp(0x85, kEAX, kEAX);                                     //   TEST EAX, EAX
            CodeLabel *CheckPassed = NewCodeLabel();
            X86EmitCondJump(CheckPassed, X86CondCode::kJZ);             // storing NULL is OK

                        X86EmitOp(0x8b, kEAX, kEAX, 0);                                         // mov EAX, [EAX]
                                                                                                                // cmp EAX, [ECX+m_ElementType];
            X86EmitOp(0x3b, kEAX, kECX, offsetof(PtrArray, m_ElementType));
            X86EmitCondJump(CheckPassed, X86CondCode::kJZ);             // Exact match is OK

                        Emit8(0xA1);                                                                            // mov EAX, [g_pObjectMethodTable]
                        Emit32((DWORD)(size_t) &g_pObjectClass);
            X86EmitOp(0x3b, kEAX, kECX, offsetof(PtrArray, m_ElementType));
            X86EmitCondJump(CheckPassed, X86CondCode::kJZ);             // Assigning to array of object is OK


            X86EmitPushReg(kEDX);      // Save EDX
            X86EmitPushReg(kECX);      // pass array object

                                // get address of value to store
            X86EmitEspOffset(0x8d, kECX, pArrayOpScript->m_fValLoc + ofsadjust + 2*sizeof(void*));      // lea ECX, [ESP+offs]
                                // get address of 'this'
            X86EmitEspOffset(0x8d, kEDX, 0);    // lea EDX, [ESP]       (address of ECX)

            X86EmitPushReg(kEDX);      // to change calling convention we need to push the arguments onto 
            X86EmitPushReg(kECX);      // the stack
            X86EmitCall(NewExternalCodeLabel((LPVOID) ArrayStoreCheck), 0);
            X86EmitAddEsp(8); // add esp,0x8 - pop the arguments of the stack

            X86EmitPopReg(kECX);        // restore regs
            X86EmitPopReg(kEDX);

            X86EmitR2ROp(0x3B, kEAX, kEAX);                             //   CMP EAX, EAX
            X86EmitCondJump(Epilog, X86CondCode::kJNZ);         // This branch never taken, but epilog walker uses it

            X86EmitR2ROp(0x85, kEAX, kEAX);                             //   TEST EAX, EAX
            X86EmitCondJump(Inner_typeMismatchexception, X86CondCode::kJZ);

            EmitLabel(CheckPassed);
        }
        else if (pArrayOpScript->m_op == ArrayOpScript::LOADADDR) {
            // Load up the hidden type parameter into 'typeReg'

            X86Reg typeReg = kEAX;
            if (pArrayOpScript->m_typeParamReg != -1)
                typeReg = GetX86ArgumentRegisterFromOffset(pArrayOpScript->m_typeParamReg);
            else
                 X86EmitEspOffset(0x8b, kEAX, pArrayOpScript->m_typeParamOffs + ofsadjust);              // Guarenteed to be at 0 offset

            // EAX holds the typeHandle for the ARRAY.  This must be a ArrayTypeDesc*, so
            // mask off the low two bits to get the TypeDesc*
            X86EmitR2ROp(0x83, (X86Reg)4, kEAX);    //   AND EAX, 0xFFFFFFFC
            Emit8(0xFC);

            // Get the parameter of the parameterize type
            // move typeReg, [typeReg.m_Arg]
            X86EmitOp(0x8b, typeReg, typeReg, offsetof(ParamTypeDesc, m_Arg));

            // Compare this against the element type of the array.
            // cmp EAX, [ECX+m_ElementType];
            X86EmitOp(0x3b, typeReg, kECX, offsetof(PtrArray, m_ElementType));

            // Throw error if not equal
            X86EmitCondJump(Inner_typeMismatchexception, X86CondCode::kJNZ);
        }
    }

    CodeLabel* DoneCheckLabel = 0;
    if (pArrayOpScript->m_rank == 1 && pArrayOpScript->m_fHasLowerBounds) {
        DoneCheckLabel = NewCodeLabel();
        CodeLabel* NotSZArrayLabel = NewCodeLabel();

        // for rank1 arrays, we might actually have two different layouts depending on
        // if we are ELEMENT_TYPE_ARRAY or ELEMENT_TYPE_SZARRAY.

        // mov EAX, [ARRAY]          // EAX holds the method table
        X86EmitOp(0x8b, kEAX, kArrayRefReg);

        // cmp BYTE [EAX+m_NormType], ELEMENT_TYPE_SZARRAY
        static const BYTE code[] = {0x80, 0x78, offsetof(MethodTable, m_NormType), ELEMENT_TYPE_SZARRAY };
        EmitBytes(code, sizeof(code));

        // jnz NotSZArrayLabel
        X86EmitCondJump(NotSZArrayLabel, X86CondCode::kJNZ);

        //Load the passed-in index into the scratch register.
        const ArrayOpIndexSpec *pai = pArrayOpScript->GetArrayOpIndexSpecs();
        X86Reg idxReg = SCRATCH_REGISTER_X86REG;
        if (pai->m_freg)
            idxReg = GetX86ArgumentRegisterFromOffset(pai->m_idxloc);
        else
            X86EmitEspOffset(0x8b, SCRATCH_REGISTER_X86REG, pai->m_idxloc + ofsadjust);

        // cmp idxReg, [kArrayRefReg + LENGTH]
        X86EmitOp(0x3b, idxReg, kArrayRefReg, ArrayBase::GetOffsetOfNumComponents());

        // jae Inner_rangeexception
        X86EmitCondJump(Inner_rangeexception, X86CondCode::kJAE);

        X86EmitR2ROp(0x8b, kTotalReg, idxReg);

        // sub ARRAY. 8                     // 8 is accounts for the Lower bound and Dim count in the ARRAY
        X86EmitSubReg(kArrayRefReg, 8);     // adjust this pointer so that indexing works out for SZARRAY

        X86EmitNearJump(DoneCheckLabel);
        EmitLabel(NotSZArrayLabel);
    }

    {
        // For each index, range-check and mix into accumulated total.
        UINT idx = pArrayOpScript->m_rank;
        BOOL firstTime = TRUE;
        while (idx--) {
            const ArrayOpIndexSpec *pai = pArrayOpScript->GetArrayOpIndexSpecs() + idx;

            //Load the passed-in index into the scratch register.
            if (pai->m_freg) {
                X86Reg srcreg = GetX86ArgumentRegisterFromOffset(pai->m_idxloc);
                X86EmitR2ROp(0x8b, SCRATCH_REGISTER_X86REG, srcreg);
            } else {
                X86EmitEspOffset(0x8b, SCRATCH_REGISTER_X86REG, pai->m_idxloc + ofsadjust);
            }

            // sub SCRATCH, [kArrayRefReg + LOWERBOUND]
            if (pArrayOpScript->m_fHasLowerBounds) {
                X86EmitOp(0x2b, SCRATCH_REGISTER_X86REG, kArrayRefReg, pai->m_lboundofs);
            }

            // cmp SCRATCH, [kArrayRefReg + LENGTH]
            X86EmitOp(0x3b, SCRATCH_REGISTER_X86REG, kArrayRefReg, pai->m_lengthofs);

            // jae Inner_rangeexception
            X86EmitCondJump(Inner_rangeexception, X86CondCode::kJAE);


            // SCRATCH == idx - LOWERBOUND
            //
            // imul SCRATCH, FACTOR
            if (!firstTime) {  //Can skip the first time since FACTOR==1
                Emit8(0x0f);        //prefix for IMUL
                X86EmitR2ROp(0xaf, SCRATCH_REGISTER_X86REG, kFactorReg);
            }

            // TOTAL += SCRATCH
            if (firstTime) {
                // First time, we must zero-init TOTAL. Since
                // zero-initing and then adding is just equivalent to a
                // "mov", emit a "mov"
                //    mov  TOTAL, SCRATCH
                X86EmitR2ROp(0x8b, kTotalReg, SCRATCH_REGISTER_X86REG);
            } else {
                //    add  TOTAL, SCRATCH
                X86EmitR2ROp(0x03, kTotalReg, SCRATCH_REGISTER_X86REG);
            }

            // FACTOR *= [kArrayRefReg + LENGTH]
            if (idx != 0) {  // No need to update FACTOR on the last iteration
                //  since we won't use it again

                if (firstTime) {
                    // must init FACTOR to 1 first: hence,
                    // the "imul" becomes a "mov"
                    // mov FACTOR, [kArrayRefReg + LENGTH]
                    X86EmitOp(0x8b, kFactorReg, kArrayRefReg, pai->m_lengthofs);
                } else {
                    // imul FACTOR, [kArrayRefReg + LENGTH]
                    Emit8(0x0f);        //prefix for IMUL
                    X86EmitOp(0xaf, kFactorReg, kArrayRefReg, pai->m_lengthofs);
                }
            }

            firstTime = FALSE;
        }
    }

    if (DoneCheckLabel != 0)
        EmitLabel(DoneCheckLabel);

    // Pass these values to X86EmitArrayOp() to generate the element address.
    X86Reg elemBaseReg   = kArrayRefReg;
    X86Reg elemScaledReg = kTotalReg;
    UINT32 elemScale     = pArrayOpScript->m_elemsize;
    UINT32 elemOfs       = pArrayOpScript->m_ofsoffirst;

    if (!(elemScale == 1 || elemScale == 2 || elemScale == 4 || elemScale == 8)) {
        switch (elemScale) {
            // No way to express this as a SIB byte. Fold the scale
            // into TOTAL.

            case 16:
                // shl TOTAL,4
                Emit8(0xc1);
                Emit8(0340|kTotalReg);
                Emit8(4);
                break;


            case 32:
                // shl TOTAL,5
                Emit8(0xc1);
                Emit8(0340|kTotalReg);
                Emit8(5);
                break;


            case 64:
                // shl TOTAL,6
                Emit8(0xc1);
                Emit8(0340|kTotalReg);
                Emit8(6);
                break;

            default:
                // imul TOTAL, elemScale
                X86EmitR2ROp(0x69, kTotalReg, kTotalReg);
                Emit32(elemScale);
                break;
        }
        elemScale = 1;
    }

    // Now, do the operation:

    switch (pArrayOpScript->m_op) {
        case pArrayOpScript->LOADADDR:
            // lea eax, ELEMADDR
            X86EmitOp(0x8d, kEAX, elemBaseReg, elemOfs, elemScaledReg, elemScale);
            break;


        case pArrayOpScript->LOAD:
            if (pArrayOpScript->m_flags & pArrayOpScript->HASRETVALBUFFER)
            {
                // Ensure that these registers have been saved!
                _ASSERTE(kTotalReg == kEDI);
                _ASSERTE(kFactorReg == kESI);

                //lea esi, ELEMADDR
                X86EmitOp(0x8d, kESI, elemBaseReg, elemOfs, elemScaledReg, elemScale);

                _ASSERTE(pArrayOpScript->m_fRetBufInReg);
                // mov edi, retbufptr
                X86EmitR2ROp(0x8b, kEDI, GetX86ArgumentRegisterFromOffset(pArrayOpScript->m_fRetBufLoc));

            COPY_VALUE_CLASS:
                {
                    size_t size = pArrayOpScript->m_elemsize;
                    size_t total = 0;
                    if(pArrayOpScript->m_gcDesc)
                    {
                        CGCDescSeries* cur = pArrayOpScript->m_gcDesc->GetHighestSeries();
                        // special array encoding
                        _ASSERTE(cur < pArrayOpScript->m_gcDesc->GetLowestSeries());
                        if ((cur->startoffset-elemOfs) > 0)
                            generate_noref_copy ((unsigned) (cur->startoffset - elemOfs), this);
                        total += cur->startoffset - elemOfs;

                        SSIZE_T cnt = (SSIZE_T) pArrayOpScript->m_gcDesc->GetNumSeries();

                        for (SSIZE_T __i = 0; __i > cnt; __i--)
                        {
                            HALF_SIZE_T skip =  cur->val_serie[__i].skip;
                            HALF_SIZE_T nptrs = cur->val_serie[__i].nptrs;
                            total += nptrs*sizeof (DWORD*);
                            do
                            {
                                X86EmitIndexPush(kESI, 0);       // source value (push dword ptr [esi])
                                X86EmitPushReg(kEDI);            // assigned-to pointer (push edi)
                                X86EmitCall(NewExternalCodeLabel((LPVOID) JIT_WriteBarrier), 0);
                                X86EmitAddEsp(8);                // clean up the stack
                                X86EmitAddReg(kEDI, 4);          // add edi, 4
                                X86EmitAddReg(kESI, 4);          // add esi, 4 (esi is used by generate_noref_copy)
                            } while (--nptrs);
                            if (skip > 0)
                            {
                                //check if we are at the end of the series
                                if (__i == (cnt + 1))
                                    skip = skip - (HALF_SIZE_T)(cur->startoffset - elemOfs);
                                if (skip > 0)
                                    generate_noref_copy (skip, this);
                            }
                            total += skip;
                        }

                        _ASSERTE (size == total);
                    }
                    else
                    {
                        // no ref anywhere, just copy the bytes.
                        _ASSERTE (size);
                        generate_noref_copy ((unsigned)size, this);
                    }
                }
            }
            else
            {
                switch (pArrayOpScript->m_elemsize) {
                    case 1:
                        // mov[zs]x eax, byte ptr ELEMADDR
                        Emit8(0x0f);
                        X86EmitOp(pArrayOpScript->m_signed ? 0xbe : 0xb6, kEAX, elemBaseReg, elemOfs, elemScaledReg, elemScale);
                        break;

                    case 2:
                        // mov[zs]x eax, word ptr ELEMADDR
                        Emit8(0x0f);
                        X86EmitOp(pArrayOpScript->m_signed ? 0xbf : 0xb7, kEAX, elemBaseReg, elemOfs, elemScaledReg, elemScale);
                        break;

                    case 4:
                        if (pArrayOpScript->m_flags & pArrayOpScript->ISFPUTYPE) {
                            // fld dword ptr ELEMADDR
                            X86EmitOp(0xd9, (X86Reg)0, elemBaseReg, elemOfs, elemScaledReg, elemScale);
                        } else {
                            // mov eax, ELEMADDR
                            X86EmitOp(0x8b, kEAX, elemBaseReg, elemOfs, elemScaledReg, elemScale);
                        }
                        break;

                    case 8:
                        if (pArrayOpScript->m_flags & pArrayOpScript->ISFPUTYPE) {
                            // fld qword ptr ELEMADDR
                            X86EmitOp(0xdd, (X86Reg)0, elemBaseReg, elemOfs, elemScaledReg, elemScale);
                        } else {
                            // mov eax, ELEMADDR
                            X86EmitOp(0x8b, kEAX, elemBaseReg, elemOfs, elemScaledReg, elemScale);
                            // mov edx, ELEMADDR + 4
                            X86EmitOp(0x8b, kEDX, elemBaseReg, elemOfs + 4, elemScaledReg, elemScale);
                        }
                        break;


                    default:
                        _ASSERTE(0);
                }
            }

            break;

        case pArrayOpScript->STORE:
            _ASSERTE(!(pArrayOpScript->m_fValInReg)); // on x86, value will never get a register: so too lazy to implement that case

            switch (pArrayOpScript->m_elemsize) {

                case 1:
                    // mov SCRATCH, [esp + valoffset]
                    X86EmitEspOffset(0x8b, SCRATCH_REGISTER_X86REG, pArrayOpScript->m_fValLoc + ofsadjust);
                    // mov byte ptr ELEMADDR, SCRATCH.b
                    X86EmitOp(0x88, SCRATCH_REGISTER_X86REG, elemBaseReg, elemOfs, elemScaledReg, elemScale);
                    break;
                case 2:
                    // mov SCRATCH, [esp + valoffset]
                    X86EmitEspOffset(0x8b, SCRATCH_REGISTER_X86REG, pArrayOpScript->m_fValLoc + ofsadjust);
                    // mov word ptr ELEMADDR, SCRATCH.w
                    Emit8(0x66);
                    X86EmitOp(0x89, SCRATCH_REGISTER_X86REG, elemBaseReg, elemOfs, elemScaledReg, elemScale);
                    break;
                case 4:
                    // mov SCRATCH, [esp + valoffset]
                    X86EmitEspOffset(0x8b, SCRATCH_REGISTER_X86REG, pArrayOpScript->m_fValLoc + ofsadjust);
                    if (pArrayOpScript->m_flags & pArrayOpScript->NEEDSWRITEBARRIER) {
                        _ASSERTE(SCRATCH_REGISTER_X86REG == kEAX); // value to store is already in EAX where we want it.
                        // lea edx, ELEMADDR
                        X86EmitOp(0x8d, kEDX, elemBaseReg, elemOfs, elemScaledReg, elemScale);

                        // call __cdecl JIT_WriteBarrier(OBJECTREF *dst, OBJECTREF ref)
                        X86EmitPushReg(SCRATCH_REGISTER_X86REG); // push eax "ref"
                        X86EmitPushReg(kEDX);                    // push edx "dst"
                        X86EmitCall(NewExternalCodeLabel((LPVOID)JIT_WriteBarrier), 0); 
                        X86EmitAddEsp(8); // add esp,0x8 - pop the arguments of the stack

                    } else {
                        // mov ELEMADDR, SCRATCH
                        X86EmitOp(0x89, SCRATCH_REGISTER_X86REG, elemBaseReg, elemOfs, elemScaledReg, elemScale);
                    }
                    break;

                case 8:
                    if (!pArrayOpScript->m_gcDesc) {
                        // mov SCRATCH, [esp + valoffset]
                        X86EmitEspOffset(0x8b, SCRATCH_REGISTER_X86REG, pArrayOpScript->m_fValLoc + ofsadjust);
                        // mov ELEMADDR, SCRATCH
                        X86EmitOp(0x89, SCRATCH_REGISTER_X86REG, elemBaseReg, elemOfs, elemScaledReg, elemScale);
                        _ASSERTE(!(pArrayOpScript->m_fValInReg)); // on x86, value will never get a register: so too lazy to implement that case
                        // mov SCRATCH, [esp + valoffset + 4]
                        X86EmitEspOffset(0x8b, SCRATCH_REGISTER_X86REG, pArrayOpScript->m_fValLoc + ofsadjust + 4);
                        // mov ELEMADDR+4, SCRATCH
                        X86EmitOp(0x89, SCRATCH_REGISTER_X86REG, elemBaseReg, elemOfs+4, elemScaledReg, elemScale);
                        break;
                    }
                        // FALL THROUGH
                default:
                    // Ensure that these registers have been saved!
                    _ASSERTE(kTotalReg == kEDI);
                    _ASSERTE(kFactorReg == kESI);
                    // lea esi, [esp + valoffset]
                    X86EmitEspOffset(0x8d, kESI, pArrayOpScript->m_fValLoc + ofsadjust);
                    // lea edi, ELEMADDR
                    X86EmitOp(0x8d, kEDI, elemBaseReg, elemOfs, elemScaledReg, elemScale);
                    goto COPY_VALUE_CLASS;
            }
            break;

        default:
            _ASSERTE(0);
    }

        EmitLabel(Epilog);
    // Restore the callee-saved registers
    _ASSERTE(ARRAYOPLOCSIZE - sizeof(MethodDesc*) == 8);
    X86EmitPopReg(kFactorReg);
    X86EmitPopReg(kTotalReg);

    // Throw away methoddesc
    X86EmitPopReg(kECX); // junk register

    // ret N
    X86EmitReturn(0);
   // Exception points must clean up the stack for all those extra args:
    EmitLabel(Inner_nullexception);
    Emit8(0xb8);        // mov EAX, <stack cleanup>
    Emit32(pArrayOpScript->m_cbretpop);
    // kFactorReg and kTotalReg could not have been modified, but let's pop
    // them anyway for consistency and to avoid future bugs.
    X86EmitPopReg(kFactorReg);
    X86EmitPopReg(kTotalReg);
    X86EmitNearJump(NewExternalCodeLabel((LPVOID) ArrayOpStubNullException));

    EmitLabel(Inner_rangeexception);
    Emit8(0xb8);        // mov EAX, <stack cleanup>
    Emit32(pArrayOpScript->m_cbretpop);
    X86EmitPopReg(kFactorReg);
    X86EmitPopReg(kTotalReg);
    X86EmitNearJump(NewExternalCodeLabel((LPVOID) ArrayOpStubRangeException));

    if (pArrayOpScript->m_flags & pArrayOpScript->NEEDSTYPECHECK) {
        EmitLabel(Inner_typeMismatchexception);
        Emit8(0xb8);        // mov EAX, <stack cleanup>
        Emit32(pArrayOpScript->m_cbretpop);
        X86EmitPopReg(kFactorReg);
        X86EmitPopReg(kTotalReg);
        X86EmitNearJump(NewExternalCodeLabel((LPVOID) ArrayOpStubTypeMismatchException));
    }
}

//===========================================================================
// Emits code to throw a rank exception
VOID StubLinkerCPU::EmitRankExceptionThrowStub(UINT cbFixedArgs)
{
    THROWSCOMPLUSEXCEPTION();

    // mov eax, cbFixedArgs
    Emit8(0xb8 + kEAX);
    Emit32(cbFixedArgs);

    X86EmitNearJump(NewExternalCodeLabel((LPVOID) ThrowRankExceptionStub));
}


//===========================================================================
// Emits code to break into debugger
VOID StubLinkerCPU::EmitDebugBreak()
{
    // int3
    Emit8(0xCC);
}


//===========================================================================
// Emits code to touch pages
// Inputs:
//   eax = first byte of data
//   edx = first byte past end of data
//
// Trashes eax, edx, ecx
//
// Pass TRUE if edx is guaranteed to be strictly greater than eax.
VOID StubLinkerCPU::EmitPageTouch(BOOL fSkipNullCheck)
{
    THROWSCOMPLUSEXCEPTION();


    CodeLabel *pEndLabel = NewCodeLabel();
    CodeLabel *pLoopLabel = NewCodeLabel();

    if (!fSkipNullCheck) {
        // cmp eax,edx
        X86EmitR2ROp(0x3b, kEAX, kEDX);

        // jnb EndLabel
        X86EmitCondJump(pEndLabel, X86CondCode::kJNB);
    }

    _ASSERTE(0 == (PAGE_SIZE & (PAGE_SIZE-1)));

    // and eax, ~(PAGE_SIZE-1)
    Emit8(0x25);
    Emit32( ~( ((UINT32)PAGE_SIZE) - 1 ));

    EmitLabel(pLoopLabel);
    // mov cl, [eax]
    X86EmitOp(0x8a, kECX, kEAX);
    // add eax, PAGESIZE
    Emit8(0x05);
    Emit32(PAGE_SIZE);
    // cmp eax, edx
    X86EmitR2ROp(0x3b, kEAX, kEDX);
    // jb LoopLabel
    X86EmitCondJump(pLoopLabel, X86CondCode::kJB);

    EmitLabel(pEndLabel);

}

VOID StubLinkerCPU::EmitProfilerComCallProlog(PVOID pFrameVptr, X86Reg regFrame)
{
    if (pFrameVptr == UMThkCallFrame::GetUMThkCallFrameVPtr())
    {
        // Save registers
        X86EmitPushReg(kEAX);
        X86EmitPushReg(kECX);
        X86EmitPushReg(kEDX);

        // Load the methoddesc into ECX (UMThkCallFrame->m_pvDatum->m_pMD)
        X86EmitIndexRegLoad(kECX, regFrame, UMThkCallFrame::GetOffsetOfDatum());
        X86EmitIndexRegLoad(kECX, kECX, UMEntryThunk::GetOffsetOfMethodDesc());

#ifdef PROFILING_SUPPORTED
        // Push arguments and notify profiler
        X86EmitPushImm32(COR_PRF_TRANSITION_CALL);    // Reason
        X86EmitPushReg(kECX);                           // MethodDesc*
        X86EmitCall(NewExternalCodeLabel((LPVOID)ProfilerUnmanagedToManagedTransitionMD), 8);
#endif // PROFILING_SUPPORTED

        // Restore registers
        X86EmitPopReg(kEDX);
        X86EmitPopReg(kECX);
        X86EmitPopReg(kEAX);
    }


    // Unrecognized frame vtbl
    else
    {
        _ASSERTE(!"Unrecognized vtble passed to EmitComMethodStubProlog with profiling turned on.");
    }
}

VOID StubLinkerCPU::EmitProfilerComCallEpilog(PVOID pFrameVptr, X86Reg regFrame)
{
    if (pFrameVptr == UMThkCallFrame::GetUMThkCallFrameVPtr())
    {
        // Save registers
        X86EmitPushReg(kEAX);
        X86EmitPushReg(kECX);
        X86EmitPushReg(kEDX);

        // Load the methoddesc into ECX (UMThkCallFrame->m_pvDatum->m_pMD)
        X86EmitIndexRegLoad(kECX, regFrame, UMThkCallFrame::GetOffsetOfDatum());
        X86EmitIndexRegLoad(kECX, kECX, UMEntryThunk::GetOffsetOfMethodDesc());

#ifdef PROFILING_SUPPORTED
        // Push arguments and notify profiler
        X86EmitPushImm32(COR_PRF_TRANSITION_RETURN);    // Reason
        X86EmitPushReg(kECX);                           // MethodDesc*
        X86EmitCall(NewExternalCodeLabel((LPVOID)ProfilerManagedToUnmanagedTransitionMD), 8);
#endif // PROFILING_SUPPORTED

        // Restore registers
        X86EmitPopReg(kEDX);
        X86EmitPopReg(kECX);
        X86EmitPopReg(kEAX);
    }


    // Unrecognized frame vtbl
    else
    {
        _ASSERTE(!"Unrecognized vtble passed to EmitComMethodStubEpilog with profiling turned on.");
    }
}


// helper to replace the prestub with a direct jump to code
void InterLockedShortcircuitPrestub(MethodDesc *pMD, const BYTE* codeAddr)
{
    StubCallInstrs *pStubCallInstrs = pMD->GetStubCallInstrs();
    _ASSERTE( (((size_t)pStubCallInstrs) & 7) == 0);

    C_ASSERT(sizeof(StubCallInstrs) == 8);
    UINT64 oldvalue = *(UINT64*)pStubCallInstrs;
    UINT64 newvalue = oldvalue;

    ((StubCallInstrs*)&newvalue)->m_op = 0xE9;  //JMP NEAR32

    INT_PTR JumpOffset = (INT_PTR)codeAddr - (INT_PTR)(1 + &(pStubCallInstrs->m_target));

    ((StubCallInstrs*)&newvalue)->m_target = (INT32)JumpOffset;

    /// We may have already performed the update 
    if (newvalue != *((UINT64 *)pStubCallInstrs)) {

        {
            InterlockedCompareExchange8b((UINT64 *)pStubCallInstrs,
                                        &newvalue,
                                        &oldvalue);
        }
    }
}

void UMEntryThunkCode::Encode(BYTE* pTargetCode, void* pvSecretParam)
{
#ifdef _DEBUG
    m_alignpad[0] = X86_INSTR_INT3;
    m_alignpad[1] = X86_INSTR_INT3;
#endif // _DEBUG
    m_movEAX     = X86_INSTR_MOV_EAX_IMM32;
    m_uet        = pvSecretParam;
    m_jmp        = X86_INSTR_JMP_REL32;
    m_execstub   = (BYTE*) ((pTargetCode) - (4+((BYTE*)&m_execstub)));
}

UMEntryThunk* UMEntryThunk::Decode(LPVOID pCallback)
{
    if (*((BYTE*)pCallback) != 0xb8 ||
        ( ((size_t)pCallback) & 3) != 2) {
        return NULL;
    }
    return *(UMEntryThunk**)( 1 + (BYTE*)pCallback );
}
