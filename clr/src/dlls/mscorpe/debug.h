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

// Ultra-simple debug print utility.  set dbFlags to non-zero to turn on debug printfs 
// Note you use dbPrintf like this.  Note the extra parentheses.  This allows variable number of args
//		dbPrintf(("val = %d\n", val));

#ifndef DEBUG
#define dbPrintf(x)	{ }
// #define dbXXXPrintf(x)	{ }						// sample of adding another debug stream
#else
#include <stdio.h>

extern unsigned dbFlags;

#define DB_ALL 			0xFFFFFFFF

#define DB_GENERIC 		0x00000001
// #define DB_XXX 			0x00000002				// sample of adding another debug stream

#define dbPrintf(x)	{ if (dbFlags & DB_GENERIC) printf x; }

// #define dbXXXPrintf(x)	{ if (dbFlags & DB_XXX) printf x; }// sample of adding another debug stream

#endif
