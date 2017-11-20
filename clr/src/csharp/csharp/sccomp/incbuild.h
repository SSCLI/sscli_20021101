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
// File: incbuild.h
//
// Routines for handling incremental builds.
// ===========================================================================

/*
 * INCREMENTALTYPE
 *
 * per-type information stored in the incremental build file
 */
class INCREMENTALTYPE
{
public:
    mdTypeDef   token;
    BITSET *    dependantFiles; // set of files that depend on this type
    AGGSYM *    cls;
};

struct INCBUILDREADTALLY;

/*
 * INCBUILD class.
 *
 * One instance of this class exists for each OUTFILE that incremental
 * build is enabled for. The OUTFILE symbol points to the instance of
 * this class.
 */
class INCBUILD
{
private:
    COMPILER * compiler;        // Points to compiler instance for this build.
    OUTFILESYM * outfile;       // Associated output file.
    
    bool GetIncbuildFilename(LPWSTR filename, int cchMax);
    void WriteString(FILE * file, LPCWSTR s);
    bool ReadString(FILE * file, LPWSTR s, unsigned cbMax);
    void WriteOutFileInfo(FILE *file, OUTFILESYM *outfile);
    bool ReadOutFileInfo(FILE *file, INCBUILDREADTALLY * tally);
    void ReadIncrementalInfo(INCBUILDREADTALLY *tally);  

public:
    // Data Members
    mdToken entryClassToken;    // token for class containing entry point method
    ULONG cBuildCount;          // Number of incremental rebuilds since a full build

    // Allocate from a no-release allocator.
    void * operator new(size_t sz, NRHEAP * allocator)
    {
        return allocator->Alloc(sz);
    }

    INCBUILD(COMPILER * compiler, OUTFILESYM * outfile);

    static bool ReadIncrementalInfo(COMPILER *compiler, bool *canIncrementallyRebuild);
    void WriteIncrementalInfo(BOOL firstOutfile); 

    static FILETIME GetFileChangeTime(LPCWSTR filename);
};

struct RVAALLOCATION
{
    ULONG offset;
    ULONG size;

    ULONG endOffset() const   { return offset + size; }
};

class ALLOCMAP
{
private:
    NRHEAP *    heap;
    ULONG       nAllocated;
    ULONG       nUsed;
    RVAALLOCATION  allocs[0];

public:

    static const ULONG INVALID_OFFSET = 0xFFFFFFFF;

    static ALLOCMAP *Create(NRHEAP *heap);
    static ALLOCMAP *AddAlloc(ALLOCMAP *map, ULONG offset, ULONG size);
    static ALLOCMAP *Allocate(ALLOCMAP *map, ULONG size, ULONG alignment, ULONG *offset, ULONG *realSize, ULONG *realOffset);
    static ALLOCMAP *Remove(ALLOCMAP *removeFrom, ALLOCMAP *toRemove);
    static ALLOCMAP *Read(NRHEAP *heap, FILE *file);
    void Write(FILE *file);

    void Clear();
    ULONG MaxUsedOffset();

private:

    static ULONG SizeInBytes(ULONG numberOfAllocs);

    static ALLOCMAP *IncrementUsedAllocs(ALLOCMAP *map);
    static ALLOCMAP *AddAllocIndex(ALLOCMAP *map, ULONG offset, ULONG size, ULONG index);
    static ALLOCMAP *Remove(ALLOCMAP *removeFrom, ULONG offset, ULONG size);
    
    static ULONG PadSize(ULONG offset, ULONG alignment);

    ULONG FindIndex(ULONG offset);

};
