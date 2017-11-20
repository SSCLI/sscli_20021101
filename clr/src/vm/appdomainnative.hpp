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
** Header:  AppDomainNative.hpp
**
** Purpose: Implements native methods for AppDomains
**
** Date:  May 20, 2000
**
===========================================================*/
#ifndef _APPDOMAINNATIVE_H
#define _APPDOMAINNATIVE_H

class AppDomainNative
{
    //struct CreateBasicDomainArgs
    //{
    //};
    //struct SetupDomainSecurityArgs
    //{
    //    DECLARE_ECALL_OBJECTREF_ARG(APPDOMAINREF, refThis);
    //    DECLARE_ECALL_OBJECTREF_ARG(void*, parentSecurityDescriptor);
    //    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, creatorsEvidence);
    //    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, providedEvidence);
    //    DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, strFriendlyName);
    //};
    //struct UpdateLoaderOptimizationArgs
    //{
    //    DECLARE_ECALL_OBJECTREF_ARG(APPDOMAINREF, refThis);
    //    DECLARE_ECALL_I4_ARG(DWORD, optimization); 
    //};
    //struct ExecuteAssemblyArgs
    //{
    //    DECLARE_ECALL_OBJECTREF_ARG(APPDOMAINREF, refThis);
    //    DECLARE_ECALL_OBJECTREF_ARG(PTRARRAYREF, stringArgs);
    //    DECLARE_ECALL_OBJECTREF_ARG(ASSEMBLYREF, assemblyName);
    //};
    //struct UnloadArgs
    //{
    //    DECLARE_ECALL_OBJECTREF_ARG(THREADBASEREF, requestingThread);
    //    DECLARE_ECALL_OBJECTREF_ARG(INT32, dwId);
    //};
    //struct IsDomainIdValidArgs
    //{
    //    DECLARE_ECALL_OBJECTREF_ARG(INT32, dwId);
    //};
    //struct ForcePolicyResolutionArgs
    //{
    //    DECLARE_ECALL_OBJECTREF_ARG(APPDOMAINREF, refThis);
    //};
    //struct GetIdArgs
    //{
    //    DECLARE_ECALL_OBJECTREF_ARG(APPDOMAINREF, refThis);
    //};
    //struct GetIdForUnloadArgs
    //{
    //    DECLARE_ECALL_OBJECTREF_ARG(APPDOMAINREF, refDomain);
    //};
    //struct NoArgs
    //{
    //    DECLARE_ECALL_OBJECTREF_ARG(APPDOMAINREF, refThis);
    //};
    //struct StringInternArgs {
    //    DECLARE_ECALL_OBJECTREF_ARG(APPDOMAINREF, refThis);
    //    DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, pString);
    //};
    //struct ForceToSharedDomainArgs {
    //    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, pObject);
    //};
    //struct GetGrantSetArgs
    //{
    //    DECLARE_ECALL_OBJECTREF_ARG(APPDOMAINREF,  refThis);
    //    DECLARE_ECALL_PTR_ARG(OBJECTREF*,       ppDenied);
    //    DECLARE_ECALL_PTR_ARG(OBJECTREF*,       ppGranted);
    //};
    //struct TypeArgs
    //{
    //    DECLARE_ECALL_OBJECTREF_ARG(APPDOMAINREF,  refThis);
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF,  type);
    //};
public:
    static AppDomain *ValidateArg(APPDOMAINREF pThis);

    static FCDECL0(Object*, CreateBasicDomain);
    static FCDECL5(void, SetupDomainSecurity, AppDomainBaseObject* refThisUNSAFE, StringObject* strFriendlyNameUNSAFE, Object* providedEvidenceUNSAFE, Object* creatorsEvidence, void* parentSecurityDescriptor);
    static FCDECL1(void*, GetSecurityDescriptor, AppDomainBaseObject* refThisUNSAFE);
    static FCDECL2(void, UpdateLoaderOptimization, AppDomainBaseObject* refThisUNSAFE, DWORD optimization);
    static FCDECL8(Object*, CreateDynamicAssembly, AppDomainBaseObject* refThisUNSAFE, AssemblyNameBaseObject* assemblyNameUNSAFE, Object* identityUNSAFE, StackCrawlMark* stackMark, Object* requiredPsetUNSAFE, Object* optionalPsetUNSAFE, Object* refusedPsetUNSAFE, INT32 access);
    static FCDECL1(Object*, GetFriendlyName, AppDomainBaseObject* refThisUNSAFE);
    static FCDECL1(Object*, GetAssemblies, AppDomainBaseObject* refThisUNSAFE); 
    static FCDECL2(Object*, GetOrInternString, AppDomainBaseObject* refThisUNSAFE, StringObject* pStringUNSAFE);
    static FCDECL3(INT32, ExecuteAssembly, AppDomainBaseObject* refThisUNSAFE, AssemblyBaseObject* assemblyNameUNSAFE, PTRArray* stringArgsUNSAFE);
    static FCDECL2(void, Unload, INT32 dwId, ThreadBaseObject* requestingThreadUNSAFE);
    static FCDECL1(void, ForcePolicyResolution, AppDomainBaseObject* refThisUNSAFE);
    static FCDECL1(Object*, GetDynamicDir, AppDomainBaseObject* refThisUNSAFE);
    static FCDECL1(INT32, GetId, AppDomainBaseObject* refThisUNSAFE);
    static FCDECL1(INT32, GetIdForUnload, AppDomainBaseObject* refDomainUNSAFE);
    static FCDECL1(INT32, IsDomainIdValid, INT32 dwId);
    static FCDECL1(Object*, GetUnloadWorker, AppDomainBaseObject* refThisUNSAFE);
    static FCDECL1(INT32, IsFinalizingForUnload, AppDomainBaseObject* refThisUNSAFE);
    static FCDECL1(void, ForceResolve, AppDomainBaseObject* refThisUNSAFE);
    static FCDECL1(void, ForceToSharedDomain, Object* pObjectUNSAFE);
    static FCDECL2(INT32, IsTypeUnloading, AppDomainBaseObject* refThisUNSAFE, ReflectClassBaseObject* typeUNSAFE);
    static FCDECL3(void, GetGrantSet, AppDomainBaseObject* refThisUNSAFE, OBJECTREF* ppGranted, OBJECTREF* ppDenied);
    static FCDECL0(Object*, GetDefaultDomain);
    static FCDECL1(LPVOID,  GetFusionContext, AppDomainBaseObject* refThis);
    static FCDECL2(Object*, IsStringInterned, AppDomainBaseObject* refThis, StringObject* pString);
    static FCDECL1(INT32,   IsUnloadingForcedFinalize, AppDomainBaseObject* refThis);
    static FCDECL3(void,    UpdateContextProperty, LPVOID fusionContext, StringObject* key, StringObject* value);
};

#endif
