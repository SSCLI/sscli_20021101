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
// File: common.h
//
// standard header for ALink front end, that has common definitions.
// ===========================================================================

#ifndef __common_h__
#define __common_h__

HANDLE OpenFileEx( LPCWSTR filename, DWORD *fileLen, LPCWSTR relPath = NULL, bool bWrite = false);

/* The tree and list are used for the response file processing to keep track
 * to keep track of added strings for freeing later, and for duplicate detection
 */
template<typename _T> struct list {
    _T arg;
    list<_T> *next;

    list(_T t, list<_T> *n) { arg = t, next = n; }
    list() : arg(), next(NULL) {}
};

template<typename _E> struct tree {
    _E name;
    tree<_E> *lChild;
    tree<_E> *rChild;
    size_t  lDepth;
    size_t  rDepth;

    tree(_E e) { name = e; lChild = rChild = NULL; lDepth = rDepth = 0; }
    ~tree() { Cleanup(); }

    bool InOrderWalk( bool (WalkFunc)(_E)) {
        if (lChild != NULL && !lChild->InOrderWalk(WalkFunc))
            return false;
        if (!WalkFunc(name))
            return false;
        if (rChild != NULL)
            return rChild->InOrderWalk(WalkFunc);
        return true;
    }

    /*
     * return the depths of the tree from here down (minimum of 1)
     */
    size_t MaxDepth() {
        return lDepth > rDepth ? lDepth + 1 : rDepth + 1;
    }

    /*
     * Search the binary tree for the given string
     * return a pointer to it was added or NULL if it
     * doesn't exists
     */
    _E * Find(_E SearchVal, int (__cdecl CompFunc)(_E, _E))
    {
        ASSERT(this != NULL);

        int cmp = CompFunc(name, SearchVal);
        if (cmp < 0) {
            if (lChild == NULL)
                return NULL;
            else
                return lChild->Find(SearchVal, CompFunc);
        } else if (cmp > 0) {
            if (rChild == NULL)
                return NULL;
            else
                return rChild->Find(SearchVal, CompFunc);
        } else
            return &name;
    }

    /*
     * Search the binary tree and add the given string
     * return true if it was added or false if it already
     * exists
     */
    bool Add(_E add, int (__cdecl CompFunc)(_E, _E))
    {
        ASSERT(this != NULL);

        int cmp = CompFunc(name, add);
REDO:
        if (cmp < 0) {
            if (lChild == NULL)
            {
                lDepth = 1;
                return ((lChild = new tree<_E>(add)) != NULL);
            }
            else if (rDepth < lDepth)
            {
                tree<_E> *temp = new tree<_E>(name);
                temp->rChild = rChild;
                temp->rDepth = rDepth;
                if (lChild != NULL &&
                    (cmp = CompFunc(lChild->name, add)) > 0) {
                    // push right
                    temp->lChild = NULL;
                    temp->lDepth = 0;
                    name = add;
                    rChild = temp;
                    rDepth++;
                    return true;
                } else if (cmp == 0) {
                    temp->rChild = NULL;
                    VSFree(temp);
                    return false;
                } else {
                    // Rotate right
                    temp->lChild = lChild->rChild;
                    temp->lDepth = lChild->rDepth;
                    name = lChild->name;
                    lDepth = lChild->lDepth;
                    rDepth = temp->MaxDepth();
                    rChild = temp;
                    temp = lChild->lChild;
                    lChild->lChild = lChild->rChild = NULL;
                    VSFree(lChild);
                    lChild = temp;
                    goto REDO;
                }
            }
            else
            {
                bool temp = lChild->Add(add, CompFunc);
                lDepth = lChild->MaxDepth();
                return temp;
            }
        } else if (cmp > 0) {
            if (rChild == NULL)
            {
                rDepth = 1;
                return ((rChild = new tree<_E>(add)) != NULL);
            }
            else if (lDepth < rDepth)
            {
                tree<_E> *temp = new tree<_E>(name);
                temp->lChild = lChild;
                temp->lDepth = lDepth;
                if (rChild != NULL &&
                    (cmp = CompFunc(rChild->name, add)) < 0) {
                    // push left
                    temp->rChild = NULL;
                    temp->rDepth = 0;
                    name = add;
                    lChild = temp;
                    lDepth++;
                    return true;
                } else if (cmp == 0) {
                    temp->lChild = NULL;
                    VSFree(temp);
                    return false;
                } else {
                    // Rotate left
                    temp->rChild = rChild->lChild;
                    temp->rDepth = rChild->lDepth;
                    name = rChild->name;
                    rDepth = rChild->rDepth;
                    lDepth = temp->MaxDepth();
                    lChild = temp;
                    temp = rChild->rChild;
                    rChild->rChild = rChild->lChild = NULL;
                    VSFree(rChild);
                    rChild = temp;
                    goto REDO;
                }
            }
            else
            {
                bool temp = rChild->Add(add, CompFunc);
                rDepth = rChild->MaxDepth();
                return temp;
            }
        } else
            return false;
    }

    /*
     * Free the memory allocated by the tree (recursive)
     */
    void Cleanup()
    {
        if (this == NULL)
            return;
        if (lChild != NULL) {
            lChild->Cleanup();
            VSFree(lChild);
            lChild = NULL;
        }
        if (rChild != NULL) {
            rChild->Cleanup();
            VSFree(rChild);
            rChild = NULL;

        }
    }

};

#endif //__common_h__

