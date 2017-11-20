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
// File: palclr.h
//
// Various macros and constants that are necessary to make the CLR portable.
// ===========================================================================

#ifndef __PALCLR_H__
#define __PALCLR_H__

#include <windef.h>

//
// CPP_ASSERT() can be used within a class definition, to perform a
// compile-time assertion involving private names within the class.
//
#ifdef _MSC_VER
#define CPP_ASSERT(n, e) C_ASSERT(e)
#else
// gcc doesn't allow redefinition of the typedef within a class, though 
// it does at file scope.
#define CPP_ASSERT(n, e) typedef char __C_ASSERT__##n[(e) ? 1 : -1];
#endif


// PORTABILITY_ASSERT and PORTABILITY_WARNING macros are meant to be used to
// mark places in the code that needs attention for portability. The usual
// usage pattern is:
//
// int get_scratch_register() {
// #if defined(_X86)
//     return eax;
// #elif defined(_PPC_)
//     return r12;
// #elif defined(_AMD64_)
//     return rax;
// #else
//     PORTABILITY_ASSERT("scratch register");
//     return 0;
// #endif
// }
//
// PORTABILITY_ASSERT is meant to be used inside functions/methods. It can
// introduce compile-time and/or run-time errors.
// PORTABILITY_WARNING is meant to be used outside functions/methods. It can
// introduce compile-time errors or warnings only.
//
// People starting new ports will first define these to just cause run-time
// errors. Once they fix all the places that need attention for portability,
// they can define PORTABILITY_ASSERT and PORTABILITY_WARNING to cause
// compile-time errors to make sure that they haven't missed anything.
// 
// If it is reasonably possible all codepaths containing PORTABILITY_ASSERT
// should be compilable (e.g. functions should return NULL or something if
// they are expected to return a value).
//
// The message in these two macros should not contain any keywords like TODO
// or NYI. It should be just the brief description of the problem.

#if defined(_X86_) || defined(_PPC_)
// Finished ports - compile-time errors
#define PORTABILITY_WARNING(message)    NEED_TO_PORT_THIS_ONE(NEED_TO_PORT_THIS_ONE)
#define PORTABILITY_ASSERT(message)     NEED_TO_PORT_THIS_ONE(NEED_TO_PORT_THIS_ONE)
#else
// Ports in progress - run-time asserts only
#define PORTABILITY_WARNING(message)
#define PORTABILITY_ASSERT(message)     _ASSERTE(false && message)
#endif


// PAL Macros
// Not all compilers support fully anonymous aggregate types, so the
// PAL provides names for those types. To allow existing definitions of
// those types to continue to work, we provide macros that should be
// used to reference fields within those types.
#ifndef V_VT
#define V_VT(X)         ((X)->n1.n2.vt)
#endif

#ifndef V_UNION
#define V_UNION(X, Y)   ((X)->n1.n2.n3.Y)
#endif

#ifndef DECIMAL_SCALE
#define DECIMAL_SCALE(dec)       ((dec).u.u.scale)
#endif

#ifndef DECIMAL_SIGN
#define DECIMAL_SIGN(dec)        ((dec).u.u.sign)
#endif

#ifndef DECIMAL_SIGNSCALE
#define DECIMAL_SIGNSCALE(dec)   ((dec).u.signscale)
#endif

#ifndef DECIMAL_LO32
#define DECIMAL_LO32(dec)        ((dec).v.v.Lo32)
#endif

#ifndef DECIMAL_MID32
#define DECIMAL_MID32(dec)       ((dec).v.v.Mid32)
#endif

#ifndef DECIMAL_HI32
#define DECIMAL_HI32(dec)       ((dec).Hi32)
#endif

#ifndef IMAGE_RELOC_FIELD
#define IMAGE_RELOC_FIELD(img, f)      ((img).u.f)
#endif

#ifndef IMAGE_IMPORT_DESC_FIELD
#define IMAGE_IMPORT_DESC_FIELD(img, f)     ((img).u.f)
#endif

#ifndef IMAGE_RDE_ID
#define IMAGE_RDE_ID(img)        ((img)->u.Id)
#endif

#ifndef IMAGE_RDE_NAME
#define IMAGE_RDE_NAME(img)      ((img)->u.Name)
#endif

#ifndef IMAGE_RDE_OFFSET
#define IMAGE_RDE_OFFSET(img)    ((img)->v.OffsetToData)
#endif

#ifndef IMAGE_RDE_NAME_FIELD
#define IMAGE_RDE_NAME_FIELD(img, f)    ((img)->u.u.f)
#endif

#ifndef IMAGE_RDE_OFFSET_FIELD
#define IMAGE_RDE_OFFSET_FIELD(img, f)  ((img)->v.v.f)
#endif

#ifndef IMAGE_FE64_FIELD
#define IMAGE_FE64_FIELD(img, f)    ((img).u.f)
#endif

#ifndef IMPORT_OBJ_HEADER_FIELD
#define IMPORT_OBJ_HEADER_FIELD(obj, f)    ((obj).u.f)
#endif


// PAL Numbers
// Used to ensure cross-compiler compatibility when declaring large
// integer constants. 64-bit integer constants should be wrapped in the
// declarations listed here.
//
// Each of the #defines here is wrapped to avoid conflicts with rotor_pal.h.

#if defined(_MSC_VER)

// MSVC's way of declaring large integer constants
#ifndef I64
#define I64(x)    x ## i64
#endif

#ifndef UI64
#define UI64(x)   x ## ui64
#endif

#elif defined(__GNUC__)

// GCC's way of declaring large integer constants
#ifndef I64
#define I64(x)    x ## LL
#endif

#ifndef UI64
#define UI64(x)   x ## ULL
#endif

#else  // !(defined(_MSC_VER) || defined(__GNUC__))
#error Unsupported compiler
#endif


// PAL SEH
// Macros for portable exception handling. The Win32 SEH is emulated using
// these macros and setjmp/longjmp on Unix
//
// Usage notes:
//
// - The filter has to be a function taking two parameters:
// LONG MyFilter(PEXCEPTION_POINTERS *pExceptionInfo, PVOID pv)
//
// - It is not possible to directly use the local variables in the filter.
// All the local information that the filter has to need to know about should
// be passed through pv parameter
//  
// - Do not use goto to jump out of the PAL_TRY block, use PAL_LEAVE instead
// (jumping out of the try block is not a good idea even on Win32, because of
// it causes stack unwind)
//
// - If multiple PAL_TRY blocks in one function are needed, use PAL_EXCEPT_EX,
// PAL_FINALLY_EX with different __LabelName for each instance
//
//
// Simple examples:
//
// PAL_TRY {
//   ....
// } PAL_FINALLY {
//   ....
// }
// PAL_ENDTRY
//
//
// PAL_TRY {
//   ....
// } PAL_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
//   ....
// }
// PAL_ENDTRY
// 
//
// LONG MyFilter(PEXCEPTION_POINTERS *pExceptionInfo, PVOID pv)
// {
// ...
// }
//
// PAL_TRY {
//   ....
// } PAL_EXCEPT_FILTER(MyFilter, NULL) {
//   ....
// }
// PAL_ENDTRY
//
//
// Complex example:
//
// struct MyParams
// {
//     ...
// } params;
//
// PAL_TRY {
//   PAL_TRY {
//     ...
//   } PAL_EXCEPT_FILTER_EX(Label1, OtherFilter, &params)
//   ...
// }
// PAL_FINALLY_EX(Label2) {
// }
//

// This is intentionally enabled for !PORTABLE_SEH && FEATURE_PAL
// to let the compiler verify that the definitions of these macros in rotor_pal.h
// are identical


#ifdef BIGENDIAN

inline UINT16 VAL16(UINT16 x)
{
    return (x >> 8) | (x << 8);
}

inline UINT32 VAL32(UINT32 x)
{
    return  (x >> 24) |
            ((x >> 8) & 0x0000FF00L) |
            ((x & 0x0000FF00L) << 8) |
            (x << 24);
}

inline UINT64 VAL64(UINT64 x)   
{
    return ((UINT64)VAL32(x) << 32) | VAL32(x >> 32);
}

inline void SwapString(WCHAR *szString)
{
    unsigned i;
    for (i = 0; szString[i] != L'\0'; i++)
    {
        szString[i] = VAL16(szString[i]);
    }
}

inline void SwapStringLength(WCHAR *szString, ULONG StringLength)
{
    unsigned i;
    for (i = 0; i < StringLength; i++)
    {
        szString[i] = VAL16(szString[i]);
    }
}

inline void SwapGuid(GUID *pGuid) 
{ 
    pGuid->Data1 = VAL32(pGuid->Data1);
    pGuid->Data2 = VAL16(pGuid->Data2);
    pGuid->Data3 = VAL16(pGuid->Data3);
}

#else // !BIGENDIAN
// For little-endian machines, do nothing
#define VAL16(x) x
#define VAL32(x) x
#define VAL64(x) x
#define SwapString(x)
#define SwapStringLength(x, y)
#define SwapGuid(x)
#endif  // !BIGENDIAN


#if defined(ALIGN_ACCESS) && !defined(_MSC_VER)

// Get Unaligned values from a potentially unaligned object
inline UINT16 GET_UNALIGNED_16(const void *pObject)
{
    UINT16 temp;
    memcpy(&temp, pObject, sizeof(temp));
    return temp; 
}
inline UINT32 GET_UNALIGNED_32(const void *pObject)
{
    UINT32 temp;
    memcpy(&temp, pObject, sizeof(temp));
    return temp; 
}
inline UINT64 GET_UNALIGNED_64(const void *pObject)
{
    UINT64 temp;
    memcpy(&temp, pObject, sizeof(temp));
    return temp; 
}

// Set Value on an potentially unaligned object
inline void SET_UNALIGNED_16(void *pObject, UINT16 Value)
{
    memcpy(pObject, &Value, sizeof(UINT16));
}
inline void SET_UNALIGNED_32(void *pObject, UINT32 Value)
{
    memcpy(pObject, &Value, sizeof(UINT32));
}
inline void SET_UNALIGNED_64(void *pObject, UINT64 Value)
{
    memcpy(pObject, &Value, sizeof(UINT64));
}

#else

// Get Unaligned values from a potentially unaligned object
#define GET_UNALIGNED_16(_pObject)  (*(UINT16 UNALIGNED *)(_pObject))
#define GET_UNALIGNED_32(_pObject)  (*(UINT32 UNALIGNED *)(_pObject))
#define GET_UNALIGNED_64(_pObject)  (*(UINT64 UNALIGNED *)(_pObject))

// Set Value on an potentially unaligned object 
#define SET_UNALIGNED_16(_pObject, _Value)  (*(UNALIGNED UINT16 *)(_pObject)) = (UINT16)(_Value)
#define SET_UNALIGNED_32(_pObject, _Value)  (*(UNALIGNED UINT32 *)(_pObject)) = (UINT32)(_Value)
#define SET_UNALIGNED_64(_pObject, _Value)  (*(UNALIGNED UINT64 *)(_pObject)) = (UINT64)(_Value) 

#endif

// Get Unaligned values from a potentially unaligned object and swap the value
#define GET_UNALIGNED_VAL16(_pObject) VAL16(GET_UNALIGNED_16(_pObject))
#define GET_UNALIGNED_VAL32(_pObject) VAL32(GET_UNALIGNED_32(_pObject))
#define GET_UNALIGNED_VAL64(_pObject) VAL64(GET_UNALIGNED_64(_pObject))

// Set a swap Value on an potentially unaligned object 
#define SET_UNALIGNED_VAL16(_pObject, _Value) SET_UNALIGNED_16(_pObject, VAL16((UINT16)_Value))
#define SET_UNALIGNED_VAL32(_pObject, _Value) SET_UNALIGNED_32(_pObject, VAL32((UINT32)_Value))
#define SET_UNALIGNED_VAL64(_pObject, _Value) SET_UNALIGNED_64(_pObject, VAL64((UINT64)_Value))

#ifdef PLATFORM_UNIX
#ifdef __APPLE__
#define MAKEDLLNAME_W(name) L"lib" name L".dylib"
#define MAKEDLLNAME_A(name)  "lib" name  ".dylib"
#else
#define MAKEDLLNAME_W(name) L"lib" name L".so"
#define MAKEDLLNAME_A(name)  "lib" name  ".so"
#endif
#else
#define MAKEDLLNAME_W(name) name L".dll"
#define MAKEDLLNAME_A(name) name  ".dll"
#endif

#ifdef UNICODE
#define MAKEDLLNAME(x) MAKEDLLNAME_W(x)
#else
#define MAKEDLLNAME(x) MAKEDLLNAME_A(x)
#endif


#endif	// __PALCLR_H__
