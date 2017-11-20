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
/*============================================================
**
** Header: COMSystem.h
**       
**
** Purpose: Native methods on System.System
**
** Date:  March 30, 1998
**
===========================================================*/

#ifndef _COMSYSTEM_H
#define _COMSYSTEM_H

#include "fcall.h"

// Return values for CanAssignArrayType
enum AssignArrayEnum {
    AssignWrongType,
    AssignWillWork,
    AssignMustCast,
    AssignBoxValueClassOrPrimitive,
    AssignUnboxValueClassAndCast,
    AssignPrimitiveWiden,
};


//
// Each function that we call through native only gets one argument,
// which is actually a pointer to it's stack of arguments.  Our structs
// for accessing these are defined below.
//

class SystemNative
{
    friend class DebugStackTrace;

public:
    struct StackTraceElement {
        SLOT ip;
        DWORD sp;
        MethodDesc *pFunc;
    };

private:
    struct CaptureStackTraceData
    {
        // Used for the integer-skip version
        INT32   skip;

        INT32   cElementsAllocated;
        INT32   cElements;
        StackTraceElement* pElements;
        void*   pStopStack;   // use to limit the crawl

        CaptureStackTraceData() : skip(0), cElementsAllocated(0), cElements(0), pElements(NULL), pStopStack((void*)-1) {}
    };

public:
    // Functions on System.Array
    static FCDECL5(void,    ArrayCopy, ArrayBase* m_pSrc, INT32 m_iSrcIndex, ArrayBase* m_pDst, INT32 m_iDstIndex, INT32 m_iLength);
    static FCDECL3(void,    ArrayClear, ArrayBase* pArrayUNSAFE, INT32 iIndex, INT32 iLength);
    static FCDECL1(Object*, GetEmptyArrayForCloning, ArrayBase* inArrayUNSAFE);

    // Functions on the System.Environment class
    static FCDECL0(UINT32, GetTickCount);
    static FCDECL0(INT64, GetWorkingSet);
	static FCDECL1(VOID,Exit,INT32 exitcode);
	static FCDECL1(UINT32, Increment32, UINT32 *location);
	static FCDECL1(VOID,SetExitCode,INT32 exitcode);
    static FCDECL0(INT32, GetExitCode);
    static FCDECL0(StringObject*, _GetCommandLine);
    static FCDECL0(Object*, GetCommandLineArgs);
    static FCDECL0(Object*, GetEnvironmentCharArray);
    static FCDECL0(Object*, GetVersionString);
    static OBJECTREF CaptureStackTrace(Frame *pStartFrame, void* pStopStack, CaptureStackTraceData *pData=NULL);

    static FCDECL0(StringObject*, GetDeveloperPath);
    static FCDECL1(Object*,       _GetEnvironmentVariable, StringObject* strVar);
    static FCDECL0(StringObject*, _GetModuleFileName);
    static FCDECL0(StringObject*, GetRuntimeDirectory);
    static FCDECL0(StringObject*, GetHostBindingFile);
    static FCDECL1(INT32, FromGlobalAccessCache, Object* refAssemblyUNSAFE);

    static FCDECL0(BOOL, HasShutdownStarted);

    // The exit code for the process is communicated in one of two ways.  If the
    // entrypoint returns an 'int' we take that.  Otherwise we take a latched
    // process exit code.  This can be modified by the app via System.SetExitCode().
    static INT32 LatchedExitCode;

    // CaptureStackTraceMethod
    // Return a method info for the method were the exception was thrown
    static FCDECL1(Object*, CaptureStackTraceMethod, ArrayBase* pStackTraceUNSAFE);

private:
    static StackWalkAction CaptureStackTraceCallback(CrawlFrame *, VOID*);

//    static FCDECL1(LPUTF8, FormatStackTraceInternal, ArrayBase* pStackTraceUNSAFE);   // is anyone using this?

    // The following functions are all helpers for ArrayCopy
    static AssignArrayEnum CanAssignArrayType(const BASEARRAYREF pSrc, const BASEARRAYREF pDest);
    static void CastCheckEachElement(BASEARRAYREF pSrc, unsigned int srcIndex, BASEARRAYREF pDest, unsigned int destIndex, unsigned int length);
    static void __stdcall BoxEachElement(BASEARRAYREF pSrc, unsigned int srcIndex, BASEARRAYREF pDest, unsigned int destIndex, unsigned int length);
    static void __stdcall UnBoxEachElement(BASEARRAYREF pSrc, unsigned int srcIndex, BASEARRAYREF pDest, unsigned int destIndex, unsigned int length, BOOL castEachElement);
    static void __stdcall PrimitiveWiden(BASEARRAYREF pSrc, unsigned int srcIndex, BASEARRAYREF pDest, unsigned int destIndex, unsigned int length);
};

inline void SetLatchedExitCode (INT32 code)
{
    SystemNative::LatchedExitCode = code;
}

inline INT32 GetLatchedExitCode (void)
{
    return SystemNative::LatchedExitCode;
}

#endif // _COMSYSTEM_H

