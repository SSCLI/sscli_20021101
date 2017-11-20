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
// File: nodes.h
//
// Definitions of all parse tree nodes
// ===========================================================================

#ifndef _NODES_H_
#define _NODES_H_

#include "constval.h"
#include "enum.h"
#include "strbuild.h"

////////////////////////////////////////////////////////////////////////////////
// Forward declarations of node structs

#define NODEKIND(n,s,g) struct s##NODE;
#include "nodekind.h"

////////////////////////////////////////////////////////////////////////////////
// NODEFLAGS
//
// NOTE:  NF_MOD_* values must be kept in sync with CLSDREC::accessTokens
//
// We have 16 bits of space for flags in BASENODE.  We have more than 16 flags.
// Thus, we compact them into groups such that no two flags that could be set on
// the same node have the same value.  This is easier thought of as groups of
// flags; no two flag GROUPS that could have representation on the same node
// can have overlap.

enum NODEFLAGS
{
    // MODIFIER flags (0x0000-0x0FFF)
    NF_MOD_ABSTRACT         = 0x0001,
    NF_MOD_NEW              = 0x0002,
    NF_MOD_OVERRIDE         = 0x0004,
    NF_MOD_PRIVATE          = 0x0008,
    NF_MOD_PROTECTED        = 0x0010,
    NF_MOD_INTERNAL         = 0x0020,
    NF_MOD_PUBLIC           = 0x0040,
    NF_MOD_ACCESSMODIFIERS  = NF_MOD_PUBLIC | NF_MOD_INTERNAL | NF_MOD_PROTECTED | NF_MOD_PRIVATE,
    NF_MOD_SEALED           = 0x0080,
    NF_MOD_STATIC           = 0x0100,
    NF_MOD_VIRTUAL          = 0x0200,
    NF_MOD_EXTERN           = 0x0400,
    NF_MOD_READONLY         = 0x0800,
    NF_MOD_VOLATILE         = 0x1000,   
    NF_MOD_UNSAFE           = 0x2000,  // ok to overlap with NF_PARMMOD_REF -- can't ever appear on an expression
    NF_MOD_MASK             = 0x3FFF,
    NF_MOD_LAST             = NF_MOD_UNSAFE,

    // EXPRESSION flags -- KEEP THESE UNIQUE, they are tested for without regard to node kind
    NF_PARMMOD_REF          = 0x2000,
    NF_PARMMOD_OUT          = 0x4000,
    NF_CALL_HADERROR        = 0x8000,

    // STATEMENT flags
    NF_UNCHECKED            = 0x0001,
    NF_GOTO_CASE            = 0x0002,
    NF_TRY_CATCH            = 0x0004,
    NF_TRY_FINALLY          = 0x0008,
    NF_CONST_DECL           = 0x0010,
    NF_FOR_FOREACH          = 0x0020,
    NF_FIXED_DECL           = 0x0040,
    NF_USING_DECL           = 0x0080,
    NF_STMT_HADERROR        = 0x0100,

    // VARDECL flags
    NF_VARDECL_EXPR         = 0x0001,
    NF_VARDECL_ARRAY        = 0x0002,

    // Numeric constant flags
    NF_CHECK_FOR_UNARY_MINUS= 0x0001,

    // Member ref flags
    NF_MEMBERREF_EMPTYARGS  = 0x0001,

    // Name flags
    NF_NAME_MISSING         = 0x0001,       // NK_NAME node where no identifier existed

    // 'new' flags
    NF_NEW_STACKALLOC       = 0x0001,       // NEWNODE contains a 'stackalloc' expression

    // MEMBER flags (EX -- stored in BASENODE::other)
    NFEX_CTOR_BASE          = 0x01,
    NFEX_CTOR_THIS          = 0x02,
    NFEX_METHOD_NOBODY      = 0x04,
    NFEX_INTERIOR_PARSED    = 0x08,         // This interior node has already been parsed for errors...
    NFEX_METHOD_VARARGS     = 0x10,
    NFEX_METHOD_PARAMS      = 0x20,
    NFEX_EVENT              = 0x40,         // field or property is actually an event.
};

////////////////////////////////////////////////////////////////////////////////
// TYPE_KIND

enum TYPE_KIND
{
    TK_PREDEFINED = 0,
    TK_ARRAY,
    TK_NAMED,
    TK_POINTER
};

////////////////////////////////////////////////////////////////////////////////
// NODEGROUP
//
// These are used to categorize nodes into groups that can easily be tested for
// by using BASENODE::InGroup(group), or given a node kind, checking
// BASENODE::m_rgNodeGroups[kind] & group.

enum NODEGROUP
{
    NG_TYPE                 = 0x00000001,       // Any non-namespace member of a namespace
    NG_AGGREGATE            = 0x00000002,       // Class, struct, enum, interface
    NG_METHOD               = 0x00000004,       // Method, ctor, dtor, operator
    NG_PROPERTY             = 0x00000008,       // Property, indexer
    NG_FIELD                = 0x00000010,       // Field, const
    NG_STATEMENT            = 0x00000020,       // Any statement node
    NG_BINOP                = 0x00000040,       // Any BINOP node
    NG_KEYED                = 0x00000080,       // Any node which can be 'keyed'
    NG_INTERIOR             = 0x00000100,       // Any node that has an interior, parsed on 2nd pass
    NG_MEMBER               = 0x00000200,       // Any member of a type (MEMBERNODE)
    NG_GLOBALCOMPLETION     = 0x00000400,       // Allow "blank-line" completion on this node
    NG_BREAKABLE            = 0x00000800,       // Statement that 'break' works for
    NG_CONTINUABLE          = 0x00001000,       // Statement that 'continue' works for
    NG_EMBEDDEDSTMTOWNER    = 0x00002000,       // Statement that 'owns' embedded stmts
};

////////////////////////////////////////////////////////////////////////////////
// PROTOFLAGS
//
// Flags to control output of BASENODE::BuildPrototype

enum PROTOFLAGS
{
    // Name control
    PTF_BASENAME            = 0x00000000,       // Base name only (default)
    PTF_FULLNAME            = 0x00000001,       // Fully-qualified name (namespace included)
    PTF_TYPENAME            = 0x00000002,       // Name of entity plus type (if member)

    // Parameter control
    PTF_PARAMETERS          = 0x00000004,       // Include parameters
};


////////////////////////////////////////////////////////////////////////////////
// Use these macros to define nodes.  Node kind names are derived from the name
// provided, as are the node structures themselves, i.e.,
//
//  DECLARE_NODE(FOO, EXPR)
//  ...
//  END_NODE()
//
// creates a FOONODE derived from EXPRNODE.

#define DECLARE_NODE(nodename, nodebase) struct nodename##NODE : public nodebase##NODE {
#define END_NODE() };

class NRHEAP;
class CInteriorNode;

////////////////////////////////////////////////////////////////////////////////
// BASENODE -- base class for all parse tree nodes

struct BASENODE
{
#ifdef DEBUG
    PCSTR       pszNodeKind;        // Debug only...
#endif

    unsigned    kind:8;         // Node kind
    unsigned    flags:16;       // Flags (NF_* bits)
    unsigned    other:8;        // Extra (usage depends on kind)

    long            tokidx;             // Token index (for access to position info)
    BASENODE        *pParent;           // All nodes have parent pointers

    // Accessor functions.
    OPERATOR    Op() { return (OPERATOR) other; }
    PREDEFTYPE  PredefType() { return (PREDEFTYPE) other; }
    TYPE_KIND   TypeKind() { ASSERT (kind == NK_TYPE); return (TYPE_KIND) other; }
    BASENODE    *GetParent();

    static  DWORD   m_rgNodeGroups[];   // Table of node group bits for each kind

    // Define all the concrete casting methods.
    #define NODEKIND(k1,k2,g) struct k2 ## NODE * as ## k1 ();
    #include "nodekind.h"

    // Little noop to enable looping:
    BASENODE        *asBASE() { return this; };

    // Some more complex casting helpers
    CLASSNODE       *asAGGREGATE ();
    METHODNODE      *asANYMETHOD ();
    PROPERTYNODE    *asANYPROPERTY ();
    FIELDNODE       *asANYFIELD ();
    MEMBERNODE      *asANYMEMBER ();

    // Handy functions
    BOOL    InGroup (DWORD dwGroups) { return (m_rgNodeGroups[kind] & dwGroups) != 0; }
    NAMENODE *LastNameOfDottedName();
    long    GetGlyph ();
    long    GetOperatorToken (long iOp);

    // Name and key construction methods (OLD, OBSOLETE)
    /*
    HRESULT AppendNameText (CComBSTR &sbstr);
    HRESULT AppendEscapedNameText (ICSNameTable *pNameTable, CComBSTR &sbstr);
    HRESULT AppendTypeText (ICSNameTable *pNameTable, CComBSTR &sbstr);                 // Escapes the names if pNameTable is not NULL
    void    AppendParametersToKey (BASENODE *pParms, CComBSTR &sbstr);
    void    AppendTypeToKey (TYPENODE *pType, CComBSTR &sbstr);
    void    AppendParametersToPrototype (BOOL fIndexer, BASENODE *pParms, CComBSTR &sbstr, BOOL fParamName);
    void    AppendTypeToPrototype (TYPENODE *pType, CComBSTR &sbstr);
    HRESULT BuildKey (ICSNameTable *pNameTable, BOOL fIncludeParent, NAME **ppKey);     // Builds a key for this node (if this is a keyable node)
    HRESULT BuildPrototype (ICSNameTable *pNameTable, DWORD dwFlags, CComBSTR &sbstr);  // Builds a prototype string for this node (must be keyable node)
    */

    // Name and key construction methods (NEW, BETTER, FASTER)
    HRESULT AppendNameText (CStringBuilder &sb, ICSNameTable *pNameTable);              // Names that are keywords are @-escaped unless pNameTable is NULL
    HRESULT AppendTypeText (CStringBuilder &sb, ICSNameTable *pNameTable);              // Ditto.
    HRESULT AppendPrototype (ICSNameTable *pNameTable, DWORD dwFlags, CStringBuilder &sb);
    void    AppendParametersToKey (BASENODE *pParms, CStringBuilder &sb);
    void    AppendTypeToKey (TYPENODE *pType, CStringBuilder &sb);
    void    AppendParametersToPrototype (BOOL fIndexer, BASENODE *pParms, CStringBuilder &sb, BOOL fParamName);
    void    AppendTypeToPrototype (TYPENODE *pType, CStringBuilder &sb);
    HRESULT BuildKey (ICSNameTable *pNameTable, BOOL fIncludeParent, NAME **ppKey);
};

typedef BASENODE * PBASENODE;

////////////////////////////////////////////////////////////////////////////////
// NAMESPACENODE

DECLARE_NODE (NAMESPACE, BASE)
    BASENODE        *pName;         // Name of namespace (possibly empty, or ""?)
    BASENODE        *pUsing;        // List of using clauses
    BASENODE        *pGlobalAttr;   // List of global attributes
    BASENODE        *pElements;     // Elements declared in this namespace
    long            iOpen;          // Open curly position
    long            iClose;         // Close curly position
    NAME            *pKey;          // Key
END_NODE()

////////////////////////////////////////////////////////////////////////////////
// USINGNODE

DECLARE_NODE (USING, BASE)
    BASENODE        *pName;         // Name used
    NAMENODE        *pAlias;        // Alias (NULL indicates using-namespace)
END_NODE()

////////////////////////////////////////////////////////////////////////////////
// NAMENODE

DECLARE_NODE (NAME, BASE)
    NAME            *pName;         // Name text
END_NODE()

////////////////////////////////////////////////////////////////////////////////
// CLASSNODE -- class/struct

DECLARE_NODE (CLASS, BASE)
    BASENODE        *pAttr;         // Attributes
    NAMENODE        *pName;         // Name of class
    BASENODE        *pBases;        // List of base class/interfaces
    MEMBERNODE      *pMembers;      // List of members
    long            iOpen;          // Open curly
    long            iClose;         // Close curly
    NAME            *pKey;          // Key
END_NODE()

////////////////////////////////////////////////////////////////////////////////
// DELEGATENODE

DECLARE_NODE (DELEGATE, BASE)
    BASENODE        *pAttr;         // Attributes
    TYPENODE        *pType;         // Return type
    NAMENODE        *pName;         // Name
    BASENODE        *pParms;        // Parameters
    long            iSemi;          // Semicolon position
    NAME            *pKey;          // Key
END_NODE()

////////////////////////////////////////////////////////////////////////////////
// MEMBERNODE -- base class for all members

DECLARE_NODE (MEMBER, BASE)
    BASENODE        *pAttr;         // Attributes
    MEMBERNODE      *pNext;         // Next member
    long            iClose;         // Semicolon or close-curly terminator
END_NODE()

////////////////////////////////////////////////////////////////////////////////
// ENUMMBRNODE -- member for enums

DECLARE_NODE (ENUMMBR, MEMBER)
    NAMENODE        *pName;
    BASENODE        *pValue;
END_NODE()

////////////////////////////////////////////////////////////////////////////////
// FIELDNODE

DECLARE_NODE (FIELD, MEMBER)
    TYPENODE        *pType;         // Type of field
    BASENODE        *pDecls;        // Declarations
END_NODE()

////////////////////////////////////////////////////////////////////////////////
// METHODNODE

DECLARE_NODE (METHOD, MEMBER)
    TYPENODE        *pType;         // Return type of method
    union
    {
        BASENODE        *pName;     // (NK_METHOD only -- Name of method)
        OPERATOR        iOp;        // (NK_OPERATOR only -- overloaded operator)
        BASENODE        *pCtorArgs; // (NK_CTOR only -- arg list to base (NF_CTOR_BASE) or this (NF_CTOR_THIS))
    };
    BASENODE        *pParms;        // Parameter list
    long            iOpen;          // Token index of open curly/semicolon
    BLOCKNODE       *pBody;         // Method body (if parsed)
    CInteriorNode   *pInteriorNode; // Interior node container object (if parsed)
END_NODE()

////////////////////////////////////////////////////////////////////////////////
// PROPERTYNODE

DECLARE_NODE (PROPERTY, MEMBER)
    BASENODE        *pName;         // Name of property, or name of interface for indexer
    TYPENODE        *pType;         // Type of property
    BASENODE        *pParms;        // Index parameters
    ACCESSORNODE    *pGet;          // Get accessor
    ACCESSORNODE    *pSet;          // Set accessor
END_NODE()

////////////////////////////////////////////////////////////////////////////////
// ACCESSORNODE

DECLARE_NODE (ACCESSOR, BASE)
    BASENODE        *pAttr;         // Attributes
    long            iOpen;          // Token index of open curly
    long            iClose;         // Token index of close curly
    BLOCKNODE       *pBody;         // Body of node
    CInteriorNode   *pInteriorNode; // Interior node container object (if parsed)
END_NODE()

////////////////////////////////////////////////////////////////////////////////
// PARAMETERNODE

DECLARE_NODE (PARAMETER, BASE)
    BASENODE        *pAttr;         // Attributes on parameter
    TYPENODE        *pType;         // Type of parameter
    NAMENODE        *pName;         // Name of parameter
END_NODE()

////////////////////////////////////////////////////////////////////////////////
// NESTEDTYPENODE

DECLARE_NODE (NESTEDTYPE, MEMBER)
    BASENODE        *pType;         // Nested type
END_NODE()

////////////////////////////////////////////////////////////////////////////////
// PARTIALMEMBERNODE

DECLARE_NODE (PARTIALMEMBER, MEMBER)
    BASENODE        *pNode;         // Whatever
END_NODE()


////////////////////////////////////////////////////////////////////////////////
// TYPENODE

DECLARE_NODE (TYPE, BASE)
    union
    {
        BYTE        iType;          // (flags & NF_TYPEMASK) == NF_TYPE_PREDEFINED
        BASENODE    *pName;         // (flags & NF_TYPEMASK) == NF_TYPE_NAMED
        TYPENODE    *pElementType;  // (flags & NF_TYPEMASK) == NF_TYPE_ARRAY or NF_TYPE_POINTER
    };
    int             iDims;          // Number of dimensions (-1 == unknown, i.e. [?])
END_NODE()


////////////////////////////////////////////////////////////////////////////////
// STATEMENTNODE

DECLARE_NODE (STATEMENT, BASE)
    STATEMENTNODE   *pNext;         // Next statement
END_NODE()

////////////////////////////////////////////////////////////////////////////////
// BLOCKNODE

DECLARE_NODE (BLOCK, STATEMENT)
    STATEMENTNODE   *pStatements;   // List of statements
    long            iClose;         // Close curly
END_NODE()

////////////////////////////////////////////////////////////////////////////////
// EXPRSTMTNODE -- a multi-purpose statement node (expression statements,
// goto, case, default, return, etc.)

DECLARE_NODE (EXPRSTMT, STATEMENT)
    BASENODE        *pArg;          // Argument
END_NODE()

////////////////////////////////////////////////////////////////////////////////
// LABELSTMTNODE

DECLARE_NODE (LABELSTMT, STATEMENT)
    BASENODE        *pLabel;        // Label name
    STATEMENTNODE   *pStmt;         // Statement
END_NODE()

////////////////////////////////////////////////////////////////////////////////
// LOOPSTMTNODE -- handles both do and while statements

DECLARE_NODE (LOOPSTMT, STATEMENT)
    BASENODE        *pExpr;         // Expression
    STATEMENTNODE   *pStmt;         // Statement
END_NODE()

////////////////////////////////////////////////////////////////////////////////
// FORSTMTNODE

DECLARE_NODE (FORSTMT, STATEMENT)
    BASENODE        *pInit;         // Init statement/expression(s)
    BASENODE        *pExpr;         // Condition expression
    BASENODE        *pInc;          // Increment expression(s)
    STATEMENTNODE   *pStmt;         // Iterated statement
END_NODE()

////////////////////////////////////////////////////////////////////////////////
// IFSTMTNODE

DECLARE_NODE (IFSTMT, STATEMENT)
    BASENODE        *pExpr;         // Condition
    STATEMENTNODE   *pStmt;         // TRUE statement
    STATEMENTNODE   *pElse;         // FALSE statement
END_NODE()

////////////////////////////////////////////////////////////////////////////////
// DECLSTMTNODE

DECLARE_NODE (DECLSTMT, STATEMENT)
    TYPENODE        *pType;         // Type of declaration
    BASENODE        *pVars;         // Declared variable list
END_NODE()

////////////////////////////////////////////////////////////////////////////////
// SWITCHSTMTNODE

DECLARE_NODE (SWITCHSTMT, STATEMENT)
    BASENODE        *pExpr;         // Switch expression
    BASENODE        *pCases;        // List of cases
    long            iOpen;          // Open curly
    long            iClose;         // Close curly
END_NODE()

////////////////////////////////////////////////////////////////////////////////
// CASENODE

DECLARE_NODE (CASE, BASE)
    BASENODE        *pLabels;       // List of case labels
    STATEMENTNODE   *pStmts;        // Statement list
END_NODE()

////////////////////////////////////////////////////////////////////////////////
// TRYSTMTNODE

DECLARE_NODE (TRYSTMT, STATEMENT)
    BLOCKNODE       *pBlock;        // Statement block (guarded)
    BASENODE        *pCatch;        // Catch blocks or finally block
END_NODE()

////////////////////////////////////////////////////////////////////////////////
// CATCHNODE

DECLARE_NODE (CATCH, BASE)
    BASENODE        *pType;         // Type caught
    NAMENODE        *pName;         // Name (optional)
    BLOCKNODE       *pBlock;        // Block
END_NODE()

////////////////////////////////////////////////////////////////////////////////
// UNOPNODE -- operator in flags

DECLARE_NODE (UNOP, BASE)
    BASENODE        *p1;            // Operand 1
END_NODE()

////////////////////////////////////////////////////////////////////////////////
// BINOPNODE -- operator in flags

DECLARE_NODE (BINOP, BASE)
    BASENODE        *p1;            // Operand 1
    BASENODE        *p2;            // Operand 2
END_NODE()

////////////////////////////////////////////////////////////////////////////////
// CALLNODE

DECLARE_NODE (CALL, BINOP)
    long            iClose;         // Token index of close paren
END_NODE()

////////////////////////////////////////////////////////////////////////////////
// CONSTVALNODE -- type in flags

DECLARE_NODE (CONSTVAL, BASE)
    CONSTVAL        val;            // Value of constant
END_NODE()

////////////////////////////////////////////////////////////////////////////////
// VARDECLNODE

DECLARE_NODE (VARDECL, BASE)
    NAMENODE        *pName;         // Name of variable
    BASENODE        *pArg;          // Init expression or array dim expression
    BASENODE        *pDecl;         // Pointer to parent decl node
END_NODE()

////////////////////////////////////////////////////////////////////////////////
// NEWNODE

DECLARE_NODE (NEW, BASE)
    TYPENODE        *pType;         // Type
    BASENODE        *pArgs;         // Constructor args or dim expression list
    BASENODE        *pInit;         // Array initializer
    long            iOpen;          // Token index of '('
    long            iClose;         // Token index of ')' or ']' for array
END_NODE ()

////////////////////////////////////////////////////////////////////////////////
// ATTRNODE

DECLARE_NODE (ATTR, BASE)
    BASENODE        *pName;         // Attribute name (possibly dotted, or possibly NULL if NK_ATTRARG)
    BASENODE        *pArgs;         // Arguments (NK_ATTR) or expression (NK_ATTRARG)
    long            iOpen;          // Token index of (
    long            iClose;         // Token index of )
END_NODE()

////////////////////////////////////////////////////////////////////////////////
// ATTRDECLNODE

DECLARE_NODE (ATTRDECL, BASE)
    NAMENODE        *pNameNode;     // User specified attr location. null if default.
    ATTRLOC         location;       // Attribute location allways valid.
    long            iClose;         // Token index of ']'
    BASENODE        *pAttr;         // Attribute list
END_NODE()

// Define all the concrete cast kinds here.
#define NODEKIND(k1,k2,g) \
    __forceinline k2 ## NODE * BASENODE::as ## k1 () {   \
        ASSERT(this == NULL || this->kind == NK_ ## k1);  \
        return static_cast<k2 ## NODE *>(this);     \
    }
#include "nodekind.h"

__forceinline CLASSNODE * BASENODE::asAGGREGATE ()
{
    ASSERT(this == NULL || InGroup (NG_AGGREGATE));
    return static_cast<CLASSNODE *> (this);
}

__forceinline METHODNODE * BASENODE::asANYMETHOD ()
{
    ASSERT(this == NULL || InGroup (NG_METHOD));
    return static_cast<METHODNODE *> (this);
}

__forceinline PROPERTYNODE * BASENODE::asANYPROPERTY ()
{
    ASSERT(this == NULL || InGroup (NG_PROPERTY));
    return static_cast<PROPERTYNODE *> (this);
}

__forceinline FIELDNODE * BASENODE::asANYFIELD ()
{
    ASSERT(this == NULL || InGroup (NG_FIELD));
    return static_cast<FIELDNODE *> (this);
}

__forceinline MEMBERNODE * BASENODE::asANYMEMBER ()
{
    ASSERT(this == NULL || InGroup (NG_MEMBER));
    return static_cast<MEMBERNODE *> (this);
}

__forceinline NAMENODE *BASENODE::LastNameOfDottedName()
{
    ASSERT(this->kind == NK_NAME || this->kind == NK_DOT);
    if (this->kind == NK_NAME)
        return this->asNAME();
    else
        return this->asDOT()->p2->asNAME();
}

__forceinline BASENODE *BASENODE::GetParent () 
{ 
    if (kind == NK_VARDECL) 
        return this->asVARDECL()->pDecl; 
    return this->pParent; 
}

////////////////////////////////////////////////////////////////////////////////
// CListIterator

// HACK:  This is copied from ..\rad\listiter.h.  Share it more correctly!

class CListIterator
{
private:
    BASENODE    *m_pCurNode;

public:
    void        Start (BASENODE *pNode) { m_pCurNode = pNode; }
    BASENODE    *Next ()
    {
        if (m_pCurNode == NULL)
            return NULL;
        if (m_pCurNode->kind == NK_LIST)
        {
            BASENODE    *pRet = m_pCurNode->asLIST()->p1;
            m_pCurNode = m_pCurNode->asLIST()->p2;
            return pRet;
        }
        else
        {
            BASENODE    *pRet = m_pCurNode;
            m_pCurNode = NULL;
            return pRet;
        }
    }
};


////////////////////////////////////////////////////////////////////////////////
// CDottedNameIterator

class CDottedNameIterator
{
private:
    BASENODE    *m_pRootNode;       // Root of dotted name tree
    NAMENODE    *m_pCurNode;        // Always the next name node to return

public:
    void        Start (BASENODE *pNode)
    {
        m_pRootNode = pNode;
        if (pNode != NULL)
        {
            while (pNode->kind == NK_DOT)
                pNode = pNode->asDOT()->p1;
        }
        m_pCurNode = pNode->asNAME();
    }

    NAMENODE    *Next ()
    {
        if (m_pCurNode == NULL)
            return NULL;

        NAMENODE    *pRet = m_pCurNode;

        if (pRet == m_pRootNode)
        {
            // This is the single-name given case
            m_pCurNode = NULL;
            return pRet;
        }

        BINOPNODE   *pParent = m_pCurNode->pParent->asDOT();        // Must be a dot; otherwise we couldn't get here

        if (pParent->p1 == m_pCurNode)
            m_pCurNode = pParent->p2->asNAME();                     // Switch from left-side to right-side
        else if (pParent == m_pRootNode)
            m_pCurNode = NULL;                                      // Last name in chain
        else
            m_pCurNode = pParent->pParent->asDOT()->p2->asNAME();   // Next name is right side of grandparent
        return pRet;
    }


};

#endif  // _NODES_H_
