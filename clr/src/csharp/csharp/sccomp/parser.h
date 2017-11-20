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
// File: parser.h
//
// ===========================================================================

#ifndef __parser_h__
#define __parser_h__

#include "errorids.h"
#include "scciface.h"
#include "tokinfo.h"
#include "alloc.h"
#include "nodes.h"
#include "locks.h"
#include "namemgr.h"
#include "lexer.h"
#include "controller.h"

class CListMaker;
class CSourceData;

////////////////////////////////////////////////////////////////////////////////
// CParser
//
// This is the base class for the CSharp parser.  Like CLexer, it is abstract;
// you must create a derivation and implement the pure virtuals.

class CParser
{
protected:
    CController     *m_pController;
    long            m_iCurTok;
    BYTE            *m_pTokens;
    POSDATA         *m_pposTokens;
    long            m_iTokens;
    PCWSTR          m_pszSource;
    long            *m_piLines;
    long            m_iLines;
    BOOL            m_fErrorOnCurTok;
    long            m_iErrors;

    NAME            *m_pMissingName;                // Name used when identifiers aren't where they were expected ("?")
    NAME            *m_pGetName;                    // Name for 'get' pseudo-keyword
    NAME            *m_pSetName;                    // Name for 'set' pseudo-keyword
    NAME            *m_pAddName;                    // Name for 'add' pseudo-keyword
    NAME            *m_pRemoveName;                 // Name for 'remove' pseudo-keyword

    enum SCANTYPE
    {
        NOT_TYPE,
        TYPE_OR_EXPR,
        POINTER_OR_MULT,
        MUST_BE_TYPE
    };

    enum PARSEDECLFLAGS
    {
        PVDF_LOCAL  = 0x0001,           // Local variable declaration (i.e. not a field of a type)
        PVDF_CONST  = 0x0002,           // Variable is CONST
    };

    NAMEMGR         *GetNameMgr() { return m_pController->GetNameMgr(); }

public:
    CParser (CController *pController);
    virtual ~CParser ();

    // Initialization
    void            SetInputData (BYTE *pTokens, POSDATA *pposTokens, long iTokens, PCWSTR pszSource, long *piLines, long iLines);
    void            SetInputData (LEXDATA *pLexData);

    // Simple parsing/token indexing helpers
    TOKENID         CurToken() { return (TOKENID)m_pTokens[m_iCurTok]; }
    POSDATA         &CurTokenPos() { return m_pposTokens[m_iCurTok]; }
    TOKENID         NextToken () { if (m_iCurTok < m_iTokens - 1) { m_iCurTok++; m_fErrorOnCurTok = FALSE; } return CurToken(); }
    TOKENID         PrevToken () { if (m_iCurTok > 0) { m_iCurTok--; m_fErrorOnCurTok = FALSE; } return CurToken(); }
    TOKENID         PeekToken (long iPeek = 1) { return (TOKENID)m_pTokens[m_iCurTok + iPeek]; }    // NOTE:  Use carefully!  No overflow checking!
    long            Mark () { return m_iCurTok; }
    void            Rewind (long iMark) { m_iCurTok = iMark; m_fErrorOnCurTok = FALSE; }

    // More advanced (non-inlined) parsing/token indexing helpers
    void            Eat (TOKENID iToken);
    BOOL            CheckToken (TOKENID iToken);
    void            RescanToken (long iToken, FULLTOKEN *pFT);
    long            GetTokenLength (long iTokenIdx);
    void            GetTokenStopPosition (long iTokenIdx, POSDATA *pposStop);

    // Error reporting
    void __cdecl    Error (enum ERRORIDS iErrorId, ...);
    void __cdecl    ErrorAtToken (long iTokenIdx, enum ERRORIDS iErrorId, ...);
    void __cdecl    ErrorAfterPrevToken (enum ERRORIDS iErrorId, ...);
    void __cdecl    ErrorAtPosition (long iLine, long iCol, long iExtent, enum ERRORIDS iErrorId, ...);
    enum ERRORIDS   ExpectedErrorFromToken (TOKENID iToken, TOKENID iTokenActual);
    PCWSTR          GetTokenText (long iTokenIdx);
    void            ErrorInvalidMemberDecl(long iTokenIdx);

    // Top-level parse/skip/scan helpers
    BOOL            SkipToNamespaceElement ();
    BOOL            SkipToMember ();
    DWORD           SkipBlock (DWORD dwFlags, long *piClose);
    DWORD           ScanMemberDeclaration (DWORD dwFlags);
    BOOL            ScanParameterList (BOOL *pfHadParms);
    BOOL            ScanCast ();
    SCANTYPE        ScanType ();

    // Literals
    WCHAR           ScanEscapeSequence (PCWSTR &p, WCHAR *surrogatePair);
    STRCONST        *ScanStringLiteral (long iTokenIndex);
    WCHAR           ScanCharLiteral (long iTokenIndex);
    void            ScanNumericLiteral (long iTokenIndex, CONSTVALNODE *pNode);

    // Node allocation
    BASENODE        *AllocNode (NODEKIND kind);
    BINOPNODE       *AllocDotNode (long iTokIdx, BASENODE *pParent, BASENODE *p1, BASENODE *p2);
    BINOPNODE       *AllocBinaryOpNode (OPERATOR op, long iTokIdx, BASENODE *pParent, BASENODE *p1, BASENODE *p2);
    UNOPNODE        *AllocUnaryOpNode (OPERATOR op, long iTokIdx, BASENODE *pParent, BASENODE *pArg);
    BASENODE        *AllocOpNode (OPERATOR op, long iTokIdx, BASENODE *pParent);
    NAMENODE        *AllocNameNode (NAME *pName, long iTokIdx);

    // Top-level parsing methods
    BASENODE        *ParseSourceModule ();
    void            ParseNamespaceBody (NAMESPACENODE *pNS);
    BASENODE        *ParseUsingClause (BASENODE *pParent);
    BASENODE        *ParseGlobalAttributes (BASENODE *pParent);
    BASENODE        *ParseNamespace (BASENODE *pParent);
    BASENODE        *ParseTypeDeclaration (BASENODE *pParent, BASENODE *pAttr);
    BASENODE        *ParseClassOrStruct (BASENODE *pParent, long iTokIdx, BASENODE *pAttrs, unsigned iMods);
    BASENODE        *ParseDelegate (BASENODE *pParent, long iTokIdx, BASENODE *pAttrs, unsigned iMods);
    BASENODE        *ParseEnum (BASENODE *pParent, long iTokIdx, BASENODE *pAttrs, unsigned iMods);
    BASENODE        *ParseInterface (BASENODE *pParent, long iTokIdx, BASENODE *pAttrs, unsigned iMods);


    MEMBERNODE      *ParseMember (CLASSNODE *pParent);
    MEMBERNODE      *ParseConstructor (CLASSNODE *pParent, long iTokIdx, BASENODE *pAttr, unsigned iMods);
    MEMBERNODE      *ParseDestructor (CLASSNODE *pParent, long iTokIdx, BASENODE *pAttr, unsigned iMods);
    MEMBERNODE      *ParseConstant (CLASSNODE *pParent, long iTokIdx, BASENODE *pAttr, unsigned iMods);
    MEMBERNODE      *ParseMethod (CLASSNODE *pParent, long iTokIdx, BASENODE *pAttr, unsigned iMods, TYPENODE *pType);
    MEMBERNODE      *ParseProperty (NODEKIND nodekind, CLASSNODE *pParent, long iTokIdx, BASENODE *pAttr, unsigned iMods, TYPENODE *pType, bool isEvent);
    MEMBERNODE      *ParseField (CLASSNODE *pParent, long iTokIdx, BASENODE *pAttr, unsigned iMods, TYPENODE *pType, bool isEvent);
    MEMBERNODE      *ParseOperator (CLASSNODE *pParent, long iTokIdx, BASENODE *pAttr, unsigned iMods, TYPENODE *pType);

    TYPENODE        *ParseType (BASENODE *pParent);
    TYPENODE        *ParseClassType (BASENODE *pParent);
    TYPENODE        *ParseReturnType (BASENODE *pParent);
    BASENODE        *ParseTypeNameList (BASENODE *pParent);
    NAMENODE        *ParseIdentifier (BASENODE *pParent);
    NAMENODE        *ParseIdentifierOrKeyword (BASENODE *pParent);
    BASENODE        *ParseName (BASENODE *pParent);
    NAMENODE        *ParseMissingName (NAMENODE *pName, BASENODE *pParent, long iTokIndex);
    BASENODE        *ParseIndexerName (BASENODE *pParent);
    unsigned        ParseModifiers (BOOL bReportDups);

    BASENODE        *ParseParameter (BASENODE *pParent, bool noName = false);
    BASENODE        *ParseParameterOrVarargs (BASENODE *pParent);
    void             ParseParameterOrVarargsList (BASENODE *pParent, BASENODE ** ppParams);
    BASENODE        *ParseAttributeSection (BASENODE *pParent, ATTRLOC defaultLocation, ATTRLOC validLocations);
    BASENODE        *ParseAttributes (BASENODE *pParent, ATTRLOC defaultLocation, ATTRLOC validLocations);
    BASENODE        *ParseAttribute (BASENODE *pParent);
    BASENODE        *ParseAttributeArgument (BASENODE *pParent, BOOL *pfNamed);
    BASENODE        *SetDefaultAttributeLocation(BASENODE *pAttrList, ATTRLOC defaultLocation, ATTRLOC validLocations);

    METHODNODE      *ParseMemberRef ();


    // Interior node parsing methods
    STATEMENTNODE   *CallStatementParser (DWORD dwStmtParserKind, BASENODE *pParent);
    STATEMENTNODE   *ParseStatement (BASENODE *pParent);
    STATEMENTNODE   *ParseBlock (BASENODE *pParent);
    STATEMENTNODE   *ParseBreakStatement (BASENODE *pParent);
    STATEMENTNODE   *ParseConstStatement (BASENODE *pParent);
    STATEMENTNODE   *ParseLabeledStatement (BASENODE *pParent);
    STATEMENTNODE   *ParseDoStatement (BASENODE *pParent);
    STATEMENTNODE   *ParseForStatement (BASENODE *pParent);
    STATEMENTNODE   *ParseForEachStatement (BASENODE *pParent);
    STATEMENTNODE   *ParseGotoStatement (BASENODE *pParent);
    STATEMENTNODE   *ParseIfStatement (BASENODE *pParent);
    STATEMENTNODE   *ParseReturnStatement (BASENODE *pParent);
    STATEMENTNODE   *ParseSwitchStatement (BASENODE *pParent);
    STATEMENTNODE   *ParseThrowStatement (BASENODE *pParent);
    STATEMENTNODE   *ParseTryStatement (BASENODE *pParent);
    STATEMENTNODE   *ParseWhileStatement (BASENODE *pParent);
    STATEMENTNODE   *ParseDeclaration (BASENODE * pParent);
    STATEMENTNODE   *ParseDeclarationStatement (BASENODE *pParent);
    STATEMENTNODE   *ParseExpressionStatement (BASENODE *pParent);
    STATEMENTNODE   *ParseLockStatement (BASENODE *pParent);
    STATEMENTNODE   *ParseFixedStatement (BASENODE *pParent);
    STATEMENTNODE   *ParseUsingStatement (BASENODE *pParent);
    STATEMENTNODE   *ParseCheckedStatement (BASENODE *pParent);
    STATEMENTNODE   *ParseEmbeddedStatement (BASENODE *pParent, BOOL fComplexCheck);
    BASENODE        *ParseVariableDeclarator (BASENODE *pParent, BASENODE *pParentDecl, DWORD dwPVDFlags, BOOL fFirst);
    BASENODE        *ParseVariableInitializer (BASENODE *pParent, BOOL fAllowStackAlloc);
    BASENODE        *ParseArrayInitializer (BASENODE *pParent);
    BASENODE        *ParseStackAllocExpression (BASENODE *pParent);
    BASENODE        *ParseSwitchCase (BASENODE *pParent);
    BASENODE        *ParseCatchClause (BASENODE *pParent, BOOL *pfEmpty);
    BASENODE        *ParseExpression (BASENODE *pParent) { BASENODE *p = ParseSubExpression (pParent, 0, NULL); p->pParent = pParent; return p; }
    BASENODE        *ParseSubExpression (BASENODE *pParent, UINT iPrecedence, BASENODE *pLeftOperand);
    BASENODE        *ParseTerm (BASENODE *pParent);
    BASENODE        *ParsePostFixOperator (BASENODE *pParent, BASENODE *pExpr);
    BASENODE        *ParseCastOrExpression (BASENODE *pParent);
    BASENODE        *ParseNumber (BASENODE *pParent);
    BASENODE        *ParseStringLiteral (BASENODE *pParent);
    BASENODE        *ParseCharLiteral (BASENODE *pParent);
    BASENODE        *ParseNewExpression (BASENODE *pParent);
    BASENODE        *ParseArgument (BASENODE *pParent);
    BASENODE        *ParseArgumentList (BASENODE *pParent);
    BASENODE        *ParseDimExpressionList (BASENODE *pParent);
    BASENODE        *ParseExpressionList (BASENODE *pParent);

    // Extent/Leaf calculators
    void            GetExtent (BASENODE *pNode, POSDATA *pposStart, POSDATA *pposEnd, ExtentFlags flags);
    BASENODE        *FindContainingMember (POSDATA pos, BASENODE *pMember);
    BASENODE        *FindContainingStatement (POSDATA pos, BASENODE *pStatement);
    BASENODE        *FindLeaf (const POSDATA pos, BASENODE *pNode, CSourceData *pData, ICSInteriorTree **ppTree);

    // Here are the pure virtuals that must be implemented by parser implementations
    virtual void    *MemAlloc (long iSize) = 0;
    virtual void    CreateNewError (short iErrorId, va_list args, const POSDATA &posStart, const POSDATA &posEnd) = 0;
    virtual void    AddToNodeTable (BASENODE *pNode) = 0;
    virtual void    GetInteriorTree (CSourceData *pData, BASENODE *pNode, ICSInteriorTree **ppTree) = 0;

    // Static token/operator information grabbers
    static  const   TOKINFO m_rgTokenInfo[TID_NUMTOKENS];
    static  const   OPINFO  m_rgOpInfo[];
    static  const   TOKINFO         *GetTokenInfo (TOKENID iTok) { return m_rgTokenInfo + iTok; }
    static  const   OPINFO          *GetOperatorInfo (OPERATOR iOp) { return m_rgOpInfo + iOp; }
    static          BOOL            IsRightAssociative (OPERATOR iOp) { return m_rgOpInfo[iOp].fRightAssoc; }
    static          UINT            GetOperatorPrecedence (OPERATOR iOp) { return m_rgOpInfo[iOp].iPrecedence; }
    static          UINT            GetCastPrecedence () { return 14; } // HACK:!
    static          BOOL            IsUnaryOperator (TOKENID iTok, OPERATOR *piOp, UINT *piPrecedence);
    static          BOOL            IsBinaryOperator (TOKENID iTok, OPERATOR *piOp, UINT *piPrecedence);
    static          long            GetFirstToken (BASENODE *pNode, ExtentFlags flags, BOOL *pfMissingName);
    static          long            GetLastToken (BASENODE *pNode, ExtentFlags flags, BOOL *pfMissingName);
    static          STATEMENTNODE   *GetLastStatement (STATEMENTNODE *pStmt);

};

////////////////////////////////////////////////////////////////////////////////
// CParser::IsUnaryOperator

inline BOOL CParser::IsUnaryOperator (TOKENID iTok, OPERATOR *piOp, UINT *piPrecedence)
{
    if (m_rgTokenInfo[iTok].iUnaryOp != OP_NONE && (m_rgTokenInfo[iTok].dwFlags & TFF_OVLOPKWD) == 0)
    {
        *piOp = (OPERATOR)m_rgTokenInfo[iTok].iUnaryOp;
        *piPrecedence = m_rgOpInfo[*piOp].iPrecedence;
        return TRUE;
    }
    return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// CParser::IsBinaryOperator

inline BOOL CParser::IsBinaryOperator (TOKENID iTok, OPERATOR *piOp, UINT *piPrecedence)
{
    if (m_rgTokenInfo[iTok].iBinaryOp != OP_NONE && (m_rgTokenInfo[iTok].dwFlags & TFF_OVLOPKWD) == 0)
    {
        if (piOp != NULL)
        {
            *piOp = (OPERATOR)m_rgTokenInfo[iTok].iBinaryOp;
        }
        if (piPrecedence != NULL)
        {
            *piPrecedence = m_rgOpInfo[*piOp].iPrecedence;
        }
        return TRUE;
    }
    return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// CSimpleParser

class CSimpleParser : public CParser
{
private:
    NRHEAP      m_heap;

public:
    CSimpleParser (CController *pController) : CParser (pController), m_heap(pController) {}

    // CParser
    void    *MemAlloc (long iSize) { return m_heap.Alloc (iSize); }
    void    CreateNewError (short iErrorId, va_list args, const POSDATA &posStart, const POSDATA &posEnd) {}
    void    AddToNodeTable (BASENODE *pNode) {}
    void    GetInteriorTree (CSourceData *pData, BASENODE *pNode, ICSInteriorTree **ppTree) {}
};

////////////////////////////////////////////////////////////////////////////////
// CTextParser
//
// This is the object that exposes ICSParser for external clients

class CTextParser :
    public CComObjectRootMT,
    public ICSParser
{
private:
    CSimpleParser       *m_pParser;
    CComPtr<ICSLexer>   m_spLexer;

public:
    BEGIN_COM_MAP(CTextParser)
        COM_INTERFACE_ENTRY(ICSParser)
    END_COM_MAP()

    CTextParser ();
    ~CTextParser ();

    HRESULT     Initialize (CController *pController);

    // ICSParser
    STDMETHOD(SetInputText)(PCWSTR pszInputText, long iLen);
    STDMETHOD(SetInputStream)(BYTE *pTokens, POSDATA *pposTokens, long iTokens, PCWSTR *ppszLines, long iLines);
    STDMETHOD(CreateParseTree)(ParseTreeScope iScope, BASENODE *pParent, BASENODE **ppNode);
    STDMETHOD(AllocateNode)(long iKind, BASENODE **ppNode);
};

#endif //__parser_h__

