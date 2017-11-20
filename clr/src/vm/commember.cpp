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
// This Module contains routines that expose properties of Member (Classes, Constructors
//  Interfaces and Fields)
//
// Date: March/April 1998
////////////////////////////////////////////////////////////////////////////////

#include "common.h"

#include "commember.h"
#include "sigformat.h"
#include "comvariant.h"
#include "comstring.h"
#include "method.hpp"
#include "threads.h"
#include "excep.h"
#include "corerror.h"
#include "security.h"
#include "expandsig.h"
#include "remoting.h"
#include "classnames.h"
#include "fcall.h"
#include "dbginterface.h"
#include "eeconfig.h"
#include "comcodeaccesssecurityengine.h"

#include "threads.inl"


// Static Fields
MethodTable* COMMember::m_pMTParameter = 0;
MethodTable* COMMember::m_pMTIMember = 0;
MethodTable* COMMember::m_pMTFieldInfo = 0;
MethodTable* COMMember::m_pMTPropertyInfo = 0;
MethodTable* COMMember::m_pMTEventInfo = 0;
MethodTable* COMMember::m_pMTType = 0;
MethodTable* COMMember::m_pMTMethodInfo = 0;
MethodTable* COMMember::m_pMTConstructorInfo = 0;
MethodTable* COMMember::m_pMTMethodBase = 0;

// This is the global access to the InvokeUtil class
InvokeUtil* COMMember::g_pInvokeUtil = 0;

// NOTE: These are defined in CallingConventions.cs.
#define CALLCONV_Standard       0x0001
#define CALLCONV_VarArgs        0x0002
#define CALLCONV_Any            CALLCONV_Standard | CALLCONV_VarArgs
#define CALLCONV_HasThis        0x0020
#define CALLCONV_ExplicitThis   0x0040


// GetFieldInfoToString
// This method will return the string representation of the information in FieldInfo
FCIMPL1(Object*, COMMember::GetFieldInfoToString, ReflectBaseObject* refThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    if (refThisUNSAFE == NULL) 
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    STRINGREF       refSig = NULL;
    FieldDesc*      pField;

    // Get the field descr
    ReflectField* pRF = (ReflectField*) refThisUNSAFE->GetData();
    pField = pRF->pField;
    _ASSERTE(pField);

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refSig);

    // Put into a basic block so SigFormat is destroyed before the throw
    {
        FieldSigFormat sigFmt(pField);
        refSig = sigFmt.GetString();
    }
    if (!refSig) {
        _ASSERTE(!"Unable to Create String");
        FATAL_EE_ERROR();
    }
    _ASSERTE(refSig);

    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refSig);
}
FCIMPLEND

// GetMethodInfoToString
// This method will return the string representation of the information in MethodInfo
FCIMPL1(Object*, COMMember::GetMethodInfoToString, ReflectBaseObject* refThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    if (refThisUNSAFE == NULL) 
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    STRINGREF       refSig = NULL;
    MethodDesc*     pMeth;

    // Get the method Descr  (this should not fail)
    //  NOTE: both a constructor and a method are represented by a MetodDesc.
    //      If this ever changes we will need to fix this.
    ReflectMethod* pRM = (ReflectMethod*) refThisUNSAFE->GetData();
    pMeth = pRM->pMethod;
    if (!pMeth) {
        _ASSERTE(!"MethodDesc Not Found");
        FATAL_EE_ERROR();
    }

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refSig);

        // Put into a basic block so SigFormat is destroyed before the throw
    {
        SigFormat sigFmt(pMeth, pRM->typeHnd);
        refSig = sigFmt.GetString();
    }
    if (!refSig) {
        _ASSERTE(!"Unable to Create String");
        FATAL_EE_ERROR();
    }
    _ASSERTE(refSig);

    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refSig);
}
FCIMPLEND

// GetPropInfoToString
// This method will return the string representation of the information in PropInfo
FCIMPL1(Object*, COMMember::GetPropInfoToString, ReflectTokenBaseObject* refThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    if (refThisUNSAFE == NULL) 
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    STRINGREF       refSig = NULL;

#ifdef _DEBUG
	ReflectClass* pRC = (ReflectClass*) refThisUNSAFE->GetReflClass();
#endif
    _ASSERTE(pRC);
    ReflectProperty* pProp = (ReflectProperty*) refThisUNSAFE->GetData();

    if (!pProp) {
        _ASSERTE(!"Reflect Property Not Found");
        FATAL_EE_ERROR();
    }

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refSig);

    // Put into a basic block so SigFormat is destroyed before the throw
    {
        PropertySigFormat sigFmt(*(MetaSig *)pProp->pSignature,pProp->szName);
        refSig = sigFmt.GetString();
    }
    if (!refSig) {
        _ASSERTE(!"Unable to Create String");
        FATAL_EE_ERROR();
    }
    _ASSERTE(refSig);

    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refSig);
}
FCIMPLEND


// GetEventInfoToString
// This method will return the string representation of the information in EventInfo
FCIMPL1(Object*, COMMember::GetEventInfoToString, ReflectBaseObject* refThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    if (refThisUNSAFE == NULL) 
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    STRINGREF       refString = NULL;

    // Get the event descr
    ReflectEvent* pRE = (ReflectEvent*) refThisUNSAFE->GetData();

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refString);

    // Locate the signature of the Add method.
    ExpandSig *pSig = pRE->pAdd->GetSig();
    void *pEnum;

    // The first parameter to Add will be the type of the event (a delegate).
    pSig->Reset(&pEnum);
    TypeHandle th = pSig->NextArgExpanded(&pEnum);
    EEClass *pClass = th.GetClass();
    _ASSERTE(pClass);
    _ASSERTE(pClass->IsAnyDelegateClass());

    DefineFullyQualifiedNameForClass();
    LPUTF8 szClass = GetFullyQualifiedNameForClass(pClass);

    // Allocate a temporary buffer for the formatted string.
    size_t uLength = strlen(szClass) + 1 + strlen(pRE->szName) + 1;
    LPUTF8 szToString = (LPUTF8)_alloca(uLength);
    sprintf(szToString, "%s %s", szClass, pRE->szName);

    refString = COMString::NewString(szToString);
    if (!refString) {
        _ASSERTE(!"Unable to Create String");
        FATAL_EE_ERROR();
    }
    _ASSERTE(refString);

    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refString);
}
FCIMPLEND


// GetMethodName
// This method will return the name of a Method
FCIMPL1(Object*, COMMember::GetMethodName, ReflectBaseObject* refThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    STRINGREF   refName = NULL;      // Return value

    if (refThisUNSAFE == NULL) 
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    ReflectMethod* pRM = (ReflectMethod*) refThisUNSAFE->GetData();

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refName);

    MethodDesc*     pMeth;

    // Get the method Descr  (this should not fail)
    //  NOTE: both a constructor and a method are represented by a MetodDesc.
    //      If this ever changes we will need to fix this.
    if (!pRM)
        COMPlusThrow(kNullReferenceException);
    pMeth = pRM->pMethod;
    _ASSERTE(pMeth);

    // Convert the name to a managed string
    refName = COMString::NewString(pMeth->GetName());
    _ASSERTE(refName);

    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refName);
}
FCIMPLEND

// GetEventName
// This method will return the name of a Event
FCIMPL1(Object*, COMMember::GetEventName, ReflectTokenBaseObject* refThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    if (refThisUNSAFE == NULL) 
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    STRINGREF       refName = NULL;

    ReflectEvent* pEvent    = (ReflectEvent*) refThisUNSAFE->GetData();
#ifdef _DEBUG
    ReflectClass* pRC       = (ReflectClass*) refThisUNSAFE->GetReflClass();
#endif
	_ASSERTE(pRC);

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refName);

    // Convert the name to a managed string
    refName = COMString::NewString(pEvent->szName);
    _ASSERTE(refName);

    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refName);
}
FCIMPLEND


// GetPropName
// This method will return the name of a Property
FCIMPL1(Object*, COMMember::GetPropName, ReflectTokenBaseObject* refThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    if (refThisUNSAFE == NULL) 
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    STRINGREF       refName = NULL;

    ReflectProperty* pProp  = (ReflectProperty*) refThisUNSAFE->GetData();
#ifdef _DEBUG
    ReflectClass*    pRC    = (ReflectClass*)    refThisUNSAFE->GetReflClass();
#endif
	_ASSERTE(pRC);

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refName);

    // Convert the name to a managed string
    refName = COMString::NewString(pProp->szName);
    _ASSERTE(refName);

    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refName);
}
FCIMPLEND

// GetPropType
// This method will return type of a property
FCIMPL1(Object*, COMMember::GetPropType, ReflectTokenBaseObject* refThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    if (refThisUNSAFE == NULL)
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    OBJECTREF o = NULL;

    ReflectProperty* pProp = (ReflectProperty*) refThisUNSAFE->GetData();
#ifdef _DEBUG
    ReflectClass*    pRC   = (ReflectClass*)    refThisUNSAFE->GetReflClass();
#endif
	_ASSERTE(pRC);

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, o);

    TypeHandle t = pProp->pSignature->GetReturnTypeHandle();
    // Ignore Return because noting has a transparent proxy property
    o = t.CreateClassObj();

    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(o);
}
FCIMPLEND

// GetReturnType
// This method checks gets the signature for a method and returns
//  a class which represents the return type of that class.
FCIMPL1(Object*, COMMember::GetReturnType, ReflectBaseObject* refThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    TypeHandle typeHnd;

    if (refThisUNSAFE == NULL) 
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    OBJECTREF ret = NULL;

    ReflectMethod*  pRM     = (ReflectMethod*) refThisUNSAFE->GetData();
    MethodDesc* pMeth = pRM->pMethod;
    _ASSERTE(pMeth);

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, ret);

    TypeHandle varTypes;
    if (pRM->typeHnd.IsArray()) 
        varTypes = pRM->typeHnd.AsTypeDesc()->GetTypeParam();
    
    PCCOR_SIGNATURE pSignature; // The signature of the found method
    DWORD       cSignature;
    pMeth->GetSig(&pSignature,&cSignature);
    MetaSig sig(pSignature, pMeth->GetModule());

    OBJECTREF Throwable = NULL;
    GCPROTECT_BEGIN(Throwable);
    typeHnd = sig.GetReturnProps().GetTypeHandle(sig.GetModule(), &Throwable, FALSE, FALSE, &varTypes);

    if (typeHnd.IsNull()) {
        if (Throwable == NULL)
            COMPlusThrow(kTypeLoadException);
        COMPlusThrow(Throwable);
    }
    GCPROTECT_END();

    // ignore because transparent proxy is not a return type
    ret = typeHnd.CreateClassObj();

    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(ret);
}
FCIMPLEND

/*=============================================================================
** GetParameterTypes
**
** This routine returns an array of Parameters
**
** args->refThis: this Field object reference
**/
FCIMPL1(Object*, COMMember::GetParameterTypes, ReflectBaseObject* refThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    REFLECTBASEREF  refThis     = (REFLECTBASEREF) refThisUNSAFE;
    PTRARRAYREF     refRetVal   = NULL;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refThis);

    if (refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    refRetVal = g_pInvokeUtil->CreateParameterArray(&refThis);

    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(refRetVal);
}
FCIMPLEND

/*=============================================================================
** GetFieldName
**
** The name of this field is returned
**
** args->refThis: this Field object reference
**/
FCIMPL1(Object*, COMMember::GetFieldName, ReflectBaseObject* refThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    REFLECTBASEREF  refThis     = (REFLECTBASEREF) refThisUNSAFE;
    STRINGREF       refName;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refThis);

    if (refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    FieldDesc*      pField;

    // Get the method Descr  (this should not fail)
    //  NOTE: both a constructor and a method are represented by a MetodDesc.
    //      If this ever changes we will need to fix this.
    ReflectField* pRF = (ReflectField*) refThis->GetData();
    pField = pRF->pField;
    _ASSERTE(pField);

    // Convert the name to a managed string
    refName = COMString::NewString(pField->GetName());
    _ASSERTE(refName);

    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refName);
}
FCIMPLEND

/*=============================================================================
** GetDeclaringClass
**
** Returns the class which declared this member. This may be a
** parent of the Class that get(Member)() was called on.  Members are always
** associated with a Class.  You cannot invoke a method/ctor from one class on
** another class even if they have the same signatures.  It is possible to do
** this with Delegates.
**
** args->refThis: this object reference
**/

FCIMPL1(LPVOID, COMMember::GetDeclaringClass, ReflectBaseObject* refThis)
{
    THROWSCOMPLUSEXCEPTION();

    LPVOID rv = NULL;

    if (refThis == NULL) 
    {
         HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, rv);
        COMPlusThrow(kNullReferenceException, L"NullReference_This");
         HELPER_METHOD_FRAME_END();
    }

    // Assign the return value
    ReflectMethod* pRM = (ReflectMethod*) refThis->GetData();
    _ASSERTE(pRM);

    // return NULL for global member
    if (pRM->pMethod->GetClass()->GetCl() != COR_GLOBAL_PARENT_TOKEN)
    {
        HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, rv);
        *((OBJECTREF*)&rv) = pRM->typeHnd.CreateClassObj();
        HELPER_METHOD_FRAME_END();
    }

    return rv;
}
FCIMPLEND

// And the field version of the same thing...
FCIMPL1(Object*, COMMember::GetFieldDeclaringClass, ReflectBaseObject* refThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    REFLECTBASEREF refThis = (REFLECTBASEREF) refThisUNSAFE;
    OBJECTREF      refRet  = NULL;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refThis);

    if (refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    FieldDesc*  pField;
    EEClass*    pVMC;

    // Assign the return value
    ReflectField* pRF = (ReflectField*) refThis->GetData();
    pField = pRF->pField;
    pVMC = pField->GetEnclosingClass();
    _ASSERTE(pVMC);

    // return NULL for global field
    if (pVMC->GetCl() != COR_GLOBAL_PARENT_TOKEN)
    {
        refRet = pVMC->GetExposedClassObject();
    }

    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refRet);
}
FCIMPLEND

// GetEventDeclaringClass
// This is the event based version
FCIMPL1(Object*, COMMember::GetEventDeclaringClass, ReflectTokenBaseObject* refThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    REFLECTTOKENBASEREF refThis = (REFLECTTOKENBASEREF) refThisUNSAFE;
    OBJECTREF           refRet  = NULL;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refThis);

    if (refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    ReflectEvent* pEvent = (ReflectEvent*) refThis->GetData();
    refRet = pEvent->pDeclCls->GetExposedClassObject();

    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refRet);
}
FCIMPLEND

// GetPropDeclaringClass
// This method returns the declaring class for the property
FCIMPL1(Object*, COMMember::GetPropDeclaringClass, ReflectBaseObject* refThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    REFLECTBASEREF refThis = (REFLECTBASEREF) refThisUNSAFE;
    OBJECTREF      refRet  = NULL;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refThis);

    if (refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    ReflectProperty* pProp = (ReflectProperty*) refThis->GetData();
    refRet = pProp->pDeclCls->GetExposedClassObject();

    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refRet);
}
FCIMPLEND

// GetReflectedClass
// This method will return the Reflected class for all REFLECTBASEREF types.
FCIMPL2(Object*, COMMember::GetReflectedClass, ReflectBaseObject* refThisUNSAFE, BYTE returnGlobalClass)
{
    THROWSCOMPLUSEXCEPTION();

    REFLECTBASEREF refThis = (REFLECTBASEREF) refThisUNSAFE;
    OBJECTREF refRet  = NULL;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refThis);

    if (refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    ReflectClass* pRC = (ReflectClass*) refThis->GetReflClass();

    // Global functions will return a null class.
    if (pRC!=NULL)
    {
        if (returnGlobalClass || pRC->GetClass()->GetCl() != COR_GLOBAL_PARENT_TOKEN)
        {
            refRet = pRC->GetClassObject();
        }
    }

    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refRet);
}
FCIMPLEND

/*=============================================================================
** GetFieldSignature
**
** Returns the signature of the field.
**
** args->refThis: this object reference
**/
FCIMPL1(Object*, COMMember::GetFieldSignature, ReflectBaseObject* refThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    REFLECTBASEREF  refThis = (REFLECTBASEREF) refThisUNSAFE;
    STRINGREF       refSig  = NULL;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refThis);

    if (refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    FieldDesc*      pField;

    // Get the field descr
    ReflectField* pRF = (ReflectField*) refThis->GetData();
    pField = pRF->pField;
    _ASSERTE(pField);

    // Put into a basic block so SigFormat is destroyed before the throw
    {
        FieldSigFormat sigFmt(pField);
        refSig = sigFmt.GetString();
    }
    if (!refSig) {
        _ASSERTE(!"Unable to Create String");
        FATAL_EE_ERROR();
    }
    _ASSERTE(refSig);

    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refSig);
}
FCIMPLEND

// GetAttributeFlags
// This method will return the attribute flag for an Member.  The 
//  attribute flag is defined in the meta data.
FCIMPL1(INT32, COMMember::GetAttributeFlags, ReflectBaseObject* refThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    REFLECTBASEREF  refThis = (REFLECTBASEREF) refThisUNSAFE;
    DWORD           attr    = 0;

    HELPER_METHOD_FRAME_BEGIN_RET_1(refThis);

    if (refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    void*     p;
    EEClass* vm;
    mdToken mb;

    // Get the method Descr  (this should not fail)
    p = refThis->GetData();
    MethodTable* thisClass = refThis->GetMethodTable();
    if (thisClass == g_pRefUtil->GetClass(RC_Method) || 
        thisClass == g_pRefUtil->GetClass(RC_Ctor)) {
        MethodDesc* pMeth = ((ReflectMethod*) p)->pMethod;
        mb = pMeth->GetMemberDef();
        vm = pMeth->GetClass();
        _ASSERTE(TypeFromToken(mb) == mdtMethodDef);
        attr = pMeth->GetAttrs();
    }
    else if (thisClass == g_pRefUtil->GetClass(RC_Field)) {
        FieldDesc* pFld = ((ReflectField*) p)->pField;
        mb = pFld->GetMemberDef();
        vm = pFld->GetEnclosingClass();
        attr = vm->GetMDImport()->GetFieldDefProps(mb);
    }
    else {
        _ASSERTE(!"Illegal Object call to GetAttributeFlags");
        FATAL_EE_ERROR();
    }

    HELPER_METHOD_FRAME_END();
    return (INT32) attr;
}
FCIMPLEND

// GetCallingConvention
// Return the calling convention
FCIMPL1(INT32, COMMember::GetCallingConvention, ReflectBaseObject* refThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    REFLECTBASEREF  refThis = (REFLECTBASEREF) refThisUNSAFE;
    INT32           retCall;

    HELPER_METHOD_FRAME_BEGIN_RET_1(refThis);

    if (refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    // Get the method Descr  (this should not fail)
    //  NOTE: both a constructor and a method are represented by a MetodDesc.
    //      If this ever changes we will need to fix this.
    ReflectMethod* pRM = (ReflectMethod*) refThis->GetData();
    if (!pRM->pSignature) {
        PCCOR_SIGNATURE pSignature;     // The signature of the found method
        DWORD       cSignature;
        pRM->pMethod->GetSig(&pSignature,&cSignature);
        pRM->pSignature = ExpandSig::GetReflectSig(pSignature,
                                pRM->pMethod->GetModule());
    }
    BYTE callConv = pRM->pSignature->GetCallingConventionInfo();

    // NOTE: These are defined in CallingConventions.cs.
    if ((callConv & IMAGE_CEE_CS_CALLCONV_MASK) == IMAGE_CEE_CS_CALLCONV_VARARG)
        retCall = CALLCONV_VarArgs;
    else
        retCall = CALLCONV_Standard;

    if ((callConv & IMAGE_CEE_CS_CALLCONV_HASTHIS) != 0)
        retCall |= CALLCONV_HasThis;
    if ((callConv & IMAGE_CEE_CS_CALLCONV_EXPLICITTHIS) != 0)
        retCall |= CALLCONV_ExplicitThis;

    HELPER_METHOD_FRAME_END();
    return retCall;
}
FCIMPLEND

// GetMethodImplFlags
// Return the method impl flags
FCIMPL1(INT32, COMMember::GetMethodImplFlags, ReflectBaseObject* refThisUNSAFE)
{
    void*     p;
    mdToken mb;
    ULONG   RVA;
    DWORD   ImplFlags;

    THROWSCOMPLUSEXCEPTION();

    REFLECTBASEREF  refThis = (REFLECTBASEREF) refThisUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_1(refThis);

    if (refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    p = refThis->GetData();
    MethodDesc* pMeth = ((ReflectMethod*) p)->pMethod;
    Module* pModule = pMeth->GetModule();
    mb = pMeth->GetMemberDef();

    pModule->GetMDImport()->GetMethodImplProps(mb, &RVA, &ImplFlags);

    HELPER_METHOD_FRAME_END();
    return ImplFlags;
}
FCIMPLEND


// GetEventAttributeFlags
// This method will return the attribute flag for an Event. 
//  The attribute flag is defined in the meta data.
FCIMPL1(INT32, COMMember::GetEventAttributeFlags, ReflectTokenBaseObject* refThisUNSAFE);
{
    THROWSCOMPLUSEXCEPTION();

    REFLECTTOKENBASEREF  refThis = (REFLECTTOKENBASEREF) refThisUNSAFE;
    ReflectEvent*        pEvents;

    HELPER_METHOD_FRAME_BEGIN_RET_1(refThis);

    if (refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    pEvents = (ReflectEvent*) refThis->GetData();

    HELPER_METHOD_FRAME_END();
    return pEvents->attr;
}
FCIMPLEND

// GetPropAttributeFlags
// This method will return the attribute flag for the property. 
//  The attribute flag is defined in the meta data.
FCIMPL1(INT32, COMMember::GetPropAttributeFlags, ReflectTokenBaseObject* refThisUNSAFE);
{
    THROWSCOMPLUSEXCEPTION();

    REFLECTTOKENBASEREF  refThis = (REFLECTTOKENBASEREF) refThisUNSAFE;
    ReflectProperty*     pProp;

    HELPER_METHOD_FRAME_BEGIN_RET_1(refThis);

    if (refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    pProp = (ReflectProperty*) refThis->GetData();

    HELPER_METHOD_FRAME_END();
    return pProp->attr;
}
FCIMPLEND

void COMMember::CanAccess(
            MethodDesc* pMeth, RefSecContext *pSCtx, 
            bool checkSkipVer, bool verifyAccess, 
            bool thisIsImposedSecurity, bool knowForSureImposedSecurityState)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(!thisIsImposedSecurity  || knowForSureImposedSecurityState);

    BOOL fRet = FALSE;
    BOOL isEveryoneFullyTrusted = FALSE;

    if (thisIsImposedSecurity || !knowForSureImposedSecurityState)
    {
        isEveryoneFullyTrusted = ApplicationSecurityDescriptor::
                                        AllDomainsOnStackFullyTrusted();

        // If all assemblies in the domain are fully trusted then we are not 
        // going to do any security checks anyway..
        if (thisIsImposedSecurity && isEveryoneFullyTrusted)
            return;
    }

    struct _gc
    {
        OBJECTREF refClassNonCasDemands;
        OBJECTREF refClassCasDemands;
        OBJECTREF refMethodNonCasDemands;
        OBJECTREF refMethodCasDemands;
        OBJECTREF refThrowable;
    } gc;
    ZeroMemory(&gc, sizeof(gc));

    GCPROTECT_BEGIN(gc);

    if (pMeth->RequiresLinktimeCheck())
    {
        // Fetch link demand sets from all the places in metadata where we might
        // find them (class and method). These might be split into CAS and non-CAS
        // sets as well.
        Security::RetrieveLinktimeDemands(pMeth,
                                          &gc.refClassCasDemands,
                                          &gc.refClassNonCasDemands,
                                          &gc.refMethodCasDemands,
                                          &gc.refMethodNonCasDemands);

        if (gc.refClassCasDemands == NULL && gc.refClassNonCasDemands == NULL &&
            gc.refMethodCasDemands == NULL && gc.refMethodNonCasDemands == NULL &&
            isEveryoneFullyTrusted)
        {
            // All code access security demands will pass anyway.
            fRet = TRUE;
            goto Exit1;
        }
    }

    if (verifyAccess)
        InvokeUtil::CheckAccess(pSCtx,
                                pMeth->GetAttrs(),
                                pMeth->GetMethodTable(),
                                REFSEC_CHECK_MEMBERACCESS|REFSEC_THROW_MEMBERACCESS);

    if (pMeth->RequiresLinktimeCheck()) {

            
        // The following logic turns link demands on the target method into full
        // stack walks in order to close security holes in poorly written
        // reflection users.

        _ASSERTE(pMeth);
        _ASSERTE(pSCtx);

        if (pSCtx->GetCallerMethod())
        { 
            // Check for untrusted caller
            // It is possible that wrappers like VBHelper libraries that are
            // fully trusted, make calls to public methods that do not have
            // safe for Untrusted caller custom attribute set.
            // Like all other link demand that gets transformed to a full stack 
            // walk for reflection, calls to public methods also gets 
            // converted to full stack walk

            if (!Security::DoUntrustedCallerChecks(
                pSCtx->GetCallerMethod()->GetAssembly(), pMeth,
                &gc.refThrowable, TRUE))
                COMPlusThrow(gc.refThrowable);
        }

        if (gc.refClassCasDemands != NULL)
            COMCodeAccessSecurityEngine::DemandSet(gc.refClassCasDemands);

        if (gc.refMethodCasDemands != NULL)
            COMCodeAccessSecurityEngine::DemandSet(gc.refMethodCasDemands);

        // Non-CAS demands are not applied against a grant
        // set, they're standalone.
        if (gc.refClassNonCasDemands != NULL)
            Security::CheckNonCasDemand(&gc.refClassNonCasDemands);

        if (gc.refMethodNonCasDemands != NULL)
            Security::CheckNonCasDemand(&gc.refMethodNonCasDemands);

        // We perform automatic linktime checks for UnmanagedCode in three cases:
        //   o  P/Invoke calls.
        //   o  Calls through an interface that have a suppress runtime check
        //      attribute on them (these are almost certainly interop calls).
        //   o  Interop calls made through method impls.
        if (pMeth->IsNDirect() ||
            (pMeth->GetClass()->IsInterface() &&
             pMeth->GetClass()->GetMDImport()->GetCustomAttributeByName(pMeth->GetClass()->GetCl(),
                                                                        COR_SUPPRESS_UNMANAGED_CODE_CHECK_ATTRIBUTE_ANSI,
                                                                        NULL,
                                                                        NULL) == S_OK) ||
            (pMeth->IsComPlusCall() && !pMeth->IsInterface()))
            COMCodeAccessSecurityEngine::SpecialDemand(SECURITY_UNMANAGED_CODE);
    }

Exit1:;

    GCPROTECT_END();

    if (!fRet)
    {
        if (checkSkipVer)
            COMCodeAccessSecurityEngine::SpecialDemand(SECURITY_SKIP_VER);
    }
}


void COMMember::CanAccessField(ReflectField* pRF, RefSecContext *pCtx)
{
    THROWSCOMPLUSEXCEPTION();

    // Check whether the field itself is well formed, i.e. if the field type is
    // accessible to the field's enclosing type. If not, we'll throw a field
    // access exception to stop the field being used.
    EEClass *pEnclosingClass = pRF->pField->GetEnclosingClass();

    EEClass *pFieldClass = GetUnderlyingClass(&pRF->thField);
    if (pFieldClass && !ClassLoader::CanAccessClass(pEnclosingClass,
                                                    pEnclosingClass->GetAssembly(),
                                                    pFieldClass,
                                                    pFieldClass->GetAssembly()))
        COMPlusThrow(kFieldAccessException);

    // Perform the normal access check (caller vs field).
    if (!pRF->pField->IsPublic() || !pRF->pField->GetEnclosingClass()->IsExternallyVisible())
        InvokeUtil::CheckAccess(pCtx,
                                pRF->dwAttr,
                                pRF->pField->GetMethodTableOfEnclosingClass(),
                                REFSEC_CHECK_MEMBERACCESS|REFSEC_THROW_FIELDACCESS);
}

// For a given type handle, return the EEClass which should be checked for
// accessibility to that type. Return NULL if the type is always accessible.
EEClass *COMMember::GetUnderlyingClass(TypeHandle *pTH)
{
    EEClass *pRetClass = NULL;

    if (pTH->IsTypeDesc()) {
        // Need to special case non-simple types.
        TypeDesc *pTypeDesc = pTH->AsTypeDesc();
        switch (pTypeDesc->GetNormCorElementType()) {
        case ELEMENT_TYPE_PTR:
        case ELEMENT_TYPE_BYREF:
        case ELEMENT_TYPE_ARRAY:
        case ELEMENT_TYPE_SZARRAY:
            // Parameterized types with a single base type. Check access to that
            // type.
            if (pTypeDesc->GetMethodTable())
                pRetClass = pTypeDesc->GetMethodTable()->GetClass();
            else {
                TypeHandle hArgType = pTypeDesc->GetTypeParam();
                pRetClass = GetUnderlyingClass(&hArgType);
            }
            break;
        case ELEMENT_TYPE_FNPTR:
            // No access restrictions on function pointers.
            break;
        default:
            _ASSERTE(!"NYI: Need to deal with new parameterized types as they are added.");
        }
    } else
        pRetClass = pTH->AsClass();

    return pRetClass;
}


// Hack to require full trust for some methods that perform stack walks and
// therefore could cause a problem if called through reflection indirected
// through a trusted wrapper.  
bool IsDangerousMethod(MethodDesc *pMD)
{
    static MethodTable *s_pTypeAppDomain = NULL;
    static MethodTable *s_pTypeAssembly = NULL;
    static MethodTable *s_pTypeAssemblyBuilder = NULL;
    static MethodTable *s_pTypeMethodRental = NULL;
    static MethodTable *s_pTypeIsolatedStorageFile = NULL;
    static MethodTable *s_pTypeMethodBase = NULL;
    static MethodTable *s_pTypeRuntimeMethodInfo = NULL;
    static MethodTable *s_pTypeConstructorInfo = NULL;
    static MethodTable *s_pTypeRuntimeConstructorInfo = NULL;
    static MethodTable *s_pTypeType = NULL;
    static MethodTable *s_pTypeRuntimeType = NULL;
    static MethodTable *s_pTypeFieldInfo = NULL;
    static MethodTable *s_pTypeRuntimeFieldInfo = NULL;
    static MethodTable *s_pTypeEventInfo = NULL;
    static MethodTable *s_pTypeRuntimeEventInfo = NULL;
    static MethodTable *s_pTypePropertyInfo = NULL;
    static MethodTable *s_pTypeRuntimePropertyInfo = NULL;
    static MethodTable *s_pTypeResourceManager = NULL;
    static MethodTable *s_pTypeActivator = NULL;

    // One time only initialization. Check relies on write ordering.
    if (s_pTypeActivator == NULL) {
        s_pTypeAppDomain = g_Mscorlib.FetchClass(CLASS__APP_DOMAIN);
        s_pTypeAssembly = g_Mscorlib.FetchClass(CLASS__ASSEMBLY);
        s_pTypeAssemblyBuilder = g_Mscorlib.FetchClass(CLASS__ASSEMBLY_BUILDER);
        s_pTypeMethodRental = g_Mscorlib.FetchClass(CLASS__METHOD_RENTAL);
        s_pTypeIsolatedStorageFile = g_Mscorlib.FetchClass(CLASS__ISS_STORE_FILE);
        s_pTypeMethodBase = g_Mscorlib.FetchClass(CLASS__METHOD_BASE);
        s_pTypeRuntimeMethodInfo = g_Mscorlib.FetchClass(CLASS__METHOD);
        s_pTypeConstructorInfo = g_Mscorlib.FetchClass(CLASS__CONSTRUCTOR_INFO);
        s_pTypeRuntimeConstructorInfo = g_Mscorlib.FetchClass(CLASS__CONSTRUCTOR);
        s_pTypeType = g_Mscorlib.FetchClass(CLASS__TYPE);
        s_pTypeRuntimeType = g_Mscorlib.FetchClass(CLASS__CLASS);
        s_pTypeFieldInfo = g_Mscorlib.FetchClass(CLASS__FIELD_INFO);
        s_pTypeRuntimeFieldInfo = g_Mscorlib.FetchClass(CLASS__FIELD);
        s_pTypeEventInfo = g_Mscorlib.FetchClass(CLASS__EVENT_INFO);
        s_pTypeRuntimeEventInfo = g_Mscorlib.FetchClass(CLASS__EVENT);
        s_pTypePropertyInfo = g_Mscorlib.FetchClass(CLASS__PROPERTY_INFO);
        s_pTypeRuntimePropertyInfo = g_Mscorlib.FetchClass(CLASS__PROPERTY);
        s_pTypeResourceManager = g_Mscorlib.FetchClass(CLASS__RESOURCE_MANAGER);
        s_pTypeActivator = g_Mscorlib.FetchClass(CLASS__ACTIVATOR);
    }
    _ASSERTE(s_pTypeAppDomain &&
             s_pTypeAssembly &&
             s_pTypeAssemblyBuilder &&
             s_pTypeMethodRental &&
             s_pTypeIsolatedStorageFile &&
             s_pTypeMethodBase &&
             s_pTypeRuntimeMethodInfo &&
             s_pTypeConstructorInfo &&
             s_pTypeRuntimeConstructorInfo &&
             s_pTypeType &&
             s_pTypeRuntimeType &&
             s_pTypeFieldInfo &&
             s_pTypeRuntimeFieldInfo &&
             s_pTypeEventInfo &&
             s_pTypeRuntimeEventInfo &&
             s_pTypePropertyInfo &&
             s_pTypeRuntimePropertyInfo &&
             s_pTypeResourceManager &&
             s_pTypeActivator);

    MethodTable *pMT = pMD->GetMethodTable();

    return pMT == s_pTypeAppDomain ||
           pMT == s_pTypeAssembly ||
           pMT == s_pTypeAssemblyBuilder ||
           pMT == s_pTypeMethodRental ||
           pMT == s_pTypeIsolatedStorageFile ||
           pMT == s_pTypeMethodBase ||
           pMT == s_pTypeRuntimeMethodInfo ||
           pMT == s_pTypeConstructorInfo ||
           pMT == s_pTypeRuntimeConstructorInfo ||
           pMT == s_pTypeType ||
           pMT == s_pTypeRuntimeType ||
           pMT == s_pTypeFieldInfo ||
           pMT == s_pTypeRuntimeFieldInfo ||
           pMT == s_pTypeEventInfo ||
           pMT == s_pTypeRuntimeEventInfo ||
           pMT == s_pTypePropertyInfo ||
           pMT == s_pTypeRuntimePropertyInfo ||
           pMT == s_pTypeResourceManager ||
           pMT == s_pTypeActivator ||
           pMT->GetClass()->IsAnyDelegateClass() ||
           pMT->GetClass()->IsAnyDelegateExact();
}


/*=============================================================================
** InvokeMethod
**
** This routine will invoke the method on an object.  It will verify that
**  the arguments passed are correct
**
** refThis: this object reference
**/

/*
struct _InvokeMethodArgs        {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis); 
        DECLARE_ECALL_I4_ARG(BOOL, verifyAccess);
        DECLARE_ECALL_OBJECTREF_ARG(ASSEMBLYREF, caller);
        DECLARE_ECALL_I4_ARG(BOOL, isBinderDefault);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, locale);
        DECLARE_ECALL_OBJECTREF_ARG(PTRARRAYREF, objs);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, binder);
        DECLARE_ECALL_I4_ARG(INT32, attrs); 
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, target);
    };
*/

FCIMPL9(Object*, COMMember::InvokeMethod, ReflectBaseObject* refThis,
                                                                        Object* target,
                                                                        INT32 attrs,
                                                                        Object* binder,
                                                                        PTRArray* objs,
                                                                        Object* locale,
                                                                        BYTE isBinderDefault,
                                                                        AssemblyBaseObject* caller,
                                                                        BYTE verifyAccess
                                                                        )
{
    OBJECTREF retVal = NULL;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, retVal);
    REQUIRE_COOPERATIVE_GC();

    retVal = COMMember::InvokeMethod_Internal(
                                                                                    (REFLECTBASEREF) refThis,
                                                                                    (OBJECTREF) target,
                                                                                    attrs,
                                                                                    (OBJECTREF) binder,
                                                                                    (PTRARRAYREF) objs,
                                                                                    (OBJECTREF) locale,
                                                                                    isBinderDefault,
                                                                                    (ASSEMBLYREF) caller,
                                                                                    verifyAccess
                                                                                    );

    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(retVal);
}
FCIMPLEND

OBJECTREF COMMember::InvokeMethod_Internal(
                                                                        REFLECTBASEREF refThisUNSAFE,
                                                                        OBJECTREF targetUNSAFE,
                                                                        INT32 attrs,
                                                                        OBJECTREF binderUNSAFE,
                                                                        PTRARRAYREF objsUNSAFE,
                                                                        OBJECTREF localeUNSAFE,
                                                                        BOOL isBinderDefault,
                                                                        ASSEMBLYREF callerUNSAFE,
                                                                        BOOL verifyAccess
                                                                        )
{
    THROWSCOMPLUSEXCEPTION();
    REQUIRE_COOPERATIVE_GC();

    if (refThisUNSAFE == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    OBJECTREF          rv = 0;
    MethodDesc*     pMeth;
    UINT            argCnt;
    EEClass*        eeClass;
    int             thisPtr;
    ARG_SLOT           ret = 0;
    EEClass*        pEECValue = 0;
//    void*           pRetValueClass = 0;

    struct _gc
    {
        REFLECTBASEREF  refThis;
        OBJECTREF       target;
        OBJECTREF       binder;
        PTRARRAYREF   objs;
        OBJECTREF       locale;
        ASSEMBLYREF   caller;
    } gc;

    gc.refThis  = (REFLECTBASEREF)  refThisUNSAFE;
    gc.target   = (OBJECTREF)       targetUNSAFE;
    gc.binder  = (OBJECTREF)       binderUNSAFE;
    gc.objs     = (PTRARRAYREF) objsUNSAFE;
    gc.locale   = (OBJECTREF)   localeUNSAFE;
    gc.caller    = (ASSEMBLYREF) callerUNSAFE;

    GCPROTECT_BEGIN(gc);

    // Setup the Method
    ReflectMethod* pRM = (ReflectMethod*) gc.refThis->GetData();
    _ASSERTE(pRM);
    pMeth = pRM->pMethod;
    _ASSERTE(pMeth);
    TypeHandle methodTH;
    if (pRM->typeHnd.IsArray()) 
        methodTH = pRM->typeHnd;
    eeClass = pMeth->GetClass();
    //WARNING: for array this is not the "real" class but rather the element type. However that is what we need to
    //         do the checks later on. 

    _ASSERTE(eeClass);

    DWORD attr = pRM->attrs;
    ExpandSig* mSig = pRM->GetSig();

    if (mSig->IsVarArg()) 
        COMPlusThrow(kNotSupportedException, IDS_EE_VARARG_NOT_SUPPORTED);

    // Get the number of args on this element
    argCnt = (int) mSig->NumFixedArgs();
    thisPtr = (IsMdStatic(attr)) ? 0 : 1;

    _ASSERTE(!(IsMdStatic(attr) && IsMdVirtual(attr)) && "A method can't be static and virtual.  How did this happen?");

    DWORD dwFlags = pRM->dwFlags;
    if (!(dwFlags & RM_ATTR_INITTED))
    {
        // First work with the local, to prevent race conditions

        // Is this a call to a potentially dangerous method? (If so, we're going
        // to demand additional permission).
        if (IsDangerousMethod(pMeth))
            dwFlags |= RM_ATTR_RISKY_METHOD;

        // Is something attempting to invoke a .ctor directly?
        if (pMeth->IsCtor())
            dwFlags |= RM_ATTR_IS_CTOR;

        // Is a security check required ?
        if (!IsMdPublic(attr) || pMeth->RequiresLinktimeCheck() || !eeClass->IsExternallyVisible() || dwFlags & RM_ATTR_IS_CTOR || gc.caller != NULL)
        {
            dwFlags |= RM_ATTR_NEED_SECURITY;
            if (pMeth->RequiresLinktimeCheck())
            {
                 // Check if we are imposing a security check on the caller though the callee didnt ask for it
                 // DONT USE THE GC REFS OTHER THAN FOR TESTING NULL !!!!!
                 OBJECTREF refClassCasDemands = NULL, refClassNonCasDemands = NULL, refMethodCasDemands = NULL, refMethodNonCasDemands = NULL;
                 Security::RetrieveLinktimeDemands(pMeth, &refClassCasDemands, &refClassNonCasDemands, &refMethodCasDemands, &refMethodNonCasDemands);
                 if (refClassCasDemands == NULL && refClassNonCasDemands == NULL && refMethodCasDemands == NULL && refMethodNonCasDemands == NULL)
                     dwFlags |= RM_ATTR_SECURITY_IMPOSED;

            }
        }

        // Do we need to get prestub address to find target ?
        if ((pMeth->IsComPlusCall() && gc.target != NULL 
             && (gc.target->GetMethodTable()->IsComObjectType()
                 || gc.target->GetMethodTable()->IsTransparentProxyType()
                 || gc.target->GetMethodTable()->IsCtxProxyType()))
            || pMeth->IsECall() || pMeth->IsIntercepted() || pMeth->IsRemotingIntercepted())
            dwFlags |= RM_ATTR_NEED_PRESTUB;

        if (pMeth->IsEnCMethod() && !pMeth->IsVirtual()) {
            dwFlags |= RM_ATTR_NEED_PRESTUB;
        }

        // Is this a virtual method ?
        if (pMeth->DontVirtualize() || pMeth->GetClass()->IsValueClass())
            dwFlags |= RM_ATTR_DONT_VIRTUALIZE;

        dwFlags |= RM_ATTR_INITTED;
        pRM->dwFlags = dwFlags;
    }

    // Check whether we're allowed to call certain dangerous methods.
    if (dwFlags & RM_ATTR_RISKY_METHOD)
        COMCodeAccessSecurityEngine::SpecialDemand(REFLECTION_MEMBER_ACCESS);

    // Make sure we're not invoking on a save-only dynamic assembly
    // Check reflected class
    ReflectClass* pRC = (ReflectClass*) gc.refThis->GetReflClass();
    if (pRC)
    {
        Assembly *pAssem = pRC->GetClass()->GetAssembly();
        if (pAssem->IsDynamic() && !pAssem->HasRunAccess())
        {
            COMPlusThrow(kNotSupportedException, L"NotSupported_DynamicAssemblyNoRunAccess");
        }
    }

    // Check declaring class
    Assembly *pAssem = eeClass->GetAssembly();
    if (pAssem->IsDynamic() && !pAssem->HasRunAccess())
    {
        COMPlusThrow(kNotSupportedException, L"NotSupported_DynamicAssemblyNoRunAccess");
    }

    TypeHandle targetTH;
    EEClass* targetClass = NULL;
    if (gc.target != NULL)
    {
        TypeHandle targetHandle = gc.target->GetTypeHandle();
        if (targetHandle.IsArray()) 
            targetTH = targetHandle; 
        targetClass = gc.target->GetTrueClass();;
    }

    VerifyType(gc.target, eeClass, targetClass, thisPtr, &pMeth, methodTH, targetTH);

    // Verify that the method isn't one of the special security methods that
    // alter the caller's stack (to add or detect a security frame object).
    // These must be early bound to work (since the direct caller is
    // reflection).
    if (IsMdRequireSecObject(attr))
    {
        COMPlusThrow(kArgumentException, L"Arg_InvalidSecurityInvoke");
    }

    // Verify that we have been provided the proper number of args
    if (!gc.objs) {
        if (argCnt > 0) {
            // The wrong number of arguments were passed
            COMPlusThrow(kTargetParameterCountException,L"Arg_ParmCnt");
        }
    }
    else {
        if (gc.objs->GetNumComponents() != argCnt) {
            // The wrong number of arguments were passed
            COMPlusThrow(kTargetParameterCountException,L"Arg_ParmCnt");
        }
    }

    // this security context will be used in cast checking as well
    RefSecContext sCtx;

    // Validate the method can be called by this caller
    

    if (dwFlags & RM_ATTR_NEED_SECURITY) 
    {
        DebuggerSecurityCodeMarkFrame __dbgSecFrame;
        
        if (gc.caller == NULL) {
			if (gc.target != NULL) {
				if (!gc.target->GetTypeHandle().IsTypeDesc()) 
					sCtx.SetClassOfInstance(targetClass);
			}

			CanAccess(pMeth, &sCtx, (dwFlags & RM_ATTR_IS_CTOR) != 0, verifyAccess != 0, (dwFlags & RM_ATTR_SECURITY_IMPOSED) != 0, TRUE);
        }
        else
        {
            // Calling assembly passed in explicitly.

            // Access check first.
            if (!pMeth->GetClass()->IsExternallyVisible() || !pMeth->IsPublic())
            {
                DWORD dwAttrs = pMeth->GetAttrs();
                if ((!IsMdAssem(dwAttrs) && !IsMdFamORAssem(dwAttrs)) ||
                    (gc.caller->GetAssembly() != pMeth->GetAssembly()))
                    
                    COMCodeAccessSecurityEngine::SpecialDemand(REFLECTION_MEMBER_ACCESS);
            }

            // Now check for security link time demands.
            if (pMeth->RequiresLinktimeCheck())
            {
                OBJECTREF refThrowable = NULL;
                GCPROTECT_BEGIN(refThrowable);
                if (!Security::LinktimeCheckMethod(gc.caller->GetAssembly(), pMeth, &refThrowable))
                    COMPlusThrow(refThrowable);
                GCPROTECT_END();
            }
        }
        
        __dbgSecFrame.Pop();
    }
    
    // We need to Prevent GC after we begin to build the stack. Validate the
    // arguments first.
    bool fDefaultBinding = (attrs & BINDER_ExactBinding) || gc.binder == NULL || isBinderDefault;
    void* pEnum;

    // Walk all of the args and allow the binder to change them
    mSig->Reset(&pEnum);
    for (int i=0;i< (int) argCnt;i++) {
        TypeHandle th = mSig->NextArgExpanded(&pEnum);

        // Check the caller has access to the arg type.
        EEClass *pArgClass = GetUnderlyingClass(&th);
        if (pArgClass && !pArgClass->IsExternallyVisible())
            InvokeUtil::CheckAccessType(&sCtx, pArgClass, REFSEC_THROW_MEMBERACCESS);

        // Finished with this arg if we're using default binding.
        if (fDefaultBinding)
            continue;

        // If we are the universal null then we can continue..
        if (gc.objs->m_Array[i] == 0)
            continue;

        DebuggerSecurityCodeMarkFrame __dbgSecFrame;
        
        // if the src cannot be cast to the dest type then 
        //  call the change type to let them fix it.
        TypeHandle srcTh = (gc.objs->m_Array[i])->GetTypeHandle();
        if (!srcTh.CanCastTo(th)) {
            OBJECTREF objRef = g_pInvokeUtil->ChangeType(gc.binder,gc.objs->m_Array[i],th,gc.locale);
            gc.objs->SetAt(i, objRef);
        }
        
        __dbgSecFrame.Pop();
    }

    // Establish the enumerator through the signature
    mSig->Reset(&pEnum);



    // Build the arguments.  This is built as a single array of arguments
    //  the this pointer is first 

    UINT   nNumSlots = mSig->NumVirtualFixedArgs(IsMdStatic(attr));
    
    ARG_SLOT* pNewArgs = (ARG_SLOT*) _alloca( nNumSlots*sizeof(ARG_SLOT) );
#ifdef _DEBUG
    memset(pNewArgs,0xEE,nNumSlots*sizeof(ARG_SLOT));
#endif

    ARG_SLOT* pTmpPtr = pNewArgs;

    for (int i = 0; i < (int)argCnt; i++) {
        TypeHandle th = mSig->NextArgExpanded(&pEnum);
        // This routine will verify that all types are correct.
        g_pInvokeUtil->CheckArg(th, &(gc.objs->m_Array[i]), &sCtx);
    }

#ifdef STRESS_HEAP
    if (g_pConfig->GetGCStressLevel() != 0)
        g_pGCHeap->StressHeap();
#endif
    
    // NO GC AFTER THIS POINT
    // actually this statement is not completely accurate. If an exception occurs a gc may happen
    // but we are going to dump the stack anyway and we do not need to protect anything.
    // But if anywhere between here and the method invocation we enter preemptive mode the stack
    // we are about to setup (pDst in the loop) may contain references to random parts of memory
    
    // Copy "this" pointer
    if (thisPtr) {
        //WARNING: because eeClass is not the real class for arrays and because array are reference types 
        //         we need to do the extra check if the eeClass happens to be a value type
        if (!eeClass->IsValueClass() || targetTH.IsArray())
            *pTmpPtr = ObjToArgSlot(gc.target);
        else {
            if (pMeth->IsVirtual())
                *pTmpPtr = ObjToArgSlot(gc.target);
            else
                *pTmpPtr = PtrToArgSlot(gc.target->UnBox());
        }
        pTmpPtr++;
    }

    OBJECTREF objRet = NULL;
    GCPROTECT_BEGIN(objRet);

    ARG_SLOT* retBuffPtr = NULL;
    
    // Take care of any return arguments
    if (mSig->IsRetBuffArg()) {
        // if we have the magic Value Class return, we need to allocate that class
        //  and place a pointer to it on the class stack.
        pEECValue = mSig->GetReturnClass();
        _ASSERTE(pEECValue->IsValueClass());
        MethodTable * mt = pEECValue->GetMethodTable();
        objRet = AllocateObject(mt);
        retBuffPtr = pTmpPtr++;
    }


    // copy args
    mSig->Reset(&pEnum);
    for (int i = 0 ; i < (int) argCnt ; i++) {
        TypeHandle th = mSig->NextArgExpanded(&pEnum);
        UINT cbSize = mSig->GetElemSize(th);
        BYTE* argDst;

        if(cbSize > sizeof(ARG_SLOT))
        {
            // argument address is placed in the slot
            argDst = (BYTE*) _alloca(cbSize);
            *pTmpPtr = PtrToArgSlot(argDst);
        }
        else
        {
            // the argument itself is placed in the slot
            argDst = ArgSlotEndianessFixup(pTmpPtr, cbSize);
        }

        g_pInvokeUtil->CopyArg(th, &(gc.objs->m_Array[i]), pTmpPtr, argDst);
        pTmpPtr ++;
    }

    // Call the method
    COMPLUS_TRY {
        if (mSig->IsRetBuffArg()) {
            _ASSERTE(objRet);
            *retBuffPtr = PtrToArgSlot(objRet->UnBox());
        }


        MetaSig threadSafeSig(*mSig);
        if (pMeth->IsInterface()) {
            // This should only happen for interop.
            _ASSERTE(gc.target->GetTrueClass()->GetMethodTable()->IsComObjectType());
             ret = pMeth->CallOnInterface(pNewArgs,&threadSafeSig); //, attr);
        } else
            ret = pMeth->Call(pNewArgs,&threadSafeSig); //, attr);
    } COMPLUS_CATCH {
        // If we get here we need to throw an TargetInvocationException
        OBJECTREF ppException = GETTHROWABLE();
        _ASSERTE(ppException);
        GCPROTECT_BEGIN(ppException);
        OBJECTREF except = g_pInvokeUtil->CreateTargetExcept(&ppException);
        COMPlusThrow(except);
        GCPROTECT_END();
    } COMPLUS_END_CATCH

    GCPROTECT_END();

    if (!pEECValue) {
        TypeHandle th = mSig->GetReturnTypeHandle();
        {
            objRet =  g_pInvokeUtil->CreateObject(th,ret);
        }
    }
  
    *((OBJECTREF *)&rv) = objRet;
    
    GCPROTECT_END();
    
    return rv;
}

// This method will verify the type relationship between the target and
//      the eeClass of the method we are trying to invoke.  It checks that for 
//      non static method, target is provided.  It also verifies that the target is
//      a subclass or implements the interface that this MethodInfo represents.  
//  We may update the MethodDesc in the case were we need to lookup the real
//      method implemented on the object for an interface.
void COMMember::VerifyType(OBJECTREF target, 
                           EEClass* eeClass, 
                           EEClass* targetClass, 
                           int thisPtr, 
                           MethodDesc** ppMeth, 
                           TypeHandle typeTH, 
                           TypeHandle targetTH)
{
    THROWSCOMPLUSEXCEPTION();

    // Make sure that the eeClass is defined if there is a this pointer.
    _ASSERTE(thisPtr == 0 || eeClass != 0);

    // Verify Static/Object relationship
    if (!target) {
        if (thisPtr == 1) {
            // Non-static without a target...
            COMPlusThrow(kTargetException,L"RFLCT.Targ_StatMethReqTarg");
        }
        return;
    }

    //  validate the class/method relationship
    if (thisPtr && (targetClass != eeClass || typeTH != targetTH)) {

        BOOL bCastOK = false;
        if(target->GetClass()->IsThunking())
        {
            // This could be a proxy and we may not have refined it to a type
            // it actually supports.
            bCastOK = CRemotingServices::CheckCast(target, eeClass);
        }

        if (!bCastOK)
        {
            // If this is an interface we need to find the real method
            if (eeClass->IsInterface()) {
                DWORD slot = 0;
                InterfaceInfo_t* pIFace = targetClass->FindInterface(eeClass->GetMethodTable());
                if (!pIFace) {
                    { 
                        // Interface not found for the object
                        COMPlusThrow(kTargetException,L"RFLCT.Targ_IFaceNotFound");
                    }
                }
                else
                {
                    slot = (*ppMeth)->GetSlot() + pIFace->m_wStartSlot;
                    MethodDesc* newMeth = targetClass->GetMethodDescForSlot(slot);          
                    _ASSERTE(newMeth != NULL);
                    *ppMeth = newMeth;
                }
            }
            else {
                // check the array case 
                if (!targetTH.IsNull()) {
                    // recevier is an array
                    if (targetTH == typeTH ||
                        eeClass == g_Mscorlib.GetClass(CLASS__ARRAY)->GetClass() ||
                        eeClass == g_Mscorlib.GetClass(CLASS__OBJECT)->GetClass()) 
                        return;
                    else
                        COMPlusThrow(kTargetException,L"RFLCT.Targ_ITargMismatch");
                }
                else if (!typeTH.IsNull())
                    COMPlusThrow(kTargetException,L"RFLCT.Targ_ITargMismatch");

                while (targetClass && targetClass != eeClass)
                    targetClass = targetClass->GetParentClass();

                if (!targetClass) {

                    // The class defined for this method is not a super class of the
                    //  target object
                    COMPlusThrow(kTargetException,L"RFLCT.Targ_ITargMismatch");
                }
            }
        }
    }
    return;
}


// This is an internal helper function to TypedReference class. 
// We already have verified that the types are compatable. Assing the object in args
// to the typed reference
FCIMPL3_VII(void, COMMember::ObjectToTypedReference, TypedByRef typedReference, Object* objUNSAFE, LPVOID handle)
{
    TypeHandle th(handle);
    InvokeUtil::_ObjectToTypedReferenceArgs args;

    args.typedReference = typedReference;
    args.obj            = ObjectToOBJECTREF(objUNSAFE);
    args.th             = th;

    HELPER_METHOD_FRAME_BEGIN_1(args.obj);

    g_pInvokeUtil->CreateTypedReference(&args);  

    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

// This is an internal helper function to TypedReference class. 
// It extracts the object from the typed reference.
FCIMPL1(Object*, COMMember::TypedReferenceToObject, TypedByRef typedReference)
{
    OBJECTREF       Obj = NULL;

    THROWSCOMPLUSEXCEPTION();


    if (typedReference.type.IsNull())
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_Type");
    
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, Obj);

    void* p = typedReference.data;
    EEClass* pClass = typedReference.type.GetClass();

        if (pClass->IsValueClass()) {
            Obj = pClass->GetMethodTable()->Box(p, FALSE);
        }
        else {
            Obj = ObjectToOBJECTREF(*((Object**)p));
        }

    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(Obj);
    }
FCIMPLEND

// InvokeCons
//
// This routine will invoke the constructor for a class.  It will verify that
//  the arguments passed are correct
//
FCIMPL6(Object*, COMMember::InvokeCons, ReflectBaseObject* refThisUNSAFE, INT32 attrs, Object* binderUNSAFE, PTRArray* objsUNSAFE, Object* localeUNSAFE, BYTE isBinderDefault)
{
    _InvokeConsArgs args;

    args.refThis  = (REFLECTBASEREF)  refThisUNSAFE;
    args.binder   = (OBJECTREF)       binderUNSAFE;
    args.objs     = (PTRARRAYREF)     objsUNSAFE;
    args.locale   = (OBJECTREF)       localeUNSAFE;

    OBJECTREF rv = NULL;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_NOPOLL(Frame::FRAME_ATTR_RETURNOBJ);
    GCPROTECT_BEGIN(args);
    HELPER_METHOD_POLL();

    rv = ObjectToOBJECTREF(InvokeConsInner(&args));

    GCPROTECT_END();
    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(rv);
}
FCIMPLEND

Object* COMMember::InvokeConsInner(_InvokeConsArgs* pArgs)
{
    THROWSCOMPLUSEXCEPTION();

    OBJECTREF rv = NULL;

    if (pArgs->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    MethodDesc*     pMeth;
    UINT            argCnt;
    EEClass*        eeClass;
    ARG_SLOT           ret = 0;
    OBJECTREF       o = 0;

    // Get the method Descr  (this should not fail)
    //  NOTE: both a constructor and a method are represented by a MetodDesc.
    //      If this ever changes we will need to fix this.
    ReflectMethod* pRM = (ReflectMethod*) pArgs->refThis->GetData();
    pMeth = pRM->pMethod;
    _ASSERTE(pMeth);
    eeClass = pMeth->GetClass();
    _ASSERTE(eeClass);

    if (pMeth->IsStatic()) {
        COMPlusThrow(kMemberAccessException,L"Acc_NotClassInit");
    }

    // if this is an abstract class then we will
    //  fail this
    if (eeClass->IsAbstract()) 
    {
        if (eeClass->IsInterface())
            COMPlusThrow(kMemberAccessException,L"Acc_CreateInterface");
        else
            COMPlusThrow(kMemberAccessException,L"Acc_CreateAbst");
    }

    if (eeClass->ContainsStackPtr()) 
        COMPlusThrow(kNotSupportedException, L"NotSupported_ContainsStackPtr");

    if (!pRM->pSignature) {
        PCCOR_SIGNATURE pSignature;     // The signature of the found method
        DWORD       cSignature;
        pRM->pMethod->GetSig(&pSignature,&cSignature);
        pRM->pSignature = ExpandSig::GetReflectSig(pSignature,
                                                   pRM->pMethod->GetModule());
    }
    ExpandSig* mSig = pRM->pSignature;

    if (mSig->IsVarArg()) 
        COMPlusThrow(kNotSupportedException, IDS_EE_VARARG_NOT_SUPPORTED);

    // Get the number of args on this element
    //  For a constructor there is always one arg which is the this pointer
    argCnt = (int) mSig->NumFixedArgs();

    ////////////////////////////////////////////////////////////
    // Validate the call
    // - Verify the number of args

    // Verify that we have been provided the proper number of
    //  args
    if (!pArgs->objs) {
        if (argCnt > 0) {
            // The wrong number of arguments were passed
            COMPlusThrow(kTargetParameterCountException,L"Arg_ParmCnt");
        }
    }
    else {
        if (pArgs->objs->GetNumComponents() != argCnt) {
            // The wrong number of arguments were passed
            COMPlusThrow(kTargetParameterCountException,L"Arg_ParmCnt");
        }
    }

    // Make sure we're not invoking on a save-only dynamic assembly
    // Check reflected class
    ReflectClass* pRC = (ReflectClass*) pArgs->refThis->GetReflClass();
    if (pRC)
    {
        Assembly *pAssem = pRC->GetClass()->GetAssembly();
        if (pAssem->IsDynamic() && !pAssem->HasRunAccess())
        {
            COMPlusThrow(kNotSupportedException, L"NotSupported_DynamicAssemblyNoRunAccess");
        }
    }

    // Check declaring class
    Assembly *pAssem = eeClass->GetAssembly();
    if (pAssem->IsDynamic() && !pAssem->HasRunAccess())
    {
        COMPlusThrow(kNotSupportedException, L"NotSupported_DynamicAssemblyNoRunAccess");
    }

    // this security context will be used in cast checking as well
    RefSecContext sCtx;

    // Validate the method can be called by this caller
    DWORD attr = pMeth->GetAttrs();
    if (!IsMdPublic(attr) || pMeth->RequiresLinktimeCheck() || !eeClass->IsExternallyVisible())
        CanAccess(pMeth, &sCtx);

    // Validate access to non-public types in the signature.
    void* pEnum;
    mSig->Reset(&pEnum);
    for (int i=0;i< (int) argCnt;i++) {
        TypeHandle th = mSig->NextArgExpanded(&pEnum);

        // Check the caller has access to the arg type.
        EEClass *pArgClass = GetUnderlyingClass(&th);
        if (pArgClass && !pArgClass->IsExternallyVisible())
            InvokeUtil::CheckAccessType(&sCtx, pArgClass, REFSEC_THROW_MEMBERACCESS);
    }

    /// Validation is done
    /////////////////////////////////////////////////////////////////////

    // Build the Args...This is in [0]
    //  All of the rest of the args are placed in reverse order
    //  into the arg array.

    // Make sure we call the <cinit>
    OBJECTREF Throwable = NULL;
    GCPROTECT_BEGIN(Throwable);
    if (!eeClass->DoRunClassInit(&Throwable)) {

        OBJECTREF except = g_pInvokeUtil->CreateTargetExcept(&Throwable);
        COMPlusThrow(except);
    }
    GCPROTECT_END();

    // If we are invoking a constructor on an array then we must
    //  handle this specially.  String objects allocate themselves
    //  so they are a special case.
    if (eeClass != g_pStringClass->GetClass()) {
        if (eeClass->IsArrayClass()) {
            o = ObjectToOBJECTREF((Object*)InvokeArrayCons((ReflectArrayClass*) pArgs->refThis->GetReflClass(),
                pMeth,&pArgs->objs,argCnt));
            goto lExit;
        }
        else if (CRemotingServices::IsRemoteActivationRequired(eeClass))
        {
            o = CRemotingServices::CreateProxyOrObject(eeClass->GetMethodTable());
        }
        else
        {
            o = AllocateObject(eeClass->GetMethodTable());
        }
    }
    else 
        o = 0;

    GCPROTECT_BEGIN(o);

    // Make sure we allocated the callArgs.  We must have
    //  at least one because of the this pointer
    ARG_SLOT *  pNewArgs = 0;
    UINT    nArgSlots = mSig->NumVirtualFixedArgs(pMeth->IsStatic());
    pNewArgs = (ARG_SLOT *) _alloca(nArgSlots*sizeof(ARG_SLOT));
#ifdef _DEBUG
    memset(pNewArgs,0xEE,nArgSlots*sizeof(ARG_SLOT));
#endif

    mSig->Reset(&pEnum);

    for (int i = 0; i < (int)argCnt; i++) {
        TypeHandle th = mSig->NextArgExpanded(&pEnum);
        // This routine will verify that all types are correct.
        g_pInvokeUtil->CheckArg(th, &(pArgs->objs->m_Array[i]), &sCtx);
    }
    
#ifdef STRESS_HEAP
    if (g_pConfig->GetGCStressLevel() != 0)
        g_pGCHeap->StressHeap();
#endif
    
    // NO GC AFTER THIS POINT
    // actually this statement is not completely accurate. If an exception occurs a gc may happen
    // but we are going to dump the stack anyway and we do not need to protect anything.
    // But if anywhere between here and the method invocation we enter preemptive mode the stack
    // we are about to setup (pDst in the loop) may contain references to random parts of memory

    // Copy "this" pointer
    if (eeClass->IsValueClass()) 
        *pNewArgs = PtrToArgSlot(o->UnBox());
    else
        *pNewArgs = ObjToArgSlot(o);
    
    // copy args
    ARG_SLOT *  pDst= pNewArgs+1;
    mSig->Reset(&pEnum);
    for (int i = 0; i < (int)argCnt; i++) {
        TypeHandle th = mSig->NextArgExpanded(&pEnum);
        UINT cbSize = mSig->GetElemSize(th);
        BYTE* argDst;

        if(cbSize > sizeof(ARG_SLOT))
        {
            // argument address is placed in the slot
            argDst = (BYTE*) _alloca(cbSize);
            *pDst = PtrToArgSlot(argDst);
        }
        else
        {
            // the argument itself is placed in the slot
            argDst = ArgSlotEndianessFixup(pDst, cbSize);
        }

        g_pInvokeUtil->CopyArg(th, &(pArgs->objs->m_Array[i]), pDst, argDst);
        pDst++;
    }

    // Call the method
    // Constructors always return null....
    COMPLUS_TRY {
        MetaSig threadSafeSig(*mSig);
        ret = pMeth->Call(pNewArgs,&threadSafeSig);
    } COMPLUS_CATCH {
        // If we get here we need to throw an TargetInvocationException
        OBJECTREF ppException = GETTHROWABLE();
        _ASSERTE(ppException);
        GCPROTECT_BEGIN(ppException);
        OBJECTREF except = g_pInvokeUtil->CreateTargetExcept(&ppException);
        COMPlusThrow(except);
        GCPROTECT_END();
    } COMPLUS_END_CATCH
    // We have a special case for Strings...The object
    //  is returned...
    if (eeClass == g_pStringClass->GetClass()) {
        o = ArgSlotToObj(ret);
    }

    GCPROTECT_END();        // object o

lExit:
    rv = o;

    return OBJECTREFToObject(rv);
}

FCIMPL4(void, COMMember::SerializationInvoke, ReflectBaseObject* refThisUNSAFE, Object * target, Object * SerializationInfoObj, StreamingContextData StreamingContextObj)
{
    _SerializationInvokeArgs args;

    args.refThis               = (REFLECTBASEREF)  refThisUNSAFE;
    args.target                = ObjectToOBJECTREF( target );
    args.serializationInfo     = ObjectToOBJECTREF( SerializationInfoObj );
    args.additionalContext     = ObjectToOBJECTREF( StreamingContextObj.additionalContext );
    args.contextStates         = StreamingContextObj.contextStates;

    HELPER_METHOD_FRAME_BEGIN_NOPOLL();
    GCPROTECT_BEGIN(args.refThis);
    GCPROTECT_BEGIN(args.target);
    GCPROTECT_BEGIN(args.serializationInfo);
    GCPROTECT_BEGIN(args.additionalContext);
    HELPER_METHOD_POLL();

    SerializationInvokeInner(&args);

    GCPROTECT_END();
    GCPROTECT_END();
    GCPROTECT_END();
    GCPROTECT_END();
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

// SerializationInvoke
void COMMember::SerializationInvokeInner(_SerializationInvokeArgs *args) {

    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    MethodDesc*     pMeth;
    EEClass*        eeClass;
    ARG_SLOT           ret;

    // Get the method Descr  (this should not fail)
    //  NOTE: both a constructor and a method are represented by a MetodDesc.
    //      If this ever changes we will need to fix this.
    ReflectMethod* pRM = (ReflectMethod*) args->refThis->GetData();
    pMeth = pRM->pMethod;
    _ASSERTE(pMeth);
    eeClass = pMeth->GetClass();
    _ASSERTE(eeClass);

    if (!pRM->pSignature) {
        PCCOR_SIGNATURE pSignature;     // The signature of the found method
        DWORD       cSignature;
        pRM->pMethod->GetSig(&pSignature,&cSignature);
        pRM->pSignature = ExpandSig::GetReflectSig(pSignature,
                                                   pRM->pMethod->GetModule());
    }
    ExpandSig* mSig = pRM->pSignature;

    // Make sure we call the <cinit>
    OBJECTREF Throwable = NULL;
    GCPROTECT_BEGIN(Throwable);
    if (!eeClass->DoRunClassInit(&Throwable)) {
        OBJECTREF except = g_pInvokeUtil->CreateTargetExcept(&Throwable);
        COMPlusThrow(except);
    }
    GCPROTECT_END();

    _ASSERTE(!(eeClass->IsArrayClass()));
    _ASSERTE(eeClass!=g_pStringClass->GetClass());

    ARG_SLOT newArgs[4];

    // make sure method has correct size sig
    //_ASSERTE(mSig->SizeOfVirtualFixedArgStack(false/*IsStatic*/) == sizeof(newArgs));

    // NO GC AFTER THIS POINT

    // Copy "this" pointer
    if (eeClass->IsValueClass()) 
        newArgs[0] = PtrToArgSlot(args->target->UnBox());
    else
        newArgs[0] = ObjToArgSlot(args->target);
    
    newArgs[1] = ObjToArgSlot(args->serializationInfo);
    newArgs[2] = ObjToArgSlot(args->additionalContext);
    newArgs[3] = args->contextStates;

    // Call the method
    // Constructors always return null....
    COMPLUS_TRY {
        MetaSig threadSafeSig(*mSig);
        ret = pMeth->Call(newArgs,&threadSafeSig);
    } COMPLUS_CATCH {
        // If we get here we need to throw an TargetInvocationException
        OBJECTREF ppException = GETTHROWABLE();
        _ASSERTE(ppException);
        GCPROTECT_BEGIN(ppException);
        OBJECTREF except = g_pInvokeUtil->CreateTargetExcept(&ppException);
        COMPlusThrow(except);
        GCPROTECT_END();
    } COMPLUS_END_CATCH
}

// InvokeArrayCons
// This method will return a new Array Object from the constructor.

LPVOID COMMember::InvokeArrayCons(ReflectArrayClass* pRC,
                MethodDesc* pMeth,PTRARRAYREF* objs,int argCnt)
{
    THROWSCOMPLUSEXCEPTION();

    LPVOID rv = 0;
    DWORD i, j;

    ArrayTypeDesc* arrayDesc = pRC->GetTypeHandle().AsArray();

    // If we're trying to create an array of pointers or function pointers,
    // check that the caller has skip verification permission.
    CorElementType et = arrayDesc->GetElementTypeHandle().GetNormCorElementType();
    if (et == ELEMENT_TYPE_PTR || et == ELEMENT_TYPE_FNPTR)
        COMCodeAccessSecurityEngine::SpecialDemand(SECURITY_SKIP_VER);

    // Validate the argCnt an the Rank. Also allow nested SZARRAY's.
    _ASSERTE(argCnt == (int) arrayDesc->GetRank() || argCnt == (int) arrayDesc->GetRank() * 2 ||
             arrayDesc->GetNormCorElementType() == ELEMENT_TYPE_SZARRAY);

    // Validate all of the parameters.  These all typed as integers
    INT32* indexes = (INT32*) _alloca(sizeof(INT32) * argCnt);
    ZeroMemory(indexes, sizeof(int) * argCnt);

    for (i=0;i<(DWORD)argCnt;i++) {
        if (!(*objs)->m_Array[i])
            COMPlusThrowArgumentException(L"parameters", L"Arg_NullIndex");
        MethodTable* pMT = ((*objs)->m_Array[i])->GetMethodTable();
        CorElementType oType = pMT->GetNormCorElementType();
        if (!InvokeUtil::IsPrimitiveType(oType) || !InvokeUtil::CanPrimitiveWiden(ELEMENT_TYPE_I4,oType))
            COMPlusThrow(kArgumentException,L"Arg_PrimWiden");
        memcpy(&indexes[i], (*objs)->m_Array[i]->UnBox(),pMT->GetClass()->GetNumInstanceFieldBytes());
    }

    // We are allocating some type of general array
    DWORD rank = arrayDesc->GetRank();
    DWORD boundsSize;
    INT32* bounds;
    if (arrayDesc->GetNormCorElementType() == ELEMENT_TYPE_ARRAY) {
        BOOL hasLowerBounds;

        // Validate the argCnt and Rank
        if (argCnt == (int) arrayDesc->GetRank()) {
            hasLowerBounds = FALSE;
            boundsSize     = rank;
        }
        else {
            hasLowerBounds = TRUE;
            boundsSize     = rank*2;
            _ASSERTE(argCnt == (int) arrayDesc->GetRank() * 2);
            _ASSERTE(arrayDesc->GetNormCorElementType() == ELEMENT_TYPE_ARRAY);
        }

        bounds = (INT32*) _alloca(boundsSize * sizeof(INT32));

        for (i=0,j=0; i<rank; i++,j+=2) {
            INT32 d = 0;
            if (hasLowerBounds)
                d = indexes[i++];
            bounds[j]   = d;
            bounds[j+1] = d + indexes[i];
        }
    }
    else {
        _ASSERTE(arrayDesc->GetNormCorElementType() == ELEMENT_TYPE_SZARRAY);
        boundsSize = argCnt;
        bounds = (INT32*) _alloca(boundsSize * sizeof(INT32));
        for (i = 0; i < (DWORD)argCnt; i++) {
            bounds[i] = indexes[i];
        }
    }

    PTRARRAYREF pRet = (PTRARRAYREF) AllocateArrayEx(TypeHandle(arrayDesc), bounds, boundsSize);
    *((PTRARRAYREF *)&rv) = pRet;
    return rv;
}

// CreateInstance
// This routine will create an instance of a Class by invoking the null constructor
//  if a null constructor is present.  
// Return LPVOID  (System.Object.)
// Args: _CreateInstanceArgs
// 
FCIMPL2(Object*, COMMember::CreateInstance, ReflectClassBaseObject* refThisUNSAFE, BYTE publicOnly)
{
    THROWSCOMPLUSEXCEPTION();

    if (refThisUNSAFE == NULL) 
    {
        FCThrow(kNullReferenceException);
    }

    EEClass* pVMC;
    MethodDesc* pMeth;
    DWORD attr;

    OBJECTREF rv = NULL;
    OBJECTREF o;
    OBJECTREF Throwable = NULL;
    REFLECTCLASSBASEREF refThis = (REFLECTCLASSBASEREF)ObjectToOBJECTREF(refThisUNSAFE);
    
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_2(Frame::FRAME_ATTR_RETURNOBJ, rv, refThis);

    // Get the EEClass and Vtable associated with refThis
    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    pVMC = pRC->GetClass();
    if (pVMC == 0)
    {
        COMPlusThrow(kMissingMethodException,L"Arg_NoDefCTor");
    }
   

    // If we are creating a COM object which has backing metadata we still
    // need to ensure that the caller has unmanaged code access permission.
    if (pVMC->GetMethodTable()->IsComObjectType())
        COMCodeAccessSecurityEngine::SpecialDemand(SECURITY_UNMANAGED_CODE);

    // if this is an abstract class then we will fail this
    if (pVMC->IsAbstract()) 
    {
        if (pVMC->IsInterface())
        {
            COMPlusThrow(kMemberAccessException,L"Acc_CreateInterface");
        }
        else
        {
            COMPlusThrow(kMemberAccessException,L"Acc_CreateAbst");
        }
    }

    if (pVMC->ContainsStackPtr()) 
    {
        COMPlusThrow(kNotSupportedException, L"NotSupported_ContainsStackPtr");
    }

    if (!pVMC->GetMethodTable()->HasDefaultConstructor())
    {
        // We didn't find the parameterless constructor,
        //  if this is a Value class we can simply allocate one and return it

        if (!pVMC->IsValueClass())
        {
            COMPlusThrow(kMissingMethodException,L"Arg_NoDefCTor");
        }

        rv = pVMC->GetMethodTable()->Allocate();
        // return rv;
        goto lExit;
    }

    pMeth = pVMC->GetMethodTable()->GetDefaultConstructor();

    // Validate the method can be called by this caller
    attr = pMeth->GetAttrs();

    if (!IsMdPublic(attr) && publicOnly)
    {
        COMPlusThrow(kMissingMethodException,L"Arg_NoDefCTor");
    }

    if (!IsMdPublic(attr) || pMeth->RequiresLinktimeCheck() || !pVMC->IsExternallyVisible()) 
    {
        RefSecContext sCtx;
        CanAccess(pMeth, &sCtx);
    }

    // call the <cinit> 
    GCPROTECT_BEGIN(Throwable);
    if (!pVMC->DoRunClassInit(&Throwable)) 
    {
        OBJECTREF except = g_pInvokeUtil->CreateTargetExcept(&Throwable);
        COMPlusThrow(except);
    }
    GCPROTECT_END();

    // We've got the class, lets allocate it and call the constructor
    if (pVMC->IsThunking())
        COMPlusThrow(kMissingMethodException,L"NotSupported_Constructor");

    if (CRemotingServices::IsRemoteActivationRequired(pVMC))
    {
        o = CRemotingServices::CreateProxyOrObject(pVMC->GetMethodTable());
    }
    else
    {
        o = AllocateObject(pVMC->GetMethodTable());
    }

    GCPROTECT_BEGIN(o)
    {
        MetaSig sig(pMeth->GetSig(),pMeth->GetModule());

        // Copy "this" pointer
        ARG_SLOT arg;

        if (pVMC->IsValueClass()) 
            arg = PtrToArgSlot(o->UnBox());
        else
            arg = ObjToArgSlot(o);

        // Call the method
        TryCallMethod(pMeth, &arg, &sig);

        rv = o;
    }
    GCPROTECT_END();

lExit: ;
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(rv);
}
FCIMPLEND

//
// This function is necessary to CreateInstance because we cannot have a COMPLUS_TRY block in an FCall
//
void COMMember::TryCallMethod(MethodDesc *pMeth, ARG_SLOT* args, MetaSig* pSig)
{
    THROWSCOMPLUSEXCEPTION();

    COMPLUS_TRY 
    {
        pMeth->Call(args, pSig);
    } 
    COMPLUS_CATCH 
    {
            // If we get here we need to throw an TargetInvocationException
            OBJECTREF ppException = GETTHROWABLE();
            _ASSERTE(ppException);
            GCPROTECT_BEGIN(ppException);
            OBJECTREF except = g_pInvokeUtil->CreateTargetExcept(&ppException);
            COMPlusThrow(except);
            GCPROTECT_END();
        } COMPLUS_END_CATCH
}

// Init the ReflectField cached info, if not done already
VOID COMMember::InitReflectField(FieldDesc *pField, ReflectField *pRF)
{
    if (pRF->type == ELEMENT_TYPE_END)
    {
        CorElementType t;
        // Get the type of the field
        pRF->thField = g_pInvokeUtil->GetFieldTypeHandle(pField, &t);
        // Field attributes
        pRF->dwAttr = pField->GetAttributes();
        //Do this last to prevent race conditions
        pRF->type = t;
    }
}

// FieldGet
// This method will get the value associated with an object
FCIMPL3(Object*, COMMember::FieldGet, ReflectBaseObject* refThisUNSAFE, Object* targetUNSAFE, BYTE requiresAccessCheck)
{
    THROWSCOMPLUSEXCEPTION();

    if (refThisUNSAFE == NULL) 
        FCThrowRes(kNullReferenceException, L"NullReference_This");
    ReflectField* pRF = (ReflectField*) refThisUNSAFE->GetData();

    OBJECTREF   target  = ObjectToOBJECTREF(targetUNSAFE);
    OBJECTREF   rv      = NULL; // not protected

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, target);

    FieldDesc*  pField;
    EEClass*    eeClass;

    // Get the field and EEClass from the Object
    pField = pRF->pField;
    _ASSERTE(pField);
    eeClass = pField->GetEnclosingClass();
    _ASSERTE(eeClass);

    // Validate the call
    g_pInvokeUtil->ValidateObjectTarget(pField,eeClass,target);

    // See if cached field information is available
    InitReflectField(pField, pRF);

    // Verify the callee/caller access
    if (requiresAccessCheck) {
        RefSecContext sCtx;
        if (target != NULL && !pField->IsStatic()) {
            if (!target->GetTypeHandle().IsTypeDesc()) {
                sCtx.SetClassOfInstance(target->GetClass());
            }
        }
        CanAccessField(pRF, &sCtx);
    }

    // There can be no GC after thing until the Object is returned.
    ARG_SLOT value;
    value = g_pInvokeUtil->GetFieldValue(pRF->type,pRF->thField,pField,&target);
    if (pRF->type == ELEMENT_TYPE_VALUETYPE ||
        pRF->type == ELEMENT_TYPE_PTR) {
        rv = ArgSlotToObj(value);
    }
    else {
        rv = g_pInvokeUtil->CreateObject(pRF->thField,value);
    }

    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(rv);
}
FCIMPLEND


FCIMPL3(Object*, COMMember::DirectFieldGet, ReflectBaseObject* refThisUNSAFE, TypedByRef target, BYTE requiresAccessCheck)
{
    THROWSCOMPLUSEXCEPTION();

    REFLECTBASEREF  refThis = (REFLECTBASEREF) refThisUNSAFE;
    OBJECTREF       refRet  = NULL;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refThis);

    if (refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    // Get the field and EEClass from the Object
    ReflectField* pRF = (ReflectField*) refThis->GetData();
    FieldDesc* pField = pRF->pField;
    _ASSERTE(pField);

    ARG_SLOT value;
    EEClass* fldEEC;

    // Find the Object and its type
    EEClass* targetEEC = target.type.GetClass();
    if (pField->IsStatic() || !targetEEC->IsValueClass())
    {
        refRet = DirectObjectFieldGet(pField, target);
        goto lExit;
    }

    // See if cached field information is available
    InitReflectField(pField, pRF);

    fldEEC = pField->GetEnclosingClass();
    _ASSERTE(fldEEC);

    // Validate that the target type can be cast to the type that owns this field info.
    if (!TypeHandle(targetEEC).CanCastTo(TypeHandle(fldEEC)))
        COMPlusThrowArgumentException(L"obj", NULL);

    // Verify the callee/caller access
    if (requiresAccessCheck) {
        RefSecContext sCtx;
        sCtx.SetClassOfInstance(targetEEC);
        CanAccessField(pRF, &sCtx);
    }

    value = (ARG_SLOT) -1;

    // This is a hack because from the previous case we may end up with an
    //  Enum.  We want to process it here.
    // Get the value from the field
    void* p;
    switch (pRF->type) {
    case ELEMENT_TYPE_VOID:
        _ASSERTE(!"Void used as Field Type!");
        COMPlusThrow(kInvalidProgramException);

    case ELEMENT_TYPE_BOOLEAN:  // boolean
    case ELEMENT_TYPE_I1:       // byte
    case ELEMENT_TYPE_U1:       // unsigned byte
        p = ((BYTE*) target.data) + pField->GetOffset();
        value = *(UINT8*) p;
        break;

    case ELEMENT_TYPE_I2:       // short
    case ELEMENT_TYPE_U2:       // unsigned short
    case ELEMENT_TYPE_CHAR:     // char
        p = ((BYTE*) target.data) + pField->GetOffset();
        value = *(UINT16*) p;
        break;

    case ELEMENT_TYPE_I4:       // int
    case ELEMENT_TYPE_U4:       // unsigned int
    IN_WIN32(case ELEMENT_TYPE_I:)
    IN_WIN32(case ELEMENT_TYPE_U:)
    case ELEMENT_TYPE_R4:       // float
        p = ((BYTE*) target.data) + pField->GetOffset();
        value = *(UINT32*) p;
        break;

    IN_WIN64(case ELEMENT_TYPE_I:)
    IN_WIN64(case ELEMENT_TYPE_U:)
    case ELEMENT_TYPE_I8:       // long
    case ELEMENT_TYPE_U8:       // unsigned long
    case ELEMENT_TYPE_R8:       // double
        p = ((BYTE*) target.data) + pField->GetOffset();
        value = *(INT64*) p;
        break;

    case ELEMENT_TYPE_OBJECT:
    case ELEMENT_TYPE_CLASS:
    case ELEMENT_TYPE_SZARRAY:          // Single Dim, Zero
    case ELEMENT_TYPE_ARRAY:            // general array
        {
            p = ((BYTE*) target.data) + pField->GetOffset();
            refRet = ObjectToOBJECTREF(*(Object**) p);
            goto lExit;
        }
        break;

    case ELEMENT_TYPE_VALUETYPE:
        {
            // Allocate an object to return...
            _ASSERTE(pRF->thField.IsUnsharedMT());
            refRet = AllocateObject(pRF->thField.AsMethodTable());

            // copy the field to the unboxed object.
            p = ((BYTE*) target.data) + pField->GetOffset();
            CopyValueClass(refRet->UnBox(), p, pRF->thField.AsMethodTable(), refRet->GetAppDomain());
            goto lExit;
        }
        break;

    case ELEMENT_TYPE_PTR:
        {
            p = ((BYTE*) target.data) + pField->GetOffset();
            value = *(size_t*) p;

            g_pInvokeUtil->InitPointers();
            OBJECTREF obj = AllocateObject(g_pInvokeUtil->_ptr.AsMethodTable());
            GCPROTECT_BEGIN(obj);
            // Ignore null return
            OBJECTREF typeOR = pRF->thField.CreateClassObj();
            g_pInvokeUtil->_ptrType->SetRefValue(obj,typeOR);
            g_pInvokeUtil->_ptrValue->SetValuePtr(obj,(PVOID)(size_t)value);
            refRet = obj;
            GCPROTECT_END();
            goto lExit;
        }
        break;

    default:
        _ASSERTE(!"Unknown Type");
        // this is really an impossible condition
        COMPlusThrow(kNotSupportedException);
    }

    refRet = g_pInvokeUtil->CreateObject(pRF->thField,value);

lExit: ;
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refRet);
}
FCIMPLEND


static void TryDemand(DWORD whatPermission, RuntimeExceptionKind reKind, LPCWSTR wszTag)
{
    THROWSCOMPLUSEXCEPTION();

    COMPLUS_TRY 
    {
        COMCodeAccessSecurityEngine::SpecialDemand(whatPermission);
    } 
    COMPLUS_CATCH 
    {
        COMPlusThrow(reKind, wszTag);
    } 
    COMPLUS_END_CATCH
}

// FieldSet
// This method will set the field of an associated object
FCIMPL8(void, COMMember::FieldSet, 
                    ReflectBaseObject*  refThisUNSAFE, 
                    Object*             targetUNSAFE, 
                    Object*             valueUNSAFE, 
                    INT32               attrs, 
                    Object*             binderUNSAFE, 
                    Object*             localeUNSAFE, 
                    BYTE                requiresAccessCheck, 
                    BYTE                isBinderDefault)
{
    THROWSCOMPLUSEXCEPTION();

    HELPER_METHOD_FRAME_BEGIN_NOPOLL();

    struct _gc
    {
        REFLECTBASEREF  refThis;
        OBJECTREF       target;
        OBJECTREF       value;
        OBJECTREF       binder;
        OBJECTREF       locale;
    } gc;

    gc.refThis  = (REFLECTBASEREF)  refThisUNSAFE;
    gc.target   = (OBJECTREF)       targetUNSAFE;
    gc.value    = (OBJECTREF)       valueUNSAFE;
    gc.binder   = (OBJECTREF)       binderUNSAFE;
    gc.locale   = (OBJECTREF)       localeUNSAFE;

    GCPROTECT_BEGIN(gc);
    HELPER_METHOD_POLL();

    if (gc.refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    HRESULT     hr;

    // Get the field and EEClass from the Object
    ReflectField* pRF = (ReflectField*) gc.refThis->GetData();
    FieldDesc*  pField = pRF->pField;
    _ASSERTE(pField);
    EEClass* eeClass = pField->GetEnclosingClass();
    _ASSERTE(eeClass);

    // Validate the target/fld type relationship
    g_pInvokeUtil->ValidateObjectTarget(pField,eeClass,gc.target);

    // See if cached field information is available
    InitReflectField(pField, pRF);

    RefSecContext sCtx;

    // Verify that the value passed can be widened into the target
    hr = g_pInvokeUtil->ValidField(pRF->thField, &gc.value, &sCtx);
    if (FAILED(hr)) {
        // Call change type so we can attempt this again...
        if (!(attrs & BINDER_ExactBinding) && gc.binder != NULL && !isBinderDefault) {

            gc.value = g_pInvokeUtil->ChangeType(gc.binder,gc.value,pRF->thField,gc.locale);

            // See if the results are now legal...
            hr = g_pInvokeUtil->ValidField(pRF->thField,&gc.value, &sCtx);
            if (FAILED(hr)) {
                if (hr == E_INVALIDARG) {
                    COMPlusThrow(kArgumentException,L"Arg_ObjObj");
                }
                // this is really an impossible condition
                COMPlusThrow(kNotSupportedException);
            }

        }
        else {
            // Not a value field for the passed argument.
            if (hr == E_INVALIDARG) {
                COMPlusThrow(kArgumentException,L"Arg_ObjObj");
            }
            // this is really an impossible condition
            COMPlusThrow(kNotSupportedException);
        }
    }   
    
    // Verify that this is not a Final Field
    if (requiresAccessCheck) 
    {
        if (IsFdInitOnly(pRF->dwAttr)) {
            TryDemand(SECURITY_SERIALIZATION, kFieldAccessException, L"Acc_ReadOnly");
        }
        if (IsFdHasFieldRVA(pRF->dwAttr)) {
            TryDemand(SECURITY_SKIP_VER, kFieldAccessException, L"Acc_ReadOnly");
        }
        if (IsFdLiteral(pRF->dwAttr)) 
            COMPlusThrow(kFieldAccessException,L"Acc_ReadOnly");
    }

    // Verify the callee/caller access
    if (requiresAccessCheck) {
        if (gc.target != NULL && !pField->IsStatic()) {
            if (!gc.target->GetTypeHandle().IsTypeDesc()) {
                sCtx.SetClassOfInstance(gc.target->GetClass());
            }
        }
        CanAccessField(pRF, &sCtx);
    }

    g_pInvokeUtil->SetValidField(pRF->type,pRF->thField,pField,&gc.target,&gc.value);

    GCPROTECT_END();
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND


// DirectFieldSet
// This method will set the field (defined by the this) on the object
//  which was passed by a TypedReference.

FCIMPL4_IVII(void, COMMember::DirectFieldSet, ReflectBaseObject* refThisUNSAFE, TypedByRef target, Object* valueUNSAFE, BYTE requiresAccessCheck)
{
    THROWSCOMPLUSEXCEPTION();

    REFLECTBASEREF  refThis = (REFLECTBASEREF)  refThisUNSAFE;
    OBJECTREF       oValue   = (OBJECTREF)       valueUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_2(refThis, oValue);

    if (refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    // Get the field and EEClass from the Object
    ReflectField* pRF = (ReflectField*) refThis->GetData();
    FieldDesc* pField = pRF->pField;
    _ASSERTE(pField);

    EEClass* fldEEC;

    // Find the Object and its type
    EEClass* targetEEC = target.type.GetClass();
    if (pField->IsStatic() || !targetEEC->IsValueClass()) {
        DirectObjectFieldSet(pField, target, &oValue, requiresAccessCheck);
        goto lExit;
    }

    fldEEC = pField->GetEnclosingClass();
    _ASSERTE(fldEEC);

    // Validate that the target type can be cast to the type that owns this field info.
    if (!TypeHandle(targetEEC).CanCastTo(TypeHandle(fldEEC)))
        COMPlusThrowArgumentException(L"obj", NULL);

    // We dont verify that the user has access because
    //  we assume that access is granted because its 
    //  a TypeReference.

    // See if cached field information is available
    InitReflectField(pField, pRF);

    {
        RefSecContext sCtx;

        // Verify that the value passed can be widened into the target
        HRESULT hr = g_pInvokeUtil->ValidField(pRF->thField, &oValue, &sCtx);
        // Not a value field for the passed argument.
        if (FAILED(hr)) {
            if (hr == E_INVALIDARG) {
                COMPlusThrow(kArgumentException,L"Arg_ObjObj");
            }
            // this is really an impossible condition
            COMPlusThrow(kNotSupportedException);
        }

        // Verify that this is not a Final Field
        if (requiresAccessCheck) {
            if (IsFdInitOnly(pRF->dwAttr)) {
                TryDemand(SECURITY_SERIALIZATION, kFieldAccessException, L"Acc_ReadOnly");
            }
            if (IsFdHasFieldRVA(pRF->dwAttr)) {
                TryDemand(SECURITY_SKIP_VER, kFieldAccessException, L"Acc_ReadOnly");
            }
            if (IsFdLiteral(pRF->dwAttr)) 
                COMPlusThrow(kFieldAccessException,L"Acc_ReadOnly");
        }
        
        // Verify the callee/caller access
        if (requiresAccessCheck) {
            sCtx.SetClassOfInstance(targetEEC);
            CanAccessField(pRF, &sCtx);
        }
    }

    // Set the field
    ARG_SLOT value;

    switch (pRF->type) {
    case ELEMENT_TYPE_VOID:
        _ASSERTE(!"Void used as Field Type!");
        COMPlusThrow(kInvalidProgramException);

    case ELEMENT_TYPE_BOOLEAN:  // boolean
    case ELEMENT_TYPE_I1:       // byte
    case ELEMENT_TYPE_U1:       // unsigned byte
        {
            value = 0;
            if (oValue != 0) {
                MethodTable* p = oValue->GetMethodTable();
                CorElementType oType = p->GetNormCorElementType();
                g_pInvokeUtil->CreatePrimitiveValue(pRF->type,oType,oValue,&value);
            }

            void* p = ((BYTE*) target.data) + pField->GetOffset();
	    *(UINT8*)p = (UINT8)value;
        }
        break;

    case ELEMENT_TYPE_I2:       // short
    case ELEMENT_TYPE_U2:       // unsigned short
    case ELEMENT_TYPE_CHAR:     // char
        {
            value = 0;
            if (oValue != 0) {
                MethodTable* p = oValue->GetMethodTable();
                CorElementType oType = p->GetNormCorElementType();
                g_pInvokeUtil->CreatePrimitiveValue(pRF->type,oType,oValue,&value);
            }

            void* p = ((BYTE*) target.data) + pField->GetOffset();
            *(UINT16*) p = (UINT16)value;
        }
        break;

    case ELEMENT_TYPE_PTR:      // pointers
        _ASSERTE(!g_pInvokeUtil->_ptr.IsNull());
        if (oValue != 0) {
            value = 0;
            if (oValue->GetTypeHandle() == g_pInvokeUtil->_ptr) {
                value = (size_t) g_pInvokeUtil->GetPointerValue(&oValue);
                void* p = ((BYTE*) target.data) + pField->GetOffset();
                *(size_t*) p = (size_t) value;
                break;
            }
        }
        // drop through
    case ELEMENT_TYPE_FNPTR:
        {
            value = 0;
            if (oValue != 0) {
                MethodTable* p = oValue->GetMethodTable();
                CorElementType oType = p->GetNormCorElementType();
                g_pInvokeUtil->CreatePrimitiveValue(oType, oType, oValue, &value);
            }
            void* p = ((BYTE*) target.data) + pField->GetOffset();
            *(size_t*) p = (size_t) value;
        }
        break;

    case ELEMENT_TYPE_I4:       // int
    case ELEMENT_TYPE_U4:       // unsigned int
    case ELEMENT_TYPE_R4:       // float
    IN_WIN32(case ELEMENT_TYPE_I:)
    IN_WIN32(case ELEMENT_TYPE_U:)
        {
            value = 0;
            if (oValue != 0) {
                MethodTable* p = oValue->GetMethodTable();
                CorElementType oType = p->GetNormCorElementType();
                g_pInvokeUtil->CreatePrimitiveValue(pRF->type,oType,oValue,&value);
            }
            void* p = ((BYTE*) target.data) + pField->GetOffset();
            *(UINT32*) p = (UINT32)value;
        }
        break;

    case ELEMENT_TYPE_I8:       // long
    case ELEMENT_TYPE_U8:       // unsigned long
    case ELEMENT_TYPE_R8:       // double
    IN_WIN64(case ELEMENT_TYPE_I:)
    IN_WIN64(case ELEMENT_TYPE_U:)
        {
            value = 0;
            if (oValue != 0) {
                MethodTable* p = oValue->GetMethodTable();
                CorElementType oType = p->GetNormCorElementType();
                g_pInvokeUtil->CreatePrimitiveValue(pRF->type,oType,oValue,&value);
            }

            void* p = ((BYTE*) target.data) + pField->GetOffset();
            *(INT64*) p = value;
        }
        break;

    case ELEMENT_TYPE_SZARRAY:          // Single Dim, Zero
    case ELEMENT_TYPE_ARRAY:            // General Array
    case ELEMENT_TYPE_CLASS:
    case ELEMENT_TYPE_OBJECT:
        {
            void* p = ((BYTE*) target.data) + pField->GetOffset();
            SetObjectReferenceUnchecked((OBJECTREF*) p, oValue);
        }
        break;

    case ELEMENT_TYPE_VALUETYPE:
        {
            _ASSERTE(pRF->thField.IsUnsharedMT());
            MethodTable* pMT = pRF->thField.AsMethodTable();
            EEClass* pEEC = pMT->GetClass();
            void* p = ((BYTE*) target.data) + pField->GetOffset();

            // If we have a null value then we must create an empty field
            if (oValue == 0) {
                InitValueClass(p, pEEC->GetMethodTable());
                goto lExit;
            }
            // Value classes require createing a boxed version of the field and then
            //  copying from the source...
           CopyValueClassUnchecked(p, oValue->UnBox(), pEEC->GetMethodTable());
        }
        break;

    default:
        _ASSERTE(!"Unknown Type");
        // this is really an impossible condition
        COMPlusThrow(kNotSupportedException);
    }

lExit: ;
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

// DirectObjectFieldGet
// When the TypedReference points to a object we call this method to
//  get the field value
OBJECTREF COMMember::DirectObjectFieldGet(FieldDesc* pField, TypedByRef target)
{
    OBJECTREF refRet;
    EEClass* eeClass = pField->GetEnclosingClass();
    _ASSERTE(eeClass);

    OBJECTREF objRef = NULL;
    GCPROTECT_BEGIN(objRef);
    if (!pField->IsStatic()) {
        objRef = ObjectToOBJECTREF(*((Object**)target.data));
    }

    // Validate the call
    g_pInvokeUtil->ValidateObjectTarget(pField,eeClass,objRef);

    // Get the type of the field
    CorElementType type;
    TypeHandle th = g_pInvokeUtil->GetFieldTypeHandle(pField,&type);

    // There can be no GC after thing until the Object is returned.
    ARG_SLOT value;
    value = g_pInvokeUtil->GetFieldValue(type,th,pField,&objRef);
    if (type == ELEMENT_TYPE_VALUETYPE) {
        refRet = ArgSlotToObj(value);
    }
    else {
        refRet = g_pInvokeUtil->CreateObject(th,value);
    }
    GCPROTECT_END();
    return refRet;
}

// DirectObjectFieldSet
// When the TypedReference points to a object we call this method to
//  set the field value
void COMMember::DirectObjectFieldSet(FieldDesc* pField, TypedByRef target, OBJECTREF* pvalue, BOOL requiresAccessCheck)
{
    THROWSCOMPLUSEXCEPTION();

    EEClass* eeClass = pField->GetEnclosingClass();
    _ASSERTE(eeClass);

    OBJECTREF objRef = NULL;
    GCPROTECT_BEGIN(objRef);
    if (!pField->IsStatic()) {
        objRef = ObjectToOBJECTREF(*((Object**)target.data));
    }
    // Validate the target/fld type relationship
    g_pInvokeUtil->ValidateObjectTarget(pField,eeClass,objRef);

    // Verify that the value passed can be widened into the target
    CorElementType type;
    TypeHandle th = g_pInvokeUtil->GetFieldTypeHandle(pField,&type);

    RefSecContext sCtx;

    HRESULT hr = g_pInvokeUtil->ValidField(th, pvalue, &sCtx);
    if (FAILED(hr)) {
        if (hr == E_INVALIDARG) {
            COMPlusThrow(kArgumentException,L"Arg_ObjObj");
        }
        // this is really an impossible condition
        COMPlusThrow(kNotSupportedException);
    }

    // Verify that this is not a Final Field
    DWORD attr = pField->GetAttributes();
    if (IsFdInitOnly(attr) || IsFdLiteral(attr)) {
        COMPlusThrow(kFieldAccessException,L"Acc_ReadOnly");
    }

    if (IsFdHasFieldRVA(attr)) {
        TryDemand(SECURITY_SKIP_VER, kFieldAccessException, L"Acc_ReadOnly");
    }

    // Verify the callee/caller access
    if (!pField->IsPublic() && requiresAccessCheck) {
        if (objRef != NULL) 
            if (!objRef->GetTypeHandle().IsTypeDesc())
                sCtx.SetClassOfInstance(objRef->GetClass());
        
        InvokeUtil::CheckAccess(&sCtx,
                                pField->GetAttributes(),
                                pField->GetMethodTableOfEnclosingClass(),
                                REFSEC_CHECK_MEMBERACCESS|REFSEC_THROW_FIELDACCESS);
    }
    else if (!eeClass->IsExternallyVisible()) {
        if (objRef != NULL) 
            if (!objRef->GetTypeHandle().IsTypeDesc())
                sCtx.SetClassOfInstance(objRef->GetClass());
        
        InvokeUtil::CheckAccess(&sCtx,
                                pField->GetAttributes(),
                                pField->GetMethodTableOfEnclosingClass(),
                                REFSEC_CHECK_MEMBERACCESS|REFSEC_THROW_FIELDACCESS);
    }

    g_pInvokeUtil->SetValidField(type,th,pField,&objRef,pvalue);
    GCPROTECT_END();
}

// MakeTypedReference
// This method will take an object, an array of FieldInfo's and create
//  at TypedReference for it (Assuming its valid).  This will throw a
//  MissingMemberException.  Outside of the first object, all the other
//  fields must be ValueTypes.
FCIMPL3(void, COMMember::MakeTypedReference, TypedByRef* value, Object* targetUNSAFE, PTRArray* fldsUNSAFE)
{
    REFLECTBASEREF fld;
    THROWSCOMPLUSEXCEPTION();
    DWORD offset = 0;

    OBJECTREF   target  = (OBJECTREF)   targetUNSAFE;
    PTRARRAYREF flds    = (PTRARRAYREF) fldsUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_2(target, flds);

    // Verify that the object is of the proper type...
    TypeHandle typeHnd = target->GetTypeHandle();
    DWORD cnt = flds->GetNumComponents();
    for (DWORD i=0;i<cnt;i++) {
        fld = (REFLECTBASEREF) flds->m_Array[i];
        if (fld == 0)
            COMPlusThrowArgumentNull(L"className",L"ArgumentNull_ArrayValue");

        // Get the field for this...
        ReflectField* pRF = (ReflectField*) fld->GetData();
        FieldDesc* pField = pRF->pField;

        // Verify that the enclosing class for the field
        //  and the class are the same.  If not this is an exception
        EEClass* p = pField->GetEnclosingClass();
        if (typeHnd.GetClass() != p)
            COMPlusThrow(kMissingMemberException,L"MissingMemberTypeRef");

        // Prevent making type references to primitives.  

        CorElementType t = pField->GetFieldType();
        if (CorIsPrimitiveType(t))
            COMPlusThrow(kArgumentException,L"Arg_TypeRefPrimitve");

        typeHnd = pField->LoadType();
        if (i<cnt-1) {
            if (!typeHnd.GetClass()->IsValueClass())
                COMPlusThrow(kMissingMemberException,L"MissingMemberNestErr");
        }
        offset += pField->GetOffset();
    }

        // Fields already are prohibted from having ArgIterator and RuntimeArgumentHandles
    _ASSERTE(!typeHnd.GetClass()->ContainsStackPtr());

    // Create the ByRef
    value->data = ((BYTE *)(target->GetAddress() + offset)) + sizeof(Object);
    value->type = typeHnd;

    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

// Equals
// This method will verify that two methods are equal....
FCIMPL2(INT32, COMMember::Equals, ReflectBaseObject* refThis, Object* obj)
{
    THROWSCOMPLUSEXCEPTION();

    if (refThis == NULL) 
        FCThrow(kNullReferenceException);

    if (!obj)
        return 0;
    if (refThis->GetClass() != obj->GetClass())
        return 0;

    REFLECTBASEREF  rb;
    INT32           retVal = 1;
    
//    HELPER_METHOD_FRAME_BEGIN_1(rb);
    rb = (REFLECTBASEREF) ObjectToOBJECTREF(obj);
    if (refThis->GetData() != rb->GetData() ||
        refThis->GetReflClass() != rb->GetReflClass())
    {
        retVal = 0;
    }
//    HELPER_METHOD_FRAME_END();
    return retVal;
}
FCIMPLEND

/*
// Equals
// This method will verify that two methods are equal....
INT32 __stdcall COMMember::TokenEquals(_TokenEqualsArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    if (!args->obj)
        return 0;
    if (args->refThis->GetClass() != args->obj->GetClass())
        return 0;

    // Check that the token is the same...
    REFLECTTOKENBASEREF rb = (REFLECTTOKENBASEREF) args->obj;
    if (args->refThis->GetToken() != rb->GetToken())
        return 0;
    return 1;
}
*/

// PropertyEquals
// Return true if the properties are the same...
FCIMPL2(INT32, COMMember::PropertyEquals, ReflectTokenBaseObject* refThisUNSAFE, Object* objUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    if (refThisUNSAFE == NULL) 
        FCThrowRes(kNullReferenceException, L"NullReference_This");

    if (!objUNSAFE)
        return 0;

    if (refThisUNSAFE->GetClass() != objUNSAFE->GetClass())
        return 0;

    //
    // We need to cast objUNSAFE down to its most derived class so that we call the 
    //  correct GetData method. We can do this because the above check essentially 
    //  verified that objUNSAFE is a ReflectTokenBaseObject
    //
    // the names are misleading as 'obj' is still not GC protected and is no
    // less unsafe than objUNSAFE...
    REFLECTTOKENBASEREF obj = (REFLECTTOKENBASEREF) ObjectToOBJECTREF(objUNSAFE);
    if (refThisUNSAFE->GetData() != obj->GetData())
        return 0;

    return 1;
}
FCIMPLEND

// GetAddMethod
// This will return the Add method for an Event
FCIMPL2(Object*, COMMember::GetAddMethod, ReflectTokenBaseObject* refThisUNSAFE, BYTE bNonPublic)
{
    THROWSCOMPLUSEXCEPTION();

    REFLECTTOKENBASEREF refThis     = (REFLECTTOKENBASEREF) refThisUNSAFE;
    REFLECTBASEREF      refMethod   = NULL;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refThis);

    if (refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    ReflectClass* pRC = (ReflectClass*) refThis->GetReflClass();
    ReflectEvent* pEvent = (ReflectEvent*) refThis->GetData();
    if (!pEvent->pAdd) 
        goto lExit;
    if (!bNonPublic && !pEvent->pAdd->IsPublic())
        goto lExit;

    {
    RefSecContext sCtx;

    if (!pEvent->pAdd->IsPublic())
        InvokeUtil::CheckAccess(&sCtx,
                                pEvent->pAdd->attrs,
                                pEvent->pAdd->pMethod->GetMethodTable(),
                                REFSEC_THROW_SECURITY);

    InvokeUtil::CheckLinktimeDemand(&sCtx, pEvent->pAdd->pMethod, true);
    }

    // Find the method...
    refMethod = pEvent->pAdd->GetMethodInfo(pRC);

lExit: ;
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refMethod);
}
FCIMPLEND

// GetRemoveMethod
// This method return the unsync method on an event
FCIMPL2(Object*, COMMember::GetRemoveMethod, ReflectTokenBaseObject* refThisUNSAFE, BYTE bNonPublic)
{
    THROWSCOMPLUSEXCEPTION();

    REFLECTTOKENBASEREF refThis     = (REFLECTTOKENBASEREF) refThisUNSAFE;
    REFLECTBASEREF      refMethod   = NULL;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refThis);

    if (refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    ReflectClass* pRC = (ReflectClass*) refThis->GetReflClass();
    ReflectEvent* pEvent = (ReflectEvent*) refThis->GetData();
    if (!pEvent->pRemove) 
        goto lExit;
    if (!bNonPublic && !pEvent->pRemove->IsPublic())
        goto lExit;

    {
    RefSecContext sCtx;

    if (!pEvent->pRemove->IsPublic())
        InvokeUtil::CheckAccess(&sCtx,
                                pEvent->pRemove->attrs,
                                pEvent->pRemove->pMethod->GetMethodTable(),
                                REFSEC_THROW_SECURITY);

    InvokeUtil::CheckLinktimeDemand(&sCtx, pEvent->pRemove->pMethod, true);
    }

    // Find the method...
    refMethod = pEvent->pRemove->GetMethodInfo(pRC);

lExit: ;
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refMethod);
}
FCIMPLEND

// GetRemoveMethod
// This method return the unsync method on an event
FCIMPL2(Object*, COMMember::GetRaiseMethod, ReflectTokenBaseObject* refThisUNSAFE, BYTE bNonPublic)
{
    THROWSCOMPLUSEXCEPTION();

    REFLECTTOKENBASEREF refThis     = (REFLECTTOKENBASEREF) refThisUNSAFE;
    REFLECTBASEREF      refMethod   = NULL;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refThis);

    if (refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    ReflectClass* pRC = (ReflectClass*) refThis->GetReflClass();
    ReflectEvent* pEvent = (ReflectEvent*) refThis->GetData();
    if (!pEvent->pFire) 
        goto lExit;
    if (!bNonPublic && !pEvent->pFire->IsPublic())
        goto lExit;

    {
    RefSecContext sCtx;

    if (!pEvent->pFire->IsPublic())
        InvokeUtil::CheckAccess(&sCtx,
                                pEvent->pFire->attrs,
                                pEvent->pFire->pMethod->GetMethodTable(),
                                REFSEC_THROW_SECURITY);

    InvokeUtil::CheckLinktimeDemand(&sCtx, pEvent->pFire->pMethod, true);
    }
    // Find the method...
    refMethod = pEvent->pFire->GetMethodInfo(pRC);

lExit: ;
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refMethod);
}
FCIMPLEND

// GetGetAccessors
// This method will return an array of the Get Accessors.  If there
//  are no GetAccessors then we will return an empty array
FCIMPL2(Object*, COMMember::GetAccessors, ReflectTokenBaseObject* refThisUNSAFE, BYTE bNonPublic)
{
    THROWSCOMPLUSEXCEPTION();

    REFLECTTOKENBASEREF refThis     = (REFLECTTOKENBASEREF) refThisUNSAFE;
    PTRARRAYREF         pRet;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refThis);

    if (refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    // There are three accessors

    ReflectClass* pRC = (ReflectClass*) refThis->GetReflClass();
    ReflectProperty* pProp = (ReflectProperty*) refThis->GetData();

    // check how many others
    int accCnt = 2;
    if (pProp->pOthers) {
        PropertyOtherList *item = pProp->pOthers;
        while (item) {
            accCnt++;
            item = item->pNext;
        }
    }
    ReflectMethod **pgRM = (ReflectMethod**)_alloca(sizeof(ReflectMethod*) * accCnt);
    memset(pgRM, 0, sizeof(ReflectMethod*) * accCnt);

    RefSecContext sCtx;
    MethodTable *pMT = pRC->GetClass()->GetMethodTable();

    accCnt = 0;
    if (bNonPublic) {
        if (pProp->pSetter &&
            InvokeUtil::CheckAccess(&sCtx, pProp->pSetter->attrs, pMT, 0) &&
            InvokeUtil::CheckLinktimeDemand(&sCtx, pProp->pSetter->pMethod, false))
            pgRM[accCnt++] = pProp->pSetter;
        if (pProp->pGetter &&
            InvokeUtil::CheckAccess(&sCtx, pProp->pGetter->attrs, pMT, 0) &&
            InvokeUtil::CheckLinktimeDemand(&sCtx, pProp->pGetter->pMethod, false))
            pgRM[accCnt++] = pProp->pGetter;
        if (pProp->pOthers) {
            PropertyOtherList *item = pProp->pOthers;
            while (item) {
                if (InvokeUtil::CheckAccess(&sCtx, item->pMethod->attrs, pMT, 0) &&
                    InvokeUtil::CheckLinktimeDemand(&sCtx, item->pMethod->pMethod, false))
                    pgRM[accCnt++] = item->pMethod;
                item = item->pNext;
            }
        }
    }
    else {
        if (pProp->pSetter &&
            IsMdPublic(pProp->pSetter->attrs) &&
            InvokeUtil::CheckLinktimeDemand(&sCtx, pProp->pSetter->pMethod, false))
            pgRM[accCnt++] = pProp->pSetter;
        if (pProp->pGetter &&
            IsMdPublic(pProp->pGetter->attrs) &&
            InvokeUtil::CheckLinktimeDemand(&sCtx, pProp->pGetter->pMethod, false))
            pgRM[accCnt++] = pProp->pGetter;
        if (pProp->pOthers) {
            PropertyOtherList *item = pProp->pOthers;
            while (item) {
                if (IsMdPublic(item->pMethod->attrs) &&
                    InvokeUtil::CheckLinktimeDemand(&sCtx, item->pMethod->pMethod, false)) 
                    pgRM[accCnt++] = item->pMethod;
                item = item->pNext;
            }
        }
    }


    pRet = (PTRARRAYREF) AllocateObjectArray(accCnt, g_pRefUtil->GetTrueType(RC_Method));
    GCPROTECT_BEGIN(pRet);
    for (int i=0;i<accCnt;i++) {
        REFLECTBASEREF refMethod = pgRM[i]->GetMethodInfo(pRC);
        pRet->SetAt(i, (OBJECTREF) refMethod);
    }

    GCPROTECT_END();

    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(pRet);
}
FCIMPLEND

// InternalSetter
// This method will return the Set Accessor method on a property
FCIMPL3(Object*, COMMember::InternalSetter, ReflectTokenBaseObject* refThisUNSAFE, BYTE bNonPublic, BYTE bVerifyAccess)
{
    THROWSCOMPLUSEXCEPTION();

    REFLECTTOKENBASEREF refThis     = (REFLECTTOKENBASEREF) refThisUNSAFE;
    REFLECTBASEREF      refMethod   = NULL;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refThis);

    if (refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    ReflectClass* pRC = (ReflectClass*) refThis->GetReflClass();
    ReflectProperty* pProp = (ReflectProperty*) refThis->GetData();
    if (!pProp->pSetter) 
        goto lExit;
    if (!bNonPublic && !pProp->pSetter->IsPublic())
        goto lExit;

    {
    RefSecContext sCtx;

        if (!pProp->pSetter->IsPublic() && bVerifyAccess)
        InvokeUtil::CheckAccess(&sCtx,
                                pProp->pSetter->attrs,
                                pProp->pSetter->pMethod->GetMethodTable(),
                                REFSEC_THROW_SECURITY);

    // If the method has a linktime security demand attached, check it now.
    InvokeUtil::CheckLinktimeDemand(&sCtx, pProp->pSetter->pMethod, true);
    }

    // Find the method...
    refMethod = pProp->pSetter->GetMethodInfo(pRC);

lExit: ;
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refMethod);
}
FCIMPLEND

// InternalGetter
// This method will return the Get Accessor method on a property
FCIMPL3(Object*, COMMember::InternalGetter, ReflectTokenBaseObject* refThisUNSAFE, BYTE bNonPublic, BYTE bVerifyAccess)
{
    THROWSCOMPLUSEXCEPTION();

    REFLECTTOKENBASEREF refThis     = (REFLECTTOKENBASEREF) refThisUNSAFE;
    REFLECTBASEREF      refMethod   = NULL;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refThis);

    if (refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    ReflectClass* pRC = (ReflectClass*) refThis->GetReflClass();
    ReflectProperty* pProp = (ReflectProperty*) refThis->GetData();
    if (!pProp->pGetter) 
        goto lExit;
    if (!bNonPublic && !pProp->pGetter->IsPublic())
        goto lExit;

    {
    RefSecContext sCtx;

        if (!pProp->pGetter->IsPublic() && bVerifyAccess)
        InvokeUtil::CheckAccess(&sCtx,
                                pProp->pGetter->attrs,
                                pProp->pGetter->pMethod->GetMethodTable(),
                                REFSEC_THROW_SECURITY);

    // If the method has a linktime security demand attached, check it now.
    InvokeUtil::CheckLinktimeDemand(&sCtx, pProp->pGetter->pMethod, true);
    }

    refMethod = pProp->pGetter->GetMethodInfo(pRC);
lExit: ;
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refMethod);
}
FCIMPLEND

// PublicProperty
// This method will check to see if the passed property has any
//  public accessors (Making it publically exposed.)
bool COMMember::PublicProperty(ReflectProperty* pProp)
{
    _ASSERTE(pProp);

    if (pProp->pSetter && IsMdPublic(pProp->pSetter->attrs))
        return true;
    if (pProp->pGetter && IsMdPublic(pProp->pGetter->attrs))
        return true;
    return false;
}

// StaticProperty
// This method will check to see if any of the accessor are static
//  which will make it a static property
bool COMMember::StaticProperty(ReflectProperty* pProp)
{
    _ASSERTE(pProp);

    if (pProp->pSetter && IsMdStatic(pProp->pSetter->attrs))
        return true;
    if (pProp->pGetter && IsMdStatic(pProp->pGetter->attrs))
        return true;
    return false;
}

// PublicEvent
// This method looks at each of the event accessors, if any
//  are public then the event is considered public
bool COMMember::PublicEvent(ReflectEvent* pEvent)
{
    _ASSERTE(pEvent);

    if (pEvent->pAdd && IsMdPublic(pEvent->pAdd->attrs))
        return true;
    if (pEvent->pRemove && IsMdPublic(pEvent->pRemove->attrs))
        return true;
    if (pEvent->pFire && IsMdPublic(pEvent->pFire->attrs))
        return true;
    return false;
}

// StaticEvent
// This method will check to see if any of the accessor are static
//  which will make it a static event
bool COMMember::StaticEvent(ReflectEvent* pEvent)
{
    _ASSERTE(pEvent);

    if (pEvent->pAdd && IsMdStatic(pEvent->pAdd->attrs))
        return true;
    if (pEvent->pRemove && IsMdStatic(pEvent->pRemove->attrs))
        return true;
    if (pEvent->pFire && IsMdStatic(pEvent->pFire->attrs))
        return true;
    return false;
}

// IsReadOnly
// This method will return a boolean value indicating if the Property is
//  a ReadOnly property.  This is defined by the lack of a Set Accessor method.
FCIMPL1(INT32, COMMember::CanRead, ReflectTokenBaseObject* refThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    REFLECTTOKENBASEREF refThis     = (REFLECTTOKENBASEREF) refThisUNSAFE;
    INT32               rv;
    HELPER_METHOD_FRAME_BEGIN_RET_1(refThis);

    if (refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    ReflectProperty* pProp = (ReflectProperty*) refThis->GetData();
    rv = (pProp->pGetter) ? 1 : 0;

    HELPER_METHOD_FRAME_END();
    return rv;
}
FCIMPLEND

// IsWriteOnly
// This method will return a boolean value indicating if the Property is
//  a WriteOnly property.  This is defined by the lack of a Get Accessor method.
FCIMPL1(INT32, COMMember::CanWrite, ReflectTokenBaseObject* refThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    REFLECTTOKENBASEREF refThis     = (REFLECTTOKENBASEREF) refThisUNSAFE;
    INT32               rv;
    HELPER_METHOD_FRAME_BEGIN_RET_1(refThis);

    if (refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    ReflectProperty* pProp = (ReflectProperty*) refThis->GetData();
    rv = (pProp->pSetter) ? 1 : 0;

    HELPER_METHOD_FRAME_END();
    return rv;
}
FCIMPLEND

// InternalGetCurrentMethod
// Return the MethodInfo that represents the current method (two above this one)
FCIMPL1(Object*, COMMember::InternalGetCurrentMethod, StackCrawlMark* stackMark)
{
    OBJECTREF o = NULL;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, o);
    SkipStruct skip;
    skip.pStackMark = stackMark;
    skip.pMeth = 0;
    StackWalkFunctions(GetThread(), SkipMethods, &skip);
    if (NULL != skip.pMeth)
    {
        o = COMMember::g_pInvokeUtil->GetMethodInfo(skip.pMeth);
    }

    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(o);
}
FCIMPLEND

// This method is called by the GetMethod function and will crawl backward
//  up the stack for integer methods.
StackWalkAction COMMember::SkipMethods(CrawlFrame* frame, VOID* data)
{
    SkipStruct* pSkip = (SkipStruct*) data;

    MethodDesc *pFunc = frame->GetFunction();

    /* We asked to be called back only for functions */
    _ASSERTE(pFunc);

    // The check here is between the address of a local variable
    // (the stack mark) and a pointer to the EIP for a frame
    // (which is actually the pointer to the return address to the
    // function from the previous frame). So we'll actually notice
    // which frame the stack mark was in one frame later. This is
    // fine since we only implement LookForMyCaller.
    _ASSERTE(*pSkip->pStackMark == LookForMyCaller);
    if (!IsInCalleesFrames(frame->GetRegisterSet(), pSkip->pStackMark))
        return SWA_CONTINUE;

    pSkip->pMeth = pFunc;
    return SWA_ABORT;
}

// GetFieldType
// This method will return a Class object which represents the
//  type of the field.
FCIMPL1(Object*, COMMember::GetFieldType, ReflectTokenBaseObject* refThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    REFLECTTOKENBASEREF refThis     = (REFLECTTOKENBASEREF) refThisUNSAFE;
    OBJECTREF           ret         = NULL;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refThis);

    if (refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    PCCOR_SIGNATURE pSig;
    DWORD           cSig;
    TypeHandle      typeHnd;

    // Get the field
    ReflectField* pRF = (ReflectField*) refThis->GetData();
    FieldDesc* pFld = pRF->pField;
    _ASSERTE(pFld);

    // Get the signature
    pFld->GetSig(&pSig, &cSig);
    FieldSig sig(pSig, pFld->GetModule());

    OBJECTREF throwable = NULL;
    GCPROTECT_BEGIN(throwable);
    typeHnd = sig.GetTypeHandle(&throwable);
    if (typeHnd.IsNull())
        COMPlusThrow(throwable);
    GCPROTECT_END();

    // Ignore null return
    ret = typeHnd.CreateClassObj();

    HELPER_METHOD_FRAME_END();
    return(OBJECTREFToObject(ret));
}
FCIMPLEND

// GetBaseDefinition
// Return the MethodInfo that represents the first definition of this
//  virtual method.
FCIMPL1(Object*, COMMember::GetBaseDefinition, ReflectBaseObject* refThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    REFLECTBASEREF  refThis = (REFLECTBASEREF) refThisUNSAFE;
    OBJECTREF       refRet  = NULL;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refThis);

    if (refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    MethodDesc*     pMeth;
    WORD            slot;
    EEClass*        pEEC;

    // Get the method Descr  (this should not fail)
    //  NOTE: both a constructor and a method are represented by a MetodDesc.
    //      If this ever changes we will need to fix this.
    ReflectMethod* pRM = (ReflectMethod*) refThis->GetData();
    pMeth = pRM->pMethod;
    _ASSERTE(pMeth);

    slot = pMeth->GetSlot();
    pEEC = pMeth->GetClass();

    // If this is an interface then this is the base definition...
    if (pEEC->IsInterface()) {
        refRet = refThis;
        goto lExit;
    }

    // If this isn't in the VTable then it must be the real implementation...
    if (slot > pEEC->GetNumVtableSlots()) {
        refRet = refThis;
        goto lExit;
    }

    // Find the first definition of this in the hierarchy....
    pEEC = pEEC->GetParentClass();
    while (pEEC) {
        WORD vtCnt = pEEC->GetNumVtableSlots();
        if (vtCnt <= slot)
            break;
        pMeth = pEEC->GetMethodDescForSlot(slot);
        pEEC = pMeth->GetClass();
        if (!pEEC)
            break;
        pEEC = pEEC->GetParentClass();
    }

    // Find the Object so we can get its version of the MethodInfo...
    _ASSERTE(pMeth);
    refRet = g_pInvokeUtil->GetMethodInfo(pMeth);

lExit: ;
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refRet);
}
FCIMPLEND

// GetParentDefinition
// Return the MethodInfo that represents the previous definition of this
//  virtual method in the inheritance chain.
FCIMPL1(Object*, COMMember::GetParentDefinition, ReflectBaseObject* refThisUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    REFLECTBASEREF  refThis = (REFLECTBASEREF) refThisUNSAFE;
    OBJECTREF       refRet  = NULL;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refThis);

    if (refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    MethodDesc*     pMeth;
    WORD            slot;
    EEClass*        pEEC;

    // Get the method Descr  (this should not fail)
    //  NOTE: both a constructor and a method are represented by a MetodDesc.
    //      If this ever changes we will need to fix this.
    ReflectMethod* pRM = (ReflectMethod*) refThis->GetData();
    pMeth = pRM->pMethod;
    _ASSERTE(pMeth);

    slot = pMeth->GetSlot();
    pEEC = pMeth->GetClass();

    WORD vtCnt = 0;

    // If this is an interface then there is no Parent Definition.
    // Get a null back serves as a terminal condition.
    if (pEEC->IsInterface()) {
        goto lExit;
    }

    // Find the parents definition of this in the hierarchy....
    pEEC = pEEC->GetParentClass();
    if (!pEEC)
        goto lExit;

    vtCnt = pEEC->GetNumVtableSlots();
    if (vtCnt <= slot) // If this isn't in the VTable then it must be the real implementation...
        goto lExit;
    pMeth = pEEC->GetMethodDescForSlot(slot);
    
    // Find the Object so we can get its version of the MethodInfo...
    _ASSERTE(pMeth);
    refRet = g_pInvokeUtil->GetMethodInfo(pMeth);

lExit: ;
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refRet);
}
FCIMPLEND

// GetTypeHandleImpl
// This method will return the MethodTypeHandle for a MethodInfo object
FCIMPL1(void*, COMMember::GetMethodHandleImpl, ReflectBaseObject* method) {
    VALIDATEOBJECTREF(method);

    ReflectMethod* pRM = (ReflectMethod*) method->GetData();
    _ASSERTE(pRM->pMethod);
    return pRM->pMethod;
}
FCIMPLEND

// GetMethodFromHandleImp
// This is a static method which will return a MethodInfo object based
//  upon the passed in Handle.
FCIMPL1(Object*, COMMember::GetMethodFromHandleImp, LPVOID handle) {

    OBJECTREF objMeth;
    MethodDesc* pMeth = (MethodDesc*) handle;
    if (pMeth == 0)
        FCThrowArgumentEx(kArgumentException, NULL, L"InvalidOperation_HandleIsNotInitialized");

    HELPER_METHOD_FRAME_BEGIN_RET_0();
    REFLECTCLASSBASEREF pRefClass = (REFLECTCLASSBASEREF) pMeth->GetClass()->GetExposedClassObject();

    if (pMeth->IsCtor() || pMeth->IsStaticInitMethod()) {
        ReflectMethod* pRM = ((ReflectClass*) pRefClass->GetData())->FindReflectConstructor(pMeth);
        objMeth = (OBJECTREF) pRM->GetConstructorInfo((ReflectClass*) pRefClass->GetData());
    }
    else {
        ReflectMethod* pRM = ((ReflectClass*) pRefClass->GetData())->FindReflectMethod(pMeth);
        objMeth = (OBJECTREF) pRM->GetMethodInfo((ReflectClass*) pRefClass->GetData());
    }
    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(objMeth);
}
FCIMPLEND

FCIMPL1(size_t, COMMember::GetFunctionPointer, size_t pMethodDesc) {
    MethodDesc *pMD = (MethodDesc*)pMethodDesc;
    if (pMD)
        return (size_t)pMD->GetPreStubAddr();
    return 0;
}
FCIMPLEND

// GetFieldHandleImpl
// This method will return the RuntimeFieldHandle for a FieldInfo object
FCIMPL1(void*, COMMember::GetFieldHandleImpl, ReflectBaseObject* field) {
    VALIDATEOBJECTREF(field);

    ReflectField* pRF = (ReflectField*) field->GetData();
    _ASSERTE(pRF->pField);
    
    return pRF->pField;
}
FCIMPLEND

// GetFieldFromHandleImp
// This is a static method which will return a FieldInfo object based
//  upon the passed in Handle.
FCIMPL1(Object*, COMMember::GetFieldFromHandleImp, LPVOID handle) {

    OBJECTREF objMeth;
    FieldDesc* pField = (FieldDesc*) handle;
    if (pField == 0)
        FCThrowArgumentEx(kArgumentException, NULL, L"InvalidOperation_HandleIsNotInitialized");
    
    HELPER_METHOD_FRAME_BEGIN_RET_0();
    REFLECTCLASSBASEREF pRefClass = (REFLECTCLASSBASEREF) pField->GetEnclosingClass()->GetExposedClassObject();

    ReflectField* pRF = ((ReflectClass*) pRefClass->GetData())->FindReflectField(pField);
    objMeth = (OBJECTREF) pRF->GetFieldInfo((ReflectClass*) pRefClass->GetData());
    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(objMeth);
}
FCIMPLEND

// DBCanConvertPrimitive
// This method checks to see of the source class can be widdened
//  to the target class.  This is a private routine so no error checking is
//  done.
FCIMPL2(INT32, COMMember::DBCanConvertPrimitive, ReflectClassBaseObject* source, ReflectClassBaseObject* target) {
    VALIDATEOBJECTREF(source);
    VALIDATEOBJECTREF(target);

    ReflectClass* pSRC = (ReflectClass*) source->GetData();
    _ASSERTE(pSRC);
    ReflectClass* pTRG = (ReflectClass*) target->GetData();
    _ASSERTE(pTRG);
    CorElementType tSRC = pSRC->GetCorElementType();
    CorElementType tTRG = pTRG->GetCorElementType();

    INT32 ret = InvokeUtil::IsPrimitiveType(tTRG) && InvokeUtil::CanPrimitiveWiden(tTRG,tSRC);
    FC_GC_POLL_RET();
    return ret;
}
FCIMPLEND
// DBCanConvertObjectPrimitive
// This method returns a boolean indicating if the source object can be 
//  converted to the target primitive.
FCIMPL2(INT32, COMMember::DBCanConvertObjectPrimitive, Object* sourceObj, ReflectClassBaseObject* target) {
    VALIDATEOBJECTREF(sourceObj);
    VALIDATEOBJECTREF(target);

    if (sourceObj == 0)
        return 1;
    MethodTable* pMT = sourceObj->GetMethodTable();
    CorElementType tSRC = pMT->GetNormCorElementType();

    ReflectClass* pTRG = (ReflectClass*) target->GetData();
    _ASSERTE(pTRG);
    CorElementType tTRG = pTRG->GetCorElementType();
    INT32 ret = InvokeUtil::IsPrimitiveType(tTRG) && InvokeUtil::CanPrimitiveWiden(tTRG,tSRC);
    FC_GC_POLL_RET();
    return ret;
}
FCIMPLEND

FCIMPL1(Object *, COMMember::InternalGetEnumUnderlyingType, ReflectClassBaseObject *target)
{
    VALIDATEOBJECTREF(target);

    ReflectClass* pSRC = (ReflectClass*) target->GetData();
    _ASSERTE(pSRC);
    TypeHandle th = pSRC->GetTypeHandle();

    if (!th.IsEnum())
        FCThrowArgument(NULL, NULL);
    OBJECTREF result = NULL;

    HELPER_METHOD_FRAME_BEGIN_RET_0();
    EEClass *pClass = g_Mscorlib.FetchElementType(th.AsMethodTable()->GetNormCorElementType())->GetClass();

    result = pClass->GetExistingExposedClassObject();

    if (result == NULL)
    {
        result = pClass->GetExposedClassObject();
    }
    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(result);
} 
FCIMPLEND

FCIMPL1(Object *, COMMember::InternalGetEnumValue, Object *pRefThis)
{
    VALIDATEOBJECTREF(pRefThis);

    if (pRefThis == NULL)
        FCThrowArgumentNull(NULL);

    OBJECTREF result = NULL;

    HELPER_METHOD_FRAME_BEGIN_RET_1(pRefThis);

    MethodTable *pMT = g_Mscorlib.FetchElementType(pRefThis->GetTrueMethodTable()->GetNormCorElementType());
    result = AllocateObject(pMT);
    CopyValueClass(result->UnBox(), pRefThis->UnBox(), pMT, GetAppDomain());
    
    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(result);
}
FCIMPLEND

FCIMPL3(void, COMMember::InternalGetEnumValues, 
        ReflectClassBaseObject *target, Object **pReturnValues, Object **pReturnNames)
{
    VALIDATEOBJECTREF(target);

    HELPER_METHOD_FRAME_BEGIN_NOPOLL();
    
    THROWSCOMPLUSEXCEPTION();

    ReflectClass* pSRC = (ReflectClass*) target->GetData();
    _ASSERTE(pSRC);
    TypeHandle th = pSRC->GetTypeHandle();

    if (!th.IsEnum())
        COMPlusThrow(kArgumentException, L"Arg_MustBeEnum");

    EnumEEClass *pClass = (EnumEEClass*) th.AsClass();

    HRESULT hr = pClass->BuildEnumTables();
    if (FAILED(hr))
        COMPlusThrowHR(hr);

    DWORD cFields = pClass->GetEnumCount();
    
    struct gc {
        I8ARRAYREF values;
        PTRARRAYREF names;
    } gc;
    gc.values = NULL;
    gc.names = NULL;
    GCPROTECT_BEGIN(gc);

    gc.values = (I8ARRAYREF) AllocatePrimitiveArray(ELEMENT_TYPE_U8, cFields);

    CorElementType type = pClass->GetMethodTable()->GetNormCorElementType();
    INT64 *pToValues = gc.values->GetDirectPointerToNonObjectElements();

	DWORD i;
    for (i=0;i<cFields;i++)
    {
        switch (type)
        {
        case ELEMENT_TYPE_I1:
            pToValues[i] = (INT8) pClass->GetEnumByteValues()[i];
            break;

        case ELEMENT_TYPE_U1:
        case ELEMENT_TYPE_BOOLEAN:
            pToValues[i] = pClass->GetEnumByteValues()[i];
            break;
    
        case ELEMENT_TYPE_I2:
            pToValues[i] = (INT16) pClass->GetEnumShortValues()[i];
            break;

        case ELEMENT_TYPE_U2:
        case ELEMENT_TYPE_CHAR:
            pToValues[i] = pClass->GetEnumShortValues()[i];
            break;
    
        case ELEMENT_TYPE_I4:
        case ELEMENT_TYPE_I:
            pToValues[i] = (INT32) pClass->GetEnumIntValues()[i];
            break;

        case ELEMENT_TYPE_U4:
        case ELEMENT_TYPE_U:
            pToValues[i] = pClass->GetEnumIntValues()[i];
            break;

        case ELEMENT_TYPE_I8:
        case ELEMENT_TYPE_U8:
            pToValues[i] = pClass->GetEnumLongValues()[i];
            break;

        default:
		    break;
        }
    }
    
    gc.names = (PTRARRAYREF) AllocateObjectArray(cFields, g_pStringClass);
    
    LPCUTF8 *pNames = pClass->GetEnumNames();
    for (i=0;i<cFields;i++)
    {
        STRINGREF str = COMString::NewString(pNames[i]);
        gc.names->SetAt(i, str);
    }
    
    *pReturnValues = OBJECTREFToObject(gc.values);
    *pReturnNames = OBJECTREFToObject(gc.names);

    GCPROTECT_END();
    
    HELPER_METHOD_FRAME_END_POLL();
}
FCIMPLEND


// InternalBoxEnumI8
// This method will create an Enum object and place the value inside
//  it and return it.  The Type has been validated before calling.
FCIMPL2(Object*, COMMember::InternalBoxEnumI8, ReflectClassBaseObject* target, INT64 value) 
{
    VALIDATEOBJECTREF(target);
    OBJECTREF ret;

    ReflectClass* pSRC = (ReflectClass*) target->GetData();
    _ASSERTE(pSRC);
    MethodTable* pMT = pSRC->GetTypeHandle().AsMethodTable();
    HELPER_METHOD_FRAME_BEGIN_RET_0();
    ret = AllocateObject(pMT);
    C_ASSERT(sizeof(value) == sizeof(ARG_SLOT));
    CopyValueClass(ret->UnBox(), 
        ArgSlotEndianessFixup((ARG_SLOT*)&value, pMT->GetClass()->GetNumInstanceFieldBytes()),
        pMT, ret->GetAppDomain());
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(ret);
}
FCIMPLEND

// InternalBoxEnumU8
// This method will create an Enum object and place the value inside
//  it and return it.  The Type has been validated before calling.
FCIMPL2(Object*, COMMember::InternalBoxEnumU8, ReflectClassBaseObject* target, UINT64 value) 
{
    VALIDATEOBJECTREF(target);
    OBJECTREF ret;

    ReflectClass* pSRC = (ReflectClass*) target->GetData();
    _ASSERTE(pSRC);
    MethodTable* pMT = pSRC->GetTypeHandle().AsMethodTable();
    HELPER_METHOD_FRAME_BEGIN_RET_0();
    ret = AllocateObject(pMT);
    C_ASSERT(sizeof(value) == sizeof(ARG_SLOT));
    CopyValueClass(ret->UnBox(), 
        ArgSlotEndianessFixup((ARG_SLOT*)&value, pMT->GetClass()->GetNumInstanceFieldBytes()),
        pMT, ret->GetAppDomain());
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(ret);
}
FCIMPLEND

FCIMPL1(INT32, COMMember::IsOverloaded, ReflectBaseObject* refThisUNSAFE)
{
    REFLECTBASEREF  refThis = (REFLECTBASEREF) refThisUNSAFE;
    INT32           rv      = false;
    HELPER_METHOD_FRAME_BEGIN_RET_1(refThis);

    ReflectClass* pRC = (ReflectClass *)refThis->GetReflClass();
    _ASSERTE(pRC);

    ReflectMethodList* pMeths = NULL;

    ReflectMethod *pRM = (ReflectMethod *)refThis->GetData();
    MethodDesc *pMeth = pRM->pMethod;
    LPCUTF8 szMethName = pMeth->GetName();

    int matchingMethods = 0;
    // See if this is a ctor
    if (IsMdInstanceInitializer(pRM->attrs, szMethName))
    {
        pMeths = pRC->GetConstructors();

        matchingMethods = pMeths->dwMethods;
    }
    else if (IsMdClassConstructor(pRM->attrs, szMethName))
    {
        // You can never overload a class constructor!
        matchingMethods = 0;
    }
    else
    {
        _ASSERTE(!IsMdRTSpecialName(pRM->attrs));

        pMeths = pRC->GetMethods();

        ReflectMethod *p = pMeths->hash.Get(szMethName);

        while (p)
        {
            if (!strcmp(p->szName, szMethName))
            {
                matchingMethods++;
                if (matchingMethods > 1)
                {
                    rv = true;
                    goto lExit;
                }

            }

            p = p->pNext;

        }
    }
    if (matchingMethods > 1)
    {
        rv = true;
}

lExit: ;
    HELPER_METHOD_FRAME_END();
    return rv;
}
FCIMPLEND

FCIMPL1(INT32, COMMember::HasLinktimeDemand, ReflectBaseObject* refThisUNSAFE)
{
    ReflectMethod*  pRM = (ReflectMethod*)refThisUNSAFE->GetData();
    INT32           rv;

    HELPER_METHOD_FRAME_BEGIN_RET_0();
    rv = pRM->pMethod->RequiresLinktimeCheck();
    HELPER_METHOD_FRAME_END();
    return rv;
}
FCIMPLEND


