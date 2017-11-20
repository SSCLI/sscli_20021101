#include <windows.h>
#include <stdio.h>
#include "profilercallback.h"

//====================================================
ULONG CProfilerCallback::AddRef()
{
    return S_OK;
}
ULONG CProfilerCallback::Release()
{
    return S_OK;
}
HRESULT CProfilerCallback::QueryInterface( REFIID riid, void **ppInterface )
{
    return E_NOTIMPL;
}
//====================================================

HRESULT CProfilerCallback::Initialize(IUnknown * pICorProfilerInfoUnk )
{
    GetInititializationParameters();


    // Get the ICorProfilerInfo interface we need, and stuff it away in
    // a member variable.
    HRESULT hr =
        pICorProfilerInfoUnk->QueryInterface( IID_ICorProfilerInfo,
                                            (LPVOID *)&m_pICorProfilerInfo );
    if ( FAILED(hr) )
        return E_INVALIDARG;

    // Indicate which events we're interested in.
    // See GetInititializationParameters
    m_pICorProfilerInfo->SetEventMask( m_dwEventMask );

    // This is the first thread, so set up the per-thread data
    // for it.  this code is essentially replicated in the
    // ::ThreadCreated method
    m_tlsIndex = TlsAlloc();
    CPerThreadData * pPerThreadData = new CPerThreadData;
    pPerThreadData->m_uNestingLevel = 0;
    TlsSetValue( m_tlsIndex, pPerThreadData );

    // Open the file that we'll write out output to
    m_pOutFile = fopen( m_szOutfileName, "wt" );

    ProfilerPrintf( "Initialize\n" );

    return S_OK;
}

HRESULT CProfilerCallback::Shutdown()
{
    ProfilerPrintf( "Shutdown\n" );
    return E_NOTIMPL;
}
HRESULT CProfilerCallback::AppDomainCreationStarted(UINT appDomainId)
{
    ProfilerPrintf( "AppDomainCreationStarted\n" );

    ChangeNestingLevel( 1 );

    return E_NOTIMPL;
}
HRESULT CProfilerCallback::AppDomainCreationFinished(UINT appDomainId,
                                                     HRESULT hrStatus)
{
    ChangeNestingLevel( -1 );

    // Now it's OK to grab the name
    wchar_t wszDomain[512];
    ULONG cchDomain = sizeof(wszDomain)/sizeof(wszDomain[0]);

    m_pICorProfilerInfo->GetAppDomainInfo( appDomainId, cchDomain, &cchDomain,
                                            wszDomain, 0 ); 

    ProfilerPrintf( "AppDomainCreationFinished: %ls\n", wszDomain );
    return E_NOTIMPL;
}
HRESULT CProfilerCallback::AppDomainShutdownStarted(UINT appDomainId)
{
    ProfilerPrintf( "AppDomainShutdownStarted\n" );

    ChangeNestingLevel( 1 );

    return E_NOTIMPL;
}
HRESULT CProfilerCallback::AppDomainShutdownFinished(UINT appDomainId,
                                                     HRESULT hrStatus)
{
    ChangeNestingLevel( -1 );

    ProfilerPrintf( "AppDomainShutdownFinished\n" );

    return E_NOTIMPL;
}
HRESULT CProfilerCallback::AssemblyLoadStarted(UINT assemblyId)
{   
    // Assembly ID can't be used yet, so we can't grab the name...
    ProfilerPrintf( "AssemblyLoadStarted\n" );

    ChangeNestingLevel( 1 );

    return E_NOTIMPL;
}
HRESULT CProfilerCallback::AssemblyLoadFinished(UINT assemblyId,
                                                    HRESULT hrStatus)
{
    ChangeNestingLevel( -1 );

    // Now it's OK to grab the name
    wchar_t wszAssembly[512];
    ULONG cchAssembly = sizeof(wszAssembly)/sizeof(wszAssembly[0]);

    m_pICorProfilerInfo->GetAssemblyInfo( assemblyId, cchAssembly,
                                            &cchAssembly, wszAssembly, 0, 0 );

    ProfilerPrintf( "AssemblyLoadFinished: %ls  Status: %08X\n", wszAssembly,
                    hrStatus );

    return E_NOTIMPL;
}
HRESULT CProfilerCallback::AssemblyUnloadStarted(UINT assemblyId)
{
    wchar_t wszAssembly[512];
    ULONG cchAssembly = sizeof(wszAssembly)/sizeof(wszAssembly[0]);

    m_pICorProfilerInfo->GetAssemblyInfo( assemblyId, cchAssembly,
                                            &cchAssembly, wszAssembly, 0, 0 );
    ProfilerPrintf( "AssemblyUnloadStarted: %ls\n", wszAssembly );

    ChangeNestingLevel( 1 );

    return E_NOTIMPL;
}
HRESULT CProfilerCallback::AssemblyUnloadFinished(UINT assemblyId,
                                                  HRESULT hrStatus)
{
    ChangeNestingLevel( -1 );

    ProfilerPrintf( "AssemblyUnloadFinished\n" );

    return E_NOTIMPL;
}
HRESULT CProfilerCallback::ModuleLoadStarted(UINT moduleId)
{
    // The moduleId can't be used for anything yet...
    ProfilerPrintf( "ModuleLoadStarted\n" );

    ChangeNestingLevel( 1 );

    return E_NOTIMPL;
}
HRESULT CProfilerCallback::ModuleLoadFinished(UINT moduleId, HRESULT hrStatus)
{
    ChangeNestingLevel( -1 );

    // Now the moduleId can be used to retrieve the module name
    wchar_t wszModule[512];
    ULONG cchModule = sizeof(wszModule)/sizeof(wszModule[0]);

    m_pICorProfilerInfo->GetModuleInfo( moduleId, 0, cchModule, &cchModule,
                                        wszModule, 0 );

    ProfilerPrintf( "ModuleLoadFinished: %ls\n", wszModule );

    return E_NOTIMPL;
}
HRESULT CProfilerCallback::ModuleUnloadStarted(UINT moduleId)
{
    // Now the moduleId can be used to retrieve the module name
    wchar_t wszModule[512];
    ULONG cchModule = sizeof(wszModule)/sizeof(wszModule[0]);

    m_pICorProfilerInfo->GetModuleInfo( moduleId, 0, cchModule, &cchModule,
                                        wszModule, 0 );

    ProfilerPrintf( "ModuleUnloadStarted: %ls\n", wszModule );

    ChangeNestingLevel( 1 );

    return E_NOTIMPL;
}
HRESULT CProfilerCallback::ModuleUnloadFinished(UINT moduleId,
                                                HRESULT hrStatus)
{
    ChangeNestingLevel( -1 );

    ProfilerPrintf( "ModuleUnloadFinished\n" );

    return E_NOTIMPL;
}
HRESULT CProfilerCallback::ModuleAttachedToAssembly(UINT moduleId,
                                                    UINT assemblyId)
{
    wchar_t wszAssembly[512] = { 0 };
    ULONG cchAssembly = sizeof(wszAssembly)/sizeof(wszAssembly[0]);

    m_pICorProfilerInfo->GetAssemblyInfo( assemblyId, cchAssembly,
                                            &cchAssembly, wszAssembly, 0, 0 );

    ProfilerPrintf( "ModuleAttachedToAssembly: %ls\n", wszAssembly );

    return S_OK;
}
HRESULT CProfilerCallback::ClassLoadStarted(UINT classId)
{
    wchar_t wszClass[512];

    if ( GetClassNameFromClassId( classId, wszClass ) )
    {
        ProfilerPrintf( "ClassLoadStarted: %ls\n", wszClass );
    }
    else
    {
        ProfilerPrintf( "ClassLoadStarted\n" );
    }

    ChangeNestingLevel( 1 );

    return S_OK;
}
HRESULT CProfilerCallback::ClassLoadFinished(UINT classId, HRESULT hrStatus)
{
    ChangeNestingLevel( -1 );

    ProfilerPrintf( "ClassLoadFinished\n" );

    return E_NOTIMPL;
}
HRESULT CProfilerCallback::ClassUnloadStarted(UINT classId)
{
    wchar_t wszClass[512];

    if ( GetClassNameFromClassId( classId, wszClass ) )
    {
        ProfilerPrintf( "ClassUnloadStarted: %ls\n", wszClass );
    }
    else
    {
        ProfilerPrintf( "ClassUnloadStarted\n" );
    }

    ChangeNestingLevel( 1 );

    return E_NOTIMPL;
}
HRESULT CProfilerCallback::ClassUnloadFinished(UINT classId, HRESULT hrStatus)
{
    ChangeNestingLevel( -1 );

    ProfilerPrintf( "ClassUnloadFinished\n" );

    return E_NOTIMPL;
}
HRESULT CProfilerCallback::FunctionUnloadStarted(UINT functionId)
{
    ProfilerPrintf( "FunctionUnloadStarted\n" );
    return E_NOTIMPL;
}
HRESULT CProfilerCallback::JITCompilationStarted(UINT functionId,
                                                BOOL fIsSafeToBlock)
{
    wchar_t wszClass[512];
    wchar_t wszMethod[512];

    if ( GetMethodNameFromFunctionId( functionId, wszClass, wszMethod ) )
    {
        ProfilerPrintf("JITCompilationStarted: %ls::%ls\n",wszClass,wszMethod);
    }
    else
    {
        ProfilerPrintf( "JITCompilationStarted\n" );
    }

    ChangeNestingLevel( 1 );

    return S_OK;
}
HRESULT CProfilerCallback::JITCompilationFinished(UINT functionId,
                                        HRESULT hrStatus, BOOL fIsSafeToBlock)
{
    ChangeNestingLevel( -1 );

    ProfilerPrintf( "JITCompilationFinished\n" );

    return E_NOTIMPL;
}
HRESULT CProfilerCallback::JITCachedFunctionSearchStarted(UINT functionId,
                                                BOOL * pbUseCachedFunction)
{
    wchar_t wszClass[512];
    wchar_t wszMethod[512];

    if ( GetMethodNameFromFunctionId( functionId, wszClass, wszMethod ) )
    {
        ProfilerPrintf( "JITCachedFunctionSearchStarted: %ls::%ls\n",
                        wszClass, wszMethod );
    }
    else
    {
        ProfilerPrintf( "JITCachedFunctionSearchStarted\n" );
    }
        
    // Force JIT'ting to occur
    *pbUseCachedFunction = FALSE;

    return S_OK;
}
HRESULT CProfilerCallback::JITCachedFunctionSearchFinished(UINT functionId,
                                                    COR_PRF_JIT_CACHE result)
{
    ChangeNestingLevel( -1 );

    ProfilerPrintf( "JITCachedFunctionSearchFinished\n" );

    return E_NOTIMPL;
}
HRESULT CProfilerCallback::JITFunctionPitched(UINT functionId)
{
    ProfilerPrintf( "JITFunctionPitched\n" );
    return E_NOTIMPL;
}
HRESULT CProfilerCallback::JITInlining(UINT callerId, UINT calleeId,
                                            BOOL * pfShouldInline)
{
    ProfilerPrintf( "JITInlining\n" );

    if (pfShouldInline == NULL)
        return E_POINTER;
        
    return E_NOTIMPL;
}
HRESULT CProfilerCallback::ThreadCreated(UINT threadId)
{
    // See if we've already created the CPerThreadData
    CPerThreadData * pPerThreadData =(CPerThreadData*)TlsGetValue(m_tlsIndex);
    if ( !pPerThreadData )  // If not, go allocate and initialize one
    {
        pPerThreadData = new CPerThreadData;
        pPerThreadData->m_uNestingLevel = 0;
        TlsSetValue( m_tlsIndex, pPerThreadData );
    }

    ProfilerPrintf( "ThreadCreated\n" );

    return E_NOTIMPL;
}
HRESULT CProfilerCallback::ThreadDestroyed(UINT threadId)
{
    ProfilerPrintf( "ThreadDestroyed\n" );
    return E_NOTIMPL;
}
HRESULT CProfilerCallback::ThreadAssignedToOSThread(UINT managedThreadId,
                                                    ULONG osThreadId)
{
    ProfilerPrintf( "ThreadAssignedToOSThread\n" );
    return E_NOTIMPL;
}
HRESULT CProfilerCallback::RemotingClientInvocationStarted()
{
    ProfilerPrintf( "RemotingClientInvocationStarted\n" );
    return E_NOTIMPL;
}
HRESULT CProfilerCallback::RemotingClientSendingMessage(GUID * pCookie,
                                                        BOOL fIsAsync)
{
    ProfilerPrintf( "RemotingClientSendingMessage\n" );
    return E_NOTIMPL;
}
HRESULT CProfilerCallback::RemotingClientReceivingReply(GUID * pCookie,
                                                        BOOL fIsAsync)
{
    ProfilerPrintf( "RemotingClientReceivingReply\n" );
    return E_NOTIMPL;
}
HRESULT CProfilerCallback::RemotingClientInvocationFinished()
{
    ProfilerPrintf( "RemotingClientInvocationFinished\n" );
    return E_NOTIMPL;
}
HRESULT CProfilerCallback::RemotingServerReceivingMessage(GUID * pCookie,
                                                            BOOL fIsAsync)
{
    ProfilerPrintf( "RemotingServerReceivingMessage\n" );
    return E_NOTIMPL;
}
HRESULT CProfilerCallback::RemotingServerInvocationStarted()
{
    ProfilerPrintf( "RemotingServerInvocationStarted\n" );
    return E_NOTIMPL;
}
HRESULT CProfilerCallback::RemotingServerInvocationReturned()
{
    ProfilerPrintf( "RemotingServerInvocationReturned\n" );
    return E_NOTIMPL;
}
HRESULT CProfilerCallback::RemotingServerSendingReply(GUID * pCookie,
                                                        BOOL fIsAsync)
{
    ProfilerPrintf( "RemotingServerSendingReply\n" );
    return E_NOTIMPL;
}
HRESULT CProfilerCallback::UnmanagedToManagedTransition(UINT functionId,
                                            COR_PRF_TRANSITION_REASON reason)
{
    wchar_t wszClass[512];
    wchar_t wszMethod[512];

    if ( reason == COR_PRF_TRANSITION_RETURN )
        ChangeNestingLevel( -1 );

    if ( GetMethodNameFromFunctionId( functionId, wszClass, wszMethod ) )
    {
        ProfilerPrintf( "UnmanagedToManagedTransition: %ls::%ls\n", wszClass,
                            wszMethod );
    }
    else
    {
        ProfilerPrintf( "UnmanagedToManagedTransition\n" );
    }

    if ( reason == COR_PRF_TRANSITION_CALL )
        ChangeNestingLevel( 1 );

    return E_NOTIMPL;
}
HRESULT CProfilerCallback::ManagedToUnmanagedTransition(UINT functionId,
                                            COR_PRF_TRANSITION_REASON reason)
{
    wchar_t wszClass[512];
    wchar_t wszMethod[512];

    if ( reason == COR_PRF_TRANSITION_RETURN )
        ChangeNestingLevel( -1 );

    if ( GetMethodNameFromFunctionId( functionId, wszClass, wszMethod ) )
    {
        ProfilerPrintf( "ManagedToUnmanagedTransition: %ls::%ls\n", wszClass,
                        wszMethod );
    }
    else
    {
        ProfilerPrintf( "ManagedToUnmanagedTransition\n" );
    }

    if ( reason == COR_PRF_TRANSITION_CALL )
        ChangeNestingLevel( 1 );

    return E_NOTIMPL;
}
HRESULT CProfilerCallback::RuntimeSuspendStarted(
                                        COR_PRF_SUSPEND_REASON suspendReason)
{
    char * pszSuspendReason = "???";

    switch ( suspendReason )
    {
        case COR_PRF_SUSPEND_FOR_GC:
            pszSuspendReason = "SUSPEND_FOR_GC"; break;
        case COR_PRF_SUSPEND_FOR_APPDOMAIN_SHUTDOWN:
            pszSuspendReason = "SUSPEND_FOR_APPDOMAIN_SHUTDOWN"; break;
        case COR_PRF_SUSPEND_FOR_CODE_PITCHING:
            pszSuspendReason = "SUSPEND_FOR_CODE_PITCHING"; break;
        case COR_PRF_SUSPEND_FOR_SHUTDOWN:
            pszSuspendReason = "SUSPEND_FOR_SHUTDOWN"; break;
        case COR_PRF_SUSPEND_FOR_INPROC_DEBUGGER:
            pszSuspendReason = "SUSPEND_FOR_INPROC_DEBUGGER"; break;
        case COR_PRF_SUSPEND_FOR_GC_PREP:
            pszSuspendReason = "SUSPEND_FOR_GC_PREP"; break;
        case COR_PRF_SUSPEND_OTHER:
            pszSuspendReason = "SUSPEND_OTHER"; break;
    }

    ProfilerPrintf( "RuntimeSuspendStarted: %s\n", pszSuspendReason );

    ChangeNestingLevel( 1 );

    return E_NOTIMPL;
}
HRESULT CProfilerCallback::RuntimeSuspendFinished()
{
    ChangeNestingLevel( -1 );

    ProfilerPrintf( "RuntimeSuspendFinished\n" );

    return E_NOTIMPL;
}
HRESULT CProfilerCallback::RuntimeSuspendAborted()
{
    ProfilerPrintf( "RuntimeSuspendAborted\n" );
    return E_NOTIMPL;
}
HRESULT CProfilerCallback::RuntimeResumeStarted()
{
    ProfilerPrintf( "RuntimeResumeStarted\n" );

    ChangeNestingLevel( 1 );

    return E_NOTIMPL;
}
HRESULT CProfilerCallback::RuntimeResumeFinished()
{
    ChangeNestingLevel( -1 );

    ProfilerPrintf( "RuntimeResumeFinished\n" );

    return E_NOTIMPL;
}
HRESULT CProfilerCallback::RuntimeThreadSuspended(UINT threadId)
{
    ProfilerPrintf( "RuntimeThreadSuspended\n" );
    return E_NOTIMPL;
}
HRESULT CProfilerCallback::RuntimeThreadResumed(UINT threadId)
{
    ProfilerPrintf( "RuntimeThreadResumed\n" );
    return E_NOTIMPL;
}
HRESULT CProfilerCallback::MovedReferences(ULONG cMovedObjectIDRanges,
                                            UINT oldObjectIDRangeStart[],
                                            UINT newObjectIDRangeStart[],
                                            ULONG cObjectIDRangeLength[])
{
    ProfilerPrintf( "MovedReferences\n" );

    return E_NOTIMPL;
}
HRESULT CProfilerCallback::ObjectAllocated(UINT objectId, UINT classId)
{
    wchar_t wszClass[512];

    if ( GetClassNameFromClassId( classId, wszClass ) )
    {
        ProfilerPrintf( "ObjectAllocated: %ls\n", wszClass );
    }
    else
    {
        ProfilerPrintf( "ObjectAllocated\n" );
    }

    return E_NOTIMPL;
}
HRESULT CProfilerCallback::ObjectsAllocatedByClass(ULONG cClassCount,
                                                    UINT classIds[],
                                                    ULONG cObjects[])
{
    ProfilerPrintf( "ObjectsAllocatedByClass\n" );
    return E_NOTIMPL;
}
HRESULT CProfilerCallback::ObjectReferences(UINT objectId, UINT classId,
                                        ULONG cObjectRefs, UINT objectRefIds[])
{
    wchar_t wszClass[512];

    if ( GetClassNameFromClassId( classId, wszClass ) )
    {
       ProfilerPrintf("ObjectReferences: %ls refs: %u\n",wszClass,cObjectRefs);
    }
    else
    {
       ProfilerPrintf( "ObjectReferences\n" );
    }

    return S_OK;
}
HRESULT CProfilerCallback::RootReferences(ULONG cRootRefs, UINT rootRefIds[])
{
    ProfilerPrintf( "RootReferences\n" );

    return E_NOTIMPL;
}
HRESULT CProfilerCallback::ExceptionThrown(UINT thrownObjectId)
{
    ClassID classId;

    HRESULT hr = m_pICorProfilerInfo->GetClassFromObject( thrownObjectId,
                                                            &classId );

    if (SUCCEEDED(hr))
    {
        wchar_t wszClass[512];

        if ( GetClassNameFromClassId( classId, wszClass ) )
        {
            ProfilerPrintf( "ExceptionThrown: %ls\n", wszClass );
        }
        else
        {
            ProfilerPrintf( "ExceptionThrown\n" );
        }
    }

    return E_NOTIMPL;
}
HRESULT CProfilerCallback::ExceptionSearchFunctionEnter(UINT functionId)
{
    wchar_t wszClass[512];
    wchar_t wszMethod[512];

    GetMethodNameFromFunctionId( functionId, wszClass, wszMethod );

    ProfilerPrintf( "ExceptionSearchFunctionEnter: %ls::%ls\n", wszClass,
                    wszMethod );

    ChangeNestingLevel( 1 );

    return E_NOTIMPL;
}
HRESULT CProfilerCallback::ExceptionSearchFunctionLeave()
{
    ChangeNestingLevel( -1 );

    ProfilerPrintf( "ExceptionSearchFunctionLeave\n" );

    return E_NOTIMPL;
}
HRESULT CProfilerCallback::ExceptionSearchFilterEnter(UINT functionId)
{
    wchar_t wszClass[512];
    wchar_t wszMethod[512];

    GetMethodNameFromFunctionId( functionId, wszClass, wszMethod );

    ProfilerPrintf( "ExceptionSearchFilterEnter: %ls::%ls\n", wszClass,
                    wszMethod );

    ChangeNestingLevel( 1 );

    return E_NOTIMPL;
}
HRESULT CProfilerCallback::ExceptionSearchFilterLeave()
{
    ChangeNestingLevel( -1 );

    ProfilerPrintf( "ExceptionSearchFilterLeave\n" );

    return E_NOTIMPL;
}
HRESULT CProfilerCallback::ExceptionSearchCatcherFound(UINT functionId)
{
    wchar_t wszClass[512];
    wchar_t wszMethod[512];

    GetMethodNameFromFunctionId( functionId, wszClass, wszMethod );

    ProfilerPrintf( "ExceptionSearchCatcherFound: %ls::%ls\n", wszClass,
                    wszMethod );

    return E_NOTIMPL;
}
HRESULT CProfilerCallback::ExceptionOSHandlerEnter(UINT functionId)
{
    wchar_t wszClass[512];
    wchar_t wszMethod[512];

    GetMethodNameFromFunctionId( functionId, wszClass, wszMethod );

    ProfilerPrintf("ExceptionOSHandlerEnter: %ls::%ls\n", wszClass, wszMethod);

    ChangeNestingLevel( 1 );

    return E_NOTIMPL;
}
HRESULT CProfilerCallback::ExceptionOSHandlerLeave(UINT functionId)
{
    ChangeNestingLevel( -1 );

    ProfilerPrintf( "ExceptionOSHandlerLeave\n" );

    return E_NOTIMPL;
}
HRESULT CProfilerCallback::ExceptionUnwindFunctionEnter(UINT functionId)
{
    wchar_t wszClass[512];
    wchar_t wszMethod[512];

    GetMethodNameFromFunctionId( functionId, wszClass, wszMethod );

    ProfilerPrintf( "ExceptionUnwindFunctionEnter: %ls::%ls\n", wszClass,
                    wszMethod );

    return E_NOTIMPL;
}
HRESULT CProfilerCallback::ExceptionUnwindFunctionLeave()
{
    ProfilerPrintf( "ExceptionUnwindFunctionLeave\n" );

    return E_NOTIMPL;
}
HRESULT CProfilerCallback::ExceptionUnwindFinallyEnter(UINT functionId)
{
    ProfilerPrintf( "ExceptionUnwindFinallyEnter\n" );

    return E_NOTIMPL;
}
HRESULT CProfilerCallback::ExceptionUnwindFinallyLeave()
{
    ProfilerPrintf( "ExceptionUnwindFinallyLeave\n" );

    return E_NOTIMPL;
}
HRESULT CProfilerCallback::ExceptionCatcherEnter(UINT functionId,UINT objectId)
{
    wchar_t wszClass[512];
    wchar_t wszMethod[512];

    GetMethodNameFromFunctionId( functionId, wszClass, wszMethod );

    ProfilerPrintf( "ExceptionCatcherEnter: %ls::%ls\n", wszClass, wszMethod );

    ChangeNestingLevel( 1 );

    return E_NOTIMPL;
}
HRESULT CProfilerCallback::ExceptionCatcherLeave()
{
    ChangeNestingLevel( -1 );

    ProfilerPrintf( "ExceptionCatcherLeave\n" );

    return E_NOTIMPL;
}

HRESULT CProfilerCallback::COMClassicVTableCreated(ClassID wrappedClassId,
                                            REFGUID implementedIID,
                                            VOID * pUnk, ULONG cSlots)
{
    wchar_t wszClass[512];

    if ( GetClassNameFromClassId( wrappedClassId, wszClass ) )
    {
        ProfilerPrintf(
            "COMClassicVTableCreated: %ls  vtable: %p  IID.Data1: %08X\n",
            wszClass, pUnk, implementedIID.Data1 );
    }
    else
    {
        ProfilerPrintf( "COMClassicVTableCreated\n" );
    }

    return E_NOTIMPL;
}

HRESULT CProfilerCallback::COMClassicVTableDestroyed(ClassID wrappedClassId,
                                                    REFGUID implementedIID,
                                                    VOID * pUnk)
{
    wchar_t wszClass[512];

    if ( GetClassNameFromClassId( wrappedClassId, wszClass ) )
    {
        ProfilerPrintf(
            "COMClassicVTableDestroyed: %ls  vtable: %p  IID.Data1: %08X\n",
            wszClass, pUnk, implementedIID.Data1 );
    }
    else
    {
        ProfilerPrintf( "COMClassicVTableDestroyed\n" );
    }

    return E_NOTIMPL;
}

HRESULT CProfilerCallback::ExceptionCLRCatcherFound(void)
{
    ProfilerPrintf( "ExceptionCLRCatcherFound\n" );
    return E_NOTIMPL;
}

HRESULT CProfilerCallback::ExceptionCLRCatcherExecute(void)
{
    ProfilerPrintf( "ExceptionCLRCatcherExecute\n" );

    return E_NOTIMPL;
}

//=============================================================================
// Helper functions

bool CProfilerCallback::GetMethodNameFromFunctionId(FunctionID functionId,
                                            LPWSTR wszClass, LPWSTR wszMethod)
{
    mdToken dwToken;
    IMetaDataImport * pIMetaDataImport = 0;

    HRESULT hr = m_pICorProfilerInfo->GetTokenAndMetaDataFromFunction(
                                functionId, IID_IMetaDataImport,
                                (LPUNKNOWN *)&pIMetaDataImport, &dwToken );
    if ( FAILED(hr) ) return false;

    wchar_t _wszMethod[512];
    DWORD cchMethod = sizeof(_wszMethod)/sizeof(_wszMethod[0]);
    mdTypeDef mdClass;

    hr = pIMetaDataImport->GetMethodProps( dwToken, &mdClass, _wszMethod,
                                        cchMethod, &cchMethod, 0, 0, 0, 0, 0 );     
    if ( FAILED(hr) ) return false;

    lstrcpyW( wszMethod, _wszMethod );

    wchar_t wszTypeDef[512];
    DWORD cchTypeDef = sizeof(wszTypeDef)/sizeof(wszTypeDef[0]);

    if ( mdClass == 0x02000000 )
        mdClass = 0x02000001;

    hr = pIMetaDataImport->GetTypeDefProps( mdClass, wszTypeDef, cchTypeDef,
                                            &cchTypeDef, 0, 0 );
    if ( FAILED(hr) ) return false;

    lstrcpyW( wszClass, wszTypeDef );
    
    pIMetaDataImport->Release();

    //
    // If we were ambitious, we'd save every FunctionID away in a map to avoid
    //  needing to hit the metatdata APIs every time.
    //

    return true;
}

bool CProfilerCallback::GetClassNameFromClassId(ClassID classId,
                                                LPWSTR wszClass )
{
    ModuleID moduleId;
    mdTypeDef typeDef;

    wszClass[0] = 0;

    HRESULT hr = m_pICorProfilerInfo->GetClassIDInfo( classId, &moduleId,
                                                        &typeDef );
    if ( FAILED(hr) )
        return false;

    if ( typeDef == 0 ) // ::GetClassIDInfo can fail, yet not set HRESULT
    {
        // __asm int 3
        return false;
    }

    IMetaDataImport * pIMetaDataImport = 0;
    hr = m_pICorProfilerInfo->GetModuleMetaData( moduleId, ofRead,
                                        IID_IMetaDataImport,
                                        (LPUNKNOWN *)&pIMetaDataImport );
    if ( FAILED(hr) )
        return false;

    if ( !pIMetaDataImport )
        return false;

    wchar_t wszTypeDef[512];
    DWORD cchTypeDef = sizeof(wszTypeDef)/sizeof(wszTypeDef[0]);

    hr = pIMetaDataImport->GetTypeDefProps( typeDef, wszTypeDef, cchTypeDef,
                                            &cchTypeDef, 0, 0 );
    if ( FAILED(hr) )
        return false;

    lstrcpyW( wszClass, wszTypeDef );

    //
    // If we were ambitious, we'd save the ClassID away in a map to avoid
    //  needing to hit the metatdata APIs every time.
    //

    pIMetaDataImport->Release();

    return true;
}

int CProfilerCallback::ProfilerPrintf(char *pszFormat, ...)
{
    char szBuffer[1024];
    int retValue = 0;
    va_list argptr;
          
    va_start( argptr, pszFormat );
    retValue = vsprintf( szBuffer, pszFormat, argptr );
    va_end( argptr );

    CPerThreadData * pPerThreadData = (CPerThreadData*)TlsGetValue(m_tlsIndex);
    unsigned uNestingLevel = pPerThreadData->m_uNestingLevel;

    // Indent the appropriate amount for the current nesting level
    for ( unsigned i = 0; i < uNestingLevel; i++ )
        fprintf( m_pOutFile, "  " );

    fputs( szBuffer, m_pOutFile );

    return retValue;
}

void CProfilerCallback::ChangeNestingLevel(int nestingChange)
{
    CPerThreadData * pPerThreadData =(CPerThreadData*)TlsGetValue(m_tlsIndex);

    pPerThreadData->m_uNestingLevel += nestingChange;
}

void CProfilerCallback::GetInititializationParameters()
{
    // Get name of the EXE we're running in the context of.  We're
    // going to dump our output file in the same directory as the EXE
    GetModuleFileName( 0, m_szOutfileName, sizeof(m_szOutfileName) );

    // Overwrite the EXE name with the name of our output file
    char * pszTemp = m_szOutfileName;
    char * pszLastSlash = strrchr( pszTemp, '\\' );
    if (pszLastSlash)
      pszTemp = pszLastSlash+1;
    
    pszLastSlash = strrchr(pszTemp, '/');
    if (pszLastSlash)
      pszTemp = pszLastSlash+1;

    strcpy( pszTemp, "DNProfiler.out" );

    // assign reasonable default values to the event mask
    m_dwEventMask = COR_PRF_MONITOR_CLASS_LOADS;
    m_dwEventMask += COR_PRF_MONITOR_MODULE_LOADS;
    m_dwEventMask += COR_PRF_MONITOR_ASSEMBLY_LOADS;

    char szProfilerOptions[256];
    if ( GetEnvironmentVariable( "DN_PROFILER_MASK", szProfilerOptions,
                                    sizeof(szProfilerOptions) ) )
    {
        m_dwEventMask = strtoul( szProfilerOptions, 0, 16 );
    }

    m_bBreakOnInitialize = false;
    if ( GetEnvironmentVariable( "DN_PROFILER_BREAK", szProfilerOptions,
                                    sizeof(szProfilerOptions) ) )
    {
        if ( strtoul( szProfilerOptions, 0, 16 ) > 0 )
            m_bBreakOnInitialize = true;
    }
}
