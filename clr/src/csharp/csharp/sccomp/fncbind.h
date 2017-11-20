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
// File: fncbind.h
//
// Defines the structure which contains information necessary to bind and generate
// code for a single function.
// ===========================================================================

class COMPILER;
class CError;

//////////////////////////////////////////////////////////////////////////////////////////////
// inline functions must be included here so they can be picked-up by metaattr.cpp
// so the definitions are at the end of this header

struct LOOPLABELS {
    EXPRLABEL * contLabel;
    EXPRLABEL * breakLabel;

    LOOPLABELS(class FUNCBREC *);
    LOOPLABELS() {};
};

struct CHECKEDCONTEXT {
    bool normal;
    bool constant;

    CHECKEDCONTEXT(class FUNCBREC *, bool checked);
    CHECKEDCONTEXT() {};
    void restore(class FUNCBREC *);
};

//////////////////////////////////////////////////////////////////////////////////////////////

// Used to string together methods in the pool of available methods...
struct METHLIST {
    METHPROPSYM * sym;
    METHLIST * next;
    short position;
    bool relevant;

    METHLIST () : sym(NULL), next(NULL), position (0){};
};

//////////////////////////////////////////////////////////////////////////////////////////////

struct SYMLISTSTACK {
    SYMLIST * syms;
    SYMLISTSTACK * next;
};

//////////////////////////////////////////////////////////////////////////////////////////////

#define LOOKUPMASK MASK_NSSYM | MASK_AGGSYM | MASK_MEMBVARSYM | MASK_LOCVARSYM | MASK_METHSYM | MASK_PROPSYM

// Flags for bindImplicitConversion/bindExplicitConversion
#define CONVERT_NOUDC 0x1   // Do not consider user defined conversions.
#define CONVERT_STANDARD 0x3   // Do standard conversions only (includes NOUDC)
#define CONVERT_ISEXPLICIT 0x4 // implicit conversion is really explicit
#define CONVERT_CHECKOVERFLOW 0x8 // check overflow (like in a checked context).

//////////////////////////////////////////////////////////////////////////////////////////////

#define BIND_RVALUEREQUIRED 0x01 // this is a get of expr, not an assignment to expr
#define BIND_MEMBERSET      0x02 // this is a set on a struct member
#define BIND_FIXEDVALUE     0x10 // ok to take address of unfixed
#define BIND_ARGUMENTS      0x20 // this is an argument list to a call...
#define BIND_BASECALL       0x40 // this is a base method or prop call
#define BIND_USINGVALUE     0x80 // local in a using stmt decl
#define BIND_STMTEXPRONLY  0x100 // only allow expressions that are valid in a statement

//////////////////////////////////////////////////////////////////////////////////////////////

class FUNCBREC {
    friend LOOPLABELS::LOOPLABELS(FUNCBREC *);
    friend CHECKEDCONTEXT::CHECKEDCONTEXT(FUNCBREC *, bool);
    friend void CHECKEDCONTEXT::restore(FUNCBREC *);

public:
    FUNCBREC();

    EXPR * compileMethod(METHSYM * pAMSym, BASENODE * pAMNode, METHINFO * info, AGGINFO * classInfo);
    EXPR * compileFirstField(MEMBVARSYM * pAFSym, BASENODE * tree);
    EXPR * compileNextField(MEMBVARSYM * pAFSym, BASENODE * tree);
    unsigned __fastcall localIsUsed(LOCVARSYM * local);
    unsigned __fastcall isThisPointer(EXPR * expr);
    LOCSLOTINFO * getThisPointerSlot();

    bool getAttributeValue(PARENTSYM *context, EXPR * val, bool *       rval);
    bool getAttributeValue(PARENTSYM *context, EXPR * val, int *        rval);
    bool getAttributeValue(PARENTSYM *context, EXPR * val, STRCONST **  rval);
    bool getAttributeValue(PARENTSYM *context, EXPR * val, TYPESYM **   rval);

    // Check the bogus flag (public function)
    bool __fastcall checkBogusNoError(SYM * sym) { return checkBogus(sym, true, NULL); }
    bool __fastcall checkBogus(SYM * sym) { return checkBogus(sym, false, NULL); }

    EXPR * bindAttribute(PARENTSYM *parent, ATTRNODE * attr, EXPR **namedArgs);
    EXPR * bindNamedAttributeValue(PARENTSYM *parent, ATTRNODE * attr);

    EXPRCALL *bindAttribute(PARENTSYM * context, AGGSYM * attributeClass, ATTRNODE * attribute, EXPR ** namedArgs);
    EXPR * bindSimplePredefinedAttribute(PARENTSYM * context, PREDEFTYPE pt);
    EXPR * bindSimplePredefinedAttribute(PARENTSYM * context, PREDEFTYPE pt, EXPR * arg);
    EXPR * bindStringPredefinedAttribute(PARENTSYM * context, PREDEFTYPE pt, LPCWSTR arg);
    EXPR * bindStructLayoutArgs();
    EXPR * bindStructLayoutNamedArgs(bool hasNonZeroSize);
    EXPR * bindSkipVerifyArgs();
    EXPR * bindSkipVerifyNamedArgs();
    EXPR * bindDebuggableArgs();

    void realizeStringConcat(EXPRCONCAT * expr);
private:
    // Persistent members:

    // Initialized w/ each function:
    METHSYM * pMSym;
    BASENODE * pTree;
    METHINFO * info;
    bool dontAddNamesToCache;
    PINFILESYM inputfile;
    int btfFlags;       // extra flags to bind type for deprecation checking


    // If initialized for a field:
    MEMBVARSYM * pFSym; // current field to evaluate
    MEMBVARSYM * pFOrigSym; // original field to evaluate

    PARENTSYM * pParent;
    AGGINFO * pClassInfo;

    NAMESPACENODE * nmTree;

    LOOPLABELS initLabels;
    LOOPLABELS * loopLabels;

    SCOPESYM * pOuterScope; // the argument scope
    SCOPESYM * pCurrentScope; // the current scope
    SCOPESYM * pFinallyScope; // the scope of the innermost finally, or pOuterScope if none...
    SCOPESYM * pTryScope; // the scope of the innermost try, or pOuterScope if none...
    SCOPESYM * pSwitchScope; // the scope of the innermost switch, or NULL if none
    SCOPESYM * pCatchScope; // the scope of the innermose catch, or NULL if none
    class EXPRBLOCK * pCurrentBlock;

    NRHEAP * allocator;

    LOCVARSYM * thisPointer;

    unsigned outParamCount;
    unsigned uninitedVarCount;
    unsigned unreachedLabelCount;
    unsigned untargetedLabelCount;
    unsigned unrealizedGotoCount;
    unsigned unrealizedConcatCount;
    unsigned unreferencedVarCount;
    unsigned finallyNestingCount;
    unsigned localCount;
    unsigned firstParentOffset;
    int errorsBeforeBind;
    BITSET * currentBitset;
    BITSET * errorDisplayedBitset;
    
    bool codeIsReachable;
    bool reachablePass;
    bool unsafeContext;
    bool unsafeErrorGiven;
    BASENODE * lastNode;

    CHECKEDCONTEXT checked;
    
    EXPR * redoList;
    EXPR * userLabelList;
    EXPR ** pUserLabelList;
    EXPRGOTO * gotos;

    bool inFieldInitializer;  // Are we evaluating a field initializer?
    
// The default one is good enough for us...
//    ~FUNCBREC();

    void initClass(AGGINFO * info = NULL);
    void initThisPointer();
    void initNextField(MEMBVARSYM * pAFSym);
    void declare(METHINFO * info);
    void declareMethodParameters(METHINFO * info);
    LOCVARSYM * declareVar(BASENODE * tree, NAME * ident, TYPESYM * type);
    LOCVARSYM * declareParam(PARAMETERNODE * param);
    LOCVARSYM * declareParam(NAME *ident, TYPESYM *type, unsigned flags, BASENODE * parseTree);
    LOCVARSYM * addParam(PNAME ident, TYPESYM *type, unsigned paramFlags);
    TYPESYM * bindType(TYPENODE * tree);
    
    LOCVARSYM * lookupLocalVar(NAME * name, SCOPESYM *scope);
    SYM * lookupClassMember(NAME* name, AGGSYM * pClass, bool bBaseCall, bool tryInaccessible, bool ignoreSpecialMethods, int forceMask = 0);

    // Check the bogus flag and update as needed (the real implementation
    bool __fastcall checkBogus(SYM * sym, bool bNoError, bool * pbUndeclared);

    EXPRZEROINIT   * newExprZero(BASENODE * tree, EXPR * op);
    EXPRZEROINIT   * newExprZero(BASENODE * tree, TYPESYM * type);
    EXPRCONSTANT * newExprConstant(BASENODE * tree, TYPESYM * type, CONSTVAL cv);
    EXPRLABEL * newExprLabel(bool userDefined = false);
    EXPRBLOCK * newExprBlock(BASENODE * tree);
    EXPRBINOP * newExprBinop(PBASENODE tree, EXPRKIND kind, TYPESYM * type, EXPR *p1, EXPR *p2);
    EXPR *      newExpr(PBASENODE tree, EXPRKIND kind, TYPESYM * type);
    EXPR *      newExpr(EXPRKIND kind) { return newExpr(NULL, kind, NULL); };
    EXPRWRAP *  newExprWrap(EXPR * wrap, TEMP_KIND tempKind);
    EXPRSTMTAS *newExprStmtAs(BASENODE * tree, EXPR * expr);
    void        newList(EXPR * newItem, EXPR *** list);
    EXPRERROR * newError(BASENODE * tree);

    EXPR *      createConstructorCall(BASENODE *tree, AGGSYM *cls, EXPR * thisExpression, EXPR *arguments, bool isNew);
    bool        isMethPropCallable(METHPROPSYM * sym, bool requireUC);
    METHPROPSYM *  verifyMethodCall(BASENODE * tree, METHPROPSYM * mp, EXPR * args, EXPR ** object, int flags, METHSYM * invoke = NULL);
    static AGGSYM *    pickNextInterface(AGGSYM * current, SYMLISTSTACK ** stack, bool * hideByNameMethodFound, SYMLISTSTACK ** newStackSlot);
    static void  cleanupInterfaces(SYMLIST * precludedList);
    METHPROPSYM *  remapToOverride(METHPROPSYM * mp, TYPESYM * object, int baseCallOverride);
    METHPROPSYM *  findBestMethod(METHLIST * list, EXPR * args, PMETHPROPSYM * ambigMeth1, PMETHPROPSYM * ambigMeth2);
    void        verifyMethodArgs(EXPR * call);
    bool        __fastcall canConvertParam(TYPESYM * actual, int actualFlags, TYPESYM * formal);
    int         whichMethodIsBetter(METHPROPSYM * method1, METHPROPSYM * method2, EXPR * args);
    unsigned    findBestConversion(SYMLIST * list, bool fromParam, bool implicit);
    METHSYM *   findIntersection(SYMLIST * fromList, unsigned fromCount, SYMLIST * toList, unsigned toCount);
    void        considerConversion(METHSYM * conversion, TYPESYM * desired, bool from, bool impl, bool * possibleWinner, bool * oldMethodsValid, SYMLIST ** newList, SYMLIST ** prevList, bool foundImplBefore);

    void        createNewScope();
    void        closeScope();
    static int __cdecl compareSwitchLabels(const void *l1, const void *l2);
    void __fastcall checkReachable(BASENODE * tree);
    void __fastcall checkStaticness(BASENODE * tree, SYM * member, EXPR ** object);
    bool __fastcall objectIsLvalue(EXPR * object);
    bool __fastcall hasCorrectType(SYM ** sym, int mask);
    METHSYM *       findMethod(BASENODE * tree, PREDEFNAME pn, AGGSYM * cls, TYPESYM *** params = NULL, bool reportError = true);
    METHSYM *       findStructDisposeMethod(AGGSYM * cls);

    bool        isConstantInRange(EXPRCONSTANT * exprSrc, TYPESYM * typeDest);
    bool        isConstantInRangeBuggy(EXPRCONSTANT * exprSrc, TYPESYM * typeDest, bool explicitCast);
    bool        bindConstantCast(BASENODE * tree, EXPR * exprSrc, TYPESYM * typeDest, EXPR ** pexprDest, bool * checkFailure);
    bool        bindSimpleCast(BASENODE * tree, EXPR * exprSrc, TYPESYM * typeDest, EXPR ** pexprDest, unsigned exprFlags = 0);
    bool        bindUserDefinedConversion(BASENODE * tree, EXPR * exprSrc, TYPESYM * typeSrc, TYPESYM * typeDest, 
                                          EXPR * * pexprDest, bool implicitOnly);
    bool        bindImplicitConversion(BASENODE * tree, EXPR * exprSrc, TYPESYM * typeSrc, TYPESYM * typeDest, 
                                       EXPR * * pexprDest, unsigned flags);
    bool        bindExplicitConversion(BASENODE * tree, EXPR * exprSrc, TYPESYM * typeSrc, TYPESYM * typeDest, 
                                       EXPR * * pexprDest, unsigned flags);

    bool        canConvert(EXPR * expr, TYPESYM * dest, BASENODE * tree, unsigned flags = 0);
    bool        canConvert(TYPESYM * src, TYPESYM * dest, BASENODE * tree, unsigned flags = 0);
    EXPR *      mustConvert(EXPR * expr, TYPESYM * dest, BASENODE * tree, unsigned flags = 0);
    EXPR *      tryConvert(EXPR * expr, TYPESYM * dest, BASENODE * tree, unsigned flags = 0);

    bool        canCast(EXPR * expr, TYPESYM * dest, BASENODE * tree, unsigned flags = 0);
    bool        canCast(TYPESYM * src, TYPESYM * dest, BASENODE * tree, unsigned flags = 0);
    EXPR *      mustCast(EXPR * expr, TYPESYM * dest, BASENODE * tree, unsigned flags = 0);
    EXPR *      tryCast(EXPR * expr, TYPESYM * dest, BASENODE * tree, unsigned flags = 0);

    EXPRBLOCK * bindBlock(BASENODE * tree, int scopeFlags = SF_NONE, SCOPESYM ** scope = NULL);
    EXPR * bindStatement(STATEMENTNODE * tree, EXPR *** newLast);
    EXPR * bindExpr(BASENODE * tree, int bindFlags = BIND_RVALUEREQUIRED); 
    EXPR * bindThisImplicit(PBASENODE tree);
    EXPR * bindThisExplicit(BASENODE * tree);
    EXPR * badOperatorTypesError(BASENODE * tree, EXPR * op1, EXPR * op2);
    EXPR * ambiguousOperatorError(BASENODE * tree, EXPR * op1, EXPR * op2);
    void   checkVacuousIntegralCompare(BASENODE * tree, EXPR * castOp, EXPR * constOp);
    EXPR * bindEnumOp(BASENODE * tree, EXPRKIND kind, unsigned flags, EXPR * op1, EXPR * op2, bool isBoolResult);
    EXPR * bindDelegateOp(BASENODE * tree, EXPRKIND kind, unsigned flags, EXPR * op1, EXPR * op2);
    EXPR * bindI4Op(BASENODE * tree, EXPRKIND kind, unsigned flags, EXPR * op1, EXPR * op2, TYPESYM * typeDest);
    EXPR * bindU4Op(BASENODE * tree, EXPRKIND kind, unsigned flags, EXPR * op1, EXPR * op2, TYPESYM * typeDest);
    EXPR * bindI8Op(BASENODE * tree, EXPRKIND kind, unsigned flags, EXPR * op1, EXPR * op2, bool op2IsInt, TYPESYM * typeDest);
    EXPR * bindU8Op(BASENODE * tree, EXPRKIND kind, unsigned flags, EXPR * op1, EXPR * op2, bool op2IsInt, TYPESYM * typeDest);
    EXPR * bindDecimalConstExpr(BASENODE * tree, EXPRKIND kind, EXPR * op1, EXPR * op2);
    EXPR * bindDecimalConstCast(BASENODE * tree, TYPESYM * destType, TYPESYM * srcType, EXPRCONSTANT * src);
    EXPR * bindBoolOp(BASENODE * tree, EXPRKIND kind, unsigned flags, EXPR * op1, EXPR * op2);
    EXPR * bindUserBoolOp(BASENODE * tree, EXPRKIND kind, EXPR * call);
    EXPR * bindFloatOp(BASENODE * tree, EXPRKIND kind, unsigned flags, EXPR * op1, EXPR * op2, TYPESYM * typeDest);
    EXPR * bindPtrOp(BASENODE * tree, EXPRKIND kind, unsigned flags, EXPR * op1, EXPR * op2);
    EXPR * bindConstStringEquality(BASENODE * tree, EXPRKIND kind, EXPR * op1, EXPR * op2);
    bool   hasSelfUDCompare(EXPR * op, EXPRKIND kind);
    EXPR * bindRefEquality(BASENODE * tree, EXPRKIND kind, EXPR * op1, EXPR * op2);
    EXPR * convertShiftOp(EXPR * op2, bool isLong);
    EXPR * bindShiftOp(BASENODE * tree, EXPRKIND kind, unsigned flags, EXPR * op1, EXPR * op2);
    EXPR * bindArithOp(BASENODE * tree, EXPRKIND kind, unsigned flags, EXPR * op1, EXPR * op, bool fpOK, bool isBoolResult);
    EXPR * bindEqualityOp(BASENODE * tree, EXPRKIND kind, EXPR * op1, EXPR * op2);
    EXPR * bindIncOp(BASENODE * tree, EXPR * op1, OPERATOR op);
    EXPR * bindPtrToArray(BASENODE * tree, EXPR * array);
    EXPR * bindPtrToString(BASENODE * tree, EXPR * string);
    EXPR * bindAssignment(BASENODE * tree, EXPR * op1, EXPR * op2);
    EXPR * bindArrayIndex(BINOPNODE * tree, EXPR * op2, int bindFlags = BIND_RVALUEREQUIRED);
    TYPESYM * chooseArrayIndexType(BASENODE * tree, EXPR * args);
    EXPR * bindBinop(BINOPNODE * tree, int bindFlags = BIND_RVALUEREQUIRED);
    EXPR * bindCall(BINOPNODE * tree, int bindFlags);
    EXPR * bindDot(BINOPNODE * tree, int mask, int bindFlags = BIND_RVALUEREQUIRED);
    EXPR * bindEventAccess(BASENODE * tree, bool isAdd, EXPREVENT * exprEvent, EXPR * exprRHS);
    EXPR * bindToField(BASENODE * tree, EXPR * object, MEMBVARSYM * field, int bindFlags = BIND_RVALUEREQUIRED);
    EXPR * bindToProperty(BASENODE * tree, EXPR * object, PROPSYM * prop, int bindFlags = BIND_RVALUEREQUIRED, EXPR * args = NULL);
    EXPR * bindToEvent(BASENODE * tree, EXPR * object, EVENTSYM * field, int bindFlags = BIND_RVALUEREQUIRED);
    EXPR * bindName(NAMENODE * name, int mask = LOOKUPMASK, int bindFlags = BIND_RVALUEREQUIRED);
    bool   bindUnevaledConstantField(MEMBVARSYM * field);
    EXPR * bindUnop(UNOPNODE * tree, int bindFlags = BIND_RVALUEREQUIRED);
    EXPR * bindTypeOf(UNOPNODE * tree, BASENODE * type = NULL);
    EXPR * bindIs(BINOPNODE * tree, EXPR * op1);
    EXPR * bindVarDecls(DECLSTMTNODE * tree, EXPR *** newLast);
    EXPR * bindFixedDecls(FORSTMTNODE * tree, EXPR *** newLast);
    EXPR * bindUsingDecls(FORSTMTNODE * tree, EXPR *** newLast);
    EXPR * bindUsingDecls(FORSTMTNODE * tree, EXPR * inits);
    EXPR * bindUsingDecls(FORSTMTNODE * tree, EXPR * first, EXPR * next);
    EXPR * bindVarDecl(VARDECLNODE * tree, TYPESYM * type, unsigned flags);
    SYM  * lookupCachedName(NAMENODE * name, int mask);
    bool   storeInCache(BASENODE * tree, NAME * name, SYM * sym, bool checkForParent = false);
    EXPR * bindReturn(EXPRSTMTNODE * tree);
    EXPR * bindIf(IFSTMTNODE * tree, EXPR *** newLast);
    EXPR * bindWhileOrDo(LOOPSTMTNODE * tree, EXPR *** newLast, bool asWhile);
    EXPR * bindFor(FORSTMTNODE * tree, EXPR *** newLast);
    EXPR * bindBreakOrContinue(BASENODE * tree, bool asBreak);
    EXPR * bindGoto(EXPRSTMTNODE * tree);
    EXPR * bindLabel(LABELSTMTNODE * tree, EXPR *** newLast);
    EXPR * bindChecked(LABELSTMTNODE * tree, EXPR *** newLast);
    EXPR * bindUnsafe(LABELSTMTNODE * tree, EXPR *** newLast);
    EXPR * bindCheckedExpr(UNOPNODE * tree, int bindFlags);
    void bindInitializers(bool isStatic, EXPR *** pLast);
    EXPR * createBaseConstructorCall(bool isThisCall);
    EXPR * bindConstructor(METHINFO * info);
    EXPR * bindDestructor(METHINFO * info);
    EXPRBLOCK *bindMethOrPropBody(BLOCKNODE *body);
    EXPR * bindMethod(METHINFO * info);
    EXPR * bindProperty(METHINFO * info);
    EXPR * bindEventAccessor(METHINFO * info);
    EXPR * bindIfaceImpl(METHINFO *info);
    EXPR * bindNew(NEWNODE * tree, bool stmtExprOnly);
    void   checkNegativeConstant(BASENODE * tree, EXPR * expr, int id);
    EXPR * bindArrayNew(ARRAYSYM * type, NEWNODE * tree);
    EXPR * bindDelegateNew(AGGSYM * type, NEWNODE * tree);
    EXPR * bindSwitch(SWITCHSTMTNODE * tree, EXPR *** newLast);
    void initForHashtableSwitch(BASENODE * tree, EXPRSWITCH * expr);
    void initForNonHashtableSwitch(BASENODE * tree);
    EXPR * verifySwitchLabel(BASENODE * tree, TYPESYM * type, bool * ok);
    NAME * getSwitchLabelName(EXPRCONSTANT * expr);
    EXPR * bindTry(TRYSTMTNODE * tree);
    EXPR * bindThrow(EXPRSTMTNODE * tree);
    EXPR * bindMultiOp(BASENODE * tree, EXPRKIND op, EXPR * op1, EXPR * op2);
    static bool isCastOrExpr(EXPR * search, EXPR * target);
    void checkGotoJump(EXPRGOTO * target);
    bool isManagedType(TYPESYM * type);
    bool isFixedExpression(EXPR * expr);
    bool isAddressable(EXPR * expr);
    EXPR * bindTypicalBinop(BASENODE * tree, EXPRKIND ek, EXPR * op1, EXPR * op2);
    void checkLvalue(EXPR * expr, bool isAssignment = true);
    void checkFieldRef(EXPRFIELD * expr, bool refParam = true);
    void checkUnsafe(BASENODE * tree, TYPESYM * type = NULL, int errCode = ERR_UnsafeNeeded);
    void markFieldAssigned(EXPR * expr);
    EXPR * bindQMark(BASENODE * tree, EXPR * op1, EXPRBINOP * op2);
    EXPR * bindStringConcat(BASENODE * tree, EXPR * op1, EXPR * op2);
    EXPR * bindNull(BASENODE * tree);
    EXPR * bindMethodName(BASENODE * tree);
    EXPR * bindArrayInit(UNOPNODE * tree, ARRAYSYM * type, EXPR * args);
    void   bindArrayInitList(UNOPNODE * tree, TYPESYM * elemType, int rank, int * size, EXPR *** ppList, int * totCount, int * constCount);
    EXPR * bindPossibleArrayInit(BASENODE * tree, TYPESYM * type, int bindFlags = 0);
    EXPR * bindPossibleArrayInitAssg(BINOPNODE * tree, TYPESYM * type, int bindFlags = 0);
    EXPR * bindConstInitializer(MEMBVARSYM * pAFSym, BASENODE * tree);
    EXPR * bindLock(LOOPSTMTNODE * tree, EXPR *** newLast);
    EXPR * bindIndexer(BASENODE * tree, EXPR * object, EXPR * args, int bindFlags);
    EXPR * bindUDArithOp(BASENODE * tree, EXPRKIND ek, EXPR * op1, EXPR * op2);
    EXPR * bindUDUnop(BASENODE * tree, EXPRKIND ek, EXPR * op1);
    EXPR * bindUDBinop(BASENODE * tree, EXPRKIND ek, EXPR * op1, EXPR * op2);
    EXPR * bindBase(BASENODE * tree);
    EXPANDEDPARAMSSYM * getParamsMethod(METHPROPSYM * meth, unsigned count);
    void noteReference(EXPR * op);
    EXPR * bindAttributeValue(PARENTSYM * parent, BASENODE * tree);
    EXPR * bindNamedAttributeValue(PARENTSYM * parent, AGGSYM *attributeClass, ATTRNODE * tree);
    EXPR * bindAttrArgs(PARENTSYM * parent, AGGSYM *attributeClass, ATTRNODE * tree, EXPR **namedArgs);
    bool verifyAttributeArg(EXPR *arg, unsigned &totalSize);
    BYTE * addAttributeArg(EXPR *arg, BYTE* buffer, BYTE* bufferEnd);
    EXPR * bindMakeRefAny(BASENODE * tree, EXPR * op);
    EXPR * bindRefType(BASENODE * tree, EXPR * op);
    EXPR * bindRefValue(BINOPNODE * tree);
    EXPR * bindPtrIndirection(UNOPNODE * tree, EXPR * op);
    EXPR * bindPtrAddr(UNOPNODE * tree, EXPR * op, int bindFlags);
    EXPR * bindLocAlloc(NEWNODE * tree, TYPESYM * type);
    EXPR * bindPtrArrayIndexer(BINOPNODE * tree, EXPR * opArr, EXPR * opIndex);
    EXPR * bindSizeOf(BASENODE * tree, TYPESYM * type);
    EXPR * bindSizeOf(UNOPNODE * tree);
    EXPR * bindForEach(FORSTMTNODE * tree, EXPR *** newLast);
    EXPR * bindForEachInner(FORSTMTNODE * tree, EXPR ** enumerator, EXPR ** initExpr);
    EXPR * bindForEachArray(FORSTMTNODE * tree, EXPR * array);
    EXPR * bindBooleanValue(BASENODE * tree, EXPR * op = NULL);
    void maybeSetOwnerOffset(LOCVARSYM * local);

    EXPR * errorBadSK(BASENODE * tree, SYM * sym, int mask);
    void errorSymName(BASENODE * tree, int id, SYM * sym, NAME * name);
    void errorNameSym(BASENODE * tree, int id, NAME * nameS, SYM * sym);
    void errorSymNameName(BASENODE * tree, int id, SYM * sym, NAME * name1, NAME * name2);
    void errorSymSym(BASENODE * tree, int id, SYM * sym1, SYM * sym2);
    void errorIntSymSym(BASENODE * tree, int id, int n, SYM * sym1, SYM * sym2);
    void errorSymInt(BASENODE * tree, int id, SYM * sym1, int n);
    void errorSym(BASENODE * tree, int id, SYM * sym);
    void errorSymSymPN(BASENODE * tree, int id, SYM * sym1, SYM * sym2, PREDEFNAME pn);
    void errorName(BASENODE * tree, int id, NAME * name);
    CError *make_errorNameInt(BASENODE * tree, int id, NAME * name, int n);
    void errorStr(BASENODE * tree, int id, LPCWSTR str);
    CError *make_errorStrSym(BASENODE *tree, int id, LPCWSTR str, PSYM sym);
    void errorStrSym(BASENODE *tree, int id, LPCWSTR str, PSYM sym);
    CError *make_errorStrSymSym(BASENODE *tree, int id, LPCWSTR str, PSYM sym1, PSYM sym2);
    void errorStrSymSym(BASENODE *tree, int id, LPCWSTR str, PSYM sym1, PSYM sym2);
    void errorSymSKSK(BASENODE * tree, int id, SYM * sym, SYMKIND sk1, SYMKIND sk2);
    void errorSymSK(BASENODE * tree, int id, SYM * sym, SYMKIND sk);
    void errorSymNameId(BASENODE * tree, int id, SYM * sym, NAME * name, int id1);
    void errorNameId(BASENODE * tree, int id, NAME * name, int id1);
    void errorN(BASENODE * tree, int id);
    CError *make_errorN(BASENODE * tree, int id);
    void warningN(BASENODE * tree, int id);
    void errorInt(BASENODE * tree, int id, int n);
    void warningSymName(BASENODE * tree, int id, SYM * sym, NAME * name);
    void warningSymNameName(BASENODE * tree, int id, SYM * sym, NAME * name1, NAME * name2);
    void warningSym(BASENODE * tree, int id, SYM * sym);
    void reportDeprecated(BASENODE * tree, SYM * sym);

    TYPESYM * getPDT(PREDEFTYPE tp);
    AGGSYM * getPDO();
    TYPESYM * getVoidType();
    PTRSYM * getVoidPtrType();

    void postBindCompile(EXPRBLOCK * expr);
    void __fastcall scanStatement(EXPR * expr);
    void __fastcall scanExpression(EXPR * expr, BITSET ** trueSet = NULL, BITSET ** falseSet = NULL, bool sense = true);
    void __fastcall markAsUsed(EXPR * expr);
    void __fastcall preinitNeeded(EXPR * expr);
    void __fastcall checkOwnerOffset(EXPRFIELD * expr);
    bool __fastcall assignToExpr(EXPR * expr, EXPR * val);
    void __fastcall addToRedoList(EXPRBLOCK * expr, int force);
    void scanRedoList(EXPRBLOCK * excludedBlock = NULL);
    void __fastcall onLabelReached(EXPR * expr, BITSET * set);
    void reportUnreferencedVars(PSCOPESYM scope);
    void checkOutParams();
    bool __fastcall onLeaveReached(EXPR * expr);
    void __fastcall visitFinallyBlock(FINALLYSTORE * store, EXPRBLOCK * block);

    void addDependancyOnType(AGGSYM *cls);

    bool realizeGoto(EXPRGOTO * expr);

    LPCWSTR opName(OPERATOR op);
    NAME * ekName(EXPRKIND ek);

    COMPILER * compiler();

    static const EXPRKIND OP2EK[OP_LAST];
    static const PREDEFNAME EK2NAME[EK_BITNOT - EK_FIRSTOP + 1];
    static const BOOL FUNCBREC::opCanBeStatement[OP_LAST];
};

//////////////////////////////////////////////////////////////////////////////////////////////

#define NODELOOP(init, child, var) \
{\
    BASENODE * _nd = init; \
    while (_nd) { \
        child ## NODE * var; \
        if (_nd->kind == NK_LIST) { \
            var = _nd->asLIST()->p1->as ## child (); \
            _nd = _nd->asLIST()->p2; \
        } else { \
            var = _nd->as ## child (); \
            _nd = NULL; \
        }

//////////////////////////////////////////////////////////////////////////////////////////////

#define EXPRLOOP(init, var) \
{\
    EXPR * _nd = init; \
    while (_nd) { \
        EXPR * var; \
        if (_nd->kind == EK_LIST) { \
            var = _nd->asBIN()->p1; \
            _nd = _nd->asBIN()->p2; \
        } else { \
            var = _nd; \
            _nd = NULL; \
        } 

//////////////////////////////////////////////////////////////////////////////////////////////

#define ENDLOOP  } }

//////////////////////////////////////////////////////////////////////////////////////////////


class BITSET {

public:
    // Use DWORD_PTRs here so everything is the same size
    class BITSETIMP {
    public:
        DWORD_PTR * arrayEnd;  // pointer to the next dword after the array ends...
        DWORD_PTR bitArray[0]; // actually longer...
    };
    DWORD_PTR bits;
    BITSET() {};

public:

    static void swap(BITSET *** bs1, BITSET *** bs2);

    unsigned __fastcall getSize();

    BITSET * __fastcall setBit(unsigned bit);
    BITSET * __fastcall setBits(unsigned start, unsigned size);
    unsigned __fastcall numberSet();
    DWORD_PTR __fastcall testBit(unsigned bit);
    DWORD_PTR __fastcall testBits(unsigned start, unsigned size);
    bool __fastcall isZero();
    BITSET * __fastcall orInto(BITSET * source);
    BITSET * __fastcall andInto(BITSET * source);
    BITSET * __fastcall andInto(BITSET * source, DWORD_PTR * changed);
    BITSET * __fastcall setInto(BITSET * source);
    BITSET * __fastcall setInto(BITSET * source, NRHEAP * alloc, DWORD_PTR * changed);
    BITSET * __fastcall setInto(BITSET * source, NRHEAP * alloc);
    BITSET * __fastcall setInto(int value);
    static BITSET * __fastcall create(unsigned size, NRHEAP * alloc, int init);
    static BITSET * __fastcall createCopy(BITSET * source, NRHEAP * alloc);
    static BITSET * __fastcall createCopy(BITSET * source, void * space);
    bool __fastcall isEqual(BITSET * other);

    // used for serialization
    unsigned __fastcall sizeInBytes();
    static BYTE *   __fastcall rawBytes(BITSET **bitset);
    static unsigned __fastcall rawSizeInBytes(unsigned numberOfBits);

private:

    static unsigned __fastcall numberSet(DWORD_PTR dw);
    BITSET * __fastcall setBitLarge(unsigned bit);
    BITSET * __fastcall setBitsLarge(unsigned start, unsigned size);
    DWORD_PTR __fastcall testBitLarge(unsigned bit);
    DWORD_PTR __fastcall testBitsLarge(unsigned start, unsigned size);
    bool __fastcall isZeroLarge();
    BITSET * __fastcall orIntoLarge(BITSET * source);
    BITSET * __fastcall andIntoLarge(BITSET * source);
    BITSET * __fastcall andIntoLarge(BITSET * source, DWORD_PTR * changed);
    BITSET * __fastcall setIntoLarge(BITSET * source, NRHEAP * alloc, DWORD_PTR * changed);
    BITSET * __fastcall setIntoLarge(BITSET * source);
    BITSET * __fastcall setIntoLarge(BITSET * source, NRHEAP * alloc);
    BITSET * __fastcall setIntoLarge(int value);
    static BITSET * __fastcall createLarge(unsigned size, NRHEAP * alloc, int init);
    static BITSET * __fastcall createCopyLarge(BITSET * source, NRHEAP * alloc);
    static BITSET * __fastcall createCopyLarge(BITSET * source, void * space);
    bool __fastcall isEqualLarge(BITSET * other);
};

//////////////////////////////////////////////////////////////////////////////////////////////

#define createBitsetOnStack(ref, source) \
{ \
    if (((DWORD_PTR)(source)) & 1) {  \
        (ref) = (source);  \
    } else { \
        unsigned size = (source)->getSize(); \
        void * space = _alloca(size); \
        (ref) = BITSET::createCopy((source), space); \
    } \
}

//////////////////////////////////////////////////////////////////////////////////////////////

#define createNewBitsetOnStack(name,size,init) \
BITSET * name; \
{ \
    if ((size) <= 31) { \
        name = (BITSET *) ((DWORD_PTR)(init) | 1); \
    }  else { \
        BITSET::BITSETIMP * rval = (BITSET::BITSETIMP*) _alloca(sizeof(BITSET::BITSETIMP) + sizeof(DWORD) * ((size >> 5) + 1)); \
        ASSERT(!(((DWORD_PTR)rval) & 1)); \
        memset(rval->bitArray, init,  ((size >> 5) + 1) * sizeof(DWORD)); \
        rval->arrayEnd = rval->bitArray + ((size >> 5) + 1); \
        name = (BITSET*) rval; \
    } \
}

//////////////////////////////////////////////////////////////////////////////////////////////

// handy class for setting/ resetting the current infile
class InfileStack
{
public:
    InfileStack(PINFILESYM *infileLocation, PINFILESYM newinfile) :
        infile(infileLocation),
        oldValue(*infileLocation)
    {
        *infileLocation = newinfile;
    }
    ~InfileStack()
    {
        *infile = oldValue;
    }
private:
    PINFILESYM *infile;
    PINFILESYM oldValue;
};
#define SETINPUTFILE(newinfile) InfileStack _infilestack(&inputfile, (newinfile));

//////////////////////////////////////////////////////////////////////////////////////////////
// Inline functions shared between metaattr.cpp and fncbind.cpp

inline CHECKEDCONTEXT::CHECKEDCONTEXT(FUNCBREC * binder, bool checked)
{
    normal = binder->checked.normal;
    constant = binder->checked.constant;

    binder->checked.normal = binder->checked.constant = checked;
}

inline void CHECKEDCONTEXT::restore(FUNCBREC * binder)
{
    binder->checked.normal = normal;
    binder->checked.constant = constant;
}

// constructor for the struct which holds the current break and continue labels...
inline LOOPLABELS::LOOPLABELS(FUNCBREC * binder)
{
    breakLabel = binder->newExprLabel();
    contLabel = binder->newExprLabel();
    binder->loopLabels = this;
}

//////////////////////////////////////////////////////////////////////////////////////////////
