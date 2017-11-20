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
////////////////////////////////////////////////////////////////////////////////

#ifndef _COMModule_H_
#define _COMModule_H_

#include "reflectwrap.h"
#include "comreflectioncommon.h"
#include "invokeutil.h"

class Module;

class COMModule
{
    friend class COMClass;
    
public:

    static unsigned COMModule::GetSigForTypeHandle(TypeHandle typeHnd, PCOR_SIGNATURE sigBuff, unsigned buffLen, IMetaDataEmit* emit, IMDInternalImport *pImport, int baseToken);


    // GetFields
    // Return an array of fields.
    static FCDECL1(Object*, GetFields, ReflectModuleBaseObject* vRefThis);

    // GetFields
    // Return an array of fields.
    static FCDECL3(Object*, GetField, ReflectModuleBaseObject* vRefThis, StringObject* name, INT32 bindingAttr);

    static FCDECL1(INT32, GetSigTypeFromClassWrapper, ReflectClassBaseObject* refType);

    // DefineDynamicModule
    // This method will create a dynamic module given an assembly
    static FCDECL4(Object*, DefineDynamicModule, AssemblyBaseObject* containingAssemblyUNSAFE, BYTE emitSymbolInfo, StringObject* filenameUNSAFE, StackCrawlMark* stackMark);


    // GetClassToken
    // This function will return the class token for the named element.
    static FCDECL5(mdTypeRef, GetClassToken, ReflectModuleBaseObject* refThisUNSAFE, 
                                             StringObject* strFullNameUNSAFE, 
                                             ReflectModuleBaseObject* refedModuleUNSAFE, 
                                             StringObject* strRefedModuleFileNameUNSAFE,
                                             INT32 tkResolution);

    // _LoadInMemoryTypeByNameArgs
    // This function will return the class token for the named element.
    static FCDECL2(Object*, LoadInMemoryTypeByName, ReflectModuleBaseObject* refThisUNSAFE, StringObject* strFullNameUNSAFE);


    // SetFieldRVAContent
    // This function is used to set the FieldRVA with the content data
    static FCDECL4(void, SetFieldRVAContent, ReflectModuleBaseObject* refThisUNSAFE, INT32 tkField, U1Array* contentUNSAFE, INT32 length);
    

    //GetArrayMethodToken
    static FCDECL6(INT32, GetArrayMethodToken, ReflectModuleBaseObject* refThisUNSAFE, 
                                               INT32 tkTypeSpec, 
                                               StringObject* methodNameUNSAFE, 
                                               U1Array* signatureUNSAFE,
                                               INT32 sigLength,
                                               INT32 baseToken);

    // GetMemberRefToken
    // This function will return the MemberRef token 
    static FCDECL4(INT32, GetMemberRefToken, ReflectModuleBaseObject* refThisUNSAFE, ReflectModuleBaseObject* refedModuleUNSAFE, INT32 tr, INT32 token);

    // This function return a MemberRef token given a MethodInfo describing a array method
    static FCDECL3(INT32, GetMemberRefTokenOfMethodInfo, ReflectModuleBaseObject* refThisUNSAFE, INT32 tr, ReflectBaseObject* methodUNSAFE);


    // GetMemberRefTokenOfFieldInfo
    // This function will return a memberRef token given a FieldInfo
    static FCDECL3(mdMemberRef, GetMemberRefTokenOfFieldInfo, ReflectModuleBaseObject* refThisUNSAFE, INT32 tr, ReflectBaseObject* fieldUNSAFE);

    // GetMemberRefTokenFromSignature
    // This function will return the MemberRef token given the signature from managed code
    static FCDECL5(INT32, GetMemberRefTokenFromSignature, ReflectModuleBaseObject* refThisUNSAFE, 
                                                          INT32 tr,
                                                          StringObject* strMemberNameUNSAFE,
                                                          U1Array* signatureUNSAFE,
                                                          INT32 sigLength);

    // GetTypeSpecToken
    static FCDECL3(mdTypeSpec, GetTypeSpecToken, ReflectModuleBaseObject* refThisUNSAFE, ReflectClassBaseObject* arrayClassUNSAFE, INT32 baseToken);

    // GetTypeSpecTokenWithBytes
    static FCDECL3(mdTypeSpec, GetTypeSpecTokenWithBytes, ReflectModuleBaseObject* refThisUNSAFE, U1Array* signatureUNSAFE, INT32 sigLength);

    // GetCaller
    // Returns the module of the calling method. A value can be
    // added to skip uninteresting frames
    static FCDECL1(Object*, GetCaller, StackCrawlMark* stackMark);

    // GetClass
    // Given a class name, this method will look for that class
    //  with in the module.
    static FCDECL5(Object*, GetClass, ReflectModuleBaseObject* refThisUNSAFE, StringObject* refClassNameUNSAFE, BYTE bIgnoreCase, BYTE bThrowOnError, StackCrawlMark* stackMark);

    // Find classes will invoke a filter against all of the
    //  classes defined within the module.  For each class
    //  accepted by the filter, it will be returned to the
    //  caller in an array.
    static HRESULT ClassNameFilter(IMDInternalImport *pInternalImport, mdTypeDef* rgTypeDefs, DWORD* pcTypeDefs,
        LPUTF8 szPrefix, DWORD cPrefix, bool bCaseSensitive);

    // Get class will return an array contain all of the classes
    //  that are defined within this Module.
    static FCDECL2(Object*, GetClasses, ReflectModuleBaseObject* refThisUNSAFE, StackCrawlMark* stackMark);
    
    // GetStringConstant
    // If this is a dynamic module, this routine will define a new 
    //  string constant or return the token of an existing constant.
    static FCDECL2(mdString, GetStringConstant, ReflectModuleBaseObject* refThisUNSAFE, StringObject* strValueUNSAFE);

    
    static FCDECL2(void, SetModuleProps, ReflectModuleBaseObject* refThisUNSAFE, StringObject* strModuleNameUNSAFE);
    

    static FCDECL1(Object*,     GetAssembly,            ReflectModuleBaseObject* refThisUNSAFE);
    static FCDECL1(INT32,       IsResource,             ReflectModuleBaseObject* refThisUNSAFE);
    static FCDECL1(Object*,     GetMethods,             ReflectModuleBaseObject* refThisUNSAFE);
    static FCDECL1(INT32,       IsDynamic,              ReflectModuleBaseObject* refThisUNSAFE);
    static FCDECL1(Object*,     GetName,                ReflectModuleBaseObject* refThisUNSAFE);
    static FCDECL1(Object*,     GetFullyQualifiedName,  ReflectModuleBaseObject* refThisUNSAFE);
    static FCDECL1(HINSTANCE,   GetHINST,               ReflectModuleBaseObject* refThisUNSAFE);


    static FCDECL6(Object*, InternalGetMethod, ReflectModuleBaseObject* refThisUNSAFE,
                                               StringObject* nameUNSAFE,
                                               INT32 invokeAttr,
                                               INT32 callConv,
                                               PTRArray* argTypesUNSAFE,
                                               INT32 argCnt);

//    static Module *ValidateThisRef(REFLECTMODULEBASEREF pThis);

    static HRESULT DefineTypeRefHelper(
        IMetaDataEmit       *pEmit,         // given emit scope
        mdTypeDef           td,             // given typedef in the emit scope
        mdTypeRef           *ptr);          // return typeref

};

#endif
