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
** Class:  COMVariant
**
**                                        
**
** Purpose: Native Implementation of the Variant Class
**
** Date:  July 22, 1998
** 
===========================================================*/

#include "common.h"
#include "object.h"
#include "excep.h"
#include "frames.h"
#include "vars.hpp"
#include "comvariant.h"
#include "comstring.h"
#include "comstringcommon.h"
#include "commember.h"
#include "field.h"

//
// Class Variable Initialization
//
EEClass *COMVariant::s_pVariantClass=NULL;
ArrayTypeDesc* COMVariant::s_pVariantArrayTypeDesc;

static const CHAR ChangeTypeName[] = "ChangeType";
static const CHAR szOAVariantClass[] = "Microsoft.Win32.OAVariantLib";

MethodDesc* COMVariant::pOAChangeTypeMD = 0;


//The Name of the classes and the eeClass that we've looked up
//for them.  The eeClass is initialized to null in all instances.
ClassItem CVClasses[] = {
    {CLASS__EMPTY,   NULL, NULL},  //CV_EMPTY
    {CLASS__VOID,    NULL, NULL},  //CV_VOID, Changing this to object messes up signature resolution very badly.
    {CLASS__BOOLEAN, NULL, NULL},  //CV_BOOLEAN
    {CLASS__CHAR,    NULL, NULL},  //CV_CHAR
    {CLASS__SBYTE,   NULL, NULL},  //CV_I1
    {CLASS__BYTE,    NULL, NULL},  //CV_U1
    {CLASS__INT16,   NULL, NULL},  //CV_I2
    {CLASS__UINT16,  NULL, NULL},  //CV_U2
    {CLASS__INT32,   NULL, NULL},  //CV_I4
    {CLASS__UINT32,  NULL, NULL},  //CV_UI4
    {CLASS__INT64,   NULL, NULL},  //CV_I8
    {CLASS__UINT64,  NULL, NULL},  //CV_UI8
    {CLASS__SINGLE,  NULL, NULL},  //CV_R4   
    {CLASS__DOUBLE,  NULL, NULL},  //CV_R8   
    {CLASS__STRING,  NULL, NULL},  //CV_STRING
    {CLASS__VOID,  NULL, NULL},  //CV_PTR...We treat this as void
    {CLASS__DATE_TIME,NULL, NULL},  //CV_DATETIME
    {CLASS__TIMESPAN,NULL, NULL},  //CV_TIMESPAN
    {CLASS__OBJECT,  NULL, NULL},  //CV_OBJECT
    {CLASS__DECIMAL, NULL, NULL},  //CV_DECIMAL
    {CLASS__CURRENCY,NULL, NULL},  //CV_CURRENCY
    {CLASS__OBJECT,  NULL, NULL},  //ENUM...We treat this as OBJECT
    {CLASS__MISSING, NULL, NULL},  //CV_MISSING
    {CLASS__NULL,    NULL, NULL},  //CV_NULL
    {CLASS__NIL, NULL, NULL},                    //CV_LAST
};

// The Attributes Table
//  20 bits for built in types and 12 bits for Properties
//  The properties are followed by the widening mask.  All types widen to them selves.
const DWORD COMVariant::VariantAttributes[CV_LAST] = {
    0x00,                       // CV_EMPTY
    0x00,                       // CV_VOID
    CVA_Primitive | 0x0004,     // CV_BOOLEAN
    CVA_Primitive | 0x3F88,     // CV_CHAR (W = U2, CHAR, I4, U4, I8, U8, R4, R8) (U2 == Char)
    CVA_Primitive | 0x3550,     // CV_I1   (W = I1, I2, I4, I8, R4, R8) 
    CVA_Primitive | 0x3FE8,     // CV_U1   (W = CHAR, U1, I2, U2, I4, U4, I8, U8, R4, R8)
    CVA_Primitive | 0x3540,     // CV_I2   (W = I2, I4, I8, R4, R8)
    CVA_Primitive | 0x3F88,     // CV_U2   (W = U2, CHAR, I4, U4, I8, U8, R4, R8)
    CVA_Primitive | 0x3500,     // CV_I4   (W = I4, I8, R4, R8)
    CVA_Primitive | 0x3E00,     // CV_U4   (W = U4, I8, R4, R8)
    CVA_Primitive | 0x3400,     // CV_I8   (W = I8, R4, R8)
    CVA_Primitive | 0x3800,     // CV_U8   (W = U8, R4, R8)
    CVA_Primitive | 0x3000,     // CV_R4   (W = R4, R8)
    CVA_Primitive | 0x2000,     // CV_R8   (W = R8) 
    0x00,                       // CV_STRING
    0x00,                       // CV_PTR
    0x00,                       // CV_DATETIME
    0x00,                       // CV_TIMESPAN
    0x00,                       // CV_OBJECT
    0x00,                       // CV_DECIMAL
    0x00,                       // CV_CURRENCY
    0x00,                       // CV_MISSING
    0x00,                       // CV_NULL
};

//
// Current Conversions
// 

FCIMPL1(R4, COMVariant::GetR4FromVar, VariantData* var) {
    _ASSERTE(var);
    return (R4)var->GetDataAsUInt32();
}
FCIMPLEND
    
FCIMPL1(R8, COMVariant::GetR8FromVar, VariantData* var) {
    _ASSERTE(var);
    return *(R8 *) var->GetData();
}
FCIMPLEND


//
// Helper Routines
//

/*=================================LoadVariant==================================
**Action:  Initializes the variant class within the runtime.  Stores pointers to the
**         EEClass and MethodTable in static members of COMVariant
**
**Arguments: None
**
**Returns: S_OK if everything succeeded, else E_FAIL
**
**Exceptions: None.
==============================================================================*/
HRESULT __stdcall COMVariant::LoadVariant() {
    THROWSCOMPLUSEXCEPTION();

    s_pVariantClass = g_Mscorlib.FetchClass(CLASS__VARIANT)->GetClass();
    //
    s_pVariantArrayTypeDesc = g_Mscorlib.FetchType(TYPE__VARIANT_ARRAY).AsArray();

    // Fixup the ELEMENT_TYPE Void
    // We never create one of these, but we do depend on the value on the class being set properly in 
    // reflection.
    EEClass* pVoid = GetTypeHandleForCVType(CV_VOID).GetClass();
    pVoid->GetMethodTable()->m_NormType = ELEMENT_TYPE_VOID;


    //
    //

    // Run class initializers for Empty, Missing, and Null to set Value field
    OBJECTREF Throwable = NULL;
    if (!GetTypeHandleForCVType(CV_EMPTY).GetClass()->DoRunClassInit(&Throwable) ||
        !GetTypeHandleForCVType(CV_MISSING).GetClass()->DoRunClassInit(&Throwable) ||
        !GetTypeHandleForCVType(CV_NULL).GetClass()->DoRunClassInit(&Throwable) ||
        !pVoid->DoRunClassInit(&Throwable))
    {
        GCPROTECT_BEGIN(Throwable);
        COMPlusThrow(Throwable);
        GCPROTECT_END();
    }



    return S_OK;
}

// Returns System.Empty.Value.
OBJECTREF VariantData::GetEmptyObjectRef() const
{
    LPHARDCODEDMETASIG sig = &gsig_Fld_Empty;
    if (CVClasses[CV_EMPTY].ClassInstance==NULL)
        CVClasses[CV_EMPTY].ClassInstance = GetTypeHandleForCVType(CV_EMPTY).GetClass();
    FieldDesc * pFD = CVClasses[CV_EMPTY].ClassInstance->FindField("Value", sig);
    _ASSERTE(pFD);
    OBJECTREF obj = pFD->GetStaticOBJECTREF();
    _ASSERTE(obj!=NULL);
    return obj;
}


/*===============================GetMethodByName================================
**Action:  Get a method of name pwzMethodName from class eeMethodClass.  This 
**         method doesn't deal with two conversion methods of the same name with
**         different signatures.  We need to establish by fiat that such a thing
**         is impossible.
**Arguments:  eeMethodClass -- the class on which to look for the given method.
**            pwzMethodName -- the name of the method to find.
**Returns: A pointer to the MethodDesc for the appropriate method or NULL if the
**         named method isn't found.
**Exceptions: None.
==============================================================================*/
MethodDesc *GetMethodByName(EEClass *eeMethodClass, LPCUTF8 pwzMethodName) {

    _ASSERTE(eeMethodClass);
    _ASSERTE(pwzMethodName);

    DWORD limit = eeMethodClass->GetNumMethodSlots();; 
    
    for (DWORD i = 0; i < limit; i++) {
        MethodDesc *pCurMethod = eeMethodClass->GetUnknownMethodDescForSlot(i);
        if (pCurMethod != NULL)
            if (strcmp(pwzMethodName, pCurMethod->GetName((USHORT) i)) == 0)
            return pCurMethod;
    }
    return NULL;  //We never found a matching method.
}


/*===============================VariantGetClass================================
**Action:  Given a cvType, returns the associated EEClass.  We cache this information
**         so once we've looked it up once, we can get it very quickly the next time.
**Arguments: cvType -- the type of the class to retrieve.
**Returns: An EEClass for the given CVType.  If we don't know what CVType represents
**         or if it's CV_UNKOWN or CV_VOID, we just return NULL.
**Exceptions: None.
==============================================================================*/
EEClass *COMVariant::VariantGetClass(const CVTypes cvType) {
    _ASSERTE(cvType>=CV_EMPTY);
    _ASSERTE(cvType<CV_LAST);
    return GetTypeHandleForCVType(cvType).GetClass();
}


/*===============================GetTypeFromClass===============================
**Action: The complement of VariantGetClass, takes an EEClass * and returns the 
**        associated CVType.
**Arguments: EEClass * -- a pointer to the class for which we want the CVType.
**Returns:  The CVType associated with the EEClass or CV_OBJECT if this can't be 
**          determined.
**Exceptions: None
==============================================================================*/
CVTypes COMVariant::GetCVTypeFromClass(EEClass *eeCls) {

    if (!eeCls) {
        return CV_EMPTY;
    }

    //We'll start looking from Variant.  Empty and Void are handled below.
    for (int i=CV_EMPTY; i<CV_LAST; i++) {      
        if (eeCls == GetTypeHandleForCVType((CVTypes)i).GetClass()) {
            return (CVTypes)i;
        }
    }

    // The 1 check is a complete hack because COM classic
    //  object may have an EEClass of 1.  If it is 1 we return
    //  CV_OBJECT
    if (eeCls != (EEClass*) 1 && eeCls->IsEnum())
        return CV_ENUM;
    return CV_OBJECT;
    
}

CVTypes COMVariant::GetCVTypeFromTypeHandle(TypeHandle th)
{

    if (th.IsNull()) {
        return CV_EMPTY;
    }

    //We'll start looking from Variant.  Empty and Void are handled below.
    for (int i=CV_EMPTY; i<CV_LAST; i++) {      
        if (th == GetTypeHandleForCVType((CVTypes)i)) {
            return (CVTypes) i;
        }
    }

    if (th.IsEnum())
        return CV_ENUM;

    return CV_OBJECT;
}

// This code should be moved out of Variant and into Type.
FCIMPL1(INT32, COMVariant::GetCVTypeFromClassWrapper, ReflectClassBaseObject* refType)
{
    VALIDATEOBJECTREF(refType);
    _ASSERTE(refType->GetData());

    ReflectClass* pRC = (ReflectClass*) refType->GetData();

    // Find out if this type is a primitive or a class object
    return pRC->GetCorElementType();
    
}
FCIMPLEND

/*==================================NewVariant==================================
**N.B.:  This method does a GC Allocation.  Any method calling it is required to
**       GC_PROTECT the OBJECTREF.
**
**Actions:  Allocates a new Variant and fills it with the appropriate data.  
**Returns:  A new Variant with all of the appropriate fields filled out.
**Exceptions: OutOfMemoryError if v can't be allocated.       
==============================================================================*/
void COMVariant::NewVariant(VariantData* dest, const CVTypes type, OBJECTREF *objRef, void *pvData) {
    
    THROWSCOMPLUSEXCEPTION();
    _ASSERTE((type!=CV_EMPTY && type!=CV_NULL && type!=CV_MISSING) || objRef==NULL);  // Don't pass an object in for Empty.

    //If both arguments are null or both are specified, we're in an illegal situation.  Bail.
    //If all three are null, we're creating an empty variant
    if ((type >= CV_LAST || type < 0) || (objRef && pvData) || 
        (!objRef && !pvData && (type!=CV_EMPTY && type!=CV_NULL && type != CV_MISSING))) {
        COMPlusThrow(kArgumentException);
    }

    OBJECTREF ObjNull;
    ObjNull = NULL;
    
    //Fill in the data.
    dest->SetType(type);
    if (objRef) {
        if (*objRef != NULL) {
            EEClass* pEEC = (*objRef)->GetClass();
            if (!pEEC->IsValueClass()) {
                dest->SetObjRef(*objRef);
            }
            else {
                if (pEEC==s_pVariantClass) {
                    VariantData* pVar = (VariantData *) (*objRef)->UnBox();
                    dest->SetObjRef(pVar->GetObjRef());
                    dest->SetFullTypeInfo(pVar->GetFullTypeInfo());
                    dest->SetDataAsInt64(pVar->GetDataAsInt64());
                    return;
                }
                void* UnboxData = (*objRef)->UnBox();
                CVTypes cvt = GetCVTypeFromClass(pEEC);
                if (cvt>=CV_BOOLEAN && cvt<=CV_U4) {
                    dest->SetObjRef(NULL);
                    dest->SetDataAsInt64(*((INT32 *)UnboxData));
                    dest->SetType(cvt);
                } else if ((cvt>=CV_I8 && cvt<=CV_R8) || (cvt==CV_DATETIME)
                           || (cvt==CV_TIMESPAN) || (cvt==CV_CURRENCY)) {
                    dest->SetObjRef(NULL);
                    dest->SetDataAsInt64(*((INT64 *)UnboxData));
                    dest->SetType(cvt);
                } else if (cvt == CV_ENUM) {
                    TypeHandle th = (*objRef)->GetTypeHandle();
                    dest->SetType(GetEnumFlags(th.AsClass()));
                    switch(th.GetNormCorElementType()) {
                    case ELEMENT_TYPE_I1:
                    case ELEMENT_TYPE_U1:
                        dest->SetDataAsInt64(*((INT8 *)UnboxData));
                        break;

                    case ELEMENT_TYPE_I2:
                    case ELEMENT_TYPE_U2:
                        dest->SetDataAsInt64(*((INT16 *)UnboxData));
                        break;

                    IN_WIN32(case ELEMENT_TYPE_U:)
                    IN_WIN32(case ELEMENT_TYPE_I:)
                    case ELEMENT_TYPE_I4:
                    case ELEMENT_TYPE_U4:
                        dest->SetDataAsInt64(*((INT32 *)UnboxData));
                        break;

                    IN_WIN64(case ELEMENT_TYPE_U:)
                    IN_WIN64(case ELEMENT_TYPE_I:)
                    case ELEMENT_TYPE_I8:
                    case ELEMENT_TYPE_U8:
                        dest->SetDataAsInt64(*((INT64 *)UnboxData));
                        break;
                        
                    default:
                        _ASSERTE(!"Unsupported enum type when calling NewVariant");
                    }
                    dest->SetObjRef(th.CreateClassObj());
               } else {
                    // Decimal and other boxed value classes handled here.
                    dest->SetObjRef(*objRef);
                    dest->SetData(0);
                }
                return;
            }
        }
        else {
            dest->SetObjRef(*objRef);
        }

        dest->SetDataAsInt64(0);
        return;
    }

    // This is the case for both a null OBJECTREF or a primitive type.
    switch (type) {
        // Must get sign extension correct for all types smaller than an Int32.
    case CV_I1:
        _ASSERTE(pvData);
        dest->SetObjRef(NULL);
        dest->SetDataAsInt8(*((INT8 *)pvData));
        break;

    case CV_U1:
    case CV_BOOLEAN:
        _ASSERTE(pvData);
        dest->SetObjRef(NULL);
        dest->SetDataAsUInt8(*((UINT8 *)pvData));
        break;

    case CV_I2:
        _ASSERTE(pvData);
        dest->SetObjRef(NULL);
        dest->SetDataAsInt16(*((INT16 *)pvData));
        break;

    case CV_U2:
    case CV_CHAR:
        _ASSERTE(pvData);
        dest->SetObjRef(NULL);
        dest->SetDataAsUInt16(*((UINT16 *)pvData));
        break;
        
    case CV_I4:
        _ASSERTE(pvData);
        dest->SetObjRef(NULL);
        dest->SetDataAsInt32(*((INT32 *)pvData));
        break;

    case CV_U4:
    case CV_R4:  // we need to do a bitwise copy only.
        _ASSERTE(pvData);
        dest->SetObjRef(NULL);
        dest->SetDataAsUInt32(*((UINT32 *)pvData));
        break;
        
    case CV_I8:
    case CV_U8:
    case CV_R8:  // we need to do a bitwise copy only.
    case CV_DATETIME:
    case CV_CURRENCY:
    case CV_TIMESPAN:
        _ASSERTE(pvData);
        dest->SetObjRef(NULL);
        dest->SetDataAsInt64(*((INT64 *)pvData));
        break;

    case CV_MISSING:
    case CV_NULL:
        {
            LPHARDCODEDMETASIG sig = &gsig_Fld_Missing;
            if (type==CV_NULL)
                sig = &gsig_Fld_Null;
            if (CVClasses[type].ClassInstance==NULL)
                CVClasses[type].ClassInstance = GetTypeHandleForCVType(type).GetClass();
            FieldDesc * pFD = CVClasses[type].ClassInstance->FindField("Value", sig);
            _ASSERTE(pFD);
            OBJECTREF obj = pFD->GetStaticOBJECTREF();
            _ASSERTE(obj!=NULL);
            dest->SetObjRef(obj);
            dest->SetDataAsInt64(0);
            return;
        }

    case CV_EMPTY:
    case CV_OBJECT:
    case CV_DECIMAL:
    case CV_STRING:
    {
        dest->SetObjRef(ObjNull);
        break;
    }
    
    case CV_VOID:
        _ASSERTE(!"Never expected Variants of type CV_VOID.");
        COMPlusThrow(kNotSupportedException, L"Arg_InvalidOleVariantTypeException");
        return;

    case CV_ENUM:   // Enums require the enum's RuntimeType.
    default:
        // Did you add any new CVTypes, such as CV_R or CV_I?
        _ASSERTE(!"This CVType in NewVariant requires a non-null OBJECTREF!");
        COMPlusThrow(kNotSupportedException, L"Arg_InvalidOleVariantTypeException");
        return;
    }
}


// We use byref here, because that TypeHandle::CreateClassObj
// may trigger GC.  If dest is on the GC heap, we have no way to protect
// dest.
void COMVariant::NewEnumVariant(VariantData* &dest,INT64 val, TypeHandle th)
{
    int type;
    // Find out what type we have.
    EEClass* pEEC = th.AsClass();
    type = GetEnumFlags(pEEC);


    dest->SetType(type);
    dest->SetDataAsInt64(val);
    dest->SetObjRef(th.CreateClassObj());
}

void COMVariant::NewPtrVariant(VariantData* &dest,INT64 val, TypeHandle th)
{
    int type;
    // Find out what type we have.
    type = CV_PTR;

    dest->SetType(type);
    dest->SetDataAsInt64(val);
    dest->SetObjRef(th.CreateClassObj());
}

int COMVariant::GetEnumFlags(EEClass* pEEC)
{

    _ASSERTE(pEEC);
    _ASSERTE(pEEC->IsEnum());

    FieldDescIterator fdIterator(pEEC, FieldDescIterator::INSTANCE_FIELDS);
    FieldDesc* p = fdIterator.Next();
    _ASSERTE(p);
#ifdef _DEBUG
    WORD fldCnt = pEEC->GetNumInstanceFields();
#endif
    _ASSERTE(fldCnt == 1);

    CorElementType cet = p[0].GetFieldType();
    switch (cet) {
    case ELEMENT_TYPE_I1:
        return (CV_ENUM | EnumI1);
    case ELEMENT_TYPE_U1:
        return (CV_ENUM | EnumU1);
    case ELEMENT_TYPE_I2:
        return (CV_ENUM | EnumI2);
    case ELEMENT_TYPE_U2:
        return (CV_ENUM | EnumU2);
    IN_WIN32(case ELEMENT_TYPE_I:)
    case ELEMENT_TYPE_I4:
        return (CV_ENUM | EnumI4);
    IN_WIN32(case ELEMENT_TYPE_U:)
    case ELEMENT_TYPE_U4:
        return (CV_ENUM | EnumU4);
    IN_WIN64(case ELEMENT_TYPE_I:)
    case ELEMENT_TYPE_I8:
        return (CV_ENUM | EnumI8);
    IN_WIN64(case ELEMENT_TYPE_U:)
    case ELEMENT_TYPE_U8:
        return (CV_ENUM | EnumU8);
    default:
        _ASSERTE(!"UNknown Type");
        return 0;
    }
}


void COMVariant::BuildVariantFromTypedByRef(EEClass* cls,void* data,VariantData* var)
{

    if (cls == s_pVariantClass) {
        CopyValueClassUnchecked(var,data,s_pVariantClass->GetMethodTable());
        return;
    }
    CVTypes type = GetCVTypeFromClass(cls);
    if (type <= CV_R8 )
        NewVariant(var,type,0,data);
    else {
       if (type == CV_DATETIME || type == CV_TIMESPAN ||
            type == CV_CURRENCY) {
            NewVariant(var,type,0,data);
       }
       else {
            if (cls->IsValueClass()) {
                OBJECTREF retO = cls->GetMethodTable()->Box(data, FALSE);
                GCPROTECT_BEGIN(retO);
                COMVariant::NewVariant(var,&retO);
                GCPROTECT_END();
            }
            else {
                OBJECTREF o = ObjectToOBJECTREF(*((Object**)data));
                NewVariant(var,type,&o,0);
           }
       }
    }
    
}


FCIMPL2_VI(void, COMVariant::VariantToTypedRefAnyEx, TypedByRef typedByRef, VariantData* pvar)
{
    HELPER_METHOD_FRAME_BEGIN_0();
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();

    VariantData newVar;
	CVTypes targetType = CV_EMPTY;
	CVTypes sourceType = CV_EMPTY;

    EEClass* cls = typedByRef.type.GetClass();
    if (cls == s_pVariantClass) {
        void* p = typedByRef.data;
        CopyValueClassUnchecked(p,pvar,s_pVariantClass->GetMethodTable());
        goto lExit;
    }
    targetType = GetCVTypeFromClass(typedByRef.type.GetClass());
    sourceType = (CVTypes) pvar->GetType();
    // see if we need to change the types
    if (targetType != sourceType) {
        if (!pOAChangeTypeMD)
            GetOAChangeTypeMethod();
        // Change type has a fixed signature that returns a new variant
        //  and takes a class
        MetaSig sig(pOAChangeTypeMD->GetSig(),pOAChangeTypeMD->GetModule());
        UINT    nNumSlots = sig.NumVirtualFixedArgs(TRUE);
        ARG_SLOT*   pNewArgs = (ARG_SLOT *) _alloca(nNumSlots*sizeof(ARG_SLOT));
        ARG_SLOT*   pDst= pNewArgs;

        // The return Variant
        *pDst = PtrToArgSlot(&newVar);
        pDst++;

        ARG_SLOT* argDst;
        if(sizeof(VariantData)>sizeof(ARG_SLOT))
        {
            // argument address is placed in the slot
            argDst = (ARG_SLOT*) _alloca(sizeof(VariantData));
            *pDst = PtrToArgSlot(argDst);
        }
        else
        {
            // the argument itself is placed in the slot
            argDst = pDst;
        }
        CopyValueClassUnchecked(argDst, pvar, s_pVariantClass->GetMethodTable());
        pDst ++;

        // This pointer is the variant passed in...
        *pDst = targetType;
        pDst ++;

        // The short flag
        *pDst = 0;

        pOAChangeTypeMD->Call(pNewArgs,&sig);
    }
    else
        CopyValueClassUnchecked(&newVar, pvar, s_pVariantClass->GetMethodTable());

    // Now set the value
    switch (targetType) {
    case CV_BOOLEAN:
    case CV_I1:
    case CV_U1:
        *(INT8*) typedByRef.data = (INT8) newVar.GetDataAsInt64();
        break;

    case CV_CHAR:
    case CV_I2:
        *(INT16*) typedByRef.data = (INT16) newVar.GetDataAsInt64();
        break;

    case CV_I4:
    case CV_R4:
        *(INT32*) typedByRef.data = (INT32) newVar.GetDataAsInt64();
        break;

    case CV_I8:
    case CV_R8:
    case CV_DATETIME:
    case CV_TIMESPAN:
    case CV_CURRENCY:
        *(INT64*) typedByRef.data = newVar.GetDataAsInt64();
        break;

    case CV_DECIMAL:
    case CV_EMPTY:
    case CV_STRING:
    case CV_VOID:
    case CV_OBJECT:
    default:
        *(OBJECTREF*) typedByRef.data = newVar.GetObjRef();
        break;
    }

lExit: ;
    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND


FCIMPL1_RET_VC(VariantData, var, COMVariant::TypedByRefToVariantEx, TypedByRef value)
{
    HELPER_METHOD_FRAME_BEGIN_RET_VC_0();
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();
    CorElementType cType = value.type.GetNormCorElementType();
    if (cType == ELEMENT_TYPE_PTR) {
        COMPlusThrow(kNotSupportedException,L"NotSupported_ArrayOnly");
    }
    _ASSERTE(value.type.GetMethodTable() != 0);
    _ASSERTE(value.data != 0);
    EEClass* cls = value.type.GetClass();
    void* p = value.data;
    BuildVariantFromTypedByRef(cls,p,var);

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND_RET_VC

void COMVariant::GetOAChangeTypeMethod()
{
    EEClass* pOA = SystemDomain::SystemAssembly()->LookupTypeHandle(szOAVariantClass, NULL).GetClass();
    _ASSERTE(pOA);
    DWORD loopCnt = pOA->GetNumMethodSlots();
    for(DWORD i=0; i<loopCnt; i++) {
        // Get the MethodDesc for current method
        MethodDesc* pCurMethod = pOA->GetUnknownMethodDescForSlot(i);
        if (strcmp(pCurMethod->GetName((USHORT) i),ChangeTypeName) == 0) {
            if (IsMdPrivate(pCurMethod->GetAttrs())) {
                pOAChangeTypeMD = pCurMethod;
                break;
            }
        }
    }
    _ASSERTE(pOAChangeTypeMD);
    return;
}  


/*=================================SetFieldsR4==================================
**
==============================================================================*/
FCIMPL2(void, COMVariant::SetFieldsR4, VariantData* var, R4 val) {
    INT64 tempData;

    _ASSERTE(var);
   tempData = *((INT32 *)(&val));
    var->SetData(&tempData);
    var->SetType(CV_R4);
}
FCIMPLEND


/*=================================SetFieldsR8==================================
**
==============================================================================*/
FCIMPL2(void, COMVariant::SetFieldsR8, VariantData* var, R8 val) {
    _ASSERTE(var);
    var->SetData((void *)(&val));
    var->SetType(CV_R8);
}
FCIMPLEND


/*===============================SetFieldsObject================================
**
==============================================================================*/
FCIMPL2(void, COMVariant::SetFieldsObject, VariantData* var, Object* vVal) {

    _ASSERTE(var);
    OBJECTREF val = ObjectToOBJECTREF(vVal);

    EEClass *valClass;
    void *UnboxData;
    CVTypes cvt;
    TypeHandle typeHandle;

    valClass = val->GetClass();

    //If this isn't a value class, we should just skip out because we're not going
    //to do anything special with it.
    if (!valClass->IsValueClass()) {
        HELPER_METHOD_FRAME_BEGIN_NOPOLL();    // Set up a frame
        var->SetObjRef(val);
        typeHandle = TypeHandle(valClass->GetMethodTable());
        if (typeHandle==GetTypeHandleForCVType(CV_MISSING)) {
            var->SetType(CV_MISSING);
        } else if (typeHandle==GetTypeHandleForCVType(CV_NULL)) {
            var->SetType(CV_NULL);
        } else if (typeHandle==GetTypeHandleForCVType(CV_EMPTY)) {
            var->SetType(CV_EMPTY);
            var->SetObjRef(NULL);
        } else {
            var->SetType(CV_OBJECT);
        }
        HELPER_METHOD_FRAME_END();
        FC_GC_POLL();
        return;  
    }

    _ASSERTE(valClass->IsValueClass());

    //If this is a primitive type, we need to unbox it, get the value and create a variant
    //with just those values.
    UnboxData = val->UnBox();

    ClearObjectReference (var->GetObjRefPtr());
    typeHandle = TypeHandle(valClass->GetMethodTable());
    CorElementType cet = typeHandle.GetSigCorElementType();
    if (cet>=ELEMENT_TYPE_BOOLEAN && cet<=ELEMENT_TYPE_STRING) {
        cvt = (CVTypes)cet;
    } else {
        // This could potentially load a type which could cause a GC.
        HELPER_METHOD_FRAME_BEGIN_NOPOLL();    // Set up a frame
        cvt = GetCVTypeFromClass(valClass);
        HELPER_METHOD_FRAME_END();
    }
    var->SetType(cvt);


    //copy all of the data.
    // Copies must be done based on the exact number of bytes to copy.
    // We don't want to read garbage from other blocks of memory.
    //CV_I8 --> CV_R8, CV_DATETIME, CV_TIMESPAN, & CV_CURRENCY are all of the 8 byte quantities
    //If we don't find one of those ranges, we've found a value class 
    //of which we don't have inherent knowledge, so just slam that into an
    //ObjectRef.
    if (cvt>=CV_BOOLEAN && cvt<=CV_U1 && cvt != CV_CHAR) {
        var->SetDataAsInt64(*((UINT8 *)UnboxData));
    } else if (cvt==CV_CHAR || cvt>=CV_I2 && cvt<=CV_U2) {
        var->SetDataAsInt64(*((UINT16 *)UnboxData));
    } else if (cvt>=CV_I4 && cvt<=CV_U4 || cvt==CV_R4) {
        var->SetDataAsInt64(*((UINT32 *)UnboxData));
    } else if ((cvt>=CV_I8 && cvt<=CV_R8) || (cvt==CV_DATETIME)
               || (cvt==CV_TIMESPAN) || (cvt==CV_CURRENCY)) {
        var->SetDataAsInt64(*((INT64 *)UnboxData));
    } else if (cvt==CV_EMPTY || cvt==CV_NULL || cvt==CV_MISSING) {
        var->SetType(cvt);
    } else if (cvt==CV_ENUM) {
        //This could potentially allocate a new object, so we set up a frame.
        HELPER_METHOD_FRAME_BEGIN_NOPOLL();    // Set up a frame
        GCPROTECT_BEGININTERIOR(var)
        var->SetDataAsInt64(*((INT32 *)UnboxData));
        var->SetObjRef(typeHandle.CreateClassObj());
        var->SetType(GetEnumFlags(typeHandle.AsClass()));
        GCPROTECT_END();
        HELPER_METHOD_FRAME_END();
    } else {
        // Decimals and other boxed value classes get handled here.
        var->SetObjRef(val);
    }

    FC_GC_POLL();
}
FCIMPLEND


    
/*================================GetBoxedObject================================
**Action:  Generates a boxed object (Int32, Double, etc) or returns the 
**         currently held object.  This is more useful if you're certain that you
**         really need an Object.
==============================================================================*/
OBJECTREF COMVariant::GetBoxedObject(VariantData* vRef) {
    CVTypes cvt;

    // To ensure correct endianess passed in Box, copy the 
    // necessary data into tempbuff
    BYTE            tempBuff[4];
    void            *pValue = NULL;


    switch (cvt=vRef->GetType()) {
    case CV_BOOLEAN:
    case CV_I1:
        pValue = tempBuff;
        *(INT8 *)tempBuff = vRef->GetDataAsInt8();
        break;
    case CV_U1:
        pValue = tempBuff;
        *(UINT8 *)tempBuff = vRef->GetDataAsUInt8();
        break;

    case CV_I2:
        pValue = tempBuff;
        *(INT16 *)tempBuff = vRef->GetDataAsInt16();
        break;
    case CV_U2:
    case CV_CHAR:
        pValue = tempBuff;
        *(UINT16 *)tempBuff = vRef->GetDataAsUInt16();
        break;
    case CV_I4:
        pValue = tempBuff;
        *(INT32 *)tempBuff = vRef->GetDataAsInt32();
        break;
    case CV_U4:
    case CV_R4: 
        pValue = tempBuff;
        *(UINT32 *)tempBuff = vRef->GetDataAsUInt32();
        break;

    case CV_I8:
    case CV_R8:
    case CV_U8:
    case CV_DATETIME:
    case CV_TIMESPAN:
    case CV_CURRENCY:
        return vRef->GetEEClass()->GetMethodTable()->Box(vRef->GetData(), FALSE);

    case CV_ENUM: {
        OBJECTREF obj = vRef->GetObjRef();
        _ASSERTE(obj != NULL);
        ReflectClass* pRC = (ReflectClass*) ((REFLECTCLASSBASEREF) obj)->GetData();
        _ASSERTE(pRC);
        EEClass* pEEC = pRC->GetClass();
        _ASSERTE(pEEC);
        MethodTable* mt = pEEC->GetMethodTable();
        _ASSERTE(mt);
        obj = mt->Box(vRef->GetData());
        return obj;
    }

    case CV_VOID:
    case CV_DECIMAL:
    case CV_EMPTY:
    case CV_STRING:
    case CV_OBJECT:
    default:
        //Check for void done as an assert instead of an extra branch on the switch table s.t. we don't expand the
        //jump table.
        _ASSERTE(cvt!=CV_VOID || "We shouldn't have been able to create an instance of a void.");
        return vRef->GetObjRef(); //We already have an object, so we'll just give it back.
    };


    MethodTable* pMT = vRef->GetEEClass()->GetMethodTable();
    _ASSERTE(pMT);
    _ASSERTE(pValue);

    return pMT->Box(pValue, FALSE);
}

FCIMPL1(void, COMVariant::InitVariant, LPVOID)
{
    HELPER_METHOD_FRAME_BEGIN_0();
    EnsureVariantInitialized();
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

FCIMPL1(Object*, COMVariant::BoxEnum, VariantData* var)
{
    OBJECTREF retO = NULL;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, retO);
    //-[autocvtpro]-------------------------------------------------------

    _ASSERTE(var);
#ifdef _DEBUG
    CVTypes vType = (CVTypes) var->GetType();
#endif
    _ASSERTE(vType == CV_ENUM);
    _ASSERTE(var->GetObjRef() != NULL);

    ReflectClass* pRC = (ReflectClass*) ((REFLECTCLASSBASEREF) var->GetObjRef())->GetData();
    _ASSERTE(pRC);

    MethodTable* mt = pRC->GetClass()->GetMethodTable();
    _ASSERTE(mt);

    retO = mt->Box(var->GetData(), FALSE);

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(retO);
}
FCIMPLEND

TypeHandle VariantData::GetTypeHandle()
{
    if (GetType() == CV_ENUM || GetType() == CV_PTR) {
        _ASSERTE(GetObjRef() != NULL);
        ReflectClass* pRC = (ReflectClass*) ((REFLECTCLASSBASEREF) GetObjRef())->GetData();
        _ASSERTE(pRC);
        return pRC->GetTypeHandle();

    }
    if (GetType() != CV_OBJECT)
        return TypeHandle(GetTypeHandleForCVType(GetType()).GetMethodTable());
    if (GetObjRef() != NULL)
        return GetObjRef()->GetTypeHandle();
    return TypeHandle(g_pObjectClass);
}


// Use this very carefully.  There is not a direct mapping between
//  CorElementType and CVTypes for a bunch of things.  In this case
//  we return CV_LAST.  You need to check this at the call site.
CVTypes COMVariant::CorElementTypeToCVTypes(CorElementType type)
{
    if (type <= ELEMENT_TYPE_STRING)
        return (CVTypes) type;
    if (type == ELEMENT_TYPE_CLASS || type == ELEMENT_TYPE_OBJECT)
        return (CVTypes) ELEMENT_TYPE_CLASS;
    return CV_LAST;
}
