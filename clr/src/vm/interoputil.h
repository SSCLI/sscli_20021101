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
#ifndef _H_INTEROP_UTIL
#define _H_INTEROP_UTIL

#include "debugmacros.h"

#include "interopconverter.h"

class TypeHandle;
struct VariantData;

// HR to exception helper.
#ifdef _DEBUG

#define IfFailThrow(EXPR) \
do { hr = (EXPR); if(FAILED(hr)) { DebBreakHr(hr); COMPlusThrowHR(hr); } } while (0)

#else // _DEBUG

#define IfFailThrow(EXPR) \
do { hr = (EXPR); if(FAILED(hr)) { COMPlusThrowHR(hr); } } while (0)

#endif // _DEBUG

// Out of memory helper.
#define IfNullThrow(EXPR) \
do {if ((EXPR) == 0) {IfFailThrow(E_OUTOFMEMORY);} } while (0)

//--------------------------------------------------------------------------------
// helper for DllCanUnload now
HRESULT STDMETHODCALLTYPE EEDllCanUnloadNow(void);

//------------------------------------------------------------------
//  HRESULT SetupErrorInfo(OBJECTREF pThrownObject)
// setup error info for exception object
//
HRESULT SetupErrorInfo(OBJECTREF pThrownObject);

//--------------------------------------------------------------------------------
// ULONG SafeRelease(IUnknown* pUnk)
// Release helper, enables and disables GC during call-outs
ULONG SafeRelease(IUnknown* pUnk);

//--------------------------------------------------------------------------------
// void SafeVariantClear(VARIANT* pVar)
// VariantClear helper GC safe.
void SafeVariantClear(VARIANT* pVar);

//--------------------------------------------------------------------------------
// Determines if a COM object can be cast to the specified type.
BOOL CanCastComObject(OBJECTREF obj, TypeHandle hndType);

struct ExceptionData;
//-------------------------------------------------------------------
// void FillExceptionData(ExceptionData* pedata, IErrorInfo* pErrInfo)
// Called from DLLMain, to initialize com specific data structures.
//-------------------------------------------------------------------
void FillExceptionData(ExceptionData* pedata, IErrorInfo* pErrInfo);



class FieldDesc;

//------------------------------------------------------------------------------
// INT64 FieldAccessor(FieldDesc* pFD, OBJECTREF oref, INT64 val, BOOL isGetter, UINT8 cbSize)
// helper to access fields from an object
INT64 FieldAccessor(FieldDesc* pFD, OBJECTREF oref, INT64 val, BOOL isGetter, U1 cbSize);

//------------------------------------------------------------------------------
// BOOL IsInstanceOf(MethodTable *pMT, MethodTable* pParentMT)
BOOL IsInstanceOf(MethodTable *pMT, MethodTable* pParentMT);




//---------------------------------------------------------------------------
// Returns the index of the LCID parameter if one exists and -1 otherwise.
int GetLCIDParameterIndex(IMDInternalImport *pInternalImport, mdMethodDef md);

//---------------------------------------------------------------------------
// Transforms an LCID into a CultureInfo.
void GetCultureInfoForLCID(LCID lcid, OBJECTREF *pCultureObj);

//---------------------------------------------------------------------------
// This method determines if a member is visible from COM.
BOOL IsMemberVisibleFromCom(IMDInternalImport *pInternalImport, mdToken tk, mdMethodDef mdAssociate);

//--------------------------------------------------------------------------------
// This method generates a stringized version of an interface that contains the
// name of the interface along with the signature of all the methods.
SIZE_T GetStringizedItfDef(TypeHandle InterfaceType, CQuickArray<BYTE> &rDef);

//--------------------------------------------------------------------------------
// Helper to get the stringized form of assembly guid.
HRESULT GetStringizedGuidForAssembly(Assembly *pAssembly, CQuickArray<BYTE> &rDef, ULONG cbCur, ULONG *pcbFetched);



inline HRESULT QuickCOMStartup()
{
    return S_OK;
}

__inline VOID LogInteropRelease(IUnknown* pUnk, ULONG cbRef, LPSTR szMsg) {}


#endif
