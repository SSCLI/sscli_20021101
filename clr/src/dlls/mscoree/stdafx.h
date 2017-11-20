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
// Precompiled headers.
//
//*****************************************************************************
#ifndef __STDAFX_H__
#define __STDAFX_H__

#include <crtwrap.h>
#include <winwrap.h>                    // Windows wrappers.

#include <ole2.h>						// OLE definitions

#if(_MSC_VER<=1300)
#define _WIN32_WINNT 0x0400
#endif


#include "intrinsic.h"					// Functions to make intrinsic.


// Helper function returns the instance handle of this module.
HINSTANCE GetModuleInst();

#endif  // __STDAFX_H__
