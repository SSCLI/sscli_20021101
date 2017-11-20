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
// -*- C++ -*-
#ifndef _FJIT_H_
#define _FJIT_H_

#include "openum.h"
#include "stdmacros.h"

/*****************************************************************************/

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                            FJit.h                                         XX
XX                                                                           XX
XX   The functionality needed for the FJIT DLL.                              XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

// Macros to decrease the number of ifdef's in the code
#if defined(_DEBUG)
#define INDEBUG(x) x
#else
#define INDEBUG(x)
#endif

#define New(var, exp) \
    if ((var = new exp) == NULL) \
        RaiseException(SEH_NO_MEMORY,EXCEPTION_NONCONTINUABLE,0,NULL);

//the following macros allow the jit helpers to be called from c-code
#define jit_call __cdecl

void logMsg(class ICorJitInfo* info, unsigned logLevel, char* fmt, ...);
#ifdef _DEBUG
#define LOGMSG(x) logMsg x
#else
#define LOGMSG(x)	0
#endif

#include "ifjitcompiler.h"
#include "log.h" 		// for LL_INFO*

#define emit_WIN64(x)
#define emit_WIN32(x) x

#define SEH_ACCESS_VIOLATION STATUS_ACCESS_VIOLATION
#define SEH_NO_MEMORY	     STATUS_NO_MEMORY
#define SEH_JIT_REFUSED      0xE04a4954  // JIT

extern class FJitCompiler* ILJitter;        // The one and only instance of this JITer

/* the jit helpers that we call at runtime */
extern BOOL FJit_HelpersInstalled;
extern unsigned __int64 (__stdcall *FJit_pHlpLMulOvf) (unsigned __int64 val1, unsigned __int64 val2);

//extern float (__stdcall *FJit_pHlpFltRem) (float divisor, float dividend);
extern double (__stdcall *FJit_pHlpDblRem) (double divisor, double dividend);

extern void (jit_call *FJit_pHlpRngChkFail) (unsigned tryIndex);
extern void (jit_call *FJit_pHlpOverFlow) (unsigned tryIndex);
extern void (jit_call *FJit_pHlpInternalThrow) (CorInfoException throwEnum);
extern void (jit_call *FJit_pHlpInternalThrowFromHelper) (CorInfoException throwEnum);
extern CORINFO_Object (jit_call *FJit_pHlpArrAddr_St) (CORINFO_Object array, int index, CORINFO_Object value);
extern void (jit_call *FJit_pHlpInitClass) (CORINFO_CLASS_HANDLE cls);
extern CORINFO_Object (jit_call *FJit_pHlpNewObj) (CORINFO_MODULE_HANDLE scopeHnd, unsigned constrTok, int argN);
extern void (jit_call *FJit_pHlpThrow) (CORINFO_Object obj);
extern void (jit_call *FJit_pHlpRethrow) ();
extern void (jit_call *FJit_pHlpPoll_GC) ();
extern void (jit_call *FJit_pHlpMonEnter) (CORINFO_Object obj);
extern void (jit_call *FJit_pHlpMonExit) (CORINFO_Object obj);
extern void (jit_call *FJit_pHlpMonEnterStatic) (CORINFO_METHOD_HANDLE method);
extern void (jit_call *FJit_pHlpMonExitStatic) (CORINFO_METHOD_HANDLE method);
extern CORINFO_Object (jit_call *FJit_pHlpChkCast) (CORINFO_CLASS_HANDLE type, CORINFO_Object obj);
extern CORINFO_Object (jit_call *FJit_pHlpNewArr) (CorInfoType type, unsigned cElem);
extern void (jit_call *FJit_pHlpAssign_Ref)(CORINFO_Object** dst, CORINFO_Object* obj); // *dst = obj, inform GC
extern BOOL (jit_call *FJit_pHlpIsInstanceOf) (CORINFO_CLASS_HANDLE type, CORINFO_Object obj);
extern CORINFO_Object (jit_call *FJit_pHlpNewArr_1_Direct) (CORINFO_CLASS_HANDLE cls, unsigned cElem);
extern CORINFO_Object (jit_call *FJit_pHlpBox) (CORINFO_CLASS_HANDLE type, void * unboxedData);
extern void* (jit_call *FJit_pHlpUnbox) (CORINFO_CLASS_HANDLE type, CORINFO_Object obj);
extern void* (jit_call *FJit_pHlpGetField32) (CORINFO_Object*, CORINFO_FIELD_HANDLE);
extern __int64 (jit_call *FJit_pHlpGetField64) (CORINFO_Object*, CORINFO_FIELD_HANDLE);
extern void* (jit_call *FJit_pHlpGetFieldObj) (CORINFO_Object*, CORINFO_FIELD_HANDLE);
extern void (jit_call *FJit_pHlpSetField32) (CORINFO_Object*, CORINFO_FIELD_HANDLE , __int32);
extern void (jit_call *FJit_pHlpSetField64) (CORINFO_Object*, CORINFO_FIELD_HANDLE , __int64);
extern void (jit_call *FJit_pHlpSetFieldObj) (CORINFO_Object*, CORINFO_FIELD_HANDLE , LPVOID);
extern void* (jit_call *FJit_pHlpGetFieldAddress) (CORINFO_Object*, CORINFO_FIELD_HANDLE);

extern void (jit_call *FJit_pHlpGetRefAny) (CORINFO_CLASS_HANDLE cls, void* refany);
extern void (jit_call *FJit_pHlpEndCatch) ();
extern void (jit_call *FJit_pHlpPinvokeCalli) ();
extern void (jit_call *FJit_pHlpTailCall) ();
extern void (jit_call *FJit_pHlpWrap) ();
extern void (jit_call *FJit_pHlpUnWrap) ();
extern void (jit_call *FJit_pHlpBreak) ();
extern CORINFO_MethodPtr* (jit_call *FJit_pHlpEncResolveVirtual) (CORINFO_Object*, CORINFO_METHOD_HANDLE);
extern void (jit_call *FJit_pHlpVerification) (int offset);

#define THROW_FROM_HELPER(exceptNum)  {                 \
        FJit_pHlpInternalThrowFromHelper(exceptNum);    \
        return; }           /* need return so we can decode the epilog */

#define THROW_FROM_HELPER_RET(exceptNum) {              \
        FJit_pHlpInternalThrowFromHelper(exceptNum);    \
        return 0; }         /* Need return so we can decode the epilog */

    // OpType encodes all we need to know about type on the opcode stack
enum OpTypeEnum {             // colapsing the CorInfoType into the catagories I care about
    typeError    = 0,         // this encoding is reserved for value classes
    typeByRef    = 1,
    typeRef      = 2,
    typeU1       = 3,
    typeU2       = 4,
    typeI1       = 5,
    typeI2       = 6,
    typeI4       = 7,
    typeI8       = 8,
    typeR4       = 9,
    typeR8       = 10,
    typeRefAny   = 11,         
    typeValClass = 12,
    typeMethod   = 13,
    typeCount    = 14,
    typeI      = typeI4,
    };

#define RefAnyClassHandle ((CORINFO_CLASS_HANDLE) typeRefAny)

struct OpType {
    OpType() {
#ifdef _DEBUG
        type_enum = typeError;
#endif
        type_handle = 0;
        }

    OpType(OpTypeEnum opEnum) {
        type_enum    = opEnum;
        type_handle  = 0;
        _ASSERTE(type_enum < typeCount);
        }

    explicit OpType(CORINFO_CLASS_HANDLE valClassHandle) {
        type_handle = valClassHandle;
        type_enum   = typeValClass;
        }

    explicit OpType(CORINFO_METHOD_HANDLE mHandle) {
        type_enum     = typeMethod;
        method_handle = mHandle;
        }

    OpType(OpTypeEnum opEnum, CORINFO_CLASS_HANDLE valClassHandle ) {
        type_enum    = opEnum;
        if ( !isPrimitive() || isRef() || isByRef() )
          type_handle  = valClassHandle;
        else
          type_handle  = NULL;
        _ASSERTE(type_enum < typeCount);
    }

    OpType(OpTypeEnum opEnum, OpTypeEnum childEnum) {
        type_enum    = opEnum;
        child_type   = childEnum;
        _ASSERTE(type_enum == typeByRef && child_type > typeRef || 
                 type_enum == typeRef   && child_type == typeRef );
    }

    explicit OpType(CorInfoType jitType, CORINFO_CLASS_HANDLE valClassHandle) {
            type_enum   = OpType(jitType).enum_();
            if ( !isPrimitive() || isRef() || isByRef() )
          type_handle  = valClassHandle;
        else
          type_handle  = NULL;
        _ASSERTE(type_enum < typeCount);
    }

    explicit OpType(CorInfoType jitType) {
        _ASSERTE(jitType < CORINFO_TYPE_COUNT);
        static const char toOpStackType[] = {
            typeError,   //CORINFO_TYPE_UNDEF
            typeError,   //CORINFO_TYPE_VOID
            typeI1,      //CORINFO_TYPE_BOOL
            typeU2,      //CORINFO_TYPE_CHAR
            typeI1,      //CORINFO_TYPE_BYTE
            typeU1,      //CORINFO_TYPE_UBYTE
            typeI2,      //CORINFO_TYPE_SHORT
            typeU2,      //CORINFO_TYPE_USHORT
            typeI4,      //CORINFO_TYPE_INT
            typeI4,      //CORINFO_TYPE_UINT
            typeI8,      //CORINFO_TYPE_LONG
            typeI8,      //CORINFO_TYPE_ULONG
            typeR4,      //CORINFO_TYPE_FLOAT
            typeR8,      //CORINFO_TYPE_DOUBLE
            typeRef,     //CORINFO_TYPE_STRING
            typeI,       //CORINFO_TYPE_PTR
            typeByRef,   //CORINFO_TYPE_BYREF
            typeValClass,   //CORINFO_TYPE_VALUECLASS
            typeRef,     //CORINFO_TYPE_CLASS
            typeRefAny,  //CORINFO_TYPE_REFANY
        };
        _ASSERTE((typeI4 > typeI2) && (typeI1 > typeU1) && (typeU2 > typeU1));
        _ASSERTE(toOpStackType[CORINFO_TYPE_REFANY] == typeRefAny);  //spot check table
        _ASSERTE(toOpStackType[CORINFO_TYPE_BYREF] == typeByRef);    //spot check table
        _ASSERTE(toOpStackType[CORINFO_TYPE_LONG] == typeI8);        //spot check table
        _ASSERTE(sizeof(toOpStackType) == CORINFO_TYPE_COUNT);
        type_enum = (OpTypeEnum) toOpStackType[jitType];
        type_handle       = 0;
	if (type_enum == typeError)
	  {  _ASSERTE(!"Bad input to the constructor"); 
            RaiseException(SEH_JIT_REFUSED,EXCEPTION_NONCONTINUABLE,0,NULL); } //_ASSERTE(type_enum != typeError);
    }
    /* OPERATORS */
    int operator==(const OpType& opType) { return(type_handle == opType.type_handle && type_enum == opType.type_enum ); }
    int operator!=(const OpType& opType) { return(type_handle != opType.type_handle || type_enum != opType.type_enum ); }
    /* ACCESSORS */
    bool isPtr() { return(type_enum == typeRef || type_enum == typeByRef || type_enum == typeI ); }
    bool isPrimitive()      { return((unsigned) type_enum <= (unsigned) typeRefAny); }    // refany is a primitive
    bool isValClass()       { return((unsigned) type_enum >= (unsigned) typeRefAny); }    // refany is a valclass too 
    bool isTargetPrimitive() { return((unsigned) child_type <= (unsigned) typeRefAny); }
    inline bool isNull()   { return  (child_type == typeRef && type_enum == typeRef); }
    inline bool isRef()    { return (type_enum == typeRef); }
    inline bool isRefAny() { return (type_enum == typeRefAny); }
    inline bool isByRef()  { return (type_enum == typeByRef); }
    inline bool isMethod() { return (type_enum == typeMethod); }
    inline OpTypeEnum            enum_() { return ( type_enum ); }
    inline CORINFO_CLASS_HANDLE  cls()   { return ( type_handle ); } 
    inline CORINFO_METHOD_HANDLE getMethod()   { return ( method_handle ); }
    inline OpTypeEnum targetAsEnum() { return child_type; }
    OpType getTarget() 
      {  return ( isTargetPrimitive() ? OpType( child_type ) : OpType( type_handle )); }
    bool matchTarget( OpType other )
	{ _ASSERTE( type_enum == typeByRef ); return isTargetPrimitive() ? other.enum_() == targetAsEnum() : other.cls() == cls(); }  
    /* MUTATORS */
    void fromInt(unsigned i){ type_handle = (CORINFO_CLASS_HANDLE) i; }   // unsafe, please limit use
    void setHandle(CORINFO_CLASS_HANDLE h)  { type_handle =  h; } 
    void setTarget( OpTypeEnum opEnum, CORINFO_CLASS_HANDLE h )
      {  if ( h == NULL ) child_type = opEnum; else type_handle = h;  
         _ASSERTE( (child_type != typeByRef && child_type != typeRef) || isNull() );}
    void setTarget( CorInfoType jitType, CORINFO_CLASS_HANDLE h )
      {  if ( h == NULL ) child_type = OpType(jitType).enum_(); else type_handle = h; 
         _ASSERTE( (child_type != typeByRef && child_type != typeRef) || isNull() );}
    void init(OpTypeEnum opEnum, CORINFO_CLASS_HANDLE valClassHandle )
      {  type_enum = opEnum; type_handle = valClassHandle; }
    void init(CorInfoType jitType, CORINFO_CLASS_HANDLE valClassHandle )
      {  type_enum = OpType(jitType).enum_(); type_handle = valClassHandle; }
    void toSigned() {
       static const OpTypeEnum Signed[] = {
            typeError , // typeError,   
            typeByRef , // typeByRef,   
            typeRef,    // typeRef,
            typeI1,     // typeU1,      
            typeI2,     // typeU2,                      
        };
    if (type_enum < typeI1)
         type_enum = Signed[type_enum];
    }

    void toNormalizedType() {    
        static const OpTypeEnum Normalize[] = {
            typeError , // typeError,   
            typeByRef , // typeByRef,   
            typeRef, // typeRef,
            typeI4, // typeU1,      
            typeI4, // typeU2,      
            typeI4, // typeI1,      
            typeI4, // typeI2,                
        };
    if (type_enum < typeI4)
         type_enum = Normalize[type_enum];
    }

    void toFPNormalizedType() {    
        static const OpTypeEnum Normalize[] = {
            typeError , // typeError,   
            typeByRef , // typeByRef,   
            typeRef,    // typeRef,
            typeI4,     // typeU1,      
            typeI4,     // typeU2,      
            typeI4,     // typeI1,      
            typeI4,     // typeI2,                
            typeI4,     // typeI4,                
            typeI8,     // typeI8,                
            typeR8,     // typeR4,                
        };
    if ( type_enum < typeR8)
          type_enum = Normalize[type_enum];
    } 
  
  // Data structure  
    OpTypeEnum type_enum;
    union {
            // Valid only for STRUCT or REF or BYREF
        CORINFO_CLASS_HANDLE  type_handle;  
            // Valid only for type METHOD 
        CORINFO_METHOD_HANDLE method_handle;
            // Valid for BYREF to primitives only
        OpTypeEnum            child_type;
    };
};

struct stackItems {
    int      offset :24;    // Only used if isReg - false
    unsigned regNum : 7;    // Only need 2 bits, only used if isReg = true
    unsigned isReg  : 1;
    OpType   type;
};

    // Note that we presently rely on the fact that statkItems and
    // argInfo have the same layout
struct argInfo {
    unsigned size   :24;      // Only used if isReg - false, size of this arg in bytes
    unsigned regNum : 7;      // Only need 2 bits, only used if isReg = true
    unsigned isReg  : 1;
    OpType   type;
};

struct CallStateInfo {
    unsigned EnregSize;
    unsigned StackSizeInSlots;
    void reset() { EnregSize = 0; StackSizeInSlots = 0;  }   
};


#define LABEL_NOT_FOUND (unsigned int) (0-1)

class LabelTable {
public:

    LabelTable();

    ~LabelTable();

    /* add a label at an il offset with a stack signature */
    void add(unsigned int ilOffset, OpType* op_stack, unsigned int op_stack_len, bool overwrite = false);

    /* find a label token from an il offset */
    unsigned int findLabel(unsigned int ilOffset);

    /* set operand stack from a label token, return the size of the stack */
    unsigned int setStackFromLabel(unsigned int labelToken, OpType* op_stack, unsigned int op_stack_size);

    /* get stack size from label */
    unsigned int getStackSize( unsigned int labelToken );
    
    /* reset table to empty */
    void reset();

private:
    struct label_table {
        unsigned int ilOffset;
        unsigned int stackToken;
    };
    unsigned char*  stacks;         //compressed buffer of stacks laid end to end
    unsigned int    stacks_size;    //allocated size of the compressed buffer
    unsigned int    stacks_len;     //num bytes used in the compressed buffer
    label_table*    labels;         //array of labels, in il offset sorted order
    unsigned int    labels_size;    //allocated size of the label table
    unsigned int    labels_len;     //num of labels in the table

    /* find the offset at which the label exists or should be inserted */
    unsigned int searchFor(unsigned int ilOffset);

    /* write an op stack into the stacks buffer, return the offset into the buffer where written */
    unsigned int compress(OpType* op_stack, unsigned int op_stack_len);

    /* grow the stacks buffer */
    void growStacks(unsigned int new_size);

    /* grow the labels array */
    void growLabels();
};

class StackEncoder {
private:
    struct labeled_stacks {
        unsigned pcOffset;
        unsigned int stackToken;
    };
    OpType*         last_stack;         //last stack encoded
    unsigned int    last_stack_len;     //logical length
    unsigned int    last_stack_size;    //allocated size
    labeled_stacks* labeled;            //array of pc offsets with stack descriptions
    unsigned int    labeled_len;        //logical length
    unsigned int    labeled_size;       //allocated length
    unsigned char*  stacks;             //buffer for holding compressed stacks
    unsigned int    stacks_len;         //logical length
    unsigned int    stacks_size;        //allocated size
    unsigned char*  gcRefs;             //temp buffers used by encodeStack, reused to reduce allocations
    unsigned char*  interiorRefs;       //  ditto
    unsigned int    gcRefs_len;         //
    unsigned int    interiorRefs_len;   //
    unsigned int    gcRefs_size;        //
    unsigned int    interiorRefs_size;  //  ditto

    /* encode the stack into the stacks buffer, return the index where it was placed */
    unsigned int encodeStackForGC(OpType* op_stack, unsigned int op_stack_len);


public:

    ICorJitInfo*       jitInfo;            //see corjit.h

    StackEncoder();

    ~StackEncoder();

    /* reset so we can be reused */
    void reset();

    /* append the stack state at pcOffset to the end */
    void append(unsigned int pcOffset, OpType* op_stack, unsigned int op_stack_len);

    /* compress the labeled stacks in gcHdrInfo format */
    void compress(unsigned char** buffer, unsigned int* buffer_len, unsigned int* buffer_size);
#ifdef _DEBUG
    void StackEncoder::PrintStacks(FJit_Encode* mapping);
    void StackEncoder::PrintStack(const char* name, unsigned char *& inPtr);
#endif
};

//*************************************************************************************************
class FixupTable {
public:
    FixupTable();
    ~FixupTable();
    CorJitResult  insert(void** pCodeBuffer);	  // inserts an entry in fixup table for jump to target at pCodeBuffer
    void  FixupTable::adjustMap(int delta) ;
    CorJitResult resolve(FJit_Encode* mapping, BYTE* startAddress, BOOL& JitAgain); // applies fix up to all entries in table
    int   verifyTargets( FJit_Encode* mapping );  // verifies that all ILOffsets correspond to non-zero PC offsets 
    void  setup();
    inline void setSavedIP( unsigned char* savedIP) { storedStartIP = savedIP; }
private:
	unsigned*	relocations;
	unsigned	relocations_len;
	unsigned	relocations_size;
        unsigned char*  storedStartIP;                           // used to produce relocatable absolute jumps 
};

#ifdef LOGGING
extern class ConfigMethodSet fJitCodeLog;
#define MIN_CODE_BUFFER_RESERVED_SIZE   (65536*16)
#else
#define MIN_CODE_BUFFER_RESERVED_SIZE   (65536*4)
#endif

	/* this is all the information that the FJIT keeps track of for every IL instruction.
	   Note that this structure takes only 1 bytes at present  */
struct FJitState {
	bool isJmpTarget	: 1;		// This is a target of a jump
	bool isTOSInReg		: 1;		// the top of the stack is in a register
        bool isTry              : 1;            // This is a beginning of a try block
	bool isHandler		: 1;		// This is the begining of a handler
	bool isFilter		: 1;		// This is a filter entry point
        bool isJitted           : 1;            // This IL offset has already been jitted
        bool isEndBlock         : 1;            // Marks an end of a try/handler block
};

#include "fjitverifier.h"

// These are error codes returned by FJit::jitCompile
enum FJitResult
{
    // Note that I dont use FACILITY_NULL for the facility number,
    // we may want to get a 'real' facility number
    FJIT_OK            =     NO_ERROR,
    FJIT_BADCODE       =     CORJIT_BADCODE,
    FJIT_OUTOFMEM      =     CORJIT_OUTOFMEM,
    FJIT_INTERNALERROR =     CORJIT_INTERNALERROR,
    FJIT_SKIPPED       =     CORJIT_SKIPPED,
    FJIT_VERIFICATIONFAILED,
    FJIT_JITAGAIN,
};

/*  Since there is only a single FJit instance, an instance of this class holds
    all of the compilation specific data
    */

class FJit {

public:
    FJitResult jitCompile(
                 BYTE **         entryAddress,
                 unsigned *      codeSize               /* IN/OUT */
                 );


    /* pass debugging information to the Runtime (eventually gets to the debugger. */
    void reportDebuggingData(CORINFO_SIG_INFO* sigInfo);

    /* get and initialize a compilation context to use for compiling */
    static FJit* GetContext(
        ICorJitInfo*       comp,
        CORINFO_METHOD_INFO* methInfo,
        DWORD           dwFlags
        );
    /* return a compilation context to the free list */
    void ReleaseContext();

    /* make sure the list of available compilation contexts is initialized at startup */
    static BOOL Init();

    /* release all of the compilation contexts at shutdown */
    static void Terminate();

    /* compute the size of an argument based on machine chip */
    unsigned int computeArgSize(CorInfoType argType, CORINFO_ARG_LIST_HANDLE argSig, CORINFO_SIG_INFO* sig);

    /* answer true if this arguement is enregistered on a machine chip */
    bool enregisteredArg(CorInfoType argType);

    /* compute the argument offsets based on the machine chip, returns total size of all the arguments */
    unsigned computeArgInfo(CORINFO_SIG_INFO* jitSigIfo, argInfo* argMap,CORINFO_CLASS_HANDLE thisCls, unsigned & enregSize);

    /* initializes state with information about exception handling */
    void initializeExceptionHandling();

    /* compress the gc info into gcHdrInfo and answer the size in bytes */
    unsigned int compressGCHdrInfo();
 
    /* reset the state array */ 
    void resetStateArray();
   
    /* grow an unsigned char[] array by allocating a new one and copying the old values into it, return the size of the new array */
    static unsigned growBuffer(unsigned char** chars, unsigned chars_len, unsigned new_chars_size);
    
    /* manipuate the opcode stack */
    OpType& topOp(unsigned back = 0);
    inline OpTypeEnum topOpE( unsigned back = 0 ) { return topOp( back ).enum_(); } 
    void popOp(unsigned cnt = 1);
    void pushOp(OpType type);
    bool isOpStackEmpty();
    void resetOpStack();
    int  mergeStacks( unsigned int size1, OpType* stack1, unsigned int size2, OpType* stack2, bool StoredStack1);

    FJit(ICorJitInfo* comp);

    void resetContextState(bool ResetVerFlags = true);       // resets all state info so the method can be rejitted 

    ~FJit();
#ifdef _DEBUG
void FJit::displayGCMapInfo();
#endif // _DEBUG

//
// Public members
//
public:
    CORINFO_METHOD_INFO*methodInfo; //see corjit.h
    unsigned int    opStack_len;    //number of operands on the opStack
    FJit_Encode*    mapping;        //il to pc mapping
    unsigned char*  gcHdrInfo;      //compressed gcInfo for FJIT_EETwain.cpp
    unsigned int    gcHdrInfo_len;  //num compressed bytes
    Fjit_hdrInfo    mapInfo;        //header info passed to the code manager (FJIT_EETwain) to do stack walk
    int             ver_failure;    //flag to indicate that the verification has failed 
    unsigned char*  codeBuffer;     // buffer in which code in initially compiled
    unsigned        codeBufferCommittedSize; // size of Buffer committed
    unsigned        EHBuffer_size;      // size of EHBuffer
    unsigned char*  EHBuffer;           
    FixupTable*     fixupTable;
    FJitResult      codeGenState; // Passive way to report failure from low level macros

#if defined(_DEBUG) || defined(LOGGING)
    bool codeLog;
#endif

//
// Private Methods
//
private:
    /*adjust the internal mem structs as needed for the size of the method being jitted*/
    void ensureMapSpace();

    /* initialize the compilation context with the method data */
    void setup();

    /* compute the locals map for the method being compiled */
    void computeLocalOffsets();

    /* compute the offset of the start of the local */
    int localOffset(unsigned base, unsigned size);

    // Helper function to help compile specific IL instructions
    FJitResult compileCEE_MUL();
    FJitResult compileCEE_MUL_OVF();
    FJitResult compileCEE_MUL_OVF_UN();
    FJitResult compileCEE_ADD();
    FJitResult compileCEE_ADD_OVF();
    FJitResult compileCEE_ADD_OVF_UN();
    FJitResult compileCEE_SUB();
    FJitResult compileCEE_SUB_OVF();
    FJitResult compileCEE_SUB_OVF_UN();
    FJitResult compileCEE_DIV();
    FJitResult compileCEE_DIV_UN();
    FJitResult compileCEE_REM();
    FJitResult compileCEE_REM_UN();
    FJitResult compileCEE_LDIND_U1();
    FJitResult compileCEE_LDIND_U2();
    FJitResult compileCEE_LDIND_U4();
    FJitResult compileCEE_LDIND_I1();
    FJitResult compileCEE_LDIND_I2();
    FJitResult compileCEE_LDIND_I4();
    FJitResult compileCEE_LDIND_I8();
    FJitResult compileCEE_LDIND_R4();
    FJitResult compileCEE_LDIND_R8();
    FJitResult compileCEE_LDIND_I();
    FJitResult compileCEE_LOCALLOC();
    FJitResult compileCEE_NEG();
    FJitResult compileCEE_LDIND_REF();
    FJitResult compileCEE_LDC_I8();
    FJitResult compileCEE_LDC_R8();
    FJitResult compileCEE_LDC_R4();
    FJitResult compileCEE_LDNULL();
    FJitResult compileCEE_LDSTR();
    FJitResult compileCEE_CPBLK();
    FJitResult compileCEE_INITBLK();
    FJitResult compileCEE_CPOBJ();
    FJitResult compileCEE_INITOBJ();
    FJitResult compileCEE_LDOBJ();
    FJitResult compileCEE_STOBJ();
    FJitResult compileCEE_STIND_I1();
    FJitResult compileCEE_STIND_I2();
    FJitResult compileCEE_STIND_I4();
    FJitResult compileCEE_STIND_I8();
    FJitResult compileCEE_STIND_I();
    FJitResult compileCEE_STIND_R4();
    FJitResult compileCEE_STIND_R8();
    FJitResult compileCEE_STIND_REF();
    FJitResult compileCEE_MKREFANY();
    FJitResult compileCEE_SIZEOF();
    FJitResult compileCEE_STELEM_I1();
    FJitResult compileCEE_STELEM_I2();
    FJitResult compileCEE_STELEM_I4();
    FJitResult compileCEE_STELEM_I8();
    FJitResult compileCEE_STELEM_I();
    FJitResult compileCEE_STELEM_R4();
    FJitResult compileCEE_STELEM_R8();
    FJitResult compileCEE_STELEM_REF();
    FJitResult compileCEE_LDELEM_U1();
    FJitResult compileCEE_LDELEM_U2();
    FJitResult compileCEE_LDELEM_U4();
    FJitResult compileCEE_LDELEM_I1();
    FJitResult compileCEE_LDELEM_I2();
    FJitResult compileCEE_LDELEM_I4();
    FJitResult compileCEE_LDELEM_I8();
    FJitResult compileCEE_LDELEM_I();
    FJitResult compileCEE_LDELEM_R4();
    FJitResult compileCEE_LDELEM_R8();
    FJitResult compileCEE_LDELEM_REF();
    FJitResult compileCEE_LDELEMA();
    FJitResult compileCEE_CEQ();
    FJitResult compileCEE_CGT();
    FJitResult compileCEE_CGT_UN();
    FJitResult compileCEE_CLT();
    FJitResult compileCEE_CLT_UN();
    FJitResult compileCEE_AND();
    FJitResult compileCEE_OR();
    FJitResult compileCEE_XOR();
    FJitResult compileCEE_NOT();
    FJitResult compileCEE_SHR();
    FJitResult compileCEE_SHR_UN();
    FJitResult compileCEE_SHL();
    FJitResult compileCEE_DUP();
    FJitResult compileCEE_POP();
    FJitResult compileCEE_CASTCLASS();
    FJitResult compileCEE_CONV_I1();
    FJitResult compileCEE_CONV_I2();
    FJitResult compileCEE_CONV_I4();
    FJitResult compileCEE_CONV_U1();
    FJitResult compileCEE_CONV_U2();
    FJitResult compileCEE_CONV_U4();
    FJitResult compileCEE_CONV_I8();
    FJitResult compileCEE_CONV_U8();
    FJitResult compileCEE_CONV_R4();
    FJitResult compileCEE_CONV_R8();
    FJitResult compileCEE_CONV_R_UN();
    FJitResult compileCEE_CONV_OVF_I1();
    FJitResult compileCEE_CONV_OVF_U1();
    FJitResult compileCEE_CONV_OVF_I2();
    FJitResult compileCEE_CONV_OVF_U2();
    FJitResult compileCEE_CONV_OVF_I4();
    FJitResult compileCEE_CONV_OVF_U4();
    FJitResult compileCEE_CONV_OVF_I8();
    FJitResult compileCEE_CONV_OVF_U8();
    FJitResult compileCEE_CONV_OVF_I1_UN();
    FJitResult compileCEE_CONV_OVF_U1_UN();
    FJitResult compileCEE_CONV_OVF_I2_UN();
    FJitResult compileCEE_CONV_OVF_U2_UN();
    FJitResult compileCEE_CONV_OVF_I4_UN();
    FJitResult compileCEE_CONV_OVF_U4_UN();
    FJitResult compileCEE_CONV_OVF_I8_UN();
    FJitResult compileCEE_CONV_OVF_U8_UN();
    FJitResult compileCEE_LDTOKEN();
    FJitResult compileCEE_BOX();
    FJitResult compileCEE_UNBOX();
    FJitResult compileCEE_ISINST();
    FJitResult compileCEE_JMP();
    FJitResult compileCEE_RET();
    FJitResult compileCEE_CALL();
    FJitResult compileCEE_CALLI();
    FJitResult compileCEE_CALLVIRT();
    FJitResult compileCEE_TAILCALL();
    FJitResult compileCEE_ENDFINALLY();
    FJitResult compileCEE_LDLEN();
    FJitResult compileCEE_NEWARR();
    FJitResult compileCEE_NEWOBJ();
    FJitResult compileCEE_THROW();
    FJitResult compileCEE_RETHROW();
    FJitResult compileCEE_SWITCH();
    FJitResult compileCEE_REFANYVAL();
    FJitResult compileCEE_REFANYTYPE();
    FJitResult compileCEE_ARGLIST();
    FJitResult compileCEE_LDFTN();
    FJitResult compileCEE_LDVIRTFTN();
    FJitResult compileCEE_ENDFILTER();

    FJitResult compileCEE_STFLD(OPCODE opcode);
    FJitResult compileCEE_LDFLD(OPCODE opcode);
    FJitResult compileCEE_LDFLDA(OPCODE opcode);
    FJitResult compileDO_LEAVE();
    FJitResult compileDO_LDFTN(CORINFO_METHOD_HANDLE targetMethod);
    FJitResult compileDO_LDARG(OPCODE opcode, unsigned offset);
    FJitResult compileDO_LDARGA(OPCODE opcode, unsigned offset);
    FJitResult compileDO_LDLOC(OPCODE opcode, unsigned offset);
    FJitResult compileDO_LDLOCA(OPCODE opcode, unsigned offset);
    FJitResult compileDO_LDVAR(OPCODE opcode, stackItems* varInfo);
    FJitResult compileDO_LDVARA(OPCODE opcode, stackItems* varInfo);
    FJitResult compileDO_LDIND_BYTYPE(OpType trackedType);
    FJitResult compileDO_STARG(unsigned offset);
    FJitResult compileDO_STLOC(unsigned offset);
    FJitResult compileDO_BR_boolean(int op);
    FJitResult compileDO_BR();
    FJitResult compileDO_JMP(int op);
    FJitResult compileDO_CEE_BEQ();
    FJitResult compileDO_CEE_BNE();
    FJitResult compileDO_CEE_BGT();
    FJitResult compileDO_CEE_BGT_UN();
    FJitResult compileDO_CEE_BGE();
    FJitResult compileDO_CEE_BGE_UN();
    FJitResult compileDO_CEE_BLT();
    FJitResult compileDO_CEE_BLT_UN();
    FJitResult compileDO_CEE_BLE();
    FJitResult compileDO_CEE_BLE_UN();

    FJitResult compileDO_PUSH_CALL_RESULT(  unsigned int argBytes, unsigned int stackPad,
                                            unsigned int token,
                                            CORINFO_SIG_INFO targetSigInfo,
                                            CORINFO_CLASS_HANDLE targetClass);
    FJitResult compileEpilog(unsigned argsTotalSize);

    /* emit helpers */
    unsigned emit_valClassCopy(CORINFO_CLASS_HANDLE valClass);
    void emit_valClassStore(CORINFO_CLASS_HANDLE valClass);
    void emit_valClassLoad(CORINFO_CLASS_HANDLE valClass);
    void emit_copyPtrAroundValClass(CORINFO_CLASS_HANDLE valClass);

    void getEnclosingClause(unsigned nextIP, CORINFO_EH_CLAUSE * retClause, int NotTry, unsigned & Start, unsigned & End );
    void pushHandlerOffsets(unsigned ilOffset);
    void setupForTailcall(CORINFO_SIG_INFO  & CallerSigInfo, CORINFO_SIG_INFO & TargetSigInfo,
                            int & stackSizeCaller, int & stackSizeTarget, int & flags);
    void storeEnregisteredArguments();
    void alignArguments();

    unsigned int Compute_EH_NestingLevel(unsigned ilOffset);
    void resetState( bool clearStacks );

    // Verification helper methods
    int verIsByRefLike( OpType Obj );
    int verifyStacks( unsigned int nextIP, int store_result );
    int verifyArrayAccess( OpType & ResultType );
    int verifyArrayLoad( OpTypeEnum LoadType, OpType & ResultType );
    int verifyArrayStore( OpTypeEnum StoreType, OpType & ResultType );
    int verifyArguments( CORINFO_SIG_INFO  & sig, int popCount, bool tailCall );
    int verifyThisPtr( CORINFO_CLASS_HANDLE & instanceClassHnd,  CORINFO_CLASS_HANDLE targetClass, 
                        int popCount, bool tailCall );
    int verifyDelegate( CORINFO_SIG_INFO  & sig, CORINFO_METHOD_HANDLE methodHnd, unsigned char* codePtr, 
                        unsigned DelStartDelta,int popCount);
    int verifyHandlers();
    FJitResult verificationFailure( INDEBUG(char * ErrorMessage) );

    FJitResult jitCompileVerificationThrow();
    OpType getTypeFromSig(CORINFO_SIG_INFO & sig, CORINFO_ARG_LIST_HANDLE args );


    /* rearrange the stack & regs to match the calling convention for the chip, return the number of parameters, inlcuding <this> */
    enum BuildCallFlags {
        CALL_NONE       = 0,
        CALL_THIS_LAST  = 1,
        CALL_TAIL       = 2,
        CALLI_UNMGD     = 4,
    };


    unsigned buildCall(CORINFO_SIG_INFO* sigInfo, BuildCallFlags flags, unsigned int & stackPadorRetBase );

//
// Private members
//
private:
    ICorJitInfo*    jitInfo;        //interface to EE, passed in when compilation starts
    DWORD           flags;          //compilation directives
    unsigned int    methodAttributes;//see corjit.h
    stackItems*     localsMap;      //local to stack offset map
    unsigned int    localsFrameSize;//total size of locals allocated in frame
    unsigned int    JitGeneratedLocalsSize; // for tracking esp distance on locallocs, and exceptions
    unsigned int    args_len;       //number of args (including this)
    stackItems*     argsMap;        //args to stack offset/reg map, offset <0 means enregisterd
    unsigned int    argsFrameSize;  //total size of args actually pushed on stack
    OpType*         opStack;        //operand stack
    OpType*         tempOpStack;    //temporary operand stack used for merge operations
    unsigned        opStack_size;   //allocated length of the opStack array
    unsigned        tempOpStack_size;//size of the temporary operand stack
    FJitState*      state;          //Information I need for every IL instruction
    LabelTable      ver_stacks;     //stacks stored for the verification 
    int             ver_failure_offset;// IL offset where verification failure occured
    SplitOffsetStack SplitOffsets;  // Offsets at which splits occured
    StackEncoder    stacks;         //labeled stacks table for call sites (pending args)
    // the following buffer is used by the EJIT at setup  and reused at jit time for
    // certain IL instructions, e.g. CPOBJ
    unsigned        localsGCRef_len;   //num of sizeof(void*) words in gc ref tail sig locals array
    unsigned char*  localsGCRef;       //true iff that word contains a GC ref
    unsigned        localsGCRef_size;  //allocated length of the localsGCRef array
    unsigned        codeBufferReservedSize; // size of Buffer reserved
    CallStateInfo   callInfo;          //used by the macros to construct helper calls

    BYTE ** entryAddress;
    unsigned * codeSize;
    unsigned char * inPtr;                                   // current IL offset 
    unsigned char * inBuff;
    unsigned char * inBuffEnd;
    DWORD         * sequencePointOffsets;
    unsigned char*  outPtr;                                  // x86 macros write here
    unsigned char*  outBuff;
    unsigned char*  storedStartIP;                           // used to produce relocatable absolute jumps          
    signed          ilrel;
    unsigned        DelegateStart;
    unsigned        InstStart;
    ULONG32         cSequencePoints;
    unsigned int    offsetVarArgToken;
    CORINFO_EH_CLAUSE       currentClause;
    // Global flags
    bool            TailCallForbidden; // if set, no tailcalls allowed
    bool            JitVerify;
    // State Flags
    bool inRegTOS;          // top of the stack is in a register
    bool LeavingTryBlock;   // Executing a "leave" from a try block
    bool LeavingCatchBlock; // Executing a "leave" from a catch block
    bool FinishedJitting;   // Are we done jitting
    bool controlContinue;   // does control we fall thru to next il instr
    bool UncondBranch;      // Executing an unconditional branch
    bool popSplitStack;     // Start jitting at the next offset on the split stack
    bool MadeTailCall;      // if a tailcall has been made
#ifdef _DEBUG
    bool didLocalAlloc;     // Has a local alloc been executed yet
#endif

#if defined(_DEBUG) || defined(LOGGING)
    const char* szDebugClassName;
    const char* szDebugMethodName;
#endif

    unsigned        OFFSET_OF_INTERFACE_TABLE;    // this is an EE constant and is being cached for performance

    unsigned state_size;            //allocated length of the state array
    unsigned locals_size;           //allocated length of the localsMap array
    unsigned args_size;             //allocated length of the argsMap array
    unsigned interiorGC_len;        //num of sizeof(void*) words in interior ptr tail sig locals array
    unsigned char* interiorGC;      //true iff that word contains a possibly interior ptr
    unsigned interiorGC_size;       //allocated length of the interiorGC array
    unsigned pinnedGC_len;          //num of sizeof(void*) words in pinnedGC array
    unsigned char* pinnedGC;        //true iff that word contains a pinned gc ref
    unsigned pinnedGC_size;         //allocated length of the pinnedGC array
    unsigned pinnedInteriorGC_len;  //num of sizeof(void*) words in pinnedInteriorGC array
    unsigned char* pinnedInteriorGC;//true iff that word contains a pinned interior ptr
    unsigned pinnedInteriorGC_size; //allocated length of the pinnedInteriorGC array
    unsigned int    gcHdrInfo_size; //size of compression buffer


};

#define FAILED_VALIDATION    0
#define FAILED_VERIFICATION  1
#define SUCCESS_VERIFICATION 2

#define MERGE_STATE_FAIL     0
#define MERGE_STATE_SUCCESS  1
#define MERGE_STATE_REJIT    2


class FJitCompiler : public IFJitCompiler
{
public:

    /* the jitting function */
    CorJitResult __stdcall compileMethod (
            ICorJitInfo*            comp,               /* IN */
            CORINFO_METHOD_INFO*    info,               /* IN */
            unsigned                flags,              /* IN */
            BYTE **                 nativeEntry,        /* OUT */
            ULONG  *                nativeSizeOfCode    /* OUT */
            );

    /*                                                                                              */
    FJit_Encode* __stdcall getEncoder();

    static BOOL Init();
    static void Terminate();

private:
    /* grab and remember the jitInterface helper addresses that we need at runtime */
    BOOL GetJitHelpers(ICorJitInfo* jitInfo);

};

/***********************************************************************************/
inline OpType& FJit::topOp(unsigned back) {
  _ASSERTE (opStack_len > back);
  if ( opStack_len <= back )
    RaiseException(SEH_JIT_REFUSED,EXCEPTION_NONCONTINUABLE,0,NULL); 
  return(opStack[opStack_len-back-1]);
}

inline void FJit::popOp(unsigned cnt) {

    _ASSERTE (opStack_len >= cnt);
    opStack_len -= cnt;
#ifdef _DEBUG
    opStack[opStack_len] = OpType(typeError);
#endif
}

inline void FJit::pushOp(OpType type) {
    _ASSERTE (opStack_len < opStack_size);
    _ASSERTE (type.isValClass() || (type.enum_() >= typeI4 || type.enum_() < typeU1));
    _ASSERTE (type.enum_() != 0 );
    opStack[opStack_len++] = type;
#ifdef _DEBUG
    opStack[opStack_len] = OpType(typeError);
#endif
}

inline void FJit::resetOpStack() {
    opStack_len = 0;
#ifdef _DEBUG
    opStack[opStack_len] = OpType(typeError);
#endif
}

inline bool FJit::isOpStackEmpty() {
    return (opStack_len == 0);
}

/* gets the size in void* sized units for the 'valClass'.  Works for RefAny's too. */
inline unsigned typeSizeInBytes(ICorJitInfo* jitInfo, CORINFO_CLASS_HANDLE valClass) {
    if (valClass == RefAnyClassHandle)
        return(2*sizeof(void*));
    return(jitInfo->getClassSize(valClass));

}

inline unsigned typeSizeInSlots(ICorJitInfo* jitInfo, CORINFO_CLASS_HANDLE valClass) {

    unsigned ret = typeSizeInBytes(jitInfo, valClass);
    ret = (ret+sizeof(void*)-1)/sizeof(void *);         // round up to full words 
    return(ret);
}

// Calculate the offset of a small valuetype within a word for small valuetypes on big endian machines
inline unsigned bigEndianOffset( unsigned int size ) { 
#if defined(BIGENDIAN)
      return size < 3 ? sizeof(void *) - size : 0;
#else
      return 0;
#endif
}
#endif //_FJIT_H_




