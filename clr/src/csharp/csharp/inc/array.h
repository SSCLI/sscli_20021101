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
// File: array.h
//
// ===========================================================================

#ifndef _ARRAY_H_
#define _ARRAY_H_

////////////////////////////////////////////////////////////////////////////////
// CSmallArray
//
// Simple array that holds pointers to T's.  Grows in blocks of 8 pointers
// (32 bytes).  Removal causes re-arrangement (the last element is moved to take
// the removed one's place).

template <class T>
class CSmallArray
{
private:


    T       **m_pElements;      // Elements of the array
    long    m_iCount;           // Number of elements in array

public:
    CSmallArray() : m_pElements(NULL), m_iCount(0) {}
    ~CSmallArray() { if (m_pElements != NULL) VSFree (m_pElements); }

    HRESULT     Add (T *p)
    {
        if ((m_iCount & 7) == 0)
        {
            // Time to grow
            long    iSize = (m_iCount + 8) * sizeof (T *);
            void    *pNew = (m_pElements == NULL) ? VSAlloc (iSize) : VSRealloc (m_pElements, iSize);

            if (pNew != NULL)
                m_pElements = (T **)pNew;
            else
                return E_OUTOFMEMORY;
        }

        m_pElements[m_iCount++] = p;
        return S_OK;
    }

    HRESULT     Remove (T *p)
    {
        for (long i=0; i<m_iCount; i++)
        {
            if (m_pElements[i] == p)
            {
                if (i < m_iCount - 1)
                    m_pElements[i] = m_pElements[m_iCount - 1];
                m_iCount--;
                return S_OK;
            }
        }

        return E_INVALIDARG;
    }

    long        Count() { return m_iCount; }
    T           *GetAt (long i) { ASSERT (i >= 0 && i < m_iCount); return m_pElements[i]; }
    T           * operator[](int i) const { ASSERT (i >= 0 && i < m_iCount); return m_pElements[i]; }
    T           **Base () { return m_pElements; }
    void        ClearAll () { m_iCount = 0; }
    void        FreeAll () { m_iCount = 0; if (m_pElements != NULL) { VSFree (m_pElements); m_pElements = NULL; } }
};

////////////////////////////////////////////////////////////////////////////////
// CStructArray
//
// Simple array that holds T's.  Grows in blocks of 8 T's at a time.  Like
// CSmallArray, removal causes re-arrangement (the last element is moved to take
// the removed one's place).
//
// NOTE:  T must not require ctor/dtor calls.  They will be zero-inited on
// initial creation.  They must have copy capability, however.  (This is intended
// for simple structs, such as RECT, TextSpan, etc.)

template <class T>
class CStructArray
{
private:
    T       *m_pElements;       // Elements of the array
    long    m_iCount;           // Number of elements in array

public:
    CStructArray() : m_pElements(NULL), m_iCount(0) {}
    ~CStructArray() { if (m_pElements != NULL) VSFree (m_pElements); }

    HRESULT     Add (long *piIndex, T **pp)        // (returns the index, and a pointer to the new T -- both optional)
    {
        if ((m_iCount & 7) == 0)
        {
            // Time to grow
            long    iSize = (m_iCount + 8) * sizeof (T);
            void    *pNew = (m_pElements == NULL) ? VSAlloc (iSize) : VSRealloc (m_pElements, iSize);

            if (pNew != NULL)
                m_pElements = (T *)pNew;
            else
                return E_OUTOFMEMORY;
        }

        ZeroMemory (m_pElements + m_iCount, sizeof (T));

        if (pp != NULL)
            *pp = m_pElements + m_iCount;

        if (piIndex != NULL)
            *piIndex = m_iCount;

        m_iCount++;
        return S_OK;
    }

    HRESULT     Add (const T &t)
    {
        T       *pNew;
        HRESULT hr = Add (NULL, &pNew);
        if (SUCCEEDED (hr))
            *pNew = t;
        return hr;
    }

    HRESULT     Remove (long i)
    {
        if (i >= 0 && i < m_iCount)
        {
            if (i < m_iCount - 1)
                m_pElements[i] = m_pElements[m_iCount - 1];
            m_iCount--;
            return S_OK;
        }

        return E_INVALIDARG;
    }

    long        Count() { return m_iCount; }
    T           *GetAt (long i) { ASSERT (i >= 0 && i < m_iCount); return m_pElements + i; }
    T           *Base () { return m_pElements; }
    T           & operator[](long i) const { ASSERT (i >= 0 && i < m_iCount); return m_pElements[i]; }
    void        ClearAll () { m_iCount = 0; }
};

////////////////////////////////////////////////////////////////////////////////
// CRefArray

template <class T>
class CRefArray : public CStructArray<T>
{
private:
    long    m_iRef;

public:
    CRefArray() : m_iRef(0) {}

    long    AddRef () { return InterlockedIncrement (&m_iRef); }
    long    Release () { long l = InterlockedDecrement (&m_iRef); if (l == 0) delete this; return l; }
};

#endif  // _ARRAY_H_
