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
// ===========================================================================
// File: CEEMAIN.CPP
// 
// ===========================================================================

#include "common.h"

// declare global variables
#define DECLARE_DATA
#include "vars.hpp"
#include "veropcodes.hpp"
#undef DECLARE_DATA

#include "dbgalloc.h"
#include "log.h"
#include "ceemain.h"
#include "clsload.hpp"
#include "object.h"
#include "hash.h"
#include "ecall.h"
#include "ceemain.h"
#include "ndirect.h"
#include "syncblk.h"
#include "commember.h"
#include "comstring.h"
#include "comsystem.h"
#include "eeconfig.h"
#include "stublink.h"
#include "handletable.h"
#include "method.hpp"
#include "codeman.h"
#include "gcscan.h"
#include "frames.h"
#include "threads.h"
#include "stackwalk.h"
#include "gc.h"
#include "interoputil.h"
#include "security.h"
#include "nstruct.h"
#include "dbginterface.h"
#include "eedbginterfaceimpl.h"
#include "debugdebugger.h"
#include "cordbpriv.h"
#include "remoting.h"
#include "comdelegate.h"
#include "appdomain.hpp"
#include "cormap.hpp"
#include "perfcounters.h"
#include "rwlock.h"
#include "ipcmanagerinterface.h"
#include "tpoolwrap.h"
#include "internaldebug.h"
#include "corhost.h"
#include "binder.h"
#include "olevariant.h"
#include "comcallwrapper.h"



#include "ipcfunccall.h"
#include "perflog.h"
#include "../dlls/mscorrc/resource.h"

#include "../classlibnative/inc/comnlsinfo.h"

#include "util.hpp"
#include "shimload.h"

#include "comthreadpool.h"

#include "stackprobe.h"
#include "posterror.h"

#include "timeline.h"


#ifdef PROFILING_SUPPORTED 
#include "proftoeeinterfaceimpl.h"
#endif // PROFILING_SUPPORTED

#include "corsvcpriv.h"

#include "strongname.h"
#include "comcodeaccesssecurityengine.h"
#include "syncclean.hpp"
#include <dump-tables.h>

#ifdef CUSTOMER_CHECKED_BUILD
#include "customerdebughelper.h"
#endif

HRESULT RunDllMain(MethodDesc *pMD, HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved);

static HRESULT InitializeIPCManager(void);
static void TerminateIPCManager(void);

static HRESULT InitializeDumpDataBlock();


static int GetThreadUICultureName(LPWSTR szBuffer, int length);
static int GetThreadUICultureParentName(LPWSTR szBuffer, int length);
static int GetThreadUICultureId();


#ifdef DEBUGGING_SUPPORTED
static HRESULT NotifyService();
#endif

BOOL g_fSuspendOnShutdown = FALSE;

#ifdef DEBUGGING_SUPPORTED
static HRESULT InitializeDebugger(void);
static void TerminateDebugger(void);
extern "C" HRESULT __cdecl CorDBGetInterface(DebugInterface** rcInterface);
static void GetDbgProfControlFlag();
#endif // DEBUGGING_SUPPORTED

#ifdef PROFILING_SUPPORTED
static HRESULT InitializeProfiling();
static void TerminateProfiling(BOOL fProcessDetach);
#endif // PROFILING_SUPPORTED

static HRESULT InitializeGarbageCollector();

// This is our Ctrl-C, Ctrl-Break, etc. handler.
static BOOL WINAPI DbgCtrlCHandler(DWORD dwCtrlType)
{
#ifdef DEBUGGING_SUPPORTED
    if (CORDebuggerAttached() &&
        (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT))
    {
        return g_pDebugInterface->SendCtrlCToDebugger(dwCtrlType);      
    }
    else
#endif // DEBUGGING_SUPPORTED
    {
        g_fInControlC = true;     // only for weakening assertions in checked build.
        return FALSE;               // keep looking for a real handler.
    }
}

BOOL g_fEEStarted = FALSE;

// ---------------------------------------------------------------------------
// %%Function: GetStartupInformation
// 
// Get Configuration Information
// 
// ---------------------------------------------------------------------------

typedef HRESULT (STDMETHODCALLTYPE* pGetHostConfigurationFile)(LPCWSTR, DWORD*);
void GetStartupInformation()
{
}


// ---------------------------------------------------------------------------
// %%Function: EEStartup
// 
// Parameters:
//  fFlags                  - Initialization flags for the engine.  See the
//                              COINITIEE enumerator for valid values.
// 
// Returns:
//  S_OK                    - On success
// 
// Description:
//  Reserved to initialize the EE runtime engine explicitly.  Right now most
//  work is actually done inside the DllMain.
// ---------------------------------------------------------------------------

void InitFastInterlockOps(); // cgenxxx.cpp
// Start up and shut down critical section, spin lock
CRITICAL_SECTION          g_LockStartup;

// Remember how the last startup of EE went.
HRESULT g_EEStartupStatus;

void OutOfMemoryCallbackForEE()
{
    FailFast(GetThread(),FatalOutOfMemory);
}

// EEStartup: all execution engine specific stuff should go
// in here

HRESULT EEStartup(DWORD fFlags)
{
    // Obtain process heap
    g_hProcessHeap = GetProcessHeap();

#ifdef _DEBUG
    CrstBase::InitializeDebugCrst();
#endif

    ::SetConsoleCtrlHandler(DbgCtrlCHandler, TRUE/*add*/);
    FnUtilCodeCallback fn = OutOfMemoryCallbackForEE;
    UtilCodeCallback::RegisterOutOfMemoryCallback(fn);

#if ENABLE_TIMELINE
    Timeline::Startup();
#endif

    // Stack probes have no dependencies
    InitStackProbes();

    // A hash of all function type descs on the system (to maintain type desc
    // identity).    
    InitializeCriticalSection(&g_sFuncTypeDescHashLock);
    LockOwner lock = {&g_sFuncTypeDescHashLock, IsOwnerOfOSCrst};
    g_sFuncTypeDescHash.Init(20, &lock);

    InitEventStore();

    // Go get configuration information this is necessary
    // before the EE has started.
    GetStartupInformation();

    HRESULT hr = S_OK;
    if (FAILED(hr = CoInitializeCor(COINITCOR_DEFAULT)))
        return hr;

    g_fEEInit = true;

    // Set the COR system directory for side by side
    IfFailGo(SetInternalSystemDirectory());

    // get any configuration information from the registry
    if (!g_pConfig)
    {
        EEConfig *pConfig = new EEConfig();
        IfNullGo(pConfig);

        PVOID pv = InterlockedCompareExchangePointer(
            (PVOID *) &g_pConfig, (PVOID) pConfig, NULL);

        if (pv != NULL)
            delete pConfig;
    }

    g_pConfig->sync();

    if (g_pConfig->GetConfigDWORD(L"BreakOnEELoad", 0)) {
#ifdef _DEBUG
        _ASSERTE(!"Loading EE!");
#else
                DebugBreak();
#endif
        }

#ifdef LOGGING
    InitializeLogging();
#endif            

#ifdef ENABLE_PERF_LOG
    PerfLog::PerfLogInitialize();
#endif //ENABLE_PERF_LOG


    // Initialize all our InterProcess Communications with COM+
    IfFailGo(InitializeIPCManager());

#ifdef ENABLE_PERF_COUNTERS 
    hr = PerfCounters::Init();
    _ASSERTE(SUCCEEDED(hr));
    IfFailGo(hr);
#endif

    // This should be false but lets reset it anyways 
    g_SystemLoad = false;
    
    // Set callbacks so that LoadStringRC knows which language our
    // threads are in so that it can return the proper localized string.
    SetResourceCultureCallbacks(
        GetThreadUICultureName,
        GetThreadUICultureId,
        GetThreadUICultureParentName
    );

    // Set up the cor handle map. This map is used to load assemblies in
    // memory instead of using the normal system load
    IfFailGo(CorMap::Attach());

    if (!EECodeInfo::Init())
        IfFailGo(E_FAIL);
    if (!HardCodedMetaSig::Init())
        IfFailGo(E_FAIL);
    if (!Stub::Init())
        IfFailGo(E_FAIL);
    if (!StubLinkerCPU::Init())
        IfFailGo(E_FAIL);
      // weak_short, weak_long, strong; no pin
    if(! Ref_Initialize())
        IfFailGo(E_FAIL);

    // Initialize remoting
    if(!CRemotingServices::Initialize())
        IfFailGo(E_FAIL);

    // Initialize contexts
    if(!Context::Initialize())
        IfFailGo(E_FAIL);

    if (!InitThreadManager())
        IfFailGo(E_FAIL);

#ifdef REMOTING_PERF
    CRemotingServices::OpenLogFile();
#endif


    // Initialize RWLocks
    CRWLock::ProcessInit();

#ifdef DEBUGGING_SUPPORTED
    // Check the debugger/profiling control environment variable to
    // see if there's any work to be done.
    GetDbgProfControlFlag();
#endif // DEBUGGING_SUPPORTED

    // Setup the domains. Threads are started in a default domain.
    IfFailGo(SystemDomain::Attach());

#ifdef DEBUGGING_SUPPORTED
    // This must be done before initializing the debugger services so that
    // if the client chooses to attach the debugger that it gets in there
    // in time for the initialization of the debugger services to
    // recognize that someone is already trying to attach and get everything
    // to work accordingly.
    IfFailGo(NotifyService());

    //
    // Initialize the debugging services. This must be done before any
    // EE thread objects are created, and before any classes or
    // modules are loaded.
    //
    hr = InitializeDebugger();
    _ASSERTE(SUCCEEDED(hr));
    IfFailGo(hr);
#endif // DEBUGGING_SUPPORTED

#ifdef PROFILING_SUPPORTED
    // Initialize the profiling services.
    hr = InitializeProfiling();

    _ASSERTE(SUCCEEDED(hr));
    IfFailGo(hr);
#endif // PROFILING_SUPPORTED

    if (!InitializeExceptionHandling())
        IfFailGo(E_FAIL);

    //
    // Install our global exception filter
    //
    InstallUnhandledExceptionFilter();

    if (SetupThread() == NULL)
        IfFailGo(E_FAIL);

    // Give PerfMon a chance to hook up to us
    IPCFuncCallSource::DoThreadSafeCall();

    if (!InitPreStubManager())
        IfFailGo(E_FAIL);


    // Before setting up the execution manager initialize the first part
    // of the JIT helpers.  
    if (!InitJITHelpers1())
        IfFailGo(E_FAIL);

    if (! SUCCEEDED(InitializeGarbageCollector()) ) 
        IfFailGo(E_FAIL);

    if (! SUCCEEDED(SyncClean::Init(FALSE))) {
        IfFailGo(E_FAIL);
    }

    if (! SyncBlockCache::Attach())
        IfFailGo(E_FAIL);

    // Start up the EE intializing all the global variables
    if (!ECall::Init())
        IfFailGo(E_FAIL);

    if (!NDirect::Init())
        IfFailGo(E_FAIL);

    if (!UMThunkInit())
        IfFailGo(E_FAIL);

    if (!COMDelegate::Init())
        IfFailGo(E_FAIL);

    // Set up the sync block
    if (! SyncBlockCache::Start())
        IfFailGo(E_FAIL);

    if (! ExecutionManager::Init())
        IfFailGo(E_FAIL);

    if (!COMNlsInfo::InitializeNLS())
        IfFailGo(E_FAIL);
    
    // Start up security
    IfFailGo(Security::Start());


    IfFailGo(SystemDomain::System()->Init());

#ifdef PROFILING_SUPPORTED
        
    SystemDomain::NotifyProfilerStartup();
#endif // PROFILING_SUPPORTED


    g_fEEInit = false;

    //
    // Now that we're done initializing, fixup token tables in any modules we've 
    // loaded so far.
    //
    SystemDomain::System()->NotifyNewDomainLoads(SystemDomain::System()->DefaultDomain());

    IfFailGo(SystemDomain::System()->DefaultDomain()->SetupSharedStatics());


#ifdef DEBUGGING_SUPPORTED

    LOG((LF_CORDB, LL_INFO1000, "EEStartup: adding default domain 0x%x\n",
        SystemDomain::System()->DefaultDomain()));
        
    // Make a call to publish the DefaultDomain for the debugger, etc
    SystemDomain::System()->PublishAppDomainAndInformDebugger(
                         SystemDomain::System()->DefaultDomain());
#endif // DEBUGGING_SUPPORTED

    IfFailGo(InitializeMiniDumpBlock());

    IfFailGo(InitializeDumpDataBlock());



    g_fEEStarted = TRUE;

    return S_OK;

ErrExit:
    CoUninitializeCor();
    if (!FAILED(hr))
        hr = E_FAIL;

    g_fEEInit = false;

    return hr;
}

// Low-level mechanism for aborting startup in error conditions
BOOL        g_fExceptionsOK = FALSE;
HRESULT     g_StartupFailure = S_OK;

static LONG FilterStartupException(PEXCEPTION_POINTERS p, PVOID pv)
{
    g_StartupFailure = (HRESULT)p->ExceptionRecord->ExceptionInformation[0];
    // Make sure we got a failure code in this case
    if (!FAILED(g_StartupFailure))
        g_StartupFailure = E_FAIL;
    
    if (p->ExceptionRecord->ExceptionCode == BOOTUP_EXCEPTION_COMPLUS)
    {
        // Don't ever handle the exception in a checked build
#ifndef _DEBUG
        return EXCEPTION_EXECUTE_HANDLER;
#endif
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

HRESULT TryEEStartup(DWORD fFlags)
{
    // If we ever fail starting up, always fail for the same reason from now
    // on.

    if (!FAILED(g_StartupFailure))
    {
        // Catch the BOOTUP_EXCEPTION_COMPLUS exception code - this is 
        // what we throw if COMPlusThrow is called before the EE is initialized

        PAL_TRY
          {
              g_StartupFailure = EEStartup(fFlags);
              g_fExceptionsOK = TRUE;
          }
        PAL_EXCEPT_FILTER (FilterStartupException, NULL)
          {
              // Make sure we got a failure code in this case
              if (!FAILED(g_StartupFailure))
                  g_StartupFailure = E_FAIL;
          }
        PAL_ENDTRY
    }

    return g_StartupFailure;
}







STDAPI ClrCreateManagedInstance(LPCWSTR typeName,
                                REFIID riid,
                                LPVOID FAR *ppv)
{
    if (ppv == NULL)
        return E_POINTER;

    if (typeName == NULL)
        return E_INVALIDARG;

    HRESULT hr = S_OK;
    OBJECTREF Throwable = NULL;
    Thread* pThread = NULL;

    BEGINCANNOTTHROWCOMPLUSEXCEPTION();

    MAKE_UTF8PTR_FROMWIDE(pName, typeName);
    EEClass* pClass = NULL;

    hr = CoInitializeEE(0);
    if (FAILED(hr))
        return hr;

    // Retrieve the current thread.
    pThread = GetThread();
    _ASSERTE(pThread);
    _ASSERTE(!pThread->PreemptiveGCDisabled());

    // Switch to cooperative GC mode.
    pThread->DisablePreemptiveGC();

    COMPLUS_TRY
    {
        GCPROTECT_BEGIN(Throwable)

        AppDomain* pDomain = SystemDomain::GetCurrentDomain();
        _ASSERTE(pDomain);
        
        pClass = pDomain->FindAssemblyQualifiedTypeHandle(pName,
                                                          true,
                                                          NULL,
                                                          NULL, 
                                                          &Throwable).GetClass();
        if (!pClass)
        {
            if(Throwable != NULL)
                COMPlusThrow(Throwable);
            hr = CLASS_E_CLASSNOTAVAILABLE;
        }
        else {
            MethodDesc *pMD = NULL;
            if (pClass->GetMethodTable()->HasDefaultConstructor())
                pMD = pClass->GetMethodTable()->GetDefaultConstructor();
            if (pMD == NULL || !pMD->IsPublic()) {
                hr = COR_E_MEMBERACCESS;
            }
            else {
                // Call class init if necessary
                if (!pClass->DoRunClassInit(&Throwable)) 
                    COMPlusThrow(Throwable);
            }
        }

        GCPROTECT_END();

        // If we failed return
        if(FAILED(hr))
            COMPLUS_LEAVE;


        BOOL fCtorAlreadyCalled = FALSE;
        OBJECTREF       newobj; 

        if (CRemotingServices::RequiresManagedActivation(pClass) != NoManagedActivation)
        {
            fCtorAlreadyCalled = TRUE;
            newobj = CRemotingServices::CreateProxyOrObject(pClass->GetMethodTable(), TRUE);
        }
        else
        {
            newobj = FastAllocateObject(pClass->GetMethodTable());
        }
        
        GCPROTECT_BEGIN(newobj);
    
        // don't call any constructors if we already have called them
        if (!fCtorAlreadyCalled)
            CallDefaultConstructor(newobj);
            
        // return the tear-off
        IUnknown* punk = GetComIPFromObjectRef(&newobj);
        hr = punk->QueryInterface(riid, ppv);
        punk->Release();

        GCPROTECT_END();

        // If we failed return
        if (FAILED(hr))
            COMPLUS_LEAVE;
    }
    COMPLUS_CATCH
    {
        hr = SetupErrorInfo(GETTHROWABLE());        
    }
    COMPLUS_END_CATCH

    // Switch back to preemptive.
    pThread->EnablePreemptiveGC();

    ENDCANNOTTHROWCOMPLUSEXCEPTION();

    return hr;
}




// Force the EE to shutdown now.
void ForceEEShutdown()
{
    Thread *pThread = GetThread();
    BOOL    toggleGC = (pThread && pThread->PreemptiveGCDisabled());

    if (toggleGC)
        pThread->EnablePreemptiveGC();

    // Don't bother to take the lock for this case.
    // EnterCriticalSection(&g_LockStartup);

    if (toggleGC)
        pThread->DisablePreemptiveGC();

    LOG((LF_SYNC, INFO3, "EEShutDown invoked from managed Runtime.Exit()\n"));
    EEShutDown(FALSE);
    SafeExitProcess(SystemNative::LatchedExitCode);   // may have changed during shutdown

    // LeaveCriticalSection(&g_LockStartup);
}


//---------------------------------------------------------------------------
// %%Function: void STDMETHODCALLTYPE CorExitProcess(int exitCode)
// 
// Parameters:
//  int exitCode :: process exit code
// 
// Returns:
//  Nothing
// 
// Description:
//  COM Objects shutdown stuff should be done here
// ---------------------------------------------------------------------------
extern "C" void STDMETHODCALLTYPE CorExitProcess(int exitCode)
{
    if (g_RefCount <=0 || g_fEEShutDown)
        return;
 
    Thread *pThread = SetupThread();
    if (pThread && !(pThread->PreemptiveGCDisabled()))
    {
        pThread->DisablePreemptiveGC();
    }


    // The exit code for the process is communicated in one of two ways.  If the
    // entrypoint returns an 'int' we take that.  Otherwise we take a latched
    // process exit code.  This can be modified by the app via System.SetExitCode().
    SystemNative::LatchedExitCode = exitCode;

    // Bump up the ref-count on the module
    for (int i =0; i<6; i++)
        WszLoadLibrary(MSCOREE_SHIM_W);

    ForceEEShutdown();

}

#if defined(STRESS_HEAP)
#endif
#include "../ildasm/dynamicarray.h"
struct RVAFSE // RVA Field Start & End
{
    BYTE* pbStart;
    BYTE* pbEnd;
};
extern DynamicArray<RVAFSE> *g_drRVAField;

// ---------------------------------------------------------------------------
// %%Function: EEShutDown(BOOL fIsDllUnloading)
// 
// Parameters:
//  BOOL fIsDllUnloading :: is it safe point for full cleanup
// 
// Returns:
//  Nothing
// 
// Description:
//  All ee shutdown stuff should be done here
// ---------------------------------------------------------------------------
void STDMETHODCALLTYPE EEShutDown(BOOL fIsDllUnloading)
{
    Thread * pThisThread = GetThread();
    BOOL fPreemptiveGCDisabled = FALSE;
    if (pThisThread && !(pThisThread->PreemptiveGCDisabled()))
    {
        fPreemptiveGCDisabled = TRUE;
        pThisThread->DisablePreemptiveGC();
    }
//#ifdef DEBUGGING_SUPPORTED
    // This memory touch just insures that the MSDEV debug helpers are 
    // not completely optimized away                              
    extern void* debug_help_array[];
    debug_help_array[0]  = 0;
//#endif // DEBUGGING_SUPPORTED

    // If the process is detaching then set the global state.
    // This is used to get around FreeLibrary problems.
    if(fIsDllUnloading)
        g_fProcessDetach = true;

// may cause AV under Win9x, but I remove it under NT too
// so if there happen to be consequences, devs would see them
//#ifdef _DEBUG
// if (!RunningOnWin95())
//    ::SetConsoleCtrlHandler(DbgCtrlCHandler, FALSE/*remove*/);
//#endif // _DEBUG

    LOG((LF_SYNC, INFO3, "EEShutDown entered\n"));

#ifdef _DEBUG
    if (_DbgBreakCount)
    {
        _ASSERTE(!"An assert was hit before EE Shutting down");
    }
#endif          

#ifdef _DEBUG
    if (g_pConfig->GetConfigDWORD(L"BreakOnEEShutdown", 0))
        _ASSERTE(!"Shutting down EE!");
#endif

#ifdef DEBUGGING_SUPPORTED
    // This is a nasty, terrible, horrible thing. If we're being
    // called from our DLL main, then the odds are good that our DLL
    // main has been called as the result of some person calling
    // ExitProcess. That rips the debugger helper thread away very
    // ungracefully. This check is an attempt to recognize that case
    // and avoid the impending hang when attempting to get the helper
    // thread to do things for us.
    if ((g_pDebugInterface != NULL) && g_fProcessDetach)
        g_pDebugInterface->EarlyHelperThreadDeath();
#endif // DEBUGGING_SUPPORTED

    BOOL fFinalizeOK = FALSE;

    // We only do the first part of the shutdown once.
    static LONG OnlyOne = -1;
    if (FastInterlockIncrement(&OnlyOne) != 0) {
        if (!fIsDllUnloading) {
            // I'm in a regular shutdown -- but another thread got here first.
            // It's a race if I return from here -- I'll call ExitProcess next, and
            // rip things down while the first thread is half-way through a
            // nice cleanup.  Rather than do that, I should just wait until the
            // first thread calls ExitProcess().  I'll die a nice death when that
            // happens.
            Thread *pThread = SetupThread();
            HANDLE h = pThread->GetThreadHandle();
            pThread->EnablePreemptiveGC();
            pThread->DoAppropriateAptStateWait(1,&h,FALSE,INFINITE,TRUE);
            _ASSERTE (!"Should not reach here");
        } else {
            // I'm in the final shutdown and the first part has already been run.
            goto part2;
        }
    }

    // Indicate the EE is the shut down phase.
    g_fEEShutDown |= ShutDown_Start;

    fFinalizeOK = TRUE;


    // We perform the final GC only if the user has requested it through the GC class.
        // We should never do the final GC for a process detach
    if (!g_fProcessDetach)
    {
        g_fEEShutDown |= ShutDown_Finalize1;
        if (pThisThread == NULL) {
            SetupThread ();
            pThisThread = GetThread();
        }
        g_pGCHeap->EnableFinalization();
        fFinalizeOK = g_pGCHeap->FinalizerThreadWatchDog();
    }

    g_RefCount = 0; // reset the ref-count 

    // Ok.  Let's stop the EE.
    if (!g_fProcessDetach)
    {
        g_fEEShutDown |= ShutDown_Finalize2;
        if (fFinalizeOK) {
            fFinalizeOK = g_pGCHeap->FinalizerThreadWatchDog();
        }
        else
            return;
    }
    
    g_fForbidEnterEE = true;

#ifdef PROFILING_SUPPORTED
    // If profiling is enabled, then notify of shutdown first so that the
    // profiler can make any last calls it needs to.  Do this only if we
    // are not detaching
    if (IsProfilerPresent())
    {

        // If EEShutdown is not being called due to a ProcessDetach event, so
        // the profiler should still be present
        if (!g_fProcessDetach)
        {
            LOG((LF_CORPROF, LL_INFO10, "**PROF: EEShutDown entered.\n"));
            g_profControlBlock.pProfInterface->Shutdown((ThreadID) GetThread());
        }

        g_fEEShutDown |= ShutDown_Profiler;
        // Free the interface objects.
        TerminateProfiling(g_fProcessDetach);

        // EEShutdown is being called due to a ProcessDetach event, so the
        // profiler has already been unloaded and we must set the profiler
        // status to profNone so we don't attempt to send any more events to
        // the profiler
        if (g_fProcessDetach)
            g_profStatus = profNone;
    }
#endif // PROFILING_SUPPORTED

    // CoEEShutDownCOM moved to
    // the Finalizer thread. See bug 87809
    if (!g_fProcessDetach)
    {
        g_fEEShutDown |= ShutDown_COM;
        if (fFinalizeOK) {
            g_pGCHeap->FinalizerThreadWatchDog();
        }
        else
            return;
    }

#ifdef _DEBUG
    else
        g_fEEShutDown |= ShutDown_COM;
#endif


#ifdef _DEBUG
    g_fEEShutDown |= ShutDown_SyncBlock;
#endif    
    
#ifdef _DEBUG
    // This releases any metadata interfaces that may be held by leaked
    // ISymUnmanagedReaders or Writers. We do this in phase two now that
    // we know any such readers or writers can no longer be in use.
    //
    // Note: we only do this in a debug build to support our wacky leak
    // detection.
    if (g_fProcessDetach)
        Module::ReleaseMemoryForTracking();
#endif

    // Save the security policy cache as necessary.
    Security::SaveCache();
    // Cleanup memory used by security engine.
    COMCodeAccessSecurityEngine::CleanupSEData();

    // This is the end of Part 1.
part2:

        
#ifdef REMOTING_PERF
    CRemotingServices::CloseLogFile();
#endif

    // On the new plan, we only do the tear-down under the protection of the loader
    // lock -- after the OS has stopped all other threads.
    if (fIsDllUnloading)
    {
        g_fEEShutDown |= ShutDown_Phase2;

        TerminateEventStore();

        SyncClean::Terminate();

        
        // Shutdown finalizer before we suspend all background threads. Otherwise we
        // never get to finalize anything. Obviously.
        
#ifdef _DEBUG
        if (_DbgBreakCount)
        {
            _ASSERTE(!"An assert was hit After Finalizer run");
        }
#endif          
    
        // No longer process exceptions
        g_fNoExceptions = true;
    
        //
        // Remove our global exception filter. If it was NULL before, we want it to be null now.
        //
        UninstallUnhandledExceptionFilter();

    
        Thread *t = GetThread();
    
    
        // 
        SystemDomain::DetachBegin();
        
        if (t != NULL)
        {
    
#ifdef DEBUGGING_SUPPORTED
            //
            // If we're debugging, let the debugger know that this thread is
            // gone. Need to do this here for the final thread because the 
            // DetachThread() function relies on the AppDomain object being around
            if (CORDebuggerAttached())
                g_pDebugInterface->DetachThread(t);
#endif // DEBUGGING_SUPPORTED
        }
        

#ifdef DEBUGGING_SUPPORTED
        // inform debugger that it should ignore any "ThreadDetach" events
        // after this point...
        if (CORDebuggerAttached())
            g_pDebugInterface->IgnoreThreadDetach();
#endif // DEBUGGING_SUPPORTED
     
        // Before we detach from the system domain we need to release all the exposed
        // thread objects. This is required since otherwise the thread's will later try
        // to access their exposed objects which will have been destroyed. Also in here
        // we will clear the threads' context and AD before these are deleted in DetachEnd

#ifdef DEBUGGING_SUPPORTED
        // Terminate the debugging services.
        // The profiling infrastructure makes one last call (to Terminate),
        // wherein the profiler may call into the in-proc debugger.  Since
        // we don't do much herein, we put this after TerminateProfiling.
        TerminateDebugger();
#endif // DEBUGGING_SUPPORTED

        Ref_Shutdown(); // shut down the handle table

        // Terminate Perf Counters as late as we can (to get the most data)
#ifdef ENABLE_PERF_COUNTERS

        PerfCounters::Terminate();
#endif // ENABLE_PERF_COUNTERS
    
        // Terminate the InterProcess Communications with COM+
        TerminateIPCManager();
    
#ifdef ENABLE_PERF_LOG
        PerfLog::PerfLogDone();
#endif //ENABLE_PERF_LOG

        // Give PerfMon a chance to hook up to us
        // Have perfmon resync list *after* we close IPC so that it will remove 
        // this process
        IPCFuncCallSource::DoThreadSafeCall();

    
        TerminateStackProbes();

#ifdef ENABLE_TIMELINE        
        Timeline::Shutdown();
#endif
#ifdef _DEBUG
        if (_DbgBreakCount)
            _ASSERTE(!"EE Shutting down after an assert");
#endif          

#ifdef _DEBUG
#endif

#ifdef _DEBUG
#endif

#ifdef LOGGING
	extern unsigned FcallTimeHist[];
#endif
    LOG((LF_STUBS, LL_INFO10, "FcallHist %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d\n",
    FcallTimeHist[0], FcallTimeHist[1], FcallTimeHist[2], FcallTimeHist[3],
    FcallTimeHist[4], FcallTimeHist[5], FcallTimeHist[6], FcallTimeHist[7],
    FcallTimeHist[8], FcallTimeHist[9], FcallTimeHist[10]));

#ifdef LOGGING
        ShutdownLogging();
#endif          


        if (pThisThread && fPreemptiveGCDisabled)
        {
            pThisThread->EnablePreemptiveGC();
        }


    }


}

// ---------------------------------------------------------------------------
// %%Function: CanRunManagedCode()
// 
// Parameters:
//  none
// 
// Returns:
//  true or false
// 
// Description: Indicates if one is currently allowed to run managed code.
// ---------------------------------------------------------------------------
bool CanRunManagedCode()
{

    // If we are shutting down the runtime, then we cannot run code.
    if (g_fForbidEnterEE == true)
        return false;

    // If we are finaling live objects or processing ExitProcess event,
    // we can not allow managed method to run unless the current thread
    // is the finalizer thread
    if ((g_fEEShutDown & ShutDown_Finalize2) && GetThread() != g_pGCHeap->GetFinalizerThread())
        return false;
    
        // If pre-loaded objects are not present, then no way.
    if (g_pPreallocatedOutOfMemoryException == NULL)
        return false;
    return true;
}


// ---------------------------------------------------------------------------
// %%Function: CoInitializeEE(DWORD fFlags)
// 
// Parameters:
//  fFlags                  - Initialization flags for the engine.  See the
//                              COINITIEE enumerator for valid values.
// 
// Returns:
//  Nothing
// 
// Description:
//  Must be called by client on shut down in order to free up the system.
// ---------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE CoInitializeEE(DWORD fFlags)
{   
    LOCKCOUNTINCL("CoInitializeEE in Ceemain");

    EnterCriticalSection(&g_LockStartup);
    // Increment RefCount, if it is one then we
    // need to initialize the EE.
    g_RefCount++;
    
    if(g_RefCount <= 1 && !g_fEEStarted && !g_fEEInit) {
        g_EEStartupStatus = TryEEStartup(fFlags);
        // We did not have a Thread structure when we EnterCriticalSection.
        // Bump the counter now to account for it.
        INCTHREADLOCKCOUNT();
        if(SUCCEEDED(g_EEStartupStatus) && (fFlags & COINITEE_MAIN) == 0) {
            SystemDomain::SetupDefaultDomain();
        }
    }

    LeaveCriticalSection(&g_LockStartup);
    LOCKCOUNTDECL("CoInitializeEE in Ceemain");


    return SUCCEEDED(g_EEStartupStatus) ? (SetupThread() ? S_OK : E_OUTOFMEMORY) : g_EEStartupStatus;
}


// ---------------------------------------------------------------------------
// %%Function: CoUninitializeEE
// 
// Parameters:
//  BOOL fIsDllUnloading :: is it safe point for full cleanup
// 
// Returns:
//  Nothing
// 
// Description:
//  Must be called by client on shut down in order to free up the system.
// ---------------------------------------------------------------------------
void STDMETHODCALLTYPE CoUninitializeEE(BOOL fIsDllUnloading)
{
    BOOL bMustShutdown = FALSE;
    
    // Take a lock and decrement the 
    // ref count. If it reaches 0 then
    // release the VM
    LOCKCOUNTINCL("CoUnInitializeEE in Ceemain");

    EnterCriticalSection(&g_LockStartup);
    if (g_RefCount > 0)
    {       
        g_RefCount--;   
        if(g_RefCount == 0) 
        {
            if (!fIsDllUnloading)
            {
                bMustShutdown = TRUE;
            } 
        }       
    }

    LeaveCriticalSection(&g_LockStartup);
    LOCKCOUNTDECL("CoUninitializeEE in Ceemain");
    
    if( bMustShutdown == TRUE )
    {
        LOG((LF_SYNC, INFO3, "EEShutDown invoked from CoUninitializeEE\n"));
        EEShutDown(fIsDllUnloading);
    }
}


//*****************************************************************************
// This entry point is called from the native DllMain of the loaded image.  
// This gives the COM+ loader the chance to dispatch the loader event.  The 
// first call will cause the loader to look for the entry point in the user 
// image.  Subsequent calls will dispatch to either the user's DllMain or
// their Module derived class.
//*****************************************************************************
BOOL STDMETHODCALLTYPE _CorDllMain(     // TRUE on success, FALSE on error.
    HINSTANCE   hInst,                  // Instance handle of the loaded module.
    DWORD       dwReason,               // Reason for loading.
    LPVOID      lpReserved              // Unused.
    )
{
    BOOL retval;


    retval = ExecuteDLL(hInst,dwReason,lpReserved);

    return retval;
}



static BOOL CacheCommandLine(LPWSTR pCmdLine, LPWSTR* ArgvW)
{
    if (pCmdLine) {
        size_t len = wcslen(pCmdLine);
        g_pCachedCommandLine = new WCHAR[len+1];
        if (!g_pCachedCommandLine)
            return FALSE;
        wcscpy(g_pCachedCommandLine, pCmdLine);
    }

    if (ArgvW != NULL && ArgvW[0] != NULL) {
        WCHAR wszModuleName[MAX_PATH];
        WCHAR wszCurDir[MAX_PATH];
        if (!WszGetCurrentDirectory(MAX_PATH, wszCurDir))
            return FALSE;
        if (PathCombine(wszModuleName, wszCurDir, ArgvW[0]) == NULL)
            return FALSE;

        size_t len = wcslen(wszModuleName);
        g_pCachedModuleFileName = new WCHAR[len+1];
        if (!g_pCachedModuleFileName)
            return FALSE;
        wcscpy(g_pCachedModuleFileName, wszModuleName);
    }

    return TRUE;
}

#ifdef _MSC_VER
#pragma warning(disable:4702)
#endif
//*****************************************************************************
// This entry point is called from the native entry piont of the loaded 
// executable image.  The command line arguments and other entry point data
// will be gathered here.  The entry point for the user image will be found
// and handled accordingly.
//*****************************************************************************
__int32 STDMETHODCALLTYPE _CorExeMain2( // Executable exit code.
    PBYTE   pUnmappedPE,                // -> memory mapped code
    DWORD   cUnmappedPE,                // Size of memory mapped code
    LPWSTR  pImageNameIn,               // -> Executable Name
    LPWSTR  pLoadersFileName,           // -> Loaders Name
    LPWSTR  pCmdLine)                   // -> Command Line
{
    BOOL bRetVal = 0;
    PEFile *pFile = NULL;
    HRESULT hr = E_FAIL;

    // Strong name validate if necessary.
    if (!StrongNameSignatureVerification(pImageNameIn,
                                         SN_INFLAG_INSTALL|SN_INFLAG_ALL_ACCESS|SN_INFLAG_RUNTIME,
                                         NULL) &&
        StrongNameErrorInfo() != (DWORD) CORSEC_E_MISSING_STRONGNAME) {
        LOG((LF_ALL, LL_INFO10, "Program exiting due to strong name verification failure\n"));
        return -1;
    }

    // Before we initialize the EE, make sure we've snooped for all EE-specific
    // command line arguments that might guide our startup.
    CorCommandLine::SetArgvW(pCmdLine);

    if (!CacheCommandLine(pCmdLine, CorCommandLine::GetArgvW(NULL))) {
        LOG((LF_ALL, LL_INFO10, "Program exiting - CacheCommandLine failed\n"));
        return -1;
    }

    HRESULT result = CoInitializeEE(COINITEE_DEFAULT);
    if (FAILED(result)) {
        VMDumpCOMErrors(result);
        SetLatchedExitCode (-1);
        goto exit;
    }

    // SystemDomain::ExecuteMainMethod has THROWSCOMPLUSEXCEPTION
    // This is here also to get the ZAPMONITOR working correctly
    INSTALL_COMPLUS_EXCEPTION_HANDLER();
    hr = PEFile::Create(pUnmappedPE, cUnmappedPE, 
                        pImageNameIn, 
                        pLoadersFileName, 
                        NULL, 
                        &pFile,
                        FALSE);

    if (SUCCEEDED(hr)) {
        // Executables are part of the system domain
        hr = SystemDomain::ExecuteMainMethod(pFile, pImageNameIn);
        bRetVal = SUCCEEDED(hr);
    }

    if (!bRetVal) {
        // The only reason I've seen this type of error in the wild is bad 
        // metadata file format versions and inadequate error handling for 
        // partially signed assemblies.  While this may happen during 
        // development, our customers should not get here.  This is a back-stop 
        // to catch CLR bugs. If you see this, please try to find a better way 
        // to handle your error, like throwing an unhandled exception.
        CorMessageBoxCatastrophic(NULL, IDS_EE_COREXEMAIN2_FAILED_TEXT, IDS_EE_COREXEMAIN2_FAILED_TITLE, MB_ICONSTOP, TRUE);
        SetLatchedExitCode (-1);
    }

    UNINSTALL_COMPLUS_EXCEPTION_HANDLER();


exit:
    LOG((LF_ALL, LL_INFO10, "Program exiting: return code = %d\n", GetLatchedExitCode()));

    LOG((LF_SYNC, INFO3, "EEShutDown invoked from _CorExeMain2\n"));
    EEShutDown(FALSE);

    SafeExitProcess(GetLatchedExitCode());
    return (GetLatchedExitCode());        // need this to make compiler happy. Never executes.
}
#ifdef _MSC_VER
#pragma warning(default:4702)
#endif

//*****************************************************************************
// This is the call point to wire up an EXE.  In this case we have the HMODULE
// and just need to make sure we do to correct self referantial things.
//*****************************************************************************

BOOL STDMETHODCALLTYPE ExecuteEXE(HMODULE hMod)
{
    BOOL retval = FALSE;

    _ASSERTE(hMod);
    if (!hMod)
        return retval;

    // Strong name validate if necessary.
    WCHAR wszImageName[MAX_PATH + 1];
    if (!WszGetModuleFileName(hMod, wszImageName, MAX_PATH))
        return retval;
    if(!StrongNameSignatureVerification(wszImageName,
                                          SN_INFLAG_INSTALL|SN_INFLAG_ALL_ACCESS|SN_INFLAG_RUNTIME,
                                          NULL)) 
    {
        HRESULT hrError=StrongNameErrorInfo();
        if (hrError != CORSEC_E_MISSING_STRONGNAME)
        {
            CorMessageBox(NULL, IDS_EE_INVALID_STRONGNAME, IDS_EE_INVALID_STRONGNAME_TITLE, MB_ICONSTOP, TRUE, wszImageName);

            if (g_fExceptionsOK)
            {
                MAKE_UTF8PTR_FROMWIDE(pImageName,wszImageName);
                PostFileLoadException(pImageName,TRUE,NULL,hrError,THROW_ON_ERROR);
            }
            return retval;
        }
    }


    PEFile *pFile;
    HRESULT hr = PEFile::Create(hMod, &pFile, FALSE);

    if (SUCCEEDED(hr)) {
        // Executables are part of the system domain
        hr = SystemDomain::ExecuteMainMethod(pFile, wszImageName);
        retval = SUCCEEDED(hr);
    }


    
    return retval;
}

//*****************************************************************************
// This is the call point to make a DLL that is already loaded into our address 
// space run. There will be other code to actually have us load a DLL due to a 
// class referance.
//*****************************************************************************
BOOL STDMETHODCALLTYPE ExecuteDLL(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved)
{
    BOOL    ret = FALSE;

    switch (dwReason) 
    {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
        {
            _ASSERTE(hInst);
            if (!hInst)
                return FALSE;

            if (dwReason == DLL_PROCESS_ATTACH) {
                if (FAILED(CoInitializeEE(COINITEE_DLL)))
                    return FALSE;
                else {
                    // If a worker thread does a LoadLibrary and the EE has already
                    // started on another thread then we need to setup this thread 
                    // correctly.
                    if(SetupThread() == NULL)
                        return NULL;
                }
            }
            // IJW assemblies cause the thread doing the process attach to 
            // re-enter ExecuteDLL and do a thread attach. This happens when
            // CoInitializeEE() above executed
            else if (! (GetThread() && GetThread()->GetDomain() && CanRunManagedCode()) )
                return TRUE;

            ret = TRUE;

            break;
        }
        
        default:
        {
            ret = TRUE;

            // If the EE is still intact, the run user entry points.  Otherwise
            // detach was handled when the app domain was stopped.
            if (CanRunManagedCode() &&
                FAILED(SystemDomain::RunDllMain(hInst, dwReason, lpReserved)))
                    ret = FALSE;
            
            // This does need to match the attach. We will only unload dll's 
            // at the end and CoUninitialize will just bounce at 0. WHEN and IF we
            // get around to unloading IL DLL's during execution prior to
            // shutdown we will need to bump the reference one to compensate
            // for this call.
            if (dwReason == DLL_PROCESS_DETACH && !g_fFatalError)
                CoUninitializeEE(TRUE);

            break;
        }
    }

    return ret;
}


//
// Initialize the Garbage Collector
//

HRESULT InitializeGarbageCollector()
{
    HRESULT hr;

    // Build the special Free Object used by the Generational GC
    g_pFreeObjectMethodTable = (MethodTable *) new (nothrow) BYTE[sizeof(MethodTable) - sizeof(SLOT)];
    if (g_pFreeObjectMethodTable == NULL)
        return (E_OUTOFMEMORY);

    // As the flags in the method table indicate there are no pointers
    // in the object, there is no gc descriptor, and thus no need to adjust
    // the pointer to skip the gc descriptor.

    g_pFreeObjectMethodTable->m_BaseSize = ObjSizeOf (ArrayBase);
    g_pFreeObjectMethodTable->m_pEEClass = NULL;
    g_pFreeObjectMethodTable->m_wFlags   = MethodTable::enum_flag_Array | 1 /* ComponentSize */;


   {
        GCHeap *pGCHeap = new (nothrow) GCHeap();
        if (!pGCHeap)
            return (E_OUTOFMEMORY);
        hr = pGCHeap->Initialize();            

        g_pGCHeap = pGCHeap;
    }            

    return(hr);
}



//
// Shutdown the Garbage Collector
//



//*****************************************************************************
// This is the part of the old-style DllMain that initializes the
// stuff that the EE team works on. It's called from the real DllMain
// up in MSCOREE land. Separating the DllMain tasks is simply for
// convenience due to the dual build trees.
//*****************************************************************************
BOOL STDMETHODCALLTYPE EEDllMain( // TRUE on success, FALSE on error.
    HINSTANCE   hInst,             // Instance handle of the loaded module.
    DWORD       dwReason,          // Reason for loading.
    LPVOID      lpReserved)        // Unused.
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            // We cache the SystemInfo for anyone to use throughout the
            // life of the DLL.
            GetSystemInfo(&g_SystemInfo);

            // init sync operations
            InitFastInterlockOps();
            // Initialization lock
            InitializeCriticalSection(&g_LockStartup);
            // Remember module instance
            g_pMSCorEE = hInst;

            break;
        }

        case DLL_PROCESS_DETACH:
        {
            // lpReserved is NULL if we're here because someone called FreeLibrary
            // and non-null if we're here because the process is exiting
            if (lpReserved)
                g_fProcessDetach = TRUE;
            
            if (g_RefCount > 0 || g_fEEStarted)
            {
                Thread *pThread = GetThread();
                if (pThread == NULL)
                    break;
                if (g_pGCHeap->IsGCInProgress()
                    && (pThread != g_pGCHeap->GetGCThread()
                        || !g_fSuspendOnShutdown))
                    break;

                // Deliberately leak this critical section, since we rely on it to
                // coordinate the EE shutdown -- even if we are called from another
                // DLL's DLL_PROCESS_DETACH notification, which is potentially after
                // we received our own detach notification and terminated.
                //
                // DeleteCriticalSection(&g_LockStartup);

                LOG((LF_SYNC, INFO3, "EEShutDown invoked from EEDllMain\n"));
                EEShutDown(TRUE); // shut down EE if it was started up
            }
            break;
        }

        case DLL_THREAD_DETACH:
        {
            // Don't destroy threads here if we're in shutdown (shutdown will
            // clean up for us instead).

            // Don't use GetThread because perhaps we didn't initialize yet, or we
            // have already shutdown the EE.  Note that there is a race here.  We
            // might ask for TLS from a slot we just released.  We are assuming that
            // nobody re-allocates that same slot while we are doing this.  It just
            // isn't worth locking for such an obscure case.
            DWORD   tlsVal = GetThreadTLSIndex();

            if (tlsVal != (DWORD)-1 && CanRunManagedCode())
            {
                Thread  *thread = (Thread *) ::TlsGetValue(tlsVal);
    
                if (thread)
                {
                    DetachThread(thread);
                }
            }
        }
    }

    return TRUE;
}


#ifdef DEBUGGING_SUPPORTED
//*****************************************************************************
// This gets the environment var control flag for Debugging and Profiling
//*****************************************************************************


static void GetDbgProfControlFlag()
{
    // Check the debugger/profiling control environment variable to
    // see if there's any work to be done.
    g_CORDebuggerControlFlags = DBCF_NORMAL_OPERATION;
    
    char buf[32];
    DWORD len = GetEnvironmentVariableA(CorDB_CONTROL_ENV_VAR_NAME,
                                        buf, sizeof(buf));
    _ASSERTE(len < sizeof(buf));

    char *szBad;
    int  iBase;
    if (len > 0 && len < sizeof(buf))
    {
        iBase = (*buf == '0' && (*(buf + 1) == 'x' || *(buf + 1) == 'X')) ? 16 : 10;
        ULONG dbg = strtoul(buf, &szBad, iBase) & DBCF_USER_MASK;

        if (dbg == 1)
            g_CORDebuggerControlFlags |= DBCF_GENERATE_DEBUG_CODE;
    }

    len = GetEnvironmentVariableA(CorDB_CONTROL_REMOTE_DEBUGGING,
                                  buf, sizeof(buf));
    _ASSERTE(len < sizeof(buf));

    if (len > 0 && len < sizeof(buf))
    {
        iBase = (*buf == '0' && (*(buf + 1) == 'x' || *(buf + 1) == 'X')) ? 16 : 10;
        ULONG rmt = strtoul(buf, &szBad, iBase);

        if (rmt == 1)
            g_CORDebuggerControlFlags |= DBCF_ACTIVATE_REMOTE_DEBUGGING;
    }
}
#endif // DEBUGGING_SUPPORTED

#ifdef PROFILING_SUPPORTED

//*****************************************************************************
// This is used to get the proc address by name of a proc in MSCORDBC.DLL
// It will perform a LoadLibrary if necessary.
//*****************************************************************************
static HRESULT GetDBCProc(char *szProcName, FARPROC *pProcAddr)
{
    _ASSERTE(szProcName != NULL);
    _ASSERTE(pProcAddr != NULL);

    HRESULT  hr = S_OK;
    Thread  *thread = GetThread();
    BOOL     toggleGC = (thread && thread->PreemptiveGCDisabled());

    if (toggleGC)
        thread->EnablePreemptiveGC();

    // If the library hasn't already been loaded, do so
    if (g_pDebuggerDll == NULL)
    {
        DWORD lgth = _MAX_PATH + 1;
        WCHAR wszFile[_MAX_PATH + 1];
        hr = GetInternalSystemDirectory(wszFile, &lgth);
        if(FAILED(hr)) goto leav;

        wcscat(wszFile, MAKEDLLNAME_W(L"mscordbc"));

        g_pDebuggerDll = WszLoadLibrary(wszFile);

        if (g_pDebuggerDll == NULL)
        {
            LOG((LF_CORPROF | LF_CORDB, LL_INFO10,
                 "MSCORDBC.DLL not found.\n"));
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto leav;
        }
    }
    _ASSERTE(g_pDebuggerDll != NULL);

    // Get the pointer to the requested function
    *pProcAddr = GetProcAddress(g_pDebuggerDll, szProcName);

    // If the proc address was not found, return error
    if (pProcAddr == NULL)
    {
        LOG((LF_CORPROF | LF_CORDB, LL_INFO10,
             "'%s' not found in MSCORDBC.DLL\n"));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto leav;
    }

leav:

    if (toggleGC)
        thread->DisablePreemptiveGC();

    return hr;
}

void LogProfFailure(PCSTR szMsg)
{
    WCHAR wzMsg[200];
    if (WszMultiByteToWideChar(CP_ACP, 0, szMsg, -1, wzMsg, sizeof(wzMsg)/sizeof(WCHAR)))
    {
        WszMessageBoxInternal(NULL, wzMsg, L"Profiling failure", MB_OK);
    }
}

#define LOGPROFFAILURE(msg) LogProfFailure(msg)

/*
 * This will initialize the profiling services, if profiling is enabled.
 */

#define ENV_PROFILER L"COR_PROFILER"
#define ENV_PROFILER_A "COR_PROFILER"
#define ENV_PROFILER_DLL L"COR_PROFILER_DLL"
#define ENV_PROFILER_DLL_A "COR_PROFILER_DLL"
static HRESULT InitializeProfiling()
{
    HRESULT hr;

    // This has to be called to initialize the WinWrap stuff so that WszXXX
    // may be called.
    OnUnicodeSystem();

    g_profControlBlock.Init();

    // Find out if profiling is enabled
    DWORD fProfEnabled = g_pConfig->GetConfigDWORD(CorDB_CONTROL_ProfilingL, 0, REGUTIL::COR_CONFIG_ALL, FALSE);
    
    // If profiling is not enabled, return.
    if (fProfEnabled == 0)
    {
        LOG((LF_CORPROF, LL_INFO10, "**PROF: Profiling not enabled.\n"));
        return (S_OK);
    }

    LOG((LF_CORPROF, LL_INFO10, "**PROF: Initializing Profiling Services.\n"));

    // Get the CLSID of the profiler to CoCreate
    LPWSTR wszCLSID = g_pConfig->GetConfigString(ENV_PROFILER, FALSE);
    LPWSTR wszProfilerDLL = g_pConfig->GetConfigString(ENV_PROFILER_DLL, FALSE);

    // If the environment variable doesn't exist, profiling is not enabled.
    if (wszCLSID == NULL)
    {
        LOG((LF_CORPROF, LL_INFO10, "**PROF: Profiling flag set, but required "
             "environment variable does not exist.\n"));

        LOGPROFFAILURE("Profiling flag set, but required environment ("
                       ENV_PROFILER_A ") was not set.");

        return (S_FALSE);
    }
    // If the environment variable doesn't exist, profiling is not enabled.
    if (wszProfilerDLL == NULL)
    {
        LOG((LF_CORPROF, LL_INFO10, "**PROF: Profiling flag set, but required "
             "environment variable does not exist.\n"));

        LOGPROFFAILURE("Profiling flag set, but required environment ("
                       ENV_PROFILER_DLL_A ") was not set.");

        return (S_FALSE);
    }

    //*************************************************************************
    // Create the EE interface to provide to the profiling services
    ProfToEEInterface *pProfEE =
        (ProfToEEInterface *) new (nothrow) ProfToEEInterfaceImpl();

    if (pProfEE == NULL)
        return (E_OUTOFMEMORY);

    // Initialize the interface
    hr = pProfEE->Init();

    if (FAILED(hr))
    {
        LOG((LF_CORPROF, LL_INFO10, "**PROF: ProfToEEInterface::Init failed.\n"));

        LOGPROFFAILURE("Internal profiling services initialization failure.");

        delete pProfEE;
        delete [] wszCLSID;
        delete [] wszProfilerDLL;
        return (S_FALSE);
    }
    
    //*************************************************************************
    // Provide the EE interface to the Profiling services
    SETPROFTOEEINTERFACE *pSetProfToEEInterface;
    hr = GetDBCProc("SetProfToEEInterface", (FARPROC *)&pSetProfToEEInterface);

    if (FAILED(hr))
    {
        LOGPROFFAILURE("Internal profiling services initialization failure.");

        delete [] wszCLSID;
        delete [] wszProfilerDLL;
        return (S_FALSE);
    }

    _ASSERTE(pSetProfToEEInterface != NULL);

    // Provide the newly created and inited interface
    pSetProfToEEInterface(pProfEE);

    //*************************************************************************
    // Get the Profiling services interface
    GETEETOPROFINTERFACE *pGetEEToProfInterface;
    hr = GetDBCProc("GetEEToProfInterface", (FARPROC *)&pGetEEToProfInterface);
    _ASSERTE(pGetEEToProfInterface != NULL);

    pGetEEToProfInterface(&g_profControlBlock.pProfInterface);
    _ASSERTE(g_profControlBlock.pProfInterface != NULL);

    // Check if we successfully got an interface to 
    if (g_profControlBlock.pProfInterface == NULL)
    {
        LOG((LF_CORPROF, LL_INFO10, "**PROF: GetEEToProfInterface failed.\n"));

        LOGPROFFAILURE("Internal profiling services initialization failure.");

        pSetProfToEEInterface(NULL);

        delete pProfEE;
        delete [] wszCLSID;
        delete [] wszProfilerDLL;
        return (S_FALSE);
    }

    //*************************************************************************
    // Now ask the profiling services to CoCreate the profiler

    // Indicate that the profiler is in initialization phase
    g_profStatus = profInInit;

    // This will CoCreate the profiler
    hr = g_profControlBlock.pProfInterface->CreateProfiler(wszCLSID, wszProfilerDLL);
    delete [] wszCLSID;
    delete [] wszProfilerDLL;


    if (FAILED(hr))
    {
        LOG((LF_CORPROF, LL_INFO10, "**PROF: No profiler registered, or "
             "CoCreate failed.  Shutting down profiling.\n"));

        LOGPROFFAILURE("Failed to CoCreate profiler.");

        // Notify the profiling services that the EE is shutting down
        g_profControlBlock.pProfInterface->Terminate(FALSE);
        g_profControlBlock.pProfInterface = NULL;
        g_profStatus = profNone;

        return (S_FALSE);
    }

    LOG((LF_CORPROF, LL_INFO10, "**PROF: Profiler created and enabled.\n"));


    // If the profiler is interested in tracking GC events, then we must
    // disable concurrent GC since concurrent GC can allocate and kill
    // objects without relocating and thus not doing a heap walk.
    if (CORProfilerTrackGC())
        g_pConfig->SetGCconcurrent(0);

    // If the profiler has requested the use of the inprocess debugging API
    // then we need to initialize the services here.
    if (CORProfilerInprocEnabled())
    {
        hr = g_pDebugInterface->InitInProcDebug();
        _ASSERTE(SUCCEEDED(hr));

        InitializeCriticalSection(&g_profControlBlock.crSuspendLock);
    }

    // Indicate that profiling is properly initialized.
    g_profStatus = profInit;
    return (hr);
}

/*
 * This will terminate the profiling services, if profiling is enabled.
 */
static void TerminateProfiling(BOOL fProcessDetach)
{
    _ASSERTE(g_profStatus != profNone);

    // If we have a profiler interface active, then terminate it.
    if (g_profControlBlock.pProfInterface)
    {
        // Notify the profiling services that the EE is shutting down
        g_profControlBlock.pProfInterface->Terminate(fProcessDetach);
        g_profControlBlock.pProfInterface = NULL;
    }

    // If the profiler has requested the use of the inprocess debugging API
    // then we need to uninitialize the critical section here
    if (CORProfilerInprocEnabled())
    {
#ifdef _DEBUG
        HRESULT hr = 
#endif
             g_pDebugInterface->UninitInProcDebug();
        _ASSERTE(SUCCEEDED(hr));

        DeleteCriticalSection(&g_profControlBlock.crSuspendLock);
    }

    g_profStatus = profNone;
}
#endif // PROFILING_SUPPORTED

#ifdef DEBUGGING_SUPPORTED
//
// InitializeDebugger initialized the Runtime-side COM+ Debugging Services
//
static HRESULT InitializeDebugger(void)
{
    HRESULT hr = S_OK;

    // The right side depends on this, so if it changes, then
    // FIELD_OFFSET_NEW_ENC_DB should be changed, as well.
    _ASSERTE(FIELD_OFFSET_NEW_ENC == 0x07FFFFFB); 
    
    LOG((LF_CORDB, LL_INFO10,
         "Initializing left-side debugging services.\n"));

    FARPROC gi = (FARPROC) &CorDBGetInterface;

    // Init the interface the EE provides to the debugger,
    // ask the debugger for its interface, and if all goes
    // well call Startup on the debugger.
    EEDbgInterfaceImpl::Init();

    if (g_pEEDbgInterfaceImpl == NULL)
        return (E_OUTOFMEMORY);

    typedef HRESULT __cdecl CORDBGETINTERFACE(DebugInterface**);
    hr = ((CORDBGETINTERFACE*)gi)(&g_pDebugInterface);

    if (SUCCEEDED(hr))
    {
        g_pDebugInterface->SetEEInterface(g_pEEDbgInterfaceImpl);
        hr = g_pDebugInterface->Startup();

        if (SUCCEEDED(hr))
        {
            // If there's a DebuggerThreadControl interface, then we
            // need to update the DebuggerSpecialThread list.
            if (CorHost::GetDebuggerThreadControl())
            {
                hr = CorHost::RefreshDebuggerSpecialThreadList();
                _ASSERTE((SUCCEEDED(hr)) && (hr != S_FALSE));
            }

            LOG((LF_CORDB, LL_INFO10,
                 "Left-side debugging services setup.\n"));
        }
        else
            LOG((LF_CORDB, LL_INFO10,
                 "Failed to Startup debugger. HR=0x%08x\n",
                 hr));
    }
    
    if (!SUCCEEDED(hr))
    {   
        LOG((LF_CORDB, LL_INFO10, "Debugger setup failed."
             " HR=0x%08x\n", hr));
        
        EEDbgInterfaceImpl::Terminate();
        g_pDebugInterface = NULL;
        g_pEEDbgInterfaceImpl = NULL;
    }
    
    // If there is a DebuggerThreadControl interface, then it was set before the debugger
    // was initialized and we need to provide this interface now.  If debugging is already
    // initialized then the IDTC pointer is passed in when it is set through CorHost
    IDebuggerThreadControl *pDTC = CorHost::GetDebuggerThreadControl();

    if (SUCCEEDED(hr) && pDTC)
        g_pDebugInterface->SetIDbgThreadControl(pDTC);

    return hr;
}


//
// TerminateDebugger shuts down the Runtime-side COM+ Debugging Services
//
static void TerminateDebugger(void)
{
    // Notify the out-of-process debugger that shutdown of the in-process debugging support has begun. This is only
    // really used in interop debugging scenarios.
    g_pDebugInterface->ShutdownBegun();


    LOG((LF_CORDB, LL_INFO10, "Shutting down left-side debugger services.\n"));
    
    g_pDebugInterface->StopDebugger();
    
    EEDbgInterfaceImpl::Terminate();

    g_CORDebuggerControlFlags = DBCF_NORMAL_OPERATION;
    g_pDebugInterface = NULL;
    g_pEEDbgInterfaceImpl = NULL;

    CorHost::CleanupDebuggerThreadControl();
}

#endif // DEBUGGING_SUPPORTED

// Import from mscoree.obj
HINSTANCE GetModuleInst();

// ---------------------------------------------------------------------------
// Initializes the shared memory block with information required to perform
// a managed minidump.
// ---------------------------------------------------------------------------

HRESULT InitializeDumpDataBlock()
{
#ifdef DEBUGGING_SUPPORTED
    g_ClassDumpData.version = 1;
    ClassDumpTableBlock* block =
      g_pIPCManagerInterface->GetClassDumpTableBlock();
    _ASSERTE(block != NULL);
    block->table = &g_ClassDumpData;
#endif  // DEBUGGING_SUPPORTED
    return S_OK;
}

// ---------------------------------------------------------------------------
// Initializes the shared memory block with information required to perform
// a managed minidump.
// ---------------------------------------------------------------------------
HRESULT InitializeMiniDumpBlock()
{
#ifdef DEBUGGING_SUPPORTED

    // This code is for NTSD's SOS extention
#include "clear-class-dump-defs.h"

#define BEGIN_CLASS_DUMP_INFO_DERIVED(klass, parent) \
    g_ClassDumpData.p##klass##Vtable = (DWORD_PTR) klass::GetFrameVtable();
#define END_CLASS_DUMP_INFO_DERIVED(klass, parent)

#define BEGIN_ABSTRACT_CLASS_DUMP_INFO(klass)
#define END_ABSTRACT_CLASS_DUMP_INFO(klass)

#define BEGIN_ABSTRACT_CLASS_DUMP_INFO_DERIVED(klass, parent)
#define END_ABSTRACT_CLASS_DUMP_INFO_DERIVED(klass, parent)

#define CDI_CLASS_MEMBER_OFFSET(member)

#include "frame-types.h"

#endif // DEBUGGING_SUPPORTED

    return (S_OK);
}


// ---------------------------------------------------------------------------
// Initialize InterProcess Communications for COM+
// 1. Allocate an IPCManager Implementation and hook it up to our interface *
// 2. Call proper init functions to activate relevant portions of IPC block
// ---------------------------------------------------------------------------
static HRESULT InitializeIPCManager(void)
{
        HRESULT hr = S_OK;
        HINSTANCE hInstIPCBlockOwner = 0;

        DWORD pid = 0;
        // Allocate the Implementation. Everyone else will work through the interface
        g_pIPCManagerInterface = new (nothrow) IPCWriterInterface();

        if (g_pIPCManagerInterface == NULL)
        {
                hr = E_OUTOFMEMORY;
                goto errExit;
        }

        pid = GetCurrentProcessId();


        // Do general init
        hr = g_pIPCManagerInterface->Init();

        if (!SUCCEEDED(hr)) 
        {
                goto errExit;
        }

        // Generate IPCBlock for our PID. Note that for the other side of the debugger,
        // they'll hook up to the debuggee's pid (and not their own). So we still
        // have to pass the PID in.
        hr = g_pIPCManagerInterface->CreatePrivateBlockOnPid(pid, FALSE, &hInstIPCBlockOwner);
        
        if (hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS)) 
        {
                // We failed to create the IPC block because it has already been created. This means that 
                // two mscoree's have been loaded into the process.
                WCHAR strFirstModule[256];
                WCHAR strSecondModule[256];

                // Get the name and path of the first loaded MSCOREE.DLL.
                if (!hInstIPCBlockOwner || !WszGetModuleFileName(hInstIPCBlockOwner, strFirstModule, 256))
                        wcscpy(strFirstModule, L"<Unknown>");

                // Get the name and path of the second loaded MSCOREE.DLL.
                if (!WszGetModuleFileName(g_pMSCorEE, strSecondModule, 256))
                        wcscpy(strSecondModule, L"<Unknown>");

                // Load the format strings for the title and the message body.
                CorMessageBox(NULL, IDS_EE_TWO_LOADED_MSCOREE_MSG, IDS_EE_TWO_LOADED_MSCOREE_TITLE, MB_ICONSTOP, TRUE, strFirstModule, strSecondModule);
                goto errExit;
        }

errExit:
        // If any failure, shut everything down.
        if (!SUCCEEDED(hr)) 
            TerminateIPCManager();

        return hr;
}

// ---------------------------------------------------------------------------
// Terminate all InterProcess operations
// ---------------------------------------------------------------------------
static void TerminateIPCManager(void)
{
    if (g_pIPCManagerInterface != NULL)
    {
        g_pIPCManagerInterface->Terminate();

        delete g_pIPCManagerInterface;
        g_pIPCManagerInterface = NULL;
    }
}

// ---------------------------------------------------------------------------
// Impl for LoadStringRC Callback: In VM, we let the thread decide culture
// copy culture name into szBuffer and return length
// ---------------------------------------------------------------------------
static int GetThreadUICultureName(LPWSTR szBuffer, int length)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    Thread * pThread = GetThread();

    if (pThread == NULL) {
        _ASSERT(length > 0);
        szBuffer[0] = 0;
        return 0;
    }

    return pThread->GetCultureName(szBuffer, length, TRUE);
}

// ---------------------------------------------------------------------------
// Impl for LoadStringRC Callback: In VM, we let the thread decide culture
// copy culture name into szBuffer and return length
// ---------------------------------------------------------------------------
static int GetThreadUICultureParentName(LPWSTR szBuffer, int length)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    Thread * pThread = GetThread();

    if (pThread == NULL) {
        _ASSERT(length > 0);
        szBuffer[0] = 0;
        return 0;
    }

    return pThread->GetParentCultureName(szBuffer, length, TRUE);
}

// ---------------------------------------------------------------------------
// Impl for LoadStringRC Callback: In VM, we let the thread decide culture
// Return an int uniquely describing which language this thread is using for ui.
// ---------------------------------------------------------------------------
static int GetThreadUICultureId()
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    Thread * pThread = GetThread();

    if (pThread == NULL) {
        return UICULTUREID_DONTCARE;
    }

    return pThread->GetCultureId(TRUE);
}

#ifdef DEBUGGING_SUPPORTED
// ---------------------------------------------------------------------------
// If requested, notify service of runtime startup
// ---------------------------------------------------------------------------
static HRESULT NotifyService()
{
    HRESULT hr = S_OK;
    ServiceIPCControlBlock *pIPCBlock = g_pIPCManagerInterface->GetServiceBlock();
    _ASSERTE(pIPCBlock);

    if (pIPCBlock->bNotifyService)
    {
        // Used for building terminal server names
        WCHAR *pSharedMemBlockName = L"Global\\" SERVICE_MAPPED_MEMORY_NAME;


        // Open the service's shared memory block
        HANDLE hEventBlock = WszOpenFileMapping(FILE_MAP_READ | FILE_MAP_WRITE,
                                                FALSE, pSharedMemBlockName);
        _ASSERTE(hEventBlock != NULL);

        // Fail gracefully, since this should not bring the entire runtime down
        if (hEventBlock == NULL)
            return (S_FALSE);

        // Get a pointer valid in this process
        ServiceEventBlock *pEventBlock = (ServiceEventBlock *) MapViewOfFile(
            hEventBlock, FILE_MAP_ALL_ACCESS, 0, 0, 0);
        _ASSERTE(pEventBlock != NULL);

        // Check for error
        if (pEventBlock == NULL)
        {
            CloseHandle(hEventBlock);
            return (S_FALSE);
        }

        // Get a handle for the service process, dup handle access
        HANDLE hSvcProc = OpenProcess(PROCESS_DUP_HANDLE, FALSE,
                                      pEventBlock->dwServiceProcId);
        _ASSERTE(hSvcProc != NULL);

        // Check for error
        if (hSvcProc == NULL)
        {
            UnmapViewOfFile(pEventBlock);
            CloseHandle(hEventBlock);
            return (S_FALSE);
        }

        // Handle to this process
        HANDLE hThisProc = GetCurrentProcess();
        _ASSERTE(hThisProc != NULL);

        // Duplicate the service lock into this process
        HANDLE hSvcLock;
        BOOL bSvcLock = DuplicateHandle(hSvcProc, pEventBlock->hSvcLock,
                                        hThisProc, &hSvcLock, 0, FALSE,
                                        DUPLICATE_SAME_ACCESS);
        _ASSERTE(bSvcLock);

        // Check for error
        if (!bSvcLock)
        {
            UnmapViewOfFile(pEventBlock);
            CloseHandle(hEventBlock);
            CloseHandle(hSvcProc);
            CloseHandle(hThisProc);
            return (S_FALSE);
        }

        // Duplicate the service lock into this process
        HANDLE hFreeEventSem;
        BOOL bFreeEventSem =
            DuplicateHandle(hSvcProc, pEventBlock->hFreeEventSem, hThisProc,
                            &hFreeEventSem, 0, FALSE, DUPLICATE_SAME_ACCESS);
        _ASSERTE(bFreeEventSem);

        // Check for error
        if (!bFreeEventSem)
        {
            CloseHandle(hSvcLock);
            UnmapViewOfFile(pEventBlock);
            CloseHandle(hEventBlock);
            CloseHandle(hSvcProc);
            CloseHandle(hThisProc);
            return (S_FALSE);
        }

        // Create the event for continuing
        HANDLE hContEvt = WszCreateEvent(NULL, TRUE, FALSE, NULL);
        _ASSERTE(hContEvt);

        if (hContEvt == NULL)
        {
            CloseHandle(hFreeEventSem);
            CloseHandle(hSvcLock);
            UnmapViewOfFile(pEventBlock);
            CloseHandle(hEventBlock);
            CloseHandle(hSvcProc);
            CloseHandle(hThisProc);
            return (S_OK);
        }

        //
        // If the service notifies a process of this runtime starting up, and
        // that process chooses not to attach, then the service will set this
        // event and the runtime will continue.  If the notified process chooses
        // to attach, then the event is set on this end when the attach is
        // finally complete and we can continue to run.  That's the reason for
        // keeping a hold of it here and also duplicating it into the service.
        //

        // Get a count from the semaphore
        WaitForSingleObject(hFreeEventSem, INFINITE);
        CloseHandle(hFreeEventSem);

        // Grab the service lock
        WaitForSingleObject(hSvcLock, INFINITE);

        if (pIPCBlock->bNotifyService)
        {
            // Get an event from the free list
            ServiceEvent *pEvent = pEventBlock->GetFreeEvent();

            // Fill out the data
            pEvent->eventType = runtimeStarted;
            pEvent->eventData.runtimeStartedData.dwProcId = GetCurrentProcessId();
            pEvent->eventData.runtimeStartedData.hContEvt = hContEvt;

            // Notify the service of the event
            HANDLE hDataAvailEvt;
            BOOL bDataAvailEvt = DuplicateHandle(
                hSvcProc, pEventBlock->hDataAvailableEvt,hThisProc, &hDataAvailEvt,
                0, FALSE, DUPLICATE_SAME_ACCESS);
            _ASSERTE(bDataAvailEvt);

            // Check for error
            if (!bDataAvailEvt)
            {
                // Add the event back to the free list
                pEventBlock->FreeEvent(pEvent);

                // Release the lock
                ReleaseMutex(hSvcLock);

                UnmapViewOfFile(pEventBlock);
                CloseHandle(hEventBlock);
                CloseHandle(hSvcProc);
                CloseHandle(hSvcLock);
                CloseHandle(hThisProc);

                return (S_FALSE);
            }

            // Queue the event
            pEventBlock->QueueEvent(pEvent);

            // Release the lock
            ReleaseMutex(hSvcLock);

            // Indicate that the event is available
            SetEvent(hDataAvailEvt);
            CloseHandle(hDataAvailEvt);

            // Wait until the notification is received and they return.
            WaitForSingleObject(hContEvt, INFINITE);
        }
        else
        {
            // Release the lock
            ReleaseMutex(hSvcLock);
        }

        // Clean up
        UnmapViewOfFile(pEventBlock);
        CloseHandle(hEventBlock);
        CloseHandle(hSvcProc);
        CloseHandle(hThisProc);
        CloseHandle(hSvcLock);
        CloseHandle(hContEvt);
    }

    // Continue with EEStartup
    return (hr);
}
#endif // DEBUGGING_SUPPORTED


// The runtime must be in the appropriate thread mode when we exit, so that we
// aren't surprised by the thread mode when our DLL_PROCESS_DETACH occurs, or when
// other DLLs call Release() on us in their detach [dangerous!], etc.
void SafeExitProcess(int exitCode)
{
    Thread *pThread = (GetThreadTLSIndex() == ~0U ? NULL : GetThread());
    BOOL    bToggleGC = (pThread && pThread->PreemptiveGCDisabled());

    if (bToggleGC)
        pThread->EnablePreemptiveGC();

    ::ExitProcess(exitCode);
}


// ---------------------------------------------------------------------------
// Export shared logging code for JIT, et.al.
// ---------------------------------------------------------------------------
#ifdef _DEBUG

extern VOID LogAssert( LPCSTR szFile, int iLine, LPCSTR expr);
extern "C"
//__declspec(dllexport)
VOID LogHelp_LogAssert( LPCSTR szFile, int iLine, LPCSTR expr)
{
    LogAssert(szFile, iLine, expr);
}

extern BOOL NoGuiOnAssert();
extern "C"
//__declspec(dllexport)
BOOL LogHelp_NoGuiOnAssert()
{
    return NoGuiOnAssert();
}

extern VOID TerminateOnAssert();
extern "C"
//__declspec(dllexport)
VOID LogHelp_TerminateOnAssert()
{
//  __asm int 3;
    TerminateOnAssert();

}

#else // !_DEBUG

extern "C"
//__declspec(dllexport)
VOID LogHelp_LogAssert( LPCSTR szFile, int iLine, LPCSTR expr) {}


extern "C"
//__declspec(dllexport)
BOOL LogHelp_NoGuiOnAssert() { return FALSE; }

extern "C"
//__declspec(dllexport)
VOID LogHelp_TerminateOnAssert() {}

#endif // _DEBUG


#ifndef ENABLE_PERF_COUNTERS
//
// perf counter stubs for builds which don't have perf counter support
// These are needed because we export these functions in our DLL


Perf_Contexts* GetPrivateContextsPerfCounters()
{
    return NULL;
}

Perf_Contexts* GetGlobalContextsPerfCounters()
{
    return NULL;
}


#endif

#ifdef PLATFORM_UNIX
//
// This routine is not directly called from within the CLR.  It is called
// by the developer debugging the CLR, from within GDB.  It's best to put
// this in your .gdbinit file:
//  define sos
//  call (void)SOS("$arg0")
//  echo \n
//  end
//
// Then you can use SOS from the (gdb) prompt like this:
//  sos Help
//  sos DumpStack\ -EE
//               ^
//               | note that space characters must be escaped!
//
extern "C" void __cdecl SOS(char *Command)
{
    static HMODULE hSOS;

    if (!Command) {
        fprintf(stderr, "SOS:  you must specify a command to run\n");
        return;
    }

    if (!hSOS) {
        hSOS = LoadLibraryA(MAKEDLLNAME_A("sos"));
        if (!hSOS) {
            fprintf(stderr, "SOS:  Failed to load " MAKEDLLNAME_A("sos") ".  LastErr=%d\n",
                GetLastError());
            return;
        }
    }

    typedef void (*SOSAPI)(HMODULE, char *);
    static SOSAPI pfnSOSAPI;

    if (!pfnSOSAPI) {
        pfnSOSAPI = (SOSAPI)GetProcAddress(hSOS, "GDBSOS");
        if (!pfnSOSAPI) {
            fprintf(stderr, "SOS:  Couldn't find GDBSOS() in libsos.so");
            return;
        }
    }

    (*pfnSOSAPI)(hSOS, Command);

    // It is a bad idea to load and free libsos back and forth since it is 
    //  just increasing the chance of crashing in the debugged process
    // FreeLibrary(hSOS);
}
#endif //PLATFORM_UNIX


