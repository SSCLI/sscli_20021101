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

#include "stdafx.h"
#include "compiler.h"
#include "map.h"

int __cdecl CompareLongs(const void* first, const void* second) {
    return *(long*)first - *(long*)second;
}

void CLineMap::Clear() {
    if (m_array != NULL)
        m_allocator->Free(m_array);
    m_array = NULL;
    m_iCount = 0;
    m_iLast = -1;
}

/* AddMap - adds a mapping from oldLine to (newLine, pFilename)
 * Assuming there is sufficient memory, this always succeeds
 * if a map for the given line already exists, it will be replaced
 */
void CLineMap::AddMap(long oldLine, long newLine, NAME* pFilename) {
    int index = FindIndex(oldLine);
    if (index == -1) {
        m_iLast++;
        if (m_iLast >= m_iCount)
            GrowMap();
        index = m_iLast;
    }
    m_array[index].oldLine = oldLine;
    m_array[index].newLine = newLine;
    m_array[index].pFilename = pFilename;
    
    // ASSERT it if it's out of order
    // The tokenizer should ALWAYS add maps in order
    ASSERT (index == m_iLast && (index == 0 ||
           (index > 0 && m_array[index-1].oldLine < oldLine)));
}

/* GrowMap - enlarges our map by 10 */
void CLineMap::GrowMap() {
    if (m_array == NULL) {
        m_array = (PPLINE*)m_allocator->Alloc( (m_iCount = 10) * sizeof(PPLINE));
    } else
        m_array = (PPLINE*)m_allocator->Realloc(m_array, (m_iCount += 10) * sizeof(PPLINE));
}

/* FindIndex - returns an index into our map array
 * if the line has a mapping, returns the index
 * else returns -1
 */
long CLineMap::FindIndex(long oldLine) {
    if (m_array == NULL) return -1;
    ASSERT(offsetof(PPLINE, oldLine) == 0);

    void *index = bsearch( &oldLine, m_array, m_iLast + 1, sizeof(PPLINE), CompareLongs);
    if (index != NULL)
        return (long)((PPLINE*)index - m_array);
    else
        return -1;
}

/* FindClosestIndexBefore - returns an index into our map array
 * if the line has a mapping, returns the index
 * if not it returns the index of the previous line map
 * if no privous map exists, returns -1
 * (This uses FindClosestIndexBSearch and FindClosestIndexLinear to do the searching)
 */
long CLineMap::FindClosestIndexBefore(long oldLine) {
    if (m_array == NULL) return -1;
    ASSERT(offsetof(PPLINE, oldLine) == 0);
    
    if (m_iCount <= 12)
        return FindClosestIndexBeforeLinear( oldLine, 0, m_iLast);
    else
        return FindClosestIndexBeforeBSearch( oldLine, 0, m_iLast);
}

/* FindClosestIndexBeforeLinear - returns an index into our map array
 * internal only method, assumes that m_array is valid
 * NOTE: this never actually finds the index, it relies on FindClosestIndexLinear
 */
long CLineMap::FindClosestIndexBeforeLinear(long oldLine, int min, int max) {

    for(int l = max; l >= min; l--) {
        if (m_array[l].oldLine <= oldLine)
            return l;
    }
    return -1;
}


/* FindClosestIndexBeforeBSearch - returns an index into our map array
 * internal only method, assumes that m_array is valid
 * NOTE: this never actually finds the index, it relies on FindClosestIndexLinear
 */
long CLineMap::FindClosestIndexBeforeBSearch(long oldLine, int min, int max) {

    if (max - min <= 12)
        return FindClosestIndexBeforeLinear( oldLine, min, max);

    int mid = (max - min) / 2 + min;
    if (m_array[mid].oldLine > oldLine) {
        return FindClosestIndexBeforeBSearch(oldLine, min, mid);
    } else {
        ASSERT(m_array[mid].oldLine < oldLine);
        return FindClosestIndexBeforeBSearch(oldLine, mid, max);
    }
}

/* InternalMap - the real work horse, maps an oldLine to
 * a new line and filename
 * Faster if ppFilename is NULL because it doesn't find the mapped filename
 * also if there is no filename specified in a previous mapping it
 * does NOT change the value of ppFilename
 */
long CLineMap::InternalMap(long oldLine, NAME** /* out */ ppFilename) {

    long index = FindClosestIndexBefore(oldLine);
    if (index == -1)
        return oldLine;

    // Are we asking for the line that had the "#line"?
    if (oldLine == m_array[index].oldLine) {
        // backup to the previous one if it exists
        if (index > 0)
            index--;
        else
            // If it doesn't, then the line must be unmapped
            return oldLine;
    }


    // Calculate the new line number
    long newLine = (oldLine - (m_array[index].oldLine + 1)) + m_array[index].newLine;

    if (ppFilename != NULL) {
        // Now get the filename in effect for this section
        while (index >= 0 && m_array[index].pFilename == NULL)
            index--;
        if (index >= 0)
            *ppFilename = m_array[index].pFilename;
    }

    ASSERT(newLine >= 0);
    return newLine;
}
