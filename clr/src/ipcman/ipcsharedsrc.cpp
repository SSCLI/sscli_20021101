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
// File: IPCSharedSrc.cpp
//
// Shared source for COM+ IPC Reader & Writer classes
//
//*****************************************************************************

#include "stdafx.h"
#include "ipcshared.h"

// Name of the Private (per-process) block. %d resolved to a PID
#define CorPrivateIPCBlock      L"Global\\Rotor_Private_IPCBlock_%d"

//-----------------------------------------------------------------------------
// Close a handle and pointer to any memory mapped file
//-----------------------------------------------------------------------------
void IPCShared::CloseGenericIPCBlock(HANDLE & hMemFile, void * & pBlock)
{
	if (pBlock != NULL) {
		UnmapViewOfFile(pBlock);
		pBlock = NULL;
	}

	if (hMemFile != NULL) {
		CloseHandle(hMemFile);
		hMemFile = NULL;
	}
}

//-----------------------------------------------------------------------------
// Based on the pid, write a unique name for a memory mapped file
//-----------------------------------------------------------------------------
void IPCShared::GenerateName(DWORD pid, WCHAR* pszBuffer, int len)
{
    // Must be large enough to hold our string
    _ASSERTE(len >= (int) (sizeof(CorPrivateIPCBlock) / sizeof(WCHAR)) + 10);
    LPCWSTR lpName = CorPrivateIPCBlock;


    swprintf(pszBuffer, lpName, pid);
}

//-----------------------------------------------------------------------------
// Setup a security descriptor for the named kernel objects if we're on NT.
//-----------------------------------------------------------------------------
HRESULT IPCShared::CreateWinNTDescriptor(DWORD pid, SECURITY_ATTRIBUTES **ppSA)
{
    HRESULT hr = NO_ERROR;

    // Gotta have a place to stick the new SA...
    if (ppSA == NULL)
    {
        _ASSERTE(!"Caller must supply ppSA");
        return E_INVALIDARG;
    }

    *ppSA = NULL;

    
    return hr;
}

