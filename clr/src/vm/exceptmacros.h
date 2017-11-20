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
// EXCEPTMACROS.H -
//
// This header file exposes mechanisms to:
//
//    1. Throw COM+ exceptions using the COMPlusThrow() function
//    2. Guard a block of code using COMPLUS_TRY, and catch
//       COM+ exceptions using COMPLUS_CATCH
//
// from the *unmanaged* portions of the EE. Much of the EE runs
// in a hybrid state where it runs like managed code but the code
// is produced by a classic unmanaged-code C++ compiler.
//
// THROWING A COM+ EXCEPTION
// -------------------------
// To throw a COM+ exception, call the function:
//
//      COMPlusThrow(OBJECTREF pThrowable);
//
// This function does not return. There are also various functions
// that wrap COMPlusThrow for convenience.
//
// COMPlusThrow() must only be called within the scope of a COMPLUS_TRY
// block. See below for more information.
//
//
// THROWING A RUNTIME EXCEPTION
// ----------------------------
// COMPlusThrow() is overloaded to take a constant describing
// the common EE-generated exceptions, e.g.
//
//    COMPlusThrow(kOutOfMemoryException);
//
// See rexcep.h for list of constants (prepend "k" to get the actual
// constant name.)
//
// You can also add a descriptive error string as follows:
//
//    - Add a descriptive error string and resource id to
//      COM99\src\dlls\mscorrc\resource.h and mscorrc.rc.
//      Embed "%1", "%2" or "%3" to leave room for runtime string
//      inserts.
//
//    - Pass the resource ID and inserts to COMPlusThrow, i.e.
//
//      COMPlusThrow(kSecurityException,
//                   IDS_CANTREFORMATCDRIVEBECAUSE,
//                   L"Formatting C drive permissions not granted.");
//
//
//
// TO CATCH COMPLUS EXCEPTIONS:
// ----------------------------
//
// Use the following syntax:
//
//      #include "exceptmacros.h"
//
//
//      OBJECTREF pThrownObject;
//
//      COMPLUS_TRY {
//          ...guarded code...
//      } COMPLUS_CATCH {
//          ...handler...
//      } COMPLUS_END_CATCH
//
//
// COMPLUS_TRY has a variant:
//
//      Thread *pCurThread = GetThread();
//      COMPLUS_TRYEX(pCurThread);
//
// The only difference is that COMPLUS_TRYEX requires you to give it
// the current Thread structure. If your code already has a pointer
// to the current Thread for other purposes, it's more efficient to
// call COMPLUS_TRYEX (COMPLUS_TRY generates a second call to GetThread()).
//
// COMPLUS_TRY blocks can be nested. 
//
// From within the handler, you can call the GETTHROWABLE() macro to
// obtain the object that was thrown.
//
// CRUCIAL POINTS
// --------------
// In order to call COMPlusThrow(), you *must* be within the scope
// of a COMPLUS_TRY block. Under _DEBUG, COMPlusThrow() will assert
// if you call it out of scope. This implies that just about every
// external entrypoint into the EE has to have a COMPLUS_TRY, in order
// to convert uncaught COM+ exceptions into some error mechanism
// more understandable to its non-COM+ caller.
//
// Any function that can throw a COM+ exception out to its caller
// has the same requirement. ALL such functions should be tagged
// with the following macro statement:
//
//      THROWSCOMPLUSEXCEPTION();
//
// at the start of the function body. Aside from making the code
// self-document its contract, the checked version of this will fire
// an assert if the function is ever called without being in scope.
//
//
// AVOIDING COMPLUS_TRY GOTCHAS
// ----------------------------
// COMPLUS_TRY/COMPLUS_CATCH actually expands into a Win32 SEH
// __try/__except structure. It does a lot of goo under the covers
// to deal with pre-emptive GC settings.
//
//    1. Do not use C++ or SEH try/__try use COMPLUS_TRY instead.
//
//    2. Remember that any function marked THROWSCOMPLUSEXCEPTION()
//       has the potential not to return. So be wary of allocating
//       non-gc'd objects around such calls because ensuring cleanup
//       of these things is not simple (you can wrap another COMPLUS_TRY
//       around the call to simulate a COM+ "try-finally" but COMPLUS_TRY
//       is relatively expensive compared to the real thing.)
//
//

#ifndef __exceptmacros_h__
#define __exceptmacros_h__

struct _EXCEPTION_REGISTRATION_RECORD;
class Thread;
VOID RealCOMPlusThrowOM();

#include <excepcpu.h>

// Forward declaration used in COMPLUS_CATCHEX
void Profiler_ExceptionCLRCatcherExecute();


//==========================================================================
// Macros to allow catching exceptions from within the EE. These are lightweight
// handlers that do not install the managed frame handler.
//
//      EE_TRY_FOR_FINALLY {
//          ...<guarded code>...
//      } EE_FINALLY {
//          ...<handler>...
//      } EE_END_FINALLY
//
//      EE_TRY(filter expr) {
//          ...<guarded code>...
//      } EE_CATCH {
//          ...<handler>...
//      }
//==========================================================================

// __GotException will only be FALSE if got all the way through the code
// guarded by the try, otherwise it will be TRUE, so we know if we got into the
// finally from an exception or not. In which case need to reset the GC state back
// to what it was for the finally to run in that state and then disable it again
// for return to OS. If got an exception, preemptive GC should always be enabled


#define EE_TRY_FOR_FINALLY                                          \
    {                                                               \
        Thread* ___pCurThread = GetThread();                        \
        BOOL __fGCDisabled = ___pCurThread->PreemptiveGCDisabled(); \
        BOOL __fGCDisabled2 = FALSE;                                \
        BOOL __GotException = TRUE;                                 \
        PAL_TRY {                                                   \
            EXCEPTION_REGISTRATION_RECORD __record_finally;         \
            __record_finally.Handler = COMPlusFinallyFilter;        \
            __record_finally.pvFilterParameter = ___pCurThread->GetFrame(); \
            __record_finally.dwFlags = PAL_EXCEPTION_FLAGS_UNWINDONLY; \
            PAL_TryHelper(&__record_finally);                       \
            {

#define GOT_EXCEPTION() __GotException

#define EE_LEAVE             \
    goto __ee_finally;

#define EE_FINALLY                                                \
        }                                                         \
    goto __ee_finally;                                            \
    __ee_finally:                                                 \
        __GotException = FALSE;                                   \
        PAL_EndTryHelper(&__record_finally, 0);                   \
    } PAL_FINALLY_EX(ee_finally) {                                \
        if (__GotException) {                                     \
            __fGCDisabled2 = ___pCurThread->PreemptiveGCDisabled(); \
            if (! __fGCDisabled) {                                \
                if (__fGCDisabled2)                               \
                    ___pCurThread->EnablePreemptiveGC();          \
            } else if (! __fGCDisabled2) {                        \
                ___pCurThread->DisablePreemptiveGC();             \
            }                                                     \
        }

#define EE_END_FINALLY                                            \
        if (__GotException) {                                     \
            if (! __fGCDisabled) {                                \
                if (__fGCDisabled2) {                             \
                    ___pCurThread->DisablePreemptiveGC();         \
                }                                                 \
            } else if (! __fGCDisabled2)                          \
                ___pCurThread->EnablePreemptiveGC();              \
        }                                                         \
    }                                                             \
    PAL_ENDTRY                                                    \
    }


//==========================================================================
// Macros to allow catching COM+ exceptions from within unmanaged code:
//
//      COMPLUS_TRY {
//          ...<guarded code>...
//      } COMPLUS_CATCH {
//          ...<handler>...
//      }
//==========================================================================

#define COMPLUS_CATCH_GCCHECK()                                                \
    if (___fGCDisabled && ! ___pCurThread->PreemptiveGCDisabled())             \
        ___pCurThread->DisablePreemptiveGC();                                  

#define SAVE_HANDLER_TYPE()                                               \
        BOOL ___ExInUnmanagedHandler = ___pExInfo->IsInUnmanagedHandler(); \
        ___pExInfo->ResetIsInUnmanagedHandler();
    
#define RESTORE_HANDLER_TYPE()                    \
    if (___ExInUnmanagedHandler) {              \
        ___pExInfo->SetIsInUnmanagedHandler();  \
    }

#define COMPLUS_CATCH_NEVER_CATCH -1
#define COMPLUS_CATCH_CHECK_CATCH 0
#define COMPLUS_CATCH_ALWAYS_CATCH 1

#define COMPLUS_TRY  COMPLUS_TRYEX(GetThread())

#ifdef _DEBUG
#define DEBUG_CATCH_DEPTH_SAVE(pCurThread, pData) \
    (pData)->oldCatchDepth = pCurThread->m_ComPlusCatchDepth;        \
    pCurThread->m_ComPlusCatchDepth = (pData)->limit;                       
#else
#define DEBUG_CATCH_DEPTH_SAVE(pCurThread, pData)
#endif

#ifdef _DEBUG
#define DEBUG_CATCH_DEPTH_RESTORE(pCurThread, pData) \
    pCurThread->m_ComPlusCatchDepth = (pData)->oldCatchDepth;
#else
#define DEBUG_CATCH_DEPTH_RESTORE(pCurThread, pData)
#endif


#define COMPLUS_TRYEX(/* Thread* */ pCurThread)                               \
    {                                                                         \
    Thread* ___pCurThread = (pCurThread);                                     \
    _ASSERTE(___pCurThread);                                                  \
    BOOL ___fGCDisabled = ___pCurThread->PreemptiveGCDisabled();              \
    ComPlusFilterData __data;                                                 \
    __data.pFrame = ___pCurThread->GetFrame();                                \
    __data.filterResult = -2;  /* An invalid value to mark non-exception path */ \
    __data.exception_unwind = 0;  /* An invalid value to mark non-exception path */ \
    __data.fCatchFlag = COMPLUS_CATCH_CHECK_CATCH;                            \
    __data.limit = GetSP();                                                   \
    ExInfo* ___pExInfo;                                                       \
    ___pExInfo = ___pCurThread->GetHandlerInfo();                             \
    DEBUG_CATCH_DEPTH_SAVE(___pCurThread, &__data);                           \
    PAL_TRY /* complus_try */ {                                               \
        EXCEPTION_REGISTRATION_RECORD __record_catch;                         \
        __record_catch.Handler = COMPlusCatchFilter;                          \
        __record_catch.pvFilterParameter = &__data;                           \
        __record_catch.dwFlags = PAL_EXCEPTION_FLAGS_UNWINDONLY;              \
        PAL_TryHelper(&__record_catch);                                       \
        {                                                                     \
             DEBUG_SAFE_TO_THROW_IN_THIS_BLOCK

                // <TRY-BLOCK> //

#define COMPLUS_CATCH_PROLOG                                        \
        }                                                           \
    goto __complus_try;                                             \
    __complus_try:                                                  \
        PAL_EndTryHelper(&__record_catch, 0);                       \
        DEBUG_CATCH_DEPTH_RESTORE(___pCurThread, &__data);          \
        COMPLUS_CATCH_GCCHECK();

// This complus passes bool indicating if should catch
#define COMPLUS_CATCH                                                           \
    COMPLUS_CATCH_PROLOG                                                        \
    } PAL_EXCEPT_FILTER_EX(complus_try, COMPlusFilter, &__data){                \
        COMPLUS_CATCH_GCCHECK();                                                \
        ___pExInfo->m_pBottomMostHandler = NULL;                                \
        Profiler_ExceptionCLRCatcherExecute();                                  \
        {

                // <CATCH-BLOCK> //

#define COMPLUS_END_CATCH                                                       \
        }                                                                       \
        bool fToggle = false;                                                   \
        if (!___pCurThread->PreemptiveGCDisabled())  {                          \
            fToggle = true;                                                     \
            ___pCurThread->DisablePreemptiveGC();                               \
        }                                                                       \
                                                                                \
        UnwindExInfo(___pExInfo, __data.limit);                                 \
                                                                                \
        if (fToggle) {                                                          \
            ___pCurThread->EnablePreemptiveGC();                                \
        }                                                                       \
    } PAL_ENDTRY /* complus_try */                                              \
    }

#define COMPLUS_FINALLY                                             \
        }                                                           \
    goto __complus_try;                                             \
    __complus_try:                                                  \
        PAL_EndTryHelper(&__record_catch, 0);                       \
    } PAL_FINALLY_EX(complus_try) {                                     \
        BOOL ___fGCDisabled2 = ___pCurThread->PreemptiveGCDisabled();   \
        if (__data.exception_unwind) {                                  \
            if (! ___fGCDisabled) {                                     \
                if (___fGCDisabled2) {                                  \
                    ___pCurThread->EnablePreemptiveGC();                \
                }                                                       \
            } else if (! ___fGCDisabled2) {                             \
                ___pCurThread->DisablePreemptiveGC();                   \
            }                                                           \
        }


#define COMPLUS_END_FINALLY                               \
        if (__data.exception_unwind) {                    \
            if (! ___fGCDisabled) {                       \
                if (___fGCDisabled2) {                    \
                    ___pCurThread->DisablePreemptiveGC(); \
                }                                         \
            } else if (! ___fGCDisabled2) {               \
                ___pCurThread->EnablePreemptiveGC();      \
            }                                             \
        }                                                 \
    } PAL_ENDTRY /* complus_try */                        \
    }

#define COMPLUS_LEAVE                                     \
    goto __complus_try


#define COMPLUS_CATCH_FLAG(flag)                          \
    __data.fCatchFlag = (flag)

#define COMPLUS_END_CATCH_MIGHT_RETHROW    COMPLUS_END_CATCH
#define COMPLUS_END_CATCH_NO_RETHROW       COMPLUS_END_CATCH

#define GETTHROWABLE()              (GetThread()->GetThrowable())
#define SETTHROWABLE(obj)           (GetThread()->SetThrowable(obj))



#define _EXCEPTION_HANDLER_DECL(funcname) \
    EXCEPTION_DISPOSITION __cdecl _##funcname(EXCEPTION_RECORD *pExceptionRecord, \
                         EXCEPTION_REGISTRATION_RECORD *pEstablisherFrame, \
                         CONTEXT *pContext, \
                         void *pDispatcherContext)

#define EXCEPTION_HANDLER_DECL(funcname) \
    LONG funcname(EXCEPTION_POINTERS *pExceptionPointers, PVOID pv); \
    _EXCEPTION_HANDLER_DECL(funcname)

#define EXCEPTION_HANDLER_IMPL(funcname) \
    _EXCEPTION_HANDLER_DECL(funcname); \
    \
    LONG funcname(EXCEPTION_POINTERS *pExceptionPointers, \
                         PVOID pv) \
        { return _##funcname(pExceptionPointers->ExceptionRecord, \
            (EXCEPTION_REGISTRATION_RECORD*)pv, \
            pExceptionPointers->ContextRecord, \
            NULL); } \
    \
    _EXCEPTION_HANDLER_DECL(funcname)

#define EXCEPTION_HANDLER_FWD(funcname) \
    _##funcname(pExceptionRecord, pEstablisherFrame, pContext, pDispatcherContext)


//================================================
// Declares a COM+ frame handler that can be used to make sure that
// exceptions that should be handled from within managed code 
// are handled within and don't leak out to give other handlers a 
// chance at them.
//=================================================== 
#define INSTALL_COMPLUS_EXCEPTION_HANDLER() INSTALL_COMPLUS_EXCEPTION_HANDLEREX(GetThread())
#define INSTALL_COMPLUS_EXCEPTION_HANDLEREX(pCurThread) \
  {                                            \
    Thread* ___pCurThread = (pCurThread);      \
    _ASSERTE(___pCurThread);                   \
    COMPLUS_TRY_DECLARE_EH_RECORD();           \
    InsertCOMPlusFrameHandler(___pExRecord);    

#define UNINSTALL_COMPLUS_EXCEPTION_HANDLER()  \
    RemoveCOMPlusFrameHandler(___pExRecord);    \
  }

#define INSTALL_NESTED_EXCEPTION_HANDLER(frame)                           \
   NestedHandlerExRecord *__pNestedHandlerExRecord = (NestedHandlerExRecord*) _alloca(sizeof(NestedHandlerExRecord));       \
   __pNestedHandlerExRecord->m_handlerInfo.m_pThrowable = NULL;               \
   __pNestedHandlerExRecord->Init(0, (PEXCEPTION_ROUTINE)COMPlusNestedExceptionHandler, frame);   \
   InsertCOMPlusFrameHandler(__pNestedHandlerExRecord);

#define UNINSTALL_NESTED_EXCEPTION_HANDLER()    \
   RemoveCOMPlusFrameHandler(__pNestedHandlerExRecord);

class Frame;


#define THROWSCOMPLUSEXCEPTION()

#define CANNOTTHROWCOMPLUSEXCEPTION()
#define BEGINCANNOTTHROWCOMPLUSEXCEPTION()
#define ENDCANNOTTHROWCOMPLUSEXCEPTION()

#define DEBUG_SAFE_TO_THROW_IN_THIS_BLOCK
#define DEBUG_SAFE_TO_THROW_BEGIN
#define DEBUG_SAFE_TO_THROW_END

#define COMPlusThrow                    RealCOMPlusThrow
#define COMPlusThrowNonLocalized        RealCOMPlusThrowNonLocalized
#define COMPlusThrowHR                  RealCOMPlusThrowHR
#define COMPlusThrowWin32               RealCOMPlusThrowWin32
#define COMPlusThrowOM                  RealCOMPlusThrowOM
#define COMPlusThrowArithmetic          RealCOMPlusThrowArithmetic
#define COMPlusThrowArgumentNull        RealCOMPlusThrowArgumentNull
#define COMPlusThrowArgumentOutOfRange  RealCOMPlusThrowArgumentOutOfRange
#define COMPlusThrowMissingMethod       RealCOMPlusThrowMissingMethod
#define COMPlusThrowMember              RealCOMPlusThrowMember
#define COMPlusThrowArgumentException   RealCOMPlusThrowArgumentException
#define COMPlusRareRethrow              RealCOMPlusRareRethrow


//======================================================
// Used when we're entering the EE from unmanaged code
// and we can assert that the gc state is cooperative.
//
// If an exception is thrown through this transition
// handler, it will clean up the EE appropriately.  See
// the definition of COMPlusCooperativeTransitionHandler
// for the details.
//======================================================
EXCEPTION_HANDLER_DECL(COMPlusCooperativeTransitionHandler);

#define COOPERATIVE_TRANSITION_BEGIN() COOPERATIVE_TRANSITION_BEGIN_EX(GetThread())

#define COOPERATIVE_TRANSITION_BEGIN_EX(pThread)                          \
  {                                                                       \
    _ASSERTE(GetThread() && GetThread()->PreemptiveGCDisabled() == TRUE); \
    DEBUG_ASSURE_NO_RETURN_IN_THIS_BLOCK                                  \
    Frame *__pFrame = pThread->m_pFrame;                                  \
    INSTALL_FRAME_HANDLING_FUNCTION(COMPlusCooperativeTransitionHandler, __pFrame)

#define COOPERATIVE_TRANSITION_END()                                      \
    UNINSTALL_FRAME_HANDLING_FUNCTION                                     \
  }

extern LONG UserBreakpointFilter(EXCEPTION_POINTERS *ep);
extern LONG DefaultCatchFilter(EXCEPTION_POINTERS *ep, LPVOID pv);

// the only valid parameter for DefaultCatchFilter
#define COMPLUS_EXCEPTION_EXECUTE_HANDLER   (PVOID)EXCEPTION_EXECUTE_HANDLER

#endif // __exceptmacros_h__
