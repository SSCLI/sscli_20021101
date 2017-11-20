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
// File: map.h
//
// Map for #line pragmas
// ===========================================================================

#ifndef __map_h__
#define __map_h__

struct NAME;

struct PPLINE {
    long    oldLine;
    long    newLine;
    NAME    *pFilename;
};

class CLineMap {
public:
    CLineMap(MEMHEAP *allocator) : m_allocator(allocator), m_array(NULL), m_iCount(0), m_iLast(-1) {};
    ~CLineMap() { Clear(); }
    void Clear();

    void AddMap(long oldLine, long newLine, NAME* pFilename);
    __inline bool IsEmpty() { return m_array == NULL && m_iCount == 0; }


    __inline long Map(long oldLine, NAME** /* out */ ppFilename) {
        if (m_array == NULL)
            return oldLine;
        else
            return InternalMap( oldLine, ppFilename);
    }

    __inline void Map(POSDATA * /* in,out */ pos, NAME** /* out */ ppFilename) {
        if (m_array != NULL)
            pos->iLine = InternalMap( pos->iLine, ppFilename);
    }


private:
    long InternalMap(long oldLine, NAME** /* out */ ppFilename);
    void GrowMap();
    long FindIndex(long oldLine);
    long FindClosestIndexBefore(long oldLine);
    long FindClosestIndexBeforeLinear(long oldLine, int min, int max);
    long FindClosestIndexBeforeBSearch(long oldLine, int min, int max);

    MEMHEAP *m_allocator;
    PPLINE  *m_array;
    long    m_iCount;
    long    m_iLast;
};

#endif //__map_h__

