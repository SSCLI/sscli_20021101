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
// EXCEPX86.H -
//
// This header file is optionally included from Excep.h if the target platform is x86
//

#ifndef __excepx86_h__
#define __excepx86_h__

#include "corerror.h"  // HResults for the COM+ Runtime

#include "../dlls/mscorrc/resource.h"

class Thread;


#define INSTALL_EXCEPTION_HANDLING_RECORD(record)               \
    {                                                           \
        PEXCEPTION_REGISTRATION_RECORD __record = (record);     \
        __record->pvFilterParameter = __record;                 \
        __record->dwFlags = PAL_EXCEPTION_FLAGS_UNWINDONLY;     \
        PAL_TryHelper(__record);                                \
    }

#define UNINSTALL_EXCEPTION_HANDLING_RECORD(record)             \
    {                                                           \
        PEXCEPTION_REGISTRATION_RECORD __record = (record);     \
        PAL_EndTryHelper(__record, 0);                          \
    }

#define INSTALL_FRAME_HANDLING_FUNCTION(handler, frame_addr)          \
    {                                                                 \
    FrameHandlerExRecord *___pExRecord = (FrameHandlerExRecord *)_alloca(sizeof(FrameHandlerExRecord)); \
    ___pExRecord->m_ExReg.Handler = (PEXCEPTION_ROUTINE)(handler);    \
    ___pExRecord->m_pEntryFrame = (frame_addr);                       \
    INSTALL_EXCEPTION_HANDLING_RECORD(&((___pExRecord)->m_ExReg));

#define UNINSTALL_FRAME_HANDLING_FUNCTION                             \
    UNINSTALL_EXCEPTION_HANDLING_RECORD(&((___pExRecord)->m_ExReg));  \
    }



// stackOverwriteBarrier is used to detect overwriting of stack which will mess up handler registration
#if defined(_DEBUG)
#define COMPLUS_TRY_DECLARE_EH_RECORD() \
    FrameHandlerExRecordWithBarrier *___pExRecordWithBarrier = (FrameHandlerExRecordWithBarrier *)_alloca(sizeof(FrameHandlerExRecordWithBarrier)); \
    for (int ___i =0; ___i < STACK_OVERWRITE_BARRIER_SIZE; ___i++) \
        ___pExRecordWithBarrier->m_StackOverwriteBarrier[___i] = STACK_OVERWRITE_BARRIER_VALUE; \
    FrameHandlerExRecord *___pExRecord = &(___pExRecordWithBarrier->m_ExRecord); \
    ___pExRecord->m_ExReg.Handler = (PEXCEPTION_ROUTINE)COMPlusFrameHandler; \
    ___pExRecord->m_pEntryFrame = ___pCurThread->GetFrame();

#else
#define COMPLUS_TRY_DECLARE_EH_RECORD() \
    FrameHandlerExRecord *___pExRecord = (FrameHandlerExRecord *)_alloca(sizeof(FrameHandlerExRecord)); \
    ___pExRecord->m_ExReg.Handler = (PEXCEPTION_ROUTINE)COMPlusFrameHandler; \
    ___pExRecord->m_pEntryFrame = ___pCurThread->GetFrame();

#endif

// Insert a handler that will catch any exceptions prior to the COMPLUS_TRY handler and attempt
// to find a user-level handler first. In debug build, actualRecord will skip past stack overwrite
// barrier and in retail build it will be same as exRecord.
#define InsertCOMPlusFrameHandler(pExRecord)                                    \
    INSTALL_EXCEPTION_HANDLING_RECORD(&((pExRecord)->m_ExReg)); /* skip overwrite barrier */

// Remove the handler from the list.
#define RemoveCOMPlusFrameHandler(pExRecord)                                    \
    UNINSTALL_EXCEPTION_HANDLING_RECORD(&((pExRecord)->m_ExReg)); /* skip overwrite barrier */

PEXCEPTION_REGISTRATION_RECORD GetCurrentSEHRecord();
PEXCEPTION_REGISTRATION_RECORD GetFirstCOMPlusSEHRecord(Thread*);

#ifndef EXCEPTION_CHAIN_END
#define EXCEPTION_CHAIN_END ((PEXCEPTION_REGISTRATION_RECORD)-1)
#endif


#endif // __excepx86_h__
