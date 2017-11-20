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
//*****************************************************************************
//
// EETwain.h
//
// This file has the definition of ICodeManager and EECodeManager.
//
// ICorJitCompiler compiles the IL of a method to native code, and stores
// auxilliary data called as GCInfo (via ICorJitInfo::allocGCInfo()).
// The data is used by the EE to manage the method's garbage collection,
// exception handling, stack-walking etc.
// This data can be parsed by an ICodeManager corresponding to that
// ICorJitCompiler.
//
// EECodeManager is an implementation of ICodeManager for a default format
// of GCInfo. Various ICorJitCompiler's are free to share this format so that
// they do not need to provide their own implementation of ICodeManager
// (though they are permitted to, if they want).
//
//*****************************************************************************
#ifndef _EETWAIN_H
#define _EETWAIN_H
//*****************************************************************************

#include "regdisp.h"
#include "corjit.h"     // For NativeVarInfo

struct EHContext;

typedef void (*GCEnumCallback)(
    LPVOID          hCallback,      // callback data
    OBJECTREF*      pObject,        // address of obect-reference we are reporting
    DWORD           flags           // is this a pinned and/or interior pointer
);

/******************************************************************************
  The stackwalker maintains some state on behalf of ICodeManager.
*/

const int CODEMAN_STATE_SIZE = 256;

struct CodeManState
{
    DWORD       dwIsSet; // Is set to 0 by the stackwalk as appropriate
    BYTE        stateBuf[CODEMAN_STATE_SIZE];
};

/******************************************************************************
   These flags are used by some functions, although not all combinations might
   make sense for all functions.
*/

enum ICodeManagerFlags 
{
    ActiveStackFrame =  0x0001, // this is the currently active function
    ExecutionAborted =  0x0002, // execution of this function has been aborted
                                    // (i.e. it will not continue execution at the
                                    // current location)
    AbortingCall    =   0x0004, // The current call will never return
    UpdateAllRegs   =   0x0008, // update full register set
    CodeAltered     =   0x0010, // code of that function might be altered
                                    // (e.g. by debugger), need to call EE
                                    // for original code
};

//*****************************************************************************
//
// This interface is used by ICodeManager to get information about the
// method whose GCInfo is being processed.
// It is useful so that some information which is available elsewhere does
// not need to be cached in the GCInfo.
// It is similar to corinfo.h - ICorMethodInfo
//

class ICodeInfo
{
public:

    // this function is for debugging only.  It returns the method name
    // and if 'moduleName' is non-null, it sets it to something that will
    // says which method (a class name, or a module name)
    virtual const char* __stdcall getMethodName(const char **moduleName /* OUT */ ) = 0;

    // Returns the  CorInfoFlag's from corinfo.h
    virtual DWORD       __stdcall getMethodAttribs() = 0;

    // Returns the  CorInfoFlag's from corinfo.h
    virtual DWORD       __stdcall getClassAttribs() = 0;

    virtual void        __stdcall getMethodSig(CORINFO_SIG_INFO *sig /* OUT */ ) = 0;

    // Start IP of the method
    virtual LPVOID      __stdcall getStartAddress() = 0;

    // Get the MethodDesc of the method. This is a hack as MethodDescs are
    // not exposed in the public APIs. However, it is currently used by
    // the EJIT to report vararg arguments to GC.
    virtual void *                getMethodDesc_HACK() = 0;
};

//*****************************************************************************
//
// ICodeManager is the abstract class that all CodeManagers 
// must inherit from.  This will probably need to move into
// cor.h and become a real com interface.
// 
//*****************************************************************************

class ICodeManager
{
public:

/*
    Last chance for the runtime support to do fixups in the context
    before execution continues inside a filter, catch handler, or fault/finally
*/

enum ContextType
{
    FILTER_CONTEXT,
    CATCH_CONTEXT,
    FINALLY_CONTEXT
};

/* Type of funclet corresponding to a shadow stack-pointer */

enum
{
    SHADOW_SP_IN_FILTER = 0x1,
    SHADOW_SP_FILTER_DONE = 0x2,
    SHADOW_SP_BITS = 0x3
};

virtual void FixContext(ContextType     ctxType,
                        EHContext      *ctx,
                        LPVOID          methodInfoPtr,
                        LPVOID          methodStart,
                        DWORD           nestingLevel,
                        OBJECTREF       thrownObject,
                        CodeManState   *pState,
                        size_t       ** ppShadowSP,             // OUT
                        size_t       ** ppEndRegion) = 0;       // OUT

/*
    Unwind the current stack frame, i.e. update the virtual register
    set in pContext. This will be similar to the state after the function
    returns back to caller (IP points to after the call, Frame and Stack
    pointer has been reset, callee-saved registers restored 
    (if UpdateAllRegs), callee-UNsaved registers are trashed)
    Returns success of operation.
*/
virtual bool UnwindStackFrame(PREGDISPLAY     pContext,
                              LPVOID          methodInfoPtr,
                              ICodeInfo      *pCodeInfo,
                              unsigned        flags,
						      CodeManState   *pState) = 0;
/*
    Is the function currently at a "GC safe point" ? 
    Can call EnumGcRefs() successfully
*/
virtual bool IsGcSafe(PREGDISPLAY     pContext,
                      LPVOID          methodInfoPtr,
                      ICodeInfo      *pCodeInfo,
                      unsigned        flags) = 0;

/*
    Enumerate all live object references in that function using
    the virtual register set. Same reference location cannot be enumerated 
    multiple times (but all differenct references pointing to the same
    object have to be individually enumerated).
    Returns success of operation.
*/
virtual bool EnumGcRefs(PREGDISPLAY     pContext,
                        LPVOID          methodInfoPtr,
                        ICodeInfo      *pCodeInfo,
                        unsigned        curOffs,
                        unsigned        flags,
                        GCEnumCallback  pCallback,
                        LPVOID          hCallBack) = 0;

/*
    Return the address of the local security object reference
    (if available).
*/
virtual OBJECTREF* GetAddrOfSecurityObject(PREGDISPLAY     pContext,
                                           LPVOID          methodInfoPtr,
                                           unsigned        relOffset,
            						       CodeManState   *pState) = 0;

/*
    Returns "this" pointer if it is a non-static method AND
    the object is still alive.
    Returns NULL in all other cases.
*/
virtual OBJECTREF GetInstance(PREGDISPLAY     pContext,
                              LPVOID          methodInfoPtr,
                              unsigned        relOffset) = 0;

/*
  Returns true if the given IP is in the given method's prolog or an epilog.
*/
virtual bool IsInPrologOrEpilog(DWORD  relPCOffset,
                                LPVOID methodInfoPtr,
                                size_t* prologSize) = 0;

/*
  Returns the size of a given function.
*/
virtual size_t GetFunctionSize(LPVOID methodInfoPtr) = 0;

/*
  Returns the size of the frame (barring localloc)
*/
virtual unsigned int GetFrameSize(LPVOID methodInfoPtr) = 0;

/* Debugger API */

virtual const BYTE*     GetFinallyReturnAddr(PREGDISPLAY pReg)=0;

virtual BOOL            IsInFilter(void *methodInfoPtr,
                                   unsigned offset,    
                                   PCONTEXT pCtx,
                                   DWORD curNestLevel) = 0;

virtual BOOL            LeaveFinally(void *methodInfoPtr,
                                     unsigned offset,    
                                     PCONTEXT pCtx,
                                     DWORD curNestLevel) = 0;

virtual void            LeaveCatch(void *methodInfoPtr,
                                   unsigned offset,    
                                   PCONTEXT pCtx)=0;

};


//*****************************************************************************
#endif // _EETWAIN_H
//*****************************************************************************
