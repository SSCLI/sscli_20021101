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
// File: compiler.cpp
//
// Defined the main compiler class.
// ===========================================================================

#include "stdafx.h"

#include "compiler.h"
#include "nodes.h"

#include "privguid.h"
#include "controller.h"
#include "fileiter.h"
#include <mscoree.h>

#if PLATFORM_UNIX
#define ENVIRONMENT_SEPARATOR L':'
#else   // PLATFORM_UNIX
#define ENVIRONMENT_SEPARATOR L';'
#endif  // PLATFORM_UNIX

#define ALINK_NAME_A      MAKEDLLNAME_A("alink")
#define ALINK_NAME_W      MAKEDLLNAME_W(L"alink")

#ifdef TRACKMEM
// Use to dump memory heap statistics
#define DUMPGLOBALHEAP(text)    if (options.trackMem) { \
    wprintf( L"Compiler Heap-%s\t%u\t%u\n", text, globalHeap.GetCurrentSize(), globalHeap.GetMaxSize()); };

#define DUMPNAMEHEAP(text)      if (options.trackMem) { \
    wprintf( L"Name Heap-%s\t%u\t%u\t%u\t%u\n", text, namemgr->GetPageHeap()->GetCurrentUseSize(), \
        namemgr->GetPageHeap()->GetMaxUseSize(), namemgr->GetPageHeap()->GetCurrentReserveSize(), \
        namemgr->GetPageHeap()->GetMaxReserveSize()); };

#define DUMPPAGEHEAP(text)      if (options.trackMem) { \
    wprintf( L"Page Heap-%s\t%u\t%u\t%u\t%u\n", text, pageheap.GetCurrentUseSize(), \
        pageheap.GetMaxUseSize(), pageheap.GetCurrentReserveSize(), pageheap.GetMaxReserveSize()); };

#define DUMPALLHEAPS(text)      if (options.trackMem) { \
    wprintf( L"Compiler Heap-%s\t%u\t%u\n", text, globalHeap.GetCurrentSize(), globalHeap.GetMaxSize()); \
    wprintf( L"Name Std Heap-%s\t%u\t%u\n", text, namemgr->GetStandardHeap()->GetCurrentSize(), namemgr->GetStandardHeap()->GetMaxSize()); \
    wprintf( L"Controller Std Heap-%s\t%u\t%u\n", text, pController->GetStandardHeap()->GetCurrentSize(), pController->GetStandardHeap()->GetMaxSize()); \
    wprintf( L"Name Heap-%s\t%u\t%u\t%u\t%u\n", text, namemgr->GetPageHeap()->GetCurrentUseSize(), \
        namemgr->GetPageHeap()->GetMaxUseSize(), namemgr->GetPageHeap()->GetCurrentReserveSize(), \
        namemgr->GetPageHeap()->GetMaxReserveSize()); \
    wprintf( L"Page Heap-%s\t%u\t%u\t%u\t%u\n", text, pageheap.GetCurrentUseSize(), \
        pageheap.GetMaxUseSize(), pageheap.GetCurrentReserveSize(), pageheap.GetMaxReserveSize()); };

#else

#define DUMPGLOBALHEAP(text)    ;
#define DUMPNAMEHEAP(text)      ;
#define DUMPPAGEHEAP(text)      ;
#define DUMPALLHEAPS(text)      ;

#endif // DEBUG

// Routine to load a helper DLL from a particular search path.
// We search the following path:
//   1. First, the directory where the compiler DLL is.
//   2. Second, the shim DLL.
//   3. The normal DLL search path.

HMODULE FindAndLoadHelperLibrary(LPCSTR filename)
{
    CHAR path[MAX_PATH];
    CHAR * pEnd;
    HMODULE hInstance;

    // 1. The directory where the compiler DLL is.
    if (GetModuleFileName(hModuleMine, path, lengthof(path))) {
        pEnd = strrchr(path, '\\');
#if PLATFORM_UNIX
        CHAR *pEndSlash = strrchr(path, '/');
        if (pEndSlash > pEnd)
        {
            pEnd = pEndSlash;
        }
#endif  // PLATFORM_UNIX
        if (pEnd && strlen(filename) + pEnd - path < (int) lengthof(path)) {
            ++pEnd;  // point just beyond.

            // Append new file
            strcpy(pEnd, filename);

            // Try to load it.
            if ((hInstance = LoadLibrary(path)))
                return hInstance;
        }
    }

    // 2. The shim DLL.
    WCHAR wpath[MAX_PATH];
    if (MultiByteToWideChar(CP_ACP, 0, filename, -1, wpath, lengthof(wpath))) {
        HRESULT hr = LoadLibraryShim(wpath, NULL, NULL, &hInstance);
        if (SUCCEEDED(hr) && hInstance)
            return hInstance;
    }

    // 3. Normal DLL search path.
    return LoadLibrary(filename);
}


/*
 * The COMPILER class is the only class that has
 * operator new and operator delete operator that
 * allocate from the global heaps. All other
 * classes allocate from heaps local to each instance
 * of COMPILER.
 */

void * COMPILER::operator new(size_t size)
{
    return VSAlloc(size);
}

void COMPILER::operator delete(void * p)
{
    VSFree(p);
}

/* Construct a compiler. All the real work
 * is done in the Init() routine. This primary initializes
 * all the sub-components.
 */
#if defined(_MSC_VER)
#pragma warning(disable:4355)  // allow "this" in member initializer list
#endif  // defined(_MSC_VER)

COMPILER::COMPILER(CController *pCtrl, PAGEHEAP &ph, NAMEMGR *pNameMgr)
    :options(*pCtrl->GetOptions()),
     pController(pCtrl),
     pageheap(ph),
     globalHeap(this, true),
     globalSymAlloc(this),
     localSymAlloc(this),
     symmgr(&globalSymAlloc, &localSymAlloc),
     ccsymbols(this),
     assemblyAttributes(0),
     cInputFiles(0),
     cOutputFiles(0),
     location(NULL),
#ifdef TRACKMEM
     uParseMemory(0),
#endif
     inputFilesById(0),
#ifdef DEBUG
     tests(this),
#endif //DEBUG
     incrementalRebuild(false),
     cChangedFiles(0),
     changedFiles(0),
     checkCLS(true),
     m_bAssemble(false),
     corSystemDirectory(0),
     pfnCreateCeeFileGen(0),
     pfnDestroyCeeFileGen(0)
{
    isInited = 0;   // not yet initialized.
    cRef = 0;
    //cError = cWarn = 0;
    errBufferNext = errBuffer;
    dispenser = NULL;
    host = pCtrl->GetHost();
    host->QueryInterface(IID_ICSCommandLineCompilerHost, (void**)&cmdHost);
    namemgr = pNameMgr;
    m_pszLibPath = NULL;

    ASSERT(!this->IsIncrementalRebuild());
}

#if defined(_MSC_VER)
#pragma warning(default:4355)
#endif  // defined(_MSC_VER)

/* Destruct the compiler. Make sure we've
 * been deinitialized.
 */
COMPILER::~COMPILER()
{
    if (isInited > 0) {
        //ASSERT(0);      // We should have been terminated by now.
        Term(false);
    }
}

/* Initialize the compiler. This does the heavy lifting of
 * setting up the memory managed, and so forth.
 */
HRESULT COMPILER::Init ()
{
    if (isInited > 0)
    {
        // This should never happen
        VSFAIL ("Compiler initialization called twice?");
        return E_FAIL;
    }

    HRESULT hr = S_OK;
    DWORD dwExceptionCode;

	PAL_TRY
    {
        COMPILER_EXCEPTION_TRYEX
        {
            hr = InitWorker();
        }
        COMPILER_EXCEPTION_EXCEPT_FILTER_EX(PAL_Except1, &dwExceptionCode)
        {
            pController->GetHost()->OnCatastrophicError (TRUE, dwExceptionCode, pController->GetExceptionAddress());
            hr = E_UNEXPECTED;
        }
        COMPILER_EXCEPTION_ENDTRY
    }
    EXCEPT_FILTER_EX(PAL_Except2, &dwExceptionCode)
    {
        if (dwExceptionCode == FATAL_EXCEPTION_CODE)
            hr = E_FAIL;
        else
            hr = E_UNEXPECTED;
    }
    PAL_ENDTRY
    return hr;
}

HRESULT COMPILER::InitWorker ()
{
    HRESULT hr = S_OK;

    ASSERT (globalHeap.GetMaxSize() == 0);
    globalHeap.SetMemTrack(options.trackMem);
    DUMPPAGEHEAP( L"Compiler::Init");
    pageheap.SetMemTrack(options.trackMem);

    // Set up all the options

    //
    // start assigning assembly indexes at 1 for imported assemblies
    // 0 is for the assembly being generated
    //
    currentAssemblyIndex = 1;
    isInited++;

    /*

    // Make sure the provided name table is our own implementation.  Do
    // this by using a HACK in its QueryInterface implementation, which
    // responds to IID_ISCPrivateNameTable and stores the NAMEMGR pointer
    // in the out param.
    if (FAILED (hr = pNameTable->QueryInterface (IID_ISCPrivateNameTable, (void **)&namemgr)))
        return hr;

    // Remember the host
    host = pHost;
    host->AddRef();
    */

    // Load ALINK.DLL and initialize it. Do this in a late bound way so
    // we load the correct copy via the shim.
    HMODULE  hinstALINK = NULL;
    linker = NULL;
    hinstALINK = FindAndLoadHelperLibrary(ALINK_NAME_A);
    if (hinstALINK != NULL) {
        HRESULT (WINAPI * pfnCreateALink)(REFIID, IUnknown**);
        pfnCreateALink = (HRESULT (WINAPI *)(REFIID, IUnknown**))GetProcAddress(hinstALINK, "CreateALink");
        if (pfnCreateALink) {
            if (FAILED(hr = (*pfnCreateALink)(IID_IALink, (IUnknown**)&linker))) 
                return hr;
            isInited++;
        }
    }
    if (linker == NULL) {
        if (hinstALINK)
            FreeLibrary(hinstALINK);
        Error(NULL, FTL_RequiredFileNotFound, ALINK_NAME_W);
        hr = HRESULT_FROM_WIN32(GetLastError());
        return hr;
    }
    // Do any initialization necessary.
    //cError = cWarn = 0;
    // Start by Reserving a large chunk of memory for error buffers
    // but we only commit 1 page now, and then commit new pages as needed
    if (NULL == (errBufferNextPage = (BYTE*)VirtualAlloc(NULL, ERROR_BUFFER_MAX_BYTES, MEM_RESERVE, PAGE_NOACCESS))) {
        Error(NULL, FTL_NoMemory);
        return HRESULT_FROM_WIN32(GetLastError());
    }
    if (NULL == (errBuffer = (WCHAR*)VirtualAlloc( errBufferNextPage, pageheap.pageSize, MEM_COMMIT, PAGE_READWRITE))) {
        Error(NULL, FTL_NoMemory);
        return HRESULT_FROM_WIN32(GetLastError());
    }
    errBufferNextPage += pageheap.pageSize;
    errBufferNext = errBuffer;
    W_IsUnicodeSystem();    // Use this to Init Unilib stuff
    isInited++;

    // Initialize global symbols that we always use.
    symmgr.Init();
    isInited++;
    importer.Init();
    isInited++;

#ifdef DEBUG
    // Make sure the error message info is consistent.
    CheckErrorMessageInfo(&globalHeap, options.showAllErrors);
#endif //DEBUG

    // Add each of the imports (must be done after initialization of symmgr...)
    if (options.m_sbstrIMPORTS != NULL)
    {
        PWSTR       buffer = (PWSTR)_alloca((options.m_sbstrIMPORTS.Length()+1) * sizeof(WCHAR));
        wcscpy(buffer, options.m_sbstrIMPORTS);
        PWSTR       pszTok = wcstok (buffer, L"|");  // imports are pipe seperated
        HRESULT     hr;

        while (pszTok != NULL)
        {
            if (FAILED (hr = AddImport (pszTok, currentAssemblyIndex++)))
                return hr;
            pszTok = wcstok (NULL, L"|");
        }
    }
    isInited++;

    // Add each of the addmodules
    if (options.m_sbstrMODULES != NULL)
    {
        PWSTR       buffer = (PWSTR)_alloca((options.m_sbstrMODULES.Length()+1) * sizeof(WCHAR));
        wcscpy(buffer, options.m_sbstrMODULES);
        PWSTR       pszTok = wcstok (buffer, L"|");
        HRESULT     hr;

        while (pszTok != NULL)
        {
            if (FAILED (hr = AddImport (pszTok, 0)))
                return hr;
            pszTok = wcstok (NULL, L"|");
        }
    }
    isInited++;


    ASSERT(isInited == 7); // Make sure we got fully initialized

    return hr;
}

/*
 * Terminate the compiler. Terminates all subsystems and
 * frees all allocated memory.
 */
void COMPILER::Term(bool normalTerm)
{
    if (isInited == 0)
        return;     // nothing to do.

    int InitLevel = isInited;
    isInited = 0; // set this to 0 to prevent recallign Term()

    // Terminate everything. Check leaks on a normal termination.

    switch(InitLevel) {
    case 7: // we did a full Init(), so do a full Term()
    {
        //
        // free metadata import interfaces for incremental rebuild
        //
        SourceOutFileIterator outfiles;
        for (outfiles.Reset(this); outfiles.Current(); outfiles.Next()) {
            if (outfiles.Current()->metaimport) {

                outfiles.Current()->metaimport->Release();
                outfiles.Current()->metaimport = NULL;
            }
        }

        emitter.Term();
        if (BuildAssembly())
            assemFile.Term();
    }

    case 6: case 5:
        if (m_pszLibPath != NULL) {
            globalHeap.Free(m_pszLibPath);
            m_pszLibPath = NULL;
        }
    
        if (dispenser) {
            dispenser->Release();
            dispenser = NULL;
        }

    case 4:
        // We didn't finish Init(), so only cleanup a few things
        importer.Term();
    
    case 3:
        // We didn't finish Init(), so only cleanup a few things
        symmgr.Term();

    case 2:
        // We didn't finish Init(), so only cleanup a few things
        ASSERT(linker != NULL);
        if (linker)
            linker->Release();
    }

    // Dump the global symbol info
#ifdef TRACKMEM
    if (options.trackMem) {

		unsigned int total = 0;
		unsigned int temp;
#define SYMBOLDEF(kind) temp = 0; wprintf( L"SK_%-24s\t%u\t%u\t%u\t%u\n", L#kind, sizeof( kind), symmgr.GetSymCount(SK_ ## kind, false), symmgr.GetSymCount(SK_ ## kind, true), (((temp = sizeof(kind) * symmgr.GetSymCount(SK_ ## kind, true)), (total += temp)), temp));
    #include "symkinds.h"
#undef SYMBOLDEF
		wprintf (L"Total : %u\n", total);
    }

#endif

    ccsymbols.ClearAll(TRUE);
    localSymAlloc.FreeHeap();
    globalSymAlloc.FreeHeap();
    VirtualFree(errBuffer, 0, MEM_DECOMMIT | MEM_FREE);
    DUMPALLHEAPS( L"Compiler::Term");
#ifdef TRACKMEM
    if (options.trackMem) {
        wprintf(L"Parse memory: %u\n", uParseMemory);
    }
#endif
    globalHeap.FreeHeap(normalTerm);

    //
    // free link to mscorpe.dll
    //
    pfnCreateCeeFileGen = NULL;
    pfnDestroyCeeFileGen = NULL;
    if (hmodCorPE) {
        FreeLibrary(hmodCorPE);
        hmodCorPE = 0;
    }

    // Done with everything, release the host.
    if (host) {
        //host->Release();
        host = NULL;
    }

    isInited = 0;
}

/*
 * Get the path to search for imports
 * = Current Directory, CORSystemDir, /LIB otpion, %LIB%
 */
LPCWSTR COMPILER::GetSearchPath()
{
    if (m_pszLibPath == NULL)
    {
        // Setup the Lib Search path
        LPWSTR pszTemp;
        DWORD dwLen;
        size_t len = 0, newLen;
        LPWSTR pszEnv;

        WCHAR EnvBuffer[_MAX_PATH];
        
        dwLen= GetEnvironmentVariableW(L"LIB", 
		  		       EnvBuffer,
				       sizeof(EnvBuffer)/sizeof(EnvBuffer[0]));
	if (dwLen && dwLen < sizeof(EnvBuffer)/sizeof(EnvBuffer[0])) {
	    pszEnv = EnvBuffer;
	} else {
	    pszEnv = NULL;
	}

        // First Estimate the path length and allocate it
        if (pszEnv && *pszEnv)
            len += wcslen(pszEnv);
        len += options.m_sbstrLIBPATH.Length();
        // Use the ANSI version, because we just want the length and ANSI always works
        // and the unicode length will never be longer than the ANSI length
        len += GetCurrentDirectoryA(0, NULL);
        // COM+ dir, semiclons, and terminating NULL
        len += MAX_PATH + 5;

        m_pszLibPath = (LPWSTR)globalHeap.Alloc(len * sizeof(WCHAR));
        if (len > UINT_MAX)
            dwLen = UINT_MAX;
        else
            dwLen = (DWORD)len - 1;
        if (0 != (newLen = W_GetCurrentDirectory(dwLen, m_pszLibPath))) {
            len -= newLen + 1;
            pszTemp = m_pszLibPath + newLen;
            *pszTemp++ = ENVIRONMENT_SEPARATOR;
        } else
            pszTemp = m_pszLibPath;

        if (len > UINT_MAX)
            dwLen = UINT_MAX;
        else
            dwLen = (DWORD)len - 1;

        wcscpy(pszTemp, GetCorSystemDirectory());
        dwLen = (DWORD) wcslen(GetCorSystemDirectory());
        pszTemp += dwLen;
        *pszTemp++ = ENVIRONMENT_SEPARATOR;
        len -= dwLen;

        if (options.m_sbstrLIBPATH.Length() && len > options.m_sbstrLIBPATH.Length()) {
            wcscpy(pszTemp, options.m_sbstrLIBPATH);
            len -= options.m_sbstrLIBPATH.Length();
            pszTemp += options.m_sbstrLIBPATH.Length();
            *pszTemp++ = ENVIRONMENT_SEPARATOR;
        }

        if (pszEnv && *pszEnv && len >= wcslen(pszEnv))
            wcscpy(pszTemp, pszEnv);
        else
            *pszTemp = 0; // NULL terminate the string
    }

    return m_pszLibPath;
}

/*
 * Discards all state accumulated in the local heap
 * and local symbols
 */
void COMPILER::DiscardLocalState()
{
    localSymAlloc.FreeHeap();
    symmgr.DestroyLocalSymbols();
}

/*
 * get the cor system directory
 */
LPCWSTR COMPILER::GetCorSystemDirectory()
{
    if (!corSystemDirectory)
    {
        // cache the CorSystemDirectory
        // Setup the Lib Search path
        HRESULT hr;
        WCHAR buffer[MAX_PATH+5];
        DWORD dwLen = MAX_PATH+4;

        if (FAILED(hr = GetMetadataDispenser()->GetCORSystemDirectory( buffer, dwLen, &dwLen))) {
            VSASSERT(hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER),
                "MAX_PATH should be big enough for the CORSystemDir!");
            // Don't report an error, we'll get it later when we try
            // "AddStandardMetaData()"
            Error(NULL, ERR_CantGetCORSystemDir, ErrHR(hr, false));
        }

        corSystemDirectory = this->namemgr->AddString(buffer);
    }

    return corSystemDirectory->text;
}

/*
 * Get the metadata dispenser.
 */
IMetaDataDispenserEx * COMPILER::GetMetadataDispenser()
{
    if (! dispenser) {
        // Obtain the dispenser.
        HRESULT hr;
        if (FAILED(hr = CoCreateInstance(CLSID_CorMetaDataDispenser, NULL, CLSCTX_SERVER,
                                         IID_IMetaDataDispenserEx,
                                         (LPVOID *) & dispenser)))
        {
            Error(NULL, FTL_ComPlusInit, ErrHR(hr));
        }

        SetDispenserOptions();

        if (FAILED(hr = linker->Init(dispenser, &alError)))
        {
            Error(NULL, FTL_ComPlusInit, ErrHR(hr));
        }
    }

    return dispenser;
}

/*
 * sets the options on the current metadata dispenser
 */
void COMPILER::SetDispenserOptions()
{
    VARIANT v;

    if (dispenser) {
        if (!incrementalRebuild) {

            // Set the emit options for maximum speed: no token remapping, no ref to def optimization,
            // no duplicate checking. We do all these optimizations
            // ourselves.

            // Only check typerefs, member refs, modulerefs and assembly refs -- we need to do this
            // because DefineImportMember or ALink may create these refs for us.
            V_VT(&v) = VT_UI4;
            V_UI4(&v) = MDDupTypeRef | MDDupMemberRef | MDDupModuleRef | MDDupAssemblyRef;

			if (options.m_fEMITDEBUGINFO)
				V_UI4(&v) |= MDDupSignature;
            dispenser->SetOption(MetaDataCheckDuplicatesFor, &v);

            // Never change refs to defs
            V_VT(&v) = VT_UI4;
            V_UI4(&v) = MDRefToDefNone;
            dispenser->SetOption(MetaDataRefToDefCheck, &v);

            // Give error if emitting out of order
            V_VT(&v) = VT_UI4;
                V_UI4(&v) = MDErrorOutOfOrderAll;
            dispenser->SetOption(MetaDataErrorIfEmitOutOfOrder, &v);

            // Turn on incremental build for the schema
            V_VT(&v) = VT_UI4;
            V_UI4(&v) = MDUpdateFull;


            dispenser->SetOption(MetaDataSetUpdate, &v);
        } else {
            // We're incrementally rebuilding, check for duplicates
            // Only check local var signatures for duplicates
            V_VT(&v) = VT_UI4;
            V_UI4(&v) = MDDupTypeRef | MDDupMemberRef | MDDupPermission |
                      MDDupParamDef | MDDupSignature | MDDupModuleRef |
                      MDDupAssemblyRef | MDDupFile | MDDupExportedType |
                      MDDupManifestResource | MDDupModuleRef | MDDupAssembly;
            dispenser->SetOption(MetaDataCheckDuplicatesFor, &v);

            // Never change refs to defs
            V_VT(&v) = VT_UI4;
            V_UI4(&v) = MDRefToDefNone;
            dispenser->SetOption(MetaDataRefToDefCheck, &v);

            // Give error if emitting out of order
            V_VT(&v) = VT_UI4;
            V_UI4(&v) = MDErrorOutOfOrderNone;
            dispenser->SetOption(MetaDataErrorIfEmitOutOfOrder, &v);

            // Turn on incremental build for the schema
            V_VT(&v) = VT_UI4;
            V_UI4(&v) = MDUpdateIncremental;
            dispenser->SetOption(MetaDataSetUpdate, &v);
        }

    }
}


/* Do the compilation. */
HRESULT COMPILER::Compile(ICSCompileProgress * progressSink)
{
    HRESULT hr = NOERROR;
    DWORD dwExceptionCode;

    PAL_TRY {
        // Do compilation here.

        // Are we timing this compile?
        if (options.m_fTIMING)
            ActivateTiming();

        if (options.runInternalTests) {
            // The internal tests flags has been specified. Just run the internal
            // tests and do nothing else.
#ifdef DEBUG
            tests.AllTests();
            if (ErrorCount() != 0)
                hr = E_FAIL;    // Some compilation errors.
#else
            ; // do nothing.
#endif //DEBUG
        }
        else {
            PAL_TRY
            {
                COMPILER_EXCEPTION_TRYEX {
                    CompileAll(progressSink);
                }
                COMPILER_EXCEPTION_EXCEPT_FILTER_EX(PAL_Except1, &dwExceptionCode)
                {
                    hr = E_UNEXPECTED;
                }
                COMPILER_EXCEPTION_ENDTRY
            }
            PAL_FINALLY_EX(PAL_Finally1)
            {
                SourceFileIterator files;
                for (files.Reset(this); files.Current(); files.Next()) {
                    if (files.Current()->pData) {
                        files.Current()->pData->Release();
                    }
                }

                if (SUCCEEDED(hr)) {
                    if (GetFirstOutFile() == symmgr.GetMDFileRoot() && !GetFirstOutFile()->nextOutfile())
                        hr = E_FAIL;    // didn't compile any source files
                    else if (ErrorCount() != 0)
                        hr = E_FAIL;
                }

                Term(true);         // Normal termination; check memory leaks.
            }
            PAL_ENDTRY
        }

    }
    EXCEPT_FILTER_EX(PAL_Except2, &dwExceptionCode)
    {
        // Exception/fatal error during cleanup. Just Terminate
        if (dwExceptionCode == FATAL_EXCEPTION_CODE)
            hr = E_FAIL;
        else
            hr = E_UNEXPECTED;
    }
    PAL_ENDTRY

    // Are we timing this compile?
    if (options.m_fTIMING) {
        FinishTiming();
        ReportTimes(stdout);
    }
    return hr;
}

#undef IfFailRet
#define IfFailRet(expr) if (FAILED((hr = (expr)))) return hr;
#undef IfFailGo
#define IfFailGo(expr) if (FAILED((hr = (expr)))) goto Error;


/*
 * Returns the current output file name
 */
LPCWSTR COMPILER::GetOutputFileName ()
{
    return GetFirstOutFile()->nextOutfile()->name->text;
}




// These indicate phases in the compile. These are confusingly similar to the "compile stages",
// yet different. They are only used for progress reporting.
enum COMPILE_PHASE {
    PHASE_DECLARETYPES,
    PHASE_IMPORTTYPES,
    PHASE_DEFINE,
    PHASE_PREPARE,
    PHASE_CHECKIFACECHANGE,
    PHASE_COMPILE,
    PHASE_WRITEOUTPUT,
    PHASE_WRITEINCFILE,

    PHASE_MAX
};

// The integers in this array indicate rough proportional amount of time
// each phase takes.
static const int relativeTimeInPhase[PHASE_MAX] = 
{
    12,         // PHASE_DECLARETYPES
    2,          // PHASE_IMPORTTYPES
    1,          // PHASE_DEFINE
    5,          // PHASE_PREPARE
    1,          // PHASE_CHECKIFACECHANGE
    40,         // PHASE_COMPILE
    15,         // PHASE_WRITEOUTPUT
    1,          // PHASE_WRITEINCFILE
};

/*
 * declare all the input files
 *
 * adds symbol table entries for all namespaces and
 * user defined types (classes, enums, structs, interfaces, delegates)
 * sets access modifiers on all user defined types
 */
void COMPILER::DeclareTypes(ICSCompileProgress * progressSink)
{
    ChangedFileIterator changedFiles;
    SourceFileIterator infiles;
    SourceOutFileIterator outfiles;
    INFILESYM *infile;
    int oldErrors = ErrorCount();

    int filesDone = 0;

    //
    // parse & declare the input files
    //
    {
    TIMEBLOCK(TIME_DECLARE);
    for (changedFiles.Reset(this); changedFiles.Current(); changedFiles.Next())
    {
        DeclareOneFile(changedFiles.Current());
        ReportProgress(progressSink, PHASE_DECLARETYPES, filesDone++, cInputFiles);
    }
    if (oldErrors != ErrorCount()) return;
    }


    {
    TIMEBLOCK(TIME_INCBUILD);
    if (!incrementalRebuild) {
        if (options.m_fINCBUILD) {
            // force the generation of incremental info
            incrementalRebuild = true;
            goto ERROR_LABEL;
        } else {
            return;
        }
    }

    //
    // determine if the type list has changed for all changed files
    // match up metadata tokens for all types
    //
    // NOTE: we already know that the list of input files hasn't changed
    // NOTE: and that all imported metadata hasn't changed

    //
    // quick check to determine if the number of types has changed
    //
    for (changedFiles.Reset(this); changedFiles.Current(); changedFiles.Next()) {
        if (changedFiles.Current()->cTypes != changedFiles.Current()->cTypesPrevious) {
            goto ERROR_LABEL;
        }
    }

    //
    // open the existing metadata PEs for non-EnC
    //
    if (!inEncMode()) {
        for (outfiles.Reset(this); outfiles.Current(); outfiles.Next()) {
            outfiles.Current()->metaimport = importer.OpenMetadataFile(outfiles.Current()->name->text, true);
            if (!outfiles.Current()->metaimport) {
                goto ERROR_LABEL;
            }
        }
    }

    //
    // the number of types hasn't changed
    // check changed files for type list change
    // also gets mdtokens for remaining unchanged & undependant types
    //
    for (infile = infiles.Reset(this); infile != NULL; infile = infiles.Next()) {

        // get the metadata import interface for the input file
        infile->metaimport[0] = infile->getOutputFile()->metaimport;

        //
        // import all the types
        //
        for (ULONG i = 0; i < infile->cTypesPrevious; i += 1) {
            AGGSYM *cls;
            if (infile->hasChanged)
                cls = importer.HasTypeDefChanged(infile, 0, infile->typesPrevious[i].token);
            else
                cls = importer.ImportOneType(infile, 0, infile->typesPrevious[i].token);

            if (!cls || cls->getInputFile() != infile) {
                goto ERROR_LABEL;
            }
            
            infile->typesPrevious[i].cls = cls;
            cls->tokenEmit = infile->typesPrevious[i].token;
            cls->tokenImport = cls->tokenEmit;


            //
            // match up the incremental build information
            //
            cls->incrementalInfo = &infile->typesPrevious[i];
            ASSERT(cls->incrementalInfo->dependantFiles);
            ASSERT((cls->parseTree  && infile->hasChanged) || (!cls->parseTree  && !infile->hasChanged));
        }
    }

        // If test flags is set dump log
        if (options.logIncrementalBuild)
            printf("Incremental Build: Type list is unchanged.\n");

    return;
    }

ERROR_LABEL:

        // If test flags is set dump log
        if (options.logIncrementalBuild)
            printf("Incremental Build: Type list is changed!\n");

    //
    // we are not going to incremental build anymore
    //
    SetFullRebuild();
    return;
}

void COMPILER::EmitTokens(OUTFILESYM *outfile)
{
    if (!IsIncrementalRebuild()) {
        InFileIterator infiles;
        INFILESYM *pInfile;
        for (pInfile = infiles.Reset(outfile); pInfile != NULL; pInfile = infiles.Next())
        {

            SETLOCATIONFILE(pInfile);

            clsDeclRec.emitTypedefsNamespace(pInfile->rootDeclaration);
        }

        for (pInfile = infiles.Reset(outfile); pInfile != NULL; pInfile = infiles.Next())
        {

            SETLOCATIONFILE(pInfile);

            clsDeclRec.emitMemberdefsNamespace(pInfile->rootDeclaration);
        }

        if (options.m_fINCBUILD) {
        }
    } else {
        //
        // free space that doesn't belong to any source input file
        //
        outfile->allocMapAll = ALLOCMAP::Remove(outfile->allocMapAll, outfile->allocMapNoSource);
        outfile->allocMapNoSource->Clear();

        ChangedFileIterator changedFiles;
        INFILESYM *pInfile;
        for (pInfile = changedFiles.Reset(this); pInfile != NULL; pInfile = changedFiles.Next()) {

            SETLOCATIONFILE(pInfile);

            //
            // update tokens incrementally
            //
            clsDeclRec.reemitMemberdefsNamespace(pInfile->rootDeclaration);

            //
            // free space in the RVA heap that is used by files we're recompiling
            //
            outfile->allocMapAll = ALLOCMAP::Remove(outfile->allocMapAll, pInfile->allocMap);
            pInfile->allocMap->Clear();

            //
            // Delete all the members in <PrivateImplementationDetails> that we added when
            // building this file (they will get re-emitted later if needed)
            //
            if (pInfile->cGlobalFields)
            {
                // only the first one is partially full
                ULONG index = pInfile->cGlobalFields & (lengthof(pInfile->globalFields->pTokens) - 1);
                for (ULONG i = 0; i < index; i++)
                    emitter.DeleteToken(pInfile->globalFields->pTokens[i]);
                for (INFILESYM::TOKENLIST * pList = pInfile->globalFields->pNext; pList != NULL; pList = pList->pNext) {
                    for (ULONG i = 0; i < lengthof(pList->pTokens); i++)
                        emitter.DeleteToken(pList->pTokens[i]);
                }
                pInfile->cGlobalFields = 0;
                pInfile->globalFields = NULL; // We can't cleanup because this was alloced  on a NRHEAP
            }
        }

        //
        // copy old RVA heap into the new file
        //
        CopyRVAHeap(outfile);
    }
}

void * COMPILER::AllocateRVABlock(INFILESYM *infile, ULONG cbCode, ULONG alignment, ULONG *codeRVA, ULONG *offset)
{
    if (options.m_fINCBUILD)
    {
        //
        // check for a free spot in the current outfile
        //
        OUTFILESYM *outfile = curFile->GetOutFile();
        void *codeLocation = NULL;
        ULONG realSize;
        ULONG realOffset;
        outfile->allocMapAll = ALLOCMAP::Allocate(outfile->allocMapAll, cbCode, alignment, offset, &realSize, &realOffset);
        if (*offset == ALLOCMAP::INVALID_OFFSET) {
            //
            // need a new allocation form the PE
            //
            codeLocation = curFile->AllocateRVABlock(cbCode, alignment, codeRVA, offset);

            //
            // if the new block was aligned, add the alignment padding to the allocation
            //
            if (*offset <= (outfile->allocMapAll->MaxUsedOffset() + alignment)) {
                realOffset = outfile->allocMapAll->MaxUsedOffset();
            }
            else 
            {
                realOffset = *offset;
            }
            realSize = *offset + cbCode - realOffset;

            outfile->allocMapAll = ALLOCMAP::AddAlloc(outfile->allocMapAll, realOffset, realSize);
        }
        else 
        {
            codeLocation = curFile->GetCodeLocationOfOffset(*offset);
            *codeRVA = curFile->GetRVAOfOffset(*offset);
        }

        //
        // mark this allocation as belonging to the infile
        //
        if (infile) {
            infile->allocMap = ALLOCMAP::AddAlloc(infile->allocMap, realOffset, realSize);
        } else {
            outfile->allocMapNoSource = ALLOCMAP::AddAlloc(outfile->allocMapNoSource, realOffset, realSize);
        }

        return codeLocation;
    }
    else 
    {
        return curFile->AllocateRVABlock(cbCode, alignment, codeRVA, offset);
    }
}

/*
 * Copies the old RVA block from the old output file into the new one
 */
HRESULT COMPILER::CopyRVAHeap(OUTFILESYM *pOutfile)
{
    HRESULT            hr = S_OK;
    PBYTE              pbMapAddress = NULL;
    PBYTE              pbData = NULL;
    HANDLE             hMapFile = NULL;
    HANDLE             hFile = INVALID_HANDLE_VALUE;
    PBYTE              pbNewData = NULL;
    ULONG              newRVA = 0;
    ULONG              offset;

    //
    // open the old file and get a pointer to the old code rva block
    //
    hFile = OpenFileEx( pOutfile->name->text, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }

    hMapFile = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (hMapFile == NULL) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }

    pbMapAddress = (PBYTE) MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, 0);
    if (!pbMapAddress) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }

    pbData = (PBYTE) CPEFile::Cor_RtlImageRvaToVa(pbMapAddress, curFile->GetRVAOfOffset(0));
    if (!pbData) {
        hr = E_FAIL;
        goto exit;
    }

    if (pOutfile->allocMapAll->MaxUsedOffset() > 0) {
        //
        // get the new RVA block
        //
        pbNewData = (PBYTE) curFile->AllocateRVABlock(pOutfile->allocMapAll->MaxUsedOffset(), 4, &newRVA, &offset);

        //
        // copy the old RVAs in to the new block
        //
        memcpy(pbNewData, pbData, pOutfile->allocMapAll->MaxUsedOffset());
    }

exit:
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle( hFile);
    if (hMapFile != NULL)
        CloseHandle( hMapFile);
    if (pbMapAddress)
        UnmapViewOfFile(pbMapAddress);

    return hr;
}


/*
 * Report compiler progress and give the user a chance to cleanly cancel the compile.
 * The compiler is now in the middle of phase "phase", and has completed "itemsComplete"
 * of the "itemsTotal" in this phase.
 *
 * returns TRUE if the compilation should be canceled (an error has already been reported.
 */

bool COMPILER::ReportProgress(ICSCompileProgress * progressSink, int phase, int itemsComplete, int itemsTotal)
{
    bool cancel = false;
    long totItemsComplete, totItems;
    const int SCALE = 1024;

    if (progressSink == NULL)
        return false;

    ASSERT(phase < PHASE_MAX);

    // convert phase , itemsComplex, itemsTotal into totItemsComplex and totItems, using the
    // relative time in phase array.
    totItems = totItemsComplete = 0;
    for (int i = 0; i < PHASE_MAX; ++i) {
        totItems += SCALE * relativeTimeInPhase[i];
        if (i < phase)
            totItemsComplete += SCALE * relativeTimeInPhase[i];
    }
    if (itemsTotal > 0)
        totItemsComplete += (SCALE * itemsComplete / itemsTotal) * relativeTimeInPhase[phase];

    cancel = !!(progressSink->ReportProgress (L"",  totItems - totItemsComplete, totItems));

    if (cancel && !isCanceled) {
        Error (NULL, ERR_CompileCancelled);
        isCanceled = true;
    }

    return isCanceled;
}


void COMPILER::ParseOneFile(INFILESYM *infile)
{
    TIMEBLOCK(TIME_PARSETOPLEVEL);

    ICSSourceModule     *pModule = NULL;
    ICSSourceData       *pData = NULL;
    HRESULT              hr;

    SETLOCATIONSTAGE(PARSE);
    SETLOCATIONFILE(infile);

    // Get a source module for this input file, and ask it for a source data
    // object (blocking for newest -- REVIEW:  is that the best idea?)
    TimerStart(TIME_HOST_GETSOURCEMODULE);
    hr = host->GetSourceModule (infile->name->text, &pModule);
    TimerStop(TIME_HOST_GETSOURCEMODULE);

    if (SUCCEEDED(hr)) {
        TIMEBLOCK(TIME_HOST_GETSOURCEDATA);
        hr = pModule->GetSourceData (TRUE, &pData);
    }
    if (FAILED (hr))
    {
        Error (NULL, ERR_NoSourceFile, infile->name->text, ErrHR(hr));
    }
    else
    {
        BASENODE                    *pNode;
        CComPtr<ICSErrorContainer>  spErrors;

#ifdef TRACKMEM
        unsigned int uPageHeapPrev = 0;
        if (options.trackMem) {
            uPageHeapPrev = pageheap.GetCurrentUseSize();
        }
#endif

        // Build the top-level parse tree.  Note that this may already be done...
        pData->ParseTopLevel (&pNode);
#ifdef TRACKMEM
        if (options.trackMem) {
            uParseMemory += (pageheap.GetCurrentUseSize() - uPageHeapPrev);
        }
#endif

        // Get any tokenization errors that may have occurred and send them to the host.
        if (SUCCEEDED (pData->GetErrors (EC_TOKENIZATION, &spErrors)))
            pController->ReportErrorsToHost (spErrors);
        spErrors.Release();

        // Same for top-level parse errors
        if (SUCCEEDED (pData->GetErrors (EC_TOPLEVELPARSE, &spErrors)))
            pController->ReportErrorsToHost (spErrors);
        spErrors.Release();

        infile->nspace = pNode->asNAMESPACE();

        infile->pData = ((CSourceData*)pData);     // NOTE:  Ref ownership transferred here...
    }

    if (pModule)
        pModule->Release();     // (if pData is valid, it has a ref on this)
}

void COMPILER::DeclareOneFile(INFILESYM *infile)
{
    // do parsing on demand
    ParseOneFile(infile);

    if (infile->nspace)
    {
        TIMEBLOCK(TIME_DECLARE);
        SETLOCATIONFILE(infile);
        SETLOCATIONSTAGE(DECLARE);
        infile->nextChangedFile = changedFiles;
        changedFiles = infile;
        this->cChangedFiles += 1;
        clsDeclRec.declareInputfile(infile->nspace, infile);
        infile->hasChanged = true;
    }
}

void COMPILER::ResolveInheritanceHierarchy()
{
    TIMEBLOCK(TIME_DEFINE);

    //
    // do object first to avoid special cases later
    //
    AGGSYM *object = compiler()->symmgr.GetObject();
    object->isResolvingBaseClasses = false;
    object->hasResolvedBaseClasses = true;

    SourceFileIterator infiles;
    for (INFILESYM *pInfile = infiles.Reset(this); pInfile != NULL; pInfile = infiles.Next())
    {
        SETLOCATIONSTAGE(DEFINE);
        SETLOCATIONFILE(pInfile);

        if (!clsDeclRec.resolveInheritance(pInfile->rootDeclaration))
        {
            //
            // we had a change in the inheritance hierarchy
            // grab parse trees for all unchanged files
            // and then finish resolving the hierachy
            //
            ASSERT(this->IsIncrementalRebuild());

            // If test flags is set dump log
            if (options.logIncrementalBuild)
                printf("Incremental Build: Inheritance Hierarchy Change Detected!\n");

            SetFullRebuild();
            ResolveInheritanceHierarchy();
            break;
        }
    }
    if (IsIncrementalRebuild()) {
            // If test flags is set dump log
            if (options.logIncrementalBuild)
                printf("Incremental Build: Inheritance Hierarchy Unchanged!\n");
    }
}

void COMPILER::DefineOneFile(INFILESYM *pInfile)
{
    if (!pInfile->isDefined) {
        TIMEBLOCK(TIME_DEFINE);
        SETLOCATIONSTAGE(DEFINE);
        SETLOCATIONFILE(pInfile);

        clsDeclRec.defineNamespace(pInfile->rootDeclaration);
        pInfile->isDefined = true;
    }
}

void COMPILER::DefineFiles(IInfileIterator &iter)
{
    INFILESYM *pInfile;
    for (pInfile = iter.Current(); pInfile != NULL; pInfile = iter.Next())
    {
        DefineOneFile(pInfile);
    }
}

void COMPILER::EvaluateConstants(INFILESYM *infile)
{
    if (infile->isConstsEvaled) {
        ASSERT(infile->isDefined);
        return;
    }

    TIMEBLOCK(TIME_DEFINE);
    DefineOneFile(infile);
    ASSERT(!infile->isConstsEvaled);
    TypeIterator types;
    for (AGGSYM *cls = types.Reset(infile); cls; cls = types.Next()) {
        clsDeclRec.evaluateConstants(cls);
    }
    infile->isConstsEvaled = true;
}

//
// check that the number of changed & dependant files is
// over our threshold
bool COMPILER::CheckChangedOverThreshold()
{
    if ((this->cChangedFiles << 1) > numberOfSourceFiles()) {
        // If test flags is set dump log
        if (options.logIncrementalBuild)
            printf("Incremental Build: Too many interface changes detected (%d/%d)!\n", this->cChangedFiles, numberOfSourceFiles());
        SetFullRebuild();
        return true;
    }

    return false;
}

//
// define all the input files
//
// adds symbols for all fields & methods including types
// does name conflict checking within a class
// checks field & method modifiers within a class
// does access checking for all types
//
// 1 - define members of changed files only
// 2 - check interface changes for changed files
// 3 a - on iface changed       - do a full rebuild
// 3 b - small iface changed    - define from source dependant files
// 4 - define unchanged (and undependant) files from metadata
//
void COMPILER::DefineMembers()
{
    int oldErrors = ErrorCount();
    INFILESYM *infile;

    {
    TIMEBLOCK(TIME_DEFINE);

    // bring object up to declared/defined state first
    // so that we don't have to special case it elsewhere
    clsDeclRec.defineObject();
#ifdef DEBUG
    // If test flags is set, import all members of all imported types.
    if (options.fullyImportAll)
        importer.DeclareAllTypes(symmgr.GetRootNS());
    if (options.testEmitter) {
        tests.TestEmitter();
        return;
    }
#endif //DEBUG

    if (symmgr.GetPredefType(PT_ATTRIBUTEUSAGE, false))
        EnsureTypeIsDefined(symmgr.GetPredefType(PT_ATTRIBUTEUSAGE, false));
    if (symmgr.GetPredefType(PT_OBSOLETE, false))
        EnsureTypeIsDefined(symmgr.GetPredefType(PT_OBSOLETE, false));
    if (symmgr.GetPredefType(PT_CONDITIONAL, false))
        EnsureTypeIsDefined(symmgr.GetPredefType(PT_CONDITIONAL, false));
    EnsureTypeIsDefined(symmgr.GetPredefType(PT_CLSCOMPLIANT, false));

    CheckChangedOverThreshold();
    for (infile = changedFiles; infile; infile = changedFiles) {
        ASSERT(infile->hasChanged);
        if (infile->isConstsEvaled) {
            changedFiles = infile->nextChangedFile;
            infile->nextChangedFile = 0;
        } else {
            SETLOCATIONFILE(infile);

            //
            // this does define members as well
            //
            EvaluateConstants(infile);

            //
            // test if any interface has changed, and if so parse all dependant files
            // and add them to the changed list
            //
            if (IsIncrementalRebuild()) {

                TIMEBLOCK(TIME_INCBUILD);
                TypeIterator types;
                AGGSYM *cls;
                for (cls = types.Reset(infile); cls; cls = types.Next()) {
                    if (importer.HasInterfaceChange(cls)) {

                        // may already have set a full rebuild
                        if (!IsIncrementalRebuild())
                        {
                            break;
                        }

                        //
                        // mark all dependant files as being changed
                        //
                        for (unsigned index = 0; index < this->cInputFiles; index += 1) {
                            if (cls->incrementalInfo->dependantFiles->testBit(index+1)) {
                                INFILESYM *dependantFile = this->inputFilesById[index];
                                if (!dependantFile->nspace) {
                                    ASSERT(!dependantFile->hasChanged);
                                    this->DeclareOneFile(dependantFile);
                                }
                                ASSERT(dependantFile->hasChanged);
                            }
                        }
                        if (CheckChangedOverThreshold())
                        {
                            break;
                        }
                    }
                }
            }
        }
    }

    // check for changes in CLS flags in incremental build
    // must happen after Define Members on changed files
    // but before we decide to do an incremental rebuild
    // Changes in CLS flags will force a full rebuild
    CheckCLS();

    if (!IsIncrementalRebuild()) {
        return;
    }

    if (oldErrors != ErrorCount()) {
        return;
    }
    }

    // If test flags is set dump log
    if (options.logIncrementalBuild)
        printf("Incremental Build: Interface changes below threshold (%d/%d)!\n", this->cChangedFiles, numberOfSourceFiles());

    ASSERT(!isDefined);
    isDefined = true;

    DUMPALLHEAPS( L"After define stage");
}

// returns if CLS complianceChecking is enabled in any of these attributes.
bool COMPILER::ScanAttributesForCLS(GLOBALATTRSYM * attributes)
{
    // Compile the CLS attributes (this will loop through all of them, so only call it once!)
    // Becuase this will also give regular compiler errors for all badly formed global attributes
    if (attributes)
        CLSGLOBALATTRBIND::Compile(this, attributes);

    while (attributes) {
        if (attributes->hasCLSattribute && attributes->isCLS) {
            return true;
        }
        attributes = attributes->nextAttr;
    }
    
    return false;
}

// Set the global CLS attribute on the infiles
void COMPILER::CheckCLS()
{
    checkCLS = false;
    
    bool assemblyCLSCheck = ScanAttributesForCLS(this->assemblyAttributes);
    
    if (assemblyCLSCheck)
        checkCLS = true;

    if (IsIncrementalRebuild()) {
        if (importer.HasAssemblyChanged(
                        SourceOutFileIterator().Reset(this)->firstInfile(), 
                        assemblyCLSCheck, 
                        TokenFromRid((BuildAssembly() ? 1 : 0), mdtAssembly))) {
            // If test flags is set dump log
            if (options.logIncrementalBuild)
                printf("Incremental Build: Assembly is changed!\n");
            SetFullRebuild();
        }
    }
    
    // Set the bit on each INFILESYM so the symbols don't have to check back with the compiler
    SourceOutFileIterator files;
    PINFILESYM pInfile;
    InFileIterator infiles;

    for (OUTFILESYM *pOutfile = files.Reset(this); pOutfile != NULL; pOutfile = files.Next())
    {
        bool moduleCLSCheck = false;        // checking on for this module (never true if building an assembly).
        
        if (!BuildAssembly()) {
            moduleCLSCheck = ScanAttributesForCLS(pOutfile->attributes);
            if (moduleCLSCheck)
                checkCLS = true;
        }

        if (IsIncrementalRebuild()) {
            if (importer.HasModuleChanged(pOutfile->firstInfile(), moduleCLSCheck)) {
            // If test flags is set dump log
            if (options.logIncrementalBuild)
                printf("Incremental Build: Module is changed!\n");
                SetFullRebuild();
            }
        }
            
        if (assemblyCLSCheck || moduleCLSCheck) {
            for (pInfile = infiles.Reset(pOutfile); pInfile != NULL; pInfile = infiles.Next())
            {
                pInfile->hasCLSattribute = true;
                pInfile->isCLS = true;
            }
        }
    }
    
    if (checkCLS && BuildAssembly()) {
        // check imports
        for (pInfile = infiles.Reset(symmgr.GetMDFileRoot()); pInfile != NULL; pInfile = infiles.Next())
        {
            if (pInfile->isAddedModule && (!pInfile->hasCLSattribute || !pInfile->isCLS)) {
                // Added modules must be CLS compliant (or at least have the bit set,
                // so we know that all defined classes are properly marked
                compiler()->Error(ERRLOC(pInfile), ERR_CLS_ModuleMissingCLS);
            }
        }
    }

}

void COMPILER::PostCompileChecks()
{
    //
    // check if any non-external fields are never assigned to
    // or private fields and events are never referenced.
    //
    INFILESYM *pInfile;
    SourceFileIterator infiles;
    for (pInfile = infiles.Reset(this); pInfile != NULL; pInfile = infiles.Next())
    {
        if (pInfile->hasChanged) {
            TypeIterator types;
            for (AGGSYM *cls = types.Reset(pInfile); cls != NULL; cls = types.Next())
            {
                // Don't Check Certain structs
                if (cls->isStruct && cls->hasExternReference)
                    continue;

                bool hasExternalVisibility = cls->hasExternalAccess();
                FOREACHCHILD(cls, member)
                    if (member->kind == SK_MEMBVARSYM)
                    {
                        MEMBVARSYM *field = member->asMEMBVARSYM();
                        // Only check non-Const internall-only fields and events
                        if (field->isConst || (hasExternalVisibility && field->hasExternalAccess()))
                            continue;

                        if (!field->isReferenced && field->access == ACC_PRIVATE)
                        {
                            clsDeclRec.errorSymbol(field, field->isEvent ? WRN_UnreferencedEvent : WRN_UnreferencedField);
                        }
                        else if (!field->isAssigned && !field->isEvent)
                        {
                            LPCWSTR zeroString;
                            if (field->type->isNumericType())
                                zeroString = L"0";
                            else if (field->type->isPredefType(PT_BOOL))
                                zeroString = L"false";
                            else if (field->type->kind == SK_AGGSYM && field->type->asAGGSYM()->isStruct)
                                zeroString = L"";
                            else
                                zeroString = L"null";
                            clsDeclRec.errorSymbolStr(field, WRN_UnassignedInternalField, zeroString);
                        }
                    }
#if USAGEHACK
                    if (member->kind == SK_METHSYM) {
                        METHSYM * meth = member->asMETHSYM();
                        if (!meth->isPropertyAccessor && !meth->isUsed && (meth->access == ACC_INTERNAL || meth->access == ACC_PRIVATE) && !meth->isOverride && meth->getInputFile()->isSource) {
                            printf("%ls : %ls\n", meth->getInputFile()->name->text, ErrSym(meth));
                        }
                    }
                    if (member->kind == SK_PROPSYM) {
                        PROPSYM * prop = member->asPROPSYM();
                        if ((prop->access == ACC_INTERNAL || prop->access == ACC_PRIVATE)&& !prop->isOverride && prop->getInputFile()->isSource) {
                            if (prop->methGet && !prop->methGet->isUsed) {
                                printf("%ls : %ls\n", prop->getInputFile()->name->text, ErrSym(prop->methGet));
                            }
                            if (prop->methSet && !prop->methSet->isUsed) {
                                printf("%ls : %ls\n", prop->getInputFile()->name->text, ErrSym(prop->methSet));
                            }
                        }
                    }
#endif
                ENDFOREACHCHILD
            }
        }
    }
}

/*
 * Do the full compile. The main process of compilation after all options
 * have been accumulated.
 */
void COMPILER::CompileAll(ICSCompileProgress * progressSink)
{

 //   BASSERT(0);

    SETLOCATIONSTAGE(BEGIN);
    OUTFILESYM *pAsmFile = NULL;
    OUTFILESYM *pOutfile = NULL;
    curFile = NULL;
    SourceOutFileIterator files;
    SourceFileIterator infiles;
    INFILESYM * pInfile = NULL;
    assemID = AssemblyIsUBM;
    HRESULT hr;
#ifdef DEBUG
    haveDefinedAnyType = false;
#endif

    int oldErrors = ErrorCount();
    isCanceled = false;
    int iFile;

    TimerStart(TIME_COMPILE);

    // Clear any existing error info object, so we don't report stale errors.
    SetErrorInfo(0, NULL);

    // Add the standard metadata to the list of files to process.
    AddStandardMetadata();

    // Handle initial incremental building step.
    isDefined = false;
    if (options.m_fINCBUILD) {
        TIMEBLOCK(TIME_INCBUILD);
        incrementalRebuild = true;
        if (INCBUILD::ReadIncrementalInfo(this, &incrementalRebuild)) {
            // no files have changed, we're done
            // If test flags is set dump log
            if (options.logIncrementalBuild)
                printf("Incremental Build: No changed files. Done build.\n");
            goto ENDCOMPILE;
        }
        // Reset the options for incremental
        SetDispenserOptions();
        DUMPALLHEAPS( L"After reading Incremental Info");
    }

    //
    // declare all the input files
    //
    // adds symbol table entries for all namespaces and
    // user defined types (classes, enums, structs, interfaces, delegates)
    // sets access modifiers on all user defined types
    //
    DeclareTypes(progressSink);
    ReportProgress(progressSink, PHASE_DECLARETYPES, 1, 1);
    DUMPALLHEAPS( L"After declaring types");
    if (oldErrors != ErrorCount()) goto ENDCOMPILE;

    // Initialize the alinker by getting the common dispenser
    GetMetadataDispenser();

    // Import meta-data.
    {
    SETLOCATIONSTAGE(PARSE);

    importer.ImportAllTypes();
    ReportProgress(progressSink, PHASE_IMPORTTYPES, 1, 1);
    if (oldErrors != ErrorCount()) goto ENDCOMPILE;

    // Initialize predefined types. This is done after a declaration
    // for every predefined type has already been seen.
    symmgr.InitPredefinedTypes();
    if (oldErrors != ErrorCount()) goto ENDCOMPILE;

    }

    //
    // resolves all using clauses
    // resolves all base classes and implemented interfaces
    //
    ResolveInheritanceHierarchy();

    //
    // cannot define any types until the inheritance hierarchy is resolved
    //
    // if this assert fires then set a breakpoint at the 2 locations
    // where this variable is set to true and rerun your build
    //
    ASSERT(!haveDefinedAnyType);

    //
    // define all members of types
    //
    DefineMembers();
    ReportProgress(progressSink, PHASE_DEFINE, 1, 1);
#ifdef DEBUG
    // If test flags is set, import all members of all imported types.
    if (options.testEmitter) {
        goto ENDCOMPILE;
    }
#endif //DEBUG

	if (oldErrors != ErrorCount()) goto ENDCOMPILE;

    //
    // prepare all the input files
    //
    // evaluates constants
    // checks field & method modifiers between classes (overriding & shadowing)
    //
    TimerStart(TIME_PREPARE);

    iFile = 0;
    for (pInfile = infiles.Reset(this); pInfile != NULL; pInfile = infiles.Next())
    {
        SETLOCATIONSTAGE(PREPARE);
        SETLOCATIONFILE(pInfile);

        clsDeclRec.prepareNamespace(pInfile->rootDeclaration);

        if (ReportProgress(progressSink, PHASE_PREPARE, ++iFile, this->cInputFiles))
            goto ENDCOMPILE;
    }
    DUMPALLHEAPS( L"After prepare stage");
    if (oldErrors != ErrorCount()) goto ENDCOMPILE;

    //
    // initialize the assembly manifest emitter
    // even if we don't emit a manifest, we still
    // emit scoped typerefs
    //
    if (!options.m_fNOCODEGEN) {
        TimerStart(TIME_ASSEMBLY);

        // decide on an Assembly file and create it!
        SourceOutFileIterator files;
        for (POUTFILESYM pOutfile = files.Reset(this); pOutfile != NULL; pOutfile = files.Next())
        {
            if (!pOutfile->isDll) {
                pAsmFile = pOutfile;
                break;
            }
            if (!pAsmFile)
                pAsmFile = pOutfile;
        }
        pAsmFile->idFile = mdTokenNil;
        assemFile.Init( this);
        assemFile.BeginOutputFile(pAsmFile);

        if (BuildAssembly()) {
            if (FAILED(hr = linker->SetAssemblyFile(pAsmFile->name->text, assemFile.GetEmit(), inEncMode() ? afNone : afNoRefHash, &assemID)))
                Error( NULL, ERR_ALinkFailed, ErrHR(hr));
            else {
                pAsmFile->idFile = assemID;

                // Now add all the addmodules
                InFileIterator modules;

                for (pInfile = modules.Reset(symmgr.GetMDFileRoot()); pInfile != NULL; pInfile = modules.Next())
                {
                    if (pInfile->isAddedModule) {
                        if (FAILED(hr = linker->AddImport( assemID, pInfile->mdImpFile, 0, &pInfile->mdImpFile))) {
                            Error( NULL, ERR_ALinkFailed, ErrHR(hr));
                        }
                    }
                }
            }
        } else {
            assemID = AssemblyIsUBM;
            if (FAILED(hr = linker->AddFile(assemID, pAsmFile->name->text, 0, assemFile.GetEmit(), &pAsmFile->idFile)))
                Error( NULL, ERR_ALinkFailed, ErrHR(hr));
        }
        TimerStop(TIME_ASSEMBLY);
    }
    if (oldErrors != ErrorCount()) goto ENDCOMPILE;

    // This second loop forces each file to find it's Main()
    // It also cause each output file to have a definite filename
    for (pOutfile = files.Reset(this); pOutfile != NULL; pOutfile = files.Next())
    {
        //
        // the entry point method may be in a class which hasn't changed
        //
        if (IsIncrementalRebuild() && pOutfile->incbuild->entryClassToken) {
            AGGSYM *entryClass = importer.ResolveTypeRef(pOutfile->firstInfile(), 0, pOutfile->incbuild->entryClassToken, true);
            if (!entryClass->getInputFile()->hasChanged) {
                clsDeclRec.prepareAggregate(entryClass);
            }
        }

        emitter.FindEntryPoint( pOutfile);
        if (pOutfile->isUnnamed()) {
            symmgr.SetOutFileName( pOutfile->firstInfile());
            pOutfile->hasDefaultName = true;
        }
    }

    if (oldErrors != ErrorCount()) goto ENDCOMPILE;
    TimerStop(TIME_PREPARE);

#ifdef DEBUG
    // If test flags is set, dump the symbol table.
    if (options.dumpSymbols)
        symmgr.DumpSymbol(symmgr.GetRootNS());
#endif //DEBUG

    //


    Sleep(200);

    //
    // compile all the input files
    //
    // emits all user defined types including fields, & method signatures
    // constant field values.
    //
    // compiles and emits all methods
    //
    // For each output file, emission of metadata happens in three phases in order to
    // have the metadata emitting work most efficiently. Each phase happens in the same
    // order.
    //   1. Emitter typedefs for all types
    //   2. Emitting memberdefs for all members
    //   3. Compiling members and emitting code, additional metadata (param tokens) onto the
    //      type and memberdefs emitted previously.
    //
    // Always does the assembly file first, then the other files.
    files.Reset(this);
    if (!(pOutfile = pAsmFile)) {
        pOutfile = files.Current();
        files.Next();
    }
    iFile = 0;
    do {
        INFILESYM * pInfile;
        InFileIterator infiles;
        CPEFile nonManifestOutputFile;

        if (!options.m_fNOCODEGEN) {
            SETLOCATIONSTAGE(EMIT);
            if (pAsmFile == pOutfile) {
                curFile = &assemFile;
            } else {
                curFile = &nonManifestOutputFile;
                curFile->Init( this);
                curFile->BeginOutputFile(pOutfile);
                if (FAILED(hr = linker->AddFile(assemID, pOutfile->name->text, 0, curFile->GetEmit(), &pOutfile->idFile))) {
                    Error(NULL, ERR_ALinkFailed, ErrHR(hr));
                    // We can't go on if we couldn't add the file
                    break;
                }
                if (pOutfile->firstInfile() == NULL)
                    Error( NULL, ERR_OutputNeedsInput, pOutfile->name->text);
            }
            emitter.BeginOutputFile();
        }

        EmitTokens(pOutfile);

        if (!options.m_fNOCODEGEN && pAsmFile == pOutfile) {
            //
            // do assembly attributes
            //
            if (this->IsIncrementalRebuild()) {
                //
                // delete existing assembly attributes
                //
                emitter.DeleteRelatedAssemblyTokens(pOutfile->firstInfile(), TokenFromRid((BuildAssembly() ? 1 : 0), mdtAssembly));
            }

            GLOBALATTRBIND::Compile(this, pAsmFile, assemblyAttributes, mdtAssembly);

            // Write the manifest and save space for the crypto-keys
            TimerStart(TIME_ASSEMBLY);
            if (BuildAssembly() && !ErrorCount())
                assemFile.WriteCryptoKey();
            TimerStop(TIME_ASSEMBLY);
        }
        
        TimerStart(TIME_COMPILENAMESPACE);
        {
        SETLOCATIONSTAGE(COMPILE);
        for (pInfile = infiles.Reset(pOutfile); pInfile != NULL; pInfile = infiles.Next()) {

            SETLOCATIONFILE(pInfile);
            if (pInfile->hasChanged) {
                clsDeclRec.compileNamespace(pInfile->rootDeclaration);
            }
            DUMPALLHEAPS( pInfile->name->text);
#ifdef TRACKMEM
            if (options.trackMem) {
                long iTopTree, iInteriorTree, iInteriorNodes;
                if (pInfile->pData) {
                    pInfile->pData->GetModule()->GetMemoryConsumption(&iTopTree, &iInteriorTree, &iInteriorNodes);
                    wprintf( L"Parse Trees-%s\t%u\t%u\t%u\n", pInfile->name->text, iTopTree, iInteriorTree, iInteriorNodes);
                }
            }
#endif

            if (ReportProgress(progressSink, PHASE_COMPILE, ++iFile, this->cInputFiles))
                goto ENDCOMPILE;
        }
        }
        TimerStop(TIME_COMPILENAMESPACE);

        if (!options.m_fNOCODEGEN) {

            if (options.m_fINCBUILD && pOutfile->entrySym) {
                pOutfile->incbuild->entryClassToken = pOutfile->entrySym->getClass()->tokenEmit;
            }

            // Write the executable file if no errors occured.
            emitter.EndOutputFile(!ErrorCount());
            if (pOutfile != pAsmFile) {

                curFile->EndOutputFile(!ErrorCount());
                if (ReportProgress(progressSink, PHASE_WRITEOUTPUT, 1, 1))
                    goto ENDCOMPILE;
            }
        }
        DUMPALLHEAPS( pOutfile->name->text);
        curFile = NULL;
        
        // Skip the manifest file because we already did it
        if ((pOutfile = files.Current()) == pAsmFile && pOutfile)
            pOutfile = files.Next();
        if (files.Current())
            files.Next();
    } while (pOutfile);

    if (!ErrorCount())
        PostCompileChecks(); // Only give these warnings if we had no errors.

    if (pAsmFile) {
        TimerStart(TIME_ASSEMBLY);

        ASSERT(!curFile);
        curFile = &assemFile;
        if (!ErrorCount()) {
            // Include the resources
            PRESFILESYM res;
            bool bFail = false;
            for (pOutfile = GetFirstOutFile(); pOutfile != NULL; pOutfile = pOutfile->nextOutfile()) {
                for( res = pOutfile->firstResfile(); res != NULL; res = res->nextResfile()) {
                    bFail |= !assemFile.CalcResource(res);
                }
            }
            if (!bFail && assemFile.AllocResourceBlob()) {
                for (pOutfile = GetFirstOutFile(); pOutfile != NULL; pOutfile = pOutfile->nextOutfile()) {
                    for( res = pOutfile->firstResfile(); res != NULL; res = res->nextResfile()) {
                        assemFile.AddResource( res);
                    }
                }
            }
        }

        if (BuildAssembly() && FAILED(hr = linker->PreCloseAssembly(assemID)))
            Error( NULL, ERR_ALinkFailed, ErrHR(hr));
        assemFile.EndOutputFile(!ErrorCount());
        curFile = NULL;
        if (ReportProgress(progressSink, PHASE_WRITEOUTPUT, 1, 1))
            goto ENDCOMPILE;

        if (!ErrorCount() && !inEncMode() && FAILED(hr = linker->CloseAssembly(assemID)))
            Error(NULL, ERR_ALinkCloseFailed, ErrHR(hr));

        TimerStop(TIME_ASSEMBLY);
    } else if (!options.m_fNOCODEGEN) {
        // Error about including resources in non-Assembly
        PRESFILESYM res;
        for (pOutfile = GetFirstOutFile(); pOutfile != NULL; pOutfile = pOutfile->nextOutfile()) {
            for( res = pOutfile->firstResfile(); res != NULL; res = res->nextResfile()) {
                Error( NULL, ERR_CantRefResource, res->name->text);
            }
        }
    }

    if (options.m_fINCBUILD && !ErrorCount()) {
        TimerStart(TIME_INCBUILD);
        BOOL firstOutfile = true;

        // If test flags is set dump log
        if (options.logIncrementalBuild)
            printf("Incremental Build: Build succeeded, writing incremental info.\n");

        if (!options.m_fNOCODEGEN) {
            for (pOutfile = files.Reset(this); pOutfile != NULL; pOutfile = files.Next())
            {
                //
                // Write incremental build information
                //
                pOutfile->incbuild->WriteIncrementalInfo(firstOutfile);
                firstOutfile = false;
            }
        }
        TimerStop(TIME_INCBUILD);
    }

    if (ReportProgress(progressSink, PHASE_WRITEINCFILE, 1, 1))
        goto ENDCOMPILE;

ENDCOMPILE:;
    TimerStop(TIME_COMPILE);
}


/*
 * Add a conditional symbol to be processed by the lexer.
 */
void COMPILER::AddConditionalSymbol(PNAME name)
{
    ccsymbols.Define (name);
}


/*
 * Helper routine for SearchPath. Tries the Unicode version,
 * than backs off to the ANSI version. Returns TRUE on success.
 * will NOT return a directory!
 */

BOOL SearchPathDual(LPCWSTR lpPath, LPCWSTR lpFileName, DWORD nBufferLength, LPWSTR lpBuffer)
{
    LPWSTR wszLocalBuffer = NULL;
    DWORD r;
    LPWSTR dummy;

    wszLocalBuffer = (LPWSTR)_alloca(nBufferLength * sizeof(WCHAR));

    // Try the unicode version.
    r = SearchPathW(lpPath, lpFileName, NULL, nBufferLength, wszLocalBuffer, &dummy);
    if (r) {
        W_GetActualFileCase(wszLocalBuffer, lpBuffer);
        return TRUE;
    }


    return FALSE;
}

/*
 * Private helper function to add one metadata file with a given assembly id.
 */
void COMPILER::AddOneMetadataFile(LPWSTR filename, ULONG assemblyIndex)
{
    symmgr.CreateMDFile(filename, assemblyIndex, symmgr.GetMDFileRoot());
}

/*
 * Add all files in the list as INFILE symbols.
 * The meta-data files to import are listed as a ';' seperated list of file. Traverse the list,
 * search for files on the search path, give error for ones that are missing, and
 * add the rest in as symbols.
 */
void COMPILER::AddMetadataFiles(LPWSTR fileList, ULONG assemblyIndex)
{
    LPWSTR file;

    while (fileList) {
        // Break off the next file name to process.
        file = fileList;
        fileList = wcschr(file, L';');
        if (fileList) {
            *fileList = L'\0';
            ++fileList;
        }

        AddMetadataFile (file, assemblyIndex);
    }
}

void COMPILER::AddMetadataFile (PCWSTR pszFile, ULONG assemblyIndex)
{
    WCHAR   szFull[MAX_PATH];

    if (SearchPathDual (GetSearchPath(), pszFile, MAX_PATH, szFull))
    {
        if (W_GetFileAttributes( szFull) & FILE_ATTRIBUTE_DIRECTORY)
            Error(NULL, ERR_CantIncludeDirectory, pszFile);
        else
            AddOneMetadataFile (szFull, assemblyIndex);
    }
    else
        Error (NULL, ERR_NoMetadataFile, pszFile);
}

// Add the standard metadata library, unless disabled. This goes last, so
// explicit libraries can override it.
void COMPILER::AddStandardMetadata()
{
    WCHAR fullFilename[MAX_PATH];

    if (! options.m_fNOSTDLIB) {
        // Search DLL path for the file.
        if (SearchPathDual(GetCorSystemDirectory(), L"mscorlib.dll", MAX_PATH, fullFilename)) {
            // Found it. Add it as an input file.
            AddOneMetadataFile(fullFilename, currentAssemblyIndex++ );
        }
        else {
            // File not found. Report error.
            Error(NULL, ERR_NoStdLib, L"mscorlib.dll");
        }
    }
}

// Return the dword which lives under HKCU\Software\Microsoft\C# Compiler<value>
// If any problems are encountered, return 0
DWORD COMPILER::GetRegDWORD(LPSTR value)
{
    return 0;
}

// Return true if the registry string which lives under HKCU\Software\Microsoft\C# Compiler<value>
// is the same one as the string provided.
bool COMPILER::IsRegString(LPCWSTR string, LPWSTR value)
{
    return 0;
}

const LPCWSTR g_stages[] = {
#define DEFINE_STAGE(stage) L###stage,
#include "stage.h"
#undef DEFINE_STAGE
};

////////////////////////////////////////////////////////////////////////////////
// CompilerExceptionFilter
//
//  This hits whenever we hit an unhandled ASSERT or GPF in the compiler
//  Here we dump the entire LOCATION stack to the error channel.
//

void COMPILER::ReportICE(EXCEPTION_POINTERS * exceptionInfo)
{
    // We shouldn't get here on a fatal error exception
    ASSERT(exceptionInfo->ExceptionRecord->ExceptionCode != FATAL_EXCEPTION_CODE);
    if (exceptionInfo->ExceptionRecord->ExceptionCode == FATAL_EXCEPTION_CODE) {
        return;
    }

    LOCATION * loc = location;
    if (loc) {
        //
        // dump probable culprit
        //
        Error(NULL, ERR_ICE_Culprit, exceptionInfo->ExceptionRecord->ExceptionCode, g_stages[loc->getStage()], exceptionInfo->ExceptionRecord->ExceptionAddress);

        //
        // dump location stack
        //
        do {

            LPCWSTR stage = g_stages[loc->getStage()];

            //
            // dump one location
            //
            SYM *symbol = loc->getSymbol();
            if (symbol) {
                //
                // we have a symbol report it nicely
                //
                clsDeclRec.errorSymbolStr(symbol, ERR_ICE_Symbol, stage);
            } else {
                INFILESYM *file = loc->getFile();
                if (file) {
                    BASENODE *node = loc->getNode();
                    if (node) {
                        //
                        // we have stage, file and node
                        //
                        clsDeclRec.errorStrFile(node, file, ERR_ICE_Node, stage);
                    } else {
                        //
                        // we have stage and file
                        //
                        Error(ERRLOC(file), ERR_ICE_File, stage);
                    }
                } else {
                    //
                    // only thing we have is the stage
                    //
                    Error(NULL, ERR_ICE_Stage, stage);
                }
            }

            loc = loc->getPrevious();
        } while (loc);
    } else {
        //
        // no location at all!
        //
        Error(NULL, ERR_ICE_Culprit, exceptionInfo->ExceptionRecord->ExceptionCode, g_stages[0], exceptionInfo->ExceptionRecord->ExceptionAddress);

    }
}

////////////////////////////////////////////////////////////////////////////////
// COMPILER::GetFirstOutFile

POUTFILESYM COMPILER::GetFirstOutFile()
{
    return symmgr.GetFileRoot()->firstChild->asOUTFILESYM();
}

void COMPILER::SetFullRebuild()
{
    if (incrementalRebuild) {
        ASSERT(!isDefined);
        incrementalRebuild = false;

        //
        // release the open metadata import interfaces
        //
        SourceOutFileIterator outfiles;
        for (outfiles.Reset(this); outfiles.Current(); outfiles.Next()) {
            if (outfiles.Current()->metaimport) {

                outfiles.Current()->metaimport->Release();
                outfiles.Current()->metaimport = NULL;
            }

            // reset the allocation map
            if (outfiles.Current()->allocMapAll) {
                outfiles.Current()->allocMapAll->Clear();
            } else {
                outfiles.Current()->allocMapAll = ALLOCMAP::Create(&globalSymAlloc);
            }
            if (outfiles.Current()->allocMapNoSource) {
                outfiles.Current()->allocMapNoSource->Clear();
            } else {
                outfiles.Current()->allocMapNoSource = ALLOCMAP::Create(&globalSymAlloc);
            }

            InFileIterator infiles;
            for (infiles.Reset(outfiles.Current()); infiles.Current(); infiles.Next()) {

                if (infiles.Current()->metaimport) {
                    infiles.Current()->metaimport = NULL;
                }

                // ensure we have a parse tree for the source file
                if (!infiles.Current()->nspace) {
                    DeclareOneFile(infiles.Current());
                }
                infiles.Current()->hasChanged = true;

                // reset the allocation map
                if (infiles.Current()->allocMap) {
                    infiles.Current()->allocMap->Clear();
                } else {
                    infiles.Current()->allocMap = ALLOCMAP::Create(&globalSymAlloc);
                }

                //
                // reset the metadata tokens for types we did read in
                //
                TypeIterator types;
                AGGSYM * cls;
                for (cls = types.Reset(infiles.Current()); cls; cls = types.Next()) {

                    cls->tokenEmit = mdTokenNil;
                    cls->tokenImport = mdTokenNil;

                    //
                    // ensure that we've got incremental info allocated for all types
                    //
                    if (!cls->incrementalInfo) {
                        cls->incrementalInfo = (INCREMENTALTYPE*) compiler()->globalSymAlloc.AllocZero(sizeof(*cls->incrementalInfo));

                        cls->incrementalInfo->dependantFiles = BITSET::create(compiler()->cInputFiles, &compiler()->globalSymAlloc, 0x00);
                        cls->incrementalInfo->cls = cls;
                        cls->incrementalInfo->token = mdTokenNil;
                    }
                    ASSERT(cls->incrementalInfo && cls->incrementalInfo->cls == cls);
                    ASSERT(cls->incrementalInfo->dependantFiles != 0);

                    cls->incrementalInfo->token = mdTokenNil;

                    FOREACHCHILD(cls, child)
                        mdToken *emitLocation = child->getTokenEmitPosition();
                        if (emitLocation) {
                            *emitLocation = mdTokenNil;
                        }
                    ENDFOREACHCHILD
                }
            }
        }

        //
        // reset the dispenser to no duplicate checks
        //
        SetDispenserOptions();

        // If test flags is set dump log
        if (options.logIncrementalBuild)
            printf("Incremental Build: setting full rebuild\n");

    }
}

#define MSCORPE_NAME    MAKEDLLNAME_W(L"mscorpe")

/*
 * Loads mscorpe.dll and gets an ICeeFileGen interface from it.
 * The ICeeFileGen interface is used for the entire compile.
 */
ICeeFileGen* COMPILER::CreateCeeFileGen()
{
    // Dynamically bind to ICeeFileGen functions.
    if (!pfnCreateCeeFileGen || !pfnDestroyCeeFileGen) {

        WCHAR wszPath[MAX_PATH];
        CHAR  szPath[MAX_PATH];
        DWORD dwWLen = 0, dwLen = 0;

        wcscpy(wszPath, GetCorSystemDirectory());
        dwWLen = (DWORD) wcslen(GetCorSystemDirectory()) + 1;

        dwLen = WideCharToMultiByte(CP_ACP, 0, wcscat(wszPath, MSCORPE_NAME), -1, szPath, MAX_PATH, NULL, NULL) ;

        if (dwLen != dwWLen + wcslen(MSCORPE_NAME)) {
            Error(NULL, FTL_ComPlusInit, ErrGetLastError());
            return NULL;
        }


        hmodCorPE = LoadLibraryA(szPath);
        if (hmodCorPE) {
            // Get the required methods.
            pfnCreateCeeFileGen  = (HRESULT (__stdcall *)(ICeeFileGen **ceeFileGen)) GetProcAddress(hmodCorPE, "CreateICeeFileGen");
            pfnDestroyCeeFileGen = (HRESULT (__stdcall *)(ICeeFileGen **ceeFileGen)) GetProcAddress(hmodCorPE, "DestroyICeeFileGen");
            if (!pfnCreateCeeFileGen || !pfnDestroyCeeFileGen)
                Error(NULL, FTL_ComPlusInit, ErrGetLastError());
        }
        else {
            // MSCorPE.DLL wasn't found.
            Error(NULL, FTL_RequiredFileNotFound, wszPath);
        }
    }

    ICeeFileGen *ceefilegen = NULL;
    TimerStart(TIME_CREATECEEFILEGEN);
    HRESULT hr = pfnCreateCeeFileGen(& ceefilegen);
    if (FAILED(hr)) {
        Error(NULL, FTL_ComPlusInit, ErrHR(hr));
    }
    TimerStop(TIME_CREATECEEFILEGEN);

    return ceefilegen;
}

void COMPILER::DestroyCeeFileGen(ICeeFileGen *ceefilegen)
{
    TimerStart(TIME_DESTROYCEEFILEGEN);
    pfnDestroyCeeFileGen(&ceefilegen);
    TimerStop(TIME_DESTROYCEEFILEGEN);
}

// Catches ALink warnigns and errors (the ones not returned as HRESULTS)
// and reports them
HRESULT ALinkError::OnError(HRESULT hrError, mdToken tkHint)
{
    // We only want ALink errors
    if (HRESULT_FACILITY(hrError) != FACILITY_ITF)
        return S_FALSE;

    // For now, we only want Warnings
    if (FAILED(hrError)) {
        compiler()->Error( NULL, ERR_ALinkFailed, compiler()->ErrHR(hrError));
    } else {
        switch (HRESULT_CODE(hrError)) {
        case 1056: // Ref has Culture
        case 1055: // Ref Not strongly named
        case 1053: // Improper Version string
            compiler()->Error( NULL, WRN_ALinkWarn, compiler()->ErrHR(hrError));
            break;
        default:
            return S_FALSE;
        }
    }
    return S_OK;
}

// Function to round a d to float precision. Does as an out-of-line
// function that isn't inlined so that the compiler won't optimize it
// away and keep higher precision.
void RoundToFloat(double d, float * f)
{
    *f = (float) d;
}
