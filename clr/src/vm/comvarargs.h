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
////////////////////////////////////////////////////////////////////////////////
// This module contains the implementation of the native methods for the
//  varargs class(es)..
////////////////////////////////////////////////////////////////////////////////
#ifndef _COMVARARGS_H_
#define _COMVARARGS_H_


struct VARARGS
{
	VASigCookie *ArgCookie;
	SigPointer	SigPtr;
	BYTE		*ArgPtr;
	int			RemainingArgs;
};

class COMVarArgs
{
public:
	//struct _VarArgsIntArgs
	//{
	//		DECLARE_ECALL_PTR_ARG(VARARGS *, _this);
	//		DECLARE_ECALL_I4_ARG(LPVOID, cookie); 
	//};
	//struct _VarArgs2IntArgs
	//{
	//		DECLARE_ECALL_PTR_ARG(VARARGS *, _this);
	//		DECLARE_ECALL_I4_ARG(LPVOID, firstArg); 
	//		DECLARE_ECALL_I4_ARG(LPVOID, cookie); 
	//};
	//struct _VarArgsGetNextArgTypeArgs
	//{
	//		DECLARE_ECALL_PTR_ARG(VARARGS *, _this);
	//};
	//struct _VarArgsGetNextArgArgs
	//{
	//		DECLARE_ECALL_PTR_ARG(VARARGS *, _this);
	//		DECLARE_ECALL_PTR_ARG(TypedByRef *, value); 
	//};
	//struct _VarArgsGetNextArg2Args
	//{
	//		DECLARE_ECALL_PTR_ARG(VARARGS *, _this);
	//		DECLARE_ECALL_PTR_ARG(TypeHandle, typehandle);
	//		DECLARE_ECALL_PTR_ARG(TypedByRef *, value); 
	//};
	//struct _VarArgsThisArgs
	//{
	//		DECLARE_ECALL_PTR_ARG(VARARGS *, _this);
	//};

	static FCDECL3(void, Init2, VARARGS* _this, LPVOID cookie, LPVOID firstArg);
	static FCDECL2(void, Init, VARARGS* _this, LPVOID cookie);
	static FCDECL1(int, GetRemainingCount, VARARGS* _this);
	static FCDECL1(void*, GetNextArgType, VARARGS* _this);
	static FCDECL0_INST_RET_TR(TypedByRef, value, GetNextArg, VARARGS* _this);
	static FCDECL1_INST_RET_TR(TypedByRef, value, GetNextArg2, VARARGS* _this, TypeHandle typehandle);

	static void GetNextArgHelper(VARARGS *data, TypedByRef *value, BOOL fData);
	static va_list MarshalToUnmanagedVaList(const VARARGS *data);
	static void    MarshalToManagedVaList(va_list va, VARARGS *dataout);
};


struct PARAMARRAY
{
	int			Count;
	BYTE		*ParamsPtr;
	BYTE		*Params[4];
	TypeHandle	Types[4];
};


#endif // _COMVARARGS_H_
