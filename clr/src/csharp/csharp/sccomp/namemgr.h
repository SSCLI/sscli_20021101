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
// File: namemgr.h
//
// Defines the structures used to store and manage names.
// ===========================================================================

#ifndef __namemgr_h__
#define __namemgr_h__

#include "enum.h"
#include "name.h"
#include "locks.h"
#include "alloc.h"

#ifndef UInt32x32To64
#define UInt32x32To64(a, b) ((DWORDLONG)((DWORD)(a)) * (DWORDLONG)((DWORD)(b)))
#endif

/*
 * A NAMEMGR stores names. An hash table is used with
 * buckets that lead to a linked list of names.
 */

class NAMEMGR : public ICSNameTable, public ALLOCHOST
{
public:
    NAMEMGR();
    ~NAMEMGR();

    //void * operator new(size_t size) { return VSAlloc (size); }
    //void operator delete(void * p) { VSFree (p); }

    static unsigned HashString(LPCWSTR str, int length);

    HRESULT     Init ();

    PNAME AddString(LPCWSTR str, int length, unsigned hash, bool * wasAdded);
    PNAME AddString(LPCWSTR str, int length);
    PNAME AddString(LPCWSTR str);
    PNAME LookupString(LPCWSTR str, int length, unsigned hash);
    PNAME LookupString(LPCWSTR str, int length);
    PNAME LookupString(LPCWSTR str);
    bool IsKeyword(PNAME name, int * iKwd);
    bool IsPPKeyword(NAME *pName, int *piPPKwd);
    bool IsAttrLoc(NAME *pName, int *piAttrLoc);
    PNAME GetAttrLoc(int index);
    PNAME KeywordName(int iKwd);

    PNAME GetPredefName(PREDEFNAME pn)      { return predefNames[pn]; }


    void ClearNames();
    void ClearAll();
    void Term();

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // ICSNameTable
    STDMETHOD(Lookup)(PCWSTR pszName, NAME **ppName);
    STDMETHOD(LookupLen)(PCWSTR pszName, long iLen, NAME **ppName);
    STDMETHOD(Add)(PCWSTR pszName, NAME **ppName);
    STDMETHOD(AddLen)(PCWSTR pszName, long iLen, NAME **ppName);
    STDMETHOD(IsValidIdentifier)(PCWSTR pszName, VARIANT_BOOL *pfValid);
    STDMETHOD(GetTokenText)(long iTokenId, PCWSTR *ppszText);
    STDMETHOD(GetPredefinedName)(long iPredefName, NAME **ppName);
    STDMETHOD(GetPreprocessorDirective)(NAME *pName, long *piToken);
    STDMETHOD(IsKeyword)(NAME *pName, long *piToken);

    // ALLOCHOST
    void        NoMemory () {}
    MEMHEAP     *GetStandardHeap () { ASSERT (pStdHeap != NULL); return pStdHeap; }
    PAGEHEAP    *GetPageHeap () { ASSERT (pPageHeap != NULL); return pPageHeap; }

#ifdef DEBUG
    void PrintStats();
#endif //DEBUG

private:
    HRESULT AddPredefNames();
    HRESULT AddKeywords();
    void GrowTable();

    ULONG       iRef;           // Ref count

    CTinyLock lock;             // This is the lock mechanism for thread safety.  NOTE:  All public operations on the name table
                                // currently acquire this lock throughout the entire operation.  If a more granular solution is
                                // necessary, we could store an additional lock per bucket, and release this 'global' lock as soon
                                // as the desired bucket head is determined (locking that bucket first of course).  We'll not do
                                // this unless performance numbers indicate the need for it, however.

    PNAME * buckets;            // the buckets of the name table.
    unsigned cBuckets;          // number of buckets
    unsigned bucketMask;        // mask, always cBuckets - 1.
    unsigned cNames;            // number of names added.

    MEMHEAP     *pStdHeap;      // allocator for general use
    NRHEAP      *pNRHeap;       // allocator to allocate NAME structures from
    PAGEHEAP    *pPageHeap;     // allocator to allocate pages from

    struct KWDENTRY {           // NOTE:  This structure's size must remain a power of 2!
        PNAME name;
        TOKENID iKwd;
    };
    KWDENTRY * kwds;            // table of all keywords.
    unsigned kwdMask;           // mask to use to look up in keyword table.

    PNAME * predefNames;        // array with all the predefined names.

    NAME    *m_rgpPreprocKwds[TID_IDENTIFIER];// Names of preprocessor keywords
    NAME    *m_rgpAttributeLocs[ATTRLOC_COUNT]; // Names of attribute locations

};


// A generally useful hashing function. Uses no state.
/*
 * Hash a 32-bit value to get a much more randomized
 * 32-bit value. (Uses 64-bit values on Win64)
 * Inlined so that
 */
__forceinline unsigned HashUInt(UINT_PTR u)
{

    // Equivalent portable code.
    unsigned __int64 l = UInt32x32To64(u, 0x7ff19519U);  // this number is prime.
    return (unsigned)l + (unsigned)(l >> 32);

}



////////////////////////////////////////////////////////////////////////////////
// NAMETAB
//

class NAMETAB
{
private:
    NAME        **m_ppNames;        // The defined names
    long        m_iCount;           // Number of defined names
    ALLOCHOST   *m_pAllocHost;      // Allocator host

public:
    NAMETAB (ALLOCHOST *p) : m_ppNames(NULL), m_iCount(0), m_pAllocHost(p) {}
    ~NAMETAB();

    void    Define (NAME *pName);
    void    Undef (NAME *pName);
    BOOL    IsDefined (NAME *pName);
    void    ClearAll (BOOL fFree = FALSE);
    long    GetCount () { return m_iCount; }
    NAME    *GetAt (long iIndex) { return m_ppNames[iIndex]; }
};

// Various inlines based on profiles.

/*
 * Check to see if a name is a keyword, and if so, return
 * the keyword id.
 */
__forceinline bool NAMEMGR::IsKeyword(PNAME name, int * iKwd)
{
    // Find index into the keyword table.
    unsigned iTable = name->hash & kwdMask;

    if (kwds[iTable].name == name) {
        // Yes, we are a keyword.
        *iKwd = kwds[iTable].iKwd;
        return true;
    }
    else {
        // No, not a keyword.
        return false;
    }
}

/*
 * Simple add string that takes just buffer and length.
 */
__forceinline PNAME NAMEMGR::AddString(LPCWSTR str, int length)
{
    bool dummy;
    return AddString(str, length, HashString(str, length), &dummy);
}

#endif //__namemgr_h__

