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
#include "appdomainhelper.h"


#include "corperme.h"

void AppDomainHelper::CopyEncodingToByteArray(IN PBYTE   pbData,
                                              IN DWORD   cbData,
                                              OUT OBJECTREF* pArray)
{
    THROWSCOMPLUSEXCEPTION();
    U1ARRAYREF pObj;
    _ASSERTE(pArray);

    if(cbData) {
        pObj = (U1ARRAYREF)AllocatePrimitiveArray(ELEMENT_TYPE_U1,cbData);
        if(pObj == NULL) COMPlusThrowOM();

        memcpyNoGCRefs(pObj->m_Array, pbData, cbData);
        *pArray = (OBJECTREF) pObj;
    } else
        *pArray = NULL;
}


void AppDomainHelper::CopyByteArrayToEncoding(IN U1ARRAYREF* pArray,
                                              OUT PBYTE*   ppbData,
                                              OUT DWORD*   pcbData)
{
    THROWSCOMPLUSEXCEPTION();
    _ASSERTE(pArray);
    _ASSERTE(ppbData);
    _ASSERTE(pcbData);

    if (*pArray == NULL) {
        *ppbData = NULL;
        *pcbData = 0;
        return;
    }

    DWORD size = (DWORD) (*pArray)->GetNumComponents();
    if(size) {
        *ppbData = (PBYTE) MallocM(size);
        if(*ppbData == NULL) COMPlusThrowOM();
        *pcbData = size;

        CopyMemory(*ppbData, (*pArray)->GetDirectPointerToNonObjectElements(), size);
    }
}


// Marshal a single object into a serialized blob.
void AppDomainHelper::MarshalObject(IN AppDomain *pDomain,
                                    IN OBJECTREF *orObject, // Object must be GC protected
                                    OUT U1ARRAYREF *porBlob)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(orObject);

    Thread *pThread = GetThread();
    AppDomain *pCurrDomain = pThread->GetDomain();
    ContextTransitionFrame frame;

    MethodDesc *pMD = g_Mscorlib.GetMethod(METHOD__APP_DOMAIN__MARSHAL_OBJECT);

    if (pCurrDomain != pDomain)
        pThread->EnterContext(pDomain->GetDefaultContext(), &frame, TRUE);

    ARG_SLOT args[] = {
        ObjToArgSlot(*orObject)
    };

    *porBlob = (U1ARRAYREF)ArgSlotToObj(pMD->Call(args, METHOD__APP_DOMAIN__MARSHAL_OBJECT));

    if (pCurrDomain != pDomain)
        pThread->ReturnToContext(&frame, TRUE);
}

// Marshal a single object into a serialized blob.
void AppDomainHelper::MarshalObject(IN AppDomain *pDomain,
                                    IN OBJECTREF *orObject, // Object must be GC protected
                                    OUT BYTE    **ppbBlob,
                                    OUT DWORD    *pcbBlob)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(orObject);

    Thread *pThread = GetThread();
    AppDomain *pCurrDomain = pThread->GetDomain();
    ContextTransitionFrame frame;
    U1ARRAYREF orBlob = NULL;

    GCPROTECT_BEGIN(orBlob);

    MethodDesc *pMD = g_Mscorlib.GetMethod(METHOD__APP_DOMAIN__MARSHAL_OBJECT);

    if (pCurrDomain != pDomain)
        pThread->EnterContextRestricted(pDomain->GetDefaultContext(), &frame, TRUE);

    ARG_SLOT args[] = {
        ObjToArgSlot(*orObject)
    };

    orBlob = (U1ARRAYREF)ArgSlotToObj(pMD->Call(args, METHOD__APP_DOMAIN__MARSHAL_OBJECT));

    if (pCurrDomain != pDomain)
        pThread->ReturnToContext(&frame, TRUE);

    CopyByteArrayToEncoding(&orBlob,
                            ppbBlob,
                            pcbBlob);
    GCPROTECT_END();
}

// Marshal two objects into serialized blobs.
void AppDomainHelper::MarshalObjects(IN AppDomain *pDomain,
                                    IN OBJECTREF  *orObject1,
                                    IN OBJECTREF  *orObject2,
                                    OUT BYTE    **ppbBlob1,
                                    OUT DWORD    *pcbBlob1,
                                    OUT BYTE    **ppbBlob2,
                                    OUT DWORD    *pcbBlob2)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(orObject1);
    _ASSERTE(orObject2);

    Thread *pThread = GetThread();
    AppDomain *pCurrDomain = pThread->GetDomain();
    ContextTransitionFrame frame;

    struct _gc {
        U1ARRAYREF  orBlob1;
        U1ARRAYREF  orBlob2;
    } gc;
    ZeroMemory(&gc, sizeof(gc));

    GCPROTECT_BEGIN(gc);

    MethodDesc *pMD = g_Mscorlib.GetMethod(METHOD__APP_DOMAIN__MARSHAL_OBJECTS);

    if (pCurrDomain != pDomain)
        pThread->EnterContextRestricted(pDomain->GetDefaultContext(), &frame, TRUE);

    ARG_SLOT args[] = {
        ObjToArgSlot(*orObject1),
        ObjToArgSlot(*orObject2),
        PtrToArgSlot(&gc.orBlob2),
    };

    gc.orBlob1 = (U1ARRAYREF)ArgSlotToObj(pMD->Call(args, METHOD__APP_DOMAIN__MARSHAL_OBJECTS));

    if (pCurrDomain != pDomain)
        pThread->ReturnToContext(&frame, TRUE);

    CopyByteArrayToEncoding(&gc.orBlob1,
                            ppbBlob1,
                            pcbBlob1);

    CopyByteArrayToEncoding(&gc.orBlob2,
                            ppbBlob2,
                            pcbBlob2);
    GCPROTECT_END();
}

// Unmarshal a single object from a serialized blob.
void AppDomainHelper::UnmarshalObject(IN AppDomain  *pDomain,
                                     IN U1ARRAYREF  *porBlob,
                                     OUT OBJECTREF  *porObject)
{
    THROWSCOMPLUSEXCEPTION();

    Thread *pThread = GetThread();
    AppDomain *pCurrDomain = pThread->GetDomain();
    ContextTransitionFrame frame;

    MethodDesc *pMD = g_Mscorlib.GetMethod(METHOD__APP_DOMAIN__UNMARSHAL_OBJECT);

    if (pCurrDomain != pDomain)
        pThread->EnterContext(pDomain->GetDefaultContext(), &frame, TRUE);

    ARG_SLOT args[] = {
        ObjToArgSlot(*porBlob)
    };

    *porObject = ArgSlotToObj(pMD->Call(args, METHOD__APP_DOMAIN__UNMARSHAL_OBJECT));

    if (pCurrDomain != pDomain)
        pThread->ReturnToContext(&frame, TRUE);
}

// Unmarshal a single object from a serialized blob.
void AppDomainHelper::UnmarshalObject(IN AppDomain   *pDomain,
                                     IN BYTE        *pbBlob,
                                     IN DWORD        cbBlob,
                                     OUT OBJECTREF  *porObject)
{
    THROWSCOMPLUSEXCEPTION();

    Thread *pThread = GetThread();
    AppDomain *pCurrDomain = pThread->GetDomain();
    ContextTransitionFrame frame;
    OBJECTREF orBlob = NULL;

    MethodDesc *pMD = g_Mscorlib.GetMethod(METHOD__APP_DOMAIN__UNMARSHAL_OBJECT);

    if (pCurrDomain != pDomain)
        pThread->EnterContextRestricted(pDomain->GetDefaultContext(), &frame, TRUE);

    GCPROTECT_BEGIN(orBlob);

    AppDomainHelper::CopyEncodingToByteArray(pbBlob,
                                            cbBlob,
                                            &orBlob);

    ARG_SLOT args[] = {
        ObjToArgSlot(orBlob)
    };

    *porObject = ArgSlotToObj(pMD->Call(args, METHOD__APP_DOMAIN__UNMARSHAL_OBJECT));

    GCPROTECT_END();

    if (pCurrDomain != pDomain)
        pThread->ReturnToContext(&frame, TRUE);
}

// Unmarshal two objects from serialized blobs.
void AppDomainHelper::UnmarshalObjects(IN AppDomain   *pDomain,
                                      IN BYTE        *pbBlob1,
                                      IN DWORD        cbBlob1,
                                      IN BYTE        *pbBlob2,
                                      IN DWORD        cbBlob2,
                                      OUT OBJECTREF  *porObject1,
                                      OUT OBJECTREF  *porObject2)
{
    THROWSCOMPLUSEXCEPTION();

    Thread *pThread = GetThread();
    AppDomain *pCurrDomain = pThread->GetDomain();
    ContextTransitionFrame frame;

    MethodDesc *pMD = g_Mscorlib.GetMethod(METHOD__APP_DOMAIN__UNMARSHAL_OBJECTS);

    struct _gc {
        OBJECTREF  orBlob1;
        OBJECTREF  orBlob2;
        OBJECTREF  orObject2;
    } gc;
    ZeroMemory(&gc, sizeof(gc));

    if (pCurrDomain != pDomain)
        pThread->EnterContextRestricted(pDomain->GetDefaultContext(), &frame, TRUE);

    GCPROTECT_BEGIN(gc);

    AppDomainHelper::CopyEncodingToByteArray(pbBlob1,
                                            cbBlob1,
                                            &gc.orBlob1);

    AppDomainHelper::CopyEncodingToByteArray(pbBlob2,
                                            cbBlob2,
                                            &gc.orBlob2);

    ARG_SLOT args[] = {
        ObjToArgSlot(gc.orBlob1),
        ObjToArgSlot(gc.orBlob2),
        PtrToArgSlot(&gc.orObject2),
    };

    *porObject1 = ArgSlotToObj(pMD->Call(args, METHOD__APP_DOMAIN__UNMARSHAL_OBJECTS));
    *porObject2 = gc.orObject2;

    GCPROTECT_END();

    if (pCurrDomain != pDomain)
        pThread->ReturnToContext(&frame, TRUE);
}

// Copy an object from the given appdomain into the current appdomain.
OBJECTREF AppDomainHelper::CrossContextCopyFrom(IN DWORD dwDomainId,
                                                IN OBJECTREF *orObject) // Object must be GC protected
{
    THROWSCOMPLUSEXCEPTION();

    AppDomain *pDomain = SystemDomain::System()->GetAppDomainAtId(dwDomainId);
    if (! pDomain)
        COMPlusThrow(kAppDomainUnloadedException);
    return CrossContextCopyFrom(pDomain, orObject);
}

// Copy an object from the given appdomain into the current appdomain.
OBJECTREF AppDomainHelper::CrossContextCopyTo(IN DWORD dwDomainId,
                                                IN OBJECTREF *orObject) // Object must be GC protected
{
    THROWSCOMPLUSEXCEPTION();

    AppDomain *pDomain = SystemDomain::System()->GetAppDomainAtId(dwDomainId);
    if (! pDomain)
        COMPlusThrow(kAppDomainUnloadedException);
    return CrossContextCopyTo(pDomain, orObject);
}

// Copy an object from the given appdomain into the current appdomain.
OBJECTREF AppDomainHelper::CrossContextCopyFrom(IN AppDomain *pDomain,
                                                IN OBJECTREF *orObject) // Object must be GC protected
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(pDomain != NULL);
    _ASSERTE(pDomain != GetAppDomain());
    _ASSERTE(orObject);

    U1ARRAYREF  orBlob = NULL;
    OBJECTREF pResult = NULL;

    GCPROTECT_BEGIN(orBlob);
    AppDomainHelper::MarshalObject(pDomain, orObject, &orBlob);
    AppDomainHelper::UnmarshalObject(GetAppDomain(), &orBlob, &pResult);
    GCPROTECT_END();

    return pResult;
}

// Copy an object to the given appdomain from the current appdomain.
OBJECTREF AppDomainHelper::CrossContextCopyTo(IN AppDomain *pDomain,
                                              IN OBJECTREF *orObject)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(pDomain != NULL);
    _ASSERTE(pDomain != GetAppDomain());
    _ASSERTE(orObject);

    U1ARRAYREF  orBlob = NULL;
    OBJECTREF pResult = NULL;

    GCPROTECT_BEGIN(orBlob);
    AppDomainHelper::MarshalObject(GetAppDomain(), orObject, &orBlob);
    AppDomainHelper::UnmarshalObject(pDomain, &orBlob, &pResult);
    GCPROTECT_END();

    return pResult;
}

