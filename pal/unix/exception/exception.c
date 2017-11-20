/*++


 Copyright (c) 2002 Microsoft Corporation.  All rights reserved.

 The use and distribution terms for this software are contained in the file
 named license.txt, which can be found in the root of this distribution.
 By using this software in any fashion, you are agreeing to be bound by the
 terms of this license.

 You must not remove this notice, or any other, from this software.


Module Name:

    exception.c

Abstract:

    Implementation of exception API functions.

--*/

#include "pal/palinternal.h"
#include "pal/dbgmsg.h"
#include "pal/thread.h"
#include "pal/critsect.h"
#include "pal/seh.h"
#include "pal/debug.h"
#include "signal.h"
#include "pal/init.h"

#include <errno.h>
#include <string.h>
#if HAVE_MACH_EXCEPTIONS
#include "machexception.h"
#else
#include <signal.h>
#endif
#include <setjmp.h>
#include <unistd.h>
#include <pthread.h>

SET_DEFAULT_DEBUG_CHANNEL(EXCEPT);

/* Constant ant type definitions **********************************************/

/* Bit 28 of exception codes is reserved. */
const UINT RESERVED_SEH_BIT = 0x800000;

/* SEH information structure in TLS */
typedef struct
{
    PPAL_EXCEPTION_REGISTRATION bottom_frame;
    EXCEPTION_RECORD       current_exception;
    BOOL safe_state;
    int signal_code;
} SEH_TLS_INFO;

typedef struct CTRL_HANDLER_LIST
{
    PHANDLER_ROUTINE handler;
    struct CTRL_HANDLER_LIST *next;
} CTRL_HANDLER_LIST;


/* Static variables ***********************************************************/

/* for manipulating process control-handler list, etc */
CRITICAL_SECTION exception_critsec;
static pthread_key_t pSEHInfo;
static LPTOP_LEVEL_EXCEPTION_FILTER pTopFilter;
static CTRL_HANDLER_LIST * pCtrlHandler;

/* Static function declarations ***********************************************/

__inline__ static SEH_TLS_INFO * SEHGetThreadInfo(void);
static SEH_TLS_INFO * SEHCreateThreadInfo(void);
static void SEHUnwind(void);

/* PAL function definitions ***************************************************/

/*++
Function:
  RaiseException

See MSDN doc.
--*/
VOID
PALAPI
RaiseException(
           IN DWORD dwExceptionCode,
           IN DWORD dwExceptionFlags,
           IN DWORD nNumberOfArguments,
           IN CONST ULONG_PTR *lpArguments)
{
    EXCEPTION_RECORD record;
    EXCEPTION_POINTERS exceptionPointers;
    CONTEXT context;

    ENTRY("RaiseException (dwCode=%#x, dwFlags=%#x, nArgs=%u, lpArgs=%p)\n",
          dwExceptionCode, dwExceptionFlags, nNumberOfArguments, lpArguments);

    /* Validate parameters */
    if( dwExceptionCode & RESERVED_SEH_BIT)
    {
        WARN("Exception code %08x has bit 28 set : clearing it.\n",
             dwExceptionCode);
        dwExceptionCode ^= RESERVED_SEH_BIT;
    }

    if( dwExceptionFlags && ( dwExceptionFlags != EXCEPTION_NONCONTINUABLE ) )
    {
        ERROR("Unkown exception flag %08x (ignoring)\n", dwExceptionFlags);
    }

    /* Build exception record */

    TRACE("Building exception record %p\n", &record);

    record.ExceptionCode = dwExceptionCode;
    record.ExceptionAddress = RaiseException;
    record.ExceptionRecord = NULL;

    if( lpArguments != NULL )
    {
        if(nNumberOfArguments>EXCEPTION_MAXIMUM_PARAMETERS)
        {
            WARN("Number of arguments (%d) exceeds the limit "
                 "EXCEPTION_MAXIMUM_PARAMETERS (%d); truncing the extra"
                 "parameters.\n",
                  nNumberOfArguments, EXCEPTION_MAXIMUM_PARAMETERS);

            nNumberOfArguments = EXCEPTION_MAXIMUM_PARAMETERS;
        }

        memcpy(record.ExceptionInformation, lpArguments,
               nNumberOfArguments*sizeof(ULONG_PTR));
    }

    record.NumberParameters = nNumberOfArguments;

    /* from rotor_pal.doc :
       "all explicitly raised exceptions may be considered non-continuable". */
    record.ExceptionFlags = EXCEPTION_NONCONTINUABLE;

    /* from rotor_pal.h : 
      On non-Win32 platforms, the CONTEXT pointer in the
      PEXCEPTION_POINTERS will contain at least the CONTEXT_CONTROL registers.
    */
    ZeroMemory(&context, sizeof context);
    context.ContextFlags = CONTEXT_CONTROL;
    if(!GetThreadContext(GetCurrentThread(),&context))
    {
        WARN("GetThreadContext failed!! ignoring...\n");
    }

    exceptionPointers.ExceptionRecord = &record;
    exceptionPointers.ContextRecord = &context;
    
    /* Let SEHRaiseException do the work */
    SEHRaiseException(&exceptionPointers, 0);

    LOGEXIT("RaiseException returns.\n");
}

LPTOP_LEVEL_EXCEPTION_FILTER
PALAPI
SetUnhandledExceptionFilter(
                IN LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter)
{
    LPTOP_LEVEL_EXCEPTION_FILTER old_filter;

    ENTRY("SetUnhandleExceptionFilter (lpFilter=%p)\n",
          lpTopLevelExceptionFilter);

    /* This load/store combination is not thread-safe.  However,
       the Win32 implementation isn't thread-safe either. */    
    old_filter = pTopFilter;
    pTopFilter = lpTopLevelExceptionFilter;

    LOGEXIT("SetUnhandleExceptionFilter returns LPTOP_LEVEL_EXCEPTION_FILTER "
          "%p\n", old_filter);
    return old_filter;
}


/*++
Function:
  SetConsoleCtrlhandler

See MSDN doc.

--*/
BOOL
PALAPI
SetConsoleCtrlHandler(
              IN PHANDLER_ROUTINE HandlerRoutine,
              IN BOOL Add)
{
    BOOL retval = FALSE;
    CTRL_HANDLER_LIST *handler;

    ENTRY("SetConsoleCtrlHandler(HandlerRoutine=%p, Add=%d\n", 
          HandlerRoutine, Add);

    SYNCEnterCriticalSection(&exception_critsec, TRUE);
    
    if(NULL == HandlerRoutine)
    {
        ASSERT("HandlerRoutine may not be NULL, control-c-ignoration is not "
               "supported\n");
        goto done;
    }


    if(Add)
    {
        handler = (CTRL_HANDLER_LIST *)malloc(sizeof(CTRL_HANDLER_LIST));
        if(!handler)
        {
            ERROR("malloc failed! error is %d (%s)\n", errno, strerror(errno));
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto done;
        }
        handler->handler = HandlerRoutine;
        /* From MSDN : 
           "handler functions are called on a last-registered, first-called 
           basis". So we can add the new handler at the head of the list. */
        handler->next = pCtrlHandler;
        pCtrlHandler = handler;
        
        TRACE("Adding Control Handler %p\n", HandlerRoutine);
        retval = TRUE;
    }
    else
    {
        CTRL_HANDLER_LIST *temp_handler;

        handler = pCtrlHandler;
        temp_handler = handler;
        while(handler)
        {
            if(handler->handler == HandlerRoutine)
            {
                break;
            }
            temp_handler = handler;
            handler = handler->next;
        }
        if(handler)
        {
            /* temp_handler it the item before the one to remove, unless it was
               first in the list, in which case handler == temp_handler */
            if(handler == temp_handler)
            {
                /* handler to remove was first in the list... */
                pCtrlHandler = handler->next;

                free(handler);
                TRACE("Removing Control Handler %p from head of list\n", 
                      HandlerRoutine );
            }
            else
            {
                /* handler was not first in the list... */
                temp_handler->next = handler->next;
                free(handler);
                TRACE("Removing Control Handler %p (not head of list)\n", 
                      HandlerRoutine );                 
            }                 
            retval = TRUE;
        }
        else
        {
            WARN("Trying to remove unknown Control Handler %p\n", 
                 HandlerRoutine);
            SetLastError(ERROR_INVALID_PARAMETER);
        }
    }
done:
    SYNCLeaveCriticalSection(&exception_critsec, TRUE);

    LOGEXIT("SetConsoleCtrlHandler returns BOOL %d\n", retval);
    return retval;
}

/*++
Function:
  GenerateConsoleCtrlEvent

See MSDN doc.

PAL specifics :
    dwProcessGroupId must be zero
              
--*/
BOOL
PALAPI
GenerateConsoleCtrlEvent(
    IN DWORD dwCtrlEvent,
    IN DWORD dwProcessGroupId
    )
{
    int sig;
    BOOL retval = FALSE;

    ENTRY("GenereateConsoleCtrlEvent(dwCtrlEvent=%d, dwProcessGroupId=%#x\n",
        dwCtrlEvent, dwProcessGroupId);

    if(0!=dwProcessGroupId)
    {
        ASSERT("dwProcessGroupId is not 0, this is not supported by the PAL\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }
    switch(dwCtrlEvent)
    {
    case CTRL_C_EVENT :
        sig = SIGINT;
        break;
    case CTRL_BREAK_EVENT:
        /* Map control-break on SIGQUIT */
        sig = SIGQUIT;
        break;
    default:
        TRACE("got unknown control event\n");
        goto done;
    }

    TRACE("sending signal %d to process %d\n", sig, gPID);
    if(-1 == kill(gPID, sig))
    {
        ASSERT("kill() failed; errno is %d (%s)\n",errno, strerror(errno));
        SetLastError(ERROR_INTERNAL_ERROR);
        goto done;
    }
    retval = TRUE;
done:
    LOGEXIT("GenerateConsoleCtrlEvent returns BOOL %d\n",retval);
    return retval;
}



/*++
Function:
  PAL_TryHelper

Abstract:
  Set up an exception frame (call setjmp, complete the exception registration
  structure and add it to thread's list

Parameters :
    PPAL_EXCEPTION_REGISTRATION pRegistration : exception registration
                structure with valid Handler and pvFilterParameter values

(no return value)
--*/
void
PALAPI
PAL_TryHelper(
    IN OUT PPAL_EXCEPTION_REGISTRATION pRegistration)
{
#if !DISABLE_EXCEPTIONS
    PPAL_EXCEPTION_REGISTRATION old_frame;

    if( pRegistration )
    {
        SEH_TLS_INFO *seh_info =pthread_getspecific((DWORD) pSEHInfo);
        if( seh_info == NULL &&  (seh_info=SEHCreateThreadInfo())==NULL)
        {
            ERROR("Cannot get thread info\n");
            return ;
        }
        old_frame = seh_info->bottom_frame;

/*
        The frames' addresses do not have to be in descending order - the optimizing compiler
        can reorder the frames if there are two or more nested handlers in one function.

        if( old_frame && old_frame < pRegistration )
        {
            ASSERT("New exception frame is above previous frame! Stack is "
                  "corrupted!\n");
        }
*/

        pRegistration->Next = old_frame;

        seh_info->bottom_frame = pRegistration;
    }
    else
    {
        ASSERT("Invalid exception registration pointer!\n");
    }

    return;
#endif // !DISABLE_EXCEPTIONS
}


/*++
Function:
  PAL_EndTryHelper

Abstract:
  Clean up the SEH stack when leaving a try block, and jump to the next
  exception frame if we're unwinding

Parameters :
    PPAL_EXCEPTION_REGISTRATION pRegistration : current exception frame
    int ExceptionCode : 0 if no exception occurred, nonzero if an exception
                        occurred and this is a stack unwind

    Always returns 0 that can be used to prevent compiler optimizations
    on setjmp/longjmp
--*/
int
PALAPI
PAL_EndTryHelper(
    IN OUT PPAL_EXCEPTION_REGISTRATION pRegistration,
    IN int ExceptionCode)
{
#if !DISABLE_EXCEPTIONS

    if( ExceptionCode == 0 )
    {
        /* If ExceptionCode is zero, there was no exception, so we
           only need to pop the exception frame. */
        SEH_TLS_INFO *seh_info = pthread_getspecific((DWORD) pSEHInfo);

        if( seh_info == NULL && (seh_info=SEHCreateThreadInfo()) == NULL)
        {
            ERROR("Cannot get thread info\n");
            goto done;
        }
 
        if( (pRegistration == NULL) ||
            (pRegistration != seh_info->bottom_frame) )
        {
            ASSERT("Exception registration pointers don't match!\n");
            goto done;
        }
        seh_info->bottom_frame = pRegistration->Next;
        
    }
    else if( pRegistration->dwFlags & PAL_EXCEPTION_FLAGS_UNWINDTARGET )
    {
        /* Stop unwinding if this is the target frame */
        
        /* If there was an exception, the current frame has already been popped 
           before the longjmp */

        if( (pRegistration == NULL) ||
            (pRegistration->Next != PAL_GetBottommostRegistration()) )
        {                 
            ASSERT("Exception registration pointers don't match!\n");
            goto done;
        }

        TRACE("Exception handled by frame %p. Lowest frame is now %p\n", 
              pRegistration, pRegistration->Next);
    }
    else
    {
        SEH_TLS_INFO *seh_info;

        TRACE("Unwinding in progress; looking for next frame\n");
        
        /* If there was an exception, the current frame has already been popped 
           before the longjmp */

        if( (pRegistration == NULL) ||
            (pRegistration->Next != PAL_GetBottommostRegistration()) )
        {
            ASSERT("Exception registration pointers don't match!\n");
            goto done;
        }
        
        SEHUnwind();
        
        TRACE("Reached top of exception stack! Exception was not "
              "handled, terminating the process.\n");

        if(!PALIsInitialized())
        {
            /* The only way we can get here is if PAL_Terminate is currently 
               running, but we haven't yet restored the default signal handlers. 
               This isn't a problem if an exception handler accepts the 
               exception, but if we reach here,TerminateProcess will have to use 
               the "non-initialized" code path : save status and exit, without 
               cleaning up shared memory etc. Not good, but no choice, so we 
               indicate the problem but proceed anyway. */
            WARN("Exception was raised during PAL termination!\n");
        }

        seh_info = SEHGetThreadInfo();

        /* exit code is exception code */
        if(NULL == seh_info)
        {
            WARN("SEH thread data not available, no exception code available! "
                 "using fake exit code\n");
            TerminateProcess(GetCurrentProcess(), -1);
        }
        TerminateProcess(GetCurrentProcess(),
                         seh_info->current_exception.ExceptionCode);
    }

done:
#endif // !DISABLE_EXCEPTIONS
    return 0;
}

/*++
Function:
  PAL_GetBottommostRegistration

Abstract:
  Return a pointer to the first PAL_EXCEPTION_REGISTRATION structure on the
  current thread's stack.

  This function is highly simplified since it is used heavily.
--*/
PPAL_EXCEPTION_REGISTRATION
PALAPI
PAL_GetBottommostRegistration(
                  VOID)
{
    SEH_TLS_INFO *seh_info;                      

    seh_info = SEHGetThreadInfo();

    if( seh_info == NULL )
    {
        ASSERT("Unable to obtain SEH information for this thread\n");
    }else
    {
        return seh_info->bottom_frame;
    }

    return NULL;
}


/*++
Function:
  PAL_SetBottommostRegistration

Abstract:
  Sets the PPAL_EXCEPTION_REGISTRATION record that is bottommost on the stack  
--*/
VOID
PALAPI
PAL_SetBottommostRegistration(
              PPAL_EXCEPTION_REGISTRATION pRegistration)
{
    SEH_TLS_INFO *seh_info;                      

    seh_info = SEHGetThreadInfo();

    if( seh_info == NULL )
    {
        ASSERT("Unable to obtain SEH information for this thread\n");
    }else
    {
        seh_info->bottom_frame = pRegistration;
    }
}




/*++
Function:
  PAL_GetBottommostRegistrationPtr

Abstract:
  Return the address where the *pointer* to the current bottommost exception 
  frame is stored for the current thread
--*/
PPAL_EXCEPTION_REGISTRATION *
PALAPI
PAL_GetBottommostRegistrationPtr(
              VOID)
{
    SEH_TLS_INFO *seh_info;                      

    ENTRY("PAL_GetBottommostRegistrationPtr () \n");

    seh_info = SEHGetThreadInfo();

    if( seh_info == NULL )
    {
        ASSERT("Unable to obtain SEH information for this thread\n");
        LOGEXIT("PAL_GetBottommostRegistrationPtr returning "
              "PPAL_EXCEPTION_REGISTRATION * 0\n");

        return NULL;
    }

    LOGEXIT("PAL_GetBottommostRegistrationPtr returning "
          "PPAL_EXCEPTION_REGISTRATION *%p\n", &seh_info->bottom_frame);
    return &seh_info->bottom_frame;
}

/* Internal function definitions **********************************************/

/*++
Function :
    SEHInitialize

    Initialize all SEH-related stuff (signals, etc)

    (no parameters)

Return value :
    TRUE  if SEH support initialization succeeded
    FALSE otherwise
--*/
BOOL SEHInitialize (void)
{
    pthread_key_t seh_tls_index;
    int key_retval;
    BOOL bRet = FALSE;

    if (PAL_TRY_LOCAL_SIZE < sizeof(sigjmp_buf))
    {
        ASSERT("PAL_TRY_LOCAL_SIZE does not match sizeof(sigjmp_buf)!\n");
    }

    key_retval = pthread_key_create(&seh_tls_index,NULL);

    if( 0 != key_retval )
    {
        ERROR("Unable to allocate TLS slot for SEH!\n");
        return bRet;
    }

    TRACE("SEH TLS index is %d\n", seh_tls_index);

    pSEHInfo = seh_tls_index;
    pTopFilter = NULL;
    pCtrlHandler = NULL;

    if (0 != SYNCInitializeCriticalSection(&exception_critsec))
    {
        ERROR("Unable to initialize SEH critical section!\n");
        if (pthread_key_delete(seh_tls_index) != 0)
        {
            ERROR("pthread_key_delete failed ...\n");
        }
    }
    else
    {
#if HAVE_MACH_EXCEPTIONS
        if (!SEHInitializeMachExceptions())
        {
            ERROR("SEHInitializeMachExceptions failed!\n");
            SEHCleanup();
        } 
#else
        SEHInitializeSignals();
#endif
        /* force allocation of TLS data for the main thread, so we don't risk a 
           malloc failure *during* exception handling */
        if(!SEHInitThread())
        {
            ERROR("SEHInitThread failed!\n");
            SEHCleanup();
        }
        else
        {
            bRet = TRUE;
        }
    }

    return bRet;
}

/*++
Function :
    SEHCleanup

    Undo work done by SEHInitialize

    (no parameters, no return value)
    
--*/
void SEHCleanup (void)
{
    pthread_key_t seh_tls_index;

    TRACE("Cleaning up SEH\n");

#if HAVE_MACH_EXCEPTIONS
    SEHCleanupExceptionPort();
#else
    SEHCleanupSignals();
#endif
    
    /* release TLS for the last thread standing */
    SEHCleanupThread();
    seh_tls_index = pSEHInfo;
    if (pthread_key_delete(seh_tls_index) != 0)
    {
        ERROR("pthread_key_delete failed ...\n");
    }
    pSEHInfo = 0;
}

/*++
Function :
    SEHInitThread

    Allocate the current thread's SEH_TLS_INFO structure

(no parameters)

Return value :
    TRUE on success, FALSE on failure

--*/
BOOL SEHInitThread(void)
{
    SEH_TLS_INFO *seh_info;

    seh_info = SEHGetThreadInfo();
    if(NULL == seh_info)
    {
        WARN("couldn't allocate SEH TLS info!\n");
        return FALSE;
    }

    return TRUE;
}

/*++
Function :
    SEHCleanupThread

    Release the current thread's SEH_TLS_INFO structure

(no parameters, no return value)
--*/
void SEHCleanupThread(void)
{
    SEH_TLS_INFO *seh_info;

    seh_info = SEHGetThreadInfo();
    free(seh_info);
}   

/*++
Function :
    SEHRaiseException

    Raise an exception given a specified exception information.

Parameters :
    PEXCEPTION_POINTERS lpExceptionPointers : specification of exception
    to raise.
    int signal_code : signal that caused the exception, if applicable; 
                      0 otherwise

(no return value)
--*/
void SEHRaiseException( PEXCEPTION_POINTERS lpExceptionPointers, 
                        int signal_code )
{
    PPAL_EXCEPTION_REGISTRATION frame;
    LPTOP_LEVEL_EXCEPTION_FILTER top_filter;
    LONG default_action;
    SEH_TLS_INFO *seh_info;
    LONG handler_retval;

    if(!lpExceptionPointers)
    {
        ASSERT("Invalid exception record!\n");
        return;
    }

    TRACE("Raising exception %#x (record is %p)\n",
          lpExceptionPointers->ExceptionRecord->ExceptionCode,
          lpExceptionPointers);

    seh_info = SEHGetThreadInfo();
    if(NULL == seh_info)
    {
        ASSERT("Couldn't get SEH thread data!\n");
    }
    else
    {
        /* save copy of exception record for access after longjmp */
        memcpy(&seh_info->current_exception,
               lpExceptionPointers->ExceptionRecord, 
               sizeof seh_info->current_exception);
    }

    seh_info->signal_code = signal_code;
    
    /* Call exception filters until one returns EXCEPTION_EXECUTE_HANDLER */

    TRACE("Looking for exception handler...\n");

    frame = PAL_GetBottommostRegistration();

    while( frame )
    {
        if (frame->Handler == NULL )
        {
            TRACE("Exception frame %p is a try/finally block; skipping.\n",
                  frame);
            frame = frame->Next;
            continue;
        }

        if((LONG_PTR)frame->Handler != EXCEPTION_EXECUTE_HANDLER)
        {
/* reset ENTRY nesting level back to zero while inside the callback... */
#if !_NO_DEBUG_MESSAGES_
            {
            int old_level;
            old_level = DBG_change_entrylevel(0);
#endif /* !_NO_DEBUG_MESSAGES_ */

            /* since we're calling the application handler,
               let's reset to safe state so exception in
               the filter could be handled properly */
            SEHSetSafeState(TRUE);
            
            handler_retval = frame->Handler(lpExceptionPointers, 
                                            frame->pvFilterParameter);

            SEHSetSafeState(FALSE);

/* ...and set nesting level back to what it was */
#if !_NO_DEBUG_MESSAGES_
            DBG_change_entrylevel(old_level);
            }
#endif /* !_NO_DEBUG_MESSAGES_ */
        }
        else
        {
            handler_retval = (LONG)frame->Handler;
        }

        if (frame->dwFlags & PAL_EXCEPTION_FLAGS_UNWINDTARGET)
        {
            ASSERT("PAL_EXCEPTION_FLAGS_UNWINDTARGET is set\n");
        }

        switch(handler_retval)
        {
        case EXCEPTION_EXECUTE_HANDLER:
            frame->dwFlags |= PAL_EXCEPTION_FLAGS_UNWINDTARGET;
            break;
        case EXCEPTION_CONTINUE_SEARCH:
            frame->dwFlags &= ~PAL_EXCEPTION_FLAGS_UNWINDTARGET;
            break;
        case EXCEPTION_CONTINUE_EXECUTION:
            TRACE("Filter returned EXCEPTION_CONTINUE_EXECUTION");
            return;
        default:
            /* Filter has returned an invalid value */
            ASSERT("Filter for frame %p has returned an invalid value!\n",
                  frame);
            break;
        }

        if( frame->dwFlags & PAL_EXCEPTION_FLAGS_UNWINDONLY)
        {
            memcpy(frame->ReservedForPAL, lpExceptionPointers->ExceptionRecord,
                min(sizeof(EXCEPTION_RECORD), PAL_TRY_LOCAL_SIZE));
        }

        if( frame->dwFlags & PAL_EXCEPTION_FLAGS_UNWINDTARGET)
        {
            TRACE("Filter for frame %p has accepted the exception.\n", frame);
            break;
        }
        TRACE("Filter for frame %p has refused the exception.\n", frame);
        frame = frame->Next;
    }

    /* once we get here we can assume the exception stack is (reasonably) 
       valid, so we can allow signals to be handled again */
    SEHSetSafeState(TRUE);

    /* If a handler is found, begin unwinding by longjmp()ing to the first
       termination handler */
    if( frame )
    {
        SEHUnwind();
        ASSERT("SEHUnwind returned, even though an exception handler had "
              "accepted the exception!\n");
    }

    /* No handler found : start default handling */

    TRACE("No handler found for exception. Using default behavior.\n");

    /* Call application-defined top-level filter. */
    top_filter = pTopFilter;

    if(top_filter)
    {
        TRACE("Calling application-defined top-level filter\n");

/* reset ENTRY nesting level back to zero while inside the callback... */
#if !_NO_DEBUG_MESSAGES_
    {
        int old_level;
        old_level = DBG_change_entrylevel(0);
#endif /* !_NO_DEBUG_MESSAGES_ */

        default_action = top_filter(lpExceptionPointers);

/* ...and set nesting level back to what it was */
#if !_NO_DEBUG_MESSAGES_
        DBG_change_entrylevel(old_level);
    }
#endif /* !_NO_DEBUG_MESSAGES_ */

        switch( default_action )
        {
        case EXCEPTION_CONTINUE_SEARCH:
            break;
        case EXCEPTION_EXECUTE_HANDLER:
            break;
        default:
            ASSERT("Application-defined top-level exception filter returned "
                  "invalid value %d\n", default_action);
            default_action = EXCEPTION_CONTINUE_SEARCH;
            break;
        }
    }
    else
    {
        default_action = EXCEPTION_CONTINUE_SEARCH;
    }

    WARN("Exception %08x not handled; process will be terminated.\n",
          lpExceptionPointers->ExceptionRecord->ExceptionCode);

    SEHUnwind();

    /* Exception not handled, no termination handlers to execute : terminate */

    TRACE("Reached top of exception stack! Exception was not handled, "
          "terminating the process.\n");

    if(!PALIsInitialized())
    {
        /* The only way we can get here is if PAL_Terminate is currently 
           running, but we haven't yet restored the default signal handlers. 
           This isn't a problem if an exception handler accepts the exception, 
           but if we reach here, TerminateProcess will have to use the 
           "non-initialized" code path : save status and exit, wihtout cleaning 
           up shared memory etc. Not good, but no choice, so we indicate the 
           problem but proceed anyway. */
        WARN("Exception was raised during PAL termination!\n");
    } 
    else if(0 != signal_code)
    {
        // Terminate unconditionally. The application is not in
        // a safe state.
        PROCCleanupProcess(TRUE);
#if HAVE_MACH_EXCEPTIONS
        SEHCleanupExceptionPort();
#else
        SEHCleanupSignals();
#endif
        /* signal handlers uninstalled : let the signal be raised again */
        return;                                                         
    }                                                                   

    /* exit code is exception code */
    TerminateProcess(GetCurrentProcess(),
                     lpExceptionPointers->ExceptionRecord->ExceptionCode);
    ASSERT("TerminateProcess() returned!\n");
}

/*++
Function :
    SEHHandleControlEvent

    handle Control-C and Control-Break events (call handler routines, 
    notify debugger)

Parameters :
    DWORD event : event that occurred
    LPVOID eip  : instruction pointer when exception occurred                                 

(no return value)

Notes :
    Handlers are called on a last-installed, first called basis, until a
    handler returns TRUE. If no handler returns TRUE (or no hanlder is
    installed), the default behavior is to call TerminateProcess
--*/
void SEHHandleControlEvent(DWORD event, LPVOID eip)
{
    /* handler is actually a copy of the original list */
    CTRL_HANDLER_LIST *handler=NULL, *handlertail, *handlertmp;

    /* second, call handler routines until one handles the event */
    SYNCEnterCriticalSection(&exception_critsec, TRUE);
    handlertmp = pCtrlHandler;

    /* list copying */
    while(NULL!=handlertmp)
    {
        handlertail = (CTRL_HANDLER_LIST *) malloc(sizeof(CTRL_HANDLER_LIST));
        if(!handlertail)
        {
            while(NULL!=handler)
            {
                handlertail=handler;
                handler = handler->next;
                free(handlertail);
            }
            handlertail = NULL;

            SYNCLeaveCriticalSection(&exception_critsec, TRUE);
            
            ERROR("Memory allocation failure!\n");

            return;
        }

        handlertail->handler = handlertmp->handler;

        if(NULL==handler)
        {
            handler = handlertail;
        }

        handlertail = handlertail->next;
        handlertmp = handlertmp->next;
    }

    while(NULL!=handler)
    {
        BOOL handler_retval;

/* reset ENTRY nesting level back to zero while inside the callback... */
#if !_NO_DEBUG_MESSAGES_
    {
        int old_level;
        old_level = DBG_change_entrylevel(0);
#endif /* !_NO_DEBUG_MESSAGES_ */

        handler_retval = handler->handler(event);

/* ...and set nesting level back to what it was */
#if !_NO_DEBUG_MESSAGES_
        DBG_change_entrylevel(old_level);
    }
#endif /* !_NO_DEBUG_MESSAGES_ */

        if(handler_retval)
        {
            TRACE("Console Control handler %p has handled event\n",
                  handler->handler);
            break;
        }
        handler = handler->next;
    }

    /* store the handler that dispatched */
    handlertmp=handler;

    /* free handle list copy */
    while(NULL!=handler)
    {
        handlertail=handler;
        handler = handler->next;
        free(handlertail);
    }
    handlertail = NULL;

    SYNCLeaveCriticalSection(&exception_critsec, TRUE);
    if(NULL == handlertmp)
    {
        if(CTRL_C_EVENT == event)
        {
            TRACE("Control-C not handled; terminating.\n");
        }
        else
        {
            TRACE("Control-Break not handled; terminating.\n");
        }
        /* tested in Win32 : this is the exit code when ^C and ^Break are not 
           handled */
        TerminateProcess(GetCurrentProcess(),CONTROL_C_EXIT);
    }
}

/*++
Function :
    SEHSetSafeState

    specify whether the current thread is in a state where exception handling 
    of signals can be done safely

Parameters:
    BOOL state : TRUE if the thread is safe, FALSE otherwise

(no return value)
--*/
void SEHSetSafeState(BOOL state)
{
    SEH_TLS_INFO *seh_info;

    seh_info = SEHGetThreadInfo();
    if(NULL == seh_info)
    {
        ASSERT("couldn't retrieve SEH TLS structure!\n");
        return;
    }
    seh_info->safe_state = state;
}

/*++
Function :
    SEHGetSafeState

    determine whether the current thread is in a state where exception handling 
    of signals can be done safely

    (no parameters)

Return value :
    TRUE if the thread is in a safe state, FALSE otherwise
--*/
BOOL SEHGetSafeState(void)
{
    SEH_TLS_INFO *seh_info;

    seh_info = SEHGetThreadInfo();
    if(NULL == seh_info)
    {
        ASSERT("couldn't retrieve SEH TLS structure!\n");
        return FALSE;
    }
    return seh_info->safe_state;
}


/* Static function definitions ************************************************/

/*++
Function :
    SEHGetThreadInfo

    Get thread information from thread-local storage

    (no parameters)

Return value :
    Pointer to filter function, or NULL if no filter was set
    (or in case of error).
--*/
static SEH_TLS_INFO *SEHGetThreadInfo(void)
{
    SEH_TLS_INFO * seh_info;
    seh_info=pthread_getspecific((DWORD) pSEHInfo);
    if(seh_info == NULL)
    {
        seh_info=SEHCreateThreadInfo();
    }    
    return seh_info;
}
/*++
Function :
    SEHCreateThreadInfo

    Create thread-local storage

    (no parameters)

Return value :
    Pointer to filter function, or NULL if no filter was set
    (or in case of error).
--*/
static SEH_TLS_INFO *SEHCreateThreadInfo(void)
{

    SEH_TLS_INFO *seh_info;

    DWORD seh_tls_index;
    int ret;
    seh_tls_index = (DWORD) pSEHInfo;

    TRACE("Need to initialize SEH information for this thread.\n");

    seh_info = (SEH_TLS_INFO *) malloc(sizeof(SEH_TLS_INFO));
    if(!seh_info)
    {
        ERROR("Memory allocation failure!\n");
        return NULL;
    }
    TRACE("Created SEH information structure %p for this thread\n",
              seh_info);
        
    if ((ret = pthread_setspecific(seh_tls_index, seh_info)) != 0)
    {
        ERROR("pthread_setspecific() failed error:%d (%s)\n",
                   ret, strerror(ret));
        free(seh_info);
        return NULL;
    }

    seh_info->bottom_frame = NULL;
    seh_info->safe_state = TRUE;

    return seh_info;
}

static void SEHUnwind()
{
    PAL_EXCEPTION_REGISTRATION *jmp_frame;
    EXCEPTION_POINTERS ep;
    LONG retval;
    SEH_TLS_INFO *seh_info;

    jmp_frame = PAL_GetBottommostRegistration();
    while(NULL != jmp_frame)
    {
        /* finally blocks have no filter */
        if( jmp_frame->Handler == NULL )
        {
            TRACE("Found termination handler frame (%p)\n", jmp_frame);
            break;
        }

        if(jmp_frame->dwFlags & PAL_EXCEPTION_FLAGS_UNWINDONLY)
        {
            jmp_frame->dwFlags &= ~PAL_EXCEPTION_FLAGS_UNWINDTARGET;

            PAL_SetBottommostRegistration(jmp_frame);

            ep.ExceptionRecord = (PEXCEPTION_RECORD) jmp_frame->ReservedForPAL;
            ep.ExceptionRecord->ExceptionFlags |= EXCEPTION_UNWINDING;
            ep.ContextRecord = NULL;

/* reset ENTRY nesting level back to zero while inside the callback... */
#if !_NO_DEBUG_MESSAGES_
            {
                int old_level;
                old_level = DBG_change_entrylevel(0);
#endif /* !_NO_DEBUG_MESSAGES_ */

                retval = jmp_frame->Handler(&ep, 
                                            jmp_frame->pvFilterParameter);
                
/* ...and set nesting level back to what it was, (if callback returned) */
#if !_NO_DEBUG_MESSAGES_
                DBG_change_entrylevel(old_level);
            }
#endif /* !_NO_DEBUG_MESSAGES_ */

            if(EXCEPTION_CONTINUE_SEARCH!=retval)
            {
                ASSERT("Win32 EH filter returned %d instead of "
                       "EXCEPTION_CONTINUE_SEARCH (%d)!\n", 
                       retval, EXCEPTION_CONTINUE_SEARCH);
                break; /* because what else can we do? */
            }

            if (jmp_frame->dwFlags & PAL_EXCEPTION_FLAGS_UNWINDTARGET)
            {
                ASSERT("PAL_EXCEPTION_FLAGS_UNWINDTARGET is set\n");
            }
        }           
        else if( jmp_frame->dwFlags & PAL_EXCEPTION_FLAGS_UNWINDTARGET )
        {
            TRACE("Found unwind target frame (%p)\n", jmp_frame);
            break;
        }

        jmp_frame = jmp_frame->Next;
    }
            
    if (jmp_frame)
    {
        TRACE("Jumping to exception frame %p\n", jmp_frame);

        /* remove the target frame from the chain : if there's an exception 
            within the handler, we don't want it to be sent back to the same
            handler */
        PAL_SetBottommostRegistration(jmp_frame->Next);

/* reset ENTRY nesting level back to zero before longjmping */
#if !_NO_DEBUG_MESSAGES_
        {
            int old_level;
            old_level = DBG_change_entrylevel(0);
#endif /* !_NO_DEBUG_MESSAGES_ */
                
            siglongjmp((LPVOID)jmp_frame->ReservedForPAL,1);

#if !_NO_DEBUG_MESSAGES_
        }
#endif /* !_NO_DEBUG_MESSAGES_ */
    }

    PAL_SetBottommostRegistration(NULL);

    seh_info = SEHGetThreadInfo();

    /* exit code is exception code */
    if(NULL == seh_info)
    {
        WARN("SEH thread data not available, no exception code available! "
             "using fake exit code\n");
        TerminateProcess(GetCurrentProcess(),-1);
    }
    if(0 == seh_info->signal_code)
    {   
        TerminateProcess(GetCurrentProcess(),
                         seh_info->current_exception.ExceptionCode);
    }
    else
    {
        int signal_code;

        TRACE("unhandled signal : shutting down the PAL and re-raising the "
              "signal\n");
        signal_code = seh_info->signal_code;
        // Terminate unconditionally. The application is not in a
        // safe state.
        PROCCleanupProcess(TRUE);
#if HAVE_MACH_EXCEPTIONS
        SEHCleanupExceptionPort();
#else
        SEHCleanupSignals();    
#endif

        /* signal handlers now uninstalled : raise the signal again */
        kill(gPID, signal_code);
    }
}
