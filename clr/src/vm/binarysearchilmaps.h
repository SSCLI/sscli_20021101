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
#ifndef _BINARYSEARCHILMAP_H
#define _BINARYSEARCHILMAP_H

#include "utilcode.h"
#include "..\debug\inc\dbgipcevents.h"

class CBinarySearchILMap : public CBinarySearch<UnorderedILMap>
{
public:
    CBinarySearchILMap(UnorderedILMap *pBase, int iCount) : 
        CBinarySearch<UnorderedILMap>(pBase, iCount)
    {
    }

    CBinarySearchILMap(void) : 
        CBinarySearch<UnorderedILMap>(NULL, 0)
    {
    }
        
    virtual int Compare( const UnorderedILMap *pFirst,
                         const UnorderedILMap *pSecond)
    {
        if (pFirst->mdMethod < pSecond->mdMethod)
            return -1;
        else if (pFirst->mdMethod == pSecond->mdMethod)
            return 0;
        else
            return 1;
    }


    void SetVariables(UnorderedILMap *pBase, int iCount)
    {
        m_pBase = pBase;
        m_iCount = iCount;
    }
} ;

#endif //_BINARYSEARCHILMAP_H
