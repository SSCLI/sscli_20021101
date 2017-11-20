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
// File: IPCWriterImpl.cpp
//
// Implementation for COM+ memory mapped file writing
//
//*****************************************************************************

#include "stdafx.h"

#include "ipcmanagerinterface.h"
#include "ipcheader.h"
#include "ipcshared.h"


// Get version numbers for IPCHeader stamp
#include "version/__official__.ver"

const USHORT BuildYear = COR_BUILD_YEAR;
const USHORT BuildNumber = COR_OFFICIAL_BUILD_NUMBER;


// Import from mscoree.obj
HINSTANCE GetModuleInst();


//-----------------------------------------------------------------------------
// Generic init
//-----------------------------------------------------------------------------
HRESULT IPCWriterInterface::Init() 
{
    // Nothing to do anymore in here...
    return S_OK;
}

//-----------------------------------------------------------------------------
// Generic terminate
//-----------------------------------------------------------------------------
void IPCWriterInterface::Terminate() 
{
    IPCShared::CloseGenericIPCBlock(m_handlePrivateBlock, (void*&) m_ptrPrivateBlock);

}


//-----------------------------------------------------------------------------
// Open our Private IPC block on the given pid.
//-----------------------------------------------------------------------------
HRESULT IPCWriterInterface::CreatePrivateBlockOnPid(DWORD pid, BOOL inService, HINSTANCE *phInstIPCBlockOwner) 
{
    // Note: if PID != GetCurrentProcessId(), we're expected to be opening
    // someone else's IPCBlock, so if it doesn't exist, we should assert.
    HRESULT hr = NO_ERROR;

    // Init the IPC block owner HINSTANCE to 0.
    *phInstIPCBlockOwner = 0;

    // Note: if our private block is open, we shouldn't be creating it again.
    _ASSERTE(!IsPrivateBlockOpen());
    
    if (IsPrivateBlockOpen()) 
    {
        // if we goto errExit, it will close the file. We don't want that.
        return ERROR_ALREADY_EXISTS;
    }

    SECURITY_ATTRIBUTES *pSA = NULL;

    // Grab the SA
    hr = CreateWinNTDescriptor(pid, &pSA);
    if (FAILED(hr))
        return hr;
    
    // Raw creation
    WCHAR szMemFileName[100];
    IPCShared::GenerateName(pid, szMemFileName, 100);

    // Connect the handle   
    m_handlePrivateBlock = WszCreateFileMapping(INVALID_HANDLE_VALUE,
                                                pSA,
                                                PAGE_READWRITE,
                                                0,
                                                sizeof(PrivateIPCControlBlock),
                                                szMemFileName);

    DWORD dwFileMapErr = GetLastError();

    LOG((LF_CORDB, LL_INFO10, "IPCWI::CPBOP: Writer: CreateFileMapping = 0x%08x, GetLastError=%d\n",
         m_handlePrivateBlock, GetLastError()));

    if (m_handlePrivateBlock == NULL)
    {
        // If the create fails, it may fail because the SA didn't match _exactly_ with what it was created with. Try to
        // Open it instead, since we may really have access!
        m_handlePrivateBlock = WszOpenFileMapping(FILE_MAP_ALL_ACCESS,
                                                  FALSE,
                                                  szMemFileName);

        dwFileMapErr = GetLastError();
    }

    _ASSERTE(m_handlePrivateBlock != NULL || GetLastError() != ERROR_ACCESS_DENIED);

    // If unsuccessful, bail
    if (m_handlePrivateBlock == NULL)
    {
        hr = HRESULT_FROM_WIN32(dwFileMapErr);
        goto errExit;
    }

    // Get the pointer - must get it even if dwFileMapErr == ERROR_ALREADY_EXISTS,
    // since the IPC block is allowed to already exist if the URT service created it.
    m_ptrPrivateBlock = (PrivateIPCControlBlock *) MapViewOfFile(m_handlePrivateBlock,
                                                                 FILE_MAP_ALL_ACCESS,
                                                                 0, 0, 0);

    // If the map failed, bail
    if (m_ptrPrivateBlock == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto errExit;
    }

    // If it already existed, see if the URT service created it.
    if (dwFileMapErr == ERROR_ALREADY_EXISTS)
    {
        // Fill in the member pointers
        OpenPrivateIPCHeader();
        
        // If the URT Service created it, then we're done. Otherwise, we need to take special action below.
        if (GetServiceBlock()->bNotifyService == FALSE)
        {
            if (inService)
            {
                // If we're in a service now, we don't want to take ownership of the IPC block, so just return the
                // error.
                hr = HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
                goto errExit;
            }
            else
            {
                // If the service didn't create it, then its possible the IPC block for this process has been held open
                // past the process's death by some other process, i.e., a slow debugger, a process with the perf
                // counters open, etc. In this case, we zero out the existing IPC block and re-initialize it for this
                // process.
                //
                // Note: this used to be an error and we would return ERROR_ALREADY_EXISTS. But this is a valid case
                // now.
                memset(m_ptrPrivateBlock, 0, sizeof(PrivateIPCControlBlock));
                CreatePrivateIPCHeader();
            }
        }
    }
    else
    {
        // Hook up each sections' pointers
        CreatePrivateIPCHeader();
    }

errExit:
    if (!SUCCEEDED(hr)) 
    {
        IPCShared::CloseGenericIPCBlock(m_handlePrivateBlock, (void*&)m_ptrPrivateBlock);
    }

    DestroySecurityAttributes(pSA);

    return hr;

}

//-----------------------------------------------------------------------------
// Accessors to get each clients' blocks
//-----------------------------------------------------------------------------
struct PerfCounterIPCControlBlock * IPCWriterInterface::GetPerfBlock() 
{
    return m_pPerf;
}

struct DebuggerIPCControlBlock * IPCWriterInterface::GetDebugBlock() 
{
    return m_pDebug;
}   

struct AppDomainEnumerationIPCBlock * IPCWriterInterface::GetAppDomainBlock() 
{
    return m_pAppDomain;
}   

struct ServiceIPCControlBlock * IPCWriterInterface::GetServiceBlock() 
{
    return m_pService;
}   


ClassDumpTableBlock* IPCWriterInterface::GetClassDumpTableBlock()
{
  return &m_ptrPrivateBlock->m_dump;
}

//-----------------------------------------------------------------------------
// Return the security attributes for the shared memory for a given process.
//-----------------------------------------------------------------------------
HRESULT IPCWriterInterface::GetSecurityAttributes(DWORD pid, SECURITY_ATTRIBUTES **ppSA)
{
    return CreateWinNTDescriptor(pid, ppSA);
}

//-----------------------------------------------------------------------------
// Helper to destroy the security attributes for the shared memory for a given
// process.
//-----------------------------------------------------------------------------
void IPCWriterInterface::DestroySecurityAttributes(SECURITY_ATTRIBUTES *pSA)
{
}

//-----------------------------------------------------------------------------
// Have ctor zero everything out
//-----------------------------------------------------------------------------
IPCWriterImpl::IPCWriterImpl()
{
    // Cache pointers to sections
    m_pPerf      = NULL;
    m_pDebug     = NULL;
    m_pAppDomain = NULL;
    m_pService   = NULL;

    // Mem-Mapped file for Private Block
    m_handlePrivateBlock    = NULL;
    m_ptrPrivateBlock       = NULL;

}

//-----------------------------------------------------------------------------
// Assert that everything was already shutdown by a call to terminate.
// Shouldn't be anything left to do in the dtor
//-----------------------------------------------------------------------------
IPCWriterImpl::~IPCWriterImpl()
{
    _ASSERTE(!IsPrivateBlockOpen());
}

//-----------------------------------------------------------------------------
// Creation / Destruction
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Setup a security descriptor for the named kernel objects if we're on NT.
//-----------------------------------------------------------------------------
HRESULT IPCWriterImpl::CreateWinNTDescriptor(DWORD pid, SECURITY_ATTRIBUTES **ppSA)
{
    HRESULT hr = NO_ERROR;
    
    *ppSA = NULL;


    return hr;
}

//-----------------------------------------------------------------------------
// Helper: Open the private block. We expect that our members have been set
// by this point.
// Note that the private block is only created once, so it fails on 
// Already Exists.
// The private block is used by DebuggerRCThread::Init and PerfCounters::Init.
//-----------------------------------------------------------------------------



void IPCWriterImpl::WriteEntryHelper(EPrivateIPCClient eClient, DWORD size)
{
    // Don't use this helper on the first entry, since it looks at the entry before it
    _ASSERTE(eClient != 0);

    EPrivateIPCClient ePrev = (EPrivateIPCClient) ((DWORD) eClient - 1);

    m_ptrPrivateBlock->m_table[eClient].m_Offset = 
        m_ptrPrivateBlock->m_table[ePrev].m_Offset + 
        m_ptrPrivateBlock->m_table[ePrev].m_Size;

    m_ptrPrivateBlock->m_table[eClient].m_Size  = size;
}


//-----------------------------------------------------------------------------
// Initialize the header for our private IPC block
//-----------------------------------------------------------------------------
void IPCWriterImpl::CreatePrivateIPCHeader()
{
    // Stamp the IPC block with the version
    m_ptrPrivateBlock->m_header.m_dwVersion = VER_IPC_BLOCK;
    m_ptrPrivateBlock->m_header.m_blockSize = sizeof(PrivateIPCControlBlock);

    m_ptrPrivateBlock->m_header.m_hInstance = GetModuleInst();

    m_ptrPrivateBlock->m_header.m_BuildYear = BuildYear;
    m_ptrPrivateBlock->m_header.m_BuildNumber = BuildNumber;

    m_ptrPrivateBlock->m_header.m_numEntries = ePrivIPC_MAX;

    // Fill out directory (offset and size of each block)
    m_ptrPrivateBlock->m_table[ePrivIPC_PerfCounters].m_Offset = 0;
    m_ptrPrivateBlock->m_table[ePrivIPC_PerfCounters].m_Size    = sizeof(PerfCounterIPCControlBlock);

    // NOTE: these must appear in the exact order they are listed in PrivateIPCControlBlock or
    //       VERY bad things will happen.
    WriteEntryHelper(ePrivIPC_Debugger, sizeof(DebuggerIPCControlBlock));
    WriteEntryHelper(ePrivIPC_AppDomain, sizeof(AppDomainEnumerationIPCBlock));
    WriteEntryHelper(ePrivIPC_Service, sizeof(ServiceIPCControlBlock));
    WriteEntryHelper(ePrivIPC_ClassDump, sizeof(ClassDumpTableBlock));

    // Cache our client pointers
    m_pPerf     = &(m_ptrPrivateBlock->m_perf);
    m_pDebug    = &(m_ptrPrivateBlock->m_dbg);
    m_pAppDomain= &(m_ptrPrivateBlock->m_appdomain);
    m_pService  = &(m_ptrPrivateBlock->m_svc);
}

//-----------------------------------------------------------------------------
// Initialize the header for our private IPC block
//-----------------------------------------------------------------------------
void IPCWriterImpl::OpenPrivateIPCHeader()
{
    // Cache our client pointers
    m_pPerf     = &(m_ptrPrivateBlock->m_perf);
    m_pDebug    = &(m_ptrPrivateBlock->m_dbg);
    m_pAppDomain= &(m_ptrPrivateBlock->m_appdomain);
    m_pService  = &(m_ptrPrivateBlock->m_svc);
}

DWORD IPCWriterInterface::GetBlockVersion()
{
    _ASSERTE(IsPrivateBlockOpen());
    return m_ptrPrivateBlock->m_header.m_dwVersion;
}

DWORD IPCWriterInterface::GetBlockSize()
{
    _ASSERTE(IsPrivateBlockOpen());
    return m_ptrPrivateBlock->m_header.m_blockSize;
}

HINSTANCE IPCWriterInterface::GetInstance()
{
    _ASSERTE(IsPrivateBlockOpen());
    return m_ptrPrivateBlock->m_header.m_hInstance;
}

USHORT IPCWriterInterface::GetBuildYear()
{
    _ASSERTE(IsPrivateBlockOpen());
    return m_ptrPrivateBlock->m_header.m_BuildYear;
}

USHORT IPCWriterInterface::GetBuildNumber()
{
    _ASSERTE(IsPrivateBlockOpen());
    return m_ptrPrivateBlock->m_header.m_BuildNumber;
}

PVOID IPCWriterInterface::GetBlockStart()
{
    return (PVOID) m_ptrPrivateBlock;
}
