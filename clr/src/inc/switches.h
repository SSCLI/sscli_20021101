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
// switches.h switch configuration of common runtime features
//


// 
//

#define STRESS_HEAP
#define STRESS_THREAD
#define META_ADDVER
#define STRESS_COMCALL
#define GC_SIZE
#define VERIFY_HEAP
#define DEBUG_FEATURES

#ifdef _DEBUG
#define STRESS_LOG
#endif



