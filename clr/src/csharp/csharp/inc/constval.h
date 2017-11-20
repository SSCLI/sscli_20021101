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
// File: constval.h
//
// ===========================================================================

#ifndef _CONSTVAL_H_
#define _CONSTVAL_H_

/*
 * A string constant. These are NOT nul terminated, and can contain
 * internal nul characters.
 */
struct STRCONST {
    int         length;
    WCHAR *     text;
};

/*
 * A constant value. We want this to use only 4 bytes, so larger
 * values are represented by pointers.
 */
union CONSTVAL
{
    INT_PTR         init;           // This is first so it will be used for initialization.
                                    // It is an INT_PTR to take advantage of the larger sizes of pointers.
    int             iVal;           // 4 byte or less integral values. (must be 0 or 1 for bools).
    unsigned int    uiVal;          // unsigned 4 byte int.

    // long values -- allocated and pointed to.
    double *        doubleVal;      // Use for "float" constants too.
    __int64 *       longVal;
    unsigned __int64 * ulongVal;    // unsigned 8 byte int.
    STRCONST *      strVal;
    DECIMAL *       decVal;       
};

union CONSTVALNS  // the same union w/o a string field...
{
    int             iVal;           // 4 byte or less integral values. (must be 0 or 1 for bools).
    unsigned int    uiVal;          // unsigned 4 byte int.

    // long values -- allocated and pointed to.
    double *        doubleVal;      // Use for "float" constants too.
    __int64 *       longVal;
    unsigned __int64 * ulongVal;    // unsigned 8 byte int.
    DECIMAL *       decVal;           
};

#endif  // _CONSTVAL_H_
