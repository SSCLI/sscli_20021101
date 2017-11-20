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
** Header:  Threads.inl
**
** Purpose: Implements Thread inline functions
**
** Date:  August 7, 2000
**
===========================================================*/
#ifndef _THREADS_INL
#define _THREADS_INL

#include "threads.h"
#include "appdomain.hpp"

inline void Thread::SetThrowable(OBJECTREF throwable)
{
    if (m_handlerInfo.m_pThrowable != NULL)
        DestroyHandle(m_handlerInfo.m_pThrowable);
    if (throwable != NULL) {
        m_handlerInfo.m_pThrowable = GetDomain()->CreateHandle(throwable);
    } else {
        m_handlerInfo.m_pThrowable = NULL;
    }

#ifdef _DEBUG
    if (throwable != NULL)
        ValidateThrowable();
#endif
}

inline void Thread::SetKickOffDomain(AppDomain *pDomain)
{
	_ASSERTE(pDomain);
	m_pKickOffDomainId = pDomain->GetId();
}

inline AppDomain *Thread::GetKickOffDomain()
{
	_ASSERTE(m_pKickOffDomainId != 0);
    return SystemDomain::GetAppDomainAtId(m_pKickOffDomainId);
}

inline void AppDomainStack::PushDomain(AppDomain *pDomain)
{
    _ASSERTE(pDomain);
    LOG((LF_APPDOMAIN, LL_INFO100, "Thread::PushDomain [%d], count now %d\n", pDomain->GetId(), m_numDomainsOnStack+1));
    if (m_numDomainsOnStack < MAX_APPDOMAINS_TRACKED)
    {
        m_pDomains[m_numDomainsOnStack] = pDomain->GetId();
#ifdef _DEBUG
        for (int i=min(m_numDomainsOnStack, MAX_APPDOMAINS_TRACKED); i >= 0; i--) {
            AppDomain *pDomain = SystemDomain::GetAppDomainAtId( m_pDomains[i] );
            if (!pDomain)
                pDomain = SystemDomain::AppDomainBeingUnloaded();
            LOG((LF_APPDOMAIN, LL_INFO100, "    stack[%d]: [%u]\n", i, pDomain ? pDomain->GetId() : (unsigned int) -1));
        }
#endif
    }
    m_numDomainsOnStack++;
}

inline void AppDomainStack::PushDomain(DWORD domainId)
{
    LOG((LF_APPDOMAIN, LL_INFO100, "Thread::PushDomain %S, count now %d\n", SystemDomain::GetAppDomainAtId( domainId )->GetFriendlyName(FALSE), m_numDomainsOnStack+1));
    if (m_numDomainsOnStack < MAX_APPDOMAINS_TRACKED)
    {
        m_pDomains[m_numDomainsOnStack] = domainId;
#ifdef _DEBUG
        for (int i=min(m_numDomainsOnStack, MAX_APPDOMAINS_TRACKED); i >= 0; i--) {
            AppDomain *pDomain = SystemDomain::GetAppDomainAtId( m_pDomains[i] );
            if (!pDomain)
                pDomain = SystemDomain::AppDomainBeingUnloaded();
            LOG((LF_APPDOMAIN, LL_INFO100, "    stack[%d]: %S\n", i, pDomain ? pDomain->GetFriendlyName(FALSE) : L"NULL"));
        }
#endif
    }
    m_numDomainsOnStack++;
}

inline void AppDomainStack::PushDomainNoDuplicates(DWORD domainId)
{
    LOG((LF_APPDOMAIN, LL_INFO100, "Thread::PushDomain %S, count now %d\n", SystemDomain::GetAppDomainAtId( domainId )->GetFriendlyName(FALSE), m_numDomainsOnStack+1));
    if (m_numDomainsOnStack < MAX_APPDOMAINS_TRACKED)
    {
        for (int i=min(m_numDomainsOnStack, MAX_APPDOMAINS_TRACKED) - 1; i >= 0; i--) {
            if (m_pDomains[i] == domainId)
                return;
        }
        m_pDomains[m_numDomainsOnStack] = domainId;
#ifdef _DEBUG
        for (int i=min(m_numDomainsOnStack, MAX_APPDOMAINS_TRACKED); i >= 0; i--) {
            AppDomain *pDomain = SystemDomain::GetAppDomainAtId( m_pDomains[i] );
            if (!pDomain)
                pDomain = SystemDomain::AppDomainBeingUnloaded();
            LOG((LF_APPDOMAIN, LL_INFO100, "    stack[%d]: %S\n", i, pDomain ? pDomain->GetFriendlyName(FALSE) : L"NULL"));
        }
#endif
    }
    m_numDomainsOnStack++;
}


inline AppDomain *AppDomainStack::PopDomain()
{
    _ASSERTE(m_numDomainsOnStack > 0);
    m_numDomainsOnStack--;
    if (m_numDomainsOnStack >= 0 && m_numDomainsOnStack < MAX_APPDOMAINS_TRACKED)
    {
        AppDomain *pDomain = SystemDomain::GetAppDomainAtId( m_pDomains[m_numDomainsOnStack] );
        if (!pDomain)
            pDomain = SystemDomain::AppDomainBeingUnloaded();
        LOG((LF_APPDOMAIN, LL_INFO100, "Thread::PopDomain popping [%u] count now %d\n",
            pDomain ? pDomain->GetId() : (unsigned int) -1, m_numDomainsOnStack));
#ifdef _DEBUG
		if (m_numDomainsOnStack > 0)
		{
			for (int i=min(m_numDomainsOnStack-1, MAX_APPDOMAINS_TRACKED); i >= 0; i--)
            {
                AppDomain *pDomain = SystemDomain::GetAppDomainAtId( m_pDomains[i] );
                if (!pDomain)
                    pDomain = SystemDomain::AppDomainBeingUnloaded();
                LOG((LF_APPDOMAIN, LL_INFO100, "    stack[%d]: [%u]\n", i, pDomain ? pDomain->GetId() : (unsigned int) -1));
            }
		}
        AppDomain *pRet = SystemDomain::GetAppDomainAtId( m_pDomains[m_numDomainsOnStack] );
        m_pDomains[m_numDomainsOnStack] = NULL;
        return pRet;
#else
        return SystemDomain::GetAppDomainAtId( m_pDomains[m_numDomainsOnStack] );
#endif
    }
    else
	{
	    LOG((LF_APPDOMAIN, LL_INFO100, "Thread::PopDomain count now %d\n", m_numDomainsOnStack));
        return NULL;
	}
}

inline AppDomain *AppDomainStack::GetNextDomainOnStack(DWORD *pIndex)
{
    _ASSERTE(*pIndex <= MAX_APPDOMAINS_TRACKED);
    if (*pIndex > 0 && *pIndex <= MAX_APPDOMAINS_TRACKED)
    {
        (*pIndex) --;
        return SystemDomain::GetAppDomainAtId( m_pDomains[*pIndex] );
    }
    else
        return NULL;
}

inline DWORD AppDomainStack::GetNextDomainIndexOnStack(DWORD *pIndex)
{
    if (*pIndex > 0 && *pIndex <= MAX_APPDOMAINS_TRACKED)
    {
        (*pIndex) --;
        return m_pDomains[*pIndex];
    }
    else
        return (DWORD) -1;
}


#endif



