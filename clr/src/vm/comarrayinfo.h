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
// COMArrayInfo
//	This file defines the native methods for the ArrayInfo class
//	found in reflection.  ArrayInfo allows for late bound access
//	to COM+ Arrays.
//
// Date: August 1998
////////////////////////////////////////////////////////////////////////////////

#ifndef __COMARRAYINFO_H__
#define __COMARRAYINFO_H__

#include "fcall.h"

class COMArrayInfo
{
private:
	// CreateObject
	// Given an array and offset, we will either 1) return the object or create a boxed version
	//	(This object is returned as a LPVOID so it can be directly returned.)
	static BOOL CreateObject(BASEARRAYREF* arrObj,DWORD dwOffset,TypeHandle elementType,ArrayClass* pArray, Object*& newObject);

	// SetFromObject
	// Given an array and offset, we will set the object or value.
	static void SetFromObject(BASEARRAYREF* arrObj,DWORD dwOffset,TypeHandle elementType,ArrayClass* pArray,OBJECTREF* pObj);

public:
	// This method will create a new array of type type, with zero lower
	//	bounds and rank.
    static FCDECL5(Object*, CreateInstance,   ReflectClassBaseObject* type, INT32 rank, INT32 length1, INT32 length2, INT32 length3);
	static FCDECL3(Object*, CreateInstanceEx, ReflectClassBaseObject* typeUNSAFE, I4Array* lengthsUNSAFE, I4Array* lowerBoundsUNSAFE);

	// GetValue
	// This method will return a value found in an array as an Object
    static FCDECL4(Object*, GetValue,   ArrayBase* _refThis, INT32 index1, INT32 index2, INT32 index3);
	static FCDECL2(Object*, GetValueEx, ArrayBase* refThisUNSAFE, I4Array* indicesUNSAFE);

	// SetValue
	// This set of methods will set a value in an array
	static FCDECL5(void, SetValue,   ArrayBase* refThisUNSAFE, Object* objUNSAFE, INT32 index1, INT32 index2, INT32 index3);
	static FCDECL3(void, SetValueEx, ArrayBase* refThisUNSAFE, Object* objUNSAFE, I4Array* indicesUNSAFE);

	// This method will initialize an array from a TypeHandle
	//	to a field.
	static FCDECL2(void, InitializeArray, ArrayBase* vArrayRef, HANDLE handle);

};


#endif
