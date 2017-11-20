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
#ifndef _HREX_H
#define _HREX_H

/* --------------------------------------------------------------------------- *
 * HR <-> SEH exception functions.
 * --------------------------------------------------------------------------- */

#ifndef IfFailThrow
#define IfFailThrow(hr) \
	do { HRESULT _hr = hr; if (FAILED(_hr)) ThrowHR(_hr); } while (0)
#endif

#define HR_EXCEPTION_CODE (0xe0000000 | 0x00204852) // ' HR'

inline void ThrowHR(HRESULT hr)
{
	ULONG_PTR parameter = hr;
	RaiseException(HR_EXCEPTION_CODE, EXCEPTION_NONCONTINUABLE, 1, &parameter);
}

inline void ThrowError(DWORD error)
{
	ThrowHR(HRESULT_FROM_WIN32(error));
}

inline void ThrowLastError()
{
	ThrowError(GetLastError());
}

inline int IsHRException(EXCEPTION_RECORD *record)
{
	return record->ExceptionCode == HR_EXCEPTION_CODE;
}

inline HRESULT GetHRException(EXCEPTION_RECORD *record)
{
	return (HRESULT) record->ExceptionInformation[0];
}

#endif
