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
/*============================================================
**
** Header: COMNativeOverlapped.h
**
**                                                  
**
** Purpose: Native methods for allocating and freeing NativeOverlapped
**
** Date:  January, 2000
** 
===========================================================*/
#include "common.h"
#include "fcall.h"
#include "nativeoverlapped.h"
	
#define structsize sizeof(NATIVE_OVERLAPPED)

FCIMPL0(BYTE*, AllocNativeOverlapped)
	BYTE* pOverlapped = new BYTE[structsize];
	LOG((LF_SLOP, LL_INFO10000, "In AllocNativeOperlapped thread 0x%x overlap 0x%x\n", GetThread(), pOverlapped));
	return (pOverlapped);
FCIMPLEND




FCIMPL1(void, FreeNativeOverlapped, BYTE* pOverlapped)
	LOG((LF_SLOP, LL_INFO10000, "In FreeNativeOperlapped thread 0x%x overlap 0x%x\n", GetThread(), pOverlapped));
	//_ASSERTE(pOverlapped);
	delete []  pOverlapped;
FCIMPLEND
