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
// CGENSYS.H -
//
// Generic header for choosing system-dependent helpers
//
// 

#ifndef __cgensys_h__
#define __cgensys_h__

class MethodDesc;
class Stub;
class PrestubMethodFrame;
class Thread;
class NDirectMethodFrame;
class PInvokeCalliFrame;
class CallSig;
class IFrameState;
class CrawlFrame;
struct EE_ILEXCEPTION_CLAUSE;


#include <cgencpu.h>


void ResumeAtJitEH   (CrawlFrame* pCf, BYTE* startPC, EE_ILEXCEPTION_CLAUSE *EHClausePtr, DWORD nestingLevel, Thread *pThread, BOOL unwindStack);
int  CallJitEHFilter (CrawlFrame* pCf, BYTE* startPC, EE_ILEXCEPTION_CLAUSE *EHClausePtr, DWORD nestingLevel, OBJECTREF thrownObj);
void CallJitEHFinally(CrawlFrame* pCf, BYTE* startPC, EE_ILEXCEPTION_CLAUSE *EHClausePtr, DWORD nestingLevel);

// Trivial wrapper designed to get around VC's restrictions regarding
// COMPLUS_TRY & object unwinding.
inline CPUSTUBLINKER *NewCPUSTUBLINKER()
{
    return new CPUSTUBLINKER();
}




#endif

