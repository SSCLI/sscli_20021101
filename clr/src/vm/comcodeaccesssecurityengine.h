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
** Header:  COMCodeAccessSecurityEngine.h
**       
** Purpose:
**
** Date:  March 21, 1998
**
===========================================================*/
#ifndef __COMCodeAccessSecurityEngine_h__
#define __COMCodeAccessSecurityEngine_h__

#include "common.h"

#include "object.h"
#include "util.hpp"
#include "fcall.h"
#include "perfcounters.h"
#include "security.h"

//-----------------------------------------------------------
// The COMCodeAccessSecurityEngine implements all the native methods
// for the interpreted System/Security/SecurityEngine.
//-----------------------------------------------------------
class COMCodeAccessSecurityEngine
{
public:
    //-----------------------------------------------------------
    // Argument declarations for native methods.
    //-----------------------------------------------------------
    
    //typedef struct _InitSecurityEngineArgs
    //{
    //    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, This);
    //} InitSecurityEngineArgs;
    
    
    //typedef struct _CheckArgs
    //{
    //    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, This);
    //    DECLARE_ECALL_I4_ARG(INT32, unrestrictedOverride);
    //    DECLARE_ECALL_I4_ARG(INT32, checkFrames);
    //    DECLARE_ECALL_PTR_ARG(StackCrawlMark*, stackMark);
    //    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, perm);
    //    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, permToken);
    //} CheckArgs;

    //typedef struct _CheckSetArgs
    //{
    //    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, This);
    //    DECLARE_ECALL_I4_ARG(INT32, unrestrictedOverride);
    //    DECLARE_ECALL_I4_ARG(INT32, checkFrames);
    //    DECLARE_ECALL_PTR_ARG(StackCrawlMark*, stackMark);
    //    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, permSet);
    //} CheckSetArgs;

    //typedef struct _CheckNReturnSOArgs
    //{
    //    DECLARE_ECALL_I4_ARG(INT32, create);
    //    DECLARE_ECALL_I4_ARG(INT32, unrestrictedOverride);
    //    DECLARE_ECALL_PTR_ARG(StackCrawlMark*, stackMark);
    //    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, perm);
    //    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, permToken);
    //} CheckNReturnSOArgs;

    //typedef struct _GetPermissionsArg
    //{
    //    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, This);
    //    DECLARE_ECALL_PTR_ARG(OBJECTREF*, ppDenied);
    //    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, pClass);
    //} GetPermissionsArg;
    
    //typedef struct _GetGrantedPermissionSetArg
    //{
    //    DECLARE_ECALL_PTR_ARG(OBJECTREF*, ppDenied);
    //    DECLARE_ECALL_PTR_ARG(OBJECTREF*, ppGranted);
    //    DECLARE_ECALL_PTR_ARG(void*, pSecDesc);
    //} GetGrantedPermissionSetArg;
    
    //typedef struct _GetCompressedStackArgs
    //{
    //    DECLARE_ECALL_PTR_ARG(StackCrawlMark*, stackMark);
    //} GetCompressedStackArgs;

    //typedef struct _GetDelayedCompressedStackArgs
    //{
    //    DECLARE_ECALL_PTR_ARG(StackCrawlMark*, stackMark);
    //} GetDelayedCompressedStackArgs;
    
    //typedef struct _GetSecurityObjectForFrameArgs
    //{
    //    DECLARE_ECALL_I4_ARG(INT32, create);
    //    DECLARE_ECALL_PTR_ARG(StackCrawlMark*, stackMark);
    //} GetSecurityObjectForFrameArgs;

public:
    // Initialize the security engine. This is called when a SecurityEngine
    // object is created, indicating that code-access security is to be
    // enforced. This should be called only once.
    static FCDECL1(void, InitSecurityEngine, Object* ThisUNSAFE);

    static void CheckSetHelper(OBJECTREF *prefDemand,
                               OBJECTREF *prefGrant,
                               OBJECTREF *prefDenied,
                               AppDomain *pGrantDomain);

	static BOOL		PreCheck(OBJECTREF demand, MethodDesc *plsMethod, DWORD whatPermission = DEFAULT_FLAG);

    // Standard code-access Permission check/demand
    static FCDECL6(void, Check, Object* ThisUNSAFE, Object* permTokenUNSAFE, Object* permUNSAFE, StackCrawlMark* stackMark, INT32 checkFrames, INT32 unrestrictedOverride);
    static void CheckInternal(OBJECTREF* pPermToken, OBJECTREF* pPerm, StackCrawlMark* stackMark, INT32 checkFrames, INT32 unrestrictedOverride);

    // EE version of Demand(). Takes care of the fully trusted case, then defers to the managed version
    static void	Demand(OBJECTREF demand);

    // Special case of Demand for unmanaged code access. Does some things possible only for this case
    static void SpecialDemand(DWORD whatPermission);

    static FCDECL5(void, CheckSet, Object* ThisUNSAFE, Object* permSetUNSAFE, StackCrawlMark* stackMark, INT32 checkFrames, INT32 unrestrictedOverride);
    static void CheckSetInternal(OBJECTREF* pPermSet, StackCrawlMark* stackMark, INT32 checkFrames, INT32 unrestrictedOverride);

    // EE version of DemandSet(). Takes care of the fully trusted case, then defers to the managed version
    static void DemandSet(OBJECTREF demand);


	// Do a CheckImmediate, and return the SecurityObject for the first frame
    static FCDECL5(Object*, CheckNReturnSO, Object* permTokenUNSAFE, Object* permUNSAFE, StackCrawlMark* stackMark, INT32 unrestrictedOverride, INT32 create);

    // Linktime check implementation for code-access permissions
    static void				  LinktimeCheck(AssemblySecurityDescriptor *pSecDesc, OBJECTREF refDemands);

    // private helper for getting a security object
//    static LPVOID   __stdcall GetSecurityObjectForFrame(const GetSecurityObjectForFrameArgs *);

    static FCDECL3(Object*, GetPermissionsP, Object* ThisUNSAFE, Object* pClassUNSAFE, OBJECTREF* ppDenied);
    static FCDECL3(void, GetGrantedPermissionSet, void* pSecDesc, OBJECTREF* ppGranted, OBJECTREF* ppDenied);
    static FCDECL1(Object*, EcallGetCompressedStack, StackCrawlMark* stackMark);
    static FCDECL1(LPVOID, EcallGetDelayedCompressedStack, StackCrawlMark* stackMark);
    static CompressedStack* __stdcall GetCompressedStack( StackCrawlMark* stackMark = NULL );
    static OBJECTREF GetCompressedStackWorker(void *pData, BOOL createList);

    static VOID UpdateOverridesCountInner(StackCrawlMark *stackMark);

    FORCEINLINE static VOID IncrementSecurityPerfCounter()
    {
        COUNTER_ONLY(GetPrivatePerfCounters().m_Security.cTotalRTChecks++);
        COUNTER_ONLY(GetGlobalPerfCounters().m_Security.cTotalRTChecks++);
    }

    static FCDECL1(VOID, UpdateOverridesCount, StackCrawlMark *stackMark);
    static FCDECL2(INT32, GetResult, DWORD whatPermission, DWORD *timeStamp);
    static FCDECL2(VOID, SetResult, DWORD whatPermission, DWORD timeStamp);
    static FCDECL1(VOID, FcallReleaseDelayedCompressedStack, CompressedStack *compressedStack);

    static void CleanupSEData();

protected:

    //-----------------------------------------------------------
    // Cached class and method pointers.
    //-----------------------------------------------------------
    typedef struct _SEData
    {
        BOOL		fInitialized;
        MethodTable    *pSecurityEngine;
        MethodTable    *pSecurityRuntime;
        MethodTable    *pPermListSet;
        MethodDesc     *pMethCheckHelper;
        MethodDesc     *pMethFrameDescHelper;
        MethodDesc     *pMethLazyCheckSetHelper;
        MethodDesc     *pMethCheckSetHelper;
        MethodDesc     *pMethFrameDescSetHelper;
        MethodDesc     *pMethStackCompressHelper;
        MethodDesc     *pMethPermListSetInit;
        MethodDesc     *pMethAppendStack;
        MethodDesc     *pMethOverridesHelper;
        MethodDesc     *pMethPLSDemand;
        MethodDesc     *pMethPLSDemandSet;
        MetaSig        *pSigPLSDemand;
        MetaSig        *pSigPLSDemandSet;
        FieldDesc      *pFSDnormalPermSet;
    } SEData;

    static void InitSEData();
    static CRITICAL_SECTION s_csLock;
    static LONG s_nInitLock;
    static BOOL s_fLockReady;

public:
	static SEData s_seData;

};


#endif /* __COMCodeAccessSecurityEngine_h__ */

