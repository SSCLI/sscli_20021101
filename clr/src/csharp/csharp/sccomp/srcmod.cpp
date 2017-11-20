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
// File: srcmod.cpp
//
// ===========================================================================

#include "stdafx.h"
#include "srcmod.h"
#include "compiler.h"
#include "nodes.h"
#include "tokdata.h"
#include "srcdata.h"
#include "inttree.h"
#include "controller.h"
#include <float.h>
#include <limits.h>

#define DEFINE_TIMER_SWITCH "C# Compiler"
#define DBGOUT_MODULE DEFINE_TIMER_SWITCH
#include "timer.h"

#ifdef DEBUG
#include "dumpnode.h"
#define TOK(name, id, flags, type, stmtfunc, binop, unop, selfop, color) L#id,
const PCWSTR    rgszTokNames[] = {
#include "tokens.h"
NULL};
TOKENID Tok(int i) { return (TOKENID)i; }
PCWSTR TokenName(int i) { return rgszTokNames[i]; }
#endif

VSDEFINE_SWITCH(gfNoIncrementalTokenization, "C# CodeSense", "Disable incremental tokenization");
VSDEFINE_SWITCH(gfRetokDbgOut, "C# CodeSense", "Incremental tokenization debug output spew");
VSDEFINE_SWITCH(gfBeepOnTopLevelParse, "C# CodeSense", "Beep on incremental parse failure (top level reparse)");
VSDEFINE_SWITCH(gfParanoidICCheck, "C# CodeSense", "Enable paranoid incremental tokenizer validation (SLOW)");
VSDEFINE_SWITCH(gfUnhandledExceptions, DBGOUT_MODULE, "Disable exception handling in lexer/parser");
VSDEFINE_SWITCH(gfTrackDataRefs, "C# CodeSense", "Track ICSSourceData references");

#ifdef DEBUG
#define RETOKSPEW(x) if (VSFSWITCH(gfRetokDbgOut)) { x; }
#define TRACKREF(x) if (VSFSWITCH(gfTrackDataRefs)) { x; }
#else
#define RETOKSPEW(x)
#define TRACKREF(x) 
#endif


const int THREAD_BLOCK_SIZE = 8;

////////////////////////////////////////////////////////////////////////////////
// CSourceLexer::CSourceLexer

CSourceLexer::CSourceLexer (CSourceModule *pMod) :
    CLexer (pMod->GetNameMgr()),
    m_pModule(pMod)
{
    m_pOptions = pMod->GetOptions();
}

////////////////////////////////////////////////////////////////////////////////
// CSourceLexer::TrackLine

void CSourceLexer::TrackLine (PCWSTR pszLine)
{
    m_pModule->TrackLine (this, pszLine);
    CLexer::TrackLine(pszLine);
}

////////////////////////////////////////////////////////////////////////////////
// CSourceLexer::ErrorAtPosition

void __cdecl CSourceLexer::ErrorAtPosition (long iLine, long iCol, long iExtent, short iErrorId, ...)
{
    va_list args;

    va_start (args, iErrorId);
    m_pModule->ErrorAtPosition (args, iLine, iCol, iExtent, iErrorId);
}

////////////////////////////////////////////////////////////////////////////////
// CSourceLexer::ScanPreprocessorLine

BOOL CSourceLexer::ScanPreprocessorLine (PCWSTR &p)
{
    if (m_fPreproc)
    {
        m_pModule->ScanPreprocessorLine (this, p, m_fFirstOnLine);
        return TRUE;
    }

    return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceLexer::RecordCommentPosition

void CSourceLexer::RecordCommentPosition (const POSDATA &pos)
{
    m_pModule->RecordCommentPosition (pos);
}



////////////////////////////////////////////////////////////////////////////////
// CSourceParser::CSourceParser

CSourceParser::CSourceParser (CSourceModule *pMod) :
    CParser(pMod->m_pController),
    m_pModule(pMod)
{
}

////////////////////////////////////////////////////////////////////////////////
// CSourceParser::MemAlloc

void *CSourceParser::MemAlloc (long iSize)
{
    return m_pModule->m_pActiveHeap->Alloc (iSize);
}

////////////////////////////////////////////////////////////////////////////////
// CSourceParser::CreateNewError

void CSourceParser::CreateNewError (short iErrorId, va_list args, const POSDATA &posStart, const POSDATA &posEnd)
{
    m_pModule->CreateNewError (iErrorId, args, posStart, posEnd);
}

////////////////////////////////////////////////////////////////////////////////
// CSourceParser::AddToNodeTable

void CSourceParser::AddToNodeTable (BASENODE *pNode)
{
    m_pModule->AddToNodeTable (pNode);
}

////////////////////////////////////////////////////////////////////////////////
// CSourceParser::GetInteriorTree

void CSourceParser::GetInteriorTree (CSourceData *pData, BASENODE *pNode, ICSInteriorTree **ppTree)
{
    m_pModule->GetInteriorParseTree (pData, pNode, ppTree);
}




////////////////////////////////////////////////////////////////////////////////
// MakePath
//
// Joins a relative or absolute filename to the given path and stores the new
// filename in lpFileName

static void MakePath(/*[in]*/LPCWSTR lpPath, /*[in,out]*/LPWSTR lpFileName, DWORD nFileNameLength)
{
    // Yes, the unicode forms work, even on Win98
    if (PathIsURLW(lpFileName) || !PathIsRelativeW(lpFileName))
        return;

    LPWSTR lpNewFile = (LPWSTR)_alloca(sizeof(WCHAR) * (nFileNameLength + wcslen(lpPath)));

    if ((lpFileName[0] == L'\\' && lpFileName[1] == L'\\') ||
        lpFileName[1] == L':') {
        // This already a fully qualified name beginning with
        // a drive letter or UNC path, just return it
        return;
    } else if ((lpFileName[0] == L'\\' || lpFileName[0] == '/') &&
               lpPath[0] != L'\0') {
        // This a root-relative path name, just cat the drive
#if !PLATFORM_UNIX
        if (lpPath[0] == L'\\') { // a UNC path, use "\\server\share" as the drive
            wcscpy(lpNewFile, lpPath);
            LPWSTR lpSlash = lpNewFile;
            // Set the 4th slash to NULL to terminate the new file-name
            for (int x = 0; x < 3 && lpSlash; x++) {
                lpSlash = wcschr(lpSlash + 1, L'\\');
                ASSERT(lpSlash);
            }
            if (lpSlash) *lpSlash = L'\0';
        } else { // a drive letter
            wcsncpy(lpNewFile, lpPath, 2);
            lpNewFile[2] = L'\0';
        }
#endif  // !PLATFORM_UNIX
        wcscat(lpNewFile, lpFileName);
    } else {
        // This is a relative path name, just cat everything
        wcscpy(lpNewFile, lpPath);
        wcscat(lpNewFile, lpFileName);
    }

    // Now fix everything up to use canonical form
    W_GetFullPathName(lpNewFile, nFileNameLength, lpFileName, NULL);
}


////////////////////////////////////////////////////////////////////////////////
// CSourceModule inlined accessors

__forceinline NAMEMGR *CSourceModule::GetNameMgr () { return m_pController->GetNameMgr(); }
__forceinline BOOL CSourceModule::CheckFlags (DWORD dwFlags) { return m_pController->CheckFlags (dwFlags); }
__forceinline COptionData *CSourceModule::GetOptions () { return m_pController->GetOptions (); }


////////////////////////////////////////////////////////////////////////////////
// CSourceModule::CSourceModule

CSourceModule::CSourceModule (CController *pController) :
    m_iRef(0),
    m_iDataRef(0),
    m_iThreadCount(0),
    m_pController(pController),
    m_pSourceText(NULL),
    m_dwTextCookie(0),
    m_tableDefines(pController),
    m_pCCStack(NULL),
    m_dwVersion(0),
    m_iTokDelta(0),
    m_iTokDeltaFirst(0),
    m_iTokDeltaLast(0),
    m_pTreeTop(NULL),
    m_pNodeTable(NULL),
    m_pNodeArray(NULL),
    m_iErrors(0),
    m_heap(pController),
    m_pActiveHeap(NULL),
    m_pInterior(NULL),
    m_LineMap(pController->GetStandardHeap()),
    m_pszSrcDir(NULL),
    m_pDefaultName(NULL),
    m_pHiddenName(NULL),
    m_iCurTok(0),
    m_pTokenErrors(NULL),
    m_pParseErrors(NULL),
    m_pCurrentErrors(NULL),
    m_fTextModified(FALSE),
    m_fEventsArrived(FALSE),
    m_fEditsReceived(FALSE),
    m_fTextStateKnown(FALSE),
    m_fInteriorHeapBusy(FALSE),
    m_fTokenizing(FALSE),
    m_fTokenized(FALSE),
    m_fParsingTop(FALSE),
    m_fParsedTop(FALSE),
    m_fTokensChanged(FALSE),
    m_fForceTopParse(FALSE),
    m_fParsingInterior(FALSE),
    m_fTrackLines(FALSE),
    m_fLimitExceeded(FALSE),
    m_fErrorOnCurTok(FALSE),
    m_fCCSymbolChange(FALSE),
    m_fParsingForErrors(FALSE)
{
    m_pThreadDataRef = (THREADDATAREF *)VSAllocZero (THREAD_BLOCK_SIZE * sizeof (THREADDATAREF));
    m_pStdHeap = pController->GetStandardHeap ();

    m_pParser = new CSourceParser (this);

    ZeroMemory (&m_lex, sizeof (m_lex));

    // Must ref the controller!
    pController->AddRef();

    m_pDefaultName = GetNameMgr()->AddString(L"default");
    m_pHiddenName = GetNameMgr()->AddString(L"hidden");

    m_posEditOldEnd.iLine = 0;
    m_posEditOldEnd.iChar = 0;

    m_fAccumulateSize = FALSE;
    m_dwInteriorSize = 0;
    m_iInteriorTrees = 0;
    W_IsUnicodeSystem();
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::~CSourceModule

CSourceModule::~CSourceModule ()
{
    BOOL    fLocked = m_StateLock.LockedByMe ();

    InternalFlush ();

    // If we were connected to the text, it would have a ref on us...
    ASSERT (m_dwTextCookie == 0);

    if (m_pThreadDataRef)
        VSFree (m_pThreadDataRef);

    if (m_pSourceText != NULL)
        m_pSourceText->Release();

    if (m_lex.piTransitionLines != NULL)
        VSFree (m_lex.piTransitionLines);

    if (m_lex.pCCStates != NULL)
        VSFree (m_lex.pCCStates);

    m_tableDefines.ClearAll (TRUE);

    m_pController->Release();

    if (m_pTokenErrors != NULL)
        m_pTokenErrors->Release();

    if (m_pParseErrors != NULL)
        m_pParseErrors->Release();

    if (fLocked)
        m_StateLock.Release();

    if (m_pNodeTable)
        delete m_pNodeTable;

    if (m_pNodeArray)
        delete m_pNodeArray;

    if (m_lex.pIdentTable)
        delete m_lex.pIdentTable;

    if (m_pszSrcDir)
        m_pStdHeap->Free(m_pszSrcDir);

    delete m_pParser;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::CreateInstance        [static]
//
// A static construction function for a source module, whose ctor/dtor functions
// are private.

HRESULT CSourceModule::CreateInstance (CController *pController, ICSSourceText *pText, ICSSourceModule **ppModule)
{
    CSourceModule   *pObj = new CSourceModule (pController);
    HRESULT         hr;

    if (pObj == NULL)
        return E_OUTOFMEMORY;

    if (SUCCEEDED (hr = pObj->Initialize (pText)))
    {
        *ppModule = pObj;
        (*ppModule)->AddRef();
    }
    else
    {
        delete pObj;
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::Initialize

HRESULT CSourceModule::Initialize (ICSSourceText *pText)
{
    CTinyGate   gate (&m_StateLock);
    HRESULT     hr;

    m_pSourceText = pText;
    pText->AddRef();

    if (CheckFlags (CCF_KEEPNODETABLES))
    {
        m_pNodeTable = new CNodeTable (GetNameMgr());
        m_pNodeArray = new CStructArray<KEYEDNODE> ();
        if (m_pNodeTable == NULL || m_pNodeArray == NULL)
            return E_OUTOFMEMORY;
    }

    if (CheckFlags (CCF_KEEPIDENTTABLES))
    {
        m_lex.pIdentTable = new CIdentTable (GetNameMgr());
        if (m_lex.pIdentTable == NULL)
            return E_OUTOFMEMORY;
    }

    if (SUCCEEDED (hr = CErrorContainer::CreateInstance (EC_TOKENIZATION, 0, &m_pTokenErrors)) &&
        SUCCEEDED (hr = CErrorContainer::CreateInstance (EC_TOPLEVELPARSE, 0, &m_pParseErrors)))
    {
        hr = GetTextAndConnect ();
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::AcceptEdits

void CSourceModule::AcceptEdits ()
{
    ASSERT (m_StateLock.LockedByMe ());

    // This function permits the next request for tokenization/parsing results
    // to consider changed text.
    m_fTokenized = FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::ApplyDelta

inline void CSourceModule::ApplyDelta (POSDATA posX, POSDATA posCX, POSDATA posCY, POSDATA *pposY)
{
    pposY->iLine = posX.iLine + (posCY.iLine - posCX.iLine);
    if (pposY->iLine == posCY.iLine)
    {
        if (VSFSWITCH(gfParanoidICCheck))
        {
            ASSERT ((long)posX.iChar + (long)(posCY.iChar - posCX.iChar) < 1000);
            ASSERT ((long)posX.iChar + (long)(posCY.iChar - posCX.iChar) >= 0);
        }
        pposY->iChar = (long)posX.iChar + (long)(posCY.iChar - posCX.iChar);
    }
    else
        pposY->iChar = posX.iChar;

    if (VSFSWITCH (gfParanoidICCheck))
    {
        // Make sure we've arrived at a sane position
        ASSERT (pposY->iChar < 1000);
    }
}


////////////////////////////////////////////////////////////////////////////////
// CSourceModule::FindThreadIndex

long CSourceModule::FindThreadIndex (BOOL fMustExist)
{
    DWORD   dwTid = GetCurrentThreadId();

    // You MUST have already locked this object, since the return value is only
    // valid while the module is locked
    ASSERT (m_StateLock.LockedByMe());

    // Find this thread in the array -- add it if it isn't there
    long i;
    for (i=0; i<m_iThreadCount; i++)
        if (m_pThreadDataRef[i].dwTid == dwTid)
            return i;

    ASSERT (!fMustExist);

    // Not there.  Grow if necessary and add.
    if (m_iThreadCount % THREAD_BLOCK_SIZE == 0)
    {
        m_pThreadDataRef = (THREADDATAREF *)VSReallocZero (m_pThreadDataRef, (m_iThreadCount + THREAD_BLOCK_SIZE) * sizeof (THREADDATAREF), m_iThreadCount * sizeof(THREADDATAREF));
    }


    m_iThreadCount++;
    ASSERT (m_iThreadCount == i + 1);
    ASSERT (m_pThreadDataRef[i].iRef == 0);
    m_pThreadDataRef[i].dwTid = dwTid;
    return i;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::ReleaseThreadIndex

void CSourceModule::ReleaseThreadIndex (long iIndex)
{
    ASSERT (m_StateLock.LockedByMe());
    ASSERT (m_pThreadDataRef[iIndex].dwTid == GetCurrentThreadId());
    ASSERT (m_pThreadDataRef[iIndex].iRef == 0);

    // Move the last thread in the array to this slot (if this isn't the last one)
    if (iIndex < m_iThreadCount - 1)
    {
        m_pThreadDataRef[iIndex] = m_pThreadDataRef[m_iThreadCount - 1];
        m_pThreadDataRef[m_iThreadCount - 1].iRef = 0;
    }

    m_iThreadCount--;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::AddDataRef

void CSourceModule::AddDataRef ()
{
    CTinyGate gate (&m_StateLock);

    // Bump the global count...
    m_iDataRef++;

    // ...as well as the per-thread count
    long    iThreadIdx = FindThreadIndex (FALSE);
    m_pThreadDataRef[iThreadIdx].iRef++;
    TRACKREF(DBGOUT(("ADD DATA REF    :  T-%4X TID:%04X TIDX:%1d MOD:%08X TRef:%d MRef:%d", GetCurrentTime() & 0xffff, GetCurrentThreadId(), iThreadIdx, this, m_pThreadDataRef[iThreadIdx].iRef, m_iDataRef)));
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::ReleaseDataRef

void CSourceModule::ReleaseDataRef ()
{
    CTinyGate   gate (&m_StateLock);

    // Decrement the per-thread count...
    long    iThreadIdx = FindThreadIndex (TRUE);
    ASSERT (m_pThreadDataRef[iThreadIdx].iRef > 0);
    TRACKREF(DBGOUT(("RELEASE DATA REF:  T-%4X TID:%04X TIDX:%1d MOD:%08X TRef:%d MRef:%d", GetCurrentTime() & 0xffff, GetCurrentThreadId(), iThreadIdx, this, m_pThreadDataRef[iThreadIdx].iRef-1, m_iDataRef-1)));
    if (--(m_pThreadDataRef[iThreadIdx].iRef) == 0)
        ReleaseThreadIndex (iThreadIdx);

    // Decrement the global data ref count -- if zero, and we were since modified,
    // "accept" the edits (causing the next data access to consider the changed
    // text) and fire the "this module has changed" event, so listeners can know
    // that they can now ask the source module for a source data and get
    // (potentially) updated bits.
    ASSERT (m_iDataRef > 0);
    if (--m_iDataRef == 0)
    {
    }
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::GetTextAndConnect

HRESULT CSourceModule::GetTextAndConnect ()
{
    ASSERT (m_StateLock.LockedByMe());

    HRESULT     hr;


    POSDATA     posEnd;

    // Now, grab the text.  Note that we must unlock our state bits while we
    // do this, because change events from the text also lock the state bits and
    // we could otherwise deadlock.  As such, we need to check to see if any change
    // events came in between our unlock and lock, and if so, grab the text again.
    do
    {
        m_fEventsArrived = FALSE;
        m_StateLock.Release();
        hr = m_pSourceText->GetText (&m_lex.pszSource, &posEnd);
        m_StateLock.Acquire();

#ifdef DEBUG
        if (m_fEventsArrived)
            RETOKSPEW (DBGOUT (("*** TEXT CHANGED DURING ICSSourceText::GetText()!  Re-obtaining text pointer...")));
#endif
    } while (m_fEventsArrived);
    m_lex.iTextLen = (long)wcslen (m_lex.pszSource);

    if (!m_fEditsReceived)
    {
        // If we haven't received any edits, we need to consider the entire text
        // as changed.
        m_posEditNewEnd = posEnd;
		m_posEditOldEnd = posEnd;
		m_posEditStart.SetEmpty();
    }

    // It's no longer modified
    m_fTextModified = FALSE;
	//m_fEditsReceived = FALSE;
    RETOKSPEW (DBGOUT (("TEXT REVERTED TO UNMODIFIED STATE")));
    return hr;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::WaitForTokenization
//
// This function checks to see if another thread is tokenizing the text, and if
// so, waits for it to complete and returns TRUE.  If nobody is tokenizing, this
// returns FALSE.

BOOL CSourceModule::WaitForTokenization ()
{
    ASSERT (m_StateLock.LockedByMe ());     // Must have state locked to call this!

    if (m_fTokenizing)
    {
        ASSERT (m_dwTokenizerThread != GetCurrentThreadId());
        while (!m_fTokenized)
        {
            m_StateLock.Release();
            Snooze ();
            m_StateLock.Acquire();
        }

        return TRUE;
    }

    return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::FindNearestPosition
//
// Find the index of the postion matching the given position, or closest to it
// (always PRIOR to given position if not equal).  Returns -1 if no position
// values appear at or before the given pos.

long CSourceModule::FindNearestPosition (POSDATA *ppos, long iSize, const POSDATA &pos)
{
    long    iTop = 0, iBot = iSize - 1, iMid = iBot >> 1;

    // Zero in on a token near pos
    while (iBot - iTop >= 3)
    {
        if (ppos[iMid] == pos)
            return iMid;        // Wham!  exact match
        if (ppos[iMid] > pos)
            iBot = iMid - 1;
        else
            iTop = iMid + 1;

        iMid = iTop + ((iBot - iTop) >> 1);
    }

    // Last-ditch -- check from iTop to iBot for a match, or closest to.
    for (iMid = iTop; iMid <= iBot; iMid++)
    {
        if (ppos[iMid] == pos)
            return iMid;
        if (ppos[iMid] > pos)
            return iMid - 1;
    }

    // This is it!
    return iMid - 1;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::DetermineStartData
//
// For an incremental tokenization, this function determines the position and
// token index of where to start tokenizing, and the potential stopping token
// index and position (both relative to old and new text).

void CSourceModule::DetermineStartData (LEXDATABLOCK &old, MERGEDATA &md)
{
    // Get a pointer (in the new source buffer) to the first line involved in the
    // edit.  Because the beginning of the line can at most be the first character
    // involved in the edit, the OLD line offset table will work just fine.  Note
    // the the NEW TEXT MUST BE IN m_lex BY NOW!!!
    PCWSTR  pszEditLineNew = m_lex.pszSource + old.piLines[m_posEditStart.iLine];

    // Back up at most 5 characters from m_posEditStart, looking for a possible
    // \u sequence.  If found, consider the \ character as the first edited char.
    for (long iScan = -1; iScan >= -5 && (long)(m_posEditStart.iChar + iScan) >= 0; iScan--)
    {
        if (pszEditLineNew[m_posEditStart.iChar + iScan] == '\\' && pszEditLineNew[m_posEditStart.iChar + iScan + 1] == 'u')
        {
            RETOKSPEW (DBGOUT (("Backed up %d chars for possible unicode escape consideration", -iScan)));
            m_posEditStart.iChar += iScan;
            break;
        }
    }

    // Assume we will NOT start at 0, 0
    md.fStartAtZero = FALSE;

    // Find the first token to scan -- which is the last token w/ start position < the edit.
    // FindNearestToken may return a token index whose position == that given.  We can't
    // start there -- must start one before it.
    md.iTokStart = FindNearestPosition (old.pposTokens, old.iTokens, m_posEditStart);
    if (md.iTokStart >= 0 && old.pposTokens[md.iTokStart] == m_posEditStart)
        md.iTokStart--;

    // Check for special case -- TID_NUMBER followed by TID_DOT must be considered as a whole,
    // since it may now absorb into a single number.
    if (md.iTokStart > 0 && old.pTokens[md.iTokStart] == TID_DOT && old.pTokens[md.iTokStart - 1] == TID_NUMBER)
        md.iTokStart--;

    if (md.iTokStart == -1)
    {
        RETOKSPEW (DBGOUT (("Change occurred prior to first token; must rescan from beginning of file")));
        md.iTokStart = 0;
        md.fStartAtZero = TRUE;
        md.posStart.iLine = md.posStart.iChar = 0;
    }
    else
    {
        RETOKSPEW (DBGOUT (("Retokenization starts at token %d (%S), position (%d, %d)", md.iTokStart, rgszTokNames[old.pTokens[md.iTokStart]], old.pposTokens[md.iTokStart].u.iLine, old.pposTokens[md.iTokStart].u.iChar)));

        // Start position is the position of the start token.
        md.posStart = old.pposTokens[md.iTokStart];

        // It is NEVER the case that the edit and the retokenization start at the same place, unless
        // the edit occured at the beginning of the file.
        ASSERT (md.posStart < m_posEditStart);
    }

    md.tokFirst = (TOKENID)old.pTokens[md.iTokStart];

    // Next find the (potentially) last token to scan -- which is the first token w/ start
    // position > the edit.
    md.iTokStop = FindNearestPosition (old.pposTokens, old.iTokens, m_posEditOldEnd) + 1;
    if (md.iTokStop == old.iTokens)
    {
        RETOKSPEW (DBGOUT (("Change affects last token; must scan to end of file")));
        ASSERT (old.pTokens[old.iTokens - 1] == TID_ENDFILE);
        md.posStopOld = old.pposTokens[old.iTokens - 1];
    }
    else
    {
        ASSERT (md.iTokStop < old.iTokens);
        ASSERT (old.pposTokens[md.iTokStop] > m_posEditOldEnd);
        RETOKSPEW (DBGOUT (("Retokenization ends at token %d (%S), OLD position (%d, %d)", md.iTokStop, rgszTokNames[old.pTokens[md.iTokStop]], old.pposTokens[md.iTokStop].u.iLine, old.pposTokens[md.iTokStop].u.iChar)));

        // pposStopOld is the stop position relative to the OLD buffer.  The
        // stop position is the position of the stop token (which, in a successful
        // incremental tokenization, will be the same token at the same location
        // relative to the NEW buffer).
        md.posStopOld = old.pposTokens[md.iTokStop];
    }

    // Now that we have the stop position relative to the OLD buffer, we can use ApplyDelta
    // to calculate the position relative to the NEW buffer.
    ApplyDelta (md.posStopOld, m_posEditOldEnd, m_posEditNewEnd, &md.posStop);
}



////////////////////////////////////////////////////////////////////////////////
// CSourceModule::ReconstructCCState

void CSourceModule::ReconstructCCState (LEXDATABLOCK &old, MERGEDATA &md, CCREC **ppCopy, NAMETAB *pOldTable)
{
    CCREC   *pCopyStack = NULL;

    ASSERT (m_pCCStack == NULL);

    // Starting with an empty stack, push/pop records based on the contents of the
    // OLD CC change array, up to md.posStart.u.iLine.  Note that this also copies
    // the CC change array to the NEW buffer.
    //
    // Also, while we create the starting stack, we also create a COPY of the
    // stack, which we "follow through" to md.posStop for comparison/validation later.
    ASSERT (m_lex.iCCStates == old.iCCStates);
    if (old.iCCStates > 0)
    {
        m_lex.pCCStates = (CCSTATE *)VSAlloc ((m_lex.iCCStates + 8) * sizeof (CCSTATE *));
        for (m_lex.iCCStates=0; m_lex.iCCStates < old.iCCStates && md.posStart.iLine > old.pCCStates[m_lex.iCCStates].iLine; m_lex.iCCStates++)
        {
            m_lex.pCCStates[m_lex.iCCStates] = old.pCCStates[m_lex.iCCStates];
            if (old.pCCStates[m_lex.iCCStates].dwFlags & CCF_ENTER)
            {
                // Push a record onto both the "real" stack and our copy
                PushCCRecord (m_pCCStack, old.pCCStates[m_lex.iCCStates].dwFlags & ~CCF_ENTER);
                PushCCRecord (pCopyStack, old.pCCStates[m_lex.iCCStates].dwFlags & ~CCF_ENTER);
            }
            else
            {
                ASSERT (m_pCCStack != NULL);
                ASSERT (pCopyStack != NULL);

                // Pop a record from both stacks
                PopCCRecord (m_pCCStack);
                PopCCRecord (pCopyStack);
            }
        }

        md.iFirstAffectedCCState = m_lex.iCCStates;

        // Continue forming our copy of the CC stack
        for (long i=m_lex.iCCStates; i < old.iCCStates && md.posStopOld.iLine > old.pCCStates[i].iLine; i++)
        {
            if (old.pCCStates[i].dwFlags & CCF_ENTER)
                PushCCRecord (pCopyStack, old.pCCStates[i].dwFlags & ~CCF_ENTER);
            else
            {
                ASSERT (pCopyStack != NULL);
                PopCCRecord (pCopyStack);
            }
        }
    }
    else
    {
        // Don't hold on to the old copy so we don't delete it later...
        old.pCCStates = NULL;
        md.iFirstAffectedCCState = 0;
    }

    *ppCopy = pCopyStack;

    // Also, we must start with the correct CC symbol table.  If we are starting at a token > 0,
    // then we just use the old symbol table -- no further changes to it are possible, since a token
    // precedes the edit.  Otherwise, we must create a copy of the old table for comparison after
    // tokenization, and start with either the original (cmd-line provided) symbols if starting < 0,
    // or the old table (if starting at token 0).  If changes are made to the table during retok,
    // we must scan the remainder of the file.
    m_fCCSymbolChange = FALSE;
    if (md.iTokStart == 0)
    {
        // Keep a copy of the old table
        for (long iSym = 0; iSym < m_tableDefines.GetCount(); iSym++)
            pOldTable->Define (m_tableDefines.GetAt (iSym));

        if (md.fStartAtZero)
        {
            // Reset to "factory defaults"
            m_tableDefines.ClearAll();

            CComBSTR    sbstr(GetOptions()->m_sbstrCCSYMBOLS);

            if (sbstr)
            {
                PWSTR   pszTok = wcstok (sbstr, L" ,;");

                while (pszTok != NULL)
                {
                    m_tableDefines.Define (GetNameMgr()->AddString (pszTok));
                    pszTok = wcstok (NULL, L" ,;");
                }
            }

            // Assume that a change has happened -- we need to check the tables for equality after
            // tokenization
            m_fCCSymbolChange = TRUE;
        }
    }
}


////////////////////////////////////////////////////////////////////////////////
// CSourceModule::MergeTokenStream

void CSourceModule::MergeTokenStream (LEXDATABLOCK &old, MERGEDATA &md)
{
    long    iTokDelta = md.fIncrementalStop ? (m_lex.iTokens - (md.iTokStop - md.iTokStart)) : (m_lex.iTokens - (old.iTokens - md.iTokStart));

    // First, update the tokens (and positions).
    if (iTokDelta == 0)
    {
        // Check for token equality -- it is very possible that no token changes were made.
        // Note that some token values may stay the same but still require parsing work
        // (such as identifiers and literals), so watch for them.  We only do this if the
        // number of tokens we parsed was small (3 or less)
        if (md.fIncrementalStop && m_lex.iTokens <= 3)
        {
            BOOL    fRude = FALSE;

            // Copy the new tokens over, checking for "dynamic" tokens (which have length == 0).
            for (long i=0; i<m_lex.iTokens; i++)
            {
                if (old.pTokens[md.iTokStart + i] != m_lex.pTokens[i])
                {
                    old.pTokens[md.iTokStart + i] = m_lex.pTokens[i];
                    fRude = TRUE;
                }
                else if (CParser::GetTokenInfo ((TOKENID)m_lex.pTokens[i])->iLen == 0)
                {
                    fRude = TRUE;
                }
            }

            if (!fRude)
            {
                // Nice!  When we're done with this, we don't have to reparse ANYTHING!  (Happens
                // a lot when typing comments or whitespace)
                md.fStreamChanged = FALSE;
                RETOKSPEW (DBGOUT (("INCREMENTAL TOKENIZATION REALLY ROCKS:  No tokens actually changed!!!")));
            }
        }
        else
        {
            // Too many to check, just copy them over and assume they're different.
            memcpy (old.pTokens + md.iTokStart, m_lex.pTokens, m_lex.iTokens * sizeof (BYTE));
        }
    }
    else
    {
        if (old.iAllocTokens < (old.iTokens + iTokDelta))
        {
            old.iAllocTokens = old.iTokens + iTokDelta;
            old.pTokens = (BYTE *)m_pStdHeap->Realloc (old.pTokens, old.iAllocTokens * sizeof (BYTE));
            old.pposTokens = (POSDATA *)m_pStdHeap->Realloc (old.pposTokens, old.iAllocTokens * sizeof (POSDATA));
        }

        // Move the old unaffected tokens, if any (and if necessary)
        if (md.fIncrementalStop && (iTokDelta != 0))
            memmove (old.pTokens + md.iTokStart + m_lex.iTokens, old.pTokens + md.iTokStop, (old.iTokens - md.iTokStop) * sizeof (BYTE));

        // Copy the new tokens over
        memcpy (old.pTokens + md.iTokStart, m_lex.pTokens, m_lex.iTokens * sizeof (BYTE));
    }

    // Since we know the token delta, calculate the last-changed token index here.  Note that
    // m_iTokDelta is an accumulation -- multiple retokenizations can occur between parses.
    m_iTokDelta += iTokDelta;

    long        iLastChanged = md.iTokStop + iTokDelta - 1;     // -1 because md.iTokStop token doesn't change if md.fIncrementalStop

    if (iLastChanged < md.iTokStart)
        iLastChanged = md.iTokStart;

    m_iTokDeltaLast = max (iLastChanged, m_iTokDeltaLast + iTokDelta);

    // Move the old unaffected token positions, if any (and if necessary)
    if (md.fIncrementalStop && (iTokDelta != 0))
        memmove (old.pposTokens + md.iTokStart + m_lex.iTokens, old.pposTokens + md.iTokStop, (old.iTokens - md.iTokStop) * sizeof (POSDATA));

    // Copy the new positions over
    memcpy (old.pposTokens + md.iTokStart, m_lex.pposTokens, m_lex.iTokens * sizeof (POSDATA));

    // Adjust positions of tokens following (if incremental stopped)
    if (md.fIncrementalStop)
    {
        // First, update all line AND column values of tokens on the same line as the end of the edit
        // (which is gauranteed to be <= md.posStop.u.iLine).
        long i;
        for (i=md.iTokStart + m_lex.iTokens; i < old.iTokens + iTokDelta && old.pposTokens[i].iLine == md.posEditOldEnd.iLine; i++)
        {
            old.pposTokens[i].iLine += md.iLineDelta;
            old.pposTokens[i].iChar += md.iCharDelta;
        }

        // Next, if there was a line delta, add it to all token position lines from here on out.
        if (md.iLineDelta != 0)
        {
            for (; i < old.iTokens + iTokDelta; i++)
                old.pposTokens[i].iLine += md.iLineDelta;
        }
    }

    old.iTokens += iTokDelta;

    // There!  Now put the old token stream back and whack the new one.
    m_pStdHeap->Free (m_lex.pTokens);
    m_pStdHeap->Free (m_lex.pposTokens);
    m_lex.pTokens = old.pTokens;
    m_lex.pposTokens = old.pposTokens;
    m_lex.iTokens = old.iTokens;
    m_lex.iAllocTokens = old.iAllocTokens;
}


////////////////////////////////////////////////////////////////////////////////
// CSourceModule::MergeCommentTable

void CSourceModule::MergeCommentTable (LEXDATABLOCK &old, MERGEDATA &md)
{
    long    iFirstAffected, iFirstUnaffected, iDelta;

    // Find the last unaffected comment in the table (and the first unaffected
    // comment past the edit if md.fIncrementalStop), and replace that span with the
    // new stuff (and update the remaining ones if there are any).
    iFirstAffected = FindNearestPosition (old.pposComments, old.iComments, md.posStart);
    if (iFirstAffected < 0 || old.pposComments[iFirstAffected] != md.posStart)
        iFirstAffected++;   // The first affected one must be >= the edit; FindNearestPosition will find the last one <= the edit.

    iFirstUnaffected = md.fIncrementalStop ? FindNearestPosition (old.pposComments, old.iComments, md.posStopOld) + 1 : old.iComments;
    iDelta = m_lex.iComments - (iFirstUnaffected - iFirstAffected);

    if (old.iAllocComments < (old.iComments + iDelta))
    {
        // Resize...
        old.iAllocComments = (old.iComments + iDelta);
        old.pposComments = (POSDATA *)m_pStdHeap->Realloc (old.pposComments, old.iAllocComments * sizeof (POSDATA));
    }

    // Move any unaffected comments to make room for new ones (or shrink for deleted ones),
    // and update their positions
    old.iComments += iDelta;
    if (md.fIncrementalStop)
    {
        if (iDelta != 0)
            memmove (old.pposComments + iFirstAffected + m_lex.iComments, old.pposComments + iFirstUnaffected, (old.iComments - iDelta - iFirstUnaffected) * sizeof (POSDATA));

	long i;
        for (i=iFirstAffected + m_lex.iComments; i<old.iComments && old.pposComments[i].iLine == md.posEditOldEnd.iLine; i++)
        {
            old.pposComments[i].iLine += md.iLineDelta;
            old.pposComments[i].iChar += md.iCharDelta;
        }

        if (md.iLineDelta != 0)
        {
            for (; i < old.iComments; i++)
                old.pposComments[i].iLine += md.iLineDelta;
        }
    }

    // Copy over new ones (if there were any)...
    if (m_lex.iComments != 0)
        memcpy (old.pposComments + iFirstAffected, m_lex.pposComments, m_lex.iComments * sizeof (POSDATA));

    // Keep track of the new vs. old if there was a change, so we can fire the right event.
    if (iFirstAffected != iFirstUnaffected || iDelta != 0 || m_lex.iComments != 0)
    {
        md.iStartComment = iFirstAffected;
        md.iNewEndComment = iFirstUnaffected + iDelta;
        md.iOldEndComment = iFirstUnaffected;
    }

    // Whack the new, copy the old (updated) back
    m_pStdHeap->Free (m_lex.pposComments);
    m_lex.pposComments = old.pposComments;
    m_lex.iComments = old.iComments;
    m_lex.iAllocComments = old.iAllocComments;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::MergeCCState

void CSourceModule::MergeCCState (LEXDATABLOCK &old, MERGEDATA &md)
{
    long    iFirstAffected, iFirstUnaffected, iDelta;

    // Transitions.  These are small in number; binary searches not necessary.
    for (iFirstAffected = 0; iFirstAffected < old.iTransitionLines && old.piTransitionLines[iFirstAffected] < (long)md.posStart.iLine; iFirstAffected++)
        ;

    if (md.fIncrementalStop)
    {
        for (iFirstUnaffected = iFirstAffected; iFirstUnaffected < old.iTransitionLines && old.piTransitionLines[iFirstUnaffected] < (long)md.posStopOld.iLine; iFirstUnaffected++)
            ;
    }
    else
    {
        iFirstUnaffected = old.iTransitionLines;
    }

    iDelta = m_lex.iTransitionLines - (iFirstUnaffected - iFirstAffected);

    // Any transitions in the retokenized text means the transitions changed.
    if (iFirstUnaffected != iFirstAffected)
        md.fTransitionsChanged = TRUE;

    // Transition array grows in blocks of 8...
    if (iDelta > 0 && ((old.iTransitionLines + iDelta + 7) & ~7) > ((old.iTransitionLines + 7) & ~7))
    {
        long    iSize = ((old.iTransitionLines + iDelta + 7) & ~7) * sizeof (long);
        old.piTransitionLines = (long *)((old.piTransitionLines == NULL) ? VSAlloc (iSize) : VSRealloc (old.piTransitionLines, iSize));
    }

    // We'll assume there's a line delta.  If the array is growing, move/update starting from the end.  
    // Otherwise move/update starting at the beginning.  Note that this refers to the unaffected 'third'.
    if (iDelta > 0)
    {
        for (long i=old.iTransitionLines - 1; i >= iFirstUnaffected; i--)
            old.piTransitionLines[i + iDelta] = old.piTransitionLines[i] + md.iLineDelta;
    }
    else if (iDelta < 0)
    {
        for (long i=iFirstUnaffected; i < old.iTransitionLines; i++)
            old.piTransitionLines[i + iDelta] = old.piTransitionLines[i] + md.iLineDelta;
    }
    else if (md.iLineDelta != 0)
    {
        // In this case the values aren't moving -- just update them
        for (long i=iFirstUnaffected; i < old.iTransitionLines; i++)  
            old.piTransitionLines[i] += md.iLineDelta;
    }

    // Copy the new ones into the array -- the space is there for them.
    memcpy (old.piTransitionLines + iFirstAffected, m_lex.piTransitionLines, m_lex.iTransitionLines * sizeof (long));
    old.iTransitionLines += iDelta;

    // Whack the new, replace with the updated old
    if (m_lex.piTransitionLines != NULL)
        VSFree (m_lex.piTransitionLines);
    m_lex.piTransitionLines = old.piTransitionLines;
    m_lex.iTransitionLines = old.iTransitionLines;

    // Update the conditional compilation state change array.  We will only have work to do
    // here if an incremental tokenization stopped short of the end of the file.
    if (md.fIncrementalStop)
    {
        // Find the first transition past the last scanned token
        while (md.iFirstAffectedCCState < old.iCCStates && old.pCCStates[md.iFirstAffectedCCState].iLine <= md.posStopOld.iLine)
            md.iFirstAffectedCCState++;

        if (md.iFirstAffectedCCState < old.iCCStates)
        {
            // Copy that one and all following to the new one (making sure it's big enough first), updating
            // the line using the line delta as we go.
            ASSERT (m_lex.pCCStates != NULL);
            m_lex.pCCStates = (CCSTATE *)VSRealloc (m_lex.pCCStates, (m_lex.iCCStates + (old.iCCStates - md.iFirstAffectedCCState)) * sizeof (CCSTATE));
            for (; md.iFirstAffectedCCState < old.iCCStates; md.iFirstAffectedCCState++)
            {
                m_lex.pCCStates[m_lex.iCCStates] = old.pCCStates[md.iFirstAffectedCCState];
                m_lex.pCCStates[m_lex.iCCStates++].iLine += md.iLineDelta;
            }
        }
    }

    if (old.pCCStates != NULL)
        VSFree (old.pCCStates);
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::MergeLineTable

void CSourceModule::MergeLineTable (LEXDATABLOCK &old, MERGEDATA &md)
{
    long    iFirstAffected, iFirstUnaffected;

    // The first affected line is the start line PLUS one.  The last one is the
    // stop line PLUS one -- and isn't really "unaffected", since we must account
    // for STREAM changes in the text.
    iFirstAffected = md.posStart.iLine + 1;
    iFirstUnaffected = md.fIncrementalStop ? md.posStopOld.iLine + 1 : old.iLines;

    // All lines up to the first affected one will be the same.  Note that we are
    // copying directly into the NEW table -- the span of lines that was just
    // tokenized will have already been updated.
    memcpy (m_lex.piLines, old.piLines, iFirstAffected * sizeof (long));

    // Now, update all lines from iFirstUnaffected to the end to account for
    // the stream delta.
    if (md.fIncrementalStop)
    {
        long    iStreamNewEnd = m_lex.piLines[md.posEditNewEnd.iLine] + md.posEditNewEnd.iChar;
        long    iStreamDelta = iStreamNewEnd - md.iStreamOldEnd;

        for (long i = iFirstUnaffected; i < old.iLines; i++)
            m_lex.piLines[i + md.iLineDelta] = old.piLines[i] + iStreamDelta;
    }

    // Whack the old copy
    m_pStdHeap->Free (old.piLines);
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::UpdateRegionTable

void CSourceModule::UpdateRegionTable ()
{
    // We can have at most m_lex.iCCStates regions (and that's only if they are
    // all nested and unterminated).  Allocate twice as much as necessary, and
    // point the end array at the midpoint (these never grow -- one less allocation).
    long    iSize = 2 * m_lex.iCCStates * sizeof (LONG);
    m_lex.piRegionStart = (long *)(m_lex.piRegionStart == NULL ? m_pStdHeap->Alloc (iSize) : m_pStdHeap->Realloc (m_lex.piRegionStart, iSize));
    m_lex.piRegionEnd = m_lex.piRegionStart + m_lex.iCCStates;

    // We also need a little stack space
    long    *piStack = STACK_ALLOC (long, m_lex.iCCStates);
    long    iStack = 0;

    // Rip through the CC states and look for CCF_REGION enters and corresponding exits
    m_lex.iRegions = 0;
    for (long i=0; i<m_lex.iCCStates; i++)
    {
        DWORD   dwFlags = m_lex.pCCStates[i].dwFlags;

        if (dwFlags & CCF_REGION)
        {
            // Here's one we care about.
            if (dwFlags & CCF_ENTER)
            {
                // This is an enter.  Store the line number in the start array
                // at the current region slot, and push that region slot on the
                // stack for the next exit.
                m_lex.piRegionStart[m_lex.iRegions] = m_lex.pCCStates[i].iLine;
                piStack[iStack++] = m_lex.iRegions++;
            }
            else
            {
                // This is an exit, so pop a slot off the stack and store the
                // line in the end array at that slot.
                ASSERT (iStack > 0);
                m_lex.piRegionEnd[piStack[--iStack]] = m_lex.pCCStates[i].iLine;
            }
        }
    }

    // For anything left on the stack, use the line count as the 'end'
    while (iStack > 0)
        m_lex.piRegionEnd[piStack[--iStack]] = m_lex.pposTokens[m_lex.iTokens - 1].iLine;
}






////////////////////////////////////////////////////////////////////////////////
// CSourceModule::HandleExceptionFromTokenization

LONG CSourceModule::HandleExceptionFromTokenization (EXCEPTION_POINTERS * exceptionInfo, PVOID pv)
{
    //  1) The original call that caused tokenization to occur returns failure, and
    //  2) The state of the source module is such that another attempt to tokenize
    //     won't just crash again (which may not be possible)
    _ASSERTE(pv);
    if (pv)
    {
        ((CSourceModule *)pv)->ClearTokenizerThread();
        ((CSourceModule *)pv)->ResetTokenizedFlag();
        ((CSourceModule *)pv)->ResetTokenizingFlag();
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::CreateTokenStream

void CSourceModule::CreateTokenStream ()
{
    if (VSFSWITCH (gfUnhandledExceptions))
    {
        InternalCreateTokenStream ();
    }
    else
    {
        PAL_TRY
        {
            InternalCreateTokenStream ();
        }
        PAL_EXCEPT_FILTER (HandleExceptionFromTokenization, this)
        {
            // NOTE:  We'll never get here...
        }
        PAL_ENDTRY
    }
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::InternalCreateTokenStream
//
// This is the main convert-text-to-token-stream function.  Called before any
// operation that depends on having a tokenized version of the source ready to
// use.
//
// This function is a monster.  All of the complexities involved in incrementally
// updating the token stream (and all other data generated by lexing) is in here.
// Needless to say this is a delicate function -- any changes made to it should
// be thoroughly reviewed.

void CSourceModule::InternalCreateTokenStream ()
{
    // Lock the state bits.  Once we determine that we actually need to do work,
    // we'll set appropriate state flags an unlock the state bits so other threads
    // can make the same determination.
    CTinyGate   gate (&m_StateLock);

    // If we're currently tokenized, we stop here.
    if (m_fTokenized)
        return;

    // We're not tokenized, but it's possible another thread is doing it already.
    // If this function returns TRUE, that is (was) the case -- the token stream
    // will have been updated by the other thread.
    if (WaitForTokenization ())
        return;

    // Okay, at this point we commit to tokenizing the current text.  Reflect that
    // in the state bits, and record our thread ID as the "tokenizer thread".  Also
    // start the timer for perf analysis.
    TimerStart (TIME_CREATETOKENSTREAM);
    SetTokenizerThread ();
    m_fTokenizing = TRUE;

    // Validate some state flags -- we can't need a tokenization while someone
    // is currently doing any parsing work...
    ASSERT (!m_fParsingTop && !m_fParsingInterior);

    // Even in the event of a non-incremental tokenization, we need to make a copy
    // of the current lexer state.
    LEXDATABLOCK    old (m_lex);

    // If the text is modified, go grab the new version.  This also makes sure we
    // are connected to the events fired by the source text (if any).
    if (m_fTextModified)
        GetTextAndConnect ();

    BOOL    fIncremental;

    // There are many factors that contribute to whether or not we can be incremental.
    // The first one is whether the text state is known before we start.  The term
    // "text state" basically means whether or not we know precisely what has happened
    // to the text between now and the last time we tokenized (via m_posEdit* vars).
    // This implies that we actually have tokenization data from last time...
    fIncremental = m_fTextStateKnown;

    // After checking the "text state known" flag, we set it to TRUE here, instead of
    // when we are finished.  This prevents "missing" changes to the text that occur
    // (from another thread) while we're tokenizing
    m_fTextStateKnown = TRUE;

    // Back-door hack for debug builds only -- force full tokenization
#ifdef DEBUG
    if (VSFSWITCH (gfNoIncrementalTokenization))
        fIncremental = FALSE;
#endif

    // Exceeding the line count or line length limits prevents incremental-ness
    if (m_fLimitExceeded)
    {
        fIncremental = FALSE;
        m_fLimitExceeded = FALSE;
    }

    // #line directives also prohibit incremental-ness.
    if (fIncremental && !m_LineMap.IsEmpty())
        fIncremental = FALSE;

    // Okay, that's it for the absolutely-no-way-we-can-be-incremental checks.  Even
    // though we may end up lexing the entire source, if fIncremental is TRUE here,
    // the tokenization is considered incremental.  If we're NOT incremental, we
    // must flush any data from the last tokenization.
    if (!fIncremental)
    {
        RETOKSPEW (DBGOUT (("NON-INCREMENTAL TOKENIZATION!")));
        InternalFlush ();

        // Copy the flushed m_lex over to old to keep things sane...
        old = m_lex;

        // NOTE:  The identifier table isn't entirely accurate -- it may contain
        // identifiers that are no longer used.  We use this opportunity to empty
        // the identifier table.
        if (m_lex.pIdentTable != NULL)
            m_lex.pIdentTable->RemoveAll ();
    }
    else
    {
        // We must have data if this is incremental!
        ASSERT (m_lex.pTokens != NULL);
    }

    MERGEDATA       md;

    if (fIncremental)
    {
        // Figure out where we can start and (try to) stop tokenizing.
        DetermineStartData (old, md);

        // Scan between iTokStop+1 and iTokStop-1 to see if any TID_OPEN/CLOSECURLY tokens exist.
        // If so, this retok will trigger a top-level reparse.  Note that we don't care about the
        // first changed token; it is compared later, and we don't care about the last token; it
        // must be the same or retokenization won't "stop incrementally".
        m_fForceTopParse = FALSE;
        for (long i=md.iTokStart + 1; i<md.iTokStop; i++)
        {
            if (old.pTokens[i] == TID_OPENCURLY || old.pTokens[i] == TID_CLOSECURLY)
            {
                m_fForceTopParse = TRUE;
                break;
            }
        }
    }
    else
    {
        // A non-incremental tokenization will always start at ground zero...
        md.iTokStart = 0;
        md.fStartAtZero = TRUE;
        md.iTokStop = old.iTokens;
        md.posStart.iLine = md.posStart.iChar = 0;

        // A non-incremental tokenization will also always for a top-level reparse.
        m_fForceTopParse = TRUE;
    }

    // This flag gets set to FALSE if an incremental tokenization doesn't stop at md.posStop correctly.
    md.fIncrementalStop = fIncremental && (md.iTokStop < old.iTokens);

    // This is the line delta -- number of lines added/removed via edit
    md.iLineDelta = m_posEditNewEnd.iLine - m_posEditOldEnd.iLine;

    // This is the char delta -- which is NOT the number of characters added/removed,
    // but rather the change in char offset of the first unmodified character (which
    // may be on a different line now -- an insertion could render md.iCharDelta < 0).
    md.iCharDelta = m_posEditNewEnd.iChar - m_posEditOldEnd.iChar;

    // This is the stream-based character index of the first unmodified character,
    // relative to the OLD buffer.  We calculate the value relative to the NEW buffer
    // later (when we have a corresponding line table).
    md.iStreamOldEnd = fIncremental ? (old.piLines[m_posEditOldEnd.iLine] + m_posEditOldEnd.iChar) : 0;

    // For error replacement, we need to position of the end-of-file.
    md.posOldEOF = (old.pposTokens == NULL) ? POSDATA(0,0) : old.pposTokens[old.iTokens - 1];

    md.posEditOldEnd = m_posEditOldEnd;
    md.posEditNewEnd = m_posEditNewEnd;

	m_posEditOldEnd = m_posEditNewEnd;

    CErrorContainer *pErrorTemp;

    if (FAILED (m_pTokenErrors->Clone (&pErrorTemp)))
    {
        VSFAIL("Out of memory");
    }

    // Release the old container and start using the new (clone) one.
    m_pTokenErrors->Release();
    m_pTokenErrors = pErrorTemp;

    // The "current error" container must be NULL at this point -- otherwise we're stepping on someone...
    ASSERT (m_pCurrentErrors == NULL);
    m_pCurrentErrors = m_pTokenErrors;

    // This tell the token error container to enter "replacement mode".
    m_pTokenErrors->BeginReplacement();

    ////////////////////////////////////////////////////////////////////////////
    //
    // At this point, we are done looking at m_posEdit* variables, have set
    // the "tokenizing" bit, and are otherwise done mucking around with state
    // that is protected by the state lock, so it is safe to release it.
    //
    ////////////////////////////////////////////////////////////////////////////
    gate.Release();



    CCREC       *pCopyStack = NULL;
    NAMETAB     OldTable (m_pController);

    // Okay, time to reconstruct the CC stack and symbol table.
    ReconstructCCState (old, md, &pCopyStack, &OldTable);

    // Allocation time.  Start with token/position/comment/line buffers that are suitably sized
    // (using hypothesized statistics) based on the amount of text we're going to scan.  Note that
    // for retokenization, we start at index 0 in these new buffers even if we're scanning token > 0.
    // This is accounted for in TrackLine (for the line table) by using the line tracking offset,
    // and is otherwise accounted for by copying the new data to the old table at the appropriate
    // index.  Note that if incremental, we know EXACTLY how many lines we'll need.
    m_lex.iAllocLines = fIncremental ? old.iLines + md.iLineDelta : ((m_lex.iTextLen >> 5) + 32) & ~31;
    m_lex.piLines = (long *)m_pStdHeap->Alloc (m_lex.iAllocLines * sizeof (long));
    m_lex.iLines = (md.fStartAtZero) ? 0 : md.posStart.iLine;
    ASSERT (m_lex.iLines <= m_lex.iAllocLines);

    // REVIEW:  This guess for tokens is totally bogus...
    m_lex.iAllocTokens = m_lex.iAllocLines * 4;
    m_lex.pTokens = (BYTE *)m_pStdHeap->Alloc (m_lex.iAllocTokens * sizeof (BYTE));
    m_lex.pposTokens = (POSDATA *)m_pStdHeap->Alloc (m_lex.iAllocTokens * sizeof (POSDATA));
    m_lex.iTokens = 0;

    // A different guess for comments.  If incremental, we (grossly) assume that every line in the
    // old file was a comment.  Non-incremental is a total guess, too, based on text size...
    m_lex.iAllocComments = fIncremental ? m_lex.iAllocLines : ((m_lex.iTextLen >> 7) + 32) & ~31;
    m_lex.pposComments = (POSDATA *)m_pStdHeap->Alloc (m_lex.iAllocComments * sizeof (POSDATA));
    m_lex.iComments = 0;

    // Start with 0 transitions.  Also, make ANOTHER copy of these for comparison afterward.
    long    iTransitionCopy = old.iTransitionLines;
    long    *piTransitionCopy = (iTransitionCopy > 0) ? STACK_ALLOC (long, iTransitionCopy) : NULL;
    if (piTransitionCopy != NULL)
        memcpy (piTransitionCopy, old.piTransitionLines, iTransitionCopy * sizeof (long));

    m_lex.iTransitionLines = 0;
	m_lex.piTransitionLines = NULL;

    // Enable line tracking
    m_fTrackLines = TRUE;

    FULLTOKEN       full;
    CSourceLexer    ctx (this);

    // Set up the lexer context to start at the appropriate place
    if (fIncremental)
    {
        ctx.m_iCurLine = md.posStart.iLine;
        ctx.m_pszCurLine = m_lex.pszSource + old.piLines[ctx.m_iCurLine];
        ctx.m_pszCurrent = ctx.m_pszCurLine + md.posStart.iChar;
    }
    else
    {
        ctx.m_pszCurLine = m_lex.pszSource;
        ctx.m_pszCurrent = m_lex.pszSource;
        ctx.m_iCurLine = 0;
    }

    m_lex.piLines[0] = 0;       // Always...
    ctx.m_fPreproc = TRUE;
    ctx.m_fFirstOnLine = TRUE;
    ctx.m_fTokensSeen = !md.fStartAtZero;



    ////////////////////////////////////////////////////////////////////////////
    //
    // Here we go!!!  Scan tokens until we hit md.posStop (if fIncremental) or EOF.
    //
    ////////////////////////////////////////////////////////////////////////////

    if (!m_lex.pIdentTable) {
        full.partialOk = true;
    }

    while (TRUE)
    {
        TOKENID     iTok = ctx.ScanToken (&full);

        if (m_lex.iTokens >= m_lex.iAllocTokens)
        {
            // Must make space for more tokens and positions.
            m_lex.iAllocTokens += (((m_lex.iTextLen - (long)(ctx.m_pszCurrent - m_lex.pszSource)) >> 5) + 32) & ~31;
            m_lex.pTokens = (BYTE *)m_pStdHeap->Realloc (m_lex.pTokens, (m_lex.iAllocTokens * sizeof (BYTE)));
            m_lex.pposTokens = (POSDATA *)m_pStdHeap->Realloc (m_lex.pposTokens, (m_lex.iAllocTokens * sizeof (POSDATA)));
        }

        m_lex.pTokens[m_lex.iTokens] = (BYTE)iTok;
        m_lex.pposTokens[m_lex.iTokens] = full.posTokenStart;

        if (iTok == TID_IDENTIFIER && m_lex.pIdentTable != NULL)
            m_lex.pIdentTable->Add (full.pName, full.pName);

        if (md.fIncrementalStop && full.posTokenStart >= md.posStop)
        {
            // Here, we check to see if we can stop short.  In order to do so, we must...
            //  1) ...have stopped on the 'stop' token, at the 'stop' position
            //  2) ...have NOT changed CC state
            //  3) ...have NOT changed CC symbol table
            if (m_lex.pTokens[m_lex.iTokens] == old.pTokens[md.iTokStop] && full.posTokenStart == md.posStop)
            {
                // So far, so good -- we stopped on the right token.  Now check the CC symbol
                // table (if necessary) to see if any symbols were added/removed by this edit.
                // As a shortcut, any define/undef will have set the m_fCCSymbolChange flag.
                if (m_fCCSymbolChange)
                {
                    // We kept a copy of last time's symbol table prior to the edit position.
                    // See if additions/deletions made.
                    if (OldTable.GetCount() == m_tableDefines.GetCount())
                    {
                        // The counts are the same -- so assume the symbols are until proven
                        // otherwise...
                        m_fCCSymbolChange = FALSE;
                        for (long i=0; i<OldTable.GetCount(); i++)
                            if (!m_tableDefines.IsDefined (OldTable.GetAt (i)))
                            {
                                m_fCCSymbolChange = TRUE;
                                break;
                            }
                    }

#ifdef DEBUG
                    if (m_fCCSymbolChange)
                        RETOKSPEW (DBGOUT (("CC SYMBOL TABLE CHANGED:  Tokenization must continue to end-of-file...")));
#endif
                }

                // If there were no CC symbol changes, check to see if CC state changes
                // were made.
                if (!m_fCCSymbolChange)
                {
                    // Make sure the CC stack is where we expect.  That means the pCopyStack should be
                    // identical to m_pCCStack
                    if (!CompareCCStacks (m_pCCStack, pCopyStack))
                    {
                        RETOKSPEW (DBGOUT (("INCOMPATIBLE CC STATE CHANGE IN EDIT:  Tokenization must continue to end-of-file...")));
                        m_fCCSymbolChange = TRUE;
                    }
                }

                if (!m_fCCSymbolChange && m_LineMap.IsEmpty())  // Only do an incremental tokenization if there still aren't any line maps
                {
                    // It's not technically an incremental stop if the stop token was the end-of-file token...
                    if (iTok != TID_ENDFILE)
                    {
                        // YES!!!
                        RETOKSPEW (DBGOUT (("INCREMENTAL TOKENIZATION ROCKS:  Scanned %d tokens instead of %d!", m_lex.iTokens + 1, old.iTokens + m_lex.iTokens - (md.iTokStop - md.iTokStart))));

                        // Since we stopped short, make sure the line count is retained.
                        m_lex.iLines = old.iLines + md.iLineDelta;
                        break;
                    }
                }
            }
            else
            {
                RETOKSPEW (DBGOUT (("STOP TOKEN/POSITION MISMATCH:  Tokenization must continue to end-of-file...")));
            }

            // Nuts.  This means we have to tokenize the rest of the file.  It also means we bail on
            // the idea of parsing incrementally.
            md.fIncrementalStop = FALSE;
            m_fForceTopParse = TRUE;
        }

        m_lex.iTokens++;

        if (iTok == TID_ENDFILE)
        {
            m_lex.iLines++;            // This "includes" the last line
            md.fIncrementalStop = FALSE;   // Since we hit EOF, we didn't stop before the end, now did we?
            break;
        }

        if (ctx.m_fLimitExceeded)
        {
            // So as not to spew any more errors, stop here...
            md.fIncrementalStop = FALSE;
            break;
        }
    }

    // Turn line tracking back off
    m_fTrackLines = FALSE;



    ////////////////////////////////////////////////////////////////////////////
    //
    // Tokenization is done -- now, on to the merge.
    //
    ////////////////////////////////////////////////////////////////////////////

    // Assume the token stream has changed, but the transitions and comments have not.
    md.fStreamChanged = TRUE;
    md.fTransitionsChanged = FALSE;
    md.iStartComment = -1;

    BOOL    fWhackOld = TRUE;           // If TRUE when we're done merging we need to delete the old stuff

    // Okay, we're done with the loop.  We either bailed early because of a successful incremental
    // tokenization, or we hit the end of the file.  Either way, we need to "merge" the new data
    // with the old -- unless of course, we don't HAVE any old data.
    if (old.pTokens != NULL)
    {
        // Check to see if we started at the beginning of the file, and had to parse the whole
        // thing.  (NOTE:  This is always the case if the user added a #define/#undef)  If so,
        // we can just delete the old stuff; the new stuff is complete.
        if (md.fIncrementalStop || !md.fStartAtZero)
        {
            ASSERT (fIncremental);

            // Merge the token stream...
            MergeTokenStream (old, md);

            // ...the comment table...
            MergeCommentTable (old, md);

            // ...the CC state transitions and change array...
            MergeCCState (old, md);

            // ...and the line table
            MergeLineTable (old, md);

            // Since the above functions did the work, we don't have to delete the old stuff...
            fWhackOld = FALSE;
        }
        else
        {
            // We started at the beginning and stopped at the end, so we'll just use the
            // new data -- all of it -- and delete the old stuff (leave fWhackOld set)
        }
    }
    else
    {
        // No old data to delete
        fWhackOld = FALSE;

        // ..except for these...
        if (old.piTransitionLines != NULL)
            VSFree (old.piTransitionLines);
        if (old.pCCStates != NULL)
            VSFree (old.pCCStates);
    }

    if (fWhackOld)
    {
        // Delete old data
        m_pStdHeap->Free (old.pTokens);
        m_pStdHeap->Free (old.pposTokens);
        m_pStdHeap->Free (old.pposComments);
        m_pStdHeap->Free (old.piLines);
        if (old.pCCStates != NULL)
            VSFree (old.pCCStates);
        if (old.piTransitionLines != NULL)
            VSFree (old.piTransitionLines);
    }

    // Destroy our copy of the CC stack
    while (pCopyStack != NULL)
    {
        CCREC   *pTmp = pCopyStack->pPrev;
        VSFree (pCopyStack);
        pCopyStack = pTmp;
    }

    // Get rid of any remaining CC stack records.  If there are some, and we scanned to the end
    // of the file, report an error.
    if (m_pCCStack != NULL)
    {
        if (!md.fIncrementalStop)
            ErrorAtPosition (m_lex.pposTokens[m_lex.iTokens-1].iLine, m_lex.pposTokens[m_lex.iTokens-1].iChar, 1, (m_pCCStack->dwFlags & CCF_REGION) ? ERR_EndRegionDirectiveExpected : ERR_EndifDirectiveExpected);

        while (PopCCRecord (m_pCCStack))
            ;
    }

    // At this point, handle whether the compiler limit was exceeded.  If so, we "revert" to
    // an empty file to avoid parsing with bogus position data.
    if (ctx.m_fLimitExceeded)
    {
        m_fLimitExceeded = TRUE;
        m_lex.iTokens = 1;
        m_lex.pTokens[0] = TID_ENDFILE;
        m_lex.iTransitionLines = 0;
        m_lex.iComments = 0;
        m_lex.iRegions = 0;
        m_lex.iCCStates = 0;
    }

    // Build our region array.  This is done after tokenization, incremental or
    // not, because it can easily be created from the CC state change table.
    UpdateRegionTable ();


    ////////////////////////////////////////////////////////////////////////////
    //
    // Okay, now we must update the state bits, so re-acquire the state lock
    //
    ////////////////////////////////////////////////////////////////////////////
    gate.Acquire ();

    // We're now tokenized (no longer tokenizing), and the version gets incremented.
    m_fTokenized = TRUE;
    m_fTokenizing = FALSE;
    m_dwVersion++;
    ClearTokenizerThread();

    // Check to see if parsing is required (only if md.fStreamChanged!)
    if (md.fStreamChanged)
    {
        // Mark the token stream as changed, and remember the first and last changed
        // token indexes, so the next top-level parse request can determine if it
        // needs to actually do any work.  Note that if we stopped incrementally,
        // m_iTokDeltaLast is already known
        m_fTokensChanged = TRUE;

        long    iFirstChanged;

        // If the first token we scanned is the same as what it was before, we
        // don't need to consider it changed...
        if (m_lex.pTokens[md.iTokStart] != md.tokFirst || CParser::GetTokenInfo(md.tokFirst)->iLen == 0)
            iFirstChanged = md.iTokStart;
        else
            iFirstChanged = md.iTokStart + 1;

        // Since the token delta is an accumulation, we do a 'min' here...
        if (iFirstChanged < m_iTokDeltaFirst)
            m_iTokDeltaFirst = iFirstChanged;

        // If we didn't stop incrementally, then the last token is the last one changed
        if (!md.fIncrementalStop)
            m_iTokDeltaLast = m_lex.iTokens - 1;
    }

    // Complete the replacement of tokenization errors.
    m_pTokenErrors->EndReplacement (md.posStart, md.fIncrementalStop ? md.posStopOld : md.posOldEOF);
    m_pCurrentErrors = NULL;
    pErrorTemp = m_pTokenErrors;        // Use this to fire events after we release the lock...

    ////////////////////////////////////////////////////////////////////////////
    //
    // Release the lock -- we need to fire events now.
    //
    ////////////////////////////////////////////////////////////////////////////
    gate.Release();


    BOOL    fFire = FALSE;
    long    iTop;

    // Check to see if firing the state-transitions-changed event is necessary.
    if (m_lex.iTransitionLines != iTransitionCopy)
    {
        fFire = TRUE;
        iTop = 0;
    }
    else
    {
        ASSERT (m_lex.iTransitionLines == 0 || piTransitionCopy != NULL);
        for (iTop=0; iTop < m_lex.iTransitionLines; iTop++)
        {
            // NOTE: if the transition is past the edit, update it with the line delta
            if (piTransitionCopy[iTop] > (long)md.posEditOldEnd.iLine)
                piTransitionCopy[iTop] += md.iLineDelta;

            if (m_lex.piTransitionLines[iTop] != piTransitionCopy[iTop])
            {
                fFire = TRUE;
                break;
            }
        }
    }


    // Stop the timer -- we're done!
    TimerStop(TIME_CREATETOKENSTREAM);
}


////////////////////////////////////////////////////////////////////////////////
// CSourceModule::TrackLine
//
// This function is called from the tokenizer when a physical end-of-line is
// found, to update the line table (growing it if necessary) and keep the lexer
// context up to date.

inline void CSourceModule::TrackLine (CLexer *pCtx, PCWSTR pszNewLine)
{
    // Lexer limit checks -- can't exceed the max sizes specified by POSDATA
    // structures (MAX_POS_LINE_LEN chars/line, MAX_POS_LINE lines)
    if (pCtx->m_iCurLine == MAX_POS_LINE - 1)
    {
        ErrorAtPosition (pCtx->m_iCurLine, 0, 1, ERR_TooManyLines, MAX_POS_LINE);
        pCtx->m_fLimitExceeded = TRUE;
    }
    else if (!pCtx->m_fThisLineTooLong && pszNewLine - pCtx->m_pszCurLine > MAX_POS_LINE_LEN + 1)        // (plus one for CRLF)
    {
        ErrorAtPosition (pCtx->m_iCurLine, MAX_POS_LINE_LEN - 1, 1, ERR_LineTooLong, MAX_POS_LINE_LEN);
        pCtx->m_fLimitExceeded = TRUE;
    }

    // Grow the array if needed
    if (m_fTrackLines && (pCtx->m_iCurLine == m_lex.iAllocLines - 1))
    {
        // Use guess to determine number of remaining lines
        m_lex.iAllocLines += (((m_lex.iTextLen - (long)(pCtx->m_pszCurrent - m_lex.pszSource)) >> 5) + 32) & ~31;
        m_lex.piLines = (long *)m_pStdHeap->Realloc (m_lex.piLines, m_lex.iAllocLines * sizeof (PCWSTR));
    }

    pCtx->m_iCurLine++;
    if (m_fTrackLines)
        m_lex.piLines[m_lex.iLines = pCtx->m_iCurLine] = (long)(pszNewLine - m_lex.pszSource);
    else
        ASSERT (m_lex.piLines[pCtx->m_iCurLine] == pszNewLine - m_lex.pszSource);

    pCtx->m_pszCurLine = pszNewLine;
    pCtx->m_fFirstOnLine = TRUE;
}


////////////////////////////////////////////////////////////////////////////////
// CSourceModule::RecordCommentPosition
//
// Called from the tokenizer to record the position of the beginning of a
// comment in the source.

void CSourceModule::RecordCommentPosition (const POSDATA &pos)
{
    if (!m_fTrackLines)
        return;     // Don't do this unless we're tracking lines as well...

    // Check for need to re-allocate
    if (m_lex.iComments >= m_lex.iAllocComments)
    {
        m_lex.iAllocComments += 32;     // Just grow in blocks of 32 from here out...
        m_lex.pposComments = (POSDATA *)m_pStdHeap->Realloc (m_lex.pposComments, m_lex.iAllocComments * sizeof (POSDATA));
    }

    m_lex.pposComments[m_lex.iComments++] = pos;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::ScanPreprocessorLine

void CSourceModule::ScanPreprocessorLine (CLexer *pCtx, PCWSTR &p, BOOL fFirstOnLine)
{
    WCHAR   ch;
    PCWSTR  pStart = p;

    if (!fFirstOnLine)
    {
        // This is an error condition -- preprocessor directives must be first token
        ErrorAtPosition (pCtx->m_iCurLine, (p-1) - pCtx->m_pszCurLine, 1, ERR_BadDirectivePlacement);
        SkipRemainingPPTokens (p);
    }
    else
    {
        // Scan the actual directive
        p = pCtx->m_pszTokenStart;
        ScanPreprocessorDirective (pCtx, p);
    }

    // Make sure we end correctly...
    ScanPreprocessorLineTerminator (pCtx, p);

    // Scan to end-of-line
    while (TRUE)
    {
        if (PEEKCHAR (pStart, p, ch) == '\n' || ch == UCH_PS || ch == UCH_LS)
        {
            PCWSTR  pszTrack = (*p == ch) ? p + 1 : NULL;
            NEXTCHAR (pStart, p, ch);
            if (pszTrack)
                TrackLine (pCtx, pszTrack); // Only track the line if it's a physical LF (non-escaped)
            break;
        }
        else if (ch == '\r')
        {
            PCWSTR  pszTrack = (*p == '\r') ? p + 1 : NULL;
            NEXTCHAR (pStart, p, ch);
            if (PEEKCHAR (pStart, p, ch) == '\n')
            {
                if (*p == '\n')
                    pszTrack = p + 1;
                NEXTCHAR (pStart, p, ch);
            }

            if (pszTrack != NULL)
                TrackLine (pCtx, pszTrack); // Only track physical (non-escaped) CR/CRLF guys
            break;
        }
        else if (*p == 0)
        {
            break;
        }
        else
        {
            NEXTCHAR (pStart, p, ch);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::ScanPreprocessorLineTerminator

void CSourceModule::ScanPreprocessorLineTerminator (CLexer *pCtx, PCWSTR &p)
{
    PCWSTR      pszStart;
    PPTOKENID   tok;
    NAME        *pName;

    tok = ScanPreprocessorToken (p, pszStart, &pName, false);
    if (tok != PPT_EOL)
        ErrorAtPosition (pCtx->m_iCurLine, pszStart - pCtx->m_pszCurLine, p - pszStart, ERR_EndOfPPLineExpected);
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::ScanPreprocessorDirective

void CSourceModule::ScanPreprocessorDirective (CLexer *pCtx, PCWSTR &p)
{
    WCHAR   ch;
    PCWSTR  pszStart;
    PCWSTR  pStart = p;

    // Scan/skip the '#' character
    ch = NEXTCHAR (pStart, p, ch);
    ASSERT (ch == '#');

    NAME        *pName;
    PPTOKENID   ppTok;

    // Scan a PP token, handle accordingly
    ppTok = ScanPreprocessorToken (p, pszStart, &pName, true);

    switch (ppTok)
    {
        case PPT_DEFINE:
        case PPT_UNDEF:
            // These can't happen beyond the first token!
            if (pCtx->m_fTokensSeen)
            {
                ErrorAtPosition (pCtx->m_iCurLine, pszStart - pCtx->m_pszCurLine, p - pszStart, ERR_PPDefFollowsToken);
                SkipRemainingPPTokens (p);
                return;
            }

            ScanPreprocessorDeclaration (p, pCtx, ppTok == PPT_DEFINE);
            break;

        case PPT_ERROR:
        case PPT_WARNING:
            ScanPreprocessorControlLine (p, pCtx, ppTok == PPT_ERROR);
            break;

        case PPT_REGION:
            ScanPreprocessorRegionStart (p, pCtx);
            break;

        case PPT_ENDREGION:
            p = pszStart;
            ScanPreprocessorRegionEnd (p, pCtx);
            break;

        case PPT_IF:
            ScanPreprocessorIfSection (p, pCtx);
            break;

        case PPT_ELIF:
        case PPT_ELSE:
        case PPT_ENDIF:
            p = pszStart;
            ScanPreprocessorIfTrailer (p, pCtx);
            break;

        case PPT_LINE:
            ScanPreprocessorLineDecl (p, pCtx);
            break;

        default:
            ErrorAtPosition (pCtx->m_iCurLine, pszStart - pCtx->m_pszCurLine, p - pszStart, ERR_PPDirectiveExpected);
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::ScanPreprocessorToken
//
// if allIds is true, any id-like pp-token is returned (if, define, region, true, ...)
// if allIds is false, only "true" and "false" are considered special tokens, and
// "if", "region", etc. are considered identifiers.

PPTOKENID CSourceModule::ScanPreprocessorToken (PCWSTR &p, PCWSTR &pszStart, NAME **ppName, bool allIds)
{
    WCHAR   ch, szTok[MAX_PPTOKEN_LEN + 1];
    PCWSTR  pStart = p;

    // Skip whitespace
    while (PEEKCHAR (pStart, p, ch) == ' ' || ch == '\t')
        NEXTCHAR (pStart, p, ch);

    pszStart = p;

    // Check for end-of-line
    if (*p == 0 || ch == '\r' || ch == '\n' || ch == UCH_PS || ch == UCH_LS)
        return PPT_EOL;

    PWSTR   pszRun = szTok;

    switch (*pszRun++ = NEXTCHAR (pStart, p, ch))
    {
        case '(':
            return PPT_OPENPAREN;

        case ')':
            return PPT_CLOSEPAREN;

        case '!':
            if (PEEKCHAR (pStart, p, ch) == '=')
            {
                NEXTCHAR (pStart, p, ch);
                return PPT_NOTEQUAL;
            }
            return PPT_NOT;

        case '=':
            if (PEEKCHAR (pStart, p, ch) == '=')
            {
                NEXTCHAR (pStart, p, ch);
                return PPT_EQUAL;
            }
            break;

        case '&':
            if (PEEKCHAR (pStart, p, ch) == '&')
            {
                NEXTCHAR (pStart, p, ch);
                return PPT_AND;
            }
            break;

        case '|':
            if (PEEKCHAR (pStart, p, ch) == '|')
            {
                NEXTCHAR (pStart, p, ch);
                return PPT_OR;
            }
            break;

        case '/':
            if (PEEKCHAR (pStart, p, ch) == '/')
            {
                p = pszStart;       // Don't go past EOL...
                return PPT_EOL;
            }
            break;

        default:
        {
            // Only other thing can be identifiers, which are post-checked
            // for the token identifiers (if, define, etc.)
            if (!IsIdentifierChar (ch))
                break;

            for (; IsIdentifierCharOrDigit (PEEKCHAR (pStart, p, ch)); NEXTCHAR (pStart, p, ch))
            {
                if (pszRun - szTok < MAX_PPTOKEN_LEN)
                    *pszRun++ = ch;
            }

            *pszRun = 0;
            *ppName = GetNameMgr()->AddString (szTok);
            int i;
            if (GetNameMgr()->IsPPKeyword (*ppName, &i)) {
                if (allIds || i == PPT_TRUE || i == PPT_FALSE)
                    return (PPTOKENID)i;
            }

            return PPT_IDENTIFIER;
        }

    }

    // If you break out of the above switch, it is assumed you didn't recognize
    // anything...
    return PPT_UNKNOWN;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::ScanPreprocessorDeclaration

void CSourceModule::ScanPreprocessorDeclaration (PCWSTR &p, CLexer *pCtx, BOOL fDefine)
{
    NAME    *pName;
    PCWSTR  pszStart;

    if (ScanPreprocessorToken (p, pszStart, &pName, false) != PPT_IDENTIFIER)
    {
        ErrorAtPosition (pCtx->m_iCurLine, pszStart - pCtx->m_pszCurLine, p - pszStart, ERR_IdentifierExpected);
        return;
    }

    if (fDefine)
        m_tableDefines.Define (pName);
    else
        m_tableDefines.Undef (pName);
    m_fCCSymbolChange = TRUE;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::ScanPreprocessorControlLine

void CSourceModule::ScanPreprocessorControlLine (PCWSTR &p, CLexer *pCtx, BOOL fError)
{
    WCHAR   ch, szBuf[MAX_PPTOKEN_LEN + 1];
    PWSTR   pszRun = szBuf;
    PCWSTR  pStart = p;

    // The rest of this line (including any single-line comments) is the error/warning text.
    // First, skip leading whitespace
    while (PEEKCHAR (pStart, p, ch) == ' ' || ch == '\t')
        NEXTCHAR (pStart, p, ch);

    PCWSTR  pszStart = p;

    // Copy the rest of the line (up to MAX_PPTOKEN_LEN chars) into szBuf
    while (TRUE)
    {
        PEEKCHAR (pStart, p, ch);
        if (ch == '\r' || ch == '\n' || ch == 0 || ch == UCH_PS || ch == UCH_LS)
            break;

        if (pszRun - szBuf < MAX_PPTOKEN_LEN)
            *pszRun++ = ch;

        NEXTCHAR (pStart, p, ch);
    }

    *pszRun = 0;

    ErrorAtPosition (pCtx->m_iCurLine, pszStart - pCtx->m_pszCurLine, p - pszStart, fError ? ERR_ErrorDirective : WRN_WarningDirective, szBuf);
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::ScanPreprocessorRegionStart

void CSourceModule::ScanPreprocessorRegionStart (PCWSTR &p, CLexer *pCtx)
{
    // This is pretty simply -- just push a record on the CC stack so that we
    // don't allow overlapping of regions with #if guys...
    PushCCRecord (m_pCCStack, CCF_REGION);
    MarkCCStateChange (pCtx->m_iCurLine, CCF_REGION|CCF_ENTER);

    // We don't care about the text following...
    SkipRemainingPPTokens (p);
    return;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::ScanPreprocessorRegionEnd

void CSourceModule::ScanPreprocessorRegionEnd (PCWSTR &p, CLexer *pCtx)
{
    NAME        *pName;
    PCWSTR      pszStart;
#ifdef DEBUG
    PPTOKENID   tok =
#endif
    ScanPreprocessorToken (p, pszStart, &pName, true);

    ASSERT (tok == PPT_ENDREGION);

    // Make sure we're in the right kind of CC stack state
    if (m_pCCStack == NULL || (m_pCCStack->dwFlags & CCF_REGION) == 0)
    {
        ErrorAtPosition (pCtx->m_iCurLine, pszStart - pCtx->m_pszCurLine, p - pszStart, m_pCCStack == NULL ? ERR_UnexpectedDirective : ERR_EndifDirectiveExpected);
    }
    else
    {
        PopCCRecord (m_pCCStack);
        MarkCCStateChange (pCtx->m_iCurLine, CCF_REGION);
    }

    SkipRemainingPPTokens (p);
    return;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::ScanPreprocessorIfSection

void CSourceModule::ScanPreprocessorIfSection (PCWSTR &p, CLexer *pCtx)
{
    NAME    *pName;
    PCWSTR  pszStart;
    PCWSTR  pStart = p;

    // First, scan and evaluate a preprocessor expression.   If TRUE, push an
    // appropriate state record and return
    if (EvalPreprocessorExpression (p, pCtx))
    {
        // Push a CC record -- it is NOT an else block.  Also mark a state
        // transition (to reconstruct this stack state at incremental tokenization
        // time)
        PushCCRecord (m_pCCStack, 0);
        MarkCCStateChange (pCtx->m_iCurLine, CCF_ENTER);
        return;
    }

    CCREC   *pSkipStack = NULL;
    WCHAR   ch;

    // Mark this line as a transition (we're going from include to exclude)
    MarkTransition (pCtx->m_iCurLine);

    // Make sure there's no other gunk on the end of this line
    ScanPreprocessorLineTerminator (pCtx, p);

    while (TRUE)
    {
        // Scan for the next preprocessor directive
        ScanExcludedCode (p, pCtx);

        // Check for '#' (if not present, ScanExcludedCode will have reported the error...)
        if (PEEKCHAR (pStart, p, ch) != '#')
        {
            // Don't leak the now-useless skip stack entries
            while (pSkipStack != NULL)
                PopCCRecord (pSkipStack);
            return;     // NOTE:  No transition, so just return from here
        }

        // Scan the directive
        NEXTCHAR (pStart, p, ch);
        PPTOKENID   tok = ScanPreprocessorToken (p, pszStart, &pName, true);

        if (pSkipStack != NULL &&
           (((pSkipStack->dwFlags & CCF_REGION) && (tok == PPT_ELSE || tok == PPT_ELIF || tok == PPT_ENDIF)) ||
            ((pSkipStack->dwFlags & CCF_REGION) == 0 && (tok == PPT_ENDREGION))))
        {
            // Bad mojo -- can't overlap #region/#endregion and #if/#else/elif/#endif blocks
            ErrorAtPosition (pCtx->m_iCurLine, pszStart - pCtx->m_pszCurLine, p - pszStart, (pSkipStack->dwFlags & CCF_REGION) ? ERR_EndRegionDirectiveExpected : ERR_EndifDirectiveExpected);
            ScanAndIgnoreDirective (tok, pszStart, p, pCtx);
            continue;
        }

        if (tok == PPT_ELSE)
        {
            if (pSkipStack == NULL)
            {
                // Push a CC record -- we ARE in the else block now.  Also mark
                // a CC state transition to enable stack reconstruction.  Then break
                // out of the while loop to mark this as a transition line
                PushCCRecord (m_pCCStack, CCF_ELSE);
                MarkCCStateChange (pCtx->m_iCurLine, CCF_ELSE|CCF_ENTER);
                break;
            }

            ASSERT ((pSkipStack->dwFlags & CCF_REGION) == 0);
        }


        if (tok == PPT_ELIF)
        {
            if (pSkipStack == NULL)
            {
                if (EvalPreprocessorExpression (p, pCtx))
                {
                    // This is the same as an IF block -- it is NOT an "else" block.
                    // Record the state change for reconstruction on retokenization.
                    // Break from the loop, which will mark this line as a transition
                    // and return.
                    PushCCRecord (m_pCCStack, 0);
                    MarkCCStateChange (pCtx->m_iCurLine, CCF_ENTER);
                    break;
                }

                // Make sure the line ends "cleanly" and then go scan another excluded block
                ScanPreprocessorLineTerminator (pCtx, p);
                continue;
            }

            ASSERT ((pSkipStack->dwFlags & CCF_REGION) == 0);
        }

        if (tok == PPT_IF)
        {
            // This is the beginning of another if-section.  Push an 'if' record on to
            // our skip stack, and then scan (and ignore) the directive to make sure it
            // is syntactically correct.
            PushCCRecord (pSkipStack, 0);
            ScanAndIgnoreDirective (tok, pszStart, p, pCtx);
            continue;
        }

        if (tok == PPT_ENDIF)
        {
            // Here's the end of an if-section (but not necessarily ours).  If we
            // are currently at the same depth as when we started, we're done.
            // Otherwise, pop the top record off of our skip stack and continue.
            if (pSkipStack != NULL)
            {
                PopCCRecord (pSkipStack);
                ScanAndIgnoreDirective (tok, pszStart, p, pCtx);
                continue;
            }

            // This was our endif.  Break out, mark this line as transition.
            break;
        }

        if (tok == PPT_REGION)
        {
            // Here's the beginning of a region.  We don't care about it, but to
            // enforce correct nesting, we push a record on our skip stack
            PushCCRecord (pSkipStack, CCF_REGION);
            ScanAndIgnoreDirective (tok, pszStart, p, pCtx);
            continue;
        }

        if (tok == PPT_ENDREGION)
        {
            // In order to be correct this must match the topmost record on
            // our skip stack
            if (pSkipStack == NULL || (pSkipStack->dwFlags & CCF_REGION) == 0)
                ErrorAtPosition (pCtx->m_iCurLine, pszStart - pCtx->m_pszCurLine, p - pszStart, pSkipStack != NULL ? ERR_EndifDirectiveExpected : ERR_UnexpectedDirective);
            else
                PopCCRecord (pSkipStack);
        }

        // Unrecognized/ignored directive/token -- skip it, checking it for errors first
        ScanAndIgnoreDirective (tok, pszStart, p, pCtx);
    }

    // Mark this line as another transition (from exclude to include)
    MarkTransition (pCtx->m_iCurLine);

    ASSERT (pSkipStack == NULL);
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::ScanPreprocessorIfTrailer

void CSourceModule::ScanPreprocessorIfTrailer (PCWSTR &p, CLexer *pCtx)
{
    NAME        *pName;
    PCWSTR      pszStart;
    PCWSTR      pStart = p;
    PPTOKENID   tok = ScanPreprocessorToken (p, pszStart, &pName, true);

    // Here, we check to see if we are currently in a CC state.  If we aren't,
    // the directive found is unexpected
    if (m_pCCStack == NULL || m_pCCStack->dwFlags & CCF_REGION)
    {
        ErrorAtPosition (pCtx->m_iCurLine, pszStart - pCtx->m_pszCurLine, p - pszStart, m_pCCStack != NULL ? ERR_EndRegionDirectiveExpected : ERR_UnexpectedDirective);

        // Scan the rest of the tokens off so we don't get more errors.
        SkipRemainingPPTokens (p);
        return;
    }

    CCREC   *pSkipStack = NULL;
    WCHAR   ch;

    // This line marks the state change (exit).  Do that now.
    MarkCCStateChange (pCtx->m_iCurLine, 0);

    // If this is not a PPT_ENDIF, we need to skip excluded code blocks
    if (tok != PPT_ENDIF)
    {
        // We're transitioning here...
        MarkTransition (pCtx->m_iCurLine);

        while (TRUE)
        {
            if (pSkipStack != NULL &&
               (((pSkipStack->dwFlags & CCF_REGION) && (tok == PPT_ELSE || tok == PPT_ELIF || tok == PPT_ENDIF)) ||
                ((pSkipStack->dwFlags & CCF_REGION) == 0 && (tok == PPT_ENDREGION))))
            {
                // Bad mojo -- can't overlap #region/#endregion and #if/#else/elif/#endif blocks
                ErrorAtPosition (pCtx->m_iCurLine, pszStart - pCtx->m_pszCurLine, p - pszStart, (pSkipStack->dwFlags & CCF_REGION) ? ERR_EndRegionDirectiveExpected : ERR_EndifDirectiveExpected);
            }
            else
            {
                // If there's nothing on our skip stack, we need to sniff this directive a bit...
                if (pSkipStack == NULL)
                {
                    ASSERT (m_pCCStack != NULL && (m_pCCStack->dwFlags & CCF_REGION) == 0);

                    // If we were in the "else" block, we can't see any more else or elif blocks!
                    if ((m_pCCStack->dwFlags & CCF_ELSE) && (tok == PPT_ELSE || tok == PPT_ELIF))
                        ErrorAtPosition (pCtx->m_iCurLine, pszStart - pCtx->m_pszCurLine, p - pszStart, ERR_UnexpectedDirective);

                    // #endregion is also bogus here...
                    if (tok == PPT_ENDREGION)
                        ErrorAtPosition (pCtx->m_iCurLine, pszStart - pCtx->m_pszCurLine, p - pszStart, ERR_UnexpectedDirective);

                    // Is this the else?  If so, mark it in the state record
                    if (tok == PPT_ELSE)
                        m_pCCStack->dwFlags |= CCF_ELSE;
                }

                // Handle the directives appropriately -- we care about #if/#region, and #endif/#endregion
                if (tok == PPT_IF || tok == PPT_REGION)
                {
                    // Push an appropriate record on the skip stack.
                    PushCCRecord (pSkipStack, tok == PPT_REGION ? CCF_REGION : 0);
                }
                else if (tok == PPT_ENDIF || tok == PPT_ENDREGION)
                {
                    // If the skip stack is empty and this was an #endif, we're done.
                    if (tok == PPT_ENDIF && pSkipStack == NULL)
                        break;

                    // Make sure the stack corresponds to the directive
                    if (pSkipStack == NULL || (tok == PPT_ENDIF && (pSkipStack->dwFlags & CCF_REGION)) || (tok == PPT_ENDREGION && (pSkipStack->dwFlags & CCF_REGION) == 0))
                    {
                        short   iErr;

                        if (pSkipStack == NULL)
                            iErr = (m_pCCStack->dwFlags & CCF_REGION) ? ERR_EndRegionDirectiveExpected : ERR_EndifDirectiveExpected;
                        else
                            iErr = (pSkipStack->dwFlags & CCF_REGION) ? ERR_EndRegionDirectiveExpected : ERR_EndifDirectiveExpected;

                        ErrorAtPosition (pCtx->m_iCurLine, pszStart - pCtx->m_pszCurLine, p - pszStart, iErr);
                    }
                    else
                    {
                        PopCCRecord (pSkipStack);
                    }
                }
            }

            // Scan and ignore the rest of whatever this directive was
            ScanAndIgnoreDirective (tok, pszStart, p, pCtx);

            // Continue scanning excluded code for another directive
            ScanExcludedCode (p, pCtx);

            // Check for '#' (if not present, ScanExcludedCode will have reported the error...)
            if (PEEKCHAR (pStart, p, ch) != '#')
            {
                // Don't leak the now worthless skip stack entries...
                while (pSkipStack != NULL)
                    PopCCRecord (pSkipStack);

                return;     // Don't mark the reverse transition...
            }

            // Scan the new directive and continue
            NEXTCHAR (pStart, p, ch);
            tok = ScanPreprocessorToken (p, pszStart, &pName, true);
        }

        // Transition back out...
        MarkTransition (pCtx->m_iCurLine);
    }

    // Okay... pop the state record and return...
    PopCCRecord (m_pCCStack);

    ASSERT (pSkipStack == NULL);
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::MapLocation
//
// Does NOT allocate memory for ppszFilename, it's just a pointer
// If no mapping changes the filename, this will use ICSSourceText->GetName()
// to supply the ppszFilename

void CSourceModule::MapLocation (POSDATA *pos, LPCWSTR *ppszFilename)
{
    if (ppszFilename != NULL)
    {
        NAME * name = NULL;
        m_LineMap.Map(pos, &name);
        if (name != NULL)
            *ppszFilename = name->text;
        else
            m_pSourceText->GetName(ppszFilename);
    }
    else
    {
        m_LineMap.Map(pos, NULL);
    }
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::MapLocation
//
// Does NOT allocate memory for ppszFilename, it's just a pointer
// If no previous mapping changes the filename, then ppszFilename is
// left unchanged (this is different behaviour from the other MapLocation)

void CSourceModule::MapLocation(long *line, NAME ** ppszFilename)
{
    *line = m_LineMap.Map(*line, ppszFilename);
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::ScanPreprocessorLineDecl

void CSourceModule::ScanPreprocessorLineDecl (PCWSTR &p, CLexer *pCtx)
{
    WCHAR   ch;
    PCWSTR  pStart = p;
    PCWSTR  pszStart = NULL;
    NAME   *pName = NULL;


    // The rest of this line (including any single-line comments) is the line and optional filename

    // First check for  "default" (this will skip past whitespace)
    PPTOKENID tok = ScanPreprocessorToken(p, pszStart, &pName, false);
    if (tok == PPT_IDENTIFIER && pName == m_pDefaultName) {
        PCWSTR w_original = NULL;
        m_pSourceText->GetName (&w_original);
        pName = GetNameMgr()->AddString( w_original);
        m_LineMap.AddMap(pCtx->m_iCurLine, pCtx->m_iCurLine + 1, pName);
    } else {
        WCHAR   szBuf[_MAX_PATH + 1];
        PWSTR   pszFilename = szBuf;
        bool    hasFilename = false;
        bool    isValidNum = true, isValidName = true;
        long    newLine = 0;

        // Reset to before the token
        p = pszStart;
        // get the line number
        while (PEEKCHAR (pStart, p, ch) >= '0' && ch <= '9')
        {
            long oldLine = (newLine == 0 ? -1 : newLine);
            newLine = newLine * 10 + (ch - '0');
            if (newLine <= oldLine && isValidNum)
            {
                ErrorAtPosition (pCtx->m_iCurLine, p - pCtx->m_pszCurLine,  0, ERR_IntOverflow);
                isValidNum = false;
                // Keep parsing the number so we don't get multiple bogus errors
            }
            NEXTCHAR (pStart, p, ch);
        }

        if (isValidNum && (newLine < 1 || newLine > MAX_POS_LINE))
            ErrorAtPosition (pCtx->m_iCurLine, p - pCtx->m_pszCurLine,  0, ERR_InvalidLineNumber);

        // do we have a filename?
        while (!hasFilename)
        {
            PEEKCHAR (pStart, p, ch);
            if (ch == '\r' || ch == '\n' || ch == 0 || ch == UCH_PS || ch == UCH_LS)
                break;
            if (ch == '\"')
                hasFilename = true; // go ahead and call NEXTCHAR to eat the quote
            else if (!IsWhitespace(ch))
                break;

            NEXTCHAR (pStart, p, ch);
        }

        if (hasFilename)
        {
            PCWSTR  pszStart = p;

            // Copy the filename (up to MAX_PPTOKEN_LEN chars) into szBuf
            while (hasFilename)
            {
                PEEKCHAR (pStart, p, ch);
                if (ch == '\r' || ch == '\n' || ch == 0 || ch == UCH_PS || ch == UCH_LS)
                {
                    ErrorAtPosition(pCtx->m_iCurLine, pszStart - pCtx->m_pszCurLine, p - pszStart, ERR_NewlineInConst);
                    return;
                }

                if (ch == '\"')
                    hasFilename = false; // go ahead and call NEXTCHAR to eat the quote
                else if (pszFilename - szBuf < _MAX_PATH)
                    *pszFilename++ = ch;
                else if (isValidName)
                {
                    ErrorAtPosition(pCtx->m_iCurLine, pszStart - pCtx->m_pszCurLine, p - pszStart, ERR_FileNameTooLong);
                    isValidName = false;
                    // Keep parsing the string so we don't get bogus errors later.
                }

                NEXTCHAR (pStart, p, ch);
            }
            *pszFilename = '\0';

            // If we had an invalid number or filename end it here
            if (!isValidNum || !isValidName)
                return;

            // now make the filename relative (if it already isn't)
            if (m_pszSrcDir == NULL)
            {
                WCHAR w_NewDir[_MAX_PATH];
                PWSTR w_file;
                PCWSTR w_original;


                m_pSourceText->GetName (&w_original);
                if (!W_GetFullPathName (w_original, _MAX_PATH, w_NewDir, &w_file))
                {
                    w_NewDir[0] = L'\0';
                }
                else
                {
                    *w_file = L'\0';
                }
                m_pszSrcDir = m_pStdHeap->AllocStr(w_NewDir);
            }

            MakePath (m_pszSrcDir, szBuf, _MAX_PATH);

            NAME * newName = GetNameMgr()->AddString( szBuf);

            // Subtract 1 because lines are Zero based in the Lexer/Parser
            m_LineMap.AddMap(pCtx->m_iCurLine, newLine - 1, newName);
        }
        else if (isValidNum)
        {
            // Don't set a filename (when it is actually mapped, it will look upward)
            // Subtract 1 because lines are Zero based in the Lexer/Parser
            m_LineMap.AddMap(pCtx->m_iCurLine, newLine - 1, NULL);

            // Check for the EOL so we can give a better error message
            pszStart = NULL;
            pName = NULL;
            if (PPT_EOL != ScanPreprocessorToken (p, pszStart, &pName, false))
            {
                ErrorAtPosition(pCtx->m_iCurLine, pszStart - pCtx->m_pszCurLine, p - pszStart, ERR_MissingPPFile);

                // Skip over these, so we don't get multiple errors
                SkipRemainingPPTokens(p);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::SkipRemainingPPTokens

void CSourceModule::SkipRemainingPPTokens (PCWSTR &p)
{
    PCWSTR      pszStart;
    NAME        *pName;
    PPTOKENID   tok;

    do
        tok = ScanPreprocessorToken (p, pszStart, &pName, false);
    while (tok != PPT_EOL);
}


////////////////////////////////////////////////////////////////////////////////
// CSourceModule::EvalPreprocessorExpression

BOOL CSourceModule::EvalPreprocessorExpression (PCWSTR &p, CLexer *pCtx, PPTOKENID tokOpPrec)
{
    NAME        *pName;
    PCWSTR      pszStart;
    PPTOKENID   tok;
    BOOL        fTerm;

    // First, we need a term -- possibly prefixed with a ! operator
    tok = ScanPreprocessorToken (p, pszStart, &pName, false);
    switch (tok)
    {
        case PPT_NOT:
            fTerm = !EvalPreprocessorExpression (p, pCtx, tok);
            break;

        case PPT_OPENPAREN:
            fTerm = EvalPreprocessorExpression (p, pCtx);   // (default (lowest) precedence)
            if (ScanPreprocessorToken (p, pszStart, &pName, false) != PPT_CLOSEPAREN)
            {
                ErrorAtPosition (pCtx->m_iCurLine, pszStart - pCtx->m_pszCurLine, p - pszStart, ERR_CloseParenExpected);
                p = pszStart;
            }
            break;

        case PPT_TRUE:
        case PPT_FALSE:
            fTerm = (tok == PPT_TRUE);
            break;

        case PPT_IDENTIFIER:
            fTerm = m_tableDefines.IsDefined (pName);
            break;

        default:
            ErrorAtPosition (pCtx->m_iCurLine, pszStart - pCtx->m_pszCurLine, p - pszStart, ERR_InvalidPreprocExpr);
            SkipRemainingPPTokens (p);
            return TRUE;        // Consider the error as "#if true"
    }

    // Okay, now look for an operator.
    tok = ScanPreprocessorToken (p, pszStart, &pName, false);
    if (tok >= PPT_OR && tok <= PPT_NOTEQUAL)
    {
        // We'll only 'take' this operator if it is of equal or higher precedence
        // than that which we were given
        if (tok < tokOpPrec)
        {
            p = pszStart;       // Back up to the token again so we don't skip it
        }
        else
        {
            BOOL    fRHS = EvalPreprocessorExpression (p, pCtx, tok == PPT_NOTEQUAL ? PPT_EQUAL : tok);

            switch (tok)
            {
                case PPT_OR:        fTerm = fTerm || fRHS;          break;
                case PPT_AND:       fTerm = fTerm && fRHS;          break;
                case PPT_EQUAL:     fTerm = (!fTerm) == (!fRHS);    break;
                case PPT_NOTEQUAL:  fTerm = (!fTerm) != (!fRHS);    break;
                default:
                    VSFAIL ("Unrecognized preprocessor expression operator!");
            }
        }
    }
    else
    {
        // Not an operator, so put the token back!
        p = pszStart;
    }

    return fTerm;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::ScanExcludedCode

void CSourceModule::ScanExcludedCode (PCWSTR &p, CLexer *pCtx)
{
    WCHAR   ch;
    BOOL    fFirstNonWhite = FALSE;
    PCWSTR  pStart = p;

    // Simply skip everything until we see another '#' character as the first
    // non-white character of a line, or until we hit end of file.  If we DO
    // hit end-of-file first, issue an error
    while (TRUE)
    {
        PEEKCHAR (pStart, p, ch);
        if (ch == '\n' || ch == UCH_PS || ch == UCH_LS)
        {
            PCWSTR  pszTrack = (*p == ch) ? p + 1 : NULL;
            NEXTCHAR (pStart, p, ch);
            if (pszTrack)
            {
                TrackLine (pCtx, pszTrack); // Only track the line if it's a physical LF (non-escaped)
                fFirstNonWhite = TRUE;      // Also, preprocessor directives must be first on PHYSICAL line
            }
        }
        else if (ch == '\r')
        {
            PCWSTR  pszTrack = (*p == '\r') ? p + 1 : NULL;
            NEXTCHAR (pStart, p, ch);
            if (PEEKCHAR (pStart, p, ch) == '\n')
            {
                if (*p == '\n')
                    pszTrack = p + 1;
                NEXTCHAR (pStart, p, ch);
            }

            if (pszTrack != NULL)
            {
                TrackLine (pCtx, pszTrack); // Only track physical (non-escaped) CR/CRLF guys
                fFirstNonWhite = TRUE;
            }
        }
        else if (ch == 0)
        {
            if (*p == 0)
            {
                ErrorAtPosition (pCtx->m_iCurLine, p - pCtx->m_pszCurLine, 1, ERR_EndifDirectiveExpected);
                return;
            }
            ch = ' ';       // Consider \u0000 as a space...
        }

        if (fFirstNonWhite)
        {
            // Skip whitespace
            while (PEEKCHAR (pStart, p, ch) == ' ' || ch == '\t')
                NEXTCHAR (pStart, p, ch);

            // If this is a '#', we're done
            if (ch == '#')
                return;

            fFirstNonWhite = FALSE;
            ch = 0;
        }

        if (ch != 0)
            NEXTCHAR (pStart, p, ch);
    }
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::ScanAndIgnoreDirective

void CSourceModule::ScanAndIgnoreDirective (PPTOKENID tok, PCWSTR pszStart, PCWSTR &p, CLexer *pCtx)
{
    if (tok == PPT_IF || tok == PPT_ELIF)
    {
        EvalPreprocessorExpression (p, pCtx);
    }
    else if (tok == PPT_ERROR || tok == PPT_WARNING || tok == PPT_REGION || tok == PPT_LINE)
    {
        SkipRemainingPPTokens (p);
    }
    else if (tok == PPT_DEFINE || tok == PPT_UNDEF)
    {
        NAME    *pName;

        tok = ScanPreprocessorToken (p, pszStart, &pName, false);
        if (tok != PPT_IDENTIFIER)
        {
            ErrorAtPosition (pCtx->m_iCurLine, pszStart - pCtx->m_pszCurLine, p - pszStart, ERR_IdentifierExpected);
            SkipRemainingPPTokens (p);
        }
    }
    else if (tok != PPT_ELSE && tok != PPT_ENDIF && tok != PPT_ENDREGION)
    {
        ErrorAtPosition (pCtx->m_iCurLine, pszStart - pCtx->m_pszCurLine, p - pszStart, ERR_PPDirectiveExpected);
        SkipRemainingPPTokens (p);
    }

    // Make sure the line ends cleanly
    ScanPreprocessorLineTerminator (pCtx, p);
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::PushCCRecord

void CSourceModule::PushCCRecord (CCREC * & pStack, DWORD dwFlags)
{
    CCREC   *pNew = (CCREC *)VSAlloc (sizeof (CCREC));
    pNew->pPrev = pStack;
    pStack = pNew;
    pNew->dwFlags = dwFlags;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::PopCCRecord

BOOL CSourceModule::PopCCRecord (CCREC * &pStack)
{
    if (pStack != NULL)
    {
        CCREC   *pTmp = pStack;
        pStack = pStack->pPrev;
        VSFree (pTmp);
        return TRUE;
    }

    return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::CompareCCStacks

BOOL CSourceModule::CompareCCStacks (CCREC *pOne, CCREC *pTwo)
{
    if (pOne == pTwo)
        return TRUE;

    if (pOne == NULL || pTwo == NULL)
        return FALSE;

    return (pOne->dwFlags == pTwo->dwFlags) && CompareCCStacks (pOne->pPrev, pTwo->pPrev);
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::MarkTransition

void CSourceModule::MarkTransition (long iLine)
{
    /* NOTE:  This was taken out for colorization purposes...

    // Optimization:  Two consecutive transitions cancel each other out...
    if (m_lex.iTransitionLines > 0 && ((m_lex.piTransitionLines[m_lex.iTransitionLines - 1]) == (iLine - 1)))
    {
        m_lex.iTransitionLines--;
        return;
    }
    */

    // Make sure there's room
    if ((m_lex.iTransitionLines & 7) == 0)
    {
        long    iSize = (m_lex.iTransitionLines + 8) * sizeof (long);
        void    *pNew = (m_lex.piTransitionLines == NULL) ? VSAlloc (iSize) : VSRealloc (m_lex.piTransitionLines, iSize);

        if (pNew != NULL)
            m_lex.piTransitionLines = (long *)pNew;
    }

    ASSERT (m_lex.piTransitionLines[m_lex.iTransitionLines] != *(long *)"END!");
    m_lex.piTransitionLines[m_lex.iTransitionLines++] = iLine;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::MarkCCStateChange

void CSourceModule::MarkCCStateChange (long iLine, DWORD dwFlags)
{
    // Make sure there's room
    if ((m_lex.iCCStates & 7) == 0)
    {
        long    iSize = (m_lex.iCCStates + 8) * sizeof (long);
        void    *pNew = (m_lex.pCCStates == NULL) ? VSAlloc (iSize) : VSRealloc (m_lex.pCCStates, iSize);

        if (pNew != NULL)
            m_lex.pCCStates = (CCSTATE *)pNew;
    }

    m_lex.pCCStates[m_lex.iCCStates].dwFlags = dwFlags;
    m_lex.pCCStates[m_lex.iCCStates++].iLine = iLine;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::AddRef

STDMETHODIMP_(ULONG) CSourceModule::AddRef ()
{
    return InterlockedIncrement ((LONG *)&m_iRef);
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::Release

STDMETHODIMP_(ULONG) CSourceModule::Release ()
{
    ULONG   iNew = InterlockedDecrement ((LONG *)&m_iRef);
    DWORD   dwExceptionCode = 0;
    if (iNew == 0)
    {
        BOOL    fLocked = m_StateLock.Acquire ();

        if (VSFSWITCH (gfUnhandledExceptions))
        {
            delete this;

            // Because of the above, this object is unlocked in the dtor...
            fLocked = FALSE;
        }
        else
        {
            PAL_TRY
            {
                delete this;

                // Because of the above, this object is unlocked in the dtor...
                fLocked = FALSE;
            }
            EXCEPT_FILTER_EX(PAL_Except1, &dwExceptionCode)
            {
                m_pController->HandleException (dwExceptionCode);
            }
            PAL_ENDTRY
        }

        if (fLocked)
        {
            ASSERT (m_StateLock.LockedByMe());
            m_StateLock.Release();
        }
    }

    return iNew;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::QueryInterface

STDMETHODIMP CSourceModule::QueryInterface (REFIID riid, void **ppObj)
{
    *ppObj = NULL;

    if (riid == IID_IUnknown || riid == IID_ICSSourceModule)
    {
        *ppObj = (ICSSourceModule *)this;
        ((ICSSourceModule *)this)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::GetSourceData

STDMETHODIMP CSourceModule::GetSourceData (BOOL fBlockForNewest, ICSSourceData **ppData)
{
    CTinyGate   gate (&m_StateLock);
    BOOL        fNewest = (m_iDataRef == 0 || !m_fTextModified);
    BOOL        fCheckThisThread = TRUE;

    while (fBlockForNewest && m_iDataRef > 0 && m_fTextModified)
    {
        // We must wait until all outstanding source data objects are released.
        if (fCheckThisThread)
        {
            long    iThreadIdx = FindThreadIndex (FALSE);

            // We must also check first to see if the calling thread holds such an
            // object, and if so, fail.
            if (m_pThreadDataRef[iThreadIdx].iRef != 0)
            {
                return E_FAIL;
            }
            fCheckThisThread = FALSE;
        }

        // Okay, we know two things:  There is at least one outstanding data
        // ref, and we don't have it.  We need to unlock, wait until the data ref
        // count goes to zero, and lock again.                                     
        m_StateLock.Release();
        BOOL    fFirst = TRUE;
        while (m_iDataRef > 0)
        {
            if (fFirst)
            {
                Sleep (0);
                fFirst = FALSE;
            }
            else
            {
                Snooze ();
            }
        }
        m_StateLock.Acquire();
        fNewest = TRUE;
    }

    HRESULT     hr = CSourceData::CreateInstance (this, ppData);
    return SUCCEEDED (hr) && !fNewest ? S_FALSE : hr;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::GetSourceText

STDMETHODIMP CSourceModule::GetSourceText (ICSSourceText **ppSrcText)
{
    *ppSrcText = m_pSourceText;
    (*ppSrcText)->AddRef();
    return S_OK;
}


////////////////////////////////////////////////////////////////////////////////
// CSourceModule::Flush

STDMETHODIMP CSourceModule::Flush ()
{
    CTinyGate   gate (&m_StateLock);

    if (m_iDataRef > 0)
    {
        VSFAIL ("Can't flush source module with outstanding data refs!");
        return E_FAIL;
    }

    InternalFlush ();
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::InternalFlush

void CSourceModule::InternalFlush ()
{
    ASSERT (m_StateLock.LockedByMe());

    if (m_lex.piLines != NULL)
    {
        m_pStdHeap->Free (m_lex.piLines);
        m_lex.piLines = NULL;
    }

    if (m_lex.pTokens != NULL)
    {
        m_pStdHeap->Free (m_lex.pTokens);
        m_lex.pTokens = NULL;
    }

    if (m_lex.pposTokens != NULL)
    {
        m_pStdHeap->Free (m_lex.pposTokens);
        m_lex.pposTokens = NULL;
    }

    if (m_lex.pposComments != NULL)
    {
        m_pStdHeap->Free (m_lex.pposComments);
        m_lex.pposComments = NULL;
    }

    if (m_lex.piRegionStart != NULL)
    {
        m_pStdHeap->Free (m_lex.piRegionStart);
        m_lex.piRegionStart = NULL;
    }

    m_LineMap.Clear();

    m_lex.iTokens = 0;

    m_heap.FreeHeap ();
    m_pActiveHeap = &m_heap;
    m_pInterior = NULL;
    m_pTreeTop = NULL;
    m_fParsedTop = FALSE;
    m_fTokenized = FALSE;
}


////////////////////////////////////////////////////////////////////////////////
// CSourceModule::GetMemoryConsumption

STDMETHODIMP CSourceModule::GetMemoryConsumption (long *piTopTree, long *piInteriorTrees, long *piInteriorNodes)
{
    // Calculate bytes consumed by top-level parse tree (assumes no interior tree exists)
    *piTopTree = (long)m_heap.CalcCommittedSize();

    // Return size consumed by interior nodes (assumes ParseForErrors() was called first)
    *piInteriorTrees = (long)m_dwInteriorSize;

    // Return number of interior trees parsed
    *piInteriorNodes = m_iInteriorTrees;
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::ParseTopLevel

HRESULT CSourceModule::ParseTopLevel (CSourceData *pData, BASENODE **ppNode)
{
    DWORD dwExceptionCode;
    TimerStart(TIME_PARSETOPLEVEL);

    // Make sure the token stream is up-to-date...
    if (VSFSWITCH (gfUnhandledExceptions))
    {
        CreateTokenStream ();
    }
    else
    {
        SRCMOD_EXCEPTION_TRYEX
        {
            CreateTokenStream ();
        }
        SRCMOD_EXCEPTION_EXCEPT_FILTER_EX(PAL_Except1, &dwExceptionCode)
        {
            m_pController->HandleException (dwExceptionCode);
            TimerStop(TIME_PARSETOPLEVEL);
            VSFAIL ("YIPES!  CRASH DURING TOKENIZATION!");
            ClearTokenizerThread();
            m_fTokenizing = FALSE;
            m_fTokenized = FALSE;
            return E_FAIL;
        }
        SRCMOD_EXCEPTION_ENDTRY
    }

    // Now, check the state to see if we need to do any parsing work.
    BOOL    fLocked = m_StateLock.Acquire();
    HRESULT hr = S_OK;

    // ParseTopLevel cannot be called while state is locked!
    ASSERT (fLocked);

    m_pParser->SetInputData (m_lex.pTokens, m_lex.pposTokens, m_lex.iTokens, m_lex.pszSource, m_lex.piLines, m_lex.iLines);

    // Check for incremental parsing possibility.  This is only possible if
    // we actually HAVE a parsed tree, and m_fTokensChanged, and the range of
    // changed tokens 1) falls entirely within an interior node, and 2) contains
    // no TID_OPENCURLY or TID_CLOSECURLY tokens.
    if (m_fParsedTop && m_fTokensChanged)
    {
        BOOL        fMustReparse = m_fForceTopParse;// || m_fExtraOpenCurly || m_fExtraCloseCurly;
        BASENODE    *pLeaf = NULL;

        ASSERT (m_pTreeTop != NULL);

        if (!fMustReparse)
        {
            // Look at all the tokens from m_iTokDeltaFirst to m_iTokDeltaLast.
            // If there are any TID_*CURLY tokens, we must reparse the top.
            for (long i=m_iTokDeltaFirst; i<= m_iTokDeltaLast; i++)
                if (m_lex.pTokens[i] == TID_OPENCURLY || m_lex.pTokens[i] == TID_CLOSECURLY)
                {
                    DBGOUT (("PARSING TOP LEVEL: Token delta contains an open/close curly"));
                    fMustReparse = TRUE;
                    break;
                }
        }
        else
        {
            DBGOUT (("PARSING TOP LEVEL:  Tokenization wasn't properly incremental, or rescanned an open/close curly"));
        }

        if (!fMustReparse)
        {
            // Find the leaf node (WITHOUT parsing interiors) that contains the
            // first token, and see if it is 1) an interior node that 2) also
            // contains the last token.
            pLeaf = m_pParser->FindLeaf (m_lex.pposTokens[m_iTokDeltaFirst], m_pTreeTop, NULL, NULL);
            if (pLeaf != NULL && pLeaf->InGroup (NG_INTERIOR))
            {
                if (pLeaf->kind == NK_ACCESSOR)
                    fMustReparse = (m_iTokDeltaLast >= pLeaf->asACCESSOR()->iClose + m_iTokDelta) || (m_iTokDeltaFirst <= pLeaf->asACCESSOR()->iOpen);
                else
                    fMustReparse = (m_iTokDeltaLast >= pLeaf->asANYMETHOD()->iClose + m_iTokDelta) || (m_iTokDeltaFirst <= pLeaf->asANYMETHOD()->iOpen);

                if (fMustReparse)
                {
                    DBGOUT (("PARSING TOP LEVEL: Token delta begins prior to open or extends beyond close of interior node"));
                }

                pLeaf->other &= ~NFEX_INTERIOR_PARSED;        // Flip this off -- the guts of the method are different!
            }
            else
            {
                DBGOUT (("PARSING TOP LEVEL: Token delta begins outside of interior node"));
                fMustReparse = TRUE;
            }
        }

        if (fMustReparse)
        {
            // Unfortunately we must reparse the top level now.  Blast the old tree
            // first, and reset ourselves so the below code will execute.
            m_heap.FreeHeap ();
            m_pActiveHeap = &m_heap;
            m_pInterior = NULL;
            m_pTreeTop = NULL;
            m_fParsedTop = FALSE;
        }
        else
        {
            DBGOUT (("KEEPING TOP-LEVEL PARSE TREE!!!!"));

            // The tree is okay -- we must might need to update token indexes in the nodes
            // from "here down".
            ASSERT (pLeaf != NULL);
            if (m_iTokDelta != 0)
            {
                // From "here down" means we need to pass a child to UpdateTokenIndexes.
                // Just pass the leaf again -- it doesn't have to be a real child of pLeaf.
                STARTTIMER ("UpdateTokenIndexes");
                UpdateTokenIndexes (pLeaf, pLeaf);
                STOPTIMER ();
            }
        }
    }

    m_fTokensChanged = FALSE;
    m_iTokDelta = 0;
    m_iTokDeltaLast = 0;
    m_iTokDeltaFirst = m_lex.iTokens;

    if (!m_fParsedTop && !m_fParsingTop)
    {
        // It's not done, so we need to do it.
        m_fParsingTop = TRUE;
        SetParserThread ();

        PAL_TRY
        {
            m_pActiveHeap = &m_heap;
            if (m_pNodeTable)
            {
                m_pNodeTable->RemoveAll();
                m_pNodeArray->ClearAll();
            }

            CErrorContainer *pNew;
            if (FAILED (hr = CErrorContainer::CreateInstance (EC_TOPLEVELPARSE, 0, &pNew)))
            {
                VSFAIL("Out of memory");
            }

            m_pParseErrors->Release();
            m_pParseErrors = pNew;
            ASSERT (m_pCurrentErrors == NULL);
            m_pCurrentErrors = m_pParseErrors;

            // Release state lock while we parse in case another thread
            // wants the token stream (for example)
            m_StateLock.Release();

            if (VSFSWITCH (gfUnhandledExceptions))
            {
                m_pTreeTop = m_pParser->ParseSourceModule ();

                // Mark the allocator -- back to here is where we rewind when
                // parsing a new interior node
                m_heap.Mark (&m_markInterior);
            }
            else
            {
                SRCMOD_EXCEPTION_TRYEX
                {
                    // Must build parse tree
                    m_pTreeTop = m_pParser->ParseSourceModule ();

                    // Mark the allocator -- back to here is where we rewind when
                    // parsing a new interior node
                    m_heap.Mark (&m_markInterior);
                }
                SRCMOD_EXCEPTION_EXCEPT_FILTER_EX(PAL_Except2, &dwExceptionCode)
                {
                    m_pController->HandleException (dwExceptionCode);
                    hr = E_FAIL;
                }
                SRCMOD_EXCEPTION_ENDTRY
            }


            // We're done parsing the top level, so reflect this in the state
            fLocked = m_StateLock.Acquire();
            ASSERT (fLocked);
            m_pCurrentErrors = NULL;
        }
        PAL_FINALLY
        {
            ClearParserThread ();
            m_fParsingTop = FALSE;
            m_fParsedTop = SUCCEEDED (hr);
        }
        PAL_ENDTRY
    }
    else if (m_fParsingTop)
    {
        // Someone else is currently parsing the top, so wait until they are done.
        ASSERT (m_dwParserThread != GetCurrentThreadId());
        while (!m_fParsedTop)
        {
            m_StateLock.Release();
            Snooze ();
            m_StateLock.Acquire();
        }
    }

    // We're done with the state lock
    m_StateLock.Release();

    *ppNode = m_pTreeTop;

    TimerStop(TIME_PARSETOPLEVEL);
    return (*ppNode == NULL) ? E_FAIL : S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::GetErrors

HRESULT CSourceModule::GetErrors (CSourceData *pData, ERRORCATEGORY iCategory, ICSErrorContainer **ppErrors)
{
    CTinyGate   gate (&m_StateLock);

    if (iCategory == EC_TOKENIZATION)
    {
        *ppErrors = m_pTokenErrors;
    }
    else if (iCategory == EC_TOPLEVELPARSE)
    {
        *ppErrors = m_pParseErrors;
    }
    else
    {
        return E_INVALIDARG;
    }

    (*ppErrors)->AddRef();
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::UpdateTokenIndexes
//
// NOTE:  To update the token positions of JUST pNode, do NOT pass a child
// pointer.  Otherwise, this update only subnodes of pNode that are "beyond"
// pChild, and also any such nodes in pNode's parent.

//#define UPDTOK(i) if (i + m_iTokDelta > m_iTokDeltaLast) { i += m_iTokDelta; DBGOUT (("Updated tok index for %s (%08X) from %d to %d", #i, &i, i - m_iTokDelta, i)); } else { ASSERT (i < m_iTokDeltaFirst); }
#define UPDTOK(i) if (i + m_iTokDelta > m_iTokDeltaLast) { i += m_iTokDelta; } else { ASSERT (i < m_iTokDeltaFirst); }

void CSourceModule::UpdateTokenIndexes (BASENODE *pNode, BASENODE *pChild)
{
    if (pNode == NULL)
        return;

    if (pNode->kind != NK_NAME)
    {
        // Update this node (unless it's a name node -- special consideration for it below)
        UPDTOK (pNode->tokidx);
    }

    // Update appropriate children
    switch (pNode->kind)
    {
        case NK_NAMESPACE:
            if (pChild != pNode->asNAMESPACE()->pName)      UpdateTokenIndexes (pNode->asNAMESPACE()->pName, NULL);
            if (pChild != pNode->asNAMESPACE()->pUsing)     UpdateTokenIndexes (pNode->asNAMESPACE()->pUsing, NULL);
            if (pChild != pNode->asNAMESPACE()->pElements)  UpdateTokenIndexes (pNode->asNAMESPACE()->pElements, NULL);
            UPDTOK (pNode->asNAMESPACE()->iClose);
            break;

        case NK_USING:
            if (pChild != pNode->asUSING()->pName)          UpdateTokenIndexes (pNode->asUSING()->pName, NULL);
            if (pChild != pNode->asUSING()->pAlias)         UpdateTokenIndexes (pNode->asUSING()->pAlias, NULL);
            break;

        case NK_NAME:
            if (pNode->flags & NF_NAME_MISSING)
            {
                // The token index of the missing name node is one prior to where it
                // was supposed to be.  Thus, it may be == m_iTokDeltaFirst.
                if (pNode->tokidx + m_iTokDelta > m_iTokDeltaLast)
                    pNode->tokidx += m_iTokDelta;
            }
            break;

        case NK_OP:
        case NK_CONSTVAL:
            break;

        case NK_CLASS:
        case NK_ENUM:
        case NK_INTERFACE:
        case NK_STRUCT:
        {
            CLASSNODE   *pClass = pNode->asAGGREGATE();

            if (pChild != pClass->pAttr)                    UpdateTokenIndexes (pClass->pAttr, NULL);
            if (pChild != pClass->pName)                    UpdateTokenIndexes (pClass->pName, NULL);
            if (pChild != pClass->pBases)                   UpdateTokenIndexes (pClass->pBases, NULL);

            MEMBERNODE  *pMbr = pClass->pMembers;

            if (pChild != NULL)
            {
                // Run the list of members to see if we find pChild
                for (pMbr = pClass->pMembers; pMbr != NULL && pMbr != pChild; pMbr = pMbr->pNext)
                    ;

                // If we didn't find it, we need to update them all.  If we did, we DON'T want to
                // update it again.
                if (pMbr == NULL)
                    pMbr = pClass->pMembers;
                else
                    pMbr = pMbr->pNext;
            }

            for ( ; pMbr != NULL; pMbr = pMbr->pNext)
                UpdateTokenIndexes (pMbr, NULL);

            UPDTOK (pClass->iClose);
            break;
        }

        case NK_DELEGATE:
        {
            DELEGATENODE    *pDel = pNode->asDELEGATE();

            if (pChild != pDel->pAttr)                      UpdateTokenIndexes (pDel->pAttr, NULL);
            if (pChild != pDel->pType)                      UpdateTokenIndexes (pDel->pType, NULL);
            if (pChild != pDel->pName)                      UpdateTokenIndexes (pDel->pName, NULL);
            if (pChild != pDel->pParms)                     UpdateTokenIndexes (pDel->pParms, NULL);
            UPDTOK (pDel->iSemi);
            break;
        }

        case NK_ENUMMBR:
            if (pChild != pNode->asENUMMBR()->pName)        UpdateTokenIndexes (pNode->asENUMMBR()->pName, NULL);
            if (pChild != pNode->asENUMMBR()->pValue)       UpdateTokenIndexes (pNode->asENUMMBR()->pValue, NULL);
            break;

        case NK_CONST:
        case NK_FIELD:
        {
            FIELDNODE   *pField = (FIELDNODE *)pNode;

            if (pChild != pField->pAttr)                    UpdateTokenIndexes (pField->pAttr, NULL);
            if (pChild != pField->pType)                    UpdateTokenIndexes (pField->pType, NULL);
            if (pChild != pField->pDecls)                   UpdateTokenIndexes (pField->pDecls, NULL);
            UPDTOK (pField->iClose);
            break;
        }

        case NK_METHOD:
        case NK_CTOR:
        case NK_DTOR:
        case NK_OPERATOR:
        {
            METHODNODE  *pMethod = pNode->asANYMETHOD();

            if (pChild != pMethod->pAttr)                                       UpdateTokenIndexes (pMethod->pAttr, NULL);
            if (pMethod->kind == NK_METHOD && pChild != pMethod->pName)         UpdateTokenIndexes (pMethod->pName, NULL);
            else if (pMethod->kind == NK_CTOR && pChild != pMethod->pCtorArgs)  UpdateTokenIndexes (pMethod->pCtorArgs, NULL);
            if (pChild != pMethod->pParms)                                      UpdateTokenIndexes (pMethod->pParms, NULL);

            UPDTOK (pMethod->iOpen);
            UPDTOK (pMethod->iClose);


            // Can't have an interior parse tree and update the token stream at the same time!
            ASSERT (pMethod->pBody == NULL);
            break;
        }

        case NK_PROPERTY:
        case NK_INDEXER:
        {
            PROPERTYNODE    *pProp = pNode->asANYPROPERTY();

            if (pChild != pProp->pAttr)                     UpdateTokenIndexes (pProp->pAttr, NULL);
            if (pChild != pProp->pName)                     UpdateTokenIndexes (pProp->pName, NULL);
            if (pChild != pProp->pType)                     UpdateTokenIndexes (pProp->pType, NULL);
            if (pChild != pProp->pParms)                    UpdateTokenIndexes (pProp->pParms, NULL);
            if (pChild != pProp->pGet)                      UpdateTokenIndexes (pProp->pGet, NULL);
            if (pChild != pProp->pSet)                      UpdateTokenIndexes (pProp->pSet, NULL);
            UPDTOK (pProp->iClose);
            break;
        }

        case NK_ACCESSOR:
        {
            ACCESSORNODE    *pAcc = pNode->asACCESSOR();

            UPDTOK (pAcc->iOpen);
            UPDTOK (pAcc->iClose);
            ASSERT (pAcc->pBody == NULL);
            break;
        }

        case NK_PARAMETER:
        {
            PARAMETERNODE   *pParm = pNode->asPARAMETER();

            if (pChild != pParm->pAttr)                     UpdateTokenIndexes (pParm->pAttr, NULL);
            if (pChild != pParm->pType)                     UpdateTokenIndexes (pParm->pType, NULL);
            if (pChild != pParm->pName)                     UpdateTokenIndexes (pParm->pName, NULL);
            break;
        }

        case NK_NESTEDTYPE:
        {
            NESTEDTYPENODE  *pNested = pNode->asNESTEDTYPE();

            if (pChild != pNested->pAttr)                   UpdateTokenIndexes (pNested->pAttr, NULL);
            if (pChild != pNested->pType)                   UpdateTokenIndexes (pNested->pType, NULL);
            UPDTOK (pNested->iClose);
            break;
        }

        case NK_PARTIALMEMBER:
        {
            PARTIALMEMBERNODE   *p = pNode->asPARTIALMEMBER();

            if (pChild != p->pAttr)                         UpdateTokenIndexes (p->pAttr, NULL);
            if (pChild != p->pNode)                         UpdateTokenIndexes (p->pNode, NULL);
            UPDTOK (p->iClose);
            break;
        }

        case NK_TYPE:
        {
            TYPENODE    *pType = pNode->asTYPE();

            if (pType->TypeKind() == TK_NAMED && pChild != pType->pName)
                UpdateTokenIndexes (pType->pName, NULL);
            else if (pType->TypeKind() != TK_PREDEFINED && pChild != pType->pElementType)
                UpdateTokenIndexes (pType->pElementType, NULL);
            break;
        }

        case NK_ARRAYINIT:
        case NK_UNOP:
        {
            UNOPNODE    *pOp = (UNOPNODE *)pNode;

            if (pChild != pOp->p1)                          UpdateTokenIndexes (pOp->p1, NULL);
            break;
        }

        case NK_ARROW:
        case NK_BINOP:
        case NK_DOT:
        {
            BINOPNODE   *pOp = (BINOPNODE *)pNode;

            if (pChild != pOp->p1)                          UpdateTokenIndexes (pOp->p1, NULL);
            if (pChild != pOp->p2)                          UpdateTokenIndexes (pOp->p2, NULL);

            if (pOp->kind == NK_BINOP && pOp->Op() == OP_CALL)
            {
                CALLNODE    *pCall = (CALLNODE *)pOp;
                UPDTOK (pCall->iClose);
            }

            break;
        }

        case NK_LIST:
        {
            BINOPNODE   *pOp = (BINOPNODE *)pNode;

            // Slightly different than other ops, because this one contains namespace members
            // (types and namespaces) that we do NOT want to unnecessarily update.  Only update
            // the left-hand subtree if our child is NULL.
            if (pChild == NULL)                             UpdateTokenIndexes (pOp->p1, NULL);
            if (pChild != pOp->p2)                          UpdateTokenIndexes (pOp->p2, NULL);
            break;
        }

        case NK_VARDECL:
        {
            VARDECLNODE *pVar = pNode->asVARDECL();

            if (pChild != pVar->pName)                      UpdateTokenIndexes (pVar->pName, NULL);
            if (pChild != pVar->pArg)                       UpdateTokenIndexes (pVar->pArg, NULL);
            break;
        }

        case NK_NEW:
        {
            NEWNODE     *pNew = pNode->asNEW();

            if (pChild != pNew->pType)                      UpdateTokenIndexes (pNew->pType, NULL);
            if (pChild != pNew->pArgs)                      UpdateTokenIndexes (pNew->pArgs, NULL);
            if (pChild != pNew->pInit)                      UpdateTokenIndexes (pNew->pInit, NULL);
            break;
        }

        case NK_ATTR:
        case NK_ATTRARG:
        {
            ATTRNODE    *pAttr = (ATTRNODE *)pNode;

            if (pChild != pAttr->pName)                     UpdateTokenIndexes (pAttr->pName, NULL);
            if (pChild != pAttr->pArgs)                     UpdateTokenIndexes (pAttr->pArgs, NULL);
            break;
        }

        case NK_ATTRDECL:
        {
            ATTRDECLNODE    *pAttrDecl = pNode->asATTRDECL();

            if (pChild != pAttrDecl->pAttr)                 UpdateTokenIndexes (pAttrDecl->pAttr, NULL);
            UPDTOK (pAttrDecl->iClose);
            break;
        }

        default:
            VSFAIL ("What's this kind of node doing in the top-level tree?!?");
            return;
    }

    // If we were given a child, update our parent
    if (pChild != NULL)
        UpdateTokenIndexes (pNode->pParent, pNode);
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::GetInteriorParseTree

HRESULT CSourceModule::GetInteriorParseTree (CSourceData *pData, BASENODE *pNode, ICSInteriorTree **ppTree)
{
    HRESULT     hr = S_OK;
    DWORD       dwExceptionCode;
    BOOL        fLocked = m_StateLock.Acquire();

    ASSERT (fLocked);       // Can't call this when the state is locked!

    // Can't parse interior node without having already done the top level!
    if (!m_fParsedTop)
    {
        if (fLocked)
            m_StateLock.Release();
        return E_INVALIDARG;
    }

    // We're done with the state lock for now
    if (fLocked)
        m_StateLock.Release();

    if (VSFSWITCH (gfUnhandledExceptions))
    {
        // Create an interior tree object.  It will call back into us to do
        // the parse (if necessary; or grab the existing tree node if already
        // there).
        hr = CInteriorTree::CreateInstance (pData, pNode, ppTree);
    }
    else
    {
        SRCMOD_EXCEPTION_TRYEX
        {
            // Create an interior tree object.  It will call back into us to do
            // the parse (if necessary; or grab the existing tree node if already
            // there).
            hr = CInteriorTree::CreateInstance (pData, pNode, ppTree);
        }
        SRCMOD_EXCEPTION_EXCEPT_FILTER_EX (PAL_Except1, &dwExceptionCode)
        {
            m_pController->HandleException (dwExceptionCode);
            hr = E_FAIL;
        }
        SRCMOD_EXCEPTION_ENDTRY
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::GetInteriorNode

HRESULT CSourceModule::GetInteriorNode (CSourceData *pData, BASENODE *pNode, CInteriorNode **ppNode)
{
    // Re-obtain the state lock
#ifdef DEBUG
    BOOL    fLocked =
#endif
    m_StateLock.Acquire ();
    DWORD   dwExceptionCode;

    ASSERT (fLocked);       // Can't call this while state lock is held
    ASSERT (m_fParsedTop);  // Must have already parsed the top (still)

    // Wait until no other thread is parsing an interior node
    while (m_fParsingInterior)
    {
        ASSERT (m_dwInteriorParserThread != GetCurrentThreadId());
        m_StateLock.Release();
        Sleep (1);
        m_StateLock.Acquire();
    }

    CInteriorNode   **ppStoredNode;

    // Determine the location of the interior node pointer in the given
    // container node.
    switch (pNode->kind)
    {
        case NK_METHOD:
        case NK_CTOR:
        case NK_DTOR:
        case NK_OPERATOR:
            ppStoredNode = &(((METHODNODE *)pNode)->pInteriorNode);
            break;
        case NK_ACCESSOR:
            ppStoredNode = &(pNode->asACCESSOR()->pInteriorNode);
            break;
        default:
            VSFAIL ("Bogus node type passed to CSourceModule::GetInteriorNode()!");
            m_StateLock.Release();
            return E_FAIL;
    }

    ASSERT (pNode->InGroup (NG_INTERIOR));

    HRESULT     hr = S_OK;

    // Has this node been parsed already?  (If so, it will have an interior
    // node object...)
    if (*ppStoredNode != NULL)
    {
        // The node has been parsed.  Before releasing the state lock, addref the
        // interior node to protect it from final release below...
        *ppNode = *ppStoredNode;
        (*ppNode)->AddRef();

        // Check to see if it has been marked as being parsed for errors.
        // If not, then we need to fire the event now (obviously the node has been
        // parsed, it just hasn't had errors reported yet.
        if ((pNode->other & NFEX_INTERIOR_PARSED) == 0)
        {
            // Mark the node as being parsed first, while the state is still 
            // locked.
            pNode->other |= NFEX_INTERIOR_PARSED;

            // Release the state lock before reporting the errors.   The errors
            // are safe because the interior node already has our ref.
            m_StateLock.Release();
        }
        else
        {
            m_StateLock.Release();
        }

        return S_OK;
    }

    // No, so we need to create one.  Mark ourselves as parsing an
    // interior node in the state bits
    m_fParsingInterior = TRUE;
    SetInteriorParserThread();

    // If no interior node is currently using the built-in heap, we'll
    // use that.  Otherwise, we need the new interior node to use its own heap.
    if (m_fInteriorHeapBusy)
    {
        CSecondaryInteriorNode  *pIntNode = CSecondaryInteriorNode::CreateInstance (this, m_pController, pNode);
        *ppStoredNode = pIntNode;
        m_pActiveHeap = pIntNode->GetAllocationHeap();
        DBGOUT (("SECONDARY INTERIOR NODE CREATED!"));
    }
    else
    {
        *ppStoredNode = CPrimaryInteriorNode::CreateInstance (this, pNode);
        m_fInteriorHeapBusy = TRUE;
        m_pActiveHeap = &m_heap;
        m_heap.Free (&m_markInterior);
    }

    DWORD   dwSizeBase = 0;
    if (m_fAccumulateSize)
        dwSizeBase = (DWORD)m_heap.CalcCommittedSize();

    // Have the interior node create an error container for itself
    ASSERT (m_pCurrentErrors == NULL);
    m_pCurrentErrors = (*ppStoredNode)->CreateErrorContainer();

    // Do the interior parse, releasing the state lock first
    m_StateLock.Release();
    if (VSFSWITCH (gfUnhandledExceptions))
    {
        ParseInteriorNode (pNode);
        if (m_fParsingForErrors)
            pNode->other |= NFEX_INTERIOR_PARSED;
    }
    else
    {
        SRCMOD_EXCEPTION_TRYEX
        {
            ParseInteriorNode (pNode);
            if (m_fParsingForErrors)
                pNode->other |= NFEX_INTERIOR_PARSED;
        }
        SRCMOD_EXCEPTION_EXCEPT_FILTER_EX(PAL_Except1, &dwExceptionCode)
        {
            m_pController->HandleException (dwExceptionCode);
            hr = E_FAIL;
        }
        SRCMOD_EXCEPTION_ENDTRY
    }


    m_StateLock.Acquire();
    m_pCurrentErrors = NULL;
    m_pActiveHeap = NULL;

    m_fParsingInterior = FALSE;
    ClearInteriorParserThread();

    if (m_fAccumulateSize)
    {
        m_dwInteriorSize += (DWORD)(m_heap.CalcCommittedSize() - dwSizeBase);
        m_iInteriorTrees++;
    }

    // NOTE:  The state lock is still acquired at this point.  The state lock
    // is used for serialization to the interior nodes' ref counts.
    *ppNode = *ppStoredNode;
    (*ppNode)->AddRef();
    m_StateLock.Release();
    return hr;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::LookupNode

HRESULT CSourceModule::LookupNode (CSourceData *pData, NAME *pKey, long iOrdinal, BASENODE **ppNode, long *piGlobalOrdinal)
{
    CTinyGate   gate (&m_StateLock);

    if (m_pNodeTable == NULL)
        return E_FAIL;      // Not available -- must create compiler with CCF_KEEPNODETABLES

    // First, the easier check -- global-orginal value.
    if (pKey == NULL)
    {
        if (iOrdinal < 0 || iOrdinal >= m_pNodeArray->Count())
            return E_INVALIDARG;

        if (piGlobalOrdinal != NULL)
            *piGlobalOrdinal = iOrdinal;        // Well, duh...

        *ppNode = (*m_pNodeArray)[iOrdinal].pNode;
        return S_OK;
    }

    // Okay, we have a key.  Find a node chain in the table.
    NODECHAIN   *pChain = m_pNodeTable->Find (pKey);

    // Pay respect to the key-ordinal
    while (pChain != NULL && iOrdinal--)
    {
        if (pChain != NULL)
            pChain = pChain->pNext;
    }

    if (pChain != NULL)
    {
        *ppNode = (*m_pNodeArray)[pChain->iGlobalOrdinal].pNode;
        if (piGlobalOrdinal != NULL)
            *piGlobalOrdinal = pChain->iGlobalOrdinal;
        return S_OK;
    }

    // Node not found...
    return E_FAIL;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::GetNodeKeyOrdinal

HRESULT CSourceModule::GetNodeKeyOrdinal (CSourceData *pData, BASENODE *pNode, NAME **ppKey, long *piKeyOrdinal)
{
    CTinyGate   gate (&m_StateLock);

    if (m_pNodeTable == NULL)
        return E_FAIL;      // Not available -- must create compiler with CCF_KEEPNODETABLES

    NAME    *pKey;
    HRESULT hr;

    // First, build the node key and look up the node chain in the table
    if (FAILED (hr = pNode->BuildKey (GetNameMgr(), TRUE, &pKey)))
        return hr;

    long        iKeyOrd = 0;

    // Find the given node in the chain
    NODECHAIN *pChain;
    for (pChain = m_pNodeTable->Find (pKey); pChain != NULL; pChain = pChain->pNext)
    {
        if (pNode == (*m_pNodeArray)[pChain->iGlobalOrdinal].pNode)
            break;
        iKeyOrd++;
    }

    if (pChain == NULL)
        return E_FAIL;      // Huh?  Maybe not a keyed node, but then BuildKey would have failed!

    if (piKeyOrdinal != NULL)
        *piKeyOrdinal = iKeyOrd;
    if (ppKey != NULL)
        *ppKey = pKey;

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::GetGlobalKeyArray

HRESULT CSourceModule::GetGlobalKeyArray (CSourceData *pData, KEYEDNODE *pKeyedNodes, long iSize, long *piCopied)
{
    CTinyGate   gate (&m_StateLock);

    if (m_pNodeTable == NULL)
        return E_FAIL;      // Not available -- must create compiler with CCF_KEEPNODETABLES

    // Caller just wants the size?
    if (pKeyedNodes == NULL)
    {
        if (piCopied != NULL)
            *piCopied = m_pNodeArray->Count();
        return S_OK;
    }

    // Copy over the amount requested, or the total number of nodes, whichever
    // is smaller...
    long    iCopy = min (iSize, m_pNodeArray->Count());

    memcpy (pKeyedNodes, m_pNodeArray->Base(), iCopy * sizeof (KEYEDNODE));
    if (piCopied != NULL)
        *piCopied = iCopy;

    return iCopy == m_pNodeArray->Count() ? S_OK : S_FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::ParseForErrors

HRESULT CSourceModule::ParseForErrors (CSourceData *pData)
{
    HRESULT     hr;
    BASENODE    *pTopNode;

    STARTTIMER ("parse for errors");

    m_fParsingForErrors = TRUE;

    // First, parse the top level.
    if (FAILED (hr = ParseTopLevel (pData, &pTopNode)))
    {
        STOPTIMER();
        m_fParsingForErrors = FALSE;
        return hr;
    }

    // Now, iterate through the tree and parse all interior nodes.
//#ifdef _DEBUG
    m_fAccumulateSize = TRUE;
    m_dwInteriorSize = 0;
    m_iInteriorTrees = 0;
//#endif
    hr = ParseInteriorsForErrors (pData, pTopNode);
//#ifdef _DEBUG
    m_fAccumulateSize = FALSE;
//#endif
    STOPTIMER();
    m_fParsingForErrors = FALSE;
    return hr;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::ParseInteriorsForErrors

HRESULT CSourceModule::ParseInteriorsForErrors (CSourceData *pData, BASENODE *pNode)
{
    if (pNode == NULL)
        return S_OK;

    switch (pNode->kind)
    {
        case NK_NAMESPACE:
            return ParseInteriorsForErrors (pData, pNode->asNAMESPACE()->pElements);

        case NK_LIST:
        {
            HRESULT     hr;

            if (SUCCEEDED (hr = ParseInteriorsForErrors (pData, pNode->asLIST()->p1)))
                hr = ParseInteriorsForErrors (pData, pNode->asLIST()->p2);
            return hr;
        }

        case NK_CLASS:
        case NK_STRUCT:
        {
            HRESULT     hr;

            for (MEMBERNODE *p = pNode->asAGGREGATE()->pMembers; p != NULL; p = p->pNext)
            {
                if (p->kind == NK_NESTEDTYPE)
                {
                    if (FAILED (hr = ParseInteriorsForErrors (pData, p->asNESTEDTYPE()->pType)))
                        return hr;
                    continue;
                }

                if (p->kind != NK_CTOR && p->kind != NK_METHOD &&
                    p->kind != NK_PROPERTY && p->kind != NK_OPERATOR && p->kind != NK_INDEXER &&
                    p->kind != NK_DTOR)
                    continue;

                ICSInteriorTree *pTree;

                if (p->kind == NK_PROPERTY || p->kind == NK_INDEXER)
                {
                    // Before getting an interior parse tree, check each node to see if it
                    // has already been parsed for errors (NF_INTERIOR_PARSED)
                    if (p->asANYPROPERTY()->pGet != NULL && (p->asANYPROPERTY()->pGet->other & NFEX_INTERIOR_PARSED) == 0)
                    {
                        if (FAILED (hr = pData->GetInteriorParseTree (p->asANYPROPERTY()->pGet, &pTree)))
                            return hr;
                        pTree->Release();
                    }
                    if (p->asANYPROPERTY()->pSet != NULL && (p->asANYPROPERTY()->pSet->other & NFEX_INTERIOR_PARSED) == 0)
                    {
                        if (FAILED (hr = pData->GetInteriorParseTree (p->asANYPROPERTY()->pSet, &pTree)))
                            return hr;
                        pTree->Release();
                    }
                }
                else if ((p->other & NFEX_INTERIOR_PARSED) == 0)
                {
                    if (FAILED (hr = pData->GetInteriorParseTree (p, &pTree)))
                        return hr;
                    pTree->Release();
                }
            }
            break;
        }
    }

    return S_OK;
}


////////////////////////////////////////////////////////////////////////////////
// CSourceModule::FindLeafNode

HRESULT CSourceModule::FindLeafNode (CSourceData *pData, const POSDATA pos, BASENODE **ppNode, ICSInteriorTree **ppTree)
{
    HRESULT     hr = E_FAIL;
    BASENODE    *pTopNode;
    DWORD       dwExceptionCode;

    if (VSFSWITCH (gfUnhandledExceptions))
    {
        if (ppTree != NULL)
            *ppTree = NULL;

        *ppNode = NULL;

        if (SUCCEEDED (hr = ParseTopLevel (pData, &pTopNode)))
        {
            // Search this node for the "lowest" containing the given position
            BASENODE    *pLeaf = m_pParser->FindLeaf (pos, pTopNode, ppTree == NULL ? NULL : pData, ppTree);

            *ppNode = pLeaf;
            hr = pLeaf ? S_OK : E_FAIL;
        }
    }
    else
    {
        SRCMOD_EXCEPTION_TRYEX
        {
            if (ppTree != NULL)
                *ppTree = NULL;

            *ppNode = NULL;

            if (SUCCEEDED (hr = ParseTopLevel (pData, &pTopNode)))
            {
                // Search this node for the "lowest" containing the given position
                BASENODE    *pLeaf = m_pParser->FindLeaf (pos, pTopNode, ppTree == NULL ? NULL : pData, ppTree);

                *ppNode = pLeaf;
                hr = pLeaf ? S_OK : E_FAIL;
            }
        }
        SRCMOD_EXCEPTION_EXCEPT_FILTER_EX (PAL_Except1, &dwExceptionCode)
        {
            m_pController->HandleException (dwExceptionCode);
            hr = E_FAIL;
        }
        SRCMOD_EXCEPTION_ENDTRY
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::IsUpToDate

HRESULT CSourceModule::IsUpToDate (BOOL *pfTokenized, BOOL *pfTopParsed)
{
    // REVIEW:  Check this....
    HRESULT     hr = ((!m_fTextModified) && m_fTokenized) ? S_OK : S_FALSE;

    if (pfTokenized != NULL)
        *pfTokenized = m_fTokenized;

    if (pfTopParsed != NULL)
        *pfTopParsed = m_fParsedTop;

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::ForceUpdate

HRESULT CSourceModule::ForceUpdate (CSourceData *pData)
{
    return E_NOTIMPL;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::GetLexResults

HRESULT CSourceModule::GetLexResults (CSourceData *pData, LEXDATA *pLexData)
{
    HRESULT     hr = E_FAIL;
    DWORD       dwExceptionCode;

    if (VSFSWITCH (gfUnhandledExceptions))
    {
        CreateTokenStream ();
        *pLexData = m_lex;
        hr = S_OK;
    }
    else
    {
        SRCMOD_EXCEPTION_TRYEX
        {
            CreateTokenStream ();
            *pLexData = m_lex;
            hr = S_OK;
        }
        SRCMOD_EXCEPTION_EXCEPT_FILTER_EX (PAL_Except1, &dwExceptionCode)
        {
            m_pController->HandleException (dwExceptionCode);
            hr = E_FAIL;
        }
        SRCMOD_EXCEPTION_ENDTRY
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::GetSingleTokenPos

HRESULT CSourceModule::GetSingleTokenPos (CSourceData *pData, long iToken, POSDATA *pposToken)
{
    CreateTokenStream ();

    if (iToken < 0 || iToken >= m_lex.iTokens)
        return E_INVALIDARG;

    *pposToken = m_lex.pposTokens[iToken];
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::GetSingleTokenData

HRESULT CSourceModule::GetSingleTokenData (CSourceData *pData, long iToken, TOKENDATA *pTokenData)
{
    CreateTokenStream ();

    if (iToken == -1) iToken = 0;

    if (iToken < 0 || iToken >= m_lex.iTokens)
        return E_INVALIDARG;

    CScanLexer      ctx (GetNameMgr());
    POSDATA         *ppos = m_lex.pposTokens + iToken;
    FULLTOKEN       full;

    // NOTE:  Because another thread may be parsing at this very moment, we must
    // provide our own FULLTOKEN structure to the token scanner here!  We can
    // NOT use the RescanToken() function, since it modifies m_tok, and thus
    // potentially hoses the other thread.
    ctx.m_pszCurLine = m_lex.pszSource + m_lex.piLines[ppos->iLine];
    ctx.m_pszCurrent = ctx.m_pszCurLine + ppos->iChar;
    ctx.m_iCurLine = ppos->iLine;

    ctx.ScanToken (&full);
    *pTokenData = full;
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::GetTokenText

HRESULT CSourceModule::GetTokenText (long iTokenId, PCWSTR *ppszText, long *piLen)
{
    if (piLen != NULL)
        *piLen = 0;
    if (ppszText != NULL)
        *ppszText = L"";

    if (iTokenId < 0 || iTokenId >= TID_NUMTOKENS)
        return E_INVALIDARG;

    // If the stored length is 0, it's unknown.  Caller must use GetSingleTokenData
    // on a specific token index to get the desired data
    if (CParser::GetTokenInfo ((TOKENID)iTokenId)->iLen == 0)
        return S_FALSE;

    if (piLen != NULL)
        *piLen = CParser::GetTokenInfo ((TOKENID)iTokenId)->iLen;
    if (ppszText != NULL)
        *ppszText = CParser::GetTokenInfo ((TOKENID)iTokenId)->pszText;

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::IsInsideComment

HRESULT CSourceModule::IsInsideComment (CSourceData *pData, const POSDATA &pos, BOOL *pfInComment)
{
    HRESULT     hr = S_OK;
    DWORD       dwExceptionCode;

    if (VSFSWITCH (gfUnhandledExceptions))
    {
        hr = InternalIsInsideComment (pData, pos, pfInComment);
    }
    else
    {
        SRCMOD_EXCEPTION_TRYEX
        {
            hr = InternalIsInsideComment (pData, pos, pfInComment);
        }
        SRCMOD_EXCEPTION_EXCEPT_FILTER_EX(PAL_Except1, &dwExceptionCode)
        {
            m_pController->HandleException (dwExceptionCode);
            hr = E_FAIL;
        }
        SRCMOD_EXCEPTION_ENDTRY
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::InternalIsInsideComment

HRESULT CSourceModule::InternalIsInsideComment (CSourceData *pData, const POSDATA &pos, BOOL *pfInComment)
{
    HRESULT     hr = S_OK;

    *pfInComment = FALSE;

    // Create the token stream
    CreateTokenStream ();

    // Find the nearest comment
    long    i = FindNearestPosition (m_lex.pposComments, m_lex.iComments, pos);

    if (i == -1 || m_lex.pposComments[i] == pos)
    {
        *pfInComment = FALSE;
    }
    else
    {
        PCWSTR  p = m_lex.TextAt (m_lex.pposComments[i]);
        PCWSTR  pszStop = m_lex.TextAt (pos);
        PCWSTR  pStart = p;
        WCHAR   ch;

        // Found a comment BEFORE this one.  Scan from it to pos and see if
        // we are still in comment state
        ch = NEXTCHAR (pStart, p, ch);
        if (ch != '/')
            hr = E_FAIL;        // Something's wrong...
        else if (PEEKCHAR (pStart, p, ch) == '/')
        {
            // A single-line comment.  We're in it if we're on the same line.
            // (BUG:  If the line is an escaped sequence, we lie here...
            *pfInComment = m_lex.pposComments[i].iLine == pos.iLine;
        }
        else
        {
            if (ch == '*')
            {
                WCHAR   chNext;

                // scan multi-line comment
                NEXTCHAR(pStart, p, ch);
                while (TRUE)
                {
                    if (p > pszStop)
                    {
                        // We're in the comment
                        *pfInComment = TRUE;
                        break;
                    }

                    PCWSTR  pszSave = p;
                    NEXTCHAR(pStart, p, ch);
                    if (*pszSave == 0)
                    {
                        // The comment didn't end.  We're in the comment.
                        *pfInComment = TRUE;
                        break;
                    }

                    if (ch == '*' && PEEKCHAR(pStart, p, chNext) == '/')
                    {
                        NEXTCHAR (pStart, p, ch);
                        *pfInComment = p > pszStop;
                        break;
                    }

                    if (ch == '\n' || ch == '\r' || ch == UCH_PS || ch == UCH_LS)
                    {
                        if (ch == '\r' && PEEKCHAR(pStart, p, ch) == '\n')
                            NEXTCHAR(pStart, p, ch);                // skip linefeed, if any
                    }
                }
            }
            else
                hr = E_FAIL;
        }
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::CreateNewError
//
// This is the error/warning creation function.  All forms of errors and warnings
// from the lexer and parser come through here.  Also, all forms of errors and
// warnings from the lexer/parser have exactly one location, so it is provided
// here.

void CSourceModule::CreateNewError (short iErrorId, va_list args, const POSDATA &posStart, const POSDATA &posEnd)
{
    // Since we're going to use the error container, we need to grab the state
    // lock for this.
    CTinyGate   gate (&m_StateLock);

    // We may not have an error container -- if not, then we ignore errors.
    if (m_pCurrentErrors == NULL)
        return;

    CError      *pError = NULL;
    HRESULT     hr;

    // First, create the error object
    if (FAILED (hr = m_pController->CreateError (iErrorId, args, &pError)))
        return;     // Hmm...

    // This may not have created anything (for example, if the error was a
    // warning that doesn't meet the warning level criteria...)
    if (hr == S_FALSE)
        return;

    ASSERT (pError != NULL);

    PCWSTR  pszFileName;
    PCWSTR  pszMapFileName;
    POSDATA mapStart(posStart), mapEnd(posEnd);

    MapLocation (&mapStart, &pszMapFileName);
    MapLocation (&mapEnd, NULL);

    if (FAILED (hr = m_pSourceText->GetName (&pszFileName)) ||
        FAILED (hr = pError->AddLocation (pszFileName, &posStart, &posEnd, pszMapFileName, &mapStart, &mapEnd)))
    {
    }

    m_iErrors++;

    // Now, add this error to the current error container.
    m_pCurrentErrors->AddError (pError);
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::ErrorAtPosition

void __cdecl CSourceModule::ErrorAtPosition (LONG_PTR iLine, LONG_PTR iCol, LONG_PTR iExtent, short iErrorId, ...)
{
    va_list args;
    va_start(args, iErrorId);

    POSDATA pos((long)iLine, (long)iCol);
    POSDATA posEnd((long)iLine, (long)(iCol + max (iExtent, 1)));
    CreateNewError (iErrorId, args, pos, posEnd);
    va_end(args);
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::ErrorAtPosition

void CSourceModule::ErrorAtPosition (va_list args, long iLine, long iCol, long iExtent, short iErrorId)
{
    POSDATA pos(iLine, iCol);
    POSDATA posEnd(iLine, iCol + max (iExtent, 1));
    CreateNewError (iErrorId, args, pos, posEnd);
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::AddToNodeTable

void CSourceModule::AddToNodeTable (BASENODE *pNode)
{
    if (m_pNodeTable == NULL)
        return;

    CTinyGate   gate (&m_StateLock);

    // Add the node to the array, so we know its global ordinal
    NODECHAIN   *pChain = (NODECHAIN *)m_heap.Alloc (sizeof (NODECHAIN));
    KEYEDNODE   *pKeyedNode;

    if (FAILED (m_pNodeArray->Add (&pChain->iGlobalOrdinal, &pKeyedNode)))
        return;     // Oh, well...

    pChain->pNext = NULL;
    pKeyedNode->pNode = pNode;

    DWORD_PTR   dwCookie;
    HRESULT     hr;

    // Build the key for this node and find/add its node chain
    if (SUCCEEDED (pNode->BuildKey (GetNameMgr(), TRUE, &pKeyedNode->pKey)) &&
        SUCCEEDED (hr = m_pNodeTable->FindOrAdd (pKeyedNode->pKey, &dwCookie)))
    {
        // The new one must go on the END of the node chain.
        if (hr == S_FALSE)
        {
            // This is the first one (most common case by huge margin...)
            m_pNodeTable->SetData (dwCookie, pChain);
        }
        else
        {
            NODECHAIN   *pExisting = m_pNodeTable->GetData (dwCookie);

            // Find the end and add it
            if (pExisting == NULL)
                return;     // WHAT?!?

            while (pExisting->pNext != NULL)
                pExisting = pExisting->pNext;

            pExisting->pNext = pChain;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::ParseInteriorNode

void CSourceModule::ParseInteriorNode (BASENODE *pNode)
{
    TimerStart(TIME_PARSEINTERIOR);

    switch (pNode->kind)
    {
        case NK_METHOD:
        case NK_CTOR:
        case NK_DTOR:
        case NK_OPERATOR:
        {
            METHODNODE  *pMethod = (METHODNODE *)pNode;

            m_pParser->Rewind (pMethod->iOpen);
            if (m_pParser->CurToken() == TID_OPENCURLY)
                pMethod->pBody = (BLOCKNODE *)m_pParser->ParseBlock (pMethod);
            break;
        }

        case NK_ACCESSOR:
        {
            ACCESSORNODE    *pAccessor = pNode->asACCESSOR();

            m_pParser->Rewind (pAccessor->iOpen);
            if (m_pParser->CurToken() == TID_OPENCURLY)
                pAccessor->pBody = (BLOCKNODE *)m_pParser->ParseBlock (pAccessor);
            break;
        }

        default:
        {
            ASSERT(!"Unknown interior node");
            break;
        }
    }

    TimerStop(TIME_PARSEINTERIOR);
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::GetExtent

HRESULT CSourceModule::GetExtent (BASENODE *pNode, POSDATA *pposStart, POSDATA *pposEnd, ExtentFlags flags)
{
    m_pParser->GetExtent (pNode, pposStart, pposEnd, flags);
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::GetTokenExtent

HRESULT CSourceModule::GetTokenExtent (BASENODE *pNode, long *piFirst, long *piLast)
{
    BOOL    fMissingName;

    if (piFirst != NULL)
        *piFirst = m_pParser->GetFirstToken (pNode, EF_ALL, &fMissingName);

    if (piLast != NULL)
        *piLast = m_pParser->GetLastToken (pNode, EF_ALL, &fMissingName);

    return S_OK;
}


////////////////////////////////////////////////////////////////////////////////
// CSourceModule::IsDocComment
BOOL CSourceModule::IsDocComment(long iComment)
{
    // A "doc comment" is formed by exactly three slashes starting a comment.
    PCWSTR  p = m_lex.TextAt (m_lex.pposComments[iComment]);
    if (p[1] == '/' && p[2] == '/' && p[3] != '/')
        return TRUE;
    else
        return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::GetDocComment

HRESULT CSourceModule::GetDocComment (BASENODE *pNode, long *piComment, long *piCount)
{
    // If we don't have any comments, this is easy...
    if (m_lex.iComments == 0)
        return E_FAIL;

    // In order to have a doc comment, this node must be a type or member.  If it
    // isn't, look up the parent chain until it is.
    while (!pNode->InGroup (NG_TYPE|NG_FIELD|NG_METHOD|NG_PROPERTY) && pNode->kind != NK_ENUMMBR && pNode->kind != NK_VARDECL)
    {
        pNode = pNode->pParent;
        if (pNode == NULL)
            return E_INVALIDARG;
    }

    BASENODE    *pAttr = NULL;

    // Go from VARDECL to parent.
    if (pNode->kind == NK_VARDECL)
    {
        pNode = pNode->asVARDECL()->pParent;
        while (pNode->kind == NK_LIST)
            pNode = pNode->pParent;
    }

    // Get the position of the first token of this thing -- which should include
    // its attribute if it has one
    if (pNode->InGroup (NG_AGGREGATE))
        pAttr = ((CLASSNODE *)pNode)->pAttr;
    else if (pNode->kind == NK_DELEGATE)
        pAttr = pNode->asDELEGATE()->pAttr;
    else if (pNode->InGroup (NG_FIELD|NG_METHOD|NG_PROPERTY) || pNode->kind == NK_ENUMMBR)
        pAttr = ((MEMBERNODE *)pNode)->pAttr;

    long    iFirstToken, iLastToken;
    BOOL    fMissing;
    long    iComment;

    iFirstToken = m_pParser->GetFirstToken ((pAttr != NULL) ? pAttr : pNode, EF_ALL, &fMissing);

    iLastToken = iFirstToken;
    iFirstToken--;
    if (iFirstToken >= 0)
    {

        // Search the comment table for the first comment that appears between iFirstToken and iLastToken.
        iComment = FindNearestPosition (m_lex.pposComments, m_lex.iComments, m_lex.pposTokens[iFirstToken]);
        if (iComment < m_lex.iComments - 1)
            iComment++;
    }
    else
    {
        // iFirstToken == -1.
        // We're at the top of the file, get the first comment.
        iComment = 0;
    }

    if (iComment >= m_lex.iComments || m_lex.pposComments[iComment] > m_lex.pposTokens[iLastToken] ||
        (iFirstToken >= 0 && m_lex.pposComments[iComment] <= m_lex.pposTokens[iFirstToken]))
        return E_FAIL;

    long iLast;
    for (iLast = iComment; iLast < m_lex.iComments - 1 && m_lex.pposComments[iLast + 1] < m_lex.pposTokens[iLastToken]; iLast++)
        ;

    // Okay, comments from iComment to iLast are before the given node.  Now,
    // search backwards from iLast and include only triple-slash comments that
    // are on consecutive lines
    // But first skip any intervening comments
    do
    {
        if (IsDocComment(iLast))
            break;
        iLast--;
    } while (iLast >= iComment);

    if (iLast < iComment) // No DocComments (just regular comments)
        return E_FAIL;

    long iFirst;
    for (iFirst = iLast + 1; iFirst > iComment; )
    {
        if (IsDocComment(iFirst-1))
            iFirst--;
        else
            break;

        if (iFirst > iComment && m_lex.pposComments[iFirst-1].iLine != m_lex.pposComments[iFirst].iLine - 1)
            break;
    }

    // Done!
    if (iFirst > iLast)
        return E_FAIL;
    *piComment = iFirst;
    *piCount = iLast - iFirst + 1;
    return S_OK;
}


////////////////////////////////////////////////////////////////////////////////
// CSourceModule::GetDocCommentText
//
// Allocates memory from m_pStdHeap for the heap.  Free it yourself
// or call FreeDocCommentText.  Sets *ppszText = NULL if there is
// no DocComment

HRESULT CSourceModule::GetDocCommentText (BASENODE *pNode, LPWSTR *ppszText, long *piComment, long *piAdjust, LPCWSTR pszIndent)
{
    HRESULT hr = S_OK;
    long iStart = 0, iLen = 0, iTextLen = 0, i;
    bool bRemoveFirstChar = true;
    int iRemoveChars = 0;
    int iAddChars = 0;

    ASSERT(ppszText != NULL);
    *ppszText = NULL;

    if (FAILED(hr = GetDocComment(pNode, &iStart, &iLen))) {
        if (hr == E_FAIL) //this means there are no DocComments for this node
            return S_FALSE;
        else
            return hr;
    }

    if (piComment)
        *piComment = iStart;

    // There must be at least one more line in the file, so use
    // the start of the comment and the start of the line after the comment
    // to find the a maximal length for the comment text
    iLen--;
    ASSERT (m_lex.iLines > (long)m_lex.pposComments[iStart + iLen].u.iLine);
    iTextLen = (long)((m_lex.pszSource + m_lex.piLines[m_lex.pposComments[iStart + iLen].iLine + 1]) - m_lex.TextAt (m_lex.pposComments[iStart]));
    iLen++;

    // Scan each line to see if we can remove the first whitespace
    for (i = iStart; i < iStart + iLen && bRemoveFirstChar; i++)
        bRemoveFirstChar = !!IsWhitespace (m_lex.TextAt (m_lex.pposComments[i])[3]);

    if (bRemoveFirstChar)
        iRemoveChars = 4;
    else
        iRemoveChars = 3;

    if (pszIndent != NULL)
        iAddChars = (int)wcslen(pszIndent);

    if (piAdjust)
        *piAdjust = iAddChars - iRemoveChars;

    // Allocate and copy in the text
    *ppszText = (LPWSTR)m_pStdHeap->Alloc(sizeof(WCHAR) * (iTextLen + (iAddChars - iRemoveChars) * (iLen + 1) + 1));
    WCHAR *p = *ppszText;
    size_t len = 0;
    for (i = iStart; i < (iStart + iLen); i++)
    {
        len = (m_lex.pszSource + m_lex.piLines[m_lex.pposComments[i].iLine + 1]) - (m_lex.TextAt (m_lex.pposComments[i]) + iRemoveChars);
        wcscpy (p, pszIndent);
        p += iAddChars;
        wcsncpy (p, m_lex.TextAt (m_lex.pposComments[i]) + iRemoveChars, len);
        p += len;
    }
    *p = L'\0';

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::GetDocCommentPos
//  iComment is an index into the comment table (returned as piComment from
//      GetDocCommentText or GetDocComment)
//  loc will be the resulting location (out parameter)
//  iLineOffest is the number of lines from the beginning
//  iCharOffset is the character offset (this must include the adjust offset)

HRESULT CSourceModule::GetDocCommentPos (long iComment, POSDATA *loc, long iLineOffset, long iCharOffset)
{
    ASSERT(loc != NULL);
    ASSERT(m_lex.iComments > iComment);

    *loc = m_lex.pposComments[iComment];
    loc->iLine += iLineOffset;
    loc->iChar += iCharOffset;
    if (m_lex.pposComments[iComment].iLine + iLineOffset != loc->iLine) {
        loc->iLine = MAX_POS_LINE;
        loc->iChar = 0;
        return S_FALSE;
    } else if (m_lex.pposComments[iComment].iChar + iCharOffset != loc->iChar) {
        loc->iChar = MAX_POS_LINE_LEN;
        return S_FALSE;
    }
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// CSourceModule::FreeDocCommentText
//      Frees memory allocated in GetDocCommentText() from m_pStdHeap

HRESULT CSourceModule::FreeDocCommentText (LPWSTR *ppszText)
{
    ASSERT(ppszText != NULL);
    if (*ppszText != NULL)
        m_pStdHeap->Free(*ppszText);
    *ppszText = NULL;
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// ExceptionFilter
LONG CSourceModule::ExceptionFilter (EXCEPTION_POINTERS *pExceptionInfo, PVOID pv)
{
    if (pv)
        ((SrcModExceptionFilterData *)pv)->pController->SetExceptionData (pExceptionInfo);

#ifdef _DEBUG
    return DebugExceptionFilter (pExceptionInfo, &(((SrcModExceptionFilterData *)pv)->ExceptionCode));
#else
    return EXCEPTION_EXECUTE_HANDLER;
#endif
}

