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
// File: parser.cpp
//
// ===========================================================================

#include "stdafx.h"
#include "parser.h"
#include "compiler.h"
#include <float.h>
#include <limits.h>
#include <oleauto.h>

////////////////////////////////////////////////////////////////////////////////
// CListMaker

class CListMaker
{
private:
    BASENODE        **ppDest;
    CParser         *pParser;

public:
    CListMaker (CParser *p, BASENODE **pp) : ppDest(pp), pParser(p) { *pp = NULL; }
    void Restart (BASENODE **pp) { ppDest = pp; *pp = NULL; }
    BASENODE *Add (BASENODE *pNew)
    {
        // Handle NULL -- this is not an error case; some functions return NULL after
        // reporting an error, which should not corrupt the list.
        if (pNew == NULL)
            return pNew;

        ASSERT (pNew->pParent != NULL && pNew->pParent != (BASENODE *)0xcccccccccccccccc);

        // If NULL, *ppDest just gets the value of pNew, which should already have
        // it's parent relationship set up (we assert that above).
        if (*ppDest == NULL)
        {
            // Do not update *ppDest -- this isn't a list yet!
            *ppDest = pNew;
            return pNew;
        }

        BINOPNODE   *pList = (BINOPNODE *)pParser->AllocNode (NK_LIST);

        // Create the new list node
        pList->p1 = *ppDest;
        pList->p2 = pNew;
        pList->pParent = pList->p1->pParent;    // Use (*ppDest)'s old parent
        pList->p1->pParent = pList->p2->pParent = pList;

        // Update the destination pointer for the next list node
        *ppDest = pList;
        ppDest = &pList->p2;

        ASSERT ((*ppDest)->kind != NK_LIST);

        return pList;       // Return the new node in case the caller wants to store flags/position, etc.
    }
};



////////////////////////////////////////////////////////////////////////////////
// CParser::m_rgTokenInfo

#define TOK(name, id, flags, type, stmtfunc, binop, unop, selfop, color) { name, stmtfunc, flags, sizeof(name)/sizeof(WCHAR)-1, type, binop, unop, selfop },
const TOKINFO   CParser::m_rgTokenInfo[TID_NUMTOKENS] = {
    #include "tokens.h"
};

////////////////////////////////////////////////////////////////////////////////
// CParser::m_rgOpInfo

#define OP(n,p,a,stmt,t,pn,e) {t,p,a},
const OPINFO    CParser::m_rgOpInfo[] = {
    #include "ops.h"
    { TID_UNKNOWN, 0, 0}};

////////////////////////////////////////////////////////////////////////////////
// CParser::CParser

CParser::CParser (CController *pController) :
    m_pController(pController),
    m_iCurTok(0),
    m_pTokens(NULL),
    m_pposTokens(NULL),
    m_iTokens(0),
    m_pszSource(NULL),
    m_piLines(NULL),
    m_iLines(0),
    m_fErrorOnCurTok(FALSE),
    m_iErrors(0),
    m_pMissingName(NULL),
    m_pGetName(NULL),
    m_pSetName(NULL)
{
    m_pMissingName = pController->GetNameMgr()->AddString (L"?");
    m_pGetName = pController->GetNameMgr()->AddString (L"get");
    m_pSetName = pController->GetNameMgr()->AddString (L"set");
    m_pAddName = pController->GetNameMgr()->AddString (L"add");
    m_pRemoveName = pController->GetNameMgr()->AddString (L"remove");
}

////////////////////////////////////////////////////////////////////////////////
// CParser::~CParser

CParser::~CParser ()
{
}

////////////////////////////////////////////////////////////////////////////////
// CParser::SetInputData

void CParser::SetInputData (BYTE *pTokens, POSDATA *pposTokens, long iTokens, PCWSTR pszSource, long *piLines, long iLines)
{
    m_pTokens = pTokens;
    m_pposTokens = pposTokens;
    m_iTokens = iTokens;
    m_pszSource = pszSource;
    m_piLines = piLines;
    m_iLines = iLines;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::SetInputData

void CParser::SetInputData (LEXDATA *pLexData)
{
    m_pTokens = pLexData->pTokens;
    m_pposTokens = pLexData->pposTokens;
    m_iTokens = pLexData->iTokens;
    m_pszSource = pLexData->pszSource;
    m_piLines = pLexData->piLines;
    m_iLines = pLexData->iLines;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::Eat

inline void CParser::Eat (TOKENID iTok)
{
    TOKENID iTokActual;
    if ((iTokActual = CurToken()) == iTok)
        NextToken();
    else
        ErrorAfterPrevToken (ExpectedErrorFromToken (iTok, iTokActual), GetTokenInfo(iTok)->pszText, GetTokenInfo(iTokActual)->pszText);
}

////////////////////////////////////////////////////////////////////////////////
// CParser::CheckToken

inline BOOL CParser::CheckToken (TOKENID iTok)
{
    TOKENID iTokActual;
    if ((iTokActual = CurToken()) != iTok)
    {
        ErrorAfterPrevToken (ExpectedErrorFromToken (iTok, iTokActual), GetTokenInfo(iTok)->pszText, GetTokenInfo(iTokActual)->pszText);
        return FALSE;
    }

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::RescanToken

void CParser::RescanToken (long iTokenIdx, FULLTOKEN *pFT)
{
    CScanLexer  lexer(GetNameMgr());

    lexer.m_iCurLine = m_pposTokens[iTokenIdx].iLine;
    lexer.m_pszCurLine = m_pszSource + m_piLines[lexer.m_iCurLine];
    lexer.m_pszCurrent = lexer.m_pszCurLine + m_pposTokens[iTokenIdx].iChar;

    lexer.ScanToken (pFT);
}

////////////////////////////////////////////////////////////////////////////////
// CParser::GetTokenLength
//
// NOTE:  This function returns the length in characters of all tokens, so if it
// extends past the end of a line (such as an @"..." string might), the length
// will be longer than the line...

long CParser::GetTokenLength (long iTokenIdx)
{
    FULLTOKEN   tok;

    RescanToken (iTokenIdx, &tok);
    return tok.iLength;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::GetTokenStopPosition

void CParser::GetTokenStopPosition (long iTokenIdx, POSDATA *pposStop)
{
    FULLTOKEN   tok;

    RescanToken (iTokenIdx, &tok);
    *pposStop = tok.posTokenStop;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::Error
//
// Produce the given error/warning at the current token.

void __cdecl CParser::Error (enum ERRORIDS iErrorId, ...)
{
    va_list args;
    va_start(args, iErrorId);

    if (!m_fErrorOnCurTok)
    {
        POSDATA pos = m_pposTokens[Mark()];
        POSDATA posEnd = pos;

        posEnd.iChar += GetTokenLength (Mark());
        CreateNewError (iErrorId, args, pos, posEnd);
        m_fErrorOnCurTok = TRUE;
    }

    m_iErrors++;
    va_end(args);
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ErrorAtToken

void __cdecl CParser::ErrorAtToken (long iTokenIdx, enum ERRORIDS iErrorId, ...)
{
    va_list args;
    va_start(args, iErrorId);

    POSDATA pos = m_pposTokens[iTokenIdx];
    POSDATA posEnd = pos;

    posEnd.iChar += GetTokenLength (iTokenIdx);
    CreateNewError (iErrorId, args, pos, posEnd);
    m_iErrors++;
    va_end(args);
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ErrorAfterPrevToken
//
// If the current token is on the same line as the previous token, then behave
// just like Error. Otherwise, report the error 1 character beyond the
// previous token. Useful for "expected semicolon" type messages.

void __cdecl CParser::ErrorAfterPrevToken (enum ERRORIDS iErrorId, ...)
{
    va_list args;
    va_start(args, iErrorId);

    long iTokenIdx = Mark();
    POSDATA posCurToken = m_pposTokens[iTokenIdx];
    POSDATA posPrevToken = m_pposTokens[(iTokenIdx > 0) ? iTokenIdx - 1 : iTokenIdx];
    POSDATA pos, posEnd;

    if (posPrevToken.iLine == posCurToken.iLine) {
        pos = posEnd = posCurToken;
        posEnd.iChar += GetTokenLength (iTokenIdx);
        if (m_fErrorOnCurTok)
            goto SKIPERROR;
        m_fErrorOnCurTok = TRUE;
    }
    else {
        pos = posEnd = posPrevToken;
        pos.iChar += GetTokenLength(iTokenIdx - 1);
        posEnd.iChar = pos.iChar + 1;
    }

    CreateNewError (iErrorId, args, pos, posEnd);

SKIPERROR:
    m_iErrors++;
    va_end(args);
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ErrorAtPosition

void __cdecl CParser::ErrorAtPosition (long iLine, long iCol, long iExtent, enum ERRORIDS iErrorId, ...)
{
    va_list args;
    va_start(args, iErrorId);

    POSDATA pos(iLine, iCol);
    POSDATA posEnd(iLine, iCol + max (iExtent, 1));
    CreateNewError (iErrorId, args, pos, posEnd);
    m_iErrors++;
    va_end(args);
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ExpectedErrorFromToken

enum ERRORIDS CParser::ExpectedErrorFromToken (TOKENID iTok, TOKENID iTokActual)
{
    enum ERRORIDS   iError;

    switch (iTok)
    {
        case TID_IDENTIFIER:    
            if (iTokActual < TID_IDENTIFIER) 
                iError = ERR_IdentifierExpectedKW;   // A keyword -- use special message.
            else
                iError = ERR_IdentifierExpected;     
            break;
        case TID_SEMICOLON:     iError = ERR_SemicolonExpected;      break;
        //case TID_COLON:         iError = ERR_ColonExpected;          break;
        //case TID_OPENPAREN:     iError = ERR_LparenExpected;         break;
        case TID_CLOSEPAREN:    iError = ERR_CloseParenExpected;     break;
        case TID_OPENCURLY:     iError = ERR_LbraceExpected;         break;
        case TID_CLOSECURLY:    iError = ERR_RbraceExpected;         break;
        //case TID_CLOSESQUARE:   iError = ERR_CloseSquareExpected;    break;
        default:                iError = ERR_SyntaxError;
    }

    return iError;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::GetTokenText

PCWSTR CParser::GetTokenText (long iTokenIdx)
{
    FULLTOKEN   tok;

    switch (m_pTokens[iTokenIdx])
    {
        case TID_IDENTIFIER:
            RescanToken (iTokenIdx, &tok);
            return tok.pName->text;

        case TID_STRINGLIT:
        case TID_NUMBER:
        case TID_CHARLIT:
        {
            PWSTR   pszBuf = (PWSTR)MemAlloc (32 * sizeof (WCHAR));     // Must allocate, but this is for error case...
            long    iLen;
            bool    bTooLong = false;

            RescanToken (iTokenIdx, &tok);
            iLen = tok.iLength;
            if (iLen >= 31)
            {
                bTooLong = true;
                iLen = 28;
            }
            wcsncpy (pszBuf, tok.pszToken, iLen);
            if (bTooLong)
                wcscpy (pszBuf + iLen, L"...");
            else
                pszBuf[iLen] = 0;

            return pszBuf;
        }

        default:
            return GetTokenInfo ((TOKENID)m_pTokens[iTokenIdx])->pszText;
    }

    ASSERT( !"How did we get here?");
    return NULL;
}


////////////////////////////////////////////////////////////////////////////////
// CParser::ErrorInvalidMemberDecl

void CParser::ErrorInvalidMemberDecl(long iTokenIdx)
{
    // An invalid token was found in a member declaration. If the token is a modifier, give a special
    // error message that seems better                                              

    PCWSTR tokenText = GetTokenText(iTokenIdx);
    if (GetTokenInfo((TOKENID)m_pTokens[iTokenIdx])->dwFlags & TFF_MODIFIER)
    {
        Error(ERR_BadModifierLocation, tokenText);
    }
    else
    {
        Error (ERR_InvalidMemberDecl, tokenText);
    }
}

////////////////////////////////////////////////////////////////////////////////
// CParser::SkipToNamespaceElement
//
// Skip ahead to something that looks like it could be the start of a namespace
// element.  Return TRUE if one is found; or FALSE if end-of-file or unmatched
// close-curly is found first.

BOOL CParser::SkipToNamespaceElement ()
{
    long    iCurlyCount = 0;

    while (TRUE)
    {
        TOKENID iTok = NextToken();

        if (m_rgTokenInfo[iTok].dwFlags & TFF_NSELEMENT)
            return TRUE;

        switch (iTok)
        {
            case TID_NAMESPACE:
                return TRUE;

            case TID_OPENCURLY:
                iCurlyCount++;
                break;

            case TID_CLOSECURLY:
                if (iCurlyCount-- == 0)
                    return FALSE;
                break;

            case TID_ENDFILE:
                return FALSE;

            default:
                break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// CParser::SkipToMember
//
// Skip ahead to something that looks like it could be the start of a member.
// Return TRUE if one is found; or FALSE if end-of-file or unmatched close-curly
// is found first.

BOOL CParser::SkipToMember ()
{
    long    iCurlyCount = 0;

    while (TRUE)
    {
        TOKENID iTok = NextToken();

        // If this token can start a member, we're done
        if (m_rgTokenInfo[iTok].dwFlags & TFF_MEMBER)
            return TRUE;


        // Watch curlies and look for end of file/close curly
        switch (iTok)
        {
            case TID_OPENCURLY:
                iCurlyCount++;
                break;

            case TID_CLOSECURLY:
                if (iCurlyCount-- == 0)
                    return FALSE;
                break;

            case TID_ENDFILE:
                return FALSE;

            default:
                break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// CParser::SkipBlock
//
// The purpose of this function is to skip method/constructor/destructor/accessor
// bodies during the top-level parse.  The trick is that while scanning for the
// matching close-curly, we check for patterns that indicate a missing close-
// curly situation.  These patterns include class member declarations, as well
// as namespace member declarations.

DWORD CParser::SkipBlock (DWORD dwFlags, long *piClose)
{
    // We're always called with the open curly as the current token...
    Eat (TID_OPENCURLY);

    long    iCurlyCount = 1;

    for (;;NextToken())
    {
        TOKENID iTok = CurToken();
        DWORD   dwTokFlags = m_rgTokenInfo[iTok].dwFlags;

        if (iTok == TID_OPENCURLY)
        {
            // Increment curly nesting
            iCurlyCount++;
            continue;
        }

        if (iTok == TID_CLOSECURLY)
        {
            // Decrement curly nesting, and bail if 0
            iCurlyCount--;
            if (iCurlyCount == 0)
            {
                // Here's the end of our block.  Remember the close token
                // index, and indicate to the caller that this was a normal skip.
                if (piClose != NULL)
                    *piClose = Mark();
                NextToken();
                if (CurToken() == TID_SEMICOLON) {
                    Error (ERR_UnexpectedSemicolon);
                }
                return SB_NORMALSKIP;
            }
            continue;
        }

        if (iTok == TID_ENDFILE)
        {
            // Whoops!  We hit the end of the file without finding our match.
            // Report the error and indicate what happened to the caller.
            if (piClose != NULL)
                *piClose = Mark();
            CheckToken (TID_CLOSECURLY);
            return SB_ENDOFFILE;
        }

        if (iTok == TID_NAMESPACE)
        {
            // These are tokens that can NEVER be valid inside a method body.
            // Assume that things are very wrong, give an error expecting a
            // close curly, and tell the caller that they should back all the
            // way up to parsing namespace elements.
            if (piClose != NULL)
                *piClose = Mark();
            CheckToken (TID_CLOSECURLY);
            return SB_NAMESPACEMEMBER;
        }

        if (dwTokFlags & TFF_MEMBER)
        {
            DWORD       dwScan;
            long        iMark = Mark();

            // This token can start a type member declaration.  Scan it to see
            // if it really looks like a member declaration that is NOT also a
            // valid statement (such as a local variable declaration)
            dwScan = ScanMemberDeclaration (dwFlags);
            Rewind (iMark);
            if (dwScan != SB_NOTAMEMBER)
            {
                // This is a member declaration, so it's out of place here.
                // Assume a close-curly is missing, and indicate what we found
                // to the caller
                if (piClose != NULL)
                    *piClose = Mark();
                CheckToken (TID_CLOSECURLY);
                return dwScan;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ScanMemberDeclaration
//
// This function is used by SkipBlock to check to see if the current token is
// the start of a valid type member declaration that is NOT a valid statement.
// Note that field declarations look like local variable declarations, so only
// certain kinds of fields will trigger this (such as const, or those with an
// access modifier such as 'public').  But methods, constructors, properties,
// etc., will always return TRUE provided that they are correct/complete enough
// (i.e. up to their own block or semicolon).  If we're inside a property
// accessor (indicated by dwSkipFlags) then we also check for another accessor.
//
// Note that the current token is UNDEFINED after calling this function...

DWORD CParser::ScanMemberDeclaration (DWORD dwSkipFlags)
{
    // First, check to see if we're in an accessor and handle detection of
    // another one.
    if (dwSkipFlags & SBF_INACCESSOR)
    {
        long    iMark = Mark();

        // The situation we're trying to detect here is a missing close-curly
        // before the second accessor in a property declaration.
        while (m_rgTokenInfo[CurToken()].dwFlags & TFF_MODIFIER)
            NextToken();

        if (CurToken() == TID_IDENTIFIER && PeekToken() == TID_OPENCURLY)
        {
            FULLTOKEN   tok;

            // Okay, we've skipped modifiers and found an identifier followed
            // by an open curly (NOTE:  NOT a semicolon; abstract guys are
            // non-deterministic because 'get' and 'set' are valid identifiers),
            // and the identifier is 'get' or 'set', then we consider ourselves
            // as having hit an accessor.
            RescanToken (Mark(), &tok);
            if (tok.pName == m_pGetName || tok.pName == m_pSetName || tok.pName == m_pAddName || tok.pName == m_pRemoveName)
                return SB_ACCESSOR;
        }

        // Not an accessor...
        Rewind (iMark);
    }

    // The remaining logic in this function should closely match that in
    // ParseMember; it just doesn't construct a parse tree.

    // The [ token will land us here, but we don't look at attributes.  That means
    // if a member DOES has attributes, we'll miss them -- but we don't care, since
    // that only means an error occurred so we won't do anything with them anyway.
    // If we cared enough, we could scan backwards for attributes if we found a
    // member declaration -- but I don't think it's worth it.
    if (CurToken() == TID_OPENSQUARE)
        return SB_NOTAMEMBER;

    BOOL        fHadModifiers = FALSE;

    // Skip any modifiers, remembering if there were any
    while (m_rgTokenInfo[CurToken()].dwFlags & TFF_MODIFIER)
    {
        // Don't consider 'new' a modifier, but skip it as if it were one.  This
        // keeps "new foo();" from looking like a ctor.  Do the same thing for
        // 'unsafe', so we don't consider unsafe statements/expressions as members.
        if (CurToken() != TID_NEW && CurToken() != TID_UNSAFE)
            fHadModifiers = TRUE;
        NextToken();
    }

    // See if this looks like a constructor (or method w/ missing type)
    if (CurToken() == TID_IDENTIFIER && PeekToken() == TID_OPENPAREN)
    {
        BOOL    fHadParms = FALSE;

        // Skip to the open paren and scan an argument list.  If we don't
        // get a valid one, this is not a member.
        NextToken();
        if (!ScanParameterList (&fHadParms))
            return SB_NOTAMEMBER;

        // NOTE:  ScanParameterList leaves the current token at what follows
        // the close paren if it was successful...

        // If the parameter list had confirmed parameters, then this MUST
        // have been a method/ctor declaration.  Also, if followed by an
        // open curly, it could only have been a method/ctor.
        if (fHadParms || CurToken() == TID_OPENCURLY)
            return SB_TYPEMEMBER;

        // Check for ":[this|base](" -- which clearly indicates a constructor
        if (CurToken() == TID_COLON && (PeekToken(1) == TID_THIS || PeekToken(1) == TID_BASE) && PeekToken(2) == TID_OPENPAREN)
            return SB_TYPEMEMBER;

        // If followed by a semicolon, and there were modifiers,
        // it must have been a member
        if (CurToken() == TID_SEMICOLON && fHadModifiers)
            return SB_TYPEMEMBER;

        // This wasn't a member...
        return SB_NOTAMEMBER;
    }

    if (CurToken() == TID_TILDE)
    {
        // Check for destructor
        if (PeekToken(1) == TID_IDENTIFIER && PeekToken(2) == TID_OPENPAREN && PeekToken(3) == TID_CLOSEPAREN && PeekToken(4) == TID_OPENCURLY)
            return SB_TYPEMEMBER;
        return SB_NOTAMEMBER;
    }

    if (CurToken() == TID_CONST)
    {
        // If preceded by modifiers, this must be a member
        if (fHadModifiers)
            return SB_TYPEMEMBER;

        /*
        // See if this looks like a constant decl
        NextToken();
        if (ScanType() == NOT_TYPE)
            return SB_NOTAMEMBER;

        if (CurToken() == TID_IDENTIFIER)
            return SB_TYPEMEMBER;

        // REVIEW:  'const' can appear nowhere else -- should we always return SB_TYPEMEMBER?
        */
        // Can't tell a const member from a const local...
        return SB_NOTAMEMBER;
    }

    if (CurToken() == TID_EVENT)
    {
        // Events are always SB_TYPEMEMBER
        return SB_TYPEMEMBER;
    }

    if (m_rgTokenInfo[CurToken()].dwFlags & TFF_TYPEDECL)
    {
        // Nested types ALWAYS trigger SB_TYPEMEMBER
        return SB_TYPEMEMBER;
    }

    if ((CurToken() == TID_IMPLICIT || CurToken() == TID_EXPLICIT) && PeekToken() == TID_OPERATOR)
    {
        // Conversion operators -- look no further
        return SB_TYPEMEMBER;
    }

    // Everything else must have a [return] type
    if (ScanType() == NOT_TYPE)
        return SB_NOTAMEMBER;

    // Anything that had modifiers is considered a member
    if (fHadModifiers)
        return SB_TYPEMEMBER;

    // Operators...
    if (CurToken() == TID_OPERATOR)
        return SB_TYPEMEMBER;

    // Indexers...
    if (CurToken() == TID_THIS && PeekToken() == TID_OPENSQUARE)
        return SB_TYPEMEMBER;

    if (CurToken() == TID_IDENTIFIER || CurToken() == TID_DOT)
    {
        // Skip possibly qualified name
        if (CurToken() == TID_IDENTIFIER)
            NextToken();
        while (CurToken() == TID_DOT)
        {
            if (NextToken() != TID_IDENTIFIER)
            {
                if (CurToken() == TID_THIS && PeekToken() == TID_OPENSQUARE)
                    return SB_TYPEMEMBER;       // Explicit interface impl of indexer
                return SB_NOTAMEMBER;
            }
            NextToken();
        }

        if (CurToken() == TID_OPENPAREN)
        {
            BOOL    fHadArgs = FALSE;

            // See if this really is a method
            if (!ScanParameterList (&fHadArgs))
                return SB_NOTAMEMBER;

            if (CurToken() == TID_OPENCURLY)
                return SB_TYPEMEMBER;

            return SB_NOTAMEMBER;
        }

        if (CurToken() == TID_OPENCURLY)
        {
            // Check to see if this really looks like a property
            NextToken();
            while (m_rgTokenInfo[CurToken()].dwFlags & TFF_MODIFIER)
                NextToken();
            if (CurToken() == TID_IDENTIFIER)
            {
                FULLTOKEN   tok;

                RescanToken (Mark(), &tok);
                if (tok.pName == m_pGetName || tok.pName == m_pSetName || tok.pName == m_pAddName || tok.pName == m_pRemoveName)
                    return SB_TYPEMEMBER;
            }
            return SB_NOTAMEMBER;
        }
    }

    return SB_NOTAMEMBER;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ScanParameterList

BOOL CParser::ScanParameterList (BOOL *pfHadParms)
{
    // NOTE:  This function completely barfs on parameters with attributes.  Need
    // the ability to scan an expression, which we don't (yet) have...

    if (CurToken() != TID_OPENPAREN)
        return FALSE;

    NextToken();
    if (CurToken() != TID_CLOSEPAREN)
    {
        while (TRUE)
        {
            if (CurToken() == TID_ARGS)
            {
                NextToken();
                break;
            }

            if (CurToken() == TID_REF || CurToken() == TID_OUT)
                NextToken();

            SCANTYPE    st = ScanType();

            if (st == NOT_TYPE)
                return FALSE;

            if (CurToken() != TID_IDENTIFIER)
                return FALSE;

            if (st == MUST_BE_TYPE)
                *pfHadParms = TRUE;

            NextToken();
            if (CurToken() == TID_COMMA)
                NextToken();
            else
                break;
        }
    }

    if (CurToken() == TID_CLOSEPAREN)
    {
        NextToken();
        return TRUE;
    }

    return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ScanCast
//
// This function returns TRUE if the current token begins the following form:
//
//  ( <type> ) <term-or-unary-operator>

BOOL CParser::ScanCast ()
{
    Mark();

    if (CurToken() != TID_OPENPAREN)
        return FALSE;

    NextToken();
    SCANTYPE isType = ScanType();
    if (isType == NOT_TYPE)
        return FALSE;

    if (CurToken() != TID_CLOSEPAREN)
        return FALSE;

    if (isType == POINTER_OR_MULT)
        return TRUE;

    NextToken();

    //
    // check for ambiguous type or expression followed by disambiguating token
    //
    //                  - or -
    //
    // non-ambiguous type
    //
    if (((isType == TYPE_OR_EXPR) && !(m_rgTokenInfo[CurToken()].dwFlags & TFF_CASTEXPR)) ||
        (isType == MUST_BE_TYPE))
    {
        return TRUE;
    }

    return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ScanType
//
// Returns TRUE if current token begins a valid type construct (doesn't build a
// parse tree for it)

CParser::SCANTYPE CParser::ScanType ()
{
    SCANTYPE result = TYPE_OR_EXPR;
    if (CurToken() == TID_IDENTIFIER)
    {
        if (CurToken() == TID_IDENTIFIER)
            NextToken();

        // Scan a name
        while (CurToken() == TID_DOT)
        {
            if (NextToken() != TID_IDENTIFIER)
                return NOT_TYPE;
            NextToken();
        }
    }
    else if (m_rgTokenInfo[CurToken()].dwFlags & TFF_PREDEFINED)
    {
        // Simple type...
        NextToken();
        result = MUST_BE_TYPE;
    }
    else
    {
        // Can't be a type!
        return NOT_TYPE;
    }

    // Now check for pointer type(s)
    while (CurToken() == TID_STAR)
    {
        NextToken();
        if (result == TYPE_OR_EXPR)
            result = POINTER_OR_MULT;
    }

    // Finally, check for array type(s)
    while (CurToken() == TID_OPENSQUARE)
    {
        NextToken();
        if (CurToken() != TID_CLOSESQUARE)
        {
            while (TRUE)
            {
                if (CurToken() != TID_COMMA)
                    break;
                NextToken();
            }
        }

        if (CurToken() != TID_CLOSESQUARE)
            return NOT_TYPE;
        NextToken();
        result = MUST_BE_TYPE;
    }

    // If we get here, it's a valid type!
    return result;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ScanEscapeSequence
//
// Scans an escape sequence, found inside either a string or char literal.

WCHAR CParser::ScanEscapeSequence (PCWSTR &p, WCHAR *surrogatePair)
{
    WCHAR   ch = *p++;

    switch (ch)
    {

        case '\'':
        case '\"':
        case '\\':
            break;
        case 'a':   ch = '\a';  break;
        case 'b':   ch = '\b';  break;
        case 'f':   ch = '\f';  break;
        case 'n':   ch = '\n';  break;
        case 'r':   ch = '\r';  break;
        case 't':   ch = '\t';  break;
        case 'v':   ch = '\v';  break;
        case 'x':
        case 'u':
        {
            int     iChar = 0;

            if (!IsHexDigit (*p))
            {
                Error (ERR_IllegalEscape);
            }
            else
            {
                for (int i=0; i<4; i++)
                {
                    if (!IsHexDigit (*p))
                    {
                        if (ch == 'u')
                            Error (ERR_IllegalEscape);
                        break;
                    }
                    iChar = (iChar << 4) + HexValue (*p++);
                }

                ch = (WCHAR)iChar;
            }
            break;
        }
        case 'U':
        {
            unsigned int     uChar = 0;

            if (!IsHexDigit (*p))
            {
                Error (ERR_IllegalEscape);
            }
            else
            {
                for (int i=0; i<8; i++)
                {
                    if (!IsHexDigit (*p))
                    {
                        if (ch == 'U')
                            Error (ERR_IllegalEscape);
                        break;
                    }
                    uChar = (uChar << 4) + HexValue (*p++);
                }

                if (uChar < 0x00010000)
                {
                    ch = (WCHAR)uChar;
                    if (surrogatePair)
                        *surrogatePair = L'\0';
                }
                else if (!surrogatePair) // This means they only allow 1 character
                    Error(ERR_TooManyCharsInConst);
                else if (uChar > 0x0010FFFF)
                    Error(ERR_IllegalEscape);
                else
                {
                    ASSERT(uChar > 0x0000FFFF && uChar <= 0x0010FFFF && surrogatePair);
                    ch = (WCHAR)((uChar - 0x00010000) / 0x0400 + 0xD800);
                    *surrogatePair = (WCHAR)((uChar - 0x00010000) % 0x0400 + 0xDC00);
                }
            }
            break;
        }
        case '0':
            ch = '\0';
            break;
                
        case '\n':
        case '\r':
        case 0:
        case UCH_LS:
        case UCH_PS:
            if (*p != 0 && *p != '\r' && *p != '\n' && *p != UCH_PS && *p != UCH_LS) {
                p--; // move this back so that we don't eat the \n
            } // we want fallthrough here...
        default:
            Error (ERR_IllegalEscape);
            break;
    }

    return ch;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ScanStringLiteral
//
// Re-scans a TID_STRINGLIT token, and creates a STRCONST object containing the
// text of the string.

STRCONST *CParser::ScanStringLiteral (long iTokenIndex)
{
    FULLTOKEN       tok;
    PCWSTR          p, pszToken;
    WCHAR           ch, chEnd;

    // Rescan the token -- we need to know how long it is.
    RescanToken (iTokenIndex, &tok);
    pszToken = tok.pszToken;
    ASSERT (tok.iToken == TID_STRINGLIT);

    // Allocate space for the STRCONST...
    STRCONST    *pConst = (STRCONST *)MemAlloc (sizeof (STRCONST));

    p = pszToken;

    PWSTR   pszBuf = NULL;
    PWSTR   psz = NULL;
    PCWSTR  pszEnd = p + tok.iLength;

    // Allocate scan-into space.  Can't be any longer than the actual token.
    // Use the heap if the token is unusually long                                        
    if (tok.iLength >= MAX_POS_LINE_LEN)
        psz = pszBuf = (PWSTR)VSAlloc((tok.iLength + 1) * sizeof(WCHAR));
    else
        psz = pszBuf = STACK_ALLOC (WCHAR, tok.iLength + 1);

    ASSERT (*p == L'\"' || *p == L'@');
    if (*p == L'@')
    {
        p += 2;             // skip the @"
        for (;;)
        {
            while (p < pszEnd && *p != L'\"')
                *psz++ = *p++;

            // If it's a double "", continue copying (skipping the second one),
            // if it's a single ", we're done.
            if (p >= pszEnd || *++p != L'\"')
                break;
            else
                *psz++ = *p++;
        }

        *psz = 0;
    }
    else
    {
        chEnd = *p;

        // Skip the open quote
        p++;

        // Scan/copy up to end of string/line.  Translate escape sequences as we go.
        while (*p != chEnd && *p != 0 && *p != '\r' && *p != '\n' && *p != UCH_PS && *p != UCH_LS)
        {
            WCHAR ch2 = L'\0';
            ch = *p++;
            if (ch == '\\')
                ch = ScanEscapeSequence (p, &ch2);
            *psz++ = ch;
            if (ch2 != L'\0')
                *psz++ = ch2;
        }

        *psz = 0;
    }

    // Now, fill in the STRCONST structure
    pConst->length = (int)(psz - pszBuf);
    pConst->text = (WCHAR *)MemAlloc ((pConst->length + 1) * sizeof(WCHAR));
    memcpy (pConst->text, pszBuf, (pConst->length + 1) * sizeof(WCHAR));

    // We must free our temporary buffer if we alloced a big one
    if (tok.iLength >= MAX_POS_LINE_LEN)
        VSFree( pszBuf);

    // Fin!
    return pConst;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ScanCharLiteral
//
// Scan a character literal and return its value.

WCHAR CParser::ScanCharLiteral (long iTokenIndex)
{
    FULLTOKEN   tok;

    RescanToken (iTokenIndex, &tok);
    ASSERT (tok.iToken == TID_CHARLIT);

    /*
    if (tok.fError)
    {
        // Error means there was no closing quote
        Error (ERR_NewlineInConst);
        return 0;
    }
    */

    PCWSTR  p = tok.pszToken;
    WCHAR   ch, chRet;

    // Get/skip the open quote
    ch = *p++;
    ASSERT (ch == '\'');

    chRet = *p++;
    if (chRet == '\'')
    {
        Error (ERR_EmptyCharConst);
    }
    else
    {
        if (chRet == '\\')
            chRet = ScanEscapeSequence (p, NULL);
        if ((*p++) != '\'')
            Error (ERR_TooManyCharsInConst);
    }

    return chRet;
}

// Include the code stolen from COM+ for numeric conversions...
namespace NumLit
{
#include "numlit.h"
};

using namespace NumLit;

////////////////////////////////////////////////////////////////////////////////
// CParser::ScanNumericLiteral
//
// Re-scan a number token and convert it to a numeric value, filling in the
// provided CONSTVALNODE structure.

void CParser::ScanNumericLiteral (long iTokenIndex, CONSTVALNODE *pConst)
{
    FULLTOKEN       tok;

    RescanToken (iTokenIndex, &tok);
    ASSERT (tok.iToken == TID_NUMBER);

    // Create a working copy of the number (removing any escapes)
    PSTR    pszBuf = STACK_ALLOC (char, tok.iLength + 1);
    PSTR    psz = pszBuf;
    PCWSTR  p = tok.pszToken, pEnd = tok.pszToken + tok.iLength;
    WCHAR   ch = 0, chNonLower;
    BOOL    fReal = FALSE;
    BOOL    fExponent = FALSE;
    BOOL    fNonZero = FALSE;       // for floating point only
    BOOL    fUnsigned = FALSE;
    BOOL    fLong = FALSE;

    while (p < pEnd)
    {
        *psz++ = (char) NEXTCHAR (tok.pszToken, p, ch);
        switch (ch)
        {
        case '.':
            fReal = TRUE;
            break;
        case 'e':
        case 'E':
            fExponent = TRUE;
            break;
        case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
            fNonZero = fNonZero || !fExponent;
            break;
        }
    }

    *psz = 0;

    // At this point, ch will contain the suffix, if any.  Make it the lower-case version
    chNonLower = ch;  // save away possible uppercase version (just for warning purposes)
    ch |= 0x20;

    TOKENID iTypeTok = (TOKENID) -1;
    BOOL    fHex = (pszBuf[0] == '0' && (pszBuf[1] == 'x' || pszBuf[1] == 'X'));

    // For error reporting, determine the type name of this constant based on the
    // suffix
    switch (ch)
    {
        case 'd':
            if (!fHex)
            {
                iTypeTok = TID_DOUBLE;
                fReal = TRUE;
            }
            break;

        case 'f':
            if (!fHex)
            {
                iTypeTok = TID_FLOAT;
                fReal = TRUE;
            }
            break;

        case 'm':   iTypeTok = TID_DECIMAL; fReal = TRUE;       break;
        case 'l':
            fLong = TRUE;
            fUnsigned = (pszBuf + 2 <= psz && (psz[-2] | 0x20) == 'u');
            if (chNonLower == 'l' && !fUnsigned)
                Error(WRN_LowercaseEllSuffix); // warn about 'l' instead of 'L' (but not 'ul' -- that not a problem)
            break;
        case 'u':
            fUnsigned = TRUE;
            fLong = (pszBuf + 2 <= psz && (psz[-2] | 0x20) == 'l');
            if (fLong && psz[-2] == 'l')
                Error(WRN_LowercaseEllSuffix); // warn about 'l' instead of 'L'
            break;
        default:    iTypeTok = (!fHex && (fReal || fExponent)) ? TID_DOUBLE : TID_INT; break;
    }

    if (fReal && fHex)
    {
        Error (ERR_InvalidNumber);
        return;
    }

    if (fHex)
    {
        // Hex number.
        unsigned __int64     val = 0;

        for (PCSTR p = pszBuf+2; IsHexDigit (*p); p++)
        {
            if (val & UI64(0xf000000000000000))
            {
                Error (ERR_IntOverflow);
                break;
            }
            val = (val << 4) | HexValue (*p);
        }

        if (pszBuf[2] == '\0')
            Error (ERR_InvalidNumber);
        if (!fLong && ((val & UI64(0xFFFFFFFF00000000)) == 0)) {
            if (fUnsigned || (val & UI64(0xFFFFFFFF80000000))) {
                pConst->other = PT_UINT;
                iTypeTok = TID_UINT;
            } else {
                pConst->other = PT_INT;
                iTypeTok = TID_INT;
            }
            pConst->val.uiVal = (unsigned int)val;
        } else {
            if (fUnsigned || (val & UI64(0x8000000000000000))) {
                pConst->other = PT_ULONG;
                iTypeTok = TID_ULONG;
            } else {
                pConst->other = PT_LONG;
                iTypeTok = TID_LONG;
            }
            pConst->val.longVal = (__int64 *)MemAlloc (sizeof (__int64));
            *pConst->val.ulongVal = val;
        }

        return;
    }
    else if (fReal || fExponent)
    {
        // Floating point number.  Check the suffix.  If 'D', it's a decimal.
        // Otherwise, remove the suffix (if there) and scan it as a double,
        // and downcast to float if the suffix is 'f'.
        if (iTypeTok == TID_DECIMAL)
        {
            // It's a decimal.  Need to allocate space for it
            pConst->other = PT_DECIMAL;
            pConst->val.decVal = (DECIMAL *)MemAlloc (sizeof (DECIMAL));

            NUMBER num;
            psz = pszBuf;
            if (!::ParseNumber(&psz, &num) || ((*psz | 0x20) != 'm') || !::NumberToDecimal(&num, pConst->val.decVal))
            {
                Error(ERR_FloatOverflow, m_rgTokenInfo[iTypeTok].pszText);
            }
        }
        else
        {
            double dbl;
            bool errConvert;

            // Convert to wide characters -- all chars are in ASCII range so no need to use MultiByteToWideChar.
            PWSTR    pwszBuf  = STACK_ALLOC (WCHAR, strlen(pszBuf) + 1);
            PWSTR    pwszTemp = pwszBuf;
            PSTR     pszTemp  = pszBuf;

            char chLast = *(psz - 1);
            if (chLast == 'f' || chLast == 'F' || chLast == 'd' || chLast == 'D')
            {
                // remove the type suffix
                *(--psz) = 0;
            }

            while ((*pwszTemp++ = *pszTemp++) != 0)
                ;

            // Convert to a double, set errConvert on error. Note that we can't use atof here because it is sensitive to setlocale,
            // and we can't call setlocale ourselves because it is process wide instead of per-thread.                              
            if (FAILED(VarR8FromStr(pwszBuf, MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT), LOCALE_NOUSEROVERRIDE, &dbl))) {
                dbl = 0.0;
                errConvert = true;
            }
            else
                errConvert = false;


            if (iTypeTok == TID_FLOAT)
            {
                pConst->other = PT_FLOAT;
                // Allocate space for it.
                pConst->val.doubleVal = (double *)MemAlloc (sizeof (double));

                // Force to R4 precision/range.
                float f;
                RoundToFloat(dbl, &f);

                if (errConvert || !_finite(f))
                {
                    Error(ERR_FloatOverflow, m_rgTokenInfo[iTypeTok].pszText);
                }
                *(pConst->val.doubleVal) = f;
            }
            else
            {
                // It's a double.  Need to allocate space for it
                pConst->other = PT_DOUBLE;
                pConst->val.doubleVal = (double *)MemAlloc (sizeof (double));
                *(pConst->val.doubleVal) = dbl;
                if (errConvert || !_finite(dbl))
                {
                    Error(ERR_FloatOverflow, m_rgTokenInfo[iTypeTok].pszText);
                }
            }
        }

        return;
    }

    // Must be simple integral form.  We always have to 64bit math because literals can
    // be ints or longs
    unsigned __int64 val = 0;

    for (PCSTR p = pszBuf; *p >= '0' && *p <= '9'; p++)
    {
        if (val > UI64(1844674407370955161) || (val == UI64(1844674407370955161) && *p > '5'))
        {
            Error (ERR_IntOverflow);
            break;
        }
        val = val * 10 + (*p - '0');
    }

    if (fUnsigned) {
        if (fLong || (val & UI64(0xFFFFFFFF00000000))) {
            pConst->other = PT_ULONG;
            pConst->val.ulongVal = (unsigned __int64 *)MemAlloc (sizeof (unsigned __int64));
            *pConst->val.ulongVal = val;
            iTypeTok = TID_ULONG;
        } else {
            pConst->other = PT_UINT;
            pConst->val.uiVal = (unsigned int)val;
            iTypeTok = TID_UINT;
        }
    } else {
        if (!fLong && !(val & UI64(0xFFFFFFFF00000000))) {
            if ((val & UI64(0xFFFFFFFF80000000)) == 0) {
                pConst->other = PT_INT;
                pConst->val.iVal = (int)val;
                iTypeTok = TID_INT;
            } else {
                pConst->other = PT_UINT;
                pConst->val.uiVal = (unsigned int)val;
                iTypeTok = TID_UINT;
                if (val == UI64(0x0000000080000000))
                    pConst->flags |= NF_CHECK_FOR_UNARY_MINUS;
            }
        } else if ((val & UI64(0x8000000000000000)) == 0) {
            pConst->other = PT_LONG;
            pConst->val.longVal = (__int64 *)MemAlloc (sizeof (__int64));
            *pConst->val.longVal = (__int64)val;
            iTypeTok = TID_LONG;
        } else {
            pConst->other = PT_ULONG;
            pConst->val.ulongVal = (unsigned __int64 *)MemAlloc (sizeof (unsigned __int64));
            *pConst->val.ulongVal = val;
            iTypeTok = TID_ULONG;
            if (val == UI64(0x8000000000000000))
                pConst->flags |= NF_CHECK_FOR_UNARY_MINUS;
        }
    }
}


////////////////////////////////////////////////////////////////////////////////
// CParser::AllocNode
//
// This is the base node allocator.  Based on the node kind provided, the right
// amount of memory is allocated from the virtual allocation function MemAlloc.

BASENODE *CParser::AllocNode (NODEKIND kind)
{
    long    iSize;
#ifdef DEBUG
    PCSTR   pszNodeKind = "<unknown>";
#endif

    switch (kind)
    {
#ifdef DEBUG
        #define NODEKIND(n,s,g) case NK_##n: iSize = sizeof (s##NODE); pszNodeKind = #s "NODE (NK_" #n ")"; break;
#else
        #define NODEKIND(n,s,g) case NK_##n: iSize = sizeof (s##NODE); break;
#endif
        #include "nodekind.h"
        default:    VSFAIL ("Unknown node kind passed to AllocNode!"); iSize = sizeof (BASENODE); break;
    }

    BASENODE    *pNew = (BASENODE *)MemAlloc (iSize);

#ifdef DEBUG
    pNew->pszNodeKind = pszNodeKind;
#endif

    pNew->kind = kind;
    pNew->flags = 0;
    pNew->other = 0;
    pNew->tokidx = -1;
    return pNew;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::AllocDotNode

BINOPNODE *CParser::AllocDotNode (long iTokIdx, BASENODE *pParent, BASENODE *p1, BASENODE *p2)
{
    BINOPNODE   *pOp = (BINOPNODE *)AllocNode (NK_DOT);
    pOp->pParent = pParent;
    pOp->p1 = p1;
    pOp->p2 = p2;
    pOp->tokidx = iTokIdx;
    p1->pParent = p2->pParent = pOp;
    return pOp;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::AllocBinaryOpNode

BINOPNODE *CParser::AllocBinaryOpNode (OPERATOR op, long iTokIdx, BASENODE *pParent, BASENODE *p1, BASENODE *p2)
{
    BINOPNODE   *pOp = (BINOPNODE *)AllocNode (NK_BINOP);
    pOp->pParent = pParent;
    pOp->p1 = p1;
    pOp->p2 = p2;
    pOp->other = op;
    pOp->tokidx = iTokIdx;
    ASSERT (p1 != NULL);
    p1->pParent = pOp;
    if (p2 != NULL)
        p2->pParent = pOp;
    return pOp;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::AllocUnaryOpNode

UNOPNODE *CParser::AllocUnaryOpNode (OPERATOR op, long iTokIdx, BASENODE *pParent, BASENODE *p1)
{
    UNOPNODE    *pOp = (UNOPNODE *)AllocNode (NK_UNOP);
    pOp->pParent = pParent;
    pOp->p1 = p1;
    pOp->other = op;
    pOp->tokidx = iTokIdx;
    p1->pParent = pOp;
    return pOp;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::AllocOpNode

BASENODE *CParser::AllocOpNode (OPERATOR op, long iTokIdx, BASENODE *pParent)
{
    BASENODE    *pOp = (BASENODE *)AllocNode (NK_OP);
    pOp->pParent = pParent;
    pOp->other = op;
    pOp->tokidx = iTokIdx;
    return pOp;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::AllocNameNode

NAMENODE *CParser::AllocNameNode (NAME *pName, long iTokIdx)
{
    NAMENODE    *pNameNode = (NAMENODE *)AllocNode (NK_NAME);
    pNameNode->tokidx = iTokIdx;
    pNameNode->pName = pName;
    return pNameNode;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseSourceModule

BASENODE *CParser::ParseSourceModule ()
{
    // Not that it matters, but start the error count at 0
    m_iErrors = 0;

    // Source modules always start at the first token in any stream..
    m_iCurTok = 0;

    // Every source module belongs in an unnamed namespace...
    NAMESPACENODE   *pNS = (NAMESPACENODE *)AllocNode (NK_NAMESPACE);
    pNS->pParent = NULL;
    pNS->pName = NULL;
    pNS->pGlobalAttr = NULL;
    pNS->pKey = GetNameMgr()->GetPredefName (PN_EMPTY);
    pNS->iOpen = pNS->iClose = -1;

    // ...and the contents of the source is just a namespace body
    ParseNamespaceBody (pNS);

    // We're done!  Thus, we should be at end-of-file.
    pNS->iClose = Mark();
    if (CurToken() != TID_ENDFILE)
        Error (ERR_EOFExpected);

    return pNS;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseNamespaceBody

void CParser::ParseNamespaceBody (NAMESPACENODE *pNS)
{
    // A namespace body consists of zero or more using clauses followed by zero
    // or more namespace element declarations
    CListMaker   using_list(this, &pNS->pUsing);

    while (CurToken() == TID_USING)
        using_list.Add (ParseUsingClause (pNS));

    // The global (file level) namespace can also contain 'global' attributes
    if (NULL == pNS->pParent)
        pNS->pGlobalAttr = ParseGlobalAttributes(pNS);

    BOOL    fEndFound = FALSE;

    CListMaker elem_list(this, &pNS->pElements);
    while (!fEndFound)
    {
        TOKENID iTok = CurToken();

        if (m_rgTokenInfo[iTok].dwFlags & TFF_NSELEMENT)
        {
            // This token can start a type declaration
            elem_list.Add (ParseTypeDeclaration (pNS, NULL));
        }
        else if (iTok == TID_NAMESPACE)
        {
            // A nested namespace
            elem_list.Add (ParseNamespace (pNS));
        }
        else if (iTok == TID_CLOSECURLY || iTok == TID_ENDFILE)
        {
            // This token marks the end of a namespace body
            fEndFound = TRUE;
        }
        else if (iTok == TID_USING)
        {
            Error (ERR_UsingAfterElements);
            using_list.Add (ParseUsingClause (pNS));
        }
        else
        {
            // A few special error cases
            if ((iTok == TID_PRIVATE || iTok == TID_PROTECTED) &&
                (m_rgTokenInfo[PeekToken()].dwFlags & TFF_NSELEMENT ||
                PeekToken() == TID_NAMESPACE))
            {
                Error (ERR_NoNamespacePrivate);
                NextToken();
                // Next loop should hit ParseTypeDeclaration
            }
            else if ((iTok == TID_NEW) && m_rgTokenInfo[PeekToken()].dwFlags & TFF_NSELEMENT)
            {
                Error(ERR_NoNewOnNamespaceElement);
                NextToken();
            }
            else
            {
                // Whoops, the code is unrecognizable.  Give an error and try to get
                // sync'd up with intended reality
                Error (ERR_NamespaceUnexpected);
                fEndFound = !SkipToNamespaceElement ();
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseUsingClause

BASENODE *CParser::ParseUsingClause (BASENODE *pParent)
{
    ASSERT (CurToken() == TID_USING);

    // Create the using node, checking for namespace keyword
    USINGNODE   *pUsing = (USINGNODE *)AllocNode (NK_USING);
    pUsing->pParent = pParent;
    pUsing->tokidx = Mark();

    NextToken();

    if (CurToken() == TID_IDENTIFIER && PeekToken(1) == TID_EQUAL)
    {
        // This is a using-alias directive.
        pUsing->pAlias = ParseIdentifier (pUsing);
        Eat (TID_EQUAL);
    }
    else
    {
        // This is a using-namespace directive.
        pUsing->pAlias = NULL;
    }

    // Parse the name and eat the semicolon
    pUsing->pName = ParseName (pUsing);
    Eat (TID_SEMICOLON);
    return pUsing;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseGlobalAttributes

BASENODE *CParser::ParseGlobalAttributes (BASENODE *pParent)
{
    if (CurToken() != TID_OPENSQUARE || PeekToken(1) != TID_IDENTIFIER || PeekToken(2) != TID_COLON)
        return NULL;

    BASENODE *pAttr;
    CListMaker   list (this, &pAttr);

    FULLTOKEN ft;
    RescanToken(m_iCurTok + 1, &ft);
    while (ft.pName == GetNameMgr()->GetPredefName(PN_ASSEMBLY) || ft.pName == GetNameMgr()->GetPredefName(PN_MODULE))
    {
        list.Add(ParseAttributeSection(pParent, (ATTRLOC) 0, (ATTRLOC) 0));

        if (CurToken() != TID_OPENSQUARE || PeekToken(1) != TID_IDENTIFIER || PeekToken(2) != TID_COLON)
            break;
        RescanToken(m_iCurTok + 1, &ft);
    }

    return pAttr;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseNamespace

BASENODE *CParser::ParseNamespace (BASENODE *pParent)
{
    ASSERT (CurToken() == TID_NAMESPACE);

    // Create the namespace node
    NAMESPACENODE   *pNS = (NAMESPACENODE *)AllocNode (NK_NAMESPACE);
    pNS->tokidx = Mark();
    pNS->pParent = pParent;
    pNS->pGlobalAttr = NULL;
    pNS->pKey = NULL;

    // Grab the name
    NextToken();
    pNS->pName = ParseName (pNS);

    // Now there should be an open curly
    pNS->iOpen = Mark();
    Eat (TID_OPENCURLY);

    // Followed by a namespace body
    ParseNamespaceBody (pNS);

    // And lastly a close curly
    pNS->iClose = Mark();
    Eat (TID_CLOSECURLY);

    // optional trailing semi-colon
    if (CurToken() == TID_SEMICOLON)
        NextToken();

    AddToNodeTable (pNS);
    return pNS;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseTypeDeclaration

BASENODE *CParser::ParseTypeDeclaration (BASENODE *pParent, BASENODE *pAttr)
{
    long        tokidx = Mark();

    // All types can start with modifiers, so parse them now, IF we weren't given
    // any to start with (it's possible the caller already parsed them for us).
    // ParseAttributes will return NULL if there are none there...
    if (pAttr == NULL)
        pAttr = ParseAttributes (pParent, AL_TYPE, (ATTRLOC)0);
    else
        tokidx = pAttr->tokidx;
    
    // Type declarations can begin with modifiers 'public', 'private', 'sealed',
    // and 'abstract'.  Not all combinations are legal, but we parse them all
    // into a bitfield anyway.
    unsigned    iMods = ParseModifiers (pParent->kind == NK_NESTEDTYPE ? FALSE : TRUE);

    BASENODE    *pType = NULL;

    // The next token should be the one indicating what type to parse (class, enum,
    // struct, interface, or delegate)
    switch (CurToken())
    {
        case TID_CLASS:
        case TID_STRUCT:
            pAttr = SetDefaultAttributeLocation(pAttr, AL_TYPE, AL_TYPE);
            pType = ParseClassOrStruct (pParent, tokidx, pAttr, iMods);
            break;

        case TID_DELEGATE:
            pAttr = SetDefaultAttributeLocation(pAttr, AL_TYPE, (ATTRLOC) (AL_TYPE | AL_RETURN));
            pType = ParseDelegate (pParent, tokidx, pAttr, iMods);
            break;

        case TID_ENUM:
            pAttr = SetDefaultAttributeLocation(pAttr, AL_TYPE, AL_TYPE);
            pType = ParseEnum (pParent, tokidx, pAttr, iMods);
            break;

        case TID_INTERFACE:
            pAttr = SetDefaultAttributeLocation(pAttr, AL_TYPE, AL_TYPE);
            pType = ParseInterface (pParent, tokidx, pAttr, iMods);
            break;

        default:
            Error (ERR_BadTokenInType);
            break;
    }

    // NOTE:  If we didn't parse anything, we return NULL.  These are added to
    // lists using the CListMaker object, which deals with a new node of NULL and
    // doesn't mess with the current list.
    return pType;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseClassOrStruct

BASENODE *CParser::ParseClassOrStruct (BASENODE *pParent, long tokidx, BASENODE *pAttr, unsigned iMods)
{
    ASSERT (CurToken() == TID_CLASS || CurToken() == TID_STRUCT);

    CLASSNODE   *pClass = (CLASSNODE *)AllocNode (CurToken() == TID_CLASS ? NK_CLASS : NK_STRUCT);

    pClass->pParent = pParent;
    pClass->tokidx = tokidx;
    pClass->flags = iMods;
    pClass->pAttr = pAttr;
    pClass->pKey = NULL;
    if (pAttr != NULL)
        pAttr->pParent = pClass;
    NextToken();

    // Grab the name of the class/struct
    pClass->pName = ParseIdentifier (pClass);

    // Check for bases
    if (CurToken() == TID_COLON)
    {
        NextToken();
        pClass->pBases = ParseTypeNameList (pClass);
    }
    else
        pClass->pBases = NULL;

    // Parse class body
    pClass->iOpen = Mark();
    Eat (TID_OPENCURLY);

    BOOL        fEndFound = FALSE;
    MEMBERNODE  **ppNext = &pClass->pMembers;

    while (!fEndFound)
    {
        TOKENID iTok = CurToken();

        if (m_rgTokenInfo[iTok].dwFlags & TFF_MEMBER)
        {
            long    iTokIdx = Mark();

            // This token can start a member -- go parse it
            *ppNext = ParseMember (pClass);
            ASSERT ((*ppNext)->iClose != 0xcccccccc);
            ppNext = &(*ppNext)->pNext;

            // If we haven't advanced the token stream, skip
            // this token (an error will have been given)                              
            if (iTokIdx == Mark())
                NextToken();

            // If we didn't advance past any tokens, skip this one.
            // An error will have been reported                                
            if (iTokIdx == Mark())
                NextToken();
        }
        else if (iTok == TID_CLOSECURLY || iTok == TID_ENDFILE)
        {
            // This marks the end of members of this class
            fEndFound = TRUE;
        }
        else
        {
            // Error -- try to sync up with intended reality
            ErrorInvalidMemberDecl(Mark());
            fEndFound = !SkipToMember ();
        }
    }

    *ppNext = NULL;

    pClass->iClose = Mark();
    Eat (TID_CLOSECURLY);

    if (CurToken() == TID_SEMICOLON)
        NextToken();

    AddToNodeTable (pClass);
    return pClass;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseDelegate

BASENODE *CParser::ParseDelegate (BASENODE *pParent, long tokidx, BASENODE *pAttr, unsigned iMods)
{
    ASSERT (CurToken() == TID_DELEGATE);

    DELEGATENODE    *pDelegate = (DELEGATENODE *)AllocNode (NK_DELEGATE);

    pDelegate->pParent = pParent;
    pDelegate->tokidx = tokidx;
    pDelegate->flags = iMods;
    pDelegate->pAttr = pAttr;
    pDelegate->pKey = NULL;
    if (pAttr != NULL)
        pAttr->pParent = pDelegate;

    NextToken();
    pDelegate->pType = ParseReturnType (pDelegate);
    pDelegate->pName = ParseIdentifier (pDelegate);


    ParseParameterOrVarargsList(pDelegate, &pDelegate->pParms);

    pDelegate->iSemi = Mark();
    Eat (TID_SEMICOLON);

    AddToNodeTable (pDelegate);
    return pDelegate;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseEnum

BASENODE *CParser::ParseEnum (BASENODE *pParent, long tokidx, BASENODE *pAttr, unsigned iMods)
{
    ASSERT (CurToken() == TID_ENUM);

    CLASSNODE   *pEnum = (CLASSNODE *)AllocNode (NK_ENUM);

    pEnum->pParent = pParent;
    pEnum->tokidx = tokidx;
    pEnum->flags = iMods;
    pEnum->pAttr = pAttr;
    pEnum->pKey = NULL;
    if (pAttr != NULL)
        pAttr->pParent = pEnum;

    NextToken();
    pEnum->pName = ParseIdentifier (pEnum);
    if (CurToken() == TID_COLON)
    {
        NextToken();

        TYPENODE    *pBase = ParseType (pEnum);

        if ((pBase->TypeKind() != TK_PREDEFINED) ||
            (pBase->iType != PT_BYTE && pBase->iType != PT_SHORT &&
             pBase->iType != PT_INT && pBase->iType != PT_LONG &&
             pBase->iType != PT_SBYTE && pBase->iType != PT_USHORT &&
             pBase->iType != PT_UINT && pBase->iType != PT_ULONG))
        {
            Error (ERR_IntegralTypeExpected);
        }

        pEnum->pBases = pBase;
    }
    else
    {
        pEnum->pBases = NULL;
    }

    pEnum->iOpen = Mark();
    Eat (TID_OPENCURLY);

    MEMBERNODE  **ppNext = &pEnum->pMembers;

    while (CurToken() != TID_CLOSECURLY)
    {
        ENUMMBRNODE *pMbr = (ENUMMBRNODE *)AllocNode (NK_ENUMMBR);

        pMbr->pParent = pEnum;
        pMbr->tokidx = Mark();
        pMbr->pAttr = ParseAttributes (pMbr, AL_FIELD, AL_FIELD);
        pMbr->pName = ParseIdentifier (pMbr);
        if (CurToken() == TID_EQUAL)
        {
            NextToken();
            if (CurToken() == TID_COMMA || CurToken() == TID_CLOSECURLY)
            {
                Error (ERR_ConstantExpected);
                pMbr->pValue = NULL;
            }
            else
            {
                pMbr->pValue = ParseExpression (pMbr);
            }
        }
        else
        {
            pMbr->pValue = NULL;
        }

        pMbr->iClose = Mark();
        AddToNodeTable (pMbr);
        *ppNext = pMbr;
        ppNext = &pMbr->pNext;

        if (CurToken() != TID_COMMA)
        {
            pMbr->iClose--;     // Don't include the } as the close for the last member...
            break;
        }

        NextToken ();
    }

    *ppNext = NULL;
    pEnum->iClose = Mark();
    Eat (TID_CLOSECURLY);

    if (CurToken() == TID_SEMICOLON)
        NextToken();

    AddToNodeTable (pEnum);
    return pEnum;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseInterface

BASENODE *CParser::ParseInterface (BASENODE *pParent, long tokidx, BASENODE *pAttr, unsigned iMods)
{
    ASSERT (CurToken() == TID_INTERFACE);

    CLASSNODE   *pInterface = (CLASSNODE *)AllocNode (NK_INTERFACE);

    pInterface->pParent = pParent;
    pInterface->tokidx = tokidx;
    pInterface->flags = iMods;
    pInterface->pAttr = pAttr;
    pInterface->pKey = NULL;
    if (pAttr != NULL)
        pAttr->pParent = pInterface;

    NextToken();
    pInterface->pName = ParseIdentifier (pInterface);

    // Check for bases
    if (CurToken() == TID_COLON)
    {
        NextToken();
        pInterface->pBases = ParseTypeNameList (pInterface);
    }
    else
        pInterface->pBases = NULL;

    // Parse class body
    pInterface->iOpen = Mark();
    Eat (TID_OPENCURLY);

    BOOL        fEndFound = FALSE;
    MEMBERNODE  **ppNext = &pInterface->pMembers;

    while (!fEndFound)
    {
        TOKENID iTok = CurToken();

        if (m_rgTokenInfo[iTok].dwFlags & TFF_MEMBER)
        {
            // This token can start a member -- go parse it
            *ppNext = ParseMember (pInterface);
            ppNext = &(*ppNext)->pNext;
        }
        else if (iTok == TID_CLOSECURLY || iTok == TID_ENDFILE)
        {
            // This marks the end of members of this class
            fEndFound = TRUE;
        }
        else
        {
            // Error -- try to sync up with intended reality
            ErrorInvalidMemberDecl(Mark());
            fEndFound = !SkipToMember ();
        }
    }

    *ppNext = NULL;

    pInterface->iClose = Mark();
    Eat (TID_CLOSECURLY);

    if (CurToken() == TID_SEMICOLON)
        NextToken();

    AddToNodeTable (pInterface);
    return pInterface;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseMember

MEMBERNODE *CParser::ParseMember (CLASSNODE *pParent)
{
    long        tokidx = Mark();
    unsigned    iMods;
    BASENODE    *pAttr;
    bool        methodWithoutType = false;
    bool        isEvent = false;

    // Attributes are potentially at the beginning of everything here.  Parse 'em,
    // we'll get NULL back if there aren't any.
    pAttr = ParseAttributes (pParent, (ATTRLOC) 0, (ATTRLOC) 0);

    long        iMark = Mark();

    // All things here can start with modifiers -- parse them into a bitfield.
    iMods = ParseModifiers (TRUE);

    // Check for [static] constructor form
    if (CurToken() == TID_IDENTIFIER && PeekToken(1) == TID_OPENPAREN)
    {
        FULLTOKEN   tok;

        RescanToken (Mark(), &tok);
        if (tok.pName == pParent->pName->pName)
            return ParseConstructor (pParent, tokidx, pAttr, iMods);

        methodWithoutType = true;
    }

    // Check for destructor form
    if (CurToken() == TID_TILDE)
        return ParseDestructor (pParent, tokidx, pAttr, iMods);

    // Check for constant
    if (CurToken() == TID_CONST)
        return ParseConstant (pParent, tokidx, pAttr, iMods);

    // Check for event. Remember for later processing.
    if (CurToken() == TID_EVENT)
    {
        Eat (TID_EVENT);
        isEvent = true;
    }

    // It's valid to have a type declaration here -- check for those
    if ((m_rgTokenInfo[CurToken()].dwFlags & TFF_TYPEDECL) && !isEvent)
    {
        BOOL    fMissing;

        Rewind (iMark);
        NESTEDTYPENODE  *pNested = (NESTEDTYPENODE *)AllocNode (NK_NESTEDTYPE);
        pNested->pParent = pParent;
        pNested->pAttr = NULL;
        pNested->tokidx = tokidx;
        pNested->pType = ParseTypeDeclaration (pNested, pAttr);
        pNested->iClose = GetLastToken (pNested->pType, EF_ALL, &fMissing);
        return pNested;
    }

    // Check for conversion operators (implicit/explicit)
    // Also allow operator here so we can give a good error message
    if ((CurToken() == TID_IMPLICIT || CurToken() == TID_EXPLICIT || CurToken() == TID_OPERATOR) && !isEvent)
        return ParseOperator (pParent, tokidx, pAttr, iMods, NULL);

    // Everything that's left -- methods, fields, properties, and
    // non-conversion operators -- starts with a type (possibly void).  Parse one.
    // Unless we have a method without a return type (identifier followed by a '(')
    TYPENODE *pType = NULL;
    if (methodWithoutType)
    {
        Error(ERR_MemberNeedsType);
        pType = (TYPENODE *)AllocNode (NK_TYPE);
        pType->pParent = NULL;
        pType->tokidx = Mark();
        pType->other = TK_PREDEFINED;
        pType->iType = PT_VOID;
    }
    else
    {
        pType = ParseReturnType (NULL);
    }


    // Check here for operators
    // Allow old-style implicit/explicit casting operator syntax, just so we can give a better error
    if ((CurToken() == TID_OPERATOR || CurToken() == TID_IMPLICIT || CurToken() == TID_EXPLICIT) && !isEvent)
        return ParseOperator (pParent, tokidx, pAttr, iMods, pType);

    // Check here for indexers
    if (CurToken() == TID_THIS && !isEvent)
        return ParseProperty (NK_INDEXER, pParent, tokidx, pAttr, iMods, pType, false);

    // Okay -- Next is an identifier or '.' for fully
    // qualified explicit interface implementations
    if (CurToken() == TID_IDENTIFIER || CurToken() == TID_DOT)
    {
        TOKENID iTok;

        // find next token after a possibly qualified name
        long peekidx = (CurToken() == TID_IDENTIFIER) ? 1 : 0;
        while (((iTok = PeekToken(peekidx)) == TID_DOT) && (PeekToken(peekidx+1) == TID_IDENTIFIER))
            peekidx += 2;

        // only methods can have a ( after a name
        if (iTok == TID_OPENPAREN && !isEvent)
            return ParseMethod (pParent, tokidx, pAttr, iMods, pType);

        // At this point, we have a property if iTok is TID_OPENCURLY
        if (iTok == TID_OPENCURLY)
            return ParseProperty (NK_PROPERTY, pParent, tokidx, pAttr, iMods, pType, isEvent);

        // At this point an indexer has a .this [
        if (iTok == TID_DOT && (PeekToken(peekidx+1) == TID_THIS) && (PeekToken(peekidx+2) == TID_OPENSQUARE) && !isEvent)
            return ParseProperty (NK_INDEXER, pParent, tokidx, pAttr, iMods, pType, false);

        // Check for the situation where an incomplete explicit member is being entered.
        // In this case, the code may look something like "void ISomeInterface." followed
        // by either the end of the type of some other member (valid or not).  For statement
        // completion purposes, we need the name to be a potentially dotted name, so we
        // can't default into a field declaration (since it's name field is a NAMENODE).
        if (iTok == TID_DOT)
        {
            // Okay, whichever of these we call will fail -- however, they can handle
            // dotted names.
            if (isEvent)
                return ParseProperty (NK_PROPERTY, pParent, tokidx, pAttr, iMods, pType, isEvent);

            return ParseMethod (pParent, tokidx, pAttr, iMods, pType);
        }

        // Must be a field
        return ParseField (pParent, tokidx, pAttr, iMods, pType, isEvent);
    }

    // We don't know what it is yet, and it's incomplete.  Create a partial member
    // node to contain the type we parsed and return that.
    ErrorInvalidMemberDecl(Mark());
    PARTIALMEMBERNODE   *pMbr = (PARTIALMEMBERNODE *)AllocNode (NK_PARTIALMEMBER);
    pMbr->pParent = pParent;
    pMbr->tokidx = tokidx;
    pMbr->pNode = pType;
    if (pType != NULL)
        pType->pParent = pMbr;
    pMbr->pAttr = pAttr;
    if (pAttr != NULL)
        pAttr->pParent = pMbr;
    pMbr->iClose = Mark();
    return pMbr;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseConstructor

MEMBERNODE *CParser::ParseConstructor (CLASSNODE *pParent, long tokidx, BASENODE *pAttr, unsigned iMods)
{
    METHODNODE  *pCtor = (METHODNODE *)AllocNode (NK_CTOR);

    pCtor->pParent = pParent;
    pCtor->tokidx = tokidx;
    pCtor->flags = iMods;
    pCtor->pAttr = SetDefaultAttributeLocation(pAttr, AL_METHOD, AL_METHOD);
    if (pAttr != NULL)
        pAttr->pParent = pCtor;
    pCtor->pInteriorNode = NULL;
    pCtor->pName = NULL;

    FULLTOKEN   tok;

    // Current token is an identifier.  Rescan it, and check to see that the name
    // matches that of the class
    ASSERT (CurToken() == TID_IDENTIFIER);

    RescanToken (Mark(), &tok);
    ASSERT (tok.pName != NULL);
    ASSERT (tok.pName == pParent->pName->pName);
    pCtor->pType = NULL;
    NextToken();

    // Get parameters
    ParseParameterOrVarargsList(pCtor, &pCtor->pParms);

    // Check for :base(args) or :this(args)
    pCtor->pCtorArgs = NULL;
    if (CurToken() == TID_COLON)
    {
        TOKENID iTok = NextToken ();

        if (iTok == TID_BASE || iTok == TID_THIS)
        {
            pCtor->other |= (iTok == TID_BASE ? NFEX_CTOR_BASE : NFEX_CTOR_THIS);
            NextToken();
            pCtor->pCtorArgs = ParseArgumentList (pCtor);
        }
        else
        {
            if (iMods & NF_MOD_STATIC) // Static constructor can't have this or base
                Error (ERR_StaticConstructorWithExplicitConstructorCall, tok.pName->text);
            else
                Error (ERR_ThisOrBaseExpected);
        }
    }

    // Next should be the block.  Remember where it started for the interior parse.
    pCtor->iOpen = Mark();
    pCtor->pBody = NULL;
    if (CurToken() == TID_OPENCURLY || pCtor->other & (NFEX_CTOR_BASE | NFEX_CTOR_THIS))
    {
        SkipBlock (FALSE, &pCtor->iClose);
        if (CurToken() == TID_SEMICOLON)
        {
            pCtor->iClose++;
            NextToken();
        }
    }
    else
    {
        pCtor->iClose = Mark();
        Eat (TID_SEMICOLON);
        pCtor->other |= NFEX_METHOD_NOBODY;
    }

    AddToNodeTable (pCtor);
    return pCtor;
}


////////////////////////////////////////////////////////////////////////////////
// CParser::ParseDestructor

MEMBERNODE *CParser::ParseDestructor (CLASSNODE *pParent, long tokidx, BASENODE *pAttr, unsigned iMods)
{
    METHODNODE  *pDtor = (METHODNODE *)AllocNode (NK_DTOR);

    pDtor->pParent = pParent;
    pDtor->tokidx = tokidx;
    pDtor->flags = iMods;
    pDtor->pAttr = SetDefaultAttributeLocation(pAttr, AL_METHOD, AL_METHOD);
    pDtor->pType = NULL;
    pDtor->pCtorArgs = NULL;
    pDtor->pParms = NULL;
    if (pAttr != NULL)
        pAttr->pParent = pDtor;
    pDtor->pInteriorNode = NULL;

    // Current token is an identifier.  Rescan it, and check to see that the name
    // matches that of the class
    ASSERT (CurToken() == TID_TILDE);
    if (NextToken() != TID_IDENTIFIER)
    {
        CheckToken(TID_IDENTIFIER); // issue error.
    }
    else
    {
        FULLTOKEN   tok;

        RescanToken (Mark(), &tok);
        if (tok.pName != pParent->pName->pName)
            Error (ERR_BadDestructorName);
        NextToken();
    }

    // Get parameters
    Eat (TID_OPENPAREN);
    Eat (TID_CLOSEPAREN);

    // Next should be the block.  Remember where it started for the interior parse.
    pDtor->iOpen = Mark();
    pDtor->pBody = NULL;
    if (CurToken() == TID_OPENCURLY)
    {
        SkipBlock (FALSE, &pDtor->iClose);
    }
    else
    {
        pDtor->iClose = Mark();
        Eat (TID_SEMICOLON);
        pDtor->other |= NFEX_METHOD_NOBODY;
    }


    AddToNodeTable (pDtor);
    return pDtor;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseConstant

MEMBERNODE *CParser::ParseConstant (CLASSNODE *pParent, long tokidx, BASENODE *pAttr, unsigned iMods)
{
    FIELDNODE   *pConst = (FIELDNODE *)AllocNode (NK_CONST);
    BOOL fFirst = TRUE;

    pConst->pParent = pParent;
    pConst->tokidx = tokidx;
    pConst->flags = iMods;
    pConst->pAttr = SetDefaultAttributeLocation(pAttr, AL_FIELD, AL_FIELD);
    if (pAttr != NULL)
        pAttr->pParent = pConst;

    NextToken();
    pConst->pType = ParseType (pConst);

    CListMaker   list (this, &pConst->pDecls);

    while (TRUE)
    {
        BASENODE    *pVar = ParseVariableDeclarator (pConst, pConst, PVDF_CONST, fFirst);

        AddToNodeTable (pVar);
        list.Add (pVar);
        if (CurToken() != TID_COMMA)
            break;
        NextToken();
        fFirst = FALSE;
    }

    pConst->iClose = Mark();
    Eat (TID_SEMICOLON);
    return pConst;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseParameterOrVarargsList

void CParser::ParseParameterOrVarargsList(BASENODE * pParent, BASENODE ** ppParams)
{
    Eat (TID_OPENPAREN);

    BOOL        fGotParams = FALSE, fGaveError = FALSE;
    long        iErrorToken = -1;
    CListMaker  list (this, ppParams);

    if (CurToken() != TID_CLOSEPAREN)
    {
        while (TRUE)
        {
            // If this is 'params' or '__arglist', remember it for error reporting
            if (iErrorToken == -1 && (CurToken() == TID_PARAMS || CurToken() == TID_ARGS))
                iErrorToken = Mark();

            // Parse a parameter
            list.Add (ParseParameterOrVarargs (pParent));

            // Check for __arglist/params correctness.  This flag stays set in the
            // parent node, so this will catch any downstream paramters...
            if (pParent->other & (NFEX_METHOD_VARARGS | NFEX_METHOD_PARAMS))
            {
                if (!fGotParams)
                {
                    fGotParams = TRUE;
                }
                else if (!fGaveError)
                {
                    ErrorAtToken (iErrorToken, ERR_ParamsVarargsLast);
                    fGaveError = TRUE;
                }
            }

            if (CurToken() != TID_COMMA) 
                break;
            NextToken();
        }
    }

    Eat (TID_CLOSEPAREN);
}


////////////////////////////////////////////////////////////////////////////////
// CParser::ParseMethod

MEMBERNODE *CParser::ParseMethod (CLASSNODE *pParent, long tokidx, BASENODE *pAttr, unsigned iMods, TYPENODE *pType)
{
    METHODNODE  *pMethod = (METHODNODE *)AllocNode (NK_METHOD);

    // Store info given to us
    pMethod->pParent = pParent;
    pMethod->tokidx = tokidx;
    pMethod->flags = iMods;
    pMethod->pType = pType;
    if (pType != NULL)
        pType->pParent = pMethod;
    pMethod->pAttr = SetDefaultAttributeLocation(pAttr, AL_METHOD, (ATTRLOC) (AL_METHOD | AL_RETURN));
    if (pAttr != NULL)
        pAttr->pParent = pMethod;
    pMethod->pInteriorNode = NULL;

    // Parse the name (it could be qualified)
    pMethod->pName = ParseName (pMethod);

    // Get parameters
    ParseParameterOrVarargsList(pMethod, &pMethod->pParms);

    // Next should be the block.  Remember where it started for the interior parse.
    pMethod->iOpen = Mark();
    pMethod->pBody = NULL;
    if (CurToken() == TID_OPENCURLY)
    {
        SkipBlock (FALSE, &pMethod->iClose);
    }
    else
    {
        pMethod->iClose = Mark();
        Eat (TID_SEMICOLON);
        pMethod->other |= NFEX_METHOD_NOBODY;
    }

    AddToNodeTable (pMethod);
    return pMethod;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseProperty

MEMBERNODE *CParser::ParseProperty (NODEKIND nodekind, CLASSNODE *pParent, long tokidx, BASENODE *pAttr, unsigned iMods, TYPENODE *pType, bool isEvent)
{
    PROPERTYNODE    *pProp = (PROPERTYNODE *)AllocNode (nodekind);

    pProp->pParent = pParent;
    pProp->tokidx = tokidx;
    pProp->flags = iMods;
    pProp->pType = pType;
    pProp->other = isEvent ? NFEX_EVENT : 0;
    pType->pParent = pProp;
    pProp->pAttr = SetDefaultAttributeLocation(pAttr, (isEvent ? AL_EVENT : AL_PROPERTY), (isEvent ? (ATTRLOC) (AL_EVENT | AL_PROPERTY) : AL_PROPERTY));
    if (pAttr != NULL)
        pAttr->pParent = pProp;
    pProp->pParms = NULL;

    // parse name and parameter list
    if (nodekind == NK_PROPERTY)
    {
        pProp->pName = ParseName (pProp);
    }
    else
    {
        pProp->pName = ParseIndexerName(pProp);

        Eat(TID_OPENSQUARE);

        if (CurToken() != TID_CLOSESQUARE)
        {
            bool paramsLast = false;
            CListMaker   list (this, &pProp->pParms);

            // parse parameter list, must have at least one
            while (TRUE)
            {
                list.Add (ParseParameterOrVarargs (pProp));
                if ((pProp->other & (NFEX_METHOD_VARARGS | NFEX_METHOD_PARAMS)) && !paramsLast)
                    paramsLast = true;
                else 
                    paramsLast = false;
                if (CurToken() != TID_COMMA)
                    break;
                NextToken();
            }
            if ((pProp->other & (NFEX_METHOD_VARARGS | NFEX_METHOD_PARAMS)) && !paramsLast)
                Error (ERR_ParamsVarargsLast);
        }
        else
        {
            ErrorAtToken (Mark(), ERR_IndexerNeedsParam);
        }

        Eat(TID_CLOSESQUARE);
    }

    Eat (TID_OPENCURLY);

    pProp->pGet = pProp->pSet = NULL;

    for (long i=0; i<4; i++) // Go to 4 just incase some user has duplicates, so we give a better error
    {
        if (CurToken() == TID_CLOSECURLY)
            break;

        BASENODE *      pAccessorAttr = ParseAttributes (pProp, AL_METHOD, (ATTRLOC) 0);
        long            iTokAccessor = Mark();
        ACCESSORNODE    **ppAccessor = NULL;

        // Special error message for modifiers on accessors.
        while (m_rgTokenInfo[CurToken()].dwFlags & TFF_MODIFIER) {
            Error(ERR_NoModifiersOnAccessor);
            NextToken();
        }

        if (CurToken() == TID_IDENTIFIER)
        {
            FULLTOKEN   tok;

            RescanToken (Mark(), &tok);
            if (isEvent) {
                if (tok.pName == m_pRemoveName) {
                    ppAccessor = &pProp->pGet;
                }
                else if (tok.pName == m_pAddName) {
                    ppAccessor = &pProp->pSet;
                }
                else
                    Error (ERR_AddOrRemoveExpected);
            }
            else {
                // Regular property
                if (tok.pName == m_pGetName)
                    ppAccessor = &pProp->pGet;
                else if (tok.pName == m_pSetName)
                    ppAccessor = &pProp->pSet;
                else
                    Error (ERR_GetOrSetExpected);
            }

            NextToken();
        }
        else
        {
            Error (isEvent ? ERR_AddOrRemoveExpected : ERR_GetOrSetExpected);
        }

        if (ppAccessor != NULL)
        {
            if (*ppAccessor != NULL)
                Error (ERR_DuplicateAccessor);

            ACCESSORNODE    *pAccessor = (ACCESSORNODE *)AllocNode (NK_ACCESSOR);
            pAccessor->pAttr = SetDefaultAttributeLocation(pAccessorAttr, AL_METHOD, (ATTRLOC)((ppAccessor == &pProp->pGet) ? AL_METHOD | AL_RETURN : AL_METHOD | AL_RETURN | AL_PARAM));
            pAccessor->pParent = pProp;
            pAccessor->tokidx = iTokAccessor;
            pAccessor->flags = 0;
            pAccessor->pBody = NULL;
            pAccessor->iOpen = Mark();
            pAccessor->pInteriorNode = NULL;
            *ppAccessor = pAccessor;
        }

        if (CurToken() == TID_OPENCURLY)
        {
            SkipBlock (TRUE, ppAccessor == NULL ? NULL : &(*ppAccessor)->iClose);
        }
        else
        {
            if (isEvent)
                Error(ERR_AddRemoveMustHaveBody);

            if (ppAccessor != NULL)
            {
                (*ppAccessor)->other |= NFEX_METHOD_NOBODY;
                (*ppAccessor)->iClose = Mark();
            }

            if (CurToken() == TID_SEMICOLON) 
                NextToken();
            else
                ErrorAfterPrevToken(ERR_SemiOrLBraceExpected);
        }
    }

    pProp->iClose = Mark();
    if (CheckToken(TID_CLOSECURLY) && (NextToken() == TID_SEMICOLON)) {
        Error (ERR_UnexpectedSemicolon);
        NextToken();
    }
    AddToNodeTable (pProp);
    return pProp;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseField

MEMBERNODE *CParser::ParseField (CLASSNODE *pParent, long tokidx, BASENODE *pAttr, unsigned iMods, TYPENODE *pType, bool isEvent)
{
    FIELDNODE   *pField = (FIELDNODE *)AllocNode (NK_FIELD);
    BOOL        fFirst = TRUE;

    pField->pParent = pParent;
    pField->tokidx = tokidx;
    pField->pType = pType;
    pType->pParent = pField;
    pField->flags = iMods;
    pField->other = isEvent ? NFEX_EVENT : 0;
    pField->pAttr = SetDefaultAttributeLocation(pAttr, (isEvent ? AL_EVENT : AL_FIELD), (isEvent ? (ATTRLOC) (AL_EVENT | AL_FIELD | AL_METHOD) : AL_FIELD));
    if (pAttr != NULL)
        pAttr->pParent = pField;

    CListMaker   list (this, &pField->pDecls);

    while (TRUE)
    {
        BASENODE    *pVar = ParseVariableDeclarator (pField, pField, 0, fFirst);

        AddToNodeTable (pVar);
        list.Add (pVar);
        if (CurToken() != TID_COMMA)
            break;
        NextToken();
        fFirst = FALSE;
    }

    pField->iClose = Mark();
    if (isEvent && CurToken() == TID_DOT)
        Error(ERR_ExplicitEventFieldImpl);  // Better error message for confusing event situation.

    Eat (TID_SEMICOLON);
    return pField;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseOperator

MEMBERNODE *CParser::ParseOperator (CLASSNODE *pParent, long tokidx, BASENODE *pAttr, unsigned iMods, TYPENODE *pType)
{
    METHODNODE  *pOperator = (METHODNODE *)AllocNode (NK_OPERATOR);

    pOperator->pParent = pParent;
    pOperator->tokidx = tokidx;
    pOperator->pType = pType;
    pOperator->flags = iMods;
    pOperator->pAttr = SetDefaultAttributeLocation(pAttr, AL_METHOD, (ATTRLOC) (AL_METHOD | AL_RETURN));
    if (pAttr != NULL)
        pAttr->pParent = pOperator;
    pOperator->pInteriorNode = NULL;

    TOKENID iOpTok;
    long errorTokenIndex;

    if (CurToken() == TID_IMPLICIT || CurToken() == TID_EXPLICIT)
    {
        errorTokenIndex = m_iCurTok;
        iOpTok = CurToken();
        NextToken();                    // Skip to 'OPERATOR'
        if (CheckToken (TID_OPERATOR)   // ...and make sure it's there
            && pType != NULL)
        {
            ErrorAtToken( pType->tokidx, ERR_BadOperatorSyntax, m_rgTokenInfo[iOpTok].pszText);
        }
    }
    else
    {
        // In this case, 'OPERATOR' is the current token.
        ASSERT (CurToken() == TID_OPERATOR);
        iOpTok = NextToken ();
        errorTokenIndex = m_iCurTok;
        if (iOpTok == TID_IMPLICIT || iOpTok == TID_EXPLICIT)
        {
            ErrorAtToken( pType ? pType->tokidx : errorTokenIndex,
                ERR_BadOperatorSyntax,
                m_rgTokenInfo[iOpTok].pszText
                );
        }
    }

    // Skip past the operator token (we'll check it after we know unary vs. binary)
    NextToken ();

    // Only implicit/explicit casting operators need a type here
    if (pType == NULL)
    {
        if (iOpTok != TID_IMPLICIT && iOpTok != TID_EXPLICIT)
            ErrorAtToken( Mark(), ERR_BadOperatorSyntax2, m_rgTokenInfo[iOpTok].pszText);
        pType = ParseType (pOperator);
        pOperator->pType = pType;
    }
    else
    {
        pType->pParent = pOperator;
    }


    Eat (TID_OPENPAREN);

    CListMaker   list(this, &pOperator->pParms);
    long        iArgs = 0;

    if (CurToken() == TID_CLOSEPAREN)
    {
        pOperator->iOp = OP_QUESTION;
        goto BadArgCount;
    }

    while (iArgs < 2)
    {
        PARAMETERNODE   *pParm = (PARAMETERNODE *)AllocNode (NK_PARAMETER);
        pParm->pAttr = NULL;
        pParm->tokidx = Mark();
        pParm->pAttr = ParseAttributes (pParm, AL_PARAM, AL_PARAM);
        pParm->pParent = pOperator;
        pParm->pType = ParseType (pParm);
        pParm->pName = ParseIdentifier (pParm);
        list.Add (pParm);
        iArgs++;
        if (CurToken() != TID_COMMA || iArgs == 2)
            break;
        NextToken();
    }

    if (iArgs == 1)
    {
        pOperator->iOp = (OPERATOR)m_rgTokenInfo[iOpTok].iUnaryOp;
        if (!(m_rgTokenInfo[iOpTok].dwFlags & TFF_OVLUNOP))
            ErrorAtToken (errorTokenIndex, ERR_OvlUnaryOperatorExpected);
    }
    else
    {
        pOperator->iOp = (OPERATOR)m_rgTokenInfo[iOpTok].iBinaryOp;
        if (!(m_rgTokenInfo[iOpTok].dwFlags & TFF_OVLBINOP))
            ErrorAtToken (errorTokenIndex, ERR_OvlBinaryOperatorExpected);
    }

    if (CurToken() != TID_CLOSEPAREN) {
BadArgCount:
        if (!(m_rgTokenInfo[iOpTok].dwFlags & (TFF_OVLUNOP | TFF_OVLBINOP)))
        {
            if (iArgs == 0)
                ErrorAtToken (errorTokenIndex, ERR_OvlOperatorExpected );
            // else bad operator token already reported
        }
        else if (m_rgTokenInfo[iOpTok].dwFlags & TFF_OVLBINOP)
            Error( ERR_BadBinOpArgs, m_rgTokenInfo[iOpTok].pszText );
        else
            Error( ERR_BadUnOpArgs, m_rgTokenInfo[iOpTok].pszText );

        // Skip past the remaining parameters
        while (CurToken() == TID_COMMA)
        {
            NextToken();

            PARAMETERNODE   *pParm = (PARAMETERNODE *)AllocNode (NK_PARAMETER);
            pParm->pAttr = NULL;
            pParm->tokidx = Mark();
            pParm->pAttr = ParseAttributes (pParm, AL_PARAM, AL_PARAM);
            pParm->pType = ParseType (pParm);
            pParm->pName = ParseIdentifier (pParm);
        }
    }

    Eat (TID_CLOSEPAREN);
    pOperator->iOpen = Mark();
    pOperator->pBody = NULL;
    if (CurToken() == TID_SEMICOLON) {
        pOperator->iClose = Mark();
        Eat (TID_SEMICOLON);
        pOperator->other |= NFEX_METHOD_NOBODY;
    } else {
        SkipBlock(FALSE, &pOperator->iClose);
    }

    AddToNodeTable (pOperator);
    return pOperator;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseType

TYPENODE *CParser::ParseType (BASENODE *pParent)
{
    TYPENODE    *pType = (TYPENODE *)AllocNode (NK_TYPE);

    pType->tokidx = Mark();
    pType->pParent = pParent;

    if (m_rgTokenInfo[CurToken()].dwFlags & TFF_PREDEFINED)
    {
        // This is a predefined type
        pType->other = TK_PREDEFINED;
        pType->iType = m_rgTokenInfo[CurToken()].iPredefType;
        NextToken();
        if (pType->iType == PT_VOID) {
            if (pParent && pParent->kind == NK_PARAMETER && CurToken() != TID_STAR) {
                ErrorAtToken(pType->tokidx, ERR_NoVoidParameter);
                return pType;
            } else if (CurToken() != TID_STAR) {
                ErrorAtToken(pType->tokidx, ERR_NoVoidHere);
                return pType;
            }
            ASSERT(CurToken() == TID_STAR);
        }
    }
    else if (CurToken() == TID_IDENTIFIER)
    {
        // This is a named type
        pType->other = TK_NAMED;
        pType->pName = ParseName (pType);
    }
    else
    {
        // We needed a type here.  Give an error, and parse a missing name node.
        Error (ERR_TypeExpected);
        pType->other = TK_NAMED;
        pType->pName = ParseMissingName (NULL, pType, Mark());
        return pType;
    }

    // Check for pointer types (only if pType is NOT an array type)
    while (CurToken() == TID_STAR)
    {
        // This is a pointer type.
        if (pType->TypeKind() == TK_PREDEFINED && (pType->iType == PT_OBJECT || pType->iType == PT_STRING))
            Error (ERR_IllegalPointerType);

        TYPENODE    *pPtr = (TYPENODE *)AllocNode (NK_TYPE);
        pPtr->pParent = pParent;
        pPtr->tokidx = Mark();
        NextToken();
        pPtr->other = TK_POINTER;
        pPtr->pElementType = pType;
        pType->pParent = pPtr;
        pType = pPtr;
    }

    // Now, check for array dimension specifiers.  Arrays "read" from left to right, meaning
    // "int[][,][,,]" is a single-dim array of 2-dim arrays of 3-dim arrays.  This means the
    // tree should be a TYPE(array d1) -> TYPE(array d2) -> TYPE(array d3) -> TYPE(<previously parsed type>).
    // Basically this means building the tree from the top down (i.e., "backwards"), and when
    // done, hooking the above-parsed type to the leaf node of the array types.
    TYPENODE    *pArrayTop = NULL, *pArrayLast = NULL;

    while (CurToken() == TID_OPENSQUARE)
    {
        NextToken();
        TYPENODE    *pNewArray = (TYPENODE *)AllocNode (NK_TYPE);
        pNewArray->tokidx = Mark();
        pNewArray->other = TK_ARRAY;
        pNewArray->iDims = 1;
        if (pArrayLast != NULL)
        {
            pArrayLast->pElementType = pNewArray;
            pNewArray->pParent = pArrayLast;
        }
        else
        {
            pArrayTop = pNewArray;
        }

        if (CurToken() != TID_CLOSESQUARE)
        {
            while (CurToken() == TID_COMMA)
            {
                pNewArray->iDims++;
                NextToken();
            }
        }

        Eat (TID_CLOSESQUARE);
        pArrayLast = pNewArray;
    }

    if (pArrayTop != NULL)
    {
        ASSERT (pArrayLast != NULL);
        pArrayLast->pElementType = pType;
        pType->pParent = pArrayLast;
        pType = pArrayTop;
        pArrayTop->pParent = pParent;
    }

    return pType;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseClassType

TYPENODE *CParser::ParseClassType (BASENODE *pParent)
{
    TYPENODE    *pType = (TYPENODE *)AllocNode (NK_TYPE);

    pType->pParent = pParent;
    pType->tokidx = Mark();
    if (CurToken() == TID_OBJECT)
    {
        pType->iType = PT_OBJECT;
        pType->other = TK_PREDEFINED;
        NextToken();
    }
    else if (CurToken() == TID_STRING)
    {
        pType->iType = PT_STRING;
        pType->other = TK_PREDEFINED;
        NextToken();
    }
    else if (CurToken() == TID_IDENTIFIER)
    {
        pType->other = TK_NAMED;
        pType->pName = ParseName (pType);
    }
    else
    {
        Error (ERR_ClassTypeExpected);
        NextToken();
    }

    return pType;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseReturnType
//
// This is called from several places -- parsing the
// type of a field/method/property.  It is just ParseType(), except that it
// allows 'void' as a type.  Parent is optional, because at the time of the
// call the parent may not yet have been created.

TYPENODE *CParser::ParseReturnType (BASENODE *pParent)
{
    if (CurToken() == TID_VOID && PeekToken(1) != TID_STAR)
    {
        // Must be 'void' type, so create such a type node and return it.
        TYPENODE    *pType = (TYPENODE *)AllocNode (NK_TYPE);
        pType->pParent = pParent;
        pType->tokidx = Mark();
        pType->other = TK_PREDEFINED;
        pType->iType = PT_VOID;
        NextToken();
        return pType;
    }

    return ParseType (pParent);
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseTypeNameList

BASENODE *CParser::ParseTypeNameList (BASENODE *pParent)
{
    BASENODE    *pList;
    CListMaker   list (this, &pList);

    while (TRUE)
    {
        TYPENODE *pType = ParseType(pParent);
        if (pType->TypeKind() != TK_PREDEFINED && pType->TypeKind() != TK_NAMED)
        {
            ErrorAtToken (pType->tokidx, ERR_BadBaseType);
        }
        else
        {
            list.Add (pType);
        }
        if (CurToken() != TID_COMMA)
            break;
        NextToken();
    }

    return pList;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseIdentifier

NAMENODE *CParser::ParseIdentifier (BASENODE *pParent)
{
    NAMENODE    *pName = (NAMENODE *)AllocNode (NK_NAME);

    pName->tokidx = Mark();
    pName->pParent = pParent;

    if (!CheckToken (TID_IDENTIFIER))
        return ParseMissingName (pName, pParent, Mark());

    FULLTOKEN   tok;

    // Need the name, so rescan the token
    RescanToken (Mark(), &tok);
    pName->pName = tok.pName;
    NextToken();
    return pName;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseIdentifierOrKeyword
//
// Parses an identifier or a keyword (not a punctation) to a NAMENODE
//

NAMENODE *CParser::ParseIdentifierOrKeyword (BASENODE *pParent)
{
    ASSERT(CurToken() <= TID_IDENTIFIER);

    if (CurToken() == TID_IDENTIFIER)
    {
        return ParseIdentifier(pParent);
    }
    else
    {
        NAMENODE    *pName = (NAMENODE *)AllocNode (NK_NAME);

        pName->tokidx = Mark();
        pName->pParent = pParent;
        pName->pName = GetNameMgr()->KeywordName(CurToken());

        NextToken();

        return pName;
    }
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseName
//
// This function will parse a possibly fully-qualified name.

BASENODE *CParser::ParseName (BASENODE *pParent)
{
    NAMENODE    *pName = (NAMENODE *)AllocNode (NK_NAME);

    pName->tokidx = Mark();
    pName->pParent = pParent;

    if (!CheckToken (TID_IDENTIFIER))
        return ParseMissingName (pName, pParent, Mark());

    FULLTOKEN   tok;

    // Need the name, so rescan the token
    RescanToken (Mark(), &tok);
    pName->pName = tok.pName;
    NextToken();

    BASENODE    *pCur = pName;

    // Continue adding names while TID_DOTs exist
    while (CurToken() == TID_DOT)
    {
        long    iDotTokIdx = Mark();

        if (NextToken() != TID_IDENTIFIER)
        {
            // This is not an identifier.  Give an error and create a missing name node.
            CheckToken (TID_IDENTIFIER);    // This issues the error...
            return AllocDotNode (iDotTokIdx, pParent, pCur, ParseMissingName (NULL, pParent, Mark()));
        }

        // Must rescan the identifier to get the name
        RescanToken (Mark(), &tok);
        pName = AllocNameNode (tok.pName, Mark());
        pCur = AllocDotNode (iDotTokIdx, pParent, pCur, pName);
        NextToken();
    }

    ASSERT (pCur->pParent == pParent);
    return pCur;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseMissingName
//
// NOTE:  The name node may have already been allocated -- if not, pName is
// NULL and this function will allocate it.

NAMENODE *CParser::ParseMissingName (NAMENODE *pName, BASENODE *pParent, long iTokIndex)
{
    if (pName == NULL)
        pName = (NAMENODE *)AllocNode (NK_NAME);

    // Missing name nodes have a token index of the token PRIOR TO the token
    // where the name should have been.
    ASSERT (iTokIndex != 0);
    pName->tokidx = iTokIndex - 1;
    pName->pParent = pParent;
    pName->pName = m_pMissingName;
    pName->flags |= NF_NAME_MISSING;
    return pName;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseIndexerName
//
// This function will parse the name of an indexer property ( a name followed by .this)

BASENODE *CParser::ParseIndexerName (BASENODE *pParent)
{
    // a non-interface impl has a NULL name
    if (CurToken() == TID_THIS)
    {
        NextToken();
        return NULL;
    }

    // an interface impl has name.this
    NAMENODE    *pName = (NAMENODE *)AllocNode (NK_NAME);

    pName->tokidx = Mark();
    pName->pParent = pParent;

    if (!CheckToken (TID_IDENTIFIER))
        return ParseMissingName (pName, pParent, Mark());

    FULLTOKEN   tok;

    // Need the name, so rescan the token
    RescanToken (Mark(), &tok);
    pName->pName = tok.pName;
    NextToken();

    BASENODE    *pCur = pName;

    // Continue adding names while TID_DOTs exist
    while (CurToken() == TID_DOT && PeekToken() != TID_THIS)
    {
        long    iDotTokIdx = Mark();

        if (NextToken() != TID_IDENTIFIER)
        {
            // This is not an identifier.  Give an error and create a missing name node.
            CheckToken (TID_IDENTIFIER);    // This issues the error...
            return AllocDotNode (iDotTokIdx, pParent, pCur, ParseMissingName (NULL, pParent, Mark()));
        }

        // Must rescan the identifier to get the name
        RescanToken (Mark(), &tok);
        pName = AllocNameNode (tok.pName, Mark());
        pCur = AllocDotNode (iDotTokIdx, pParent, pCur, pName);
        NextToken();
    }

    Eat(TID_DOT);
    Eat(TID_THIS);

    ASSERT (pCur->pParent == pParent);
    return pCur;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseModifiers

unsigned CParser::ParseModifiers (BOOL bReportDups)
{
    unsigned    iMods = 0, iNewMod;
    bool        bNoDups = true, bNoDupAccess = true;

    while (TRUE)
    {
        switch (CurToken())
        {
            case TID_PRIVATE:   iNewMod = NF_MOD_PRIVATE;   break;
            case TID_PROTECTED: iNewMod = NF_MOD_PROTECTED; break;
            case TID_INTERNAL:  iNewMod = NF_MOD_INTERNAL;  break;
            case TID_PUBLIC:    iNewMod = NF_MOD_PUBLIC;    break;
            case TID_SEALED:    iNewMod = NF_MOD_SEALED;    break;
            case TID_ABSTRACT:  iNewMod = NF_MOD_ABSTRACT;  break;
            case TID_STATIC:    iNewMod = NF_MOD_STATIC;    break;
            case TID_VIRTUAL:   iNewMod = NF_MOD_VIRTUAL;   break;
            case TID_EXTERN:    iNewMod = NF_MOD_EXTERN;    break;
            case TID_NEW:       iNewMod = NF_MOD_NEW;       break;
            case TID_OVERRIDE:  iNewMod = NF_MOD_OVERRIDE;  break;
            case TID_READONLY:  iNewMod = NF_MOD_READONLY;  break;
            case TID_VOLATILE:  iNewMod = NF_MOD_VOLATILE;  break;

            case TID_UNSAFE:    iNewMod = NF_MOD_UNSAFE;    break;
            default:
                return iMods;
        }

        if (iMods & iNewMod && bReportDups)
        {
            if (bNoDups)
                Error (ERR_DuplicateModifier, GetTokenInfo(CurToken())->pszText);
            bNoDups = false;
        }
        else if ((iMods & NF_MOD_ACCESSMODIFIERS) && (iNewMod & NF_MOD_ACCESSMODIFIERS) && bReportDups)
        {
            // Can't have two different access modifiers.
            // Exception: "internal protected" or "protected internal" is allowed.
            if (! (((iNewMod == NF_MOD_PROTECTED) && (iMods & NF_MOD_INTERNAL)) || ((iNewMod == NF_MOD_INTERNAL) && (iMods & NF_MOD_PROTECTED))))
            {
                if (bNoDupAccess)
                    Error (ERR_BadMemberProtection);
                bNoDupAccess = false;
                iNewMod = 0;
            }
        }

        iMods |= iNewMod;
        NextToken();
    }
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseParameter

BASENODE *CParser::ParseParameter (BASENODE *pParent, bool noName)
{
    PARAMETERNODE   *pParm = (PARAMETERNODE *)AllocNode (NK_PARAMETER);

    pParm->pParent = pParent;
    pParm->tokidx = Mark();
    pParm->pAttr = ParseAttributes (pParm, AL_PARAM, AL_PARAM);

    // Parse parameter modifiers
    if (CurToken() == TID_REF)
    {
        pParm->flags = NF_PARMMOD_REF;
        NextToken();
    }
    else if (CurToken() == TID_OUT)
    {
        pParm->flags = NF_PARMMOD_OUT;
        NextToken();
    }

    // Parse a type
    pParm->pType = ParseType (pParm);

    if ((pParm->pType->TypeKind() == TK_PREDEFINED) &&
        pParm->pType->iType == PT_VOID && CurToken() == TID_CLOSEPAREN)
        return NULL;

    // ...and then a name
    if (!noName) {
        pParm->pName = ParseIdentifier (pParm);

        // When the user type "int foo[]", give them a useful error
        if (CurToken() == TID_OPENSQUARE && PeekToken() == TID_CLOSESQUARE) {
            ErrorAtToken( Mark(), ERR_BadArraySyntax);
            // Eat the "[]" so we can continue parsing
            NextToken();
            NextToken();
        }
    }

    return pParm;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseParameterOrVarargs

BASENODE *CParser::ParseParameterOrVarargs (BASENODE *pParent)
{
    BASENODE    *param;
    long        iTok = Mark();

    if (CurToken() == TID_ARGS)
    {
        NextToken();
        pParent->other |= NFEX_METHOD_VARARGS;
        return NULL;
    }

    if (CurToken() == TID_PARAMS)
    {
        NextToken();
        pParent->other |= NFEX_METHOD_PARAMS;
    }

    param = ParseParameter(pParent);
    if (param != NULL)
        param->tokidx = iTok;

    if (CurToken() == TID_EQUAL) {
        // User meant to put a default argument, probably. Give error
        // and eat expression.
        Error(ERR_NoDefaultArgs);
        NextToken(); // skip equals.
        ParseExpression(pParent); // skip expression
    }

    return param;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::SetDefaultAttributeLocation
//
// Sets the attribute location for attribute declarations which don't
// have locations set explicitly by the user
//

BASENODE *CParser::SetDefaultAttributeLocation(BASENODE *pAttrList, ATTRLOC defaultLocation, ATTRLOC validLocations)
{
    ASSERT(0 != defaultLocation);
    ASSERT(0 != validLocations);
    ASSERT(0 != (defaultLocation & validLocations));

    NODELOOP(pAttrList, ATTRDECL, pAttrSection)
        if (pAttrSection->location == 0)
        {
            pAttrSection->location = defaultLocation;
        }
        else if (!(validLocations & pAttrSection->location))
        {
            // build up valid locations string
            WCHAR szValidLocations[256];
            szValidLocations[0] = 0;
            ATTRLOC validTemp = validLocations;
            for (int i = 0; validTemp != 0; i += 1, (validTemp = (ATTRLOC) (validTemp >> 1)))
            {
                if (validTemp & 1)
                {
                    if (szValidLocations[0])
                    {
                        wcscat(szValidLocations, L", ");
                    }
                    wcscat(szValidLocations, GetNameMgr()->GetAttrLoc(i)->text);
                }
            }

            ErrorAtToken(pAttrSection->tokidx, ERR_AttributeLocationOnBadDeclaration, pAttrSection->pNameNode->pName->text, szValidLocations);
            pAttrSection->location = defaultLocation;
        }
    ENDLOOP

    return pAttrList;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseAttributes
//
// This function checks for the '[' token indicating attributes, and if found,
// returns the parse tree for them -- if not found, it returns NULL without
// error, because all attributes are optional.

BASENODE *CParser::ParseAttributes (BASENODE *pParent, ATTRLOC defaultLocation, ATTRLOC validLocations)
{
    if (CurToken() != TID_OPENSQUARE)
        return NULL;

    BASENODE    *pList;
    CListMaker   list (this, &pList);
    BASENODE    *pSection;
    long        iTok = Mark();

    while (NULL != (pSection = ParseAttributeSection(pParent, defaultLocation, validLocations)))
    {
        list.Add(pSection);
    }

    if (pList != NULL && pList->tokidx == -1)
        pList->tokidx = iTok;

    return pList;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseAttributeSection
//
// This function parses a single [ position-opt : attribute-list ] block

BASENODE *CParser::ParseAttributeSection (BASENODE *pParent, ATTRLOC defaultLocation, ATTRLOC validLocations)
{
    if (CurToken() != TID_OPENSQUARE)
        return NULL;

    ATTRDECLNODE    *pAttr = (ATTRDECLNODE *)AllocNode (NK_ATTRDECL);

    pAttr->pParent = pParent;
    pAttr->tokidx = Mark();
    pAttr->location = (ATTRLOC) 0;
    pAttr->pNameNode = NULL;

    Eat (TID_OPENSQUARE);

    //
    // Check for optional position :
    //
    if (CurToken() <= TID_IDENTIFIER && PeekToken() == TID_COLON)
    {
        pAttr->pNameNode = ParseIdentifierOrKeyword(pAttr);
        Eat(TID_COLON);
        if (!GetNameMgr()->IsAttrLoc(pAttr->pNameNode->pName, (int*) &pAttr->location))
        {
            Error(ERR_InvalidAttributeLocation, pAttr->pNameNode->pName->text);
            pAttr->location = (ATTRLOC) 0;
        }
        else if (validLocations != 0)
        {
            SetDefaultAttributeLocation(pAttr, defaultLocation, validLocations);
        }
    }

    //
    // set default location if available
    //
    if (pAttr->location == 0)
    {
        pAttr->pNameNode = NULL;
        pAttr->location = defaultLocation;
    }

    //
    // Attribute List
    //
    CListMaker       list (this, &pAttr->pAttr);
    while (TRUE)
    {
        list.Add (ParseAttribute (pAttr));
        if (CurToken() != TID_COMMA)
            break;
        NextToken();
        if (CurToken() == TID_CLOSESQUARE)
            break;
    }
    Eat(TID_CLOSESQUARE);

    pAttr->iClose = Mark() - 1;      // -1 because we've advanced past it...
    return pAttr;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseAttribute

BASENODE *CParser::ParseAttribute (BASENODE *pParent)
{
    ATTRNODE    *pAttr = (ATTRNODE *)AllocNode (NK_ATTR);

    pAttr->pParent = pParent;
    pAttr->tokidx = Mark();

    pAttr->pName = ParseName (pAttr);

    CListMaker   list (this, &pAttr->pArgs);

    pAttr->iOpen = pAttr->iClose = -1;
    if (CurToken() == TID_OPENPAREN)
    {
        pAttr->iOpen = Mark();
        NextToken();

        long        iTokOpen = Mark(), iTokIdx = iTokOpen;
        BOOL        fNamed = FALSE;
        long        iErrors = m_iErrors;

        if (CurToken() != TID_CLOSEPAREN)
        {
            while (TRUE)
            {
                BASENODE    *pNew = list.Add (ParseAttributeArgument (pAttr, &fNamed));

                if (iTokIdx != iTokOpen)
                    pNew->tokidx = iTokIdx;

                if (CurToken() == TID_COMMA)
                    iTokIdx = Mark();
                else
                    break;

                NextToken ();
            }
        }

        pAttr->iClose = Mark();
        Eat (TID_CLOSEPAREN);
        if (iErrors != m_iErrors)
            pAttr->flags |= NF_CALL_HADERROR;
    }

    return pAttr;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseAttributeArgument

BASENODE *CParser::ParseAttributeArgument (BASENODE *pParent, BOOL *pfNamed)
{
    ATTRNODE    *pArg = (ATTRNODE *)AllocNode (NK_ATTRARG);

    pArg->pParent = pParent;
    pArg->tokidx = Mark();
    pArg->iOpen = pArg->iClose = -1;

    // Check for named argument -- identifier followed by '='
    if (CurToken() == TID_IDENTIFIER && PeekToken (1) == TID_EQUAL)
    {
        pArg->pName = ParseIdentifier (pArg);
        Eat (TID_EQUAL);
        *pfNamed = TRUE;
    }
    else
    {
        // Not named -- give an error if it's supposed to be
        if (*pfNamed)
            Error (ERR_NamedArgumentExpected);
        pArg->pName = NULL;
    }

    pArg->pArgs = ParseExpression (pArg);
    return pArg;
}

STATEMENTNODE *CParser::CallStatementParser (DWORD dwStmtParserKind, BASENODE *pParent)
{
    switch(dwStmtParserKind)
    {
    case EParseBreakStatement:
      return ParseBreakStatement(pParent);
    case EParseTryStatement:
      return ParseTryStatement(pParent);
    case EParseCheckedStatement:
      return ParseCheckedStatement(pParent);
    case EParseConstStatement:
      return ParseConstStatement(pParent);
    case EParseDoStatement:
      return ParseDoStatement(pParent);
    case EParseFixedStatement:
      return ParseFixedStatement(pParent);
    case EParseForStatement:
      return ParseForStatement(pParent);
    case EParseForEachStatement:
      return ParseForEachStatement(pParent);
    case EParseGotoStatement:
      return ParseGotoStatement(pParent);
    case EParseIfStatement:
      return ParseIfStatement(pParent);
    case EParseLockStatement:
      return ParseLockStatement(pParent);
    case EParseReturnStatement:
      return ParseReturnStatement(pParent);
    case EParseSwitchStatement:
      return ParseSwitchStatement(pParent);
    case EParseThrowStatement:
      return ParseThrowStatement(pParent);
    case EParseUsingStatement:
      return ParseUsingStatement(pParent);
    case EParseWhileStatement:
      return ParseWhileStatement(pParent);
    case EParseBlock:
      return ParseBlock(pParent);
    default:
      _ASSERTE(!"Invalid state");
      return NULL;
    }
}
////////////////////////////////////////////////////////////////////////////////
// CParser::ParseStatement

STATEMENTNODE *CParser::ParseStatement (BASENODE *pParent)
{
    TOKENID         iTok = CurToken();
    STATEMENTNODE   *pStmt;
    long            iErrorCount = m_iErrors;

    // Check for statements that are parsed by entries in the statement parse function table.
    // These include block, selection, iteration, jump, and try statements.
    if (m_rgTokenInfo[iTok].dwStmtParserKind != 0)
    {
        pStmt = CallStatementParser(m_rgTokenInfo[iTok].dwStmtParserKind, pParent);
    }
    else if (iTok == TID_IDENTIFIER && PeekToken(1) == TID_COLON)
    {
        // Labeled statement
        pStmt = ParseLabeledStatement (pParent);
    }
    else if (iTok == TID_SEMICOLON)
    {
        // Empty statement
        pStmt = (STATEMENTNODE *)AllocNode (NK_EMPTYSTMT);
        pStmt->pParent = pParent;
        pStmt->tokidx = Mark();
        NextToken();
    }
    else
    {
        long    iMark = Mark();
        SCANTYPE st = ScanType();

        // Declaration statement
        if ((st == MUST_BE_TYPE && CurToken() != TID_DOT) || ((st != NOT_TYPE) && CurToken() == TID_IDENTIFIER))
        {
            Rewind (iMark);
            pStmt = ParseDeclarationStatement (pParent);
        }
        else
        {
            // Must be an expression statement
            Rewind (iMark);
            pStmt = ParseExpressionStatement (pParent);
        }
    }

    pStmt->pNext = NULL;
    if (iErrorCount != m_iErrors)
        pStmt->flags |= NF_STMT_HADERROR;
    return pStmt;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseBlock

STATEMENTNODE *CParser::ParseBlock (BASENODE *pParent)
{
    BLOCKNODE   *pBlock = (BLOCKNODE *)AllocNode (NK_BLOCK);

    long iClose;
    switch (pParent->kind) {
    case NK_ACCESSOR:
        iClose = pParent->asACCESSOR()->iClose;
        break;

    case NK_METHOD:
    case NK_CTOR:
    case NK_DTOR:
    case NK_OPERATOR:
        iClose = pParent->asANYMETHOD()->iClose;
        break;

    case NK_INDEXER:
    case NK_PROPERTY:
        iClose = pParent->asANYPROPERTY()->iClose;
        break;

    default:
        iClose = -1;
    }

    pBlock->pParent = pParent;
    pBlock->tokidx = Mark();
    pBlock->pNext = NULL;
    pBlock->pStatements = NULL;

    Eat (TID_OPENCURLY);

    STATEMENTNODE   **ppNext = &pBlock->pStatements;

    while (CurToken() != TID_ENDFILE)
    {
        if ((iClose != -1 && Mark() >= iClose) ||
            (iClose == -1 &&
            (CurToken() == TID_CLOSECURLY || CurToken() == TID_FINALLY || CurToken() == TID_CATCH)))
            break;
        if (CurToken() == TID_CLOSECURLY) {
            Eat (TID_OPENCURLY);
            NextToken();
            continue;
        }
        *ppNext = ParseStatement (pBlock);
        ppNext = &(*ppNext)->pNext;
        ASSERT (*ppNext == NULL);
    }

    pBlock->iClose = Mark();
    Eat (TID_CLOSECURLY);
    return pBlock;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseBreakStatement
//
// NOTE:  Handles both 'break' and 'continue' statements

STATEMENTNODE *CParser::ParseBreakStatement (BASENODE *pParent)
{
    ASSERT (CurToken() == TID_BREAK || CurToken() == TID_CONTINUE);

    EXPRSTMTNODE    *pStmt = (EXPRSTMTNODE *)AllocNode (CurToken() == TID_BREAK ? NK_BREAK : NK_CONTINUE);
    pStmt->pParent = pParent;
    pStmt->tokidx = Mark();
    NextToken();
    pStmt->pArg = NULL;
    Eat (TID_SEMICOLON);
    return pStmt;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseConstStatement

STATEMENTNODE *CParser::ParseConstStatement (BASENODE *pParent)
{
    Eat(TID_CONST);

    DECLSTMTNODE    *pStmt = (DECLSTMTNODE *)AllocNode (NK_DECLSTMT);
    pStmt->pParent = pParent;
    pStmt->tokidx = Mark();
    pStmt->pType = ParseType (pStmt);
    pStmt->flags |= NF_CONST_DECL;

    BOOL         fFirst = TRUE;
    CListMaker   list(this, &pStmt->pVars);
    while (TRUE)
    {
        list.Add (ParseVariableDeclarator (pStmt, pStmt, PVDF_CONST, fFirst));
        if (CurToken() != TID_COMMA)
            break;
        NextToken();
        fFirst = FALSE;
    }
    Eat (TID_SEMICOLON);
    pStmt->pNext = NULL;
    return pStmt;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseLabeledStatement

STATEMENTNODE *CParser::ParseLabeledStatement (BASENODE *pParent)
{
    ASSERT (CurToken() == TID_IDENTIFIER);

    LABELSTMTNODE   *pStmt = (LABELSTMTNODE *)AllocNode (NK_LABEL);

    pStmt->pParent = pParent;
    pStmt->tokidx = Mark();
    pStmt->pLabel = ParseIdentifier (pStmt);
    Eat (TID_COLON);
    pStmt->pStmt = ParseStatement (pStmt);
    return pStmt;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseDoStatement

STATEMENTNODE *CParser::ParseDoStatement (BASENODE *pParent)
{
    ASSERT (CurToken() == TID_DO);

    LOOPSTMTNODE    *pStmt = (LOOPSTMTNODE *)AllocNode (NK_DO);
    pStmt->pParent = pParent;
    pStmt->tokidx = Mark();
    NextToken();
    pStmt->pStmt = ParseEmbeddedStatement (pStmt, false);
    Eat (TID_WHILE);
    Eat (TID_OPENPAREN);
    pStmt->pExpr = ParseExpression (pStmt);
    Eat (TID_CLOSEPAREN);
    Eat (TID_SEMICOLON);
    return pStmt;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseForStatement

STATEMENTNODE *CParser::ParseForStatement (BASENODE *pParent)
{
    ASSERT (CurToken() == TID_FOR);

    FORSTMTNODE     *pStmt = (FORSTMTNODE *)AllocNode (NK_FOR);
    pStmt->pParent = pParent;
    pStmt->tokidx = Mark();
    NextToken();
    Eat (TID_OPENPAREN);

    long    iMark = Mark();

    // Here can be either a declaration or an expression statement list.  Scan
    // for a declaration first.
    if ((ScanType() != NOT_TYPE) && CurToken() == TID_IDENTIFIER)
    {
        Rewind (iMark);
        pStmt->pInit = ParseDeclarationStatement (pStmt);     // NOTE:  This eats the semicolon...
    }
    else
    {
        // Not a type followed by an identifier, so it must be an expression list.
        Rewind (iMark);
        if (CurToken() == TID_SEMICOLON)
        {
            pStmt->pInit = NULL;
            NextToken();
        }
        else
        {
            pStmt->pInit = ParseExpressionList (pStmt);
            Eat (TID_SEMICOLON);
        }
    }

    pStmt->pExpr = (CurToken() != TID_SEMICOLON) ? ParseExpression (pStmt) : NULL;
    Eat (TID_SEMICOLON);
    pStmt->pInc = (CurToken() != TID_CLOSEPAREN) ? ParseExpressionList (pStmt) : NULL;
    Eat (TID_CLOSEPAREN);
    pStmt->pStmt = ParseEmbeddedStatement (pStmt, true);
    return pStmt;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseForEachStatement

STATEMENTNODE *CParser::ParseForEachStatement (BASENODE *pParent)
{
    ASSERT (CurToken() == TID_FOREACH);

    FORSTMTNODE     *pStmt = (FORSTMTNODE *)AllocNode (NK_FOR);
    pStmt->pParent = pParent;
    pStmt->tokidx = Mark();
    pStmt->flags = NF_FOR_FOREACH;
    pStmt->pInc = NULL;
    NextToken();
    Eat (TID_OPENPAREN);

    // Syntax for foreach is:
    //  foreach ( <type> <identifier> in <expr> ) <embedded-statement>
    //
    // ...so first, parse the declaration.  Note that it is NOT a
    // declaration-statement, since it cannot have an initializer.  Create a
    // DECLSTMTNODE by hand.
    DECLSTMTNODE    *pDecl = (DECLSTMTNODE *)AllocNode (NK_DECLSTMT);
    pDecl->pParent = pStmt;
    pDecl->tokidx = Mark();
    pDecl->pType = ParseType (pDecl);
    pDecl->pNext = NULL;

    if (CurToken() == TID_IN) {
        Error (ERR_BadForeachDecl);
    }

    VARDECLNODE *pVar = (VARDECLNODE *)AllocNode (NK_VARDECL);
    pVar->pArg = NULL;
    pVar->pParent = pDecl;
    pVar->pDecl = pDecl;
    pVar->tokidx = Mark();
    pVar->pName = ParseIdentifier (pVar);
    pDecl->pVars = pVar;
    pStmt->pInit = pDecl;

    FULLTOKEN   tok;

    // Next, eat the 'in' and the expression 
    RescanToken (Mark(), &tok);
    if (CurToken() != TID_IN)
        Error (ERR_InExpected);
    else
        NextToken();

    pStmt->pExpr = ParseExpression (pStmt);
    Eat (TID_CLOSEPAREN);
    pStmt->pStmt = ParseEmbeddedStatement (pStmt, true);
    return pStmt;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseGotoStatement

STATEMENTNODE *CParser::ParseGotoStatement (BASENODE *pParent)
{
    ASSERT (CurToken() == TID_GOTO);

    EXPRSTMTNODE    *pStmt = (EXPRSTMTNODE *)AllocNode (NK_GOTO);
    pStmt->pParent = pParent;
    pStmt->tokidx = Mark();
    NextToken();
    if (CurToken() == TID_CASE || CurToken() == TID_DEFAULT)
    {
        TOKENID tok = CurToken();

        pStmt->flags = NF_GOTO_CASE;
        NextToken();
        pStmt->pArg = (tok == TID_CASE) ? ParseExpression (pStmt) : NULL;
    }
    else
    {
        pStmt->pArg = ParseIdentifier (pStmt);
    }

    Eat (TID_SEMICOLON);
    return pStmt;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseIfStatement

STATEMENTNODE *CParser::ParseIfStatement (BASENODE *pParent)
{
    ASSERT (CurToken() == TID_IF);

    IFSTMTNODE      *pStmt = (IFSTMTNODE *)AllocNode (NK_IF);
    pStmt->pParent = pParent;
    pStmt->tokidx = Mark();
    NextToken();
    Eat (TID_OPENPAREN);
    pStmt->pExpr = ParseExpression (pStmt);
    Eat (TID_CLOSEPAREN);
    pStmt->pStmt = ParseEmbeddedStatement (pStmt, false);
    if (CurToken() == TID_ELSE)
    {
        NextToken();
        pStmt->pElse = ParseEmbeddedStatement (pStmt, false);
    }
    else
    {
        pStmt->pElse = NULL;
    }
    return pStmt;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseReturnStatement

STATEMENTNODE *CParser::ParseReturnStatement (BASENODE *pParent)
{
    ASSERT (CurToken() == TID_RETURN);

    EXPRSTMTNODE    *pStmt = (EXPRSTMTNODE *)AllocNode (NK_RETURN);
    pStmt->pParent = pParent;
    pStmt->tokidx = Mark();
    NextToken();
    if (CurToken() == TID_SEMICOLON)
        pStmt->pArg = NULL;
    else
        pStmt->pArg = ParseExpression (pStmt);
    Eat (TID_SEMICOLON);
    return pStmt;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseSwitchStatement

STATEMENTNODE *CParser::ParseSwitchStatement (BASENODE *pParent)
{
    ASSERT (CurToken() == TID_SWITCH);

    SWITCHSTMTNODE  *pStmt = (SWITCHSTMTNODE *)AllocNode (NK_SWITCH);
    pStmt->pParent = pParent;
    pStmt->tokidx = Mark();
    NextToken();
    Eat (TID_OPENPAREN);
    pStmt->pExpr = ParseExpression (pStmt);
    Eat (TID_CLOSEPAREN);
    pStmt->iOpen = Mark();
    Eat (TID_OPENCURLY);

    CListMaker   list (this, &pStmt->pCases);

    if (CurToken() == TID_CLOSECURLY)
        Error (WRN_EmptySwitch);
    while (CurToken() != TID_CLOSECURLY && CurToken() != TID_ENDFILE)
        list.Add (ParseSwitchCase (pStmt));
    pStmt->iClose = Mark();
    Eat (TID_CLOSECURLY);
    return pStmt;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseThrowStatement

STATEMENTNODE *CParser::ParseThrowStatement (BASENODE *pParent)
{
    ASSERT (CurToken() == TID_THROW);

    EXPRSTMTNODE    *pStmt = (EXPRSTMTNODE *)AllocNode (NK_THROW);
    pStmt->pParent = pParent;
    pStmt->tokidx = Mark();
    NextToken();
    pStmt->pArg = (CurToken() != TID_SEMICOLON) ? ParseExpression (pStmt) : NULL;
    Eat (TID_SEMICOLON);
    return pStmt;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseTryStatement

STATEMENTNODE *CParser::ParseTryStatement (BASENODE *pParent)
{
    TRYSTMTNODE     *pStmt = (TRYSTMTNODE *)AllocNode (NK_TRY);
    pStmt->pParent = pParent;
    pStmt->tokidx = Mark();

    switch (CurToken())
    {
    case TID_TRY:
        // valid try/catch/finally construct
        NextToken ();
        pStmt->pBlock = ParseBlock (pStmt)->asBLOCK();
        break;
    case TID_CATCH:
    case TID_FINALLY:
        // catch/finally without try!
        Eat(TID_TRY);   // Give the "Expected 'try'" error
        pStmt->pBlock = NULL;
        break;
    default:
        ASSERT(!"CurToken() not try, catch, or finally");
    }

    CListMaker   list (this, &pStmt->pCatch);
    if (CurToken() == TID_CATCH)
    {
        BOOL    fEmpty = FALSE;
        BASENODE * pLastAdded = NULL;

        while (CurToken() == TID_CATCH)
        {
            pLastAdded = ParseCatchClause (pStmt, &fEmpty);
            list.Add (pLastAdded);
        }

        pStmt->flags |= NF_TRY_CATCH;

        if (CurToken() == TID_FINALLY)
        {
            TRYSTMTNODE *pInnerTry = pStmt;

            pStmt = (TRYSTMTNODE *)AllocNode(NK_TRY);
            pStmt->pParent = pParent;
            pStmt->tokidx = pInnerTry->tokidx;

            BLOCKNODE   *pBlock = (BLOCKNODE *)AllocNode (NK_BLOCK);
            pBlock->pParent = pStmt;
            pBlock->tokidx = pStmt->tokidx;
            pBlock->pNext = NULL;
            pBlock->pStatements = pInnerTry;
            pBlock->iClose = pLastAdded->asCATCH()->pBlock->iClose;

            pInnerTry->pParent = pBlock;
            pInnerTry->pNext = NULL;
            pStmt->pBlock = pBlock;

            NextToken();
            pStmt->pCatch = ParseBlock (pStmt);
            pStmt->flags |= NF_TRY_FINALLY;
        }
    }
    else if (CurToken() == TID_FINALLY)
    {
        NextToken();
        pStmt->pCatch = ParseBlock (pStmt);
        pStmt->flags |= NF_TRY_FINALLY;
    }
    else
    {
        Error (ERR_ExpectedEndTry);
    }

    return pStmt;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseWhileStatement

STATEMENTNODE *CParser::ParseWhileStatement (BASENODE *pParent)
{
    ASSERT (CurToken() == TID_WHILE);

    LOOPSTMTNODE    *pStmt = (LOOPSTMTNODE *)AllocNode (NK_WHILE);
    pStmt->pParent = pParent;
    pStmt->tokidx = Mark();
    NextToken();
    Eat (TID_OPENPAREN);
    pStmt->pExpr = ParseExpression (pStmt);
    Eat (TID_CLOSEPAREN);
    pStmt->pStmt = ParseEmbeddedStatement (pStmt, true);
    return pStmt;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseDeclaration

STATEMENTNODE *CParser::ParseDeclaration (BASENODE *pParent)
{
    DECLSTMTNODE    *pStmt = (DECLSTMTNODE *)AllocNode (NK_DECLSTMT);
    pStmt->pParent = pParent;
    pStmt->tokidx = Mark();
    pStmt->pType = ParseType (pStmt);

    BOOL         fFirst = TRUE;
    CListMaker   list(this, &pStmt->pVars);
    while (TRUE)
    {
        list.Add (ParseVariableDeclarator (pStmt, pStmt, PVDF_LOCAL, fFirst));
        if (CurToken() != TID_COMMA)
            break;
        NextToken();
        fFirst = FALSE;
    }
    pStmt->pNext = NULL;
    return pStmt;

}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseDeclarationStatement

STATEMENTNODE *CParser::ParseDeclarationStatement (BASENODE *pParent)
{
    STATEMENTNODE * node = ParseDeclaration (pParent);
    Eat (TID_SEMICOLON);
    return node;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseExpressionStatement

STATEMENTNODE *CParser::ParseExpressionStatement (BASENODE *pParent)
{
    EXPRSTMTNODE    *pStmt = (EXPRSTMTNODE *)AllocNode (NK_EXPRSTMT);
    pStmt->pParent = pParent;
    pStmt->tokidx = Mark();
    pStmt->pArg = ParseExpression (pStmt);
    Eat (TID_SEMICOLON);
    pStmt->pNext = NULL;
    return pStmt;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseLockStatement

STATEMENTNODE *CParser::ParseLockStatement (BASENODE *pParent)
{
    ASSERT (CurToken() == TID_LOCK);

    LOOPSTMTNODE    *pStmt = (LOOPSTMTNODE *)AllocNode (NK_LOCK);
    pStmt->pParent = pParent;
    pStmt->tokidx = Mark();
    NextToken ();
    Eat (TID_OPENPAREN);
    pStmt->pExpr = ParseExpression (pStmt);
    Eat (TID_CLOSEPAREN);
    pStmt->pStmt = ParseEmbeddedStatement (pStmt, false);
    return pStmt;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseFixedStatement

STATEMENTNODE *CParser::ParseFixedStatement (BASENODE *pParent)
{
    FORSTMTNODE     *pStmt = (FORSTMTNODE *)AllocNode (NK_FOR);

    pStmt->flags |= NF_FIXED_DECL;
    pStmt->pParent = pParent;
    pStmt->tokidx = Mark();
    NextToken();
    Eat (TID_OPENPAREN);

    pStmt->pInit = ParseDeclaration (pStmt);
    pStmt->pInit->flags |= NF_FIXED_DECL;

    pStmt->pExpr = NULL;
    pStmt->pInc = NULL;

    Eat (TID_CLOSEPAREN);

    pStmt->pStmt = ParseEmbeddedStatement (pStmt, false);
    pStmt->pNext = NULL;

    return pStmt;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseUsingStatement

STATEMENTNODE *CParser::ParseUsingStatement (BASENODE *pParent)
{
    FORSTMTNODE     *pStmt = (FORSTMTNODE *)AllocNode (NK_FOR);

    pStmt->flags |= NF_USING_DECL;
    pStmt->pParent = pParent;
    pStmt->tokidx = Mark();
    NextToken();
    Eat (TID_OPENPAREN);

    // Now, this can be either an expression or a decl list

    long    iMark = Mark();
    SCANTYPE st = ScanType();

    if ((st == MUST_BE_TYPE && CurToken() != TID_DOT) || ((st != NOT_TYPE) && CurToken() == TID_IDENTIFIER))
    {
        Rewind (iMark);
        pStmt->pInit = ParseDeclaration (pStmt);
        pStmt->pInit->flags |= NF_USING_DECL;
        pStmt->pExpr = NULL;
    }
    else
    {
        // Must be an expression statement
        Rewind (iMark);
        pStmt->pExpr = ParseExpression(pStmt);
        pStmt->pInit = NULL;
    }

    pStmt->pInc = NULL;

    Eat (TID_CLOSEPAREN);

    pStmt->pStmt = ParseEmbeddedStatement (pStmt, false);
    pStmt->pNext = NULL;

    return pStmt;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseCheckedStatement
STATEMENTNODE *CParser::ParseCheckedStatement (BASENODE *pParent)
{
    ASSERT (CurToken() == TID_CHECKED || CurToken() == TID_UNCHECKED || CurToken() == TID_UNSAFE);

    LABELSTMTNODE   *pStmt = (LABELSTMTNODE *)AllocNode (CurToken() == TID_UNSAFE ? NK_UNSAFE : NK_CHECKED);

    pStmt->pParent = pParent;
    pStmt->tokidx = Mark();
    pStmt->pLabel = NULL;
    if (CurToken() == TID_UNCHECKED)
        pStmt->flags |= NF_UNCHECKED;
    NextToken();
    pStmt->pStmt = ParseBlock (pStmt);
    return pStmt;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseEmbeddedStatement

STATEMENTNODE *CParser::ParseEmbeddedStatement (BASENODE *pParent, BOOL fComplexCheck)
{
    if (CurToken() == TID_SEMICOLON)
    {
        if (!fComplexCheck || PeekToken() == TID_OPENCURLY)
        {
            Error (WRN_PossibleMistakenNullStatement);
        }
    }

    // An "embedded" statement is simply a statement that is not a labelled
    // statement or a declaration statement.  Parse a normal statement and post-
    // check for the error case.
    STATEMENTNODE   *pStmt = ParseStatement (pParent);

    if (pStmt != NULL)
    {
        if (pStmt->kind == NK_LABEL || pStmt->kind == NK_DECLSTMT)
            ErrorAtToken (pStmt->tokidx, ERR_BadEmbeddedStmt);
    }

    return pStmt;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseVariableDeclarator
//
// fFirst indicates whether this is the first, or subsequent variable declaration
// in a list. Currently this is just used to report a better error message.

BASENODE *CParser::ParseVariableDeclarator (BASENODE *pParent, BASENODE *pParentDecl, DWORD dwFlags, BOOL fFirst)
{
    VARDECLNODE *pVar = (VARDECLNODE *)AllocNode (NK_VARDECL);
    pVar->pParent = pParent;
    pVar->pDecl = pParentDecl;
    pVar->tokidx = Mark();
    pVar->pName = ParseIdentifier (pVar);
    pVar->pArg = NULL;

    switch (CurToken())
    {
    case TID_EQUAL:
        pVar->flags |= NF_VARDECL_EXPR;
        Eat (TID_EQUAL);
        pVar->pArg = ParseVariableInitializer (pVar, (dwFlags & (PVDF_LOCAL|PVDF_CONST)) == PVDF_LOCAL);
        pVar->pArg = AllocBinaryOpNode (OP_ASSIGN, pVar->tokidx, pVar, AllocNameNode (pVar->pName->pName, pVar->pName->tokidx), pVar->pArg);
        break;

    case TID_OPENPAREN:     // Special case for C-style constructors
        Error (ERR_BadVarDecl);
        pVar->pArg = ParseArgumentList (pVar);
        break;

    case TID_OPENSQUARE:
        Error (ERR_CStyleArray);
        // buffers have been postponed to post Version 1
        if ((dwFlags & PVDF_CONST) == 0)
        {
            NextToken();
            pVar->flags |= NF_VARDECL_ARRAY;
            pVar->pArg = ParseExpression (pVar);
            Eat (TID_CLOSESQUARE);
            break;
        }
        goto DEFAULTCASE;  // else handle with the default processing.
        
    case TID_IDENTIFIER:
        if (! fFirst) {
            // Give better error message in the case where the user did something like:
            // using (X x = expr1, Y y = expr2) ...
            Error(ERR_MultiTypeInDeclaration);
            NextToken();
            if (CurToken() == TID_EQUAL) {
                NextToken();
                ParseVariableInitializer(pVar, TRUE); // just eat the initializer, if any, to prevent cascading errors.
            }            
            break;
        }
        
        goto DEFAULTCASE; // else handle with the default processing.
        
    default:
DEFAULTCASE:    
        if (dwFlags & PVDF_CONST)
            Error (ERR_ConstValueRequired);  // Error here for missing constant initializers
        pVar->pArg = NULL;
    }

    return pVar;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseVariableInitializer

BASENODE *CParser::ParseVariableInitializer (BASENODE *pParent, BOOL fAllowStackAlloc)
{
    if (fAllowStackAlloc && CurToken() == TID_STACKALLOC)
        return ParseStackAllocExpression (pParent);

    if (CurToken() == TID_OPENCURLY)
        return ParseArrayInitializer (pParent);

    return ParseExpression (pParent);
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseArrayInitializer

BASENODE *CParser::ParseArrayInitializer (BASENODE *pParent)
{
    UNOPNODE    *pInit = (UNOPNODE *)AllocNode (NK_ARRAYINIT);
    pInit->pParent = pParent;
    pInit->tokidx = Mark();

    Eat (TID_OPENCURLY);

    // NOTE:  This loop allows " { <initexpr>, } " but not " { , } "
    CListMaker list (this, &pInit->p1);
    while (CurToken() != TID_CLOSECURLY)
    {
        list.Add (ParseVariableInitializer (pInit, FALSE));
        if (CurToken() != TID_COMMA)
            break;
        NextToken();
    }

    Eat (TID_CLOSECURLY);
    return pInit;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseStackAllocExpression

BASENODE *CParser::ParseStackAllocExpression (BASENODE *pParent)
{
    ASSERT (CurToken() == TID_STACKALLOC);

    NEWNODE     *pNew = (NEWNODE *)AllocNode (NK_NEW);
    pNew->pParent = pParent;
    pNew->tokidx = Mark();
    pNew->flags |= NF_NEW_STACKALLOC;

    Eat (TID_STACKALLOC);

    pNew->pType = (TYPENODE *)AllocNode (NK_TYPE);
    pNew->pType->tokidx = Mark();
    pNew->pType->pParent = pNew;
    pNew->pInit = NULL;
    pNew->pArgs = NULL;
    pNew->iOpen = pNew->iClose = -1;

    if (m_rgTokenInfo[CurToken()].dwFlags & TFF_PREDEFINED)
    {
        // This is a predefined type
        pNew->pType->other = TK_PREDEFINED;
        pNew->pType->iType = m_rgTokenInfo[CurToken()].iPredefType;
        NextToken();
        if (pNew->pType->iType == PT_VOID)
            CheckToken (TID_STAR);
    }
    else if (CurToken() == TID_IDENTIFIER)
    {
        // This is a named type
        pNew->pType->other = TK_NAMED;
        pNew->pType->pName = ParseName (pNew->pType);
    }
    else
    {
        // Should have been one or the other!
        Error (ERR_TypeExpected);
        pNew->pType->other = TK_NAMED;
        pNew->pType->pName = ParseMissingName (NULL, pNew->pType, Mark());
        return pNew;
    }

    // Check for pointer types
    while (CurToken() == TID_STAR)
    {
        // This is a pointer type.
        if (pNew->pType->TypeKind() == TK_PREDEFINED && (pNew->pType->iType == PT_OBJECT || pNew->pType->iType == PT_STRING))
            Error (ERR_IllegalPointerType);

        TYPENODE    *pPtr = (TYPENODE *)AllocNode (NK_TYPE);
        pPtr->pParent = pNew;
        pPtr->tokidx = Mark();
        NextToken();
        pPtr->other = TK_POINTER;
        pPtr->pElementType = pNew->pType;
        pNew->pType->pParent = pPtr;
        pNew->pType = pPtr;
    }

    // Here, we MUST have a '[' in order to be valid.
    if (CurToken() != TID_OPENSQUARE)
    {
        Error (ERR_BadStackAllocExpr);
        return pNew;
    }

    // The return type of this stackalloc is actually a pointer to the type we just parsed.
    TYPENODE    *pPointerType = (TYPENODE *)AllocNode (NK_TYPE);
    pPointerType->tokidx = Mark();
    pPointerType->pParent = pNew;
    pPointerType->other = TK_POINTER;
    pPointerType->pElementType = pNew->pType;
    pNew->pType->pParent = pPointerType;
    pNew->pType = pPointerType;

    Eat (TID_OPENSQUARE);
    pNew->pArgs = ParseExpression (pNew);
    Eat (TID_CLOSESQUARE);

    return pNew;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseSwitchCase

BASENODE *CParser::ParseSwitchCase (BASENODE *pParent)
{
    CASENODE    *pCase = (CASENODE *)AllocNode (NK_CASE);
    pCase->pParent = pParent;
    pCase->tokidx = Mark();

    if (CurToken() != TID_CASE && CurToken() != TID_DEFAULT)
        Error (ERR_StmtNotInCase);

    CListMaker   list (this, &pCase->pLabels);

    // First, parse case label(s)
    while (CurToken() == TID_CASE || CurToken() == TID_DEFAULT)
    {
        UNOPNODE    *pLabel = (UNOPNODE *)AllocNode (NK_CASELABEL);
        pLabel->pParent = pCase;
        pLabel->tokidx = Mark();
        if (CurToken() == TID_CASE)
        {
            NextToken();
            if (CurToken() == TID_COLON)
            {
                Error( ERR_ConstantExpected);
                pLabel->p1 = NULL;
            }
            else
            {
                pLabel->p1 = ParseExpression (pLabel);
            }
        }
        else
        {
            NextToken();
            pLabel->p1 = NULL;
        }

        Eat (TID_COLON);
        list.Add (pLabel);
    }

    // Next, parse statement list
    STATEMENTNODE   **ppNext = &pCase->pStmts;

    while (CurToken() != TID_CASE && CurToken() != TID_DEFAULT && CurToken() != TID_ENDFILE && CurToken() != TID_CLOSECURLY)
    {
        *ppNext = ParseStatement (pCase);
        ppNext = &(*ppNext)->pNext;
    }

    *ppNext = NULL;
    return pCase;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseCatchClause

BASENODE *CParser::ParseCatchClause (BASENODE *pParent, BOOL *pfEmpty)
{
    ASSERT (CurToken() == TID_CATCH);

    CATCHNODE   *pCatch = (CATCHNODE *)AllocNode (NK_CATCH);
    pCatch->pParent = pParent;
    pCatch->tokidx = Mark();

    // Check for the error of catch clause following empty catch here.
    if (*pfEmpty)
        Error (ERR_TooManyCatches);

    NextToken();

    if (CurToken() == TID_OPENPAREN)
    {
        NextToken();
        pCatch->pType = ParseClassType (pCatch);
        if (CurToken() == TID_IDENTIFIER)
            pCatch->pName = ParseIdentifier (pCatch);
        else
            pCatch->pName = NULL;
        Eat (TID_CLOSEPAREN);
    }
    else
    {
        pCatch->pType = NULL;
        pCatch->pName = NULL;
        *pfEmpty = TRUE;
    }

    pCatch->pBlock = ParseBlock (pCatch)->asBLOCK();
    return pCatch;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseSubExpression

BASENODE *CParser::ParseSubExpression (BASENODE *pParent, UINT iPrecedence, BASENODE *pLeftOperand)
{
    UINT        iNewPrec;
    OPERATOR    op;

    if (pLeftOperand == NULL)
    {
        // No left operand, so we need to parse one -- possibly preceded by a
        // unary operator.
        if (IsUnaryOperator (CurToken(), &op, &iNewPrec))
        {
            long        iTokIdx = Mark();
            BASENODE    *pTerm = NULL;

            NextToken ();

            switch (op)
            {
            // Check for special cases 'sizeof' and 'typeof' -- they have different syntax.
            case OP_TYPEOF:
                Eat(TID_OPENPAREN);

                pTerm = ParseReturnType (pParent);

                Eat (TID_CLOSEPAREN);

                pLeftOperand = AllocUnaryOpNode (op, iTokIdx, pParent, pTerm);
                pLeftOperand = ParsePostFixOperator(pParent, pLeftOperand);
                break;

            case OP_SIZEOF:
                Eat(TID_OPENPAREN);

                pTerm = ParseType (pParent);

                Eat (TID_CLOSEPAREN);

                pLeftOperand = AllocUnaryOpNode (op, iTokIdx, pParent, pTerm);
                pLeftOperand = ParsePostFixOperator(pParent, pLeftOperand);
                break;

            // check for special operators with function-like syntax
            case OP_MAKEREFANY:
            case OP_REFTYPE:
            case OP_CHECKED:
            case OP_UNCHECKED:
                Eat(TID_OPENPAREN);

                pTerm = ParseSubExpression (pParent, 0, NULL);

                Eat (TID_CLOSEPAREN);

                pLeftOperand = AllocUnaryOpNode (op, iTokIdx, pParent, pTerm);
                pLeftOperand = ParsePostFixOperator(pParent, pLeftOperand);
                break;

            case OP_REFVALUE:
            {
                Eat(TID_OPENPAREN);

                pTerm = ParseSubExpression (pParent, 0, NULL);

                Eat (TID_COMMA);

                BASENODE * pType = ParseType(pParent);

                Eat (TID_CLOSEPAREN);

                pLeftOperand = AllocBinaryOpNode (op, iTokIdx, pParent, pTerm, pType);
                pLeftOperand = ParsePostFixOperator(pParent, pLeftOperand);
                break;
            }

            // regular unary operator
            default:
                pTerm = ParseSubExpression (pParent, iNewPrec, NULL);
                if (op == OP_NEG && pTerm->kind == NK_CONSTVAL && pTerm->flags & NF_CHECK_FOR_UNARY_MINUS)
                {
                    // This is where we "collapse" the tree generated by "-2147483648", which will
                    // fit into an int but not before being made negative, so that the value stays "in"
                    // the data type the user expects.  However, we need to check for this case in
                    // GetLastToken, since the token index is the '-'; the EXTENT of this single node
                    // then becomes 2 tokens.
                    pLeftOperand = pTerm;
                    pLeftOperand->other = ((pTerm->other == PT_ULONG) ? PT_LONG : PT_INT);
                    pLeftOperand->tokidx = iTokIdx;
                }
                else
                {
                    pLeftOperand = AllocUnaryOpNode (op, iTokIdx, pParent, pTerm);
                }
            }
        }
        else
        {
            pLeftOperand = ParseTerm (pParent);
        }
    }

    while (TRUE)
    {
        // We either have a binary operator here, or we're finished.
        if (!IsBinaryOperator (CurToken(), &op, &iNewPrec))
            break;

        ASSERT (iNewPrec > 0);      // All binary operators must have precedence > 0!

        // Check the precedence to see if we should "take" this operator
        if (iNewPrec < iPrecedence)
            break;      // Nope -- precedence is too low

        if ((iNewPrec == iPrecedence) && !IsRightAssociative (op))
            break;      // Same precedence, but not right-associative -- deal with this "later"

        // Precedence is okay, so we'll "take" this operator.
        long    iTokOp = Mark();
        NextToken();

        if (op == OP_QUESTION)
        {
            // Special case for question-mark operator
            BASENODE    *pLeft = ParseSubExpression (pParent, iNewPrec - 1, NULL);
            long        iTokOpColon = Mark();
            Eat (TID_COLON);
            BASENODE    *pRight = ParseSubExpression (pParent, iNewPrec - 1, NULL);
            BASENODE    *pColon = AllocBinaryOpNode (OP_COLON, iTokOpColon, pParent, pLeft, pRight);
            pLeftOperand = AllocBinaryOpNode (OP_QUESTION, iTokOp, pParent, pLeftOperand, pColon);
        }
        else if (op == OP_IS || op == OP_AS)
        {
            // the is and as operators can only take a type
            pLeftOperand = AllocBinaryOpNode (op, iTokOp, pParent, pLeftOperand, ParseType (pParent));
        }
        else
        {
            pLeftOperand = AllocBinaryOpNode (op, iTokOp, pParent, pLeftOperand, ParseSubExpression (pParent, iNewPrec, NULL));
        }
    }

    pLeftOperand->pParent = pParent;
    return pLeftOperand;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseTerm

BASENODE *CParser::ParseTerm (BASENODE *pParent)
{
    TOKENID     iTok = CurToken();
    BASENODE    *pExpr = NULL;

    switch (iTok)
    {
        case TID_DOT:
        case TID_IDENTIFIER:
            pExpr = ParseName (pParent);
            break;

        case TID_ARGS:
        case TID_BASE:
        case TID_FALSE:
        case TID_THIS:
        case TID_TRUE:
        case TID_NULL:
            pExpr = AllocOpNode ((OPERATOR)m_rgTokenInfo[iTok].iSelfOp, Mark(), pParent);
            NextToken();
            break;

        case TID_OPENPAREN:
            pExpr = ParseCastOrExpression (pParent);
            break;

        case TID_NUMBER:
            pExpr = ParseNumber (pParent);
            break;

        case TID_STRINGLIT:
            pExpr = ParseStringLiteral (pParent);
            break;

        case TID_CHARLIT:
            pExpr = ParseCharLiteral (pParent);
            break;

        case TID_NEW:
            pExpr = ParseNewExpression (pParent);
            break;

        default:
            // check for intrinsic type followed by '.'
            if ((m_rgTokenInfo[CurToken()].dwFlags & TFF_PREDEFINED) && (m_rgTokenInfo[CurToken()].iPredefType != PT_VOID))
            {
                TYPENODE    *pType = (TYPENODE *)AllocNode (NK_TYPE);

                pType->tokidx = Mark();
                pType->pParent = pParent;
                pType->other = TK_PREDEFINED;
                pType->iType = m_rgTokenInfo[CurToken()].iPredefType;
                NextToken();

                if (CurToken() != TID_DOT)
                {
                    pExpr = AllocOpNode (OP_NOP, Mark(), pParent);
                    Error (ERR_InvalidExprTerm, m_rgTokenInfo[iTok].pszText);
                    NextToken();
                }
                else
                {
                    pExpr = pType;
                    // fall through to post-fix check below
                }
            }
            else
            {
                pExpr = AllocOpNode (OP_NOP, Mark(), pParent);
                Error (ERR_InvalidExprTerm, m_rgTokenInfo[iTok].pszText);
                NextToken();
            }
            break;
    }

    return ParsePostFixOperator(pParent, pExpr);
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParsePostFixOperator

BASENODE *CParser::ParsePostFixOperator (BASENODE *pParent, BASENODE *pExpr)
{
    // Look for post-fix operators
    while (TRUE)
    {
        TOKENID iTok = CurToken();
        long    iTokOp = Mark();

        switch (iTok)
        {
            case TID_OPENPAREN:
            {
                CALLNODE    *pOp = (CALLNODE *)AllocNode (NK_CALL);
                pOp->kind = NK_BINOP;       // NOTE:  This is just an overgrown BINOPNODE...
                pOp->pParent = pParent;
                pOp->p1 = pExpr;
                long    iErrorCount = m_iErrors;
                pOp->other = OP_CALL;
                pOp->p2 = ParseArgumentList (pOp);
                pOp->tokidx = iTokOp;
                pExpr->pParent = pOp;
                pOp->iClose = Mark() - 1;    // NOTE:  -1 because ParseArgumentList advanced past it...
                if (m_pTokens[pOp->iClose] != TID_CLOSEPAREN)
                    pOp->iClose++;
                if (iErrorCount != m_iErrors)
                    pOp->flags |= NF_CALL_HADERROR;         // Parameter tips key on this for window placement...
                pExpr = pOp;
                break;
            }

            case TID_OPENSQUARE:
            {
                CALLNODE    *pOp = (CALLNODE *)AllocNode (NK_CALL);
                pOp->kind = NK_BINOP;       // NOTE:  This is just an overgrown BINOPNODE...
                pOp->pParent = pParent;
                pOp->p1 = pExpr;
                long    iErrorCount = m_iErrors;
                pOp->p2 = ParseDimExpressionList (pOp);
                pOp->other = OP_DEREF;
                pOp->tokidx = iTokOp;
                pExpr->pParent = pOp;
                pOp->iClose = Mark() - 1;    // NOTE:  -1 because ParseDimExpressionList advanced past it...
                if (m_pTokens[pOp->iClose] != TID_CLOSESQUARE)
                    pOp->iClose++;
                if (iErrorCount != m_iErrors)
                    pOp->flags |= NF_CALL_HADERROR;         // Parameter tips key on this for window placement...
                pExpr = pOp;
                break;
            }

            case TID_PLUSPLUS:
            case TID_MINUSMINUS:
                pExpr = AllocUnaryOpNode (iTok == TID_PLUSPLUS ? OP_POSTINC : OP_POSTDEC, iTokOp, pParent, pExpr);
                NextToken();
                break;

            case TID_ARROW:
            case TID_DOT:
                if (NextToken() != TID_IDENTIFIER)
                {
                    CheckToken(TID_IDENTIFIER); // issue error.
                    pExpr = AllocDotNode (iTokOp, pParent, pExpr, ParseMissingName (NULL, pParent, Mark()));
                }
                else
                {
                    pExpr = AllocDotNode (iTokOp, pParent, pExpr, ParseIdentifier (pParent));
                }
                if (iTok == TID_ARROW)
                    pExpr->kind = NK_ARROW;
                break;

            default:
                return pExpr;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseCastOrExpression

BASENODE *CParser::ParseCastOrExpression (BASENODE *pParent)
{
    ASSERT (CurToken() == TID_OPENPAREN);

    long    iMark = Mark();

    // We have a decision to make -- is this a cast, or is it a parenthesized
    // expression?  Because look-ahead is cheap with our token stream, we check
    // to see if this "looks like" a cast (without constructing any parse trees)
    // to help us make the decision.
    if (ScanCast ())
    {
        // Looks like a cast, so parse it as one.
        Rewind (iMark);
        long    tokidx = Mark();
        NextToken();
        TYPENODE    *pType = ParseType (NULL);
        Eat (TID_CLOSEPAREN);
        return AllocBinaryOpNode (OP_CAST, tokidx, pParent, pType, ParseSubExpression(pParent, GetCastPrecedence(), NULL));
    }
    else
    {
        // Doesn't look like a cast, so parse this as a parenthesized expression.
        Rewind (iMark);
        long    tokidx = Mark();
        NextToken();
        BASENODE    *pExpr = ParseSubExpression (pParent, 0, NULL);
        Eat (TID_CLOSEPAREN);
        return AllocUnaryOpNode (OP_PAREN, tokidx, pParent, pExpr);
    }
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseNumber

BASENODE *CParser::ParseNumber (BASENODE *pParent)
{
    ASSERT (CurToken() == TID_NUMBER);

    CONSTVALNODE    *pConst = (CONSTVALNODE *)AllocNode (NK_CONSTVAL);

    // Scan number into actual numeric value
    pConst->pParent = pParent;
    pConst->tokidx = Mark();
    CParser::ScanNumericLiteral (m_iCurTok, pConst);
    NextToken();
    return pConst;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseStringLiteral

BASENODE *CParser::ParseStringLiteral (BASENODE *pParent)
{
    ASSERT (CurToken() == TID_STRINGLIT);

    CONSTVALNODE    *pConst = (CONSTVALNODE *)AllocNode (NK_CONSTVAL);

    // Scan string into STRCONST object
    pConst->pParent = pParent;
    pConst->tokidx = Mark();
    pConst->other = PT_STRING;
    pConst->val.strVal = ScanStringLiteral (m_iCurTok);
    NextToken();
    return pConst;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseCharLiteral

BASENODE *CParser::ParseCharLiteral (BASENODE *pParent)
{
    ASSERT (CurToken() == TID_CHARLIT);

    CONSTVALNODE    *pConst = (CONSTVALNODE *)AllocNode (NK_CONSTVAL);

    // Scan char into actual value
    pConst->pParent = pParent;
    pConst->tokidx = Mark();
    pConst->other = PT_CHAR;        // HACK:
    pConst->val.iVal = ScanCharLiteral (m_iCurTok);
    NextToken();
    return pConst;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseNewExpression

BASENODE *CParser::ParseNewExpression (BASENODE *pParent)
{
    ASSERT (CurToken() == TID_NEW);

    NEWNODE     *pNew = (NEWNODE *)AllocNode (NK_NEW);

    pNew->pParent = pParent;
    pNew->tokidx = Mark();
    pNew->iOpen = pNew->iClose = -1;

    NextToken();
    pNew->pType = (TYPENODE *)AllocNode (NK_TYPE);
    pNew->pType->tokidx = Mark();
    pNew->pType->pParent = pNew;
    pNew->pInit = NULL;
    pNew->pArgs = NULL;

    if (m_rgTokenInfo[CurToken()].dwFlags & TFF_PREDEFINED)
    {
        // This is a predefined type
        pNew->pType->other = TK_PREDEFINED;
        pNew->pType->iType = m_rgTokenInfo[CurToken()].iPredefType;
        NextToken();
        if (pNew->pType->iType == PT_VOID)
            CheckToken (TID_STAR);
    }
    else if (CurToken() == TID_IDENTIFIER)
    {
        // This is a named type
        pNew->pType->other = TK_NAMED;
        pNew->pType->pName = ParseName (pNew->pType);
    }
    else
    {
        // Should have been one or the other!
        Error (ERR_TypeExpected);
        pNew->pType->other = TK_NAMED;
        pNew->pType->pName = ParseMissingName (NULL, pNew->pType, Mark());
        return pNew;
    }

    // Okay, now check for object-creation, which is a class type followed by '('
    if (CurToken() == TID_OPENPAREN)
    {
        long    iErrors = m_iErrors;

        // This is a valid object-creation-expression (assuming the arg list is okay)
        pNew->iOpen = Mark();
        pNew->pArgs = ParseArgumentList (pNew);
        pNew->iClose = Mark() - 1;       // -1 because ParseArgumentList skipped it
        if (m_pTokens[pNew->iClose] != TID_CLOSEPAREN)
            pNew->iClose++;
        if (iErrors != m_iErrors)
            pNew->flags |= NF_CALL_HADERROR;
        return pNew;
    }

    // Check for pointer types
    while (CurToken() == TID_STAR)
    {
        // This is a pointer type.
        if (pNew->pType->TypeKind() == TK_PREDEFINED && (pNew->pType->iType == PT_OBJECT || pNew->pType->iType == PT_STRING))
            Error (ERR_IllegalPointerType);

        TYPENODE    *pPtr = (TYPENODE *)AllocNode (NK_TYPE);
        pPtr->pParent = pNew;
        pPtr->tokidx = Mark();
        NextToken();
        pPtr->other = TK_POINTER;
        pPtr->pElementType = pNew->pType;
        pNew->pType->pParent = pPtr;
        pNew->pType = pPtr;
    }

    // Here, we MUST have a '[' in order to be valid.
    if (CurToken() != TID_OPENSQUARE)
    {
        Error(ERR_BadNewExpr);
        pNew->flags |= NF_CALL_HADERROR;
        return pNew;
    }

    BOOL        fNeedInitializer = FALSE;
    TYPENODE    *pArrayTop = NULL, *pArrayLast = NULL;

    // Similar to ParseType (in the array case) we must build a tree in a top-down fashion to
    // generate the appropriate left-to-right reading of array expressions.  However, we must
    // check the first (leftmost) rank specifier for an expression list, which is allowed on
    // a 'new expression'.  So, if we don't a ']' OR ',' parse an expression list and
    // store it in the 'new' node.
    if (PeekToken(1) != TID_CLOSESQUARE && PeekToken(1) != TID_COMMA)
    {
        // First, construct the type node for this array dimension.
        pArrayTop = (TYPENODE *)AllocNode (NK_TYPE);
        pArrayTop->tokidx = Mark();
        pArrayTop->pParent = pNew;
        pArrayTop->other = TK_ARRAY;

        // Grab the expression list
        pNew->pArgs = ParseDimExpressionList (pNew);

        // Count the expressions supplied -- that's our rank
        pArrayTop->iDims = 1;
        for (BASENODE *p = pNew->pArgs; p->kind == NK_LIST; p = p->asLIST()->p2)
            pArrayTop->iDims++;

        pArrayLast = pArrayTop;
    }
    else
    {
        pNew->pArgs = NULL;
        fNeedInitializer = TRUE;
    }

    // Continue parsing rank specifiers
    while (CurToken() == TID_OPENSQUARE)
    {
        TYPENODE    *pArray = (TYPENODE *)AllocNode (NK_TYPE);
        pArray->tokidx = Mark();
        NextToken();
        if (pArrayTop != NULL)
        {
            pArray->pParent = pArrayLast;
            pArrayLast->pElementType = pArray;
        }
        else
        {
            pArrayTop = pArray;
            pArrayTop->pParent = pNew;
        }
        pArray->other = TK_ARRAY;
        pArray->iDims = 1;

        if (CurToken() != TID_CLOSESQUARE)
        {
            while (CurToken() == TID_COMMA)
            {
                NextToken();
                pArray->iDims++;
            }

            if (pArray->iDims == 1)
                Error (ERR_InvalidArray);
        }

        Eat (TID_CLOSESQUARE);
        pArrayLast = pArray;
    }

    pNew->iClose = Mark() - 1;       // -1 because we skipped the ']'

    if (pArrayTop != NULL)
    {
        ASSERT (pArrayLast != NULL);
        pArrayLast->pElementType = pNew->pType;
        pNew->pType->pParent = pArrayLast;
        pNew->pType = pArrayTop;
    }

    if (fNeedInitializer && CurToken() != TID_OPENCURLY)
        Error(ERR_MissingArraySize);

    if (CurToken() == TID_OPENCURLY)
        pNew->pInit = ParseArrayInitializer (pNew);

    return pNew;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseArgument

BASENODE *CParser::ParseArgument (BASENODE *pParent)
{
    DWORD       dwFlags = 0;

    if (CurToken() == TID_REF)
    {
        dwFlags |= NF_PARMMOD_REF;
        NextToken();
    }
    else if (CurToken() == TID_OUT)
    {
        dwFlags |= NF_PARMMOD_OUT;
        NextToken();
    }

    BASENODE    *pExpr = ParseExpression (pParent);
    pExpr->flags |= dwFlags;
    return pExpr;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseArgumentList

BASENODE *CParser::ParseArgumentList (BASENODE *pParent)
{
    Eat(TID_OPENPAREN);

    long        iTokOpen = Mark(), iTokIdx = iTokOpen;
    BASENODE    *pArgs;
    CListMaker   list (this, &pArgs);

    if (CurToken() != TID_CLOSEPAREN)
    {
        while (TRUE)
        {
            BASENODE    *pArg = ParseArgument (pParent);
            BASENODE    *pNew = list.Add (pArg);

            if (iTokIdx != iTokOpen)
                pNew->tokidx = iTokIdx;

            if (CurToken() == TID_COMMA)
                iTokIdx = Mark();
            else
                break;

            NextToken();
        }
    }

    Eat (TID_CLOSEPAREN);
    return pArgs;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseDimExpressionList

BASENODE *CParser::ParseDimExpressionList (BASENODE *pParent)
{
    ASSERT (CurToken() == TID_OPENSQUARE);

    long        iTokOpen = Mark(), iTokIdx = iTokOpen;
    BASENODE    *pArgs;
    CListMaker   list (this, &pArgs);

    while (TRUE)
    {
        NextToken();

        BASENODE    *pArg = ParseExpression(pParent);
        BASENODE    *pNew = list.Add (pArg);

        if (iTokIdx != iTokOpen)
            pNew->tokidx = iTokIdx;

        if (CurToken() == TID_COMMA)
            iTokIdx = Mark();
        else
            break;
    }

    // Eat the close brace and we're done
    Eat (TID_CLOSESQUARE);
    return pArgs;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::ParseExpressionList
//
// Parse a list of expressions separated by commas, terminated by semicolon or
// close-paren (currently only called from ParseForStatement).  The terminating
// token is NOT consumed.

BASENODE *CParser::ParseExpressionList (BASENODE *pParent)
{
    BASENODE    *pArgs;
    CListMaker   list (this, &pArgs);

    if (CurToken() != TID_CLOSEPAREN && CurToken() != TID_SEMICOLON)
    {
        while (TRUE)
        {
            list.Add (ParseExpression (pParent));
            if (CurToken() != TID_COMMA)
                break;
            NextToken();
        }
    }

    // we're done
    return pArgs;
}


////////////////////////////////////////////////////////////////////////////////
// CParser::ParseMemberRef
//
// This function will parse a member reference that looks like:
//    id
//    id(arglist)
//    id.id.id
//    id(arglist)
//    this
//    this[arglist]
//    id.id.this
//    id.id.this[arglist]
//    operator <op>
//    operator <op> (arglist)
//    id.id.operator <op>
//    id.id.operator <op> (arglist)
//    <implicit|explicit> operator <type>
//    <implicit|explicit> operator <type> (arglist)
//    id.id.<implicit|explicit> operator <type>
//    id.id.<implicit|explicit> operator <type> (arglist)
//
//    (First id can also be a predefined type (int, string, etc.)
//
// This is used for parsing cref references in XML comments. The returned
// NK_METHOD node has only the following fields filled in:
//    pName, pParms (if parameters), pType (if conversion operator).
//
// The pName field can actually have a NK_OP node in it for an operator <op>
// or "this" (OP_THIS). This is different than how operator/indexer are usually
// represented.

METHODNODE *CParser::ParseMemberRef ()
{
    METHODNODE  *pMethod = (METHODNODE *)AllocNode(NK_METHOD);
    BASENODE    *pParent = pMethod;
    BASENODE    *pName;
    BASENODE    *pParms = NULL;
    BASENODE    *pOp = NULL;
    TYPENODE    *pType = NULL;
    BASENODE    *pFullName = NULL;  // contains the full name tree.
    TOKENID     curTok;
    TOKENID     opTok = (TOKENID) -1;
    FULLTOKEN   tok;
    long        iDotTokIdx = -1;
    bool        isThis = false;
    int         cParms = 0;

    for (;;) {

        // Get a name component: id, this, operator.
        curTok = CurToken();
        if (curTok == TID_IDENTIFIER) {
            // Need the name, so rescan the token
            pName = AllocNode (NK_NAME);
            ((NAMENODE *)pName)->tokidx = Mark();
            ((NAMENODE *)pName)->pParent = pParent;
            RescanToken (Mark(), &tok);
            ((NAMENODE *)pName)->pName = tok.pName;
            NextToken();
        }
        else if (curTok == TID_THIS) {
            isThis = true;
            pName = AllocOpNode(OP_THIS, Mark(), pParent);
            NextToken();
            if (CurToken() != TID_OPENSQUARE && CurToken() != TID_ENDFILE)
                CheckToken(TID_OPENSQUARE);
        }
        else if (curTok == TID_OPERATOR) {
            NextToken();

            opTok = CurToken();
            if (opTok == TID_IMPLICIT || opTok == TID_EXPLICIT) {
                Error(ERR_BadOperatorSyntax, m_rgTokenInfo[opTok].pszText);
            }
            pName = pOp = AllocOpNode((OPERATOR)0, Mark(), pParent); // actual operator filled in later...
            NextToken();
            if (CurToken() != TID_ENDFILE)
                CheckToken(TID_OPENPAREN);
        }
        else if (curTok == TID_IMPLICIT || curTok == TID_EXPLICIT) {

            opTok = curTok;
            NextToken();

            if (CurToken() != TID_ENDFILE)
                Eat(TID_OPERATOR);

            pName = pOp = AllocOpNode((OPERATOR)0, Mark(), pParent); // actual operator filled in later...

            pType = ParseType( pParent);
        }
        else if (pFullName == NULL && (m_rgTokenInfo[curTok].dwFlags & TFF_PREDEFINED) && curTok != TID_VOID)
        {
            // This is a predefined type
            pName = (TYPENODE *)AllocNode (NK_TYPE);

            ((TYPENODE *)pName)->tokidx = Mark();
            ((TYPENODE *)pName)->pParent = pParent;
            ((TYPENODE *)pName)->other = TK_PREDEFINED;
            ((TYPENODE *)pName)->iType = m_rgTokenInfo[CurToken()].iPredefType;

            NextToken();
        }
        else {
            CheckToken(TID_IDENTIFIER);
            pName = ParseMissingName(NULL, pParent, Mark() + 1);
        }

        // Add onto the name already parsed, if any
        if (pFullName == NULL)
            pFullName = pName;
        else
            pFullName = AllocDotNode (iDotTokIdx, pParent, pFullName, pName);

        // Either get a dot to continue the name, a '(' or '[' for the arg list, or the end.
        curTok = CurToken();
        if (curTok == TID_DOT) {
            iDotTokIdx = Mark();
            NextToken();
        }
        else if (curTok == TID_OPENPAREN || curTok == TID_OPENSQUARE) {
            TOKENID endTok = (curTok == TID_OPENSQUARE) ? TID_CLOSESQUARE : TID_CLOSEPAREN;
            if (!isThis) {
                if (! CheckToken(TID_OPENPAREN)) // '[' illegal unless following 'this'
                    break;
            }
            NextToken();

            if (CurToken() != endTok) {
                // Scan parameter list.
                CListMaker   list (this, &pParms);
                if (CurToken() != endTok)
                {
                    while (TRUE)
                    {
                        list.Add (ParseParameter (pParent, true));
                        ++cParms;
                        if (CurToken() != TID_COMMA)
                            break;
                        NextToken();
                    }
                }
            }
            else {
                // empty param list -- must differentiate from no param list.
                pParms = NULL;
                pMethod->flags |= NF_MEMBERREF_EMPTYARGS;
            }

            // Must end arg list and have end of string.
            Eat(endTok);
            if (CurToken() != TID_ENDFILE)
                Error (ERR_SyntaxError, L"\"");
            break;
        }
        else if (curTok == TID_ENDFILE) {
            break;
        }
        else {
            ErrorAfterPrevToken (ERR_ExpectedDotOrParen);
            break;
        }
    }

    if (pOp) {
        // We can't validate the operator until we know the number of parameters.
        if (cParms == 0)
        {
            // No parms given -- pick the right one (binary over unary if both).
            if (m_rgTokenInfo[opTok].dwFlags & TFF_OVLBINOP)
                pOp->other = (OPERATOR)m_rgTokenInfo[opTok].iBinaryOp;
            else if (m_rgTokenInfo[opTok].dwFlags & TFF_OVLUNOP)
                pOp->other = (OPERATOR)m_rgTokenInfo[opTok].iUnaryOp;
            else
                ErrorAtToken (pOp->tokidx, ERR_OvlOperatorExpected);
        }
        else if (cParms == 1)
        {
            pOp->other = (OPERATOR)m_rgTokenInfo[opTok].iUnaryOp;
            if (!(m_rgTokenInfo[opTok].dwFlags & TFF_OVLUNOP))
                ErrorAtToken (pOp->tokidx, ERR_OvlUnaryOperatorExpected);
        }
        else
        {
            pOp->other = (OPERATOR)m_rgTokenInfo[opTok].iBinaryOp;
            if (!(m_rgTokenInfo[opTok].dwFlags & TFF_OVLBINOP))
                ErrorAtToken (pOp->tokidx, ERR_OvlBinaryOperatorExpected);
        }
    }

    pMethod->pName = pFullName;
    pMethod->pParms = pParms;
    pMethod->pType = pType;
    return pMethod;
}


////////////////////////////////////////////////////////////////////////////////
// CParser::GetFirstToken

long CParser::GetFirstToken (BASENODE *pNode, ExtentFlags flags, BOOL *pfMissingName)
{
    *pfMissingName = FALSE;

    if (pNode == NULL)
    {
        VSFAIL ("NULL node given to GetFirstToken!!!");
        return 0xf0000000;
    }

    switch (pNode->kind)
    {
        case NK_BINOP:
            // Special case the check for a cast -- the tokidx of the OP_CAST binop
            // node has the token index of the open paren
            if (pNode->Op() == OP_CAST)
                return pNode->tokidx;
            // fall through...
        case NK_ARROW:
        case NK_DOT:
        case NK_LIST:
            return GetFirstToken (((BINOPNODE *)pNode)->p1, flags, pfMissingName);

        case NK_UNOP:
            if (pNode->Op() == OP_POSTINC || pNode->Op() == OP_POSTDEC)
                return GetFirstToken (pNode->asUNOP()->p1, flags, pfMissingName);
            return pNode->tokidx;

        case NK_TYPE:
            if (pNode->TypeKind() == TK_PREDEFINED)
                return pNode->tokidx;
            return GetFirstToken (pNode->asTYPE()->pName, flags, pfMissingName);      // Name/element type comes before everything!

        case NK_NAMESPACE:
            if (pNode->tokidx == -1)
                return 0;
            return pNode->tokidx;

        case NK_NAME:
            *pfMissingName = (pNode->flags & NF_NAME_MISSING);
            return pNode->tokidx;

        case NK_NESTEDTYPE:
            return GetFirstToken (pNode->asNESTEDTYPE()->pType, flags, pfMissingName);

        case NK_DO:
            if (flags & EF_SINGLESTMT)
            {
                BOOL fMissing = FALSE;
                return GetFirstToken ( pNode->asDO()->pExpr, flags, &fMissing) - 2; // Include 'while ('
            }
            else
                goto DEFAULT;

        default:
DEFAULT:
            // MOST nodes' token index values are the first for their construct
            ASSERT (pNode->tokidx != -1);       // If you hit this, you'll either need to fix the parser or special-case this node type!
            return pNode->tokidx;
    }
}

////////////////////////////////////////////////////////////////////////////////
// CParser::GetLastToken

long CParser::GetLastToken (BASENODE *pNode, ExtentFlags flags, BOOL *pfMissingName)
{
    *pfMissingName = FALSE;
    BOOL        fMissing;       // Use this instead of pfMissing if adding offset to return value!
    long        offset = 0;

REDO:
    if (pNode == NULL)
    {
        VSFAIL ("NULL node given to GetLastToken!!!");
        return 0xf0000000;
    }

    switch (pNode->kind)
    {
        case NK_ACCESSOR:
            if (flags & EF_SINGLESTMT)
                return pNode->asACCESSOR()->iOpen;
            return pNode->asACCESSOR()->iClose;

        case NK_ARRAYINIT:
            if (pNode->asARRAYINIT()->p1 == NULL)
                return pNode->tokidx + 1 + offset;                                 // Gets the close curly
            offset++; // For the close curly
            pNode = pNode->asARRAYINIT()->p1;
            pfMissingName = &fMissing;
            goto REDO;

        case NK_BINOP:
            if ((pNode->Op() == OP_CALL || pNode->Op() == OP_DEREF) && ((BINOPNODE *)pNode)->p2 == NULL)
                return pNode->tokidx + 1 + offset;       // Get to the ) or ] in empty arg list
            if (pNode->Op() == OP_CALL || pNode->Op() == OP_DEREF)
                return GetLastToken (((BINOPNODE *)pNode)->p2, flags, &fMissing) + 1 + offset; // Gets the ) or ]
            // fall thru...
        case NK_ARROW:
        case NK_DOT:
        case NK_LIST:
            pNode = ((BINOPNODE *)pNode)->p2;
            goto REDO;

        case NK_ATTR:
            if (pNode->asATTR()->iClose != -1)
                return pNode->asATTR()->iClose + offset;

        case NK_ATTRARG:
            if (((ATTRNODE *)pNode)->pArgs != NULL)
                pNode = ((ATTRNODE *)pNode)->pArgs;
            else
                pNode = ((ATTRNODE *)pNode)->pName;
            goto REDO;

        case NK_ATTRDECL:
            return pNode->asATTRDECL()->iClose + offset;

        case NK_BLOCK:
            return pNode->asBLOCK()->iClose + offset;

        case NK_BREAK:
        case NK_CONTINUE:
        case NK_EXPRSTMT:
        case NK_GOTO:
        case NK_THROW:
        case NK_RETURN:
            if (((EXPRSTMTNODE *)pNode)->pArg != NULL)
                return GetLastToken (((EXPRSTMTNODE *)pNode)->pArg, flags, &fMissing) + 1 + offset;       // Includes semicolon...
            return pNode->tokidx + 1 + offset;       // Includes the semicolon...

        case NK_CASE:
            if (pNode->asCASE()->pStmts != NULL)
                return GetLastToken (GetLastStatement (pNode->asCASE()->pStmts), flags, pfMissingName) + offset;

            // The only case where the statement list can be null is if this is the last
            // set of case labels, and there were no statements there.  In this case, we want
            // FindLeafNode to "land" on the case, so include an extra token.
            return GetLastToken (pNode->asCASE()->pLabels, flags, pfMissingName) + 1 + offset;

        case NK_CASELABEL:
            if (pNode->asCASELABEL()->p1 != NULL)
                return GetLastToken (pNode->asCASELABEL()->p1, flags, &fMissing) + 1 + offset;  // Includes the colon...
            return pNode->tokidx + 1 + offset;                                           // Includes the colon...

        case NK_CATCH:
            if (flags & EF_SINGLESTMT)
            {
                if (pNode->asCATCH()->pName != NULL)
                    return GetLastToken(pNode->asCATCH()->pName, flags, &fMissing) + 1 + offset; // Includes the ')'
                if (pNode->asCATCH()->pType != NULL)
                    return GetLastToken(pNode->asCATCH()->pType, flags, &fMissing) + 1 + offset; // Includes the ')'
            }
            else
            {
                if (pNode->asCATCH()->pBlock != NULL)
                    return pNode->asCATCH()->pBlock->iClose + offset;
            }
            return pNode->tokidx + offset; // Nothing to include

        case NK_CHECKED:
        case NK_LABEL:
        case NK_UNSAFE:
            return GetLastToken (((LABELSTMTNODE *)pNode)->pStmt, flags, pfMissingName) + offset;

        case NK_CLASS:
        case NK_ENUM:
        case NK_INTERFACE:
        case NK_STRUCT:
            if (flags & EF_SINGLESTMT)
                return ((CLASSNODE*)pNode)->iOpen - 1 + offset; // Don't include the open curly
            return ((CLASSNODE *)pNode)->iClose + offset;

        case NK_FIELD:
            if (flags & EF_SINGLESTMT)
            {
                if (pNode->asFIELD()->pDecls->kind == NK_LIST)
                    return GetLastToken (((BINOPNODE*)pNode->asFIELD()->pDecls)->p1, flags, pfMissingName) + offset;   // Semicolon included by the NK_VARDECL
                return GetLastToken (pNode->asFIELD()->pDecls, flags, pfMissingName) + offset;     // Semicolon included by the NK_VARDECL
            }
            // fallthrough

        case NK_CTOR:
        case NK_DTOR:
        case NK_METHOD:
        case NK_OPERATOR:
            if (flags & EF_SINGLESTMT)
                return pNode->asANYMETHOD()->iOpen - 1 + offset; // Don't Include the open curly
            goto ANYMEMBER;

        case NK_NESTEDTYPE:
            if (flags & EF_SINGLESTMT)
                return GetLastToken(((NESTEDTYPENODE*)pNode)->pType, flags, pfMissingName) + offset;
            goto ANYMEMBER;

        case NK_INDEXER:
        case NK_PROPERTY:
        case NK_PARTIALMEMBER:
        case NK_ENUMMBR:
        case NK_CONST:
ANYMEMBER:
            return ((MEMBERNODE *)pNode)->iClose + offset;

        case NK_CONSTVAL:
            // Must special case check for the collapsed unary minus expression, which is two tokens instead of one.
            if ((pNode->flags & NF_CHECK_FOR_UNARY_MINUS) && (pNode->other == PT_LONG || pNode->other == PT_INT))
                return pNode->tokidx + 1 + offset;
            return pNode->tokidx + offset;

        case NK_EMPTYSTMT:
        case NK_OP:
            return pNode->tokidx + offset;

        case NK_DECLSTMT:
            if (flags & EF_SINGLESTMT)
            {
                if (pNode->asDECLSTMT()->pVars->kind == NK_LIST)
                    return GetLastToken (((BINOPNODE*)pNode->asDECLSTMT()->pVars)->p1, flags, pfMissingName) + offset;   // Semicolon included by the NK_VARDECL
                return GetLastToken (pNode->asDECLSTMT()->pVars, flags, pfMissingName) + offset;     // Semicolon included by the NK_VARDECL
            }
            else
                return GetLastToken (pNode->asDECLSTMT()->pVars, flags, &fMissing) + 1 + offset;     // Include the semicolon

        case NK_DELEGATE:
            return pNode->asDELEGATE()->iSemi + offset;

        case NK_DO:
            return GetLastToken (pNode->asDO()->pExpr, flags, &fMissing) + 2 + offset;     // Includes close paren and semicolon

        case NK_FOR:
            if (flags & EF_SINGLESTMT)
            {
                return pNode->tokidx + offset; // This is just a foreach
            }
            return GetLastToken (pNode->asFOR()->pStmt, flags, pfMissingName) + offset;

        case NK_IF:
            if (flags & EF_SINGLESTMT)
            {
                if (pNode->asIF()->pExpr != NULL)
                    return GetLastToken (pNode->asIF()->pExpr, flags, &fMissing) + 1 + offset;   // Includes close paren
                return pNode->tokidx + offset;
            }
            else
            {
                if (pNode->asIF()->pElse != NULL)
                    return GetLastToken (pNode->asIF()->pElse, flags, pfMissingName) + offset;
                return GetLastToken (pNode->asIF()->pStmt, flags, pfMissingName) + offset;
            }

        case NK_LOCK:
        case NK_WHILE:
            if (flags & EF_SINGLESTMT)
            {
                if (((LOOPSTMTNODE *)pNode)->pExpr != NULL)
                    return GetLastToken (((LOOPSTMTNODE *)pNode)->pExpr, flags, &fMissing) + 1 + offset; // Includes close paren
                return pNode->tokidx + offset;
            }
            else
                return GetLastToken (((LOOPSTMTNODE *)pNode)->pStmt, flags, pfMissingName) + offset;

        case NK_NAME:
            *pfMissingName = (pNode->flags & NF_NAME_MISSING);
            return pNode->tokidx + offset;

        case NK_NAMESPACE:
            if (flags & EF_SINGLESTMT)
                return pNode->asNAMESPACE()->iOpen + offset;
            return pNode->asNAMESPACE()->iClose + offset;

        case NK_NEW:
            if (pNode->asNEW()->pInit != NULL)
                return GetLastToken (pNode->asNEW()->pInit, flags, pfMissingName) + offset;
            if (pNode->asNEW()->iClose != -1)
                return pNode->asNEW()->iClose + offset;
            if (pNode->asNEW()->pType != NULL && pNode->asNEW()->pType->TypeKind() == TK_ARRAY && pNode->asNEW()->pArgs != NULL)
                return GetLastToken (pNode->asNEW()->pArgs, flags, pfMissingName) + 1 + offset;
            return GetLastToken (pNode->asNEW()->pType, flags, pfMissingName) + offset;

        case NK_PARAMETER:
            return GetLastToken (pNode->asPARAMETER()->pName, flags, pfMissingName) + offset;

        case NK_TRY:
            if (!(flags & EF_SINGLESTMT))
            {
                if (pNode->asTRY()->pCatch != NULL)
                    return GetLastToken (pNode->asTRY()->pCatch, flags, pfMissingName) + offset;
                if (pNode->asTRY()->pBlock != NULL)
                    return GetLastToken (pNode->asTRY()->pBlock, flags, pfMissingName) + offset;
            }
            return pNode->tokidx + offset;

        case NK_TYPE:
            if (pNode->TypeKind() == TK_PREDEFINED || pNode->TypeKind() == TK_POINTER)
                return pNode->tokidx + offset;
            if (pNode->TypeKind() == TK_NAMED)
                return GetLastToken (pNode->asTYPE()->pName, flags, pfMissingName) + offset;

            // For array types, the token index is the open '['.  Add the number of
            // dimensions and you land on the ']' (1==[], 2==[,], 3==[,,], etc).
            if (pNode->asTYPE()->iDims == -1)
                return pNode->tokidx + 2 + offset;               // unknown rank is [?]
            return pNode->tokidx + pNode->asTYPE()->iDims + offset;

        case NK_SWITCH:
            if (flags & EF_SINGLESTMT)
            {
                if (pNode->asSWITCH()->pExpr != NULL)
                    return GetLastToken (pNode->asSWITCH()->pExpr, flags, &fMissing) + 1 + offset; // Includes close paren
            }
            return pNode->asSWITCH()->iClose + offset;

        case NK_UNOP:
            if (pNode->Op() == OP_POSTINC || pNode->Op() == OP_POSTDEC)
                return pNode->tokidx + offset;
            if (pNode->Op() == OP_PAREN || pNode->Op() == OP_UNCHECKED || pNode->Op() == OP_CHECKED ||
                pNode->Op() == OP_TYPEOF || pNode->Op() == OP_SIZEOF)
                return GetLastToken (pNode->asUNOP()->p1, flags, &fMissing) + 1 + offset;      // Get the close paren
            return GetLastToken (pNode->asUNOP()->p1, flags, pfMissingName) + offset;

        case NK_USING:
            return GetLastToken (pNode->asUSING()->pName, flags, &fMissing) + 1 + offset;      // Includes semicolon

        case NK_VARDECL:
            if (flags & EF_SINGLESTMT)
            {
                if (pNode->flags & NF_VARDECL_ARRAY)
                    return GetLastToken (pNode->asVARDECL()->pArg, flags, &fMissing) + 2 + offset;     // Includes ']' and semicolon or comma
                if (pNode->flags & NF_VARDECL_EXPR)
                    return GetLastToken (pNode->asVARDECL()->pArg, flags, &fMissing) + 1 + offset;     // Includes semicolon or comma
                return GetLastToken (pNode->asVARDECL()->pName, flags, &fMissing) + 1 + offset;     // Includes semicolon or comma
            }
            else
            {
                if (pNode->flags & NF_VARDECL_ARRAY)
                    return GetLastToken (pNode->asVARDECL()->pArg, flags, &fMissing) + 1 + offset;     // Includes ']'
                if (pNode->flags & NF_VARDECL_EXPR)
                    return GetLastToken (pNode->asVARDECL()->pArg, flags, pfMissingName) + offset;
                return GetLastToken (pNode->asVARDECL()->pName, flags, pfMissingName) + offset;
            }

        default:
            // RARELY are nodes singularly represented (only names, constants, or other terms).
            // Represent them all explicitly!
            VSFAIL ("Unhandled node type in GetLastToken!");
            return pNode->tokidx + offset;
    }
}

////////////////////////////////////////////////////////////////////////////////
// CParser::GetLastStatement

STATEMENTNODE *CParser::GetLastStatement (STATEMENTNODE *pNode)
{
    while (pNode != NULL && pNode->pNext != NULL)
        pNode = pNode->pNext;

    return pNode;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::FindContainingMember

BASENODE *CParser::FindContainingMember (POSDATA pos, BASENODE *pMember)
{
    for (MEMBERNODE *pMbr = (MEMBERNODE *)pMember; pMbr != NULL; pMbr = pMbr->pNext)
    {
        POSDATA posStart, posEnd;

        GetExtent (pMbr, &posStart, &posEnd, EF_ALL);
        if (pos >= posStart && pos <= posEnd)
            return pMbr;

        if (pos < posStart)
            return NULL;
    }

    return NULL;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::FindContainingStatement

BASENODE *CParser::FindContainingStatement (POSDATA pos, BASENODE *pStatement)
{
    for (STATEMENTNODE *pStmt = (STATEMENTNODE *)pStatement; pStmt != NULL; pStmt = pStmt->pNext)
    {
        POSDATA posStart, posEnd;

        GetExtent (pStmt, &posStart, &posEnd, EF_ALL);
        if (pos >= posStart && pos <= posEnd)
            return pStmt;

        if (pos < posStart)
            return NULL;
    }

    return NULL;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::FindLeaf

BASENODE *CParser::FindLeaf (const POSDATA pos, BASENODE *pNode, CSourceData *pData, ICSInteriorTree **ppTree)
{
    // Check here rather than at every call site...
    if (pNode == NULL)
        return NULL;

    BASENODE    *pLeaf = NULL;
    POSDATA     posStart, posEnd;

    // If this is an intrinsically chained node, run the chain until we
    // find one that "contains" the given position
    if (pNode->InGroup (NG_STATEMENT))
    {
        // Search this statement chain (might just be pNode)
        pNode = FindContainingStatement (pos, pNode);
    }
    else if (pNode->InGroup (NG_MEMBER))
    {
        // Search this member chain (might just be pNode)
        pNode = FindContainingMember (pos, pNode);
    }
    else
    {
        // If this node doesn't "contain" this position, we can bail here.
        GetExtent (pNode, &posStart, &posEnd, EF_ALL);
        if (pos < posStart || pos > posEnd)
            pNode = NULL;
    }

    if (pNode == NULL)
        return NULL;

    // Okay, the rest depends on the type of node.
    switch (pNode->kind)
    {
        case NK_NAMESPACE:
            if (!(pLeaf = FindLeaf (pos, pNode->asNAMESPACE()->pName, pData, ppTree)) &&
                !(pLeaf = FindLeaf (pos, pNode->asNAMESPACE()->pUsing, pData, ppTree)))
                pLeaf = FindLeaf (pos, pNode->asNAMESPACE()->pElements, pData, ppTree);
            break;

        case NK_USING:
            if (!(pLeaf = FindLeaf (pos, pNode->asUSING()->pName, pData, ppTree)))
                pLeaf = FindLeaf (pos, pNode->asUSING()->pAlias, pData, ppTree);
            break;

        case NK_CLASS:
        case NK_ENUM:
        case NK_INTERFACE:
        case NK_STRUCT:
        {
            CLASSNODE   *pClass = (CLASSNODE *)pNode;

            if (!(pLeaf = FindLeaf (pos, pClass->pMembers, pData, ppTree)) &&
                !(pLeaf = FindLeaf (pos, pClass->pBases, pData, ppTree)) &&
                !(pLeaf = FindLeaf (pos, pClass->pName, pData, ppTree)))
                pLeaf = FindLeaf (pos, pClass->pAttr, pData, ppTree);
            break;
        }

        case NK_DELEGATE:
            if (!(pLeaf = FindLeaf (pos, pNode->asDELEGATE()->pParms, pData, ppTree)) &&
                !(pLeaf = FindLeaf (pos, pNode->asDELEGATE()->pName, pData, ppTree)) &&
                !(pLeaf = FindLeaf (pos, pNode->asDELEGATE()->pType, pData, ppTree)))
                pLeaf = FindLeaf (pos, pNode->asDELEGATE()->pAttr, pData, ppTree);
            break;

        case NK_MEMBER:
            ASSERT (FALSE);
            return NULL;

        case NK_ENUMMBR:
            if (!(pLeaf = FindLeaf (pos, pNode->asENUMMBR()->pValue, pData, ppTree))&&
                !(pLeaf = FindLeaf (pos, pNode->asENUMMBR()->pName, pData, ppTree)))
                pLeaf = FindLeaf (pos, pNode->asENUMMBR()->pAttr, pData, ppTree);
            break;

        case NK_CONST:
            if (!(pLeaf = FindLeaf (pos, pNode->asCONST()->pDecls, pData, ppTree)) &&
                !(pLeaf = FindLeaf (pos, pNode->asCONST()->pType, pData, ppTree)))
                pLeaf = FindLeaf (pos, pNode->asCONST()->pAttr, pData, ppTree);
            break;

        case NK_FIELD:
            if (!(pLeaf = FindLeaf (pos, pNode->asFIELD()->pDecls, pData, ppTree)) &&
                !(pLeaf = FindLeaf (pos, pNode->asFIELD()->pType, pData, ppTree)))
                pLeaf = FindLeaf (pos, pNode->asFIELD()->pAttr, pData, ppTree);
            break;

        case NK_METHOD:
        case NK_OPERATOR:
        case NK_CTOR:
        case NK_DTOR:
        {
            METHODNODE  *pMethod = (METHODNODE *)pNode;

            // This is an 'interior' node.  If the position follows the opening of
            // the body (assuming there is one, i.e. !NF_METHOD_NOBODY), then we need
            // to parse the interior.  If we weren't given a source data, then we DON'T
            // parse the interior and return this method node (this is used to check
            // if a token change is contained within an interior node for incremental
            // parsing)
            if ((pMethod->other & NFEX_METHOD_NOBODY) == 0 && pos >= m_pposTokens[pMethod->iOpen])
            {
                if (pData == NULL)
                    return pMethod;

                ASSERT (*ppTree == NULL);

                GetInteriorTree (pData, pMethod, ppTree);
                pLeaf = FindLeaf (pos, pMethod->pBody, pData, ppTree);
                return pLeaf ? pLeaf : pNode;
            }

            if (pMethod->kind == NK_CTOR)
            {
                // For NK_CTOR nodes, there is no name, but constructor arguments to
                // this/base, which FOLLOW the arguments to the constructor itself,
                // so the order must be this:
                if (!(pLeaf = FindLeaf (pos, pMethod->pCtorArgs, pData, ppTree)))
                    pLeaf = FindLeaf (pos, pMethod->pParms, pData, ppTree);
            }
            else if (pMethod->kind == NK_METHOD)
            {
                // Methods have names, which come before the parameters.
                if (!(pLeaf = FindLeaf (pos, pMethod->pParms, pData, ppTree)))
                    pLeaf = FindLeaf (pos, pMethod->pName, pData, ppTree);
            }
            else
            {
                // Operators and destructors have no name and no constructor args.
                ASSERT (pMethod->kind == NK_OPERATOR || pMethod->kind == NK_DTOR);
                pLeaf = FindLeaf (pos, pMethod->pParms, pData, ppTree);
            }

            if (pLeaf != NULL)
                return pLeaf;

            if (!(pLeaf = FindLeaf (pos, pMethod->pType, pData, ppTree)))
                pLeaf = FindLeaf (pos, pMethod->pAttr, pData, ppTree);
            break;
        }

        case NK_PROPERTY:
        case NK_INDEXER:
        {
            PROPERTYNODE    *pProp = pNode->asANYPROPERTY();

            if (!(pLeaf = FindLeaf (pos, pProp->pGet, pData, ppTree)) &&
                !(pLeaf = FindLeaf (pos, pProp->pSet, pData, ppTree)) &&
                !(pLeaf = FindLeaf (pos, pProp->pParms, pData, ppTree)) &&
                !(pLeaf = FindLeaf (pos, pProp->pName, pData, ppTree)) &&
                !(pLeaf = FindLeaf (pos, pProp->pType, pData, ppTree)))
                pLeaf = FindLeaf (pos, pProp->pAttr, pData, ppTree);
            break;
        }

        case NK_ACCESSOR:
        {
            // Accessors are interior nodes.  If we weren't given a source data, we can't
            // build an interior tree so return this node.
            if (pData == NULL)
                return pNode;

            GetInteriorTree (pData, pNode, ppTree);
            pLeaf = FindLeaf (pos, pNode->asACCESSOR()->pBody, pData, ppTree);
            break;
        }

        case NK_PARAMETER:
            if (!(pLeaf = FindLeaf (pos, pNode->asPARAMETER()->pType, pData, ppTree)) &&
                !(pLeaf = FindLeaf (pos, pNode->asPARAMETER()->pName, pData, ppTree)))
                pLeaf = FindLeaf (pos, pNode->asPARAMETER()->pAttr, pData, ppTree);
            break;

        case NK_NESTEDTYPE:
            pLeaf = FindLeaf (pos, pNode->asNESTEDTYPE()->pType, pData, ppTree);
            break;

        case NK_PARTIALMEMBER:
            if (!(pLeaf = FindLeaf (pos, pNode->asPARTIALMEMBER()->pNode, pData, ppTree)))
                pLeaf = FindLeaf (pos, pNode->asPARTIALMEMBER()->pAttr, pData, ppTree);
            break;

        case NK_TYPE:
            if (pNode->TypeKind() == TK_PREDEFINED)
                pLeaf = pNode;
            else
                pLeaf = FindLeaf (pos, pNode->asTYPE()->pName, pData, ppTree); // (Serves both name and element type)
            break;

        case NK_BLOCK:
            pLeaf = FindLeaf (pos, pNode->asBLOCK()->pStatements, pData, ppTree);
            break;

        case NK_BREAK:
        case NK_CONTINUE:
        case NK_EXPRSTMT:
        case NK_GOTO:
        case NK_RETURN:
        case NK_THROW:
            pLeaf = FindLeaf (pos, ((EXPRSTMTNODE *)pNode)->pArg, pData, ppTree);
            break;

        case NK_CHECKED:
        case NK_LABEL:
        case NK_UNSAFE:
            if (!(pLeaf = FindLeaf (pos, ((LABELSTMTNODE *)pNode)->pStmt, pData, ppTree)))
                pLeaf = FindLeaf (pos, ((LABELSTMTNODE *)pNode)->pLabel, pData, ppTree);
            break;

        case NK_DO:
            if (!(pLeaf = FindLeaf (pos, pNode->asDO()->pExpr, pData, ppTree)))
                pLeaf = FindLeaf (pos, pNode->asDO()->pStmt, pData, ppTree);
            break;

        case NK_LOCK:
        case NK_WHILE:
            if (!(pLeaf = FindLeaf (pos, ((LOOPSTMTNODE *)pNode)->pStmt, pData, ppTree)))
                pLeaf = FindLeaf (pos, ((LOOPSTMTNODE *)pNode)->pExpr, pData, ppTree);
            break;

        case NK_FOR:
            if (!(pLeaf = FindLeaf (pos, pNode->asFOR()->pInit, pData, ppTree)) &&
                !(pLeaf = FindLeaf (pos, pNode->asFOR()->pExpr, pData, ppTree)) &&
                !(pLeaf = FindLeaf (pos, pNode->asFOR()->pInc, pData, ppTree)))
                pLeaf = FindLeaf (pos, pNode->asFOR()->pStmt, pData, ppTree);
            break;

        case NK_IF:
            if (!(pLeaf = FindLeaf (pos, pNode->asIF()->pExpr, pData, ppTree)) &&
                !(pLeaf = FindLeaf (pos, pNode->asIF()->pStmt, pData, ppTree)))
                pLeaf = FindLeaf (pos, pNode->asIF()->pElse, pData, ppTree);
            break;

        case NK_DECLSTMT:
            if (!(pLeaf = FindLeaf (pos, pNode->asDECLSTMT()->pVars, pData, ppTree)))
                pLeaf = FindLeaf (pos, pNode->asDECLSTMT()->pType, pData, ppTree);
            break;

        case NK_SWITCH:
            if (!(pLeaf = FindLeaf (pos, pNode->asSWITCH()->pCases, pData, ppTree)))
                pLeaf = FindLeaf (pos, pNode->asSWITCH()->pExpr, pData, ppTree);
            break;

        case NK_CASE:
            if (!(pLeaf = FindLeaf (pos, pNode->asCASE()->pStmts, pData, ppTree)))
                pLeaf = FindLeaf (pos, pNode->asCASE()->pLabels, pData, ppTree);
            break;

        case NK_TRY:
            if (!(pLeaf = FindLeaf (pos, pNode->asTRY()->pCatch, pData, ppTree)))
                pLeaf = FindLeaf (pos, pNode->asTRY()->pBlock, pData, ppTree);
            break;

        case NK_CATCH:
            if (!(pLeaf = FindLeaf (pos, pNode->asCATCH()->pBlock, pData, ppTree)) &&
                !(pLeaf = FindLeaf (pos, pNode->asCATCH()->pName, pData, ppTree)))
                pLeaf = FindLeaf (pos, pNode->asCATCH()->pType, pData, ppTree);
            break;

        case NK_ARRAYINIT:
        case NK_CASELABEL:
        case NK_UNOP:
            pLeaf = FindLeaf (pos, ((UNOPNODE *)pNode)->p1, pData, ppTree);
            break;

        case NK_ARROW:
        case NK_BINOP:
        case NK_LIST:
        case NK_DOT:
            if (!(pLeaf = FindLeaf (pos, ((BINOPNODE *)pNode)->p2, pData, ppTree)))
                pLeaf = FindLeaf (pos, ((BINOPNODE *)pNode)->p1, pData, ppTree);
            break;

        case NK_OP:
        case NK_CONSTVAL:
            pLeaf = pNode;
            break;

        case NK_VARDECL:
            if (!(pLeaf = FindLeaf (pos, pNode->asVARDECL()->pArg, pData, ppTree)))
                pLeaf = FindLeaf (pos, pNode->asVARDECL()->pName, pData, ppTree);
            break;

        case NK_NEW:
            if (!(pLeaf = FindLeaf (pos, pNode->asNEW()->pInit, pData, ppTree)) &&
                !(pLeaf = FindLeaf (pos, pNode->asNEW()->pArgs, pData, ppTree)))
                pLeaf = FindLeaf (pos, pNode->asNEW()->pType, pData, ppTree);
            break;

        case NK_ATTR:
        case NK_ATTRARG:
            if (!(pLeaf = FindLeaf (pos, ((ATTRNODE *)pNode)->pArgs, pData, ppTree)))
                pLeaf = FindLeaf (pos, ((ATTRNODE *)pNode)->pName, pData, ppTree);
            break;

        case NK_ATTRDECL:
            pLeaf = FindLeaf (pos, pNode->asATTRDECL()->pAttr, pData, ppTree);
    }

    if (pLeaf == NULL)
        pLeaf = pNode;

    return pLeaf;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::GetExtent

void CParser::GetExtent (BASENODE *pNode, POSDATA *pposStart, POSDATA *pposEnd, ExtentFlags flags)
{
    BOOL    fMissingName;

    if (pposStart != NULL)
    {
        long    iTok = GetFirstToken (pNode, flags, &fMissingName);

        if (iTok >= m_iTokens)
            iTok = m_iTokens - 1;

        ASSERT (iTok >= 0 && iTok < m_iTokens);
        *pposStart = m_pposTokens[iTok];
        if (fMissingName)
        {
            // The first token in this span was the "missing name" node.  So, the
            // start position of that token is the first character FOLLOWING the
            // token at the specified index.  Note that we only need to rescan the
            // token for that if it isn't a 'fixed' token.
            if (m_rgTokenInfo[m_pTokens[iTok]].iLen == 0)
                GetTokenStopPosition (iTok, pposStart);
            else
                pposStart->iChar += m_rgTokenInfo[m_pTokens[iTok]].iLen;
        }
    }

    if (pposEnd != NULL)
    {
        long    iTok = GetLastToken (pNode, flags, &fMissingName);

        if (iTok >= m_iTokens)
            iTok = m_iTokens - 1;

        if (iTok != m_iTokens - 1 && m_rgTokenInfo[m_pTokens[iTok]].iLen == 0)
            GetTokenStopPosition (iTok, pposEnd);
        else
        {
            *pposEnd = m_pposTokens[iTok];
            pposEnd->iChar += m_rgTokenInfo[m_pTokens[iTok]].iLen;
        }

        if (fMissingName)
        {
            // Adjust the end of this span to include all trailing whitespace.  The
            // token found is the "missing name" token, whose index is actually
            // the token PRECEDING the intended identifier.
            while ((m_pszSource + m_piLines[pposEnd->iLine])[pposEnd->iChar] == ' ' || (m_pszSource + m_piLines[pposEnd->iLine])[pposEnd->iChar] == '\t')
                pposEnd->iChar++;
        }
    }
}




////////////////////////////////////////////////////////////////////////////////
// CTextParser::CTextParser

CTextParser::CTextParser () :
    m_pParser(NULL)
{
}

////////////////////////////////////////////////////////////////////////////////
// CTextParser::~CTextParser

CTextParser::~CTextParser ()
{
    if (m_pParser != NULL)
        delete m_pParser;
}

////////////////////////////////////////////////////////////////////////////////
// CTextParser::Initialize

HRESULT CTextParser::Initialize (CController *pController)
{
    m_pParser = new CSimpleParser (pController);
    if (m_pParser == NULL)
        return E_OUTOFMEMORY;

    return CTextLexer::CreateInstance (pController->GetNameMgr(), &m_spLexer);
}

////////////////////////////////////////////////////////////////////////////////
// CTextParser::SetInputText

STDMETHODIMP CTextParser::SetInputText (PCWSTR pszText, long iLen)
{
    return m_spLexer->SetInput (pszText, iLen);
}

////////////////////////////////////////////////////////////////////////////////
// CTextParser::SetInputStream

STDMETHODIMP CTextParser::SetInputStream (BYTE *pTokens, POSDATA *pposTokens, long iTokens, PCWSTR *ppszLines, long iLines)
{
    // This version doesn't implement this...
    return E_NOTIMPL;
}

////////////////////////////////////////////////////////////////////////////////
// CTextParser::CreateParseTree

STDMETHODIMP CTextParser::CreateParseTree (ParseTreeScope iScope, BASENODE *pParent, BASENODE **ppNode)
{
    LEXDATA     ld;
    HRESULT     hr;

    if (FAILED (hr = m_spLexer->GetLexResults (&ld)))
        return hr;

    m_pParser->SetInputData (&ld);
    m_pParser->Rewind (0);

    switch (iScope)
    {
        case PTS_NAMESPACEBODY:
        case PTS_TYPEBODY:
        case PTS_ENUMBODY:
            return E_NOTIMPL;

        case PTS_STATEMENT:     *ppNode = m_pParser->ParseStatement (pParent);              break;
        case PTS_EXPRESION:     *ppNode = m_pParser->ParseExpression (pParent);             break;
        case PTS_TYPE:          *ppNode = m_pParser->ParseType (pParent);                   break;
        case PTS_MEMBER:        *ppNode = m_pParser->ParseMember (pParent->asAGGREGATE());  break;
        case PTS_PARAMETER:     *ppNode = m_pParser->ParseParameter (pParent, FALSE);       break;

        default:
            return E_INVALIDARG;
    }

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// CTextParser::AllocateNode

STDMETHODIMP CTextParser::AllocateNode (long iKind, BASENODE **ppNode)
{
    return E_NOTIMPL;
}

