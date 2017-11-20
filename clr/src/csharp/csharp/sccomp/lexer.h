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
// File: lexer.h
//
// ===========================================================================

#ifndef __lexer_h__
#define __lexer_h__

#include "scciface.h"
#include "tokinfo.h"
#include "namemgr.h"
#include "array.h"

class COptionData;

////////////////////////////////////////////////////////////////////////////////
// CLexer
//
// This is the base lexer class.  It's abstract -- you must create a derivation
// and implement the pure virtuals.

class CLexer
{
public:
    COptionData     *m_pOptions;
    NAMEMGR         *m_pNameMgr;                // Name manager
    PCWSTR          m_pszCurrent;               // Current character
    PCWSTR          m_pszCurLine;               // Beginning of current line
    long            m_iCurLine;                 // Index of current line
    PCWSTR          m_pszTokenStart;            // Start of token
    unsigned        m_fEscaped:1;               // TRUE if token had escapes
    unsigned        m_fFirstOnLine:1;           // TRUE if no tokens have been scanned from this line yet
    unsigned        m_fPreproc:1;               // TRUE if preprocessor tokens are valid
    unsigned        m_fTokensSeen:1;            // TRUE if tokens have been seen (and thus #define/#undef are invalid)
    unsigned        m_fLimitExceeded:1;         // TRUE if too many lines or line too long
    unsigned        m_fThisLineTooLong:1;       // TRUE if this line is too long and an error has already been given

    CLexer (NAMEMGR *pNameMgr) :
        m_pOptions(NULL),
        m_pNameMgr(pNameMgr),
        m_pszCurrent(NULL),
        m_pszCurLine(NULL),
        m_iCurLine(0),
        m_pszTokenStart(NULL),
        m_fEscaped(FALSE),
        m_fFirstOnLine(FALSE),
        m_fPreproc(FALSE),
        m_fTokensSeen(FALSE),
        m_fLimitExceeded(FALSE),
        m_fThisLineTooLong(FALSE) {}
    virtual ~CLexer () {}

    BOOL        PositionOf (PCWSTR psz, POSDATA *ppos);
    TOKENID     ScanToken (FULLTOKEN *pFT);

    // Overridables
    // Derived classes must call CLexer::TrackLine so that we can reset m_fThisLineTooLong
    // Otherwise we can't properly report exactly 1 error message when a line is too long
    virtual void            TrackLine (PCWSTR pszNewLine) {m_fThisLineTooLong = FALSE; }
    virtual void __cdecl    ErrorAtPosition (long iLine, long iCol, long iExtent, short iErrorId, ...) = 0;
    virtual BOOL            ScanPreprocessorLine (PCWSTR &p) = 0;
    virtual void            RecordCommentPosition (const POSDATA &pos) = 0;
};

////////////////////////////////////////////////////////////////////////////////
// CTextLexer
//
// This is the implementation of a CLexer that also exposes ISCLexer for
// external clients.  It duplicates the given input text (and thus isn't intended
// for large-scale lexing tasks), creates a line table and token stream, and has
// rescan capability.

class CTextLexer :
    public CComObjectRootMT,
    public ICSLexer,
    public CLexer
{
private:
    CStructArray<BYTE>      m_arrayTokens;
    CStructArray<POSDATA>   m_arrayTokenPos;
    PWSTR                   m_pszInput;
    CStructArray<long>      m_arrayLines;
    BOOL                    m_fTokenized;
    HRESULT                 m_hr;

public:
    BEGIN_COM_MAP(CTextLexer)
        COM_INTERFACE_ENTRY(ICSLexer)
    END_COM_MAP()

    CTextLexer () : CLexer(NULL), m_pszInput(NULL), m_fTokenized(FALSE), m_hr(S_OK) {}
    ~CTextLexer ();

    static  HRESULT CreateInstance (ICSNameTable *pNameTable, ICSLexer **ppLexer);

    HRESULT         Initialize (ICSNameTable *pNameTable);

    // ICSLexer
    STDMETHOD(SetInput)(PCWSTR pszInput, long iLen);
    STDMETHOD(GetLexResults)(LEXDATA *pLexData);
    STDMETHOD(RescanToken)(long iToken, TOKENDATA *pTokenData);

    // CLexer
    void            TrackLine (PCWSTR pszNewLine);
    void __cdecl    ErrorAtPosition (long iLine, long iCol, long iExtent, short iErrorId, ...) {}
    BOOL            ScanPreprocessorLine (PCWSTR &p) { return FALSE; }
    void            RecordCommentPosition (const POSDATA &pos) {}
};


////////////////////////////////////////////////////////////////////////////////
// CScanLexer
//
// This is the simplest form of a lexer, which allows you to initialize it with
// a piece of null-terminated text, and then call ScanToken() repeatedly until
// it returns TID_ENDFILE.  There is no rescan capability, short of calling
// SetInput() again.  Also, no line tracking is done, so the entire input text
// is assumed to be a single line (this unfortunately means it can only scan
// text <= MAX_LINE_LEN in length; otherwise it will overflow the POSDATA in
// the fulltoken structure...
//                   The above comments about lines is no longer true...

class CScanLexer : public CLexer
{
public:
    CScanLexer (NAMEMGR *pNameMgr) : CLexer (pNameMgr) {}

    void        SetInput (PCWSTR pszInput) { m_pszCurLine = m_pszCurrent = pszInput; }

    // CLexer overrides.  Note, nothing happens for ALL of them (except TrackLine)...
    void            TrackLine (PCWSTR pszNewLine) { m_pszCurLine = pszNewLine; m_iCurLine++; CLexer::TrackLine(pszNewLine); }
    void __cdecl    ErrorAtPosition (long iLine, long iCol, long iExtent, short iErrorId, ...) {}
    BOOL            ScanPreprocessorLine (PCWSTR &p) { return FALSE; }
    void            RecordCommentPosition (const POSDATA &pos) {}
};

#endif //__lexer_h__

