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
** Header:  Security.cpp
**       
** Purpose: Security implementation and initialization.
**
** Date:  April 15, 1998
**
===========================================================*/

#include "common.h"

#include "object.h"
#include "excep.h"
#include "vars.hpp"
#include "security.h"
#include "permset.h"
#include "perfcounters.h"
#include "comcodeaccesssecurityengine.h"
#include "comsecurityruntime.h"
#include "comsecurityconfig.h"
#include "comstring.h"
#include "../classlibnative/inc/nlstable.h"
#include "frames.h"
#include "ndirect.h"
#include "strongname.h"
#include "wsperf.h"
#include "eeconfig.h"
#include "field.h"
#include "appdomainhelper.h"


#include "threads.inl"

static const DWORD DCL_FLAG_MAP[] =
{
    0,                      // dclActionNil
    DECLSEC_REQUESTS,       // dclRequest
    DECLSEC_DEMANDS,        // dclDemand
    DECLSEC_ASSERTIONS,     //
    DECLSEC_DENIALS,        //
    DECLSEC_PERMITONLY,     //
    DECLSEC_LINK_CHECKS,    //
    DECLSEC_INHERIT_CHECKS, // dclInheritanceCheck
    DECLSEC_REQUESTS,
    DECLSEC_REQUESTS,
    DECLSEC_REQUESTS,
    0,
    0,
    DECLSEC_NONCAS_DEMANDS,
    DECLSEC_NONCAS_LINK_DEMANDS,
    DECLSEC_NONCAS_INHERITANCE,
};

#define  DclToFlag(dcl) DCL_FLAG_MAP[dcl]

// Statics
DWORD Security::s_dwGlobalSettings = 0;
BOOL SecurityDescriptor::s_quickCacheEnabled = TRUE;


void *SecurityProperties::operator new(size_t size, LoaderHeap *pHeap)
{
    return pHeap->AllocMem(size);
}

void SecurityProperties::operator delete(void *pMem)
{
    // No action required
}


Security::StdSecurityInfo Security::s_stdData;



void Security::InitData()
{
    THROWSCOMPLUSEXCEPTION();

    static BOOL initialized = FALSE;

    if (!initialized)
    {
        // Note: these buffers should be at least as big as the longest possible
        // string that will be placed into them by the code below.
        WCHAR* cache = new WCHAR[MAX_PATH + sizeof( L"defaultusersecurity.config.cch" ) / sizeof( WCHAR ) + 1];
        WCHAR* config = new WCHAR[MAX_PATH + sizeof( L"defaultusersecurity.config.cch" ) / sizeof( WCHAR ) + 1];

        if (cache == NULL || config == NULL)
            FATAL_EE_ERROR();

        BOOL result;
        
        result = COMSecurityConfig::GetMachineDirectory( config, MAX_PATH );

        _ASSERTE( result );
        if (!result)
            FATAL_EE_ERROR();

        wcscat( config, L"security.config" );
        wcscpy( cache, config );
        wcscat( cache, L".cch" );
        COMSecurityConfig::InitData( COMSecurityConfig::MachinePolicyLevel, config, cache );

        result = COMSecurityConfig::GetMachineDirectory( config, MAX_PATH );

        _ASSERTE( result );
        if (!result)
            FATAL_EE_ERROR();

        wcscat( config, L"enterprisesec.config" );
        wcscpy( cache, config );
        wcscat( cache, L".cch" );
        COMSecurityConfig::InitData( COMSecurityConfig::EnterprisePolicyLevel, config, cache );

        result = COMSecurityConfig::GetUserDirectory( config, MAX_PATH, FALSE );

        if (!result)
        {
            result = COMSecurityConfig::GetMachineDirectory( config, MAX_PATH );

            _ASSERTE( result );
            if (!result)
                FATAL_EE_ERROR();

            wcscat( config, L"defaultusersecurity.config" );
        }
        else
        {
            wcscat( config, L"security.config" );
        }

        wcscpy( cache, config );
        wcscat( cache, L".cch" );
        COMSecurityConfig::InitData( COMSecurityConfig::UserPolicyLevel, config, cache );

        delete [] cache;
        delete [] config;


        initialized = TRUE;
    }
}



HRESULT Security::Start()
{

    ApplicationSecurityDescriptor::s_LockForAppwideFlags = ::new Crst("Appwide Security Flags", CrstPermissionLoad);

#ifdef _DEBUG
    if (g_pConfig->GetSecurityOptThreshold())
        ApplicationSecurityDescriptor::s_dwSecurityOptThreshold = g_pConfig->GetSecurityOptThreshold();
#endif

    SecurityHelper::Init();
    CompressedStack::Init();

    COMSecurityConfig::Init();

    return GetSecuritySettings(&s_dwGlobalSettings);
}

void Security::Stop()
{
    ::delete ApplicationSecurityDescriptor::s_LockForAppwideFlags;

    SecurityHelper::Shutdown();
    CompressedStack::Shutdown();
}

void Security::SaveCache()
{
    COMSecurityConfig::SaveCacheData( COMSecurityConfig::MachinePolicyLevel );
    COMSecurityConfig::SaveCacheData( COMSecurityConfig::UserPolicyLevel );
    COMSecurityConfig::SaveCacheData( COMSecurityConfig::EnterprisePolicyLevel );

    COMSecurityConfig::Cleanup();
}

//-----------------------------------------------------------
// Currently this method is only useful for setting up a
// cached pointer to the SecurityManager class.
// In the future it may be used to set up other structures.
// For now, it does not even need to be called unless the
// cached pointer is to be used.
//-----------------------------------------------------------
void Security::InitSecurity()
{
        // In the event that we need to run the class initializer (ie, the first time we
        // call this method), running the class initializer allocates a string.  Hence,
        // you can never call InitSecurity (even indirectly) from any FCALL method.  The GC will
        // assert this later of course, but this may save you 10 mins. of debugging.                        
        TRIGGERSGC();

    if (IsInitialized())
        return;

    Thread *pThread = GetThread();
    BOOLEAN bGCDisabled = pThread->PreemptiveGCDisabled();
    if (!bGCDisabled)
        pThread->DisablePreemptiveGC();

    COMPLUS_TRY
    {
        s_stdData.pMethPermSetContains = g_Mscorlib.GetMethod(METHOD__PERMISSION_SET__CONTAINS);
        s_stdData.pMethGetCodeAccessEngine = g_Mscorlib.GetMethod(METHOD__SECURITY_MANAGER__GET_SECURITY_ENGINE);
        s_stdData.pMethResolvePolicy = g_Mscorlib.GetMethod(METHOD__SECURITY_MANAGER__RESOLVE_POLICY);
        s_stdData.pMethCheckFileAccess = g_Mscorlib.GetMethod(METHOD__SECURITY_MANAGER__CHECK_FILE_ACCESS);
        s_stdData.pMethCheckGrantSets = g_Mscorlib.GetMethod(METHOD__SECURITY_MANAGER__CHECK_GRANT_SETS);
        s_stdData.pMethCreateSecurityIdentity = g_Mscorlib.GetMethod(METHOD__ASSEMBLY__CREATE_SECURITY_IDENTITY);
        s_stdData.pMethAppDomainCreateSecurityIdentity = g_Mscorlib.GetMethod(METHOD__APP_DOMAIN__CREATE_SECURITY_IDENTITY);
        s_stdData.pMethPermSetDemand = g_Mscorlib.GetMethod(METHOD__PERMISSION_SET__DEMAND);
        s_stdData.pMethPrivateProcessMessage = g_Mscorlib.FetchMethod(METHOD__STACK_BUILDER_SINK__PRIVATE_PROCESS_MESSAGE);
        s_stdData.pTypeRuntimeMethodInfo = g_Mscorlib.FetchClass(CLASS__METHOD);
        s_stdData.pTypeMethodBase = g_Mscorlib.FetchClass(CLASS__METHOD_BASE);
        s_stdData.pTypeRuntimeConstructorInfo = g_Mscorlib.FetchClass(CLASS__CONSTRUCTOR);
        s_stdData.pTypeConstructorInfo = g_Mscorlib.FetchClass(CLASS__CONSTRUCTOR_INFO);
        s_stdData.pTypeRuntimeType = g_Mscorlib.FetchClass(CLASS__CLASS);
        s_stdData.pTypeType = g_Mscorlib.FetchClass(CLASS__TYPE);
        s_stdData.pTypeRuntimeEventInfo = g_Mscorlib.FetchClass(CLASS__EVENT);
        s_stdData.pTypeEventInfo = g_Mscorlib.FetchClass(CLASS__EVENT_INFO);
        s_stdData.pTypeRuntimePropertyInfo = g_Mscorlib.FetchClass(CLASS__PROPERTY);
        s_stdData.pTypePropertyInfo = g_Mscorlib.FetchClass(CLASS__PROPERTY_INFO);
        s_stdData.pTypeActivator = g_Mscorlib.FetchClass(CLASS__ACTIVATOR);
        s_stdData.pTypeAppDomain = g_Mscorlib.FetchClass(CLASS__APP_DOMAIN);
        s_stdData.pTypeAssembly = g_Mscorlib.FetchClass(CLASS__ASSEMBLY);
    }
    COMPLUS_CATCH
    {
        // We shouldn't get any exceptions. We just have the handler to keep the
        // EE exception handler happy.
        _ASSERTE(FALSE);
    } 
    COMPLUS_END_CATCH

    if (!bGCDisabled)
        pThread->EnablePreemptiveGC();

    SetInitialized();
}

// This is an ecall
FCIMPL3(void, Security::GetGrantedPermissions, OBJECTREF* ppGranted, OBJECTREF* ppDenied, OBJECTREF* stackmark)
{
    HELPER_METHOD_FRAME_BEGIN_0();
    //-[autocvtpro]-------------------------------------------------------

    // Skip one frame (SecurityManager.IsGranted) to get to the caller

    AppDomain* pDomain;
    Assembly* callerAssembly = SystemDomain::GetCallersAssembly( (StackCrawlMark*)stackmark, &pDomain );

    _ASSERTE( callerAssembly != NULL);

    AssemblySecurityDescriptor* pSecDesc = callerAssembly->GetSecurityDescriptor(pDomain);

    _ASSERTE( pSecDesc != NULL );

    OBJECTREF token = pSecDesc->GetGrantedPermissionSet(ppDenied);
    *(ppGranted) = token;

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

FCIMPL0(DWORD, Security::IsSecurityOnNative)
{
    return IsSecurityOn();
}
FCIMPLEND

// This is for ecall.
FCIMPL0(DWORD, Security::GetGlobalSecurity)
{
    DWORD retVal;
    HELPER_METHOD_FRAME_BEGIN_RET_0();
    //-[autocvtpro]-------------------------------------------------------

    retVal = GlobalSettings();

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return retVal;
}
FCIMPLEND


FCIMPL2(void, Security::SetGlobalSecurity, DWORD mask, DWORD flags)
{
    HELPER_METHOD_FRAME_BEGIN_0();
    //-[autocvtpro]-------------------------------------------------------

    Security::SetGlobalSettings( mask, flags );

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

FCIMPL0(void, Security::SaveGlobalSecurity)
{
    HELPER_METHOD_FRAME_BEGIN_0();
    //-[autocvtpro]-------------------------------------------------------

    ::SetSecuritySettings(GlobalSettings());

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

void Security::_GetSharedPermissionInstance(OBJECTREF *perm, int index)
{
    _ASSERTE(index < (int) NUM_PERM_OBJECTS);

    COMPLUS_TRY
    {
     AppDomain *pDomain = GetAppDomain();
    SharedPermissionObjects *pShared = &pDomain->m_pSecContext->m_rPermObjects[index];

    if (pShared->hPermissionObject == NULL) {
        pShared->hPermissionObject = pDomain->CreateHandle(NULL);
        *perm = NULL;
    }
    else
        *perm = ObjectFromHandle(pShared->hPermissionObject);

    if (*perm == NULL)
    {
        MethodTable *pMT = NULL;
        OBJECTREF p = NULL;

        GCPROTECT_BEGIN(p);

        pMT = g_Mscorlib.GetClass(pShared->idClass);
        MethodDesc *pCtor = g_Mscorlib.GetMethod(pShared->idConstructor);

        p = AllocateObject(pMT);

        ARG_SLOT argInit[2] =
        {
            ObjToArgSlot(p),
            (ARG_SLOT) pShared->bPermissionFlag
        };

        pCtor->Call(argInit, pShared->idConstructor);

        StoreObjectInHandle(pShared->hPermissionObject, p);

        *perm = p;
        GCPROTECT_END();
    }
}
    COMPLUS_CATCH
    {
        *perm = NULL;
    }
    COMPLUS_END_CATCH
}

//---------------------------------------------------------
// Does a method have a REQ_SO CustomAttribute?
//
// S_OK    = yes
// S_FALSE = no
// FAILED  = unknown because something failed.
//---------------------------------------------------------
/*static*/
HRESULT Security::HasREQ_SOAttribute(IMDInternalImport *pInternalImport, mdToken token)
{
    _ASSERTE(TypeFromToken(token) == mdtMethodDef);

    DWORD       dwAttr = pInternalImport->GetMethodDefProps(token);

    return (dwAttr & mdRequireSecObject) ? S_OK : S_FALSE;
}

// Called at the beginning of code-access and code-identity stack walk
// callback functions.
//
// Returns true if action set
//         false if no action
BOOL Security::SecWalkCommonProlog (SecWalkPrologData * pData,
                                    MethodDesc * pMeth,
                                    StackWalkAction * pAction,
                                    CrawlFrame * pCf)
{
    _ASSERTE (pData && pMeth && pAction) ; // args should never be null

    // First check if the walk has skipped the required frames. The check
    // here is between the address of a local variable (the stack mark) and a
    // pointer to the EIP for a frame (which is actually the pointer to the
    // return address to the function from the previous frame). So we'll
    // actually notice which frame the stack mark was in one frame later. This
    // is fine for our purposes since we're always looking for the frame of the
    // caller (or the caller's caller) of the method that actually created the
    // stack mark.
    _ASSERTE((pData->pStackMark == NULL) || (*pData->pStackMark == LookForMyCaller) || (*pData->pStackMark == LookForMyCallersCaller));
    if ((pData->pStackMark != NULL) &&
        !IsInCalleesFrames(pCf->GetRegisterSet(), pData->pStackMark))
    {
        DBG_TRACE_STACKWALK("        Skipping before start...\n", true);

        *pAction = SWA_CONTINUE;
        return TRUE;
    }

    // Skip reflection invoke / remoting frames if we've been asked to.
    if (pData->dwFlags & CORSEC_SKIP_INTERNAL_FRAMES)
    {
        InitSecurity();

        if (pMeth == s_stdData.pMethPrivateProcessMessage)
        {
            _ASSERTE(!pData->bSkippingRemoting);
            pData->bSkippingRemoting = TRUE;
            DBG_TRACE_STACKWALK("        Skipping remoting PrivateProcessMessage frame...\n", true);
            *pAction = SWA_CONTINUE;
            return TRUE;
        }
        if (!pCf->IsFrameless() && pCf->GetFrame()->GetFrameType() == Frame::TYPE_TP_METHOD_FRAME)
        {
            _ASSERTE(pData->bSkippingRemoting);
            pData->bSkippingRemoting = FALSE;
            DBG_TRACE_STACKWALK("        Skipping remoting TP frame...\n", true);
            *pAction = SWA_CONTINUE;
            return TRUE;
        }
        if (pData->bSkippingRemoting)
        {
            DBG_TRACE_STACKWALK("        Skipping remoting frame...\n", true);
            *pAction = SWA_CONTINUE;
            return TRUE;
        }

        MethodTable *pMT = pMeth->GetMethodTable();
        if (pMT == s_stdData.pTypeRuntimeMethodInfo ||
            pMT == s_stdData.pTypeMethodBase ||
            pMT == s_stdData.pTypeRuntimeConstructorInfo ||
            pMT == s_stdData.pTypeConstructorInfo ||
            pMT == s_stdData.pTypeRuntimeType ||
            pMT == s_stdData.pTypeType ||
            pMT == s_stdData.pTypeRuntimeEventInfo ||
            pMT == s_stdData.pTypeEventInfo ||
            pMT == s_stdData.pTypeRuntimePropertyInfo ||
            pMT == s_stdData.pTypePropertyInfo ||
            pMT == s_stdData.pTypeActivator ||
            pMT == s_stdData.pTypeAppDomain ||
            pMT == s_stdData.pTypeAssembly)
        {
            DBG_TRACE_STACKWALK("        Skipping reflection invoke frame...\n", true);
            *pAction = SWA_CONTINUE;
            return TRUE;
        }
    }

    // If we're looking for the caller's caller, skip the frame after the stack
    // mark as well.
    if ((pData->pStackMark != NULL) &&
        (*pData->pStackMark == LookForMyCallersCaller) &&
        !pData->bFoundCaller)
    {
        DBG_TRACE_STACKWALK("        Skipping before start...\n", true);
        pData->bFoundCaller = TRUE;
        *pAction = SWA_CONTINUE;
        return TRUE;
    }

    // Then check if the walk has checked the maximum required frames.
    if (pData->cCheck >= 0)
    {
        if (pData->cCheck == 0)
        {
            DBG_TRACE_STACKWALK("        Halting stackwalk - check limit reached.\n", false);
            pData->dwFlags |= CORSEC_STACKWALK_HALTED;
            *pAction = SWA_ABORT;
            return TRUE;
        }
        else
        {
            --(pData->cCheck);
            // ...and fall through to perform a check...
        }
    }

    return FALSE;
}



// Returns TRUE if the token has declarations of the type specified by 'action'
BOOL Security::TokenHasDeclarations(IMDInternalImport *pInternalImport, mdToken token, CorDeclSecurity action)
{
    HRESULT hr = S_OK;
    HENUMInternal hEnumDcl;
    DWORD cDcl;

    // Check if the token has declarations for
    // the action specified.
    hr = pInternalImport->EnumPermissionSetsInit(
        token,
        action,
        &hEnumDcl);

    if (FAILED(hr) || hr == S_FALSE)
        return FALSE;

    cDcl = pInternalImport->EnumGetCount(&hEnumDcl);
    pInternalImport->EnumClose(&hEnumDcl);

    return (cDcl > 0);
}

// Accumulate status of declarative security.
// NOTE: This method should only be called after it has been
//       determined that the token has declarative security.
//       It will work if there is no security, but it is an
//       expensive call to make to find out.
HRESULT Security::GetDeclarationFlags(IMDInternalImport *pInternalImport, mdToken token, DWORD* pdwFlags, DWORD* pdwNullFlags)
{
    HENUMInternal   hEnumDcl;
    HRESULT         hr;
    DWORD           dwFlags = 0;
    DWORD           dwNullFlags = 0;

    _ASSERTE(pdwFlags);
    *pdwFlags = 0;

    if (pdwNullFlags)
        *pdwNullFlags = 0;

    hr = pInternalImport->EnumPermissionSetsInit(token, dclActionNil, &hEnumDcl);
    if (FAILED(hr))
        goto Exit;

    if (hr == S_OK)
    {
        mdPermission    perms;
        DWORD           dwAction;
        DWORD           dwDclFlags;
        ULONG           cbPerm;
        PBYTE           pbPerm;

        while (pInternalImport->EnumNext(&hEnumDcl, &perms))
        {
           pInternalImport->GetPermissionSetProps(
                perms,
                &dwAction,
                (const void**)&pbPerm,
                &cbPerm);

            dwDclFlags = DclToFlag(dwAction);

            dwFlags |= dwDclFlags;

            if (cbPerm == 0)
                dwNullFlags |= dwDclFlags;
        }
    }

    pInternalImport->EnumClose(&hEnumDcl);

    // Disable any runtime checking of UnmanagedCode permission if the correct
    // custom attribute is present.
    if (pInternalImport->GetCustomAttributeByName(token,
                                                  COR_SUPPRESS_UNMANAGED_CODE_CHECK_ATTRIBUTE_ANSI,
                                                  NULL,
                                                  NULL) == S_OK)
    {
        dwFlags |= DECLSEC_UNMNGD_ACCESS_DEMAND;
        dwNullFlags |= DECLSEC_UNMNGD_ACCESS_DEMAND;
    }

    *pdwFlags = dwFlags;
    if (pdwNullFlags)
        *pdwNullFlags = dwNullFlags;

Exit:
    return hr;
}

BOOL Security::ClassInheritanceCheck(EEClass *pClass, EEClass *pParent, OBJECTREF *pThrowable)
{
    _ASSERTE(pClass);
    _ASSERTE(!pClass->IsInterface());
    _ASSERTE(pParent);
//    _ASSERTE(pThrowable);

    if (pClass->GetModule()->IsSystem())
        return TRUE;
/*
    if (pClass->GetAssembly() == pParent->GetAssembly())
        return TRUE;
*/
    BOOL fResult = TRUE;

    _ASSERTE(pParent->RequiresInheritanceCheck());

    {
        // If we have a class that requires inheritance checks,
        // then we require a thread to perform the checks.
        // We won't have a thread when some of the system classes
        // are preloaded, so make sure that none of them have
        // inheritance checks.
        _ASSERTE(GetThread() != NULL);

        COMPLUS_TRY
        {
            struct gc {
                OBJECTREF refCasDemands;
                OBJECTREF refNonCasDemands;
            } gc;
            ZeroMemory(&gc, sizeof(gc));

            GCPROTECT_BEGIN(gc);

            if (pParent->RequiresCasInheritanceCheck())
                gc.refCasDemands = pParent->GetModule()->GetCasInheritancePermissions(pParent->GetCl());

            if (pParent->RequiresNonCasInheritanceCheck())
                gc.refNonCasDemands = pParent->GetModule()->GetNonCasInheritancePermissions(pParent->GetCl());

            if (gc.refCasDemands != NULL)
            {
                // @nice: Currently the linktime check does exactly what
                //        we want for the inheritance check. We might want
                //        give it a more general name.
                COMCodeAccessSecurityEngine::LinktimeCheck(pClass->GetAssembly()->GetSecurityDescriptor(), gc.refCasDemands);
            }

            if (gc.refNonCasDemands != NULL)
            {
                InitSecurity();
                ARG_SLOT arg = ObjToArgSlot(gc.refNonCasDemands);
                Security::s_stdData.pMethPermSetDemand->Call(&arg, METHOD__PERMISSION_SET__DEMAND);
            }

            GetAppDomain()->OnLinktimeCheck(pClass->GetAssembly(), gc.refCasDemands, gc.refNonCasDemands);

            GCPROTECT_END();
        }
        COMPLUS_CATCH
        {
            fResult = FALSE;
            UpdateThrowable(pThrowable);
        }
        COMPLUS_END_CATCH
    }

    return fResult;
}

BOOL Security::MethodInheritanceCheck(MethodDesc *pMethod, MethodDesc *pParent, OBJECTREF *pThrowable)
{
    _ASSERTE(pParent != NULL);
    _ASSERTE (pParent->RequiresInheritanceCheck());
    _ASSERTE(GetThread() != NULL);

    BOOL            fResult = TRUE;
    HRESULT         hr = S_FALSE;
    PBYTE           pbPerm = NULL;
    ULONG           cbPerm = 0;
    void const **   ppData = const_cast<void const**> (reinterpret_cast<void**> (&pbPerm));
    mdPermission    tkPerm;
    HENUMInternal   hEnumDcl;
    DWORD           dwAction;
    IMDInternalImport *pInternalImport = pParent->GetModule()->GetMDImport();

    if (pMethod->GetModule()->IsSystem())
        return TRUE;
/*
    if (pMethod->GetAssembly() == pParent->GetAssembly())
        return TRUE;
*/
    // Lookup the permissions for the given declarative action type.
    hr = pInternalImport->EnumPermissionSetsInit(
        pParent->GetMemberDef(),
        dclActionNil,
        &hEnumDcl);

    if (FAILED(hr))
        return fResult;     // Nothing to check

    if (hr != S_FALSE)
    {
        while (pInternalImport->EnumNext(&hEnumDcl, &tkPerm))
        {
            pInternalImport->GetPermissionSetProps(tkPerm,
                                                   &dwAction,
                                                   ppData,
                                                   &cbPerm);

            if (!pbPerm)
                continue;

            if ((dwAction != dclNonCasInheritance) && (dwAction != dclInheritanceCheck))
                continue;

            COMPLUS_TRY
            {
                if (dwAction == dclNonCasInheritance)
                {
                    OBJECTREF refNonCasDemands = NULL;

                    GCPROTECT_BEGIN(refNonCasDemands);

                    // Decode the bits from the metadata.
                    SecurityHelper::LoadPermissionSet(pbPerm,
                                                      cbPerm,
                                                      &refNonCasDemands,
                                                      NULL);

                    if (refNonCasDemands != NULL)
                    {
                        Security::InitSecurity();
                        ARG_SLOT arg = ObjToArgSlot(refNonCasDemands);
                        Security::s_stdData.pMethPermSetDemand->Call(&arg, METHOD__PERMISSION_SET__DEMAND);
                    }

                    GetAppDomain()->OnLinktimeCheck(pMethod->GetAssembly(), NULL, refNonCasDemands);

                    GCPROTECT_END();
                }
                else if (dwAction == dclInheritanceCheck)  // i,e. If Cas demands..
                {
                    OBJECTREF refCasDemands = NULL;
                    AssemblySecurityDescriptor *pInheritorAssem = pMethod->GetModule()->GetAssembly()->GetSecurityDescriptor();
                    DWORD dwSetIndex;

                    if (SecurityHelper::LookupPermissionSet(pbPerm, cbPerm, &dwSetIndex))
                    {
                        // See if inheritor's assembly has passed this demand before
                        DWORD index = 0;
                        for (; index < pInheritorAssem->m_dwNumPassedDemands; index++)
                        {
                            if (pInheritorAssem->m_arrPassedLinktimeDemands[index] == dwSetIndex)
                                break;
                        }

                        if (index < pInheritorAssem->m_dwNumPassedDemands)
                            goto InheritanceCheckDone;

                        // This is a new demand
                        refCasDemands = SecurityHelper::GetPermissionSet(dwSetIndex);
                    }

                    if (refCasDemands == NULL)
                        // Decode the bits from the metadata.
                        SecurityHelper::LoadPermissionSet(pbPerm,
                                                          cbPerm,
                                                          &refCasDemands,
                                                          NULL,
                                                          &dwSetIndex);

                    if (refCasDemands != NULL)
                    {
                        GCPROTECT_BEGIN(refCasDemands);

                        COMCodeAccessSecurityEngine::LinktimeCheck(pMethod->GetAssembly()->GetSecurityDescriptor(), refCasDemands);
                        // Demand passed. Add it to the Inheritor's assembly's list of passed demands
                        if (pInheritorAssem->m_dwNumPassedDemands <= (MAX_PASSED_DEMANDS - 1))
                            pInheritorAssem->m_arrPassedLinktimeDemands[pInheritorAssem->m_dwNumPassedDemands++] = dwSetIndex;
                        GetAppDomain()->OnLinktimeCheck(pMethod->GetAssembly(), refCasDemands, NULL);

                        GCPROTECT_END();
                    }

                InheritanceCheckDone: ;
                }   // Cas or NonCas
            }
            COMPLUS_CATCH
            {
                fResult = FALSE;
                UpdateThrowable(pThrowable);
            }
            COMPLUS_END_CATCH
        }   // While there are more permission sets
    }   // If Metadata init was ok
    return fResult;
}

void Security::InvokeLinktimeChecks(Assembly *pCaller,
                                    Module *pModule,
                                    mdToken token,
                                    BOOL *pfResult,
                                    OBJECTREF *pThrowable)
{
    _ASSERTE( pCaller );
    _ASSERTE( pModule );

    COMPLUS_TRY
    {
        struct gc {
            OBJECTREF refNonCasDemands;
            OBJECTREF refCasDemands;
        } gc;
        ZeroMemory(&gc, sizeof(gc));

        GCPROTECT_BEGIN(gc);

        gc.refCasDemands = pModule->GetLinktimePermissions(token, &gc.refNonCasDemands);

        if (gc.refCasDemands != NULL)
        {
            COMCodeAccessSecurityEngine::LinktimeCheck(pCaller->GetSecurityDescriptor(), gc.refCasDemands);
        }

        if (gc.refNonCasDemands != NULL)
        {
            // Make sure s_stdData is initialized
            Security::InitSecurity();

            ARG_SLOT arg = ObjToArgSlot(gc.refNonCasDemands);
            Security::s_stdData.pMethPermSetDemand->Call(&arg, METHOD__PERMISSION_SET__DEMAND);
        }

        GetAppDomain()->OnLinktimeCheck(pCaller, gc.refCasDemands, gc.refNonCasDemands);

        GCPROTECT_END();

    }
    COMPLUS_CATCH
    {
        *pfResult = FALSE;
        UpdateThrowable(pThrowable);
    }
    COMPLUS_END_CATCH
}

/*static*/
// This was part of DoUntrustedCallerChecks, but I had
// to separate it because you can't have two COMPLUS_TRY blocks
// in the same method.
void Security::GetSecurityException(OBJECTREF *pThrowable)
{
	THROWSCOMPLUSEXCEPTION();
	COMPLUS_TRY
        {
        	DEBUG_SAFE_TO_THROW_IN_THIS_BLOCK;
		COMPlusThrow(kSecurityException);
	}
	COMPLUS_CATCH
	{
		UpdateThrowable(pThrowable);
	}        
	COMPLUS_END_CATCH
}

/*static*/
// Do a fulltrust check on the caller if the callee is fully trusted and
// callee did not enable AllowUntrustedCallerChecks
BOOL Security::DoUntrustedCallerChecks(
        Assembly *pCaller, MethodDesc *pCallee, OBJECTREF *pThrowable, 
        BOOL fFullStackWalk)
{
    THROWSCOMPLUSEXCEPTION();
    BOOL fRet = TRUE;

#ifdef _DEBUG
    if (!g_pConfig->Do_AllowUntrustedCaller_Checks())
        return TRUE;
#endif

    if (!MethodIsVisibleOutsideItsAssembly(pCallee->GetAttrs()) ||
        !ClassIsVisibleOutsideItsAssembly(pCallee->GetClass()->GetAttrClass())||
        pCallee->GetAssembly()->AllowUntrustedCaller() ||
        (pCaller == pCallee->GetAssembly()))
        return TRUE;

    // Expensive calls after this point, this could end up resolving policy

    OBJECTREF permSet;
    
    if (fFullStackWalk)
    {
        // It is possible that wrappers like VBHelper libraries that are
        // fully trusted, make calls to public methods that do not have
        // safe for Untrusted caller custom attribute set.
        // Like all other link demand that gets transformed to a full stack 
        // walk for reflection, calls to public methods also gets 
        // converted to full stack walk

        permSet = NULL;
        GCPROTECT_BEGIN(permSet);

        GetPermissionInstance(&permSet, SECURITY_FULL_TRUST);
        COMPLUS_TRY
        {
            DEBUG_SAFE_TO_THROW_IN_THIS_BLOCK;
            COMCodeAccessSecurityEngine::DemandSet(permSet);
            
        }
        COMPLUS_CATCH
        {
            fRet = FALSE;
            if (pThrowable != RETURN_ON_ERROR)
                UpdateThrowable(pThrowable);
        }        
        COMPLUS_END_CATCH

        GCPROTECT_END();
    }
    else
    {
        _ASSERTE(pCaller);

        // Link Demand only, no full stack walk here
        if (!pCaller->GetSecurityDescriptor()->IsFullyTrusted())
        {
            fRet = FALSE;
            
            // Construct the exception object
            if (pThrowable != RETURN_ON_ERROR)
            {
		GetSecurityException(pThrowable);
            }
        }
        else
        {
            // Add fulltrust permission Set to the prejit case.
            GetAppDomain()->OnLinktimeFullTrustCheck(pCaller);
        }
    }

    return fRet;
}

//---------------------------------------------------------
// Invoke linktime checks on the caller if demands exist
// for the callee.
//
// TRUE  = check pass
// FALSE = check failed
//---------------------------------------------------------
/*static*/
BOOL Security::LinktimeCheckMethod(Assembly *pCaller, MethodDesc *pCallee, OBJECTREF *pThrowable)
{
/*
    if (pCaller == pCallee->GetAssembly())
        return TRUE;
*/

    // Do a fulltrust check on the caller if the callee is fully trusted and
    // callee did not enable AllowUntrustedCallerChecks
    if (!Security::DoUntrustedCallerChecks(pCaller, pCallee, pThrowable, FALSE))
        return FALSE;

    // Disable linktime checks from mscorlib to mscorlib (non-virtual methods
    // only). This prevents nasty recursions when the managed code used to
    // implement the check requires a security check itself.
    if (!pCaller->GetSecurityDescriptor()->IsSystemClasses() ||
        !pCallee->GetAssembly()->GetSecurityDescriptor()->IsSystemClasses() ||
        pCallee->IsVirtual())
    {
        BOOL        fResult = TRUE;
        EEClass    *pTargetClass = pCallee->GetClass();

        // Track perfmon counters. Linktime security checkes.
        COUNTER_ONLY(GetPrivatePerfCounters().m_Security.cLinkChecks++);
        COUNTER_ONLY(GetGlobalPerfCounters().m_Security.cLinkChecks++);

        // If the class has its own linktime checks, do them first...
        if (pTargetClass->RequiresLinktimeCheck())
        {
            InvokeLinktimeChecks(pCaller,
                                 pTargetClass->GetModule(),
                                 pTargetClass->GetCl(),
                                 &fResult,
                                 pThrowable);
        }

        // If the previous check passed, check the method for
        // method-specific linktime checks...
        if (fResult && IsMdHasSecurity(pCallee->GetAttrs()) &&
            (TokenHasDeclarations(pTargetClass->GetMDImport(),
                                  pCallee->GetMemberDef(),
                                  dclLinktimeCheck) ||
             TokenHasDeclarations(pTargetClass->GetMDImport(),
                                  pCallee->GetMemberDef(),
                                  dclNonCasLinkDemand)))
        {
            InvokeLinktimeChecks(pCaller,
                                 pTargetClass->GetModule(),
                                 pCallee->GetMemberDef(),
                                 &fResult,
                                 pThrowable);
        }

        // We perform automatic linktime checks for UnmanagedCode in three cases:
        //   o  P/Invoke calls
        //   o  Calls through an interface that have a suppress runtime check
        //      attribute on them (these are almost certainly interop calls).
        //   o  Interop calls made through method impls.
        if (pCallee->IsNDirect() ||
            (pTargetClass->IsInterface() &&
             pTargetClass->GetMDImport()->GetCustomAttributeByName(pTargetClass->GetCl(),
                                                                   COR_SUPPRESS_UNMANAGED_CODE_CHECK_ATTRIBUTE_ANSI,
                                                                   NULL,
                                                                   NULL) == S_OK) ||
            (pCallee->IsComPlusCall() && !pCallee->IsInterface()))
        {
            if (pCaller->GetSecurityDescriptor()->CanCallUnmanagedCode(pThrowable))
            {
                GetAppDomain()->OnLinktimeCanCallUnmanagedCheck(pCaller);
            }
            else
                fResult = FALSE;
        }

        return fResult;
    }

    return TRUE;
}


OBJECTREF Security::GetCompressedStack(StackCrawlMark* stackMark)
{
    // Set up security before using its cached pointers.
    InitSecurity();

    _ASSERTE(GetThread()->PreemptiveGCDisabled());

    _ASSERTE(s_stdData.pMethGetCodeAccessEngine != NULL);

    MethodDesc *pMD = g_Mscorlib.GetMethod(METHOD__SECURITY_ENGINE__GET_COMPRESSED_STACK);

    ARG_SLOT args[2];

    OBJECTREF refCodeAccessSecurityEngine =
        ArgSlotToObj(s_stdData.pMethGetCodeAccessEngine->Call((ARG_SLOT*)NULL, METHOD__SECURITY_MANAGER__GET_SECURITY_ENGINE));

    if (refCodeAccessSecurityEngine == NULL)
        return NULL;

    args[0] = ObjToArgSlot(refCodeAccessSecurityEngine);
    args[1] = PtrToArgSlot(stackMark);

    ARG_SLOT ret = pMD->Call(args, METHOD__SECURITY_ENGINE__GET_COMPRESSED_STACK);

    return ArgSlotToObj(ret);
}

CompressedStack* Security::GetDelayedCompressedStack(void)
{
    return COMCodeAccessSecurityEngine::GetCompressedStack();
}


typedef struct _SkipFunctionsData
{
    INT32           cSkipFunctions;
    StackCrawlMark* pStackMark;
    BOOL            bUseStackMark;
    BOOL            bFoundCaller;
    MethodDesc*     pFunction;
    OBJECTREF*      pSecurityObject;
    AppDomain*      pSecurityObjectAppDomain;
} SkipFunctionsData;

static
StackWalkAction SkipFunctionsCB(CrawlFrame* pCf, VOID* pData)
{
    SkipFunctionsData *skipData = (SkipFunctionsData*)pData;
    _ASSERTE(skipData != NULL);

    MethodDesc *pFunc = pCf->GetFunction();
#ifdef _DEBUG
    // Get the interesting info now, so we can get a trace
    // while debugging...
    OBJECTREF  *pSecObj;
    pSecObj = pCf->GetAddrOfSecurityObject();
#endif

    if (skipData->bUseStackMark)
    {
        // First check if the walk has skipped the required frames. The check
        // here is between the address of a local variable (the stack mark) and a
        // pointer to the EIP for a frame (which is actually the pointer to the
        // return address to the function from the previous frame). So we'll
        // actually notice which frame the stack mark was in one frame later. This
        // is fine for our purposes since we're always looking for the frame of the
        // caller of the method that actually created the stack mark. 
        if ((skipData->pStackMark != NULL) &&
            !IsInCalleesFrames(pCf->GetRegisterSet(), skipData->pStackMark))
            return SWA_CONTINUE;
    }
    else
    {
        if (skipData->cSkipFunctions-- > 0)
            return SWA_CONTINUE;
    }

    skipData->pFunction                 = pFunc;
    skipData->pSecurityObject           = pCf->GetAddrOfSecurityObject();
    skipData->pSecurityObjectAppDomain  = pCf->GetAppDomain();
    return SWA_ABORT; // This actually indicates success.
}

// This function skips extra frames created by reflection in addition
// to the number of frames specified in the argument.
BOOL Security::SkipAndFindFunctionInfo(INT32 cSkipFns, MethodDesc ** ppFunc, OBJECTREF ** ppObj, AppDomain ** ppAppDomain)
{
    _ASSERTE(cSkipFns >= 0);
    _ASSERTE(ppFunc != NULL || ppObj != NULL || !"Why was this function called?!");

    SkipFunctionsData walkData;
    walkData.cSkipFunctions = cSkipFns;
    walkData.bUseStackMark = FALSE;
    walkData.pFunction = NULL;
    walkData.pSecurityObject = NULL;
    StackWalkAction action = StackWalkFunctions(GetThread(), SkipFunctionsCB, &walkData);
    if (action == SWA_ABORT)
    {
        if (ppFunc != NULL)
            *ppFunc = walkData.pFunction;
        if (ppObj != NULL)
        {
            *ppObj = walkData.pSecurityObject;
            if (ppAppDomain != NULL)
                *ppAppDomain = walkData.pSecurityObjectAppDomain;
        }
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

// Version of the above method that looks for a stack mark (the address of a
// local variable in a frame called by the target frame).
BOOL Security::SkipAndFindFunctionInfo(StackCrawlMark* stackMark, MethodDesc ** ppFunc, OBJECTREF ** ppObj, AppDomain ** ppAppDomain)
{
    _ASSERTE(ppFunc != NULL || ppObj != NULL || !"Why was this function called?!");

    SkipFunctionsData walkData;
    walkData.pStackMark = stackMark;
    walkData.bUseStackMark = TRUE;
    walkData.bFoundCaller = FALSE;
    walkData.pFunction = NULL;
    walkData.pSecurityObject = NULL;
    StackWalkAction action = StackWalkFunctions(GetThread(), SkipFunctionsCB, &walkData);
    if (action == SWA_ABORT)
    {
        if (ppFunc != NULL)
            *ppFunc = walkData.pFunction;
        if (ppObj != NULL)
        {
            *ppObj = walkData.pSecurityObject;
            if (ppAppDomain != NULL)
                *ppAppDomain = walkData.pSecurityObjectAppDomain;
        }
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

Stub* Security::CreateStub(StubLinker *pstublinker,
                           MethodDesc* pMD,
                           DWORD dwDeclFlags,
                           Stub* pRealStub,
                           LPVOID pRealAddr)
{
    DeclActionInfo *actionsNeeded = DetectDeclActions(pMD, dwDeclFlags);
    if (actionsNeeded == NULL)
        return pRealStub;       // Nothing to do

    // Wrapper needs to know if the stub is intercepted interpreted or jitted.  pRealStub
    // is null when it is jitted.
    BOOL fToStub = pRealStub != NULL;
    CPUSTUBLINKER *psl = (CPUSTUBLINKER*)pstublinker;
    if(dwDeclFlags & DECLSEC_FRAME_ACTIONS)
        psl->EmitSecurityWrapperStub(pMD->SizeOfActualFixedArgStack(), pMD, fToStub, pRealAddr, actionsNeeded);
    else
        psl->EmitSecurityInterceptorStub(pMD, fToStub, pRealAddr, actionsNeeded);

    Stub* result = psl->LinkInterceptor(pMD->GetClass()->GetDomain()->GetStubHeap(),
                                           pRealStub, pRealAddr);
    return result;
}

DeclActionInfo *DeclActionInfo::Init(MethodDesc *pMD, DWORD dwAction, DWORD dwSetIndex)
{
    DeclActionInfo *pTemp;
    WS_PERF_SET_HEAP(LOW_FREQ_HEAP);    
    pTemp = (DeclActionInfo *)pMD->GetClass()->GetClassLoader()->GetLowFrequencyHeap()->AllocMem(sizeof(DeclActionInfo));

    if (pTemp == NULL)
        return NULL;

    WS_PERF_UPDATE_DETAIL("DeclActionInfo low freq", sizeof(DeclActionInfo), pTemp);

    pTemp->dwDeclAction = dwAction;
    pTemp->dwSetIndex = dwSetIndex;
    pTemp->pNext = NULL;

    return pTemp;
}

// Here we see what declarative actions are needed everytime a method is called,
// and create a list of these actions, which will be emitted as an argument to 
// DoDeclarativeSecurity
DeclActionInfo *Security::DetectDeclActions(MethodDesc *pMeth, DWORD dwDeclFlags)
{
    DeclActionInfo              *pDeclActions = NULL;

    EEClass *pCl = pMeth -> GetClass () ;
    _ASSERTE(pCl && "Should be a EEClass pointer here") ;

#ifdef _DEBUG
    PSecurityProperties psp = pCl -> GetSecurityProperties () ;
#endif
    _ASSERTE(psp && "Should be a PSecurityProperties here") ;

    Module *pModule = pMeth -> GetModule () ;
    _ASSERTE(pModule && "Should be a Module pointer here") ;

#ifdef _DEBUG
    AssemblySecurityDescriptor *pMSD = pModule -> GetSecurityDescriptor () ;
#endif
    _ASSERTE(pMSD && "Should be a security descriptor here") ;

    IMDInternalImport          *pInternalImport = pModule->GetMDImport();

    // Lets check the Ndirect/Interop cases first
    if (dwDeclFlags & DECLSEC_UNMNGD_ACCESS_DEMAND)
    {
        HRESULT hr = pInternalImport->GetCustomAttributeByName(pMeth->GetMemberDef(),
                                                               COR_SUPPRESS_UNMANAGED_CODE_CHECK_ATTRIBUTE_ANSI,
                                                               NULL,
                                                               NULL);
        _ASSERTE(SUCCEEDED(hr));
        if (hr == S_OK)
            dwDeclFlags &= ~DECLSEC_UNMNGD_ACCESS_DEMAND;
        else
        {
            if (pMeth->GetClass())
            {
                hr = pInternalImport->GetCustomAttributeByName(pMeth->GetClass()->GetCl(),
                                                               COR_SUPPRESS_UNMANAGED_CODE_CHECK_ATTRIBUTE_ANSI,
                                                               NULL,
                                                               NULL);
                _ASSERTE(SUCCEEDED(hr));
                if (hr == S_OK)
                    dwDeclFlags &= ~DECLSEC_UNMNGD_ACCESS_DEMAND;
            }
        }
        // Check if now there are no actions left
        if (dwDeclFlags == 0)
            return NULL;

        if (dwDeclFlags & DECLSEC_UNMNGD_ACCESS_DEMAND)
        {
            // A NDirect/Interop demand is required. 
            DeclActionInfo *temp = DeclActionInfo::Init(pMeth, DECLSEC_UNMNGD_ACCESS_DEMAND, NULL);
            if (!pDeclActions)
                pDeclActions = temp;
            else
            {
                temp->pNext = pDeclActions;
                pDeclActions = temp;
            }
        }
    } // if DECLSEC_UNMNGD_ACCESS_DEMAND

    // Look for the other actions
    mdToken         tk;
    PBYTE           pbPerm = NULL;
    ULONG           cbPerm = 0;
    void const **   ppData = const_cast<void const**> (reinterpret_cast<void**> (&pbPerm));
    mdPermission    tkPerm;
    DWORD           dwAction;
    HENUMInternal   hEnumDcl;
    OBJECTREF       refPermSet = NULL;
    int             loops = 2;
    DWORD           dwSetIndex;

    while (loops-- > 0)
    {
        if (loops == 1)
            tk = pMeth->GetMemberDef();
        else
            tk = pCl->GetCl();

        HRESULT hr = pInternalImport->EnumPermissionSetsInit(tk,
                                                     dclActionNil, // look up all actions
                                                     &hEnumDcl);

        if (hr != S_OK) // EnumInit returns S_FALSE if it didn't find any records.
        {
            continue;
        }

        while (pInternalImport->EnumNext(&hEnumDcl, &tkPerm))
        {
            pInternalImport->GetPermissionSetProps(tkPerm,
                                                   &dwAction,
                                                   ppData,
                                                   &cbPerm);

            // Only perform each action once.
            // Note that this includes "null" actions, where
            // the permission data is empty. This offers a
            // way to omit a global check.
            DWORD dwActionFlag = DclToFlag(dwAction);
            if (dwDeclFlags & dwActionFlag)
                dwDeclFlags &= ~dwActionFlag;
            else
                continue;

            // If there's nothing on which to perform an action,
            // skip it!
            if (! (cbPerm > 0 && pbPerm != NULL))
                continue;

            // Decode the permissionset and add this action to the linked list
            SecurityHelper::LoadPermissionSet(pbPerm,
                                              cbPerm,
                                              &refPermSet,
                                              NULL,
                                              &dwSetIndex);

            DeclActionInfo *temp = DeclActionInfo::Init(pMeth, dwActionFlag, dwSetIndex);
            if (!pDeclActions)
                pDeclActions = temp;
            else
            {
                temp->pNext = pDeclActions;
                pDeclActions = temp;
            }
        } // permission enum loop

        pInternalImport->EnumClose(&hEnumDcl);

    } // Method and class enum loop

    return pDeclActions;
}

extern LPVOID GetSecurityObjectForFrameInternal(StackCrawlMark *stackMark, INT32 create, OBJECTREF *pRefSecDesc);

inline void UpdateFrameSecurityObj(DWORD dwAction, OBJECTREF *refPermSet, OBJECTREF * pSecObj)
{
    FieldDesc *actionFld;   // depends on assert, deny, permitonly etc

    GetSecurityObjectForFrameInternal(NULL, true, pSecObj);
    actionFld = COMSecurityRuntime::GetFrameSecDescField(dwAction);

    _ASSERTE(actionFld && "Could not find a field inside FrameSecurityDescriptor. Renamed ?");
    actionFld->SetRefValue(*pSecObj, *refPermSet);
}

void InvokeDeclarativeActions (MethodDesc *pMeth, DeclActionInfo *pActions, OBJECTREF * pSecObj)
{
    OBJECTREF       refPermSet = NULL;
    ARG_SLOT           arg = 0;

    refPermSet = SecurityHelper::GetPermissionSet(pActions->dwSetIndex);
    _ASSERTE(refPermSet != NULL);

    // If we get a real PermissionSet, then invoke the action.
    if (refPermSet != NULL)
    {
        switch (pActions->dwDeclAction)
        {
        case DECLSEC_DEMANDS:
            COMCodeAccessSecurityEngine::DemandSet(refPermSet);
            break;
    
        case DECLSEC_ASSERTIONS:
            {
                Module * pModule = pMeth -> GetModule () ;
                AssemblySecurityDescriptor * pMSD = pModule -> GetSecurityDescriptor () ;

                GCPROTECT_BEGIN(refPermSet);
                // Check if this Assembly has permission to assert
                if (!pMSD->AssertPermissionChecked())
                {
                    if (!pMSD->IsFullyTrusted() && !pMSD->CheckSecurityPermission(SECURITY_ASSERT))
                        Security::ThrowSecurityException(g_SecurityPermissionClassName, SPFLAGSASSERTION);

                    pMSD->SetCanAssert();

                    pMSD->SetAssertPermissionChecked();
                }

                if (!pMSD->CanAssert())
                    Security::ThrowSecurityException(g_SecurityPermissionClassName, SPFLAGSASSERTION);

                // Now update the frame security object
                UpdateFrameSecurityObj(dclAssert, &refPermSet, pSecObj);
                GCPROTECT_END();
                break;
            }
    
        case DECLSEC_DENIALS:
            // Update the frame security object
            GCPROTECT_BEGIN(refPermSet);
            UpdateFrameSecurityObj(dclDeny, &refPermSet, pSecObj);
            Security::IncrementOverridesCount();
            GCPROTECT_END();
            break;
    
        case DECLSEC_PERMITONLY:
            // Update the frame security object
            GCPROTECT_BEGIN(refPermSet);
            UpdateFrameSecurityObj(dclPermitOnly, &refPermSet, pSecObj);
            Security::IncrementOverridesCount();
            GCPROTECT_END();
            break;

        case DECLSEC_NONCAS_DEMANDS:
            GCPROTECT_BEGIN(refPermSet);
            Security::InitSecurity();
            GCPROTECT_END();
            arg = ObjToArgSlot(refPermSet);
            Security::s_stdData.pMethPermSetDemand->Call(&arg, METHOD__PERMISSION_SET__DEMAND);
            break;

        default:
            _ASSERTE(!"Unknown action requested in InvokeDeclarativeActions");
            break;

        } // switch
    } // if refPermSet != null
}

void Security::DoDeclarativeActions(MethodDesc *pMeth, DeclActionInfo *pActions, LPVOID pSecObj)
{
    THROWSCOMPLUSEXCEPTION();

    // --------------------------------------------------------------------------- //
    //          D E C L A R A T I V E   S E C U R I T Y   D E M A N D S            //
    // --------------------------------------------------------------------------- //
    // The frame is now fully formed, arguments have been copied into place,
    // and synchronization monitors have been entered if necessary.  At this
    // point, we are prepared for something to throw an exception, so we may
    // check for declarative security demands and execute them.  We need a
    // well-formed frame and synchronization domain to accept security excep-
    // tions thrown by the SecurityManager.  We MAY need argument values in
    // the frame so that the arguments may be finalized if security throws an
    // exception across them (unknown).                          

    if (pActions->dwDeclAction == DECLSEC_UNMNGD_ACCESS_DEMAND && 
        pActions->pNext == NULL)
    {
        /* We special-case the security check on single pinvoke/interop calls
           so we can avoid setting up the GCFrame */

        COMCodeAccessSecurityEngine::SpecialDemand(SECURITY_UNMANAGED_CODE);
        return;
    }
    else
    {
        OBJECTREF secObj = NULL;
        GCPROTECT_BEGIN(secObj);

        for (/**/; pActions; pActions = pActions->pNext)
        {
            if (pActions->dwDeclAction == DECLSEC_UNMNGD_ACCESS_DEMAND)
            {
                COMCodeAccessSecurityEngine::SpecialDemand(SECURITY_UNMANAGED_CODE);
            }
            else
            {
                InvokeDeclarativeActions(pMeth, pActions, &secObj);
            }
        }

        // Required for InterceptorFrame::GetInterception() to work correctly
        _ASSERTE(*((Object**) pSecObj) == NULL);

        *((Object**) pSecObj) = OBJECTREFToObject(secObj);

        GCPROTECT_END();
    }
}

// This functions is logically part of the security stub
VOID __stdcall DoDeclarativeSecurity(MethodDesc *pMeth, DeclActionInfo *pActions, InterceptorFrame* frame)
{
    THROWSCOMPLUSEXCEPTION();

    LPVOID pSecObj = frame->GetAddrOfSecurityDesc();
    *((Object**) pSecObj) = NULL;
    Security::DoDeclarativeActions(pMeth, pActions, pSecObj);
}

OBJECTREF Security::ResolvePolicy(OBJECTREF evidence, OBJECTREF reqdPset, OBJECTREF optPset,
                                  OBJECTREF denyPset, OBJECTREF* grantdenied, int* grantIsUnrestricted)
{
    // If we got here, then we are going to do at least one security
    // check. Make sure security is initialized.

    struct _gc {
        OBJECTREF reqdPset;         // Required Requested Permissions
        OBJECTREF optPset;          // Optional Requested Permissions
        OBJECTREF denyPset;         // Denied Permissions
        OBJECTREF evidence;         // Object containing evidence
    } gc;
    ZeroMemory(&gc, sizeof(gc));
    gc.evidence = evidence;
    gc.reqdPset = reqdPset;
    gc.denyPset = denyPset;
    gc.optPset = optPset;

    GCPROTECT_BEGIN(gc);
    InitSecurity();
    GCPROTECT_END();

    ARG_SLOT args[6];
    args[0] = ObjToArgSlot(gc.evidence);
    args[1] = ObjToArgSlot(gc.reqdPset);
    args[2] = ObjToArgSlot(gc.optPset);
    args[3] = ObjToArgSlot(gc.denyPset);
    args[4] = PtrToArgSlot(grantdenied);
    args[5] = PtrToArgSlot(grantIsUnrestricted);

    return ArgSlotToObj(s_stdData.pMethResolvePolicy->Call(args, METHOD__SECURITY_MANAGER__RESOLVE_POLICY));
}

static BOOL IsCUIApp( PEFile* pFile )
{
    if (pFile == NULL)
        return FALSE;

    IMAGE_NT_HEADERS *pNTHeader = pFile->GetNTHeader();

    if (pNTHeader == NULL)
        return FALSE;

    // Sanity check we have a real header and didn't mess up this parsing.
    _ASSERTE((pNTHeader->Signature == VAL32(IMAGE_NT_SIGNATURE)) &&
        (pNTHeader->FileHeader.SizeOfOptionalHeader == VAL16(IMAGE_SIZEOF_NT_OPTIONAL_HEADER)) &&
        (pNTHeader->OptionalHeader.Magic == VAL16(IMAGE_NT_OPTIONAL_HDR_MAGIC)));
    
    return (pNTHeader->OptionalHeader.Subsystem & 
        VAL16(IMAGE_SUBSYSTEM_WINDOWS_CUI | IMAGE_SUBSYSTEM_OS2_CUI | IMAGE_SUBSYSTEM_POSIX_CUI));
}

static HRESULT HandleEarlyResolveException(Assembly *pAssembly, OBJECTREF *pThrowable)
{
    HRESULT hr;

    OBJECTREF orThrowable = NULL;
    GCPROTECT_BEGIN( orThrowable );
    orThrowable = GETTHROWABLE();
    hr = SecurityHelper::MapToHR(orThrowable);

    if (pThrowableAvailable(pThrowable)) {
        *pThrowable = orThrowable;
    } else {
        PEFile *pManifestFile = pAssembly->GetManifestFile();
        if (((pManifestFile != NULL) &&
                IsNilToken(VAL32(pManifestFile->GetCORHeader()->EntryPointToken))) ||
            SystemDomain::System()->DefaultDomain() != GetAppDomain()) {
#ifdef _DEBUG
            DefineFullyQualifiedNameForClassOnStack();
            LPUTF8 szClass = GetFullyQualifiedNameForClassNestedAware(orThrowable->GetClass());
            if (strcmp(g_SerializationExceptionName, szClass) == 0)
                printf("***** Error: failure in decode of permission requests\n" );
            else
                printf("***** Error: failed to grant minimum permissions\n");
#endif
            DEBUG_SAFE_TO_THROW_IN_THIS_BLOCK;
            COMPlusThrow(orThrowable);
        } else {
            // Print out the exception message ourselves.
            CQuickWSTRBase sMessage;
            LPCWSTR wszMessage = NULL;
            sMessage.Init();

            COMPLUS_TRY {
                GetExceptionMessage(orThrowable, &sMessage);
                wszMessage = sMessage.Ptr();
            }
            COMPLUS_CATCH {
            }
            COMPLUS_END_CATCH

            if (sMessage.Size() > 0)
                wszMessage = sMessage.Ptr();
            else
                wszMessage = L"<could not generate exception message>";

            if (pManifestFile != NULL && IsCUIApp( pManifestFile ))
                printf("%S\n", wszMessage);
            else
                WszMessageBoxInternal( NULL, wszMessage, NULL, MB_OK | MB_ICONERROR );

            sMessage.Destroy();
        }
    }
    GCPROTECT_END();

    return hr;
}

// Call this version of resolve against an assembly that's not yet fully loaded.
// It copes with exceptions due to minimal permission request grant or decodes
// failing and does the necessary magic to stop the debugger single stepping the
// managed code that gets invoked.
HRESULT Security::EarlyResolve(Assembly *pAssembly, AssemblySecurityDescriptor *pSecDesc, OBJECTREF *pThrowable)
{
    static const COMSecurityConfig::QuickCacheEntryType executionTable[] =
        { COMSecurityConfig::ExecutionZoneMyComputer,
          COMSecurityConfig::ExecutionZoneIntranet,
          COMSecurityConfig::ExecutionZoneInternet,
          COMSecurityConfig::ExecutionZoneTrusted,
          COMSecurityConfig::ExecutionZoneUntrusted };

    static const COMSecurityConfig::QuickCacheEntryType requestSkipVerificationTable[] =
        { COMSecurityConfig::RequestSkipVerificationZoneMyComputer,
          COMSecurityConfig::RequestSkipVerificationZoneIntranet,
          COMSecurityConfig::RequestSkipVerificationZoneInternet,
          COMSecurityConfig::RequestSkipVerificationZoneTrusted,
          COMSecurityConfig::RequestSkipVerificationZoneUntrusted };

    BOOL    fExecutionCheckEnabled = Security::IsExecutionPermissionCheckEnabled();
    BOOL    fRequest = SecurityHelper::PermissionsRequestedInAssembly(pAssembly);
    HRESULT hr = S_OK;
        
    if (fExecutionCheckEnabled || fRequest)
    {
        BEGIN_ENSURE_COOPERATIVE_GC();

        COMPLUS_TRY {
            DebuggerClassInitMarkFrame __dcimf;
                    
            if (fRequest)
            {
                // If we make a required request for skip verification
                // and nothing else, detect that and use the QuickCache

                PermissionRequestSpecialFlags specialFlags;
                OBJECTREF required, optional, refused;

                required = pSecDesc->GetRequestedPermissionSet( &optional, &refused, &specialFlags, FALSE );

                if (specialFlags.required == SkipVerification &&
                    (specialFlags.refused == NoSet || specialFlags.refused == EmptySet))
                {
                    if (!pSecDesc->CheckQuickCache( COMSecurityConfig::RequestSkipVerificationAll, requestSkipVerificationTable, CORSEC_SKIP_VERIFICATION ))
                    {
                        pSecDesc->Resolve();
                    }
                    else
                    {
                        // We have skip verification rights so now check
                        // for execution rights as needed.
                
                        if (fExecutionCheckEnabled)
                        {
                            if (!pSecDesc->CheckQuickCache( COMSecurityConfig::ExecutionAll, executionTable ))
                                pSecDesc->Resolve();
                        }
                    }
                }
              else
                {
                    pSecDesc->Resolve();
                }
            }
            else if (fExecutionCheckEnabled)
            {
                if (!pSecDesc->CheckQuickCache( COMSecurityConfig::ExecutionAll, executionTable ))
                    pSecDesc->Resolve();
            }
            else
            {
                pSecDesc->Resolve();
            }

            __dcimf.Pop();
        } COMPLUS_CATCH {
            hr = ::HandleEarlyResolveException(pAssembly, pThrowable);
        } COMPLUS_END_CATCH
       
        END_ENSURE_COOPERATIVE_GC();
    }

    return hr;
}

FCIMPL1(VOID, Security::Log, StringObject* msgUNSAFE)
{
    STRINGREF msg = (STRINGREF) msgUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_1(msg);
    //-[autocvtpro]-------------------------------------------------------

    WCHAR *p = NULL;
    int length = 0;
    CHAR *str = NULL;

    if (msg == NULL || msg->GetStringLength() <= 0)
    {
        goto lExit;
    }

    p = msg->GetBuffer();
    length = msg->GetStringLength();
    str = new (nothrow) CHAR[length + 1];
    
    if (str == NULL)
    {
        goto lExit;
    }

    for (int i=0; i<length; ++i)    // Wchar -> Char
        str[i] = (CHAR) p[i];       // This is only for debug.

    str[length] = 0;

    LOG((LF_SECURITY, LL_INFO10, str));

    delete [] str;

lExit: ;
    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

BOOL Security::_CanSkipVerification(Module *pModule, BOOL fLazy)
{
    _ASSERTE(pModule != NULL);

    if (IsSecurityOff())
        return TRUE;

    AssemblySecurityDescriptor *pSec = pModule->GetSecurityDescriptor();

    _ASSERTE(pSec);

    if (fLazy)
        return pSec->QuickCanSkipVerification();

    return pSec->CanSkipVerification();
}

BOOL Security::QuickCanSkipVerification(Module *pModule)
{
    _ASSERTE(pModule != NULL);

    if (IsSecurityOff())
        return TRUE;

    AssemblySecurityDescriptor *pSec = pModule->GetSecurityDescriptor();

    _ASSERTE(pSec);

    return pSec->QuickCanSkipVerification();
}


BOOL Security::CanCallUnmanagedCode(Module *pModule)
{
    _ASSERTE(pModule != NULL);

    if (IsSecurityOff())
        return TRUE;

    AssemblySecurityDescriptor *pSec = pModule->GetSecurityDescriptor();

    _ASSERTE(pSec);

    return pSec->CanCallUnmanagedCode();
}

BOOL Security::AppDomainCanCallUnmanagedCode(OBJECTREF *pThrowable)
{
    if (IsSecurityOff())
        return TRUE;

    Thread *pThread = GetThread();
    if (pThread == NULL)
        return TRUE;

    AppDomain *pDomain = pThread->GetDomain();
    if (pDomain == NULL)
        return TRUE;

    ApplicationSecurityDescriptor *pSecDesc = pDomain->GetSecurityDescriptor();
    if (pSecDesc == NULL)
        return TRUE;

    return pSecDesc->CanCallUnmanagedCode(pThrowable);
}

// Retrieve the public portion of a public/private key pair. The key pair is
// either exported (available as a byte array) or encapsulated in a Crypto API
// key container (identified by name).
FCIMPL4(Object*, Security::GetPublicKey, Object* pThisUNSAFE, BYTE bExported, U1Array* pArrayUNSAFE, StringObject* pContainerUNSAFE)
{
    OBJECTREF refRetVal = NULL;
    struct _gc
    {
        OBJECTREF pThis;
        U1ARRAYREF pArray;
        STRINGREF pContainer;
    } gc;
    gc.pThis = (OBJECTREF) pThisUNSAFE;
    gc.pArray = (U1ARRAYREF) pArrayUNSAFE;
    gc.pContainer = (STRINGREF) pContainerUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_NOPOLL(Frame::FRAME_ATTR_RETURNOBJ);
    GCPROTECT_BEGIN(gc);
    HELPER_METHOD_POLL();
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();

    LPWSTR      wszKeyContainer = NULL;
    DWORD       cchKeyContainer;
    BYTE       *pbKeyPair = NULL;
    DWORD       cbKeyPair = 0;
    BYTE       *pbPublicKey;
    DWORD       cbPublicKey;
    OBJECTREF   orOutputArray;

    // Read the arguments; either a byte array or a container name.
    if (bExported) {
        // Key pair provided directly in a byte array.
        cbKeyPair = gc.pArray->GetNumComponents();
        pbKeyPair = (BYTE*)_alloca(cbKeyPair);
        memcpy(pbKeyPair, gc.pArray->GetDataPtr(), cbKeyPair);
    } else {
        // Key pair referenced by key container name.
        cchKeyContainer = gc.pContainer->GetStringLength();
        wszKeyContainer = (LPWSTR)_alloca((cchKeyContainer + 1) * sizeof(WCHAR));
        memcpy(wszKeyContainer, gc.pContainer->GetBuffer(), cchKeyContainer * sizeof(WCHAR));
        wszKeyContainer[cchKeyContainer] = L'\0';
    }

    // Call the strong name routine to extract the public key. Need to switch
    // into GC pre-emptive mode for this call since it might perform a load
    // library (don't need to bother for the StrongNameFreeBuffer call later on).
    Thread *pThread = GetThread();
    pThread->EnablePreemptiveGC();
    BOOL fResult = StrongNameGetPublicKey(wszKeyContainer,
                                          pbKeyPair,
                                          cbKeyPair,
                                          &pbPublicKey,
                                          &cbPublicKey);
    pThread->DisablePreemptiveGC();
    if (!fResult)
        COMPlusThrow(kArgumentException, L"Argument_StrongNameGetPublicKey");

    // Translate the unmanaged byte array into managed form.
    SecurityHelper::CopyEncodingToByteArray(pbPublicKey, cbPublicKey, &orOutputArray);

    StrongNameFreeBuffer(pbPublicKey);

    refRetVal = orOutputArray;

    //-[autocvtepi]-------------------------------------------------------
    GCPROTECT_END();
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refRetVal);
}
FCIMPLEND

FCIMPL1(DWORD, Security::CreateFromUrl, StringObject* urlUNSAFE)
{
    DWORD dwZone = (DWORD) NoZone;

    STRINGREF url = (STRINGREF) urlUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, url);
    //-[autocvtpro]-------------------------------------------------------


    const WCHAR* rootFile = url->GetBuffer();
    
    if (rootFile != NULL)
    {
        dwZone = Security::QuickGetZone( (WCHAR*)rootFile );

    }

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return dwZone;
}
FCIMPLEND

DWORD Security::QuickGetZone( WCHAR* url )
{
    // If we aren't given an url, just early out.
    if (url == NULL)
        return (DWORD) NoZone;

    // A simple portable security zone mapping: 
    //  file URLs are LocalMachine, everything else is Internet
    return UrlIsW(url, URLIS_FILEURL) ? LocalMachine : Internet;
}


// Retrieve all linktime demands sets for a method. This includes both CAS and
// non-CAS sets for LDs at the class and the method level, so we could get up to
// four sets.
void Security::RetrieveLinktimeDemands(MethodDesc  *pMD,
                                       OBJECTREF   *pClassCas,
                                       OBJECTREF   *pClassNonCas,
                                       OBJECTREF   *pMethodCas,
                                       OBJECTREF   *pMethodNonCas)
{
    EEClass *pClass = pMD->GetClass();
    
    // Class level first.
    if (pClass->RequiresLinktimeCheck())
        *pClassCas = pClass->GetModule()->GetLinktimePermissions(pClass->GetCl(), pClassNonCas);

    // Then the method level.
    if (IsMdHasSecurity(pMD->GetAttrs()))
        *pMethodCas = pMD->GetModule()->GetLinktimePermissions(pMD->GetMemberDef(), pMethodNonCas);
}


// Used by interop to simulate the effect of link demands when the caller is
// in fact script constrained by an appdomain setup by IE.
void Security::CheckLinkDemandAgainstAppDomain(MethodDesc *pMD)
{
    THROWSCOMPLUSEXCEPTION();

    if (!pMD->RequiresLinktimeCheck())
        return;

    // Find the outermost (closest to caller) appdomain. This
    // represents the domain in which the unmanaged caller is
    // considered to "live" (or, at least, be constrained by).
    AppDomain *pDomain = GetThread()->GetInitialDomain();

    // The link check is only performed if this app domain has
    // security permissions associated with it, which will be
    // the case for all IE scripting callers that have got this
    // far because we automatically reported our managed classes
    // as "safe for scripting".
    ApplicationSecurityDescriptor *pSecDesc = pDomain->GetSecurityDescriptor();
    if (pSecDesc == NULL || pSecDesc->IsDefaultAppDomain())
        return;

    struct _gc
    {
        OBJECTREF refThrowable;
        OBJECTREF refGrant;
        OBJECTREF refDenied;
        OBJECTREF refClassNonCasDemands;
        OBJECTREF refClassCasDemands;
        OBJECTREF refMethodNonCasDemands;
        OBJECTREF refMethodCasDemands;
    } gc;
    ZeroMemory(&gc, sizeof(gc));

    GCPROTECT_BEGIN(gc);

    // Do a fulltrust check on the caller if the callee did not enable
    // AllowUntrustedCallerChecks. Pass a NULL caller assembly:
    // DoUntrustedCallerChecks needs to be able to cope with this.
    if (!Security::DoUntrustedCallerChecks(NULL, pMD, &gc.refThrowable, TRUE))
        COMPlusThrow(gc.refThrowable);

    // Fetch link demand sets from all the places in metadata where we might
    // find them (class and method). These might be split into CAS and non-CAS
    // sets as well.
    Security::RetrieveLinktimeDemands(pMD,
                                      &gc.refClassCasDemands,
                                      &gc.refClassNonCasDemands,
                                      &gc.refMethodCasDemands,
                                      &gc.refMethodNonCasDemands);

    if (gc.refClassCasDemands != NULL || gc.refMethodCasDemands != NULL)
    {
        // Get grant (and possibly denied) sets from the app
        // domain.
        gc.refGrant = pSecDesc->GetGrantedPermissionSet(&gc.refDenied);

        // Check one or both of the demands.
        if (gc.refClassCasDemands != NULL)
            COMCodeAccessSecurityEngine::CheckSetHelper(&gc.refClassCasDemands,
                                                        &gc.refGrant,
                                                        &gc.refDenied,
                                                        pDomain);

        if (gc.refMethodCasDemands != NULL)
            COMCodeAccessSecurityEngine::CheckSetHelper(&gc.refMethodCasDemands,
                                                        &gc.refGrant,
                                                        &gc.refDenied,
                                                        pDomain);
    }

    // Non-CAS demands are not applied against a grant
    // set, they're standalone.
    if (gc.refClassNonCasDemands != NULL)
        CheckNonCasDemand(&gc.refClassNonCasDemands);

    if (gc.refMethodNonCasDemands != NULL)
        CheckNonCasDemand(&gc.refMethodNonCasDemands);
 
    // We perform automatic linktime checks for UnmanagedCode in three cases:
    //   o  P/Invoke calls (shouldn't get these here, but let's be paranoid).
    //   o  Calls through an interface that have a suppress runtime check
    //      attribute on them (these are almost certainly interop calls).
    //   o  Interop calls made through method impls.
    // Just walk the stack in these cases, they'll be extremely rare and the
    // perf delta isn't that huge.
    if (pMD->IsNDirect() ||
        (pMD->GetClass()->IsInterface() &&
         pMD->GetClass()->GetMDImport()->GetCustomAttributeByName(pMD->GetClass()->GetCl(),
                                                                  COR_SUPPRESS_UNMANAGED_CODE_CHECK_ATTRIBUTE_ANSI,
                                                                  NULL,
                                                                  NULL) == S_OK) ||
        (pMD->IsComPlusCall() && !pMD->IsInterface()))
        COMCodeAccessSecurityEngine::SpecialDemand(SECURITY_UNMANAGED_CODE);

    GCPROTECT_END();
}

FCIMPL1(BOOL, Security::LocalDrive, StringObject* pathUNSAFE)
{
    return TRUE;
}
FCIMPLEND

FCIMPL1(Object*, Security::EcallGetLongPathName, StringObject* shortPathUNSAFE)
{
    STRINGREF refRetVal = NULL;
    STRINGREF shortPath = (STRINGREF) shortPathUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, shortPath);
    //-[autocvtpro]-------------------------------------------------------

    WCHAR* wszShortPath = shortPath->GetBuffer();
    WCHAR wszBuffer[MAX_PATH];

    DWORD size = Security::GetLongPathName( wszShortPath, wszBuffer, MAX_PATH );

    if (size == 0)
    {
        // We have to deal with files that do not exist so just
        // because GetLongPathName doesn't give us anything doesn't
        // mean that we can give up.  We iterate through the input
        // trying GetLongPathName on every subdirectory until
        // it succeeds or we run out of string.

        WCHAR wszIntermediateBuffer[MAX_PATH];

        if (wcslen( wszShortPath ) >= MAX_PATH)
        {
            refRetVal = shortPath;
            goto lExit;
        }

        wcscpy( wszIntermediateBuffer, wszShortPath );

        size_t index = wcslen( wszIntermediateBuffer );

        do
        {
            while (index > 0 && wszIntermediateBuffer[index-1] != L'\\')
                --index;

            if (index == 0)
                break;

            wszIntermediateBuffer[index-1] = L'\0';

            size = Security::GetLongPathName( wszIntermediateBuffer, wszBuffer, MAX_PATH );

            if (size != 0)
            {
                size_t sizeBuffer = wcslen( wszBuffer );

                if (sizeBuffer + wcslen( &wszIntermediateBuffer[index] ) > MAX_PATH - 2)
                {
                    refRetVal = shortPath;
                    goto lExit;
                }
                else
                {
                    if (wszBuffer[sizeBuffer-1] != L'\\')
                        wcscat( wszBuffer, L"\\" );
                    wcscat( wszBuffer, &wszIntermediateBuffer[index] );
                    refRetVal = COMString::NewString( wszBuffer );
                    goto lExit;
                }
            }
        }
        while( true );

        refRetVal = shortPath;
        goto lExit;
    }
    else if (size > MAX_PATH)
    {
        refRetVal = shortPath;
        goto lExit;
    }
    else
    {
        refRetVal = COMString::NewString( wszBuffer );
        goto lExit;
    }

lExit: ;
    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refRetVal);
}
FCIMPLEND



FCIMPL1(Object*, Security::GetDeviceName, StringObject* driveLetterUNSAFE)
{
    return NULL;
}
FCIMPLEND

///////////////////////////////////////////////////////////////////////////////
//
//  [SecurityDescriptor]
//  |
//  |
//  +----[ApplicationSecurityDescriptor]
//  |
//  |
//  +----[AssemblySecurityDescriptor]
//
///////////////////////////////////////////////////////////////////////////////

BOOL SecurityDescriptor::IsFullyTrusted( BOOL lazy )
{
    BOOL bIsFullyTrusted = FALSE;

    static const COMSecurityConfig::QuickCacheEntryType fullTrustTable[] =
        { COMSecurityConfig::FullTrustZoneMyComputer,
          COMSecurityConfig::FullTrustZoneIntranet,
          COMSecurityConfig::FullTrustZoneInternet,
          COMSecurityConfig::FullTrustZoneTrusted,
          COMSecurityConfig::FullTrustZoneUntrusted };

    if (Security::IsSecurityOff())
        return TRUE;

    // If this is an AppDomain with no zone set, we can assume full trust
    // without having to initialize security or resolve policy.
    if (GetProperties(CORSEC_DEFAULT_APPDOMAIN))
        return TRUE;

    BOOL fullTrustFlagSet = GetProperties(CORSEC_FULLY_TRUSTED);

    if (fullTrustFlagSet || GetProperties(CORSEC_RESOLVED))
        return fullTrustFlagSet;

    if (CheckQuickCache( COMSecurityConfig::FullTrustAll, fullTrustTable ))
    {
        PermissionRequestSpecialFlags specialFlags;
        OBJECTREF required, optional, refused;

        // We need to make sure the request doesn't take away what the grant would be.
        COMPLUS_TRY
        {
            required = this->GetRequestedPermissionSet( &optional, &refused, &specialFlags, FALSE );

            if ((specialFlags.optional == NoSet) &&
                (specialFlags.refused == NoSet || specialFlags.refused == EmptySet))
            {   // fall through finally, since returning in try is expensive
                SetProperties(CORSEC_FULLY_TRUSTED);
                bIsFullyTrusted = TRUE;
            }
        }
        COMPLUS_CATCH
        {
            // If we throw an exception in the above, just fall through (since
            // using the cache is optional anyway).
        }
        COMPLUS_END_CATCH
        if (bIsFullyTrusted)
            return TRUE;
    }

    if (!lazy)
    {
        // Resolve() operation need to be done before we can determine
        // if this is fully trusted

        TryResolve();
    }

    return GetProperties(CORSEC_FULLY_TRUSTED);
}

OBJECTREF SecurityDescriptor::GetGrantedPermissionSet(OBJECTREF* DeniedPermissions)
{
    THROWSCOMPLUSEXCEPTION();

    COMPLUS_TRY
    {
        // Resolve() operation need to be done before we can get the
        // granted PermissionSet
        Resolve();
    }
    COMPLUS_CATCH
    {
        OBJECTREF pThrowable = GETTHROWABLE();
        DefineFullyQualifiedNameForClassOnStack();
        LPUTF8 szClass = GetFullyQualifiedNameForClass(pThrowable->GetClass());
        if ((strcmp(g_ThreadAbortExceptionClassName, szClass) == 0) ||
            (strcmp(g_AppDomainUnloadedExceptionClassName, szClass) == 0))
        {
            COMPlusThrow(pThrowable);
        }
        else
        {
            // We don't really expect an exception here.
            _ASSERTE(FALSE);
            // but if we do get an exception, we need to rethrow
            COMPlusThrow(pThrowable);
        }
    }
    COMPLUS_END_CATCH

    OBJECTREF pset = NULL;

    pset = ObjectFromHandle(m_hGrantedPermissionSet);

    if (pset == NULL)
    {
        // We're going to create a new permission set and we might be called in
        // the wrong context. Always create the object in the context of the
        // descriptor, the caller must be able to cope with this if they're
        // outside that context.
        bool fSwitchContext = false;
        ContextTransitionFrame frame;
        Thread *pThread = GetThread();
        if (m_pAppDomain != GetAppDomain() && !IsSystem())
        {
            fSwitchContext = true;
            pThread->EnterContextRestricted(m_pAppDomain->GetDefaultContext(), &frame, TRUE);
        }

        if ((GetProperties(CORSEC_FULLY_TRUSTED) != 0) || 
            Security::IsSecurityOff())
        {
            // MarkAsFullyTrusted() call could set FULLY_TRUSTED flag without
            // setting the granted permissionSet.

            pset = SecurityHelper::CreatePermissionSet(TRUE);
        }
        else
        {
            // This could happen if Resolve was unable to obtain an
            // IdentityInfo.

            pset = SecurityHelper::CreatePermissionSet(FALSE);
            SetGrantedPermissionSet(pset, NULL);
        }

        if (fSwitchContext)
            pThread->ReturnToContext(&frame, TRUE);

        *DeniedPermissions = NULL;
    } else
        *DeniedPermissions = ObjectFromHandle(m_hGrantDeniedPermissionSet);

    return pset;
}

BOOL SecurityDescriptor::CheckQuickCache( COMSecurityConfig::QuickCacheEntryType all, const COMSecurityConfig::QuickCacheEntryType* zoneTable, DWORD successFlags )
{
    BOOL bFallThroughFinally = FALSE;

    if (Security::IsSecurityOff())
        return TRUE;

    if (!SecurityDescriptor::QuickCacheEnabled())
        return FALSE;

    if (GetProperties(CORSEC_SYSTEM_CLASSES) != 0 || GetProperties(CORSEC_DEFAULT_APPDOMAIN) != 0)
        return TRUE;

    if (!m_pAppDomain->GetSecurityDescriptor()->QuickCacheEnabled())
        return FALSE;

    BEGIN_ENSURE_COOPERATIVE_GC();

    COMPLUS_TRY
    {
        if (((APPDOMAINREF)m_pAppDomain->GetExposedObject())->HasSetPolicy())
        {   // returning before falling through finally is expensive, so avoid in common path situations
            bFallThroughFinally = TRUE;
            goto FallThroughFinally;
        }

        _ASSERTE( this->m_hAdditionalEvidence != NULL );

        if (ObjectFromHandle( this->m_hAdditionalEvidence ) != NULL)
        {   // returning before falling through finally is expensive, so avoid in common path situations
            bFallThroughFinally = TRUE;
            goto FallThroughFinally;
        }

        Security::InitData();
FallThroughFinally:;
    }
    COMPLUS_CATCH
    {
        bFallThroughFinally = TRUE;
    }
    COMPLUS_END_CATCH

    END_ENSURE_COOPERATIVE_GC();

    if (bFallThroughFinally)
        return FALSE;
        
    BOOL machine, user, enterprise;

    // First, check the quick cache for the all case.

    machine = COMSecurityConfig::GetQuickCacheEntry( COMSecurityConfig::MachinePolicyLevel, all );
    user = COMSecurityConfig::GetQuickCacheEntry( COMSecurityConfig::UserPolicyLevel, all );
    enterprise = COMSecurityConfig::GetQuickCacheEntry( COMSecurityConfig::EnterprisePolicyLevel, all );

    if (machine && user && enterprise)
    {
        SetProperties( successFlags );
        return TRUE;
    }
        
    // If we can't match for all, try for our zone.
    
    DWORD zone = GetZone();

    if (zone == 0xFFFFFFFF)
        return FALSE;

    machine = COMSecurityConfig::GetQuickCacheEntry( COMSecurityConfig::MachinePolicyLevel, zoneTable[zone] );
    user = COMSecurityConfig::GetQuickCacheEntry( COMSecurityConfig::UserPolicyLevel, zoneTable[zone] );
    enterprise = COMSecurityConfig::GetQuickCacheEntry( COMSecurityConfig::EnterprisePolicyLevel, zoneTable[zone] );
    
    if (machine && user && enterprise)
    {
        SetProperties( successFlags );
        return TRUE;
    }
        
    return FALSE;
}    
    
    
void SecurityDescriptor::Resolve()
{
    THROWSCOMPLUSEXCEPTION();

    if (Security::IsSecurityOff())
        return;

    if (GetProperties(CORSEC_RESOLVED) != 0)
        return;

    // For assembly level resolution we go through a synchronization process
    // (since we can have a single assembly loaded into multiple appdomains, but
    // must compute the same grant set for all).
    if (m_pAssem) {
        AssemblySecurityDescriptor *pASD = (AssemblySecurityDescriptor*)this;
        pASD->GetSharedSecDesc()->Resolve(pASD);
    } else
        ResolveWorker();
}


void SecurityDescriptor::TryResolve()
{
    COMPLUS_TRY
    {
        Resolve();
    }
    COMPLUS_CATCH
    {
    }
    COMPLUS_END_CATCH
}

void SecurityDescriptor::ResolveWorker()
{
    THROWSCOMPLUSEXCEPTION();

    struct _gc {
        OBJECTREF reqdPset;         // Required Requested Permissions
        OBJECTREF optPset;          // Optional Requested Permissions
        OBJECTREF denyPset;         // Denied Permissions
        OBJECTREF evidence;         // Object containing evidence
        OBJECTREF granted;          // Policy based Granded Permission
        OBJECTREF grantdenied;      // Policy based explicitly Denied Permissions
    } gc;
    ZeroMemory(&gc, sizeof(gc));

    // In the following code we must avoid using m_pAppDomain if we're a system
    // assembly descriptor. That's because only one descriptor is allocated for
    // such assemblies and the appdomain is arbitrary. We special case system
    // assemblies enough so that this doesn't matter.
    // Note that IsSystem() automatically checks the difference between
    // appdomain and assembly descriptors (it's always false for appdomains).

    // Turn the security demand optimization off for the duration of the resolve,
    // to prevent some recursive resolves
    ApplicationSecurityDescriptor *pAppSecDesc = NULL;
    if (!IsSystem())
    {
        pAppSecDesc = m_pAppDomain->GetSecurityDescriptor();
        if (pAppSecDesc)
            pAppSecDesc->DisableOptimization();
    }

    // Resolve is one of the few SecurityDescriptor routines that may be called
    // from the wrong appdomain context. If that's the case we will transition
    // into the correct appdomain for the duration of the call.
    bool fSwitchContext = false;
    ContextTransitionFrame frame;
    Thread *pThread = GetThread();
    if (m_pAppDomain != GetAppDomain() && !IsSystem())
    {
        fSwitchContext = true;
        pThread->EnterContextRestricted(m_pAppDomain->GetDefaultContext(), &frame, TRUE);
    }

    // If we got here, then we are going to do at least one security check.
    // Make sure security is initialized.

    BEGIN_ENSURE_COOPERATIVE_GC();

    GCPROTECT_BEGIN(gc);

    // Short circuit system classes assembly to avoid recursion.
    // Also, AppDomains which weren't created with any input evidence are fully
    // trusted.
    if (GetProperties(CORSEC_SYSTEM_CLASSES) != 0 || GetProperties(CORSEC_DEFAULT_APPDOMAIN) != 0)
    {
        // Create the unrestricted permission set.
        if (!IsSystem())
        {
            gc.granted = SecurityHelper::CreatePermissionSet(TRUE);
            SetGrantedPermissionSet(gc.granted, NULL);
        }
        MarkAsFullyTrusted();
    }
    else
    {
        Security::InitSecurity();

        if (GetProperties(CORSEC_EVIDENCE_COMPUTED))
            gc.evidence = ObjectFromHandle(m_hAdditionalEvidence);
        else
            gc.evidence = GetEvidence();

        gc.reqdPset = GetRequestedPermissionSet(&gc.optPset, &gc.denyPset);

        COMPLUS_TRY {
            int grantIsUnrestricted;
            gc.granted = Security::ResolvePolicy(gc.evidence, gc.reqdPset, gc.optPset, gc.denyPset, &gc.grantdenied, &grantIsUnrestricted);
            if (grantIsUnrestricted)
                MarkAsFullyTrusted();
        } COMPLUS_CATCH {
            // In the specific case of a PolicyException, we have failed to
            // grant the required part of an explicit request. In this case
            // we need to rethrow the exception to trigger the assembly load
            // to fail.
            OBJECTREF pThrowable = GETTHROWABLE();
            DefineFullyQualifiedNameForClassOnStack();
            LPUTF8 szClass = GetFullyQualifiedNameForClass(pThrowable->GetClass());
            if (strcmp(g_PolicyExceptionClassName, szClass) == 0)
            {
                if (pAppSecDesc)
                    pAppSecDesc->EnableOptimization();
                COMPlusThrow(pThrowable);
            }
            else if ((strcmp(g_ThreadAbortExceptionClassName, szClass) == 0) ||
                     (strcmp(g_AppDomainUnloadedExceptionClassName, szClass) == 0))
            {
                COMPlusThrow(pThrowable);
            }
            gc.granted = SecurityHelper::CreatePermissionSet(FALSE);
        } COMPLUS_END_CATCH

        SetGrantedPermissionSet(gc.granted, gc.grantdenied);

    }

    GCPROTECT_END();

    END_ENSURE_COOPERATIVE_GC();

    if (fSwitchContext)
        pThread->ReturnToContext(&frame, TRUE);

    // Turn the security demand optimization on again
    if (pAppSecDesc)
        pAppSecDesc->EnableOptimization();
}

OBJECTREF SecurityDescriptor::GetRequestedPermissionSet(OBJECTREF *pOptionalPermissionSet,
                                                        OBJECTREF *pDeniedPermissionSet,
                                                        PermissionRequestSpecialFlags *pSpecialFlags,
                                                        BOOL fCreate)
{
    _ASSERTE( Security::IsSecurityOn() && "This method should not be called if security is not on" );

    SEC_CHECKCONTEXT();

    OBJECTREF RequestedPermission = NULL;

    RequestedPermission = ObjectFromHandle(m_hRequiredPermissionSet);
    *pOptionalPermissionSet = ObjectFromHandle(m_hOptionalPermissionSet);
    *pDeniedPermissionSet = ObjectFromHandle(m_hDeniedPermissionSet);

    if (pSpecialFlags != NULL)
        *pSpecialFlags = m_SpecialFlags;

    return RequestedPermission;
}

OBJECTREF ApplicationSecurityDescriptor::GetEvidence()
{
    SEC_CHECKCONTEXT();

    OBJECTREF   evidence = NULL;
    OBJECTREF   retval = NULL;

    Security::InitSecurity();

    struct _gc {
        OBJECTREF rootAssemblyEvidence;
        OBJECTREF additionalEvidence;
    } gc;
    ZeroMemory(&gc, sizeof(_gc));

    GCPROTECT_BEGIN(gc);

    if (m_pAppDomain->m_pRootAssembly != NULL)
    {
        gc.rootAssemblyEvidence = m_pAppDomain->m_pRootAssembly->GetSecurityDescriptor()->GetEvidence();
    } 

    OBJECTREF orAppDomain = m_pAppDomain->GetExposedObject();

    gc.additionalEvidence = ObjectFromHandle(m_hAdditionalEvidence);

    ARG_SLOT args[] = {
        ObjToArgSlot(orAppDomain),
        ObjToArgSlot(gc.rootAssemblyEvidence),
        ObjToArgSlot(gc.additionalEvidence),
    };

    evidence = ArgSlotToObj(Security::s_stdData.pMethAppDomainCreateSecurityIdentity->Call(args, 
                                                                                         METHOD__APP_DOMAIN__CREATE_SECURITY_IDENTITY));

    if (gc.additionalEvidence == NULL)
    {
        SetProperties(CORSEC_DEFAULT_APPDOMAIN);
        retval = NULL;
    }
    else
    {
        retval = evidence;
    }

    GCPROTECT_END();

    return retval;
}


DWORD ApplicationSecurityDescriptor::GetZone()
{
    DWORD dwZone = (DWORD) NoZone; 

    if ((m_pAppDomain == SystemDomain::System()->DefaultDomain()) &&
        m_pAppDomain->m_pRootFile)
    {
        const WCHAR* rootFile = m_pAppDomain->m_pRootFile->GetFileName();
        
        if (rootFile != NULL)
        {
            dwZone = Security::QuickGetZone( (WCHAR*)rootFile );

        }
    } 

    return dwZone;
}


OBJECTREF AssemblySecurityDescriptor::GetRequestedPermissionSet(OBJECTREF *pOptionalPermissionSet,
                                                                OBJECTREF *pDeniedPermissionSet,
                                                                PermissionRequestSpecialFlags *pSpecialFlags,
                                                                BOOL fCreate)
{
    _ASSERTE( Security::IsSecurityOn() && "This method should not be called if security is not on" );

    if (fCreate)
        SEC_CHECKCONTEXT();

    OBJECTREF req = NULL;

    req = ObjectFromHandle(m_hRequiredPermissionSet);

    if (req == NULL)
    {
        // Try to load permission requests from assembly first.
        SecurityHelper::LoadPermissionRequestsFromAssembly(m_pAssem,
                                                           &req,
                                                           pOptionalPermissionSet,
                                                           pDeniedPermissionSet,
                                                           &m_SpecialFlags,
                                                           fCreate);

        if (pSpecialFlags != NULL)
            *pSpecialFlags = m_SpecialFlags;
        if (fCreate)
            SetRequestedPermissionSet(req, *pOptionalPermissionSet, *pDeniedPermissionSet);
    }
    else
    {
        *pOptionalPermissionSet = ObjectFromHandle(m_hOptionalPermissionSet);
        *pDeniedPermissionSet = ObjectFromHandle(m_hDeniedPermissionSet);
        if (pSpecialFlags)
            *pSpecialFlags = m_SpecialFlags;
    }

    return req;
}

// Checks if the granted permission set has a security permission
// using stored Permission Object instances.
BOOL SecurityDescriptor::CheckSecurityPermission(int index)
{
    _ASSERTE( Security::IsSecurityOn() && "This method should not be called if security is not on" );

    SEC_CHECKCONTEXT();

    BOOL    fRet = FALSE;

    BEGIN_ENSURE_COOPERATIVE_GC();

    COMPLUS_TRY
    {

        // Check for Security.SecurityPermission.SkipVerification
        struct _gc {
            OBJECTREF granted;
            OBJECTREF denied;
            OBJECTREF perm;
        } gc;
        ZeroMemory(&gc, sizeof(_gc));

        GCPROTECT_BEGIN(gc);

        Security::_GetSharedPermissionInstance(&gc.perm, index);
            if (gc.perm == NULL)
            {
                _ASSERTE(FALSE);
                goto Exit;
            }

        // This will call Resolve(), that inturn calls InitSecurity()
        gc.granted = GetGrantedPermissionSet(&gc.denied);

        // Denied permission set should NOT contain
        // SecurityPermission.xxx

        if (gc.denied != NULL)
        {
            // s_stdData.pMethPermSetContains needs to be initialised.
            _ASSERTE(Security::IsInitialized());

        ARG_SLOT arg[2] =
            {
            ObjToArgSlot(gc.denied),
            ObjToArgSlot(gc.perm),
            };

            if ((BOOL)(Security::s_stdData.pMethPermSetContains->Call(arg, METHOD__PERMISSION_SET__CONTAINS)))
                goto Exit;
        }

        // Granted permission set should contain
        // SecurityPermission.SkipVerification

        if (gc.granted != NULL)
        {
            // s_stdData.pMethPermSetContains needs to be initialised.
            _ASSERTE(Security::IsInitialized());

        ARG_SLOT arg[2] =
            {
            ObjToArgSlot(gc.granted),
            ObjToArgSlot(gc.perm),
            };

            if ((BOOL)(Security::s_stdData.pMethPermSetContains->Call(arg, METHOD__PERMISSION_SET__CONTAINS)))
                            fRet = TRUE;
        }

Exit:
        GCPROTECT_END();
    }
    COMPLUS_CATCH
    {
        _ASSERTE(FALSE);
        fRet = FALSE;
    }
    COMPLUS_END_CATCH

    END_ENSURE_COOPERATIVE_GC();

    return fRet;
}

static void CreateSecurityException(char *szDemandClass, DWORD dwFlags, OBJECTREF *pThrowable)
{
    if (pThrowable == RETURN_ON_ERROR)
        return;

    COMPLUS_TRY
    {
        Security::ThrowSecurityException(g_SecurityPermissionClassName, SPFLAGSUNMANAGEDCODE);
    }
    COMPLUS_CATCH
    {
        UpdateThrowable(pThrowable);
    }        
    COMPLUS_END_CATCH
}

BOOL SecurityDescriptor::CanCallUnmanagedCode(OBJECTREF *pThrowable)
{
    BOOL bCallUnmanagedCode = FALSE;

    static const COMSecurityConfig::QuickCacheEntryType unmanagedTable[] =
        { COMSecurityConfig::UnmanagedZoneMyComputer,
          COMSecurityConfig::UnmanagedZoneIntranet,
          COMSecurityConfig::UnmanagedZoneInternet,
          COMSecurityConfig::UnmanagedZoneTrusted,
          COMSecurityConfig::UnmanagedZoneUntrusted };

    if (Security::IsSecurityOff())
        return TRUE;

    if (LazyCanCallUnmanagedCode())
        return TRUE;

    if (CheckQuickCache( COMSecurityConfig::UnmanagedAll, unmanagedTable ))
    {
        PermissionRequestSpecialFlags specialFlags;
        OBJECTREF required, optional, refused;

        // We need to make sure the request doesn't take away what the grant would be.
        COMPLUS_TRY
        {
            required = this->GetRequestedPermissionSet( &optional, &refused, &specialFlags, FALSE );

            if ((specialFlags.optional == NoSet) &&
                (specialFlags.refused == NoSet || specialFlags.refused == EmptySet))
            {   // fall through finally, since returning in try is expensive
                SetProperties(CORSEC_CALL_UNMANAGEDCODE);
                bCallUnmanagedCode = TRUE;
            }
        }
        COMPLUS_CATCH
        {
            // If we throw an exception in the above, just fall through (since
            // using the cache is optional anyway).
        }
        COMPLUS_END_CATCH
        if (bCallUnmanagedCode)
            return TRUE;
    }

    // IsFullyTrusted will cause a Resolve() if not already done.

    if (IsFullyTrusted())
    {
        SetProperties(CORSEC_CALL_UNMANAGEDCODE);
        return TRUE;
    }

    if (CheckSecurityPermission(SECURITY_UNMANAGED_CODE))
    {
        SetProperties(CORSEC_CALL_UNMANAGEDCODE);
        return TRUE;
    }

    ::CreateSecurityException(g_SecurityPermissionClassName, SPFLAGSUNMANAGEDCODE, pThrowable);
    return FALSE;
}

BOOL SecurityDescriptor::CanRetrieveTypeInformation()
{
    if (Security::IsSecurityOff())
        return TRUE;

    if (IsFullyTrusted())
    {
        SetProperties(CORSEC_TYPE_INFORMATION);
        return TRUE;
    }

    if (CheckSecurityPermission(REFLECTION_TYPE_INFO))
    {
        SetProperties(CORSEC_TYPE_INFORMATION);
        return TRUE;
    }

    return FALSE;
}

BOOL SecurityDescriptor::CanSkipVerification()
{
    BOOL bSkipVerification = FALSE;

    static const COMSecurityConfig::QuickCacheEntryType skipVerificationTable[] =
        { COMSecurityConfig::SkipVerificationZoneMyComputer,
          COMSecurityConfig::SkipVerificationZoneIntranet,
          COMSecurityConfig::SkipVerificationZoneInternet,
          COMSecurityConfig::SkipVerificationZoneTrusted,
          COMSecurityConfig::SkipVerificationZoneUntrusted };


    _ASSERTE( Security::IsSecurityOn() && "This method should not be called if security is not on" );

    if (LazyCanSkipVerification())
        return TRUE;

    if (CheckQuickCache( COMSecurityConfig::SkipVerificationAll, skipVerificationTable ))
    {
        PermissionRequestSpecialFlags specialFlags;
        OBJECTREF required, optional, refused;

        // We need to make sure the request doesn't take away what the grant would be.
        COMPLUS_TRY
        {
            required = this->GetRequestedPermissionSet( &optional, &refused, &specialFlags, FALSE );

            if ((specialFlags.optional == NoSet) &&
                (specialFlags.refused == NoSet || specialFlags.refused == EmptySet))
            {   // fall through finally, since returning in try is expensive
                SetProperties(CORSEC_SKIP_VERIFICATION);
                bSkipVerification = TRUE;
            }
        }
        COMPLUS_CATCH
        {
            // If we throw an exception in the above, just fall through (since
            // using the cache is optional anyway).
        }
        COMPLUS_END_CATCH
        if (bSkipVerification)
            return TRUE;
    }

    // IsFullyTrusted will cause a Resolve() if not already done.

    if (IsFullyTrusted())
    {
        SetProperties(CORSEC_SKIP_VERIFICATION);
        return TRUE;
    }

    if (CheckSecurityPermission(SECURITY_SKIP_VER))
    {
        SetProperties(CORSEC_SKIP_VERIFICATION);
        return TRUE;
    }

    return FALSE;
}

BOOL SecurityDescriptor::QuickCanSkipVerification()
{
    BOOL bSkipVerification = FALSE;
    
    static const COMSecurityConfig::QuickCacheEntryType skipVerificationTable[] =
        { COMSecurityConfig::SkipVerificationZoneMyComputer,
          COMSecurityConfig::SkipVerificationZoneIntranet,
          COMSecurityConfig::SkipVerificationZoneInternet,
          COMSecurityConfig::SkipVerificationZoneTrusted,
          COMSecurityConfig::SkipVerificationZoneUntrusted };


    _ASSERTE( Security::IsSecurityOn() && "This method should not be called if security is not on" );

    if (LazyCanSkipVerification())
        return TRUE;

    if (CheckQuickCache( COMSecurityConfig::SkipVerificationAll, skipVerificationTable ))
    {
        PermissionRequestSpecialFlags specialFlags;
        OBJECTREF required, optional, refused;

        // We need to make sure the request doesn't take away what the grant would be.
        COMPLUS_TRY
        {
            required = this->GetRequestedPermissionSet( &optional, &refused, &specialFlags, FALSE );

            if ((specialFlags.optional == NoSet) &&
                (specialFlags.refused == NoSet || specialFlags.refused == EmptySet))
            {   // fall through finally, since returning in try is expensive
                SetProperties(CORSEC_SKIP_VERIFICATION);
                bSkipVerification = TRUE;
            }
        }
        COMPLUS_CATCH
        {
            // If we throw an exception in the above, just fall through (since
            // using the cache is optional anyway).
        }
        COMPLUS_END_CATCH
        if (bSkipVerification)
            return TRUE;
    }

    return FALSE;
}

OBJECTREF AssemblySecurityDescriptor::GetSerializedEvidence()
{
    DWORD cbResource;
    U1ARRAYREF serializedEvidence;
    PBYTE pbInMemoryResource = NULL;

    SEC_CHECKCONTEXT();

    // Get the resource, and associated file handle, from the assembly.
    if (FAILED(m_pAssem->GetResource(s_strSecurityEvidence, NULL,
                                     &cbResource, &pbInMemoryResource,
                                     NULL, NULL, NULL)))
        return NULL;

    // Allocate the proper size unsigned byte array.
    serializedEvidence = (U1ARRAYREF)AllocatePrimitiveArray(ELEMENT_TYPE_U1, cbResource);

    memcpyNoGCRefs(serializedEvidence->GetDirectPointerToNonObjectElements(),
               pbInMemoryResource, cbResource);

    // Successfully read all data, set the allocated array as the return value.
    return (OBJECTREF) serializedEvidence;
}

// Gather all raw materials to construct evidence and punt them up to the managed
// assembly, which constructs the actual Evidence object and returns it (as well
// as caching it).
OBJECTREF AssemblySecurityDescriptor::GetEvidence()
{
    HRESULT     hr;
    OBJECTREF   evidence = NULL;
    LPWSTR      wszCodebase = NULL;
    DWORD       dwZone = (DWORD) NoZone;
    BYTE        rbUniqueID[MAX_SIZE_SECURITY_ID];
    DWORD       cbUniqueID = sizeof(rbUniqueID);

    SEC_CHECKCONTEXT();

    Security::InitSecurity();

    hr = m_pAssem->GetSecurityIdentity(&wszCodebase, &dwZone, rbUniqueID, &cbUniqueID);
    _ASSERTE(SUCCEEDED(hr));

    if (FAILED(hr))
    {
        dwZone = (DWORD) NoZone;
        wszCodebase = NULL;
    }

    hr = LoadSignature();
    _ASSERTE(SUCCEEDED(hr));

    struct _gc {
        STRINGREF url;
        OBJECTREF uniqueID;
        OBJECTREF cert;
        OBJECTREF serializedEvidence;
        OBJECTREF additionalEvidence;
    } gc;
    ZeroMemory(&gc, sizeof(_gc));

    GCPROTECT_BEGIN(gc);

    if (wszCodebase != NULL && *wszCodebase)
        gc.url = COMString::NewString(wszCodebase);
    else
        gc.url = NULL;

    if (m_pSignature && m_pSignature->pbSigner && m_pSignature->cbSigner)
        SecurityHelper::CopyEncodingToByteArray(m_pSignature->pbSigner,
                                                m_pSignature->cbSigner,
                                                &gc.cert);

    gc.serializedEvidence = GetSerializedEvidence();

    gc.additionalEvidence = ObjectFromHandle(m_hAdditionalEvidence);

    OBJECTREF assemblyref = m_pAssem->GetExposedObject();

    ARG_SLOT args[] = {
        ObjToArgSlot(assemblyref),
        ObjToArgSlot(gc.url),
        ObjToArgSlot(gc.uniqueID),
        (ARG_SLOT)dwZone,
        ObjToArgSlot(gc.cert),
        ObjToArgSlot(gc.serializedEvidence),
        ObjToArgSlot(gc.additionalEvidence),
    };

    evidence = ArgSlotToObj(Security::s_stdData.pMethCreateSecurityIdentity->Call(args, METHOD__ASSEMBLY__CREATE_SECURITY_IDENTITY));

    GCPROTECT_END();

    return evidence;
}


DWORD AssemblySecurityDescriptor::GetZone()
{
    HRESULT     hr;
    LPWSTR      wszCodebase;
    DWORD       dwZone = (DWORD) NoZone;
    BYTE        rbUniqueID[MAX_SIZE_SECURITY_ID];
    DWORD       cbUniqueID = sizeof(rbUniqueID);

    hr = m_pAssem->GetSecurityIdentity(&wszCodebase, &dwZone, rbUniqueID, &cbUniqueID);
    _ASSERTE(SUCCEEDED(hr));

    if (FAILED(hr))
        dwZone = (DWORD) NoZone;

    return dwZone;
}


HRESULT AssemblySecurityDescriptor::LoadSignature(COR_TRUST **ppSignature)
{
    if (GetProperties(CORSEC_SIGNATURE_LOADED) != 0) {
        if (ppSignature)
            *ppSignature = m_pSignature;
        return S_OK;
    }

    // Dynamic modules never have a signature.
    if (m_pSharedSecDesc->GetManifestFile() == NULL)
        return S_OK;

    HRESULT hr        = S_OK;
    IMAGE_DATA_DIRECTORY *pDir = m_pSharedSecDesc->GetManifestFile()->GetSecurityHeader();

    if (pDir == NULL || pDir->VirtualAddress == 0 || pDir->Size == 0)
    {
        SetProperties(CORSEC_SIGNATURE_LOADED);
        LOG((LF_SECURITY, LL_INFO1000, "No certificates found in module\n"));
        _ASSERTE(m_pSignature == NULL);
        if (ppSignature)
            *ppSignature = NULL;
        return hr;
    }


    SetProperties(CORSEC_SIGNATURE_LOADED);

    if (ppSignature)
        *ppSignature = m_pSignature;

    return S_OK;
}

AssemblySecurityDescriptor::~AssemblySecurityDescriptor()
{
    if (m_pSignature != NULL)
        FreeM(m_pSignature);

    if (!m_pAssem)
        return;

    if (m_pSharedSecDesc)
        m_pSharedSecDesc->RemoveSecDesc(this);

    if (m_pSharedSecDesc == NULL || !IsSystem()) {

        // If we're not a system assembly (loaded into an arbitrary appdomain),
        // remove this ASD from the owning domain's ASD list. This should happen at
        // only two points: appdomain unload and assembly load failure. We don't
        // really care about the removal in case one, but case two would result in a
        // corrupt list off the appdomain.
        RemoveFromAppDomainList();

        ApplicationSecurityDescriptor *asd = m_pAppDomain->GetSecurityDescriptor();
        if (asd)
            asd->RemoveSecDesc(this);
    }
}

AssemblySecurityDescriptor *AssemblySecurityDescriptor::Init(Assembly *pAssembly, bool fLink)
{
    SharedSecurityDescriptor *pSharedSecDesc = pAssembly->GetSharedSecurityDescriptor();
    m_pAssem = pAssembly;
    m_pSharedSecDesc = pSharedSecDesc;
    
    AssemblySecurityDescriptor *pSecDesc = this;

    // Attempt to insert onto list of assembly descriptors (one per domain this
    // assembly is loaded into to). We could be racing to do this, so if we fail
    // the insert back off and use the descriptor that won.
    if (fLink && !pSharedSecDesc->InsertSecDesc(this)) {
        pSecDesc = pSharedSecDesc->FindSecDesc(m_pAppDomain);
        delete this;
    }

    return pSecDesc;
}

bool SecurityDescriptor::IsSystem()
{
    // Always return false for appdomains.
    if (m_pAssem == NULL)
        return false;

    return ((AssemblySecurityDescriptor*)this)->m_pSharedSecDesc->IsSystem();
}


// Add ASD to list of ASDs seen in the current AppDomain context.
void AssemblySecurityDescriptor::AddToAppDomainList()
{
    SecurityContext *pSecContext = GetAppDomain()->m_pSecContext;
    EnterCriticalSection(&pSecContext->m_sAssembliesLock);
    m_pNextAssembly = pSecContext->m_pAssemblies;
    pSecContext->m_pAssemblies = this;
    LeaveCriticalSection(&pSecContext->m_sAssembliesLock);
}

// Remove ASD from list of ASDs seen in the current AppDomain context.
void AssemblySecurityDescriptor::RemoveFromAppDomainList()
{
    SecurityContext *pSecContext = GetAppDomain()->m_pSecContext;
    EnterCriticalSection(&pSecContext->m_sAssembliesLock);
    AssemblySecurityDescriptor **ppSecDesc = &pSecContext->m_pAssemblies;
    while (*ppSecDesc && *ppSecDesc != this)
        ppSecDesc = &(*ppSecDesc)->m_pNextAssembly;
    if (*ppSecDesc)
        *ppSecDesc = m_pNextAssembly;
    LeaveCriticalSection(&pSecContext->m_sAssembliesLock);
}


// Creates the PermissionListSet which holds the AppDomain level intersection of 
// granted and denied permission sets of all assemblies in the domain
OBJECTREF ApplicationSecurityDescriptor::Init()
{
    THROWSCOMPLUSEXCEPTION();

    if (Security::IsSecurityOn())
    {
        OBJECTREF refPermListSet = NULL;
        GCPROTECT_BEGIN(refPermListSet);

        ContextTransitionFrame frame;
        Thread *pThread = GetThread();
        Context *pContext = this->m_pAppDomain->GetDefaultContext();

        MethodTable *pMT = g_Mscorlib.GetClass(CLASS__PERMISSION_LIST_SET);
        MethodDesc *pCtor = g_Mscorlib.GetMethod(METHOD__PERMISSION_LIST_SET__CTOR);

        pThread->EnterContextRestricted(pContext, &frame, TRUE);

        refPermListSet = AllocateObject(pMT);

        ARG_SLOT arg[1] = { 
            ObjToArgSlot(refPermListSet)
        };

        pCtor->Call(arg, METHOD__PERMISSION_LIST_SET__CTOR);
    
        if (!IsFullyTrusted() && !GetProperties(CORSEC_DEFAULT_APPDOMAIN))
        {
            OBJECTREF refGrantedSet, refDeniedSet;
            ARG_SLOT ilargs[5];
            refGrantedSet = GetGrantedPermissionSet(&refDeniedSet);
            ilargs[0] = ObjToArgSlot(refPermListSet);
            ilargs[1] = (ARG_SLOT)FALSE;
            ilargs[2] = ObjToArgSlot(refGrantedSet);
            ilargs[3] = ObjToArgSlot(refDeniedSet);
            ilargs[4] = ObjToArgSlot(NULL);

            COMCodeAccessSecurityEngine::s_seData.pMethStackCompressHelper->Call(&(ilargs[0]), METHOD__SECURITY_ENGINE__STACK_COMPRESS_WALK_HELPER);
            m_fEveryoneFullyTrusted = FALSE;
        }

        pThread->ReturnToContext(&frame, TRUE);

        GCPROTECT_END();

        return refPermListSet;
    }
    else
    {
        return NULL;
    }
}

// Whenever a new assembly is added to the domain, we need to update the PermissionListSet
// This method updates the linked list of assemblies yet to be (lazily) added to the PLS
VOID ApplicationSecurityDescriptor::AddNewSecDesc(SecurityDescriptor *pNewSecDescriptor)
{
    // NOTE: even when the optimization is "turned off", this code should still
    // be run.  This allows us to turn the optimization back on with no lose of
    // information.

    m_LockForAssemblyList.Enter();

    // Set the next to null (we don't clean up when a security descriptor is removed from
    // the list)

    pNewSecDescriptor->m_pNext = NULL;

    // Add the descriptor to this list so that is gets intersected into the AppDomain list

    SecurityDescriptor *head = m_pNewSecDesc;
    if (head == NULL)
    {
        m_pNewSecDesc = pNewSecDescriptor;
    }
    else
    {
        while (head->m_pNext != NULL)
            head = head->m_pNext;
        head->m_pNext = pNewSecDescriptor;
    }

    // Add the descriptor to this list (if necessary) so that it gets re-resolved at the
    // completion of the current resolve.

    if (GetProperties(CORSEC_RESOLVE_IN_PROGRESS) != 0)
    {
        pNewSecDescriptor->m_pPolicyLoadNext = NULL;

        SecurityDescriptor *head = m_pPolicyLoadSecDesc;
        if (head == NULL)
        {
            m_pPolicyLoadSecDesc = pNewSecDescriptor;
        }
        else
        {
            while (head->m_pPolicyLoadNext != NULL)
                head = head->m_pPolicyLoadNext;
            head->m_pNext = pNewSecDescriptor;
        }
    }
    GetNextTimeStamp();

    m_LockForAssemblyList.Leave();

    ResetStatusOf(ALL_STATUS_FLAGS);
}

// Remove assembly security descriptors added using AddNewSecDesc above.
VOID ApplicationSecurityDescriptor::RemoveSecDesc(SecurityDescriptor *pSecDescriptor)
{
    m_LockForAssemblyList.Enter();

    SecurityDescriptor *pSecDesc = m_pNewSecDesc;

    if (pSecDesc == pSecDescriptor)
    {
        m_pNewSecDesc = pSecDesc->m_pNext;
    }
    else
        while (pSecDesc)
        {
            if (pSecDesc->m_pNext == pSecDescriptor)
            {
                pSecDesc->m_pNext = pSecDescriptor->m_pNext;
                break;
            }
            pSecDesc = pSecDesc->m_pNext;
        }

    GetNextTimeStamp();

    m_LockForAssemblyList.Leave();

    ResetStatusOf(ALL_STATUS_FLAGS);
}

VOID ApplicationSecurityDescriptor::ResolveLoadList( void )
{
    THROWSCOMPLUSEXCEPTION();

    // We have to do some ugly stuff in here where we keep
    // locking and unlocking the assembly list.  This is so
    // that we can clear the head of the list off before
    // each resolve but allow the list to be added to during
    // the resolve.

    while (m_pPolicyLoadSecDesc != NULL)
    {
        m_LockForAssemblyList.Enter();

        SecurityDescriptor *head = m_pPolicyLoadSecDesc;
        m_pPolicyLoadSecDesc = m_pPolicyLoadSecDesc->m_pPolicyLoadNext;

        m_LockForAssemblyList.Leave();

        head->ResetProperties( CORSEC_RESOLVED );

        COMPLUS_TRY
        {
            head->Resolve();
        }
        COMPLUS_CATCH
        {
            OBJECTREF pThrowable = GETTHROWABLE();
            DefineFullyQualifiedNameForClassOnStack();
            LPUTF8 szClass = GetFullyQualifiedNameForClass(pThrowable->GetClass());
            if ((strcmp(g_ThreadAbortExceptionClassName, szClass) == 0) ||
                (strcmp(g_AppDomainUnloadedExceptionClassName, szClass) == 0))

            {
                COMPlusThrow(pThrowable);
            }
            else
            {
                // We don't really expect an exception here.
                _ASSERTE(FALSE);
                // but if we do get an exception, we need to rethrow
                COMPlusThrow(pThrowable);

            }
        }
        COMPLUS_END_CATCH
    }
}


// Add the granted/denied permissions of newly added assemblies to the AppDomain level intersection
FCIMPL1(LPVOID, ApplicationSecurityDescriptor::UpdateDomainPermissionListSet, DWORD *pStatus)
{

    if (Security::IsSecurityOff())
    {
        *pStatus = SECURITY_OFF;
        return NULL;
    }


    _ASSERTE (!g_pGCHeap->IsHeapPointer(pStatus));     // should be on the stack, not in the heap
    LPVOID ret;
    HELPER_METHOD_FRAME_BEGIN_RET_NOPOLL();

    ret = UpdateDomainPermissionListSetInner(pStatus);

    HELPER_METHOD_FRAME_END_POLL();

    return ret;
}
FCIMPLEND

LPVOID ApplicationSecurityDescriptor::UpdateDomainPermissionListSetInner(DWORD *pStatus)
{
    _ASSERTE( Security::IsSecurityOn() && "This method should not be called if security is not on" );

    LPVOID retVal = NULL;
    
    ApplicationSecurityDescriptor *appSecDesc = GetAppDomain()->GetSecurityDescriptor();
    
    _ASSERT(appSecDesc && "Cannot update uninitialized PermissionListSet");

    // Update the permission list for this domain.
    retVal = appSecDesc->UpdateDomainPermissionListSetStatus(pStatus);

    if (GetThread()->GetNumAppDomainsOnThread() > 1)
    {
        // The call to GetDomainPermissionListSet should perform the update for all domains.
        // So, a call to this routine shouldn't have been made if we have mulitple appdomains.
        // This means that appdomain(s) were created in the small window between a get and an update.
        // Bail in this case and the next time we try to get the permilistset the update would occur.
        *pStatus = MULTIPLE_DOMAINS;
    }

    return retVal;
}

LPVOID ApplicationSecurityDescriptor::UpdateDomainPermissionListSetStatus(DWORD *pStatus)
{
    _ASSERTE( Security::IsSecurityOn() && "This method should not be called if security is not on" );

    SEC_CHECKCONTEXT();

    THROWSCOMPLUSEXCEPTION();
    ARG_SLOT ilargs[5];
    INT32 retCode;
    LPVOID ret;
    OBJECTREF refGrantedSet, refDeniedSet, refPermListSet = NULL;
    OBJECTREF refNewPermListSet = NULL;
    SecurityDescriptor *pLastAssem = NULL;

    if (m_fInitialised == FALSE)
    {
        refNewPermListSet = Init();
    }

    // First see if anyone else is already got in here
    m_LockForAssemblyList.Enter();

    if (m_fInitialised == FALSE)
    {
        StoreObjectInHandle(m_hDomainPermissionListSet, refNewPermListSet);
        m_fInitialised = TRUE;
    }

    DWORD fIsBusy = m_fPLSIsBusy;
    if (fIsBusy == FALSE)
        m_fPLSIsBusy = TRUE;


    if (fIsBusy == TRUE)
    {
        m_LockForAssemblyList.Leave();
        *pStatus = PLS_IS_BUSY;
        return NULL;
    }

    // Next make sure the linked list integrity is maintained, by noting a time stamp and comparing later
    SecurityDescriptor *head = m_pNewSecDesc;
    DWORD startTimeStamp = GetNextTimeStamp();

    m_LockForAssemblyList.Leave();

    GCPROTECT_BEGIN(refPermListSet);

    refPermListSet = ObjectFromHandle(m_hDomainPermissionListSet);

Add_Assemblies:

    while (head != NULL)
    {
        pLastAssem = head;
        if (!head->IsFullyTrusted())
        {
            refGrantedSet = head->GetGrantedPermissionSet(&refDeniedSet);
            ilargs[0] = ObjToArgSlot(refPermListSet);
            ilargs[1] = (ARG_SLOT)FALSE;
            ilargs[2] = ObjToArgSlot(refGrantedSet);
            ilargs[3] = ObjToArgSlot(refDeniedSet);
            ilargs[4] = ObjToArgSlot(NULL);
    
            retCode = (INT32)COMCodeAccessSecurityEngine::s_seData.pMethStackCompressHelper->Call(&(ilargs[0]), METHOD__SECURITY_ENGINE__STACK_COMPRESS_WALK_HELPER);
            m_fEveryoneFullyTrusted = FALSE;
        }
        head = head->m_pNext;
    }

    
    m_LockForAssemblyList.Enter();

    if (startTimeStamp == m_dTimeStamp)
    {
        // No new assembly has been added since I started in this method
        m_pNewSecDesc = NULL;
        m_LockForAssemblyList.Leave();
    }
    else
    {
        // A new assembly MAY been added at the end of the list
        if (pLastAssem != NULL && (head = pLastAssem->m_pNext) != NULL)
        {
            // A new assembly HAS been added
            startTimeStamp = GetNextTimeStamp();
            m_LockForAssemblyList.Leave();
            goto Add_Assemblies;
        }
        m_LockForAssemblyList.Leave();
    }


    m_fPLSIsBusy = FALSE;


    *((OBJECTREF*)&ret) = refPermListSet;
    GCPROTECT_END();

    // See if there are any security overrides
    if (GetThread()->GetOverridesCount() > 0)
        *pStatus = OVERRIDES_FOUND;
    else if (m_fEveryoneFullyTrusted == TRUE)
        *pStatus = FULLY_TRUSTED;
    else
        *pStatus = CONTINUE;
    
    return ret;
}

FCIMPL4(Object*, ApplicationSecurityDescriptor::GetDomainPermissionListSet, DWORD *pStatus, Object* demand, int capOrSet, DWORD whatPermission)
{
    if (Security::IsSecurityOff())
    {
        *pStatus = SECURITY_OFF;
        return NULL;
    }
    
    // Uncomment this code if you want to rid yourself of all those superfluous security exceptions
    // you see in the debugger.
    //*pStatus = NEED_STACKWALK;
    //return NULL;

    // Track perfmon counters. Runtime security checks.
    COMCodeAccessSecurityEngine::IncrementSecurityPerfCounter();
    
    Object* retVal = NULL;
    
    Thread* currentThread = GetThread();

    if (!currentThread->GetPLSOptimizationState())
    {
        *pStatus = NEED_STACKWALK;
        retVal = NULL;
    }
    else if (currentThread->GetNumAppDomainsOnThread() > 1)
    {
        // If there are multiple app domains then check the fully trusted'ness of each
        // See if there is an overflow of appdomains
        if (GetThread()->GetNumAppDomainsOnThread() <= MAX_APPDOMAINS_TRACKED)
        {
            MethodDesc *pMeth;
            if (capOrSet == CHECK_CAP)
                pMeth = COMCodeAccessSecurityEngine::s_seData.pMethPLSDemand;
            else
            {
                pMeth = COMCodeAccessSecurityEngine::s_seData.pMethPLSDemandSet;
            }
            
            // pStatus is set to the cumulative status of all the domains.
            OBJECTREF thisDemand(demand);
            HELPER_METHOD_FRAME_BEGIN_RET_1(thisDemand);
            retVal = ApplicationSecurityDescriptor::GetDomainPermissionListSetForMultipleAppDomains (pStatus, thisDemand, pMeth, whatPermission);
            HELPER_METHOD_FRAME_END();
        }
        else
        {
            *pStatus = MULTIPLE_DOMAINS;
            retVal = NULL;
        }
    }
    else
    {
        ApplicationSecurityDescriptor *appSecDesc = GetAppDomain()->GetSecurityDescriptor();
        retVal = appSecDesc->GetDomainPermissionListSetStatus (pStatus);
    }

    FC_GC_POLL_AND_RETURN_OBJREF(retVal);
}
FCIMPLEND

LPVOID ApplicationSecurityDescriptor::GetDomainPermissionListSetInner(DWORD *pStatus, 
                                                                      OBJECTREF demand, 
                                                                      MethodDesc *plsMethod,
                                                                      DWORD whatPermission)
{
    _ASSERTE( Security::IsSecurityOn() && "This method should not be called if security is not on" );

    //*pStatus = NEED_STACKWALK;
    //return NULL;

    // If there are multiple app domains then check the fully trusted'ness of each
    if ((GetThread()->GetNumAppDomainsOnThread() > 1))
    {
        if (GetThread()->GetNumAppDomainsOnThread() <= MAX_APPDOMAINS_TRACKED)
            // pStatus is set to the cumulative status of all the domains.
            return ApplicationSecurityDescriptor::GetDomainPermissionListSetForMultipleAppDomains (pStatus, demand, plsMethod, whatPermission);
        else
        {
            *pStatus = NEED_STACKWALK;
            return NULL;
        }
    }
    else
    {
        ApplicationSecurityDescriptor *appSecDesc = GetAppDomain()->GetSecurityDescriptor();
        return appSecDesc->GetDomainPermissionListSetStatus (pStatus);
    }
}

Object* ApplicationSecurityDescriptor::GetDomainPermissionListSetStatus (DWORD *pStatus)
{
    _ASSERTE( Security::IsSecurityOn() && "This method should not be called if security is not on" );

    *pStatus = CONTINUE;
    
    // Make sure the number of security checks in this domain has crossed a threshold, before we pull in the optimization
    if (m_dNumDemands < s_dwSecurityOptThreshold)
    {
        m_dNumDemands++;
        *pStatus = BELOW_THRESHOLD;
        return NULL;
    }

    // Optimization may be turned off in a domain, for example to avoid recursion while DomainListSet is being generated
    if (!IsOptimizationEnabled())
    {
        *pStatus = NEED_STACKWALK;
        return NULL;
    }

    // Check if there is suspicion of any Deny() or PermitOnly() on the stack
    if (GetThread()->GetOverridesCount() > 0)
    {
        *pStatus = OVERRIDES_FOUND;
        return NULL;
    }
    
    // Check if DomainListSet needs update (either uninitialised or new assembly has been added etc)
    if ((m_fInitialised == FALSE) || (m_pNewSecDesc != NULL))
    {
        *pStatus = NEED_UPDATED_PLS;
        return NULL;
    }

    // Check if all assemblies so far have full trust. This allows us to do some optimizations
    if (m_fEveryoneFullyTrusted == TRUE)
    {
        *pStatus = FULLY_TRUSTED;
    }

    OBJECTREF refPermListSet = ObjectFromHandle(m_hDomainPermissionListSet);
    return OBJECTREFToObject(refPermListSet);
}

// Helper for GetDomainPermissionListSetForMultipleAppDomains, needed because of destructor
// on AppDomainIterator
static DWORD CallHelper(MethodDesc *plsMethod, ARG_SLOT *args, MetaSig *pSig)
{
    DWORD status = 0;

    COMPLUS_TRY 
      {
          ARG_SLOT retval = plsMethod->Call(args, pSig);

          // Due to how the runtime returns bools we need to mask off the higher order 32 bits.

          if ((0x00000000FFFFFFFF & retval) == 0)
              status = NEED_STACKWALK;
          else
              status = DEMAND_PASSES;
      }
    COMPLUS_CATCH
      {
          status = NEED_STACKWALK;
          // An exception is okay. It just means the short-path didnt work, need to do stackwalk
      }
    COMPLUS_END_CATCH;

    return status;
}

// OUT PARAM: pStatus   RETURNS : Always returns NULL.
//  FULLY_TRUSTED   : All domains fully trusted, return NULL
//  NEED_STACKWALK  : Need a stack walk ; return NULL
//  DEMAND_PASSES   : Check against all PLSs passed

Object* ApplicationSecurityDescriptor::GetDomainPermissionListSetForMultipleAppDomains (DWORD *pStatus, 
                                                                                        OBJECTREF demand, 
                                                                                        MethodDesc *plsMethod,
                                                                                        DWORD whatPermission)
{
    _ASSERTE( Security::IsSecurityOn() && "This method should not be called if security is not on" );

    THROWSCOMPLUSEXCEPTION();
    // First pass:
    // Update the permission list of all app domains. Side effect is that we know
    // whether all the domains are fully trusted or not.
    // This is ncessary since assemblies could have been added to these app domains.
    
    LPVOID pDomainListSet;

    Thread *pThread = GetThread();
    AppDomain *pExecDomain = GetAppDomain();
    AppDomain *pDomain = NULL, *pPrevAppDomain = NULL;
    ContextTransitionFrame frame;
    DWORD   dwAppDomainIndex = 0;

    struct _gc {
        OBJECTREF orOriginalDemand;
        OBJECTREF orDemand;
    } gc;
    gc.orOriginalDemand = demand;
    gc.orDemand = NULL;

    GCPROTECT_BEGIN (gc);

    pThread->InitDomainIteration(&dwAppDomainIndex);
    
    // Need to turn off the PLS optimization here to avoid recursion when we
    // marshal and unmarshal objects. 
    ApplicationSecurityDescriptor *pAppSecDesc;
    pAppSecDesc = pExecDomain->GetSecurityDescriptor();
    if (!pAppSecDesc->IsOptimizationEnabled())
    {
        *pStatus = NEED_STACKWALK;
        goto Finished;
    }

    // Make sure all domains on the stack have crossed the threshold..
    _ASSERT(SystemDomain::System() && "SystemDomain not yet created!");
    while ((pDomain = pThread->GetNextDomainOnStack(&dwAppDomainIndex)) != NULL)
    {

        ApplicationSecurityDescriptor *currAppSecDesc = pDomain->GetSecurityDescriptor();
        if (currAppSecDesc == NULL)
            // AppDomain still being created.
            continue;

        if (currAppSecDesc->m_dNumDemands < s_dwSecurityOptThreshold)
        {
            *pStatus = NEED_STACKWALK;
            goto Finished;
        }
    }

    // If an appdomain in the stack has been unloaded, we'll get a null before
    // reaching the last appdomain on the stack.  In this case we want to
    // permanently disable the optimization.
    if (dwAppDomainIndex != 0)
    {
        *pStatus = NEED_STACKWALK;
        goto Finished;
    }

    // We're going to make a demand against grant sets in multiple appdomains,
    // so we must marshal the demand for each one. We can perform the serialize
    // half up front and just deserialize as we iterate over the appdomains.
    BYTE *pbDemand;
    DWORD cbDemand;
    pAppSecDesc->DisableOptimization();
    AppDomainHelper::MarshalObject(pExecDomain,
                                  &(gc.orOriginalDemand),
                                  &pbDemand,
                                  &cbDemand);
    pAppSecDesc->EnableOptimization();

    pThread->InitDomainIteration(&dwAppDomainIndex);
    _ASSERT(SystemDomain::System() && "SystemDomain not yet created!");
    while ((pDomain = pThread->GetNextDomainOnStack(&dwAppDomainIndex)) != NULL)
    {

        if (pDomain == pPrevAppDomain)
            continue;

        ApplicationSecurityDescriptor *currAppSecDesc = pDomain->GetSecurityDescriptor();
        if (currAppSecDesc == NULL)
            // AppDomain still being created.
            continue;

        // Transition into current appdomain before doing any work with
        // managed objects that could bleed across boundaries otherwise.
        // We might also need to deserialize the demand into the new appdomain.
        if (pDomain != pExecDomain)
        {
            pAppSecDesc = pDomain->GetSecurityDescriptor();
            if (!pAppSecDesc->IsOptimizationEnabled())
            {
                *pStatus = NEED_STACKWALK;
                break;
            }

            pThread->EnterContextRestricted(pDomain->GetDefaultContext(), &frame, TRUE);
            pAppSecDesc->DisableOptimization();
            AppDomainHelper::UnmarshalObject(pDomain,
                                            pbDemand,
                                            cbDemand,
                                            &gc.orDemand);
            pAppSecDesc->EnableOptimization();
        }
        else
            gc.orDemand = gc.orOriginalDemand;

        // Get the status of permission list for domain.
        pDomainListSet = currAppSecDesc->GetDomainPermissionListSetStatus(pStatus);
            
        // Does this PLS require an update? If so do it now.
        if (*pStatus == NEED_UPDATED_PLS)
            pDomainListSet = currAppSecDesc->UpdateDomainPermissionListSetStatus(pStatus);

        if (*pStatus == CONTINUE || *pStatus == FULLY_TRUSTED)
        {

            OBJECTREF refDomainPLS = ObjectToOBJECTREF((Object *)pDomainListSet);
            ARG_SLOT arg[2] = { ObjToArgSlot(refDomainPLS), ObjToArgSlot(gc.orDemand)};

            MetaSig sSig(plsMethod == COMCodeAccessSecurityEngine::s_seData.pMethPLSDemand ?
                         COMCodeAccessSecurityEngine::s_seData.pSigPLSDemand :
                         COMCodeAccessSecurityEngine::s_seData.pSigPLSDemandSet);

            if ((*pStatus = CallHelper(plsMethod, &arg[0], &sSig)) == NEED_STACKWALK)
            {
                if (pDomain != pExecDomain)
                    pThread->ReturnToContext(&frame, TRUE);
                break;
            }
        }
        else
        {
            // If the permissions are anything but FULLY TRUSTED or CONTINUE (e.g. OVERRIDES etc. then bail
            *pStatus = NEED_STACKWALK;
            if (pDomain != pExecDomain)
                pThread->ReturnToContext(&frame, TRUE);
            break;
        }

        if (pDomain != pExecDomain)
            pThread->ReturnToContext(&frame, TRUE);
    }

    FreeM(pbDemand);

    // If an appdomain in the stack has been unloaded, we'll get a null before
    // reaching the last appdomain on the stack.  In this case we want to
    // permanently disable the optimization.
    if (dwAppDomainIndex != 0)
    {
        *pStatus = NEED_STACKWALK;
    }


 Finished:
    GCPROTECT_END();

    _ASSERT ((*pStatus == FULLY_TRUSTED) || (*pStatus == NEED_STACKWALK) || (*pStatus == DEMAND_PASSES));
    return NULL;
}


SharedSecurityDescriptor::SharedSecurityDescriptor(Assembly *pAssembly) :
    m_pAssembly(pAssembly),
    m_pManifestFile(NULL),
    m_pDescs(NULL),
    m_fIsSystem(false),
    m_fModifiedGrant(false),
    m_pbGrantSetBlob(NULL),
    m_cbGrantSetBlob(0),
    m_pbDeniedSetBlob(NULL),
    m_cbDeniedSetBlob(0),
    m_fResolved(false),
    m_fFullyTrusted(false),
    m_pResolvingThread(NULL)
{
    InitializeCriticalSection(&m_sLock);
}

SharedSecurityDescriptor::~SharedSecurityDescriptor()
{
    // Sever ties to any AssemblySecurityDescriptors we have (they'll be cleaned
    // up in AD unload).
    AssemblySecurityDescriptor *pSecDesc = m_pDescs;
    while (pSecDesc) {
        pSecDesc->m_pSharedSecDesc = NULL;
        pSecDesc = pSecDesc->m_pNextContext;
    }

    DeleteCriticalSection(&m_sLock);
    if (m_pbGrantSetBlob)
        FreeM(m_pbGrantSetBlob);
    if (m_pbDeniedSetBlob)
        FreeM(m_pbDeniedSetBlob);
}

bool SharedSecurityDescriptor::InsertSecDesc(AssemblySecurityDescriptor *pSecDesc)
{
    EnterLock();

    // Search list for duplicate first (another descriptor for the same
    // appdomain).
    AssemblySecurityDescriptor *pDesc = m_pDescs;
    while (pDesc) {
        if (pDesc->m_pAppDomain == pSecDesc->m_pAppDomain) {
            LeaveLock();
            return false;
        }
        pDesc = pDesc->m_pNextContext;
    }

    // No duplicate, insert at head of list (just for simplicity, there's no
    // implied ordering in the list).
    pSecDesc->m_pNextContext = m_pDescs;
    m_pDescs = pSecDesc;

    // We also link the ASD in the other direction: all ASDs (for different
    // assemblies) loaded in the same appdomain.
    pSecDesc->AddToAppDomainList();

    LeaveLock();

    return true;
}

void SharedSecurityDescriptor::RemoveSecDesc(AssemblySecurityDescriptor *pSecDesc)
{
    // Check that we're not removing a security descriptor that contains the
    // only copy of the grant set (this should be guaranteed by a call to
    // MarshalGrantSet in AppDomain::Exit).
    _ASSERTE(!pSecDesc->IsResolved() || m_pbGrantSetBlob != NULL || m_fIsSystem);

    EnterLock();

    AssemblySecurityDescriptor *pDesc = m_pDescs;
    if (pDesc == pSecDesc)
        m_pDescs = pSecDesc->m_pNextContext;
    else
        while (pDesc) {
            if (pDesc->m_pNextContext == pSecDesc) {
                pDesc->m_pNextContext = pSecDesc->m_pNextContext;
                break;
            }
            pDesc = pDesc->m_pNextContext;
        }

    LeaveLock();
}

AssemblySecurityDescriptor *SharedSecurityDescriptor::FindSecDesc(AppDomain *pDomain)
{
    // System assemblies only have only descriptor.
    if (m_fIsSystem)
        return m_pDescs;

    EnterLock();
    AssemblySecurityDescriptor *pDesc = m_pDescs;
    while (pDesc) {
        if (pDesc->m_pAppDomain == pDomain) {
            LeaveLock();
            return pDesc;
        }
        pDesc = pDesc->m_pNextContext;
    }
    LeaveLock();
    return NULL;
}

void SharedSecurityDescriptor::Resolve(AssemblySecurityDescriptor *pSecDesc)
{
    THROWSCOMPLUSEXCEPTION();

    // Shortcut for resolving when we don't care which appdomain context we use.
    // If there are no instances of this assembly currently loaded, we don't
    // have anything to do (the resolve was already done and the results
    // serialized, or we cannot resolve).
    if (pSecDesc == NULL) {
        pSecDesc = m_pDescs;
        if (pSecDesc == NULL)
            return;
    }

    // Quick, low-cost check.
    if (m_fResolved) {
        UpdateGrantSet(pSecDesc);
        return;
    }

    EnterLock();

    // Check again now we're synchronized.
    if (m_fResolved) {
        LeaveLock();
        UpdateGrantSet(pSecDesc);
        return;
    }

    // Policy resolution might be in progress. If so, it's either a
    // recursive call (which we should allow through into the main resolve
    // logic), or another thread, which we should block until policy is
    // fully resolved.
    Thread *pThread = GetThread();
    _ASSERTE(pThread);

    // Recursive case.
    if (m_pResolvingThread == pThread) {
        LeaveLock();
        pSecDesc->ResolveWorker();
        return;
    }

    // Multi-threaded case.
    if (m_pResolvingThread) {
        while (!m_fResolved && m_pResolvingThread) {
            LeaveLock();

            // We need to enable GC while we sleep. Resolve takes a while, and
            // we don't want to spin on a different CPU on an MP machine, so
            // we'll wait a reasonable amount of time. This contention case is
            // unlikely, so there's not really a perf issue with waiting a few
            // ms extra for the result of the resolve on the second thread.
            // Given the length of the wait, it's probably not worth optimizing
            // out the mutex acquisition/relinquishment.
            BEGIN_ENSURE_PREEMPTIVE_GC();
            ::Sleep(50);
            END_ENSURE_PREEMPTIVE_GC();

            EnterLock();
        }
        if (m_fResolved) {
            LeaveLock();
            UpdateGrantSet(pSecDesc);
            return;
        }
        // The resolving thread threw an exception and gave up. Fall through and
        // let this thread have a go.
    }

    // This thread will do the resolve.
    m_pResolvingThread = pThread;
    LeaveLock();

    EE_TRY_FOR_FINALLY {
        pSecDesc->ResolveWorker();
        _ASSERTE(pSecDesc->IsResolved());
    } EE_FINALLY {
        EnterLock();
        if (pSecDesc->IsResolved()) {
            m_fFullyTrusted = pSecDesc->IsFullyTrusted() != 0;
            m_fResolved = true;
        }
        m_pResolvingThread = NULL;
        LeaveLock();
    } EE_END_FINALLY
}

void SharedSecurityDescriptor::EnsureGrantSetSerialized(AssemblySecurityDescriptor *pSecDesc)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(!m_fIsSystem);
    _ASSERTE(m_fResolved);
    _ASSERTE(pSecDesc == NULL || pSecDesc->IsResolved());

    // Quick early exit.
    if (m_pbGrantSetBlob != NULL)
        return;

    BOOL fMustRelease = FALSE;

    // We don't have a serialized version of the grant set (though someone might
    // be racing to create it), but we know some assembly security descriptor
    // has it in object form (we're guaranteed this since the act of shutting
    // down the only assembly instance with a copy of the grant set will
    // automatically serialize it). We must take the lock to synchronize the act
    // of finding that someone has beaten us to the serialization or finding and
    // ref counting an appdomain to perform the serialization for us.
    if (pSecDesc == NULL) {
        EnterLock();

        if (m_pbGrantSetBlob == NULL) {
            pSecDesc = m_pDescs;
            while (pSecDesc) {
                if (pSecDesc->IsResolved())
                    break;
                pSecDesc = pSecDesc->m_pNextContext;
            }
            _ASSERTE(pSecDesc);
            pSecDesc->m_pAppDomain->AddRef();
            fMustRelease = TRUE;
        }

        LeaveLock();
    }

    if (pSecDesc) {

        COMPLUS_TRY {

            BYTE       *pbGrantedBlob = NULL;
            DWORD       cbGrantedBlob = 0;
            BYTE       *pbDeniedBlob = NULL;
            DWORD       cbDeniedBlob = 0;

            // Since we're outside the lock we're potentially racing to serialize
            // the sets. This is OK, all threads will produce essentially the same
            // serialized blob and the loser will simply discard their copy.

            struct _gc {
                OBJECTREF orGranted;
                OBJECTREF orDenied;
            } gc;
            ZeroMemory(&gc, sizeof(gc));
            GCPROTECT_BEGIN(gc);
            gc.orGranted = ObjectFromHandle(pSecDesc->m_hGrantedPermissionSet);
            gc.orDenied = ObjectFromHandle(pSecDesc->m_hGrantDeniedPermissionSet);

            if (gc.orDenied == NULL) {
                AppDomainHelper::MarshalObject(pSecDesc->m_pAppDomain,
                                               &(gc.orGranted),
                                               &pbGrantedBlob,
                                               &cbGrantedBlob);
            } else {
                AppDomainHelper::MarshalObjects(pSecDesc->m_pAppDomain,
                                                &(gc.orGranted),
                                                &(gc.orDenied),
                                                &pbGrantedBlob,
                                                &cbGrantedBlob,
                                                &pbDeniedBlob,
                                                &cbDeniedBlob);
            }
            GCPROTECT_END();

            // Acquire lock to update shared fields atomically.
            EnterLock();

            if (m_pbGrantSetBlob == NULL) {
                m_pbGrantSetBlob = pbGrantedBlob;
                m_cbGrantSetBlob = cbGrantedBlob;
                m_pbDeniedSetBlob = pbDeniedBlob;
                m_cbDeniedSetBlob = cbDeniedBlob;
            } else {
                FreeM(pbGrantedBlob);
                if (pbDeniedBlob)
                    FreeM(pbDeniedBlob);
            }

            LeaveLock();

        } COMPLUS_CATCH {

            // First we need to check what type of exception occured.
            // If it is anything other than an appdomain unloaded exception
            // than we should just rethrow it.

            OBJECTREF pThrowable = GETTHROWABLE();
            DefineFullyQualifiedNameForClassOnStack();
            LPUTF8 szClass = GetFullyQualifiedNameForClass(pThrowable->GetClass());
            if (strcmp(g_AppDomainUnloadedExceptionClassName, szClass) != 0)
            {
                if (fMustRelease)
                    pSecDesc->m_pAppDomain->Release();
                FATAL_EE_ERROR();
            }

            Thread* pThread = GetThread();

            // If this thread references the appdomain that we are trying to
            // marshal into (and which is being unloaded), then our job is
            // done and we should just get out of here as the unload process
            // will take care of everything for us.

            if (pThread->IsRunningIn( pSecDesc->m_pAppDomain, NULL ) == NULL)
            {
            // We could get really unlucky here: we found an appdomain with the
            // resolved grant set, but the appdomain is unloading and won't allow us
            // to enter it in order to marshal the sets out. The unload operation
            // itself will serialize the set, so all we need to do is wait.

            // Wait till the appdomain has no more managed objects.
            BEGIN_ENSURE_PREEMPTIVE_GC();

            DWORD dwWaitCount = 0;

            while (pSecDesc->m_pAppDomain->ShouldHaveRoots()) {
                    pThread->UserSleep( 100 );

                if (++dwWaitCount > 300)
                    {
                        if (fMustRelease)
                            pSecDesc->m_pAppDomain->Release();
                        FATAL_EE_ERROR();
                    }
            }
            END_ENSURE_PREEMPTIVE_GC();

            _ASSERTE(m_pbGrantSetBlob);
                if (!m_pbGrantSetBlob)
                {
                    if (fMustRelease)
                        pSecDesc->m_pAppDomain->Release();
                    FATAL_EE_ERROR();
                }
            }
            else
            {
                if (fMustRelease)
                    pSecDesc->m_pAppDomain->Release();
                COMPlusThrow(pThrowable);
            }

        } COMPLUS_END_CATCH

        if (fMustRelease)
            pSecDesc->m_pAppDomain->Release();
    }
}

void SharedSecurityDescriptor::UpdateGrantSet(AssemblySecurityDescriptor *pSecDesc)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(m_fResolved);

    if (pSecDesc->IsResolved())
        return;

    _ASSERTE(!m_fIsSystem);

    // Since we don't have a copy of the grant and denied sets in the context of
    // the given appdomain, but we do know they have been calculated in at least
    // one appdomain for this shared assembly, we need to deserialize the sets
    // into our appdomain. If we're lucky the serialization half has been
    // performed already, otherwise we'll have to hunt down an appdomain that's
    // already resolved (we're guaranteed to find one, since if we try to unload
    // such an appdomain and grant sets haven't been serialized, the
    // serialization is performed then and there).
    EnsureGrantSetSerialized();

    // We should have a serialized set by now; deserialize into the correct
    // context.
    _ASSERTE(m_pbGrantSetBlob);

    OBJECTREF   orGranted = NULL;
    OBJECTREF   orDenied = NULL;

    if (m_pbDeniedSetBlob == NULL) {
        AppDomainHelper::UnmarshalObject(pSecDesc->m_pAppDomain,
                                        m_pbGrantSetBlob,
                                        m_cbGrantSetBlob,
                                        &orGranted);
    } else {
        AppDomainHelper::UnmarshalObjects(pSecDesc->m_pAppDomain,
                                         m_pbGrantSetBlob,
                                         m_cbGrantSetBlob,
                                         m_pbDeniedSetBlob,
                                         m_cbDeniedSetBlob,
                                         &orGranted,
                                         &orDenied);
    }

    StoreObjectInHandle(pSecDesc->m_hGrantedPermissionSet, orGranted);
    StoreObjectInHandle(pSecDesc->m_hGrantDeniedPermissionSet, orDenied);

    pSecDesc->SetProperties(CORSEC_RESOLVED | (m_fFullyTrusted ? CORSEC_FULLY_TRUSTED : 0));
}

OBJECTREF SharedSecurityDescriptor::GetGrantedPermissionSet(OBJECTREF* pDeniedPermissions)
{
    THROWSCOMPLUSEXCEPTION();

    AppDomain *pDomain = GetAppDomain();
    AssemblySecurityDescriptor *pSecDesc = NULL;

    // Resolve if needed (in the current context if possible).
    if (!m_fResolved) {
        pSecDesc = FindSecDesc(pDomain);
        if (pSecDesc == NULL)
            pSecDesc = m_pDescs;
        _ASSERTE(pSecDesc);
        COMPLUS_TRY {
            pSecDesc->Resolve();
        } COMPLUS_CATCH {
            OBJECTREF pThrowable = GETTHROWABLE();
            DefineFullyQualifiedNameForClassOnStack();
            LPUTF8 szClass = GetFullyQualifiedNameForClass(pThrowable->GetClass());

            if ((strcmp(g_ThreadAbortExceptionClassName, szClass) == 0) ||
                (strcmp(g_AppDomainUnloadedExceptionClassName, szClass) == 0))

            {
                COMPlusThrow(pThrowable);
            }
            else
            {
                // We don't really expect an exception here.
                _ASSERTE(FALSE);
                // but if we do get an exception, we need to rethrow
                COMPlusThrow(pThrowable);

            }
        }
        COMPLUS_END_CATCH
    }

    // System case is simple.
    if (m_fIsSystem) {
        *pDeniedPermissions = NULL;
        return SecurityHelper::CreatePermissionSet(TRUE);
    }

    // If we happen to have the results in the right context, use them.
    if (pSecDesc && pSecDesc->m_pAppDomain == pDomain)
        return pSecDesc->GetGrantedPermissionSet(pDeniedPermissions);

    // Serialize grant/deny sets.
    EnsureGrantSetSerialized();

    // Deserialize into current context.
    OBJECTREF   orGranted = NULL;
    OBJECTREF   orDenied = NULL;

    if (m_pbDeniedSetBlob == NULL) {
        AppDomainHelper::UnmarshalObject(pDomain,
                                         m_pbGrantSetBlob,
                                         m_cbGrantSetBlob,
                                         &orGranted);
    } else {
        AppDomainHelper::UnmarshalObjects(pDomain,
                                          m_pbGrantSetBlob,
                                          m_cbGrantSetBlob,
                                          m_pbDeniedSetBlob,
                                          m_cbDeniedSetBlob,
                                          &orGranted,
                                          &orDenied);
    }

    // Return the results.
    *pDeniedPermissions = orDenied;
    return orGranted;
}

BOOL SharedSecurityDescriptor::IsFullyTrusted( BOOL lazy )
{
    if (this->m_fIsSystem)
        return TRUE;
    
    AssemblySecurityDescriptor *pSecDesc = m_pDescs;

    // Returning false here should be fine since the worst thing
    // that happens is that someone doesn't do something that
    // they would do if we were fully trusted.

    if (pSecDesc == NULL)
        return FALSE;

    return pSecDesc->IsFullyTrusted( lazy );
}


bool SharedSecurityDescriptor::MarshalGrantSet(AppDomain *pDomain)
{
    THROWSCOMPLUSEXCEPTION();

    // This is called when one of the appdomains in which this assembly is
    // loaded is being shut down. If we've resolved policy and not yet
    // serialized our grant/denied sets for the use of other instances of this
    // assembly in different appdomain contexts, we need to do it now.

    AssemblySecurityDescriptor *pSecDesc = FindSecDesc(pDomain);
    _ASSERTE(pSecDesc);

    if (pSecDesc->IsResolved() &&
        m_pbGrantSetBlob == NULL
        && !m_fIsSystem) {

        BEGIN_ENSURE_COOPERATIVE_GC();
        EnsureGrantSetSerialized(pSecDesc);
        END_ENSURE_COOPERATIVE_GC();
        return true;
    }

    return false;
}

long ApplicationSecurityDescriptor::s_iAppWideTimeStamp = 0;
Crst *ApplicationSecurityDescriptor::s_LockForAppwideFlags = NULL;
DWORD ApplicationSecurityDescriptor::s_dwSecurityOptThreshold = MAGIC_THRESHOLD;

OBJECTREF Security::GetDefaultMyComputerPolicy( OBJECTREF* porDenied )
{
    THROWSCOMPLUSEXCEPTION();

    MethodDesc* pMd = g_Mscorlib.GetMethod(METHOD__SECURITY_MANAGER__GET_DEFAULT_MY_COMPUTER_POLICY);
    
    ARG_SLOT arg[] = {
        PtrToArgSlot(porDenied)
    };
    
    return ArgSlotToObj(pMd->Call(arg, METHOD__SECURITY_MANAGER__GET_DEFAULT_MY_COMPUTER_POLICY));
}   

FCIMPL1(Object*, Security::GetEvidence, AssemblyBaseObject* pThisUNSAFE)
{
    OBJECTREF   orEvidence  = NULL;
    ASSEMBLYREF pThis       = (ASSEMBLYREF) pThisUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, pThis);


    THROWSCOMPLUSEXCEPTION();

    if (!pThis)
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    Assembly *pAssembly = pThis->GetAssembly();

    orEvidence = pAssembly->GetSecurityDescriptor()->GetEvidence();


    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(orEvidence);
}
FCIMPLEND

FCIMPL1(VOID, Security::SetOverridesCount, DWORD numAccessOverrides)
{
    GetThread()->SetOverridesCount(numAccessOverrides);
}
FCIMPLEND

FCIMPL0(DWORD, Security::IncrementOverridesCount)
{
    return GetThread()->IncrementOverridesCount();
}
FCIMPLEND

FCIMPL0(DWORD, Security::DecrementOverridesCount)
{
    return GetThread()->DecrementOverridesCount();
}
FCIMPLEND


#define DEFAULT_GLOBAL_POLICY 0

static const WCHAR* gszGlobalPolicySettings = L"GlobalSettings";


HRESULT STDMETHODCALLTYPE
GetSecuritySettings(DWORD* pdwState)
{
    HRESULT hr = S_OK;
    DWORD state = DEFAULT_GLOBAL_POLICY;

    if (pdwState == NULL) 
        return E_INVALIDARG;


    DWORD val;
    WCHAR temp[16];

    if (PAL_FetchConfigurationString(TRUE, gszGlobalPolicySettings, temp, sizeof(temp) / sizeof(WCHAR)))
    {
        LPWSTR endPtr;
        val = wcstol(temp, &endPtr, 16);         // treat it has hex
        if (endPtr != temp)                      // success
            state = val;
    }


    *pdwState = state;

    return hr;
}

HRESULT STDMETHODCALLTYPE
SetSecuritySettings(DWORD dwState)
{
    HRESULT hr = S_OK;


    CORTRY {
        WCHAR temp[16];

        // treat it has hex
        if (dwState == DEFAULT_GLOBAL_POLICY)
        {
            PAL_SetConfigurationString(TRUE, gszGlobalPolicySettings, L"");
        }
        else
        {
            _snwprintf(temp, sizeof(temp) / sizeof(WCHAR), L"%08x", dwState);

            if (!PAL_SetConfigurationString(TRUE, gszGlobalPolicySettings, temp))
            {
                CORTHROW(HRESULT_FROM_GetLastError());
            }
        }
    }
    CORCATCH(err) {
        hr = err.corError;
    } COREND;


    return hr;
}

static BOOL CallCanSkipVerification( AssemblySecurityDescriptor* pSecDesc )
{
    BOOL bRetval = FALSE;

    BEGIN_ENSURE_COOPERATIVE_GC();

    if (pSecDesc->QuickCanSkipVerification())
        bRetval = TRUE;

    if (!bRetval)
    {
        COMPLUS_TRY
        {
            pSecDesc->Resolve();
            bRetval = pSecDesc->CanSkipVerification();
        }
        COMPLUS_CATCH
        {
            bRetval = FALSE;
        }
        COMPLUS_END_CATCH
    }

    END_ENSURE_COOPERATIVE_GC();

    return bRetval;
}


BOOL
Security::CanLoadUnverifiableAssembly( PEFile* pFile, OBJECTREF* pExtraEvidence )
{
    if (Security::IsSecurityOff())
    {
        return TRUE;
    }

    // We do most of the logic to load the assembly
    // and then resolve normally.

    Assembly* pAssembly = NULL;
    AssemblySecurityDescriptor* pSecDesc = NULL;
    HRESULT hResult;
    BOOL bRetval = FALSE;

    pAssembly = new Assembly();

    _ASSERTE( pAssembly != NULL );

    if(pAssembly == NULL)
    {
        goto CLEANUP;
    }

    pAssembly->SetParent(SystemDomain::GetCurrentDomain());

    hResult = pAssembly->Init(false);

    if (FAILED(hResult))
    {
        goto CLEANUP;
    }

    hResult = Assembly::CheckFileForAssembly(pFile);

    if (FAILED(hResult))
    {
        goto CLEANUP;
    }
    

    hResult = pAssembly->AddManifest(pFile, FALSE, FALSE);

    if (FAILED(hResult))
    {
        goto CLEANUP;
    }

    pSecDesc = AssemSecDescHelper::Allocate(SystemDomain::GetCurrentDomain());

    if (pSecDesc == NULL)
    {
        goto CLEANUP;
    }

    pAssembly->GetSharedSecurityDescriptor()->SetManifestFile(pFile);
    pSecDesc = pSecDesc->Init(pAssembly);
    if (pAssembly->IsSystem())
        pSecDesc->GetSharedSecDesc()->SetSystem();

    if(pExtraEvidence != NULL) {
        BEGIN_ENSURE_COOPERATIVE_GC();
        pSecDesc->SetAdditionalEvidence(*pExtraEvidence);
        END_ENSURE_COOPERATIVE_GC();
    }

    // Then just call our existing APIs.

    bRetval = CallCanSkipVerification( pSecDesc );

CLEANUP:
    if (pAssembly != NULL) {
        pAssembly->Terminate( FALSE );
        delete pAssembly;
    }

    return bRetval;
}


