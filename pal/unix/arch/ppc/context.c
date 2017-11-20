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
    the PowerPC platform. These functions are processor dependent.

--*/

#include "pal/palinternal.h"
#include <mach/message.h>
#include <mach/thread_act.h>
#include "pal/dbgmsg.h"
#include "pal/thread.h"
#include "pal/debug.h"
#include "pal/utils.h"

#include <sys/ptrace.h> 
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

SET_DEFAULT_DEBUG_CHANNEL(DEBUG);

// in interlock.s
extern void CONTEXT_CaptureContext(LPCONTEXT lpContext, BOOL fCaptureFloatingPoint);

/*++
Function:
  CONTEXT_GetThreadContextFromPort

  Helper for GetThreadContext that uses a mach_port
--*/
BOOL
CONTEXT_GetThreadContextFromPort(
        mach_port_t Port,
        LPCONTEXT lpContext)
{
  // Extract the CONTEXT from the Mach thread.

  BOOL ret = FALSE;
  kern_return_t MachRet;
  mach_msg_type_number_t StateCount;

  if (lpContext->ContextFlags & (CONTEXT_CONTROL|CONTEXT_INTEGER)) {
    ppc_thread_state_t State;

    StateCount = PPC_THREAD_STATE_COUNT;
    MachRet = thread_get_state(Port,
			       PPC_THREAD_STATE,
			       (thread_state_t)&State,
			       &StateCount);
    if (MachRet != KERN_SUCCESS) {
      UTIL_SetLastErrorFromMach(MachRet);
      goto EXIT;
    }
    // Copy in the GPRs and the various other control registers
    memcpy(&lpContext->Gpr0, &State.r0, 32*sizeof(unsigned int));
    lpContext->Cr = State.cr;
    lpContext->Xer = State.xer;
    lpContext->Msr = State.srr1;
    lpContext->Iar = State.srr0;
    lpContext->Lr = State.lr;
    lpContext->Ctr = State.ctr;

  }


  if (lpContext->ContextFlags & CONTEXT_FLOATING_POINT) {
    ppc_float_state_t State;

    StateCount = PPC_FLOAT_STATE_COUNT;
    MachRet = thread_get_state(Port,
			       PPC_FLOAT_STATE,
			       (thread_state_t)&State,
			       &StateCount);
    if (MachRet != KERN_SUCCESS) {
      UTIL_SetLastErrorFromMach(MachRet);
      goto EXIT;
    }
    // Copy the FPRs
    memcpy(&lpContext->Fpr0, &State.fpregs, 32*sizeof(double));
    // Mach's pState->fpscr is an integer, but the PAL's is a double
    // so convert carefully.
    State.fpscr_pad = 0;
    lpContext->Fpscr = *(double *)&State.fpscr_pad;

  }
  return TRUE;
EXIT:
  return ret;

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
    DWORD processId;
    BOOL ret = FALSE;
    THREAD *thread;

    if (lpContext == NULL) {
        ERROR("Invalid lpContext parameter value\n");
        SetLastError(ERROR_NOACCESS);
        goto EXIT;
    }
    if (!(processId = THREADGetThreadProcessId(hThread))) {
        ERROR("Couldn't retrieve the process owner of hThread:%p\n", hThread);
        SetLastError(ERROR_INTERNAL_ERROR);
        goto EXIT;
    }

    if (processId != GetCurrentProcessId()) {
        // GetThreadContext() of a thread in another process
        ASSERT("Cross-process GetThreadContext() is not supported\n");
        SetLastError(ERROR_NOACCESS);
        goto EXIT;
    }

    thread = (THREAD *) HMGRLockHandle2(hThread, HOBJ_THREAD);

    if (thread == NULL) {
        ERROR("Failed to lock the thread handle!\n");
        SetLastError(ERROR_ACCESS_DENIED);
        goto EXIT;
    }

    if (thread->dwThreadId != GetCurrentThreadId()) {
        // hThread is in the current process, but isn't the current
        // thread.  Extract the CONTEXT from the Mach thread.
            
        mach_port_t Port;
        Port = pthread_mach_thread_np((pthread_t)thread->dwThreadId);

        HMGRUnlockHandle(hThread, &thread->objHeader);        

        return CONTEXT_GetThreadContextFromPort(Port, lpContext);
    } else {
        HMGRUnlockHandle(hThread, &thread->objHeader);        
        CONTEXT_CaptureContext(lpContext, (lpContext->ContextFlags & CONTEXT_FLOATING_POINT));
        ret = TRUE;
    }
  
EXIT:
    return ret;
}

/*++
Function:
  SetThreadContextOnPort

  Helper for CONTEXT_SetThreadContext
--*/
BOOL
CONTEXT_SetThreadContextOnPort(
           mach_port_t Port,
           IN CONST CONTEXT *lpContext)
{
    BOOL ret = FALSE;
    kern_return_t MachRet;

    if (lpContext->ContextFlags & (CONTEXT_CONTROL|CONTEXT_INTEGER)) {
        ppc_thread_state_t State;

        memcpy(&State.r0, &lpContext->Gpr0, 32*sizeof(unsigned int));
        State.cr = lpContext->Cr;
        State.xer = lpContext->Xer;
        State.srr0 = lpContext->Iar;
        State.srr1 = lpContext->Msr;
        State.lr = lpContext->Lr;
        State.ctr = lpContext->Ctr;

        MachRet = thread_set_state(Port,
                                    PPC_THREAD_STATE,
                                    (thread_state_t)&State,
                                    PPC_THREAD_STATE_COUNT);
        if (MachRet != KERN_SUCCESS) {
            UTIL_SetLastErrorFromMach(MachRet);
            goto EXIT;
        }
    }

    if (lpContext->ContextFlags & CONTEXT_FLOATING_POINT) {
        ppc_float_state_t State;

        memcpy(&State.fpregs, &lpContext->Fpr0, 32*sizeof(double));
        // Mach's State.fpscr is an integer, but the PAL's is a double
        // so convert carefully.
        *(double *)&State.fpscr_pad = lpContext->Fpscr;

        MachRet = thread_set_state(Port,
                                    PPC_FLOAT_STATE,
                                    (thread_state_t)&State,
                                    PPC_FLOAT_STATE_COUNT);
        if (MachRet != KERN_SUCCESS) {
            UTIL_SetLastErrorFromMach(MachRet);
            goto EXIT;
        }

    }
    return TRUE;
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
    DWORD processId;
    BOOL ret = FALSE;
    THREAD *thread;

    if (lpContext == NULL) {
        ERROR("Invalid lpContext parameter value\n");
        SetLastError(ERROR_NOACCESS);
        goto EXIT;
    }
    if (!(processId = THREADGetThreadProcessId(hThread))) {
        ERROR("Couldn't retrieve the process owner of hThread:%p\n", hThread);
        SetLastError(ERROR_INTERNAL_ERROR);
        goto EXIT;
    }

    if (processId != GetCurrentProcessId()) {
        // GetThreadContext() of a thread in another process
        ASSERT("Cross-process GetThreadContext() is not supported\n");
        SetLastError(ERROR_NOACCESS);
        goto EXIT;
    }

    thread = (THREAD *) HMGRLockHandle2(hThread, HOBJ_THREAD);

    if (thread == NULL) {
        ERROR("Failed to lock the thread handle!\n");
        SetLastError(ERROR_ACCESS_DENIED);
        goto EXIT;
    }

    if (thread->dwThreadId != GetCurrentThreadId()) {
        // hThread is in the current process, but isn't the current
        // thread.  Extract the CONTEXT from the Mach thread.

        mach_port_t Port;

        Port = pthread_mach_thread_np((pthread_t)thread->dwThreadId);
        HMGRUnlockHandle(hThread, &thread->objHeader);        
        return CONTEXT_SetThreadContextOnPort(Port, lpContext);
    } else {
        ASSERT("SetThreadContext() of current thread is not supported.\n");
        HMGRUnlockHandle(hThread, &thread->objHeader);        
    }

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
    __asm__ __volatile__("trap");
}

