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
// This module contains the native methods for the delegate class
//
// Date: June 1998
////////////////////////////////////////////////////////////////////////////////

#ifndef _COMDELEGATE_H_
#define _COMDELEGATE_H_

class Stub;
class EEClass;
class ShuffleThunkCache;

#include "cgensys.h"
#include "nexport.h"
#include "comvariant.h"
#include "mlcache.h"

// This class represents the native methods for the Delegate class
class COMDelegate
{
private:
        //struct _InternalCreateArgs      {
        //        DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis);
        //        DECLARE_ECALL_I1_ARG(bool, ignoreCase);
        //        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, methodName);
        //        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, target);
        //};
        //struct _InternalCreateStaticArgs        {
        //        DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis); 
        //        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, methodName);
        //        DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, target);
        //};
        //struct _InternalCreateMethodArgs        {
        //        DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis); 
        //        DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, targetMethod);
        //        DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, invokeMeth);
        //};

        // This method will validate that the target method for a delegate
        //      and the delegate's invoke method have compatible signatures....
        static bool ValidateDelegateTarget(MethodDesc* pMeth,EEClass* pDel);

    friend VOID CPUSTUBLINKER::EmitMulticastInvoke(UINT numStackBytes, BOOL fSingleCast
#ifdef _PPC_
        , BOOL fHasReturnBuffer
#endif
        );
    friend VOID CPUSTUBLINKER::EmitShuffleThunk(struct ShuffleEntry *pShuffeEntryArray);
    friend BOOL StubLinkStubManager::TraceManager(Thread *thread, 
                                                  TraceDestination *trace,
                                                  CONTEXT *pContext, 
                                                  BYTE **pRetAddr);
    friend BOOL StubLinkStubManager::IsStaticDelegate(BYTE *pbDel);
    friend BYTE **StubLinkStubManager::GetStaticDelegateRealDest(BYTE *pbDel);
    friend BYTE **StubLinkStubManager::GetSingleDelegateRealDest(BYTE *pbDel);
    friend BOOL MulticastFrame::TraceFrame(Thread *thread, BOOL fromPatch, 
                                TraceDestination *trace, REGDISPLAY *regs);

    static FieldDesc* m_pORField;       // Object reference field...
    static FieldDesc* m_pFPField;       // Function Pointer Address field...
    static FieldDesc* m_pPRField;       // _prev field (MulticastDelegate)
    static FieldDesc* m_pFPAuxField;    // aux field for statics
    static FieldDesc* m_pMethInfoField; // Method Info
    static FieldDesc* m_ppNextField;    // Method Info


    static ArgBasedStubCache *m_pMulticastStubCache;

	static MethodTable* s_pIAsyncResult;	// points to System.IAsyncResult's method table
	static MethodTable* s_pAsyncCallback;	// points to System.AsyncCallBack's method table

public:
    static ShuffleThunkCache *m_pShuffleThunkCache; 

    // One time init.
    static BOOL Init();

    // Termination

    // Initialize fields
    static void InitFields();

    static FCDECL3(void, DelegateConstruct, ReflectBaseObject* refThis, Object* target, SLOT method);

    //struct _InternalAllocArgs {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, target);
    //};
    static FCDECL1(Object*, InternalAlloc, ReflectClassBaseObject* targetUNSAFE);



    // InternalCreate
    // Internal Create is called from the constructor.  It does the internal
    //      initialization of the Delegate.
    static FCDECL4(void, InternalCreate, ReflectBaseObject* refThisUNSAFE, Object* targetUNSAFE, StringObject* methodNameUNSAFE, BYTE ignoreCase);

    // InternalCreateStatic
    // Internal Create is called from the constructor. The method must
    //      be a static method.
    static FCDECL3(void, InternalCreateStatic, ReflectBaseObject* refThisUNSAFE, ReflectClassBaseObject* targetUNSAFE, StringObject* methodNameUNSAFE);

    // InternalCreateMethod
    // This method will create initalize a delegate based upon a MethodInfo
    //      for a static method.
    static FCDECL3(void, InternalCreateMethod, ReflectBaseObject* refThisUNSAFE, ReflectBaseObject* invokeMethUNSAFE, ReflectBaseObject* targetMethodUNSAFE);

    // InternalFindMethodInfo
    // This gets the MethodInfo for a delegate, creating it if necessary
    static FCDECL1(Object*, InternalFindMethodInfo, ReflectBaseObject* refThis);

    // Marshals a delegate to a unmanaged callback.
    static LPVOID ConvertToCallback(OBJECTREF pDelegate);

    // Marshals an unmanaged callback to Delegate
    static OBJECTREF ConvertToDelegate(LPVOID pCallback);

    // Decides if pcls derives from Delegate.
    static BOOL IsDelegate(EEClass *pcls);

    // GetMethodPtr
    // Returns the FieldDesc* for the MethodPtr field
    static FieldDesc* GetMethodPtr();

    // GetMethodAuxPtr
    // Returns the FieldDesc* for the MethodPtrAux field
    static FieldDesc* GetMethodPtrAux();

    // GetOR
    // Returns the FieldDesc* for the Object reference field
    static FieldDesc* GetOR();

    // GetDelegateThunkInfo
    // Returns the DelegateThunkInfo field
    static FieldDesc* GetDelegateThunkInfo();
    
    // GetpNext
    // Returns the pNext field
    static FieldDesc* GetpNext();

    // Get the cpu stub for a delegate invoke.
    static Stub *GetInvokeMethodStub(CPUSTUBLINKER *psl, EEImplMethodDesc* pMD);

    static MethodDesc * __fastcall GetMethodDesc(OBJECTREF obj);

    static MethodDesc * FindDelegateInvokeMethod(EEClass *pcls);

    static Stub* SetupShuffleThunk(DelegateEEClass *pDelCls);

    // Method to do static validation of delegate .ctor
    static BOOL ValidateCtor(MethodDesc *pFtn, EEClass *pDlgt, EEClass *pInst);
private:
	static BOOL ValidateBeginInvoke(DelegateEEClass* pClass);		// make certain the BeginInvoke method is consistant with the Invoke Method
	static BOOL ValidateEndInvoke(DelegateEEClass* pClass);		// make certain the EndInvoke method is consistant with the Invoke Method
};


// Want no unused bits in ShuffleEntry since unused bits can make
// equivalent ShuffleEntry arrays look unequivalent and deoptimize our
// hashing.
#include <pshpack1.h>

// To handle a call to a static delegate, we create an array of ShuffleEntry
// structures. Each entry instructs the shuffler to move a chunk of bytes.
// The size of the chunk is StackElemSize (typically a DWORD): long arguments
// have to be expressed as multiple ShuffleEntry's.
//
// The ShuffleEntry array serves two purposes:
//
//  1. A platform-indepedent blueprint for creating the platform-specific
//     shuffle thunk.
//  2. A hash key for finding the shared shuffle thunk for a particular
//     signature.
struct ShuffleEntry
{
    enum {
        REGMASK  = 0x8000,
        OFSMASK  = 0x7fff,
        SENTINEL = 0xffff,
    };

    // Special values:
    //  -1       - indicates end of shuffle array: stacksizedelta
    //             == difference in stack size between virtual and static sigs.
    //  high bit - indicates a register argument: mask it off and
    //             the result is an offset into ArgumentRegisters.
    UINT16    srcofs;
    union {
        UINT16    dstofs;           //if srcofs != SENTINEL
        UINT16    stacksizedelta;   //if dstofs == SENTINEL
    };
};


#include <poppack.h>

#endif  // _COMDELEGATE_H_





