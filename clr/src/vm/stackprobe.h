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
//-----------------------------------------------------------------------------
// Stack Probe Header
// Used to setup stack guards
//-----------------------------------------------------------------------------
#ifndef __STACKPROBE_h__
#define __STACKPROBE_h__


inline void InitStackProbes() { }
inline void TerminateStackProbes() { }

#define REQUIRES_N4K_STACK(n)
#define REQUIRES_4K_STACK
#define REQUIRES_8K_STACK
#define REQUIRES_12K_STACK
#define REQUIRES_16K_STACK

#define BEGIN_REQUIRES_4K_STACK
#define BEGIN_REQUIRES_N4K_STACK(n)
#define BEGIN_REQUIRES_8K_STACK
#define BEGIN_REQUIRES_12K_STACK
#define BEGIN_REQUIRES_16K_STACK
#define END_CHECK_STACK

#define CREATE_UNINIT_STACK_GUARD	
#define POST_REQUIRES_N4K_STACK(n)

#define SAFE_REQUIRES_N4K_STACK(n)


#endif  // __STACKPROBE_h__
