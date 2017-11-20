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
// File: clsdrec.h
//
// Defines the structure which contains information necessary to declare
// a single class
// ===========================================================================

enum CONVTYPE {
    CONV_IMPL,
    CONV_EXPL,
    CONV_NONE,
    CONV_OTHER,  // for example, struct to byte,
    CONV_ERROR,  // meaning an ICE if we ever encounter it

};

enum BINDTYPEFLAG {
    BTF_NONE = 0,
    BTF_STARTATNS       = 1,    // start the search at the enclosing NS, not the current class
    BTF_NSOK            = 2,    // namespaces are ok to be returned
    BTF_ANYBADACCESS    = 4,    // on not found, report bad access on any type of symbol with same name
    BTF_ATTRIBUTE       = 8,    // on not found, check for Attribute suffix
    BTF_NODECLARE       = 16,   // don't force anything to be declared during the lookup
    BTF_NODEPRECATED    = 32,   // don't report deprecated errors
    BTF_NOERRORS        = 64,   // don't report any warnings or errors.
    BTF_USINGALIAS      = 128,  // when binding the RHS of a using alias (skips first level of using clauses)
};

#define XML_INDENT  L"    "

class COMPILER;
class CError;


class CLSDREC {
    friend struct XMLRefError;

private:
    bool isAtLeastAsVisibleAs(SYM * sym1, SYM * sym2);
    void checkConstituentVisibility(SYM * main, SYM * constituent, int errCode);

    void checkUnsafe(BASENODE * tree, PINFILESYM inputfile, TYPESYM * type);
    bool checkForBadMember(NAME *name, BASENODE *parseTree, AGGSYM *cls);
    bool checkForBadMember(NAME *name, SYMKIND symkind, PTYPESYM *params, BASENODE *parseTree, AGGSYM *cls);
    bool checkForBadMember(NAMENODE *nameNode, SYMKIND symkind, PTYPESYM *params, AGGSYM *cls);
    bool checkForDuplicateSymbol(NAME *name, BASENODE *parseTree, AGGSYM *cls);
    bool checkForDuplicateSymbol(NAME *name, SYMKIND symkind, PTYPESYM *params, BASENODE *parseTree, AGGSYM *cls);
    bool checkForDuplicateSymbol(NAMENODE *nameNode, SYMKIND symkind, PTYPESYM *params, AGGSYM *cls);
    void checkCLSnaming(AGGSYM *cls);
    void checkCLSnaming(NSSYM *cls);
    void checkForMethodImplOnOverride(METHSYM *method, METHSYM *overridenMethod);
    PTYPESYM * RemoveRefOut(int cTypes, PTYPESYM * types);

    SYM *checkExplicitImpl(BASENODE *name, PTYPESYM returnType, PTYPESYM *params, SYM *sym, AGGSYM *cls);

    SYM *findDuplicateConversion(bool conversionsOnly, PTYPESYM *parameterTypes, PTYPESYM returnType, PNAME name, AGGSYM* cls);

    void declareNamespace(PNSDECLSYM declaration);
    void declareGlobalAttribute(ATTRDECLNODE *pNode, NSDECLSYM *declaration);
    void declareAggregate(CLASSNODE * pClassTree, PARENTSYM * parent);

    void ensureBasesAreResolved(AGGSYM *cls);
    void ensureUsingClausesAreResolved(NSDECLSYM *nsDeclaration, AGGSYM *classBeingResolved);
    bool bindUsingAlias(ALIASSYM *alias, SYM **returnValue, SYM **badAccess, AGGSYM *classBeingResolved);

    void defineAggregateMembers(AGGSYM *cls);
    void defineEnumMembers(AGGSYM *cls);
    void defineDelegateMembers(AGGSYM *cls);
    void defineProperty(PROPERTYNODE *propertyNode, AGGSYM *cls);
    METHSYM *definePropertyAccessor(PNAME name, BASENODE *parseTree, unsigned cParam, PTYPESYM *params, PTYPESYM retType, unsigned allowableFlags, unsigned propertyFlags, METHSYM **accessor, AGGSYM *cls);
    void definePropertyAccessors(PROPSYM *property);
    PNAME createAccessorName(PNAME propertyName, LPCWSTR prefix);
    void checkParamsArgument(BASENODE * tree, AGGSYM * cls, unsigned cParam, PTYPESYM *paramTypes);
    METHSYM * defineMethod(METHODNODE * pMethodTree, AGGSYM * pClass);
    bool defineFields(FIELDNODE * pFieldTree, AGGSYM * pClass);
    bool defineOperator(METHODNODE * operatorNode, AGGSYM * cls);
    void checkMatchingOperator(PREDEFNAME pn, AGGSYM * cls);
    EVENTSYM * defineEvent(SYM * implementingSym, BASENODE * pTree);
    METHSYM * defineEventAccessor(PNAME name, BASENODE * parseTree, PTYPESYM eventType, ACCESS access, unsigned flags, METHSYM **accessor, AGGSYM * cls);

    AGGSYM * addAggregate(BASENODE *aggregateNode, NAMENODE* nameNode, PARENTSYM* parent);
    NSSYM * addNamespace(NAMENODE* name, NSDECLSYM* parent);
    NSDECLSYM * addNamespaceDeclaration(NAMENODE* name, NAMESPACENODE *parseTree, NSDECLSYM *containingDeclaration);

    SYM * bindDottedTypeName(BINOPNODE * name, PARENTSYM * symContext, int flags, AGGSYM *classBeingResolved);
    bool searchUsingClauses(NAME* name, BASENODE *node, NSDECLSYM * nsDeclaration, PARENTSYM *context, SYM **returnValue, SYM ** badAccess, SYM ** badKind, AGGSYM *classBeingResolved);
    bool searchNamespace(NAME *name, BASENODE *node, NSDECLSYM * nsDeclaration, PARENTSYM *context, SYM **returnValue, SYM ** badAccess, SYM ** badKind, bool fUsingAlias, AGGSYM *classBeingResolved);
    bool searchClass(NAME *name, AGGSYM * classToSearchIn, PARENTSYM *context, SYM **returnValue, bool declareIfNeeded, SYM ** badAccess, SYM ** badKind);
    METHSYM *findSameSignature(METHSYM *method, AGGSYM *classToSearchIn);
    PROPSYM *findSameSignature(PROPSYM *prop, AGGSYM *classToSearchIn);
    void checkHiddenSymbol(SYM *newSymbol, SYM *hiddenSymbol);
    void checkFlags(SYM * item, unsigned allowedFlags, unsigned actualFlags, bool error = true);

    void resolveStructLayout(AGGSYM *cls);
    void prepareFields(MEMBVARSYM *field);
    NAME *getMethodConditional(METHSYM *method);
    void prepareMethod(METHSYM * method);
    void prepareConversion(METHSYM * conversion);
    void prepareOperator(METHSYM * op);
    void prepareProperty(PROPSYM * property);
    void prepareEvent(EVENTSYM * property);
    void prepareInterfaceMember(METHPROPSYM * method);
    void prepareClassOrStruct(AGGSYM * cls);
    void reportHiding(SYM *sym, SYM *hiddenSymbol, unsigned flags);
    void checkSimpleHiding(SYM *sym, unsigned flags);
    void checkSimpleHiding(SYM *sym, unsigned flags, SYMKIND symkind, PTYPESYM *params);
    void checkIfaceHiding(SYM *sym, unsigned flags, SYMKIND symkind, PTYPESYM *params);
    SYM *findHiddenSymbol(NAME *name, AGGSYM *classToSearchIn, AGGSYM *context);
    void addUniqueInterfaces(SYMLIST *list, SYMLIST *** addToList, SYMLIST *source);
    void addUniqueInterfaces(SYMLIST *list, SYMLIST *** addToList, AGGSYM *sym);
    void prepareInterface(AGGSYM * cls);
    void prepareEnum(AGGSYM *cls);
    METHSYM *needExplicitImpl(METHSYM *ifaceMethod, METHSYM *implMethod, AGGSYM *cls);
    void findEntryPoint(AGGSYM *cls);

    void emitTypedefsAggregate(AGGSYM *cls);
    void emitMemberdefsAggregate(AGGSYM *cls);
    void reemitMemberdefsAggregate(AGGSYM *cls);

    void compileAggregate(AGGSYM * cls, bool UnsafeContext);
    void compileMethod(METHSYM * method, AGGINFO * info);
    void compileField(MEMBVARSYM * field, AGGINFO * info);
    void compileProperty(PROPSYM * property, AGGINFO * info);
    void compileEvent(EVENTSYM * event, AGGINFO * aggInfo);

    typedef void (CLSDREC::*MEMBER_OPERATION)(SYM *aggregateMember, VOID *info);
    void EnumMembersInEmitOrder(AGGSYM *cls, VOID *info, MEMBER_OPERATION doMember);
    void EmitMemberdef(SYM *member, VOID *unused);
    void CompileMember(SYM *member, AGGINFO *info);


    COMPILER * compiler();
    CController * controller();

    static const TOKENID accessTokens[];

public:
    CLSDREC();

    void combineInterfaceLists(AGGSYM *aggregate);
    void buildAbstractMethodsList(AGGSYM * cls);
    void setOverrideBits(AGGSYM * cls);

    // XML DocComments
    void BeginDocFile();
    void DocSym(SYM *sym, int cParam = 0, PARAMINFO * params = NULL);
    void EndDocFile(BOOL saveFile);

    void declareInputfile(NAMESPACENODE * parseTree, PINFILESYM infile);
    bool hasBaseChanged(AGGSYM *cls, AGGSYM *newBaseClass);
    AGGSYM *getEnumUnderlyingType(CLASSNODE *parseTree);
    bool resolveInheritance(NSDECLSYM *nsdecl);
    bool resolveInheritance(AGGSYM *cls);
    bool resolveInheritanceRec(AGGSYM *cls);
    bool setBaseType(AGGSYM *cls, AGGSYM *baseType);
    void prepareNamespace(NSDECLSYM *nsDeclaration);
    void defineObject();    // for our friend the symmgr
    void defineNamespace(NSDECLSYM *nsDeclaration);
    void defineAggregate(AGGSYM *cls);
    void emitTypedefsNamespace(NSDECLSYM *nsDeclaration);
    void emitMemberdefsNamespace(NSDECLSYM *nsDeclaration);
    void reemitMemberdefsNamespace(NSDECLSYM *nsDeclaration);
    void compileNamespace(NSDECLSYM *nsDeclaration);
    void prepareAggregate(AGGSYM * cls);

    bool evaluateFieldConstant(MEMBVARSYM * fieldFirst, MEMBVARSYM * fieldCurrent);
    void evaluateConstants(AGGSYM *cls);

    TYPESYM * bindType(TYPENODE * type, PARENTSYM* symContext, int flags, AGGSYM *classBeingResolved);
    SYM * bindSingleTypeName(NAMENODE * name, PARENTSYM * symContext, int flags, AGGSYM *classBeingResolved);
    SYM * lookupIfAccessOk(NAME *, PARENTSYM * parent, int mask, PARENTSYM * current, bool declareIfNeeded, SYM ** badAccess, SYM ** badKind);
    PSYMLIST getConversionList(AGGSYM *cls);
    SYM *findNextName(NAME *name, AGGSYM *classToSearchIn, SYM *current);
    SYM *findNextAccessibleName(NAME *name, AGGSYM *classToSearchIn, PARENTSYM *context, SYM *current, bool bAllowAllProtected, bool ignoreSpecialMethods);
    SYM *findAnyAccessHiddenSymbol(NAME *name, SYMKIND symkind, PTYPESYM *params, AGGSYM *classToSearchIn);
    SYM *findHiddenSymbol(NAME *name, SYMKIND symkind, PTYPESYM *params, AGGSYM *classToSearchIn, AGGSYM *context);
    SYM *findExplicitInterfaceImplementation(AGGSYM *cls,SYM *explicitImpl);
    SYM *findSymName(LPCWSTR fullyQualifiedName, size_t cchName = 0);

    OPERATOR operatorOfName(PNAME name);

    BOOL isAttributeClass(TYPESYM *type);
    bool isAttributeType(TYPESYM *type);
    SYM * bindTypeName(BASENODE *name, PARENTSYM * symContext, int flags, AGGSYM *classBeingResolved);

    void AddRelatedSymbolLocations (CError *pError, SYM *pSym);

    void errorSymbolSymbolSymbol(SYM *symbol, SYM *relatedSymbol1, SYM *relatedSymbol2, int id);
    void errorSymbolAndRelatedSymbol(SYM *symbol, SYM *relatedSymbol, int id);
    void errorNamespaceAndRelatedNamespace(NSSYM *NSsym, NSSYM *relatedNS, int id);

    void errorNameRef(BASENODE *name, PPARENTSYM refContext, int id);
    void errorNameRef(NAME *name, BASENODE *parseTree, PPARENTSYM refContext, int id);
    void errorNameRefStr(BASENODE *name, PPARENTSYM refContext, LPCWSTR string, int id);
    void errorNameRefStr(NAME *name, BASENODE *parseTree, PPARENTSYM refContext, LPCWSTR string, int id);
    void errorNameRefSymbol(BASENODE *name, PPARENTSYM refContext, SYM *relatedSymbol, int id);
    void errorNameRefStrSymbol(BASENODE *name, PPARENTSYM refContext, LPCWSTR string, SYM *relatedSymbol, int id);
    void errorNameRefStrSymbol(NAME *name, BASENODE *parseTree, PPARENTSYM refContext, LPCWSTR string, SYM *relatedSymbol, int id);
    void errorNameRefSymbol(NAME *name, BASENODE *parseTree, PPARENTSYM refContext, SYM *relatedSymbol, int id);
    void errorNameRefSymbolSymbol(NAME * name, BASENODE *node, PPARENTSYM refContext, SYM *relatedSymbol1, SYM *relatedSymbol2, int id);
    void errorType(TYPENODE *type, PPARENTSYM refContext, int id);
    void errorTypeSymbol(TYPENODE *type, PPARENTSYM refContext, SYM *relatedSymbol, int id);

    CError *make_errorSymbol(SYM *symbol, int id);
    void errorSymbol(SYM *symbol, int id);

    CError *make_errorStrStrSymbol(BASENODE *location, PINFILESYM inputFile, LPCWSTR str1, LPCWSTR str2, SYM *symbol, int id);

    CError *make_errorStrSymbol(BASENODE *location, PINFILESYM inputFile, LPCWSTR str, SYM *symbol, int id);
    void errorStrSymbol(BASENODE *location, PINFILESYM inputFile, LPCWSTR str, SYM *symbol, int id);

    CError *make_errorSymbolStr(SYM *symbol, int id, LPCWSTR str1);
    void errorSymbolStr(SYM *symbol, int id, LPCWSTR str);
    
    CError  *make_errorSymbolStrStr(SYM *symbol, int id, LPCWSTR str1, LPCWSTR str2);
    void    errorSymbolStrStr(SYM *symbol, int id, LPCWSTR str1, LPCWSTR str2);

    CError  *make_errorStrStrStrFile(BASENODE *tree, PINFILESYM inputfile, int id, LPCWSTR str1, LPCWSTR str2, LPCWSTR str3);
    void    errorStrStrStrFile(BASENODE *tree, PINFILESYM inputfile, int id, LPCWSTR str1, LPCWSTR str2, LPCWSTR str3);

    CError  *make_errorStrStrFile(BASENODE *tree, PINFILESYM inputfile, int id, LPCWSTR str1, LPCWSTR str2);
    void    errorStrStrFile(BASENODE *tree, PINFILESYM inputfile, int id, LPCWSTR str1, LPCWSTR str2);

    CError *make_errorStrFile(BASENODE *tree, PINFILESYM inputfile, int id, LPCWSTR str);
    void    errorStrFile(BASENODE *tree, PINFILESYM inputfile, int id, LPCWSTR str);

    void errorFile(BASENODE *tree, PINFILESYM inputfile, int id);
    void errorFileSymbol(BASENODE *tree, PINFILESYM inputfile, int id, SYM *sym);

    void CheckSymbol(BASENODE * tree, SYM * rval, PARENTSYM * symContext, int flags);
    bool checkAccess(SYM * target, PARENTSYM * cls, SYM ** badAccess, SYM * qualifier);
    void reportDeprecated(BASENODE * tree, PSYM refContext, SYM * sym);
    void reportDeprecatedType(BASENODE * tree, PSYM refContext, TYPESYM * type);
    void reportDeprecatedMethProp(BASENODE *tree, METHPROPSYM *methprop);
private:

    static const PREDEFNAME operatorNames[OP_LAST];
    static const bool FTConvTable[FT_COUNT][FT_COUNT];
    HANDLE  hDocFile;
    bool    bSaveDocs;
    bool    bHadIncludes;
    long commentIndex;
    long commentAdjust;

    void    CheckParamTags(SYM * sym, int cParam, PARAMINFO * params);
    LPWSTR  EncodeSymName(SYM* sym);
    LPWSTR  EncodeParams(int cParams, TYPESYM **params);
    LPWSTR  EncodeTypeSym(TYPESYM *type);
    LPWSTR  AddToString(LPWSTR *ppStart, LPWSTR pInsert, LPWSTR *ppEnd, LPCWSTR pszNew);
    bool    ReplaceReferences(SYM * sym);
    static HRESULT WriteFileAsUTF8(HANDLE hFile, LPCWSTR pszText);
};
