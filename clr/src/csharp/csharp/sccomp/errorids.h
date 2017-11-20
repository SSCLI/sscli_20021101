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
// File: errorids.h
//
// Defines the error IDs for the compiler.
// ===========================================================================

#ifndef __error_ids_h__
#define __error_ids_h__

/* Compiler error numbers.
 */
#define ERRORDEF(num, level, name, strid) name,

enum ERRORIDS {
    ERR_NONE,
    #include "errors.h"
    ERR_COUNT
};

#undef ERRORDEF


#endif  // __error_ids_h__
