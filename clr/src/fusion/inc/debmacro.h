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
#ifndef _FUSION_INC_DEBMACRO_H_INCLUDED_
#define _FUSION_INC_DEBMACRO_H_INCLUDED_


#if DBG
#define ASSERT(x) if (!(x)) { DebugBreak(); }
#else
#define ASSERT(x)
#endif

#endif  // _FUSION_INC_DEBMACRO_H_INCLUDED_
