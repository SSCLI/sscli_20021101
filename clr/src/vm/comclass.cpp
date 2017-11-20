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
// Date: March 27, 1998
////////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "comclass.h"
#include "reflectutil.h"
#include "comvariant.h"
#include "comstring.h"
#include "commember.h"
#include "commodule.h"
#include "comarrayinfo.h"
#include "corerror.h"
#include "gcscan.h"
#include "method.hpp"
#include "field.h"
#include "assemblynative.hpp"
#include "appdomain.hpp"
#include "comreflectioncache.hpp"
#include "eeconfig.h"
#include "comcodeaccesssecurityengine.h"
#include "security.h"
#include "customattribute.h"


/*======================================================================================
** COMClass data
**/
bool         COMClass::m_fAreReflectionStructsInitialized = false;

MethodTable* COMClass::m_pMTRC_Class = NULL;
FieldDesc*   COMClass::m_pDescrTypes = NULL;
FieldDesc*   COMClass::m_pDescrRetType = NULL;
FieldDesc*   COMClass::m_pDescrRetModType = NULL;
FieldDesc*   COMClass::m_pDescrMatchFlag = NULL;
//FieldDesc*   COMClass::m_pDescrCallConv = NULL;
FieldDesc*   COMClass::m_pDescrAttributes = NULL;

LONG COMClass::m_ReflectCrstInitialized = 0;
CRITICAL_SECTION    COMClass::m_ReflectCrst;
CRITICAL_SECTION    *COMClass::m_pReflectCrst = NULL;

//The serialization bit work is temporary until 3/15/2000.  After that point, we will
//always check the serialization bit.
#define SERIALIZATION_BIT_UNKNOWN   0xFFFFFFFF
#define SERIALIZATION_BIT_ZERO      0x0
#define SERIALIZATION_BIT_KEY       L"IgnoreSerializationBit"
#define SERIALIZATION_LOG_KEY       L"LogNonSerializable"
DWORD COMClass::m_checkSerializationBit = SERIALIZATION_BIT_UNKNOWN;

Assembly *GetCallersAssembly(StackCrawlMark *stackMark, void *returnIP)
{
    Assembly *pCallersAssembly = NULL;
    if (stackMark)
        pCallersAssembly = SystemDomain::GetCallersAssembly(stackMark);
    else {
        MethodDesc *pCallingMD = IP2MethodDesc((const BYTE *)returnIP);
        if (pCallingMD)
            pCallersAssembly = pCallingMD->GetAssembly();
        else
            // If we failed to determine the caller's method desc, this might
            // indicate a late bound call through reflection. Attempt to
            // determine the real caller through the slower stackwalk method.
            pCallersAssembly = SystemDomain::GetCallersAssembly((StackCrawlMark*)NULL);
    }

    return pCallersAssembly;
}

EEClass *GetCallersClass(StackCrawlMark *stackMark, void *returnIP)
{
    EEClass *pCallersClass = NULL;
    if (stackMark)
        pCallersClass = SystemDomain::GetCallersClass(stackMark);
    else {
        MethodDesc *pCallingMD = IP2MethodDesc((const BYTE *)returnIP);
        if (pCallingMD)
            pCallersClass = pCallingMD->GetClass();
        else
            // If we failed to determine the caller's method desc, this might
            // indicate a late bound call through reflection. Attempt to
            // determine the real caller through the slower stackwalk method.
            pCallersClass = SystemDomain::GetCallersClass((StackCrawlMark*)NULL);
    }

    return pCallersClass;
}

FCIMPL5(Object*, COMClass::GetMethodFromCache, ReflectClassBaseObject* _refThis, StringObject* _name, INT32 invokeAttr, INT32 argCnt, PtrArray* _args)
{
    MemberMethodsCache *pMemberMethodsCache = GetAppDomain()->GetRefMemberMethodsCache();

    REFLECTCLASSBASEREF refThis = REFLECTCLASSBASEREF(_refThis);
    STRINGREF name = STRINGREF(_name);
    PTRARRAYREF args = PTRARRAYREF(_args);
    
    _ASSERTE (argCnt < 6);
    MemberMethods vMemberMethods;
    vMemberMethods.pRC = (ReflectClass*) refThis->GetData();
    vMemberMethods.name = &name;
    vMemberMethods.argCnt = argCnt;
    vMemberMethods.invokeAttr = invokeAttr;
    OBJECTREF* argArray = args->m_Array;
    for (int i = 0; i < argCnt; i ++)
        vMemberMethods.vArgType[i] = (argArray[i] != 0)?argArray[i]->GetMethodTable():0;

    OBJECTREF method;
    if (!pMemberMethodsCache->GetFromCache (&vMemberMethods, method))
        method = NULL;

    FC_GC_POLL_AND_RETURN_OBJREF(OBJECTREFToObject(method));
}
FCIMPLEND

FCIMPL6(void,COMClass::AddMethodToCache, ReflectClassBaseObject* refThis, StringObject* name, INT32 invokeAttr, INT32 argCnt, PtrArray* args, Object* invokeMethod)
{
    MemberMethodsCache *pMemberMethodsCache = GetAppDomain()->GetRefMemberMethodsCache();
    _ASSERTE (pMemberMethodsCache);
    _ASSERTE (argCnt < 6);
    MemberMethods vMemberMethods;
    vMemberMethods.pRC = (ReflectClass*) REFLECTCLASSBASEREF(refThis)->GetData();
    vMemberMethods.name = (STRINGREF*) &name;
    vMemberMethods.argCnt = argCnt;
    vMemberMethods.invokeAttr = invokeAttr;
    OBJECTREF *argArray = (OBJECTREF*)((BYTE*)args + args->GetDataOffset());
    for (int i = 0; i < argCnt; i ++)
        vMemberMethods.vArgType[i] = !argArray[i] ? 0 : argArray[i]->GetMethodTable();
    pMemberMethodsCache->AddToCache (&vMemberMethods, ObjectToOBJECTREF((Object *)invokeMethod));

    FC_GC_POLL();
}
FCIMPLEND

void COMClass::InitializeReflectCrst()
{   
    // There are 3 cases when we come here
    // 1. m_ReflectCrst has not been initialized (m_pReflectCrst == 0)
    // 2. m_ReflectCrst is being initialized (m_pReflectCrst == 1)
    // 3. m_ReflectCrst has been initialized (m_pReflectCrst != 0 and m_pReflectCrst != 1)

    if (m_pReflectCrst == NULL)
    {   
        if (InterlockedCompareExchange(&m_ReflectCrstInitialized, 1, 0) == 0)
        {
            // first one to get in does the initialization
            InitializeCriticalSection(&m_ReflectCrst);
            m_pReflectCrst = &m_ReflectCrst;
        }
        else 
        {
            while (m_pReflectCrst == NULL)
                __SwitchToThread(0);
        }
    }


}

// MinimalReflectionInit
// This method will intialize reflection.  It is executed once.
//  This method is synchronized so multiple threads don't attempt to 
//  initalize reflection.
void COMClass::MinimalReflectionInit()
{

    Thread  *thread = GetThread();

    _ASSERTE(thread->PreemptiveGCDisabled());

    thread->EnablePreemptiveGC();
    LOCKCOUNTINCL("MinimalReflectionInit in COMClass.cpp");

    InitializeReflectCrst();

    EnterCriticalSection(&m_ReflectCrst);
    thread->DisablePreemptiveGC();

    if (m_fAreReflectionStructsInitialized) {
        LeaveCriticalSection(&m_ReflectCrst);
        LOCKCOUNTDECL("MinimalReflectionInit in COMClass.cpp");

        return;
    }

    COMMember::CreateReflectionArgs();
    ReflectUtil::Create();
    // At various places we just assume Void has been loaded and m_NormType initialized
    MethodTable* pVoidMT = g_Mscorlib.FetchClass(CLASS__VOID);
    pVoidMT->m_NormType = ELEMENT_TYPE_VOID;

    // Prevent recursive entry...
    m_fAreReflectionStructsInitialized = true;
    LeaveCriticalSection(&m_ReflectCrst);
    LOCKCOUNTDECL("MinimalReflectionInit in COMClass.cpp");

}

MethodTable *COMClass::GetRuntimeType()
{
    if (m_pMTRC_Class)
        return m_pMTRC_Class;

    MinimalReflectionInit();
    _ASSERTE(g_pRefUtil);

    m_pMTRC_Class = g_pRefUtil->GetClass(RC_Class);
    _ASSERTE(m_pMTRC_Class);

    return m_pMTRC_Class;
}

// This is called during termination...

// See if a Type object for the given Array already exists.  May very 
// return NULL.
OBJECTREF COMClass::QuickLookupExistingArrayClassObj(ArrayTypeDesc* arrayType) 
{
    // This is designed to be called from FCALL, and we don't want any GC allocations.
    // So make sure Type class has been loaded
    if (!m_pMTRC_Class)
        return NULL;

    // Lookup the array to see if we have already built it.
    ReflectArrayClass* newArray = (ReflectArrayClass*)
        arrayType->GetReflectClassIfExists();
    if (!newArray) {
        return NULL;
    }
    return newArray->GetClassObject();
}

// This will return the Type handle for an object.  It doesn't create
//  the Type Object when called.
FCIMPL1(void*, COMClass::GetTHFromObject, Object* obj)
    if (obj==NULL)
        FCThrowArgumentNull(L"obj");

    VALIDATEOBJECTREF(obj);
    return obj->GetMethodTable();
FCIMPLEND


// This will determine if a class represents a ByRef.
FCIMPL1(INT32, COMClass::IsByRefImpl, ReflectClassBaseObject* refThis)
    VALIDATEOBJECTREF(refThis);

    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    _ASSERTE(pRC);
    return pRC->IsByRef();
FCIMPLEND

// This will determine if a class represents a ByRef.
FCIMPL1(INT32, COMClass::IsPointerImpl, ReflectClassBaseObject* refThis) {
    VALIDATEOBJECTREF(refThis);

    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    _ASSERTE(pRC);
    TypeHandle th = pRC->GetTypeHandle();
    return (th.GetSigCorElementType() == ELEMENT_TYPE_PTR) ? 1 : 0;
}
FCIMPLEND

// IsPointerImpl
// This method will return a boolean indicating if the Type
//  object is a ByRef
FCIMPL1(INT32, COMClass::IsNestedTypeImpl, ReflectClassBaseObject* refThis)
{
    VALIDATEOBJECTREF(refThis);

    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    EEClass* pEEC = pRC->GetClass();
    return (pEEC && pEEC->IsNested()) ? 1 : 0;
}
FCIMPLEND

// GetNestedDeclaringType
// Return the declaring class for a nested type.
FCIMPL1(Object*, COMClass::GetNestedDeclaringType, ReflectClassBaseObject* refThis)
{
    VALIDATEOBJECTREF(refThis);
    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    EEClass* pEEC = pRC->GetClass();

    OBJECTREF o;
    HELPER_METHOD_FRAME_BEGIN_RET_0();
    pEEC = pEEC->GetEnclosingClass();
    o = pEEC->GetExposedClassObject();
    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(o);
}
FCIMPLEND

void COMClass::CreateClassObjFromEEClass(EEClass* pVMCClass, REFLECTCLASSBASEREF* pRefClass)
{
    // This only throws the possible exception raised by the <cinit> on the class
    THROWSCOMPLUSEXCEPTION();

    // call the <cinit> Class
    OBJECTREF Throwable;
    if (!g_pRefUtil->GetClass(RC_Class)->CheckRunClassInit(&Throwable)) {
        COMPlusThrow(Throwable);
    }

    // There was an expectation that we would never come here for e.g. Arrays.  But there
    // are far too many clients who were unaware of that expectation.  The most expedient
    // thing to do for V1 is to simply handle that case here:
    if (pVMCClass->IsArrayClass())
    {
        ArrayClass      *pArrayClass = (ArrayClass *) pVMCClass;
        TypeHandle       th = pArrayClass->GetClassLoader()->FindArrayForElem(
                                pArrayClass->GetElementTypeHandle(),
                                pArrayClass->GetMethodTable()->GetNormCorElementType(),
                                pArrayClass->GetRank());

        *pRefClass = (REFLECTCLASSBASEREF) th.CreateClassObj();

        _ASSERTE(*pRefClass != NULL);
    }
    else
    {
    // Check to make sure this has a member.  If not it must be
    //  special
    _ASSERTE(pVMCClass->GetCl() != mdTypeDefNil);

    // Create a COM+ Class object
    *pRefClass = (REFLECTCLASSBASEREF) AllocateObject(g_pRefUtil->GetClass(RC_Class));

    // Set the data in the COM+ object
    ReflectClass* p = new (pVMCClass->GetDomain()) ReflectBaseClass();
    if (!p)
        COMPlusThrowOM();
    REFLECTCLASSBASEREF tmp = *pRefClass;
    GCPROTECT_BEGIN(tmp);
    p->Init(pVMCClass);
    *pRefClass = tmp;
    GCPROTECT_END();
    (*pRefClass)->SetData(p);
}
}

// GetMemberMethods
// This method will return all of the members methods which match the specified attributes flag
FCIMPL7(Object*, COMClass::GetMemberMethods, ReflectClassBaseObject* refThisUNSAFE, StringObject* nameUNSAFE, INT32 invokeAttr, INT32 callConv, PTRArray* argTypesUNSAFE, INT32 argCnt, BYTE verifyAccess)
{
    THROWSCOMPLUSEXCEPTION();

    PTRARRAYREF refRetVal;

    struct _gc
    {
        REFLECTCLASSBASEREF refThis; 
        STRINGREF           name;    
        PTRARRAYREF         argTypes;
    } gc;

    gc.refThis  = (REFLECTCLASSBASEREF) refThisUNSAFE;
    gc.name     = (STRINGREF)           nameUNSAFE;
    gc.argTypes = (PTRARRAYREF)         argTypesUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_NOPOLL(Frame::FRAME_ATTR_RETURNOBJ);
    GCPROTECT_BEGIN(gc);
    HELPER_METHOD_POLL();

    if (gc.name == NULL)
        COMPlusThrow(kNullReferenceException);


    bool    checkCall;
    
    ReflectClass* pRC = (ReflectClass*) gc.refThis->GetData();
    _ASSERTE(pRC);

    // Check the calling convention.
    checkCall = (callConv == Any_CC) ? false : true;

    CQuickBytes bytes;
    LPSTR szName;
    DWORD cName;

    szName = GetClassStringVars((STRINGREF) gc.name, &bytes, &cName);

    ReflectMethodList* pMeths = pRC->GetMethods();

    // Find methods....
    refRetVal = (PTRARRAYREF) (PTRArray*) COMMember::g_pInvokeUtil->FindMatchingMethods(invokeAttr,
                                                         szName,
                                                         cName,
                                                         (gc.argTypes != NULL) ? &gc.argTypes : NULL,
                                                         argCnt,
                                                         checkCall,
                                                         callConv,
                                                         pRC,
                                                         pMeths, 
                                                         g_pRefUtil->GetTrueType(RC_Method),
                                                         verifyAccess != 0);

    GCPROTECT_END();
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refRetVal);
}
FCIMPLEND

// GetMemberCons
// This method returns all of the constructors that have a set number of methods.
FCIMPL7(LPVOID, COMClass::GetMemberCons, 
                        ReflectClassBaseObject* refThisUNSAFE, 
                        INT32       invokeAttr, 
                        INT32       callConv, 
                        PTRArray*   argTypesUNSAFE, 
                        INT32       argCnt, 
                        BYTE        verifyAccess, 
                        BYTE*       isDelegate)
{
    LPVOID  rv = NULL;
    bool    checkCall;
    
    REFLECTCLASSBASEREF refThis     = (REFLECTCLASSBASEREF) refThisUNSAFE;
    PTRARRAYREF         argTypes    = (PTRARRAYREF)         argTypesUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_2(Frame::FRAME_ATTR_RETURNOBJ, refThis, argTypes);
    
    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    _ASSERTE(pRC);

    // properly get rid of any non sense from the binding flags
    invokeAttr &= ~BINDER_FlattenHierarchy;
    invokeAttr &= ~BINDER_IgnoreCase;
    invokeAttr |= BINDER_DeclaredOnly;

    // Check the calling convention.
    checkCall = (callConv == Any_CC) ? false : true;

    ReflectMethodList* pCons = pRC->GetConstructors();

    // Find methods....
    rv = COMMember::g_pInvokeUtil->FindMatchingMethods(invokeAttr,
                                                       NULL,
                                                       0,
                                                       (argTypes != NULL) ? &argTypes : NULL,
                                                       argCnt,
                                                       checkCall,
                                                       callConv,
                                                       pRC,
                                                       pCons, 
                                                       g_pRefUtil->GetTrueType(RC_Ctor),
                                                       verifyAccess != 0);
    
    // Also return whether the type is a delegate (some extra security checks
    // need to be made in this case).
    *isDelegate = (pRC->IsClass()) ? !!pRC->GetClass()->IsAnyDelegateClass() : 0;

    HELPER_METHOD_FRAME_END();

    return rv;
}
FCIMPLEND

// GetMemberField
// This method returns all of the fields which match the specified
//  name.
FCIMPL4(Object*, COMClass::GetMemberField, ReflectClassBaseObject* refThisUNSAFE, StringObject* nameUNSAFE, INT32 invokeAttr, BYTE verifyAccess)
{
    PTRARRAYREF         refArr  = NULL;
    REFLECTCLASSBASEREF refThis = (REFLECTCLASSBASEREF) refThisUNSAFE;
    STRINGREF           name    = (STRINGREF)           nameUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_2(Frame::FRAME_ATTR_RETURNOBJ, refThis, name);

    DWORD           i;
    RefSecContext   sCtx;

    THROWSCOMPLUSEXCEPTION();

    if (name == NULL)
        COMPlusThrow(kNullReferenceException);



    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    _ASSERTE(pRC);

    ReflectFieldList* pFields = pRC->GetFields();

    CQuickBytes bytes;
    LPSTR szFieldName;
    DWORD cFieldName;

    szFieldName = GetClassStringVars(name, &bytes, &cFieldName);

    int fldCnt = 0;
    int* matchFlds = (int*) _alloca(sizeof(int) * pFields->dwTotal);
    memset(matchFlds,0,sizeof(int) * pFields->dwTotal);

    MethodTable *pParentMT = pRC->GetClass()->GetMethodTable();

    DWORD propToLookup = (invokeAttr & BINDER_FlattenHierarchy) ? pFields->dwTotal : pFields->dwFields;
    for(i=0; i<propToLookup; i++) {
        // Get the FieldDesc
        if (MatchField(pFields->fields[i].pField, cFieldName, szFieldName, pRC, invokeAttr) &&
            (!verifyAccess || InvokeUtil::CheckAccess(&sCtx, pFields->fields[i].pField->GetFieldProtection(), pParentMT, 0)))
                matchFlds[fldCnt++] = i;
    }

    // If we didn't find any methods then return
    if (fldCnt == 0)
        goto lExit;
    // Allocate the MethodInfo Array and return it....
    refArr = (PTRARRAYREF) AllocateObjectArray(fldCnt, g_pRefUtil->GetTrueType(RC_Field));
    GCPROTECT_BEGIN(refArr);
    for (int i=0;i<fldCnt;i++) {
        // Do not change this code.  This is done this way to
        //  prevent a GC hole in the SetObjectReference() call.  The compiler
        //  is free to pick the order of evaluation.
        OBJECTREF o = (OBJECTREF) pFields->fields[matchFlds[i]].GetFieldInfo(pRC);
        refArr->SetAt(i, o);
    }
    GCPROTECT_END();

lExit: ;
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refArr);
}
FCIMPLEND


// GetMemberProperties
// This method returns all of the properties that have a set number
//  of arguments.  The methods will be either get or set methods depending
//  upon the invokeAttr flag.
FCIMPL5(Object*, COMClass::GetMemberProperties, ReflectClassBaseObject* refThisUNSAFE, StringObject* nameUNSAFE, INT32 invokeAttr, INT32 argCnt, BYTE verifyAccess)
{
    PTRARRAYREF         refArr  = NULL;
    REFLECTCLASSBASEREF refThis = (REFLECTCLASSBASEREF) refThisUNSAFE;
    STRINGREF           name    = (STRINGREF)           nameUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_2(Frame::FRAME_ATTR_RETURNOBJ, refThis, name);

    THROWSCOMPLUSEXCEPTION();

    if (name == NULL)
        COMPlusThrow(kNullReferenceException);

    bool            loose;
    RefSecContext   sCtx;

    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    _ASSERTE(pRC);

    EEClass* pEEC = pRC->GetClass();

    loose = (invokeAttr & BINDER_OptionalParamBinding) ? true : false;

    // The Search modifiers
    bool ignoreCase = ((invokeAttr & BINDER_IgnoreCase)  != 0);
    bool declaredOnly = ((invokeAttr & BINDER_DeclaredOnly)  != 0);

    // The search filters
    bool addStatic = ((invokeAttr & BINDER_Static)  != 0);
    bool addInst = ((invokeAttr & BINDER_Instance)  != 0);
    bool addPriv = ((invokeAttr & BINDER_NonPublic) != 0);
    bool addPub = ((invokeAttr & BINDER_Public) != 0);

    int bSetter = (invokeAttr & BINDER_SetProperty) ? 1 : 0;

    LPSTR szName = NULL;
	DWORD searchSpace = 0;
	MethodTable *pParentMT = NULL;
    int propCnt = 0;
	int *matchProps = NULL;

	// Get the Properties from the Class
    ReflectPropertyList* pProps = pRC->GetProperties();
    if (pProps->dwTotal == 0)
    {
        goto lExit;
    }

    {
    CQuickBytes bytes;
    DWORD cName;
        szName = GetClassStringVars(name, &bytes, &cName);
    }

    searchSpace = ((invokeAttr & BINDER_FlattenHierarchy) != 0) ? pProps->dwTotal : pProps->dwProps;

    pParentMT = pEEC->GetMethodTable();

    matchProps = (int*) _alloca(sizeof(int) * searchSpace);
    memset(matchProps,0,sizeof(int) * searchSpace);
    for (DWORD i = 0; i < searchSpace; i++) {

        // Check on the name
        if (ignoreCase) {
            if (_stricmp(pProps->props[i].szName, szName) != 0)
                continue;
        }
        else {
            if (strcmp(pProps->props[i].szName, szName) != 0)
                continue;
        }

        // Test the publics/nonpublics
        if (COMMember::PublicProperty(&pProps->props[i])) {
            if (!addPub) continue;
        }
        else {
            if (!addPriv) continue;
            if (verifyAccess && !InvokeUtil::CheckAccess(&sCtx, mdAssem, pParentMT, 0)) continue;
        }

        // Check for static instance 
        if (COMMember::StaticProperty(&pProps->props[i])) {
            if (!addStatic) continue;
        }
        else {
            if (!addInst) continue;
        }

        // Checked the declared methods.
        if (declaredOnly) {
            if (pProps->props[i].pDeclCls != pEEC)
                 continue;
        }

        // Check the specific accessor
        ReflectMethod* pMeth;
        if (bSetter) {
            pMeth = pProps->props[i].pSetter;           
        }
        else {
            pMeth = pProps->props[i].pGetter;
            
        }
        if (pMeth == 0)
            continue;

        ExpandSig* pSig = pMeth->GetSig();
        int nArgs = pSig->NumFixedArgs();

        if (nArgs != argCnt) {
            
            IMDInternalImport *pInternalImport = pMeth->pMethod->GetMDImport();
            HENUMInternal   hEnumParam;
            mdParamDef      paramDef = mdParamDefNil;
            mdToken methodTk = pMeth->GetToken();
            if (!IsNilToken(methodTk)) {
            
                HRESULT hr = pInternalImport->EnumInit(mdtParamDef, methodTk, &hEnumParam);
                if (SUCCEEDED(hr)) {
                    if (nArgs < argCnt || nArgs == argCnt + 1) {
                        // we must have a param array under the first condition, could be a param array under the second

                        int propArgCount = nArgs - bSetter;
                        // get the sig of the last param
                        LPVOID pEnum;
                        pSig->Reset(&pEnum);
                        TypeHandle lastArgType;
                        for (INT32 i = 0; i < propArgCount; i++) 
                            lastArgType = pSig->NextArgExpanded(&pEnum);

                        pInternalImport->EnumReset(&hEnumParam);

                        // get metadata info and token for the last param
                        ULONG paramCount = pInternalImport->EnumGetCount(&hEnumParam);
                        for (ULONG ul = 0; ul < paramCount; ul++) {
                            pInternalImport->EnumNext(&hEnumParam, &paramDef);
                            if (paramDef != mdParamDefNil) {
                                LPCSTR  name;
                                SHORT   seq;
                                DWORD   revWord;
                                name = pInternalImport->GetParamDefProps(paramDef,(USHORT*) &seq, &revWord);
                                if (seq == propArgCount) {
                                    // looks good! check that it is in fact a param array
                                    if (lastArgType.IsArray()) {
                                        if (COMCustomAttribute::IsDefined(pMeth->GetModule(), paramDef, TypeHandle(InvokeUtil::GetParamArrayAttributeTypeHandle()))) {
                                            pInternalImport->EnumClose(&hEnumParam);
                                            goto matchFound;
                                        }
                                    }
                                }
                            }
                        }
                    }

                    if (loose && nArgs > argCnt) {
                        pInternalImport->EnumReset(&hEnumParam);
                        ULONG cArg = (ULONG)(argCnt + 1 - bSetter);
                        while (pInternalImport->EnumNext(&hEnumParam, &paramDef)) {
                            LPCSTR  name;
                            SHORT   seq;
                            DWORD   revWord;
                            name = pInternalImport->GetParamDefProps(paramDef,(USHORT*) &seq, &revWord);
                            if ((ULONG)seq < cArg) 
                                continue;
                            else if ((ULONG)seq == cArg && (revWord & pdOptional)) {
                                cArg++;
                continue;
        }
                            else {
                                if (!bSetter || (int)seq != nArgs) 
                                    break; // not an optional param, no match
                            }
                        }
                        if (cArg == (ULONG)nArgs + 1 - bSetter) {
                            pInternalImport->EnumClose(&hEnumParam);
                            goto matchFound;
                        }
                    }
                    
                    pInternalImport->EnumClose(&hEnumParam);
                }
            }
            
            continue; // no good
        }
    matchFound:

        if (verifyAccess && !InvokeUtil::CheckAccess(&sCtx, pMeth->attrs, pParentMT, 0)) continue;

        // If the method has a linktime security demand attached, check it now.
        if (verifyAccess && !InvokeUtil::CheckLinktimeDemand(&sCtx, pMeth->pMethod, false))
            continue;

        matchProps[propCnt++] = i;
    }
    // If we didn't find any methods then return
    if (propCnt == 0)
    {
        goto lExit;
    }

    // Allocate the MethodInfo Array and return it....
    refArr = (PTRARRAYREF) AllocateObjectArray( propCnt, 
        g_pRefUtil->GetTrueType(RC_Method));
    GCPROTECT_BEGIN(refArr);
    for (int i=0;i<propCnt;i++) {
        ReflectMethod* pMeth;
        if (invokeAttr & BINDER_SetProperty)
            pMeth = pProps->props[matchProps[i]].pSetter;           
        else 
            pMeth = pProps->props[matchProps[i]].pGetter;

        // Do not change this code.  This is done this way to
        //  prevent a GC hole in the SetObjectReference() call.  The compiler
        //  is free to pick the order of evaluation.
        OBJECTREF o = (OBJECTREF) pMeth->GetMethodInfo(pProps->props[matchProps[i]].pRC);
        refArr->SetAt(i, o);
    }
    GCPROTECT_END();

lExit: ;
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refArr);
}
FCIMPLEND

// GetMatchingProperties
// This basically does a matching based upon the properties abstract 
//  signature.

FCIMPL6(Object*, COMClass::GetMatchingProperties, ReflectClassBaseObject* refThisUNSAFE, 
                                                  StringObject* nameUNSAFE,
                                                  INT32 invokeAttr,
                                                  INT32 argCnt,
                                                  ReflectClassBaseObject* returnTypeUNSAFE,
                                                  BYTE verifyAccess)
{
    THROWSCOMPLUSEXCEPTION();

    ReflectClass* pRC = (ReflectClass*) refThisUNSAFE->GetData();
    _ASSERTE(pRC);

    STRINGREF           name        = (STRINGREF)           nameUNSAFE;
//    REFLECTCLASSBASEREF returnType  = (REFLECTCLASSBASEREF) returnTypeUNSAFE;
    PTRARRAYREF         refArr      = NULL;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, name);

    RefSecContext   sCtx;
    CQuickBytes bytes;


    EEClass* pEEC = pRC->GetClass();

    // The Search modifiers
    bool ignoreCase = ((invokeAttr & BINDER_IgnoreCase)  != 0);
    bool declaredOnly = ((invokeAttr & BINDER_DeclaredOnly)  != 0);

    // The search filters
    bool addStatic = ((invokeAttr & BINDER_Static)  != 0);
    bool addInst = ((invokeAttr & BINDER_Instance)  != 0);
    bool addPriv = ((invokeAttr & BINDER_NonPublic) != 0);
    bool addPub = ((invokeAttr & BINDER_Public) != 0);

    DWORD searchSpace = 0;
	MethodTable *pParentMT = NULL;
	int propCnt = 0;
	int *matchProps = NULL;

    // Get the Properties from the Class
    ReflectPropertyList* pProps = pRC->GetProperties();
    if (pProps->dwTotal == 0)
    {
        goto lExit;
    }

    LPSTR szName;
    DWORD cName;

    if (name == NULL)
        COMPlusThrow(kNullReferenceException);



    szName = GetClassStringVars(name, &bytes, &cName);

    searchSpace = ((invokeAttr & BINDER_FlattenHierarchy) != 0) ? pProps->dwTotal : pProps->dwProps;

    pParentMT = pEEC->GetMethodTable();

    matchProps = (int*) _alloca(sizeof(int) * searchSpace);
    memset(matchProps,0,sizeof(int) * searchSpace);
    for (DWORD i = 0; i < searchSpace; i++) {

        // Check on the name
        if (ignoreCase) {
            if (_stricmp(pProps->props[i].szName, szName) != 0)
                continue;
        }
        else {
            if (strcmp(pProps->props[i].szName, szName) != 0)
                continue;
        }

        int argCnt = pProps->props[i].pSignature->NumFixedArgs();
        if (argCnt != -1 && argCnt != argCnt)
            continue;

        // Test the publics/nonpublics
        if (COMMember::PublicProperty(&pProps->props[i])) {
            if (!addPub) continue;
        }
        else {
            if (!addPriv) continue;
            if (verifyAccess && !InvokeUtil::CheckAccess(&sCtx, mdAssem, pParentMT, 0)) continue;
        }

        // Check for static instance 
        if (COMMember::StaticProperty(&pProps->props[i])) {
            if (!addStatic) continue;
        }
        else {
            if (!addInst) continue;
        }

        // Checked the declared methods.
        if (declaredOnly) {
            if (pProps->props[i].pDeclCls != pEEC)
                 continue;
        }

        matchProps[propCnt++] = i;
    }
    // If we didn't find any methods then return
    if (propCnt == 0)
    {
        goto lExit;
    }

    // Allocate the MethodInfo Array and return it....
    refArr = (PTRARRAYREF) AllocateObjectArray(propCnt, g_pRefUtil->GetTrueType(RC_Prop));
    GCPROTECT_BEGIN(refArr);
    for (int i=0;i<propCnt;i++) {
        // Do not change this code.  This is done this way to
        //  prevent a GC hole in the SetObjectReference() call.  The compiler
        //  is free to pick the order of evaluation.
        OBJECTREF o = (OBJECTREF) pProps->props[matchProps[i]].GetPropertyInfo(pProps->props[matchProps[i]].pRC);
        refArr->SetAt(i, o);
    }
    GCPROTECT_END();

lExit: ;
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refArr);
}
FCIMPLEND


// GetMethod
// This method returns an array of MethodInfo object representing all of the methods
//  defined for this class.
FCIMPL2(Object*, COMClass::GetMethods, ReflectClassBaseObject* refThisUNSAFE, DWORD bindingAttr)
{
    PTRARRAYREF         refArrMethods   = NULL;
    REFLECTCLASSBASEREF refThis         = (REFLECTCLASSBASEREF) refThisUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refThis);

    THROWSCOMPLUSEXCEPTION();

    // Get the EEClass and Vtable associated with refThis
    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    _ASSERTE(pRC);

    ReflectMethodList* pMeths = pRC->GetMethods();
    refArrMethods = g_pRefUtil->CreateClassArray(RC_Method,pRC,pMeths,bindingAttr, true);

    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refArrMethods);
}
FCIMPLEND

// GetConstructor
// This method returns a single constructor which matchs the passed
//  in criteria.
FCIMPL3(Object*, COMClass::GetConstructors, ReflectClassBaseObject* refThisUNSAFE, DWORD bindingAttr, BYTE verifyAccess)
{
    PTRARRAYREF         refArrCtors = NULL;
    REFLECTCLASSBASEREF refThis     = (REFLECTCLASSBASEREF) refThisUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refThis);

    THROWSCOMPLUSEXCEPTION();

    // Get the EEClass and Vtable associated with refThis
    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    _ASSERTE(pRC);
    ReflectMethodList* pCons = pRC->GetConstructors();
    refArrCtors = g_pRefUtil->CreateClassArray(RC_Ctor,pRC,pCons,bindingAttr, verifyAccess != 0);

    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refArrCtors);
}
FCIMPLEND



// GetField
// This method will return the specified field
FCIMPL3(Object*, COMClass::GetField, ReflectClassBaseObject* refThisUNSAFE, StringObject* fieldNameUNSAFE, DWORD fBindAttr)
{
    THROWSCOMPLUSEXCEPTION();

    REFLECTBASEREF refField = NULL;

    REFLECTCLASSBASEREF refThis = (REFLECTCLASSBASEREF) refThisUNSAFE;
    STRINGREF fieldName = (STRINGREF) fieldNameUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_2(Frame::FRAME_ATTR_RETURNOBJ, refThis, fieldName);

    DWORD          i;
    RefSecContext  sCtx;

    if (fieldName == 0)
        COMPlusThrowArgumentNull(L"name",L"ArgumentNull_String");

    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    _ASSERTE(pRC);

    ReflectFieldList* pFields = pRC->GetFields();
    DWORD maxCnt;

    if (fBindAttr & BINDER_FlattenHierarchy)
        maxCnt = pFields->dwTotal;
    else
        maxCnt = pFields->dwFields;

    CQuickBytes bytes;
    LPSTR szFieldName;
    DWORD cFieldName;

    szFieldName = GetClassStringVars(fieldName, &bytes, &cFieldName);

    GCPROTECT_BEGIN(refField);

    for (i=0; i < maxCnt; i++) 
    {
        if (MatchField(pFields->fields[i].pField,cFieldName,szFieldName, pRC, fBindAttr) &&
            InvokeUtil::CheckAccess(&sCtx, pFields->fields[i].pField->GetFieldProtection(), pRC->GetClass()->GetMethodTable(), 0)) 
        {
            // Found the first field that matches, so return it
            refField = pFields->fields[i].GetFieldInfo(pRC);
            break;
        }
    }

    GCPROTECT_END();
    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(refField);
}
FCIMPLEND

LPVOID __stdcall COMClass::MatchField(FieldDesc* pCurField,DWORD cFieldName,
    LPUTF8 szFieldName,ReflectClass* pRC,int bindingAttr)
{
    _ASSERTE(pCurField);

    // Public/Private members
    bool addPub = ((bindingAttr & BINDER_Public) != 0);
    bool addPriv = ((bindingAttr & BINDER_NonPublic) != 0);
    if (pCurField->IsPublic()) {
        if (!addPub) return 0;
    }
    else {
        if (!addPriv) return 0;
    }

    // Check for static instance 
    bool addStatic = ((bindingAttr & BINDER_Static)  != 0);
    bool addInst = ((bindingAttr & BINDER_Instance)  != 0);
    if (pCurField->IsStatic()) {
        if (!addStatic) return 0;
    }
    else {
        if (!addInst) return 0;
    }

    // Get the name of the field
    LPCUTF8 pwzCurFieldName = pCurField->GetName();

    // If the names do not match, reject field
    if(strlen(pwzCurFieldName) != cFieldName)
        return 0;

    // Case sensitive compare
    bool ignoreCase = ((bindingAttr & BINDER_IgnoreCase)  != 0);
    if (ignoreCase) {
        if (_stricmp(pwzCurFieldName, szFieldName) != 0)
            return 0;
    }
    else {
        if (memcmp(pwzCurFieldName, szFieldName, cFieldName))
            return 0;
    }

    bool declaredOnly = ((bindingAttr & BINDER_DeclaredOnly)  != 0);
    if (declaredOnly) {
        EEClass* pEEC = pRC->GetClass();
        if (pCurField->GetEnclosingClass() != pEEC)
             return 0;
    }

     return pCurField;
}

// GetFields
// This method will return a FieldInfo array of all of the
//  fields defined for this Class
FCIMPL3(Object*, COMClass::GetFields, ReflectClassBaseObject* refThisUNSAFE, DWORD bindingAttr, BYTE bRequiresAccessCheck)
{
    PTRARRAYREF rv = NULL;

    THROWSCOMPLUSEXCEPTION();

    // Get the class for this object
    ReflectClass* pRC = (ReflectClass*) refThisUNSAFE->GetData();
    _ASSERTE(pRC);

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, rv);

    ReflectFieldList* pFields = pRC->GetFields();
    PTRARRAYREF         refArrFields    = g_pRefUtil->CreateClassArray(RC_Field,pRC,pFields,bindingAttr, bRequiresAccessCheck != 0);
    
    rv = refArrFields;

    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(rv);
}
FCIMPLEND


// GetEvent
// This method will return the specified event based upon
//  the name
FCIMPL3(Object*, COMClass::GetEvent, ReflectClassBaseObject* refThisUNSAFE, StringObject* eventNameUNSAFE, DWORD bindingAttr)
{
    REFLECTTOKENBASEREF refMethod   = NULL;
    REFLECTCLASSBASEREF refThis     = (REFLECTCLASSBASEREF) refThisUNSAFE;
    STRINGREF           eventName   = (STRINGREF)           eventNameUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_2(Frame::FRAME_ATTR_RETURNOBJ, refThis, eventName);


    THROWSCOMPLUSEXCEPTION();
    if (eventName == NULL)
        COMPlusThrowArgumentNull(L"name",L"ArgumentNull_String");

    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    _ASSERTE(pRC);

    EEClass* pEEC = pRC->GetClass();

    bool ignoreCase = false;
	bool declaredOnly = false;
	bool addStatic = false;
	bool addInst = false;
	bool addPriv = false;
	bool addPub = false;

	MethodTable *pParentMT = NULL;
	ReflectEvent *ev = NULL;
	DWORD searchSpace = 0;

    // Get the events from the Class
    ReflectEventList* pEvents = pRC->GetEvents();
    if (pEvents->dwTotal == 0)
        goto lExit;

    LPSTR szName;
    {
        CQuickBytes bytes;
    DWORD cName;
        szName = GetClassStringVars(eventName, &bytes, &cName);
    }

    // The Search modifiers
    ignoreCase = ((bindingAttr & BINDER_IgnoreCase)  != 0);
    declaredOnly = ((bindingAttr & BINDER_DeclaredOnly)  != 0);

    // The search filters
    addStatic = ((bindingAttr & BINDER_Static)  != 0);
    addInst = ((bindingAttr & BINDER_Instance)  != 0);
    addPriv = ((bindingAttr & BINDER_NonPublic) != 0);
    addPub = ((bindingAttr & BINDER_Public) != 0);

    pParentMT = pEEC->GetMethodTable();

    // check the events to see if we find one that matches...
    ev = 0;
    searchSpace = ((bindingAttr & BINDER_FlattenHierarchy) != 0) ? pEvents->dwTotal : pEvents->dwEvents;
    for (DWORD i = 0; i < searchSpace; i++) {
        // Check for access to publics, non-publics
        if (COMMember::PublicEvent(&pEvents->events[i])) {
            if (!addPub) continue;
        }
        else {
            RefSecContext   sCtx;

            if (!addPriv) continue;
            if (!InvokeUtil::CheckAccess(&sCtx, mdAssem, pParentMT, 0)) continue;
        }

        if (declaredOnly) {
            if (pEvents->events[i].pDeclCls != pEEC)
                 continue;
        }

        // Check fo static instance 
        if (COMMember::StaticEvent(&pEvents->events[i])) {
            if (!addStatic) continue;
        }
        else {
            if (!addInst) continue;
        }

        // Check on the name
        if (ignoreCase) {
            if (_stricmp(pEvents->events[i].szName, szName) != 0)
                continue;
        }
        else {
            if (strcmp(pEvents->events[i].szName, szName) != 0)
                continue;
        }

        // Ignore case can cause Ambiguous situations, we need to check for
        //  these.
        if (ev)
            COMPlusThrow(kAmbiguousMatchException);
        ev = &pEvents->events[i];
        if (!ignoreCase)
            break;

    }

    if (ev)
    {
    // Found the first method that matches, so return it
        refMethod = (REFLECTTOKENBASEREF) ev->GetEventInfo(pRC);
    }

lExit: ;
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refMethod);
}
FCIMPLEND

// GetEvents
// This method will return an array of EventInfo for each of the events
//  defined in the class
FCIMPL2(Object*, COMClass::GetEvents, ReflectClassBaseObject* refThisUNSAFE, DWORD bindingAttr)
{
    PTRARRAYREF         pRet    = NULL;
    REFLECTCLASSBASEREF refThis = (REFLECTCLASSBASEREF) refThisUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refThis);

    REFLECTTOKENBASEREF     refMethod;
    RefSecContext   sCtx;

    THROWSCOMPLUSEXCEPTION();

    // Find the properties
    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    _ASSERTE(pRC);

    EEClass* pEEC = pRC->GetClass();

    bool ignoreCase = false;
	bool declaredOnly = false;
	bool addStatic = false;
	bool addInst = false;
	bool addPriv = false;
	bool addPub = false;

	DWORD searchSpace = 0;
	MethodTable *pParentMT = NULL;

	// Get the events from the class
    ReflectEventList* pEvents = pRC->GetEvents();
    if (pEvents->dwTotal == 0)
    {
        pRet = (PTRARRAYREF) AllocateObjectArray(0,g_pRefUtil->GetTrueType(RC_Event));
        goto lExit;
    }

    // The Search modifiers
    ignoreCase = ((bindingAttr & BINDER_IgnoreCase)  != 0);
    declaredOnly = ((bindingAttr & BINDER_DeclaredOnly)  != 0);

    // The search filters
    addStatic = ((bindingAttr & BINDER_Static)  != 0);
    addInst = ((bindingAttr & BINDER_Instance)  != 0);
    addPriv = ((bindingAttr & BINDER_NonPublic) != 0);
    addPub = ((bindingAttr & BINDER_Public) != 0);

    searchSpace = ((bindingAttr & BINDER_FlattenHierarchy) != 0) ? pEvents->dwTotal : pEvents->dwEvents;

    pRet = (PTRARRAYREF) AllocateObjectArray(searchSpace, g_pRefUtil->GetTrueType(RC_Event));
    GCPROTECT_BEGIN(pRet);

    pParentMT = pEEC->GetMethodTable();

    // Loop through all of the Events and see how many match
    //  the binding flags.
	ULONG i;
	ULONG pos;
    for (i = 0, pos = 0; i < searchSpace; i++) {
        // Check for access to publics, non-publics
        if (COMMember::PublicEvent(&pEvents->events[i])) {
            if (!addPub) continue;
        }
        else {
            if (!addPriv) continue;
            if (!InvokeUtil::CheckAccess(&sCtx, mdAssem, pParentMT, 0)) continue;
        }

        if (declaredOnly) {
            if (pEvents->events[i].pDeclCls != pEEC)
                 continue;
        }

        // Check fo static instance 
        if (COMMember::StaticEvent(&pEvents->events[i])) {
            if (!addStatic) continue;
        }
        else {
            if (!addInst) continue;
        }

        refMethod = (REFLECTTOKENBASEREF) pEvents->events[i].GetEventInfo(pRC);
        pRet->SetAt(pos++, (OBJECTREF) refMethod);
    }

    // Copy to a new array if we didn't fill up the first array
    if (i != pos) {
        PTRARRAYREF retArray = (PTRARRAYREF) AllocateObjectArray(pos, 
            g_pRefUtil->GetTrueType(RC_Event));
        for(i = 0; i < pos; i++)
            retArray->SetAt(i, pRet->GetAt(i));
        pRet = retArray;
    }

    GCPROTECT_END();

lExit: ;
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(pRet);
}
FCIMPLEND

// GetProperties
// This method will return an array of Properties for each of the
//  properties defined in this class.  An empty array is return if
//  no properties exist.
FCIMPL2(Object*, COMClass::GetProperties, ReflectClassBaseObject* refThisUNSAFE, DWORD bindingAttr)
{
    PTRARRAYREF         pRet    = NULL;
    REFLECTCLASSBASEREF refThis = (REFLECTCLASSBASEREF) refThisUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refThis);

    RefSecContext   sCtx;


    THROWSCOMPLUSEXCEPTION();

    // Find the properties
    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    _ASSERTE(pRC);

    EEClass* pEEC = pRC->GetClass();

    bool ignoreCase = false;
	bool declaredOnly = false;
	bool addStatic = false;
	bool addInst = false;
	bool addPriv = false;
	bool addPub = false;

	DWORD searchSpace = 0;
	MethodTable *pParentMT = NULL;

	// Get the Properties from the Class
    ReflectPropertyList* pProps = pRC->GetProperties();
    if (pProps->dwTotal == 0) 
    {
        pRet = (PTRARRAYREF) AllocateObjectArray(0, g_pRefUtil->GetTrueType(RC_Prop));
        goto lExit;
    }

    // The Search modifiers
    ignoreCase = ((bindingAttr & BINDER_IgnoreCase)  != 0);
    declaredOnly = ((bindingAttr & BINDER_DeclaredOnly)  != 0);

    // The search filters
    addStatic = ((bindingAttr & BINDER_Static)  != 0);
    addInst = ((bindingAttr & BINDER_Instance)  != 0);
    addPriv = ((bindingAttr & BINDER_NonPublic) != 0);
    addPub = ((bindingAttr & BINDER_Public) != 0);

    searchSpace = ((bindingAttr & BINDER_FlattenHierarchy) != 0) ? pProps->dwTotal : pProps->dwProps;

    pRet = (PTRARRAYREF) AllocateObjectArray(searchSpace, g_pRefUtil->GetTrueType(RC_Prop));
    GCPROTECT_BEGIN(pRet);

    pParentMT = pEEC->GetMethodTable();

    ULONG i;
	ULONG pos;
	for (i = 0, pos = 0; i < searchSpace; i++) {
        // Check for access to publics, non-publics
        if (COMMember::PublicProperty(&pProps->props[i])) {
            if (!addPub) continue;
        }
        else {
            if (!addPriv) continue;
            if (!InvokeUtil::CheckAccess(&sCtx, mdAssem, pParentMT, 0)) continue;
        }

        if (declaredOnly) {
            if (pProps->props[i].pDeclCls != pEEC)
                 continue;
        }
        // Check for static instance 
        if (COMMember::StaticProperty(&pProps->props[i])) {
            if (!addStatic) continue;
        }
        else {
            if (!addInst) continue;
        }

        OBJECTREF o = (OBJECTREF) pProps->props[i].GetPropertyInfo(pRC);
        pRet->SetAt(pos++, o);
    }

    // Copy to a new array if we didn't fill up the first array
    if (i != pos) {
        PTRARRAYREF retArray = (PTRARRAYREF) AllocateObjectArray(pos, 
            g_pRefUtil->GetTrueType(RC_Prop));
        for(i = 0; i < pos; i++)
            retArray->SetAt(i, pRet->GetAt(i));
        pRet = retArray;
    }

    GCPROTECT_END();

lExit: ;
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(pRet);
}
FCIMPLEND

void COMClass::GetNameInternal(ReflectClass *pRC, int nameType, CQuickBytes *qb)
{
    LPCUTF8           szToName;
    bool              fNameSpace = (nameType & TYPE_NAMESPACE) ? true : false;
    bool              fAssembly = (nameType & TYPE_ASSEMBLY) ? true : false;
    mdTypeDef         mdEncl;
    IMDInternalImport *pImport;
    bool              fSetName = false;

    THROWSCOMPLUSEXCEPTION();

    szToName = _GetName(pRC, fNameSpace && !pRC->IsNested(), qb);

    pImport = pRC->GetModule()->GetMDImport();

    // Get original element for parameterized type
    EEClass *pTypeClass = pRC->GetTypeHandle().GetClassOrTypeParam();
    _ASSERTE(pTypeClass);
    mdEncl = pTypeClass->GetCl();

    // Only look for nesting chain if this is a nested type.
    DWORD dwAttr;
    pTypeClass->GetMDImport()->GetTypeDefProps(mdEncl, &dwAttr, NULL);
    if (fNameSpace && (IsTdNested(dwAttr)))
    {   // Build the nesting chain.
        while (SUCCEEDED(pImport->GetNestedClassProps(mdEncl, &mdEncl))) {
            CQuickBytes qb2;
            CQuickBytes qb3;
            LPCUTF8 szEnclName;
            LPCUTF8 szEnclNameSpace;
            pImport->GetNameOfTypeDef(mdEncl,
                                      &szEnclName,
                                      &szEnclNameSpace);

            ns::MakePath(qb2, szEnclNameSpace, szEnclName);
            ns::MakeNestedTypeName(qb3, (LPCUTF8) qb2.Ptr(), szToName);
            
            int iLen = (int)strlen((LPCUTF8) qb3.Ptr()) + 1;
            if (qb->Alloc(iLen) == NULL)
                COMPlusThrowOM();
            strncpy((LPUTF8) qb->Ptr(), (LPCUTF8) qb3.Ptr(), iLen);
            szToName = (LPCUTF8) qb->Ptr();
            fSetName = true;
        }
    }

    if(fAssembly) {
        CQuickBytes qb2;
        Assembly* pAssembly = pRC->GetTypeHandle().GetAssembly();
        LPCWSTR pAssemblyName;
        if(SUCCEEDED(pAssembly->GetFullName(&pAssemblyName))) {
            MAKE_WIDEPTR_FROMUTF8(wName, szToName);
            ns::MakeAssemblyQualifiedName(qb2, wName, pAssemblyName);
            MAKE_UTF8PTR_FROMWIDE(szQualName, (LPWSTR)qb2.Ptr());

            int iLen = (int)strlen(szQualName) + 1;
            if (qb->Alloc(iLen) == NULL)
                COMPlusThrowOM();
            strncpy((LPUTF8) qb->Ptr(), szQualName, iLen);
            fSetName = true;
        }
    }

    // In some cases above, we have written the Type name into the QuickBytes pointer already.
    // Make sure we don't call qb.Alloc then, which will free that memory, allocate new memory 
    // then try using the freed memory.
    if (!fSetName && qb->Ptr() != (void*)szToName) {
        int iLen = (int)strlen(szToName) + 1;
        if (qb->Alloc(iLen) == NULL)
            COMPlusThrowOM();
        strncpy((LPUTF8) qb->Ptr(), szToName, iLen);
    }
}

LPCUTF8 COMClass::_GetName(ReflectClass* pRC, BOOL fNameSpace, CQuickBytes *qb)
{
    THROWSCOMPLUSEXCEPTION();

    LPCUTF8         szcNameSpace;
    LPCUTF8         szToName;
    LPCUTF8         szcName;

    // Convert the name to a string
    pRC->GetName(&szcName, (fNameSpace) ? &szcNameSpace : NULL);
    if(!szcName) {
        _ASSERTE(!"Unable to get Name of Class");
        FATAL_EE_ERROR();
    }

    // Construct the fully qualified name
    if (fNameSpace && szcNameSpace && *szcNameSpace)
    {
        ns::MakePath(*qb, szcNameSpace, szcName);
        szToName = (LPCUTF8) qb->Ptr();
    }

    else
    {
        // For Arrays we really only have a single name which is fully qualified.
        // We need to remove the full qualification
        if (pRC->IsArray() && !fNameSpace) {
            szToName = ns::FindSep(szcName);
            if (szToName)
                ++szToName;
            else
                szToName = szcName;
        }
        else 
            szToName = szcName;
    }

    return szToName;
}

// _GetName
// If the bFullName is true, the fully qualified class name is returned
//  otherwise just the class name is returned.
StringObject* COMClass::_GetName(ReflectClassBaseObject* refThis, int nameType)
{
    StringObject        *refName;
    CQuickBytes       qb;

    THROWSCOMPLUSEXCEPTION();

    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    _ASSERTE(pRC);

    GetNameInternal(pRC, nameType, &qb);
    refName = (StringObject*)( OBJECTREFToObject( COMString::NewString((LPUTF8)qb.Ptr()) ));
    
    return refName;
}

// GetClassHandle
// This method with return a unique ID meaningful to the EE and equivalent to
// the result of the ldtoken instruction.
FCIMPL1(void*, COMClass::GetClassHandle, ReflectClassBaseObject* refThisUNSAFE)
{
    void*               pv      = NULL;
    REFLECTCLASSBASEREF refThis = (REFLECTCLASSBASEREF) refThisUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_1(refThis);

    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    _ASSERTE(pRC);

    if (pRC->IsArray()) {
        ReflectArrayClass* pRAC = (ReflectArrayClass*) pRC;
        TypeHandle ret = pRAC->GetTypeHandle();
        pv = ret.AsPtr();
    }
    else
    if (pRC->IsTypeDesc()) {
        ReflectTypeDescClass* pRTD = (ReflectTypeDescClass*) pRC;
        TypeHandle ret = pRTD->GetTypeHandle();
        pv = ret.AsPtr();
    }
    else
    {
        pv = pRC->GetClass()->GetMethodTable();
    }

    HELPER_METHOD_FRAME_END();
    return pv;
}
FCIMPLEND

// GetClassFromHandle
// This method with return a unique ID meaningful to the EE and equivalent to
// the result of the ldtoken instruction.
FCIMPL1(Object*, COMClass::GetClassFromHandle, LPVOID handle) {
    Object* retVal;

    if (handle == 0)
        FCThrowArgumentEx(kArgumentException, NULL, L"InvalidOperation_HandleIsNotInitialized");

    //
    // Get the TypeHandle from our handle and convert that to an EEClass.
    //
    TypeHandle typeHnd(handle);
    if (!typeHnd.IsTypeDesc()) {
        EEClass *pClass = typeHnd.GetClass();

        //
        // If we got an EEClass, check to see if we've already allocated 
        // a type object for it.  If we have, then simply return that one
        // and don't build a method frame.
        //
        if (pClass) {
            OBJECTREF o = pClass->GetExistingExposedClassObject();
            if (o != NULL) {
                return (OBJECTREFToObject(o));
            }
        }
    }

    //
    // We haven't already created the type object.  Create the helper 
    // method frame (we're going to be allocating an object) and call
    // the helper to create the object
    //
    HELPER_METHOD_FRAME_BEGIN_RET_0();
    retVal = OBJECTREFToObject(typeHnd.CreateClassObj());
    HELPER_METHOD_FRAME_END();
    return retVal;
}
FCIMPLEND

// This method triggers the class constructor for a give type
FCIMPL1(void, COMClass::RunClassConstructor, LPVOID handle) 
{
    if (handle == NULL)
        FCThrowArgumentVoidEx(kArgumentException, NULL, L"InvalidOperation_HandleIsNotInitialized");

    TypeHandle typeHnd(handle);
    if (typeHnd.IsUnsharedMT()) 
    {
        MethodTable *pMT = typeHnd.AsMethodTable();

        if (pMT->IsClassInited())
            return;

        if (pMT->IsShared())
        {
            DomainLocalBlock *pLocalBlock = GetAppDomain()->GetDomainLocalBlock();

            if (pLocalBlock->IsClassInitialized(pMT->GetSharedClassIndex()))
                return;
        }
                    
        OBJECTREF pThrowable;
        HELPER_METHOD_FRAME_BEGIN_1(pThrowable);
        if (!pMT->CheckRunClassInit(&pThrowable))
        {
            THROWSCOMPLUSEXCEPTION();
            COMPlusThrow(pThrowable);
        }
        HELPER_METHOD_FRAME_END();
    }
}
FCIMPLEND

INT32  __stdcall COMClass::InternalIsPrimitive(REFLECTCLASSBASEREF args)
{
    ReflectClass* pRC = (ReflectClass*) args->GetData();
    _ASSERTE(pRC);
    CorElementType type = pRC->GetCorElementType();
    return (InvokeUtil::IsPrimitiveType(type)) ? 1 : 0;
}   

// GetProperName 
// This method returns the fully qualified name of any type.  In other
// words, it now does the same thing as GetFullName() below.
FCIMPL1(Object*, COMClass::GetProperName, ReflectClassBaseObject* refThis)
{
    StringObject* refString = NULL;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refString);
    refString = _GetName(refThis, TYPE_NAME | TYPE_NAMESPACE);
    HELPER_METHOD_FRAME_END();
    return refString;
}
FCIMPLEND

// GetName 
// This method returns the unqualified name of a primitive as a String
FCIMPL1(Object*, COMClass::GetName, ReflectClassBaseObject* refThis)
{
   	StringObject* refString = NULL;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refString);
    refString = _GetName(refThis, TYPE_NAME);
    HELPER_METHOD_FRAME_END();

    return refString;
}
FCIMPLEND


// GetFullyName
// This will return the fully qualified name of the class as a String.
FCIMPL1(Object*, COMClass::GetFullName, ReflectClassBaseObject* refThis )
{
	StringObject* refString = NULL;
	HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refString);
	refString = _GetName( refThis, TYPE_NAME | TYPE_NAMESPACE);
	HELPER_METHOD_FRAME_END();
    
	return refString;
}
FCIMPLEND

// GetAssemblyQualifiedyName
// This will return the assembly qualified name of the class as a String.
FCIMPL1(Object*, COMClass::GetAssemblyQualifiedName, ReflectClassBaseObject* refThisUNSAFE)
{
    REFLECTCLASSBASEREF     refThis   = (REFLECTCLASSBASEREF)refThisUNSAFE;;
    STRINGREF               refString = NULL;;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_2(Frame::FRAME_ATTR_RETURNOBJ, refThis, refString);
    refString = ObjectToSTRINGREF(_GetName((ReflectClassBaseObject*)OBJECTREFToObject(refThis), TYPE_NAME | TYPE_NAMESPACE | TYPE_ASSEMBLY));
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refString);
}
FCIMPLEND

// GetNameSpace
// This will return the name space of a class as a String.
FCIMPL1(LPVOID, COMClass::GetNameSpace, ReflectClassBaseObject* refThis)
{

    LPVOID          rv                          = NULL;      // Return value
    LPCUTF8         szcName;
    LPCUTF8         szcNameSpace;
    STRINGREF       refName = NULL;


    THROWSCOMPLUSEXCEPTION();

    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    _ASSERTE(pRC);

    // Convert the name to a string
    pRC->GetName(&szcName, &szcNameSpace);
    if(!szcName) 
    {
        _ASSERTE(!"Unable to get Name of Class");
        FATAL_EE_ERROR();
    }

    if( !(szcNameSpace && *szcNameSpace) ) 
    {
        if (pRC->IsNested()) 
        {
            if (pRC->IsArray() || pRC->IsTypeDesc()) 
            {
                EEClass *pTypeClass = pRC->GetTypeHandle().GetClassOrTypeParam();
                _ASSERTE(pTypeClass);
                mdTypeDef mdEncl = pTypeClass->GetCl();
                IMDInternalImport *pImport = pTypeClass->GetMDImport();

                // Only look for nesting chain if this is a nested type.
                DWORD dwAttr = 0;
                pImport->GetTypeDefProps(mdEncl, &dwAttr, NULL);
                if (IsTdNested(dwAttr))
                {   // Get to the outermost class
                    while (SUCCEEDED(pImport->GetNestedClassProps(mdEncl, &mdEncl)));
                    pImport->GetNameOfTypeDef(mdEncl, &szcName, &szcNameSpace);
                }
            }
        }
        else 
        {
            if (pRC->IsArray()) 
            {
                int len = (int)strlen(szcName);
                const char* p = (len == 0) ? szcName : (szcName + len - 1);
                while (p != szcName && *p != '.') p--;
                if (p != szcName) 
                {
                    len = (int)(p - szcName);
                    char *copy = (char*) _alloca(len + 1);
                    strncpy(copy,szcName,len);
                    copy[len] = 0;
                    szcNameSpace = copy;
                }               
            }
        }
    }
    
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_2(Frame::FRAME_ATTR_RETURNOBJ, rv, refName);

    if(szcNameSpace && *szcNameSpace) 
    {
        // Create the string object
        refName = COMString::NewString(szcNameSpace);
    }

    *((STRINGREF *)&rv) = refName;

    HELPER_METHOD_FRAME_END();

    return rv;
}
FCIMPLEND

// GetGUID
// This method will return the version-independent GUID for the Class.  This is 
//  a CLSID for a class and an IID for an Interface.
FCIMPL1_INST_RET_VC(GUID, retRef, COMClass::GetGUID, ReflectClassBaseObject* refThisUNSAFE)
{
    REFLECTCLASSBASEREF refThis = (REFLECTCLASSBASEREF) refThisUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_VC_1(refThis);

    EEClass*        pVMC;

    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    pVMC = pRC->GetClass();
    THROWSCOMPLUSEXCEPTION();

    if (retRef == NULL)
        COMPlusThrow(kNullReferenceException);


    if (pRC->IsArray() || pRC->IsTypeDesc()) {
        memset(retRef,0,sizeof(GUID));
        goto lExit;
    }

    _ASSERTE(pVMC);
    GUID guid;
    pVMC->GetGuid(&guid, TRUE);
    memcpyNoGCRefs(retRef, &guid, sizeof(GUID));

lExit: ;
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND_RET_VC

// GetAttributeFlags
// Return the attributes that are associated with this Class.
FCIMPL1(INT32, COMClass::GetAttributeFlags, ReflectClassBaseObject* refThis) {
   
    VALIDATEOBJECTREF(refThis);

    THROWSCOMPLUSEXCEPTION();

    DWORD dwAttributes = tdPublic;

    ReflectClassBaseObject* reflectThis = NULL;
    HELPER_METHOD_FRAME_BEGIN_RET_1(reflectThis);
    reflectThis = refThis;
    
    if (reflectThis == NULL)
       COMPlusThrow(kNullReferenceException, L"NullReference_This");

    {
        ReflectClass* pRC = (ReflectClass*) (reflectThis->GetData());
        _ASSERTE(pRC);

        if (pRC == NULL)
            COMPlusThrow(kNullReferenceException);

        dwAttributes = pRC->GetAttributes();
    }
    HELPER_METHOD_FRAME_END();
        
    return dwAttributes;
}
FCIMPLEND

// IsArray
// This method return true if the Class represents an array.
FCIMPL1(INT32, COMClass::IsArray, ReflectClassBaseObject* refThisUNSAFE)
{
    INT32 ret = 0; 
    REFLECTCLASSBASEREF refThis = (REFLECTCLASSBASEREF) refThisUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_1(refThis);

    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    _ASSERTE(pRC);
    ret = pRC->IsArray();

    HELPER_METHOD_FRAME_END();
    return ret;
}
FCIMPLEND

// Invalidate the cached nested type information
FCIMPL1(INT32, COMClass::InvalidateCachedNestedType, ReflectClassBaseObject* refThisUNSAFE)
{
    REFLECTCLASSBASEREF refThis = (REFLECTCLASSBASEREF) refThisUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_1(refThis);

    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    _ASSERTE(pRC);
    pRC->InvalidateCachedNestedTypes();

    HELPER_METHOD_FRAME_END();
    return 0;
}   //InvalidateCachedNestedType
FCIMPLEND

// GetArrayElementType
// This routine will return the base type of a composite type.  
// It returns null if it is a plain type
FCIMPL1(Object*, COMClass::GetArrayElementType, ReflectClassBaseObject* refThisUNSAFE)
{
    REFLECTCLASSBASEREF  refThis    = (REFLECTCLASSBASEREF)refThisUNSAFE;
    OBJECTREF            refRetVal  = NULL;

    HELPER_METHOD_FRAME_BEGIN_RET_2(refThis, refRetVal);

    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    _ASSERTE(pRC);


    // If this is not an array class then throw an exception
    if (pRC->IsArray()) 
    {
        // Get the Element type handle and return the Type representing it.
        ReflectArrayClass* pRAC = (ReflectArrayClass*) pRC;
        TypeHandle elemType = pRAC->GetElementTypeHandle();
        // We can ignore the possible null return because this will not fail
        refRetVal = elemType.CreateClassObj();
    }
    else
    if (pRC->IsTypeDesc())
    {
        TypeDesc* td = pRC->GetTypeHandle().AsTypeDesc();
        TypeHandle th = td->GetTypeParam();
        // We can ignore the possible null return because this will not fail
        refRetVal = th.CreateClassObj();
    }

    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(refRetVal);
}
FCIMPLEND

// InternalGetArrayRank
// This routine will return the rank of an array assuming the Class represents an array.  
FCIMPL1(INT32, COMClass::InternalGetArrayRank, ReflectClassBaseObject* refThisUNSAFE)
{
    INT32               retVal  = 0;
    REFLECTCLASSBASEREF refThis = (REFLECTCLASSBASEREF) refThisUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_1(refThis);

    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    _ASSERTE(pRC);
    _ASSERTE( pRC->IsArray() );
 
    ReflectArrayClass* pRAC = (ReflectArrayClass*) pRC;
    retVal = pRAC->GetTypeHandle().AsArray()->GetRank();

    HELPER_METHOD_FRAME_END();
    return retVal;
}
FCIMPLEND


//CanCastTo
//Check to see if we can cast from one runtime type to another.
FCIMPL2(INT32, COMClass::CanCastTo, ReflectClassBaseObject* refFrom, ReflectClassBaseObject *refTo) 
{
    VALIDATEOBJECTREF(refFrom);
    VALIDATEOBJECTREF(refTo);

    if (refFrom->GetMethodTable() != g_pRefUtil->GetClass(RC_Class) ||
        refTo->GetMethodTable() != g_pRefUtil->GetClass(RC_Class))
        FCThrow(kArgumentException);

    // Find the properties
    ReflectClass* pRC = (ReflectClass*) refTo->GetData();
    TypeHandle toTH = pRC->GetTypeHandle();
    pRC = (ReflectClass*) refFrom->GetData();
    TypeHandle fromTH = pRC->GetTypeHandle();
    return fromTH.CanCastTo(toTH) ? 1 : 0;
}
FCIMPLEND


// IsPrimitive
// This method return true if the Class represents primitive type
FCIMPL1(INT32, COMClass::IsPrimitive, ReflectClassBaseObject* refThis) {
    VALIDATEOBJECTREF(refThis);

    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    _ASSERTE(pRC);
    CorElementType type = pRC->GetCorElementType();
    if (type == ELEMENT_TYPE_I && !pRC->GetClass()->IsTruePrimitive())
        return 0;
    return (InvokeUtil::IsPrimitiveType(type)) ? 1 : 0;
}
FCIMPLEND

// GetClass
// This is a static method defined on Class that will get a named class.
//  The name of the class is passed in by string.  The class name may be
//  either case sensitive or not.  This currently causes the class to be loaded
//  because it goes through the class loader.

// You get here from Type.GetType(typename)
// ECALL frame is used to find the caller

FCIMPL1(Object*, COMClass::GetClass1Arg, StringObject* classNameUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    STRINGREF   className = (STRINGREF) classNameUNSAFE;
    Object*     refRetVal = NULL;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, className);

    refRetVal = GetClassInner(&className, false, false, NULL, NULL, true, false);

    HELPER_METHOD_FRAME_END();

    return refRetVal;
}
FCIMPLEND

// You get here from Type.GetType(typename, bThowOnError)
// ECALL frame is used to find the caller
FCIMPL2(Object*, COMClass::GetClass2Args, StringObject* classNameUNSAFE, BYTE bThrowOnError)
{
    THROWSCOMPLUSEXCEPTION();

    STRINGREF   className = (STRINGREF) classNameUNSAFE;
    Object*     refRetVal = NULL;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, className);

    refRetVal = GetClassInner(&className, bThrowOnError, false, NULL, NULL, true, false);

    HELPER_METHOD_FRAME_END();

    return refRetVal;
}
FCIMPLEND

// You get here from Type.GetType(typename, bThowOnError, bIgnoreCase)
// ECALL frame is used to find the caller
FCIMPL3(Object*, COMClass::GetClass3Args, StringObject* classNameUNSAFE, BYTE bThrowOnError, BYTE bIgnoreCase)
{
    THROWSCOMPLUSEXCEPTION();

    STRINGREF   className = (STRINGREF) classNameUNSAFE;
    Object*     refRetVal = NULL;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, className);

    refRetVal = GetClassInner(&className, bThrowOnError, bIgnoreCase, NULL, NULL, true, false);

    HELPER_METHOD_FRAME_END();

    return refRetVal;
}
FCIMPLEND


// Called internally by mscorlib. No security checking performed.
FCIMPL4(Object*, COMClass::GetClassInternal, StringObject* classNameUNSAFE, BYTE bThrowOnError, BYTE bIgnoreCase, BYTE bPublicOnly)
{
    THROWSCOMPLUSEXCEPTION();

    STRINGREF   className = (STRINGREF) classNameUNSAFE;
    Object*     refRetVal = NULL;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, className);
    refRetVal = GetClassInner(&className, bThrowOnError, bIgnoreCase, NULL, NULL, false, bPublicOnly);
    HELPER_METHOD_FRAME_END();

    return refRetVal;
}
FCIMPLEND

// You get here if some BCL method calls RuntimeType.GetTypeImpl. In this case we cannot
// use the ECALL frame to find the caller, as it'll point to mscorlib ! In this case we use stackwalk/stackmark 
// to find the caller
FCIMPL5(Object*, COMClass::GetClass, StringObject* classNameUNSAFE, BYTE bThrowOnError, BYTE bIgnoreCase, StackCrawlMark* stackMark, BYTE* pbAssemblyIsLoading)
{
    THROWSCOMPLUSEXCEPTION();

    STRINGREF   className = (STRINGREF) classNameUNSAFE;
    Object*     refRetVal = NULL;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, className);

    BYTE* pfResult = NULL;
    if (*pbAssemblyIsLoading) 
    {
        *pbAssemblyIsLoading = FALSE;
        pfResult = pbAssemblyIsLoading;
    }

    refRetVal = GetClassInner(&className, bThrowOnError, bIgnoreCase, stackMark, pfResult, true, false);

    HELPER_METHOD_FRAME_END();

    return refRetVal;
}
FCIMPLEND

Object* COMClass::GetClassInner(STRINGREF *refClassName, 
                               BOOL bThrowOnError, 
                               BOOL bIgnoreCase, 
                               StackCrawlMark *stackMark,
                               BYTE *pbAssemblyIsLoading,
                               BOOL bVerifyAccess,
                               BOOL bPublicOnly)
{

    THROWSCOMPLUSEXCEPTION();

    STRINGREF       sRef = *refClassName;
    if (!sRef)
        COMPlusThrowArgumentNull(L"className",L"ArgumentNull_String");

    BOOL            errorInArrayDefinition = FALSE;
    LPUTF8          szSimpleClassName = NULL, szNameSpaceSep = NULL;
    DWORD           strLen = sRef->GetStringLength() + 1;
    LPUTF8          szFullClassName = (LPUTF8)_alloca(strLen);
    CQuickBytes     bytes;
    DWORD           cClassName;

    // Get the class name in UTF8
    if (!COMString::TryConvertStringDataToUTF8(sRef, szFullClassName, strLen))
        szFullClassName = GetClassStringVars(sRef, &bytes, &cClassName);

    if(!*szFullClassName)
      errorInArrayDefinition = TRUE;
    char* assembly = szFullClassName;

    // make sure every parameterized specification is skipped (i.e. int[*,*,*]) 
    BOOL normalize = FALSE;
    for (; *assembly; assembly++) {

        // break if a ',' - that is ASSEMBLY_SEPARATOR_CHAR - is encountered
        if (*assembly == ASSEMBLY_SEPARATOR_CHAR) {

            // "\," means that the comma is part of the original type name
            BOOL evenSlashes = TRUE;
            for (char *ptr=assembly;
                 (ptr != szFullClassName) && (*(ptr-1) == BACKSLASH_CHAR);
                 ptr--)
                evenSlashes = !evenSlashes;

            // Even # of slashes means there is no slash for this comma
            if (evenSlashes) {

                *assembly = '\0'; // so we have the name of the class with no noise (assembly name) in szFullClassName

                if (assembly - szFullClassName >= MAX_CLASSNAME_LENGTH)
                    COMPlusThrow(kArgumentException, L"Argument_TypeNameTooLong");

                while (COMCharacter::nativeIsWhiteSpace(*(++assembly))); // assembly now points to the assembly name
                break;
            }
        }
        else if (*assembly == '[') {
            // "\[" means that the bracket is part of the original type name
            BOOL evenSlashes = TRUE;
            for (char *ptr=assembly;
                 (ptr != szFullClassName) && (*(ptr-1) == BACKSLASH_CHAR);
                 ptr--)
                evenSlashes = !evenSlashes;

            // Even # of slashes means there is no slash for this bracket
            if (evenSlashes) {

                // array may contain ',' inside so skip to the closing array bracket
                for (;*assembly && *assembly != ']'; assembly++) {
                    if (*assembly == '*' || *assembly == '?')
                        normalize = TRUE;
                }
                if (!*assembly) { // array is malformed (no closing bracket)
                    errorInArrayDefinition = TRUE;
                    break;
                }
            }
        } 
        else if (*assembly == NAMESPACE_SEPARATOR_CHAR) {
            szNameSpaceSep = assembly;
            szSimpleClassName = assembly + 1;
        }
    }
    if (normalize) {
        // this function will change szFullClassName, notice that it can only shrink it
        if (!NormalizeArrayTypeName(szFullClassName, (*assembly) ? (DWORD)(assembly - szFullClassName - 1) : (DWORD) (assembly - szFullClassName) ))
            errorInArrayDefinition = TRUE;
    }

    if (!*szFullClassName)
      COMPlusThrow(kArgumentException, L"Format_StringZeroLength");

    // No assembly info with the type name - check full length
    if ((!*assembly) &&
        (assembly - szFullClassName >= MAX_CLASSNAME_LENGTH))
        COMPlusThrow(kArgumentException, L"Argument_TypeNameTooLong");

    OBJECTREF Throwable = NULL;
    EEClass *pCallersClass = NULL;
    Assembly *pCallersAssembly = NULL;
    void *returnIP = NULL;
    BOOL fCheckedPerm = FALSE;


    // Even if there's an error in the array def, we need the pCallersAssembly
    // or returnIP set for the exception message.
    if (bVerifyAccess || (assembly && *assembly)) {
        // Find the return address. This can be used to find caller's assembly.
        // If we're not checking security, the caller is always mscorlib.

        // 

        if (!bVerifyAccess)
            fCheckedPerm = TRUE;
    } else {
        pCallersAssembly = SystemDomain::SystemAssembly();
        fCheckedPerm = TRUE;
    }


    if (!errorInArrayDefinition) {
        LOG((LF_CLASSLOADER, 
             LL_INFO100, 
             "Get class %s through reflection\n", 
             szFullClassName));

        Assembly* pAssembly = NULL;
        TypeHandle typeHnd;
        NameHandle typeName;
        char noNameSpace = '\0';

        if (szNameSpaceSep) {
            *szNameSpaceSep = '\0';
            typeName.SetName(szFullClassName, szSimpleClassName);
        }
        else
            typeName.SetName(&noNameSpace, szFullClassName);

        if(bIgnoreCase)
            typeName.SetCaseInsensitive();

        // We need to pass in the throwable
        GCPROTECT_BEGIN(Throwable);

        if(assembly && *assembly) {

            AssemblySpec spec;
            HRESULT hr = spec.Init(assembly);

            if (SUCCEEDED(hr)) {

                pCallersClass = GetCallersClass(stackMark, returnIP);
                pCallersAssembly = (pCallersClass) ? pCallersClass->GetAssembly() : NULL;
                if (pCallersAssembly && (!pCallersAssembly->IsShared()))
                    spec.GetCodeBase()->SetParentAssembly(pCallersAssembly->GetFusionAssembly());

                hr = spec.LoadAssembly(&pAssembly, &Throwable, NULL, (pbAssemblyIsLoading != NULL));
                if(SUCCEEDED(hr)) {
                    typeHnd = pAssembly->FindNestedTypeHandle(&typeName, &Throwable);

                    if (typeHnd.IsNull() && (Throwable == NULL)) 
                        // If it wasn't in the available table, maybe it's an internal type
                            typeHnd = pAssembly->GetInternalType(&typeName, bThrowOnError, &Throwable);
                    }
                else if (pbAssemblyIsLoading &&
                         (hr == MSEE_E_ASSEMBLYLOADINPROGRESS))
                    *pbAssemblyIsLoading = TRUE;
            }
        }
        else {
            // Look for type in caller's assembly
            if (pCallersAssembly == NULL) {
                pCallersClass = GetCallersClass(stackMark, returnIP);
                pCallersAssembly = (pCallersClass) ? pCallersClass->GetAssembly() : NULL;
            }
            if(pCallersAssembly) {
                typeHnd = pCallersAssembly->FindNestedTypeHandle(&typeName, &Throwable);
                if (typeHnd.IsNull() && (Throwable == NULL))
                    // If it wasn't in the available table, maybe it's an internal type
                    typeHnd = pCallersAssembly->GetInternalType(&typeName, bThrowOnError, &Throwable);
            }

            // Look for type in system assembly
            if (typeHnd.IsNull() && (Throwable == NULL) && (pCallersAssembly != SystemDomain::SystemAssembly()))
                typeHnd = SystemDomain::SystemAssembly()->FindNestedTypeHandle(&typeName, &Throwable);

            BaseDomain *pDomain = SystemDomain::GetCurrentDomain();
            if (typeHnd.IsNull() &&
                (pDomain != SystemDomain::System())) {
                if (szNameSpaceSep)
                    *szNameSpaceSep = NAMESPACE_SEPARATOR_CHAR;
                if ((pAssembly = ((AppDomain*) pDomain)->RaiseTypeResolveEvent(szFullClassName, &Throwable)) != NULL) {
                    if (szNameSpaceSep)
                        *szNameSpaceSep = '\0';
                    typeHnd = pAssembly->FindNestedTypeHandle(&typeName, &Throwable);
                    
                    if (typeHnd.IsNull() && (Throwable == NULL)) {
                        // If it wasn't in the available table, maybe it's an internal type
                            typeHnd = pAssembly->GetInternalType(&typeName, bThrowOnError, &Throwable);
                        }
                    else
                        Throwable = NULL;
                }
            }

            if (!typeHnd.IsNull())
                pAssembly = typeHnd.GetAssembly();
        }
        
        if (Throwable != NULL && bThrowOnError)
            COMPlusThrow(Throwable);
        GCPROTECT_END();
        
        BOOL fVisible = TRUE;
        if (!typeHnd.IsNull() && !fCheckedPerm && bVerifyAccess) {

            // verify visibility
            EEClass *pClass = typeHnd.GetClassOrTypeParam();
            
            if (bPublicOnly && !(IsTdPublic(pClass->GetProtection()) || IsTdNestedPublic(pClass->GetProtection())))
                // the user is asking for a public class but the class we have is not public, discard
                fVisible = FALSE;
            else {
                // if the class is a top level public there is no check to perform
            if (!IsTdPublic(pClass->GetProtection())) {
                    if (!pCallersAssembly) {
                        pCallersClass = GetCallersClass(stackMark, returnIP);
                        pCallersAssembly = (pCallersClass) ? pCallersClass->GetAssembly() : NULL;
                    }
                
                    if (pCallersAssembly && // full trust for interop
                        !ClassLoader::CanAccess(pCallersClass,
                                                pCallersAssembly,
                                                pClass,
                                                pClass->GetAssembly(),
                                                pClass->GetAttrClass())) {
                        // This is not legal if the user doesn't have reflection permission
                        if (!AssemblyNative::HaveReflectionPermission(bThrowOnError))
                            fVisible = FALSE;
                    }
                }
            }
        }
        
        if ((!typeHnd.IsNull()) && fVisible)
            return(OBJECTREFToObject(typeHnd.CreateClassObj()));
    }

    if (bThrowOnError) {
        Throwable = NULL;
        GCPROTECT_BEGIN(Throwable);
        if (szNameSpaceSep)
            *szNameSpaceSep = NAMESPACE_SEPARATOR_CHAR;

        if (errorInArrayDefinition) 
            COMPlusThrow(kArgumentException, L"Argument_InvalidArrayName");
        else if (assembly && *assembly) {
            MAKE_WIDEPTR_FROMUTF8(pwzAssemblyName, assembly);
            PostTypeLoadException(NULL, szFullClassName, pwzAssemblyName,
                                  NULL, IDS_CLASSLOAD_GENERIC, &Throwable);
        }
        else if (pCallersAssembly ||
                 (pCallersAssembly = GetCallersAssembly(stackMark, returnIP)) != NULL)
            pCallersAssembly->PostTypeLoadException(szFullClassName, 
                                                    IDS_CLASSLOAD_GENERIC,
                                                    &Throwable);
        else {
            WCHAR   wszTemplate[30];
            if (FAILED(LoadStringRC(IDS_EE_NAME_UNKNOWN,
                                    wszTemplate,
                                    sizeof(wszTemplate)/sizeof(wszTemplate[0]),
                                    FALSE)))
                wszTemplate[0] = L'\0';
            PostTypeLoadException(NULL, szFullClassName, wszTemplate,
                                  NULL, IDS_CLASSLOAD_GENERIC, &Throwable);
        }

        COMPlusThrow(Throwable);
        GCPROTECT_END();
    }

    return NULL;
}

// GetSuperclass
// This method returns the Class Object representing the super class of this
//  Class.  If there is not super class then we return null.
FCIMPL1(LPVOID, COMClass::GetSuperclass, ReflectClassBaseObject* refThis)
{
    THROWSCOMPLUSEXCEPTION();

    // The the EEClass for this class (This must exist)
    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    EEClass*    pEEC = pRC->GetClass();
    if (pEEC) {
        if (pEEC->IsInterface())
            return 0;
    }
    TypeHandle typeHnd = pRC->GetTypeHandle();
    if (typeHnd.IsNull())
        return 0;
    TypeHandle parentType = typeHnd.GetParent();

    REFLECTCLASSBASEREF  refClass = 0;
    // We can ignore the Null return because Transparent proxy if final...
    if (!parentType.IsNull()) 
    {
        HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_NOPOLL(Frame::FRAME_ATTR_RETURNOBJ);
        refClass = (REFLECTCLASSBASEREF) parentType.CreateClassObj();
        HELPER_METHOD_FRAME_END();
    }
    
    return OBJECTREFToObject(refClass);
}
FCIMPLEND

// GetInterfaces
// This routine returns a Class[] containing all of the interfaces implemented
//  by this Class.  If the class implements no interfaces an array of length
//  zero is returned.
FCIMPL1(Object*, COMClass::GetInterfaces, ReflectClassBaseObject* refThisUNSAFE)
{
    PTRARRAYREF         refArrIFace = NULL;
    REFLECTCLASSBASEREF refThis     = (REFLECTCLASSBASEREF) refThisUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refThis);

    DWORD           i;

    THROWSCOMPLUSEXCEPTION();
    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    EEClass*    pVMC = pRC->GetClass();
    if (pVMC == 0) {
        _ASSERTE(pRC->IsTypeDesc());
        refArrIFace = (PTRARRAYREF) AllocateObjectArray(0, 
            g_pRefUtil->GetTrueType(RC_Class));

        goto lExit;
    }
    _ASSERTE(pVMC);

    // Allocate the COM+ array
    refArrIFace = (PTRARRAYREF) AllocateObjectArray(
        pVMC->GetNumInterfaces(), g_pRefUtil->GetTrueType(RC_Class));
    GCPROTECT_BEGIN(refArrIFace);

    // Create interface array
    for(i = 0; i < pVMC->GetNumInterfaces(); i++)
    {
        // Do not change this code.  This is done this way to
        //  prevent a GC hole in the SetObjectReference() call.  The compiler
        //  is free to pick the order of evaluation.
        OBJECTREF o = pVMC->GetInterfaceMap()[i].m_pMethodTable->GetClass()->GetExposedClassObject();
        refArrIFace->SetAt(i, o);
        _ASSERTE(refArrIFace->m_Array[i]);
    }

    GCPROTECT_END();
lExit: ;
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refArrIFace);
}
FCIMPLEND

// GetInterface
//  This method returns the interface based upon the name of the method.
FCIMPL3(Object*, COMClass::GetInterface, ReflectClassBaseObject* refThisUNSAFE, StringObject* interfaceNameUNSAFE, BYTE bIgnoreCase);
{
    REFLECTCLASSBASEREF refIFace        = NULL;
    REFLECTCLASSBASEREF refThis         = (REFLECTCLASSBASEREF) refThisUNSAFE;
    STRINGREF           interfaceName   = (STRINGREF)           interfaceNameUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_2(Frame::FRAME_ATTR_RETURNOBJ, refThis, interfaceName);

    LPCUTF8         pszIFaceName;
    LPCUTF8         pszIFaceNameSpace;
	LPUTF8          pszIFaceNameTmp;
    LPCUTF8         pszcCurIFaceName;
    LPCUTF8         pszcCurIFaceNameSpace;
    DWORD           cIFaceName;
    DWORD           dwNumIFaces;
    EEClass**       rgpVMCIFaces;
    EEClass*        pVMCCurIFace       = NULL;
    DWORD           i;

    THROWSCOMPLUSEXCEPTION();

    if (interfaceName == NULL)
        COMPlusThrow(kNullReferenceException);

    ReflectClass*   pRC = (ReflectClass*) refThis->GetData();
	EEClass*        pVMC = NULL;

    if (pRC->IsTypeDesc())
        goto lExit;

    pVMC = pRC->GetClass();
    _ASSERTE(pVMC);

    {
    CQuickBytes bytes;
    // Get the class name in UTF8
        pszIFaceNameTmp = GetClassStringVars(interfaceName,
                                           &bytes, &cIFaceName);
    }

    ns::SplitInline(pszIFaceNameTmp, pszIFaceNameSpace, pszIFaceName);

    // Get the array of interfaces
    dwNumIFaces = ReflectInterfaces::GetMaxCount(pVMC, false);
    
    if(dwNumIFaces)
    {
        rgpVMCIFaces = (EEClass**) _alloca(dwNumIFaces * sizeof(EEClass*));
        dwNumIFaces = ReflectInterfaces::GetInterfaces(pVMC, rgpVMCIFaces, false);
    }
    else
        rgpVMCIFaces = NULL;

    // Look for a matching interface
    for(i = 0; i < dwNumIFaces; i++)
    {
        // Get an interface's EEClass
        pVMCCurIFace = rgpVMCIFaces[i];
        _ASSERTE(pVMCCurIFace);

        // Convert the name to a string
        pVMCCurIFace->GetMDImport()->GetNameOfTypeDef(pVMCCurIFace->GetCl(),
            &pszcCurIFaceName, &pszcCurIFaceNameSpace);
        _ASSERTE(pszcCurIFaceName);

        if(pszIFaceNameSpace &&
           strcmp(pszIFaceNameSpace, pszcCurIFaceNameSpace))
            continue;

        // If the names are a match, break
        if(!bIgnoreCase)
        {
            if(!strcmp(pszIFaceName, pszcCurIFaceName))
                break;
        }
        else
            if(!_stricmp(pszIFaceName, pszcCurIFaceName))
                break;
    }

    // If we found an interface then lets save it
    if (i != dwNumIFaces)
    {

        refIFace = (REFLECTCLASSBASEREF) pVMCCurIFace->GetExposedClassObject();
        _ASSERTE(refIFace);
    }

lExit: ;
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refIFace);
}
FCIMPLEND

// GetMembers
// This method returns an array of Members containing all of the members
//  defined for the class.  Members include constructors, events, properties,
//  methods and fields.
FCIMPL2(Object*, COMClass::GetMembers, ReflectClassBaseObject* refThisUNSAFE, DWORD bindingAttr)
{
    PTRARRAYREF         pMembers    = NULL;
    REFLECTCLASSBASEREF refThis     = (REFLECTCLASSBASEREF) refThisUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refThis);

    DWORD           dwMembers;
    DWORD           dwCur;
    RefSecContext   sCtx;

    THROWSCOMPLUSEXCEPTION();
    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    _ASSERTE(pRC);

    COMMember::GetMemberInfo();
    _ASSERTE(COMMember::m_pMTIMember);

    bool ignoreCase = false;
    bool declaredOnly = false;

    bool addStatic = false;
    bool addInst = false;
    bool addPriv = false;
    bool addPub = false;

    ReflectMethodList* pMeths = NULL;
    ReflectMethodList* pCons = NULL;
    ReflectFieldList* pFlds = NULL;
    ReflectPropertyList *pProps = NULL;
    ReflectEventList *pEvents = NULL;
    ReflectTypeList* pNests = NULL;

	MethodTable *pParentMT = NULL;

	EEClass* pEEC = pRC->GetClass();
    if (pEEC == NULL){
        pMembers = (PTRARRAYREF)AllocateObjectArray(0, COMMember::m_pMTIMember->GetClass()->GetMethodTable());
        goto lExit;
    }

    // The Search modifiers
    ignoreCase = ((bindingAttr & BINDER_IgnoreCase)  != 0);
    declaredOnly = ((bindingAttr & BINDER_DeclaredOnly)  != 0);

    // The search filters
    addStatic = ((bindingAttr & BINDER_Static)  != 0);
    addInst = ((bindingAttr & BINDER_Instance)  != 0);
    addPriv = ((bindingAttr & BINDER_NonPublic) != 0);
    addPub = ((bindingAttr & BINDER_Public) != 0);

    // The member list...
    pMeths = pRC->GetMethods();
    pCons = pRC->GetConstructors();
    pFlds = pRC->GetFields();
    pProps = pRC->GetProperties();
    pEvents = pRC->GetEvents();
    pNests = pRC->GetNestedTypes();

    // We adjust the total number of members.
    dwMembers = pFlds->dwTotal + pMeths->dwTotal + pCons->dwTotal +
        pProps->dwTotal + pEvents->dwTotal + pNests->dwTypes;

    // Now create an array of IMembers
    pMembers = (PTRARRAYREF) AllocateObjectArray(
        dwMembers, COMMember::m_pMTIMember->GetClass()->GetMethodTable());
    GCPROTECT_BEGIN(pMembers);

    pParentMT = pEEC->GetMethodTable();

    dwCur = 0;

    // Fields
    if (pFlds->dwTotal) {
        // Load all those fields into the Allocated object array
        DWORD searchSpace = ((bindingAttr & BINDER_FlattenHierarchy) != 0) ? pFlds->dwTotal : pFlds->dwFields;
        for (DWORD i=0;i<searchSpace;i++) {
            // Check for access to publics, non-publics
            if (pFlds->fields[i].pField->IsPublic()) {
                if (!addPub) continue;
            }
            else {
                if (!addPriv) continue;
                if (!InvokeUtil::CheckAccess(&sCtx, pFlds->fields[i].pField->GetFieldProtection(), pParentMT, 0)) continue;
            }

            // Check for static instance 
            if (pFlds->fields[i].pField->IsStatic()) {
                if (!addStatic) continue;
            }
            else {
                if (!addInst) continue;
            }

            if (declaredOnly) {
                if (pFlds->fields[i].pField->GetEnclosingClass() != pEEC)
                    continue;
            }
              // Check for access to non-publics
            if (!addPriv && !pFlds->fields[i].pField->IsPublic())
                continue;

            // Do not change this code.  This is done this way to
            //  prevent a GC hole in the SetObjectReference() call.  The compiler
            //  is free to pick the order of evaluation.
            OBJECTREF o = (OBJECTREF) pFlds->fields[i].GetFieldInfo(pRC);
            pMembers->SetAt(dwCur++, o);
        }       
    }

    // Methods
    if (pMeths->dwTotal) {
        // Load all those fields into the Allocated object array
        DWORD searchSpace = ((bindingAttr & BINDER_FlattenHierarchy) != 0) ? pMeths->dwTotal : pMeths->dwMethods;
        for (DWORD i=0;i<searchSpace;i++) {
            // Check for access to publics, non-publics
            if (pMeths->methods[i].IsPublic()) {
                if (!addPub) continue;
            }
            else {
                if (!addPriv) continue;
                if (!InvokeUtil::CheckAccess(&sCtx, pMeths->methods[i].attrs, pParentMT, 0)) continue;
            }

            // Check for static instance 
            if (pMeths->methods[i].IsStatic()) {
                if (!addStatic) continue;
            }
            else {
                if (!addInst) continue;
            }

            if (declaredOnly) {
                if (pMeths->methods[i].pMethod->GetClass() != pEEC)
                    continue;
            }

            // If the method has a linktime security demand attached, check it now.
            if (!InvokeUtil::CheckLinktimeDemand(&sCtx, pMeths->methods[i].pMethod, false))
                continue;

            // Do not change this code.  This is done this way to
            //  prevent a GC hole in the SetObjectReference() call.  The compiler
            //  is free to pick the order of evaluation.
            OBJECTREF o = (OBJECTREF) pMeths->methods[i].GetMethodInfo(pRC);
            pMembers->SetAt(dwCur++, o);
        }       
    }

    // Constructors
    if (pCons->dwTotal) {
        for (DWORD i=0;i<pCons->dwMethods;i++) {
            // Check for static .cctors vs. instance .ctors
            if (pCons->methods[i].IsStatic()) {
                if (!addStatic) continue;
            }
            else {
                if (!addInst) continue;
            }

            // Check for access to publics, non-publics
            if (pCons->methods[i].IsPublic()) {
                if (!addPub) continue;
            }
            else {
                if (!addPriv) continue;
                if (!InvokeUtil::CheckAccess(&sCtx, pCons->methods[i].attrs, pParentMT, 0)) continue;
            }

            // If the method has a linktime security demand attached, check it now.
            if (!InvokeUtil::CheckLinktimeDemand(&sCtx, pCons->methods[i].pMethod, false))
                continue;

            // Do not change this code.  This is done this way to
            //  prevent a GC hole in the SetObjectReference() call.  The compiler
            //  is free to pick the order of evaluation.
            OBJECTREF o = (OBJECTREF) pCons->methods[i].GetConstructorInfo(pRC);
            pMembers->SetAt(dwCur++, o);
        }       
    }

    //Properties
    if (pProps->dwTotal) {
        DWORD searchSpace = ((bindingAttr & BINDER_FlattenHierarchy) != 0) ? pProps->dwTotal : pProps->dwProps;
        for (DWORD i = 0; i < searchSpace; i++) {
            // Check for access to publics, non-publics
            if (COMMember::PublicProperty(&pProps->props[i])) {
                if (!addPub) continue;
            }
            else {
                if (!addPriv) continue;
                if (!InvokeUtil::CheckAccess(&sCtx, mdAssem, pParentMT, 0)) continue;
            }

            if (declaredOnly) {
                if (pProps->props[i].pDeclCls != pEEC)
                     continue;
            }

            // Check for static instance 
            if (COMMember::StaticProperty(&pProps->props[i])) {
                if (!addStatic) continue;
            }
            else {
                if (!addInst) continue;
            }

            // Do not change this code.  This is done this way to
            //  prevent a GC hole in the SetObjectReference() call.  The compiler
            //  is free to pick the order of evaluation.
            OBJECTREF o = (OBJECTREF) pProps->props[i].GetPropertyInfo(pRC);
            pMembers->SetAt(dwCur++, o);
        }       
    }

    //Events
    if (pEvents->dwTotal) {
        DWORD searchSpace = ((bindingAttr & BINDER_FlattenHierarchy) != 0) ? pEvents->dwTotal : pEvents->dwEvents;
        for (DWORD i = 0; i < searchSpace; i++) {
            // Check for access to publics, non-publics
            if (COMMember::PublicEvent(&pEvents->events[i])) {
                if (!addPub) continue;
            }
            else {
                if (!addPriv) continue;
                if (!InvokeUtil::CheckAccess(&sCtx, mdAssem, pParentMT, 0)) continue;
            }

            if (declaredOnly) {
                if (pEvents->events[i].pDeclCls != pEEC)
                     continue;
            }
            // Check for static instance 
            if (COMMember::StaticEvent(&pEvents->events[i])) {
                if (!addStatic) continue;
            }
            else {
                if (!addInst) continue;
            }

            // Do not change this code.  This is done this way to
            //  prevent a GC hole in the SetObjectReference() call.  The compiler
            //  is free to pick the order of evaluation.
            OBJECTREF o = (OBJECTREF) pEvents->events[i].GetEventInfo(pRC);
            pMembers->SetAt(dwCur++, o);
        }       
    }

    //Nested Types
    if (pNests->dwTypes) {
        for (DWORD i=0;i<pNests->dwTypes;i++) {

            // Check for access to publics, non-publics
            if (IsTdNestedPublic(pNests->types[i]->GetAttrClass())) {
                if (!addPub) continue;
            }
            else {
                if (!addPriv) continue;
            }
            if (!InvokeUtil::CheckAccessType(&sCtx, pNests->types[i], 0)) continue;

            // Do not change this code.  This is done this way to
            //  prevent a GC hole in the SetObjectReference() call.  The compiler
            //  is free to pick the order of evaluation.
            OBJECTREF o = (OBJECTREF) pNests->types[i]->GetExposedClassObject();
            pMembers->SetAt(dwCur++, o);
        }       
    }

    _ASSERTE(dwCur <= dwMembers);

    if (dwCur != dwMembers) {
        PTRARRAYREF retArray = (PTRARRAYREF) AllocateObjectArray(
            dwCur, COMMember::m_pMTIMember->GetClass()->GetMethodTable());

        for(DWORD i = 0; i < dwCur; i++)
            retArray->SetAt(i, pMembers->GetAt(i));
        pMembers = retArray;
    }

    GCPROTECT_END();

lExit: ;
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(pMembers);
}
FCIMPLEND

/*========================GetSerializationRegistryValues========================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
FCIMPL2(void, COMClass::GetSerializationRegistryValues, BYTE *ignoreBit, BYTE *logNonSerializable) {
    *ignoreBit = !!g_pConfig->GetConfigDWORD(SERIALIZATION_BIT_KEY, SERIALIZATION_BIT_ZERO);
    *logNonSerializable = !!g_pConfig->GetConfigDWORD(SERIALIZATION_LOG_KEY, SERIALIZATION_BIT_ZERO);
}
FCIMPLEND

/*============================GetSerializableMembers============================
**Action: Creates an array of all non-static fields and properties
**        on a class.  Properties are also excluded if they don't have get and set
**        methods. Transient fields and properties are excluded based on the value 
**        of args->bFilterTransient.  Essentially, transients are exluded for 
**        serialization but not for cloning.
**Returns: An array of all of the members that should be serialized.
**Arguments: args->refClass: The class being serialized
**           args->bFilterTransient: True if transient members should be excluded.
**Exceptions:
==============================================================================*/
FCIMPL2(Object*, COMClass::GetSerializableMembers, ReflectClassBaseObject* refClassUNSAFE, BYTE bFilterTransient)
{
    PTRARRAYREF         pMembers = NULL;
    REFLECTCLASSBASEREF refClass = (REFLECTCLASSBASEREF) refClassUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refClass);

    DWORD           dwMembers;
    DWORD           dwCur;
    mdFieldDef      fieldDef;
    DWORD           dwFlags;

    //All security checks should be handled in managed code.

    THROWSCOMPLUSEXCEPTION();

    if (refClass == NULL)
        COMPlusThrow(kNullReferenceException);

    ReflectClass* pRC = (ReflectClass*) refClass->GetData();
    _ASSERTE(pRC);

    COMMember::GetMemberInfo();
    _ASSERTE(COMMember::m_pMTIMember);

    ReflectFieldList* pFlds = pRC->GetFields();

    dwMembers = pFlds->dwFields;

    // Now create an array of IMembers
    pMembers = (PTRARRAYREF) AllocateObjectArray(dwMembers, COMMember::m_pMTIMember->GetClass()->GetMethodTable());
    GCPROTECT_BEGIN(pMembers);

    dwCur = 0;
    // Fields
    if (pFlds->dwFields) {
        // Load all those fields into the Allocated object array
        for (DWORD i=0;i<pFlds->dwFields;i++) {
            //We don't serialize static fields.
            if (pFlds->fields[i].pField->IsStatic()) {
                continue;
            }

            //Check for the transient (e.g. don't serialize) bit.  
            fieldDef = (pFlds->fields[i].pField->GetMemberDef());
            dwFlags = (pFlds->fields[i].pField->GetMDImport()->GetFieldDefProps(fieldDef));
            if (IsFdNotSerialized(dwFlags)) {
                continue;
            }

            // Do not change this code.  This is done this way to
            //  prevent a GC hole in the SetObjectReference() call.  The compiler
            //  is free to pick the order of evaluation.
            OBJECTREF o = (OBJECTREF) pFlds->fields[i].GetFieldInfo(pRC);
            pMembers->SetAt(dwCur++, o);
        }       
    }

    //We we have extra space in the array, copy before returning.
    if (dwCur != dwMembers) {
        PTRARRAYREF retArray = (PTRARRAYREF) AllocateObjectArray(
            dwCur, COMMember::m_pMTIMember->GetClass()->GetMethodTable());

        for(DWORD i = 0; i < dwCur; i++)
            retArray->SetAt(i, pMembers->GetAt(i));

        pMembers = retArray;        
    }

    GCPROTECT_END();
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(pMembers);
}
FCIMPLEND

// GetMember
// This method will return an array of Members which match the name
//  passed in.  There may be 0 or more matching members.
FCIMPL4(Object*, COMClass::GetMember, ReflectClassBaseObject* refThisUNSAFE, StringObject* memberNameUNSAFE, DWORD memberType, DWORD bindingAttr)
{
    PTRARRAYREF         refArrIMembers  = NULL;
    REFLECTCLASSBASEREF refThis         = (REFLECTCLASSBASEREF) refThisUNSAFE;
    STRINGREF           memberName      = (STRINGREF)           memberNameUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_2(Frame::FRAME_ATTR_RETURNOBJ, refThis, memberName);

    THROWSCOMPLUSEXCEPTION();

    ReflectField**      rgpFields = NULL;
    ReflectMethod**     rgpMethods = NULL;
    ReflectMethod**     rgpCons = NULL;
    ReflectProperty**   rgpProps = NULL;
    ReflectEvent**      rgpEvents = NULL;
    EEClass**           rgpNests = NULL;

    DWORD           dwNumFields = 0;
    DWORD           dwNumMethods = 0;
    DWORD           dwNumCtors = 0;
    DWORD           dwNumProps = 0;
    DWORD           dwNumEvents = 0;
    DWORD           dwNumNests = 0;

    DWORD           dwNumMembers = 0;

    DWORD           i;
    DWORD           dwCurIndex;
    bool            bIsPrefix       = false;
    RefSecContext   sCtx;

    if (memberName == 0)
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_String");

    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    _ASSERTE(pRC);

    EEClass* pEEC = pRC->GetClass();
    MethodTable *pParentMT = NULL;
    if (pEEC) 
        pParentMT = pEEC->GetMethodTable();

    // The Search modifiers
    bool bIgnoreCase = ((bindingAttr & BINDER_IgnoreCase) != 0);
    bool declaredOnly = ((bindingAttr & BINDER_DeclaredOnly)  != 0);

    // The search filters
    bool addStatic = ((bindingAttr & BINDER_Static)  != 0);
    bool addInst = ((bindingAttr & BINDER_Instance)  != 0);
    bool addPriv = ((bindingAttr & BINDER_NonPublic) != 0);
    bool addPub = ((bindingAttr & BINDER_Public) != 0);

    CQuickBytes bytes;
    LPSTR szMemberName;
    DWORD cMemberName;

    // Convert the STRINGREF to UTF8
    szMemberName = GetClassStringVars(memberName, 
                                      &bytes, &cMemberName);

    // Check to see if wzPrefix requires an exact match of method names or is just a prefix
    if(szMemberName[cMemberName-1] == '*') {
        bIsPrefix = true;
        szMemberName[--cMemberName] = '\0';
    }

    // Get the maximums for each member type
    // Fields
    ReflectFieldList* pFlds = NULL;
    if ((memberType & MEMTYPE_Field) != 0) {
        pFlds = pRC->GetFields();
        rgpFields = (ReflectField**) _alloca(pFlds->dwTotal * sizeof(ReflectField*));
    }

    // Methods
    ReflectMethodList* pMeths = NULL;
    if ((memberType & MEMTYPE_Method) != 0) {
        pMeths = pRC->GetMethods();
        rgpMethods = (ReflectMethod**) _alloca(pMeths->dwTotal * sizeof(ReflectMethod*));
    }
    
    // Properties
    ReflectPropertyList *pProps = NULL;
    if ((memberType & MEMTYPE_Property) != 0) {
        pProps = pRC->GetProperties();
        rgpProps = (ReflectProperty**) _alloca(pProps->dwTotal * sizeof (ReflectProperty*));
    }

    // Events
    ReflectEventList *pEvents = NULL;
    if ((memberType & MEMTYPE_Event) != 0) {
        pEvents = pRC->GetEvents();
        rgpEvents = (ReflectEvent**) _alloca(pEvents->dwTotal * sizeof (ReflectEvent*));
    }

    // Nested Types
    ReflectTypeList*    pNests = NULL;
    if ((memberType & MEMTYPE_NestedType) != 0) {
        pNests = pRC->GetNestedTypes();
        rgpNests = (EEClass**) _alloca(pNests->dwTypes * sizeof (EEClass*));
    }

    // Filter the constructors
    ReflectMethodList* pCons = 0;

    // Check to see if they are looking for the constructors
    if ((memberType & MEMTYPE_Constructor) != 0) {
        if((!bIsPrefix && strlen(COR_CTOR_METHOD_NAME) != cMemberName)
           || (!bIgnoreCase && strncmp(COR_CTOR_METHOD_NAME, szMemberName, cMemberName))
           || (bIgnoreCase && _strnicmp(COR_CTOR_METHOD_NAME, szMemberName, cMemberName)))
        {
            pCons = 0;
        }
        else {
            pCons = pRC->GetConstructors();
            DWORD searchSpace = ((bindingAttr & BINDER_FlattenHierarchy) != 0) ? pCons->dwTotal : pCons->dwMethods;
            rgpCons = (ReflectMethod**) _alloca(searchSpace * sizeof(ReflectMethod*));
            dwNumCtors = 0;
            for (i = 0; i < searchSpace; i++) {
                // Ignore the class constructor  (if one was present)
                if (pCons->methods[i].IsStatic())
                    continue;

                // Check for access to non-publics
                if (pCons->methods[i].IsPublic()) {
                    if (!addPub) continue;
                }
                else {
                    if (!addPriv) continue;
                    if (!InvokeUtil::CheckAccess(&sCtx, pCons->methods[i].attrs, pParentMT, 0)) continue;
                }

                if (declaredOnly) {
                    if (pCons->methods[i].pMethod->GetClass() != pEEC)
                        continue;
                }

                // If the method has a linktime security demand attached, check it now.
                if (!InvokeUtil::CheckLinktimeDemand(&sCtx, pCons->methods[i].pMethod, false))
                    continue;

                rgpCons[dwNumCtors++] = &pCons->methods[i];
            }
        }

        // check for the class initializer  (We can only be doing either
        //  the class initializer or a constructor so we are using the 
        //  same set of variables.
        if((!bIsPrefix && strlen(COR_CCTOR_METHOD_NAME) != cMemberName)
           || (!bIgnoreCase && strncmp(COR_CCTOR_METHOD_NAME, szMemberName, cMemberName))
           || (bIgnoreCase && _strnicmp(COR_CCTOR_METHOD_NAME, szMemberName, cMemberName)))
        {
            pCons = 0;
        }
        else {
            pCons = pRC->GetConstructors();
            DWORD searchSpace = ((bindingAttr & BINDER_FlattenHierarchy) != 0) ? pCons->dwTotal : pCons->dwMethods;
            rgpCons = (ReflectMethod**) _alloca(searchSpace * sizeof(ReflectMethod*));
            dwNumCtors = 0;
            for (i = 0; i < searchSpace; i++) {
                // Ignore the normal constructors constructor  (if one was present)
                if (!pCons->methods[i].IsStatic())
                    continue;

                // Check for access to non-publics
                if (pCons->methods[i].IsPublic()) {
                    if (!addPub) continue;
                }
                else {
                    if (!addPriv) continue;
                    if (!InvokeUtil::CheckAccess(&sCtx, pCons->methods[i].attrs, pParentMT, 0)) continue;
                }

                if (declaredOnly) {
                    if (pCons->methods[i].pMethod->GetClass() != pEEC)
                        continue;
                }

                // If the method has a linktime security demand attached, check it now.
                if (!InvokeUtil::CheckLinktimeDemand(&sCtx, pCons->methods[i].pMethod, false))
                    continue;

                rgpCons[dwNumCtors++] = &pCons->methods[i];
            }
        }
    }
    else
        dwNumCtors = 0;

    // Filter the fields
    if ((memberType & MEMTYPE_Field) != 0) {
        DWORD searchSpace = ((bindingAttr & BINDER_FlattenHierarchy) != 0) ? pFlds->dwTotal : pFlds->dwFields;
        for(i = 0, dwCurIndex = 0; i < searchSpace; i++)
        {
            // Check for access to publics, non-publics
            if (pFlds->fields[i].pField->IsPublic()) {
                if (!addPub) continue;
            }
            else {
                if (!addPriv) continue;
                if (!InvokeUtil::CheckAccess(&sCtx, pFlds->fields[i].pField->GetFieldProtection(), pParentMT, 0)) continue;
            }

            // Check for static instance 
            if (pFlds->fields[i].pField->IsStatic()) {
                if (!addStatic) continue;
            }
            else {
                if (!addInst) continue;
            }

            if (declaredOnly) {
                if (pFlds->fields[i].pField->GetEnclosingClass() != pEEC)
                    continue;
            }

            // Get the name of the current field
            LPCUTF8 pszCurFieldName = pFlds->fields[i].pField->GetName();

            // Check to see that the current field matches the name requirements
            if(!bIsPrefix && strlen(pszCurFieldName) != cMemberName)
                continue;

            // Determine if it passes criteria
            if(!bIgnoreCase)
            {
                if(strncmp(pszCurFieldName, szMemberName, cMemberName))
                    continue;
            }
            else {
                if(_strnicmp(pszCurFieldName, szMemberName, cMemberName))
                    continue;
            }

            // Field passed, so save it
            rgpFields[dwCurIndex++] = &pFlds->fields[i];
        }
        dwNumFields = dwCurIndex;
    }
    else 
        dwNumFields = 0;

    // Filter the methods
    if ((memberType & MEMTYPE_Method) != 0) {
        DWORD searchSpace = ((bindingAttr & BINDER_FlattenHierarchy) != 0) ? pMeths->dwTotal : pMeths->dwMethods;
        for (i = 0, dwCurIndex = 0; i < searchSpace; i++) {
            // Check for access to publics, non-publics
            if (pMeths->methods[i].IsPublic()) {
                if (!addPub) continue;
            }
            else {
                if (!addPriv) continue;
                if (!InvokeUtil::CheckAccess(&sCtx, pMeths->methods[i].attrs, pParentMT, 0)) continue;
            }

            // Check for static instance 
            if (pMeths->methods[i].IsStatic()) {
                if (!addStatic) continue;
            }
            else {
                if (!addInst) continue;
            }

            if (declaredOnly) {
                if (pMeths->methods[i].pMethod->GetClass() != pEEC)
                    continue;
            }

            // Check to see that the current method matches the name requirements
            if(!bIsPrefix && pMeths->methods[i].dwNameCnt != cMemberName)
                continue;

            // Determine if it passes criteria
            if(!bIgnoreCase)
            {
                if(strncmp(pMeths->methods[i].szName, szMemberName, cMemberName))
                    continue;
            }
            else {
                if(_strnicmp(pMeths->methods[i].szName, szMemberName, cMemberName))
                    continue;
            }
        
            // If the method has a linktime security demand attached, check it now.
            if (!InvokeUtil::CheckLinktimeDemand(&sCtx, pMeths->methods[i].pMethod, false))
                continue;

            // Field passed, so save it
            rgpMethods[dwCurIndex++] = &pMeths->methods[i];
        }
        dwNumMethods = dwCurIndex;
    }
    else
        dwNumMethods = 0;

    //Filter the properties
    if ((memberType & MEMTYPE_Property) != 0) {
        DWORD searchSpace = ((bindingAttr & BINDER_FlattenHierarchy) != 0) ? pProps->dwTotal : pProps->dwProps;
        for (i = 0, dwCurIndex = 0; i < searchSpace; i++) {
            // Check for access to publics, non-publics
            if (COMMember::PublicProperty(&pProps->props[i])) {
                if (!addPub) continue;
            }
            else {
                if (!addPriv) continue;
                if (!InvokeUtil::CheckAccess(&sCtx, mdAssem, pParentMT, 0)) continue;
            }

            if (declaredOnly) {
                if (pProps->props[i].pDeclCls != pEEC)
                     continue;
            }
            // Check fo static instance 
            if (COMMember::StaticProperty(&pProps->props[i])) {
                if (!addStatic) continue;
            }
            else {
                if (!addInst) continue;
            }

            // Check to see that the current property matches the name requirements
            DWORD dwNameCnt = (DWORD)strlen(pProps->props[i].szName);
            if(!bIsPrefix && dwNameCnt != cMemberName)
                continue;

            // Determine if it passes criteria
            if(!bIgnoreCase)
            {
                if(strncmp(pProps->props[i].szName, szMemberName, cMemberName))
                    continue;
            }
            else {
                if(_strnicmp(pProps->props[i].szName, szMemberName, cMemberName))
                    continue;
            }

            // Property passed, so save it
            rgpProps[dwCurIndex++] = &pProps->props[i];
        }
        dwNumProps = dwCurIndex;
    }
    else
        dwNumProps = 0;

    //Filter the events
    if ((memberType & MEMTYPE_Event) != 0) {
        DWORD searchSpace = ((bindingAttr & BINDER_FlattenHierarchy) != 0) ? pEvents->dwTotal : pEvents->dwEvents;
        for (i = 0, dwCurIndex = 0; i < searchSpace; i++) {
            // Check for access to publics, non-publics
            if (COMMember::PublicEvent(&pEvents->events[i])) {
                if (!addPub) continue;
            }
            else {
                if (!addPriv) continue;
                if (!InvokeUtil::CheckAccess(&sCtx, mdAssem, pParentMT, 0)) continue;
            }

            if (declaredOnly) {
                if (pEvents->events[i].pDeclCls != pEEC)
                     continue;
            }

            // Check fo static instance 
            if (COMMember::StaticEvent(&pEvents->events[i])) {
                if (!addStatic) continue;
            }
            else {
                if (!addInst) continue;
            }

            // Check to see that the current event matches the name requirements
            DWORD dwNameCnt = (DWORD)strlen(pEvents->events[i].szName);
            if(!bIsPrefix && dwNameCnt != cMemberName)
                continue;

            // Determine if it passes criteria
            if(!bIgnoreCase)
            {
                if(strncmp(pEvents->events[i].szName, szMemberName, cMemberName))
                    continue;
            }
            else {
                if(_strnicmp(pEvents->events[i].szName, szMemberName, cMemberName))
                    continue;
            }

            // Property passed, so save it
            rgpEvents[dwCurIndex++] = &pEvents->events[i];
        }
        dwNumEvents = dwCurIndex;
    }
    else
        dwNumEvents = 0;

    // Filter the Nested Classes
    if ((memberType & MEMTYPE_NestedType) != 0) {
        LPCUTF8          pszNestName;
        LPCUTF8          pszNestNameSpace;

        ns::SplitInline(szMemberName, pszNestNameSpace, pszNestName);
        DWORD cNameSpace;
        if (pszNestNameSpace)
            cNameSpace = (DWORD)strlen(pszNestNameSpace);
        else
            cNameSpace = 0;
        DWORD cName = (DWORD)strlen(pszNestName);
        for (i = 0, dwCurIndex = 0; i < pNests->dwTypes; i++) {
            // Check for access to non-publics
            if (!addPriv && !IsTdNestedPublic(pNests->types[i]->GetAttrClass()))
                continue;
            if (!InvokeUtil::CheckAccessType(&sCtx, pNests->types[i], 0)) continue;

            LPCUTF8 szcName;
            LPCUTF8 szcNameSpace;
            REFLECTCLASSBASEREF o = (REFLECTCLASSBASEREF) pNests->types[i]->GetExposedClassObject();
            ReflectClass* thisRC = (ReflectClass*) o->GetData();
            _ASSERTE(thisRC);

            //******************************************************************
            //  This code will simply take any nested class whose namespace 
            //  has the given namespace as a prefix AND has a name with the 
            //  given name as a prefix.
            // Note that this code also assumes that nested classes are not
            //  decorated as Containing$Nested.
            
            thisRC->GetName(&szcName,&szcNameSpace);
            if(pszNestNameSpace) {

                // Check to see that the nested type matches the namespace requirements
                if(strlen(szcNameSpace) != cNameSpace)
                    continue;

                if (!bIgnoreCase) {
                    if (strncmp(pszNestNameSpace, szcNameSpace, cNameSpace))
                        continue;
                }
                else {
                    if (_strnicmp(pszNestNameSpace, szcNameSpace, cNameSpace))
                        continue;
                }
            }

            // Check to see that the nested type matches the name requirements
            if(!bIsPrefix && strlen(szcName) != cName)
                continue;

            // If the names are a match, break
            if (!bIgnoreCase) {
                if (strncmp(pszNestName, szcName, cName))
                    continue;
            }
            else {
                if (_strnicmp(pszNestName, szcName, cName))
                    continue;
            }

            // Nested Type passed, so save it
            rgpNests[dwCurIndex++] = pNests->types[i];
        }
        dwNumNests = dwCurIndex;
    }
    else
        dwNumNests = 0;


    // Get a grand total
    dwNumMembers = dwNumFields + dwNumMethods + dwNumCtors + dwNumProps + dwNumEvents + dwNumNests;

    // Now create an array of proper MemberInfo
    MethodTable *pArrayType = NULL;
    if (memberType == MEMTYPE_Method) {
        _ASSERTE(dwNumFields + dwNumCtors + dwNumProps + dwNumEvents + dwNumNests == 0);
        if (!COMMember::m_pMTMethodInfo) {
            COMMember::m_pMTMethodInfo = g_Mscorlib.GetClass(CLASS__METHOD_INFO);
        }
        pArrayType = COMMember::m_pMTMethodInfo;
    }
    else if (memberType == MEMTYPE_Field) {
        _ASSERTE(dwNumMethods + dwNumCtors + dwNumProps + dwNumEvents + dwNumNests == 0);
        if (!COMMember::m_pMTFieldInfo) {
            COMMember::m_pMTFieldInfo = g_Mscorlib.GetClass(CLASS__FIELD_INFO);
        }
        pArrayType = COMMember::m_pMTFieldInfo;
    }
    else if (memberType == MEMTYPE_Property) {
        _ASSERTE(dwNumFields + dwNumMethods + dwNumCtors + dwNumEvents + dwNumNests == 0);
        if (!COMMember::m_pMTPropertyInfo) {
            COMMember::m_pMTPropertyInfo = g_Mscorlib.GetClass(CLASS__PROPERTY_INFO);
        }
        pArrayType = COMMember::m_pMTPropertyInfo;
    }
    else if (memberType == MEMTYPE_Constructor) {
        _ASSERTE(dwNumFields + dwNumMethods + dwNumProps + dwNumEvents + dwNumNests == 0);
        if (!COMMember::m_pMTConstructorInfo) {
            COMMember::m_pMTConstructorInfo = g_Mscorlib.GetClass(CLASS__CONSTRUCTOR_INFO);
        }
        pArrayType = COMMember::m_pMTConstructorInfo;
    }
    else if (memberType == MEMTYPE_Event) {
        _ASSERTE(dwNumFields + dwNumMethods + dwNumCtors + dwNumProps + dwNumNests == 0);
        if (!COMMember::m_pMTEventInfo) {
            COMMember::m_pMTEventInfo = g_Mscorlib.GetClass(CLASS__EVENT_INFO);
        }
        pArrayType = COMMember::m_pMTEventInfo;
    }
    else if (memberType == MEMTYPE_NestedType) {
        _ASSERTE(dwNumFields + dwNumMethods + dwNumCtors + dwNumProps + dwNumEvents == 0);
        if (!COMMember::m_pMTType) {
            COMMember::m_pMTType = g_Mscorlib.GetClass(CLASS__TYPE);
        }
        pArrayType = COMMember::m_pMTType;
    }
    else if (memberType == (MEMTYPE_Constructor | MEMTYPE_Method)) {
        _ASSERTE(dwNumFields + dwNumProps + dwNumEvents + dwNumNests == 0);
        if (!COMMember::m_pMTMethodBase) {
            COMMember::m_pMTMethodBase = g_Mscorlib.GetClass(CLASS__METHOD_BASE);
        }
        pArrayType = COMMember::m_pMTMethodBase;
    }

    COMMember::GetMemberInfo();
    _ASSERTE(COMMember::m_pMTIMember);

    if (pArrayType == NULL)
        pArrayType = COMMember::m_pMTIMember;

    refArrIMembers = (PTRARRAYREF) AllocateObjectArray(dwNumMembers, pArrayType->GetClass()->GetMethodTable());
    GCPROTECT_BEGIN(refArrIMembers);

    // NO GC Below here
    // Now create and assign the reflection objects into the array
    for (i = 0, dwCurIndex = 0; i < dwNumFields; i++, dwCurIndex++)
    {
        // Do not change this code.  This is done this way to
        //  prevent a GC hole in the SetObjectReference() call.  The compiler
        //  is free to pick the order of evaluation.
        OBJECTREF o = (OBJECTREF) rgpFields[i]->GetFieldInfo(pRC);
        refArrIMembers->SetAt(dwCurIndex, o);
    }

    for (i = 0; i < dwNumMethods; i++, dwCurIndex++)
    {
        // Do not change this code.  This is done this way to
        //  prevent a GC hole in the SetObjectReference() call.  The compiler
        //  is free to pick the order of evaluation.
        OBJECTREF o = (OBJECTREF) rgpMethods[i]->GetMethodInfo(pRC);
        refArrIMembers->SetAt(dwCurIndex, o);
    }

    for (i = 0; i < dwNumCtors; i++, dwCurIndex++)
    {
        // Do not change this code.  This is done this way to
        //  prevent a GC hole in the SetObjectReference() call.  The compiler
        //  is free to pick the order of evaluation.
        OBJECTREF o = (OBJECTREF) rgpCons[i]->GetConstructorInfo(pRC);
        refArrIMembers->SetAt(dwCurIndex, o);
    }

    for (i = 0; i < dwNumProps; i++, dwCurIndex++)
    {
        // Do not change this code.  This is done this way to
        //  prevent a GC hole in the SetObjectReference() call.  The compiler
        //  is free to pick the order of evaluation.
        OBJECTREF o = (OBJECTREF) rgpProps[i]->GetPropertyInfo(pRC);
        refArrIMembers->SetAt(dwCurIndex, o);
    }

    for (i = 0; i < dwNumEvents; i++, dwCurIndex++)
    {
        // Do not change this code.  This is done this way to
        //  prevent a GC hole in the SetObjectReference() call.  The compiler
        //  is free to pick the order of evaluation.
        OBJECTREF o = (OBJECTREF) rgpEvents[i]->GetEventInfo(pRC);
        refArrIMembers->SetAt(dwCurIndex, o);
    }

    for (i = 0; i < dwNumNests; i++, dwCurIndex++)
    {
        // Do not change this code.  This is done this way to
        //  prevent a GC hole in the SetObjectReference() call.  The compiler
        //  is free to pick the order of evaluation.
        OBJECTREF o = (OBJECTREF) rgpNests[i]->GetExposedClassObject();
        refArrIMembers->SetAt(dwCurIndex, o);
    }

    GCPROTECT_END();

    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refArrIMembers);
}
FCIMPLEND

// GetModule
// This will return the module that the class is defined in.
FCIMPL1(LPVOID, COMClass::GetModule, ReflectClassBaseObject* refThis)
{
    OBJECTREF   refModule;
    LPVOID      rv;
    Module*     mod;

    ReflectClass* pRC = (ReflectClass*)refThis->GetData();
    _ASSERTE(pRC);

    // Get the module,  This may fail because
    //  there are classes which don't have modules (Like arrays)
    mod = pRC->GetModule();
    if (!mod)
        return 0;

    HELPER_METHOD_FRAME_BEGIN_RET_1(refThis);

    // Get the exposed Module Object -- We create only one per Module instance.
    refModule = (OBJECTREF) mod->GetExposedModuleObject();
    _ASSERTE(refModule);

    // Assign the return value
    *((OBJECTREF*) &rv) = refModule;

    HELPER_METHOD_FRAME_END();

    // Return the object
    return rv;
}
FCIMPLEND

// GetAssembly
// This will return the assembly that the class is defined in.
FCIMPL1(Object*, COMClass::GetAssembly, ReflectClassBaseObject* refThisUNSAFE)
{
    OBJECTREF   refAssembly = NULL;
    Module*     mod;
    Assembly*   assem;

    REFLECTCLASSBASEREF refThis = (REFLECTCLASSBASEREF) refThisUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_2(Frame::FRAME_ATTR_RETURNOBJ, refThis, refAssembly);

    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    _ASSERTE(pRC);

    // Get the module,  This may fail because
    //  there are classes which don't have modules (Like arrays)
    mod = pRC->GetModule();
    if (mod)
    {
    // Grab the module's assembly.
    assem = mod->GetAssembly();
    _ASSERTE(assem);

    // Get the exposed Assembly Object.
    refAssembly = assem->GetExposedObject();
    _ASSERTE(refAssembly);
    }

    HELPER_METHOD_FRAME_END();

    // Return the object
    return OBJECTREFToObject(refAssembly);
}
FCIMPLEND

// CreateClassObjFromModule
// This method will create a new Module class given a Module.
HRESULT COMClass::CreateClassObjFromModule(
    Module* pModule,
    REFLECTMODULEBASEREF* prefModule)
{
    //HRESULT  hr   = E_FAIL;

    // This only throws the possible exception raised by the <cinit> on the class
    THROWSCOMPLUSEXCEPTION();

    //if(!m_fIsReflectionInitialized)
    //{
    //  hr = InitializeReflection();
    //  if(FAILED(hr))
    //  {
    //      _ASSERTE(!"InitializeReflection failed in COMClass::SetStandardFilterCriteria.");
    //      return hr;
    //  }
    //}

    // Create the module object
    *prefModule = (REFLECTMODULEBASEREF) g_pRefUtil->CreateReflectClass(RC_Module,0,pModule);
    return S_OK;
}


// CreateClassObjFromDynModule
// This method will create a new ModuleBuilder class given a Module.
HRESULT COMClass::CreateClassObjFromDynamicModule(
    Module* pModule,
    REFLECTMODULEBASEREF* prefModule)
{
    // This only throws the possible exception raised by the <cinit> on the class
    THROWSCOMPLUSEXCEPTION();

    // Create the module object
    *prefModule = (REFLECTMODULEBASEREF) g_pRefUtil->CreateReflectClass(RC_DynamicModule,0,pModule);
    return S_OK;
}


/*=============================GetUnitializedObject=============================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
FCIMPL1(Object*, COMClass::GetUninitializedObject, ReflectClassBaseObject* objTypeUNSAFE)
{
    OBJECTREF           retVal  = NULL;
    REFLECTCLASSBASEREF objType = (REFLECTCLASSBASEREF) objTypeUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, objType);

    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(objType);
    
    if (objType==NULL) {
        COMPlusThrowArgumentNull(L"type", L"ArgumentNull_Type");
    }

    ReflectClass* pRC = (ReflectClass*) objType->GetData();
    _ASSERTE(pRC);
    EEClass *pEEC = pRC->GetClass();
    _ASSERTE(pEEC);
    
    //We don't allow unitialized strings.
    if (pEEC == g_pStringClass->GetClass()) {
        COMPlusThrow(kArgumentException, L"Argument_NoUninitializedStrings");
    }

    retVal = pEEC->GetMethodTable()->Allocate();     
    
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(retVal);
}
FCIMPLEND


FCIMPL2(INT32, COMClass::SupportsInterface, ReflectClassBaseObject* refThisUNSAFE, Object* objUNSAFE)
{
    INT32               iRetVal = 0;
    REFLECTCLASSBASEREF refThis = (REFLECTCLASSBASEREF) refThisUNSAFE;
    OBJECTREF           obj     = (OBJECTREF) objUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_2(refThis, obj);

    THROWSCOMPLUSEXCEPTION();
    if (obj == NULL)
        COMPlusThrow(kNullReferenceException);

    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    EEClass* pEEC = pRC->GetClass();
    _ASSERTE(pEEC);
    MethodTable* pMT = pEEC->GetMethodTable();

    iRetVal = obj->GetClass()->SupportsInterface(obj, pMT);
    
    HELPER_METHOD_FRAME_END();
    return iRetVal;
}
FCIMPLEND


// GetTypeDefToken
// This method returns the typedef token of this EEClass
FCIMPL1(INT32, COMClass::GetTypeDefToken, ReflectClassBaseObject* refThis) 
{
    VALIDATEOBJECTREF(refThis);

    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    _ASSERTE(pRC);

    EEClass* pEEC = pRC->GetClass();
    _ASSERTE(pEEC);
    return pEEC->GetCl();
}
FCIMPLEND

FCIMPL1(INT32, COMClass::IsContextful, ReflectClassBaseObject* refThis) 
{
    VALIDATEOBJECTREF(refThis);

    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    _ASSERTE(pRC);

    EEClass* pEEC = pRC->GetClass();
    BOOL isContextful = FALSE;
    // Some classes do not have an underlying EEClass such as
    // COM classic or pointer classes 
    // We will return false for such classes
    if(NULL != pEEC)
    {
        isContextful = pEEC->IsContextful();
    }

    return isContextful;
}
FCIMPLEND

// This will return TRUE is a type has a non-default proxy attribute
// associated with it.
FCIMPL1(INT32, COMClass::HasProxyAttribute, ReflectClassBaseObject* refThis)
{
    VALIDATEOBJECTREF(refThis);

    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    _ASSERTE(pRC);

    EEClass* pEEC = pRC->GetClass();
    BOOL hasProxyAttribute = FALSE;
    // Some classes do not have an underlying EEClass such as
    // COM classic or pointer classes
    // We will return false for such classes
    if(NULL != pEEC)
    {
        hasProxyAttribute = pEEC->HasRemotingProxyAttribute();
    }

    return hasProxyAttribute;
}
FCIMPLEND

FCIMPL1(INT32, COMClass::IsMarshalByRef, ReflectClassBaseObject* refThis) 
{
    VALIDATEOBJECTREF(refThis);

    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    _ASSERTE(pRC);

    EEClass* pEEC = pRC->GetClass();
    BOOL isMarshalByRef = FALSE;
    // Some classes do not have an underlying EEClass such as
    // COM classic or pointer classes 
    // We will return false for such classes
    if(NULL != pEEC)
    {
        isMarshalByRef = pEEC->IsMarshaledByRef();
    }

    return isMarshalByRef;
}
FCIMPLEND

//
// [MethodImplAttribute(MethodImplOptions.InternalCall)]
// private extern InterfaceMapping InternalGetInterfaceMap(RuntimeType interfaceType);
//
FCIMPL2_INST_RET_VC(InterfaceMapData, data, COMClass::GetInterfaceMap, ReflectClassBaseObject* refThis, ReflectClassBaseObject* type)
{
    THROWSCOMPLUSEXCEPTION();
    VALIDATEOBJECTREF(refThis);
    VALIDATEOBJECTREF(type);

    HELPER_METHOD_FRAME_BEGIN_RET_VC_NOPOLL();

    // Cast to the Type object
    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    if (pRC->IsTypeDesc()) 
        COMPlusThrow(kArgumentException, L"Arg_NotFoundIFace");
    EEClass* pTarget = pRC->GetClass();

    // Cast to the Type object
    ReflectClass* pIRC = (ReflectClass*) type->GetData();
    EEClass* pIface = pIRC->GetClass();

    ZeroMemory(data, sizeof(*data));

    data->m_targetType = REFLECTCLASSBASEREF(refThis);
    data->m_interfaceType = REFLECTCLASSBASEREF(type);

    // data is a return value - we have to protect by ourselves
    GCPROTECT_BEGIN(*data);

    ReflectMethodList* pRM = pRC->GetMethods();
    ReflectMethodList* pIRM = pIRC->GetMethods();   // this causes a GC !!!

    _ASSERTE(pIface->IsInterface());
    if (pTarget->IsInterface())
        COMPlusThrow(kArgumentException, L"Argument_InterfaceMap");

    unsigned slotCnt = pIface->GetMethodTable()->GetInterfaceMethodSlots();
    InterfaceInfo_t* pII = pTarget->FindInterface(pIface->GetMethodTable());
    if (!pII) 
        COMPlusThrow(kArgumentException, L"Arg_NotFoundIFace");

    data->m_targetMethods = PTRARRAYREF(AllocateObjectArray(slotCnt, g_pRefUtil->GetTrueType(RC_Class)));

    data->m_interfaceMethods = PTRARRAYREF(AllocateObjectArray(slotCnt, g_pRefUtil->GetTrueType(RC_Class)));

    for (unsigned i=0;i<slotCnt;i++) {
        // Build the interface array...
        MethodDesc* pCurMethod = pIface->GetUnknownMethodDescForSlot(i);
        ReflectMethod* pRMeth = pIRM->FindMethod(pCurMethod);
        _ASSERTE(pRMeth);

        OBJECTREF o = (OBJECTREF) pRMeth->GetMethodInfo(pIRC);
        data->m_interfaceMethods->SetAt(i, o);

        // Build the type array...
        pCurMethod = pTarget->GetUnknownMethodDescForSlot(i+pII->m_wStartSlot);
        pRMeth = pRM->FindMethod(pCurMethod);
        if (pRMeth) 
            o = (OBJECTREF) pRMeth->GetMethodInfo(pRC);
        else
            o = NULL;
        data->m_targetMethods->SetAt(i, o);
    }

    GCPROTECT_END ();
    HELPER_METHOD_FRAME_END_POLL();
}
FCIMPLEND_RET_VC

// GetNestedType
// This method will search for a nested type based upon the name
FCIMPL3(Object*, COMClass::GetNestedType, ReflectClassBaseObject* pRefThis, StringObject* vStr, INT32 invokeAttr)
{
    THROWSCOMPLUSEXCEPTION();

    Object* rv = 0;
    STRINGREF str(vStr);
    REFLECTCLASSBASEREF refThis(pRefThis);
    HELPER_METHOD_FRAME_BEGIN_RET_2(refThis, str);
    RefSecContext sCtx;

    LPCUTF8          pszNestName;
    LPCUTF8          pszNestNameSpace;
    if (str == 0)
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_String");

    
    // Get the underlying type
    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    _ASSERTE(pRC);

    ReflectTypeList* pTypes = pRC->GetNestedTypes();
    if (pTypes->dwTypes != 0)
    {

        CQuickBytes bytes;
        LPSTR szNestName;
        DWORD cNestName;

        //Get the name and split it apart into namespace, name
        szNestName = GetClassStringVars(str, &bytes, &cNestName);

        ns::SplitInline(szNestName, pszNestNameSpace, pszNestName);

        // The Search modifiers
        bool ignoreCase = ((invokeAttr & BINDER_IgnoreCase)  != 0);
        bool declaredOnly = ((invokeAttr & BINDER_DeclaredOnly)  != 0);

        // The search filters
        bool addPriv = ((invokeAttr & BINDER_NonPublic) != 0);
        bool addPub = ((invokeAttr & BINDER_Public) != 0);

        EEClass* pThisEEC = pRC->GetClass();

        EEClass* retEEC = 0;
        for (DWORD i=0;i<pTypes->dwTypes;i++) {
            LPCUTF8 szcName;
            LPCUTF8 szcNameSpace;
            REFLECTCLASSBASEREF o;
            o = (REFLECTCLASSBASEREF) pTypes->types[i]->GetExposedClassObject();
            ReflectClass* thisRC = (ReflectClass*) o->GetData();
            _ASSERTE(thisRC);

            // Check for access to non-publics
            if (IsTdNestedPublic(pTypes->types[i]->GetAttrClass())) {
                if (!addPub)
                    continue;
            }
            else {
                if (!addPriv)
                    continue;
            }
            if (!InvokeUtil::CheckAccessType(&sCtx, pTypes->types[i], 0)) continue;

            // Are we only looking at the declared nested classes?
            if (declaredOnly) {
                EEClass* pEEC = pTypes->types[i]->GetEnclosingClass();
                if (pEEC != pThisEEC)
                    continue;
            }

            thisRC->GetName(&szcName,&szcNameSpace);
            if(pszNestNameSpace) {
                if (!ignoreCase) {
                    if (strcmp(pszNestNameSpace, szcNameSpace))
                        continue;
                }
                else {
                    if (_stricmp(pszNestNameSpace, szcNameSpace))
                        continue;
                }
            }

            // If the names are a match, break
            if (!ignoreCase) {
                if(strcmp(pszNestName, szcName))
                    continue;
            }
            else {
                if(_stricmp(pszNestName, szcName))
                    continue;
            }
            if (retEEC)
                COMPlusThrow(kAmbiguousMatchException);
            retEEC = pTypes->types[i];
            if (!ignoreCase)
                break;
        }
        if (retEEC)
            rv = OBJECTREFToObject(retEEC->GetExposedClassObject());
    }
    HELPER_METHOD_FRAME_END();
    return rv;
}
FCIMPLEND

// GetNestedTypes
// This method will return an array of types which are the nested types
//  defined by the type.  If no nested types are defined, a zero length
//  array is returned.
FCIMPL2(Object*, COMClass::GetNestedTypes, ReflectClassBaseObject* vRefThis, INT32 invokeAttr)
{
    Object* rv = 0;
    REFLECTCLASSBASEREF refThis(vRefThis);
    PTRARRAYREF nests((PTRARRAYREF)(size_t)NULL);
    HELPER_METHOD_FRAME_BEGIN_RET_2(refThis, nests);    // Set up a frame
    RefSecContext sCtx;

    // Get the underlying type
    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    _ASSERTE(pRC);

    // Allow GC Protection so we can 
    ReflectTypeList* pTypes = pRC->GetNestedTypes();
    nests = (PTRARRAYREF) AllocateObjectArray(pTypes->dwTypes, g_pRefUtil->GetTrueType(RC_Class));

    // The Search modifiers
    bool declaredOnly = ((invokeAttr & BINDER_DeclaredOnly)  != 0);

    // The search filters
    bool addPriv = ((invokeAttr & BINDER_NonPublic) != 0);
    bool addPub = ((invokeAttr & BINDER_Public) != 0);

    EEClass* pThisEEC = pRC->GetClass();
    unsigned int pos = 0;
    for (unsigned int i=0;i<pTypes->dwTypes;i++) {
        if (IsTdNestedPublic(pTypes->types[i]->GetAttrClass())) {
            if (!addPub)
                continue;
        }
        else {
            if (!addPriv)
                continue;
        }
        if (!InvokeUtil::CheckAccessType(&sCtx, pTypes->types[i], 0)) continue;
        if (declaredOnly) {
            EEClass* pEEC = pTypes->types[i]->GetEnclosingClass();
            if (pEEC != pThisEEC)
                continue;
        }
        OBJECTREF o = pTypes->types[i]->GetExposedClassObject();
        nests->SetAt(pos++, o);
    }

    if (pos != pTypes->dwTypes) {
        PTRARRAYREF p = (PTRARRAYREF) AllocateObjectArray(
            pos, g_pRefUtil->GetTrueType(RC_Class));
        for (unsigned int i=0;i<pos;i++)
            p->SetAt(i, nests->GetAt(i));
        nests = p;   
    }

    rv = OBJECTREFToObject(nests);
    HELPER_METHOD_FRAME_END();
    _ASSERTE(rv);
    return rv;
}
FCIMPLEND

FCIMPL2(INT32, COMClass::IsSubClassOf, ReflectClassBaseObject* refThis, ReflectClassBaseObject* refOther);
{
    if (refThis == NULL)
        FCThrow(kNullReferenceException);
    if (refOther == NULL)
        FCThrowArgumentNull(L"c");

    VALIDATEOBJECTREF(refThis);
    VALIDATEOBJECTREF(refOther);

    MethodTable *pType = refThis->GetMethodTable();
    MethodTable *pBaseType = refOther->GetMethodTable();
    if (pType != pBaseType || pType != g_Mscorlib.FetchClass(CLASS__CLASS)) 
        return false;

    ReflectClass *pRCThis = (ReflectClass *)refThis->GetData();
    ReflectClass *pRCOther = (ReflectClass *)refOther->GetData();


    EEClass *pEEThis = pRCThis->GetClass();
    EEClass *pEEOther = pRCOther->GetClass();

    // If these types aren't actually classes, they're not subclasses. 
    if ((!pEEThis) || (!pEEOther))
        return false;

    if (pEEThis == pEEOther)
        // this api explicitly tests for proper subclassness
        return false;

    if (pEEThis == pEEOther)
        return false;

    do 
    {
        if (pEEThis == pEEOther)
            return true;

        pEEThis = pEEThis->GetParentClass();

    } 
    while (pEEThis != NULL);

    return false;
}
FCIMPLEND
