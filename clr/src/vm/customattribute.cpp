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
#include "common.h"

#include "customattribute.h"
#include "invokeutil.h"
#include "commember.h"
#include "sigformat.h"
#include "comstring.h"
#include "method.hpp"
#include "threads.h"
#include "excep.h"
#include "corerror.h"
#include "security.h"
#include "expandsig.h"
#include "classnames.h"
#include "fcall.h"
#include "assemblynative.hpp"


// internal utility functions defined atthe end of this file                                               
TypeHandle GetTypeHandleFromBlob(Assembly *pCtorAssembly,
                                    CorSerializationType objType, 
                                    BYTE **pBlob, 
                                    const BYTE *endBlob,
                                    Module *pModule);
int GetStringSize(BYTE **pBlob, const BYTE *endBlob);
ARG_SLOT GetDataFromBlob(Assembly *pCtorAssembly,
                      CorSerializationType type, 
                      TypeHandle th, 
                      BYTE **pBlob, 
                      const BYTE *endBlob, 
                      Module *pModule, 
                      BOOL *bObjectCreated);
void ReadArray(Assembly *pCtorAssembly,
               CorSerializationType arrayType, 
               int size, 
               TypeHandle th,
               BYTE **pBlob, 
               const BYTE *endBlob, 
               Module *pModule,
               BASEARRAYREF *pArray);
BOOL AccessCheck(Module *pTargetModule, mdToken tkCtor, EEClass *pCtorClass)
{
    bool fResult = true;

    OBJECTREF Throwable = NULL;
    GCPROTECT_BEGIN(Throwable);

    // Security access check. Filter unless the attribute (and ctor)
    // are public or defined in the same assembly as the decorated
    // entity. Assume the attribute isn't decorating itself or its
    // descendents to make the access check simple.
    DWORD dwCtorAttrs;
    if (TypeFromToken(tkCtor) == mdtMemberRef) {
        MethodDesc* ctorMeth = NULL;
        pCtorClass->GetMethodDescFromMemberRef(pTargetModule, tkCtor, &ctorMeth, &Throwable);
        dwCtorAttrs = ctorMeth->GetAttrs();
    } else {
        _ASSERTE(TypeFromToken(tkCtor) == mdtMethodDef);
        dwCtorAttrs = pTargetModule->GetMDImport()->GetMethodDefProps(tkCtor);
    }
    Assembly *pCtorAssembly = pCtorClass->GetAssembly();
    Assembly *pTargetAssembly = pTargetModule->GetAssembly();

    if (pCtorAssembly != pTargetAssembly && !pCtorClass->IsExternallyVisible())
        fResult = false;
    else if (!IsMdPublic(dwCtorAttrs)) {
        if (pCtorAssembly != pTargetAssembly)
            fResult = false;
        else if (!IsMdAssem(dwCtorAttrs) && !IsMdFamORAssem(dwCtorAttrs))
            fResult = false;
    }

    // Additionally, if the custom attribute class comes from an assembly which
    // doesn't allow untrusted callers, we must check for the trust level of
    // decorated entity's assembly.
    if (fResult &&
        pCtorAssembly != pTargetAssembly &&
        !pCtorAssembly->AllowUntrustedCaller())
        fResult = pTargetAssembly->GetSecurityDescriptor()->IsFullyTrusted() != 0;

    GCPROTECT_END();

    return fResult;
}

// custom attributes utility functions
FCIMPL2(INT32, COMCustomAttribute::GetMemberToken, BaseObjectWithCachedData *pMember, INT32 memberType) {
    CANNOTTHROWCOMPLUSEXCEPTION();
    VALIDATEOBJECTREF(pMember);

    switch (memberType) {
    
    case MEMTYPE_Constructor:
    case MEMTYPE_Method:
        {
        ReflectMethod *pMem = (ReflectMethod*)((ReflectBaseObject*)pMember)->GetData();
        return pMem->GetToken();
        }
    
    case MEMTYPE_Event:
        {
        ReflectEvent *pMem = (ReflectEvent*)((ReflectBaseObject*)pMember)->GetData();
        return pMem->GetToken();
        }
    
    case MEMTYPE_Field:
        {
        ReflectField *pMem = (ReflectField*)((ReflectBaseObject*)pMember)->GetData();
        return pMem->GetToken();
        }
    
    case MEMTYPE_Property:
        {
        ReflectProperty *pMem = (ReflectProperty*)((ReflectBaseObject*)pMember)->GetData();
        return pMem->GetToken();
        }
    
    case MEMTYPE_TypeInfo:
    case MEMTYPE_NestedType:
        {
        ReflectClass *pMem = (ReflectClass*)((ReflectClassBaseObject*)pMember)->GetData();
        return pMem->GetToken();
        }
    
    default:
        _ASSERTE(!"what is this?");
    }

    return 0;
}
FCIMPLEND

FCIMPL2(LPVOID, COMCustomAttribute::GetMemberModule, BaseObjectWithCachedData *pMember, INT32 memberType) {
    CANNOTTHROWCOMPLUSEXCEPTION();
    VALIDATEOBJECTREF(pMember);

    switch (memberType) {
    
    case MEMTYPE_Constructor:
    case MEMTYPE_Method:
        {
        ReflectMethod *pMem = (ReflectMethod*)((ReflectBaseObject*)pMember)->GetData();
        return pMem->GetModule();
        }
    
    case MEMTYPE_Event:
        {
        ReflectEvent *pMem = (ReflectEvent*)((ReflectBaseObject*)pMember)->GetData();
        return pMem->GetModule();
        }
    
    case MEMTYPE_Field:
        {
        ReflectField *pMem = (ReflectField*)((ReflectBaseObject*)pMember)->GetData();
        return pMem->GetModule();
        }
    
    case MEMTYPE_Property:
        {
        ReflectProperty *pMem = (ReflectProperty*)((ReflectBaseObject*)pMember)->GetData();
        return pMem->GetModule();
        }
    
    case MEMTYPE_TypeInfo:
    case MEMTYPE_NestedType:
        {
        ReflectClass *pMem = (ReflectClass*)((ReflectClassBaseObject*)pMember)->GetData();
        return pMem->GetModule();
        }
    
    default:
        _ASSERTE(!"Wrong MemberType for CA");
    }

    return NULL;
}
FCIMPLEND

FCIMPL1(INT32, COMCustomAttribute::GetAssemblyToken, AssemblyBaseObject *assembly) {
    CANNOTTHROWCOMPLUSEXCEPTION();
    VALIDATEOBJECTREF(assembly);
    mdAssembly token = 0;
    Assembly *pAssembly = assembly->GetAssembly();
    IMDInternalImport *mdImport = pAssembly->GetManifestImport();
    if (mdImport)
        mdImport->GetAssemblyFromScope(&token);
    return token;
}
FCIMPLEND

FCIMPL1(LPVOID, COMCustomAttribute::GetAssemblyModule, AssemblyBaseObject *assembly) {
    CANNOTTHROWCOMPLUSEXCEPTION();
    VALIDATEOBJECTREF(assembly);
    return (LPVOID)assembly->GetAssembly()->GetSecurityModule();
}
FCIMPLEND

FCIMPL1(INT32, COMCustomAttribute::GetModuleToken, ReflectModuleBaseObject *module) {
    CANNOTTHROWCOMPLUSEXCEPTION();
    VALIDATEOBJECTREF(module);
    mdModule token = 0;
    Module *pModule = (Module*)module->GetData();
    if (!pModule->IsResource())
        pModule->GetImporter()->GetModuleFromScope(&token);
    return token;
}
FCIMPLEND

FCIMPL1(LPVOID, COMCustomAttribute::GetModuleModule, ReflectModuleBaseObject *module) {
    CANNOTTHROWCOMPLUSEXCEPTION();
    VALIDATEOBJECTREF(module);
    return (LPVOID)module->GetData();
}
FCIMPLEND

FCIMPL1(INT32, COMCustomAttribute::GetMethodRetValueToken, BaseObjectWithCachedData *method) {
    ReflectMethod *pRM = (ReflectMethod*)((ReflectBaseObject*)method)->GetData();
    MethodDesc* pMeth = pRM->pMethod;
    mdMethodDef md = pMeth->GetMemberDef();
    IMDInternalImport* pInternalImport = pMeth->GetMDImport();

    // Get an enum on the Parameters.
    HENUMInternal   hEnum;
    HRESULT hr = pInternalImport->EnumInit(mdtParamDef, md, &hEnum);
    if (FAILED(hr)) 
        return 0;
    
    // Findout how many parameters there are.
    ULONG paramCount = pInternalImport->EnumGetCount(&hEnum);
    if (paramCount == 0) {
        pInternalImport->EnumClose(&hEnum);
        return 0;
    }

    // Get the parameter information for the first parameter.
    mdParamDef paramDef;
    pInternalImport->EnumNext(&hEnum, &paramDef);

    // get the Properties for the parameter.  If the sequence is not 0
    //  then we need to return;
    SHORT   seq;
    DWORD   revWord;
    pInternalImport->GetParamDefProps(paramDef,(USHORT*) &seq, &revWord);
    pInternalImport->EnumClose(&hEnum);

    // The parameters are sorted by sequence number.  If we don't get 0 then,
    //  nothing is defined for the return type.
    if (seq != 0) 
        return 0;
    return paramDef;
}
FCIMPLEND


FCIMPL3(INT32, COMCustomAttribute::IsCADefined, ReflectClassBaseObject* caType, LPVOID module, DWORD token)
{
    THROWSCOMPLUSEXCEPTION();

    BOOL isDefined = FALSE;

    HELPER_METHOD_FRAME_BEGIN_RET_0();

    TypeHandle caTH;
    ReflectClass *pClass = (ReflectClass*)caType->GetData();
    if (pClass) 
        caTH = pClass->GetTypeHandle();
    isDefined = COMCustomAttribute::IsDefined((Module*)module, token, caTH, TRUE);

    HELPER_METHOD_FRAME_END();

    return isDefined;
}
FCIMPLEND

INT32 __stdcall COMCustomAttribute::IsDefined(Module *pModule,
                                              mdToken token,
                                              TypeHandle attributeClass,
                                              BOOL checkAccess)
{
    THROWSCOMPLUSEXCEPTION();
    BOOL isDefined = FALSE;

    IMDInternalImport *pInternalImport = pModule->GetMDImport();
    BOOL isSealed = FALSE;

    HRESULT         hr;
    HENUMInternal   hEnum;
    TypeHandle caTH;
    
    // Get the enum first but don't get any values
    hr = pInternalImport->EnumInit(mdtCustomAttribute, token, &hEnum);
    if (SUCCEEDED(hr)) 
    {
        ULONG cMax = pInternalImport->EnumGetCount(&hEnum);
        if (cMax) 
        {
            // we have something to look at

            OBJECTREF Throwable = NULL;
            GCPROTECT_BEGIN(Throwable);

            if (!attributeClass.IsNull()) 
                isSealed = attributeClass.GetClass()->GetAttrClass() & tdSealed;

            // Loop through the Attributes and look for the requested one
            mdCustomAttribute cv;
            while (pInternalImport->EnumNext(&hEnum, &cv)) 
            {
                //
                // fetch the ctor
                mdToken     tkCtor; 
                pInternalImport->GetCustomAttributeProps(cv, &tkCtor);

                mdToken tkType = TypeFromToken(tkCtor);
                if(tkType != mdtMemberRef && tkType != mdtMethodDef) 
                    continue; // we only deal with the ctor case

                //
                // get the info to load the type, so we can check whether the current
                // attribute is a subtype of the requested attribute
                hr = pInternalImport->GetParentToken(tkCtor, &tkType);
                if (FAILED(hr)) 
                {
                    _ASSERTE(!"GetParentToken Failed, bogus metadata");
                    COMPlusThrow(kInvalidProgramException);
                }
                _ASSERTE(TypeFromToken(tkType) == mdtTypeRef || TypeFromToken(tkType) == mdtTypeDef);
                // load the type
                ClassLoader* pLoader = pModule->GetClassLoader();
                NameHandle name(pModule, tkType);
                Throwable = NULL;
                if (isSealed) 
                {
                    if (TypeFromToken(tkType) == mdtTypeDef)
                        name.SetTokenNotToLoad(tdAllTypes);
                    caTH = pLoader->LoadTypeHandle(&name, NULL);
                    if (caTH.IsNull()) 
                        continue;
                }
                else 
                {
                    caTH = pLoader->LoadTypeHandle(&name, &Throwable);
                }
                if (Throwable != NULL)
                    COMPlusThrow(Throwable);
                // a null class implies all custom attribute
                if (!attributeClass.IsNull()) 
                {
                    if (isSealed) 
                    {
                        if (attributeClass != caTH)
                            continue;
                    }
                    else 
                    {
                        if (!caTH.CanCastTo(attributeClass))
                            continue;
                    }
                }

                // Security access check. Filter unless the attribute (and ctor)
                // are public or defined in the same assembly as the decorated
                // entity.
                if (!AccessCheck(pModule, tkCtor, caTH.GetClass()))
                    continue;

                //
                // if we are here we got one
                isDefined = TRUE;
                break;
            }
            GCPROTECT_END();
        }
        
        pInternalImport->EnumClose(&hEnum);
    }
    else 
    {
        _ASSERTE(!"EnumInit Failed");
        FATAL_EE_ERROR();
    }
    
    return isDefined;
}

FCIMPL5(LPVOID, COMCustomAttribute::GetCustomAttributeList, DWORD token, LPVOID module, ReflectClassBaseObject* caTypeUNSAFE, CustomAttributeClass* caItemUNSAFE, INT32 level)
{

    THROWSCOMPLUSEXCEPTION();

    CUSTOMATTRIBUTEREF  caItem      = (CUSTOMATTRIBUTEREF)  caItemUNSAFE;
    REFLECTCLASSBASEREF caType      = (REFLECTCLASSBASEREF) caTypeUNSAFE;
    OBJECTREF           Throwable   = NULL;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_2(Frame::FRAME_ATTR_RETURNOBJ, caItem, caType)
    GCPROTECT_BEGIN(Throwable);

    Module *pModule = (Module*)module;
    IMDInternalImport *pInternalImport = pModule->GetMDImport();
    ReflectClass *pClass = NULL;
    if (caType != NULL)
        pClass = (ReflectClass*)caType->GetData();
    TypeHandle srcTH;
    BOOL isSealed = FALSE;
    // get the new inheritance level
    INT32 inheritLevel = level;

    HRESULT         hr;
    HENUMInternal   hEnum;
    TypeHandle caTH;
    
    // Get the enum first but don't get any values
    hr = pInternalImport->EnumInit(mdtCustomAttribute, token, &hEnum);
    if (SUCCEEDED(hr)) {
        ULONG cMax = pInternalImport->EnumGetCount(&hEnum);
        if (cMax) {
            // we have something to look at

            if (pClass != NULL) {
                isSealed = pClass->GetAttributes() & tdSealed;
                srcTH = pClass->GetTypeHandle();
            }

            // Loop through the Attributes and create the CustomAttributes
            mdCustomAttribute cv;
            while (pInternalImport->EnumNext(&hEnum, &cv)) {
                //
                // fetch the ctor
                mdToken     tkCtor; 
                pInternalImport->GetCustomAttributeProps(cv, &tkCtor);

                mdToken tkType = TypeFromToken(tkCtor);
                if(tkType != mdtMemberRef && tkType != mdtMethodDef) 
                    continue; // we only deal with the ctor case

                //
                // get the info to load the type, so we can check whether the current
                // attribute is a subtype of the requested attribute
                hr = pInternalImport->GetParentToken(tkCtor, &tkType);
                if (FAILED(hr)) {
                    _ASSERTE(!"GetParentToken Failed, bogus metadata");
                    COMPlusThrow(kInvalidProgramException);
                }
                _ASSERTE(TypeFromToken(tkType) == mdtTypeRef || TypeFromToken(tkType) == mdtTypeDef);
                // load the type
                ClassLoader* pLoader = pModule->GetClassLoader();
                Throwable = NULL;
                NameHandle name(pModule, tkType);
                if (isSealed) {
                    if (TypeFromToken(tkType) == mdtTypeDef)
                        name.SetTokenNotToLoad(tdAllTypes);
                    caTH = pLoader->LoadTypeHandle(&name, NULL);
                    if (caTH.IsNull()) 
                        continue;
                }
                else {
                    caTH = pLoader->LoadTypeHandle(&name, &Throwable);
                }
                if (Throwable != NULL)
                    COMPlusThrow(Throwable);
                // a null class implies all custom attribute
                if (pClass) {
                    if (isSealed) {
                        if (srcTH != caTH)
                            continue;
                    }
                    else {
                        if (!caTH.CanCastTo(srcTH))
                            continue;
                    }
                }

                // Security access check. Filter unless the attribute (and ctor)
                // are public or defined in the same assembly as the decorated
                // entity.
                if (!AccessCheck(pModule, tkCtor, caTH.GetClass()))
                    continue;

                //
                // if we are here the attribute is a good match, get the blob
                const void* blobData;
                ULONG blobCnt;
                pInternalImport->GetCustomAttributeAsBlob(cv, &blobData, &blobCnt);

                COMMember::g_pInvokeUtil->CreateCustomAttributeObject(caTH.GetClass(), 
                                                           tkCtor, 
                                                           blobData, 
                                                           blobCnt, 
                                                           pModule, 
                                                           inheritLevel,
                                                           (OBJECTREF*)&caItem);
            }
        }
        
        pInternalImport->EnumClose(&hEnum);
    }
    else {
        _ASSERTE(!"EnumInit Failed");
        FATAL_EE_ERROR();
    }
    
    GCPROTECT_END();
    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(caItem);
}
FCIMPLEND


//
// Create a custom attribute object based on the info in the CustomAttribute (managed) object
//
FCIMPL3(LPVOID, COMCustomAttribute::CreateCAObject, CustomAttributeClass* refThisUNSAFE, INT32* propNum, Object** assembly)
{
    THROWSCOMPLUSEXCEPTION();

    if (refThisUNSAFE == NULL) 
        FCThrow(kNullReferenceException);


    OBJECTREF           ca      = NULL;
    CUSTOMATTRIBUTEREF  refThis = (CUSTOMATTRIBUTEREF)ObjectToOBJECTREF(refThisUNSAFE);
    
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_2(Frame::FRAME_ATTR_RETURNOBJ, refThis, ca);
    
    EEClass *pCAType = ((ReflectClass*)((REFLECTCLASSBASEREF)refThis->GetType())->GetData())->GetClass();
    Module *pModule = refThis->GetModule();

    // get the ctor
    mdToken tkCtor = refThis->GetToken();
    MethodDesc* ctorMeth = NULL;
    OBJECTREF Throwable = NULL;
    GCPROTECT_BEGIN(Throwable);
    pCAType->GetMethodDescFromMemberRef(pModule, tkCtor, &ctorMeth, &Throwable);
    if (Throwable != NULL)
        COMPlusThrow(Throwable);
    else if (ctorMeth == 0 || !ctorMeth->IsCtor())
        COMPlusThrow(kMissingMethodException, L"MissingMethodCustCtor");

    // If the ctor has a security link demand attached, process it now (wrt to
    // the assembly to which the attribute is attached).
    if (ctorMeth->RequiresLinktimeCheck() &&
        !Security::LinktimeCheckMethod(pModule->GetAssembly(), ctorMeth, &Throwable))
        COMPlusThrow(Throwable);

    GCPROTECT_END();

    // Return the exposed assembly object for the managed code's use.
    *assembly = OBJECTREFToObject(pModule->GetAssembly()->GetExposedObject());

    //
    // we got a valid ctor, check the sig and compare with the blob while building the arg list

    // make a sig object we can inspect
    PCCOR_SIGNATURE corSig = ctorMeth->GetSig();
    MetaSig sig = MetaSig(corSig, pCAType->GetModule());

    // get the blob
    BYTE *blob = (BYTE*)refThis->GetBlob();
    BYTE *endBlob = (BYTE*)refThis->GetBlob() + refThis->GetBlobCount();

    // get the number of arguments and allocate an array for the args
    ARG_SLOT *arguments = NULL;
    UINT argsNum = sig.NumFixedArgs() + 1; // make room for the this pointer
    UINT i = 1; // used to flag that we actually get the right number of arg from the blob
    arguments = (ARG_SLOT*)_alloca(argsNum * sizeof(ARG_SLOT));
    memset((void*)arguments, 0, argsNum * sizeof(ARG_SLOT));
    OBJECTREF *argToProtect = (OBJECTREF*)_alloca(argsNum * sizeof(OBJECTREF));
    memset((void*)argToProtect, 0, argsNum * sizeof(OBJECTREF));
    // load the this pointer
    argToProtect[0] = AllocateObject(pCAType->GetMethodTable()); // this is the value to return after the ctor invocation

    if (blob) {
        if (blob < endBlob) {
            INT16 prolog = GET_UNALIGNED_VAL16(blob);
            if (prolog != 1) 
                COMPlusThrow(kCustomAttributeFormatException);
            blob += 2;
        }
        if (argsNum > 1) {
            GCPROTECT_ARRAY_BEGIN(*argToProtect, argsNum);
            // loop through the args
            for (i = 1; i < argsNum; i++) {
                CorElementType type = sig.NextArg();
                if (type == ELEMENT_TYPE_END) 
                    break;
                BOOL bObjectCreated = FALSE;
                TypeHandle th = sig.GetTypeHandle();
                if (th.IsArray())
                    // get the array element 
                    th = th.AsArray()->GetElementTypeHandle();
                ARG_SLOT data = GetDataFromBlob(ctorMeth->GetAssembly(), (CorSerializationType)type, th, &blob, endBlob, pModule, &bObjectCreated);
                if (bObjectCreated) 
                    argToProtect[i] = ArgSlotToObj(data);
                else
                    arguments[i] = data;
            }
            GCPROTECT_END();
            for (i = 1; i < argsNum; i++) {
                if (argToProtect[i] != NULL) {
                    _ASSERTE(arguments[i] == NULL);
                    arguments[i] = ObjToArgSlot(argToProtect[i]);
                }
            }
        }
    }
    arguments[0] = ObjToArgSlot(argToProtect[0]);
    if (i != argsNum)
        COMPlusThrow(kCustomAttributeFormatException);
    
    //
    // the argument array is ready

    // check if there are any named properties to invoke, if so set the by ref int passed in to point 
    // to the blob position where name properties start
    *propNum = 0;
    if (blob && blob != endBlob) {
        if (blob + 2  > endBlob) 
            COMPlusThrow(kCustomAttributeFormatException);
        *propNum = GET_UNALIGNED_VAL16(blob);
        refThis->SetCurrPos((UINT32)(blob + 2 - (BYTE*)refThis->GetBlob()));
        blob += 2;
    }
    if (*propNum == 0 && blob != endBlob) 
        COMPlusThrow(kCustomAttributeFormatException);
    
    // make the invocation to the ctor
    ca = ArgSlotToObj(arguments[0]);
    if (pCAType->IsValueClass()) 
        arguments[0] = PtrToArgSlot(OBJECTREFToObject(ca)->UnBox());
//    GCPROTECT_BEGIN(ca);
    ctorMeth->Call(arguments, &sig);
//    GCPROTECT_END();

    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(ca);
}
FCIMPLEND


FCIMPL5(LPVOID, COMCustomAttribute::GetDataForPropertyOrField, CustomAttributeClass* refThisUNSAFE, BYTE* isProperty, OBJECTREF* value, OBJECTREF* type, BYTE isLast)
{
    THROWSCOMPLUSEXCEPTION();

    STRINGREF name = NULL;

    CUSTOMATTRIBUTEREF  refThis = (CUSTOMATTRIBUTEREF)ObjectToOBJECTREF(refThisUNSAFE);
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refThis);

    if (refThis == NULL)
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    if (isProperty == NULL) 
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_Generic");

    Module *pModule = refThis->GetModule();
    Assembly *pCtorAssembly = refThis->GetType()->GetMethodTable()->GetAssembly();
    BYTE *blob = (BYTE*)refThis->GetBlob() + refThis->GetCurrPos();
    BYTE *endBlob = (BYTE*)refThis->GetBlob() + refThis->GetBlobCount();
    MethodTable *pMTValue = NULL;
    CorSerializationType arrayType = SERIALIZATION_TYPE_BOOLEAN;
    BOOL bObjectCreated = FALSE;
    TypeHandle nullTH;

    if (blob + 2 > endBlob) 
        COMPlusThrow(kCustomAttributeFormatException);
    
    // get whether it is a field or a property
    CorSerializationType propOrField = (CorSerializationType)*blob;
    blob++;
    if (propOrField == SERIALIZATION_TYPE_FIELD) 
        *isProperty = FALSE;
    else if (propOrField == SERIALIZATION_TYPE_PROPERTY) 
        *isProperty = TRUE;
    else 
        COMPlusThrow(kCustomAttributeFormatException);
    
    // get the type of the field
    CorSerializationType fieldType = (CorSerializationType)*blob;
    blob++;
    if (fieldType == SERIALIZATION_TYPE_SZARRAY) {
        arrayType = (CorSerializationType)*blob;
        if (blob + 1 > endBlob) 
            COMPlusThrow(kCustomAttributeFormatException);
        blob++;
    }
    if (fieldType == SERIALIZATION_TYPE_ENUM || arrayType == SERIALIZATION_TYPE_ENUM) {
        // get the enum type
        ReflectClassBaseObject *pEnum = (ReflectClassBaseObject*)OBJECTREFToObject(ArgSlotToObj(GetDataFromBlob(pCtorAssembly,
                                                                                                              SERIALIZATION_TYPE_TYPE, 
                                                                                                              nullTH, 
                                                                                                              &blob, 
                                                                                                              endBlob, 
                                                                                                              pModule, 
                                                                                                              &bObjectCreated)));
        _ASSERTE(bObjectCreated);
        EEClass* pEEEnum = ((ReflectClass*)pEnum->GetData())->GetClass();
        _ASSERTE(pEEEnum->IsEnum());
        pMTValue = pEEEnum->GetMethodTable();
        if (fieldType == SERIALIZATION_TYPE_ENUM) 
            // load the enum type to pass it back
            *type = pEEEnum->GetExposedClassObject();
        else 
            nullTH = TypeHandle(pMTValue);
    }

    //
    // get the string representing the field/property name
    name = ArgSlotToString(GetDataFromBlob(pCtorAssembly,
                                         SERIALIZATION_TYPE_STRING, 
                                         nullTH, 
                                         &blob, 
                                         endBlob, 
                                         pModule, 
                                         &bObjectCreated));
    _ASSERTE(bObjectCreated || name == NULL);

    // create the object and return it
    GCPROTECT_BEGIN(name);
    switch (fieldType) {
    case SERIALIZATION_TYPE_TAGGED_OBJECT:
        *type = g_Mscorlib.GetClass(CLASS__OBJECT)->GetClass()->GetExposedClassObject();
    case SERIALIZATION_TYPE_TYPE:
    case SERIALIZATION_TYPE_STRING:
        *value = ArgSlotToObj(GetDataFromBlob(pCtorAssembly,
                                                  fieldType, 
                                                  nullTH, 
                                                  &blob, 
                                                  endBlob, 
                                                  pModule, 
                                                  &bObjectCreated));
        _ASSERTE(bObjectCreated || *value == NULL);
        if (*value == NULL) {
            // load the proper type so that code in managed knows which property to load
            if (fieldType == SERIALIZATION_TYPE_STRING) 
                *type = g_Mscorlib.GetElementType(ELEMENT_TYPE_STRING)->GetClass()->GetExposedClassObject();
            else if (fieldType == SERIALIZATION_TYPE_TYPE) 
                *type = g_Mscorlib.GetClass(CLASS__TYPE)->GetClass()->GetExposedClassObject();
        }
        break;
    case SERIALIZATION_TYPE_SZARRAY:
    {
        int arraySize = (int)GetDataFromBlob(pCtorAssembly, SERIALIZATION_TYPE_I4, nullTH, &blob, endBlob, pModule, &bObjectCreated);
        if (arraySize != -1) {
            _ASSERTE(!bObjectCreated);
            if (arrayType == SERIALIZATION_TYPE_STRING) 
                nullTH = TypeHandle(g_Mscorlib.GetElementType(ELEMENT_TYPE_STRING));
            else if (arrayType == SERIALIZATION_TYPE_TYPE) 
                nullTH = TypeHandle(g_Mscorlib.GetClass(CLASS__TYPE));
            else if (arrayType == SERIALIZATION_TYPE_TAGGED_OBJECT)
                nullTH = TypeHandle(g_Mscorlib.GetClass(CLASS__OBJECT));
            ReadArray(pCtorAssembly, arrayType, arraySize, nullTH, &blob, endBlob, pModule, (BASEARRAYREF*)value);
        }
        if (*value == NULL) {
            TypeHandle arrayTH;
            switch (arrayType) {
            case SERIALIZATION_TYPE_STRING:
                arrayTH = TypeHandle(g_Mscorlib.GetElementType(ELEMENT_TYPE_STRING));
                break;
            case SERIALIZATION_TYPE_TYPE:
                arrayTH = TypeHandle(g_Mscorlib.GetClass(CLASS__TYPE));
                break;
            case SERIALIZATION_TYPE_TAGGED_OBJECT:
                arrayTH = TypeHandle(g_Mscorlib.GetClass(CLASS__OBJECT));
                break;
            default:
                if (SERIALIZATION_TYPE_BOOLEAN <= arrayType && arrayType <= SERIALIZATION_TYPE_R8) 
                    arrayTH = TypeHandle(g_Mscorlib.GetElementType((CorElementType)arrayType));
            }
            if (!arrayTH.IsNull()) {
                arrayTH = SystemDomain::Loader()->FindArrayForElem(arrayTH, ELEMENT_TYPE_SZARRAY);
                *type = arrayTH.CreateClassObj();
            }
        }
        break;
    }
    default:
        if (SERIALIZATION_TYPE_BOOLEAN <= fieldType && fieldType <= SERIALIZATION_TYPE_R8) 
            pMTValue = g_Mscorlib.GetElementType((CorElementType)fieldType);
        else if(fieldType == SERIALIZATION_TYPE_ENUM)
            fieldType = (CorSerializationType)pMTValue->GetNormCorElementType();
        else
            COMPlusThrow(kCustomAttributeFormatException);
        ARG_SLOT val = GetDataFromBlob(pCtorAssembly, fieldType, nullTH, &blob, endBlob, pModule, &bObjectCreated);
        _ASSERTE(!bObjectCreated);
        *value = pMTValue->Box((void*)ArgSlotEndianessFixup(&val, pMTValue->GetClass()->GetNumInstanceFieldBytes()));
    }
    GCPROTECT_END();

    refThis->SetCurrPos((UINT32)(blob - (BYTE*)refThis->GetBlob()));
    
    if (isLast && blob != endBlob) 
        COMPlusThrow(kCustomAttributeFormatException);

    HELPER_METHOD_FRAME_END();

    FC_GC_POLL_AND_RETURN_OBJREF(OBJECTREFToObject(name));
}
FCIMPLEND
    
// utility functions
TypeHandle GetTypeHandleFromBlob(Assembly *pCtorAssembly,
                                    CorSerializationType objType, 
                                    BYTE **pBlob, 
                                    const BYTE *endBlob,
                                    Module *pModule)
{
    THROWSCOMPLUSEXCEPTION();
    // we must box which means we must get the method table, switch again on the element type
    MethodTable *pMTType = NULL;
    TypeHandle nullTH;
    TypeHandle RtnTypeHnd;

    switch (objType) {
    case SERIALIZATION_TYPE_BOOLEAN:
    case SERIALIZATION_TYPE_I1:
    case SERIALIZATION_TYPE_U1:
    case SERIALIZATION_TYPE_CHAR:
    case SERIALIZATION_TYPE_I2:
    case SERIALIZATION_TYPE_U2:
    case SERIALIZATION_TYPE_I4:
    case SERIALIZATION_TYPE_U4:
    case SERIALIZATION_TYPE_R4:
    case SERIALIZATION_TYPE_I8:
    case SERIALIZATION_TYPE_U8:
    case SERIALIZATION_TYPE_R8:
    case SERIALIZATION_TYPE_STRING:
        pMTType = g_Mscorlib.GetElementType((CorElementType)objType);
        RtnTypeHnd = TypeHandle(pMTType);
        break;

    case ELEMENT_TYPE_CLASS:
        pMTType = g_Mscorlib.GetClass(CLASS__TYPE);
        RtnTypeHnd = TypeHandle(pMTType);
        break;

    case SERIALIZATION_TYPE_TAGGED_OBJECT:
        pMTType = g_Mscorlib.GetClass(CLASS__OBJECT);
        RtnTypeHnd = TypeHandle(pMTType);
        break;

    case SERIALIZATION_TYPE_TYPE:
    {
        int size = GetStringSize(pBlob, endBlob);
        if (size == -1) 
            return nullTH;
        if (size > 0) {
            if (*pBlob + size > endBlob) 
                COMPlusThrow(kCustomAttributeFormatException);
            LPUTF8 szName = (LPUTF8)_alloca(size + 1);
            memcpy(szName, *pBlob, size);
            *pBlob += size;
            szName[size] = 0;
            NameHandle name(szName);
            OBJECTREF Throwable = NULL;
            TypeHandle typeHnd;
            GCPROTECT_BEGIN(Throwable);

            typeHnd = pModule->GetAssembly()->FindNestedTypeHandle(&name, &Throwable);
            if (typeHnd.IsNull()) {
                Throwable = NULL;
                typeHnd = SystemDomain::GetCurrentDomain()->FindAssemblyQualifiedTypeHandle(szName, FALSE, NULL, NULL, &Throwable);
                if (Throwable != NULL) 
                    COMPlusThrow(Throwable);
            }
            GCPROTECT_END();
            if (typeHnd.IsNull()) 
                COMPlusThrow(kCustomAttributeFormatException);
            RtnTypeHnd = typeHnd;

            // Security access check. Custom attribute assembly must follow the
            // usual rules for type access (type must be public, defined in the
            // same assembly or the accessing assembly must have the requisite
            // reflection permission).
            if (!IsTdPublic(typeHnd.GetClass()->GetProtection()) &&
                pModule->GetAssembly() != pCtorAssembly &&
                !pModule->GetAssembly()->GetSecurityDescriptor()->CanRetrieveTypeInformation())
                RtnTypeHnd = nullTH;
        }
        else 
            COMPlusThrow(kCustomAttributeFormatException);
        break;
    }

    case SERIALIZATION_TYPE_ENUM:
    {
        //
        // get the enum type
        BOOL isObject = FALSE;
        ReflectClassBaseObject *pType = (ReflectClassBaseObject*)OBJECTREFToObject(ArgSlotToObj(GetDataFromBlob(pCtorAssembly,
                                                                                                              SERIALIZATION_TYPE_TYPE, 
                                                                                                              nullTH, 
                                                                                                              pBlob, 
                                                                                                              endBlob, 
                                                                                                              pModule, 
                                                                                                              &isObject)));
        _ASSERTE(isObject);
        EEClass* pEEType = ((ReflectClass*)pType->GetData())->GetClass();
        _ASSERTE((objType == SERIALIZATION_TYPE_ENUM) ? pEEType->IsEnum() : TRUE);
        RtnTypeHnd = TypeHandle(pEEType->GetMethodTable());
        break;
    }

    default:
        COMPlusThrow(kCustomAttributeFormatException);
    }

    return RtnTypeHnd;
}

// retrieve the string size in a CA blob. Advance the blob pointer to point to
// the beginning of the string immediately following the size
int GetStringSize(BYTE **pBlob, const BYTE *endBlob)
{
    THROWSCOMPLUSEXCEPTION();
    int size = -1;

    // Null string - encoded as a single byte
    if (**pBlob != 0xFF) {
        if ((**pBlob & 0x80) == 0) 
            // encoded as a single byte
            size = **pBlob;
        else if ((**pBlob & 0xC0) == 0x80) {
            if (*pBlob + 1 > endBlob) 
                COMPlusThrow(kCustomAttributeFormatException);
            // encoded in two bytes
            size = (**pBlob & 0x3F) << 8;
            size |= *(++*pBlob); // This is in big-endian format
        }
        else {
            if (*pBlob + 3 > endBlob)
                COMPlusThrow(kCustomAttributeFormatException);
            // encoded in four bytes
            size = (**pBlob & ~0xC0) << 24;
            size |= *(++*pBlob) << 16;
            size |= *(++*pBlob) << 8;
            size |= *(++*pBlob);
        }
    }

    if (*pBlob + 1 > endBlob) 
        COMPlusThrow(kCustomAttributeFormatException);
    *pBlob += 1;

    return size;
}

// read the whole array as a chunk
void ReadArray(Assembly *pCtorAssembly,
               CorSerializationType arrayType, 
               int size, 
               TypeHandle th,
               BYTE **pBlob, 
               const BYTE *endBlob, 
               Module *pModule,
               BASEARRAYREF *pArray)
{    
    THROWSCOMPLUSEXCEPTION();    
    
    BYTE *pData = NULL;
    ARG_SLOT element = 0;

    switch (arrayType) {
    case SERIALIZATION_TYPE_BOOLEAN:
    case SERIALIZATION_TYPE_I1:
    case SERIALIZATION_TYPE_U1:
        *pArray = (BASEARRAYREF)AllocatePrimitiveArray((CorElementType)arrayType, size);
        pData = (*pArray)->GetDataPtr();
        if (*pBlob + size > endBlob) 
            goto badBlob;
        memcpyNoGCRefs(pData, *pBlob, size);
        *pBlob += size;
        break;

    case SERIALIZATION_TYPE_CHAR:
    case SERIALIZATION_TYPE_I2:
    case SERIALIZATION_TYPE_U2:
        *pArray = (BASEARRAYREF)AllocatePrimitiveArray((CorElementType)arrayType, size);
        pData = (*pArray)->GetDataPtr();
        if (*pBlob + (size * 2) > endBlob) 
            goto badBlob;
        memcpyNoGCRefs(pData, *pBlob, size * 2);
        *pBlob += size * 2;
        break;

    case SERIALIZATION_TYPE_I4:
    case SERIALIZATION_TYPE_U4:
    case SERIALIZATION_TYPE_R4:
        *pArray = (BASEARRAYREF)AllocatePrimitiveArray((CorElementType)arrayType, size);
        pData = (*pArray)->GetDataPtr();
        if (*pBlob + (size * 4) > endBlob) 
            goto badBlob;
        memcpyNoGCRefs(pData, *pBlob, size * 4);
        *pBlob += size * 4;
        break;

    case SERIALIZATION_TYPE_I8:
    case SERIALIZATION_TYPE_U8:
    case SERIALIZATION_TYPE_R8:
        *pArray = (BASEARRAYREF)AllocatePrimitiveArray((CorElementType)arrayType, size);
        pData = (*pArray)->GetDataPtr();
        if (*pBlob + (size * 8) > endBlob) 
            goto badBlob;
        memcpyNoGCRefs(pData, *pBlob, size * 8);
        *pBlob += size * 8;
        break;

    case ELEMENT_TYPE_CLASS:
    case SERIALIZATION_TYPE_TYPE:
    case SERIALIZATION_TYPE_STRING:
    case SERIALIZATION_TYPE_SZARRAY:
    case SERIALIZATION_TYPE_TAGGED_OBJECT:
    {
        BOOL isObject;
        *pArray = (BASEARRAYREF)AllocateObjectArray(size, th);
        if (arrayType == SERIALIZATION_TYPE_SZARRAY) 
            // switch the th to be the proper one 
            th = th.AsArray()->GetElementTypeHandle();
        for (int i = 0; i < size; i++) {
            element = GetDataFromBlob(pCtorAssembly, arrayType, th, pBlob, endBlob, pModule, &isObject);
            _ASSERTE(isObject || element == NULL);
            ((PTRARRAYREF)(*pArray))->SetAt(i, ArgSlotToObj(element));
        }
        break;
    }

    case SERIALIZATION_TYPE_ENUM:
    {
        INT32 bounds = size;
        unsigned elementSize = th.GetSize();
        ClassLoader *cl = th.AsMethodTable()->GetAssembly()->GetLoader();
        TypeHandle arrayHandle = cl->FindArrayForElem(th, ELEMENT_TYPE_SZARRAY);
        if (arrayHandle.IsNull()) 
            goto badBlob;
        *pArray = (BASEARRAYREF)AllocateArrayEx(arrayHandle, &bounds, 1);
        pData = (*pArray)->GetDataPtr();
        size *= elementSize;
        if (*pBlob + size > endBlob) 
            goto badBlob;
        memcpyNoGCRefs(pData, *pBlob, size);
        *pBlob += size;
        break;
    }

    default:
    badBlob:
        COMPlusThrow(kCustomAttributeFormatException);
    }

}

// get data out of the blob according to a CorElementType
ARG_SLOT GetDataFromBlob(Assembly *pCtorAssembly,
                      CorSerializationType type, 
                      TypeHandle th, 
                      BYTE **pBlob, 
                      const BYTE *endBlob, 
                      Module *pModule, 
                      BOOL *bObjectCreated)
{
    THROWSCOMPLUSEXCEPTION();
    ARG_SLOT retValue = 0;
    *bObjectCreated = FALSE;
    TypeHandle nullTH;
    TypeHandle typeHnd;

    switch (type) {

    case SERIALIZATION_TYPE_BOOLEAN:
    case SERIALIZATION_TYPE_I1:
    case SERIALIZATION_TYPE_U1:
        if (*pBlob + 1 <= endBlob) {
            retValue = (ARG_SLOT)**pBlob;
            *pBlob += 1;
            break;
        }
        goto badBlob;

    case SERIALIZATION_TYPE_CHAR:
    case SERIALIZATION_TYPE_I2:
    case SERIALIZATION_TYPE_U2:
        if (*pBlob + 2 <= endBlob) {
            retValue = (ARG_SLOT)GET_UNALIGNED_VAL16(*pBlob);
            *pBlob += 2;
            break;
        }
        goto badBlob;

    case SERIALIZATION_TYPE_I4:
    case SERIALIZATION_TYPE_U4:
    case SERIALIZATION_TYPE_R4:
        if (*pBlob + 4 <= endBlob) {
            retValue = (ARG_SLOT)GET_UNALIGNED_VAL32(*pBlob);
            *pBlob += 4;
            break;
        }
        goto badBlob;

    case SERIALIZATION_TYPE_I8:
    case SERIALIZATION_TYPE_U8:
    case SERIALIZATION_TYPE_R8:
        if (*pBlob + 8 <= endBlob) {
            retValue = (ARG_SLOT)GET_UNALIGNED_VAL64(*pBlob);
            *pBlob += 8;
            break;
        }
        goto badBlob;

    case SERIALIZATION_TYPE_STRING:
    stringType:
    {
        int size = GetStringSize(pBlob, endBlob);
        *bObjectCreated = TRUE;
        if (size > 0) {
            if (*pBlob + size > endBlob) 
                goto badBlob;
            retValue = ObjToArgSlot(COMString::NewString((LPCUTF8)*pBlob, size));
            *pBlob += size;
        }
        else if (size == 0) 
            retValue = ObjToArgSlot(COMString::NewString(0));
        else
            *bObjectCreated = FALSE;

        break;
    }

    // this is coming back from sig but it's not a serialization type, 
    // essentialy the type in the blob and the type in the sig don't match
    case ELEMENT_TYPE_VALUETYPE:
    {
        if (!th.IsEnum()) 
            goto badBlob;
        CorSerializationType enumType = (CorSerializationType)th.GetNormCorElementType();
        BOOL cannotBeObject = FALSE;
        retValue = GetDataFromBlob(pCtorAssembly, enumType, nullTH, pBlob, endBlob, pModule, &cannotBeObject);
        _ASSERTE(!cannotBeObject);
        break;
    }

    // this is coming back from sig but it's not a serialization type, 
    // essentialy the type in the blob and the type in the sig don't match
    case ELEMENT_TYPE_CLASS:
        if (th.IsArray())
            goto typeArray;
        else {
            MethodTable *pMT = th.AsMethodTable();
            if (pMT == g_Mscorlib.GetClass(CLASS__STRING)) 
                goto stringType;
            else if (pMT == g_Mscorlib.GetClass(CLASS__OBJECT)) 
                goto typeObject;
            else if (pMT == g_Mscorlib.GetClass(CLASS__TYPE)) 
                goto typeType;
        }

        goto badBlob;

    case SERIALIZATION_TYPE_TYPE:
    typeType:
    {
        typeHnd = GetTypeHandleFromBlob(pCtorAssembly, SERIALIZATION_TYPE_TYPE, pBlob, endBlob, pModule);
        if (!typeHnd.IsNull())
            retValue = ObjToArgSlot(typeHnd.CreateClassObj());
        *bObjectCreated = TRUE;
        break;
    }

    // this is coming back from sig but it's not a serialization type, 
    // essentialy the type in the blob and the type in the sig don't match
    case ELEMENT_TYPE_OBJECT:
    case SERIALIZATION_TYPE_TAGGED_OBJECT:
    typeObject:
    {
        // get the byte representing the real type and call GetDataFromBlob again
        if (*pBlob + 1 > endBlob) 
            goto badBlob;
        CorSerializationType objType = (CorSerializationType)**pBlob;
        *pBlob += 1;
        switch (objType) {
        case SERIALIZATION_TYPE_SZARRAY:
        {
            if (*pBlob + 1 > endBlob) 
                goto badBlob;
            CorSerializationType arrayType = (CorSerializationType)**pBlob;
            *pBlob += 1;
            if (arrayType == SERIALIZATION_TYPE_TYPE) 
                arrayType = (CorSerializationType)ELEMENT_TYPE_CLASS;
            // grab the array type and make a type handle for it
            nullTH = GetTypeHandleFromBlob(pCtorAssembly, arrayType, pBlob, endBlob, pModule);
        }
        case SERIALIZATION_TYPE_TYPE:
        case SERIALIZATION_TYPE_STRING:
            // notice that the nullTH is actually not null in the array case (see case above)
            retValue = GetDataFromBlob(pCtorAssembly, objType, nullTH, pBlob, endBlob, pModule, bObjectCreated);
            _ASSERTE(*bObjectCreated || retValue == 0);
            break;
        case SERIALIZATION_TYPE_ENUM:
        {
            //
            // get the enum type
            typeHnd = GetTypeHandleFromBlob(pCtorAssembly, SERIALIZATION_TYPE_ENUM, pBlob, endBlob, pModule);
            _ASSERTE(typeHnd.IsTypeDesc() == false);
            
            // ok we have the class, now we go and read the data
            CorSerializationType objType = (CorSerializationType)typeHnd.AsMethodTable()->GetNormCorElementType();
            BOOL isObject = FALSE;
            retValue = GetDataFromBlob(pCtorAssembly, objType, nullTH, pBlob, endBlob, pModule, &isObject);
            _ASSERTE(!isObject);
            retValue= ObjToArgSlot(typeHnd.AsMethodTable()->Box((void*)&retValue));
            *bObjectCreated = TRUE;
            break;
        }
        default:
        {
            // the common primitive type case. We need to box the primitive
            typeHnd = GetTypeHandleFromBlob(pCtorAssembly, objType, pBlob, endBlob, pModule);
            _ASSERTE(typeHnd.IsTypeDesc() == false);
            retValue = GetDataFromBlob(pCtorAssembly, objType, nullTH, pBlob, endBlob, pModule, bObjectCreated);
            _ASSERTE(!*bObjectCreated);
            retValue= ObjToArgSlot(typeHnd.AsMethodTable()->Box((void*)&retValue));
            *bObjectCreated = TRUE;
            break;
        }
        }
        break;
    }

    case SERIALIZATION_TYPE_SZARRAY:
    typeArray:
    {
        // read size
        BOOL isObject = FALSE;
        int size = (int)GetDataFromBlob(pCtorAssembly, SERIALIZATION_TYPE_I4, nullTH, pBlob, endBlob, pModule, &isObject);
        _ASSERTE(!isObject);
        
        if (size != -1) {
            CorSerializationType arrayType;
            if (th.IsEnum()) 
                arrayType = SERIALIZATION_TYPE_ENUM;
            else
                arrayType = (CorSerializationType)th.GetNormCorElementType();
        
            BASEARRAYREF array = NULL;
            GCPROTECT_BEGIN(array);
            ReadArray(pCtorAssembly, arrayType, size, th, pBlob, endBlob, pModule, &array);
            retValue = ObjToArgSlot(array);
            GCPROTECT_END();
        }
        *bObjectCreated = TRUE;
        break;
    }

    default:
    badBlob:
        COMPlusThrow(kCustomAttributeFormatException);
    }

    return retValue;
}


