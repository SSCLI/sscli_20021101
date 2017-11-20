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
// COMNDIRECT.H -
//
// ECall's for the NDirect classlibs
//


#ifndef __COMNDIRECT_H__
#define __COMNDIRECT_H__

#include "fcall.h"

//!!! Must be kept in sync with ArrayWithOffset class layout.
struct ArrayWithOffsetData
{
    BASEARRAYREF    m_Array;
    UINT32          m_cbOffset;
    UINT32          m_cbCount;
};

FCDECL4(void, CopyToNative, ArrayBase* psrcUNSAFE, UINT32 startindex, LPVOID pdst, UINT32 length);
FCDECL4(void, CopyToManaged, LPVOID psrc, ArrayBase* pdstUNSAFE, UINT32 startindex, UINT32 length);
FCDECL1(UINT32, SizeOfClass, ReflectClassBaseObject* refClass);


FCDECL1(UINT32, FCSizeOfObject, LPVOID pNStruct);
FCDECL2(LPVOID, FCUnsafeAddrOfPinnedArrayElement, ArrayBase *arr, INT32 index);

FCDECL1(UINT32, OffsetOfHelper, ReflectBaseObject* refFieldUNSAFE);
FCDECL0(UINT32, GetLastWin32Error);
FCDECL1(UINT32, CalculateCount, ArrayWithOffsetData* pRef);
FCDECL2(Object*, PtrToStringAnsi, LPVOID ptr, INT32 len);
FCDECL2(Object*, PtrToStringUni, LPVOID ptr, INT32 len);

FCDECL3(VOID, StructureToPtr, Object* pObjUNSAFE, LPVOID ptr, BYTE fDeleteOld);
FCDECL3(VOID, PtrToStructureHelper, LPVOID ptr, Object* pObjIn, BYTE allowValueClasses);
FCDECL2(VOID, DestroyStructure, LPVOID ptr, ReflectClassBaseObject* refClassUNSAFE);


FCDECL0(UINT32, GetSystemMaxDBCSCharSize);

FCDECL2(LPVOID, GCHandleInternalAlloc, Object *obj, int type);
FCDECL1(VOID, GCHandleInternalFree, OBJECTHANDLE handle);
FCDECL1(LPVOID, GCHandleInternalGet, OBJECTHANDLE handle);
FCDECL3(VOID, GCHandleInternalSet, OBJECTHANDLE handle, Object *obj, BYTE isPinned);
FCDECL4(VOID, GCHandleInternalCompareExchange, OBJECTHANDLE handle, Object *obj, Object* oldObj, BYTE isPinned);
FCDECL1(LPVOID, GCHandleInternalAddrOfPinnedObject, OBJECTHANDLE handle);
FCDECL1(VOID, GCHandleInternalCheckDomain, OBJECTHANDLE handle);
void GCHandleValidatePinnedObject(OBJECTREF obj);



	//====================================================================
	// *** Interop Helpers ***
	//====================================================================


class Interop
{
public:
    //====================================================================
    // These methods convert between an HR and and a managed exception.
    //====================================================================
    static FCDECL2(void, ThrowExceptionForHR, INT32 errorCode, LPVOID errorInfo);
    static FCDECL1(int, GetHRForException, Object* eUNSAFE);


};

#endif
