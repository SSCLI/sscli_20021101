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
#ifndef _SYNCCLEAN_HPP_
#define _SYNCCLEAN_HPP_

// We keep a list of memory blocks to be freed at the end of GC, but before we resume EE.
// To make this work, we need to make sure that these data are accessed in cooperative GC
// mode.

class Bucket;
struct EEHashEntry;
class Crst;

class SyncClean {
public:
    static HRESULT Init (BOOL fFailFast=TRUE);
    static void Terminate ();

    static void AddHashMap (Bucket *bucket);
    static void AddEEHashTable (EEHashEntry** entry);
    static void CleanUp ();

private:
    static Crst *m_Crst;                 // Lock for adding to our cleanup list.
    static Bucket* m_HashMap;            // Cleanup list for HashMap
    static EEHashEntry** m_EEHashTable;  // Cleanup list for EEHashTable
};
#endif
