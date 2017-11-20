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
** Header:  AssemblyNative.hpp
**
** Purpose: Implements AssemblyNative (loader domain) architecture
**
** Date:  Oct 26, 1998
**
===========================================================*/
#ifndef _ASSEMBLYNATIVE_H
#define _ASSEMBLYNATIVE_H


class AssemblyNative
{
    friend class COMClass;
    friend class Assembly;
    friend class BaseDomain;

protected:

    static BOOL HaveReflectionPermission(BOOL ThrowOnFalse);
    
public:
    static Object* GetTypeInner(Assembly *pAssem,
                               STRINGREF *refClassName, 
                               BOOL bThrowOnError, 
                               BOOL bIgnoreCase, 
                               BOOL bVerifyAccess,
                               BOOL bPublicOnly);

    //
    // static FCALLs
    //
    static FCDECL0(Object*,         GetEntryAssembly);
    static FCDECL4(Object*,         LoadImage,                  U1Array* PEByteArrayUNSAFE, U1Array* SymByteArrayUNSAFE, Object* securityUNSAFE, StackCrawlMark* stackMark);
    static FCDECL1(Object*,         GetExecutingAssembly,       StackCrawlMark* stackMark);
    static FCDECL2(Object*,         CreateQualifiedName,        StringObject* strAssemblyNameUNSAFE, StringObject* strTypeNameUNSAFE);
    static FCDECL7(Object*,         Load,                       AssemblyNameBaseObject* assemblyNameUNSAFE, 
                                                                StringObject* codeBaseUNSAFE, 
                                                                BYTE fIsStringized, 
                                                                Object* securityUNSAFE, 
                                                                BYTE fThrowOnFileNotFound, 
                                                                AssemblyBaseObject* locationHintUNSAFE,
                                                                StackCrawlMark* stackMark);


    //
    // instance FCALLs
    //
    static FCDECL1(Object*,         GetLocale,                  Object* refThis);
    static FCDECL1(INT32,           GetHashAlgorithm,           Object* refThis);
    static FCDECL1(LPVOID,          GetSimpleName,              Object* refThis);
    static FCDECL1(Object*,         GetPublicKey,               Object* refThis);
    static FCDECL1(INT32,           GetFlags,                   Object* refThis);
    static FCDECL1(Object*,         GetStringizedName,          Object* refThis);
    static FCDECL1(Object*,         GetLocation,                Object* refThis);
    static FCDECL3(Object*,         GetCodeBase,                Object* refThis, BYTE fCopiedName, BYTE fEscape);
    static FCDECL5(BYTE*,           GetResource,                Object* refThis, StringObject* name, UINT64* length, StackCrawlMark* stackMark, bool skipSecurityCheck);
    static FCDECL5(INT32,           GetVersion,                 Object* refThis, INT32* pMajorVersion, INT32* pMinorVersion, INT32*pBuildNumber, INT32* pRevisionNumber);
    static FCDECL5(Object*,         LoadModuleImage,            Object* refThisUNSAFE, StringObject* moduleNameUNSAFE, U1Array* PEByteArrayUNSAFE, U1Array* SymByteArrayUNSAFE, Object* securityUNSAFE);
    static FCDECL2(Object*,         GetType1Args,               Object* refThisUNSAFE, StringObject* nameUNSAFE);
    static FCDECL3(Object*,         GetType2Args,               Object* refThisUNSAFE, StringObject* nameUNSAFE, BYTE bThrowOnError);
    static FCDECL4(Object*,         GetType3Args,               Object* refThisUNSAFE, StringObject* nameUNSAFE, BYTE bThrowOnError, BYTE bIgnoreCase);
    static FCDECL5(Object*,         GetTypeInternal,            Object* refThisUNSAFE, StringObject* nameUNSAFE, BYTE bThrowOnError, BYTE bIgnoreCase, BYTE bPublicOnly);
    static FCDECL5(INT32,           GetManifestResourceInfo,    Object* refThisUNSAFE, StringObject* nameUNSAFE, OBJECTREF* pAssemblyRef, STRINGREF* pFileName, StackCrawlMark* stackMark);
    static FCDECL3(Object*,         GetModules,                 Object* refThisUNSAFE, INT32 fLoadIfNotFound, INT32 fGetResourceModules);
    static FCDECL2(Object*,         GetModule,                  Object* refThisUNSAFE, StringObject* strFileNameUNSAFE);
    static FCDECL1(Object*,         GetExportedTypes,           Object* refThisUNSAFE);
    static FCDECL1(Object*,         GetResourceNames,           Object* refThisUNSAFE);
    static FCDECL1(Object*,         GetReferencedAssemblies,    Object* refThisUNSAFE);
    static FCDECL1(Object*,         GetEntryPoint,              Object* refThisUNSAFE);
    static FCDECL1(void,            ForceResolve,               Object* refThisUNSAFE);
    static FCDECL1(Object*,         GetOnDiskAssemblyModule,    Object* refThisUNSAFE);
    static FCDECL1(Object*,         GetInMemoryAssemblyModule,  Object* refThisUNSAFE);
    static FCDECL1(INT32,           GlobalAssemblyCache,        Object* refThisUNSAFE);
    static FCDECL2(void,            PrepareSavingManifest,      Object* refThisUNSAFE, ReflectModuleBaseObject* moduleUNSAFE);
    static FCDECL2(mdFile,          AddFileList,                Object* refThisUNSAFE, StringObject* strFileNameUNSAFE);
    static FCDECL3(void,            SetHashValue,               Object* refThisUNSAFE, INT32 tkFile, StringObject* strFullFileNameUNSAFE);
    static FCDECL5(mdExportedType,  AddExportedType,            Object* refThisUNSAFE, StringObject* strCOMTypeNameUNSAFE, INT32 ar, INT32 tkTypeDef, INT32 flags);    
    static FCDECL5(void,            AddStandAloneResource,      Object* refThisUNSAFE, StringObject* strNameUNSAFE, StringObject* strFileNameUNSAFE, StringObject* strFullFileNameUNSAFE, INT32 iAttribute);
    static FCDECL4(void,            SavePermissionRequests,     Object* refThisUNSAFE, U1Array* requiredUNSAFE, U1Array* optionalUNSAFE, U1Array* refusedUNSAFE);
    static FCDECL4(void,            SaveManifestToDisk,         Object* refThisUNSAFE, StringObject* strManifestFileNameUNSAFE, INT32 entrypoint, INT32 fileKind);
    static FCDECL3(void,            AddFileToInMemoryFileList,  Object* refThisUNSAFE, StringObject* strModuleFileNameUNSAFE, Object* refModuleUNSAFE);
    static FCDECL3(void,            GetGrantSet,                Object* refThisUNSAFE, OBJECTREF* ppGranted, OBJECTREF* ppDenied);
    static FCDECL1(Object*,         ParseTypeName,              StringObject* typeNameUNSAFE);

    // Parse the type name to find the assembly Name,
    static HRESULT FindAssemblyName(LPUTF8 szFullClassName,
                                    LPUTF8* pszAssemblyName);
};


#endif

