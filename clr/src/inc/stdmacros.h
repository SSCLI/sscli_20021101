//
// 
//  Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
// 
//  The use and distribution terms for this software are contained in the file
//  named license.txt, which can be found in the root of this distribution.
//  By using this software in any fashion, you are agreeing to be bound by the
//  terms of this license.
// 
//  You must not remove this notice, or any other, from this software.
// 
//
// common.h - precompiled headers include for the COM+ Execution Engine
//

//
// Make sure _ASSERTE is defined before including this header file
// Other than that, please keep this header self-contained so that it can be included in
//  all dlls
//

#ifndef _stdmacros_h_
#define _stdmacros_h_

#ifndef _ASSERTE
#error Please define _ASSERTE before including StdMacros.h
#endif


/********************************************/
/*         Portability macros               */
/********************************************/

    #define LOG2_PTRSIZE       2
    typedef unsigned short     HALF_SIZE_T;
    #define INVALID_POINTER_CC 0xcccccccc
    #define INVALID_POINTER_CD 0xcdcdcdcd
    #define FMT_ADDR           " %08x "
    #define LFMT_ADDR          L" %08x "
    #define DBG_ADDR(ptr)      (ptr)

#if defined(_X86_) || defined(_PPC_)
    #define PAGE_SIZE          0x1000
    #define CPU_HAS_FP_SUPPORT 1
#else
    #error Please add a new #elif clause and define all portability macros for the new platform
#endif      // platform-specific


#define OS_PAGE_SIZE PAGE_SIZE

#ifndef ALLOC_ALIGN_CONSTANT
#define ALLOC_ALIGN_CONSTANT ((1<<LOG2_PTRSIZE)-1)
#endif


inline void *GetTopMemoryAddress(void)
{
    static void *result = NULL;
    if( NULL == result )
    {
        SYSTEM_INFO sysInfo;
        GetSystemInfo( &sysInfo );
        result = sysInfo.lpMaximumApplicationAddress;
    }
    return result;
}
inline void *GetBotMemoryAddress(void)
{
    static void *result = NULL;
    if( NULL == result )
    {
        SYSTEM_INFO sysInfo;
        GetSystemInfo( &sysInfo );
        result = sysInfo.lpMinimumApplicationAddress;
    }
    return result;
}

#define TOP_MEMORY (GetTopMemoryAddress())
#define BOT_MEMORY (GetBotMemoryAddress())


//
// This macro returns val rounded up as necessary to be a multiple of alignment; alignment must be a power of 2
//
inline size_t ALIGN_UP( size_t val, size_t alignment )
{
	// alignment must be a power of 2 for this implementation to work (need modulo otherwise)
	_ASSERTE( 0 == (alignment & (alignment - 1)) ); 
	size_t result = (val + (alignment - 1)) & ~(alignment - 1);
	_ASSERTE( result >= val );		// check for overflow
	return result;
}
inline void* ALIGN_UP( void* val, size_t alignment )
{
	return (void*) ALIGN_UP( (size_t)val, alignment );
}

inline BOOL IS_ALIGNED( size_t val, size_t alignment )
{
	// alignment must be a power of 2 for this implementation to work (need modulo otherwise)
	_ASSERTE( 0 == (alignment & (alignment - 1)) ); 
	return 0 == (val & (alignment - 1));
}	
inline BOOL IS_ALIGNED( void* val, size_t alignment )
{
	return IS_ALIGNED( (size_t) val, alignment );
}

//
// define some useful macros for logging object
//

#define FMT_OBJECT  "object" FMT_ADDR
#define FMT_HANDLE  "handle" FMT_ADDR
#define FMT_CLASS   "%s"
#define FMT_REG     "r%d "
#define FMT_STK     "sp%s0x%02x "
#define FMT_PIPTR   "%s%s pointer "


#define DBG_GET_CLASS_NAME(pMT)        \
        (pMT)->GetClass()->GetDebugClassName()

#define DBG_CLASS_NAME_MT(pMT)         \
        (DBG_GET_CLASS_NAME(pMT) == NULL) ? "<null-class>" : DBG_GET_CLASS_NAME(pMT) 

#define DBG_GET_MT_FROM_OBJ(obj)       \
        (MethodTable*)((size_t)((Object*) (obj))->GetMethodTable() & ~0x3) 
            // ~0x3 is to make the macro work during GC time

#define DBG_CLASS_NAME_OBJ(obj)        \
        ((obj) == NULL)  ? "null" : DBG_CLASS_NAME_MT(DBG_GET_MT_FROM_OBJ(obj)) 

#define DBG_CLASS_NAME_IPTR2(obj,iptr) \
        ((iptr) != 0)    ? ""     : DBG_CLASS_NAME_MT(DBG_GET_MT_FROM_OBJ(obj))

#define DBG_CLASS_NAME_IPTR(obj,iptr)  \
        ((obj)  == NULL) ? "null" : DBG_CLASS_NAME_IPTR2(obj,iptr)

#define DBG_STK(off)                   \
        (off >= 0) ? "+" : "-",        \
        (off >= 0) ? off : -off

#define DBG_PIN_NAME(pin)              \
        (pin)  ? "pinned "  : ""

#define DBG_IPTR_NAME(iptr)            \
        (iptr) ? "interior" : "base"


#define LOG_HANDLE_OBJECT_CLASS(str1, hnd, str2, obj)    \
        str1 FMT_HANDLE str2 FMT_OBJECT FMT_CLASS "\n",  \
        DBG_ADDR(hnd), DBG_ADDR(obj), DBG_CLASS_NAME_OBJ(obj)

#define LOG_OBJECT_CLASS(obj)                            \
        FMT_OBJECT FMT_CLASS "\n",                       \
        DBG_ADDR(obj), DBG_CLASS_NAME_OBJ(obj)

#define LOG_PIPTR_OBJECT_CLASS(obj, pin, iptr)           \
        FMT_PIPTR FMT_ADDR FMT_CLASS "\n",               \
        DBG_PIN_NAME(pin), DBG_IPTR_NAME(iptr),          \
        DBG_ADDR(obj), DBG_CLASS_NAME_IPTR(obj,iptr)

#endif //_stdmacros_h_
