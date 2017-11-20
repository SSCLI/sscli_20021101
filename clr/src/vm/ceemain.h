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
// File: CEEMAIN.H
//

// CEEMAIN.H defines the entrypoints into the Virtual Execution Engine and
// gets the load/run process going.
// ===========================================================================
#ifndef CEEMain_H
#define CEEMain_H

#include <windef.h> // for HFILE, HANDLE, HMODULE

// This is a placeholder for getting going while there are still holes in the execution method.
BOOL STDMETHODCALLTYPE ExecuteEXE(HMODULE hMod);
BOOL STDMETHODCALLTYPE ExecuteDLL(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved);

// Force shutdown of the EE
void ForceEEShutdown();

// Internal replacement for OS ::ExitProcess()
void SafeExitProcess(int exitCode);

HRESULT InitializeMiniDumpBlock();

#endif
