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
** Header: DelegateInfo.h
**       
**
** Purpose: Native methods on System.ThreadPool
**          and its inner classes
**
** Date:  August, 1999
** 
===========================================================*/
#ifndef DELEGATE_INFO
#define DELEGATE_INFO

#include "security.h"
#include "threadpool.h"


struct DelegateInfo;
typedef DelegateInfo* DelegateInfoPtr;

struct DelegateInfo
{
    DWORD           m_appDomainId;
    OBJECTHANDLE    m_delegateHandle;
    OBJECTHANDLE    m_stateHandle;
    OBJECTHANDLE    m_eventHandle;
    OBJECTHANDLE    m_registeredWaitHandle;
    CompressedStack* m_compressedStack;
    DWORD           m_overridesCount;
    AppDomainStack  m_ADStack;
    BOOL            m_hasSecurityInfo;
    



    void SetThreadSecurityInfo( Thread* thread, StackCrawlMark* stackMark )
    {
        CompressedStack* compressedStack = Security::GetDelayedCompressedStack();

        _ASSERTE( compressedStack != NULL && "Unable to generate compressed stack for this thread" );

        // Purposely do not AddRef here since GetDelayedCompressedStack() already returns with a refcount
        // of one.

        m_compressedStack = compressedStack;

        m_hasSecurityInfo = TRUE;
    }

    void Release()
    {
        AppDomain *pAppDomain = SystemDomain::GetAppDomainAtId(m_appDomainId);

        if (pAppDomain != NULL)
        {
            DestroyHandle(m_delegateHandle);
            DestroyHandle(m_stateHandle);
            DestroyHandle(m_eventHandle);
            DestroyHandle(m_registeredWaitHandle);
        }

        if (m_compressedStack != NULL)
            m_compressedStack->Release();
    }

    static DelegateInfo  *MakeDelegateInfo(AppDomain *pAppDomain,
                                           OBJECTREF delegate,
                                           OBJECTREF state,
                                           OBJECTREF waitEvent,
                                           OBJECTREF registeredWaitObject);
};





#endif // DELEGATE_INFO
