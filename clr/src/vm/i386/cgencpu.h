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
// CGENX86.H -
//
// Various helper routines for generating x86 assembly code.
//
// DO NOT INCLUDE THIS FILE DIRECTLY - ALWAYS USE CGENSYS.H INSTEAD
//


#ifndef _X86_
#error Should only include "cgenx86.h" for X86 builds
#endif

#ifndef __cgenx86_h__
#define __cgenx86_h__

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

struct StubCallInstrs
{
    UINT16      m_wTokenRemainder;      //a portion of the methoddef token. The rest is stored in the chunk
    BYTE        m_chunkIndex;           //index to recover chunk

// This is a stable and efficient entrypoint for the method
    BYTE        m_op;                   //this is either a jump (0xe9) or a call (0xe8)
    UINT32      m_target;               //pc-relative target for jump or call
};

#include <poppack.h>

#define TOKEN_IN_PREPAD                         1

#define METHOD_ENTRY_CHUNKS                     0 // 1 if we place method entry points in chunka
#define PREPAD_IS_CALLABLE                      1 // 1 if we place a method entry point in the prepad
#define PREPAD_IS_POINTER                       0 // 1 if we place a pointer to the method entry point in the prepad

#define METHOD_PREPAD                           8 // # extra bytes to allocate in addition to sizeof(Method)
#define METHOD_CALL_PRESTUB_SIZE                5 // x86: CALL(E8) xx xx xx xx
#define METHOD_ALIGN                            8 // required alignment for StubCallInstrs

#define JUMP_ALLOCATE_SIZE                      8 // # bytes to allocate for a jump instrucation
#define METHOD_DESC_CHUNK_ALIGNPAD_BYTES        4 // # bytes required to pad MethodDescChunk to correct size
#define METHOD_ENTRY_CHUNK_ALIGNPAD_BYTES       0 // # bytes required to pad MethodDescChunk to correct size
#define COMPLUSCALL_METHOD_DESC_ALIGNPAD_BYTES  4 // # bytes required to pad ComPlusCallMethodDesc to correct size

#define CODE_SIZE_ALIGN                         4
#define CACHE_LINE_SIZE                         32  // As per Intel Optimization Manual the cache line size is 32 bytes

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


inline BYTE *getStubCallAddr(MethodDesc *pMD) {
    return ((BYTE*)pMD) - METHOD_CALL_PRESTUB_SIZE;
}

inline BYTE *getStubJumpAddr(BYTE *pBuf) {
    return ((BYTE*)pBuf) + 3;   // have allocate 8 bytes, so go in 3 to find jmp instr point
}

//**********************************************************************
// Frames
//**********************************************************************
//--------------------------------------------------------------------
// This represents some of the FramedMethodFrame fields that are
// stored at negative offsets.
//--------------------------------------------------------------------
struct CalleeSavedRegisters {
    INT32       edi;
    INT32       esi;
    INT32       ebx;
    INT32       ebp;
};

//--------------------------------------------------------------------
// This represents the arguments that are stored in volatile registers.
// This should not overlap the CalleeSavedRegisters since those are already
// saved separately and it would be wasteful to save the same register twice.
// If we do use a non-volatile register as an argument, then the ArgIterator
// will probably have to communicate this back to the PromoteCallerStack
// routine to avoid a double promotion.
//
//--------------------------------------------------------------------
struct ArgumentRegisters {

#define DEFINE_ARGUMENT_REGISTER_BACKWARD(regname)  INT32 regname;
#include "eecallconv.h"

};

// forward decl
typedef struct _REGDISPLAY REGDISPLAY, *PREGDISPLAY;

// Sufficient context for Try/Catch restoration.
struct EHContext {
    INT32       Eax;
    INT32       Ebx;
    INT32       Ecx;
    INT32       Edx;
    INT32       Esi;
    INT32       Edi;
    INT32       Ebp;
    INT32       Esp;
    INT32       Eip;

    void Setup(LPVOID resumePC, PREGDISPLAY regs);

    inline LPVOID GetSP() {
        return (LPVOID)(UINT_PTR)Esp;
    }
    inline void SetSP(LPVOID esp) {
        Esp = (UINT32)(size_t)esp;
    }

    inline LPVOID GetFP() {
        return (LPVOID)(UINT_PTR)Ebp;
    }

    inline void SetArg(LPVOID arg) {
        Eax = (UINT32)(size_t)arg;
    }
};


#define ARGUMENTREGISTERS_SIZE sizeof(ArgumentRegisters)


#define PLATFORM_FRAME_ALIGN(val) (val)

#ifdef _DEBUG

//-----------------------------------------------------------------------
// Under DEBUG, stubs push 8 additional bytes of info in order to
// allow the VC debugger to stacktrace through stubs. This info
// is pushed right after the callee-saved-registers. The stubs
// also must keep ebp pointed to this structure. Note that this
// precludes the use of ebp by the stub itself.
//-----------------------------------------------------------------------
struct VC5Frame
{
    INT32      m_savedebp;
    INT32      m_returnaddress;
};
#define VC5FRAME_SIZE   sizeof(VC5Frame)
#else
#define VC5FRAME_SIZE   0
#endif



//**********************************************************************
// Exception handling
//**********************************************************************

inline LPVOID GetIP(CONTEXT *context) {
    return (LPVOID)(size_t)(context->Eip);
}

inline void SetIP(CONTEXT *context, LPVOID eip) {
    context->Eip = (UINT32)(size_t)eip;
}

inline LPVOID GetSP(CONTEXT *context) {
    return (LPVOID)(size_t)(context->Esp);
}

extern "C" LPVOID __stdcall GetSP();

inline void SetSP(CONTEXT *context, LPVOID esp) {
    context->Esp = (UINT32)(size_t)esp;
}

inline void SetFP(CONTEXT *context, LPVOID ebp) {
    context->Ebp = (UINT32)(size_t)ebp;
}
inline LPVOID GetFP(CONTEXT* context)
{
    return (LPVOID)(UINT_PTR)context->Ebp;
}

inline void emitStubCall(MethodDesc *pMD, BYTE *stubAddr) {
    StubCallInstrs *pInstrs = ((StubCallInstrs*)pMD) - 1;
    pInstrs->m_op = 0xe8; // CALL NEAR32
    pInstrs->m_target = (UINT32)(stubAddr - (BYTE*)pMD);
}


inline void emitCall(LPBYTE pBuffer, LPVOID target)
{
    pBuffer[0] = 0xe8; //CALLNEAR32
    *((LPVOID*)(1+pBuffer)) = (LPVOID) (((LPBYTE)target) - (pBuffer+5));
}

inline LPVOID getCallTarget(const BYTE *pCall)
{
    _ASSERTE(pCall[0] == 0xe8);
    return (LPVOID) (pCall + 5 + *((UINT32*)(1 + pCall)));
}

inline void emitJump(LPBYTE pBuffer, LPVOID target)
{
    pBuffer[0] = 0xe9; //JUMPNEAR32
    *((LPVOID*)(1+pBuffer)) = (LPVOID) (((LPBYTE)target) - (pBuffer+5));
}

inline void updateJumpTarget(LPBYTE pBuffer, LPVOID target)
{
    pBuffer[0] = 0xe9; //JUMPNEAR32
    InterlockedExchange((LONG*)(1+pBuffer), (DWORD) (((LPBYTE)target) - (pBuffer+5)));
}

inline LPVOID getJumpTarget(const BYTE *pJump)
{
    _ASSERTE(pJump[0] == 0xe9);
    return (LPVOID) (pJump + 5 + *((UINT32*)(1 + pJump)));
}

inline const BYTE* setCallAddrInterlocked(MethodDesc *pMD, const BYTE* stubAddr, 
                                     const BYTE* expectedStubAddr)
{
    StubCallInstrs *pInstrs = ((StubCallInstrs*)pMD) - 1;

    // the target of the call is relative against the address following 
    // the call instruction which happens to be (BYTE*)pMD

    PVOID result = (BYTE*)pMD + (size_t)
        FastInterlockCompareExchangePointer((void**)&pInstrs->m_target, 
                                            (void *)(stubAddr - (BYTE*)pMD), 
                                            (void *)(expectedStubAddr - (BYTE*)pMD));

    // result is the previous value of the stub
    return (const BYTE*)result;
}

void InterLockedShortcircuitPrestub(MethodDesc *pMD, const BYTE* codeAddr);

inline BOOL isShortcircuitedStub(MethodDesc *pMD)
{
    StubCallInstrs *pInstrs = ((StubCallInstrs*)pMD) - 1;
    return pInstrs->m_op != 0xe8; /* call */
}

inline const BYTE *getStubAddr(MethodDesc *pMD)
{    
    StubCallInstrs *pInstrs = ((StubCallInstrs*)pMD) - 1;

    // the target of the call is relative against the address following 
    // the call instruction which happens to be (BYTE*)pMD
    return (BYTE*)pMD + (size_t)pInstrs->m_target;
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
#ifdef _MSC_VER
	__asm rep nop		// This is the intel pause
#elif defined(__GNUC__)
    __asm__ __volatile__ (
	    "rep\n"
	    "nop"
    );
#else
#error Unsupported compiler
#endif
}

// Memory Barrier
inline void MemoryBarrier()
{
    // used by hash table lookup and insert methods
    // to make sure the key was fetched before the value
    // key and value fetch operations should not be reordered
    // also, load key should preceed, load value
    // and access volatile memory for X86
    static int memoryBarrier = 0;
    *(volatile int *)&memoryBarrier = 1;
}


inline int MLParmSize(int parmSize)
{
    return ((parmSize + sizeof(INT32) - 1) & ~((ULONG)(sizeof(INT32) - 1)));
}


EXTERN_C void __stdcall setFPReturn(int fpSize, INT64 retVal);
EXTERN_C void __stdcall getFPReturn(int fpSize, INT64 *pretval);

//----------------------------------------------------------------------
// Encodes X86 registers. The numbers are chosen to match Intel's opcode
// encoding.
//----------------------------------------------------------------------
enum X86Reg {
    kEAX = 0,
    kECX = 1,
    kEDX = 2,
    kEBX = 3,
    // kESP intentionally omitted because of its irregular treatment in MOD/RM
    kEBP = 5,
    kESI = 6,
    kEDI = 7

};




//----------------------------------------------------------------------
// Encodes X86 conditional jumps. The numbers are chosen to match
// Intel's opcode encoding.
//----------------------------------------------------------------------
class X86CondCode {
    public:
        enum cc {
            kJA   = 0x7,
            kJAE  = 0x3,
            kJB   = 0x2,
            kJBE  = 0x6,
            kJC   = 0x2,
            kJE   = 0x4,
            kJZ   = 0x4,
            kJG   = 0xf,
            kJGE  = 0xd,
            kJL   = 0xc,
            kJLE  = 0xe,
            kJNA  = 0x6,
            kJNAE = 0x2,
            kJNB  = 0x3,
            kJNBE = 0x7,
            kJNC  = 0x3,
            kJNE  = 0x5,
            kJNG  = 0xe,
            kJNGE = 0xc,
            kJNL  = 0xd,
            kJNLE = 0xf,
            kJNO  = 0x1,
            kJNP  = 0xb,
            kJNS  = 0x9,
            kJNZ  = 0x5,
            kJO   = 0x0,
            kJP   = 0xa,
            kJPE  = 0xa,
            kJPO  = 0xb,
            kJS   = 0x8,
        };
};


//----------------------------------------------------------------------
// StubLinker with extensions for generating X86 code.
//----------------------------------------------------------------------
class StubLinkerCPU : public StubLinker
{
    public:
        VOID X86EmitAddReg(X86Reg reg, INT32 imm32);
        VOID X86EmitSubReg(X86Reg reg, INT32 imm32);
        VOID X86EmitPushReg(X86Reg reg);
        VOID X86EmitPopReg(X86Reg reg);
        VOID X86EmitPushRegs(unsigned regSet);
        VOID X86EmitPopRegs(unsigned regSet);
        VOID X86EmitPushImm32(UINT value);
        VOID X86EmitPushImm32(CodeLabel &pTarget);
        VOID X86EmitPushImm8(BYTE value);
        VOID X86EmitZeroOutReg(X86Reg reg);
        VOID X86EmitNearJump(CodeLabel *pTarget);
        VOID X86EmitCondJump(CodeLabel *pTarget, X86CondCode::cc condcode);
        VOID X86EmitCall(CodeLabel *target, int iArgBytes, BOOL returnLabel = FALSE);
        VOID X86EmitReturn(int iArgBytes);
        VOID X86EmitCurrentThreadFetch();
        VOID X86EmitTLSFetch(DWORD idx, X86Reg dstreg, unsigned preservedRegSet);
        VOID X86EmitSetupThread();
        VOID X86EmitIndexRegLoad(X86Reg dstreg, X86Reg srcreg, __int32 ofs);
        VOID X86EmitIndexRegStore(X86Reg dstreg, __int32 ofs, X86Reg srcreg);
        VOID X86EmitIndexPush(X86Reg srcreg, __int32 ofs);
        VOID X86EmitSPIndexPush(__int8 ofs);
        VOID X86EmitIndexPop(X86Reg srcreg, __int32 ofs);
        VOID X86EmitSubEsp(INT32 imm32);
        VOID X86EmitAddEsp(INT32 imm32);
        VOID X86EmitOffsetModRM(BYTE opcode, X86Reg altreg, X86Reg indexreg, __int32 ofs);
        VOID X86EmitEspOffset(BYTE opcode, X86Reg altreg, __int32 ofs);

        // These are used to emit calls to notify the profiler of transitions in and out of
        // managed code through COM->COM+ interop or N/Direct
        VOID EmitProfilerComCallProlog(PVOID pFrameVptr, X86Reg regFrame);
        VOID EmitProfilerComCallEpilog(PVOID pFrameVptr, X86Reg regFrame);



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
        //    

        VOID X86EmitOp(BYTE    opcode,
                       X86Reg  altreg,
                       X86Reg  basereg,
                       __int32 ofs = 0,
                       X86Reg  scaledreg = (X86Reg)0,
                       BYTE    scale = 0
                       );


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

        VOID X86EmitR2ROp(BYTE opcode, X86Reg altreg, X86Reg modrmreg);



        VOID EmitEnable(CodeLabel *pForwardRef);
        VOID EmitRareEnable(CodeLabel *pRejoinPoint);

        VOID EmitDisable(CodeLabel *pForwardRef);
        VOID EmitRareDisable(CodeLabel *pRejoinPoint, BOOL bIsCallIn);
        VOID EmitRareDisableHRESULT(CodeLabel *pRejoinPoint, CodeLabel *pExitPoint);

        VOID X86EmitSetup(CodeLabel *pForwardRef);
        VOID EmitRareSetup(CodeLabel* pRejoinPoint);

        void EmitComMethodStubProlog(LPVOID pFrameVptr, CodeLabel** rgRareLabels,
                                     CodeLabel** rgRejoinLabels, LPVOID pSEHHandler,
                                     BOOL bShouldProfile);

        void EmitEnterManagedStubEpilog(LPVOID pFrameVptr, unsigned numStackBytes,
                    CodeLabel** rgRareLabels, CodeLabel** rgRejoinLabels,
                    BOOL bShouldProfile);

        void EmitComMethodStubEpilog(LPVOID pFrameVptr, unsigned numStackBytes,
                            CodeLabel** rgRareLabels, CodeLabel** rgRejoinLabels,
                            LPVOID pSEHHAndler, BOOL bShouldProfile);

        //========================================================================
        //  void EmitSEHProlog(LPVOID pvFrameHandler)
        //  Prolog for setting up SEH for stubs that enter managed code from unmanaged
        //  assumptions: esi has the current frame pointer
        void EmitSEHProlog(LPVOID pvFrameHandler);

        //===========================================================================
        //  void EmitUnLinkSEH(unsigned offset)
        //  negOffset is the offset from the current frame where the next exception record
        //  pointer is stored in the stack
        //  for e.g. COM to managed frames the pointer to next SEH record is in the stack
        //          after the ComMethodFrame::NegSpaceSize() + 4 ( address of handler)
        //
        //  also assumes ESI is pointing to the current frame
        void EmitUnLinkSEH(unsigned offset);

        VOID EmitMethodStubProlog(LPVOID pFrameVptr);
        VOID EmitMethodStubEpilog(__int16 numArgBytes, StubStyle style,
                                  __int16 shadowStackArgBytes = 0);

        VOID EmitUnboxMethodStub(MethodDesc* pRealMD);



        VOID EmitSecurityWrapperStub(UINT numStackBytes, MethodDesc* pMD, BOOL fToStub, LPVOID pRealStub, DeclActionInfo *pActions);
        VOID EmitSecurityInterceptorStub(MethodDesc* pMD, BOOL fToStub, LPVOID pRealStub, DeclActionInfo *pActions);

        //===========================================================================
        // Emits code for MulticastDelegate.Invoke()
        VOID EmitMulticastInvoke(UINT numStackBytes, BOOL fSingleCast);

        //===========================================================================
        // Emits code to adjust for a static delegate target.
        VOID EmitShuffleThunk(struct ShuffleEntry *pShuffeEntryArray);

        //===========================================================================
        // Emits code to capture the lasterror code.
        VOID EmitSaveLastError();


        //===========================================================================
        // Emits code to do an array operation.
        VOID EmitArrayOpStub(const ArrayOpScript*);

        //===========================================================================
        // Emits code to throw a rank exception
        VOID EmitRankExceptionThrowStub(UINT cbFixedArgs);

        //===========================================================================
        // Emits code to break into debugger
        VOID EmitDebugBreak();

        //===========================================================================
        // Emits code to touch pages
        // Inputs:
        //   eax = first byte of data
        //   edx = first byte past end of data
        //
        // Trashes eax, edx, ecx
        //
        // Pass TRUE if edx is guaranteed to be strictly greater than eax.
        VOID EmitPageTouch(BOOL fSkipNullCheck);


#ifdef _DEBUG
        VOID X86EmitDebugTrashReg(X86Reg reg);
#endif
    private:
        VOID X86EmitSubEspWorker(INT32 imm32);

    public:
        static BOOL Init();

};






#ifdef _DEBUG
//-------------------------------------------------------------------------
// This is a helper function that stubs in DEBUG go through to call
// outside code. This is only there to provide a code section return
// address because VC's stack tracing breaks otherwise.
//
// WARNING: Trashes ESI. This is not a C-callable function.
//-------------------------------------------------------------------------
extern "C" VOID __stdcall WrapCall(LPVOID pFunc);
#endif

// Adjust the generic interlocked operations for any platform specific ones we
// might have.
void InitFastInterlockOps();


// SEH info forward declarations

struct ComToManagedExRecord; // defined in cgenx86.cpp


inline BOOL IsUnmanagedValueTypeReturnedByRef(UINT sizeofvaluetype) 
{
    // odd-sized small structures are not 
    //  enregistered e.g. struct { char a,b,c; }
    return (sizeofvaluetype > 8) ||
        (sizeofvaluetype & (sizeofvaluetype-1)); // check that the size is power of two
}

inline BOOL IsManagedValueTypeReturnedByRef(UINT sizeofvaluetype) 
{
    return TRUE;
}

#include <pshpack1.h>
struct UMEntryThunkCode
{
    BYTE            m_alignpad[2];  // used to guarantee alignment of backpactched portion
    BYTE            m_movEAX;   //MOV EAX,imm32
    LPVOID          m_uet;      // pointer to start of this structure
    BYTE            m_jmp;      //JMP NEAR32
    const BYTE *    m_execstub; // pointer to destination code  // make sure the backpatched portion is dword aligned.

    void Encode(BYTE* pTargetCode, void* pvSecretParam);

    LPCBYTE GetEntryPoint() const
    {
        return (LPCBYTE)&m_movEAX;
    }

    static int GetEntryPointOffset()
    {
        return 2;
    }
    
};
#include <poppack.h>

#define X86_INSTR_CALL_REL32    0xE8        // call rel2
#define X86_INSTR_CALL_IND      0x15FF      // call dword ptr[addr32]
#define X86_INSTR_JMP_REL32     0xE9        // jmp rel32
#define X86_INSTR_JMP_IND       0x25FF      // jmp dword ptr[addr32]
#define X86_INSTR_JMP_EAX       0xE0FF      // jmp eax
#define X86_INSTR_MOV_EAX_IMM32 0xB8        // mov eax, imm32
#define X86_INSTR_MOV_EAX_IND   0x058B      // mov eax, dword ptr[addr32]

#define X86_INSTR_NOP           0x90        // nop
#define X86_INSTR_INT3          0xCC        // int 3
#define X86_INSTR_HLT           0xF4        // hlt

#endif // __cgenx86_h__
