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
// File: compiler.h
//
// Defined the main compiler class, which contains all the other
// sub-parts of the compiler.
// ===========================================================================

#ifndef __compiler_h__
#define __compiler_h__

#include "scciface.h"

class COMPILER;

#include "errorids.h"
#include "timing.h"
#include "posdata.h"
#include "alloc.h"
#include "namemgr.h"
#include "symmgr.h"
#include "srcmod.h"
#include "import.h"
#include "nodes.h"
#include "exprlist.h"
#include "fncbind.h"
#include "clsdrec.h"
#include "pefile.h"
#include "emitter.h"
#include "ilgen.h"
#include "incbuild.h"
#include "msgsids.h"
#include "tests.h"
#include "srcdata.h"
#include "options.h"
#include "controller.h"
#include "metaattr.h"
#include "metahelp.h"

class CInputSet;

////////////////////////////////////////////////////////////////////////////////
// HANDLE_EXCEPTION
//
// This macro should be used in all __try/__except blocks (for 2nd chance exceptions)
// In the debug build, set breakpoints on DebugExceptionFilter
// to debug cleanup-code exceptions
#ifdef _DEBUG
extern LONG DebugExceptionFilter(EXCEPTION_POINTERS *exceptionInfo, PVOID pv);
#define EXCEPT_FILTER_EX(label, b) PAL_EXCEPT_FILTER_EX(label, DebugExceptionFilter, b)
#else
#define EXCEPT_FILTER_EX(a, b) PAL_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
#endif

/*
 * This NT exception code is used to propagate a fatal
 * exception within the compiler. It has the "customer" bit
 * set and is chosen to probably not collide with any other
 * code.
 */
#define FATAL_EXCEPTION_CODE 0xE004BEBE


////////////////////////////////////////////////////////////////////////////////
// HANDLE_COMPILER_EXCEPTION (or HANDLE_NON_COMPILER_EXCEPTION if scope does not have compiler pointer)
//
// These macros should be used in all __try/__except blocks inside the compiler (for 1st chance exceptions)
// This allows for JIT debugging, as well as normal-compiler handled exceptions in both debug and retail

extern LONG CompilerExceptionFilter(EXCEPTION_POINTERS* ep, LPVOID pv);
struct CompilerExceptionFilterData
{
    COMPILER *pCompiler;  // [Optional, In] Compiler
    DWORD ExceptionCode;  // Should be set by filter function
};

#define COMPILER_EXCEPTION_TRYEX                                \
    {                                                           \
        CompilerExceptionFilterData __data;                     \
        __data.pCompiler = NULL;                                \
        PAL_TRY

#define NOCOMPILER_EXCEPTION_EXCEPT_FILTER_EX(__LabelName, __ExceptionCode)      \
        PAL_EXCEPT_FILTER_EX(__LabelName, CompilerExceptionFilter, &__data)      \
        *__ExceptionCode = __data.ExceptionCode;

#define COMPILER_EXCEPTION_EXCEPT_FILTER_EX(__LabelName, __ExceptionCode)        \
        __data.pCompiler = compiler();                                           \
        PAL_EXCEPT_FILTER_EX(__LabelName, CompilerExceptionFilter, &__data)      \
        *__ExceptionCode = __data.ExceptionCode;

#define COMPILER_EXCEPTION_ENDTRY                               \
        PAL_ENDTRY                                              \
    }


/*
 * ERRLOC location for reporting errors
 */
class ERRLOC
{
public:
    ERRLOC() :
        m_sourceData(NULL),
        m_fileName(NULL),
        m_mapFile(NULL)
    {
        start.SetEmpty();
        end.SetEmpty();
        mapStart.SetEmpty();
        mapEnd.SetEmpty();
    }

    ERRLOC(INFILESYM *inputFile, BASENODE *node) :
        m_sourceData(inputFile->pData),
        m_fileName(inputFile->name->text),
        m_mapFile(inputFile->name->text)
    {
        if (node) {
            SetLine(node);
            SetStart(node);
            SetEnd(node);
            mapStart = start;
            mapEnd = end;
            m_sourceData->GetModule()->MapLocation(&mapStart, &m_mapFile);
            m_sourceData->GetModule()->MapLocation(&mapEnd, NULL);
        }
        else {
            start.SetEmpty();
            mapEnd = mapStart = end = start;
        }
    }

    ERRLOC(SYM *sym)
    {
        if (sym->kind == SK_INFILESYM) {
            m_sourceData = sym->asINFILESYM()->pData;
            m_fileName = sym->asINFILESYM()->name->text;
            m_mapFile = m_fileName;
            start.SetEmpty();
            end.SetEmpty();
            mapStart.SetEmpty();
            mapEnd.SetEmpty();
            return;
        }

        INFILESYM *inputfile;
        inputfile = sym->getInputFile();
        if (inputfile) {
            m_sourceData = inputfile->pData;
            m_fileName = inputfile->name->text;
        }
        else {
            m_sourceData = NULL;
            m_fileName = NULL;
        }
        if (sym->getParseTree()) {
            SetLine(sym->getParseTree());
            SetStart(sym->getParseTree());
            SetEnd(sym->getParseTree());
            mapStart = start;
            mapEnd = end;
            ASSERT (m_sourceData);
            m_sourceData->GetModule()->MapLocation(&mapStart, &m_mapFile);
            m_sourceData->GetModule()->MapLocation(&mapEnd, NULL);
        }
        else {
            m_mapFile = m_fileName;
            start.SetEmpty();
            mapEnd = mapStart = end = start;
        }
    }
    ERRLOC(CSourceData *sourceData, POSDATA start, POSDATA end) :
        m_sourceData(sourceData),
        start(start),
        end(end),
        mapStart(start),
        mapEnd(end)
    {
        ICSSourceText *srcText;
        m_sourceData->GetModule()->GetSourceText(&srcText);
        srcText->GetName(&m_fileName);
        srcText->Release();
        m_sourceData->GetModule()->MapLocation(&mapStart, &m_mapFile);
        m_sourceData->GetModule()->MapLocation(&mapEnd, NULL);
    }

    ERRLOC(LPCWSTR filename, POSDATA start, POSDATA end) :
        m_sourceData(NULL),
        start(start),
        end(end),
        mapStart(start),
        mapEnd(end)
    {
            m_mapFile = m_fileName = filename;
    }

    void            SetStart(BASENODE* node);
    void            SetLine(BASENODE* node);
    void            SetEnd(BASENODE* node);


    LPCWSTR         fileName() const      { return m_fileName; }
    LPCWSTR         mapFile() const       { return m_mapFile; }
    BOOL            hasLocation() const   { return !start.IsEmpty(); }
    int             line() const          { return !start.IsEmpty() ? (int) start.iLine : -1; }
    int             mapLine() const       { return !mapStart.IsEmpty() ? (int) mapStart.iLine : -1; }
    int             column() const        { return !start.IsEmpty() ? (int) start.iChar : -1; }
    int             extent() const        { return start.iLine == end.iLine ? end.iChar - start.iChar : 1; }
    ICSSourceData * sourceData() const    { return m_sourceData; }

private:
    CSourceData *   m_sourceData;
    LPCWSTR         m_fileName;
    LPCWSTR         m_mapFile;
    POSDATA         start;
    POSDATA         end;
    POSDATA         mapStart;
    POSDATA         mapEnd;
};

/*
 * ALink Error catcher/filter
 * All the interesting code is in ALinkError::OnError
 */
class ALinkError : public IMetaDataError {
public:
    STDMETHOD_(ULONG, AddRef) () { return 1; }
    STDMETHOD_(ULONG, Release) () { return 1; }
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID* obp) {
        if (obp == NULL)
            return E_POINTER;
        if (riid == IID_IMetaDataError) {
            *obp = (IMetaDataError*)this;
        } else if (riid != IID_IUnknown) {
            *obp = (IUnknown*)this;
        } else {
            return E_NOINTERFACE;
        }
        return S_OK;
    }
    STDMETHOD(OnError)(HRESULT hrError, mdToken tkHint);

    COMPILER* compiler();
};

HANDLE OpenFileEx( LPCWSTR filename, DWORD *fileLen);

/*
 * The big kahuna -- the main class that holds everything together.
 */
class COMPILER: public ALLOCHOST {
public:
    //OPTIONS options;            // Current compiler options.
    COptionData options;        // Compiler options, kindly provided by our controller
    ULONG  currentAssemblyIndex;    // index of the next imported assembly. Used for determining
                                    // assembly equality before assembly info has been emitted

    CController *pController;   // This is our parent "controller"

    // Embedded components of the compiler.
    PAGEHEAP &pageheap;         // heap for allocating pages.  NOTE:  This is a reference to the page heap in our controller!
    MEMHEAP globalHeap;         // General heap for allocating global memory.  This heap is SPECIFIC TO THIS COMPILER INSTANCE!  When this COMPILER is destroyed, so is this heap.
    NRHEAP globalSymAlloc;      // Allocator for global symbols & names.  Again, specific to this compiler instance.
    NRHEAP localSymAlloc;       // Allocator for local symbols & names.  Again, specific to this compiler instance.
    FUNCBREC funcBRec;          // Record for compiling an individual function
    CLSDREC clsDeclRec;         // Record for declaring a namespace & classes in it
    ILGENREC ilGenRec;          // Record for generating il for a method
    ALinkError alError;         // Catches ALink OnError calls

    NAMEMGR  *namemgr;          // Name table (NOTE:  This is a referenced pointer!)
    SYMMGR   symmgr;            // Symbol manager
    IMPORTER importer;          // Metadata importer
    EMITTER  emitter;           // the file emitter.
    NAMETAB  ccsymbols;         // table of CC symbols
    CPEFile   *curFile;          // The file currently being emitted
    CPEFile   assemFile;         // The file with the Assembly Manifest
    GLOBALATTRSYM *assemblyAttributes;     // the attributes for the current assembly
    mdAssembly assemID;         // The global assembly ID for the ALink interface

    ULONG    cInputFiles;       // Number of input files
    ULONG    cOutputFiles;      // Number of output files

    ICSCompilerHost * host;           // compiler host.
    ICSCommandLineCompilerHost * cmdHost;
    IALink          * linker;         // Assembly linker
    /*
     * the current compiler stage
     */
    enum STAGE
    {
#define DEFINE_STAGE(stage) stage,
#include "stage.h"
#undef DEFINE_STAGE
    };

    /*
     * CONSIDER: may want to remove this for release builds
     */
    class LOCATION *    location;       // head of current location stack

    // Create/destruction.
    void * operator new(size_t size);
    void operator delete(void * p);
    COMPILER (CController *pController, PAGEHEAP &ph, NAMEMGR *pNameMgr);
    ~COMPILER();

    HRESULT     Init ();
    HRESULT     InitWorker ();
    void        Term (bool normalTerm);

    void DiscardLocalState();

    // Error handling methods.
    CError  * __cdecl MakeError (int id, ...);
    CError  *  MakeError (int id, va_list args);
    CError  *  MakeErrorWithWarnOverride (int id, va_list args, int warnLevelOverride);
    void    AddLocationToError (CError *pError, const ERRLOC *pErrLoc);
    void    AddLocationToError (CError *pError, const ERRLOC& pErrLoc);
    void    SubmitError (CError *pError);
    void __cdecl Error(const ERRLOC& location, int id, ...);
    void __cdecl Error(const ERRLOC *location, int id, ...);
    void    ErrorWithList(const ERRLOC *location, int id, va_list args);

    void ReportICE(EXCEPTION_POINTERS * exceptionInfo);
    void HandleException(DWORD exceptionCode);
    static DWORD GetRegDWORD(LPSTR value);
    static bool IsRegString(LPCWSTR string, LPWSTR value);
    LPWSTR ErrHR(HRESULT hr, bool useGetErrorInfo = true);
    LPWSTR ErrGetLastError();
    LPWSTR ErrSym(PSYM sym);
    LPWSTR ErrDelegate(PAGGSYM sym);
    LPWSTR ErrParamList(int cParams, TYPESYM **params, bool isVarargs, bool isParamArray);
    LPWSTR ErrSK(SYMKIND sk);
    LPCWSTR ErrName(PNAME name);
    LPCWSTR ErrNameNode(BASENODE *name);
    LPCWSTR ErrTypeNode(TYPENODE *type);
    LPWSTR ErrId(int id);
    LPCWSTR ErrAccess(ACCESS acc);
    void NotifyHostOfBinaryFile(LPCWSTR filename)   { if (cmdHost) cmdHost->NotifyBinaryFile(filename); }
    void NotifyHostOfMetadataFile(LPCWSTR filename)   { if (cmdHost) cmdHost->NotifyMetadataFile(filename, GetCorSystemDirectory()); }


    long        ErrorCount () { return pController->ErrorsReported(); }
    bool        GetCheckedMode () { return !!(options.m_fCHECKED); } // for now...
    HRESULT     Compile (ICSCompileProgress * progressSink);
    LPCWSTR     GetOutputFileName ();

    // ALLOCHOST methods
    void        NoMemory () { Error (NULL, FTL_NoMemory); }
    MEMHEAP     *GetStandardHeap () { return &globalHeap; }
    PAGEHEAP    *GetPageHeap () { return &pageheap; }

#ifdef TRACKMEM
    unsigned int uParseMemory;
#endif

    // Miscellaneous.
    IMetaDataDispenserEx * GetMetadataDispenser();
    LPCWSTR GetCorSystemDirectory();
    POUTFILESYM GetFirstOutFile();

    HRESULT     AddInputSet (CInputSet *pSet);
    HRESULT     AddInputSetWorker (CInputSet *pSet);
    HRESULT     AddImport (PCWSTR pszFileName, ULONG assemblyIndex);
    void        AddMetadataFile(PCWSTR file, ULONG assemblyIndex);  // Used by the Manifest to add extra files
    void        ResetErrorBuffer () { errBufferNext = errBuffer; }

    bool        ScanAttributesForCLS(GLOBALATTRSYM * attributes);
    bool        CheckForCLS() { return checkCLS; }
    bool        BuildAssembly() { return m_bAssemble; }

    // Incremental rebuild
    bool        IsIncrementalRebuild() { return incrementalRebuild; }
    bool        HaveTypesBeenDefined() { return isDefined; }
    void        MakeFileDependOnType(PARENTSYM *file, AGGSYM *context);
    void        SetFullRebuild();
    void *      AllocateRVABlock(INFILESYM *infile, ULONG cbCode, ULONG alignment, ULONG *codeRVA, ULONG *offset);
    void        EnsureTypeIsDefined(AGGSYM *cls);
    unsigned    numberOfSourceFiles() { return (cInputFiles - symmgr.GetMDFileRoot()->cInputFiles); }
    INFILESYM **inputFilesById; // inputfiles ordered by incrmenetal build id

    // Edit And Continue

#define TEMPORARY_NAME_PREFIX L"CS$"

    bool inEncMode() { return false; }

#ifdef DEBUG
    TESTS tests;
    bool        haveDefinedAnyType;
#endif //DEBUG

    ICeeFileGen        *CreateCeeFileGen();
    void                DestroyCeeFileGen(ICeeFileGen *ceefile);

private:
    bool incrementalRebuild;    // If true, the output files will be updated incrementally.
                                // If false, the output files will be fully rebuilt.
    bool isDefined;             // only valid if incrementalRebuild is true
                                // true if we've passed the define stage
    unsigned    cChangedFiles;  // number of changed input files
    INFILESYM * changedFiles;   // list of files which need constants evaled
    bool CheckChangedOverThreshold();
    bool checkCLS;              // If true, the global CLS compliant attribute is set and we should enforce CLS compliance.
                                // If false, compile anything (don't care about CLS rules).
    bool m_bAssemble;           // True if we are building an assembly (i.e. the first output file is NOT a module)

    // Compiler hosting data.
    ULONG cRef;                       // COM reference count
    short isInited;                   // Have we been initialized (and how far did Init() succeed)?
    bool isCanceled;                  // Has compile been canceled?

    // General compilation.
    void CompileAll(ICSCompileProgress * progressSink);
    void ParseOneFile(INFILESYM *infile);
    void DeclareOneFile(INFILESYM *infile);
    void ResolveInheritanceHierarchy();
    void DefineMembers();
    void DefineOneFile(INFILESYM *pInfile);
    void DefineFiles(class IInfileIterator &iter);
    void EvaluateConstants(INFILESYM *pInfile);
    void CheckCLS();
    void PostCompileChecks();
    void EmitTokens(OUTFILESYM *outfile);

    void DeclareTypes(ICSCompileProgress * progressSink);
    HRESULT CopyRVAHeap(OUTFILESYM *pOutfile);
    void AddOneMetadataFile(LPWSTR filename, ULONG assemblyIndex);
    void AddMetadataFiles(LPWSTR fileList, ULONG assemblyIndex);
    void AddStandardMetadata();
    void AddConditionalSymbol(PNAME name);
    COMPILER * compiler()                     { return this; }
    void SetDispenserOptions();
    bool ReportProgress(ICSCompileProgress * progressSink, int phase, int itemsComplete, int itemsTotal);

    // Error handling methods and data (error.cpp)
    //int cWarn, cError;
    // Buffer for accumulating error messages; cleared when error is reported.
    // 2MB is Reservced and individual pages are committed as needed
#define ERROR_BUFFER_MAX_WCHARS (1024*1024)
#define ERROR_BUFFER_MAX_BYTES  (ERROR_BUFFER_MAX_WCHARS*sizeof(WCHAR))
    friend LONG CompilerExceptionFilter(EXCEPTION_POINTERS* exceptionInfo, LPVOID pvData );
    WCHAR * errBuffer;
    WCHAR * errBufferNext;
    BYTE * errBufferNextPage;
    int ErrBufferLeft()
        { return (int)(ERROR_BUFFER_MAX_WCHARS - (errBuffer - errBufferNext)); }

    //void ShowErrorMessage(ERRORKIND errkind, ERRLOC *location, int id, va_list args);
    void ThrowFatalException();

    // Miscellaneous.
    IMetaDataDispenserEx * dispenser;
    NAME *            corSystemDirectory;
    LPCWSTR GetSearchPath();
    LPWSTR m_pszLibPath;

    // No import library is provided for these guys, so we
    // bind to them dynamically.
    HRESULT (__stdcall *pfnCreateCeeFileGen)(ICeeFileGen **ceeFileGen); // call this to instantiate
    HRESULT (__stdcall *pfnDestroyCeeFileGen)(ICeeFileGen **ceeFileGen); // call this to delete
    HMODULE hmodCorPE;

};


/*
 * Define inline routine for getting to the COMPILER class from
 * various embedded classes.
 */

__forceinline COMPILER * SYMMGR::compiler()
{ return (COMPILER *) (((BYTE *)this) - offsetof(COMPILER, symmgr)); }

__forceinline COMPILER * IMPORTER::compiler()
{ return (COMPILER *) (((BYTE *)this) - offsetof(COMPILER, importer)); }

__forceinline COMPILER * FUNCBREC::compiler()
{ return (COMPILER *) (((BYTE *)this) - offsetof(COMPILER, funcBRec)); }

__forceinline COMPILER * CLSDREC::compiler()
{ return (COMPILER *) (((BYTE *)this) - offsetof(COMPILER, clsDeclRec)); }

__forceinline COMPILER * EMITTER::compiler()
{ return (COMPILER *) (((BYTE *)this) - offsetof(COMPILER, emitter)); }

__forceinline COMPILER * ILGENREC::compiler()
{ return (COMPILER *) (((BYTE *)this) - offsetof(COMPILER, ilGenRec)); }

__forceinline COMPILER * ALinkError::compiler()
{ return (COMPILER *) (((BYTE *)this) - offsetof(COMPILER, alError)); }

__forceinline CController * CLSDREC::controller() { return compiler()->pController; }


__forceinline void COMPILER::MakeFileDependOnType(PARENTSYM *file, AGGSYM *context)
{
    if (this->options.m_fINCBUILD && context->incrementalInfo) {
        context->incrementalInfo->dependantFiles = context->incrementalInfo->dependantFiles->setBit(file->getInputFile()->idIncbuild + 1);
    }
}

/*
 *   My DLL module handle.
 */
extern HINSTANCE hModuleMine;

/*
 *   My message DLL module handle.
 */
extern HINSTANCE hModuleMessages;

/*
 * LOCATION for reporting errors
 *
 * they link together in a stack, top of the stack
 * is the most recent location.
 *
 * they should only be created as local variables
 * using the SETLOCATIONXXX(xxx) macros below
 *
 * these guys are designed for fast construction/destruction
 * and slow query since they are only queried after an ICE
 *
 * CONSIDER:
 *
 *  - making an empty version of the stage/location for release builds
 *  - making super debug/loggin versions of SETLOCATIONXXX(xxx)
 *
 */
class LOCATION
{
public:

            ~LOCATION()                     { *listHead = previousLocation; }

    virtual SYM *           getSymbol()     = 0;
    virtual INFILESYM *     getFile()       = 0;
    virtual BASENODE *      getNode()       = 0;
    virtual COMPILER::STAGE getStage()      { return getPrevious()->getStage(); }

            LOCATION *      getPrevious()   { return previousLocation; }

protected:

    LOCATION(LOCATION **lh) : previousLocation(*lh), listHead(lh)
    {
        *listHead = this;
    }

private:

    LOCATION *  previousLocation;
    LOCATION ** listHead;
};

class SYMLOCATION : LOCATION
{
public:

    SYMLOCATION(LOCATION **lh, SYM *sym) :
        LOCATION(lh),
        symbol(sym)
    {}

    SYM *           getSymbol() { return symbol; }
    INFILESYM *     getFile()   { return symbol->getInputFile(); }
    BASENODE *      getNode()   { return symbol->getParseTree(); }

private:

    SYM *       symbol;

};

class NODELOCATION : LOCATION
{
public:

    NODELOCATION(LOCATION **lh, BASENODE *n) :
        LOCATION(lh),
        node(n)
    {}

    SYM *           getSymbol() { return NULL; }
    INFILESYM *     getFile()   { return getPrevious()->getFile(); }
    BASENODE *      getNode()   { return node; }

private:

    BASENODE *      node;

};

class FILELOCATION : LOCATION
{
public:

    FILELOCATION(LOCATION **lh, INFILESYM * f) :
        LOCATION(lh),
        file(f)
    {}

    SYM *           getSymbol() { return NULL; }
    INFILESYM *     getFile()   { return file; }
    BASENODE *      getNode()   { return NULL; }

private:

    INFILESYM *     file;

};

class STAGELOCATION : LOCATION
{
public:

    STAGELOCATION(LOCATION **lh, COMPILER::STAGE s) :
        LOCATION(lh),
        stage(s)
    {}

    SYM *           getSymbol() { return getPrevious() ? getPrevious()->getSymbol() : NULL; }
    INFILESYM *     getFile()   { return getPrevious() ? getPrevious()->getFile()   : NULL; }
    BASENODE *      getNode()   { return getPrevious() ? getPrevious()->getNode()   : NULL; }
    COMPILER::STAGE getStage()  { return stage; }

private:

    COMPILER::STAGE stage;

};

#define SETLOCATIONFILE(file)   FILELOCATION    _fileLocation(&compiler()->location,    (file));
#define SETLOCATIONNODE(node)   NODELOCATION    _nodeLocation(&compiler()->location,    (node));
#define SETLOCATIONSYM(sym)     SYMLOCATION     _symLocation (&compiler()->location,    (sym));
#define SETLOCATIONSTAGE(stage) STAGELOCATION   _stageLocation(&compiler()->location,   (COMPILER::stage));


// Utility function I couldn't find a better place for.
extern void RoundToFloat(double d, float * f);
extern BOOL SearchPathDual(LPCWSTR lpPath, LPCWSTR lpFileName, DWORD nBufferLength, LPWSTR lpBuffer);
extern HMODULE FindAndLoadHelperLibrary(LPCSTR filename);


__forceinline void COMPILER::EnsureTypeIsDefined(AGGSYM *cls)
{
    if (!cls->isDefined) {
        INFILESYM *infile = cls->getInputFile();
        if (infile->isSource && !this->isDefined) {
            // may need to parse the file in the incremental scenario
            if (!cls->parseTree) {
                ASSERT(this->incrementalRebuild);
                DeclareOneFile(infile);
            }
            ASSERT(cls->parseTree);

            DefineOneFile(infile);
            ASSERT(infile->isDefined);
        } else {
            // import from metadata
            importer.DefineImportedType(cls);
        }
    }
    ASSERT(cls->isDefined);
}

#endif //__compiler_h__

