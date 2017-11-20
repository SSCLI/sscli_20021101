/*++


 Copyright (c) 2002 Microsoft Corporation.  All rights reserved.

 The use and distribution terms for this software are contained in the file
 named license.txt, which can be found in the root of this distribution.
 By using this software in any fashion, you are agreeing to be bound by the
 terms of this license.

 You must not remove this notice, or any other, from this software.


Module Name:

    context.c

Abstract:

    Implementation of GetThreadContext/SetThreadContext/DebugBreak functions for
    the Intel x86 platform. These functions are processor dependent.

--*/

#include "pal/palinternal.h"
#include "pal/dbgmsg.h"
#include "pal/thread.h"
#include "pal/context.h"
#include "pal/debug.h"

#include <sys/ptrace.h> 
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#if HAVE_BSD_REGS_T
#include <machine/reg.h>
#include <machine/npx.h>
#endif  // HAVE_BSD_REGS_T
#if HAVE_PT_REGS
#include <asm/ptrace.h>
#endif  // HAVE_PT_REGS

SET_DEFAULT_DEBUG_CHANNEL(DEBUG);


/*++
Function:
  CONTEXT_GetRegisters

Abstract
  retrieve the machine registers value of the indicated process.

Parameter
  processId: process ID
  registers: reg structure in which the machine registers value will be returned.
Return
 returns TRUE if it succeeds, FALSE otherwise
--*/
#if HAVE_GETCONTEXT
BOOL CONTEXT_GetRegisters(DWORD processId, ucontext_t *registers)
#elif HAVE_BSD_REGS_T
BOOL CONTEXT_GetRegisters(DWORD processId, struct reg *registers)
#endif  // HAVE_BSD_REGS_T
{
#if HAVE_BSD_REGS_T
    char buf[MAX_PATH];
    int regFd = -1;
#endif  // HAVE_BSD_REGS_T
    BOOL bRet = FALSE;

    if (processId == GetCurrentProcessId()) 
    {
#if HAVE_GETCONTEXT
        if (getcontext(registers) != 0)
        {
            ASSERT("getcontext() failed %d (%s)\n", errno, strerror(errno));
            return FALSE;
        }
#elif HAVE_BSD_REGS_T
        sprintf(buf, "/proc/%d/regs", processId);

        if ((regFd = open(buf, O_RDONLY)) == -1) 
        {
          ASSERT("open() failed %d (%s) \n", errno, strerror(errno));
          return FALSE;
        }

        if (lseek(regFd, 0, 0) == -1)
        {
            ASSERT("lseek() failed %d (%s)\n", errno, strerror(errno));
            goto EXIT;
        }

        if (read(regFd, registers, sizeof(*registers)) != sizeof(*registers))
        {
            ASSERT("read() failed %d (%s)\n", errno, strerror(errno));
            goto EXIT;
        }
#endif  // HAVE_BSD_REGS_T
    }
    else
    {
#if HAVE_GETCONTEXT
        struct pt_regs ptrace_registers;
#endif  // HAVE_GETCONTEXT
        if (DBGAttachProcess(processId) == FALSE)
        {
            goto EXIT;
        }

        if (ptrace(PT_GETREGS, processId, (caddr_t) registers, 0) == -1)
        {
            ASSERT("Failed ptrace(PT_GETREGS, processId:%d) errno:%d (%s)\n",
                   processId, errno, strerror(errno));
        }
        
#if HAVE_GETCONTEXT
        // Copy the registers into our ucontext_t.
        registers->uc_mcontext.gregs[REG_EBX] = ptrace_registers.ebx;
        registers->uc_mcontext.gregs[REG_ECX] = ptrace_registers.ecx;
        registers->uc_mcontext.gregs[REG_EDX] = ptrace_registers.edx;
        registers->uc_mcontext.gregs[REG_ESI] = ptrace_registers.esi;
        registers->uc_mcontext.gregs[REG_EDI] = ptrace_registers.edi;
        registers->uc_mcontext.gregs[REG_EBP] = ptrace_registers.ebp;
        registers->uc_mcontext.gregs[REG_EAX] = ptrace_registers.eax;
        registers->uc_mcontext.gregs[REG_EIP] = ptrace_registers.eip;
        registers->uc_mcontext.gregs[REG_CS] = ptrace_registers.xcs;
        registers->uc_mcontext.gregs[REG_EFL] = ptrace_registers.eflags;
        registers->uc_mcontext.gregs[REG_ESP] = ptrace_registers.esp;
        registers->uc_mcontext.gregs[REG_SS] = ptrace_registers.xss;
#endif  // HAVE_GETCONTEXT

        if (!DBGDetachProcess(processId))
        {            
            goto EXIT;
        }
    }
    
    bRet = TRUE;
EXIT:
#if HAVE_BSD_REGS_T
    if (regFd != -1)
    {
        close(regFd);
    }
#endif  // HAVE_BSD_REGS_T
    return bRet;
}

/*++
Function:
  GetThreadContext

See MSDN doc.
--*/
BOOL
CONTEXT_GetThreadContext(
         HANDLE hThread,
         LPCONTEXT lpContext)
{    
    BOOL ret = FALSE;
    DWORD processId;

#if HAVE_GETCONTEXT
    ucontext_t registers;
#elif HAVE_BSD_REGS_T
    struct reg registers;
#endif  // HAVE_BSD_REGS_T
    
    if (lpContext == NULL)
    {
        ERROR("Invalid lpContext parameter value\n");
        SetLastError(ERROR_NOACCESS);
        goto EXIT;
    }
    
    /* How to consider the case when hThread is different from the current
       thread of its owner process. Machine registers values could be retreived
       by a ptrace(pid, ...) call or from the "/proc/%pid/reg" file content. 
       Unfortunately, these two methods only depend on process ID, not on 
       thread ID. */
    if (!(processId = THREADGetThreadProcessId(hThread)))
    {
        ERROR("Couldn't retrieve the process owner of hThread:%p\n", hThread);
        SetLastError(ERROR_INTERNAL_ERROR);
        goto EXIT;
    }

    if (processId == GetCurrentProcessId())
    {
        THREAD *thread;

        thread = (THREAD *) HMGRLockHandle2(hThread, HOBJ_THREAD);

        if ((thread == NULL) || (thread->dwThreadId != GetCurrentThreadId()))
        {
            DWORD flags;
            // There aren't any APIs for this. We can potentially get the
            // context of another thread by using per-thread signals, but
            // on FreeBSD signal handlers that are called as a result
            // of signals raised via pthread_kill don't get a valid
            // sigcontext or ucontext_t. But we need this to return TRUE
            // to avoid an assertion in the CLR in code that manages to
            // cope reasonably well without a valid thread context.
            // Given that, we'll zero out our structure and return TRUE.
            ERROR("GetThreadContext on a thread other than the current "
                  "thread is returning TRUE\n");
            flags = lpContext->ContextFlags;
            memset(lpContext, 0, sizeof(*lpContext));
            lpContext->ContextFlags = flags;
            HMGRUnlockHandle(hThread, &(thread->objHeader));
            ret = TRUE;
            goto EXIT;
        }

        HMGRUnlockHandle(hThread, &(thread->objHeader));
    }

    if (lpContext->ContextFlags & 
        (CONTEXT_CONTROL | CONTEXT_INTEGER))
    {        
        if (CONTEXT_GetRegisters(processId, &registers) == FALSE)
        {
            SetLastError(ERROR_INTERNAL_ERROR);
            goto EXIT;
        }

        if (lpContext->ContextFlags & CONTEXT_CONTROL)
        {
#if HAVE_GETCONTEXT
            lpContext->Ebp    = registers.uc_mcontext.gregs[REG_EBP];
            lpContext->Eip    = registers.uc_mcontext.gregs[REG_EIP];
            lpContext->SegCs  = registers.uc_mcontext.gregs[REG_CS];
            lpContext->EFlags = registers.uc_mcontext.gregs[REG_EFL];
            lpContext->Esp    = registers.uc_mcontext.gregs[REG_ESP];
            lpContext->SegSs  = registers.uc_mcontext.gregs[REG_SS];
#elif HAVE_BSD_REGS_T
            lpContext->Ebp    = registers.r_ebp;
            lpContext->Eip    = registers.r_eip;
            lpContext->SegCs  = registers.r_cs;
            lpContext->EFlags = registers.r_eflags;
            lpContext->Esp    = registers.r_esp;            
            lpContext->SegSs  = registers.r_ss;
#endif  // HAVE_BSD_REGS_T
        }        
        if (lpContext->ContextFlags & CONTEXT_INTEGER)
        {
#if HAVE_GETCONTEXT
            lpContext->Edi    = registers.uc_mcontext.gregs[REG_EDI];
            lpContext->Esi    = registers.uc_mcontext.gregs[REG_ESI];
            lpContext->Ebx    = registers.uc_mcontext.gregs[REG_EBX];
            lpContext->Edx    = registers.uc_mcontext.gregs[REG_EDX];
            lpContext->Ecx    = registers.uc_mcontext.gregs[REG_ECX];
            lpContext->Eax    = registers.uc_mcontext.gregs[REG_EAX];
#elif HAVE_BSD_REGS_T
            lpContext->Edi = registers.r_edi;
            lpContext->Esi = registers.r_esi;
            lpContext->Ebx = registers.r_ebx;
            lpContext->Edx = registers.r_edx;
            lpContext->Ecx = registers.r_ecx;
            lpContext->Eax = registers.r_eax;
#endif  // HAVE_BSD_REGS_T
        }
    }    

    ret = TRUE;

EXIT:
    return ret;
}

/*++
Function:
  SetThreadContext

See MSDN doc.
--*/
BOOL
CONTEXT_SetThreadContext(
           IN HANDLE hThread,
           IN CONST CONTEXT *lpContext)
{
    BOOL ret = FALSE;
    DWORD processId;
    
#if HAVE_PT_REGS
    struct pt_regs registers;
#elif HAVE_BSD_REGS_T
    struct reg registers;
#endif  // HAVE_BSD_REGS_T
    
    if (lpContext == NULL)
    {
        ERROR("Invalid lpContext parameter value\n");
        SetLastError(ERROR_NOACCESS);
        goto EXIT;
    }
    
    /* How to consider the case when hThread is different from the current
       thread of its owner process. Machine registers values could be retreived
       by a ptrace(pid, ...) call or from the "/proc/%pid/reg" file content. 
       Unfortunately, these two methods only depend on process ID, not on 
       thread ID. */
    if (!(processId = THREADGetThreadProcessId(hThread)))
    {
        ASSERT("Couldn't retrieve the process owner of hThread:%p\n", hThread);
        SetLastError(ERROR_INTERNAL_ERROR);
        goto EXIT;
    }
        
    if (processId == GetCurrentProcessId())
    {
        ASSERT("SetThreadContext should be called for cross-process only.\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        goto EXIT;
    }
    
    if (lpContext->ContextFlags  & 
        (CONTEXT_CONTROL | CONTEXT_INTEGER))
    {        
        if (ptrace(PT_GETREGS, processId, 0, (int) &registers ) == -1)
        {
            ASSERT("Failed ptrace(PT_GETREGS, processId:%d) errno:%d (%s)\n",
                   processId, errno, strerror(errno));
             SetLastError(ERROR_INTERNAL_ERROR);
             goto EXIT;
        }
        
        if (lpContext->ContextFlags & CONTEXT_CONTROL)
        {
#if HAVE_PT_REGS
            registers.ebp = lpContext->Ebp;
            registers.eip = lpContext->Eip;
            registers.xcs = lpContext->SegCs;
            registers.eflags = lpContext->EFlags;
            registers.esp = lpContext->Esp;
            registers.xss = lpContext->SegSs;
#elif HAVE_BSD_REGS_T
            registers.r_ebp = lpContext->Ebp;
            registers.r_eip = lpContext->Eip;            
            registers.r_cs = lpContext->SegCs;
            registers.r_eflags = lpContext->EFlags;
            registers.r_esp = lpContext->Esp;            
            registers.r_ss = lpContext->SegSs;            
#endif  // HAVE_BSD_REGS_T
        }
        if (lpContext->ContextFlags & CONTEXT_INTEGER)
        {
#if HAVE_PT_REGS
            registers.edi = lpContext->Edi;
            registers.esi = lpContext->Esi;
            registers.ebx = lpContext->Ebx;
            registers.edx = lpContext->Edx;
            registers.ecx = lpContext->Ecx;
            registers.eax = lpContext->Eax;
#elif HAVE_BSD_REGS_T
            registers.r_edi = lpContext->Edi;            
            registers.r_esi = lpContext->Esi;            
            registers.r_ebx = lpContext->Ebx;
            registers.r_edx = lpContext->Edx;
            registers.r_ecx = lpContext->Ecx;
            registers.r_eax = lpContext->Eax;
#endif  // HAVE_BSD_REGS_T
        }
        
        if (ptrace(PT_SETREGS, processId, 0, (int) &registers) == -1)
        {
            ASSERT("Failed ptrace(PT_SETREGS, processId:%d) errno:%d (%s)\n",
                   processId, errno, strerror(errno));
            SetLastError(ERROR_INTERNAL_ERROR);
            goto EXIT;
        }
    }

    ret = TRUE;
   EXIT:
     return ret;
}

/*++
Function:
  DBG_DebugBreak: same as DebugBreak

See MSDN doc.
--*/
VOID
DBG_DebugBreak()
{
    __asm__ __volatile__("int3");
}


/*++
Function:
  DBG_FlushInstructionCache: processor-specific portion of 
  FlushInstructionCache

See MSDN doc.
--*/
BOOL
DBG_FlushInstructionCache(
                          IN LPCVOID lpBaseAddress,
                          IN SIZE_T dwSize)
{
    // Intel x86 hardware has cache coherency, so nothing needs to be done.
    return TRUE;
}

/*++
Function :
    CONTEXTToNativeContext
    
    Converts a CONTEXT record to a native context.

Parameters :
    CONST CONTEXT *lpContext : CONTEXT to convert
    native_context_t *native : native context to fill in
    ULONG contextFlags : flags that determine which registers are valid in
                         lpContext and which ones to set in native

Return value :
    None

--*/
void CONTEXTToNativeContext(CONST CONTEXT *lpContext, native_context_t *native,
                            ULONG contextFlags)
{
    if (contextFlags != (CONTEXT_CONTROL | CONTEXT_INTEGER))
    {
        ASSERT("Invalid contextFlags in CONTEXTToNativeContext!");
    }
    // Set the control and integer registers
#if HAVE_GREGSET_T
    native->uc_mcontext.gregs[REG_EIP] = lpContext->Eip;            
    native->uc_mcontext.gregs[REG_EFL] = lpContext->EFlags;
    native->uc_mcontext.gregs[REG_EAX] = lpContext->Eax;
    native->uc_mcontext.gregs[REG_EBX] = lpContext->Ebx;
    native->uc_mcontext.gregs[REG_ECX] = lpContext->Ecx;
    native->uc_mcontext.gregs[REG_EDX] = lpContext->Edx;
    native->uc_mcontext.gregs[REG_ESI] = lpContext->Esi;
    native->uc_mcontext.gregs[REG_EDI] = lpContext->Edi;
    native->uc_mcontext.gregs[REG_EBP] = lpContext->Ebp;
    native->uc_mcontext.gregs[REG_ESP] = lpContext->Esp;
    native->uc_mcontext.gregs[REG_CS] = lpContext->SegCs;
    native->uc_mcontext.gregs[REG_SS] = lpContext->SegSs;
#else   // HAVE_GREGSET_T
    native->uc_mcontext.mc_eip = lpContext->Eip;            
    native->uc_mcontext.mc_eflags = lpContext->EFlags;
    native->uc_mcontext.mc_eax = lpContext->Eax;
    native->uc_mcontext.mc_ebx = lpContext->Ebx;
    native->uc_mcontext.mc_ecx = lpContext->Ecx;
    native->uc_mcontext.mc_edx = lpContext->Edx;
    native->uc_mcontext.mc_esi = lpContext->Esi;
    native->uc_mcontext.mc_edi = lpContext->Edi;
    native->uc_mcontext.mc_ebp = lpContext->Ebp;
    native->uc_mcontext.mc_esp = lpContext->Esp;
    native->uc_mcontext.mc_cs = lpContext->SegCs;
    native->uc_mcontext.mc_ss = lpContext->SegSs;
#endif  // HAVE_GREGSET_T
}

/*++
Function :
    CONTEXTFromNativeContext
    
    Converts a native context to a CONTEXT record.

Parameters :
    const native_context_t *native : native context to convert
    LPCONTEXT lpContext : CONTEXT to fill in
    ULONG contextFlags : flags that determine which registers are valid in
                         native and which ones to set in lpContext

Return value :
    None

--*/
void CONTEXTFromNativeContext(const native_context_t *native, LPCONTEXT lpContext,
                              ULONG contextFlags)
{
    if (contextFlags != (CONTEXT_CONTROL | CONTEXT_INTEGER))
    {
        ASSERT("Invalid contextFlags in CONTEXTFromNativeContext!");
    }
    lpContext->ContextFlags = contextFlags;
#if HAVE_GREGSET_T
    lpContext->EFlags = native->uc_mcontext.gregs[REG_EFL];
    lpContext->Eax = native->uc_mcontext.gregs[REG_EAX];
    lpContext->Ebx = native->uc_mcontext.gregs[REG_EBX];
    lpContext->Ecx = native->uc_mcontext.gregs[REG_ECX];
    lpContext->Edx = native->uc_mcontext.gregs[REG_EDX];
    lpContext->Esi = native->uc_mcontext.gregs[REG_ESI];
    lpContext->Edi = native->uc_mcontext.gregs[REG_EDI];
    lpContext->Eip = native->uc_mcontext.gregs[REG_EIP];
    lpContext->Ebp = native->uc_mcontext.gregs[REG_EBP];
    lpContext->Esp = native->uc_mcontext.gregs[REG_ESP];
    lpContext->SegCs = native->uc_mcontext.gregs[REG_CS];
    lpContext->SegSs = native->uc_mcontext.gregs[REG_SS];
#else   // HAVE_GREGSET_T
    lpContext->EFlags = native->uc_mcontext.mc_eflags;
    lpContext->Eax = native->uc_mcontext.mc_eax;
    lpContext->Ebx = native->uc_mcontext.mc_ebx;
    lpContext->Ecx = native->uc_mcontext.mc_ecx;
    lpContext->Edx = native->uc_mcontext.mc_edx;
    lpContext->Esi = native->uc_mcontext.mc_esi;
    lpContext->Edi = native->uc_mcontext.mc_edi;
    lpContext->Eip = native->uc_mcontext.mc_eip;
    lpContext->Ebp = native->uc_mcontext.mc_ebp;
    lpContext->Esp = native->uc_mcontext.mc_esp;
    lpContext->SegCs = native->uc_mcontext.mc_cs;
    lpContext->SegSs = native->uc_mcontext.mc_ss;
#endif  // HAVE_GREGSET_T
}

/*++
Function :
    CONTEXTGetPC
    
    Returns the program counter from the native context.

Parameters :
    const native_context_t *native : native context

Return value :
    The program counter from the native context.

--*/
LPVOID CONTEXTGetPC(const native_context_t *context)
{
#if HAVE_GREGSET_T
    return (LPVOID) (context->uc_mcontext.gregs[REG_EIP]);
#else   // HAVE_GREGSET_T
    return (LPVOID) (context->uc_mcontext.mc_eip);
#endif  // HAVE_GREGSET_T
}

/*++
Function :
    CONTEXTGetExceptionCodeForSignal
    
    Translates signal and context information to a Win32 exception code.

Parameters :
    const siginfo_t *siginfo : signal information from a signal handler
    const native_context_t *context : context information

Return value :
    The Win32 exception code that corresponds to the signal and context
    information.

--*/
#ifdef ILL_ILLOPC
// If si_code values are available for all signals, use those.
DWORD CONTEXTGetExceptionCodeForSignal(const siginfo_t *siginfo,
                                       const native_context_t *context)
{
    switch (siginfo->si_signo)
    {
        case SIGILL:
            switch (siginfo->si_code)
            {
                case ILL_ILLOPC:    // Illegal opcode
                case ILL_ILLOPN:    // Illegal operand
                case ILL_ILLADR:    // Illegal addressing mode
                case ILL_ILLTRP:    // Illegal trap
                case ILL_COPROC:    // Co-processor error
                    return EXCEPTION_ILLEGAL_INSTRUCTION;
                case ILL_PRVOPC:    // Privileged opcode
                case ILL_PRVREG:    // Privileged register
                    return EXCEPTION_PRIV_INSTRUCTION;
                case ILL_BADSTK:    // Internal stack error
                    return EXCEPTION_STACK_OVERFLOW;
                default:
                    break;
            }
            break;
        case SIGFPE:
            switch (siginfo->si_code)
            {
                case FPE_INTDIV:
                    return EXCEPTION_INT_DIVIDE_BY_ZERO;
                case FPE_INTOVF:
                    return EXCEPTION_INT_OVERFLOW;
                case FPE_FLTDIV:
                    return EXCEPTION_FLT_DIVIDE_BY_ZERO;
                case FPE_FLTOVF:
                    return EXCEPTION_FLT_OVERFLOW;
                case FPE_FLTUND:
                    return EXCEPTION_FLT_UNDERFLOW;
                case FPE_FLTRES:
                    return EXCEPTION_FLT_INEXACT_RESULT;
                case FPE_FLTINV:
                    return EXCEPTION_FLT_INVALID_OPERATION;
                case FPE_FLTSUB:
                    return EXCEPTION_FLT_INVALID_OPERATION;
                default:
                    break;
            }
            break;
        case SIGSEGV:
            switch (siginfo->si_code)
            {
                case SI_USER:       // User-generated signal, sometimes sent
                                    // for SIGSEGV under normal circumstances
                case SEGV_MAPERR:   // Address not mapped to object
                case SEGV_ACCERR:   // Invalid permissions for mapped object
                    return EXCEPTION_ACCESS_VIOLATION;
                default:
                    break;
            }
            break;
        case SIGBUS:
            switch (siginfo->si_code)
            {
                case BUS_ADRALN:    // Invalid address alignment
                    return EXCEPTION_DATATYPE_MISALIGNMENT;
                case BUS_ADRERR:    // Non-existent physical address
                    return EXCEPTION_ACCESS_VIOLATION;
                case BUS_OBJERR:    // Object-specific hardware error
                default:
                    break;
            }
        case SIGTRAP:
            switch (siginfo->si_code)
            {
            case SI_KERNEL:
                case SI_USER:
                case TRAP_BRKPT:    // Process breakpoint
                    return EXCEPTION_BREAKPOINT;
                case TRAP_TRACE:    // Process trace trap
                    return EXCEPTION_SINGLE_STEP;
                default:
                    break;
            }
        default:
            break;
    }
    ASSERT("Got unknown signal number %d with code %d\n",
           siginfo->si_signo, siginfo->si_code);
    return EXCEPTION_ILLEGAL_INSTRUCTION;
}
#else   // ILL_ILLOPC
DWORD CONTEXTGetExceptionCodeForSignal(const siginfo_t *siginfo,
                                       const native_context_t *context)
{
    int trap;

    if (siginfo->si_signo == SIGFPE)
    {
        // Floating point exceptions are mapped by their si_code.
        switch (siginfo->si_code)
        {
            case FPE_INTDIV :
                TRACE("Got signal SIGFPE:FPE_INTDIV; raising "
                      "EXCEPTION_INT_DIVIDE_BY_ZERO\n");
                return EXCEPTION_INT_DIVIDE_BY_ZERO;
                break;
            case FPE_INTOVF :
                TRACE("Got signal SIGFPE:FPE_INTOVF; raising "
                      "EXCEPTION_INT_OVERFLOW\n");
                return EXCEPTION_INT_OVERFLOW;
                break;
            case FPE_FLTDIV :
                TRACE("Got signal SIGFPE:FPE_FLTDIV; raising "
                      "EXCEPTION_FLT_DIVIDE_BY_ZERO\n");
                return EXCEPTION_FLT_DIVIDE_BY_ZERO;
                break;
            case FPE_FLTOVF :
                TRACE("Got signal SIGFPE:FPE_FLTOVF; raising "
                      "EXCEPTION_FLT_OVERFLOW\n");
                return EXCEPTION_FLT_OVERFLOW;
                break;
            case FPE_FLTUND :
                TRACE("Got signal SIGFPE:FPE_FLTUND; raising "
                      "EXCEPTION_FLT_UNDERFLOW\n");
                return EXCEPTION_FLT_UNDERFLOW;
                break;
            case FPE_FLTRES :
                TRACE("Got signal SIGFPE:FPE_FLTRES; raising "
                      "EXCEPTION_FLT_INEXACT_RESULT\n");
                return EXCEPTION_FLT_INEXACT_RESULT;
                break;
            case FPE_FLTINV :
                TRACE("Got signal SIGFPE:FPE_FLTINV; raising "
                      "EXCEPTION_FLT_INVALID_OPERATION\n");
                return EXCEPTION_FLT_INVALID_OPERATION;
                break;
            case FPE_FLTSUB :/* subscript out of range */
                TRACE("Got signal SIGFPE:FPE_FLTSUB; raising "
                      "EXCEPTION_FLT_INVALID_OPERATION\n");
                return EXCEPTION_FLT_INVALID_OPERATION;
                break;
            default:
                ASSERT("Got unknown signal code %d\n", siginfo->si_code);
                break;
        }
    }

    trap = context->uc_mcontext.mc_trapno;
    switch (trap)
    {
        case T_PRIVINFLT : /* privileged instruction */
            TRACE("Trap code T_PRIVINFLT mapped to EXCEPTION_PRIV_INSTRUCTION\n");
            return EXCEPTION_PRIV_INSTRUCTION; 
        case T_BPTFLT :    /* breakpoint instruction */
            TRACE("Trap code T_BPTFLT mapped to EXCEPTION_BREAKPOINT\n");
            return EXCEPTION_BREAKPOINT;
        case T_ARITHTRAP : /* arithmetic trap */
            TRACE("Trap code T_ARITHTRAP maps to floating point exception...\n");
            return 0;      /* let the caller pick an exception code */
#ifdef T_ASTFLT
        case T_ASTFLT :    /* system forced exception : ^C, ^\. SIGINT signal 
                              handler shouldn't be calling this function, since
                              it doesn't need an exception code */
            ASSERT("Trap code T_ASTFLT received, shouldn't get here\n");
            return 0;
#endif  // T_ASTFLT
        case T_PROTFLT :   /* protection fault */
            TRACE("Trap code T_PROTFLT mapped to EXCEPTION_ACCESS_VIOLATION\n");
            return EXCEPTION_ACCESS_VIOLATION; 
        case T_TRCTRAP :   /* debug exception (sic) */
            TRACE("Trap code T_TRCTRAP mapped to EXCEPTION_SINGLE_STEP\n");
            return EXCEPTION_SINGLE_STEP;
        case T_PAGEFLT :   /* page fault */
            TRACE("Trap code T_PAGEFLT mapped to EXCEPTION_ACCESS_VIOLATION\n");
            return EXCEPTION_ACCESS_VIOLATION;
        case T_ALIGNFLT :  /* alignment fault */
            TRACE("Trap code T_ALIGNFLT mapped to EXCEPTION_DATATYPE_MISALIGNMENT\n");
            return EXCEPTION_DATATYPE_MISALIGNMENT;
        case T_DIVIDE :
            TRACE("Trap code T_DIVIDE mapped to EXCEPTION_INT_DIVIDE_BY_ZERO\n");
            return EXCEPTION_INT_DIVIDE_BY_ZERO;
        case T_NMI :       /* non-maskable trap */
            TRACE("Trap code T_NMI mapped to EXCEPTION_ILLEGAL_INSTRUCTION\n");
            return EXCEPTION_ILLEGAL_INSTRUCTION;
        case T_OFLOW :
            TRACE("Trap code T_OFLOW mapped to EXCEPTION_INT_OVERFLOW\n");
            return EXCEPTION_INT_OVERFLOW;
        case T_BOUND :     /* bound instruction fault */
            TRACE("Trap code T_BOUND mapped to EXCEPTION_ARRAY_BOUNDS_EXCEEDED\n");
            return EXCEPTION_ARRAY_BOUNDS_EXCEEDED; 
        case T_DNA :       /* device not available fault */
            TRACE("Trap code T_DNA mapped to EXCEPTION_ILLEGAL_INSTRUCTION\n");
            return EXCEPTION_ILLEGAL_INSTRUCTION; 
        case T_DOUBLEFLT : /* double fault */
            TRACE("Trap code T_DOUBLEFLT mapped to EXCEPTION_ILLEGAL_INSTRUCTION\n");
            return EXCEPTION_ILLEGAL_INSTRUCTION; 
        case T_FPOPFLT :   /* fp coprocessor operand fetch fault */
            TRACE("Trap code T_FPOPFLT mapped to EXCEPTION_FLT_INVALID_OPERATION\n");
            return EXCEPTION_FLT_INVALID_OPERATION; 
        case T_TSSFLT :    /* invalid tss fault */
            TRACE("Trap code T_TSSFLT mapped to EXCEPTION_ILLEGAL_INSTRUCTION\n");
            return EXCEPTION_ILLEGAL_INSTRUCTION; 
        case T_SEGNPFLT :  /* segment not present fault */
            TRACE("Trap code T_SEGNPFLT mapped to EXCEPTION_ACCESS_VIOLATION\n");
            return EXCEPTION_ACCESS_VIOLATION; 
        case T_STKFLT :    /* stack fault */
            TRACE("Trap code T_STKFLT mapped to EXCEPTION_STACK_OVERFLOW\n");
            return EXCEPTION_STACK_OVERFLOW; 
        case T_MCHK :      /* machine check trap */
            TRACE("Trap code T_MCHK mapped to EXCEPTION_ILLEGAL_INSTRUCTION\n");
            return EXCEPTION_ILLEGAL_INSTRUCTION; 
        case T_RESERVED :  /* reserved (unknown) */
            TRACE("Trap code T_RESERVED mapped to EXCEPTION_ILLEGAL_INSTRUCTION\n");
            return EXCEPTION_ILLEGAL_INSTRUCTION; 
        default:
            ASSERT("Got unknown trap code %d\n", trap);
            break;
    }
    return EXCEPTION_ILLEGAL_INSTRUCTION;
}
#endif  // ILL_ILLOPC
