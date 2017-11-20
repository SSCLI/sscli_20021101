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
** File:    StackBuilderSink.h
**       
**
** Purpose: Native implementaion for Microsoft.Runtime.StackBuilderSink
**
** Date:    Mar 24, 1999
**
===========================================================*/
#ifndef __STACKBUILDERSINK_H__
#define __STACKBUILDERSINK_H__


void CallDescrWithObjectArray(OBJECTREF& pServer, ReflectMethod *pMD, 
                  const BYTE *pTarget, MetaSig* sig, VASigCookie *pCookie,
                  BOOL fIsStatic, PTRARRAYREF& pArguments,
                  OBJECTREF* pVarRet, PTRARRAYREF* ppVarOutParams);

//+----------------------------------------------------------
//
//  Class:      CStackBuilderSink
// 
//  Synopsis:   EE counterpart to 
//              Microsoft.Runtime.StackBuilderSink
//              Code helper to build a stack of parameter 
//              arguments and make a function call on an 
//              object.
//
//------------------------------------------------------------
class CStackBuilderSink
{
public:    
   
    //struct PrivateProcessMessageArgs
    //{
    //    DECLARE_ECALL_OBJECTREF_ARG( OBJECTREF, pSBSink );
    //    DECLARE_ECALL_OBJECTREF_ARG( PTRARRAYREF*,  ppVarOutParams);
    //    DECLARE_ECALL_I4_ARG       (BOOL, fContext);
    //    DECLARE_ECALL_PTR_ARG      ( void*, iMethodPtr);
    //    DECLARE_ECALL_OBJECTREF_ARG( OBJECTREF, pServer);
    //    DECLARE_ECALL_OBJECTREF_ARG( PTRARRAYREF,  pArgs);
    //    DECLARE_ECALL_OBJECTREF_ARG( REFLECTBASEREF, pMethodBase);
    //};
    static FCDECL7(Object*, PrivateProcessMessage, Object* pSBSinkUNSAFE, ReflectBaseObject* pMethodBaseUNSAFE, PTRArray* pArgsUNSAFE, Object* pServerUNSAFE, void* iMethodPtr, BOOL fContext, PTRARRAYREF* ppVarOutParams);
};

#endif  // __STACKBUILDERSINK_H__
