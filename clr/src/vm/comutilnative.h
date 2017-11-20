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
** Header:  COMUtilNative
**       
**
** Purpose: A dumping ground for classes which aren't large
** enough to get their own file in the VM.
**
** Date:  April 8, 1998
** 
===========================================================*/
#ifndef _COMUTILNATIVE_H_
#define _COMUTILNATIVE_H_

#include "object.h"
#include "util.hpp"
#include "cgensys.h"
#include "fcall.h"

//
//
// COMCHARACTER
//
//
class COMCharacter {
public:
    static FCDECL1(Object*, ToString, WCHAR c);

    //These are here for support from native code.  They are never called from our managed classes.
    static BOOL nativeIsWhiteSpace(WCHAR c);
    static BOOL nativeIsDigit(WCHAR c);
};

//
//
// PARSE NUMBERS
//
//

#define MinRadix 2
#define MaxRadix 36

class ParseNumbers {
    
    enum FmtFlags {
      LeftAlign = 0x1,  //Ensure that these conform to the values specified in the managed files.
      CenterAlign = 0x2,
      RightAlign = 0x4,
      PrefixSpace = 0x8,
      PrintSign = 0x10,
      PrintBase = 0x20,
      TreatAsUnsigned = 0x10,
      PrintAsI1 = 0x40,
      PrintAsI2 = 0x80,
      PrintAsI4 = 0x100,
      PrintRadixBase = 0x200,
      AlternateForm = 0x400};

public:

    static INT32 GrabInts(const INT32, WCHAR *, const int, int *, BOOL);
    static INT64 GrabLongs(const INT32, WCHAR *, const int, int *, BOOL);

    static FCDECL5(LPVOID, IntToString, INT32 l, INT32 radix, INT32 width, WCHAR paddingChar, INT32 flags);
    static FCDECL1(LPVOID, IntToDecimalString, INT32 l);
    static FCDECL5(LPVOID, LongToString, INT32 radix, INT32 width, INT64 l, WCHAR paddingChar, INT32 flags);
    static FCDECL4(INT32, StringToInt, StringObject * s, INT32 radix, INT32 flags, I4Array *currPos);
    static FCDECL4(INT64, StringToLong, StringObject * s, INT32 radix, INT32 flags, I4Array *currPos);
    static FCDECL4(INT64, RadixStringToLong, StringObject *s, INT32 radix, BYTE isTight, I4Array *currPos);

};

//
//
// EXCEPTION NATIVE
//
//

struct ExceptionData
{
    HRESULT hr;
    BSTR    bstrDescription;
    BSTR    bstrSource;
    BSTR    bstrHelpFile;
    DWORD   dwHelpContext;
        GUID    guid;
};

void FreeExceptionData(ExceptionData *pedata);


class ExceptionNative
{
public:
    static FCDECL1(Object*, GetClassName, Object* pThisUNSAFE);
    static BOOL      IsException(EEClass* pClass);

    // NOTE: caller cleans up any partially initialized BSTRs in pED
    static void      GetExceptionData(OBJECTREF, ExceptionData *);

    // Note: these are on the PInvoke class to hide these from the user.
    static FCDECL0(EXCEPTION_POINTERS*, GetExceptionPointers);
    static FCDECL0(INT32, GetExceptionCode);
};


//
//
// GUID NATIVE
//
//

class GuidNative
{
    //typedef struct {
    //            DECLARE_ECALL_PTR_ARG(GUID*, thisPtr);
    //    } _CompleteGuidArgs;
public:
    static FCDECL1(void, CompleteGuid, GUID* thisPtr);
    static void FillGUIDFromObject(GUID *pguid, OBJECTREF const *prefGuid);
    static void FillObjectFromGUID(GUID* poutGuid, const GUID *pguid);
    static OBJECTREF CreateGuidObject(const GUID *pguid);
};


//
// BitConverter
//
class BitConverter {
private:
    static U1ARRAYREF __stdcall ByteCopyHelper(int arraySize, void *data);
public:
    //typedef struct {
    //    DECLARE_ECALL_I4_ARG(INT32, value); 
    //} _CharToBytesArgs;
    //typedef struct {
    //    DECLARE_ECALL_I4_ARG(UINT32, value); 
    //} _U2ToBytesArgs;
    //typedef struct {
    //    DECLARE_ECALL_I4_ARG(UINT32, value); 
    //} _U4ToBytesArgs;
    //typedef struct {
    //    DECLARE_ECALL_I8_ARG(UINT64, value); 
    //} _U8ToBytesArgs;
    //typedef struct {
    //    DECLARE_ECALL_I4_ARG(INT32, StartIndex); 
    //    DECLARE_ECALL_OBJECTREF_ARG(PTRARRAYREF, value); 
    //} _BytesToXXArgs;
    //typedef struct {
    //    DECLARE_ECALL_I4_ARG(INT32, Length); 
    //    DECLARE_ECALL_I4_ARG(INT32, StartIndex); 
    //    DECLARE_ECALL_OBJECTREF_ARG(PTRARRAYREF, value); 
    //} _BytesToStringArgs;

    static FCDECL1(Object*, CharToBytes, INT32 value);
    static FCDECL1(U1Array*, I2ToBytes, INT32 value);
    static FCDECL1(U1Array*, I4ToBytes, INT32 value);
    static FCDECL1(U1Array*, I8ToBytes, INT64 value);
    static FCDECL1(Object*, U2ToBytes, UINT32 value);
    static FCDECL1(Object*, U4ToBytes, UINT32 value);
    static FCDECL1(Object*, U8ToBytes, UINT64 value);
    static FCDECL2(INT32, BytesToChar, PTRArray* valueUNSAFE, INT32 StartIndex);
    static FCDECL2(INT32, BytesToI2, PTRArray* valueUNSAFE, INT32 StartIndex);
    static FCDECL2(INT32, BytesToI4, PTRArray* valueUNSAFE, INT32 StartIndex);
    static FCDECL2(INT64, BytesToI8, PTRArray* valueUNSAFE, INT32 StartIndex);
    static FCDECL2(UINT32, BytesToU2, PTRArray* valueUNSAFE, INT32 StartIndex);
    static FCDECL2(UINT32, BytesToU4, PTRArray* valueUNSAFE, INT32 StartIndex);
    static FCDECL2(UINT64, BytesToU8, PTRArray* valueUNSAFE, INT32 StartIndex);
    static FCDECL2(R4, BytesToR4, PTRArray* valueUNSAFE, INT32 StartIndex);
    static FCDECL2(R8, BytesToR8, PTRArray* valueUNSAFE, INT32 StartIndex);
    static FCDECL3(Object*, BytesToString, PTRArray* valueUNSAFE, INT32 StartIndex, INT32 Length);
    static FCDECL3(Object*, ByteArrayToBase64String, U1Array* pInArray, INT32 offset, INT32 length);
    static FCDECL1(Object*, Base64StringToByteArray, StringObject* pvInString);
    static FCDECL5(INT32, ByteArrayToBase64CharArray, U1Array* pInArray, INT32 offsetIn, INT32 length, CHARArray* pOutArray, INT32 offsetOut);
    static FCDECL3(Object*, Base64CharArrayToByteArray, CHARArray* pInCharArray, INT32 offset, INT32 length);
    
    static INT32 ConvertToBase64Array(WCHAR *outChars,U1 *inData,UINT offset,UINT length);
    static INT32 ConvertBase64ToByteArray(INT32 *value,WCHAR *c,UINT offset,UINT length, UINT* trueLength);
    static INT32 ConvertByteArrayToByteStream(INT32 *value,U1* b,UINT length);

    static const WCHAR base64[];
};


//
// Buffer
//
class Buffer {
private:
    //struct _GetByteArgs
    //{
    //    DECLARE_ECALL_I4_ARG(INT32, index);
    //    DECLARE_ECALL_OBJECTREF_ARG(BASEARRAYREF, array);
    //};
    //struct _SetByteArgs
    //{
    //    DECLARE_ECALL_I1_ARG(BYTE, value);
    //    DECLARE_ECALL_I4_ARG(INT32, index);
    //    DECLARE_ECALL_OBJECTREF_ARG(BASEARRAYREF, array);
    //};
    //struct _ArrayArgs
    //{
    //    DECLARE_ECALL_OBJECTREF_ARG(BASEARRAYREF, array);
    //};

public:
    
    // BlockCopy
    // This method from one primitive array to another based
    //      upon an offset into each an a byte count.
    static FCDECL5(VOID, BlockCopy, ArrayBase *src, int srcOffset, ArrayBase *dst, int dstOffset, int count);
    static FCDECL5(VOID, InternalBlockCopy, ArrayBase *src, int srcOffset, ArrayBase *dst, int dstOffset, int count);
    
    static FCDECL2(BYTE, GetByte, ArrayBase* arrayUNSAFE, INT32 index);
    static FCDECL3(void, SetByte, ArrayBase* arrayUNSAFE, INT32 index, BYTE value);
    static FCDECL1(INT32, ByteLength, ArrayBase* arrayUNSAFE);
};

class GCInterface {
private:
    static BOOL m_cacheCleanupRequired;
    static MethodDesc *m_pCacheMethod;

    public:
    static BOOL IsCacheCleanupRequired();
    static void CleanupCache();
    static void SetCacheCleanupRequired(BOOL);
    
    // The following structure is provided to the stack skip function.  It will
    // skip until the frame below the supplied stack crawl mark.
    struct SkipStruct {
        StackCrawlMark *stackMark;
        MethodDesc*     pMeth;
    };
    static StackWalkAction SkipMethods(CrawlFrame*, VOID*);

    static FCDECL1(int,     GetGenerationWR, LPVOID handle);
    static FCDECL1(int,     GetGeneration, Object* objUNSAFE);
    static FCDECL0(INT64,   GetTotalMemory);
    static FCDECL1(void,    CollectGeneration, INT32 generation);
    static FCDECL0(int,     GetMaxGeneration); 
    static FCDECL0(void,    RunFinalizers);
    static FCDECL1(VOID,    KeepAlive, Object *obj);
    static FCDECL1(LPVOID,  InternalGetCurrentMethod, StackCrawlMark* stackMark);
    static FCDECL1(int,     FCSuppressFinalize, Object *obj);
    static FCDECL1(int,     FCReRegisterForFinalize, Object *obj);
    static FCDECL0(void,    NativeSetCleanupCache);
};

class COMInterlocked
{
public:
        static FCDECL1(UINT32, Increment32, UINT32 *location);
        static FCDECL1(UINT32, Decrement32, UINT32 *location);
        static FCDECL1(UINT64, Increment64, UINT64 *location);
        static FCDECL1(UINT64, Decrement64, UINT64 *location);
        static FCDECL2(UINT32, Exchange, UINT32 *location, UINT32 value);
        static FCDECL3(UINT32, CompareExchange,        UINT32* location, UINT32 value, UINT32 comparand);
        static FCDECL3(LPVOID, CompareExchangePointer, LPVOID* location, LPVOID value, LPVOID comparand);
        static FCDECL2(R4, ExchangeFloat, R4 *location, R4 value);
        static FCDECL3_IVV(R4, CompareExchangeFloat, R4 *location, R4 value, R4 comparand);
        static FCDECL2(LPVOID, ExchangeObject, LPVOID* location, LPVOID value);
        static FCDECL3(LPVOID, CompareExchangeObject, LPVOID* location, LPVOID value, LPVOID comparand);
};

class ManagedLoggingHelper {

public:
    static FCDECL5(INT32, GetRegistryLoggingValues, BYTE *bLoggingEnabled, BYTE *bLogToConsole, INT32 *bLogLevel, BYTE *bPerfWarnings, BYTE *bCorrectnessWarnings);
};


class ValueTypeHelper {
public:
    static FCDECL1(LPVOID, GetMethodTablePtr, Object* obj);
    static FCDECL1(BOOL, CanCompareBits, Object* obj);
    static FCDECL2(BOOL, FastEqualsCheck, Object* obj1, Object* obj2);
};

#endif // _COMUTILNATIVE_H_
