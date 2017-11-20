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
// File: dumpnode.h
//
// ===========================================================================

// NOTE:  This is actually an implementation file, it is just included into
// SRCMOD.CPP to keep it from making the file too big.  Debug only.

#define NODEKIND(k,t,g) "NK_" #k,
PCSTR rgszKind[] = {
#include "nodekind.h"
};

#define OP(n,p,a,stmt,t,pn,e) "OP_" #n,
PCSTR rgszOpName[] = {
#include "ops.h"
};

// make an indentation string
PSTR Spaces (int iIndent)
{
    static  CHAR    szBuf[256];
    memset (szBuf, ' ', iIndent * 2);
    szBuf[iIndent*2] = 0;
    return szBuf;
}

PWSTR BuildQualifiedName (BASENODE *pName)
{
    static  WCHAR   szBuf[1024];
    WCHAR   *psz = szBuf;

    while (pName->asDOT()->p1->kind == NK_DOT)
        pName = pName->asDOT()->p1;

    wcscpy (psz, pName->asDOT()->p1->asNAME()->pName->text);
    psz += wcslen (psz);
    while (pName->kind == NK_DOT)
    {
        *psz++ = '.';
        wcscpy (psz, pName->asDOT()->p2->asNAME()->pName->text);
        psz += wcslen (psz);
       pName = pName->pParent;
    }
    return szBuf;
}

void Heading (PSTR psz, int iIndent)
{
    printf ("%s%s:\n", Spaces(iIndent), psz);
}

#define HEADING(t) Heading (t, iIndent)
#define DUMPNEXT(p) \
            if (p != NULL) \
            { \
                printf ("%s----------\n", Spaces(iIndent)); \
                iIndent--; \
                DumpTree (p); \
                iIndent++; \
            }


void DumpTree (BASENODE *pNode)
{
    static  int iIndent = -1;

    if (pNode == NULL)
    {
        printf ("%s<null>\n", Spaces(iIndent+1));
        return;
    }

    iIndent++;

    // general node data                                                       
    printf ("%s%06X: %s (tokidx %d)\n", Spaces(iIndent), pNode->flags, rgszKind[pNode->kind], pNode->tokidx);

    // specifics
    switch (pNode->kind)
    {
        case NK_ACCESSOR          :    // ACCESSOR
            DumpTree (pNode->asACCESSOR()->pBody);
            break;

        case NK_ARRAYINIT         :    // UNOP
            DumpTree (((UNOPNODE *)pNode)->p1);
            break;

        case NK_ATTR              :    // ATTR
        case NK_ATTRARG           :    // ATTR
            HEADING ("NAME");
            DumpTree (((ATTRNODE *)pNode)->pName);
            HEADING (pNode->kind == NK_ATTR ? "ARGS" : "EXPR");
            DumpTree (((ATTRNODE *)pNode)->pArgs);
            break;

        case NK_BINOP             :    // BINOP
            printf ("%sOPERATOR: %s\n", Spaces(iIndent), rgszOpName[pNode->Op()]);
            HEADING ("LEFT");
            DumpTree (pNode->asBINOP()->p1);
            HEADING ("RIGHT");
            DumpTree (pNode->asBINOP()->p2);
            break;

        case NK_BLOCK             :    // BLOCK
            DumpTree (pNode->asBLOCK()->pStatements);
            break;

        case NK_BREAK             :    // EXPRSTMT
        case NK_CONTINUE          :    // EXPRSTMT
        case NK_EXPRSTMT          :    // EXPRSTMT
        case NK_GOTO              :    // EXPRSTMT
        case NK_RETURN            :    // EXPRSTMT
        case NK_THROW             :    // EXPRSTMT
            DumpTree (((EXPRSTMTNODE *)pNode)->pArg);       // NOTE:  Can't use as*() here, because of different kinds...
            break;

        case NK_CASE              :    // BASE
            HEADING ("LABELS");
            DumpTree (pNode->asCASE()->pLabels);
            HEADING ("STATEMENTS");
            DumpTree (pNode->asCASE()->pStmts);
            break;

        case NK_CASELABEL         :    // UNOP
            DumpTree (pNode->asCASELABEL()->p1);
            break;

        case NK_CATCH             :    // CATCH
            HEADING ("TYPE");
            DumpTree (pNode->asCATCH()->pType);
            HEADING ("NAME");
            DumpTree (pNode->asCATCH()->pName);
            HEADING ("BLOCK");
            DumpTree (pNode->asCATCH()->pBlock);
            break;

        case NK_CONST             :    // CONST
            HEADING ("ATTR");
            DumpTree (((MEMBERNODE *)pNode)->pAttr);
            HEADING ("TYPE");
            DumpTree (pNode->asCONST()->pType);
            HEADING ("DECLARED");
            DumpTree (pNode->asCONST()->pDecls);
            break;

        case NK_CLASS             :    // CLASS
        case NK_ENUM              :    // CLASS
        case NK_INTERFACE         :    // CLASS
        case NK_STRUCT            :    // CLASS
            HEADING ("ATTRIBUTES");
            DumpTree (((CLASSNODE *)pNode)->pAttr);
            HEADING ("NAME");
            DumpTree (((CLASSNODE *)pNode)->pName);
            HEADING ("BASES");
            DumpTree (((CLASSNODE *)pNode)->pBases);
            HEADING ("MEMBERS");
            DumpTree (((CLASSNODE *)pNode)->pMembers);
            break;

        case NK_CONSTVAL          :    // CONSTVAL

            if (pNode->PredefType() == PT_STRING)
                printf ("%sPT_STRING: '%S'\n", Spaces(iIndent), pNode->asCONSTVAL()->val.strVal->text);
            if (pNode->PredefType() == PT_CHAR)
                printf ("%sPT_CHAR: '%c' (%d)\n", Spaces(iIndent), pNode->asCONSTVAL()->val.iVal, pNode->asCONSTVAL()->val.iVal);
            else if (pNode->PredefType() == PT_LONG)
                printf ("%sPT_LONG: %I64d (%016I64x)\n", Spaces(iIndent), *(pNode->asCONSTVAL()->val.longVal), *(pNode->asCONSTVAL()->val.longVal));
            else if (pNode->PredefType() == PT_INT)
                printf ("%sPT_INT: %d (%08x)\n", Spaces(iIndent), pNode->asCONSTVAL()->val.iVal, pNode->asCONSTVAL()->val.iVal);
            else if (pNode->PredefType() == PT_BYTE)
                printf ("%sPT_BYTE: %u (%02x)\n", Spaces(iIndent), pNode->asCONSTVAL()->val.uiVal, pNode->asCONSTVAL()->val.uiVal);
            else if (pNode->PredefType() == PT_SHORT)
                printf ("%sPT_SHORT: %d (%04x)\n", Spaces(iIndent), pNode->asCONSTVAL()->val.iVal, pNode->asCONSTVAL()->val.iVal);
            else if (pNode->PredefType() == PT_ULONG)
                printf ("%sPT_ULONG: %I64u (%016I64x)\n", Spaces(iIndent), *(pNode->asCONSTVAL()->val.ulongVal), *(pNode->asCONSTVAL()->val.ulongVal));
            else if (pNode->PredefType() == PT_UINT)
                printf ("%sPT_UINT: %u (%08x)\n", Spaces(iIndent), pNode->asCONSTVAL()->val.uiVal, pNode->asCONSTVAL()->val.uiVal);
            else if (pNode->PredefType() == PT_SBYTE)
                printf ("%sPT_SBYTE: %d (%02x)\n", Spaces(iIndent), pNode->asCONSTVAL()->val.iVal, pNode->asCONSTVAL()->val.iVal);
            else if (pNode->PredefType() == PT_USHORT)
                printf ("%sPT_USHORT: %u (%04x)\n", Spaces(iIndent), pNode->asCONSTVAL()->val.uiVal, pNode->asCONSTVAL()->val.uiVal);
            else if (pNode->PredefType() == PT_FLOAT)
                printf ("%sPT_FLOAT: %f\n", Spaces(iIndent), *(pNode->asCONSTVAL()->val.doubleVal));
            else if (pNode->PredefType() == PT_DOUBLE)
                printf ("%sPT_DOUBLE: %lf\n", Spaces(iIndent), *(pNode->asCONSTVAL()->val.doubleVal));
            else
                printf ("%sTYPE: %d\n", Spaces(iIndent), pNode->PredefType());
            break;

        case NK_DECLSTMT          :    // DECLSTMT
            HEADING ("TYPE");
            DumpTree (pNode->asDECLSTMT()->pType);
            HEADING ("DECLARED");
            DumpTree (pNode->asDECLSTMT()->pVars);
            break;

        case NK_DELEGATE          :    // DELEGATE
            HEADING ("TYPE");
            DumpTree (pNode->asDELEGATE()->pType);
            HEADING ("NAME");
            DumpTree (pNode->asDELEGATE()->pName);
            HEADING ("PARAMETERS");
            DumpTree (pNode->asDELEGATE()->pParms);
            break;

        case NK_DO                :    // LOOPSTMT
        case NK_WHILE             :    // LOOPSTMT
            HEADING ("EXPRESION");
            DumpTree (((LOOPSTMTNODE *)pNode)->pExpr);
            HEADING ("STATEMENT");
            DumpTree (((LOOPSTMTNODE *)pNode)->pStmt);
            break;

        case NK_DOT               :    // BINOP
        case NK_ARROW             :    // BINOP
            HEADING ("LEFT");
            DumpTree (((BINOPNODE *)pNode)->p1);
            HEADING ("RIGHT");
            DumpTree (((BINOPNODE *)pNode)->p2);
            break;

        case NK_EMPTYSTMT         :    // STATEMENT
            break;

        case NK_ENUMMBR           :    // ENUMMBR
            HEADING ("ATTR");
            DumpTree (((MEMBERNODE *)pNode)->pAttr);
            HEADING ("NAME");
            DumpTree (pNode->asENUMMBR()->pName);
            HEADING ("VALUE");
            DumpTree (pNode->asENUMMBR()->pValue);
            break;

        case NK_FIELD             :    // FIELD
            HEADING ("ATTR");
            DumpTree (((MEMBERNODE *)pNode)->pAttr);
            HEADING ("TYPE");
            DumpTree (pNode->asFIELD()->pType);
            HEADING ("DECLARED");
            DumpTree (pNode->asFIELD()->pDecls);
            break;

        case NK_FOR               :    // FORSTMT
            HEADING ("INIT");
            DumpTree (pNode->asFOR()->pInit);
            HEADING ("CONDITION");
            DumpTree (pNode->asFOR()->pExpr);
            HEADING ("INCREMENT");
            DumpTree (pNode->asFOR()->pInc);
            HEADING ("STATEMENT");
            DumpTree (pNode->asFOR()->pStmt);
            break;

        case NK_IF                :    // IFSTMT
            HEADING ("CONDITION");
            DumpTree (pNode->asIF()->pExpr);
            HEADING ("TRUE STATEMENT");
            DumpTree (pNode->asIF()->pStmt);
            HEADING ("FALSE STATEMENT");
            DumpTree (pNode->asIF()->pElse);
            break;

        case NK_LABEL             :    // LABELSTMT
            HEADING ("LABEL");
            DumpTree (pNode->asLABEL()->pLabel);
            HEADING ("STATEMENT");
            DumpTree (pNode->asLABEL()->pStmt);
            break;

        case NK_LIST              :    // BINOP
            HEADING ("P1");
            DumpTree (pNode->asLIST()->p1);
            HEADING ("P2");
            DumpTree (pNode->asLIST()->p2);
            break;

        case NK_MEMBER            :    // MEMBER
            ASSERT (FALSE);     // No node should use this node kind...
            break;

        case NK_CTOR              :    // METHOD
        case NK_DTOR              :    // METHOD
        case NK_METHOD            :    // METHOD
        case NK_OPERATOR          :    // OPERATOR
            HEADING ("ATTR");
            DumpTree (((MEMBERNODE *)pNode)->pAttr);
            if (pNode->kind == NK_METHOD)
            {
                HEADING ("NAME");
                DumpTree (((METHODNODE *)pNode)->pName);
            }
            if (pNode->kind == NK_OPERATOR)
            {
                printf ("%sOPERATOR: %s\n", Spaces(iIndent), rgszOpName[((METHODNODE *)pNode)->iOp]);
            }
            if (pNode->kind != NK_CTOR && pNode->kind != NK_DTOR)
            {
                HEADING ("RETURN TYPE");
                DumpTree (((METHODNODE *)pNode)->pType);
            }
            HEADING ("PARAMETERS");
            DumpTree (((METHODNODE *)pNode)->pParms);
            if (pNode->kind == NK_CTOR && (pNode->other & (NFEX_CTOR_BASE|NFEX_CTOR_THIS)))
            {
                HEADING ((pNode->other & NFEX_CTOR_BASE) ? "ARGS TO BASE" : "ARGS TO THIS");
                DumpTree (((METHODNODE *)pNode)->pCtorArgs);
            }

            HEADING ("BODY");
            DumpTree (((METHODNODE *)pNode)->pBody);
            break;

        case NK_NAME              :    // NAME
            printf ("%sNAME: \"%S\"\n", Spaces(iIndent), pNode->asNAME()->pName->text);
            break;

        case NK_NAMESPACE         :    // NAMESPACE
            HEADING ("NAME");
            DumpTree (pNode->asNAMESPACE()->pName);
            HEADING ("USING");
            DumpTree (pNode->asNAMESPACE()->pUsing);
            HEADING ("ELEMENTS");
            DumpTree (pNode->asNAMESPACE()->pElements);
            break;

        case NK_NESTEDTYPE        :    // NESTEDTYPE
            DumpTree (pNode->asNESTEDTYPE()->pType);
            break;

        case NK_NEW:
            HEADING ("TYPE");
            DumpTree (pNode->asNEW()->pType);
            HEADING ("ARGS/DIMS");
            DumpTree (pNode->asNEW()->pArgs);
            HEADING ("INIT");
            DumpTree (pNode->asNEW()->pInit);
            break;

        case NK_OP                :    // BASE
            printf ("%sOPERATOR: %s\n", Spaces(iIndent), rgszOpName[pNode->Op()]);
            break;

        case NK_PARAMETER         :    // PARAMETER
            HEADING ("ATTR");
            DumpTree (pNode->asPARAMETER()->pAttr);
            HEADING ("TYPE");
            DumpTree (pNode->asPARAMETER()->pType);
            HEADING ("NAME");
            DumpTree (pNode->asPARAMETER()->pName);
            break;

        case NK_PARTIALMEMBER     :    // PARTIALMEMBER
            HEADING ("ATTR");
            DumpTree (((MEMBERNODE *)pNode)->pAttr);
            HEADING ("PARTIAL TREE");
            DumpTree (pNode->asPARTIALMEMBER()->pNode);
            break;

        case NK_INDEXER           :    // INDEXER
        case NK_PROPERTY          :    // PROPERTY
            HEADING ("ATTR");
            DumpTree (((MEMBERNODE *)pNode)->pAttr);
            HEADING ("TYPE");
            DumpTree (pNode->asANYPROPERTY()->pType);
            HEADING ("NAME");
            DumpTree (pNode->asANYPROPERTY()->pName);
            HEADING ("PARAMETERS");
            DumpTree (pNode->asANYPROPERTY()->pParms);
            HEADING ("GET ACCESSOR");
            DumpTree (pNode->asANYPROPERTY()->pGet);
            HEADING ("SET ACCESSOR");
            DumpTree (pNode->asANYPROPERTY()->pSet);
            break;

        case NK_TRY               :    // TRYSTMT
            HEADING ("GUARDED STATMENT");
            DumpTree (pNode->asTRY()->pBlock);
            if (pNode->flags & NF_TRY_CATCH)
                HEADING ("CATCHES");
            else
                HEADING ("FINALLY");
            DumpTree (pNode->asTRY()->pCatch);
            break;

        case NK_TYPE              :    // TYPE
            switch (pNode->TypeKind())
            {
                case TK_ARRAY:
                    printf ("%sARRAY[%d]:\n", Spaces(iIndent), pNode->asTYPE()->iDims);
                    DumpTree (pNode->asTYPE()->pElementType);
                    break;
                case TK_POINTER:
                    HEADING ("POINTER");
                    DumpTree (pNode->asTYPE()->pElementType);
                    break;
                case TK_NAMED:
                    HEADING ("NAMED");
                    DumpTree (pNode->asTYPE()->pName);
                    break;
                case TK_PREDEFINED:
                    printf ("%sPREDEFINED TYPE %d\n", Spaces(iIndent), pNode->asTYPE()->iType);
                    break;
            }
            break;

        case NK_SWITCH            :    // SWITCHSTMT
            HEADING ("EXPRESSION");
            DumpTree (pNode->asSWITCH()->pExpr);
            HEADING ("CASES");
            DumpTree (pNode->asSWITCH()->pCases);
            break;

        case NK_UNOP              :    // UNOP
            printf ("%sOPERATOR: %s\n", Spaces(iIndent), rgszOpName[pNode->Op()]);
            HEADING ("OPERAND");
            DumpTree (pNode->asUNOP()->p1);
            break;

        case NK_USING             :    // USING
            HEADING ("NAME");
            DumpTree (pNode->asUSING()->pName);
            HEADING ("ALIAS");
            DumpTree (pNode->asUSING()->pAlias);
            break;

        case NK_VARDECL           :    // VARDECL
            HEADING ("NAME");
            DumpTree (pNode->asVARDECL()->pName);
            HEADING ("INIT EXPR");
            DumpTree (pNode->asVARDECL()->pArg);
            break;
    }

    // Check for nodes that have 'next' pointers
    switch (pNode->kind)
    {
        // Statements
        case NK_BLOCK             :    // BLOCK
        case NK_BREAK             :    // EXPRSTMT
        case NK_CONTINUE          :    // EXPRSTMT
        case NK_EXPRSTMT          :    // EXPRSTMT
        case NK_GOTO              :    // EXPRSTMT
        case NK_RETURN            :    // EXPRSTMT
        case NK_THROW             :    // EXPRSTMT
        case NK_DECLSTMT          :    // DECLSTMT
        case NK_DO                :    // LOOPSTMT
        case NK_EMPTYSTMT         :    // STATEMENT
        case NK_FOR               :    // FORSTMT
        case NK_IF                :    // IFSTMT
        case NK_LABEL             :    // LABELSTMT
        case NK_TRY               :    // TRYSTMT
        case NK_SWITCH            :    // SWITCHSTMT
        case NK_WHILE             :    // LOOPSTMT
            DUMPNEXT (((STATEMENTNODE *)pNode)->pNext);
            break;

        // Members
        case NK_CONST             :    // CONST
        case NK_CTOR              :    // METHOD
        case NK_DTOR              :    // METHOD
        case NK_ENUMMBR           :    // ENUMMBR
        case NK_FIELD             :    // FIELD
        case NK_METHOD            :    // METHOD
        case NK_NESTEDTYPE        :    // NESTEDTYPE
        case NK_PARTIALMEMBER     :    // PARTIALMEMBER
        case NK_PROPERTY          :    // PROPERTY
        case NK_OPERATOR          :    // OPERATOR
            DUMPNEXT (((MEMBERNODE *)pNode)->pNext);
            break;
    }

    iIndent--;
}



