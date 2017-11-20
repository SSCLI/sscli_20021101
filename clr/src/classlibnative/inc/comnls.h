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
////////////////////////////////////////////////////////////////////////////
//
//  Module:   COMNLS
//  Purpose:  This module defines the common header information for
//            the Globalization classes.
//
//  Date:     August 12, 1998
//
////////////////////////////////////////////////////////////////////////////


#ifndef _COMNLS_H
#define _COMNLS_H


//
//  Constant Declarations.
//

#define LCID_ENGLISH_US 0x0409

#define ASSERT_API(Win32API)  \
    if ((Win32API) == 0)      \
        FATAL_EE_ERROR();

#define ASSERT_ARGS(pargs)    \
    ASSERT((pargs) != NULL);  \


////////////////////////////////////////////////////////////////////////////
//
//  internalGetField
//
////////////////////////////////////////////////////////////////////////////

template<class T>
inline T internalGetField(OBJECTREF pObjRef, char* szArrayName, HardCodedMetaSig* Sig)
{
    ASSERT((pObjRef != NULL) && (szArrayName != NULL) && (Sig != NULL));

    THROWSCOMPLUSEXCEPTION();

    FieldDesc* pFD = pObjRef->GetClass()->FindField(szArrayName, Sig);
    if (pFD == NULL)
    {
        ASSERT(FALSE);
        FATAL_EE_ERROR();
    }

    T dataArrayRef = (T)ArgSlotToObj((ARG_SLOT)pFD->GetValue32(pObjRef));
    return (dataArrayRef);
};


#endif
