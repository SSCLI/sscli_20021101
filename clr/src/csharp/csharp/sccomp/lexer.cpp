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
// File: lexer.cpp
//
// ===========================================================================

#include "stdafx.h"
#include "srcmod.h"
#include "compiler.h"
#include "lexer.h"
#include "tokdata.h"
#include "escape.h"
#include "privguid.h"

////////////////////////////////////////////////////////////////////////////////
// CLexer::PositionOf

inline BOOL CLexer::PositionOf (PCWSTR psz, POSDATA *ppos)
{
    if (psz - m_pszCurLine >= MAX_POS_LINE_LEN)
        return FALSE;
    ppos->iLine = m_iCurLine;
    ppos->iChar = psz - m_pszCurLine;
    return TRUE;
}



////////////////////////////////////////////////////////////////////////////////
// CLexer::ScanToken
//
// This function scans the next token, starting from the character pointed to by
// the lexer context.  Full token information is placed in pFT.

TOKENID CLexer::ScanToken (FULLTOKEN *pFT)
{
    WCHAR       ch, chQuote;
    PCWSTR      p = m_pszCurrent, pszHold = NULL;

    // Initialize for new token scan
    pFT->iToken = TID_INVALID;
    pFT->dwFlags = 0;

    BOOL    fReal = FALSE;

    if (pFT->pszAllocToken != NULL)
    {
        // Only one thread is allowed to munge pszAllocToken at a time!
        //ASSERT (m_dwTokenizerThread + m_dwParserThread + m_dwInteriorParserThread == GetCurrentThreadId());
        VSFree (pFT->pszAllocToken);
        pFT->pszAllocToken = NULL;
    }

    m_fEscaped = FALSE;

    BOOL    fAtPrefix = FALSE;

    // Start scanning the token
    while (pFT->iToken == TID_INVALID)
    {
        m_pszTokenStart = p;
        if (!PositionOf (p, &pFT->posTokenStart) && !m_fThisLineTooLong)
        {
            ErrorAtPosition (m_iCurLine, MAX_POS_LINE_LEN - 1, 1, ERR_LineTooLong, MAX_POS_LINE_LEN);
            m_fLimitExceeded = TRUE;
            m_fThisLineTooLong = TRUE;
        }

        pFT->pszToken = p;

        switch (ch = *p++)
        {
            case 0:
            {
                // If this is an escaped NUL (\u0000), we gag with a TID_UNKNOWN
                if (m_pszTokenStart[0] != 0)
                {
                    ASSERT (m_pszTokenStart[0] == '\\');  // Must be the case...
                    pFT->iToken = TID_UNKNOWN;
                }
                else
                {
                    p = m_pszTokenStart;        // Back up to point to the 0 again...
                    pFT->iToken = TID_ENDFILE;
                }
                break;
            }

            case '\t':
            case ' ':
            {
                // Tabs and spaces tend to roam in groups... scan them together
                while (*p == ' ' || *p == '\t')
                    p++;
                break;
            }

            case UCH_PS:
            case UCH_LS:
            case '\n':
            {
                // This is a new line
                TrackLine (p);
                break;
            }

            case '\r':
            {
                // Bare CR's are lines, but CRLF pairs are considered a single line.
                if (*p == '\n')
                    p++;
                TrackLine (p);
                break;
            }

            // Other Whitespace characters
            case UCH_BOM:   // Unicode Byte-order marker
            case 0x001A:    // Ctrl+Z
            case '\v':      // Vertical Tab
            case '\f':      // Form-feed
            {
                break;
            }

            case '#':
            {
                if (!ScanPreprocessorLine (p))
                    pFT->iToken = TID_UNKNOWN;
                break;
            }

            case '\"':
            case '\'':
            {
                // "Normal" strings (double-quoted and single-quoted (char) literals)
                chQuote = ch;
                while (*p != chQuote)
                {
                    if (*p == '\\')
                        p++;       // Skip escaped characters (they're handled in ScanStringLiteral)

                    if (*p == '\n' || *p == '\r' || *p == UCH_PS || *p == UCH_LS || *p == 0)
                    {
                        ASSERT (p > pFT->pszToken);
                        ErrorAtPosition (m_iCurLine, (long)(pFT->pszToken - m_pszCurLine), (long)(p - pFT->pszToken), ERR_NewlineInConst);
                        pFT->dwFlags |= TF_UNTERMINATED;
                        break;
                    }

                    p++;
                }

                if (*p == chQuote)
                    p++;

                pFT->iToken = chQuote == '\'' ? TID_CHARLIT : TID_STRINGLIT;
                break;
            }

            case '/':
            {
                // Lotsa things start with slash...
                switch (*p)
                {
                    case '/':
                    {
                        // Single-line comments...
                        RecordCommentPosition (pFT->posTokenStart);

                        // Only check the length of Doc Comments
                        bool CheckLen = (p[1] == '/' && p[2] != '/');
                        while (*p != 0 && *p != '\n' && *p != '\r' && *p != UCH_PS && *p != UCH_LS) {
                            if (CheckLen && p - m_pszCurLine >= MAX_POS_LINE_LEN && !m_fThisLineTooLong)
                            {
                                ErrorAtPosition (m_iCurLine, MAX_POS_LINE_LEN - 1, 1, ERR_LineTooLong, MAX_POS_LINE_LEN);
                                m_fLimitExceeded = TRUE;
                                m_fThisLineTooLong = TRUE;
                            }

                            p++;
                        }
                        break;
                    }

                    case '*':
                    {
                        long    iLine = m_iCurLine, iCol = (long)(pFT->pszToken - m_pszCurLine);
                        BOOL    fDone = FALSE;

                        // Multi-line comments...
                        RecordCommentPosition (pFT->posTokenStart);
                        p++;
                        while (!fDone)
                        {
                            if (*p == 0)
                            {
                                // The comment didn't end.  Report an error at the start point.
                                ErrorAtPosition (iLine, iCol, 2, ERR_OpenEndedComment);
                                fDone = TRUE;
                                break;
                            }

                            if (*p == '*' && p[1] == '/')
                            {
                                p += 2;
                                break;
                            }

                            if (*p == '\n' || *p == '\r' || *p == UCH_PS || *p == UCH_LS)
                            {
                                if (*p == '\r' && p[1] == '\n')
                                    p++;
                                TrackLine (++p);
                            }
                            else
                            {
                                p++;
                            }
                        }

                        m_fFirstOnLine = FALSE;
                        break;
                    }

                    case '=':
                    {
                        p++;
                        pFT->iToken = TID_SLASHEQUAL;
                        break;
                    }

                    default:
                    {
                        pFT->iToken = TID_SLASH;
                        break;
                    }
                }

                break;
            }

            case '.':
            {
                if (*p >= '0' && *p <= '9')
                {
                    p++;
                    ch = 0;
                    goto _parseNumber;
                }
                pFT->iToken = TID_DOT;
                break;
            }

            case ',':
                pFT->iToken = TID_COMMA;
                break;

            case ':':
                pFT->iToken = TID_COLON;
                break;

            case ';':
                pFT->iToken = TID_SEMICOLON;
                break;

            case '~':
                pFT->iToken = TID_TILDE;
                break;

            case '!':
            {
                if (*p == '=')
                {
                    pFT->iToken = TID_NOTEQUAL;
                    p++;
                }
                else
                {
                    pFT->iToken = TID_BANG;
                }
                break;
            }

            case '=':
            {
                if (*p == '=')
                {
                    pFT->iToken = TID_EQUALEQUAL;
                    p++;
                }
                else
                {
                    pFT->iToken = TID_EQUAL;
                }
                break;
            }

            case '*':
            {
                if (*p == '=')
                {
                    pFT->iToken = TID_SPLATEQUAL;
                    p++;
                }
                else
                {
                    pFT->iToken = TID_STAR;
                }
                break;
            }

            case '(':
            {
                pFT->iToken = TID_OPENPAREN;
                break;
            }

            case ')':
            {
                pFT->iToken = TID_CLOSEPAREN;
                break;
            }

            case '{':
            {
                pFT->iToken = TID_OPENCURLY;
                break;
            }

            case '}':
            {
                pFT->iToken = TID_CLOSECURLY;
                break;
            }

            case '[':
            {
                pFT->iToken = TID_OPENSQUARE;
                break;
            }

            case ']':
            {
                pFT->iToken = TID_CLOSESQUARE;
                break;
            }

            case '?':
            {
                pFT->iToken = TID_QUESTION;
                break;
            }

            case '+':
            {
                if (*p == '=')
                {
                    p++;
                    pFT->iToken = TID_PLUSEQUAL;
                }
                else if (*p == '+')
                {
                    p++;
                    pFT->iToken = TID_PLUSPLUS;
                }
                else
                {
                    pFT->iToken = TID_PLUS;
                }
                break;
            }

            case '-':
            {
                if (*p == '=')
                {
                    p++;
                    pFT->iToken = TID_MINUSEQUAL;
                }
                else if (*p == '-')
                {
                    p++;
                    pFT->iToken = TID_MINUSMINUS;
                }
                else if (*p == '>')
                {
                    p++;
                    pFT->iToken = TID_ARROW;
                }
                else
                {
                    pFT->iToken = TID_MINUS;
                }
                break;
            }

            case '%':
            {
                if (*p == '=')
                {
                    p++;
                    pFT->iToken = TID_MODEQUAL;
                }
                else
                {
                    pFT->iToken = TID_PERCENT;
                }
                break;
            }

            case '&':
            {
                if (*p == '=')
                {
                    p++;
                    pFT->iToken = TID_ANDEQUAL;
                }
                else if (*p == '&')
                {
                    p++;
                    pFT->iToken = TID_LOG_AND;
                }
                else
                {
                    pFT->iToken = TID_AMPERSAND;
                }
                break;
            }

            case '^':
            {
                if (*p == '=')
                {
                    p++;
                    pFT->iToken = TID_HATEQUAL;
                }
                else
                {
                    pFT->iToken = TID_HAT;
                }
                break;
            }

            case '|':
            {
                if (*p == '=')
                {
                    p++;
                    pFT->iToken = TID_BAREQUAL;
                }
                else if (*p == '|')
                {
                    p++;
                    pFT->iToken = TID_LOG_OR;
                }
                else
                {
                    pFT->iToken = TID_BAR;
                }
                break;
            }

            case '<':
            {
                if (*p == '=')
                {
                    p++;
                    pFT->iToken = TID_LESSEQUAL;
                }
                else if (*p == '<')
                {
                    p++;
                    if (*p == '=')
                    {
                        p++;
                        pFT->iToken = TID_SHIFTLEFTEQ;
                    }
                    else
                    {
                        pFT->iToken = TID_SHIFTLEFT;
                    }
                }
                else
                {
                    pFT->iToken = TID_LESS;
                }
                break;
            }

            case '>':
            {
                if (*p == '=')
                {
                    p++;
                    pFT->iToken = TID_GREATEREQUAL;
                }
                else if (*p == '>')
                {
                    p++;
                    if (*p == '=')
                    {
                        p++;
                        pFT->iToken = TID_SHIFTRIGHTEQ;
                    }
                    else
                    {
                        pFT->iToken = TID_SHIFTRIGHT;
                    }
                }
                else
                {
                    pFT->iToken = TID_GREATER;
                }
                break;
            }

            case '@':
            {
                if (*p == '"')
                {
                    // NEW FORM of 'back-tick' strings -- preceded with '@' character is a string
                    // with no escape sequences -- if a quote is desired, it is doubled.
                    p++;
SCAN_STRING_LITERAL:
                    pFT->iToken = TID_STRINGLIT;
                    pFT->dwFlags |= TF_VERBATIMSTRING;

                    long    iLine = m_iCurLine, iCol = (long)(pFT->pszToken - m_pszCurLine);
                    BOOL    fDone = FALSE;

                    // Scan until we see the delimiter, tracking newlines.  We pay no regard
                    // to escape characters, because there are none in these kind of strings.
                    while (!fDone)
                    {
                        switch (*p++)
                        {
                            case UCH_PS:
                            case UCH_LS:
                            case '\n':
                            {
                                TrackLine (p);
                                break;
                            }

                            case '\r':
                            {
                                if (*p == '\n')
                                    p++;
                                TrackLine (p);
                                break;
                            }

                            case '\"':
                            {
                                if (*p == '\"')
                                    p++;            // Doubled quoted; skip
                                else
                                    fDone = TRUE;
                                break;
                            }

                            case 0:
                            {
                                // Reached the end of the source without finding the end-quote.  Give
                                // an error back at the starting point.
                                ErrorAtPosition (iLine, iCol, 2, ERR_UnterminatedStringLit);
                                pFT->dwFlags |= TF_UNTERMINATED;
                                fDone = TRUE;
                                p--;
                                break;
                            }
                        }
                    }

                    break;
                }

                // Check for identifiers.  NOTE:  Must use PEEKCHAR macro; unicode escapes are allowed here!
                if (!IsIdentifierChar (PEEKCHAR (m_pszTokenStart, p, ch)))
                {
                    // After the '@' we have neither an identifier nor and string quote, so assume it is a string
                    // and tell the user to add a quote
                    ErrorAtPosition (m_iCurLine, (long)(p - m_pszCurLine), 2, ERR_SyntaxError, L"\"");
                    goto SCAN_STRING_LITERAL;
                }

                NEXTCHAR (m_pszTokenStart, p, ch);
                fAtPrefix = TRUE;
                goto _ParseIdentifier;  // (Goto avoids the IsSpaceSeparator() check and the redundant IsIdentifierChar() check below...)
            }

            case '\\':
                // Could be unicode escape. Try that.
                --p;
                NEXTCHAR(m_pszTokenStart,p, ch);

                // If we had a unicode escape, ch is it. If we didn't, ch is still a backslash. Unicode escape
                // must start an identifers, so check only for identifiers now.
                goto _CheckIdentifier;

            default:
                if (IsSpaceSeparator (ch))    // Unicode class 'Zs'
                {
                    p++;
                    break;
                }
_CheckIdentifier:
                if (!IsIdentifierChar (ch))
                {
                    pFT->iToken = TID_UNKNOWN;      // treat other characters as unknown
                    break;
                }
                // Fall through case.  All the 'common' identifier characters are represented directly in
                // these switch cases for optimal perf.  Calling IsIdentifierChar() functions is relatively
                // expensive.
            case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
            case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
            case '_':
_ParseIdentifier:
            {
                long    iLength = fAtPrefix ? 2 : 1;

                // Remember, because we're processing identifiers here, unicode escape sequences are
                // allowed and must be handled (use NEXTCHAR and PEEKCHAR macros)
                do
                {
                    PEEKCHAR(m_pszTokenStart, p, ch);
                    switch (ch)
                    {
                        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
                        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
                        case '_':
                        case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
                        {
                            // Again, these are the 'common' identifier characters...
                            break;
                        }
                        case ' ': case '\t': case '.': case ';': case '(': case ')': case ',':
                        {
                            // ...and these are the 'common' stop characters.
                            goto LoopExit;
                        }
                        default:
                        {
                            // This is the 'expensive' call
                            if (IsIdentifierCharOrDigit (ch))
                                break;
                            goto LoopExit;
                        }
                    }
                    iLength++;
                    NEXTCHAR(m_pszTokenStart,p,ch);
                }
                while (ch);

LoopExit:
                if ((m_fEscaped = (p - m_pszTokenStart > iLength)))
                {
                    // This identifier token contained escape characters.  We need to
                    // copy the name from the buffer, translating its unicode escapes.
                    pFT->pszAllocToken = (WCHAR *)VSAlloc ((iLength + 1) * sizeof (WCHAR));
                    p = m_pszTokenStart;
                    for (int i = 0; i < iLength; i++)
                        pFT->pszAllocToken[i] = NEXTCHAR (m_pszTokenStart,p, ch);
                    pFT->pszAllocToken[iLength] = 0;
                    pFT->pszToken = pFT->pszAllocToken;     // Point the token text at the allocated version
                }

                PCWSTR  pszName = (m_fEscaped ? pFT->pszAllocToken : m_pszTokenStart);

                if (fAtPrefix)
                {
                    ASSERT (*pszName == '@');
                    pszName++;
                    iLength--;
                }

                if (iLength >= MAX_IDENT_SIZE)
                {
                    ErrorAtPosition (m_iCurLine, (long)(m_pszTokenStart - m_pszCurLine), (long)(p - m_pszTokenStart), ERR_IdentifierTooLong);
                    iLength = MAX_IDENT_SIZE;
                }

                pFT->pName = m_pNameMgr->AddString (pszName, iLength);
                if (fAtPrefix || m_fEscaped || ! m_pNameMgr->IsKeyword (pFT->pName, (int *)&pFT->iToken))
                    pFT->iToken = TID_IDENTIFIER;

                break;
            }

            case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
            {
                if (ch == '0' && (*p == 'x' || *p == 'X'))
                {
                    // it's a hex constant
                    p++;

                    // It's OK if it has no digits after the '0x' -- we'll catch it in ScanNumericLiteral
                    // and give a proper error then.
                    while (*p <= 'f' && isxdigit (*p))
                        p++;

                    if (*p == 'L' || *p == 'l')
                    {
                        p++;
                        if (*p == 'u' || *p == 'U')
                            p++;
                    }
                    else if (*p == 'u' || *p == 'U')
                    {
                        p++;
                        if (*p == 'L' || *p == 'l')
                            p++;
                    }
                }
                else
                {
                    // skip digits
                    while (*p >= '0' && *p <= '9')
                        p++;

                    if (*p == '.')
                    {
                        pszHold = p++;
                        if (*p >= '0' && *p <= '9')
                        {
                            // skip digits after decimal point
                            p++;
    _parseNumber:
                            fReal = TRUE;
                            while (*p >= '0' && *p <= '9')
                                p++;
                        }
                        else
                        {
                            // Number + dot + non-digit -- these are separate tokens, so don't absorb the
                            // dot token into the number.
                            p = pszHold;
                            pFT->iToken = TID_NUMBER;
                            break;
                        }
                    }

                    if (*p == 'E' || *p == 'e')
                    {
                        fReal = TRUE;

                        // skip exponent
                        p++;
                        if (*p == '+' || *p == '-')
                            p++;

                        while (*p >= '0' && *p <= '9')
                            p++;
                    }

                    if (fReal)
                    {
                        if (*p == 'f' || *p == 'F' || *p == 'D' || *p == 'd' || *p == 'm' || *p == 'M')
                            p++;
                    }
                    else if (*p == 'F' || *p == 'f' || *p == 'D' || *p == 'd' || *p == 'm' || *p == 'M')
                    {
                        p++;
                    }
                    else if (*p == 'L' || *p == 'l')
                    {
                        p++;
                        if (*p == 'u' || *p == 'U')
                            p++;
                    }
                    else if (*p == 'u' || *p == 'U')
                    {
                        p++;
                        if (*p == 'L' || *p == 'l')
                            p++;
                    }
                }
                pFT->iToken = TID_NUMBER;
                break;
            }
        } // switch
    } // while

    if (!PositionOf (p, &pFT->posTokenStop) && !m_fThisLineTooLong)
    {
        ErrorAtPosition (m_iCurLine, MAX_POS_LINE_LEN - 1, 1, ERR_LineTooLong, MAX_POS_LINE_LEN);
        m_fLimitExceeded = TRUE;
        m_fThisLineTooLong = TRUE;
    }

    m_pszCurrent = p;
    pFT->iLength = (long)(p - m_pszTokenStart);
    m_fFirstOnLine = FALSE;
    m_fTokensSeen = TRUE;
    return pFT->iToken;
}



////////////////////////////////////////////////////////////////////////////////
// CTextLexer::~CTextLexer

CTextLexer::~CTextLexer ()
{
    if (m_pNameMgr != NULL)
        m_pNameMgr->Release();

    if (m_pszInput != NULL)
        VSFree (m_pszInput);
}

////////////////////////////////////////////////////////////////////////////////
// CTextLexer::CreateInstance

HRESULT CTextLexer::CreateInstance (ICSNameTable *pNameTable, ICSLexer **ppLexer)
{
    HRESULT                 hr = E_OUTOFMEMORY;
    CComObject<CTextLexer>  *pObj;

    if (SUCCEEDED (hr = CComObject<CTextLexer>::CreateInstance (&pObj)))
    {
        if (FAILED (hr = pObj->Initialize (pNameTable)) ||
            FAILED (hr = pObj->QueryInterface (IID_ICSLexer, (void **)ppLexer)))
        {
            delete pObj;
        }
    }
    return hr;
}

////////////////////////////////////////////////////////////////////////////////
// CTextLexer::Initialize

HRESULT CTextLexer::Initialize (ICSNameTable *pNameTable)
{
    return pNameTable->QueryInterface (IID_ICSPrivateNameTable, (void **)&m_pNameMgr);
}

////////////////////////////////////////////////////////////////////////////////
// CTextLexer::SetInput

STDMETHODIMP CTextLexer::SetInput (PCWSTR pszInput, long iLen)
{
    HRESULT     hr;

    // Release anything we already have
    if (m_pszInput != NULL)
        VSFree (m_pszInput);
    m_arrayTokens.ClearAll();
    m_arrayTokenPos.ClearAll();
    m_arrayLines.ClearAll();
    m_fTokenized = FALSE;
    m_hr = S_OK;

    // If iLen is -1, use the null-terminated length
    if (iLen == -1)
        iLen = (long)wcslen (pszInput);

    // Copy the text, ensuring that it's null-terminated
    m_pszInput = (PWSTR)VSAlloc ((iLen + 1) * sizeof (WCHAR));
    if (m_pszInput == NULL)
        return E_OUTOFMEMORY;

    memcpy (m_pszInput, pszInput, iLen * sizeof (WCHAR));
    m_pszInput[iLen] = 0;

    long    *piLine;

    // Establish the first entry in the line table
    if (FAILED (hr = m_arrayLines.Add (NULL, &piLine)))
        return hr;

    *piLine = 0;

    // Start the input stream for tokenization
    m_pszCurrent = m_pszCurLine = m_pszInput;
    m_iCurLine = 0;

    FULLTOKEN   ft;
    TOKENID     tok = TID_UNKNOWN;
    BYTE        *pToken;
    POSDATA     *pposToken;

    // Scan the text
    while (tok != TID_ENDFILE && !FAILED (m_hr))
    {
        tok = ScanToken (&ft);
        if (FAILED (hr = m_arrayTokens.Add (NULL, &pToken)) ||
            FAILED (hr = m_arrayTokenPos.Add (NULL, &pposToken)))
        {
            return hr;
        }

        ASSERT (m_arrayTokens.Count() == m_arrayTokenPos.Count());
        *pToken = tok;
        *pposToken = ft.posTokenStart;
    }

    // That's it!  If we didn't fail, signal ourselves as being tokenized
    if (!FAILED (m_hr))
        m_fTokenized = TRUE;

    return m_hr;
}

////////////////////////////////////////////////////////////////////////////////
// CTextLexer::GetLexResults

HRESULT CTextLexer::GetLexResults (LEXDATA *pLexData)
{
    // Must be tokenized
    if (!m_fTokenized)
        return E_FAIL;

    // Just refer to the arrays accumulated in SetInput
    pLexData->pTokens = m_arrayTokens.Base();
    pLexData->pposTokens = m_arrayTokenPos.Base();
    ASSERT (m_arrayTokens.Count() == m_arrayTokenPos.Count());
    pLexData->iTokens = m_arrayTokens.Count();

    pLexData->pszSource = m_pszInput;
    pLexData->iLines = m_arrayLines.Count();
    pLexData->piLines = m_arrayLines.Base();

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// CTextLexer::RescanToken

STDMETHODIMP CTextLexer::RescanToken (long iToken, TOKENDATA *pTokenData)
{
    // Must be tokenized
    if (!m_fTokenized)
        return E_FAIL;

    // Make sure the token index given is valid...
    if (iToken < 0 || iToken >= m_arrayTokens.Count())
        return E_INVALIDARG;

    FULLTOKEN   tok;
    POSDATA     *pposToken = m_arrayTokenPos.GetAt (iToken);

    // Set the current pointer to the beginning of the token
    m_pszCurLine = m_pszInput + m_arrayLines[pposToken->iLine];
    m_pszCurrent = m_pszCurLine + pposToken->iChar;

    // Scan it and return the token data
    ScanToken (&tok);
    *pTokenData = tok;
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// CTextLexer::TrackLine

void CTextLexer::TrackLine (PCWSTR pszLine)
{
    // We only update the line table here if !m_fTokenized (which means we're
    // currently doing the initial tokenization of new input).
    if (!m_fTokenized)
    {
        long    *piLine;

        if (SUCCEEDED (m_hr = m_arrayLines.Add (NULL, &piLine)))
            *piLine = (long)(pszLine - m_pszInput);
    }

    m_pszCurLine = pszLine;
    m_iCurLine++;

    CLexer::TrackLine(pszLine);
}
