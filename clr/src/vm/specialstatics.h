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
/*===========================================================================
**
** File:    SpecialStatics.h
**       
**
** Purpose: Defines the data structures for thread relative, context relative
**          statics.
**          
**
** Date:    Feb 28, 2000
**
=============================================================================*/
#ifndef _H_SPECIALSTATICS_
#define _H_SPECIALSTATICS_

class AppDomain;

// Data structure for storing special static data like thread relative or
// context relative static data.
typedef struct _STATIC_DATA
{
    WORD            cElem;
    LPVOID          dataPtr[0];
} STATIC_DATA;

typedef struct _STATIC_DATA_LIST
{
    struct _STATIC_DATA_LIST *m_pNext;
    AppDomain   *m_pDomain;
    STATIC_DATA *m_pUnsharedStaticData;
    STATIC_DATA *m_pSharedStaticData;
} STATIC_DATA_LIST;

#endif
