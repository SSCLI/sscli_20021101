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
// ECALL.H -
//
// Handles our private native calling interface.
//


#ifndef _ECALL_H_
#define _ECALL_H_

#include "fcall.h"
#include "mlcache.h"
#include "corinfo.h"


class StubLinker;
class ECallMethodDesc;
class ArgBasedStubCache;
class MethodDesc;

enum  StubStyle;

struct ECFunc {
    BOOL IsFCall()                      { return TRUE; }
    CorInfoIntrinsics   IntrinsicID()   { return CorInfoIntrinsics(m_intrinsicID); }

    LPCUTF8            m_wszMethodName;
    LPHARDCODEDMETASIG m_wszMethodSig;
    LPVOID             m_pImplementation;
    MethodDesc*        m_pMD;               // for reverse mapping

    int                m_intrinsicID : 8;

    ECFunc*            m_pNext;             // linked list for hash table
};


struct ECClass
{
    LPCUTF8      m_wszClassName;
    LPCUTF8      m_wszNameSpace;
    ECFunc      *m_pECFunc;
};


class ArrayStubCache : public MLStubCache
{
    virtual MLStubCompilationMode CompileMLStub(const BYTE *pRawMLStub,
                                                StubLinker *psl,
                                                void *callerContext);
    virtual UINT Length(const BYTE *pRawMLStub);
};


//=======================================================================
// Collects code and data pertaining to the ECall interface.
//=======================================================================
class ECall
{
    public:
        //---------------------------------------------------------
        // One-time init
        //---------------------------------------------------------
        static BOOL Init();

        //---------------------------------------------------------
        // One-time cleanup
        //---------------------------------------------------------

        //---------------------------------------------------------
        // Either creates or retrieves from the cache, a stub to
        // invoke ECall methods. Each call refcounts the returned stub.
        // This routines throws a COM+ exception rather than returning
        // NULL.
        //---------------------------------------------------------
        static Stub* GetECallMethodStub(StubLinker *psl, ECallMethodDesc *pMD);

        //---------------------------------------------------------
        // Call at strategic times to discard unused stubs.
        //---------------------------------------------------------


        //---------------------------------------------------------
        // Stub cache for ECall Method stubs
        //---------------------------------------------------------
        static ArgBasedStubCache *m_pECallStubCache;  


        static CorInfoIntrinsics IntrinsicID(MethodDesc*);

        //---------------------------------------------------------
        // Cache for array stubs
        //---------------------------------------------------------
        static ArrayStubCache *m_pArrayStubCache;


    private:
        friend MethodDesc* MapTargetBackToMethod(const void* pTarg);
        friend void* FindImplForMethod(MethodDesc *pMD);

        static ECFunc* FindTarget(const void* pTarg);


        ECall() {};     // prevent "new"'s on this class

};

#endif // _ECALL_H_
