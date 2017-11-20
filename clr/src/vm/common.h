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
//
// common.h - precompiled headers include for the COM+ Execution Engine
//

#ifndef _common_h_
#define _common_h_

#ifdef _MSC_VER
    // These don't seem useful, so turning them off is no big deal
#pragma warning(disable:4201)   // nameless struct/union
#pragma warning(disable:4510)   // can't generate default constructor
//#pragma warning(disable:4511)   // can't generate copy constructor
#pragma warning(disable:4512)   // can't generate assignment constructor
#pragma warning(disable:4610)   // user defined constructor required 
#pragma warning(disable:4211)   // nonstandard extention used (char name[0] in structs)
#pragma warning(disable:4268)   // 'const' static/global data initialized with compiler generated default constructor fills the object with zeros
#pragma warning(disable:4238)   // nonstandard extension used : class rvalue used as lvalue
#pragma warning(disable:4291)   // no matching operator delete found
#pragma warning(disable :4786)	 // identifier was truncated to '255' characters in the browser (or debug) information

    // Depending on the code base, you may want to not disable these
#pragma warning(disable:4245)   // assigning signed / unsigned
//#pragma warning(disable:4146)   // unary minus applied to unsigned
#pragma warning(disable:4244)   // loss of data int -> char ..
#pragma warning(disable:4127)   // conditional expression is constant
#pragma warning(disable:4100)   // unreferenced formal parameter

#ifndef DEBUG
#pragma warning(disable:4189)   // local variable initialized but not used
#pragma warning(disable:4505)   // unreferenced local function has been removed
//#pragma warning(disable:4702)   // unreachable code
#pragma warning(disable:4313)   // 'format specifier' in format string conflicts with argument %d of type 'type'
#endif

    // CONSIDER put these back in
#pragma warning(disable:4063)   // bad switch value for enum (only in Disasm.cpp)
#pragma warning(disable:4710)   // function not inlined
#pragma warning(disable:4527)   // user-defined destructor required
#pragma warning(disable:4513)   // destructor could not be generated

//#pragma warning(disable:4701)   // local variable may be used without being initialized 
#endif  // defined(_MSC_VER)

#define _CRT_DEPENDENCY_   //this code depends on the crt file functions



#include <winwrap.h>

#include <windows.h>
#include <stdlib.h>
#include <wchar.h>
#include <objbase.h>
#include <stddef.h>
#include <float.h>
#include <math.h>
#include <time.h>
#include <limits.h>


#ifdef _MSC_VER
//non inline intrinsics are faster
#pragma function(memcpy,memcmp,memset,strcmp,strcpy,strlen,strcat)
#endif

#include "stdmacros.h"

//
// This macro is here to CATCH bad casts at runtime while bringing
// new portions of the code online on IA64. 
//
inline size_t UnsafePointerTruncate(void *expr)  { return (size_t) expr; }


extern bool isMemoryReadable(const void* start, unsigned len);


inline VOID EE_EnterCriticalSection(LPCRITICAL_SECTION);
#define EnterCriticalSection EE_EnterCriticalSection

inline VOID EE_LeaveCriticalSection(LPCRITICAL_SECTION);
#define LeaveCriticalSection EE_LeaveCriticalSection


#define POISONC ((sizeof(int *) == 4)?0xCCCCCCCCL:I64(0xCCCCCCCCCCCCCCCC))

#include "switches.h"
#include "classnames.h"
#include "dbgalloc.h"
#include "util.hpp"
#include "new.hpp"
#include "corpriv.h"
//#include "WarningControl.h"

#ifndef memcpyGCRefs_f
#define memcpyGCRefs_f
class Object;
void SetCardsAfterBulkCopy( Object**, size_t );
// use this when you want to memcpy something that contains GC refs
inline void *  memcpyGCRefs(void *dest, const void *src, size_t len) 
{
    void *ret = memcpy(dest, src, len);
    SetCardsAfterBulkCopy((Object**) dest, len);
    return ret;
}
#endif

//
// By default logging, and debug GC are enabled under debug
//
// These can be enabled in non-debug by removing the #ifdef _DEBUG
// allowing one to log/check_gc a free build.
//
#ifdef _DEBUG
    #define LOGGING

        // You should be using CopyValueClass if you are doing an memcpy
        // in the CG heap.  
    #if defined(COMPLUS_EE) && !defined(memcpy)
    inline void* memcpyNoGCRefs(void * dest, const void * src, size_t len) {
            return(memcpy(dest, src, len));
        }
    extern "C" void *  __cdecl GCSafeMemCpy(void *, const void *, size_t);
    #define memcpy(dest, src, len) GCSafeMemCpy(dest, src, len)
    #endif

    #if !defined(CHECK_APP_DOMAIN_LEAKS)
    #define CHECK_APP_DOMAIN_LEAKS 1
    #endif
#else
    #define memcpyNoGCRefs memcpy
#endif

#include "log.h"
#include "vars.hpp"
#include "crst.h"
#include "argslot.h"
#include "stublink.h"
#include "cgensys.h"
#include "ceemain.h"
#include "hash.h"
#include "ceeload.h"
#include "handletable.h"
#include "objecthandle.h"
#include "codeman.h"
#include "class.h"
#include "appdomain.hpp"
#include "assembly.hpp"
#include "clsload.hpp"
#include "eehash.h"
#include "gcdesc.h"
#include "list.h"
#include "syncblk.h"
#include "object.h"
#include "method.hpp"
#include "regdisp.h"
#include "frames.h"
#include "stackwalk.h"
#include "threads.h"
#include "stackingallocator.h"
#include "interoputil.h"
#include "excep.h"
#include "wrappers.h"


#undef EnterCriticalSection
inline VOID EE_EnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
    EnterCriticalSection(lpCriticalSection);
    INCTHREADLOCKCOUNT();
}
#define EnterCriticalSection EE_EnterCriticalSection

#undef LeaveCriticalSection
inline VOID EE_LeaveCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
    LeaveCriticalSection(lpCriticalSection);
    DECTHREADLOCKCOUNT();
}
#define LeaveCriticalSection EE_LeaveCriticalSection

#endif
