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
** Header:  AssemblySink.cpp
**
** Purpose: Implements AssemblySink, event objects that block
**          the current thread waiting for an asynchronous load
**          of an assembly to succeed.
**
** Date:  June 16, 1999
**
===========================================================*/

#include "common.h"

#include <stdlib.h>
#include "assemblysink.h"

AssemblySink::AssemblySink(AppDomain* pDomain) : m_Domain(pDomain->GetId())
{
}

ULONG AssemblySink::Release()
{
    ULONG   cRef = InterlockedDecrement(&m_cRef);
    if (!cRef) {
        Reset();
        AssemblySink* ret = this;
        // If we have a domain we keep a pool of one around. If we get an entry
        // back from the pool then we were not added to the pool and need to be deleted.
        // If we do not have a pool then we need to delete it.
        
        
        AppDomain* pDomain = NULL;

        Thread *pCurThread = GetThread();
        if(pCurThread == NULL) 
            pCurThread = SetupThread();

        BOOL toggleGC = !pCurThread->PreemptiveGCDisabled();
        if (toggleGC) 
            pCurThread->DisablePreemptiveGC();    
        
        if(m_Domain) {
            pDomain = SystemDomain::GetAppDomainAtId(m_Domain);
            if(pDomain) {
                ret = (AssemblySink*) FastInterlockCompareExchangePointer((void**) &(pDomain->m_pAsyncPool),
                                                               this,
                                                               NULL);
        }
        }

        if (toggleGC)
            pCurThread->EnablePreemptiveGC();

        if(ret != NULL) 
            delete this;
    }
    return (cRef);
}

HRESULT AssemblySink::Wait()
{
    HRESULT hr = S_OK;
    DWORD   dwReturn = 0;

    Thread* pThread = GetThread();
    BOOL fWasGCDisabled = pThread->PreemptiveGCDisabled();
    if (fWasGCDisabled)
        pThread->EnablePreemptiveGC();
    

    
    // Looping until we get a signal from fusion - which we are gaurenteed <sp> to get.
    // We do a WaitForMultipleObjects (STA and MTA) and pump messages in the STA case
    // in the call so we shouldn't freeze the system.
    do 
    {
        EE_TRY_FOR_FINALLY {
            dwReturn = pThread->DoAppropriateAptStateWait(1, &m_hEvent, FALSE, 100, TRUE);
        } EE_FINALLY {
            // If we get an exception then we will just release this sink. It may be the
            // case that the appdomain was terminated. Other exceptions will cause the
            // sink to be scavenged but this is ok. A new one will be generated for the
            // next bind.
            if(__GotException)
                m_Domain = 0;
        } EE_END_FINALLY;

    } while (dwReturn != WAIT_OBJECT_0);
    
    if (fWasGCDisabled)
        pThread->DisablePreemptiveGC();

    return hr;
}



