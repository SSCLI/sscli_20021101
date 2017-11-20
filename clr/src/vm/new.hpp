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
#ifndef _NEW_HPP_
#define _NEW_HPP_

#include "exceptmacros.h"

struct NoThrow { int x; };
extern const NoThrow nothrow;

struct Throws { int x; };
extern const Throws throws;

static inline void * __cdecl operator new(size_t n, const NoThrow&) { 
    return ::operator new(n); 
}
static inline void * __cdecl operator new[](size_t n, const NoThrow&) { 
    return ::operator new[](n);  
}
static inline void __cdecl operator delete(void *p, const NoThrow&) { 
    ::operator delete(p);
}
static inline void __cdecl operator delete[](void *p, const NoThrow&) { 
    ::operator delete[](p);
}

static inline void * __cdecl operator new(size_t n, const Throws&) { 
    THROWSCOMPLUSEXCEPTION();
    void *p = ::operator new(n); 
    if (!p) COMPlusThrowOM();
    return p;
}
static inline void * __cdecl operator new[](size_t n, const Throws&) { 
    THROWSCOMPLUSEXCEPTION();
    void *p = ::operator new[](n);  
    if (!p) COMPlusThrowOM();
    return p;
}
static inline void __cdecl operator delete(void *p, const Throws&) { 
    ::operator delete(p);
}
static inline void __cdecl operator delete[](void *p, const Throws&) { 
    ::operator delete[](p);
}

#endif // _NEW_HPP_
