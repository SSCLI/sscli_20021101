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
// File: name.h
//
// ===========================================================================

#ifndef _NAME_H_
#define _NAME_H_

#if defined(_MSC_VER)
#pragma warning(disable:4200)
#endif  // defined(_MSC_VER)

/*
 * A NAME structure stores a single name in the name table.
 * It is allocated out of a no-release allocator.
 */

struct NAME {
    unsigned hash;         // hash value
    NAME * nextInBucket;   // next NAME in this hash bucket
    const wchar_t text[];  // text of the name
};

typedef NAME * PNAME;       // pointer to name.

#endif  // _NAME_H_
