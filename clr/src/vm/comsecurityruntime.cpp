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
** Header:  COMSecurityRuntime.cpp
**
**                                             
**
** Purpose:
**
** Date:  March 21, 1998
**
===========================================================*/
#include "common.h"

#include "object.h"
#include "excep.h"
#include "vars.hpp"
#include "comsecurityruntime.h"
#include "security.h"
#include "gcscan.h"
#include "appdomainhelper.h"


//-----------------------------------------------------------+
// P R I V A T E   H E L P E R S 
//-----------------------------------------------------------+

LPVOID GetSecurityObjectForFrameInternal(StackCrawlMark *stackMark, INT32 create, OBJECTREF *pRefSecDesc)
{ 
    THROWSCOMPLUSEXCEPTION();

    // This is a package protected method. Assumes correct usage.

    Thread *pThread = GetThread();
    AppDomain * pAppDomain = pThread->GetDomain();
    if (pRefSecDesc == NULL)
    {
        if (!Security::SkipAndFindFunctionInfo(stackMark, NULL, &pRefSecDesc, &pAppDomain))
            return NULL;
    }

    if (pRefSecDesc == NULL)
        return NULL;

    // Is security object frame in a different context?
    bool fSwitchContext = pAppDomain != pThread->GetDomain();

    if (create && *pRefSecDesc == NULL)
    {
        ContextTransitionFrame frame;

        COMSecurityRuntime::InitSRData();

        // If necessary, shift to correct context to allocate security object.
        if (fSwitchContext)
            pThread->EnterContextRestricted(pAppDomain->GetDefaultContext(), &frame, TRUE);

        *pRefSecDesc = AllocateObject(COMSecurityRuntime::s_srData.pFrameSecurityDescriptor);

        if (fSwitchContext)
            pThread->ReturnToContext(&frame, TRUE);
    }

    // If we found or created a security object in a different context, make a
    // copy in the current context.
    LPVOID rv;
    if (fSwitchContext && *pRefSecDesc != NULL)
        *((OBJECTREF*)&rv) = AppDomainHelper::CrossContextCopyFrom(pAppDomain, pRefSecDesc);
    else
        *((OBJECTREF*)&rv) = *pRefSecDesc;

    return rv;
}

FCIMPL2(Object*, COMSecurityRuntime::GetSecurityObjectForFrame, StackCrawlMark* stackMark, INT32 create)
{
    OBJECTREF refRetVal = NULL;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, refRetVal);
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();

    refRetVal = ObjectToOBJECTREF((Object*)GetSecurityObjectForFrameInternal(stackMark, create, NULL));

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refRetVal);
}
FCIMPLEND

FCIMPL2(void, COMSecurityRuntime::SetSecurityObjectForFrame, StackCrawlMark* stackMark, Object* pInputRefSecDescUNSAFE)
{
    OBJECTREF pInputRefSecDesc = (OBJECTREF) pInputRefSecDescUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_1(pInputRefSecDesc);
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();

    // This is a package protected method. Assumes correct usage.

    OBJECTREF* pCurrentRefSecDesc;
    bool fSwitchContext = false;

    Thread *pThread = GetThread();
    AppDomain * pAppDomain = pThread->GetDomain();

    if (!Security::SkipAndFindFunctionInfo(stackMark, NULL, &pCurrentRefSecDesc, &pAppDomain))
        goto LExit;

    if (pCurrentRefSecDesc == NULL)
        goto LExit;

    COMSecurityRuntime::InitSRData();

    // Is security object frame in a different context?
    fSwitchContext = pAppDomain != pThread->GetDomain();

    if (fSwitchContext && pInputRefSecDesc != NULL)
        pInputRefSecDesc = AppDomainHelper::CrossContextCopyFrom(pAppDomain, &pInputRefSecDesc);

    // This is a stack based objectref
    // and therefore SetObjectReference
    // is not necessary.
    *pCurrentRefSecDesc = pInputRefSecDesc;

LExit: ;
    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

//-----------------------------------------------------------+
// I N I T I A L I Z A T I O N
//-----------------------------------------------------------+

COMSecurityRuntime::SRData COMSecurityRuntime::s_srData;

void COMSecurityRuntime::InitSRData()
{
    THROWSCOMPLUSEXCEPTION();

    if (s_srData.fInitialized == FALSE)
    {
        s_srData.pSecurityRuntime = g_Mscorlib.GetClass(CLASS__SECURITY_RUNTIME);
        s_srData.pFrameSecurityDescriptor = g_Mscorlib.GetClass(CLASS__FRAME_SECURITY_DESCRIPTOR);
        
        s_srData.pFSD_assertions = NULL;
        s_srData.pFSD_denials = NULL;
        s_srData.pFSD_restriction = NULL;
        s_srData.fInitialized = TRUE;
    }
}

//-----------------------------------------------------------
// Initialization of native security runtime.
// Called when SecurityRuntime is constructed.
//-----------------------------------------------------------
FCIMPL1(void, COMSecurityRuntime::InitSecurityRuntime, Object* ThisUNSAFE)
{
    OBJECTREF This = (OBJECTREF) ThisUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_1(This);
    //-[autocvtpro]-------------------------------------------------------

    InitSRData();

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

FieldDesc *COMSecurityRuntime::GetFrameSecDescField(DWORD dwAction)
{
    switch (dwAction)
    {
    case dclAssert:
        if (s_srData.pFSD_assertions == NULL)
            s_srData.pFSD_assertions = g_Mscorlib.GetField(FIELD__FRAME_SECURITY_DESCRIPTOR__ASSERT_PERMSET);
        return s_srData.pFSD_assertions;
        break;
        
    case dclDeny:
        if (s_srData.pFSD_denials == NULL)
            s_srData.pFSD_denials = g_Mscorlib.GetField(FIELD__FRAME_SECURITY_DESCRIPTOR__DENY_PERMSET);
        return s_srData.pFSD_denials;
        break;
        
    case dclPermitOnly:
        if (s_srData.pFSD_restriction == NULL)
            s_srData.pFSD_restriction = g_Mscorlib.GetField(FIELD__FRAME_SECURITY_DESCRIPTOR__RESTRICTION_PERMSET);
        return s_srData.pFSD_restriction;
        break;
        
    default:
        _ASSERTE(!"Unknown action requested in UpdateFrameSecurityObj");
        return NULL;
        break;
    }

}
//-----------------------------------------------------------+
// T E M P O R A R Y   M E T H O D S ! ! !
//-----------------------------------------------------------+

//-----------------------------------------------------------
// Warning!! This is passing out a reference to the permissions
// for the module. It must be deep copied before passing it out
//
// This only returns the declared permissions for the class
//-----------------------------------------------------------
FCIMPL3(Object*, COMSecurityRuntime::GetDeclaredPermissionsP, Object* ThisUNSAFE, Object* pClassUNSAFE, INT32 iType)
{
    OBJECTREF refRetVal = NULL;
    OBJECTREF This = (OBJECTREF) ThisUNSAFE;
    OBJECTREF pClass = (OBJECTREF) pClassUNSAFE;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_2(Frame::FRAME_ATTR_RETURNOBJ, This, pClass);
    //-[autocvtpro]-------------------------------------------------------

    THROWSCOMPLUSEXCEPTION();

    // An exception is thrown by the SecurityManager wrapper to ensure this case.
    _ASSERTE(pClass != NULL);
    _ASSERTE((CorDeclSecurity)iType > dclActionNil &&
             (CorDeclSecurity)iType <= dclMaximumValue);

    OBJECTREF objRef = pClass;
    EEClass* pClass = objRef->GetClass();
    _ASSERTE(pClass);
    _ASSERTE(pClass->GetModule());

    // Return the token that belongs to the Permission just asserted.
    OBJECTREF refDecls;
    HRESULT hr = SecurityHelper::GetDeclaredPermissions(pClass->GetModule()->GetMDImport(),
                                                        pClass->GetCl(),
                                                        (CorDeclSecurity)iType,
                                                        &refDecls);
    if (FAILED(hr))
        COMPlusThrowHR(hr);

    refRetVal = refDecls;

    //-[autocvtepi]-------------------------------------------------------
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(refRetVal);
}
FCIMPLEND


