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
// CGENPPC.H -
//
// Various helper routines for generating ppc assembly code.
//
// DO NOT INCLUDE THIS FILE DIRECTLY - ALWAYS USE CGENSYS.H INSTEAD
//


#ifndef _PPC_
#error Should only include "cgenppc.h" for PPC builds
#endif

#ifndef __cgenppc_h__
#define __cgenppc_h__

#include "stublink.h"
#include "utilcode.h"

// preferred alignment for data
#define DATA_ALIGNMENT 4

class MethodDesc;
class FramedMethodFrame;
class Module;
struct ArrayOpScript;
struct DeclActionInfo;

// default return value type
typedef INT64 PlatformDefaultReturnType;

// CPU-dependent functions
Stub * GeneratePrestub();
// Non-CPU-specific helper functions called by the CPU-dependent code
extern "C" const BYTE * __stdcall PreStubWorker(PrestubMethodFrame *pPFrame);
extern "C" INT64 __stdcall NDirectGenericStubWorker(Thread *pThread, NDirectMethodFrame *pFrame);
extern "C" INT64 __stdcall PInvokeCalliWorker(Thread *pThread, PInvokeCalliFrame* pFrame);

extern "C" DWORD __stdcall GetSpecificCpuType();

inline LPVOID GetEEFuncEntryPoint(void* pvFunctionDescriptor)
{
    return pvFunctionDescriptor;
}

//**********************************************************************
// This structure captures the format of the METHOD_PREPAD area (behind
// the MethodDesc.)
//**********************************************************************
#include <pshpack1.h>

struct StubCallInstrs {
    // this has better be padded to 8 bytes, so use the space for the token
    UINT16 m_wTokenRemainder;      //a portion of the methoddef token. The rest is stored in the chunk
    BYTE   m_chunkIndex;           //index to recover chunk
    BYTE   m_pad;

    UINT32 instr[4]; 
    // mr r11, r12
    // lwz r12, 16(r12)
    // mtctr r12
    // bctr

    // actual code address
    BYTE* pCode;
};

#include <poppack.h>

#define TOKEN_IN_PREPAD                         1

#define METHOD_ENTRY_CHUNKS                     0 // 1 if we place method entry points in chunka
#define PREPAD_IS_CALLABLE                      1 // 1 if we place a method entry point in the prepad
#define PREPAD_IS_POINTER                       0 // 1 if we place a pointer to the method entry point in the prepad

#define METHOD_PREPAD                           24 // # extra bytes to allocate in addition to sizeof(Method)
#define METHOD_CALL_PRESTUB_SIZE                20 // sizeof(StubCallInstrs)
#define METHOD_ALIGN                            8 // required alignment for StubCallInstrs

#define JUMP_ALLOCATE_SIZE                      20 // # bytes to allocate for a jump instruction
#define METHOD_DESC_CHUNK_ALIGNPAD_BYTES        4 // # bytes required to pad MethodDescChunk to correct size
#define METHOD_ENTRY_CHUNK_ALIGNPAD_BYTES       0 // # bytes required to pad MethodDescChunk to correct size
#define COMPLUSCALL_METHOD_DESC_ALIGNPAD_BYTES  4 // # bytes required to pad ComPlusCallMethodDesc to correct size

#define CODE_SIZE_ALIGN                         4
#define CACHE_LINE_SIZE                         32

//=======================================================================
// IMPORTANT: This value is used to figure out how much to allocate
// for a fixed array of FieldMarshaler's. That means it must be at least
// as large as the largest FieldMarshaler subclass. This requirement
// is guarded by an assert.
//=======================================================================
#define MAXFIELDMARSHALERSIZE               36

//**********************************************************************
// Parameter size
//**********************************************************************

typedef INT32 StackElemType;
#define STACK_ELEM_SIZE sizeof(StackElemType)


// !! This expression assumes STACK_ELEM_SIZE is a power of 2.
#define StackElemSize(parmSize) (((parmSize) + STACK_ELEM_SIZE - 1) & ~((ULONG)(STACK_ELEM_SIZE - 1)))

// Get value of actual arg within widened arg
#define ExtractArg(stack, type)   (*(type *) ((BYTE*)stack + StackElemSize(sizeof(type)) - sizeof(type)))

// The stack should be aligned on 16 bytes
#define AlignStack(stack)  (((stack)+15)&~15)

inline BYTE *getStubCallAddr(MethodDesc *pMD) {
    return ((BYTE*)pMD) - METHOD_CALL_PRESTUB_SIZE;
}

inline BYTE *getStubJumpAddr(BYTE *pBuf) {
    return pBuf;
}

//**********************************************************************
// Frames
//**********************************************************************

#include "eecallconv.h"

//--------------------------------------------------------------------
// This represents some of the FramedMethodFrame fields that are
// stored at negative offsets.
//--------------------------------------------------------------------
struct CalleeSavedRegisters {
    INT32   cr;                                 // cr
    INT32   r[NUM_CALLEESAVED_REGISTERS];       // r13 .. r31
    DOUBLE  f[NUM_FLOAT_CALLEESAVED_REGISTERS]; // fpr14 .. fpr31
};

//--------------------------------------------------------------------
// This represents the arguments that are stored in volatile registers.
// This should not overlap the CalleeSavedRegisters since those are already
// saved separately and it would be wasteful to save the same register twice.
// If we do use a non-volatile register as an argument, then the ArgIterator
// will probably have to communicate this back to the PromoteCallerStack
// routine to avoid a double promotion.
//--------------------------------------------------------------------
struct ArgumentRegisters {
    INT32   r[NUM_ARGUMENT_REGISTERS];          // r3 .. r10
    DOUBLE  f[NUM_FLOAT_ARGUMENT_REGISTERS];    // fpr1 .. fpr13
};

//--------------------------------------------------------------------
// PPC linkage area
//--------------------------------------------------------------------
struct LinkageArea {
    INT32 SavedSP; // stack pointer
    INT32 SavedCR; // flags
    INT32 SavedLR; // return address
    INT32 Reserved1;
    INT32 Reserved2;
    INT32 Reserved3;
};

// forward decl
typedef struct _REGDISPLAY REGDISPLAY, *PREGDISPLAY;

// Sufficient context for Try/Catch restoration.
struct EHContext {
    // R1 = SP
    // R12 = IP
    // R30 = FP

    INT32       R[32];
    DOUBLE      F[NUM_FLOAT_CALLEESAVED_REGISTERS]; // fpr14 .. fpr31
    INT32       CR;

    void Setup(LPVOID resumePC, PREGDISPLAY regs);

    inline LPVOID GetSP() {
        return (LPVOID)(UINT_PTR)R[1];
    }
    inline void SetSP(LPVOID esp) {
        R[1] = (ULONG)(size_t)esp;
    }

    inline LPVOID GetFP() {
        return (LPVOID)(UINT_PTR)R[30];
    }

    inline void SetArg(LPVOID arg) {
        R[11] = (UINT32)(size_t)arg;
    }
};


#define ARGUMENTREGISTERS_SIZE sizeof(ArgumentRegisters)


#define PLATFORM_FRAME_ALIGN(val) (val)


//**********************************************************************
// Exception handling
//**********************************************************************

inline LPVOID GetIP(CONTEXT *context) {
    return (LPVOID)(size_t)context->Iar;
}

inline void SetIP(CONTEXT *context, LPVOID eip) {
    context->Iar = (ULONG)(size_t)eip;
}

inline LPVOID GetSP(CONTEXT *context) {
    return (LPVOID)(size_t)context->Gpr1;
}

extern "C" LPVOID GetSP();

inline void SetSP(CONTEXT *context, LPVOID esp) {
    context->Gpr1 = (ULONG)(size_t)esp;
}

inline LPVOID GetFP(PCONTEXT context) {
    return (LPVOID)(UINT_PTR)context->Gpr30;
}

inline void SetFP(PCONTEXT context, LPVOID ebp) {
    context->Gpr30 = (ULONG)(size_t)ebp;
}

inline void emitCall(LPBYTE pBuffer, LPVOID target)
{
    UINT32* p = (UINT32*)pBuffer;
    p[0] = 0x7D8B6378; // mr r11, r12
    p[1] = 0x818C0010; // lwz r12, 16(r12)
    p[2] = 0x7D8903A6; // mtctr r12
    p[3] = 0x4E800420; // bctr
    p[4] = (UINT32)(size_t)target; // <actual code address>
    FlushInstructionCache(GetCurrentProcess(), p, 16);
}

inline void emitStubCall(MethodDesc *pMD, BYTE *stubAddr) {
    emitCall(getStubCallAddr(pMD), stubAddr);
}

inline LPVOID getCallTarget(const BYTE *pCall)
{
    UINT32* pCode = (UINT32*)pCall;
    _ASSERTE(pCode[0] == 0x7D8B6378);
    _ASSERTE(pCode[1] == 0x818C0010);
    _ASSERTE(pCode[2] == 0x7D8903A6);
    _ASSERTE(pCode[3] == 0x4E800420);
    return (LPVOID)(size_t)(pCode[4]);
}

inline void emitJump(LPBYTE pBuffer, LPVOID target)
{
    emitCall(pBuffer, target);
}

inline void updateJumpTarget(LPBYTE pBuffer, LPVOID target)
{
    UINT32* pCode = (UINT32*)pBuffer;
    _ASSERTE(pCode[0] == 0x7D8B6378);
    _ASSERTE(pCode[1] == 0x818C0010);
    _ASSERTE(pCode[2] == 0x7D8903A6);
    _ASSERTE(pCode[3] == 0x4E800420);
    pCode[4] = (UINT32)(size_t)target;
}

inline LPVOID getJumpTarget(const BYTE *pJump)
{
    return getCallTarget(pJump);
}

inline const BYTE* setCallAddrInterlocked(MethodDesc *pMD, const BYTE* stubAddr, 
                                     const BYTE* expectedStubAddr)
{
    StubCallInstrs *pInstrs = ((StubCallInstrs*)pMD) - 1;

    PVOID result =
      FastInterlockCompareExchangePointer((void**)&pInstrs->pCode,
                                          (void *)stubAddr,
                                          (void *)expectedStubAddr);

    // result is the previous value of the stub
    return (const BYTE*)result;
}

inline void InterLockedShortcircuitPrestub(MethodDesc *pMD, const BYTE* codeAddr)
{
    StubCallInstrs *pInstrs = ((StubCallInstrs*)pMD) - 1;

    /// We may have already performed the update 
    if (codeAddr != pInstrs->pCode) {
        FastInterlockExchangePointer((void**)&pInstrs->pCode, (void*)codeAddr);
        pInstrs->instr[0] = 0x60000000; /* nop */
    }
}

inline BOOL isShortcircuitedStub(MethodDesc *pMD)
{
    StubCallInstrs *pInstrs = ((StubCallInstrs*)pMD) - 1;
    return pInstrs->instr[0] != 0x7D8B6378; /* mr r11, r12 */
}

inline const BYTE *getStubAddr(MethodDesc *pMD) {
    StubCallInstrs *pInstrs = ((StubCallInstrs*)pMD) - 1;
    return (const BYTE*)pInstrs->pCode;
}

//----------------------------------------------------------
// Marshalling Language support
//----------------------------------------------------------
typedef INT32 SignedParmSourceType;
typedef UINT32 UnsignedParmSourceType;
typedef float FloatParmSourceType;
typedef double DoubleParmSourceType;
typedef INT32 SignedI1TargetType;
typedef UINT32 UnsignedI1TargetType;
typedef INT32 SignedI2TargetType;
typedef UINT32 UnsignedI2TargetType;
typedef INT32 SignedI4TargetType;
typedef UINT32 UnsignedI4TargetType;


	// This instruction API meaning is do whatever is needed to 
	// when you want to indicate to the CPU that your are busy waiting
	// (this is a good time for this CPU to give up any resources that other
	// processors might put to good use).   On many machines this is a nop.
FORCEINLINE void pause()
{
}


// Memory Barrier
inline void MemoryBarrier()
{
    __asm__ __volatile__ ("sync");
}


inline int MLParmSize(int parmSize)
{
    return ((parmSize + sizeof(INT32) - 1) & ~((ULONG)(sizeof(INT32) - 1)));
}


EXTERN_C void __stdcall setFPReturn(int fpSize, INT64 retVal);
EXTERN_C void __stdcall getFPReturn(int fpSize, INT64 *pretval);

//----------------------------------------------------------------------
// Encodes PPC registers.
//----------------------------------------------------------------------
enum PPCReg {
    kR0 = 0,
    kR1 = 1,
    kR2 = 2,
    kR3 = 3,
    kR4 = 4,
    kR5 = 5,
    kR6 = 6,
    kR7 = 7,
    kR8 = 8,
    kR9 = 9,
    kR10 = 10,
    kR11 = 11,
    kR12 = 12,
    kR13 = 13,
    kR14 = 14,
    kR15 = 15,
    kR16 = 16,
    kR17 = 17,
    kR18 = 18,
    kR19 = 19,
    kR20 = 20,
    kR21 = 21,
    kR22 = 22,
    kR23 = 23,
    kR24 = 24,
    kR25 = 25,
    kR26 = 26,
    kR27 = 27,
    kR28 = 28,
    kR29 = 29,
    kR30 = 30,
    kR31 = 31,
};




//----------------------------------------------------------------------
// Encodes PPC conditional jumps.
//----------------------------------------------------------------------
class PPCCondCode {
    public:
        enum cc {
            kGE     = 0x00800000,
            kLT     = 0x01800000,

            kLE     = 0x00810000,
            kGT     = 0x01810000,

            kNE     = 0x00820000,
            kEQ     = 0x01820000,

            kAlways = 0x02800000
        };
};


//----------------------------------------------------------------------
// StubLinker with extensions for generating PPC code.
//----------------------------------------------------------------------
class StubLinkerCPU : public StubLinker
{
    public:
        // helper for emitting instructions with reg, reg, disp16 format
        VOID PPCEmitRegRegDisp16(UINT32 code, PPCReg r1, PPCReg r2, int disp16);

        // helper for emitting instructions with reg, reg, reg format
        VOID PPCEmitRegRegReg(UINT32 code, PPCReg r1, PPCReg r2, PPCReg r3);

        // rD = *(rA+d)
        VOID PPCEmitLwz(PPCReg rD, PPCReg rA, int d = 0);

        // rD = rA+d
        VOID PPCEmitAddi(PPCReg rD, PPCReg rA, int d = 0);

        // cr = (rA - d)
        VOID PPCEmitCmpwi(PPCReg rA, int d = 0);

        // cr = (rA - rB)
        VOID PPCEmitCmpw(PPCReg rA, PPCReg rB);

        // *(rA+d) = rS
        VOID PPCEmitStw(PPCReg rS, PPCReg rA, int d = 0);

        // rA = rS
        VOID PPCEmitMr(PPCReg rA, PPCReg rS);

        // rD = value
        VOID PPCEmitLoadImm(PPCReg rD, INT value);

        // rD = value
        // special flavor of LoadImm for stubs that have to have predictable layout
        VOID PPCEmitLoadImmNonoptimized(PPCReg rD, INT value);

        // call target
        VOID PPCEmitCall(CodeLabel *target);

        // conditional branch
        VOID PPCEmitBranch(CodeLabel *target, PPCCondCode::cc cond = PPCCondCode::kAlways);

        // indirect branch
        VOID PPCEmitBranchR12();

        // indirect call
        VOID PPCEmitCallR12();

        // rD = GetThread()
        VOID PPCEmitCurrentThreadFetch(PPCReg rD);

        // *(ArgumentRegisters*)(basereg + ofs) = <machine state>
        VOID EmitArgumentRegsSave(PPCReg basereg, int ofs);
        // <machine state> = *(ArgumentRegisters*)(basereg + ofs)
        VOID EmitArgumentRegsRestore(PPCReg basereg, int ofs);

        // *(CalleeSavedRegisters*)(basereg + ofs) = <machine state>
        VOID EmitCalleeSavedRegsSave(PPCReg basereg, int ofs);
        // <machine state> = *(CalleeSavedRegisters*)(basereg + ofs)
        VOID EmitCalleeSavedRegsRestore(PPCReg basereg, int ofs);

        // These are used to emit calls to notify the profiler of transitions in and out of
        // managed code through COM->COM+ interop or N/Direct
        VOID EmitProfilerComCallProlog(PVOID pFrameVptr);
        VOID EmitProfilerComCallEpilog(PVOID pFrameVptr);

        void EmitComMethodStubProlog(LPVOID pFrameVptr, LPVOID pSEHHandler,
                                     BOOL bShouldProfile);

        void EmitComMethodStubEpilog(LPVOID pFrameVptr, 
                            LPVOID pSEHHAndler, BOOL bShouldProfile, BOOL bReloop);

        VOID EmitMethodStubProlog(LPVOID pFrameVptr, BOOL fDeregister = TRUE);
        VOID EmitMethodStubEpilog(StubStyle style);

        VOID EmitUnboxMethodStub(MethodDesc* pRealMD);

        VOID EmitSecurityWrapperStub(UINT numStackBytes, MethodDesc* pMD, BOOL fToStub, LPVOID pRealStub, DeclActionInfo *pActions);
        VOID EmitSecurityInterceptorStub(MethodDesc* pMD, BOOL fToStub, LPVOID pRealStub, DeclActionInfo *pActions);

        //===========================================================================
        // Emits code for MulticastDelegate.Invoke()
        VOID EmitMulticastInvoke(UINT numStackBytes, BOOL fSingleCast, BOOL fHasReturnBuffer);

        //===========================================================================
        // Emits code to adjust for a static delegate target.
        VOID EmitShuffleThunk(struct ShuffleEntry *pShuffeEntryArray);


        //===========================================================================
        // Emits code to do an array operation.
        VOID EmitArrayOpStub(const ArrayOpScript*);

        //===========================================================================
        // Emits code to throw a rank exception
        VOID EmitRankExceptionThrowStub(UINT cbFixedArgs);

        //===========================================================================
        // Emits code to break into debugger
        VOID EmitDebugBreak();

    public:
        static BOOL Init();

    private:
        PPCReg ArrayOpEnregisterFromOffset(UINT offset, PPCReg regScratch);
};


// Adjust the generic interlocked operations for any platform specific ones we
// might have.
void InitFastInterlockOps();



// SEH info forward declarations

struct ComToManagedExRecord; // defined in cgenppc.cpp

inline BOOL IsUnmanagedValueTypeReturnedByRef(UINT sizeofvaluetype) 
{
    return TRUE;
}

inline BOOL IsManagedValueTypeReturnedByRef(UINT sizeofvaluetype) 
{
    return TRUE;
}

struct UMEntryThunkCode
{
    DWORD           m_code[4];
    const BYTE *    m_execstub; // pointer to destination code

    void Encode(BYTE* pTargetCode, void* pvSecretParam);

    LPCBYTE GetEntryPoint() const
    {
        return (LPCBYTE)&m_code;
    }

    static int GetEntryPointOffset()
    {
        return 0;
    }
};

#endif // __cgenppc_h__
