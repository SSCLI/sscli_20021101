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
/*============================================================
**
** Header:  COMStringCommon
**       
**
** Purpose: Macros and function headers common to all types of
** strings.
**
** Date:  March 30, 1998
** 
===========================================================*/
#ifndef  _COMSTRINGCOMMON_H
#define _COMSTRINGCOMMON_H
//
// The following constants will need to be visible from managed code at some point.
// How do we go about making constants visible outside of our class?
//
#define TRIM_START 0
#define TRIM_END 1
#define TRIM_BOTH 2
#define CASE_INSENSITIVE_BIT 1
#define LOCALE_INSENSITIVE_BIT 2
//"Locale"
#define CASE_AND_LOCALE 0
//"IgnoreCase"
#define CASE_INSENSITIVE 1
//"Exact"
#define LOCALE_INSENSITIVE 2
//No longer used.
#define CASE_AND_LOCALE_INSENSTIVE 3


INT32 RefInterpretGetLength(OBJECTREF);
//WCHAR *RefInterpretGetCharPointer(OBJECTREF);
//BOOL RefInterpretGetValues(OBJECTREF, WCHAR **, int *);

/*====================================RETURN====================================
**This macro is designed to handle the funky returns from ecall functions.
**value is the type to be returned, and is expected to be OBJECTREF, STRINGREF,
**etc.  Type is the type of the return (e.g. STRINGREF). The name for the return
**value (r_-v) is deliberately obtuse so that it is less likely that developers
**have used a value by the same name in their code.
==============================================================================*/
#define RETURN(value, type) \
   {LPVOID r_v_; \
   *((type *)(&r_v_))=value; \
                                 return r_v_;}

#endif // _COMSTRINGCOMMON_H

