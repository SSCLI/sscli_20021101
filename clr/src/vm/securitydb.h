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
#ifndef _SECURITY_DB_H_
#define _SECURITY_DB_H_

#define INDEX_FILE L"securitydb.idx"
#define DB_FILE    L"securitydb.db"
#define RAW_FILE   L"securitydb.raw"

struct Index {
    DWORD pXml;
    DWORD cXml;
    DWORD pAsn;
    DWORD cAsn;
    Index() : pXml(0), cXml(0), pAsn(0), cAsn(0) {}
};

class List;

class List {
public :
    List *pNext;
    Index *pData;

public :
    List() : pNext(NULL), pData(NULL) {}

    ~List()
    {
        if (pData)
            delete pData;

        // This could be made iterative..
        if (pNext)
            delete pNext;
    }

    BOOL Add(Index *pd)
    {
        _ASSERTE(pd);

        if (!pData)
        {
            // This is the first node
            pData = pd;
            return TRUE;
        }

        List* lst = new List;
        if (!lst)
            return FALSE;

        lst->pData = pd;
        lst->pNext = pNext;
        pNext = lst;

        return TRUE;
    }
};

class SecurityDB {
private:
    DWORD   nRec;
    Index*  pIndex;
    WORD    nNewRec;
    List*   pNewIndex;
    HANDLE  hDB;
    BOOL    dirty;
    BOOL    error;
    WCHAR   szIndexFile[MAX_PATH + 1];
    WCHAR   szDbFile[MAX_PATH + 1];
    WCHAR   szRawFile[MAX_PATH + 1];

public:
    SecurityDB();
    ~SecurityDB();
    BOOL Convert(BYTE* pXml, DWORD cXml, BYTE** pAsn, DWORD* cAsn);

private:
    void FlushIndex();
    void SwapIndex(Index* pIndex, DWORD nCount) {
#ifdef BIGENDIAN
        DWORD i;
        for (i = 0; i < nCount; i++, pIndex++) {
            pIndex->pXml = VAL32(pIndex->pXml);
            pIndex->cXml = VAL32(pIndex->cXml);
            pIndex->pAsn = VAL32(pIndex->pAsn);
            pIndex->cAsn = VAL32(pIndex->cAsn);            
        }
#endif
    }
    BOOL Add(BYTE* pXml, DWORD cXml);
};

#endif
