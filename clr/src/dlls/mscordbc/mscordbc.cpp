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
// MSCorDBC.cpp
//
// COM+ Debugging Services -- Runtime Controller DLL
//
// Dll* routines for entry points.
//
//*****************************************************************************
#include "stdafx.h"

//*****************************************************************************
// The main dll entry point for this module.  This routine is called by the
// OS when the dll gets loaded.  Nothing needs to be done for this DLL.
//*****************************************************************************
extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{

	OnUnicodeSystem();

    return TRUE;
}


