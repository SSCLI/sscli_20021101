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
** Purpose: Native methods for allocating and freeing NativeOverlapped
**
** Date:  January, 2000
** 
===========================================================*/

#ifndef _OVERLAPPED_H
#define _OVERLAPPED_H

// IMPORTANT: This struct is mirrored in Overlapped.cs. If it changes in either 
// place the other file must be modified as well

typedef struct  { 
    DWORD  Internal; 
    DWORD  InternalHigh; 
    DWORD  Offset; 
    DWORD  OffsetHigh; 
    HANDLE hEvent; 
	void*  CORReserved1;
	void*  CORReserved2;
	void*  CORReserved3;
	void*  ClasslibReserved;
} NATIVE_OVERLAPPED; 

FCDECL0(BYTE*, AllocNativeOverlapped);

FCDECL1(void, FreeNativeOverlapped, BYTE* pOverlapped);

#endif
