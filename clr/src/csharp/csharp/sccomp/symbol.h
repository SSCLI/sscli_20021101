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
// File: symbol.h
//
// Defines the shapes of various symbols in the symbol table.
// ===========================================================================

#ifndef __symbol_h__
#define __symbol_h__

#include "constval.h"

// Number of simple types in predeftype.h. You must
// change appropriate conversion table definitions 
// if this changes.
#define NUM_SIMPLE_TYPES 13   
#define NUM_QSIMPLE_TYPES 4

class ALLOCMAP;

/*
 * Fundemental types. These are the fundemental storage types
 * that are available and used by the code generator.
 */
enum FUNDTYPE {
    FT_NONE,        // No fundemental type
    FT_I1,
    FT_I2,
    FT_I4,
    FT_U1,
    FT_U2,
    FT_U4,
    FT_LASTNONLONG = FT_U4,  // Last one that fits in a int.
    FT_I8,
    FT_U8,          // integral types
    FT_LASTINTEGRAL = FT_U8,
    FT_R4,
    FT_R8,          // floating types
    FT_REF,         // reference type
    FT_STRUCT,      // structure type
    FT_PTR,         // pointer to unmanaged memory

    FT_COUNT        // number of enumerators.
};

/*
 * Define the different access levels that symbols can have.
 */
enum ACCESS {
    ACC_UNKNOWN,    // Not yet determined.
    ACC_PRIVATE,
    ACC_INTERNAL,
    ACC_PROTECTED,
    ACC_INTERNALPROTECTED,  // internal OR protected
    ACC_PUBLIC
};

/*
 * Define the different symbol kinds. Only non-abstract symbol kinds are here.
 */

#define SYMBOLDEF(kind) SK_ ## kind,
enum SYMKIND {
    #include "symkinds.h"
    SK_COUNT
};
#undef SYMBOLDEF

/*
 * Forward declare all symbol types.
 */
#define SYMBOLDEF(kind) typedef class kind * P ## kind;
#include "symkinds.h"
#undef SYMBOLDEF

/*
 * Define values for symbol masks.
 */
#define SYMBOLDEF(kind) MASK_ ## kind = (1 << SK_ ## kind),
enum {
    #include "symkinds.h"
    MASK_ALL = ~(MASK_FAKEMETHSYM | MASK_FAKEPROPSYM | MASK_EXPANDEDPARAMSSYM),

};
#undef SYMBOLDEF

// Typedefs for some pointer types up front.
typedef class SYM * PSYM;
typedef class PARENTSYM * PPARENTSYM;
typedef class TYPESYM * PTYPESYM;

struct METHODNODE;
struct BASENODE;
class  CSourceData;


/*
 *  SYMLIST - a list of symbols.
 */
struct SYMLIST {
    PSYM sym;
    SYMLIST * next;

    bool contains(PSYM sym);
};
typedef SYMLIST * PSYMLIST;

#define FOREACHSYMLIST(_list, _elem)    \
{                                       \
    for (SYMLIST * _next = (_list);     \
         _next;                         \
         _next = _next->next) {         \
        PSYM _elem;                     \
        _elem = _next->sym;             \

#define ENDFOREACHSYMLIST               \
    }                                   \
}

/*
 *  NAMELIST - a list of symbols.
 */
struct NAMELIST {
    PNAME name;
    NAMELIST * next;

    bool contains(PNAME name);
};
typedef NAMELIST * PNAMELIST;

#define FOREACHNAMELIST(_list, _elem)    \
{                                       \
    for (NAMELIST * _next = (_list);     \
         _next;                         \
         _next = _next->next) {         \
        PNAME _elem = _next->name;

#define ENDFOREACHNAMELIST               \
    }                                   \
}

/*
 * SYM - the base symbol.
 */
class SYM {
public:
    SYMKIND kind: 8;     // the symbol kind
    BYTE isLocal: 1;     // a local or global symbol?
    bool isPrepared: 1;  // If this is FALSE, only the following info is known for sure:
                         //    name, parent namespace, inputfile, class/struct/interface/enum.
                         // If this is TRUE, everything is known, including children.
    ACCESS access: 4;    // access level
    bool isDeprecated: 1;// symbol is deprecated and should give a warning when used
    bool isDeprecatedError: 1; // If isDeprecated, indicates error rather than warning

    bool hasCLSattribute: 1; // True iff this symbol has an attribute
    bool isCLS: 1;           // if hasCLSattribute, indicates whether symbol is CLS compliant
    bool isBogus: 1;     // can't be used in our language -- unsupported type(s)
    bool checkedBogus: 1;// Have we checked a method args/return for bogus types

    WCHAR * deprecatedMessage;  // If isDeprecated, has message to show.

    PNAME name;             // name of the symbol
    PPARENTSYM parent;      // parent of the symbol
    PSYM nextChild;    // next child of this parent
    PSYM nextSameName; // next child of this parent with same name.

    // We have member functions here to do casts that, in DEBUG, check the
    // symbol kind to make sure it is right. For example, the casting method
    // for METHODSYM is called "asMETHODSYM".

    // Define all the concrete casting methods.
    // We define them explicitly so that VC's ide knows about them

    class NSSYM                     * asNSSYM();
    class NSDECLSYM                 * asNSDECLSYM();
    class AGGSYM                    * asAGGSYM();
    class INFILESYM                 * asINFILESYM();
    class RESFILESYM                * asRESFILESYM();
    class OUTFILESYM                * asOUTFILESYM();
    class MEMBVARSYM                * asMEMBVARSYM();
    class LOCVARSYM                 * asLOCVARSYM();
    class METHSYM                   * asMETHSYM();
    class FAKEMETHSYM               * asFAKEMETHSYM();
    class PROPSYM                   * asPROPSYM();
    class FAKEPROPSYM               * asFAKEPROPSYM();
    class METHPROPSYM               * asMETHPROPSYM();
    class SCOPESYM                  * asSCOPESYM();
    class ARRAYSYM                  * asARRAYSYM();
    class PTRSYM                    * asPTRSYM();
    class PINNEDSYM                 * asPINNEDSYM();
    class PARAMMODSYM               * asPARAMMODSYM();
    class VOIDSYM                   * asVOIDSYM();
    class NULLSYM                   * asNULLSYM();
    class CACHESYM                  * asCACHESYM();
    class LABELSYM                  * asLABELSYM();
    class ERRORSYM                  * asERRORSYM();
    class ALIASSYM                  * asALIASSYM();
    class EXPANDEDPARAMSSYM         * asEXPANDEDPARAMSSYM();
    class INDEXERSYM                * asINDEXERSYM();
    class PREDEFATTRSYM             * asPREDEFATTRSYM();
    class GLOBALATTRSYM             * asGLOBALATTRSYM();
    class EVENTSYM                  * asEVENTSYM();
    class IFACEIMPLMETHSYM          * asIFACEIMPLMETHSYM();
    class XMLFILESYM                * asXMLFILESYM();
    class DELETEDTYPESYM            * asDELETEDTYPESYM();
    class SYNTHINFILESYM            * asSYNTHINFILESYM();

    // Define the ones for the abstract classes.
    class PARENTSYM                 * asPARENTSYM();
    class TYPESYM                   * asTYPESYM();
    class VARSYM                    * asVARSYM();

    // Define the ones which traverse subclass relationships:
    class METHSYM                   * asFMETHSYM();
    class PROPSYM                   * asFPROPSYM();
    class INFILESYM                 * asANYINFILESYM();

    // Allocate zeroed from a no-release allocator.
    #undef new
    void * operator new(size_t sz, NRHEAP * allocator)
    {
        return allocator->AllocZero(sz);
    }

    int mask() { return 1 << kind; };

    void copyInto(SYM * sym) {
        sym->access = access;
    }

    bool isStructType();
    bool hasExternalAccess();
    bool checkForCLS();         // true iff !this->isNonCLS and parent->checkForCLS()

    // returns parse tree for classes, and class members
    BASENODE *      getParseTree();
    INFILESYM *     getInputFile();
    DWORD           getImportScope() const;
    SYM *           firstDeclaration();
    ULONG           getAssemblyIndex() const;
    mdToken *       getTokenEmitPosition();
    mdToken         getTokenEmit();
    bool            isUserCallable();
    CorAttributeTargets getElementKind();
    PPARENTSYM      containingDeclaration();
    BASENODE *      getAttributesNode();
    bool            isContainedInDeprecated() const;
    bool            isVirtualSym() ;

#if DEBUG
    virtual void zDummy() {};
#endif
};


/*
 * CACHESYM - a symbol which wraps other symbols so that
 * they can be cached in the local scope by name
 * LOCVARSYMs are never cached in the introducing scope
 */
class CACHESYM: public SYM {
public:
    PSYM sym; // The symbol this cache entry points to
    PSCOPESYM scope;  // The scope in which this name is bound to that symbol
};

/*
 * LABELSYM - a symbol representing a label.  
 */
class LABELSYM: public SYM {
public:
    class EXPRLABEL * labelExpr; // The corespoding label statement
    bool reached:1; // has this label been reached by a reachable goto, or by falltrough?
    bool targeted:1; // has this label been targeted by a goto? (ie, is it referenced?)
};


/*
 * PARENTSYM - a symbol that can contain other symbols as children.
 */

class PARENTSYM: public SYM {
public:
    PSYM firstChild;    // list of all children of this symbol
    PSYM lastChild;     // last member of this list

    PPARENTSYM  getScope();     // gets the scope symbol corresponding to this symbol
    PPARENTSYM  containingDeclaration();
};

#define FOREACHCHILD(_parent, _child)           \
{                                               \
    for (SYM * _child = (_parent)->firstChild;  \
         _child;                                \
         _child = _child->nextChild) {          \

#define ENDFOREACHCHILD                         \
    }                                           \
}



/*
 * TYPESYM - a symbol that can be a type. Our handling of derived types
 * (like arrays and pointers) requires that all types extend PARENTSYM.
 */

class TYPESYM: public PARENTSYM {
public:
    // Get the fundemental type of the symbol.
    FUNDTYPE fundType();
    TYPESYM * underlyingType();
    AGGSYM * underlyingAggregate();
    unsigned short getFieldsSize();
    bool     isSimpleType();
    bool     isQSimpleType();
    bool     isNumericType();
    bool     isCLS_Type();
    bool     isAnyStructType(); // struct or simple
    bool     isUnsigned();
    bool     isUnsafe();
    bool     isPredefType(PREDEFTYPE pt);
    bool     isSpecialByRefType();
    static AGGSYM * commonBase(TYPESYM * type1, TYPESYM * type2);
};


/*
 * OUTFILESYM -- a symbol that represents an output file we are creating.
 * Its children all all input files that contribute. The symbol name is the
 * file name.
 */
class OUTFILESYM: public PARENTSYM {
public:
    bool    isDll: 1;               // A dll or an exe?
    bool    isResource: 1;          // Is this a resource file that we linked?
    bool    isConsoleApp: 1;        // A console application?
    bool    buildIncrementally: 1;  // Build incrementally?
    bool    multiEntryReported: 1;  // Has the 'Multiple Entry Points' error been reported for this file?
    bool    makeResFile: 1;         // True if we autogenerate .RES file.  False if we use resourceFile
    bool    hasDefaultName: 1;

    class INCBUILD * incbuild;      // If buildIncrementally==true, the INCBUILD class to manage it.
    
    ULONG   imageBase;              // Image base (or 0 for default)
    ULONG   fileAlign;              // File Alignment (or 0 for default)
    union {
        LPWSTR  resourceFile;           // resource file name. (valid iff makeResFile == false)
        LPWSTR  iconFile;               // icon file name.     (valid iff makeResFile == true)
    };
    AGGSYM *globalClass;            // global class of which we hang native methodrefs
    LPWSTR  entryClassName;         // User specified entryPoint Fully-Qualified Class name
    PMETHSYM entrySym;              // 'Main' method symbol (for EXEs only)
    mdFile  idFile;                 // MetaData token for the file
    mdModuleRef idModRef;           // Used for scoped TypeRefs
    PGLOBALATTRSYM attributes;      // Attributes for this module
    ULONG   cInputFiles;            // nummber of input files
    ULONG   cTypes;                 // number of types defined in this file
    ULONG   cOldTypes;              // number of types defined in this file in previous incremental build
    mdToken globalTypeToken;        // type token for arrray init constants and switch on string hash tables

    IMetaDataImport * metaimport;   // meta-data importation interface for incremental build
    ALLOCMAP *allocMapAll;          // map of all RVA's used in this output file
    ALLOCMAP *allocMapNoSource;     // map of RVA's used by this output file which don't come from source
                                    // this includes: CryptoKey, Debug Directory, Resources

    PINFILESYM  firstInfile() {
        PSYM sym;
        for( sym = firstChild; sym && sym->kind != SK_INFILESYM; sym = sym->nextChild);
        return sym ? sym->asINFILESYM() : NULL; }
    PRESFILESYM firstResfile() {
        PSYM sym;
        for( sym = firstChild; sym && sym->kind != SK_RESFILESYM; sym = sym->nextChild);
        return sym ? sym->asRESFILESYM() : NULL; }
    POUTFILESYM nextOutfile() { return nextChild->asOUTFILESYM(); }
    bool isUnnamed() { return !wcscmp(name->text, L"?"); }
};


/*
 * RESFILESYM - a symbol that represents a resource input file.
 * Its parent is the output file it contributes to, or Default
 * if it will be embeded in the Assembly file.
 * The symbol name is the resource Identifier.
 */
class RESFILESYM: public SYM {
public:

    LPWSTR  filename;
    bool    isVis;
    bool    isEmbedded;

    PRESFILESYM nextResfile() {
        PSYM sym;
        for( sym = nextChild; sym && sym->kind != SK_RESFILESYM; sym = sym->nextChild);
        return sym ? sym->asRESFILESYM() : NULL; }
    POUTFILESYM getOutputFile() { return parent->asOUTFILESYM(); }
};

class INCREMENTALTYPE;

/*
 * INFILESYM - a symbol that represents an input file, either source
 * code or meta-data, of a file we may read. Its parent is the output
 * file it contributes to. The symbol name is the file name.
 * It is a PARENTSYM because assembly imports can have several 'sub-files'
 */
class INFILESYM: public PARENTSYM {
public:
    bool   isSource: 1;         // If true, source code, if false, metadata
    bool   isAddedModule: 1;
    bool   hasBeenRefed: 1;     // If true, then a type in this md file has been referenced
    bool   canRefThisFile: 1;   // Only true for imported assemblies (except mscorlib.dll)
    bool   hasChanged: 1;       // This file has changed since the last incremental build
    bool   hasGlobalAttr: 1;
    bool   isDefined: 1;        // have symbols for this file been defined
    bool   isConstsEvaled: 1;   // have compile time constants been evaluated for this file
    bool   isBCL: 1;            // is this the infilesym for mscorlib.dll

    ULONG  idIncbuild;          // Incremental build assigns each input file a unique id.
    ULONG  assemblyIndex;       // index of the assembly(used to determine 
                                // when types are in the same assembly.
                                // 0 - the assembly being built
    class NSDECLSYM *rootDeclaration;    // the top level declaration for this file

    FILETIME timeLastChange;    // Last change date for the input file. All zero if unavailable.
    ALLOCMAP *allocMap;         // map of RVA's used by this input file

    int    * xmlComments;
    int    cXmlCommentsAlloced;
    int    cXmlComments;        // list of comment indices of each processed XML comment in this source file.
    

    // If metadata, then the following are available.
    DWORD   dwScopes;
    mdToken mdImpFile;
    IMetaDataImport ** metaimport;  // meta-data importation interfaces.
    IMetaDataAssemblyImport *assemimport; // assembly meta-data import interface.
    NAME *  assemblyName;       // text version of assembly for attributes

    mdAssemblyRef     idLocalAssembly;   // Assembly id for use in scoped TypeRefs.

    // If a source file, then the following are available.
    struct NAMESPACENODE * nspace;  // The top level namespace associated w/ the file
    CSourceData * pData; // Associated source module data
    ISymUnmanagedDocumentWriter * documentWriter;
    ULONG                   cTypes;             // number of types defined in this file
    ULONG                   cTypesPrevious;     // number of types defined in previous incremental build
    INCREMENTALTYPE *       typesPrevious;      // array of types defined in previous incremental build
    INFILESYM *             nextChangedFile;    // list of infiles that need constants evaled
    
    struct TOKENLIST {
        TOKENLIST * pNext;
        mdToken     pTokens[8];     // Keep this a power of 2
    }                     * globalFields;       // Array/list of global fields in <PID> used in the compilation of this file
    ULONG                   cGlobalFields;      // number of global fields in <PID> used in the compilation of this file

    PINFILESYM nextInfile() {
        PSYM sym;
        for( sym = nextChild; sym && sym->kind != SK_INFILESYM; sym = sym->nextChild);
        return sym ? sym->asINFILESYM() : NULL; }
    POUTFILESYM getOutputFile() { return ((parent->kind == SK_OUTFILESYM) ? parent->asOUTFILESYM() : parent->asINFILESYM()->getOutputFile()); }
    bool isSymbolDefined(NAME *symbol);
};

/* 
 * Same as above. Only used for #line directives
 */
class SYNTHINFILESYM : public INFILESYM {
};

/*
 * Namespaces, Namespace Declarations, and their members.
 *
 *
 * The parent, child, nextChild relationships are overloaded for namespaces.
 * The cause of all of this is that a namespace can be declared in multiple
 * places. This would not be a problem except that the using clauses(which
 * effect symbol lookup) are related to the namespace declaration not the
 * namespace itself. The result is that each namespace needs lists of all of
 * its declarations, and its members. Each namespace declaration needs a list
 * the declarations and types declared within it. Each member of a namespace
 * needs to access both the namespace it is contained in and the namespace
 * declaration it is contained in.
 */

/*
 * NSSYM - a symbol representing a name space. 
 *
 * firstChild/lastChild/firstChild->nextDeclaration enumerates
 * the declarations of this namespace.
 *
 * parent is the containing namespace.
 */
class NSSYM: public PARENTSYM {
public:
    bool    hasPredefinedClasses : 1;   // Does this package contained predefined classes?
    bool    checkedForCLS        : 1;   // Have we already checked children for CLS name clashes?
    bool    checkingForCLS       : 1;   // Have we added this NSSYM to it's parent's list?
    bool    isDefinedInSource    : 1;   // Is this namespace (or rather one of it's NSDECLS) defined in source?

    NSSYM * getParentNS() { return parent->asNSSYM(); }

    // list of declarations of this namespace in source files
    class NSDECLSYM * firstDeclaration()  { return firstChild->asNSDECLSYM(); }
    class NSDECLSYM * lastDeclaration()   { return lastChild->asNSDECLSYM(); }
};

/*
 * NSDECLSYM - a symbol representing a declaration
 * of a namspace in the source. 
 *
 * firstChild/lastChild/firstChild->nextChild enumerates the 
 * namespace declarations and types declared within this declaration.
 *
 * parent is the containing namespace declaration.
 *
 * namespaceSymbol is the namespace corresponding to this declaration.
 *
 * nextChild is the next declaration for the same namespace.
 */
class NSDECLSYM : public PARENTSYM {
public:

    NSSYM *                 namespaceSymbol;
    INFILESYM *             inputfile;
    NAMESPACENODE *         parseTree;
    SYMLIST *               usingClauses;
    NSDECLSYM *             nextDeclaration;

    bool                    usingClausesResolved:1; // true if the using clauses for this
                                                    // declaration have been resolved
                                                    // this can occur between declare and
                                                    // define steps in compilation

    // accessors
    bool                    isRootDeclaration()     { return !parent; }
    NSDECLSYM *             containingDeclaration() { return parent->asNSDECLSYM(); }
    PNSSYM                  getScope()              { return namespaceSymbol; }
    ULONG                   getAssemblyIndex() const { return inputfile->assemblyIndex; }
    bool                    isDottedDeclaration();  // true for a&b in namespace a.b.c {}
};

/*
 * PREDEFATTR - enum of predefined attributes
 */
enum PREDEFATTR
{

#define PREDEFATTRDEF(id,name, iPredef, validOn) id,
#include "predefattr.h"
#undef PREDEFATTRDEF

    PA_COUNT
};

/*
 * AGGSYM - a symbol representing an aggregate type. These are classes,
 * interfaces, and structs. Parent is a namespace or class. Children are methods,
 * properties, and member variables, and types.
 *
 * If this type is contained in a namespace then it is linked in
 * a little differently:
 *
 * parent is the containing namespace.
 * 
 * nextChild is the next declaration within this namespace 
 * declaration only.
 *
 * declaration is the containing namespace declaration.
 */
class AGGSYM: public TYPESYM {
public:
    // These four are mutually exclusive.
    bool    isClass: 1;         // Is it a class?
    bool    isStruct: 1;        // Is it a struct?
    bool    isInterface: 1;     // Is it an interface?
    bool    isEnum: 1;          // Is it an enum?

    bool    isDelegate :1;      // Is it a deleagete? If so, isClass is always set also.

    bool    isAbstract: 1;      // Can it be instantiated?
    bool    isSealed: 1;        // Can it be derived from?

    bool    isMarshalByRef: 1;  // App-domain bound or context bound?

    bool    isResolvingBaseClasses:1;   // is the base class being resolved right now?
    bool    hasResolvedBaseClasses:1;   // the base class has been resolved
    bool    isResolvingLayout:1;    // is the layout of this struct being resolved?
    bool    isLayoutResolved:1;     // is the layout of this struct resolved
    bool    isDefined:1;        // have members of this type been defined (does NOT ensure base types have been defined)

    bool    isTypeDefEmitted: 1; // has type defs been emitted?
    bool    isMemberDefsEmitted: 1; // have all member defs been emitted?
    bool    isCompiled:1;       // have members been compiled?

    bool    isPredefined: 1;    // A special predefined type.
    bool    isEmptyStruct: 1;   // A struct w. no fields
    unsigned iPredef: 7;        // index of the predefined type, if isPredefined.
    bool    hasConversion: 1;   // set if this type or any base type has user defined conversion operators
    bool    isMultipleAttribute: 1; // set if this class is an attribute class 
                                    // which can be applied multiple times to a single symbol
    bool    hasZeroParamConstructor:1; // Valid only for structs...
    bool    isSecurityAttribute: 1; // is this a class which derives from System.Security.CodeAccessPermission
    bool    isAttribute: 1;         // is this a class which derives from System.Attribute

    bool    isPrecluded: 1;         // is this class precluded from being considered the source for methods
                                    // used only on interfaces...
    bool    hasCandidateMethod: 1;  // this class has been looked at already, so don't consider it again

    bool    hasMultipleDefs: 1;     // this symbols is defined in multiple public places

    bool    hasExplicitImpl: 1;     // class has explicit impls (used only on structs)
    bool    hasStaticCtor: 1;       // Class has a user-defined static constructor
    bool    hasExternReference: 1;  // private struct members hsould not be checked for assignment or refencees

    bool    hasUDselfEQ: 1; // has operator == defined on itself
    bool    hasUDselfNE: 1; // has operator != defined on itself
    bool    hasUDselfEQRes: 1; // above bit is valid
    bool    hasUDselfNERes: 1; // above bit is valid
    bool    isPreparing: 1;
    bool    isComImport: 1;     // Does it have [ComImport]

    AGGSYM * baseClass;         // For a class/struct/enum, the base class. For iface: unused.
    AGGSYM * underlyingType;    // For enum, the underlying type. For iface, the resolved CoClass. Not used for class/struct.
    PSYMLIST ifaceList;         // List of explicit base interfaces for a class or interface.
    PSYMLIST allIfaceList;      // Recursive closure of base interfaces for an interface.

    PPARENTSYM declaration;     // enclosing declaration for this type

    mdToken tokenImport;        // Meta-data token for imported class.
    DWORD   importScope;        // Meta-data import scope #

    mdExportedType tokenComType;	// The ComType token used for nested classes
    mdToken tokenEmit;          // Meta-data token (typeRef or typeDef) in the current output file.
    mdToken tokenEmitRef;       // If tokenEmit is a typeDef, this is the corresponding typeRef.

    struct BASENODE * parseTree;      // The parse tree for this class.  
                                      // Either a CLASSNODE or a DELEGATENODE

    PSYMLIST        abstractMethods;    // for abstract classes the list of all unoverriden abstract methods(inherited and new)

    PSYMLIST        conversionOperators;    // list of locally defined and inherited conversion operators.
                                            // Does not include inherited conversions which are shadowed by local conversions.

    CorAttributeTargets attributeClass; // symbol type this type can be an attribute on. 
                                    // 0 == not an attribute class
                                    // -1 == unknown (not imported)

    INCREMENTALTYPE *incrementalInfo; // information about type for incremental build
    LPWSTR  comImportCoClass;   // If isInterface and class has ComImport and CoClass attributes, this is the unresolved CoClass string

private:
    unsigned short fieldsSize;   // if struct, the count of all encompassed fields, otherwise 1

public:
    unsigned short getFieldsSize();
    void storeFieldOffsets(unsigned * list);
    unsigned hasInnerFields();

    PPARENTSYM      containingDeclaration() { return declaration; }
    bool            isNested() const { return (parent->kind == SK_AGGSYM); }
    unsigned        allowableMemberAccess();
    PINFILESYM      getInputFile();  // The input file defining this class.
    DWORD           getImportScope() const;   // The import scope
    ULONG           getAssemblyIndex() const;
    PNSDECLSYM      getNamespaceDecl() const;
    BASENODE *      getAttributesNode();
    PREDEFATTR      getPredefAttr() const;
    CorAttributeTargets     getElementKind();
};

/*
 * PERMSETINFO - capability security on a symbol
 */
class PERMSETINFO
{
public:
    bool        isAnySet;
    bool        isGuid[10];
    STRCONST *  str[10];

    bool        isSet(int index)        { return str[index] != 0; }
};
typedef PERMSETINFO * PPERMSETINFO;

/*
 * AGGINFO - Additional information about an aggregate symbol that isn't
 * needed by other code binding against this aggregate. This structure
 * lives only when this particular aggregate is being compiled.
 */
class AGGINFO
{
public:
    bool hasStructLayout:1;
    bool hasExplicitLayout:1;
    bool hasUuid:1;     // COM classic attribute
    bool isComimport:1; // COM classic attribute
    bool isUnsafe: 1;   // has 'unsafe' modifier (can be 'inherited' from parent

    unsigned *offsetList;
};
typedef AGGINFO * PAGGINFO;


// The pseudo-methods uses for accessing arrays (except in
// the optimized 1-d case.
enum ARRAYMETHOD {
    ARRAYMETH_LOAD,
    ARRAYMETH_LOADADDR,
    ARRAYMETH_STORE,
    ARRAYMETH_CTOR, 
    ARRAYMETH_GETAT,  // Keep these in this order!!!

    ARRAYMETH_COUNT
};

/*
 * ALIASSYM - a symbol representing an using alias clause
 *
 * It is not linked into a parent child relationship
 */
class ALIASSYM: public SYM {
public:
    PARENTSYM *     boundType;
    BASENODE *      parseTree;
};

/*
 * ARRAYSYM - a symbol representing an array.
 */
class ARRAYSYM: public TYPESYM
{
public:
    int rank;               // rank of the array.                                                 
                            // zero means unknown rank int [?].
    PTYPESYM elementType()
        { return parent->asTYPESYM(); }  // parent is the element type.

    mdTypeRef  tokenEmit;                // Metadata token (typeRef) in the current output file.

    // Metadata tokens for ctor/load/loadaddr/store special methods.
    mdMemberRef  tokenEmitPseudoMethods[ARRAYMETH_COUNT];

};


/*
 * PARAMMODSYM - a symbol representing parameter modifier -- either
 * out or ref.
 */
class PARAMMODSYM: public TYPESYM
{
public:
    bool isRef: 1;            // One of these two bits must be set,
    bool isOut: 1;            // indication a ref or out parameter.

    PTYPESYM paramType()
        { return parent->asTYPESYM(); }  // parent is the parameter type.
};
 

/*
 * PTRSYM - a symbol representing a pointer type
 */
class PTRSYM: public TYPESYM
{
public:
    mdTypeRef  tokenEmit;                // Metadata token (typeRef) in the current output file.
    PTYPESYM baseType()
        { return parent->asTYPESYM(); }  // parent is the base type.
};

/*
 * PINNEDSYM - a symbol representing a pinned type
 *      used only to communicate between ilgen & emitter
 */
class PINNEDSYM: public TYPESYM
{
public:
    PTYPESYM baseType()
        { return parent->asTYPESYM(); }  // parent is the base type.
};

/*
 * VOIDSYM - represents the type "void".
 */
class VOIDSYM: public TYPESYM
{
};

/*
 * NULLSYM - represents the null type -- the type of the "null constant".
 */
class NULLSYM: public TYPESYM
{
};

/*
 * DELETEDTYPESYM - represents a type which was deleted in an EnC
 */
class DELETEDTYPESYM: public TYPESYM
{
public:
    ULONG32 signatureSize;
};

/*
 * METHPROPSYM - abstract class representing a method or a property. There
 * are a bunch of algorithms in the compiler (e.g., override and overload resolution)
 * that want to treat methods and properties the same. This abstract base class
 * has the common parts. 
 */
class METHPROPSYM: public SYM {
public:
    unsigned short cParams;         // Number of parameters.
    bool isStatic: 1;               // Static member?
    bool isOverride: 1;             // Overrides an inherited member. Only valid if isVirtual is set.
                                    // false implies that a new vtable slot is required for this method.

    bool useMethInstead: 1;         // Only valid iff isBogus == TRUE && kind == SK_PROPSYM.
                                    // If this is true then tell the user to call the accessors directly.

    bool isOperator: 1;             // a user defined operator(or default indexed property)

    bool isVarargs: 1;              // has varargs
        
    bool isParamArray: 1;           // new style varargs

    bool hasCmodOpt: 1;             // has CMOD_OPT in signature

    bool isHideByName: 1;           // this property hides all below it regardless of signature 

    SYM *explicitImpl;              // If non-NULL, this member is an explicit interface member implementation,
                                    // and the member being implemented is pointed to by this member. In this case
                                    // the name of this member is NULL. For method, is a method. For property, can
                                    // be property or event.

    mdToken     tokenImport;        // Meta-data token for imported method.

    mdToken     tokenEmit;          // Metadata token (memberRef or memberDef) in the current output file.

    PTYPESYM    retType;            // Return type.
    PTYPESYM *  params;             // array of cParams parameter types.

    BASENODE *  parseTree;          // Valid only between define & prepare stages...

    AGGSYM *    getClass() const { return parent->asAGGSYM(); }

    ULONG       getAssemblyIndex()  const { return getClass()->getAssemblyIndex(); }

    void copyInto(METHPROPSYM * sym) {
        SYM::copyInto(sym);
        sym->cParams = cParams;
        sym->isStatic = isStatic;
        sym->isBogus = isBogus;
        sym->retType = retType;
        sym->params = params;
        sym->isVarargs = isVarargs;
        sym->isParamArray = isParamArray;
    }
    METHPROPSYM ** getFParentSym();
};
typedef METHPROPSYM * PMETHPROPSYM;

/*
 * METHSYM - a symbol representing a method. Parent is a struct, interface
 * or class (aggregate). No children.
 */
class METHSYM: public METHPROPSYM {
public:
    bool isExternal : 1;            // Has external definition.
    bool isSysNative :1;            // Has definition implemented by the runtime.
    bool isVirtual: 1;              // Virtual member?
    bool isMetadataVirtual: 1;      // Marked as virtual in the metadata (if mdVirtual + mdSealed, this will be true, but isVirtual will be false).
    bool isAbstract: 1;             // Abstract method?
    bool isCtor: 1;                 // Is a constructor or static constructor (depending on isStatic).
    bool isDtor: 1;                 // Is a destructor
    bool isPropertyAccessor: 1;     // true if this method is a property set or get method
    bool isEventAccessor: 1;        // true if this method is an event add/remove method
    bool isExplicit: 1;             // is user defined explicit conversion operator
    bool isImplicit: 1;             // is user defined implicit conversion operator
    bool isIfaceImpl: 1;            // is really a IFACEIMPLMETHSYM
    bool checkedCondSymbols: 1;     // conditionalSymbols already includes parent symbols if override
    bool requiresMethodImplForOverride:1;       // only valid if isOverride = true.

    NAMELIST *  conditionalSymbols; // set if a conditional symbols for method
    NAMELIST *  getBaseConditionalSymbols(class SYMMGR * symmgr);

    bool isCompilerGeneratedCtor();
    PROPSYM *getProperty();         // returns property. Only valid to call if isPropertyAccessor is true
    EVENTSYM *getEvent();           // returns event. Only valid to call if isEventAccessor is true

    // returns true if accessor is a get accessor. Only valid if isPropertyAcessor is true.
    bool isGetAccessor();
    bool isConversionOperator() { return (isExplicit || isImplicit); }
    bool isUserCallable()       { return !isPropertyAccessor && !isOperator && !isEventAccessor; }
    IFACEIMPLMETHSYM *asIFACEIMPLMETHSYM() { ASSERT(isIfaceImpl); return (IFACEIMPLMETHSYM*) this; }

    void copyInto(METHSYM * sym) {
        METHPROPSYM::copyInto(sym);
        sym->isVirtual = isVirtual;
        sym->isMetadataVirtual = isMetadataVirtual;
        sym->isAbstract = isAbstract;
        sym->isCtor = isCtor;
    }

    BASENODE* getParamParseTree(int iParam);
    BASENODE *  getAttributesNode();

#if USAGEHACK
    bool isUsed: 1;
#endif
};

// symbol for temporary use for overload resolution of "params" methods.
class EXPANDEDPARAMSSYM: public METHPROPSYM {
public:
    METHPROPSYM * realMethod;
};

/*
 * FAKEMETHSYM - a method which we insert into the symbol tree to ease binding of base. calls
 * Basically, if we have:
 * class a { public void foo() {} }
 * class b:a { }
 * class c:b {public void foo() { base.foo() {} }
 * the base call in c should bind to b.foo() even though no such beast exists.
 * So we would insert a FAKEMETHSYM into class b
 */
class FAKEMETHSYM: public METHSYM {
public:
    METHSYM * parentMethSym;
};

/*
 * an explicit method impl generated by the compiler
 * usef for CMOD_OPT interop
 */
class IFACEIMPLMETHSYM : public METHSYM {
public:
    METHSYM * implMethod;
};

/* 
 * PARAMINFO - Additional information about a parameter symbol that isn't
 * needed by other code binding against this method. This structure
 * lives only when this particular method is being compiled.
 */
class PARAMINFO
{
public:
    NAME *name;

    // COM classic attributes
    bool isIn:1;
    bool isOut:1;
    bool isParamArray:1;

    mdToken         tokenEmit;
};

/* 
 * METHINFO - Additional information about an method symbol that isn't
 * needed by other code binding against this method. This structure
 * lives only when this particular method is being compiled.
 */
class METHINFO
{
public:
    PSCOPESYM outerScope;               // The arg scope if this method, if any...

    bool isMagicImpl : 1;               // This is a "magic" method with run-time supplied implementation:
                                        //   e.g.: delegate Invoke or delegate ctor.
    bool hasRetAsLeave: 1;              // has a return inside of a try or catch

    bool noDebugInfo: 1;                // Don't generate debug information. Used for compiler-created methods.
    bool isUnsafe: 1;
    bool isSynchronized: 1;             // synchronized bit (only used for event accessor's; not settable from code).

    bool preInitsNeeded : 1;
    bool hasReturnValueAttributes: 1;

    METHSYM * hTableCreate;
    METHSYM * hTableAdd;
    METHSYM * hTableGet;
    METHSYM * hIntern;
    METHSYM * hInitArray;
    METHSYM * hStringOffset;

    class EXPR * switchList;

    // COM Classic attributes
    PARAMINFO *paramInfos;              // Parameter marshalling info (has METHSYM::cParams elements)
    PARAMINFO returnValueInfo;
};
typedef METHINFO * PMETHINFO;

/*
 * PROPSYM - a symbol representing a property. Parent is a struct, interface
 * or class (aggregate). No children.
 */
class PROPSYM: public METHPROPSYM {
public:
    bool        isEvent : 1;        // This field is the implementation for an event.

    METHSYM    *methGet;            // Getter method (always has same parent)
    METHSYM    *methSet;            // Setter method (always has same parent)

    bool        isIndexer()     { return isOperator; }
    class INDEXERSYM * asINDEXERSYM() { ASSERT(isIndexer()); return (class INDEXERSYM*)this; }
    NAME *      getRealName();
    void copyInto(PROPSYM * sym) {
        METHPROPSYM::copyInto(sym);
    }
    EVENTSYM *getEvent(class SYMMGR *);   // returns event. Only valid to call if isEvent is true
    BASENODE *getAttributesNode();
};
typedef PROPSYM * PPROPSYM;

/*
 * INDEXERSYM - a symbol representing an indexed property. Parent is a struct, interface
 * or class (aggregate). No children.
 *
 * Has kind == SK_PROPSYM.
 */
class INDEXERSYM: public PROPSYM
{
public:
    NAME *      realName;       // the 'real' name of the indexer. All indexers have the same name.
};

/*
 * FAKEPROPSYM - see FAKEMETHSYM for an explanation
 */
class FAKEPROPSYM: public PROPSYM {
public:
    METHSYM * parentPropSym;
};

/* 
 * PROPINFO - Additional information about an property symbol that isn't
 * needed by other code binding against this method. This structure
 * lives only when this particular method is being compiled.
 */
class PROPINFO
{
public:
    PARAMINFO * paramInfos;          // Parameter name info (has PROPSYM::cParams elements)
};
typedef PROPINFO * PPROPINFO;

/* 
 * VARSYM - a symbol representing a variable. Specific subclasses are 
 * used - MEMBVARSYM for member variables, LOCVARSYM for local variables
 * and formal parameters. 
 */
class VARSYM: public SYM {
public:
    PTYPESYM    type;                       // Type of the field.
};
typedef VARSYM * PVARSYM;


/*
 * MEMBVARSYM - a symbol representing a member variable of a class. Parent
 * is a struct or class.
 */
class MEMBVARSYM: public VARSYM {
public:
    bool isStatic: 1;               // Static member?
    bool isConst: 1;                // Is a compile-time constant; see constVal for value.
    bool isReadOnly : 1;            // Can only be changed from within constructor.
    bool isEvent : 1;               // This field is the implementation for an event.
    bool isVolatile : 1;            // This fields is marked volatile

    bool isUnevaled: 1;             // This has an unevaluated constant value
    bool isReferenced:1;            // Has this been referenced by the user?
    bool isAssigned:1;              // Has this ever been assigned by the user?

    CONSTVAL constVal;              // If isConst is set, a constant value.
    struct BASENODE * parseTree;    // parse tree, could be a VARDECLNODE or a ENUMMEMBRNODE

    mdToken tokenImport;            // Meta-data token for imported variable.

    mdToken tokenEmit;              // Metadata token (memberRef or memberDef) in the current output file.

    union
    {
        unsigned short offset;          // offset in parent
        MEMBVARSYM *previousEnumerator; // used for enumerator values only
                                        // pointer to previous enumerator in enum declaration
    };

    AGGSYM *    getClass() const { return parent->asAGGSYM(); }
    ULONG       getAssemblyIndex()  const{ return getClass()->getAssemblyIndex(); }
    BASENODE *  getBaseExprTree();    // returns the base of the expression tree for this initializer
    BASENODE *  getConstExprTree();   // returns the constant expression tree(after the =) or null
    EVENTSYM *  getEvent(class SYMMGR *);   // returns event. Only valid to call if isEvent is true
    BASENODE *  getAttributesNode();
};

/*
 * MEMBVARINFO - Additional information about an meber variable symbol that isn't
 * needed by other code binding against this variable. This structure
 * lives only when this particular variable is being compiled.
 */
class MEMBVARINFO
{
public:
    bool        foundOffset;
};
typedef MEMBVARINFO * PMEMBVARINFO;

/* 
 * EVENTSYM - a symbol representing an event. The symbol points to the AddOn and RemoveOn methods
 * that handle adding and removing delegates to the event. If the event wasn't imported, it
 * also points to the "implementation" of the event -- a field or property symbol that is always
 * private.
 */
class EVENTSYM: public SYM
{
public:
    bool        isStatic: 1;        // Static member?

    bool useMethInstead: 1;         // Only valid iff isBogus == TRUE.
                                    // If this is true then tell the user to call the accessors directly.

    PTYPESYM    type;               // Type of the event.

    METHSYM    *methAdd;            // Adder method (always has same parent)
    METHSYM    *methRemove;         // Remover method (always has same parent)

    SYM        *implementation;     // underlying field or property the implements the event.
    EVENTSYM   *explicitImpl;       // if an explicit impl, the explicitly implemented event.

    mdToken     tokenImport;        // Meta-data token for imported event.
    mdToken     tokenEmit;          // Metadata token (memberRef or memberDef) in the current output file.

    struct BASENODE * parseTree;    // parse tree, could be a VARDECLNODE or a PROPDECLNODE

    AGGSYM *    getClass() const { return parent->asAGGSYM(); }
    EVENTSYM   *getExplicitImpl()
    {
        return explicitImpl;
    }
    BASENODE * getAttributesNode();

    ULONG       getAssemblyIndex()  const { return getClass()->getAssemblyIndex(); }
};
typedef EVENTSYM * PEVENTSYM;

/* 
 * EVENTINFO - Additional information about an event symbol that isn't needed by other code
 * binding against this event. This structure lives only when this particular variable is
 * being compiled.
 */
class EVENTINFO
{
public:
};
typedef EVENTINFO * PEVENTINFO;


//////////////////////////////////////////////////////////////////////////////////////////////

// different kinds of generated temporaries for ENC
enum TEMP_KIND
{
    TK_REUSABLE = 0,        // must be == 0
    TK_DELETEDENCVARIABLE,
    TK_SHORTLIVED,
    TK_RETURN,
    TK_LOCK,
    TK_USING,
    TK_FOREACH_GETENUM,
    TK_FOREACH_ARRAY,
    TK_FOREACH_ARRAYINDEX_0,       
    // NOTE: this must be the last one. 
    // NOTE: additional kinds are created based on the rank of the foreached array
    TK_FOREACH_ARRAYLIMIT_0 = TK_FOREACH_ARRAYINDEX_0 + 256,
    // NOTE: OK, I lied. we need two extendible kinds of temps, so limit the arrayindexes in EnC to 256
    TK_FIXED_STRING_0 = TK_FOREACH_ARRAYLIMIT_0 + 256,
};

class LOCSLOTINFO{
public:
    PTYPESYM type;
    unsigned ilSlot; // also used for var init checking...
    bool isParam:1;
    bool isRefParam:1; // also set if outparam...
    bool isTemporary:1;
    bool hasInit:1;
    bool isUsed:1;
    bool isPBUsed:1;
    bool isReferenced:1;
    bool isReferencedAssg:1;
    bool mustBePinned:1; // this is set when the variable is declared
    bool isPinned:1; // and this is set when it is first assigned to
    bool needsPreInit:1;  // used as an out variable before being initialized
    // The following fields apply to temporaries only:
    bool isTaken:1;
    bool isDeleted:1;   // this is a deleted variable... happens in EnC
    bool canBeReusedInEnC:1;
    TEMP_KIND tempKind;
#if DEBUG
    char * lastFile;
    unsigned lastLine;
#endif
};

/*
 * PREDEFATTRSYM - a symbol representing a predefined attribute type
 */
class PREDEFATTRSYM : public SYM
{
public:
    PREDEFATTR  attr;
    PREDEFTYPE  iPredef;
};

/*
 * GLOBALATTRSYM - a symbol representing a global attribute on an assembly or module
 */
class GLOBALATTRSYM : public SYM
{
public:
    struct BASENODE *   parseTree;
    CorAttributeTargets elementKind;
    GLOBALATTRSYM *     nextAttr;

    bool isAssembly()       { return elementKind == catAssembly; }
    bool isModule()         { return elementKind == catModule; }
};

/*
 * LOCVARSYM - a symbol representing a local variable or parameter. Parent
 * is a scope.
 */
class LOCVARSYM: public VARSYM {
public:
    LOCSLOTINFO slot;
    bool isConst : 1;
    bool isNonWriteable : 1;    // used for catch variables, and fixed variables
    CONSTVAL constVal;
    POSDATA firstUsed; // line of decl...
    struct BBLOCK * debugBlockFirstUsed;  // If debug info on: IL location of first use
    unsigned        debugOffsetFirstUsed;
    struct BASENODE * declTree;
    unsigned ownerOffset;
};

// Flags which any scope may have
enum SCOPEFLAGS {
    SF_NONE                 = 0x00,
    SF_CATCHSCOPE           = 0x01,
    SF_TRYSCOPE             = 0x02,
    SF_SWITCHSCOPE          = 0x04,
    SF_FINALLYSCOPE         = 0x08,
    SF_KINDMASK             = 0x0F,
    SF_HASRETHROW           = 0x10,
    SF_ARGSCOPE            = 0x20, // special scope for base (or this) call args
    SF_HASVARS             = 0x40,  // this, or a child scope, has locals of interest
    SF_TRYOFFINALLY        = 0x80,  // this is a try scope of a try-finally...
};

/*
 * SCOPESYM - a symbol represent a scope that holds other symbols. Typically
 * unnamed.
 */
class SCOPESYM: public PARENTSYM {
public:
    unsigned nestingOrder;  // the nesting order of this scopes. outermost == 0
    union {
        class EXPRBLOCK * block; // the associated block, or NULL... (for finally scopes only)
        SCOPESYM * finallyScope; // for try scopes only...
    };
    BASENODE * tree; // last statement in this scope...
    struct BBLOCK * debugBlockStart;  // If debug info on: location of first IL instruction in scope
    unsigned        debugOffsetStart;
    struct BBLOCK * debugBlockEnd;    // If debug info on: location of first IL instruction after scope
    unsigned        debugOffsetEnd;
    int scopeFlags;
};


/*
 * ERRORSYM - a symbol representing an error that has been reported.
 */
class ERRORSYM: public TYPESYM {
public:
};


/*
 * We have member functions here to do casts that, in DEBUG, check the 
 * symbol kind to make sure it is right. For example, the casting method
 * for METHODSYM is called "asMETHODSYM". In retail builds, these 
 * methods optimize away to nothing.
 */

// Define all the concrete kinds here.
#define SYMBOLDEF(k) \
    __forceinline k * SYM::as ## k () {   \
        ASSERT(this == NULL || this->kind == SK_ ## k);  \
        return static_cast<k *>(this);     \
    }
#include "symkinds.h"
#undef SYMBOLDEF

// Define the symbol casting functions for the abstract symbol kinds.

__forceinline PARENTSYM * SYM::asPARENTSYM() 
{
    ASSERT(this == NULL ||
           this->kind == SK_AGGSYM     || this->kind == SK_NSSYM  ||
           this->kind == SK_ERRORSYM   || this->kind == SK_SCOPESYM ||
           this->kind == SK_OUTFILESYM || this->kind == SK_ARRAYSYM ||
           this->kind == SK_NSDECLSYM);

    return static_cast<PARENTSYM *>(this);
}

__forceinline TYPESYM * SYM::asTYPESYM() 
{
    ASSERT(this == NULL ||
           this->kind == SK_AGGSYM     || this->kind == SK_ARRAYSYM  ||
           this->kind == SK_PTRSYM     || this->kind == SK_PARAMMODSYM ||
           this->kind == SK_VOIDSYM    || this->kind == SK_NULLSYM   ||
           this->kind == SK_ERRORSYM);

    return static_cast<TYPESYM *>(this);
}

__forceinline VARSYM * SYM::asVARSYM() 
{
    ASSERT(this == NULL ||
           this->kind == SK_MEMBVARSYM || this->kind == SK_LOCVARSYM);

    return static_cast<VARSYM *>(this);
}


__forceinline METHPROPSYM * SYM::asMETHPROPSYM() 
{
    ASSERT(this == NULL ||
           this->kind == SK_METHSYM || this->kind == SK_PROPSYM ||
           this->kind == SK_FAKEMETHSYM || this->kind == SK_FAKEPROPSYM || 
           this->kind == SK_EXPANDEDPARAMSSYM
           );

    return static_cast<METHPROPSYM *>(this);
}

__forceinline METHSYM * SYM::asFMETHSYM()
{
    ASSERT(this == NULL ||
            this->kind == SK_METHSYM || this->kind == SK_FAKEMETHSYM ||
            this->kind == SK_EXPANDEDPARAMSSYM
            );

    return static_cast<METHSYM *>(this);
}

__forceinline PROPSYM * SYM::asFPROPSYM()
{
    ASSERT(this == NULL ||
            this->kind == SK_PROPSYM || this->kind == SK_FAKEPROPSYM
            );

    return static_cast<PROPSYM *>(this);
}

__forceinline INFILESYM * SYM::asANYINFILESYM()
{
    ASSERT(this == NULL ||
            this->kind == SK_INFILESYM || this->kind == SK_SYNTHINFILESYM
            );

    return static_cast<INFILESYM *>(this);
}

__forceinline bool SYM::isStructType()
{
    return kind == SK_AGGSYM && (asAGGSYM()->isStruct || asAGGSYM()->isEnum);
}


__forceinline bool SYM::hasExternalAccess()
{
    ASSERT(parent || kind == SK_NSSYM || kind == SK_NSDECLSYM);
    return (kind == SK_NSSYM || kind == SK_NSDECLSYM) || 
        ((access == ACC_PUBLIC || access == ACC_PROTECTED || access == ACC_INTERNALPROTECTED) && parent->hasExternalAccess());
}

__forceinline TYPESYM * TYPESYM::underlyingType() 
{
    if (kind == SK_AGGSYM && asAGGSYM()->isEnum) return asAGGSYM()->underlyingType;
    return this;
}

__forceinline bool TYPESYM::isUnsafe()
{
    // Pointer types are the only unsafe types.
    return (this != NULL && this->kind == SK_PTRSYM);
}

__forceinline NAME *PROPSYM::getRealName()
{
    if (isIndexer()) {
        return ((INDEXERSYM*)this)->realName;
    } else {
        return name;
    }
}

inline METHPROPSYM ** METHPROPSYM::getFParentSym()
{
    ASSERT(kind == SK_FAKEPROPSYM || kind == SK_FAKEMETHSYM);

    if (kind == SK_FAKEPROPSYM) {
        return (METHPROPSYM**)&(((FAKEPROPSYM*)this)->parentPropSym);
    } else {
        return (METHPROPSYM**)&(((FAKEMETHSYM*)this)->parentMethSym);
    }
}

#endif //__symbol_h__

