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
// File: node.cpp
//
// ===========================================================================

#include "pch.h"
#include "scciface.h"
#include "nodes.h"
#include "name.h"
#include "strbuild.h"
#include "tokdata.h"
#include "posdata.h"
#include "images.h"
#include "stdio.h"

struct PTNAME
{
    PCWSTR  pszFull;
    PCWSTR  pszNice;
    PCWSTR  pszKeySig;
};

#define PREDEFTYPEDEF(id, name, required, simple, numeric, isclass, isstruct, isiface, isdelegate, ft, et, nicename, zero, qspec, asize, st, attr, sig) { L##name, nicename, L##sig },
static  const   PTNAME  rgPredefNames[] = {
#include "predeftype.h"
    { L"", NULL, NULL },
    { L"System.Void", L"void", L"void" }
};
#undef PREDEFTYPEDEF

#define OP(o,p,r,s,t,pn,ek) t,
static  const   long    rgOpTokens[] = {
#include "ops.h"
};


////////////////////////////////////////////////////////////////////////////////
// BASENODE::m_rgNodeGroups

DWORD   BASENODE::m_rgNodeGroups[] = {
#define NODEKIND(n,s,g) g,
#include "nodekind.h"
0};

////////////////////////////////////////////////////////////////////////////////
// BASENODE::GetOperatorToken

long BASENODE::GetOperatorToken (long iOp)
{
    return rgOpTokens[iOp];
}

////////////////////////////////////////////////////////////////////////////////
// BASENODE::AppendNameText

HRESULT BASENODE::AppendNameText (CStringBuilder &sb, ICSNameTable *pNameTable)
{
    if (this->kind == NK_NAME)
    {
        if (pNameTable != NULL && pNameTable->IsKeyword (this->asNAME()->pName, NULL) == S_OK)
            sb += L"@";
        sb += this->asNAME()->pName->text;
        return S_OK;
    }

    if (this->kind == NK_DOT)
    {
        this->asDOT()->p1->AppendNameText (sb, pNameTable);
        sb += L".";
        if (pNameTable != NULL && pNameTable->IsKeyword (this->asDOT()->p2->asNAME()->pName, NULL) == S_OK)
            sb += L"@";
        sb += this->asDOT()->p2->asNAME()->pName->text;
        return S_OK;
    }

    VSFAIL ("AppendNameText called on non-name node...");
    return E_FAIL;
}

////////////////////////////////////////////////////////////////////////////////
// BASENODE::AppendTypeText

HRESULT BASENODE::AppendTypeText (CStringBuilder &sb, ICSNameTable *pNameTable)
{
    // Back up to a type node
    BASENODE *pPar;
    for (pPar = this->GetParent(); pPar != NULL; pPar = pPar->GetParent())
        if (pPar->InGroup (NG_AGGREGATE))
            break;

    HRESULT     hr = E_FAIL;        // If nothing is added, then this isn't a type or contained by one
    BOOL        fNeedDot = FALSE;

    if (pPar != NULL)
    {
        // Put it's type text in the bstr first...
        if (FAILED (hr = pPar->AppendTypeText (sb, pNameTable)))
            return hr;

        fNeedDot = TRUE;
    }

    // Now this, if it is in fact a type itself
    if (this->InGroup (NG_AGGREGATE) || this->kind == NK_DELEGATE)
    {
        if (fNeedDot)
            sb += L".";

        NAME    *pName = (this->kind == NK_DELEGATE) ? this->asDELEGATE()->pName->pName : this->asAGGREGATE()->pName->pName;

        if (pNameTable != NULL && pNameTable->IsKeyword (pName, NULL) == S_OK)
            sb += L"@";

        sb += pName->text;
        hr = S_OK;
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
// BASENODE::BuildKey

HRESULT BASENODE::BuildKey (ICSNameTable *pNameTable, BOOL fIncludeParent, NAME **ppKey)
{
    // Must be in the group of key-able nodes...
    if (!this->InGroup (NG_KEYED))
        return E_FAIL;

    // NG_KEYED contains:
    //  NK_CLASS
    //  NK_CTOR
    //  NK_DELEGATE
    //  NK_DTOR
    //  NK_ENUM
    //  NK_ENUMMBR
    //  NK_INTERFACE
    //  NK_METHOD
    //  NK_NAMESPACE
    //  NK_OPERATOR
    //  NK_PROPERTY
    //  NK_INDEXER
    //  NK_STRUCT
    //  NK_VARDECL      NOTE:  You'll notice FIELD and CONST do not belong; only VARDECL, which is used for each field/const declaration

    NAME            **ppCachedKey = NULL;
    CStringBuilder  sb;
    HRESULT         hr;

    if (fIncludeParent)
    {
        // Some nodes cache their keys.  Check for those nodes here, and return the
        // cached key (if created).  Note that the global namespace (the root of everything)
        // has an empty key, which is assigned at parse time -- so this is gauranteed to
        // stop there at least.
        // Also, note that we only do this if our caller asked to include the parent key,
        // since only "full" keys are cached.
        if (this->InGroup (NG_AGGREGATE))
            ppCachedKey = &this->asAGGREGATE()->pKey;
        if (this->kind == NK_NAMESPACE)
            ppCachedKey = &this->asNAMESPACE()->pKey;
        if (this->kind == NK_DELEGATE)
            ppCachedKey = &this->asDELEGATE()->pKey;

        if (ppCachedKey != NULL && *ppCachedKey != NULL)
        {
            *ppKey = *ppCachedKey;
            return S_OK;
        }

        // Okay, we have to build it.  All keys consist of <parentkey> + "." + <thiskey>
        // (unless our parent key is empty, in which case it's just <thiskey>), so find our parent's key.
        BASENODE *p;
        for (p = this->GetParent(); p != NULL && !p->InGroup (NG_KEYED); p = p->GetParent())
            ;

        NAME    *pParentKey;

        if (FAILED (hr = p->BuildKey (pNameTable, TRUE, &pParentKey)))
            return hr;

        ASSERT (pParentKey != NULL);

        sb = pParentKey->text;
        if (pParentKey->text[0] != 0)
            sb += L".";
    }

    // Okay, now do the right thing based on the kind of this node...
    switch (this->kind)
    {
        case NK_NAMESPACE:
            this->asNAMESPACE()->pName->AppendNameText (sb, NULL);
            break;

        case NK_CLASS:
        case NK_STRUCT:
        case NK_INTERFACE:
        case NK_ENUM:
            this->asAGGREGATE()->pName->AppendNameText (sb, NULL);
            break;

        case NK_DELEGATE:
            this->asDELEGATE()->pName->AppendNameText (sb, NULL);
            break;

        case NK_ENUMMBR:
            sb += this->asENUMMBR()->pName->pName->text;
            break;

        case NK_VARDECL:
            sb += this->asVARDECL()->pName->pName->text;
            break;

        case NK_METHOD:
            this->asANYMETHOD()->pName->AppendNameText (sb, NULL);
            AppendParametersToKey (this->asANYMETHOD()->pParms, sb);
            break;

        case NK_OPERATOR:
        {
            WCHAR   szBuf[16];

            swprintf (szBuf, L"#op%d", this->asANYMETHOD()->iOp);
            sb += szBuf;
            AppendParametersToKey (this->asANYMETHOD()->pParms, sb);
            break;
        }

        case NK_CTOR:
            if (this->flags & NF_MOD_STATIC)
                sb += L"#sctor";
            else
                sb += L"#ctor";
            AppendParametersToKey (this->asANYMETHOD()->pParms, sb);
            break;

        case NK_DTOR:
            sb += L"#dtor()";
            break;

        case NK_INDEXER:
        case NK_PROPERTY:
            if (this->asANYPROPERTY()->pName != NULL)
                this->asANYPROPERTY()->pName->AppendNameText (sb, NULL);
            if (this->kind == NK_INDEXER)
            {
                if (this->asANYPROPERTY()->pName != NULL)
                    sb += L".";
                sb += L"#this";
                AppendParametersToKey (this->asANYPROPERTY()->pParms, sb);
            }
            break;

        default:
            VSFAIL ("Unhandled node type in BuildKey!");
            return E_FAIL;
    }

    // Make it into a name and return it, caching it if this is a key caching node
    if (SUCCEEDED (hr = sb.CreateName (pNameTable, ppKey)))
    {
        if (ppCachedKey != NULL)
        {
            ASSERT (fIncludeParent);
            *ppCachedKey = *ppKey;
        }
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
// BASENODE::AppendPrototype

HRESULT BASENODE::AppendPrototype (ICSNameTable *pNameTable, DWORD dwFlags, CStringBuilder &sb)
{
    // Must be in the group of key-able nodes...
    if (!this->InGroup (NG_KEYED))
        return E_FAIL;

    // NG_KEYED contains:
    //  NK_CLASS            *
    //  NK_CTOR
    //  NK_DELEGATE         *
    //  NK_DTOR
    //  NK_ENUM             *
    //  NK_ENUMMBR
    //  NK_INTERFACE        *
    //  NK_METHOD
    //  NK_NAMESPACE        *
    //  NK_OPERATOR
    //  NK_PROPERTY
    //  NK_INDEXER
    //  NK_STRUCT           *
    //  NK_VARDECL      NOTE:  You'll notice FIELD and CONST do not belong; only VARDECL, which is used for each field/const declaration

    HRESULT     hr;

    // The starred ones are nodes for which the key will do fine if PTF_FULLNAME is specified,
    // and just this node's name will do otherwise...
    if (this->InGroup (NG_TYPE) || this->kind == NK_NAMESPACE)
    {
        if (dwFlags & PTF_FULLNAME)
        {
            NAME    *pKey;

            if (FAILED (hr = this->BuildKey (pNameTable, TRUE, &pKey)))
                return hr;

            sb += pKey->text;
            return S_OK;
        }

        if (this->kind == NK_NAMESPACE)
            return this->asNAMESPACE()->pName->AppendNameText (sb, NULL);

        if (this->kind == NK_DELEGATE)
            return this->asDELEGATE()->pName->AppendNameText (sb, NULL);

        return this->asAGGREGATE()->AppendNameText (sb, NULL);
    }

    // Okay, this is a member.  If PTF_FULLNAME or PTF_TYPENAME is provided, get our parent's prototype.
    if (dwFlags & (PTF_FULLNAME|PTF_TYPENAME))
    {
        BASENODE *pPar;
        for (pPar = this->pParent; pPar != NULL && !pPar->InGroup (NG_KEYED); pPar = pPar->pParent)
            ;

        if (FAILED (hr = pPar->AppendPrototype (pNameTable, dwFlags, sb)))
            return hr;

        sb += L".";
    }

    // Okay, now do the right thing based on the kind of this node...
    switch (this->kind)
    {
        case NK_ENUMMBR:
            sb += this->asENUMMBR()->pName->pName->text;
            break;

        case NK_VARDECL:
            sb += this->asVARDECL()->pName->pName->text;
            break;

        case NK_METHOD:
            this->asANYMETHOD()->pName->AppendNameText (sb, NULL);
            if (dwFlags & PTF_PARAMETERS)
                AppendParametersToPrototype (FALSE, this->asANYMETHOD()->pParms, sb, TRUE);
            break;

        case NK_OPERATOR:
        {
            PCWSTR  pszOpToken;
            long    iOp = this->asANYMETHOD()->iOp;

            // Map operator to operator name.  implicit/explicit must be
            // handled differently.
            pNameTable->GetTokenText (rgOpTokens[iOp], &pszOpToken);
            if (iOp == OP_IMPLICIT || iOp == OP_EXPLICIT)
            {
                sb += pszOpToken;
                sb += L" operator";
            }
            else
            {
                sb += L"operator ";
                sb += pszOpToken;
            }

            if (dwFlags & PTF_PARAMETERS)
                AppendParametersToPrototype (FALSE, this->asANYMETHOD()->pParms, sb, TRUE);
            break;
        }

        case NK_CTOR:
        case NK_DTOR:
        {
            BASENODE *p;
            for (p = this->pParent; p != NULL && !p->InGroup (NG_AGGREGATE); p = p->pParent)
                ;
            if (this->kind == NK_DTOR)
                sb += L"~";
            sb += p->asAGGREGATE()->pName->pName->text;
            if (dwFlags & PTF_PARAMETERS)
                AppendParametersToPrototype (FALSE, this->asANYMETHOD()->pParms, sb, TRUE);
            break;
        }

        case NK_INDEXER:
        case NK_PROPERTY:
            if (this->asANYPROPERTY()->pName != NULL)
                this->asANYPROPERTY()->pName->AppendNameText (sb, NULL);
            if (this->kind == NK_INDEXER)
            {
                if (this->asANYPROPERTY()->pName != NULL)
                    sb += L".";
                sb += L"this";
                if (dwFlags & PTF_PARAMETERS)
                    AppendParametersToPrototype (TRUE, this->asANYPROPERTY()->pParms, sb, TRUE);
            }
            break;

        default:
            VSFAIL ("Unhandled node type in AppendPrototype!");
            return E_FAIL;
    }

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// BASENODE::AppendParametersToKey

void BASENODE::AppendParametersToKey (BASENODE *pParms, CStringBuilder &sb)
{
    sb += L"(";

    CListIterator   li;
    long            iParms = 0;

    li.Start (pParms);
    for (BASENODE *pParm = li.Next(); pParm != NULL; pParm = li.Next())
    {
        if (iParms > 0)
            sb += L",";
        AppendTypeToKey (pParm->asPARAMETER()->pType, sb);
        iParms++;
    }

    sb += L")";
}

////////////////////////////////////////////////////////////////////////////////
// BASENODE::AppendParametersToPrototype

void BASENODE::AppendParametersToPrototype (BOOL fIndexer, BASENODE *pParms, CStringBuilder &sb, BOOL fParamName)
{
    sb += (fIndexer ? L"[" : L"(");

    CListIterator   li;
    long            iParms = 0;
    BASENODE        *pParm, *pNext = NULL;

    li.Start (pParms);
    pNext = li.Next();
    for (pParm = pNext, pNext = li.Next(); pParm != NULL; pParm = pNext, pNext = li.Next())
    {
        if (iParms > 0)
            sb += L",";

        if (pParm->flags & NF_PARMMOD_REF)
            sb += L"ref ";
        else if (pParm->flags & NF_PARMMOD_OUT)
            sb += L"out ";

        if ((pParms->pParent->other & NFEX_METHOD_PARAMS) && pNext == NULL)
            sb += L"params ";

        AppendTypeToPrototype (pParm->asPARAMETER()->pType, sb);
	
        // add parameter names
        if (fParamName)
        {
            sb += L" ";
            sb += pParm->asPARAMETER()->pName->pName->text;
        }
        iParms++;
    }

    sb += (fIndexer ? L"]" : L")");
}

////////////////////////////////////////////////////////////////////////////////
// BASENODE::AppendTypeToKey

void BASENODE::AppendTypeToKey (TYPENODE *pType, CStringBuilder &sb)
{
    if (pType == NULL)
    {
        sb += L"?";
        return;
    }

    if (pType->TypeKind() == TK_NAMED)
    {
        pType->pName->AppendNameText (sb, NULL);
    }
    else if (pType->TypeKind() == TK_PREDEFINED)
    {
        sb += rgPredefNames[pType->iType].pszKeySig;
    }
    else if (pType->TypeKind() == TK_ARRAY)
    {
        AppendTypeToKey (pType->pElementType, sb);
        sb += L"[]";
    }
    else if (pType->TypeKind() == TK_POINTER)
    {
        sb += L"*";
        AppendTypeToKey (pType->pElementType, sb);
    }
}

////////////////////////////////////////////////////////////////////////////////
// BASENODE::AppendTypeToPrototype

void BASENODE::AppendTypeToPrototype (TYPENODE *pType, CStringBuilder &sb)
{
    if (pType == NULL)
    {
        sb += L"?";
        return;
    }

    if (pType->TypeKind() == TK_NAMED)
    {
        pType->pName->AppendNameText (sb, NULL);
    }
    else if (pType->TypeKind() == TK_PREDEFINED)
    {
        sb += (rgPredefNames[pType->iType].pszNice == NULL) ? rgPredefNames[pType->iType].pszFull : rgPredefNames[pType->iType].pszNice;
    }
    else if (pType->TypeKind() == TK_ARRAY)
    {
        AppendTypeToPrototype (pType->pElementType, sb);
        sb += L"[]";
    }
    else if (pType->TypeKind() == TK_POINTER)
    {
        AppendTypeToPrototype (pType->pElementType, sb);
        sb += L"*";
    }
}

////////////////////////////////////////////////////////////////////////////////
// BASENODE::GetGlyph

long BASENODE::GetGlyph ()
{
    long        iGlyph = IMG_ERROR;
    BASENODE    *pAcc = this;

    switch (this->kind)
    {
        case NK_CLASS:      iGlyph = IMG_CLASS;         break;
        case NK_STRUCT:     iGlyph = IMG_STRUCT;        break;
        case NK_INTERFACE:  iGlyph = IMG_INTERFACE;     break;
        case NK_ENUM:       iGlyph = IMG_ENUM;          break;
        case NK_DELEGATE:   iGlyph = IMG_DELEGATE;      break;
        case NK_METHOD:
        case NK_CTOR:
        case NK_DTOR:       iGlyph = IMG_METHOD;        break;

        case NK_NAMESPACE:  return IMG_NAMESPACE;
        case NK_ENUMMBR:    return IMG_ENUMMEMBER;
        case NK_INDEXER:
        case NK_PROPERTY:   iGlyph = IMG_PROPERTY;      break;
        case NK_OPERATOR:   iGlyph = IMG_OPERATOR;      break;
        case NK_VARDECL:
        {
            BASENODE *p;
            for (p = this->pParent; !p->InGroup (NG_FIELD); p = p->pParent)
                ;

            if (!p)
                return IMG_ERROR;

            iGlyph = p->kind == NK_FIELD ? IMG_FIELD : IMG_CONSTANT;
            pAcc = p;
            break;
        }

        default:
            return IMG_ERROR;
    }

    if (pAcc->pParent != NULL && pAcc->pParent->kind == NK_INTERFACE)
        return iGlyph;      // Everything in an interface is public

    DWORD   dwFlags = pAcc->flags & NF_MOD_ACCESSMODIFIERS;

    if (dwFlags & NF_MOD_INTERNAL)
        iGlyph += IMG_ACC_INTERNAL;
    else if (dwFlags & NF_MOD_PROTECTED)
        iGlyph += IMG_ACC_PROTECTED;
    else if ((dwFlags & NF_MOD_PUBLIC) == 0)
        iGlyph += IMG_ACC_PRIVATE;     // Specifying nothing defaults to private

    return iGlyph;
}


////////////////////////////////////////////////////////////////////////////////
// REVIEW:  Find another home for these functions?

////////////////////////////////////////////////////////////////////////////////
// LEXDATA::GetDocCommentXML

HRESULT LEXDATA::GetDocCommentXML (long iFirst, long iCount, BSTR *pbstrXML)
{
    if (iFirst < 0 || iFirst + iCount > this->iComments)
        return E_INVALIDARG;

    CStringBuilder      sb;
    POSDATA             *ppos = this->pposComments + iFirst;

    sb.Append (L"<doc>\r\n");
    for (long i=0; i<iCount; i++)
    {
        PCWSTR  psz = this->TextAt (*ppos) + 3;

        if (*psz == ' ' || *psz == '\t')
            psz++;

        size_t  iLen = (ppos->iLine < (unsigned)this->iLines) ? this->TextAt (ppos->iLine + 1, 0) - psz : (size_t)-1;

        sb.Append (psz, iLen);
        ppos++;
    }

    sb.Append (L"</doc>");
    return sb.CreateBSTR (pbstrXML);
}

////////////////////////////////////////////////////////////////////////////////
// LEXDATA::FindFirstTokenOnLine
//
// This function finds the first token on the given line.  If the line contains
// tokens, the return value will be S_OK -- if there are no tokens on the line,
// the return value will be S_FALSE.  *piFirst will always be set to a token
// index; if S_FALSE, the index will be the first token prior to iLine, which
// could be -1 (if all tokens are beyond iLine).
//
// Note that piFirst can be NULL, in which case this function serves as a
// "Are there any tokens on this line?" function.

HRESULT LEXDATA::FindFirstTokenOnLine (long iLine, long *piFirst)
{
    POSDATA pos(iLine, 0);
    long    iTok = POSDATA::FindNearestPosition (pposTokens, iTokens, pos);
    long    iIndex = -1;
    HRESULT hr = S_FALSE;

    // iTok will either be/start on iLine, or a line < iLine.  If it is on iLine
    // then we're done (there's a token a column zero, which is actually very
    // rare...)
    if (iTok >= 0 && pposTokens[iTok].iLine == (unsigned long) iLine)
    {
        // Boom -- we're done.
        iIndex = iTok;
        hr = S_OK;
    }
    else
    {
        // Look at the next token -- if it starts on iLine, we're golden.
        if (iTok + 1 < iTokens && pposTokens[iTok+1].iLine == (unsigned long) iLine)
        {
            iIndex = iTok + 1;
            hr = S_OK;
        }
        else
        {
            // No dice -- there are no tokens on this line.
            iIndex = iTok;
            hr = S_FALSE;
        }
    }

    if (piFirst != NULL)
        *piFirst = iIndex;

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
// LEXDATA::FindFirstNonWhiteChar

HRESULT LEXDATA::FindFirstNonWhiteChar (long iLine, long *piChar)
{
    if (iLine < 0 || iLine >= this->iLines)
        return E_INVALIDARG;

    long    iChar = 0;

    PCWSTR psz;
    for (psz = this->TextAt (iLine, 0); IsWhitespaceChar (*psz); psz++)
        iChar++;

    *piChar = iChar;
    return (IsEndOfLineChar (*psz) || *psz == 0) ? S_FALSE : S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// LEXDATA::FindEndOfComment

HRESULT LEXDATA::FindEndOfComment (long iComment, POSDATA *pposEnd)
{
    if (iComment < 0 || iComment >= this->iComments)
        return E_INVALIDARG;

    POSDATA     p = this->pposComments[iComment];
    if (this->CharAt (p.iLine, p.iChar + 1) == '/')
    {
        // Single-line comment -- the end is the end of the line.
        p.iChar = this->GetLineLength (p.iLine);
        *pposEnd = p;
        return S_OK;
    }

    if (this->CharAt (p.iLine, p.iChar + 1) != '*')
    {
        VSFAIL ("Malformed comment!");
        return E_FAIL;
    }

    // Multi-line comment -- scan until we find a '*'-'/' sequence
    long    iCharStart = p.iChar + 2;
    for (long iLine = p.iLine; iLine < this->iLines; iLine++)
    {
        PCWSTR  psz = this->TextAt (iLine, iCharStart);
        PCWSTR  pszEnd = (iLine == this->iLines - 1) ? psz + wcslen (psz) : this->TextAt (iLine + 1, 0);

        while (psz < pszEnd)
        {
            if (*psz == '*' && psz[1] == '/')
            {
                // Here's the end...
                psz += 2;
                pposEnd->iLine = iLine;
                pposEnd->iChar = psz - this->TextAt (iLine, 0);
                return S_OK;
            }
            psz++;
        }

        iCharStart = 0;
    }

    // Getting here means the comment didn't end.  Indicate this by S_FALSE, and
    // supply the position of the EOL marker
    *pposEnd = this->pposTokens[this->iTokens-1];
    return S_FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// LEXDATA::GetPreprocessorDirective
//
// If the given line is a preprocessor line, this function returns the PPTOKENID
// of the directive (i.e. PPT_DEFINE, etc.).  If not, this will return E_FAIL.

HRESULT LEXDATA::GetPreprocessorDirective (long iLine, ICSNameTable *pNameTable, PPTOKENID *piToken, long *piStart, long *piEnd)
{
    if (iLine < 0 || iLine >= this->iLines)
        return E_INVALIDARG;

    PCWSTR  psz = this->TextAt (iLine, 0), pszLine = psz;

    // Skip whitespace...
    while (IsWhitespaceChar (*psz))
        psz++;

    *piStart = (long)(psz - pszLine);

    // Check for '#' -- if not there, fail
    if (*psz++ != '#')
        return E_FAIL;

    // Skip more whitespace...
    while (IsWhitespaceChar (*psz))
        psz++;

    // Scan an identifier
    if (!IsIdentifierChar (*psz))
        return E_FAIL;

    PCWSTR  pszStart = psz;

    while (IsIdentifierCharOrDigit (*psz))
        psz++;

    *piEnd = (long)(psz - pszLine);

    NAME    *pName;
    HRESULT hr;
    long    iToken;

    // Make a name out of it and see if it's a preprocessor keyword
    if (SUCCEEDED (hr = pNameTable->AddLen (pszStart, (long)(psz - pszStart), &pName)) &&
        SUCCEEDED (hr = pNameTable->GetPreprocessorDirective (pName, &iToken)))
    {
        *piToken = (PPTOKENID)iToken;
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
// LEXDATA::GetLineLength

long LEXDATA::GetLineLength (long iLine)
{
    if (iLine < 0 || iLine >= this->iLines)
        return 0;

    if (iLine == this->iLines - 1)
        return (long)wcslen (this->TextAt (iLine, 0));

    PCWSTR  psz = this->TextAt (iLine + 1, 0) - 1, pszThis = this->TextAt (iLine, 0);

    while (psz > pszThis && IsEndOfLineChar (*(psz-1)))
        psz--;

    return (long)(psz - pszThis);
}

////////////////////////////////////////////////////////////////////////////////
// LEXDATA::IsWhitespace

BOOL LEXDATA::IsWhitespace (long iStartLine, long iStartChar, long iEndLine, long iEndChar)
{
    if (iStartLine < 0 || iEndLine >= this->iLines || iStartLine > iEndLine)
        return FALSE;

    PCWSTR  psz = this->TextAt (iStartLine, iStartChar);
    PCWSTR  pszStop = this->TextAt (iEndLine, iEndChar);

    while (psz < pszStop)
    {
        if (!IsWhitespaceChar (*psz) && !IsEndOfLineChar (*psz))
            return FALSE;
        psz++;
    }

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
// POSDATA::FindNearestPosition
//
// Given an array of POSDATA's and a single position, this function finds that
// position in the array using a binary search and returns its index.  If the
// position doesn't exist in the array, this function returns the index of last
// entry PRIOR to the given position (obviously assuming that the array is
// sorted).  The return value can thus be -1 (meaning all entries in the array
// are beyond the given position).
//
// (NOTE:  This is a static function; it is a member of POSDATA purely as a
// matter of organization.)

long POSDATA::FindNearestPosition (POSDATA *ppos, long iSize, const POSDATA &pos)
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
