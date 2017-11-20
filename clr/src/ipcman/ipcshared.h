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
// File: IPCShared.h
//
// Shared private utility functions for COM+ IPC operations
//
//*****************************************************************************

#ifndef _IPCSHARED_H_
#define _IPCSHARED_H_

class IPCShared
{
public:
// Close a handle and pointer to any memory mapped file
	static void CloseGenericIPCBlock(HANDLE & hMemFile, void * & pBlock);

// Based on the pid, write a unique name for a memory mapped file
	static void GenerateName(DWORD pid, WCHAR* pszBuffer, int len);

    static HRESULT CreateWinNTDescriptor(DWORD pid, SECURITY_ATTRIBUTES **ppSA);

};

#endif
