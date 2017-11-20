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
#include "appdomain.hpp"
#include "appdomainnative.hpp"
#include "remoting.h"
#include "comstring.h"
#include "security.h"
#include "eeconfig.h"
#include "comsystem.h"
#include "appdomainhelper.h"

//************************************************************************
inline AppDomain *AppDomainNative::ValidateArg(APPDOMAINREF pThis)
{
    THROWSCOMPLUSEXCEPTION();
    if (pThis == NULL)
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    // Should not get here with a Transparent proxy for the this pointer -
    // should have always called through onto the real object
    _ASSERTE(! CRemotingServices::IsTransparentProxy(OBJECTREFToObject(pThis)));

    AppDomain* pDomain = (AppDomain*)pThis->GetDomain();

    if(!pDomain)
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    // should not get here with an invalid appdomain. Once unload it, we won't let anyone else
    // in and any threads that are already in will be unwound.
    _ASSERTE(SystemDomain::GetAppDomainAtIndex(pDomain->GetIndex()) != NULL);
    return pDomain;
}

//************************************************************************
FCIMPL0(Object*, AppDomainNative::CreateBasicDomain)
{
    OBJECTREF refRetVal = NULL;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refRetVal);

    THROWSCOMPLUSEXCEPTION();
    CHECKGC();
    // Create the domain adding the appropriate arguments

    AppDomain *pDomain = NULL;

    HRESULT hr = SystemDomain::NewDomain(&pDomain);
    if (FAILED(hr)) 
        COMPlusThrowHR(hr);

#ifdef DEBUGGING_SUPPORTED    
    // Notify the debugger here, before the thread transitions into the 
    // AD to finish the setup.  If we don't, stepping won't work right                              
    SystemDomain::PublishAppDomainAndInformDebugger(pDomain);
#endif // DEBUGGING_SUPPORTED

    refRetVal = pDomain->GetAppDomainProxy();

    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refRetVal);
}
FCIMPLEND

FCIMPL5(void, AppDomainNative::SetupDomainSecurity, AppDomainBaseObject* refThisUNSAFE, StringObject* strFriendlyNameUNSAFE, Object* providedEvidenceUNSAFE, Object* creatorsEvidenceUNSAFE, void* parentSecurityDescriptor)
{
    struct _gc
    {
        APPDOMAINREF    refThis;
        STRINGREF       strFriendlyName;
        OBJECTREF       providedEvidence;
        OBJECTREF       creatorsEvidence;
    } gc;

    gc.refThis          = (APPDOMAINREF) refThisUNSAFE;
    gc.strFriendlyName  = (STRINGREF)    strFriendlyNameUNSAFE;
    gc.providedEvidence = (OBJECTREF)    providedEvidenceUNSAFE;
    gc.creatorsEvidence = (OBJECTREF)    creatorsEvidenceUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_NOPOLL()
    GCPROTECT_BEGIN(gc);
    HELPER_METHOD_POLL();

    THROWSCOMPLUSEXCEPTION();
    CHECKGC();
    // Load the class from this module (fail if it is in a different one).
    AppDomain* pDomain = ValidateArg(gc.refThis);
    
    // Set up Security
    ApplicationSecurityDescriptor *pCreatorSecDesc = (ApplicationSecurityDescriptor*)parentSecurityDescriptor;
    
    // If the AppDomain that created this one is a default appdomain and
    // no evidence is provided, then this new AppDomain is also a default appdomain.
    // If there is no provided evidence but the creator is not a default appdomain,
    // then this new appdomain just gets the same evidence as the creator.
    // If evidence is provided, the new appdomain is not a default appdomain and
    // we simply use the provided evidence.
    
    BOOL resolveRequired = FALSE;
    OBJECTREF orEvidence = NULL;
    GCPROTECT_BEGIN(orEvidence);
    if (gc.providedEvidence == NULL) {
        if (pCreatorSecDesc->GetProperties(CORSEC_DEFAULT_APPDOMAIN) != 0)
            pDomain->GetSecurityDescriptor()->SetDefaultAppDomainProperty();
        orEvidence = gc.creatorsEvidence;
    }
    else {
        if (Security::IsExecutionPermissionCheckEnabled())
            resolveRequired = TRUE;
        orEvidence = gc.providedEvidence;
    }
    
    pDomain->GetSecurityDescriptor()->SetEvidence( orEvidence );
    GCPROTECT_END();

    // If the user created this domain, need to know this so the debugger doesn't
    // go and reset the friendly name that was provided.
    pDomain->SetIsUserCreatedDomain();
    
    WCHAR* pFriendlyName = NULL;
    Thread *pThread = GetThread();

    void* checkPointMarker = pThread->m_MarshalAlloc.GetCheckpoint();
    if (gc.strFriendlyName != NULL) {
        WCHAR* pString = NULL;
        int    iString;
        RefInterpretGetStringValuesDangerousForGC(gc.strFriendlyName, &pString, &iString);
        pFriendlyName = (WCHAR*) pThread->m_MarshalAlloc.Alloc((++iString) * sizeof(WCHAR));

        // Check for a valid string allocation
        if (pFriendlyName == (WCHAR*)-1)
           pFriendlyName = NULL;
        else
           memcpy(pFriendlyName, pString, iString*sizeof(WCHAR));
    }
    
    if (resolveRequired)
        pDomain->GetSecurityDescriptor()->Resolve();

    // once domain is loaded it is publically available so if you have anything 
    // that a list interrogator might need access to if it gets a hold of the
    // appdomain, then do it above the LoadDomain.
    HRESULT hr = SystemDomain::LoadDomain(pDomain, pFriendlyName);

#ifdef PROFILING_SUPPORTED
    // Need the first assembly loaded in to get any data on an app domain.
    if (CORProfilerTrackAppDomainLoads())
        g_profControlBlock.pProfInterface->AppDomainCreationFinished((ThreadID) GetThread(), (AppDomainID) pDomain, hr);
#endif // PROFILING_SUPPORTED

    // We held on to a reference until we were added to the list (see CreateBasicDomain)
    // Once in the list we can safely release this reference.
    pDomain->Release();

    pThread->m_MarshalAlloc.Collapse(checkPointMarker);
    
    if (FAILED(hr)) {
        pDomain->Release();
        if (hr == E_FAIL)
            hr = MSEE_E_CANNOTCREATEAPPDOMAIN;
        COMPlusThrowHR(hr);
    }

#ifdef _DEBUG
    LOG((LF_APPDOMAIN, LL_INFO100, "AppDomainNative::CreateDomain domain [%d] %p %S\n", pDomain->GetIndex(), pDomain, pDomain->GetFriendlyName()));
#endif

    GCPROTECT_END();
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

FCIMPL1(LPVOID, AppDomainNative::GetFusionContext, AppDomainBaseObject* refThis)
{
    THROWSCOMPLUSEXCEPTION();
    CHECKGC();
    LPVOID rv = NULL;
    HRESULT hr;

    AppDomain* pApp = ValidateArg((APPDOMAINREF)refThis);

    IApplicationContext* pContext;
    if(SUCCEEDED(hr = pApp->CreateFusionContext(&pContext)))
    {
        rv = pContext;
    }
    else
    {
        COMPlusThrowHR(hr);
    }

    return rv;
}
FCIMPLEND

FCIMPL1(void*, AppDomainNative::GetSecurityDescriptor, AppDomainBaseObject* refThisUNSAFE)
{
    void*        pvRetVal = NULL;    
    APPDOMAINREF refThis = (APPDOMAINREF) refThisUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_1(refThis);

    THROWSCOMPLUSEXCEPTION();
    CHECKGC();
    pvRetVal = ValidateArg(refThis)->GetSecurityDescriptor();

    HELPER_METHOD_FRAME_END();
    return pvRetVal;
}
FCIMPLEND


FCIMPL2(void, AppDomainNative::UpdateLoaderOptimization, AppDomainBaseObject* refThisUNSAFE, DWORD optimization)
{
    APPDOMAINREF refThis = (APPDOMAINREF) refThisUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_1(refThis);

    THROWSCOMPLUSEXCEPTION();
    CHECKGC();

    ValidateArg(refThis)->SetSharePolicy((BaseDomain::SharePolicy) optimization);

    HELPER_METHOD_FRAME_END();
}
FCIMPLEND


FCIMPL3(void, AppDomainNative::UpdateContextProperty, LPVOID fusionContext, StringObject* keyUNSAFE, StringObject* valueUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    struct _gc
    {
        STRINGREF key;
        STRINGREF value;
    } gc;

    gc.key   = ObjectToSTRINGREF(keyUNSAFE);
    gc.value = ObjectToSTRINGREF(valueUNSAFE);


    if(gc.key != NULL) 
    {
        HELPER_METHOD_FRAME_BEGIN_NOPOLL();
        GCPROTECT_BEGIN(gc);
        HELPER_METHOD_POLL();

        IApplicationContext* pContext = (IApplicationContext*) fusionContext;
        CQuickBytes qb;

        DWORD lgth = gc.key->GetStringLength();
        LPWSTR key = (LPWSTR) qb.Alloc((lgth+1)*sizeof(WCHAR));
        memcpy(key, gc.key->GetBuffer(), lgth*sizeof(WCHAR));
        key[lgth] = L'\0';
        
        AppDomain::SetContextProperty(pContext, key, (OBJECTREF*)&gc.value);

        GCPROTECT_END();
        HELPER_METHOD_FRAME_END();
    }
}
FCIMPLEND

FCIMPL3(INT32, AppDomainNative::ExecuteAssembly, AppDomainBaseObject* refThisUNSAFE, AssemblyBaseObject* assemblyNameUNSAFE, PTRArray* stringArgsUNSAFE)
{
    INT32 iRetVal = 0;

    struct _gc
{
        APPDOMAINREF    refThis;
        ASSEMBLYREF     assemblyName;
        PTRARRAYREF     stringArgs;
    } gc;

    gc.refThis      = (APPDOMAINREF) refThisUNSAFE;
    gc.assemblyName = (ASSEMBLYREF)  assemblyNameUNSAFE;
    gc.stringArgs   = (PTRARRAYREF)  stringArgsUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_NOPOLL()
    GCPROTECT_BEGIN(gc);
    HELPER_METHOD_POLL();

    CHECKGC();
    THROWSCOMPLUSEXCEPTION();

    AppDomain* pDomain = ValidateArg(gc.refThis);

    if (!gc.assemblyName)
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_Generic");

    Assembly* pAssembly = (Assembly*) gc.assemblyName->GetAssembly();

    if((BaseDomain*) pDomain == SystemDomain::System()) 
        COMPlusThrow(kUnauthorizedAccessException, L"UnauthorizedAccess_SystemDomain");

    if (!pDomain->m_pRootFile)
        pDomain->m_pRootFile = pAssembly->GetSecurityModule()->GetPEFile();



    HRESULT hr = pAssembly->ExecuteMainMethod(&gc.stringArgs);
    if(FAILED(hr)) 
        COMPlusThrowHR(hr);


    iRetVal = GetLatchedExitCode ();

    GCPROTECT_END();
    HELPER_METHOD_FRAME_END();

    return iRetVal;
}
FCIMPLEND


FCIMPL8(Object*, AppDomainNative::CreateDynamicAssembly, AppDomainBaseObject* refThisUNSAFE, AssemblyNameBaseObject* assemblyNameUNSAFE, Object* identityUNSAFE, StackCrawlMark* stackMark, Object* requiredPsetUNSAFE, Object* optionalPsetUNSAFE, Object* refusedPsetUNSAFE, INT32 access)
{
    ASSEMBLYREF refRetVal = NULL;

    CreateDynamicAssemblyArgs   args;

    args.refThis        = (APPDOMAINREF)    refThisUNSAFE;
    args.assemblyName   = (ASSEMBLYNAMEREF) assemblyNameUNSAFE;
    args.identity       = (OBJECTREF)       identityUNSAFE;
    args.requiredPset   = (OBJECTREF)       requiredPsetUNSAFE;
    args.optionalPset   = (OBJECTREF)       optionalPsetUNSAFE;
    args.refusedPset    = (OBJECTREF)       refusedPsetUNSAFE;

    args.access         = access;
    args.stackMark      = stackMark;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_NOPOLL(Frame::FRAME_ATTR_RETURNOBJ);
    GCPROTECT_BEGIN(args.refThis);
    GCPROTECT_BEGIN(args.assemblyName);
    GCPROTECT_BEGIN(args.identity);
    GCPROTECT_BEGIN(args.requiredPset);
    GCPROTECT_BEGIN(args.optionalPset);
    GCPROTECT_BEGIN(args.refusedPset);
    HELPER_METHOD_POLL();

    THROWSCOMPLUSEXCEPTION();

    Assembly*       pAssem;
    AppDomain* pAppDomain = ValidateArg(args.refThis);

    if((BaseDomain*) pAppDomain == SystemDomain::System()) 
        COMPlusThrow(kUnauthorizedAccessException, L"UnauthorizedAccess_SystemDomain");

    HRESULT hr = pAppDomain->CreateDynamicAssembly(&args, &pAssem);
    if (FAILED(hr))
        COMPlusThrowHR(hr);

    pAppDomain->AddAssembly(pAssem);
    refRetVal = (ASSEMBLYREF) pAssem->GetExposedObject();

    GCPROTECT_END();
    GCPROTECT_END();
    GCPROTECT_END();
    GCPROTECT_END();
    GCPROTECT_END();
    GCPROTECT_END();
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refRetVal);
}
FCIMPLEND



FCIMPL1(Object*, AppDomainNative::GetFriendlyName, AppDomainBaseObject* refThisUNSAFE)
{
    STRINGREF    str     = NULL;
    APPDOMAINREF refThis = (APPDOMAINREF) refThisUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refThis);

    THROWSCOMPLUSEXCEPTION();

    AppDomain* pApp = ValidateArg(refThis);

    LPCWSTR wstr = pApp->GetFriendlyName();
    if (wstr)
        str = COMString::NewString(wstr);   

    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(str);
}
FCIMPLEND


FCIMPL1(Object*, AppDomainNative::GetAssemblies, AppDomainBaseObject* refThisUNSAFE);
{
    PTRARRAYREF     AsmArray = NULL;
    APPDOMAINREF    refThis  = (APPDOMAINREF) refThisUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refThis);

    CHECKGC();
    size_t numSystemAssemblies;
    THROWSCOMPLUSEXCEPTION();

    MethodTable *pAssemblyClass = g_Mscorlib.GetClass(CLASS__ASSEMBLY);

    AppDomain* pApp = ValidateArg(refThis);
    _ASSERTE(SystemDomain::System()->GetSharePolicy() == BaseDomain::SHARE_POLICY_ALWAYS);

    BOOL fNotSystemDomain = ( (AppDomain*) SystemDomain::System() != pApp );
    GCPROTECT_BEGIN(AsmArray);

    if (fNotSystemDomain) {
        SystemDomain::System()->EnterLoadLock();
        numSystemAssemblies = SystemDomain::System()->m_Assemblies.GetCount();
    }
    else
        numSystemAssemblies = 0;

    pApp->EnterLoadLock();

    AsmArray = (PTRARRAYREF) AllocateObjectArray((DWORD) (numSystemAssemblies +
                                                 pApp->m_Assemblies.GetCount()),
                                                 pAssemblyClass);
    if (!AsmArray) {
        pApp->LeaveLoadLock();
        if (fNotSystemDomain)
            SystemDomain::System()->LeaveLoadLock();
        COMPlusThrowOM();
    }

    if (fNotSystemDomain) {
        AppDomain::AssemblyIterator systemIterator = SystemDomain::System()->IterateAssemblies();
        while (systemIterator.Next()) {
            // Do not change this code.  This is done this way to
            //  prevent a GC hole in the SetObjectReference() call.  The compiler
            //  is free to pick the order of evaluation.
            OBJECTREF o = (OBJECTREF) systemIterator.GetAssembly()->GetExposedObject();
            AsmArray->SetAt(systemIterator.GetIndex(), o);
        }
    }

    AppDomain::AssemblyIterator i = pApp->IterateAssemblies();
    while (i.Next()) {
        // Do not change this code.  This is done this way to
        //  prevent a GC hole in the SetObjectReference() call.  The compiler
        //  is free to pick the order of evaluation.
        OBJECTREF o = (OBJECTREF) i.GetAssembly()->GetExposedObject();
        AsmArray->SetAt(numSystemAssemblies++, o);
    }

    pApp->LeaveLoadLock();
    if (fNotSystemDomain)
        SystemDomain::System()->LeaveLoadLock();


    GCPROTECT_END();
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(AsmArray);
}
FCIMPLEND

FCIMPL2(void, AppDomainNative::Unload, INT32 dwId, ThreadBaseObject* requestingThreadUNSAFE)
{
    THREADBASEREF requestingThread = (THREADBASEREF) requestingThreadUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_1(requestingThread);

    THROWSCOMPLUSEXCEPTION();
    AppDomain *pApp = SystemDomain::System()->GetAppDomainAtId(dwId);

    _ASSERTE(pApp); // The call to GetIdForUnload should ensure we have a valid domain
    
    Thread *pRequestingThread = NULL;
    if (requestingThread != NULL)
        pRequestingThread = requestingThread->GetInternal();

    pApp->Unload(FALSE, pRequestingThread);

    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

FCIMPL1(INT32, AppDomainNative::IsDomainIdValid, INT32 dwId)
{
    INT32 iRetVal = 0;
    HELPER_METHOD_FRAME_BEGIN_RET_0()

    THROWSCOMPLUSEXCEPTION();
    // NOTE: This assumes that appDomain IDs are not recycled post unload
    // thus relying on GetAppDomainAtId to return NULL if the appDomain
    // got unloaded or the id is bogus.
    iRetVal = (SystemDomain::System()->GetAppDomainAtId(dwId) != NULL);
    HELPER_METHOD_FRAME_END();
    return iRetVal;
}
FCIMPLEND

FCIMPL0(Object*, AppDomainNative::GetDefaultDomain)
{
    THROWSCOMPLUSEXCEPTION();
    APPDOMAINREF rv = NULL;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, rv);

    if (GetThread()->GetDomain() == SystemDomain::System()->DefaultDomain())
        rv = (APPDOMAINREF) SystemDomain::System()->DefaultDomain()->GetExposedObject();
    else
        rv = (APPDOMAINREF) SystemDomain::System()->DefaultDomain()->GetAppDomainProxy();

    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(rv);
}
FCIMPLEND

FCIMPL1(INT32, AppDomainNative::GetId, AppDomainBaseObject* refThisUNSAFE)
{
    INT32        iRetVal = 0;
    APPDOMAINREF refThis = (APPDOMAINREF) refThisUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_1(refThis);

    THROWSCOMPLUSEXCEPTION();
    AppDomain* pApp = ValidateArg(refThis);
    // can only be accessed from within current domain
    _ASSERTE(GetThread()->GetDomain() == pApp);

    iRetVal = pApp->GetId();

    HELPER_METHOD_FRAME_END();
    return iRetVal;
}
FCIMPLEND

FCIMPL1(INT32, AppDomainNative::GetIdForUnload, AppDomainBaseObject* refDomainUNSAFE)
{
    INT32        iRetVal   = 0;
    APPDOMAINREF refDomain = (APPDOMAINREF) refDomainUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_1(refDomain);
    THROWSCOMPLUSEXCEPTION();

    AppDomain *pDomain = NULL;

    if (refDomain == NULL)
       COMPlusThrow(kArgumentNullException, L"ArgumentNull_Obj");

    if (! refDomain->GetMethodTable()->IsTransparentProxyType()) {
        pDomain = (AppDomain*)(refDomain->GetDomain());

        if (!pDomain)
            COMPlusThrow(kNullReferenceException, L"NullReference");
    } 
    else {
        // so this is an proxy type, now get it's underlying appdomain which will be null if non-local
        Context *pContext = CRemotingServices::GetServerContextForProxy((OBJECTREF)refDomain);
        if (pContext)
            pDomain = pContext->GetDomain();
        else
            COMPlusThrow(kCannotUnloadAppDomainException, IDS_EE_ADUNLOAD_NOT_LOCAL);
    }

    _ASSERTE(pDomain);

    if (! pDomain->IsOpen() || pDomain->GetId() == 0)
        COMPlusThrow(kAppDomainUnloadedException);

    iRetVal = pDomain->GetId();

    HELPER_METHOD_FRAME_END();
    return iRetVal;
}
FCIMPLEND

FCIMPL1(Object*, AppDomainNative::GetUnloadWorker, AppDomainBaseObject* refThisUNSAFE)
{
    OBJECTREF    refRetVal = NULL;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refRetVal);

    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(GetThread()->GetDomain() == SystemDomain::System()->DefaultDomain());
    refRetVal = SystemDomain::System()->DefaultDomain()->GetUnloadWorker();

    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refRetVal);
}
FCIMPLEND

FCIMPL1(void, AppDomainNative::ForcePolicyResolution, AppDomainBaseObject* refThisUNSAFE)
{
    APPDOMAINREF refThis = (APPDOMAINREF) refThisUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_1(refThis);

    THROWSCOMPLUSEXCEPTION();
    AppDomain* pApp = ValidateArg(refThis);

    // Force a security policy resolution on each assembly currently loaded into
    // the domain.
    AppDomain::AssemblyIterator i = pApp->IterateAssemblies();
    while (i.Next())
        i.GetAssembly()->GetSecurityDescriptor(pApp)->Resolve();

    HELPER_METHOD_FRAME_END();
}
FCIMPLEND


FCIMPL2(Object*, AppDomainNative::IsStringInterned, AppDomainBaseObject* refThisUNSAFE, StringObject* pStringUNSAFE)
{
    THROWSCOMPLUSEXCEPTION();

    APPDOMAINREF    refThis     = (APPDOMAINREF)ObjectToOBJECTREF(refThisUNSAFE);
    STRINGREF       refString   = ObjectToSTRINGREF(pStringUNSAFE);
    STRINGREF*      prefRetVal  = NULL;

	ValidateArg(refThis);

    if (refString == NULL)
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_String");
    
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_2(Frame::FRAME_ATTR_RETURNOBJ, refThis, refString);

    prefRetVal = refThis->GetDomain()->IsStringInterned(&refString);

    HELPER_METHOD_FRAME_END();

    if (prefRetVal == NULL)
        return NULL;

    return OBJECTREFToObject(*prefRetVal);
}
FCIMPLEND

FCIMPL2(Object*, AppDomainNative::GetOrInternString, AppDomainBaseObject* refThisUNSAFE, StringObject* pStringUNSAFE)
{
    STRINGREF    refRetVal  = NULL;
    APPDOMAINREF refThis    = (APPDOMAINREF) refThisUNSAFE;
    STRINGREF    pString    = (STRINGREF)    pStringUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_2(Frame::FRAME_ATTR_RETURNOBJ, refThis, pString);

    THROWSCOMPLUSEXCEPTION();
    ValidateArg(refThis);

    if (pString == NULL)
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_String");

    STRINGREF* stringVal = refThis->GetDomain()->GetOrInternString(&pString);
    if (stringVal != NULL)
    {
        refRetVal = *stringVal;
    }

    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refRetVal);
}
FCIMPLEND


FCIMPL1(Object*, AppDomainNative::GetDynamicDir, AppDomainBaseObject* refThisUNSAFE)
{
    STRINGREF    str        = NULL;
    APPDOMAINREF refThis    = (APPDOMAINREF) refThisUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refThis);

    THROWSCOMPLUSEXCEPTION();
    AppDomain *pDomain = ValidateArg(refThis);

    HRESULT hr;
    LPWSTR pDynamicDir;
    if (SUCCEEDED(hr = pDomain->GetDynamicDir(&pDynamicDir))) 
    {
        str = COMString::NewString(pDynamicDir);
    }
    // return NULL when the dyn dir wasn't set
    else if (hr != HRESULT_FROM_WIN32(ERROR_NOT_FOUND))
    {
        COMPlusThrowHR(hr);
    }

    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(str);
}
FCIMPLEND

FCIMPL1(void, AppDomainNative::ForceResolve, AppDomainBaseObject* refThisUNSAFE)
{
    APPDOMAINREF refThis = (APPDOMAINREF) refThisUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_1(refThis);

    THROWSCOMPLUSEXCEPTION();

    AppDomain* pAppDomain = ValidateArg(refThis);
  
    // We get the evidence so that even if security is off
    // we generate the evidence properly.
    Security::InitSecurity();
    pAppDomain->GetSecurityDescriptor()->GetEvidence();
    pAppDomain->GetSecurityDescriptor()->Resolve();

    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

FCIMPL3(void, AppDomainNative::GetGrantSet, AppDomainBaseObject* refThisUNSAFE, OBJECTREF* ppGranted, OBJECTREF* ppDenied)
{
    APPDOMAINREF refThis = (APPDOMAINREF) refThisUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_1(refThis);

    THROWSCOMPLUSEXCEPTION();

    AppDomain* pAppDomain = ValidateArg(refThis);    
    ApplicationSecurityDescriptor *pSecDesc = pAppDomain->GetSecurityDescriptor();
    pSecDesc->Resolve();
    OBJECTREF granted = pSecDesc->GetGrantedPermissionSet(ppDenied);
    *ppGranted = granted;

    HELPER_METHOD_FRAME_END();
}
FCIMPLEND


FCIMPL2(INT32, AppDomainNative::IsTypeUnloading, AppDomainBaseObject* refThisUNSAFE, ReflectClassBaseObject* typeUNSAFE)
{
    INT32               iRetVal = 0;
    APPDOMAINREF        refThis = (APPDOMAINREF)        refThisUNSAFE;
    REFLECTCLASSBASEREF type    = (REFLECTCLASSBASEREF) typeUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_2(refThis, type);

    THROWSCOMPLUSEXCEPTION();

    AppDomain* pApp = ValidateArg(refThis);
    MethodTable *pMT = type->GetMethodTable();

    iRetVal = (!pMT->IsShared()) && (pMT->GetDomain() == pApp);

    HELPER_METHOD_FRAME_END();
    return iRetVal;
}
FCIMPLEND
    
FCIMPL1(INT32, AppDomainNative::IsUnloadingForcedFinalize, AppDomainBaseObject* refThis)
{
    THROWSCOMPLUSEXCEPTION();

    AppDomain* pApp = ValidateArg((APPDOMAINREF)refThis);

    return pApp->IsFinalized();
}
FCIMPLEND

FCIMPL1(INT32, AppDomainNative::IsFinalizingForUnload, AppDomainBaseObject* refThisUNSAFE)
{
    INT32           iRetVal = 0;
    APPDOMAINREF    refThis = (APPDOMAINREF) refThisUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_1(refThis);
    THROWSCOMPLUSEXCEPTION();

    AppDomain* pApp = ValidateArg(refThis);
    iRetVal = pApp->IsFinalizing();

    HELPER_METHOD_FRAME_END();
    return iRetVal;
}
FCIMPLEND


