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
// stdafx.h
//
// Common include file for utility code.
//*****************************************************************************
#include <rotor_palrt.h>

// Note that we want right-side specific code
#define RIGHT_SIDE_ONLY

#include "corpub.h"
#include "../inc/cordb.h"

#ifdef _DEBUG
#include "dbgalloc.h"
#include "utilcode.h"
#endif

