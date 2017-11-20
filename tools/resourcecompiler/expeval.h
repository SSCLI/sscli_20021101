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
// File: expeval.h
// Purpose: Header file for Expression Evaluator
// ===========================================================================

#ifndef __EXPEVAL_H__
#define __EXPEVAL_H__

typedef enum _TokenType {
  TK_MOD,           //1
  TK_PLUS,          //2
  TK_MINUS,         //3
  TK_STAR,          //4
  TK_DIVIDE,        //5
  TK_NUMBER,        //6
  TK_EOS            //7

} TOKENTYPE, *PTOKENTYPE;

typedef struct _Token {
  TOKENTYPE Type;
  char* Name;
  ULONG Value;
} TOKEN, *PTOKEN;


ULONG
Expr_Eval(
    void
    );

ULONG
Expr_Eval_1(
    void
    );

ULONG
Expr_Eval_2(void);

#endif	/* __EXPEVAL_H__ */
