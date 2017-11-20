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
// File: namemgr.cpp
//
// Routines for storing and handling names.
// ===========================================================================


#include "stdafx.h"

#include "compiler.h"
#include "privguid.h"

// NOTE:  If you add a keyword and the assert in AddKeywords() fires, do this:
//  1) uncomment the following #define
//  2) rebuild
//  3) run the command line driver (no arguments).  IT WILL CRASH (bp) but not before reporting a new hash start value
//  4) replace the value of giHashStart below with the value reported above
//  5) comment this line back out and rebuild.

//#define FIND_HASH_START


#ifndef FIND_HASH_START
const
#endif
unsigned    giHashStart = 0xB16358D8;

/*
 * Static array of the predefined names.
 */
#define PREDEFNAMEDEF(id, name)      L##name,

const LPCWSTR predefNameInfo[] =
{
    #include "predefname.h"
};

/*
 *********************************************
 * NAMEMGR methods.
 */

#if defined(_MSC_VER)
#pragma warning(disable:4355)  // allow "this" in member initializer list
#endif  // defined(_MSC_VER)

/*
 * Constructor.
 */
NAMEMGR::NAMEMGR() :
    iRef(0),
    buckets(NULL),
    cBuckets(0),
    bucketMask(0),
    cNames(0),
    pStdHeap(NULL),
    pNRHeap(NULL),
    pPageHeap(NULL)
{
}

#if defined(_MSC_VER)
#pragma warning(default:4355)
#endif  // defined(_MSC_VER)

/*
 * Destructor.
 */
NAMEMGR::~NAMEMGR()
{
    //PrintStats();  // Uncomment to see name managed hashing statistics after the end of the run.
    ClearAll();
    if (pNRHeap)
        delete pNRHeap;
    if (pPageHeap)
        delete pPageHeap;
    if (pStdHeap)
        delete pStdHeap;
}


/*
 * Clear all names, leaving the bucket table allocated.
 */
void NAMEMGR::ClearNames()
{
    // Acquire the lock
    CTinyGate   gate (&lock);

    // Free all the allocated names and empty the bucket table.
    if (pNRHeap)
        pNRHeap->FreeHeap();
    memset(buckets, 0, cBuckets * sizeof(PNAME));
    cNames = NULL;
}

/*
 * Clear all the names and free the bucket table.
 */
void NAMEMGR::ClearAll()
{
    // Acquire the lock
    CTinyGate   gate (&lock);

    ClearNames();

    // Free the bucket table.
    if (buckets) {
        size_t cb = cBuckets * sizeof(PNAME);
        if (cb < PAGEHEAP::pageSize)
            cb = PAGEHEAP::pageSize;
        pPageHeap->FreePages(buckets, cb);
    }

    // Free the keyword table.
    if (kwds) {
        pPageHeap->FreePages(kwds, PAGEHEAP::pageSize);
    }

    kwds = NULL;
    buckets = NULL;
    cBuckets = bucketMask = 0;
}

// HashUInt moved to namemgr.h, to be inlined

/*
 * Hash a string to return a 32-bit hash value.
 */

unsigned NAMEMGR::HashString(LPCWSTR str, int length)
{

    unsigned hash = giHashStart;


    ASSERT(sizeof(DWORD)/sizeof(WCHAR) == 2);

    // Hash two characters/one dword at a time.
    while (length >= 2) {
        length -= 2;
#if BIGENDIAN
        // There's an assumption that the hash values fit into certain
        // ranges so make sure it matches little endian format
        hash += *str | *(str+1) << 16;
#else
        hash += GET_UNALIGNED_32(str);
#endif
        hash += (hash << 10);
        hash ^= (hash >> 6);
        str += 2;
    }

    // Hash the last character, if any.
    if (length > 0) {
        hash += *str;
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }

    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);

    return HashUInt(hash);
}


/*
 * Lookup a string in the name table. If it's not there, then return NULL.
 */
PNAME NAMEMGR::LookupString(LPCWSTR str, int length, unsigned hash)
{
    unsigned ibucket;
    PNAME name;

#ifdef DEBUG
    // Embedded NUL characters are NOT allowed in the string.
    for (int i = 0; i < length; ++i)
        ASSERT(str[i] != 0);
#endif //DEBUG

    // Acquire the lock
    CTinyGate   gate (&lock);

    if (buckets == NULL)
        return NULL;        // Nothing in the table.

    // Get the bucket.
    ibucket = (hash & bucketMask);

    // Search all names in the bucket for the string we want.
    name = buckets[ibucket];
    while (name) {
        if (name->hash == hash) {
            // Hash values match... pretty good sign. Check length
            // and contents.
            if ((int)wcslen(name->text) == length &&
                memcmp(name->text, str, length * sizeof(WCHAR)) == 0)
            {
                // Names match!
                return name;
            }
        }
        name = name->nextInBucket;
    }

    return NULL;
}

/*
 * Simple lookup string that takes just buffer and length.
 */
PNAME NAMEMGR::LookupString(LPCWSTR str, int length)
{
    return LookupString(str, length, HashString(str, length));
}

/*
 * Simple lookup string the just takes zero-terminated string.
 */
PNAME NAMEMGR::LookupString(LPCWSTR str)
{
    int length = (int)wcslen(str);
    return LookupString(str, length, HashString(str, length));
}



/*
 * Add a string to the name table. If it's already there
 * return it.
 */
PNAME NAMEMGR::AddString(LPCWSTR str, int length, unsigned hash, bool * wasAdded)
{
    unsigned ibucket;
    PNAME name;
    PNAME * link; // link to place to add new name.
#ifdef DEBUG
    int cInBucket = 0;  // number of items in this bucket.
#endif //DEBUG

#ifdef DEBUG
    // Embedded NULL characters are NOT allowed in the string.
    for (int i = 0; i < length; ++i)
        ASSERT(str[i] != 0);
#endif //DEBUG

    // Acquire the lock
    CTinyGate   gate (&lock);

    if (buckets == NULL) {
        // Nothing in the table. Create a new empty table.
        ASSERT(cNames == 0);

        buckets = (PNAME *) pPageHeap->AllocPages(PAGEHEAP::pageSize);
        if (buckets == NULL)
            return NULL;

        ASSERT((PAGEHEAP::pageSize / sizeof(PNAME) < UINT_MAX));
        cBuckets = (unsigned)(PAGEHEAP::pageSize / sizeof(PNAME));

        memset(buckets, 0, cBuckets * sizeof(PNAME));
        bucketMask = cBuckets - 1;
    }

    // Get the bucket.
    ibucket = (hash & bucketMask);

    // Traverse all names in the bucket.
    link = & buckets[ibucket];
    for (;;) {
        name = *link;

        if (!name)
            break;  // End of list.

#ifdef DEBUG
        ++cInBucket;
#endif //DEBUG

        if (name->hash == hash) {
            // Hash values match... pretty good sign. Check contents.

            // Check one DWORD at a time, in an unrolled loop.
            const WCHAR * pLastChar = str + length;  // Point to beyond last char .
            const DWORD * pdw1 = (DWORD *) str;
            const DWORD * pdw2 = (DWORD *) name->text;
#ifdef ALIGN_ACCESS
            ASSERT(((BYTE)pdw2 & 0x03) == 0);  // The namemgr keeps it's strings pointer aligned
            if (((BYTE)pdw1 & 0x03) != 0) {
                // If the lookup string is not aligned, just use a string compare
                if (wcsncmp(str, name->text, length) == 0 &&
                    // Also check the length (in case the new string has the same hash and is a
                    // perfect substring of the existing one)
                    name->text[length] == L'\0')
                    goto MATCH;
                else
                    goto NOMATCH;
            }
#endif
            for (;;) {
                switch (pLastChar - (const WCHAR *)pdw1) {
                default:
                    if (*pdw1++ != *pdw2++)
                        goto NOMATCH;
                case 15:
                case 14:
                    if (*pdw1++ != *pdw2++)
                        goto NOMATCH;
                case 13:
                case 12:
                    if (*pdw1++ != *pdw2++)
                        goto NOMATCH;
                case 11:
                case 10:
                    if (*pdw1++ != *pdw2++)
                        goto NOMATCH;
                case 9:
                case 8:
                    if (*pdw1++ != *pdw2++)
                        goto NOMATCH;
                case 7:
                case 6:
                    if (*pdw1++ != *pdw2++)
                        goto NOMATCH;
                case 5:
                case 4:
                    if (*pdw1++ != *pdw2++)
                        goto NOMATCH;
                case 3:
                case 2:
                    if (*pdw1++ != *pdw2++)
                        goto NOMATCH;
                    break;

                case 1:
                    // check last character and presence of null terminator on pdw2
                    if (*(WCHAR*)pdw1 != *(WCHAR*)pdw2 || *((WCHAR*)pdw2 + 1) != 0)
                        goto NOMATCH;
                    goto MATCH;

                case 0:
                    if (*(WCHAR *)pdw2 != L'\0')
                        goto NOMATCH;
                    goto MATCH;
                }
            }

        MATCH:
            // Names match!
            *wasAdded = false;
            return name;

        NOMATCH:
            ; // Names didn't match.
        }
        link = & name->nextInBucket;
    }

    ASSERT(cInBucket < 12);      // If this triggers, our hash function is no good.

    // Name not found; create new and add it to the table.
    name = (NAME *) pNRHeap->Alloc(sizeof(NAME) + (length + 1) * sizeof(WCHAR));
    if (name == NULL)
        return NULL;

    *link = name;

    // Fill in the new name structure and copy in name.
    name->hash = hash;
    name->nextInBucket = 0;
    memcpy((PWSTR)name->text, str, length * sizeof(WCHAR));
    *((WCHAR *)name->text + length) = L'\0';

    // If the name table is full, resize it.
    ++cNames;
    if (cNames >= cBuckets)
        GrowTable();

    *wasAdded = true;
    return name;
}

/*
 * Simple add string the just takes zero-terminated string.
 */
PNAME NAMEMGR::AddString(LPCWSTR str)
{
    bool dummy;
    int length = (int)wcslen(str);
    return AddString(str, length, HashString(str, length), &dummy);
}

/*
 * Double the bucket size of the name table.
 */
void NAMEMGR::GrowTable()
{
    unsigned newSize;
    size_t cbNew;
    PNAME * newBuckets;
    unsigned iBucket;
    PNAME * linkLow, * linkHigh;
    PNAME name;

    // Must be locked!
    ASSERT (lock.LockedByMe());

    // Allocate the new bucket table.
    newSize = cBuckets * 2;
    cbNew = newSize * sizeof(PNAME);
    if (cbNew < PAGEHEAP::pageSize)
        cbNew = PAGEHEAP::pageSize;
    newBuckets = (PNAME *) pPageHeap->AllocPages(cbNew);
    if (newBuckets == NULL)
    {
        return;
    }

    // Go throw each bucket in the old table, and link the names
    // into the new tables. Because the old table is a power of 2 in size,
    // and the new table is twice that size, names from bucket n in the
    // old table must map to either bucket n or bucket n + oldSize in the new
    // table. Which of these two is determined by (hash & oldSize).
    for (iBucket = 0; iBucket < cBuckets; ++iBucket)
    {
        // Get starting links for the two new buckets.
        linkLow = &newBuckets[iBucket];
        linkHigh = &newBuckets[iBucket + cBuckets];

        // Go through each name in the old bucket, redistributing
        // to the two new buckets.
        name = buckets[iBucket];
        while (name) {
            if (name->hash & cBuckets) {
                *linkHigh = name;
                linkHigh = & name->nextInBucket;
            }
            else {
                *linkLow = name;
                linkLow = & name->nextInBucket;
            }

            name = name->nextInBucket;
        }

        // Terminate both new linked lists.
        *linkLow = *linkHigh = NULL;
    }

    // Free the old bucket table.
    size_t cb = cBuckets * sizeof(PNAME);
    if (cb < PAGEHEAP::pageSize)
        cb = PAGEHEAP::pageSize;
    pPageHeap->FreePages(buckets, cb);

    // Remember the new bucket table and size.
    buckets = newBuckets;
    cBuckets = newSize;
    bucketMask = cBuckets - 1;
}


/*
 * Add all the keywords to the name table. To enable fast determination
 * of whether a particular NAME is a keywords, a seperate keyword hash table
 * is used that is indexed by the hash value of the name. At most one keyword
 * has the same index, so we don't need a collision resolution scheme.
 */
HRESULT NAMEMGR::AddKeywords()
{
    PNAME name;
    unsigned sizeTable;
    unsigned iTable;
    int i;

    // Must be locked!
    ASSERT (lock.LockedByMe());

    // Allocate the keyword table, using one page of memory.
    ASSERT((PAGEHEAP::pageSize / sizeof(KWDENTRY) < UINT_MAX));
    sizeTable = (unsigned)(PAGEHEAP::pageSize / sizeof(KWDENTRY));
    kwdMask = sizeTable - 1;
    kwds = (KWDENTRY *) pPageHeap->AllocPages(PAGEHEAP::pageSize);
    if (kwds == NULL)
        return E_OUTOFMEMORY;

#ifdef FIND_HASH_START
    unsigned    iHashStartOrg = giHashStart;
tryagain:
#endif
    memset(kwds, 0, PAGEHEAP::pageSize);

    // Add each keyword first to the name table, then record the
    // name and keyword id in the keyword table.
    for (i = 0; i < TID_IDENTIFIER; ++i) {
        const TOKINFO * tokinfo = CParser::GetTokenInfo((TOKENID)i);

        name = AddString(tokinfo->pszText);
        if (name == NULL)
            return E_OUTOFMEMORY;
        iTable = (name->hash & kwdMask);

#ifdef FIND_HASH_START
        if (kwds[iTable].name != NULL)
        {
            // Whack the table, adjust the starting hash value and try again
            ClearNames();
            if (++giHashStart == iHashStartOrg)
            {
                // No value works.  Either make a new hash function, or grow the array!
                printf ("ERROR:  HASH FUNCTION DOESN'T WORK!  (See %s(%d))\n", __FILE__, __LINE__);
                DebugBreak();
            }

            goto tryagain;
        }
#else
        // If this triggers, we have a collision.  See the top of
        // this file for how to fix...
        ASSERT(kwds[iTable].name == NULL);
#endif

        kwds[iTable].name = name;
        kwds[iTable].iKwd = (TOKENID)i;
    }

#ifdef FIND_HASH_START
    printf ("APPROPRIATE HASH START VALUE FOUND: %08X\n", giHashStart);
    DebugBreak(); // can't continue; name table is potentially missing some names now...
#endif

    // Initialize the preprocessor keywords
    #define PPKWD(n,id,d) n,
    const PCWSTR  rgszKwds[] = {
        #include "ppkwds.h"
        NULL };

    ASSERT (PPT_DEFINE == 0);
    for (i=PPT_DEFINE; i < PPT_IDENTIFIER; i++)
        m_rgpPreprocKwds[i] = AddString (rgszKwds[i]);

    // Initialize the attribute location table
    #define ATTRLOCDEF(id, value, str) str,
    const PCWSTR rgszAttrLocs[] = {
        #include "attrloc.h"
        };

    for (i = 0; i < ATTRLOC_COUNT; i++)
        m_rgpAttributeLocs[i] = AddString(rgszAttrLocs[i]);

    return S_OK;
}

bool NAMEMGR::IsPPKeyword (NAME *p, int *piPP)
{
    // NOTE:  There are currently only 8 pp keywords, so a linear search is okay...
    for (int i=0; i<PPT_IDENTIFIER; i++)
    {
        if (m_rgpPreprocKwds[i] == p)
        {
            *piPP = i;
            return true;
        }
    }

    return false;
}

bool NAMEMGR::IsAttrLoc(NAME *pName, int *piAttrLoc)
{
    for (int i = 0; i < ATTRLOC_COUNT; i ++)
    {
        if (m_rgpAttributeLocs[i] == pName)
        {
            *piAttrLoc = (1 << i);
            return true;
        }
    }

    return false;
}

PNAME NAMEMGR::GetAttrLoc(int index)
{
    ASSERT(index >=0 && index < ATTRLOC_COUNT);
    return m_rgpAttributeLocs[index];
}

/*
 * Get the name corresponding to a keyword.
 */
PNAME NAMEMGR::KeywordName(int iKwd)
{
    return LookupString (CParser::GetTokenInfo((TOKENID)iKwd)->pszText);
}

/*
 * Add all the predefined names to the name table. These are common
 * names we use a lot. We also all the keywords.
 */
HRESULT NAMEMGR::AddPredefNames()
{
    HRESULT     hr;

    // Add the keywords.
    if (FAILED (hr = AddKeywords()))
        return hr;

    // Allocate space for the predefined names.
    predefNames = (PNAME *) pNRHeap->Alloc(PN_COUNT * sizeof(PNAME));
    if (predefNames == NULL)
        return E_OUTOFMEMORY;

    // Add all the predefined names to the name table, and remember their names.
    for (int i = 0; i < PN_COUNT; ++i) {
        if ((predefNames[i] = AddString(predefNameInfo[i])) == NULL)
            return E_OUTOFMEMORY;
    }

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// NAMEMGR::Init

HRESULT NAMEMGR::Init ()
{
    CTinyGate   gate (&lock);

    if (pStdHeap != NULL)
        return E_FAIL;      // Already initialized.  What are you doing?

    pStdHeap = new MEMHEAP (this, true);
    if (pStdHeap == NULL)
        return E_OUTOFMEMORY;

    pPageHeap = new PAGEHEAP (this, true);
    if (pPageHeap == NULL)
        return E_OUTOFMEMORY;

    pNRHeap = new NRHEAP (this);
    if (pNRHeap == NULL)
        return E_OUTOFMEMORY;

    return AddPredefNames ();
}

////////////////////////////////////////////////////////////////////////////////
// NAMEMGR::Lookup

STDMETHODIMP NAMEMGR::Lookup (PCWSTR pszName, NAME **ppName)
{
    *ppName = LookupString (pszName);
    return *ppName ? S_OK : S_FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// NAMEMGR::Lookup

STDMETHODIMP NAMEMGR::LookupLen (PCWSTR pszName, long iLen, NAME **ppName)
{
    *ppName = LookupString (pszName, iLen);
    return *ppName ? S_OK : S_FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// NAMEMGR::Add

STDMETHODIMP NAMEMGR::Add (PCWSTR pszName, NAME **ppName)
{
    *ppName = AddString (pszName);
    return *ppName ? S_OK : E_OUTOFMEMORY;
}

////////////////////////////////////////////////////////////////////////////////
// NAMEMGR::Add

STDMETHODIMP NAMEMGR::AddLen (PCWSTR pszName, long iLen, NAME **ppName)
{
    *ppName = AddString (pszName, iLen);
    return *ppName ? S_OK : E_OUTOFMEMORY;
}

////////////////////////////////////////////////////////////////////////////////
// NAMEMGR::IsValidIdentifer

STDMETHODIMP NAMEMGR::IsValidIdentifier (PCWSTR pszName, VARIANT_BOOL *pfValid)
{
    *pfValid = VARIANT_TRUE;

    bool atKeyword = false;

    if (*pszName == L'@')
    {
        pszName++;
        atKeyword = true;
    }

    if (!IsIdentifierChar (*pszName))
    {
        *pfValid = VARIANT_FALSE;
    }
    else
    {
        PCWSTR p;
        for (p = pszName + 1; *p; p++)
        {
            if (!IsIdentifierCharOrDigit (*p))
            {
                *pfValid = VARIANT_FALSE;
                return S_OK;
            }
        }

        if (p - pszName >= MAX_IDENT_SIZE)
            *pfValid = VARIANT_FALSE;  // too long!

        NAME    *pName = AddString (pszName);

        if (pName == NULL)
        {
            return E_OUTOFMEMORY;
        }

        int     kwd;
        if (!atKeyword && IsKeyword (pName, &kwd))
            *pfValid = VARIANT_FALSE;
    }

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// NAMEMGR::GetTokenText

STDMETHODIMP NAMEMGR::GetTokenText (long iTokenId, PCWSTR *ppszText)
{
    if (iTokenId < 0 || iTokenId >= TID_INVALID)
        return E_INVALIDARG;

    *ppszText = CParser::GetTokenInfo((TOKENID)iTokenId)->pszText;
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// NAMEMGR::GetPredefinedName

STDMETHODIMP NAMEMGR::GetPredefinedName (long iPredefName, NAME **ppName)
{
    if (iPredefName < 0 || iPredefName >= PN_COUNT)
        return E_FAIL;

    *ppName = GetPredefName ((PREDEFNAME)iPredefName);
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// NAMEMGR::GetPreprocessorDirective

STDMETHODIMP NAMEMGR::GetPreprocessorDirective (NAME *pName, long *piToken)
{
    int     i;

    if (IsPPKeyword (pName, &i))
    {
        *piToken = i;
        return S_OK;
    }

    return E_FAIL;
}

////////////////////////////////////////////////////////////////////////////////
// NAMEMGR::IsKeyword

STDMETHODIMP NAMEMGR::IsKeyword (NAME *pName, long *piToken)
{
    int     iKwd;

    if (IsKeyword (pName, &iKwd))
    {
        if (piToken != NULL)
            *piToken = (long)iKwd;
        return S_OK;
    }

    return S_FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// NAMEMGR::QueryInterface

STDMETHODIMP NAMEMGR::QueryInterface (REFIID riid, void **ppObj)
{
    if (riid == IID_IUnknown || riid == IID_ICSNameTable)
    {
        *ppObj = (ICSNameTable *)this;
        AddRef();
        return S_OK;
    }

    if (riid == IID_ICSPrivateNameTable)
    {
        // NOTE:  This is a hack -- the caller of QI with this guid assumes the ppObj out is a NAMEMGR *
        *ppObj = this;
        AddRef();
        return S_OK;
    }

    *ppObj = NULL;
    return E_NOINTERFACE;
}

////////////////////////////////////////////////////////////////////////////////
// NAMEMGR::AddRef

STDMETHODIMP_(ULONG) NAMEMGR::AddRef ()
{
    return InterlockedIncrement ((LONG *)&iRef);
}

////////////////////////////////////////////////////////////////////////////////
// NAMEMGR::Release

STDMETHODIMP_(ULONG) NAMEMGR::Release ()
{
    ULONG   i = InterlockedDecrement ((LONG *)&iRef);

    if (i == 0)
        delete this;

    return i;
}



////////////////////////////////////////////////////////////////////////////////
// NAMETAB::~NAMETAB

NAMETAB::~NAMETAB ()
{
    if (m_ppNames != NULL)
        m_pAllocHost->GetStandardHeap()->Free (m_ppNames);
}

////////////////////////////////////////////////////////////////////////////////
// NAMETAB::Define

void NAMETAB::Define (NAME *pName)
{
    // Make sure it's not already there
    for (long i=0; i<m_iCount; i++)
        if (m_ppNames[i] == pName)
            return;

    // Make sure there's room
    if ((m_iCount & 7) == 0)
    {
        long    iSize = (m_iCount + 8) * sizeof (NAME *);
        NAME    **ppNew = (NAME **)(m_ppNames == NULL ? m_pAllocHost->GetStandardHeap()->Alloc (iSize) :
                                                        m_pAllocHost->GetStandardHeap()->Realloc (m_ppNames, iSize));

        if (ppNew == NULL)
            return;     // REVIEW:  Silent failure?  Seems bad...

        m_ppNames = ppNew;
    }

    m_ppNames[m_iCount++] = pName;
}

////////////////////////////////////////////////////////////////////////////////
// NAMETAB::Undef

void NAMETAB::Undef (NAME *pName)
{
    // Find it
    for (long i=0; i<m_iCount; i++)
    {
        if (m_ppNames[i] == pName)
        {
            // Remove it.  Replace this slot with the one at the end, if this
            // isn't the one on the end...
            if (i < m_iCount - 1)
                m_ppNames[i] = m_ppNames[m_iCount - 1];

            m_iCount--;
            break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// NAMETAB::IsDefined

BOOL NAMETAB::IsDefined (NAME *pName)
{
    for (long i=0; i<m_iCount; i++)
        if (m_ppNames[i] == pName)
            return TRUE;

    return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// NAMETAB::ClearAll

void NAMETAB::ClearAll (BOOL fFree)
{
    m_iCount = 0;
    if (fFree && (m_ppNames != NULL))
    {
        m_pAllocHost->GetStandardHeap()->Free (m_ppNames);
        m_ppNames = NULL;
    }
}



#ifdef DEBUG
/*
 * Print statistics on the hash table. This can be used to
 * discover whether the hash function is performing adequately.
 */
void NAMEMGR::PrintStats()
{
    unsigned iBucket;
    int cEmpty = 0, cOneEntry = 0, cTwoEntry = 0, cManyEntries = 0, maxEntries = 0;;

    // Acquire the lock
    CTinyGate   gate (&lock);

    for (iBucket = 0; iBucket < cBuckets; ++iBucket) {
        int cEntries = 0;
        PNAME name;

        // Count entries in the bucket.
        name = buckets[iBucket];
        while (name != NULL) {
            ++ cEntries;
            name = name->nextInBucket;
        }

        // Classify.
        if (cEntries == 0)
            ++cEmpty;
        else if (cEntries == 1)
            ++cOneEntry;
        else if (cEntries == 2)
            ++cTwoEntry;
        else
            ++cManyEntries;

        if (cEntries > maxEntries)
            maxEntries = cEntries;
    }

    // Print stats.
    printf("Total buckets:                %d\n", cBuckets);
    printf("Total names:                  %d\n", cNames);
    printf("\n");
    printf("Empty buckets:                %d (%.2f%%)\n", cEmpty, (double)cEmpty/cBuckets*100.0);
    printf("Buckets with one entry:       %d (%.2f%%)\n", cOneEntry, (double)cOneEntry/cBuckets*100.0);
    printf("Buckets with two entries:     %d (%.2f%%)\n", cTwoEntry, (double)cTwoEntry/cBuckets*100.0);
    printf("Buckets with 3+ entries:      %d (%.2f%%)\n", cManyEntries, (double)cManyEntries/cBuckets*100.0);
    printf("Max bucket size:              %d\n", maxEntries);
}
#endif //DEBUG

/*
////////////////////////////////////////////////////////////////////////////////
// HACK:  Lock contention profiling data

long CTinyLock::m_iSpins = 0;
long CTinyLock::m_iSleeps = 0;
*/
