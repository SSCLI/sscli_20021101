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

// This file contains an implementation of the NTSD dbgeng.dll interfaces,
// used to host sos.dll in-proc inside a Rotor process, so it can be
// called from gdb.

#ifdef PLATFORM_UNIX    // entire file

#include <palclr.h>
#include "gdbwrap.h"
#include <wdbgexts.h>

#define INITGUID
#include "guiddef.h"
#include "dbgeng.h"

typedef HRESULT (*ExtensionApi)(PDEBUG_CLIENT Client, PCSTR args);

typedef struct _RegNameList {
    const char *Name;
    size_t Offset;
    } REGNAMELIST;

#ifdef _X86_
const REGNAMELIST RegNameList[] = {
    { "eax", FIELD_OFFSET(CONTEXT, Eax) },
    { "ebx", FIELD_OFFSET(CONTEXT, Ebx) },
    { "ecx", FIELD_OFFSET(CONTEXT, Ecx) },
    { "edx", FIELD_OFFSET(CONTEXT, Edx) },
    { "esi", FIELD_OFFSET(CONTEXT, Esi) },
    { "edi", FIELD_OFFSET(CONTEXT, Edi) },
    { "esp", FIELD_OFFSET(CONTEXT, Esp) },
    { "eip", FIELD_OFFSET(CONTEXT, Eip) },
    { "ebp", FIELD_OFFSET(CONTEXT, Ebp) }
};
#elif defined(_PPC_)
const REGNAMELIST RegNameList[] = {
    { "r0", FIELD_OFFSET(CONTEXT, Gpr0) },
    { "r1", FIELD_OFFSET(CONTEXT, Gpr1) },
    { "r2", FIELD_OFFSET(CONTEXT, Gpr2) },
    { "r3", FIELD_OFFSET(CONTEXT, Gpr3) },
    { "r4", FIELD_OFFSET(CONTEXT, Gpr4) },
    { "r5", FIELD_OFFSET(CONTEXT, Gpr5) },
    { "r6", FIELD_OFFSET(CONTEXT, Gpr6) },
    { "r7", FIELD_OFFSET(CONTEXT, Gpr7) },
    { "r8", FIELD_OFFSET(CONTEXT, Gpr8) },
    { "r9", FIELD_OFFSET(CONTEXT, Gpr9) },
    { "r10", FIELD_OFFSET(CONTEXT, Gpr10) },
    { "r11", FIELD_OFFSET(CONTEXT, Gpr11) },
    { "r12", FIELD_OFFSET(CONTEXT, Gpr12) },
    { "r13", FIELD_OFFSET(CONTEXT, Gpr13) },
    { "r14", FIELD_OFFSET(CONTEXT, Gpr14) },
    { "r15", FIELD_OFFSET(CONTEXT, Gpr15) },
    { "r16", FIELD_OFFSET(CONTEXT, Gpr16) },
    { "r17", FIELD_OFFSET(CONTEXT, Gpr17) },
    { "r18", FIELD_OFFSET(CONTEXT, Gpr18) },
    { "r19", FIELD_OFFSET(CONTEXT, Gpr19) },
    { "r20", FIELD_OFFSET(CONTEXT, Gpr20) },
    { "r21", FIELD_OFFSET(CONTEXT, Gpr21) },
    { "r22", FIELD_OFFSET(CONTEXT, Gpr22) },
    { "r23", FIELD_OFFSET(CONTEXT, Gpr23) },
    { "r24", FIELD_OFFSET(CONTEXT, Gpr24) },
    { "r25", FIELD_OFFSET(CONTEXT, Gpr25) },
    { "r26", FIELD_OFFSET(CONTEXT, Gpr26) },
    { "r27", FIELD_OFFSET(CONTEXT, Gpr27) },
    { "r28", FIELD_OFFSET(CONTEXT, Gpr28) },
    { "r29", FIELD_OFFSET(CONTEXT, Gpr29) },
    { "r30", FIELD_OFFSET(CONTEXT, Gpr30) },
    { "r31", FIELD_OFFSET(CONTEXT, Gpr31) },
};
#else
PORTABILITY_WARNING("Register offsets are not specified for this platform.")
const REGNAMELIST RegNameList[] = {
	{ "reg", 0 }
};
#endif


ULONG64 WDBGAPI GetExpressionRoutine(PCSTR lpExpression)
{
    char *EndPtr;
    unsigned long Val;
    // We assume all expressions are hex numbers, just like on windows
    Val = strtoul(lpExpression, &EndPtr, 16);
    return Val;
}

#define MAX_FORMAT_LENGTH 256

void vOutputRoutine(PCSTR lpFormat, va_list ap)
{
    char FormatCopy[MAX_FORMAT_LENGTH+1];
    int i;
    char *Copy;

    // Replace all %p by %08I64x
    i = 0;
    Copy = FormatCopy;
    while (*lpFormat && i < MAX_FORMAT_LENGTH) {
        if (*lpFormat == '%' && tolower(*(lpFormat+1))=='p') {
            if (i+7 >= MAX_FORMAT_LENGTH) {
                break;
            }
            Copy[i++] = '%';
            Copy[i++] = '0';
            Copy[i++] = '8';
            Copy[i++] = 'I';
            Copy[i++] = '6';
            Copy[i++] = '4';
            Copy[i++] = 'x';
            lpFormat++;
        } else {
            Copy[i++] = *lpFormat;
        }
        lpFormat++;
    }
    Copy[i] = '\0';
    vfprintf(stderr, FormatCopy, ap);

}

void WDBGAPIV OutputRoutine(PCSTR lpFormat, ...)
{
    va_list ap;

    va_start(ap, lpFormat);
    vOutputRoutine(lpFormat, ap);
    va_end(ap);
}



class CDebugAdvanced : public IDebugAdvanced {
public:
    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        )
        {
            if (IsEqualGUID(InterfaceId, IID_IUnknown)) {
                AddRef();
                *Interface = (PVOID)this;
            } else if (IsEqualGUID(InterfaceId, IID_IDebugAdvanced)) {
                AddRef();
                *Interface = (PVOID)this;
            } else {
                fprintf(stderr, "Error: CDebugAdvanced::QueryInterface called "
                        "with unknown IID at address %x\n", &InterfaceId);
                return E_NOINTERFACE;
            }
            return S_OK;
        }
    STDMETHOD_(ULONG, AddRef)(
        THIS
        ) 
        { return InterlockedIncrement(&m_RefCount); }
    STDMETHOD_(ULONG, Release)(
        THIS
        ) 
        { ULONG c = InterlockedDecrement(&m_RefCount);
            if (c == 0) {
                delete this;
            }
            return c;
        }

    // IDebugAdvanced.
    STDMETHOD(GetThreadContext)(
        THIS_
        OUT /* align_is(16) */ PVOID Context,
        IN ULONG ContextSize
        )
        {
            if (::GetThreadContext(GetCurrentThread(),
                                   (PCONTEXT)Context)) {
                return S_OK;
            } else {
                return E_FAIL;
            }
        }
    STDMETHOD(SetThreadContext)(
        THIS_
        IN /* align_is(16) */ PVOID Context,
        IN ULONG ContextSize
        ) { return E_NOTIMPL;}

    CDebugAdvanced() { m_RefCount = 1;}
    virtual ~CDebugAdvanced() { }
private:
    LONG m_RefCount;
};


class CDebugControl: public IDebugControl {
public:
    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        )
        {
            if (IsEqualGUID(InterfaceId, IID_IUnknown)) {
                AddRef();
                *Interface = (PVOID)this;
            } else if (IsEqualGUID(InterfaceId, IID_IDebugAdvanced)) {
                AddRef();
                *Interface = (PVOID)this;
            } else {
                fprintf(stderr, "Error: CDebugControl::QueryInterface called "
                        "with unknown IID at address %x\n", &InterfaceId);
                return E_NOINTERFACE;
            }
            return S_OK;
        }
    STDMETHOD_(ULONG, AddRef)(
        THIS
        ) 
        { return InterlockedIncrement(&m_RefCount); }
    STDMETHOD_(ULONG, Release)(
        THIS
        ) 
        { ULONG c = InterlockedDecrement(&m_RefCount);
            if (c == 0) {
                delete this;
            }
            return c;
        }

    // IDebugControl.
    
    STDMETHOD(GetInterrupt)(
        THIS
        ) { return E_NOTIMPL; }
    STDMETHOD(SetInterrupt)(
        THIS_
        IN ULONG Flags
        ) { return E_NOTIMPL; }
    STDMETHOD(GetInterruptTimeout)(
        THIS_
        OUT PULONG Seconds
        ) { return E_NOTIMPL; }
    STDMETHOD(SetInterruptTimeout)(
        THIS_
        IN ULONG Seconds
        ) { return E_NOTIMPL; }
    STDMETHOD(GetLogFile)(
        THIS_
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG FileSize,
        OUT PBOOL Append
        ) { return E_NOTIMPL; }
    STDMETHOD(OpenLogFile)(
        THIS_
        IN PCSTR File,
        IN BOOL Append
        ) { return E_NOTIMPL; }
    STDMETHOD(CloseLogFile)(
        THIS
        ) { return E_NOTIMPL; }
    STDMETHOD(GetLogMask)(
        THIS_
        OUT PULONG Mask
        ) { return E_NOTIMPL; }
    STDMETHOD(SetLogMask)(
        THIS_
        IN ULONG Mask
        ) { return E_NOTIMPL; }
    STDMETHOD(Input)(
        THIS_
        OUT PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG InputSize
        ) { return E_NOTIMPL; }
    STDMETHOD(ReturnInput)(
        THIS_
        IN PCSTR Buffer
        ) { return E_NOTIMPL; }
    STDMETHODV(Output)(
        THIS_
        IN ULONG Mask,
        IN PCSTR Format,
        ...
        ) { return E_NOTIMPL; }
    STDMETHOD(OutputVaList)(
        THIS_
        IN ULONG Mask,
        IN PCSTR Format,
        IN va_list Args
        ) 
        { 
            vOutputRoutine(Format, Args);
            return S_OK;
        }
    STDMETHODV(ControlledOutput)(
        THIS_
        IN ULONG OutputControl,
        IN ULONG Mask,
        IN PCSTR Format,
        ...
        ) { return E_NOTIMPL; }
    STDMETHOD(ControlledOutputVaList)(
        THIS_
        IN ULONG OutputControl,
        IN ULONG Mask,
        IN PCSTR Format,
        IN va_list Args
        ) { return E_NOTIMPL; }
    STDMETHODV(OutputPrompt)(
        THIS_
        IN ULONG OutputControl,
        IN OPTIONAL PCSTR Format,
        ...
        ) { return E_NOTIMPL; }
    STDMETHOD(OutputPromptVaList)(
        THIS_
        IN ULONG OutputControl,
        IN OPTIONAL PCSTR Format,
        IN va_list Args
        ) { return E_NOTIMPL; }
    STDMETHOD(GetPromptText)(
        THIS_
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG TextSize
        ) { return E_NOTIMPL; }
    STDMETHOD(OutputCurrentState)(
        THIS_
        IN ULONG OutputControl,
        IN ULONG Flags
        ) { return E_NOTIMPL; }
    STDMETHOD(OutputVersionInformation)(
        THIS_
        IN ULONG OutputControl
        ) { return E_NOTIMPL; }
    STDMETHOD(GetNotifyEventHandle)(
        THIS_
        OUT PULONG64 Handle
        ) { return E_NOTIMPL; }
    STDMETHOD(SetNotifyEventHandle)(
        THIS_
        IN ULONG64 Handle
        ) { return E_NOTIMPL; }
    STDMETHOD(Assemble)(
        THIS_
        IN ULONG64 Offset,
        IN PCSTR Instr,
        OUT PULONG64 EndOffset
        ) { return E_NOTIMPL; }
    STDMETHOD(Disassemble)(
        THIS_
        IN ULONG64 Offset,
        IN ULONG Flags,
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG DisassemblySize,
        OUT PULONG64 EndOffset
        ) { return E_NOTIMPL; }
    STDMETHOD(GetDisassembleEffectiveOffset)(
        THIS_
        OUT PULONG64 Offset
        ) { return E_NOTIMPL; }
    STDMETHOD(OutputDisassembly)(
        THIS_
        IN ULONG OutputControl,
        IN ULONG64 Offset,
        IN ULONG Flags,
        OUT PULONG64 EndOffset
        ) { return E_NOTIMPL; }
    STDMETHOD(OutputDisassemblyLines)(
        THIS_
        IN ULONG OutputControl,
        IN ULONG PreviousLines,
        IN ULONG TotalLines,
        IN ULONG64 Offset,
        IN ULONG Flags,
        OUT OPTIONAL PULONG OffsetLine,
        OUT OPTIONAL PULONG64 StartOffset,
        OUT OPTIONAL PULONG64 EndOffset,
        OUT OPTIONAL /* size_is(TotalLines) */ PULONG64 LineOffsets
        ) { return E_NOTIMPL; }
    STDMETHOD(GetNearInstruction)(
        THIS_
        IN ULONG64 Offset,
        IN LONG Delta,
        OUT PULONG64 NearOffset
        ) { return E_NOTIMPL; }
    STDMETHOD(GetStackTrace)(
        THIS_
        IN ULONG64 FrameOffset,
        IN ULONG64 StackOffset,
        IN ULONG64 InstructionOffset,
        OUT /* size_is(FramesSize) */ PDEBUG_STACK_FRAME Frames,
        IN ULONG FramesSize,
        OUT OPTIONAL PULONG FramesFilled
        ) { return E_NOTIMPL; }
    STDMETHOD(GetReturnOffset)(
        THIS_
        OUT PULONG64 Offset
        ) { return E_NOTIMPL; }
    STDMETHOD(OutputStackTrace)(
        THIS_
        IN ULONG OutputControl,
        IN OPTIONAL /* size_is(FramesSize) */ PDEBUG_STACK_FRAME Frames,
        IN ULONG FramesSize,
        IN ULONG Flags
        ) { return E_NOTIMPL; }
    STDMETHOD(GetDebuggeeType)(
        THIS_
        OUT PULONG Class,
        OUT PULONG Qualifier
        )
        {
            *Class = DEBUG_CLASS_USER_WINDOWS;
            *Qualifier = 0; // not a .dmp file
            return S_OK;
        }
    STDMETHOD(GetActualProcessorType)(
        THIS_
        OUT PULONG Type
        ) { return E_NOTIMPL; }
    STDMETHOD(GetExecutingProcessorType)(
        THIS_
        OUT PULONG Type
        ) { return E_NOTIMPL; }
    STDMETHOD(GetNumberPossibleExecutingProcessorTypes)(
        THIS_
        OUT PULONG Number
        ) { return E_NOTIMPL; }
    STDMETHOD(GetPossibleExecutingProcessorTypes)(
        THIS_
        IN ULONG Start,
        IN ULONG Count,
        OUT /* size_is(Count) */ PULONG Types
        ) { return E_NOTIMPL; }
    STDMETHOD(GetNumberProcessors)(
        THIS_
        OUT PULONG Number
        ) { return E_NOTIMPL; }
    STDMETHOD(GetSystemVersion)(
        THIS_
        OUT PULONG PlatformId,
        OUT PULONG Major,
        OUT PULONG Minor,
        OUT OPTIONAL PSTR ServicePackString,
        IN ULONG ServicePackStringSize,
        OUT OPTIONAL PULONG ServicePackStringUsed,
        OUT PULONG ServicePackNumber,
        OUT OPTIONAL PSTR BuildString,
        IN ULONG BuildStringSize,
        OUT OPTIONAL PULONG BuildStringUsed
        )
        {
            *PlatformId = VER_PLATFORM_UNIX;
            *Major = 1;
            *Minor = 0;
            *ServicePackNumber = 0;
            return S_OK;
        }
    STDMETHOD(GetPageSize)(
        THIS_
        OUT PULONG Size
        ) 
        { 
            SYSTEM_INFO si;

            GetSystemInfo(&si);
            *Size = si.dwPageSize; 
            return S_OK;
        }
    STDMETHOD(IsPointer64Bit)(
        THIS
        ) { return E_NOTIMPL; }
    STDMETHOD(ReadBugCheckData)(
        THIS_
        OUT PULONG Code,
        OUT PULONG64 Arg1,
        OUT PULONG64 Arg2,
        OUT PULONG64 Arg3,
        OUT PULONG64 Arg4
        ) { return E_NOTIMPL; }
    STDMETHOD(GetNumberSupportedProcessorTypes)(
        THIS_
        OUT PULONG Number
        ) { return E_NOTIMPL; }
    STDMETHOD(GetSupportedProcessorTypes)(
        THIS_
        IN ULONG Start,
        IN ULONG Count,
        OUT /* size_is(Count) */ PULONG Types
        ) { return E_NOTIMPL; }
    STDMETHOD(GetProcessorTypeNames)(
        THIS_
        IN ULONG Type,
        OUT OPTIONAL PSTR FullNameBuffer,
        IN ULONG FullNameBufferSize,
        OUT OPTIONAL PULONG FullNameSize,
        OUT OPTIONAL PSTR AbbrevNameBuffer,
        IN ULONG AbbrevNameBufferSize,
        OUT OPTIONAL PULONG AbbrevNameSize
        ) { return E_NOTIMPL; }
    STDMETHOD(GetEffectiveProcessorType)(
        THIS_
        OUT PULONG Type
        ) { return E_NOTIMPL; }
    STDMETHOD(SetEffectiveProcessorType)(
        THIS_
        IN ULONG Type
        ) { return E_NOTIMPL; }
    STDMETHOD(GetExecutionStatus)(
        THIS_
        OUT PULONG Status
        ) { return E_NOTIMPL; }
    STDMETHOD(SetExecutionStatus)(
        THIS_
        IN ULONG Status
        ) { return E_NOTIMPL; }
    STDMETHOD(GetCodeLevel)(
        THIS_
        OUT PULONG Level
        ) { return E_NOTIMPL; }
    STDMETHOD(SetCodeLevel)(
        THIS_
        IN ULONG Level
        ) { return E_NOTIMPL; }
    STDMETHOD(GetEngineOptions)(
        THIS_
        OUT PULONG Options
        ) { return E_NOTIMPL; }
    STDMETHOD(AddEngineOptions)(
        THIS_
        IN ULONG Options
        ) { return E_NOTIMPL; }
    STDMETHOD(RemoveEngineOptions)(
        THIS_
        IN ULONG Options
        ) { return E_NOTIMPL; }
    STDMETHOD(SetEngineOptions)(
        THIS_
        IN ULONG Options
        ) { return E_NOTIMPL; }
    STDMETHOD(GetSystemErrorControl)(
        THIS_
        OUT PULONG OutputLevel,
        OUT PULONG BreakLevel
        ) { return E_NOTIMPL; }
    STDMETHOD(SetSystemErrorControl)(
        THIS_
        IN ULONG OutputLevel,
        IN ULONG BreakLevel
        ) { return E_NOTIMPL; }
    STDMETHOD(GetTextMacro)(
        THIS_
        IN ULONG Slot,
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG MacroSize
        ) { return E_NOTIMPL; }
    STDMETHOD(SetTextMacro)(
        THIS_
        IN ULONG Slot,
        IN PCSTR Macro
        ) { return E_NOTIMPL; }
    STDMETHOD(GetRadix)(
        THIS_
        OUT PULONG Radix
        ) { return E_NOTIMPL; }
    STDMETHOD(SetRadix)(
        THIS_
        IN ULONG Radix
        ) { return E_NOTIMPL; }
    STDMETHOD(Evaluate)(
        THIS_
        IN PCSTR Expression,
        IN ULONG DesiredType,
        OUT PDEBUG_VALUE Value,
        OUT OPTIONAL PULONG RemainderIndex
        ) { return E_NOTIMPL; }
    STDMETHOD(CoerceValue)(
        THIS_
        IN PDEBUG_VALUE In,
        IN ULONG OutType,
        OUT PDEBUG_VALUE Out
        ) { return E_NOTIMPL; }
    STDMETHOD(CoerceValues)(
        THIS_
        IN ULONG Count,
        IN /* size_is(Count) */ PDEBUG_VALUE In,
        IN /* size_is(Count) */ PULONG OutTypes,
        OUT /* size_is(Count) */ PDEBUG_VALUE Out
        ) { return E_NOTIMPL; }
    STDMETHOD(Execute)(
        THIS_
        IN ULONG OutputControl,
        IN PCSTR Command,
        IN ULONG Flags
        ) { return E_NOTIMPL; }
    STDMETHOD(ExecuteCommandFile)(
        THIS_
        IN ULONG OutputControl,
        IN PCSTR CommandFile,
        IN ULONG Flags
        ) { return E_NOTIMPL; }
    STDMETHOD(GetNumberBreakpoints)(
        THIS_
        OUT PULONG Number
        ) { return E_NOTIMPL; }
    STDMETHOD(GetBreakpointByIndex)(
        THIS_
        IN ULONG Index,
        OUT PDEBUG_BREAKPOINT* Bp
        ) { return E_NOTIMPL; }
    STDMETHOD(GetBreakpointById)(
        THIS_
        IN ULONG Id,
        OUT PDEBUG_BREAKPOINT* Bp
        ) { return E_NOTIMPL; }
    STDMETHOD(GetBreakpointParameters)(
        THIS_
        IN ULONG Count,
        IN OPTIONAL /* size_is(Count) */ PULONG Ids,
        IN ULONG Start,
        OUT /* size_is(Count) */ PDEBUG_BREAKPOINT_PARAMETERS Params
        ) { return E_NOTIMPL; }
    STDMETHOD(AddBreakpoint)(
        THIS_
        IN ULONG Type,
        IN ULONG DesiredId,
        OUT PDEBUG_BREAKPOINT* Bp
        ) { return E_NOTIMPL; }
    STDMETHOD(RemoveBreakpoint)(
        THIS_
        IN PDEBUG_BREAKPOINT Bp
        ) { return E_NOTIMPL; }
    STDMETHOD(AddExtension)(
        THIS_
        IN PCSTR Path,
        IN ULONG Flags,
        OUT PULONG64 Handle
        ) { return E_NOTIMPL; }
    STDMETHOD(RemoveExtension)(
        THIS_
        IN ULONG64 Handle
        ) { return E_NOTIMPL; }
    STDMETHOD(GetExtensionByPath)(
        THIS_
        IN PCSTR Path,
        OUT PULONG64 Handle
        ) { return E_NOTIMPL; }
    STDMETHOD(CallExtension)(
        THIS_
        IN ULONG64 Handle,
        IN PCSTR Function,
        IN OPTIONAL PCSTR Arguments
        ) { return E_NOTIMPL; }
    STDMETHOD(GetExtensionFunction)(
        THIS_
        IN ULONG64 Handle,
        IN PCSTR FuncName,
        OUT FARPROC* Function
        ) { return E_NOTIMPL; }
    STDMETHOD(GetWindbgExtensionApis32)(
        THIS_
        IN OUT PWINDBG_EXTENSION_APIS32 Api
        ) { return E_NOTIMPL; }
    STDMETHOD(GetWindbgExtensionApis64)(
        THIS_
        IN OUT PWINDBG_EXTENSION_APIS64 Api
        )
        {
            memset(Api, 0, sizeof(*Api));
            Api->lpOutputRoutine = OutputRoutine;
            Api->lpGetExpressionRoutine = GetExpressionRoutine;
            return S_OK;
        }
    STDMETHOD(GetNumberEventFilters)(
        THIS_
        OUT PULONG SpecificEvents,
        OUT PULONG SpecificExceptions,
        OUT PULONG ArbitraryExceptions
        ) { return E_NOTIMPL; }
    STDMETHOD(GetEventFilterText)(
        THIS_
        IN ULONG Index,
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG TextSize
        ) { return E_NOTIMPL; }
    STDMETHOD(GetEventFilterCommand)(
        THIS_
        IN ULONG Index,
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG CommandSize
        ) { return E_NOTIMPL; }
    STDMETHOD(SetEventFilterCommand)(
        THIS_
        IN ULONG Index,
        IN PCSTR Command
        ) { return E_NOTIMPL; }
    STDMETHOD(GetSpecificFilterParameters)(
        THIS_
        IN ULONG Start,
        IN ULONG Count,
        OUT /* size_is(Count) */ PDEBUG_SPECIFIC_FILTER_PARAMETERS Params
        ) { return E_NOTIMPL; }
    STDMETHOD(SetSpecificFilterParameters)(
        THIS_
        IN ULONG Start,
        IN ULONG Count,
        IN /* size_is(Count) */ PDEBUG_SPECIFIC_FILTER_PARAMETERS Params
        ) { return E_NOTIMPL; }
    STDMETHOD(GetSpecificFilterArgument)(
        THIS_
        IN ULONG Index,
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG ArgumentSize
        ) { return E_NOTIMPL; }
    STDMETHOD(SetSpecificFilterArgument)(
        THIS_
        IN ULONG Index,
        IN PCSTR Argument
        ) { return E_NOTIMPL; }
    STDMETHOD(GetExceptionFilterParameters)(
        THIS_
        IN ULONG Count,
        IN OPTIONAL /* size_is(Count) */ PULONG Codes,
        IN ULONG Start,
        OUT /* size_is(Count) */ PDEBUG_EXCEPTION_FILTER_PARAMETERS Params
        ) { return E_NOTIMPL; }
    STDMETHOD(SetExceptionFilterParameters)(
        THIS_
        IN ULONG Count,
        IN /* size_is(Count) */ PDEBUG_EXCEPTION_FILTER_PARAMETERS Params
        ) { return E_NOTIMPL; }
    STDMETHOD(GetExceptionFilterSecondCommand)(
        THIS_
        IN ULONG Index,
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG CommandSize
        ) { return E_NOTIMPL; }
    STDMETHOD(SetExceptionFilterSecondCommand)(
        THIS_
        IN ULONG Index,
        IN PCSTR Command
        ) { return E_NOTIMPL; }
    STDMETHOD(WaitForEvent)(
        THIS_
        IN ULONG Flags,
        IN ULONG Timeout
        ) { return E_NOTIMPL; }
    STDMETHOD(GetLastEventInformation)(
        THIS_
        OUT PULONG Type,
        OUT PULONG ProcessId,
        OUT PULONG ThreadId,
        OUT OPTIONAL PVOID ExtraInformation,
        IN ULONG ExtraInformationSize,
        OUT OPTIONAL PULONG ExtraInformationUsed,
        OUT OPTIONAL PSTR Description,
        IN ULONG DescriptionSize,
        OUT OPTIONAL PULONG DescriptionUsed
        ) { return E_NOTIMPL; }

    CDebugControl() { m_RefCount = 1;}
    virtual ~CDebugControl() { }

 private:
    LONG m_RefCount;
};


class CDebugDataSpaces: public IDebugDataSpaces {
public:
    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        )
        {
            if (IsEqualGUID(InterfaceId, IID_IUnknown)) {
                AddRef();
                *Interface = (PVOID)this;
            } else if (IsEqualGUID(InterfaceId, IID_IDebugDataSpaces)) {
                AddRef();
                *Interface = (PVOID)this;
            } else {
                fprintf(stderr, 
                        "Error: CDebugDataSpaces::QueryInterface called "
                        "with unknown IID at address %x\n", &InterfaceId);
                return E_NOINTERFACE;
            }
            return S_OK;
        }
    STDMETHOD_(ULONG, AddRef)(
        THIS
        ) 
        { return InterlockedIncrement(&m_RefCount); }
    STDMETHOD_(ULONG, Release)(
        THIS
        ) 
        { ULONG c = InterlockedDecrement(&m_RefCount);
            if (c == 0) {
                delete this;
            }
            return c;
        }

    // IDebugDataSpaces.
    STDMETHOD(ReadVirtual)(
        THIS_
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        ) 
        {
            SIZE_T BytesReadLocal;
            BOOL b;
            b = ReadProcessMemory(GetCurrentProcess(),
                                 (LPCVOID)(UINT_PTR)Offset,
                                 Buffer,
                                 BufferSize,
                                 &BytesReadLocal);
            if(BytesRead) {
                *BytesRead = (ULONG)BytesReadLocal;
            }
            if (b) {
                return S_OK;
            } else {
                return E_FAIL;
            }
        }
    STDMETHOD(WriteVirtual)(
        THIS_
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        )
        {
            BOOL b;

            b = WriteProcessMemory(GetCurrentProcess(),
                                   (PVOID)(UINT_PTR)Offset,
                                   Buffer,
                                   BufferSize,
                                   BytesWritten);

            if (b) { 
                return S_OK;
            } else {
                return E_FAIL;
            }
        }
    STDMETHOD(SearchVirtual)(
        THIS_
        IN ULONG64 Offset,
        IN ULONG64 Length,
        IN PVOID Pattern,
        IN ULONG PatternSize,
        IN ULONG PatternGranularity,
        OUT PULONG64 MatchOffset
        ) { return E_NOTIMPL; }
    STDMETHOD(ReadVirtualUncached)(
        THIS_
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        ) { return E_NOTIMPL; }
    STDMETHOD(WriteVirtualUncached)(
        THIS_
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        ) { return E_NOTIMPL; }
    STDMETHOD(ReadPointersVirtual)(
        THIS_
        IN ULONG Count,
        IN ULONG64 Offset,
        OUT /* size_is(Count) */ PULONG64 Ptrs
        ) { return E_NOTIMPL; }
    STDMETHOD(WritePointersVirtual)(
        THIS_
        IN ULONG Count,
        IN ULONG64 Offset,
        IN /* size_is(Count) */ PULONG64 Ptrs
        ) { return E_NOTIMPL; }
    STDMETHOD(ReadPhysical)(
        THIS_
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        ) { return E_NOTIMPL; }
    STDMETHOD(WritePhysical)(
        THIS_
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        ) { return E_NOTIMPL; }
    STDMETHOD(ReadControl)(
        THIS_
        IN ULONG Processor,
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        ) { return E_NOTIMPL; }
    STDMETHOD(WriteControl)(
        THIS_
        IN ULONG Processor,
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        ) { return E_NOTIMPL; }
    STDMETHOD(ReadIo)(
        THIS_
        IN ULONG InterfaceType,
        IN ULONG BusNumber,
        IN ULONG AddressSpace,
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        ) { return E_NOTIMPL; }
    STDMETHOD(WriteIo)(
        THIS_
        IN ULONG InterfaceType,
        IN ULONG BusNumber,
        IN ULONG AddressSpace,
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        ) { return E_NOTIMPL; }
    STDMETHOD(ReadMsr)(
        THIS_
        IN ULONG Msr,
        OUT PULONG64 Value
        ) { return E_NOTIMPL; }
    STDMETHOD(WriteMsr)(
        THIS_
        IN ULONG Msr,
        IN ULONG64 Value
        ) { return E_NOTIMPL; }
    STDMETHOD(ReadBusData)(
        THIS_
        IN ULONG BusDataType,
        IN ULONG BusNumber,
        IN ULONG SlotNumber,
        IN ULONG Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        ) { return E_NOTIMPL; }
    STDMETHOD(WriteBusData)(
        THIS_
        IN ULONG BusDataType,
        IN ULONG BusNumber,
        IN ULONG SlotNumber,
        IN ULONG Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        ) { return E_NOTIMPL; }
    STDMETHOD(CheckLowMemory)(
        THIS
        ) { return E_NOTIMPL; }
    STDMETHOD(ReadDebuggerData)(
        THIS_
        IN ULONG Index,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG DataSize
        ) { return E_NOTIMPL; }
    STDMETHOD(ReadProcessorSystemData)(
        THIS_
        IN ULONG Processor,
        IN ULONG Index,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG DataSize
        ) { return E_NOTIMPL; }

    CDebugDataSpaces() { m_RefCount = 1; }
    virtual ~CDebugDataSpaces() { }

private:
    LONG m_RefCount;
};


class CDebugRegisters : public IDebugRegisters {
public:
    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        )
        {
            if (IsEqualGUID(InterfaceId, IID_IUnknown)) {
                AddRef();
                *Interface = (PVOID)this;
            } else if (IsEqualGUID(InterfaceId, IID_IDebugRegisters)) {
                AddRef();
                *Interface = (PVOID)this;
            } else {
                fprintf(stderr, 
                        "Error: CDebugRegisters::QueryInterface called "
                        "with unknown IID at address %x\n", &InterfaceId);
                return E_NOINTERFACE;
            }
            return S_OK;
        }
    STDMETHOD_(ULONG, AddRef)(
        THIS
        ) 
        { return InterlockedIncrement(&m_RefCount); }
    STDMETHOD_(ULONG, Release)(
        THIS
        ) 
        { ULONG c = InterlockedDecrement(&m_RefCount);
            if (c == 0) {
                delete this;
            }
            return c;
        }

    // IDebugRegisters.
    STDMETHOD(GetNumberRegisters)(
        THIS_
        OUT PULONG Number
        ) { return E_NOTIMPL; }
    STDMETHOD(GetDescription)(
        THIS_
        IN ULONG Register,
        OUT OPTIONAL PSTR NameBuffer,
        IN ULONG NameBufferSize,
        OUT OPTIONAL PULONG NameSize,
        OUT OPTIONAL PDEBUG_REGISTER_DESCRIPTION Desc
        ) { return E_NOTIMPL; }
    STDMETHOD(GetIndexByName)(
        THIS_
        IN PCSTR Name,
        OUT PULONG Index
        )
        {
            ULONG i;

            for (i=0; i<sizeof(RegNameList)/sizeof(RegNameList[0]); ++i) {
                if (strcmp(RegNameList[i].Name, Name) == 0) {
                    *Index = i;
                    return S_OK;
                }
            }
            return E_FAIL;
        }
    STDMETHOD(GetValue)(
        THIS_
        IN ULONG Register,
        OUT PDEBUG_VALUE Value
        )
        {
            if (Register > sizeof(RegNameList)/sizeof(RegNameList[0])) {
                return E_FAIL;
            }
            FetchContext();
            Value->I32 = *(ULONG*) ((PBYTE)&m_Context + RegNameList[Register].Offset);
            Value->Type = DEBUG_VALUE_INT32;
            return S_OK;
        }
    STDMETHOD(SetValue)(
        THIS_
        IN ULONG Register,
        IN PDEBUG_VALUE Value
        ) { return E_NOTIMPL; }
    STDMETHOD(GetValues)(
        THIS_
        IN ULONG Count,
        IN OPTIONAL /* size_is(Count) */ PULONG Indices,
        IN ULONG Start,
        OUT /* size_is(Count) */ PDEBUG_VALUE Values
        ) { return E_NOTIMPL; }
    STDMETHOD(SetValues)(
        THIS_
        IN ULONG Count,
        IN OPTIONAL /* size_is(Count) */ PULONG Indices,
        IN ULONG Start,
        IN /* size_is(Count) */ PDEBUG_VALUE Values
        ) { return E_NOTIMPL; }
    STDMETHOD(OutputRegisters)(
        THIS_
        IN ULONG OutputControl,
        IN ULONG Flags
        ) { return E_NOTIMPL; }
    STDMETHOD(GetInstructionOffset)(
        THIS_
        OUT PULONG64 Offset
        ) 
        {
            FetchContext();

#if defined(_X86_)
            *Offset = (ULONG64)m_Context.Eip;
#elif defined(_PPC_)
            *Offset = (ULONG64)m_Context.Iar;
#else
            *Offset = 0;
            PORTABILITY_ASSERT("Need a PC for this architecture.");
#endif
            return S_OK;
        }
    STDMETHOD(GetStackOffset)(
        THIS_
        OUT PULONG64 Offset
        )
        {
            FetchContext();
#if defined(_X86_)
            *Offset = (ULONG64)m_Context.Esp;
#elif defined(_PPC_)
            *Offset = (ULONG64)m_Context.Gpr1;
#else
            *Offset = 0;
            PORTABILITY_ASSERT("Need a stack pointer for this architecture.");
#endif
            return S_OK;
        }
    STDMETHOD(GetFrameOffset)(
        THIS_
        OUT PULONG64 Offset
        ) { return E_NOTIMPL; }

    CDebugRegisters() { m_RefCount = 1; m_bContextFetched = FALSE; }
    virtual ~CDebugRegisters() { };

private:
    LONG m_RefCount;
    BOOL m_bContextFetched;
    CONTEXT m_Context;

    void FetchContext(void) {
        if (m_bContextFetched)
            return;
        m_bContextFetched = TRUE;
        m_Context.ContextFlags = CONTEXT_CONTROL|CONTEXT_INTEGER;
        if (!GetThreadContext(GetCurrentThread(), &m_Context)) {
            fprintf(stderr, "SOS: GetThreadContext() failed.  LastErr=%d\n",
                    GetLastError());
            memset(&m_Context, 0xfe, sizeof(m_Context));
        }
    }
};


class CDebugClient : public IDebugClient {
public:
    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        )
        {
            *Interface = NULL;
            if (IsEqualGUID(InterfaceId, IID_IUnknown)) {
                AddRef();
                *Interface = (PVOID)this;
            } else if (IsEqualGUID(InterfaceId, IID_IDebugClient)) {
                AddRef();
                *Interface = (PVOID)this;
            } else if (IsEqualGUID(InterfaceId, IID_IDebugAdvanced)) {
                *Interface = (PVOID)(new CDebugAdvanced);
            } else if (IsEqualGUID(InterfaceId, IID_IDebugControl)) {
                *Interface = (PVOID)(new CDebugControl);
            } else if (IsEqualGUID(InterfaceId, IID_IDebugDataSpaces) || 
                       IsEqualGUID(InterfaceId, IID_IDebugDataSpaces2)) {
                *Interface = (PVOID)(new CDebugDataSpaces);
            } else if (IsEqualGUID(InterfaceId, IID_IDebugRegisters)) {
                *Interface = (PVOID)(new CDebugRegisters);
            } else if (IsEqualGUID(InterfaceId, IID_IDebugSystemObjects) ||
                       IsEqualGUID(InterfaceId, IID_IDebugSystemObjects2)) {
                *Interface = (PVOID)1;
            } else {
                fprintf(stderr, "Error: CDebugClient::QueryInterface called "
                        "with unknown IID at address %x\n", &InterfaceId);
                return E_NOINTERFACE;
            }
            if (!*Interface) {
              return E_OUTOFMEMORY;
            }
            return S_OK;
        }
    STDMETHOD_(ULONG, AddRef)(
        THIS
        ) 
        { return InterlockedIncrement(&m_RefCount); }
    STDMETHOD_(ULONG, Release)(
        THIS
        ) 
        { ULONG c = InterlockedDecrement(&m_RefCount);
            if (c == 0) {
                delete this;
            }
            return c;
        }

    // IDebugClient.
    STDMETHOD(AttachKernel)(
        THIS_
        IN ULONG Flags,
        IN OPTIONAL PCSTR ConnectOptions
        ) {return E_NOTIMPL;}
    STDMETHOD(GetKernelConnectionOptions)(
        THIS_
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG OptionsSize
        ) { return E_NOTIMPL;}
    STDMETHOD(SetKernelConnectionOptions)(
        THIS_
        IN PCSTR Options
        ) { return E_NOTIMPL;}
    STDMETHOD(StartProcessServer)(
        THIS_
        IN ULONG Flags,
        IN PCSTR Options,
        IN PVOID Reserved
        ) { return E_NOTIMPL;}
    STDMETHOD(ConnectProcessServer)(
        THIS_
        IN PCSTR RemoteOptions,
        OUT PULONG64 Server
        ) { return E_NOTIMPL;}
    STDMETHOD(DisconnectProcessServer)(
        THIS_
        IN ULONG64 Server
        ) { return E_NOTIMPL;}
    STDMETHOD(GetRunningProcessSystemIds)(
        THIS_
        IN ULONG64 Server,
        OUT OPTIONAL /* size_is(Count) */ PULONG Ids,
        IN ULONG Count,
        OUT OPTIONAL PULONG ActualCount
        ) { return E_NOTIMPL;}
    STDMETHOD(GetRunningProcessSystemIdByExecutableName)(
        THIS_
        IN ULONG64 Server,
        IN PCSTR ExeName,
        IN ULONG Flags,
        OUT PULONG Id
        ) {return E_NOTIMPL;}
    STDMETHOD(GetRunningProcessDescription)(
        THIS_
        IN ULONG64 Server,
        IN ULONG SystemId,
        IN ULONG Flags,
        OUT OPTIONAL PSTR ExeName,
        IN ULONG ExeNameSize,
        OUT OPTIONAL PULONG ActualExeNameSize,
        OUT OPTIONAL PSTR Description,
        IN ULONG DescriptionSize,
        OUT OPTIONAL PULONG ActualDescriptionSize
        ) { return E_NOTIMPL;}
     STDMETHOD(AttachProcess)(
        THIS_
        IN ULONG64 Server,
        IN ULONG ProcessId,
        IN ULONG AttachFlags
        ) { return E_NOTIMPL; }
    STDMETHOD(CreateProcess)(
        THIS_
        IN ULONG64 Server,
        IN PSTR CommandLine,
        IN ULONG CreateFlags
        ) { return E_NOTIMPL;}
    STDMETHOD(CreateProcessAndAttach)(
        THIS_
        IN ULONG64 Server,
        IN OPTIONAL PSTR CommandLine,
        IN ULONG CreateFlags,
        IN ULONG ProcessId,
        IN ULONG AttachFlags
        ) { return E_NOTIMPL;}
    STDMETHOD(GetProcessOptions)(
        THIS_
        OUT PULONG Options
        ) { return E_NOTIMPL;}
    STDMETHOD(AddProcessOptions)(
        THIS_
        IN ULONG Options
        ) { return E_NOTIMPL;}
    STDMETHOD(RemoveProcessOptions)(
        THIS_
        IN ULONG Options
        ) { return E_NOTIMPL;}
    STDMETHOD(SetProcessOptions)(
        THIS_
        IN ULONG Options
        ) { return E_NOTIMPL;}
    STDMETHOD(OpenDumpFile)(
        THIS_
        IN PCSTR DumpFile
        ) { return E_NOTIMPL;}
    STDMETHOD(WriteDumpFile)(
        THIS_
        IN PCSTR DumpFile,
        IN ULONG Qualifier
        ) { return E_NOTIMPL;}
    STDMETHOD(ConnectSession)(
        THIS_
        IN ULONG Flags,
        IN ULONG HistoryLimit
        ) { return E_NOTIMPL;}
    STDMETHOD(StartServer)(
        THIS_
        IN PCSTR Options
        ) { return E_NOTIMPL;}
    STDMETHOD(OutputServers)(
        THIS_
        IN ULONG OutputControl,
        IN PCSTR Machine,
        IN ULONG Flags
        ) { return E_NOTIMPL;}
    STDMETHOD(TerminateProcesses)(
        THIS
        ) { return E_NOTIMPL;}
    STDMETHOD(DetachProcesses)(
        THIS
        ) { return E_NOTIMPL;}
    STDMETHOD(EndSession)(
        THIS_
        IN ULONG Flags
        ) { return E_NOTIMPL;}
    STDMETHOD(GetExitCode)(
        THIS_
        OUT PULONG Code
        ) { return E_NOTIMPL;}
    STDMETHOD(DispatchCallbacks)(
        THIS_
        IN ULONG Timeout
        ) { return E_NOTIMPL;}
    STDMETHOD(ExitDispatch)(
        THIS_
        IN PDEBUG_CLIENT Client
        ) { return E_NOTIMPL;}
    STDMETHOD(CreateClient)(
        THIS_
        OUT PDEBUG_CLIENT* Client
        ) { return E_NOTIMPL;}    
    STDMETHOD(GetInputCallbacks)(
        THIS_
        OUT PDEBUG_INPUT_CALLBACKS* Callbacks
        ) { return E_NOTIMPL;}
    STDMETHOD(SetInputCallbacks)(
        THIS_
        IN PDEBUG_INPUT_CALLBACKS Callbacks
        ) { return E_NOTIMPL;}
    STDMETHOD(GetOutputCallbacks)(
        THIS_
        OUT PDEBUG_OUTPUT_CALLBACKS* Callbacks
        ) { return E_NOTIMPL;}
    STDMETHOD(SetOutputCallbacks)(
        THIS_
        IN PDEBUG_OUTPUT_CALLBACKS Callbacks
        ) { return E_NOTIMPL;}
    STDMETHOD(GetOutputMask)(
        THIS_
        OUT PULONG Mask
        ) { return E_NOTIMPL;}
    STDMETHOD(SetOutputMask)(
        THIS_
        IN ULONG Mask
        ) { return E_NOTIMPL;}
    STDMETHOD(GetOtherOutputMask)(
        THIS_
        IN PDEBUG_CLIENT Client,
        OUT PULONG Mask
        ) { return E_NOTIMPL;}
    STDMETHOD(SetOtherOutputMask)(
        THIS_
        IN PDEBUG_CLIENT Client,
        IN ULONG Mask
        ) { return E_NOTIMPL;}
    STDMETHOD(GetOutputWidth)(
        THIS_
        OUT PULONG Columns
        ) { return E_NOTIMPL;}
    STDMETHOD(SetOutputWidth)(
        THIS_
        IN ULONG Columns
        ) { return E_NOTIMPL;}
    STDMETHOD(GetOutputLinePrefix)(
        THIS_
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG PrefixSize
        ) { return E_NOTIMPL;}
    STDMETHOD(SetOutputLinePrefix)(
        THIS_
        IN OPTIONAL PCSTR Prefix
        ) { return E_NOTIMPL;}
    STDMETHOD(GetIdentity)(
        THIS_
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG IdentitySize
        ) { return E_NOTIMPL;}
    STDMETHOD(OutputIdentity)(
        THIS_
        IN ULONG OutputControl,
        IN ULONG Flags,
        IN PCSTR Format
        ) { return E_NOTIMPL;}
    STDMETHOD(GetEventCallbacks)(
        THIS_
        OUT PDEBUG_EVENT_CALLBACKS* Callbacks
        ) { return E_NOTIMPL;}
    STDMETHOD(SetEventCallbacks)(
        THIS_
        IN PDEBUG_EVENT_CALLBACKS Callbacks
        ) { return E_NOTIMPL;}
    STDMETHOD(FlushCallbacks)(
        THIS
        ) { return E_NOTIMPL;}

    CDebugClient() { m_RefCount = 1; }
    virtual ~CDebugClient() { }
private:
    LONG m_RefCount;
};

extern "C"
HRESULT
CALLBACK
DebugExtensionInitialize(PULONG Version, PULONG Flags);

extern "C"
void
CALLBACK
DebugExtensionNotify(ULONG Notify, ULONG64 /*Argument*/);

char * SOSCommands[] = {
    "IP2MD",
    "DumpMD",
    "DumpClass",
    "DumpMT",
    "DumpObj",
#ifndef PLATFORM_UNIX
    "DumpStack",
    "EEStack",
    "u",
#endif
    "DumpStackObjects",
    "SyncBlk",
    "Threads",
    "ThreadPool",
    "DumpModule",
    "DumpDomain",
    "DumpAssembly",
    "DumpLoader",
    "SearchStack",
    "DumpCrawlFrame",
    "Token2EE",
    "Name2EE",
    "SymOpt",
    "Help",
};

// This is called from sscoree.so's SOS() function
extern "C" void __cdecl GDBSOS(HMODULE hSOS, char *Command)
{
    char *CommandArgs;
    ExtensionApi pfn;
    CDebugClient Client;
    HRESULT hr;
    ULONG Version;
    ULONG Flags;
    unsigned i;

    // If the command string came in quoted, then strip the quotes
    if (*Command == '\"' && Command[strlen(Command)-1] == '\"') {
      Command++;
      Command[strlen(Command)-1] = '\0';
    }

    // skip leading spaces
    while (isspace(*Command)) {
        Command++;
    }

    // Find the end of the command name, and the beginning of its arguments
    CommandArgs = Command;
    while (!isspace(*CommandArgs)) {
        CommandArgs++;
    }
    *CommandArgs++ = '\0';
    while (isspace(*CommandArgs)) {
        CommandArgs++;
    }

    for (i = 0; i < sizeof(SOSCommands)/sizeof(char *); i++)
    {
        if (_stricmp(SOSCommands[i], Command) == 0)
        {
            Command = SOSCommands[i];
            break;
        }
    }

    if (i >= sizeof(SOSCommands)/sizeof(char *))
    {
        fprintf(stderr, "SOS:  Command '%s' not found.\n", Command);
        return;
    }

    pfn = (ExtensionApi)GetProcAddress(hSOS, Command);
    if (!pfn) {
	fprintf(stderr, "SOS:  Command '%s' not found.\n", Command);
	return;
    }

    hr = DebugExtensionInitialize(&Version, &Flags);
    if (FAILED(hr)) {
        fprintf(stderr, "SOS:  DebugExtensionInitialize() failed.  hr=%x\n",
                hr);
        return;
    }
    DebugExtensionNotify(DEBUG_NOTIFY_SESSION_ACCESSIBLE, 0);

    // Call the extension API
    PAL_TRY {
        (*pfn)(&Client, CommandArgs);
    } PAL_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        fprintf(stderr, "SOS:  Command '%s' threw an exception.\n", Command);
    }
    PAL_ENDTRY
}

STDAPI
DebugCreate(
            IN REFIID InterfaceId,
            OUT PVOID *Interface)
{
  if (IsEqualGUID(InterfaceId, IID_IDebugClient)) {
    *Interface = (PVOID)(new CDebugClient);
    if (!*Interface) {
      return E_OUTOFMEMORY;
    }
    return S_OK;
  }
  return E_NOINTERFACE;
}


#endif //PLATFORM_UNIX  // entire file

