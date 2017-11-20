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
// File: tokdata.h
//
// ===========================================================================

#ifndef _TOKDATA_H_
#define _TOKDATA_H_

#include "enum.h"
#include "posdata.h"

#include "unichar.h"
#include "uniprop.h"
#undef scope

struct NAME;

////////////////////////////////////////////////////////////////////////////////
// TOKFLAGS

enum
{
    TFF_NSELEMENT       = 0x00000001,   // Token belongs to first set for namespace elements
    TFF_MEMBER          = 0x00000002,   // Token belongs to first set for type members
    TFF_TYPEDECL        = 0x00000004,   // Token belongs to first set for type declarations (sans modifiers)
    TFF_PREDEFINED      = 0x00000008,   // Token is a predefined type
    TFF_TERM            = 0x00000010,   // Token belongs to first set for term-or-unary-operator (follows casts)
    TFF_OVLUNOP         = 0x00000020,   // Token is an overloadable unary operator
    TFF_OVLBINOP        = 0x00000040,   // Token is an overloadable binary operator
    TFF_OVLOPKWD        = 0x00000080,   // Token is NOT an operator even though op != OP_NONE
    TFF_CASTEXPR        = 0x00000100,   // Token after an ambiguous type-or-expr cast forces a cast
    TFF_MODIFIER        = 0x00000200,   // Token is a modifier keyword

    // These flags are for instances of tokens
    TF_UNTERMINATED     = 0x00000001,   // Unterminated token such as TID_STRINGLIT or TID_CHARLIT
    TF_VERBATIMSTRING   = 0x00000002,   // TID_STRINGLIT is a @"..." form
};

////////////////////////////////////////////////////////////////////////////////
// TOKENDATA

struct TOKENDATA
{
    TOKENID     iToken;                 // Token ID
    POSDATA     posTokenStart;          // First character of token
    POSDATA     posTokenStop;           // First character following token
    NAME        *pName;                 // Name (iff TID_IDENTIFIER)
    long        iLength;                // Length of token -- INCLUDING multiple lines in @"..." string cases
    DWORD       dwFlags;                // Flags about token (TF_* flags above, NOT TFF_* flags...)
};

class CIdentTable;

////////////////////////////////////////////////////////////////////////////////
// LEXDATA
//
// This structure contains all the data that is accumlated/calculated/produced
// by bringing a source module's token stream up to date.  This includes the
// token stream (tokens, positions, count), the line table (line offsets and
// count), the raw source text, the comment table (positions and count), the
// conditional compilation state transitions (line numbers and count), etc., etc.
// Note that some of the fields in this struct will not be filled with data
// unless the appropriate flags were passed to the construction of the
// compiler (controller) object -- for example, the identifier table will not
// be present unless CCF_KEEPIDENTTABLES was given, etc.
//
// All the data pointed to by this structure is owned by the originating
// source module, and is only valid while the calling thread holds a reference
// to the source data object through which it obtained this data!

struct LEXDATA
{
    // Raw source text pointer
    PCWSTR           pszSource;             // Pointer to the entire provided source text, null-terminated

    // Line table data
    long            *piLines;               // Array of offsets from pszSource of beginning of each line (piLines[0] == 0 always)
    long            iLines;                 // Line count

    // Token stream data
    BYTE            *pTokens;               // Array of TOKENID values (stored in bytes)
    POSDATA         *pposTokens;            // Array of positions corresponding to each token in pTokens
    long            iTokens;                // Token count

    // Comment table data                   (CCF_TRACKCOMMENTS flags required)
    POSDATA         *pposComments;          // Array of position values for each comment in file (including XML-comments)
    long            iComments;              // Comment count

    // Conditional compilation data
    long            *piTransitionLines;     // Array of line numbers indicating conditional inclusion/exclusion transitions
    long            iTransitionLines;       // Transition count

    // Identifier table                     (CCF_KEEPIDENTTABLES flag required)
    CIdentTable     *pIdentTable;           // Identifier table

    // Region table data
    long            *piRegionStart;         // Array of line numbers indicating #region directives
    long            *piRegionEnd;           // Array of line numbers indicating #endregion directives (CORRESPONDING -- these could be nested but are represented here in order of appearance of #region)
    long            iRegions;               // Region count

    // Helpful little translator functions
    PCWSTR          TextAt (long iLine, long iChar) { return pszSource + piLines[iLine] + iChar; }
    PCWSTR          TextAt (const POSDATA &pos) { return TextAt (pos.iLine, pos.iChar); }
    WCHAR           CharAt (long iLine, long iChar) { return TextAt (iLine, iChar)[0]; }
    WCHAR           CharAt (const POSDATA &pos) { return CharAt (pos.iLine, pos.iChar); }
    HRESULT         GetDocCommentXML (long iFirst, long iCount, BSTR *pbstrXML);
    HRESULT         FindFirstTokenOnLine (long iLine, long *piToken);
    HRESULT         FindFirstNonWhiteChar (long iLine, long *piChar);
    HRESULT         FindEndOfComment (long iComment, POSDATA *pposEnd);
    HRESULT         GetPreprocessorDirective (long iLine, ICSNameTable *pNameTable, PPTOKENID *piToken, long *piStart, long *piEnd);
    long            GetLineLength (long iLine);
    BOOL            IsWhitespace (long iStartLine, long iStartChar, long iEndLine, long iEndChar);
    BOOL            IsLineWhitespaceBefore (long iLine, long iChar) { return IsWhitespace (iLine, 0, iLine, iChar); }
    BOOL            IsLineWhitespaceBefore (const POSDATA &pos) { return IsWhitespace (pos.iLine, 0, pos.iLine, pos.iChar); }
    BOOL            IsLineWhitespaceAfter (long iLine, long iChar) { return IsWhitespace (iLine, iChar, iLine, GetLineLength (iLine)); }
    BOOL            IsLineWhitespaceAfter (const POSDATA &pos) { return IsWhitespace (pos.iLine, pos.iChar, pos.iLine, GetLineLength (pos.iLine)); }
};

// Useful functions...
inline BOOL IsIdentifierProp (BYTE prop) { return IsPropAlpha(prop) || IsPropLetterDigit(prop); }
inline BOOL IsIdentifierChar (WCHAR c) { return IsIdentifierProp(UProp(c)) || c == L'_'; }
inline BOOL IsIdentifierPropOrDigit(BYTE prop) { return IsIdentifierProp(prop) || IsPropDecimalDigit(prop) || IsPropNonSpacingMark(prop) || IsPropCombiningMark(prop)  || IsPropConnectorPunctuation(prop) || IsPropOtherFormat(prop); }
inline BOOL IsIdentifierCharOrDigit (WCHAR c) { return IsIdentifierPropOrDigit(UProp(c)); }
inline BOOL IsHexDigit (WCHAR c) { return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'); }
inline int  HexValue (WCHAR c) { ASSERT (IsHexDigit (c)); return (c >= '0' && c <= '9') ? c - '0' : (c & 0xdf) - 'A' + 10; }
inline BOOL IsWhitespaceChar (WCHAR ch) { return ch == ' ' || ch == '\t' || ch == UCH_BOM || ch == 0x001a || ch == '\v' || ch == '\f'; }
inline BOOL IsEndOfLineChar (WCHAR ch) { return ch == '\r' || ch == '\n' || ch == UCH_PS || ch == UCH_LS; }

#endif  // _TOKDATA_H_
