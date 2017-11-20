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
// COMNDIRECT.CPP
//
// ECall's for the PInvoke classlibs
//


#include "common.h"

#include "clsload.hpp"
#include "method.hpp"
#include "class.h"
#include "object.h"
#include "field.h"
#include "util.hpp"
#include "excep.h"
#include "siginfo.hpp"
#include "threads.h"
#include "stublink.h"
#include "ecall.h"
#include "comclass.h"
#include "ndirect.h"
#include "gcdesc.h"
#include "jitinterface.h"
#include "eeconfig.h"
#include "log.h"
#include "nstruct.h"
#include "cgensys.h"
#include "gc.h"
#include "reflectutil.h"
#include "reflectwrap.h"
#include "security.h"
#include "comstringbuffer.h"
#include "dbginterface.h"
#include "objecthandle.h"
#include "comndirect.h"
#include "fcall.h"
#include "nexport.h"
#include "ml.h"
#include "comstring.h"
#include "remoting.h"


#define IDISPATCH_NUM_METHS 7
#define IUNKNOWN_NUM_METHS 3

BOOL IsStructMarshalable(EEClass *pcls)
{
    const FieldMarshaler *pFieldMarshaler = pcls->GetLayoutInfo()->GetFieldMarshalers();
    UINT  numReferenceFields              = pcls->GetLayoutInfo()->GetNumCTMFields();

    while (numReferenceFields--) {

        if (pFieldMarshaler->GetClass() == pFieldMarshaler->CLASS_ILLEGAL)
        {
            return FALSE;
        }

        ((BYTE*&)pFieldMarshaler) += MAXFIELDMARSHALERSIZE;
    }
    return TRUE;

}

//struct _StructureToPtrArgs
//{
//    DECLARE_ECALL_I4_ARG(INT32, fDeleteOld);
//    DECLARE_ECALL_I4_ARG(LPVOID, ptr);
//    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, pObj);
//};

FCIMPL3(VOID, StructureToPtr, Object* pObjUNSAFE, LPVOID ptr, BYTE fDeleteOld)
{
    THROWSCOMPLUSEXCEPTION();

    OBJECTREF pObj = (OBJECTREF) pObjUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_1(pObj);

    if (ptr == NULL)
        COMPlusThrowArgumentNull(L"ptr");
    if (pObj == NULL) 
        COMPlusThrowArgumentNull(L"structure");

    // Code path will accept both regular layout objects and boxed value classes
    // with layout.

    MethodTable *pMT = pObj->GetMethodTable();
    EEClass     *pcls = pMT->GetClass();
    if (pcls->IsBlittable()) {
        memcpyNoGCRefs(ptr, pObj->GetData(), pMT->GetNativeSize());
    } else if (pcls->HasLayout()) {
        if (fDeleteOld) {
            LayoutDestroyNative(ptr, pcls);
        }
        FmtClassUpdateNative( &(pObj), (LPBYTE)(ptr) );
    } else {
        COMPlusThrowArgumentException(L"structure", L"Argument_MustHaveLayoutOrBeBlittable");
    }

    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

FCIMPL3(VOID, PtrToStructureHelper, LPVOID ptr, Object* pObjIn, BYTE allowValueClasses)
{
    THROWSCOMPLUSEXCEPTION();

    OBJECTREF  pObj = ObjectToOBJECTREF(pObjIn);


    if (ptr == NULL)
        FCThrowArgumentNullVoid(L"ptr");
    if (pObj == NULL) 
        FCThrowArgumentNullVoid(L"structure");

    // Code path will accept regular layout objects.
    MethodTable *pMT = pObj->GetMethodTable();
    EEClass     *pcls = pMT->GetClass();

    // Validate that the object passed in is not a value class.
    if (!allowValueClasses && pcls->IsValueClass()) {
        FCThrowArgumentVoid(L"structure", L"Argument_StructMustNotBeValueClass");
    } else if (pcls->IsBlittable()) {
        memcpyNoGCRefs(pObj->GetData(), ptr, pMT->GetNativeSize());
    } else if (pcls->HasLayout()) {
        HELPER_METHOD_FRAME_BEGIN_1(pObj);
        LayoutUpdateComPlus( (LPVOID*) &(pObj), Object::GetOffsetOfFirstField(), pcls, (LPBYTE)(ptr), FALSE);
        HELPER_METHOD_FRAME_END();
    } else {
        FCThrowArgumentVoid(L"structure", L"Argument_MustHaveLayoutOrBeBlittable");
    }

}
FCIMPLEND


//struct _DestroyStructureArgs
//{
//    DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refClass);
//    DECLARE_ECALL_I4_ARG(LPVOID, ptr);
//};

FCIMPL2(VOID, DestroyStructure, LPVOID ptr, ReflectClassBaseObject* refClassUNSAFE)
{
    REFLECTCLASSBASEREF refClass = (REFLECTCLASSBASEREF) refClassUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_1(refClass);
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();

    if (ptr == NULL)
        COMPlusThrowArgumentNull(L"ptr");
    if (refClass == NULL)
        COMPlusThrowArgumentNull(L"structureType");
    if (refClass->GetMethodTable() != g_pRefUtil->GetClass(RC_Class))
        COMPlusThrowArgumentException(L"structureType", L"Argument_MustBeRuntimeType");

    EEClass *pcls = ((ReflectClass*) refClass->GetData())->GetClass();

    if (pcls->IsBlittable()) {
        // ok to call with blittable structure, but no work to do in this case.
    } else if (pcls->HasLayout()) {
        LayoutDestroyNative(ptr, pcls);
    } else {
        COMPlusThrowArgumentException(L"structureType", L"Argument_MustHaveLayoutOrBeBlittable");
    }

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND


/************************************************************************
 * PInvoke.SizeOf(Class)
 */
FCIMPL1(UINT32, SizeOfClass, ReflectClassBaseObject* refClass)
{
    THROWSCOMPLUSEXCEPTION();

    UINT32 rv;
    HELPER_METHOD_FRAME_BEGIN_RET_0();

    if (refClass == NULL)
        COMPlusThrowArgumentNull(L"t");
    if (refClass->GetMethodTable() != g_pRefUtil->GetClass(RC_Class))
        COMPlusThrowArgumentException(L"t", L"Argument_MustBeRuntimeType");

    EEClass *pcls = ((ReflectClass*) refClass->GetData())->GetClass();
    if (!(pcls->HasLayout() || pcls->IsBlittable())) 
    {
        DefineFullyQualifiedNameForClassW();
        GetFullyQualifiedNameForClassW(pcls);
        COMPlusThrow(kArgumentException, IDS_CANNOT_MARSHAL, _wszclsname_);
    }

    if (!IsStructMarshalable(pcls))
    {
        DefineFullyQualifiedNameForClassW();
        GetFullyQualifiedNameForClassW(pcls);
        COMPlusThrow(kArgumentException, IDS_CANNOT_MARSHAL, _wszclsname_);
    }

    rv = pcls->GetMethodTable()->GetNativeSize();
    HELPER_METHOD_FRAME_END();
    return rv;
}
FCIMPLEND


/************************************************************************
 * PInvoke.UnsafeAddrOfPinnedArrayElement(Array arr, int index)
 */

FCIMPL2(LPVOID, FCUnsafeAddrOfPinnedArrayElement, ArrayBase *arr, INT32 index) 
{   
    if (!arr)
        FCThrowArgumentNull(L"arr");

    return (arr->GetDataPtr() + (index*arr->GetComponentSize())); 
}
FCIMPLEND


/************************************************************************
 * PInvoke.SizeOf(Object)
 */

FCIMPL1(UINT32, FCSizeOfObject, LPVOID pVNStruct)
{

    OBJECTREF pNStruct;
    *((LPVOID*)&pNStruct) = pVNStruct;
    if (!pNStruct)
        FCThrow(kArgumentNullException);

    MethodTable *pMT = pNStruct->GetMethodTable();
    if (!(pMT->GetClass()->HasLayout() || pMT->GetClass()->IsBlittable()))
    {
        DefineFullyQualifiedNameForClassWOnStack();
        GetFullyQualifiedNameForClassW(pMT->GetClass());
        FCThrowEx(kArgumentException, IDS_CANNOT_MARSHAL, _wszclsname_, NULL, NULL);
    }

    if (!IsStructMarshalable(pMT->GetClass()))
    {
        DefineFullyQualifiedNameForClassWOnStack();
        GetFullyQualifiedNameForClassW(pMT->GetClass());
        FCThrowEx(kArgumentException, IDS_CANNOT_MARSHAL, _wszclsname_, NULL, NULL);
    }

    return pMT->GetNativeSize();
}
FCIMPLEND


//struct _OffsetOfHelperArgs
//{
//    DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF,      refField);
//};

/************************************************************************
 * PInvoke.OffsetOfHelper(Class, Field)
 */
FCIMPL1(UINT32, OffsetOfHelper, ReflectBaseObject* refFieldUNSAFE);
{
    UINT32          uRetVal = 0;
    REFLECTBASEREF  refField = (REFLECTBASEREF) refFieldUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_1(refField);

    THROWSCOMPLUSEXCEPTION();

    // Managed code enforces this envariant.
    _ASSERTE(refField);

    if (refField->GetMethodTable() != g_pRefUtil->GetClass(RC_Field))
        COMPlusThrowArgumentException(L"f", L"Argument_MustBeRuntimeFieldInfo");

    ReflectField* pRF = (ReflectField*) refField->GetData();
    FieldDesc *pField = pRF->pField;
    EEClass *pcls = pField->GetEnclosingClass();

    if (!(pcls->IsBlittable() || pcls->HasLayout()))
    {
        DefineFullyQualifiedNameForClassW();
        GetFullyQualifiedNameForClassW(pcls);
        COMPlusThrow(kArgumentException, IDS_CANNOT_MARSHAL, _wszclsname_);
    }
    
    if (!IsStructMarshalable(pcls))
    {
        DefineFullyQualifiedNameForClassW();
        GetFullyQualifiedNameForClassW(pcls);
        COMPlusThrow(kArgumentException, IDS_CANNOT_MARSHAL, _wszclsname_);
    }

    const FieldMarshaler *pFieldMarshaler = pcls->GetLayoutInfo()->GetFieldMarshalers();
    UINT  numReferenceFields              = pcls->GetLayoutInfo()->GetNumCTMFields();

    while (numReferenceFields--) 
    {
        if (pFieldMarshaler->m_pFD == pField) 
        {
            uRetVal = pFieldMarshaler->m_dwExternalOffset;
            goto lExit;
        }
        ((BYTE*&)pFieldMarshaler) += MAXFIELDMARSHALERSIZE;
    }

    {
        DefineFullyQualifiedNameForClassW();
        GetFullyQualifiedNameForClassW(pcls);
        COMPlusThrow(kArgumentException, IDS_EE_OFFSETOF_NOFIELDFOUND, _wszclsname_);
    }

lExit: ;
    HELPER_METHOD_FRAME_END();
    return uRetVal;
}
FCIMPLEND






FCIMPL0(UINT32, GetSystemMaxDBCSCharSize)
{
    return GetMaxDBCSCharByteSize();
}
FCIMPLEND


//struct _PtrToStringArgs
//{
//    DECLARE_ECALL_I4_ARG       (INT32,        len);
//    DECLARE_ECALL_I4_ARG       (LPVOID,       ptr);
//};

/************************************************************************
 * PInvoke.PtrToStringAnsi()
 */

FCIMPL2(Object*, PtrToStringAnsi, LPVOID ptr, INT32 len)
{
    STRINGREF pString = NULL;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, pString);
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();

    if (ptr == NULL)
        COMPlusThrowArgumentNull(L"ptr");
    if (len < 0)
        COMPlusThrowNonLocalized(kArgumentException, L"len");

    int nwc = 0;
    if (len != 0) {
        nwc = MultiByteToWideChar(CP_ACP,
                                  MB_PRECOMPOSED,
                                  (LPCSTR)(ptr),
                                  len,
                                  NULL,
                                  0);
        if (nwc == 0)
            COMPlusThrow(kArgumentException, IDS_UNI2ANSI_FAILURE);
    }                                      
    pString = COMString::NewString(nwc);
    MultiByteToWideChar(CP_ACP,
                        MB_PRECOMPOSED,
                        (LPCSTR)(ptr),
                        len,
                        pString->GetBuffer(),
                        nwc);

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(pString);
}
FCIMPLEND


FCIMPL2(Object*, PtrToStringUni, LPVOID ptr, INT32 len)
{
    STRINGREF pString = NULL;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, pString);
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();

    if (ptr == NULL)
        COMPlusThrowArgumentNull(L"ptr");
    if (len < 0)
        COMPlusThrowNonLocalized(kArgumentException, L"len");

    pString = COMString::NewString(len);
    memcpyNoGCRefs(pString->GetBuffer(), (LPVOID)(ptr), len*sizeof(WCHAR));

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(pString);
}
FCIMPLEND


//struct _CopyToNativeArgs
//{
//    DECLARE_ECALL_I4_ARG       (UINT32,       length);
//    DECLARE_ECALL_PTR_ARG      (LPVOID,       pdst);
//    DECLARE_ECALL_I4_ARG       (UINT32,       startindex);
//    DECLARE_ECALL_OBJECTREF_ARG(BASEARRAYREF, psrc);
//};

/************************************************************************
 * Handles all PInvoke.Copy(array source, ....) methods.
 */
FCIMPL4(void, CopyToNative, ArrayBase* psrcUNSAFE, UINT32 startindex, LPVOID pdst, UINT32 length)
{
    BASEARRAYREF psrc = (BASEARRAYREF) psrcUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_1(psrc);

    THROWSCOMPLUSEXCEPTION();

    if (pdst == NULL)
        COMPlusThrowArgumentNull(L"destination");
    if (psrc == NULL)
        COMPlusThrowArgumentNull(L"source");

    DWORD numelem = psrc->GetNumComponents();

    if (startindex > numelem  ||
        length > numelem      ||
        startindex > (numelem - length)) {
        COMPlusThrow(kArgumentOutOfRangeException, IDS_EE_COPY_OUTOFRANGE);
    }

    UINT32 componentsize = psrc->GetMethodTable()->GetComponentSize();

    CopyMemory(pdst,
               componentsize*startindex + (BYTE*)(psrc->GetDataPtr()),
               componentsize*length);

    HELPER_METHOD_FRAME_END();
}
FCIMPLEND


//struct _CopyToManagedArgs
//{
//    DECLARE_ECALL_I4_ARG       (UINT32,       length);
//    DECLARE_ECALL_I4_ARG       (UINT32,       startindex);
//    DECLARE_ECALL_OBJECTREF_ARG(BASEARRAYREF, pdst);
//    DECLARE_ECALL_PTR_ARG      (LPVOID,       psrc);
//};

/************************************************************************
 * Handles all PInvoke.Copy(..., array dst, ....) methods.
 */
FCIMPL4(void, CopyToManaged, LPVOID psrc, ArrayBase* pdstUNSAFE, UINT32 startindex, UINT32 length)
{
    BASEARRAYREF pdst = (BASEARRAYREF) pdstUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_1(pdst);

    THROWSCOMPLUSEXCEPTION();

    if (pdst == NULL)
        COMPlusThrowArgumentNull(L"destination");
    if (psrc == NULL)
        COMPlusThrowArgumentNull(L"source");

    DWORD numelem = pdst->GetNumComponents();

    if (startindex > numelem  ||
        length > numelem      ||
        startindex > (numelem - length)) {
        COMPlusThrow(kArgumentOutOfRangeException, IDS_EE_COPY_OUTOFRANGE);
    }

    UINT32 componentsize = pdst->GetMethodTable()->GetComponentSize();

    _ASSERTE(CorTypeInfo::IsPrimitiveType(pdst->GetElementTypeHandle().GetNormCorElementType()));
    memcpyNoGCRefs(componentsize*startindex + (BYTE*)(pdst->GetDataPtr()),
               psrc,
               componentsize*length);

    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

/************************************************************************
 * PInvoke.GetLastWin32Error
 */
FCIMPL0(UINT32, GetLastWin32Error)
{
    THROWSCOMPLUSEXCEPTION();
    return (UINT32)(GetThread()->m_dwLastError);
}
FCIMPLEND


/************************************************************************
 * Support for the GCHandle class.
 */

// Allocate a handle of the specified type, containing the specified
// object.
FCIMPL2(LPVOID, GCHandleInternalAlloc, Object *obj, int type)
{
    OBJECTREF objRef(obj);
    OBJECTHANDLE hnd;

    HELPER_METHOD_FRAME_BEGIN_RET_NOPOLL();
    THROWSCOMPLUSEXCEPTION();

    // If it is a pinned handle, check the object type.
    if (type == HNDTYPE_PINNED) GCHandleValidatePinnedObject(objRef);

    // Create the handle.
    if((hnd = GetAppDomain()->CreateTypedHandle(objRef, type)) == NULL)
        COMPlusThrowOM();
    HELPER_METHOD_FRAME_END_POLL();
    return (LPVOID) hnd;
}
FCIMPLEND

// Free a GC handle.
FCIMPL1(VOID, GCHandleInternalFree, OBJECTHANDLE handle)
{
    HELPER_METHOD_FRAME_BEGIN_0();
    THROWSCOMPLUSEXCEPTION();

    DestroyTypedHandle(handle);
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

// Get the object referenced by a GC handle.
FCIMPL1(LPVOID, GCHandleInternalGet, OBJECTHANDLE handle)
{
    OBJECTREF objRef;

    HELPER_METHOD_FRAME_BEGIN_RET_0();
    THROWSCOMPLUSEXCEPTION();

    objRef = ObjectFromHandle(handle);

    HELPER_METHOD_FRAME_END();
    return *((LPVOID*)&objRef);
}
FCIMPLEND

// Update the object referenced by a GC handle.
FCIMPL3(VOID, GCHandleInternalSet, OBJECTHANDLE handle, Object *obj, BYTE isPinned)
{
    OBJECTREF objRef(obj);
    HELPER_METHOD_FRAME_BEGIN_NOPOLL();
    THROWSCOMPLUSEXCEPTION();

    if (isPinned) GCHandleValidatePinnedObject(objRef);

    // Update the stored object reference.
    StoreObjectInHandle(handle, objRef);
    HELPER_METHOD_FRAME_END_POLL();
}
FCIMPLEND

// Update the object referenced by a GC handle.
FCIMPL4(VOID, GCHandleInternalCompareExchange, OBJECTHANDLE handle, Object *obj, Object* oldObj, BYTE isPinned)
{
    OBJECTREF newObjref(obj);
    OBJECTREF oldObjref(oldObj);
    HELPER_METHOD_FRAME_BEGIN_NOPOLL();
    THROWSCOMPLUSEXCEPTION();

    if (isPinned) GCHandleValidatePinnedObject(newObjref);

    // Update the stored object reference.
    InterlockedCompareExchangeObjectInHandle(handle, newObjref, oldObjref);
    HELPER_METHOD_FRAME_END_POLL();
}
FCIMPLEND

// Get the address of a pinned object referenced by the supplied pinned
// handle.  This routine assumes the handle is pinned and does not check.
FCIMPL1(LPVOID, GCHandleInternalAddrOfPinnedObject, OBJECTHANDLE handle)
{
    LPVOID p;
    HELPER_METHOD_FRAME_BEGIN_RET_0();
    THROWSCOMPLUSEXCEPTION();

    OBJECTREF objRef = ObjectFromHandle(handle);
    if (objRef == NULL)
        p = NULL;
    else
    {
        // Get the interior pointer for the supported pinned types.
        if (objRef->GetMethodTable() == g_pStringClass)
        {
            p = ((*(StringObject **)&objRef))->GetBuffer();
        }
        else if (objRef->GetMethodTable()->IsArray())
        {
            p = (*((ArrayBase**)&objRef))->GetDataPtr();
        }
        else
        {
            p = objRef->GetData();
        }
    }

    HELPER_METHOD_FRAME_END();
    return p;
}
FCIMPLEND

// Make sure the handle is accessible from the current domain.  (Throw if not.)
FCIMPL1(VOID, GCHandleInternalCheckDomain, OBJECTHANDLE handle)
{
    DWORD index = HndGetHandleTableADIndex(HndGetHandleTable(handle));

    if (index != 0 && index != GetAppDomain()->GetIndex())
        FCThrowArgumentVoid(L"handle", L"Argument_HandleLeak");
}
FCIMPLEND

// Check that the supplied object is valid to put in a pinned handle.
// Throw an exception if not.
void GCHandleValidatePinnedObject(OBJECTREF obj)
{
    THROWSCOMPLUSEXCEPTION();

    // NULL is fine.
    if (obj == NULL) return;

    if (obj->GetMethodTable() == g_pStringClass)
    {
        return;
    }

    if (obj->GetMethodTable()->IsArray())
    {
        BASEARRAYREF asArray = (BASEARRAYREF) obj;
        if (CorTypeInfo::IsPrimitiveType(asArray->GetElementType())) 
        {
            return;
        }
        {
            TypeHandle th = asArray->GetElementTypeHandle();
            if (th.IsUnsharedMT())
            {
                MethodTable *pMT = th.AsMethodTable();
                if (pMT->IsValueClass() && pMT->GetClass()->IsBlittable())
                {
                    return;
                }
            }
        }
        
    } 
    else if (obj->GetMethodTable()->GetClass()->IsBlittable())
    {
        return;
    }

    COMPlusThrow(kArgumentException, IDS_EE_NOTISOMORPHIC);

}

//struct _CalculateCountArgs
//{
//    DECLARE_ECALL_OBJECTREF_ARG(ArrayWithOffsetData*, pRef);
//};
FCIMPL1(UINT32, CalculateCount, ArrayWithOffsetData* pRefUNSAFE)
{
    UINT32 uRetVal = 0;
    ArrayWithOffsetData* pRef = pRefUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_1(pRef);

    THROWSCOMPLUSEXCEPTION();

    if (pRef->m_Array != NULL)
    {
        if (!(pRef->m_Array->GetMethodTable()->IsArray()))
        {
            COMPlusThrow(kArgumentException, IDS_EE_NOTISOMORPHIC);
        }
        GCHandleValidatePinnedObject(pRef->m_Array);
    }

    BASEARRAYREF pArray = pRef->m_Array;

    if (pArray == NULL) {
        if (pRef->m_cbOffset != 0) {
            COMPlusThrow(kIndexOutOfRangeException, IDS_EE_ARRAYWITHOFFSETOVERFLOW);
        }
        goto lExit;
    }

    {
    BASEARRAYREF pArrayBase = *((BASEARRAYREF*)&pArray);
    UINT32 cbTotalSize = pArrayBase->GetNumComponents() * pArrayBase->GetMethodTable()->GetComponentSize();
        if (pRef->m_cbOffset > cbTotalSize) {
        COMPlusThrow(kIndexOutOfRangeException, IDS_EE_ARRAYWITHOFFSETOVERFLOW);
    }

        uRetVal = cbTotalSize - pRef->m_cbOffset;
    }

lExit: ;
    HELPER_METHOD_FRAME_END();
    return uRetVal;
}
FCIMPLEND




    //====================================================================
    // *** Interop Helpers ***
    //====================================================================

//struct __ThrowExceptionForHR
//{
//    DECLARE_ECALL_I4_ARG(LPVOID, errorInfo); 
//    DECLARE_ECALL_I4_ARG(INT32, errorCode); 
//};

FCIMPL2(void, Interop::ThrowExceptionForHR, INT32 errorCode, LPVOID errorInfo)
{
    HELPER_METHOD_FRAME_BEGIN_0();
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();

    // Only throw on failure error codes.
    if (FAILED(errorCode))
    {
        // Retrieve the IErrorInfo to use.
        IErrorInfo *pErrorInfo = (IErrorInfo*)errorInfo;
        if (pErrorInfo == (IErrorInfo*)(-1))
        {
            pErrorInfo = NULL;
        }
        else if (!pErrorInfo)
        {
            if (GetErrorInfo(0, &pErrorInfo) != S_OK)
                pErrorInfo = NULL;
        }

        // Throw the exception based on the HR and the IErrorInfo.
        COMPlusThrowHR(errorCode, pErrorInfo);
    }

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND


//struct __GetHRForExceptionArgs        
//{
//    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, e);
//};

FCIMPL1(int, Interop::GetHRForException, Object* eUNSAFE)
{
    int retVal;
    OBJECTREF e = (OBJECTREF) eUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_1(e);
    //-[autocvtpro]-------------------------------------------------------

    retVal = SetupErrorInfo(e);

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return retVal;
}
FCIMPLEND





