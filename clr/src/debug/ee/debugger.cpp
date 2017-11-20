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
//*****************************************************************************
// File: debugger.cpp
//
// Debugger runtime controller routines.
//
//*****************************************************************************

#include "stdafx.h"
#include "comsystem.h"
#include "debugdebugger.h"
#include "ipcmanagerinterface.h"
#include "../inc/common.h"
#include "comstring.h"
#include "corsvcpriv.h"
#include "perflog.h"
#include "eeconfig.h" // This is here even for retail & free builds...
#include "../../dlls/mscorrc/resource.h"
#include "remoting.h"
#include "context.h"
#include "vars.hpp"

#ifdef _DEBUG
#ifdef _MSC_VER
#pragma optimize("agpstwy", off)        // turn off compiler optimization, to avoid compiler ASSERT
#endif
#endif

#define COMPLUS_MB_SERVICE_NOTIFICATION MB_SERVICE_NOTIFICATION


#ifdef _DEBUG
    char* g_ppszAttachStateToSZ[]=
    {
    "SYNC_STATE_0" , 
    "SYNC_STATE_1" , 
    "SYNC_STATE_2" , 
    "SYNC_STATE_3" , 
    "SYNC_STATE_10", 
    "SYNC_STATE_11", 
    "SYNC_STATE_20", 
    };    
#endif


/* ------------------------------------------------------------------------ *
 * Global variables
 * ------------------------------------------------------------------------ */

Debugger                *g_pDebugger = NULL;
EEDebugInterface        *g_pEEInterface = NULL;
DebuggerRCThread        *g_pRCThread = NULL;

#ifdef ENABLE_PERF_LOG
__int64 g_debuggerTotalCycles = 0;
__int64 g_symbolTotalCycles = 0;
__int64 g_symbolCreateTotalCycles = 0;
__int64 g_symbolReadersCreated = 0;
BOOL    g_fDbgPerfOn = false;

#define START_DBG_PERF() \
    LARGE_INTEGER __cdbgstart; \
    if (g_fDbgPerfOn) \
        QueryPerformanceCounter(&__cdbgstart);

#define STOP_DBG_PERF() \
    if (g_fDbgPerfOn) \
    { \
        LARGE_INTEGER cstop; \
        QueryPerformanceCounter(&cstop); \
        g_debuggerTotalCycles += (cstop.QuadPart - __cdbgstart.QuadPart); \
    }

#define START_SYM_PERF() \
    LARGE_INTEGER __csymstart; \
    if (g_fDbgPerfOn) \
        QueryPerformanceCounter(&__csymstart);

#define STOP_SYM_PERF() \
    if (g_fDbgPerfOn) \
    { \
        LARGE_INTEGER cstop; \
        QueryPerformanceCounter(&cstop); \
        g_symbolTotalCycles += (cstop.QuadPart - __csymstart.QuadPart); \
    }

#define START_SYM_CREATE_PERF() \
    LARGE_INTEGER __csymcreatestart; \
    if (g_fDbgPerfOn) \
        QueryPerformanceCounter(&__csymcreatestart);

#define STOP_SYM_CREATE_PERF() \
    if (g_fDbgPerfOn) \
    { \
        LARGE_INTEGER cstop; \
        QueryPerformanceCounter(&cstop); \
        g_symbolCreateTotalCycles += (cstop.QuadPart - __csymcreatestart.QuadPart); \
    }
#else
#define START_DBG_PERF()
#define STOP_DBG_PERF()
#define START_SYM_PERF()
#define STOP_SYM_PERF()
#define START_SYM_CREATE_PERF()
#define STOP_SYM_CREATE_PERF()
#endif

/* ------------------------------------------------------------------------ *
 * DLL export routine
 * ------------------------------------------------------------------------ */

//
// CorDBGetInterface is exported to the Runtime so that it can call
// the Runtime Controller.
//
extern "C"{
HRESULT __cdecl CorDBGetInterface(DebugInterface** rcInterface)
{
    HRESULT hr = S_OK;
    
    if (rcInterface != NULL)
    {
        if (g_pDebugger == NULL)
        {
            LOG((LF_CORDB, LL_INFO10,
                 "CorDBGetInterface: initializing debugger.\n"));
            
            g_pDebugger = new Debugger();
            TRACE_ALLOC(g_pDebugger);

            if (g_pDebugger == NULL)
                hr = E_OUTOFMEMORY;
        }
    
        *rcInterface = g_pDebugger;
    }

    return hr;
}
}

static LONG FilterAccessViolation(LPEXCEPTION_POINTERS ep, PVOID pv)
{
    return (ep->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) 
        ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH;
}

// Validate an object. Returns E_INVALIDARG or S_OK.
HRESULT ValidateObject(Object *objPtr)
{
    HRESULT hr = S_OK;

    PAL_TRY
    {
        // NULL is certinally valid...
        if (objPtr != NULL)
        {
            EEClass *objClass = objPtr->GetClass();
            MethodTable *pMT = objPtr->GetMethodTable();
        
            if (pMT != objClass->GetMethodTable())
            {
                LOG((LF_CORDB, LL_INFO10000, "GAV: MT's don't match.\n"));
                hr = E_INVALIDARG;
                PAL_LEAVE;
            }
        }
    }
    PAL_EXCEPT_FILTER(FilterAccessViolation, NULL)
    {
        LOG((LF_CORDB, LL_INFO10000, "GAV: exception indicated ref is bad.\n"));
        hr = E_INVALIDARG;
    }
    PAL_ENDTRY

    return hr;
}   // ValidateObject

/* ------------------------------------------------------------------------ *
 * DebuggerPatchTable routines
 * ------------------------------------------------------------------------ */

void DebuggerPatchTable::ClearPatchesFromModule(Module *pModule)
{
    LOG((LF_CORDB, LL_INFO100000, "DPT::CPFM mod:0x%x (%S)\n",
        pModule, pModule->GetFileName()));

    HASHFIND f;
    for (DebuggerControllerPatch *patch = GetFirstPatch(&f);
         patch != NULL;
         patch = GetNextPatch(&f))
    {
        if (patch->dji != (DebuggerJitInfo*)DebuggerJitInfo::DJI_VERSION_INVALID && 
            patch->dji->m_fd->GetModule() == pModule)
        {
            LOG((LF_CORDB, LL_EVERYTHING, "Removing patch 0x%x\n", 
                patch));
            // we shouldn't be both hitting this patch AND
            // unloading hte module it belongs to.
            _ASSERTE(!patch->triggering);
            // Note that we don't DeactivatePatch since the
            // memory the patch was at has already gone away.
            RemovePatch(patch);
        }
    }
}


/* ------------------------------------------------------------------------ *
 * Debugger routines
 * ------------------------------------------------------------------------ */

//
// a Debugger object represents the global state of the debugger program.
//

//
// Constructor & Destructor
//

/******************************************************************************
 *
 ******************************************************************************/
Debugger::Debugger()
  :
#ifdef _DEBUG
    m_mutexCount(0),
#endif //_DEBUG
    m_fGCPrevented(FALSE),
    m_pRCThread(NULL),
    m_trappingRuntimeThreads(FALSE),
    m_stopped(FALSE),
    m_unrecoverableError(FALSE),
    m_ignoreThreadDetach(FALSE),
    m_pJitInfos(NULL),
    m_eventHandlingEvent(NULL),
    m_syncingForAttach(SYNC_STATE_0),
    m_threadsAtUnsafePlaces(0),
    m_exAttachEvent(NULL),
    m_exAttachAbortEvent(NULL),
    m_runtimeStoppedEvent(NULL),
    m_attachingForException(FALSE),
    m_exLock(0),
    m_LoggingEnabled(TRUE),
    m_pAppDomainCB(NULL),
    m_pMemBlobs(NULL),
    m_pModules(NULL),
    m_debuggerAttached(FALSE),
    m_pIDbgThreadControl(NULL),
    m_pPendingEvals(NULL),
    m_RCThreadHoldsThreadStoreLock(FALSE),
    m_heap(NULL)
{
    m_processId = GetCurrentProcessId();
}

/******************************************************************************
 *
 ******************************************************************************/
Debugger::~Debugger()
{
    HASHFIND info;

    if (m_pJitInfos != NULL)
    {
        for (DebuggerJitInfo *dji = m_pJitInfos->GetFirstJitInfo(&info);
         dji != NULL;
             dji = m_pJitInfos->GetNextJitInfo(&info))
    {
        LOG((LF_CORDB, LL_EVERYTHING, "D::~D: delete DJI 0x%x\n", dji));

        DebuggerJitInfo *djiPrev = NULL;

        while(dji != NULL)
        {
            djiPrev = dji->m_prevJitInfo;
            
            TRACE_FREE(dji);
                DeleteInteropSafe(dji);
            
            dji = djiPrev;
        }
    }

        DeleteInteropSafe(m_pJitInfos);
        m_pJitInfos = NULL;
    }

    if (m_pModules != NULL)
    {
        DeleteInteropSafe(m_pModules);
        m_pModules = NULL;
    }

    if (m_pPendingEvals)
    {
        DeleteInteropSafe(m_pPendingEvals);
        m_pPendingEvals = NULL;
    }

    if (m_pMemBlobs != NULL)
    {
        USHORT cBlobs = m_pMemBlobs->Count();
        BYTE **rgpBlobs = m_pMemBlobs->Table();

        for (int i = 0; i < cBlobs; i++)
        {
            ReleaseRemoteBuffer(rgpBlobs[i], false);            
        }

        DeleteInteropSafe(m_pMemBlobs);
    }
    
    if (m_eventHandlingEvent != NULL)
        CloseHandle(m_eventHandlingEvent);

    if (m_exAttachEvent != NULL)
        CloseHandle(m_exAttachEvent);

    if (m_CtrlCMutex != NULL)
        CloseHandle(m_CtrlCMutex);

    if (m_debuggerAttachedEvent != NULL)
        CloseHandle(m_debuggerAttachedEvent);

    if (m_exAttachAbortEvent != NULL)
        CloseHandle(m_exAttachAbortEvent);

    if (m_runtimeStoppedEvent != NULL)
        CloseHandle(m_runtimeStoppedEvent);

    DebuggerController::Uninitialize();

    DeleteCriticalSection(&m_jitInfoMutex);
    DeleteCriticalSection(&m_mutex);

    // Also clean up the AppDomain
    TerminateAppDomainIPC ();

    // Finally, destroy our heap...
    if (m_heap != NULL)
    {
        delete m_heap;
    }

    // Release any debugger thread control object we might be holding.
    // We leak this in V1
}

// Checks if the JitInfos table has been allocated, and if not does so.
HRESULT Debugger::CheckInitJitInfoTable()
{
    if (m_pJitInfos == NULL)
    {
        DebuggerJitInfoTable *pJitInfos = new (interopsafe) DebuggerJitInfoTable();
        _ASSERTE(pJitInfos);

        if (pJitInfos == NULL)
            return (E_OUTOFMEMORY);

        if (InterlockedCompareExchangePointer((PVOID *)&m_pJitInfos, (PVOID)pJitInfos, NULL) != NULL)
        {
            DeleteInteropSafe(pJitInfos);
        }
    }

    return (S_OK);
}

// Checks if the m_pModules table has been allocated, and if not does so.
HRESULT Debugger::CheckInitModuleTable()
{
    if (m_pModules == NULL)
    {
        DebuggerModuleTable *pModules = new (interopsafe) DebuggerModuleTable();
        _ASSERTE(pModules);

        if (pModules == NULL)
            return (E_OUTOFMEMORY);

        if (InterlockedCompareExchangePointer((PVOID *)&m_pModules, (PVOID)pModules, NULL) != NULL)
        {
            DeleteInteropSafe(pModules);
        }
    }

    return (S_OK);
}

// Checks if the m_pModules table has been allocated, and if not does so.
HRESULT Debugger::CheckInitPendingFuncEvalTable()
{
    if (m_pPendingEvals == NULL)
    {
        DebuggerPendingFuncEvalTable *pPendingEvals = new (interopsafe) DebuggerPendingFuncEvalTable();
        _ASSERTE(pPendingEvals);

        if (pPendingEvals == NULL)
            return (E_OUTOFMEMORY);

        if (InterlockedCompareExchangePointer((PVOID *)&m_pPendingEvals, (PVOID)pPendingEvals, NULL) != NULL)
        {
            DeleteInteropSafe(pPendingEvals);
        }
    }

    return (S_OK);
}

#ifdef _DEBUG_DJI_TABLE
// Returns the number of (official) entries in the table
ULONG DebuggerJitInfoTable::CheckDjiTable(void)
{
	USHORT cApparant = 0;
	USHORT cOfficial = 0;

    if (NULL != m_pcEntries)
    {
    	DebuggerJitInfoEntry *dcp;
    	int i = 0;
    	while (i++ <m_iEntries)
    	{
    		dcp = (DebuggerJitInfoEntry*)&(((DebuggerJitInfoEntry *)m_pcEntries)[i]);
            if(dcp->pFD != 0 && 
               dcp->pFD != (MethodDesc*)0xcdcdcdcd &&
               dcp->ji != NULL)
            {
                cApparant++;
                
                _ASSERTE( dcp->pFD == dcp->ji->m_fd );
				LOG((LF_CORDB, LL_INFO1000, "DJIT::CDT:Entry:0x%x ji:0x%x\nPrevs:\n",
					dcp, dcp->ji));
				DebuggerJitInfo *dji = dcp->ji->m_prevJitInfo;
				
				while(dji != NULL)
				{
					LOG((LF_CORDB, LL_INFO1000, "\t0x%x\n", dji));
					dji = dji->m_prevJitInfo;
				}
				dji = dcp->ji->m_nextJitInfo;
				
				LOG((LF_CORDB, LL_INFO1000, "Nexts:\n", dji));
				while(dji != NULL)
				{
					LOG((LF_CORDB, LL_INFO1000, "\t0x%x\n", dji));
					dji = dji->m_nextJitInfo;
				}
				
				LOG((LF_CORDB, LL_INFO1000, "DJIT::CDT:DONE\n",
					dcp, dcp->ji));
			}
        }

    	if (m_piBuckets == 0)
    	{
        	LOG((LF_CORDB, LL_INFO1000, "DJIT::CDT: The table is officially empty!\n"));
        	return cOfficial;
        }
        
		LOG((LF_CORDB, LL_INFO1000, "DJIT::CDT:Looking for official entries:\n"));

	    USHORT iNext = m_piBuckets[0];
		USHORT iBucket = 1;
        HASHENTRY   *psEntry = NULL;
        while (TRUE)
		{
        	while (iNext != 0xffff)
	        {
                cOfficial++;
	        
	            psEntry = EntryPtr(iNext);
				dcp = ((DebuggerJitInfoEntry *)psEntry);

				LOG((LF_CORDB, LL_INFO1000, "\tEntry:0x%x ji:0x%x @idx:0x%x @bucket:0x%x\n",
					dcp, dcp->ji, iNext, iBucket));

				iNext = psEntry->iNext;
			}

	        // Advance to the next bucket.
	        if (iBucket < m_iBuckets)
	            iNext = m_piBuckets[iBucket++];
	        else
	            break;
		}            

		LOG((LF_CORDB, LL_INFO1000, "DJIT::CDT:Finished official entries: ****************"));
    }

    return cOfficial;
}
#endif // _DEBUG_DJI_TABLE

//
// Startup initializes any necessary debugger objects, including creating
// and starting the Runtime Controller thread. Once the RC thread is started
// and we return successfully, the Debugger object can expect to have its
// event handlers called.
/*******************************************************************************/
HRESULT Debugger::Startup(void)
{
    HRESULT hr = S_OK;
    SECURITY_ATTRIBUTES *pSA = NULL;

    _ASSERTE(g_pEEInterface != NULL);

#ifdef ENABLE_PERF_LOG
    // Should we track perf info?
    char buf[32];
    g_fDbgPerfOn = GetEnvironmentVariableA("DBG_PERF_OUTPUT", buf, sizeof(buf));
#endif
    
    // First, initialize our heap.
    m_heap = new DebuggerHeap();

    if (m_heap != NULL)
    {
        m_heap->Init("Debugger Heap");
    }
    else
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    
    // Must be done before the RC thread is initialized.
    // The helper thread will be able to determine if someone was trying
    // to attach before the runtime was loaded and set the appropriate
    // flags that will cause CORDebuggerAttached to return true
    if (CORLaunchedByDebugger())
        DebuggerController::Initialize();

    // We must initialize the debugger lock before kicking off the
    // helper thread. The helper thread will try to use the lock right
    // away to guard against certian race conditions.
    InitializeCriticalSection(&m_mutex);

#ifdef _DEBUG
    m_mutexOwner = 0;
#endif    

    // Create the runtime controller thread, a.k.a, the debug helper thread.
    m_pRCThread = new (interopsafe) DebuggerRCThread(this);
    TRACE_ALLOC(m_pRCThread);

    if (m_pRCThread != NULL)
    {
        hr = m_pRCThread->Init();

        if (SUCCEEDED(hr))
            hr = m_pRCThread->Start();

        if (!SUCCEEDED(hr))
        {
            TRACE_FREE(m_pRCThread);
            DeleteInteropSafe(m_pRCThread);
            m_pRCThread = NULL;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }
        
    NAME_EVENT_BUFFER;
    m_eventHandlingEvent = WszCreateEvent(NULL, FALSE, TRUE, NAME_EVENT(L"EventHandlingEvent"));
    
    if (m_eventHandlingEvent == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }

    m_exAttachEvent = WszCreateEvent(NULL, TRUE, FALSE, NAME_EVENT(L"ExAttachEvent"));
    
    if (m_exAttachEvent == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }

    m_exAttachAbortEvent = WszCreateEvent(NULL, TRUE, FALSE, NAME_EVENT(L"ExAttachAbortEvent"));

    if (m_exAttachAbortEvent == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }

    m_runtimeStoppedEvent = WszCreateEvent(NULL, FALSE, FALSE, NAME_EVENT(L"RuntimeStoppedEvent"));

    if (m_runtimeStoppedEvent == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }

    InitializeCriticalSection(&m_jitInfoMutex);

    m_CtrlCMutex = WszCreateEvent(NULL, FALSE, FALSE, NAME_EVENT(L"CtrlCMutex"));

    if (m_CtrlCMutex == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }
    
	WCHAR tmpName[256];

        swprintf(tmpName, CorDBDebuggerAttachedEvent, GetCurrentProcessId());

    hr = g_pIPCManagerInterface->GetSecurityAttributes(GetCurrentProcessId(), &pSA);

    if (FAILED(hr))
        goto exit;
    
    LOG((LF_CORDB, LL_INFO10000, "DRCT::I: creating DebuggerAttachedEvent with name [%S]\n", tmpName));
    m_debuggerAttachedEvent = WszCreateEvent(pSA, TRUE, FALSE, tmpName);
    
    if (m_debuggerAttachedEvent == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }
    
    m_DebuggerHandlingCtrlC = FALSE;

    // Also initialize the AppDomainEnumerationIPCBlock 
    m_pAppDomainCB = g_pIPCManagerInterface->GetAppDomainBlock();

    if (m_pAppDomainCB == NULL)
    {
       LOG((LF_CORDB, LL_INFO100,
             "D::S: Failed to get AppDomain IPC block from IPCManager.\n"));
       hr = E_FAIL;
       goto exit;
    }

    hr = InitAppDomainIPC();
    
    if (hr != S_OK)
    {
       LOG((LF_CORDB, LL_INFO100,
             "D::S: Failed to Initialize AppDomain IPC block.\n"));

       goto exit;
    }

    m_pMemBlobs = new (interopsafe) UnorderedBytePtrArray();
    
    if (m_pMemBlobs == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    
    // We set m_debuggerAttachedEvent to indicate that a debugger is now attached to the process. This is used by the
    // interop debugging hijacks to ensure that a debugger is attached enough to the process to proceed through the
    // hijacks. We do this here when the process was launched by a debugger since we know 100% for sure that there is a
    // debugger attached now.
    if (CORLaunchedByDebugger())
        SetEvent(m_debuggerAttachedEvent);
        
exit:
    g_pIPCManagerInterface->DestroySecurityAttributes(pSA);
    
    return hr;
}


/******************************************************************************
// Called to set the interface that the Runtime exposes to us.
 ******************************************************************************/
void Debugger::SetEEInterface(EEDebugInterface* i)
{
    g_pEEInterface = i;
}


/******************************************************************************
// Called to shut down the debugger. This stops the RC thread and cleans
// the object up.
 ******************************************************************************/
void Debugger::StopDebugger(void)
{
    if (m_pRCThread != NULL)
    {
        HRESULT hr = m_pRCThread->Stop();

        if (SUCCEEDED(hr))
        {
            TRACE_FREE(m_pRCThread);
            DeleteInteropSafe(m_pRCThread);
            m_pRCThread = NULL;
        }
    }

    delete this;

#ifdef ENABLE_PERF_LOG
    if (g_fDbgPerfOn)
    {
        LARGE_INTEGER cycleFreq;

        QueryPerformanceFrequency(&cycleFreq);

        double dtot, freq, stot, sctot;
        
        freq = (double)cycleFreq.QuadPart;
        dtot = (double)g_debuggerTotalCycles;
        stot = (double)g_symbolTotalCycles;
        sctot = (double)g_symbolCreateTotalCycles;

        WCHAR buf[1024];

#define SHOWINFOCYC(_b, _m, _d) \
        swprintf((_b), L"%-30S%12.3g cycles", (_m), (_d)); \
        fprintf(stderr, "%S\n", buf);
                                              
#define SHOWINFOSEC(_b, _m, _d) \
        swprintf((_b), L"%-30S%12.3g sec", (_m), (_d)); \
        fprintf(stderr, "%S\n", buf);
                                              
#define SHOWINFO(_b, _m, _d) \
        swprintf((_b), L"%-30S%12d", (_m), (_d)); \
        fprintf(stderr, "%S\n", buf);
                                              
        SHOWINFOCYC(buf, L"Debugger Tracking Cycles", (dtot - stot - sctot));
        SHOWINFOSEC(buf, L"Debugger Tracking Time", (dtot - stot - sctot)/freq);
        SHOWINFOCYC(buf, L"Symbol Store Cycles", stot);
        SHOWINFOSEC(buf, L"Symbol Store Time", stot/freq);
        SHOWINFOCYC(buf, L"Symbol Store Create Cycles", sctot);
        SHOWINFOSEC(buf, L"Symbol Store Create Time", sctot/freq);
        SHOWINFO(buf, L"Symbol readers created", g_symbolReadersCreated);
    }
#endif
}


/* ------------------------------------------------------------------------ *
 * JIT Interface routines
 * ------------------------------------------------------------------------ */

//
// This is only fur internal debugging.
//
#ifdef LOGGING
static void _dumpVarNativeInfo(ICorJitInfo::NativeVarInfo* vni)
{
    LOG((LF_CORDB, LL_INFO1000000, "Var %02d: 0x%04x-0x%04x vlt=",
            vni->varNumber,
            vni->startOffset, vni->endOffset,
            vni->loc.vlType));

    switch (vni->loc.vlType)
    {
    case ICorJitInfo::VLT_REG:
        LOG((LF_CORDB, LL_INFO1000000, "REG reg=%d\n", vni->loc.vlReg.vlrReg));
        break;
        
    case ICorJitInfo::VLT_STK:
        LOG((LF_CORDB, LL_INFO1000000, "STK reg=%d off=0x%04x (%d)\n",
             vni->loc.vlStk.vlsBaseReg,
             vni->loc.vlStk.vlsOffset,
             vni->loc.vlStk.vlsOffset));
        break;
        
    case ICorJitInfo::VLT_REG_REG:
        LOG((LF_CORDB, LL_INFO1000000, "REG_REG reg1=%d reg2=%d\n",
             vni->loc.vlRegReg.vlrrReg1,
             vni->loc.vlRegReg.vlrrReg2));
        break;
        
    case ICorJitInfo::VLT_REG_STK:
        LOG((LF_CORDB, LL_INFO1000000, "REG_STK reg=%d basereg=%d off=0x%04x (%d)\n",
             vni->loc.vlRegStk.vlrsReg,
             vni->loc.vlRegStk.vlrsStk.vlrssBaseReg,
             vni->loc.vlRegStk.vlrsStk.vlrssOffset,
             vni->loc.vlRegStk.vlrsStk.vlrssOffset));
        break;
        
    case ICorJitInfo::VLT_STK_REG:
        LOG((LF_CORDB, LL_INFO1000000, "STK_REG basereg=%d off=0x%04x (%d) reg=%d\n",
             vni->loc.vlStkReg.vlsrStk.vlsrsBaseReg,
             vni->loc.vlStkReg.vlsrStk.vlsrsOffset,
             vni->loc.vlStkReg.vlsrStk.vlsrsOffset,
             vni->loc.vlStkReg.vlsrReg));
        break;
        
    case ICorJitInfo::VLT_STK2:
        LOG((LF_CORDB, LL_INFO1000000, "STK_STK reg=%d off=0x%04x (%d)\n",
             vni->loc.vlStk2.vls2BaseReg,
             vni->loc.vlStk2.vls2Offset,
             vni->loc.vlStk2.vls2Offset));
        break;
        
    case ICorJitInfo::VLT_FPSTK:
        LOG((LF_CORDB, LL_INFO1000000, "FPSTK reg=%d\n",
             vni->loc.vlFPstk.vlfReg));
        break;

    case ICorJitInfo::VLT_FIXED_VA:
        LOG((LF_CORDB, LL_INFO1000000, "FIXED_VA offset=%d (%d)\n",
             vni->loc.vlFixedVarArg.vlfvOffset,
             vni->loc.vlFixedVarArg.vlfvOffset));
        break;
        
        
    default:
        LOG((LF_CORDB, LL_INFO1000000, "???\n"));
        break;
    }
}
#endif

/******************************************************************************
 *
 ******************************************************************************/
DebuggerJitInfo *Debugger::CreateJitInfo(MethodDesc *fd)
{
    //
    // Create a jit info struct to hold info about this function.
    //
//    CHECK_DJI_TABLE_DEBUGGER;

    //
    //
    DebuggerJitInfo *ji = new (interopsafe) DebuggerJitInfo(fd);

    TRACE_ALLOC(ji);

    if (ji != NULL )
    {
        //
        // Lock a mutex when changing the table.
        //
        HRESULT hr;
        hr =g_pDebugger->InsertAtHeadOfList( ji );

        if (FAILED(hr))
        {
            DeleteInteropSafe(ji);
            return NULL;
        }
    }

    return ji;
}

/******************************************************************************
 *
 ******************************************************************************/
void DebuggerJitInfo::SetVars(ULONG32 cVars, ICorDebugInfo::NativeVarInfo *pVars, bool fDelete)
{
    _ASSERTE(m_varNativeInfo == NULL);

    m_varNativeInfo = pVars;
    m_varNativeInfoCount = cVars;
    m_varNeedsDelete = fDelete;

    LOG((LF_CORDB, LL_INFO1000000, "D::sV: var count is %d\n",
         m_varNativeInfoCount));

#ifdef LOGGING    
    for (unsigned int i = 0; i < m_varNativeInfoCount; i++)
    {
        ICorJitInfo::NativeVarInfo* vni = &(m_varNativeInfo[i]);
        _dumpVarNativeInfo(vni);
    }
#endif    
}

/*
class MapSortIL:  A template class that will sort an array of DebuggerILToNativeMap.  This class is intended to be instantiated on the stack / in temporary storage, and used to reorder the sequence map.
 */
class MapSortIL : public CQuickSort<DebuggerILToNativeMap>
{
  public:
    //Constructor
    MapSortIL(DebuggerILToNativeMap *map, 
              int count)
      : CQuickSort<DebuggerILToNativeMap>(map, count) {}

    //Comparison operator
    int Compare(DebuggerILToNativeMap *first, 
                DebuggerILToNativeMap *second) 
    {
        //PROLOGs go first
        if (first->ilOffset == (ULONG) ICorDebugInfo::PROLOG
            && second->ilOffset == (ULONG) ICorDebugInfo::PROLOG)
        {
            return 0;
        } else if (first->ilOffset == (ULONG) ICorDebugInfo::PROLOG)
        {
            return -1;
        } else if (second->ilOffset == (ULONG) ICorDebugInfo::PROLOG)
        {
            return 1;
        }
        //NO_MAPPING go last
        else if (first->ilOffset == (ULONG) ICorDebugInfo::NO_MAPPING
            && second->ilOffset == (ULONG) ICorDebugInfo::NO_MAPPING)
        {
            return 0;
        } else if (first->ilOffset == (ULONG) ICorDebugInfo::NO_MAPPING)
        {
            return 1;
        } else if (second->ilOffset == (ULONG) ICorDebugInfo::NO_MAPPING)
        {
            return -1;
        }
        //EPILOGs go next-to-last
        else if (first->ilOffset == (ULONG) ICorDebugInfo::EPILOG
            && second->ilOffset == (ULONG) ICorDebugInfo::EPILOG)
        {
            return 0;
        } else if (first->ilOffset == (ULONG) ICorDebugInfo::EPILOG)
        {
            return 1;
        } else if (second->ilOffset == (ULONG) ICorDebugInfo::EPILOG)
        {
            return -1;
        }
        //normal offsets compared otherwise
        else if (first->ilOffset < second->ilOffset)
            return -1;
        else if (first->ilOffset == second->ilOffset)
            return 0;
        else
            return 1;
    }
};

/*
class MapSortNative:  A template class that will sort an array of DebuggerILToNativeMap by the nativeStartOffset field.  This class is intended to be instantiated on the stack / in temporary storage, and used to reorder the sequence map.
*/
class MapSortNative : public CQuickSort<DebuggerILToNativeMap>
{
  public:
    //Constructor
    MapSortNative(DebuggerILToNativeMap *map,
                  int count) 
      : CQuickSort<DebuggerILToNativeMap>(map, count) {}

    //Returns -1,0,or 1 if first's nativeStartOffset is less than, equal to, or greater than second's
    int Compare(DebuggerILToNativeMap *first, 
                DebuggerILToNativeMap *second)
    {
        if (first->nativeStartOffset < second->nativeStartOffset)
            return -1;
        else if (first->nativeStartOffset == second->nativeStartOffset)
            return 0;
        else
            return 1;
    }
};

/******************************************************************************
 *
 ******************************************************************************/
HRESULT DebuggerJitInfo::SetBoundaries(ULONG32 cMap, ICorDebugInfo::OffsetMapping *pMap)
{
    _ASSERTE((cMap == 0) == (pMap == NULL));
    _ASSERTE(m_sequenceMap == NULL);

    SIZE_T ilLast = 0;

    //
    //
    m_sequenceMapCount = cMap;
    m_sequenceMap = (DebuggerILToNativeMap *)new (interopsafe) DebuggerILToNativeMap[m_sequenceMapCount];
    if (NULL == m_sequenceMap)
        return E_OUTOFMEMORY;

    ICorDebugInfo::OffsetMapping *pMapEntry = pMap;
    DebuggerILToNativeMap *m = m_sequenceMap;
    DebuggerILToNativeMap *mEnd = m + m_sequenceMapCount;
    
    //
    //

    // If the IL stream was processed out of order than the debugger can't
    // depend on the start of the next instruction being the end of the current one.
    // Currently EconoJIT does out of order processing.
    bool OutOfOrderILProcessing = false;
    IJitManager* pEEJM = ExecutionManager::FindJitMan((SLOT)m_addrOfCode);

    if ( pEEJM )
    {
      if ( pEEJM->GetCodeType() == miManaged_IL_EJIT )
        OutOfOrderILProcessing = true;
    }
    else
      OutOfOrderILProcessing = true;

    while (m < mEnd)
    {
        _ASSERTE(m>=m_sequenceMap);

        ilLast = max( (int)ilLast, (int)pMapEntry->ilOffset );
            
        // Simply copy everything over, since we translate to
        // CorDebugMappingResults immediately prior to handing
        // back to user...
        m->nativeStartOffset = pMapEntry->nativeOffset;

        // Keep in mind that if we have an instrumented code translation
        // table, we may have asked for completely different IL offsets
        // than the user thinks we did.....
        m->ilOffset = TranslateToInstIL(pMapEntry->ilOffset,
                                        bInstrumentedToOriginal);

        // Grab the source into the separate array.
        m->source = pMapEntry->source;
    
        pMapEntry++;
        
        if (m>m_sequenceMap && (m-1)->ilOffset == m->ilOffset)
        {
            // JIT gave us an extra entry (probably zero), so mush
            // it into the one we've already got.
            m_sequenceMapCount--;
            m--;
            _ASSERTE(m>=m_sequenceMap);
            mEnd--;
        }

        if ( m + 1 >=  mEnd )
        {
             m->nativeEndOffset = 0;
             m->source = (ICorDebugInfo::SourceTypes)
                ((DWORD)m->source | 
                (DWORD)ICorDebugInfo::NATIVE_END_OFFSET_UNKNOWN);
        }
        else if (!OutOfOrderILProcessing)
	{        
             if (m + 1 < mEnd)
               m->nativeEndOffset = pMapEntry->nativeOffset;
        }
        else
	{
            // Because we compile the IL out of order the start of the next instruction is not
            // necessarily the end of the current one. We are forced to search for the instruction
            // which has a start offset that is the smallest start offset that is bigger then the 
            // current start offset
            ICorDebugInfo::OffsetMapping *pMapEntryTemp = pMap;
            m->nativeEndOffset = (unsigned)-1;
            // For all entries in the map
            for (unsigned int i = 0; i < cMap; i++ )
	    {
              if ( pMapEntryTemp->nativeOffset > m->nativeStartOffset && 
                   pMapEntryTemp->nativeOffset < m->nativeEndOffset )
	              m->nativeEndOffset =  pMapEntryTemp->nativeOffset;
              pMapEntryTemp++;
	    }
        }

        

#if defined(DEBUG) 
        //while we're doing this, check that it's sorted by  native order
        if (m+1 < mEnd && !OutOfOrderILProcessing)
            _ASSERTE( m->nativeStartOffset < (m+1)->nativeStartOffset );

#endif //DEBUG
            
        m++;
        _ASSERTE(m>=m_sequenceMap);
    }

    m_lastIL = ilLast;

    MapSortIL isort(m_sequenceMap, m_sequenceMapCount);

    isort.Sort();
      
    m_sequenceMapSorted = true;

    LOG((LF_CORDB, LL_INFO1000000, "D::sB: boundary count is %d\n",
         m_sequenceMapCount));

#ifdef LOGGING    
    for (unsigned int i = 0; i < m_sequenceMapCount; i++)
    {
        if( m_sequenceMap[i].ilOffset ==
            (SIZE_T) ICorDebugInfo::PROLOG )
            LOG((LF_CORDB, LL_INFO1000000,
                 "D::sB: PROLOG               --> 0x%08x -- 0x%08x",
                 m_sequenceMap[i].nativeStartOffset,
                 m_sequenceMap[i].nativeEndOffset));
        else if ( m_sequenceMap[i].ilOffset ==
                  (SIZE_T) ICorDebugInfo::EPILOG )
            LOG((LF_CORDB, LL_INFO1000000,
                 "D::sB: EPILOG              --> 0x%08x -- 0x%08x",
                 m_sequenceMap[i].nativeStartOffset,
                 m_sequenceMap[i].nativeEndOffset));
        else if ( m_sequenceMap[i].ilOffset ==
                  (SIZE_T) ICorDebugInfo::NO_MAPPING )
            LOG((LF_CORDB, LL_INFO1000000,
                 "D::sB: NO MAP              --> 0x%08x -- 0x%08x",
                 m_sequenceMap[i].nativeStartOffset,
                 m_sequenceMap[i].nativeEndOffset));
        else
            LOG((LF_CORDB, LL_INFO1000000,
                 "D::sB: 0x%04x (Real:0x%04x) --> 0x%08x -- 0x%08x",
                 m_sequenceMap[i].ilOffset,
                 TranslateToInstIL(m_sequenceMap[i].ilOffset,
                                   bOriginalToInstrumented),
                 m_sequenceMap[i].nativeStartOffset,
                 m_sequenceMap[i].nativeEndOffset));

        LOG((LF_CORDB, LL_INFO1000000, " Src:0x%x\n", m_sequenceMap[i].source));
    }
#endif //LOGGING
    
    return S_OK;
}

/******************************************************************************
 *
 ******************************************************************************/
ICorDebugInfo::SourceTypes DebuggerJitInfo::GetSrcTypeFromILOffset(SIZE_T ilOffset)
{
    BOOL exact = FALSE;
    DebuggerILToNativeMap *pMap = MapILOffsetToMapEntry(ilOffset, &exact);

    LOG((LF_CORDB, LL_INFO100000, "DJI::GSTFILO: for il 0x%x, got entry 0x%x,"
        "(il 0x%x) nat 0x%x to 0x%x, SourceTypes 0x%x, exact:%x\n", ilOffset, pMap, 
        pMap->ilOffset, pMap->nativeStartOffset, pMap->nativeEndOffset, pMap->source,
        exact));

    if (!exact)
    {
        return ICorDebugInfo::SOURCE_TYPE_INVALID;
    }
    
    return pMap->source;
}

/******************************************************************************
// void Debugger::JITBeginning()   JITBeginning is called 
//  from vm/jitinterface.cpp when a JIT is about to occur for a given function,
// either if debug info needs to be tracked for the method, or if a debugger is attached.
//  Remember that this gets called before
//  the start address of the MethodDesc gets set, and so methods like
//  GetFunctionAddress & GetFunctionSize won't work.
// MethodDesc* fd:  MethodDesc of the method about to be
//      JITted.
// bool trackJITInfo:
 ******************************************************************************/
void Debugger::JITBeginning(MethodDesc* fd, bool trackJITInfo)
{
    START_DBG_PERF();

    _ASSERTE(trackJITInfo || CORDebuggerAttached());
    if (CORDBUnrecoverableError(this) || !trackJITInfo)
        goto Exit;

    LOG((LF_CORDB,LL_INFO10000,"De::JITBeg: %s::%s\n", 
        fd->m_pszDebugClassName,fd->m_pszDebugMethodName));

    // We don't necc. want to create another DJI.  In particular, if
    // 1) we're reJITting pitched code then we don't want to create
    // yet another DJI.
    //
    //
    DebuggerJitInfo * prevJi;
    prevJi = GetJitInfo(fd, NULL);
    
#ifdef LOGGING      
    if (prevJi != NULL )
        LOG((LF_CORDB,LL_INFO10000,"De::JITBeg: Got DJI 0x%x, "
            "from 0x%x to 0x%x\n",prevJi, prevJi->m_addrOfCode, 
            prevJi->m_addrOfCode+prevJi->m_sizeOfCode));
#endif //LOGGING

    // If this is a re-JIT, then bail now before fresh or EnC stuff
    // If this is a JIT that has been begun by the profiler, then
    // don't create another one...
    if (!((prevJi != NULL) && ((prevJi->m_codePitched == true) ||
                               (prevJi->m_jitComplete == false))))
        CreateJitInfo(fd);

Exit:    
    STOP_DBG_PERF();
}   

/******************************************************************************
// void Debugger::JITComplete():   JITComplete is called by 
// the jit interface when the JIT completes, either if debug info needs
// to be tracked for the method, or if a debugger is attached. If newAddress is 
// NULL then the JIT failed.  Remember that this gets called before
// the start address of the MethodDesc gets set, and so methods like
// GetFunctionAddress & GetFunctionSize won't work.
// MethodDesc* fd:  MethodDesc of the code that's been JITted
// BYTE* newAddress:  The address of that the method begins at
//                                                                       
 ******************************************************************************/
void Debugger::JITComplete(MethodDesc* fd, BYTE* newAddress, SIZE_T sizeOfCode, bool trackJITInfo)
{
    START_DBG_PERF();
    
    _ASSERTE(trackJITInfo || CORDebuggerAttached());
    if (CORDBUnrecoverableError(this))
        goto Exit;

    LOG((LF_CORDB, LL_INFO100000,
         "D::JitComplete: address of methodDesc 0x%x (%s::%s)"
         "jitted code is 0x%08x, Size::0x%x \n",fd,
         fd->m_pszDebugClassName,fd->m_pszDebugMethodName,
         newAddress, sizeOfCode));

    if (!trackJITInfo)
    {
        _ASSERTE(CORDebuggerAttached());
#ifdef _DEBUG
        HRESULT hr =
#endif
        MapAndBindFunctionPatches(NULL, fd, newAddress);
        _ASSERTE(SUCCEEDED(hr));
        goto Exit;
    }

    DebuggerJitInfo *ji;
    ji = GetJitInfo(fd,NULL);
    
    if (newAddress == 0 && sizeOfCode == 0)
    {
        // JIT is actually telling us that the JIT aborted -
        // toss the DJI
        _ASSERTE(ji != NULL); //must be something there.
        _ASSERTE(ji->m_jitComplete == false); //must have stopped 1/2 way through
        
        DeleteHeadOfList( fd );
        LOG((LF_CORDB, LL_INFO100000, "The JIT actually gave us"
            "an error result, so the version that hasn't been"
            " JITted properly has been ditched\n"));
        goto Exit;
    }

    if (ji == NULL)
    {
        // setBoundaries may run out of mem & eliminated the DJI
        LOG((LF_CORDB,LL_INFO10000,"De::JitCo:Got NULL Ptr - out of mem?\n"));
        goto Exit; 
    }
    else
    {
        ji->m_addrOfCode = PTR_TO_CORDB_ADDRESS(newAddress);
        ji->m_sizeOfCode = sizeOfCode;
        ji->m_jitComplete = true;
        ji->m_codePitched = false;
        
        LOG((LF_CORDB,LL_INFO10000,"De::JITCo:Got DJI 0x%x,"
            "from 0x%x to 0x%xvarCount=%d  seqCount=%d\n",ji,
            (ULONG)ji->m_addrOfCode, 
            (ULONG)ji->m_addrOfCode+(ULONG)ji->m_sizeOfCode,
            ji->m_varNativeInfoCount,
            ji->m_sequenceMapCount));
        
        HRESULT hr = S_OK;

        // Don't need to do this unless a debugger is attached.
        if (CORDebuggerAttached())
        {
            hr = MapAndBindFunctionPatches(ji, fd, newAddress);
            _ASSERTE(SUCCEEDED(hr));
        }

        if (SUCCEEDED(hr = CheckInitJitInfoTable()))
        {
            _ASSERTE(ji->m_fd != NULL);
            if (!m_pJitInfos->EnCRemapSentForThisVersion(ji->m_fd->GetModule(),
                                                         ji->m_fd->GetMemberDef(),
                                                         ji->m_nVersion))
            {
                LOG((LF_CORDB,LL_INFO10000,"De::JITCo: Haven't yet remapped this,"
                    "so go ahead & send the event\n"));
                LockAndSendEnCRemapEvent(fd, TRUE);
            }
        }
        else
            _ASSERTE(!"Error allocating jit info table");
    }

Exit:
    STOP_DBG_PERF();
}

/******************************************************************************
// void Debugger::FunctionStubInitialized()  This is called by the JIT
//  into the debugger to tell the Debuger that the function stub has been
//  initialized.  This is called shortly after Debugger::JITComplete.
//  We use this opportunity to invoke BindFunctionPatches, which will 
//  ensure that any patches for this method bound by DebuggerFunctionKey
//  are rebound to an actual address.
//  Remember that this gets called before
//  the start address of the MethodDesc gets set, and so methods like
//  GetFunctionAddress & GetFunctionSize won't work.
// MethodDesc * fd:  MethodDesc of the method that's been initialized
// const BYTE * code:  Where the method was JITted to.
//                                                                            
 ******************************************************************************/
void Debugger::FunctionStubInitialized(MethodDesc *fd, const BYTE *code)
{
    // Remember that there may not be a DJI for this method, if
    // earlier errors prevented us from allocating one.

    // Remember also that if we grab a lock here, we'll have to disable
    // cooperative GC beforehand (see JITBeginnging, JITComplete,etc)
}

/******************************************************************************
// void Debugger::PitchCode()  This is called when the 
// FJIT tosses some code out - we should do all the work
// we need to in order to prepare the function to have
// it's native code removed
// MethodDesc * fd:  MethodDesc of the method to be pitched
// const BYTE *pbAddr:
 ******************************************************************************/
void Debugger::PitchCode( MethodDesc *fd, const BYTE *pbAddr )
{
    _ASSERTE( fd != NULL );

    LOG((LF_CORDB,LL_INFO10000,"D:PC: Pitching method %s::%s 0x%x\n", 
        fd->m_pszDebugClassName, fd->m_pszDebugMethodName,fd ));
    
    //ask for the JitInfo no matter what it's state
    DebuggerJitInfo *ji = GetJitInfo( fd, pbAddr );
    
    if ( ji != NULL )
    {
        LOG((LF_CORDB,LL_INFO10000,"De::PiCo: For addr 0x%x, got "
            "DJI 0x%x, from 0x%x, size:0x%x\n",pbAddr, ji, 
            ji->m_addrOfCode, ji->m_sizeOfCode));
            
        DebuggerController::UnbindFunctionPatches( fd );
        ji->m_jitComplete = false;
        ji->m_codePitched = true;
        ji->m_addrOfCode = (CORDB_ADDRESS)NULL;
        ji->m_sizeOfCode = 0;
    }
}

/******************************************************************************
// void Debugger::MovedCode():  This is called when the 
// code has been moved.  Currently, code that the FJIT doesn't
// pitch, it moves to another spot, and then tells us about here.
// This method should be called after the code has been copied
// over, but while the original code is still present, so we can
// change the original copy (removing patches & such).
// Note that since the code has already been moved, we need to 
// save the opcodes for the rebind.
// MethodDesc * fd:  MethodDesc of the method to be pitched
// const BYTE * pbNewAddress:  Address that it's being moved to
 ******************************************************************************/
void Debugger::MovedCode( MethodDesc *fd, const BYTE *pbOldAddress,
    const BYTE *pbNewAddress)
{
    _ASSERTE( fd != NULL );
    _ASSERTE( pbOldAddress != NULL );
    _ASSERTE( pbNewAddress != NULL );

    LOG((LF_CORDB, LL_INFO1000, "De::MoCo: %s::%s moved from 0x%8x to 0x%8x\n",
        fd->m_pszDebugClassName, fd->m_pszDebugMethodName,
        pbOldAddress, pbNewAddress ));

    DebuggerJitInfo *ji = GetJitInfo(fd, pbOldAddress);
    if( ji != NULL )
    {
        LOG((LF_CORDB,LL_INFO10000,"De::MoCo: For code 0x%x, got DJI 0x%x, "
            "from 0x%x to 0x%x\n", pbOldAddress, ji, ji->m_addrOfCode, 
            ji->m_addrOfCode+ji->m_sizeOfCode));

        ji->m_addrOfCode = PTR_TO_CORDB_ADDRESS(pbNewAddress);
    }
    
    DebuggerController::UnbindFunctionPatches( fd, true);
    DebuggerController::BindFunctionPatches(fd, pbNewAddress);
}

/******************************************************************************
 *
 ******************************************************************************/
SIZE_T Debugger::GetArgCount(MethodDesc *fd,BOOL *fVarArg)
{
    // Create a MetaSig for the given method's sig. (Eaiser than
    // picking the sig apart ourselves.)
    PCCOR_SIGNATURE pCallSig = fd->GetSig();

    MetaSig *msig = new (interopsafe) MetaSig(pCallSig, g_pEEInterface->MethodDescGetModule(fd), MetaSig::sigMember);

    // Get the arg count.
    UINT32 NumArguments = msig->NumFixedArgs();

    // Account for the 'this' argument.
    if (!(g_pEEInterface->MethodDescIsStatic(fd)))
        NumArguments++;

    // Is this a VarArg's function?
    if (msig->IsVarArg() && fVarArg != NULL)
    {
        NumArguments++;
        *fVarArg = true;
    }
    
    // Destroy the MetaSig now that we're done using it.
    DeleteInteropSafe(msig);

    return NumArguments;
}

/******************************************************************************
    DebuggerJitInfo * Debugger::GetJitInfo():   GetJitInfo
    will return a pointer to a DebuggerJitInfo.  If the DJI
    doesn't exist, or it does exist, but the method has actually 
    been pitched (and the caller wants pitched methods filtered out),
    then we'll return NULL.

    MethodDesc* fd:  MethodDesc for the method we're interested in.
    const BYTE * pbAddr:  Address within the code, to indicate which
            version we want.  If this is NULL, then we want the
            head of the DebuggerJitInfo list, whether it's been
            JITted or not.
 ******************************************************************************/
DebuggerJitInfo *Debugger::GetJitInfo(MethodDesc *fd, 
                                      const BYTE *pbAddr,
                                      bool fByVersion)
{
    DebuggerJitInfo *info = NULL;

    LockJITInfoMutex();

//    CHECK_DJI_TABLE_DEBUGGER;
    
    if (m_pJitInfos != NULL)
        info = m_pJitInfos->GetJitInfo(fd);
    
   

    // If code pitching is enabled the address may be invalid
    if ( g_pEEInterface->GetEEState() & EEDebugInterface::EE_STATE_CODE_PITCHING )
    {
        // Couldn't find info by address, but since ENC &&
        // pitching don't work together, we know that we'll
        // have only one version anyways.
        if (m_pJitInfos != NULL)
           info = m_pJitInfos->GetJitInfo(fd);

        LOG((LF_CORDB, LL_INFO1000, "*** *** DJI not found, but we're "
                                    "pitching, so use 0x%x as DJI\n", info ));

        _ASSERTE(info == NULL || fd == info->m_fd);
    }
    else if (fByVersion)
        info = info->GetJitInfoByVersionNumber((SIZE_T)pbAddr, GetVersionNumber(fd));
    else
    {
        if (pbAddr != NULL )
        {
            info = info->GetJitInfoByAddress(pbAddr);
            
            if (info == NULL) //may have been given address of a thunk
            {
                LOG((LF_CORDB,LL_INFO1000,"Couldn't find a DJI by address 0x%x, "
                    "so it might be a stub or thunk\n", pbAddr));
                TraceDestination trace;

                g_pEEInterface->TraceStub(pbAddr, &trace);
                if ((trace.type == TRACE_MANAGED) &&
                    (pbAddr != trace.address))
                {
                    LOG((LF_CORDB,LL_INFO1000,"Address thru thunk"
                        ": 0x%x\n", trace.address));
                    info = g_pDebugger->GetJitInfo(fd,trace.address);
                }
#ifdef LOGGING  
                else
                {
                    _ASSERTE( trace.type != TRACE_UNJITTED_METHOD ||
                            fd == (MethodDesc*)trace.address);
                    LOG((LF_CORDB,LL_INFO1000,"Address not thunked - "
                        "must be to unJITted method, or normal managed "
                        "method lacking a DJI!\n"));
                }
#endif //LOGGING
            }
        }
    }
    
    if (info != NULL)
    {
        info->SortMap();
        LOG((LF_CORDB,LL_INFO10000, "D::GJI: found dji 0x%x for %s::%s "
            "(start,size):(0x%x,0x%x) nVer:0x%x\n",
            info, info->m_fd->m_pszDebugClassName,
            info->m_fd->m_pszDebugMethodName,
            (ULONG)info->m_addrOfCode, 
            (ULONG)info->m_sizeOfCode, 
            (ULONG)info->m_nVersion));
    }   
    UnlockJITInfoMutex();

    return info;
}

/******************************************************************************
 * GetILToNativeMapping returns a map from IL offsets to native
 * offsets for this code. An array of COR_PROF_IL_TO_NATIVE_MAP
 * structs will be returned, and some of the ilOffsets in this array
 * may be the values specified in CorDebugIlToNativeMappingTypes.
 ******************************************************************************/
HRESULT Debugger::GetILToNativeMapping(MethodDesc *pMD, ULONG32 cMap,
                                       ULONG32 *pcMap, COR_DEBUG_IL_TO_NATIVE_MAP map[])
{
    // Get the JIT info by functionId
    DebuggerJitInfo *pDJI = GetJitInfo(pMD, NULL);

    // Dunno what went wrong
    if (pDJI == NULL)
        return (E_FAIL);

    // If they gave us space to copy into...
    if (map != NULL)
    {
        // Only copy as much as either they gave us or we have to copy.
        SIZE_T cpyCount = min(cMap, pDJI->m_sequenceMapCount);

        // Read the map right out of the Left Side.
        if (cpyCount > 0)
            ExportILToNativeMap(cpyCount,
                        map,
                        pDJI->m_sequenceMap,
                        pDJI->m_sizeOfCode);
    }
    
    // Return the true count of entries
    if (pcMap)
        *pcMap = pDJI->m_sequenceMapCount;
    
    return (S_OK);
}




/******************************************************************************
 *
 ******************************************************************************/
DebuggerJitInfo::~DebuggerJitInfo()
{
    TRACE_FREE(m_sequenceMap);
    if (m_sequenceMap != NULL)
    {
        DeleteInteropSafe(((BYTE *)m_sequenceMap));
    }

    TRACE_FREE(m_varNativeInfo);
    if (m_varNativeInfo != NULL && m_varNeedsDelete)
    {
        DeleteInteropSafe(m_varNativeInfo);
    }

    TRACE_FREE(m_OldILToNewIL);
    if (m_OldILToNewIL != NULL)
    {
        DeleteInteropSafe(m_OldILToNewIL);
    }

    if (m_rgInstrumentedILMap != NULL)
        CoTaskMemFree(m_rgInstrumentedILMap);

    if (m_pDcq != NULL)
    {
        DeleteInteropSafe(m_pDcq);
    }

    LOG((LF_CORDB,LL_EVERYTHING, "DJI::~DJI : deleted at 0x%x\n", this));
}

/******************************************************************************
 *
 ******************************************************************************/
void DebuggerJitInfo::SortMap()
{
    //
    // Note that this routine must be called inside of a mutex.
    //  
    if (!m_sequenceMapSorted)
    {
        if (m_sequenceMap != NULL)
        {
            //
            // Sort by native offset.
            //

            MapSortNative nsort(m_sequenceMap, m_sequenceMapCount);

            nsort.Sort();

            //
            // Now, fill in the end ranges.
            //

            DebuggerILToNativeMap *m = m_sequenceMap;
            DebuggerILToNativeMap *mEnd = m + m_sequenceMapCount;

            while (m < mEnd)
            {
                m->nativeEndOffset = (m+1)->nativeStartOffset;
                m++;
            }

            //
            // Now, sort by il offset.
            //

            MapSortIL isort(m_sequenceMap, m_sequenceMapCount);

            isort.Sort();
        }

        m_sequenceMapSorted = true;
    }
}

/******************************************************************************
 *
 ******************************************************************************/
void * Debugger::allocateArray(SIZE_T cBytes)
{
    START_DBG_PERF();
    void *ret;
    
    if (cBytes > 0)
        ret = (void *) new (interopsafe) BYTE [cBytes];
    else
        ret = NULL;
    
    STOP_DBG_PERF();

    return ret;
}

/******************************************************************************
 *
 ******************************************************************************/
void Debugger::freeArray(void *array)
{
    START_DBG_PERF();

    if (array != NULL)
    {
        DeleteInteropSafe((BYTE *) array);
    }

    STOP_DBG_PERF();
}



/******************************************************************************
// Use an ISymUnmanagedReader to get method sequence points.
 ******************************************************************************/
void Debugger::getBoundaries(CORINFO_METHOD_HANDLE ftn,
                             unsigned int *cILOffsets,
                             DWORD **pILOffsets,
                             ICorDebugInfo::BoundaryTypes *implicitBoundaries)
{
    START_DBG_PERF();

	*cILOffsets = 0;
	*pILOffsets = NULL;
	*implicitBoundaries = NO_BOUNDARIES;

    // If there has been an unrecoverable Left Side error, then we
    // just pretend that there are no boundaries.
    if (CORDBUnrecoverableError(this))
    {
        STOP_DBG_PERF();
        return;
    }

    MethodDesc *md = (MethodDesc*)ftn;

    // If JIT optimizations are allowed for the module this function
    // lives in, then don't grab specific boundaries from the symbol
    // store since any boundaries we give the JIT will be pretty much
    // ignored anyway.
    bool allowJITOpts =
        CORDebuggerAllowJITOpts(md->GetModule()->GetDebuggerInfoBits());
    
    if (allowJITOpts)
    {
        *implicitBoundaries  = BoundaryTypes(STACK_EMPTY_BOUNDARIES |
                                             CALL_SITE_BOUNDARIES);
        return;
    }

    // Grab the JIT info struct for this method.
    DebuggerJitInfo *ji = GetJitInfo(md, NULL);

    // This may be called before JITBeginning in a compilation domain
    if (ji == NULL)
        ji = CreateJitInfo(md);

    _ASSERTE(ji != NULL); // to pitch, must first jit ==> it must exist

    LOG((LF_CORDB,LL_INFO10000,"De::NGB: Got DJI 0x%x\n",ji));

    if (ji != NULL)
    {
        {
            // Note: we need to make sure to enable preemptive GC here just in case we block in the symbol reader.
            bool disabled = g_pEEInterface->IsPreemptiveGCDisabled();

            if (disabled)
                g_pEEInterface->EnablePreemptiveGC();

            Module *pModule = ji->m_fd->GetModule();
            _ASSERTE(pModule != NULL);

            START_SYM_CREATE_PERF();
            ISymUnmanagedReader *pReader = pModule->GetISymUnmanagedReader();
            STOP_SYM_CREATE_PERF();
            
            // If we got a reader, use it.
            if (pReader != NULL)
            {
                // Grab the sym reader's method.
                ISymUnmanagedMethod *pISymMethod;

                START_SYM_PERF();
                
                HRESULT hr = pReader->GetMethod(ji->m_fd->GetMemberDef(),
                                                &pISymMethod);

                ULONG32 n = 0;
                
                if (SUCCEEDED(hr))
                {
                    // Get the count of sequence points.
                    hr = pISymMethod->GetSequencePointCount(&n);
                    _ASSERTE(SUCCEEDED(hr));

                    STOP_SYM_PERF();
                    
                    LOG((LF_CORDB, LL_INFO1000000,
                         "D::NGB: Reader seq pt count is %d\n", n));
                    
                    ULONG32 *p;

                    if (n > 0 
                        && (p = new (interopsafe) ULONG32 [n]) != NULL)
                    {
                        ULONG32 dummy;

                        START_SYM_PERF();
                        hr = pISymMethod->GetSequencePoints(n, &dummy,
                                                            p, NULL, NULL, NULL,
                                                            NULL, NULL);
                        STOP_SYM_PERF();
                        _ASSERTE(SUCCEEDED(hr));
                        _ASSERTE(dummy == n);
                                                            
                        *pILOffsets = (DWORD*)p;

                        // Translate the IL offets based on an
                        // instrumented IL map if one exists.
                        if (ji->m_cInstrumentedILMap > 0)
                        {
                            for (SIZE_T i = 0; i < n; i++)
                            {
                                long origOffset = *p;
                                
                                *p = ji->TranslateToInstIL(
                                                      origOffset,
                                                      bOriginalToInstrumented);

                                LOG((LF_CORDB, LL_INFO1000000,
                                     "D::NGB: 0x%04x (Real IL:0x%x)\n",
                                     origOffset, *p));

                                p++;
                            }
                        }
#ifdef LOGGING
                        else
                        {
                            for (SIZE_T i = 0; i < n; i++)
                            {
                                LOG((LF_CORDB, LL_INFO1000000,
                                     "D::NGB: 0x%04x \n", *p));
                                p++;
                            }
                        }
#endif                    
                    }
                    else
                        *pILOffsets = NULL;

                    pISymMethod->Release();
                }
                else
                {
                    STOP_SYM_PERF();
                    
                    *pILOffsets = NULL;

                    LOG((LF_CORDB, LL_INFO10000,
                         "De::NGB: failed to find method 0x%x in sym reader.\n",
                         ji->m_fd->GetMemberDef()));
                }

                *implicitBoundaries = CALL_SITE_BOUNDARIES;
                *cILOffsets = n;
            }
            else
            {
                LOG((LF_CORDB, LL_INFO1000000, "D::NGB: no reader.\n"));
                *implicitBoundaries  = BoundaryTypes(STACK_EMPTY_BOUNDARIES | CALL_SITE_BOUNDARIES);
            }

            // Re-disable preemptive GC if we enabled it above.
            if (disabled)
                g_pEEInterface->DisablePreemptiveGC();
        }
    }
    
    LOG((LF_CORDB, LL_INFO1000000, "D::NGB: cILOffsets=%d\n", *cILOffsets));
    STOP_DBG_PERF();
}

/******************************************************************************
// void Debugger::setBoundaries():  Called by JIT to tell the
// debugger what the IL to native map (the sequence map) is.  The
// information is stored in an array of DebuggerILToNativeMap
// structures, which is stored in the DebuggerJitInfo obtained
// from the DebuggerJitInfoTable.  The DebuggerJitInfo is placed
// there by the call to JitBeginning.
 ******************************************************************************/
void Debugger::setBoundaries(CORINFO_METHOD_HANDLE ftn, ULONG32 cMap,
                             OffsetMapping *pMap)
{
    START_DBG_PERF();
    
    if (CORDBUnrecoverableError(this))
        goto Exit;

    LOG((LF_CORDB,LL_INFO10000,"D:sB:%s::%s MethodDef:0x%x\n\n",
        ((MethodDesc*)ftn)->m_pszDebugClassName, 
        ((MethodDesc*)ftn)->m_pszDebugMethodName,
        ((MethodDesc*)ftn)->GetMemberDef()));
    
    DebuggerJitInfo *ji;
    ji = GetJitInfo((MethodDesc*)ftn, NULL);
    LOG((LF_CORDB,LL_INFO10000,"De::sB: Got DJI 0x%x\n",ji));

    if (ji != NULL && ji->m_codePitched == false)
    {
        if (FAILED(ji->SetBoundaries(cMap, pMap)))
        {
            DeleteHeadOfList((MethodDesc*)ftn);
        }
    }

    if (cMap)
    {
        DeleteInteropSafe(pMap);
    }

Exit:
    STOP_DBG_PERF();
}

/******************************************************************************
 *
 ******************************************************************************/
void Debugger::getVars(CORINFO_METHOD_HANDLE ftn, ULONG32 *cVars, ILVarInfo **vars, 
                       bool *extendOthers)
{
    START_DBG_PERF();
 
        // At worst return no information
    *cVars = 0;
    *vars = NULL;
    *extendOthers = false;
    
    if (CORDBUnrecoverableError(this))
        goto Exit;

    //
    //
    DebuggerJitInfo *ji;
    ji = GetJitInfo((MethodDesc*)ftn,NULL);

    // This may be called before JITBeginning in a compilation domain
    if (ji == NULL)
        ji = CreateJitInfo((MethodDesc*)ftn);

    _ASSERTE( ji != NULL ); // to pitch, must first jit ==> it must exist
    LOG((LF_CORDB,LL_INFO10000,"De::gV: Got DJI 0x%x\n",ji));

    if (ji != NULL)
    {
        // Just tell the JIT to extend everything.
        *extendOthers = true;

        // But, is this a vararg function?
        BOOL fVarArg = false;
        GetArgCount((MethodDesc*)ftn, &fVarArg);
        
        if (fVarArg)
        {
            // It is, so we need to tell the JIT to give us the
            // varags handle.
            ILVarInfo *p = new (interopsafe) ILVarInfo[1];

            if (p != NULL)
            {
                COR_ILMETHOD_DECODER header(g_pEEInterface->MethodDescGetILHeader((MethodDesc*)ftn));
                unsigned int ilCodeSize = header.GetCodeSize();
                    
                p->startOffset = 0;
                p->endOffset = ilCodeSize;
                p->varNumber = (DWORD) ICorDebugInfo::VARARGS_HANDLE;

                *cVars = 1;
                *vars = p;
            }
        }
    }

    LOG((LF_CORDB, LL_INFO1000000, "D::gV: cVars=%d, extendOthers=%d\n",
         *cVars, *extendOthers));

Exit:
    STOP_DBG_PERF();
}

/******************************************************************************
 *
 ******************************************************************************/
void Debugger::setVars(CORINFO_METHOD_HANDLE ftn, ULONG32 cVars, NativeVarInfo *vars)
{
    START_DBG_PERF();
    
    if (CORDBUnrecoverableError(this))
        goto Exit;

    _ASSERTE((cVars == 0) == (vars == NULL));

    DebuggerJitInfo *ji;
    ji = GetJitInfo((MethodDesc*)ftn, NULL);
    
    if ( ji == NULL )
    {
        // setBoundaries may run out of mem & eliminated the DJI
        LOG((LF_CORDB,LL_INFO10000,"De::sV:Got NULL Ptr - out of mem?\n"));
        goto Exit; 
    }

    LOG((LF_CORDB,LL_INFO10000,"De::sV: Got DJI 0x%x\n",ji));

    if (ji != NULL && ji->m_codePitched == false)
    {
        ji->SetVars(cVars, vars, true);
    }
    else
    {
        if (cVars)
        {
            DeleteInteropSafe(vars);
        }
    }

Exit:
    STOP_DBG_PERF();
}

// We want to keep the 'worst' HRESULT - if one has failed (..._E_...) & the
// other hasn't, take the failing one.  If they've both/neither failed, then
// it doesn't matter which we take.
// Note that this macro favors retaining the first argument
#define WORST_HR(hr1,hr2) (FAILED(hr1)?hr1:hr2)
/******************************************************************************
 *
 ******************************************************************************/
HRESULT Debugger::SetIP( bool fCanSetIPOnly, Thread *thread,Module *module, 
                         mdMethodDef mdMeth, DebuggerJitInfo* dji, 
                         SIZE_T offsetTo, BOOL fIsIL, void *firstExceptionHandler)
{
#ifdef _DEBUG
    static ConfigDWORD breakOnSetIP;
    if (breakOnSetIP.val(L"DbgBreakOnSetIP", 0)) _ASSERTE(!"DbgBreakOnSetIP");
#endif

    _ASSERTE( thread != NULL);
    _ASSERTE( module != NULL);
    _ASSERTE( mdMeth != mdMethodDefNil);

    HRESULT hr = S_OK;
    HRESULT hrAdvise = S_OK;
    
    MethodDesc *pFD = NULL;
    DWORD offIL;
    CorDebugMappingResult map;
    DWORD whichIgnore;
    
    ControllerStackInfo csi;
    CONTEXT  Ctx;
    CONTEXT  realCtx;           // in case actual context is needed
    
    BOOL exact;
    SIZE_T offNat;

    BYTE    *pbDest = NULL;
    BYTE    *pbBase = NULL;
    CONTEXT *pCtx   = NULL;
    DWORD    dwSize = 0;
    DWORD *rgVal1 = NULL;
    DWORD *rgVal2 = NULL;
    BYTE **pVCs   = NULL;
    
    LOG((LF_CORDB, LL_INFO1000, "D::SIP: In SetIP ==> fCanSetIPOnly:0x%x <==!\n", fCanSetIPOnly));  

    pCtx = g_pEEInterface->GetThreadFilterContext(thread);

    _ASSERTE(!(g_pEEInterface->GetThreadFilterContext(thread) && ISREDIRECTEDTHREAD(thread)));

    if (pCtx == NULL && ISREDIRECTEDTHREAD(thread))
    {
        pCtx = GETREDIRECTEDCONTEXT(thread);
    }

    if (pCtx == NULL)
    {
        realCtx.ContextFlags = CONTEXT_CONTROL;

        if (!GetThreadContext(thread->GetThreadHandle(), &realCtx))
            _ASSERTE(!"GetThreadContext failed.");

        pCtx = &realCtx;
    }

    // Implicit Caveat: We need to be the active frame.
    csi.GetStackInfo(thread, NULL, &Ctx, false);

    pFD = g_pEEInterface->LookupMethodDescFromToken(
        module,mdMeth);
    _ASSERTE( pFD != NULL );
    _ASSERTE( dji == NULL || dji->m_fd == pFD);

    if (dji == NULL )
    {
        dji = GetJitInfo( pFD, NULL );
        LOG((LF_CORDB, LL_INFO1000, "D::SIP:Not given token "
                "from right side - GJI returned 0x%x!\n", dji));
    }

    if (dji == NULL) //we don't have info about this method - attach scenario
    {
        if (fIsIL)
        {
            LOG((LF_CORDB, LL_INFO1000, "D::SIP:Couldn't obtain version info -"
            "SetIP by IL offset can't work\n"));
            hrAdvise = WORST_HR(hrAdvise, CORDBG_E_SET_IP_IMPOSSIBLE);
            goto LExit;
        }

        LOG((LF_CORDB, LL_INFO1000, "D::SIP:Couldn't obtain version info - "
                "SetIP by native offset proceeding via GetFunctionAddress\n"));
                
        pbBase = (BYTE*)g_pEEInterface->GetFunctionAddress(pFD);
        if (pbBase == NULL)
        {
            LOG((LF_CORDB, LL_INFO1000, "D::SIP:GetFnxAddr failed!\n"));
            hrAdvise = WORST_HR(hrAdvise, CORDBG_E_SET_IP_IMPOSSIBLE);
            goto LExit;
        }
        dwSize = (DWORD)g_pEEInterface->GetFunctionSize(pFD);
        
        offNat = offsetTo;
        pbDest = pbBase + offsetTo;
    }
    else
    {
        LOG((LF_CORDB, LL_INFO1000, "D::SIP:Got version info fine\n"));

        // Caveat: we need to start from a sequence point
        offIL = dji->MapNativeOffsetToIL(csi.m_activeFrame.relOffset,
                                         &map, &whichIgnore);
        if ( !(map & MAPPING_EXACT) )
        {
            LOG((LF_CORDB, LL_INFO1000, "D::SIP:Starting native offset is bad!\n"));
            hrAdvise = WORST_HR(hrAdvise, CORDBG_S_BAD_START_SEQUENCE_POINT);
        }
        else
        {   // exact IL mapping

            if (!(dji->GetSrcTypeFromILOffset(offIL) & ICorDebugInfo::STACK_EMPTY))
            {
                LOG((LF_CORDB, LL_INFO1000, "D::SIP:Starting offset isn't stack empty!\n"));
                hrAdvise = WORST_HR(hrAdvise, CORDBG_S_BAD_START_SEQUENCE_POINT);
            }
        }

        // Caveat: we need to go to a sequence point
        if (fIsIL )
        {
            offNat = dji->MapILOffsetToNative(offsetTo, &exact);
            if (!exact)
            {
                LOG((LF_CORDB, LL_INFO1000, "D::SIP:Dest (via IL offset) is bad!\n"));
                hrAdvise = WORST_HR(hrAdvise, CORDBG_S_BAD_END_SEQUENCE_POINT);
            }
        }
        else
        {
            offNat = offsetTo;
            LOG((LF_CORDB, LL_INFO1000, "D::SIP:Dest of 0x%x (via native "
                "offset) is fine!\n", offNat));
        }

        CorDebugMappingResult mapping;
        DWORD which;
        offsetTo = dji->MapNativeOffsetToIL(offNat, &mapping, &which);

        // We only want to perhaps return CORDBG_S_BAD_END_SEQUENCE_POINT if
        // we're not already returning CORDBG_S_BAD_START_SEQUENCE_POINT.
        if (hr != CORDBG_S_BAD_START_SEQUENCE_POINT && 
            (mapping != MAPPING_EXACT ||
             !(dji->GetSrcTypeFromILOffset(offIL) & ICorDebugInfo::STACK_EMPTY)))
        {
            LOG((LF_CORDB, LL_INFO1000, "D::SIP:Ending offset isn't a sequence"
                                        " point, or not stack empty!\n"));
            hrAdvise = WORST_HR(hrAdvise, CORDBG_S_BAD_END_SEQUENCE_POINT);
        }

        // Caveat: can't setip if there's no code
        if (dji->m_codePitched) 
        {
            LOG((LF_CORDB, LL_INFO1000, "D::SIP:Code has been pitched!\n"));
            hrAdvise = WORST_HR(hrAdvise, CORDBG_E_CODE_NOT_AVAILABLE);
            if (fCanSetIPOnly)
                goto LExit;
        }

        pbBase = (BYTE*)dji->m_addrOfCode;
        dwSize = (DWORD)dji->m_sizeOfCode;
        pbDest = (BYTE*)dji->m_addrOfCode + offNat;
        LOG((LF_CORDB, LL_INFO1000, "D::SIP:Dest is 0x%x\n", pbDest));
    }

    if (!fCanSetIPOnly)
    {
        hr = ShuffleVariablesGet(dji, 
                                 csi.m_activeFrame.relOffset, 
                                 pCtx,
                                 &rgVal1,
                                 &rgVal2,
                                 &pVCs);
        if (FAILED(hr))
        {
            // This will only fail fatally, so exit.
            hrAdvise = WORST_HR(hrAdvise, hr);
            goto LExit;
        }
    }

    hr =g_pEEInterface->SetIPFromSrcToDst(thread,
                                          csi.m_activeFrame.pIJM,
                                          csi.m_activeFrame.MethodToken,
                                          pbBase,
                                          csi.m_activeFrame.relOffset, 
                                          offNat,
                                          fCanSetIPOnly,
                                          &(csi.m_activeFrame.registers),
                                          pCtx,
                                          dwSize,
                                          firstExceptionHandler,
                                          (void *)dji);
    // Get the return code, if any                                          
    if (hr != S_OK)
    {
        hrAdvise = WORST_HR(hrAdvise, hr);
        goto LExit;
    }

    // If we really want to do this, we'll have to put the
    // variables into their new locations.
    if (!fCanSetIPOnly && !FAILED(hrAdvise))
    {
        ShuffleVariablesSet(dji, 
                            offNat, 
                            pCtx,
                            &rgVal1,
                            &rgVal2,
                            pVCs);

        _ASSERTE(pbDest != NULL);
        
        ::SetIP(pCtx, pbDest);

        if (pCtx == &realCtx)
        {
            if (!SetThreadContext(thread->GetThreadHandle(), &realCtx))
                _ASSERTE(!"SetThreadContext failed.");
        }

        LOG((LF_CORDB, LL_INFO1000, "D::SIP:Set IP to be 0x%x\n", GetIP(pCtx)));
    }

    
LExit:
    if (rgVal1 != NULL)
        delete [] rgVal1;
        
    if (rgVal2 != NULL)
        delete [] rgVal2;
        
    LOG((LF_CORDB, LL_INFO1000, "D::SIP:Returning 0x%x\n", hr));
    return hrAdvise;
}

#include "nativevaraccessors.h"

/******************************************************************************
 *
 ******************************************************************************/

HRESULT Debugger::ShuffleVariablesGet(DebuggerJitInfo  *dji, 
                                      SIZE_T            offsetFrom, 
                                      CONTEXT          *pCtx,
                                      DWORD           **prgVal1,
                                      DWORD           **prgVal2,
                                      BYTE           ***prgpVCs)
{
    _ASSERTE(dji != NULL);
    _ASSERTE(pCtx != NULL);
    _ASSERTE(prgVal1 != NULL);
    _ASSERTE(prgVal2 != NULL);
    _ASSERTE(dji->m_sizeOfCode >= offsetFrom);

    HRESULT hr = S_OK;
    DWORD *rgVal1 = new DWORD[dji->m_varNativeInfoCount];
    DWORD *rgVal2 = NULL;

    if (rgVal1 == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto LExit;
    }


    rgVal2 = new DWORD[dji->m_varNativeInfoCount];
    
    if (rgVal2 == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto LExit;
    }
    
    GetVariablesFromOffset(dji->m_fd,
                           dji->m_varNativeInfoCount, 
                           dji->m_varNativeInfo,
                           offsetFrom,
                           pCtx, 
                           rgVal1,
                           rgVal2,
                           prgpVCs);
                                  
LExit:
    if (!FAILED(hr))
    {
        (*prgVal1) = rgVal1;
        (*prgVal2) = rgVal2;
    }
    else
    {
        LOG((LF_CORDB, LL_INFO100, "D::SVG: something went wrong hr=0x%x!", hr));

        (*prgVal1) = NULL;
        (*prgVal2) = NULL;
        
        if (rgVal1 != NULL)
            delete rgVal1;
        
        if (rgVal2 != NULL)
            delete rgVal2;
    }

    return hr;
}

/******************************************************************************
 *
 ******************************************************************************/
void Debugger::ShuffleVariablesSet(DebuggerJitInfo  *dji, 
                                   SIZE_T            offsetTo, 
                                   CONTEXT          *pCtx,
                                   DWORD           **prgVal1,
                                   DWORD           **prgVal2,
                                   BYTE            **rgpVCs)
{
    _ASSERTE(dji != NULL);
    _ASSERTE(pCtx != NULL);
    _ASSERTE(prgVal1 != NULL);
    _ASSERTE(prgVal2 != NULL);
    _ASSERTE(dji->m_sizeOfCode >= offsetTo);

    SetVariablesAtOffset(dji->m_fd,
                         dji->m_varNativeInfoCount, 
                         dji->m_varNativeInfo,
                         offsetTo,
                         pCtx, 
                         *prgVal1,
                         *prgVal2,
                         rgpVCs);

    delete (*prgVal1);
    (*prgVal1) = NULL;
    delete (*prgVal2);
    (*prgVal2) = NULL;
}

// Helper method pair to grab all, then set all, variables at a given
// point in a routine. 
// It's assumed that varNativeInfo[i] is the ith variable of the method
// Note that GetVariablesFromOffset and SetVariablesAtOffset are 
// very similar - modifying one will probably need to be reflected in the other...
HRESULT Debugger::GetVariablesFromOffset(MethodDesc       *pMD,
                               UINT                        varNativeInfoCount, 
                               ICorJitInfo::NativeVarInfo *varNativeInfo,
                               SIZE_T                      offsetFrom, 
                               CONTEXT                    *pCtx,
                               DWORD                      *rgVal1,
                               DWORD                      *rgVal2,
                               BYTE                     ***rgpVCs)
{
        // if there are no locals, well, we are done!
    if (varNativeInfoCount == 0)
	{
		*rgpVCs = NULL;
		return S_OK;
	}

    _ASSERTE(varNativeInfo != NULL);
    _ASSERTE(rgVal1 != NULL);
    _ASSERTE(rgVal2 != NULL);
    
    LOG((LF_CORDB, LL_INFO10000, "D::GVFO: %s::%s, infoCount:0x%x, from:0x%x\n",
        pMD->m_pszDebugClassName, pMD->m_pszDebugMethodName, varNativeInfoCount,
        offsetFrom));

    HRESULT hr = S_OK;
    BOOL  res;
    unsigned i;
    BYTE **rgpValueClasses = NULL;
    MetaSig *pLocals = NULL;
    ULONG cValueClasses = 0;
    CorElementType cet = ELEMENT_TYPE_END;
    
    memset( rgVal1, 0, sizeof(DWORD)*varNativeInfoCount);
    memset( rgVal2, 0, sizeof(DWORD)*varNativeInfoCount);

    COR_ILMETHOD_DECODER decoderOldIL(pMD->GetILHeader());
    mdSignature mdLocalSig = (decoderOldIL.GetLocalVarSigTok())?(decoderOldIL.GetLocalVarSigTok()):
                        (mdSignatureNil);

    // If there isn't a local sig, then there can't be any VCs
    BOOL fVCs = (mdLocalSig != mdSignatureNil);
    if (fVCs)
    {
        ULONG cbSig;
        PCCOR_SIGNATURE sig = pMD->GetModule()->GetMDImport()->GetSigFromToken(mdLocalSig, &cbSig);

        pLocals = new MetaSig(sig, pMD->GetModule(), FALSE, MetaSig::sigLocalVars);
        while((cet = pLocals->NextArg()) != ELEMENT_TYPE_END)
        {
            if (cet == ELEMENT_TYPE_VALUETYPE)
                cValueClasses++;
        }
        pLocals->Reset();
        if (cValueClasses > 0)
        {
            LOG((LF_CORDB, LL_INFO10000, "D::GVFO: 0x%x value types!\n", cValueClasses));

            rgpValueClasses = new BYTE *[cValueClasses];
            if (rgpValueClasses == NULL)
                goto LExit;
                
            memset(rgpValueClasses, 0, sizeof(BYTE *)*cValueClasses);
        }
        cValueClasses = 0; // now becomes a VC index
    }        
#ifdef _DEBUG
    else
    {
        LOG((LF_CORDB, LL_INFO100, "D::SVAO: No locals!"));
        _ASSERTE(cet != ELEMENT_TYPE_VALUETYPE);
    }
#endif //_DEBUG

    for (i = 0;i< varNativeInfoCount;i++)
    {
        if (fVCs)
            cet = pLocals->NextArg();
        
        if (varNativeInfo[i].startOffset <= offsetFrom &&
            varNativeInfo[i].endOffset >= offsetFrom &&
            varNativeInfo[i].loc.vlType != ICorDebugInfo::VLT_INVALID)
        {
            if (fVCs && 
                cet == ELEMENT_TYPE_VALUETYPE &&
                varNativeInfo[i].loc.vlType != ICorDebugInfo::VLT_REG &&
                varNativeInfo[i].loc.vlType != ICorDebugInfo::VLT_REG_REG &&
                varNativeInfo[i].loc.vlType != ICorDebugInfo::VLT_REG_STK &&
                varNativeInfo[i].loc.vlType != ICorDebugInfo::VLT_STK_REG
                )
            {
                SigPointer sp = pLocals->GetArgProps();
#ifdef _DEBUG
                CorElementType cet2 = sp.GetElemType();
#endif
                _ASSERTE(cet == cet2);
                mdToken token = sp.GetToken();
                
                EEClass *pClass = g_pEEInterface->FindLoadedClass(pMD->GetModule(),
                    token);
                _ASSERTE(pClass->IsValueClass());
                if (pClass->GetMethodTable()->GetNormCorElementType() != ELEMENT_TYPE_VALUETYPE)
                    goto DO_PRIMITIVE;
                
                _ASSERTE(varNativeInfo[i].loc.vlType == ICorDebugInfo::VLT_STK);
                SIZE_T cbClass = pClass->GetAlignedNumInstanceFieldBytes();
                
                LOG((LF_CORDB, LL_INFO10000, "D::GVFO: var 0x%x is a VC,"
                    " of type %s, size:0x%x\n",i, pClass->m_szDebugClassName,
                    cbClass));
                
                // Make space for it - note that it uses teh VC index,
                // NOT the variable index
                rgpValueClasses[cValueClasses] = new BYTE[cbClass];
                if (rgpValueClasses[cValueClasses] == NULL)
                {
                    hr = E_OUTOFMEMORY;
                    goto LExit;
                }
                memmove(rgpValueClasses[cValueClasses], 
                    NativeVarStackAddr(varNativeInfo[i].loc, pCtx),
                    cbClass);
                
                // Move index up.                                                        
                cValueClasses++;
            }
            else
            {
DO_PRIMITIVE:
                // Note: negative variable numbers are possible for special cases (i.e., -2 for a value class return
                // buffer, etc.) so we filter those out here...
                //
                // Note: we case to (int) to ensure that we can do the negative number check.
                //
                if ((int)varNativeInfo[i].varNumber >= 0)
                {
                    //Xfer the variable from the old location to temp storage
                    res = GetNativeVarVal(varNativeInfo[i].loc, 
                                          pCtx, 
                                          &(rgVal1[varNativeInfo[i].varNumber]),
                                          &(rgVal2[varNativeInfo[i].varNumber]));
                    assert(res == TRUE);
                }
            }
        }
    }
    
LExit:
    if (FAILED(hr))
    {
        LOG((LF_CORDB, LL_INFO100, "D::GVFO: error:0x%x",hr));
        if (rgpValueClasses != NULL)
        {   // free any memory we allocated for VCs here
            while(cValueClasses > 0) 
            {
                --cValueClasses;
                delete rgpValueClasses[cValueClasses];  // OK to delete NULL
            }  
            delete rgpValueClasses;
            rgpValueClasses = NULL;
        }       
    }
    if (pLocals != NULL)
        delete pLocals;
    
    (*rgpVCs) = rgpValueClasses;
    return hr;
}

// Note that GetVariablesFromOffset and SetVariablesAtOffset are 
// very similar - modifying one will probably need to be reflected in the other...
void Debugger::SetVariablesAtOffset(MethodDesc       *pMD,
                          UINT                        varNativeInfoCount, 
                          ICorJitInfo::NativeVarInfo *varNativeInfo,
                          SIZE_T                      offsetTo, 
                          CONTEXT                    *pCtx,
                          DWORD                      *rgVal1,
                          DWORD                      *rgVal2,
                          BYTE                      **rgpVCs)
{
    _ASSERTE(varNativeInfoCount == 0 || varNativeInfo != NULL);
    _ASSERTE(pCtx != NULL);
    _ASSERTE(rgVal1 != NULL);
    _ASSERTE(rgVal2 != NULL);

    BOOL  res;
    unsigned i;
    CorElementType cet = ELEMENT_TYPE_END;
    MetaSig *pLocals = NULL;
    ULONG iVC = 0;    
    
    COR_ILMETHOD_DECODER decoderOldIL(pMD->GetILHeader());
    mdSignature mdLocalSig = (decoderOldIL.GetLocalVarSigTok())?(decoderOldIL.GetLocalVarSigTok()):
                        (mdSignatureNil);

    // If there isn't a local sig, then there can't be any VCs
    BOOL fVCs = (mdLocalSig != mdSignatureNil);
    if (fVCs)
    {
        ULONG cbSig;
        PCCOR_SIGNATURE sig = pMD->GetModule()->GetMDImport()->GetSigFromToken(mdLocalSig, &cbSig);

        pLocals = new MetaSig(sig, pMD->GetModule(), FALSE, MetaSig::sigLocalVars);
    }        
#ifdef _DEBUG
    else   
    {
        LOG((LF_CORDB, LL_INFO100, "D::SVAO: No locals!"));
        _ASSERTE(cet != ELEMENT_TYPE_VALUETYPE);
    }
#endif //_DEBUG

    // Note that since we obtain all the variables in the first loop, we
    // can now splatter those variables into their new locations
    // willy-nilly, without the fear that variable locations that have
    // been swapped might accidentally overwrite a variable value.
    for (i = 0;i< varNativeInfoCount;i++)
    {
        if (fVCs)
            cet = pLocals->NextArg();
        
        LOG((LF_CORDB, LL_INFO100000, "SVAO: var 0x%x: offTo:0x%x "
            "range: 0x%x - 0x%x\n typ:0x%x", varNativeInfo[i].varNumber,
            offsetTo, varNativeInfo[i].startOffset,
            varNativeInfo[i].endOffset, varNativeInfo[i].loc.vlType));
        
        if (varNativeInfo[i].startOffset <= offsetTo &&
            varNativeInfo[i].endOffset >= offsetTo &&
            varNativeInfo[i].loc.vlType != ICorDebugInfo::VLT_INVALID)
        {
            _ASSERTE(varNativeInfo[i].loc.vlType
                != ICorDebugInfo::VLT_COUNT);
                
            if (fVCs && 
                cet == ELEMENT_TYPE_VALUETYPE &&
                varNativeInfo[i].loc.vlType != ICorDebugInfo::VLT_REG &&
                varNativeInfo[i].loc.vlType != ICorDebugInfo::VLT_REG_REG &&
                varNativeInfo[i].loc.vlType != ICorDebugInfo::VLT_REG_STK &&
                varNativeInfo[i].loc.vlType != ICorDebugInfo::VLT_STK_REG
                )
            {
                SigPointer sp = pLocals->GetArgProps();
#ifdef _DEBUG
                CorElementType cet2 = sp.GetElemType();
#endif
                _ASSERTE(cet == cet2);
                mdToken token = sp.GetToken();
                
                EEClass *pClass = g_pEEInterface->FindLoadedClass(pMD->GetModule(),
                                                                 token);
                _ASSERTE(pClass->IsValueClass());
                if (pClass->GetMethodTable()->GetNormCorElementType() != ELEMENT_TYPE_VALUETYPE)
                    goto DO_PRIMITIVE;
                
                _ASSERTE(varNativeInfo[i].loc.vlType == ICorDebugInfo::VLT_STK);
                SIZE_T cbClass = pClass->GetAlignedNumInstanceFieldBytes();

                LOG((LF_CORDB, LL_INFO10000, "D::SVAO: var 0x%x is a VC,"
                    " of type %s, size:0x%x\n", i, pClass->m_szDebugClassName,
                    cbClass));

                // We'll always allocate enough ptrs for all the VC's.
                // However, if a VC comes into scope, we won't have gotten
                // memory for it back in GetVariablesFromOffset.
                // If it's a new variable, then just initialize it to 0 here.
                if (rgpVCs[iVC] != NULL)
                {
                    LOG((LF_CORDB, LL_INFO10000, "D::SVAO: moved 0x%x bytes to 0x%x"
                        " sample:0x%x 0x%x 0x%x 0x%x\n", cbClass, 
                        NativeVarStackAddr(varNativeInfo[i].loc, pCtx)));
                        
                    memmove(NativeVarStackAddr(varNativeInfo[i].loc, pCtx),
                            rgpVCs[iVC],
                            cbClass);

                    // Now get rid of the memory                            
                    delete rgpVCs[iVC];
                    rgpVCs[iVC] = NULL;
                }
                else
                {
                    LOG((LF_CORDB, LL_INFO10000, "D::SVAO: memset 0x%x bytes at 0x%x\n",
                        cbClass, NativeVarStackAddr(varNativeInfo[i].loc, pCtx)));
                        
                    memset(NativeVarStackAddr(varNativeInfo[i].loc, pCtx),
                           0,
                           cbClass);
                }
                iVC++;                            
            }
            else
            {
            DO_PRIMITIVE:
                // Note: negative variable numbers are possible for special cases (i.e., -2 for a value class return
                // buffer, etc.) so we filter those out here...
                //
                // Note: we case to (int) to ensure that we can do the negative number check.
                //
                if ((int)varNativeInfo[i].varNumber >= 0)
                {
                    res = SetNativeVarVal(varNativeInfo[i].loc, 
                                          pCtx,
                                          rgVal1[varNativeInfo[i].varNumber],
                                          rgVal2[varNativeInfo[i].varNumber]);
                    assert(res == TRUE);
                }
            }
        }
    }

    if (pLocals != NULL)
        delete pLocals;
    if (rgpVCs != NULL)
        delete rgpVCs;
}


DebuggerILToNativeMap *DebuggerJitInfo::MapILOffsetToMapEntry(SIZE_T offset, BOOL *exact)
{
    _ASSERTE(m_sequenceMapSorted);

    //
    // Binary search for matching map element.
    //

    DebuggerILToNativeMap *mMin = m_sequenceMap;
    DebuggerILToNativeMap *mMax = mMin + m_sequenceMapCount;

    _ASSERTE( mMin < mMax ); //otherwise we have no code

    if (exact)
        *exact = FALSE;
    while (mMin + 1 < mMax)
    {
        _ASSERTE(mMin>=m_sequenceMap);
        DebuggerILToNativeMap *mMid = mMin + ((mMax - mMin)>>1);
        _ASSERTE(mMid>=m_sequenceMap);
        
        if (offset == mMid->ilOffset) {
            if (exact)
                *exact = TRUE;
            return mMid;
        }
        else if (offset < mMid->ilOffset)
            mMax = mMid;
        else
            mMin = mMid;
    }

    if (exact && offset == mMin->ilOffset)
        *exact = TRUE;
    return mMin;
}


SIZE_T DebuggerJitInfo::MapILOffsetToNative(SIZE_T ilOffset, BOOL *exact)
{
    _ASSERTE(m_sequenceMapSorted);

    DebuggerILToNativeMap *map = MapILOffsetToMapEntry(ilOffset, exact);
        
    _ASSERTE( map != NULL );
    LOG((LF_CORDB, LL_INFO10000, "DJI::MILOTN: ilOff 0x%x to nat 0x%x pExact"
        ":0x%x (valu:0x%x)(Entry IL Off:0x%x)\n", ilOffset, map->nativeStartOffset, 
        exact, (exact?*exact:0), map->ilOffset));

    return map->nativeStartOffset;
}

bool DbgIsSpecialILOffset(DWORD offset)
{
    return (offset == (ULONG) ICorDebugInfo::PROLOG ||
            offset == (ULONG) ICorDebugInfo::EPILOG ||
            offset == (ULONG) ICorDebugInfo::NO_MAPPING);
}

// SIZE_T DebuggerJitInfo::MapSpecialToNative():  Maps something like
//      a prolog to a native offset.
// CordDebugMappingResult mapping:  Mapping type to be looking for.
// SIZE_T which:  Which one.                                                  

SIZE_T DebuggerJitInfo::MapSpecialToNative(CorDebugMappingResult mapping, 
                                           SIZE_T which,
                                           BOOL *pfAccurate)
{
    LOG((LF_CORDB, LL_INFO10000, "DJI::MSTN map:0x%x which:0x%x\n", mapping, which));
    _ASSERTE(m_sequenceMapSorted);
    _ASSERTE(NULL != pfAccurate);
    
    bool fFound;
    SIZE_T  cFound = 0;
    
    DebuggerILToNativeMap *m = m_sequenceMap;
    DebuggerILToNativeMap *mEnd = m + m_sequenceMapCount;
    while( m < mEnd )
    {
        _ASSERTE(m>=m_sequenceMap);
        
        fFound = false;
        
        if (DbgIsSpecialILOffset(m->ilOffset))
            cFound++;
        
        if (cFound == which)
        {
            _ASSERTE( (mapping == MAPPING_PROLOG && 
                m->ilOffset == (SIZE_T) ICorDebugInfo::PROLOG) ||
                      (mapping == MAPPING_EPILOG &&
                m->ilOffset == (SIZE_T) ICorDebugInfo::EPILOG) ||
                      ((mapping == MAPPING_NO_INFO || mapping == MAPPING_UNMAPPED_ADDRESS) &&
                m->ilOffset == (SIZE_T) ICorDebugInfo::NO_MAPPING)
                    );

            (*pfAccurate) = TRUE;                    
            LOG((LF_CORDB, LL_INFO10000, "DJI::MSTN found mapping to nat:0x%x\n",
                m->nativeStartOffset));
            return m->nativeStartOffset;
        }
        m++;
    }

    LOG((LF_CORDB, LL_INFO10000, "DJI::MSTN No mapping found :(\n"));
    (*pfAccurate) = FALSE;

    return 0;
}

// void DebuggerJitInfo::MapILRangeToMapEntryRange():   MIRTMER
// calls MapILOffsetToNative for the startOffset (putting the
// result into start), and the endOffset (putting the result into end).
// SIZE_T startOffset:  IL offset from beginning of function.
// SIZE_T endOffset:  IL offset from beginngin of function,
// or zero to indicate that the end of the function should be used.
// DebuggerILToNativeMap **start:  Contains start & end
// native offsets that correspond to startOffset.  Set to NULL if
// there is no mapping info.
// DebuggerILToNativeMap **end:  Contains start & end native
// offsets that correspond to endOffset. Set to NULL if there
// is no mapping info.
void DebuggerJitInfo::MapILRangeToMapEntryRange(SIZE_T startOffset,
                                                SIZE_T endOffset,
                                                DebuggerILToNativeMap **start,
                                                DebuggerILToNativeMap **end)
{
    _ASSERTE(m_sequenceMapSorted);

    LOG((LF_CORDB, LL_INFO1000000,
         "DJI::MIRTMER: IL 0x%04x-0x%04x\n",
         startOffset, endOffset));

    if (m_sequenceMapCount == 0)
    {
        *start = NULL;
        *end = NULL;
        return;
    }

    *start = MapILOffsetToMapEntry(startOffset);

    //
    // end points to the last range that endOffset maps to, not past
    // the last range.
    // We want to return the last IL, and exclude the epilog
    if (endOffset == 0)
    {
        *end = m_sequenceMap + m_sequenceMapCount - 1;
        _ASSERTE(*end>=m_sequenceMap);
        
        while ( ((*end)->ilOffset == (ULONG) ICorDebugInfo::EPILOG||
                (*end)->ilOffset == (ULONG) ICorDebugInfo::NO_MAPPING)
               && (*end) > m_sequenceMap)
        {               
            (*end)--;
            _ASSERTE(*end>=m_sequenceMap);

        }
    }
    else
        *end = MapILOffsetToMapEntry(endOffset - 1);

    _ASSERTE(*end>=m_sequenceMap);


    LOG((LF_CORDB, LL_INFO1000000,
         "DJI::MIRTMER: IL 0x%04x-0x%04x --> 0x%04x 0x%08x-0x%08x\n"
         "                               --> 0x%04x 0x%08x-0x%08x\n",
         startOffset, endOffset,
         (*start)->ilOffset,
         (*start)->nativeStartOffset, (*start)->nativeEndOffset,
         (*end)->ilOffset,
         (*end)->nativeStartOffset, (*end)->nativeEndOffset));
}


// DWORD DebuggerJitInfo::MapNativeOffsetToIL():   Given a native
//  offset for the DebuggerJitInfo, compute
//  the IL offset from the beginning of the same method.
// Returns: Offset of the IL instruction that contains
//  the native offset,
// SIZE_T nativeOffset:  [IN] Native Offset
// CorDebugMappingResult *map:  [OUT] explains the
//  quality of the matching & special cases
// SIZE_T which:  It's possible to have multiple EPILOGs, or
//  multiple unmapped regions within a method.  This opaque value
//  specifies which special region we're talking about.  This
//  param has no meaning if map & (MAPPING_EXACT|MAPPING_APPROXIMATE)
//  Basically, this gets handed back to MapSpecialToNative, later.
DWORD DebuggerJitInfo::MapNativeOffsetToIL(DWORD nativeOffset, 
                                            CorDebugMappingResult *map,
                                            DWORD *which)
{
    _ASSERTE(m_sequenceMapSorted);
    _ASSERTE(map != NULL);
    _ASSERTE(which != NULL);

    (*which) = 0;
    DebuggerILToNativeMap *m = m_sequenceMap;
    DebuggerILToNativeMap *mEnd = m + m_sequenceMapCount;

    LOG((LF_CORDB,LL_INFO10000,"DJI::MNOTI: nativeOffset = 0x%x\n", nativeOffset));
    
    while (m < mEnd)
    {
        _ASSERTE(m>=m_sequenceMap);

#ifdef LOGGING
        if (m->ilOffset == (SIZE_T) ICorDebugInfo::PROLOG )
            LOG((LF_CORDB,LL_INFO10000,"DJI::MNOTI: m->natStart:0x%x m->natEnd:0x%x il:PROLOG\n", m->nativeStartOffset, m->nativeEndOffset));
        else if (m->ilOffset == (SIZE_T) ICorDebugInfo::EPILOG )
            LOG((LF_CORDB,LL_INFO10000,"DJI::MNOTI: m->natStart:0x%x m->natEnd:0x%x il:EPILOG\n", m->nativeStartOffset, m->nativeEndOffset));
        else if (m->ilOffset == (SIZE_T) ICorDebugInfo::NO_MAPPING)
            LOG((LF_CORDB,LL_INFO10000,"DJI::MNOTI: m->natStart:0x%x m->natEnd:0x%x il:NO MAP\n", m->nativeStartOffset, m->nativeEndOffset));
        else
            LOG((LF_CORDB,LL_INFO10000,"DJI::MNOTI: m->natStart:0x%x m->natEnd:0x%x il:0x%x\n", m->nativeStartOffset, m->nativeEndOffset, m->ilOffset));
#endif // LOGGING

        if (m->ilOffset == (ULONG) ICorDebugInfo::PROLOG ||
            m->ilOffset == (ULONG) ICorDebugInfo::EPILOG ||
            m->ilOffset == (ULONG) ICorDebugInfo::NO_MAPPING)
        {
            (*which)++;
        }

        if (nativeOffset >= m->nativeStartOffset
            && ((m->nativeEndOffset == 0 && 
                m->ilOffset != (ULONG) ICorDebugInfo::PROLOG)
                 || nativeOffset < m->nativeEndOffset))
        {
            SIZE_T ilOff = m->ilOffset;

            if( m->ilOffset == (ULONG) ICorDebugInfo::PROLOG )
            {
                ilOff = 0;
                (*map) = MAPPING_PROLOG;
                LOG((LF_CORDB,LL_INFO10000,"DJI::MNOTI: MAPPING_PROLOG\n"));
                
            }
            else if (m->ilOffset == (ULONG) ICorDebugInfo::NO_MAPPING)
            {
                ilOff = 0;
                (*map) = MAPPING_UNMAPPED_ADDRESS ;
                LOG((LF_CORDB,LL_INFO10000,"DJI::MNOTI:MAPPING_"
                    "UNMAPPED_ADDRESS\n"));
            }
            else if( m->ilOffset == (ULONG) ICorDebugInfo::EPILOG )
            {
                ilOff = m_lastIL;
                (*map) = MAPPING_EPILOG;
                LOG((LF_CORDB,LL_INFO10000,"DJI::MNOTI:MAPPING_EPILOG\n"));
            }
            else if (nativeOffset == m->nativeStartOffset)
            {
                (*map) = MAPPING_EXACT;
                LOG((LF_CORDB,LL_INFO10000,"DJI::MNOTI:MAPPING_EXACT\n"));
            }
            else
            {
                (*map) = MAPPING_APPROXIMATE;
                LOG((LF_CORDB,LL_INFO10000,"DJI::MNOTI:MAPPING_"
                    "APPROXIMATE\n"));
            }
            
            return ilOff;
        }
        m++;
    }

    return 0;
}

DebuggerJitInfo *DebuggerJitInfo::GetJitInfoByVersionNumber(SIZE_T nVer,
                                                            SIZE_T nVerMostRecentlyEnC)
{
    DebuggerJitInfo *dji = this;
    DebuggerJitInfo *djiMostRecent = NULL;
    SIZE_T nVerMostRecent = DJI_VERSION_FIRST_VALID;

    while( dji != NULL )
    {
        if (dji->m_nVersion == nVer && nVer>=DJI_VERSION_FIRST_VALID)
        {
            // we've found the one we're after, so stop here
            LOG((LF_CORDB,LL_INFO10000, "DJI:GJIBVN: We've found an exact "
                "match for ver 0x%x\n", nVer));
            break;
        }

        if ((nVer==DJI_VERSION_MOST_RECENTLY_JITTED ||
             nVer==DJI_VERSION_MOST_RECENTLY_EnCED)
            && dji->m_nVersion >= nVerMostRecent)// &&dji->m_jitComplete==false
        {
            LOG((LF_CORDB,LL_INFO10000, "DJI:GJIBVN: Found a version, perhaps "
                "most recent?0x%x, ver:0x%x\n", dji, dji->m_nVersion));
            nVerMostRecent = dji->m_nVersion;
            djiMostRecent = dji;
        }
        dji = dji->m_prevJitInfo;
    }

    if (nVer==DJI_VERSION_MOST_RECENTLY_JITTED)
    {
        dji = djiMostRecent;
        LOG((LF_CORDB,LL_INFO10000, "DJI:GJIBVN: Asked for most recently JITted. "
            "Found 0x%x, Ver#:0x%x\n", dji, nVerMostRecent));
    }

    if (nVer==DJI_VERSION_MOST_RECENTLY_EnCED &&
        djiMostRecent != NULL &&
        nVerMostRecentlyEnC==djiMostRecent->m_nVersion)
    {
        dji = djiMostRecent;
        LOG((LF_CORDB,LL_INFO10000, "DJI:GJIBVN: Asked for most recently EnCd. "
            "Found 0x%x, Ver#:0x%x\n", dji, nVerMostRecent));
    }

#ifdef LOGGING
    if (dji == NULL)
    {
        LOG((LF_CORDB,LL_INFO10000, "DJI:GJIBVN couldn't find a "
            "DJI corresponding to ver 0x%x\n", nVer));
    }
#endif //LOGGING

    return dji;
}


DebuggerJitInfo *DebuggerJitInfo::GetJitInfoByAddress(const BYTE *pbAddr )
{
    DebuggerJitInfo *dji = this;

    // If it's not NULL, but not in the range m_addrOfCode to end of function,
    //  then get the previous one.
    while( dji != NULL && 
            !(dji->m_addrOfCode<=PTR_TO_CORDB_ADDRESS(pbAddr) && 
              PTR_TO_CORDB_ADDRESS(pbAddr)<(dji->m_addrOfCode+
                    PTR_TO_CORDB_ADDRESS(dji->m_sizeOfCode))))
    {
        LOG((LF_CORDB,LL_INFO10000,"DJI:GJIBA: pbAddr 0x%x is not in code "
            "0x%x (size:0x%x)\n", pbAddr, (ULONG)dji->m_addrOfCode,
            (ULONG)dji->m_sizeOfCode));
        dji = dji->m_prevJitInfo;
    }

#ifdef LOGGING
    if (dji == NULL)
    {
        LOG((LF_CORDB,LL_INFO10000,"DJI:GJIBA couldn't find a DJI "
            "corresponding to addr 0x%x\n", pbAddr));
    }
#endif //LOGGING
    return dji;
}


// HRESULT DebuggerJitInfo::LoadEnCILMap() Grabs the old il to
//  new IL map.  If this DebuggerJitInfo already has an IL to IL map, 
//  then we must have been EnC'd twice without getting JITted, so we should
//  create a new DJI, load the EnCIL map into it, and then put it at the
//  head of the list (ie, in front of this DJI).
HRESULT DebuggerJitInfo::LoadEnCILMap(UnorderedILMap *ilMap)
{
    if (m_OldILToNewIL==NULL)
    {
        LOG((LF_CORDB,LL_INFO10000,"DJI::LEnCILM:Map for 0x%x!\n",this));
        
        _ASSERTE( m_cOldILToNewIL==0 );
        if (ilMap != NULL )
        {
            m_cOldILToNewIL = ilMap->cMap;
            m_OldILToNewIL = (DebuggerOldILToNewILMap*)new (interopsafe) DebuggerOldILToNewILMap[m_cOldILToNewIL];
            if (NULL == m_OldILToNewIL)
            {
                LOG((LF_CORDB,LL_INFO10000, "DJI::LEnCILM Not enough memory "
                    "to allocate EnC Map!\n"));
                return E_OUTOFMEMORY;
            }

            TRACE_ALLOC(m_OldILToNewIL);

            for (ULONG i = 0; i < m_cOldILToNewIL; i++)
            {
                m_OldILToNewIL[i].ilOffsetOld = ilMap->pMap[i].oldOffset;
                m_OldILToNewIL[i].ilOffsetNew = ilMap->pMap[i].newOffset;
                m_OldILToNewIL[i].fAccurate = ilMap->pMap[i].fAccurate;
            }
        }
        
        return S_OK;
    }
    else
    {  
        LOG((LF_CORDB,LL_INFO10000, "DJI::LEnCILM: Found existing "
            "map, extending chain after 0x%x\n", this));

        _ASSERTE( m_nextJitInfo == NULL );
        DebuggerJitInfo *dji = new (interopsafe) DebuggerJitInfo(m_fd);
        if (NULL == dji)
            return E_OUTOFMEMORY;

        HRESULT hr = dji->LoadEnCILMap( ilMap );
        if (FAILED(hr))
            return hr;
            
        hr = g_pDebugger->InsertAtHeadOfList( dji );
        _ASSERTE( m_nextJitInfo == dji);
        _ASSERTE( this == dji->m_prevJitInfo);
        return hr;
    }
}

SIZE_T DebuggerJitInfo::TranslateToInstIL(SIZE_T offOrig, bool fOrigToInst)
{
    if (m_cInstrumentedILMap == 0 || 
        ((int)offOrig < 0))
    {
        return offOrig;
    }
    else
    {
        _ASSERTE(m_rgInstrumentedILMap != NULL);
        
        SIZE_T iMap;
        for (iMap = 0; iMap < m_cInstrumentedILMap; iMap++)
        {
            if( offOrig < (fOrigToInst?m_rgInstrumentedILMap[iMap].oldOffset:
                                      m_rgInstrumentedILMap[iMap].newOffset))
            {
                break;
            }
        }

        iMap--; // Either we went one beyond what we wanted, or one beyond
                // the end of the array.
        if( (int)iMap < 0)
            return offOrig;
            
        SIZE_T offOldCompare;
        SIZE_T offNewCompare;
        
        if (fOrigToInst)
        {
            offOldCompare = m_rgInstrumentedILMap[iMap].oldOffset;
            offNewCompare = m_rgInstrumentedILMap[iMap].newOffset;
        }
        else
        {
            offOldCompare = m_rgInstrumentedILMap[iMap].newOffset;
            offNewCompare = m_rgInstrumentedILMap[iMap].oldOffset;
        }

        if ((int)offOldCompare > (int)offOrig)
            return offOrig;

        SIZE_T offTo;
        if (offOrig == offOldCompare)
        {
            offTo = offNewCompare;
        }
        else
        {
            // Integer math so that negative numbers get handled correctly
            offTo = offOrig + ((int)offNewCompare - 
                        (int)offOldCompare);
        }
            
        return offTo;
    }
}

BOOL IsDuplicatePatch(SIZE_T *rgEntries, USHORT cEntries,
                      SIZE_T Entry )
{
    for( int i = 0; i < cEntries;i++)
    {
        if (rgEntries[i] == Entry)
            return TRUE;
    }
    return FALSE;
}

/******************************************************************************
// HRESULT Debugger::MapAndBindFunctionBreakpoints():  For each breakpoint 
//      that we've set in any version of the existing function,
//      set a correponding breakpoint in the new function if we haven't moved
//      the patch to the new version already.
//
//      This must be done _AFTER_ the MethodDesc has been udpated 
//      with the new address (ie, when GetFunctionAddress pFD returns 
//      the address of the new EnC code)
//                                                                         
//                                                                                                 
 ******************************************************************************/
HRESULT Debugger::MapAndBindFunctionPatches(DebuggerJitInfo *djiNew,
                                            MethodDesc * fd,
                                            BYTE* addrOfCode)
{
    _ASSERTE(!djiNew || djiNew->m_fd == fd);

    HRESULT     hr =                S_OK;
    HASHFIND    hf;
    SIZE_T      *pidTableEntry =    NULL;
    SIZE_T      pidInCaseTableMoves;
    Module      *pModule =          g_pEEInterface->MethodDescGetModule(fd);
    mdMethodDef md =                fd->GetMemberDef();

    LOG((LF_CORDB,LL_INFO10000,"D::MABFP: All BPs will be mapped to "
        "Ver:0x%04x (DJI:0x%08x)\n", djiNew?djiNew->m_nVersion:0, djiNew));

    // First lock the patch table so it doesn't move while we're
    //  examining it.
    LOG((LF_CORDB,LL_INFO10000, "D::MABFP: About to lock patch table\n"));
    DebuggerController::Lock();

    // Manipulate tables AFTER lock's been acquired.
    DebuggerPatchTable *pPatchTable = DebuggerController::GetPatchTable();
    m_BPMappingDuplicates.Clear(); //dups are tracked per-version

    DebuggerControllerPatch *dcp = pPatchTable->GetFirstPatch(&hf);

    while (!FAILED(hr) && dcp != NULL)
    {
        // If we're missing the {module,methodDef} key, then use the 
        // MethodDesc to ensure that we're only mapping BPs for the
        // method indicated by djiNew, and not any others....
        if (dcp->key.module == NULL || dcp->key.md == mdTokenNil)
        {
            _ASSERTE(dcp->address != NULL);
            
            if (g_pEEInterface->IsManagedNativeCode(dcp->address))
            {
                MethodDesc *patchFD = g_pEEInterface->GetNativeCodeMethodDesc(dcp->address);

                if (patchFD != fd)
                    goto LNextLoop;
            }
        }

        // Only copy over breakpoints that are in this method
        if (dcp->key.module != pModule || dcp->key.md != md)
        {
            goto LNextLoop;
        }

        // 
        // If this is an EnC patch, which we only want to map
        // over if the patch belongs to a DebuggerBreakpoint OR
        // DebuggerStepper
        //
        // If neither of these is true, then we're EnCing and looking at
        // a patch that we don't want to bind - skip this patch
        if (dcp->dji != (DebuggerJitInfo*)DebuggerJitInfo::DJI_VERSION_INVALID && 
            !(dcp->controller->GetDCType() == DEBUGGER_CONTROLLER_BREAKPOINT||
              dcp->controller->GetDCType() == DEBUGGER_CONTROLLER_STEPPER)
            )
        {
            LOG((LF_CORDB, LL_INFO100000, "Neither stepper nor BP, & valid"
                 "DJI! - getting next patch!\n"));
            goto LNextLoop;
        }

        // The patch is for a 'BindFunctionPatches' call, but it's already bound
        if (dcp->dji == (DebuggerJitInfo*)DebuggerJitInfo::DJI_VERSION_INVALID && 
            dcp->address != NULL )
        {
            goto LNextLoop;
        }

        if (djiNew == NULL)
        {
            _ASSERTE(dcp->controller->GetDCType() == DEBUGGER_CONTROLLER_BREAKPOINT ||
                     dcp->controller->GetDCType() == DEBUGGER_CONTROLLER_STEPPER);
            _ASSERTE(dcp->native && dcp->offset == 0);

            DebuggerController::g_patches->BindPatch(dcp, addrOfCode);
            DebuggerController::ActivatePatch(dcp);
            goto LNextLoop;
        }

        if (dcp->controller->GetDCType() == DEBUGGER_CONTROLLER_STEPPER)
        {
			// Update the stepping patches if we have a new version of
			// the method being stepped
			DebuggerStepper * stepper = (DebuggerStepper*)dcp->controller;

			if (stepper->IsSteppedMethod(djiNew->m_fd))
				stepper->MoveToCurrentVersion(djiNew);
        }

        pidInCaseTableMoves = dcp->pid;
        
        // If we've already mapped this one to the current version,
        //  don't map it again.
        LOG((LF_CORDB,LL_INFO10000,"D::MABFP: Checking if 0x%x is a dup...", 
            pidInCaseTableMoves));
            
        if ( IsDuplicatePatch( m_BPMappingDuplicates.Table(), 
                              m_BPMappingDuplicates.Count(),
                              pidInCaseTableMoves) )
        {
            LOG((LF_CORDB,LL_INFO10000,"it is!\n"));
            goto LNextLoop;
        }
        LOG((LF_CORDB,LL_INFO10000,"nope!\n"));
        
        // Attempt mapping from patch to new version of code, and
        // we don't care if it turns out that there isn't a mapping.
        hr = MapPatchToDJI( dcp, djiNew );
        if (CORDBG_E_CODE_NOT_AVAILABLE == hr )
            hr = S_OK;

        if (FAILED(hr))
            break;

        //Remember the patch id to prevent duplication later
        pidTableEntry = m_BPMappingDuplicates.Append();
        if (NULL == pidTableEntry)
            hr = E_OUTOFMEMORY;

        *pidTableEntry = pidInCaseTableMoves;
        LOG((LF_CORDB,LL_INFO10000,"D::MABFP Adding 0x%x to list of "
            "already mapped patches\n", pidInCaseTableMoves));
LNextLoop:
        dcp = pPatchTable->GetNextPatch( &hf );
    }

    // Lastly, unlock the patch table so it doesn't move while we're
    //  examining it.
    DebuggerController::Unlock();
    LOG((LF_CORDB,LL_INFO10000, "D::MABFP: Unlocked patch table\n"));

    return hr;
}

/******************************************************************************
// HRESULT Debugger::MapPatchToDJI():  Maps the given
//  patch to the corresponding location at the new address.
//  We assume that the new code has been JITTed.
// Returns:  CORDBG_E_CODE_NOT_AVAILABLE - Indicates that a mapping wasn't
//  available, and thus no patch was placed.  The caller may or may
//  not care.
 ******************************************************************************/
HRESULT Debugger::MapPatchToDJI( DebuggerControllerPatch *dcp,DebuggerJitInfo *djiTo)
{
    _ASSERTE( djiTo->m_jitComplete == true );

    HRESULT hr = S_OK;
    BOOL fMappingForwards; //'forwards' mean from an earlier version to a more
        //recent version, ie, from a lower version number towards a higher one.
    
    SIZE_T ilOffsetOld;
    SIZE_T ilOffsetNew;
    SIZE_T natOffsetNew;

    DebuggerJitInfo *djiCur; //for walking the list

    bool fNormalMapping = true;
    CorDebugMappingResult mapping;
    DWORD which;
    BOOL irrelevant2;

    // If it's hashed by address, then there should be an opcode
    // Otherwise, don't do anything with it, since it isn't valid
    _ASSERTE( dcp->opcode == 0 || dcp->address != NULL);
    if (dcp->address != 0 && dcp->opcode == 0)
    {
        return S_OK;
    }

    // Grab the version it actually belongs to, then bring it forward
    djiCur = dcp->dji;

    if (djiCur == NULL) //then the BP has been mapped forwards into the
    {   // current version, or we're doing a BindFunctionPatches.  Either
        // way, we simply want the most recent version
        djiCur = g_pDebugger->GetJitInfo( djiTo->m_fd, NULL);
        dcp->dji = djiCur;
    }
    
    _ASSERTE( NULL != djiCur );

    // If the source and destination is the same, then this method
    // decays into BindFunctionPatch's BindPatch function
    if (djiCur == djiTo )
    {
        if (DebuggerController::BindPatch(dcp, 
                                          (const BYTE*)djiTo->m_addrOfCode, 
                                          NULL))
        {
            DebuggerController::ActivatePatch(dcp);
            LOG((LF_CORDB, LL_INFO1000, "Application went fine!\n" ));
            return S_OK;
        }
        else
        {
            LOG((LF_CORDB, LL_INFO1000, "Didn't apply for some reason!\n"));

            // Send an event to the Right Side so we know this patch didn't bind...
            LockAndSendBreakpointSetError(dcp);
            
            return CORDBG_E_CODE_NOT_AVAILABLE;
        }
    }
    
    LOG((LF_CORDB,LL_INFO10000,"D::MPTDJI: From pid 0x%x, "
        "Ver:0x%04x (DJI:0x%08x) to Ver:0x%04x (DJI:0x%08x)\n", 
        dcp->pid, djiCur->m_nVersion,djiCur,djiTo->m_nVersion, djiTo));

    // Grab the original IL offset
    if (dcp->native == TRUE)
    {
        ilOffsetOld = djiCur->MapNativeOffsetToIL(dcp->offset,&mapping,
                            &which);
        LOG((LF_CORDB,LL_INFO10000, "D::MPTDJI: offset is native0x%x, "
            "mapping to IL 0x%x mapping:0x%x, which:0x%x\n", 
            dcp->offset, ilOffsetOld, mapping, which));
    }
    else
    {
        ilOffsetOld = dcp->offset; 
        mapping = MAPPING_EXACT;
        LOG((LF_CORDB,LL_INFO10000, "D::MPTDJI: offset is IL 0x%x\n",
            ilOffsetOld));
    }               

    fMappingForwards = (djiCur->m_nVersion<djiTo->m_nVersion)?(TRUE):(FALSE);
#ifdef LOGGING
    if (fMappingForwards)
        LOG((LF_CORDB,LL_INFO1000,"D::MPTDJI: Mapping forwards from 0x%x to 0x%x!\n", 
            djiCur->m_nVersion,djiTo->m_nVersion));
    else
        LOG((LF_CORDB,LL_INFO1000,"D::MPTDJI: Mapping backwards from 0x%x to 0x%x!\n", 
            djiCur->m_nVersion,djiTo->m_nVersion));
#endif //LOGGING

    ilOffsetNew = ilOffsetOld;
    
    // Translate it to the new IL offset (through multiple versions, if needed)
    fNormalMapping = (mapping&(MAPPING_EXACT|MAPPING_APPROXIMATE))!=0;
    if ( fNormalMapping )
    {
        BOOL fAccurateIgnore;
        MapThroughVersions( ilOffsetOld, 
                            djiCur,
                            &ilOffsetNew, 
                            djiTo, 
                            fMappingForwards,
                            &fAccurateIgnore);
        djiCur = djiTo;
    }
    
    // Translate IL --> Native, if we want to
    if (!FAILED(hr))
    {
        if (fNormalMapping)
        {
            natOffsetNew = djiTo->MapILOffsetToNative(ilOffsetNew, &irrelevant2);
            LOG((LF_CORDB,LL_INFO10000, "D::MPTDJI: Mapping IL 0x%x (ji:0x%08x) "
                "to native offset 0x%x\n", ilOffsetNew, djiCur, natOffsetNew));
        }
        else
        {
            natOffsetNew = djiTo->MapSpecialToNative(mapping, which, &irrelevant2);
            LOG((LF_CORDB,LL_INFO10000, "D::MPTDJI: Mapping special 0x%x (ji:0x%8x) "
                "to native offset 0x%x\n", mapping, djiCur, natOffsetNew));
        }

        DebuggerBreakpoint *dbp = (DebuggerBreakpoint*)dcp->controller;
        //!!! TYPECAST ONLY WORKS B/C OF PRIOR TYPE CHECK, ABOVE!!!

        LOG((LF_CORDB,LL_INFO10000,"Adding patch to existing BP 0x%x\n",dbp));

        // Note that we don't want to create a new breakpoint, we just want
        // to put another patch down (in the new version) for the existing breakpoint
        // This will allow BREAKPOINT_REMOVE to continue to work.
        DebuggerController::AddPatch(dbp, 
                                     djiTo->m_fd, 
                                     true, 
                                     (const BYTE*)djiTo->m_addrOfCode+natOffsetNew, 
                                     dcp->fp,
                                     djiTo, 
                                     dcp->pid,
                                     natOffsetNew);

        LOG((LF_CORDB,LL_INFO10000, "D::MPTDJI: Copied bp\n"));
    }

    return hr;
}

/******************************************************************************
 *
 ******************************************************************************/
HRESULT Debugger::MapThroughVersions( SIZE_T fromIL, 
    DebuggerJitInfo *djiFrom,  
    SIZE_T *toIL, 
    DebuggerJitInfo *djiTo, 
    BOOL fMappingForwards,
    BOOL *fAccurate)
{
#ifdef LOGGING
    if (fMappingForwards)
        LOG((LF_CORDB,LL_INFO1000000, "D:MTV: From 0x%x (ver:0x%x) forwards to"
            " ver 0x%x\n", fromIL, djiFrom->m_nVersion, djiTo->m_nVersion));
    else
        LOG((LF_CORDB,LL_INFO1000000, "D:MTV: From 0x%x (ver:0x%x) backwards to"
            " ver 0x%x\n", fromIL, djiFrom->m_nVersion, djiTo->m_nVersion));
#endif //LOGGING

    _ASSERTE(fAccurate != NULL);
    
    DebuggerJitInfo *djiCur = djiFrom;
    HRESULT hr = S_OK;
    (*fAccurate) = TRUE;
    BOOL fAccurateTemp = TRUE;
    *toIL = fromIL; 
    
    while (djiCur != djiTo && djiCur != NULL)
    {
        hr = g_pDebugger->MapOldILToNewIL(fMappingForwards,
                                          djiCur->m_OldILToNewIL,
            djiCur->m_OldILToNewIL+djiCur->m_cOldILToNewIL,
                                          fromIL, 
                                          toIL,
                                          &fAccurateTemp);      
        if (!fAccurateTemp)
            (*fAccurate) = FALSE;
        
        if (FAILED(hr))
        {
            hr = CORDBG_E_CODE_NOT_AVAILABLE;
            break;
        }
        
        LOG((LF_CORDB,LL_INFO10000, "D::MPTDJI: Mapping IL 0x%x (ji:0x%08x) "
            "to IL 0x%x (ji:0x%08x)\n", fromIL, djiCur, *toIL, 
            djiCur->m_nextJitInfo));
            
        fromIL = *toIL;

        if (fMappingForwards)
            djiCur = djiCur->m_nextJitInfo;
        else
            djiCur = djiCur->m_prevJitInfo;
    }
    return hr;
}
/******************************************************************************
// HRESULT DebuggerJitInfo::MapOldILToNewIL():  Maps oldIL to the
//      corresponding newIL offset.  E_FAIL is returned if not matching
//      offset could be found.  
// BOOL fOldToNew:  If TRUE then we map from old to new.  Otherwise,
//      map from new to old
// DebuggerOldILToNewILMap *max:  This should be the 
//      DebuggerOldILToNewILMap element that's actually one beyond
//      the last valid map entry.  That way our binary search algorithm
//      will check the 'topmost' element if it has to.
 ******************************************************************************/
HRESULT Debugger::MapOldILToNewIL( BOOL fOldToNew, 
        DebuggerOldILToNewILMap *min, 
        DebuggerOldILToNewILMap *max, 
        SIZE_T oldIL, 
        SIZE_T *newIL,
        BOOL *fAccurate )
{
    _ASSERTE( newIL != NULL );
    _ASSERTE( fAccurate != NULL );
    if (min == NULL)
    {
        _ASSERTE( max==NULL );
        *newIL = oldIL;
        return S_OK;
    }       
    
    _ASSERTE( max!=NULL );
    _ASSERTE( min!=NULL );

    DebuggerOldILToNewILMap *mid = min + ((max - min)>>1);

    while (min + 1 < max)
    {

        if ( (fOldToNew && oldIL == mid->ilOffsetOld) ||
             (!fOldToNew && oldIL == mid->ilOffsetNew))
        {
            if (fOldToNew)
                *newIL = mid->ilOffsetNew;
            else
                *newIL = mid->ilOffsetOld;
            
            LOG((LF_CORDB,LL_INFO10000, "DJI::MOILTNIL map oldIL:0x%x "
                "to newIL:0x%x fAcc:0x%x\n", oldIL, 
                mid->ilOffsetNew,
                mid->fAccurate));
            (*fAccurate) = mid->fAccurate;
            
            return S_OK;
        }
        else if (oldIL < mid->ilOffsetOld)
            max = mid;
        else
            min = mid;
            
        mid = min + ((max - min)>>1);
    }

    if (fOldToNew)
    {
        _ASSERTE(oldIL >= min->ilOffsetOld);
        *newIL = min->ilOffsetNew + (oldIL - min->ilOffsetOld);

        // If we're not exact, then the user should check the result
        (*fAccurate) = (oldIL == min->ilOffsetOld)?min->fAccurate:FALSE; 
        
        LOG((LF_CORDB,LL_INFO10000, "DJI::MOILTNIL forwards oldIL:0x%x min->old:0x%x"
            "min->new:0x%x to newIL:0x%x fAcc:FALSE\n", oldIL, min->ilOffsetOld, 
            mid->ilOffsetNew, *newIL));
            
        return S_OK;
    }

    if (!fOldToNew)
    {
        _ASSERTE(oldIL >= min->ilOffsetNew);
        *newIL = min->ilOffsetOld + (oldIL - min->ilOffsetNew);
        
        // If we're not exact, then the user should check the result
        (*fAccurate) = (oldIL == min->ilOffsetOld)?min->fAccurate:FALSE; 

        LOG((LF_CORDB,LL_INFO10000, "DJI::MOILTNIL backwards oldIL:0x%x min->old:0x%x"
            "min->new:0x%x to newIL:0x%x fAcc:FALSE\n", oldIL, 
            min->ilOffsetOld, 
            mid->ilOffsetNew, 
            *newIL));
        return S_OK;
    }

    LOG((LF_CORDB,LL_INFO10000, "DJI::MOILTNIL unable to match "
        "oldIL of 0x%x!\n", oldIL));
    _ASSERTE( !"DJI::MOILTNIL We'll never get here unless we're FUBAR");
    
    return E_FAIL;
}

/* ------------------------------------------------------------------------ *
 * EE Interface routines
 * ------------------------------------------------------------------------ */

//
// DisableEventHandling ensures that only the calling Runtime thread
// is able to handle a debugger event. When it returns, the calling
// thread may take an action that could cause an IPC event to be sent
// to the Right Side. While event handling is "disabled", other
// Runtime threads will block.
//
void Debugger::DisableEventHandling(void)
{
    LOG((LF_CORDB,LL_INFO1000,"D::DEH about to wait\n"));

    if (!g_fProcessDetach)
    {
    rewait:
        // If there is an IDbgThreadControl interface, then someone wants
        // notification if this thread is going to block waiting to take
        // the lock.  So wait for 1 second and then if we timeout notify
        // the client.  If there is no client, wait without timeout.
        DWORD dwRes = WaitForSingleObject(
            m_eventHandlingEvent, m_pIDbgThreadControl ? 1000 : INFINITE);

        switch (dwRes)
        {
        case WAIT_TIMEOUT:
            _ASSERTE(m_pIDbgThreadControl);

            // If there is a IDebuggerThreadControl client, notify them of the
            // fact that the thread is blocking because of the debugger.
            m_pIDbgThreadControl->ThreadIsBlockingForDebugger();

            // When it returns, need to re-attempt taking of the lock, and
            // if it still takes a while, will re-notify the client.
            goto rewait;

            _ASSERTE(!"D::DEH - error, should not be here.");
            break;

        // Got the lock, so proceed as normal
        case WAIT_OBJECT_0:
            break;

#ifdef _DEBUG
        // Error case
        case WAIT_ABANDONED:
            _ASSERTE(!"D::DEH::WaitForSingleObject failed!");

        // Should never get here
        default:
            _ASSERTE(!"D::DEH reached default case in error.");
#endif
        }
    }

    LOG((LF_CORDB,LL_INFO1000,"D::DEH finished wait\n"));
}


//
// EnableEventHandling allows other Runtime threads to handle a
// debugger event. This funciton only enables event handling if the
// process is not stopped. If the process is stopped, then other
// Runtime threads should block instead of sending IPC events to the
// Right Side.
//
// STRONG NOTE: you better know exactly what the heck you're doing if
// you ever call this function with forceIt = true.
//
void Debugger::EnableEventHandling(bool forceIt)
{
    LOG((LF_CORDB,LL_INFO1000,"D::EEH about to signal forceIt:0x%x\n"
        ,forceIt));
        
    if (!g_fProcessDetach)
    {
        _ASSERTE(ThreadHoldsLock());

        if (!m_stopped || forceIt)
        {
            SetEvent(m_eventHandlingEvent);
        }
#ifdef LOGGING
        else
            LOG((LF_CORDB, LL_INFO10000,
                 "D::EEH: skipping enable event handling due to m_stopped\n"));
#endif                
    }
}

//
// SendSyncCompleteIPCEvent sends a Sync Complete event to the Right Side.
//
void Debugger::SendSyncCompleteIPCEvent()
{
    _ASSERTE(ThreadHoldsLock());

    LOG((LF_CORDB, LL_INFO10000, "D::SSCIPCE: sync complete.\n"));

    // We had better be trapping Runtime threads and not stopped yet.
    _ASSERTE(!m_stopped && m_trappingRuntimeThreads);

    // Okay, we're stopped now.
    m_stopped = TRUE;
    g_fRelaxTSLRequirement = true;

    // If we're attaching, then this is the first time that all the
    // threads in the process have synchronized. Before sending the
    // sync complete event, we send back events telling the Right Side
    // which modules have already been loaded.
    if (m_syncingForAttach == SYNC_STATE_1)
    {
        LOG((LF_CORDB, LL_INFO10000, "D::SSCIPCE: syncing for attach, sending "
             "current module set.\n"));

        HRESULT hr;
        BOOL fAtleastOneEventSent = FALSE;

        hr = IterateAppDomainsForAttach(ONLY_SEND_APP_DOMAIN_CREATE_EVENTS,
                                        &fAtleastOneEventSent, TRUE);
        _ASSERTE (fAtleastOneEventSent == TRUE || FAILED(hr));

        // update the state
        m_syncingForAttach = SYNC_STATE_2;
        LOG((LF_CORDB, LL_INFO10000, "Attach state is now %s\n", g_ppszAttachStateToSZ[m_syncingForAttach]));
    }

    {
        // Send the Sync Complete event to the Right Side
        DebuggerIPCEvent* ipce = m_pRCThread->GetIPCEventSendBuffer(IPC_TARGET_OUTOFPROC);
        InitIPCEvent(ipce, DB_IPCE_SYNC_COMPLETE);
        m_pRCThread->SendIPCEvent(IPC_TARGET_OUTOFPROC);
    }

    // This thread has sent a sync complete event. Now, its simply
    // going to go and wait for a Continue message (essentially.)
    // However, if the helper thread isn't started up yet, then there
    // will be a problem: no Continue event will ever be received. So,
    // if there is no helper thread, then we need to do temporary
    // helper thread duty now.
    if (!m_pRCThread->IsRCThreadReady())
    {
        DoHelperThreadDuty(true);
    }
}

DebuggerModule* Debugger::LookupModule(Module* pModule, AppDomain *pAppDomain)
{
    // if this is a module belonging to the system assembly, then scan
    // the complete list of DebuggerModules looking for the one
    // with a matching appdomain id
    // it.
    if (m_pModules == NULL)
        return (NULL);
    else if ((pModule->GetAssembly() == SystemDomain::SystemAssembly()) || pModule->GetAssembly()->IsShared())
    {
        // We have to make sure to lookup the module with the app domain parameter if the module lives in a shared
        // assembly or the system assembly. Bugs 65943 & 81728.
        return m_pModules->GetModule(pModule, pAppDomain);
    }
    else
        return m_pModules->GetModule(pModule);
}

//
// TrapAllRuntimeThreads causes every Runtime thread that is executing
// in the EE to trap and send the at safe point event to the RC thread as
// soon as possible. It also sets the EE up so that Runtime threads that
// are outside of the EE will trap when they try to re-enter.
//
BOOL Debugger::TrapAllRuntimeThreads(AppDomain *pAppDomain, BOOL fHoldingThreadStoreLock)
{
    pAppDomain = NULL;
    BOOL ret = FALSE;
    
    _ASSERTE(ThreadHoldsLock());

    // Only try to start trapping if we're not already trapping.
    if (m_trappingRuntimeThreads == FALSE)
    {
        LOG((LF_CORDB, LL_INFO10000,
             "D::TART: Trapping all Runtime threads.\n"));

        // There's no way that we should be stopped and still trying to call this function.
        _ASSERTE(!m_stopped);

        // Mark that we're trapping now.
        m_trappingRuntimeThreads = TRUE;

        // Take the thread store lock if we need to.
        if (!fHoldingThreadStoreLock)
        {
            LOG((LF_CORDB,LL_INFO1000, "About to lock thread Store\n"));
            ThreadStore::LockThreadStore(GCHeap::SUSPEND_FOR_DEBUGGER, FALSE);
            LOG((LF_CORDB,LL_INFO1000, "Locked thread store\n"));
        }

        // At this point, we know we have the thread store lock. We can therefore reset m_runtimeStoppedEvent, which may
        // have never been waited on. (If a Runtime thread trips in RareEnablePreemptiveGC, it will grab the thread
        // store lock but not call BlockAndReleaseTSLIfNecessary, which means the event remains high going into the new
        // trapping.) This can accidently let a thread release the TSL in BlockAndReleaseTSLIfNecessary prematurely.
        ResetEvent(m_runtimeStoppedEvent);
        
        // If all threads sync'd right away, go ahead and send now.
        if (g_pEEInterface->StartSuspendForDebug(pAppDomain, fHoldingThreadStoreLock))
        {
            LOG((LF_CORDB,LL_INFO1000, "Doin' the sync-complete!\n"));
            // Sets m_stopped = true...
            SendSyncCompleteIPCEvent();

            // Tell the caller that they own the thread store lock
            ret = TRUE;
        }
        else
        {
            LOG((LF_CORDB,LL_INFO1000, "NOT Doing' the sync thing\n"));
          
            // Otherwise, we are waiting for some number of threads to synchronize. Some of these threads will be
            // running in jitted code that is not interruptable. So we tell the RC Thread to check for such threads now
            // and then and help them get synchronized. (This is similar to what is done when suspending threads for GC
            // with the HandledJITCase() function.)
            m_pRCThread->WatchForStragglers();

            // Note, the caller shouldn't own the thread store lock in this case since the helper thread will need to be
            // able to take it to sweep threads.
            if (!fHoldingThreadStoreLock)
            {
                LOG((LF_CORDB,LL_INFO1000, "About to unlock thread store!\n"));
                ThreadStore::UnlockThreadStore();
                LOG((LF_CORDB,LL_INFO1000, "TART: Unlocked thread store!\n"));
            }
        }
    }

    return ret;
}


//
// ReleaseAllRuntimeThreads releases all Runtime threads that may be
// stopped after trapping and sending the at safe point event.
//
void Debugger::ReleaseAllRuntimeThreads(AppDomain *pAppDomain)
{
    pAppDomain = NULL;
    
    // Make sure that we were stopped...
    _ASSERTE(m_trappingRuntimeThreads && m_stopped);
    _ASSERTE(ThreadHoldsLock());

    LOG((LF_CORDB, LL_INFO10000, "D::RART: Releasing all Runtime threads"
        "for AppD 0x%x.\n", pAppDomain));

    // Mark that we're on our way now...
    m_trappingRuntimeThreads = FALSE;
    m_stopped = FALSE;

    // Go ahead and resume the Runtime threads.
    g_pEEInterface->ResumeFromDebug(pAppDomain);
}


/******************************************************************************
 *
 ******************************************************************************/
bool Debugger::FirstChanceNativeException(EXCEPTION_RECORD *exception,
                                          CONTEXT *context, 
                                          DWORD code,
                                          Thread *thread)
{
    if (!CORDBUnrecoverableError(this))
        return DebuggerController::DispatchNativeException(exception, context, 
                                                           code, thread);
    else
        return false;
}


/******************************************************************************
 *
 ******************************************************************************/
DWORD Debugger::GetPatchedOpcode(const BYTE *ip)
{
    if (!CORDBUnrecoverableError(this))
        return DebuggerController::GetPatchedOpcode(ip);
    else
        return 0;
}

/******************************************************************************
 *
 ******************************************************************************/
void Debugger::TraceCall(const BYTE *code)
{
    if (!CORDBUnrecoverableError(this))
        DebuggerController::DispatchTraceCall(g_pEEInterface->GetThread(), code);
}

/******************************************************************************
 *
 ******************************************************************************/
void Debugger::PossibleTraceCall(UMEntryThunk *pUMEntryThunk, Frame *pFrame)
{
    if (!CORDBUnrecoverableError(this))
        DebuggerController::DispatchPossibleTraceCall(g_pEEInterface->GetThread(), pUMEntryThunk, pFrame);
}

/******************************************************************************
 *
 ******************************************************************************/
bool Debugger::ThreadsAtUnsafePlaces(void)
{
    return (m_threadsAtUnsafePlaces != 0);
}

//
// SendBreakpoint is called by Runtime threads to send that they've
// hit a breakpoint to the Right Side.
//
void Debugger::SendBreakpoint(Thread *thread, CONTEXT *context, 
                              DebuggerBreakpoint *breakpoint)
{
    if (CORDBUnrecoverableError(this))
        return;

    LOG((LF_CORDB, LL_INFO10000, "D::SB: breakpoint BP:0x%x\n", breakpoint));

    _ASSERTE((g_pEEInterface->GetThread() &&
             !g_pEEInterface->GetThread()->m_fPreemptiveGCDisabled) ||
             g_fInControlC);

    _ASSERTE(ThreadHoldsLock());

    // Send a breakpoint event to the Right Side
    DebuggerIPCEvent* ipce = m_pRCThread->GetIPCEventSendBuffer(IPC_TARGET_OUTOFPROC);
    InitIPCEvent(ipce, 
                 DB_IPCE_BREAKPOINT, 
                 thread->GetThreadId(),
                 (void*) thread->GetDomain());
    ipce->BreakpointData.breakpointToken = breakpoint;
    _ASSERTE( breakpoint->m_pAppDomain == ipce->appDomainToken);

    m_pRCThread->SendIPCEvent(IPC_TARGET_OUTOFPROC);
}

//
// SendRawUserBreakpoint is called by Runtime threads to send that
// they've hit a user breakpoint to the Right Side. This is the event
// send only part, since it can be called from a few different places.
//
void Debugger::SendRawUserBreakpoint(Thread *thread)
{
    if (CORDBUnrecoverableError(this))
        return;

    LOG((LF_CORDB, LL_INFO10000, "D::SRUB: user breakpoint\n"));

    _ASSERTE(!g_pEEInterface->IsPreemptiveGCDisabled());
    _ASSERTE(ThreadHoldsLock());

    // Send a breakpoint event to the Right Side
    DebuggerIPCEvent* ipce = m_pRCThread->GetIPCEventSendBuffer(IPC_TARGET_OUTOFPROC);
    InitIPCEvent(ipce, 
                 DB_IPCE_USER_BREAKPOINT, 
                 thread->GetThreadId(),
                 thread->GetDomain());

    m_pRCThread->SendIPCEvent(IPC_TARGET_OUTOFPROC);
}

//
// SendStep is called by Runtime threads to send that they've
// completed a step to the Right Side.
//
void Debugger::SendStep(Thread *thread, CONTEXT *context, 
                        DebuggerStepper *stepper,
                        CorDebugStepReason reason)
{
    if (CORDBUnrecoverableError(this))
        return;
    
    LOG((LF_CORDB, LL_INFO10000, "D::SS: step:token:0x%x reason:0x%x\n",
        stepper, reason));

    _ASSERTE((g_pEEInterface->GetThread() &&
             !g_pEEInterface->GetThread()->m_fPreemptiveGCDisabled) ||
             g_fInControlC);

    _ASSERTE(ThreadHoldsLock());

    // Send a step event to the Right Side
    DebuggerIPCEvent* ipce = m_pRCThread->GetIPCEventSendBuffer(IPC_TARGET_OUTOFPROC);
    InitIPCEvent(ipce, 
                 DB_IPCE_STEP_COMPLETE, 
                 thread->GetThreadId(),
                 thread->GetDomain());
    ipce->StepData.stepperToken = stepper;
    ipce->StepData.reason = reason;
    m_pRCThread->SendIPCEvent(IPC_TARGET_OUTOFPROC);

    stepper->Delete();
}

void Debugger::SendEncRemapEvents(UnorderedEnCRemapArray *pEnCRemapInfo)
{                            
    LOG((LF_CORDB, LL_INFO10000, "D::SEnCRE: pEnCRemapInfo:0x%x\n", pEnCRemapInfo));

    if (CORDBUnrecoverableError(this))
        return;

    _ASSERTE(ThreadHoldsLock());

    USHORT cEvents = pEnCRemapInfo->Count();
    EnCRemapInfo *rgRemap = pEnCRemapInfo->Table();
    DebuggerIPCEvent* ipce = NULL;
    
    for (USHORT i = 0; i < cEvents; i++)
    {
        DebuggerModule *pDM = (DebuggerModule *)rgRemap[i].m_debuggerModuleToken;
        MethodDesc* pFD = g_pEEInterface->LookupMethodDescFromToken(
                pDM->m_pRuntimeModule, 
                rgRemap[i].m_funcMetadataToken);
        _ASSERTE(pFD != NULL);
        SIZE_T nVersionCur = GetVersionNumber(pFD);

        if (m_pJitInfos != NULL && !m_pJitInfos->EnCRemapSentForThisVersion(pFD->GetModule(),
                                                    pFD->GetMemberDef(), 
                                                    nVersionCur))
        {
            ipce = m_pRCThread->GetIPCEventSendBuffer(IPC_TARGET_OUTOFPROC);
            InitIPCEvent(ipce, 
                         DB_IPCE_ENC_REMAP,
                         rgRemap[i].m_threadId,
                         rgRemap[i].m_pAppDomainToken);

            ipce->EnCRemap.fAccurate = rgRemap[i].m_fAccurate;
            ipce->EnCRemap.funcMetadataToken = rgRemap[i].m_funcMetadataToken ;
            ipce->EnCRemap.debuggerModuleToken = rgRemap[i].m_debuggerModuleToken;
            ipce->EnCRemap.RVA = rgRemap[i].m_RVA;
            ipce->EnCRemap.localSigToken = rgRemap[i].m_localSigToken;

            LOG((LF_CORDB, LL_INFO10000, "D::SEnCRE: Sending remap for "
                "unjitted %s::%s MD:0x%x, debuggermodule:0x%x nVerCur:0x%x\n", 
                pFD->m_pszDebugClassName, pFD->m_pszDebugMethodName,
                ipce->EnCRemap.funcMetadataToken, 
                ipce->EnCRemap.debuggerModuleToken,
                nVersionCur));
            
     
            m_pRCThread->SendIPCEvent(IPC_TARGET_OUTOFPROC);

            // Remember not to send this event again if we can help it...
            m_pJitInfos->SetVersionNumberLastRemapped(pFD->GetModule(),
                                                    pFD->GetMemberDef(),
                                                    nVersionCur);
        }
    }

    pEnCRemapInfo->Clear();
    
    LOG((LF_CORDB, LL_INFO10000, "D::SEnCRE: Cleared queue, sending"
        "SYNC_COMPLETE\n"));
    
    ipce = m_pRCThread->GetIPCEventSendBuffer(IPC_TARGET_OUTOFPROC);
    InitIPCEvent(ipce, DB_IPCE_SYNC_COMPLETE);
    m_pRCThread->SendIPCEvent(IPC_TARGET_OUTOFPROC);
}

void Debugger::LockAndSendEnCRemapEvent(MethodDesc *pFD,
                                        BOOL fAccurate)
{
    if (CORDBUnrecoverableError(this))
        return;
    
    LOG((LF_CORDB, LL_INFO10000, "D::LASEnCRE:\n"));

    bool disabled;
    
    disabled =  g_pEEInterface->IsPreemptiveGCDisabled();

    if (disabled)
        g_pEEInterface->EnablePreemptiveGC();

    // Prevent other Runtime threads from handling events.
    BOOL threadStoreLockOwner = FALSE;
    
    LockForEventSending();
    
    if (CORDebuggerAttached())
    {
        // Send an EnC remap event to the Right Side.
        DebuggerIPCEvent* ipce = m_pRCThread->GetIPCEventSendBuffer(IPC_TARGET_OUTOFPROC);
        Thread *thread = g_pEEInterface->GetThread();
        InitIPCEvent(ipce, 
                     DB_IPCE_ENC_REMAP,
                     GetCurrentThreadId(),
                     (void *) thread->GetDomain());
        ipce->EnCRemap.fAccurate = fAccurate;
        ipce->EnCRemap.funcMetadataToken = pFD->GetMemberDef();

        Module *pRuntimeModule = pFD->GetModule();

        ipce->EnCRemap.debuggerModuleToken = g_pDebugger->LookupModule(
                                                pRuntimeModule,
                                                thread->GetDomain());

        // lotsa' args, just to get the local signature token, in case
        // we have to create the CordbFunction object on the right side.
        MethodDesc *pFDTemp;
        BYTE  *codeStartIgnore;
        unsigned int codeSizeIgnore;
        
        GetFunctionInfo(
             pRuntimeModule,
             ipce->EnCRemap.funcMetadataToken,
             &pFDTemp,
             &(ipce->EnCRemap.RVA),
             &codeStartIgnore,
             &codeSizeIgnore,
             &(ipce->EnCRemap.localSigToken) );

        _ASSERTE(pFD == pFDTemp);
        
        LOG((LF_CORDB, LL_INFO10000, "D::LASEnCRE: %s::%s fAcc:0x%x"
            "dmod:0x%x, methodDef:0x%x localsigtok:0x%x RVA:0x%x\n",
            pFD->m_pszDebugClassName, pFD->m_pszDebugMethodName,
            ipce->EnCRemap.fAccurate, ipce->EnCRemap.debuggerModuleToken,
            ipce->EnCRemap.funcMetadataToken, ipce->EnCRemap.localSigToken,
            ipce->EnCRemap.RVA));

        m_pRCThread->SendIPCEvent(IPC_TARGET_OUTOFPROC);

        // Stop all Runtime threads
        threadStoreLockOwner = TrapAllRuntimeThreads(thread->GetDomain());
    }
    
    // Let other Runtime threads handle their events.
    UnlockFromEventSending();

    BlockAndReleaseTSLIfNecessary(threadStoreLockOwner);

    g_pEEInterface->DisablePreemptiveGC();

    if (!disabled)
        g_pEEInterface->EnablePreemptiveGC();
}

//
// Send a BreakpointSetError event to the Right Side if the given patch is for a breakpoint. Note: we don't care if this
// fails, there is nothing we can do about it anyway, and the breakpoint just wont hit.
//
void Debugger::LockAndSendBreakpointSetError(DebuggerControllerPatch *patch)
{
    if (CORDBUnrecoverableError(this))
        return;

    // Only do this for breakpoint controllers
    DebuggerController *controller = patch->controller;

    if (controller->GetDCType() != DEBUGGER_CONTROLLER_BREAKPOINT)
        return;
    
    LOG((LF_CORDB, LL_INFO10000, "D::LASBSE:\n"));

    bool disabled;
    
    disabled =  g_pEEInterface->IsPreemptiveGCDisabled();

    if (disabled)
        g_pEEInterface->EnablePreemptiveGC();

    // Prevent other Runtime threads from handling events.
    BOOL threadStoreLockOwner = FALSE;
    
    LockForEventSending();    
    
    if (CORDebuggerAttached())   
    {
        // Send a breakpoint set error event to the Right Side.
        DebuggerIPCEvent* ipce = m_pRCThread->GetIPCEventSendBuffer(IPC_TARGET_OUTOFPROC);
        Thread *thread = g_pEEInterface->GetThread();
        InitIPCEvent(ipce, DB_IPCE_BREAKPOINT_SET_ERROR, GetCurrentThreadId(), (void *) thread->GetDomain());

        ipce->BreakpointSetErrorData.breakpointToken = controller;

        m_pRCThread->SendIPCEvent(IPC_TARGET_OUTOFPROC);

        // Stop all Runtime threads
        threadStoreLockOwner = TrapAllRuntimeThreads(thread->GetDomain());
    } 
    else 
    {
        LOG((LF_CORDB,LL_INFO1000, "D::LASBSE: Skipping SendIPCEvent because RS detached."));
    }

    // Let other Runtime threads handle their events.
    UnlockFromEventSending();

    BlockAndReleaseTSLIfNecessary(threadStoreLockOwner);

    g_pEEInterface->DisablePreemptiveGC();

    if (!disabled)
        g_pEEInterface->EnablePreemptiveGC();
}

//
// Called from the controller to lock the debugger for event
// sending. This is called before controller events are sent, like
// breakpoint, step complete, and thread started.
//
// Note that it's possible that the debugger detached (and destroyed our IPC
// events) while we're waiting for our turn. 
// So Callers should check for that case.
void Debugger::LockForEventSending(BOOL fNoRetry)
{
    // Any thread that has locked for event sending can't be interrupted by breakpoints or exceptions when were interop
    // debugging. SetDebugCantStop(true) helps us remember that. This is removed in BlockAndReleaseTSLIfNecessary.
    if (g_pEEInterface->GetThread())
        g_pEEInterface->GetThread()->SetDebugCantStop(true);
    
retry:

    // Prevent other Runtime threads from handling events.
    DisableEventHandling();
    Lock();

    if (m_stopped && !fNoRetry)
    {
        Unlock();
        goto retry;
    }
}

//
// Called from the controller to unlock the debugger from event
// sending. This is called after controller events are sent, like
// breakpoint, step complete, and thread started.
//
void Debugger::UnlockFromEventSending()
{
    // Let other Runtime threads handle their events.
    EnableEventHandling();
    Unlock();
}

//
// Called by threads that are holding the thread store lock. We'll block until the Runtime is resumed, then release the
// thread store lock.
//
void Debugger::BlockAndReleaseTSLIfNecessary(BOOL fHoldingThreadStoreLock)
{
    // Do nothing if we're not holding the thread store lock.
    if (fHoldingThreadStoreLock)
    {
        // We set the syncThreadIsLockFree event here. If we're in this call, with fHoldingThreadStoreLock true, then it
        // means that we're on the thread that sent up the sync complete flare, and we've released the debuger lock. By
        // setting this event, we allow the Right Side to suspend this thread now. (Note: this is all for Win32
        // debugging support.)
        if (m_pRCThread->GetDCB(IPC_TARGET_OUTOFPROC)->m_rightSideIsWin32Debugger)
            SetEvent(m_pRCThread->GetDCB(IPC_TARGET_OUTOFPROC)->m_syncThreadIsLockFree);
        
        // If the debugger wants notification of when a thread is going to block than this is
        // also a place where we should notify it.
        // NOTE: if the debugger chooses not to return control of the thread when we've give a
        //       ReleaseAllRuntimeThreads callback then the runtime will hang because this thread
        //       holds the ThreadStore lock.
        IDebuggerThreadControl *pDbgThreadControl = CorHost::GetDebuggerThreadControl();

        if (pDbgThreadControl)
            pDbgThreadControl->ThreadIsBlockingForDebugger();

        // Wait for the Runtime to be resumed.
        WaitForSingleObject(m_runtimeStoppedEvent, INFINITE);

        // Release the thread store lock.
        ThreadStore::UnlockThreadStore();
    }

    // Any thread that has locked for event sending can't be interrupted by breakpoints or exceptions when were interop
    // debugging. SetDebugCantStop helps us remember that. This was set in LockForEventSending.
    if (g_pEEInterface->GetThread())
        g_pEEInterface->GetThread()->SetDebugCantStop(false);
}

//
// Called from the controller after all events have been sent for a
// thread to sync the process.
//
BOOL Debugger::SyncAllThreads()
{
    if (CORDBUnrecoverableError(this))
        return FALSE;
    
    LOG((LF_CORDB, LL_INFO10000, "D::SAT: sync all threads.\n"));

    Thread *pThread = g_pEEInterface->GetThread();
    _ASSERTE((pThread &&
             !pThread->m_fPreemptiveGCDisabled) ||
              g_fInControlC);

    _ASSERTE(ThreadHoldsLock());
    
    // Stop all Runtime threads
    return TrapAllRuntimeThreads(pThread->GetDomain());
}

/******************************************************************************
 *
 ******************************************************************************/
SIZE_T Debugger::GetVersionNumber(MethodDesc *fd)
{
    LockJITInfoMutex();

    SIZE_T ver;
    if (m_pJitInfos != NULL && 
        fd != NULL)
        ver = m_pJitInfos->GetVersionNumber(fd->GetModule(), fd->GetMemberDef());
    else
        ver = DebuggerJitInfo::DJI_VERSION_FIRST_VALID;

    UnlockJITInfoMutex();

    return ver;
}

/******************************************************************************
 * If nVersionRemapped == DJI_VERSION_INVALID (0), then we'll set the 
 * last remapped version to whatever the current version number is.
 ******************************************************************************/
void Debugger::SetVersionNumberLastRemapped(MethodDesc *fd, SIZE_T nVersionRemapped)
{
    _ASSERTE(nVersionRemapped >=  DebuggerJitInfo::DJI_VERSION_FIRST_VALID);
    if (fd == NULL)
        return;

    LockJITInfoMutex();
#ifdef _DEBUG
    HRESULT hr =
#endif
    CheckInitJitInfoTable();
#ifdef _DEBUG
    VERIFY(SUCCEEDED(hr));
#endif
    _ASSERTE(m_pJitInfos != NULL);
    m_pJitInfos->SetVersionNumberLastRemapped(fd->GetModule(), 
                                            fd->GetMemberDef(), 
                                            nVersionRemapped);
    UnlockJITInfoMutex();
}

/******************************************************************************
 * 
 ******************************************************************************/
HRESULT Debugger::IncrementVersionNumber(Module *pModule, mdMethodDef token)
{
    LOG((LF_CORDB,LL_INFO10000,"D::INV:About to increment version number\n"));

    LockJITInfoMutex();
    HRESULT hr = m_pJitInfos->IncrementVersionNumber(pModule, token);
    UnlockJITInfoMutex();

    return hr;
}


/******************************************************************************
 *
 ******************************************************************************/
HRESULT Debugger::LaunchDebuggerForUser (void)
{
    LOG((LF_CORDB, LL_INFO10000, "D::LDFU: Attaching Debugger.\n"));

    // Should we ask the user if they want to attach here?

    return(AttachDebuggerForBreakpoint(g_pEEInterface->GetThread(),
                                       L"Launch for user"));
}

/******************************************************************************
 *
 ******************************************************************************/
WCHAR *Debugger::GetDebuggerLaunchString(void)
{
    WCHAR *cmd = NULL;
    DWORD len;

    // First, try the environment...
    len = WszGetEnvironmentVariable(CorDB_ENV_DEBUGGER_KEY, NULL, 0);

    if (len > 0)
    {
        // Len includes the terminating null. Note: using (interopsafe) because we may be out of other memory, not
        // because we're on the helper thread.
        cmd = new (interopsafe) WCHAR[len];
            
        if (cmd)
        {
            DWORD newlen = WszGetEnvironmentVariable(CorDB_ENV_DEBUGGER_KEY, cmd, len);

            if (newlen == 0)
            {
                DeleteInteropSafe(cmd);
                cmd = NULL;
            }
        }
    }
    
    if (cmd == NULL)
    {
        // Note: using (interopsafe) because we may be out of other memory,
        // not because we're on the helper thread.
        cmd = new (interopsafe) WCHAR[MAX_PATH];
        if (cmd) {
            if (!PAL_FetchConfigurationStringW(TRUE,
                               CorDB_REG_DEBUGGER_KEY,
                               cmd,
                               MAX_PATH)) {
                DeleteInteropSafe(cmd);
                cmd = NULL;
            }
        }
    }

    return cmd;
}


// Proxy code for EDA
struct EnsureDebuggerAttachedParams
{
    Debugger*                   m_pThis;
    AppDomain *                 m_pAppDomain;
    LPWSTR                      m_wszAttachReason;
    HRESULT                     m_retval;
    EnsureDebuggerAttachedParams() : 
        m_pThis(NULL), m_pAppDomain(NULL), m_wszAttachReason(NULL), m_retval(E_FAIL) {}
};

// This is called by the helper thread
void EDAHelperStub(EnsureDebuggerAttachedParams * p)
{
    p->m_retval = p->m_pThis->EDAHelper(p->m_pAppDomain, p->m_wszAttachReason);
}

// This gets called just like the normal version, but it sends the call over to the helper thread
HRESULT Debugger::EDAHelperProxy(AppDomain *pAppDomain, LPWSTR wszAttachReason)
{
    EnsureDebuggerAttachedParams p;
    p.m_pThis = this;
    p.m_pAppDomain= pAppDomain;
    p.m_wszAttachReason = wszAttachReason;
    
    m_pRCThread->DoFavor((DebuggerRCThread::FAVORCALLBACK) EDAHelperStub, &p);
    
    return p.m_retval;
}

// We can't have the helper thread execute all of EDA because it will deadlock.
// EDA will wait on m_exAttachEvent, which can only be set by the helper thread
// processing DB_IPCE_CONTINUE. But if the helper is stuck waiting in EDA, it
// can't handle the event and we deadlock.

// So, we factor out the stack intensive portion (CreateProcess & MessageBox)
// of EnsureDebuggerAttached. Conviently, this portion doesn't block 
// and so won't cause any deadlock.
HRESULT Debugger::EDAHelper(AppDomain *pAppDomain, LPWSTR wszAttachReason)
{
    HRESULT hr = S_OK;
    
    LOG((LF_CORDB, LL_INFO10000,
         "D::EDA: first process, initiating send\n"));

    DWORD pid = GetCurrentProcessId();

    // We provide a default debugger command, just for grins...
#ifdef PLATFORM_UNIX
    WCHAR *defaultDbgCmd = L"cordbg !a 0x%x";
#else
    WCHAR *defaultDbgCmd = L"cordbg.exe !a 0x%x";
#endif

    LOG((LF_CORDB, LL_INFO10000, "D::EDA: thread 0x%x is launching the debugger.\n", GetCurrentThreadId()));
    
    // Get the debugger to launch. realDbgCmd will point to a buffer that was allocated with (interopsafe) if
    // there is a user-specified debugger command string.
    WCHAR *realDbgCmd = GetDebuggerLaunchString();
    
    // Grab the ID for this appdomain.
    ULONG appId = pAppDomain->GetId();

    // Launch the debugger.
    DWORD len;
    
    if (realDbgCmd != NULL)
        len = wcslen(realDbgCmd)
              + 10                        // 10 for pid
              + 10                        // 10 for appid
              + wcslen(wszAttachReason)   // size of exception name
              + 10                        // 10 for handle value
              + 1;                        // 1 for null
    else
        len = wcslen(defaultDbgCmd) + 10 + 1;

    //
    // Note: We're using (interopsafe) allocations here not because the helper thread will ever run this code,
    // but because we may get here in low memory cases where our interop safe heap may still have a little room.
    //
    WCHAR *argsBuf = new (interopsafe) WCHAR[len];

    BOOL ret;
    STARTUPINFOW startupInfo = {0};
    startupInfo.cb = sizeof(STARTUPINFOW);
    PROCESS_INFORMATION processInfo = {0};

    if (argsBuf) 
    {

        if (realDbgCmd != NULL)
        {
            swprintf(argsBuf, realDbgCmd, pid, appId, wszAttachReason, m_exAttachAbortEvent);
        }
        else
            swprintf(argsBuf, defaultDbgCmd, pid, appId, wszAttachReason, m_exAttachAbortEvent);

        LOG((LF_CORDB, LL_INFO10000, "D::EDA: launching with command [%S]\n", argsBuf));

        // Grab the current directory.
        WCHAR *currentDir = NULL; // no current dir if this fails...
        WCHAR *currentDirBuf = new (interopsafe) WCHAR[MAX_PATH];
        
        if (currentDirBuf)
        {
            DWORD currentDirResult = WszGetCurrentDirectory(MAX_PATH, currentDirBuf);

            if (currentDirResult)
                currentDir = currentDirBuf;

            // Create the debugger process
            ret = WszCreateProcess(NULL, argsBuf,
                                   NULL, NULL, false, 
                                   CREATE_NEW_CONSOLE,
                                   NULL, currentDir, 
                                   &startupInfo,
                                   &processInfo);
        } else {
            ret = FALSE;
        }

        DeleteInteropSafe(currentDirBuf);

    }
    else
    {
        ret = FALSE;
    }
    
    if (ret)
    {
        LOG((LF_CORDB, LL_INFO10000,
             "D::EDA: debugger launched successfully.\n"));

        // We don't need a handle to the debugger process.
        CloseHandle(processInfo.hProcess);
	CloseHandle(processInfo.hThread);
    }
    else
    {
        DWORD err = GetLastError();
        
        int result = CorMessageBox(NULL, IDS_DEBUG_JIT_DEBUGGER_UNAVAILABLE, IDS_DEBUG_NO_DEBUGGER_FOUND,
                                   MB_RETRYCANCEL | MB_ICONEXCLAMATION | COMPLUS_MB_SERVICE_NOTIFICATION,
                                   TRUE, err, err, argsBuf);
        
        // If the user wants to attach a debugger manually (they press Retry), then pretend as if the launch
        // succeeded.
        if (result == IDRETRY)
            hr = S_OK;
        else
            hr = E_ABORT;
    }

    DeleteInteropSafe(argsBuf);     // DeleteInteropSafe does handle NULL safely.
    DeleteInteropSafe(realDbgCmd);  // ditto.

    if (FAILED(hr))
    {
        LOG((LF_CORDB, LL_INFO10000,
             "D::EDA: debugger did not launch successfully.\n"));

        // Make sure that any other threads that entered leave
        VERIFY(SetEvent(m_exAttachAbortEvent));
    }

    return hr;
}


/******************************************************************************
 *
 ******************************************************************************/
HRESULT Debugger::EnsureDebuggerAttached(AppDomain *pAppDomain,
                                         LPWSTR wszAttachReason)
{
    LOG( (LF_CORDB,LL_INFO10000,"D::EDA\n") );

    HRESULT hr = S_OK;

    if (!m_debuggerAttached)
    {
        Lock();

        // Remember that an exception is causing the attach.
        m_attachingForException = TRUE;

        // Only one thread throwing an exception when there is no
        // debugger attached should launch the debugger...
        m_exLock++;

        if (m_exLock == 1)
            hr = EDAHelperProxy(pAppDomain, wszAttachReason);

        if (SUCCEEDED(hr))
        {
            // Wait for the debugger to begin attaching to us.
            LOG((LF_CORDB, LL_INFO10000, "D::EDA: waiting on m_exAttachEvent "
                 "and m_exAttachAbortEvent\n"));

            HANDLE arrHandles[2] = {m_exAttachEvent, m_exAttachAbortEvent};

            // Let other threads in now
            Unlock();

            // Wait for one or the other to be set
            DWORD res = WaitForMultipleObjects(2, arrHandles, FALSE, INFINITE);

            // Finish up with lock
            Lock();

            // Indicate to the caller that the attach was aborted
            if (res == WAIT_OBJECT_0 + 1)
            {
                LOG((LF_CORDB, LL_INFO10000,
                     "D::EDA: m_exAttachAbortEvent set\n"));

                hr = E_ABORT;
            }

            // Otherwise, attach was successful
            else
            {
                _ASSERTE(res == WAIT_OBJECT_0 &&
                         "WaitForMultipleObjects failed!");

                LOG((LF_CORDB, LL_INFO10000, "D::EDA: m_exAttachEvent set\n"));
            }
        }

        // If this is the last thread, then reset the attach logic.
        m_exLock--;

        if (m_exLock == 0 && hr == E_ABORT)
        {
            // Reset the attaching logic.
            m_attachingForException = FALSE;
            ResetEvent(m_exAttachAbortEvent);
        }

        Unlock();
    }
    else
        hr = S_FALSE;

    LOG( (LF_CORDB, LL_INFO10000, "D::EDA:Leaving\n") );
    return hr;
}

/******************************************************************************
 *
 ******************************************************************************/
HRESULT Debugger::FinishEnsureDebuggerAttached()
{
    HRESULT hr = S_OK;

    LOG( (LF_CORDB,LL_INFO10000,"D::FEDA\n") );
    if (!m_debuggerAttached)
    {
        LOG((LF_CORDB, LL_INFO10000, "D::SE: sending sync complete.\n"));
        
        _ASSERTE(m_syncingForAttach != SYNC_STATE_0);
        
        // Send the Sync Complete event next...
        DebuggerIPCEvent* ipce = m_pRCThread->GetIPCEventSendBuffer(IPC_TARGET_OUTOFPROC);
        InitIPCEvent(ipce, DB_IPCE_SYNC_COMPLETE);
        hr = m_pRCThread->SendIPCEvent(IPC_TARGET_OUTOFPROC);
        LOG( (LF_CORDB,LL_INFO10000,"D::FEDA: just sent SYNC_COMPLETE\n") );

        LOG((LF_CORDB, LL_INFO10000,"D::FEDA: calling PAL_InitializeDebug.\n"));
        // Tell the PAL that we're trying to debug
        PAL_InitializeDebug();

        // Attach is complete now.
        LOG((LF_CORDB, LL_INFO10000, "D::FEDA: Attach Complete!"));
        g_pEEInterface->MarkDebuggerAttached();
        m_syncingForAttach = SYNC_STATE_0;
        LOG((LF_CORDB, LL_INFO10000, "Attach state is now %s\n", g_ppszAttachStateToSZ[m_syncingForAttach]));

        m_debuggerAttached = TRUE;
        m_attachingForException = FALSE;
    }
    LOG( (LF_CORDB,LL_INFO10000,"D::FEDA leaving\n") );

    _ASSERTE(SUCCEEDED(hr) && "FinishEnsureDebuggerAttached failed.");
    return (hr);
}

//
// SendException is called by Runtime threads to send that they've hit an exception to the Right Side.
//
HRESULT Debugger::SendException(Thread *thread, bool firstChance, bool continuable, bool fAttaching)
{
    LOG((LF_CORDB, LL_INFO10000, "D::SendException\n"));

    if (CORDBUnrecoverableError(this))
        return (E_FAIL);

    // Mark if we're at an unsafe place.
    bool atSafePlace = g_pDebugger->IsThreadAtSafePlace(thread);

    if (!atSafePlace)
        g_pDebugger->IncThreadsAtUnsafePlaces();

    // Is preemptive GC disabled on entry here?
    bool disabled = g_pEEInterface->IsPreemptiveGCDisabled();

    // We can only access the exception object while preemptive GC is disabled, so disable it if we need to.
    if (!disabled)
        g_pEEInterface->DisablePreemptiveGC();

    // Grab the exception name from the current exception object to pass to the JIT attach.
    OBJECTHANDLE *h = g_pEEInterface->GetThreadException(thread);
    OBJECTREF *o = *((OBJECTREF**)h);
    LPWSTR exceptionName;

    LPWSTR buf = new (interopsafe) WCHAR[MAX_CLASSNAME_LENGTH];
    
    if (o != NULL && *o != NULL && buf)
    {
        EEClass *c = (*o)->GetMethodTable()->GetClass();
        exceptionName = c->_GetFullyQualifiedNameForClass(buf, MAX_CLASSNAME_LENGTH);
    }
    else
        exceptionName = L"<Unknown exception>";

    // We have to send enabled, so enable now.
    g_pEEInterface->EnablePreemptiveGC();
    
    // If no debugger is attached, then launch one to attach to us.  Ignore hr: if EDA fails, app suspends in EDA &
    // waits for a debugger to attach to us.

    HRESULT hr = S_FALSE; // Return value of EDA if debugger already attached
    
    if (fAttaching)
    {
        hr = EnsureDebuggerAttached(thread->GetDomain(), exceptionName);
    }

    exceptionName = NULL;               // We can delete the buffer now.
    DeleteInteropSafe(buf);             

    BOOL threadStoreLockOwner = FALSE;
    
    if (SUCCEEDED(hr))
    {
        // Prevent other Runtime threads from handling events.

        // NOTE: if EnsureDebuggerAttached returned S_FALSE, this means that a debugger was already attached and
        // LockForEventSending should behave as normal.  If there was no debugger attached, then we have a special case
        // where this event is a part of debugger attaching and we've previously sent a sync complete event which means
        // that LockForEventSending will retry until a continue is called - however, with attaching logic the previous
        // continue didn't enable event handling and didn't continue the process - it's waiting for this event to be
        // sent, so we do so even if the process appears to be stopped.
        LockForEventSending(hr == S_OK);

        // In the JITattach case, an exception may be sent before the debugger is fully attached.        
        if (CORDebuggerAttached() || fAttaching)   
        {

            // Send an exception event to the Right Side
            DebuggerIPCEvent* ipce = m_pRCThread->GetIPCEventSendBuffer(IPC_TARGET_OUTOFPROC);
            InitIPCEvent(ipce, DB_IPCE_EXCEPTION, thread->GetThreadId(), (void*) thread->GetDomain());
            ipce->Exception.exceptionHandle = (void *) g_pEEInterface->GetThreadException(thread);
            ipce->Exception.firstChance = firstChance;
            ipce->Exception.continuable = continuable;

            LOG((LF_CORDB, LL_INFO10000, "D::SE: sending exception event from "
                "Thread:0x%x AD 0x%x.\n", ipce->threadId, ipce->appDomainToken));
            hr = m_pRCThread->SendIPCEvent(IPC_TARGET_OUTOFPROC);

            if (SUCCEEDED(hr))
            {
                // Stop all Runtime threads
                threadStoreLockOwner = TrapAllRuntimeThreads(thread->GetDomain());

                // If we're still syncing for attach, send sync complete now and mark that the debugger has completed
                // attaching.
                if (fAttaching) 
                {
                    hr = FinishEnsureDebuggerAttached();
                }
            }

            _ASSERTE(SUCCEEDED(hr) && "D::SE: Send exception event failed.");

        }
        else 
        {
            LOG((LF_CORDB,LL_INFO1000, "D:SE: Skipping SendIPCEvent because RS detached."));
        }
        
        // Let other Runtime threads handle their events.
        UnlockFromEventSending();
    }

    BlockAndReleaseTSLIfNecessary(threadStoreLockOwner);
    
    // Disable PGC
    g_pEEInterface->DisablePreemptiveGC();

    // If we weren't at a safe place when we enabled PGC, then go ahead and unmark that fact now that we've successfully
    // disabled.
    if (!atSafePlace)
        g_pDebugger->DecThreadsAtUnsafePlaces();

    //
    // Note: if there is a filter context installed, we may need remove it, do the eval, then put it back. I'm not 100%
    // sure which yet... it kinda depends on whether or not we really need the filter context updated due to a
    // collection during the func eval...
    //
    // If we need to do a func eval on this thread, then there will be a pending eval registered for this thread. We'll
    // loop so long as there are pending evals registered. We block in FuncEvalHijackWorker after sending up the
    // FuncEvalComplete event, so if the user asks for another func eval then there will be a new pending eval when we
    // loop and check again.
    //
    DebuggerPendingFuncEval *pfe;
    bool needRethrow = false;
    
    while (m_pPendingEvals != NULL && (pfe = m_pPendingEvals->GetPendingEval(thread)) != NULL)
    {
        DebuggerEval *pDE = pfe->pDE;

        _ASSERTE(pDE->m_evalDuringException);

        // Remove the pending eval from the hash. This ensures that if we take a first chance exception during the eval
        // that we can do another nested eval properly.
        m_pPendingEvals->RemovePendingEval(thread);

        // Go ahead and do the pending func eval.
#ifdef _DEBUG
        void *ret =
#endif
        ::FuncEvalHijackWorker(pDE);

        // The return value should be NULL when FuncEvalHijackWorker is called as part of an exception.
        _ASSERTE(ret == NULL);

        // If this eval ended in a ThreadAbortException, remember that we need to rethrow it after all evals are done.
        needRethrow |= pDE->m_rethrowAbortException;
    }

    // If we need to re-throw a ThreadAbortException, go ahead and do it now.
    if (needRethrow)
        thread->UserAbort(NULL);

    if (!disabled)
        g_pEEInterface->EnablePreemptiveGC();

    return (hr);
}


//
// FirstChanceManagedException is called by Runtime threads when an exception is first detected, but before any filters have
// been run.
//
bool Debugger::FirstChanceManagedException(bool continuable, CONTEXT *pContext)
{
    LOG((LF_CORDB, LL_INFO10000, "D::FCE: First chance exception, continuable:0x%x\n", continuable));

    Thread *thread = g_pEEInterface->GetThread();

#ifdef _DEBUG
    static ConfigDWORD d_fce;
    if (d_fce.val(L"D::FCE", 0))
        _ASSERTE(!"Stop in Debugger::FirstChanceManagedException?");
#endif

    SendException(thread, true, continuable, FALSE);
    
    if (continuable && g_pEEInterface->IsThreadExceptionNull(thread))
        return true;

    return false;
}

//
// ExceptionFilter is called by the Runtime threads when an exception
// is being processed.
//
void Debugger::ExceptionFilter(BYTE *pStack, MethodDesc *fd, SIZE_T offset)
{
    LOG((LF_CORDB,LL_INFO10000, "D::EF: pStack:0x%x MD: %s::%s, offset:0x%x\n",
        pStack, fd->m_pszDebugClassName, fd->m_pszDebugMethodName, offset));

    //
    // !!! Need to think through logic for when to step through filter code - 
    // perhaps only during a "step in".
    //

    // 
    // !!! Eventually there may be some weird mechanics introduced for
    // returning from the filter that we have to understand.  For now we should
    // be able to proceed normally.
    // 
    
    DebuggerController::DispatchUnwind(g_pEEInterface->GetThread(), 
                                       fd, offset, pStack, STEP_EXCEPTION_FILTER);
    
}


//
// ExceptionHandle is called by Runtime threads when an exception is
// being handled.
//
void Debugger::ExceptionHandle(BYTE *pStack, MethodDesc *fd, SIZE_T offset)
{   
    
    DebuggerController::DispatchUnwind(g_pEEInterface->GetThread(), 
                                       fd, offset, pStack, STEP_EXCEPTION_HANDLER);
    
}

//
// ExceptionCLRCatcherFound() is called by Runtime when we determine that we're crossing back into unmanaged code and
// we're going to turn an exception in a HR.
//
void Debugger::ExceptionCLRCatcherFound()
{
    DebuggerController::DispatchCLRCatch(g_pEEInterface->GetThread());
}

/******************************************************************************
 *
 ******************************************************************************/
LONG Debugger::LastChanceManagedException(EXCEPTION_RECORD *pExceptionRecord, 
                                          CONTEXT *pContext,
                                          Thread *pThread,
                                          UnhandledExceptionLocation location)
{
    LOG((LF_CORDB, LL_INFO10000, "D::LastChanceManagedException\n"));

    if (CORDBUnrecoverableError(this))
        return ExceptionContinueSearch;

    // We don't do anything on the second pass
    if ((pExceptionRecord->ExceptionFlags & EXCEPTION_UNWINDING) != 0)
        return ExceptionContinueSearch;

    // Let the controllers have a chance at it - this may be the only handler which can catch the exception if this is a
    // native patch.
   
    if (pThread != NULL && m_debuggerAttached && DebuggerController::DispatchNativeException(pExceptionRecord, pContext, 
                                                                                             pExceptionRecord->ExceptionCode, 
                                                                                             pThread))
        return ExceptionContinueExecution;

    // If this is a non-EE exception, don't do anything.
    if (pThread == NULL || g_pEEInterface->IsThreadExceptionNull(pThread))
        return ExceptionContinueSearch;

    // Otherwise, run our last chance exception logic
    ATTACH_ACTION action = ATTACH_NO;


    if (m_debuggerAttached || ((action = ShouldAttachDebuggerProxy(false, location)) == ATTACH_YES))
    {
        LOG((LF_CORDB, LL_INFO10000, "D::BEH ... debugger attached.\n"));

        Thread *thread = g_pEEInterface->GetThread();

        // ExceptionFlags is 0 for continuable, EXCEPTION_NONCONTINUABLE otherwise
        bool continuable = (pExceptionRecord->ExceptionFlags == 0);

        LOG((LF_CORDB, LL_INFO10000, "D::BEH ... sending exception.\n"));

        // We pass the attaching status to SendException so that it knows
        // whether to attach a debugger or not. We should really do the 
        // attach stuff out here and not bother with the flag.
        SendException(thread, false, continuable, action == ATTACH_YES);

        if (continuable && g_pEEInterface->IsThreadExceptionNull(thread))
            return ExceptionContinueExecution;
    }
    else
    {
        // Note: we don't do anything on NO or TERMINATE. We just return to the exception logic, which will abort the
        // app or not depending on what the CLR impl decides is appropiate.
        _ASSERTE(action == ATTACH_TERMINATE || action == ATTACH_NO);
    }

    return ExceptionContinueSearch;
}



// This function checks the registry for the debug launch setting upon encountering an exception or breakpoint.
DebuggerLaunchSetting Debugger::GetDbgJITDebugLaunchSetting(void)
{
    // Query for the value "DbgJITDebugLaunchSetting"
    DWORD dwSetting = 0;

#ifdef PLATFORM_UNIX
    // don't launch the cordbg by default on Unix
    dwSetting = 1;
#else
    // show a dialog by default on Windows
    dwSetting = 0;
#endif

    dwSetting = REGUTIL::GetConfigDWORD(CorDB_REG_QUESTION_KEY, dwSetting);

    DebuggerLaunchSetting ret = (DebuggerLaunchSetting)dwSetting;
    
    return ret;
}


//
// NotifyUserOfFault notifies the user of a fault (unhandled exception
// or user breakpoint) in the process, giving them the option to
// attach a debugger or terminate the application.
//
int Debugger::NotifyUserOfFault(bool userBreakpoint, DebuggerLaunchSetting dls)
{
    LOG((LF_CORDB, LL_INFO1000000, "D::NotifyUserOfFault\n"));

    int result = IDCANCEL;

    if (!m_debuggerAttached)
    {
        DWORD pid;
        DWORD tid;
        
        pid = GetCurrentProcessId();
        tid = GetCurrentThreadId();

        DWORD flags = 0;


        if (userBreakpoint)
        {
            result = CorMessageBox(NULL, IDS_DEBUG_USER_BREAKPOINT_MSG, IDS_DEBUG_SERVICE_CAPTION,
                                   MB_ABORTRETRYIGNORE | MB_ICONEXCLAMATION | flags, TRUE, pid, pid, tid, tid);
        }
        else
        {
            result = CorMessageBox(NULL, IDS_DEBUG_UNHANDLED_EXCEPTION_MSG, IDS_DEBUG_SERVICE_CAPTION,
                                   MB_OKCANCEL | MB_ICONEXCLAMATION | flags, TRUE, pid, pid, tid, tid);
        }
    }

    LOG((LF_CORDB, LL_INFO1000000, "D::NotifyUserOfFault left\n"));
    return result;
}


// Proxy for ShouldAttachDebugger
struct ShouldAttachDebuggerParams {
    Debugger*                   m_pThis;
    bool                        m_fIsUserBreakpoint;
    UnhandledExceptionLocation  m_location;
    Debugger::ATTACH_ACTION     m_retval;
};

// This is called by the helper thread
void ShouldAttachDebuggerStub(ShouldAttachDebuggerParams * p)
{
    p->m_retval = p->m_pThis->ShouldAttachDebugger(p->m_fIsUserBreakpoint, p->m_location);
}

// This gets called just like the normal version, but it sends the call over to the helper thread
Debugger::ATTACH_ACTION Debugger::ShouldAttachDebuggerProxy(bool fIsUserBreakpoint, UnhandledExceptionLocation location)
{
    ShouldAttachDebuggerParams p;
    p.m_pThis = this;
    p.m_fIsUserBreakpoint = fIsUserBreakpoint;
    p.m_location = location;

    LOG((LF_CORDB, LL_INFO1000000, "D::SADProxy\n"));
    m_pRCThread->DoFavor((DebuggerRCThread::FAVORCALLBACK) ShouldAttachDebuggerStub, &p);
    LOG((LF_CORDB, LL_INFO1000000, "D::SADProxy return %d\n", p.m_retval));
    
    return p.m_retval;
}

// Returns true if the debugger is not attached and DbgJITDebugLaunchSetting is set to either ATTACH_DEBUGGER or
// ASK_USER and the user request attaching.
Debugger::ATTACH_ACTION Debugger::ShouldAttachDebugger(bool fIsUserBreakpoint, UnhandledExceptionLocation location)
{
    LOG((LF_CORDB, LL_INFO1000000, "D::SAD\n"));

    // If the debugger is already attached, not necessary to re-attach
    if (m_debuggerAttached)
        return ATTACH_NO;

    // Check if the user has specified a seting in the registry about what he wants done when an unhandled exception
    // occurs.
    DebuggerLaunchSetting dls = GetDbgJITDebugLaunchSetting();   

    // First, we just don't attach if the location of the exception doesn't fit what the user is looking for. Note: a
    // location of 0 indicates none specified, in which case we only let locations specified in DefaultDebuggerAttach
    // through. This is for backward compatability and convience.
    UnhandledExceptionLocation userLoc = (UnhandledExceptionLocation)(dls >> DLS_LOCATION_SHIFT);

    if ((userLoc == 0) && !(location & DefaultDebuggerAttach))
    {
        return ATTACH_NO;
    }
    else if ((userLoc != 0) && !(userLoc & location))
    {
        return ATTACH_NO;
    }

    // Now that we've passed the location test, how does the user want to attach?
    if (dls & DLS_ATTACH_DEBUGGER)
    {
        // Attach without asking the user...
        return ATTACH_YES;
    }
    else if (dls & DLS_TERMINATE_APP)
    {
        // We just want to ignore user breakpoints if the registry says to "terminate" the app.
        if (fIsUserBreakpoint)
            return ATTACH_NO;
        else
            return ATTACH_TERMINATE;
    }
    else
    {
        // Only ask the user once if they wish to attach a debugger.  This is because LastChanceManagedException can be called
        // twice, which causes ShouldAttachDebugger to be called twice, which causes the user to have to answer twice.
        static BOOL s_fHasAlreadyAsked = FALSE;
        static ATTACH_ACTION s_action;

        // This lock is also part of the above hack.
        Lock();

        // We always want to ask about user breakpoints!
        if (!s_fHasAlreadyAsked || fIsUserBreakpoint)
        {
            if (!fIsUserBreakpoint)
                s_fHasAlreadyAsked = TRUE;
            
            // Ask the user if they want to attach
            int iRes = NotifyUserOfFault(fIsUserBreakpoint, dls);

            // If it's a user-defined breakpoint, they must hit Retry to launch
            // the debugger.  If it's an unhandled exception, user must press
            // Cancel to attach the debugger.
            if ((iRes == IDCANCEL) || (iRes == IDRETRY))
                s_action = ATTACH_YES;

            else if ((iRes == IDABORT) || (iRes == IDOK))
                s_action = ATTACH_TERMINATE;

            else
                s_action = ATTACH_NO;
        }

        Unlock();

        return s_action;
    }
}


/******************************************************************************
 *
 ******************************************************************************/
HRESULT Debugger::AttachDebuggerForBreakpoint(Thread *thread,
                                              WCHAR *wszLaunchReason)
{
    // Mark if we're at an unsafe place.
    bool atSafePlace = g_pDebugger->IsThreadAtSafePlace(thread);

    if (!atSafePlace)
        g_pDebugger->IncThreadsAtUnsafePlaces();

    // Enable preemptive GC...
    bool disabled = g_pEEInterface->IsPreemptiveGCDisabled();

    if (disabled)
        g_pEEInterface->EnablePreemptiveGC();
    
    // If no debugger is attached, then launch one to attach to us.
    HRESULT hr = EnsureDebuggerAttached(thread->GetDomain(), wszLaunchReason);

    BOOL threadStoreLockOwner = FALSE;
    
    if (SUCCEEDED(hr))
    {
        // Prevent other Runtime threads from handling events.

        // NOTE: if EnsureDebuggerAttached returned S_FALSE, this means that
        // a debugger was already attached and LockForEventSending should
        // behave as normal.  If there was no debugger attached, then we have
        // a special case where this event is a part of debugger attaching and
        // we've previously sent a sync complete event which means that
        // LockForEventSending will retry until a continue is called - however,
        // with attaching logic the previous continue didn't enable event
        // handling and didn't continue the process - it's waiting for this
        // event to be sent, so we do so even if the process appears to be
        // stopped.

        LockForEventSending(hr == S_OK);
        
        // Send a user breakpoint event to the Right Side
        SendRawUserBreakpoint(thread);

        // Stop all Runtime threads
        threadStoreLockOwner = TrapAllRuntimeThreads(thread->GetDomain());

        // If we're still syncing for attach, send sync complete now and
        // mark that the debugger has completed attaching.
        hr = FinishEnsureDebuggerAttached();
        
        // Let other Runtime threads handle their events.
        UnlockFromEventSending();
    }

    BlockAndReleaseTSLIfNecessary(threadStoreLockOwner);
    
    // Set back to disabled GC
    g_pEEInterface->DisablePreemptiveGC();

    if (!disabled)
        g_pEEInterface->EnablePreemptiveGC();

    if (!atSafePlace)
        g_pDebugger->DecThreadsAtUnsafePlaces();

    return (hr);
}


//
// SendUserBreakpoint is called by Runtime threads to send that they've hit
// a user breakpoint to the Right Side.
//
void Debugger::SendUserBreakpoint(Thread *thread)
{
    if (CORDBUnrecoverableError(this))
        return;

    // Ask the user if they want to attach the debugger
    ATTACH_ACTION dbgAction;
    
    // If user wants to attach the debugger, do so
    if (m_debuggerAttached || ((dbgAction = ShouldAttachDebugger(true, ProcessWideHandler)) == ATTACH_YES))
    {
        _ASSERTE(g_pEEInterface->GetThreadFilterContext(thread) == NULL);
        _ASSERTE(!ISREDIRECTEDTHREAD(thread));

        if (m_debuggerAttached)
        {
            // A debugger is already attached, so setup a DebuggerUserBreakpoint controller to get us out of the helper
            // that got us here. The DebuggerUserBreakpoint will call AttachDebuggerForBreakpoint for us when we're out
            // of the helper. The controller will delete itself when its done its work.
            new (interopsafe) DebuggerUserBreakpoint(thread);
        }
        else
        {
            // No debugger attached, so go ahead and just try to send the user breakpoint
            // event. AttachDebuggerForBreakpoint will ensure that the debugger is attached before sending the event.
#ifdef _DEBUG
            HRESULT hr =
#endif
            AttachDebuggerForBreakpoint(thread, L"Launch for user");
            _ASSERTE(SUCCEEDED(hr) || hr == E_ABORT);
        }
    }
    else if (dbgAction == ATTACH_TERMINATE)
    {
        // ATTACH_TERMINATE indicates the the user wants to terminate the app.
        LOG((LF_CORDB, LL_INFO10000, "D::SUB: terminating this process due to user request\n"));

        TerminateProcess(GetCurrentProcess(), 0);
        _ASSERTE(!"Should never reach this point.");
    }
    else
    {
        _ASSERTE(dbgAction == ATTACH_NO);
    }
}


// void Debugger::ThreadCreated():  ThreadCreated is called when 
// a new Runtime thread has been created, but before its ever seen 
// managed code.  This is a callback invoked by the EE into the Debugger.
// This will create a DebuggerThreadStarter patch, which will set
// a patch at the first instruction in the managed code.  When we hit
// that patch, the DebuggerThreadStarter will invoke ThreadStarted, below.
//
// Thread* pRuntimeThread:  The EE Thread object representing the
//      runtime thread that has just been created.
void Debugger::ThreadCreated(Thread* pRuntimeThread)
{
    if (CORDBUnrecoverableError(this))
        return;

    LOG((LF_CORDB, LL_INFO100, "D::TC: thread created for 0x%x. ******\n",
         pRuntimeThread->GetThreadId()));

    // Create a thread starter and enable its WillEnterManaged code
    // callback. This will cause the starter to trigger once the
    // thread has hit managed code, which will cause
    // Debugger::ThreadStarted() to be called.  NOTE: the starter will
    // be deleted automatically when its done its work.
    DebuggerThreadStarter *starter = new (interopsafe) DebuggerThreadStarter(pRuntimeThread);

    if (!starter)
    {
        CORDBDebuggerSetUnrecoverableWin32Error(this, 0, false);
        return;
    }
    
    starter->EnableTraceCall(NULL);
}

    
// void Debugger::ThreadStarted():  ThreadStarted is called when 
// a new Runtime thread has reached its first managed code. This is
// called by the DebuggerThreadStarter patch's SendEvent method.
//
// Thread* pRuntimeThread:  The EE Thread object representing the
//      runtime thread that has just hit managed code.
void Debugger::ThreadStarted(Thread* pRuntimeThread,
                             BOOL fAttaching)
{
    if (CORDBUnrecoverableError(this))
        return;

    LOG((LF_CORDB, LL_INFO100, "D::TS: thread attach : ID=%#x AD:%#x isAttaching:%d.\n",
         pRuntimeThread->GetThreadId(), pRuntimeThread->GetDomain(), fAttaching));

    //
    // If we're attaching, then we only need to send the event. We
    // don't need to disable event handling or lock the debugger
    // object.
    //
#ifdef _DEBUG
    if (!fAttaching)
    {
        _ASSERTE((g_pEEInterface->GetThread() &&
                 !g_pEEInterface->GetThread()->m_fPreemptiveGCDisabled) ||
                 g_fInControlC);
        _ASSERTE(ThreadHoldsLock());
    }
#endif

    DebuggerIPCEvent* ipce = m_pRCThread->GetIPCEventSendBuffer(IPC_TARGET_OUTOFPROC);
    InitIPCEvent(ipce, 
                 DB_IPCE_THREAD_ATTACH, 
                 pRuntimeThread->GetThreadId(),
                 (void *) pRuntimeThread->GetDomain());
    ipce->ThreadAttachData.debuggerThreadToken = (void*) pRuntimeThread;
#if PLATFORM_UNIX
    ipce->ThreadAttachData.threadHandle = INVALID_HANDLE_VALUE;
#else
    ipce->ThreadAttachData.threadHandle = pRuntimeThread->GetThreadHandle();
#endif
    ipce->ThreadAttachData.firstExceptionHandler = (void *)pRuntimeThread->GetExceptionListPtr();
    m_pRCThread->SendIPCEvent(IPC_TARGET_OUTOFPROC);

    if (!fAttaching)
    {
        //
        // Well, if this thread got created _after_ we started sync'ing
        // then its Runtime thread flags don't have the fact that there
        // is a debug suspend pending. We need to call over to the
        // Runtime and set the flag in the thread now...
        //
        if (m_trappingRuntimeThreads)
            g_pEEInterface->MarkThreadForDebugSuspend(pRuntimeThread);
    }
}

// DetachThread is called by Runtime threads when they are completing
// their execution and about to be destroyed.
//
void Debugger::DetachThread(Thread *pRuntimeThread, BOOL fHoldingThreadstoreLock)
{
    if (CORDBUnrecoverableError(this))
        return;

    if (m_ignoreThreadDetach)
        return;

    //
    _ASSERTE(!fHoldingThreadstoreLock);
    _ASSERTE (pRuntimeThread != NULL);

    if (!g_fEEShutDown && !IsDebuggerAttachedToAppDomain(pRuntimeThread))
        return;

    LOG((LF_CORDB, LL_INFO100, "D::DT: thread detach : ID=%#x AD:%#x.\n",
         pRuntimeThread->GetThreadId(), pRuntimeThread->GetDomain()));

    bool disabled = g_pEEInterface->IsPreemptiveGCDisabled();

    if (disabled)
        g_pEEInterface->EnablePreemptiveGC();

    // Prevent other Runtime threads from handling events.
    BOOL threadStoreLockOwner = FALSE;
    
    LockForEventSending();
    
    if (CORDebuggerAttached()) 
    {
        // Send a detach thread event to the Right Side.
        DebuggerIPCEvent* ipce = m_pRCThread->GetIPCEventSendBuffer(IPC_TARGET_OUTOFPROC);
        InitIPCEvent(ipce, 
                     DB_IPCE_THREAD_DETACH, 
                     pRuntimeThread->GetThreadId(),
                     (void *) pRuntimeThread->GetDomain());
        m_pRCThread->SendIPCEvent(IPC_TARGET_OUTOFPROC);

        // Stop all Runtime threads
        threadStoreLockOwner = TrapAllRuntimeThreads(pRuntimeThread->GetDomain(), fHoldingThreadstoreLock);

        // This prevents a race condition where we blocked on the Lock()
        // above while another thread was sending an event and while we
        // were blocked the debugger suspended us and so we wouldn't be
        // resumed after the suspension about to happen below.
        pRuntimeThread->ResetThreadStateNC(Thread::TSNC_DebuggerUserSuspend);
    } 
    else 
    {
        LOG((LF_CORDB,LL_INFO1000, "D::DT: Skipping SendIPCEvent because RS detached."));
    }
    
    // Let other Runtime threads handle their events.
    UnlockFromEventSending();
    
    BlockAndReleaseTSLIfNecessary(threadStoreLockOwner);
    
    g_pEEInterface->DisablePreemptiveGC();

    if (!disabled)
        g_pEEInterface->EnablePreemptiveGC();
}


//
// SuspendComplete is called when the last Runtime thread reaches a safe point in response to having its trap flags set.
//
BOOL Debugger::SuspendComplete(BOOL fHoldingThreadstoreLock)
{
    _ASSERTE((!g_pEEInterface->GetThread() || !g_pEEInterface->GetThread()->m_fPreemptiveGCDisabled) || g_fInControlC);

    LOG((LF_CORDB, LL_INFO10000, "D::SC: suspension complete\n"));

    // Prevent other Runtime threads from handling events.
    LockForEventSending();

    // We're stopped now...
    _ASSERTE(!m_stopped && m_trappingRuntimeThreads);

    // We have to grab the thread store lock now so this thread can hold it during this stopping.
    if (!fHoldingThreadstoreLock)
        ThreadStore::LockThreadStore(GCHeap::SUSPEND_FOR_DEBUGGER, FALSE);
    
    // Send the sync complete event to the Right Side.
    SendSyncCompleteIPCEvent(); // sets m_stopped = true...

    // Unlock the debugger mutex. This will let the RCThread handle
    // requests from the Right Side. But we do _not_ re-enable the
    // handling of events. Runtime threads that were not counted in
    // the suspend count (because they were outside the Runtime when
    // the suspension started) may actually be trying to handle their
    // own Runtime events, say, trying to hit a breakpoint. By not
    // re-enabling event handling, we prevent those threads from
    // sending their events to the Right Side and effectivley queue
    // them up.
    //
    // Event handling is re-enabled by the RCThread in response to a
    // continue message from the Right Side.
    Unlock();

    // We set the syncThreadIsLockFree event here. This thread just sent up the sync complete flare, and we've released
    // the debuger lock. By setting this event, we allow the Right Side to suspend this thread now. (Note: this is all
    // for Win32 debugging support.)
    if (m_pRCThread->GetDCB(IPC_TARGET_OUTOFPROC)->m_rightSideIsWin32Debugger)
        SetEvent(m_pRCThread->GetDCB(IPC_TARGET_OUTOFPROC)->m_syncThreadIsLockFree);

    // Any thread that has locked for event sending can't be interrupted by breakpoints or exceptions when were interop
    // debugging. SetDebugCantStop helps us remember that. This was set in LockForEventSending.
    if (g_pEEInterface->GetThread())
        g_pEEInterface->GetThread()->SetDebugCantStop(false);
    
    
    // We unconditionally grab the thread store lock, so return that we're holding it.
    return TRUE;
}

ULONG inline Debugger::IsDebuggerAttachedToAppDomain(Thread *pThread)
{
    _ASSERTE(pThread != NULL);
    
    AppDomain *pAppDomain = pThread->GetDomain();
    
    if (pAppDomain != NULL)
        return pAppDomain->IsDebuggerAttached();
    else
    {
        _ASSERTE (g_fEEShutDown);
        return 0;
    }
}


//
// SendCreateAppDomainEvent is called when a new AppDomain gets created.
//
void Debugger::SendCreateAppDomainEvent(AppDomain* pRuntimeAppDomain,
                                        BOOL fAttaching)
{
    if (CORDBUnrecoverableError(this))
        return;
    
    LOG((LF_CORDB, LL_INFO100, "D::SCADE: Create AppDomain 0x%08x (0x%08x) (Attaching: %s).\n",
        pRuntimeAppDomain, pRuntimeAppDomain->GetId(),
        fAttaching?"TRUE":"FALSE"));

    bool disabled = false;

    //
    // If we're attaching, then we only need to send the event. We
    // don't need to disable event handling or lock the debugger
    // object.
    //
    if (!fAttaching)
    {
        disabled =  g_pEEInterface->IsPreemptiveGCDisabled();

        if (disabled)
            g_pEEInterface->EnablePreemptiveGC();

        // Prevent other Runtime threads from handling events.
        LockForEventSending();
    }

    // We may have detached while waiting in LockForEventSending, 
    // in which case we can't send the event. 
    // Note that CORDebuggerAttached() wont return true until we're finished
    // the attach, but if fAttaching, then there's a debugger listening
    if (CORDebuggerAttached() || fAttaching)
    {
        // Send a create appdomain event to the Right Side.
        DebuggerIPCEvent* ipce = m_pRCThread->GetIPCEventSendBuffer(
            IPC_TARGET_OUTOFPROC);
        InitIPCEvent(ipce, 
                     DB_IPCE_CREATE_APP_DOMAIN,
                     GetCurrentThreadId(),
                     (void *) pRuntimeAppDomain);

        ipce->AppDomainData.id = pRuntimeAppDomain->GetId();
        WCHAR *pszName = (WCHAR *)pRuntimeAppDomain->GetFriendlyName();
        if (pszName != NULL)
            wcscpy ((WCHAR *)ipce->AppDomainData.rcName, pszName);
        else
            wcscpy ((WCHAR *)ipce->AppDomainData.rcName, L"<UnknownName>");

        m_pRCThread->SendIPCEvent(IPC_TARGET_OUTOFPROC);
    } 
    else 
    {
        LOG((LF_CORDB,LL_INFO1000, "D::SCADE: Skipping SendIPCEvent because RS detached."));
    }
    
    if (!fAttaching)
    {
        // Stop all Runtime threads if we actually sent an event
        BOOL threadStoreLockOwner = FALSE;
        if (CORDebuggerAttached())
        {
            threadStoreLockOwner = TrapAllRuntimeThreads(pRuntimeAppDomain);
        }

        // Let other Runtime threads handle their events.
        UnlockFromEventSending();

        BlockAndReleaseTSLIfNecessary(threadStoreLockOwner);
    
        g_pEEInterface->DisablePreemptiveGC();

        if (!disabled)
            g_pEEInterface->EnablePreemptiveGC();
    }
}


//
// SendExitAppDomainEvent is called when an app domain is destroyed.
//
void Debugger::SendExitAppDomainEvent(AppDomain* pRuntimeAppDomain)
{
    if (CORDBUnrecoverableError(this))
        return;
    
    LOG((LF_CORDB, LL_INFO100, "D::EAD: Exit AppDomain 0x%08x.\n",
        pRuntimeAppDomain));

    bool disabled = true;
    if (GetThread() != NULL)
    {
        disabled =  g_pEEInterface->IsPreemptiveGCDisabled();

        if (disabled)
            g_pEEInterface->EnablePreemptiveGC();
    }
    
    // Prevent other Runtime threads from handling events.
    BOOL threadStoreLockOwner = FALSE;
    
    LockForEventSending();
    
    if (CORDebuggerAttached())   
    {
        
        // Send the exit appdomain event to the Right Side.
        DebuggerIPCEvent* ipce = m_pRCThread->GetIPCEventSendBuffer(IPC_TARGET_OUTOFPROC);
        InitIPCEvent(ipce, 
                     DB_IPCE_EXIT_APP_DOMAIN,
                     GetCurrentThreadId(),
                     (void *) pRuntimeAppDomain);
        m_pRCThread->SendIPCEvent(IPC_TARGET_OUTOFPROC);
        
        // Delete any left over modules for this appdomain.
        if (m_pModules != NULL)
            m_pModules->RemoveModules(pRuntimeAppDomain);
        
        // Stop all Runtime threads
        threadStoreLockOwner = TrapAllRuntimeThreads(pRuntimeAppDomain);
    } 
    else 
    {
        LOG((LF_CORDB,LL_INFO1000, "D::EAD: Skipping SendIPCEvent because RS detached."));
    }
    
    // Let other Runtime threads handle their events.
    UnlockFromEventSending();
    
    BlockAndReleaseTSLIfNecessary(threadStoreLockOwner);

    if (GetThread() != NULL)
    {
        g_pEEInterface->DisablePreemptiveGC();

        if (!disabled)
            g_pEEInterface->EnablePreemptiveGC();
    }
}


//
// LoadAssembly is called when a new Assembly gets loaded.
//
void Debugger::LoadAssembly(AppDomain* pRuntimeAppDomain, 
                            Assembly *pAssembly,
                            BOOL fIsSystemAssembly,
                            BOOL fAttaching)
{
    if (CORDBUnrecoverableError(this))
        return;
        
    LOG((LF_CORDB, LL_INFO100, "D::LA: Load Assembly Asy:%#08x AD:%#08x %s\n", 
        pAssembly, pRuntimeAppDomain, (pAssembly->GetName()?pAssembly->GetName():"Unknown name") ));

    bool disabled = false;
    
    //
    // If we're attaching, then we only need to send the event. We
    // don't need to disable event handling or lock the debugger
    // object.
    //
    if (!fAttaching)
    {
        disabled =  g_pEEInterface->IsPreemptiveGCDisabled();

        if (disabled)
            g_pEEInterface->EnablePreemptiveGC();

        // Prevent other Runtime threads from handling events.
        LockForEventSending();
    }

    if (CORDebuggerAttached() || fAttaching)
    {
        // Send a load assembly event to the Right Side.
        DebuggerIPCEvent* ipce = m_pRCThread->GetIPCEventSendBuffer(IPC_TARGET_OUTOFPROC);
        InitIPCEvent(ipce, 
                     DB_IPCE_LOAD_ASSEMBLY,
                     GetCurrentThreadId(),
                     (void *) pRuntimeAppDomain);

        ipce->AssemblyData.debuggerAssemblyToken = (void *) pAssembly;
        ipce->AssemblyData.fIsSystemAssembly =  fIsSystemAssembly;

        // Use the filename from the module that holds the assembly so
        // that we have the full path to the assembly and not just some
        // half-ass simple name.
        wcscpy ((WCHAR *)ipce->AssemblyData.rcName,
                pAssembly->GetSecurityModule()->GetFileName());

        m_pRCThread->SendIPCEvent(IPC_TARGET_OUTOFPROC);
    } 
    else 
    {
        LOG((LF_CORDB,LL_INFO1000, "D::LA: Skipping SendIPCEvent because RS detached."));
    }
    
    if (!fAttaching)
    {
        // Stop all Runtime threads
        BOOL threadStoreLockOwner = FALSE;
        
        if (CORDebuggerAttached())
        {
            threadStoreLockOwner = TrapAllRuntimeThreads(pRuntimeAppDomain);
        }

        // Let other Runtime threads handle their events.
        UnlockFromEventSending();

        BlockAndReleaseTSLIfNecessary(threadStoreLockOwner);
    
        g_pEEInterface->DisablePreemptiveGC();

        if (!disabled)
            g_pEEInterface->EnablePreemptiveGC();
    }
}


//
// UnloadAssembly is called when a Runtime thread unloads an assembly.
//
// !!WARNING: The assembly object has already been deleted before this 
// method is called. So do not call any methods on the pAssembly object!!
void Debugger::UnloadAssembly(AppDomain *pAppDomain, 
                              Assembly* pAssembly)
{
    _ASSERTE(pAppDomain->IsDebuggerAttached());
    
    if (CORDBUnrecoverableError(this))
        return;
    
    LOG((LF_CORDB, LL_INFO100, "D::UA: Unload Assembly Asy:%#08x AD:%#08x which:0x%x %s\n", 
         pAssembly, pAppDomain, (pAssembly->GetName()?pAssembly->GetName():"Unknown name") ));
        
    bool disabled = true;
    if (GetThread() != NULL)
    {
        disabled =  g_pEEInterface->IsPreemptiveGCDisabled();

        if (disabled)
            g_pEEInterface->EnablePreemptiveGC();
    }
    
    // Prevent other Runtime threads from handling events.
    BOOL threadStoreLockOwner = FALSE;
    LockForEventSending();
    
    if (CORDebuggerAttached())   
    {
        // Send the unload assembly event to the Right Side.
        DebuggerIPCEvent* ipce = m_pRCThread->GetIPCEventSendBuffer(IPC_TARGET_OUTOFPROC);
       
        InitIPCEvent(ipce, 
                     DB_IPCE_UNLOAD_ASSEMBLY,
                     GetCurrentThreadId(),
                     (void *) pAppDomain);
        ipce->AssemblyData.debuggerAssemblyToken = (void *) pAssembly;

        m_pRCThread->SendIPCEvent(IPC_TARGET_OUTOFPROC);
        
        // Stop all Runtime threads
        threadStoreLockOwner = TrapAllRuntimeThreads(pAppDomain);

    }
    else
    {
        LOG((LF_CORDB,LL_INFO1000, "D::UA: Skipping SendIPCEvent because RS detached."));
    }
    
    // Let other Runtime threads handle their events.
    UnlockFromEventSending();
    
    BlockAndReleaseTSLIfNecessary(threadStoreLockOwner);

    if (GetThread() != NULL)
    {
        g_pEEInterface->DisablePreemptiveGC();

        if (!disabled)
            g_pEEInterface->EnablePreemptiveGC();
    }
}

// Create a new module
DebuggerModule* Debugger::AddDebuggerModule(Module* pRuntimeModule,
                                    AppDomain *pAppDomain)
{
    DebuggerModule*  module = new (interopsafe) DebuggerModule(pRuntimeModule, pAppDomain);
    TRACE_ALLOC(module);

    _ASSERTE (module != NULL);

    if (FAILED(CheckInitModuleTable()))
        return (NULL);

    m_pModules->AddModule(module);

    return module;
}

// Return an existing module
DebuggerModule* Debugger::GetDebuggerModule(Module* pRuntimeModule,
                                    AppDomain *pAppDomain)
{
    if (FAILED(CheckInitModuleTable()))
        return (NULL);

    return m_pModules->GetModule(pRuntimeModule, pAppDomain);
}

//
// LoadModule is called when a Runtime thread loads a new module.
//
void Debugger::LoadModule(Module* pRuntimeModule,
                          IMAGE_COR20_HEADER* pCORHeader,
                          VOID* baseAddress,
                          LPCWSTR pszModuleName,
                          DWORD dwModuleName,
                          Assembly *pAssembly,
                          AppDomain *pAppDomain,
                          BOOL fAttaching,
                          CorLoadFlags LoadFlags)
{
    if (CORDBUnrecoverableError(this))
        return;
    
    BOOL disabled = FALSE;
    BOOL threadStoreLockOwner = FALSE;
    
    //
    // If we're attaching, then we only need to send the event. We
    // don't need to disable event handling or lock the debugger
    // object.
    //
    if (!fAttaching)
    {
        disabled =  g_pEEInterface->IsPreemptiveGCDisabled();

        if (disabled)
            g_pEEInterface->EnablePreemptiveGC();

        // Prevent other Runtime threads from handling events.
        LockForEventSending();
    }

    // We don't actually want to get the symbol reader, we just 
    // want to make sure that the .pdbs have been copied into 
    // the fusion cache.
    // CAVEAT: If the symbols were updated (eg, recompiled) after the process
    // began using the module, the debugger will get a symbol mismatch error.
    // A fix is to run the program in debug mode.
    pRuntimeModule->GetISymUnmanagedReader();

    DebuggerModule *module = GetDebuggerModule(pRuntimeModule,pAppDomain);
    DebuggerIPCEvent* ipce = NULL;
    DWORD length = 0;

    // Don't create new record if already loaded.
    if (module)
    {
        LOG((LF_CORDB, LL_INFO100, "D::LM: module already loaded Mod:%#08x "
            "Asy:%#08x AD:%#08x isDynamic:0x%x runtimeMod:%#08x ModName:%ls\n",
             module, pAssembly, pAppDomain, pRuntimeModule->IsReflection(), pRuntimeModule, pszModuleName));
        goto LExit;
    }
    module = AddDebuggerModule(pRuntimeModule, pAppDomain);

    LOG((LF_CORDB, LL_INFO100, "D::LM: load module Mod:%#08x "
        "Asy:%#08x AD:%#08x isDynamic:0x%x runtimeMod:%#08x ModName:%ls\n",
         module, pAssembly, pAppDomain, pRuntimeModule->IsReflection(), pRuntimeModule, pszModuleName));
    
    // Send a load module event to the Right Side.
    ipce = m_pRCThread->GetIPCEventSendBuffer(IPC_TARGET_OUTOFPROC);
    InitIPCEvent(ipce,DB_IPCE_LOAD_MODULE, GetCurrentThreadId(), (void *) pAppDomain);
    ipce->LoadModuleData.debuggerModuleToken = (void*) module;
    ipce->LoadModuleData.debuggerAssemblyToken = (void *) pAssembly;
    ipce->LoadModuleData.pPEBaseAddress = (void *) baseAddress;

    if (pRuntimeModule->IsPEFile())
    {
        // Get the PEFile structure.
        PEFile *pPEFile = pRuntimeModule->GetPEFile();

        _ASSERTE(pPEFile->GetNTHeader() != NULL);
        _ASSERTE(pPEFile->GetNTHeader()->OptionalHeader.SizeOfImage != 0);

        ipce->LoadModuleData.nPESize = VAL32(pPEFile->GetNTHeader()->OptionalHeader.SizeOfImage);
    }
    else
        ipce->LoadModuleData.nPESize = 0;

    if ((pszModuleName == NULL) || (*pszModuleName == L'\0'))
        ipce->LoadModuleData.fInMemory = TRUE;
    else
        ipce->LoadModuleData.fInMemory = FALSE;
        
    ipce->LoadModuleData.fIsDynamic = pRuntimeModule->IsReflection();

    if (!ipce->LoadModuleData.fIsDynamic)
    {
        if (LoadFlags == CorLoadUndefinedMap || 
            LoadFlags == CorLoadDataMap || 
            LoadFlags == CorLoadOSImage ||
            LoadFlags == CorLoadOSMap) 
        {
            IMAGE_NT_HEADERS    *pNT;
            IMAGE_COR20_HEADER  *pCOR;
            IMAGE_DOS_HEADER    *pDos;

            HRESULT hr = CorMap::ReadHeaders((PBYTE)baseAddress, &pDos, &pNT, &pCOR, true, 0);
            if (!FAILED(hr))
            {
                ipce->LoadModuleData.pMetadataStart = 
                    (LPVOID)Cor_RtlImageRvaToVa(pNT, (PBYTE)baseAddress, VAL32(pCORHeader->MetaData.VirtualAddress), 0);

                ipce->LoadModuleData.nMetadataSize = VAL32(pCORHeader->MetaData.Size);
            }
            else
            {
                ipce->LoadModuleData.pMetadataStart = 0; //get this
                ipce->LoadModuleData.nMetadataSize = 0;
            }
        }
        else
        {
            ipce->LoadModuleData.pMetadataStart =
                (LPVOID)((size_t)VAL32(pCORHeader->MetaData.VirtualAddress) + (size_t)baseAddress);
            ipce->LoadModuleData.nMetadataSize = VAL32(pCORHeader->MetaData.Size);
        }
    }
    else
    {
        BYTE *rgb;
        DWORD cb;
        HRESULT hr = ModuleMetaDataToMemory( pRuntimeModule, &rgb, &cb);
        if (!FAILED(hr))
        {
            ipce->LoadModuleData.pMetadataStart = rgb; //get this
            ipce->LoadModuleData.nMetadataSize = cb;
        }
        else
        {
            ipce->LoadModuleData.pMetadataStart = 0; //get this
            ipce->LoadModuleData.nMetadataSize = 0;
        }
        LOG((LF_CORDB,LL_INFO10000, "D::LM: putting dynamic, new mD at 0x%x, "
            "size 0x%x\n",ipce->LoadModuleData.pMetadataStart,
            ipce->LoadModuleData.nMetadataSize));

        // Dynamic modules must receive ClassLoad callbacks in order to receive metadata updates as the module
        // evolves. So we force this on here and refuse to change it for all dynamic modules.
        module->EnableClassLoadCallbacks(TRUE);
    }

    LOG((LF_CORDB, LL_INFO100000, "D::LM: Size 0x%x, pMetadataStart 0x%x\n", ipce->LoadModuleData.nMetadataSize, ipce->LoadModuleData.pMetadataStart));

    // Never give an empty module name...
    const WCHAR *moduleName;

    if (dwModuleName > 0)
        moduleName = pszModuleName;
    else
    {
        if (pRuntimeModule->IsReflection())
        {
            ReflectionModule *rm = pRuntimeModule->GetReflectionModule();
            moduleName = rm->GetFileName();

            if (moduleName)
            {
                dwModuleName = wcslen(moduleName);
            }
            else
            {
                moduleName = L"<Unknown or dynamic module>";
                dwModuleName = wcslen(moduleName);
            }
        }
        else
        {
            moduleName = L"<Unknown or dynamic module>";
            dwModuleName = wcslen(moduleName);
        }
    }
    
    length = dwModuleName < MAX_PATH ? dwModuleName : MAX_PATH;
    memcpy(ipce->LoadModuleData.rcName, moduleName, length*sizeof(WCHAR));
    ipce->LoadModuleData.rcName[length] = L'\0';

    m_pRCThread->SendIPCEvent(IPC_TARGET_OUTOFPROC);

    if (fAttaching)
    {
        UpdateModuleSyms(pRuntimeModule,
                         pAppDomain,
                         fAttaching);
    }
    else // !fAttaching
    {
        // Stop all Runtime threads
        threadStoreLockOwner = TrapAllRuntimeThreads(pAppDomain);
    }

LExit:
    if (!fAttaching)
    {
        // Let other Runtime threads handle their events.
        UnlockFromEventSending();

        BlockAndReleaseTSLIfNecessary(threadStoreLockOwner);
    
        g_pEEInterface->DisablePreemptiveGC();

        if (!disabled)
            g_pEEInterface->EnablePreemptiveGC();
    }
}

//
// UpdateModuleSyms is called when the symbols for a module need to be
// sent to the Right Side because they've changed.
//
void Debugger::UpdateModuleSyms(Module* pRuntimeModule,
                                AppDomain *pAppDomain,
                                BOOL fAttaching)
{
    DebuggerIPCEvent* ipce = NULL;

    if (CORDBUnrecoverableError(this))
        return;
    
    CGrowableStream *pStream = pRuntimeModule->GetInMemorySymbolStream();

    LOG((LF_CORDB, LL_INFO10000, "D::UMS: update module syms "
         "RuntimeModule:0x%08x CGrowableStream:0x%08x\n",
         pRuntimeModule, pStream));
         
    DebuggerModule* module = LookupModule(pRuntimeModule, pAppDomain);
    _ASSERTE(module != NULL);

   if (pStream == NULL || module->GetHasLoadedSymbols())
    {
        // No symbols to update (eg, symbols are on-disk),
        // or the symbols have already been sent.
        LOG((LF_CORDB, LL_INFO10000, "D::UMS: no in-memory symbols, or "
            "symbols already loaded!\n"));
        return;
    }
    
    STATSTG SizeData = {0};
    DWORD streamSize = 0;
    HRESULT hr = S_OK;
    bool disabled = false;

    hr = pStream->Stat(&SizeData, STATFLAG_NONAME);
    
    streamSize = SizeData.cbSize.u.LowPart;
    if (FAILED(hr))
    {
        goto LExit;
    }

    if (SizeData.cbSize.u.HighPart > 0)
    {
        hr = E_OUTOFMEMORY;
        goto LExit;
    }

    if (!fAttaching && GetThread() != NULL)
    {
        disabled =  g_pEEInterface->IsPreemptiveGCDisabled();

        if (disabled)
            g_pEEInterface->EnablePreemptiveGC();
    }

    if (!fAttaching)
    {
        // Prevent other Runtime threads from handling events.
        LockForEventSending();
    }
    
    if (CORDebuggerAttached() || fAttaching)
    {
        // Send a update module syns event to the Right Side.
        ipce = m_pRCThread->GetIPCEventSendBuffer(IPC_TARGET_OUTOFPROC);
        InitIPCEvent(ipce, DB_IPCE_UPDATE_MODULE_SYMS,
                     GetCurrentThreadId(),
                     (void *) pAppDomain);
        ipce->UpdateModuleSymsData.debuggerModuleToken = (void*) module;
        ipce->UpdateModuleSymsData.debuggerAppDomainToken = (void *) pAppDomain;
        ipce->UpdateModuleSymsData.pbSyms = (BYTE *)pStream->GetBuffer();
        ipce->UpdateModuleSymsData.cbSyms = streamSize;
        ipce->UpdateModuleSymsData.needToFreeMemory = false;
        
        m_pRCThread->SendIPCEvent(IPC_TARGET_OUTOFPROC);
    } 
    else 
    {
        LOG((LF_CORDB,LL_INFO1000, "D::UMS: Skipping SendIPCEvent because RS detached."));
    }
    
    // We used to set HasLoadedSymbols here, but we don't really want
    // to do that in the face of the same module being in multiple app
    // domains.

    if(!fAttaching)
    {
        // Stop all Runtime threads if we sent a message
        BOOL threadStoreLockOwner = FALSE;
        
        if (CORDebuggerAttached())
        {
            threadStoreLockOwner = TrapAllRuntimeThreads(pAppDomain);
        }

        // Let other Runtime threads handle their events.
        UnlockFromEventSending();

        BlockAndReleaseTSLIfNecessary(threadStoreLockOwner);

        if (GetThread() != NULL)
        {
            g_pEEInterface->DisablePreemptiveGC();
        
            if (!disabled)
                g_pEEInterface->EnablePreemptiveGC();
        }
    }
LExit:
    ; // Debugger must free buffer using RELEASE_BUFFER message!
}

/******************************************************************************
 *
 ******************************************************************************/
HRESULT Debugger::ModuleMetaDataToMemory(Module *pMod, BYTE **prgb, DWORD *pcb)
{
    IMetaDataEmit *pIMDE = pMod->GetEmitter();
    HRESULT hr;

    hr = pIMDE->GetSaveSize(cssQuick, pcb);
    if (FAILED(hr))
    {
        *pcb = 0;
        return hr;
    }
    
    (*prgb) = new (interopsafe) BYTE[*pcb];
    if (NULL == (*prgb))
    {
        *pcb = 0;
        return E_OUTOFMEMORY;
    }

    hr = pIMDE->SaveToMemory((*prgb), *pcb);
    if (FAILED(hr))
    {
        *pcb = 0;
        return hr;
    }

    pIMDE = NULL; // note that the emiiter SHOULD NOT be released

    LOG((LF_CORDB,LL_INFO1000, "D::MMDTM: Saved module 0x%x MD to 0x%x "
        "(size:0x%x)\n", pMod, *prgb, *pcb));

    return S_OK;
}

//
// UnloadModule is called when a Runtime thread unloads a module.
//
void Debugger::UnloadModule(Module* pRuntimeModule, 
                            AppDomain *pAppDomain)
{
    if (CORDBUnrecoverableError(this))
        return;
    
    _ASSERTE(pAppDomain->IsDebuggerAttached());

    LOG((LF_CORDB, LL_INFO100, "D::UM: unload module Mod:%#08x AD:%#08x runtimeMod:%#08x modName:%ls\n", 
         LookupModule(pRuntimeModule, pAppDomain), pAppDomain, pRuntimeModule, pRuntimeModule->GetFileName()));
        
    BOOL disabled =true;
    BOOL threadStoreLockOwner = FALSE;
    
    if (GetThread() != NULL)
    {
        disabled =  g_pEEInterface->IsPreemptiveGCDisabled();

        if (disabled)
            g_pEEInterface->EnablePreemptiveGC();
    }
    
    // Prevent other Runtime threads from handling events.
    LockForEventSending();
    
    if (CORDebuggerAttached())   
    {
        
        DebuggerModule* module = LookupModule(pRuntimeModule, pAppDomain);
        if (module == NULL)
        {
            LOG((LF_CORDB, LL_INFO100, "D::UM: module already unloaded AD:%#08x runtimeMod:%#08x modName:%ls\n", 
                 pAppDomain, pRuntimeModule, pRuntimeModule->GetFileName()));
            goto LExit;
        }
        _ASSERTE(module != NULL);

        // Note: the appdomain the module was loaded in must match the appdomain we're unloading it from. If it doesn't,
        // then we've either found the wrong DebuggerModule in LookupModule or we were passed bad data.
        _ASSERTE(!module->m_fDeleted);
        _ASSERTE(module->m_pAppDomain == pAppDomain);

        // Send the unload module event to the Right Side.
        DebuggerIPCEvent* ipce = m_pRCThread->GetIPCEventSendBuffer(IPC_TARGET_OUTOFPROC);
        InitIPCEvent(ipce, DB_IPCE_UNLOAD_MODULE, GetCurrentThreadId(), (void*) pAppDomain);
        ipce->UnloadModuleData.debuggerModuleToken = (void*) module;
        ipce->UnloadModuleData.debuggerAssemblyToken = (void*) pRuntimeModule->GetClassLoader()->GetAssembly();
        m_pRCThread->SendIPCEvent(IPC_TARGET_OUTOFPROC);

        // Delete the Left Side representation of the module.
        if (m_pModules != NULL)
            m_pModules->RemoveModule(pRuntimeModule, pAppDomain);
        
        // Stop all Runtime threads
        threadStoreLockOwner = TrapAllRuntimeThreads(pAppDomain);
    } 
    else 
    {
        LOG((LF_CORDB,LL_INFO1000, "D::UM: Skipping SendIPCEvent because RS detached."));
    }
    
LExit:
    // Let other Runtime threads handle their events.
    UnlockFromEventSending();
    
    BlockAndReleaseTSLIfNecessary(threadStoreLockOwner);

    if (GetThread() != NULL)
    {
        g_pEEInterface->DisablePreemptiveGC();

        if (!disabled)
            g_pEEInterface->EnablePreemptiveGC();
    }
}

void Debugger::DestructModule(Module *pModule)
{
    // We want to remove all references to the module from the various 
    // tables.  It's not just possible, but probable, that the module
    // will be re-loaded at the exact same address, and in that case,
    // we'll have piles of entries in our DJI table that mistakenly
    // match this new module.
    // Note that this doesn't apply to shared assemblies, that only
    // get unloaded when the process dies.  We won't be reclaiming their
    // DJIs/patches b/c the process is going to die, so we'll reclaim
    // the memory when the various hashtables are unloaded.  
    
    if (DebuggerController::g_patches != NULL)
    {
        // Note that we'll explicitly NOT delete DebuggerControllers, so that
        // the Right Side can delete them later.
        Lock();
        DebuggerController::g_patches->ClearPatchesFromModule(pModule);
        Unlock();
    }

    
    if (m_pJitInfos != NULL)
    {
        LockJITInfoMutex();
        m_pJitInfos->ClearMethodsOfModule(pModule);
        UnlockJITInfoMutex();
    }
}

/******************************************************************************
 *
 ******************************************************************************/
void Debugger::SendClassLoadUnloadEvent (mdTypeDef classMetadataToken,
                                         DebuggerModule *classModule,
                                         Assembly *pAssembly,
                                         AppDomain *pAppDomain,
                                         BOOL fIsLoadEvent)
{
    LOG((LF_CORDB,LL_INFO10000, "D::SCLUE: Tok:0x%x isLoad:0x%x Mod:%#08x AD:%#08x %ls\n",
        classMetadataToken, fIsLoadEvent, classModule, pAppDomain, pAppDomain->GetFriendlyName(FALSE)));

    DebuggerIPCEvent* ipce = m_pRCThread->GetIPCEventSendBuffer(IPC_TARGET_OUTOFPROC);
    if (fIsLoadEvent == TRUE)
    {
        BOOL isReflection = classModule->m_pRuntimeModule->IsReflection();

        // If this is a reflection module, send the message to update
        // the module symbols before sending the class load event.
        if (isReflection)
            // We're not actually attaching, but it's behaviourly identical
            UpdateModuleSyms(classModule->m_pRuntimeModule, pAppDomain, TRUE);
        
        InitIPCEvent(ipce, 
                     DB_IPCE_LOAD_CLASS,
                     GetCurrentThreadId(),
                     (void*) pAppDomain);
        ipce->LoadClass.classMetadataToken = classMetadataToken;
        ipce->LoadClass.classDebuggerModuleToken = (void*) classModule;
        ipce->LoadClass.classDebuggerAssemblyToken =
                            (void*) pAssembly;

        if (isReflection)
        {
            HRESULT hr;
            hr = ModuleMetaDataToMemory(classModule->m_pRuntimeModule,
                &(ipce->LoadClass.pNewMetaData),
                &(ipce->LoadClass.cbNewMetaData));
            _ASSERTE(!FAILED(hr));
        }
    }
    else
    {
        InitIPCEvent(ipce, 
                     DB_IPCE_UNLOAD_CLASS,
                     GetCurrentThreadId(),
                     (void*) pAppDomain);
        ipce->UnloadClass.classMetadataToken = classMetadataToken;
        ipce->UnloadClass.classDebuggerModuleToken = (void*) classModule;
        ipce->UnloadClass.classDebuggerAssemblyToken =
                            (void*) pAssembly;
    }

    m_pRCThread->SendIPCEvent(IPC_TARGET_OUTOFPROC);
}



/******************************************************************************
 *
 ******************************************************************************/
BOOL Debugger::SendSystemClassLoadUnloadEvent(mdTypeDef classMetadataToken,
                                              Module *classModule,
                                              BOOL fIsLoadEvent)
{
    BOOL fRetVal = FALSE;
    Assembly *pAssembly = classModule->GetAssembly();

    if (!m_pAppDomainCB->Lock())
        return (FALSE);

    AppDomainInfo *pADInfo = m_pAppDomainCB->FindFirst();

    while (pADInfo != NULL)
    {
        AppDomain *pAppDomain = pADInfo->m_pAppDomain;
        _ASSERTE(pAppDomain != NULL);

        if ((pAppDomain->IsDebuggerAttached() || (pAppDomain->GetDebuggerAttached() & AppDomain::DEBUGGER_ATTACHING_THREAD)) &&
            (pAppDomain->ContainsAssembly(pAssembly) || pAssembly->IsSystem()) &&
			!(fIsLoadEvent && pAppDomain->IsUnloading()) )
        {
            // Find the Left Side module that this class belongs in.
            DebuggerModule* pModule = LookupModule(classModule, pAppDomain);
            //_ASSERTE(pModule != NULL);
                
            // Only send a class load event if they're enabled for this module.
            if (pModule && pModule->ClassLoadCallbacksEnabled())
            {
                SendClassLoadUnloadEvent(classMetadataToken,
                                         pModule,
                                         pAssembly,
                                         pAppDomain,
                                         fIsLoadEvent);
                fRetVal = TRUE;
            }
        }

        pADInfo = m_pAppDomainCB->FindNext(pADInfo);
    } 

    m_pAppDomainCB->Unlock();

    return fRetVal;
}


//
// LoadClass is called when a Runtime thread loads a new Class.
// Returns TRUE if an event is sent, FALSE otherwise
BOOL  Debugger::LoadClass(EEClass   *pRuntimeClass,
                          mdTypeDef  classMetadataToken,
                          Module    *classModule,
                          AppDomain *pAppDomain,
                          BOOL fSendEventToAllAppDomains,
                          BOOL fAttaching)
{
    BOOL fRetVal = FALSE;
    BOOL threadStoreLockOwner = FALSE;
    
    if (CORDBUnrecoverableError(this))
        return FALSE;

    LOG((LF_CORDB, LL_INFO10000, "D::LC: load class Tok:%#08x Mod:%#08x AD:%#08x classMod:%#08x modName:%ls\n", 
         classMetadataToken, LookupModule(classModule, pAppDomain), pAppDomain, classModule, classModule->GetFileName()));

    //
    // If we're attaching, then we only need to send the event. We
    // don't need to disable event handling or lock the debugger
    // object.
    //
    bool disabled = false;
    
    if (!fAttaching)
    {
        // Enable preemptive GC...
        disabled = g_pEEInterface->IsPreemptiveGCDisabled();

        if (disabled)
            g_pEEInterface->EnablePreemptiveGC();

        // Prevent other Runtime threads from handling events.
        LockForEventSending();
    }

    if (CORDebuggerAttached() || fAttaching) 
    {
        fRetVal = SendSystemClassLoadUnloadEvent(classMetadataToken, classModule, TRUE);
               
        if (fRetVal == TRUE)
        {    
            // Stop all Runtime threads
            threadStoreLockOwner = TrapAllRuntimeThreads(pAppDomain);
        }
    }
    else 
    {
        LOG((LF_CORDB,LL_INFO1000, "D::LC: Skipping SendIPCEvent because RS detached."));
    }
    
    if (!fAttaching)
    {
        // Let other Runtime threads handle their events.
        UnlockFromEventSending();
        
        BlockAndReleaseTSLIfNecessary(threadStoreLockOwner);
    
        g_pEEInterface->DisablePreemptiveGC();
        
        if (!disabled)
            g_pEEInterface->EnablePreemptiveGC();
    }

    return fRetVal;
}


//
// UnloadClass is called when a Runtime thread unloads a Class.
//
void Debugger::UnloadClass(mdTypeDef classMetadataToken,
                           Module *classModule,
                           AppDomain *pAppDomain,
                           BOOL fSendEventToAllAppDomains)
{
    BOOL fRetVal = FALSE;
        
    if (CORDBUnrecoverableError(this))
        return;
    
    LOG((LF_CORDB, LL_INFO10000, "D::UC: unload class Tok:0x%08x Mod:%#08x AD:%#08x runtimeMod:%#08x modName:%ls\n", 
         classMetadataToken, LookupModule(classModule, pAppDomain), pAppDomain, classModule, classModule->GetFileName()));

    bool toggleGC = false;
    if (GetThread() != NULL)
    {
        toggleGC =  g_pEEInterface->IsPreemptiveGCDisabled();

        if (toggleGC)
            g_pEEInterface->EnablePreemptiveGC();
    }
    
    // Prevent other Runtime threads from handling events.
    BOOL threadStoreLockOwner = FALSE;
    
    LockForEventSending();
    if (CORDebuggerAttached())
    {
        Assembly *pAssembly = classModule->GetClassLoader()->GetAssembly();
        DebuggerModule *pModule = LookupModule(classModule, pAppDomain);
        if (pModule != NULL)
        {
        _ASSERTE(pAppDomain != NULL && pAssembly != NULL && pModule != NULL);

        SendClassLoadUnloadEvent(classMetadataToken, pModule, pAssembly, pAppDomain, FALSE);
        fRetVal = TRUE;
        }

        if (fRetVal == TRUE)
        {    
            // Stop all Runtime threads
            threadStoreLockOwner = TrapAllRuntimeThreads(pAppDomain);
        }
    }
    else 
    {
        LOG((LF_CORDB,LL_INFO1000, "D::UC: Skipping SendIPCEvent because RS detached."));
    }
    
    // Let other Runtime threads handle their events.
    UnlockFromEventSending();

    BlockAndReleaseTSLIfNecessary(threadStoreLockOwner);
    
    if (GetThread() != NULL && toggleGC)
        g_pEEInterface->DisablePreemptiveGC();
}

/******************************************************************************
 *
 ******************************************************************************/
void Debugger::FuncEvalComplete(Thread* pThread, DebuggerEval *pDE)
{
    if (CORDBUnrecoverableError(this))
        return;
    
    LOG((LF_CORDB, LL_INFO10000, "D::FEC: func eval complete pDE:%08x evalType:%d %s %s\n",
        pDE, pDE->m_evalType, pDE->m_successful ? "Success" : "Fail", pDE->m_aborted ? "Abort" : "Completed"));

    _ASSERTE(pDE->m_completed);
    _ASSERTE((g_pEEInterface->GetThread() && !g_pEEInterface->GetThread()->m_fPreemptiveGCDisabled) || g_fInControlC);

    _ASSERTE(ThreadHoldsLock());
    
    // Send a func eval complete event to the Right Side.
    DebuggerIPCEvent* ipce = m_pRCThread->GetIPCEventSendBuffer(IPC_TARGET_OUTOFPROC);
    InitIPCEvent(ipce, DB_IPCE_FUNC_EVAL_COMPLETE, pThread->GetThreadId(), pThread->GetDomain());
    ipce->FuncEvalComplete.funcEvalKey = pDE->m_funcEvalKey;
    ipce->FuncEvalComplete.successful = pDE->m_successful;
    ipce->FuncEvalComplete.aborted = pDE->m_aborted;
    ipce->FuncEvalComplete.resultAddr = &(pDE->m_result);
    ipce->FuncEvalComplete.resultType = pDE->m_resultType;

    // We must adjust the result address to point to the right place
    unsigned size = GetSizeForCorElementType(pDE->m_resultType);
    ipce->FuncEvalComplete.resultAddr =
        ArgSlotEndianessFixup((ARG_SLOT*)ipce->FuncEvalComplete.resultAddr, size);

    if (pDE->m_resultModule != NULL)
    {
        ipce->FuncEvalComplete.resultDebuggerModuleToken =
            (void*) LookupModule(pDE->m_resultModule, (AppDomain *)ipce->appDomainToken);
    }
    else
    {
        ipce->FuncEvalComplete.resultDebuggerModuleToken = NULL;
    }

    m_pRCThread->SendIPCEvent(IPC_TARGET_OUTOFPROC);
}

/* ------------------------------------------------------------------------ *
 * Right Side Interface routines
 * ------------------------------------------------------------------------ */

//
// GetFunctionInfo returns various bits of function information given
// a module and a token. The info will come from a MethodDesc, if
// one exists (and the fd will be returned) or the info will come from
// metadata.
//
HRESULT Debugger::GetFunctionInfo(Module *pModule, mdToken functionToken,
                                  MethodDesc **ppFD,
                                  ULONG *pRVA,
                                  BYTE  **pCodeStart,
                                  unsigned int *pCodeSize,
                                  mdToken *pLocalSigToken)
{
    HRESULT hr = S_OK;

    // First, lets see if we've got a MethodDesc for this function.
    MethodDesc* pFD =
        g_pEEInterface->LookupMethodDescFromToken(pModule, functionToken);

    if (pFD != NULL)
    {
        LOG((LF_CORDB, LL_INFO10000, "D::GFI: fd found.\n"));

        // If this is not IL, then this function was called in error.
        if(!pFD->IsIL())
            return(CORDBG_E_FUNCTION_NOT_IL);

        COR_ILMETHOD_DECODER header(g_pEEInterface->MethodDescGetILHeader(pFD));
        
        *ppFD = pFD;
        *pRVA = g_pEEInterface->MethodDescGetRVA(pFD);
        *pCodeStart = const_cast<BYTE*>(header.Code);
        *pCodeSize = header.GetCodeSize();
        // I don't see why COR_ILMETHOD_DECODER doesn't simply set this field to 
        // be mdSignatureNil in the absence of a local signature, but since it sets
        // LocalVarSigTok to zero, we have to set it to what we expect - mdSignatureNil.
        *pLocalSigToken = (header.GetLocalVarSigTok())?(header.GetLocalVarSigTok()):(mdSignatureNil);
    }
    else
    {
        LOG((LF_CORDB, LL_INFO10000, "D::GFI: fd not found.\n"));

        *ppFD = NULL; // no MethodDesc yet...

        DWORD implFlags;

        // Get the RVA and impl flags for this method.
        hr = g_pEEInterface->GetMethodImplProps(pModule,
                                                functionToken,
                                                pRVA,
                                                &implFlags);

        if (SUCCEEDED(hr))
        {
            // If the RVA is 0 or it's native, then the method is not IL
            if (*pRVA == 0 || IsMiNative(implFlags))
                return (CORDBG_E_FUNCTION_NOT_IL);

            // The IL Method Header is at the given RVA in this module.
            COR_ILMETHOD *ilMeth = (COR_ILMETHOD*) pModule->ResolveILRVA(*pRVA, FALSE);
            COR_ILMETHOD_DECODER header(ilMeth);

            // Snagg the IL code info.
            *pCodeStart = const_cast<BYTE*>(header.Code);
            *pCodeSize = header.GetCodeSize();

            if (header.GetLocalVarSigTok() != NULL)
                *pLocalSigToken = header.GetLocalVarSigTok();
            else
                *pLocalSigToken = mdSignatureNil;
        }
    }
    
    return hr;
}


/******************************************************************************
 *
 ******************************************************************************/
bool Debugger::ResumeThreads(AppDomain* pAppDomain)
{
    // Okay, mark that we're not stopped anymore and let the
    // Runtime threads go...
    ReleaseAllRuntimeThreads(pAppDomain);

    // If we have any thread blocking while holding the thread store lock (see BlockAndReleaseTSLIfNecessary), release
    // it now. Basically, there will be some thread holding onto the thread store lock for us if the RC Thread is not
    // holding the thread store lock.
    if (!m_RCThreadHoldsThreadStoreLock)
        SetEvent(m_runtimeStoppedEvent);
    
    // We no longer need to relax the thread store lock requirement.
    g_fRelaxTSLRequirement = false;
    
    // Re-enable event handling here. Event handling was left disabled after sending a sync complete event to the Right
    // Side. This prevents more events being sent while the process is synchronized. Re-enabling here allows any Runtime
    // threads that were queued waiting to send to actually go ahead and send.
    EnableEventHandling();

    // Return that we've continued the process.
    return true;
}

//
// HandleIPCEvent is called by the RC thread in response to an event
// from the Debugger Interface. No other IPC events, nor any Runtime
// events will come in until this method returns. Returns true if this
// was a Continue event.
//
bool Debugger::HandleIPCEvent(DebuggerIPCEvent* event, IpcTarget iWhich)
{
    bool ret = false;
    HRESULT hr = S_OK;
    
    LOG((LF_CORDB, LL_INFO10000, "D::HIPCE: got %s\n", IPCENames::GetName(event->type)));

    //
    // Lock the debugger mutex around the handling of all Right Side
    // events. This allows Right Side events to be handled safely
    // while the process is unsynchronized.
    //
    
    Lock();
    
    switch (event->type & DB_IPCE_TYPE_MASK)
    {
    case DB_IPCE_ASYNC_BREAK:
        // Simply trap all Runtime threads if we're not already trying to.
        if (!m_trappingRuntimeThreads)
        {
            m_RCThreadHoldsThreadStoreLock = TrapAllRuntimeThreads((AppDomain*)event->appDomainToken);

            // We set the syncThreadIsLockFree event here since the helper thread will never be suspended by the Right
            // Side.  (Note: this is all for Win32 debugging support.)
            if (m_pRCThread->GetDCB(IPC_TARGET_OUTOFPROC)->m_rightSideIsWin32Debugger)
                SetEvent(m_pRCThread->GetDCB(IPC_TARGET_OUTOFPROC)->m_syncThreadIsLockFree);
        }
        
        break;

    case DB_IPCE_CONTINUE:
        _ASSERTE(iWhich != IPC_TARGET_INPROC); //inproc should never go anywhere
    
        // We had better be stopped...
        _ASSERTE(m_stopped);

        // if we receive IPCE_CONTINUE and m_syncingForAttach is != SYNC_STATE_0,
        // we send loaded assembly, modules, classes and started threads, and finally 
        // another sync event. We _do_not_ release the threads in this case.

        // Here's how the attach logic works:
        // 1. Set m_syncingForAttach to SYNC_STATE_1
        // 2. Send all CreateAppDomain events to the right side
        // 3. Set m_syncingForAttach to SYNC_STATE_2
        // 4. The right side sends AttachToAppDomain events for every app domain
        //    that it wishes to attach to. Then the right side sends IPCE_CONTINUE
        // 5. Upon receiving IPCE_CONTINUE, m_syncingForAttach is SYNC_STATE_2. This
        //    indicates that we should send all the Load Assembly and Load Module 
        //    events to the right side for all the app domains to which the debugger
        //    is attaching.
        // 6. Set m_syncingForAttach to SYNC_STATE_3
        // 7. Upon receiving IPCE_CONTINUE when m_syncingForAttach is in SYNC_STATE_3,
        //    send out all the LoadClass events for all the modules which the right 
        //    side is interested in.
        // 8. Set m_syncingForAttach to SYNC_STATE_0. This indicates that the 
        //    attach has completed!!
        if (m_syncingForAttach != SYNC_STATE_0)
        {
            _ASSERTE (m_syncingForAttach != SYNC_STATE_1);
            LOG((LF_CORDB, LL_INFO10, "D::HIPCE: Got DB_IPCE_CONTINUE.  Attach state is currently %s\n", g_ppszAttachStateToSZ[m_syncingForAttach]));

            HRESULT hr;
            BOOL fAtleastOneEventSent = FALSE;

            if (m_syncingForAttach == SYNC_STATE_20)
            {
                SendEncRemapEvents(&m_EnCRemapInfo);
                
                m_syncingForAttach = SYNC_STATE_0;
                LOG((LF_CORDB, LL_INFO10, "D::HIPCE: Attach state is now %s\n", g_ppszAttachStateToSZ[m_syncingForAttach]));
                
                break; // out of "event->type & DB_IPCE_TYPE_MASK" switch
            }

        syncForAttachRetry:
            if ((m_syncingForAttach == SYNC_STATE_2) ||
                (m_syncingForAttach == SYNC_STATE_10))
            {
                    hr = IterateAppDomainsForAttach(DONT_SEND_CLASS_EVENTS,
                                                    &fAtleastOneEventSent,
                                                    TRUE);

                    // This is for the case that we're attaching at a point where
                    // only an AppDomain is loaded, so we can't send any
                    // assembly load events but it's valid and so we should just
                    // move on to SYNC_STATE_3 and retry this stuff.  This
                    // happens in particular when we are trying to use the
                    // service to do a synchronous attach at runtime load.
                    if (FAILED(hr) || !fAtleastOneEventSent)
                    {
                        m_syncingForAttach = SYNC_STATE_3;
                        LOG((LF_CORDB, LL_INFO10, "D::HIPCE: Attach state is now %s\n", g_ppszAttachStateToSZ[m_syncingForAttach]));
                        goto syncForAttachRetry;
                    }
            }
            else
            {
                _ASSERTE ((m_syncingForAttach == SYNC_STATE_3) ||
                          (m_syncingForAttach == SYNC_STATE_11));

                hr = IterateAppDomainsForAttach(ONLY_SEND_CLASS_EVENTS,
                                                &fAtleastOneEventSent,
                                                TRUE);
                
                // Send thread attaches...
                if (m_syncingForAttach == SYNC_STATE_3)
                    hr = g_pEEInterface->IterateThreadsForAttach(
                                                 &fAtleastOneEventSent,
                                                 TRUE);

                // Change the debug state of all attaching app domains to attached
                MarkAttachingAppDomainsAsAttachedToDebugger();
            }

            // If we're attaching due to an exception, set
            // exAttachEvent, which will let all excpetion threads
            // go. They will send their events as normal, and will
            // also cause the sync complete to be sent to complete the
            // attach. Therefore, we don't need to do this here.
            if (m_attachingForException && 
                (m_syncingForAttach == SYNC_STATE_3))
            {
                LOG((LF_CORDB, LL_INFO10000,
                     "D::HIPCE: Calling SetEvent on m_exAttachEvent= %x\n",
                     m_exAttachEvent));

                // Note: we have to force enable event handling right
                // here to be sure that at least one thread that is
                // blocked waiting for the attach to complete will be
                // able to send its exception or user breakpoint
                // event.
                EnableEventHandling(true);
                
                SetEvent(m_exAttachEvent);
            }
            else
            {
                if (fAtleastOneEventSent == TRUE)
                {
                    // Send the Sync Complete event next...
                    DebuggerIPCEvent* ipce = 
                        m_pRCThread->GetIPCEventSendBuffer(IPC_TARGET_OUTOFPROC);
                    InitIPCEvent(ipce, DB_IPCE_SYNC_COMPLETE);
                    m_pRCThread->SendIPCEvent(IPC_TARGET_OUTOFPROC);
                }

                if ((m_syncingForAttach == SYNC_STATE_3) ||
                    (m_syncingForAttach == SYNC_STATE_11))
                {
                    LOG((LF_CORDB, LL_INFO10000,"D::HIPCE: calling PAL_InitializeDebug.\n"));
                    // Tell the PAL that we're trying to debug
                    PAL_InitializeDebug();

                    // Attach is complete now.
                    LOG((LF_CORDB, LL_INFO10000, "D::HIPCE: Attach Complete!\n"));
                    g_pEEInterface->MarkDebuggerAttached();
                    m_syncingForAttach = SYNC_STATE_0;
                    LOG((LF_CORDB, LL_INFO10, "D::HIPCE: Attach state is now %s\n", g_ppszAttachStateToSZ[m_syncingForAttach]));

                    m_debuggerAttached = TRUE; //No-op for INPROC
                }
                else
                {
                    _ASSERTE ((m_syncingForAttach == SYNC_STATE_2) ||
                              (m_syncingForAttach == SYNC_STATE_10));

                    if (m_syncingForAttach == SYNC_STATE_2)
                    {
                        m_syncingForAttach = SYNC_STATE_3;
                        LOG((LF_CORDB, LL_INFO10, "D::HIPCE: Attach state is now %s\n", g_ppszAttachStateToSZ[m_syncingForAttach]));
                    }
                    else
                    {
                        m_syncingForAttach = SYNC_STATE_11;
                        LOG((LF_CORDB, LL_INFO10, "D::HIPCE: Attach state is now %s\n", g_ppszAttachStateToSZ[m_syncingForAttach]));
                    }
                }

                if (fAtleastOneEventSent == FALSE)
                    goto LetThreadsGo;
            }
        }
        else
        {
LetThreadsGo:
            ret = ResumeThreads((AppDomain*)event->appDomainToken);

            // If the helper thread is the owner of the thread store lock, then it got it via an async break, an attach,
            // or a successful sweeping. Go ahead and release it now that we're continuing. This ensures that we've held
            // the thread store lock the entire time the Runtime was just stopped.
            if (m_RCThreadHoldsThreadStoreLock)
            {
                m_RCThreadHoldsThreadStoreLock = FALSE;
                ThreadStore::UnlockThreadStore();
            }
        }

        break;

    case DB_IPCE_BREAKPOINT_ADD:
        {
            //
            // Currently, we can't create a breakpoint before a 
            // function desc is available.
            // Also, we can't know if a breakpoint is ok
            // prior to the method being JITted.
            //
            
            _ASSERTE(hr == S_OK);
            DebuggerBreakpoint *bp;
            bp = NULL;

            DebuggerModule *module;
            module = (DebuggerModule *) 
              event->BreakpointData.funcDebuggerModuleToken;

            if (m_pModules->IsDebuggerModuleDeleted(module))
            {
                LOG((LF_CORDB, LL_INFO1000000,"D::HIPCE: BP: Tried to set a bp"
                    " in a module that's been unloaded\n"));
                hr = CORDBG_E_MODULE_NOT_LOADED;
            }
            else
            {
                MethodDesc *pFD = g_pEEInterface->LookupMethodDescFromToken(
                        module->m_pRuntimeModule, 
                        event->BreakpointData.funcMetadataToken);

                DebuggerJitInfo *pDji = NULL;
                if ( NULL != pFD )
                    pDji = GetJitInfo(pFD, NULL );
                {
                    BOOL fSucceed;
                    // If we haven't been either JITted or EnC'd yet, then
                    // we'll put a patch in by offset, implicitly relative
                    // to the first version of the code.

                    bp = new (interopsafe) DebuggerBreakpoint(module->m_pRuntimeModule,
                                           event->BreakpointData.funcMetadataToken,
                                           (AppDomain *)event->appDomainToken,
                                           event->BreakpointData.offset,  
                                           !event->BreakpointData.isIL, 
                                           pDji,
                                           &fSucceed,
                                           FALSE);
                    TRACE_ALLOC(bp); 
                    if (bp != NULL && !fSucceed)
                    {
                        DeleteInteropSafe(bp);
                        bp = NULL;
                        hr = CORDBG_E_UNABLE_TO_SET_BREAKPOINT;
                    } 
                }

                if(NULL == bp && !FAILED(hr))
                {
                    hr = E_OUTOFMEMORY;
                }
                
                LOG((LF_CORDB,LL_INFO10000,"\tBP Add: DJI:0x%x BPTOK:"
                    "0x%x, tok=0x%08x, offset=0x%x, isIL=%d\n", pDji, bp,
                     event->BreakpointData.funcMetadataToken,
                     event->BreakpointData.offset,  
                     event->BreakpointData.isIL));
            }
            
            //
            // We're using a two-way event here, so we place the
            // result event into the _receive_ buffer, not the send
            // buffer.
            //
            
            _ASSERTE( iWhich != IPC_TARGET_INPROC );
            DebuggerIPCEvent *result = m_pRCThread->GetIPCEventReceiveBuffer(
                iWhich);
            InitIPCEvent(result, 
                         DB_IPCE_BREAKPOINT_ADD_RESULT,
                         GetCurrentThreadId(),
                         event->appDomainToken);
            result->BreakpointData.breakpoint =
                event->BreakpointData.breakpoint;
            result->BreakpointData.breakpointToken = bp;
            result->hr = hr;

            m_pRCThread->SendIPCReply(iWhich);
        }
        break;

    case DB_IPCE_STEP:
        {
            LOG((LF_CORDB,LL_INFO10000, "D::HIPCE: stepIn:0x%x frmTok:0x%x"
                "StepIn:0x%x RangeIL:0x%x RangeCount:0x%x MapStop:0x%x "
                "InterceptStop:0x%x AppD:0x%x\n",
                event->StepData.stepIn,
                event->StepData.frameToken, 
                event->StepData.stepIn,
                event->StepData.rangeIL, 
                event->StepData.rangeCount,
                event->StepData.rgfMappingStop, 
                event->StepData.rgfInterceptStop,
                event->appDomainToken));

            Thread *thread = (Thread *) event->StepData.threadToken;
            AppDomain *pAppDomain;
            pAppDomain = (AppDomain*)event->appDomainToken;

            DebuggerStepper *stepper = new (interopsafe) DebuggerStepper(thread,
                                            event->StepData.rgfMappingStop,
                                            event->StepData.rgfInterceptStop,
                                            pAppDomain);
            if (!stepper)
            {
                DebuggerIPCEvent *result = m_pRCThread->GetIPCEventReceiveBuffer(iWhich);

                InitIPCEvent(result, 
                             DB_IPCE_STEP_RESULT,
                             thread->GetThreadId(),
                             event->appDomainToken);
                result->hr = E_OUTOFMEMORY;                             

                m_pRCThread->SendIPCReply(iWhich);

                break;
            }
            TRACE_ALLOC(stepper);

            unsigned int cRanges = event->StepData.totalRangeCount;

            
            COR_DEBUG_STEP_RANGE *ranges = new (interopsafe) COR_DEBUG_STEP_RANGE [cRanges+1];
            if (!ranges)
            {
                DebuggerIPCEvent *result = m_pRCThread->GetIPCEventReceiveBuffer(iWhich);

                InitIPCEvent(result, 
                             DB_IPCE_STEP_RESULT,
                             thread->GetThreadId(),
                             event->appDomainToken);
                result->hr = E_OUTOFMEMORY;                             

                m_pRCThread->SendIPCReply(iWhich);

                delete stepper;
                break;
            }
            
                // The "+1"is for internal use, when we need to 
                // set an intermediate patch in pitched code.  Isn't
                // used unless the method is pitched & a patch is set
                // inside it.  Thus we still pass cRanges as the
                // range count.
                
            TRACE_ALLOC(ranges);
            // !!! failure

            if (cRanges > 0)
            {
                COR_DEBUG_STEP_RANGE *r = ranges;
                COR_DEBUG_STEP_RANGE *rEnd = r + cRanges;

                while (r < rEnd)
                {
                    COR_DEBUG_STEP_RANGE *rFrom = &event->StepData.range;
                    COR_DEBUG_STEP_RANGE *rFromEnd = rFrom +
                        event->StepData.rangeCount;

                    while (rFrom < rFromEnd)
                        *r++ = *rFrom++;
                }

                stepper->Step(event->StepData.frameToken,
                              event->StepData.stepIn,
                              ranges, cRanges,
                              event->StepData.rangeIL);
            }
            else
                stepper->Step(event->StepData.frameToken,
                              event->StepData.stepIn, ranges, 0, FALSE);

            _ASSERTE( iWhich != IPC_TARGET_INPROC );
            DebuggerIPCEvent *result = m_pRCThread->GetIPCEventReceiveBuffer(
                iWhich);
            InitIPCEvent(result, 
                         DB_IPCE_STEP_RESULT,
                         thread->GetThreadId(),
                         event->appDomainToken);
            result->StepData.stepper = event->StepData.stepper;
            result->StepData.stepperToken = stepper;

            LOG((LF_CORDB, LL_INFO10000, "Stepped stepper 0x%x | R: 0x%x "
                "E: 0x%x\n", stepper, result->StepData.stepper, event->StepData.stepper));

            m_pRCThread->SendIPCReply(iWhich);
        }
        break;

    case DB_IPCE_STEP_OUT:
        {
            Thread *thread = (Thread *) event->StepData.threadToken;
            AppDomain *pAppDomain;
            pAppDomain = (AppDomain*)event->appDomainToken;
            
            DebuggerStepper *stepper = new (interopsafe) DebuggerStepper(thread,
                                            event->StepData.rgfMappingStop,
                                            event->StepData.rgfInterceptStop,
                                            pAppDomain);

            if (!stepper)
            {
                DebuggerIPCEvent *result = m_pRCThread->GetIPCEventReceiveBuffer(iWhich);

                InitIPCEvent(result, 
                             DB_IPCE_STEP_RESULT,
                             thread->GetThreadId(),
                             pAppDomain);
                result->hr = E_OUTOFMEMORY;                             

                m_pRCThread->SendIPCReply(iWhich);

                break;
            }
                                                        
            TRACE_ALLOC(stepper);

            stepper->StepOut(event->StepData.frameToken);

            _ASSERTE( iWhich != IPC_TARGET_INPROC );
            DebuggerIPCEvent *result = m_pRCThread->GetIPCEventReceiveBuffer(
                iWhich);
            InitIPCEvent(result, 
                         DB_IPCE_STEP_RESULT,
                         thread->GetThreadId(),
                         pAppDomain);
            result->StepData.stepper = event->StepData.stepper;
            result->StepData.stepperToken = stepper;

            m_pRCThread->SendIPCReply(iWhich);
        }
        break;

    case DB_IPCE_BREAKPOINT_REMOVE:
        {

            DebuggerBreakpoint *bp 
              = (DebuggerBreakpoint *) event->BreakpointData.breakpointToken;
            
            bp->Delete();
        }
        break;

    case DB_IPCE_STEP_CANCEL:
        {
            LOG((LF_CORDB,LL_INFO10000, "D:HIPCE:Got STEP_CANCEL for stepper "
                "0x%x\n",(DebuggerStepper *) event->StepData.stepperToken));
            DebuggerStepper *stepper 
              = (DebuggerStepper *) event->StepData.stepperToken;
            
            stepper->Delete();
        }
        break;

    case DB_IPCE_STACK_TRACE:
        {
            Thread* thread =
                (Thread*) event->StackTraceData.debuggerThreadToken;

            //
            //
            LOG((LF_CORDB,LL_INFO1000, "Stack trace to :iWhich:0x%x\n",iWhich));
                        
            DebuggerThread::TraceAndSendStack(thread, m_pRCThread, iWhich);
        }
        break;

    case DB_IPCE_SET_DEBUG_STATE:
        {
            Thread* thread = (Thread*) event->SetDebugState.debuggerThreadToken;
            CorDebugThreadState debugState = event->SetDebugState.debugState;

            LOG((LF_CORDB,LL_INFO10000,"HandleIPCE:SetDebugState: thread 0x%x (ID:0x%x) to state 0x%x\n",
                thread,thread->GetThreadId(), debugState));

            g_pEEInterface->SetDebugState(thread, debugState);
            
            LOG((LF_CORDB,LL_INFO10000,"HandleIPC: Got 0x%x back from SetDebugState\n", hr));

            _ASSERTE( iWhich != IPC_TARGET_INPROC );
            DebuggerIPCEvent *result = m_pRCThread->GetIPCEventReceiveBuffer(iWhich);
            InitIPCEvent(result, DB_IPCE_SET_DEBUG_STATE_RESULT, 0, NULL);
            result->hr = S_OK;
            
            m_pRCThread->SendIPCReply(iWhich);
                
        }
        break;
        
    case DB_IPCE_SET_ALL_DEBUG_STATE:
        {
            Thread* et = (Thread*) event->SetAllDebugState.debuggerExceptThreadToken;
            CorDebugThreadState debugState = event->SetDebugState.debugState;

            LOG((LF_CORDB,LL_INFO10000,"HandleIPCE: SetAllDebugState: except thread 0x%08x (ID:0x%x) to state 0x%x\n",
                et, et != NULL ? et->GetThreadId() : 0, debugState));

            if (!g_fProcessDetach)
                g_pEEInterface->SetAllDebugState(et, debugState);
            
            LOG((LF_CORDB,LL_INFO10000,"HandleIPC: Got 0x%x back from SetAllDebugState\n", hr));

            _ASSERTE(iWhich != IPC_TARGET_INPROC);

            // Just send back an HR.
            DebuggerIPCEvent *result = m_pRCThread->GetIPCEventReceiveBuffer(iWhich);
            InitIPCEvent(result, DB_IPCE_SET_DEBUG_STATE_RESULT, 0, NULL);
            result->hr = S_OK;
            m_pRCThread->SendIPCReply(iWhich);
        }
        break;
        
    case DB_IPCE_GET_FUNCTION_DATA:
        {
            //
            //
            _ASSERTE(!m_pModules->IsDebuggerModuleDeleted(
                (DebuggerModule *)event->GetFunctionData.funcDebuggerModuleToken));
                
            GetAndSendFunctionData(
                               m_pRCThread,
                               event->GetFunctionData.funcMetadataToken,
                               event->GetFunctionData.funcDebuggerModuleToken,
                               event->GetFunctionData.nVersion,
                               iWhich);
        }
        break;
        
    case DB_IPCE_GET_OBJECT_INFO:
        {
            //
            //
            GetAndSendObjectInfo(
                               m_pRCThread,
                               (AppDomain *)event->appDomainToken,
                               event->GetObjectInfo.objectRefAddress,
                               event->GetObjectInfo.objectRefInHandle,

                               event->GetObjectInfo.objectRefIsValue,
                               event->GetObjectInfo.objectType,
                               event->GetObjectInfo.makeStrongObjectHandle,
                               iWhich != IPC_TARGET_INPROC, //make a handle only in out-of-proc case
                               iWhich);
        }
        break;

    case DB_IPCE_VALIDATE_OBJECT:
        {
            LOG((LF_CORDB,LL_INFO1000, "HandleIPCEvent:DB_IPCE_VALIDATE_OBJECT\n"));
        
            GetAndSendObjectInfo(
                               m_pRCThread,
                               (AppDomain *)event->appDomainToken,
                               event->ValidateObject.objectToken,
                               true, // In handle
                               true, // Is the value itself
                               event->ValidateObject.objectType,
                               false, 
                               false,
                               iWhich); //don't make a handle
            break;
        }

    case DB_IPCE_DISCARD_OBJECT:
        {
            if (iWhich != IPC_TARGET_INPROC)
            {
            g_pEEInterface->DbgDestroyHandle( 
                (OBJECTHANDLE)event->DiscardObject.objectToken, 
                event->DiscardObject.fStrong);
            }
            break;
        }

    case DB_IPCE_GET_CLASS_INFO:
        {
            //
            //
            _ASSERTE(!m_pModules->IsDebuggerModuleDeleted(
                (DebuggerModule *)event->GetClassInfo.classDebuggerModuleToken));

            GetAndSendClassInfo(
                               m_pRCThread,
                               event->GetClassInfo.classDebuggerModuleToken,
                               event->GetClassInfo.classMetadataToken,
                               (AppDomain *)event->appDomainToken,
                               mdFieldDefNil,
                               NULL,
                               iWhich);
        }
        break;

    case DB_IPCE_GET_SPECIAL_STATIC:
        {
            GetAndSendSpecialStaticInfo(
                               m_pRCThread,
                               event->GetSpecialStatic.fldDebuggerToken,
                               event->GetSpecialStatic.debuggerThreadToken,
                               iWhich);
        }
        break;

    case DB_IPCE_GET_JIT_INFO:
        {
            //
            //
            _ASSERTE(!m_pModules->IsDebuggerModuleDeleted(
                (DebuggerModule *)event->GetJITInfo.funcDebuggerModuleToken));

            GetAndSendJITInfo(
                               m_pRCThread,
                               event->GetJITInfo.funcMetadataToken,
                               event->GetJITInfo.funcDebuggerModuleToken,
                               (AppDomain *)event->appDomainToken,
                               iWhich);
        }
        break;
        
        
    case DB_IPCE_GET_CODE:
        {
            MethodDesc *fd = NULL;
            ULONG RVA;
            const BYTE *code;
            unsigned int codeSize;
            mdToken localSigToken;
            BOOL fSentEvent = FALSE;
            void *appDomainToken = event->appDomainToken;

            DebuggerModule* pDebuggerModule =
                (DebuggerModule*) event->GetCodeData.funcDebuggerModuleToken;

            if (m_pModules->IsDebuggerModuleDeleted(pDebuggerModule))
                hr = CORDBG_E_MODULE_NOT_LOADED;
            else
            {

                // get all the info about the function using metadata as a key.
                HRESULT hr = GetFunctionInfo(
                                     pDebuggerModule->m_pRuntimeModule,
                                     event->GetCodeData.funcMetadataToken,
                                     (MethodDesc**) &fd, &RVA,
                                     (BYTE**) &code, &codeSize,
                                     &localSigToken);

                if (SUCCEEDED(hr))
                {
                    DebuggerJitInfo *ji = (DebuggerJitInfo *)
                                            event->GetCodeData.CodeVersionToken;

                    // No DJI? Lets see if one has been created since the
                    // original data was sent to the Right Side...
                    if (ji == NULL)
                        ji = GetJitInfo( 
                            fd, 
                            (const BYTE*)DebuggerJitInfo::DJI_VERSION_FIRST_VALID, 
                            true );

                    // If the code has been pitched, then we simply tell
                    // the Right Side we can't get the code.
                    if (ji != NULL && ji->m_codePitched)
                    {
                        _ASSERTE( ji->m_prevJitInfo == NULL );
                        
                        // The code that the right side is asking for has
                        // been pitched since the last time it was referenced.
                        DebuggerIPCEvent *result =
                            m_pRCThread->GetIPCEventSendBuffer(iWhich);
                            
                        InitIPCEvent(result, 
                                     DB_IPCE_GET_CODE_RESULT,
                                     GetCurrentThreadId(),
                                     appDomainToken);
                        result->hr = CORDBG_E_CODE_NOT_AVAILABLE;
                        
                        fSentEvent = TRUE; //The event is 'sent' in-proc AND oop
                        if (iWhich ==IPC_TARGET_OUTOFPROC)
                        {
                            m_pRCThread->SendIPCEvent(iWhich);
                        }
                    } 
                    else
                    {
                        if (!event->GetCodeData.il)
                        {
                            _ASSERTE(fd != NULL);

                            // Grab the function address from the most
                            // reasonable place.
                            if ((ji != NULL) && ji->m_jitComplete)
                                code = (const BYTE*)ji->m_addrOfCode;
                            else
                                code = g_pEEInterface->GetFunctionAddress(fd);
                            
                            _ASSERTE(code != NULL);
                        }
                        
                        const BYTE *cStart = code + event->GetCodeData.start;
                        const BYTE *c = cStart;
                        const BYTE *cEnd = code + event->GetCodeData.end;

                        _ASSERTE(c < cEnd);

                        DebuggerIPCEvent *result = NULL;
                        DebuggerIPCEvent *resultT = NULL;

                        while (c < cEnd && 
                            (!result || result->hr != E_OUTOFMEMORY))
                        {
                            if (c == cStart || iWhich == IPC_TARGET_OUTOFPROC)
                                resultT = result = m_pRCThread->GetIPCEventSendBuffer(iWhich);
                            else
                            {
                                resultT = m_pRCThread->
                                    GetIPCEventSendBufferContinuation(result);
                                if (resultT != NULL)
                                    result = resultT;
                            }
                            
                            if (resultT == NULL)
                            {
                                result->hr = E_OUTOFMEMORY;
                            }
                            else
                            {
                                InitIPCEvent(result, 
                                             DB_IPCE_GET_CODE_RESULT,
                                             GetCurrentThreadId(),
                                             appDomainToken);
                                result->GetCodeData.start = c - code;
                            
                                BYTE *p = &result->GetCodeData.code;
                                BYTE *pMax = ((BYTE *) result) + CorDBIPC_BUFFER_SIZE;

                                SIZE_T size = pMax - p;

                                if ((SIZE_T)(cEnd - c) < size)
                                    size = cEnd - c;

                                result->GetCodeData.end = result->GetCodeData.start + size;

                                memcpy(p, c, size);
                                c += size;

                                DebuggerController::UnapplyPatchesInCodeCopy(
                                                     pDebuggerModule->m_pRuntimeModule,
                                                     event->GetCodeData.funcMetadataToken,
                                                     ji,
                                                     fd,
                                                     !event->GetCodeData.il,
                                                     p,
                                                     result->GetCodeData.start,
                                                     result->GetCodeData.end);
                            }
                            
                            fSentEvent = TRUE; //The event is 'sent' in-proc AND oop
                            if (iWhich ==IPC_TARGET_OUTOFPROC)
                            {
                                LOG((LF_CORDB,LL_INFO10000, "D::HIPCE: Get code sending"
                                    "to LS addr:0x%x\n", c));
                                m_pRCThread->SendIPCEvent(iWhich);
                                LOG((LF_CORDB,LL_INFO10000, "D::HIPCE: Code Sent\n"));
                            }
                        }
                    }
                }
            }
            
            // Something went wrong, so tell the right side so it's not left
            // hanging.
            if (!fSentEvent)
            {
                LOG((LF_CORDB,LL_INFO100000, "D::HIPCE: Get code failed!\n"));
            
                // Send back something that makes sense.
                if (hr == S_OK)
                    hr = E_FAIL;
            
                // failed to get any function info, so couldn't send
                // code. Send back the hr.
                DebuggerIPCEvent *result =
                    m_pRCThread->GetIPCEventSendBuffer(iWhich);
                    
                InitIPCEvent(result, 
                             DB_IPCE_GET_CODE_RESULT,
                             GetCurrentThreadId(),
                             appDomainToken);
                result->hr = hr;

                if (iWhich ==IPC_TARGET_OUTOFPROC)
                    m_pRCThread->SendIPCEvent(iWhich);
            }
        }
        
        LOG((LF_CORDB,LL_INFO10000, "D::HIPCE: Finished sending code!\n"));
        break;

    case DB_IPCE_GET_BUFFER:
        {
            _ASSERTE( iWhich != IPC_TARGET_INPROC );
        
            GetAndSendBuffer(m_pRCThread, event->GetBuffer.bufSize);
        }
        break;
    
    case DB_IPCE_RELEASE_BUFFER:
        {
            _ASSERTE( iWhich != IPC_TARGET_INPROC );
            
            SendReleaseBuffer(m_pRCThread, (BYTE *)event->ReleaseBuffer.pBuffer);
        }
        break;

        
    case DB_IPCE_SET_CLASS_LOAD_FLAG:
        {
            DebuggerModule *pModule =
                    (DebuggerModule*) event->SetClassLoad.debuggerModuleToken;
            _ASSERTE(pModule != NULL);
            if (!m_pModules->IsDebuggerModuleDeleted(pModule))
            {
                LOG((LF_CORDB, LL_INFO10000,
                     "D::HIPCE: class load flag is %d for module 0x%08x\n",
                     event->SetClassLoad.flag, pModule));
                
                pModule->EnableClassLoadCallbacks((BOOL)event->SetClassLoad.flag);
            }
        }
        break;
        
    case DB_IPCE_CONTINUE_EXCEPTION:
        {
            Thread *thread =
                (Thread *) event->ClearException.debuggerThreadToken;

            g_pEEInterface->ClearThreadException(thread);
        }
        break;
        
    case DB_IPCE_ATTACHING:
        // Perform some initialization necessary for debugging
        LOG((LF_CORDB,LL_INFO10000, "D::HIPCE: Attach begins!\n"));

        DebuggerController::Initialize();

        // Remember that we're attaching now...
        m_syncingForAttach = SYNC_STATE_1;
        LOG((LF_CORDB, LL_INFO10000, "Attach state is now %s\n", g_ppszAttachStateToSZ[m_syncingForAttach]));
        
        // Simply trap all Runtime threads...
        // This is an 'attach to process' msg, so it's not
        // unreasonable that we stop all the appdomains
        if (!m_trappingRuntimeThreads)
        {
            // Need to take the event handling lock so no other threads
            // try and send an event while we're sync'd, which can happen
            // if we successfully suspend all threads on the first pass.
            // i.e., a threads could have PGC enabled, and then come into
            // the runtime and send an event when we weren't expecting it
            // since event handling was not disabled.
            DisableEventHandling();

            m_RCThreadHoldsThreadStoreLock = TrapAllRuntimeThreads(NULL);

            // We set the syncThreadIsLockFree event here since the helper thread will never be suspended by the Right
            // Side.  (Note: this is all for Win32 debugging support.)
            if (m_pRCThread->GetDCB(IPC_TARGET_OUTOFPROC)->m_rightSideIsWin32Debugger)
                SetEvent(m_pRCThread->GetDCB(IPC_TARGET_OUTOFPROC)->m_syncThreadIsLockFree);
            
            // This will only enable event handling if TrapAllRuntimeThreads
            // successfully stopped all threads on the first pass.
            EnableEventHandling();
        }

        break;


    case DB_IPCE_IS_TRANSITION_STUB:

        GetAndSendTransitionStubInfo((const BYTE*)event->IsTransitionStub.address,
                                     iWhich);
        break;

    case DB_IPCE_MODIFY_LOGSWITCH:
        g_pEEInterface->DebuggerModifyingLogSwitch (
                            event->LogSwitchSettingMessage.iLevel,
                            &event->LogSwitchSettingMessage.Dummy[0]);

        break;

    case DB_IPCE_ENABLE_LOG_MESSAGES:
        {
            bool fOnOff = event->LogSwitchSettingMessage.iLevel ? true:false;
            EnableLogMessages (fOnOff);
        }
        break;
        
    case DB_IPCE_SET_IP:
            // This is a synchronous event (reply required)
            _ASSERTE( iWhich != IPC_TARGET_INPROC );
            _ASSERTE( event->SetIP.firstExceptionHandler != NULL);
        
            event = m_pRCThread->GetIPCEventReceiveBuffer(iWhich);
            Module *pModule;
            pModule = ((DebuggerModule*)(event->SetIP.debuggerModule))
                ->m_pRuntimeModule;

            // Don't have an explicit reply msg                
            InitIPCEvent(event, 
                         DB_IPCE_SET_IP,
                         event->threadId,
                         (void *)event->appDomainToken);
            
            if (!g_fProcessDetach)
            {
                event->hr = SetIP(  event->SetIP.fCanSetIPOnly,
                                    (Thread*)event->SetIP.debuggerThreadToken,
                                    pModule,
                                    event->SetIP.mdMethod,
                                    (DebuggerJitInfo*)event->SetIP.versionToken,
                                    event->SetIP.offset, 
                                    event->SetIP.fIsIL,
                                    event->SetIP.firstExceptionHandler);
            }
            else
                event->hr = S_OK;
            // Send the result
            m_pRCThread->SendIPCReply(iWhich);
        break;

    case DB_IPCE_ATTACH_TO_APP_DOMAIN:
        // Mark that we need to attach to a specific app domain (state
        // 10), but only if we're not already attaching to the process
        // as a whole (state 2).
        if (m_syncingForAttach != SYNC_STATE_2)
        {
            m_syncingForAttach = SYNC_STATE_10;
            LOG((LF_CORDB, LL_INFO10000, "Attach state is now %s\n", g_ppszAttachStateToSZ[m_syncingForAttach]));
        }
        
        // Simply trap all Runtime threads...
        if (!m_trappingRuntimeThreads)
        {
            m_RCThreadHoldsThreadStoreLock = TrapAllRuntimeThreads(NULL);

            // We set the syncThreadIsLockFree event here since the helper thread will never be suspended by the Right
            // Side.  (Note: this is all for Win32 debugging support.)
            if (m_pRCThread->GetDCB(IPC_TARGET_OUTOFPROC)->m_rightSideIsWin32Debugger)
                SetEvent(m_pRCThread->GetDCB(IPC_TARGET_OUTOFPROC)->m_syncThreadIsLockFree);
        }

        event->hr = AttachDebuggerToAppDomain(event->AppDomainData.id);
        break;

    case DB_IPCE_DETACH_FROM_APP_DOMAIN:
        AppDomain *ad;

        hr = DetachDebuggerFromAppDomain(event->AppDomainData.id, &ad);
		if (FAILED(hr) )
		{
			event->hr = hr;
			break;
		}	
        
        if (ad != NULL)
        {
            LOG((LF_CORDB, LL_INFO10000, "Detaching from AppD:0x%x\n", ad));
        
            ClearAppDomainPatches(ad);
        }

        break;

    case DB_IPCE_DETACH_FROM_PROCESS:
        LOG((LF_CORDB, LL_INFO10000, "Detaching from process!\n"));

        // At this point, all patches should have been removed
        // by detaching from the appdomains.

        // Commented out for hotfix bug 94625.  Should be re-enabled at some point.
        //_ASSERTE(DebuggerController::GetNumberOfPatches() == 0);

        g_pEEInterface->MarkDebuggerUnattached();
        m_debuggerAttached = FALSE;

        // Need to close it before we recreate it.
        if (m_pRCThread->m_SetupSyncEvent == NULL)
        {
            hr = m_pRCThread->CreateSetupSyncEvent();
            if (FAILED(hr))
            {
                event->hr = hr;
                break;
            }
        }

        SetEvent(m_pRCThread->m_SetupSyncEvent);
        
        m_pRCThread->RightSideDetach();

        // Clean up the hash of DebuggerModules
        // This method is overridden to also free all DebuggerModule objects
        if (m_pModules != NULL)
            m_pModules->Clear();

        // Reply to the detach message before we release any Runtime threads. This ensures that the debugger will get
        // the detach reply before the process exits if the main thread is near exiting.
        m_pRCThread->SendIPCReply(iWhich);

        // Let the process run free now... there is no debugger to bother it anymore.
        ret = ResumeThreads(NULL);

        // If the helper thread is the owner of the thread store lock, then it got it via an async break, an attach, or
        // a successful sweeping. Go ahead and release it now that we're continuing. This ensures that we've held the
        // thread store lock the entire time the Runtime was just stopped.
        if (m_RCThreadHoldsThreadStoreLock)
        {
            m_RCThreadHoldsThreadStoreLock = FALSE;
            ThreadStore::UnlockThreadStore();
        }
        
        break;

    case DB_IPCE_FUNC_EVAL:
        {
            // This is a synchronous event (reply required)
            _ASSERTE( iWhich != IPC_TARGET_INPROC );
            _ASSERTE(!m_pModules->IsDebuggerModuleDeleted(
                (DebuggerModule *)event->FuncEval.funcDebuggerModuleToken));
                
            event = m_pRCThread->GetIPCEventReceiveBuffer(iWhich);
            Thread *pThread;
            pThread = (Thread*)(event->FuncEval.funcDebuggerThreadToken);

            InitIPCEvent(event, DB_IPCE_FUNC_EVAL_SETUP_RESULT, pThread->GetThreadId(), pThread->GetDomain());

            BYTE *argDataArea = NULL;
            void *debuggerEvalKey = NULL;
            
            event->hr = FuncEvalSetup(&(event->FuncEval), &argDataArea, &debuggerEvalKey);
      
            // Send the result of how the func eval setup went.
            event->FuncEvalSetupComplete.argDataArea = argDataArea;
            event->FuncEvalSetupComplete.debuggerEvalKey = debuggerEvalKey;
            
            m_pRCThread->SendIPCReply(iWhich);
        }

        break;

    case DB_IPCE_SET_REFERENCE:
            // This is a synchronous event (reply required)
            _ASSERTE( iWhich != IPC_TARGET_INPROC );
            event = m_pRCThread->GetIPCEventReceiveBuffer(iWhich);
            InitIPCEvent(event, 
                         DB_IPCE_SET_REFERENCE_RESULT, 
                         event->threadId,
                         (void *)event->appDomainToken);
            
            event->hr = SetReference(event->SetReference.objectRefAddress,
                                     event->SetReference.objectRefInHandle,
                                     event->SetReference.newReference);
      
            // Send the result of how the set reference went.
            m_pRCThread->SendIPCReply(iWhich);

        break;

    case DB_IPCE_SET_VALUE_CLASS:
            // This is a synchronous event (reply required)
            _ASSERTE(iWhich != IPC_TARGET_INPROC);
            event = m_pRCThread->GetIPCEventReceiveBuffer(iWhich);
            
            InitIPCEvent(event, DB_IPCE_SET_VALUE_CLASS_RESULT, event->threadId, (void *)event->appDomainToken);
            
            event->hr = SetValueClass(event->SetValueClass.oldData,
                                      event->SetValueClass.newData,
                                      event->SetValueClass.classMetadataToken,
                                      event->SetValueClass.classDebuggerModuleToken);
      
            // Send the result of how the set reference went.
            m_pRCThread->SendIPCReply(iWhich);

        break;

    case DB_IPCE_GET_APP_DOMAIN_NAME:
        {
            WCHAR *pszName = NULL;
            AppDomain *pAppDomain = (AppDomain *)event->appDomainToken;

            // This is a synchronous event (reply required)
            event = m_pRCThread->GetIPCEventReceiveBuffer(iWhich);
            InitIPCEvent(event, 
                         DB_IPCE_APP_DOMAIN_NAME_RESULT, 
                         event->threadId, // ==> don't change it.
                         event->appDomainToken);
                         
            pszName = (WCHAR *)pAppDomain->GetFriendlyName();
            if (pszName != NULL)
                wcscpy ((WCHAR *)event->AppDomainNameResult.rcName, 
                    pszName);
            else
                wcscpy ((WCHAR *)event->AppDomainNameResult.rcName, 
                    L"<UnknownName>");

            event->hr = S_OK;
            m_pRCThread->SendIPCReply(iWhich);
        }

        break;

    case DB_IPCE_FUNC_EVAL_ABORT:
        LOG((LF_CORDB, LL_INFO1000, "D::HIPCE: Got FuncEvalAbort for pDE:%08x\n",
            event->FuncEvalAbort.debuggerEvalKey));

        // This is a synchronous event (reply required)
        _ASSERTE( iWhich != IPC_TARGET_INPROC );
        
        event = m_pRCThread->GetIPCEventReceiveBuffer(iWhich);
        InitIPCEvent(event, 
                     DB_IPCE_FUNC_EVAL_ABORT_RESULT, 
                     event->threadId,
                     event->appDomainToken);

        event->hr = FuncEvalAbort(event->FuncEvalAbort.debuggerEvalKey);
      
        m_pRCThread->SendIPCReply(iWhich);

        break;

    case DB_IPCE_FUNC_EVAL_CLEANUP:

        // This is a synchronous event (reply required)
        _ASSERTE(iWhich != IPC_TARGET_INPROC);
        
        event = m_pRCThread->GetIPCEventReceiveBuffer(iWhich);
        InitIPCEvent(event, 
                     DB_IPCE_FUNC_EVAL_CLEANUP_RESULT, 
                     event->threadId,
                     event->appDomainToken);

        event->hr = FuncEvalCleanup(event->FuncEvalCleanup.debuggerEvalKey);
      
        m_pRCThread->SendIPCReply(iWhich);

        break;

    case DB_IPCE_GET_THREAD_OBJECT:
        {
            // This is a synchronous event (reply required)
            Thread *pRuntimeThread =
                (Thread *)event->ObjectRef.debuggerObjectToken;
            _ASSERTE(pRuntimeThread != NULL);
            
            event = m_pRCThread->GetIPCEventReceiveBuffer(iWhich);
            InitIPCEvent(event, 
                         DB_IPCE_THREAD_OBJECT_RESULT, 
                         0,
                         (void *)(pRuntimeThread->GetDomain()));

            Thread::ThreadState ts = pRuntimeThread->GetSnapshotState();

            if ((ts & Thread::TS_Dead) ||
                (ts & Thread::TS_Unstarted) ||
                (ts & Thread::TS_Detached) ||
                g_fProcessDetach)
            {
                event->hr =  CORDBG_E_BAD_THREAD_STATE;
            }
            else
            {    
                event->ObjectRef.managedObject = (void *)
                    pRuntimeThread->GetExposedObjectHandleForDebugger();

                event->hr = S_OK;
            }
            m_pRCThread->SendIPCReply(iWhich);
        }
        break;

    case DB_IPCE_CHANGE_JIT_DEBUG_INFO:
        {
            Module *module = NULL;
            DWORD dwBits = 0;
            DebuggerModule *deModule = (DebuggerModule *) 
                event->JitDebugInfo.debuggerModuleToken;

            if (m_pModules->IsDebuggerModuleDeleted(deModule))
            {
                hr = CORDBG_E_MODULE_NOT_LOADED;
            }
            else
            {
                module = deModule->m_pRuntimeModule;
                _ASSERTE(NULL != module);


                if (event->JitDebugInfo.fTrackInfo)
                    dwBits |= DACF_TRACK_JIT_INFO;

                if (event->JitDebugInfo.fAllowJitOpts)
                    dwBits |= DACF_ALLOW_JIT_OPTS;
                else
                    dwBits |= DACF_ENC_ENABLED;

                // Settings from the debugger take precedence over all
                // other settings.
                dwBits |= DACF_USER_OVERRIDE;
            }
            
            event = m_pRCThread->GetIPCEventReceiveBuffer(iWhich);
            InitIPCEvent(event, DB_IPCE_CHANGE_JIT_INFO_RESULT, 0, NULL);

            if (FAILED(hr))
            {
                event->hr = hr;
            }
            else
            {
                _ASSERTE(module != NULL);
                {
                    module->SetDebuggerInfoBits((DebuggerAssemblyControlFlags)dwBits);
                    event->hr = S_OK;
                }
            }
            
            m_pRCThread->SendIPCReply(iWhich);
        }
        break;

    case DB_IPCE_CONTROL_C_EVENT_RESULT:
        if (event->hr == S_OK)
            m_DebuggerHandlingCtrlC = TRUE;
        else
            m_DebuggerHandlingCtrlC = FALSE;
        SetEvent(m_CtrlCMutex);

        break;

    case DB_IPCE_GET_SYNC_BLOCK_FIELD:
        GetAndSendSyncBlockFieldInfo(event->GetSyncBlockField.debuggerModuleToken,
                                     event->GetSyncBlockField.classMetadataToken,
                                     (Object *)event->GetSyncBlockField.pObject,
                                     event->GetSyncBlockField.objectType,
                                     event->GetSyncBlockField.offsetToVars,
                                     event->GetSyncBlockField.fldToken,
                                     (BYTE *)event->GetSyncBlockField.staticVarBase,
                                     m_pRCThread,
                                     iWhich);
       break;                                     
    
    default:
        LOG((LF_CORDB, LL_INFO10000, "Unknown event type: 0x%08x\n",
             event->type));
    }

    Unlock();

    return ret;
}

//
// After a class has been loaded, if a field has been added via EnC'd, 
// we'll have to jump through some hoops to get at it.
//
HRESULT Debugger::GetAndSendSyncBlockFieldInfo(void *debuggerModuleToken,
                                               mdTypeDef classMetadataToken,
                                               Object *pObject,
                                               CorElementType objectType,
                                               SIZE_T offsetToVars,
                                               mdFieldDef fldToken,
                                               BYTE *staticVarBase,
                                               DebuggerRCThread* rcThread,
                                               IpcTarget iWhich)
{
    LOG((LF_CORDB, LL_INFO100000, "D::GASSBFI: dmtok:0x%x Obj:0x%x, objType"
        ":0x%x, offset:0x%x\n", debuggerModuleToken, pObject, objectType,
        offsetToVars));

    DebuggerModule *dm;
    
    dm = (DebuggerModule *)(debuggerModuleToken);
     
    HRESULT hr = S_OK;

    // We'll wrap this in an SEH handler, even though we should
    // never actually get whacked by this - the CordbObject should
    // validate the pointer first.
    PAL_TRY
    {
        FieldDesc *pFD = NULL;

        // Note that GASCI will scribble over both the data in the incoming
        // mesage, and the outgoing message, so don't bother to prep the reply
        // before calling this.
        hr = GetAndSendClassInfo(rcThread,
                                 debuggerModuleToken,
                                 classMetadataToken,
                                 dm->m_pAppDomain,
                                 fldToken,
                                 &pFD, //OUT
                                 iWhich);
        DebuggerIPCEvent *result = rcThread->GetIPCEventReceiveBuffer(
            iWhich);
        InitIPCEvent(result, 
                     DB_IPCE_GET_SYNC_BLOCK_FIELD_RESULT);
                     
        if (pFD == NULL)
        {
            result->hr = CORDBG_E_ENC_HANGING_FIELD;

            hr = rcThread->SendIPCReply(iWhich);
            PAL_LEAVE;
        }
            
        _ASSERTE(pFD->IsEnCNew()); // Shouldn't be here if it wasn't added to an
            //already loaded class.

        const BYTE *pORField = (BYTE *)pFD->GetAddress(pObject);

        result->GetSyncBlockFieldResult.fStatic = pFD->IsStatic(); 
        DebuggerIPCE_FieldData *currentFieldData = 
            &(result->GetSyncBlockFieldResult.fieldData);
       
        currentFieldData->fldDebuggerToken = (void*)pFD;
        currentFieldData->fldIsTLS = (pFD->IsThreadStatic() == TRUE);
        currentFieldData->fldMetadataToken = pFD->GetMemberDef();
        currentFieldData->fldIsRVA = (pFD->IsRVA() == TRUE);
        currentFieldData->fldIsContextStatic = (pFD->IsContextStatic() == TRUE);


        // We'll get the sig out of the metadata on the right side
        currentFieldData->fldFullSigSize = 0;
        currentFieldData->fldFullSig = NULL;
        
        PCCOR_SIGNATURE pSig = NULL;
        DWORD cSig = 0;

        g_pEEInterface->FieldDescGetSig(pFD, &pSig, &cSig);
        _ASSERTE(*pSig == IMAGE_CEE_CS_CALLCONV_FIELD);
        ++pSig;
        
        ULONG cb = _skipFunkyModifiersInSignature(pSig);
        pSig = &pSig[cb];
        
        currentFieldData->fldType = (CorElementType) *pSig;

        if (pFD->IsStatic())
        {
            if (pFD->IsThreadStatic())
            {
                // fldOffset is used to store the pointer directly, so that
                // we can get it out in the right side.
                currentFieldData->fldOffset = (SIZE_T)pORField;
            }
            else if (pFD->IsContextStatic())
            {
                _ASSERTE(!"NYI!");
            }
            else
            {
                // fldOffset is computed to work correctly with GetStaticFieldValue
                // which computes:
                // addr of pORField = staticVarBase + offsetToFld
                currentFieldData->fldOffset = pORField - staticVarBase;
            }
        }
        else
        {
            // fldOffset is computed to work correctly with GetFieldValue
            // which computes:
            // addr of pORField = object + offsetToVars + offsetToFld
            currentFieldData->fldOffset = pORField - ((BYTE *)pObject + offsetToVars);
        }
        hr = rcThread->SendIPCReply(iWhich);
    }
    PAL_EXCEPT_FILTER(FilterAccessViolation, NULL)
    {
        _ASSERTE(!"Given a bad ref to GASSBFI for!");
        hr = CORDBG_E_BAD_REFERENCE_VALUE;
    }
    PAL_ENDTRY

    return hr;
}

//
// GetAndSendFunctionData gets the necessary data for a function and
// sends it back to the right side.
//
HRESULT Debugger::GetAndSendFunctionData(DebuggerRCThread* rcThread,
                                         mdMethodDef funcMetadataToken,
                                         void* funcDebuggerModuleToken,
                                         SIZE_T nVersion,
                                         IpcTarget iWhich)
{
    LOG((LF_CORDB, LL_INFO10000, "D::GASFD: getting function data for "
         "0x%08x 0x%08x.\n", funcMetadataToken, funcDebuggerModuleToken));

    // Make sure we've got good data from the right side.
    _ASSERTE(funcDebuggerModuleToken != NULL);
    _ASSERTE(funcMetadataToken != NULL);

    DebuggerModule* pDebuggerModule =
        (DebuggerModule*) funcDebuggerModuleToken;
    _ASSERTE(pDebuggerModule->m_pRuntimeModule != NULL);

    BaseDomain *bd = pDebuggerModule->m_pRuntimeModule->GetDomain();
    // Setup the event that we'll be sending the results in.
    DebuggerIPCEvent* event = rcThread->GetIPCEventReceiveBuffer(iWhich);
    InitIPCEvent(event, 
                 DB_IPCE_FUNCTION_DATA_RESULT, 
                 0,
                 (void *)(AppDomain *)bd);
    event->FunctionDataResult.funcMetadataToken = funcMetadataToken;
    event->FunctionDataResult.funcDebuggerModuleToken =
        funcDebuggerModuleToken;
    event->FunctionDataResult.funcRVA = 0;
    event->FunctionDataResult.classMetadataToken = mdTypeDefNil;
    event->FunctionDataResult.ilStartAddress = NULL;
    event->FunctionDataResult.ilSize = 0;
    event->FunctionDataResult.ilnVersion = DJI_VERSION_INVALID;
    event->FunctionDataResult.nativeStartAddressPtr = NULL;
    event->FunctionDataResult.nativeSize = 0;
    event->FunctionDataResult.nativenVersion = DJI_VERSION_INVALID;
    event->FunctionDataResult.CodeVersionToken = NULL;
    event->FunctionDataResult.nVersionMostRecentEnC = DJI_VERSION_INVALID;
    
#ifdef DEBUG
    event->FunctionDataResult.nativeOffset = 0xdeadbeef;
        // Since Populate doesn't create a CordbNativeFrame, we don't
        // need the nativeOffset field to contain anything valid...
#endif //DEBUG
    event->FunctionDataResult.localVarSigToken = mdSignatureNil;
    event->FunctionDataResult.ilToNativeMapAddr = NULL;
    event->FunctionDataResult.ilToNativeMapSize = 0;

    MethodDesc *pFD=NULL;

    HRESULT hr = GetFunctionInfo(
          pDebuggerModule->m_pRuntimeModule,
         funcMetadataToken, &pFD,
         (ULONG*)&event->FunctionDataResult.funcRVA,
         (BYTE**) &event->FunctionDataResult.ilStartAddress,
         (unsigned int *) &event->FunctionDataResult.ilSize,
         &event->FunctionDataResult.localVarSigToken);

    
    if (SUCCEEDED(hr))
    {
        if (pFD != NULL)
        {
            DebuggerJitInfo *ji = GetJitInfo(pFD, (const BYTE*)nVersion, true);
            
            if (ji != NULL && ji->m_jitComplete)
            {
                LOG((LF_CORDB, LL_INFO10000, "EE:D::GASFD: JIT info found.\n"));
                
                // Send over the native info
                // Note that m_addrOfCode may be NULL (if the code was pitched)
                event->FunctionDataResult.nativeStartAddressPtr = 
                    &(ji->m_addrOfCode);

                // We should use the DJI rather than GetFunctionSize because
                // LockAndSendEnCRemapEvent will stop us at a point
                // that's prior to the MethodDesc getting updated, so it will
                // look as though the method hasn't been JITted yet, even
                // though we may get the LockAndSendEnCRemapEvent as a result
                // of a JITComplete callback
                event->FunctionDataResult.nativeSize = ji->m_sizeOfCode;

                event->FunctionDataResult.nativenVersion = ji->m_nVersion;
                event->FunctionDataResult.CodeVersionToken = (void*)ji;

                // Pass back the pointers to the sequence point map so
                // that the RIght Side can copy it out if needed.
                _ASSERTE(ji->m_sequenceMapSorted);
                
                event->FunctionDataResult.ilToNativeMapAddr =
                    ji->m_sequenceMap;
                event->FunctionDataResult.ilToNativeMapSize =
                    ji->m_sequenceMapCount;
            }
            else
            {
                event->FunctionDataResult.CodeVersionToken = NULL;
            }

            SIZE_T nVersionMostRecentlyEnCd = GetVersionNumber(pFD);
    
            event->FunctionDataResult.nVersionMostRecentEnC = nVersionMostRecentlyEnCd;

            // There's no way to do an EnC on a method with an IL body without 
            // providing IL, so either we can't get the IL, or else the version
            // number of the IL is the same as the most recently EnC'd version.
            event->FunctionDataResult.ilnVersion = nVersionMostRecentlyEnCd;

            // Send back the typeDef token for the class that this
            // function belongs to.
            event->FunctionDataResult.classMetadataToken =
                pFD->GetClass()->GetCl();

            LOG((LF_CORDB, LL_INFO10000, "D::GASFD: function is class. "
                 "0x%08x\n",
                 event->FunctionDataResult.classMetadataToken));
        }
        else
        {
            // No MethodDesc, so the class hasn't been loaded yet.
            // Get the class this method is in.
            mdToken tkParent;
            
            hr = g_pEEInterface->GetParentToken(
                                          pDebuggerModule->m_pRuntimeModule,
                                          funcMetadataToken,
                                          &tkParent);

            if (SUCCEEDED(hr))
            {
                _ASSERTE(TypeFromToken(tkParent) == mdtTypeDef);
            
                event->FunctionDataResult.classMetadataToken = tkParent;

                LOG((LF_CORDB, LL_INFO10000, "D::GASFD: function is class. "
                     "0x%08x\n",
                     event->FunctionDataResult.classMetadataToken));
            }
        }
    }

    // If we didn't get the MethodDesc, then we didn't get the version
    // number b/c it was never set (the DJI tables are indexed by MethodDesc)
    if (pFD == NULL)
    {
        event->FunctionDataResult.nVersionMostRecentEnC = DebuggerJitInfo::DJI_VERSION_FIRST_VALID;
        event->FunctionDataResult.ilnVersion = DebuggerJitInfo::DJI_VERSION_FIRST_VALID;
    }
    
    event->hr = hr;
    
    LOG((LF_CORDB, LL_INFO10000, "D::GASFD: sending result->nSAP:0x%x\n",
            event->FunctionDataResult.nativeStartAddressPtr));

    // Send off the data to the right side.
    hr = rcThread->SendIPCReply(iWhich);
    
    return hr;
}


// If the module lookup for (*pobjClassDebuggerModuleToken) failed 
// and we're in-process debugging, assume it's an inmemory module and 
// that it needs to be added. 
void Debugger::EnsureModuleLoadedForInproc(
    void ** pobjClassDebuggerModuleToken, // in-out
    EEClass *objClass, // in
    AppDomain *pAppDomain, // in
    IpcTarget iWhich // in
)
{
    _ASSERTE(pobjClassDebuggerModuleToken != NULL);
    
    if (*pobjClassDebuggerModuleToken == NULL && iWhich == IPC_TARGET_INPROC)
    {
        // Get the module for the class (it should be in-memory)
        Module *pMod = objClass->GetModule();
        _ASSERTE(pMod != NULL);

        // Add the module and get a DebuggerModule back
        DebuggerModule *pDMod = AddDebuggerModule(pMod, pAppDomain);
        _ASSERTE(pDMod != NULL);
        _ASSERTE(LookupModule(objClass->GetModule(), pAppDomain) != NULL);

        // Now set the token
        *pobjClassDebuggerModuleToken = (void*)pDMod;
            (void*) LookupModule(objClass->GetModule(), pAppDomain);
    }
    _ASSERTE (*pobjClassDebuggerModuleToken != NULL);
}

//
// GetAndSendObjectInfo gets the necessary data for an object and
// sends it back to the right side.
//
HRESULT Debugger::GetAndSendObjectInfo(DebuggerRCThread* rcThread,
                                       AppDomain *pAppDomain,
                                       void* objectRefAddress,
                                       bool objectRefInHandle,
                                       bool objectRefIsValue,
                                       CorElementType objectType,
                                       bool fStrongNewRef,
                                       bool fMakeHandle,
                                       IpcTarget iWhich)
{
    LOG((LF_CORDB, LL_INFO10000, "D::GASOI: getting info for "
         "0x%08x %d %d.\n", objectRefAddress, objectRefInHandle,
         objectRefIsValue));

    Object *objPtr = NULL;
    void *objRef;
        
    // Setup the event that we'll be sending the results in.
    DebuggerIPCEvent* event = rcThread->GetIPCEventReceiveBuffer(iWhich);
    InitIPCEvent(event, 
                 DB_IPCE_GET_OBJECT_INFO_RESULT, 
                 0,
                 (void *)pAppDomain);

    DebuggerIPCE_ObjectData *oi = &(event->GetObjectInfoResult);
    oi->objRef = NULL;
    oi->objRefBad = false;
    oi->objSize = 0;
    oi->objOffsetToVars = 0;
    oi->objectType = objectType;
    oi->objClassMetadataToken = mdTypeDefNil;
    oi->objClassDebuggerModuleToken = NULL;
    oi->nstructInfo.size = 0;
    oi->nstructInfo.ptr = NULL;
    oi->objToken = NULL;

    bool badRef = false;
    bool plainSend = false;
    
    // We wrap this in SEH just in case the object reference is bad.
    // We can trap the access violation and return a reasonable result.
    PAL_TRY
    {
        // We use this method for getting info about TypedByRef's,
        // too. But they're somewhat different than you're standard
        // object ref, so we special case here.
        if (objectType == ELEMENT_TYPE_TYPEDBYREF)
        {
            // The objectRefAddress really points to a TypedByRef struct.
            TypedByRef *ra = (TypedByRef*) objectRefAddress;

            // Grab the class. This will be NULL if its an array ref type.
            EEClass *cl = ra->type.AsClass();
            if (cl != NULL)
            {
                // If we have a non-array class, pass back the class
                // token and module.
                oi->objClassMetadataToken = cl->GetCl();
                oi->objClassDebuggerModuleToken =
                    (void*) LookupModule(cl->GetModule(), pAppDomain);
                _ASSERTE (oi->objClassDebuggerModuleToken != NULL);
            }

            // The reference to the object is in the data field of the TypedByRef.
            oi->objRef = ra->data;
        
            LOG((LF_CORDB, LL_INFO10000, "D::GASOI: sending REFANY result: "
                 "ref=0x%08x, cls=0x%08x, mod=0x%08x\n",
                 oi->objRef,
                 oi->objClassMetadataToken,
                 oi->objClassDebuggerModuleToken));

            // Send off the data to the right side.
            plainSend = true;
            PAL_LEAVE;
        }
    
        // Grab the pointer to the object.
        if (objectRefIsValue)
            objRef = objectRefAddress;
        else
            objRef = *((void**)objectRefAddress);
    
        MethodTable *pMT = NULL;
    
        if (objectRefInHandle)
        {
            OBJECTHANDLE oh = (OBJECTHANDLE) objRef;

            if (oh != NULL)
                objPtr = (Object*) g_pEEInterface->GetObjectFromHandle(oh);
            else
            {
                LOG((LF_CORDB, LL_INFO10000,
                     "D::GASOI: bad ref due to null object handle.\n"));
                
                objPtr = NULL;
                badRef = true;
                PAL_LEAVE;
            }
        }
        else
            objPtr = (Object*) objRef;
        
        // Pass back the object pointer.
        oi->objRef = objPtr;

        // Shortcut null references now...
        if (objPtr == NULL)
        {
            LOG((LF_CORDB, LL_INFO10000, "D::GASOI: ref is NULL.\n"));

            badRef = true;
            PAL_LEAVE;
        }
        
        EEClass *objClass = objPtr->GetClass();
        pMT = objPtr->GetMethodTable();

        // Try to verify the integrity of the object. This is not fool proof.
        if (pMT != objClass->GetMethodTable())
        {
            LOG((LF_CORDB, LL_INFO10000, "D::GASOI: MT's don't match.\n"));

            badRef = true;
            PAL_LEAVE;
        }

        // Save basic object info.
        oi->objSize = objPtr->GetSize();
        oi->objOffsetToVars =
            (UINT_PTR)((Object*)objPtr)->GetData() - (UINT_PTR)objPtr;

        // If this is a string object, set the type to ELEMENT_TYPE_STRING.
        if (g_pEEInterface->IsStringObject((Object*)objPtr))
            oi->objectType = ELEMENT_TYPE_STRING;
        else
        {
            if (objClass->IsArrayClass())
            {
                // If this is an array object, set its type appropiatley.
                ArrayClass *ac = (ArrayClass*)objClass;

                //
                //
                //
                if (ac->GetRank() == 1)
                    oi->objectType = ELEMENT_TYPE_SZARRAY;
                else
                    oi->objectType = ELEMENT_TYPE_ARRAY;
            }
            else
            {
                // Its not an array class... but if the element type
                // indicates array, then we have an Object in place of
                // an Array, so we need to change the element type
                // appropiatley.
                if ((oi->objectType == ELEMENT_TYPE_ARRAY) ||
                    (oi->objectType == ELEMENT_TYPE_SZARRAY))
                {
                    oi->objectType = ELEMENT_TYPE_CLASS;
                }
                else if (oi->objectType == ELEMENT_TYPE_STRING)
                {
                    // Well, we thought we had a string, but it turns
                    // out its not an array, nor is it a string. So
                    // we'll just assume the basic object and go from
                    // there.
                    oi->objectType = ELEMENT_TYPE_CLASS;
                }
            }
        }
        
        switch (oi->objectType)
        {
        case ELEMENT_TYPE_STRING:
            {
                LOG((LF_CORDB, LL_INFO10000, "D::GASOI: its a string.\n"));

                StringObject *so = (StringObject*)objPtr;

//                (void*) LookupModule(objClass->GetModule(), pAppDomain);
                oi->stringInfo.length =
                    g_pEEInterface->StringObjectGetStringLength(so);
                oi->stringInfo.offsetToStringBase =
                    (UINT_PTR) g_pEEInterface->StringObjectGetBuffer(so) -
                    (UINT_PTR) objPtr;

                // Pass back the object's class
                oi->objClassMetadataToken = objClass->GetCl();
                oi->objClassDebuggerModuleToken =
                    (void*) LookupModule(objClass->GetModule(), pAppDomain);

                EnsureModuleLoadedForInproc(&oi->objClassDebuggerModuleToken, 
                    objClass, pAppDomain, iWhich);
            }

            break;

        case ELEMENT_TYPE_CLASS:
        case ELEMENT_TYPE_OBJECT:
            // Pass back the object's class
            oi->objClassMetadataToken = objClass->GetCl();
            oi->objClassDebuggerModuleToken =
                (void*) LookupModule(objClass->GetModule(), pAppDomain);

            EnsureModuleLoadedForInproc(&oi->objClassDebuggerModuleToken, 
                objClass, pAppDomain, iWhich);
            
            break;

        //
        //
        case ELEMENT_TYPE_SZARRAY:
        case ELEMENT_TYPE_ARRAY:
            {
                if (!pMT->IsArray())
                {
                    LOG((LF_CORDB, LL_INFO10000,
                         "D::GASOI: object should be an array.\n"));

                    badRef = true;
                    PAL_LEAVE;
                }

                ArrayBase *arrPtr = (ArrayBase*)objPtr;
                
                oi->arrayInfo.componentCount = arrPtr->GetNumComponents();
                oi->arrayInfo.offsetToArrayBase =
                    (UINT_PTR)arrPtr->GetDataPtr() - (UINT_PTR)arrPtr;

                if (arrPtr->IsMultiDimArray())
                {
                    oi->arrayInfo.offsetToUpperBounds =
                        (UINT_PTR)arrPtr->GetBoundsPtr() - (UINT_PTR)arrPtr;

                    oi->arrayInfo.offsetToLowerBounds =
                        (UINT_PTR)arrPtr->GetLowerBoundsPtr() - (UINT_PTR)arrPtr;
                }
                else
                {
                    oi->arrayInfo.offsetToUpperBounds = 0;
                    oi->arrayInfo.offsetToLowerBounds = 0;
                }
                
                oi->arrayInfo.rank = arrPtr->GetRank();
                oi->arrayInfo.elementSize =
                    arrPtr->GetMethodTable()->GetComponentSize();
                oi->arrayInfo.elementType =
                    g_pEEInterface->ArrayGetElementType(arrPtr);

                // If the element type is a value type, then we have
                // an array of value types. Adjust the element's class
                // accordingly.
                if (oi->arrayInfo.elementType == ELEMENT_TYPE_VALUETYPE)
                {
                    // For value class elements, we must pass the
                    // exact class of the elements back to the
                    // Right Side for proper dereferencing.
                    EEClass *cl = arrPtr->GetElementTypeHandle().GetClass();

                    oi->objClassMetadataToken = cl->GetCl();
                    oi->objClassDebuggerModuleToken = (void*) LookupModule(
                                                                cl->GetModule(),
                                                                pAppDomain);

                    EnsureModuleLoadedForInproc(&oi->objClassDebuggerModuleToken, 
                        cl, pAppDomain, iWhich);
                }
                
                LOG((LF_CORDB, LL_INFO10000, "D::GASOI: array info: "
                     "baseOff=%d, lowerOff=%d, upperOff=%d, cnt=%d, rank=%d, "
                     "eleSize=%d, eleType=0x%02x\n",
                     oi->arrayInfo.offsetToArrayBase,
                     oi->arrayInfo.offsetToLowerBounds,
                     oi->arrayInfo.offsetToUpperBounds,
                     oi->arrayInfo.componentCount,
                     oi->arrayInfo.rank,
                     oi->arrayInfo.elementSize,
                     oi->arrayInfo.elementType));
            }
        
            break;
            
        default:
            ASSERT(!"Invalid object type!");
        }
    }
    PAL_EXCEPT_FILTER(FilterAccessViolation, NULL)
    {
        LOG((LF_CORDB, LL_INFO10000,
             "D::GASOI: exception indicated ref is bad.\n"));

        badRef = true;
    }
    PAL_ENDTRY

    if (plainSend) {
        return rcThread->SendIPCReply(iWhich);
    }

    if (fMakeHandle)
    {
        if(badRef || objPtr ==NULL)
        {
            oi->objToken = NULL;
        }
        else
        {
            oi->objToken = g_pEEInterface->GetHandleFromObject(objPtr, fStrongNewRef, pAppDomain);
        }
    }
    else
    {
        LOG((LF_CORDB, LL_INFO1000, "D::GASOI: WON'T create a new, "
            "handle!\n"));
            
        // This implies that we got here through a DB_IPCE_VALIDATE_OBJECT
        // message.
        oi->objToken = objectRefAddress;
    }


    oi->objRefBad = badRef;

    LOG((LF_CORDB, LL_INFO10000, "D::GASOI: sending result.\n"));

    // Send off the data to the right side.
    return rcThread->SendIPCReply(iWhich);
}

//
// GetAndSendClassInfo gets the necessary data for an Class and
// sends it back to the right side.
//
// This method operates in one of two modes - the "send class info"
// mode, and "find me the field desc" mode, which is used by 
// GetAndSendSyncBlockFieldInfo to get a FieldDesc for a specific
// field.  If fldToken is mdFieldDefNil, then we're in 
// the first mode, if not, then we're in the FieldDesc mode.  
//  FIELD DESC MODE: We DON'T send message in the FieldDesc mode.  
//      We indicate success by setting *pFD to nonNULL, failure by
//      setting *pFD to NULL.
//
HRESULT Debugger::GetAndSendClassInfo(DebuggerRCThread* rcThread,
                                      void* classDebuggerModuleToken,
                                      mdTypeDef classMetadataToken,
                                      AppDomain *pAppDomain,
                                      mdFieldDef fldToken,
                                      FieldDesc **pFD, //OUT
                                      IpcTarget iWhich)
{
    LOG((LF_CORDB, LL_INFO10000, "D::GASCI: getting info for 0x%08x 0x%0x8.\n",
         classDebuggerModuleToken, classMetadataToken));

    HRESULT hr = S_OK;

    _ASSERTE( fldToken == mdFieldDefNil || pFD != NULL);

    BOOL fSendClassInfoMode = fldToken == mdFieldDefNil;

#ifdef _DEBUG
    if (!fSendClassInfoMode)
    {
        _ASSERTE(pFD != NULL);
        (*pFD) = NULL;
    }
#endif //_DEBUG

    // Setup the event that we will return the results in
    DebuggerIPCEvent* event= rcThread->GetIPCEventSendBuffer(iWhich);
    InitIPCEvent(event, DB_IPCE_GET_CLASS_INFO_RESULT, 0, pAppDomain);
    
    // Find the class given its module and token. The class must be loaded.
    DebuggerModule *pDebuggerModule = (DebuggerModule*) classDebuggerModuleToken;
    
    EEClass *pClass = g_pEEInterface->FindLoadedClass(pDebuggerModule->m_pRuntimeModule, classMetadataToken);

    // If we can't find the class, return the proper HR to the right side. Note: if the class is not a value class and
    // the class is also not restored, then we must pretend that the class is still not loaded. We are gonna let
    // unrestored value classes slide, though, and special case access to the class's parent below.
    if ((pClass == NULL) || (!pClass->IsValueClass() && !pClass->IsRestored()))
    {
        LOG((LF_CORDB, LL_INFO10000, "D::GASCI: class isn't loaded.\n"));
        
        event->hr = CORDBG_E_CLASS_NOT_LOADED;
        
        if (iWhich == IPC_TARGET_OUTOFPROC && fSendClassInfoMode)
            return rcThread->SendIPCEvent(IPC_TARGET_OUTOFPROC);
        else
            return S_OK;
    }
    
    // Count the instance and static fields for this class.
    unsigned int parentIFCount = 0;

    // Note: don't try to access the parent if this is an unrestored value class. The parent doesn't have any fields to
    // contribute in such a case anyway...
    if (!pClass->IsValueClass() || pClass->IsRestored())
        if (pClass->GetParentClass() != NULL)
            parentIFCount = pClass->GetParentClass()->GetNumInstanceFields();

    unsigned int IFCount = pClass->GetNumInstanceFields() - parentIFCount;
    unsigned int SFCount = pClass->GetNumStaticFields();
    unsigned int totalFields = IFCount + SFCount;
    unsigned int fieldCount = 0;

    event->GetClassInfoResult.isValueClass = (pClass->IsValueClass() != 0);
    event->GetClassInfoResult.objectSize = pClass->GetNumInstanceFieldBytes();

    if (classMetadataToken == COR_GLOBAL_PARENT_TOKEN)
    {
        // The static var base for the global class in a module is really just the Module's base address.
        event->GetClassInfoResult.staticVarBase = pClass->GetModule()->GetPEFile()->GetBase();
    }
    else if (pClass->IsShared())
    {
        // For shared classes, we have to lookup the static var base for the app domain that we're currently working in.
        DomainLocalClass *pLocalClass = pClass->GetDomainLocalClassNoLock(pDebuggerModule->m_pAppDomain);

        if (pLocalClass)
            event->GetClassInfoResult.staticVarBase = pLocalClass->GetStaticSpace();
        else
            event->GetClassInfoResult.staticVarBase = NULL;
    }
    else
    {
        // For normal, non-shared classes, the static var base if just the class's vtable. Note: the class must be
        // restored for its statics to be available!
        if (pClass->IsRestored())
            event->GetClassInfoResult.staticVarBase = pClass->GetVtable();
        else
            event->GetClassInfoResult.staticVarBase = NULL;
    }
    
    event->GetClassInfoResult.instanceVarCount = IFCount;
    event->GetClassInfoResult.staticVarCount = SFCount;
    event->GetClassInfoResult.fieldCount = 0;

    DebuggerIPCE_FieldData *currentFieldData = &(event->GetClassInfoResult.fieldData);
    unsigned int eventSize = (UINT_PTR)currentFieldData - (UINT_PTR)event;
    unsigned int eventMaxSize = CorDBIPC_BUFFER_SIZE;
    
    LOG((LF_CORDB, LL_INFO10000, "D::GASCI: total fields=%d.\n", totalFields));
    
    FieldDescIterator fdIterator(pClass, FieldDescIterator::INSTANCE_FIELDS | FieldDescIterator::STATIC_FIELDS);
    FieldDesc* fd;

    while ((fd = fdIterator.Next()) != NULL)
    {
        if (!fSendClassInfoMode)
        {
            // We're looking for a specific fieldDesc, see if we got it.
            if (fd->GetMemberDef() == fldToken)
            {
                (*pFD) = fd;
                return S_OK;
            }
            else
                continue;
        }
        
        currentFieldData->fldIsStatic = (fd->IsStatic() == TRUE);
        currentFieldData->fldIsPrimitive = (fd->IsPrimitive() == TRUE);
        
        {
            // Otherwise, we'll simply grab the info & send it back.
            
            currentFieldData->fldDebuggerToken = (void*)fd;
            currentFieldData->fldOffset = fd->GetOffset();
            currentFieldData->fldIsTLS = (fd->IsThreadStatic() == TRUE);
            currentFieldData->fldMetadataToken = fd->GetMemberDef();
            currentFieldData->fldIsRVA = (fd->IsRVA() == TRUE);
            currentFieldData->fldIsContextStatic = (fd->IsContextStatic() == TRUE);

            PCCOR_SIGNATURE pSig = NULL;
            DWORD cSig = 0;

            g_pEEInterface->FieldDescGetSig(fd, &pSig, &cSig);
            _ASSERTE(*pSig == IMAGE_CEE_CS_CALLCONV_FIELD);
            ++pSig;
            
            ULONG cb = _skipFunkyModifiersInSignature(pSig);
            pSig = &pSig[cb];
            
            currentFieldData->fldType = (CorElementType) *pSig;
        }
        
        _ASSERTE( currentFieldData->fldType != ELEMENT_TYPE_CMOD_REQD);

        // Bump our counts and pointers for the next event.
        event->GetClassInfoResult.fieldCount++;
        fieldCount++;
        currentFieldData++;
        eventSize += sizeof(DebuggerIPCE_FieldData);

        // If that was the last field that will fit, send the event now and prep the next one.
        if ((eventSize + sizeof(DebuggerIPCE_FieldData)) >= eventMaxSize)
        {
            LOG((LF_CORDB, LL_INFO10000, "D::GASCI: sending a result, fieldCount=%d, totalFields=%d\n",
                 event->GetClassInfoResult.fieldCount, totalFields));

            if (iWhich == IPC_TARGET_OUTOFPROC)
            {
                hr = rcThread->SendIPCEvent(IPC_TARGET_OUTOFPROC);
            }
            else
            {
                DebuggerIPCEvent *newEvent = m_pRCThread->GetIPCEventSendBufferContinuation(event);

                if (newEvent != NULL)
                {
                    InitIPCEvent(newEvent, DB_IPCE_GET_CLASS_INFO_RESULT, 0, pAppDomain);
                    newEvent->GetClassInfoResult.isValueClass = event->GetClassInfoResult.isValueClass;
                    newEvent->GetClassInfoResult.objectSize = event->GetClassInfoResult.objectSize;
                    newEvent->GetClassInfoResult.staticVarBase = event->GetClassInfoResult.staticVarBase;
                    newEvent->GetClassInfoResult.instanceVarCount = event->GetClassInfoResult.instanceVarCount;
                    newEvent->GetClassInfoResult.staticVarCount = event->GetClassInfoResult.staticVarCount;

                    event = newEvent;
                }
                else
                    return E_OUTOFMEMORY;
            }   
            
            event->GetClassInfoResult.fieldCount = 0;
            currentFieldData = &(event->GetClassInfoResult.fieldData);
            eventSize = (UINT_PTR)currentFieldData - (UINT_PTR)event;
        }
    }

    _ASSERTE(!fSendClassInfoMode || 
             fieldCount == totalFields);

    if (fSendClassInfoMode && 
         (event->GetClassInfoResult.fieldCount > 0 || totalFields == 0))
    {
        LOG((LF_CORDB, LL_INFO10000, "D::GASCI: sending final result, fieldCount=%d, totalFields=%d\n",
             event->GetClassInfoResult.fieldCount, totalFields));

        if (iWhich == IPC_TARGET_OUTOFPROC)
            hr = rcThread->SendIPCEvent(IPC_TARGET_OUTOFPROC);
        else
            hr = S_OK;
    }
    
    return hr;
}


//
// GetAndSendClassInfo gets the necessary data for an Class and
// sends it back to the right side.
//
HRESULT Debugger::GetAndSendSpecialStaticInfo(DebuggerRCThread* rcThread,
                                              void *fldDebuggerToken,
                                              void *debuggerThreadToken,
                                              IpcTarget iWhich)
{
    LOG((LF_CORDB, LL_INFO10000, "D::GASSSI: getting info for "
         "0x%08x 0x%0x8.\n", fldDebuggerToken, debuggerThreadToken));

    HRESULT hr = S_OK;

    // Setup the event that we'll be sending the results in.
    DebuggerIPCEvent* event = rcThread->GetIPCEventReceiveBuffer(iWhich);
    InitIPCEvent(event, 
                 DB_IPCE_GET_SPECIAL_STATIC_RESULT, 
                 0, NULL);

    // Find out where the field is living...
    Thread *pRuntimeThread = (Thread*)debuggerThreadToken;
    FieldDesc *pField = (FieldDesc*)fldDebuggerToken;

    if (pField->IsThreadStatic())
    {
        event->GetSpecialStaticResult.fldAddress =
            pRuntimeThread->GetStaticFieldAddrForDebugger(pField);
    }
    else if (pField->IsContextStatic())
    {
        event->GetSpecialStaticResult.fldAddress = Context::GetStaticFieldAddrForDebugger(pRuntimeThread, pField);
    }
    else
    {
        // In case, we have more special cases added. You will never know!
        _ASSERTE(!"NYI");
    }

    // Send off the data to the right side.
    hr = rcThread->SendIPCReply(iWhich);
    
    return hr;
}


//
// GetAndSendJITInfo gets the necessary JIT data for a function and
// sends it back to the right side.
//
HRESULT Debugger::GetAndSendJITInfo(DebuggerRCThread* rcThread,
                                    mdMethodDef funcMetadataToken,
                                    void *funcDebuggerModuleToken,
                                    AppDomain *pAppDomain,
                                    IpcTarget iWhich)
{
    LOG((LF_CORDB, LL_INFO10000, "D::GASJI: getting info for "
         "0x%08x 0x%08x\n", funcMetadataToken, funcDebuggerModuleToken));
    
    unsigned int totalNativeInfos = 0;
    unsigned int argCount = 0;

    HRESULT hr = S_OK;

    DebuggerModule *pDebuggerModule =
        (DebuggerModule*) funcDebuggerModuleToken;
    
    MethodDesc* pFD = g_pEEInterface->LookupMethodDescFromToken(
                                          pDebuggerModule->m_pRuntimeModule,
                                          funcMetadataToken);

    DebuggerJitInfo *pJITInfo = NULL;

    //
    // Find the JIT info for this function.
    //
    if (pFD != NULL)
        pJITInfo = GetJitInfo(pFD, NULL);
    else
        LOG((LF_CORDB, LL_INFO10000, "D::GASJI: no fd found...\n"));

    if ((pJITInfo != NULL) && (pJITInfo->m_jitComplete))
    {
        argCount = GetArgCount(pFD);
        totalNativeInfos = pJITInfo->m_varNativeInfoCount;
    }
    else
    {
        pJITInfo = NULL;
        LOG((LF_CORDB, LL_INFO10000, "D::GASJI: no JIT info found...\n"));
    }
    
    //
    // Prepare the result event.
    //
    DebuggerIPCEvent* event = rcThread->GetIPCEventSendBuffer(iWhich);
    InitIPCEvent(event, 
                 DB_IPCE_GET_JIT_INFO_RESULT, 
                 0,
                 pAppDomain);
    event->GetJITInfoResult.totalNativeInfos = totalNativeInfos;
    event->GetJITInfoResult.argumentCount = argCount;
    event->GetJITInfoResult.nativeInfoCount = 0;
    if (pJITInfo == NULL)
    {
        event->GetJITInfoResult.nVersion = DebuggerJitInfo::DJI_VERSION_INVALID;
    }
    else
    {
        event->GetJITInfoResult.nVersion = pJITInfo->m_nVersion;
    }

    ICorJitInfo::NativeVarInfo *currentNativeInfo =
        &(event->GetJITInfoResult.nativeInfo);
    unsigned int eventSize = (UINT_PTR)currentNativeInfo - (UINT_PTR)event;
    unsigned int eventMaxSize = CorDBIPC_BUFFER_SIZE;

    unsigned int nativeInfoCount = 0;
    
    while (nativeInfoCount < totalNativeInfos)
    {
        *currentNativeInfo = pJITInfo->m_varNativeInfo[nativeInfoCount];

        //
        // Bump our counts and pointers for the next event.
        //
        event->GetJITInfoResult.nativeInfoCount++;
        nativeInfoCount++;
        currentNativeInfo++;
        eventSize += sizeof(*currentNativeInfo);

        //
        // If that was the last field that will fit, send the event now
        // and prep the next one.
        //
        if ((eventSize + sizeof(*currentNativeInfo)) >= eventMaxSize)
        {
            LOG((LF_CORDB, LL_INFO10000, "D::GASJI: sending a result\n"));

            if (iWhich == IPC_TARGET_OUTOFPROC)
            {
                hr = rcThread->SendIPCEvent(IPC_TARGET_OUTOFPROC);
            }
            else
            {
                _ASSERTE( iWhich == IPC_TARGET_INPROC);
                event = rcThread->GetIPCEventSendBufferContinuation(event);
                if (NULL == event)
                    return E_OUTOFMEMORY;

                InitIPCEvent(event, 
                             DB_IPCE_GET_JIT_INFO_RESULT, 
                             0,
                             pAppDomain);
                event->GetJITInfoResult.totalNativeInfos = totalNativeInfos;
                event->GetJITInfoResult.argumentCount = argCount;
            }
            
            event->GetJITInfoResult.nativeInfoCount = 0;
            currentNativeInfo = &(event->GetJITInfoResult.nativeInfo);
            eventSize = (UINT_PTR)currentNativeInfo - (UINT_PTR)event;
        }
    }

    if (((event->GetJITInfoResult.nativeInfoCount > 0) ||
         (totalNativeInfos == 0)) && (iWhich ==IPC_TARGET_OUTOFPROC))
    {
        LOG((LF_CORDB, LL_INFO10000, "D::GASJI: sending final result\n"));
                 
        hr = rcThread->SendIPCEvent(IPC_TARGET_OUTOFPROC);
    }
    
    return hr;
}

//
// GetAndSendTransitionStubInfo figures out if an address is a stub
// address and sends the result back to the right side.
//
void Debugger::GetAndSendTransitionStubInfo(const BYTE *stubAddress, IpcTarget iWhich)
{
    LOG((LF_CORDB, LL_INFO10000, "D::GASTSI: IsTransitionStub. Addr=0x%08x\n", stubAddress));
            
    bool result = false;
    
    PAL_TRY
    {
        // Try to see if this address is for a stub. If the address is
        // completely bogus, then this might fault, so we protect it
        // with SEH.
        result = g_pEEInterface->IsStub(stubAddress);
    }
    PAL_EXCEPT_FILTER(FilterAccessViolation, NULL)
    {
        LOG((LF_CORDB, LL_INFO10000, "D::GASTSI: exception indicated addr is bad.\n"));

        result = false;
    }
    PAL_ENDTRY

    // We control excluding the CLR from the calculation based on a reg key. This lets CLR devs override the check and
    // step through the CLR codebase.
    static DWORD excludeCLR = 0;
    static bool  excludeCLRInited = false;

    if (!excludeCLRInited)
    {
        excludeCLR = REGUTIL::GetConfigDWORD(L"DbgCLRDEV", 0);
        excludeCLRInited = true;
    }


    // This is a synchronous event (reply required)
    DebuggerIPCEvent *event = m_pRCThread->GetIPCEventReceiveBuffer(iWhich);
    InitIPCEvent(event, DB_IPCE_IS_TRANSITION_STUB_RESULT, 0, NULL);
    event->IsTransitionStubResult.isStub = result;
        
    // Send the result
    m_pRCThread->SendIPCReply(iWhich);
}

/*
 * A generic request for a buffer
 *
 * This is a synchronous event (reply required).
 */
HRESULT Debugger::GetAndSendBuffer(DebuggerRCThread* rcThread, ULONG bufSize)
{
    // This is a synchronous event (reply required)
    DebuggerIPCEvent* event = rcThread->GetIPCEventReceiveBuffer(IPC_TARGET_OUTOFPROC);
    InitIPCEvent(event, DB_IPCE_GET_BUFFER_RESULT, 0, NULL);

    // Allocate the buffer
    event->GetBufferResult.pBuffer = new (interopsafe) BYTE[bufSize];

    LOG((LF_CORDB, LL_EVERYTHING, "D::GASB: new'd 0x%x\n", event->GetBufferResult.pBuffer));

    // Check for out of memory error
    if (event->GetBufferResult.pBuffer == NULL)
        event->GetBufferResult.hr = E_OUTOFMEMORY;
    else
    {
        _ASSERTE(m_pMemBlobs != NULL);
        BYTE **ppNextBlob = m_pMemBlobs->Append();
        (*ppNextBlob) = (BYTE *)event->GetBufferResult.pBuffer;
        
        event->GetBufferResult.hr = S_OK;
    }
    
    // Send the result
    return rcThread->SendIPCReply(IPC_TARGET_OUTOFPROC);
}

/*
 * Used to release a previously-requested buffer
 *
 * This is a synchronous event (reply required).
 */
HRESULT Debugger::SendReleaseBuffer(DebuggerRCThread* rcThread, BYTE *pBuffer)
{
    LOG((LF_CORDB,LL_INFO10000, "D::SRB for buffer 0x%x\n", pBuffer));

    // This is a synchronous event (reply required)
    DebuggerIPCEvent* event = rcThread->GetIPCEventReceiveBuffer(IPC_TARGET_OUTOFPROC);
    InitIPCEvent(event, DB_IPCE_RELEASE_BUFFER_RESULT, 0, NULL);

    _ASSERTE(pBuffer != NULL);

    // Free the memory
    ReleaseRemoteBuffer(pBuffer, true);

    // Indicate success in reply
    event->ReleaseBufferResult.hr = S_OK;
    
    // Send the result
    return rcThread->SendIPCReply(IPC_TARGET_OUTOFPROC);
}


//
// Used to delete the buffer previously-requested  by the right side.
// We've factored the code since both the ~Debugger and SendReleaseBuffer
// methods do this.
//
HRESULT Debugger::ReleaseRemoteBuffer(BYTE *pBuffer, bool removeFromBlobList)
{
    LOG((LF_CORDB, LL_EVERYTHING, "D::RRB: Releasing RS-alloc'd buffer 0x%x\n", pBuffer));

    // Remove the buffer from the blob list if necessary.
    if (removeFromBlobList && (m_pMemBlobs != NULL))
    {
        USHORT cBlobs = m_pMemBlobs->Count();
        BYTE **rgpBlobs = m_pMemBlobs->Table();

        for (USHORT i = 0; i < cBlobs; i++)
        {
            if (rgpBlobs[i] == pBuffer)
            {
                m_pMemBlobs->DeleteByIndex(i);
                break;
            }
        }
    }

    // Delete the buffer.
    DeleteInteropSafe(pBuffer);

    return S_OK;
}

//
// UnrecoverableError causes the Left Side to enter a state where no more
// debugging can occur and we leave around enough information for the
// Right Side to tell what happened.
//
void Debugger::UnrecoverableError(HRESULT errorHR,
                                  unsigned int errorCode,
                                  const char *errorFile,
                                  unsigned int errorLine,
                                  bool exitThread)
{
    LOG((LF_CORDB, LL_INFO10,
         "Unrecoverable error: hr=0x%08x, code=%d, file=%s, line=%d\n",
         errorHR, errorCode, errorFile, errorLine));
        
    //
    // Setting this will ensure that not much else happens...
    //
    m_unrecoverableError = TRUE;
    
    //
    // Fill out the control block with the error.
    //
    DebuggerIPCControlBlock *pDCB = m_pRCThread->GetDCB(
        IPC_TARGET_OUTOFPROC); // in-proc will find out when the
            // function fails

    pDCB->m_errorHR = errorHR;
    pDCB->m_errorCode = errorCode;

    //
    // Let an unmanaged debugger know that we're here...
    //
    DebugBreak();
    
    //
    // If we're told to, exit the thread.
    //
    if (exitThread)
    {
        LOG((LF_CORDB, LL_INFO10,
             "Thread exiting due to unrecoverable error.\n"));
        ExitThread(errorHR);
    }
}

//
// Callback for IsThreadAtSafePlace's stack walk.
//
StackWalkAction Debugger::AtSafePlaceStackWalkCallback(CrawlFrame *pCF,
                                                       VOID* data)
{
    bool *atSafePlace = (bool*)data;
    LOG((LF_CORDB, LL_INFO100000, "D:AtSafePlaceStackWalkCallback\n"));

    if (pCF->IsFrameless() && pCF->IsActiveFunc())
    {
        LOG((LF_CORDB, LL_INFO1000000, "D:AtSafePlaceStackWalkCallback, IsFrameLess() and IsActiveFunc()\n"));
        if (g_pEEInterface->CrawlFrameIsGcSafe(pCF))
        {
            LOG((LF_CORDB, LL_INFO1000000, "D:AtSafePlaceStackWalkCallback - TRUE: CrawlFrameIsGcSafe()\n"));
            *atSafePlace = true;
        }
    }
    return SWA_ABORT;
}

//
// Determine, via a quick one frame stack walk, if a given thread is
// in a gc safe place.
//
bool Debugger::IsThreadAtSafePlace(Thread *thread)
{
    bool atSafePlace = false;
    
    // Setup our register display.
    REGDISPLAY rd;
    CONTEXT *context = g_pEEInterface->GetThreadFilterContext(thread);
        
    _ASSERTE(!(g_pEEInterface->GetThreadFilterContext(thread) && ISREDIRECTEDTHREAD(thread)));

    if (context != NULL)
        g_pEEInterface->InitRegDisplay(thread, &rd, context, TRUE);
    else if (ISREDIRECTEDTHREAD(thread))
        thread->GetFrame()->UpdateRegDisplay(&rd);
    else
    {
        CONTEXT ctx;
        g_pEEInterface->InitRegDisplay(thread, &rd, &ctx, FALSE);
    }

    // Do the walk. If it fails, we don't care, because we default
    // atSafePlace to false.
    g_pEEInterface->StackWalkFramesEx(
                                 thread,
                                 &rd,
                                 Debugger::AtSafePlaceStackWalkCallback,
                                 (VOID*)(&atSafePlace),
                                 QUICKUNWIND | HANDLESKIPPEDFRAMES);

#ifdef LOGGING
    if (!atSafePlace)
        LOG((LF_CORDB | LF_GC, LL_INFO1000,
             "Thread 0x%x is not at a safe place.\n",
             thread->GetThreadId()));
#endif    
    
    return atSafePlace;
}

/******************************************************************************
 *
 ******************************************************************************/
void Debugger::GetVarInfo(MethodDesc *       fd,   // [IN] method of interest
                    void *DebuggerVersionToken,    // [IN] which edit version
                    SIZE_T *           cVars,      // [OUT] size of 'vars'
                    const NativeVarInfo **vars     // [OUT] map telling where local vars are stored
                    )
{
    DebuggerJitInfo * ji = (DebuggerJitInfo *)DebuggerVersionToken;

    if (ji == NULL)
    {
        ji = GetJitInfo(fd, NULL);
    }

    _ASSERTE(ji);

    *vars = ji->m_varNativeInfo;
    *cVars = ji->m_varNativeInfoCount;
}

#include "openum.h"




/******************************************************************************
 *
 ******************************************************************************/
bool Debugger::GetILOffsetFromNative (MethodDesc *pFunc, const BYTE *pbAddr,
                                      DWORD nativeOffset, DWORD *ilOffset)
{
    DebuggerJitInfo *jitInfo = 
            g_pDebugger->GetJitInfo(pFunc,
                                    (const BYTE*)pbAddr);

    if (jitInfo != NULL)
    {
        CorDebugMappingResult map;    
        DWORD whichIDontCare;

        *ilOffset = jitInfo->MapNativeOffsetToIL(
                                        nativeOffset,
                                        &map,
                                        &whichIDontCare);

        return true;
    }

    return false;
}

/******************************************************************************
 *
 ******************************************************************************/
DWORD Debugger::GetHelperThreadID(void )
{
    return m_pRCThread->GetDCB(IPC_TARGET_OUTOFPROC)
        ->m_temporaryHelperThreadId;
}

// HRESULT Debugger::DeleteHeadOfList():  Removes the
//  current head of the list, and repl
//
HRESULT Debugger::DeleteHeadOfList( MethodDesc *pFD )
{
  LOG((LF_CORDB,LL_INFO10000,"D:DHOL for %s::%s\n",
            pFD->m_pszDebugClassName,
            pFD->m_pszDebugMethodName));

  LockJITInfoMutex();

  if (m_pJitInfos != NULL && pFD != NULL)
    m_pJitInfos->RemoveJitInfo( pFD);
  
  UnlockJITInfoMutex();
  
  LOG((LF_CORDB,LL_INFO10000,"D:DHOL: Finished removing head of the list"));

  return S_OK;
}

// HRESULT Debugger::InsertAtHeadOfList():  Make sure
//  that there's only one head of the the list of DebuggerJitInfos
//  for the (implicitly) given MethodDesc.
HRESULT 
Debugger::InsertAtHeadOfList( DebuggerJitInfo *dji )
{
    LOG((LF_CORDB,LL_INFO10000,"D:IAHOL: dji:0x%08x\n", dji));

    HRESULT hr = S_OK;

    _ASSERTE(dji != NULL);

    LockJITInfoMutex();

//    CHECK_DJI_TABLE_DEBUGGER;

    hr = CheckInitJitInfoTable();

    if (FAILED(hr))
        return (hr);

    DebuggerJitInfo *djiPrev = m_pJitInfos->GetJitInfo(dji->m_fd);
    LOG((LF_CORDB,LL_INFO10000,"D:IAHOL: current head of dji list:0x%08x\n",djiPrev));
    _ASSERTE(djiPrev == NULL || dji->m_fd == djiPrev->m_fd);
        
    dji->m_nVersion = GetVersionNumber(dji->m_fd);

    if (djiPrev != NULL)
    {
        dji->m_prevJitInfo = djiPrev;
        djiPrev->m_nextJitInfo = dji;
        
        _ASSERTE(dji->m_fd != NULL);
        hr = m_pJitInfos->OverwriteJitInfo(dji->m_fd->GetModule(), 
                                         dji->m_fd->GetMemberDef(), 
                                         dji, 
                                         FALSE);

        LOG((LF_CORDB,LL_INFO10000,"D:IAHOL: DJI version 0x%04x for %s\n", 
            dji->m_nVersion,dji->m_fd->m_pszDebugMethodName));
    }
    else
    {
        hr = m_pJitInfos->AddJitInfo(dji->m_fd, dji, dji->m_nVersion);
    }
#ifdef _DEBUG
    djiPrev = m_pJitInfos->GetJitInfo(dji->m_fd);
    LOG((LF_CORDB,LL_INFO10000,"D:IAHOL: new head of dji list:0x%08x\n",
        djiPrev));
#endif //_DEBUG        
    UnlockJITInfoMutex();

    return hr;
}


// This method sends a log message over to the right side for the debugger to log it.
void Debugger::SendLogMessage(int iLevel, WCHAR *pCategory, int iCategoryLen,
                              WCHAR *pMessage, int iMessageLen)
{
    DebuggerIPCEvent* ipce;
    int iBytesToCopy;
    bool disabled;
    HRESULT hr = S_OK;

    LOG((LF_CORDB, LL_INFO10000, "D::SLM: Sending log message.\n"));

    // Send the message only if the debugger is attached to this appdomain.
    // Note the the debugger may detach at any time, so we'll have to check
    // this again after we get the lock.
    AppDomain *pAppDomain = g_pEEInterface->GetThread()->GetDomain();
    
    if (!pAppDomain->IsDebuggerAttached())
        return;

    disabled = g_pEEInterface->IsPreemptiveGCDisabled();

    if (disabled)
        g_pEEInterface->EnablePreemptiveGC();

    // EnsureDebuggerAttached is going to trigger a bunch of messages going back
    // and forth. If we lock before it, we'll block those messages.
    // If we lock after it, it's _possible_ that the debugger may detach 
    // before we get to the lock.
    
    // If panic message & no debugger is attached, then launch one to attach to us.
    if (iLevel == PanicLevel)
        hr = EnsureDebuggerAttached(g_pEEInterface->GetThread()->GetDomain(),
                                    L"Log message");

    BOOL threadStoreLockOwner = FALSE;

    if (SUCCEEDED(hr))
    {
        // Prevent other Runtime threads from handling events.

        // NOTE: if EnsureDebuggerAttached returned S_FALSE, this means that
        // a debugger was already attached and LockForEventSending should
        // behave as normal.  If there was no debugger attached, then we have
        // a special case where this event is a part of debugger attaching and
        // we've previously sent a sync complete event which means that
        // LockForEventSending will retry until a continue is called - however,
        // with attaching logic the previous continue didn't enable event
        // handling and didn't continue the process - it's waiting for this
        // event to be sent, so we do so even if the process appears to be
        // stopped.

        LockForEventSending(hr == S_OK);

        // It's possible that the debugger dettached while we were waiting
        // for our lock. Check again and abort the event if it did.
        if (pAppDomain->IsDebuggerAttached())
        {
            ipce = m_pRCThread->GetIPCEventSendBuffer(IPC_TARGET_OUTOFPROC);

            // Check if the whole message will fit in one SendBuffer or
            // if we need to send multiple send buffers.
            // (the category string should always fit in the first message.
            _ASSERTE ((iCategoryLen >= 0) && (((iCategoryLen * sizeof (WCHAR)) +
                        (int)(((char*)&ipce->FirstLogMessage.Dummy[0] - 
                        (char*)ipce + 
                        (char*)LOG_MSG_PADDING))) < CorDBIPC_BUFFER_SIZE));

            bool fFirstMsg = true;
            bool fMore = false;
            int iMsgIndex = 0;

            if ((int)((iCategoryLen+iMessageLen) * sizeof (WCHAR)) > 
                (CorDBIPC_BUFFER_SIZE - (int)((char*)&ipce->FirstLogMessage.Dummy[0] -
                                        (char*)ipce +
                                        (char*)LOG_MSG_PADDING)))
            {
                fMore = true;
            }
            do
            {

                if (fFirstMsg)
                {
                    fFirstMsg = false;

                    // Send a LogMessage event to the Right Side
                    InitIPCEvent(ipce, 
                                 DB_IPCE_FIRST_LOG_MESSAGE, 
                                 g_pEEInterface->GetThread()->GetThreadId(),
                                 g_pEEInterface->GetThread()->GetDomain());

                    ipce->FirstLogMessage.fMoreToFollow = fMore;
                    ipce->FirstLogMessage.iLevel = iLevel;
                    ipce->FirstLogMessage.iCategoryLength = iCategoryLen;
                    ipce->FirstLogMessage.iMessageLength = iMessageLen;

                    wcsncpy (&ipce->FirstLogMessage.Dummy[0], pCategory, iCategoryLen);
                    ipce->FirstLogMessage.Dummy [iCategoryLen] = L'\0';

                    // We have already calculated whether or not the message string
                    // will fit in this buffer.
                    if (fMore)
                    {
                        iBytesToCopy = (CorDBIPC_BUFFER_SIZE - (
                                        (int)((char*)&ipce->FirstLogMessage.Dummy[0] - 
                                        (char*)ipce + 
                                        (char*)LOG_MSG_PADDING) 
                                        + (iCategoryLen * sizeof (WCHAR)))) / sizeof (WCHAR);

                        wcsncpy (&ipce->FirstLogMessage.Dummy [iCategoryLen+1], 
                                        pMessage, iBytesToCopy);

                        iMessageLen -= iBytesToCopy;

                        iMsgIndex += iBytesToCopy;
                    }
                    else
                    {
                        wcsncpy (&ipce->FirstLogMessage.Dummy [iCategoryLen+1],
                                        pMessage, iMessageLen);
                    }
                }
                else
                {
                    _ASSERTE (iMessageLen > 0);

                    iBytesToCopy = (CorDBIPC_BUFFER_SIZE - 
                                    (int)((char*)&ipce->ContinuedLogMessage.Dummy[0] - 
                                    (char*)ipce)) / sizeof (WCHAR);

                    if (iBytesToCopy >= iMessageLen)
                    {
                        iBytesToCopy = iMessageLen;
                        fMore = false;
                    }
                    else
                        iMessageLen -= iBytesToCopy;

                    ipce = m_pRCThread->GetIPCEventSendBuffer(IPC_TARGET_OUTOFPROC);
                    InitIPCEvent(ipce, 
                                 DB_IPCE_CONTINUED_LOG_MESSAGE, 
                                 g_pEEInterface->GetThread()->GetThreadId(),
                                 g_pEEInterface->GetThread()->GetDomain());

                    ipce->ContinuedLogMessage.fMoreToFollow = fMore;
                    ipce->ContinuedLogMessage.iMessageLength = iBytesToCopy;

                    wcsncpy (&ipce->ContinuedLogMessage.Dummy[0], &pMessage [iMsgIndex],
                                                iBytesToCopy);
                    iMsgIndex += iBytesToCopy;
                }

                m_pRCThread->SendIPCEvent(IPC_TARGET_OUTOFPROC);


            } while (fMore == true);

            if (iLevel == PanicLevel)
            {
                // Send a user breakpoint event to the Right Side
                DebuggerIPCEvent* ipce = m_pRCThread
                    ->GetIPCEventSendBuffer(IPC_TARGET_OUTOFPROC);
                InitIPCEvent(ipce, 
                             DB_IPCE_USER_BREAKPOINT, 
                             g_pEEInterface->GetThread()->GetThreadId(),
                             g_pEEInterface->GetThread()->GetDomain());

                LOG((LF_CORDB, LL_INFO10000,
                     "D::SLM: sending user breakpoint event.\n"));
                m_pRCThread->SendIPCEvent(IPC_TARGET_OUTOFPROC);
            }   

            threadStoreLockOwner = TrapAllRuntimeThreads(pAppDomain);

            // If we're still syncing for attach, send sync complete now and
            // mark that the debugger has completed attaching.

            if (iLevel == PanicLevel)
                FinishEnsureDebuggerAttached();

        }
        else 
        {
            LOG((LF_CORDB,LL_INFO1000, "D::SLM: Skipping SendIPCEvent because RS detached."));
        }
        
        UnlockFromEventSending();
    }

    BlockAndReleaseTSLIfNecessary(threadStoreLockOwner);
    
    g_pEEInterface->DisablePreemptiveGC();
    
    if (!disabled)
        g_pEEInterface->EnablePreemptiveGC();
}


// This function sends a message to the right side informing it about
// the creation/modification of a LogSwitch
void Debugger::SendLogSwitchSetting(int iLevel, int iReason, 
                                    WCHAR *pLogSwitchName, WCHAR *pParentSwitchName)
{
    LOG((LF_CORDB, LL_INFO1000, "D::SLSS: Sending log switch message switch=%S parent=%S.\n",
        pLogSwitchName, pParentSwitchName));

    // Send the message only if the debugger is attached to this appdomain.
    AppDomain *pAppDomain = g_pEEInterface->GetThread()->GetDomain();

    if (!pAppDomain->IsDebuggerAttached())
        return;

    bool disabled = g_pEEInterface->IsPreemptiveGCDisabled();

    if (disabled)
        g_pEEInterface->EnablePreemptiveGC();

    // Prevent other Runtime threads from handling events.
    BOOL threadStoreLockOwner = FALSE;
    
    LockForEventSending();
    
    if (pAppDomain->IsDebuggerAttached()) 
    {
        DebuggerIPCEvent* ipce = m_pRCThread->GetIPCEventSendBuffer(IPC_TARGET_OUTOFPROC);
        InitIPCEvent(ipce, 
                     DB_IPCE_LOGSWITCH_SET_MESSAGE, 
                     g_pEEInterface->GetThread()->GetThreadId(),
                     g_pEEInterface->GetThread()->GetDomain());

        ipce->LogSwitchSettingMessage.iLevel = iLevel;
        ipce->LogSwitchSettingMessage.iReason = iReason;

        wcscpy (&ipce->LogSwitchSettingMessage.Dummy [0], pLogSwitchName);

        if (pParentSwitchName == NULL)
            pParentSwitchName = L"";

        wcscpy (&ipce->LogSwitchSettingMessage.Dummy [wcslen(pLogSwitchName)+1],
                pParentSwitchName);

        m_pRCThread->SendIPCEvent(IPC_TARGET_OUTOFPROC);

        // Stop all Runtime threads
        threadStoreLockOwner = TrapAllRuntimeThreads(pAppDomain);
    } 
    else 
    {
        LOG((LF_CORDB,LL_INFO1000, "D::SLSS: Skipping SendIPCEvent because RS detached."));
    }

    UnlockFromEventSending();

    BlockAndReleaseTSLIfNecessary(threadStoreLockOwner);
    
    g_pEEInterface->DisablePreemptiveGC();

    if (!disabled)
        g_pEEInterface->EnablePreemptiveGC();
}

/******************************************************************************
 * Add the AppDomain to the list stored in the IPC block.
 ******************************************************************************/
HRESULT Debugger::AddAppDomainToIPC(AppDomain *pAppDomain)
{
    HRESULT hr = S_OK;
    LPCWSTR szName = NULL;

    LOG((LF_CORDB, LL_INFO100, "D::AADTIPC: Executing AADTIPC for AppDomain 0x%08x (0x%x).\n",
        pAppDomain,
        pAppDomain->GetId()));

    _ASSERTE(m_pAppDomainCB->m_iTotalSlots > 0);
    _ASSERTE(m_pAppDomainCB->m_rgListOfAppDomains != NULL);

    // Lock the list
    if (!m_pAppDomainCB->Lock())
        return E_FAIL;
    
    // Get a free entry from the list
    AppDomainInfo *pADInfo = m_pAppDomainCB->GetFreeEntry();

    // Function returns NULL if the list is full and a realloc failed.
    if (!pADInfo)
    {
        hr = E_OUTOFMEMORY;
        goto ErrExit;
    }

    // copy the ID
    pADInfo->m_id = pAppDomain->GetId();

    // Now set the AppDomainName. 
    szName = pAppDomain->GetFriendlyName();
    pADInfo->SetName(szName);

    // Save on to the appdomain pointer
    pADInfo->m_pAppDomain = pAppDomain;

    // bump the used slot count
    m_pAppDomainCB->m_iNumOfUsedSlots++;

ErrExit:
    // UnLock the list
    m_pAppDomainCB->Unlock();

    // Send event to debugger if one is attached.  Don't send the event if a debugger is already attached to
    // the domain, since the debugger could have attached to the process and domain in the time it takes
    // between creating the domain and when we notify the debugger.
    if (m_debuggerAttached && !pAppDomain->IsDebuggerAttached())
        SendCreateAppDomainEvent(pAppDomain, FALSE);
    
    return hr;
}

    
/******************************************************************************
 * Remove the AppDomain from the list stored in the IPC block.
 ******************************************************************************/
HRESULT Debugger::RemoveAppDomainFromIPC (AppDomain *pAppDomain)
{

    HRESULT hr = E_FAIL;

    LOG((LF_CORDB, LL_INFO100, "D::RADFIPC: Executing RADFIPC for AppDomain 0x%08x (0x%x).\n",
        pAppDomain,
        pAppDomain->GetId()));

    // if none of the slots are occupied, then simply return.
    if (m_pAppDomainCB->m_iNumOfUsedSlots == 0)
        return hr;

    // Lock the list
    if (!m_pAppDomainCB->Lock())
        return (E_FAIL);


    // Look for the entry
    AppDomainInfo *pADInfo = m_pAppDomainCB->FindEntry(pAppDomain);

    // Shouldn't be trying to remove an appdomain that was never added
    if (!pADInfo)
    {
        // We'd like to assert this, but there is a small window where we may have
        // called AppDomain::Init (and so it's fair game to call Stop, and hence come here),
        // but not yet published the app domain. 
        // _ASSERTE(!"D::RADFIPC: trying to remove an AppDomain that was never added");
        hr = (E_FAIL);
        goto ErrExit;
    }

    // Release the entry
    m_pAppDomainCB->FreeEntry(pADInfo);
    
ErrExit:
    // UnLock the list
    m_pAppDomainCB->Unlock();

    // send event to debugger if one is attached
    if (m_debuggerAttached)
        SendExitAppDomainEvent(pAppDomain);
    
    return hr;
}

/******************************************************************************
 * Update the AppDomain in the list stored in the IPC block.
 ******************************************************************************/
HRESULT Debugger::UpdateAppDomainEntryInIPC(AppDomain *pAppDomain)
{
    HRESULT hr = S_OK;
    LPCWSTR szName = NULL;

    LOG((LF_CORDB, LL_INFO100,
         "D::UADEIIPC: Executing UpdateAppDomainEntryInIPC ad:0x%x.\n", 
         pAppDomain));

    // if none of the slots are occupied, then simply return.
    if (m_pAppDomainCB->m_iNumOfUsedSlots == 0)
        return (E_FAIL);

    // Lock the list
    if (!m_pAppDomainCB->Lock())
        return (E_FAIL);

    // Look up the info entry
    AppDomainInfo *pADInfo = m_pAppDomainCB->FindEntry(pAppDomain);

    if (!pADInfo)
    {
        hr = E_FAIL;
        goto ErrExit;
    }

    // Update the name only if new name is non-null
    szName = pADInfo->m_pAppDomain->GetFriendlyName();
    pADInfo->SetName(szName);

    LOG((LF_CORDB, LL_INFO100,
         "D::UADEIIPC: New name:%ls (AD:0x%x)\n", pADInfo->m_szAppDomainName,
         pAppDomain));
         
ErrExit:
    // UnLock the list
    m_pAppDomainCB->Unlock();

    return hr;
}

/******************************************************************************
 * When attaching to a process, this is called to enumerate all of the
 * AppDomains currently in the process and communicate that information to the
 * debugger.
 ******************************************************************************/
HRESULT Debugger::IterateAppDomainsForAttach(
    AttachAppDomainEventsEnum EventsToSend, 
    BOOL *fEventSent, BOOL fAttaching)
{
#ifdef LOGGING
    static const char *(ev[]) = {"all", "app domain create", "don't send class events", "only send class events"};
#endif
    LOG((LF_CORDB, LL_INFO100, "EEDII::IADFA: Entered function IterateAppDomainsForAttach() isAttaching:%d Events:%s\n", fAttaching, ev[EventsToSend]));
    HRESULT hr = S_OK;

    int flags = 0;
    switch (EventsToSend)
    {
    case SEND_ALL_EVENTS:
        flags = ATTACH_ALL;
        break;
    case ONLY_SEND_APP_DOMAIN_CREATE_EVENTS:
        flags = 0;
        break;
    case DONT_SEND_CLASS_EVENTS:
        flags = ATTACH_ASSEMBLY_LOAD | ATTACH_MODULE_LOAD;
        break;
    case ONLY_SEND_CLASS_EVENTS:
        flags = ATTACH_CLASS_LOAD;
        break;
    default:
        _ASSERTE(!"unknown enum");
    }

    // Lock the list
    if (!m_pAppDomainCB->Lock())
        return (E_FAIL);

    // Iterate through the app domains
    AppDomainInfo *pADInfo = m_pAppDomainCB->FindFirst();

    while (pADInfo)
    {
        LOG((LF_CORDB, LL_INFO100, "EEDII::IADFA: Iterating over domain %#08x AD:%#08x %ls\n", pADInfo->m_pAppDomain->GetId(), pADInfo->m_pAppDomain, pADInfo->m_szAppDomainName));

        // Send CreateAppDomain events for each app domain
        if (EventsToSend == ONLY_SEND_APP_DOMAIN_CREATE_EVENTS)
        {
            LOG((LF_CORDB, LL_INFO100, "EEDII::IADFA: Sending AppDomain Create Event for 0x%08x\n",pADInfo->m_pAppDomain->GetId()));
            g_pDebugInterface->SendCreateAppDomainEvent(
                pADInfo->m_pAppDomain, fAttaching);

            *fEventSent = TRUE;
        }
        else
        {
            DWORD dwFlags = pADInfo->m_pAppDomain->GetDebuggerAttached();

            if ((dwFlags == AppDomain::DEBUGGER_ATTACHING) ||
                (dwFlags == AppDomain::DEBUGGER_ATTACHING_THREAD && 
                    EventsToSend == ONLY_SEND_CLASS_EVENTS))
            {
                LOG((LF_CORDB, LL_INFO100, "EEDII::IADFA: Mark as attaching thread for 0x%08x\n",pADInfo->m_pAppDomain->GetId()));

                // Send Load events for the assemblies, modules, and/or classes
                // We have to remember if any event needs it's 'synch complete'
                // msg to be sent later.
                *fEventSent = pADInfo->m_pAppDomain->
                    NotifyDebuggerAttach(flags, fAttaching) || *fEventSent;

                pADInfo->m_pAppDomain->SetDebuggerAttached(
                    AppDomain::DEBUGGER_ATTACHING_THREAD);

                hr = S_OK;
            }
            else
            {
                LOG((LF_CORDB, LL_INFO100, "EEDII::IADFA: Doing nothing for 0x%08x\n",pADInfo->m_pAppDomain->GetId()));
            }
        }

        // Get the next appdomain in the list
        pADInfo = m_pAppDomainCB->FindNext(pADInfo);
    }           

    // Unlock the list
    m_pAppDomainCB->Unlock();

    LOG((LF_CORDB, LL_INFO100, "EEDII::IADFA: Exiting function IterateAppDomainsForAttach\n"));
    
    return hr;
}

/******************************************************************************
 * Attach the debugger to a specific appdomain given its id.
 ******************************************************************************/
HRESULT Debugger::AttachDebuggerToAppDomain(ULONG id)
{
    LOG((LF_CORDB, LL_INFO1000, "EEDII:ADTAD: Entered function AttachDebuggerToAppDomain 0x%08x()\n", id));

    HRESULT hr = S_OK;

    // Lock the list
    if (!m_pAppDomainCB->Lock())
        return (E_FAIL);

    // Iterate through the app domains
    AppDomainInfo *pADInfo = m_pAppDomainCB->FindFirst();

    hr = E_FAIL;
    
    while (pADInfo)
    {
        if (pADInfo->m_pAppDomain->GetId() == id)
        {
            LOG((LF_CORDB, LL_INFO1000, "EEDII:ADTAD: Marked AppDomain 0x%08x as attaching\n", id));
            pADInfo->m_pAppDomain->SetDebuggerAttached(AppDomain::DEBUGGER_ATTACHING);
            
            hr = S_OK;  
            break;
        }

        // Get the next appdomain in the list
        pADInfo = m_pAppDomainCB->FindNext(pADInfo);
    }           

    // Unlock the list
    m_pAppDomainCB->Unlock();

    return hr;
}


/******************************************************************************
 * Mark any appdomains that we are in the process of attaching to as attached
 ******************************************************************************/
HRESULT Debugger::MarkAttachingAppDomainsAsAttachedToDebugger(void)
{
    LOG((LF_CORDB, LL_INFO1000, "EEDII:MAADAATD: Entered function MarkAttachingAppDomainsAsAttachedToDebugger\n"));

    HRESULT hr = S_OK;

    // Lock the list
    if (!m_pAppDomainCB->Lock())
        return (E_FAIL);

    // Iterate through the app domains
    AppDomainInfo *pADInfo = m_pAppDomainCB->FindFirst();

    hr = E_FAIL;
    
    while (pADInfo)
    {
        if (pADInfo->m_pAppDomain->GetDebuggerAttached() == AppDomain::DEBUGGER_ATTACHING_THREAD)
        {
            pADInfo->m_pAppDomain->SetDebuggerAttached(AppDomain::DEBUGGER_ATTACHED);

            LOG((LF_CORDB, LL_INFO10000, "EEDII:MAADAATD: AppDomain 0x%08x (0x%x) marked as attached\n",
                pADInfo->m_pAppDomain,
                pADInfo->m_pAppDomain->GetId()));            
        }

        // Get the next appdomain in the list
        pADInfo = m_pAppDomainCB->FindNext(pADInfo);
    }           

    // Unlock the list
    m_pAppDomainCB->Unlock();

    return hr;
}


/******************************************************************************
 * Detach the debugger from a specific appdomain given its id.
 ******************************************************************************/
HRESULT Debugger::DetachDebuggerFromAppDomain(ULONG id, AppDomain **ppAppDomain)
{
    HRESULT hr = S_OK;

    // Lock the list
    if (!m_pAppDomainCB->Lock())
        return (E_FAIL);

    // Iterate through the app domains
    AppDomainInfo *pADInfo = m_pAppDomainCB->FindFirst();

    while (pADInfo)
    {
        if (pADInfo->m_pAppDomain->GetId() == id)
        {
            pADInfo->m_pAppDomain->SetDebuggerAttached(AppDomain::DEBUGGER_NOT_ATTACHED);
            (*ppAppDomain) = pADInfo->m_pAppDomain;
            break;
        }

        // Get the next appdomain in the list
        pADInfo = m_pAppDomainCB->FindNext(pADInfo);
    }           

    // Unlock the list
    m_pAppDomainCB->Unlock();

    return hr;
}


/******************************************************************************
 *
 ******************************************************************************/
HRESULT Debugger::InitAppDomainIPC(void)
{
    HRESULT hr = S_OK;
    DWORD dwStrLen = 0;
    WCHAR szExeName[MAX_PATH];
    int i;
    NAME_EVENT_BUFFER;

    m_pAppDomainCB->m_iNumOfUsedSlots = 0;
    m_pAppDomainCB->m_iLastFreedSlot = 0;
    m_pAppDomainCB->m_iTotalSlots = 0;
    m_pAppDomainCB->m_szProcessName = NULL;
    m_pAppDomainCB->m_fLockInvalid = FALSE;

    // Create a mutex to allow the Left and Right Sides to properly
    // synchronize. The Right Side will spin until m_hMutex is valid,
    // then it will acquire it before accessing the data.
    m_pAppDomainCB->m_hMutex = WszCreateMutex(NULL, TRUE/*held*/, NAME_EVENT(L"pAppDomainCB->m_hMutex"));

    _ASSERTE(m_pAppDomainCB->m_hMutex != NULL);

    if (m_pAppDomainCB->m_hMutex == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }

    m_pAppDomainCB->m_hRemoteMutex = PAL_LocalHandleToRemote(m_pAppDomainCB->m_hMutex);
    if (m_pAppDomainCB->m_hRemoteMutex == INVALID_HANDLE_VALUE)
    {
        LOG((LF_CORDB, LL_INFO100,
             "D::IADIPC: Failed to create local mutex handle.\n"));

        hr = E_OUTOFMEMORY;
        goto exit;
    }

    m_pAppDomainCB->m_iSizeInBytes = INITIAL_APP_DOMAIN_INFO_LIST_SIZE * 
                                                sizeof (AppDomainInfo);

    // Number of slots in AppDomainListElement array
    m_pAppDomainCB->m_rgListOfAppDomains =
        (AppDomainInfo *) malloc(m_pAppDomainCB->m_iSizeInBytes);

    if (m_pAppDomainCB->m_rgListOfAppDomains == NULL)
    {
        LOG((LF_CORDB, LL_INFO100,
             "D::IADIPC: Failed to allocate memory for  AppDomainInfo.\n"));

        hr = E_OUTOFMEMORY;
        goto exit;
    }

    m_pAppDomainCB->m_iTotalSlots = INITIAL_APP_DOMAIN_INFO_LIST_SIZE;

    // Initialize each AppDomainListElement
    for (i = 0; i < INITIAL_APP_DOMAIN_INFO_LIST_SIZE; i++)
    {
        m_pAppDomainCB->m_rgListOfAppDomains[i].FreeEntry();
    }

    // also initialize the process name
    dwStrLen = WszGetModuleFileName(NULL,
                                    szExeName,
                                    MAX_PATH);

    // If we couldn't get the name, then use a nice default.
    if (dwStrLen == 0)
    {
        wcscpy(szExeName, L"<NoProcessName>");
        dwStrLen = wcslen(szExeName);
    }

    // If we got the name, copy it into a buffer. dwStrLen is the
    // count of characters in the name, not including the null
    // terminator.
    m_pAppDomainCB->m_szProcessName = new WCHAR[dwStrLen + 1];
        
    if (m_pAppDomainCB->m_szProcessName == NULL)
    {
        LOG((LF_CORDB, LL_INFO100,
             "D::IADIPC: Failed to allocate memory for ProcessName.\n"));

        hr = E_OUTOFMEMORY;

        goto exit;
    }

    wcscpy(m_pAppDomainCB->m_szProcessName, szExeName);

    // Add 1 to the string length so the Right Side will copy out the
    // null terminator, too.
    m_pAppDomainCB->m_iProcessNameLengthInBytes =
        (dwStrLen + 1) * sizeof(WCHAR);

exit:
    if (m_pAppDomainCB->m_hMutex != NULL)
        m_pAppDomainCB->Unlock();
    
    return hr;
}

/******************************************************************************
 *
 ******************************************************************************/
HRESULT Debugger::TerminateAppDomainIPC(void)
{
    HRESULT hr = S_OK;

    // Lock the list
    if (!m_pAppDomainCB->Lock())
        return (E_FAIL);

    // The shared IPC segment could still be around after the debugger
    // object has been destroyed during process shutdown. So, reset
    // the UsedSlots count to 0 so that any out of process clients
    // enumeratingthe app domains in this process see 0 AppDomains.
    m_pAppDomainCB->m_iNumOfUsedSlots = 0;
    m_pAppDomainCB->m_iTotalSlots = 0;

    // Now delete the memory alloacted for AppDomainInfo  array
    free(m_pAppDomainCB->m_rgListOfAppDomains);
    m_pAppDomainCB->m_rgListOfAppDomains = NULL;

    delete [] m_pAppDomainCB->m_szProcessName;
    m_pAppDomainCB->m_szProcessName = NULL;
    m_pAppDomainCB->m_iProcessNameLengthInBytes = 0;

    // We're done. Set the mutex handle to NULL, release and close the
    // mutex. If the Right Side acquires the mutex, it will verify
    // that the handle is still not NULL. If it is, then it knows it
    // really lost.
    HANDLE m = m_pAppDomainCB->m_hMutex;
    m_pAppDomainCB->m_hMutex = NULL;

    ReleaseMutex(m);
    CloseHandle(m);

    return hr;
}



/* ------------------------------------------------------------------------ *
 * Func Eval stuff 
 * ------------------------------------------------------------------------ */

//
// Small method to setup a DebuggerFuncEvalComplete. We do this because we can't make a new object in
// FuncEvalHijackWorker due to odd C++ rules about SEH.
//
static void SetupDebuggerFuncEvalComplete(Thread *pThread, void *dest)
{
#ifdef _DEBUG
    DebuggerFuncEvalComplete *comp =
#endif
    new (interopsafe) DebuggerFuncEvalComplete(pThread, dest);
    _ASSERTE(comp != NULL);
}

//
// Given a register, return the value.
//
static DWORD GetRegisterValue(DebuggerEval *pDE, CorDebugRegister reg, void *regAddr)
{
    DWORD ret = 0;

    // A non-NULL register address indicates the value of the register was pushed because we're not on the leaf frame,
    // so we use the address of the register given to us instead of the register value in the context.
    if (regAddr != NULL)
    {
        ret = *((DWORD*)regAddr);
    }
    else
    {
        switch (reg)
        {
        case REGISTER_STACK_POINTER:
            ret = (DWORD)GetSP(&pDE->m_context);
            break;
        
        case REGISTER_FRAME_POINTER:
            ret = (DWORD)GetFP(&pDE->m_context);
            break;

#ifdef _X86_    
        case REGISTER_X86_EAX:
            ret = pDE->m_context.Eax;
            break;
        
        case REGISTER_X86_ECX:
            ret = pDE->m_context.Ecx;
            break;
        
        case REGISTER_X86_EDX:
            ret = pDE->m_context.Edx;
            break;
        
        case REGISTER_X86_EBX:
            ret = pDE->m_context.Ebx;
            break;
                        
        case REGISTER_X86_ESI:
            ret = pDE->m_context.Esi;
            break;
        
        case REGISTER_X86_EDI:
            ret = pDE->m_context.Edi;
            break;
#endif // X86
        
        default:
            _ASSERT(!"Invalid register number!");
        
        }
    }

    return ret;
}

//
// Given a register, set its value.
//
static void SetRegisterValue(DebuggerEval *pDE, CorDebugRegister reg, void *regAddr, DWORD newValue)
{
    // A non-NULL register address indicates the value of the register was pushed because we're not on the leaf frame,
    // so we use the address of the register given to us instead of the register value in the context.
    if (regAddr != NULL)
    {
        *((DWORD*)regAddr) = newValue;
    }
    else
    {
        switch (reg)
        {
        case REGISTER_STACK_POINTER:
            SetSP(&pDE->m_context, (LPVOID)newValue);
            break;
        
        case REGISTER_FRAME_POINTER:
            SetFP(&pDE->m_context, (LPVOID)newValue);
            break;

#ifdef _X86_    
        case REGISTER_X86_EAX:
            pDE->m_context.Eax = newValue;
            break;
        
        case REGISTER_X86_ECX:
            pDE->m_context.Ecx = newValue;
            break;
        
        case REGISTER_X86_EDX:
            pDE->m_context.Edx = newValue;
            break;
        
        case REGISTER_X86_EBX:
            pDE->m_context.Ebx = newValue;
            break;
                
        case REGISTER_X86_ESI:
            pDE->m_context.Esi = newValue;
            break;
        
        case REGISTER_X86_EDI:
            pDE->m_context.Edi = newValue;
            break;
#endif // _X86_
        
        default:
            _ASSERT(!"Invalid register number!");
        
        }
    }
}

//
// Given info about an argument, place its value on the stack, even if
// enregistered. Homes enregistered byrefs into either the
// PrimitiveByRefArg or ObjectRefByRefArg arrays if necessary.
// argSigType is the type for this argument described in signature.
//
static void GetArgValue(DebuggerEval *pDE,
                        DebuggerIPCE_FuncEvalArgData *pFEAD,
                        bool isByRef,
                        bool fNeedBoxOrUnbox,
                        TypeHandle argTH,
                        CorElementType byrefArgSigType,
                        ARG_SLOT *pStack,
                        INT64 *pPrimitiveArg,
                        OBJECTREF *pObjectRefArg,
                        CorElementType argSigType)
{
    THROWSCOMPLUSEXCEPTION();

    switch (pFEAD->argType)
    {
    case ELEMENT_TYPE_I8:
    case ELEMENT_TYPE_U8:
    case ELEMENT_TYPE_R8:
        _ASSERTE((argSigType == ELEMENT_TYPE_I8) ||
                 (argSigType == ELEMENT_TYPE_U8) ||
                 (argSigType == ELEMENT_TYPE_R8));

        // 64bit values
        if (pFEAD->argAddr != NULL)
        {
            if (!isByRef)
                *((INT64*)pStack) = *(INT64*)(pFEAD->argAddr);
            else
                *pStack = PtrToArgSlot(pFEAD->argAddr);
        }
        else if (pFEAD->argIsLiteral)
        {
            _ASSERTE(sizeof(pFEAD->argLiteralData) >= sizeof(INT64));

            // If this is a literal arg, then we just copy the data onto the stack.
            if (!isByRef)
            {
                memcpy(pStack, pFEAD->argLiteralData, sizeof(INT64));
            }
            else
            {
                // If this is a byref literal arg, then we copy the data into the primitive arg array as if this were an
                // enregistered value.
                *pStack = PtrToArgSlot(pPrimitiveArg);
                INT64 v = 0;
                memcpy(&v, pFEAD->argLiteralData, sizeof(v));
                *pPrimitiveArg = v;
            }
        }
        else
        {
            // RAK_REG is the only 4 byte type, all others are 8 byte types.
            _ASSERTE(pFEAD->argHome.kind != RAK_REG);

            INT64 bigVal = 0;
            DWORD *pHigh = (DWORD*)(&bigVal);
            DWORD *pLow  = pHigh + 1;

            switch (pFEAD->argHome.kind)
            {
            case RAK_REGREG:
                *pHigh = GetRegisterValue(pDE, pFEAD->argHome.u.reg2, pFEAD->argHome.u.reg2Addr);
                *pLow = GetRegisterValue(pDE, pFEAD->argHome.reg1, pFEAD->argHome.reg1Addr);
                break;
                
            case RAK_MEMREG:
                *pHigh = GetRegisterValue(pDE, pFEAD->argHome.reg1, pFEAD->argHome.reg1Addr);
                *pLow = *((DWORD*)pFEAD->argHome.addr);
                break;

            case RAK_REGMEM:
                *pHigh = *((DWORD*)pFEAD->argHome.addr);
                *pLow = GetRegisterValue(pDE, pFEAD->argHome.reg1, pFEAD->argHome.reg1Addr);
                break;

            default:
                break;
            }

            if (!isByRef)
                *((INT64*)pStack) = bigVal;
            else
            {
                *pStack = PtrToArgSlot(pPrimitiveArg);
                *pPrimitiveArg = bigVal;
            }
        }
        break;
        
    case ELEMENT_TYPE_VALUETYPE:
        {
            DWORD       v;
            LPVOID      pAddr = NULL;
            if (pFEAD->argAddr != NULL)
            {
                pAddr = pFEAD->argAddr;
            }
            else if (pFEAD->argHome.kind == RAK_REG)
            {
                // Simply grab the value out of the proper register.
                v = GetRegisterValue(pDE, pFEAD->argHome.reg1, pFEAD->argHome.reg1Addr);
                pAddr = &v;
            }
            else
            {
                COMPlusThrow(kArgumentNullException, L"ArgumentNull_Generic");
            }

            _ASSERTE(pAddr);

            // Grab the class of this value type.
            EEClass *pBase = argTH.GetClass();

            if (!isByRef && !fNeedBoxOrUnbox)
            {
                _ASSERTE(pBase);
                unsigned size = pBase->GetNumInstanceFieldBytes();
                if (size <= sizeof(ARG_SLOT))
                    CopyValueClassUnchecked(ArgSlotEndianessFixup(pStack, sizeof(LPVOID)), pAddr, pBase->GetMethodTable());
                else {
                    *pStack = PtrToArgSlot(pAddr);
                }
            }
            else
            {
                if (fNeedBoxOrUnbox)
                {
                    // Grab the class of this value type.
                    DebuggerModule *pDebuggerModule = (DebuggerModule*) pFEAD->GetClassInfo.classDebuggerModuleToken;

                    EEClass *pClass = g_pEEInterface->FindLoadedClass(pDebuggerModule->m_pRuntimeModule,
                                                                      pFEAD->GetClassInfo.classMetadataToken);
                    MethodTable * pMT = pClass->GetMethodTable();
                    // We have to keep byref values in a seperate array that is GCPROTECT'd.
                    *pObjectRefArg = pMT->Box(pAddr, TRUE);
                    *pStack = ObjToArgSlot(*pObjectRefArg);
                }
                else
                {
                    if (pFEAD->argAddr)
                        *pStack = PtrToArgSlot(pAddr);
                    else
                    {
                        // The argument is the address of where we're holding the primitive in the PrimitiveArg array. We
                        // stick the real value from the register into the PrimitiveArg array.
                        *pStack = PtrToArgSlot(pPrimitiveArg);
                        *pPrimitiveArg = (INT64)v;
                    }
                }
            }
        }
        break;
    
    default:
        // 32bit values

        if (pFEAD->argAddr != NULL)
        {
            if (!isByRef)
                if (pFEAD->argRefsInHandles)
                {
                    OBJECTHANDLE oh = *((OBJECTHANDLE*)(pFEAD->argAddr));
                    *pStack = PtrToArgSlot(g_pEEInterface->GetObjectFromHandle(oh));
                }
                else
                    *pStack = *(DWORD*)pFEAD->argAddr; 
            else
                if (pFEAD->argRefsInHandles)
                {
                    *pStack = *(DWORD*)pFEAD->argAddr;
                }
                else
                {
                    // We have a 32bit parameter, but if we're passing it byref to a function that's expecting a 64bit
                    // param then we need to copy the 32bit param to the PrimitiveArray and pass the address of its
                    // location in the PrimitiveArray. If we don't do this, then we'll be bashing memory right next to
                    // the 32bit value as the function being called acts upon a 64bit value.
                    if ((byrefArgSigType == ELEMENT_TYPE_I8) ||
                        (byrefArgSigType == ELEMENT_TYPE_U8) ||
                        (byrefArgSigType == ELEMENT_TYPE_R8))
                    {
                        *pStack = PtrToArgSlot(pPrimitiveArg);
                        *pPrimitiveArg = (INT64)(*(INT32*)pFEAD->argAddr);
                    }
                    else
                        *pStack = PtrToArgSlot(pFEAD->argAddr);
                }
        }
        else if (pFEAD->argIsLiteral)
        {
            _ASSERTE(sizeof(pFEAD->argLiteralData) >= sizeof(INT32));

            // All literal args are passed as 32 bit values.
            // However, the called function may expect a larger/smaller value.
            // So we convert the value to the right type.

            _ASSERTE(
                ((argSigType>=ELEMENT_TYPE_BOOLEAN) && (argSigType<=ELEMENT_TYPE_R8)) ||
                (argSigType == ELEMENT_TYPE_CLASS) ||
                (isByRef && (((byrefArgSigType>=ELEMENT_TYPE_BOOLEAN) && (byrefArgSigType<=ELEMENT_TYPE_R8)) ||
                             (byrefArgSigType == ELEMENT_TYPE_CLASS))));

            INT32 inval = *(INT32*)pFEAD->argLiteralData;
            BYTE *outptr = NULL;

            if (isByRef) {
                outptr = (BYTE*)pPrimitiveArg;
                *pStack = PtrToArgSlot(outptr);
            }
            else
                outptr = (BYTE*)pStack;

            switch (isByRef ? byrefArgSigType : argSigType)
            {
            case ELEMENT_TYPE_BOOLEAN:
                if (isByRef) {
                    *(BYTE*)outptr = (BYTE)!!inval;
                }
                else {
                    *(ARG_SLOT*)outptr = (BYTE)!!inval;
                }
                break;
            case ELEMENT_TYPE_I1:
                if (isByRef) {
                    *(INT8*)outptr = (INT8)inval;
                }
                else {
                    *(ARG_SLOT*)outptr = (INT8)inval;
                }
                break;
            case ELEMENT_TYPE_U1:
                if (isByRef) {
                    *(UINT8*)outptr = (UINT8)inval;
                }
                else {
                    *(ARG_SLOT*)outptr = (UINT8)inval;
                }
                break;
            case ELEMENT_TYPE_I2:
                if (isByRef) {
                    *(INT16*)outptr = (INT16)inval;
                }
                else {
                    *(ARG_SLOT*)outptr = (INT16)inval;
                }
                break;
            case ELEMENT_TYPE_U2:
            case ELEMENT_TYPE_CHAR:
                if (isByRef) {
                    *(UINT16*)outptr = (UINT16)inval;
                }
                else {
                    *(ARG_SLOT*)outptr = (UINT16)inval;
                }
                break;
            case ELEMENT_TYPE_I4:
                if (isByRef) {
                    *(int*)outptr = (int)inval;
                }
                else {
                    *(ARG_SLOT*)outptr = (int)inval;
                }
                break;
            case ELEMENT_TYPE_U4:
            case ELEMENT_TYPE_R4:
                if (isByRef) {
                    *(unsigned*)outptr = (unsigned)inval;
                }
                else {
                    *(ARG_SLOT*)outptr = (unsigned)inval;
                }
                break;
            case ELEMENT_TYPE_I8:
            case ELEMENT_TYPE_U8:
            case ELEMENT_TYPE_R8:
                if (isByRef) {
                    *(INT64*)outptr = (INT64)inval;
                }
                else {
                    *(ARG_SLOT*)outptr = (INT64)inval;
                }
                break;
            case ELEMENT_TYPE_CLASS:
                if (isByRef) {
                    *(LPVOID*)outptr = (LPVOID)inval;
                }
                else {
                    *(ARG_SLOT*)outptr = PtrToArgSlot(inval);
                }
                break;
            default:
                // This should never happen
                _ASSERTE(false);
            }
        }
        else
        {
            // RAK_REG is the only valid 4 byte type.
            _ASSERTE(pFEAD->argHome.kind == RAK_REG);

            // Simply grab the value out of the proper register.
            DWORD v = GetRegisterValue(pDE, pFEAD->argHome.reg1, pFEAD->argHome.reg1Addr);

            if (!isByRef)
                *pStack = v;
            else
            {
                // Do we have a something that needs to be GC protected?
                if (pFEAD->argType == ELEMENT_TYPE_CLASS)
                {
                    // We have to keep byref values in a seperate array that is GCPROTECT'd.
                    *pStack = PtrToArgSlot(pObjectRefArg);
                    *pObjectRefArg = ArgSlotToObj((INT64)v);
                }
                else
                {
                    // The argument is the address of where we're holding the primitive in the PrimitiveArg array. We
                    // stick the real value from the register into the PrimitiveArg array.
                    *pStack = PtrToArgSlot(pPrimitiveArg);
                    *pPrimitiveArg = (INT64)v;
                }
            }
        }

        // If we need to unbox, then unbox the arg now.
        if (fNeedBoxOrUnbox)
        {
            if (!isByRef)
            {
                // function expects valuetype, argument received is class or object

                // Take the ObjectRef off the stack.
                ARG_SLOT oi1 = *pStack;
                if (oi1 == 0)
                    COMPlusThrow(kArgumentException, L"ArgumentNull_Obj");
                OBJECTREF o1 = ArgSlotToObj(oi1);

                _ASSERTE(o1->GetClass()->IsValueClass());

                // Unbox the little fella to get a pointer to the raw data.
                void *pData = o1->UnBox();

                // Get its size to make sure it fits in an ARG_SLOT
                unsigned size = o1->GetClass()->GetNumInstanceFieldBytes();
            
                if (size <= sizeof(ARG_SLOT)) {
                    // Its not ByRef, so we need to copy the value class onto the ARG_SLOT.
                    CopyValueClassUnchecked(ArgSlotEndianessFixup(pStack, sizeof(LPVOID)), pData, o1->GetMethodTable());
                }
                else {
                    // Store pointer to the space in the ARG_SLOT
                    *pStack = PtrToArgSlot(pData);
                }
            }
            else
            {
                // Function expects byref valuetype, argument received is byref class.

                // Grab the ObjectRef off the stack via the pointer on the stack. Note: the stack has a pointer to the
                // ObjectRef since the arg was specified as byref.
                OBJECTREF* op1 = (OBJECTREF*)ArgSlotToPtr(*pStack);
                if (op1 == NULL || (*op1) == NULL)
                    COMPlusThrow(kArgumentException, L"ArgumentNull_Obj");

                OBJECTREF o1 = *op1;
                _ASSERTE(o1->GetClass()->IsValueClass());

                // Unbox the little fella to get a pointer to the raw data.
                void *pData = o1->UnBox();
            
                // If it is ByRef, then we just replace the ObjectRef with a pointer to the data.
                *pStack = PtrToArgSlot(pData);
            }
        }

        // Validate any objectrefs that are supposed to be on the stack.
        if (!fNeedBoxOrUnbox)
        {
			Object *objPtr;
			if (!isByRef)
			{
                if ((argSigType == ELEMENT_TYPE_CLASS) ||
                    (argSigType == ELEMENT_TYPE_OBJECT) ||
                    (argSigType == ELEMENT_TYPE_STRING) ||
                    (argSigType == ELEMENT_TYPE_SZARRAY) || 
                    (argSigType == ELEMENT_TYPE_ARRAY)) 
                {
				    // validate the integrity of the object
				    objPtr = (Object*)ArgSlotToPtr(*pStack);
                    if (FAILED(ValidateObject(objPtr)))
                        COMPlusThrow(kArgumentException, L"Argument_BadObjRef");
                }
			}
			else
			{
                _ASSERTE(argSigType == ELEMENT_TYPE_BYREF);
                if ((byrefArgSigType == ELEMENT_TYPE_CLASS) ||
                    (byrefArgSigType == ELEMENT_TYPE_OBJECT) ||
                    (byrefArgSigType == ELEMENT_TYPE_STRING) ||
                    (byrefArgSigType == ELEMENT_TYPE_SZARRAY) || 
                    (byrefArgSigType == ELEMENT_TYPE_ARRAY)) 
                {
				    objPtr = *(Object**)(ArgSlotToPtr(*pStack));
                    if (FAILED(ValidateObject(objPtr)))
                        COMPlusThrow(kArgumentException, L"Argument_BadObjRef");
                }
			}
        }
    }
}

//
// Given info about a byref argument, retrieve the current value from
// either the PrimitiveByRefArg or the ObjectRefByRefArg arrays and
// place it back into the proper register.
//
static void SetByRefArgValue(DebuggerEval *pDE,
                             DebuggerIPCE_FuncEvalArgData *pFEAD,
                             CorElementType byrefArgSigType,
                             INT64 primitiveByRefArg,
                             OBJECTREF objectRefByRegArg)
{
    switch (pFEAD->argType)
    {
    case ELEMENT_TYPE_I8:
    case ELEMENT_TYPE_U8:
    case ELEMENT_TYPE_R8:
        // 64bit values
        {
            if (pFEAD->argIsLiteral)
            {
                // If this was a literal arg, then copy the updated primitive back into the literal.
                memcpy(pFEAD->argLiteralData, &primitiveByRefArg, sizeof(pFEAD->argLiteralData));
            }
            else if (pFEAD->argAddr != NULL)
            {
                // Don't copy 64bit values back if the value wasn't enregistered...
                return;
            }
            else
            {
                // RAK_REG is the only 4 byte type, all others are 8 byte types.
                _ASSERTE(pFEAD->argHome.kind != RAK_REG);

                DWORD *pHigh = (DWORD*)(&primitiveByRefArg);
                DWORD *pLow  = pHigh + 1;

                switch (pFEAD->argHome.kind)
                {
                case RAK_REGREG:
                    SetRegisterValue(pDE, pFEAD->argHome.u.reg2, pFEAD->argHome.u.reg2Addr, *pHigh);
                    SetRegisterValue(pDE, pFEAD->argHome.reg1, pFEAD->argHome.reg1Addr, *pLow);
                    break;
                
                case RAK_MEMREG:
                    SetRegisterValue(pDE, pFEAD->argHome.reg1, pFEAD->argHome.reg1Addr, *pHigh);
                    *((DWORD*)pFEAD->argHome.addr) = *pLow;
                    break;

                case RAK_REGMEM:
                    *((DWORD*)pFEAD->argHome.addr) = *pHigh;
                    SetRegisterValue(pDE, pFEAD->argHome.reg1, pFEAD->argHome.reg1Addr, *pLow);
                    break;

                default:
                    break;
                }
            }
        }
        break;
        
    default:
        // 32bit values
        {
            if (pFEAD->argIsLiteral)
            {
                // If this was a literal arg, then copy the updated primitive back into the literal.
                memcpy(pFEAD->argLiteralData, &primitiveByRefArg, sizeof(pFEAD->argLiteralData));
            }
            else if (pFEAD->argAddr == NULL)
            {
                // If the 32bit value is enregistered, copy it back to the proper regs.
                
                // RAK_REG is the only valid 4 byte type.
                _ASSERTE(pFEAD->argHome.kind == RAK_REG);

                // Shove the result back into the proper register.
                if (pFEAD->argType == ELEMENT_TYPE_CLASS)
                    SetRegisterValue(pDE, pFEAD->argHome.reg1, pFEAD->argHome.reg1Addr, (DWORD)ObjToArgSlot(objectRefByRegArg));
                else
                    SetRegisterValue(pDE, pFEAD->argHome.reg1, pFEAD->argHome.reg1Addr, (DWORD)primitiveByRefArg);
            }
            else
            {
                // If the value wasn't enregistered, then we still need to check to see if we need to put a 32bit value
                // back from where we may have moved it into the primitive array. (Right now we only do this when you
                // pass a 32bit value as a byref when a 64bit byref value was expected.
                if ((byrefArgSigType == ELEMENT_TYPE_I8) ||
                    (byrefArgSigType == ELEMENT_TYPE_U8) ||
                    (byrefArgSigType == ELEMENT_TYPE_R8))
                {
                    *(INT32*)pFEAD->argAddr = (INT32)primitiveByRefArg;
                }
            }
        }
    }
}

//
// Perform the bulk of the work of a function evaluation. Sets up
// arguments, places the call, and process any changed byrefs and the
// return value, if any.
//
static void DoNormalFuncEval(DebuggerEval *pDE)
{
    THROWSCOMPLUSEXCEPTION();


    // We'll need to know if this is a static method or not.
    BOOL staticMethod = pDE->m_md->IsStatic();

    // Grab the signature of the method we're working on.
    MetaSig mSig(pDE->m_md->GetSig(), pDE->m_md->GetModule());
    
    BYTE callingconvention = mSig.GetCallingConvention();
    if (!isCallConv(callingconvention, IMAGE_CEE_CS_CALLCONV_DEFAULT))
    {
        // We don't support calling vararg!
        COMPlusThrow(kArgumentException, L"Argument_CORDBBadVarArgCallConv");
    }

    _ASSERTE((pDE->m_evalType == DB_IPCE_FET_NORMAL) || !staticMethod);

    // If necessary, create a new object.
    OBJECTREF newObj = NULL;
    GCPROTECT_BEGIN(newObj);

    SIZE_T allocArgCnt = 0;
    
    if (pDE->m_evalType == DB_IPCE_FET_NEW_OBJECT)
    {
        newObj = AllocateObject(pDE->m_md->GetMethodTable());

        // Note: we account for an extra argument in the count passed
        // in. We use this to increase the space allocated for args,
        // and we use it to control the number of args copied into
        // those arrays below. Note: stackSize already includes space
        // for this.
        allocArgCnt = pDE->m_argCount + 1;
    }
    else
        allocArgCnt = pDE->m_argCount;
    
    // Validate the argument count with mSig.
    if (allocArgCnt != (mSig.NumFixedArgs() + (staticMethod ? 0 : 1)))
        COMPlusThrow(kTargetParameterCountException, L"Arg_ParmCnt");

    // If function has return buffer, need extra arg slot
    if (mSig.HasRetBuffArg())
        allocArgCnt++;

    // Allocate space for arguments
    ARG_SLOT *pArguments = (ARG_SLOT*)_alloca(sizeof(ARG_SLOT)*allocArgCnt);

    LOG((LF_CORDB, LL_INFO100000,
         "Func eval for %s::%s: allocArgCnt=%d\n",
         pDE->m_md->m_pszDebugClassName,
         pDE->m_md->m_pszDebugMethodName,
         allocArgCnt));

    // An array to hold primitive args for the byref case. If there is
    // an enregistered primitive, we'll copy it to this array and
    // place a ptr to it onto the stack.
    INT64 *pPrimitiveArgs = (INT64*)_alloca(sizeof(INT64) * allocArgCnt);

    // An array to hold object ref args. This is both for the byref
    // case, just like for the pPrimitiveArgs, and as a holding area
    // while we're building the stack. This array is protected from
    // GC's.
    OBJECTREF *pObjectRefArgs = 
        (OBJECTREF*)_alloca(sizeof(OBJECTREF) * allocArgCnt);
    memset(pObjectRefArgs, 0, sizeof(OBJECTREF) * allocArgCnt);
    GCPROTECT_ARRAY_BEGIN(*pObjectRefArgs, allocArgCnt);

    // Start at the first argument
    unsigned curr = 0;

    // Special handling for functions that return value classes.
    EEClass *pRetClass = NULL;
    BYTE    *pRetValueClass = NULL;
    bool    hasHiddenParam = false;
    
    if (mSig.HasRetBuffArg())
    {
        hasHiddenParam = true;
        pRetClass = mSig.GetRetTypeHandle().GetClass();
        _ASSERTE(pRetClass->IsValueClass());

        pRetValueClass =
            (BYTE*)_alloca(pRetClass->GetAlignedNumInstanceFieldBytes());
        memset(pRetValueClass, 0,
               pRetClass->GetAlignedNumInstanceFieldBytes());

        pArguments[curr++] = PtrToArgSlot(pRetValueClass);
    }
    else if (mSig.GetReturnType() == ELEMENT_TYPE_VALUETYPE && mSig.GetReturnType() != mSig.GetReturnTypeNormalized())
    {
        // This is the case where return type is really a VALUETYPE but our calling convention is
        // treating it as primitive. We just need to remember the pretValueClass so that we will box it properly
        // on our way out.
        //
        pRetClass = mSig.GetRetTypeHandle().GetClass();
        _ASSERTE(pRetClass->IsValueClass());
    }
    
    DebuggerIPCE_FuncEvalArgData *argData =
        (DebuggerIPCE_FuncEvalArgData*) pDE->m_argData;

    if (pDE->m_argCount > 0)
    {
        // For non-static methods, 'this' is always the first arg in
        // the array.
        // For static methods, there is no 'this' in the given arg
        // list (indexed by i.) This is also true when creating a new
        // object.
        if (!staticMethod && (pDE->m_evalType != DB_IPCE_FET_NEW_OBJECT))
            curr += 1;

        bool fNeedBoxOrUnbox;
        
        for (;curr < allocArgCnt;curr++)
        {
            DebuggerIPCE_FuncEvalArgData *pFEAD = &argData[mSig.HasRetBuffArg() ? curr-1 : curr];
            
            // Move to the next arg in the signature.
            CorElementType argSigType = mSig.NextArgNormalized();
            _ASSERTE(argSigType != ELEMENT_TYPE_END);

            // If this arg is a byref arg, then we'll need to know what type we're referencing for later...
            EEClass *byrefClass = NULL;
            CorElementType byrefArgSigType = ELEMENT_TYPE_END;

            if (argSigType == ELEMENT_TYPE_BYREF)
                byrefArgSigType = mSig.GetByRefType(&byrefClass);

            LOG((LF_CORDB, LL_INFO100000,
                "curr=%d: argSigType=0x%x, byrefArgSigType=0x%0x, inType=0x%0x\n",
                 curr, argSigType, byrefArgSigType, pFEAD->argType));

            // If the sig says class but we've got a value class parameter, then remember that we need to box it.  If
            // the sig says value class, but we've got a boxed value class, then remember that we need to unbox it.
            fNeedBoxOrUnbox = ((argSigType == ELEMENT_TYPE_CLASS) && (pFEAD->argType == ELEMENT_TYPE_VALUETYPE)) ||
                ((argSigType == ELEMENT_TYPE_VALUETYPE) && ((pFEAD->argType == ELEMENT_TYPE_CLASS) || (pFEAD->argType == ELEMENT_TYPE_OBJECT)) ||
                // This is when method signature is expecting a BYREF ValueType, yet we recieve the boxed valuetype's handle. 
                (pFEAD->argAddr && pFEAD->argType == ELEMENT_TYPE_CLASS && argSigType == ELEMENT_TYPE_BYREF && byrefArgSigType == ELEMENT_TYPE_VALUETYPE));

            GetArgValue(pDE,
                        pFEAD,
                        argSigType == ELEMENT_TYPE_BYREF,
                        fNeedBoxOrUnbox,
                        mSig.GetTypeHandle(),
                        byrefArgSigType,
                        &pArguments[curr],
                        &pPrimitiveArgs[curr],
                        &pObjectRefArgs[curr],
                        argSigType);
        }

        // Place 'this' first in the array for non-static methods.
        if (!staticMethod && (pDE->m_evalType != DB_IPCE_FET_NEW_OBJECT))
        {
            TypeHandle dummyTH;
            bool isByRef = false;
            fNeedBoxOrUnbox = false;

            // We had better have an object for a 'this' argument!
            CorElementType et = argData[0].argType;

            if (!((et == ELEMENT_TYPE_CLASS) || (et == ELEMENT_TYPE_STRING) || (et == ELEMENT_TYPE_OBJECT) ||
                  (et == ELEMENT_TYPE_VALUETYPE) || (et == ELEMENT_TYPE_SZARRAY) || (et == ELEMENT_TYPE_ARRAY)))
                COMPlusThrow(kArgumentOutOfRangeException, L"ArgumentOutOfRange_Enum");

            if (pDE->m_md->GetClass()->IsValueClass())
            {
                // For value classes, the 'this' parameter is always passed by reference.
                isByRef = true;

                // Remember if we need to unbox this parameter, though
                if ((et == ELEMENT_TYPE_CLASS) || (et == ELEMENT_TYPE_OBJECT))
                    fNeedBoxOrUnbox = true;
            }
            else if (et == ELEMENT_TYPE_VALUETYPE)
            {
                // When the method that we invoking is defined on non value type and we receive the ValueType as input,
                // we are calling methods on System.Object. In this case, we need to box the input ValueType.
                fNeedBoxOrUnbox = true;
            }

            GetArgValue(pDE,
                        &argData[0],
                        isByRef,
                        fNeedBoxOrUnbox,                          
                        dummyTH,
                        ELEMENT_TYPE_CLASS,
                        &pArguments[0],
                        &pPrimitiveArgs[0],
                        &pObjectRefArgs[0],
                        ELEMENT_TYPE_OBJECT);

            LOG((LF_CORDB, LL_INFO100000,
                "this = 0x%08x\n", ArgSlotToPtr(pArguments[0])));

            // We need to check 'this' for a null ref ourselves... NOTE: only do this if we put an object reference on
            // the stack. If we put a byref for a value type, then we don't need to do this!
            if (!isByRef)
            {
                // The this pointer is not a unboxed value type. 

                ARG_SLOT oi1 = pArguments[0];   
                OBJECTREF o1 = ArgSlotToObj(oi1);

                if (o1 == NULL)
                    COMPlusThrow(kNullReferenceException, L"NullReference_This");

                // For interface method, we have already done the check early on.
                if (!pDE->m_md->IsInterface())
                {
                    // We also need to make sure that the method that we are invoking is either defined on this object or the direct/indirect
                    // base objects.
                    Object  *objPtr = OBJECTREFToObject(o1);
                    MethodTable *pMT = objPtr->GetMethodTable();
                    if (!pMT->IsArray() && !pMT->IsTransparentProxyType())
                    {
                        TypeHandle thFrom = TypeHandle(pMT);
                        TypeHandle thTarget = TypeHandle(pDE->m_md->GetMethodTable());
                        if (!thFrom.CanCastTo(thTarget))
                            COMPlusThrow(kArgumentException, L"Argument_CORDBBadMethod");

                    }
                }
            }
        }
    }

    // If this is a new object op, then we need to fill in the 0'th
    // arg slot with the 'this' ptr.
    if (pDE->m_evalType == DB_IPCE_FET_NEW_OBJECT)
    {
        // If we are invoking a function on a value class, but we have a boxed VC for 'this', then go ahead and unbox it
        // and leave a ref to the vc on the stack as 'this'.
        if (pDE->m_md->GetClass()->IsValueClass())
        {
            ARG_SLOT oi1 = pArguments[0];
            OBJECTREF o1 = ArgSlotToObj(oi1);
            _ASSERTE(o1->GetClass()->IsValueClass());
            void *pData = o1->UnBox();
            *pArguments = PtrToArgSlot(pData);
        }
        else
        {
            *pArguments = ObjToArgSlot(newObj);
        }

    }

    // Do a Call on the MethodDesc to execute the method.  If the object was a COM object,
    // then we may not be able to fully resolve the MethodDesc, and so we do a
    // CallOnInterface
    if (pDE->m_md->IsInterface())
        pDE->m_result = pDE->m_md->CallOnInterface(pArguments, &mSig);

    // Otherwise, make a call on a regular runtime MethodDesc, which we are guaranteed we
    // can fully resolve since the original object is a runtime object.
    else 
        pDE->m_result = pDE->m_md->CallDebugHelper(pArguments, &mSig);

    // Ah, but if this was a new object op, then the result is really
    // the object we allocated above...
    if (pDE->m_evalType == DB_IPCE_FET_NEW_OBJECT)
        pDE->m_result = ObjToArgSlot(newObj);
    else if (pRetClass != NULL)
    {
     
        // Create an object from the return buffer.
        OBJECTREF retObject = AllocateObject(pRetClass->GetMethodTable());
        if (hasHiddenParam)
        {
            _ASSERTE(pRetValueClass != NULL);

            // box the object
            CopyValueClass(retObject->UnBox(), pRetValueClass,
                           pRetClass->GetMethodTable(), 
                           retObject->GetAppDomain());

        }
        else
        {
            _ASSERTE(pRetValueClass == NULL);

            // box the primitive returned
            CopyValueClass(retObject->UnBox(), &(pDE->m_result),
                           pRetClass->GetMethodTable(), 
                           retObject->GetAppDomain());

        }
        pDE->m_result = ObjToArgSlot(retObject);
    }
    
    // No exception, so it worked as far as we're concerned.
    pDE->m_successful = true;

    // To pass back the result to the right side, we need the basic
    // element type of the result and the module that the signature is
    // valid id (the function's module).
    pDE->m_resultModule = pDE->m_md->GetModule();

    if ((pRetClass != NULL) || (pDE->m_evalType == DB_IPCE_FET_NEW_OBJECT))
    {
        // We always return value classes boxed, and constructors called during a new object operation
        // always return an object...
        pDE->m_resultType = ELEMENT_TYPE_CLASS;
    }
    else
        pDE->m_resultType = mSig.GetReturnTypeNormalized();

    // If the result is an object, then place the object
    // reference into a strong handle and place the handle into the
    // pDE to protect the result from a collection.
    if ((pDE->m_resultType == ELEMENT_TYPE_CLASS) ||
        (pDE->m_resultType == ELEMENT_TYPE_SZARRAY) ||
        (pDE->m_resultType == ELEMENT_TYPE_OBJECT) ||
        (pDE->m_resultType == ELEMENT_TYPE_ARRAY) ||
        (pDE->m_resultType == ELEMENT_TYPE_STRING))
    {
        OBJECTHANDLE oh = pDE->m_thread->GetDomain()->CreateStrongHandle(ArgSlotToObj(pDE->m_result));
        pDE->m_result = (INT64)(LONG_PTR)oh;
    }

    // Update any enregistered byrefs with their new values from the
    // proper byref temporary array.
    if (pDE->m_argCount > 0)
    {
        mSig.Reset();
        
        unsigned int curr = 0;

        // For non-static methods, 'this' is always the first arg in
        // the array. All other args are put in in reverse order.
        if (!staticMethod && (pDE->m_evalType != DB_IPCE_FET_NEW_OBJECT))
            curr += 1;
            
        for (; curr < allocArgCnt; curr++)
        {
            CorElementType argSigType = mSig.NextArgNormalized();
            _ASSERTE(argSigType != ELEMENT_TYPE_END);

            if (argSigType == ELEMENT_TYPE_BYREF)
            {
                EEClass *byrefClass = NULL;
                CorElementType byrefArgSigType = mSig.GetByRefType(&byrefClass);
            
                SetByRefArgValue(pDE, &argData[mSig.HasRetBuffArg() ? curr-1 : curr], byrefArgSigType, pPrimitiveArgs[curr], pObjectRefArgs[curr]);
            }
        }
    }

    GCPROTECT_END();
    GCPROTECT_END();
}


//
// FuncEvalHijackWorker is the function that managed threads start executing in order to perform a function
// evaluation. Control is transfered here on the proper thread by hijacking that that's IP to this method in
// Debugger::FuncEvalSetup. This function can also be called directly by a Runtime thread that is stopped sending a
// first or second chance exception to the Right Side.
//
void *FuncEvalHijackWorker(DebuggerEval *pDE)
{
    LOG((LF_CORDB, LL_INFO100000, "D:FEHW for pDE:%08x evalType:%d\n", pDE, pDE->m_evalType));

    // Preemptive GC is disabled at the start of this method.
    _ASSERTE(g_pEEInterface->IsPreemptiveGCDisabled());

    // If we've got a filter context still installed, then remove it while we do the work...
    CONTEXT *filterContext = g_pEEInterface->GetThreadFilterContext(pDE->m_thread);

    if (filterContext)
    {
        _ASSERTE(pDE->m_evalDuringException);
        g_pEEInterface->SetThreadFilterContext(pDE->m_thread, NULL);
    } 

    // Push our FuncEvalFrame. The return address is equal to the IP in the saved context in the DebuggerEval. The
    // m_Datum becomes the ptr to the DebuggerEval.
    FuncEvalFrame FEFrame(pDE, (void*)GetIP(&pDE->m_context));
    FEFrame.Push();

    // Special handling for a re-abort eval. We don't setup a COMPLUS_TRY or try to lookup a function to call. All we do
    // is have this thread abort itself.
    if (pDE->m_evalType == DB_IPCE_FET_RE_ABORT)
    {
        pDE->m_thread->UserAbort(NULL);
        _ASSERTE(!"Should not return from UserAbort here!");
        return NULL;
    }
    
    // Wrap everything in a COMPLUS_TRY so we catch any exceptions that could be thrown.
    COMPLUS_TRY
    {
        switch (pDE->m_evalType)
        {
        case DB_IPCE_FET_NORMAL:
        case DB_IPCE_FET_NEW_OBJECT:
            {
                OBJECTREF Throwable = NULL;
                GCPROTECT_BEGIN(Throwable);
        
                // Find the proper MethodDesc that we need to call.
                HRESULT hr = EEClass::GetMethodDescFromMemberRef(pDE->m_debuggerModule->m_pRuntimeModule,
                                                           pDE->m_methodToken,
                                                           &(pDE->m_md),
                                                           &Throwable);

                if (FAILED(hr))
                    COMPlusThrow(Throwable);
                                      
                // We better have a MethodDesc at this point.
                _ASSERTE(pDE->m_md != NULL);

                IMDInternalImport   *pInternalImport = pDE->m_md->GetMDImport();
                DWORD       dwAttr = pInternalImport->GetMethodDefProps(pDE->m_methodToken);

                if (dwAttr & mdRequireSecObject)
                {
                    // command window cannot evaluate a function with mdRequireSecObject is turned on because
                    // this is expecting to put a security object into caller's frame which we don't have.
                    //
                    COMPlusThrow(kArgumentException,L"Argument_CantCallSecObjFunc");
                }


                // If this is a method on an interface, we have to resolve that down to the method on the class of the
                // 'this' parameter.
                if (pDE->m_md->GetClass()->IsInterface())
                {
                    MethodDesc *pMD = NULL;

                    // Assuming that a constructor can't be an interface method...
                    _ASSERTE(pDE->m_evalType == DB_IPCE_FET_NORMAL);

                    // We need to go grab the 'this' argument to figure out what class we're headed for...
                    _ASSERTE(pDE->m_argCount > 0);
                    DebuggerIPCE_FuncEvalArgData *argData = (DebuggerIPCE_FuncEvalArgData*) pDE->m_argData;

                    // Assume we can only have this for real objects, not value classes...
                    _ASSERTE((argData[0].argType == ELEMENT_TYPE_OBJECT) || (argData[0].argType == ELEMENT_TYPE_CLASS));

                    // We should have a valid this pointer. 
                    if (argData[0].argHome.kind == RAK_NONE && argData[0].argAddr == NULL)
                        COMPlusThrow(kArgumentNullException, L"ArgumentNull_Generic");
    
                    // Suck out the first arg. We're gonna trick GetArgValue by passing in just our object ref as the
                    // stack.
                    TypeHandle	dummyTH;
                    ARG_SLOT    objSlot;

                    // Note that we are passing ELEMENT_TYPE_END in the last parameter because we want to supress the the valid object ref
                    // check since it will be done properly in DoNormalFuncEval.
                    //
                    GetArgValue(pDE, &(argData[0]), false, false, dummyTH, ELEMENT_TYPE_CLASS, &objSlot, NULL, NULL, ELEMENT_TYPE_END);
                    OBJECTREF	objRef = ArgSlotToObj(objSlot);
                    Object      *objPtr = *((Object**) ((BYTE *)&objRef));
                    if (FAILED(ValidateObject(objPtr)))
                        COMPlusThrow(kArgumentException, L"Argument_BadObjRef");

                    // Null isn't valid in this case!
                    if (objPtr == NULL)
                        COMPlusThrow(kArgumentNullException, L"ArgumentNull_Obj");

                    // Now, find the proper MethodDesc for this interface method based on the object we're invoking the
                    // method on.
                    pMD = g_pEEInterface->GetVirtualMethod(pDE->m_debuggerModule->m_pRuntimeModule,
                                                                 OBJECTREFToObject(objRef),
                                                                 pDE->m_methodToken);
					if (pMD == NULL)
					{
						if  (OBJECTREFToObject(objRef)->GetMethodTable()->IsThunking())
						{
							// give it another try. It can be a proxy object
							if (OBJECTREFToObject(objRef)->GetMethodTable()->IsTransparentProxyType())
							{	
                                // Make sure the proxied object is loaded.
                                CRemotingServices::GetClass(objRef);
                                pMD = OBJECTREFToObject(objRef)->GetMethodTable()->GetMethodDescForInterfaceMethod(pDE->m_md, objRef);
							}
						}
                        if (pMD) 
                            pDE->m_md = pMD;
                        else
                            COMPlusThrow(kArgumentException, L"Argument_CORDBBadInterfaceMethod");
					}
                    else 
                        pDE->m_md = pMD;

                    _ASSERTE(pDE->m_md);

                    _ASSERTE(!pDE->m_md->GetClass()->IsInterface()
                             || OBJECTREFToObject(objRef)->GetMethodTable()->IsComObjectType());
                }

                // If this is a new object operation, then we should have a .ctor.
                if ((pDE->m_evalType == DB_IPCE_FET_NEW_OBJECT) && !pDE->m_md->IsCtor())
                    COMPlusThrow(kArgumentException, L"Argument_MissingDefaultConstructor");
                
                // Run the Class Init for this class, if necessary.
                if (!pDE->m_md->GetMethodTable()->CheckRunClassInit(&Throwable))
                    COMPlusThrow(Throwable);

                GCPROTECT_END();
        
                // Do the bulk of the calling work.
                DoNormalFuncEval(pDE);
                break;
            }
        
        case DB_IPCE_FET_NEW_OBJECT_NC:
            {
                OBJECTREF Throwable = NULL;
                GCPROTECT_BEGIN(Throwable);

                // Find the class.
                pDE->m_class = g_pEEInterface->LoadClass(pDE->m_debuggerModule->m_pRuntimeModule,
                                                         pDE->m_classToken);

                if (pDE->m_class == NULL)
                    COMPlusThrow(kArgumentNullException, L"ArgumentNull_Type");

                // Run the Class Init for this class, if necessary.
                if (!pDE->m_class->GetMethodTable()->CheckRunClassInit(&Throwable))
                    COMPlusThrow(Throwable);

                GCPROTECT_END();
        
                // Create a new instance of the class
                OBJECTREF newObj = NULL;
                GCPROTECT_BEGIN(newObj);

                newObj = AllocateObject(pDE->m_class->GetMethodTable());

                // No exception, so it worked.
                pDE->m_successful = true;

                // Result module is easy.
                pDE->m_resultModule = pDE->m_class->GetModule();

                // So is the result type.
                pDE->m_resultType = ELEMENT_TYPE_CLASS;

                // Make a strong handle for the result.
                OBJECTHANDLE oh = pDE->m_thread->GetDomain()->CreateStrongHandle(newObj);
                pDE->m_result = (INT64)(LONG_PTR)oh;
                GCPROTECT_END();
                
                break;
            }
        
        case DB_IPCE_FET_NEW_STRING:
            {
                // Create the string. m_argData is null terminated...
                STRINGREF sref = COMString::NewString((WCHAR*)pDE->m_argData);
                GCPROTECT_BEGIN(sref);

                // No exception, so it worked.
                pDE->m_successful = true;

                // No module needed since the result type is a string.
                pDE->m_resultModule = NULL;

                // Result type is, of course, a string.
                pDE->m_resultType = ELEMENT_TYPE_STRING;

                // Place the result in a strong handle to protect it from a collection.
                OBJECTHANDLE oh = pDE->m_thread->GetDomain()->CreateStrongHandle((OBJECTREF) sref);
                pDE->m_result = (INT64)(LONG_PTR)oh;
                GCPROTECT_END();
                
                break;
            }
        
        case DB_IPCE_FET_NEW_ARRAY:
            {
                OBJECTREF arr = NULL;
                GCPROTECT_BEGIN(arr);
                
                if (pDE->m_arrayRank > 1)
                    COMPlusThrow(kRankException, L"Rank_MultiDimNotSupported");

                // Gotta be a primitive, class, or System.Object.
                if (((pDE->m_arrayElementType < ELEMENT_TYPE_BOOLEAN) || (pDE->m_arrayElementType > ELEMENT_TYPE_R8)) &&
                    (pDE->m_arrayElementType != ELEMENT_TYPE_CLASS) &&
                    (pDE->m_arrayElementType != ELEMENT_TYPE_OBJECT))
                    COMPlusThrow(kArgumentOutOfRangeException, L"ArgumentOutOfRange_Enum");

                // Grab the dims from the arg/data area.
                SIZE_T *dims;
                dims = (SIZE_T*)pDE->m_argData;

                if (pDE->m_arrayElementType == ELEMENT_TYPE_CLASS)
                {
                    // Find the class we want to make the array elements out of.
                    pDE->m_class = g_pEEInterface->LoadClass(pDE->m_arrayClassDebuggerModuleToken->m_pRuntimeModule,
                                                             pDE->m_arrayClassMetadataToken);

                    arr = AllocateObjectArray(dims[0], TypeHandle(pDE->m_class->GetMethodTable()));
                }
                else if (pDE->m_arrayElementType == ELEMENT_TYPE_OBJECT)
                {
                    // We want to just make an array of System.Objects, so we don't require the user to pass in a
                    // specific class.
                    pDE->m_class = g_pObjectClass->GetClass();

                    arr = AllocateObjectArray(dims[0], TypeHandle(pDE->m_class->GetMethodTable()));
                }
                else
                {
                    // Create a simple array. Note: we can only do this type of create here due to the checks above.
                    arr = AllocatePrimitiveArray(pDE->m_arrayElementType, dims[0]);
                }
                
                    // No exception, so it worked.
                pDE->m_successful = true;

                // Use the module that the array belongs in.
                pDE->m_resultModule = arr->GetMethodTable()->GetModule();

                // Result type is, of course, the type of the array.
                pDE->m_resultType = arr->GetMethodTable()->GetNormCorElementType();

                // Place the result in a strong handle to protect it from a collection.
                OBJECTHANDLE oh = pDE->m_thread->GetDomain()->CreateStrongHandle(arr);
                pDE->m_result = (INT64)(LONG_PTR)oh;
                GCPROTECT_END();
                
                break;
            }

        default:
            _ASSERTE(!"Invalid eval type!");
        }
    }
    COMPLUS_CATCH
    {
        // Note: a fault in here makes things go poorly. The fault will be essentially ignored, and will cause this
        // catch handler to be essentially ingored by our exception system, making it seem as if this catch isn't
        // working.
        
        // We got an exception. Grab the exception object and make that into our result.
        pDE->m_successful = false;

        // Grab the exception.
        OBJECTREF ppException = GETTHROWABLE();
        GCPROTECT_BEGIN(ppException);
        
        // If this is a thread stop exception, and we tried to abort this eval, then the exception is ours.
        if (IsExceptionOfType(kThreadStopException, &ppException) && pDE->m_aborting)
        {
            pDE->m_result = NULL;
            pDE->m_resultType = ELEMENT_TYPE_VOID;
            pDE->m_resultModule = NULL;
            pDE->m_aborted = true;

            // Since we threw that thread stop exception, we need to reset the request to have it thrown.
            pDE->m_thread->ResetStopRequest();
        }   
        else
        {
            // Special handling for thread abort exceptions. We need to explicitly reset the abort request on the EE
            // thread, then make sure to place this thread on a thunk that will re-raise the exception when we continue
            // the process. Note: we still pass this thread abort exception up as the result of the eval.
            if (IsExceptionOfType(kThreadAbortException, &ppException))
            {
                // Reset the abort request and remember that we need to rethrow it.
                pDE->m_thread->UserResetAbort();
                pDE->m_rethrowAbortException = true;
            }   

            // The result is the exception object.
            pDE->m_result = ObjToArgSlot(ppException);

            if (pDE->m_md)
                pDE->m_resultModule = pDE->m_md->GetModule();
            else
                pDE->m_resultModule = NULL;
        
            pDE->m_resultType = ELEMENT_TYPE_CLASS;
            OBJECTHANDLE oh = pDE->m_thread->GetDomain()->CreateStrongHandle(ArgSlotToObj(pDE->m_result));
            pDE->m_result = (INT64)(LONG_PTR)oh;
        }
        GCPROTECT_END();
    }
    COMPLUS_END_CATCH

    // The func-eval is now completed, successfully or with failure, aborted or run-to-completion.
    pDE->m_completed = true;

    // Codepitching can hijack our frame's return address. That means that we'll need to update PC in our saved context
    // so that when its restored, its like we've returned to the codepitching hijack. At this point, the old value of
    // EIP is worthless anyway.
    if (!pDE->m_evalDuringException)
        SetIP(&pDE->m_context, FEFrame.GetReturnAddress());
    
    // Pop the FuncEvalFrame now that we're pretty much done.
    FEFrame.Pop();

    if (!pDE->m_evalDuringException)
    {
        // Signal to the helper thread that we're done with our func eval.  Start by creating a DebuggerFuncEvalComplete
        // object. Give it an address at which to create the patch, which is a chunk of memory inside of our
        // DebuggerEval big enough to hold a breakpoint instruction.
        void *dest = &(pDE->m_breakpointInstruction);

        // Here is kind of a cheat... we make sure that the address that we patch and jump to is actually also the ptr
        // to our DebuggerEval. This works because m_breakpointInstruction is the first field of the DebuggerEval
        // struct.
        _ASSERTE(dest == pDE);

        SetupDebuggerFuncEvalComplete(pDE->m_thread, dest);
    
        return dest;
    }
    else
    {
        // We don't have to setup any special hijacks to return from here when we've been processing during an
        // exception. We just go ahead and send the FuncEvalComplete event over now. Don't forget to enable/disable PGC
        // around the call...
        _ASSERTE(g_pEEInterface->IsPreemptiveGCDisabled());

        if (filterContext != NULL)
            g_pEEInterface->SetThreadFilterContext(pDE->m_thread, filterContext);
        
        g_pEEInterface->EnablePreemptiveGC();

        BOOL threadStoreLockOwner = FALSE;
        g_pDebugger->LockForEventSending();

        if (CORDebuggerAttached()) {
            g_pDebugger->FuncEvalComplete(pDE->m_thread, pDE);

            threadStoreLockOwner = g_pDebugger->SyncAllThreads();
        }
        
        g_pDebugger->UnlockFromEventSending();

        g_pDebugger->BlockAndReleaseTSLIfNecessary(threadStoreLockOwner);
    
        g_pEEInterface->DisablePreemptiveGC();
        
        return NULL;
    }
}

//
// FuncEvalSetup sets up a function evaluation for the given method on the given thread.
//
HRESULT Debugger::FuncEvalSetup(DebuggerIPCE_FuncEvalInfo *pEvalInfo,
                                BYTE **argDataArea,
                                void **debuggerEvalKey)
{
    Thread *pThread = (Thread*)pEvalInfo->funcDebuggerThreadToken;
    
    // If TS_StopRequested (which may have been set by a pending FuncEvalAbort),
    // we will not be able to do a new func-eval
    if (pThread->m_State & Thread::TS_StopRequested)
        return CORDBG_E_FUNC_EVAL_BAD_START_POINT;

    if (g_fProcessDetach)
        return CORDBG_E_FUNC_EVAL_BAD_START_POINT;
    
    bool fInException = pEvalInfo->evalDuringException;

    // The thread has to be at a GC safe place for now, just in case the func eval causes a collection. Processing an
    // exception also counts as a "safe place." Eventually, we'd like to have to avoid this check and eval anyway, but
    // that's a way's off...
    if (!fInException && !g_pDebugger->IsThreadAtSafePlace(pThread))
        return CORDBG_E_FUNC_EVAL_BAD_START_POINT;

    _ASSERTE(!(g_pEEInterface->GetThreadFilterContext(pThread) && ISREDIRECTEDTHREAD(pThread)));
    
    // For now, we assume that the target thread must be stopped in managed code due to a single step or a
    // breakpoint. Being stopped while sending a first or second chance exception is also valid, and there may or may
    // not be a filter context when we do a func eval from such places. This will loosen over time, eventually allowing
    // threads that are stopped anywhere in managed code to perform func evals.
    CONTEXT *filterContext = g_pEEInterface->GetThreadFilterContext(pThread);

    // If the thread is redirected, then we can also perform a FuncEval with it since we now have all the necessary
    // frames set up to protect the managed stack at the point the thread was suspended.
    if (filterContext == NULL && ISREDIRECTEDTHREAD(pThread))
    {
        filterContext = GETREDIRECTEDCONTEXT(pThread);
    }
    
    if (filterContext == NULL && !fInException)
        return CORDBG_E_FUNC_EVAL_BAD_START_POINT;

    // Create a DebuggerEval to hold info about this eval while its in progress. Constructor copies the thread's
    // CONTEXT.
    DebuggerEval *pDE = new (interopsafe) DebuggerEval(filterContext, pEvalInfo, fInException);

    if (pDE == NULL)
        return E_OUTOFMEMORY;

    SIZE_T argDataAreaSize = 0;
    
    if ((pEvalInfo->funcEvalType == DB_IPCE_FET_NORMAL) ||
        (pEvalInfo->funcEvalType == DB_IPCE_FET_NEW_OBJECT) ||
        (pEvalInfo->funcEvalType == DB_IPCE_FET_NEW_OBJECT_NC))
        argDataAreaSize = pEvalInfo->argCount * sizeof(DebuggerIPCE_FuncEvalArgData);
    else if (pEvalInfo->funcEvalType == DB_IPCE_FET_NEW_STRING)
        argDataAreaSize = pEvalInfo->stringSize;
    else if (pEvalInfo->funcEvalType == DB_IPCE_FET_NEW_ARRAY)
        argDataAreaSize = pEvalInfo->arrayDataLen;

    if (argDataAreaSize > 0)
    {
        pDE->m_argData = new (interopsafe) BYTE[argDataAreaSize];

        if (pDE->m_argData == NULL)
        {
            DeleteInteropSafe(pDE);
            return E_OUTOFMEMORY;
        }

        // Pass back the address of the argument data area so the right side can write to it for us.
        *argDataArea = pDE->m_argData;
    }
    
    // Set the thread's IP (in the filter context) to our hijack function if we're stopped due to a breakpoint or single
    // step.
    if (!fInException)
    {
        _ASSERTE(filterContext != NULL);
        
        ::SetIP(filterContext, (LPVOID)::FuncEvalHijack);

        // Don't be fooled into thinking you can push things onto the thread's stack now. If the thread is stopped at a
        // breakpoint or from a single step, then its really suspended in the SEH filter. ESP in the thread's CONTEXT,
        // therefore, points into the middle of the thread's current stack. So we pass things we need in the hijack in
        // the thread's registers.

        // Set EAX to point to the DebuggerEval.
#if defined(_X86_)
        filterContext->Eax = (DWORD)pDE;
#elif defined(_PPC_)
        filterContext->Gpr11 = (DWORD)pDE;
#else
        PORTABILITY_ASSERT("Debugger::FuncEvalSetup is not implemented on this platform.");
#endif
    }
    else
    {
        HRESULT hr = CheckInitPendingFuncEvalTable();
        _ASSERTE(SUCCEEDED(hr));

        if (FAILED(hr))
            return (hr);

        // If we're in an exception, then add a pending eval for this thread. This will cause us to perform the func
        // eval when the user continues the process after the current exception event.
        g_pDebugger->m_pPendingEvals->AddPendingEval(pDE->m_thread, pDE);
    }

    // Return that all went well. Tracing the stack at this point should not show that the func eval is setup, but it
    // will show a wrong IP, so it shouldn't be done.
    *debuggerEvalKey = (void*)pDE;
    
    LOG((LF_CORDB, LL_INFO100000, "D:FES for pDE:%08x evalType:%d on thread %#x, id=0x%x\n",
        pDE, pDE->m_evalType, pThread, pThread->GetThreadId()));

    return S_OK;
}

//
// FuncEvalSetupReAbort sets up a function evaluation specifically to rethrow a ThreadAbortException on the given
// thread.
//
HRESULT Debugger::FuncEvalSetupReAbort(Thread *pThread)
{
    LOG((LF_CORDB, LL_INFO1000,
            "D::FESRA: performing reabort on thread %#x, id=0x%x\n",
            pThread, pThread->GetThreadId()));


    // The thread has to be at a GC safe place. It should be, since this is only done in response to a previous eval
    // completing with a ThreadAbortException.
    if (!g_pDebugger->IsThreadAtSafePlace(pThread))
        return CORDBG_E_FUNC_EVAL_BAD_START_POINT;
    
    _ASSERTE(!(g_pEEInterface->GetThreadFilterContext(pThread) && ISREDIRECTEDTHREAD(pThread)));

    // Grab the filter context.
    CONTEXT *filterContext = g_pEEInterface->GetThreadFilterContext(pThread);
    
    // If the thread is redirected, then we can also perform a FuncEval with it since we now have all the necessary
    // frames set up to protect the managed stack at the point the thread was suspended.
    if (filterContext == NULL && ISREDIRECTEDTHREAD(pThread))
    {
        filterContext = GETREDIRECTEDCONTEXT(pThread);
    }
    
    if (filterContext == NULL)
        return CORDBG_E_FUNC_EVAL_BAD_START_POINT;

    // Create a DebuggerEval to hold info about this eval while its in progress. Constructor copies the thread's
    // CONTEXT.
    DebuggerEval *pDE = new (interopsafe) DebuggerEval(filterContext, pThread);

    if (pDE == NULL)
        return E_OUTOFMEMORY;

    // Set the thread's IP (in the filter context) to our hijack function.
    _ASSERTE(filterContext != NULL);

#ifdef _X86_ // reliance on filterContext->Eip & Eax
    filterContext->Eip = (DWORD)::FuncEvalHijack;
    // Set EAX to point to the DebuggerEval.
    filterContext->Eax = (DWORD)pDE;
#elif defined(_PPC_)
    filterContext->Iar = (DWORD)::FuncEvalHijack;
    // Set r11 to point to the DebuggerEval.
    filterContext->Gpr11 = (DWORD)pDE;
#else
    PORTABILITY_ASSERT("FuncEvalSetupReAbort (Debugger.cpp) is not implemented on this platform.");
#endif

    // Now clear the bit requesting a re-abort
    pThread->ResetThreadStateNC(Thread::TSNC_DebuggerReAbort);

    // Return that all went well. Tracing the stack at this point should not show that the func eval is setup, but it
    // will show a wrong IP, so it shouldn't be done.
    
    return S_OK;
}

//
// FuncEvalAbort aborts a function evaluation already in progress.
//
HRESULT Debugger::FuncEvalAbort(void *debuggerEvalKey)
{
    DebuggerEval *pDE = (DebuggerEval*) debuggerEvalKey;

    if (pDE->m_aborting == false)
    {
        // Remember that we're aborting this func eval.
        pDE->m_aborting = true;
    
        LOG((LF_CORDB, LL_INFO1000,
             "D::FEA: performing UserStopForDebugger on thread %#x, id=0x%x\n",
             pDE->m_thread, pDE->m_thread->GetThreadId()));

        if (!g_fProcessDetach && !pDE->m_completed)
        {
            // Perform a user stop on the thread that the eval is running on.
            // This will cause a ThreadStopException to be thrown on the thread.
            
            if (m_stopped)
                pDE->m_thread->SetStopRequest(); // Queue a stop-request for whenever the thread is resumed
            else
                pDE->m_thread->UserStopForDebugger(); // Try to stop the running thread now
        }
        LOG((LF_CORDB, LL_INFO1000, "D::FEA: UserStopForDebugger complete.\n"));
    }

    return S_OK;
}

//
// FuncEvalCleanup cleans up after a function evaluation is released.
//
HRESULT Debugger::FuncEvalCleanup(void *debuggerEvalKey)
{
    DebuggerEval *pDE = (DebuggerEval*) debuggerEvalKey;

    _ASSERTE(pDE->m_completed);

    LOG((LF_CORDB, LL_INFO1000, "D::FEC: pDE:%08x 0x%08x, id=0x%x\n",
         pDE, pDE->m_thread, pDE->m_thread->GetThreadId()));

    DeleteInteropSafe(pDE);

    return S_OK;
}


unsigned FuncEvalFrame::GetFrameAttribs()
{
    if (((DebuggerEval*)m_Datum)->m_evalDuringException)
        return FRAME_ATTR_NONE;
    else
        return FRAME_ATTR_RESUMABLE;    // Treat the next frame as the top frame.
}


LPVOID* FuncEvalFrame::GetReturnAddressPtr()
{
    if (((DebuggerEval*)m_Datum)->m_evalDuringException)
        return NULL;
    else
        return &m_ReturnAddress;
}

//
// This updates the register display for a FuncEvalFrame.
//
void FuncEvalFrame::UpdateRegDisplay(const PREGDISPLAY pRD)
{
    DebuggerEval *pDE = (DebuggerEval*)GetDebuggerEval();

    // No context to update if we're doing a func eval from within exception processing.
    if (pDE->m_evalDuringException)
        return;

    // Reset pContext; it's only valid for active (top-most) frame.
    pRD->pContext = NULL;

    pRD->SP  = (DWORD)GetSP(&pDE->m_context);
    pRD->pPC  = (SLOT*)GetReturnAddressPtr();

#ifdef _X86_

    // Update all registers in the reg display from the CONTEXT we stored when the thread was hijacked for this func
    // eval. We have to update all registers, not just the callee saved registers, because we can hijack a thread at any
    // point for a func eval, not just at a call site.
    pRD->pEdi = &(pDE->m_context.Edi);
    pRD->pEsi = &(pDE->m_context.Esi);
    pRD->pEbx = &(pDE->m_context.Ebx);
    pRD->pEdx = &(pDE->m_context.Edx);
    pRD->pEcx = &(pDE->m_context.Ecx);
    pRD->pEax = &(pDE->m_context.Eax);
    pRD->pEbp = &(pDE->m_context.Ebp);

#elif defined(_PPC_)

    // update all registers in the reg display
    for (int i=0; i<NUM_CALLEESAVED_REGISTERS; i++)
        pRD->pR[i] = ((DWORD *)&(pDE->m_context.Gpr13)) + i;

    for (int i=0; i<NUM_FLOAT_CALLEESAVED_REGISTERS; i++)
        pRD->pF[i] = &(pDE->m_context.Fpr14) + i;

    pRD->CR = pDE->m_context.Cr;

#else // _X86_
    PORTABILITY_ASSERT("FuncEvalFrame::UpdateRegDisplay is not implemented on this platform.");
#endif
}


//
// SetReference sets an object reference for the Right Side,
// respecting the write barrier for references that are in the heap.
//
HRESULT Debugger::SetReference(void *objectRefAddress,
                               bool  objectRefInHandle,
                               void *newReference)
{
    HRESULT     hr = S_OK;

    // If the object ref isn't in a handle, then go ahead and use
    // SetObjectReference.
    if (!objectRefInHandle)
    {
        OBJECTREF *dst = (OBJECTREF*)objectRefAddress;
        OBJECTREF  src = *((OBJECTREF*)&newReference);

        SetObjectReferenceUnchecked(dst, src);
    }
    else
    {
        hr = ValidateObject((Object *)newReference);

        if (SUCCEEDED(hr))
        {
            // If the object reference to set is inside of a handle, then
            // fixup the handle.
            OBJECTHANDLE h = *((OBJECTHANDLE*)objectRefAddress);
            OBJECTREF  src = *((OBJECTREF*)&newReference);
            HndAssignHandle(h, src);
        }
    }
    
    return hr;
}

//
// SetValueClass sets a value class for the Right Side, respecting the write barrier for references that are embedded
// within in the value class.
//
HRESULT Debugger::SetValueClass(void *oldData, void *newData, mdTypeDef classMetadataToken, void *classDebuggerModuleToken)
{
    HRESULT hr = S_OK;

    // Find the class given its module and token. The class must be loaded.
    DebuggerModule *pDebuggerModule = (DebuggerModule*) classDebuggerModuleToken;
    EEClass *pClass = g_pEEInterface->FindLoadedClass(pDebuggerModule->m_pRuntimeModule, classMetadataToken);

    if (pClass == NULL)
        return CORDBG_E_CLASS_NOT_LOADED;

    // Update the value class.
    CopyValueClassUnchecked(oldData, newData, pClass->GetMethodTable());
    
    // Free the buffer that is holding the new data. This is a buffer that was created in response to a GET_BUFFER
    // message, so we release it with ReleaseRemoteBuffer.
    ReleaseRemoteBuffer((BYTE*)newData, true);
    
    return hr;
}

/******************************************************************************
 *
 ******************************************************************************/
HRESULT Debugger::SetILInstrumentedCodeMap(MethodDesc *fd,
                                           BOOL fStartJit,
                                           ULONG32 cILMapEntries,
                                           COR_IL_MAP rgILMapEntries[])
{
    if (fStartJit == TRUE)
        JITBeginning(fd, true);

    DebuggerJitInfo *dji = GetJitInfo(fd,NULL);

    _ASSERTE(dji != NULL);

    if (dji->m_rgInstrumentedILMap != NULL)
    {
        CoTaskMemFree(dji->m_rgInstrumentedILMap);
    }
    
    dji->m_cInstrumentedILMap = cILMapEntries;
    dji->m_rgInstrumentedILMap =  rgILMapEntries;
    
    return S_OK;
}

//
// EarlyHelperThreadDeath handles the case where the helper
// thread has been ripped out from underneath of us by
// ExitProcess or TerminateProcess. These calls are pure evil, wacking
// all threads except the caller in the process. This can happen, for
// instance, when an app calls ExitProcess. All threads are wacked,
// the main thread calls all DLL main's, and the EE starts shutting
// down in its DLL main with the helper thread nuked.
//
void Debugger::EarlyHelperThreadDeath(void)
{
    if (m_pRCThread)    
        m_pRCThread->EarlyHelperThreadDeath();
}

//
// This tells the debugger that shutdown of the in-proc debugging services has begun. We need to know this during
// managed/unmanaged debugging so we can stop doing certian things to the process (like hijacking threads.)
//
void Debugger::ShutdownBegun(void)
{
    if (m_pRCThread != NULL)
    {
        DebuggerIPCControlBlock *dcb = m_pRCThread->GetDCB(IPC_TARGET_OUTOFPROC);

        if ((dcb != NULL) && (dcb->m_rightSideIsWin32Debugger))
            dcb->m_shutdownBegun = true;
    }
}

#include "corpub.h"
#include "cordb.h"


/******************************************************************************
 *
 ******************************************************************************/
HRESULT Debugger::SetCurrentPointerForDebugger( void *ptr,PTR_TYPE ptrType)
{
    _ASSERTE(m_pRCThread->m_cordb != NULL);

    INPROC_LOCK();

    CordbBase *b = m_pRCThread->m_cordb->m_processes.GetBase(
        GetCurrentProcessId());

    _ASSERTE( b != NULL );
    CordbProcess *p = (CordbProcess *)b;
    b = p->m_userThreads.GetBase(GetCurrentThreadId());
    CordbThread *t = (CordbThread *)b;
    
    // Current proc can't be NULL, thread might not have started.
    if (t == NULL)         
    {
        LOG((LF_CORDB, LL_INFO10000, "D::SCPFD: thread is null!\n"));

        INPROC_UNLOCK();
        return CORDBG_E_BAD_THREAD_STATE;
    }
    
    switch(ptrType)
    {
        case PT_MODULE:
#ifdef _DEBUG
            _ASSERTE((!t->m_pModuleSpecial ^ !ptr) ||
                      t->m_pModuleSpecial == ptr);
#endif //_DEBUG
            LOG((LF_CORDB, LL_INFO10000, "D::SCPFD: PT_MODULE:0x%x\n",ptr));
            t->m_pModuleSpecial = (Module *)ptr;
            break;            

        case PT_ASSEMBLY:
            LOG((LF_CORDB, LL_INFO10000, "D::SCPFD: PT_ASSEMBLY:0x%x\n",ptr));
            if (ptr == NULL)
            {
#ifdef _DEBUG
                _ASSERTE(t->m_pAssemblySpecialCount > 0);
#endif
                t->m_pAssemblySpecialCount--;
            }
            else
            {
                if (t->m_pAssemblySpecialCount == t->m_pAssemblySpecialAlloc)
                {
                    Assembly **pOldStack;
                    USHORT newAlloc;
                    if (t->m_pAssemblySpecialAlloc == 1)
                    {
                        // Special case - size one stack doesn't allocate
                        pOldStack = &t->m_pAssemblySpecial;
                        newAlloc = 5;
                    }
                    else
                    {
                        pOldStack = t->m_pAssemblySpecialStack;
                        newAlloc = t->m_pAssemblySpecialAlloc*2;
                    }

                    Assembly **pNewStack = 
                      new Assembly* [newAlloc];
                    
                    memcpy(pNewStack, pOldStack, 
                           t->m_pAssemblySpecialCount * sizeof(Assembly*));
                    if (pOldStack != &t->m_pAssemblySpecial)
                        delete [] pOldStack;

                    t->m_pAssemblySpecialAlloc = newAlloc;
                    t->m_pAssemblySpecialStack = pNewStack;
                }

                if (t->m_pAssemblySpecialAlloc == 1)
                    t->m_pAssemblySpecial = (Assembly*)ptr;
                else
                    t->m_pAssemblySpecialStack[t->m_pAssemblySpecialCount] 
                      = (Assembly*)ptr;

                t->m_pAssemblySpecialCount++;
            }
            break;            
        
        default:
            _ASSERTE( !"Debugger::SetCurrentPointerForDebugger given invalid type!\n");
    }

    INPROC_UNLOCK();
    return S_OK;

}

/******************************************************************************
 *
 ******************************************************************************/
HRESULT Debugger::GetInprocICorDebug( IUnknown **iu, bool fThisThread)
{
    _ASSERTE(m_pRCThread != NULL && m_pRCThread->m_cordb != NULL);

    if (fThisThread)
    {
        INPROC_LOCK();

        CordbBase *b = m_pRCThread->m_cordb->m_processes.GetBase(
            GetCurrentProcessId());

        // Current proc can't be NULL
        _ASSERTE( b != NULL );
        CordbProcess *p = (CordbProcess *)b;
        b = p->m_userThreads.GetBase(GetCurrentThreadId());
        CordbThread *t = (CordbThread *)b;

        INPROC_UNLOCK();
        
        if (t != NULL)         
        {
            return  t->QueryInterface(IID_IUnknown, (void**)iu);
        }
        else
        {
            // If we weren't able to find it, it's because it's not a managed
            // thread.  Perhaps it hasn't started, perhaps it's 'dead', perhaps
            // we're the concurrent GC thread.
            return CORPROF_E_NOT_MANAGED_THREAD;
        }
    }
    else
    {
        return m_pRCThread->m_cordb->QueryInterface(IID_IUnknown, (void**)iu);;
    }
}

/****************************************************************************/
HRESULT Debugger::SetInprocActiveForThread(BOOL fIsActive)
{
    INPROC_LOCK();

    CordbBase *b = m_pRCThread->m_cordb->m_processes.GetBase(GetCurrentProcessId());

    // Current proc can't be NULL
    _ASSERTE( b != NULL );
    CordbProcess *p = (CordbProcess *)b;

    // Get this thread's object
    b = p->m_userThreads.GetBase(GetCurrentThreadId());
    _ASSERTE(b != NULL);

    CordbThread *t = (CordbThread *)b;

    // Set the value
    t->m_fThreadInprocIsActive = fIsActive;

    // Always set the framesFresh to false
    t->m_framesFresh = false;

    INPROC_UNLOCK();

    return (S_OK);
}

/****************************************************************************/
BOOL Debugger::GetInprocActiveForThread()
{
    INPROC_LOCK();

    CordbBase *b = m_pRCThread->m_cordb->m_processes.GetBase(GetCurrentProcessId());

    // Current proc can't be NULL
    _ASSERTE( b != NULL );
    CordbProcess *p = (CordbProcess *)b;

    // Get this thread's object
    b = p->m_userThreads.GetBase(GetCurrentThreadId());
    _ASSERTE(b != NULL);

    CordbThread *t = (CordbThread *)b;

    // Make sure that we're not re-entering the 
    BOOL fIsActive = t->m_fThreadInprocIsActive;

    INPROC_UNLOCK();

    return (fIsActive);
}

/****************************************************************************/
void Debugger::InprocOnThreadDestroy(Thread *pThread)
{
    CordbProcess *pdbProc = (CordbProcess *) m_pRCThread->m_cordb->m_processes.GetBase(GetCurrentProcessId());
    _ASSERTE(pdbProc != NULL);

    CordbThread *pdbThread = (CordbThread *) pdbProc->m_userThreads.GetBase(pThread->GetThreadId(), FALSE);

    if (pdbThread != NULL)
    {
        pdbThread = (CordbThread *) pdbProc->m_userThreads.RemoveBase(pThread->GetThreadId());
        _ASSERTE(pdbThread != NULL);
    }
}

/****************************************************************************
 * This will perform the duties of the helper thread if none already exists.
 * This is called in the case that the loader lock is held and so no new
 * threads can be spun up to be the helper thread, so the existing thread
 * must be the helper thread until a new one can spin up.
 ***************************************************************************/
void Debugger::DoHelperThreadDuty(bool temporaryHelp)
{
    _ASSERTE(ThreadHoldsLock());

    LOG((LF_CORDB, LL_INFO1000,
         "D::SSCIPCE: helper thread is not ready, doing helper "
         "thread duty...\n"));

    // We're the temporary helper thread now.
    m_pRCThread->GetDCB(IPC_TARGET_OUTOFPROC)->m_temporaryHelperThreadId =
        GetCurrentThreadId();

    // Make sure the helper thread has something to wait on while
    // we're trying to be the helper thread.
    ResetEvent(m_pRCThread->GetHelperThreadCanGoEvent());

    // Release the debugger lock.
    Unlock();

    // We set the syncThreadIsLockFree event here. If we're in this call, then it means that we're on the thread that
    // sent up the sync complete flare, and we've released the debuger lock. By setting this event, we allow the Right
    // Side to suspend this thread now. (Note: this is all for Win32 debugging support.)
    if (m_pRCThread->GetDCB(IPC_TARGET_OUTOFPROC)->m_rightSideIsWin32Debugger)
        SetEvent(m_pRCThread->GetDCB(IPC_TARGET_OUTOFPROC)->m_syncThreadIsLockFree);
    
    // Do helper thread duty. We pass true to it knows that we're
    // the temporary helper thread.
    m_pRCThread->MainLoop(temporaryHelp);

    // Re-lock the debugger.
    Lock();

    LOG((LF_CORDB, LL_INFO1000,
         "D::SSCIPCE: done doing helper thread duty. "
         "Current helper thread id=0x%x\n",
         m_pRCThread->GetDCB(IPC_TARGET_OUTOFPROC)->m_helperThreadId));

    // We're not the temporary helper thread anymore.
    m_pRCThread->GetDCB(IPC_TARGET_OUTOFPROC)->m_temporaryHelperThreadId = 0;

    // Let the helper thread go if its waiting on us.
    SetEvent(m_pRCThread->GetHelperThreadCanGoEvent());
}

// Some of this code is copied in DebuggerRCEventThead::Mainloop
HRESULT Debugger::VrpcToVls(DebuggerIPCEvent *event)
{
    // Make room for any Right Side event on the stack.
    DebuggerIPCEvent *e =
        (DebuggerIPCEvent *) _alloca(CorDBIPC_BUFFER_SIZE);

    memcpy(e, 
           m_pRCThread->GetIPCEventReceiveBuffer(IPC_TARGET_INPROC), 
           CorDBIPC_BUFFER_SIZE);

    bool fIrrelevant;
    fIrrelevant = HandleIPCEvent(e, IPC_TARGET_INPROC);

    return S_OK;
}


// This function is called from the EE to notify the right side
// whenever the name of a thread or AppDomain changes
HRESULT Debugger::NameChangeEvent(AppDomain *pAppDomain, Thread *pThread)
{
    // Don't try to send one of these if the thread really isn't setup
    // yet. This can happen when initially setting up an app domain,
    // before the appdomain create event has been sent. Since the app
    // domain create event hasn't been sent yet in this case, its okay
    // to do this...
    if (g_pEEInterface->GetThread() == NULL)
        return S_OK;
    
    LOG((LF_CORDB, LL_INFO1000, "D::NCE: Sending NameChangeEvent 0x%x 0x%x\n",
        pAppDomain, pThread));

    bool disabled = g_pEEInterface->IsPreemptiveGCDisabled();

    if (disabled)
        g_pEEInterface->EnablePreemptiveGC();

    // Prevent other Runtime threads from handling events.
    BOOL threadStoreLockOwner = FALSE;
    
    LockForEventSending();
    
    if (CORDebuggerAttached())
    {

        DebuggerIPCEvent* ipce = m_pRCThread->GetIPCEventSendBuffer(IPC_TARGET_OUTOFPROC);
        InitIPCEvent(ipce, 
                     DB_IPCE_NAME_CHANGE, 
                     g_pEEInterface->GetThread()->GetThreadId(),
                     (void *)(g_pEEInterface->GetThread()->GetDomain()));


        if (pAppDomain)
        {
            ipce->NameChange.eventType = APP_DOMAIN_NAME_CHANGE;
            ipce->NameChange.debuggerAppDomainToken = (void *)pAppDomain;
        }
        else
        {
            ipce->NameChange.eventType = THREAD_NAME_CHANGE;
            _ASSERTE (pThread);
            ipce->NameChange.debuggerThreadToken = pThread->GetThreadId();
        }

        m_pRCThread->SendIPCEvent(IPC_TARGET_OUTOFPROC);

        // Stop all Runtime threads
        threadStoreLockOwner = TrapAllRuntimeThreads(g_pEEInterface->GetThread()->GetDomain());
    } 
    else 
    {
        LOG((LF_CORDB,LL_INFO1000, "D::NCE: Skipping SendIPCEvent because RS detached."));
    }
    
    UnlockFromEventSending();

    BlockAndReleaseTSLIfNecessary(threadStoreLockOwner);
    
    g_pEEInterface->DisablePreemptiveGC();

    if (!disabled)
        g_pEEInterface->EnablePreemptiveGC();

    return S_OK;

}

/******************************************************************************
 *
 ******************************************************************************/
BOOL Debugger::SendCtrlCToDebugger(DWORD dwCtrlType)
{    
    LOG((LF_CORDB, LL_INFO1000, "D::SCCTD: Sending CtrlC Event 0x%x\n",
        dwCtrlType));

    // Prevent other Runtime threads from handling events.
    BOOL threadStoreLockOwner = FALSE;
    
    LockForEventSending();
    
    if (CORDebuggerAttached())
    {
        DebuggerIPCEvent* ipce = m_pRCThread->GetIPCEventSendBuffer(IPC_TARGET_OUTOFPROC);
        InitIPCEvent(ipce, 
                     DB_IPCE_CONTROL_C_EVENT, 
                     GetCurrentThreadId(),
                     NULL);

        ipce->Exception.exceptionHandle = (void *)dwCtrlType;

        m_pRCThread->SendIPCEvent(IPC_TARGET_OUTOFPROC);

        // Stop all Runtime threads
        threadStoreLockOwner = TrapAllRuntimeThreads(NULL);
    }
    else 
    {
        LOG((LF_CORDB,LL_INFO1000, "D::SCCTD: Skipping SendIPCEvent because RS detached."));
    }
    
    UnlockFromEventSending();

    // now wait for notification from the right side about whether or not
    // the out-of-proc debugger is handling ControlC events.
    WaitForSingleObject(m_CtrlCMutex, INFINITE);

    BlockAndReleaseTSLIfNecessary(threadStoreLockOwner);
    
    return m_DebuggerHandlingCtrlC;
}

/******************************************************************************
 *
 ******************************************************************************/
void Debugger::ClearAppDomainPatches(AppDomain *pAppDomain)
{
    LOG((LF_CORDB, LL_INFO10000, "D::CADP\n"));

    _ASSERTE(pAppDomain != NULL);
    
    Lock();

    DebuggerController::DeleteAllControllers(pAppDomain);

    Unlock();
}

// Allows the debugger to keep an up to date list of special threads
HRESULT Debugger::UpdateSpecialThreadList(DWORD cThreadArrayLength,
                                        DWORD *rgdwThreadIDArray)
{
    _ASSERTE(g_pRCThread != NULL);
    
    DebuggerIPCControlBlock *pIPC = g_pRCThread->GetDCB(IPC_TARGET_OUTOFPROC);
    _ASSERTE(pIPC);

    if (!pIPC)
        return (E_FAIL);

    // Save the thread list information, and mark the dirty bit so
    // the right side knows.
    pIPC->m_specialThreadList = rgdwThreadIDArray;
    pIPC->m_specialThreadListLength = cThreadArrayLength;
    pIPC->m_specialThreadListDirty = true;

    return (S_OK);
}

// Updates the pointer for the debugger services
void Debugger::SetIDbgThreadControl(IDebuggerThreadControl *pIDbgThreadControl)
{
    if (m_pIDbgThreadControl)
        m_pIDbgThreadControl->Release();

    m_pIDbgThreadControl = pIDbgThreadControl;

    if (m_pIDbgThreadControl)
        m_pIDbgThreadControl->AddRef();
}

//
// If a thread is Win32 suspended right after hitting a breakpoint instruction, but before the OS has transitioned the
// thread over to the user-level exception dispatching logic, then we may see the IP pointing after the breakpoint
// instruction. There are times when the Runtime will use the IP to try to determine what code as run in the prolog or
// epilog, most notably when unwinding a frame. If the thread is suspended in such a case, then the unwind will believe
// that the instruction that the breakpoint replaced has really been executed, which is not true. This confuses the
// unwinding logic. This function is called from Thread::HandledJITCase() to help us recgonize when this may have
// happened and allow us to skip the unwind and abort the HandledJITCase.
//
// The criteria is this:
//
// 1) If a debugger is attached.
//
// 2) If the instruction before the IP is a breakpoint instruction.
//
// 3) If the IP is in the prolog or epilog of a managed function.
//
BOOL Debugger::IsThreadContextInvalid(Thread *pThread)
{
    BOOL invalid = FALSE;

    // Get the thread context.
    CONTEXT ctx;
    ctx.ContextFlags = CONTEXT_CONTROL;
    BOOL success = ::GetThreadContext(pThread->GetThreadHandle(), &ctx);

    if (success)
    {
        DWORD inst = 0;
        
        PAL_TRY
        {
#ifdef _X86_
            // Grab Eip - 1
            inst = (DWORD)*(((BYTE*)GetIP(&ctx)) - 1);
#elif defined(_PPC_) || defined(_SPARC_)
            inst = *(((DWORD*)GetIP(&ctx)) - 1);
#else
            PORTABILITY_ASSERT("IsThreadContextInvalid (debugger.cpp) is not implemented on this platform.");
#endif
        }
        PAL_EXCEPT_FILTER(FilterAccessViolation, NULL)
        {
            // If we fault trying to read the byte before EIP, then we know that its not a breakpoint.
            inst = 0;
        }
        PAL_ENDTRY

        // Is it a breakpoint?
        if (inst == CORDbg_BREAK_INSTRUCTION)
        {
            size_t prologSize; // Unused...

            if (g_pEEInterface->IsInPrologOrEpilog((BYTE*)GetIP(&ctx), &prologSize))
            {
                LOG((LF_CORDB, LL_INFO1000, "D::ITCI: thread is after a BP and in prolog or epilog.\n"));
                invalid = TRUE;
            }
        }
    }
    else
    {
        // If we can't get the context, then its definetly invalid... ;)
        LOG((LF_CORDB, LL_INFO1000, "D::ITCI: couldn't get thread's context!\n"));
        invalid = TRUE;
    }

    return invalid;
}

/* ------------------------------------------------------------------------ *
 * DebuggerHeap impl
 * ------------------------------------------------------------------------ */

DebuggerHeap::~DebuggerHeap()
{
    if (m_heap != NULL)
    {
        delete m_heap;
        m_heap = NULL;

        DeleteCriticalSection(&m_cs);
    }
}

HRESULT DebuggerHeap::Init(char *name)
{
    // Allocate a new heap object.
    m_heap = new gmallocHeap();

    if (m_heap != NULL)
    {
        // Init the heap
        HRESULT hr = m_heap->Init(name);

        if (SUCCEEDED(hr))
        {
            // Init the critical section we'll use to lock the heap.
            InitializeCriticalSection(&m_cs);

            return S_OK;
        }
        else
        {
            // Init failed, so delete the heap.
            delete m_heap;
            m_heap = NULL;
            
            return hr;
        }
    }
    else
        return E_OUTOFMEMORY;
}

void *DebuggerHeap::Alloc(DWORD size)
{
    void *ret;

    _ASSERTE(m_heap != NULL);
    
    EnterCriticalSection(&m_cs);
    ret = m_heap->Alloc(size);
    LeaveCriticalSection(&m_cs);

    return ret;
}

void *DebuggerHeap::Realloc(void *pMem, DWORD newSize)
{
    void *ret;

    _ASSERTE(m_heap != NULL);
    
    EnterCriticalSection(&m_cs);
    ret = m_heap->ReAlloc(pMem, newSize);
    LeaveCriticalSection(&m_cs);
    
    return ret;
}

void DebuggerHeap::Free(void *pMem)
{
    if (pMem != NULL)
    {
        _ASSERTE(m_heap != NULL);
    
        EnterCriticalSection(&m_cs);
        m_heap->Free(pMem);
        LeaveCriticalSection(&m_cs);
    }
}

/******************************************************************************
 *
 ******************************************************************************/

