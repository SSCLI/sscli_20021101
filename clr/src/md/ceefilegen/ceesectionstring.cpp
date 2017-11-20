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
// File: CeeSectionString.cpp
// 
// ===========================================================================
#include "stdafx.h"

struct StringTableEntry {
    ULONG m_hashId;
    int m_offset;
    StringTableEntry *m_next;
};

CeeSectionString::CeeSectionString(CCeeGen &ceeFile, CeeSectionImpl &impl)
    : CeeSection(ceeFile, impl)
{
    memset(stringTable, 0, sizeof(stringTable));
}

void CeeSectionString::deleteEntries(StringTableEntry *e)
{
    if (!e)
        return;
    deleteEntries(e->m_next);
    delete e;
}

#ifdef RDATA_STATS
int CeeSectionString::dumpEntries(StringTableEntry *e)
{
    if (!e)
        return 0;
    else {
        printf("    HashId: %d, value: %S\n", e->m_hashId, computOffset(e->m_offset));
        return dumpEntries(e->m_next) + 1;
    }
}

void CeeSectionString::dumpTable()
{
    int sum = 0, count = 0;
    for (int i=0; i < MaxRealEntries; i++) {
        if (stringTable[i]) {
            printf("Bucket %d\n", i);
            printf("Total size: %d\n\n", 
                    count = dumpEntries(stringTable[i]));
            sum += count;
        }
    }
    printf("Total number strings: %d\n\n", sum);
}
#endif

CeeSectionString::~CeeSectionString()
{
#ifdef RDATA_STATS
    dumpTable();
#endif
    for (int i=0; i < MaxRealEntries; i++)
        deleteEntries(stringTable[i]);
}

StringTableEntry* CeeSectionString::createEntry(LPWSTR target, ULONG hashId) 
{
    StringTableEntry *entry = new StringTableEntry;
    if (!entry)
        return NULL;
    entry->m_next = NULL;
    entry->m_hashId = hashId;
    entry->m_offset = dataLen();
    ULONG len = (lstrlenW(target)+1) * sizeof(wchar_t);
    void *buf = getBlock(len);
    if (!buf) {
        delete entry;
        return NULL;
    }
    memcpy(buf, target, len);
    return entry;
}

// Searches through the linked list looking for a match on hashID. If
// multiple elements hash to the same value, a strcmp must be done to
// check for match. The goal is to have very large hashId space so that
// string compares are minimized
StringTableEntry *CeeSectionString::findStringInsert(
                        StringTableEntry *&head, LPWSTR target, ULONG hashId)
{
    StringTableEntry *cur, *prev;
    cur = prev = head;
    while (cur && cur->m_hashId < hashId) {
        prev = cur;
        cur = cur->m_next;
    }
    while (cur && cur->m_hashId == hashId) {
        if (wcscmp(target, (LPWSTR)(computePointer(cur->m_offset))) == 0)
            return cur;
        prev = cur;
        cur = cur->m_next;
    }
    // didn't find in chain so insert at prev
    StringTableEntry *entry = createEntry(target, hashId);
    if (cur == head) {
        head = entry;
        entry->m_next = prev;
    } else {
        prev->m_next = entry;
        entry->m_next = cur;
    }
    return entry;
}

HRESULT CeeSectionString::getEmittedStringRef(LPWSTR target, StringRef *ref)
{
    TESTANDRETURN(ref!=NULL, E_POINTER);
    ULONG hashId = HashString(target) % MaxVirtualEntries;
    ULONG bucketIndex = hashId / MaxRealEntries;

    StringTableEntry *entry;
    entry = findStringInsert(stringTable[bucketIndex], target, hashId);

    if (! entry)
        return E_OUTOFMEMORY;
    *ref = entry->m_offset;
    return S_OK;
}
