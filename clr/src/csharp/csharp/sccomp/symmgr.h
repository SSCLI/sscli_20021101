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
// File: symmgr.h
//
// Defines the symbol manager, which manages the storage and lookup of symbols
// ===========================================================================

#include "symbol.h"
#include "enum.h"

/* A symbol table is a helper class used by the symbol manager. There are
 * two symbol tables; a global and a local.
 */
class SYMTBL {
public:
    SYMTBL(COMPILER * comp, unsigned log2Buckets);
    ~SYMTBL();

    PSYM LookupSym(PNAME name, PPARENTSYM parent, unsigned kindmask);
    PSYM LookupNextSym(PSYM symPrev, PPARENTSYM parent, unsigned kindmask);

    void Clear();
    void Term();

    void InsertChild(PPARENTSYM parent, PSYM child);

private:

    void InsertChildNoGrow(PSYM child);

    void GrowTable();
    unsigned Bucket(PNAME name, PPARENTSYM parent, unsigned * jump);

    PSYM * buckets;        // the buckets of the hash table.
    unsigned cBuckets;          // number of buckets
    unsigned bucketMask;        // mask, always cBuckets - 1.
    unsigned bucketShift;       // log2(cBuckets).
    unsigned cSyms;             // number of syms added.
    COMPILER * compiler;        // Containing compiler.
};


/*
 * The main symbol manager.
 */
class SYMMGR
{
public:
    SYMMGR(NRHEAP * allocGlobal, NRHEAP * allocLocal);
    ~SYMMGR();
    void Init();
    void Term();

    void InitPredefinedTypes();
    COMPILER * compiler();
    void DestroyLocalSymbols();

    /* Core general creation and lookup of symbols. */
    PSYM CreateGlobalSym(SYMKIND symkind, PNAME name, PPARENTSYM parent);
    PSYM CreateLocalSym(SYMKIND symkind, PNAME name, PPARENTSYM parent);
    PSYM LookupGlobalSym(PNAME name, PPARENTSYM parent, unsigned kindmask);
    PSYM LookupLocalSym(PNAME name, PPARENTSYM parent, unsigned kindmask);
    PSYM LookupNextSym(PSYM symPrev, PPARENTSYM parent, unsigned kindmask);

    /* Specific routines for specific symbol types. */
    PNSSYM CreateNamespace(PNAME name, PNSSYM parent);
    PALIASSYM CreateAlias(PNAME name);
    PAGGSYM CreateAggregate(PNAME name, PPARENTSYM parent);
    PNSDECLSYM CreateNamespaceDeclaration(PNSSYM nspace, PNSDECLSYM parent, PINFILESYM inputfile, NAMESPACENODE * parseTree);
    PMEMBVARSYM CreateMembVar(PNAME name, PAGGSYM parent);
    PMETHSYM CreateMethod(PNAME name, PAGGSYM parent);
    PIFACEIMPLMETHSYM CreateIfaceImplMethod(PAGGSYM parent);
    PPROPSYM CreateProperty(PNAME name, PAGGSYM parent);
    PINDEXERSYM CreateIndexer(PNAME name, PAGGSYM parent);
    PEVENTSYM CreateEvent(PNAME name, PAGGSYM parent);
    POUTFILESYM CreateOutFile(LPCWSTR filename, bool isDll, bool isWinApp, LPCWSTR entrySym, LPCWSTR resource, LPCWSTR icon, bool buildIncrementally);
    PRESFILESYM CreateSeperateResFile(LPCWSTR filename, OUTFILESYM *outfileSym, LPCWSTR Ident, bool bVisible);
    PRESFILESYM CreateEmbeddedResFile(LPCWSTR filename, LPCWSTR Ident, bool bVisible);
    PINFILESYM CreateMDFile(LPCWSTR filename, ULONG assemblyIndex, PARENTSYM *parent);
    PINFILESYM CreateSourceFile(LPCWSTR filename, OUTFILESYM *outfile, FILETIME timeLastChange);
    PINFILESYM CreateSynthSourceFile(LPCWSTR filename, OUTFILESYM *outfile);
    PGLOBALATTRSYM CreateGlobalAttribute(PNAME name, NSDECLSYM *parent);

    PARRAYSYM GetArray(PTYPESYM elementType, int rank);
    PPTRSYM GetPtrType(PTYPESYM baseType);
    PPINNEDSYM GetPinnedType(PTYPESYM baseType);
    PPARAMMODSYM GetParamModifier(PTYPESYM baseType, bool isOut);
    PTYPESYM * AllocParams(int cTypes, PTYPESYM * types, mdToken ** ppToken = NULL);
    static unsigned NumberOfParams(PTYPESYM *types)
        { return (types ? ((TYPEARRAY*) (((BYTE*) (types)) - offsetof(TYPEARRAY, types)))->cTypes : 0); }
    /* Get special symbols */
    PVOIDSYM GetVoid()
        { return voidSym; }
    PNULLSYM GetNullType()
        { return nullType; }
    PNSSYM GetRootNS()
        { return rootNS; }
    PSCOPESYM GetFileRoot()
        { return fileroot; }
    POUTFILESYM GetMDFileRoot()
        { return mdfileroot; }
    PPARENTSYM GetXMLFileRoot()
        { return xmlfileroot; }
    PERRORSYM GetErrorSym()
        { return errorSym; }
    PTYPESYM GetArglistSym()
        { return arglistSym; }
    PTYPESYM GetNaturalIntSym()
        { return naturalIntSym; }
    PAGGSYM GetObject()    // return prefined object type
        { return GetPredefType(PT_OBJECT, false); }
    PAGGSYM GetPredefType(PREDEFTYPE pt, bool declared = true);
    CONSTVAL GetPredefZero(PREDEFTYPE pt);
    PREDEFATTRSYM *GetPredefAttr(NAME *name)
        { return LookupGlobalSym(name, attrroot, MASK_ALL)->asPREDEFATTRSYM(); }
    PREDEFATTRSYM *GetPredefAttr(PREDEFATTR pa)
        { return predefinedAttributes[pa]; }

    /* Other helper routines */
    inline void DeclareType(PSYM sym) 
    { 
        if (! sym->isPrepared)
            MakeTypeDeclared(sym);
    }
    BYTE GetElementType(PAGGSYM type);
    static LPWSTR GetNiceName(PAGGSYM type);
    static LPWSTR GetNiceName(PREDEFTYPE pt);
    static int GetAttrArgSize(PREDEFTYPE pt);
    LPWSTR GetFullName(PREDEFTYPE pt);

    bool IsBaseType(PAGGSYM derived, PAGGSYM base);

    void AddToGlobalSymList(PSYM sym, PSYMLIST * * symLink);
    void AddToLocalSymList(PSYM sym, PSYMLIST * * symLink);
    void SetOutFileName(PINFILESYM in);
    void AddToGlobalNameList(PNAME name, PNAMELIST * * nameLink);
    void AddToLocalNameList(PNAME name, PNAMELIST * * nameLink);
    DELETEDTYPESYM* GetDeletedType(ULONG32 signatureSize, PCCOR_SIGNATURE signature);

#ifdef DEBUG
    PINFILESYM GetPredefInfile();

    void DumpSymbol(PSYM sym, int indent = 0);
    void DumpType(PTYPESYM sym);
#endif //DEBUG

#ifdef TRACKMEM
    unsigned GetSymCount(SYMKIND kind, bool global) { return global ? m_iGlobalSymCount[kind] : m_iLocalSymCount[kind]; }
#endif

private:
    // add a symbol in the regular way into a symbol table
    void AddChild(SYMTBL *tabl, PPARENTSYM parent, PSYM child);

    // Identifies an array of types, typically for parameters.
    struct TYPEARRAY {
        unsigned hash;
        TYPEARRAY * next;
        int cTypes;
        PTYPESYM types[];
    };
    typedef TYPEARRAY * PTYPEARRAY;

    NRHEAP *    allocGlobal;
    NRHEAP *    allocLocal;
    SYMTBL      tableGlobal;
    SYMTBL      tableLocal;

    PTYPESYM    arglistSym;
	PTYPESYM	naturalIntSym;
    PERRORSYM   errorSym;
    PVOIDSYM    voidSym;
    PNULLSYM    nullType;
    PNSSYM      rootNS;                 // The "root" (unnamed) namespace.
    PSCOPESYM   fileroot;               // All output and input file symbols rooted here.
    PSCOPESYM   attrroot;               // All predefined attributes are rooted here.
    PSCOPESYM   deletedtypes;           // All deleted types in EnC go here
    PAGGSYM *   predefSyms;             // array of predefined symbol types.
    PAGGSYM *   arrayMethHolder;
    POUTFILESYM mdfileroot;             // The dummy output file for all imported metadata files
    PPARENTSYM  xmlfileroot;            // The dummy output file for all included XML files
    PPREDEFATTRSYM predefinedAttributes[PA_COUNT];


    // These guys manage the hash table for type arrays.
    PTYPEARRAY * buckets;       // the buckets of the type array table.
    unsigned cBuckets;          // number of buckets
    unsigned bucketMask;        // mask, always cBuckets - 1.
    unsigned cTypeArrays;       // number of names added.

    PSYM AllocSym(SYMKIND symkind, PNAME name, NRHEAP * allocator);
    void MakeTypeDeclared(PSYM sym);
    void GrowTypeArrayTable();
    AGGSYM * FindPredefinedType(LPCWSTR typeName, bool isRequired);
    void AddToSymList(NRHEAP *heap, PSYM sym, PSYMLIST * * symLink);
    void AddToNameList(NRHEAP *heap, PNAME name, PNAMELIST * * nameLink);

#ifdef DEBUG
    PINFILESYM  predefInputFile;        // dummy inputfile for testing purposes only

    void DumpChildren(PPARENTSYM sym, int indent);
    void DumpAccess(ACCESS acc);
    void DumpConst(PTYPESYM type, CONSTVAL * constVal);
#endif //DEBUG

#ifdef TRACKMEM
	unsigned m_iGlobalSymCount[SK_COUNT];
    unsigned m_iLocalSymCount[SK_COUNT];
#endif

};

