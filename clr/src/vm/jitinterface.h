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
// ===========================================================================
// File: JITinterface.H
//
// ===========================================================================

#ifndef JITINTERFACE_H
#define JITINTERFACE_H

#include "corjit.h"
#include "fcall.h"
#include "object.h"

// Some defines for jitting
#define PREINIT_SIZE 1
#define NPDW   (sizeof(DWORD)*2)    // (4-bit) nibbles per dword
#define BYTES_PER_BUCKET    32
#define MAXCODEHEAPSIZE 64*1024
#define HEAP2MAPSIZE(x)  ((x) / (BYTES_PER_BUCKET*NPDW)*sizeof(DWORD))
#define ADDR2POS(x) ((x) >> 5)
#define ADDR2OFFS(x) (DWORD)((((x)&0x1f)>> 2)+1)

#define POSOFF2ADDR(pos, of) (DWORD*)(((pos) << 5) + (((of)-1)<< 2))
#define HEAP2MAPSIZE(x)  ((x) / (BYTES_PER_BUCKET*NPDW)*sizeof(DWORD))

#define POS2SHIFTCOUNT(x) (DWORD)(28 - (((x)%NPDW)<< 2))

#define RD2PAGE(x) (((x)+0xfff)/0x1000 * 0x1000)
#define SETHEADER(m, x) NibbleMapSet(m, ADDR2POS(x), ADDR2OFFS(x))
#define RESETHEADER(m, x) NibbleMapSet(m, ADDR2POS(x), 0)
#define PPMDTOJITSTART(x)               (void *)((DWORD)(x)+sizeof(void*))
#define JITSTARTTOPPMD(x)               ((MethodDesc**)(x) - 1)
#define JITSTARTTOPMD(x)                *JITSTARTTOPPMD(x)

#define SafeDelete(x) {if (x) delete(x);}

#define LARGE_ELEMENT_SIZE  16

#define JIT_DEFAULT_INLINE_SIZE 32

class Stub;
class MethodDesc;
class FieldDesc;
enum RuntimeExceptionKind;

extern "C"
{
    void __stdcall JIT_WriteBarrierEAX();        // JIThelp.asm/JIThelp.s
    void __stdcall JIT_WriteBarrierEBX();        // JIThelp.asm/JIThelp.s
    void __stdcall JIT_WriteBarrierECX();        // JIThelp.asm/JIThelp.s
    void __stdcall JIT_WriteBarrierESI();        // JIThelp.asm/JIThelp.s
    void __stdcall JIT_WriteBarrierEDI();        // JIThelp.asm/JIThelp.s
    void __stdcall JIT_WriteBarrierEBP();        // JIThelp.asm/JIThelp.s
}

BOOL InitJITHelpers1();
BOOL InitJITHelpers2();
Stub* JITFunction(MethodDesc* ftn, COR_ILMETHOD_DECODER* header, BOOL *fEJit, DWORD flags=0);
void* getJitHelper(CorInfoHelpFunc);


HRESULT LoadHelperTable(Module *currentModule, 
                        void **table, SIZE_T tableSize);
HRESULT LoadDynamicInfoEntry(Module *currentModule,
                             Module *pInfoModule, BYTE *pBlob, 
                             int tableIndex, DWORD *entry);

extern FCDECL1(void, JIT_MonEnter, Object *obj);         // JITinterfaceX86.cpp/JITinterfaceGen.cpp
extern FCDECL2(BOOL, JIT_MonTryEnter, Object *obj, __int32 timeout); // JITinterfaceX86.cpp/JITinterfaceGen.cpp
extern FCDECL1(void, JIT_MonExit, Object *obj);          // JITinterfaceX86.cpp/JITinterfaceGen.cpp
extern FCDECL1(void, JIT_MonEnterStatic, AwareLock *lock);// JITinterfaceX86.cpp/JITinterfaceGen.cpp
extern FCDECL1(void, JIT_MonExitStatic, AwareLock *lock); // JITinterfaceX86.cpp/JITinterfaceGen.cpp
extern FCDECL2(__int32, JIT_GetField32, Object *obj, FieldDesc *pFD);
extern FCDECL2(__int64, JIT_GetField64, Object *obj, FieldDesc *pFD);
extern FCDECL2(Object* , JIT_GetFieldObj, Object *obj, FieldDesc *pFD);
extern FCDECL3(VOID, JIT_GetFieldStruct, LPVOID retBuff, Object *obj, FieldDesc *pFD);
extern FCDECL3(VOID, JIT_SetField32, Object *obj, FieldDesc *pFD, __int32 value);
extern FCDECL3(VOID, JIT_SetField64, Object *obj, FieldDesc *pFD, __int64 value);
extern FCDECL3(VOID, JIT_SetFieldObj, Object *obj, FieldDesc *pFD, Object *value);
extern "C" FCDECL2(VOID, JIT_CheckedWriteBarrier, Object **dst, Object *ref);
extern "C" FCDECL2(VOID, JIT_WriteBarrier, Object **dst, Object *ref);
extern FCDECL3(VOID, JIT_StructWriteBarrier, void *dst, void *src, CORINFO_CLASS_HANDLE typeHnd_);
    
extern FCDECL3(VOID, JIT_SetFieldStruct, Object *obj, FieldDesc *pFD, LPVOID valuePtr);
extern FCDECL1(Object*, JIT_Unwrap, Object* obj);;
extern FCDECL1(Object*, JIT_Wrap, Object* obj);
extern "C" __int64 __stdcall JIT_Dbl2Lng(double val);

extern "C" float __stdcall JIT_FltRem(float divisor, float dividend);
extern "C" double __stdcall JIT_DblRem(double divisor, double dividend);
extern BOOL __stdcall JIT_CheckBoundaryForInline(CORINFO_METHOD_HANDLE hCaller, CORINFO_METHOD_HANDLE hCallee);

extern BYTE *JIT_WriteBarrier_Buf_Start; 
extern BYTE *JIT_WriteBarrier_Buf_End; 

#ifdef _X86_
//typedef StringObject* (__fastcall * FastStringAllocatorFuncPtr)(DWORD cchArrayLength);
//static FastStringAllocatorFuncPtr fastStringAllocator;
#else
extern FCDECL1(StringObject*, FastAllocateStringGeneric, DWORD cchString);
#endif
extern LPVOID *FCallFastAllocateStringImpl;

/*********************************************************************/
/*********************************************************************/
/* This is a little helper class that CEEInfo uses */

#include "siginfo.hpp"

struct ValueArrayDescr
{
    ValueArrayDescr(SigPointer aSig, Module* aModule, ValueArrayDescr* aNext=0) :
        sig(aSig), module(aModule), next(aNext) {}
    ~ValueArrayDescr() { if (next != 0) delete next; }

    SigPointer          sig;    // points to the signature describing the value array
    Module             *module; // module the signature comes from
    ValueArrayDescr    *next;
};

class CEEInfo : public virtual ICorDynamicInfo {
public:
    // ICorClassInfo stuff
    CorInfoType __stdcall asCorInfoType (CORINFO_CLASS_HANDLE cls);
    CORINFO_MODULE_HANDLE __stdcall getClassModule(CORINFO_CLASS_HANDLE clsHnd);
    const char* __stdcall getClassName (CORINFO_CLASS_HANDLE cls);
    DWORD __stdcall getClassAttribs (CORINFO_CLASS_HANDLE cls, CORINFO_METHOD_HANDLE context);
    CORINFO_CLASS_HANDLE __stdcall getSDArrayForClass(CORINFO_CLASS_HANDLE clsHnd);
    unsigned __stdcall getClassSize (CORINFO_CLASS_HANDLE cls);
    unsigned __stdcall getClassGClayout (CORINFO_CLASS_HANDLE cls, BYTE* gcPtrs); /* really GCType* gcPtrs */
    unsigned __stdcall getClassNumInstanceFields(CORINFO_CLASS_HANDLE cls);
    unsigned __stdcall getFieldNumber(CORINFO_FIELD_HANDLE fldHnd);

    // return the enclosing class of the field
    CORINFO_CLASS_HANDLE __stdcall getEnclosingClass (
                        CORINFO_FIELD_HANDLE    field );

    // Check Visibility rules.
    // For Protected (family access) members, type of the instance is also
    // considered when checking visibility rules.
    BOOL __stdcall canAccessField(
                        CORINFO_METHOD_HANDLE   context,
                        CORINFO_FIELD_HANDLE    target,
                        CORINFO_CLASS_HANDLE    instance);

    CorInfoHelpFunc __stdcall getNewHelper(CORINFO_CLASS_HANDLE newCls, CORINFO_METHOD_HANDLE scopeHnd);
    CorInfoHelpFunc __stdcall getNewArrHelper(CORINFO_CLASS_HANDLE arrayCls,
                                              CORINFO_METHOD_HANDLE context);
    CorInfoHelpFunc __stdcall getIsInstanceOfHelper(CORINFO_CLASS_HANDLE IsInstClsHnd);
    CorInfoHelpFunc __stdcall getChkCastHelper(CORINFO_CLASS_HANDLE IsInstClsHnd);
    BOOL __stdcall initClass (CORINFO_CLASS_HANDLE cls, CORINFO_METHOD_HANDLE context, BOOL speculative);
    BOOL __stdcall loadClass (CORINFO_CLASS_HANDLE cls, CORINFO_METHOD_HANDLE context, BOOL speculative);
    CORINFO_CLASS_HANDLE __stdcall getBuiltinClass(CorInfoClassId classId);

    // "System.Int32" ==> CORINFO_TYPE_INT..
    CorInfoType __stdcall getTypeForPrimitiveValueClass(
            CORINFO_CLASS_HANDLE        cls
            );

    // TRUE if child is a subtype of parent
    // if parent is an interface, then does child implement / extend parent
    BOOL __stdcall canCast(
            CORINFO_CLASS_HANDLE        child, 
            CORINFO_CLASS_HANDLE        parent
            );

    // returns is the intersection of cls1 and cls2.
    CORINFO_CLASS_HANDLE __stdcall mergeClasses(
            CORINFO_CLASS_HANDLE        cls1, 
            CORINFO_CLASS_HANDLE        cls2
            );

    // Given a class handle, returns the Parent type.
    // For COMObjectType, it returns Class Handle of System.Object.
    // Returns 0 if System.Object is passed in.
    CORINFO_CLASS_HANDLE __stdcall getParentType (
            CORINFO_CLASS_HANDLE        cls
            );

    // Returns the CorInfoType of the "child type". If the child type is
    // not a primitive type, *clsRet will be set.
    // Given an Array of Type Foo, returns Foo.
    // Given BYREF Foo, returns Foo
    CorInfoType __stdcall getChildType (
            CORINFO_CLASS_HANDLE       clsHnd,
            CORINFO_CLASS_HANDLE       *clsRet
            );

    // Check Visibility rules.
    BOOL __stdcall canAccessType(
            CORINFO_METHOD_HANDLE       context,
            CORINFO_CLASS_HANDLE        target
            );

    // Is this a Single dimentional array 
    BOOL __stdcall isSDArray(
            CORINFO_CLASS_HANDLE        cls
            );

    // Static helpers
    static BOOL CanCast(
            CorElementType el1, 
            CorElementType el2);

        // ICorScopeInfo stuff
    DWORD __stdcall getModuleAttribs (CORINFO_MODULE_HANDLE cls);
    CORINFO_CLASS_HANDLE __stdcall findClass(CORINFO_MODULE_HANDLE  scopeHnd, unsigned metaTOK, CORINFO_METHOD_HANDLE context);
    CORINFO_FIELD_HANDLE __stdcall findField(CORINFO_MODULE_HANDLE  scopeHnd, unsigned metaTOK, CORINFO_METHOD_HANDLE context);
    CORINFO_METHOD_HANDLE __stdcall findMethod(CORINFO_MODULE_HANDLE  scopeHnd, unsigned metaTOK, CORINFO_METHOD_HANDLE context);
    void __stdcall findSig(CORINFO_MODULE_HANDLE scopeHnd, unsigned sigTOK, CORINFO_SIG_INFO* sig);
    void __stdcall findCallSiteSig(CORINFO_MODULE_HANDLE scopeHnd, unsigned methTOK, CORINFO_SIG_INFO* sig);
    CORINFO_GENERIC_HANDLE __stdcall findToken (CORINFO_MODULE_HANDLE  scopeHnd, unsigned metaTOK, CORINFO_METHOD_HANDLE context,
                                                CORINFO_CLASS_HANDLE& tokenType);
    const char * __stdcall findNameOfToken (CORINFO_MODULE_HANDLE module, mdToken metaTOK);
    BOOL __stdcall canSkipVerification(CORINFO_MODULE_HANDLE moduleHnd);

    // Checks if the given metadata token is valid
    BOOL __stdcall isValidToken (
            CORINFO_MODULE_HANDLE       module,
            mdToken                    metaTOK);

    // Checks if the given metadata token is valid StringRef
    BOOL __stdcall isValidStringRef (
            CORINFO_MODULE_HANDLE       module,
            mdToken                    metaTOK);

    static const char * __stdcall findNameOfToken (Module* module, mdToken metaTOK);

        // ICorMethodInfo stuff
    const char* __stdcall getMethodName (CORINFO_METHOD_HANDLE ftnHnd, const char** scopeName);
    unsigned __stdcall getMethodHash (CORINFO_METHOD_HANDLE ftnHnd);
    DWORD __stdcall getMethodAttribs (CORINFO_METHOD_HANDLE ftnHnd, CORINFO_METHOD_HANDLE context);
    void __stdcall setMethodAttribs (CORINFO_METHOD_HANDLE ftnHnd, DWORD attribs);
    
    bool __stdcall getMethodInfo (
            CORINFO_METHOD_HANDLE ftnHnd,
            CORINFO_METHOD_INFO*  methInfo);

    CorInfoInline __stdcall canInline (
            CORINFO_METHOD_HANDLE  callerHnd,
            CORINFO_METHOD_HANDLE  calleeHnd,
            CORINFO_ACCESS_FLAGS   flags);

    bool __stdcall canTailCall (
            CORINFO_METHOD_HANDLE  callerHnd,
            CORINFO_METHOD_HANDLE  calleeHnd,
            CORINFO_ACCESS_FLAGS   flags);

    void __stdcall getMethodSig (
            CORINFO_METHOD_HANDLE ftnHnd,
            CORINFO_SIG_INFO* sigInfo);

    void __stdcall getEHinfo(
            CORINFO_METHOD_HANDLE ftn,
            unsigned      EHnumber,
            CORINFO_EH_CLAUSE* clause);

    CORINFO_CLASS_HANDLE __stdcall getMethodClass (CORINFO_METHOD_HANDLE methodHnd);
    CORINFO_MODULE_HANDLE __stdcall getMethodModule (CORINFO_METHOD_HANDLE methodHnd);
    unsigned __stdcall getMethodVTableOffset (CORINFO_METHOD_HANDLE methodHnd);
    CorInfoIntrinsics __stdcall getIntrinsicID(CORINFO_METHOD_HANDLE method);

    BOOL __stdcall canPutField(
            CORINFO_METHOD_HANDLE method,
            CORINFO_FIELD_HANDLE field);
    CorInfoUnmanagedCallConv __stdcall getUnmanagedCallConv(CORINFO_METHOD_HANDLE method);
    BOOL __stdcall pInvokeMarshalingRequired(CORINFO_METHOD_HANDLE method, CORINFO_SIG_INFO* callSiteSig);

    // Generate a cookie based on the signature that would needs to be passed
    //  to the above generic stub
    LPVOID GetCookieForPInvokeCalliSig(CORINFO_SIG_INFO* szMetaSig, void ** ppIndirection);

    // Returns TRUE if the methods have compatible sig
    BOOL __stdcall compatibleMethodSig(
        CORINFO_METHOD_HANDLE child, 
        CORINFO_METHOD_HANDLE parent);

    // Check Visibility rules.
    // For Protected (family access) members, type of the instance is also
    // considered when checking visibility rules.
    BOOL __stdcall canAccessMethod(
            CORINFO_METHOD_HANDLE       context,
            CORINFO_METHOD_HANDLE       target,
            CORINFO_CLASS_HANDLE        instance);

    // Given a Delegate type and a method, check if the method signature
    // is Compatible with the Invoke method of the delegate.
    BOOL __stdcall isCompatibleDelegate(
            CORINFO_CLASS_HANDLE        objCls,
            CORINFO_METHOD_HANDLE       method,
            CORINFO_METHOD_HANDLE       delegateCtor);

    // Static helper
    static BOOL CompatibleMethodSig(
            MethodDesc *pMethDescA, 
            MethodDesc *pMethDescB);

        // ICorFieldInfo stuff
    const char* __stdcall getFieldName (CORINFO_FIELD_HANDLE field, 
                                        const char** scopeName);

    DWORD __stdcall getFieldAttribs (CORINFO_FIELD_HANDLE  field, 
                                     CORINFO_METHOD_HANDLE context,
                                     CORINFO_ACCESS_FLAGS  flags);

    CORINFO_CLASS_HANDLE __stdcall getFieldClass (CORINFO_FIELD_HANDLE field);
    CorInfoType __stdcall getFieldType (CORINFO_FIELD_HANDLE field, CORINFO_CLASS_HANDLE* structType);
    unsigned __stdcall getFieldOffset (CORINFO_FIELD_HANDLE field);
    void* __stdcall getFieldAddress(CORINFO_FIELD_HANDLE field, void **ppIndirection);
    CorInfoHelpFunc __stdcall getFieldHelper(CORINFO_FIELD_HANDLE field, enum CorInfoFieldAccess kind);


    virtual CorInfoFieldCategory __stdcall getFieldCategory (CORINFO_FIELD_HANDLE field);
    virtual unsigned __stdcall getIndirectionOffset ();

    // ICorDebugInfo stuff
    void * __stdcall allocateArray(SIZE_T cBytes);
    void __stdcall freeArray(void *array);
    void __stdcall getBoundaries(CORINFO_METHOD_HANDLE ftn,
                                 unsigned int *cILOffsets, DWORD **pILOffsets,
                                 ICorDebugInfo::BoundaryTypes *implictBoundaries);
    void __stdcall setBoundaries(CORINFO_METHOD_HANDLE ftn,
                                 ULONG32 cMap, OffsetMapping *pMap);
    void __stdcall getVars(CORINFO_METHOD_HANDLE ftn, ULONG32 *cVars,
                           ILVarInfo **vars, bool *extendOthers);
    void __stdcall setVars(CORINFO_METHOD_HANDLE ftn, ULONG32 cVars,
                           NativeVarInfo *vars);

        // ICorArgInfo stuff

    virtual CorInfoTypeWithMod __stdcall getArgType (
            CORINFO_SIG_INFO*       sig,
            CORINFO_ARG_LIST_HANDLE    args,
            CORINFO_CLASS_HANDLE       *vcTypeRet
            );
    CORINFO_CLASS_HANDLE __stdcall getArgClass (
            CORINFO_SIG_INFO*       sig,
            CORINFO_ARG_LIST_HANDLE    args
            );
    CORINFO_ARG_LIST_HANDLE __stdcall getArgNext (
            CORINFO_ARG_LIST_HANDLE args
            );

    // ICorLinkInfo stuff

    bool __stdcall deferLocation(
            CORINFO_METHOD_HANDLE           ftn,
            IDeferredLocation              *pIDL
            ) { return false; }
    void __stdcall recordRelocation(
            void                       **location,
            WORD                         fRelocType
            ) { }

    // ICorErrorInfo stuff

    HRESULT __stdcall GetErrorHRESULT();
    CORINFO_CLASS_HANDLE __stdcall GetErrorClass();
    ULONG __stdcall GetErrorMessage(LPWSTR buffer, ULONG bufferLength);
    int __stdcall FilterException(struct _EXCEPTION_POINTERS *pExceptionPointers);

        // ICorStaticInfo stuff
    void __stdcall getEEInfo(CORINFO_EE_INFO *pEEInfoOut);
    void* __stdcall findPtr(CORINFO_MODULE_HANDLE scopeHnd, unsigned ptrTOK);

        //ICorDynamicInfo stuff
    virtual DWORD __stdcall getFieldThreadLocalStoreID (CORINFO_FIELD_HANDLE field, void **ppIndirection);
    void* __stdcall getInterfaceID (CORINFO_CLASS_HANDLE  cls, void **ppIndirection);
    unsigned __stdcall getInterfaceTableOffset (CORINFO_CLASS_HANDLE    cls, void **ppIndirection);
    unsigned __stdcall getClassDomainID (CORINFO_CLASS_HANDLE   cls, void **ppIndirection);
    CORINFO_VARARGS_HANDLE __stdcall getVarArgsHandle(CORINFO_SIG_INFO *sig, void **ppIndirection);
    virtual void* __stdcall getMethodSync(CORINFO_METHOD_HANDLE ftnHnd, void **ppIndirection);
    virtual void** __stdcall AllocHintPointer(CORINFO_METHOD_HANDLE methodHnd, void **ppIndirection);
    CorInfoCallCategory __stdcall getMethodCallCategory(CORINFO_METHOD_HANDLE   method);
    void* __stdcall getPInvokeUnmanagedTarget(CORINFO_METHOD_HANDLE method, void **ppIndirection);
    void* __stdcall getAddressOfPInvokeFixup(CORINFO_METHOD_HANDLE method, void **ppIndirection);
    CORINFO_PROFILING_HANDLE __stdcall GetProfilingHandle(CORINFO_METHOD_HANDLE method, BOOL *bpHookFunction, void **ppIndirection);
    LPVOID __stdcall constructStringLiteral(CORINFO_MODULE_HANDLE scopeHnd, mdToken metaTok, void **ppIndirection);
    CORINFO_CLASS_HANDLE __stdcall findMethodClass(CORINFO_MODULE_HANDLE module, mdToken methodTok);
    LPVOID __stdcall getInstantiationParam(CORINFO_MODULE_HANDLE module, mdToken methodTok, void **ppIndirection);

    DWORD __stdcall getThreadTLSIndex(void **ppIndirection);
    const void * __stdcall getInlinedCallFrameVptr(void **ppIndirection);
    LONG * __stdcall getAddrOfCaptureThreadGlobal(void **ppIndirection);
    void* __stdcall getHelperFtn (CorInfoHelpFunc ftnNum, void **ppIndirection);
    void* __stdcall getFunctionEntryPoint(CORINFO_METHOD_HANDLE ftn,
                                          InfoAccessType       *pAccessType,
                                          CORINFO_ACCESS_FLAGS  flags);
    void* __stdcall getFunctionFixedEntryPoint(CORINFO_METHOD_HANDLE ftn,
                                       InfoAccessType       *pAccessType,
                                       CORINFO_ACCESS_FLAGS  flags);

    CORINFO_MODULE_HANDLE __stdcall embedModuleHandle(CORINFO_MODULE_HANDLE handle,
                                                      void **ppIndirection);
    CORINFO_CLASS_HANDLE __stdcall embedClassHandle(CORINFO_CLASS_HANDLE handle,
                                                    void **ppIndirection);
    CORINFO_FIELD_HANDLE __stdcall embedFieldHandle(CORINFO_FIELD_HANDLE handle,
                                                    void **ppIndirection);
    CORINFO_METHOD_HANDLE __stdcall embedMethodHandle(CORINFO_METHOD_HANDLE handle,
                                                      void **ppIndirection);
    CORINFO_GENERIC_HANDLE __stdcall embedGenericHandle(
                        CORINFO_MODULE_HANDLE   module,
                        unsigned                metaTOK,
                        CORINFO_METHOD_HANDLE   context,
                        void                  **ppIndirection,
                        CORINFO_CLASS_HANDLE& tokenType);

    void __stdcall setOverride(ICorDynamicInfo *pOverride) { m_pOverride = pOverride; }

    CEEInfo()
      : m_ValueArrays(NULL),
        m_pOverride(NULL)
      {
          m_pOverride = this;
      }

    ~CEEInfo() { delete m_ValueArrays; }

private:
    void* __stdcall getMethodEntryPoint(CORINFO_METHOD_HANDLE ftnHnd, 
                                        CORINFO_ACCESS_FLAGS  flags,
                                        void **ppIndirection);
    void* __stdcall getMethodPointer(CORINFO_METHOD_HANDLE methodHnd, 
                                     CORINFO_ACCESS_FLAGS  flags,
                                     void **ppIndirection);

        // allocate a descriptor that will be removed when this ICorJitInfo goes away
    ValueArrayDescr* allocDescr(SigPointer sig, Module* mod) {
        m_ValueArrays = new ValueArrayDescr(sig, mod, m_ValueArrays);
        return(m_ValueArrays);
    }

        // Value arrays descriptors have the low order bits set to indicate
        // that it is a value array descriptor
    static CORINFO_CLASS_HANDLE markAsValueArray(ValueArrayDescr* descr) {
        _ASSERTE(((size_t)descr & 3) == 0);
        return(CORINFO_CLASS_HANDLE((size_t)descr | 3));
    }
    static bool isValueArray(CORINFO_CLASS_HANDLE clsHnd) {
        return(((size_t)clsHnd & 3) == 3);
    }
    static ValueArrayDescr* asValueArray(CORINFO_CLASS_HANDLE clsHnd) {
        _ASSERTE(isValueArray(clsHnd));
        return((ValueArrayDescr*)(((size_t)clsHnd & ~3)));
    }

    ValueArrayDescr* m_ValueArrays;     // descriptions of value class arrays
#ifdef _DEBUG
    char clsNameBuff[MAX_CLASSNAME_LENGTH];  // used to form fully qualified class names
#endif

    // Cached type handles
    static MethodTable * s_pRuntimeTypeHandle;
    static MethodTable * s_pRuntimeMethodHandle;
    static MethodTable * s_pRuntimeFieldHandle;
    static MethodTable * s_pRuntimeArgumentHandle;
    static MethodTable * s_pTypedReference;

    static void InitStaticTypeHandles();

protected:
    ICorDynamicInfo *       m_pOverride;    
};


/*********************************************************************/

class  IJitManager;
struct _hpCodeHdr;
typedef struct _hpCodeHdr CodeHeader;

#ifdef _MSC_VER
#pragma warning (disable:4250)
#endif

class CEEJitInfo : public CEEInfo, public ICorJitInfo
{
public:

        // ICorJitInfo stuff
    HRESULT __stdcall allocMem (
            ULONG               codeSize,
            ULONG               roDataSize,
            ULONG               rwDataSize,
            void **             codeBlock,
            void **             roDataBlock,
            void **             rwDataBlock
            );

    HRESULT __stdcall alloc (
            ULONG code_len, unsigned char** ppCode,
            ULONG EHinfo_len, unsigned char** ppEHinfo,
            ULONG GCinfo_len, unsigned char** ppGCinfo
            );

    HRESULT __stdcall allocGCInfo (
            ULONG              size,
            void **             block
            );
    HRESULT __stdcall setEHcount (
            unsigned      cEH);
    void __stdcall setEHinfo (
            unsigned      EHnumber,
            const CORINFO_EH_CLAUSE* clause);

    void __stdcall getEHinfo(
            CORINFO_METHOD_HANDLE ftn,              /* IN  */
            unsigned      EHnumber,                 /* IN */
            CORINFO_EH_CLAUSE* clause               /* OUT */
            );

    BOOL _cdecl logMsg(unsigned level, const char* fmt, va_list args);
    virtual int doAssert(const char* szFile, int iLine, const char* szExpr);

    CorInfoHelpFunc __stdcall getNewHelper(CORINFO_CLASS_HANDLE newCls, CORINFO_METHOD_HANDLE scopeHnd);

    ICorJitInfo* getNewFuncletInfo();
    void         releaseFuncletInfo(ICorJitInfo* pInfo);


    CodeHeader* GetCodeHeader()
    {
        return m_CodeHeader;
    }

    CEEJitInfo(MethodDesc* fd, COR_ILMETHOD_DECODER* header, 
               IJitManager* jm, bool isStub = false)
      : m_jitManager(jm), m_FD(fd), m_CodeHeader(NULL), m_ILHeader(header), m_isStub(isStub) 
    {
    }

    //
    // overrides for JITing stubs
    //
    virtual CorInfoTypeWithMod __stdcall getArgType(
                CORINFO_SIG_INFO*           sig,
                CORINFO_ARG_LIST_HANDLE     args,
                CORINFO_CLASS_HANDLE*       vcTypeRet);

    virtual CORINFO_METHOD_HANDLE __stdcall findMethod(
                CORINFO_MODULE_HANDLE   scopeHnd, 
                unsigned                metaTOK, 
                CORINFO_METHOD_HANDLE   context);


private :
    IJitManager*            m_jitManager;   // responsible for allocating memory
    MethodDesc*             m_FD;           // method being compiled
    CodeHeader*             m_CodeHeader;   // descriptor for JITTED code
    COR_ILMETHOD_DECODER *  m_ILHeader;     // the code header as exist in the file
    OBJECTREF               pThrowable;     // pointer to exception object
    bool                    m_isStub;
};


/*********************************************************************/
/*********************************************************************/


typedef struct {
#ifdef _DEBUG
    CorInfoHelpFunc code;
#endif // _DEBUG
    void * pfnHelper;
    void * pfnStub;
//    short params;
    short flags;
} VMHELPDEF;

enum JIT_UTIL_FUNCS
{
        JIT_UTIL_MON_ENTER,
        JIT_UTIL_MON_TRY_ENTER,
        JIT_UTIL_MON_ENTER_STATIC,
        JIT_UTIL_MON_EXIT,
        JIT_UTIL_MON_CONTENTION,
        JIT_UTIL_CHKCASTBIZARRE,
        JIT_UTIL_ISINSTANCEBIZARRE
};

#if _X86_

class JIT_TrialAlloc
{
public:
    enum Flags
    {
        NORMAL       = 0x0,
        MP_ALLOCATOR = 0x1,
        SIZE_IN_EAX  = 0x2,
        OBJ_ARRAY    = 0x4,
        ALIGN8       = 0x8,     // insert a dummy object to insure 8 byte alignment (until the next GC)
        ALIGN8OBJ    = 0x10,
        NO_FRAME     = 0x20,    // call is from unmanaged code - don't try to put up a frame
    };

    static void *GenAllocSFast(Flags flags);
    static void *GenBox(Flags flags);
    static void *GenAllocArray(Flags flags);
    static void *GenAllocString(Flags flags);

private:
    static void EmitAlignmentRoundup(CPUSTUBLINKER *psl,X86Reg regTestAlign, X86Reg regToAdj, Flags flags);
    static void EmitDummyObject(CPUSTUBLINKER *psl, X86Reg regTestAlign, Flags flags);
    static void EmitCore(CPUSTUBLINKER *psl, CodeLabel *noLock, CodeLabel *noAlloc, Flags flags);
    static void EmitNoAllocCode(CPUSTUBLINKER *psl, Flags flags);

#if CHECK_APP_DOMAIN_LEAKS
    static void EmitSetAppDomain(CPUSTUBLINKER *psl);
    static void EmitCheckRestore(CPUSTUBLINKER *psl);
#endif
};

#endif

void *GenFastGetSharedStaticBase();


BOOL                 F_CALL_CONV ArrayStoreCheck(Object** pElement, PtrArray** pArray);
extern StringObject* F_CALL_CONV FramedAllocateString(DWORD stringLength);

BOOL ObjIsInstanceOf(Object *pElement, TypeHandle toTypeHnd);



extern LPVOID GetEEGp();

enum ILSTUB_SPECIAL_TOKENS
{
    TOKEN_ILSTUB_THIS_METHOD,
    TOKEN_ILSTUB_HELPER_STR_TO_ASCII,
    TOKEN_ILSTUB_MARSHAL_LAYOUTCLASS,
    TOKEN_ILSTUB_UNMARSHAL_LAYOUTCLASS,
    TOKEN_ILSTUB_MARSHAL_DELEGATE,
    TOKEN_ILSTUB_FREE_MARSHALER_MEMORY,
    TOKEN_ILSTUB_PIN_OBJECT,
    TOKEN_ILSTUB_UNPIN_OBJECT,
    TOKEN_ILSTUB_HELPER_SET_LAST_ERROR,
    
    TOKEN_ILSTUB_HELPER_CREATE_MARSHALER,
    TOKEN_ILSTUB_HELPER_DO_CONVERSION_3ARGS,
    TOKEN_ILSTUB_HELPER_BACK_PROPAGATE,
    TOKEN_ILSTUB_HELPER_BEGIN,
    TOKEN_ILSTUB_HELPER_END,

    NUM_ILSTUB_HELPERS
};

Object* __stdcall JIT_IsInstanceOfClassWorker(OBJECTREF objref, EEClass *pClass, BOOL bThrow);
Object* __stdcall JIT_AllocateObjectSpecial(CORINFO_CLASS_HANDLE typeHnd_);
Object* __stdcall JIT_NewCrossContextHelper(CORINFO_CLASS_HANDLE typeHnd_);
#ifdef _DEBUG
Object* __stdcall AllocObjectWrapper(MethodTable *pMT);
#endif

extern "C" VMHELPDEF hlpFuncTable[];
extern "C" VMHELPDEF utilFuncTable[];

#endif // JITINTERFACE_H
