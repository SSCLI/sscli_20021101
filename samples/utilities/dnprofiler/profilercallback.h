//==========================================
// Matt Pietrek
// MSDN Magazine, 2001
// FILE: ProfilerCallback.h
//==========================================
#include <cor.h>
#include <corprof.h>

class CProfilerCallback : public ICorProfilerCallback
{
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    STDMETHOD(QueryInterface)( REFIID riid, void **ppInterface );           

// ICorProfilerCallback
    STDMETHOD(Initialize)(IUnknown * pICorProfilerInfoUnk);
    STDMETHOD(Shutdown)();
    STDMETHOD(AppDomainCreationStarted)(UINT appDomainId);
    STDMETHOD(AppDomainCreationFinished)(UINT appDomainId, HRESULT hrStatus);
    STDMETHOD(AppDomainShutdownStarted)(UINT appDomainId);
    STDMETHOD(AppDomainShutdownFinished)(UINT appDomainId, HRESULT hrStatus);
    STDMETHOD(AssemblyLoadStarted)(UINT assemblyId);
    STDMETHOD(AssemblyLoadFinished)(UINT assemblyId, HRESULT hrStatus);
    STDMETHOD(AssemblyUnloadStarted)(UINT assemblyId);
    STDMETHOD(AssemblyUnloadFinished)(UINT assemblyId, HRESULT hrStatus);
    STDMETHOD(ModuleLoadStarted)(UINT moduleId);
    STDMETHOD(ModuleLoadFinished)(UINT moduleId, HRESULT hrStatus);
    STDMETHOD(ModuleUnloadStarted)(UINT moduleId);
    STDMETHOD(ModuleUnloadFinished)(UINT moduleId, HRESULT hrStatus);
    STDMETHOD(ModuleAttachedToAssembly)(UINT moduleId, UINT assemblyId);
    STDMETHOD(ClassLoadStarted)(UINT classId);
    STDMETHOD(ClassLoadFinished)(UINT classId, HRESULT hrStatus);
    STDMETHOD(ClassUnloadStarted)(UINT classId);
    STDMETHOD(ClassUnloadFinished)(UINT classId, HRESULT hrStatus);
    STDMETHOD(FunctionUnloadStarted)(UINT functionId);
    STDMETHOD(JITCompilationStarted)(UINT functionId, BOOL fIsSafeToBlock);
    STDMETHOD(JITCompilationFinished)(UINT functionId, HRESULT hrStatus,
                                        BOOL fIsSafeToBlock);
    STDMETHOD(JITCachedFunctionSearchStarted)(UINT functionId,
                                                BOOL * pbUseCachedFunction);
    STDMETHOD(JITCachedFunctionSearchFinished)(UINT functionId,
                                                COR_PRF_JIT_CACHE result);
    STDMETHOD(JITFunctionPitched)(UINT functionId);
    STDMETHOD(JITInlining)(UINT callerId, UINT calleeId,
                            BOOL * pfShouldInline);
    STDMETHOD(ThreadCreated)(UINT threadId);
    STDMETHOD(ThreadDestroyed)(UINT threadId);
    STDMETHOD(ThreadAssignedToOSThread)(UINT managedThreadId,
                                            ULONG osThreadId);
    STDMETHOD(RemotingClientInvocationStarted)();
    STDMETHOD(RemotingClientSendingMessage)(GUID * pCookie, BOOL fIsAsync);
    STDMETHOD(RemotingClientReceivingReply)(GUID * pCookie, BOOL fIsAsync);
    STDMETHOD(RemotingClientInvocationFinished)();
    STDMETHOD(RemotingServerReceivingMessage)(GUID * pCookie, BOOL fIsAsync);
    STDMETHOD(RemotingServerInvocationStarted)();
    STDMETHOD(RemotingServerInvocationReturned)();
    STDMETHOD(RemotingServerSendingReply)(GUID * pCookie, BOOL fIsAsync);
    STDMETHOD(UnmanagedToManagedTransition)(UINT functionId,
                                            COR_PRF_TRANSITION_REASON reason);
    STDMETHOD(ManagedToUnmanagedTransition)(UINT functionId,
                                            COR_PRF_TRANSITION_REASON reason);
    STDMETHOD(RuntimeSuspendStarted)(COR_PRF_SUSPEND_REASON suspendReason);
    STDMETHOD(RuntimeSuspendFinished)();
    STDMETHOD(RuntimeSuspendAborted)();
    STDMETHOD(RuntimeResumeStarted)();
    STDMETHOD(RuntimeResumeFinished)();
    STDMETHOD(RuntimeThreadSuspended)(UINT threadId);
    STDMETHOD(RuntimeThreadResumed)(UINT threadId);
    STDMETHOD(MovedReferences)(ULONG cMovedObjectIDRanges,
                                UINT oldObjectIDRangeStart[],
                                UINT newObjectIDRangeStart[],
                                ULONG cObjectIDRangeLength[]);
    STDMETHOD(ObjectAllocated)(UINT objectId, UINT classId);
    STDMETHOD(ObjectsAllocatedByClass)(ULONG cClassCount, UINT classIds[],
                                        ULONG cObjects[]);
    STDMETHOD(ObjectReferences)(UINT objectId, UINT classId,
                                    ULONG cObjectRefs, UINT objectRefIds[]);
    STDMETHOD(RootReferences)(ULONG cRootRefs, UINT rootRefIds[]);
    STDMETHOD(ExceptionThrown)(UINT thrownObjectId);
    STDMETHOD(ExceptionSearchFunctionEnter)(UINT functionId);
    STDMETHOD(ExceptionSearchFunctionLeave)();
    STDMETHOD(ExceptionSearchFilterEnter)(UINT functionId);
    STDMETHOD(ExceptionSearchFilterLeave)();
    STDMETHOD(ExceptionSearchCatcherFound)(UINT functionId);
    STDMETHOD(ExceptionOSHandlerEnter)(UINT functionId);
    STDMETHOD(ExceptionOSHandlerLeave)(UINT functionId);
    STDMETHOD(ExceptionUnwindFunctionEnter)(UINT functionId);
    STDMETHOD(ExceptionUnwindFunctionLeave)();
    STDMETHOD(ExceptionUnwindFinallyEnter)(UINT functionId);
    STDMETHOD(ExceptionUnwindFinallyLeave)();
    STDMETHOD(ExceptionCatcherEnter)(UINT functionId, UINT objectId);
    STDMETHOD(ExceptionCatcherLeave)();
    STDMETHOD(COMClassicVTableCreated)(ClassID wrappedClassId,
                        REFGUID implementedIID, void *pVTable, ULONG cSlots);        
    STDMETHOD(COMClassicVTableDestroyed)(ClassID wrappedClassId,
                        REFGUID implementedIID, void *pVTable);
    STDMETHOD(ExceptionCLRCatcherFound)(void);        
    STDMETHOD(ExceptionCLRCatcherExecute)(void);

// End of ICorProfilerCallback interface

private:

    void ChangeNestingLevel( signed nestingChange );
    int ProfilerPrintf( char * pszFormat, ... );
    FILE * m_pOutFile;

    struct CPerThreadData
    {
        unsigned m_uNestingLevel;           
    };
    
    DWORD m_tlsIndex;

    char m_szOutfileName[MAX_PATH];
    DWORD m_dwEventMask;
    char szOutfileName[MAX_PATH];
    void GetInititializationParameters();
    bool GetMethodNameFromFunctionId( FunctionID, LPWSTR wszClass,
                                        LPWSTR wszFunction);
    bool GetClassNameFromClassId( ClassID classID, LPWSTR wszClass );
    ICorProfilerInfo * m_pICorProfilerInfo;
    bool m_bBreakOnInitialize;
};
