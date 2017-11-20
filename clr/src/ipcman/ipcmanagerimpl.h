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
// File: IPCManagerImpl.h
//
// Defines Classes to implement InterProcess Communication Manager for a COM+
//
//*****************************************************************************

#ifndef _IPCManagerImpl_H_
#define _IPCManagerImpl_H_



enum EPrivateIPCClient;

struct PrivateIPCControlBlock;


// Version of the IPC Block that this lib was compiled for.
const int VER_IPC_BLOCK = 2;


//-----------------------------------------------------------------------------
// Implementation for the IPCManager for COM+.
//-----------------------------------------------------------------------------
class IPCWriterImpl
{
public:
    IPCWriterImpl();
    ~IPCWriterImpl();

    // All interface functions should be provided in a derived class

protected:
    // Helpers
    HRESULT CreateWinNTDescriptor(DWORD pid, SECURITY_ATTRIBUTES **ppSA);

    void CloseGenericIPCBlock(HANDLE & hMemFile, void * & pBlock);
    HRESULT CreateNewPrivateIPCBlock();
    
    void WriteEntryHelper(EPrivateIPCClient eClient, DWORD size);
    void CreatePrivateIPCHeader();
    void OpenPrivateIPCHeader();

    bool IsPrivateBlockOpen() const;

    // Cache pointers to each section
    struct PerfCounterIPCControlBlock   *m_pPerf;
    struct DebuggerIPCControlBlock      *m_pDebug;
    struct AppDomainEnumerationIPCBlock *m_pAppDomain;
    struct ServiceIPCControlBlock       *m_pService;

    // Stats on MemoryMapped file for the given pid 
    HANDLE                               m_handlePrivateBlock;
    PrivateIPCControlBlock              *m_ptrPrivateBlock;

};


//-----------------------------------------------------------------------------
// IPCReader class connects to a COM+ IPC block and reads from it
//-----------------------------------------------------------------------------
class IPCReaderImpl
{
public:
    IPCReaderImpl();
    ~IPCReaderImpl();

protected:

    HANDLE  m_handlePrivateBlock;
    PrivateIPCControlBlock * m_ptrPrivateBlock;
};



//-----------------------------------------------------------------------------
// Return true if our Private block is available.
//-----------------------------------------------------------------------------
inline bool IPCWriterImpl::IsPrivateBlockOpen() const
{
    return m_ptrPrivateBlock != NULL;
}



#endif // _IPCManagerImpl_H_
