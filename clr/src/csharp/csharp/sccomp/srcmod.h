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
// File: srcmod.h
//
// ===========================================================================

#ifndef __srcmod_h__
#define __srcmod_h__

#include "scciface.h"
#include "tokinfo.h"
#include "alloc.h"
#include "nodes.h"
#include "locks.h"
#include "namemgr.h"
#include "table.h"
#include "escape.h"
#include "map.h"
#include "lexer.h"
#include "parser.h"
#include "array.h"

struct SrcModExceptionFilterData
{
    CController *pController;  // [Optional, In] Compiler
    DWORD ExceptionCode;       // Should be set by filter function
};

#define SRCMOD_EXCEPTION_TRYEX                                \
    {                                                         \
        SrcModExceptionFilterData __data;                     \
        __data.pController = NULL;                            \
        PAL_TRY

#define SRCMOD_EXCEPTION_EXCEPT_FILTER_EX(__LabelName, __ExceptionCode)  \
        __data.pController = m_pController;                              \
        PAL_EXCEPT_FILTER_EX(__LabelName, ExceptionFilter, &__data)      \
        *__ExceptionCode = __data.ExceptionCode;

#define SRCMOD_EXCEPTION_ENDTRY                               \
        PAL_ENDTRY                                            \
    }

class CSourceModule;

////////////////////////////////////////////////////////////////////////////////
// CSourceLexer
//
// This is the derivation of CLexer that is used by source modules to do
// the main lexing of a source file.

class CSourceLexer : public CLexer
{
private:
    CSourceModule   *m_pModule;

public:
    CSourceLexer (CSourceModule *pMod);

    // Context-specific implementation...
    void            TrackLine (PCWSTR pszNewLine);
    void __cdecl    ErrorAtPosition (long iLine, long iCol, long iExtent, short iErrorId, ...);
    BOOL            ScanPreprocessorLine (PCWSTR &p);
    void            RecordCommentPosition (const POSDATA &pos);
};

////////////////////////////////////////////////////////////////////////////////
// CSourceParser
//
// This is the derivation of CParser that is used by source modules for parsing.

class CSourceParser : public CParser
{
private:
    CSourceModule   *m_pModule;

public:
    CSourceParser (CSourceModule *pMod);

    // Virtuals from CParser
    void        RescanToken (long iToken, FULLTOKEN *pFT);
    void        *MemAlloc (long iSize);
    void        CreateNewError (short iErrorId, va_list args, const POSDATA &posStart, const POSDATA &posEnd);
    void        AddToNodeTable (BASENODE *pNode);
    void        GetInteriorTree (CSourceData *pData, BASENODE *pNode, ICSInteriorTree **ppTree);
};



class CController;
struct BASENODE;
class CInteriorNode;
class CSourceData;
class CErrorContainer;
enum PPTOKENID;

////////////////////////////////////////////////////////////////////////////////
// THREADDATAREF

struct THREADDATAREF
{
    DWORD       dwTid;
    long        iRef;
};

////////////////////////////////////////////////////////////////////////////////
// SKIPFLAGS -- flags directing behavior of, and indicating results of, SkipBlock()

enum SKIPFLAGS
{
    // Behavior direction flags
    SBF_INACCESSOR      = 0x00000001,   // The block to be skipped is a getter/setter

    // Result flags
    SB_NORMALSKIP       = 0,            // Normal skip; no errors
    SB_NOTAMEMBER       = 0,            // Returned by ScanMemberDeclaration in failure case
    SB_TYPEMEMBER,                      // Detected a type member declaration (beginning at current token)
    SB_NAMESPACEMEMBER,                 // Detected a namespace member declaration (at current token)
    SB_ACCESSOR,                        // Detected an accessor (at current token)
    SB_ENDOFFILE,                       // Hit end of file (error reported)
};

////////////////////////////////////////////////////////////////////////////////
// CCFLAGS
//
// Flags indicating nuances of CC state changes and stack records

enum CCFLAGS
{
    CCF_REGION  = 0x0001,           // Record is for #region (as opposed to #if)
    CCF_ELSE    = 0x0002,           // Indicates an #else block (implies !CCF_REGION)
    CCF_ENTER   = 0x0004,           // State change only -- indicates entrance (push) as opposed to exit (pop)
};

////////////////////////////////////////////////////////////////////////////////
// CCREC -- conditional compilation stack record.  This includes #region
// directives as well.

struct CCREC
{
    DWORD       dwFlags;            // CCF_* flags
    CCREC       *pPrev;             // Previous state record
};

////////////////////////////////////////////////////////////////////////////////
// CCSTATE -- conditional compilation state change record

struct CCSTATE
{
    unsigned long   dwFlags:3;      // CCF_* flags
    unsigned long   iLine:29;       // Line number of CC directive causing change
};


////////////////////////////////////////////////////////////////////////////////
// CNodeTable, NODECHAIN

struct NODECHAIN
{
    long        iGlobalOrdinal;     // Index of this node in global-ordinal array
    NODECHAIN   *pNext;             // Next node with the same key (key-ordinal + 1)
};

typedef CTable<NODECHAIN>   CNodeTable;

////////////////////////////////////////////////////////////////////////////////
// LEXDATABLOCK
//
// NOTE:  This structure is used by the source module to represent all data
// calculated and/or generated by lexing.  It derives from LEXDATA, which is the
// exposed version, and also includes some extra goo.

struct LEXDATABLOCK : public LEXDATA
{
    // Source text
    long        iTextLen;

    // Token stream
    long        iAllocTokens;

    // Line offset table
    long        iAllocLines;

    // Comment table
    long        iAllocComments;

    // CC states
    CCSTATE     *pCCStates;
    long        iCCStates;
};

////////////////////////////////////////////////////////////////////////////////
// MERGEDATA

struct MERGEDATA
{
    long        iTokStart;              // First token in token stream to be scanned
    long        iTokStop;               // Last token to be rescanned (in incremental retok is successful)
    BOOL        fStartAtZero;           // TRUE if scanner is starting at 0,0
    POSDATA     posStart;               // Position to start scanning at
    TOKENID     tokFirst;               // Token ID of iTokStart (old)
    POSDATA     posStop;                // Position (relative to NEW buffer) of stop token (if retok is successful)
    POSDATA     posStopOld;             // Same as above, only relative to OLD buffer

    POSDATA     posEditOldEnd;          // Copy of m_posEditOldEnd
    POSDATA     posEditNewEnd;          // Copy of m_posEditNewEnd
    POSDATA     posOldEOF;              // Position of TID_ENDFILE (old buffer)

    BOOL        fIncrementalStop;       // TRUE if retokenization stopped successfully
    long        iLineDelta;             // Change in line count
    long        iCharDelta;             // Change in character position of first unmodified character
    long        iStreamOldEnd;          // STREAM position of first unmodified character (old)

    long        iFirstAffectedCCState;  // First CC state record affected by change (calculated in ReconstructCCStack)
    BOOL        fStreamChanged;         // TRUE if token stream changed
    BOOL        fTransitionsChanged;    // TRUE if transition lines have changed

    long        iStartComment;          // First changed comment
    long        iOldEndComment;         // First unchanged comment (in old table)
    long        iNewEndComment;         // First unchanged comment (in new table)
};




////////////////////////////////////////////////////////////////////////////////
// CSourceModule
//
// This is the object that implements ICSSourceModule.  It also holds all of
// the data to which callers can access ONLY through ICSSourceData objects.
//
// Clearly, there is a LOT of state contained in a source module.  Most of this
// has to do with the complexities involved in incremental lexing, compounded by
// free-threadedness.
//
// When adding any new state/member variables, please try to conform to the
// organization of logical groupings, and be aware of multi-threaded access and
// whether or not your change affects incremental lexing.

class CSourceModule :
    public ICSSourceModule
{
    friend class CSourceLexer;
    friend class CSourceParser;

    enum { COMMENT_BLOCK_SIZE = 32 };       // NOTE:  Must be power of 2...

private:
    // Reference counting (per-thread and global)
    ULONG           m_iRef;                         // Ref count
    long            m_iDataRef;                     // Data ref count (outstanding ICSSourceData object count)
    THREADDATAREF   *m_pThreadDataRef;              // Per-thread data ref counts
    long            m_iThreadCount;                 // Number of threads represented in array

    // Owner
    CController     *m_pController;                 // Owning controller

    // Source text and edit data
    ICSSourceText   *m_pSourceText;                 // Source text object
    DWORD_PTR       m_dwTextCookie;                 // Event cookie for edit events from text
    POSDATA         m_posEditStart;                 // Accumulated edit start point
    POSDATA         m_posEditOldEnd;                // Accumulated edit old end point
    POSDATA         m_posEditNewEnd;                // Accumulated edit new end point

    // NEW
    // Lexing data
    LEXDATABLOCK    m_lex;                          // All lexing data

    // CC data
    NAMETAB         m_tableDefines;                 // Table of defined preprocessor symbols
    CCREC           *m_pCCStack;                    // Stack of CC records

    // State lock
    CTinyLock       m_StateLock;                    // Lock for state data

    // Incremental lexing data
    DWORD           m_dwVersion;                    // "Version" of the current token stream
    long            m_iTokDelta;                    // Change in token count (if m_fTokensChanged)
    long            m_iTokDeltaFirst;               // First changed token (if m_fTokensChanged)
    long            m_iTokDeltaLast;                // Last changed token (in NEW stream) (if m_fTokensChanged)

    // Data accumulated during parse
    BASENODE                    *m_pTreeTop;        // Top of parse tree
    CNodeTable                  *m_pNodeTable;      // Table of key-indexable nodes
    CStructArray<KEYEDNODE>     *m_pNodeArray;      // Array of key-indexable nodes
    long                        m_iErrors;          // Used ONLY to detect errors report during some time span...

    // Heaps
    MEMHEAP         *m_pStdHeap;                    // Standard heap (from controller)
    NRHEAP          m_heap;                         // Heap for allocating nodes
    NRMARK          m_markInterior;                 // Start point for interior node parse
    NRHEAP          *m_pActiveHeap;                 // "Active" allocation heap

    // Interior node parsing
    BASENODE        *m_pInterior;                   // Last-parsed interior node

    // #line data
    CLineMap        m_LineMap;                      // Map of source lines to #lines
    PWSTR           m_pszSrcDir;                    // Directory of source file (used only for LineMap)
    NAME            *m_pDefaultName;                // "default"
    NAME            *m_pHiddenName;                 // "hidden"

    // Parser data
    CSourceParser   *m_pParser;                     // This would be our parser...
    long            m_iCurTok;                      // Current token

    // Error tracking
    CErrorContainer *m_pTokenErrors;                // Container for EC_TOKENIZATION errors
    CErrorContainer *m_pParseErrors;                // Container for EC_TOPLEVELPARSE errors
    CErrorContainer *m_pCurrentErrors;              // Depending on mode, points to one of the above or an interior parse container

    // Bits/Flags/etc...
    //
    // These bits are protected by m_StateLock
    unsigned        m_fTextModified:1;              // TRUE if text has been modified since last tokenization
    unsigned        m_fEventsArrived:1;             // TRUE if text change events have arrived since last attempt to acquire text
    unsigned        m_fEditsReceived:1;             // TRUE if text change events have arrived since creation
    unsigned        m_fTextStateKnown:1;            // TRUE if mods to text are known in m_posEdit* vars (i.e. no disconnects since last tokenization)
    unsigned        m_fInteriorHeapBusy:1;          // TRUE if a CPrimaryInteriorNode exists
    unsigned        m_fTokenizing:1;                // TRUE if a thread has committed to tokenizing current text
    unsigned        m_fTokenized:1;                 // TRUE if token stream for current text is available
    unsigned        m_fParsingTop:1;                // TRUE if a thread has committed to parsing the top level
    unsigned        m_fParsedTop:1;                 // TRUE if the top level parse tree for the current text is available
    unsigned        m_fTokensChanged:1;             // TRUE if token stream has changed since m_fParsedTop was set
    unsigned        m_fForceTopParse:1;             // TRUE if rescanned tokens contained { or } tokens
    unsigned        m_fParsingInterior:1;           // TRUE if a thread is parsing an interior node

    unsigned        m_bitpad:20;                    // This pad ensures that mods to the unprotected bits don't whack the protected ones

    // These bits are NOT PROTECTED and must be kept on the "other side" of the pad
    unsigned        m_fTrackLines:1;                // TRUE if line tracking should occur (first time tokenization; creates line table)
    unsigned        m_fLimitExceeded:1;             // TRUE if line count/length limit exceeded on last tokenization
    unsigned        m_fErrorOnCurTok:1;             // TRUE if an error has been reported on the current token
    unsigned        m_fCCSymbolChange:1;            // TRUE if a CC symbol was defined/undefined
    unsigned        m_fParsingForErrors:1;          // TRUE if in ParseForError call only

    // Memory consumption tracking                                          
    BOOL            m_fAccumulateSize;              // Set this if size accumulation is desired (during parse for errors)
    DWORD           m_dwInteriorSize;               // Accumulation of interior parse tree space
    long            m_iInteriorTrees;               // Number of interior trees parsed (during parse for errors)

#ifdef _DEBUG
    // Debug-only data
    DWORD           m_dwTokenizerThread;            // These three values are used for asserting thread behavior during parsing...
    DWORD           m_dwParserThread;
    DWORD           m_dwInteriorParserThread;
    void            SetTokenizerThread () { m_dwTokenizerThread = GetCurrentThreadId(); }
    void            ClearTokenizerThread () { m_dwTokenizerThread = 0; }
    void            SetParserThread () { m_dwParserThread = GetCurrentThreadId(); }
    void            ClearParserThread () { m_dwParserThread = 0; }
    void            SetInteriorParserThread () { m_dwInteriorParserThread = GetCurrentThreadId(); }
    void            ClearInteriorParserThread () { m_dwInteriorParserThread = 0; }
#else
    void            SetTokenizerThread () {}
    void            ClearTokenizerThread () {}
    void            SetParserThread () {}
    void            ClearParserThread () {}
    void            SetInteriorParserThread () {}
    void            ClearInteriorParserThread () {}
#endif



    // Some accessor functions for things we get from the controller
    NAMEMGR         *GetNameMgr ();
    BOOL            CheckFlags (DWORD dwFlags);
    COptionData     *GetOptions ();

    // Tokenization routines
    BOOL            WaitForTokenization ();
    long            FindNearestPosition (POSDATA *ppos, long iSize, const POSDATA &pos);
    void            DetermineStartData (LEXDATABLOCK &old, MERGEDATA &md);
    void            ReconstructCCState (LEXDATABLOCK &old, MERGEDATA &md, CCREC **ppCopy, NAMETAB *pOldTable);
    void            MergeTokenStream (LEXDATABLOCK &old, MERGEDATA &md);
    void            MergeCommentTable (LEXDATABLOCK &old, MERGEDATA &md);
    void            MergeCCState (LEXDATABLOCK &old, MERGEDATA &md);
    void            MergeLineTable (LEXDATABLOCK &old, MERGEDATA &md);
    void            UpdateRegionTable ();
    static LONG     HandleExceptionFromTokenization (EXCEPTION_POINTERS * exceptionInfo, PVOID pv);
    void            InternalCreateTokenStream ();
    void            CreateTokenStream ();
    HRESULT         GetTextAndConnect ();
    void            InternalFlush ();
    void            AcceptEdits ();
    void            TrackLine (CLexer *pCtx, PCWSTR pszNewLine);
    void            RecordCommentPosition (const POSDATA &pos);

    void            ScanPreprocessorLine (CLexer *pCtx, PCWSTR &pszCur, BOOL fFirstOnLine);
    void            ScanPreprocessorLineTerminator (CLexer *pCtx, PCWSTR &pszCur);
    void            ScanPreprocessorDirective (CLexer *pCtx, PCWSTR &pszCur);
    PPTOKENID       ScanPreprocessorToken (PCWSTR &p, PCWSTR &pszStart, NAME **ppName, bool allIds);
    void            ScanPreprocessorDeclaration (PCWSTR &p, CLexer *pCtx, BOOL fDefine);
    void            ScanPreprocessorControlLine (PCWSTR &p, CLexer *pCtx, BOOL fError);
    void            ScanPreprocessorRegionStart (PCWSTR &p, CLexer *pCtx);
    void            ScanPreprocessorRegionEnd (PCWSTR &p, CLexer *pCtx);
    void            ScanPreprocessorIfSection (PCWSTR &p, CLexer *pCtx);
    void            ScanPreprocessorIfTrailer (PCWSTR &p, CLexer *pCtx);
    void            ScanPreprocessorLineDecl (PCWSTR &p, CLexer *pCtx);
    BOOL            EvalPreprocessorExpression (PCWSTR &p, CLexer *pCtx, PPTOKENID tokPrecedence = PPT_OR);
    void            ScanExcludedCode (PCWSTR &p, CLexer *pCtx);
    void            ScanAndIgnoreDirective (PPTOKENID tok, PCWSTR pszStart, PCWSTR &p, CLexer *pCtx);
    void            SkipRemainingPPTokens (PCWSTR &p);

    void            PushCCRecord (CCREC * &pStack, DWORD dwFlags);
    BOOL            PopCCRecord (CCREC * &pStack);
    BOOL            CompareCCStacks (CCREC *pOne, CCREC *pTwo);
    void            MarkTransition (long iLine);
    void            MarkCCStateChange (long iLine, DWORD dwFlags);
    void            ApplyDelta (POSDATA posX, POSDATA posCX, POSDATA posCY, POSDATA *pposY);
    void            UpdateTokenIndexes (BASENODE *pNode, BASENODE *pChild);

    void            UnparseInteriorNode ();
    void            ParseInteriorNode (BASENODE *pNode);
    HRESULT         ParseInteriorsForErrors (CSourceData *pData, BASENODE *pNode);

    void            CreateNewError (short iErrorId, va_list args, const POSDATA &posStart, const POSDATA &posEnd);
    void            ErrorAtPosition (va_list args, long iLine, long iCol, long iExtent, short iErrorId);
    // Use LONG_PTR's here so we don't have to cast when calculating positions based on pointer arithmetic
    void __cdecl    ErrorAtPosition (LONG_PTR iLine, LONG_PTR iCol, LONG_PTR iExtent, short iErrorId, ...);

    static LONG     ExceptionFilter (EXCEPTION_POINTERS * exceptionInfo, PVOID pv);

    NAME            *BuildNodeKey (BASENODE *pNode);
    void            AddToNodeTable (BASENODE *pNode);

    CSourceModule (CController *pController);
    ~CSourceModule ();

public:
    static  HRESULT CreateInstance (CController *pController, ICSSourceText *pText, ICSSourceModule **ppStream);

    HRESULT Initialize (ICSSourceText *pText);

    // Source data reference management
    void    AddDataRef ();
    void    ReleaseDataRef ();
    long    FindThreadIndex (BOOL fMustExist);
    void    ReleaseThreadIndex (long iIndex);

    // Access to our state lock for external serialization
    CTinyLock   *GetStateLock () { return &m_StateLock; }

    // Source data accessors
    HRESULT GetLexResults (CSourceData *pData, LEXDATA *pLexData);
    HRESULT GetSingleTokenData (CSourceData *pData, long iToken, TOKENDATA *pTokenData);
    HRESULT GetSingleTokenPos (CSourceData *pData, long iToken, POSDATA *pposToken);
    HRESULT GetTokenText (long iTokenId, PCWSTR *ppszText, long *piLength);
    HRESULT IsInsideComment (CSourceData *pData, const POSDATA &pos, BOOL *pfInComment);
    HRESULT InternalIsInsideComment (CSourceData *pData, const POSDATA &pos, BOOL *pfInComment);
    HRESULT ParseTopLevel (CSourceData *pData, BASENODE **ppNode);
    HRESULT GetErrors (CSourceData *pData, ERRORCATEGORY iCategory, ICSErrorContainer **ppErrors);
    HRESULT GetInteriorParseTree (CSourceData *pData, BASENODE *pNode, ICSInteriorTree **ppTree);
    HRESULT LookupNode (CSourceData *pData, NAME *pKey, long iOrdinal, BASENODE **ppNode, long *piGlobalOrdinal);
    HRESULT GetNodeKeyOrdinal (CSourceData *pData, BASENODE *pNode, NAME **ppKey, long *piKeyOrdinal);
    HRESULT GetGlobalKeyArray (CSourceData *pData, KEYEDNODE *pKeyedNodes, long iSize, long *piCopied);
    HRESULT ParseForErrors (CSourceData *pData);
    HRESULT FindLeafNode (CSourceData *pData, const POSDATA pos, BASENODE **ppNode, ICSInteriorTree **ppTree);
    HRESULT GetExtent (BASENODE *pNode, POSDATA *pposStart, POSDATA *pposEnd, ExtentFlags flags);
    HRESULT GetTokenExtent (BASENODE *pNode, long *piFirst, long *piLast);
    long    GetFirstToken (BASENODE *pNode, BOOL *pfMissingName);
    long    GetLastToken (BASENODE *pNode, BOOL *pfMissingName);
    STATEMENTNODE   *GetLastStatement (STATEMENTNODE *pStmt);
    BOOL    IsDocComment(long iComment);
    HRESULT GetDocComment (BASENODE *pNode, long *piComment, long *piCount);
    HRESULT GetDocCommentText (BASENODE *pNode, LPWSTR *ppszText, long *piComment = NULL, long *piAdjust = NULL, LPCWSTR pszIndent = NULL);
    HRESULT FreeDocCommentText (LPWSTR *ppszText);
    HRESULT GetDocCommentPos (long iComment, POSDATA *loc, long iLineOffset = 0, long iCharOffset = 0);

    HRESULT IsUpToDate (BOOL *pfTokenized, BOOL *pfTopParsed);
    HRESULT ForceUpdate (CSourceData *pData);
    HRESULT GetInteriorNode (CSourceData *pData, BASENODE *pNode, CInteriorNode **ppInteriorNode);
    void    ResetHeapBusyFlag () { m_fInteriorHeapBusy = FALSE; }
    void    ResetTokenizedFlag () { m_fTokenized = FALSE; }
    void    ResetTokenizingFlag () { m_fTokenizing = FALSE; }
    BOOL    ReportError (int iErrorId, ERRORKIND errKind, int iLine, int iCol, int iExtent, PCWSTR pszText);
    BOOL    IsSymbolDefined(PNAME symbol) { return m_tableDefines.IsDefined(symbol); }
    void    MapLocation(POSDATA *pos, LPCWSTR *ppszFilename);
    void    MapLocation(long *line, NAME **ppszFilename);
    BOOL    hasMap() { return !m_LineMap.IsEmpty(); }

    // IUnknown
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    STDMETHOD(QueryInterface)(REFIID riid, void **ppObj);

    // ICSSourceModule
    STDMETHOD(GetSourceData)(BOOL fBlockForNewest, ICSSourceData **ppData);
    STDMETHOD(GetSourceText)(ICSSourceText **ppSrcText);
    STDMETHOD(Flush)();
    STDMETHOD(GetMemoryConsumption)(long *piTopTree, long *piInteriorTrees, long *piInteriorNodes);

};

#endif //__srcmod_h__

