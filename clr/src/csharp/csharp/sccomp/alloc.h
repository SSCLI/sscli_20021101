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
// File: alloc.h
//
// ===========================================================================

#ifndef __alloc_h__
#define __alloc_h__

/*
 * Round an allocation size up to a multiple of 4 (DWORD alignment).
 * (or 8 which is QWORD alignment for Itanium)
 */
inline size_t RoundSize(size_t sz)
{
#ifdef ALIGN_ACCESS
    return (sz + 7) & ~0x7;
#else
    return (sz + 3) & ~0x3;
#endif
}

class MEMHEAP;
class PAGEHEAP;

////////////////////////////////////////////////////////////////////////////////
// ALLOCHOST
//
// Each allocator must be created with a pointer to a "host".  A host is a sink
// for "out of memory events" (which may or MAY NOT throw an exception), and is
// also an access point for getting to other allocators which this allocator
// should use as appropriate.

class ALLOCHOST
{
public:
    virtual void        NoMemory () = 0;
    virtual MEMHEAP     *GetStandardHeap () = 0;
    virtual PAGEHEAP    *GetPageHeap () = 0;
};

////////////////////////////////////////////////////////////////////////////////
// Alloc, AllocZero are MACROS!
// In DEBUG, these add the file and line to the allocation for leak detection.

#ifdef DEBUG
#define Alloc(s) _Alloc (s, __FILE__, __LINE__)
#define AllocZero(s) _AllocZero (s, __FILE__, __LINE__)
#else
#define Alloc(s) _Alloc (s)
#define AllocZero(s) _AllocZero (s)
#endif

/* Standard memory heap; supports alloc/free/realloc.
 * Out-of-memory failures return NULL, but not before calling ALLOCHOST::NoMemory.
 * Not synchronized.
 */
class MEMHEAP {
public:
    MEMHEAP(ALLOCHOST * host, bool bTrackMem = false);
    ~MEMHEAP();

    //void * operator new(size_t size) { return VSAlloc (size); }
    //void operator delete(void * p) { VSFree (p); }

#ifdef DEBUG
    void * _Alloc(size_t sz, LPCSTR pszFile, UINT iLine);
    void * _AllocZero(size_t sz, LPCSTR pszFile, UINT iLine);
#else
    void * _Alloc(size_t sz);
    void * _AllocZero(size_t sz);
#endif
    LPWSTR AllocStr(LPCWSTR str);
    void * Realloc(void * p, size_t sz);
    void * ReallocZero(void * p, size_t sz);
    void Free(void * p);
    void FreeHeap(bool checkLeaks = true);

#ifdef TRACKMEM    // Memory tracking
    unsigned GetCurrentSize() { return (unsigned)m_curSize; }
    unsigned GetMaxSize() { return (unsigned)m_maxSize; }
    void SetMemTrack( bool bTrackMem) { if (!(m_bTrackMem = bTrackMem)) m_curSize = m_maxSize = 0; }
#else
    __forceinline unsigned GetCurrentSize() { return 0; }
    __forceinline unsigned GetMaxSize() { return 0; }
    __forceinline void SetMemTrack( bool bTrackMem) { m_bTrackMem = bTrackMem; }
#endif

private:
    HANDLE heap;
    ALLOCHOST * host;
    bool m_bTrackMem;
#ifdef TRACKMEM
    size_t m_curSize, m_maxSize;
#endif
};


/* Page allocation heap. Allocates and frees pages
 * or groups of pages. Allocation must be a multiple
 * of the system page size. Memory is not zeroed.
 */

#define PAGES_PER_ARENA 1024     // 4M or 8M on typical systems.
#define BIGALLOC_SIZE   (256 * 1024) // more than this alloc is not done from an arena.

#define DWORD_BIT_SHIFT 5        // log2 of bits in a DWORD.
#define BITS_DWORD      (1 << DWORD_BIT_SHIFT)
#define DWORD_BIT_MASK  (BITS_DWORD - 1)


class PAGEHEAP {
    // Store information about a memory arena we allocate pages from.
    struct PAGEARENA {
        PAGEARENA * nextArena;  // the arena.
        void * pages;           // the pages in the arena.
        size_t size;            // size of the arena.
        bool largeAlloc;        // if true, a "large allocation", and the bitmaps aren't used.
        DWORD used[PAGES_PER_ARENA / BITS_DWORD];        // bit map of in-use pages in this arena.
        DWORD committed[PAGES_PER_ARENA / BITS_DWORD];   // bit map of committed pages in this arena.
    };

public:
    PAGEHEAP(ALLOCHOST * host, bool bTrackMem = false);
    ~PAGEHEAP();

    //void * operator new(size_t size) { return VSAlloc (size); }
    //void operator delete(void * p) { VSFree (p); }

    void * AllocPages(size_t sz);
    void FreePages(void * p, size_t sz);
    void FreeAllPages(bool checkLeaks = true);

    static size_t pageSize;         // The system page size.
    static int pageShift;           // log2 of the page size

#ifdef TRACKMEM    // Memory tracking
    unsigned GetCurrentUseSize() { return (unsigned)(m_pageCurUse * pageSize); }
    unsigned GetMaxUseSize() { return (unsigned)(m_pageMaxUse * pageSize); }
    unsigned GetCurrentReserveSize() { return (unsigned)(m_pageCurReserve * pageSize); }
    unsigned GetMaxReserveSize() { return (unsigned)(m_pageMaxReserve * pageSize); }
    void SetMemTrack( bool bTrackMem) 
        { if (!(m_bTrackMem = bTrackMem)) m_pageCurUse = m_pageMaxUse = m_pageCurReserve = m_pageMaxReserve = 0; }
#else
    __forceinline unsigned GetCurrentUseSize() { return 0; }
    __forceinline unsigned GetMaxUseSize() { return 0; }
    __forceinline unsigned GetCurrentReserveSize() { return 0; }
    __forceinline unsigned GetMaxReserveSize() { return 0; }
    __forceinline void SetMemTrack( bool bTrackMem) { m_bTrackMem = bTrackMem; }
#endif

private:
    PAGEARENA * CreateArena(bool isLarge, size_t sz);
    PAGEARENA * FindArena(void * p);

    void * LargeAlloc(size_t sz);
    void LargeFree(void * p, size_t sz);

    PAGEARENA * arenaList;          // List of memory arenas.
    PAGEARENA * arenaLast;          // Last memory arena in list.
    ALLOCHOST * host;               // Alloc host

    bool m_bTrackMem;

#ifdef TRACKMEM
    size_t m_pageCurUse, m_pageMaxUse;
    size_t m_pageCurReserve, m_pageMaxReserve;
#endif
};


/* No-release allocator. Allocates very fast, cannot
 * be released in general, except in a LIFO fashion
 * to a particular mark.
 */

// Holds a page of memory being used by the NRHEAP.
struct NRPAGE {
    NRPAGE * next;      // next page in use.
    BYTE * firstAvail;  // first available byte for allocation.
    BYTE * limitAvail;  // limit for allocation.
};



class NRMARK {
private:
    friend class NRHEAP;

    NRPAGE * page;      // page.
    BYTE * nextFree;    // first free location within the page.
};

class NRHEAP {
public:
    NRHEAP(ALLOCHOST * pHost, bool bTrackMem = false);
    ~NRHEAP();

    //void * operator new(size_t size) { return VSAlloc (size); }
    //void operator delete(void * p) { VSFree (p); }

#ifdef DEBUG
    void * _Alloc(size_t sz, LPCSTR pszFile, UINT iLine);
    void * _AllocZero(size_t sz, LPCSTR pszFile, UINT iLine);
#else
    void * _Alloc(size_t sz);
    void * _AllocZero(size_t sz);
#endif
    LPWSTR AllocStr(LPCWSTR str);
    void Mark(NRMARK * mark);
    void Free(NRMARK * mark);
    void FreeHeap();
    size_t CalcCommittedSize ();

private:
    BYTE * nextFree;                // location of free area
    BYTE * limitFree;               // just beyond end of free area in this page.

    NRPAGE * pageList;              // list of pages used by this allocator.
    NRPAGE * pageLast;              // last page in the list.

    ALLOCHOST   * host;             // Host

    void * AllocMore(size_t sz);
    NRPAGE * NewPage(size_t sz);

    bool m_bTrackMem;
};

/*
 * Allocate a new block of memory. This should be VERY FAST
 * and VERY SMALL. The complex case where a new page must
 * be allocated is put into an out of line method.
 */
__forceinline void * NRHEAP::_Alloc(size_t sz
#ifdef DEBUG
, LPCSTR pszFile, UINT iLine        // NOTE:  These aren't used here, since NRHEAP's aren't leak-checked (obviously)...
#endif
)
{
    void * p;

    p = nextFree;
    if ((nextFree += RoundSize(sz)) > limitFree) // 4-byte alignment
        return AllocMore(sz);
    else {
#ifdef DEBUG
        // fill in with non-zero values so user doesn't expect zeroed memory.
        memset(p, 0xCC, sz);
#endif //DEBUG
        return p;
    }
}
#endif // __alloc_h__

