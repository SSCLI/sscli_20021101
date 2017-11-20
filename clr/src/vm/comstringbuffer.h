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
** Header:  COMStringBuffer.h
**       
**
** Purpose: Contains types and method signatures for the 
** StringBuffer class.
**
** Date:  March 12, 1998
** 
===========================================================*/

//
// Each function that we call through native only gets one argument,
// which is actually a pointer to it's stack of arguments.  Our structs
// for accessing these are defined below.
//


//
// The type signatures and methods for String Buffer
//

#ifndef _STRINGBUFFER_H
#define _STRINGBUFFER_H

#define CAPACITY_LOW  10000
#define CAPACITY_MID  15000
#define CAPACITY_HIGH 20000
#define CAPACITY_FIXEDINC 5000
#define CAPACITY_PERCENTINC 1.25



/*======================RefInterpretGetStringBufferValues=======================
**Intprets a StringBuffer.  Returns a pointer to the character array and the length
**of the string in the buffer.
**
**Args: (IN)ref -- the StringBuffer to be interpretted.
**      (OUT)chars -- a pointer to the characters in the buffer.
**      (OUT)length -- a pointer to the length of the buffer.
**Returns: void
**Exceptions: None.
==============================================================================*/
inline void RefInterpretGetStringBufferValues(STRINGBUFFERREF ref, WCHAR **chars, int *length) {
    *length = (ref)->GetStringRef()->GetStringLength();
    *chars  = (ref)->GetStringRef()->GetBuffer();
}
        
  



class COMStringBuffer {

private:


public:
    //typedef struct {
    //    DECLARE_ECALL_OBJECTREF_ARG(STRINGBUFFERREF, thisRef); 
    //    DECLARE_ECALL_I4_ARG(INT32, capacity);
    //} _setCapacityArgs;
    //typedef struct {
    //    DECLARE_ECALL_OBJECTREF_ARG(STRINGBUFFERREF, thisRef); 
    //    DECLARE_ECALL_I4_ARG(INT32, count); 
    //    DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, value);
    //    DECLARE_ECALL_I4_ARG(INT32, index); 
    //} _insertStringArgs;
    //typedef struct {
    //    DECLARE_ECALL_OBJECTREF_ARG(STRINGBUFFERREF, thisRef); 
    //    DECLARE_ECALL_I4_ARG(INT32, charCount); 
    //    DECLARE_ECALL_I4_ARG(INT32, startIndex); 
    //    DECLARE_ECALL_OBJECTREF_ARG(CHARARRAYREF, value);
    //    DECLARE_ECALL_I4_ARG(INT32, index); 
    //} _insertCharArrayArgs;
    //typedef struct {
    //    DECLARE_ECALL_OBJECTREF_ARG(STRINGBUFFERREF, thisRef); 
    //    DECLARE_ECALL_I4_ARG(INT32, length); 
    //    DECLARE_ECALL_I4_ARG(INT32, startIndex);
    //} _removeArgs;
    //typedef struct {
    //    DECLARE_ECALL_OBJECTREF_ARG(STRINGBUFFERREF, thisRef);
    //    DECLARE_ECALL_I4_ARG(INT32, count); 
    //    DECLARE_ECALL_I4_ARG(INT32, startIndex);
    //    DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, newValue); 
    //    DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, oldValue); 
    //} _replaceStringArgs;

    static MethodTable* s_pStringBufferClass;

    //
    // NATIVE HELPER METHODS
    //
    static FCDECL0(void*, GetCurrentThread);
    static STRINGREF GetThreadSafeString(STRINGBUFFERREF thisRef,void **currentThread);
    static INT32  NativeGetCapacity(STRINGBUFFERREF thisRef);
    static BOOL   NeedsAllocation(STRINGBUFFERREF thisRef, INT32 requiredSize);
    static void   ReplaceStringRef(STRINGBUFFERREF thisRef, void *currentThread,STRINGREF value);
	// Note the String can change if multiple threads hit a StringBuilder, hence we don't get the String from the StringBuffer to make it threadsafe against GC corruption
    static STRINGREF GetRequiredString(STRINGBUFFERREF *thisRef, STRINGREF thisString, int requiredCapacity);
    static INT32  NativeGetLength(STRINGBUFFERREF thisRef);
    static WCHAR* NativeGetBuffer(STRINGBUFFERREF thisRef);
    static void ReplaceBuffer(STRINGBUFFERREF *thisRef, WCHAR *newBuffer, INT32 newLength);
    static void ReplaceBufferAnsi(STRINGBUFFERREF *thisRef, CHAR *newBuffer, INT32 newCapacity);    
    static INT32 CalculateCapacity(STRINGBUFFERREF thisRef, int, int);
    static STRINGREF CopyString(STRINGBUFFERREF *thisRef, STRINGREF thisString, int newCapacity);
    static INT32 LocalIndexOfString(WCHAR *base, WCHAR *search, int strLength, int patternLength, int startPos);

    static STRINGBUFFERREF NewStringBuffer(INT32 size);
    
    //
    // CLASS INITIALIZERS
    //
    static HRESULT __stdcall LoadStringBuffer();

    //
    //CONSTRUCTORS
    //
    static FCDECL5(void, MakeFromString, StringBufferObject* thisRef, StringObject* value, INT32 startIndex, INT32 length, INT32 capacity);

    //
    // BUFFER STATE QUERIES AND MODIFIERS
    //
    static FCDECL2(Object*, SetCapacity, StringBufferObject* thisRefUNSAFE, INT32 capacity);
    
    //
    // SEARCHES
    //


    //
    // MODIFIERS
    //
    static FCDECL4(Object*, InsertString, StringBufferObject* thisRefUNSAFE, INT32 index, StringObject* valueUNSAFE, INT32 count);
    static FCDECL5(Object*, InsertCharArray, StringBufferObject* thisRefUNSAFE, INT32 index, CHARArray* valueUNSAFE, INT32 startIndex, INT32 charCount);
    static FCDECL3(Object*, Remove, StringBufferObject* thisRefUNSAFE, INT32 startIndex, INT32 length);
    static FCDECL5(Object*, ReplaceString, StringBufferObject* thisRefUNSAFE, StringObject* oldValueUNSAFE, StringObject* newValueUNSAFE, INT32 startIndex, INT32 count);
};

/*=================================GetCapacity==================================
**This function is designed to mask the fact that we have a null terminator on 
**the end of the strings and to provide external visibility of the capacity from
**native.
**
**Args: thisRef:  The stringbuffer for which to return the capacity.
**Returns:  The capacity of the StringBuffer.
**Exceptions: None.
==============================================================================*/
inline INT32 COMStringBuffer::NativeGetCapacity(STRINGBUFFERREF thisRef) {
    _ASSERTE(thisRef);
    return (thisRef->GetArrayLength()-1);
}


/*===============================NativeGetLength================================
**
==============================================================================*/
inline INT32 COMStringBuffer::NativeGetLength(STRINGBUFFERREF thisRef) {
    _ASSERTE(thisRef);
    return thisRef->GetStringRef()->GetStringLength();
}


/*===============================NativeGetBuffer================================
**
==============================================================================*/
inline WCHAR* COMStringBuffer::NativeGetBuffer(STRINGBUFFERREF thisRef) {
    _ASSERTE(thisRef);
    return thisRef->GetStringRef()->GetBuffer();
}

inline BOOL COMStringBuffer::NeedsAllocation(STRINGBUFFERREF thisRef, INT32 requiredSize) {
    INT32 currCapacity = NativeGetCapacity(thisRef);
    //Don't need <=.  NativeGetCapacity accounts for the terminating null.
    return currCapacity<requiredSize;}

inline void COMStringBuffer::ReplaceStringRef(STRINGBUFFERREF thisRef, void* currentThead,STRINGREF value) {
    thisRef->SetCurrentThread(currentThead);
    thisRef->SetStringRef(value);
}

#endif // _STRINGBUFFER_H








