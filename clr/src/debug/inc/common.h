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
#ifndef DEBUGGER_COMMON_H
#define DEBUGGER_COMMON_H

#ifdef _X86_
#include "i386/debugcpu.h"
#elif defined(_PPC_)
#include "ppc/debugcpu.h"
#else
#error Unsupported platform
#endif


#define NAME_EVENT_BUFFER   
#define NAME_EVENT(wczEventName) NULL


#define PTR_TO_CORDB_ADDRESS(_ptr) (CORDB_ADDRESS)(ULONG)(_ptr)
            
/* ------------------------------------------------------------------------- *
 * Constant declarations
 * ------------------------------------------------------------------------- */

enum
{
    NULL_THREAD_ID = -1,
    NULL_PROCESS_ID = -1
};

/* ------------------------------------------------------------------------- *
 * Macros
 * ------------------------------------------------------------------------- */
// Put this line in to detext these errors more easily
//    _ASSERTE( !"Null ptr where there shouldn't be!" );

#define VALIDATE_POINTER_TO_OBJECT(ptr, type)                                \
if ((ptr) == NULL)                                                           \
{                                                                            \
    return E_INVALIDARG;                                                     \
}

#define VALIDATE_POINTER_TO_OBJECT_OR_NULL(ptr, type)                        \
if ((ptr) == NULL)                                                           \
    goto LEndOfValidation##ptr;                                              \
VALIDATE_POINTER_TO_OBJECT((ptr), (type));                                   \
LEndOfValidation##ptr:

#define VALIDATE_POINTER_TO_OBJECT_ARRAY(ptr, type, cElt, fRead, fWrite)     \
if ((ptr) == NULL)                                                           \
{                                                                            \
    return E_INVALIDARG;                                                     \
}                                                                            \
if ((fRead) == true && IsBadReadPtr( (const void *)(ptr),                    \
    (cElt)*sizeof(type)))                                                    \
{                                                                            \
    return E_INVALIDARG;                                                     \
}                                                                            \
if ((fWrite) == true && IsBadWritePtr( (void *)(ptr),                        \
    (cElt)*sizeof(type)))                                                    \
{                                                                            \
    return E_INVALIDARG;                                                     \
}                                                                            

#define VALIDATE_POINTER_TO_OBJECT_ARRAY_OR_NULL(ptr, type,cElt,fRead,fWrite)\
if ((ptr)==NULL)                                                             \
{                                                                            \
    goto LEndOfValidation##ptr;                                              \
}                                                                            \
VALIDATE_POINTER_TO_OBJECT_ARRAY(ptr,type,cElt,fRead,fWrite);                \
LEndOfValidation##ptr:

/* ------------------------------------------------------------------------- *
 * Function Prototypes
 * ------------------------------------------------------------------------- */

// _skipFunkyModifiersInSignature will skip the modifiers that
// we don't care about.  Everything we care about is listed as
// a case in CreateValueByType. 
ULONG _skipFunkyModifiersInSignature(PCCOR_SIGNATURE sig);

// Skips past the calling convention, argument count (saving that into
// *pCount), then moves past the return type.
ULONG _skipMethodSignatureHeader(PCCOR_SIGNATURE sig, ULONG *pCount);

// _skipTypeInSignature -- skip past a type in a given signature.
// Returns the number of bytes used by the type in the signature.
//
ULONG _skipTypeInSignature(PCCOR_SIGNATURE sig,
                           bool *pfPassedVarArgSentinel = NULL);

// Returns 0 if the next thing in the signature ISN'T a VarArgs sentinel;
// returns the size (in bytes) of the VA sentinel otherwise.
ULONG _detectAndSkipVASentinel(PCCOR_SIGNATURE sig);

// Return the size of the next element in the method.  U1, I1 = 1 byte,etc.
//
// If the type is a value class, we'll assign the token of the class to
// *pmdValueClass, if pmdValueClass is nonNULL.  The caller can then
// invoke something like EEClass::GetAlignedNumInstanceFieldBytes(), or
// CordbClass::GetObjectSize() to get the actual size.  The return value
// will be zero, in this case.  If the type isn't a value class, then
// pmdValueClass is mdTokenNil after _sizeOfElementInstance returns.
ULONG _sizeOfElementInstance(PCCOR_SIGNATURE sig,
                             mdTypeDef *pmdValueClass = NULL);

void _CopyThreadContext(CONTEXT *c1, CONTEXT *c2);

// We only want to use ReadProcessMemory if we're truely out of process
#ifdef RIGHT_SIDE_ONLY

#define ReadProcessMemoryI(hProcess,lpBaseAddress,lpBuffer,nSize,lpNumberOfBytesRead) \
    ReadProcessMemory(hProcess,lpBaseAddress,lpBuffer,nSize,(SIZE_T*)lpNumberOfBytesRead) 

#else //in-proc

BOOL inline ReadProcessMemoryI(
  HANDLE hProcess,  // handle to the process whose memory is read
  LPCVOID lpBaseAddress,
                    // address to start reading
  LPVOID lpBuffer,  // address of buffer to place read data
  DWORD nSize,      // number of bytes to read
  LPDWORD lpNumberOfBytesRead 
                    // address of number of bytes read
)
{
    SIZE_T  cbBytesRead;
    BOOL    bRetVal;

    bRetVal = ReadProcessMemory(GetCurrentProcess(), lpBaseAddress, lpBuffer, nSize, &cbBytesRead);

    if (NULL != lpNumberOfBytesRead)
    {
        *lpNumberOfBytesRead = (DWORD)cbBytesRead;
    }
    return bRetVal;
}

#endif //RIGHT_SIDE_ONLY

// Linear search through an array of NativeVarInfos, to find
// the variable of index dwIndex, valid at the given ip.
//
// returns CORDBG_E_IL_VAR_NOT_AVAILABLE if the variable isn't
// valid at the given ip.
//
// This should be inlined
HRESULT FindNativeInfoInILVariableArray(DWORD dwIndex,
                                        SIZE_T ip,
                                        ICorJitInfo::NativeVarInfo **ppNativeInfo,
                                        unsigned int nativeInfoCount,
                                        ICorJitInfo::NativeVarInfo *nativeInfo);


#define VALIDATE_HEAP
//HeapValidate(GetProcessHeap(), 0, NULL);

// This contains enough information to determine what went wrong with
// an edit and continue.  One info per error reported.
#define ENCERRORINFO_MAX_STRING_SIZE (60)
typedef struct tagEnCErrorInfo
{
	HRESULT 	m_hr;
	void 	   *m_module; // In the left side, this is a pointer to a
					 // DebuggerModule, which then is used to look
					 // up the CordbModule on the right side.
	void       *m_appDomain; // The domain to which the changes are
					// being applied.
	mdToken		m_token; // Metadata token that the error pertains to.
	WCHAR		m_sz[ENCERRORINFO_MAX_STRING_SIZE];
					// in-line for ease-of-transfer across processes.
	
} EnCErrorInfo;

#define ADD_ENC_ERROR_ENTRY(pEnCError, hr, module, token)					\
	(pEnCError)->m_hr = (hr);												\
	(pEnCError)->m_module = (module);										\
	(pEnCError)->m_token = (token);												

typedef CUnorderedArray<EnCErrorInfo, 11> UnorderedEnCErrorInfoArray;

typedef struct tagEnCRemapInfo
{
    BOOL            m_fAccurate;
    void           *m_debuggerModuleToken; //LS pointer to DebuggerModule
    // Carry along enough info to instantiate, if we have to.
    mdMethodDef     m_funcMetadataToken ;
    mdToken         m_localSigToken;
    ULONG           m_RVA;
    DWORD           m_threadId;
    void           *m_pAppDomainToken;
} EnCRemapInfo;

typedef CUnorderedArray<EnCRemapInfo, 31> UnorderedEnCRemapArray;


#ifndef RIGHT_SIDE_ONLY

// This can be called from anywhere.
#define CHECK_INPROC_PROCESS_STATE() (g_pGCHeap->IsGCInProgress() && g_profControlBlock.fIsSuspended)

// To be used only from a CordbThread object.
#define CHECK_INPROC_THREAD_STATE() (CHECK_INPROC_PROCESS_STATE() || m_fThreadInprocIsActive)

#endif

//  struct DebuggerILToNativeMap:   Holds the IL to Native offset map
//	Great pains are taken to ensure that this each entry corresponds to the
//	first IL instruction in a source line.  It isn't actually a mapping
//	of _every_ IL instruction in a method, just those for source lines.
//  SIZE_T ilOffset:  IL offset of a source line.
//  SIZE_T nativeStartOffset:  Offset within the method where the native
//		instructions corresponding to the IL offset begin.
//  SIZE_T nativeEndOffset:  Offset within the method where the native
//		instructions corresponding to the IL offset end.
//
//  Note: any changes to this struct need to be reflected in
//  COR_DEBUG_IL_TO_NATIVE_MAP in CorDebug.idl. These structs must
//  match exactly.
//
struct DebuggerILToNativeMap
{
    ULONG ilOffset;
    ULONG nativeStartOffset;
    ULONG nativeEndOffset;
    ICorDebugInfo::SourceTypes source;
};

void ExportILToNativeMap(ULONG32 cMap,             
             COR_DEBUG_IL_TO_NATIVE_MAP mapExt[],  
             struct DebuggerILToNativeMap mapInt[],
             SIZE_T sizeOfCode);

//
// Routines used by debugger support functions such as codepatch.cpp or
// exception handling code.
//
// GetInstruction, InsertBreakpoint, and SetInstruction all operate on
// a _single_ byte of memory on _x86_. This is really important. If you only
// save one byte from the instruction stream before placing a breakpoint,
// you need to make sure to only replace one byte later on.
//

inline DWORD CORDbgGetInstruction(const BYTE* address)
{
    return *(INSTRUCTION_TYPE *)address; // retrieving only one byte is important
}


inline void CORDbgInsertBreakpoint(const BYTE* address)
{
    *((INSTRUCTION_TYPE *)address) = CORDbg_BREAK_INSTRUCTION;

    FlushInstructionCache(GetCurrentProcess(),
                          address,
                          sizeof(INSTRUCTION_TYPE));

}

inline void CORDbgSetInstruction(const unsigned char* address,
                                 DWORD instruction)
{
    *((INSTRUCTION_TYPE *)address) = (INSTRUCTION_TYPE)instruction;

    FlushInstructionCache(GetCurrentProcess(),
                          address,
                          sizeof(INSTRUCTION_TYPE));

}


inline bool CORDbgIsPatchedAddress(const BYTE *address)
{
        return *(INSTRUCTION_TYPE *)address == CORDbg_BREAK_INSTRUCTION;
}

#endif //DEBUGGER_COMMON_H
