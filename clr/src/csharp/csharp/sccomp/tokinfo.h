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
// File: tokinfo.h
//
// ===========================================================================

#ifndef __tokinfo_h__
#define __tokinfo_h__

#include "enum.h"
#include "posdata.h"
#include "tokdata.h"

class CParser;
struct STATEMENTNODE;
enum STMTPARSERKIND {
  ENoStatement,
  EParseBreakStatement,
  EParseTryStatement,
  EParseCheckedStatement,
  EParseConstStatement,
  EParseDoStatement,
  EParseFixedStatement,
  EParseForStatement,
  EParseForEachStatement,
  EParseGotoStatement,
  EParseIfStatement,
  EParseLockStatement,
  EParseReturnStatement,
  EParseSwitchStatement,
  EParseThrowStatement,
  EParseUsingStatement,
  EParseWhileStatement,
  EParseBlock,
};

////////////////////////////////////////////////////////////////////////////////
// TOKINFO

#include <pshpack1.h>
struct TOKINFO
{
    PCWSTR      pszText;            // token text
    DWORD       dwStmtParserKind;   // Statement parser function kind
    // Keep the pointers first for better 64-bit porting
    DWORD       dwFlags;            // token flags
    BYTE        iLen;               // token length
    BYTE        iPredefType;        // predefined type represented by token
    BYTE        iBinaryOp;          // Binary operator
    BYTE        iUnaryOp;           // Unary operator
    BYTE        iSelfOp;            // Self operator (like true, false, this...)
};
#include <poppack.h>

////////////////////////////////////////////////////////////////////////////////
// OPINFO

struct OPINFO
{
    BYTE        iToken:8;           // Token ID
    BYTE        iPrecedence:7;      // Operator precedence
    BYTE        fRightAssoc:1;      // Associativity
};

struct NAME;
class COMPILER;

////////////////////////////////////////////////////////////////////////////////
// FULLTOKEN
//
// This is the "full" version of a tokendata struct.  It is the one that has
// the ctor/dtor that manages the pszAllocToken (in case of escaped tokens)

struct FULLTOKEN : public TOKENDATA
{
    PCWSTR      pszToken;               // Pointer to token text (inside pszAllocToken if escaped), NOT NULL TERMINATED!
    PWSTR       pszAllocToken;          // Allocated space to hold token iff it contained unicode escapes
    bool        partialOk;

    FULLTOKEN () : pszAllocToken(NULL), partialOk(false) {}
    ~FULLTOKEN () { if (pszAllocToken) VSFree (pszAllocToken); }
};

#endif //__tokinfo_h__

