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
// This file contains the native methods that support the ArrayInfo class
//
// Date: August, 1998
////////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "comarrayinfo.h"
#include "reflectwrap.h"
#include "excep.h"
#include "commember.h"
#include "field.h"
#include "remoting.h"
#include "comcodeaccesssecurityengine.h"

FCIMPL5(Object*, COMArrayInfo::CreateInstance, ReflectClassBaseObject* typeUNSAFE, INT32 rank, INT32 length1, INT32 length2, INT32 length3)
{
    THROWSCOMPLUSEXCEPTION();

    PTRARRAYREF pRet = NULL;

    struct _gc
    {
        REFLECTCLASSBASEREF type;
        OBJECTREF           throwable;
    } gc;

    gc.type         = (REFLECTCLASSBASEREF)typeUNSAFE;
    gc.throwable    = NULL;
    
    _ASSERTE(gc.type != 0);
    ReflectClass* pRC = (ReflectClass*) gc.type->GetData();

    // Never create an array of TypedReferences, at least for now.
    if (pRC->GetTypeHandle().GetClass()->ContainsStackPtr())
    {
        HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, pRet);
        COMPlusThrow(kNotSupportedException, L"NotSupported_ContainsStackPtr[]");
        HELPER_METHOD_FRAME_END();
    }

    CorElementType CorType = pRC->GetCorElementType();

    // If we're trying to create an array of pointers or function pointers,
    // check that the caller has skip verification permission.
    if (CorType == ELEMENT_TYPE_PTR || CorType == ELEMENT_TYPE_FNPTR)
        COMCodeAccessSecurityEngine::SpecialDemand(SECURITY_SKIP_VER);

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, pRet);

    // Allocate the rank one array
    if (rank==1 && CorType >= ELEMENT_TYPE_BOOLEAN && CorType <= ELEMENT_TYPE_R8) 
    {
        pRet = (PTRARRAYREF)AllocatePrimitiveArray(CorType,length1);
    }
    else
    {
    // Find the Array class...
    ClassLoader* pLoader = pRC->GetModule()->GetClassLoader();
    TypeHandle typeHnd;

    _ASSERTE(pLoader);

    // Why not use FindArrayForElem??
        NameHandle typeName(rank == 1 ? ELEMENT_TYPE_SZARRAY : ELEMENT_TYPE_ARRAY,pRC->GetTypeHandle(),rank);

        GCPROTECT_BEGIN(gc);
        typeHnd = pLoader->FindTypeHandle(&typeName, &gc.throwable);
        if(gc.throwable != 0)
        {
            COMPlusThrow(gc.throwable);
        }

    _ASSERTE(!typeHnd.IsNull());

        _ASSERTE(rank >= 1 && rank <= 3);
    DWORD  boundsSize;
    INT32* bounds;
    if (typeHnd.AsArray()->GetNormCorElementType() != ELEMENT_TYPE_ARRAY) {
        boundsSize = rank;
        bounds = (INT32*) _alloca(boundsSize * sizeof(INT32));
            bounds[0] = length1;
            if (rank > 1) {
                bounds[1] = length2;
                if (rank == 3) {
                    bounds[2] = length3;
            }
        }
    }
    else {
        boundsSize = rank * 2;
        bounds = (INT32*) _alloca(boundsSize * sizeof(INT32));
        bounds[0] = 0;
            bounds[1] = length1;
            if (rank > 1) {
            bounds[2] = 0;
                bounds[3] = length2;
                if (rank == 3) {
                bounds[4] = 0;
                    bounds[5] = length3;
            }
        }
    }

        pRet = (PTRARRAYREF)AllocateArrayEx(typeHnd, bounds, boundsSize);
        GCPROTECT_END();
    }

    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(pRet);
}
FCIMPLEND

FCIMPL3(Object*, COMArrayInfo::CreateInstanceEx, ReflectClassBaseObject* typeUNSAFE, I4Array* lengthsUNSAFE, I4Array* lowerBoundsUNSAFE)
{
    PTRARRAYREF pRet = NULL;

    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(typeUNSAFE != 0);
    ReflectClass* pRC = (ReflectClass*) typeUNSAFE->GetData();

    I4ARRAYREF  lengths     = (I4ARRAYREF) lengthsUNSAFE;
    I4ARRAYREF  lowerBounds = (I4ARRAYREF) lowerBoundsUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_2(Frame::FRAME_ATTR_RETURNOBJ, lengths, lowerBounds);

    int  rank   = lengths->GetNumComponents();
    bool lowerb = (lowerBounds != NULL) ? true : false;


    // Never create an array of TypedReferences, ArgIterator, RuntimeArgument handle
    if (pRC->GetTypeHandle().GetClass()->ContainsStackPtr())
        COMPlusThrow(kNotSupportedException, L"NotSupported_ContainsStackPtr[]");

    CorElementType CorType = pRC->GetCorElementType();

    // If we're trying to create an array of pointers or function pointers,
    // check that the caller has skip verification permission.
    if (CorType == ELEMENT_TYPE_PTR || CorType == ELEMENT_TYPE_FNPTR)
        COMCodeAccessSecurityEngine::SpecialDemand(SECURITY_SKIP_VER);

    // Find the Array class...
    ClassLoader* pLoader = pRC->GetModule()->GetClassLoader();
    TypeHandle typeHnd;
    _ASSERTE(pLoader);
    OBJECTREF throwable = 0;
    GCPROTECT_BEGIN(throwable);

    // Why not use FindArrayForElem??
    NameHandle typeName((rank == 1 && !lowerb) ? ELEMENT_TYPE_SZARRAY : ELEMENT_TYPE_ARRAY,pRC->GetTypeHandle(),rank);
    typeHnd = pLoader->FindTypeHandle(&typeName, &throwable);
    if(throwable != 0)
        COMPlusThrow(throwable);
    GCPROTECT_END();

    _ASSERTE(!typeHnd.IsNull());

    DWORD  boundsSize;
    INT32* bounds;
    if (typeHnd.AsArray()->GetNormCorElementType() != ELEMENT_TYPE_ARRAY) {
        boundsSize = rank;
        bounds = (INT32*) _alloca(boundsSize * sizeof(INT32));

        for (int i=0;i<rank;i++) {
            bounds[i] = lengths->m_Array[i];
        }
    }
    else {
        boundsSize = rank*2;
        bounds = (INT32*) _alloca(boundsSize * sizeof(INT32));

        int i,j;
        for (i=0,j=0;i<rank;i++,j+=2) {
            if (lowerBounds != 0) {
                bounds[j] = lowerBounds->m_Array[i];
                bounds[j+1] = lengths->m_Array[i];
            }
            else  {
                bounds[j] = 0;
                bounds[j+1] = lengths->m_Array[i];
            }
        }
    }

    pRet = (PTRARRAYREF) AllocateArrayEx(typeHnd, bounds, boundsSize);

    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(pRet);
}
FCIMPLEND

FCIMPL4(Object*, COMArrayInfo::GetValue, ArrayBase * _refThis, INT32 index1, INT32 index2, INT32 index3)
{
    _ASSERTE(_refThis != NULL);
    BASEARRAYREF refThis(_refThis);
    ArrayClass*     pArray;
    TypeHandle      arrayElementType;

    // Validate the array args
    THROWSCOMPLUSEXCEPTION();
    arrayElementType = refThis->GetElementTypeHandle();
    EEClass* pEEC = refThis->GetClass();
    pArray = (ArrayClass*) pEEC;

    DWORD Rank                   = pArray->GetRank();
    DWORD dwOffset               = 0;
    DWORD dwMultiplier           = 1;
    const INT32 *pBoundsPtr      = refThis->GetBoundsPtr();
    const INT32 *pLowerBoundsPtr = refThis->GetLowerBoundsPtr();

    _ASSERTE(Rank <= 3);

    for (int i = Rank-1; i >= 0; i--) {
        INT32 curIndex;
        if (i == 2)
            curIndex = index3 - pLowerBoundsPtr[i];
        else if (i == 1)
            curIndex = index2 - pLowerBoundsPtr[i];
        else
            curIndex = index1 - pLowerBoundsPtr[i];

        // Bounds check each index
        // Casting to unsigned allows us to use one compare for [0..limit-1]
        if (((UINT32) curIndex) >= ((UINT32) pBoundsPtr[i]))
            FCThrow(kIndexOutOfRangeException);

        dwOffset     += curIndex * dwMultiplier;
        dwMultiplier *= pBoundsPtr[i];
    }

    // If it's a value type, erect a helper method frame before
    // calling CreateObject.
    Object* rv = NULL;
    if (arrayElementType.GetMethodTable()->IsValueClass()) {
        HELPER_METHOD_FRAME_BEGIN_RET_1(refThis);
         if (!CreateObject(&refThis, dwOffset, arrayElementType, pArray, rv))
			COMPlusThrow(kNotSupportedException, L"NotSupported_Type");		// createObject only fails if it sees a type it does not know about
        HELPER_METHOD_FRAME_END();
    }
    else {
        if (!CreateObject(&refThis, dwOffset, arrayElementType, pArray, rv))
			FCThrowRes(kNotSupportedException, L"NotSupported_Type");		// createObject only fails if it sees a type it does not know about
    }
    FC_GC_POLL_AND_RETURN_OBJREF(rv);
}
FCIMPLEND


FCIMPL2(Object*, COMArrayInfo::GetValueEx, ArrayBase* refThisUNSAFE, I4Array* indicesUNSAFE)
{
    ArrayClass*     pArray;
    TypeHandle      arrayElementType;

    Object*         rv       = NULL;
    BASEARRAYREF    refThis  = (BASEARRAYREF) refThisUNSAFE;
    I4ARRAYREF      pIndices = (I4ARRAYREF)   indicesUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_2(Frame::FRAME_ATTR_RETURNOBJ, refThis, pIndices);

    // Validate the array args
    THROWSCOMPLUSEXCEPTION();
    arrayElementType = refThis->GetElementTypeHandle();
    EEClass* pEEC = refThis->GetClass();
    pArray = (ArrayClass*) pEEC;

    DWORD Rank                   = pArray->GetRank();
    DWORD dwOffset               = 0;
    DWORD dwMultiplier           = 1;
    const INT32 *pBoundsPtr      = refThis->GetBoundsPtr();
    const INT32 *pLowerBoundsPtr = refThis->GetLowerBoundsPtr();

    for (int i = Rank-1; i >= 0; i--) {
        INT32 curIndex = pIndices->m_Array[i] - pLowerBoundsPtr[i];

        // Bounds check each index
        // Casting to unsigned allows us to use one compare for [0..limit-1]
        if (((UINT32) curIndex) >= ((UINT32) pBoundsPtr[i]))
            COMPlusThrow(kIndexOutOfRangeException);

        dwOffset     += curIndex * dwMultiplier;
        dwMultiplier *= pBoundsPtr[i];
    }

    if (!CreateObject(&refThis,dwOffset,arrayElementType,pArray, rv))
		COMPlusThrow(kNotSupportedException, L"NotSupported_Type");		// createObject only fails if it sees a type it does not know about

    HELPER_METHOD_FRAME_END();

    return rv;
}
FCIMPLEND


FCIMPL5(void, COMArrayInfo::SetValue, ArrayBase* refThisUNSAFE, Object* objUNSAFE, INT32 index1, INT32 index2, INT32 index3)
{
    ArrayClass*     pArray;
    TypeHandle      arrayElementType;

    // Validate the array args
    THROWSCOMPLUSEXCEPTION();

    BASEARRAYREF    refThis = (BASEARRAYREF) refThisUNSAFE;
    OBJECTREF       obj     = (OBJECTREF)    objUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_2(refThis, obj);

    arrayElementType = refThis->GetElementTypeHandle();
    EEClass* pEEC = refThis->GetClass();
    pArray = (ArrayClass*) pEEC;

    DWORD Rank                   = pArray->GetRank();
    DWORD dwOffset               = 0;
    DWORD dwMultiplier           = 1;
    const INT32 *pBoundsPtr      = refThis->GetBoundsPtr();
    const INT32 *pLowerBoundsPtr = refThis->GetLowerBoundsPtr();

    _ASSERTE(Rank <= 3);

    for (int i = Rank-1; i >= 0; i--) {
        INT32 curIndex;
        if (i == 2)
            curIndex = index3 - pLowerBoundsPtr[i];
        else if (i == 1)
            curIndex = index2 - pLowerBoundsPtr[i];
        else
            curIndex = index1 - pLowerBoundsPtr[i];

        // Bounds check each index
        // Casting to unsigned allows us to use one compare for [0..limit-1]
        if (((UINT32) curIndex) >= ((UINT32) pBoundsPtr[i]))
            COMPlusThrow(kIndexOutOfRangeException);

        dwOffset     += curIndex * dwMultiplier;
        dwMultiplier *= pBoundsPtr[i];
    }

    SetFromObject(&refThis,dwOffset,arrayElementType,pArray,&obj);

    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

FCIMPL3(void, COMArrayInfo::SetValueEx, ArrayBase* refThisUNSAFE, Object* objUNSAFE, I4Array* indicesUNSAFE)
{
    ArrayClass*     pArray;
    TypeHandle      arrayElementType;

    HELPER_METHOD_FRAME_BEGIN_NOPOLL();

    struct _gc
    {
        BASEARRAYREF    refThis;
        OBJECTREF       obj;
        I4ARRAYREF      pIndices;
    } gc;

    gc.refThis  = (BASEARRAYREF) refThisUNSAFE;
    gc.obj      = (OBJECTREF)    objUNSAFE;
    gc.pIndices = (I4ARRAYREF)   indicesUNSAFE;

    GCPROTECT_BEGIN(gc);

    // Validate the array args
    THROWSCOMPLUSEXCEPTION();
    arrayElementType = gc.refThis->GetElementTypeHandle();
    EEClass* pEEC = gc.refThis->GetClass();
    pArray = (ArrayClass*) pEEC;

    DWORD Rank                   = pArray->GetRank();
    DWORD dwOffset               = 0;
    DWORD dwMultiplier           = 1;
    const INT32 *pBoundsPtr      = gc.refThis->GetBoundsPtr();
    const INT32 *pLowerBoundsPtr = gc.refThis->GetLowerBoundsPtr();

    for (int i = Rank-1; i >= 0; i--) {
        INT32 curIndex = gc.pIndices->m_Array[i] - pLowerBoundsPtr[i];

        // Bounds check each index
        // Casting to unsigned allows us to use one compare for [0..limit-1]
        if (((UINT32) curIndex) >= ((UINT32) pBoundsPtr[i]))
            COMPlusThrow(kIndexOutOfRangeException);

        dwOffset     += curIndex * dwMultiplier;
        dwMultiplier *= pBoundsPtr[i];
    }

    SetFromObject(&gc.refThis,dwOffset,arrayElementType,pArray,&gc.obj);

    GCPROTECT_END();
    HELPER_METHOD_FRAME_END_POLL();
}
FCIMPLEND

// CreateObject
// Given an array and offset, we will either set rv to the object or create a boxed version
//  (This object is returned as a LPVOID so it can be directly returned.)
// Returns true if successful - otherwise, you should throw an exception.
BOOL COMArrayInfo::CreateObject(BASEARRAYREF* arrObj,DWORD dwOffset,TypeHandle elementType,ArrayClass* pArray, Object* &rv)
{
    // Get the type of the element...
    CorElementType type = elementType.GetSigCorElementType();
    switch (type) {
    case ELEMENT_TYPE_VOID:
        rv = 0;
        return true;

    case ELEMENT_TYPE_PTR:
        _ASSERTE(0);
        //COMVariant::NewPtrVariant(retObj,value,th);
        break;

    case ELEMENT_TYPE_CLASS:        // Class
    case ELEMENT_TYPE_SZARRAY:      // Single Dim, Zero
    case ELEMENT_TYPE_ARRAY:        // General Array
    case ELEMENT_TYPE_STRING:
    case ELEMENT_TYPE_OBJECT:
        {
            _ASSERTE(pArray->GetMethodTable()->GetComponentSize() == sizeof(OBJECTREF));
            BYTE* pData  = ((BYTE*) (*arrObj)->GetDataPtr()) + (dwOffset * sizeof(OBJECTREF));
            OBJECTREF o (*(Object **) pData);
            rv = OBJECTREFToObject(o);
            return true;
        }
        break;

    case ELEMENT_TYPE_VALUETYPE:
    case ELEMENT_TYPE_BOOLEAN:      // boolean
    case ELEMENT_TYPE_I1:           // sbyte
    case ELEMENT_TYPE_U1:
    case ELEMENT_TYPE_I2:           // short
    case ELEMENT_TYPE_U2:           
    case ELEMENT_TYPE_CHAR:         // char
    case ELEMENT_TYPE_I4:           // int
    case ELEMENT_TYPE_I:
    case ELEMENT_TYPE_U:
    case ELEMENT_TYPE_U4:
    case ELEMENT_TYPE_I8:           // long
    case ELEMENT_TYPE_U8:       
    case ELEMENT_TYPE_R4:           // float
    case ELEMENT_TYPE_R8:           // double
        {
            // Watch for GC here.  We allocate the object and then
            //  grab the void* to the data we are going to copy.
            OBJECTREF obj = AllocateObject(elementType.AsMethodTable());
            WORD wComponentSize = pArray->GetMethodTable()->GetComponentSize();
            BYTE* pData  = ((BYTE*) (*arrObj)->GetDataPtr()) + (dwOffset * wComponentSize);
            CopyValueClass(obj->UnBox(), pData, elementType.AsMethodTable(), obj->GetAppDomain());
            rv = OBJECTREFToObject(obj);
            return true;
        }
        break;
    case ELEMENT_TYPE_END:
    default:
        _ASSERTE(!"Unknown Type");
        return false;
    }
    // This is never hit because we exit from the switch statement.
    return false;
}


// SetFromObject
// Given an array and offset, we will set the object or value.  Returns whether it
// succeeded or failed (due to an unknown primitive type, etc).
void COMArrayInfo::SetFromObject(BASEARRAYREF* arrObj,DWORD dwOffset,TypeHandle elementType,
            ArrayClass* pArray,OBJECTREF* pObj)
{
    THROWSCOMPLUSEXCEPTION();

    // Get the type of the element...
    CorElementType elemtype = elementType.GetSigCorElementType();
    CorElementType srcType = ELEMENT_TYPE_END;
    if ((*pObj) != 0)
        srcType = (*pObj)->GetMethodTable()->GetNormCorElementType();

    switch (elemtype) {
    case ELEMENT_TYPE_VOID:
        break;

    case ELEMENT_TYPE_PTR:
        _ASSERTE(0);
        //COMVariant::NewPtrVariant(retObj,value,th);
        break;

    case ELEMENT_TYPE_CLASS:        // Class
    case ELEMENT_TYPE_SZARRAY:      // Single Dim, Zero
    case ELEMENT_TYPE_ARRAY:        // General Array
    case ELEMENT_TYPE_STRING:
    case ELEMENT_TYPE_OBJECT:
        {
            BYTE *pData;

            // This is the univeral zero so we set that and go
            if (*pObj == 0) {
                _ASSERTE(pArray->GetMethodTable()->GetComponentSize() == sizeof(OBJECTREF));
                pData  = ((BYTE*) (*arrObj)->GetDataPtr()) + (dwOffset * sizeof(OBJECTREF));
                ClearObjectReference(((OBJECTREF*)pData));
                return;
            }
            TypeHandle srcTh = (*pObj)->GetTypeHandle();

            if (srcTh.GetMethodTable()->IsThunking()) {
                srcTh = TypeHandle(srcTh.GetMethodTable()->AdjustForThunking(*pObj));
            }

            //  cast to the target.
            if (!srcTh.CanCastTo(elementType)) {
                BOOL fCastOK = FALSE;
                if ((*pObj)->GetMethodTable()->IsThunking()) {
                    fCastOK = CRemotingServices::CheckCast(*pObj, elementType.AsClass());
                }
                if (!fCastOK) {
                    COMPlusThrow(kInvalidCastException,L"InvalidCast_StoreArrayElement");
                }
            }

            // CRemotingServices::CheckCast above may have allowed a GC.  So delay
            // calculation until here.
            _ASSERTE(pArray->GetMethodTable()->GetComponentSize() == sizeof(OBJECTREF));
            pData  = ((BYTE*) (*arrObj)->GetDataPtr()) + (dwOffset * sizeof(OBJECTREF));
            SetObjectReference(((OBJECTREF*)pData),*pObj,(*arrObj)->GetAppDomain());
        }
        break;

    case ELEMENT_TYPE_VALUETYPE:
        {
            WORD wComponentSize = pArray->GetMethodTable()->GetComponentSize();
            BYTE* pData  = ((BYTE*) (*arrObj)->GetDataPtr()) + (dwOffset * wComponentSize);

            // Null is the universal zero...
            if (*pObj == 0) {
                InitValueClass(pData,elementType.AsMethodTable());
                return;
            }
            TypeHandle srcTh = (*pObj)->GetTypeHandle();

            //  cast to the target.
            if (!srcTh.CanCastTo(elementType))
                COMPlusThrow(kInvalidCastException, L"InvalidCast_StoreArrayElement");
            CopyValueClass(pData,(*pObj)->UnBox(),elementType.AsMethodTable(),
                           (*arrObj)->GetAppDomain());
            break;
        }
        break;

    case ELEMENT_TYPE_BOOLEAN:      // boolean
    case ELEMENT_TYPE_I1:           // byte
    case ELEMENT_TYPE_U1:
    case ELEMENT_TYPE_I2:           // short
    case ELEMENT_TYPE_U2:           
    case ELEMENT_TYPE_CHAR:         // char
    case ELEMENT_TYPE_I4:           // int
    case ELEMENT_TYPE_U4:
    case ELEMENT_TYPE_I:
    case ELEMENT_TYPE_U:
    case ELEMENT_TYPE_I8:           // long
    case ELEMENT_TYPE_U8:       
    case ELEMENT_TYPE_R4:           // float
    case ELEMENT_TYPE_R8:           // double
        {
            // Get a properly widened type
            ARG_SLOT value = 0;

            if (*pObj != 0) {
                if (!InvokeUtil::IsPrimitiveType(srcType))
                    COMPlusThrow(kInvalidCastException, L"InvalidCast_StoreArrayElement");

                COMMember::g_pInvokeUtil->CreatePrimitiveValue(elemtype,srcType,*pObj,&value);
            }

            WORD wComponentSize = pArray->GetMethodTable()->GetComponentSize();
            BYTE* pData  = ((BYTE*) (*arrObj)->GetDataPtr()) + (dwOffset * wComponentSize);
            memcpyNoGCRefs(pData, ArgSlotEndianessFixup(&value, wComponentSize), wComponentSize);
            break;
        }
        break;
    case ELEMENT_TYPE_END:
    default:
			// As the assert says, this should never happen unless we get a wierd type
        _ASSERTE(!"Unknown Type");
		COMPlusThrow(kNotSupportedException, L"NotSupported_Type");
    }
}

// This method will initialize an array from a TypeHandle to a field.

FCIMPL2(void, COMArrayInfo::InitializeArray, ArrayBase* pArrayRef, HANDLE handle)

    BASEARRAYREF arr = BASEARRAYREF(pArrayRef);
    if (arr == 0)
        FCThrowVoid(kArgumentNullException);
        
    FieldDesc* pField = (FieldDesc*) handle;
    if (pField == NULL)
        FCThrowVoid(kArgumentNullException);
    if (!pField->IsRVA())
        FCThrowVoid(kArgumentException);

    // Note that we do not check that the field is actually in the PE file that is initializing 
    // the array. Basically the data being published is can be accessed by anyone with the proper
    // permissions (C# marks these as assembly visibility, and thus are protected from outside 
    // snooping)

    CorElementType type = arr->GetElementType();
    if (!CorTypeInfo::IsPrimitiveType(type))
        FCThrowVoid(kArgumentException);

    DWORD dwCompSize = arr->GetComponentSize();
    DWORD dwElemCnt = arr->GetNumComponents();
    DWORD dwTotalSize = dwCompSize * dwElemCnt;

    DWORD size;


    HELPER_METHOD_FRAME_BEGIN_1(arr);
    size = pField->GetSize();
    HELPER_METHOD_FRAME_END();

    // make certain you don't go off the end of the rva static
    if (dwTotalSize > size)
        FCThrowVoid(kArgumentException);

    void *src = pField->GetStaticAddressHandle(NULL);
    void *dest = arr->GetDataPtr();

#ifdef BIGENDIAN
    DWORD i;
    switch (dwCompSize) {
    case 1:
        memcpyNoGCRefs(dest, src, dwElemCnt);
        break;
    case 2:
        for (i = 0; i < dwElemCnt; i++)
            *((UINT16*)dest + i) = GET_UNALIGNED_VAL16((UINT16*)src + i);
        break;
    case 4:
        for (i = 0; i < dwElemCnt; i++)

            *((UINT32*)dest + i) = GET_UNALIGNED_VAL32((UINT32*)src + i);
        break;
    case 8:
        for (i = 0; i < dwElemCnt; i++)
            *((UINT64*)dest + i) = GET_UNALIGNED_VAL64((UINT64*)src + i);
        break;
    default:
        // only primitive types are allowed - we should never get here
        _ASSERTE(false);
        break;
    }
#else
    memcpyNoGCRefs(dest, src, dwTotalSize);
#endif

FCIMPLEND
