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

#include "openum.h"

typedef struct
{
    char *  pszName;
    
    OPCODE   Ref;   // reference codes
    
    BYTE    Type;   // Inline0 etc.

    BYTE    Len;    // std mapping
    BYTE    Std1;   
    BYTE    Std2;
} opcodeinfo_t;

#ifdef DECLARE_DATA
opcodeinfo_t OpcodeInfo[] =
{
#define OPDEF(c,s,pop,push,args,type,l,s1,s2,ctrl) { s,c,args,l,s1,s2 },
#include "opcode.def"
#undef OPDEF
};
#else
extern opcodeinfo_t OpcodeInfo[];
#endif


