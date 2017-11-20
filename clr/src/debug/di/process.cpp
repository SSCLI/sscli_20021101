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
// File: process.cpp
//
//*****************************************************************************
#include "stdafx.h"

#ifdef UNDEFINE_RIGHT_SIDE_ONLY
#undef RIGHT_SIDE_ONLY
#endif //UNDEFINE_RIGHT_SIDE_ONLY


#ifndef SM_REMOTESESSION
#define SM_REMOTESESSION 0x1000
#endif

// Get version numbers for IPCHeader stamp
#include "version/__file__.ver"

#include "corpriv.h"
#include "../../dlls/mscorrc/resource.h"

#define CORDB_WAIT_TIMEOUT 360000 // milliseconds

inline DWORD CordbGetWaitTimeout()
{
#ifdef _DEBUG
    // 0 = Wait forever
    // 1 = Wait for CORDB_WAIT_TIMEOUT
    // n = Wait for n milliseconds
    static ConfigDWORD *cordbWaitTimeout = NULL;
    if (cordbWaitTimeout == NULL) {
    	cordbWaitTimeout = new ConfigDWORD;
        memset(cordbWaitTimeout, 0, sizeof(ConfigDWORD));
    }
    DWORD timeoutVal = cordbWaitTimeout->val(L"DbgWaitTimeout", 1);
    if (timeoutVal == 0)
        return DWORD(-1);
    else if (timeoutVal != 1)
        return timeoutVal;
    else    
#endif
    {
        return CORDB_WAIT_TIMEOUT;
    }
}


#ifdef _DEBUG
void vDbgNameEvent(PWCHAR wczName, DWORD dwNameSize, DWORD dwLine, PCHAR szFile, const PWCHAR wczEventName)
{
    MAKE_WIDEPTR_FROMANSI(wczFile, szFile)
    PWCHAR pwc = wczFile;

    // Replace \ characters with . characters 
    // See documenation for CreateEvent. It doesn't like \ characters to be in the name.
    while(*pwc != L'\0')
    {
        if (L'\\' == *pwc)
        {
            *pwc = L'.';
        }
        pwc++;
    }

    // An example name might be "RotorDBI.DI.process.cpp@3203 CrazyWin98WorkAround ProcId=1239"
    swprintf(wczName, L"RotorDBI.%s@%d %s ProcId=%d", wczFile, dwLine, wczEventName, GetCurrentProcessId());
}

LONG CordbBase::m_saDwInstance[enumMaxDerived];
LONG CordbBase::m_saDwAlive[enumMaxDerived];
PVOID CordbBase::m_sdThis[enumMaxDerived][enumMaxThis];

DWORD            g_dwInprocLockOwner = 0;
DWORD            g_dwInprocLockRecursionCount = 0;
#endif

CRITICAL_SECTION g_csInprocLock;
/* ------------------------------------------------------------------------- *
 * CordbBase class
 * ------------------------------------------------------------------------- */
void CordbBase::NeuterAndClearHashtable(CordbHashTable * pCordbHashtable)
{
    HASHFIND hfDT;
    CordbBase * pCordbBase;

    while ((pCordbBase = ((CordbBase *)pCordbHashtable->FindFirst(&hfDT))) != 0)
    {
        pCordbBase->Neuter();
        pCordbHashtable->RemoveBase(pCordbBase->m_id);
    }
} 

/* ------------------------------------------------------------------------- *
 * Cordb class
 * ------------------------------------------------------------------------- */

bool Cordb::m_runningOnNT = false;

Cordb::Cordb()
  : CordbBase(0, enumCordb), 
    m_managedCallback(NULL), m_unmanagedCallback(NULL), m_processes(11),
    m_pMetaDispenser(NULL),
    m_pCorHost(NULL),
    m_crazyWin98WorkaroundEvent(NULL),
#ifndef RIGHT_SIDE_ONLY
    m_procThis(NULL),
#endif //INPROC only
    m_initialized(false)
{
}

Cordb::~Cordb()
{
    LOG((LF_CORDB, LL_INFO10, "C::~C Terminating Cordb object.\n"));
#ifndef RIGHT_SIDE_ONLY
    if (m_rcEventThread != NULL)
    {
        delete m_rcEventThread;
        m_rcEventThread = NULL;
    }  

    if(m_procThis != NULL)
    {
        m_procThis->Release();
        m_procThis = NULL;
    }

    if(m_pMetaDispenser != NULL)
    {
        m_pMetaDispenser->Release();
        m_pMetaDispenser = NULL;
    }
#endif //INPROC only
}

void Cordb::Neuter()
{
    AddRef();
    {
        NeuterAndClearHashtable(&m_processes);

        CordbBase::Neuter();
    }
    Release();
}

HRESULT Cordb::Terminate()
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    LOG((LF_CORDB, LL_INFO10000, "[%x] Terminating Cordb\n", GetCurrentThreadId()));

    if (!m_initialized)
        return E_FAIL;
            
    //
    // Stop the event handling threads.
    //
    if (m_win32EventThread != NULL)
    {
        m_win32EventThread->Stop();
        delete m_win32EventThread;
    }

    if (m_rcEventThread != NULL)
    {
        m_rcEventThread->Stop();
        delete m_rcEventThread;
    }

    if (m_crazyWin98WorkaroundEvent != NULL)
    {
    	CloseHandle(m_crazyWin98WorkaroundEvent);
        m_crazyWin98WorkaroundEvent = NULL;
   	}
    
    //
    // Delete all of the processes.
    //
    CordbBase* entry;
    HASHFIND find;

    for (entry =  m_processes.FindFirst(&find);
         entry != NULL;
         entry =  m_processes.FindNext(&find))
    {
        CordbProcess* p = (CordbProcess*) entry;
        LOG((LF_CORDB, LL_INFO1000, "[%x] Releasing process %d\n",
             GetCurrentThreadId(), p->m_id));
        p->Release();
    }

    DeleteCriticalSection(&m_processListMutex);
    
    //
    // Release the metadata interfaces
    //
    if (m_pMetaDispenser)
        m_pMetaDispenser->Release();
    
    //
    // Release the callbacks
    //
    if (m_managedCallback)
        m_managedCallback->Release();

    if (m_unmanagedCallback)
        m_unmanagedCallback->Release();

    if (m_pCorHost)
    {
        m_pCorHost->Stop();
        m_pCorHost->Release();
        m_pCorHost = NULL;
    }

#ifdef LOGGING
    ShutdownLogging();
#endif

    m_initialized = FALSE;

    return S_OK;
#endif //RIGHT_SIDE_ONLY    
}

HRESULT Cordb::QueryInterface(REFIID id, void **pInterface)
{
    if (id == IID_ICorDebug)
        *pInterface = (ICorDebug*)this;
    else if (id == IID_IUnknown)
        *pInterface = (IUnknown*)(ICorDebug*)this;
    else
    {
        *pInterface = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

//
// Initialize -- setup the ICorDebug object by creating any objects
// that the object needs to operate and starting the two needed IPC
// threads.
//
HRESULT Cordb::Initialize(void)
{
    HRESULT hr = S_OK;
	INPROC_LOCK();
	
    if (!m_initialized)
    {
#ifdef RIGHT_SIDE_ONLY
#ifdef LOGGING
        InitializeLogging();
#endif
#endif //RIGHT_SIDE_ONLY

        LOG((LF_CORDB, LL_INFO10, "Initializing ICorDebug...\n"));

        // Ensure someone hasn't screwed up the IPC buffer size
        _ASSERTE(sizeof(DebuggerIPCEvent) <= CorDBIPC_BUFFER_SIZE);
        
        m_runningOnNT = (RunningOnWinNT() != FALSE);
        
        //
        // Init things that the Cordb will need to operate
        //
        InitializeCriticalSection(&m_processListMutex);
        
#ifdef RIGHT_SIDE_ONLY
        //
        // Create the win32 event listening thread
        //
        m_win32EventThread = new CordbWin32EventThread(this);
        
        if (m_win32EventThread != NULL)
        {
            hr = m_win32EventThread->Init();

            if (SUCCEEDED(hr))
                hr = m_win32EventThread->Start();

            if (FAILED(hr))
            {
                delete m_win32EventThread;
                m_win32EventThread = NULL;
            }
        }
        else
            hr = E_OUTOFMEMORY;

        if (FAILED(hr))
            goto exit;

        NAME_EVENT_BUFFER;
        m_crazyWin98WorkaroundEvent = WszCreateEvent(NULL, FALSE, FALSE, NAME_EVENT(L"CrazyWin98WorkaroundEvent"));
        
        if (m_crazyWin98WorkaroundEvent == NULL)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto exit;
        }

#endif //RIGHT_SIDE_ONLY

        //
        // Create the runtime controller event listening thread
        //
        m_rcEventThread = new CordbRCEventThread(this);

        if (m_rcEventThread == NULL)
            hr = E_OUTOFMEMORY;
            
#ifdef RIGHT_SIDE_ONLY
        else
        {
            // This stuff only creates events & starts the thread-
            // inproc doesn't want to do this
            hr = m_rcEventThread->Init();

            if (SUCCEEDED(hr))
                hr = m_rcEventThread->Start();

            if (FAILED(hr))
            {
                delete m_rcEventThread;
                m_rcEventThread = NULL;
            }
        }
            
        if (FAILED(hr))
            goto exit;

	hr = PAL_CoCreateInstance(CLSID_CorMetaDataDispenser,
				  IID_IMetaDataDispenser,
				  (void **)&m_pMetaDispenser);
        
        if (FAILED(hr))
            goto exit;
        
#else
        hr = MetaDataGetDispenser(CLSID_CorMetaDataDispenser,
                                  IID_IMetaDataDispenser,
                                  (void**)&m_pMetaDispenser);        
        if (FAILED(hr))
            goto exit;

        // Don't need to muck w/ environment variables since we're in
        // the already-running process
        
#endif //RIGHT_SIDE_ONLY
        
        m_initialized = TRUE;
    }
    
exit:
	INPROC_UNLOCK();
    return hr;
}

//
// Do we allow another process?
// This is highly dependent on the wait sets in the Win32 & RCET threads.
//
bool Cordb::AllowAnotherProcess()
{
    bool fAllow;
    
    LockProcessList();

    // Cordb, Win32, and RCET all have process sets, but Cordb's is the
    // best count of total debuggees. The RCET set is volatile (processes
    // are added / removed when they become synchronized), and Win32's set
    // doesn't include all processes.
    int cCurProcess = m_processes.GetCount();

    // In order to accept another debuggee, we must have a free slot in all
    // wait sets. Currently, we don't expose the size of those sets, but
    // we know they're MAXIMUM_WAIT_OBJECTS. Note that we lose one slot
    // to the control event.
    if (cCurProcess >= MAXIMUM_WAIT_OBJECTS - 1)
    {
        fAllow = false;
    } else {
        fAllow = true;
    }
    
    UnlockProcessList();

    return fAllow;
}

//
// AddProcess -- add a process object to this ICorDebug's hash of processes.
// This also tells this ICorDebug's runtime controller thread that the
// process set has changed so it can update its list of wait events.
//
HRESULT Cordb::AddProcess(CordbProcess* process)
{
    // At this point, we should have already checked that we 
    // can have another debuggee.
    _ASSERTE(AllowAnotherProcess());
    
    LockProcessList();

    HRESULT hr = m_processes.AddBase(process);
    
#ifdef RIGHT_SIDE_ONLY
    if (SUCCEEDED(hr))
        m_rcEventThread->ProcessStateChanged();
#endif //RIGHT_SIDE_ONLY
    
    UnlockProcessList();

    return hr;
}

//
// RemoveProcess -- remove a process object from this ICorDebug's hash of
// processes. This also tells this ICorDebug's runtime controller thread
// that the process set has changed so it can update its list of wait events.
//
void Cordb::RemoveProcess(CordbProcess* process)
{
    LockProcessList();
    m_processes.RemoveBase(process->m_id);

#ifdef RIGHT_SIDE_ONLY    
    m_rcEventThread->ProcessStateChanged();
#endif //RIGHT_SIDE_ONLY
    
    UnlockProcessList();    
}

//
// LockProcessList -- Lock the process list.
//
void Cordb::LockProcessList(void)
{
	LOCKCOUNTINC("LockProcessList in Process.cpp");
    EnterCriticalSection(&m_processListMutex);
}

//
// UnlockProcessList -- Unlock the process list.
//
void Cordb::UnlockProcessList(void)
{
    LeaveCriticalSection(&m_processListMutex);
	LOCKCOUNTDECL("UnlockProcessList in Process.cpp");
}


HRESULT Cordb::SendIPCEvent(CordbProcess* process,
                            DebuggerIPCEvent* event,
                            SIZE_T eventSize)
{
    return m_rcEventThread->SendIPCEvent(process, event, eventSize);
}


void Cordb::ProcessStateChanged(void)
{
    m_rcEventThread->ProcessStateChanged();
}


HRESULT Cordb::WaitForIPCEventFromProcess(CordbProcess* process,
                                          CordbAppDomain *pAppDomain,
                                          DebuggerIPCEvent* event)
{
    return m_rcEventThread->WaitForIPCEventFromProcess(process, 
                                                       pAppDomain, 
                                                       event);
}

HRESULT Cordb::GetFirstContinuationEvent(CordbProcess *process, 
                                         DebuggerIPCEvent *event)
{
    return m_rcEventThread->ReadRCEvent(process,
                                        event);
}

HRESULT Cordb::GetNextContinuationEvent(CordbProcess *process, 
                                        DebuggerIPCEvent *event)
{
    _ASSERTE( event->next != NULL );
    if ( event->next == NULL)
        return E_FAIL;
        
    m_rcEventThread->CopyRCEvent((BYTE*)event->next, (BYTE*)event);

    return S_OK;
}

HRESULT Cordb::GetCorRuntimeHost(ICorRuntimeHost **ppHost)
{
    // If its already created, pass it out with an extra reference.
    if (m_pCorHost != NULL)
    {
        m_pCorHost->AddRef();
        *ppHost = m_pCorHost;
        return S_OK;
    }

    // Create the cor host.
    HRESULT hr = PAL_CoCreateInstance(CLSID_CorRuntimeHost,
				      IID_ICorRuntimeHost,
				      (void **)&m_pCorHost);

    if (SUCCEEDED(hr))
    {
        // Start it up.
        hr = m_pCorHost->Start();

        if (SUCCEEDED(hr))
        {
            *ppHost = m_pCorHost;

            // Keep a ref for ourselves.
            m_pCorHost->AddRef();
        }
        else
        {
            m_pCorHost->Release();
            m_pCorHost = NULL;
        }
    }

    return hr;
}

HRESULT Cordb::GetCorDBPrivHelper(ICorDBPrivHelper **ppHelper)
{
    ICorRuntimeHost *pCorHost;

    HRESULT hr = GetCorRuntimeHost(&pCorHost);

    if (SUCCEEDED(hr))
    {
        hr = pCorHost->QueryInterface(IID_ICorDBPrivHelper,
                                      (void**)ppHelper);

        pCorHost->Release();
    }

    return hr;
}

//-----------------------------------------------------------
// ICorDebug
//-----------------------------------------------------------

// Set the handler for callbacks on managed events
// This can not be NULL.
HRESULT Cordb::SetManagedHandler(ICorDebugManagedCallback *pCallback)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    if (!m_initialized)
        return E_FAIL;

    VALIDATE_POINTER_TO_OBJECT(pCallback, ICorDebugManagedCallback*);
    
    if (m_managedCallback)
        m_managedCallback->Release();
    
    m_managedCallback = pCallback;

    if (m_managedCallback != NULL)
        m_managedCallback->AddRef();
    
    return S_OK;
#endif //RIGHT_SIDE_ONLY    
}

HRESULT Cordb::SetUnmanagedHandler(ICorDebugUnmanagedCallback *pCallback)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    if (!m_initialized)
        return E_FAIL;

    VALIDATE_POINTER_TO_OBJECT_OR_NULL(pCallback, ICorDebugUnmanagedCallback*);
    
    if (m_unmanagedCallback)
        m_unmanagedCallback->Release();
    
    m_unmanagedCallback = pCallback;

    if (m_unmanagedCallback != NULL)
    {
        m_unmanagedCallback->AddRef();

        // There is a crazy problem on Win98 with VS7 where they may do a CreateProcess with Win32 attach, but not
        // register a handler yet. So we need to block before sending any unamanged events out. This will release us if
        // we're blocking.
        SetEvent(m_crazyWin98WorkaroundEvent);
    }

    return S_OK;
#endif //RIGHT_SIDE_ONLY        
}

HRESULT Cordb::CreateProcess(LPCWSTR lpApplicationName,
                             LPWSTR lpCommandLine,
                             LPSECURITY_ATTRIBUTES lpProcessAttributes,
                             LPSECURITY_ATTRIBUTES lpThreadAttributes,
                             BOOL bInheritHandles,
                             DWORD dwCreationFlags,
                             PVOID lpEnvironment,
                             LPCWSTR lpCurrentDirectory,
                             LPSTARTUPINFOW lpStartupInfo,
                             LPPROCESS_INFORMATION lpProcessInformation,
                             CorDebugCreateProcessFlags debuggingFlags,
                             ICorDebugProcess **ppProcess)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    _ASSERTE(lpCommandLine == NULL || 
        IsBadWritePtr(lpCommandLine, sizeof(WCHAR) * (wcslen(lpCommandLine) + 1)) == FALSE);

    VALIDATE_POINTER_TO_OBJECT(ppProcess, ICorDebugProcess**);

    if (!m_initialized)
        return E_FAIL;


    // Check that we can even accept another debuggee before trying anything.
    if (!AllowAnotherProcess())
    {
        return CORDBG_E_TOO_MANY_PROCESSES;
    }
            
    HRESULT hr = S_OK;
    
    hr = m_win32EventThread->SendCreateProcessEvent(lpApplicationName,
                                                    lpCommandLine,
                                                    lpProcessAttributes,
                                                    lpThreadAttributes,
                                                    bInheritHandles,
                                                    dwCreationFlags,
                                                    lpEnvironment,
                                                    lpCurrentDirectory,
                                                    lpStartupInfo,
                                                    lpProcessInformation,
                                                    debuggingFlags);

    if (SUCCEEDED(hr))
    {
        LockProcessList();
        CordbProcess *process = (CordbProcess*) m_processes.GetBase(
                                          lpProcessInformation->dwProcessId);
        UnlockProcessList();

        _ASSERTE(process != NULL);

        *ppProcess = (ICorDebugProcess*) process;
        (*ppProcess)->AddRef();

        // also indicate that this process was started under the debugger 
        // as opposed to attaching later.
        process->m_attached = false;
    }

    return hr;
#endif //RIGHT_SIDE_ONLY        
}

HRESULT Cordb::DebugActiveProcess(DWORD processId,
                                  BOOL win32Attach,
                                  ICorDebugProcess **ppProcess)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    if (!m_initialized)
        return E_FAIL;

    VALIDATE_POINTER_TO_OBJECT(ppProcess, ICorDebugProcess **);

    // Check that we can even accept another debuggee before trying anything.
    if (!AllowAnotherProcess())
    {
        return CORDBG_E_TOO_MANY_PROCESSES;
    }

    if (win32Attach)
	return CORDBG_E_INTEROP_NOT_SUPPORTED;
            
    HRESULT hr =
        m_win32EventThread->SendDebugActiveProcessEvent(processId,
                                                        win32Attach == TRUE,
                                                        NULL);

    // If that worked, then there will be a process object...
    if (SUCCEEDED(hr))
    {
        LockProcessList();
        CordbProcess* process =
            (CordbProcess*) m_processes.GetBase(processId);
        UnlockProcessList();

        _ASSERTE(process != NULL);

        // If the process was already setup for attach, then we go
        // ahead and send the attach event and wait for the resulting
        // events to come in. However, if the process wasn't setup for
        // attach, then it may not have entered managed code yet, so
        // we simply wait for the normal sequence of events to occur,
        // as if we created the process.
        if (process->m_sendAttachIPCEvent)
        {

            DebuggerIPCEvent *event =
                (DebuggerIPCEvent*) _alloca(CorDBIPC_BUFFER_SIZE);
            process->InitIPCEvent(event, 
                                  DB_IPCE_ATTACHING, 
                                  false,
                                  NULL);

            LOG((LF_CORDB, LL_INFO1000, "[%x] CP::S: sending attach.\n",
                 GetCurrentThreadId()));

            hr = SendIPCEvent(process, event, CorDBIPC_BUFFER_SIZE);

            LOG((LF_CORDB, LL_INFO1000, "[%x] CP::S: sent attach.\n",
                 GetCurrentThreadId()));

            // Must set this after we've sent the event, as we use this flag to indicate to other parts of interop
            // debugging that we need to be able to send this event.
            process->m_sendAttachIPCEvent = false;
        }

        *ppProcess = (ICorDebugProcess*) process;
        (*ppProcess)->AddRef();

        // also indicate that this process was attached to, as  
        // opposed to being started under the debugger.
        process->m_attached = true;
    }
    
    return hr;
#endif //RIGHT_SIDE_ONLY        
}

HRESULT Cordb::GetProcess(DWORD dwProcessId, ICorDebugProcess **ppProcess)
{
#ifndef RIGHT_SIDE_ONLY
#ifdef PROFILING_SUPPORTED
    // Need to check that this thread is in a valid state for in-process debugging.
    if (!CHECK_INPROC_PROCESS_STATE())
        return (CORPROF_E_INPROC_NOT_ENABLED);
#endif // PROFILING_SUPPORTED
#endif // RIGHT_SIDE_ONLY

    if (!m_initialized)
        return E_FAIL;

    VALIDATE_POINTER_TO_OBJECT(ppProcess, ICorDebugProcess**);
            
    CordbProcess *p = (CordbProcess *) m_processes.GetBase(dwProcessId);

    if (p == NULL)
        return E_INVALIDARG;

    *ppProcess = (ICorDebugProcess*)p;
    (*ppProcess)->AddRef();

    return S_OK;
}

HRESULT Cordb::EnumerateProcesses(ICorDebugProcessEnum **ppProcesses)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    if (!m_initialized)
        return E_FAIL;

    VALIDATE_POINTER_TO_OBJECT(ppProcesses, ICorDebugProcessEnum **);
            
    CordbHashTableEnum *e = new CordbHashTableEnum(&m_processes,
                                                   IID_ICorDebugProcessEnum);
    if (e == NULL)
        return E_OUTOFMEMORY;

    *ppProcesses = (ICorDebugProcessEnum*)e;
    e->AddRef();

    return S_OK;
#endif //RIGHT_SIDE_ONLY        
}



HRESULT Cordb::CanLaunchOrAttach(DWORD dwProcessId, BOOL win32DebuggingEnabled)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    if (!m_initialized)
        return E_FAIL;

    if (!AllowAnotherProcess())
        return CORDBG_E_TOO_MANY_PROCESSES;

    
    return S_OK;
#endif //RIGHT_SIDE_ONLY        
}

DWORD MetadataPointerCache::dwInsert(DWORD dwProcessId, PVOID pRemoteMetadataPtr, PBYTE pLocalMetadataPtr, DWORD dwMetadataSize)
{
    MetadataCache * pMetadataCache = new MetadataCache;

    if (!pMetadataCache)
    {
        return E_OUTOFMEMORY;
    }

    memset(pMetadataCache, 0, sizeof(MetadataCache));

    pMetadataCache->pRemoteMetadataPtr = pRemoteMetadataPtr;
    pMetadataCache->pLocalMetadataPtr = pLocalMetadataPtr;
    pMetadataCache->dwProcessId = dwProcessId;
    pMetadataCache->dwRefCount = 1;
    pMetadataCache->dwMetadataSize = dwMetadataSize;
    pMetadataCache->pNext = m_pHead;
    
    m_pHead = pMetadataCache;
    return S_OK;
}

BOOL MetadataPointerCache::bFindMetadataCache(DWORD dwProcessId, PVOID pKey, MetadataCache *** pppNext, BOOL bRemotePtr)
{
    MetadataCache ** ppNext = &m_pHead;
    MetadataCache * pMetadataCache;

    while(*ppNext != NULL)
    {
        pMetadataCache = *ppNext;
        
        _ASSERTE(pMetadataCache);
        _ASSERTE(pMetadataCache->pRemoteMetadataPtr);
        _ASSERTE(pMetadataCache->pLocalMetadataPtr);
        _ASSERTE(pMetadataCache->dwRefCount);
        
        if (bRemotePtr ? 
            pMetadataCache->pRemoteMetadataPtr == pKey : 
            pMetadataCache->pLocalMetadataPtr == pKey)
        {
            if (dwProcessId == pMetadataCache->dwProcessId)
            {
                *pppNext = ppNext;
                return true;
            }
        }

        ppNext = &pMetadataCache->pNext;
    }

    *pppNext = NULL;
    
    return false;
}

void MetadataPointerCache::vRemoveNode(MetadataCache **ppNext)
{
    MetadataCache * pMetadataCache = *ppNext;

    _ASSERTE(pMetadataCache);
    *ppNext = pMetadataCache->pNext;

    _ASSERTE(pMetadataCache->pLocalMetadataPtr);
    delete pMetadataCache->pLocalMetadataPtr;

    delete pMetadataCache;
}

void MetadataPointerCache::AddRef()
{
    InterlockedIncrement(&m_RefCount);
}

void MetadataPointerCache::Release()
{
    LONG Count = InterlockedDecrement(&m_RefCount);
    if (Count == 0) {
	Neuter();
    }
}
 
void MetadataPointerCache::Neuter()
{
    while(m_pHead != NULL)
    {
        vRemoveNode(&m_pHead);
    }
    _ASSERTE(m_pHead == NULL);
}

BOOL MetadataPointerCache::IsEmpty()
{
    return m_pHead== NULL;
}

DWORD MetadataPointerCache::CopyRemoteMetadata(
    HANDLE hProcess, PVOID pRemoteMetadataPtr, DWORD dwMetadataSize, PBYTE* ppLocalMetadataPtr)
{
    // Allocate space for the local copy of the metadata
    PBYTE pLocalMetadataPtr = new BYTE[dwMetadataSize];
    
    if (pLocalMetadataPtr == NULL)
    {
        *ppLocalMetadataPtr = NULL;
        return E_OUTOFMEMORY;
    }
    
    memset(pLocalMetadataPtr, 0, dwMetadataSize);

    // Copy the metadata from the left side
    BOOL succ;
    succ = ReadProcessMemoryI(hProcess,
                              pRemoteMetadataPtr,
                              pLocalMetadataPtr,
                              dwMetadataSize,
                              NULL);
                              
    if (!succ)
    {
        *ppLocalMetadataPtr = NULL;
        return HRESULT_FROM_WIN32(GetLastError());
    }

    *ppLocalMetadataPtr = pLocalMetadataPtr;
    return S_OK;
}

DWORD MetadataPointerCache::AddRefCachePointer(HANDLE hProcess, DWORD dwProcessId, 
                                                PVOID pRemoteMetadataPtr, DWORD dwMetadataSize, PBYTE* ppLocalMetadataPtr)
{
    _ASSERTE(pRemoteMetadataPtr && dwMetadataSize && ppLocalMetadataPtr);

    *ppLocalMetadataPtr = NULL;
     
    MetadataCache ** ppNext = NULL;   

    BOOL bHit = bFindMetadataCache(dwProcessId, pRemoteMetadataPtr, &ppNext, true);
    DWORD dwErr;
    
    if (bHit)
    {
        // Cache hit case:
        MetadataCache* pMetadataCache = *ppNext;
        
        _ASSERTE(pMetadataCache);
        _ASSERTE(pMetadataCache->dwMetadataSize == dwMetadataSize);
        _ASSERTE(pMetadataCache->dwProcessId == dwProcessId);
        _ASSERTE(pMetadataCache->pLocalMetadataPtr);
        
        pMetadataCache->dwRefCount++;

        *ppLocalMetadataPtr = pMetadataCache->pLocalMetadataPtr;
        return S_OK;
    }
    else
    {
        // Cache miss case:
        PBYTE pLocalMetadataPtr = NULL;

        dwErr = CopyRemoteMetadata(hProcess, pRemoteMetadataPtr, dwMetadataSize, &pLocalMetadataPtr);
        if (SUCCEEDED(dwErr))
        {
            dwErr = dwInsert(dwProcessId, pRemoteMetadataPtr, pLocalMetadataPtr, dwMetadataSize);
            if (SUCCEEDED(dwErr))
            {
                *ppLocalMetadataPtr = pLocalMetadataPtr;
                return S_OK;                    
            }
        }
    }

    _ASSERTE(!ppLocalMetadataPtr);

    return dwErr;
}

void MetadataPointerCache::ReleaseCachePointer(DWORD dwProcessId, PBYTE pLocalMetadataPtr, PVOID pRemotePtr, DWORD dwMetadataSize)
{
    _ASSERTE(pLocalMetadataPtr);

    MetadataCache ** ppNext;
    MetadataCache * pMetadataCache;
#ifdef _DEBUG
    BOOL bCacheHit =
#endif
    bFindMetadataCache(dwProcessId, pLocalMetadataPtr, &ppNext, false);

    // If Release is called then there should be an entry in the the cache to release.
    _ASSERTE(bCacheHit);
    _ASSERTE(ppNext);

    pMetadataCache = *ppNext;
    _ASSERT(pMetadataCache); // The pNext pointer shouldn't be NULL
    _ASSERT(pMetadataCache->pLocalMetadataPtr == pLocalMetadataPtr); // We used the local pointer to look up the entry
    _ASSERT(pMetadataCache->dwRefCount); // The refCount should be > 0 if we are releasing it
    _ASSERT(pMetadataCache->pRemoteMetadataPtr == pRemotePtr); // The remote metadata pointer should match
    _ASSERT(pMetadataCache->dwMetadataSize == dwMetadataSize); // The user should also know the size

    pMetadataCache->dwRefCount--;

    if(0 == pMetadataCache->dwRefCount)
    {
        // If the refcount hits zero remove the node and free the local copy of the metadata
        // and the node in the hashtable.
        vRemoveNode(ppNext);
    }
}

/* ------------------------------------------------------------------------- *
 * Process class
 * ------------------------------------------------------------------------- */
    
CordbProcess::CordbProcess(Cordb* cordb, DWORD processID, HANDLE handle)
  : CordbBase(processID, enumCordbProcess), m_cordb(cordb), m_handle(handle),
    m_attached(false), m_detached(false), m_uninitializedStop(false),
    m_creating(false),
    m_exiting(false),
    m_firstExceptionHandled(false),
    m_terminated(false),
    m_unrecoverableError(false), m_sendAttachIPCEvent(false),
    m_firstManagedEvent(false), m_specialDeferment(false),
    m_helperThreadDead(false),
    m_loaderBPReceived(false),
    m_stopCount(0),
    m_synchronized(false),
    m_syncCompleteReceived(false),
    m_oddSync(false),
    m_userThreads(11),
    m_unmanagedThreads(11),
    m_appDomains(11),
    m_steppers(11),
    m_continueCounter(1),
    m_EnCCounter(1), //set to 1 so that functions can start at zero
    m_DCB(NULL),
    m_leftSideEventAvailable(NULL),
    m_leftSideEventRead(NULL),
    m_rightSideEventAvailable(NULL),
    m_rightSideEventRead(NULL),
    m_leftSideUnmanagedWaitEvent(NULL),
    m_syncThreadIsLockFree(NULL),
    m_SetupSyncEvent(NULL),
    m_debuggerAttachedEvent(NULL),
    m_initialized(false),
    m_queuedEventList(NULL),
    m_lastQueuedEvent(NULL),
    m_dispatchingEvent(false),
    m_stopRequested(false),
    m_stopWaitEvent(NULL),
    m_miscWaitEvent(NULL),
#ifdef _DEBUG
    m_processMutexOwner(0),
    m_processMutexRecursionCount(0),
#endif
    m_lastIBStoppingEvent(NULL),
    m_dispatchingUnmanagedEvent(false),
    m_dispatchingOOBEvent(false),
    m_doRealContinueAfterOOBBlock(false),
    m_deferContinueDueToOwnershipWait(false),
    m_helperThreadId(0),
    m_state(0),
    m_awaitingOwnershipAnswer(0),
    m_pPatchTable(NULL),
    m_rgData(NULL),
    m_rgNextPatch(NULL),
    m_cPatch(0),
    m_rgUncommitedOpcode(NULL),
    m_minPatchAddr(MAX_ADDRESS),
    m_maxPatchAddr(MIN_ADDRESS),
    m_iFirstPatch(0),
    // EnC stuff
    m_pbRemoteBuf(NULL),
    m_cbRemoteBuf(0),
    m_pSnapshotInfos(NULL)
{
#ifndef RIGHT_SIDE_ONLY
    m_appDomains.m_guid = IID_ICorDebugAppDomainEnum;
    m_appDomains.m_creator.lsAppD.m_proc = this;

    m_userThreads.m_guid = IID_ICorDebugThreadEnum;
    m_userThreads.m_creator.lsThread.m_proc = this;
#endif //RIGHT_SIDE_ONLY
}

/*
    A list of which resources owned by this object are accounted for.

    UNKNOWN
        Cordb*                      m_cordb;  
        CordbHashTable              m_unmanagedThreads; // Released in CordbProcess but not removed from hash
        DebuggerIPCControlBlock     *m_DCB;
        DebuggerIPCEvent*           m_lastQueuedEvent; 
        
        // CordbUnmannagedEvent is a struct which is not derrived from CordbBase.
        // It contains a CordbUnmannagedThread which may need to be released.
        CordbUnmanagedEvent         *m_unmanagedEventQueue;
        CordbUnmanagedEvent         *m_lastQueuedUnmanagedEvent;
        CordbUnmanagedEvent         *m_lastIBStoppingEvent;
        CordbUnmanagedEvent         *m_outOfBandEventQueue;
        CordbUnmanagedEvent         *m_lastQueuedOOBEvent;

        BYTE*                       m_pPatchTable;
        BYTE                        *m_rgData;
        void                        *m_pbRemoteBuf;
        UnorderedSnapshotInfoArray  *m_pSnapshotInfos;
        
   RESOLVED
        // Nutered
        CordbHashTable        m_userThreads;
        CordbHashTable        m_appDomains;        

        // Cleaned up in ExitProcess
        HANDLE                m_SetupSyncEvent; 
        DebuggerIPCEvent*     m_queuedEventList; 
        
        CordbHashTable        m_steppers; // Closed in ~CordbProcess

        // Closed in CloseIPCEventHandles called from ~CordbProcess
        HANDLE                m_leftSideEventAvailable;         
        HANDLE                m_leftSideEventRead; 

        // Closed in ~CordbProcess
        HANDLE                m_handle; 
        HANDLE                m_rightSideEventAvailable;
        HANDLE                m_rightSideEventRead;
        HANDLE                m_leftSideUnmanagedWaitEvent;
        HANDLE                m_syncThreadIsLockFree;
        HANDLE                m_stopWaitEvent;
        HANDLE                m_miscWaitEvent;
        HANDLE                m_debuggerAttachedEvent;

        // Deleted in ~CordbProcess
        CRITICAL_SECTION      m_processMutex;
        CRITICAL_SECTION      m_sendMutex;
*/

CordbProcess::~CordbProcess()
{
    CordbBase* entry;
    HASHFIND find;
    
#ifdef _DEBUG
    _ASSERTE(!m_cordb->m_processes.GetBase(m_id));
#endif
    
    LOG((LF_CORDB, LL_INFO1000, "[%x]CP::~CP: deleting process 0x%08x\n", 
         GetCurrentThreadId(), this));

#ifdef RIGHT_SIDE_ONLY
	CordbProcess::CloseIPCHandles();
    if (m_rightSideEventAvailable != NULL)
    {
    	CloseHandle(m_rightSideEventAvailable);
        m_rightSideEventAvailable = NULL;
   	}

    if (m_rightSideEventRead != NULL)
    {
    	CloseHandle(m_rightSideEventRead);
        m_rightSideEventRead = NULL;
    }

    if (m_leftSideUnmanagedWaitEvent != NULL)
    {
    	CloseHandle(m_leftSideUnmanagedWaitEvent);
        m_leftSideUnmanagedWaitEvent = NULL;
   	}

    if (m_syncThreadIsLockFree != NULL)
    {
    	CloseHandle(m_syncThreadIsLockFree);
        m_syncThreadIsLockFree = NULL;
   	}

    if (m_stopWaitEvent != NULL)
    {
    	CloseHandle(m_stopWaitEvent);
        m_stopWaitEvent = NULL;
   	}

    if (m_miscWaitEvent != NULL)
    {
    	CloseHandle(m_miscWaitEvent);
        m_miscWaitEvent = NULL;
   	}

    if (m_debuggerAttachedEvent != NULL)
    {
    	CloseHandle(m_debuggerAttachedEvent);
        m_debuggerAttachedEvent = NULL;
   	}

    //
    // Disconnect any active steppers
    //
    for (entry =  m_steppers.FindFirst(&find);
         entry != NULL;
         entry =  m_steppers.FindNext(&find))
    {
        CordbStepper *stepper = (CordbStepper*) entry;
        stepper->Disconnect();
    }

#endif //RIGHT_SIDE_ONLY

    ClearPatchTable();
    
    DeleteCriticalSection(&m_processMutex);
    DeleteCriticalSection(&m_sendMutex);

    if (m_handle != NULL)
        CloseHandle(m_handle);

    if (m_pSnapshotInfos)
    {
        delete m_pSnapshotInfos;
        m_pSnapshotInfos = NULL;
    }

    // Delete any left over unmanaged thread objects. There are a
    // number of cases where the OS doesn't send us all of the proper
    // exit thread events.
    for (entry =  m_unmanagedThreads.FindFirst(&find);
         entry != NULL;
         entry =  m_unmanagedThreads.FindNext(&find))
    {
        CordbUnmanagedThread* ut = (CordbUnmanagedThread*) entry;
        ut->Release();
    }
}

// Neutered when process dies
void CordbProcess::Neuter()
{
    AddRef();
    {        
        NeuterAndClearHashtable(&m_userThreads);
        NeuterAndClearHashtable(&m_appDomains);

        CordbBase::Neuter();
    }        
    Release();
}

void CordbProcess::CloseIPCHandles(void)
{
	// Close off Right Side's IPC handles.

    if (m_leftSideEventAvailable != NULL)
    {
        CloseHandle(m_leftSideEventAvailable);
        m_leftSideEventAvailable = NULL;
	}
	
    if (m_leftSideEventRead != NULL)
	{
		CloseHandle(m_leftSideEventRead);
        m_leftSideEventRead = NULL;
	}
}

//
// Init -- create any objects that the process object needs to operate.
// Currently, this is just a few events.
//
HRESULT CordbProcess::Init(bool win32Attached)
{
    HRESULT hr = S_OK;
#ifdef RIGHT_SIDE_ONLY
    BOOL succ = TRUE;
    WCHAR tmpName[256];
#endif //RIGHT_SIDE_ONLY    

    _ASSERTE(!win32Attached);
    
    IPCWriterInterface *pIPCManagerInterface = new IPCWriterInterface();

    if (pIPCManagerInterface == NULL)
        return (E_OUTOFMEMORY);

    hr = pIPCManagerInterface->Init();

    if (FAILED(hr))
        return (hr);
        
    InitializeCriticalSection(&m_processMutex);
    InitializeCriticalSection(&m_sendMutex);

    // Grab the security attributes that we'll use to create kernel objects for the target process.
    SECURITY_ATTRIBUTES *pSA = NULL;

    hr = pIPCManagerInterface->GetSecurityAttributes(m_id, &pSA);

    if (FAILED(hr))
        return hr;
    
    //
    // Setup events needed by the left side to send the right side an event.
    //
#ifdef RIGHT_SIDE_ONLY
        swprintf(tmpName, CorDBIPCLSEventAvailName, m_id);

    m_leftSideEventAvailable = WszCreateEvent(pSA, FALSE, FALSE, tmpName);

    if (m_leftSideEventAvailable == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());

        goto exit;
    }

    // IMPORTANT: The existence of this event determines whether or not
    // another debugger has attached to this process.  I assume that this
    // event is destroyed on a detach, so this shouldn't screw up a
    // reattach.  I'm picking this because it's early enough in the init
    // to make backing out easy.
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        hr = CORDBG_E_DEBUGGER_ALREADY_ATTACHED;

        goto exit;
    }
    
        swprintf(tmpName, CorDBIPCLSEventReadName, m_id);

    m_leftSideEventRead = WszCreateEvent(pSA, FALSE, FALSE, tmpName);
    
    if (m_leftSideEventRead == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }

    _ASSERTE(GetLastError() != ERROR_ALREADY_EXISTS);
    
        swprintf(tmpName, CorDBDebuggerAttachedEvent, m_id);

    m_debuggerAttachedEvent = WszCreateEvent(pSA, TRUE, FALSE, tmpName);

    
    NAME_EVENT_BUFFER;
    m_stopWaitEvent = WszCreateEvent(NULL, TRUE, FALSE, NAME_EVENT(L"StopWaitEvent"));
    if (m_stopWaitEvent == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }

    m_miscWaitEvent = WszCreateEvent(NULL, FALSE, FALSE, NAME_EVENT(L"MiscWaitEvent"));
    if (m_miscWaitEvent == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }

    //
    // Duplicate our own copy of the process's handle because the handle
    // we've got right now is also passed back to the caller of CreateProcess
    // in the process info structure, and they're supposed to be able to
    // close that handle whenever they want to. 
    
    HANDLE tmpHandle;
    
    succ = DuplicateHandle(GetCurrentProcess(),
                           m_handle,
                           GetCurrentProcess(),
                           &tmpHandle,
                           NULL,
                           FALSE,
                           DUPLICATE_SAME_ACCESS);

    if (!succ)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }

    m_handle = tmpHandle;

        swprintf(tmpName, CorDBIPCSetupSyncEventName, m_id);
    
    LOG((LF_CORDB, LL_INFO10000,
         "CP::I: creating setup sync event with name [%S]\n", tmpName));

    m_SetupSyncEvent = WszCreateEvent(pSA, TRUE, FALSE, tmpName);

    if (m_SetupSyncEvent == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        // If the event already exists, then the Left Side has already
        // setup the shared memory.
        LOG((LF_CORDB, LL_INFO10000, "CP::I: setup sync event already exists.\n"));

        // Wait for the Setup Sync event before continuing. This
        // ensures that the Left Side is finished setting up the
        // control block.
        DWORD ret = WaitForSingleObject(m_SetupSyncEvent, INFINITE);

        if (ret != WAIT_OBJECT_0)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto exit;
        }

        // We no longer need this event now.
        CloseHandle(m_SetupSyncEvent);
        m_SetupSyncEvent = NULL;

        hr = m_IPCReader.OpenPrivateBlockOnPid(m_id);
        if (!SUCCEEDED(hr))
        {            
            goto exit;
        }

        m_DCB = m_IPCReader.GetDebugBlock();

#else
        _ASSERTE( g_pRCThread != NULL ); // the Debugger part of the EE had
                                         // better be initialized already
        m_DCB = g_pRCThread->GetInprocControlBlock();

        // The same-process structures should not exist
        _ASSERTE(m_DCB->m_leftSideEventAvailable == NULL);
        _ASSERTE(m_DCB->m_leftSideEventRead == NULL);
        _ASSERTE(m_rightSideEventRead == NULL);
        _ASSERTE(m_rightSideEventAvailable == NULL);
        _ASSERTE(m_DCB->m_leftSideUnmanagedWaitEvent == NULL);
        _ASSERTE(m_DCB->m_syncThreadIsLockFree == NULL);
        
#endif //RIGHT_SIDE_ONLY    
        
        if (m_DCB == NULL)
        {
            hr = ERROR_FILE_NOT_FOUND;
            goto exit;
        }

#ifdef RIGHT_SIDE_ONLY
        // Verify that the control block is valid.
        hr = VerifyControlBlock();

        if (FAILED(hr))
            goto exit;


	HANDLE hTemp;

        // Dup LSEA and LSER into the remote process.
        succ = DuplicateHandle(GetCurrentProcess(),
                               m_leftSideEventAvailable,
                               m_handle,
                               &(m_DCB->m_leftSideEventAvailable),
                               NULL, FALSE, DUPLICATE_SAME_ACCESS);
        if (!succ)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto exit;
        }

        succ = DuplicateHandle(GetCurrentProcess(),
                               m_leftSideEventRead,
                               m_handle,
                               &(m_DCB->m_leftSideEventRead),
                               NULL, FALSE, DUPLICATE_SAME_ACCESS);

        if (!succ)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto exit;
        }

        // Dup our own process handle into the remote process.
        succ = DuplicateHandle(GetCurrentProcess(),
                               GetCurrentProcess(),
                               m_handle,
                               &(m_DCB->m_rightSideProcessHandle),
                               NULL, FALSE, DUPLICATE_SAME_ACCESS);

        if (!succ)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto exit;
        }

        // Dup RSEA and RSER into this process.
        hTemp = PAL_RemoteHandleToLocal(m_DCB->m_remoteRightSideEventAvailable);
        if (hTemp == INVALID_HANDLE_VALUE) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto exit;
	}
        succ = DuplicateHandle(m_handle,
                               hTemp,
                               GetCurrentProcess(),
                               &m_rightSideEventAvailable,
                               NULL, FALSE, DUPLICATE_SAME_ACCESS);
        if (!succ)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto exit;
        }

	hTemp = PAL_RemoteHandleToLocal(m_DCB->m_remoteRightSideEventRead);
        if (hTemp == INVALID_HANDLE_VALUE) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto exit;
	}
        succ = DuplicateHandle(m_handle,
                               hTemp,
                               GetCurrentProcess(),
                               &m_rightSideEventRead,
                               NULL, FALSE, DUPLICATE_SAME_ACCESS);
        if (!succ)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto exit;
        }

	hTemp = PAL_RemoteHandleToLocal(m_DCB->m_remoteLeftSideUnmanagedWaitEvent);
        if (hTemp == INVALID_HANDLE_VALUE) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto exit;
	}
        succ = DuplicateHandle(m_handle,
                               hTemp,
                               GetCurrentProcess(),
                               &m_leftSideUnmanagedWaitEvent,
                               NULL, FALSE, DUPLICATE_SAME_ACCESS);
        if (!succ)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto exit;
        }
        
        hTemp = PAL_RemoteHandleToLocal(m_DCB->m_remoteSyncThreadIsLockFree);
        if (hTemp == INVALID_HANDLE_VALUE) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto exit;
	}
        succ = DuplicateHandle(m_handle,
                               hTemp,
                               GetCurrentProcess(),
                               &m_syncThreadIsLockFree,
                               NULL, FALSE, DUPLICATE_SAME_ACCESS);
        if (!succ)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto exit;
        }

	m_DCB->m_remoteLeftSideEventAvailable = PAL_LocalHandleToRemote(m_DCB->m_leftSideEventAvailable);
        m_DCB->m_remoteLeftSideEventRead = PAL_LocalHandleToRemote(m_DCB->m_leftSideEventRead);
        m_DCB->m_remoteRightSideProcessHandle = PAL_LocalHandleToRemote(m_DCB->m_rightSideProcessHandle);
	if (!m_DCB->m_remoteLeftSideEventAvailable ||
	    !m_DCB->m_remoteLeftSideEventRead ||
	    !m_DCB->m_remoteRightSideProcessHandle) 
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto exit;
	}


#else  //!RIGHT_SIDE_ONLY

	//
	// Unmarshal the left-side remote HANDLEs
	//
    HANDLE hLocal;

	hLocal = PAL_RemoteHandleToLocal(m_DCB->m_remoteLeftSideEventAvailable);
	if (hLocal == INVALID_HANDLE_VALUE) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto exit;
	}
    m_DCB->m_leftSideEventAvailable = hLocal;

	hLocal = PAL_RemoteHandleToLocal(m_DCB->m_remoteLeftSideEventRead);
	if (hLocal == INVALID_HANDLE_VALUE) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
	}
    m_DCB->m_leftSideEventRead = hLocal;

	hLocal = PAL_RemoteHandleToLocal(m_DCB->m_remoteLeftSideUnmanagedWaitEvent);
	if (hLocal == INVALID_HANDLE_VALUE) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
	}
	m_DCB->m_leftSideUnmanagedWaitEvent = hLocal;

        
#endif //!RIGHT_SIDE_ONLY        

        m_sendAttachIPCEvent = true;

        m_DCB->m_rightSideIsWin32Debugger = win32Attached;

#ifndef RIGHT_SIDE_ONLY
        _ASSERTE( !win32Attached );
#endif //RIGHT_SIDE_ONLY
#ifdef RIGHT_SIDE_ONLY
        
        // At this point, the control block is complete and all four
        // events are available and valid for the remote process.
    }
    else
    {
        // If the event was created by us, then we need to signal
        // its state. The fields in the shared mem which need to be
        // filled out by us will be done upon receipt of the first
        // event from the LHS
        LOG((LF_CORDB, LL_INFO10000, "DRCT::I: setup sync event was created.\n"));

        // Set the Setup Sync event 
        SetEvent(m_SetupSyncEvent);
    }

#endif //RIGHT_SIDE_ONLY    

    m_pSnapshotInfos = new UnorderedSnapshotInfoArray();
    if (NULL == m_pSnapshotInfos)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

exit:
    if (pSA != NULL)
        pIPCManagerInterface->DestroySecurityAttributes(pSA);
    
    if (pIPCManagerInterface != NULL)
    {
        pIPCManagerInterface->Terminate();
        delete pIPCManagerInterface;
    }

    if (FAILED(hr))
    {
        if (m_leftSideEventAvailable)
        {
            CloseHandle(m_leftSideEventAvailable);
            m_leftSideEventAvailable = NULL;
        }
    }

    return hr;
}



// Spliced here so we don't have include EnC.cpp in the left side build process

COM_METHOD CordbProcess::CanCommitChanges(ULONG cSnapshots, 
                ICorDebugEditAndContinueSnapshot *pSnapshots[], 
                ICorDebugErrorInfoEnum **pError)
{
    return CORDBG_E_INPROC_NOT_IMPL;;
}

COM_METHOD CordbProcess::CommitChanges(ULONG cSnapshots, 
    ICorDebugEditAndContinueSnapshot *pSnapshots[], 
    ICorDebugErrorInfoEnum **pError)
{
    return CORDBG_E_INPROC_NOT_IMPL;;
}



//
// Terminating -- places the process into the terminated state. This should
// also get any blocking process functions unblocked so they'll return
// a failure code. 
//
void CordbProcess::Terminating(BOOL fDetach)
{
    LOG((LF_CORDB, LL_INFO1000,"CP::T: Terminating process 0x%x detach=%d\n", m_id, fDetach));
    m_terminated = true;

    m_cordb->ProcessStateChanged();

    SetEvent(m_leftSideEventRead);
    SetEvent(m_rightSideEventRead);
    SetEvent(m_leftSideEventAvailable);
    SetEvent(m_stopWaitEvent);

    if (fDetach)
    {
        // This averts a race condition wherein we'll detach, then reattach,
        // and find these events in the still-signalled state.
        ResetEvent(m_rightSideEventAvailable);
        ResetEvent(m_rightSideEventRead);
    }
}


//
// HandleManagedCreateThread processes a managed create thread debug event.
//
void CordbProcess::HandleManagedCreateThread(DWORD dwThreadId,
                                             HANDLE hThread)
{
    LOG((LF_CORDB, LL_INFO10000, "[%x] CP::HMCT: Create Thread %#x\n",
         GetCurrentThreadId(),
         dwThreadId));

    Lock();
    
    CordbThread* t = new CordbThread(this, dwThreadId, hThread);

    if (t != NULL)
    {
        HRESULT hr = m_userThreads.AddBase(t);

        if (FAILED(hr))
        {
            delete t;

            LOG((LF_CORDB, LL_INFO10000,
                 "Failed adding thread to process!\n"));
            CORDBSetUnrecoverableError(this, hr, 0);
        }
    }
    else
    {
        LOG((LF_CORDB, LL_INFO10000, "New CordbThread failed!\n"));
        CORDBSetUnrecoverableError(this, E_OUTOFMEMORY, 0);
    }

    Unlock();
}


HRESULT CordbProcess::QueryInterface(REFIID id, void **pInterface)
{
    if (id == IID_ICorDebugProcess)
        *pInterface = (ICorDebugProcess*)this;
    else if (id == IID_ICorDebugController)
        *pInterface = (ICorDebugController*)(ICorDebugProcess*)this;
    else if (id == IID_IUnknown)
        *pInterface = (IUnknown*)(ICorDebugProcess*)this;
    else
    {
        *pInterface = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

HRESULT CordbProcess::Detach()
{
    LOG((LF_CORDB, LL_INFO1000, "CP::Detach - beginning\n"));
#ifndef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOK(this);
    
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    // A very important note: we require that the process is synchronized before doing a detach. This ensures
    // that no events are on their way from the Left Side. We also require that the user has drained the
    // managed event queue, but there is currently no way to really enforce that here.
    CORDBRequireProcessStateOKAndSync(this, NULL);

    HASHFIND hf;
    HRESULT hr = S_OK;

    // Detach from each AD before detaching from the entire process.
    CordbAppDomain *cad = (CordbAppDomain *)m_appDomains.FindFirst(&hf);

    while (cad != NULL)
    {
        hr = cad->Detach();

        if (FAILED(hr))
            return hr;
            
        cad = (CordbAppDomain *)m_appDomains.FindNext(&hf);
    }

    if (m_SetupSyncEvent != NULL)
    {
        CloseHandle(m_SetupSyncEvent);
        m_SetupSyncEvent = NULL;
    }

    // Go ahead and detach from the entire process now.
    DebuggerIPCEvent *event = (DebuggerIPCEvent*) _alloca(CorDBIPC_BUFFER_SIZE);
    InitIPCEvent(event, DB_IPCE_DETACH_FROM_PROCESS, true, (void *)m_id);

    hr = m_cordb->SendIPCEvent(this, event, CorDBIPC_BUFFER_SIZE);

	if (!FAILED(hr))
	{
		m_cordb->m_win32EventThread->SendDetachProcessEvent(this);
		CloseIPCHandles();

        // Since we're auto-continuing when we detach, we should set the stop count back to zero. This
        // prevents anyone from calling Continue on this process after this call returns.
		m_stopCount = 0;

        // Remember that we've detached from this process object. This will prevent any further operations on
        // this process, just in case... :)
        m_detached = true;
    }

    LOG((LF_CORDB, LL_INFO1000, "CP::Detach - returning w/ hr=0x%x\n", hr));
    return hr;
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbProcess::Terminate(unsigned int exitCode)
{
    LOG((LF_CORDB, LL_INFO1000, "CP::Terminate: with exitcode %u\n", exitCode));
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    // You must be stopped and sync'd before calling Terminate.
    CORDBRequireProcessStateOKAndSync(this, NULL);

    // When we terminate the process, it's handle will become signaled and
    // Win32 Event Thread will leap into action and call CordbWin32EventThread::ExitProcess
    // Unfortunately, that may destroy this object if the ExitProcess callback
    // decides to call Release() on the process.

    // Indicate that the process is exiting so that (among other things) we don't try and
    // send messages to the left side while it's being nuked.
    Lock();

    m_exiting = true;

    // Free all the remaining events
    DebuggerIPCEvent *pCur = m_queuedEventList;
    while (pCur != NULL)
    {
        LOG((LF_CORDB, LL_INFO1000, "CP::Terminate: Deleting queued event: '%s'\n", IPCENames::GetName(pCur->type)));
                    
        DebuggerIPCEvent *pDel = pCur;
        pCur = pCur->next;
        free(pDel);
    }
    m_queuedEventList = NULL;

    Unlock();
    
    
    // We'd like to just take a lock around everything here, but that may deadlock us
    // since W32ET will wait on the lock, and Continue may wait on W32ET.
    // So we just do an extra AddRef/Release to make sure we're still around

    AddRef();
    
    // Right now, we simply pass through to the Win32 terminate...
    TerminateProcess(m_handle, exitCode);
       
    // Get the process to continue automatically.
    Continue(FALSE);

    // After this release, this object may be destroyed. So don't use any member functions
    // (including Locks) after here.
    Release();

    return S_OK;
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbProcess::GetID(DWORD *pdwProcessId)
{
    VALIDATE_POINTER_TO_OBJECT(pdwProcessId, DWORD *);

    *pdwProcessId = m_id;

    return S_OK;
}

HRESULT CordbProcess::GetHandle(HANDLE *phProcessHandle)
{
    VALIDATE_POINTER_TO_OBJECT(phProcessHandle, HANDLE *);
    *phProcessHandle = m_handle;

    return S_OK;
}

HRESULT CordbProcess::IsRunning(BOOL *pbRunning)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    VALIDATE_POINTER_TO_OBJECT(pbRunning, BOOL*);

    *pbRunning = !GetSynchronized();

    return S_OK;
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbProcess::EnableSynchronization(BOOL bEnableSynchronization)
{
    /* !!! */

    return E_NOTIMPL;
}

HRESULT CordbProcess::Stop(DWORD dwTimeout)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#endif
    return StopInternal(dwTimeout, NULL);
}

HRESULT CordbProcess::StopInternal(DWORD dwTimeout, void *pAppDomainToken)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    DebuggerIPCEvent* event;
    HRESULT hr = S_OK;

    LOG((LF_CORDB, LL_INFO1000, "CP::S: stopping process 0x%x(%d) with timeout %d\n", m_id, m_id,  dwTimeout));

    CORDBRequireProcessStateOK(this);
    
    Lock();
    
    // Don't need to stop if the process hasn't even executed any managed code yet.
    if (!m_initialized)
    {
        LOG((LF_CORDB, LL_INFO1000, "CP::S: process isn't initialized yet.\n"));

        // Mark the process as synchronized so no events will be dispatched until the thing is continued.
        SetSynchronized(true);

        // Remember uninitialized stop...
        m_uninitializedStop = true;


        // Get the RC Event Thread to stop listening to the process.
        m_cordb->ProcessStateChanged();
        
        hr = S_OK;
        goto Exit;
    }

    // Don't need to stop if the process is already synchronized.
    if (GetSynchronized())
    {
        LOG((LF_CORDB, LL_INFO1000, "CP::S: process was already synchronized.\n"));

        hr = S_OK;
        goto Exit;
    }

    LOG((LF_CORDB, LL_INFO1000, "CP::S: process not sync'd, requesting stop.\n"));

    m_stopRequested = true;
    Unlock();
    
    BOOL asyncBreakSent;
    
    CORDBSyncFromWin32StopIfNecessaryCheck(this, &asyncBreakSent);

    if (asyncBreakSent)
    {
        hr = S_OK;
        Lock();

        m_stopRequested = false;
        
        goto Exit;
    }

    // Send the async break event to the RC.
    event = (DebuggerIPCEvent*) _alloca(CorDBIPC_BUFFER_SIZE);
    InitIPCEvent(event, DB_IPCE_ASYNC_BREAK, false, pAppDomainToken);
    
    LOG((LF_CORDB, LL_INFO1000, "CP::S: sending async stop to appd 0x%x.\n", pAppDomainToken));

    hr = m_cordb->SendIPCEvent(this, event, CorDBIPC_BUFFER_SIZE);

    LOG((LF_CORDB, LL_INFO1000, "CP::S: sent async stop to appd 0x%x.\n", pAppDomainToken));

    // Wait for the sync complete message to come in. Note: when the sync complete message arrives to the RCEventThread,
    // it will mark the process as synchronized and _not_ dispatch any events. Instead, it will set m_stopWaitEvent
    // which will let this function return. If the user wants to process any queued events, they will need to call
    // Continue.
    LOG((LF_CORDB, LL_INFO1000, "CP::S: waiting for event.\n"));

    DWORD ret;
    ret = WaitForSingleObject(m_stopWaitEvent, dwTimeout);

    LOG((LF_CORDB, LL_INFO1000, "CP::S: got event, %d.\n", ret));

    if (m_terminated)
        return CORDBG_E_PROCESS_TERMINATED;
    
    if (ret == WAIT_OBJECT_0)
    {
        LOG((LF_CORDB, LL_INFO1000, "CP::S: process stopped.\n"));
        
        m_stopRequested = false;
        m_cordb->ProcessStateChanged();

        hr = S_OK;
        Lock();
        goto Exit;
    }
    else
        hr = HRESULT_FROM_WIN32(GetLastError());

    // We came out of the wait, but we weren't signaled because a sync complete event came in. Re-check the process and
    // remove the stop requested flag.
    Lock();
    m_stopRequested = false;

    if (GetSynchronized())
    {
        LOG((LF_CORDB, LL_INFO1000, "CP::S: process stopped.\n"));
        
        m_cordb->ProcessStateChanged();

        hr = S_OK;
    }

Exit:
    if (SUCCEEDED(hr))
        m_stopCount++;

    LOG((LF_CORDB, LL_INFO1000, "CP::S: returning from Stop, hr=0x%08x, m_stopCount=%d.\n", hr, m_stopCount));
    
    Unlock();
    
    return hr;
#endif //RIGHT_SIDE_ONLY    
}


void CordbProcess::MarkAllThreadsDirty(void)
{
    CordbBase* entry;
    HASHFIND find;

    Lock();
    
    for (entry =  m_userThreads.FindFirst(&find);
         entry != NULL;
         entry =  m_userThreads.FindNext(&find))
    {
        CordbThread* t = (CordbThread*) entry;
        _ASSERTE(t != NULL);

        t->MarkStackFramesDirty();
    }

    ClearPatchTable();

    Unlock();
}

HRESULT CordbProcess::Continue(BOOL fIsOutOfBand)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#endif
    return ContinueInternal(fIsOutOfBand, NULL);
}

HRESULT CordbProcess::ContinueInternal(BOOL fIsOutOfBand, void *pAppDomainToken)
{
    DebuggerIPCEvent* event;
    HRESULT hr = S_OK;

    if (m_unrecoverableError)
        return CORDBHRFromProcessState(this, NULL);

    Lock();

    LOG((LF_CORDB, LL_INFO1000, "CP::CI: continuing, fIsOutOfBand=%d\n", fIsOutOfBand));

    
    _ASSERTE(fIsOutOfBand == FALSE);

    // If we've got multiple Stop calls, we need a Continue for each one. So, if the stop count > 1, just go ahead and
    // return without doing anything. Note: this is only for in-band or managed events. OOB events are still handled as
    // normal above.
    _ASSERTE(m_stopCount > 0);

    if (m_stopCount == 0)
        return CORDBG_E_SUPERFLOUS_CONTINUE;
    
    m_stopCount--;

    // We give managed events priority over unmanaged events. That way, the entire queued managed state can drain before
    // we let any other unmanaged events through.

    // If we're processing a CreateProcess managed event, then simply mark that we're not syncronized anymore and
    // return. This is because the CreateProcess event isn't a real managed event.
    if (m_creating)
    {
        LOG((LF_CORDB, LL_INFO1000, "CP::CI: continuing from CreateProcess event.\n"));
        
        MarkAllThreadsDirty();
        
        m_creating = false;
        
        Unlock();
        return S_OK;
    }

    // Every stop or event must be matched by a corresponding Continue. m_stopCount counts outstanding stopping events
    // along with calls to Stop. If the count is high at this point, we simply return. This ensures that even if someone
    // calls Stop just as they're receiving an event that they can call Continue for that Stop and for that event
    // without problems.
    if (m_stopCount > 0)
    {
        LOG((LF_CORDB, LL_INFO1000, "CP::CI: m_stopCount=%d, Continue just returning S_OK...\n", m_stopCount));

        Unlock();
        return S_OK;
    }

    // We're no longer stopped, so reset the m_stopWaitEvent.
    ResetEvent(m_stopWaitEvent);
    
    // If we're continuing from an uninitialized stop, then we don't need to do much at all. No event need be sent to
    // the Left Side (duh, it isn't even there yet.) We just need to get the RC Event Thread to start listening to the
    // process again, and resume any unmanaged threads if necessary.
    if (m_uninitializedStop)
    {
        LOG((LF_CORDB, LL_INFO1000,
             "CP::CI: continuing from uninitialized stop.\n"));

        // No longer synchronized (it was a half-assed sync in the
        // first place.)
        SetSynchronized(false);
        MarkAllThreadsDirty();

        // No longer in an uninitialized stop.
        m_uninitializedStop = false;

        // Notify the RC Event Thread.
        m_cordb->ProcessStateChanged();


        Unlock();
        return S_OK;
    }
    
    // If there are more managed events, get them dispatched now.
    if ((m_queuedEventList != NULL) && GetSynchronized())
    {
        LOG((LF_CORDB, LL_INFO1000, "CP::CI: managed event queued.\n"));
        
        // Mark that we're not synchronized anymore.
        SetSynchronized(false);
        MarkAllThreadsDirty();

        // If we're in the middle of dispatching a managed event, then simply return. This indicates to HandleRCEvent
        // that the user called Continue and HandleRCEvent will dispatch the next queued event. But if Continue was
        // called outside the managed callback, all we have to do is tell the RC event thread that something about the
        // process has changed and it will dispatch the next managed event.
        if (!m_dispatchingEvent)
        {
            LOG((LF_CORDB, LL_INFO1000,
                 "CP::CI: continuing while not dispatching managed event.\n"));

            m_cordb->ProcessStateChanged();
        }

        Unlock();
        return S_OK;
    }
    
    // At this point, if the managed event queue is empty, m_synchronized may still be true if we had previously
    // synchronized.


    // Both the managed and unmanaged event queues are now empty. Go
    // ahead and continue the process for real.
    LOG((LF_CORDB, LL_INFO1000, "CP::CI: headed for true continue.\n"));

    // We need to check these while under the lock, but action must be
    // taked outside of the lock.
    bool isExiting = m_exiting;
    bool wasSynchronized = GetSynchronized();

    // Mark that we're no longer synchronized.
    if (wasSynchronized)
    {
        LOG((LF_CORDB, LL_INFO1000, "CP::CI: process was synchronized.\n"));
        
        SetSynchronized(false);
        m_syncCompleteReceived = false;
        MarkAllThreadsDirty();
    
        // Tell the RC event thread that something about this process has changed.
        m_cordb->ProcessStateChanged();
    }

    m_continueCounter++;

    // If m_oddSync is set, then out last synchronization was due to us syncing the process because we were Win32
    // stopped. Therefore, while we do need to do most of the work to continue the process below, we don't actually have
    // to send the managed continue event. Setting wasSynchronized to false here helps us do that.
    if (m_oddSync)
    {
        wasSynchronized = false;
        m_oddSync = false;
    }
    
    
    Unlock();


    // If we're processing an ExitProcess managed event, then we don't want to really continue the process, so just fall
    // thru.  Note: we did let the unmanaged continue go through above for this case.
    if (isExiting)
    {
        LOG((LF_CORDB, LL_INFO1000, "CP::CI: continuing from exit case.\n"));
    }
    else if (wasSynchronized)
    {
        LOG((LF_CORDB, LL_INFO1000, "CP::CI: Sending continue to AppD:0x%x.\n", pAppDomainToken));
    
        // Send to the RC to continue the process.
        event = (DebuggerIPCEvent*) _alloca(CorDBIPC_BUFFER_SIZE);
        InitIPCEvent(event, DB_IPCE_CONTINUE, false, pAppDomainToken);

        hr = m_cordb->SendIPCEvent(this, event, CorDBIPC_BUFFER_SIZE);
        
        LOG((LF_CORDB, LL_INFO1000, "CP::CI: Continue sent to AppD:0x%x.\n", pAppDomainToken));
    }


    LOG((LF_CORDB, LL_INFO1000, "CP::CI: continue done, returning.\n"));
    
    return hr;
}

HRESULT CordbProcess::HasQueuedCallbacks(ICorDebugThread *pThread,
                                         BOOL *pbQueued)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    VALIDATE_POINTER_TO_OBJECT_OR_NULL(pThread,ICorDebugThread *);
    VALIDATE_POINTER_TO_OBJECT(pbQueued,BOOL *);

    CORDBRequireProcessStateOKAndSync(this, NULL);

    Lock();

    if (pThread == NULL)
        *pbQueued = (m_queuedEventList != NULL);
    else
    {
        *pbQueued = FALSE;

        for (DebuggerIPCEvent *event = m_queuedEventList;
             event != NULL;
             event = event->next)
        {
            CordbThread *t =
                (CordbThread*) m_userThreads.GetBase(event->threadId);

            if (t == (CordbThread*)pThread)
            {
                *pbQueued = TRUE;
                break;
            }
        }
    }

    Unlock();

    return S_OK;
#endif //RIGHT_SIDE_ONLY    
}

#ifdef RIGHT_SIDE_ONLY
//
// A small helper function to convert a CordbBreakpoint to an ICorDebugBreakpoint based on its type.
//
static ICorDebugBreakpoint *CordbBreakpointToInterface(CordbBreakpoint *bp)
{
    //
    // I really hate this. We've got three subclasses of CordbBreakpoint, but we store them all into the same hash
    // (m_breakpoints), so when we get one out of the hash, we don't really know what type it is. But we need to know
    // what type it is because we need to cast it to the proper interface before passing it out. I.e., when we create a
    // function breakpoint, we return the breakpoint casted to an ICorDebugFunctionBreakpoint. But if we grab that same
    // breakpoint out of the hash as a CordbBreakpoint and pass it out as an ICorDebugBreakpoint, then that's a
    // different pointer, and its wrong. So I've added the type to the breakpoint so we can cast properly here. I'd love
    // to do this a different way, though...
    //
    // -- Mon Dec 14 21:06:46 1998
    //
    switch(bp->GetBPType())
    {
    case CBT_FUNCTION:
        return ((ICorDebugFunctionBreakpoint*)(CordbFunctionBreakpoint*)bp);
        break;
                    
    case CBT_MODULE:
        return ((ICorDebugFunctionBreakpoint*)(CordbModuleBreakpoint*)bp);
        break;

    case CBT_VALUE:
        return ((ICorDebugFunctionBreakpoint*)(CordbValueBreakpoint*)bp);
        break;

    default:
        _ASSERTE(!"Invalid breakpoint type!");
    }

    return NULL;
}
#endif // RIGHT_SIDE_ONLY

//
// DispatchRCEvent -- dispatches a previously queued IPC event reveived
// from the runtime controller. This represents the last amount of processing
// the DI gets to do on an event before giving it to the user.
//
void CordbProcess::DispatchRCEvent(void)
{
    //
    // Note: the current thread should have the process locked when it
    // enters this method.
    //
    _ASSERTE(ThreadHoldsProcessLock());

    _ASSERTE(m_cordb->m_managedCallback != NULL);
    
    //
    // Snag the first event off the queue.
    //
    DebuggerIPCEvent* event = m_queuedEventList;

    if (event == NULL)
        return;

    m_queuedEventList = event->next;

    if (m_queuedEventList == NULL)
        m_lastQueuedEvent = NULL;

    // Bump up the stop count. Either we'll dispatch a managed event,
    // or the logic below will decide not to dispatch one and call
    // Continue itself. Either way, the stop count needs to go up by
    // one...
    m_stopCount++;
    
    //
    // Set m_dispatchingEvent to true to guard against calls to Continue()
    // from within the user's callback. We need Continue() to behave a little
    // bit differently in such a case.
    // 
    // Also note that Win32EventThread::ExitProcess will take the lock and free all 
    // events in the queue. (the current event is already off the queue, so 
    // it will be ok). But we can't do the EP callback in the middle of this dispatch
    // so if this flag is set, EP will wait on the miscWaitEvent (which will 
    // get set in FlushQueuedEvents when we return from here) and let us finish here.
    //        
    m_dispatchingEvent = true;

    // The thread may have moved the appdomain it occupies since the last time
    // we saw it, so update it.
    CordbAppDomain *pAppDomain = NULL;
    CordbThread* thread = NULL;
    
    thread = (CordbThread*)m_userThreads.GetBase(event->threadId);
    pAppDomain =(CordbAppDomain*) m_appDomains.GetBase(
            (ULONG)event->appDomainToken);

    // Update the app domain that this thread lives in.
    if (thread != NULL && pAppDomain != NULL)
    {
        thread->m_pAppDomain = pAppDomain;
    }

    Unlock();
    
    switch (event->type & DB_IPCE_TYPE_MASK)
    {
    case DB_IPCE_BREAKPOINT:
        {
#ifdef RIGHT_SIDE_ONLY
            LOG((LF_CORDB, LL_INFO1000, "[%x] RCET::DRCE: breakpoint.\n",
                 GetCurrentThreadId()));

            Lock();

            _ASSERTE(thread != NULL); 
            _ASSERTE (pAppDomain != NULL);

            // Find the breakpoint object on this side.
            CordbBreakpoint *bp = (CordbBreakpoint *) 
              thread->m_pAppDomain->m_breakpoints.GetBase((unsigned long) 
                                    event->BreakpointData.breakpointToken);

            ICorDebugBreakpoint *ibp = NULL;
            if (bp != NULL)
            {
                bp->AddRef();
                ibp = CordbBreakpointToInterface(bp);
                _ASSERTE(ibp != NULL);
            }
            
            Unlock();

            if (m_cordb->m_managedCallback && (bp != NULL))
            {
                m_cordb->m_managedCallback->Breakpoint((ICorDebugAppDomain*) thread->m_pAppDomain,
                                                       (ICorDebugThread*) thread,
                                                       ibp);
            }

            if (bp != NULL)
                bp->Release();
            else
            {
                // If we didn't find a breakpoint object on this side,
                // the we have an extra BP event for a breakpoint that
                // has been removed and released on this side. Just
                // ignore the event.
                Continue(FALSE);
            }
#else
        _ASSERTE( !"Inproc got a breakpoint alert, which it shouldn't have!" );
#endif        
        }
        break;

    case DB_IPCE_USER_BREAKPOINT:
        {
            LOG((LF_CORDB, LL_INFO1000, "[%x] RCET::DRCE: user breakpoint.\n",
                 GetCurrentThreadId()));

            Lock();

            _ASSERTE(thread != NULL);
            _ASSERTE (pAppDomain != NULL);

            Unlock();

            if (m_cordb->m_managedCallback)
            {
                _ASSERTE(thread->m_pAppDomain != NULL);

                m_cordb->m_managedCallback->Break((ICorDebugAppDomain*) thread->m_pAppDomain,
                                                  (ICorDebugThread*) thread);

            }
        }
        break;

    case DB_IPCE_STEP_COMPLETE:
        {
            LOG((LF_CORDB, LL_INFO1000, "[%x] RCET::DRCE: step complete.\n", 
                 GetCurrentThreadId()));

            Lock();
            _ASSERTE(thread != NULL);

            CordbStepper *stepper = (CordbStepper *) 
              thread->m_process->m_steppers.GetBase((unsigned long) 
                                    event->StepData.stepperToken);

            if (stepper != NULL)
            {
                stepper->AddRef();
                stepper->m_active = false;
                thread->m_process->m_steppers.RemoveBase(stepper->m_id);
            }

            Unlock();

            if (m_cordb->m_managedCallback)
            {
                _ASSERTE(thread->m_pAppDomain != NULL);

                m_cordb->m_managedCallback->StepComplete(
                                                   (ICorDebugAppDomain*) thread->m_pAppDomain,
                                                   (ICorDebugThread*) thread,
                                                   (ICorDebugStepper*) stepper,
                                                   event->StepData.reason);

            }

            if (stepper != NULL)
                stepper->Release();
        }
        break;

    case DB_IPCE_EXCEPTION:
        {
            LOG((LF_CORDB, LL_INFO1000, "[%x] RCET::DRCE: exception.\n",
                 GetCurrentThreadId()));

            _ASSERTE(thread != NULL);
            _ASSERTE(pAppDomain != NULL);
           
            thread->m_exception = true;
            thread->m_continuable = event->Exception.continuable;
            thread->m_thrown = event->Exception.exceptionHandle;

            if (m_cordb->m_managedCallback)
            {
                _ASSERTE (thread->m_pAppDomain != NULL);

                m_cordb->m_managedCallback->Exception((ICorDebugAppDomain*) thread->m_pAppDomain,
                                                      (ICorDebugThread*) thread,
                                                      !event->Exception.firstChance);

            }
        }
        break;

    case DB_IPCE_SYNC_COMPLETE:
        _ASSERTE(!"Should have never queued a sync complete event.");
        break;

    case DB_IPCE_THREAD_ATTACH:
        {
            LOG((LF_CORDB, LL_INFO100, "[%x] RCET::DRCE: thread attach : ID=%x.\n", 
                 GetCurrentThreadId(), event->threadId));

            Lock();

#ifdef _DEBUG
            _ASSERTE(thread == NULL);
#endif
            // Dup the runtime thread's handle into our process.
            HANDLE threadHandle;
            BOOL succ = DuplicateHandle(this->m_handle,
                                        event->ThreadAttachData.threadHandle,
                                        GetCurrentProcess(),
                                        &threadHandle,
                                        NULL, FALSE, DUPLICATE_SAME_ACCESS);

            if (succ)
            {
                HandleManagedCreateThread(event->threadId, threadHandle);

                thread =
                    (CordbThread*) m_userThreads.GetBase(event->threadId);

                _ASSERTE(thread != NULL);

				thread->m_debuggerThreadToken =
					event->ThreadAttachData.debuggerThreadToken;
                thread->m_firstExceptionHandler = 
                    event->ThreadAttachData.firstExceptionHandler;

                _ASSERTE(thread->m_firstExceptionHandler != NULL);

				thread->AddRef();

                _ASSERTE (pAppDomain != NULL);
                
                thread->m_pAppDomain = pAppDomain;
				pAppDomain->m_fHasAtLeastOneThreadInsideIt = true;
			
                Unlock();
        
                if (m_cordb->m_managedCallback)
                {
                    m_cordb->m_managedCallback->CreateThread(
                                                 (ICorDebugAppDomain*) pAppDomain,
                                                 (ICorDebugThread*) thread);

                }

                thread->Release();
            }
            else
            {
            // If we failed b/c the LS exited, then just ignore this event
            // and make way for the ExitProcess() callback.            
                if (CheckIfLSExited())
                {
                    Unlock();
                    Continue(FALSE);
                    break;                    
                }
                
                Unlock();
                CORDBProcessSetUnrecoverableWin32Error(this, 0);
            }
        }
        break;
        
    case DB_IPCE_THREAD_DETACH:
        {
            LOG((LF_CORDB, LL_INFO100, "[%x] RCET::HRCE: thread detach : ID=%x \n", 
                 GetCurrentThreadId(), event->threadId));

            Lock();

            // If the runtime thread never entered managed code, there
            // won't be a CordbThread, and CreateThread was never
            // called, so don't bother calling ExitThread.
            if (thread != NULL)
            {
                thread->AddRef();

                _ASSERTE(pAppDomain != NULL);
                _ASSERTE(thread->m_detached);

                // Remove the thread from the hash.
                m_userThreads.RemoveBase(event->threadId);

                // Remove this app domain if we can.
                if (pAppDomain->IsMarkedForDeletion() == TRUE)
                {
                    pAppDomain->AddRef();
                    pAppDomain->m_pProcess->Release();
                    m_appDomains.RemoveBase((ULONG)event->appDomainToken);
                }
            
                Unlock();

                if (m_cordb->m_managedCallback)
                {
                    LOG((LF_CORDB, LL_INFO1000, "[%x] RCET::HRCE: sending "
                         "thread detach.\n", 
                         GetCurrentThreadId()));
                    
                    m_cordb->m_managedCallback->ExitThread(
                                       (ICorDebugAppDomain*) pAppDomain,
                                       (ICorDebugThread*) thread);

                }

                if (pAppDomain->IsMarkedForDeletion() == TRUE)
                {
                    pAppDomain->Release();
                }

                thread->Release();
            }
            else
            {
                Unlock();
                Continue(FALSE);
            }
        }
        break;
        
    case DB_IPCE_LOAD_MODULE:
        {
            LOG((LF_CORDB, LL_INFO100,
                 "RCET::HRCE: load module on thread %#x Mod:0x%08x Asm:0x%08x AD:0x%08x Metadata:0x%08x/%d IsDynamic:%d\n", 
                 event->threadId,
                 event->LoadModuleData.debuggerModuleToken,
                 event->LoadModuleData.debuggerAssemblyToken,
                 event->appDomainToken,
                 event->LoadModuleData.pMetadataStart,
                 event->LoadModuleData.nMetadataSize,
                 event->LoadModuleData.fIsDynamic));

            _ASSERTE (pAppDomain != NULL);

            CordbModule *moduleDup = (CordbModule*) pAppDomain->LookupModule (
							event->LoadModuleData.debuggerModuleToken);
            if (moduleDup != NULL)
            {
                LOG((LF_CORDB, LL_INFO100, "Already loaded Module - continue()ing!" ));
#ifdef RIGHT_SIDE_ONLY
                pAppDomain->Continue(FALSE);

#endif //RIGHT_SIDE_ONLY    
                break;
            }
            _ASSERTE(moduleDup == NULL);

            CordbAssembly *pAssembly =
				(CordbAssembly *)pAppDomain->m_assemblies.GetBase (
							(ULONG)event->LoadModuleData.debuggerAssemblyToken);

            // It is possible to get a load module event before the corresponding 
            // assembly has been loaded. Therefore, just ignore the event and continue. 
            // A load module event for this module will be sent by the left side
            // after it has loaded the assembly.
            if (pAssembly == NULL)
            {
                LOG((LF_CORDB, LL_INFO100, "Haven't loaded Assembly "
                    "yet - continue()ing!" ));
#ifdef RIGHT_SIDE_ONLY
                pAppDomain->Continue(FALSE);

#endif //RIGHT_SIDE_ONLY    
                
            }
            else
            {
                HRESULT hr = S_OK;
                CordbModule* module = new CordbModule(
                                        this,
                                        pAssembly,
                                        event->LoadModuleData.debuggerModuleToken,
                                        event->LoadModuleData.pMetadataStart,
                                        event->LoadModuleData.nMetadataSize,
                                        event->LoadModuleData.pPEBaseAddress,
                                        event->LoadModuleData.nPESize,
                                        event->LoadModuleData.fIsDynamic,
                                        event->LoadModuleData.fInMemory,
                                        event->LoadModuleData.rcName,
                                        pAppDomain);

                if (module != NULL)
                {
                    hr = module->Init();

                    if (SUCCEEDED(hr))
                    {
                        hr = pAppDomain->m_modules.AddBase(module);

                        if (SUCCEEDED(hr))
                        {
                            if (m_cordb->m_managedCallback)
                            {
                                m_cordb->m_managedCallback->LoadModule(
                                                 (ICorDebugAppDomain*) pAppDomain,
                                                 (ICorDebugModule*) module);

                            }
                        }
                        else
                            CORDBSetUnrecoverableError(this, hr, 0);
                    }
                    else
                        CORDBSetUnrecoverableError(this, hr, 0);
                }
                else
                    CORDBSetUnrecoverableError(this, E_OUTOFMEMORY, 0);
            }
        }


        break;

    case DB_IPCE_UNLOAD_MODULE:
        {
            LOG((LF_CORDB, LL_INFO100, "RCET::HRCE: unload module on thread %#x Mod:0x%x AD:0x%08x\n", 
                 event->threadId,
                 event->UnloadModuleData.debuggerModuleToken,
                 event->appDomainToken));

            _ASSERTE (pAppDomain != NULL);

            CordbModule *module = (CordbModule*) pAppDomain->LookupModule (
                            event->UnloadModuleData.debuggerModuleToken);
            if (module == NULL)
            {
                LOG((LF_CORDB, LL_INFO100, "Already unloaded Module - continue()ing!" ));
#ifdef RIGHT_SIDE_ONLY
                pAppDomain->Continue(FALSE);

#endif //RIGHT_SIDE_ONLY    
                break;
            }
            _ASSERTE(module != NULL);

            // The appdomain we're unloading in must be the appdomain we were loaded in. Otherwise, we've got mismatched
            // module and appdomain pointers. Bugs 65943 & 81728.
            _ASSERTE(pAppDomain == module->GetAppDomain());

            if (m_cordb->m_managedCallback)
            {
                m_cordb->m_managedCallback->UnloadModule((ICorDebugAppDomain*) pAppDomain,
                                                         (ICorDebugModule*) module);

            }

            pAppDomain->m_modules.RemoveBase(
                         (ULONG) event->UnloadModuleData.debuggerModuleToken);
        }
        break;

    case DB_IPCE_LOAD_CLASS:
        {
            HRESULT hrConvert = S_OK;
            void * remotePtr = NULL;

            LOG((LF_CORDB, LL_INFO10000,
                 "RCET::HRCE: load class on thread %#x Tok:0x%08x Mod:0x%08x Asm:0x%08x AD:0x%08x\n", 
                 event->threadId,
                 event->LoadClass.classMetadataToken,
                 event->LoadClass.classDebuggerModuleToken,
                 event->LoadClass.classDebuggerAssemblyToken,
                 event->appDomainToken));

			_ASSERTE (pAppDomain != NULL);

            CordbModule* module =
                (CordbModule*) pAppDomain->LookupModule(event->LoadClass.classDebuggerModuleToken);
            if (module == NULL)
            {
                LOG((LF_CORDB, LL_INFO100, "Load Class on not-loaded Module - continue()ing!" ));
#ifdef RIGHT_SIDE_ONLY
                pAppDomain->Continue(FALSE);

#endif //RIGHT_SIDE_ONLY    
                break;
            }
            _ASSERTE(module != NULL);

            BOOL dynamic = FALSE;
            HRESULT hr = module->IsDynamic(&dynamic);
            if (FAILED(hr))
            {
                Continue(FALSE);
                break;
            }

            // If this is a class load in a dynamic module, then we'll have
            // to grab an up-to-date copy of the metadata from the left side,
            // then send the "release buffer" message to free the memory.
            if (dynamic && !FAILED(hr))
            {
                BYTE *pMetadataCopy;
                            
                // Get it
                remotePtr = event->LoadClass.pNewMetaData;
                BOOL succ = TRUE;
                if (remotePtr != NULL)
                {
                    DWORD dwErr;
                    dwErr = CordbModule::m_metadataPointerCache.CopyRemoteMetadata(
                        m_handle,
                        remotePtr, 
                        event->LoadClass.cbNewMetaData, 
                        &pMetadataCopy);
                    
                    if(dwErr == (DWORD) E_OUTOFMEMORY)
                    {
                        Continue(FALSE);
                        break; // out of the switch
                    }
                    else if (FAILED(dwErr))
                    {
                        succ = false;
                    }
                }
                // Deal with problems involved in getting it.
                if (succ)
                {
                    event->LoadClass.pNewMetaData = pMetadataCopy;
                }
                else
                {
                    event->LoadClass.pNewMetaData = NULL;
                }

                hrConvert = module->ConvertToNewMetaDataInMemory(
                    event->LoadClass.pNewMetaData,
                    event->LoadClass.cbNewMetaData);
                    
                if (FAILED(hrConvert))
                {
                    LOG((LF_CORDB, LL_INFO1000, "RCET::HRCE: Failed to convert MD!\n"));
                    Continue(FALSE);
                    break;
    	        }
    	        
            }

            CordbClass *pClass = module->LookupClass(
                                       event->LoadClass.classMetadataToken);

            if (pClass == NULL)
            {
                HRESULT hr = module->CreateClass(
                                     event->LoadClass.classMetadataToken,
                                     &pClass);

                if (!SUCCEEDED(hr))
                    pClass = NULL;
            }

            if (pClass->m_loadEventSent)
            {
                // Dynamic modules are dynamic at the module level - 
                // you can't add a new version of a class once the module
                // is baked.
                // EnC adds completely new classes.
                // There shouldn't be any other way to send multiple
                // ClassLoad events.
                // Except that there are race conditions between loading
                // an appdomain, and loading a class, so if we get the extra
                // class load, we should ignore it.

                Continue(FALSE);
                break; //out of the switch statement
            }

            pClass->m_loadEventSent = TRUE;

            if (dynamic && remotePtr != NULL)
            {
                // Free it on the left side
                // Now free the left-side memory
                DebuggerIPCEvent eventReleaseBuffer;

                InitIPCEvent(&eventReleaseBuffer, 
                             DB_IPCE_RELEASE_BUFFER, 
                             true,
                             NULL);

                // Indicate the buffer to release
                eventReleaseBuffer.ReleaseBuffer.pBuffer = remotePtr;

                // Make the request, which is synchronous
                hr = SendIPCEvent(&eventReleaseBuffer, sizeof(eventReleaseBuffer));
#ifdef _DEBUG
                if (FAILED(hr))
                    LOG((LF_CORDB, LL_INFO1000, "RCET::HRCE: Failed to send msg!\n"));
#endif                    
            }

            if (pClass != NULL)
            {
                if (m_cordb->m_managedCallback)
                {
                    m_cordb->m_managedCallback->LoadClass(
                                               (ICorDebugAppDomain*) pAppDomain,
                                               (ICorDebugClass*) pClass);

                }
            }
        }
        break;

    case DB_IPCE_UNLOAD_CLASS:
        {
            LOG((LF_CORDB, LL_INFO10000,
                 "RCET::HRCE: unload class on thread %#x Tok:0x%08x Mod:0x%08x AD:0x%08x\n", 
                 event->threadId,
                 event->UnloadClass.classMetadataToken,
                 event->UnloadClass.classDebuggerModuleToken,
                 event->appDomainToken));

            // get the appdomain object
            _ASSERTE (pAppDomain != NULL);

            CordbModule *module = (CordbModule*) pAppDomain->LookupModule (
                            event->UnloadClass.classDebuggerModuleToken);
            if (module == NULL)
            {
                LOG((LF_CORDB, LL_INFO100, "Unload Class on not-loaded Module - continue()ing!" ));
#ifdef RIGHT_SIDE_ONLY
                pAppDomain->Continue(FALSE);

#endif //RIGHT_SIDE_ONLY    
                break;
            }
            _ASSERTE(module != NULL);

            CordbClass *pClass = module->LookupClass(
                                       event->UnloadClass.classMetadataToken);
            
            if (pClass != NULL && !pClass->m_hasBeenUnloaded)
            {
                pClass->m_hasBeenUnloaded = true;
                if (m_cordb->m_managedCallback)
                {
                    m_cordb->m_managedCallback->UnloadClass(
                                            (ICorDebugAppDomain*) pAppDomain,
                                            (ICorDebugClass*) pClass);

                }
            }
            else
            {
                LOG((LF_CORDB, LL_INFO10000,
                     "Unload already unloaded class 0x%08x.\n",
                     event->UnloadClass.classMetadataToken));
                
#ifdef RIGHT_SIDE_ONLY
                Continue(FALSE);
#endif //RIGHT_SIDE_ONLY    
            }
        }
        break;

    case DB_IPCE_FIRST_LOG_MESSAGE:
            ProcessFirstLogMessage (event);

        break;

    case DB_IPCE_CONTINUED_LOG_MESSAGE:
            ProcessContinuedLogMessage (event);

        break;

    case DB_IPCE_LOGSWITCH_SET_MESSAGE:
        {

            LOG((LF_CORDB, LL_INFO10000, 
                "[%x] RCET::DRCE: Log Switch Setting Message.\n",
                 GetCurrentThreadId()));

            Lock();

            _ASSERTE(thread != NULL);

            Unlock();

            int iSwitchNameLength = wcslen (&event->LogSwitchSettingMessage.Dummy[0]);
            int iParentNameLength = wcslen (
                        &event->LogSwitchSettingMessage.Dummy[iSwitchNameLength+1]);

            // allocate memory for storing the logswitch name and parent's name
            // This memory will be free by us after returning from the callback.
            WCHAR *pstrLogSwitchName;
            WCHAR *pstrParentName = NULL;

            _ASSERTE (iSwitchNameLength > 0);
            if (
                ((pstrLogSwitchName = new WCHAR [iSwitchNameLength+1])
                    != NULL)
                &&
                ((pstrParentName = new WCHAR [iParentNameLength+1])
                    != NULL)
                )
            {
                wcscpy (pstrLogSwitchName, 
                    &event->LogSwitchSettingMessage.Dummy[0]);
                wcscpy (pstrParentName, 
                    &event->LogSwitchSettingMessage.Dummy[iSwitchNameLength+1]);

                // Do the callback...
                if (m_cordb->m_managedCallback)
                {
                    // from the thread object get the appdomain object
                    pAppDomain = thread->m_pAppDomain;
                    _ASSERTE (pAppDomain != NULL);

                    m_cordb->m_managedCallback->LogSwitch(
                                               (ICorDebugAppDomain*) pAppDomain,
                                               (ICorDebugThread*) thread,
                                               event->LogSwitchSettingMessage.iLevel,
                                               event->LogSwitchSettingMessage.iReason,
                                               pstrLogSwitchName,
                                               pstrParentName);

                }

                delete [] pstrLogSwitchName;
                delete [] pstrParentName;
            }
            else
            {
                if (pstrLogSwitchName != NULL)
                    delete [] pstrLogSwitchName;
            }
        }

        break;

    case DB_IPCE_CREATE_APP_DOMAIN:
        {
            LOG((LF_CORDB, LL_INFO100,
                 "RCET::HRCE: create appdomain on thread %#x AD:0x%08x \n", 
                 event->threadId,
                 event->appDomainToken));

			CordbAppDomain* pAppDomainDup =
					(CordbAppDomain*) m_appDomains.GetBase(
							(ULONG)event->appDomainToken);

            // Remove this app domain if we can.
            if (pAppDomainDup)
            {
                _ASSERTE(pAppDomainDup->IsMarkedForDeletion());

                pAppDomainDup->AddRef();
                pAppDomainDup->m_pProcess->Release();
                m_appDomains.RemoveBase((ULONG)event->appDomainToken);
                pAppDomainDup->Release();
            }

            pAppDomain = new CordbAppDomain(
                                    this,
                                    event->appDomainToken,
                                    event->AppDomainData.id,
                                    event->AppDomainData.rcName);

            if (pAppDomain != NULL)
            {
                this->AddRef();

                HRESULT hr = m_appDomains.AddBase(pAppDomain);

                if (SUCCEEDED(hr))
                {
                    if (m_cordb->m_managedCallback)
                    {
                        hr = m_cordb->m_managedCallback->CreateAppDomain(
                                         (ICorDebugProcess*) this,
                                         (ICorDebugAppDomain*) pAppDomain);

                        // If they don't implement this callback, then just attach and continue.
                        if (hr == E_NOTIMPL)
                        {
                            pAppDomain->Attach();
                            pAppDomain->Continue(FALSE);
                        }
                    }
                }
                else
                    CORDBSetUnrecoverableError(this, hr, 0);
            }
            else
                CORDBSetUnrecoverableError(this, E_OUTOFMEMORY, 0);
        }


        break;

    case DB_IPCE_EXIT_APP_DOMAIN:
        {
            LOG((LF_CORDB, LL_INFO100, "RCET::HRCE: exit appdomain on thread %#x AD:0x%08x \n", 
                 event->threadId,
                 event->appDomainToken));

            _ASSERTE (pAppDomain != NULL);

            if (m_cordb->m_managedCallback)
            {
                HRESULT hr = m_cordb->m_managedCallback->ExitAppDomain(
                                               (ICorDebugProcess*) this,
                                               (ICorDebugAppDomain*) pAppDomain);

                // Just continue if they didn't implement the callback.
                if (hr == E_NOTIMPL)
                {
                    pAppDomain->Continue(FALSE);
                }
            }


            // Mark the app domain for deletion from the appdomain hash. Need to 
            // do this since the app domain is destroyed before the last thread
            // has exited. Therefore, this appdomain will be removed from the hash
            // list upon receipt of the "ThreadDetach" event.
            pAppDomain->MarkForDeletion();
        }

        break;

    case DB_IPCE_LOAD_ASSEMBLY:
        {
            HRESULT hr = S_OK;

            LOG((LF_CORDB, LL_INFO100,
                 "RCET::HRCE: load assembly on thread %#x Asm:0x%08x AD:0x%08x \n", 
                 event->threadId,
                 event->AssemblyData.debuggerAssemblyToken,
                 event->appDomainToken));

			_ASSERTE (pAppDomain != NULL);

            // If the debugger detached from, then reattached to an AppDomain,
            //  this side may get LoadAssembly messages for previously loaded
            //  Assemblies.
                        
            // Determine if this Assembly is cached.
            CordbAssembly* assembly =
                (CordbAssembly*) pAppDomain->m_assemblies.GetBase(
                         (ULONG) event->AssemblyData.debuggerAssemblyToken);
            
            if (assembly != NULL)
            { 
                // We may receive multiple LOAD_ASSEMBLY events in the case of shared assemblies
                // (since the EE doesn't quite produce them in a reliable way.)  So if we see
                // a duplicate here, just ignore it.
                
                // If the Assembly is cached, assert that the properties are unchanged.
                _ASSERTE(wcscmp(assembly->m_szAssemblyName, event->AssemblyData.rcName) == 0);
                _ASSERTE(assembly->m_fIsSystemAssembly == event->AssemblyData.fIsSystemAssembly);

                pAppDomain->Continue(FALSE);
            }
            else
            {
                //currently, event->AssemblyData.fIsSystemAssembly is never true
                assembly = new CordbAssembly(
                                pAppDomain,
                                event->AssemblyData.debuggerAssemblyToken,
                                event->AssemblyData.rcName,
                                event->AssemblyData.fIsSystemAssembly);
    
                if (assembly != NULL)
                {
                    hr = pAppDomain->m_assemblies.AddBase(assembly);
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }

                // If created, or have, an Assembly, notify callback.
                if (SUCCEEDED(hr))
                {
                    if (m_cordb->m_managedCallback)
                    {
                        hr = m_cordb->m_managedCallback->LoadAssembly(
                                                                      (ICorDebugAppDomain*) pAppDomain,
                                                                      (ICorDebugAssembly*) assembly);

                        // Just continue if they didn't implement the callback.
                        if (hr == E_NOTIMPL)
                        {
                            pAppDomain->Continue(FALSE);
                        }
                    }
                }
                else
                    CORDBSetUnrecoverableError(this, hr, 0);
            }
        }

        break;

    case DB_IPCE_UNLOAD_ASSEMBLY:
        {
            LOG((LF_CORDB, LL_INFO100, "RCET::DRCE: unload assembly on thread %#x Asm:0x%x AD:0x%x\n", 
                 event->threadId,
                 event->AssemblyData.debuggerAssemblyToken,
                 event->appDomainToken));

            _ASSERTE (pAppDomain != NULL);

            CordbAssembly* assembly =
                (CordbAssembly*) pAppDomain->m_assemblies.GetBase(
                         (ULONG) event->AssemblyData.debuggerAssemblyToken);
            if (assembly == NULL)
            {
                LOG((LF_CORDB, LL_INFO100, "Assembly not loaded - continue()ing!" ));
#ifdef RIGHT_SIDE_ONLY
                pAppDomain->Continue(FALSE);

#endif //RIGHT_SIDE_ONLY    
                break;
            }
            _ASSERTE(assembly != NULL);

            if (m_cordb->m_managedCallback)
            {
                HRESULT hr = m_cordb->m_managedCallback->UnloadAssembly(
                                               (ICorDebugAppDomain*) pAppDomain,
                                               (ICorDebugAssembly*) assembly);

                // Just continue if they didn't implement this callback.
                if (hr == E_NOTIMPL)
                {
                    pAppDomain->Continue(FALSE);
                }
            }

            pAppDomain->m_assemblies.RemoveBase(
                         (ULONG) event->AssemblyData.debuggerAssemblyToken);
        }

        break;

    case DB_IPCE_FUNC_EVAL_COMPLETE:
        {
            LOG((LF_CORDB, LL_INFO1000, "RCET::DRCE: func eval complete.\n"));

            CordbEval *pEval = (CordbEval*)event->FuncEvalComplete.funcEvalKey;

            Lock();

            _ASSERTE(thread != NULL);
            _ASSERTE(pAppDomain != NULL);

            // Hold the data about the result in the CordbEval for later.
            pEval->m_complete = true;
            pEval->m_successful = event->FuncEvalComplete.successful;
            pEval->m_aborted = event->FuncEvalComplete.aborted;
            pEval->m_resultAddr = event->FuncEvalComplete.resultAddr;
            pEval->m_resultType = event->FuncEvalComplete.resultType;
            pEval->m_resultDebuggerModuleToken = event->FuncEvalComplete.resultDebuggerModuleToken;
            pEval->m_resultAppDomainToken = event->appDomainToken;

            // If we did this func eval with this thread stopped at an excpetion, then we need to pretend as if we
            // really didn't continue from the exception, since, of course, we really didn't on the Left Side.
            if (pEval->m_evalDuringException)
            {
                thread->m_exception = true;
            }
            
            Unlock();

            bool evalCompleted = pEval->m_successful || pEval->m_aborted;

            // Corresponding AddRef() in CallFunction()
            // If a CallFunction() is aborted, the LHS may not complete the abort
            // immediately and hence we cant do a SendCleanup() at that point. Also,
            // the debugger may (incorrectly) release the CordbEval before this
            // DB_IPCE_FUNC_EVAL_COMPLETE event is received. Hence, we maintain an
            // extra ref-count to determine when this can be done.
            // Note that this can cause a two-way DB_IPCE_FUNC_EVAL_CLEANUP event
            // to be sent. Hence, it has to be done before the Continue (see bug 102745).

            pEval->Release();

            if (m_cordb->m_managedCallback)
            {
                // Note that if the debugger has already (incorrectly) released the CordbEval,
                // pEval will be pointing to garbage and should not be used by the debugger.
                if (evalCompleted)
                    m_cordb->m_managedCallback->EvalComplete(
                                          (ICorDebugAppDomain*)pAppDomain,
                                          (ICorDebugThread*)thread,
                                          (ICorDebugEval*)pEval);
                else
                    m_cordb->m_managedCallback->EvalException(
                                          (ICorDebugAppDomain*)pAppDomain,
                                          (ICorDebugThread*)thread,
                                          (ICorDebugEval*)pEval);
            }
            else
            {
#ifdef RIGHT_SIDE_ONLY
                pAppDomain->Continue(FALSE);
#endif //RIGHT_SIDE_ONLY    
            }
        }
        break;


    case DB_IPCE_NAME_CHANGE:
        {
            LOG((LF_CORDB, LL_INFO1000, "[%x] RCET::HRCE: Name Change %d  0x%08x 0x%08x\n", 
                 GetCurrentThreadId(),
                 event->threadId,
                 event->NameChange.debuggerAppDomainToken,
                 event->NameChange.debuggerThreadToken));

            thread = NULL;
            pAppDomain = NULL;
            if (event->NameChange.eventType == THREAD_NAME_CHANGE)
            {
                // Lookup the CordbThread that matches this runtime thread.
                thread = (CordbThread*) m_userThreads.GetBase(
                                        event->NameChange.debuggerThreadToken);
            }
            else
            {
                _ASSERTE (event->NameChange.eventType == APP_DOMAIN_NAME_CHANGE);
                pAppDomain = (CordbAppDomain*) m_appDomains.GetBase(
                                (ULONG)event->appDomainToken);
                if (pAppDomain)
                    pAppDomain->m_nameIsValid = false;
            }

            if (thread || pAppDomain)
            {
                if (m_cordb->m_managedCallback)
                {
                    m_cordb->m_managedCallback->NameChange(
                                                   (ICorDebugAppDomain*) pAppDomain,
                                                   (ICorDebugThread*) thread);
                }
            }
            else
            {
                Continue(FALSE);
            }
        }

        break;
        
    case DB_IPCE_UPDATE_MODULE_SYMS:
        {
            LOG((LF_CORDB, LL_INFO1000,
                 "RCET::HRCE: update module syms 0x%08x 0x%08x 0x%08x %d\n", 
                 event->UpdateModuleSymsData.debuggerModuleToken,
                 event->UpdateModuleSymsData.debuggerAppDomainToken,
                 event->UpdateModuleSymsData.pbSyms,
                 event->UpdateModuleSymsData.cbSyms));

            // Find the app domain the module lives in.
            _ASSERTE (pAppDomain != NULL);

            // Find the Right Side module for this module.
            CordbModule *module = (CordbModule*) pAppDomain->LookupModule (
                            event->UpdateModuleSymsData.debuggerModuleToken);
            _ASSERTE(module != NULL);

            // Make room for the memory on this side.
            BYTE *syms = new BYTE[event->UpdateModuleSymsData.cbSyms];
            
            _ASSERTE(syms != NULL);
            if (!syms)
            {
                CORDBSetUnrecoverableError(this, E_OUTOFMEMORY, 0);            
                break;
            }
            
            // Read the data from the Left Side.
#ifdef _DEBUG
            BOOL succ =
#endif
            ReadProcessMemoryI(m_handle,
                                           event->UpdateModuleSymsData.pbSyms,
                                           syms,
                                           event->UpdateModuleSymsData.cbSyms,
                                           NULL);
            _ASSERTE(succ);

            // Create a stream from the memory.
            IStream *pStream = NULL;
#ifdef _DEBUG
            HRESULT hr =
#endif
            CInMemoryStream::CreateStreamOnMemoryCopy(
                                         syms,
                                         event->UpdateModuleSymsData.cbSyms,
                                         &pStream);
            _ASSERTE(SUCCEEDED(hr) && (pStream != NULL));

            // Free memory on the left side if we need to.
            if (event->UpdateModuleSymsData.needToFreeMemory)
            {
                DebuggerIPCEvent eventReleaseBuffer;

                InitIPCEvent(&eventReleaseBuffer, 
                             DB_IPCE_RELEASE_BUFFER, 
                             true,
                             NULL);

                // Indicate the buffer to release.
                eventReleaseBuffer.ReleaseBuffer.pBuffer =
                    event->UpdateModuleSymsData.pbSyms;

                // Make the request, which is synchronous.
                SendIPCEvent(&eventReleaseBuffer, sizeof(eventReleaseBuffer));
            }
            
            if (m_cordb->m_managedCallback)
            {
                m_cordb->m_managedCallback->UpdateModuleSymbols(
                                     (ICorDebugAppDomain*) pAppDomain,
                                     (ICorDebugModule*) module,
                                     pStream);
            }

            pStream->Release();
            delete [] syms;
        }
        break;

    case DB_IPCE_CONTROL_C_EVENT:
        {
            LOG((LF_CORDB, LL_INFO1000, "[%x] RCET::HRCE: ControlC Event %d  0x%08x\n", 
                 GetCurrentThreadId(),
                 event->threadId,
                 event->Exception.exceptionHandle));


			HRESULT hr = S_FALSE;

			if (m_cordb->m_managedCallback)
            {
                hr = m_cordb->m_managedCallback->ControlCTrap((ICorDebugProcess*) this);
            }

            DebuggerIPCEvent eventControlCResult;

            InitIPCEvent(&eventControlCResult, 
                         DB_IPCE_CONTROL_C_EVENT_RESULT, 
                         false,
                         NULL);

            // Indicate the buffer to release.
            eventControlCResult.hr = hr;

            // Send the event
            SendIPCEvent(&eventControlCResult, sizeof(eventControlCResult));

        }
		break;

        case DB_IPCE_ENC_REMAP:
        {
#ifdef RIGHT_SIDE_ONLY
            LOG((LF_CORDB, LL_INFO1000, "[%x] RCET::DRCE: EnC Remap!.\n",
                 GetCurrentThreadId()));

            _ASSERTE(NULL != pAppDomain);
            if (m_cordb->m_managedCallback)
            {
                CordbModule* module = (CordbModule *)pAppDomain->LookupModule(
                                            event->EnCRemap.debuggerModuleToken);
                CordbFunction *f = NULL;

                if (module != NULL)
                {
                    f = (CordbFunction *)module->LookupFunction(
                            event->EnCRemap.funcMetadataToken);
                    if (f == NULL)
                    {
                        HRESULT hr = module->CreateFunction(
                                    (mdMethodDef)event->EnCRemap.funcMetadataToken,
                                    event->EnCRemap.RVA,
                                    &f);
                        _ASSERTE( SUCCEEDED(hr) || !"FAILURE" );
                        if (SUCCEEDED(hr))
                            f->SetLocalVarToken(event->EnCRemap.localSigToken);
                    }
                }
                
                m_cordb->m_managedCallback->EditAndContinueRemap(
                                      (ICorDebugAppDomain*)pAppDomain,
                                      (ICorDebugThread*) thread,
                                      (ICorDebugFunction *)f,
                                      event->EnCRemap.fAccurate);
            }
#endif // RIGHT_SIDE_ONLY
        }
        break;

        case DB_IPCE_BREAKPOINT_SET_ERROR:
        {
#ifdef RIGHT_SIDE_ONLY
            LOG((LF_CORDB, LL_INFO1000, "RCET::DRCE: breakpoint set error.\n"));

            Lock();

            _ASSERTE(thread != NULL); 
            _ASSERTE(pAppDomain != NULL);

            // Find the breakpoint object on this side.
            CordbBreakpoint *bp = (CordbBreakpoint *)thread->m_pAppDomain->m_breakpoints.GetBase(
                                                           (unsigned long) event->BreakpointSetErrorData.breakpointToken);

            if (bp != NULL)
                bp->AddRef();

            ICorDebugBreakpoint *ibp = CordbBreakpointToInterface(bp);
            _ASSERTE(ibp != NULL);
            
            Unlock();

            if (m_cordb->m_managedCallback && (bp != NULL))
            {
                m_cordb->m_managedCallback->BreakpointSetError((ICorDebugAppDomain*) thread->m_pAppDomain,
                                                               (ICorDebugThread*) thread,
                                                               ibp,
                                                               0);
            }

            if (bp != NULL)
                bp->Release();
            else
            {
                // If we didn't find a breakpoint object on this side,
                // the we have an extra BP event for a breakpoint that
                // has been removed and released on this side. Just
                // ignore the event.
                Continue(FALSE);
            }
#endif // RIGHT_SIDE_ONLY
        }
        break;
    default:
        LOG((LF_CORDB, LL_INFO1000,
             "[%x] RCET::HRCE: Unknown event: 0x%08x\n", 
             GetCurrentThreadId(), event->type));
    }

    Lock();

    //
    // Set for Continue().
    //
    m_dispatchingEvent = false;

    free(event);
}


HRESULT CordbProcess::EnumerateThreads(ICorDebugThreadEnum **ppThreads)
{
    VALIDATE_POINTER_TO_OBJECT(ppThreads,ICorDebugThreadEnum **);

    CordbHashTableEnum *e = new CordbHashTableEnum(&m_userThreads, 
                                                   IID_ICorDebugThreadEnum);
    if (e == NULL)
        return E_OUTOFMEMORY;

    *ppThreads = (ICorDebugThreadEnum*)e;
    e->AddRef();

    return S_OK;
}

HRESULT CordbProcess::GetThread(DWORD dwThreadId, ICorDebugThread **ppThread)
{
    VALIDATE_POINTER_TO_OBJECT(ppThread, ICorDebugThread **);

	HRESULT hr = S_OK;
	INPROC_LOCK();

    CordbThread *t = (CordbThread *) m_userThreads.GetBase(dwThreadId);

    if (t == NULL)
    {
    	hr = E_INVALIDARG;
    	goto LExit;
    }

    *ppThread = (ICorDebugThread*)t;
    (*ppThread)->AddRef();
    
LExit:
	INPROC_UNLOCK();
         
    return hr;
}

HRESULT CordbProcess::ThreadForFiberCookie(DWORD fiberCookie,
                                           ICorDebugThread **ppThread)
{
    HASHFIND find;
    CordbThread *t = NULL;

	INPROC_LOCK();

    Lock();
    
    for (t  = (CordbThread*)m_userThreads.FindFirst(&find);
         t != NULL;
         t  = (CordbThread*)m_userThreads.FindNext(&find))
    {
        // The fiber cookie is really a ptr to the EE's Thread object,
        // which is exactly what out m_debuggerThreadToken is.
        if ((DWORD)t->m_debuggerThreadToken == fiberCookie)
            break;
    }

    Unlock();

    INPROC_UNLOCK();
    
    if (t == NULL)
        return S_FALSE;
    else
    {
        *ppThread = (ICorDebugThread*)t;
        (*ppThread)->AddRef();

        return S_OK;
    }
}

HRESULT CordbProcess::GetHelperThreadID(DWORD *pThreadID)
{
    if (pThreadID == NULL)
        return (E_INVALIDARG);

    // Return the ID of the current helper thread. There may be no thread in the process, or there may be a true helper
    // thread.
    if ((m_helperThreadId != 0) && !m_helperThreadDead)
        *pThreadID = m_helperThreadId;
    else if ((m_DCB != NULL) && (m_DCB->m_helperThreadId != 0))
        *pThreadID = m_DCB->m_helperThreadId;
    else
        *pThreadID = 0;

    return S_OK;
}

HRESULT CordbProcess::SetAllThreadsDebugState(CorDebugThreadState state,
                                              ICorDebugThread *pExceptThread)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    VALIDATE_POINTER_TO_OBJECT_OR_NULL(pExceptThread,ICorDebugThread *);

    CORDBLeftSideDeadIsOkay(this);
    CORDBSyncFromWin32StopIfNecessary(this);
    CORDBRequireProcessStateOKAndSync(this, NULL);
    
    CordbThread *et = (CordbThread*)pExceptThread;
    
    LOG((LF_CORDB, LL_INFO1000, "CP::SATDS: except thread=0x%08x 0x%x\n", pExceptThread, et != NULL ? et->m_id : 0));

    // Send one event to the Left Side to twiddle each thread's state.
    DebuggerIPCEvent event;
    InitIPCEvent(&event, DB_IPCE_SET_ALL_DEBUG_STATE, true, NULL);
    event.SetAllDebugState.debuggerExceptThreadToken = et != NULL ? et->m_debuggerThreadToken : NULL;
    event.SetAllDebugState.debugState = state;

    HRESULT hr = SendIPCEvent(&event, sizeof(DebuggerIPCEvent));

    // If that worked, then loop over all the threads on this side and set their states.
    if (SUCCEEDED(hr))
    {
        HASHFIND        find;
        CordbThread    *thread;
    
        for (thread = (CordbThread*)m_userThreads.FindFirst(&find);
             thread != NULL;
             thread = (CordbThread*)m_userThreads.FindNext(&find))
        {
            if (thread != et)
                thread->m_debugState = state;
        }
    }

    return hr;
#endif //RIGHT_SIDE_ONLY    
}


HRESULT CordbProcess::EnumerateObjects(ICorDebugObjectEnum **ppObjects)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    /* !!! */
    VALIDATE_POINTER_TO_OBJECT(ppObjects, ICorDebugObjectEnum **);

    return E_NOTIMPL;
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbProcess::IsTransitionStub(CORDB_ADDRESS address, BOOL *pbTransitionStub)
{
#ifdef RIGHT_SIDE_ONLY
    VALIDATE_POINTER_TO_OBJECT(pbTransitionStub, BOOL *);

    // Default to FALSE
    *pbTransitionStub = FALSE;
    
    CORDBLeftSideDeadIsOkay(this);

    // If we're not initialized, then it can't be a stub...
    if (!m_initialized)
        return S_OK;
    
    CORDBRequireProcessStateOK(this);
    CORDBSyncFromWin32StopIfNecessary(this);

    LOG((LF_CORDB, LL_INFO1000, "CP::ITS: addr=0x%08x\n", address));
    
    DebuggerIPCEvent *event = 
      (DebuggerIPCEvent *) _alloca(CorDBIPC_BUFFER_SIZE);

    InitIPCEvent(event, DB_IPCE_IS_TRANSITION_STUB, true, NULL);

    event->IsTransitionStub.address = (void*) address;

    HRESULT hr = SendIPCEvent(event, CorDBIPC_BUFFER_SIZE);

    if (FAILED(hr))
        return hr;

    _ASSERTE(event->type == DB_IPCE_IS_TRANSITION_STUB_RESULT);

    *pbTransitionStub = event->IsTransitionStubResult.isStub;

    return S_OK;
#else 
    return CORDBG_E_INPROC_NOT_IMPL;
#endif //RIGHT_SIDE_ONLY    
}


HRESULT CordbProcess::SetStopState(DWORD threadID, CorDebugThreadState state)
{
#ifdef RIGHT_SIDE_ONLY
    return E_NOTIMPL;
#else 
    return CORDBG_E_INPROC_NOT_IMPL;
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbProcess::IsOSSuspended(DWORD threadID, BOOL *pbSuspended)
{
#ifdef RIGHT_SIDE_ONLY
    // Gotta have a place for the result!
    if (!pbSuspended)
        return E_INVALIDARG;

    // Have we seen this thread?
    CordbUnmanagedThread *ut = GetUnmanagedThread(threadID);

    // If we have, and if we've suspended it, then say so.
    if (ut && ut->IsSuspended())
        *pbSuspended = TRUE;
    else
        *pbSuspended = FALSE;

    return S_OK;
#else 
    VALIDATE_POINTER_TO_OBJECT(pbSuspended, BOOL *);

    return CORDBG_E_INPROC_NOT_IMPL;
#endif //RIGHT_SIDE_ONLY    
}

//
// This routine reads a thread context from the process being debugged, taking into account the fact that the context
// record may be a different size than the one we compiled with. On systems < NT5, then OS doesn't usually allocate
// space for the extended registers. However, the CONTEXT struct that we compile with does have this space.
//
HRESULT CordbProcess::SafeReadThreadContext(void *pRemoteContext, CONTEXT *pCtx)
{
    HRESULT hr = S_OK;
    DWORD nRead = 0;
#if defined(CONTEXT_EXTENDED_REGISTERS)
    DWORD minContextSize = offsetof(CONTEXT, ExtendedRegisters);
    // The extended registers are optional...
    DWORD extRegSize = sizeof(CONTEXT) - minContextSize;

    // Start of the extended registers, in the remote process and in the current process
    void *pRmtExtReg = (void*)((UINT_PTR)pRemoteContext + minContextSize);
    void *pCurExtReg = (void*)((UINT_PTR)pCtx + minContextSize);
#else
    DWORD minContextSize = sizeof(CONTEXT);
#endif

    // Read the minimum part.
    BOOL succ = ReadProcessMemoryI(m_handle, pRemoteContext, pCtx, minContextSize, &nRead);

    if (!succ || (nRead != minContextSize))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

#if defined(CONTEXT_EXTENDED_REGISTERS)
    // Now, read the extended registers if the context contains them. If the context does not have extended registers,
    // just set them to zero.
    if (SUCCEEDED(hr) && (pCtx->ContextFlags & CONTEXT_EXTENDED_REGISTERS) == CONTEXT_EXTENDED_REGISTERS)
    {
        succ = ReadProcessMemoryI(m_handle, pRmtExtReg, pCurExtReg, extRegSize, &nRead);

        if (!succ || (nRead != extRegSize))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    else
    {
        memset(pCurExtReg, 0, extRegSize);
    }
#endif

    return hr;
}

//
// This routine writes a thread context to the process being debugged, taking into account the fact that the context
// record may be a different size than the one we compiled with. On systems < NT5, then OS doesn't usually allocate
// space for the extended registers. However, the CONTEXT struct that we compile with does have this space.
//
HRESULT CordbProcess::SafeWriteThreadContext(void *pRemoteContext, CONTEXT *pCtx)
{
    HRESULT hr = S_OK;
    SIZE_T nWritten = 0;
    DWORD sizeToWrite = sizeof(CONTEXT);

#if defined(CONTEXT_EXTENDED_REGISTERS)
    // If our context has extended registers, then write the whole thing. Otherwise, just write the minimum part.
    if ((pCtx->ContextFlags & CONTEXT_EXTENDED_REGISTERS) != CONTEXT_EXTENDED_REGISTERS)
        sizeToWrite = offsetof(CONTEXT, ExtendedRegisters);
#endif

    // Write the context.
    BOOL succ = WriteProcessMemory(m_handle, pRemoteContext, pCtx, sizeToWrite, &nWritten);

    if (!succ || (nWritten != sizeToWrite))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    return hr;
}


HRESULT CordbProcess::GetThreadContext(DWORD threadID, ULONG32 contextSize, BYTE context[])
{
#ifdef RIGHT_SIDE_ONLY  // This is not permitted in-proc

    if (contextSize != sizeof(CONTEXT))
    {
        LOG((LF_CORDB, LL_INFO10000, "CP::GTC: thread=0x%x, context size is invalid.\n", threadID));
        return E_INVALIDARG;
    }

    VALIDATE_POINTER_TO_OBJECT_ARRAY(context, BYTE, contextSize, true, true);

    // Find the unmanaged thread
    CordbUnmanagedThread *ut = GetUnmanagedThread(threadID);

    if (ut == NULL)
    {
        LOG((LF_CORDB, LL_INFO10000, "CP::GTC: thread=0x%x, thread id is invalid.\n", threadID));

        return E_INVALIDARG;
    }

    // If the thread is first chance hijacked, then read the context from the remote process. If the thread is generic
    // hijacked, then we have a copy of the thread's context already. Otherwise call the normal Win32 function.
	HRESULT hr = S_OK;
    
    LOG((LF_CORDB, LL_INFO10000, "CP::GTC: thread=0x%x, flags=0x%x.\n", threadID, ((CONTEXT*)context)->ContextFlags));
    
    if (ut->IsFirstChanceHijacked() || ut->IsHideFirstChanceHijackState())
    {
        LOG((LF_CORDB, LL_INFO10000, "CP::GTC: getting context from first chance hijack, addr=0x%08x.\n",
             ut->m_pLeftSideContext));

        // Read the context into a temp context then copy to the out param.
        CONTEXT tempContext;
        
        hr = SafeReadThreadContext(ut->m_pLeftSideContext, &tempContext);

        if (SUCCEEDED(hr))
            _CopyThreadContext((CONTEXT*)context, &tempContext);
    }
    else if (ut->IsGenericHijacked() || ut->IsSecondChanceHijacked())
    {
        LOG((LF_CORDB, LL_INFO10000, "CP::GTC: getting context from generic/2nd chance hijack.\n"));

        _CopyThreadContext((CONTEXT*)context, &(ut->m_context));
    }
    else
    {
        LOG((LF_CORDB, LL_INFO10000, "CP::GTC: getting context from win32.\n"));
        
        BOOL succ = ::GetThreadContext(ut->m_handle, (CONTEXT*)context);

        if (!succ)
            hr = HRESULT_FROM_WIN32(GetLastError());
    }

#ifdef _X86_
    LOG((LF_CORDB, LL_INFO10000,
         "CP::GTC: Eip=0x%08x, Esp=0x%08x, Eflags=0x%08x\n", ((CONTEXT*)context)->Eip, ((CONTEXT*)context)->Esp,
         ((CONTEXT*)context)->EFlags));
#elif defined(_PPC_)
    LOG((LF_CORDB, LL_INFO10000,
         "CP::GTC: Program Counter(Iar)=0x%08x, StackPointer(Gpr1)=0x%08x, Cr=0x%08x\n",
         ((CONTEXT*)context)->Iar, 
         ((CONTEXT*)context)->Gpr1,
         ((CONTEXT*)context)->Cr));
#else   // _X86_
    PORTABILITY_ASSERT("GetThreadContext's logging code needs a PC and stack pointer.");
#endif  // _X86_
    
    return hr;

#else  // In-proc

    return (CORDBG_E_INPROC_NOT_IMPL);

#endif
}

HRESULT CordbProcess::SetThreadContext(DWORD threadID, ULONG32 contextSize, BYTE context[])
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    if (contextSize != sizeof(CONTEXT))
    {
        LOG((LF_CORDB, LL_INFO10000, "CP::STC: thread=0x%x, context size is invalid.\n", threadID));
        return E_INVALIDARG;
    }

    VALIDATE_POINTER_TO_OBJECT_ARRAY(context, BYTE, contextSize, true, true);
    
    // Find the unmanaged thread
    CordbUnmanagedThread *ut = GetUnmanagedThread(threadID);

    if (ut == NULL)
    {
        LOG((LF_CORDB, LL_INFO10000, "CP::STC: thread=0x%x, thread is invalid.\n", threadID));
        return E_INVALIDARG;
    }

    LOG((LF_CORDB, LL_INFO10000,
         "CP::STC: thread=0x%x, flags=0x%x.\n", threadID, ((CONTEXT*)context)->ContextFlags));

#ifdef _X86_    
    LOG((LF_CORDB, LL_INFO10000,
         "CP::STC: Eip=0x%08x, Esp=0x%08x, Eflags=0x%08x\n", ((CONTEXT*)context)->Eip, ((CONTEXT*)context)->Esp,
         ((CONTEXT*)context)->EFlags));
#elif defined(_PPC_)
    LOG((LF_CORDB, LL_INFO10000,
         "CP::STC: Program Counter(Iar)=0x%08x, StackPointer(Gpr1)=0x%08x, Cr=0x%08x\n",
         ((CONTEXT*)context)->Iar, 
         ((CONTEXT*)context)->Gpr1,
         ((CONTEXT*)context)->Cr));
#else   // _X86_
    PORTABILITY_ASSERT("SetThreadContext's logging code needs a PC and stack pointer.");
#endif  // _X86_
    
    // If the thread is first chance hijacked, then write the context into the remote process. If the thread is generic
    // hijacked, then update the copy of the context that we already have. Otherwise call the normal Win32 function.
    HRESULT hr = S_OK;
    
    if (ut->IsFirstChanceHijacked() || ut->IsHideFirstChanceHijackState())
    {
        LOG((LF_CORDB, LL_INFO10000, "CP::STC: setting context from first chance hijack, addr=0x%08x.\n",
             ut->m_pLeftSideContext));

        // Grab the context from the left side into a temporary context, do the proper copy, then shove it back.
        CONTEXT tempContext;

        hr = SafeReadThreadContext(ut->m_pLeftSideContext, &tempContext);

        if (SUCCEEDED(hr))
        {
#ifdef _X86_
            LOG((LF_CORDB, LL_INFO10000,
                 "CP::STC: current FCH context: Eip=0x%08x, Esp=0x%08x, Eflags=0x%08x\n",
                 tempContext.Eip, tempContext.Esp, tempContext.EFlags));
#elif defined(_PPC_)
            LOG((LF_CORDB, LL_INFO10000,
                "CP::STC: current FCH context: Program Counter(Iar)=0x%08x, StackPointer(Gpr1)=0x%08x, Cr=0x%08x\n",
                ((CONTEXT*)context)->Iar, 
                ((CONTEXT*)context)->Gpr1,
                ((CONTEXT*)context)->Cr));
#else 
            PORTABILITY_ASSERT("SetThreadContext's logging code needs a PC and stack pointer.");
#endif  // _X86_
            
            _CopyThreadContext(&tempContext, (CONTEXT*)context);
            
            hr = SafeWriteThreadContext(ut->m_pLeftSideContext, &tempContext);
        }
    }
    else if (ut->IsGenericHijacked() || ut->IsSecondChanceHijacked()) 
    {
        LOG((LF_CORDB, LL_INFO10000, "CP::STC: setting context from generic/2nd chance hijack.\n"));

        _CopyThreadContext(&(ut->m_context), (CONTEXT*)context);
    }
    else
    {
        LOG((LF_CORDB, LL_INFO10000, "CP::STC: setting context from win32.\n"));
        
        BOOL succ = ::SetThreadContext(ut->m_handle, (CONTEXT*)context);

        if (!succ)
            hr = HRESULT_FROM_WIN32(GetLastError());
    }

    if (SUCCEEDED(hr))
    {
        // Find the managed thread
        CordbThread *pTh = (CordbThread *) m_userThreads.GetBase(threadID);
 
        if (pTh != NULL)
        {
            if (pTh->m_stackChains != NULL && pTh->m_stackChainCount > 0)
            {
                if (pTh->m_stackChains[0]->m_managed == false)
                {
                    SetDebuggerRegDisplayFromContext(&pTh->m_stackChains[0]->m_rd, (CONTEXT *) context);
                }
            }
        }
    }
  
    return hr;
#endif //RIGHT_SIDE_ONLY    
}


HRESULT CordbProcess::ReadMemory(CORDB_ADDRESS address, 
                                 DWORD size,
                                 BYTE buffer[], 
                                 LPDWORD read)
{
    // A read of 0 bytes is okay.
    if (size == 0)
        return S_OK;

    VALIDATE_POINTER_TO_OBJECT_ARRAY(buffer, BYTE, size, true, true);
    VALIDATE_POINTER_TO_OBJECT(buffer, SIZE_T *);
    
    if (address == NULL)
        return E_INVALIDARG;

    *read = 0;

	INPROC_LOCK();

    HRESULT hr = S_OK;
    HRESULT hrSaved = hr; // this will hold the 'real' hresult in case of a partially completed operation.
    HRESULT hrPartialCopy = HRESULT_FROM_WIN32(ERROR_PARTIAL_COPY);


    CORDBRequireProcessStateOK(this);

    //grab the memory we want to read
    if (ReadProcessMemoryI(m_handle, (LPCVOID)address, buffer, size, read) == 0)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());

        if (hr != hrPartialCopy)
            goto LExit;
        else
            hrSaved = hr;
    }
    
    // There seem to be strange cases where ReadProcessMemory will return a seemingly negative number into *read, which
    // is an unsigned value. So we check the sanity of *read by ensuring that its no bigger than the size we tried to
    // read.
    if ((*read > 0) && (*read <= size))
    {
        LOG((LF_CORDB, LL_INFO100000, "CP::RM: read %d bytes from 0x%08x, first byte is 0x%x\n",
             *read, (DWORD)address, buffer[0]));
        
        if (m_initialized)
        {
            // If m_pPatchTable is NULL, then it's been cleaned out b/c of a Continue for the left side.  Get the table
            // again. Only do this, of course, if the managed state of the process is initialized.
            if (m_pPatchTable == NULL)
            {
                hr = RefreshPatchTable(address, *read, buffer);
            }
            else
            {
                // The previously fetched table is still good, so run through it & see if any patches are applicable
                hr = AdjustBuffer(address, *read, buffer, NULL, AB_READ);
            }
        }
    }

LExit:    
    if (FAILED(hr))
    {
        ClearPatchTable();
    }   
    else if (FAILED(hrSaved))
    {
        hr = hrSaved;
    }

	INPROC_UNLOCK();

    return hr;
}

HRESULT CordbProcess::AdjustBuffer( CORDB_ADDRESS address,
                                    SIZE_T size,
                                    BYTE buffer[],
                                    BYTE **bufferCopy,
                                    AB_MODE mode,
                                    BOOL *pbUpdatePatchTable)
{
    _ASSERTE(m_initialized);
    
    if (    address == NULL
         || size == NULL
         || buffer == NULL
         || (mode != AB_READ && mode != AB_WRITE) )
        return E_INVALIDARG;

    if (pbUpdatePatchTable != NULL )
        *pbUpdatePatchTable = FALSE;

    // If we don't have a patch table loaded, then return S_OK since there are no patches to adjust
    if (m_pPatchTable == NULL)
        return S_OK;

    //is the requested memory completely out-of-range?
    if ((m_minPatchAddr > (address + (size - 1))) ||
        (m_maxPatchAddr < address))
    {
        return S_OK;
    }

    LOG((LF_CORDB,LL_INFO10000, "CordbProcess::AdjustBuffer at addr 0x%x\n", address));

    USHORT iNextFree = m_iFirstPatch;
    while( iNextFree != DPT_TERMINATING_INDEX )
    {
        BYTE *DebuggerControllerPatch = m_pPatchTable +
            m_runtimeOffsets.m_cbPatch*iNextFree;
        DWORD opcode = *(DWORD *)(DebuggerControllerPatch +
                                  m_runtimeOffsets.m_offOpcode);
        BYTE *patchAddress = *(BYTE**)(DebuggerControllerPatch +
                                       m_runtimeOffsets.m_offAddr);

        if ((PTR_TO_CORDB_ADDRESS(patchAddress) >= address) &&
            (PTR_TO_CORDB_ADDRESS(patchAddress) <= (address+(size-1))))
        {
            if (mode == AB_READ )
            {
                CORDbgSetInstruction( buffer+(PTR_TO_CORDB_ADDRESS(patchAddress)
                                              -address), opcode);
            }
            else if (mode == AB_WRITE )
            {
                _ASSERTE( pbUpdatePatchTable != NULL );
                _ASSERTE( bufferCopy != NULL );

                // We don't want to mess up the original copy of the buffer, so
                // for right now, just copy it wholesale.
                (*bufferCopy) = new BYTE[size];
                if (NULL == (*bufferCopy))
                    return E_OUTOFMEMORY;

                memmove((*bufferCopy), buffer, size);
                
                // Copy this back to the copy of the patch table.
                if ( *(INSTRUCTION_TYPE *)((buffer+(PTR_TO_CORDB_ADDRESS(patchAddress)-address))) != CORDbg_BREAK_INSTRUCTION)
                {
                    //There can be multiple patches at the same address:
                    //we don't want 2nd+ patches to get the break opcode
                    
                    m_rgUncommitedOpcode[iNextFree] = 
                        (unsigned int) CORDbgGetInstruction(
                                buffer+(PTR_TO_CORDB_ADDRESS(patchAddress)-address) );
                    //put the breakpoint into the memory itself
                    CORDbgInsertBreakpoint(buffer+(PTR_TO_CORDB_ADDRESS(patchAddress)
                                                   -address));
                }
                else
                { 
                    // One of two situations exists: a prior patch for this address
                    // has been found already (which is why it's patched), or it's
                    // not ours at all.  If the first situation exists, then simply
                    // copy the opcode to here.  Otherwise ignore the breakpoint
                    // since it's not ours & thus we don't care.
                    USHORT iNextSearch = m_iFirstPatch;
                    bool fFound = false;
                    while( iNextSearch != DPT_TERMINATING_INDEX  &&
                           iNextSearch < iNextFree)
                    {
                        BYTE *DCPatchSearch = m_pPatchTable +
                            m_runtimeOffsets.m_cbPatch*iNextSearch;
                        BYTE *patchAddressSearch=*(BYTE**)(DCPatchSearch
                                                + m_runtimeOffsets.m_offAddr);

                        if (patchAddressSearch == patchAddress)
                        {
                            // Copy the previous opcode into the current
                            // patch.
                            m_rgUncommitedOpcode[iNextFree] =
                                m_rgUncommitedOpcode[iNextSearch];
                            fFound = true;
                            break;
                        }

                        iNextSearch = m_rgNextPatch[iNextSearch];
                    }
                    // Must be somebody else's - trash it.
                    if( !fFound )
                    {
                        m_rgUncommitedOpcode[iNextFree] = 
                            (unsigned int) CORDbgGetInstruction(
                                buffer+(PTR_TO_CORDB_ADDRESS(patchAddress)-address) );
                        //put the breakpoint into the memory itself
                        CORDbgInsertBreakpoint(buffer+(PTR_TO_CORDB_ADDRESS(patchAddress)
                                                   -address));
                    }
                }
                    *pbUpdatePatchTable = TRUE;
            }
            else
                _ASSERTE( !"CordbProcess::AdjustBuffergiven non(Read|Write) mode!" );
        }

        iNextFree = m_rgNextPatch[iNextFree];
    }

    return S_OK;
}


void CordbProcess::CommitBufferAdjustments( CORDB_ADDRESS start,
                                            CORDB_ADDRESS end )
{
    _ASSERTE(m_initialized);
    
    USHORT iPatch = m_iFirstPatch;
    while( iPatch != DPT_TERMINATING_INDEX )
    {
        BYTE *DebuggerControllerPatch = m_pPatchTable +
            m_runtimeOffsets.m_cbPatch*iPatch;

        BYTE *patchAddress = *(BYTE**)(DebuggerControllerPatch +
                                       m_runtimeOffsets.m_offAddr);

        if (PTR_TO_CORDB_ADDRESS(patchAddress) >= start &&
            PTR_TO_CORDB_ADDRESS(patchAddress) <= end &&
            m_rgUncommitedOpcode[iPatch] != CORDbg_BREAK_INSTRUCTION)
        {
            //copy this back to the copy of the patch table
            *(unsigned int *)(DebuggerControllerPatch +
                              m_runtimeOffsets.m_offOpcode) =
                m_rgUncommitedOpcode[iPatch];
        }

        iPatch = m_rgNextPatch[iPatch];
    }
}

void CordbProcess::ClearBufferAdjustments( )
{
    USHORT iPatch = m_iFirstPatch;
    while( iPatch != DPT_TERMINATING_INDEX )
    {
        m_rgUncommitedOpcode[iPatch] = CORDbg_BREAK_INSTRUCTION;
        iPatch = m_rgNextPatch[iPatch];
    }
}

void CordbProcess::ClearPatchTable(void )
{
    if (m_pPatchTable != NULL )
    {
        delete [] m_pPatchTable;
        m_pPatchTable = NULL;

        delete [] m_rgNextPatch;
        m_rgNextPatch = NULL;

        delete [] m_rgUncommitedOpcode;
        m_rgUncommitedOpcode = NULL;

        m_iFirstPatch = DPT_TERMINATING_INDEX;
        m_minPatchAddr = MAX_ADDRESS;
        m_maxPatchAddr = MIN_ADDRESS;
        m_rgData = NULL;
        m_cPatch = 0;
    }
}

HRESULT CordbProcess::RefreshPatchTable(CORDB_ADDRESS address, SIZE_T size, BYTE buffer[])
{
    _ASSERTE(m_initialized);
    
    HRESULT hr = S_OK;
    BYTE *rgb = NULL;    
    DWORD dwRead = 0;
    BOOL fOk = false;

    _ASSERTE( m_runtimeOffsets.m_cbOpcode == sizeof(DWORD) );
    
    CORDBRequireProcessStateOK(this);
    
    if (m_pPatchTable == NULL )
    {
        // First, check to be sure the patch table is valid on the Left Side. If its not, then we won't read it.
        BOOL fPatchTableValid = FALSE;

        fOk = ReadProcessMemoryI(m_handle, m_runtimeOffsets.m_pPatchTableValid,
                                 &fPatchTableValid, sizeof(fPatchTableValid), &dwRead);

        if (!fOk || (dwRead != sizeof(fPatchTableValid)) || !fPatchTableValid)
        {
            LOG((LF_CORDB, LL_INFO10000, "Wont refresh patch table because its not valid now.\n"));
            return S_OK;
        }
        
        SIZE_T offStart = 0;
        SIZE_T offEnd = 0;
        UINT cbTableSlice = 0;

        // Grab the patch table info
        offStart = min(m_runtimeOffsets.m_offRgData, m_runtimeOffsets.m_offCData);
        offEnd   = max(m_runtimeOffsets.m_offRgData, m_runtimeOffsets.m_offCData) + sizeof(SIZE_T);
        cbTableSlice = offEnd - offStart;

        if (cbTableSlice == 0)
        {
            LOG((LF_CORDB, LL_INFO10000, "Wont refresh patch table because its not valid now.\n"));
            return S_OK;
        }
        
        rgb = new BYTE[cbTableSlice];
        
        if (rgb == NULL )
        {
            hr = E_OUTOFMEMORY;
            goto LExit;
        }
    
        fOk = ReadProcessMemoryI(m_handle, (BYTE*)m_runtimeOffsets.m_pPatches + offStart,
                                 rgb, cbTableSlice, &dwRead);

        if ( !fOk || (dwRead != cbTableSlice ) )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto LExit;
        }

        // Note that rgData is a pointer in the left side address space
        m_rgData = *(BYTE**)(rgb + m_runtimeOffsets.m_offRgData - offStart);
        m_cPatch = *(USHORT*)(rgb + m_runtimeOffsets.m_offCData - offStart);

        // Grab the patch table
        UINT cbPatchTable = m_cPatch * m_runtimeOffsets.m_cbPatch;

        if (cbPatchTable == 0)
        {
            LOG((LF_CORDB, LL_INFO10000, "Wont refresh patch table because its not valid now.\n"));
            return S_OK;
        }
        
        m_pPatchTable = new BYTE[ cbPatchTable ];
        m_rgNextPatch = new USHORT[m_cPatch];
        m_rgUncommitedOpcode = new DWORD[m_cPatch];
        
        if (m_pPatchTable == NULL || m_rgNextPatch == NULL || m_rgUncommitedOpcode == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto LExit;
        }

        fOk = ReadProcessMemoryI(m_handle, m_rgData, m_pPatchTable, cbPatchTable, &dwRead);

        if ( !fOk || (dwRead != cbPatchTable ) )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto LExit;
        }

        //As we go through the patch table we do three things:
        //
        // 1. collect min,max address seen for quick fail check
        //
        // 2. Link all valid entries into a linked list, the first entry of which is m_iFirstPatch
        //
        // 3. Initialize m_rgUncommitedOpcode, so that we can undo local patch table changes if WriteMemory can't write
        // atomically.
        //
        // 4. If the patch is in the memory we grabbed, unapply it.

        USHORT iDebuggerControllerPatchPrev = DPT_TERMINATING_INDEX;

        m_minPatchAddr = MAX_ADDRESS;
        m_maxPatchAddr = MIN_ADDRESS;
        m_iFirstPatch = DPT_TERMINATING_INDEX;

        for (USHORT iPatch = 0; iPatch < m_cPatch;iPatch++)
        {
            BYTE *DebuggerControllerPatch = m_pPatchTable + m_runtimeOffsets.m_cbPatch * iPatch;
            DWORD opcode = *(DWORD*)(DebuggerControllerPatch + m_runtimeOffsets.m_offOpcode);
            BYTE *patchAddress = *(BYTE**)(DebuggerControllerPatch + m_runtimeOffsets.m_offAddr);
                        
            // A non-zero opcode indicates to us that this patch is valid.
            if (opcode != 0)
            {
                _ASSERTE( patchAddress != 0 );

                // (1), above
                if (m_minPatchAddr > PTR_TO_CORDB_ADDRESS(patchAddress) )
                    m_minPatchAddr = PTR_TO_CORDB_ADDRESS(patchAddress);
                if (m_maxPatchAddr < PTR_TO_CORDB_ADDRESS(patchAddress) )
                    m_maxPatchAddr = PTR_TO_CORDB_ADDRESS(patchAddress);

                // (2), above
                if ( m_iFirstPatch == DPT_TERMINATING_INDEX)
                {
                    m_iFirstPatch = iPatch;
                    _ASSERTE( iPatch != DPT_TERMINATING_INDEX);
                }

                if (iDebuggerControllerPatchPrev != DPT_TERMINATING_INDEX)
                {
                    m_rgNextPatch[iDebuggerControllerPatchPrev] = iPatch;
                }

                iDebuggerControllerPatchPrev = iPatch;

                // (3), above
                m_rgUncommitedOpcode[iPatch] = CORDbg_BREAK_INSTRUCTION;
                
                // (4), above
                if  (address != NULL && 
                    PTR_TO_CORDB_ADDRESS(patchAddress) >= address && PTR_TO_CORDB_ADDRESS(patchAddress) <= address + (size - 1))
                {
                    _ASSERTE( buffer != NULL );
                    _ASSERTE( size != NULL );
                    
                    //unapply the patch here.
                    CORDbgSetInstruction(buffer + (PTR_TO_CORDB_ADDRESS(patchAddress) - address), opcode);
                }
            }
        }
        
        if (iDebuggerControllerPatchPrev != DPT_TERMINATING_INDEX)
            m_rgNextPatch[iDebuggerControllerPatchPrev] = DPT_TERMINATING_INDEX;
    }

 LExit:
    if (rgb != NULL )
        delete [] rgb;

    if (FAILED( hr ) )
        ClearPatchTable();

    return hr;
}

//
// Given an address, see if there is a patch in the patch table that matches it and return if its an unmanaged patch or
// not.
//
// Note: this method is pretty in-efficient. It refreshes the patch table, then scans it. Refreshing the patch table
// involves a scan, too, so this method could be folded with that.
//
HRESULT CordbProcess::FindPatchByAddress(CORDB_ADDRESS address, bool *patchFound, bool *patchIsUnmanaged)
{
    _ASSERTE(patchFound != NULL && patchIsUnmanaged != NULL);
    
    *patchFound = false;
    *patchIsUnmanaged = false;

    // First things first. If the process isn't initialized, then there can be no patch table, so we know the breakpoint
    // doesn't belong to the Runtime.
    if (!m_initialized)
        return S_OK;
    
    // This method is called from the main loop of the win32 event thread in response to a first chance breakpoint event
    // that we know is not a flare. The process has been runnning, and it may have invalidated the patch table, so we'll
    // flush it here before refreshing it to make sure we've got the right thing.
    //
    // Note: we really should have the Left Side mark the patch table dirty to help optimize this.
    ClearPatchTable();

    // Refresh the patch table.
    HRESULT hr = RefreshPatchTable();

    if (FAILED(hr))
    {
        LOG((LF_CORDB, LL_INFO1000, "CP::FPBA: failed to refresh the patch table\n"));
        return hr;
    }

    // If there is no patch table yet, then we know there is no patch at the given address, so return S_OK with
    // *patchFound = false.
    if (m_pPatchTable == NULL)
    {
        LOG((LF_CORDB, LL_INFO1000, "CP::FPBA: no patch table\n"));
        return S_OK;
    }

    // Scan the patch table for a matching patch.
    for (USHORT iNextPatch = m_iFirstPatch; iNextPatch != DPT_TERMINATING_INDEX; iNextPatch = m_rgNextPatch[iNextPatch])
    {
        BYTE *patch = m_pPatchTable + (m_runtimeOffsets.m_cbPatch * iNextPatch);
        BYTE *patchAddress = *(BYTE**)(patch + m_runtimeOffsets.m_offAddr);
        DWORD traceType = *(DWORD*)(patch + m_runtimeOffsets.m_offTraceType);

        if (address == PTR_TO_CORDB_ADDRESS(patchAddress))
        {
            *patchFound = true;

            if (traceType == m_runtimeOffsets.m_traceTypeUnmanaged)
                *patchIsUnmanaged = true;
            
            break;
        }
    }

    // If we didn't find a patch, its actually still possible that this breakpoint exception belongs to us. There are
    // races with very large numbers of threads entering the Runtime through the same managed function. We will have
    // multiple threads adding and removing ref counts to an int 3 in the code stream. Sometimes, this count will go to
    // zero and the int 3 will be removed, then it will come back up and the int 3 will be replaced. The in-process
    // logic takes pains to ensure that such cases are handled properly, therefore we need to perform the same check
    // here to make the correct decision. Basically, the check is to see if there is indeed an int 3 at the exception
    // address. If there is _not_ an int 3 there, then we've hit this race. We will lie and say a managed patch was
    // found to cover this case. This is tracking the logic in DebuggerController::ScanForTriggers, where we call
    // IsPatched.
    if (*patchFound == false)
    {
        // Read one instruction from the faulting address...
        INSTRUCTION_TYPE TrapCheck = 0;
        
        BOOL succ = ReadProcessMemoryI(m_handle, (void*)address, &TrapCheck, sizeof(TrapCheck), NULL);

        if (succ && (TrapCheck != CORDbg_BREAK_INSTRUCTION))
        {
            LOG((LF_CORDB, LL_INFO1000, "CP::FPBA: patchFound=true based on odd missing int 3 case.\n"));
            
            *patchFound = true;
        }
    }
    
    LOG((LF_CORDB, LL_INFO1000, "CP::FPBA: patchFound=%d, patchIsUnmanaged=%d\n", *patchFound, *patchIsUnmanaged));
    
    return S_OK;
}

HRESULT CordbProcess::WriteMemory(CORDB_ADDRESS address, DWORD size,
                                  BYTE buffer[], LPDWORD written)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    CORDBRequireProcessStateOK(this);

    if (size == 0 || address == NULL)
        return E_INVALIDARG;

    VALIDATE_POINTER_TO_OBJECT_ARRAY(buffer, BYTE, size, true, true);
    VALIDATE_POINTER_TO_OBJECT(written, SIZE_T *);
    
    *written = 0;
    
    HRESULT hr = S_OK;
    HRESULT hrSaved = hr; // this will hold the 'real' hresult in case of a 
                          // partially completed operation
    HRESULT hrPartialCopy = HRESULT_FROM_WIN32(ERROR_PARTIAL_COPY);

    SIZE_T dwWritten = 0;
    BOOL bUpdateOriginalPatchTable = FALSE;
    BYTE *bufferCopy = NULL;


    // Only update the patch table if the managed state of the process
    // is initialized.
    if (m_initialized)
    {
        if (m_pPatchTable == NULL )
        {
            if (!SUCCEEDED( hr = RefreshPatchTable() ) )
            {
                goto LExit;
            }
        }

        if ( !SUCCEEDED( hr = AdjustBuffer( address,
                                            size,
                                            buffer,
                                            &bufferCopy,
                                            AB_WRITE,
                                            &bUpdateOriginalPatchTable)))
        {
            goto LExit;
        }
    }

    //conveniently enough, WPM will fail if it can't complete the entire
    //operation
    if ( WriteProcessMemory( m_handle,
                             (LPVOID)address,
                             buffer,
                             size,
                             (SIZE_T*)written) == 0)
    {
        hr = HRESULT_FROM_WIN32(GetLastError() );
        if(hr != hrPartialCopy)
            goto LExit;
        else
            hrSaved = hr;
    }

    LOG((LF_CORDB, LL_INFO100000, "CP::WM: wrote %d bytes at 0x%08x, first byte is 0x%x\n",
         *written, (DWORD)address, buffer[0]));

    if (bUpdateOriginalPatchTable == TRUE )
    {
        //don't tweak patch table for stuff that isn't written to LeftSide
        CommitBufferAdjustments( address, address + *written);
        
        // The only way this should be able to fail is if
        //someone else fiddles with the memory protections on the
        //left side while it's frozen
        WriteProcessMemory( m_handle,
                            (LPVOID)m_rgData,
                            m_pPatchTable,
                            m_cPatch*m_runtimeOffsets.m_cbPatch,
                            &dwWritten);
        _ASSERTE( dwWritten ==m_cPatch*m_runtimeOffsets.m_cbPatch);
    }

    // Since we may have
    // overwritten anything (objects, code, etc), we should mark
    // everything as needing to be re-cached.  
    m_continueCounter++;

 LExit:
    if (m_initialized)
        ClearBufferAdjustments( );

    //we messed up our local copy, so get a clean copy the next time
    //we need it
    if (bUpdateOriginalPatchTable==TRUE)
    {
        if (bufferCopy != NULL)
        {
            memmove(buffer, bufferCopy, size);
            delete bufferCopy;
        }
    }
    
    if (FAILED( hr ))
    {
        //we messed up our local copy, so get a clean copy the next time
        //we need it
        if (bUpdateOriginalPatchTable==TRUE)
        {
            ClearPatchTable();
        }
    }
    else if( FAILED(hrSaved) )
    {
        hr = hrSaved;
    }
    
    return hr;
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbProcess::ClearCurrentException(DWORD threadID)
{
    return E_INVALIDARG;
}

CordbUnmanagedThread *CordbProcess::HandleUnmanagedCreateThread(DWORD dwThreadId, HANDLE hThread, void *lpThreadLocalBase)
{
    CordbUnmanagedThread *ut = new CordbUnmanagedThread(this, dwThreadId, hThread, lpThreadLocalBase);

    if (ut != NULL)
    {
        HRESULT hr = m_unmanagedThreads.AddBase(ut);

        if (!SUCCEEDED(hr))
        {
            delete ut;

            LOG((LF_CORDB, LL_INFO10000, "Failed adding unmanaged thread to process!\n"));
            CORDBSetUnrecoverableError(this, hr, 0);
        }
    }
    else
    {
        LOG((LF_CORDB, LL_INFO10000, "New CordbThread failed!\n"));
        CORDBSetUnrecoverableError(this, E_OUTOFMEMORY, 0);
    }

    return ut;
}

//
// Verify that the version info in the control block matches what we expect. The minimum supported protocol from the
// Left Side must be greater or equal to the minimum required protocol of the Right Side. Note: its the Left Side's job
// to conform to whatever protocol the Right Side requires, so long as minimum is supported.
//
HRESULT CordbProcess::VerifyControlBlock(void)
{
    // Fill in the protocol numbers for the Right Side.
    m_DCB->m_rightSideProtocolCurrent = CorDB_RightSideProtocolCurrent;
    m_DCB->m_rightSideProtocolMinSupported = CorDB_RightSideProtocolMinSupported;

    // For V1 the size of the control block must match exactly.
    if (m_DCB->m_DCBSize != sizeof(DebuggerIPCControlBlock))
        return CORDBG_E_INCOMPATIBLE_PROTOCOL;

    // The Left Side has to support at least our minimum required protocol.
    if (m_DCB->m_leftSideProtocolCurrent < m_DCB->m_rightSideProtocolMinSupported)
        return CORDBG_E_INCOMPATIBLE_PROTOCOL;

    // The Left Side has to be able to emulate at least our minimum required protocol.
    if (m_DCB->m_leftSideProtocolMinSupported > m_DCB->m_rightSideProtocolCurrent)
        return CORDBG_E_INCOMPATIBLE_PROTOCOL;

#ifdef _DEBUG
    char buf[MAX_PATH];
    DWORD len = GetEnvironmentVariableA("CORDBG_NotCompatible", buf, sizeof(buf));
    _ASSERTE(len < sizeof(buf));

    if (len > 0)
        return CORDBG_E_INCOMPATIBLE_PROTOCOL;
#endif    
    
    return S_OK;
}

HRESULT CordbProcess::GetRuntimeOffsets(void)
{
    BOOL succ;
    succ = ReadProcessMemoryI(m_handle,
                             m_DCB->m_runtimeOffsets,
                             &m_runtimeOffsets,
                             sizeof(DebuggerIPCRuntimeOffsets),
                             NULL);

    if (!succ)
        return HRESULT_FROM_WIN32(GetLastError());

    LOG((LF_CORDB, LL_INFO10000, "CP::GRO: got runtime offsets: \n"));

    LOG((LF_CORDB, LL_INFO10000, "    m_TLSIndex=                       0x%08x\n",
         m_runtimeOffsets.m_TLSIndex));
    LOG((LF_CORDB, LL_INFO10000, "    m_EEThreadStateOffset=            0x%08x\n",
         m_runtimeOffsets.m_EEThreadStateOffset));
    LOG((LF_CORDB, LL_INFO10000, "    m_EEThreadStateNCOffset=          0x%08x\n",
         m_runtimeOffsets.m_EEThreadStateNCOffset));
    LOG((LF_CORDB, LL_INFO10000, "    m_EEThreadPGCDisabledOffset=      0x%08x\n",
         m_runtimeOffsets.m_EEThreadPGCDisabledOffset));
    LOG((LF_CORDB, LL_INFO10000, "    m_EEThreadPGCDisabledValue=       0x%08x\n",
         m_runtimeOffsets.m_EEThreadPGCDisabledValue));
    LOG((LF_CORDB, LL_INFO10000, "    m_EEThreadDebuggerWord2Offset=    0x%08x\n",
         m_runtimeOffsets.m_EEThreadDebuggerWord2Offset));
    LOG((LF_CORDB, LL_INFO10000, "    m_EEThreadFrameOffset=            0x%08x\n",
         m_runtimeOffsets.m_EEThreadFrameOffset));
    LOG((LF_CORDB, LL_INFO10000, "    m_EEThreadMaxNeededSize=          0x%08x\n",
         m_runtimeOffsets.m_EEThreadMaxNeededSize));
    LOG((LF_CORDB, LL_INFO10000, "    m_EEThreadSteppingStateMask=      0x%08x\n",
         m_runtimeOffsets.m_EEThreadSteppingStateMask));
    LOG((LF_CORDB, LL_INFO10000, "    m_EEMaxFrameValue=                0x%08x\n",
         m_runtimeOffsets.m_EEMaxFrameValue));
    LOG((LF_CORDB, LL_INFO10000, "    m_EEThreadDebuggerWord1Offset=    0x%08x\n",
         m_runtimeOffsets.m_EEThreadDebuggerWord1Offset));
    LOG((LF_CORDB, LL_INFO10000, "    m_EEThreadCantStopOffset=         0x%08x\n",
         m_runtimeOffsets.m_EEThreadCantStopOffset));
    LOG((LF_CORDB, LL_INFO10000, "    m_EEFrameNextOffset=              0x%08x\n",
         m_runtimeOffsets.m_EEFrameNextOffset));
    LOG((LF_CORDB, LL_INFO10000, "    m_EEBuiltInExceptionCode1=        0x%08x\n",
         m_runtimeOffsets.m_EEBuiltInExceptionCode1));
    LOG((LF_CORDB, LL_INFO10000, "    m_EEBuiltInExceptionCode2=        0x%08x\n",
         m_runtimeOffsets.m_EEBuiltInExceptionCode2));
    LOG((LF_CORDB, LL_INFO10000, "    m_EEIsManagedExceptionStateMask=  0x%08x\n",
         m_runtimeOffsets.m_EEIsManagedExceptionStateMask));
    LOG((LF_CORDB, LL_INFO10000, "    m_pPatches=                       0x%08x\n",
         m_runtimeOffsets.m_pPatches));          
    LOG((LF_CORDB, LL_INFO10000, "    m_offRgData=                      0x%08x\n",
         m_runtimeOffsets.m_offRgData));         
    LOG((LF_CORDB, LL_INFO10000, "    m_offCData=                       0x%08x\n",
         m_runtimeOffsets.m_offCData));          
    LOG((LF_CORDB, LL_INFO10000, "    m_cbPatch=                        0x%08x\n",
         m_runtimeOffsets.m_cbPatch));           
    LOG((LF_CORDB, LL_INFO10000, "    m_offAddr=                        0x%08x\n",
         m_runtimeOffsets.m_offAddr));           
    LOG((LF_CORDB, LL_INFO10000, "    m_offOpcode=                      0x%08x\n",
         m_runtimeOffsets.m_offOpcode));         
    LOG((LF_CORDB, LL_INFO10000, "    m_cbOpcode=                       0x%08x\n",
         m_runtimeOffsets.m_cbOpcode));          
    LOG((LF_CORDB, LL_INFO10000, "    m_offTraceType=                   0x%08x\n",
         m_runtimeOffsets.m_offTraceType));          
    LOG((LF_CORDB, LL_INFO10000, "    m_traceTypeUnmanaged=             0x%08x\n",
         m_runtimeOffsets.m_traceTypeUnmanaged));          

    return S_OK;
}


HRESULT CordbProcess::StartSyncFromWin32Stop(BOOL *asyncBreakSent)
{
    HRESULT hr = S_OK;

    if (asyncBreakSent)
        *asyncBreakSent = FALSE;


    return hr;
}

// Check if the left side has exited. If so, get the right-side
// into shutdown mode. Only use this to avert us from going into
// an unrecoverable error.
bool CordbProcess::CheckIfLSExited()
{    
// Check by waiting on the handle with no timeout.
    if (WaitForSingleObject(m_handle, 0) == WAIT_OBJECT_0)
    {
        Lock();
        m_terminated = true;
        m_exiting = true;
        Unlock();
    }

    LOG((LF_CORDB, LL_INFO10, "CP::IsLSExited() returning '%s'\n", 
        m_exiting ? "true" : "false"));
            
    return m_exiting;
}

// Call this if something really bad happened and we can't do
// anything meaningful with the CordbProcess. 
void CordbProcess::UnrecoverableError(HRESULT errorHR,
                                      unsigned int errorCode,
                                      const char *errorFile,
                                      unsigned int errorLine)
{
    LOG((LF_CORDB, LL_INFO10, "[%x] CP::UE: unrecoverable error 0x%08x "
         "(%d) %s:%d\n",
         GetCurrentThreadId(),
         errorHR, errorCode, errorFile, errorLine));

    if (errorHR != CORDBG_E_INCOMPATIBLE_PROTOCOL)
    {
        _ASSERTE(!"Unrecoverable internal error!");
    
        m_unrecoverableError = true;
    
        //
        // Mark the process as no longer synchronized.
        //
        Lock();
        SetSynchronized(false);
        m_stopCount++;
        Unlock();
    }
    
    // Set the error flags in the process so that if parts of it are
    // still alive, it will realize that its in this mode and do the
    // right thing.
    if (m_DCB != NULL)
    {
        m_DCB->m_errorHR = errorHR;
        m_DCB->m_errorCode = errorCode;
    }

    //
    // Let the user know that we've hit an unrecoverable error.    
    //
    if (m_cordb->m_managedCallback)
        m_cordb->m_managedCallback->DebuggerError((ICorDebugProcess*) this,
                                                  errorHR,
                                                  errorCode);
}


HRESULT CordbProcess::CheckForUnrecoverableError(void)
{
    HRESULT hr = S_OK;
    
    if (m_DCB->m_errorHR != S_OK)
    {
        UnrecoverableError(m_DCB->m_errorHR,
                           m_DCB->m_errorCode,
                           __FILE__, __LINE__);

        hr = m_DCB->m_errorHR;
    }

    return hr;
}


/*
 * EnableLogMessages enables/disables sending of log messages to the 
 * debugger for logging.
 */
HRESULT CordbProcess::EnableLogMessages(BOOL fOnOff)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    HRESULT hr = S_OK;

    DebuggerIPCEvent *event = (DebuggerIPCEvent*) _alloca(CorDBIPC_BUFFER_SIZE);
    InitIPCEvent(event, DB_IPCE_ENABLE_LOG_MESSAGES, false, NULL);
    event->LogSwitchSettingMessage.iLevel = (int)fOnOff;

    hr = m_cordb->SendIPCEvent(this, event, CorDBIPC_BUFFER_SIZE);

    LOG((LF_CORDB, LL_INFO10000, "[%x] CP::EnableLogMessages: EnableLogMessages=%d sent.\n", 
         GetCurrentThreadId(), fOnOff));

    return hr;
#endif //RIGHT_SIDE_ONLY    
}

/*
 * ModifyLogSwitch modifies the specified switch's severity level.
 */
COM_METHOD CordbProcess::ModifyLogSwitch(WCHAR *pLogSwitchName, LONG lLevel)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    HRESULT hr = S_OK;
    
    _ASSERTE (pLogSwitchName != NULL);

    DebuggerIPCEvent *event = (DebuggerIPCEvent*) _alloca(CorDBIPC_BUFFER_SIZE);
    InitIPCEvent(event, DB_IPCE_MODIFY_LOGSWITCH, false, NULL);
    event->LogSwitchSettingMessage.iLevel = lLevel;
    wcscpy (&event->LogSwitchSettingMessage.Dummy[0], pLogSwitchName);
    
    hr = m_cordb->SendIPCEvent(this, event, CorDBIPC_BUFFER_SIZE);

    LOG((LF_CORDB, LL_INFO10000, "[%x] CP::ModifyLogSwitch: ModifyLogSwitch sent.\n", 
         GetCurrentThreadId()));

    return hr;
#endif //RIGHT_SIDE_ONLY    
}


void CordbProcess::ProcessFirstLogMessage (DebuggerIPCEvent* event)
{
    LOG((LF_CORDB, LL_INFO10000, "[%x] RCET::DRCE: FirstLogMessage.\n",
         GetCurrentThreadId()));

    Lock();

    CordbThread* thread =
            (CordbThread*) m_userThreads.GetBase(event->threadId);
    
    if(thread == NULL)
    {
        return; //we haven't seen the thread yet, so don't do anything
    }

    Unlock();

    thread->m_fLogMsgContinued = event->FirstLogMessage.fMoreToFollow;
    thread->m_iLogMsgLevel = event->FirstLogMessage.iLevel;
    thread->m_iLogMsgIndex = 0;
    thread->m_iTotalCatLength = event->FirstLogMessage.iCategoryLength;
    thread->m_iTotalMsgLength = event->FirstLogMessage.iMessageLength;

    // allocate memory for storing the logswitch name and log message
    if (
        ((thread->m_pstrLogSwitch = new WCHAR [thread->m_iTotalCatLength+1])
            != NULL)
        &&
        ((thread->m_pstrLogMsg = new WCHAR [thread->m_iTotalMsgLength+1]) != NULL))
    {
        // copy the LogSwitch name
        wcscpy (thread->m_pstrLogSwitch, &event->FirstLogMessage.Dummy[0]);

        int iBytesToCopy;
        // are there more messages to follow?
        if (thread->m_fLogMsgContinued)
        {
            iBytesToCopy = (CorDBIPC_BUFFER_SIZE - 
                ((int)((char*)&event->FirstLogMessage.Dummy[0] - 
                (char*)event + 
                (char*)LOG_MSG_PADDING) 
                + (thread->m_iTotalCatLength * sizeof (WCHAR)))) / sizeof (WCHAR);

            wcsncpy (thread->m_pstrLogMsg,
                    &event->FirstLogMessage.Dummy[thread->m_iTotalCatLength+1],
                    iBytesToCopy);

            thread->m_iLogMsgIndex = iBytesToCopy;

            Continue(FALSE);
        }
        else
        {
            wcsncpy (thread->m_pstrLogMsg,
                    &event->FirstLogMessage.Dummy[thread->m_iTotalCatLength+1], 
                    thread->m_iTotalMsgLength);

            thread->m_pstrLogMsg [thread->m_iTotalMsgLength] = L'\0';

            // do a callback to the debugger
            if (m_cordb->m_managedCallback)
            {
                // from the thread object get the appdomain object
                CordbAppDomain *pAppDomain = thread->m_pAppDomain;
                _ASSERTE (pAppDomain != NULL);

                m_cordb->m_managedCallback->LogMessage(
                                           (ICorDebugAppDomain*) pAppDomain,
                                           (ICorDebugThread*) thread,
                                           thread->m_iLogMsgLevel,
                                           thread->m_pstrLogSwitch,
                                           thread->m_pstrLogMsg);
            }

            thread->m_iLogMsgIndex = 0;
            thread->m_pstrLogSwitch = NULL;
            thread->m_pstrLogMsg = NULL;

            // free up the memory which was allocated
            delete [] thread->m_pstrLogSwitch;
            delete [] thread->m_pstrLogMsg;

            thread->m_pstrLogSwitch = NULL;
            thread->m_pstrLogMsg = NULL;
        }
    }
    else
    {
        if (thread->m_pstrLogSwitch != NULL)
        {
            delete [] thread->m_pstrLogSwitch;
            thread->m_pstrLogSwitch = NULL;
        }

        // signal that we're out of memory
        CORDBSetUnrecoverableError(this, E_OUTOFMEMORY, 0);
    }

}



void CordbProcess::ProcessContinuedLogMessage (DebuggerIPCEvent* event)
{

    LOG((LF_CORDB, LL_INFO10000, "[%x] RCET::DRCE: ContinuedLogMessage.\n",
         GetCurrentThreadId()));

    Lock();

    CordbThread* thread =
            (CordbThread*) m_userThreads.GetBase(event->threadId);
    
    if(thread == NULL)
    {
        return;
    }

    Unlock();

    // if there was an error while allocating memory when the first log
    // message buffer was received, then no point processing this 
    // message.
    if (thread->m_pstrLogMsg != NULL)
    {
        int iBytesToCopy = event->ContinuedLogMessage.iMessageLength;
        
        _ASSERTE ((iBytesToCopy+thread->m_iLogMsgIndex) <= 
                    thread->m_iTotalMsgLength);

        wcsncpy (&thread->m_pstrLogMsg [thread->m_iLogMsgIndex], 
                &event->ContinuedLogMessage.Dummy[0],
                iBytesToCopy);

        thread->m_iLogMsgIndex += iBytesToCopy;

        if (event->ContinuedLogMessage.fMoreToFollow == false)
        {
            thread->m_pstrLogMsg [thread->m_iTotalMsgLength] = L'\0';

            // make the callback.
            if (m_cordb->m_managedCallback)
            {
                // from the thread object get the appdomain object
                CordbAppDomain *pAppDomain = thread->m_pAppDomain;
                _ASSERTE (pAppDomain != NULL);

                m_cordb->m_managedCallback->LogMessage(
                                           (ICorDebugAppDomain*) pAppDomain,
                                           (ICorDebugThread*) thread,
                                           thread->m_iLogMsgLevel,
                                           thread->m_pstrLogSwitch,
                                           thread->m_pstrLogMsg);
            }


            thread->m_iLogMsgIndex = 0;
            thread->m_pstrLogSwitch = NULL;
            thread->m_pstrLogMsg = NULL;

            // free up the memory which was allocated
            delete [] thread->m_pstrLogSwitch;
            delete [] thread->m_pstrLogMsg;

            thread->m_pstrLogSwitch = NULL;
            thread->m_pstrLogMsg = NULL;
        }
        else
            Continue(FALSE);
    }
    else
        CORDBSetUnrecoverableError(this, E_FAIL, 0);

}


/* 
 * EnumerateAppDomains enumerates all app domains in the process.
 */
HRESULT CordbProcess::EnumerateAppDomains(ICorDebugAppDomainEnum **ppAppDomains)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    VALIDATE_POINTER_TO_OBJECT(ppAppDomains, ICorDebugAppDomainEnum **);
    
    CordbHashTableEnum *e = new CordbHashTableEnum(&m_appDomains, 
                                                   IID_ICorDebugAppDomainEnum);
    if (e == NULL)
        return E_OUTOFMEMORY;

    *ppAppDomains = (ICorDebugAppDomainEnum*)e;
    e->AddRef();
    
#ifndef RIGHT_SIDE_ONLY
    e->m_enumerator.lsAppD.m_proc = this;
#endif

    return S_OK;
#endif
}

/*
 * GetObject returns the runtime process object.
 * Note: This method is not yet implemented.
 */
HRESULT CordbProcess::GetObject(ICorDebugValue **ppObject)
{
    VALIDATE_POINTER_TO_OBJECT(ppObject, ICorDebugObjectValue **);
	INPROC_LOCK();

	INPROC_UNLOCK();
    return E_NOTIMPL;
}


HRESULT inline CordbProcess::Attach(ULONG AppDomainId)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    HRESULT hr = S_OK;

    DebuggerIPCEvent *event = (DebuggerIPCEvent*) _alloca(CorDBIPC_BUFFER_SIZE);
    InitIPCEvent(event, DB_IPCE_ATTACH_TO_APP_DOMAIN, false, NULL);
    event->AppDomainData.id = AppDomainId;
    event->appDomainToken = NULL;

    hr = m_cordb->SendIPCEvent(this, event, CorDBIPC_BUFFER_SIZE);

    LOG((LF_CORDB, LL_INFO100, "[%x] CP::Attach: pProcess=%x sent.\n", 
         GetCurrentThreadId(), this));

    return hr;
#endif //RIGHT_SIDE_ONLY    
}

void CordbProcess::SetSynchronized(bool fSynch)
{
    m_synchronized = fSynch;
}

bool CordbProcess::GetSynchronized(void)
{
    return m_synchronized;
}

/* ------------------------------------------------------------------------- *
 * Runtime Controller Event Thread class
 * ------------------------------------------------------------------------- */

//
// Constructor
//
CordbRCEventThread::CordbRCEventThread(Cordb* cordb)
{
    _ASSERTE(cordb != NULL);
    
    m_cordb = cordb;
    m_thread = NULL;
    m_run = TRUE;
    m_threadControlEvent = NULL;
    m_processStateChanged = FALSE;
}


//
// Destructor. Cleans up all of the open handles and such.
// This expects that the thread has been stopped and has terminated
// before being called.
//
CordbRCEventThread::~CordbRCEventThread()
{
    if (m_threadControlEvent != NULL)
        CloseHandle(m_threadControlEvent);
    
    if (m_thread != NULL)
        CloseHandle(m_thread);
}

#ifndef RIGHT_SIDE_ONLY
// Some of this code is copied in CordbRCEventThead::ThreadProc
HRESULT CordbRCEventThread::VrpcToVrs(CordbProcess *process,
                                      DebuggerIPCEvent* event)
{
    HRESULT hr = S_OK;

    if (!process->m_initialized)
    {
        LOG((LF_CORDB, LL_INFO1000,"RCET::Vrpc: first inproc event\n"));
        
        hr = HandleFirstRCEvent(process);

        if (!SUCCEEDED(hr))
            goto LExit;
    }

    DebuggerIPCEvent *eventCopy;
    eventCopy = NULL;
    eventCopy = (DebuggerIPCEvent*) malloc(CorDBIPC_BUFFER_SIZE);

    if (eventCopy == NULL)
        CORDBSetUnrecoverableError(process, E_OUTOFMEMORY, 0);
    else
    {
        hr = ReadRCEvent(process, eventCopy);

        if (SUCCEEDED(hr))
        {
            process->Lock();
            HandleRCEvent(process, eventCopy);
            process->Unlock();
        }
        else
            free(eventCopy);
    }

LExit:
    return hr;
}
#endif //RIGHT_SIDE_ONLY
//
// Init sets up all the objects that the thread will need to run.
//
HRESULT CordbRCEventThread::Init(void)
{
    if (m_cordb == NULL)
        return E_INVALIDARG;
        
    NAME_EVENT_BUFFER;
    m_threadControlEvent = WszCreateEvent(NULL, FALSE, FALSE, NAME_EVENT(L"ThreadControlEvent"));
    
    if (m_threadControlEvent == NULL)
        return HRESULT_FROM_WIN32(GetLastError());

    return S_OK;
}


//
// HandleFirstRCEvent -- called to handle the very first IPC event from
// the runtime controller. The first event is special because at that point
// we don't know the handles for RSEA and RSER if we launched the process.
//
HRESULT CordbRCEventThread::HandleFirstRCEvent(CordbProcess* process)
{
    HRESULT hr = S_OK;    

#ifdef RIGHT_SIDE_ONLY
    BOOL succ;

    LOG((LF_CORDB, LL_INFO1000, "[%x] RCET::HFRCE: first event...\n", GetCurrentThreadId()));

    if (!(process->m_IPCReader.IsPrivateBlockOpen())) 
    {
        // Open the shared memory segment which contains the control block.

        hr = process->m_IPCReader.OpenPrivateBlockOnPid(process->m_id);

        if (!SUCCEEDED(hr)) 
        {
            goto exit;
        }

        process->m_DCB = process->m_IPCReader.GetDebugBlock();      

        if (process->m_DCB == NULL)
        {
            hr = ERROR_FILE_NOT_FOUND;
            goto exit;
        }
    }

    if ((process->m_DCB != NULL) && (process->m_DCB->m_rightSideProtocolCurrent == 0))
    {
        // Verify that the control block is valid. This is the create case. If we fail validation, then we need to send
        // up a DebuggerError with an hr of CORDBG_E_INCOMPATIBLE_PROTOCOL and kill the process.
        hr = process->VerifyControlBlock();

        if (FAILED(hr))
        {
            _ASSERTE(hr == CORDBG_E_INCOMPATIBLE_PROTOCOL);

            // Send up the DebuggerError event
            process->UnrecoverableError(hr, 0, NULL, 0);

            // Kill the process...
            TerminateProcess(process->m_handle, hr);

            return hr;
        }
    }

    // Dup our own process handle into the remote process.
    succ = DuplicateHandle(GetCurrentProcess(),
                           GetCurrentProcess(),
                           process->m_handle,
                           &(process->m_DCB->m_rightSideProcessHandle),
                           NULL, FALSE, DUPLICATE_SAME_ACCESS);

    if (!succ)
    {
        goto exit;
    }

    HANDLE hTemp;

    // Dup RSEA and RSER into this process.
    hTemp = PAL_RemoteHandleToLocal(process->m_DCB->m_remoteRightSideEventAvailable);
    if (hTemp == INVALID_HANDLE_VALUE) {
	goto exit;
    }
    succ = DuplicateHandle(process->m_handle,
                           hTemp,
                           GetCurrentProcess(),
                           &(process->m_rightSideEventAvailable),
                           NULL, FALSE, DUPLICATE_SAME_ACCESS);
    if (!succ)
    {
        goto exit;
    }

    hTemp = PAL_RemoteHandleToLocal(process->m_DCB->m_remoteRightSideEventRead);
    if (hTemp == INVALID_HANDLE_VALUE) {
	goto exit;
    }
    succ = DuplicateHandle(process->m_handle,
                           hTemp,
                           GetCurrentProcess(),
                           &(process->m_rightSideEventRead),
                           NULL, FALSE, DUPLICATE_SAME_ACCESS);
    if (!succ)
    {
        goto exit;
    }

    hTemp = PAL_RemoteHandleToLocal(process->m_DCB->m_remoteLeftSideUnmanagedWaitEvent);
    if (hTemp == INVALID_HANDLE_VALUE) {
	goto exit;
    }
    succ = DuplicateHandle(process->m_handle,
                           hTemp,
                           GetCurrentProcess(),
                           &(process->m_leftSideUnmanagedWaitEvent),
                           NULL, FALSE, DUPLICATE_SAME_ACCESS);
    if (!succ)
    {
        goto exit;
    }

    hTemp = PAL_RemoteHandleToLocal(process->m_DCB->m_remoteSyncThreadIsLockFree);
    if (hTemp == INVALID_HANDLE_VALUE) {
	goto exit;
    }
    succ = DuplicateHandle(process->m_handle,
                           hTemp,
                           GetCurrentProcess(),
                           &(process->m_syncThreadIsLockFree),
                           NULL, FALSE, DUPLICATE_SAME_ACCESS);
    if (!succ)
    {
        goto exit;
    }

    process->m_DCB->m_remoteRightSideProcessHandle = PAL_LocalHandleToRemote(process->m_DCB->m_rightSideProcessHandle);
    if (process->m_DCB->m_remoteRightSideProcessHandle == INVALID_HANDLE_VALUE) 
    {
        goto exit;
    }


#else //!RIGHT_SIDE_ONLY

    process->m_DCB->m_rightSideProcessHandle = PAL_RemoteHandleToLocal(process->m_DCB->m_remoteRightSideProcessHandle);
    if (process->m_DCB->m_rightSideProcessHandle == INVALID_HANDLE_VALUE) {
        hr = CORDBProcessSetUnrecoverableWin32Error(process, 0);
        return hr;
    }
#endif
    // Read the Runtime Offsets struct out of the debuggee.
    hr = process->GetRuntimeOffsets();

    if (SUCCEEDED(hr))
    {
        process->m_initialized = true;

#ifdef RIGHT_SIDE_ONLY
        process->m_DCB->m_rightSideIsWin32Debugger = 
            (process->m_state & process->PS_WIN32_ATTACHED) ? true : false;
#endif
        
        LOG((LF_CORDB, LL_INFO1000, "[%x] RCET::HFRCE: ...went fine\n", GetCurrentThreadId()));
        
        return hr;
    }
    
#ifdef RIGHT_SIDE_ONLY
exit:
    if (process->m_rightSideEventAvailable != NULL)
    {
        CloseHandle(process->m_rightSideEventAvailable);
        process->m_rightSideEventAvailable = NULL;
    }

    if (process->m_rightSideEventRead != NULL)
    {
        CloseHandle(process->m_rightSideEventRead);
        process->m_rightSideEventRead = NULL;

    }

    if (process->m_leftSideUnmanagedWaitEvent != NULL)
    {
        CloseHandle(process->m_leftSideUnmanagedWaitEvent);
        process->m_leftSideUnmanagedWaitEvent = NULL;
    }

    if (process->m_syncThreadIsLockFree != NULL)
    {
        CloseHandle(process->m_syncThreadIsLockFree);
        process->m_syncThreadIsLockFree = NULL;
    }

    process->m_DCB = NULL;
    if (process->m_IPCReader.IsPrivateBlockOpen())
    {       
        process->m_IPCReader.ClosePrivateBlock();       
    }

    LOG((LF_CORDB, LL_INFO1000, "[%x] RCET::HFRCE: ...failed\n", 
             GetCurrentThreadId()));
#endif

    hr = CORDBProcessSetUnrecoverableWin32Error(process, 0);
    return hr;
}


//
// ReadRCEvent -- read an IPC event sent by the runtime controller from the
// remote process.
//
HRESULT CordbRCEventThread::ReadRCEvent(CordbProcess* process,
                                        DebuggerIPCEvent* event)
{
    _ASSERTE(event != NULL);

    CopyRCEvent((BYTE*)process->m_DCB->m_sendBuffer, (BYTE*)event);
    
    return event->hr;
}

void inline CordbRCEventThread::CopyRCEvent(BYTE *src,
                                            BYTE *dst)
{
    memmove(dst, src, CorDBIPC_BUFFER_SIZE);
}

//
// SendIPCEvent -- send an IPC event to the runtime controller. All this
// really does is copy the event into the process's send buffer and sets
// the RSEA then waits on RSER.
//
// Note: when sending a two-way event (replyRequired = true), the
// eventSize must be large enough for both the event sent and the
// result event.
//
HRESULT CordbRCEventThread::SendIPCEvent(CordbProcess* process,
                                         DebuggerIPCEvent* event,
                                         SIZE_T eventSize)
{
#ifdef RIGHT_SIDE_ONLY
    // The Win32 event thread is pretty much the last thread you'd ever, ever want calling this method.
    _ASSERTE(!m_cordb->m_win32EventThread->IsWin32EventThread());

    CORDBRequireProcessStateOK(process);
#endif

    HRESULT hr = S_OK;
#ifdef RIGHT_SIDE_ONLY
    BOOL succ = TRUE;
#endif

    _ASSERTE(event != NULL);
    
    // NOTE: the eventSize parameter is only so you can specify an event size that is SMALLER than the process send
    // buffer size!!
    if (eventSize > CorDBIPC_BUFFER_SIZE)
        return E_INVALIDARG;
    
    // Sending events to the left side must be synchronized on a per-process basis.
    process->LockSendMutex();

    if (process->m_terminated)
    {
        hr = CORDBG_E_PROCESS_TERMINATED;
        goto exit;
    }

    LOG((LF_CORDB, LL_INFO1000, "CRCET::SIPCE: sending %s to AD 0x%x, proc 0x%x(%d)\n", 
         IPCENames::GetName(event->type), event->appDomainToken, process->m_id, process->m_id));
    
    // Copy the event into the shared memory segment.
    memcpy(process->m_DCB->m_receiveBuffer, event, eventSize);

#ifdef RIGHT_SIDE_ONLY
    LOG((LF_CORDB,LL_INFO1000, "Set RSEA\n"));

    // Tell the runtime controller there is an event ready.
    succ = SetEvent(process->m_rightSideEventAvailable);

    if (succ)
    {
        LOG((LF_CORDB, LL_INFO1000, "CRCET::SIPCE: sent...\n"));
    
        // If this is an async send, then don't wait for the left side to acknowledge that its read the event.
        _ASSERTE(!event->asyncSend || !event->replyRequired);
        
        if (!event->asyncSend)
        {
            DWORD ret;
            
            LOG((LF_CORDB, LL_INFO1000,"CRCET::SIPCE: waiting for left side "
                "to read event. (on RSER)\n"));

            // Wait for the runtime controller to read the event.
            ret = WaitForSingleObject(process->m_rightSideEventRead, CordbGetWaitTimeout());

            if (process->m_terminated)
            {
                hr = CORDBG_E_PROCESS_TERMINATED;
            }
            else if (ret == WAIT_OBJECT_0)
            {
                LOG((LF_CORDB, LL_INFO1000, "CRCET::SIPCE: left side read the event.\n"));
            
                // If this was a two-way event, then the result is already ready for us. Simply copy the result back
                // over the original event that was sent. Otherwise, the left side has simply read the event and is
                // processing it...
                if (event->replyRequired)
                    memcpy(event, process->m_DCB->m_receiveBuffer, eventSize);
            }
            else if (ret == WAIT_TIMEOUT)
            {
                // If we timed out, check the left side to see if it is in the unrecoverable error mode. If it is,
                // return the HR from the left side that caused the error.  Otherwise, return that we timed out and that
                // we don't really know why.
                HRESULT realHR = HRESULT_FROM_WIN32(GetLastError());
        
                hr = process->CheckForUnrecoverableError();

                if (hr == S_OK)
                {
                    CORDBSetUnrecoverableError(process, realHR, 0);
                    hr = realHR;
                }
            }
            else
                hr = CORDBProcessSetUnrecoverableWin32Error(process, 0);
        }
    }
    else
        hr = CORDBProcessSetUnrecoverableWin32Error(process, 0);
#else
    // Make the call directly.
    hr = g_pDebugger->VrpcToVls(event);

    // We won't return until the other side is good and ready, so we don't have to wait.
    if (event->replyRequired)
    {
        memcpy(event, process->m_DCB->m_receiveBuffer, eventSize);
    }
    
#endif //RIGHT_SIDE_ONLY    

exit:
    process->UnlockSendMutex();
    
    return hr;
}


//
// FlushQueuedEvents flushes a process's event queue.
//
void CordbRCEventThread::FlushQueuedEvents(CordbProcess* process)
{
    LOG((LF_CORDB,LL_INFO10000, "CRCET::FQE: Beginning to flush queue\n"));

    // We should only call this is we already have queued events
    _ASSERTE(process->m_queuedEventList != NULL);
    
    //
    // Dispatch queued events so long as they keep calling Continue()
    // before returning from their callback. If they call Continue(),
    // process->m_synchronized will be false again and we know to
    // loop around and dispatch the next event.
    //
#ifdef _DEBUG
    // NOTE: the process lock should be held here...
    _ASSERTE(process->m_processMutexOwner == GetCurrentThreadId());
#endif //_DEBUG

    // If this is the first managed event, go ahead and
    // send the CreateProcess callback now that the
    // process is synchronized... the currently queued
    // events will prevent dispatch of any incoming
    // in-band unmanaged events, thus ensuring there will
    // be no race between sending this special
    // CreateProcess event and dispatching the unmanaged
    // events.
    if (process->m_firstManagedEvent)
    {
        LOG((LF_CORDB,LL_INFO10000, "CRCET::FQE: prep for FirstManagedEvent\n"));
        process->m_firstManagedEvent = false;
            
        process->m_creating = true;
        process->m_dispatchingEvent = true;

        process->m_stopCount++;
        process->Unlock();
                            
        m_cordb->m_managedCallback->CreateProcess((ICorDebugProcess*)process);

        process->Lock();

        // User must call Continue from within the CreateProcess
        // callback...
        _ASSERTE(process->m_creating == false);

        process->m_dispatchingEvent = false;
    }

    // There's a small chance that we asynchronously emptied our queue during 
    // the CreateProcess callback above (if the ls process terminated, ExitProcess
    // will delete all queued events).
    if (process->m_queuedEventList != NULL)
    {      

        // Main dispatch loop here. DispatchRCEvent will take events out of the
        // queue and invoke callbacks
        do
        {
            _ASSERTE(process->m_queuedEventList != NULL);

            process->SetSynchronized(true);
            process->DispatchRCEvent();
                
            LOG((LF_CORDB,LL_INFO10000, "CRCET::FQE: Finished w/ "
                 "DispatchRCEvent\n"));
        }
        while ((process->GetSynchronized() == false) &&
               (process->m_queuedEventList != NULL) &&
               (process->m_unrecoverableError == false));
    }
    
    //
    // If they returned from a callback without calling Continue() then
    // the process is still synchronized, so let the rc event thread
    // know that it need to update its process list and remove the
    // process's event.
    //
    if (process->GetSynchronized())
        ProcessStateChanged();

    // If we were waiting for the managed event queue to drain to
    // dispatch ExitProcess, go ahead and let it happen now.
    if (process->m_exiting)
    {
        LOG((LF_CORDB, LL_INFO1000,
             "CRCET::FQE: managed event queue drained for exit case.\n"));
        
        SetEvent(process->m_miscWaitEvent);
    }

    LOG((LF_CORDB,LL_INFO10000, "CRCET::FQE: finished\n"));
}


//
// HandleRCEvent -- handle an IPC event received from the runtime controller.
// This really just queues events where necessary and performs various
// DI-releated housekeeping for each event received. The events are
// dispatched via DispatchRCEvent.
//
void CordbRCEventThread::HandleRCEvent(CordbProcess* process,
                                       DebuggerIPCEvent* event)
{
    _ASSERTE(process->ThreadHoldsProcessLock());

    switch (event->type & DB_IPCE_TYPE_MASK)
    {
    case DB_IPCE_SYNC_COMPLETE:
        //
        // The RC has synchronized the process. Flush any queued events.
        //
        LOG((LF_CORDB, LL_INFO1000, "[%x] RCET::HRCE: sync complete.\n",
             GetCurrentThreadId()));
        
        free(event);

        process->SetSynchronized(true);
        process->m_syncCompleteReceived = true;

        if (!process->m_stopRequested)
        {
#ifdef RIGHT_SIDE_ONLY
            // Note: we set the m_stopWaitEvent all the time and leave it high while we're stopped. Also note that we
            // can only do this after we've checked process->m_stopRequested!
            SetEvent(process->m_stopWaitEvent);
#endif //RIGHT_SIDE_ONLY            

            _ASSERTE( (process->m_queuedEventList != NULL) ||
                    !"Sync complete received with no queued managed events!");

            FlushQueuedEvents(process);
        }
        else
        {
            LOG((LF_CORDB, LL_INFO1000,
                 "[%x] RCET::HRCE: stop requested, setting event.\n", 
                 GetCurrentThreadId()));
            
#ifdef RIGHT_SIDE_ONLY
            SetEvent(process->m_stopWaitEvent);
#endif //RIGHT_SIDE_ONLY            

            LOG((LF_CORDB, LL_INFO1000, "[%x] RCET::HRCE: set stop event.\n", 
                 GetCurrentThreadId()));
        }
                
        break;

    case DB_IPCE_THREAD_DETACH:
        {
            // Remember that we know the current thread has detached
            // so we won't try to work with it anymore. This prevents
            // people from making mistakes before draining an
            // ExitThread event.
            CordbThread *pThread =
                (CordbThread*)process->m_userThreads.GetBase(event->threadId);

            if (pThread != NULL)
                pThread->m_detached = true;
        }

        // Fall through...
    
    case DB_IPCE_BREAKPOINT:
    case DB_IPCE_USER_BREAKPOINT:
    case DB_IPCE_EXCEPTION:
    case DB_IPCE_STEP_COMPLETE:
    case DB_IPCE_THREAD_ATTACH:
    case DB_IPCE_LOAD_MODULE:
    case DB_IPCE_UNLOAD_MODULE:
    case DB_IPCE_LOAD_CLASS:
    case DB_IPCE_UNLOAD_CLASS:
    case DB_IPCE_FIRST_LOG_MESSAGE:
    case DB_IPCE_CONTINUED_LOG_MESSAGE:
    case DB_IPCE_LOGSWITCH_SET_MESSAGE:
    case DB_IPCE_CREATE_APP_DOMAIN:
    case DB_IPCE_EXIT_APP_DOMAIN:
    case DB_IPCE_LOAD_ASSEMBLY:
    case DB_IPCE_UNLOAD_ASSEMBLY:
    case DB_IPCE_FUNC_EVAL_COMPLETE:
    case DB_IPCE_NAME_CHANGE:
	case DB_IPCE_UPDATE_MODULE_SYMS:
	case DB_IPCE_CONTROL_C_EVENT:
    case DB_IPCE_ENC_REMAP:
    case DB_IPCE_BREAKPOINT_SET_ERROR:
        //
        // Queue all of these events.
        //

        LOG((LF_CORDB, LL_INFO1000, "[%x] RCET::HRCE: Queue event: %s\n", 
             GetCurrentThreadId(), IPCENames::GetName(event->type)));

        event->next = NULL;
        
        if (process->m_queuedEventList == NULL)
            process->m_queuedEventList = event;
        else
            process->m_lastQueuedEvent->next = event;

        process->m_lastQueuedEvent = event;

        break;

    default:
        LOG((LF_CORDB, LL_INFO1000,
             "[%x] RCET::HRCE: Unknown event: 0x%08x\n", 
             GetCurrentThreadId(), event->type));
    }
}

// We shouldn't get these events too much longer - this
// is here for the time between when the debugger says "detach",
// and the time that the left side removes all this AD's
// breakpoints.
//
// Return true if we should ignore the given event b/c we've already
// detatched from the relevant appdomain.  False means we should
// dispatch it.
//
// Please note that we assume that the process has been Lock()'d
// prior to this invocation.
bool CordbProcess::IgnoreRCEvent(DebuggerIPCEvent* event,
                                 CordbAppDomain **ppAppDomain)
{
    LOG((LF_CORDB,LL_INFO1000, "CP::IE: event %s\n", IPCENames::GetName(event->type)));
    _ASSERTE(ThreadHoldsProcessLock());

#ifdef _DEBUG
    _ASSERTE( m_processMutexOwner == GetCurrentThreadId());
#endif //_DEBUG

    CordbAppDomain *cad = NULL;
    
    // Get the appdomain
    switch(event->type)
    {
        case DB_IPCE_ENC_REMAP:
            // We'll arrive here if we've done an EnC on the process,
            // thus don't have a particular AD.  So we should dispatch it.
            cad =(CordbAppDomain*) m_appDomains.GetBase(
                    (ULONG)event->appDomainToken);
            if (cad == NULL)
                return false;
            break;
            
        case DB_IPCE_BREAKPOINT:
        case DB_IPCE_USER_BREAKPOINT:
        case DB_IPCE_EXCEPTION:
        case DB_IPCE_STEP_COMPLETE:
        case DB_IPCE_THREAD_ATTACH:
        case DB_IPCE_LOAD_MODULE:
        case DB_IPCE_UNLOAD_MODULE:
        case DB_IPCE_LOAD_CLASS:
        case DB_IPCE_UNLOAD_CLASS:
        case DB_IPCE_FIRST_LOG_MESSAGE:
        case DB_IPCE_CONTINUED_LOG_MESSAGE:
        case DB_IPCE_LOGSWITCH_SET_MESSAGE:
        case DB_IPCE_FUNC_EVAL_COMPLETE:
        case DB_IPCE_LOAD_ASSEMBLY:
        case DB_IPCE_UNLOAD_ASSEMBLY:
        case DB_IPCE_UPDATE_MODULE_SYMS:
        case DB_IPCE_BREAKPOINT_SET_ERROR:
        
            _ASSERTE(0 != (ULONG)event->appDomainToken);
            cad =(CordbAppDomain*) m_appDomains.GetBase(
                    (ULONG)event->appDomainToken);
            break;
            
            // We should never get this for an appdomain that we've detached from.
        case DB_IPCE_CREATE_APP_DOMAIN:
        case DB_IPCE_EXIT_APP_DOMAIN:
            // Annoyingly enough, we can get name change events before we get the create_app_domain event.  Other code
            // no-ops in that case.
        case DB_IPCE_NAME_CHANGE:
        case DB_IPCE_THREAD_DETACH:
		case DB_IPCE_CONTROL_C_EVENT:
            return false;
            break;
            
        default:
            _ASSERTE( !"We've gotten an unknown event to ignore" );
            break;
    }

    (*ppAppDomain) = cad;
    if (cad == NULL || !cad->m_fAttached)
    {
        LOG((LF_CORDB,LL_INFO1000, "CP::IE: event from appD %S should "
            "be ignored!\n", (cad!=NULL?cad->m_szAppDomainName:L"<None>")));

        // Some events may need some clean-up in order to properly ignore them.            
        switch (event->type)
        {
            case DB_IPCE_STEP_COMPLETE:
            {
                CordbThread* thread =
                    (CordbThread*) m_userThreads.GetBase(event->threadId);
                _ASSERTE(thread != NULL);

                CordbStepper *stepper = (CordbStepper *) 
                    thread->m_process->m_steppers.GetBase((unsigned long)
                    event->StepData.stepperToken);

                if (stepper != NULL)
                {
                    stepper->m_active = false;
                    thread->m_process->m_steppers.RemoveBase(stepper->m_id);
                }
                break;
            }
            
            case DB_IPCE_EXIT_APP_DOMAIN:
            {
                // If there were never any threads in it, remove it 
                // directly rather than waiting for a thread detach that 
                // we'll never get.
                if(cad != NULL &&
                   !cad->m_fHasAtLeastOneThreadInsideIt)
                {
                    LOG((LF_CORDB, LL_INFO100, "CP::IRCE: A.D. is "
                        "thread-less: Release now\n"));
                        
                    cad->m_pProcess->Release();
                    m_appDomains.RemoveBase(cad->m_id);
                }
                else
                {                  
                    LOG((LF_CORDB, LL_INFO100, "CP::IRCE: A.D. had at "
                        "least one thread - defer Release\n"));
                    if (cad != NULL)
                        cad->MarkForDeletion();
                }
                break;
            }

            default:
                break;
        }

        return true;
    }

    LOG((LF_CORDB,LL_INFO1000, "CP::IE: event from appD %S should "
        "be dispatched!\n", cad->m_szAppDomainName));
    return false;
}

//
// ProcessStateChanged -- tell the rc event thread that the ICorDebug's
// process list has changed by setting its flag and thread control event.
// This will cause the rc event thread to update its set of handles to wait
// on.
//
void CordbRCEventThread::ProcessStateChanged(void)
{
    m_cordb->LockProcessList();
    LOG((LF_CORDB, LL_INFO100000, "CRCET::ProcessStateChanged\n"));
    m_processStateChanged = TRUE;
#ifdef RIGHT_SIDE_ONLY
    SetEvent(m_threadControlEvent);
#endif
    m_cordb->UnlockProcessList();
}


//
// Primary loop of the Runtime Controller event thread.
//
// Some of this code is copied in CordbRCEventThead::VrpcToVrs
void CordbRCEventThread::ThreadProc(void)
{
    HANDLE        waitSet[MAXIMUM_WAIT_OBJECTS];
    CordbProcess* processSet[MAXIMUM_WAIT_OBJECTS];
    unsigned int  waitCount;

#ifdef _DEBUG
    memset(&processSet, NULL, MAXIMUM_WAIT_OBJECTS * sizeof(CordbProcess *));
    memset(&waitSet, NULL, MAXIMUM_WAIT_OBJECTS * sizeof(HANDLE));
#endif
    

    // First event to wait on is always the thread control event.
    waitSet[0] = m_threadControlEvent;
    processSet[0] = NULL;
    waitCount = 1;
    
    while (m_run)
    {
        DWORD ret = WaitForMultipleObjects(waitCount, waitSet, FALSE, 2000);
        if (ret == WAIT_FAILED)
        {
            LOG((LF_CORDB, LL_INFO10000, "CordbRCEventThread::ThreadProc WaitFor"
                "MultipleObjects failed: 0x%x\n", GetLastError()));
        }
        else if ((ret != WAIT_TIMEOUT) && m_run)
        {
            // Got an event. Figure out which process it came from.
            unsigned int wn = ret - WAIT_OBJECT_0;

            LOG((LF_CORDB, LL_INFO1000, "RCET::TP: good event on %d\n", wn));
            
            if (wn != 0)
            {
                _ASSERTE(wn < MAXIMUM_WAIT_OBJECTS);
                CordbProcess* process = processSet[wn];
                _ASSERTE(process != NULL);

                HRESULT hr = S_OK;
            
                // Handle the first event from this process differently then all other events. The first event from the
                // process is special because it signals the first time that we know we can send events to the left side
                // if we actually launched this process.
                process->Lock();
                
                // Note: we also include a check of m_firstManagedEvent because m_initialized can go back to being false
                // during shutdown, even though we'll still be receiving some managed events (like module unloads, etc.)
                if (!process->m_firstManagedEvent && !process->m_initialized && CORDBCheckProcessStateOK(process))
                {
                    
                    LOG((LF_CORDB, LL_INFO1000, "RCET::TP: first event, pid 0x%x(%d)\n", wn, process->m_id, process->m_id));
                    
                    // This can fail with the incompatable version HR. The process has already been terminated if this
                    // is the case.
                    hr = HandleFirstRCEvent(process);

                    _ASSERTE(SUCCEEDED(hr) || (hr == CORDBG_E_INCOMPATIBLE_PROTOCOL));

                    if (SUCCEEDED(hr))
                    {
                        // Remember that we're processing the first managed event... this will be used in HandleRCEvent
                        // below once the new event gets queued.
                        process->m_firstManagedEvent = true;
                    }
                }

                if (CORDBCheckProcessStateOK(process) && SUCCEEDED(hr) && !process->m_exiting)
                {
                    LOG((LF_CORDB, LL_INFO1000, "RCET::TP: other event, pid 0x%x(%d)\n", process->m_id, process->m_id));
                    
                    // Got a real IPC event.
                    DebuggerIPCEvent* event;
                
                    event = (DebuggerIPCEvent*) malloc(CorDBIPC_BUFFER_SIZE);

                    if (event == NULL)
                        CORDBSetUnrecoverableError(process, E_OUTOFMEMORY, 0);
                    else
                    {
                        hr = ReadRCEvent(process, event);
                        SetEvent(process->m_leftSideEventRead);

                        if (SUCCEEDED(hr))
                        {
                            HandleRCEvent(process, event);
                        }
                        else
                        {
                            free(event);
                            CORDBSetUnrecoverableError(process, hr, 0);
                        }
                    }
                }

                process->Unlock();
            }
        }

        // Check a flag to see if we need to update our list of processes to wait on.
        if (m_processStateChanged)
        {
            LOG((LF_CORDB, LL_INFO1000, "RCET::TP: refreshing process list.\n"));

            // Pass 1: iterate the hash of all processes and collect the unsynchronized ones into the wait list.
            m_cordb->LockProcessList();
            m_processStateChanged = FALSE;

            unsigned int i;

            // free the old wait list first, though...
            for (i = 1; i < waitCount; i++)
                processSet[i]->Release();
            
            waitCount = 1;

            CordbHashTable* ph = &(m_cordb->m_processes);
            CordbBase* entry;
            HASHFIND find;

            for (entry =  ph->FindFirst(&find); entry != NULL; entry =  ph->FindNext(&find))
            {
                _ASSERTE(waitCount < MAXIMUM_WAIT_OBJECTS);
                
                CordbProcess* p = (CordbProcess*) entry;

                // Only listen to unsynchronized processes. Processes that are synchronized will not send events without
                // being asked by us first, so there is no need to async listen to them.
                //
                // Note: if a process is not synchronized then there is no way for it to transition to the syncrhonized
                // state without this thread receiving an event and taking action. So there is no need to lock the
                // per-process mutex when checking the process's synchronized flag here.
                if (!p->GetSynchronized() && CORDBCheckProcessStateOK(p))
				{
					LOG((LF_CORDB, LL_INFO1000, "RCET::TP: listening to process 0x%x(%d)\n", p->m_id, p->m_id));
                
					waitSet[waitCount] = p->m_leftSideEventAvailable;
					processSet[waitCount] = p;
					processSet[waitCount]->AddRef();

					waitCount++;
				}
            }

            m_cordb->UnlockProcessList();

            // Pass 2: for each process that we placed in the wait list, determine if there are any existing queued
            // events that need to be flushed.

            // Start i at 1 to skip the control event...
            i = 1;
            while(i < waitCount)            
            {
                CordbProcess *p = processSet[i];

                // Take the process lock so we can check the queue safely
                p->Lock();
                _ASSERTE(!p->GetSynchronized() || p->m_exiting);

                // Flush the queue if necessary. Note, we only do this if we've actually received a SyncComplete message
                // from this process. If we haven't received a SyncComplete yet, then we don't attempt to drain any
                // queued events yet. They'll be drained when the SyncComplete event is actually received.
                if ((p->m_syncCompleteReceived == true) &&
                    (p->m_queuedEventList != NULL) &&
                    !p->GetSynchronized())
				{
                    FlushQueuedEvents(p);
				}

                // Flushing could have left the process synchronized...
                if (p->GetSynchronized())
                {
                    // remove the process from the wait list by sucking all the other processes down one.
                    if ((i + 1) < waitCount)
                    {
                        memcpy(&processSet[i], &processSet[i+1], sizeof(processSet[0]) * (waitCount - i - 1));
                        memcpy(&waitSet[i], &waitSet[i+1], sizeof(waitSet[0]) * (waitCount - i - 1));
                    }

                    // drop the count of processes to wait on 
                    waitCount--;
                    
                    p->Unlock();

                    // make sure to release the reference we added when the process was added to the wait list.
                    p->Release();

                    // We don't have to increment i because we've copied the next element into
                    // the current value at i.                    
                }
                else
                {
                    // Even after flushing, its still not syncd, so leave it in the wait list.
                    p->Unlock();

                    // Increment i normally.
                    i++;
                }
            }
        }
    }
}


//
// This is the thread's real thread proc. It simply calls to the
// thread proc on the given object.
//
/*static*/ DWORD WINAPI CordbRCEventThread::ThreadProc(LPVOID parameter)
{
    CordbRCEventThread* t = (CordbRCEventThread*) parameter;
    t->ThreadProc();
    return 0;
}

//
// WaitForIPCEventFromProcess waits for an event from just the specified
// process. This should only be called when the process is in a synchronized
// state, which ensures that the RCEventThread isn't listening to the
// process's event, too, which would get confusing.
//
HRESULT CordbRCEventThread::WaitForIPCEventFromProcess(
                                                   CordbProcess* process,
                                                   CordbAppDomain *pAppDomain,
                                                   DebuggerIPCEvent* event)
{
    CORDBRequireProcessStateOKAndSync(process, pAppDomain);
    
    BOOL ret = WaitForSingleObject(process->m_leftSideEventAvailable,
                                   CordbGetWaitTimeout());

    if (process->m_terminated)
        return CORDBG_E_PROCESS_TERMINATED;
    
    if (ret == WAIT_OBJECT_0)
    {
        HRESULT hr = ReadRCEvent(process, event);
        SetEvent(process->m_leftSideEventRead);

        return hr;
    }
    else if (ret == WAIT_TIMEOUT)
    {
        //
        // If we timed out, check the left side to see if it is in the
        // unrecoverable error mode. If it is, return the HR from the
        // left side that caused the error. Otherwise, return that we timed
        // out and that we don't really know why.
        //
        HRESULT realHR = HRESULT_FROM_WIN32(GetLastError());
        
        HRESULT hr = process->CheckForUnrecoverableError();

        if (hr == S_OK)
        {
            CORDBSetUnrecoverableError(process, realHR, 0);
            return realHR;
        }
        else
            return hr;
    }
    else
        return CORDBProcessSetUnrecoverableWin32Error(process, 0);
}


//
// Start actually creates and starts the thread.
//
HRESULT CordbRCEventThread::Start(void)
{
    if (m_threadControlEvent == NULL)
        return E_INVALIDARG;

    DWORD dummy;
    m_thread = CreateThread(NULL, 0, &CordbRCEventThread::ThreadProc,
                            (LPVOID) this, 0, &dummy);

    if (m_thread == NULL)
        return E_OUTOFMEMORY;

    return S_OK;
}


//
// Stop causes the thread to stop receiving events and exit. It
// waits for it to exit before returning.
//
HRESULT CordbRCEventThread::Stop(void)
{
    if (m_thread != NULL)
    {
        LOG((LF_CORDB, LL_INFO100000, "CRCET::Stop\n"));
        m_run = FALSE;
        SetEvent(m_threadControlEvent);

        DWORD ret = WaitForSingleObject(m_thread, INFINITE);
                
        if (ret != WAIT_OBJECT_0)
            return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;
}


/* ------------------------------------------------------------------------- *
 * Win32 Event Thread class
 * ------------------------------------------------------------------------- */

enum
{
    W32ETA_NONE              = 0,
    W32ETA_CREATE_PROCESS    = 1,
    W32ETA_ATTACH_PROCESS    = 2,
    W32ETA_CONTINUE          = 3,
    W32ETA_DETACH            = 4
};


//
// Constructor
//
CordbWin32EventThread::CordbWin32EventThread(Cordb* cordb) :
    m_cordb(cordb), m_thread(NULL), m_threadControlEvent(NULL),
    m_actionTakenEvent(NULL), m_run(TRUE),
    m_waitTimeout(INFINITE),
    m_waitCount(0), m_action(W32ETA_NONE)
{
    _ASSERTE(cordb != NULL);
}


//
// Destructor. Cleans up all of the open handles and such.
// This expects that the thread has been stopped and has terminated
// before being called.
//
CordbWin32EventThread::~CordbWin32EventThread()
{
    if (m_thread != NULL)
        CloseHandle(m_thread);

    if (m_threadControlEvent != NULL)
        CloseHandle(m_threadControlEvent);

    if (m_actionTakenEvent != NULL)
        CloseHandle(m_actionTakenEvent);

    DeleteCriticalSection(&m_sendToWin32EventThreadMutex);
}


//
// Init sets up all the objects that the thread will need to run.
//
HRESULT CordbWin32EventThread::Init(void)
{
    if (m_cordb == NULL)
        return E_INVALIDARG;
        

    InitializeCriticalSection(&m_sendToWin32EventThreadMutex);
    
    NAME_EVENT_BUFFER;
    m_threadControlEvent = WszCreateEvent(NULL, FALSE, FALSE, NAME_EVENT(L"ThreadControlEvent"));
    if (m_threadControlEvent == NULL)
        return HRESULT_FROM_WIN32(GetLastError());

    m_actionTakenEvent = WszCreateEvent(NULL, FALSE, FALSE, NAME_EVENT(L"ThreadControlEvent"));
    if (m_actionTakenEvent == NULL)
        return HRESULT_FROM_WIN32(GetLastError());

    
    return S_OK;
}


//
// Main function of the Win32 Event Thread
//
void CordbWin32EventThread::ThreadProc(void)
{
    // The first element in the wait set is always the thread control
    // event.
    m_waitSet[0] = m_threadControlEvent;
    m_processSet[0] = NULL;
    m_waitCount = 1;

    // Run the top-level event loop. 
    Win32EventLoop();
}

//
// Primary loop of the Win32 debug event thread.
//
void CordbWin32EventThread::Win32EventLoop(void)
{
    // This must be called from the win32 event thread.
    _ASSERTE(IsWin32EventThread());

    LOG((LF_CORDB, LL_INFO1000, "W32ET::W32EL: entered win32 event loop\n"));


    while (m_run)
    {
        BOOL eventAvailable = FALSE;


        // See if any process that we aren't attached to as the Win32 debugger have exited. (Note: this is a
        // polling action if we are also waiting for Win32 debugger events. We're also looking at the thread
        // control event here, too, to see if we're supposed to do something, like attach.
        DWORD ret = WaitForMultipleObjects(m_waitCount, m_waitSet, FALSE, m_waitTimeout);
        _ASSERTE(ret == WAIT_TIMEOUT || ret < m_waitCount);
        
        if (!m_run)
            break;

        LOG((LF_CORDB, LL_INFO1000, "W32ET::W32EL: WaitForMultipleObjects returned 0x%x\n", ret));

        // If we haven't timed out, or if it wasn't the thread control event that was set, then a process has
        // exited...
        if ((ret != WAIT_TIMEOUT) && (ret != WAIT_OBJECT_0))
        {
            // Grab the process that exited.
            unsigned int wn = ret - WAIT_OBJECT_0;
            _ASSERTE(wn > 0);
            _ASSERTE(wn < NumItems(m_processSet));
            CordbProcess *process = m_processSet[wn];

            ExitProcess(process, wn);
        }

        // Should we create a process?
        else if (m_action == W32ETA_CREATE_PROCESS)
            CreateProcess();

        // Should we attach to a process?
        else if (m_action == W32ETA_ATTACH_PROCESS)
            AttachProcess();

        // Should we detach from a process?
        else if (m_action == W32ETA_DETACH)
            ExitProcess(m_actionData.detachData.pProcess, CW32ET_UNKNOWN_PROCESS_SLOT);
            

        // Sweep over processes to ensure that any FCH threads are still running.
        SweepFCHThreads();
        
        // Only process an event if one is available.
        if (!eventAvailable)
            continue;

    }

    LOG((LF_CORDB, LL_INFO1000, "W32ET::W32EL: exiting event loop\n"));

    return;
}


//
// Checks to see if the given second chance exception event actually signifies the death of the process due to a second
// stack overflow special case.
//
// There are strange cases with stack overflow exceptions. If a dumb application catches a stack overflow exception and
// handles it, without resetting the guard page, then the app will get an AV when it blows the stack a second time. We
// will get the first chance AV, but when we continue from it the OS won't run any SEH handlers, so our FCH won't
// actually work. Instead, we'll get the AV back on second chance right away.
//
bool CordbProcess::IsSpecialStackOverflowCase(CordbUnmanagedThread *pUThread, DEBUG_EVENT *pEvent)
{
    return false;
}

void CordbWin32EventThread::SweepFCHThreads(void)
{
    CordbProcess *processSet[MAXIMUM_WAIT_OBJECTS];
    DWORD processCount = 0;
    CordbBase* entry;
    HASHFIND find;

    // We build a list of the processes that may be awaiting ownership answers while holding the process list
    // lock. Then, we release the lock and work on each process while holding each individual process lock. We have to
    // do this to respect the lock hierarchy, which is per-process lock first, process list lock second.
    m_cordb->LockProcessList();

    for (entry = m_cordb->m_processes.FindFirst(&find); entry != NULL; entry = m_cordb->m_processes.FindNext(&find))
    {
        // We can only have MAXIMUM_WAIT_OBJECTS processes at the maximum. This is enforced elsewhere, but we'll guard
        // against it here, just in case...
        _ASSERTE(processCount < MAXIMUM_WAIT_OBJECTS);

        if (processCount >= MAXIMUM_WAIT_OBJECTS)
            continue;
        
        CordbProcess* p = (CordbProcess*) entry;
        _ASSERTE(p != NULL);

        // Note, we can check p->m_awaitingOwnershipAnswer without taking the process lock because we know that it will
        // only be decremented on another thread. It is only ever incremented on this thread (the Win32 event thread.)
        if (p->m_awaitingOwnershipAnswer > 0)
        {
            processSet[processCount++] = p;
            p->AddRef();
        }
    }

    m_cordb->UnlockProcessList();

    // Now, sweep each individual process...
    for (DWORD i = 0; i < processCount; i++)
    {
        CordbProcess *p = processSet[i];
        
        p->Lock();

        // Re-check p->m_awaitingOwnershipAnswer now that we have the process lock to see if we really need to sweep...
        if (p->m_awaitingOwnershipAnswer > 0)
            p->SweepFCHThreads();
            
        p->Unlock();

        p->Release();
    }
}

void CordbProcess::SweepFCHThreads(void)
{
    _ASSERTE(ThreadHoldsProcessLock());

    // Iterate over all unmanaged threads...
    CordbBase* entry;
    HASHFIND find;

    for (entry = m_unmanagedThreads.FindFirst(&find); entry != NULL; entry =  m_unmanagedThreads.FindNext(&find))
    {
        CordbUnmanagedThread* ut = (CordbUnmanagedThread*) entry;

        // We're only interested in first chance hijacked threads that we're awaiting an ownership answer on. These
        // threads must be allowed to run, and cannot remain suspended.
        if (ut->IsFirstChanceHijacked() && ut->IsAwaitingOwnershipAnswer() && !ut->IsSuspended())
        {
            // Suspend the thread to get its _current_ suspend count.
            DWORD sres = SuspendThread(ut->m_handle);

            if (sres != (DWORD) -1)
            {
                // If we succeeded in suspending the thread, resume it to bring its suspend count back to the proper
                // value. SuspendThread returns the _previous_ suspend count...
                ResumeThread(ut->m_handle);

                // Finally, if the thread was suspended, then resume it until it is not suspended anymore.
                if (sres > 0)
                    while (sres--)
                        ResumeThread(ut->m_handle);
            }
        }
    }
}


//
// This is the thread's real thread proc. It simply calls to the
// thread proc on the given object.
//
/*static*/ DWORD WINAPI CordbWin32EventThread::ThreadProc(LPVOID parameter)
{
    CordbWin32EventThread* t = (CordbWin32EventThread*) parameter;
    t->ThreadProc();
    return 0;
}


//
// Send a CreateProcess event to the Win32 thread to have it create us
// a new process.
//
HRESULT CordbWin32EventThread::SendCreateProcessEvent(
                                  LPCWSTR programName,
                                  LPWSTR  programArgs,
                                  LPSECURITY_ATTRIBUTES lpProcessAttributes,
                                  LPSECURITY_ATTRIBUTES lpThreadAttributes,
                                  BOOL bInheritHandles,
                                  DWORD dwCreationFlags,
                                  PVOID lpEnvironment,
                                  LPCWSTR lpCurrentDirectory,
                                  LPSTARTUPINFOW lpStartupInfo,
                                  LPPROCESS_INFORMATION lpProcessInformation,
                                  CorDebugCreateProcessFlags corDebugFlags)
{
    HRESULT hr = S_OK;
    
    LockSendToWin32EventThreadMutex();
    
    m_actionData.createData.programName = programName;
    m_actionData.createData.programArgs = programArgs;
    m_actionData.createData.lpProcessAttributes = lpProcessAttributes;
    m_actionData.createData.lpThreadAttributes = lpThreadAttributes;
    m_actionData.createData.bInheritHandles = bInheritHandles;
    m_actionData.createData.dwCreationFlags = dwCreationFlags;
    m_actionData.createData.lpEnvironment = lpEnvironment;
    m_actionData.createData.lpCurrentDirectory = lpCurrentDirectory;
    m_actionData.createData.lpStartupInfo = lpStartupInfo;
    m_actionData.createData.lpProcessInformation = lpProcessInformation;
    m_actionData.createData.corDebugFlags = corDebugFlags;

    // m_action is set last so that the win32 event thread can inspect
    // it and take action without actually having to take any
    // locks. The lock around this here is simply to prevent multiple
    // threads from making requests at the same time.
    m_action = W32ETA_CREATE_PROCESS;

    BOOL succ = SetEvent(m_threadControlEvent);

    if (succ)
    {
        DWORD ret = WaitForSingleObject(m_actionTakenEvent, INFINITE);

        if (ret == WAIT_OBJECT_0)
            hr = m_actionResult;
        else
            hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else
        hr = HRESULT_FROM_WIN32(GetLastError());
        
    UnlockSendToWin32EventThreadMutex();

    return hr;
}


//
// Create a new process. This is called in the context of the Win32
// event thread to ensure that if we're Win32 debugging the process
// that the same thread that waits for debugging events will be the
// thread that creates the process.
//
void CordbWin32EventThread::CreateProcess(void)
{
    m_action = W32ETA_NONE;
    HRESULT hr = S_OK;

    // Process information is passed in the action struct
    PROCESS_INFORMATION *pi =
        m_actionData.createData.lpProcessInformation;

    DWORD dwCreationFlags = m_actionData.createData.dwCreationFlags;

    // Ensure that any environment block actually contains CORDBG_ENABLE.
    BYTE *lpEnvironment = (BYTE*) m_actionData.createData.lpEnvironment;

    bool needToFreeEnvBlock = false;


    // We should have already verified that we can have another debuggee
    _ASSERTE(m_cordb->AllowAnotherProcess());
    

	bool fRemoveControlEnvVar = false;
    if (lpEnvironment != NULL)
    {
        {
            needToFreeEnvBlock = EnsureCorDbgEnvVarSet(
                                (CHAR**)&lpEnvironment,
                                (CHAR *) (CorDB_CONTROL_ENV_VAR_NAME "="),
                                false,
                                (DWORD)DBCF_GENERATE_DEBUG_CODE);
        }
    }
    else 
    {
        // If an environment was not passed in, and CorDB_CONTROL_ENV_VAR_NAMEL
        // is not set, set it here.
        WCHAR buf[32];
        DWORD len = WszGetEnvironmentVariable(CorDB_CONTROL_ENV_VAR_NAMEL,
                                              buf, NumItems(buf));
        _ASSERTE(len < sizeof(buf));

        if (len > 0)
            LOG((LF_CORDB, LL_INFO10, "%S already set to %S\n",
                 CorDB_CONTROL_ENV_VAR_NAMEL, buf));
        else
        {
            BOOL succ = WszSetEnvironmentVariable(CorDB_CONTROL_ENV_VAR_NAMEL,
                                                  L"1");

            if (!succ)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto exit;
            }

            LOG((LF_CORDB, LL_INFO10, "Set %S to 1\n",
                 CorDB_CONTROL_ENV_VAR_NAMEL));
            
            fRemoveControlEnvVar = true;
        }

    }
    
    // If the creation flags has DEBUG_PROCESS in them, then we're
    // Win32 debugging this process. Otherwise, we have to create
    // suspended to give us time to setup up our side of the IPC
    // channel.
    BOOL clientWantsSuspend;
    clientWantsSuspend = (dwCreationFlags & CREATE_SUSPENDED);

        dwCreationFlags |= CREATE_SUSPENDED;

    // Have Win32 create the process...
    BOOL ret;
    ret = ::WszCreateProcess(
                      m_actionData.createData.programName,
                      m_actionData.createData.programArgs,
                      m_actionData.createData.lpProcessAttributes,
                      m_actionData.createData.lpThreadAttributes,
                      m_actionData.createData.bInheritHandles,
                      dwCreationFlags,
                      lpEnvironment,
                      m_actionData.createData.lpCurrentDirectory,
                      m_actionData.createData.lpStartupInfo,
                      m_actionData.createData.lpProcessInformation);

    // If we set it earlier, remove it now
    if (fRemoveControlEnvVar)
    {
        BOOL succ = 
            WszSetEnvironmentVariable(CorDB_CONTROL_ENV_VAR_NAMEL, NULL);

        if (!succ)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto exit;
        }
    }


    

    if (ret)
    {
        // Create a process object to represent this process.
        CordbProcess* process = new CordbProcess(m_cordb,
                                                 pi->dwProcessId,
                                                 pi->hProcess);

        if (process != NULL)
        {
            process->AddRef();

            // Initialize the process. This will setup our half of the
            // IPC channel, too.
            hr = process->Init(
                                FALSE
);

            // Shouldn't happen on a create, only an attach
            _ASSERTE(hr != CORDBG_E_DEBUGGER_ALREADY_ATTACHED);

            // Remember the process in the global list of processes.
            if (SUCCEEDED(hr))
                hr = m_cordb->AddProcess(process);

            if (!SUCCEEDED(hr))
                process->Release();
        }
        else
            hr = E_OUTOFMEMORY;

        // If we're Win32 attached to this process, then increment the
        // proper count, otherwise add this process to the wait set
        // and resume the process's main thread.
        if (SUCCEEDED(hr))
        {
            {
                // We're not Win32 attached, so we'll need to wait on
                // this process's handle to see when it exits. Add the
                // process and its handle into the wait set.
                _ASSERTE(m_waitCount >= 0 && m_waitCount < NumItems(m_waitSet));
                
                m_waitSet[m_waitCount] = process->m_handle;
                m_processSet[m_waitCount] = process;
                m_waitCount++;

                // Also, pretend that we've already received the loader breakpoint so that managed events will get
                // dispatched.
                process->m_loaderBPReceived = true;

                
                // Resume the process's main thread now that
                // everything is set up. But only resume if the user
                // didn't specify that they wanted the process created
                // suspended!
                if (!clientWantsSuspend)
                    ResumeThread(pi->hThread);
            }
        }
    }
    else
        hr = HRESULT_FROM_WIN32(GetLastError());

exit:

	// If we created this environment block, then free it.
    if (needToFreeEnvBlock)
        delete [] lpEnvironment;
    
    //
    // Signal the hr to the caller.
    //
    m_actionResult = hr;
    SetEvent(m_actionTakenEvent);
}


//
// Send a DebugActiveProcess event to the Win32 thread to have it attach to
// a new process.
//
HRESULT CordbWin32EventThread::SendDebugActiveProcessEvent(
                                                  DWORD pid, 
                                                  bool fWin32Attach,
                                                  CordbProcess *pProcess)
{
    HRESULT hr = S_OK;

    LockSendToWin32EventThreadMutex();
        
    m_actionData.attachData.processId = pid;
    m_actionData.attachData.pProcess = pProcess;

    // m_action is set last so that the win32 event thread can inspect
    // it and take action without actually having to take any
    // locks. The lock around this here is simply to prevent multiple
    // threads from making requests at the same time.
    m_action = W32ETA_ATTACH_PROCESS;

    BOOL succ = SetEvent(m_threadControlEvent);

    if (succ)
    {
        DWORD ret = WaitForSingleObject(m_actionTakenEvent, INFINITE);

        if (ret == WAIT_OBJECT_0)
            hr = m_actionResult;
        else
            hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else
        hr = HRESULT_FROM_WIN32(GetLastError());
        
    UnlockSendToWin32EventThreadMutex();

    return hr;
}

//
// This function is used to close a handle that we've dup'd into another process. We duplicate the handle back to this
// process, telling DuplicateHandle to close the source handle. This closes the handle in the other process, leaving us
// to close the duplicate here.
//
void CordbProcess::CloseDuplicateHandle(HANDLE *pHandle)
{
    if (*pHandle != NULL)
    {
	*pHandle = NULL;
    }
}

//
// Cleans up the Left Side's DCB after a failed attach attempt.
//
void CordbProcess::CleanupHalfBakedLeftSide(void)
{
    if (m_DCB != NULL)
    {
        CloseDuplicateHandle(&(m_DCB->m_leftSideEventAvailable));
        CloseDuplicateHandle(&(m_DCB->m_leftSideEventRead));
        CloseDuplicateHandle(&(m_DCB->m_rightSideProcessHandle));

        m_DCB->m_rightSideIsWin32Debugger = false;
    }

    // We need to close the setup sync event if we still have it, since a) we shouldn't leak the handle and b) if the
    // debuggee doesn't have a CLR loaded into it, then it shouldn't have a setup sync event created! This was the cause
    // of bug 98348.
    if (m_SetupSyncEvent != NULL)
    {
        CloseHandle(m_SetupSyncEvent);
        m_SetupSyncEvent = NULL;
    }
}

//
// Attach to a process. This is called in the context of the Win32
// event thread to ensure that if we're Win32 debugging the process
// that the same thread that waits for debugging events will be the
// thread that attaches the process.
//
void CordbWin32EventThread::AttachProcess()
{
    CordbProcess* process = NULL;
    m_action = W32ETA_NONE;
    HRESULT hr = S_OK;


    // We should have already verified that we can have another debuggee
    _ASSERTE(m_cordb->AllowAnotherProcess());
    
    // We need to get a handle to the process.
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, m_actionData.attachData.processId);

    LOG((LF_CORDB, LL_INFO10000, "[%x] W32ET::TP: process handle 0x%08x\n", GetCurrentThreadId(), hProcess));
            
    if (hProcess != NULL)
    {
        // Create a process object to represent this process.
        process = new CordbProcess(m_cordb, m_actionData.attachData.processId, hProcess);

        if (process != NULL)
        {
            process->AddRef();
            
            // Initialize the process. This will setup our half of the IPC channel, too.
            hr = process->Init(
                               FALSE
			       );

            // Remember the process in the global list of processes.
            if (SUCCEEDED(hr))
                hr = m_cordb->AddProcess(process);

            if (!SUCCEEDED(hr))
            {
                process->CleanupHalfBakedLeftSide();
                process->Release();
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
            CloseHandle(hProcess);
        }

        // If we're Win32 attaching to this process, then increment
        // the proper count, otherwise add this process to the wait
        // set and resume the process's main thread.
        if (SUCCEEDED(hr))
        {
            {
                // We're not Win32 attached, so we'll need to wait on
                // this process's handle to see when it exits. Add the
                // process and its handle into the wait set.
                _ASSERTE(m_waitCount >= 0 && m_waitCount < NumItems(m_waitSet));
                
                m_waitSet[m_waitCount] = process->m_handle;
                m_processSet[m_waitCount] = process;
                m_waitCount++;

                // Also, pretend that we've already received the loader breakpoint so that managed events will get
                // dispatched.
                process->m_loaderBPReceived = true;
            }
        }
    }
    else
        hr = HRESULT_FROM_WIN32(GetLastError());

    //
    // Signal the hr to the caller.
    //
    m_actionResult = hr;
    SetEvent(m_actionTakenEvent);
}

// Note that the actual 'DetachProcess' method is really ExitProcess with CW32ET_UNKNOWN_PROCESS_SLOT ==
// processSlot
HRESULT CordbWin32EventThread::SendDetachProcessEvent(CordbProcess *pProcess)
{
    LOG((LF_CORDB, LL_INFO1000, "W32ET::SDPE\n"));    
    HRESULT hr = S_OK;
    
    LockSendToWin32EventThreadMutex();
    
    m_actionData.detachData.pProcess = pProcess;

    // m_action is set last so that the win32 event thread can inspect it and take action without actually
    // having to take any locks. The lock around this here is simply to prevent multiple threads from making
    // requests at the same time.
    m_action = W32ETA_DETACH;

    BOOL succ = SetEvent(m_threadControlEvent);

    if (succ)
    {
        DWORD ret = WaitForSingleObject(m_actionTakenEvent, INFINITE);

        if (ret == WAIT_OBJECT_0)
            hr = m_actionResult;
        else
            hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else
        hr = HRESULT_FROM_WIN32(GetLastError());
        
    UnlockSendToWin32EventThreadMutex();

    return hr;
}



//
// ExitProcess is called when a process exits. This does our final cleanup and removes the process from our
// wait sets.
//
void CordbWin32EventThread::ExitProcess(CordbProcess *process, unsigned int processSlot)
{
    LOG((LF_CORDB, LL_INFO1000,"W32ET::EP: begin ExitProcess, processSlot=%d\n", processSlot));
    
    // We're either here because we're detaching (fDetach == TRUE), or because the process has really exited,
    // and we're doing shutdown logic.
    BOOL fDetach = CW32ET_UNKNOWN_PROCESS_SLOT == processSlot;

    // Mark the process teminated. After this, the RCET will never call FlushQueuedEvents. It will
    // ignore all events it receives (including a SyncComplete) and the RCET also does not listen
    // to terminated processes (so ProcessStateChange() won't cause a FQE either).
    process->Terminating(fDetach);
    
    // Take care of the race where the process exits right after the user calls Continue() from the last
    // managed event but before the handler has actually returned.
    //
    // Also, To get through this lock means that either:
    // 1. FlushQueuedEvents is not currently executing and no one will call FQE. 
    // 2. FQE is exiting but is in the middle of a callback (so m_dispatchingEvent = true)
    // 
    process->Lock();

    process->m_exiting = true;
            
    if (fDetach)
        process->SetSynchronized(false);

    // Close off the handle to the setup sync event now, since we know that the pid could be reused at this
    // point (depending on how the exit occured.)
    if (process->m_SetupSyncEvent != NULL)
    {
        LOG((LF_CORDB, LL_INFO1000,"W32ET::EP: Shutting down setupsync event\n"));
        
        CloseHandle(process->m_SetupSyncEvent);
        process->m_SetupSyncEvent = NULL;

        process->m_DCB = NULL;

        if (process->m_IPCReader.IsPrivateBlockOpen())
        {           
            process->m_IPCReader.ClosePrivateBlock();
            LOG((LF_CORDB, LL_INFO1000,"W32ET::EP: Closing private block\n"));
        }

    }
    
    // If we are exiting, we *must* dispatch the ExitProcess callback, but we will delete all the events
    // in the queue and not bother dispatching anything else. If (and only if) we are currently dispatching
    // an event, then we will wait while that event is finished before invoking ExitProcess.
    // (Note that a dispatched event has already been removed from the queue)


    // Delete all queued events while under the lock
    LOG((LF_CORDB, LL_INFO1000, "W32ET::EP: Begin deleting queued events\n"));

    DebuggerIPCEvent* event = process->m_queuedEventList;
    while (event != NULL)
    {        
        LOG((LF_CORDB, LL_INFO1000, "W32ET::EP: Deleting queued event: '%s'\n", IPCENames::GetName(event->type)));

        DebuggerIPCEvent* next = event->next;                
        free(event);
        event = next;
    }    
    process->m_queuedEventList = NULL;
    
    LOG((LF_CORDB, LL_INFO1000, "W32ET::EP: Finished deleting queued events\n"));

        
    // Allow a concurrently executing callback to finish before invoke the ExitProcess callback
    // Note that we must check this flag before unlocking (to avoid a race)
    if (process->m_dispatchingEvent)
    {
        process->Unlock();
        LOG((LF_CORDB, LL_INFO1000, "W32ET::EP: event currently dispatching. Waiting for it to finish\n"));

        // This will be signaled by FlushQueuedEvents() when we return from the currently 
        // dispatched event
#ifdef LOGGING
        DWORD ret =
#endif
        WaitForSingleObject(process->m_miscWaitEvent, INFINITE);

        LOG((LF_CORDB, LL_INFO1000, "W32ET::EP: event finished dispatching, ret = %d\n", ret));
    }
    else 
    {
        LOG((LF_CORDB, LL_INFO1000, "W32ET::EP: No event dispatching. Not waiting\n"));
        process->Unlock();
    }

        

    if (!fDetach)
    {
        LOG((LF_CORDB, LL_INFO1000,"W32ET::EP: ExitProcess callback\n"));

        process->Lock();

        // We're synchronized now, so mark the process as such.
        process->SetSynchronized(true);
        
        process->m_stopCount++;

        process->Unlock();

        // Invoke the ExitProcess callback. This is very important since the a shell
        // may rely on it for proper shutdown and may hang if they don't get it.
        if (m_cordb->m_managedCallback)
            m_cordb->m_managedCallback->ExitProcess((ICorDebugProcess*)process);

        LOG((LF_CORDB, LL_INFO1000,"W32ET::EP: returned from ExitProcess callback\n"));
    }
    
    // Remove the process from the global list of processes.
    m_cordb->RemoveProcess(process);

    // Release the process.
    process->Neuter();
    process->Release();

	// If it's a managed process, somewhere, go find it.
	if (fDetach)
	{
        LOG((LF_CORDB, LL_INFO1000,"W32ET::EP: Detach find proc!\n"));
	    _ASSERTE(CW32ET_UNKNOWN_PROCESS_SLOT == processSlot);

		processSlot = 0;

		for(unsigned int i = 0; i < m_waitCount; i++)
		{
			if (m_processSet[i] == process)
				processSlot = i;
		}
	}

    // Was this process in the non-Win32 attached wait list?
    if (processSlot > 0)
    {
        LOG((LF_CORDB, LL_INFO1000,"W32ET::EP: non Win32\n"));

        // Remove the process from the wait list by sucking all the other processes down one.
        if ((processSlot + 1) < m_waitCount)
        {
            LOG((LF_CORDB, LL_INFO1000,"W32ET::EP: Proc shuffle down!\n"));
        
            memcpy(&m_processSet[processSlot],
                   &m_processSet[processSlot+1],
                   sizeof(m_processSet[0]) * (m_waitCount - processSlot - 1));
            memcpy(&m_waitSet[processSlot], &m_waitSet[processSlot+1],
                   sizeof(m_waitSet[0]) * (m_waitCount - processSlot - 1));
        }

        // Drop the count of non-Win32 attached processes to wait on.
        m_waitCount--;
    }
    else
    {
	_ASSERTE(FALSE);
    }

    if (fDetach)
    {
        // Signal the hr to the caller.
        LOG((LF_CORDB, LL_INFO1000,"W32ET::EP: Detach: send result back!\n"));
        
        m_actionResult = S_OK;
        SetEvent(m_actionTakenEvent);
    }    
}


//
// Start actually creates and starts the thread.
//
HRESULT CordbWin32EventThread::Start(void)
{
    HRESULT hr = S_OK;
    if (m_threadControlEvent == NULL)
        return E_INVALIDARG;

    // Create the thread suspended to make sure that m_threadId is set
    // before CordbWin32EventThread::ThreadProc runs
    m_thread = CreateThread(NULL, 0, &CordbWin32EventThread::ThreadProc,
                            (LPVOID) this, CREATE_SUSPENDED, &m_threadId);

    if (m_thread == NULL)
        return E_OUTOFMEMORY;

    DWORD succ = ResumeThread(m_thread);
    if (succ == (DWORD)-1)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}


//
// Stop causes the thread to stop receiving events and exit. It
// waits for it to exit before returning.
//
HRESULT CordbWin32EventThread::Stop(void)
{
    HRESULT hr = S_OK;
    
    if (m_thread != NULL)
    {
        m_run = FALSE;
        SetEvent(m_threadControlEvent);

        DWORD ret = WaitForSingleObject(m_thread, INFINITE);
                
        if (ret != WAIT_OBJECT_0)
            hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}





/* ------------------------------------------------------------------------- *
 * AppDomain class methods
 * ------------------------------------------------------------------------- */
CordbAppDomain::CordbAppDomain(CordbProcess* pProcess, 
                               REMOTE_PTR pAppDomainToken,
                               ULONG id,
                               WCHAR *szName)

    : CordbBase((ULONG)pAppDomainToken, enumCordbAppDomain),
    m_fAttached(FALSE),
    m_fHasAtLeastOneThreadInsideIt(false),
    m_pProcess(pProcess),
    m_nameIsValid(false),
    m_AppDomainId(id),
    m_assemblies(9),
    m_modules(17),
    m_breakpoints(17),
    m_synchronizedAD(false),
    m_fMarkedForDeletion(FALSE)
{
#ifndef RIGHT_SIDE_ONLY
    m_assemblies.m_guid = IID_ICorDebugAssemblyEnum;
    m_assemblies.m_creator.lsAssem.m_appDomain = this;

    m_modules.m_guid = IID_ICorDebugModuleEnum;
    m_modules.m_creator.lsMod.m_proc = pProcess;
    m_modules.m_creator.lsMod.m_appDomain = this;
#endif //RIGHT_SIDE_ONLY

    // Make a copy of the name. 
    if (szName == NULL)
        szName = L"<UnknownName>";
    else
        m_nameIsValid = true; // We've been passed a good name.

    m_szAppDomainName = new WCHAR[wcslen(szName) + 1];

    if (m_szAppDomainName)
        wcscpy(m_szAppDomainName, szName);

    LOG((LF_CORDB,LL_INFO10000, "CAD::CAD: this:0x%x (void*)this:0x%x"
        "<%S>\n", this, (void *)this, m_szAppDomainName));
    
    InitializeCriticalSection (&m_hCritSect);
}

/*
    A list of which resources owened by this object are accounted for.

    RESOLVED:
        // AddRef() in CordbHashTable::GetBase for a special InProc case
        // AddRef() on the DB_IPCE_CREATE_APP_DOMAIN event from the LS
        // Release()ed in Neuter
        CordbProcess        *m_pProcess; 
        
        WCHAR               *m_szAppDomainName; // Deleted in ~CordbAppDomain
        
        // Cleaned up in Neuter
        CordbHashTable      m_assemblies;
        CordbHashTable      m_modules;
        CordbHashTable      m_breakpoints; // Disconnect()ed in ~CordbAppDomain

    private:
        CRITICAL_SECTION    m_hCritSect; // Deleted in ~CordbAppDomain
*/

CordbAppDomain::~CordbAppDomain()
{
    if (m_szAppDomainName)
        delete [] m_szAppDomainName;

#ifdef RIGHT_SIDE_ONLY
    //
    // Disconnect any active breakpoints
    //
    CordbBase* entry;
    HASHFIND find;

    for (entry =  m_breakpoints.FindFirst(&find);
         entry != NULL;
         entry =  m_breakpoints.FindNext(&find))
    {
        CordbStepper *breakpoint = (CordbStepper*) entry;
        breakpoint->Disconnect();
    }
#endif //RIGHT_SIDE_ONLY

    DeleteCriticalSection(&m_hCritSect);
}

// Neutered by process
void CordbAppDomain::Neuter()
{
    AddRef();
    {
        _ASSERTE(m_pProcess);
        m_pProcess->Release(); 

        NeuterAndClearHashtable(&m_assemblies);
        NeuterAndClearHashtable(&m_modules);
        NeuterAndClearHashtable(&m_breakpoints);

        CordbBase::Neuter();
    }        
    Release();
}


HRESULT CordbAppDomain::QueryInterface(REFIID id, void **ppInterface)
{
    if (id == IID_ICorDebugAppDomain)
        *ppInterface = (ICorDebugAppDomain*)this;
    else if (id == IID_ICorDebugController)
        *ppInterface = (ICorDebugController*)(ICorDebugAppDomain*)this;
    else if (id == IID_IUnknown)
        *ppInterface = (IUnknown*)(ICorDebugAppDomain*)this;
    else
    {
        *ppInterface = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}


HRESULT CordbAppDomain::Stop(DWORD dwTimeout)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    return (m_pProcess->StopInternal(dwTimeout, (void *)m_id));
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbAppDomain::Continue(BOOL fIsOutOfBand)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    return (m_pProcess->ContinueInternal(fIsOutOfBand, NULL)); //(void *)m_id));
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbAppDomain::IsRunning(BOOL *pbRunning)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    VALIDATE_POINTER_TO_OBJECT(pbRunning, BOOL *);

    *pbRunning = !m_pProcess->GetSynchronized();

    return S_OK;
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbAppDomain::HasQueuedCallbacks(ICorDebugThread *pThread, BOOL *pbQueued)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    VALIDATE_POINTER_TO_OBJECT_OR_NULL(pThread,ICorDebugThread *);
    VALIDATE_POINTER_TO_OBJECT(pbQueued,BOOL *);

    return m_pProcess->HasQueuedCallbacks (pThread, pbQueued);
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbAppDomain::EnumerateThreads(ICorDebugThreadEnum **ppThreads)
{
    VALIDATE_POINTER_TO_OBJECT(ppThreads,ICorDebugThreadEnum **);

    HRESULT hr = S_OK;
    CordbEnumFilter *pThreadEnum;

#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessSynchronized(this->m_pProcess, this);
#endif //RIGHT_SIDE_ONLY

    CordbHashTableEnum *e = new CordbHashTableEnum(&m_pProcess->m_userThreads, 
                                                   IID_ICorDebugThreadEnum);
    if (e == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    pThreadEnum = new CordbEnumFilter;
    if (pThreadEnum == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    hr = pThreadEnum->Init (e, this);

    if (SUCCEEDED (hr))
    {
        *ppThreads = (ICorDebugThreadEnum *) pThreadEnum;
        pThreadEnum->AddRef();
    }
    else
        delete pThreadEnum;

Error:
    if (e != NULL)
        delete e;
    return hr;
}


HRESULT CordbAppDomain::SetAllThreadsDebugState(CorDebugThreadState state,
                                   ICorDebugThread *pExceptThisThread)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    return m_pProcess->SetAllThreadsDebugState(state, pExceptThisThread);
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbAppDomain::Detach()
{
    LOG((LF_CORDB, LL_INFO1000, "CAD::Detach - beginning\n"));
#ifndef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOK(m_pProcess);
    
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    CORDBRequireProcessStateOKAndSync(m_pProcess, NULL);
    
    HRESULT hr = S_OK;

    if (m_fAttached)
    {
        // Remember that we're no longer attached to this AD.
        m_fAttached = FALSE;

        // Tell the Left Side that we're no longer attached to this AD.
        DebuggerIPCEvent *event =
            (DebuggerIPCEvent*) _alloca(CorDBIPC_BUFFER_SIZE);
        m_pProcess->InitIPCEvent(event, 
                                 DB_IPCE_DETACH_FROM_APP_DOMAIN, 
                                 false, 
                                 (void *)m_id);
        event->AppDomainData.id = m_AppDomainId;
    
        hr = m_pProcess->m_cordb->SendIPCEvent(this->m_pProcess,
                                               event,
                                               CorDBIPC_BUFFER_SIZE);

        LOG((LF_CORDB, LL_INFO1000, "[%x] CAD::Detach: pProcess=%x sent.\n", 
             GetCurrentThreadId(), this));
    }

    LOG((LF_CORDB, LL_INFO10000, "CP::Detach - returning w/ hr=0x%x\n", hr));
    return hr;
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbAppDomain::Terminate(unsigned int exitCode)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    return E_NOTIMPL;
#endif //RIGHT_SIDE_ONLY    
}


HRESULT CordbAppDomain::CanCommitChanges(
    ULONG cSnapshots, 
    ICorDebugEditAndContinueSnapshot *pSnapshots[], 
    ICorDebugErrorInfoEnum **pError)
{    
    return CORDBG_E_INPROC_NOT_IMPL;
}

HRESULT CordbAppDomain::CommitChanges(
    ULONG cSnapshots, 
    ICorDebugEditAndContinueSnapshot *pSnapshots[], 
    ICorDebugErrorInfoEnum **pError)
{
    return CORDBG_E_INPROC_NOT_IMPL;
}


/*      
 * GetProcess returns the process containing the app domain
 */
HRESULT CordbAppDomain::GetProcess(ICorDebugProcess **ppProcess)
{
    VALIDATE_POINTER_TO_OBJECT(ppProcess,ICorDebugProcess **);
    
    _ASSERTE (m_pProcess != NULL);

    *ppProcess = (ICorDebugProcess *)m_pProcess;
    (*ppProcess)->AddRef();

    return S_OK;
}

/*        
 * EnumerateAssemblies enumerates all assemblies in the app domain
 */
HRESULT CordbAppDomain::EnumerateAssemblies(ICorDebugAssemblyEnum **ppAssemblies)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    VALIDATE_POINTER_TO_OBJECT(ppAssemblies,ICorDebugAssemblyEnum **);

    CordbHashTableEnum *e = new CordbHashTableEnum(&m_assemblies,
                                                   IID_ICorDebugAssemblyEnum);
    if (e == NULL)
        return E_OUTOFMEMORY;

    *ppAssemblies = (ICorDebugAssemblyEnum*)e;
    e->AddRef();

    return S_OK;
#endif
}


HRESULT CordbAppDomain::GetModuleFromMetaDataInterface(
                                                  IUnknown *pIMetaData,
                                                  ICorDebugModule **ppModule)
{
    IMetaDataImport *imi = NULL;

#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOKAndSync(this->m_pProcess, this);
#else 
    // For the Virtual Right Side (In-proc debugging), we'll
    // always be synched, but not neccessarily b/c we've
    // gotten a synch message.
    CORDBRequireProcessStateOK(this->m_pProcess);
#endif    
    VALIDATE_POINTER_TO_OBJECT(pIMetaData, IUnknown *);
    VALIDATE_POINTER_TO_OBJECT(ppModule, ICorDebugModule **);

	INPROC_LOCK();

    HRESULT hr = S_OK;
#ifndef RIGHT_SIDE_ONLY

    //Load the table in-proc.
    CordbHashTableEnum *e = new CordbHashTableEnum(&m_modules, 
                                                   IID_ICorDebugModuleEnum);
    if (e == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    ICorDebugThreadEnum *pThreads;
    hr = EnumerateThreads(&pThreads);
    if (FAILED(hr))
        goto exit;

    _ASSERTE(pThreads != NULL);

    e->m_enumerator.lsMod.m_enumThreads = pThreads;
    e->m_enumerator.lsMod.m_appDomain = this;

    ICorDebugModule *pModule;
    ULONG cElt;
    hr = e->Next(1, &pModule, &cElt);
    
    while(!FAILED(hr) && cElt == 1)
    {
        hr = e->Next(1, &pModule, &cElt);
    }
    
#endif //RIGHT_SIDE_ONLY
    
    // Grab the interface we need...
    hr = pIMetaData->QueryInterface(IID_IMetaDataImport,
                                            (void**)&imi);

    if (FAILED(hr))
    {
        hr =  E_INVALIDARG;
        goto exit;
    }
    
    // Get the mvid of the given module.
    GUID matchMVID;
    hr = imi->GetScopeProps(NULL, 0, 0, &matchMVID);

    if (FAILED(hr))
        goto exit;

    CordbBase* moduleentry;
    HASHFIND findmodule;

    for (moduleentry =  m_modules.FindFirst(&findmodule);
         moduleentry != NULL;
         moduleentry =  m_modules.FindNext(&findmodule))
    {
        CordbModule* m = (CordbModule*) moduleentry;

        // Get the mvid of this module
        GUID MVID;
        hr = m->m_pIMImport->GetScopeProps(NULL, 0, 0, &MVID);

        if (FAILED(hr))
            goto exit;

        if (MVID == matchMVID)
        {
            *ppModule = (ICorDebugModule*)m;
            (*ppModule)->AddRef();

            goto exit;
        }
    }

    hr = E_INVALIDARG;
    
exit:
	INPROC_UNLOCK();
    imi->Release();
    return hr;
}

HRESULT CordbAppDomain::ResolveClassByName(LPWSTR fullClassName,
                                           CordbClass **ppClass)
{
    CordbBase* moduleentry;
    HASHFIND findmodule;

    for (moduleentry =  m_modules.FindFirst(&findmodule);
         moduleentry != NULL;
         moduleentry =  m_modules.FindNext(&findmodule))
    {
        CordbModule* m = (CordbModule*) moduleentry;

        if (SUCCEEDED(m->LookupClassByName(fullClassName, ppClass)))
            return S_OK;
    }

    return E_FAIL;
}

CordbModule *CordbAppDomain::GetAnyModule(void)
{   
    // Get the first module in the assembly
    HASHFIND find;
    CordbModule *module = (CordbModule*) m_modules.FindFirst(&find);

    return module;
}


HRESULT CordbAppDomain::EnumerateBreakpoints(ICorDebugBreakpointEnum **ppBreakpoints)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    VALIDATE_POINTER_TO_OBJECT(ppBreakpoints, ICorDebugBreakpointEnum **);

    CORDBRequireProcessSynchronized(this->m_pProcess, this);

    CordbHashTableEnum *e = new CordbHashTableEnum(&m_breakpoints, 
                                                   IID_ICorDebugBreakpointEnum);
    if (e == NULL)
        return E_OUTOFMEMORY;

    *ppBreakpoints = (ICorDebugBreakpointEnum*)e;
    e->AddRef();

    return S_OK;
#endif //RIGHT_SIDE_ONLY
}

HRESULT CordbAppDomain::EnumerateSteppers(ICorDebugStepperEnum **ppSteppers)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    VALIDATE_POINTER_TO_OBJECT(ppSteppers,ICorDebugStepperEnum **);

    CORDBRequireProcessSynchronized(this->m_pProcess, this);

    //
    // !!! m_steppers may be modified while user is enumerating,
    // if steppers complete (if process is running)
    //

    CordbHashTableEnum *e = new CordbHashTableEnum(&(m_pProcess->m_steppers),
                                                   IID_ICorDebugStepperEnum);
    if (e == NULL)
        return E_OUTOFMEMORY;

    *ppSteppers = (ICorDebugStepperEnum*)e;
    e->AddRef();

    return S_OK;
#endif //RIGHT_SIDE_ONLY    
}


/*
 * IsAttached returns whether or not the debugger is attached to the 
 * app domain.  The controller methods on the app domain cannot be used
 * until the debugger attaches to the app domain.
 */
HRESULT CordbAppDomain::IsAttached(BOOL *pbAttached)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    VALIDATE_POINTER_TO_OBJECT(pbAttached, BOOL *);

    *pbAttached = m_fAttached;

    return S_OK;
#endif //RIGHT_SIDE_ONLY    
}

HRESULT inline CordbAppDomain::Attach (void)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    m_pProcess->Attach(m_AppDomainId);
    m_fAttached = TRUE;

    return S_OK;
#endif //RIGHT_SIDE_ONLY    
}

/*
 * GetName returns the name of the app domain. 
 */
HRESULT CordbAppDomain::GetName(ULONG32 cchName, 
                                ULONG32 *pcchName,
                                WCHAR szName[]) 
{
    VALIDATE_POINTER_TO_OBJECT_ARRAY_OR_NULL(szName, WCHAR, cchName, true, true);
    VALIDATE_POINTER_TO_OBJECT_OR_NULL(pcchName, ULONG32 *);

	INPROC_LOCK();
	
    // send message across to the left side to get the app domain name
    SIZE_T iTrueLen;
    HRESULT hr = S_OK;

    // Some reasonable defaults
    if (szName)
        *szName = 0;

    if (pcchName)
        *pcchName = 0;
    
    // Get the name from the left side
    if (!m_nameIsValid)
    {
        // send message across to the left side to get the app domain name  
        DebuggerIPCEvent event;

        m_pProcess->InitIPCEvent(&event, 
                                 DB_IPCE_GET_APP_DOMAIN_NAME, 
                                 true,
                                 (void*)m_id);
        event.GetAppDomainName.id = m_AppDomainId;

        // Note: two-way event here...
        hr = m_pProcess->m_cordb->SendIPCEvent(m_pProcess, &event,
                                               sizeof(DebuggerIPCEvent));

        // Stop now if we can't even send the event.
        if (!SUCCEEDED(hr))
            goto LExit;

        _ASSERTE(event.type == DB_IPCE_APP_DOMAIN_NAME_RESULT);

        hr = event.hr;

        // delete the old cached name
        if (m_szAppDomainName)
            delete [] m_szAppDomainName;

        // true length of the name, includes null
        iTrueLen = wcslen(event.AppDomainNameResult.rcName) + 1;

        m_szAppDomainName = new WCHAR[iTrueLen];
        
        if (m_szAppDomainName)
            wcscpy(m_szAppDomainName, event.AppDomainNameResult.rcName);
        else
        {
            hr = E_OUTOFMEMORY;
            goto LExit;
		}
		
        m_nameIsValid = true;
    }
    else
        iTrueLen = wcslen(m_szAppDomainName) + 1; // includes null
    
    // If they provided a buffer, fill it in
    if (szName)
    {
        // Figure out the safe string length to copy
        SIZE_T iCopyLen = min(cchName, iTrueLen);

        // Do a safe buffer copy including null if there is room.
        wcsncpy(szName, m_szAppDomainName, iCopyLen);

        // Force a null no matter what, and return the count if desired.
        szName[iCopyLen-1] = 0;
    }

    if (pcchName)
        *pcchName = iTrueLen;
        
LExit:    
	INPROC_UNLOCK();
	
    return hr;
}

/*
 * GetObject returns the runtime app domain object. 
 * Note:   This method is not yet implemented.
 */
HRESULT CordbAppDomain::GetObject(ICorDebugValue **ppObject)
{
    VALIDATE_POINTER_TO_OBJECT(ppObject,ICorDebugObjectValue **);

    return E_NOTIMPL;
}

/*
 * Get the ID of the app domain.
 */
HRESULT CordbAppDomain::GetID (ULONG32 *pId)
{
    VALIDATE_POINTER_TO_OBJECT(pId, ULONG32 *);

    *pId = m_AppDomainId;

    return S_OK;
}

//
// LookupModule finds an existing CordbModule given the address of the
// corresponding DebuggerModule object on the RC-side.
//
CordbModule* CordbAppDomain::LookupModule(REMOTE_PTR debuggerModuleToken)
{
    CordbModule *pModule;

    // first, check to see if the module is present in this app domain
    pModule = (CordbModule*) m_modules.GetBase((ULONG) debuggerModuleToken);

    if (pModule == NULL)
    {
        // it is possible that this module is loaded as part of some other 
        // app domain. This can happen, for eg., when a thread in the debuggee
        // moves to a different app domain. so check all other app domains.


        HASHFIND findappdomain;
        CordbBase *appdomainentry;
        

        for (appdomainentry =  m_pProcess->m_appDomains.FindFirst(&findappdomain);
             appdomainentry != NULL;
             appdomainentry =  m_pProcess->m_appDomains.FindNext(&findappdomain))
        {
            CordbAppDomain* ad = (CordbAppDomain*) appdomainentry;
            if (ad == this)
                continue;

            pModule = (CordbModule*) ad->m_modules.GetBase((ULONG) debuggerModuleToken);

            if (pModule)
                break;
        }
    }

    return pModule;
}


/* ------------------------------------------------------------------------- *
 * Assembly class
 * ------------------------------------------------------------------------- */
CordbAssembly::CordbAssembly(CordbAppDomain* pAppDomain, 
                    REMOTE_PTR debuggerAssemblyToken, 
                    const WCHAR *szName,
                    BOOL fIsSystemAssembly)

    : CordbBase((ULONG)debuggerAssemblyToken, enumCordbAssembly),
      m_pAppDomain(pAppDomain),
      m_fIsSystemAssembly(fIsSystemAssembly)
{
    // Make a copy of the name. 
    if (szName == NULL)
        szName = L"<Unknown>";

    if (szName != NULL)
    {
        m_szAssemblyName = new WCHAR[wcslen(szName) + 1];
        if (m_szAssemblyName)
            wcscpy(m_szAssemblyName, szName);
    }
}

/*
    A list of which resources owned by this object are accounted for.

    public:
        CordbAppDomain      *m_pAppDomain; // Assigned w/o addRef(), Deleted in ~CordbAssembly
*/

CordbAssembly::~CordbAssembly()
{
    delete [] m_szAssemblyName;
}

HRESULT CordbAssembly::QueryInterface(REFIID id, void **ppInterface)
{
    if (id == IID_ICorDebugAssembly)
        *ppInterface = (ICorDebugAssembly*)this;
    else if (id == IID_IUnknown)
        *ppInterface = (IUnknown*)(ICorDebugAssembly*)this;
    else
    {
        *ppInterface = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

// Neutered by AppDomain
void CordbAssembly::Neuter()
{
    AddRef();
    {
        CordbBase::Neuter();
    }
    Release();
}

/*      
 * GetProcess returns the process containing the assembly
 */
HRESULT CordbAssembly::GetProcess(ICorDebugProcess **ppProcess)
{
    VALIDATE_POINTER_TO_OBJECT(ppProcess, ICorDebugProcess **);

    return (m_pAppDomain->GetProcess (ppProcess));
}

/*      
 * GetAppDomain returns the app domain containing the assembly.
 * Returns null if this is the system assembly
 */
HRESULT CordbAssembly::GetAppDomain(ICorDebugAppDomain **ppAppDomain)
{
    VALIDATE_POINTER_TO_OBJECT(ppAppDomain, ICorDebugAppDomain **);

    if (m_fIsSystemAssembly == TRUE)
    {
        *ppAppDomain = NULL;
    }
    else
    {
        _ASSERTE (m_pAppDomain != NULL);

        *ppAppDomain = (ICorDebugAppDomain *)m_pAppDomain;
        (*ppAppDomain)->AddRef();
    }
    return S_OK;
}

/*        
 * EnumerateModules enumerates all modules in the assembly
 */
HRESULT CordbAssembly::EnumerateModules(ICorDebugModuleEnum **ppModules)
{    
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    HRESULT hr = S_OK;
    CordbEnumFilter *pModEnum;

    VALIDATE_POINTER_TO_OBJECT(ppModules, ICorDebugModuleEnum **);

    CORDBRequireProcessSynchronized(m_pAppDomain->m_pProcess, GetAppDomain());

    CordbHashTableEnum *e = new CordbHashTableEnum(&m_pAppDomain->m_modules, 
                                                   IID_ICorDebugModuleEnum);
    if (e == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    pModEnum = new CordbEnumFilter;
    if (pModEnum == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    hr = pModEnum->Init (e, this);

    if (SUCCEEDED (hr))
    {
        *ppModules = (ICorDebugModuleEnum *) pModEnum;
        pModEnum->AddRef();
    }
    else
        delete pModEnum;

Error:
    if (e != NULL)
        delete e;

    return hr;
#endif
}


/*
 * GetCodeBase returns the code base used to load the assembly
 */
HRESULT CordbAssembly::GetCodeBase(ULONG32 cchName, 
                    ULONG32 *pcchName,
                    WCHAR szName[])
{
    VALIDATE_POINTER_TO_OBJECT_ARRAY(szName, WCHAR, cchName, true, true);
    VALIDATE_POINTER_TO_OBJECT_OR_NULL(pcchName, ULONG32 *);
    
    return E_NOTIMPL;
}

/*
 * GetName returns the name of the assembly
 */
HRESULT CordbAssembly::GetName(ULONG32 cchName, 
                               ULONG32 *pcchName,
                               WCHAR szName[])
{
    VALIDATE_POINTER_TO_OBJECT_ARRAY_OR_NULL(szName, WCHAR, cchName, true, true);
    VALIDATE_POINTER_TO_OBJECT_OR_NULL(pcchName, ULONG32 *);

    const WCHAR *szTempName = m_szAssemblyName;

    // In case we didn't get the name (most likely out of memory on ctor).
	if (!szTempName)
    	szTempName = L"<unknown>";

    // True length of the name, including NULL
    SIZE_T iTrueLen = wcslen(szTempName) + 1;

    if (szName)
    {
        // Do a safe buffer copy including null if there is room.
        SIZE_T iCopyLen = min(cchName, iTrueLen);
        wcsncpy(szName, szTempName, iCopyLen);

        // Force a null no matter what, and return the count if desired.
        szName[iCopyLen - 1] = 0;
    }
    
    if (pcchName)
        *pcchName = iTrueLen;

    return S_OK;
}


//****************************************************************************
//************ App Domain Publishing Service API Implementation **************
//****************************************************************************


// ******************************************
// CorpubPublish
// ******************************************

CorpubPublish::CorpubPublish()
    : CordbBase(0),
      m_pProcess(NULL),
      m_pHeadIPCReaderList(NULL)
{
}

CorpubPublish::~CorpubPublish()
{
    // Release all the IPC readers.
    while (m_pHeadIPCReaderList != NULL)
    {
        IPCReaderInterface *pIPCReader = (IPCReaderInterface *)
                                            m_pHeadIPCReaderList->GetData();
        pIPCReader->ClosePrivateBlock();
        delete pIPCReader;

        EnumElement *pTemp = m_pHeadIPCReaderList;
        m_pHeadIPCReaderList = m_pHeadIPCReaderList->GetNext();
        delete pTemp;
    }

    // Release all the processes.
    while (m_pProcess)
    {
        CorpubProcess *pTmp = m_pProcess;
        m_pProcess = m_pProcess->GetNextProcess();
        pTmp->Release();
    }
}

COM_METHOD CorpubPublish::QueryInterface(REFIID id, void **ppInterface)
{
    if (id == IID_ICorPublish)
        *ppInterface = (ICorPublish*)this;
    else if (id == IID_IUnknown)
        *ppInterface = (IUnknown*)(ICorPublish*)this;
    else
    {
        *ppInterface = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;

}


COM_METHOD CorpubPublish::EnumProcesses(COR_PUB_ENUMPROCESS Type,
                                        ICorPublishProcessEnum **ppIEnum)
{
    return E_NOTIMPL;
}


HRESULT CorpubPublish::GetProcess(unsigned pid, 
                                  ICorPublishProcess **ppProcess)
{
    return EnumProcessesInternal(COR_PUB_MANAGEDONLY, 
                                 NULL,
                                 pid,
                                 ppProcess,
                                 TRUE);
}


// This method enumerates all/given process in the system If the flag
// "fOnlyOneProcess" is TRUE, then only the process with the given PID
// is evaluated and returned in ICorPublishProcess.  Otherwise all
// managed processes are evaluated and returned via
// ICorPublishProcessEnum.
HRESULT CorpubPublish::EnumProcessesInternal(
                                    COR_PUB_ENUMPROCESS Type,
                                    ICorPublishProcessEnum **ppIEnum,
                                    unsigned pid, 
                                    ICorPublishProcess **ppProcess,
                                    BOOL fOnlyOneProcess)
{
    HRESULT hr = S_OK;

    if (fOnlyOneProcess == FALSE)
    {
        _ASSERTE(ppIEnum != NULL);
        *ppIEnum = NULL;
    }
    else
    {
        _ASSERTE(ppProcess != NULL);
        *ppProcess = NULL;
    }

    // call function to get PIDs for all processes in the system
#define MAX_PROCESSES  512

    DWORD ProcessId[MAX_PROCESSES];
    DWORD dwNumProcesses = 0;

    IPCReaderInterface  *pIPCReader = NULL;
    EnumElement *pCurIPCReader = NULL;

    BOOL fAllIsWell = FALSE;
    
    if (fOnlyOneProcess == FALSE)
    {
	_ASSERTE(FALSE);
    }
    else
    {
        // first, check to see if a ICorPublishProcess object with the
        // requested process id already exists. This could happen if
        // the user calls EnumerateProcesses() before calling
        // GetProcess()
        CorpubProcess *pProcess = m_pProcess;
        
        while (pProcess != NULL)
        {
            if (pProcess->m_dwProcessId == pid)
            {
                return pProcess->QueryInterface(IID_ICorPublishProcess, 
                                                (void **) ppProcess);
            }

            pProcess = pProcess->GetNextProcess();
        }

        // Didn't find a match, so just pretend we have an array of
        // this one process.
        dwNumProcesses = 1;
        ProcessId[0] = pid;
        fAllIsWell = TRUE;
    }

    if (fAllIsWell)
    {
        CorpubProcess *pCurrent = NULL;

        // iterate over all the processes to fetch all the managed processes 
        for (int i = 0; i < (int)dwNumProcesses; i++)
        {
            if (pIPCReader == NULL)
            {
                pIPCReader = new IPCReaderInterface;

                if (pIPCReader == NULL)
                {
                    LOG((LF_CORDB, LL_INFO100, "CP::EP: Failed to allocate memory for IPCReaderInterface.\n"));
                    hr = E_OUTOFMEMORY;

                    goto exit;
                }
            }
        
            // See if it is a managed process by trying to open the shared
            // memory block.
            hr = pIPCReader->OpenPrivateBlockOnPid(ProcessId[i]);

            if (FAILED(hr))
                continue;

            // Get the AppDomainIPCBlock
            AppDomainEnumerationIPCBlock *pAppDomainCB = pIPCReader->GetAppDomainBlock();

            _ASSERTE (pAppDomainCB != NULL);

            if (pAppDomainCB == NULL)
            {
                LOG((LF_CORDB, LL_INFO1000, "CP::EP: Failed to obtain AppDomainIPCBlock.\n"));

                pIPCReader->ClosePrivateBlock();

                hr = S_FALSE;
                continue;
            }

            // Get the process handle
            HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcessId[i]);

            _ASSERTE (hProcess != NULL);

            if (hProcess == NULL)
            {
                LOG((LF_CORDB, LL_INFO1000, "CP::EP: OpenProcess() returned NULL handle.\n"));

                pIPCReader->ClosePrivateBlock();

                hr = S_FALSE;
                continue;
            }

            // We need to wait for the data block to get filled in
            // properly. But we won't wait forever... two seconds here...
            HANDLE hMutex;

            for (int w = 0; (w < 100) && (pAppDomainCB->m_hMutex == NULL); w++)
                Sleep(20);

            // If the mutex was never filled in, then we most probably
            // lost a shutdown race.
            if (pAppDomainCB->m_hMutex == NULL)
            {
                LOG((LF_CORDB, LL_INFO1000, "CP::EP: IPC block was never properly filled in.\n"));

                pIPCReader->ClosePrivateBlock();

                hr = S_FALSE;
                continue;
            }

            // Dup the valid mutex handle into this process.
	    BOOL succ;
            HANDLE hLocalMutex = PAL_RemoteHandleToLocal(pAppDomainCB->m_hRemoteMutex);
	    if (hLocalMutex == INVALID_HANDLE_VALUE)
	    {
		succ = FALSE;
	    } 
            else 
            {
                succ = DuplicateHandle(hProcess,
                                        hLocalMutex,
                                        GetCurrentProcess(),
                                        &hMutex,
                                        NULL, FALSE, DUPLICATE_SAME_ACCESS);
	    }

            if (succ)
            {
                // Acquire the mutex. Again, only wait two seconds.
                DWORD dwRetVal = WaitForSingleObject(hMutex, 2000);

                if (dwRetVal == WAIT_OBJECT_0)
                {
                    // Make sure the mutex handle is still valid. If
                    // its not, then we lost a shutdown race.
                    if (pAppDomainCB->m_hMutex == NULL)
                    {
                        LOG((LF_CORDB, LL_INFO1000, "CP::EP: lost shutdown race, skipping...\n"));

                        ReleaseMutex(hMutex);
                        CloseHandle(hMutex);
                        succ = FALSE;
                    }
                }
                else
                {
                    // Again, landing here is most probably a shutdown race. Its okay, though...
                    LOG((LF_CORDB, LL_INFO1000, "CP::EP: failed to get IPC mutex.\n"));

                    if (dwRetVal == WAIT_ABANDONED)
                    {
                        ReleaseMutex(hMutex);
                    }
                    CloseHandle(hMutex);
                    succ = FALSE;
                }
            }

            if (!succ)
            {
                pIPCReader->ClosePrivateBlock();

                hr = S_FALSE;
                continue;
            }

            // If we get here, then hMutex is held by this process.

            // Now create the CorpubProcess object for the ProcessID
            pCurrent = new CorpubProcess(ProcessId[i],
                                         true,
                                         hProcess,
                                         hMutex,
                                         pAppDomainCB);

            // Release our lock on the IPC block.
            ReleaseMutex(hMutex);
            
            _ASSERTE (pCurrent != NULL);

            if (pCurrent == NULL)
            {
                hr = E_OUTOFMEMORY;                 
                goto exit;
            }

            // Keep a reference to each process.
            pCurrent->AddRef();

            // Save the IPCReaderInterface pointer
            pCurIPCReader = new EnumElement;
            
            if (pCurIPCReader == NULL)
            {
                hr = E_OUTOFMEMORY;                 
                goto exit;                  
            }

            pCurIPCReader->SetData((void *)pIPCReader);
            pIPCReader = NULL;

            pCurIPCReader->SetNext(m_pHeadIPCReaderList);
            m_pHeadIPCReaderList = pCurIPCReader;

            // Add the process to the list.
            pCurrent->SetNext(m_pProcess);
            m_pProcess = pCurrent;
        }

        if (fOnlyOneProcess == TRUE)
        {
            // If it's NULL here, then we weren't able to attach (target's dead?)
            if(pCurrent == NULL)
            {
                hr = E_FAIL;
                goto exit;
            }
            
            hr = pCurrent->QueryInterface(IID_ICorPublishProcess, 
                                          (void**)ppProcess);
        }
        else
        {
            // create and return the ICorPublishProcessEnum object
            CorpubProcessEnum *pTemp = new CorpubProcessEnum(m_pProcess);

            if (pTemp == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto exit;
            }

            hr = pTemp->QueryInterface(IID_ICorPublishProcessEnum, 
                                       (void**)ppIEnum);
        }
    }
    else
    {
        hr = E_FAIL;
        goto exit;
    }

exit:
    if (FAILED(hr))
    {
        if (fOnlyOneProcess)
            *ppProcess = NULL;
        else
            *ppIEnum = NULL;
    }

    return hr;
}



// ******************************************
// CorpubProcess
// ******************************************

// Constructor
CorpubProcess::CorpubProcess(DWORD dwProcessId,
                             bool fManaged,
                             HANDLE hProcess, 
                             HANDLE hMutex,
                             AppDomainEnumerationIPCBlock *pAD)
    : CordbBase(0, enumCorpubProcess),
      m_dwProcessId(dwProcessId),
      m_fIsManaged(fManaged),
      m_hProcess(hProcess),
      m_hMutex(hMutex),
      m_AppDomainCB(pAD),
      m_pNext(NULL),
      m_pAppDomain(NULL)
{
    // also fetch the process name from the AppDomainIPCBlock
    _ASSERTE (pAD->m_szProcessName != NULL);

    if (pAD->m_szProcessName == NULL)
        m_szProcessName = NULL;
    else
    {
        DWORD dwNumBytesRead;

        _ASSERTE(pAD->m_iProcessNameLengthInBytes > 0);

        // Note: this assumes we're reading the null terminator from
        // the IPC block.
        m_szProcessName = (WCHAR*) new char[pAD->m_iProcessNameLengthInBytes];
        
        if (m_szProcessName == NULL)
        {
            LOG((LF_CORDB, LL_INFO1000,
             "CP::CP: Failed to allocate memory for ProcessName.\n"));

            goto exit;          
        }

        BOOL bSucc = ReadProcessMemoryI(hProcess,
                                        pAD->m_szProcessName,
                                        m_szProcessName,
                                        pAD->m_iProcessNameLengthInBytes,
                                        &dwNumBytesRead);

        _ASSERTE (bSucc != 0);

        if ((bSucc == 0) ||
            (dwNumBytesRead < (DWORD)pAD->m_iProcessNameLengthInBytes))
        {
            LOG((LF_CORDB, LL_INFO1000,
             "CP::EAD: ReadProcessMemoryI (ProcessName) failed.\n"));
        }       
    }

exit:
    ;
}

CorpubProcess::~CorpubProcess()
{
    delete [] m_szProcessName;
    CloseHandle(m_hProcess);
    CloseHandle(m_hMutex);

    // Delete all the app domains in this process
    while (m_pAppDomain)
    {
        CorpubAppDomain *pTemp = m_pAppDomain;
        m_pAppDomain = m_pAppDomain->GetNextAppDomain();
        pTemp->Release();
    }
}



HRESULT CorpubProcess::QueryInterface(REFIID id, void **ppInterface)
{
    if (id == IID_ICorPublishProcess)
        *ppInterface = (ICorPublishProcess*)this;
    else if (id == IID_IUnknown)
        *ppInterface = (IUnknown*)(ICorPublishProcess*)this;
    else
    {
        *ppInterface = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}


HRESULT CorpubProcess::IsManaged(BOOL *pbManaged)
{
    *pbManaged = (m_fIsManaged == true) ? TRUE : FALSE;

    return S_OK;
}

//
// Enumerate the list of known application domains in the target process.
//
HRESULT CorpubProcess::EnumAppDomains(ICorPublishAppDomainEnum **ppIEnum)
{

    int i;

    HRESULT hr = S_OK;

    // Lock the IPC block:
    WaitForSingleObject(m_hMutex, INFINITE);

    int iAppDomainCount = 0;

    _ASSERTE(m_AppDomainCB->m_rgListOfAppDomains != NULL);
    _ASSERTE(m_AppDomainCB->m_iSizeInBytes > 0);

    // Allocate memory to read the remote process' memory into
    AppDomainInfo *pADI = (AppDomainInfo *) 
                                    new char[m_AppDomainCB->m_iSizeInBytes];

    _ASSERTE (pADI != NULL);

    if (pADI == NULL)
    {
        LOG((LF_CORDB, LL_INFO1000,
         "CP::EAD: Failed to allocate memory for AppDomainInfo.\n"));

        hr = E_OUTOFMEMORY;

        goto exit;
    }

    DWORD   dwNumBytesRead;

    // Need to read in the remote process' memory
    if (m_AppDomainCB->m_rgListOfAppDomains != NULL)
    {

        BOOL bSucc = ReadProcessMemoryI(m_hProcess,
                                        m_AppDomainCB->m_rgListOfAppDomains,
                                        pADI,
                                        m_AppDomainCB->m_iSizeInBytes,
                                        &dwNumBytesRead);
        _ASSERTE (bSucc != 0);

        if ((bSucc == 0) ||
            ((int)dwNumBytesRead < m_AppDomainCB->m_iSizeInBytes))
        {
            LOG((LF_CORDB, LL_INFO1000,
             "CP::EAD: ReadProcessMemoryI (AppDomainInfo) failed.\n"));

            hr = E_OUTOFMEMORY;

            goto exit;
        }
    }

    for (i = 0; i < m_AppDomainCB->m_iTotalSlots; i++)
    {
        if (!pADI[i].IsEmpty())
        {

            _ASSERTE(pADI[i].m_iNameLengthInBytes > 0);

            WCHAR *pAppDomainName =
                (WCHAR *) new char[pADI[i].m_iNameLengthInBytes];
            
            if (pAppDomainName == NULL)
            {
                LOG((LF_CORDB, LL_INFO1000,
                 "CP::EAD: Failed to allocate memory for AppDomainName.\n"));

                hr = E_OUTOFMEMORY;
                goto exit;
            }

            BOOL bSucc = ReadProcessMemoryI(m_hProcess,
                                            pADI[i].m_szAppDomainName,
                                            pAppDomainName,
                                            pADI[i].m_iNameLengthInBytes,
                                            &dwNumBytesRead);
            _ASSERTE (bSucc != 0);

            if ((bSucc == 0) ||
                (dwNumBytesRead < (DWORD)pADI[i].m_iNameLengthInBytes))
            {
                LOG((LF_CORDB, LL_INFO1000,
                 "CP::EAD: ReadProcessMemoryI (AppDomainName) failed.\n"));

                hr = E_FAIL;
                goto exit;
            }

            // create a new AppDomainObject
            CorpubAppDomain *pCurrent = new CorpubAppDomain(pAppDomainName, 
                                                            pADI[i].m_id);
            
            if (pCurrent == NULL)
            {
                LOG((LF_CORDB, LL_INFO1000,
                 "CP::EAD: Failed to allocate memory for CorpubAppDomain.\n"));

                hr = E_OUTOFMEMORY;
                goto exit;
            }

            // Keep a reference to each app domain.
            pCurrent->AddRef();

            // Add the appdomain to the list.
            pCurrent->SetNext(m_pAppDomain);
            m_pAppDomain = pCurrent;

            if (++iAppDomainCount >= m_AppDomainCB->m_iNumOfUsedSlots)
                break;
        }
    }

    {
        _ASSERTE ((iAppDomainCount >= m_AppDomainCB->m_iNumOfUsedSlots)
                  && (i <= m_AppDomainCB->m_iTotalSlots));

        // create and return the ICorPublishAppDomainEnum object
        CorpubAppDomainEnum *pTemp = new CorpubAppDomainEnum(m_pAppDomain);

        if (pTemp == NULL)
        {
            hr = E_OUTOFMEMORY;
            *ppIEnum = NULL;
            goto exit;
        }

        hr = pTemp->QueryInterface(IID_ICorPublishAppDomainEnum,
                                   (void **)ppIEnum);
    }

exit:
    ReleaseMutex(m_hMutex);

    return hr;
}

/*
 * Returns the OS ID for the process in question.
 */
HRESULT CorpubProcess::GetProcessID(unsigned *pid)
{
    *pid = m_dwProcessId;

    return S_OK;
}

/*
 * Get the display name for a process.
 */
HRESULT CorpubProcess::GetDisplayName(ULONG32 cchName, 
                                      ULONG32 *pcchName,
                                      WCHAR szName[])
{
    VALIDATE_POINTER_TO_OBJECT_ARRAY_OR_NULL(szName, WCHAR, cchName, true, true);
    VALIDATE_POINTER_TO_OBJECT_OR_NULL(pcchName, ULONG32 *);

    // Reasonable defaults
    if (szName)
        *szName = 0;

    if (pcchName)
        *pcchName = 0;
    
    SIZE_T      iCopyLen, iTrueLen;
    const WCHAR *szTempName = m_szProcessName;

    // In case we didn't get the name (most likely out of memory on ctor).
    if (!szTempName)
        szTempName = L"<unknown>";

    iTrueLen = wcslen(szTempName) + 1;  // includes null

    if (szName)
    {
        // Do a safe buffer copy including null if there is room.
        iCopyLen = min(cchName, iTrueLen);
        wcsncpy(szName, szTempName, iCopyLen);

        // Force a null no matter what, and return the count if desired.
        szName[iCopyLen - 1] = 0;
    }
    
    if (pcchName)
        *pcchName = iTrueLen;

    return S_OK;
}


// ******************************************
// CorpubAppDomain
// ******************************************

CorpubAppDomain::CorpubAppDomain (WCHAR *szAppDomainName, ULONG Id)
    : CordbBase (0, enumCorpubAppDomain),
    m_pNext (NULL),
    m_szAppDomainName (szAppDomainName),
    m_id (Id)
{
}

CorpubAppDomain::~CorpubAppDomain()
{
    delete [] m_szAppDomainName;
}

HRESULT CorpubAppDomain::QueryInterface (REFIID id, void **ppInterface)
{
    if (id == IID_ICorPublishAppDomain)
        *ppInterface = (ICorPublishAppDomain*)this;
    else if (id == IID_IUnknown)
        *ppInterface = (IUnknown*)(ICorPublishAppDomain*)this;
    else
    {
        *ppInterface = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}


/*
 * Get the name and ID for an application domain.
 */
HRESULT CorpubAppDomain::GetID (ULONG32 *pId)
{
    VALIDATE_POINTER_TO_OBJECT(pId, ULONG32 *);

    *pId = m_id;

    return S_OK;
}

/*
 * Get the name for an application domain.
 */
HRESULT CorpubAppDomain::GetName(ULONG32 cchName, 
                                ULONG32 *pcchName, 
                                WCHAR szName[])
{
    VALIDATE_POINTER_TO_OBJECT_ARRAY_OR_NULL(szName, WCHAR, cchName, true, true);
    VALIDATE_POINTER_TO_OBJECT_OR_NULL(pcchName, ULONG32 *);
    
    const WCHAR *szTempName = m_szAppDomainName;

    // In case we didn't get the name (most likely out of memory on ctor).
    if (!szTempName)
        szTempName = L"<unknown>";

    SIZE_T iTrueLen = wcslen(szTempName) + 1;  // includes null

    if (szName)
    {
        // Do a safe buffer copy including null if there is room.
        SIZE_T iCopyLen = min(cchName, iTrueLen);
        wcsncpy(szName, szTempName, iCopyLen);

        // Force a null no matter what, and return the count if desired.
        szName[iCopyLen - 1] = 0;
    }
    
    if (pcchName)
        *pcchName = iTrueLen;

    return S_OK;
}



// ******************************************
// CorpubProcessEnum
// ******************************************

CorpubProcessEnum::CorpubProcessEnum (CorpubProcess *pFirst)
    : CordbBase (0, enumCorpubProcessEnum),
    m_pFirst (pFirst),
    m_pCurrent (pFirst)
{   
}


HRESULT CorpubProcessEnum::QueryInterface (REFIID id, void **ppInterface)
{
    if (id == IID_ICorPublishProcessEnum)
        *ppInterface = (ICorPublishProcessEnum*)this;
    else if (id == IID_IUnknown)
        *ppInterface = (IUnknown*)(ICorPublishProcessEnum*)this;
    else
    {
        *ppInterface = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}


HRESULT CorpubProcessEnum::Skip(ULONG celt)
{
	INPROC_LOCK();
	
	while ((m_pCurrent != NULL) && (celt-- > 0))
    {
        m_pCurrent = m_pCurrent->GetNextProcess();
    }

	INPROC_UNLOCK();
    return S_OK;
}

HRESULT CorpubProcessEnum::Reset()
{
    m_pCurrent = m_pFirst;

    return S_OK;
}

HRESULT CorpubProcessEnum::Clone(ICorPublishEnum **ppEnum)
{
    VALIDATE_POINTER_TO_OBJECT(ppEnum, ICorPublishEnum **);
    INPROC_LOCK();

    INPROC_UNLOCK();
    return E_NOTIMPL;
}

HRESULT CorpubProcessEnum::GetCount(ULONG *pcelt)
{
    VALIDATE_POINTER_TO_OBJECT(pcelt, ULONG *);

	INPROC_LOCK();
    
    CorpubProcess *pTemp = m_pFirst;

    *pcelt = 0;

    while (pTemp != NULL) 
    {
        (*pcelt)++;
        pTemp = pTemp->GetNextProcess();
    }

	INPROC_UNLOCK();

    return S_OK;
}

HRESULT CorpubProcessEnum::Next(ULONG celt,
                ICorPublishProcess *objects[],
                ULONG *pceltFetched)
{
    VALIDATE_POINTER_TO_OBJECT_ARRAY(objects, ICorPublishProcess *, 
        celt, true, true);
    VALIDATE_POINTER_TO_OBJECT(pceltFetched, ULONG *);

	INPROC_LOCK();
    
    HRESULT hr = S_OK;

    *pceltFetched = 0;

    while ((m_pCurrent != NULL) && (*pceltFetched < celt))
    {
        hr = m_pCurrent->QueryInterface (IID_ICorPublishProcess,
                                        (void**)&objects [*pceltFetched]);

        if (hr != S_OK)
            break;

        (*pceltFetched)++;
        m_pCurrent = m_pCurrent->GetNextProcess();
    }

	INPROC_UNLOCK();

    return hr;
}

// ******************************************
// CorpubAppDomainEnum
// ******************************************
CorpubAppDomainEnum::CorpubAppDomainEnum (CorpubAppDomain *pFirst)
    : CordbBase (0, enumCorpubAppDomainEnum),
    m_pFirst (pFirst),
    m_pCurrent (pFirst)
{   
}


HRESULT CorpubAppDomainEnum::QueryInterface (REFIID id, void **ppInterface)
{
    if (id == IID_ICorPublishAppDomainEnum)
        *ppInterface = (ICorPublishAppDomainEnum*)this;
    else if (id == IID_IUnknown)
        *ppInterface = (IUnknown*)(ICorPublishAppDomainEnum*)this;
    else
    {
        *ppInterface = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}


HRESULT CorpubAppDomainEnum::Skip(ULONG celt)
{
	INPROC_LOCK();

	while ((m_pCurrent != NULL) && (celt-- > 0))
    {
        m_pCurrent = m_pCurrent->GetNextAppDomain();
    }

	INPROC_UNLOCK();

    return S_OK;
}

HRESULT CorpubAppDomainEnum::Reset()
{
	m_pCurrent = m_pFirst;

    return S_OK;
}

HRESULT CorpubAppDomainEnum::Clone(ICorPublishEnum **ppEnum)
{
    VALIDATE_POINTER_TO_OBJECT(ppEnum, ICorPublishEnum **);
	INPROC_LOCK();

	INPROC_UNLOCK();
    return E_NOTIMPL;
}

HRESULT CorpubAppDomainEnum::GetCount(ULONG *pcelt)
{
    VALIDATE_POINTER_TO_OBJECT(pcelt, ULONG *);

	INPROC_LOCK();
    
    CorpubAppDomain *pTemp = m_pFirst;

    *pcelt = 0;

    while (pTemp != NULL) 
    {
        (*pcelt)++;
        pTemp = pTemp->GetNextAppDomain();
    }

	INPROC_UNLOCK();

    return S_OK;
}

HRESULT CorpubAppDomainEnum::Next(ULONG celt,
                ICorPublishAppDomain *objects[],
                ULONG *pceltFetched)
{
    VALIDATE_POINTER_TO_OBJECT_ARRAY(objects, ICorPublishProcess *, 
        celt, true, true);
    VALIDATE_POINTER_TO_OBJECT(pceltFetched, ULONG *);
    
    HRESULT hr = S_OK;

    *pceltFetched = 0;

    while ((m_pCurrent != NULL) && (*pceltFetched < celt))
    {
        hr = m_pCurrent->QueryInterface (IID_ICorPublishAppDomain,
                                        (void **)&objects [*pceltFetched]);

        if (hr != S_OK)
            break;

        (*pceltFetched)++;
        m_pCurrent = m_pCurrent->GetNextAppDomain();
    }

    return hr;
}



//***********************************************************************
//              ICorDebugTMEnum (Thread and Module enumerator)
//***********************************************************************
CordbEnumFilter::CordbEnumFilter()
    : CordbBase (0),
    m_pFirst (NULL),
    m_pCurrent (NULL),
    m_iCount (0)
{
}

CordbEnumFilter::CordbEnumFilter(CordbEnumFilter *src) 
    : CordbBase (0),
    m_pFirst (NULL),
    m_pCurrent (NULL)
{
    int iCountSanityCheck = 0;
    EnumElement *pElementCur = NULL;
    EnumElement *pElementNew = NULL;
    EnumElement *pElementNewPrev = NULL;
    
    m_iCount = src->m_iCount;

    pElementCur = src->m_pFirst;

    while (pElementCur != NULL)
    {
        pElementNew = new EnumElement;
        if (pElementNew == NULL)
        {
            // Out of memory. Clean up and bail out.
            goto Error;
        }

        if (pElementNewPrev == NULL)
        {
            m_pFirst = pElementNew;
        }
        else
        {
            pElementNewPrev->SetNext(pElementNew);
        }

        pElementNewPrev = pElementNew;

        // Copy the element, including the AddRef part
        pElementNew->SetData(pElementCur->GetData());
        IUnknown *iu = (IUnknown *)pElementCur->GetData();
        iu->AddRef();

        if (pElementCur == src->m_pCurrent)
            m_pCurrent = pElementNew;
        
        pElementCur = pElementCur->GetNext();
        iCountSanityCheck++;
    }

    _ASSERTE(iCountSanityCheck == m_iCount);

    return;
Error:
    // release all the allocated memory before returning
    pElementCur = m_pFirst;

    while (pElementCur != NULL)
    {
        pElementNewPrev = pElementCur;
        pElementCur = pElementCur->GetNext();

        ((ICorDebugModule *)pElementNewPrev->GetData())->Release();
        delete pElementNewPrev;
    }
}

CordbEnumFilter::~CordbEnumFilter()
{
    EnumElement *pElement = m_pFirst;
    EnumElement *pPrevious = NULL;

    while (pElement != NULL)
    {
        pPrevious = pElement;
        pElement = pElement->GetNext();
        delete pPrevious;
    }
}



HRESULT CordbEnumFilter::QueryInterface(REFIID id, void **ppInterface)
{
    if (id == IID_ICorDebugModuleEnum)
        *ppInterface = (ICorDebugModuleEnum*)this;
    else if (id == IID_ICorDebugThreadEnum)
        *ppInterface = (ICorDebugThreadEnum*)this;
    else if (id == IID_IUnknown)
        *ppInterface = this;
    else
    {
        *ppInterface = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

HRESULT CordbEnumFilter::Skip(ULONG celt)
{
	INPROC_LOCK();

	while (celt-- > 0 && m_pCurrent != NULL)
        m_pCurrent = m_pCurrent->GetNext();

	INPROC_UNLOCK();

    return S_OK;
}

HRESULT CordbEnumFilter::Reset()
{
	m_pCurrent = m_pFirst;
	
    return S_OK;
}

HRESULT CordbEnumFilter::Clone(ICorDebugEnum **ppEnum)
{
    VALIDATE_POINTER_TO_OBJECT(ppEnum, ICorDebugEnum **);

	INPROC_LOCK();
    
    CordbEnumFilter *clone = new CordbEnumFilter(this);

	INPROC_UNLOCK();

    if (NULL == clone)
        return E_OUTOFMEMORY;

    clone->AddRef();

    (*ppEnum) = (ICorDebugThreadEnum *)clone;
    
    return S_OK;
}

HRESULT CordbEnumFilter::GetCount(ULONG *pcelt)
{
    VALIDATE_POINTER_TO_OBJECT(pcelt, ULONG *);
    
    *pcelt = (ULONG)m_iCount;
    return S_OK;
}

HRESULT CordbEnumFilter::Next(ULONG celt,
                ICorDebugModule *objects[],
                ULONG *pceltFetched)
{
    VALIDATE_POINTER_TO_OBJECT_ARRAY(objects, ICorDebugModule *, 
        celt, true, true);
    VALIDATE_POINTER_TO_OBJECT(pceltFetched, ULONG *);

	INPROC_LOCK();

    *pceltFetched = 0;
    while (celt-- > 0 && m_pCurrent != NULL)
    {
        objects [(*pceltFetched)++] = (ICorDebugModule *)m_pCurrent->GetData();
        m_pCurrent = m_pCurrent->GetNext();
    }

	INPROC_UNLOCK();

    return S_OK;
}


HRESULT CordbEnumFilter::Next(ULONG celt,
                ICorDebugThread *objects[],
                ULONG *pceltFetched)
{
    VALIDATE_POINTER_TO_OBJECT_ARRAY(objects, ICorDebugThread *, 
        celt, true, true);
    VALIDATE_POINTER_TO_OBJECT(pceltFetched, ULONG *);

	INPROC_LOCK();

    *pceltFetched = 0;
    while (celt-- > 0 && m_pCurrent != NULL)
    {
        objects [(*pceltFetched)++] = (ICorDebugThread *)m_pCurrent->GetData();
        m_pCurrent = m_pCurrent->GetNext();
    }

	INPROC_UNLOCK();

    return S_OK;
}


HRESULT CordbEnumFilter::Init (ICorDebugModuleEnum *pModEnum, CordbAssembly *pAssembly)
{
    ICorDebugModule *pCorModule = NULL;
    CordbModule *pModule = NULL;
    ULONG ulDummy = 0;
    
    HRESULT hr = pModEnum->Next(1, &pCorModule, &ulDummy);
    if (FAILED (hr))
        return hr;

    EnumElement *pPrevious = NULL;
    EnumElement *pElement = NULL;

    while (ulDummy != 0)
    {   
        pModule = (CordbModule *)(ICorDebugModule *)pCorModule;
        // Is this module part of the assembly for which we're enumerating?
        if (pModule->m_pAssembly == pAssembly)
        {
            pElement = new EnumElement;
            if (pElement == NULL)
            {
                // Out of memory. Clean up and bail out.
                hr = E_OUTOFMEMORY;
                goto Error;
            }

            pElement->SetData ((void *)pCorModule);
            m_iCount++;

            if (m_pFirst == NULL)
            {
                m_pFirst = pElement;
            }
            else
            {
                _ASSERTE(pPrevious != NULL);
                pPrevious->SetNext (pElement);
            }
            pPrevious = pElement;
        }
        else
            ((ICorDebugModule *)pModule)->Release();

        hr = pModEnum->Next(1, &pCorModule, &ulDummy);
        if (FAILED (hr))
            goto Error;
    }

    m_pCurrent = m_pFirst;

    return S_OK;

Error:
    // release all the allocated memory before returning
    pElement = m_pFirst;

    while (pElement != NULL)
    {
        pPrevious = pElement;
        pElement = pElement->GetNext();

        ((ICorDebugModule *)pPrevious->GetData())->Release();
        delete pPrevious;
    }

    return hr;
}

HRESULT CordbEnumFilter::Init (ICorDebugThreadEnum *pThreadEnum, CordbAppDomain *pAppDomain)
{
    ICorDebugThread *pCorThread = NULL;
    CordbThread *pThread = NULL;
    ULONG ulDummy = 0;
    
    HRESULT hr = pThreadEnum->Next(1, &pCorThread, &ulDummy);
    if (FAILED (hr))
        return hr;

    EnumElement *pPrevious = NULL;
    EnumElement *pElement = NULL;

    while (ulDummy > 0)
    {   
        pThread = (CordbThread *)(ICorDebugThread *) pCorThread;

        // Is this module part of the appdomain for which we're enumerating?
        if (pThread->m_pAppDomain == pAppDomain)
        {
            pElement = new EnumElement;
            if (pElement == NULL)
            {
                // Out of memory. Clean up and bail out.
                hr = E_OUTOFMEMORY;
                goto Error;
            }

            pElement->SetData ((void *)pCorThread);
            m_iCount++;

            if (m_pFirst == NULL)
            {
                m_pFirst = pElement;
            }
            else
            {
                _ASSERTE(pPrevious != NULL);
                pPrevious->SetNext (pElement);
            }

            pPrevious = pElement;
        }
        else
            ((ICorDebugThread *)pThread)->Release();

        //  get the next thread in the thread list
        hr = pThreadEnum->Next(1, &pCorThread, &ulDummy);
        if (FAILED (hr))
            goto Error;
    }

    m_pCurrent = m_pFirst;

    return S_OK;

Error:
    // release all the allocated memory before returning
    pElement = m_pFirst;

    while (pElement != NULL)
    {
        pPrevious = pElement;
        pElement = pElement->GetNext();

        ((ICorDebugThread *)pPrevious->GetData())->Release();
        delete pPrevious;
    }

    return hr;
}

