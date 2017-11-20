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
** Header: COMMutex.h
**       
**
** Purpose: Native methods on System.Threading.Mutex
**
** Date:  February, 2000
** 
===========================================================*/

#ifndef _COMMUTEX_H
#define _COMMUTEX_H

#include "fcall.h"
#include "comwaithandle.h"

class MutexNative :public WaitHandleNative
{

public:
    static FCDECL3(HANDLE, CorCreateMutex, BYTE initialOwnershipRequested, StringObject* pName, BYTE* gotOwnership);
    static FCDECL1(void, CorReleaseMutex, HANDLE handle);
};

#endif
