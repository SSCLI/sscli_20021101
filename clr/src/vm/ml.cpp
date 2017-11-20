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
// ML.CPP -
//
// Marshaling engine.

#include "common.h"

#include "vars.hpp"
#include "ml.h"
#include "excep.h"
#include "frames.h"
#include "interoputil.h"
#include "comstring.h"
#include "comstringbuffer.h"
#include "comvariant.h"

#include "comclass.h"
#include "commember.h"
#include "comdelegate.h"
#include "comvarargs.h"
#include "invokeutil.h"
#include "nstruct.h"
#include "comndirect.h"
#include "comvariant.h"


#include "marshaler.h"
#include "perfcounters.h"




//----------------------------------------------------------------------
// An image of the record placed on LOCAL array by the ML_OBJECT_C2N
// instruction.
//----------------------------------------------------------------------
struct ML_OBJECT_C2N_SR
{
    public:
        LPVOID DoConversion(OBJECTREF       *ppProtectedObjectRef,  
                            BYTE             inout,
                            BYTE             fIsAnsi,
                            CleanupWorkList *pCleanup         
                           );


        VOID BackPropagate(BOOL *pfDeferredException);

    private:
        enum BackPropType {
            BP_NONE,
            BP_UNMARSHAL,
        };


        // DoConversion() delegates to some other conversion code
        // depending on the runtime type of the parameter.
        // Then it saves it away here so that BackPropagate can delegate
        // to the appropriate propagater code without redoing the type
        // analysis.
        UINT8      m_backproptype;
        BYTE       m_inout;
        BYTE       m_fIsAnsi;

        union {
            BYTE                      m_marshaler;
            BYTE                      m_cstrmarshaler[sizeof(CSTRMarshaler)];
            BYTE                      m_wstrmarshaler[sizeof(WSTRMarshaler)];
            BYTE                      m_cstrbuffermarshaler[sizeof(CSTRBufferMarshaler)];
            BYTE                      m_wstrbuffermarshaler[sizeof(WSTRBufferMarshaler)];
            BYTE                      m_nativearraymarshaler[sizeof(NativeArrayMarshaler)];
            BYTE                      m_layoutclassptrmarshaler[sizeof(LayoutClassPtrMarshaler)];
        };
};



UINT SizeOfML_OBJECT_C2N_SR()
{
    return sizeof(ML_OBJECT_C2N_SR);
}




//----------------------------------------------------------------------
// Generate a database of MLCode information.
//----------------------------------------------------------------------
const MLInfo gMLInfo[] = {

#undef DEFINE_ML
#ifdef _DEBUG
#define DEFINE_ML(name,operandbytes,frequiredCleanup,cblocals, hndl) {operandbytes,frequiredCleanup, ((cblocals)+3) & ~3, hndl, #name},
#else
#define DEFINE_ML(name,operandbytes,frequiredCleanup,cblocals, hndl) {operandbytes,frequiredCleanup, ((cblocals)+3) & ~3, hndl},
#endif

#include "mlopdef.h"

};



//----------------------------------------------------------------------
// struct to compute the summary of a series of ML codes
//----------------------------------------------------------------------

VOID MLSummary::ComputeMLSummary(const MLCode *pMLCode)
{
    const MLCode *pml = pMLCode;
    MLCode code;
    while (ML_END != (code = *(pml++))) 
    {
        m_fRequiresCleanup = m_fRequiresCleanup || gMLInfo[code].m_frequiresCleanup; 
        m_cbTotalHandles += (gMLInfo[code].m_frequiresHandle ? 1 : 0);
        pml += gMLInfo[code].m_numOperandBytes; 
        m_cbTotalLocals+= gMLInfo[code].m_cbLocal;
    }
    m_cbMLSize = (unsigned)(pml - pMLCode);
}


//----------------------------------------------------------------------
// Computes the length of an MLCode stream in bytes, including
// the terminating ML_END opcode.
//----------------------------------------------------------------------
UINT MLStreamLength(const MLCode * const pMLCode)
{
    MLSummary summary;
    summary.ComputeMLSummary(pMLCode);
    return summary.m_cbMLSize;
}


//----------------------------------------------------------------------
// checks if MLCode stream requires cleanup
//----------------------------------------------------------------------

BOOL MLStreamRequiresCleanup(const MLCode  *pMLCode)
{
    MLSummary summary;
    summary.ComputeMLSummary(pMLCode);
    return summary.m_fRequiresCleanup;
}


//----------------------------------------------------------------------
// Executes MLCode up to the next ML_END or ML_INTERRUPT opcode.
//
// Inputs:
//    psrc             - sets initial value of SRC register
//    pdst             - sets initial value of DST register
//    plocals          - pointer to ML local var array
//    dstinc           - -1 or +1 depending on direction of DST movement.
//                       (the SRC always moves in the +1 direction)
//    pCleanupWorkList - (optional) pointer to initialized
//                       CleanupWorkList. this pointer may be NULL if none
//                       of the opcodes in the MLCode stream uses it.
//----------------------------------------------------------------------

VOID DisassembleMLStream(const MLCode *pMLCode);
const MLCode *
RunML(const MLCode    *       pMLCode,
      const VOID      *       psrc,
            VOID      *       pdst,
            UINT8     * const plocals,
      CleanupWorkList * const pCleanupWorkList
#ifdef _PPC_
             , DOUBLE   *       pFloatRegs /*=NULL*/
#endif
      )
{
    THROWSCOMPLUSEXCEPTION();

#ifdef _DEBUG
    LOG((LF_INTEROP, LL_INFO10000, "------------RunML------------\n"));
    VOID DisassembleMLStream(const MLCode *pMLCode);
    DisassembleMLStream(pMLCode);
#endif //_DEBUG

// Perf Counter "%Time in marshalling" support
// Implementation note: Pentium counter implementation 
// is too expensive for marshalling. So, we implement
// it as a counter.
    COUNTER_ONLY(GetPrivatePerfCounters().m_Interop.cMarshalling++);
    COUNTER_ONLY(GetGlobalPerfCounters().m_Interop.cMarshalling++);


    UINT8 *plocalwalk = plocals;
    BOOL   fDeferredException = FALSE;
    HRESULT deferredExceptionHR = E_OUTOFMEMORY;
    UINT   specialErrorCode = 0;
    enum {
        kSpecialError_InvalidRedim = 1,
    };

    RetValBuffer *ptempRetValBuffer;
    STRINGREF  tempStr;
    LPVOID pv;
    BYTE        inout;
    UINT16      elemsize;
    INT64       tempFPBuffer;
    UINT32      tempU32;
    Marshaler   *pMarshaler = NULL;
    MethodTable *pMT;
    OBJECTREF   tempOR;
    bool        bVariant = false;
    // We handle a Variant Marshaler differently:  We do not want to have a copy of the VariantData
    // included in the marshaling code.  Instead we put a referrence to the VariantData there.
    // If we meet a Variant Marshaler, we set resetbVariant to 2. The next ML code will use this Variant
    // marshaler.  After it is used (at which time resetbVariant becomes 0), we reset bVariant to false.
    UINT8       resetbVariant = 0;
    MethodDesc* tempMD;
#ifdef _PPC_
    UINT        nFloatRegs = 0;
#endif

#if defined(STACK_GROWS_DOWN_ON_ARGS_WALK)
#define STDST(type,val)         (*((type*)( ((BYTE*&)pdst) -= sizeof(type) )) = (val))
#else
#define STDST(type,val)         (*(((type*&)((BYTE*&)pdst))++) = (val))
#endif

#define STPTRDST(type, val)     STDST(type, val)
#define LDSTR4()                STDST(UINT32, (UINT32)LDSRC(UnsignedParmSourceType))
#define LDSTR8()                LDSTR4(); LDSTR4()


#define PTRSRC(type)    ((type)psrc)
#define LDSRC(type)     (INCSRC(type), *( (type*) (((BYTE*)(psrc))-StackElemSize(sizeof(type))) ))

#define INCSRC(type)     ((BYTE*&)psrc)+=StackElemSize(sizeof(type))

#define INCLOCAL(size)   plocalwalk += ((size)+3) & ~3; 

#define STLOCAL(type, val)    *((type*&)plocalwalk) = (val)
#define LDLOCAL(type)    *((type*&)plocalwalk)
#define PTRLOCAL(type)   (type)plocalwalk

#define LDCODE(object, type)    { memcpy(object, pMLCode, sizeof(type)); pMLCode = (BYTE *)pMLCode + sizeof(type); }
#define LDCODE8()    *((BYTE*&)pMLCode)++
#define LDCODE16()    GET_UNALIGNED_16(((UINT16*&)pMLCode)++)
#define LDCODE32()    GET_UNALIGNED_32(((UINT32*&)pMLCode)++)

#define LDCODE_POINTER()    ((VOID *)(size_t)GET_UNALIGNED_32(((UINT32*&)pMLCode)++))

#ifdef BIGENDIAN
#define ENDIANFIX(size)     (size < 3 ? sizeof(void *) - size : 0)
#else
#define ENDIANFIX(size)     0
#endif

    for(;;) {
#ifdef _DEBUG
        const MLCode *pMLCodeSave = pMLCode;
        UINT8 *poldlocalwalk = plocalwalk;
#endif

        switch (*(pMLCode++))
        {
            case ML_COPYI1: // sign extend one byte 
                STDST( SignedI1TargetType, (INT8)LDSRC(SignedParmSourceType) );
                break;
            
            case ML_COPYU1: // zero extend one byte 
                STDST( UnsignedI1TargetType, (UINT8)LDSRC(UnsignedParmSourceType));
                break;
            
            case ML_COPYI2: // sign extend two byte val 
                STDST( SignedI2TargetType, (INT16)LDSRC(SignedParmSourceType) );
                break;

            case ML_COPYU2: // zero extend 2 bytes 
                STDST( UnsignedI2TargetType, (UINT16)LDSRC(UnsignedParmSourceType) );
                break;

            case ML_COPYI4: // sign extend 4 byte val
                STDST( SignedI4TargetType, (INT32)LDSRC(SignedParmSourceType) );
                break;

            case ML_COPYU4: // zero extend 4 byte val
                STDST( UnsignedI4TargetType, (UINT32)LDSRC(UnsignedParmSourceType) );
                break;

            case ML_COPY4:
                //
                // By convention, we treat COPY4 the same as COPYU4 on IA64, so 
                // if we have a problem with missing a sign-extension, then the
                // code that generated the ML is at fault.
                //
                STDST( UnsignedI4TargetType, (UINT32)LDSRC(UnsignedParmSourceType) );
                break;
    
            case ML_COPY8:
                STDST( UINT64, LDSRC(UINT64) );
                break;

#ifdef _PPC_
            case ML_COPYR4_N2C: // copy 4 byte float val
                {
                FLOAT value;
                value = LDSRC(FLOAT);
                if (nFloatRegs < NUM_FLOAT_ARGUMENT_REGISTERS) {
                    value = *pFloatRegs++;
                    nFloatRegs++;
                }
                STDST(FLOAT, value);
                }
                break;

            case ML_COPYR4_C2N:
                {
                FLOAT value;
                value = LDSRC(FLOAT);
                if (nFloatRegs < NUM_FLOAT_ARGUMENT_REGISTERS) {
                    *pFloatRegs++ = value;
                    nFloatRegs++;
                }
                STDST(FLOAT, value);
                }
                break;

            case ML_COPYR8_N2C: // copy 8 byte float val
                {
                DOUBLE value;
                value = LDSRC(DOUBLE);
                if (nFloatRegs < NUM_FLOAT_ARGUMENT_REGISTERS) {
                    value = *pFloatRegs++;
                    nFloatRegs++;
                }
                STDST(DOUBLE, value);
                }
                break;

            case ML_COPYR8_C2N:
                {
                DOUBLE value;
                value = LDSRC(DOUBLE);
                if (nFloatRegs < NUM_FLOAT_ARGUMENT_REGISTERS) {
                    *pFloatRegs++ = value;
                    nFloatRegs++;
                }
                STDST(DOUBLE, value);
                }
                break;
#endif

            case ML_END:  // intentional fallthru
            case ML_INTERRUPT:
                if (fDeferredException) {
					switch (specialErrorCode)
					{
					    case kSpecialError_InvalidRedim:
							COMPlusThrow(kInvalidOperationException, IDS_INVALID_REDIM);

					    default:
							// An ML opcode encountered an error but chose to defer
							// the exception. Normally, this occurs during the backpropagation
							// phase.
							//
							// While it'd be preferable to throw the original exception
							// rather than an uninformative OutOfMemory exception,
							// it would require more overhead than I like to propagate
							// the exception backward in a GC-safe manner.
							COMPlusThrowHR(deferredExceptionHR);
					}
                }
                return pMLCode;

    

            //-------------------------------------------------------------------------
                

            case ML_BOOL_N2C:
                STDST(SignedI4TargetType, LDSRC(BOOL) ? 1 : 0);
                break;

#ifdef BIGENDIAN
            // Note: For big-endian architectures, we have to
            // push a pointer to the middle of the RetValBuffer.
            // That's why we have separate ML opcodes for each
            // possible buffer size. 
            case ML_PUSHRETVALBUFFER1:
                ptempRetValBuffer = PTRLOCAL(RetValBuffer*);
                ptempRetValBuffer->m_i32 = 0;
                STPTRDST(PVOID, (BYTE*)ptempRetValBuffer+3);
                INCLOCAL(sizeof(RetValBuffer));
                break;
            case ML_PUSHRETVALBUFFER2:
                ptempRetValBuffer = PTRLOCAL(RetValBuffer*);
                ptempRetValBuffer->m_i32 = 0;
                STPTRDST(PVOID, (BYTE*)ptempRetValBuffer+2);
                INCLOCAL(sizeof(RetValBuffer));
                break;
            case ML_PUSHRETVALBUFFER4:
                ptempRetValBuffer = PTRLOCAL(RetValBuffer*);
                ptempRetValBuffer->m_i32 = 0;
                STPTRDST(PVOID, ptempRetValBuffer);
                INCLOCAL(sizeof(RetValBuffer));
                break;
#else
            case ML_PUSHRETVALBUFFER1: //fallthru
            case ML_PUSHRETVALBUFFER2: //fallthru
            case ML_PUSHRETVALBUFFER4:
                ptempRetValBuffer = PTRLOCAL(RetValBuffer*);
                ptempRetValBuffer->m_i32 = 0;
                STPTRDST(PVOID, ptempRetValBuffer);
                INCLOCAL(sizeof(RetValBuffer));
                break;
#endif

            case ML_PUSHRETVALBUFFER8:
                ptempRetValBuffer = PTRLOCAL(RetValBuffer*);
                ptempRetValBuffer->m_i64 = 0;
                STPTRDST( RetValBuffer*, ptempRetValBuffer);
                INCLOCAL(sizeof(RetValBuffer));
                break;

            case ML_SETSRCTOLOCAL:
                psrc = (const VOID *)(plocals + LDCODE16());
                break;

            case ML_OBJECT_C2N:
                inout = LDCODE8();
                STPTRDST( LPVOID,
                       ((ML_OBJECT_C2N_SR*)plocalwalk)->DoConversion((OBJECTREF*)psrc, inout, (StringType)LDCODE8(), pCleanupWorkList) );
                INCSRC(OBJECTREF* &);
                INCLOCAL(sizeof(ML_OBJECT_C2N_SR));
                break;

            case ML_OBJECT_C2N_POST:
                ((ML_OBJECT_C2N_SR*)(plocals + LDCODE16()))->BackPropagate(&fDeferredException);
                break;

            case ML_LATEBOUNDMARKER:
                _ASSERTE(!"This ML stub should never be interpreted! This method should always be called using the ComPlusToComLateBoundWorker!");
                break;

            case ML_COMEVENTCALLMARKER:
                _ASSERTE(!"This ML stub should never be interpreted! This method should always be called using the ComEventCallWorker!");
                break;

            case ML_BUMPSRC:
                ((BYTE*&)psrc) += (INT16)LDCODE16();
                break;

            case ML_BUMPDST:
                ((BYTE*&)pdst) += (INT16)LDCODE16();
                break;



            case ML_R4_FROM_TOS:
                getFPReturn(4, &tempFPBuffer);
                STDST(INT32, (INT32) tempFPBuffer);
                break;
        
            case ML_R8_FROM_TOS:
                getFPReturn(8, &tempFPBuffer);
                STDST(INT64, tempFPBuffer);
                break;



            case ML_ARRAYWITHOFFSET_C2N:
                STPTRDST( LPVOID,
                          ((ML_ARRAYWITHOFFSET_C2N_SR*)plocalwalk)->DoConversion(
                                    &( ((ArrayWithOffsetData*)psrc)->m_Array ),
                                    ((ArrayWithOffsetData*)psrc)->m_cbOffset,
                                    ((ArrayWithOffsetData*)psrc)->m_cbCount,
                                    pCleanupWorkList)
                        );
                INCSRC(ArrayWithOffsetData);
                INCLOCAL(sizeof(ML_ARRAYWITHOFFSET_C2N_SR));
                break;

            case ML_ARRAYWITHOFFSET_C2N_POST:
                ((ML_ARRAYWITHOFFSET_C2N_SR*)(plocals + LDCODE16()))->BackPropagate();
                break;


            // 
            // Marshaler opcodes
            //

            case ML_CREATE_MARSHALER_GENERIC_1:
                pMarshaler = new (plocalwalk) CopyMarshaler1(pCleanupWorkList);
                _ASSERTE(pMarshaler != NULL);
                plocalwalk += sizeof(CopyMarshaler1);
                break;

            case ML_CREATE_MARSHALER_GENERIC_U1:
                pMarshaler = new (plocalwalk) CopyMarshalerU1(pCleanupWorkList);
                _ASSERTE(pMarshaler != NULL);
                plocalwalk += sizeof(CopyMarshalerU1);
                break;

            case ML_CREATE_MARSHALER_GENERIC_2:
                pMarshaler = new (plocalwalk) CopyMarshaler2(pCleanupWorkList);
                _ASSERTE(pMarshaler != NULL);
                plocalwalk += sizeof(CopyMarshaler2);
                break;

            case ML_CREATE_MARSHALER_GENERIC_U2:
                pMarshaler = new (plocalwalk) CopyMarshalerU2(pCleanupWorkList);
                _ASSERTE(pMarshaler != NULL);
                plocalwalk += sizeof(CopyMarshalerU2);
                break;

            case ML_CREATE_MARSHALER_GENERIC_4:
                pMarshaler = new (plocalwalk) CopyMarshaler4(pCleanupWorkList);
                _ASSERTE(pMarshaler != NULL);
                plocalwalk += sizeof(CopyMarshaler4);
                break;

            case ML_CREATE_MARSHALER_GENERIC_8:
                pMarshaler = new (plocalwalk) CopyMarshaler8(pCleanupWorkList);
                _ASSERTE(pMarshaler != NULL);
                plocalwalk += sizeof(CopyMarshaler8);
                break;

            case ML_CREATE_MARSHALER_WINBOOL:
                pMarshaler = new (plocalwalk) WinBoolMarshaler(pCleanupWorkList);
                _ASSERTE(pMarshaler != NULL);
                plocalwalk += sizeof(WinBoolMarshaler);
                break;

            case ML_CREATE_MARSHALER_CBOOL:
                pMarshaler = new (plocalwalk) CBoolMarshaler(pCleanupWorkList);
                _ASSERTE(pMarshaler != NULL);
                plocalwalk += sizeof(CBoolMarshaler);
                break;

            case ML_CREATE_MARSHALER_ANSICHAR:
                pMarshaler = new (plocalwalk) AnsiCharMarshaler(pCleanupWorkList);
                _ASSERTE(pMarshaler != NULL);
                plocalwalk += sizeof(AnsiCharMarshaler);
                break;

            case ML_CREATE_MARSHALER_FLOAT:
                pMarshaler = new (plocalwalk) FloatMarshaler(pCleanupWorkList);
                _ASSERTE(pMarshaler != NULL);
                plocalwalk += sizeof(FloatMarshaler);
                break;

            case ML_CREATE_MARSHALER_DOUBLE:
                pMarshaler = new (plocalwalk) DoubleMarshaler(pCleanupWorkList);
                _ASSERTE(pMarshaler != NULL);
                plocalwalk += sizeof(DoubleMarshaler);
                break;

            case ML_CREATE_MARSHALER_CURRENCY:
                pMarshaler = new (plocalwalk) CurrencyMarshaler(pCleanupWorkList);
                _ASSERTE(pMarshaler != NULL);
                plocalwalk += sizeof(CurrencyMarshaler);
                break;

            case ML_CREATE_MARSHALER_DECIMAL:
                pMarshaler = new (plocalwalk) DecimalMarshaler(pCleanupWorkList);
                _ASSERTE(pMarshaler != NULL);
                plocalwalk += sizeof(DecimalMarshaler);
                break;

            case ML_CREATE_MARSHALER_DECIMAL_PTR:
                pMarshaler = new (plocalwalk) DecimalPtrMarshaler(pCleanupWorkList);
                _ASSERTE(pMarshaler != NULL);
                plocalwalk += sizeof(DecimalPtrMarshaler);
                break;

            case ML_CREATE_MARSHALER_GUID:
                pMarshaler = new (plocalwalk) GuidMarshaler(pCleanupWorkList);
                _ASSERTE(pMarshaler != NULL);
                plocalwalk += sizeof(GuidMarshaler);
                break;

            case ML_CREATE_MARSHALER_GUID_PTR:
                pMarshaler = new (plocalwalk) GuidPtrMarshaler(pCleanupWorkList);
                _ASSERTE(pMarshaler != NULL);
                plocalwalk += sizeof(GuidPtrMarshaler);
                break;

            case ML_CREATE_MARSHALER_DATE:
                pMarshaler = new (plocalwalk) DateMarshaler(pCleanupWorkList);
                _ASSERTE(pMarshaler != NULL);
                plocalwalk += sizeof(DateMarshaler);
                break;

            case ML_CREATE_MARSHALER_WSTR:
                pMarshaler = new (plocalwalk) WSTRMarshaler(pCleanupWorkList);
                _ASSERTE(pMarshaler != NULL);
                plocalwalk += sizeof(WSTRMarshaler);
                break;

            case ML_CREATE_MARSHALER_CSTR:
                pMarshaler = new (plocalwalk) CSTRMarshaler(pCleanupWorkList);
                _ASSERTE(pMarshaler != NULL);
                plocalwalk += sizeof(CSTRMarshaler);
                break;

            case ML_CREATE_MARSHALER_WSTR_BUFFER:
                pMarshaler = new (plocalwalk) WSTRBufferMarshaler(pCleanupWorkList);
                _ASSERTE(pMarshaler != NULL);
                plocalwalk += sizeof(WSTRBufferMarshaler);
                break;

            case ML_CREATE_MARSHALER_CSTR_BUFFER:
                pMarshaler = new (plocalwalk) CSTRBufferMarshaler(pCleanupWorkList);
                _ASSERTE(pMarshaler != NULL);
                plocalwalk += sizeof(CSTRBufferMarshaler);
                break;

            case ML_CREATE_MARSHALER_WSTR_X:
                pMarshaler = new (plocalwalk) WSTRMarshalerEx(pCleanupWorkList);
                _ASSERTE(pMarshaler != NULL);
                ((WSTRMarshalerEx*)pMarshaler)->SetMethodTable((MethodTable *)LDCODE_POINTER());
                plocalwalk += sizeof(WSTRMarshalerEx);
                break;

            case ML_CREATE_MARSHALER_CSTR_X:
                pMarshaler = new (plocalwalk) CSTRMarshalerEx(pCleanupWorkList);
                _ASSERTE(pMarshaler != NULL);
                ((CSTRMarshalerEx*)pMarshaler)->SetMethodTable((MethodTable *)LDCODE_POINTER());
                plocalwalk += sizeof(CSTRMarshalerEx);
                break;

            case ML_CREATE_MARSHALER_WSTR_BUFFER_X:
                pMarshaler = new (plocalwalk) WSTRBufferMarshalerEx(pCleanupWorkList);
                _ASSERTE(pMarshaler != NULL);
                ((WSTRBufferMarshalerEx*)pMarshaler)->SetMethodTable((MethodTable *)LDCODE_POINTER());
                plocalwalk += sizeof(WSTRBufferMarshalerEx);
                break;

            case ML_CREATE_MARSHALER_CSTR_BUFFER_X:
                pMarshaler = new (plocalwalk) CSTRBufferMarshalerEx(pCleanupWorkList);
                _ASSERTE(pMarshaler != NULL);
                ((CSTRBufferMarshalerEx*)pMarshaler)->SetMethodTable((MethodTable *)LDCODE_POINTER());
                plocalwalk += sizeof(CSTRBufferMarshalerEx);
                break;

            case ML_CREATE_MARSHALER_CARRAY:
                {
                    ML_CREATE_MARSHALER_CARRAY_OPERANDS mops;
                    LDCODE(&mops, ML_CREATE_MARSHALER_CARRAY_OPERANDS);
                    pMarshaler = new (plocalwalk) NativeArrayMarshaler(pCleanupWorkList);
                    _ASSERTE(pMarshaler != NULL);
                    ((NativeArrayMarshaler*)pMarshaler)->SetElementMethodTable(mops.methodTable);
                    ((NativeArrayMarshaler*)pMarshaler)->SetElementType(mops.elementType);

                    DWORD numelems = mops.additive;
                    if (mops.multiplier != 0 && mops.countSize != 0) // don't dereference ofsbump+psrc if countsize is zero!!!  
                    {
                        const BYTE *pCount = mops.offsetbump + (const BYTE *)psrc;
                        switch (mops.countSize)
                        {
                            case 1: numelems += mops.multiplier * (DWORD)*((UINT8*)pCount); break;
                            case 2: numelems += mops.multiplier * (DWORD)*((UINT16*)pCount); break;
                            case 4: numelems += mops.multiplier * (DWORD)*((UINT32*)pCount); break;
                            case 8: numelems += mops.multiplier * (DWORD)*((UINT64*)pCount); break;
                            default:
                                _ASSERTE(0); 
                        }
                    }
                    
                    ((NativeArrayMarshaler*)pMarshaler)->SetElementCount(numelems);
                    plocalwalk += sizeof(NativeArrayMarshaler);
                }
                break;

            case ML_CREATE_MARSHALER_DELEGATE:
                pMarshaler = new (plocalwalk) DelegateMarshaler(pCleanupWorkList);
                _ASSERTE(pMarshaler != NULL);
                plocalwalk += sizeof(DelegateMarshaler);
                break;

            case ML_CREATE_MARSHALER_BLITTABLEPTR:
                pMarshaler = new (plocalwalk) BlittablePtrMarshaler(pCleanupWorkList, (MethodTable *)LDCODE_POINTER());
                _ASSERTE(pMarshaler != NULL);
                plocalwalk += sizeof(BlittablePtrMarshaler);
                break;

            case ML_CREATE_MARSHALER_LAYOUTCLASSPTR:
                pMarshaler = new (plocalwalk) LayoutClassPtrMarshaler(pCleanupWorkList, (MethodTable *)LDCODE_POINTER());
                _ASSERTE(pMarshaler != NULL);
                plocalwalk += sizeof(LayoutClassPtrMarshaler);
                break;

            case ML_CREATE_MARSHALER_ARRAYWITHOFFSET:
                _ASSERTE(!"The ML_CREATE_MARSHALER_ARRAYWITHOFFSET marshaler should never be created!");
                break;

            case ML_CREATE_MARSHALER_BLITTABLEVALUECLASS:
                _ASSERTE(!"The ML_CREATE_MARSHALER_BLITTABLEVALUECLASS marshaler should never be created!");
                break;

            case ML_CREATE_MARSHALER_VALUECLASS:
                _ASSERTE(!"The ML_CREATE_MARSHALER_VALUECLASS marshaler should never be created!");
                break;

            case ML_CREATE_MARSHALER_REFERENCECUSTOMMARSHALER:
                pMarshaler = new (plocalwalk) ReferenceCustomMarshaler(pCleanupWorkList, (CustomMarshalerHelper *)LDCODE_POINTER());
                _ASSERTE(pMarshaler != NULL);
                plocalwalk += sizeof(ReferenceCustomMarshaler);
                break;

            case ML_CREATE_MARSHALER_VALUECLASSCUSTOMMARSHALER:
                pMarshaler = new (plocalwalk) ValueClassCustomMarshaler(pCleanupWorkList, (CustomMarshalerHelper *)LDCODE_POINTER());
                _ASSERTE(pMarshaler != NULL);
                plocalwalk += sizeof(ValueClassCustomMarshaler);
                break;

            case ML_CREATE_MARSHALER_ARGITERATOR:
                _ASSERTE(!"The ML_CREATE_MARSHALER_ARGITERATOR marshaler should never be created!");
                break;

            case ML_CREATE_MARSHALER_BLITTABLEVALUECLASSWITHCOPYCTOR:
                _ASSERTE(!"The ML_CREATE_MARSHALER_BLITTABLEVALUECLASSWITHCOPYCTOR marshaler should never be created!");
                break;

            case ML_MARSHAL_N2C:
#ifdef STACK_GROWS_DOWN_ON_ARGS_WALK
                (BYTE*&)pdst -= pMarshaler->m_cbCom;
                pMarshaler->MarshalNativeToCom((void *)psrc, pdst);
                (BYTE*&)psrc += pMarshaler->m_cbNative;
#else
                pMarshaler->MarshalNativeToCom((void *)psrc, pdst);
                (BYTE*&)pdst += pMarshaler->m_cbCom;
                (BYTE*&)psrc += pMarshaler->m_cbNative;
#endif
                break;

            case ML_MARSHAL_N2C_OUT:
#ifdef STACK_GROWS_DOWN_ON_ARGS_WALK
                (BYTE*&)pdst -= pMarshaler->m_cbCom;
                pMarshaler->MarshalNativeToComOut((void *)psrc, pdst);
                (BYTE*&)psrc += pMarshaler->m_cbNative;
#else
                pMarshaler->MarshalNativeToComOut((void *)psrc, pdst);
                (BYTE*&)pdst += pMarshaler->m_cbCom;
                (BYTE*&)psrc += pMarshaler->m_cbNative;
#endif
                break;

            case ML_MARSHAL_N2C_BYREF:
#ifdef STACK_GROWS_DOWN_ON_ARGS_WALK
                (BYTE*&)pdst -= sizeof(void*);
                pMarshaler->MarshalNativeToComByref((void *)psrc, pdst);
                (BYTE*&)psrc += sizeof(void*);
#else
                pMarshaler->MarshalNativeToComByref((void *)psrc, pdst);
                (BYTE*&)pdst += sizeof(void*);
                (BYTE*&)psrc += sizeof(void*);
#endif
                break;

            case ML_MARSHAL_N2C_BYREF_OUT:
#ifdef STACK_GROWS_DOWN_ON_ARGS_WALK
                (BYTE*&)pdst -= sizeof(void*);
                pMarshaler->MarshalNativeToComByrefOut((void *)psrc, pdst);
                (BYTE*&)psrc += sizeof(void*);
#else
                pMarshaler->MarshalNativeToComByrefOut((void *)psrc, pdst);
                (BYTE*&)pdst += sizeof(void*);
                (BYTE*&)psrc += sizeof(void*);
#endif 
                break;

            case ML_UNMARSHAL_N2C_IN:
                pMarshaler = (Marshaler*) (plocals + LDCODE16());
                pMarshaler->UnmarshalNativeToComIn();
                break;

            case ML_UNMARSHAL_N2C_OUT:
                pMarshaler = (Marshaler*) (plocals + LDCODE16());
                pMarshaler->UnmarshalNativeToComOut();
                break;

            case ML_UNMARSHAL_N2C_IN_OUT:
                pMarshaler = (Marshaler*) (plocals + LDCODE16());
                pMarshaler->UnmarshalNativeToComInOut();
                break;

            case ML_UNMARSHAL_N2C_BYREF_IN:
                pMarshaler = (Marshaler*) (plocals + LDCODE16());
                pMarshaler->UnmarshalNativeToComByrefIn();
                break;

            case ML_UNMARSHAL_N2C_BYREF_OUT:
                pMarshaler = (Marshaler*) (plocals + LDCODE16());
                pMarshaler->UnmarshalNativeToComByrefOut();
                break;

            case ML_UNMARSHAL_N2C_BYREF_IN_OUT:
                pMarshaler = (Marshaler*) (plocals + LDCODE16());
                pMarshaler->UnmarshalNativeToComByrefInOut();
                break;

            case ML_MARSHAL_C2N:
#ifdef STACK_GROWS_DOWN_ON_ARGS_WALK
                (BYTE*&)pdst -= pMarshaler->m_cbNative;
                pMarshaler->MarshalComToNative(bVariant?(void*)(&psrc):(void *)psrc, pdst);
                (BYTE*&)psrc += pMarshaler->m_cbCom;
#else
                pMarshaler->MarshalComToNative(bVariant?(void*)(&psrc):(void *)psrc, pdst);
                (BYTE*&)pdst += pMarshaler->m_cbNative;
                (BYTE*&)psrc += pMarshaler->m_cbCom;
#endif
                break;

            case ML_MARSHAL_C2N_OUT:
#ifdef STACK_GROWS_DOWN_ON_ARGS_WALK
                (BYTE*&)pdst -= pMarshaler->m_cbNative;
                pMarshaler->MarshalComToNativeOut(bVariant?(void*)(&psrc):(void *)psrc, pdst);
                (BYTE*&)psrc += pMarshaler->m_cbCom;
#else
                pMarshaler->MarshalComToNativeOut(bVariant?(void*)(&psrc):(void *)psrc, pdst);
                (BYTE*&)pdst += pMarshaler->m_cbNative;
                (BYTE*&)psrc += pMarshaler->m_cbCom;
#endif
                break;

            case ML_MARSHAL_C2N_BYREF:
#ifdef STACK_GROWS_DOWN_ON_ARGS_WALK
                (BYTE*&)pdst -= sizeof(void*);
                pMarshaler->MarshalComToNativeByref((void *)psrc, pdst);
                (BYTE*&)psrc += sizeof(void*);
#else
                pMarshaler->MarshalComToNativeByref((void *)psrc, pdst);
                (BYTE*&)pdst += sizeof(void*);
                (BYTE*&)psrc += sizeof(void*);
#endif
                break;

            case ML_MARSHAL_C2N_BYREF_OUT:
#ifdef STACK_GROWS_DOWN_ON_ARGS_WALK
                (BYTE*&)pdst -= sizeof(void*);
                pMarshaler->MarshalComToNativeByrefOut((void *)psrc, pdst);
                (BYTE*&)psrc += sizeof(void*);
#else
                pMarshaler->MarshalComToNativeByrefOut((void *)psrc, pdst);
                (BYTE*&)pdst += sizeof(void*);
                (BYTE*&)psrc += sizeof(void*);
#endif
                break;

            case ML_UNMARSHAL_C2N_IN:
                pMarshaler = (Marshaler*) (plocals + LDCODE16());
                pMarshaler->UnmarshalComToNativeIn();
                break;

            case ML_UNMARSHAL_C2N_OUT:
                pMarshaler = (Marshaler*) (plocals + LDCODE16());
                pMarshaler->UnmarshalComToNativeOut();
                break;

            case ML_UNMARSHAL_C2N_IN_OUT:
                pMarshaler = (Marshaler*) (plocals + LDCODE16());
                pMarshaler->UnmarshalComToNativeInOut();
                break;

            case ML_UNMARSHAL_C2N_BYREF_IN:
                pMarshaler = (Marshaler*) (plocals + LDCODE16());
                pMarshaler->UnmarshalComToNativeByrefIn();
                break;

            case ML_UNMARSHAL_C2N_BYREF_OUT:
                pMarshaler = (Marshaler*) (plocals + LDCODE16());
                pMarshaler->UnmarshalComToNativeByrefOut();
                break;

            case ML_UNMARSHAL_C2N_BYREF_IN_OUT:
                pMarshaler = (Marshaler*) (plocals + LDCODE16());
                pMarshaler->UnmarshalComToNativeByrefInOut();
                break;

            case ML_PRERETURN_N2C:
#ifdef STACK_GROWS_DOWN_ON_ARGS_WALK
                if (pMarshaler->m_fReturnsComByref)
                    (BYTE*&)pdst -= sizeof(void*);
#endif

                pMarshaler->PrereturnNativeFromCom((void *)psrc, pdst);

#ifdef STACK_GROWS_UP_ON_ARGS_WALK
                if (pMarshaler->m_fReturnsComByref)
                    (BYTE*&)pdst += sizeof(void*);
#endif
                break;

            case ML_RETURN_N2C:
                pMarshaler = (Marshaler*) (plocals + LDCODE16());

#ifdef STACK_GROWS_DOWN_ON_ARGS_WALK
                if (pMarshaler->m_fReturnsNativeByref)
                    (BYTE*&)pdst -= sizeof(void*);
                else
                    (BYTE*&)pdst -= pMarshaler->m_cbNative;
#endif

                pMarshaler->ReturnNativeFromCom(pdst, (void *) psrc);

                if (!pMarshaler->m_fReturnsComByref)
                    (BYTE*&)psrc += pMarshaler->m_cbCom;
#ifdef STACK_GROWS_UP_ON_ARGS_WALK
                if (pMarshaler->m_fReturnsNativeByref)
                    (BYTE*&)pdst += sizeof(void*);
                else
                    (BYTE*&)pdst += pMarshaler->m_cbNative;
#endif
                break;

            case ML_PRERETURN_N2C_RETVAL:
#ifdef STACK_GROWS_DOWN_ON_ARGS_WALK
                if (pMarshaler->m_fReturnsComByref)
                    (BYTE*&)pdst -= sizeof(void*);
#endif
                pMarshaler->PrereturnNativeFromComRetval((void *)psrc, pdst);

                (BYTE*&)psrc += sizeof(void*);
#ifdef STACK_GROWS_UP_ON_ARGS_WALK
                if (pMarshaler->m_fReturnsComByref)
                    (BYTE*&)pdst += sizeof(void*);
#endif
                break;

            case ML_RETURN_N2C_RETVAL:
                pMarshaler = (Marshaler*) (plocals + LDCODE16());

                pMarshaler->ReturnNativeFromComRetval(pdst, (void *) psrc);

                if (!pMarshaler->m_fReturnsComByref)
                    (BYTE*&)psrc += pMarshaler->m_cbCom;
                break;

            case ML_PRERETURN_C2N:
                pMarshaler->PrereturnComFromNative((void *)psrc, pdst);
                if (pMarshaler->m_fReturnsComByref)
                    (BYTE*&)psrc += sizeof(void*);
                break;

            case ML_RETURN_C2N:
                pMarshaler = (Marshaler*) (plocals + LDCODE16());

#ifdef STACK_GROWS_DOWN_ON_ARGS_WALK
                if (!pMarshaler->m_fReturnsComByref)
                    (BYTE*&)pdst -= pMarshaler->m_cbCom;
#endif
                pMarshaler->ReturnComFromNative(pdst, (void *) psrc);

                if (pMarshaler->m_fReturnsNativeByref)
                    (BYTE*&)psrc += sizeof(void*);
                else
                    (BYTE*&)psrc += pMarshaler->m_cbNative;
#ifdef STACK_GROWS_UP_ON_ARGS_WALK
                if (!pMarshaler->m_fReturnsComByref)
                    (BYTE*&)pdst += pMarshaler->m_cbCom;
#endif
                break;

            case ML_PRERETURN_C2N_RETVAL:
                (BYTE*&)pdst -= sizeof(void*);
                pMarshaler->PrereturnComFromNativeRetval((void *)psrc, pdst);
                if (pMarshaler->m_fReturnsComByref)
                    (BYTE*&)psrc += sizeof(void*);
                break;

            case ML_RETURN_C2N_RETVAL:
                pMarshaler = (Marshaler*) (plocals + LDCODE16());

#ifdef STACK_GROWS_DOWN_ON_ARGS_WALK
                if (!pMarshaler->m_fReturnsComByref)
                    (BYTE*&)pdst -= pMarshaler->m_cbCom;
#endif
                pMarshaler->ReturnComFromNativeRetval(pdst, (void *) psrc);
#ifdef STACK_GROWS_UP_ON_ARGS_WALK
                if (!pMarshaler->m_fReturnsComByref)
                    (BYTE*&)pdst += pMarshaler->m_cbCom;
#endif

                break;
    
            case ML_SET_COM:
                pMarshaler->SetCom((void *) psrc, pdst);
                break;
    
            case ML_GET_COM:
                pMarshaler->GetCom(pdst, (void *) psrc);
                break;
    
            case ML_PREGET_COM_RETVAL:
                pMarshaler->PregetComRetval((void *) psrc, pdst);
                break;
    


            case ML_PINNEDUNISTR_C2N:
                tempStr = LDSRC(STRINGREF);
                STPTRDST( const WCHAR *, (tempStr == NULL ? NULL : tempStr->GetBuffer()) );
#ifdef TOUCH_ALL_PINNED_OBJECTS
                if (tempStr != NULL) {
                    TouchPages(tempStr->GetBuffer(), sizeof(WCHAR)*(1 + tempStr->GetStringLength()));
                }
#endif
                break;


            case ML_BLITTABLELAYOUTCLASS_C2N:
                tempOR = LDSRC(OBJECTREF);
                _ASSERTE(tempOR == NULL ||
                         (tempOR->GetClass()->HasLayout() && tempOR->GetClass()->IsBlittable()));

#ifdef TOUCH_ALL_PINNED_OBJECTS
                if (tempOR != NULL) {
                    TouchPages(tempOR->GetData(), tempOR->GetMethodTable()->GetNativeSize());
                }
#endif
                STPTRDST( LPVOID, tempOR == NULL ? NULL : tempOR->GetData() );
                       
                break;


            case ML_BLITTABLEVALUECLASS_C2N:
                tempU32 = LDCODE32();
#ifdef STACK_GROWS_DOWN_ON_ARGS_WALK
                ((BYTE*&)pdst) -= MLParmSize(tempU32);
#endif
                memcpyNoGCRefs((BYTE*)pdst + ENDIANFIX(tempU32),
                               (BYTE*)psrc + ENDIANFIX(tempU32),
                               tempU32);
#ifdef STACK_GROWS_UP_ON_ARGS_WALK
                ((BYTE*&)pdst) += MLParmSize(tempU32);
#endif
                ((BYTE*&)psrc) += StackElemSize(tempU32);
                break;

            case ML_BLITTABLEVALUECLASS_N2C:
                tempU32 = LDCODE32();
#ifdef STACK_GROWS_DOWN_ON_ARGS_WALK
                ((BYTE*&)pdst) -= StackElemSize(tempU32);
#endif
                memcpyNoGCRefs((BYTE*)pdst + ENDIANFIX(tempU32),
                               (BYTE*)psrc + ENDIANFIX(tempU32),
                               tempU32);
#ifdef STACK_GROWS_UP_ON_ARGS_WALK
                ((BYTE*&)pdst) += StackElemSize(tempU32);
#endif
                ((BYTE*&)psrc) += MLParmSize(tempU32);
                break;

            case ML_REFBLITTABLEVALUECLASS_C2N:
                tempU32 = LDCODE32();
                pv = LDSRC(LPVOID);
#ifdef STACK_GROWS_DOWN_ON_ARGS_WALK
                ((BYTE*&)pdst) -= sizeof(LPVOID);
#endif
                *((LPVOID*)pdst) = pv;
#ifdef STACK_GROWS_UP_ON_ARGS_WALK
                ((BYTE*&)pdst) += sizeof(LPVOID);
#endif
#ifdef TOUCH_ALL_PINNED_OBJECTS
                TouchPages(pv, tempU32);
#endif
                break;


            case ML_VALUECLASS_C2N:
                pMT = (MethodTable *)LDCODE_POINTER();
                
                _ASSERTE(pMT->GetClass()->IsValueClass());
                pv = pCleanupWorkList->NewScheduleLayoutDestroyNative(pMT);
                FmtValueTypeUpdateNative( (LPVOID)&psrc, pMT, (BYTE*)pv );

                ((BYTE*&)psrc) += StackElemSize(pMT->GetClass()->GetAlignedNumInstanceFieldBytes());
#ifdef STACK_GROWS_DOWN_ON_ARGS_WALK
                ((BYTE*&)pdst) -= MLParmSize(pMT->GetNativeSize());
#endif

                memcpyNoGCRefs(pdst, pv, pMT->GetNativeSize());
#ifdef STACK_GROWS_UP_ON_ARGS_WALK
                ((BYTE*&)pdst) += MLParmSize(pMT->GetNativeSize());
#endif

                break;


            case ML_VALUECLASS_N2C:
                pMT = (MethodTable *)LDCODE_POINTER();
                _ASSERTE(pMT->GetClass()->IsValueClass());
#ifdef STACK_GROWS_DOWN_ON_ARGS_WALK
                ((BYTE*&)pdst) -= MLParmSize(pMT->GetClass()->GetAlignedNumInstanceFieldBytes());
#endif
                FmtValueTypeUpdateComPlus( &pdst, pMT, (BYTE*)psrc , FALSE);
#ifdef STACK_GROWS_UP_ON_ARGS_WALK
                ((BYTE*&)pdst) += MLParmSize(pMT->GetClass()->GetAlignedNumInstanceFieldBytes());
#endif
                ((BYTE*&)psrc) += StackElemSize(pMT->GetNativeSize());
                break;



            case ML_REFVALUECLASS_C2N:
                inout = LDCODE8();
                pMT = (MethodTable *)LDCODE_POINTER();
                STPTRDST( LPVOID, ((ML_REFVALUECLASS_C2N_SR*)plocalwalk)->DoConversion((VOID**)psrc, pMT, inout, pCleanupWorkList));
                INCLOCAL(sizeof(ML_REFVALUECLASS_C2N_SR));

                ((BYTE*&)psrc) += StackElemSize( sizeof(LPVOID) );
                break;

            case ML_REFVALUECLASS_C2N_POST:
                ((ML_REFVALUECLASS_C2N_SR*)(plocals + LDCODE16()))->BackPropagate(&fDeferredException);
                break;



            case ML_REFVALUECLASS_N2C:
                inout = LDCODE8();
                pMT = (MethodTable *)LDCODE_POINTER();
                STPTRDST( LPVOID, ((ML_REFVALUECLASS_N2C_SR*)plocalwalk)->DoConversion(*(LPVOID*)psrc, inout, pMT, pCleanupWorkList));
                INCLOCAL(sizeof(ML_REFVALUECLASS_N2C_SR));

                ((BYTE*&)psrc) += StackElemSize( sizeof(LPVOID) );
                break;

            case ML_REFVALUECLASS_N2C_POST:
                ((ML_REFVALUECLASS_N2C_SR*)(plocals + LDCODE16()))->BackPropagate(&fDeferredException);
                break;



            case ML_PINNEDISOMORPHICARRAY_C2N:
                elemsize = LDCODE16();
                tempOR = LDSRC(OBJECTREF);
                STPTRDST( LPVOID, tempOR == NULL ? NULL : (*(BASEARRAYREF*)&tempOR)->GetDataPtr() );

#ifdef TOUCH_ALL_PINNED_OBJECTS
                if (tempOR != NULL) {
                    TouchPages((*(BASEARRAYREF*)&tempOR)->GetDataPtr(), elemsize * (*(BASEARRAYREF*)&tempOR)->GetNumComponents());
                }
#endif
                break;

            case ML_PINNEDISOMORPHICARRAY_C2N_EXPRESS:
                
#ifdef TOUCH_ALL_PINNED_OBJECTS
                _ASSERTE(!"Shouldn't have gotten here.");
#endif
                elemsize = LDCODE16();
                _ASSERTE(elemsize != 0);
                tempOR = LDSRC(OBJECTREF);
                STPTRDST( BYTE*, tempOR == NULL ? NULL : (*(BYTE**)&tempOR) + elemsize );

                break;

            case ML_ARGITERATOR_C2N:
                STDST(va_list, COMVarArgs::MarshalToUnmanagedVaList((VARARGS*)psrc));
                INCSRC(VARARGS);
                break;

            case ML_ARGITERATOR_N2C:
#ifdef STACK_GROWS_DOWN_ON_ARGS_WALK
                ((BYTE*&)pdst) -= StackElemSize( sizeof(VARARGS) );
#endif
                COMVarArgs::MarshalToManagedVaList(LDSRC(va_list), (VARARGS*)pdst);
#ifdef STACK_GROWS_UP_ON_ARGS_WALK
                ((BYTE*&)pdst) += StackElemSize( sizeof(VARARGS) );
#endif
                break;

            case ML_COPYCTOR_C2N:
                pMT     = (MethodTable *)LDCODE_POINTER();
                tempMD  = (MethodDesc *)LDCODE_POINTER();
                tempU32 = MLParmSize(pMT->GetNativeSize());
#ifdef STACK_GROWS_DOWN_ON_ARGS_WALK
                ((BYTE*&)pdst) -= tempU32;
#endif
                pv = *((LPVOID*)psrc);
                if (tempMD) {
                    ARG_SLOT args[2] = { PtrToArgSlot(pv), PtrToArgSlot(pdst) };
                    tempMD->Call(args);
                } else {
					memcpyNoGCRefs(pdst, pv, tempU32);
				}

#ifdef STACK_GROWS_UP_ON_ARGS_WALK
                ((BYTE*&)pdst) += tempU32;
#endif
                pv = LDSRC(LPVOID); //reload again from gc-protected memory - just to be paranoid
                tempMD = (MethodDesc *)LDCODE_POINTER();
                if (tempMD) {
                    ARG_SLOT arg = PtrToArgSlot(pv);
                    tempMD->Call(&arg);
                }
                
                break;


            case ML_COPYCTOR_N2C:
                pMT = (MethodTable *)LDCODE_POINTER();
                tempU32 = MLParmSize(pMT->GetNativeSize());
                STPTRDST(const VOID *, psrc);
                ((BYTE*&)psrc) += tempU32;
                break;

            case ML_CAPTURE_PSRC:
                tempU32 = (UINT32)(INT32)LDCODE16();
                *((BYTE**)plocalwalk) = ((BYTE*)psrc) + (INT32)tempU32;
                INCLOCAL(sizeof(BYTE**));
                break;

            case ML_MARSHAL_RETVAL_SMBLITTABLEVALUETYPE_C2N:
                // Note: we only handle blittables here.
                tempU32 = LDCODE32(); // get the size
                memcpyNoGCRefs( **((BYTE***)(plocals + LDCODE16())) + ENDIANFIX(tempU32),
                                (BYTE *)psrc + ENDIANFIX(tempU32), tempU32 );
                break;

            case ML_MARSHAL_RETVAL_SMBLITTABLEVALUETYPE_N2C:
                // Note: we only handle blittables here.
                tempU32 = LDCODE32(); // get the size
#ifdef STACK_GROWS_DOWN_ON_ARGS_WALK
                ((BYTE*&)pdst) -= MLParmSize(tempU32);
#endif
                memcpyNoGCRefs( (BYTE*)pdst + ENDIANFIX(tempU32), plocals + LDCODE16() + ENDIANFIX(tempU32), tempU32 );
#ifdef STACK_GROWS_UP_ON_ARGS_WALK
                ((BYTE*&)pdst) += MLParmSize(tempU32);
#endif
                break;

            case ML_MARSHAL_RETVAL_LGBLITTABLEVALUETYPE_C2N:
                tempU32 = LDCODE32(); // get the size;
                STPTRDST( LPVOID, ((ML_MARSHAL_RETVAL_LGBLITTABLEVALUETYPE_C2N_SR*)plocalwalk)->DoConversion(psrc, tempU32));
                INCLOCAL(sizeof(ML_MARSHAL_RETVAL_LGBLITTABLEVALUETYPE_C2N_SR));

                ((BYTE*&)psrc) += StackElemSize( sizeof(LPVOID) );
                break;

            case ML_MARSHAL_RETVAL_LGBLITTABLEVALUETYPE_C2N_POST:
                ((ML_MARSHAL_RETVAL_LGBLITTABLEVALUETYPE_C2N_SR*)(plocals + LDCODE16()))->BackPropagate(&fDeferredException);
                break;

            case ML_MARSHAL_RETVAL_LGBLITTABLEVALUETYPE_N2C:
                pv = LDSRC(LPVOID);
                STPTRDST(LPVOID, pv);
                *((LPVOID*)plocalwalk) = pv;
                INCLOCAL(sizeof(LPVOID));
                break;

            case ML_MARSHAL_RETVAL_LGBLITTABLEVALUETYPE_N2C_POST:
                pv = *((LPVOID*)(plocals + LDCODE16() ));
                STPTRDST(LPVOID, pv);
                break;

            case ML_PUSHVASIGCOOKIEEX:
#ifdef _DEBUG
                FillMemory(plocalwalk, sizeof(VASigCookieEx), 0xcc);
#endif
                ((VASigCookieEx*)plocalwalk)->sizeOfArgs = LDCODE16();
                ((VASigCookieEx*)plocalwalk)->mdVASig = NULL;
                ((VASigCookieEx*)plocalwalk)->m_pArgs = (const BYTE *)psrc;
                STPTRDST(LPVOID, plocalwalk);
                INCLOCAL(sizeof(VASigCookieEx));
                break;
                
            case ML_CSTR_C2N:
                STPTRDST( LPSTR, ((ML_CSTR_C2N_SR*)plocalwalk)->DoConversion(LDSRC(STRINGREF), pCleanupWorkList) );
                INCLOCAL(sizeof(ML_CSTR_C2N_SR));
                break;

            case ML_HANDLEREF_C2N:
                STPTRDST( LPVOID, LDSRC(HANDLEREF).m_handle );
                break;
            
            case ML_WSTRBUILDER_C2N:
                STPTRDST( LPWSTR, ((ML_WSTRBUILDER_C2N_SR*)plocalwalk)->DoConversion((STRINGBUFFERREF*)psrc, pCleanupWorkList) );
                INCSRC(STRINGBUFFERREF);
                INCLOCAL(sizeof(ML_WSTRBUILDER_C2N_SR));
                break;

            case ML_WSTRBUILDER_C2N_POST:
                ((ML_WSTRBUILDER_C2N_SR*)(plocals + LDCODE16()))->BackPropagate(&fDeferredException);
                break;

            case ML_CSTRBUILDER_C2N:
                STPTRDST( LPSTR, ((ML_CSTRBUILDER_C2N_SR*)plocalwalk)->DoConversion((STRINGBUFFERREF*)psrc, pCleanupWorkList) );
                INCSRC(STRINGBUFFERREF);
                INCLOCAL(sizeof(ML_CSTRBUILDER_C2N_SR));
                break;

            case ML_CSTRBUILDER_C2N_POST:
                ((ML_CSTRBUILDER_C2N_SR*)(plocals + LDCODE16()))->BackPropagate(&fDeferredException);
                break;

            case ML_CBOOL_C2N:
                STDST(SignedI4TargetType, LDSRC(BYTE) ? 1 : 0);
                break;

            case ML_CBOOL_N2C:
                STDST(SignedI4TargetType, LDSRC(BYTE) ? 1 : 0);
                break;

            case ML_LCID_C2N:
                STDST(LCID, GetThread()->GetCultureId(FALSE));
                break;

            case ML_LCID_N2C:
            {
                BOOL bSuccess = FALSE;
                OBJECTREF OldCulture = GetThread()->GetCulture(FALSE);
                GCPROTECT_BEGIN(OldCulture)
                {
                    EE_TRY_FOR_FINALLY
                    {               
                        GetThread()->SetCultureId(LDSRC(LCID), FALSE);
                        pCleanupWorkList->ScheduleUnconditionalCultureRestore(GetThread()->GetCulture(FALSE));
                        bSuccess = TRUE;
                    }
                    EE_FINALLY
                    {
                        // If we failed to either set the culture or schedule the restore then restore the
                        // thread's culture to the old culture.
                        if (!bSuccess)
                            GetThread()->SetCulture(OldCulture, FALSE);
                    }           
                    EE_END_FINALLY
                }
                GCPROTECT_END();
                break;			
            }


            case ML_STRUCTRETN2C:
                pMT = (MethodTable *)LDCODE_POINTER();
                ((ML_STRUCTRETN2C_SR*)plocalwalk)->m_pNativeRetBuf = LDSRC(LPVOID);

                if (! (((ML_STRUCTRETN2C_SR*)plocalwalk)->m_pNativeRetBuf) )
                {
                    COMPlusThrow(kArgumentNullException, L"ArgumentNull_Generic");
                }

                ((ML_STRUCTRETN2C_SR*)plocalwalk)->m_pMT = pMT;
                ((ML_STRUCTRETN2C_SR*)plocalwalk)->m_ppProtectedBoxedObj = pCleanupWorkList->NewProtectedObjectRef(FastAllocateObject(pMT));
                STDST(LPVOID, (*((ML_STRUCTRETN2C_SR*)plocalwalk)->m_ppProtectedBoxedObj)->GetData()); 
                INCLOCAL(sizeof(ML_STRUCTRETN2C_SR));
                break;

            case ML_STRUCTRETN2C_POST:
                ((ML_STRUCTRETN2C_SR*)(plocals + LDCODE16()))->MarshalRetVal(&fDeferredException);
                break;

            case ML_STRUCTRETC2N:
                pMT = (MethodTable *)LDCODE_POINTER();
                ((ML_STRUCTRETC2N_SR*)plocalwalk)->m_ppProtectedValueTypeBuf = (LPVOID*)psrc;
                INCSRC(LPVOID);
                ((ML_STRUCTRETC2N_SR*)plocalwalk)->m_pMT = pMT;
                ((ML_STRUCTRETC2N_SR*)plocalwalk)->m_pNativeRetBuf = (LPVOID)(GetThread()->m_MarshalAlloc.Alloc(pMT->GetNativeSize()));
                STDST(LPVOID, ((ML_STRUCTRETC2N_SR*)plocalwalk)->m_pNativeRetBuf);
                INCLOCAL(sizeof(ML_STRUCTRETC2N_SR));
                break;

            case ML_STRUCTRETC2N_POST:
                ((ML_STRUCTRETC2N_SR*)(plocals + LDCODE16()))->MarshalRetVal(&fDeferredException);
                break;

            case ML_CURRENCYRETC2N:
                ((ML_CURRENCYRETC2N_SR*)plocalwalk)->m_ppProtectedValueTypeBuf = (DECIMAL**)psrc;
                INCSRC(DECIMAL*);
                STDST(CURRENCY*, &( ((ML_CURRENCYRETC2N_SR*)plocalwalk)->m_cy ));
                INCLOCAL(sizeof(ML_CURRENCYRETC2N_SR));
                break;

            case ML_CURRENCYRETC2N_POST:
                ((ML_CURRENCYRETC2N_SR*)(plocals + LDCODE16()))->MarshalRetVal(&fDeferredException);
                break;


            case ML_COPYPINNEDGCREF:
                STDST( LPVOID, LDSRC(LPVOID) );
                break;

            case ML_CURRENCYRETN2C:
                ((ML_CURRENCYRETN2C_SR*)plocalwalk)->m_pcy = LDSRC(CURRENCY*);
                if (!(((ML_CURRENCYRETN2C_SR*)plocalwalk)->m_pcy))
                {
                    COMPlusThrow(kArgumentNullException, L"ArgumentNull_Generic");
                }

                STDST(DECIMAL*, &( ((ML_CURRENCYRETN2C_SR*)plocalwalk)->m_decimal ));
                INCLOCAL(sizeof(ML_CURRENCYRETN2C_SR));
                break;

            case ML_CURRENCYRETN2C_POST:
                ((ML_CURRENCYRETN2C_SR*)(plocals + LDCODE16()))->MarshalRetVal(&fDeferredException);
                break;

            case ML_DATETIMERETN2C:
                ((ML_DATETIMERETN2C_SR*)plocalwalk)->m_pdate = LDSRC(DATE*);
                if (!(((ML_DATETIMERETN2C_SR*)plocalwalk)->m_pdate))
                {
                    COMPlusThrow(kArgumentNullException, L"ArgumentNull_Generic");
                }

                STDST(INT64*, &( ((ML_DATETIMERETN2C_SR*)plocalwalk)->m_datetime ));
                INCLOCAL(sizeof(ML_DATETIMERETN2C_SR));
                break;

            case ML_DATETIMERETN2C_POST:
                ((ML_DATETIMERETN2C_SR*)(plocals + LDCODE16()))->MarshalRetVal(&fDeferredException);
                break;

            case ML_CREATE_MARSHALER_INTERFACE:
                pMarshaler = new (plocalwalk) InterfaceMarshaler(pCleanupWorkList);
                _ASSERTE(pMarshaler != NULL);
                ((InterfaceMarshaler*)pMarshaler)->SetClassMT((MethodTable *)LDCODE_POINTER());
                ((InterfaceMarshaler*)pMarshaler)->SetItfMT((MethodTable *)LDCODE_POINTER());
                plocalwalk += sizeof(InterfaceMarshaler);
                break;

            case ML_THROWIFHRFAILED:
                {
                    HRESULT   tempHR = LDSRC(HRESULT);
                    if (FAILED(tempHR))
                        COMPlusThrowHR(tempHR);
                }
                break;


            default:

#ifndef _DEBUG
                __assume(0);
#endif
                _ASSERTE(!"RunML: Unrecognized ML opcode");
                break;
        }
        if (bVariant)
        {
            resetbVariant --;
            if (resetbVariant == 0)
                bVariant = false;
        }
        _ASSERTE(plocalwalk - poldlocalwalk == gMLInfo[*pMLCodeSave].m_cbLocal);
        _ASSERTE(pMLCode - pMLCodeSave == gMLInfo[*pMLCodeSave].m_numOperandBytes + 1);
    }

#undef LDSRC
#undef STDST
#undef INCLOCAL

}






//===========================================================================
// Do conversion for N/Direct parameters of type "Object"
// "Object" is a catch-all type that supports a number of conversions based
// on the runtime type.
//===========================================================================
LPVOID ML_OBJECT_C2N_SR::DoConversion(OBJECTREF       *ppProtectedObjectRef,  
                                      BYTE             inout,
                                      BYTE             fIsAnsi,
                                      CleanupWorkList *pCleanup)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(inout == ML_IN || inout == ML_OUT || inout == (ML_IN|ML_OUT));

    m_backproptype = BP_NONE;
    m_inout        = inout;
    m_fIsAnsi      = fIsAnsi;

    if (*ppProtectedObjectRef == NULL) {
        return NULL;
    } else {

        MethodTable *pMT = (*ppProtectedObjectRef)->GetMethodTable();
        if (pMT->IsArray()) {
            CorElementType etyp = ((ArrayClass *) pMT->GetClass())->GetElementType();
            VARTYPE vt = VT_EMPTY;

            switch (etyp)
            {
                case ELEMENT_TYPE_I1:      vt = VT_I1; break;
                case ELEMENT_TYPE_U1:      vt = VT_UI1; break;
                case ELEMENT_TYPE_I2:      vt = VT_I2; break;
                case ELEMENT_TYPE_U2:      vt = VT_UI2; break;
                IN_WIN32(case ELEMENT_TYPE_I:)
                case ELEMENT_TYPE_I4:      vt = VT_I4; break;
                IN_WIN32(case ELEMENT_TYPE_U:)
                case ELEMENT_TYPE_U4:      vt = VT_UI4; break;
                IN_WIN64(case ELEMENT_TYPE_I:)
                case ELEMENT_TYPE_I8:      vt = VT_I8; break;
                IN_WIN64(case ELEMENT_TYPE_U:)
                case ELEMENT_TYPE_U8:      vt = VT_UI8; break;
                case ELEMENT_TYPE_R4:      vt = VT_R4; break;
                case ELEMENT_TYPE_R8:      vt = VT_R8; break;
                case ELEMENT_TYPE_R:       vt = VT_R8; break;
                case ELEMENT_TYPE_CHAR:    vt = m_fIsAnsi ? VTHACK_ANSICHAR : VT_UI2 ; break;
                case ELEMENT_TYPE_BOOLEAN: vt = VTHACK_WINBOOL; break;
                default:
                    COMPlusThrow(kArgumentException, IDS_EE_NDIRECT_BADOBJECT);
            }

            m_backproptype = BP_UNMARSHAL;
            Marshaler *pMarshaler;
            pMarshaler = new (&m_nativearraymarshaler) NativeArrayMarshaler(pCleanup);
            _ASSERTE(pMarshaler != NULL);
            ((NativeArrayMarshaler*)pMarshaler)->SetElementMethodTable(pMT);
            ((NativeArrayMarshaler*)pMarshaler)->SetElementType(vt);
            ((NativeArrayMarshaler*)pMarshaler)->SetElementCount( (*((BASEARRAYREF*)ppProtectedObjectRef))->GetNumComponents() );


        } else {
            if (pMT == g_pStringClass) {
                m_inout = ML_IN;
                m_backproptype = BP_UNMARSHAL;

                if (m_fIsAnsi) {
                    new (&m_cstrmarshaler) CSTRMarshaler(pCleanup);
                } else {
                    new (&m_wstrmarshaler) WSTRMarshaler(pCleanup);
                }
            } else {
                if (g_Mscorlib.IsClass(pMT, CLASS__STRING_BUILDER)
                    || GetAppDomain()->IsSpecialStringBuilderClass(pMT)) {
                    if (m_fIsAnsi) {
                        new (&m_cstrbuffermarshaler) CSTRBufferMarshaler(pCleanup);
                    } else {
                        new (&m_wstrbuffermarshaler) WSTRBufferMarshaler(pCleanup);
                    }
                    m_backproptype = BP_UNMARSHAL;

                } else if (pMT->GetClass()->HasLayout()) {
                    m_backproptype = BP_UNMARSHAL;
                    Marshaler *pMarshaler;
                    pMarshaler = new (&m_layoutclassptrmarshaler) LayoutClassPtrMarshaler(pCleanup, pMT);
                    _ASSERTE(pMarshaler != NULL);
                }

            }
        }

        if (m_backproptype == BP_UNMARSHAL)
        {
            LPVOID nativevalue = NULL;
            switch (m_inout)
            {
                case ML_IN: //fallthru
                case ML_IN|ML_OUT:
                    ((Marshaler*)&m_marshaler)->MarshalComToNative(ppProtectedObjectRef, &nativevalue);
                    break;
                case ML_OUT:
                    ((Marshaler*)&m_marshaler)->MarshalComToNativeOut(ppProtectedObjectRef, &nativevalue);
                    break;
                default:
                    _ASSERTE(0);
            }
            return nativevalue;
        }
    }


    // If we got here, we were passed an unsupported type
    COMPlusThrow(kArgumentException, IDS_EE_NDIRECT_BADOBJECT);
    return NULL;


}



VOID ML_OBJECT_C2N_SR::BackPropagate(BOOL *pfDeferredException)
{
    switch (m_backproptype) {
        case BP_NONE:
            //nothing
            break;

        case BP_UNMARSHAL:
            switch (m_inout)
            {
                case ML_IN:
                    ((Marshaler*)&m_marshaler)->UnmarshalComToNativeIn();
                    break;

                case ML_OUT:
                    ((Marshaler*)&m_marshaler)->UnmarshalComToNativeOut();
                    break;

                case ML_IN|ML_OUT:
                    ((Marshaler*)&m_marshaler)->UnmarshalComToNativeInOut();
                    break;

                default:
                    _ASSERTE(0);

            }
            break;

        default:
            _ASSERTE(0);

    }
}




#ifdef _DEBUG

VOID DisassembleMLStream(const MLCode *pMLCode)
{
    MLCode mlcode;
    while (ML_END != (mlcode = *(pMLCode++)))
    {
        UINT numOperands = gMLInfo[mlcode].m_numOperandBytes;
        LOG((LF_INTEROP, LL_INFO10000, "  %-20s ", gMLInfo[mlcode].m_szDebugName));
        for (UINT i = 0; i < numOperands; i++)
        {
            LOG((LF_INTEROP, LL_INFO10000, "%lxh ", (ULONG)*(pMLCode++)));
        }
        LOG((LF_INTEROP, LL_INFO10000, "\n"));
    }
    LOG((LF_INTEROP, LL_INFO10000, "  ML_END\n"));
}
#endif



//----------------------------------------------------------------------
// Convert ArrayWithOffset to native array
//----------------------------------------------------------------------
LPVOID ML_ARRAYWITHOFFSET_C2N_SR::DoConversion(BASEARRAYREF    *ppProtectedArrayRef, //pointer to GC-protected BASERARRAYREF,
                                               UINT32           cbOffset,
                                               UINT32           cbCount,
                                               CleanupWorkList *pCleanup)
{
    THROWSCOMPLUSEXCEPTION();

    m_ppProtectedArrayRef = ppProtectedArrayRef;
    if (!*ppProtectedArrayRef) {
        return NULL;
    } else {
        m_cbOffset = cbOffset;
        m_cbCount  = cbCount;
        if (cbCount > kStackBufferSize) {
            m_pNativeArray = GetThread()->m_MarshalAlloc.Alloc(cbCount);
        } else {
            m_pNativeArray = m_StackBuffer;
        }
        memcpyNoGCRefs(m_pNativeArray, cbOffset + (LPBYTE) ((*ppProtectedArrayRef)->GetDataPtr()), cbCount);
        return m_pNativeArray;
    }
}





//----------------------------------------------------------------------
// Backpropagates changes to the native array back to the COM+ array.
//----------------------------------------------------------------------
VOID   ML_ARRAYWITHOFFSET_C2N_SR::BackPropagate()
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    if (*m_ppProtectedArrayRef != NULL) {
       memcpyNoGCRefs(m_cbOffset + (LPBYTE) ((*m_ppProtectedArrayRef)->GetDataPtr()), m_pNativeArray, m_cbCount);
    }
}







LPVOID ML_REFVALUECLASS_C2N_SR::DoConversion(LPVOID          *ppProtectedData,
                                             MethodTable     *pMT,
                                             BYTE             fInOut,
                                             CleanupWorkList *pCleanup         
                                           )
{
    m_ppProtectedData = ppProtectedData;
    m_pMT             = pMT;
    m_inout           = fInOut;
    _ASSERTE( fInOut == ML_IN || fInOut == ML_OUT || fInOut == (ML_IN|ML_OUT) );

    m_buf = (BYTE*)(pCleanup->NewScheduleLayoutDestroyNative(pMT));
    if (m_inout & ML_IN)
    {
        FmtValueTypeUpdateNative(ppProtectedData, pMT, m_buf);
    }
    else
    {
        FillMemory(m_buf, pMT->GetNativeSize(), 0);
    }
    return m_buf;

}

VOID ML_REFVALUECLASS_C2N_SR::BackPropagate(BOOL *pfDeferredException)
{
    if (m_inout & ML_OUT)
    {
        COMPLUS_TRY {
            FmtValueTypeUpdateComPlus(m_ppProtectedData, m_pMT, m_buf, FALSE);
        } COMPLUS_CATCH {
            *pfDeferredException = TRUE;
        } COMPLUS_END_CATCH
    }
}



LPVOID ML_REFVALUECLASS_N2C_SR::DoConversion(LPVOID pUmgdVALUECLASS, BYTE fInOut, MethodTable *pMT, CleanupWorkList *pCleanup)
{
    THROWSCOMPLUSEXCEPTION();

    m_fInOut = fInOut;
    m_pMT    = pMT;
    m_pUnmgdVC = pUmgdVALUECLASS;

    OBJECTREF pBoxedVALUECLASS = FastAllocateObject(pMT);
    m_pObjHnd = pCleanup->NewScheduledProtectedHandle(pBoxedVALUECLASS);

    if (fInOut & ML_IN) {
        LayoutUpdateComPlus((VOID**)m_pObjHnd, Object::GetOffsetOfFirstField(), pMT->GetClass(), (BYTE*)pUmgdVALUECLASS, FALSE);
    }
    return (LPVOID)(ObjectFromHandle(m_pObjHnd)->GetData());
}
        

VOID ML_REFVALUECLASS_N2C_SR::BackPropagate(BOOL *pfDeferredException)
{
    if (m_fInOut & ML_OUT) {
        COMPLUS_TRY {
            if (m_fInOut & ML_IN) {
                FmtClassDestroyNative(m_pUnmgdVC, m_pMT->GetClass());
            }
            LayoutUpdateNative((VOID**)m_pObjHnd, Object::GetOffsetOfFirstField(), m_pMT->GetClass(), (BYTE*)m_pUnmgdVC, NULL);
        } COMPLUS_CATCH {
            *pfDeferredException = TRUE;
        } COMPLUS_END_CATCH
    }
}

LPVOID ML_MARSHAL_RETVAL_LGBLITTABLEVALUETYPE_C2N_SR::DoConversion(const VOID * psrc, UINT32 cbSize)
{
    THROWSCOMPLUSEXCEPTION();
    m_psrc = psrc;
    m_cbSize = cbSize;
    m_pTempCopy = (LPVOID)(GetThread()->m_MarshalAlloc.Alloc(cbSize));
    return m_pTempCopy;
}

VOID ML_MARSHAL_RETVAL_LGBLITTABLEVALUETYPE_C2N_SR::BackPropagate(BOOL *pfDeferredException)
{
    memcpyNoGCRefs(*((VOID**)m_psrc), m_pTempCopy, m_cbSize);
}




LPSTR ML_CSTR_C2N_SR::DoConversion(STRINGREF pStringRef, CleanupWorkList *pCleanup)
{
    THROWSCOMPLUSEXCEPTION();

    if (pStringRef == NULL)
    {
        return NULL;
    }
    else
    {
        int    nc;
        WCHAR *pc;
        RefInterpretGetStringValuesDangerousForGC(pStringRef, &pc, &nc);
        int    cb = (nc + 1) * GetMaxDBCSCharByteSize() + 1;
        BYTE  *pbuf = m_buf;
        if (cb > (int) sizeof(m_buf))
        {
            pbuf = (BYTE*)CoTaskMemAlloc(cb);
            if (!pbuf)
            {
                COMPlusThrowOM();
            }
            pCleanup->ScheduleCoTaskFree(pbuf);
        }
        
        DWORD mblength = 0;

        if (nc+1)
        {
            mblength = WszWideCharToMultiByte(CP_ACP, 0,
                                    pStringRef->GetBuffer(), (nc+1),
                                    (LPSTR) pbuf, ((nc+1) * GetMaxDBCSCharByteSize()) + 1,
                                    NULL, NULL);
            if (mblength == 0)
                COMPlusThrowWin32();
        }
        ((CHAR*)pbuf)[mblength] = '\0';

        return (LPSTR)(pbuf);
    }
}




LPWSTR ML_WSTRBUILDER_C2N_SR::DoConversion(STRINGBUFFERREF *ppProtectedStringBuffer, CleanupWorkList *pCleanup)
{

    m_ppProtectedStringBuffer = ppProtectedStringBuffer;
    STRINGBUFFERREF stringRef = *m_ppProtectedStringBuffer;


    if (stringRef == NULL)
    {
        m_pNative = NULL;
    }
    else
    {
        UINT32 capacity = (UINT32)COMStringBuffer::NativeGetCapacity(stringRef);
        m_pNative = (LPWSTR)(GetThread()->m_MarshalAlloc.Alloc((unsigned int) (max(256, (capacity+3) * sizeof(WCHAR)))));
        // HACK: N/Direct can be used to call Win32 apis that don't
        // strictly follow COM+ in/out semantics and thus may leave
        // garbage in the buffer in circumstances that we can't detect.
        // To prevent the marshaler from crashing when converting the
        // contents back to COM, ensure that there's a hidden null terminator
        // past the end of the official buffer.
        m_pNative[capacity+1] = L'\0';
        m_pSentinel = &(m_pNative[capacity+1]);
        m_pSentinel[1] = 0xabab;
#ifdef _DEBUG
        FillMemory(m_pNative, (capacity+1)*sizeof(WCHAR), 0xcc);
#endif
        SIZE_T length = COMStringBuffer::NativeGetLength(stringRef);

        memcpyNoGCRefs((WCHAR *) m_pNative, COMStringBuffer::NativeGetBuffer(stringRef),
                       length * sizeof(WCHAR));
        ((WCHAR*)m_pNative)[length] = 0;

    
    
    }
    return m_pNative;
}



VOID ML_WSTRBUILDER_C2N_SR::BackPropagate(BOOL *pfDeferredException)
{
    THROWSCOMPLUSEXCEPTION();
    if (m_pNative != NULL)
    {
        if (L'\0' != m_pSentinel[0] || 0xabab != m_pSentinel[1])
        {
            // ! Though our normal protocol is to set *pfDeferredException to TRUE and return normally,
            // ! this mechanism doesn't let us return informative exceptions. So we'll throw this one 
            // ! immediately. The excuse for doing this is that this exception's only purpose in
            // ! life is to tell the programmer that he introduced a bug that probably corrupted the heap.
            // ! Hence, assuming the process even survives long enough to deliver our message,
            // ! memory leaks from any other backpropagations bypassed as a result is the least of 
            // ! our problems.
            COMPlusThrow(kIndexOutOfRangeException, IDS_PINVOKE_STRINGBUILDEROVERFLOW);
        }

        COMStringBuffer::ReplaceBuffer(m_ppProtectedStringBuffer,
                                       m_pNative, (INT32)wcslen(m_pNative));
    
    
    }
}






LPSTR ML_CSTRBUILDER_C2N_SR::DoConversion(STRINGBUFFERREF *ppProtectedStringBuffer, CleanupWorkList *pCleanup)
{

    THROWSCOMPLUSEXCEPTION();

    m_ppProtectedStringBuffer = ppProtectedStringBuffer;
    STRINGBUFFERREF stringRef = *m_ppProtectedStringBuffer;


    if (stringRef == NULL)
    {
        m_pNative = NULL;
    }
    else
    {
        UINT32 capacity = (UINT32)COMStringBuffer::NativeGetCapacity(stringRef);


        // capacity is the count of wide chars, allocate buffer big enough for maximum
        // conversion to DBCS.
        m_pNative = (LPSTR)(GetThread()->m_MarshalAlloc.Alloc(max(256, (capacity * GetMaxDBCSCharByteSize()) + 5)));

        // HACK: N/Direct can be used to call Win32 apis that don't
        // strictly follow COM+ in/out semantics and thus may leave
        // garbage in the buffer in circumstances that we can't detect.
        // To prevent the marshaler from crashing when converting the
        // contents back to COM, ensure that there's a hidden null terminator
        // past the end of the official buffer.
        m_pSentinel = &(m_pNative[capacity+1]);
        ((CHAR*)m_pSentinel)[0] = '\0';
        ((CHAR*)m_pSentinel)[1] = '\0';
        ((CHAR*)m_pSentinel)[2] = '\0';
        ((CHAR*)m_pSentinel)[3] = (CHAR)(SIZE_T)0xab;

#ifdef _DEBUG
        FillMemory(m_pNative, (capacity+1) * sizeof(CHAR), 0xcc);
#endif
        UINT32 length = (UINT32)COMStringBuffer::NativeGetLength(stringRef);
        DWORD mblength = 0;

        if (length)
        {

            mblength = WszWideCharToMultiByte(CP_ACP, 0,
                                    COMStringBuffer::NativeGetBuffer(stringRef), length,
                                    (LPSTR) m_pNative, (capacity * GetMaxDBCSCharByteSize()) + 4,
                                    NULL, NULL);
            if (mblength == 0)
                COMPlusThrowWin32();
        }
        ((CHAR*)m_pNative)[mblength] = '\0';


    }
    return m_pNative;
}



VOID ML_CSTRBUILDER_C2N_SR::BackPropagate(BOOL *pfDeferredException)
{
    THROWSCOMPLUSEXCEPTION();
    if (m_pNative != NULL)
    {
        if (0 != m_pSentinel[0] || 
            0 != m_pSentinel[1] ||
            0 != m_pSentinel[2]  ||
            (CHAR)(SIZE_T)0xab != m_pSentinel[3])
        {
            // ! Though our normal protocol is to set *pfDeferredException to TRUE and return normally,
            // ! this mechanism doesn't let us return informative exceptions. So we'll throw this one 
            // ! immediately. The excuse for doing this is that this exception's only purpose in
            // ! life is to tell the programmer that he introduced a bug that probably corrupted the heap.
            // ! Hence, assuming the process even survives long enough to deliver our message,
            // ! memory leaks from any other backpropagations bypassed as a result is the least of 
            // ! our problems.
            COMPlusThrow(kIndexOutOfRangeException, IDS_PINVOKE_STRINGBUILDEROVERFLOW);
        }

        COMStringBuffer::ReplaceBufferAnsi(m_ppProtectedStringBuffer,
                                           m_pNative, (INT32)strlen(m_pNative));
    
    
    }
}

//====================================================================
// Helper fcns called from standalone stubs.
//====================================================================
VOID STDMETHODCALLTYPE DoMLCreateMarshalerWStr(Frame *pFrame, CleanupWorkList *pCleanup, UINT8 *plocalwalk)
{
    pCleanup->IsVisibleToGc();
    new (plocalwalk) WSTRMarshaler(pCleanup);
}

VOID STDMETHODCALLTYPE DoMLCreateMarshalerCStr(Frame *pFrame, CleanupWorkList *pCleanup, UINT8 *plocalwalk)
{
    pCleanup->IsVisibleToGc();
    new (plocalwalk) CSTRMarshaler(pCleanup);
}


VOID STDMETHODCALLTYPE DoMLPrereturnC2N(Marshaler *pMarshaler, LPVOID pstackout)
{
    pMarshaler->PrereturnComFromNativeRetval(NULL, pstackout);
}


LPVOID STDMETHODCALLTYPE DoMLReturnC2NRetVal(Marshaler *pMarshaler)
{
    // WARNING!!!!! "dstobjref" holds an OBJECTREF: Don't add any operations
    // that might cause a GC!
    LPVOID dstobjref;

    pMarshaler->ReturnComFromNativeRetval( (LPVOID)&dstobjref, NULL );
    return dstobjref;
}


void ML_STRUCTRETN2C_SR::MarshalRetVal(BOOL *pfDeferredException)
{
    COMPLUS_TRY {

        LayoutUpdateNative( (LPVOID*)m_ppProtectedBoxedObj, Object::GetOffsetOfFirstField(), m_pMT->GetClass(), (BYTE*)m_pNativeRetBuf, NULL);

    } COMPLUS_CATCH {
        *pfDeferredException = TRUE;
    } COMPLUS_END_CATCH
}




void ML_STRUCTRETC2N_SR::MarshalRetVal(BOOL *pfDeferredException)
{
    COMPLUS_TRY {

        LayoutUpdateComPlus( m_ppProtectedValueTypeBuf, 0, m_pMT->GetClass(), (BYTE*)m_pNativeRetBuf, TRUE);

    } COMPLUS_CATCH {
        *pfDeferredException = TRUE;
    } COMPLUS_END_CATCH
}



void ML_CURRENCYRETC2N_SR::MarshalRetVal(BOOL *pfDeferredException)
{
    HRESULT hr;
    hr = VarDecFromCy(m_cy, *m_ppProtectedValueTypeBuf);
    if (FAILED(hr))
    {
        *pfDeferredException = TRUE;
    }

}


void ML_CURRENCYRETN2C_SR::MarshalRetVal(BOOL *pfDeferredException)
{
    HRESULT hr;
    hr = VarCyFromDec(&m_decimal, m_pcy);
    if (FAILED(hr))
    {
        *pfDeferredException = TRUE;
    }
}


void ML_DATETIMERETN2C_SR::MarshalRetVal(BOOL *pfDeferredException)
{
    *m_pdate = COMDateTime::TicksToDoubleDate(m_datetime);
}


