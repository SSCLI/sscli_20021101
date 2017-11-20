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
#ifndef __eestructs_h__
#define __eestructs_h__

#ifdef STRIKE
#ifdef _MSC_VER
#pragma warning(disable:4200)
#endif //_MSC_VER
#include "../../vm/specialstatics.h"
#ifdef _MSC_VER
#pragma warning(default:4200)
#endif //_MSC_VER
#include "data.h"

#endif //STRIKE

#define volatile

#include "symbol.h"

#if defined(_X86_) || defined(_PPC_) || defined(_SPARC_)
#define TOKEN_IN_PREPAD 1
#endif

#include <dump-type-info.h>

#ifdef STRIKE
#define DEFINE_STD_FILL_FUNCS(klass)                                                    \
    DWORD_PTR m_vLoadAddr;                                                              \
    void Fill(DWORD_PTR &dwStartAddr);                                                  \
    static ULONG GetFieldOffset(offset_member_##klass::members field);     \
    static ULONG size();                                                                
//    virtual PWSTR GetFrameTypeName() { return L#klass; }
#else
#define DEFINE_STD_FILL_FUNCS(klass)                                                    \
    DWORD_PTR m_vLoadAddr;                                                              \
    void Fill(DWORD_PTR &dwStartAddr);
#endif    

    
#include "miniee.h"

#ifdef STRIKE
#include "strikeee.h"
#endif

#endif  // __eestructs_h__
