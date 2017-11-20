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
// File: alloc.cpp
//
// Handles the memory allocation for the compiler
//
// There are three memory allocators:
//
// MEMHEAP - a pretty standard Alloc/Free/Realloc heap. Has optional
//           leak checking.
// PAGEHEAP - allocates pages from the operation system.
// NRHEAP - "no-release" heap; incrementally allocates from pages
//          allocated from the PAGEHEAP.
// ===========================================================================

#include "stdafx.h"
#include "alloc.h"

/*
 * We have special allocation rules in the compiler, where everything
 * should be allocated out of a heap associated with the compiler
 * instance we are in. Prevent people from using new/delete or
 * malloc/free.
 *       
 *
 * This is a great idea, but in fact the above rules are not 100% true.
 * There are some allocations which "outlive" compiler instances (the
 * compiler has been made multi-instance since the rules were made)
 * such as allocations from the name table, and the name table itself
 * (among other COM objects whose references are provided to parties
 * outside the compiler, such as source modules, source data objects,
 * etc.)
 *
 * These allocations are now made from the module's default heap, using
 * VSMEM's allocation tracking mechanism for standard leak detection,
 * etc.

PVOID __cdecl operator new(size_t size)
      {
        VSFAIL("Never use the global operator new in the compiler");
        return 0;
      }
PVOID __cdecl operator new(size_t size, LPCSTR pszFile, UINT uLine)
      {
        VSFAIL("Never use the global operator new in the compiler");
        return 0;
      }
void  __cdecl operator delete(PVOID pv)
      {
        VSFAIL("Never use the global operator delete in the compiler");
      }
 */


/*
 ************************** MEMHEAP methods ******************************
 */

/* The "MEMHEAP" is a simple heap like the Win32 heaps, but with leak
 * checking and fatal error on out of memory. Just a simple wrapper
 * on top of the VS heap routines.
 */

/* Create a new heap for allocation.
 */

MEMHEAP::MEMHEAP(ALLOCHOST *pHost, bool bTrackMem)
{
    ASSERT(pHost != 0);

    host = pHost;
    m_bTrackMem = bTrackMem;
#ifdef TRACKMEM
    m_curSize = m_maxSize = 0;
#endif

	heap = GetProcessHeap();

}

/* The heap is being destroyed. Free everything if
 * not done already.
 */
MEMHEAP::~MEMHEAP()
{
    FreeHeap();
}

/*
 * Free everything in the heap. The heap can no longer be used.
 */
void MEMHEAP::FreeHeap(bool checkLeaks)
{

#ifdef TRACKMEM
    if (m_bTrackMem) {
        m_curSize = 0;
    }
#endif
}

/*
 * Allocate from the heap. Memory will NOT be zeroed.
 */
void * MEMHEAP::_Alloc(size_t sz
#ifdef DEBUG
, LPCSTR pszFile, UINT iLine
#endif
)
{
    ASSERT(heap != 0);

#ifdef DEBUG
    // Use the actual internal alloc function so we can pass in the file/line values
    void * p = VsDebugAllocInternal(heap, 0, sz, pszFile, iLine, INSTANCE_GLOBAL, NULL);
#else
    void * p = VSHeapAlloc(heap, sz);
#endif
    if (p == 0)
        host->NoMemory();

#ifdef TRACKMEM
    else if (m_bTrackMem) {
        m_curSize += sz;
        if (m_curSize > m_maxSize)
            m_maxSize = m_curSize;
    }
#endif

    return p;
}

/*
 * Allocate from the heap. Memory WILL be zeroed.
 */
void * MEMHEAP::_AllocZero(size_t sz
#ifdef DEBUG
, LPCSTR pszFile, UINT iLine
#endif
)
{
    ASSERT(heap != 0);

#ifdef DEBUG
    // Use the actual internal alloc function so we can pass in the file/line values
    void * p = VsDebugAllocInternal(heap, HEAP_ZERO_MEMORY, sz, pszFile, iLine, INSTANCE_GLOBAL, NULL);
#else
    void * p = VSHeapAllocZero(heap, sz);
#endif
    if (p == 0)
        host->NoMemory();

#ifdef TRACKMEM
    else if (m_bTrackMem) {
        m_curSize += sz;
        if (m_curSize > m_maxSize)
            m_maxSize = m_curSize;
    }
#endif

    return p;
}

/*
 * Allocate (duplicate) a wide char string.
 */
LPWSTR MEMHEAP::AllocStr(LPCWSTR str)
{
    if (str == NULL)
        return NULL;

    LPWSTR strNew = (LPWSTR) Alloc((wcslen(str) + 1) * sizeof(WCHAR));
    wcscpy(strNew, str);
    return strNew;
}

/*
 * Realloc a block on the heap. New memory won't be zeroed.
 */
void * MEMHEAP::Realloc(void * p, size_t sz)
{
    ASSERT(heap != 0);

#ifdef TRACKMEM
    size_t ch = (m_bTrackMem ? ch = sz - VSHeapSize(heap, p) : 0);
#endif

    void * newp = VSHeapRealloc(heap, p, sz);
    if (newp == 0)
        host->NoMemory();

#ifdef TRACKMEM
    else if (m_bTrackMem) {
        m_curSize += ch;
        ASSERT (m_curSize >= 0);
        if (m_curSize > m_maxSize)
            m_maxSize = m_curSize;
    }
#endif

    return newp;
}

/*
 * Realloc a block on the heap. New memory WILL be zeroed.
 */
void * MEMHEAP::ReallocZero(void * p, size_t sz)
{
    ASSERT(heap != 0);

#ifdef TRACKMEM
    size_t ch = (m_bTrackMem ? ch = sz - VSHeapSize(heap, p) : 0);
#endif

    void * newp = VSHeapReallocZero(heap, p, sz);
    if (newp == 0)
        host->NoMemory();

#ifdef TRACKMEM
    else if (m_bTrackMem) {
        m_curSize += ch;
        ASSERT (m_curSize >= 0);
        if (m_curSize > m_maxSize)
            m_maxSize = m_curSize;
    }
#endif

    return newp;
}

/*
 * Free memory from the heap
 */
void MEMHEAP::Free(void * p)
{
    ASSERT(heap != 0);

#ifdef TRACKMEM
    if (m_bTrackMem) {
        m_curSize -= VSHeapSize(heap, p);
    }
#endif

    VSHeapFree(heap, p);
}


/*
 ************************** PAGEHEAP methods ******************************
 */

/* The biggest trickiness with the page heap is that it is inefficient
 * to allocate single pages from the operating system - NT will allocate
 * only on 64K boundaries, so allocating a 4k page is needlessly inefficient.
 * We use the ability to reserve then commit pages to reserve moderately
 * large chunks of memory (a PAGEARENA), then commit pages in the arena.
 * This also allows us to track pages allocated aned detect leaks.
 */

/*
 * static data members
 */
size_t PAGEHEAP::pageSize;         // The system page size.
int PAGEHEAP::pageShift;           // log2 of the page size

/*
 * Constructor.
 */
PAGEHEAP::PAGEHEAP (ALLOCHOST *pHost, bool bTrackMem)
{
    m_bTrackMem = bTrackMem;
#ifdef TRACKMEM
    m_pageCurUse = m_pageMaxUse = m_pageCurReserve = m_pageMaxReserve = 0;
#endif

    // First time through -- get system page size.
    if (!PAGEHEAP::pageSize) {
        // Get the system page size.
        SYSTEM_INFO sysinfo;
        GetSystemInfo(&sysinfo);
        PAGEHEAP::pageSize = sysinfo.dwPageSize;

        // Determine the page shift.
        int shift = 0;
        size_t size = PAGEHEAP::pageSize;
        while (size != 1) {
            shift += 1;
            size >>= 1;
        }
        PAGEHEAP::pageShift = shift;

        ASSERT((size_t)(1 << PAGEHEAP::pageShift) == PAGEHEAP::pageSize);
    }

    arenaList = arenaLast = NULL;

    // Remember the host
    host = pHost;
}

/*
 * Destructor. Free everything.
 */
PAGEHEAP::~PAGEHEAP()
{
    FreeAllPages();
}

/*
 * Allocate a set of pages from the page heap. Memory is not
 * zero-filled.
 */
void * PAGEHEAP::AllocPages(size_t sz)
{
    ASSERT(sz % pageSize == 0 && sz != 0);     // must be page size multiple.

#ifdef TRACKMEM
    if (m_bTrackMem) {
        m_pageCurUse += sz / pageSize;
        if (m_pageCurUse > m_pageMaxUse)
            m_pageMaxUse = m_pageCurUse;
    }
#endif

    // Handle very large allocations differently.
    if (sz > BIGALLOC_SIZE) {
        return LargeAlloc(sz);
    }

    // How many pages are being allocated?
    size_t cPages = (sz >> pageShift);
    void * p;
    PAGEARENA * arena;

    // Check each arena in turn for enough contiguous pages.
    for (arena = arenaList; arena != NULL; arena = arena->nextArena)
    {
        if (arena->largeAlloc)
            continue;           // Large allocation arenas are not interesting.

        // Search this area for free pages. First, find a dword that isn't all used.
        // This loop quickly skips (32 at a time) the used pages that will tend to
        // be at the beginning of an arena.
        int dwIndex;
        for (dwIndex = 0; dwIndex < PAGES_PER_ARENA / BITS_DWORD; ++dwIndex) {
            if (arena->used[dwIndex] != 0xFFFFFFFF)
                break;      // not all used.
        }
        if (dwIndex >= PAGES_PER_ARENA / BITS_DWORD)
            continue;       // No free pages in this arena, go to next one.

        // Now scan for pages starting in this dword.
        size_t iPage = dwIndex * BITS_DWORD;

        while (iPage < PAGES_PER_ARENA) {
            size_t c;
            bool allCommitted;

            // Scan to see if cPages pages starting at iPage are not in use.
            // While scanning, remember if they are all committed or not.
            c = cPages;
            allCommitted = true;
            while (c) {
                if (arena->used[iPage >> DWORD_BIT_SHIFT] & (1 << (iPage & DWORD_BIT_MASK)))
                    break;
                if (! (arena->committed[iPage >> DWORD_BIT_SHIFT] & (1 << (iPage & DWORD_BIT_MASK))))
                    allCommitted = false;

                ++iPage;
                --c;
            }

            if (c) {
                // Enough free pages not found. Start looking again at next page.
                ++iPage;
                continue;
            }

            // Success, we contiguous found free pages.

            iPage -= cPages;    // Back up to first page again.
            p = (BYTE *)arena->pages + (iPage << pageShift);    // Calculate address of allocation.

            //  Commit the pages from the OS if needed.
            if (!allCommitted) {
                if (VirtualAlloc(p, sz, MEM_COMMIT, PAGE_READWRITE) != p) {
                    host->NoMemory ();
                    return NULL;
                }
            }

            // Mark them as in use and committed.
            c = cPages;
            while (c--) {
                ASSERT(! (arena->used[iPage >> DWORD_BIT_SHIFT] & (1 << (iPage & DWORD_BIT_MASK))));

                arena->used     [iPage >> DWORD_BIT_SHIFT] |= (1 << (iPage & DWORD_BIT_MASK));
                arena->committed[iPage >> DWORD_BIT_SHIFT] |= (1 << (iPage & DWORD_BIT_MASK));

                ++iPage;
            }

#ifdef DEBUG
            // Make sure they aren't zero filled.
            memset(p, 0xCC, sz);
#endif //DEBUG

            // Return the address of the allocated pages.
            return p;
        }
    }

    // No arenas have enough free space. Create a new arena and allocate
    // at the beginning of that arena.
    arena = CreateArena(false, PAGES_PER_ARENA * pageSize);
    if (arena == NULL)
        return NULL;

    p = arena->pages;

#ifdef TRACKMEM
    if (m_bTrackMem) {
        m_pageCurReserve += PAGES_PER_ARENA;
        if (m_pageCurReserve > m_pageMaxReserve)
            m_pageMaxReserve = m_pageCurReserve;
    }
#endif

    // Commit the memory we need.
    if (VirtualAlloc(p, sz, MEM_COMMIT, PAGE_READWRITE) != p) {
        host->NoMemory ();
        return NULL;
    }

    // Mark the pages as in use and committed.
    size_t c = cPages;
    for (size_t iPage = 0; c > 0; --c, ++iPage) {
        ASSERT(! (arena->used[iPage >> DWORD_BIT_SHIFT] & (1 << (iPage & DWORD_BIT_MASK))));

        arena->used     [iPage >> DWORD_BIT_SHIFT] |= (1 << (iPage & DWORD_BIT_MASK));
        arena->committed[iPage >> DWORD_BIT_SHIFT] |= (1 << (iPage & DWORD_BIT_MASK));
    }

#ifdef DEBUG
    // Make sure they aren't zero filled.
    memset(p, 0xCC, sz);
#endif //DEBUG

    return p;
}

/*
 * Free pages page to the page heap. The size must be the
 * same as when allocated.
 */
void PAGEHEAP::FreePages(void * p, size_t sz)
{
    ASSERT(sz % pageSize == 0 && sz != 0);     // must be page size multiple.

#ifdef TRACKMEM
    if (m_bTrackMem)
        m_pageCurUse -= sz / pageSize;
#endif
    
    // Handle very large allocations differently.
    if (sz > BIGALLOC_SIZE) {
        LargeFree(p, sz);
        return;
    }

    // Find the arena this page is in.
    PAGEARENA * arena = FindArena(p);
    ASSERT(arena);

    // Get page index within this arena, and page count.
    size_t iPage = ((BYTE *)p - (BYTE *)arena->pages) >> pageShift;
    size_t cPages = sz >> pageShift;

    // Pages must be in-use and committed. Set the pages to not-in-use. We could
    // decommit the pages here, but it seems more efficient to keep them around
    // committed because we'll probably want them again. We might want to support
    // a "decommit to system" call that would decommit the pages to the system
    // so that other processes or compilations could use them.
    while (cPages--) {
        ASSERT(arena->used[iPage >> DWORD_BIT_SHIFT]      & (1 << (iPage & DWORD_BIT_MASK)));
        ASSERT(arena->committed[iPage >> DWORD_BIT_SHIFT] & (1 << (iPage & DWORD_BIT_MASK)));

        arena->used[iPage >> DWORD_BIT_SHIFT] &= ~(1 << (iPage & DWORD_BIT_MASK));

        ++iPage;
    }

#ifdef DEBUG
    // Fill pages with junk to indicated unused.
    memset(p, 0xEE, sz);
#endif //DEBUG
}

/*
 * Allocate a very large allocation. An entire arena is allocated for the allocation.
 */
void * PAGEHEAP::LargeAlloc(size_t sz)
{
    // Create an arena for this large allocation.
    PAGEARENA * newArena = CreateArena(true, sz);
    if (newArena == NULL)
        return NULL;

#ifdef TRACKMEM
    // Make sure they aren't zero filled.
    memset(newArena->pages, 0xCC, sz);

    if (m_bTrackMem) {
        m_pageCurReserve += sz / pageSize;
        if (m_pageCurReserve > m_pageMaxReserve)
            m_pageMaxReserve = m_pageCurReserve;
    }
#endif //DEBUG

    return newArena->pages;
}

/*
 * Free a large allocation made via LargeAlloc.
 */
void PAGEHEAP::LargeFree(void * p, size_t sz)
{
    // Find the arena corresponding to this large allocation.
    PAGEARENA * arena = FindArena(p);
    ASSERT(arena && arena->largeAlloc && arena->pages == p && arena->size == sz);

#ifdef TRACKMEM
    if (m_bTrackMem)
        m_pageCurReserve -= sz / pageSize;
#endif

    // Free the pages.
    BOOL b;
    b = VirtualFree(p, 0, MEM_RELEASE); ASSERT(b);

    // Remove the arena from the arena list.
    if (arenaList == arena) {
        arenaList = arena->nextArena;
        if (arenaLast == arena)
            arenaLast = NULL;
    }
    else {
        PAGEARENA * arenaPrev;

        // Find arena just before the one we want to remove
        for (arenaPrev = arenaList; arenaPrev->nextArena != arena; arenaPrev = arenaPrev->nextArena)
            ;

        ASSERT(arenaPrev->nextArena == arena);
        arenaPrev->nextArena = arena->nextArena;
        if (arenaLast == arena)
            arenaLast = arenaPrev;
   }

    // Free the arena structure.
    host->GetStandardHeap()->Free(arena);
}

/*
 * Free everything allocated by the page heap; optionally checking for
 * leak (memory that hasn't been freed via FreePages).
 */
void PAGEHEAP::FreeAllPages(bool checkLeaks)
{
    PAGEARENA * arena, *nextArena;

    for (arena = arenaList; arena != NULL; arena = nextArena) {
        nextArena = arena->nextArena;

        // Check arena for leaks, if desired.
        if (checkLeaks) {
            ASSERT(! arena->largeAlloc);        // Large allocation should have been freed by now.

            for (int dwIndex = 0; dwIndex < PAGES_PER_ARENA / BITS_DWORD; ++dwIndex) {
                ASSERT(arena->used[dwIndex] == 0);  // All pages in this arena should be free.
            }
        }


        // Free the pages in the arena.
        BOOL b;
        b = VirtualFree(arena->pages, arena->size, MEM_DECOMMIT); ASSERT(b);
        b = VirtualFree(arena->pages, 0, MEM_RELEASE);            ASSERT(b);

        // Free the arena structure.
        host->GetStandardHeap()->Free(arena);
    }

#ifdef TRACKMEM
    if (m_bTrackMem)
        m_pageCurUse = m_pageCurReserve = 0;
#endif


    arenaList = arenaLast = NULL;
}

/*
 * Create a new memory arena of a size and reserve or commit pages for it.
 * If isLarge is true, set this as a "large allocation" arena and commit
 * the memory. If not just reserve the memory.
 */
PAGEHEAP::PAGEARENA * PAGEHEAP::CreateArena(bool isLarge, size_t sz)
{
    PAGEARENA * newArena;

    // Allocate an arena and reserve pages for it.
    newArena = (PAGEARENA *) host->GetStandardHeap()->AllocZero(sizeof(PAGEARENA));
    newArena->pages = VirtualAlloc(0, sz, isLarge ? MEM_COMMIT : MEM_RESERVE, PAGE_READWRITE);
    if (!newArena->pages) {
        host->NoMemory ();
        return NULL;
    }

    newArena->size = sz;
    newArena->largeAlloc = isLarge;

    // Add to arena list. For efficiency, large allocation arenas are placed
    // at the end, but regular arenas at the beginning. This ensures that
    // regular allocation are almost always satisfied by the first arena in the list.
    if (!arenaList) {
        arenaList = arenaLast = newArena;
    }
    else if (isLarge) {
        // Add to end of arena list.
        arenaLast->nextArena = newArena;
        arenaLast = newArena;
    }
    else {
        // Add to front of arena list
        newArena->nextArena = arenaList;
        arenaList = newArena;
    }

    return newArena;
}

/*
 * Find an arena that contains a particular pointer.
 */
PAGEHEAP::PAGEARENA * PAGEHEAP::FindArena(void * p)
{
    PAGEARENA * arena;

    for (arena = arenaList; arena != NULL; arena = arena->nextArena) {
        if (p >= arena->pages && p < (BYTE *)arena->pages + arena->size)
            return arena;
    }

    ASSERT(0);      // Should find the arena.
    return NULL;
}



/*
 ************************** NRHEAP methods ******************************
 */

/* The "no-release" heap works by incrementally allocating from a page
 * of memory that was gotten from the page allocator. An allocation operation
 * is just an increment of the current pointer, and a range check to make
 * sure that we haven't gone beyond the end of the page. If we do go beyond
 * the end of the page, we just allocate a new page. All the pages
 * are linked together.
 */

/*
 * Constructor.
 */
NRHEAP::NRHEAP(ALLOCHOST * pHost, bool bTrackMem)
{
    host = pHost;
    nextFree = limitFree = NULL;
    pageList = pageLast = NULL;
    m_bTrackMem = bTrackMem;
}

/*
 * Destructor
 */
NRHEAP::~NRHEAP()
{
    FreeHeap();
}

/*
 * Allocate zeroed memory.
 */
void * NRHEAP::_AllocZero(size_t sz
#ifdef DEBUG
, LPCSTR pszFile, UINT iLine    // Again, these aren't used
#endif
)
{
    void * p = _Alloc(sz
#ifdef DEBUG
                      , pszFile, iLine
#endif
    );
    memset(p, 0, sz);  // zero the memory.
    return p;
}

/*
 * Allocate (duplicate) a wide char string.
 */
LPWSTR NRHEAP::AllocStr(LPCWSTR str)
{
    if (str == NULL)
        return NULL;

    LPWSTR strNew = (LPWSTR) Alloc((wcslen(str) + 1) * sizeof(WCHAR));
    wcscpy(strNew, str);
    return strNew;
}


/*
 * An allocation request has overflowed the current page.
 * allocate a new page and try again.
 */
void * NRHEAP::AllocMore(size_t sz)
{
    sz = RoundSize(sz);   // round to appropriate byte boundary.
    NRPAGE * newPage = NewPage(sz);
    if (!newPage) {
        host->NoMemory();
        return NULL;
    }

    ASSERT(newPage->next == NULL);

    // Set all the memory in this page as available for alloc.
    nextFree = newPage->firstAvail;
    limitFree = newPage->limitAvail;

    // Link the page in to the list.
    if (pageLast) {
        pageLast->next = newPage;
        pageLast = newPage;
    }
    else {
        // First page.
        pageList = pageLast = newPage;
    }

    // We must now be able to allocate the memory.
    return Alloc(sz);
}

/*
 * Allocate a new page of memory, with enough size
 * to handle a block of size sz.
 */
NRPAGE * NRHEAP::NewPage(size_t sz)
{
    size_t pageSize = PAGEHEAP::pageSize; // Page size.
    size_t allocSize;
    NRPAGE * newPage;

    // Round up (sz + sizeof(NRPAGE)) to the nearest page size.
    allocSize = (sz + sizeof(NRPAGE) + pageSize - 1) & ~(pageSize - 1);

    // Allocate the new page.
    newPage = (NRPAGE *) host->GetPageHeap()->AllocPages(allocSize);

    if (newPage) {
        // Initialize the new page.
        newPage->next = NULL;
        newPage->firstAvail = (BYTE *)newPage + RoundSize(sizeof(NRPAGE));
        newPage->limitAvail = ((BYTE *)newPage) + allocSize;

        // We should have enough room in the new page!
        ASSERT(newPage->limitAvail > newPage->firstAvail);
        ASSERT((unsigned)(newPage->limitAvail - newPage->firstAvail) >= sz);
    }

    return newPage;
}

/*
 * Return a mark of the current allocation state, so you
 * can free all memory allocated subsequent.
 */
void NRHEAP::Mark(NRMARK * mark)
{
    // Record current page and location.
    mark->page = pageLast;
    mark->nextFree = nextFree;
}

size_t NRHEAP::CalcCommittedSize ()
{
    size_t    iCommit = 0;
    for (NRPAGE *p = pageList; p; p = p->next)
        iCommit += p->limitAvail - (BYTE *)p;

    return iCommit;
}

/*
 * Free all memory allocated after the mark.
 */
void NRHEAP::Free(NRMARK * mark)
{
    if (mark->page == NULL) {
        // Mark was before anything was allocated.
        FreeHeap();
    }
    else {
        NRPAGE * page, *nextPage;

        // Free all pages after the new last one.
        for (page = mark->page->next; page != NULL; page = nextPage)
        {
            nextPage = page->next;
            host->GetPageHeap()->FreePages(page, (BYTE *)page->limitAvail - (BYTE *)page);
        }

        // Reset the last page and location.
        pageLast = mark->page;
        pageLast->next = NULL;
        nextFree = mark->nextFree;
        limitFree = pageLast->limitAvail;

#ifdef DEBUG
        // Free rest of page with junk
        memset(nextFree, 0xEE, limitFree - nextFree);
#endif //DEBUG
    }
}

/*
 * Free the entire heap.
 */
void NRHEAP::FreeHeap()
{
    NRPAGE * page, *nextPage;

    // Free all the pages.
    for (page = pageList; page != NULL; page = nextPage) {
        nextPage = page->next;
        host->GetPageHeap()->FreePages(page, (BYTE *)page->limitAvail - (BYTE *)page);
    }

    // Reset the allocator.
    nextFree = limitFree = NULL;
    pageList = pageLast = NULL;
}
