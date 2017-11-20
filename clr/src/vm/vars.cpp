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
//
// vars.cpp - Global Var definitions
//

#include "common.h"
#include "vars.hpp"
#include "cordbpriv.h"
#include "eeprofinterfaces.h"


//
// Default install library
//
const WCHAR g_pwBaseLibrary[]     = L"mscorlib.dll";
const WCHAR g_pwBaseLibraryName[] = L"mscorlib";
const char g_psBaseLibrary[]      = "mscorlib.dll";
const char g_psBaseLibraryName[]  = "mscorlib";


//---------------------------------------------------------
// IMPORTANT:
// If you add a global variable you must clean it up in 
// CoUninitializeEE and set it back to NULL:


// For [<I1, etc. up to and including [Object
ArrayTypeDesc *      g_pPredefinedArrayTypes[ELEMENT_TYPE_MAX];
HINSTANCE            g_pMSCorEE;
GCHeap *             g_pGCHeap;
ThreadStore *        g_pThreadStore;
LONG                 g_TrapReturningThreads;
#ifdef _DEBUG
bool                 g_TrapReturningThreadsPoisoned;

char *               g_ExceptionFile;   // Source of the last thrown exception (COMPLUSThrow())
DWORD                g_ExceptionLine;   // ... ditto ...
void *               g_ExceptionEIP;    // Managed EIP of the last guy to call JITThrow.

#endif
EEConfig*            g_pConfig = NULL;          // configuration data (from the registry)

MethodTable *        g_pObjectClass;
MethodTable *        g_pStringClass;
MethodTable *        g_pByteArrayClass;
MethodTable *        g_pArrayClass;
MethodTable *        g_pExceptionClass;
MethodTable *        g_pThreadStopExceptionClass;
MethodTable *        g_pThreadAbortExceptionClass;
MethodTable *        g_pOutOfMemoryExceptionClass;
MethodTable *        g_pStackOverflowExceptionClass;
MethodTable *        g_pExecutionEngineExceptionClass;
MethodTable *        g_pDateClass;
MethodTable *        g_pDelegateClass;
MethodTable *        g_pMultiDelegateClass;
MethodTable *        g_pValueTypeClass;
MethodTable *        g_pEnumClass;
MethodTable *        g_pSharedStaticsClass;
MethodTable *        g_pThreadClass;

//                                          determines whether the verifier throws an exception when something fails
bool                g_fVerifierOff;

OBJECTHANDLE         g_pPreallocatedOutOfMemoryException;
OBJECTHANDLE         g_pPreallocatedStackOverflowException;
OBJECTHANDLE         g_pPreallocatedExecutionEngineException;

MethodTable *        g_pFreeObjectMethodTable;
// Global SyncBlock cache

SyncTableEntry *     g_pSyncTable;


// 
//
// Global System Info
//
SYSTEM_INFO g_SystemInfo;
HANDLE      g_hProcessHeap;

bool        g_SystemLoad;               // Indicates the system class libraries are loading
LONG        g_RefCount = 0;             // Global counter for EE initialization.


// support for IPCManager 
IPCWriterInterface* g_pIPCManagerInterface = NULL;

#ifdef DEBUGGING_SUPPORTED
//
// Support for the COM+ Debugger.
//
DebugInterface *     g_pDebugInterface            = NULL;
EEDbgInterfaceImpl*  g_pEEDbgInterfaceImpl        = NULL;
DWORD                g_CORDebuggerControlFlags    = 0;
HINSTANCE            g_pDebuggerDll               = NULL;
#endif // DEBUGGING_SUPPORTED

#ifdef PROFILING_SUPPORTED
// Profiling support
ProfilerStatus      g_profStatus = profNone;
ProfControlBlock    g_profControlBlock;
#endif // PROFILING_SUPPORTED


// Global default for Concurrent GC. The default is value is 1
int g_IGCconcurrent = 1;

//
// Global state variable indicating if the EE is in its init phase.
//
bool g_fEEInit = false;

//
// Two globals that are used as dummy placements for new(throws) and new(nothrow)
//
const Throws throws = { 0 };
const NoThrow nothrow = { 0 };

//
// Global state variables indicating which stage of shutdown we are in
//
DWORD g_fEEShutDown = 0;
bool g_fForbidEnterEE = false;
bool g_fFinalizerRunOnShutDown = false;
bool g_fProcessDetach = false;
bool g_fManagedAttach = false;
bool g_fNoExceptions = false;
bool g_fFatalError = false;

//
// Global state variable indicating that thread store lock requirements may be relaxed in certian special places.
//
bool g_fRelaxTSLRequirement = false;

DWORD g_dwGlobalSharePolicy = BaseDomain::SHARE_POLICY_UNSPECIFIED;

//
// Do we own the lifetime of the process, ie. is it an EXE?
//
bool g_fWeControlLifetime = false;

#ifdef _DEBUG
// The following should only be used for assertions.  (Famous last words).
bool dbg_fDrasticShutdown = false;
#endif
bool g_fInControlC = false;

// A hash of all function type descs on the system (to maintain type desc
// identity).
EEFuncTypeDescHashTable g_sFuncTypeDescHash;
CRITICAL_SECTION g_sFuncTypeDescHashLock;


//
// Cached command line file provided by the host.
//
LPWSTR g_pCachedCommandLine = NULL;
LPWSTR g_pCachedModuleFileName = 0;

// host configuration file. If set, it is added to every AppDomain (fusion context)
LPCWSTR g_pszHostConfigFile = NULL;
DWORD   g_dwHostConfigFile = 0;

//
// Meta-Sig
//
#define DEFINE_METASIG(varname, sig)
#define DEFINE_METASIG_PARAMS_1(varname, p1)                     static const USHORT gparams_ ## varname [1] = { CLASS__ ## p1 };
#define DEFINE_METASIG_PARAMS_2(varname, p1, p2)                 static const USHORT gparams_ ## varname [2] = { CLASS__ ## p1, CLASS__ ## p2 };
#define DEFINE_METASIG_PARAMS_3(varname, p1, p2, p3)             static const USHORT gparams_ ## varname [3] = { CLASS__ ## p1, CLASS__ ## p2, CLASS__ ## p3 };
#define DEFINE_METASIG_PARAMS_4(varname, p1, p2, p3, p4)         static const USHORT gparams_ ## varname [4] = { CLASS__ ## p1, CLASS__ ## p2, CLASS__ ## p3, CLASS__ ## p4 };
#define DEFINE_METASIG_PARAMS_5(varname, p1, p2, p3, p4, p5)     static const USHORT gparams_ ## varname [5] = { CLASS__ ## p1, CLASS__ ## p2, CLASS__ ## p3, CLASS__ ## p4, CLASS__ ## p5 };
#define DEFINE_METASIG_PARAMS_6(varname, p1, p2, p3, p4, p5, p6) static const USHORT gparams_ ## varname [6] = { CLASS__ ## p1, CLASS__ ## p2, CLASS__ ## p3, CLASS__ ## p4, CLASS__ ## p5, CLASS__ ## p6 };

#include "metasig.h"

#define DEFINE_METASIG(varname, sig)                             HardCodedMetaSig gsig_ ## varname = { sig, NULL, NULL, 0};
#define DEFINE_METASIG_T(varname, sig, params)                   HardCodedMetaSig gsig_ ## varname = { sig, gparams_ ## params, NULL, 0};

#include "metasig.h"



//----------------------------------------------------------
// Huge macro to reinitialize all the global binary sigs
//----------------------------------------------------------

#include "version/corver.ver"
char g_Version[] = VER_PRODUCTVERSION_STR;
