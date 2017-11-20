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
** Header: COMMutex.cpp
**       
**
** Purpose: Native methods on System.Threading.Mutex
**
** Date:  February, 2000
** 
===========================================================*/
#include "common.h"

#include "commutex.h"

#define FORMAT_MESSAGE_BUFFER_LENGTH 1024
#define CreateExceptionMessage(wszFinal) \
        WszFormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,					\
                                                           NULL         /*ignored msg source*/,			\
                                                           ::GetLastError(),							\
                                                           0            /*pick appropriate languageId*/,\
                                                           wszFinal,									\
                                                           FORMAT_MESSAGE_BUFFER_LENGTH-1,				\
                                                           0            /*arguments*/);

FCIMPL3(HANDLE, MutexNative::CorCreateMutex, BYTE initialOwnershipRequested, StringObject* pName, BYTE* gotOwnership)
{
	STRINGREF mutexString(pName);
    WCHAR* mutexName = L""; 
	if (mutexString != NULL)
		mutexName = mutexString->GetBuffer();

	if (gotOwnership == NULL)
    {
        FCThrow(kNullReferenceException);
    }

	HANDLE mutexHandle = WszCreateMutex(NULL,
										initialOwnershipRequested,
										mutexName);

	if (mutexHandle == NULL)
    {
		WCHAR   wszBuff[FORMAT_MESSAGE_BUFFER_LENGTH];
		WCHAR* wszFinal =wszBuff;
		CreateExceptionMessage(wszFinal)
        FCThrowEx(kApplicationException,0,wszFinal,0,0);
    }
	
    DWORD status = ::GetLastError();
    *gotOwnership = !!(status != ERROR_ALREADY_EXISTS);

    FC_GC_POLL_RET();
    return mutexHandle;
}
FCIMPLEND


FCIMPL1(void, MutexNative::CorReleaseMutex, HANDLE handle)
{
	_ASSERTE(handle);
	BOOL res = ::ReleaseMutex(handle);
	if (res == NULL)
	{
		WCHAR   wszBuff[FORMAT_MESSAGE_BUFFER_LENGTH];
		WCHAR* wszFinal = wszBuff;
		CreateExceptionMessage(wszFinal)
        FCThrowExVoid(kApplicationException,0,wszFinal,0,0);
	}
    FC_GC_POLL();
}
FCIMPLEND



