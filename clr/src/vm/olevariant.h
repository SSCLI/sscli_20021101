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
#ifndef _H_OLEVARIANT_
#define _H_OLEVARIANT_

class COMVariant;
#include "comvariant.h"


// The COM interop native array marshaler is built on top of VT_* types.
// The P/Invoke marshaler supports marshaling to WINBOOL's and ANSICHAR's.
// This is an annoying hack to shoehorn these non-OleAut types into
// the COM interop marshaler.
#define VTHACK_NONBLITTABLERECORD 251
#define VTHACK_BLITTABLERECORD 252
#define VTHACK_ANSICHAR        253
#define VTHACK_WINBOOL         254

class OleVariant
{
  public:

    // New variant conversion

    static void MarshalOleVariantForObject(OBJECTREF *pObj, VARIANT *pOle);
    static void MarshalObjectForOleVariant(const VARIANT *pOle, OBJECTREF *pObj);
    static void MarshalOleRefVariantForObject(OBJECTREF *pObj, VARIANT *pOle);

	// Variant conversion

	static void MarshalComVariantForOleVariant(VARIANT *pOle, VariantData *pCom);
	static void MarshalOleVariantForComVariant(VariantData *pCom, VARIANT *pOle);
	static void MarshalOleRefVariantForComVariant(VariantData *pCom, VARIANT *pOle);


	// Type conversion utilities
    static void ExtractContentsFromByrefVariant(VARIANT *pByrefVar, VARIANT *pDestVar);
    static void InsertContentsIntoByrefVariant(VARIANT *pSrcVar, VARIANT *pByrefVar);
    static void CreateByrefVariantForVariant(VARIANT *pSrcVar, VARIANT *pByrefVar);

	static VARTYPE GetVarTypeForComVariant(VariantData *pComVariant);
	static CVTypes GetCVTypeForVarType(VARTYPE vt);
	static VARTYPE GetVarTypeForCVType(CVTypes);
	static VARTYPE GetVarTypeForTypeHandle(TypeHandle typeHnd);

	static VARTYPE GetVarTypeForValueClassArrayName(LPCUTF8 pArrayClassName);
	static VARTYPE GetElementVarTypeForArrayRef(BASEARRAYREF pArrayRef);

	// Note that Rank == 0 means SZARRAY (that is rank 1, no lower bounds)
	static TypeHandle GetArrayForVarType(VARTYPE vt, TypeHandle elemType, unsigned rank=0, OBJECTREF* pThrowable=NULL);
	static UINT GetElementSizeForVarType(VARTYPE vt, MethodTable *pInterfaceMT);


    // Helper called from MarshalIUnknownArrayComToOle and MarshalIDispatchArrayComToOle.
    static void MarshalInterfaceArrayComToOleHelper(BASEARRAYREF *pComArray, void *oleArray,
                                                    MethodTable *pElementMT, BOOL bDefaultIsDispatch);

	struct Marshaler
	{
		void (*OleToComVariant)(VARIANT *pOleVariant, VariantData *pComVariant);
		void (*ComToOleVariant)(VariantData *pComVariant, VARIANT *pOleVariant);
		void (*OleRefToComVariant)(VARIANT *pOleVariant, VariantData *pComVariant);
		void (*OleToComArray)(void *oleArray, BASEARRAYREF *pComArray,
							  MethodTable *pInterfaceMT);
		void (*ComToOleArray)(BASEARRAYREF *pComArray, void *oleArray,
							  MethodTable *pInterfaceMT);
		void (*ClearOleArray)(void *oleArray, SIZE_T cElements, MethodTable *pInterfaceMT);
	};

	static Marshaler *GetMarshalerForVarType(VARTYPE vt);

#ifdef CUSTOMER_CHECKED_BUILD
    static BOOL CheckVariant(VARIANT *pOle);
#endif

private:

	// Specific marshaler functions

	static void MarshalBoolVariantOleToCom(VARIANT *pOleVariant, VariantData *pComVariant);
	static void MarshalBoolVariantComToOle(VariantData *pComVariant, VARIANT *pOleVariant);
	static void MarshalBoolVariantOleRefToCom(VARIANT *pOleVariant, VariantData *pComVariant);
	static void MarshalBoolArrayOleToCom(void *oleArray, BASEARRAYREF *pComArray,
										 MethodTable *pInterfaceMT);
	static void MarshalBoolArrayComToOle(BASEARRAYREF *pComArray, void *oleArray,
										 MethodTable *pInterfaceMT);

	static void MarshalWinBoolVariantOleToCom(VARIANT *pOleVariant, VariantData *pComVariant);
	static void MarshalWinBoolVariantComToOle(VariantData *pComVariant, VARIANT *pOleVariant);
	static void MarshalWinBoolVariantOleRefToCom(VARIANT *pOleVariant, VariantData *pComVariant);
	static void MarshalWinBoolArrayOleToCom(void *oleArray, BASEARRAYREF *pComArray,
										 MethodTable *pInterfaceMT);
	static void MarshalWinBoolArrayComToOle(BASEARRAYREF *pComArray, void *oleArray,
										 MethodTable *pInterfaceMT);

	static void MarshalAnsiCharVariantOleToCom(VARIANT *pOleVariant, VariantData *pComVariant);
	static void MarshalAnsiCharVariantComToOle(VariantData *pComVariant, VARIANT *pOleVariant);
	static void MarshalAnsiCharVariantOleRefToCom(VARIANT *pOleVariant, VariantData *pComVariant);
	static void MarshalAnsiCharArrayOleToCom(void *oleArray, BASEARRAYREF *pComArray,
										 MethodTable *pInterfaceMT);
	static void MarshalAnsiCharArrayComToOle(BASEARRAYREF *pComArray, void *oleArray,
										 MethodTable *pInterfaceMT);

	static void MarshalInterfaceVariantOleToCom(VARIANT *pOleVariant, VariantData *pComVariant);
	static void MarshalInterfaceVariantComToOle(VariantData *pComVariant, VARIANT *pOleVariant);
	static void MarshalInterfaceVariantOleRefToCom(VARIANT *pOleVariant, VariantData *pComVariant);
	static void MarshalInterfaceArrayOleToCom(void *oleArray, BASEARRAYREF *pComArray,
											  MethodTable *pInterfaceMT);
	static void MarshalIUnknownArrayComToOle(BASEARRAYREF *pComArray, void *oleArray,
											 MethodTable *pInterfaceMT);
	static void ClearInterfaceArray(void *oleArray, SIZE_T cElements, MethodTable *pInterfaceMT);

	static void MarshalBSTRVariantOleToCom(VARIANT *pOleVariant, VariantData *pComVariant);
	static void MarshalBSTRVariantComToOle(VariantData *pComVariant, VARIANT *pOleVariant);
	static void MarshalBSTRVariantOleRefToCom(VARIANT *pOleVariant, VariantData *pComVariant);
	static void MarshalBSTRArrayOleToCom(void *oleArray, BASEARRAYREF *pComArray,
										 MethodTable *pInterfaceMT);
	static void MarshalBSTRArrayComToOle(BASEARRAYREF *pComArray, void *oleArray,
										 MethodTable *pInterfaceMT);
	static void ClearBSTRArray(void *oleArray, SIZE_T cElements, MethodTable *pInterfaceMT);

	static void MarshalNonBlittableRecordArrayOleToCom(void *oleArray, BASEARRAYREF *pComArray,
										 MethodTable *pInterfaceMT);
	static void MarshalNonBlittableRecordArrayComToOle(BASEARRAYREF *pComArray, void *oleArray,
										 MethodTable *pInterfaceMT);
	static void ClearNonBlittableRecordArray(void *oleArray, SIZE_T cElements, MethodTable *pInterfaceMT);

	static void MarshalLPWSTRArrayOleToCom(void *oleArray, BASEARRAYREF *pComArray,
										 MethodTable *pInterfaceMT);
	static void MarshalLPWSTRRArrayComToOle(BASEARRAYREF *pComArray, void *oleArray,
										 MethodTable *pInterfaceMT);
	static void ClearLPWSTRArray(void *oleArray, SIZE_T cElements, MethodTable *pInterfaceMT);

	static void MarshalLPSTRArrayOleToCom(void *oleArray, BASEARRAYREF *pComArray,
										 MethodTable *pInterfaceMT);
	static void MarshalLPSTRRArrayComToOle(BASEARRAYREF *pComArray, void *oleArray,
										 MethodTable *pInterfaceMT);
	static void ClearLPSTRArray(void *oleArray, SIZE_T cElements, MethodTable *pInterfaceMT);

	static void MarshalDateVariantOleToCom(VARIANT *pOleVariant, VariantData *pComVariant);
	static void MarshalDateVariantComToOle(VariantData *pComVariant, VARIANT *pOleVariant);
	static void MarshalDateVariantOleRefToCom(VARIANT *pOleVariant, VariantData *pComVariant);
	static void MarshalDateArrayOleToCom(void *oleArray, BASEARRAYREF *pComArray,
										  MethodTable *pInterfaceMT);
	static void MarshalDateArrayComToOle(BASEARRAYREF *pComArray, void *oleArray,
										  MethodTable *pInterfaceMT);

	static void MarshalDecimalVariantOleToCom(VARIANT *pOleVariant, VariantData *pComVariant);
	static void MarshalDecimalVariantComToOle(VariantData *pComVariant, VARIANT *pOleVariant);
	static void MarshalDecimalVariantOleRefToCom(VARIANT *pOleVariant, VariantData *pComVariant);


    static void MarshalRecordVariantOleToCom(VARIANT *pOleVariant, VariantData *pComVariant);
    static void MarshalRecordVariantComToOle(VariantData *pComVariant, VARIANT *pOleVariant);
    static void MarshalRecordVariantOleRefToCom(VARIANT *pOleVariant, VariantData *pComVariant);
    static void MarshalRecordArrayOleToCom(void *oleArray, BASEARRAYREF *pComArray, MethodTable *pElementMT);
    static void MarshalRecordArrayComToOle(BASEARRAYREF *pComArray, void *oleArray, MethodTable *pElementMT);
    static void ClearRecordArray(void *oleArray, SIZE_T cElements, MethodTable *pElementMT);
};

#endif
