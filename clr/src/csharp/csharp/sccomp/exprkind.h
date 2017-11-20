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
// File: exprkind.h
//
// Types of nodes in the bound expression tree
// ===========================================================================

#if !defined(EXPRDEF)
#error Must define EXPRDEF macro before including exprkind.h
#endif

EXPRDEF(BLOCK)
EXPRDEF(BINOP)
EXPRDEF(STMTAS)
EXPRDEF(CALL)
EXPRDEF(EVENT)
EXPRDEF(FIELD)
EXPRDEF(LOCAL)
EXPRDEF(RETURN)
EXPRDEF(DECL)
EXPRDEF(CONSTANT)
EXPRDEF(CLASS)
EXPRDEF(NSPACE)
EXPRDEF(ERROR)
EXPRDEF(LABEL)
EXPRDEF(GOTO)
EXPRDEF(GOTOIF)
EXPRDEF(FUNCPTR)
EXPRDEF(SWITCH)
EXPRDEF(SWITCHLABEL)
EXPRDEF(PROP)
EXPRDEF(TRY)
EXPRDEF(HANDLER)
EXPRDEF(THROW)
EXPRDEF(MULTI)
EXPRDEF(WRAP)
EXPRDEF(CONCAT)
EXPRDEF(ARRINIT)
EXPRDEF(ASSERT)
EXPRDEF(CAST)
EXPRDEF(TYPEOF)
EXPRDEF(SIZEOF)
EXPRDEF(ZEROINIT)
EXPRDEF(NOOP)
EXPRDEF(NAMEDATTRARG)
EXPRDEF(USERLOGOP)
