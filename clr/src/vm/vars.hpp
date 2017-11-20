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
// vars.hpp
//
// Global variables
//
#ifndef _VARS_HPP
#define _VARS_HPP

// This will need ifdefs for non-x86 processors (ia64 is pointer to 128bit instructions)!
#define SLOT    PBYTE

/* Define the implementation dependent size types */

#ifndef _INTPTR_T_DEFINED
typedef int                 intptr_t;
#define _INTPTR_T_DEFINED
#endif

#ifndef _UINTPTR_T_DEFINED
typedef unsigned int        uintptr_t;
#define _UINTPTR_T_DEFINED
#endif

#ifndef _PTRDIFF_T_DEFINED
typedef int                 ptrdiff_t;
#define _PTRDIFF_T_DEFINED
#endif


#ifndef _SIZE_T_DEFINED
typedef unsigned int     size_t;
#define _SIZE_T_DEFINED
#endif


#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif

#include "util.hpp"
#include <corpriv.h>
#include <cordbpriv.h>
#include "eeprofinterfaces.h"
#include "profilepriv.h"
#include "eehash.h"

class ClassLoader;
class LoaderHeap;
class GCHeap;
class Object;
class Object;
class StringObject;
class EEClass;
class ArrayClass;
class MethodTable;
class SyncBlockCache;
class SyncTableEntry;
class ThreadStore;
class IPCWriterInterface;
class DebugInterface;
class EEDbgInterfaceImpl;
class EECodeManager;
class Crst;
class MethodNameCache;


//
// object handles are opaque types that track object pointers
//
DECLARE_HANDLE(OBJECTHANDLE);


//
// _UNCHECKED_OBJECTREF is for code that can't deal with DEBUG OBJECTREFs
//
typedef Object * _UNCHECKED_OBJECTREF;

#if defined(_DEBUG) // && !defined(_IA64_)
#define USE_CHECKED_OBJECTREFS
#else
#undef  USE_CHECKED_OBJECTREFS
#endif

#ifndef DEFINE_OBJECTREF
#define DEFINE_OBJECTREF
#ifdef USE_CHECKED_OBJECTREFS
class OBJECTREF;
#else
class Object;
typedef Object *        OBJECTREF;
#endif
#endif


#ifdef USE_CHECKED_OBJECTREFS


//=========================================================================
// In the retail build, OBJECTREF is typedef'd to "Object*".
// In the debug build, we use operator overloading to detect
// common programming mistakes that create GC holes. The critical
// rules are:
//
//   1. Your thread must have disabled preemptive GC before
//      reading or writing any OBJECTREF. When preemptive GC is enabled,
//      another other thread can suspend you at any time and
//      move or discard objects.
//   2. You must guard your OBJECTREF's using a root pointer across
//      any code that might trigger a GC.
//
// Each of the overloads validate that:
//
//   1. Preemptive GC is currently disabled
//   2. The object looks consistent (checked by comparing the
//      object's methodtable pointer with that of the class.)
//
// Limitations:
//    - Can't say
//
//          if (or) {}
//
//      must say
//
//          if (or != NULL) {}
//
//
//=========================================================================
class OBJECTREF {
    private:
        // Holds the real object pointer.
        // The union gives us better debugger pretty printing
    union {
        Object *m_asObj;
        class StringObject* m_asString;
        class ArrayBase* m_asArray;
        class PtrArray* m_asPtrArray;
    };

    public:
        //-------------------------------------------------------------
        // Default constructor, for non-initializing declarations:
        //
        //      OBJECTREF or;
        //-------------------------------------------------------------
        OBJECTREF();

        //-------------------------------------------------------------
        // Copy constructor, for passing OBJECTREF's as function arguments.
        //-------------------------------------------------------------
        OBJECTREF(const OBJECTREF & objref);

        //-------------------------------------------------------------
        // To allow NULL to be used as an OBJECTREF. 
        //-------------------------------------------------------------
        OBJECTREF(size_t nul);

        //-------------------------------------------------------------
        // Test against NULL.
        //-------------------------------------------------------------
        int operator!() const;

        //-------------------------------------------------------------
        // Compare two OBJECTREF's.
        //-------------------------------------------------------------
        int operator==(const OBJECTREF &objref) const;

        //-------------------------------------------------------------
        // Compare two OBJECTREF's.
        //-------------------------------------------------------------
        int operator!=(const OBJECTREF &objref) const;

        //-------------------------------------------------------------
        // Forward method calls.
        //-------------------------------------------------------------
        Object* operator->();
        const Object* operator->() const;

        //-------------------------------------------------------------
        // Assignment. We don't validate the destination so as not
        // to break the sequence:
        //
        //      OBJECTREF or;
        //      or = ...;
        //-------------------------------------------------------------
        OBJECTREF& operator=(const OBJECTREF &objref);
        OBJECTREF& operator=(int nul);

            // allow explict casts
        explicit OBJECTREF(Object *pObject);
};

//-------------------------------------------------------------
//  template class REF for different types of REF class to be used 
//  in the debug mode
//  Template type should be a class that extends Object
//-------------------------------------------------------------



template <class T> 
class REF : public OBJECTREF
{
    public:
        
        //-------------------------------------------------------------
        // Default constructor, for non-initializing declarations:
        //
        //      OBJECTREF or;
        //-------------------------------------------------------------
      REF() :OBJECTREF ()
        { 
            // no op
        }

        //-------------------------------------------------------------
        // Copy constructor, for passing OBJECTREF's as function arguments.
        //-------------------------------------------------------------
      explicit REF(const OBJECTREF& objref) : OBJECTREF(objref)
        {
            //no op
        }


        //-------------------------------------------------------------
        // To allow NULL to be used as an OBJECTREF.
        //-------------------------------------------------------------
      REF(size_t nul) : OBJECTREF (nul)
        {
            // no op
        }

      explicit REF(T* pObject) : OBJECTREF(pObject) 
        { 
            // no op
        }
        
        //-------------------------------------------------------------
        // Forward method calls.
        //-------------------------------------------------------------
        T* operator->()
        {
            return (T *)OBJECTREF::operator->();
        }

        const T* operator->() const
        {
            return (const T *)OBJECTREF::operator->();
        }

        //-------------------------------------------------------------
        // Assignment. We don't validate the destination so as not
        // to break the sequence:
        //
        //      OBJECTREF or;
        //      or = ...;
        //-------------------------------------------------------------
        REF<T> &operator=(OBJECTREF &objref)
        {
            return (REF<T>&)OBJECTREF::operator=(objref);
        }

};

#define VALIDATEOBJECTREF(objref) ((objref)->Validate())

#define ObjectToOBJECTREF(obj)     (OBJECTREF(obj))
#define OBJECTREFToObject(objref)  ((Object*&) (objref))
#define ObjectToSTRINGREF(obj)     (STRINGREF(obj))

#else   //_DEBUG

typedef Object *        OBJECTREF;

#define VALIDATEOBJECTREF(objref)

#define ObjectToOBJECTREF(obj)    ((Object*) (obj))
#define OBJECTREFToObject(objref) ((Object*) (objref))
#define ObjectToSTRINGREF(obj)    ((StringObject*) (obj))

#endif //_DEBUG


// EEClass Needed Defines 
#define MAX_CLASSNAME_LENGTH    1024
#define MAX_NAMESPACE_LENGTH    1024

class EEConfig;
class ClassLoaderList;
class Module;
class ArrayTypeDesc;

#define EXTERN extern

// For [<I1, etc. up to and including [Object
EXTERN ArrayTypeDesc *     g_pPredefinedArrayTypes[ELEMENT_TYPE_MAX];
EXTERN HINSTANCE            g_pMSCorEE;
EXTERN GCHeap *             g_pGCHeap;
EXTERN ThreadStore *        g_pThreadStore;
EXTERN LONG                 g_TrapReturningThreads;
#ifdef _DEBUG
EXTERN bool                 g_TrapReturningThreadsPoisoned;
EXTERN char *               g_ExceptionFile;
EXTERN DWORD                g_ExceptionLine;
EXTERN void *               g_ExceptionEIP;
#endif
EXTERN EEConfig*            g_pConfig;          // configuration data (from the registry)
EXTERN LONG                 g_RefCount;
EXTERN MethodTable *        g_pObjectClass;
EXTERN MethodTable *        g_pStringClass;
EXTERN MethodTable *        g_pByteArrayClass;
EXTERN MethodTable *        g_pArrayClass;
EXTERN MethodTable *        g_pExceptionClass;
EXTERN MethodTable *        g_pThreadStopExceptionClass;
EXTERN MethodTable *        g_pThreadAbortExceptionClass;
EXTERN MethodTable *        g_pOutOfMemoryExceptionClass;
EXTERN MethodTable *        g_pStackOverflowExceptionClass;
EXTERN MethodTable *        g_pExecutionEngineExceptionClass;
EXTERN MethodTable *        g_pDelegateClass;
EXTERN MethodTable *        g_pMultiDelegateClass;
EXTERN MethodTable *        g_pFreeObjectMethodTable;
EXTERN MethodTable *        g_pValueTypeClass;
EXTERN MethodTable *        g_pEnumClass;
EXTERN MethodTable *        g_pThreadClass;

MethodTable *        TheSByteClass();
MethodTable *        TheInt16Class();
MethodTable *        TheInt32Class();

MethodTable *        TheByteClass();
MethodTable *        TheUInt16Class();
MethodTable *        TheUInt32Class();

MethodTable *        TheBooleanClass();
MethodTable *        TheSingleClass();
MethodTable *        TheDoubleClass();

MethodTable *        TheIntPtrClass();
MethodTable *        TheUIntPtrClass();

EXTERN bool                 g_fVerifierOff;

// Global System Information
extern HANDLE      g_hProcessHeap;
extern SYSTEM_INFO g_SystemInfo;

extern bool        g_SystemLoad; // Indicates the system class libraries are loading

EXTERN OBJECTHANDLE         g_pPreallocatedOutOfMemoryException;
EXTERN OBJECTHANDLE         g_pPreallocatedStackOverflowException;
EXTERN OBJECTHANDLE         g_pPreallocatedExecutionEngineException;

// Global SyncBlock cache
EXTERN SyncTableEntry *     g_pSyncTable;


// support for IPCManager 
EXTERN IPCWriterInterface*  g_pIPCManagerInterface;

#ifdef DEBUGGING_SUPPORTED
//
// Support for the COM+ Debugger.
//
EXTERN DebugInterface *     g_pDebugInterface;
EXTERN EEDbgInterfaceImpl*  g_pEEDbgInterfaceImpl;
EXTERN DWORD                g_CORDebuggerControlFlags;
EXTERN HINSTANCE            g_pDebuggerDll;
#endif // DEBUGGING_SUPPORTED

// Global default for Concurrent GC. The default is on (value 1)
EXTERN int g_IGCconcurrent;
//
// Can we run managed code?
//
bool CanRunManagedCode(void);

//
// Global state variable indicating if the EE is in its init phase.
//
EXTERN bool g_fEEInit;

extern BOOL g_fEEStarted;

//
// Global state variables indicating which stage of shutdown we are in
//
EXTERN DWORD g_fEEShutDown;
extern BOOL g_fSuspendOnShutdown;
EXTERN bool g_fForbidEnterEE;
EXTERN bool g_fFinalizerRunOnShutDown;
EXTERN bool g_fProcessDetach;
EXTERN bool g_fManagedAttach;
EXTERN bool g_fNoExceptions;
EXTERN bool g_fFatalError;

//
// Global state variable indicating that thread store lock requirements may be relaxed in certian special places.
//
EXTERN bool g_fRelaxTSLRequirement;

//
// Default install library
//
EXTERN const WCHAR g_pwBaseLibrary[];
EXTERN const WCHAR g_pwBaseLibraryName[];
EXTERN const char g_psBaseLibrary[];
EXTERN const char g_psBaseLibraryName[];


EXTERN DWORD g_dwGlobalSharePolicy;

//
// Do we own the lifetime of the process, ie. is it an EXE?
//
EXTERN bool g_fWeControlLifetime;

#ifdef _DEBUG
// The following should only be used for assertions.  (Famous last words).
EXTERN bool dbg_fDrasticShutdown;
#endif
EXTERN bool g_fInControlC;

EXTERN BOOL                 g_fExceptionsOK;

// There is a global table of prime numbers that's available for e.g. hashing
extern const DWORD g_rgPrimes[70];

// A hash of all function type descs on the system (to maintain type desc
// identity).
extern EEFuncTypeDescHashTable g_sFuncTypeDescHash;
extern CRITICAL_SECTION g_sFuncTypeDescHashLock;

//
// Cached command line file provided by the host.
//
extern LPWSTR g_pCachedCommandLine;
extern LPWSTR g_pCachedModuleFileName;

//
// Host configuration file. One per process.
//
extern LPCWSTR g_pszHostConfigFile;
extern DWORD   g_dwHostConfigFile;

#ifdef DEBUGGING_SUPPORTED


//
// Macros to check debugger and profiler settings.
//
#define CORDebuggerAttached() (g_CORDebuggerControlFlags & DBCF_ATTACHED)

#define CORDebuggerTrackJITInfo(dwDebuggerBits)                    \
    (((dwDebuggerBits) & DACF_TRACK_JIT_INFO)                      \
     ||                                                            \
     ((g_CORDebuggerControlFlags & DBCF_GENERATE_DEBUG_CODE) &&    \
      !((dwDebuggerBits) & DACF_USER_OVERRIDE)))
     
#define CORDebuggerAllowJITOpts(dwDebuggerBits)                    \
    (((dwDebuggerBits) & DACF_ALLOW_JIT_OPTS)                      \
     ||                                                            \
     ((g_CORDebuggerControlFlags & DBCF_ALLOW_JIT_OPT) &&          \
      !((dwDebuggerBits) & DACF_USER_OVERRIDE)))
    
#define CORDebuggerEnCMode(dwDebuggerBits)                         \
    ((dwDebuggerBits) & DACF_ENC_ENABLED)
     
#define CORDebuggerTraceCall() \
    (CORDebuggerAttached() && GetThread()->IsTraceCall())

#define CORActivateRemoteDebugging()    \
    (g_CORDebuggerControlFlags & DBCF_ACTIVATE_REMOTE_DEBUGGING)

#define CORLaunchedByDebugger() \
    (g_CORDebuggerControlFlags & DBCF_GENERATE_DEBUG_CODE)

#else

#define CORDebuggerEnCMode(dwDebuggerBits) (0)

#endif // DEBUGGING_SUPPORTED

        
#define DEFINE_METASIG(varname, sig)                                extern HardCodedMetaSig gsig_ ## varname;
#include "metasig.h"




#define NEWCALLINGCONVENTION 1
#ifdef NEWCALLINGCONVENTION

#define NEEDSCALLINGCONVENTIONWORK(funcname) _ASSERTE(!(#funcname ": This code path hasn't been updated to work with alternate calling conventions."))

#else

#define NEEDSCALLINGCONVENTIONWORK(funcname)

#endif


//                     Undefine this once our native calling convention follows VC style
// of returned values and passed arguments containing garbage in unused
// high bits.
#define WRONGCALLINGCONVENTIONHACK

// This tests for valid variants coming in from COM on debug builds.
#if defined(_DEBUG)
#define CHECK_FOR_VALID_VARIANTS
#endif


#endif /* _VARS_HPP */
