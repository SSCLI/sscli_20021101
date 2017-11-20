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
//*****************************************************************************
// File: CompressionFormat.h
//
//*****************************************************************************

// Describes the on-disk compression format for encoding/decoding tables for
// compressed opcodes

#ifndef _COMPRESSIONFORMAT_H
#define _COMPRESSIONFORMAT_H

#include <pshpack1.h>

typedef struct
{
    // Number of macros defined in the table
    // Macro opcodes start at 1
    DWORD  dwNumMacros;

    // Cumulative number of instructions from all macros - used to help the
    // decoder determine decoding table size
    DWORD  dwNumMacroComponents;
} CompressionMacroHeader;

#include <poppack.h>

#endif /* _COMPRESSIONFORMAT_H */
