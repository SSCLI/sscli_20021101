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
// File: fncbind.cpp
//
// Routines for binding the body of a function
// ===========================================================================

#include "stdafx.h"
#include "oleauto.h"

#include "compiler.h"

#include <math.h>

// handy macros:
#define RETBAD(x) if (x && !x->isOK()) return (x);

#define FAILCHECK(cond, tree) \
{ \
    if (!(cond)) { \
        errorN(tree, ERR_CheckedOverflow); \
    } \
} \


/*
 * Define the simple type conversions table.
 */

#define CONV_KIND_ID  1         // Identity conversion
#define CONV_KIND_IMP 2         // Implicit conversion
#define CONV_KIND_EXP 3         // Explicit conversion
#define CONV_KIND_MASK 0x0F     // Mask for above.
#define CONV_KIND_USEUDC 0x40   // Use an User defined operation function to make the conversion.

#define ID  CONV_KIND_ID      // shorthands for defining the table
#define IMP CONV_KIND_IMP
#define EXP CONV_KIND_EXP
#define UDC CONV_KIND_USEUDC
static const BYTE simpleTypeConversions[NUM_SIMPLE_TYPES][NUM_SIMPLE_TYPES] = {
//        to: BYTE    I2      I4      I8      FLT     DBL     DEC     CHAR    BOOL    SBYTE     U2      U4      U8
/* from */
/* BYTE */ {   ID    ,IMP    ,IMP    ,IMP    ,IMP    ,IMP    ,IMP|UDC,EXP    ,0  ,       EXP    ,IMP    ,IMP    ,IMP    },
/*   I2 */ {  EXP    ,ID     ,IMP    ,IMP    ,IMP    ,IMP    ,IMP|UDC,EXP    ,0  ,       EXP    ,EXP    ,EXP    ,EXP    },
/*   I4 */ {  EXP    ,EXP    ,ID     ,IMP    ,IMP    ,IMP    ,IMP|UDC,EXP    ,0  ,       EXP    ,EXP    ,EXP    ,EXP    },
/*   I8 */ {  EXP    ,EXP    ,EXP    ,ID     ,IMP    ,IMP    ,IMP|UDC,EXP    ,0  ,       EXP    ,EXP    ,EXP    ,EXP    },
/*  FLT */ {  EXP    ,EXP    ,EXP    ,EXP    ,ID     ,IMP    ,EXP|UDC,EXP    ,0  ,       EXP    ,EXP    ,EXP    ,EXP    },
/*  DBL */ {  EXP    ,EXP    ,EXP    ,EXP    ,EXP    ,ID     ,EXP|UDC,EXP    ,0  ,       EXP    ,EXP    ,EXP    ,EXP    },
/*  DEC */ {  EXP|UDC,EXP|UDC,EXP|UDC,EXP|UDC,EXP|UDC,EXP|UDC,ID     ,EXP|UDC,0  ,       EXP|UDC,EXP|UDC,EXP|UDC,EXP|UDC},
/* CHAR */ {  EXP    ,EXP    ,IMP    ,IMP    ,IMP    ,IMP    ,IMP|UDC,ID     ,0  ,       EXP    ,IMP    ,IMP    ,IMP    },
/* BOOL */ {    0    ,0      ,0      ,0      ,0      ,0      ,0      ,0      ,ID ,       0      ,0      ,0      ,0      },
/*SBYTE */ {  EXP    ,IMP    ,IMP    ,IMP    ,IMP    ,IMP    ,IMP|UDC,EXP    ,0  ,       ID     ,EXP    ,EXP    ,EXP    },
/*   U2 */ {  EXP    ,EXP    ,IMP    ,IMP    ,IMP    ,IMP    ,IMP|UDC,EXP    ,0  ,       EXP    ,ID     ,IMP    ,IMP    },
/*   U4 */ {  EXP    ,EXP    ,EXP    ,IMP    ,IMP    ,IMP    ,IMP|UDC,EXP    ,0  ,       EXP    ,EXP    ,ID     ,IMP    },
/*   U8 */ {  EXP    ,EXP    ,EXP    ,EXP    ,IMP    ,IMP    ,IMP|UDC,EXP    ,0  ,       EXP    ,EXP    ,EXP    ,ID     },
};
#undef ID
#undef IMP
#undef EXP
#undef UDC



// Return the predefined void type
__forceinline TYPESYM * FUNCBREC::getVoidType()
{
    return compiler()->symmgr.GetVoid();
}

__forceinline PTRSYM * FUNCBREC::getVoidPtrType()
{
    return compiler()->symmgr.GetPtrType(getVoidType());
}


// return the given predefined type (including void)
__forceinline TYPESYM * FUNCBREC::getPDT(PREDEFTYPE tp)
{
    if (tp == PT_VOID) return compiler()->symmgr.GetVoid();
    return compiler()->symmgr.GetPredefType(tp, true);
}


__forceinline AGGSYM * FUNCBREC::getPDO()
{
    return compiler()->symmgr.GetObject();
}

__forceinline void FUNCBREC::addDependancyOnType(AGGSYM *cls)
{
    compiler()->EnsureTypeIsDefined(cls);
    compiler()->MakeFileDependOnType(this->pParent, cls);
}

// Constructor, this gets called only at compiler init time...
FUNCBREC::FUNCBREC()
{
    pOuterScope = NULL;
    pCurrentScope = NULL;
    pParent = NULL;
    thisPointer = NULL;
    inFieldInitializer = false;
    dontAddNamesToCache = false;
    allocator = &(compiler()->localSymAlloc);
    inputfile = NULL;
    pFSym = NULL;
    pMSym = NULL;
    info = NULL;
}

// Error displaying functions.
// Each takes a parseTree for the position, an error id, and
// more items which are indicated in the name of the function.
void FUNCBREC::errorSymName(BASENODE *tree, int id, SYM *sym, NAME* name)
{
    LPWSTR szSym = compiler()->ErrSym(sym);
    LPCWSTR szName = compiler()->ErrName(name);
    compiler()->Error(ERRLOC(inputfile, tree), id, szSym, szName);
}

void FUNCBREC::errorNameSym(BASENODE *tree, int id,  NAME* name, SYM *sym)
{
    LPCWSTR szName = compiler()->ErrName(name);
    LPWSTR szSym = compiler()->ErrSym(sym);
    compiler()->Error(ERRLOC(inputfile, tree), id, szName, szSym);
}

void FUNCBREC::errorSymSymPN(BASENODE * tree, int id, SYM * sym1, SYM * sym2, PREDEFNAME pn)
{
    LPWSTR szSym1 = compiler()->ErrSym(sym1);
    LPWSTR szSym2 = compiler()->ErrSym(sym2);
    LPCWSTR szName = compiler()->ErrName(compiler()->namemgr->GetPredefName(pn));
    compiler()->Error(ERRLOC(inputfile, tree), id, szSym1, szSym2, szName);
}

void FUNCBREC::errorSymNameName(BASENODE *tree, int id, SYM *sym, NAME* name1, NAME* name2)
{
    LPWSTR szSym = compiler()->ErrSym(sym);
    LPCWSTR szName1 = compiler()->ErrName(name1);
    LPCWSTR szName2 = compiler()->ErrName(name2);
    compiler()->Error(ERRLOC(inputfile, tree), id, szSym, szName1, szName2);
}

void FUNCBREC::errorSymNameId(BASENODE *tree, int id, SYM *sym, NAME* name, int id1)
{
    LPWSTR szSym = compiler()->ErrSym(sym);
    LPCWSTR szName = compiler()->ErrName(name);
    LPWSTR szId = compiler()->ErrId(id1);
    compiler()->Error(ERRLOC(inputfile, tree), id, szSym, szName, szId);
}

void FUNCBREC::errorNameId(BASENODE *tree, int id, NAME* name, int id1)
{
    LPCWSTR szName = compiler()->ErrName(name);
    LPWSTR szId = compiler()->ErrId(id1);
    compiler()->Error(ERRLOC(inputfile, tree), id, szName, szId);
}

CError * FUNCBREC::make_errorNameInt(BASENODE *tree, int id, NAME* name, int n)
{
    LPCWSTR szName = compiler()->ErrName(name);
    CError  *pError = compiler()->MakeError (id, szName, n);
    compiler()->AddLocationToError (pError, ERRLOC(inputfile, tree));
    return pError;
}

CError * FUNCBREC::make_errorN(BASENODE *tree, int id)
{
    CError  *pError = compiler()->MakeError (id);
    compiler()->AddLocationToError (pError, ERRLOC(inputfile, tree));
    return pError;
}

void FUNCBREC::errorName(BASENODE *tree, int id, NAME* name)
{
    LPCWSTR szName = compiler()->ErrName(name);
    compiler()->Error(ERRLOC(inputfile, tree), id, szName);
}

void FUNCBREC::errorSym(BASENODE *tree, int id, SYM *sym)
{
    LPWSTR szSym = compiler()->ErrSym(sym);
    compiler()->Error(ERRLOC(inputfile, tree), id, szSym);
}

void FUNCBREC::errorStr(BASENODE *tree, int id, LPCWSTR str)
{
    compiler()->Error(ERRLOC(inputfile, tree), id, str);
}

CError *FUNCBREC::make_errorStrSym(BASENODE *tree, int id, LPCWSTR str, PSYM sym)
{
    LPWSTR  szSym = compiler()->ErrSym(sym);
    CError  *pError = compiler()->MakeError (id, str, szSym);
    compiler()->AddLocationToError (pError, ERRLOC(inputfile, tree));
    return pError;
}

void FUNCBREC::errorStrSym(BASENODE *tree, int id, LPCWSTR str, PSYM sym)
{
    compiler()->SubmitError (make_errorStrSym (tree, id, str, sym));
}

CError *FUNCBREC::make_errorStrSymSym(BASENODE *tree, int id, LPCWSTR str, PSYM sym1, PSYM sym2)
{
    LPWSTR szSym1 = compiler()->ErrSym(sym1);
    LPWSTR szSym2 = compiler()->ErrSym(sym2);
    CError  *pError = compiler()->MakeError (id, str, szSym1, szSym2);
    compiler()->AddLocationToError (pError, ERRLOC(inputfile, tree));
    return pError;
}

void FUNCBREC::errorStrSymSym(BASENODE *tree, int id, LPCWSTR str, PSYM sym1, PSYM sym2)
{
    compiler()->SubmitError (make_errorStrSymSym (tree, id, str, sym1, sym2));
}

void FUNCBREC::errorSymSym(BASENODE *tree, int id, SYM *sym1, SYM *sym2)
{
    LPWSTR szSym1 = compiler()->ErrSym(sym1);
    LPWSTR szSym2 = compiler()->ErrSym(sym2);
    compiler()->Error(ERRLOC(inputfile, tree), id, szSym1, szSym2);
}

void FUNCBREC::errorIntSymSym(BASENODE *tree, int id, int n, SYM *sym1, SYM *sym2)
{
    LPWSTR szSym1 = compiler()->ErrSym(sym1);
    LPWSTR szSym2 = compiler()->ErrSym(sym2);
    compiler()->Error(ERRLOC(inputfile, tree), id, n, szSym1, szSym2);
}

void FUNCBREC::errorSymInt(BASENODE *tree, int id, SYM *sym1, int n)
{
    LPWSTR szSym1 = compiler()->ErrSym(sym1);
    compiler()->Error(ERRLOC(inputfile, tree), id, szSym1, n);
}

void FUNCBREC::errorSymSKSK(BASENODE * tree, int id, SYM * sym, SYMKIND sk1, SYMKIND sk2)
{
    LPWSTR szSym = compiler()->ErrSym(sym);
    LPWSTR szSK1 = compiler()->ErrSK(sk1);
    LPWSTR szSK2 = compiler()->ErrSK(sk2);
    compiler()->Error(ERRLOC(inputfile, tree), id, szSym, szSK1, szSK2);

}

void FUNCBREC::errorSymSK(BASENODE * tree, int id, SYM * sym, SYMKIND sk)
{
    LPWSTR szSym = compiler()->ErrSym(sym);
    LPWSTR szSK = compiler()->ErrSK(sk);
    compiler()->Error(ERRLOC(inputfile, tree), id, szSym, szSK);

}

// This ones just displays the error w/o any additional info...
void FUNCBREC::errorN(BASENODE *tree, int id)
{
    compiler()->Error(ERRLOC(inputfile, tree), id);
}

void FUNCBREC::warningN(BASENODE *tree, int id)
{
    compiler()->Error(ERRLOC(inputfile, tree), id);
}

// Error with a single integer argument.
void FUNCBREC::errorInt(BASENODE * tree, int id, int n)
{
    compiler()->Error(ERRLOC(inputfile, tree), id, n);
}

void FUNCBREC::warningSymName(BASENODE * tree, int id, SYM * sym, NAME * name)
{
    LPWSTR szSym = compiler()->ErrSym(sym);
    LPCWSTR szName = compiler()->ErrName(name);
    compiler()->Error(ERRLOC(inputfile, tree), id, szSym, szName);
}

void FUNCBREC::warningSymNameName(BASENODE *tree, int id, SYM *sym, NAME* name1, NAME* name2)
{
    LPWSTR szSym = compiler()->ErrSym(sym);
    LPCWSTR szName1 = compiler()->ErrName(name1);
    LPCWSTR szName2 = compiler()->ErrName(name2);
    compiler()->Error(ERRLOC(inputfile, tree), id, szSym, szName1, szName2);
}

void FUNCBREC::warningSym(BASENODE * tree, int id, SYM * sym)
{
    LPWSTR szSym = compiler()->ErrSym(sym);
    compiler()->Error(ERRLOC(inputfile, tree), id, szSym);
}

// Report a deprecation error on a symbol.
void FUNCBREC::reportDeprecated(BASENODE * tree, SYM * sym)
{
    ASSERT(sym->isDeprecated);

    if (!pMSym || !pMSym->isDeprecated) {
        compiler()->clsDeclRec.reportDeprecated(tree, pParent, sym);
    }
}


// compile a method.  pAMNode can point to a METHODNODE, a CTORNODE, or a
// CLASSNODE if compiling a synthetized constructor...
EXPR * FUNCBREC::compileMethod(METHSYM * pAMSym, BASENODE * pAMNode, METHINFO * info, AGGINFO * classInfo)
{
    TimerStart(TIME_BINDMETHOD);

    SETINPUTFILE(pAMSym->getInputFile());

#if DEBUG
    {
        if ((pAMSym->name && COMPILER::IsRegString(pAMSym->name->text, L"Method"))) {
            if (COMPILER::IsRegString(pAMSym->parent->name->text, L"Class")) {
                BASSERT(!"Compilation breakpoint hit");
            }
        }
    }
#endif

    //
    // give correct unsafe errors for methods with no statements
    //
    unsafeErrorGiven = false;

    pMSym = pAMSym;
    pFSym = NULL;
    btfFlags = pMSym->isContainedInDeprecated() ? BTF_NODEPRECATED : 0;
    ASSERT(pAMNode->kind == NK_CTOR || pAMNode->kind == NK_METHOD ||
           pAMNode->kind == NK_CLASS || pAMNode->kind == NK_STRUCT ||
           pAMNode->kind == NK_ACCESSOR || pAMNode->kind == NK_OPERATOR ||
           pAMNode->kind == NK_DTOR || pAMNode->kind == NK_VARDECL || pAMNode->kind == NK_PROPERTY);
    ASSERT(pAMNode->kind != NK_CLASS || (pAMSym->isCtor || pAMSym->isIfaceImpl));
    ASSERT((pAMNode->kind != NK_VARDECL && pAMNode->kind != NK_PROPERTY) || pAMSym->isEventAccessor);
    pTree = pAMNode;

    if (pParent != pMSym->parent || classInfo != pClassInfo) {
        initClass(classInfo);
    }

    this->info = info;
    unsafeContext = info->isUnsafe;
    
    checked.normal = compiler()->GetCheckedMode();
    checked.constant = true; 

    errorsBeforeBind = compiler()->ErrorCount();

    EXPR * rval;

    if (pAMSym->isCtor) {
        rval = bindConstructor(info);
    } else if (pAMSym->isDtor) {
        rval = bindDestructor(info);
    } else if (pAMSym->isPropertyAccessor) {
        rval = bindProperty(info);
    } else if (pAMSym->isEventAccessor) {
        rval = bindEventAccessor(info);
    } else if (pAMSym->isIfaceImpl) {
        rval = bindIfaceImpl(info);
    } else {
        rval = bindMethod(info);
    }

    if (errorsBeforeBind == compiler()->ErrorCount() && rval != NULL) {
        SETLOCATIONSTAGE(SCAN);
        postBindCompile(rval->asBLOCK());
    }

    // Don't check property/event accessors because we check the property itself.
    if (compiler()->CheckForCLS() && pAMSym->checkForCLS() && pAMSym->hasExternalAccess() && !pAMSym->isPropertyAccessor && !pAMSym->isEventAccessor) {
        if (pAMSym->isVarargs)
            warningN( pAMSym->getParseTree(), ERR_CLS_NoVarArgs);
        else if (pAMSym->params) {
            for (int i = 0; i < pAMSym->cParams; i++) {
                if (!pAMSym->params[i]->isCLS_Type()) {
                    warningSym( pAMSym->getParamParseTree(i), ERR_CLS_BadArgType, pAMSym->params[i]);
                }
            }
        }
        if (pAMSym->retType && !pAMSym->retType->isCLS_Type()) {
            warningSym( pAMSym->getParseTree()->asANYMETHOD()->pType, ERR_CLS_BadReturnType, pAMSym);
        }
    }

    this->info = NULL;

    TimerStop(TIME_BINDMETHOD);

    return rval;
}

// Bind a const initializer. The value of the initializer must
// be implicitly converted to the constant type.
EXPR * FUNCBREC::bindConstInitializer(MEMBVARSYM * pAFSym, BASENODE * tree)
{

    CHECKEDCONTEXT checked(this, compiler()->GetCheckedMode());
    this->checked.constant = true;

    // Just bind the expression...
    EXPR * rval = bindExpr(tree);

    checked.restore(this);

    // Convert to the correct destination type
    if (pAFSym->parent->asAGGSYM()->isEnum) {
        ASSERT(pAFSym->type == pAFSym->parent);  // enumerator must be of correct type.

        // Enumerator fields are handled somewhat special. The initializer
        // must be convertable to the *underlying* type of the enum.
        rval = mustConvert(rval, pAFSym->type->asAGGSYM()->underlyingType, tree);
    }
    else {
        // Regular const field.
        rval = mustConvert(rval, pAFSym->type, tree);
    }

    return rval;
}

// called to compile the value of a constant field.  tree here points to
// the expression being assigned to the field...
EXPR * FUNCBREC::compileFirstField(MEMBVARSYM * pAFSym, BASENODE * tree)
{
    TimerStart(TIME_BINDMETHOD);

    SETINPUTFILE(pAFSym->getInputFile());

    MEMBVARSYM * pFOrigSymOld = pFOrigSym;
    MEMBVARSYM * pFSymOld = pFSym;

    pMSym = NULL;
    pOuterScope = NULL;
    ASSERT(pAFSym->isUnevaled);
    pFOrigSym = pFSym = pAFSym;
    btfFlags = pFSym->isContainedInDeprecated() ? BTF_NODEPRECATED : 0;
    // This constant field is a different one than the one we are doing
    // currently, and so it may be from a different class...
    if (pParent != pFSym->parent) {
        initClass();
    }

    // Just bind the initializer...
    EXPR * rval = bindConstInitializer(pAFSym, tree);

    pFOrigSym = pFOrigSymOld;
    pFSym = pFSymOld;
    btfFlags = pFSym->isContainedInDeprecated() ? BTF_NODEPRECATED : 0;

    TimerStop(TIME_BINDMETHOD);
    return rval;
}

// called to compile a constant field which was referenced during the compilation
// of another constant field...
EXPR * FUNCBREC::compileNextField(MEMBVARSYM * pAFSym, BASENODE * tree)
{
    TimerStart(TIME_BINDMETHOD);

    SETINPUTFILE(pAFSym->getInputFile());

    initNextField(pAFSym);

    // Just bind the initializer...
    EXPR * rval = bindConstInitializer(pAFSym, tree);

    TimerStop(TIME_BINDMETHOD);
    return rval;
}

// Initialize the expr generator for the eval of a field
// constant, if another constant is being evaled already...
void FUNCBREC::initNextField(MEMBVARSYM * pAFSym)
{
    ASSERT(!pMSym);
    pFSym = pAFSym;
    btfFlags = pFSym->isContainedInDeprecated() ? BTF_NODEPRECATED : 0;
    if (pFSym && pParent != pFSym->parent) {
        initClass();
    }
}

// Call if compiling from a different class than the last one
void FUNCBREC::initClass(AGGINFO * info)
{
    BASENODE * bn;
    if (pMSym) {
        pParent = pMSym->parent->asAGGSYM();
        ASSERT(pTree->kind == NK_CLASS || pTree->kind == NK_STRUCT || 
               pTree->kind == NK_METHOD || pTree->kind == NK_CTOR ||
               pTree->kind == NK_ACCESSOR || pTree->kind == NK_OPERATOR ||
               pTree->kind == NK_DTOR  || pTree->kind == NK_VARDECL || pTree->kind == NK_PROPERTY);
        bn = pTree;
        pClassInfo = info;
        btfFlags = pMSym->isContainedInDeprecated() ? BTF_NODEPRECATED : 0;
    } else if (pFSym) {
        pParent = pFSym->parent->asAGGSYM();
        bn = pFSym->parseTree;
        pClassInfo = NULL;
        btfFlags = pFSym->isContainedInDeprecated() ? BTF_NODEPRECATED : 0;
    } else {
        bn = pParent->getParseTree();
        pClassInfo = NULL;
        btfFlags = pParent->isContainedInDeprecated() ? BTF_NODEPRECATED : 0;
    }
    while (bn->kind != NK_NAMESPACE) {
        bn = bn->pParent;
    }
    nmTree = bn->asNAMESPACE();
}


// Declare the variable of a given name and type in the current scope.
LOCVARSYM * FUNCBREC::declareVar(BASENODE * tree, NAME * ident, TYPESYM * type) 
{

    checkUnsafe(tree, type);

    SYM * rval = compiler()->symmgr.LookupLocalSym(ident, pCurrentScope, MASK_LOCVARSYM);
    if (rval) {
        errorName(tree, ERR_LocalDuplicate, ident);
        return NULL;
    }

    localCount ++;

    if (localCount > 0xffff) {
        errorN(tree, ERR_TooManyLocals);
    }

    rval = compiler()->symmgr.CreateLocalSym(SK_LOCVARSYM, ident, pCurrentScope);
    rval->asLOCVARSYM()->type = type;
    rval->asLOCVARSYM()->declTree = tree;

    storeInCache(tree, ident, rval, true);

    return rval->asLOCVARSYM();
}



// Declare the given parameter in the outermost scope.
// Checks for ducplicate parameter names.
inline LOCVARSYM * FUNCBREC::declareParam(NAME *ident, TYPESYM *type, unsigned flags, BASENODE *parseTree)
{
    ASSERT(type);

    checkUnsafe(parseTree, type);

    LOCVARSYM * rval = compiler()->symmgr.LookupLocalSym(ident, pOuterScope , MASK_LOCVARSYM)->asLOCVARSYM();
    if (rval) {
        errorName(parseTree, ERR_DuplicateParamName, ident);
        return NULL;
    }

    return addParam(ident, type, flags);
}

inline LOCVARSYM * FUNCBREC::declareParam(PARAMETERNODE * param)
{
    TYPESYM * type = bindType(param->pType);
    if (!type) return NULL;

    return declareParam(param->pName->pName, type, param->flags, param);
}

// adds a parameter to the outermost scope
// does not do duplicate parameter error checking
LOCVARSYM * FUNCBREC::addParam(PNAME ident, TYPESYM *type, unsigned paramFlags)
{
    LOCVARSYM * rval = compiler()->symmgr.CreateLocalSym(SK_LOCVARSYM, ident, pOuterScope)->asLOCVARSYM();
    rval->slot.hasInit = true;
    rval->slot.isParam = true;
    rval->slot.isReferenced = true;     // Set this so we never report unreferenced parameters
    if (paramFlags & (NF_PARMMOD_REF | NF_PARMMOD_OUT)) {
        rval->slot.isRefParam = true;
        rval->slot.isUsed = true;
        rval->slot.isPBUsed = true;
        if (paramFlags & NF_PARMMOD_OUT) {
            outParamCount ++;
            rval->slot.hasInit = false;
            rval->slot.ilSlot = uninitedVarCount + 1;
            uninitedVarCount += type->getFieldsSize();
        }
    }
    rval->type = type;
    return rval;
}


// Bind the given type assuming the current namespace and class context
// The type is declared.
TYPESYM * FUNCBREC::bindType(TYPENODE * tree)
{
    PTYPESYM type;
    type = compiler()->clsDeclRec.bindType(tree, pParent, BTF_NONE | btfFlags, NULL);
    if (type) {
        compiler()->symmgr.DeclareType(type);
        if (type->kind == SK_PTRSYM) {
            if (isManagedType(type->asPTRSYM()->parent->asTYPESYM())) {
                errorSym(tree, ERR_ManagedAddr, type->parent);
            }
        }
    }
    return type;
}



// Lookup the method or field in this and all base classes.
// If tryInaccessible is true, inaccessible names are found also.
SYM * FUNCBREC::lookupClassMember(NAME* name, AGGSYM * cls, bool bBaseCall, bool tryInaccessible, bool ignoreSpecialMethods, int forceMask)
{
    SYM* rval = NULL;

    if (cls->isInterface) {
        PSYMLIST list = cls->allIfaceList;
        do
        {
            addDependancyOnType(cls);
            if (tryInaccessible)
                rval = compiler()->clsDeclRec.findNextName(name, cls, rval);
            else
                rval = compiler()->clsDeclRec.findNextAccessibleName(name, cls, pParent, rval, false, ignoreSpecialMethods);

            if (rval) {
                if (!forceMask || (rval->mask() & forceMask)) {
                    return rval;
                } else {
                    rval = NULL;
                }
            }

            if (list) {
                cls = list->sym->asAGGSYM();
                list = list->next;
            } else {
                cls = NULL;
            }
        } while (cls);

        // maybe its a method on object...
        cls = getPDT(PT_OBJECT)->asAGGSYM();
    }

    do {
        addDependancyOnType(cls);
        if (tryInaccessible) 
            rval = compiler()->clsDeclRec.findNextName(name, cls, rval);
        else
            rval = compiler()->clsDeclRec.findNextAccessibleName(name, cls, pParent, rval, bBaseCall, ignoreSpecialMethods);

        if (rval)
        {
            cls = rval->parent->asAGGSYM();
            bBaseCall = true;
        }

        // don't return override members
    } while (rval && ((forceMask && !(rval->mask() & forceMask)) ||
          (((rval->kind == SK_METHSYM) || (rval->kind == SK_PROPSYM)) && 
           !isMethPropCallable(rval->asMETHPROPSYM(), false))));
            
    return rval;
}


// Store the given symbol in the local cache of symbols.
// Check for parent indicates whether to check that no cache entry exists which
// denotes a symbol bound in a parent scope, which we would be rebinding now (which
// is illegal)  It is possible to optimize away this check if we know that
// no symbol was found which was visible in our scope, since visiblity would
// imply that it had to be bound in a parent scope.
// We know the absence of this visibility if a cache lookup for the symbol
// failed, which is what happens when this function is called from bindName(...)
bool FUNCBREC::storeInCache(BASENODE * tree, NAME * name, SYM * sym, bool checkForParent)
{
    if (!pOuterScope || dontAddNamesToCache) return true;

    CACHESYM * cacheEntry;
    SYM * prev = compiler()->symmgr.LookupLocalSym(name, pOuterScope, MASK_CACHESYM | MASK_LOCVARSYM);
    if (prev) {
        int id;

        // If we have a previous cache entry, then the current one might clash and 
        // not be valid, so we need to check:
        if (prev->kind == SK_LOCVARSYM) {
            // trying to rebind the name of a param, this is ALWAYS a nono:
            id = IDS_PARENT;
            goto DISPERROR;
        }
        cacheEntry = prev->asCACHESYM();

        prev = cacheEntry->sym;

        // if the cached name already denotes the same symbol, there can be no conflict...
        if (prev == sym) {

            // Note that it is impossible at this point for sym's scope to be a 
            // child of prev's scope.  This is because if it were so then prev would
            // be visible in this scope and would have been found on a cache lookup,
            // since this is only called on a loopup failure, except in the case of
            // Local decls.  However, in the case of local decls it is impossible for
            // the decl to be already in the cache since it is being declared.
            ASSERT(!checkForParent);
            cacheEntry->scope = pCurrentScope;
            return true;
        }

        // first check whether previous gives a meaning to the name in our
        // current scope...  this means that there exists a parent/child
        // relationship between previous' scope and the current scope...

        SCOPESYM * current;
        if (checkForParent
#if DEBUG
            || true
#endif
            ) {
            // Does the previous definition occur in a parent scope the thereby
            // contribute a meaning to our scope?
            current = pCurrentScope;
            unsigned prevOrdinal = cacheEntry->scope->nestingOrder;
            ASSERT(prevOrdinal > 0);
            // we only need to check up to the same tree height, obviously a 
            // parent of our scope has to have a lower ordinal
            while (current->nestingOrder >= prevOrdinal) {
                if (current == cacheEntry->scope) {
                    // ok, the previous definition is visible in this scope, and so cannot
                    // be overriden:
                    id = IDS_PARENT;
                    ASSERT(checkForParent);
                    // The failure of the above assert means sombody called this
                    // w/ checkForParent turned off w/o doing a cache lookup
                    // on this name first...
                    goto DISPERROR;
                }
                current = current->parent->asSCOPESYM();
            }
        }

        // Does the previous definition occur in a child scope of the current one?
        current = cacheEntry->scope;
        // Obviously, pCurrentScope->nestingOrder is always at least 1 since a 0
        // would imply arguments scope. So, the loop will break before current
        // becomes NULL...
        while (current->nestingOrder >= pCurrentScope->nestingOrder) {
            if (current == pCurrentScope) {
                // ok, the current definition would override a previous one and be
                // visible in the scope of the previous one, so in that scope
                // the name could have two possible meanings... this is also
                // illegal...
                id = IDS_CHILD;
                goto DISPERROR;
            }
            current = current->parent->asSCOPESYM();
        }

        // ok, the definitions would not clash, so we just need to rebind the 
        // cache entry:

        // [The reason this works is that we assume that we bind the parsetree
        //  in lexical order, so that all child scopes are bound before doing 
        //  any code in parent scopes that follows the child scope in question...]

        goto MAKENEWBINDING;

DISPERROR:
        if (sym->kind == SK_LOCVARSYM) {
            errorNameId(tree, ERR_LocalIllegallyOverrides, name, id);
        } else {
            errorSymNameId(tree, ERR_NameIllegallyOverrides, sym, name, id);
        }

        return false;
    
    } else { // a cache miss:

        cacheEntry = compiler()->symmgr.CreateLocalSym(SK_CACHESYM, name, pOuterScope)->asCACHESYM();

MAKENEWBINDING:
        cacheEntry->sym = sym;
        cacheEntry->scope = pCurrentScope;

        return true;
    }

}


METHSYM * FUNCBREC::findMethod(BASENODE * tree, PREDEFNAME pn, AGGSYM * cls, TYPESYM *** params, bool reportError)
{
    NAME * name = compiler()->namemgr->GetPredefName(pn);

    SYM * rval = lookupClassMember(name, cls, false, false, false);
    if (!rval || rval->kind != SK_METHSYM) {
LERROR:
        if (reportError) errorSymName(tree, ERR_NoAccessibleMember, cls, name);
        return NULL;
    }
    if (!params) {
        return rval->asMETHSYM();
    }

    while (rval->asMETHSYM()->params != *params) {
        rval = compiler()->symmgr.LookupNextSym(rval, rval->parent, MASK_METHSYM);
        if (!rval) goto LERROR;
    }
    return rval->asMETHSYM();
}

// See if the given name is already bound in the current scope...
// Returns either the sym, if its of a correct type, 
// NULL, if nothing found, or symError if found but wrong type
SYM * FUNCBREC::lookupCachedName(NAMENODE * name, int mask)
{
    if (!pOuterScope) return NULL;

    SYM * rval = compiler()->symmgr.LookupLocalSym(
        name->pName, 
        pOuterScope, 
        (mask & MASK_LOCVARSYM) | MASK_CACHESYM);

    if (!rval) return NULL;

    if (rval->kind == SK_LOCVARSYM) { // this is a param...
        goto FOUNDVISIBLE;
    }

    SCOPESYM * cacheScope;
    cacheScope = rval->asCACHESYM()->scope;
    rval = rval->asCACHESYM()->sym;

    // Check that the definition is visible in our scope: 
    // (ie, it occurs in our parent, or in our scope)

    SCOPESYM * current;
    current = pCurrentScope;
    unsigned rvalOrdinal;
    rvalOrdinal = cacheScope->nestingOrder;

    ASSERT(rvalOrdinal > 0);
    // Note that the ordinal of rval is at least 1, since a 0 would imply the
    // outer scope and only params are found there, which we already checked
    // above.  So, a current ordinal of 0 will break this loop before
    // current becomes null.
    while (current->nestingOrder >= rvalOrdinal) {
        if (current == cacheScope) {
FOUNDVISIBLE:
            // ok, this definition is visible and defines the meaning for the name
            // is it of the right type?
            if (rval->mask() & mask) {
                return rval;
            } else {
                errorBadSK(name, rval, mask);

                return compiler()->symmgr.GetErrorSym();
            }
        }
        current = current->parent->asSCOPESYM();
    }

    // no parent relation, or no visible definition:
    return NULL;

}

// Display an error based on the fact that the given symbol is not of the
// expected kind...
EXPR * FUNCBREC::errorBadSK(BASENODE * tree, SYM * sym, int mask)
{
    ASSERT(!(sym->mask() & mask));

    SYMKIND expected;
    switch (mask) {
    case MASK_METHSYM: expected = SK_METHSYM; break;
    case MASK_MEMBVARSYM: expected = SK_MEMBVARSYM; break;
    case MASK_AGGSYM: expected = SK_AGGSYM; break;

    case MASK_LOCVARSYM | MASK_MEMBVARSYM | MASK_PROPSYM: 
    case MASK_LOCVARSYM | MASK_MEMBVARSYM:
    case MASK_MEMBVARSYM | MASK_PROPSYM:
        expected = SK_LOCVARSYM; break;

    default: expected = (SYMKIND)0;
    }
    if (sym->kind == SK_EVENTSYM && (mask & MASK_MEMBVARSYM)) {
        if (sym->asEVENTSYM()->implementation)
            errorSymSym(tree, ERR_BadEventUsage, sym, sym->parent);
        else
            errorSym(tree, ERR_BadEventUsageNoField, sym);
    }   
    else if (expected) {
        if (sym->kind == SK_METHSYM && (expected == SK_LOCVARSYM || expected == SK_MEMBVARSYM))
            errorSym(tree, ERR_BadUseOfMethod, sym);
        else
            errorSymSKSK(tree, ERR_BadSKknown, sym, sym->kind, expected);
    } else {
        errorSymSK(tree, ERR_BadSKunknown, sym, sym->kind);
    }

    // Return an error node, possible with additional information.
    EXPRERROR * expr = newError(tree);
    if (expected == SK_LOCVARSYM && sym->kind == SK_AGGSYM &&
        (sym->asAGGSYM()->isEnum || sym->asAGGSYM()->isNumericType() || sym->asAGGSYM()->isPredefType(PT_OBJECT)))
    {
        // This could be a case where the user wrote (E)-1 instead of (E)(-1) where E is an
        // type that a negative number could be cast to. If it is of this form, report
        // an additional hint to the user.
        expr->extraInfo = ERROREXTRA_MAYBECONFUSEDNEGATIVECAST;
    }

    return expr;
}


// try to bind to a constant field.
// returns true on success.
bool FUNCBREC::bindUnevaledConstantField(MEMBVARSYM * field)
{
    ASSERT(field->isUnevaled);
    ASSERT(!pMSym);

    MEMBVARSYM * currentField = pFSym;

    bool rval = compiler()->clsDeclRec.evaluateFieldConstant(pFOrigSym, field);
    // the above might fail, in which case we will bind to a non-constant
    // expression, and evantually return such, which will cause us to 
    // give an approporiate error...

    // restore the previous field we were binding
    initNextField(currentField);

    return rval;
}

// this determines whether the expression as an object of a prop or field is an lvalue
bool __fastcall FUNCBREC::objectIsLvalue(EXPR * object)
{

    return (
        !object || // statics are always lvalues

        (object->kind == EK_LOCAL && object->asLOCAL()->local == thisPointer) || // the this pointer's fields or props are lvalues

        ((object->flags & EXF_LVALUE) && (object->kind != EK_PROP)) ||
        // things marked as lvalues have props/fields which are lvalues, with one exception:  props of structs
        // do not have fields/structs as lvalues

        !object->type->isStructType()
        // non-struct types are lvalues (such as non-struct method returns)
        );

}

void __fastcall FUNCBREC::checkStaticness(BASENODE * tree, SYM * member, EXPR ** object)
{
    bool isStatic;
    
    switch (member->kind) {
    case SK_MEMBVARSYM:
        isStatic = member->asMEMBVARSYM()->isStatic; break;
    case SK_EVENTSYM:
        isStatic = member->asEVENTSYM()->isStatic; break;
    default:
        isStatic = member->asMETHPROPSYM()->isStatic; break;
    }

    if (isStatic) {
        if (*object && ((*object)->kind != EK_LOCAL || !((*object)->flags & EXF_IMPLICITTHIS))) {
            // Now, this could still be ok if we are in the wierd E.M case where
            // E can be either the class name or the prop.
            if ((*object)->flags & EXF_NAMEEXPR) {
                if ((*object)->type->name == (*object)->tree->asNAME()->pName) {
                    SYM * type = compiler()->clsDeclRec.bindSingleTypeName((*object)->tree->asNAME(), pParent, BTF_NONE, NULL);
                    if (type && type == (*object)->type) {
                        goto NOOBJECT;
                    }
                }
            }
            errorSym(tree, ERR_ObjectProhibited, member);
        }
NOOBJECT:
        *object = NULL;
    } else {  // !isStatic
        if (member->kind != SK_METHSYM || !member->asMETHSYM()->isCtor) {
            if (!*object) {
                if (inFieldInitializer && !pMSym->isStatic && pParent == member->parent) 
                    errorSym(tree, ERR_FieldInitRefNonstatic, member); // give better error message for common mistake                                  
                else
                    errorSym(tree, ERR_ObjectRequired, member);
            } else {
                // This check ensures that we do not bind to methods in an outerclass
                // which are visible, but whose this pointer is of an incorrect
                // type...
                // NOTE:  this will fail for enums because there is no implicit
                // conversion from the enum to the method on the enum, so let's catch that first...
                if ((*object)->type->kind != SK_AGGSYM || !(*object)->type->asAGGSYM()->isEnum) {
                    EXPR * newObject = tryConvert(*object, member->parent->asAGGSYM(), tree, CONVERT_NOUDC);
                    if (! newObject) 
                        errorSymSym(tree, ERR_WrongNestedThis, member->parent, (*object)->type);
                    *object = newObject;
                }
            }
        }
    }
}

void FUNCBREC::verifyMethodArgs(EXPR * call)
{
    EXPR **argsPtr = call->getArgsPtr();

    if (!argsPtr) return;

    METHPROPSYM * mp = call->getMember()->asMETHPROPSYM();
    int paramCount = mp->cParams;
    TYPESYM ** destType = mp->params;
    bool markTypeFromExternCall = (mp->kind == SK_METHSYM ? mp->asMETHSYM()->isExternal : false);

    if (mp->isVarargs) {
        paramCount--; // we don't care about the vararg sentinel
    }

    bool argListSeen = false;

    EXPR ** indir = NULL;
    
    if (!*argsPtr) {
        if (mp->isParamArray) goto FIXUPPARAMLIST;
        return;
    }

    while (true) {

        // this will splice the optional arguments into the list
        if (argsPtr[0]->kind == EK_ARGLIST) {
            if (argListSeen) {
                errorN(argsPtr[0]->tree, ERR_IllegalArglist);
            }
            argsPtr[0] = argsPtr[0]->asBIN()->p1;
            argListSeen = true;
            if (!argsPtr[0]) break;
        }

        if (argsPtr[0]->kind == EK_LIST) {
            indir = &(argsPtr[0]->asBIN()->p1);
        } else {
            indir = argsPtr;
        }
        
        if (indir[0]->type->kind == SK_PARAMMODSYM) {
            call->flags |= EXF_HASREFPARAM;
            if (paramCount) paramCount--;
            if (markTypeFromExternCall) {
                AGGSYM * temp = indir[0]->type->underlyingAggregate();
                if (temp && temp->isStruct)
                    temp->hasExternReference = true;
            }
        } else if (paramCount) {
            if (paramCount == 1 && indir != argsPtr && mp->isParamArray) {
                // we arrived at the last formal, and we have more than one actual, so 
                // we need to put the rest in an array...
                goto FIXUPPARAMLIST;
            }
            EXPR * rval = tryConvert(indir[0], *destType, indir[0]->tree);
            if (!rval) {
                // the last arg failed to fix up, so it must fixup into the array element
                // (we will be passing a 1 element array...)
                ASSERT(mp->isParamArray);
                goto FIXUPPARAMLIST;
            }
            ASSERT(rval);
            indir[0] = rval;
            paramCount--;
        }
        // note that destype might not be valid if we are in varargs, but then we won't ever use it...
        destType++;

        if (indir == argsPtr) {
            if (paramCount && mp->isParamArray) {
                // we run out of actuals, but we still have formals, so this is an empty array being passed
                // into the last param...
                indir = NULL;
                goto FIXUPPARAMLIST;
            } 
            break;
        }

        argsPtr = &(argsPtr[0]->asBIN()->p2);
    }

    return;

FIXUPPARAMLIST:

    // we need to create an array and put it as the last arg...

    TYPESYM * arrayType = mp->params[mp->cParams - 1];
    TYPESYM * elemType = arrayType->asARRAYSYM()->elementType();


    if (!argsPtr[0] || !indir) {
        // zero args case:
        CONSTVAL cv;
        cv.iVal = 0;
        EXPR * array = newExprBinop(NULL, EK_NEWARRAY, arrayType, newExprConstant(NULL, getPDT(PT_INT), cv), NULL);
        if (!argsPtr[0]) {
            argsPtr[0] = array;
        } else {
            argsPtr[0] = newExprBinop(NULL, EK_LIST, NULL, argsPtr[0], array);
        }
    } else {
        EXPRARRINIT * arrayInit = newExpr(NULL, EK_ARRINIT, arrayType)->asARRINIT();
        arrayInit->dimSizes = &(arrayInit->dimSize);

        EXPR ** lastArg = argsPtr;

        unsigned count = 0;

        while (true) {

            if (argsPtr[0]->kind == EK_LIST) {
                indir = &(argsPtr[0]->asBIN()->p1);
            } else {
                indir = argsPtr;
            }
        
            count++;
            EXPR * rval = tryConvert(indir[0], elemType, indir[0]->tree);
            ASSERT(rval);

            indir[0] = rval;

            if (indir == argsPtr) {
                break;
            }

            argsPtr = &(argsPtr[0]->asBIN()->p2);
        }

        arrayInit->dimSize = count;
        arrayInit->args = lastArg[0];
        lastArg[0] = arrayInit;

    }
}


EXPR * FUNCBREC::bindToProperty(BASENODE * tree, EXPR * object, PROPSYM * prop, int bindFlags, EXPR * args)
{
    METHPROPSYM * candidate = verifyMethodCall(tree, prop, args, &object, bindFlags & EXF_BASECALL);

    if (!candidate) goto LERROR;

    ASSERT(candidate->kind == SK_PROPSYM);

    EXPRPROP * rval;
    rval = newExpr(tree, EK_PROP, candidate->asPROPSYM()->retType)->asPROP();

    rval->prop = candidate->asPROPSYM();
    rval->args = args;
    rval->object = object;

    ASSERT(EXF_BASECALL == BIND_BASECALL);
    rval->flags |= (EXF_BASECALL & bindFlags);

    verifyMethodArgs(rval);

    // if we are doing a get on this thing, and there is no get, and
    // most imporantly, we are not leaving the arguments to be bound by the array index
    // then error...
    if (bindFlags & BIND_RVALUEREQUIRED) {
        if (!candidate->asPROPSYM()->methGet) {
            errorSym(tree, ERR_PropertyLacksGet, candidate);
        } else {
            // if the get exists, but is abstract, forbid the call as well...
            if ((bindFlags & BIND_BASECALL) && candidate->asPROPSYM()->methGet->isAbstract) {
                errorSym(tree, ERR_AbstractBaseCall, candidate);
            }
        }
    }

    if (candidate->asPROPSYM()->methSet && objectIsLvalue(rval->object)) {
        rval->flags |= EXF_LVALUE;
    }

    if (rval->type)
        compiler()->symmgr.DeclareType (rval->type);    

    return rval;

LERROR:

    return newError(tree);
}


// Construct the EXPR node which corresponds to a field expression
// for a given field and object pointer.
EXPR * FUNCBREC::bindToField(BASENODE * tree, EXPR * object, MEMBVARSYM * field, int bindFlags)
{
    EXPR * expr;

    if (field->isDeprecated) {
        reportDeprecated(tree, field);
    }

    if (checkBogus(field)) {
        compiler()->clsDeclRec.errorNameRefSymbol(field->name, tree, pParent, field, ERR_BindToBogus);
        return newError(tree);
    }

    checkStaticness(tree, field, &object);
    if (field->isUnevaled) {
        if (!bindUnevaledConstantField(field)) {
            return newError(tree);
        }
    }
    if ((bindFlags & BIND_RVALUEREQUIRED) && !field->isReferenced) {
        field->isReferenced = true;
    }
    if (field->isConst) {
        TYPESYM * fieldType = field->type;
        
        // Special enum rule: if we evaluating an enumerator initializer, and the constant is any other enumerator,
        // it's treated as a constant of it's underlying type.
        if (!pMSym && pFSym && pParent->asAGGSYM()->isEnum && fieldType->kind == SK_AGGSYM && fieldType->asAGGSYM()->isEnum)
        {
            fieldType = fieldType->asAGGSYM()->underlyingType;
        }

        expr = newExprConstant(tree, fieldType, field->constVal);
    } else {

        expr = newExpr(tree, EK_FIELD, field->type);
        expr->asFIELD()->ownerOffset = 0;

        unsigned short parentOffset = object ? object->getOffset() : 0;
        if (parentOffset && object->type->isAnyStructType()) {
            expr->asFIELD()->offset = parentOffset + field->offset;
            if (object->kind == EK_LOCAL && object->asLOCAL()->local == thisPointer && pClassInfo && pClassInfo->offsetList && pMSym && pMSym->isCtor) {
                expr->asFIELD()->ownerOffset = firstParentOffset + pClassInfo->offsetList[field->offset];
            } else {
                expr->asFIELD()->ownerOffset = object->getOwnerOffset();
            }
            //ASSERT(expr->asFIELD()->ownerOffset < 100);
        } else {
            expr->asFIELD()->offset = 0;
            expr->asFIELD()->ownerOffset = 0;
        }

        expr->asFIELD()->object = object;
        expr->asFIELD()->field = field;
        ASSERT(BIND_MEMBERSET == EXF_MEMBERSET);
        expr->flags |= (bindFlags & BIND_MEMBERSET);
        if ((object && object->type->kind == SK_PTRSYM) || (objectIsLvalue(object))) {
            expr->flags |= EXF_LVALUE;

            // Exception: a readonly field is not an lvalue unless we're in the constructor/static constructor appropriate
            // for the field.
            if (field->isReadOnly) {
                if (pParent->kind != SK_AGGSYM || 
                    !pMSym || !pMSym->isCtor ||
                    field->getClass() != pParent->asAGGSYM() ||
                    pMSym->isStatic != field->isStatic ||
                    (object != NULL && (object->kind != EK_LOCAL || object->asLOCAL()->local != thisPointer)))
                {
                    expr->flags &= ~EXF_LVALUE;
                }
            }

        }

    }

    if (expr->type)
        compiler()->symmgr.DeclareType (expr->type);    

    return expr;
}

// Construct the EXPR node which corresponds to am event expression
// for a given event and object pointer.
EXPR * FUNCBREC::bindToEvent(BASENODE * tree, EXPR * object, EVENTSYM * event, int bindFlags)
{
    EXPR * expr;

    if (event->isDeprecated)
        reportDeprecated(tree, event);

    if (checkBogus(event)) {
        if (event->useMethInstead) {
            CError  *pError;
            if (event->methAdd && event->methRemove)
                pError = make_errorStrSymSym( tree, ERR_BindToBogusProp2, event->name->text, event->methAdd, event->methRemove);
            else
                pError = make_errorStrSym( tree, ERR_BindToBogusProp1, event->name->text,
                // The casts are here because we can't just bitwise-or the getter and setter (we know one is NULL)
                (METHSYM*)((INT_PTR)event->methAdd | (INT_PTR)event->methRemove));
            compiler()->clsDeclRec.AddRelatedSymbolLocations (pError, event);
            compiler()->SubmitError (pError);
        } else {
            compiler()->clsDeclRec.errorNameRefSymbol(event->name, tree, pParent, event, ERR_BindToBogus);
        }
        return newError(tree);
    }

    checkStaticness(tree, event, &object);

    expr = newExpr(tree, EK_EVENT, event->type);
    expr->asEVENT()->object = object;
    expr->asEVENT()->event = event;
    ASSERT(EXF_BASECALL == BIND_BASECALL);
    expr->flags = (EXF_BASECALL & bindFlags);

    return expr;
}


void FUNCBREC::maybeSetOwnerOffset(LOCVARSYM * local)
{
    if (local->type->asAGGSYM()->hasInnerFields()) {
        local->ownerOffset = ++uninitedVarCount;
    }
}

// Bind the unqualified name.  mask details the allowed return types
EXPR * FUNCBREC::bindName(NAMENODE *name, int mask, int bindFlags)
{

    NAME * ident = name->pName;
    SYM * rval = lookupCachedName(name, mask);
    EXPR * expr;

    // If we found a cache entry, dispatch based on what we found:
    if (rval) {
        switch (rval->kind) {
        case SK_ERRORSYM: goto RETERROR;
        case SK_LOCVARSYM: {
            // No need to goto anywhere, since locals are only found as a result of
            // cache hits...
            LOCVARSYM * local = rval->asLOCVARSYM();
            checkUnsafe(name, local->type);
            if (local->isConst) {
                expr = newExprConstant(name, local->type, local->constVal);
            } else {
                expr = newExpr(name, EK_LOCAL, local->type);
                expr->asLOCAL()->local = local;
                ASSERT(BIND_MEMBERSET == EXF_MEMBERSET);
                expr->flags |= (bindFlags & BIND_MEMBERSET);
                if ((!local->slot.isPinned && !local->isNonWriteable) || (bindFlags & BIND_USINGVALUE)) {
                    expr->flags |= EXF_LVALUE;
                }
            }
            if ((bindFlags & BIND_RVALUEREQUIRED)) {
                if (!local->slot.isReferenced) {
                    local->slot.isReferenced = true;
                    unreferencedVarCount --;
                }
                if (!local->slot.isUsed) {
                    local->slot.isUsed  = true;
                    if (!local->slot.hasInit && !local->slot.ilSlot) {
                        local->slot.ilSlot = ++uninitedVarCount;
                        if (local->type->fundType() == FT_STRUCT) {
                            uninitedVarCount += local->type->asAGGSYM()->getFieldsSize() - 1;
                            maybeSetOwnerOffset(local);
                        }
                    }
                }
            }
            return expr;
        }
        case SK_PROPSYM:
        case SK_MEMBVARSYM:
        case SK_METHSYM: goto METHORMEMBVAR;
        case SK_NSSYM: goto NS;
        case SK_AGGSYM: goto AGG;
        default: ASSERT(!"bad kind");
        }
    }

    // ok, we got a cache miss:
    SYM * cls;
    cls = pParent;

    while (cls->kind == SK_AGGSYM) {
        // See if it is a field or method in the current class...
        rval = lookupClassMember(ident, cls->asAGGSYM(), false, false, true);
        if (rval) {
            if (!(rval->mask() & mask)) {
                goto BADKIND;
            }
            storeInCache(name, name->pName, rval);
METHORMEMBVAR:
            EXPR * thisExpr = bindThisImplicit(name);

            if (rval->kind == SK_MEMBVARSYM) {
                if (!rval->asMEMBVARSYM()->isStatic && (mask & MASK_AGGSYM)) {
                    if (!pMSym || (pMSym->isStatic || !this->thisPointer) || !canConvert(pParent->asAGGSYM(), rval->parent->asAGGSYM(), name, CONVERT_NOUDC)) {
                        if (rval->asMEMBVARSYM()->type->name == ident) {
                            SYM * type = compiler()->clsDeclRec.bindSingleTypeName(name, pParent, BTF_NONE | btfFlags, NULL);
                            if (!type) return newError(name);
                            if (type == rval->asMEMBVARSYM()->type) {
                                return newExpr(name, EK_CLASS, type->asTYPESYM());
                            }
                        }
                    }
                }
                expr = bindToField(name, thisExpr, rval->asMEMBVARSYM(), bindFlags );
            } else if (rval->kind == SK_METHSYM) {
                expr = newExpr(name, EK_CALL, rval->asMETHSYM()->retType);
                expr->asCALL()->method = rval->asMETHSYM();
                expr->asCALL()->object = thisExpr;
                expr->asCALL()->args = NULL;
            } else if (rval->kind == SK_PROPSYM) {
                if (!rval->asPROPSYM()->isStatic && (mask & MASK_AGGSYM)) {
                    if (!pMSym || (pMSym->isStatic || !this->thisPointer) || !canConvert(pParent->asAGGSYM(), rval->parent->asAGGSYM(), name, CONVERT_NOUDC)) {
                        if (rval->asPROPSYM()->retType->name == ident) {
                            SYM * type = compiler()->clsDeclRec.bindSingleTypeName(name, pParent, BTF_NONE | btfFlags, NULL);
                            if (!type) return newError(name);
                            if (type == rval->asPROPSYM()->retType) {
                                return newExpr(name, EK_CLASS, type->asTYPESYM());
                            }
                        }
                    }
                }
                expr = bindToProperty(name, thisExpr, rval->asPROPSYM(), bindFlags);
            } else if (rval->kind == SK_EVENTSYM) {
                if (!rval->asEVENTSYM()->isStatic && (mask & MASK_AGGSYM)) {
                    if (!pMSym || (pMSym->isStatic || !this->thisPointer) || !canConvert(pParent->asAGGSYM(), rval->parent->asAGGSYM(), name, CONVERT_NOUDC)) {
                        if (rval->asEVENTSYM()->type->name == ident) {
                            SYM * type = compiler()->clsDeclRec.bindSingleTypeName(name, pParent, BTF_NONE | btfFlags, NULL);
                            if (!type) return newError(name);
                            if (type == rval->asMEMBVARSYM()->type) {
                                return newExpr(name, EK_CLASS, type->asTYPESYM());
                            }
                        }
                    }
                }
                expr = bindToEvent(name, thisExpr, rval->asEVENTSYM(), bindFlags);
            }
            else 
                goto CONSIDERCLASS;

            if (expr->type) {
                checkUnsafe(name, expr->type);
            }
            return expr;
        }
        cls = cls->asAGGSYM()->parent;
    }

    int flags;
    flags = 0;


    if (! (mask & (MASK_AGGSYM | MASK_NSSYM))) {
        // this could not be a type, so we already know we are lost...
        rval = compiler()->clsDeclRec.bindSingleTypeName(name, pParent, BTF_STARTATNS | BTF_NSOK | BTF_ANYBADACCESS | BTF_NOERRORS, NULL);
        if (!rval) {

            // maybe it exists but is inaccesible?
            cls = pParent;
            while (cls->kind == SK_AGGSYM) {
                // See if it is a field or method in the current class...
                rval = lookupClassMember(ident, cls->asAGGSYM(), false, true, true);
                if (rval) {
                    if (!rval->isUserCallable()) {
                        errorSym(name, ERR_CantCallSpecialMethod, rval);
                    } else {
                        errorSym(name, ERR_BadAccess, rval);
                    }
                    goto RETERROR;
                }
                cls = cls->asAGGSYM()->parent;
            }

            errorStrSym(name, ERR_NameNotInContext, name->pName->text, pParent);
        }
                
    } else {
        rval = compiler()->clsDeclRec.bindSingleTypeName(name, pParent, BTF_STARTATNS | BTF_NSOK | BTF_ANYBADACCESS | btfFlags, NULL);
    }

    if (rval) {
        if (!(rval->mask() & mask)) goto BADKIND;
        storeInCache(name, name->pName, rval);
CONSIDERCLASS:
        if (rval->kind == SK_AGGSYM) {
AGG:
            compiler()->symmgr.DeclareType(rval);
            expr = newExpr(name, EK_CLASS, rval->asTYPESYM());
        } else {
            ASSERT(rval->kind == SK_NSSYM);
NS:
            expr = newExpr(name, EK_NSPACE, getVoidType());
            expr->asNSPACE()->nspace = rval->asNSSYM();
        }
        return expr;
    }

RETERROR:
    return newError(name);  // error already announced by bindSingleType...


BADKIND:
    return errorBadSK(name, rval, mask);
}


unsigned __fastcall FUNCBREC::isThisPointer(EXPR * expr)
{
    return expr->kind == EK_LOCAL && expr->asLOCAL()->local == thisPointer;
}

// Return an expression that refers to the this pointer. Returns null
// if no this is available. See bindThis is error reporting is required.
EXPR * FUNCBREC::bindThisImplicit(PBASENODE tree)
{
    if (!thisPointer) {
        return NULL;
    }

    EXPR * thisExpr;

    thisExpr = newExpr(tree, EK_LOCAL, pParent->asAGGSYM());
    thisExpr->asLOCAL()->local = thisPointer;
    thisExpr->flags |= EXF_IMPLICITTHIS | EXF_CANTBENULL;
    if (pParent->asAGGSYM()->isStruct) {
        thisExpr->flags |= EXF_LVALUE;
    }
    return thisExpr;
}


// Return an expression that refers to the this pointer. Prints appropriate
// error if this is not available. See bindThisExpr if no error is desired.
EXPR * FUNCBREC::bindThisExplicit(BASENODE * tree)
{
    EXPR * thisExpr;

    thisExpr = bindThisImplicit(tree);

    if (! thisExpr) {
        if (pMSym && pMSym->isStatic)
            errorN(tree, ERR_ThisInStaticMeth);
        else
            errorN(tree, ERR_ThisInBadContext); // 'this' isn't available for some other reason.

        return newError(tree);
    } else {
        thisExpr->flags &= ~EXF_IMPLICITTHIS;
    }

    return thisExpr;
}


// Creates a new scopes as a child of the current scope.  You should always call
// this instead of creating the scope by hand as this correctly increments the
// scope nesting order...
void FUNCBREC::createNewScope()
{
    SCOPESYM * rval = compiler()->symmgr.CreateLocalSym(SK_SCOPESYM, NULL, pCurrentScope)->asSCOPESYM();
    rval->nestingOrder = pCurrentScope->nestingOrder + 1;
    pCurrentScope = rval;
}

void FUNCBREC::closeScope()
{
    pCurrentScope->tree = lastNode;
    pCurrentScope = pCurrentScope->parent->asSCOPESYM();

}


void FUNCBREC::initThisPointer()
{
    if (!pMSym->isStatic) {
        // Enter the this pointer:
        thisPointer = compiler()->symmgr.CreateLocalSym(SK_LOCVARSYM, compiler()->namemgr->GetPredefName(PN_THIS), pOuterScope)->asLOCVARSYM();
        thisPointer->access = ACC_PUBLIC;
        thisPointer->type = pParent->asAGGSYM();
        thisPointer->slot.isParam = true;
        if (pParent->asAGGSYM()->isStruct) {
            thisPointer->slot.isRefParam = true;
            if (pMSym->isCtor && !pMSym->isStatic) {
                thisPointer->slot.ilSlot = uninitedVarCount + 1;
                uninitedVarCount += pParent->asAGGSYM()->getFieldsSize();
                if (pClassInfo->offsetList) {
                    firstParentOffset = uninitedVarCount + 1;
                    uninitedVarCount += pClassInfo->offsetList[pParent->asAGGSYM()->getFieldsSize() - 1] + 1;
                }
            }
        }
    } else {
        thisPointer = NULL;
    }
}


// Prepares compilation for a method.
// Declare the this pointer for whatever we are compiling
// Does NOT create declarations for method parameters
void FUNCBREC::declare(METHINFO * info)
{
    pCatchScope = pTryScope = pFinallyScope = pCurrentScope = pOuterScope = compiler()->symmgr.CreateLocalSym(SK_SCOPESYM, NULL, NULL)->asSCOPESYM();
    pSwitchScope = NULL;
    pOuterScope->nestingOrder = 0;

    initLabels.breakLabel = NULL;
    initLabels.contLabel = NULL;
    loopLabels = &initLabels;

    unreachedLabelCount = 0;
    uninitedVarCount = 0;
    unreferencedVarCount = 0;
    unrealizedConcatCount = 0;
    unrealizedGotoCount = 0;
    untargetedLabelCount = 0;
    outParamCount = 0;
    finallyNestingCount = 0;
    localCount = 0;
    gotos = NULL;

    reachablePass = false;

    userLabelList = NULL;
    pUserLabelList = &userLabelList;

    initThisPointer();

    pCurrentBlock = NULL;
    lastNode = NULL;

    codeIsReachable = true;
}

// Create declarations for method parameters
void FUNCBREC::declareMethodParameters(METHINFO * info)
{
    BASENODE * params;
    if (pTree->kind == NK_METHOD || pTree->kind == NK_CTOR || 
        pTree->kind == NK_OPERATOR || pTree->kind == NK_DTOR) {

        params = pTree->asANYMETHOD()->pParms;

    } else {
        ASSERT(pTree->kind == NK_CLASS || pTree->kind == NK_STRUCT);
        params = NULL;
    }

    PARAMINFO *paramInfo = info->paramInfos;
    NODELOOP(params, PARAMETER, param)
        paramInfo->name = param->pName->pName;
        paramInfo++;
        declareParam(param);
    ENDLOOP;

    info->outerScope = pOuterScope;
}


// compile the code for a method
EXPR * FUNCBREC::bindMethod(METHINFO * info)
{
    declare(info);
    declareMethodParameters(info);

    return bindMethOrPropBody(pTree->asANYMETHOD()->pBody);
}

// binds a property accessor
EXPR *FUNCBREC::bindProperty(METHINFO * info)
{

    declare(info);

    //
    // add parameters for indexed properties
    //
    int iParam = 0;
    PARAMINFO *paramInfo = info->paramInfos;
    NODELOOP(pTree->pParent->asANYPROPERTY()->pParms, PARAMETER, param)
        paramInfo->name = param->pName->pName;
        paramInfo++;
        declareParam(param);
        ++iParam;
    ENDLOOP;


    //
    // declare property parameters
    //
    if (pMSym->isGetAccessor()) {
        // get accessors have no parameters, so we do nothing
    } else {
        //
        // set accessor. We have one implicit parameter whose name is 'value'
        // Note that we want to do duplicate name checking on this puppy
        //
        paramInfo->name = compiler()->namemgr->GetPredefName(PN_VALUE);
        declareParam(paramInfo->name, pMSym->params[pMSym->cParams-1], 0 /* no REF or OUT flags */, pTree);
    }

    //
    // Prepares our scopes for lookups.
    //
    info->outerScope = pOuterScope;

    return bindMethOrPropBody(pTree->asACCESSOR()->pBody);
}

// binds a event accessor
EXPR *FUNCBREC::bindEventAccessor(METHINFO * info)
{
    LOCVARSYM * param;
    EVENTSYM * event;
    SYM * fieldOrProperty;
    BASENODE * tree = pMSym->parseTree;
    bool wasUnreferenced = false;

    declare(info);

    //
    // declare single parameter called "handler".
    //
    info->paramInfos[0].name = compiler()->namemgr->GetPredefName(PN_VALUE);
    param = declareParam(info->paramInfos[0].name, pMSym->params[0], 0 /* no REF or OUT flags */, pTree);

    //
    // Prepares our scopes for lookups.
    //
    info->outerScope = pOuterScope;

    if (pMSym->isAbstract || pMSym->isExternal)
        return NULL;            // in interface.

    // Get the associated event.
    event = pMSym->getEvent();
    fieldOrProperty = event->implementation;

    if (fieldOrProperty) {
        // The code for this is either:
        //    event = Delegate.Combine(event, handler)
        //    event = Delegate.Remove(event, handler)
        // wheren event is the member field or property, and handler is the argument.

        EXPR * op1, * op2;   // the two arguments to Combine/Remove

        // Should not be any user-provided code for this event.
        ASSERT(pTree->kind != NK_ACCESSOR);

        // Get current value of event.
        if (fieldOrProperty->kind == SK_MEMBVARSYM) {
            if (! fieldOrProperty->asMEMBVARSYM()->isReferenced)
                wasUnreferenced = true;
            op1 = bindToField(tree, bindThisImplicit(tree), fieldOrProperty->asMEMBVARSYM(), BIND_RVALUEREQUIRED);
        }
        else {
            op1 = bindToProperty(tree, bindThisImplicit(tree), fieldOrProperty->asPROPSYM(), BIND_RVALUEREQUIRED);
        }

        // get "handler" parameter
        op2 = newExpr(tree, EK_LOCAL, param->type);
        op2->asLOCAL()->local = param;
        param->slot.isUsed = true;

        // Construct argument list from the two
        EXPR * args = newExprBinop(tree, EK_LIST, getVoidType(), op1, op2);

        // Find and bind the Delegate.Combine or Delegate.Remove call.
        EXPR * obj = NULL;
        NAME * memb = compiler()->namemgr->GetPredefName((pMSym == event->methAdd) ? PN_COMBINE : PN_REMOVE);
        METHSYM * method = compiler()->symmgr.LookupGlobalSym(
            memb, getPDT(PT_DELEGATE), MASK_METHSYM)->asMETHSYM();
        if (!method) {
            errorSymName(tree, ERR_MissingPredefinedMember, getPDT(PT_DELEGATE), memb);
            return newError( tree);
        }
        SYM * verified = verifyMethodCall(tree, method, args, &obj, 0);

        // Create the call node for the call.
        EXPRCALL * call = newExpr(tree, EK_CALL, getPDT(PT_DELEGATE))->asCALL();
        call->object = obj;
        call->flags |= EXF_BOUNDCALL;
        call->method = verified->asMETHSYM();
        call->args = args;

        // Cast the result to the delegate type.
        EXPR * cast = mustCast(call, event->type, tree);

        // assign results of the call back to the property/field
        EXPR * lvalue;
        if (fieldOrProperty->kind == SK_MEMBVARSYM) {
            lvalue = bindToField(tree, bindThisImplicit(tree), fieldOrProperty->asMEMBVARSYM());
        }
        else {
            lvalue = bindToProperty(tree, bindThisImplicit(tree), fieldOrProperty->asPROPSYM());
        }

        EXPR * assg = newExprBinop(tree, EK_ASSG, lvalue->type, lvalue, cast);
        assg->flags |= EXF_ASSGOP;
    
        // Wrap it all in a statement and then a block, and add implicit return.
        EXPRBLOCK * block = newExprBlock(NULL);
        block->statements = newExprStmtAs(tree, assg);
        block->flags |= EXF_NEEDSRET;

        if (wasUnreferenced) {
            // Any references made while binding this event accessor is not considered a reference to the event field.
            fieldOrProperty->asMEMBVARSYM()->isReferenced = false;
        }

        return block; 
    }
    else {
        return bindMethOrPropBody(pTree->asACCESSOR()->pBody);
    }
}

EXPR * FUNCBREC::bindIfaceImpl(METHINFO *info)
{
    declare(info);

    this->unsafeContext = true; // Don't generate unsafe warnings

    //
    // declare parameters ... need to give them names
    //
    WCHAR paramName[2];
    paramName[0] = 'a';
    paramName[1] = 0;
    for (int i = 0; i < pMSym->cParams; i += 1) {
        paramName[0] += 1;
        info->paramInfos[i].name = compiler()->namemgr->AddString(paramName);
        unsigned flags = 0;
        TYPESYM *type = pMSym->params[i];
        if (type->kind == SK_PARAMMODSYM) {
            flags = (type->asPARAMMODSYM()->isOut ? NF_PARMMOD_OUT : NF_PARMMOD_REF);
            type = type->asPARAMMODSYM()->paramType();
        }
        declareParam(info->paramInfos[i].name, type, flags, NULL);
    }
    info->outerScope = pOuterScope;

    info->noDebugInfo = true;   // Don't generate debug info for this method, since there is no source code.

    // The code for this is call of the real implementation

    EXPR *args = NULL;
    EXPR ** pargs = &args;
    bool hasRefParam = false;
    for (int i = 0; i < pMSym->cParams; i += 1) {
        EXPRLOCAL * expr = newExpr(NULL, EK_LOCAL, pMSym->params[i])->asLOCAL();
        LOCVARSYM *local = compiler()->symmgr.LookupLocalSym(
            info->paramInfos[i].name, 
            pOuterScope, 
            MASK_LOCVARSYM)->asLOCVARSYM();

        if (expr->type->kind == SK_PARAMMODSYM) {
            hasRefParam = true;
        }
        expr->local = local;
        expr->flags = EXF_LVALUE;
        if (!local->slot.isReferenced) {
            local->slot.isReferenced = true;
            unreferencedVarCount --;
        }
        local->slot.isUsed  = true;
        newList(expr, &pargs);
    }

    EXPRCALL * call = newExpr(NULL, EK_CALL, pMSym->retType)->asCALL();
    call->method = pMSym->asIFACEIMPLMETHSYM()->implMethod;
    call->object = bindThisImplicit(NULL);
    call->args = args;
    call->flags = EXF_BOUNDCALL | (hasRefParam ? EXF_HASREFPARAM : 0);

    // Wrap it all in a block, figure out the return statement
    EXPRBLOCK * block = newExprBlock(NULL);
    if (pMSym->retType == getVoidType()) {
        block->statements = newExprStmtAs(NULL, call);
        block->flags |= EXF_NEEDSRET;       // implicit return statement
    } else {

        // explicit return statement
        EXPRRETURN * returnExpr;
        returnExpr = newExpr(NULL, EK_RETURN, NULL)->asRETURN();
        returnExpr->object = call;

        codeIsReachable = false;

        block->statements = returnExpr;
    }


    return block; 
}

// binds the body of a method or property accessor
// the FUNCBREC must already be set up with the this ptr and arguments
// declared.
EXPRBLOCK *FUNCBREC::bindMethOrPropBody(BLOCKNODE *body)
{
    if (body) {
        EXPRBLOCK * block;

        block = bindBlock(body);

        if (codeIsReachable && !reachablePass && !unreachedLabelCount && errorsBeforeBind == compiler()->ErrorCount()) {
            // We eill emit the RET/give error here if any code precedes the procedure exit...
            if (pMSym->retType == getVoidType()) {
                block->flags |= EXF_NEEDSRET;
            } else {
                errorSym(pTree, ERR_ReturnExpected, pMSym);
            }
        }

        return block;
    } else {
        // abstract method ...
        return NULL;
    }
}

// compile the code for a constructor, this also deals with field inits and
// synthetized bodies
EXPR * FUNCBREC::bindConstructor(METHINFO * info)
{
    EXPR * list = NULL;
    EXPR ** pList = &list;

    //
    // determine if we are calling this class or a base class
    // Note that the parse tree may be a CTORNODE for explicit constructors
    // or its a CLASSNODE for implicit constructors
    //
    ASSERT(pTree->kind == NK_CTOR || pTree->kind == NK_CLASS || pTree->kind == NK_STRUCT);
    bool isThisCall;
    bool hasFieldInits = false;
    if (pTree->kind == NK_CTOR) {
        // parser should never generate both flags
        ASSERT((pTree->other & (NFEX_CTOR_THIS | NFEX_CTOR_BASE)) != (NFEX_CTOR_THIS | NFEX_CTOR_BASE));

        isThisCall = (pTree->other & NFEX_CTOR_THIS) ? true : false;
    } else {
        // implicit constructors never call a this constructor
        isThisCall = false;
    }

    //
    // set up the lookup table cache and this pointer which
    // is needed before we bind our initializers
    //
    declare(info);

    //
    // static constructors allways generate field initializers
    //
    // non-static constructors generate field initializers
    // if we don't explicitly call another contructor
    // on this class
    //
    if ((pMSym->isStatic || !isThisCall) && !pMSym->isExternal) {
        // bind field inits
        bindInitializers(pMSym->isStatic, &pList);
        hasFieldInits = (list != NULL);
    }

    //
    // declare the constructor arguments
    //
    // Note that this must come after initializers
    // are done, otherwise constructor arguments will be available
    // to field initializers. Oops.
    //
    declareMethodParameters(info);

    //
    // for compiler generated constructors of comimport classes
    //
    if (pMSym->isExternal) {
        return NULL;
    }

    //
    // generate the call to the base class constructor
    //
    if (!pMSym->isStatic) {
        //
        // structs can never explicitly(or implictly call a base constructor)
        //
        if (pParent->asAGGSYM()->isStruct && pTree->other & NFEX_CTOR_BASE) {

            compiler()->clsDeclRec.errorSymbol(pMSym, ERR_StructWithBaseConstructorCall);
            return NULL;

        //
        // are we compiling constructor for object
        //
        } else if (!isThisCall && !pParent->asAGGSYM()->baseClass) {
            ASSERT(pParent == compiler()->symmgr.GetObject());

            //
            // constructor call for object is attempting
            // to call a base class constrcutor ...
            //
            if ((pTree->kind == NK_CTOR) && (pTree->other & NFEX_CTOR_BASE)) {
                compiler()->clsDeclRec.errorSymbol(pParent, ERR_ObjectCallingBaseConstructor);
            } else {
                //
                // object doesn't need to call a base class constructor
                //
            }

        //
        // generate constructor call for all classes and structs
        // calling a this constructor
        //
        } else if (!pParent->asAGGSYM()->isStruct || isThisCall) {
            EXPR * baseCall = createBaseConstructorCall(isThisCall);
            if (pTree->kind != NK_CTOR)
                baseCall->flags |= EXF_NODEBUGINFO;
            newList(baseCall, &pList);
        }
    }

    // if we got a body, bind it...
    if (pTree->kind != NK_CLASS && pTree->kind != NK_STRUCT) {
        ASSERT(pTree->kind == NK_CTOR);


        EXPR * body = bindBlock(pTree->asCTOR()->pBody);
        newList(body, &pList);
    }
    else {
        // No user-supplied body, so no debug info please.
        // Unless we had some field initializers
        info->noDebugInfo = !hasFieldInits;
    }

    //
    // if we have a default constructor for a struct with no
    // members, then we must force an empty function to be generated
    // because folks will want to call it ...
    // Same for Object's default constructor
    //
    if (!list && pMSym->isCtor && !pMSym->isStatic &&
        (pParent->asAGGSYM()->isStruct ||
        (!isThisCall && !pParent->asAGGSYM()->baseClass && pTree->kind != NK_CTOR))) {
        EXPRRETURN * ret = newExpr(pTree, EK_RETURN, NULL)->asRETURN();
        ret->object = NULL;
        newList(ret, &pList);
    }

    // wrap it all in a block...
    if (list && list->kind != EK_BLOCK) {
        EXPRBLOCK * block = newExprBlock(NULL);
        block->statements = list;
        list = block;
    }

    // Constructors have an implicit return at the end.
    if (list && codeIsReachable)
        list->asBLOCK()->flags |= EXF_NEEDSRET;

    return list;
}


// compile the code for a constructor, this also deals with field inits and
// synthetized bodies
EXPR * FUNCBREC::bindDestructor(METHINFO * info)
{
    ASSERT(pTree->kind == NK_DTOR);

    EXPR * list = NULL;
    EXPR ** pList = &list;

    //
    // set up the lookup table cache and this pointer
    //
    declare(info);
    info->outerScope = pOuterScope;

    //
    // for compiler generated constructors of comimport classes
    //
    if (pMSym->isExternal) {
        return NULL;
    }

    SYM *baseDestructor = NULL;
    AGGSYM *cls = pParent->asAGGSYM()->baseClass;
    if (cls) {
        //
        // find a destructor in a base class, ignoring everything
        // which doesn't look like a destructor
        //
        while ((baseDestructor = compiler()->clsDeclRec.findNextName(pMSym->name, cls, baseDestructor)) &&
               (baseDestructor->kind != SK_METHSYM ||
                (baseDestructor->asMETHSYM()->access != ACC_PROTECTED && baseDestructor->asMETHSYM()->access != ACC_INTERNALPROTECTED) ||
                baseDestructor->asMETHSYM()->params != NULL ||
                baseDestructor->asMETHSYM()->retType != getPDT(PT_VOID))) 
        {
            cls = baseDestructor->parent->asAGGSYM();
        }
    }

    EXPRTRY * tryExpr = NULL;
    if (baseDestructor) {
        tryExpr = newExpr(NULL, EK_TRY, NULL)->asTRY();

        tryExpr->flags |= EXF_ISFINALLY;
        tryExpr->finallyStore = (FINALLYSTORE*) allocator->AllocZero(sizeof(FINALLYSTORE));

        finallyNestingCount ++;
    
        SCOPESYM * scopeOfTry = NULL;
        EXPR * body = bindBlock(pTree->asDTOR()->pBody, SF_TRYSCOPE, &scopeOfTry);

        tryExpr->tryblock = body->asBLOCK();

        finallyNestingCount --;

        //
        // generate the this pointer from our first param
        //
        ASSERT(thisPointer);
        EXPR * thisExpression = bindThisImplicit(pTree);

        //
        // create the call expression
        //
        EXPRCALL * baseCall;
        baseCall = newExpr(pTree, EK_CALL, getVoidType())->asCALL();
        baseCall->object = thisExpression;
        baseCall->args = NULL;
        baseCall->method = baseDestructor->asMETHSYM();
        baseCall->flags |= EXF_BOUNDCALL | EXF_BASECALL;


        tryExpr->handlers = newExprBlock(NULL);
        tryExpr->handlers->asBLOCK()->statements = newExprStmtAs(pTree, baseCall);
        newList(tryExpr, &pList);

    } else {

        EXPR * body = bindBlock(pTree->asDTOR()->pBody);
        newList(body, &pList);
    }

    // wrap it all in a block...
    if (list && list->kind != EK_BLOCK) {
        EXPRBLOCK * block = newExprBlock(NULL);
        block->statements = list;
        list = block;
    }

     // Destructors have an implicit return at the end.
    if (list && codeIsReachable)
        list->asBLOCK()->flags |= EXF_NEEDSRET;

    return list;
}


void __fastcall FUNCBREC::checkReachable(BASENODE * tree)
{
    if (codeIsReachable) return;
    codeIsReachable = true;
    reachablePass = true;
    warningN(tree, WRN_UnreachableCode);
}

// Bind the given block by binding its statement list..
EXPRBLOCK * FUNCBREC::bindBlock(BASENODE * tree, int scopeFlags, SCOPESYM ** scope)
{

    createNewScope();
    if (scope) *scope = pCurrentScope;
    pCurrentScope->scopeFlags = scopeFlags;

    SCOPESYM ** pSaved = NULL;
    SCOPESYM * oldScope = NULL;

    pCurrentBlock = newExprBlock(tree);

    switch (scopeFlags & SF_KINDMASK) {
    case SF_CATCHSCOPE: pSaved = &pCatchScope; break;
    case SF_TRYSCOPE: pSaved = &pTryScope; break;
    case SF_FINALLYSCOPE: pSaved = &pFinallyScope; pCurrentScope->block = pCurrentBlock; break;
    case SF_SWITCHSCOPE: pSaved = &pSwitchScope; break;
    case SF_NONE: break;
    default:
        ASSERT(!"bad sf type");
    }

    if (pSaved) {
        oldScope = *pSaved;
        *pSaved = pCurrentScope;
    }


    STATEMENTNODE * stms = tree->kind == NK_BLOCK ? tree->asBLOCK()->pStatements : (STATEMENTNODE*) tree;

    EXPR * list = NULL;
    EXPR ** plist = &list;
    EXPR ** newPlist = NULL;

    while (stms) {
        EXPR * expr = bindStatement(stms, &newPlist);
        newList(expr, &plist);
        // if a single parsetree statement turned into more than one
        // expr statement, we inline the resulting list into the one we keep, and
        // set the last list pointer accordingly...
        if (newPlist) {
            plist = newPlist;
            newPlist = NULL;
        }
        stms = stms->pNext;
    }

    EXPRBLOCK * rval = pCurrentBlock;
    pCurrentBlock = rval->owningBlock;

    rval->statements = list;
    rval->scopeSymbol = pCurrentScope;

    closeScope();

    if (pSaved) {
        *pSaved = oldScope;
    }

    return rval;

}


// Bind an individial statement
EXPR * FUNCBREC::bindStatement(STATEMENTNODE *sn, EXPR *** newLast)
{
    BASENODE * tree = sn;
    EXPR * rval;

    SETLOCATIONNODE(sn);

    //  give one error per statement at most...
    unsafeErrorGiven = false;

AGAIN:

    if (!tree) return NULL;

#if DEBUG
    POSDATA pd;
    {
        if (tree) {
            if (pMSym) {
                pMSym->getInputFile()->pData->GetSingleTokenPos(tree->tokidx, &pd);
            } else if (pFSym) {
                pFSym->getInputFile()->pData->GetSingleTokenPos(tree->tokidx, &pd);
            }
        }
    }
#endif

    lastNode = sn;

    switch (tree->kind) {
    case NK_BLOCK:
        return bindBlock(tree->asBLOCK());
    case NK_LABEL:
        return bindLabel(tree->asLABEL(), newLast);
    case NK_UNSAFE:
         return bindUnsafe(tree->asUNSAFE(), newLast);
    case NK_CHECKED:
        return bindChecked(tree->asCHECKED(), newLast);
    case NK_EMPTYSTMT:
        return NULL;
    }

    checkReachable(tree);

    switch (tree->kind) {
    default:
        // This expression is not a legal statement.
        errorN(tree, ERR_IllegalStatement);
        return newError(tree);

    case NK_NAME: case NK_DOT: 
    case NK_OP: case NK_BINOP: case NK_UNOP:
    case NK_NEW:    
        rval = bindExpr(tree, BIND_RVALUEREQUIRED | BIND_STMTEXPRONLY);
        return newExprStmtAs(sn, rval);

    case NK_EXPRSTMT:
        tree = tree->asEXPRSTMT()->pArg;
        goto AGAIN;

    case NK_BREAK:
        return bindBreakOrContinue(tree, true);
    case NK_CONTINUE:
        return bindBreakOrContinue(tree, false);
    case NK_RETURN: 
        return bindReturn(tree->asRETURN());
    case NK_DECLSTMT:
        return bindVarDecls(tree->asDECLSTMT(), newLast);
    case NK_IF:
        return bindIf(tree->asIF(), newLast);
    case NK_DO:
        return bindWhileOrDo(tree->asDO(), newLast, false);
    case NK_WHILE:
        return bindWhileOrDo(tree->asWHILE(), newLast, true);
    case NK_FOR:
        return bindFor(tree->asFOR(), newLast);
    case NK_GOTO:
        return bindGoto(tree->asGOTO());
    case NK_SWITCH:
        return bindSwitch(tree->asSWITCH(), newLast);
    case NK_TRY:
        return bindTry(tree->asTRY());
    case NK_THROW:
        return bindThrow(tree->asTHROW());
    case NK_LOCK:
        return bindLock(tree->asLOCK(), newLast);
    }
}


EXPR * FUNCBREC::verifySwitchLabel(BASENODE * tree, TYPESYM * type, bool * ok)
{

    FUNDTYPE ft;
    *ok = true;

    if (tree) {
        EXPR * rval = bindExpr(tree);
        if (type) {
            rval = mustConvert(rval, type, tree);
        }
        if (rval->kind != EK_CONSTANT) {
            if (rval->isOK()) {
                errorN(tree , ERR_ConstantExpected);
            }
        } else {
            if (!type && ((ft = rval->type->fundType()) != FT_REF && ft > FT_I8)) {
                // an ugly float constant or something equally bad...
                errorN(tree, ERR_IntegralTypeValueExpected);
            } else {
                return rval;
            }
        }
        *ok = false;
    }

    return NULL;
}

// bind a goto statement...
EXPR * FUNCBREC::bindGoto(EXPRSTMTNODE * tree)
{
    EXPRGOTO * expr = newExpr(tree, EK_GOTO, NULL)->asGOTO();

    if (tree->flags & NF_GOTO_CASE) {
        EXPR * key;
        bool ok;
        if (tree->pArg) {
            key = verifySwitchLabel(tree->pArg, NULL, &ok);
        } else {
            key = NULL;
        }

        expr->flags |= EXF_GOTOCASE;
        expr->labelName = getSwitchLabelName(key->asCONSTANT());
        if (!pSwitchScope) {
            errorN(expr->tree, ERR_InvalidGotoCase);
            goto LERROR;
        }
        expr->targetScope = pSwitchScope;
        checkGotoJump(expr);

    } else {

        expr->labelName = tree->pArg->asNAME()->pName;
    }

    expr->currentScope = pCurrentScope;

    codeIsReachable = false;

    unrealizedGotoCount ++;
    // We don't check validity now, but during the def-use pass
    expr->flags |= EXF_UNREALIZEDGOTO;
    expr->prev = gotos;
    gotos = expr;

    return expr;

LERROR:

    return newError(tree);
}

EXPR * FUNCBREC::bindUnsafe(LABELSTMTNODE * tree, EXPR *** newLast)
{
    if (!compiler()->options.m_fUNSAFE) {
        errorN(tree, ERR_IllegalUnsafe);
    }

    bool oldUnsafe = unsafeContext;

    unsafeContext = true;

    EXPR * stmt = bindStatement(tree->pStmt, newLast);

    unsafeContext = oldUnsafe;

    return stmt;

}


EXPR * FUNCBREC::bindChecked(LABELSTMTNODE * tree, EXPR *** newLast)
{
    CHECKEDCONTEXT checked(this, !(tree->flags & NF_UNCHECKED));


    EXPR * stmt = bindStatement(tree->pStmt, newLast);

    checked.restore(this);

    return stmt;

}

// bind a label statement...
EXPR * FUNCBREC::bindLabel(LABELSTMTNODE * tree, EXPR *** newLast)
{
    NAME * ident = tree->pLabel->asNAME()->pName;

    LABELSYM * label = compiler()->symmgr.LookupLocalSym(ident, pCurrentScope, MASK_LABELSYM)->asLABELSYM();

    if (label) {
        errorName(tree, ERR_DuplicateLabel, ident);
    } else {
        label = compiler()->symmgr.CreateLocalSym(SK_LABELSYM, ident, pCurrentScope)->asLABELSYM();
        unreachedLabelCount++;
        untargetedLabelCount++;
    }

    EXPR * list = newExprLabel(true);
    list->tree = tree;

    // store it in our list so that we know where to find unreferenced or 
    // unreachable labels...
    newList(list, &pUserLabelList);

    label->labelExpr = list->asLABEL();

    EXPR ** pList  = &list;

    // for now assume all labels are reached...
    codeIsReachable = true;

    EXPR * stmt = bindStatement(tree->pStmt, newLast);

    if (stmt) {
        newList(stmt, &pList);

        if (!*newLast) *newLast = pList;
    }

    return list;
}

// bind the initializers for fields (either static or instance based...)
void FUNCBREC::bindInitializers(bool isStatic, EXPR ***pLast)
{

    ASSERT(pParent == pMSym->parent->asAGGSYM());
    AGGSYM * cls = pParent->asAGGSYM();

    //
    // we turn off name caching here ....
    //
    // the valid names available when we are binding instance constructors
    // is different for the initializers and the constructor body.
    //
    // this can lead to problems when we have a non-static field with
    // an initializer referencing static member and an instance constructor
    // with a parameter with the same name as the static member.
    //
    // The field initializer binds the name to the static field, then when we
    // want to declare the parameter the name cache already contains
    // a conflicting entry referencing the static member.
    //
    // sooooo we turn off adding names to the name cache while we compile
    // initializers so we avoid this whole freaking mess.
    //
    dontAddNamesToCache = true;

    // we will need this later...
    LOCVARSYM * thisPointer = this->thisPointer;
    bool oldUnsafe = unsafeContext;

    //
    // for all class members
    //
    FOREACHCHILD(cls, child)
        if (child->kind == SK_MEMBVARSYM) {
            MEMBVARSYM * memb = child->asMEMBVARSYM();
            //
            // if the static-ness matches and its not a constant
            //
            if ((memb->isStatic == isStatic) && (!memb->isConst || memb->type->isPredefType(PT_DECIMAL))) {
                EXPR *item = NULL;
                
                unsafeContext = !!(memb->getParseTree()->pParent->flags & NF_MOD_UNSAFE);
                unsafeErrorGiven = false;

                checkUnsafe(memb->getParseTree(), memb->type);

                //
                // check for explicit initializer
                //
                if (memb->getBaseExprTree()) {
                    ASSERT(!cls->isStruct || isStatic);
                    //
                    // we have an explicit field initializer
                    //
                    ASSERT(!memb->isUnevaled);

                    // This gives us the lefthand side, ie the field...
                    // Need to do this inline because we may be binding to a
                    // constant decimal, which won't come out right in bindExpr
                    //
                    EXPR * op1;
                    op1 = newExpr(memb->getBaseExprTree()->asBINOP()->p1, EK_FIELD, memb->type);
                    op1->asFIELD()->offset = 0;
                    op1->asFIELD()->ownerOffset = 0;
                    if (isStatic)
                        op1->asFIELD()->object = 0;
                    else
                        op1->asFIELD()->object = bindThisImplicit(memb->getBaseExprTree()->asBINOP()->p1);
                    op1->asFIELD()->field = memb;
                    op1->flags |= EXF_LVALUE;

                    this->thisPointer = NULL;
                    inFieldInitializer = true;
                    // Hide the this pointer for the duration of the right side...


                    EXPR * op2 = bindPossibleArrayInit(memb->getBaseExprTree()->asBINOP()->p2, memb->type);

                    inFieldInitializer = false;
                    this->thisPointer = thisPointer;


                    item = bindAssignment(memb->getBaseExprTree()->asBINOP(), op1, op2);
                }

                unsafeContext = oldUnsafe;

                if (item) {
                    //
                    // if we generated an initializer, wrap it up in
                    // a statement and add it to the list
                    // Use the field node for the parse tree, for the first field in each list
                    BASENODE *pNode = memb->getParseTree();
                    if (pNode && pNode->pParent) {
                        if (pNode->pParent->kind == NK_FIELD)
                            pNode = pNode->pParent;
                        else if (pNode->pParent->kind == NK_LIST && pNode == pNode->pParent->asLIST()->p1 && pNode->pParent->pParent && pNode->pParent->pParent->kind == NK_FIELD)
                            pNode = pNode->pParent->pParent;
                    }
                    newList(newExprStmtAs(pNode, item), pLast);
                }
            }
        }

    ENDFOREACHCHILD

    //
    // reenable caching of names
    //
    dontAddNamesToCache = false;
}

//
// creates a constructor call expr
//
// does overload resolution and access checks on the constructor
//
// tree         - parse tree to do error reporting
// cls          - class whose constructor we're calling
// arguments    - the constructor's argument list
// isNew
//      true    - generate a call for a new<type> expression
//                return type is the type of the class being constructed
//      false   - generate a call to the constructor only - no new
//                return type is void
//
EXPR * FUNCBREC::createConstructorCall(BASENODE *tree, AGGSYM *cls, EXPR *thisExpression, EXPR *arguments, bool isNew)
{
    //
    // find any of that classes constructors
    //
    METHSYM * method;
    addDependancyOnType(cls);
    method = compiler()->symmgr.LookupGlobalSym(compiler()->namemgr->GetPredefName(PN_CTOR), cls, MASK_METHSYM)->asMETHSYM();
    if (!method) {
        errorSym(tree, ERR_NoConstructors, cls);
        return newError(tree);
    }

    //
    // create the call expression
    // the return type is the class type for new calls or void
    // for explicit base constructor calls
    //
    EXPRCALL * expr;
    expr = newExpr(tree, EK_CALL, (isNew ? cls : getVoidType()))->asCALL();
    expr->object = thisExpression;
    expr->args = arguments;
    if (isNew) {
        expr->flags |= EXF_NEWOBJCALL | EXF_CANTBENULL;
    }
    expr->flags |= EXF_BOUNDCALL;

    //
    // do the overload resolution and access checks
    //
    expr->method = verifyMethodCall(tree, method, arguments, &thisExpression, (isNew ? EXF_NEWOBJCALL : 0))->asMETHSYM();
    if (expr->method) {
        verifyMethodArgs(expr);
    }

    if (cls->fundType() <= FT_R8 && arguments && arguments->kind != EK_LIST) {
        return expr->args;
    }

    return expr;
}

// synthetize a call to the constructor of our base class, if any...
//
// pMSym    - set to the method symbol
EXPR * FUNCBREC::createBaseConstructorCall(bool isThisCall)
{
    // check that the FNCBREC is setup the way we expect
    ASSERT(pMSym->parent == pParent);

    //
    // the class to call the base constructor on is
    // either our base class, or our own class
    //
    AGGSYM * classToCallConstructorOn;
    if (isThisCall) {
        classToCallConstructorOn = pParent->asAGGSYM();
    } else {
        classToCallConstructorOn = pParent->asAGGSYM()->baseClass;
    }

    //
    // Evaluate the arguments for our base constructor call.
    // this will do the right thing when we have no explicit
    // base constructor call: it will generate the same expr
    // as an argument list with no arguments
    //
    // NOTE: we remove the availability of the this pointer
    //       when evaluating the arguments since our base class
    //       hasn't been initialized yet
    //
    EXPR *arguments;
    if (pTree->kind == NK_CTOR) {

        // Create a new scope so that the names in the args don't interfere w/ the body
        createNewScope();
        pCurrentScope->scopeFlags |= SF_ARGSCOPE;

        // check the parse tree is as expected
        ASSERT(!(pTree->asCTOR()->pCtorArgs && !(pTree->other & (NFEX_CTOR_THIS | NFEX_CTOR_BASE))));
        ASSERT((pTree->other & (NFEX_CTOR_THIS | NFEX_CTOR_BASE)) != (NFEX_CTOR_THIS | NFEX_CTOR_BASE));

        LOCVARSYM * thisPointer = this->thisPointer;
        this->thisPointer = NULL;
        arguments = bindExpr(pTree->asCTOR()->pCtorArgs, BIND_RVALUEREQUIRED | BIND_ARGUMENTS);
        this->thisPointer = thisPointer;

        closeScope();

    } else {
        arguments = NULL;
    }

    //
    // generate the this pointer from our first param
    //
    ASSERT(thisPointer);
    EXPR * exprThis = bindThisImplicit(pTree);

    //
    // create the constructor call. Doing overload resolution and access checks.
    //
    EXPR *exprCall = createConstructorCall(pTree, classToCallConstructorOn, exprThis, arguments, false);
    if (isThisCall && pParent->asAGGSYM()->isStruct) {
        exprCall->flags |= EXF_NEWSTRUCTASSG;
    }
    else if (!isThisCall) {
        exprCall->flags |= EXF_BASECALL;
    }

    //
    // check for single step recursive constructor calls
    //
    if ((exprCall->kind == EK_CALL) && (exprCall->asCALL()->method == pMSym)) {
        compiler()->clsDeclRec.errorSymbol(pMSym, ERR_RecursiveConstructorCall);
    }

    //
    // wrap this all in a statement...
    //
    EXPRSTMTAS * rval = newExprStmtAs(pTree, exprCall);

    return rval;
}


void FUNCBREC::checkGotoJump(EXPRGOTO * expr)
{
    ASSERT(pCurrentScope->nestingOrder >= expr->targetScope->nestingOrder);
    if (pTryScope->nestingOrder > expr->targetScope->nestingOrder || pCatchScope->nestingOrder > expr->targetScope->nestingOrder) {
        expr->flags |= EXF_GOTOASLEAVE;
        if (finallyNestingCount) { 
            reachablePass = true;
            expr->flags |= EXF_GOTOASFINALLYLEAVE;
            expr->finallyStore = (FINALLYSTORE*) allocator->AllocZero (sizeof(FINALLYSTORE) * finallyNestingCount);
        }
    }

    // are we crossing a finally boundary?

    if (pFinallyScope->nestingOrder > expr->targetScope->nestingOrder) {
        errorN(expr->tree, ERR_BadFinallyLeave);
    }

}

// bind a break or continue label...
EXPR * FUNCBREC::bindBreakOrContinue(BASENODE * tree, bool asBreak)
{
    EXPRGOTO * expr;
    EXPRLABEL * target;

    expr = newExpr(tree, EK_GOTO, NULL)->asGOTO();
    // both break and continue are always forward jumps...
    expr->flags |= EXF_GOTOFORWARD;

    target = asBreak ? loopLabels->breakLabel : loopLabels->contLabel;
    if (!target) goto LERROR;
    target->reachedByBreak = true;
    expr->label = target;
    expr->targetScope = target->scope;
    expr->currentScope = pCurrentScope;

    // now, find out if we are crossing a try boundary which means that we need to make
    // this a leave instead of a br, or are crossing a finally boundary which is illegal...

    checkGotoJump(expr);

    expr->prev = gotos;
    gotos = expr;

    codeIsReachable = false;

    return expr;

LERROR:
    errorN(tree, ERR_NoBreakOrCont);
    return newError(tree);
}



// Bind the expression
EXPR * FUNCBREC::bindExpr(BASENODE *tree, int bindFlags)
{

#if DEBUG
    POSDATA pd;
    {
        if (tree) {
            if (pMSym) {
                pMSym->getInputFile()->pData->GetSingleTokenPos(tree->tokidx, &pd);
            } else if (pFSym) {
                pFSym->getInputFile()->pData->GetSingleTokenPos(tree->tokidx, &pd);
            }
        }
    }
#endif

    if (!tree) return NULL;
    
    // Remember if we are restriction to statement-expressions only. Don't propagate this value
    // to sub-expressions.
    bool stmtExprOnly = !!(bindFlags & BIND_STMTEXPRONLY);
    bindFlags &= ~BIND_STMTEXPRONLY;

    lastNode = tree;

    int mods = tree->flags & (NF_PARMMOD_REF | NF_PARMMOD_OUT);

    if (mods & NF_PARMMOD_OUT) {
        bindFlags &= ~ BIND_RVALUEREQUIRED;
        bindFlags |= BIND_MEMBERSET;
    }

    EXPR * rval;
    switch(tree->kind) {
    case NK_NEW:
        rval = bindNew(tree->asNEW(), stmtExprOnly);
        break;
    case NK_CONSTVAL:
        rval = newExprConstant(tree, getPDT(tree->PredefType()), tree->asCONSTVAL()->val);
        rval->flags |= EXF_LITERALCONST;
        break;
    case NK_BINOP:
        if (stmtExprOnly && ! opCanBeStatement[tree->Op()]) 
            errorN(tree, ERR_IllegalStatement);

        rval = bindBinop(tree->asBINOP(), bindFlags);
        break;
    case NK_UNOP:
        if (stmtExprOnly && ! opCanBeStatement[tree->Op()]) 
            errorN(tree, ERR_IllegalStatement);

        rval = bindUnop(tree->asUNOP(), bindFlags);
        break;
    case NK_LIST: {
        rval = NULL;
        EXPR ** prval = &rval;
        if (stmtExprOnly)
            bindFlags |= BIND_STMTEXPRONLY;
        NODELOOP(tree, BASE, elem)
            EXPR * item = bindExpr(elem, bindFlags);
            newList(item, &prval);
        ENDLOOP;
        break;
                  }
    case NK_NAME:
        if (stmtExprOnly) 
            errorN(tree, ERR_IllegalStatement);
            
        rval = bindName(tree->asNAME(), MASK_LOCVARSYM | MASK_MEMBVARSYM | MASK_PROPSYM, bindFlags);
        // since we could have bound to a constant expression, we do not
        // mark this as an lvalue here, but rather let bindName do it...
        break;
    case NK_DOT:
        if (stmtExprOnly) 
            errorN(tree, ERR_IllegalStatement);
            
        rval = bindDot(tree->asDOT(), MASK_MEMBVARSYM | MASK_PROPSYM, bindFlags);
        break;
    case NK_ARRAYINIT:
        errorN(tree, ERR_ArrayInitInBadPlace);
        rval = newError(tree);
        break;
    case NK_OP:
        if (stmtExprOnly && ! opCanBeStatement[tree->Op()]) 
            errorN(tree, ERR_IllegalStatement);

        CONSTVAL c;
        switch(tree->Op()) {
        case OP_NULL:
            rval = bindNull(tree);
            break;

        case OP_TRUE:
            c.iVal = true;
            goto OPBOOL;

        case OP_FALSE:
            c.iVal = false;
OPBOOL:
            rval = newExprConstant(tree, getPDT(PT_BOOL), c);
            rval->flags |= EXF_LITERALCONST;
            break;

        case OP_THIS:
            rval = bindThisExplicit(tree);
            break;

        case OP_ARGS:
            if (pMSym->isVarargs) {
                rval = newExprBinop(tree, EK_ARGS, getPDT(PT_ARGUMENTHANDLE), NULL, NULL);
            } else {
                errorN(tree, ERR_ArgsInvalid);
                rval = newError(tree);
            }
            break;

        case OP_BASE:

            errorN(tree, ERR_BaseIllegal);
            rval = newError(tree);

            break;

        default:
            ASSERT(!"Bogus operator");
            rval = NULL;
        }
        break;
    case NK_TYPE: {
        TYPESYM * type = bindType(tree->asTYPE());
        if (!type) return newError(tree);

        rval = newExpr(tree, EK_CLASS, type);
        break;
                  }
    case NK_ARROW:
        rval = bindDot(tree->asARROW(), MASK_MEMBVARSYM | MASK_PROPSYM, bindFlags);
        break;
    default:
        ASSERT(!"unknown expression");
        rval = NULL;
    }

    if (mods) {
        checkLvalue(rval, false);
        // do not give this error if we also failed the lvalue check...
        if (rval->kind == EK_PROP && (rval->flags & EXF_LVALUE)) {
            errorN(tree, ERR_RefProperty);
        }
        if (rval->kind == EK_FIELD) {
            checkFieldRef(rval->asFIELD(), true);
        }
        noteReference(rval);
        if (mods & NF_PARMMOD_OUT) {
            if (rval->kind == EK_LOCAL) {
                LOCVARSYM * local = rval->asLOCAL()->local;
                if (!local->slot.hasInit || (local == thisPointer && pMSym->isCtor)) {
                    reachablePass = true;
                    if (!local->slot.ilSlot) {
                        local->slot.ilSlot = ++uninitedVarCount;
                        if (local->type->fundType() == FT_STRUCT) {
                            uninitedVarCount += local->type->asAGGSYM()->getFieldsSize() - 1;
                            maybeSetOwnerOffset(local);
                        }
                    }
                }
            }
            rval->type = compiler()->symmgr.GetParamModifier(rval->type, true);
        } else {
            rval->type = compiler()->symmgr.GetParamModifier(rval->type, false);
        }
    }

    if (rval->type) {
        checkUnsafe(tree, rval->type);
        compiler()->symmgr.DeclareType (rval->type);    
    }

    return rval;
}


EXPR * FUNCBREC::bindNull(BASENODE * tree)
{
    CONSTVAL cv;
    cv.strVal = NULL;
    return newExprConstant(tree, compiler()->symmgr.GetNullType(), cv);
}


EXPR * FUNCBREC::bindDelegateNew(AGGSYM * type, NEWNODE * tree)
{

    // First, find the constructor:
    METHSYM * method;
    BASENODE * ntree;
    METHSYM * invoke;
    EXPR * expr;
    EXPRCALL * call;

    method = compiler()->symmgr.LookupGlobalSym(compiler()->namemgr->GetPredefName(PN_CTOR), type, MASK_METHSYM)->asMETHSYM();
    while (method) {
        // The constructor has got to take an object and an uintptr...
        if (method->cParams == 2 && method->params[0]->isPredefType(PT_OBJECT) &&
            (method->params[1]->isPredefType(PT_UINTPTR) || method->params[1]->isPredefType(PT_INTPTR))) 
            break;
        
        method = compiler()->symmgr.LookupNextSym(method, type, MASK_METHSYM)->asMETHSYM();
    }

    if (!method) {
        errorSym(tree, ERR_BadDelegateConstructor, type);
        goto LERROR;
    }

    invoke = compiler()->symmgr.LookupGlobalSym(compiler()->namemgr->GetPredefName(PN_INVOKE), type, MASK_METHSYM)->asMETHSYM();
    if (!invoke) {
        errorSym(tree, ERR_NoInvoke, type);
        goto LERROR;
    }

    ntree = tree->pArgs;
    ASSERT(ntree);

    expr = bindMethodName(ntree);
    RETBAD(expr);

    call = expr->asCALL();

    invoke = verifyMethodCall(ntree, call->method, NULL, &(call->object), expr->flags & EXF_BASECALL, invoke)->asMETHSYM();
    if (!invoke) goto LERROR;

    EXPRFUNCPTR * funcPtr;
    funcPtr = newExpr(tree->pArgs, EK_FUNCPTR, getVoidType())->asFUNCPTR();
    funcPtr->func = invoke;
    funcPtr->flags |= (call->flags & EXF_BASECALL);

    if (!invoke->isStatic) {
        EXPR * obj;
        
        obj = funcPtr->object = call->object;

        if (obj && obj->type->fundType() != FT_REF) {
            // Must box the object before creating a delegate to it.
            obj = mustConvert(obj, getPDT(PT_OBJECT), tree);
        }

        call->args = newExprBinop(NULL, EK_LIST, getVoidType(), obj, funcPtr);
    } else {
        funcPtr->object = NULL;
        call->args = newExprBinop(NULL, EK_LIST, getVoidType(), bindNull(NULL), funcPtr);
    }
    call->object = NULL;
    call->method = method;
    call->flags = EXF_NEWOBJCALL | EXF_BOUNDCALL | EXF_CANTBENULL;
    call->tree = tree;
    call->type = type;

    return call;

LERROR:
    return newError(tree);
}

// bind the new keyword...
EXPR * FUNCBREC::bindNew(NEWNODE * tree, bool stmtExprOnly)
{
    EXPR * rval = NULL;
    TYPESYM * type = bindType(tree->pType);
    if (!type) return newError(tree);
    TYPESYM * coclassType = type;

    if (tree->flags & NF_NEW_STACKALLOC) {
        rval = bindLocAlloc(tree, type);
        goto CHECKSTMTONLY;
    }

    // arrays are done separately...
    if (type->kind == SK_ARRAYSYM) {
        rval = bindArrayNew(type->asARRAYSYM(), tree);
        goto CHECKSTMTONLY;
    }

    if (type->asAGGSYM()->isDelegate && tree->pArgs != NULL && tree->pArgs->kind != EK_LIST) {
        // this could be a special new which takes a func pointer:
        rval = bindDelegateNew(type->asAGGSYM(), tree);
CHECKSTMTONLY:
        if (stmtExprOnly) {
            errorN(tree, ERR_IllegalStatement);
        }
        return rval;
    }


    // first the args...
    EXPR * args = bindExpr(tree->pArgs, BIND_RVALUEREQUIRED | BIND_ARGUMENTS);

    // Parameterless struct constructor calls == zero init
    if (!args 
        && (type->asAGGSYM()->isStruct || type->asAGGSYM()->isEnum) 
        && (!type->asAGGSYM()->hasZeroParamConstructor || type->asAGGSYM()->isSimpleType())) {
        return newExprZero(tree, type);
    }

    if (type->asAGGSYM()->isAbstract) {
        if (type->asAGGSYM()->isInterface && type->asAGGSYM()->comImportCoClass ) {
            AGGSYM * agg = type->asAGGSYM();
            if (agg->underlyingType) {
                // The coclass has already been resolved
                if (agg->underlyingType == agg) {
                    // This means the coclass was not imported, an error was already given
                    // when we first discovered this, so don't give another one
                    goto LERROR;
                } else {
                    // Check accessibility
                    SYM * badAccess = NULL;
                    for (SYM * p = agg->underlyingType; p && p->kind == SK_AGGSYM; p = p->parent) {
                        if (!compiler()->clsDeclRec.checkAccess(p, pParent, NULL, NULL)) {
                            // find the outer-most unaccessible symbol
                            badAccess = p;
                        }
                    }
                    if (badAccess) {
                        errorSym(tree->pType, ERR_BadAccess, badAccess);
                        goto LERROR;
                    }

                    // just new the coclass instead of the interface
                    coclassType = agg->underlyingType;
                }
            } else {
                // Resolve the coclass text
                AGGSYM * coclass = compiler()->importer.ResolveTypeName(agg->comImportCoClass, agg->getInputFile(), agg->getImportScope(), mdAssemblyRefNil, true, true);

                // Check accessibility
                SYM * badAccess = NULL;
                for (SYM * p = coclass; p && p->kind == SK_AGGSYM; p = p->parent) {
                    if (!compiler()->clsDeclRec.checkAccess(p, pParent, NULL, NULL)) {
                        // find the outer-most unaccessible symbol
                        badAccess = p;
                    }
                }
                if (badAccess) {
                    errorSym(tree->pType, ERR_BadAccess, badAccess);
                } else if (coclass) {
                    // Check bogus-ness, decprecated, etc.
                    compiler()->clsDeclRec.CheckSymbol(tree->pType, coclass, pParent, 0);
                }

                if (coclass != NULL) {
                    agg->underlyingType = coclass;
                    if (badAccess)
                        goto LERROR;
                    else
                        coclassType = coclass;
                } else {
                    errorStrSym(tree->pType, ERR_MissingCoClass, agg->comImportCoClass, agg);
                    // Set the coclass to this, so we don't more errors on this same interface
                    agg->underlyingType = agg;
                    goto LERROR;
                }
            }
        }

        if (coclassType->asAGGSYM()->isAbstract) {
            errorSym(tree, ERR_NoNewAbstract, type);
            goto LERROR;
        }
    }

    // now find any of that classes constructors
    rval = createConstructorCall(tree, coclassType->asAGGSYM(), NULL, args, true);
    if (coclassType != type)
        rval = mustCast(rval, type, tree);

    return rval;

LERROR:
    return newError(tree);

}

void FUNCBREC::bindArrayInitList(UNOPNODE * tree, TYPESYM * elemType, int rank, int * size, EXPR *** ppList, int * totCount, int * constCount)
{
    int count = 0;
 
    NODELOOP(tree->p1, BASE, item)
        count++;
        if (rank == 1) {
            EXPR * expr = mustConvert(bindExpr(item), elemType, item);
            if (expr->kind == EK_CONSTANT) {
                if (!expr->isDefValue()) {
                    (*constCount)++;
                }
            } else {
                (*totCount)++;
            }
            newList(expr, ppList);
        } else {
            if (item->kind == NK_ARRAYINIT && item->asARRAYINIT()->p1) {
                bindArrayInitList(item->asARRAYINIT(), elemType, rank - 1, size + 1, ppList, totCount, constCount);
            } else {
                errorN(item, ERR_InvalidArray);
            }
        }
    ENDLOOP;

    if (size[0] != -1) {
        if (size[0] != count) {
            errorN(tree, ERR_InvalidArray);
        }
    } else {
        size[0] = count;
    }
}


EXPR * FUNCBREC::bindArrayInit(UNOPNODE * tree, ARRAYSYM * type, EXPR * args)
{
    
    EXPRARRINIT * rval = newExpr(tree, EK_ARRINIT, type)->asARRINIT();

    TYPESYM * elemType = type->elementType();

    int * pointer;
    int * pointerEnd;

    if (type->rank > 1) {
        rval->dimSizes = (int*) allocator->Alloc(sizeof (int) * type->rank);
    } else {
        rval->dimSizes = &(rval->dimSize);
    }
    pointer = rval->dimSizes;
    pointerEnd = pointer + type->rank;

    if (args) {
        EXPRLOOP(args, arg)
            if (pointer == pointerEnd) {
                errorN(arg->tree, ERR_InvalidArray);
                break;
            }
            if (arg->kind != EK_CONSTANT) {
                errorN(arg->tree, ERR_ConstantExpected);
                *pointer = -1;
            } else {
                *pointer = arg->asCONSTANT()->getVal().iVal;
            }
            pointer++;
        ENDLOOP;
        if (pointer != pointerEnd) {
            errorN(args->tree, ERR_InvalidArray);
        }
    }

    memset(pointer, -1, (pointerEnd - pointer) * sizeof (int));

    rval->args = NULL;
    EXPR ** pList = &(rval->args);

    bool constant = elemType->isSimpleType() && compiler()->symmgr.GetAttrArgSize((PREDEFTYPE) elemType->asAGGSYM()->iPredef) > 0;
    int totCount = 0; // the count of non-constants
    int constCount = 0; // the count of non-zero constants

    bindArrayInitList(tree, elemType, type->rank, rval->dimSizes, &pList, &totCount, &constCount);

    if (constant && (constCount > 2) && (constCount * 3 >= totCount) && info != NULL) {
        rval->flags |= (totCount == 0) ? EXF_ARRAYALLCONST : EXF_ARRAYCONST;
        if (!info->hInitArray) {
            info->hInitArray = findMethod(tree, PN_INITIALIZEARRAY, getPDT(PT_RUNTIMEHELPERS)->asAGGSYM());
        }
    }
    
    return rval;
}

// Check an expression for a negative constant value, and issue error/warning if so.
void FUNCBREC::checkNegativeConstant(BASENODE * tree, EXPR * expr, int id)
{
    if (expr->kind == EK_CONSTANT && expr->type->fundType() != FT_U8 && expr->asCONSTANT()->getI64Value() < 0) 
        errorN(tree, id);
}


// bind an array creation expression...
EXPR * FUNCBREC::bindArrayNew(ARRAYSYM * type, NEWNODE * tree)
{

    EXPR * args = NULL;
    EXPR ** pArgs = &args;
    EXPR * temp, *expr;
    TYPESYM * destType;
    int count = 0;

    AGGSYM * intType = getPDT(PT_INT)->asAGGSYM();

    NODELOOP(tree->pArgs, BASE, arg)
        count ++;
        expr = bindExpr(arg);
        destType = chooseArrayIndexType(tree, expr); // use int, long, uint, or ulong?
        if (!destType) {
            // using int as the type will allow us to give a better error...
            destType = intType;
        }
        temp = mustConvert(expr, destType, arg);
        checkNegativeConstant(arg, temp, ERR_NegativeArraySize);

        if (destType != getPDT(PT_UINT)) {
            // use UINT because that's what the EE uses -- negative should cause exception!
            temp = mustCast(temp, getPDT(PT_UINT), arg, CONVERT_CHECKOVERFLOW);
        }
        newList(temp, &pArgs);
    ENDLOOP;

    if (tree->pInit) {
        return bindArrayInit(tree->pInit->asARRAYINIT(), type, args);
    }

    if (count != type->rank) {
        errorInt(tree, ERR_BadIndexCount, type->rank);
    }


    EXPR * rval = newExprBinop(tree, EK_NEWARRAY, type, args, NULL);
    rval->flags |= EXF_ASSGOP | EXF_CANTBENULL;

    return rval;
}

EXPR * FUNCBREC::bindPossibleArrayInit(BASENODE * tree, TYPESYM * destinationType, int bindFlags)
{
    if (!tree) {
        return NULL;
    }

    if (tree->kind == NK_ARRAYINIT) {
        if (destinationType->kind != SK_ARRAYSYM) {
            errorN(tree, ERR_ArrayInitToNonArrayType);
            return newError(tree);
        }
        return bindArrayInit(tree->asARRAYINIT(), destinationType->asARRAYSYM(), NULL);
    }

    return bindExpr(tree, bindFlags | BIND_RVALUEREQUIRED);
}

EXPR * FUNCBREC::bindPossibleArrayInitAssg(BINOPNODE * tree, TYPESYM * type, int bindFlags)
{
    if (!tree) {
        return NULL;
    }

    EXPR * op1 = bindExpr(tree->p1, BIND_MEMBERSET | (bindFlags & BIND_USINGVALUE));
    RETBAD(op1);
    EXPR * op2 = bindPossibleArrayInit(tree->p2, type, bindFlags);
    RETBAD(op2);

    return bindAssignment(tree, op1, op2);
}

// Bind a return statement...  Check that the value is of the correct type...
EXPR * FUNCBREC::bindReturn(EXPRSTMTNODE * tree)
{

    // First, see if inside of a finally block:

    if (pFinallyScope != pOuterScope) {
        errorN(tree, ERR_BadFinallyLeave);
    }

    EXPR * arg = bindExpr(tree->pArg);

    RETBAD(arg);

    TYPESYM * retType = pMSym->retType;

    if (retType == getVoidType()) {
        if (arg) {
            errorSym(tree, ERR_RetNoObjectRequired, pMSym);
            goto LERROR;
        }
    } else {
        if (!arg) {
            errorSym(tree, ERR_RetObjectRequired, retType);
            goto LERROR;
        }

        checkUnsafe(tree->pArg, retType);

        arg = mustConvert(arg, retType, tree->pArg);
    }

    EXPRRETURN * rval;
    rval = newExpr(tree, EK_RETURN, NULL)->asRETURN();
    rval->object = arg;

    if (pTryScope != pOuterScope || pCatchScope != pOuterScope) {
        rval->currentScope = pCurrentScope;
        rval->flags |= EXF_RETURNASLEAVE;
        if (finallyNestingCount) {
            reachablePass = true;
            rval->flags |= EXF_RETURNASFINALLYLEAVE;
            rval->finallyStore = (FINALLYSTORE*) allocator->AllocZero (sizeof(FINALLYSTORE) * finallyNestingCount);
        }
        info->hasRetAsLeave = true;
    }

    codeIsReachable = false;

    return rval;

LERROR:

    return newError(tree);
}

EXPR * FUNCBREC::bindThrow(EXPRSTMTNODE * tree)
{
    EXPRTHROW * rval = newExpr(tree, EK_THROW, NULL)->asTHROW();

    if (tree->pArg) {
        EXPR * expr = bindExpr(tree->pArg);
        if (!canConvert(expr, compiler()->symmgr.GetPredefType(PT_EXCEPTION), tree->pArg)) {
            errorN(tree->pArg, ERR_BadExceptionType);
        }
        rval->object = expr;
    } else {

        rval->object = NULL;
        // find out if we are enclosed in a catch scope...

        SCOPESYM * scope = pCurrentScope;
        do {
            if (scope->scopeFlags & SF_CATCHSCOPE) break;
            scope = scope->parent->asSCOPESYM();
        } while (scope);

        if (scope) {
            scope->scopeFlags |= SF_HASRETHROW;
        } else {
            errorN(tree, ERR_BadEmptyThrow);
        } 
    }

    codeIsReachable = false;

    return rval;
}

EXPR * FUNCBREC::bindTry(TRYSTMTNODE * tree)
{

    SCOPESYM *scope;
    EXPRTRY * rval = newExpr(tree, EK_TRY, NULL)->asTRY();
    rval->handlers = NULL;
    EXPR ** pList = &(rval->handlers);

    if (!(tree->flags & NF_TRY_CATCH)) {
        finallyNestingCount ++;
    }

    SCOPESYM * tryScope;
    rval->tryblock = bindBlock(tree->pBlock, SF_TRYSCOPE | (!(tree->flags & NF_TRY_CATCH) ? SF_TRYOFFINALLY : 0), &tryScope);

    bool reachableFromAny = codeIsReachable;

    if (tree->flags & NF_TRY_CATCH) {


        NODELOOP(tree->pCatch, CATCH, handler)
            EXPRHANDLER * expr = newExpr(handler, EK_HANDLER, NULL)->asHANDLER();
            lastNode = handler;
            expr->param = NULL;

            bool declared = false;

            if (handler->pType) {
                lastNode = handler->pType;
                TYPESYM * type =  bindType(handler->pType->asTYPE());
                if (type) {
                    if (!canConvert(type, compiler()->symmgr.GetPredefType(PT_EXCEPTION),handler->pType)) {
                        errorN(handler->pType, ERR_BadExceptionType);
                    } else {
                        expr->type = type;
                        // need to check whether this would even be reachable...
                        EXPRLOOP(rval->handlers, hand)
                            EXPRHANDLER * prevH = hand->asHANDLER();
                            if (prevH->type && canConvert(type, prevH->type, handler->pType)) {
                                errorSym(handler->pType, ERR_UnreachableCatch, prevH->type);
                                break;
                            }
                        ENDLOOP;
                    }
                    if (handler->pName) {
                        unreferencedVarCount++;
                        lastNode = handler->pName;
                        createNewScope();
                        declared = true;
                        LOCVARSYM * var = declareVar(handler, handler->pName->pName, type);
                        if (var) {
                            expr->param = var;
                            var->slot.hasInit = true;
                        }
                    }
                }
            } else {
                expr->type = compiler()->symmgr.GetPredefType(PT_OBJECT);
            }

            // assume the exception may happen:

            codeIsReachable = true;

            expr->handlerBlock = bindBlock(handler->pBlock, SF_CATCHSCOPE, &scope);
            if (scope->scopeFlags & SF_HASRETHROW) {
                expr->flags |= EXF_HASRETHROW;
            }

            reachableFromAny |= codeIsReachable;

            if (declared) {
                expr->handlerBlock->scopeSymbol = pCurrentScope; // bind scope symbol to the outer scope.
                closeScope();
            }

            newList(expr, &pList);

        ENDLOOP;

        codeIsReachable = reachableFromAny;

    } else { // finally...

        finallyNestingCount--;

        codeIsReachable = true;

        rval->handlers = bindBlock(tree->pCatch->asBLOCK(), SF_FINALLYSCOPE, &(tryScope->finallyScope));
        rval->flags |= EXF_ISFINALLY;
        rval->finallyStore = (FINALLYSTORE*) allocator->AllocZero(sizeof(FINALLYSTORE));

        if (!codeIsReachable) {
            rval->flags |= EXF_FINALLYBLOCKED;    
        }

        // code after the finally is reachable if both the finally end falls through,
        // and the try end fell through...
        codeIsReachable = codeIsReachable && reachableFromAny;

    }

    return rval;
}


NAME * FUNCBREC::getSwitchLabelName(EXPRCONSTANT * expr)
{
    // this returns "default:" which relies on the : to guarantee uniqueness...
    if (!expr) return compiler()->namemgr->GetPredefName(PN_DEFAULT);

    WCHAR bufferW[256];

    if (expr->type->isPredefType(PT_STRING)) {
        int len = expr->isNull() ? 4 : expr->getSVal().strVal->length;
        WCHAR * buffer = (WCHAR*) allocator->Alloc(sizeof(WCHAR) * 2 * (len + 10));
        if (expr->isNull()) {
            memcpy(buffer, L"case null:", sizeof(WCHAR) * 11);
        } else {
            memcpy(buffer, L"case \"", sizeof(WCHAR) * 6);
            WCHAR * ptr = buffer + 6;
            WCHAR * text = expr->getSVal().strVal->text;
            WCHAR * textEnd = text + len;
            while (text < textEnd) {
                if (*text == 0) {
                    *(ptr++) = L'\\';
                    *(ptr++) = L'0';
                } else if (*text == L'\\') {
                    *(ptr++) = L'\\';
                    *(ptr++) = L'\\';
                } else {
                    *(ptr++) = *text;
                }
                text++;
            }
            *(ptr++) = L'\"';
            *(ptr++) = L':';
            *ptr = (WCHAR) NULL;
        }

        return compiler()->namemgr->AddString(buffer);

    } else {

        // The space and the : below guarantee uniqueness...
        if (!swprintf(bufferW, L"case %I64d:", expr->getI64Value()) && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED) {
            CHAR bufferA[256];
            sprintf(bufferA, "case %I64d:", expr->getI64Value());
            MultiByteToWideChar(CP_ACP, 0, bufferA, -1, bufferW, sizeof(bufferW) / sizeof(WCHAR));
        }
    }

    return compiler()->namemgr->AddString(bufferW);
}


int FUNCBREC::compareSwitchLabels(const void *l1, const void *l2)
{
    EXPRSWITCHLABEL * e1 = *(EXPRSWITCHLABEL**)l1;
    EXPRSWITCHLABEL * e2 = *(EXPRSWITCHLABEL**)l2;

    if (e1 == e2) return 0;

    if (e1->key && e2->key) {
    
        __int64 v1 = e1->key->asCONSTANT()->getI64Value();
        __int64 v2 = e2->key->asCONSTANT()->getI64Value();
        if (v1 < v2) return -1;
        if (v2 < v1) return 1;
        return 0;
    } else if (!e1->key) {
        return 1;
    } else return -1;
}


void FUNCBREC::initForNonHashtableSwitch(BASENODE * tree)
{
    if (!info->hIntern) {
        TYPESYM * oneArg[1];
        oneArg[0] = getPDT(PT_STRING);
        TYPESYM ** oneArgList = compiler()->symmgr.AllocParams(1, oneArg);

        info->hIntern = findMethod(tree, PN_ISINTERNED, getPDT(PT_STRING)->asAGGSYM(), &oneArgList);
    }
}

void FUNCBREC::initForHashtableSwitch(BASENODE * tree, EXPRSWITCH * expr)
{
    expr->flags |= EXF_HASHTABLESWITCH;

    if (!info->hTableCreate) {
        TYPESYM * twoArgs[2];
        twoArgs[0] = getPDT(PT_INT);
        twoArgs[1] = getPDT(PT_FLOAT);
        TYPESYM ** twoArgList = compiler()->symmgr.AllocParams(2, twoArgs);

        info->hTableCreate = findMethod(tree, PN_CTOR, getPDT(PT_HASHTABLE)->asAGGSYM(), &twoArgList);
        if (!info->hTableCreate) return;

        twoArgs[0] = twoArgs[1] = getPDO();
        twoArgList = compiler()->symmgr.AllocParams(2, twoArgs);

        info->hTableAdd = findMethod(tree, PN_ADD, getPDT(PT_HASHTABLE)->asAGGSYM(), &twoArgList);
        if (!info->hTableAdd) return;

        twoArgList = compiler()->symmgr.AllocParams(1, twoArgs);

        info->hTableGet = findMethod(tree, PN_GETITEM, getPDT(PT_HASHTABLE)->asAGGSYM(), &twoArgList);
        if (!info->hTableGet) return;
    }

    if (info->switchList) {
        info->switchList = newExprBinop(NULL, EK_LIST, NULL, expr, info->switchList);
    } else {
        info->switchList = expr;
    }
}



EXPR * FUNCBREC::bindSwitch(SWITCHSTMTNODE * tree, EXPR *** newLast)
{
    EXPRSWITCH * rval = newExpr(tree, EK_SWITCH, NULL)->asSWITCH();
    rval->hashtableToken = 0;
    rval->nullLabel = NULL;

    rval->arg = bindExpr(tree->pExpr);

    bool isConstant = rval->arg->kind == EK_CONSTANT;
    CONSTVAL val;
    EXPRGOTO * constantGoto = NULL;
    if (isConstant) {
        val = rval->arg->asCONSTANT()->getSVal();
        constantGoto = newExpr(tree, EK_GOTO, NULL)->asGOTO();
        constantGoto->flags |= EXF_GOTOFORWARD;
        constantGoto->label = NULL;
    }

    TYPESYM * stringType = getPDT(PT_STRING);

    TYPESYM * type = rval->arg->type;
    compiler()->symmgr.DeclareType (type);
    if (type->fundType() > FT_LASTINTEGRAL && type != stringType) {
        int matches = 0;
        TYPESYM * dest = NULL, * toType = NULL;
        SYMLIST * listSrc = (type->kind == SK_AGGSYM) ? compiler()->clsDeclRec.getConversionList(type->asAGGSYM()) : NULL;
        METHSYM * conversion;

        while( listSrc) {
            conversion = listSrc->sym->asMETHSYM();
            ASSERT(conversion->cParams == 1);
            if (conversion->isImplicit) {
                toType = conversion->retType;
                if ((toType->isNumericType() && toType->fundType() <= FT_LASTINTEGRAL) || toType == stringType) {
                    matches++;
                    dest = toType;
                }
            }
            listSrc = listSrc->next;
        }
        if (matches != 1) {
            errorN(tree->pExpr, ERR_IntegralTypeValueExpected);
            type = compiler()->symmgr.GetErrorSym();
        } else {
            rval->arg = tryConvert( rval->arg, dest, tree);
            type = rval->arg->type;
        }
    }

    // ok, create labels for all case statements:

    SCOPESYM * prevSwitch = pSwitchScope;
    createNewScope();
    pSwitchScope = pCurrentScope;
    pCurrentScope->scopeFlags |= SF_SWITCHSCOPE;

    LOOPLABELS * prev = loopLabels;
    LOOPLABELS ll(this);
    loopLabels->contLabel = prev->contLabel;
    rval->breakLabel = loopLabels->breakLabel;

    unsigned count = 0;
    NODELOOP(tree->pCases, CASE, codeLab)
        NODELOOP(codeLab->pLabels, BASE, caseLab)
            count++;
        ENDLOOP;
    ENDLOOP;

    if (type == stringType) {
        if (count >= 10 && compiler()->symmgr.GetPredefType(PT_HASHTABLE, false)) {
            initForHashtableSwitch(tree, rval);
        } else {
            initForNonHashtableSwitch(tree);
        }
    }

    EXPRSWITCHLABEL ** labArray = (EXPRSWITCHLABEL**) allocator->Alloc(sizeof(EXPRSWITCHLABEL*) * count);
    rval->labels = labArray;

    EXPR * labelList = NULL;
    EXPR ** pLabelList = &labelList;

    NODELOOP(tree->pCases, CASE, codeLab)

        NAME * labelName = NULL;
        EXPRSWITCHLABEL * label = NULL;

        NODELOOP(codeLab->pLabels, BASE, caseLab)
            EXPR * key;
            if (caseLab->asCASELABEL()->p1) {
                bool ok;
                key = verifySwitchLabel(caseLab->asCASELABEL()->p1, type, &ok);
                if (!ok) {
                    count --;
                    continue;
                }
            } else {
                rval->flags |= EXF_HASDEFAULT;
                key = NULL;
            }
            labelName = getSwitchLabelName(key->asCONSTANT());
            LABELSYM * sym = compiler()->symmgr.LookupLocalSym(labelName, pCurrentScope, MASK_LABELSYM)->asLABELSYM();
            if (sym) {
                errorName(caseLab->asCASELABEL()->p1, ERR_DuplicateCaseLabel, labelName);
            } else {
                sym = compiler()->symmgr.CreateLocalSym(SK_LABELSYM, labelName, pCurrentScope)->asLABELSYM();
            }
            label = newExpr(caseLab, EK_SWITCHLABEL, NULL)->asSWITCHLABEL();
            if (key && key->isNull()) {
                rval->nullLabel = label;
            }
            label->statements = NULL;
            label->block = NULL;
            label->owningBlock = pCurrentBlock;
            label->key = key;
            label->entranceBitset = NULL;
            label->definitelyReachable = false;
            label->reachedByBreak = false;
            label->fellThrough = false;
            label->flags |= EXF_HASNOCODE;
            if (isConstant) {
                if (!key) {
                    if (!constantGoto->label) {
                        constantGoto->label = label;
                    }
                } else if (key->asCONSTANT()->isEqual(rval->arg->asCONSTANT())) {
                    constantGoto->label = label;
                }
                reachablePass = true;  
                unreachedLabelCount++;
                label->userDefined = true;
                newList(label, &pUserLabelList);
            } else {
                label->userDefined = false;
            }
            sym->labelExpr = label;
            *labArray = label;
            labArray ++;
            newList(label, &pLabelList);
        ENDLOOP;

        if (!label) continue;
        label->statements = NULL;
        EXPR ** pList = &(label->statements);

        EXPR ** newLast = NULL;
        STATEMENTNODE * stms = codeLab->pStmts;

        codeIsReachable = true;

        while (stms) {
            newList(bindStatement(stms, &newLast), &pList);
            if (newLast) pList = newLast;
            stms = stms ->pNext;
        }

        if (label->statements) {
            label->flags &= ~EXF_HASNOCODE;
        }

        // Note that we will ignore reachability if there are any user labels present since
        // then we are guaranteed a second pass, and since we assumed that all labels
        // are reachable, we don't really know if the one falsely made use think that this
        // code is reachable...
        if (codeIsReachable && !unreachedLabelCount && !reachablePass) {
            label->fellThrough = true;
            errorName(tree, ERR_SwitchFallThrough, labelName);
        }

    ENDLOOP;


    codeIsReachable =
        ll.breakLabel->reachedByBreak ||
        !(rval->flags & EXF_HASDEFAULT) ||
        codeIsReachable; // from the default...

    if (type != stringType) {
        qsort(rval->labels, count, sizeof(EXPR*), &FUNCBREC::compareSwitchLabels);
    } else if (rval->flags & EXF_HASDEFAULT) {
        // have to move the default to the last position...
        for (unsigned int cc = 0; cc < count; cc++) {
            if (!rval->labels[cc]->key) {
                EXPRSWITCHLABEL * def = rval->labels[cc];
				unsigned int jj;
                for (jj = cc + 1; jj < count; jj++) {
                    rval->labels[jj - 1] = rval->labels[jj];
                }
                rval->labels[jj-1] = def;
                break;
            }
        }
    }

    rval->bodies = labelList;
    rval->labelCount = count;

    loopLabels = prev;
    closeScope();
    pSwitchScope = prevSwitch;

    EXPR * list = NULL;
    EXPR ** pList = &list;
    if (isConstant) {
        if (!constantGoto->label) {
            constantGoto->label = ll.breakLabel;
        }
        newList(constantGoto, &pList);
    }
    newList(rval, &pList);
    newList(ll.breakLabel, &pList);
    *newLast = pList;
    return list;

}


EXPR * FUNCBREC::bindLock(LOOPSTMTNODE * tree, EXPR *** newLast)
{

    EXPR * arg = bindExpr(tree->pExpr);

    RETBAD(arg);
    if (arg->type->fundType() != FT_REF) {
        errorSym(tree, ERR_LockNeedsReference, arg->type);
    }

    EXPRWRAP * wrap = newExprWrap(arg, TK_LOCK);
    wrap->flags |= EXF_WRAPASTEMP;
    arg = newExprBinop(arg->tree, EK_SAVE, arg->type, arg, wrap);

    EXPRTRY * tryExpr = newExpr(tree, EK_TRY, NULL)->asTRY();
    tryExpr->flags |= EXF_ISFINALLY;
    tryExpr->finallyStore = (FINALLYSTORE*) allocator->AllocZero(sizeof(FINALLYSTORE));

    finallyNestingCount ++;

    EXPR * body = bindBlock(tree->pStmt, SF_TRYSCOPE);

    finallyNestingCount --;

    tryExpr->tryblock = body->asBLOCK();

    METHSYM * exitM = findMethod(tree, PN_EXIT, getPDT(PT_CRITICAL)->asAGGSYM());
    if (!exitM) return newError(tree);

    EXPRCALL * call = newExpr(tree, EK_CALL, exitM->retType)->asCALL();
    call->args = wrap;
    call->object = NULL;
    call->method = exitM;
    call->flags |= EXF_BOUNDCALL;

    EXPR * stmt = newExprStmtAs(NULL, call);
    stmt->flags |= EXF_NODEBUGINFO;  //                               don't emit debug info for the unlocking.
    body = newExprBlock(tree);
    body->asBLOCK()->statements = stmt;

    tryExpr->handlers = body;

    METHSYM * enterM = findMethod(tree, PN_ENTER, getPDT(PT_CRITICAL)->asAGGSYM());
    if (!enterM) return newError(tree);

    call = newExpr(tree, EK_CALL, enterM->retType)->asCALL();
    call->args = arg;
    call->object = NULL;
    call->method = enterM;
    call->flags |= EXF_BOUNDCALL;

    EXPR * list = newExprStmtAs(tree, call);
    EXPR ** pList = &list;
    newList(tryExpr, &pList);
    *newLast = pList;

    return list;
}



EXPR * FUNCBREC::bindUsingDecls(FORSTMTNODE * tree, EXPR * inits)
{
    EXPR * next;
    if (inits->kind == EK_LIST) {
        next = inits->asBIN()->p2;
        inits = inits->asBIN()->p1;
    } else {
        next = NULL;
    }
    return bindUsingDecls(tree, inits, next);
}

EXPR * FUNCBREC::bindUsingDecls(FORSTMTNODE * tree, EXPR * first, EXPR * next)
{
    EXPR * list = NULL;
    EXPR ** pList = &list;


    EXPRWRAP * wrap = NULL;

    if (first->kind == EK_DECL) {
        newList(first, &pList);
    } else {
        wrap = newExprWrap(first, TK_USING);
        wrap->flags |= EXF_WRAPASTEMP;
        wrap->doNotFree = true;
        wrap->needEmptySlot = true;
        
        EXPR * assignment;
        assignment = newExprStmtAs(tree->pExpr, bindAssignment(tree->pExpr, wrap, first));
        newList(assignment, &pList);
    }

    EXPRTRY * tryExpr = newExpr(tree, EK_TRY, NULL)->asTRY();

    newList(tryExpr, &pList);

    tryExpr->flags |= EXF_ISFINALLY;
    tryExpr->finallyStore = (FINALLYSTORE*) allocator->AllocZero(sizeof(FINALLYSTORE));

    finallyNestingCount ++;

    createNewScope();
    pCurrentScope->scopeFlags = SF_TRYSCOPE;
    pCurrentBlock = newExprBlock(tree);
    tryExpr->tryblock = pCurrentBlock;
    SCOPESYM * oldScope = pTryScope;
    pTryScope = pCurrentScope;
    pCurrentBlock->scopeSymbol = pCurrentScope;

    if (next) {
        pCurrentBlock->statements = bindUsingDecls(tree, next);
    } else {
        pCurrentBlock->statements = bindStatement(tree->pStmt, NULL);
    }
    pCurrentBlock = pCurrentBlock->owningBlock;
    closeScope();
    pTryScope = oldScope; 

    finallyNestingCount --;

    tryExpr->handlers = newExprBlock(tree);

    if (first->kind == EK_DECL || wrap) {

        EXPR * exitList = NULL;
        pList = &exitList;

        EXPR * object;
        if (wrap) {
            object = wrap;
        } else {
            object = newExpr(NULL, EK_LOCAL, first->asDECL()->sym->asLOCVARSYM()->type);
            object->asLOCAL()->local = first->asDECL()->sym;
            first->asDECL()->sym->slot.isUsed = true;
        }

        EXPRLABEL * pLabel = NULL;

        bool isStruct = true;

        if (!object->type->isAnyStructType()) {
            isStruct = false;
            // we must check for null...
            pLabel = newExprLabel();
            EXPRGOTOIF * gtif = newExpr(NULL, EK_GOTOIF, getVoidType())->asGOTOIF();
            gtif->flags |= EXF_GOTOFORWARD;
            gtif->condition = newExprBinop(NULL, EK_EQ, getPDT(PT_BOOL), object, bindNull(NULL));
            gtif->condition->flags |= EXF_COMPARE;
            gtif->label = pLabel;
            gtif->sense = true;

            newList(gtif, &pList);
        }

        EXPR * iDispObject = mustConvert(object, getPDT(PT_IDISPOSABLE), tree);
        if (!iDispObject) return newError(tree);

        METHSYM * disposeM = NULL;
        if (isStruct) {

            // we have a struct which implements IDisposable, so let's use the method directly
            // instead of casting to an interface...
            disposeM = findStructDisposeMethod(object->type->asAGGSYM());
            if (disposeM) {
                iDispObject = object;
            }
        }
        
        if (!disposeM) {
            disposeM = findMethod(tree, PN_DISPOSE, getPDT(PT_IDISPOSABLE)->asAGGSYM());
        }
        if (!disposeM) return newError(tree);

        EXPRCALL * call = newExpr(tree, EK_CALL, disposeM->retType)->asCALL();
        call->args = NULL;
        call->object = iDispObject;
        call->method = disposeM;
        call->flags |= EXF_BOUNDCALL;

        EXPR * stmt = newExprStmtAs(NULL, call);
        stmt->flags |= EXF_NODEBUGINFO;  //                               don't emit debug info for the unlocking.

        newList(stmt, &pList);
        if (pLabel) {
            newList(pLabel, &pList);
        }
        if (wrap) {
            newList(newExprStmtAs(NULL, newExprWrap(wrap, TK_USING)), &pList);
        }

        tryExpr->handlers->asBLOCK()->statements = exitList;
    }

    return list;
}

METHSYM * FUNCBREC::findStructDisposeMethod(AGGSYM * cls)
{

    if (cls->hasExplicitImpl) return NULL;

    NAME * name = compiler()->namemgr->GetPredefName(PN_DISPOSE);

    SYM * rval = NULL;

    while (true) {
        rval = compiler()->clsDeclRec.findNextAccessibleName(name, cls, pParent, rval, false, true);
        if (rval) {
            if (rval->kind != SK_METHSYM) continue;
            if (rval->asMETHSYM()->cParams != 0) continue;
            return rval->asMETHSYM();
        } else {
            return NULL;
        }
    }
}


EXPR * FUNCBREC::bindUsingDecls(FORSTMTNODE * tree, EXPR *** newLast)
{
    createNewScope();
    pCurrentBlock = newExprBlock(tree);

    EXPR * inits = NULL;
    EXPR ** pInits = &inits;

    bool exprCase;
    if (tree->pInit) {
        exprCase = false;
        DECLSTMTNODE * decls = tree->pInit->asDECLSTMT();
        ASSERT(decls->flags & NF_USING_DECL);
        inits = bindVarDecls(decls, &pInits);
        if (!inits) {
            bindStatement(tree->pStmt, newLast);
            return newError(tree);
        }
        inits = bindUsingDecls(tree, inits);
    } else {
        exprCase = true;
        inits = bindExpr(tree->pExpr);
        if (inits->isOK()) {
            inits = bindUsingDecls(tree, inits);
        }
    }

    EXPRBLOCK * rval = pCurrentBlock;
    rval->statements = inits;

    rval->scopeSymbol = pCurrentScope;
    pCurrentBlock = rval->owningBlock;

    closeScope();
    return rval;

}

EXPR * FUNCBREC::bindFixedDecls(FORSTMTNODE * tree, EXPR *** newLast)
{

    EXPRTRY * tryExpr = newExpr(tree, EK_TRY, NULL)->asTRY();

    EXPRGOTO * oldGotos = gotos;

    tryExpr->flags |= EXF_ISFINALLY;
    tryExpr->finallyStore = (FINALLYSTORE*) allocator->AllocZero(sizeof(FINALLYSTORE));

    finallyNestingCount ++;

    createNewScope();
    pCurrentScope->scopeFlags = SF_TRYSCOPE;
    pCurrentBlock = newExprBlock(tree);
    tryExpr->tryblock = pCurrentBlock;
    SCOPESYM * oldScope = pTryScope;
    SCOPESYM * scopeOfCurrentTry = pTryScope = pCurrentScope;
    pCurrentBlock->scopeSymbol = pCurrentScope;

    EXPR * inits = NULL;
    EXPR ** pInits = &inits;

    DECLSTMTNODE * decls = tree->pInit->asDECLSTMT();
    ASSERT(decls->flags & NF_FIXED_DECL);
    inits = bindVarDecls(decls, &pInits);

    // here, we need to walk the tree and replace any fixed strings with temporaries

    TYPESYM * stringTyp = getPDT(PT_STRING);

    EXPRLOOP(inits, init)
        unsigned count = 0;
        if (init->kind == EK_DECL && init->asDECL()->sym) {
            EXPRDECL * decl = init->asDECL();
            if (decl->init && decl->init->kind == EK_ASSG && decl->init->asBIN()->p2 && decl->init->asBIN()->p2->kind == EK_ADDR) {
                EXPR * stringExpr = decl->init->asBIN()->p2->asBIN()->p1;
                if (stringExpr && stringExpr->type == stringTyp) {
                    // what we have is:
                    // charptr = &stringexpr
                    // we replace this with
                    // wrap = stringexpr, charptr = &wrap
                    EXPRWRAP * wrap = newExprWrap(stringExpr, (TEMP_KIND)(TK_FIXED_STRING_0 + count));
                    wrap->type = compiler()->symmgr.GetPinnedType(stringTyp);
                    wrap->pinned = true;
                    wrap->doNotFree = true;
                    EXPRBINOP * assignment = newExprBinop(NULL, EK_ASSG, stringTyp, wrap, stringExpr);
                    assignment->flags |= EXF_ASSGOP;
                    EXPRBINOP * sequence = newExprBinop(NULL, EK_SEQUENCE, NULL, assignment, decl->init);
                    sequence->asBIN()->p2->asBIN()->p2->asBIN()->p1 = wrap;
                    sequence->asBIN()->p2->asBIN()->p2->flags |= EXF_ASSGOP;
                    decl->init = sequence;
                    decl->sym->slot.isPinned = false;
                }
            }
        }
    ENDLOOP;

    EXPR * stmt = NULL;
    EXPR ** pStmt = &stmt;
    stmt = bindStatement(tree->pStmt, &pStmt);

    newList(stmt, &pInits);
    
    pCurrentBlock->statements = inits;
    pCurrentBlock = pCurrentBlock->owningBlock;

    closeScope();
    pTryScope = oldScope;

    finallyNestingCount--;

    tryExpr->handlers = newExprBlock(tree);

    // now, we need to add the code which nulls out the fixed locals...

    EXPR * nulls = NULL;
    EXPR ** pNulls = &nulls;

    EXPRLOOP(inits, init)
        if (init->kind == EK_DECL && init->asDECL()->sym && init->asDECL()->sym->type) {
            EXPR * target = NULL;
            if (init->asDECL()->sym->slot.isPinned) {
                target = newExpr(NULL, EK_LOCAL, init->asDECL()->sym->type);
                target->asLOCAL()->local = init->asDECL()->sym;
                target->flags |= EXF_LVALUE;
            } else {
                if (init->asDECL()->init && init->asDECL()->init->kind == EK_SEQUENCE) {
                    target = init->asDECL()->init->asBIN()->p1->asBIN()->p1->asWRAP();
                }
            }
            if (target) {
                EXPR * value = tryConvert(bindNull(NULL), target->kind == EK_WRAP ? stringTyp : target->type, NULL);
                if (value) {
                    EXPR * assignment = newExprBinop(NULL, EK_ASSG, target->type, target, value);
                    assignment->flags |= EXF_ASSGOP;

                    newList(newExprStmtAs(NULL, assignment), &pNulls);
                    if (target->kind == EK_WRAP) {
                        target = newExprWrap(target, target->asWRAP()->tempKind);
                        newList(newExprStmtAs(NULL, target), &pNulls);
                    }
                }
            }
        }
    ENDLOOP;
    tryExpr->handlers->asBLOCK()->statements = nulls;
       
    // see if we can eliminate the try-finally
    // this is ok if we meet all the following conditions:
    // we are not enclosed in a try-catch block
    // the body does not contain any gotos to outside

    bool canEliminate = true;

    SCOPESYM * scope = pTryScope;
    while (scope) {
        if ((scope->scopeFlags & (SF_TRYSCOPE | SF_TRYOFFINALLY)) == SF_TRYSCOPE) {
            canEliminate = false;
            break;
        }
        scope = scope->parent->asSCOPESYM();
    }

    if (canEliminate) {
        EXPRGOTO * newGotos = gotos;
        while (newGotos != oldGotos) {
            if ((newGotos->flags & EXF_GOTOCASE) || !(newGotos->flags & EXF_UNREALIZEDGOTO)) {
                if (newGotos->targetScope->nestingOrder < scopeOfCurrentTry->nestingOrder) {
                    canEliminate = false;
                    break;
                }
            } else {
                // this goto is not yet realized and we don't know where its going, so it could be going outside
                canEliminate = false;
                break;
            }
            newGotos = newGotos->prev;
        }
    }

    if (canEliminate) {
        tryExpr->flags |= EXF_REMOVEFINALLY;
    }

    return tryExpr;
}

struct ARRAYFOREACHSLOT {
    EXPRWRAP * limit;
    EXPRWRAP * current;
    EXPRLABEL * body;
    EXPRLABEL * test;

};


EXPR * FUNCBREC::bindForEachArray(FORSTMTNODE * tree, EXPR * array)
{
    EXPR * list = NULL;
    EXPR ** pList = &list;

    ASSERT(array->type->kind == SK_ARRAYSYM);
    ASSERT(array->type->asARRAYSYM()->rank != 0);

    int rank;
    rank = array->type->asARRAYSYM()->rank;

    ARRAYFOREACHSLOT * slots;
    slots = (ARRAYFOREACHSLOT*) _alloca(sizeof(ARRAYFOREACHSLOT) * rank);

    EXPRWRAP * arrayTemp;
    arrayTemp = newExprWrap(array, TK_FOREACH_ARRAY);
    arrayTemp->flags |= EXF_WRAPASTEMP;
    arrayTemp->doNotFree = true;

    EXPR * assignment;
    assignment = newExprBinop(tree, EK_ASSG, arrayTemp->type, arrayTemp, array);
    assignment->asBIN()->flags |= EXF_ASSGOP;
    assignment = newExprStmtAs(tree->pExpr, assignment);
    newList(assignment, &pList);

    TYPESYM * intType;
    intType = getPDT(PT_INT);

    TYPESYM * oneIntArg[1];
    oneIntArg[0] = intType;
    TYPESYM ** oneIntArgList;
    oneIntArgList = compiler()->symmgr.AllocParams(1, oneIntArg);

    // First initialize locals to the maximum sizes for each dimension
    int curRank;
    for (curRank = 0; curRank < rank; curRank++) {
        
        if (rank == 1) {
            // we do not prefetch the array limit as that results in worse code...
            slots[curRank].limit = NULL;
        } else {
            EXPRWRAP * limit = NULL;
            limit = newExpr(EK_WRAP)->asWRAP();
            limit->expr = NULL;
            limit->type = intType;
            limit->slot = NULL;
            limit->doNotFree = true;
            limit->flags |= EXF_WRAPASTEMP;
            limit->tempKind = (TEMP_KIND) (TK_FOREACH_ARRAYLIMIT_0 + curRank);

            slots[curRank].limit = limit;

            METHSYM * getLimit;
            getLimit = findMethod(tree, PN_GETUPPERBOUND, getPDT(PT_ARRAY)->asAGGSYM(), &oneIntArgList);
            ASSERT(getLimit);

            EXPRCALL * callGetLimit;
            callGetLimit = newExpr(tree, EK_CALL, NULL)->asCALL();
            callGetLimit->method = getLimit;
            CONSTVAL cv;
            cv.iVal = curRank;
            callGetLimit->args = newExprConstant(NULL, intType, cv);
            callGetLimit->object = arrayTemp;
            callGetLimit->flags |= EXF_BOUNDCALL;
            callGetLimit->type = intType;

            assignment = newExprBinop(tree, EK_ASSG, intType, limit, callGetLimit);

            assignment->asBIN()->flags |= EXF_ASSGOP;
            assignment = newExprStmtAs(tree->pExpr, assignment);
            newList(assignment, &pList);
        }

    }

    for (curRank = 0; curRank < rank; curRank++) {
        EXPRWRAP * current = newExpr(EK_WRAP)->asWRAP();
        current->expr = NULL;
        current->type = intType;
        current->slot = NULL;
        current->doNotFree = true;
        current->flags |= EXF_WRAPASTEMP;
        current->tempKind = (TEMP_KIND) (TK_FOREACH_ARRAYINDEX_0 + curRank);

        slots[curRank].current = current;

        if (rank > 1) {
            METHSYM * getLimit;
            getLimit = findMethod(tree, PN_GETLOWERBOUND, getPDT(PT_ARRAY)->asAGGSYM(), &oneIntArgList);
            ASSERT(getLimit);

            EXPRCALL * callGetLimit;
            callGetLimit = newExpr(tree, EK_CALL, NULL)->asCALL();
            callGetLimit->method = getLimit;
            CONSTVAL cv;
            cv.iVal = curRank;
            callGetLimit->args = newExprConstant(NULL, intType, cv);
            callGetLimit->object = arrayTemp;
            callGetLimit->flags |= EXF_BOUNDCALL;
            callGetLimit->type = intType;

            assignment = newExprBinop(tree, EK_ASSG, intType, current, callGetLimit);

            assignment->asBIN()->flags |= EXF_ASSGOP | EXF_NODEBUGINFO;
            assignment = newExprStmtAs(tree->pExpr, assignment);
            newList(assignment, &pList);

        } else {

            CONSTVAL cv;
            cv.iVal = 0;

            assignment = newExprBinop(tree, EK_ASSG, intType, current, newExprConstant(tree, intType, cv));
            assignment->asBIN()->flags |= EXF_ASSGOP | EXF_NODEBUGINFO;
            assignment = newExprStmtAs(tree->pInit, assignment);
            newList(assignment, &pList);
        }

        EXPRLABEL * test = newExprLabel()->asLABEL();

        slots[curRank].test = test;
        
        EXPR * gotoTest;
        gotoTest = newExpr(EK_GOTO);
        gotoTest->asGT()->flags |= EXF_GOTOFORWARD | EXF_NODEBUGINFO;
        gotoTest->asGT()->label = test;
        newList(gotoTest, &pList);

        EXPRLABEL * body = newExprLabel()->asLABEL();
        slots[curRank].body = body;
        newList(body, &pList);

    }

    EXPR ** initLast;
    initLast = NULL;
    EXPR * init;
    init = bindVarDecls(tree->pInit->asDECLSTMT(), &initLast);
    if (!init || init->kind != EK_DECL) {
        bindStatement(tree->pStmt, &initLast);
        return newError(tree);
    }

    LOCVARSYM * local = init->asDECL()->sym;

    EXPR * indexList;
    EXPR ** pIndexList;
    indexList = NULL;
    pIndexList = &indexList;

    for (curRank = 0; curRank < rank; curRank ++) {
        newList(slots[curRank].current, &pIndexList);
    }

    EXPR * arrayIndexing;
    arrayIndexing = newExprBinop(tree, EK_ARRINDEX, array->type->asARRAYSYM()->elementType(), arrayTemp, indexList);
    arrayIndexing->flags |= EXF_LVALUE | EXF_ASSGOP;

    assignment = newExprBinop(tree, EK_ASSG, local->type, newExpr(tree, EK_LOCAL, local->type), mustCast(arrayIndexing, local->type, tree));
    assignment->asBIN()->flags |= EXF_ASSGOP | EXF_NODEBUGINFO;
    assignment->asBIN()->p1->asLOCAL()->local = local;
    assignment = newExprStmtAs(tree->pInit, assignment);
    newList(assignment, &pList);

    local->isNonWriteable = true;

    EXPR * body;
    EXPR ** bodyLast;
    bodyLast = NULL;
    body = bindStatement(tree->pStmt, &bodyLast);

    local->isNonWriteable = false;
    
    newList(body, &pList);
    if (bodyLast) {
        pList = bodyLast;
    }

    newList(loopLabels->contLabel, &pList);

    for (curRank = rank - 1; curRank >= 0; curRank--) {
        CONSTVAL cv;
        cv.iVal = 1;

        assignment = newExprBinop(tree, EK_ASSG, intType, slots[curRank].current, newExprBinop(tree, EK_ADD, intType, slots[curRank].current, newExprConstant(tree, intType, cv)));
        assignment->asBIN()->flags |= EXF_ASSGOP;
        assignment = newExprStmtAs(tree, assignment);
        newList(assignment, &pList);

        newList(slots[curRank].test, &pList);

        EXPR * gotoBody;
        gotoBody = newExpr(tree, EK_GOTOIF, NULL);
        gotoBody->asGOTOIF()->sense = true;
        gotoBody->asGT()->label = slots[curRank].body;
        gotoBody->asGOTOIF()->condition = newExprBinop(tree, rank == 1 ? EK_LT : EK_LE, getPDT(PT_BOOL), slots[curRank].current, rank == 1 ? newExprBinop(tree, EK_ARRLEN, intType, arrayTemp, NULL)->asEXPR() : slots[curRank].limit->asEXPR());
        newList(gotoBody, &pList);
        
    }

    newList(loopLabels->breakLabel, &pList);

    // this will free the temporaries
    for (curRank = 0; curRank < rank; curRank++) {
        assignment = newExprWrap(slots[curRank].current, TK_SHORTLIVED);
        newList(newExprStmtAs(NULL, assignment), &pList);
        if (rank != 1) {
            assignment = newExprWrap(slots[curRank].limit, TK_SHORTLIVED);
            newList(newExprStmtAs(NULL, assignment), &pList);
        }
    }

    assignment = newExprWrap(arrayTemp, TK_FOREACH_ARRAY);
    newList(newExprStmtAs(NULL, assignment), &pList);

    return list;    

}

EXPR * FUNCBREC::bindForEach(FORSTMTNODE * tree, EXPR *** newLast)
{

    EXPRTRY * tryExpr = newExpr(tree, EK_TRY, NULL)->asTRY();

    tryExpr->flags |= EXF_ISFINALLY;
    tryExpr->finallyStore = (FINALLYSTORE*) allocator->AllocZero(sizeof(FINALLYSTORE));
    tryExpr->handlers = newExprBlock(tree);

    finallyNestingCount ++;

    createNewScope();
    pCurrentScope->scopeFlags = SF_TRYSCOPE;
    pCurrentBlock = newExprBlock(tree);
    tryExpr->tryblock = pCurrentBlock;
    SCOPESYM * oldScope = pTryScope;
    pTryScope = pCurrentScope;
    pCurrentBlock->scopeSymbol = pCurrentScope;


    EXPR * enumerator = NULL;
    EXPR * init = NULL;
   
    pCurrentBlock->statements = bindForEachInner(tree, &enumerator, &init);


    pCurrentBlock = pCurrentBlock->owningBlock;
    closeScope();
    pTryScope = oldScope; 

    finallyNestingCount --;


    // Now, do we need a finally block?

    EXPR * list = NULL;
    EXPR ** pList = &list;

    
    if (enumerator && enumerator->type->kind == SK_AGGSYM) {

        bool mustCheckAtRuntime = false;
        bool hasInterface = false;
        bool doesNotHaveInterface = false;

        AGGSYM * typDisp = getPDT(PT_IDISPOSABLE)->asAGGSYM();

        bool isStruct = enumerator->type->asAGGSYM()->isStruct;

        if (enumerator->type == getPDT(PT_IENUMERATOR)) {
            mustCheckAtRuntime = true;
        } else {
            if (canConvert(enumerator->type, typDisp, NULL)) {
                hasInterface = true;
            } else {
                doesNotHaveInterface = true;
            }
        }

        ASSERT(mustCheckAtRuntime || hasInterface || doesNotHaveInterface);
        ASSERT(!mustCheckAtRuntime || !(hasInterface || doesNotHaveInterface));
        ASSERT(!hasInterface || !(mustCheckAtRuntime || doesNotHaveInterface));
        ASSERT(!doesNotHaveInterface || !(hasInterface || mustCheckAtRuntime));
        
        EXPR * dispObject = enumerator;
        if (hasInterface) {
            // we only need to cast to the interface, and make the call...

CHECKNULL:
            METHSYM * dispMethod = NULL;
            EXPRLABEL * label = NULL;

            if (isStruct) {

                dispMethod = findStructDisposeMethod(enumerator->type->asAGGSYM());
            } else {
                // we also need to check for null before calling...
                label = newExprLabel();
                EXPRGOTOIF * gtif = newExpr(tree, EK_GOTOIF, getVoidType())->asGOTOIF();
                gtif->flags |= EXF_GOTOFORWARD | EXF_NODEBUGINFO;
                gtif->condition = newExprBinop(tree, EK_EQ, getPDT(PT_BOOL), dispObject, bindNull(NULL));
                gtif->condition->flags |= EXF_COMPARE;
                gtif->label = label;
                gtif->sense = true;

                newList(gtif, &pList);
            }

            if (!dispMethod) {
                dispObject = mustConvert(dispObject, typDisp, tree);
                dispMethod = findMethod(tree, PN_DISPOSE, typDisp);
                if (!dispMethod) return newError(tree);
            }

            EXPRCALL * call = newExpr(tree, EK_CALL, dispMethod->retType)->asCALL();
            call->args = NULL;
            call->object = dispObject;
            call->method = dispMethod;
            call->flags |= EXF_BOUNDCALL;

            EXPR * stmt = newExprStmtAs(tree, call);

            newList(stmt, &pList);

            if (dispObject->kind == EK_WRAP && dispObject->asWRAP()->expr->kind == EK_AS) {
                dispObject = newExprWrap(dispObject, TK_SHORTLIVED);
                newList(newExprStmtAs(tree, dispObject), &pList);
            }

            if (label) {
                newList(label, &pList);
            }
        } else if (mustCheckAtRuntime) {
            
            EXPRTYPEOF * typeofExpr = newExpr(tree, EK_TYPEOF, getPDT(PT_TYPE))->asTYPEOF();
            typeofExpr->sourceType = typDisp;
            typeofExpr->method = NULL;

            EXPR * exprAs = newExprBinop(tree, EK_AS, typDisp, enumerator, typeofExpr);
            
            EXPRWRAP * disp = newExprWrap(exprAs, TK_SHORTLIVED);
            disp->flags |= EXF_WRAPASTEMP;
            disp->doNotFree = true;

            EXPR * assignment = newExprBinop(tree, EK_ASSG, typDisp, disp, exprAs);
            assignment->flags |= EXF_ASSGOP;

            newList(newExprStmtAs(tree, assignment), &pList);

            isStruct = false;

            dispObject = disp;

            goto CHECKNULL;
        

        } else { // no dispose to call, so get rid of the try/finally mechanism
            tryExpr->flags |= EXF_REMOVEFINALLY;
        }
        
        tryExpr->handlers->asBLOCK()->statements = list;


        list = init;
        pList = &list;

        newList(tryExpr, &pList);

        enumerator = newExprWrap(enumerator, TK_SHORTLIVED);
        newList(newExprStmtAs(tree->pInit, enumerator), &pList);

        *newLast = pList;
    } else {
        // this was either an error, or an array foreach...

        if (tryExpr->tryblock->statements) {

            tryExpr->flags |= EXF_REMOVEFINALLY;
            tryExpr->handlers->asBLOCK()->statements = list;
            list = tryExpr;
        } else {
            list = newError(tree);
        }
    }

    return list;

}


EXPR * FUNCBREC::bindForEachInner(FORSTMTNODE * tree, EXPR ** enumeratorExpr, EXPR ** initExpr)
{
    AGGSYM * collType = NULL;
    TYPESYM ** emptyArgList = NULL;
    EXPR * list = NULL;
    EXPR ** pList;
    pList = &list;

    // Note that the caller has prepared a block and scope for us already, so we just fill in the
    // statements into that block

    LOOPLABELS * prev = loopLabels;
    LOOPLABELS ll(this);

    EXPR * collection = bindExpr(tree->pExpr);
    if (!collection->isOK()) goto LERROR;

    if (collection->type->kind == SK_ARRAYSYM) {
        list = bindForEachArray(tree, collection);
        goto LERROR;
    }

    METHSYM * getEnum;
    bool hasInterface;

    hasInterface = canConvert(collection, getPDT(PT_IENUMERABLE), tree);

    if (collection->type->kind != SK_AGGSYM) {
        goto NOFOREACH;
    }
    collType = collection->type->asAGGSYM();
    getEnum = findMethod(tree, PN_GETENUMERATOR, collType, &emptyArgList, false);
    if (!getEnum || getEnum->access != ACC_PUBLIC) {
NOFOREACH:
        if (hasInterface) {
TRYINTERFACE:
            hasInterface = false; // to prevent looping
            collType = getPDT(PT_IENUMERABLE)->asAGGSYM();
            collection = mustConvert(collection, collType, tree);
            getEnum = findMethod(tree, PN_GETENUMERATOR, collType, &emptyArgList, false);
        } else {
            errorSymSymPN(tree, ERR_ForEachMissingMember, collType, collType, PN_GETENUMERATOR);
            goto LERROR;
        }
    }

    EXPRCALL * callGetEnum;
    callGetEnum = newExpr(tree, EK_CALL, NULL)->asCALL();
    callGetEnum->method = verifyMethodCall(tree, getEnum, NULL, &collection, 0)->asMETHSYM();
    if (!callGetEnum->method) goto LERROR;
    callGetEnum->args = NULL;
    callGetEnum->object = collection;
    callGetEnum->flags |= EXF_BOUNDCALL;
    callGetEnum->type = callGetEnum->method->retType;
    if (callGetEnum->type->kind != SK_AGGSYM) {
        if (hasInterface) {
            goto TRYINTERFACE;
        }
        errorSym(tree, ERR_BadGetEnumerator, callGetEnum->type);
        goto LERROR;
    }

    compiler()->symmgr.DeclareType(callGetEnum->type);
    if (callGetEnum->type->isDeprecated) {
        reportDeprecated(tree, callGetEnum->type);
    }

    METHSYM * moveNext;
    moveNext = findMethod(tree, PN_MOVENEXT, callGetEnum->type->asAGGSYM(), &emptyArgList, false);
    if (!moveNext || moveNext->access != ACC_PUBLIC || moveNext->retType != getPDT(PT_BOOL)) {
        if (hasInterface) {
            goto TRYINTERFACE;
        }
        errorSymSymPN(tree, ERR_ForEachMissingMember, collType, callGetEnum->type, PN_MOVENEXT);
        goto LERROR;
    }

    SYM * current;
    current = lookupClassMember(compiler()->namemgr->GetPredefName(PN_CURRENT), callGetEnum->type->asAGGSYM(), false, false, true);
    if (!current || current->kind != SK_PROPSYM || current->asPROPSYM()->methGet == NULL || current->access != ACC_PUBLIC) {
        if (hasInterface) {
            goto TRYINTERFACE;
        } 
        errorSymSymPN(tree, ERR_ForEachMissingMember, collType, callGetEnum->type, PN_CURRENT);
        goto LERROR;
    }

    EXPRWRAP * enumerator;
    enumerator = newExprWrap(callGetEnum, TK_FOREACH_GETENUM);
    enumerator->flags |= EXF_WRAPASTEMP;
    enumerator->doNotFree = true;

    *enumeratorExpr = enumerator;

    EXPR ** initLast;
    initLast = NULL;
    EXPR * init;
    init = bindVarDecls(tree->pInit->asDECLSTMT(), &initLast);
    ASSERT(!initLast);


    EXPRCALL * callMoveNext;
    callMoveNext = newExpr(tree, EK_CALL, NULL)->asCALL();
    callMoveNext->object = enumerator;
    callMoveNext->method = verifyMethodCall(tree, moveNext, NULL, &(callMoveNext->object),0)->asMETHSYM();
    if (!callMoveNext->method) goto LERROR;
    callMoveNext->args = NULL;
    callMoveNext->object = enumerator;
    callMoveNext->flags |= EXF_BOUNDCALL;
    callMoveNext->type = callMoveNext->method->retType;


    EXPR * callCurrent;
    callCurrent = bindToProperty(tree, enumerator, current->asPROPSYM());
    RETBAD(callCurrent);

    if (init->kind != EK_DECL) goto LERROR;
    LOCVARSYM * local;
    local = init->asDECL()->sym;
    local->isNonWriteable = true;

    EXPR ** bodyLast;
    bodyLast = NULL;
    EXPR * body;
    body = bindStatement(tree->pStmt, &bodyLast);

    local->isNonWriteable = false;

    EXPR * assignment;
    assignment = newExprBinop(tree, EK_ASSG, local->type, newExpr(tree, EK_LOCAL, local->type), mustCast(callCurrent, local->type, tree));
    assignment->asBIN()->flags |= EXF_ASSGOP | EXF_NODEBUGINFO;
    assignment->asBIN()->p1->asLOCAL()->local = local;
    assignment = newExprStmtAs(tree->pInit, assignment);

    init = newExprBinop(tree, EK_ASSG, enumerator->type, enumerator, callGetEnum);
    init->asBIN()->flags |= EXF_ASSGOP;
    init = newExprStmtAs(tree->pExpr, init);
    
    *initExpr = init;    

    list = NULL;

    EXPR * gt;
    gt = newExpr(EK_GOTO);
    gt->asGT()->flags |= EXF_GOTOFORWARD;
    gt->asGT()->label = ll.contLabel;

    newList(gt, &pList);
    EXPRLABEL * labelAgain;
    labelAgain = newExprLabel()->asLABEL();
    
    newList(labelAgain, &pList);

    newList(assignment, &pList);
    
    newList(body, &pList);
    if (bodyLast) pList = bodyLast;

    newList(ll.contLabel, &pList);

    gt = newExpr(EK_GOTOIF);
    gt->asGOTOIF()->sense = true;
    gt->asGOTOIF()->label = labelAgain;
    gt->asGOTOIF()->condition = mustConvert(callMoveNext, getPDT(PT_BOOL), tree);
    gt->tree = tree;

    newList(gt, &pList);
    
    newList(ll.breakLabel, &pList);

LERROR:
    loopLabels = prev;

    // always
    codeIsReachable = true;

    return list;

}


// bind a for statement... the result is usually several expr statements
EXPR * FUNCBREC::bindFor(FORSTMTNODE * tree, EXPR *** newLast)
{

    if (tree->flags & NF_FOR_FOREACH) {
        return bindForEach(tree, newLast);
    }

    if (tree->flags & NF_FIXED_DECL) {
        return bindFixedDecls(tree, newLast);
    }

    if (tree->flags & NF_USING_DECL) {
        return bindUsingDecls(tree, newLast);
    }

    // a for statament introduces a new scope, and to get the debug info right, 
    // we need to introduce a new block new in the expression list also.

    createNewScope();
    pCurrentBlock = newExprBlock(tree);

    LOOPLABELS * prev = loopLabels;
    LOOPLABELS ll(this);

    EXPR ** initLast = NULL;
    EXPR * init;
    if (tree->pInit) {
        if (tree->pInit->kind == NK_DECLSTMT) {
            init = bindVarDecls(tree->pInit->asDECLSTMT(), &initLast);
        } else {
            init = newExprStmtAs(tree->pInit, bindExpr(tree->pInit, BIND_RVALUEREQUIRED | BIND_STMTEXPRONLY));
        }
    } else {
        init = NULL;
    }
    EXPR * test;
    if (tree->pExpr) {
        test = bindBooleanValue(tree->pExpr);
    } else {
        CONSTVAL cv;
        cv.iVal = 1;
        test = newExprConstant(tree, getPDT(PT_BOOL), cv);
    }

    if (test->isFalse()) {
        codeIsReachable = false;
    }

    EXPR * incr = bindExpr(tree->pInc, BIND_RVALUEREQUIRED | BIND_STMTEXPRONLY);
    EXPR ** bodyLast = NULL;
    EXPR * body = bindStatement(tree->pStmt, &bodyLast);

    codeIsReachable = !test->isTrue() || ll.breakLabel->reachedByBreak;

    loopLabels = prev;

    // this looks like:

    // [init]
    // goto labelTest
    // label1
    // [body]
    // labelContinue
    // [inc]
    // labelTest
    //   gotoif test, label1
    // labelBreak

    EXPR * list;
    EXPR ** pList = &list;

    list = init;
    if (initLast) pList = initLast;

    EXPR * expr;
    // We will now try to assert the evaluation of the condition
    // at the top of the loop, this will prevent us from re-scanning this
    // block in the post-bind pass when we encounter the backward branch.

    // THis is only legal, if the top of the loop is reachable, and
    // is ONLY reachable through the backward gotoif
    if (!test->isFalse() && (!body || body->kind != EK_LABEL)) {
        expr = newExpr(EK_ASSERT);
        expr->asASSERT()->sense = true;
        expr->asASSERT()->expression = test;
    } else {
        expr = newExpr(EK_GOTO);
    }

    EXPRLABEL * labelTest = newExprLabel();

    expr->flags |= EXF_GOTOFORWARD | EXF_NODEBUGINFO;
    expr->tree = tree;
    expr->asGT()->label = labelTest;

    newList(expr, &pList);

    EXPRLABEL * label1 = newExprLabel();


    newList(label1, &pList);

    newList(body, &pList);
    if (bodyLast) pList = bodyLast;

    newList(ll.contLabel, &pList);

    expr = newExprStmtAs(tree->pInc, incr);
    newList(expr, &pList);

    newList(labelTest, &pList);

    if (test) {
        expr = newExpr(EK_GOTOIF);
        expr->asGOTOIF()->label = label1;
        expr->asGOTOIF()->condition = test;
        expr->asGOTOIF()->sense = true;
    } else {
        expr = newExpr(EK_GOTO);
        expr->asGOTO()->label = label1;
    }
    expr->tree = tree->pExpr;


    newList(expr, &pList);

    newList(ll.breakLabel, &pList);

    EXPRBLOCK * rval = pCurrentBlock;
    pCurrentBlock = rval->owningBlock;
    rval->statements = list;
    rval->scopeSymbol = pCurrentScope;

    closeScope();

    return rval;
}


// bind a while or do loop...
EXPR * FUNCBREC::bindWhileOrDo(LOOPSTMTNODE * tree, EXPR *** newLast, bool asWhile)
{

    LOOPLABELS * prev = loopLabels;
    LOOPLABELS ll(this);  // The constructor sets up the links and creates the labels...

    EXPR * cond = bindBooleanValue(tree->pExpr);

    if (asWhile && cond->kind == EK_CONSTANT && !cond->asCONSTANT()->getVal().iVal) {
        codeIsReachable = false;
    }

    EXPR ** bodyLast = NULL;
    EXPR * body = bindStatement(tree->pStmt, &bodyLast);

    codeIsReachable = !cond->isTrue() || ll.breakLabel->reachedByBreak;
    
    loopLabels = prev;

    // the result looks like:

    // goto labelCont   [optional] if a while loop
    // label2
    // [body]
    // labelCont
    // gotoif cond, label2
    // labelBreak


    EXPR * list = NULL;
    EXPR ** pList = &list;

    if (asWhile) {

        // We will now try to assert the evaluation of the condition
        // at the top of the loop, this will prevent us from re-scanning this
        // block in the post-bind pass when we encounter the backward branch.

        // THis is only legal, if the top of the loop is reachable, and
        // is ONLY reachable through the backward gotoif
        if (!cond->isFalse() && (!body || body->kind != EK_LABEL)) {
            list = newExpr(EK_ASSERT);
            list->asASSERT()->sense = true;
            list->asASSERT()->expression = cond;
        } else {
            list = newExpr(EK_GOTO);
        }
        list->tree = tree;
        list->flags |= EXF_GOTOFORWARD | EXF_NODEBUGINFO;
        list->asGT()->label = ll.contLabel;
    }

    EXPRLABEL * label2 = newExprLabel();

    newList(label2, &pList);

    newList(body, &pList);
    if (bodyLast) pList = bodyLast;

    newList(ll.contLabel, &pList);

    EXPRGOTOIF * expr = newExpr( tree, EK_GOTOIF, NULL)->asGOTOIF();
    expr->condition = cond;
    expr->label = label2;
    expr->sense = true;

    newList(expr, &pList);

    newList(ll.breakLabel, &pList);

    *newLast = pList;

    return list;

}


EXPR * FUNCBREC::bindBooleanValue(BASENODE * tree, EXPR * op)
{
    EXPR * rval = op ? op : bindExpr(tree);

    RETBAD(rval);

    // Give a warning for something like "if (x = false)"
    if (rval->kind == EK_ASSG) {
        EXPR * rhs = rval->asBIN()->p2;
        if (rhs->kind == EK_CONSTANT && rhs->type == getPDT(PT_BOOL) && (rhs->flags & EXF_LITERALCONST))
            errorN(tree, WRN_IncorrectBooleanAssg);
    }

    EXPR * newRval = tryConvert(rval, getPDT(PT_BOOL), tree);

    if (newRval) return newRval;

    newRval = bindUDUnop(tree, EK_TRUE, rval);

    if (!newRval) {
        newRval = rval;
    }

    // this will either give an error if no opTRUE exists, or ensure that its return type is bool
    return mustConvert(newRval, getPDT(PT_BOOL), tree);

}


// Bind an if statement.  (morphs it into gotoifs and gotos...)
EXPR * FUNCBREC::bindIf(IFSTMTNODE * tree, EXPR *** newLast)
{
    EXPR * cond = bindBooleanValue(tree->pExpr);
    EXPR ** ifLast = NULL;

    if(cond->isFalse()) codeIsReachable = false;

    EXPR * ifArm = bindStatement(tree->pStmt, &ifLast);

    bool reachableIf = codeIsReachable;

    if(cond->isTrue()) {
        codeIsReachable = false;
    } else {
        codeIsReachable = true;
    }

    EXPR ** elseLast = NULL;
    EXPR * elseArm;
    if (tree->pElse) {
        elseArm = bindStatement(tree->pElse, &elseLast);
    } else {
        elseArm = NULL;
         codeIsReachable= true;
    }

    if (cond->isTrue()) {
        codeIsReachable = reachableIf;
    } else if (!cond->isFalse()) {
        codeIsReachable |= reachableIf;
    }

    // ok, now we construct the following:

    // gotoif !cond, label1
    // [ifArm]
    // goto label2  [optional]
    // label1
    // [elseArm]    [optional]
    // label2       [optional]

    EXPR * list = NULL;
    EXPR ** pList = &list;

    EXPRLABEL * label1 = newExprLabel();

    EXPRLABEL * label2 = elseArm ? newExprLabel() : NULL;

    list = newExpr(tree, EK_GOTOIF, getVoidType());
    list->flags |= EXF_GOTOFORWARD;
    list->asGOTOIF()->condition = cond;
    list->asGOTOIF()->label = label1;
    list->asGOTOIF()->sense = false;

    newList(ifArm, &pList);
    if (ifLast) pList = ifLast;

    if (elseArm) {
        EXPR * expr = newExpr(EK_GOTO);
        expr->flags |= EXF_GOTOFORWARD | EXF_OPTIMIZABLEGOTO;
        expr->asGOTO()->label = label2;
        newList(expr, &pList);
    }

    newList(label1, &pList);

    if (elseArm) {
        newList(elseArm, &pList);
        if (elseLast) pList = elseLast;

        newList(label2, &pList);
    }

    *newLast = pList;

    return list;
}


EXPR * FUNCBREC::bindUDUnop(BASENODE * tree, EXPRKIND ek, EXPR * op)
{

    NAME * name = ekName(ek);
    ASSERT(name);

    if (op->type->kind != SK_AGGSYM) return NULL;

    SYM * sym = lookupClassMember(name, op->type->asAGGSYM(), false, false, false);
    if (!sym || sym->kind != SK_METHSYM || !sym->asMETHSYM()->isOperator) return NULL;

    if (checkBogus(sym)) {
        compiler()->clsDeclRec.errorNameRefSymbol(sym->name, tree, pParent, sym, ERR_BindToBogus);
        return newError(tree);
    }

    EXPRCALL * call = newExpr(tree, EK_CALL, sym->asMETHSYM()->retType)->asCALL();
    call->args = op;
    call->object = NULL;
    call->method = sym->asMETHSYM();
    call->flags |= EXF_BOUNDCALL;

    verifyMethodArgs(call);

    return call;
}



EXPR * FUNCBREC::bindUDArithOp(BASENODE * tree, EXPRKIND ek, EXPR * op1, EXPR * op2)
{
    // decimal has user-defined operations, but we want to fold constant expressions
    // of this type. So check for that first.
    EXPR * rval = bindDecimalConstExpr(tree, ek, op1, op2);
    if (rval)
        return rval;

    // Make sure both types are declared, so we have their User-Defined operators.
    compiler()->symmgr.DeclareType(op1->type);
    if (op2) compiler()->symmgr.DeclareType(op2->type);

    switch (ek) {
    case EK_UPLUS:
    case EK_NEG:
    case EK_BITNOT:
        return bindUDUnop(tree, ek, op1);
    case EK_ADD:
    case EK_SUB:
    case EK_MUL:
    case EK_DIV:
    case EK_MOD:
    case EK_BITXOR:
    case EK_BITOR:
    case EK_BITAND:
        return bindUDBinop(tree, ek, op1, op2);
    case EK_LSHIFT:
    case EK_RSHIFT:
        ASSERT(!"shifts should go directly to bindUDBinop");
        return NULL;
    case EK_LE:
    case EK_LT:
    case EK_GT:
    case EK_GE:
    case EK_EQ:
    case EK_NE:
        rval = bindUDBinop(tree, ek, op1, op2);
        return rval;

    default:

        return NULL;
    }
}


/*
 * Get the name of an operator for error reporting purposes.
 */
LPCWSTR FUNCBREC::opName(OPERATOR op)
{
    return CParser::GetTokenInfo((TOKENID) CParser::GetOperatorInfo(op)->iToken)->pszText;
}

/*
 * Report a bad operator types error to the user.
 */
EXPR * FUNCBREC::badOperatorTypesError(BASENODE * tree, EXPR * op1, EXPR * op2)
{
    // Bad arg types - report error to user.
    if (op2)
        errorStrSymSym(tree, ERR_BadBinaryOps,
                     opName(tree->Op()), op1->type, op2->type);
    else
        errorStrSym(tree, ERR_BadUnaryOp,
                     opName(tree->Op()), op1->type);
    return newError(tree);
}

/*
 * Report an ambiguous operator types error.
 */
EXPR * FUNCBREC::ambiguousOperatorError(BASENODE * tree, EXPR * op1, EXPR * op2)
{
    // Bad arg types - report error to user.
    if (op2)
        errorStrSymSym(tree, ERR_AmbigBinaryOps,
                     opName(tree->Op()), op1->type, op2->type);
    else
        errorStrSym(tree, ERR_AmbigUnaryOp,
                     opName(tree->Op()), op1->type);
    return newError(tree);
}

/* Check for a integral comparison operation that can't be right. If one operand
 * is a constant, the other operand has been cast from a smaller integration type, and the
 * constant isn't in the range of the smaller type, the comparison is vacuous, and will
 * always be true or false.
 */
void FUNCBREC::checkVacuousIntegralCompare(BASENODE * tree, EXPR * castOp, EXPR * constOp)
{
    ASSERT(castOp->kind == EK_CAST);
    ASSERT(constOp->kind == EK_CONSTANT);

    do {
        // Ensure this is a cast from one integral/enum type to another.
        if (castOp->flags & (EXF_BOX | EXF_UNBOX))
            break;

        if (castOp->type->kind != SK_AGGSYM) break;
        if (castOp->asCAST()->p1->type->kind != SK_AGGSYM) break;

        AGGSYM * tpDest = castOp->type->asAGGSYM();
        if (tpDest->isEnum)
            tpDest = tpDest->underlyingType;
        AGGSYM * tpSrc = castOp->asCAST()->p1->type->asAGGSYM();
        if (tpSrc->isEnum)
            tpSrc = tpSrc->underlyingType;

        if (! tpSrc->isSimpleType() || ! tpDest->isSimpleType())
            break;

        // If the cast is a narrowing cast, then it could change the 
        // value of the expression, so this test isn't valid                                  
        unsigned convertKind = simpleTypeConversions[tpSrc->iPredef][tpDest->iPredef];
        if ((convertKind & CONV_KIND_MASK) == CONV_KIND_EXP)
            break;

        // The cast meets all the requirements, so check if the constant is in 
        // range of the original integral type of the casted expression.
        if (! isConstantInRange(constOp->asCONSTANT(), tpSrc)) {
            errorSym(tree, WRN_VacuousIntegralComp, tpSrc);
            break;
        }

        castOp = castOp->asCAST()->p1;
    } while (castOp->kind == EK_CAST);
}


/*
 * Bind an enum operator: +, -, ^, |, &, <, >, <=, >=, ==, !=. If both operands are constant, the result
 * will be a constant node also. op2 can be null for a unary op.
 *        
 *
 * Returns NULL if no valid enum op is available for the types of the operators. Does not report an error.
 */
EXPR * FUNCBREC::bindEnumOp(BASENODE * tree, EXPRKIND kind, unsigned flags, EXPR * op1, EXPR * op2, bool isBoolResult)
{
    PTYPESYM typeOp1 = op1->type;
    PTYPESYM typeOp2 = op2 ? op2->type : NULL;
    AGGSYM * typeEnum;
    AGGSYM * typeDest;

    // Check and convert types accordingly. Sets these variables:
    //   typeEnum - the enum type in questions
    //   typeDest - the destination type
    //   op1 - the first operator, converted to correct type.
    //   op2 - the second operator, converted to correct type.
    //
    // The operators allowed are (E = enum, U = underlying, B = boolean):
    //     E + U -> E       U + E -> E      E - E -> U   E - U -> E
    //     E | E -> E       E ^ E -> E      E & E -> E   ~E -> E
    //     E == E -> B      E != E -> B     E <= E -> B  E >= E -> B
    //     E < E -> B       E > E -> B

    // One of the two arguments must be of enum type. The other must be convertable to
    // either the enum type or the underlying type, depending on the operator.
    if (typeOp1->kind == SK_AGGSYM && typeOp1->asAGGSYM()->isEnum) {
        // First argument is of enum type.
        typeDest = typeEnum = typeOp1->asAGGSYM();
        if (op2) {
            if (isBoolResult) {
                EXPR * op2Enum = tryConvert(op2, typeEnum, tree);
                if (!op2Enum)
                    goto BADTYPES;
                else
                    op2 = op2Enum;
                typeDest = getPDT(PT_BOOL)->asAGGSYM();
            }
            else if (kind == EK_BITOR || kind == EK_BITAND || kind == EK_BITXOR) {
                EXPR * op2Enum = tryConvert(op2, typeEnum, tree);
                if (!op2Enum)
                    goto BADTYPES;
                else
                    op2 = op2Enum;
            }
            else if (kind == EK_ADD) {
                EXPR * op2Underlying = tryConvert(op2, typeEnum->underlyingType, tree);
                if (!op2Underlying)
                    goto BADTYPES;
                else
                    op2 = op2Underlying;
            }
            else if (kind == EK_SUB) {
                EXPR * op2Enum = tryConvert(op2, typeEnum, tree);
                EXPR * op2Underlying = tryConvert(op2, typeEnum->underlyingType, tree);

                if (op2Enum && op2Underlying)
                    return ambiguousOperatorError(tree, op1, op2);      // ambiguous: could be op-op or op-int.
                else if (op2Enum) {
                    op2 = op2Enum;
                    typeDest = typeEnum->underlyingType;  // enum-enum produces underlying.
                }
                else if (op2Underlying)
                    op2 = op2Underlying;
                else
                    goto BADTYPES;      // bad op2 type.

            }
            else
                goto BADTYPES;  // bad operator.
        }
        else if (kind != EK_BITNOT)
            goto BADTYPES;
    }
    else {
        // 2nd op must be of enum type.
        ASSERT(typeOp2->kind == SK_AGGSYM && typeOp2->asAGGSYM()->isEnum);
        typeDest = typeEnum = typeOp2->asAGGSYM();

        if (isBoolResult) {
            EXPR * op1Enum = tryConvert(op1, typeEnum, tree);
            if (!op1Enum)
                goto BADTYPES;
            else
                op1 = op1Enum;
            typeDest = getPDT(PT_BOOL)->asAGGSYM();
        }
        else if (kind == EK_BITOR || kind == EK_BITAND || kind == EK_BITXOR) {
            EXPR * op1Enum = tryConvert(op1, typeEnum, tree);
            if (!op1Enum)
                goto BADTYPES;
            else
                op1 = op1Enum;
        }
        else if (kind == EK_ADD) {
            EXPR * op1Underlying = tryConvert(op1, typeEnum->underlyingType, tree);
            if (!op1Underlying)
                goto BADTYPES;
            else
                op1 = op1Underlying;
        }
        else if (kind == EK_SUB) {
            EXPR * op1Enum = tryConvert(op1, typeEnum, tree);
            if (!op1Enum)
                goto BADTYPES;
            else {
                op1 = op1Enum;
                typeDest = typeEnum->underlyingType;  // enum-enum produces underlying.
            }
        }
        else
            goto BADTYPES;  // bad operator.
    }

    if (typeEnum->fundType() > FT_LASTNONLONG)
        return bindI8Op(tree, kind, flags, op1, op2, false, typeDest);
    else
        return bindI4Op(tree, kind, flags, op1, op2, typeDest);

BADTYPES:
    return NULL;
}


/*
 * Bind a delegate operator: +, -. + is equivalent to Delegate.Combine and - is equivalent to Delegate.Remove.
 * One operand must be a delegate type and the other operand convertable to that delegate type.
 *
 * Returns NULL if no valid delegate op is available for the types of the operators. Does not report an error.
 */
EXPR * FUNCBREC::bindDelegateOp(BASENODE * tree, EXPRKIND kind, unsigned flags, EXPR * op1, EXPR * op2)
{
    ASSERT(kind == EK_ADD || kind == EK_SUB);

    TYPESYM * delegateType = NULL;   // Type of the delegate.

    if (op1->type->kind == SK_AGGSYM && op1->type->asAGGSYM()->isDelegate) {
        delegateType = op1->type;
        op2 = tryConvert(op2, delegateType, tree, 0);
        if (! op2)
            return NULL;   // Couldn't converts op2 to the type of op1.
    }
    else if (op2->type->kind == SK_AGGSYM && op2->type->asAGGSYM()->isDelegate) {
        delegateType = op2->type;
        op1 = tryConvert(op1, delegateType, tree, 0);
        if (! op1)
            return NULL;   // Couldn't converts op1 to the type of op2.
    }

    // Both operands are now of the same delegate type.
    ASSERT(delegateType == op1->type && delegateType == op2->type && delegateType->asAGGSYM()->isDelegate);

    // Construct argument list from the two delegates.
    EXPR * args = newExprBinop(NULL, EK_LIST, getVoidType(), op1, op2);

    // Find and bind the Delegate.Combine or Delegate.Remove call.
    EXPR * obj = NULL;
    NAME * memb = compiler()->namemgr->GetPredefName((kind == EK_ADD) ? PN_COMBINE : PN_REMOVE);
    METHSYM * method = compiler()->symmgr.LookupGlobalSym(
        memb, getPDT(PT_DELEGATE), MASK_METHSYM)->asMETHSYM();
    if (!method) {
        errorSymName(tree, ERR_MissingPredefinedMember, getPDT(PT_DELEGATE), memb);
        return newError( tree);
    }
    SYM * verified = verifyMethodCall(tree, method, args, &obj, 0);

    // Create the call node for the call.
    EXPRCALL * call = newExpr(tree, EK_CALL, getPDT(PT_DELEGATE))->asCALL();
    call->object = obj;
    call->flags |= EXF_BOUNDCALL;
    call->method = verified->asMETHSYM();
    call->args = args;

    // Cast the result to the delegate type.
    return mustCast(call, delegateType, tree);
}

/*
 * Bind an bool operator: ==, !=, &&, ||, !, &, |, ^. If both operands are constant, the result
 * will be a constant node also. op2 can be null for a unary operator (!).
 */
EXPR * FUNCBREC::bindBoolOp(BASENODE * tree, EXPRKIND kind, unsigned flags, EXPR * op1, EXPR * op2)
{
    EXPR * expr;

    // Get the result type and operand type.
    PTYPESYM typeBool = getPDT(PT_BOOL);

    if (kind == EK_LOGNOT && op1->type != typeBool) {
        EXPR * userOp = bindUDUnop(tree, kind, op1);
        if (userOp) {
            return userOp;
        }
    } else if ((kind == EK_LOGAND || kind == EK_LOGOR) && op1->type != typeBool && op2->type != typeBool) {
        EXPR * userOp = bindUDBinop(tree, (EXPRKIND)(kind - EK_LOGAND + EK_BITAND), op1, op2);
        if (userOp) {
            return bindUserBoolOp(tree, kind, userOp);
        }
    }


    // Can we convert both operands to the operand type? If, do the conversion; if not,
    // report an error.
    EXPR * oldOp1 = op1;
    EXPR * oldOp2 = op2;
    op1 = tryConvert(op1, typeBool, tree);
    if (! op1 || (op2 && ! (op2 = tryConvert(op2, typeBool, tree)))) {
        // Bad arg types - report error to user.
        if (!op1) op1 = oldOp1;
        if (!op2) op2 = oldOp2;
        return badOperatorTypesError(tree, op1, op2);
    }

    // Check for constants and fold them.

    if (op1->kind == EK_CONSTANT && (!op2 || op2->kind == EK_CONSTANT)) {
        // Get the operands
        bool d1 = op1->asCONSTANT()->getVal().iVal ? true : false;
        bool d2 = op2 ? (op2->asCONSTANT()->getVal().iVal ? true : false) : false;
        bool result; 

        // Do the operation.
        switch (kind) {
        case EK_EQ:      result = (d1 == d2); break;
        case EK_NE:      result = (d1 != d2); break;
        case EK_LOGAND:  result = (d1 && d2); break;
        case EK_LOGOR:   result = (d1 || d2); break;
        case EK_LOGNOT:  result = (!d1); break;

        case EK_BITXOR:  result = (d1 != d2); break;
        case EK_BITAND:  result = (d1 && d2); break;
        case EK_BITOR:   result = (d1 || d2); break;
        default:         
            return badOperatorTypesError(tree, op1, op2);
        }

        CONSTVAL cv;
        cv.iVal = result;

        // Allocate the result node.
        expr = newExprConstant(tree, typeBool, cv);
    }
    else {
        switch (kind) {
        case EK_EQ: case EK_NE:
        case EK_LOGAND: case EK_LOGOR: case EK_LOGNOT:
        case EK_BITXOR: case EK_BITAND: case EK_BITOR:
            break;
        default:         
            return badOperatorTypesError(tree, op1, op2);
        }
        // Allocate the result expression.
        expr = newExprBinop(tree, kind, typeBool, op1, op2);
        expr->flags |= flags;
    }

    return expr;
}


/*
 * Convert and constant fold an expression involved I4 operands. Constants
 * are folded if needed. The operands are assumed to be already converted to the
 * correct types. See Also bindU4Op, bindI8Op, bindU8Op
 */
EXPR * FUNCBREC::bindI4Op(BASENODE * tree, EXPRKIND kind, unsigned flags, EXPR * op1, EXPR * op2, TYPESYM * typeDest)
{
    EXPR * expr;

    // Check for constants.
    bool op1IsConst = (op1->kind == EK_CONSTANT);
    bool op2IsConst = op2 ? (op2->kind == EK_CONSTANT) : true;

    // Zero as second operand is special for many operators.
    if (op2 && op2->isZero())
    {
        switch (kind) {
        case EK_ADD: case EK_SUB: case EK_BITOR: case EK_BITXOR:
        case EK_LSHIFT: case EK_RSHIFT: 
            return mustConvert(op1, typeDest, tree);   // Adding, subtracting, oring, xoring, shifting by zero is a no-op.

        case EK_DIV: case EK_MOD:
            // Integer division by zero -- an error.
            errorN(tree, ERR_IntDivByZero);
            return newError(tree);

        case EK_MUL: case EK_BITAND:
            ASSERT( op1);
            if (!op1->hasSideEffects( compiler()))
                // Multiplying or Anding with zero is zero (if there are no side-effects)
                return mustConvert(op2, typeDest, tree);
            break;
        default:
            break;
        }
    }

    // Zero as first operand is special for some operators.
    if (op1->isZero())
    {
        switch (kind) {
        case EK_ADD: case EK_BITOR: case EK_BITXOR:
            return mustConvert(op2, typeDest, tree);   // Adding, oring, xoring zero is a no-op.

        case EK_MUL: case EK_BITAND: case EK_LSHIFT:
        case EK_RSHIFT: case EK_DIV: case EK_MOD:
            ASSERT( op2);
            if (!op2->hasSideEffects( compiler()))
                // Multiplying, Dividing, Shifting, Anding all result in 0
                return mustConvert(op1, typeDest, tree);
            break;

        case EK_SUB:
            ASSERT( op2 != NULL);
            kind = EK_NEG;
            op1 = op2;
            op2 = NULL;
            op1IsConst = op2IsConst;
            op2IsConst = true;
            break;
        default:
            break;
        }
    }


    // Fold operation if both args are constant.
    if (op1IsConst && op2IsConst) {
        // Get the operands
        int d1 = op1->asCONSTANT()->getVal().iVal;
        int d2 = op2 ? op2->asCONSTANT()->getVal().iVal : 0;
        int result;     

        // Do the operation.
        switch (kind) {
        case EK_ADD:     
            result = d1 + d2; 
            if (checked.constant) {
                FAILCHECK((d1 ^ d2) < 0 || (d1 ^ result) >= 0, tree);
            }
            break;
        case EK_SUB:
            result = d1 - d2; 
            if (checked.constant) {
                FAILCHECK((d1 ^ d2) >= 0 || (d1 ^ result) >= 0, tree);
            }
            break;
        case EK_MUL:     
            result = d1 * d2; 
            if (checked.constant) {
                FAILCHECK(((unsigned int) d2 != 0x80000000 || d1 != -1) && result / d1 == d2, tree);
            }
            break;
        case EK_DIV:   
            // if we don't check this in either mode, then we will get an exception...
            if (d2 == -1 && (unsigned int) d1 == 0x80000000) {
                FAILCHECK(!checked.constant, tree);
                result = d1 * d2;
            } else {
                result = d1 / d2; 
            }
            break;
        case EK_MOD:
            // if we don't check this, then 0x80000000 % -1 will cause an exception...
            if (d2 == -1) {
                result = 0;
            } else {
                result = d1 % d2; 
            }
            break;
        case EK_NEG:     
            result = -d1; 
            if (checked.constant) {
                FAILCHECK( (unsigned int) d1 != 0x80000000, tree);
            }
            break;
        case EK_UPLUS:   result = d1; break;
        case EK_BITAND:  result = d1 & d2; break;
        case EK_BITOR:   result = d1 | d2; break;
        case EK_BITXOR:  result = d1 ^ d2; break;
        case EK_BITNOT:  result = ~d1; break;
        case EK_LSHIFT:  result = d1 << d2; break;
        case EK_RSHIFT:  result = d1 >> d2; break;
        case EK_EQ:      result = (d1 == d2); break;
        case EK_NE:      result = (d1 != d2); break;
        case EK_LE:      result = (d1 <= d2); break;
        case EK_LT:      result = (d1 < d2);  break;
        case EK_GE:      result = (d1 >= d2); break;
        case EK_GT:      result = (d1 > d2);  break;
        default:         ASSERT(0); result = 0;  // unexpected operation.
        }

        // Allocate the result node.
        CONSTVAL cv;
        cv.iVal = result;
        expr = newExprConstant(tree, typeDest, cv);
    }
    else {
        if (kind == EK_NEG && (flags & EXF_CHECKOVERFLOW)) {
            CONSTVAL cv;
            cv.iVal = 0;
            op2 = op1;
            op1 = newExprConstant(tree, typeDest, cv);
            kind = EK_SUB;
        }
        else if (kind == EK_BITOR) {
            // Want to warn about code like:
            //        int hi = 1;
            //        int lo = -1;
            //        long value = (((long)hi) << 32) | lo;   // bad

            FUNDTYPE ft;
            bool isSignExtend = false;
            if (op1->kind == EK_CAST) {
                ft = op1->asCAST()->p1->type->fundType();
                if (ft == FT_I2 || ft == FT_I1)
                    isSignExtend = true;
            }
            if (op2->kind == EK_CAST) {
                ft = op2->asCAST()->p1->type->fundType();
                if (ft == FT_I2 || ft == FT_I1)
                    isSignExtend = true;
            }
            if (isSignExtend) {
                errorN(tree, WRN_BitwiseOrSignExtend);
            }
        }

        // Allocate the result expression.
        expr = newExprBinop(tree, kind, typeDest, op1, op2);
        expr->flags |= flags;
    }

    // Give warning if comparing to a constant that isn't in range.  E.g., int i; i < 0x80000000000.
    if (op2 && (flags & EXF_COMPARE)) {
        if (op1IsConst && op2->kind == EK_CAST) {
            checkVacuousIntegralCompare(tree, op2, op1);    
        }
        else if (op2IsConst && op1->kind == EK_CAST) {
            checkVacuousIntegralCompare(tree, op1, op2);    
        }
    }

    return expr;
}


/*
 * Convert and constant fold an expression involved U4 operands. Constants
 * are folded if needed. The operands are assumed to be already converted to the
 * correct types. See Also bindI4Op, bindI8Op, bindU8Op
 */
EXPR * FUNCBREC::bindU4Op(BASENODE * tree, EXPRKIND kind, unsigned flags, EXPR * op1, EXPR * op2, TYPESYM * typeDest)
{
    EXPR * expr;

    // Check for constants.
    bool op1IsConst = (op1->kind == EK_CONSTANT);
    bool op2IsConst = op2 ? (op2->kind == EK_CONSTANT) : true;

    // Zero as second operand is special for many operators.
    if (op2 && op2->isZero())
    {
        switch (kind) {
        case EK_ADD: case EK_SUB: case EK_BITOR: case EK_BITXOR:
        case EK_LSHIFT: case EK_RSHIFT: 
            return mustConvert(op1, typeDest, tree);   // Adding, subtracting, oring, xoring, shifting by zero is a no-op.

        case EK_DIV: case EK_MOD:
            // Integer division by zero -- an error.
            errorN(tree, ERR_IntDivByZero);
            return newError(tree);

        case EK_MUL: case EK_BITAND:
            ASSERT( op1);
            if (!op1->hasSideEffects( compiler()))
                // Multiplying or Anding with zero is zero (if there are no side-effects)
                return mustConvert(op2, typeDest, tree);
            break;
        default:
            break;
        }
    }

    // Zero as first operand is special for some operators.
    if (op1->isZero())
    {
        switch (kind) {
        case EK_ADD: case EK_BITOR: case EK_BITXOR:
            return mustConvert(op2, typeDest, tree);   // Adding, oring, xoring zero is a no-op.

        case EK_MUL: case EK_BITAND: case EK_LSHIFT:
        case EK_RSHIFT: case EK_DIV: case EK_MOD:
            ASSERT( op2);
            if (!op2->hasSideEffects( compiler()))
                // Multiplying, Dividing, Shifting, Anding all result in 0
                return mustConvert(op1, typeDest, tree);
            break;
        default:
            break;
        }
    }


    // Fold operation if both args are constant.
    if (op1IsConst && op2IsConst) {
        // Get the operands
        unsigned int d1 = op1->asCONSTANT()->getVal().uiVal;
        unsigned int d2 = op2 ? op2->asCONSTANT()->getVal().uiVal : 0;
        unsigned int result;     

        // Do the operation.
        switch (kind) {
        case EK_ADD:
            result = d1 + d2; 
            if (checked.constant) {
                FAILCHECK(result > d1 && result > d2, tree);
            }
            break;
        case EK_SUB:     
            result = d1 - d2; 
            if (checked.constant) {
                FAILCHECK(d2 <= d1, tree);
            }
            break;
        case EK_MUL:
            result = d1 * d2; 
            if (checked.constant) {
                FAILCHECK(result / d1 == d2, tree);
            }
            break;
        case EK_DIV:     
            result = d1 / d2; 
            break;
        case EK_MOD:     
            result = d1 % d2; 
            break;
        case EK_NEG:
            // Special case: a unary minus promotes a uint to a long
            {
                CONSTVAL cv;
                cv.longVal = (__int64 *) allocator->Alloc(sizeof(__int64));
                * cv.longVal = ((unsigned __int64)d1) * -1;
                return expr = newExprConstant(tree, getPDT(PT_LONG), cv);
            }
        case EK_UPLUS:   result = d1; break;
        case EK_BITAND:  result = d1 & d2; break;
        case EK_BITOR:   result = d1 | d2; break;
        case EK_BITXOR:  result = d1 ^ d2; break;
        case EK_BITNOT:  result = ~d1; break;
        case EK_LSHIFT:  result = d1 << d2; break;
        case EK_RSHIFT:  result = d1 >> d2; break;
        case EK_EQ:      result = (d1 == d2); break;
        case EK_NE:      result = (d1 != d2); break;
        case EK_LE:      result = (d1 <= d2); break;
        case EK_LT:      result = (d1 < d2);  break;
        case EK_GE:      result = (d1 >= d2); break;
        case EK_GT:      result = (d1 > d2);  break;
        default:         ASSERT(0); result = 0;  // unexpected operation.
        }

        // Allocate the result node.
        CONSTVAL cv;
        cv.uiVal = result;
        expr = newExprConstant(tree, typeDest, cv);
    }
    else {
        if (kind == EK_NEG && op1->type->fundType() == FT_U4) {
            op1 = mustConvert(op1, getPDT(PT_LONG), tree);
            typeDest = getPDT(PT_LONG);
        }
        else if (kind == EK_BITOR) {
            // Want to warn about code like:
            //        int hi = 1;
            //        int lo = -1;
            //        long value = (((long)hi) << 32) | lo;   // bad

            FUNDTYPE ft;
            bool isSignExtend = false;
            if (op1->kind == EK_CAST) {
                ft = op1->asCAST()->p1->type->fundType();
                if (ft == FT_I2 || ft == FT_I1)
                    isSignExtend = true;
            }
            if (op2->kind == EK_CAST) {
                ft = op2->asCAST()->p1->type->fundType();
                if (ft == FT_I2 || ft == FT_I1)
                    isSignExtend = true;
            }
            if (isSignExtend) {
                errorN(tree, WRN_BitwiseOrSignExtend);
            }
        }
        
        // Allocate the result expression.
        expr = newExprBinop(tree, kind, typeDest, op1, op2);
        expr->flags |= flags;
    }

    // Give warning if comparing to a constant that isn't in range.  E.g., int i; i < 0x80000000000.
    if (op2 && (flags & EXF_COMPARE)) {
        if (op1IsConst && op2->kind == EK_CAST) {
            checkVacuousIntegralCompare(tree, op2, op1);    
        }
        else if (op2IsConst && op1->kind == EK_CAST) {
            checkVacuousIntegralCompare(tree, op1, op2);    
        }
    }

    return expr;
}



/*
 * Convert and constant fold an expression involving I8 operands. Constants
 * are folded if needed. The operands are assumed to be already converted to the
 * correct types. Helper for bindLongOp and bindEnumOp.
 */
EXPR * FUNCBREC::bindI8Op(BASENODE * tree, EXPRKIND kind, unsigned flags, EXPR * op1, EXPR * op2, bool op2IsInt, TYPESYM * typeDest)
{
    EXPR * expr;

    // Check for constants.
    bool op1IsConst = (op1->kind == EK_CONSTANT);
    bool op2IsConst = op2 ? (op2->kind == EK_CONSTANT) : true;

    // Zero as second operand is special for many operators.
    if (op2 && op2->isZero())
    {
        switch (kind) {
        case EK_ADD: case EK_SUB: case EK_BITOR: case EK_BITXOR:
        case EK_LSHIFT: case EK_RSHIFT: 
            return mustConvert(op1, typeDest, tree);   // Adding, subtracting, oring, xoring, shifting by zero is a no-op.

        case EK_DIV: case EK_MOD:
            // Integer division by zero -- an error.
            errorN(tree, ERR_IntDivByZero);
            return newError(tree);

        case EK_MUL: case EK_BITAND:
            ASSERT( op1);
            if (!op1->hasSideEffects( compiler()))
                // Multiplying or Anding with zero is zero (if there are no side-effects)
                return mustConvert(op2, typeDest, tree);
            break;
        default:
            break;
        }
    }

    // Zero as first operand is special for some operators.
    if (op1->isZero())
    {
        switch (kind) {
        case EK_ADD: case EK_BITOR: case EK_BITXOR:
            return mustConvert(op2, typeDest, tree);   // Adding, oring, xoring zero is a no-op.

        case EK_MUL: case EK_BITAND: case EK_LSHIFT:
        case EK_RSHIFT: case EK_DIV: case EK_MOD:
            ASSERT( op2);
            if (!op2->hasSideEffects( compiler()))
                // Multiplying, Dividing, Shifting, Anding all result in 0
                return mustConvert(op1, typeDest, tree);
            break;

        case EK_SUB:
            kind = EK_NEG;
            op1 = op2;
            op2 = NULL;
            op1IsConst = op2IsConst;
            op2IsConst = true;
            break;
        default:
            break;
        }
    }


    // Check for constants and fold them.
    if (op1IsConst && op2IsConst) {
        // Get the operands
        __int64 d1 = * op1->asCONSTANT()->getVal().longVal;
        __int64 d2 = 0;           // If op2IsInt is true
        int d2_int = 0;           // If op2IsInt is false
        __int64 result = 0;       // If isBoolResult is false
        bool result_b = false;    // If isBoolResult is true

        if (op2) {
            if (op2IsInt)
                d2_int = op2->asCONSTANT()->getVal().iVal;
            else
                d2 = * op2->asCONSTANT()->getVal().longVal;
        }

        // Do the operation.
        switch (kind) {
        case EK_ADD:     
            result = d1 + d2; 
            if (checked.constant) {
                FAILCHECK((d1 ^ d2) < 0 || (d1 ^ result) >= 0, tree);
            }
            break;
        case EK_SUB:     
            result = d1 - d2; 
            if (checked.constant) {
                FAILCHECK((d1 ^ d2) >= 0 || (d1 ^ result) >= 0, tree);
            }
            break;
        case EK_MUL:     
            result = d1 * d2; 
            if (checked.constant) {
                FAILCHECK((d1 != -1 || (unsigned __int64) d2 != UI64(0x8000000000000000)) && result / d1 == d2, tree);
            }
            break;
        case EK_DIV:     
            result = d1 / d2; 
            if (checked.constant) {
                FAILCHECK((unsigned __int64) d1 != UI64(0x8000000000000000) || d2 != -1, tree);
            }
            break;
        case EK_MOD:     
            result = d1 % d2; 
            break;
        case EK_NEG:     
            result = -d1; 
            if (checked.constant) {
                FAILCHECK((unsigned __int64) d1 != UI64(0x8000000000000000), tree);
            }
            break;
        case EK_UPLUS:   result = d1; break;
        case EK_BITAND:  result = d1 & d2; break;
        case EK_BITOR:   result = d1 | d2; break;
        case EK_BITXOR:  result = d1 ^ d2; break;
        case EK_BITNOT:  result = ~d1; break;
        case EK_LSHIFT:  result = d1 << d2_int; break;
        case EK_RSHIFT:  result = d1 >> d2_int; break;
        case EK_EQ:      result_b = (d1 == d2); break;
        case EK_NE:      result_b = (d1 != d2); break;
        case EK_LE:      result_b = (d1 <= d2); break;
        case EK_LT:      result_b = (d1 < d2);  break;
        case EK_GE:      result_b = (d1 >= d2); break;
        case EK_GT:      result_b = (d1 > d2);  break;
        default:         ASSERT(0); result = 0;  // unexpected operation.
        }

        // Allocate the result node.
        CONSTVAL cv;
        if (typeDest->fundType() == FT_I1) {
            cv.iVal = result_b;
        }
        else {
            cv.longVal = (__int64 *) allocator->Alloc(sizeof(__int64));
            * cv.longVal = result;
        }
        expr = newExprConstant(tree, typeDest, cv);
    }
    else {
        if (kind == EK_NEG && (flags & EXF_CHECKOVERFLOW)) {
            op2 = op1;
            CONSTVAL cv;
            cv.longVal = (__int64 *) allocator->Alloc(sizeof(__int64));
            * cv.longVal = 0;
            op1 = newExprConstant(tree, typeDest, cv);
            kind = EK_SUB;
        }
        else if (kind == EK_BITOR) {
            // Want to warn about code like:
            //        int hi = 1;
            //        int lo = -1;
            //        long value = (((long)hi) << 32) | lo;   // bad

            FUNDTYPE ft;
            bool isSignExtend = false;
            if (op1->kind == EK_CAST) {
                ft = op1->asCAST()->p1->type->fundType();
                if (ft == FT_I4 || ft == FT_I2 || ft == FT_I1)
                    isSignExtend = true;
            }
            if (op2->kind == EK_CAST) {
                ft = op2->asCAST()->p1->type->fundType();
                if (ft == FT_I4 || ft == FT_I2 || ft == FT_I1)
                    isSignExtend = true;
            }
            if (isSignExtend) {
                errorN(tree, WRN_BitwiseOrSignExtend);
            }
        }

        // Allocate the result expression.
        expr = newExprBinop(tree, kind, typeDest, op1, op2);
        expr->flags |= flags;
    }

    // Give warning if comparing to a constant that isn't in range.  E.g., int i; i < 0x80000000000.
    if (op2 && (flags & EXF_COMPARE)) {
        if (op1IsConst && op2->kind == EK_CAST) {
            checkVacuousIntegralCompare(tree, op2, op1);    
        }
        else if (op2IsConst && op1->kind == EK_CAST) {
            checkVacuousIntegralCompare(tree, op1, op2);    
        }
    }

    return expr;
}


/*
 * Convert and constant fold an expression involving U8 operands. Constants
 * are folded if needed. The operands are assumed to be already converted to the
 * correct types. See also bindU4Op, bindI8Op
 */
EXPR * FUNCBREC::bindU8Op(BASENODE * tree, EXPRKIND kind, unsigned flags, EXPR * op1, EXPR * op2, bool op2IsInt, TYPESYM * typeDest)
{
    EXPR * expr;

    // Check for constants.
    bool op1IsConst = (op1->kind == EK_CONSTANT);
    bool op2IsConst = op2 ? (op2->kind == EK_CONSTANT) : true;

    // Zero as second operand is special for many operators.
    if (op2 && op2->isZero())
    {
        switch (kind) {
        case EK_ADD: case EK_SUB: case EK_BITOR: case EK_BITXOR:
        case EK_LSHIFT: case EK_RSHIFT: 
            return mustConvert(op1, typeDest, tree);   // Adding, subtracting, oring, xoring, shifting by zero is a no-op.

        case EK_DIV: case EK_MOD:
            // Integer division by zero -- an error.
            errorN(tree, ERR_IntDivByZero);
            return newError(tree);

        case EK_MUL: case EK_BITAND:
            ASSERT( op1);
            if (!op1->hasSideEffects( compiler()))
                // Multiplying or Anding with zero is zero (if there are no side-effects)
                return mustConvert(op2, typeDest, tree);
            break;
        default:
            break;
        }
    }

    // Zero as first operand is special for some operators.
    if (op1->isZero())
    {
        switch (kind) {
        case EK_ADD: case EK_BITOR: case EK_BITXOR:
            return mustConvert(op2, typeDest, tree);   // Adding, oring, xoring zero is a no-op.

        case EK_MUL: case EK_BITAND: case EK_LSHIFT:
        case EK_RSHIFT: case EK_DIV: case EK_MOD:
            ASSERT( op2);
            if (!op2->hasSideEffects( compiler()))
                // Multiplying, Dividing, Shifting, Anding all result in 0
                return mustConvert(op1, typeDest, tree);
            break;
        default:
            break;
        }
    }


    // Check for constants and fold them.
    if (op1IsConst && op2IsConst) {
        // Get the operands
        unsigned __int64 d1 = * op1->asCONSTANT()->getVal().ulongVal;
        unsigned __int64 d2 = 0;       // If op2IsInt is true
        unsigned int d2_int = 0;       // If op2IsInt is false
        unsigned __int64 result = 0;   // If isBoolResult is false
        bool result_b = false;         // If isBoolResult is true

        if (op2) {
            if (op2IsInt)
                d2_int = op2->asCONSTANT()->getVal().uiVal;
            else
                d2 = * op2->asCONSTANT()->getVal().ulongVal;
        }

        // Do the operation.
        switch (kind) {
        case EK_ADD:     
            result = d1 + d2; 
            if (checked.constant) {
                FAILCHECK(result > d1 && result > d2, tree);
            }
            break;
        case EK_SUB:     
            result = d1 - d2; 
            if (checked.constant) {
                FAILCHECK(d2 <= d1, tree);
            }
            break;
        case EK_MUL:     
            result = d1 * d2; 
            if (checked.constant) {
                FAILCHECK(result / d1 == d2, tree);
            }
            break;
        case EK_DIV:     
            result = d1 / d2; 
            break;
        case EK_MOD:     
            result = d1 % d2; 
            break;
        case EK_NEG:     
            //You can't do this!
            return badOperatorTypesError( tree, op1, op2);
        case EK_UPLUS:   result = d1; break;
        case EK_BITAND:  result = d1 & d2; break;
        case EK_BITOR:   result = d1 | d2; break;
        case EK_BITXOR:  result = d1 ^ d2; break;
        case EK_BITNOT:  result = ~d1; break;
        case EK_LSHIFT:  result = d1 << d2_int; break;
        case EK_RSHIFT:  result = d1 >> d2_int; break;
        case EK_EQ:      result_b = (d1 == d2); break;
        case EK_NE:      result_b = (d1 != d2); break;
        case EK_LE:      result_b = (d1 <= d2); break;
        case EK_LT:      result_b = (d1 < d2);  break;
        case EK_GE:      result_b = (d1 >= d2); break;
        case EK_GT:      result_b = (d1 > d2);  break;
        default:         ASSERT(0); result = 0;  // unexpected operation.
        }

        // Allocate the result node.
        CONSTVAL cv;
        if (typeDest->fundType() == FT_I1) {
            cv.iVal = result_b;
        }
        else {
            cv.ulongVal = (unsigned __int64 *) allocator->Alloc(sizeof(unsigned __int64));
            * cv.ulongVal = result;
        }
        expr = newExprConstant(tree, typeDest, cv);
    }
    else {
        if (kind == EK_NEG)
            return badOperatorTypesError( tree, op1, op2);
        else if (kind == EK_BITOR) {
            // Want to warn about code like:
            //        int hi = 1;
            //        int lo = -1;
            //        long value = (((long)hi) << 32) | lo;   // bad

            FUNDTYPE ft;
            bool isSignExtend = false;
            if (op1->kind == EK_CAST) {
                ft = op1->asCAST()->p1->type->fundType();
                if (ft == FT_I4 || ft == FT_I2 || ft == FT_I1)
                    isSignExtend = true;
            }
            if (op2->kind == EK_CAST) {
                ft = op2->asCAST()->p1->type->fundType();
                if (ft == FT_I4 || ft == FT_I2 || ft == FT_I1)
                    isSignExtend = true;
            }
            if (isSignExtend) {
                errorN(tree, WRN_BitwiseOrSignExtend);
            }
        }

        // Allocate the result expression.
        expr = newExprBinop(tree, kind, typeDest, op1, op2);
        expr->flags |= flags;
    }

    // Give warning if comparing to a constant that isn't in range.  E.g., int i; i < 0x80000000000.
    if (op2 && (flags & EXF_COMPARE)) {
        if (op1IsConst && op2->kind == EK_CAST) {
            checkVacuousIntegralCompare(tree, op2, op1);    
        }
        else if (op2IsConst && op1->kind == EK_CAST) {
            checkVacuousIntegralCompare(tree, op1, op2);    
        }
    }

    return expr;
}


/*
 * Bind an float/double operator: +, -, *, /, %, <, >, <=, >=, ==, !=. If both operations are constants, the result
 * will be a constant also. op2 can be null for a unary operator. The operands are assumed
 * to be already converted to the correct type.
 */
EXPR * FUNCBREC::bindFloatOp(BASENODE * tree, EXPRKIND kind, unsigned flags, EXPR * op1, EXPR * op2, TYPESYM * typeDest)
{
    EXPR * expr;

    // Check for constants and fold them.
    if (op1->kind == EK_CONSTANT && (!op2 || op2->kind == EK_CONSTANT)) {
        // Get the operands
        double d1 = * op1->asCONSTANT()->getVal().doubleVal;
        double d2 = op2 ? * op2->asCONSTANT()->getVal().doubleVal : 0.0;
        double result = 0;      // if isBoolResult is false
        bool result_b = false;  // if isBoolResult is true

        // Do the operation.
        switch (kind) {
        case EK_ADD:  result = d1 + d2; break;
        case EK_SUB:  result = d1 - d2; break;
        case EK_MUL:  result = d1 * d2; break;
        case EK_DIV:  result = d1 / d2; break;
        case EK_NEG:  result = -d1; break;
        case EK_UPLUS: result = d1; break;
        case EK_MOD:  result = fmod(d1, d2); break;
        case EK_EQ:   result_b = (d1 == d2); break;
        case EK_NE:   result_b = (d1 != d2); break;
        case EK_LE:   result_b = (d1 <= d2); break;
        case EK_LT:   result_b = (d1 < d2);  break;
        case EK_GE:   result_b = (d1 >= d2); break;
        case EK_GT:   result_b = (d1 > d2);  break;
        default:      ASSERT(0); result = 0.0;  // unexpected operation.
        }

        CONSTVAL cv;
        // Allocate the result node.
        if (typeDest->fundType() == FT_I1) {
            cv.iVal = result_b;
        }
        else {
            cv.doubleVal = (double *) allocator->Alloc(sizeof(double));

            // NaN has some implementation defined bits that differ between platforms. 
            // Normalize it to produce identical images accross all platforms
            if (_isnan(result)) {
                * (unsigned __int64 *) cv.doubleVal = UI64(0xFFF8000000000000);
                _ASSERTE(_isnan(* cv.doubleVal));
            }
            else {
                * cv.doubleVal = result;
            }
        }
        expr = newExprConstant(tree, typeDest, cv);
    }
    else {
        // Allocate the result expression.
        expr = newExprBinop(tree, kind, typeDest, op1, op2);
        flags &= ~EXF_CHECKOVERFLOW;
        expr->flags |= flags;
    }

    return expr;
}

bool FUNCBREC::hasSelfUDCompare(EXPR * op, EXPRKIND kind)
{
    if (op->type->kind != SK_AGGSYM) return false;

    AGGSYM * sym = op->type->asAGGSYM();
    if (kind == EK_EQ) {
        if (!sym->hasUDselfEQRes) {
            sym->hasUDselfEQRes = true;
            sym->hasUDselfEQ = (bindUDBinop(NULL, EK_EQ, op, op) != NULL);
        }
        return sym->hasUDselfEQ;
    } else {
        ASSERT(kind == EK_NE);
        if (!sym->hasUDselfNERes) {
            sym->hasUDselfNERes = true;
            sym->hasUDselfNE = (bindUDBinop(NULL, EK_NE, op, op) != NULL);
        }
        return sym->hasUDselfNE;
    }
}

/*
 * Bind == or != on operands of reference type.
 */
EXPR * FUNCBREC::bindRefEquality(BASENODE * tree, EXPRKIND kind, EXPR * op1, EXPR * op2)
{
    EXPR * expr;

    if (op1->type->kind == SK_PTRSYM || op2->type->kind == SK_PTRSYM) {
        return bindArithOp(tree, kind, EXF_COMPARE, op1, op2, false, true);
    }

    // Both args must be of reference type, and one of the expressions must
    // be castable to the type of the other.
    if (op1->type->fundType() != FT_REF || op2->type->fundType() != FT_REF ||
        (! canCast(op1, op2->type, tree, CONVERT_NOUDC) && ! canCast(op2, op1->type, tree, CONVERT_NOUDC)))
    {
        // Bad arg types - report error to user.
        return badOperatorTypesError(tree, op1, op2);
    }

    if (op1->kind == EK_CONSTANT && op2->kind == EK_CONSTANT) {
        // Both references are constants. Compare values at compile time. Only null vs. non-null can possible matter here.
        bool result;
        STRCONST * s1 = op1->asCONSTANT()->getSVal().strVal;
        STRCONST * s2 = op2->asCONSTANT()->getSVal().strVal;
        result = (s1 == NULL && s2 == NULL);
        if (kind == EK_NE)
            result = !result;

        CONSTVAL cv;
        cv.iVal = (int)result;
        expr = newExprConstant(tree, getPDT(PT_BOOL), cv);
        return expr;
    }



    // now we check if maybe you are making a mistake... :)
    if (op1->type != op2->type) {
        if ((op2->kind != EK_CONSTANT || op2->asCONSTANT()->getSVal().strVal) && hasSelfUDCompare(op1, kind)) {
            errorSym(tree, WRN_BadRefCompareRight, op1->type);
        } else if ((op1->kind != EK_CONSTANT || op1->asCONSTANT()->getSVal().strVal) && hasSelfUDCompare(op2, kind)) {
            errorSym(tree, WRN_BadRefCompareLeft, op2->type);
        }
    }


    expr = newExprBinop(tree, kind, getPDT(PT_BOOL), op1, op2);
    expr->flags |= EXF_COMPARE;
    return expr;
}

/*
 * Bind == or != on operands of string type. Both operands must already have been converted
 * to strings.
 *
 * Binds to a call to String.Equals(op1, op2).
 */
EXPR * FUNCBREC::bindConstStringEquality(BASENODE * tree, EXPRKIND kind, EXPR * op1, EXPR * op2)
{
    EXPR * rval;

    ASSERT(op1->type->isPredefType(PT_STRING) && op2->type->isPredefType(PT_STRING));
    ASSERT(kind == EK_NE || kind == EK_EQ);

    ASSERT(op1->kind == EK_CONSTANT && op2->kind == EK_CONSTANT);

    // Both strings are constants. Compare values at compile time.
    bool result;
    STRCONST * s1 = op1->asCONSTANT()->getSVal().strVal;
    STRCONST * s2 = op2->asCONSTANT()->getSVal().strVal;
    if (s1 == NULL || s2 == NULL) {
        result = (s1 == s2);
    }
    else {
        if (s1->length == s2->length && memcmp(s1->text, s2->text, s1->length * sizeof(WCHAR)) == 0)
            result = true;
        else 
            result = false;
    }
    
    if (kind == EK_NE)
        result = !result;

    CONSTVAL cv;
    cv.iVal = (int)result;
    rval = newExprConstant(tree, getPDT(PT_BOOL), cv);
    return rval;
}

EXPR * FUNCBREC::convertShiftOp(EXPR * op2, bool isLong)
{
    bool isConstant = op2->kind == EK_CONSTANT;

    CONSTVAL cv;
    cv.iVal = isLong ? 0x3f : 0x1f;
    if (isConstant) {
        cv.iVal = op2->asCONSTANT()->getVal().iVal & cv.iVal;
        return newExprConstant(op2->tree, op2->type, cv);
    }

    
    return newExprBinop(op2->tree, EK_BITAND, op2->type, op2, newExprConstant(NULL, getPDT(PT_INT), cv));

}

/*
 * Bind an shift operator: <<, >>. These can have integer or long first operands,
 * and second operand must be int.
 */
EXPR * FUNCBREC::bindShiftOp(BASENODE * tree, EXPRKIND kind, unsigned flags, EXPR * op1, EXPR * op2)
{
    EXPR * op1Conv;
    EXPR * op2Conv;

    EXPR * rval = bindUDBinop(tree, kind, op1, op2);
    if (rval) return rval;

    if (! (op2Conv = tryConvert(op2, getPDT(PT_INT), tree)))
        return badOperatorTypesError(tree, op1, op2);

    if ((op1Conv = tryConvert(op1, getPDT(PT_INT), tree))) {
        op2Conv = convertShiftOp(op2, false);
        return bindI4Op(tree, kind, flags, op1Conv, op2Conv, getPDT(PT_INT));
    }
    else if ((op1Conv = tryConvert(op1, getPDT(PT_UINT), tree))) {
        op2Conv = convertShiftOp(op2, false);
        return bindU4Op(tree, kind, flags, op1Conv, op2Conv, getPDT(PT_UINT));
    }
    else if ((op1Conv = tryConvert(op1, getPDT(PT_LONG), tree))) {
        op2Conv = convertShiftOp(op2, true);
        return bindI8Op(tree, kind, flags, op1Conv, op2Conv, true, getPDT(PT_LONG));
    }
    else if ((op1Conv = tryConvert(op1, getPDT(PT_ULONG), tree))) {
        op2Conv = convertShiftOp(op2, true);
        return bindU8Op(tree, kind, flags, op1Conv, op2Conv, true, getPDT(PT_ULONG));
    }
    else {
        // No valid types.
        return badOperatorTypesError(tree, op1, op2);
    }
}


/*
 * Bind a unary/binary operator: +, -, *, /, %, <, >, <=, >=, ==, !=, ^, |, &, ~. These can be either
 * integer, long, or floating point operands (unless fpOK is false).
 * The isBoolResult identifies the comparison
 * operators that have boolean result.
 * op2 can be null for a unary operator (+, -, ~)
 */
EXPR * FUNCBREC::bindArithOp(BASENODE * tree, EXPRKIND kind, unsigned flags, EXPR * op1, EXPR * op2, bool fpOK, bool isBoolResult)
{
    TYPESYM * type1 = op1->type;
    TYPESYM * type2 = op2 ? op2->type : NULL;
    bool couldBeStringConcat = false;
    bool couldBeDecimalOp = false;
    EXPR * op1Conv;
    EXPR * op2Conv = NULL;
    EXPR * rval = NULL;

    if (type1->fundType() > FT_R8 || type1->fundType() == FT_NONE ||
		(type2 != NULL && (type2->fundType() > FT_R8 || type2->fundType() == FT_NONE)))
		rval = bindUDArithOp(tree, kind, op1, op2);
    if (rval) return rval;

    // Could we have string concatination? If so, save that info to see if we have an
    // ambiguity later.
    if (kind == EK_ADD) {
        if ((canConvert(op1, getPDT(PT_STRING), tree) ||   // OR is right here, could be str+str, str+obj, obj+str
             canConvert(op2, getPDT(PT_STRING), tree)) &&
            canConvert(op1, getPDT(PT_OBJECT), tree) &&
            canConvert(op2, getPDT(PT_OBJECT), tree))
        {
            couldBeStringConcat = true;
        }
    }

    // We need a built-in operator. The rule is:
    //   If either op is an enum, use an enum operator. If either op is a delegate, use
    //   a delegate operator. Else do the equivalent of
    //   overload resolution among the built-in types. The overload resolution is 
    //   hard coded for efficiency.

    // Enum type?
    if ((type1->kind == SK_AGGSYM && type1->asAGGSYM()->isEnum) ||
        (type2 && type2->kind == SK_AGGSYM && type2->asAGGSYM()->isEnum))
    {
        rval = bindEnumOp(tree, kind, flags, op1, op2, isBoolResult);
        if (rval) {
            if (couldBeStringConcat) 
                return ambiguousOperatorError(tree, op1, op2);    // Ambiguity!
            else
                return rval;
        }
    }

    // Delegate type (+ and - only)?
    if ((kind == EK_ADD || kind == EK_SUB) && 
        ((type1->kind == SK_AGGSYM && type1->asAGGSYM()->isDelegate) ||
         (type2 && type2->kind == SK_AGGSYM && type2->asAGGSYM()->isDelegate)))
    {
        rval = bindDelegateOp(tree, kind, flags, op1, op2);
        if (rval) {
            if (couldBeStringConcat) 
                return ambiguousOperatorError(tree, op1, op2);    // Ambiguity!
            else
                return rval;
        }
    }


    // Exact match on type1?
    if (type1->kind == SK_AGGSYM && type1->asAGGSYM()->isPredefined) {
        if (op2)
            op2Conv = tryConvert(op2, type1, tree);
        if (op2Conv || op2 == NULL) {
            switch (type1->asAGGSYM()->iPredef) {
            case PT_BOOL:
                return bindBoolOp(tree, kind, flags, op1, op2Conv); 
            case PT_UINT:
                return bindU4Op(tree, kind, flags, op1, op2Conv, getPDT(isBoolResult ? PT_BOOL : PT_UINT)); 
            case PT_INT:    
                return bindI4Op(tree, kind, flags, op1, op2Conv, getPDT(isBoolResult ? PT_BOOL : PT_INT)); 
            case PT_ULONG:
                return bindU8Op(tree, kind, flags, op1, op2Conv, false, getPDT(isBoolResult ? PT_BOOL : PT_ULONG)); 
            case PT_LONG:   
                return bindI8Op(tree, kind, flags, op1, op2Conv, false, getPDT(isBoolResult ? PT_BOOL : PT_LONG)); 
            case PT_FLOAT:  
                if (fpOK)
                    return bindFloatOp(tree, kind, flags, op1, op2Conv, getPDT(isBoolResult ? PT_BOOL : PT_FLOAT)); 
                break;
            case PT_DOUBLE: 
                if (fpOK)
                    return bindFloatOp(tree, kind, flags, op1, op2Conv, getPDT(isBoolResult ? PT_BOOL : PT_DOUBLE)); 
                break;
            case PT_STRING: 
                if (kind == EK_ADD)
                    return bindStringConcat(tree, op1, op2Conv);
                break;
            }
        }
    }

    // Exact match on type2?
    if (type2 && type2->kind == SK_AGGSYM && type2->asAGGSYM()->isPredefined) {
        op1Conv = tryConvert(op1, type2, tree);
        if (op1Conv) {
            switch (type2->asAGGSYM()->iPredef) {
            case PT_BOOL:
                return bindBoolOp(tree, kind, flags, op1Conv, op2); 
            case PT_UINT: 
                return bindU4Op(tree, kind, flags, op1Conv, op2, getPDT(isBoolResult ? PT_BOOL : PT_UINT)); 
            case PT_INT: 
                return bindI4Op(tree, kind, flags, op1Conv, op2, getPDT(isBoolResult ? PT_BOOL : PT_INT)); 
            case PT_ULONG: 
                return bindU8Op(tree, kind, flags, op1Conv, op2, false, getPDT(isBoolResult ? PT_BOOL : PT_ULONG)); 
            case PT_LONG: 
                return bindI8Op(tree, kind, flags, op1Conv, op2, false, getPDT(isBoolResult ? PT_BOOL : PT_LONG)); 
            case PT_FLOAT: 
                if (fpOK)
                    return bindFloatOp(tree, kind, flags, op1Conv, op2, getPDT(isBoolResult ? PT_BOOL : PT_FLOAT)); 
                break;
            case PT_DOUBLE: 
                if (fpOK)
                    return bindFloatOp(tree, kind, flags, op1Conv, op2, getPDT(isBoolResult ? PT_BOOL : PT_DOUBLE)); 
                break;
            case PT_STRING: 
                if (kind == EK_ADD)
                    return bindStringConcat(tree, op1Conv, op2);
                break;
            }
        }
    }

    TYPESYM * typeDecimal = compiler()->symmgr.GetPredefType(PT_DECIMAL, false);

    if (fpOK && // We need to know this because there is no ordering between floats and Decimals
        typeDecimal &&
        canConvert(op1, typeDecimal, tree, flags) &&   // are they both convertible to Decimal?
        (op2 == NULL || canConvert(op2, typeDecimal, tree, flags)))
    {
        couldBeDecimalOp = true;
    }

    // Try in order from most specific to least specific: int, uint, long, ulong, float, double.
    if ((op1Conv = tryConvert(op1, getPDT(PT_INT), tree)) && 
        (op2 == NULL || (op2Conv = tryConvert(op2, getPDT(PT_INT), tree))))
    {
        if (couldBeStringConcat) return ambiguousOperatorError(tree, op1, op2);
        return bindI4Op(tree, kind, flags, op1Conv, op2Conv, getPDT(isBoolResult ? PT_BOOL : PT_INT));
    }
    else if ((op1Conv = tryConvert(op1, getPDT(PT_UINT), tree)) && 
             (op2 == NULL || (op2Conv = tryConvert(op2, getPDT(PT_UINT), tree))))
    {
        if (couldBeStringConcat) return ambiguousOperatorError(tree, op1, op2);
        return bindU4Op(tree, kind, flags, op1Conv, op2Conv, getPDT(isBoolResult ? PT_BOOL : PT_UINT));
    }
    else if ((op1Conv = tryConvert(op1, getPDT(PT_LONG), tree)) && 
             (op2 == NULL || (op2Conv = tryConvert(op2, getPDT(PT_LONG), tree))))
    {
        if (couldBeStringConcat) return ambiguousOperatorError(tree, op1, op2);
        return bindI8Op(tree, kind, flags, op1Conv, op2Conv, false, getPDT(isBoolResult ? PT_BOOL : PT_LONG));
    }
    else if ((op1Conv = tryConvert(op1, getPDT(PT_ULONG), tree)) && 
             (op2 == NULL || (op2Conv = tryConvert(op2, getPDT(PT_ULONG), tree))))
    {
        if (couldBeStringConcat) return ambiguousOperatorError(tree, op1, op2);
        return bindI8Op(tree, kind, flags, op1Conv, op2Conv, false, getPDT(isBoolResult ? PT_BOOL : PT_ULONG));
    }
    else if (fpOK &&
             (op1Conv = tryConvert(op1, getPDT(PT_FLOAT), tree)) && 
             (op2 == NULL || (op2Conv = tryConvert(op2, getPDT(PT_FLOAT), tree))))
    {
        if (couldBeStringConcat || couldBeDecimalOp) return ambiguousOperatorError(tree, op1, op2);
        return bindFloatOp(tree, kind, flags, op1Conv, op2Conv, getPDT(isBoolResult ? PT_BOOL : PT_FLOAT));
    }
    else if (fpOK &&
             (op1Conv = tryConvert(op1, getPDT(PT_DOUBLE), tree)) && 
             (op2 == NULL || (op2Conv = tryConvert(op2, getPDT(PT_DOUBLE), tree))))
    {
        if (couldBeStringConcat  || couldBeDecimalOp) return ambiguousOperatorError(tree, op1, op2);
        return bindFloatOp(tree, kind, flags, op1Conv, op2Conv, getPDT(isBoolResult ? PT_BOOL : PT_DOUBLE));
    }
    else if (fpOK && couldBeDecimalOp)
    {
        if (couldBeStringConcat) return ambiguousOperatorError(tree, op1, op2);
        op1Conv = tryConvert(op1, getPDT(PT_DECIMAL), tree, flags);
        if (op2)
            op2Conv = tryConvert(op2, getPDT(PT_DECIMAL), tree, flags);
        else 
            op2Conv = NULL;
        return bindUDArithOp(tree, kind, op1Conv, op2Conv);
    }

    // If string concatination was possible, and no other operator was, then string concat it is.
    if (couldBeStringConcat)
    {
        // Could be str+str, str+obj, obj+str.
        // If we can convert to string, that's more specific than object, so use that.
        if ((op1Conv = tryConvert(op1, getPDT(PT_STRING), tree)))
            op1 = op1Conv;
        else 
            op1 = mustConvert(op1, getPDT(PT_OBJECT), tree); // must work due to test above.

        if ((op2Conv = tryConvert(op2, getPDT(PT_STRING), tree)))
            op2 = op2Conv;
        else 
            op2 = mustConvert(op2, getPDT(PT_OBJECT), tree); // must work due to test above.

        return bindStringConcat(tree, op1, op2);
    }

    if (type2) {
        if (type1->kind == SK_PTRSYM && type2->kind == SK_PTRSYM ) {
            if (isBoolResult || (kind == EK_SUB && type1 == type2)) {
                return bindPtrOp(tree, kind, flags, op1, op2);
            } else {
                return badOperatorTypesError(tree, op1, op2);
            }
        }
            
        
        if (type1->kind == SK_PTRSYM || type2->kind == SK_PTRSYM) {

            if (kind == EK_EQ || kind == EK_NE) {
                if (type1 == compiler()->symmgr.GetNullType() || type2 == compiler()->symmgr.GetNullType()) {
                    return bindPtrOp(tree, kind, flags, op1, op2);
                }
            }

            if (!(op2Conv = tryConvert(op2, getPDT(PT_INT), tree)) && !(op1Conv = tryConvert(op1, getPDT(PT_INT), tree)) &&
                !(op2Conv = tryConvert(op2, getPDT(PT_UINT), tree)) && !(op1Conv = tryConvert(op1, getPDT(PT_UINT), tree)) &&
                !(op2Conv = tryConvert(op2, getPDT(PT_LONG), tree)) && !(op1Conv = tryConvert(op1, getPDT(PT_LONG), tree)) &&
                !(op2Conv = tryConvert(op2, getPDT(PT_ULONG), tree)) && !(op1Conv = tryConvert(op1, getPDT(PT_ULONG), tree))) {

                return badOperatorTypesError(tree, op1, op2);
            }
            // ok, one of them converted to some integral type...
            if (kind == EK_SUB) {
                if (type2->kind != SK_PTRSYM) {
                    return bindPtrOp(tree, kind, flags, op1, op2);
                }
            } else if (kind == EK_ADD) {
                if (type1->kind == SK_PTRSYM) {
                    return bindPtrOp(tree, kind, flags, op1, op2Conv);
                } else {
                    return bindPtrOp(tree, kind, flags, op1Conv, op2);
                }
            }
        }
    }

    // No valid built-in operator was applicable.
    return badOperatorTypesError(tree, op1, op2);
}


EXPR * FUNCBREC::bindUserBoolOp(BASENODE * tree, EXPRKIND kind, EXPR * call)
{
    if (call->kind != EK_CALL) return call;

    TYPESYM * retType = call->type;

    ASSERT(call->asCALL()->method->cParams == 2);
    if (call->asCALL()->method->params[0] != retType || call->asCALL()->method->params[1] != retType) {
        errorSym(tree, ERR_BadBoolOp, call->asCALL()->method);
        return newError(tree);
    }

    EXPR * op1 = call->asCALL()->args->asBIN()->p1;
    EXPR * op2 = call->asCALL()->args->asBIN()->p2;

    op1 = mustConvert(op1, retType, tree);
    if (!op1) return newError(tree);

    op2 = mustConvert(op2, retType, tree);
    if (!op2) return newError(tree);

    EXPRWRAP * wrap = newExprWrap(op1, TK_SHORTLIVED);

    call->asCALL()->args->asBIN()->p1 = wrap;

    EXPR * callT = bindUDUnop(tree, EK_TRUE, wrap);
    EXPR * callF = bindUDUnop(tree, EK_FALSE, wrap);

    if (!callT || !callF) {
        errorSym(tree, ERR_MustHaveOpTF, retType);
        return newError(tree);
    }

    callT = mustConvert(callT, getPDT(PT_BOOL), tree);
    callF = mustConvert(callF, getPDT(PT_BOOL), tree);

    EXPRUSERLOGOP * rval = newExpr(tree, EK_USERLOGOP, retType)->asUSERLOGOP();
    rval->opX = op1;
    rval->callTF = kind == EK_LOGAND ? callF : callT;
    rval->callOp = call;
    rval->flags |= EXF_ASSGOP;

    return rval;
}

EXPR * FUNCBREC::bindPtrOp(BASENODE * tree, EXPRKIND kind, unsigned flags, EXPR * op1, EXPR * op2)
{
    EXPR * rval;

    checkUnsafe(tree);

    if (kind == EK_SUB && op1->type == op2->type) {
        if (op1->type == getVoidPtrType()) {
            errorN(tree, ERR_VoidError);
            return newError(tree);
        }
        // simple enough, just a substraction divided by the size of the underlying type...
        rval = newExprBinop(tree, EK_SUB, op1->type, op1, op2);
        rval = newExprBinop(tree, EK_DIV, getPDT(PT_INT), rval, bindSizeOf(tree, op1->type->parent->asTYPESYM()));
        rval = mustCast(rval, getPDT(PT_LONG), tree);
        return rval;
    }

    EXPR * intExpr, * ptrExpr;
    if (op1->type->kind == SK_PTRSYM) {
        intExpr = op2;
        ptrExpr = op1;
    } else {
        intExpr = op1;
        ptrExpr = op2;
    }

    if (flags & EXF_COMPARE) {
        // this is just a simple direct comparison...

        if (intExpr->type == compiler()->symmgr.GetNullType()) {
            intExpr = mustConvert(intExpr, getVoidPtrType(), tree);
            if (op1->type->kind == SK_PTRSYM) {
                rval = newExprBinop(tree, kind, getPDT(PT_BOOL), op1, intExpr);
            } else {
                rval = newExprBinop(tree, kind, getPDT(PT_BOOL), intExpr, op2);
            }
        } else {
            rval = newExprBinop(tree, kind, getPDT(PT_BOOL), op1, op2);
        }
        rval->flags |= flags;
        return rval;
    }

    ASSERT(kind == EK_SUB || kind == EK_ADD);

    if (ptrExpr->type == getVoidPtrType()) {
        errorN(tree, ERR_VoidError);
        return newError(tree);
    }

    bool largeSize = intExpr->type->fundType() == FT_I8 || intExpr->type->fundType() == FT_U8;

    EXPR * sizeExpr = bindSizeOf(tree, ptrExpr->type->parent->asTYPESYM());

    if (sizeExpr->kind != EK_CONSTANT || sizeExpr->asCONSTANT()->getVal().iVal != 1) {
        if (largeSize) {
            sizeExpr = mustCast(sizeExpr, intExpr->type, NULL);
        }
        intExpr = newExprBinop(tree, EK_MUL, intExpr->type, intExpr, sizeExpr);

    }
    


    if (largeSize) {
        intExpr = mustCast(intExpr, ptrExpr->type, NULL);
    }

    if (op1->type->kind == SK_PTRSYM) {
        rval = newExprBinop(tree, kind, ptrExpr->type, op1, intExpr);
    } else {
        rval = newExprBinop(tree, kind, ptrExpr->type, intExpr, op2);
    }
    rval->flags |= flags;

    return rval;
    
}


/*
 * Bind an equality operator: ==, !=. These can occur on integer, long, float, double, bool, or reference types.
 */
EXPR * FUNCBREC::bindEqualityOp(BASENODE * tree, EXPRKIND kind, EXPR * op1, EXPR * op2)
{
    FUNDTYPE ft1 = op1->type->fundType();
    FUNDTYPE ft2 = op2->type->fundType();
    EXPR * rval = NULL;
    BOOL noUserDefined = false;
    
    // decimal has user-defined operations, but we want to fold constant expressions
    // of this type. So check for that.
    rval = bindDecimalConstExpr(tree, kind, op1, op2);
    if (rval) return rval;

    // If we're comparing a delegate to null constant, skip the user-defined operator search and just
    // do a reference comparison.
    if ((op1->type->kind == SK_NULLSYM && op2->type->kind == SK_AGGSYM && op2->type->asAGGSYM()->isDelegate) ||
        (op2->type->kind == SK_NULLSYM && op1->type->kind == SK_AGGSYM && op1->type->asAGGSYM()->isDelegate))
    {
        noUserDefined = true;
    }

    TYPESYM * stringType = getPDT(PT_STRING);

    
    if (ft1 == FT_REF || ft2 == FT_REF) {


        // string compared to null is taken care of by bindRefEquality
        if ((op1->type->kind == SK_NULLSYM && op2->type == stringType) || 
            (op1->type == stringType && op2->type->kind == SK_NULLSYM)) {
            return bindRefEquality(tree, kind, op1, op2);
        }
        
        // comparisons of constant strings is done at compile time...
        if (op1->type == stringType && op1->kind == EK_CONSTANT && op2->type == stringType && op2->kind == EK_CONSTANT) {
            return bindConstStringEquality(tree, kind, op1, op2);
        }
    }

    // Try user-defined operator, but only if it's not a predefined type.
    if (! (ft1 > FT_R8 || ft1 == FT_NONE || ft2 > FT_R8 || ft2 == FT_NONE))
        noUserDefined = true;

    // Try user-defined operator.
    if (!noUserDefined) {
        rval = bindUDBinop(tree, kind, op1, op2);
        if (rval) return rval;
    }

    if (ft1 == FT_REF || ft2 == FT_REF) {
        return bindRefEquality(tree, kind, op1, op2);
    } else {
        TYPESYM * boolType = getPDT(PT_BOOL);
        if (op1->type == boolType || op2->type == boolType) { // Only accept BOOLs not SBytes                                                                                         
            return bindBoolOp(tree, kind, EXF_COMPARE, op1, op2);
        } else {
            return bindArithOp(tree, kind, EXF_COMPARE, op1, op2, true, true); 
        }
    }
}


/*
 * Constant fold an expression involving constant decimal operands. Return NULL if we weren't able to 
 * fold it.
 */
EXPR * FUNCBREC::bindDecimalConstExpr(BASENODE * tree, EXPRKIND kind, EXPR * op1, EXPR * op2)
{
    DECIMAL d1;
    DECIMAL d2;       
    bool isBoolResult = false;
    DECIMAL result;              // used if isBoolResult is false.
    bool result_b = false;       // used if isBoolResult is true.
    PTYPESYM typeDecimal = compiler()->symmgr.GetPredefType(PT_DECIMAL, false);
    HRESULT hr = NOERROR;

    if (!typeDecimal)
        return NULL;

    // Operands must be constants, and one or the other must be decimal..
    if (! (op1->kind == EK_CONSTANT && (!op2 || op2->kind == EK_CONSTANT)))
        return NULL;

    // Coerce operands to decimal. One must be decimal, but the other need not be if
    // it can be converted to a decimal constant.
    if (op1->type != typeDecimal) {
        if (!op2 || op2->type != typeDecimal)
            return NULL;
        op1 = tryConvert(op1, typeDecimal, tree);
        if (!op1 || op1->kind != EK_CONSTANT)
            return NULL;
    }
    else if (op2 && op2->type != typeDecimal) {
        if (op1->type != typeDecimal)
            return NULL;
        op2 = tryConvert(op2, typeDecimal, tree);
        if (!op2 || op2->kind != EK_CONSTANT)
            return NULL;
    }

    // OK we have two decimal constants.
    // Get operands.
    d1 = * op1->asCONSTANT()->getVal().decVal;
    if (op2) {
        d2 = * op2->asCONSTANT()->getVal().decVal;
    }

    // Do the operation.
    switch (kind) {
    case EK_ADD:     hr = VarDecAdd(&d1, &d2, &result); break;
    case EK_SUB:     hr = VarDecSub(&d1, &d2, &result); break;
    case EK_MUL:     hr = VarDecMul(&d1, &d2, &result); break;
    case EK_NEG:     hr = VarDecNeg(&d1, &result); break;
    case EK_DIV:     
        if (DECIMAL_HI32(d2) == 0 && DECIMAL_LO32(d2) == 0 && DECIMAL_MID32(d2) == 0) {
            errorN(tree, ERR_IntDivByZero);
            hr = E_FAIL;
        }
        else
            hr = VarDecDiv(&d1, &d2, &result); 
        break;

    case EK_MOD:  
    {
        /* n % d = n - d * truncate(n/d) */
        DECIMAL ndivd, trunc, product;

        if (DECIMAL_HI32(d2) == 0 && DECIMAL_LO32(d2) == 0 && DECIMAL_MID32(d2) == 0) {
            errorN(tree, ERR_IntDivByZero);
            hr = E_FAIL;
        }
        else
            hr = VarDecDiv(&d1, &d2, &ndivd); 

        if (SUCCEEDED(hr))
            hr = VarDecFix(&ndivd, &trunc);  
        if (SUCCEEDED(hr))
            hr = VarDecMul(&trunc, &d2, &product);
        if (SUCCEEDED(hr))
            hr = VarDecSub(&d1, &product, &result);
        break;
    }

    case EK_EQ:      hr = VarDecCmp(&d1, &d2); 
                     result_b = (hr == VARCMP_EQ); 
                     isBoolResult = true;
                     break;
    case EK_NE:      hr = VarDecCmp(&d1, &d2);
                     result_b = (hr != VARCMP_EQ); 
                     isBoolResult = true;
                     break;
    case EK_LE:      hr = VarDecCmp(&d1, &d2); 
                     result_b = (hr == VARCMP_LT || hr == VARCMP_EQ); 
                     isBoolResult = true;
                     break;
    case EK_LT:      hr = VarDecCmp(&d1, &d2); 
                     result_b = (hr == VARCMP_LT); 
                     isBoolResult = true;
                     break;
    case EK_GE:      hr = VarDecCmp(&d1, &d2);
                     result_b = (hr == VARCMP_GT || hr == VARCMP_EQ); 
                     isBoolResult = true;
                     break;
    case EK_GT:      hr = VarDecCmp(&d1, &d2); 
                     result_b = (hr == VARCMP_GT); 
                     isBoolResult = true;
                     break;

    default:         return NULL; // not a constant operation we support.
    }

    if (FAILED(hr))
        return NULL;        // operation failed.

    // Allocate the result node.
    CONSTVAL cv;
    PREDEFTYPE pt;
    EXPR * expr;

    if (isBoolResult) {
        cv.iVal = result_b;
        pt = PT_BOOL;
    }
    else {
        cv.decVal = (DECIMAL *) allocator->Alloc(sizeof(DECIMAL));
        * cv.decVal = result;
        pt = PT_DECIMAL;
    }

    expr = newExprConstant(tree, getPDT(pt), cv);
    return expr;
}

/*
 * Bind a constant cast to or from decimal. Return null if cast can't be done.
 */
EXPR * FUNCBREC::bindDecimalConstCast(BASENODE * tree, TYPESYM * destType, TYPESYM * srcType, EXPRCONSTANT * src)
{
    TYPESYM * typeDecimal = compiler()->symmgr.GetPredefType(PT_DECIMAL, false);
    HRESULT hr = NOERROR;
    CONSTVAL cv;

    if (!typeDecimal)
        return NULL;

    if (destType == typeDecimal) {
        // Casting to decimal.

        FUNDTYPE ftSrc = srcType->fundType();
        DECIMAL result;

        switch (ftSrc) {

        case FT_I1:
        case FT_I2:
        case FT_I4:
            hr = VarDecFromI4(src->getVal().iVal, &result); break;
        case FT_U1:
        case FT_U2:
        case FT_U4:
            hr = VarDecFromUI4(src->getVal().uiVal, &result); break;
        case FT_R4:
            hr = VarDecFromR4((float) (* src->getVal().doubleVal), &result); break;
        case FT_R8:
            hr = VarDecFromR8(* src->getVal().doubleVal, &result); break;
        case FT_U8:
        case FT_I8:
            /* I8,U8 -> decimal. No API for this, do it manually. */
            __int64 srcLong;
            
            srcLong = *src->getVal().longVal;

            DECIMAL_SCALE(result) = 0;
            DECIMAL_SIGN(result) = 0;
            if (srcLong < 0 && ftSrc == FT_I8) { // Only worry about the sign if it is signed
                DECIMAL_SIGN(result) |= DECIMAL_NEG;
                srcLong = -srcLong;
            }

            DECIMAL_MID32(result) = (ULONG)(srcLong>>32);
            DECIMAL_LO32(result) = (ULONG)srcLong;
            DECIMAL_HI32(result) = 0;
            break;

        default:
            hr = E_FAIL;  // Not supported cast.
        }

        if (FAILED(hr))
            return NULL;

        cv.decVal = (DECIMAL *) allocator->Alloc(sizeof(DECIMAL));
        * cv.decVal = result;
        return newExprConstant(tree, typeDecimal, cv);
    }
    else if (srcType == typeDecimal) {
        // Casting from decimal

        FUNDTYPE ftDest = destType->fundType();

        switch (ftDest) {
        case FT_I1:
            CHAR c;
            hr = VarI1FromDec(src->getVal().decVal, &c);
            cv.iVal = c;
            break;

        case FT_U1: 
            BYTE b;
            hr = VarUI1FromDec(src->getVal().decVal, &b);
            cv.uiVal = b;
            break;

        case FT_I2:
            SHORT s;
            hr = VarI2FromDec(src->getVal().decVal, &s);
            cv.iVal = s;
            break;

        case FT_U2:
            USHORT u;
            hr = VarUI2FromDec(src->getVal().decVal, &u);
            cv.uiVal = u;
            break;

        case FT_I4:
            LONG i;
            hr = VarI4FromDec(src->getVal().decVal, &i);
            cv.iVal = i;
            break;

        case FT_U4:
            ULONG ui;
            hr = VarUI4FromDec(src->getVal().decVal, &ui);
            cv.uiVal = ui;
            break;

        case FT_I8:
            /* convert decimal to long -- no API for this (sigh) */
            DECIMAL decTrunc;

            // First truncate to integer.
            hr = VarDecFix(src->getVal().decVal, &decTrunc);

            if (SUCCEEDED(hr)) {
                ASSERT(decTrunc.scale == 0);

                // Are we out of range of a long. The most negative long makes this test more complex than you would think.
                if (DECIMAL_HI32(decTrunc) != 0 || 
                    ((DECIMAL_MID32(decTrunc) & 0x80000000) != 0 && 
                    (DECIMAL_MID32(decTrunc) != 0x80000000 || DECIMAL_LO32(decTrunc) != 0 || (DECIMAL_SIGN(decTrunc) & DECIMAL_NEG))))
                {
                    hr = E_FAIL;  // Out of range.
                }
                else {
                    cv.longVal = (__int64 *) allocator->Alloc(sizeof(__int64));
                    *cv.longVal = (__int64)(((ULONGLONG)DECIMAL_MID32(decTrunc) << 32) | DECIMAL_LO32(decTrunc));
                    if (DECIMAL_SIGN(decTrunc) & DECIMAL_NEG)
                    {
                        *cv.longVal = -(__int64)*cv.longVal;
                    }
                }
            }
            break;

        case FT_U8:
            /* convert decimal to ulong -- no API for this (sigh) */

            // First truncate to integer.
            hr = VarDecFix(src->getVal().decVal, &decTrunc);

            if (SUCCEEDED(hr)) {
                ASSERT(decTrunc.scale == 0);

                // Are we out of range of a ulong.
                if (DECIMAL_HI32(decTrunc) != 0 || (DECIMAL_SIGN(decTrunc) & DECIMAL_NEG))
                {
                    hr = E_FAIL;  // Out of range.
                }
                else {
                    cv.ulongVal = (unsigned __int64 *) allocator->Alloc(sizeof(unsigned __int64));
                    *cv.ulongVal = (ULONGLONG)(((ULONGLONG)DECIMAL_MID32(decTrunc) << 32) | DECIMAL_LO32(decTrunc));
                }
            }
            break;

        case FT_R4:
            float f;
            hr = VarR4FromDec(src->getVal().decVal, &f);
            cv.doubleVal = (double *) allocator->Alloc(sizeof(double));
            * cv.doubleVal = f;
            break;

        case FT_R8:
            double d;
            hr = VarR8FromDec(src->getVal().decVal, &d);
            cv.doubleVal = (double *) allocator->Alloc(sizeof(double));
            * cv.doubleVal = d;
            break;

        default:
            hr = E_FAIL;  // Not supported cast.
        }

        if (FAILED(hr))
            return NULL;

        return newExprConstant(tree, destType, cv);
    }
    else
        return NULL;
}



inline void FUNCBREC::checkLvalue(EXPR * expr, bool isAssignment)
{

    if (expr->isOK()) {
        if (!(expr->flags & EXF_LVALUE)) {
            // We have a lvalue failure. Was the reason because this field
            // was marked readonly? Give special messages for this case..
            bool isReadonly = (expr->kind == EK_FIELD && expr->asFIELD()->field->isReadOnly);
            bool isStatic = false;

            if (isReadonly)
                isStatic = expr->asFIELD()->field->isStatic;

            if (isAssignment) {
                if (expr->kind == EK_PROP && expr->asPROP()->prop->methSet == NULL) 
                    errorSym(expr->tree, ERR_AssgReadonlyProp, expr->asPROP()->prop);
                else if (expr->kind == EK_ARRLEN) {
                    errorSym(expr->tree, ERR_AssgReadonlyProp, 
                             lookupClassMember(compiler()->namemgr->GetPredefName(PN_LENGTH), 
                                               getPDT(PT_ARRAY)->asAGGSYM(), false, false, true));
                }
                else if (expr->kind == EK_LOCAL)
                    errorName(expr->tree, ERR_AssgReadonlyLocal, expr->asLOCAL()->local->name);
                else {
                    EXPR * object = NULL;
                    if (expr->kind == EK_PROP) {
                        ASSERT(expr->asPROP()->prop->methSet);
                        object = expr->getObject();
                    } else if (expr->kind == EK_FIELD && !isReadonly && !isStatic)
                        object = expr->getObject();

                    if (object && (object->kind == EK_CALL || object->kind == EK_PROP)) {
                        errorSym(object->tree, ERR_ReturnNotLValue, object->getMember());
                    } else {
                        errorN(expr->tree, isReadonly ? (isStatic ? ERR_AssgReadonlyStatic : ERR_AssgReadonly) : ERR_AssgLvalueExpected);
                    }
                }
            }
            else if (expr->kind == EK_LOCAL)
                errorName(expr->tree, ERR_RefReadonlyLocal, expr->asLOCAL()->local->name);
            else if (expr->kind == EK_PROP)
                errorN(expr->tree, ERR_RefProperty);
            else
                errorN(expr->tree, isReadonly ? (isStatic ? ERR_RefReadonlyStatic : ERR_RefReadonly) : ERR_RefLvalueExpected);
        } else {
            // otherwise, even if it is an lvalue, it may be an abstract prop call, which is also illegel...
            if (expr->kind == EK_PROP && (expr->flags & EXF_BASECALL) && expr->asPROP()->prop->methSet->isAbstract) {
                errorSym(expr->tree, ERR_AbstractBaseCall, expr->asPROP()->prop);
            }
            markFieldAssigned(expr);
        }
    }
}

// Sets the isAssigned bit
void FUNCBREC::markFieldAssigned(EXPR * expr)
{
    if (expr->isOK() && (expr->flags & EXF_LVALUE) && expr->kind == EK_FIELD) {
        EXPRFIELD *field;
        
        do {
            field = expr->asFIELD();
            field->field->isAssigned = true;
            expr = field->object;
        } while (field->field->getClass()->isStruct && !field->field->isStatic && expr && expr->kind == EK_FIELD);
    }
}


void FUNCBREC::checkUnsafe(BASENODE * tree, TYPESYM * type, int errCode)
{
    if ((type == NULL || type->isUnsafe()) && !unsafeContext && !unsafeErrorGiven &&
        (pClassInfo == NULL || !pClassInfo->isUnsafe)) {
        unsafeErrorGiven = true;
        errorN(tree, errCode);
    }
}

// Taking the reference of a field is illegal if the type is marshalbyref.
// Also checks for passing volatile field by-ref.
void FUNCBREC::checkFieldRef(EXPRFIELD * expr, bool refParam)
{
    if (expr->field->parent->asAGGSYM()->isMarshalByRef) {
        // we ignore it, if its the this pointer..., or a static field
        if (!expr->field->isStatic && expr->object && (expr->object->kind != EK_LOCAL || expr->object->asLOCAL()->local != thisPointer)) {

            if (refParam || expr->type->isStructType()) {
                errorSymSym(expr->tree, ERR_ByRefNonAgileField, expr->field, expr->field->parent);
            }
        }
    }
    if (refParam && expr->field->isVolatile)
        errorSym(expr->tree, ERR_VolatileByRef, expr->field);
}


inline void FUNCBREC::noteReference(EXPR * op)
{

    MEMBVARSYM * field;

    if (op->kind == EK_LOCAL && !op->asLOCAL()->local->slot.isReferenced) {
        op->asLOCAL()->local->slot.isReferenced = true;
        unreferencedVarCount --;
    } else if (op->kind == EK_FIELD && !(field = op->asFIELD()->field)->isReferenced) {
        field->isReferenced = true;
    }
}

EXPR * FUNCBREC::bindPtrToString(BASENODE * tree, EXPR * string)
{
    if (!info->hStringOffset) {
        NAME * name = compiler()->namemgr->GetPredefName(PN_OFFSETTOSTRINGDATA);
        AGGSYM * helpers = getPDT(PT_RUNTIMEHELPERS)->asAGGSYM();
        SYM * property = lookupClassMember(name, helpers, false, false, true);
        if (!property || property->kind != SK_PROPSYM || !property->asPROPSYM()->methGet) {
            errorSymName(tree, ERR_MissingPredefinedMember, helpers, name);
            return newError( tree);
        }
        info->hStringOffset = property->asPROPSYM()->methGet;
    }

    EXPR * rval = newExprBinop(tree, EK_ADDR, compiler()->symmgr.GetPtrType(getPDT(PT_CHAR)), string, NULL);

    return rval;

}


EXPR * FUNCBREC::bindPtrToArray(BASENODE * tree, EXPR * array)
{
    
    TYPESYM * elem = array->type->asARRAYSYM()->elementType();

    // element must be unmanaged...
    if (isManagedType(elem)) {
        errorSym(tree, ERR_ManagedAddr, elem);
    }

    {
        AGGSYM * temp = elem->underlyingAggregate();
        if (temp && temp->isStruct)
            temp->hasExternReference = true;
    }

    EXPR * list = NULL;
    EXPR ** pList = &list;

    for (int cc = 0; cc < array->type->asARRAYSYM()->rank; cc++) {
        CONSTVAL cv;
        cv.iVal = 0;
        newList(newExprConstant(tree, getPDT(PT_INT), cv), &pList);
    }

    ASSERT(list);

    EXPR * rval = newExprBinop(tree, EK_ARRINDEX, elem, array, list);
    rval->flags |= EXF_ASSGOP;
    
    rval = newExprBinop(tree, EK_ADDR, compiler()->symmgr.GetPtrType(elem), rval, NULL);

    return rval;

}


/*
 * Bind the simple assignment operator =.
 */
EXPR * FUNCBREC::bindAssignment(BASENODE * tree, EXPR * op1, EXPR * op2)
{
    EXPR * expr;

    if (op1->kind != EK_LOCAL || !op1->asLOCAL()->local->slot.mustBePinned) {
        checkLvalue(op1);
    } else {
        op1->asLOCAL()->local->slot.mustBePinned = false;
        op1->asLOCAL()->local->slot.isPinned = true;
        if (op2->type->kind == SK_ARRAYSYM) {
            op2 = bindPtrToArray(tree, op2);
        } else if (op2->type == getPDT(PT_STRING)) {
            op2 = bindPtrToString(tree, op2);
        } else if (op2->kind != EK_ADDR && op2->kind != EK_ERROR) {
            if (op2->kind == EK_CAST) {
                errorN(tree, ERR_BadCastInFixed);
            } else {
                errorN(tree, ERR_FixedNotNeeded);
            }
        }
    }

    op2 = mustConvert(op2, op1->type, op2->tree ? op2->tree : tree);

    if (op2->kind != EK_CONSTANT) {
        noteReference(op1);
        if (op2->kind == EK_CALL && (op2->flags & EXF_NEWOBJCALL) && (op2->type->isStructType()) && op1->kind != EK_PROP) {
            ASSERT(!op2->asCALL()->object);
            op2->asCALL()->object = op1;
            op2->type = op1->type;
            op2->flags &= ~EXF_NEWOBJCALL;
            op2->flags |= EXF_NEWSTRUCTASSG;
            return op2;
        } else if (op2->kind == EK_ZEROINIT && !op2->asZEROINIT()->p1 && op1->kind != EK_PROP) {
            op2->asZEROINIT()->p1 = op1;
            return op2;
        }
    } else {
        if (op1->kind == EK_LOCAL) {
            op1->asLOCAL()->local->slot.isReferencedAssg = true;
        }
    }

    expr = newExprBinop(tree, EK_ASSG, op1->type, op1, op2);
    expr->flags |= EXF_ASSGOP;

    return expr;
}


EXPR * FUNCBREC::bindIndexer(BASENODE * tree, EXPR * object, EXPR * args, int bindFlags)
{

    SYM * prop;
    NAME * items;
    TYPESYM * type = object->type;

    if (type->kind != SK_AGGSYM) goto LERROR;

    items = compiler()->namemgr->GetPredefName(PN_INDEXERINTERNAL);

    prop = lookupClassMember(items, type->asAGGSYM(), 0 != (bindFlags & BIND_BASECALL), false, false);

    if (!prop || prop->kind != SK_PROPSYM) goto LERROR;

    return bindToProperty(tree, object, prop->asPROPSYM(), bindFlags, args);

LERROR:

    errorSym(tree, ERR_BadIndexLHS, type);
    return newError(tree);

}


EXPR * FUNCBREC::bindBase(BASENODE * tree)
{
    EXPR * rval = bindThisImplicit(tree);
    if (! rval) {
        if (pMSym && pMSym->isStatic)
            errorN(tree, ERR_BaseInStaticMeth);
        else
            errorN(tree, ERR_BaseInBadContext); // 'this' isn't available for some other reason.

        return newError(tree);
    } else {
        rval->flags &= ~EXF_IMPLICITTHIS;
        if (rval->type->asAGGSYM()->baseClass) {
            rval = tryConvert(rval, rval->type->asAGGSYM()->baseClass, tree);
            ASSERT(rval);
        } else {
            errorN(tree, ERR_NoBaseClass);
        }
    }

    return rval;
}


EXPR * FUNCBREC::bindSizeOf(BASENODE * tree, TYPESYM * type)
{


    if (type->isSimpleType()) {
        CONSTVAL cv;
        cv.iVal = compiler()->symmgr.GetAttrArgSize((PREDEFTYPE) type->asAGGSYM()->iPredef);
        if (cv.iVal > 0) {
            return newExprConstant(tree, getPDT(PT_INT), cv);
        }
    }

    EXPRSIZEOF * rval = newExpr(tree, EK_SIZEOF, getPDT(PT_INT))->asSIZEOF();
    rval->sourceType = type;

    return rval;
}


EXPR * FUNCBREC::bindPtrArrayIndexer(BINOPNODE * tree, EXPR * opArr, EXPR * opIndex)
{

    ASSERT(opArr->type->kind == SK_PTRSYM);

    if (opIndex->kind == EK_LIST) {
        errorN(tree, ERR_PtrIndexSingle);
        return newError(tree);
    }
    if (opArr->type == getVoidPtrType()) {
        errorN(tree, ERR_VoidError);
        return newError(tree);
    }

    TYPESYM * destType = chooseArrayIndexType(tree, opIndex);

    if (!destType) {
        // using int as the type will allow us to give a better error...
        destType = getPDT(PT_INT);
    }

    opIndex = mustConvert(opIndex, destType, opIndex->tree);

    bool largeSize = opIndex->type->fundType() > FT_LASTNONLONG;

    TYPESYM * type = opArr->type->parent->asTYPESYM();

    EXPR * size = bindSizeOf(tree, type);

    if (size->kind == EK_CONSTANT && size->asCONSTANT()->getVal().iVal == 1) {
        if (largeSize) {
            opIndex = mustCast(opIndex, opArr->type, NULL);
        }
    } else {
        if (largeSize) {
            size = mustCast(size, destType, NULL);
        }
        opIndex = newExprBinop(tree, EK_MUL, destType, size, opIndex);
        if (largeSize) {
            opIndex = mustCast(opIndex, opArr->type, NULL);
        }
    }

    EXPR * add = newExprBinop(tree, EK_ADD, opArr->type, opArr, opIndex);
    EXPR * rval = newExprBinop(tree, EK_INDIR, type, add, NULL);
    rval->flags |= EXF_LVALUE | EXF_ASSGOP;

    return rval;

}

TYPESYM * FUNCBREC::chooseArrayIndexType(BASENODE * tree, EXPR * args)
{
    TYPESYM *possibleTypes[4];
        
    possibleTypes[0] = getPDT(PT_INT);
    possibleTypes[1] = getPDT(PT_UINT);
    possibleTypes[2] = getPDT(PT_LONG);
    possibleTypes[3] = getPDT(PT_ULONG);


    // first, select the allowable types
    for (int i = 0; i < 4; i++) {
        EXPRLOOP(args, arg)
            if (!canConvert(arg, possibleTypes[i], arg->tree)) {
                goto NEXTI;
            }
        ENDLOOP;
        return possibleTypes[i];
NEXTI:;
    }

    return NULL;
}


/*
 * Bind an array being dereferenced by one or more indexes: a[x], a[x,y], ...
 */
EXPR * FUNCBREC::bindArrayIndex(BINOPNODE * tree, EXPR * op2, int bindFlags)
{

    TYPESYM * intType = getPDT(PT_INT);

    PARRAYSYM arrayType;
    int cIndices = 0;
    int rank;
    EXPR * expr;

    EXPR * op1;

    // base[foobar] is actually valid...
    if (tree->p1->kind == NK_OP && tree->p1->Op() == OP_BASE) {
        expr = bindIndexer(tree, bindBase(tree->p1), op2, bindFlags | BIND_BASECALL);
        expr->flags |= EXF_BASECALL;
        return expr;
    }

    op1 = bindExpr(tree->p1);

    RETBAD(op1);

    // Array indexing must occur on an array type.
    compiler()->symmgr.DeclareType(op1->type);
    if (op1->type->kind != SK_ARRAYSYM) {

        if (op1->type->kind == SK_PTRSYM) {
            return bindPtrArrayIndexer(tree, op1, op2);
        }

        return bindIndexer(tree, op1, op2, bindFlags);

    }
    arrayType = op1->type->asARRAYSYM();

    // Check the rank of the array against the number of indices provided, and
    // convert the indexes to ints

    TYPESYM * destType = chooseArrayIndexType(tree, op2);

    if (!destType) {
        // using int as the type will allow us to give a better error...
        destType = intType;
    }

    EXPR * temp;
    EXPR ** next = &op2;
    EXPR ** first;
AGAIN:
    if (next[0]->kind == EK_LIST) {
        first = &(next[0]->asBIN()->p1);
        next = &(next[0]->asBIN()->p2);
LASTONE:
        cIndices++;
        temp = mustConvert(*first, destType, first[0]->tree);
        checkNegativeConstant(first[0]->tree, temp, WRN_NegativeArrayIndex);
        if (destType == intType) {
            *first = temp;
        } else {
            *first = newExpr(first[0]->tree, EK_CAST, destType);
            first[0]->asCAST()->p1 = temp;
            first[0]->asCAST()->flags |= EXF_INDEXEXPR;
        }
        if (first != next) goto AGAIN;
    } else {
        first = next;
        goto LASTONE;
    }

    rank = arrayType->rank;
    if (cIndices != rank) {
        errorInt(tree, ERR_BadIndexCount, rank);
        return newError(tree);
    }

    // Allocate a new expression, the type is the element type of the array.
    // Array index operations are always lvalues.
    expr = newExprBinop(tree, EK_ARRINDEX, arrayType->elementType(), op1, op2);
    expr->flags |= EXF_LVALUE | EXF_ASSGOP;

    return expr;
}

void EXPRCONSTANT::realizeStringConstant()
{
    if (!list) return;

    unsigned totLen = 0;
    if (val.strVal) {
        totLen = val.strVal->length;
    }
    EXPRLOOP(list, item)
        EXPRCONSTANT * econst = item->asCONSTANT();
        ASSERT(!econst->list);
        if (econst->val.strVal) {
            totLen += econst->val.strVal->length;
        }
    ENDLOOP;

    STRCONST * str = (STRCONST*) allocator->Alloc(sizeof(STRCONST));

    str->text = (WCHAR*) allocator->Alloc(sizeof(WCHAR) * totLen);
    str->length = totLen;
    WCHAR * ptr = str->text;
    if (val.strVal) {
        memcpy(ptr, val.strVal->text, val.strVal->length * sizeof(WCHAR));
        ptr += val.strVal->length;
    }
    EXPRLOOP(list, item)
        EXPRCONSTANT * econst = item->asCONSTANT();
        if (econst->val.strVal) {
            memcpy(ptr, econst->val.strVal->text, econst->val.strVal->length * sizeof(WCHAR));
            ptr += econst->val.strVal->length;
        }
    ENDLOOP;
    val.strVal = str;

    list = NULL;
    pList = &list;
}


EXPR * FUNCBREC::bindStringConcat(BASENODE * tree, EXPR * op1, EXPR * op2)
{
    EXPR  * firstB, * lastA;

    if (op1->kind == EK_CONCAT) {
        lastA = *(op1->asCONCAT()->pList);
    } else {
        lastA = op1;
    }

    if (op2->kind == EK_CONCAT) {
        firstB = op2->asCONCAT()->list->asBIN()->p1;
    } else {
        firstB = op2;
    }

    if (lastA->kind == EK_CONSTANT && firstB->kind == EK_CONSTANT) {

        EXPR * second;
        if (firstB->asCONSTANT()->list) {
            // we need to convert it to a BINOP list
            second = newExprBinop(NULL, EK_LIST, NULL, firstB, firstB->asCONSTANT()->list);
            firstB->asCONSTANT()->list = NULL;
            firstB->asCONSTANT()->pList = &(firstB->asCONSTANT()->list);
        } else {
            second = firstB;
        }

        newList(second, &(lastA->asCONSTANT()->pList));
        lastA->asCONSTANT()->allocator = allocator;

        if (firstB == op2) { // if second was an item, and not a list
            return op1; // just return the first...
        }

        if (op2->asCONCAT()->count == 2) {
            // now this list is really an item...
            op2 = firstB = op2->asCONCAT()->list->asBIN()->p2;
        } else {
            op2->asCONCAT()->list = op2->asCONCAT()->list->asBIN()->p2;
            op2->asCONCAT()->count --;
        }
    }

    if (firstB == op2) { // ???? + item
        if (lastA != op1) { // LIST + ????
            newList(op2, &op1->asCONCAT()->pList);
            op1->asCONCAT()->count += 1;
            return op1;
        } else { // item + ????
            EXPRCONCAT * rval = newExpr(tree, EK_CONCAT, getPDT(PT_STRING))->asCONCAT();
            rval->list = op1;
            rval->pList = &rval->list;
            newList(op2, &rval->pList);
            rval->count = 2;
            rval->flags |= EXF_UNREALIZEDCONCAT;
            unrealizedConcatCount++;
            return rval;
        }
    } else { // ???? + LIST
        if (lastA != op1) { // LIST + ????
            newList(op2->asCONCAT()->list, &op1->asCONCAT()->pList);
            op1->asCONCAT()->pList = op2->asCONCAT()->pList;
            op1->asCONCAT()->count += op2->asCONCAT()->count;
            return op1;
        } else { // item + ????
            op2->asCONCAT()->list = newExprBinop(NULL, EK_LIST, NULL, op1, op2->asCONCAT()->list);
            op2->asCONCAT()->count ++;
            return op2;
        }
    }
}



// Bind "is" or "as" operator.
EXPR * FUNCBREC::bindIs(BINOPNODE * tree, EXPR * op1)
{
    ASSERT(tree->Op() == OP_IS || tree->Op() == OP_AS);

    bool isAs = (tree->Op() == OP_AS);
    BASENODE * op2 = tree->p2;

    TYPESYM * type2 = NULL;
    TYPESYM * type1;

    type1 = op1->type;

    // 2nd operand can be literal type or a expression of type System.Type.
    type2 = bindType(op2->asTYPE());

    if (!type2 || type2 == compiler()->symmgr.GetErrorSym()) goto LERROR;

    if (type1->kind == SK_PTRSYM || type2->kind == SK_PTRSYM) {
        errorN(tree, ERR_PointerInAsOrIs);
        goto LERROR;
    }

    // Now, we know that "A is B" is true iff A is implicitly convertible to B
    // and B is a reference... Otherwise, if B is not a reference, then we know that
    // it is B if B is Object or A. 

    FUNDTYPE ft2;
    ft2 = type2->fundType();

    if (isAs && type2->fundType() != FT_REF) {
        // "as" must have reference type.
        errorSym(tree, ERR_AsMustHaveReferenceType, type2);
        goto LERROR;
    }

    if (type1 == type2 || type2 == getPDO() || (ft2 == FT_REF && canConvert(op1, type2, tree, CONVERT_NOUDC))) {
        if (isAs) {
            return mustConvert(op1, type2, tree);
        }
        else {
            if (op1->type == compiler()->symmgr.GetNullType()) // in the case of "null", the correct mesage is "always false".
                errorSym(tree, WRN_IsAlwaysFalse, type2);
            else
                errorSym(tree, WRN_IsAlwaysTrue, type2);

            // There are two sub-cases here. If type1 is a value type, then the result is known to always be true. 
            // If type1 is a reference type, however, then the results is true iff op1 is non-null. So we compile
            // to the equivalent of (op1 != null).
            if (type1->fundType() == FT_REF) {
                EXPR * rval = bindRefEquality(tree, EK_NE, op1, bindNull(tree));
                return rval;
            }
            else {
                CONSTVAL cv;
                cv.iVal = 1;
                EXPR * rval = newExprBinop(tree, EK_SEQUENCE, getPDT(PT_BOOL), op1, newExprConstant(op2, getPDT(PT_BOOL), cv));
                return rval;
            }
        }
    }

    // Now, we know that "A is B" is always false if there exists no explicit conversion to B,
    // as long as B is a reference.  If B is not a reference, then the answer is always false
    // unless A is B, which was caught above, or A is of a reference type that B implicitly converts to (e.g., Object,
    // or an interface implemented by B).
    if ((ft2 == FT_REF && !canCast(op1, type2, tree, CONVERT_NOUDC)) ||
        (ft2 != FT_REF && !(type1->fundType() == FT_REF && canConvert(type2, type1, tree, CONVERT_NOUDC))))
    {

        if (isAs) {
            errorSymSym(tree, ERR_NoExplicitBuiltinConv, type1, type2);
            EXPR * rval = newExprBinop(tree, EK_SEQUENCE, type2, op1, mustConvert(bindNull(tree), type2, tree));
            return rval;
        }
        else {
            CONSTVAL cv;
            errorSym(tree, WRN_IsAlwaysFalse, type2);
            cv.iVal = 0;
            EXPR * rval = newExprBinop(tree, EK_SEQUENCE, getPDT(PT_BOOL), op1, newExprConstant(op2, getPDT(PT_BOOL), cv));
            return rval;
        }
    }

    ASSERT(type1->fundType() == FT_REF);

    EXPR * expr2;
    expr2 = newExpr(op2, EK_TYPEOF, getPDT(PT_TYPE));
    expr2->asTYPEOF()->sourceType = type2;
    expr2->asTYPEOF()->method = NULL;

    if (op1->isNull()) {
        errorN(op1->tree, ERR_NullNotValid);
    }

    if (isAs) 
        return newExprBinop(tree, EK_AS, type2, op1, expr2);
    else
        return newExprBinop(tree, EK_IS, getPDT(PT_BOOL), op1, expr2);

LERROR:

    return newError(tree);

}

EXPR * FUNCBREC::bindRefValue(BINOPNODE * tree)
{
    EXPR * rany = bindExpr(tree->p1);
    rany = mustConvert(rany, getPDT(PT_REFANY), tree->p1);
    TYPESYM * type = bindType(tree->p2->asTYPE());
    if (!type) return newError(tree);

    EXPR * rval = newExprBinop(tree, EK_VALUERA, type, rany, newExpr(EK_TYPEOF));
    rval->asBIN()->p2->asTYPEOF()->sourceType = type;

    rval->flags |= EXF_ASSGOP | EXF_LVALUE;

    return rval;
}

// Bind an event add (+=) or event remove (-=) expression. Becomes
// a call to the adder or remover accessor.
EXPR * FUNCBREC::bindEventAccess(BASENODE * tree, bool isAdd, EXPREVENT * exprEvent, EXPR * exprRHS)
{
    // Convert RHS to the type of the event.
    EVENTSYM * eventSym = exprEvent->event;
    exprRHS = mustConvert(exprRHS, exprEvent->type, tree);

    // Get the correct accessor to call.
    METHSYM * accessor = isAdd ? eventSym->methAdd : eventSym->methRemove;

    if ((exprEvent->flags & EXF_BASECALL) && accessor->asMETHSYM()->isAbstract) {
        errorSym(tree, ERR_AbstractBaseCall, eventSym);
    }

    // Call the accessor.
    EXPRCALL * call = newExpr(tree, EK_CALL, getVoidType())->asCALL();
    call->object = exprEvent->object;
    call->flags |= EXF_BOUNDCALL | (exprEvent->flags & EXF_BASECALL);
    call->method = accessor;
    call->args = exprRHS;

    return call;
}

// Bind a binary expression (or statement...)
EXPR * FUNCBREC::bindBinop(BINOPNODE * tree, int bindFlags)
{
    ASSERT(tree->kind == NK_BINOP);

    OPERATOR op = tree->Op();
    EXPR * op1;
    EXPR * op2;

    // first, consider the special expressions:
    switch (op) {
    case OP_CALL:
        return bindCall(tree, bindFlags);
    case OP_REFVALUE:
        return bindRefValue(tree);
    case OP_ASSIGN: {
        // Simple assignment.
        EXPR * op1 = bindExpr(tree->p1, BIND_MEMBERSET);
        RETBAD(op1);
        EXPR * op2 = bindExpr(tree->p2);
        RETBAD(op2);
        return bindAssignment(tree, op1, op2);
                    }

    case OP_DEREF: {
        // Array indexing
        EXPR * op2 = bindExpr(tree->p2);
        RETBAD(op2);
        return bindArrayIndex(tree, op2, bindFlags);
                   }

    case OP_CAST: {
        TYPESYM * typeDest;

        // p1 is the type to cast to. p2 is the thing to cast.
        typeDest = bindType(tree->p1->asTYPE());
        return mustCast(bindExpr(tree->p2, BIND_RVALUEREQUIRED | (bindFlags & BIND_FIXEDVALUE)), typeDest, tree);
                  }

    case OP_ADDEQ:
    case OP_SUBEQ: 
    {
        // These are special if LHS is an event. Bind the LHS, but allow events. If we get an
        // event, then process specially, otherwise, bind +=, -= as usual.
        BASENODE * p1 = tree->p1;
        if (p1->kind == NK_NAME) 
            op1 = bindName(p1->asNAME(), MASK_EVENTSYM | MASK_MEMBVARSYM | MASK_LOCVARSYM | MASK_PROPSYM);
        else if (p1->kind == NK_DOT) 
            op1 = bindDot(p1->asDOT(), MASK_EVENTSYM | MASK_MEMBVARSYM | MASK_PROPSYM);
        else
            op1 = bindExpr(p1);
        RETBAD(op1);

        if (op1->kind == EK_EVENT) {
            op2 = bindExpr(tree->p2);
            RETBAD(op2);
            return bindEventAccess(tree, (op == OP_ADDEQ), op1->asEVENT(), op2);
        }
        else    
            goto BOUND_OP1;  // LHS isn't an event; go into regular +=, -= processing.

    }
    default:
        break;
    }

    // Then normal binop expressions where both children are always bound first:
    op1 = bindExpr(tree->p1);
    if (op1 && !op1->isOK()) {
        // Special check for badly formed cast of negative number.
        if (op1->asERROR()->extraInfo == ERROREXTRA_MAYBECONFUSEDNEGATIVECAST && op == OP_SUB)
        {
            errorN(tree, ERR_PossibleBadNegCast);
            op1->asERROR()->extraInfo = ERROREXTRA_NONE;
        }

        return op1;
    }

BOUND_OP1:

    if (op == OP_IS || op == OP_AS) {
        return bindIs(tree, op1);
    }

    op2 = bindExpr(tree->p2);
    RETBAD(op2);

    EXPRKIND ek = OP2EK[op];

    if (ek != EK_COUNT) {
        if (ek > EK_MULTIOFFSET) {
            ek = (EXPRKIND) (ek - EK_MULTIOFFSET);

            return bindMultiOp(tree, ek, op1, op2);
        } else {
            return bindTypicalBinop(tree, ek, op1, op2);
        }
    }

    switch(op) {
    case OP_QUESTION:
        return bindQMark(tree, op1, op2->asBINOP());
    case OP_COLON:
        return newExprBinop(tree, EK_BINOP, NULL, op1, op2);
    default:
        break;
    }

    errorStr(tree, ERR_UnimplementedOp, opName(op));
    return newError(tree);
}

EXPR * FUNCBREC::bindQMark(BASENODE * tree, EXPR * op1, EXPRBINOP * op2) 
{
    ASSERT(op2->kind == EK_BINOP);
    op1 = bindBooleanValue(tree, op1);

    EXPR * T = op2->asBIN()->p1;
    EXPR * S = op2->asBIN()->p2;
    EXPR * rval;

    if (T->type != S->type) {
        EXPR * T1 = tryConvert(T, S->type, T->tree);
        EXPR * S1 = tryConvert(S, T->type, S->tree);
        if (T1) {
            if (!S1) {
                op2->asBIN()->p1 = T1;
                ASSERT(T1->type == S->type);
            } else {
                RETBAD(T);
                errorSymSym(op2->tree, ERR_AmbigQM, T->type, S->type);
                goto LERROR;
            }
        } else { // ! T1
            if (S1) {
                op2->asBIN()->p2 = S1;
                ASSERT(T->type == S1->type);
            } else {
                errorSymSym(op2->tree, ERR_InvalidQM, T->type, S->type);
                goto LERROR;
            }
        }
    }

    if (op1->kind == EK_CONSTANT) {
        if (op1->asCONSTANT()->getVal().iVal) {
            // controlling bool is constant true.
            rval = op2->asBIN()->p1;
        }
        else {
            // controlling bool is constant false.
            rval = op2->asBIN()->p2;
        }
    }
    else {
        // Usual case: controlling bool is not a constant.
        rval = newExprBinop(tree, EK_QMARK, op2->asBIN()->p1->type, op1, op2);
    }

    return rval;

LERROR:

    return newError(tree);
}


EXPR * FUNCBREC::bindTypicalBinop(BASENODE * tree, EXPRKIND ek, EXPR *op1, EXPR * op2)
{
    switch (ek) {
    case EK_ADD:
    case EK_SUB:
    case EK_MUL:    return bindArithOp   (tree, ek, checked.normal ? EXF_CHECKOVERFLOW : 0, op1, op2, true, false);
    case EK_DIV:
    case EK_MOD:    return bindArithOp   (tree, ek, checked.normal ? EXF_CHECKOVERFLOW | EXF_ASSGOP : EXF_ASSGOP, op1, op2, true, false);
    case EK_BITAND: 
    case EK_BITXOR: 
    case EK_BITOR:  return bindArithOp   (tree, ek,     0, op1, op2, false, false);
    case EK_LSHIFT: 
    case EK_RSHIFT: return bindShiftOp   (tree, ek, 0, op1, op2);
    case EK_LOGOR:  
    case EK_LOGAND: return bindBoolOp    (tree, ek,  EXF_COMPARE, op1, op2);
    case EK_LT:
    case EK_LE:
    case EK_GT:
    case EK_GE:     return bindArithOp   (tree, ek,      EXF_COMPARE, op1, op2, true, true);
    case EK_EQ:
    case EK_NE:     return bindEqualityOp(tree, ek,      op1, op2);
    default:
        break;
    }

    ASSERT(!"bad expression... this should be checked in bindBinop...");
    return newError(tree);

}

bool FUNCBREC::isCastOrExpr(EXPR * search, EXPR * target)
{
AGAIN:
    if (search == target) return true;

    if (search && search->kind == EK_CAST) {
        search = search->asCAST()->p1;
        goto AGAIN;
    }
       
    return false;
}


EXPR * FUNCBREC::bindMultiOp (BASENODE * tree, EXPRKIND ek, EXPR * op1, EXPR * op2)
{
    checkLvalue(op1);

    // we have [op1 = right]
    // where
    // right is [wrap OP op2]

    EXPRWRAP * wrap = newExprWrap(op1, TK_SHORTLIVED)->asWRAP();
    EXPR * right = bindTypicalBinop(tree, ek, wrap, op2);

    EXPR * rval;
    EXPR ** pExpr;
    if (right->kind == EK_CONSTANT) { 
        // the binop was optimized away (for example, foo *= 0)
        rval = newExprBinop(tree, EK_ASSG, op1->type, op1, NULL);
        pExpr = &(rval->asBIN()->p2);
        
    } else {
    
        right->flags |= EXF_VALONSTACK;

        rval = newExpr(tree, EK_MULTI, op1->type);

    
        rval->asMULTI()->wrap = wrap;
        rval->asMULTI()->left = op1;
        pExpr = &(rval->asMULTI()->op);
        ek = right->kind;
    }
    rval->flags |= EXF_ASSGOP;

    // there is a subtle assumption at work here, namely that if 
    // the binop is optimized to an expression, then any casts involved
    // are not user defined.  

    switch (ek) {
    case EK_CALL:
        *pExpr = mustConvert(right, op1->type, right->tree);
        break;
    case EK_LSHIFT:
    case EK_RSHIFT:
        *pExpr = mustCast(right, op1->type, right->tree);
        break;
    default:
        *pExpr = tryConvert(right, op1->type, right->tree);
        if (!*pExpr) {
            *pExpr = mustCast(right, op1->type, tree);
            mustConvert(op2, op1->type, tree);
        }
    }

    return rval;

}

EXPR * FUNCBREC::bindSizeOf(UNOPNODE * tree)
{

    checkUnsafe(tree, NULL, ERR_SizeofUnsafe);

    TYPESYM * type = bindType(tree->p1->asTYPE());

    if (!type) return newError(tree);

	if (isManagedType(type)) {
        errorSym(tree->p1, ERR_ManagedAddr, type);
    }

    return bindSizeOf(tree, type);
}


EXPR * FUNCBREC::bindTypeOf(UNOPNODE * tree, BASENODE * op)
{

    TYPESYM * param;
    TYPESYM ** paramList;

    if (!op) op = tree->p1;

    TYPESYM * type = bindType(op->asTYPE());
    if (!type) goto LERROR;

    if (type == compiler()->symmgr.GetErrorSym()) goto LERROR;

    EXPRTYPEOF *rval;
    rval = newExpr(op, EK_TYPEOF, getPDT(PT_TYPE))->asTYPEOF();
    rval->asTYPEOF()->sourceType = type;
    PMETHSYM meth;
    param = getPDT(PT_TYPEHANDLE);
    paramList = compiler()->symmgr.AllocParams(1, &param);
    meth = findMethod(tree, PN_GETTYPEFROMHANDLE, getPDT(PT_TYPE)->asAGGSYM(), &paramList)->asMETHSYM();

    if (!meth) goto LERROR;

    rval->asTYPEOF()->method = meth;
    rval->flags |= EXF_CANTBENULL;

    return rval;

LERROR:
    return newError(tree);

}

EXPR * FUNCBREC::bindMakeRefAny(BASENODE * tree, EXPR * op)
{
    checkLvalue(op);

    if (op->kind == EK_FIELD)
        checkFieldRef(op->asFIELD(), true);

    EXPR * rval = newExprBinop(tree, EK_MAKERA, getPDT(PT_REFANY), op, NULL);

    return rval;
}

EXPR * FUNCBREC::bindRefType(BASENODE * tree, EXPR * op)
{
    op = mustConvert(op, getPDT(PT_REFANY), tree);

    EXPR *rval;

    rval = newExpr(tree, EK_TYPEOF, getPDT(PT_TYPE))->asTYPEOF();
    // this is a dummy, we will instead use the type of the refany to load...
    rval->asTYPEOF()->sourceType = getPDT(PT_OBJECT);
    
    TYPESYM * param = getPDT(PT_TYPEHANDLE);
    TYPESYM ** paramList = compiler()->symmgr.AllocParams(1, &param);
    METHSYM * meth = findMethod(tree, PN_GETTYPEFROMHANDLE, getPDT(PT_TYPE)->asAGGSYM(), &paramList)->asMETHSYM();

    if (!meth) return newError(tree);

    rval->asTYPEOF()->method = meth;

    rval = newExprBinop(tree, EK_TYPERA, getPDT(PT_TYPE), op, rval);

    return rval;
}


EXPR * FUNCBREC::bindCheckedExpr(UNOPNODE * tree, int bindFlags)
{
    CHECKEDCONTEXT checked(this, tree->Op() == OP_CHECKED);

    EXPR * rval = bindExpr(tree->p1, bindFlags);

    checked.restore(this);

    return rval;

}


// Bind a unary expression
EXPR * FUNCBREC::bindUnop(UNOPNODE * tree, int bindFlags)
{
    ASSERT(tree->kind == NK_UNOP);

    OPERATOR op = tree->Op();

    switch(op) {
    case OP_TYPEOF:
        return bindTypeOf(tree);
    case OP_SIZEOF:
        return bindSizeOf(tree);
    case OP_CHECKED:
    case OP_UNCHECKED:
        return bindCheckedExpr(tree, bindFlags);
    default:
        break;
    }

    // Bind the child node.
    EXPR * op1 = bindExpr(tree->p1);
    RETBAD(op1);

    switch (op)
    {
    case OP_PAREN:   return op1;  // Parenthesises expression is ignored.

    case OP_UPLUS:   return bindArithOp   (tree, EK_UPLUS,   0, op1, NULL, true, false);
    case OP_NEG:     return bindArithOp   (tree, EK_NEG,     checked.normal ? EXF_CHECKOVERFLOW : 0, op1, NULL, true, false);
    case OP_BITNOT:  return bindArithOp   (tree, EK_BITNOT,  0, op1, NULL, false, false);
    case OP_LOGNOT:  return bindBoolOp    (tree, EK_LOGNOT,  EXF_COMPARE , op1, NULL);
    case OP_POSTDEC:
    case OP_PREDEC:
    case OP_POSTINC:
    case OP_PREINC:  return bindIncOp     (tree, op1, op);
    case OP_MAKEREFANY: return bindMakeRefAny(tree, op1);
    case OP_REFTYPE: return bindRefType(tree, op1);
    case OP_INDIR:   return bindPtrIndirection(tree, op1);
    case OP_ADDR:    return bindPtrAddr(tree, op1, bindFlags);

    default:
        errorStr(tree, ERR_UnimplementedOp, opName(op));
        return newError(tree);
    }
}

bool FUNCBREC::isManagedType(TYPESYM * type)
{
    compiler()->symmgr.DeclareType(type);
    
    if (type == compiler()->symmgr.GetVoid()) {
        return false;
    }

    switch (type->fundType()) {
    case FT_NONE:
    case FT_REF: return true;
    case FT_STRUCT:
        for (SYM * ps = type->firstChild; ps; ps = ps->nextChild) {
            if (ps->kind == SK_MEMBVARSYM && !ps->asMEMBVARSYM()->isStatic) {
                if (isManagedType(ps->asMEMBVARSYM()->type)) return true;
            }
        }
    default:
        return false;
    }
}

// prop returns, array indexes and refanys are not addressable
// This means that you can say &X in either a fixed statement or outside of it.
// you can never say &(foo[4]), not even in a fixed statement...
// [Note, however, that you can say &(foo[3].la) in a fixed statement...
bool FUNCBREC::isAddressable(EXPR * expr)
{

    switch (expr->kind) {
    case EK_PROP:
    case EK_VALUERA:
        return false;
    case EK_ARRINDEX:
    case EK_FIELD:
    case EK_LOCAL:
    case EK_INDIR:
        return true;
    default:
        ASSERT(!"bad lvalue expr");
        return false;
    }

}

bool FUNCBREC::isFixedExpression(EXPR * expr)
{

AGAIN:
    EXPR  * object;
    LOCVARSYM * local;

    switch (expr->kind) {
    case EK_FIELD:
        if (expr->asFIELD()->field->isStatic) return false;
        object = expr->asFIELD()->object;
        if (!object) return false;
        if (!(object->flags & EXF_LVALUE)) return false;
        compiler()->symmgr.DeclareType(object->type);
        if (object->type->fundType() == FT_REF) return false;
        expr = object;
        goto AGAIN;
    case EK_LOCAL:
        local = expr->asLOCAL()->local;
        if (local->slot.isParam) {
            return !local->slot.isRefParam;
        } 
        return local != thisPointer;
    case EK_LOCALLOC:
    case EK_INDIR:
        return true;
    default:
        return false;
    }
}


EXPR * FUNCBREC::bindLocAlloc(NEWNODE * tree, TYPESYM * type)
{
    checkUnsafe(tree);

    // the type is a pointer type, we need the underlying type...
    type = type->parent->asTYPESYM();

    if (pCatchScope != pOuterScope || pFinallyScope != pOuterScope) {
        errorN(tree, ERR_StackallocInCatchFinally);
    }

    EXPR * arg = bindExpr(tree->pArgs);

    arg = mustConvert(arg, getPDT(PT_INT), tree->pArgs);

    checkNegativeConstant(tree->pArgs, arg, ERR_NegativeStackAllocSize);

    arg = newExprBinop(tree, EK_MUL, getPDT(PT_INT), bindSizeOf(tree, type), arg);

    EXPR * rval = newExprBinop(tree, EK_LOCALLOC, compiler()->symmgr.GetPtrType(type), arg, NULL);

    rval->flags |= EXF_ASSGOP;

    return rval;

}



EXPR * FUNCBREC::bindPtrAddr(UNOPNODE * tree, EXPR * op, int bindFlags)
{
    // Can only take the address of lvalues...
    // but don't call checkLvalue(op, false);
    // because it gives an ugly error message
    // but still mark fields as assigned
    checkUnsafe(tree);

    if (op->kind == EK_FIELD) {
        checkFieldRef(op->asFIELD(), true);
        markFieldAssigned(op);
    }

    if (isManagedType(op->type)) {
        errorSym(tree, ERR_ManagedAddr, op->type);
    }

    if ((op->isOK() && (op->flags & EXF_LVALUE) == 0) || !isAddressable(op)) {
        errorN(tree, ERR_InvalidAddrOp);
    }

    bool needsFixing = !isFixedExpression(op);

    if (needsFixing != (bool) !!(bindFlags & BIND_FIXEDVALUE)) {
        errorN(tree, !needsFixing ? ERR_FixedNotNeeded : ERR_FixedNeeded);
    }

    {
        AGGSYM * temp = op->type->underlyingAggregate();
        if (temp && temp->isStruct)
            temp->hasExternReference = true;
    }

    EXPR * rval = newExprBinop(tree, EK_ADDR, compiler()->symmgr.GetPtrType(op->type), op, NULL);

    return rval;
}

EXPR * FUNCBREC::bindPtrIndirection(UNOPNODE * tree, EXPR * op)
{
    if (op->type->kind != SK_PTRSYM) {
        errorN(tree, ERR_PtrExpected);
        return newError(tree);
    }
    if (op->type == getVoidPtrType()) {
        errorN(tree, ERR_VoidError);
        return newError(tree);
    }
    EXPR * rval = newExprBinop(tree, EK_INDIR, op->type->asPTRSYM()->parent->asTYPESYM(), op, NULL);
    rval->flags |= EXF_ASSGOP | EXF_LVALUE;
    return rval;
}


EXPR * FUNCBREC::bindIncOp(BASENODE * tree, EXPR * op1, OPERATOR op)
{
    checkLvalue(op1);

    bool isFloat = false;

    EXPRWRAP * wrap = newExprWrap(op1, TK_SHORTLIVED)->asWRAP();

    EXPRMULTI * rval = newExpr(tree, EK_MULTI, op1->type)->asMULTI();

    TYPESYM * addType = op1->type;

    rval->flags |= EXF_ASSGOP;
    rval->wrap = wrap;
    rval->left = op1;

    EXPRKIND ek = (EXPRKIND) -1;

    switch(op) {
    case OP_POSTINC:
        rval->flags |= EXF_ISPOSTOP;
    case OP_PREINC:
        ek = EK_ADD;
        break;
    case OP_POSTDEC:
        rval->flags |= EXF_ISPOSTOP;
    case OP_PREDEC:
        ek = EK_SUB;
        break;
    default:
        ASSERT(!"bad op.  should have been checked before");
    }

    CONSTVAL cv;
    EXPR * right;
    EXPR * one;

    switch(op1->type->fundType()) {
    case FT_U2:
    case FT_NONE:
    case FT_I1:
    case FT_U1:
    case FT_I2:
    case FT_I4: 
        cv.iVal = 1;
        addType = getPDT(PT_INT);
        break;
    case FT_U4: 
        cv.iVal = 1;
        addType = getPDT(PT_UINT);
        break;
    case FT_I8:
    case FT_U8:
    case FT_R4:
    case FT_R8: {
        cv.doubleVal = (double*) allocator->Alloc(sizeof(double));
        if (op1->type->fundType() <= FT_LASTINTEGRAL) {
            *(cv.longVal) = (__int64) 1;
        } else {
            isFloat = true;
            *(cv.doubleVal) = 1.0;
        }
                }
        break;
    case FT_PTR:
        if (op1->type == getVoidPtrType()) {
            errorN(tree, ERR_VoidError);
            return newError(tree);
        }
        one = bindSizeOf(tree, op1->type->parent->asTYPESYM());
        goto GOTONE;

    case FT_REF:
    case FT_STRUCT:
    default:

        right = bindUDUnop(tree, (EXPRKIND) (ek - EK_ADD + EK_INC), wrap);
        if (right) {

            right->asCALL()->flags |= EXF_VALONSTACK;
            if (right->type->kind != SK_ERRORSYM && right->type != op1->type) {
                right = mustConvert(right, op1->type, tree);
            }

            rval->op = right;
            return rval;
        } else {
            const TOKINFO * tok = CParser::GetTokenInfo(ek == EK_ADD ? TID_PLUSPLUS : TID_MINUSMINUS);
            errorStrSym(tree, ERR_NoSuchOperator, tok->pszText, wrap->type);
            return newError(tree);
        }
    }

    one = newExprConstant(tree, op1->type, cv);


GOTONE:
    right = newExprBinop(NULL, ek, addType, wrap, one);
    if (checked.normal && !isFloat) {
        right->flags |= EXF_CHECKOVERFLOW;
    }

    rval->op = mustCast(right, op1->type, tree);
    return rval;

}

// bind a list of variable declarations  newLast is set if
// the result consists of multuiple EXPR statatments
EXPR * FUNCBREC::bindVarDecls(DECLSTMTNODE * tree, EXPR *** newLast)
{
    TYPESYM * type = bindType(tree->pType);
    if (!type) goto LERROR;

    if (tree->flags & NF_FIXED_DECL && type->kind != SK_PTRSYM) {
        errorN(tree, ERR_BadFixedInitType);
    }

    EXPR * rval;
    EXPR ** prval;
    rval = NULL;
    prval = &rval;

    NODELOOP(tree->pVars , VARDECL, decl)
        newList(bindVarDecl(decl, type, tree->flags & (NF_CONST_DECL | NF_FIXED_DECL | NF_USING_DECL)), &prval);
    ENDLOOP;

    if (prval != &rval) *newLast = prval;

    if (rval) {
        // For debug info associate the NK_DECLSTMT with the first variable declaration
        if (rval->kind == EK_DECL)
            rval->tree = tree;
        else if (rval->kind != EK_ERROR) {
            ASSERT(rval->kind == EK_LIST);
            if (rval->asBIN()->p1->kind == EK_DECL)
                rval->asBIN()->p1->tree = tree;
            else
                ASSERT(rval->asBIN()->p1->kind == EK_ERROR);
        }
    }   

    return rval;

LERROR:
    return newError(tree);
}


// Bind a single variable declaration
EXPR * FUNCBREC::bindVarDecl(VARDECLNODE * tree, TYPESYM * type, unsigned flags)
{

    bool isConst = !!(flags & NF_CONST_DECL);
    bool isFixed = !!(flags & NF_FIXED_DECL);
    bool isUsing = !!(flags & NF_USING_DECL);

    ASSERT(!(isConst && isFixed));
    ASSERT(!(isConst && isUsing));
    ASSERT(!(isUsing && isFixed));

    LOCVARSYM * sym = declareVar(tree, tree->pName->pName, type);
    if (!sym) goto LERROR;

    if (isFixed) {
        sym->slot.mustBePinned = true;
        sym->isNonWriteable = true;
    } else if (isUsing) {
        sym->isNonWriteable = true;
    }

    sym->declTree = tree;
    unreferencedVarCount++;

    EXPR * init;
    init = bindPossibleArrayInitAssg(tree->pArg->asBINOP(), sym->type, isFixed ? BIND_FIXEDVALUE : (isUsing ? BIND_USINGVALUE : 0));
    RETBAD(init);

    if (isFixed || isUsing) {
        sym->slot.hasInit = true;
        if (!init) {
            errorN(tree, ERR_FixedMustInit);
        }
    } else if (isConst) {
        if (init->kind == EK_ASSG) {
            init = init->asBIN()->p2;
        }
        if (init->kind != EK_CONSTANT) {
            if (init->kind != EK_ERROR) {  // error reported by compile already...
                errorSym(tree->pArg, ERR_NotConstantExpression, sym);
            }
        } else {
            sym->isConst = true;
            sym->constVal = init->asCONSTANT()->getSVal();
        }
        init = 0;
    } else if (init && !pSwitchScope && (unrealizedGotoCount == 0)) {
        sym->slot.hasInit = true;
    }

    EXPRDECL * rval;
    rval = newExpr(tree, EK_DECL, getVoidType())->asDECL();

    rval->sym = sym;
    rval->init = init;

    return rval;

LERROR:
    return newError(tree);
}


/*
 * Create a cast node with the given expression flags. Always returns true. If pexprDest is NULL, 
 * just return true.
 */
bool FUNCBREC::bindSimpleCast(BASENODE * tree, EXPR * exprSrc, TYPESYM * typeDest, EXPR ** pexprDest, unsigned exprFlags)
{

    if (!pexprDest)
        return true;

    // If the source is a constant, and cast is really simple (no change in fundemental
    // type, no flags), the create a new constant node with the new type instead of
    // creating a cast node. This allows compile-time constants to be easily recognized.
    if (exprSrc->kind == EK_CONSTANT && exprFlags == 0 && 
        exprSrc->type->fundType() == typeDest->fundType() &&
        (!exprSrc->type->isPredefType(PT_STRING) || exprSrc->asCONSTANT()->getSVal().strVal == NULL))
    {
        EXPRCONSTANT * expr = newExprConstant(tree, typeDest, exprSrc->asCONSTANT()->getSVal());
        *pexprDest = expr;
        return true;
    }

    // Just alloc a new node and fill it in.
    EXPRCAST * expr;
    expr = newExpr(tree, EK_CAST, typeDest)->asCAST();
    expr->p1 = exprSrc;
    expr->flags = exprFlags;
    if (checked.normal) {
        expr->flags |= EXF_CHECKOVERFLOW;
    }
    *pexprDest = expr;
    return true;
}


/*
 * Check to see if an integral constant is within range of a integral destination type.
 */
bool FUNCBREC::isConstantInRange(EXPRCONSTANT * exprSrc, TYPESYM * typeDest)
{
    FUNDTYPE ftSrc = exprSrc->type->fundType();
    FUNDTYPE ftDest = typeDest->fundType();

    if (ftSrc > FT_LASTINTEGRAL || ftDest > FT_LASTINTEGRAL) {
        return false;
    }

    __int64 value = exprSrc->asCONSTANT()->getI64Value();

    // U8 src is unsigned, so deal with values > MAX_LONG here.
    if (ftSrc == FT_U8) {
        if (ftDest == FT_U8)
            return true;
        if (value < 0)  // actually > MAX_LONG.
            return false;  
    }

    switch (ftDest) {
    case FT_I1: if (value >= -128 && value <= 127) return true;  break;
    case FT_I2: if (value >= -0x8000 && value <= 0x7fff) return true;  break;
    case FT_I4: if (value >= I64(-0x80000000) && value <= I64(0x7fffffff)) return true;  break;
    case FT_I8: return true; 
    case FT_U1: if (value >= 0 && value <= 0xff) return true;  break;
    case FT_U2: if (value >= 0 && value <= 0xffff) return true;  break;
    case FT_U4: if (value >= 0 && value <= I64(0xffffffff)) return true;  break;
    case FT_U8: if (value >= 0) return true;    break;
    default:
        break;
    }

    return false;
}

#if defined(_MSC_VER)
#pragma optimize("p", on)  // consistent floating point.
#endif // defined(_MSC_VER)

/*
 * Fold a constant cast. Returns true if the constant could be folded.
 */
bool FUNCBREC::bindConstantCast(BASENODE * tree, EXPR * exprSrc, TYPESYM * typeDest, EXPR ** pexprDest, bool * checkFailure)
{

    __int64 valueInt = 0;
    double valueFlt = 0;
    FUNDTYPE ftSrc = exprSrc->type->fundType();
    FUNDTYPE ftDest = typeDest->fundType();
    bool srcIntegral = (ftSrc <= FT_LASTINTEGRAL);
    bool destIntegral = (ftDest <= FT_LASTINTEGRAL);

    ASSERT(exprSrc->kind == EK_CONSTANT);
    EXPRCONSTANT * constSrc = exprSrc->asCONSTANT();

    bool explicitConversion = *checkFailure;

    *checkFailure = false;

    if (ftSrc == FT_STRUCT || ftDest == FT_STRUCT)
    {
        // Do constant folding involving decimal constants.
        EXPR * expr = bindDecimalConstCast(tree, typeDest, exprSrc->type, constSrc);

        if (expr) {
            if (pexprDest)
                *pexprDest = expr;
            return true;
        } else {
            return false;
        }
    }

    if (explicitConversion && checked.constant && srcIntegral && destIntegral && !isConstantInRange(exprSrc->asCONSTANT(), typeDest)) {
        *checkFailure = true;
        return false;
    }

    if (!pexprDest)
        return true;

    // Get the source constant value into valueInt or valueFlt.
    if (srcIntegral)
        valueInt = constSrc->getI64Value(); 
    else
        valueFlt = * constSrc->getVal().doubleVal;

    // Convert constant to the destination type, truncating if necessary.
    // valueInt or valueFlt contains the result of the conversion.
    switch (ftDest) {
    case FT_I1:
        if (!srcIntegral)  valueInt = (__int64) valueFlt;
        valueInt = (signed char) (valueInt & 0xFF); 
        break;
    case FT_I2: 
        if (!srcIntegral)  valueInt = (__int64) valueFlt;
        valueInt = (signed short) (valueInt & 0xFFFF); 
        break;
    case FT_I4: 
        if (!srcIntegral)  valueInt = (__int64) valueFlt;
        valueInt = (signed int) (valueInt & 0xFFFFFFFF); 
        break;
    case FT_I8: 
        if (!srcIntegral)  valueInt = (__int64) valueFlt;
        break;
    case FT_U1: 
        if (!srcIntegral)  valueInt = (__int64) valueFlt;
        valueInt = (unsigned char) (valueInt & 0xFF); 
        break;
    case FT_U2: 
        if (!srcIntegral)  valueInt = (__int64) valueFlt;
        valueInt = (unsigned short) (valueInt & 0xFFFF); 
        break;
    case FT_U4:
        if (!srcIntegral)  valueInt = (__int64) valueFlt;
        valueInt = (unsigned int) (valueInt & 0xFFFFFFFF); 
        break;
    case FT_U8: 
        if (!srcIntegral)  valueInt = (unsigned __int64) valueFlt;
        break;
    case FT_R4:
    case FT_R8:
        if (srcIntegral) {
            if (ftSrc == FT_U8)
                valueFlt = (double) (unsigned __int64) valueInt;
            else
                valueFlt = (double) valueInt;
        }

        if (ftDest == FT_R4) {
            // Force to R4 precision/range. 
            float f;
            RoundToFloat(valueFlt, &f);
            valueFlt = f;
        }

        break;
    default:
        ASSERT(0); break;
    }

    // Create a new constant with the value in "valueInt" or "valueFlt".
    if (pexprDest) {
        CONSTVAL cv;
        if (ftDest == FT_U4) 
            cv.uiVal = (unsigned int) valueInt;
        else if (ftDest <= FT_LASTNONLONG)
            cv.iVal = (int) valueInt;
        else if (ftDest <= FT_LASTINTEGRAL) {
            cv.longVal = (__int64 *) allocator->Alloc(sizeof(__int64));
            * cv.longVal = valueInt;
        }
        else {
            cv.doubleVal = (double *) allocator->Alloc(sizeof(double));
            * cv.doubleVal = valueFlt;
        }
        EXPRCONSTANT * expr = newExprConstant(tree, typeDest, cv);

        *pexprDest = expr;
    }

    return true;
}

#if defined(_MSC_VER)
#pragma optimize("", on)  // restore default optimizations
#endif // defined(_MSC_VER)

void FUNCBREC::considerConversion(METHSYM * conversion, TYPESYM * desired, bool from, bool impl, bool * newPossibleWinner, bool * oldMethodsValid, SYMLIST ** newList, SYMLIST ** prevList, bool foundImplBefore)
{
    TYPESYM * prevType = *prevList ? prevList[0]->sym->asTYPESYM() : NULL;
    TYPESYM * actualType = from ? conversion->params[0] : conversion->retType;

    if (!prevType) {
USENEW:
        *prevList = *newList;
        *newList = NULL;
USETHIS:
        prevList[0]->next = NULL;
        prevList[0]->sym = actualType;
        if (actualType != prevType) {
            *oldMethodsValid = false;
        }
        return;
    } else if (prevType == desired && actualType != prevType) {
        *newPossibleWinner = false;
        return;
    }

    if ((impl && !foundImplBefore) || desired == actualType) {
        if (*prevList) goto USETHIS;
        goto USENEW;
    }

    SYMLIST * prev = NULL;
    SYMLIST * empty = *newList;
    for(SYMLIST * list = *prevList; list;) {

        if (actualType == list->sym) {
            *newPossibleWinner = false;
            return;
        }

        TYPESYM * type1, * type2;

        if (from ^ impl) {
            // want encompassing, which is one to which everyone else converts
            type1 = actualType;
            type2 = prevType;
        } else {
            // want encompassed, which is one which converts to everyone else
            type1 = prevType;
            type2 = actualType;
        }

        if (canConvert(type2, type1, NULL, CONVERT_STANDARD)) {
            // conversion is better than previous...
            // previous should be deleted...
            *oldMethodsValid = false;
            empty = list;
            if (!prev) {
                list = list->next;
                *prevList = list;
            } else {
                prev->next = list->next;
                list = list->next;
            }
        } else {
            if (canConvert(type1, type2, NULL, CONVERT_STANDARD)) {
                // ok, conversion is worse than previous...
                // no need to consider it further...
                *newPossibleWinner = false;
                return;
            } else {
                // conversion is no better, nor worse than previous,
                // so neither can be the winner
                *oldMethodsValid = false;
            }
            prev = list;
            list = list->next;
        }
    } 
    empty->sym = actualType;
    empty->next = *prevList;
    if (empty->next) {
        *newPossibleWinner = false;
    }
    *prevList = empty;

    if (empty == *newList) {
        *newList = NULL;
    }
    
}





/* 
 * Binds a user-defined conversion. The parameters to this procedure are
 * the same as bindImplicitConversion, except the last.
 *    implicitOnly - only consider implicit conversions.
 *
 * This is a helper routine for bindImplicitConversion and bindExplicitConversion.
 */
bool FUNCBREC::bindUserDefinedConversion(BASENODE * tree, EXPR * exprSrc, TYPESYM * typeSrc, TYPESYM * typeDest, EXPR * * pexprDest, bool implicitOnly)
{
    if (!typeSrc || !typeDest) return false;

    SYMLIST * listSrc = (typeSrc->kind == SK_AGGSYM) ? compiler()->clsDeclRec.getConversionList(typeSrc->asAGGSYM()) : NULL;
    SYMLIST * listDest = (typeDest->kind == SK_AGGSYM) ? compiler()->clsDeclRec.getConversionList(typeDest->asAGGSYM()) : NULL;

    // optimization...
    if (!listSrc && !listDest) return false;

    SYMLIST * prevFrom, * prevTo, * newList;

    METHSYM * best = NULL;
    bool ambig = false;

    prevTo = prevFrom = newList = NULL;

    bool prevImplFrom = false;
    bool prevImplTo = false;

    for (;;listSrc = listSrc->next) {
        if (!listSrc) {
            if (listDest) {
                listSrc = listDest;
                listDest = NULL;
            } else {
                break;
            }
        }
        
        METHSYM * conversion = listSrc->sym->asMETHSYM();
        ASSERT(conversion->cParams == 1);

        if (implicitOnly && !conversion->isImplicit) continue;

        TYPESYM * fromType = conversion->params[0];
        TYPESYM * toType = conversion->retType;

        bool implFrom = false;
        bool implTo = false;
        if (exprSrc) {
            implFrom = canConvert(exprSrc, fromType, NULL, CONVERT_STANDARD);
        } else {
            implFrom = canConvert(typeSrc, fromType, NULL, CONVERT_STANDARD);
        }
        if (!implFrom) {
            if (implicitOnly || prevImplFrom) continue;
            if (exprSrc) {
                if (!canCast(exprSrc, fromType, NULL, CONVERT_STANDARD)) continue;
            } else {
                if (!canCast(typeSrc, fromType, NULL, CONVERT_STANDARD)) continue;
            }
        }

        implTo = canConvert(toType, typeDest, NULL, CONVERT_STANDARD);
        if (!implTo) {
            if (implicitOnly || prevImplTo) continue;
            if (!canCast(toType, typeDest, NULL, CONVERT_STANDARD)) continue;
        }

    
        bool oldMethodsValid = true;
        bool newPossibleWinner = true;

        if (!newList) newList = (SYMLIST*) _alloca(sizeof(SYMLIST));

        considerConversion(conversion, typeSrc, true, implFrom, &newPossibleWinner, &oldMethodsValid, &newList, &prevFrom, prevImplFrom);

        prevImplFrom |= implFrom;

        if (!newList) newList = (SYMLIST*) _alloca(sizeof(SYMLIST));

        considerConversion(conversion, typeDest, false, implTo, &newPossibleWinner, &oldMethodsValid, &newList, &prevTo, prevImplTo);

        prevImplTo |= implTo;

        if (newPossibleWinner) {
            if (oldMethodsValid) {
                if (best) {
                    ASSERT(conversion->retType == best->retType);
                    ASSERT(conversion->params[0] == best->params[0]);
                    best = NULL;
                    ambig = true;
                } else {
                    best = conversion;
                }
            } else {
                best = conversion;
            }
        } else {
            if (!oldMethodsValid) {
                best = NULL;
            }
        }

    }

    // optimization...
    if (!best) return false;

    if (best->isDeprecated && tree) {
        reportDeprecated(tree, best);
    }

    if (checkBogus(best)) {
        if (tree) {
            compiler()->clsDeclRec.errorNameRefSymbol(best->name, tree, pParent, best, ERR_BindToBogus);
        }
        return false;
    }

    if (pexprDest) {
//        if (tree) {
//            checkUnsafe(tree, best->retType);
//            checkUnsafe(tree, best->params[0]);
//        }
        EXPR * fromConv = tryCast(exprSrc, best->params[0], tree, CONVERT_NOUDC);
        ASSERT(fromConv);
        EXPRCALL * call = newExpr(tree, EK_CALL, best->retType)->asCALL();
        call->flags |= EXF_BOUNDCALL;
        call->args = fromConv;
        call->object = NULL;
        call->method = best;
        EXPR * toConv = tryCast(call, typeDest, tree, CONVERT_NOUDC);
        ASSERT(toConv);
        *pexprDest = toConv;
    }

    return true;
}



/*
 * bindImplicitConversion
 *
 * This is a complex routine with complex parameter. Generally, this should
 * be called through one of the helper methods that insulates you
 * from the complexity of the interface. This routine handles all the logic
 * associated with implicit conversions.
 *
 * exprSrc - the expression being converted. Can be NULL if only type conversion
 *           info is being supplied.
 * typeSrc - type of the source
 * typeDest - type of the destination
 * exprDest - returns an expression of the src converted to the dest. If NULL, we
 *            only care about whether the conversion can be attempted, not the
 *            expression tree.
 * flags    - flags possibly customizing the conversions allowed. E.g., can suppress
 *            user-defined conversions.
 *
 * returns true if the conversion can be made, false if not.
 */
bool FUNCBREC::bindImplicitConversion(BASENODE * tree, EXPR * exprSrc, TYPESYM * typeSrc, TYPESYM * typeDest, EXPR * * pexprDest, unsigned flags)
{

    if (!typeSrc || !typeDest || typeSrc->kind == SK_ERRORSYM || typeDest->kind == SK_ERRORSYM) return false;

    ASSERT(typeSrc && typeDest);                            // types must be supplied.
    ASSERT(exprSrc == NULL || typeSrc == exprSrc->type);    // type of source should be correct if source supplied
    ASSERT(pexprDest == NULL || exprSrc != NULL);           // need source expr to create dest expr

    // Make sure both types are declared.
    compiler()->symmgr.DeclareType(typeSrc);
    compiler()->symmgr.DeclareType(typeDest);

    // Does the trivial conversion exist?
    if (typeSrc == typeDest) {
        if (!(flags & CONVERT_ISEXPLICIT) || typeSrc->kind != SK_AGGSYM || (!typeSrc->isPredefType(PT_FLOAT) && !typeSrc->isPredefType(PT_DOUBLE))) {
            if (pexprDest)
                *pexprDest = exprSrc;
            return true;
        }
    }

    // Can't convert to the error type.
    if (typeDest->kind == SK_ERRORSYM)
        return false;


    // Get the fundemental types of destination.
    FUNDTYPE ftDest = typeDest->fundType();
    ASSERT(ftDest != FT_NONE || typeDest->kind == SK_PARAMMODSYM || typeDest->kind == SK_VOIDSYM);
    if (typeDest->kind == SK_NULLSYM)
        return false;  // Can never convert TO the null type.

    switch (typeSrc->kind) {
    default:
        ASSERT(0);
        break;

    case SK_VOIDSYM:
    case SK_ERRORSYM:
    case SK_PARAMMODSYM:
        return false;

    case SK_NULLSYM: 
        /*
         * Handle null type conversions.
         * null type can be implicitly converted to any reference type or pointer type.
         */
        if (exprSrc->kind != EK_CONSTANT)
            return false;
        else {
            if (ftDest == FT_REF || ftDest == FT_PTR) {
                return bindSimpleCast(tree, exprSrc, typeDest, pexprDest);
            }
        }

        break;

    case SK_ARRAYSYM:
        /*
         * Handle array conversions.
         * An array type can be implicitly converted to some other array types, Object,
         * or Array or some base class/interface that Array handles.
         */
        if (typeDest->kind == SK_ARRAYSYM) {
            // Handle array -> array conversions. Ranks must match (or cast to unknown rank),
            // and element type must be same or built-in reference conversion.
            PARRAYSYM arraySrc = typeSrc->asARRAYSYM();
            PARRAYSYM arrayDest = typeDest->asARRAYSYM();
            PTYPESYM elementSrc = arraySrc->elementType();
            PTYPESYM elementDest = arrayDest->elementType();

            if (arraySrc->rank != arrayDest->rank)
                break;  // Ranks do not match.

            if (elementSrc == elementDest ||
                (elementSrc->fundType() == FT_REF && elementDest->fundType() == FT_REF &&
                 canConvert(elementSrc, elementDest, tree, CONVERT_NOUDC)))
            {
                // Element types are compatible.
                return bindSimpleCast(tree, exprSrc, typeDest, pexprDest);
            }

            break;
        }
        else if (canConvert(getPDT(PT_ARRAY), typeDest, tree, CONVERT_NOUDC)) {
            // The above if checks for dest==Array, object or an interface Array implements.
            return bindSimpleCast(tree, exprSrc, typeDest, pexprDest);
        }

        break;

    case SK_PTRSYM:
        /* 
         * Handle pointer conversions.
         *
         * A pointer can be implicitly converted to void *. That's it.
         */
        if (typeDest->kind == SK_PTRSYM && typeDest->asPTRSYM()->baseType() == getVoidType())
            return bindSimpleCast(tree, exprSrc, typeDest, pexprDest);

        break;

    case SK_AGGSYM: {
        AGGSYM * aggSrc = typeSrc->asAGGSYM();

        if (aggSrc->isEnum) {
            /* 
             * Handle enum conversions.
             * An enum can convert to Object, Value, or Enum.
             */
            if (typeDest->kind == SK_AGGSYM && typeDest->asAGGSYM()->isPredefined &&
                (typeDest->isPredefType(PT_OBJECT) || typeDest->isPredefType(PT_VALUE) || typeDest->isPredefType(PT_ENUM)))
                return bindSimpleCast(tree, exprSrc, typeDest, pexprDest, EXF_BOX | EXF_CANTBENULL);

            break;
        }

        // TypeReference and ArgIterator can't be boxed (or converted to anything else)
        if (typeSrc->isSpecialByRefType())
            return false;

        if (typeDest->kind == SK_AGGSYM && typeDest->asAGGSYM()->isEnum) {
            // No implicit conversion TO enum except the identity conversion
            // or literal '0'.  
            // NB: this is NOT a standard conversion
            if (aggSrc->iPredef != PT_BOOL && exprSrc && exprSrc->kind == EK_CONSTANT && exprSrc->isZero() && (exprSrc->flags & EXF_LITERALCONST) && !(flags & CONVERT_STANDARD))
                return bindSimpleCast(tree, exprSrc, typeDest, pexprDest, 0);
            break;
        }

        if (aggSrc->isSimpleType() && typeDest->isSimpleType()) {
            /*
             * Handle conversions between simple types. (int->long, float->double, etc...)
             */

            ASSERT(aggSrc->isPredefined && typeDest->asAGGSYM()->isPredefined);
            int ptSrc = aggSrc->iPredef;
            int ptDest = typeDest->asAGGSYM()->iPredef;
            unsigned convertKind;

            ASSERT(ptSrc < NUM_SIMPLE_TYPES && ptDest < NUM_SIMPLE_TYPES);

            if (exprSrc && exprSrc->kind == EK_CONSTANT && 
                ((ptSrc == PT_INT && ptDest != PT_BOOL && ptDest != PT_CHAR) ||
                 (ptSrc == PT_LONG && ptDest == PT_ULONG)) &&
                isConstantInRange(exprSrc->asCONSTANT(), typeDest))
            {
                // Special case (CLR 6.1.6): if integral constant is in range, the conversion is a legal implicit conversion.
                convertKind = CONV_KIND_IMP;
            } else if (ptSrc == ptDest) {

                // special case: precision limiting casts to float or double

                ASSERT(ptSrc == PT_FLOAT || ptSrc == PT_DOUBLE);
                ASSERT(flags & CONVERT_ISEXPLICIT);

                convertKind = CONV_KIND_IMP;

            } else {

                convertKind = simpleTypeConversions[ptSrc][ptDest];
                ASSERT((convertKind & CONV_KIND_MASK) != CONV_KIND_ID);  // identity conversion should have been handled at first.
            }

            if ((convertKind & CONV_KIND_MASK) == CONV_KIND_IMP) {
                // An implicit conversion exists. Do the conversion.

                if (exprSrc && exprSrc->kind == EK_CONSTANT) {
                    // Fold the constant cast if possible. 
                    bool checkFailure = false;
                    if (bindConstantCast(tree, exprSrc, typeDest, pexprDest, &checkFailure)) {
                        ASSERT(!checkFailure);
                        return true;  // else, don't fold and use a regular cast, below.
                    }
                    ASSERT(!checkFailure); // since this is an implicit cast, it can never fail the check...
                }

                if (convertKind & CONV_KIND_USEUDC) {
                    if (!pexprDest) return true;
                    // According the language, this is a standard conversion, but it is implemented
                    // through a user-defined conversion. Because it's a standard conversion, we don't
                    // test the CONVERT_NOUDC flag here.
                    return bindUserDefinedConversion(tree, exprSrc, typeSrc, typeDest, pexprDest, true);
                }
                else {
                    return bindSimpleCast(tree, exprSrc, typeDest, pexprDest);
                }
            }

            // No break here, continue testing for derived to base conversions below.
        }

        /*
         * Handle struct, class and interface conversions. Delegates are handled just
         * like classes here. A class, struct, or interface is implicitly convertable
         * to a base class or interface. (Note the test against object is needed below
         * because object is NOT a base type of interfaces.)
         */
        if (typeDest->kind == SK_AGGSYM &&
            (typeDest->isPredefType(PT_OBJECT) || compiler()->symmgr.IsBaseType(aggSrc, typeDest->asAGGSYM())))
        {
            if (aggSrc->isStruct && ftDest == FT_REF)
                return bindSimpleCast(tree, exprSrc, typeDest, pexprDest, EXF_BOX | EXF_CANTBENULL);
            else
                return bindSimpleCast(tree, exprSrc, typeDest, pexprDest, exprSrc ? (exprSrc->flags & EXF_CANTBENULL) : 0);
        }

        break;
    }}

    // No built-in conversion was found. Maybe a user-defined conversion?
    if (! (flags & CONVERT_NOUDC)) {
        return bindUserDefinedConversion(tree, exprSrc, typeSrc, typeDest, pexprDest, true);
    }

    // No conversion was found.
    return false;
}


// returns true if an implicit conversion exists from source type to dest type. flags is an optional parameter.
bool FUNCBREC::canConvert(TYPESYM * src, TYPESYM * dest, BASENODE * tree, unsigned flags)
{
    return bindImplicitConversion(tree, NULL, src, dest, NULL, flags);
}

// returns true if a implicit conversion exists from source expr to dest type. flags is an optional parameter.
bool FUNCBREC::canConvert(EXPR * expr, TYPESYM * dest, BASENODE *tree, unsigned flags)
{
    return bindImplicitConversion(tree, expr, expr->type, dest, NULL, flags);
}

// performs an implicit conversion if its possible. otherwise displays an error. flags is an optional parameter.
EXPR * FUNCBREC::mustConvert(EXPR * expr, TYPESYM * dest, BASENODE * tree, unsigned flags)
{
    EXPR * exprResult;

    if (bindImplicitConversion(tree, expr, expr->type, dest, &exprResult, flags)) {
        // Conversion works.
        return exprResult;
    }
    else {
        if (expr->isOK() && dest->kind != SK_ERRORSYM) {
            // don't report cascading error.

            // For certain situations, try to give a better error.

            FUNDTYPE ftSrc = expr->type->fundType();
            FUNDTYPE ftDest = dest->fundType();

            if (expr->kind == EK_CONSTANT &&
                expr->type->isSimpleType() && dest->isSimpleType())
            {
                if ((ftSrc == FT_I4 && (ftDest <= FT_LASTNONLONG || ftDest == FT_U8)) ||
                    (ftSrc == FT_I8 && ftDest == FT_U8))
                {
                    // Failed because value was out of range. Report nifty error message.
                    WCHAR value[30];
                    swprintf(value, L"%I64d", expr->asCONSTANT()->getI64Value());

                    errorStrSym(tree, ERR_ConstOutOfRange, value, dest);
                    return newError(tree);
                }
                else if (ftSrc == FT_R8 && (expr->flags & EXF_LITERALCONST) &&
                         (dest->isPredefType(PT_FLOAT) || dest->isPredefType(PT_DECIMAL)))
                {
                    // Tried to assign a literal of type decimal to a float or decimal. Suggest use
                    // of a 'F' or 'M' suffix.
                    errorStrSym(tree, ERR_LiteralDoubleCast, dest->isPredefType(PT_DECIMAL) ? L"M" : L"F", dest);
                    return newError(tree);
                }
            }

            if (expr->type->kind == SK_NULLSYM && dest->fundType() != FT_REF) {
                errorSym(tree, ERR_ValueCantBeNull, dest);
            }
            else {
                // Generic "can't convert" error.
                errorSymSym(tree, ERR_NoImplicitConv, expr->type, dest);
            }
        }

        return newError(tree);
    }
}

// performs an implicit conversion if its possible. otherwise returns null. flags is an optional parameter.
EXPR * FUNCBREC::tryConvert(EXPR * expr, TYPESYM * dest, BASENODE * tree, unsigned flags)
{
    EXPR * exprResult;

    if (bindImplicitConversion(tree, expr, expr->type, dest, &exprResult, flags)) {
        // Conversion works.
        return exprResult;
    }
    else {
        return NULL;
    }
}


/*
 * bindExplicitConversion
 *
 * This is a complex routine with complex parameter. Generally, this should
 * be called through one of the helper methods that insulates you
 * from the complexity of the interface. This routine handles all the logic
 * associated with explicit conversions.
 *
 * Note that this function calls bindImplicitConversion first, so the main
 * logic is only concerned with conversions that can be made implicitly, but
 * not explicitly.
 */
bool FUNCBREC::bindExplicitConversion(BASENODE * tree, EXPR * exprSrc, TYPESYM * typeSrc, TYPESYM * typeDest, EXPR * * pexprDest, unsigned flags)
{
    // All implicit conversions are explicit conversions. Don't try user-defined conversions now because we'll
    // try them again later.
    if (bindImplicitConversion(tree, exprSrc, typeSrc, typeDest, pexprDest, flags | CONVERT_NOUDC | CONVERT_ISEXPLICIT))
        return true;

    // if we were casting an integral constant to another constant type,
    // then, if the constant were in range, then the above call would have succeeded.

    // But it failed, and so we know that the constant is not in range

    if (!typeSrc || !typeDest || typeSrc->kind == SK_ERRORSYM || typeDest->kind == SK_ERRORSYM) return false;

    switch (typeDest->kind) {
    default:
        ASSERT(0);
        break;

    case SK_NULLSYM:
        return false;  // Can never convert TO the null type.

    case SK_ARRAYSYM:
        /*
         * Handle conversions to arrays.
         * An array type can be explicitly converted to some other array types. Object, and Array
         * and their interface can be explicit converted to any array type.
         */
        if (typeSrc->kind == SK_ARRAYSYM) {
            // Handle array -> array conversions. Ranks must match (or cast from unknown rank),
            // and element type must be same or built-in explicit reference conversion.
            PARRAYSYM arraySrc = typeSrc->asARRAYSYM();
            PARRAYSYM arrayDest = typeDest->asARRAYSYM();
            PTYPESYM elementSrc = arraySrc->elementType();
            PTYPESYM elementDest = arrayDest->elementType();

            if (arraySrc->rank != arrayDest->rank)
                break;  // Ranks do not match.

            if (elementSrc == elementDest ||
                (elementSrc->fundType() == FT_REF && elementDest->fundType() == FT_REF &&
                 canCast(elementSrc, elementDest, tree, CONVERT_NOUDC)))
            {
                // Element types are compatible.
                return bindSimpleCast(tree, exprSrc, typeDest, pexprDest, EXF_REFCHECK);
            }

            break;
        }
        else if (canConvert(getPDT(PT_ARRAY), typeSrc, tree, CONVERT_NOUDC)) {
            // The above if checks for src==Array, object or an interface Array implements.
            return bindSimpleCast(tree, exprSrc, typeDest, pexprDest, EXF_REFCHECK);
        }

        break;

    case SK_PTRSYM:
        /* 
         * Handle pointer conversions.
         *
         * Any pointer can be explicitly converted to any other pointer.
         */
        // numeric integral types can be converted to a pointer, but that is not a standard conversion
        if (typeSrc->kind == SK_PTRSYM || (typeSrc->fundType() <= FT_LASTINTEGRAL && typeSrc->isNumericType() && !(flags & CONVERT_STANDARD))) {
            return bindSimpleCast(tree, exprSrc, typeDest, pexprDest);
        }

        break;

    case SK_AGGSYM: {
        AGGSYM * aggDest = typeDest->asAGGSYM();

        if (typeSrc->kind == SK_AGGSYM && typeSrc->asAGGSYM()->isEnum) {
            /* 
             * Handle explicit conversion from enum.
             * enums can explicitly convert to any numeric type or any other enum.
             */

            if (aggDest->isNumericType() || aggDest->isEnum) {
                if (exprSrc && exprSrc->kind == EK_CONSTANT) {
                    bool checkFailure = true;
                    if (bindConstantCast(tree, exprSrc, typeDest, pexprDest, &checkFailure))
                        return true;
                    if (checkFailure) {
                        return false;
                    }
                }

                return bindSimpleCast(tree, exprSrc, typeDest, pexprDest);
            }    

            break;
        }

        // TypeReference and ArgIterator can't be boxed (or converted to anything else)
        if (typeSrc->isSpecialByRefType())
            return false;

        if (aggDest->isEnum) {
            /* 
             * Handle conversions to enum.
             * Object or numeric types can explicitly convert to enums.
             * However, this is not considered on a par to a user-defined
             * conversion.
             */
            if (! (flags & CONVERT_NOUDC)) {
                if (typeSrc->isNumericType()) {
                    // Transform constant to constant.
                    if (exprSrc && exprSrc->kind == EK_CONSTANT) {
                        bool checkFailure = true;
                        if (bindConstantCast(tree, exprSrc, typeDest, pexprDest, &checkFailure))
                            return true;
                        if (checkFailure) {
                            return false;
                        }
                    }

                    return bindSimpleCast(tree, exprSrc, typeDest, pexprDest);
                }
                else if (typeSrc->kind == SK_AGGSYM && typeSrc->asAGGSYM()->isPredefined &&
                         (typeSrc->isPredefType(PT_OBJECT) || typeSrc->isPredefType(PT_VALUE) || typeSrc->isPredefType(PT_ENUM)))
                    return bindSimpleCast(tree, exprSrc, typeDest, pexprDest, EXF_UNBOX);
            }

            break;
        }


        if (typeSrc->isSimpleType() && typeDest->isSimpleType()) {
            /*
             * Handle conversions between simple types. (int->long, float->double, etc...)
             */
            ASSERT(typeSrc->asAGGSYM()->isPredefined && aggDest->isPredefined);
            int ptSrc = typeSrc->asAGGSYM()->iPredef;
            int ptDest = aggDest->iPredef;

            ASSERT(ptSrc < NUM_SIMPLE_TYPES && ptDest < NUM_SIMPLE_TYPES);

            unsigned convertKind = simpleTypeConversions[ptSrc][ptDest];
            ASSERT((convertKind & CONV_KIND_MASK) != CONV_KIND_ID);   // identity conversion should have been handled at first.
            ASSERT((convertKind & CONV_KIND_MASK) != CONV_KIND_IMP);  // implicit conversion should have been handled earlier.

            if ((convertKind & CONV_KIND_MASK) == CONV_KIND_EXP) {
                // An explicit conversion exists.

                if (exprSrc && exprSrc->kind == EK_CONSTANT) {
                    // Fold the constant cast if possible. 
                    bool checkFailure = true;
                    if (bindConstantCast(tree, exprSrc, typeDest, pexprDest, &checkFailure))
                        return true;  // else, don't fold and use a regular cast, below.
                    if (checkFailure && !(flags & CONVERT_CHECKOVERFLOW)) {
                        return false;
                    }
                }

                if (convertKind & CONV_KIND_USEUDC) {
                    if (!pexprDest) return true;
                    // According the language, this is a standard conversion, but it is implemented
                    // through a user-defined conversion. Because it's a standard conversion, we don't
                    // test the CONVERT_NOUDC flag here.
                    return bindUserDefinedConversion(tree, exprSrc, typeSrc, typeDest, pexprDest, false);
                }
                else {
                    return bindSimpleCast(tree, exprSrc, typeDest, pexprDest, (flags & CONVERT_CHECKOVERFLOW) ? EXF_CHECKOVERFLOW : 0);
                }
            }

            // No break here, continue testing for derived to base conversions below.
        }

        /* 
         * Handle struct, class and interface conversions. A class, struct, or interface is explicity convertable
         * to a derived class or interface. (Note the test against object is needed below
         * because object is NOT a base type of interfaces.)
         */
        if (typeSrc->kind == SK_AGGSYM) {
            AGGSYM * aggSrc = typeSrc->asAGGSYM();

            if (aggSrc->isPredefType(PT_OBJECT) || compiler()->symmgr.IsBaseType(aggDest, aggSrc)) 
            {
                if (aggDest->isStruct && aggSrc->fundType() == FT_REF)
                    return bindSimpleCast(tree, exprSrc, typeDest, pexprDest, EXF_UNBOX);
                else
                    return bindSimpleCast(tree, exprSrc, typeDest, pexprDest, EXF_REFCHECK | (exprSrc ? (exprSrc->flags & EXF_CANTBENULL) : 0));
            }

            /* A non-sealed class can cast to any interface, any interface can cast to a non-sealed class,
             * and any interface can cast to any other interface.
             */
            if ((aggSrc->isClass && !aggSrc->isSealed && aggDest->isInterface) ||
                (aggSrc->isInterface && aggDest->isClass && !aggDest->isSealed) ||
                (aggSrc->isInterface && aggDest->isInterface))
            {
                return bindSimpleCast(tree, exprSrc, typeDest, pexprDest, EXF_REFCHECK | (exprSrc ? (exprSrc->flags & EXF_CANTBENULL) : 0));
            }
        }

        // conversions from pointers to non-pointers are not standard conversions
        if (typeSrc->kind == SK_PTRSYM && typeDest->fundType() <= FT_LASTINTEGRAL && typeDest->isNumericType() && !(flags & CONVERT_STANDARD)) {
            return bindSimpleCast(tree, exprSrc, typeDest, pexprDest);
        }

        break;
    }}

    // No built-in conversion was found. Maybe a user-defined conversion?
    if (! (flags & CONVERT_NOUDC)) {
        return bindUserDefinedConversion(tree, exprSrc, typeSrc, typeDest, pexprDest, false);
    }

    return false;
}

// returns true if an explicit conversion exists from source type to dest type. flags is an optional parameter.
bool FUNCBREC::canCast(TYPESYM * src, TYPESYM * dest, BASENODE * tree, unsigned flags)
{
    return bindExplicitConversion(tree, NULL, src, dest, NULL, flags);
}

// returns true if a explicit conversion exists from source expr to dest type. flags is an optional parameter.
bool FUNCBREC::canCast(EXPR * expr, TYPESYM * dest, BASENODE *tree, unsigned flags)
{
    return bindExplicitConversion(tree, expr, expr->type, dest, NULL, flags);
}

// performs an explicit conversion if its possible. otherwise displays an error. flags is an optional parameter.
EXPR * FUNCBREC::mustCast(EXPR * expr, TYPESYM * dest, BASENODE * tree, unsigned flags)
{
    EXPR * exprResult;

    if (bindExplicitConversion(tree, expr, expr->type, dest, &exprResult, flags)) {
        // Conversion works.
        return exprResult;
    }
    else {

        if (expr->isOK() && dest && dest->kind != SK_ERRORSYM)  {// don't report cascading error.

            // For certain situations, try to give a better error.

            if (expr->kind == EK_CONSTANT && checked.constant &&
                expr->type->isSimpleType() && dest->isSimpleType())
            {
                // check if we failed because we are in checked mode...
                CHECKEDCONTEXT checked(this, false);
                bool okNow = bindExplicitConversion(tree, expr, expr->type, dest, NULL, flags | CONVERT_NOUDC);
                checked.restore(this);

                if (!okNow) goto CANTCONVERT;

                // Failed because value was out of range. Report nifty error message.
                WCHAR value[30];
                if (expr->type->isUnsigned())
                    swprintf(value, L"%I64u", expr->asCONSTANT()->getI64Value());
                else
                    swprintf(value, L"%I64d", expr->asCONSTANT()->getI64Value());

                errorStrSym(tree, ERR_ConstOutOfRangeChecked, value, dest);
            }
            else if (expr->type->kind == SK_NULLSYM && dest->fundType() != FT_REF) {
                errorSym(tree, ERR_ValueCantBeNull, dest);
            }
            else {
CANTCONVERT:
                // Generic "can't convert" error.
                errorSymSym(tree, ERR_NoExplicitConv, expr->type, dest);
            }
        }

        return newError(tree);


    }
}

// performs an explicit conversion if its possible. otherwise returns null. flags is an optional parameter.
EXPR * FUNCBREC::tryCast(EXPR * expr, TYPESYM * dest, BASENODE * tree, unsigned flags)
{
    EXPR * exprResult;

    if (bindExplicitConversion(tree, expr, expr->type, dest, &exprResult, flags)) {
        // Conversion works.
        return exprResult;
    }
    else {
        return NULL;
    }
}




// Bind the dot operator.  mask indicates allowed return values
EXPR * FUNCBREC::bindDot(BINOPNODE * tree, int mask, int bindFlags)
{
    // The only valid masks are:
    // MASK_MEMBVARSYM where we want a field
    // MASK_PROPSYM where we want a property
    // MASK_METHSYM where we want a method name, and we return an EXPRCALL
    // MASK_AGGSYN where we want a class name, and we return an EXPRCLASS
    // MASK_NSSSYM where we want a namespace, and we return an EXPRNSPACE

    ASSERT(mask);
    ASSERT(!(mask & ~(MASK_EVENTSYM | MASK_METHSYM | MASK_AGGSYM | MASK_NSSYM | MASK_MEMBVARSYM | MASK_PROPSYM)));

    BASENODE * first = tree->p1;
    NAMENODE * name = tree->p2->asNAME();
    bool base = false;

    EXPR * left;

    const int leftmask = MASK_AGGSYM | MASK_NSSYM | MASK_MEMBVARSYM | MASK_PROPSYM;
    if (tree->kind == NK_ARROW) {
        left = bindExpr(tree->p1);
        RETBAD(left);

        if (left->type->kind != SK_PTRSYM) {
            errorN(tree, ERR_PtrExpected);
            return newError(tree);
        }

        left = newExprBinop(tree, EK_INDIR, left->type->asPTRSYM()->parent->asTYPESYM(), left, NULL);
        left->flags |= EXF_ASSGOP | EXF_LVALUE;

    } else if (first->kind == NK_DOT) {
        // the left side is also a dot...
        left = bindDot(first->asDOT(), leftmask, BIND_RVALUEREQUIRED | bindFlags);
    } else if (first->kind == NK_NAME) {
        // The left could be anything...
        left = bindName(first->asNAME(), leftmask | MASK_LOCVARSYM, BIND_RVALUEREQUIRED | bindFlags);
        left->flags |= EXF_NAMEEXPR;
    } else if (first->kind == NK_OP && first->Op() == OP_BASE) {
        left = bindBase(first);
        base = true;
    } else {
        left = bindExpr(first);
    }
    RETBAD(left);

    PARENTSYM * parent;

    NAME * rIdent = name->pName;
    SYM * right;
    SYM * inaccessibleSym = NULL;

    if (left->kind == EK_NSPACE) {
        SYM * badSymKind = NULL;  // This can never get set since MASK_ALL is used below.
        parent = left->asNSPACE()->nspace;
        //We need to check accessibility
        right = compiler()->clsDeclRec.lookupIfAccessOk(rIdent, parent, MASK_ALL, pParent, true, &inaccessibleSym, &badSymKind);
    } else {
        parent = left->type;
        if (!base) {

            if (parent->kind == SK_ARRAYSYM) {
                // Length property on rank 1 arrays is converted into the ldlen instruction.
                if (rIdent == compiler()->namemgr->GetPredefName(PN_LENGTH) && parent->asARRAYSYM()->rank == 1 && left->kind != EK_CLASS) {
                    EXPR * rval = newExprBinop(name, EK_ARRLEN, getPDT(PT_INT), left, NULL);
                    rval->flags |= EXF_ASSGOP;
                    return rval;

                } else {

                    parent = compiler()->symmgr.GetPredefType(PT_ARRAY, true);
                }
            }
        }

        compiler()->symmgr.DeclareType(parent);
        if (parent->kind != SK_AGGSYM) {
            errorStrSym(tree, ERR_BadUnaryOp, L".", parent);
            return newError(tree);
        }
        right = lookupClassMember(rIdent, parent->asAGGSYM(), base, false, true);

NEWRIGHT:

        // can only reference type names through their containing type
        if (right && (right->kind == SK_AGGSYM) && (left->kind != EK_CLASS)) {
            compiler()->clsDeclRec.errorNameRefStr(name, pParent, compiler()->ErrSym(right), ERR_BadTypeReference);
            return newError(name);
        }
        if (!right) {
            // For better error reporting, see if there was a symbol, but it was inaccessible.
            inaccessibleSym = lookupClassMember(rIdent, parent->asAGGSYM(), true, true, true);
        }
    }

    if (!right) {
        if (inaccessibleSym) {
            if (inaccessibleSym->isUserCallable()) {
                if ((inaccessibleSym->access == ACC_PROTECTED || 
                     (inaccessibleSym->access == ACC_INTERNALPROTECTED && pParent->getAssemblyIndex() != inaccessibleSym->getAssemblyIndex())) && 
                    !base && lookupClassMember(rIdent, parent->asAGGSYM(), true, false, true))
                {
                    errorSymNameName(tree, ERR_BadProtectedAccess, inaccessibleSym, parent->name, pParent->name);
                }
                else
                    errorSym(tree, ERR_BadAccess, inaccessibleSym);
            } else {
                errorSym(tree, ERR_CantCallSpecialMethod, inaccessibleSym);
            }
        } else if (parent->kind == SK_AGGSYM && parent->asAGGSYM()->isDelegate &&
                rIdent == compiler()->namemgr->GetPredefName(PN_INVOKE)) {
            errorN(name, ERR_DontUseInvoke);
            return left;
        } else {
            if (parent->kind == SK_NSSYM) 
                errorNameSym(tree, ERR_TypeNameNotFound, name->pName, parent);
            else
                errorSymName(tree, ERR_NoSuchMember, parent, name->pName);
        }
        return newError(tree);
    }
    if (!(right->mask() & mask)) {
        // if we are looking at itf members and we got a method where we wanted a prop, or the reverse,
        // then we redo the search looking for the right type, and return that...

        // we will later catch the ambiguity (which is inevitable in this case) in verifymethodcall
        if (parent->kind == SK_AGGSYM && parent->asAGGSYM()->isInterface) {
            if (right->mask() & (MASK_METHSYM | MASK_PROPSYM)) {
                if (mask & (MASK_METHSYM | MASK_PROPSYM)) {
                    SYM * newRight = lookupClassMember(rIdent, parent->asAGGSYM(), base, false, true, mask & (MASK_PROPSYM | MASK_METHSYM));
                    if (newRight) {
                        right = newRight;
                        goto NEWRIGHT;
                    }
                }
            }
        }
        return errorBadSK(name, right, mask);
    }

    EXPR * rval;

    EXPR * object = (left->kind == EK_CLASS) ? NULL : left;

    switch(right->kind) {
    case SK_MEMBVARSYM:
        ASSERT(left->kind != EK_NSPACE);
        rval = bindToField(tree, object, right->asMEMBVARSYM(), bindFlags);
        break;
    case SK_PROPSYM:
        ASSERT(left->kind != EK_NSPACE);
        rval = bindToProperty(tree, object, right->asPROPSYM(), base ? (bindFlags | BIND_BASECALL) : bindFlags);
        break;
    case SK_EVENTSYM:
        ASSERT(left->kind != EK_NSPACE);
        rval = bindToEvent(tree, object, right->asEVENTSYM(), base ? (bindFlags | BIND_BASECALL) : bindFlags);
        break;
    case SK_NSSYM:
        rval = newExpr(tree, EK_NSPACE, NULL);
        rval->asNSPACE()->nspace = right->asNSSYM();
        break;
    case SK_AGGSYM:
        rval = newExpr(tree, EK_CLASS, right->asAGGSYM());
        compiler()->symmgr.DeclareType(right->asAGGSYM());
        if (checkBogus(right)) {
            errorSym(tree, ERR_BogusType, right);    
        }
        else {
            if (right->asAGGSYM()->hasMultipleDefs)
                errorSymName(tree, WRN_MultipleTypeDefs, right, right->getInputFile()->name);
            if (right->isDeprecated)
                reportDeprecated(tree, right);
        }
        break;
    case SK_METHSYM:
        rval = newExpr(tree, EK_CALL, NULL);
        rval->asCALL()->object = object;
        rval->asCALL()->args = NULL;
        rval->asCALL()->method = right->asMETHSYM();
        if (base) {
            rval->flags |= EXF_BASECALL;
        }

        break;
    default: ASSERT(!"bad kind"); rval = NULL;
    }

    if (rval->type) {
        checkUnsafe(tree, rval->type);
    }

    return rval;
}


EXPR * FUNCBREC::bindMethodName(BASENODE * tree)
{
    EXPRCALL * call;
    EXPR * expr;
    if (tree->kind == NK_NAME) {
        expr = bindName(tree->asNAME(), MASK_METHSYM | MASK_MEMBVARSYM | MASK_LOCVARSYM | MASK_PROPSYM);
    } else if (tree->kind == NK_DOT || tree->kind == NK_ARROW) {
        expr = bindDot((BINOPNODE*)tree, MASK_METHSYM | MASK_MEMBVARSYM | MASK_PROPSYM);
    } else {
        expr = bindExpr(tree);
    }

    RETBAD(expr);

    if (expr->kind != EK_CALL || expr->flags & EXF_BOUNDCALL) {
        if (expr->type->kind == SK_AGGSYM && expr->type->asAGGSYM()->isDelegate) {
            METHSYM * invoke = compiler()->symmgr.LookupGlobalSym(compiler()->namemgr->GetPredefName(PN_INVOKE), expr->type->asAGGSYM(), MASK_METHSYM)->asMETHSYM();
            if (!invoke) goto LERROR;
            call = newExpr(tree, EK_CALL, NULL)->asCALL();
            call->object = expr;
            call->method = invoke;
            call->args = NULL;
        } else {
            // Not a delegate, so to try to get a better error message, rebind, allowing only a method.

            if (tree->kind == NK_NAME) {
                expr = bindName(tree->asNAME(), MASK_METHSYM);
            } else if (tree->kind == NK_DOT) {
                expr = bindDot(tree->asDOT(), MASK_METHSYM);
            } else {
                expr = bindExpr(tree);
            }

            RETBAD(expr);

            // Some probably that rebinding didn't catch, so use the generic error message.
            errorN(tree, ERR_MethodNameExpected);
            goto LERROR;
        }
    } else {
        call = expr->asCALL();
    }

    return call;

LERROR:
    return newError(tree);
}

// Bind a function call
EXPR * FUNCBREC::bindCall(BINOPNODE * tree, int bindFlags)
{
    ASSERT(tree->Op() == OP_CALL);
    EXPR * args = bindExpr(tree->p2, BIND_RVALUEREQUIRED | BIND_ARGUMENTS);
    RETBAD(args);

    if (tree->p1->kind == NK_OP && tree->p1->Op() == OP_ARGS) {
        if (!(bindFlags & BIND_ARGUMENTS)) {
            errorN(tree, ERR_IllegalArglist);
        }
        return newExprBinop(tree, EK_ARGLIST, compiler()->symmgr.GetArglistSym(), args, NULL); 
    }


    EXPR * expr = bindMethodName(tree->p1);
    RETBAD(expr);

    EXPRCALL * call = expr->asCALL();

    call->args = args;
    call->method = verifyMethodCall(tree, call->method, args, &(call->object), call->flags & EXF_BASECALL)->asMETHSYM();
    call->flags |= EXF_BOUNDCALL;
    if (call->method) {
        verifyMethodArgs(call);
        call->type = call->method->asMETHSYM()->retType;
        NAMELIST *conditionalList = call->method->getBaseConditionalSymbols(& compiler()->symmgr );
        if (conditionalList) {
            do
            {
                if (pMSym->getInputFile()->isSymbolDefined(conditionalList->name)) {
                    break;
                }
                conditionalList = conditionalList->next;
            } while (conditionalList);

            if (!conditionalList) {
                //
                // turn this call into a no-op
                //
                return newExpr(tree, EK_NOOP, getPDT(PT_VOID));
            }
        }
    } else {
        return newError(tree);
    }

    return call;
}


// This table is used to implement the last set of 'better' conversion rules
// when there are no implicit conversions between T1(down) and T2 (across)
// Use all the simple types plus 1 more for Object
// See CLR section 7.4.1.3
static const unsigned char betterConversionTable[NUM_SIMPLE_TYPES + 1][NUM_SIMPLE_TYPES + 1] = {
//          BYTE    SHORT   INT     LONG    FLOAT   DOUBLE  DECIMAL CHAR    BOOL    SBYTE   USHORT  UINT    ULONG   OBJECT
/* BYTE   */{0,     0,      0,      0,      0,      0,      0,      0,      0,      2,      0,      0,      0,      0},
/* SHORT  */{0,     0,      0,      0,      0,      0,      0,      0,      0,      0,      1,      1,      1,      0},
/* INT    */{0,     0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      1,      1,      0},
/* LONG   */{0,     0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      1,      0},
/* FLOAT  */{0,     0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0},
/* DOUBLE */{0,     0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0},
/* DECIMAL*/{0,     0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0},
/* CHAR   */{0,     0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0},
/* BOOL   */{0,     0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0},
/* SBYTE  */{1,     0,      0,      0,      0,      0,      0,      0,      0,      0,      1,      1,      1,      0},
/* USHORT */{0,     2,      0,      0,      0,      0,      0,      0,      0,      2,      0,      0,      0,      0},
/* UINT   */{0,     2,      2,      0,      0,      0,      0,      0,      0,      2,      0,      0,      0,      0},
/* ULONG  */{0,     2,      2,      2,      0,      0,      0,      0,      0,      2,      0,      0,      0,      0},
/* OBJECT */{0,     0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0}};



// Determine which method is better for the pruposes of overload resolution.
// Better means: as least as good in all params, and better in at least one param.
// Better w/r to a param means is an ordering, from best down:
// 1) same type as argument
// 2) implicit conversion from argument to formal type
// Because of user defined conversion opers this relation is nontransitive
// returns 1 = m1, -1, m2, 0 neither
int FUNCBREC::whichMethodIsBetter(METHPROPSYM * m1, METHPROPSYM * m2, EXPR * args)
{

    bool a2b, b2a;
    int better = 0;
    bool sameArgs = true;

    bool exp1 = m1->kind == SK_EXPANDEDPARAMSSYM;
    bool exp2 = m2->kind == SK_EXPANDEDPARAMSSYM;

    if (m1->parent->asAGGSYM()->isInterface) {
        ASSERT(m2->parent->asAGGSYM()->isInterface);
        if (m1->parent != m2->parent) {
            bool a2b = canConvert(m1->parent->asAGGSYM(), m2->parent->asAGGSYM(), NULL, CONVERT_NOUDC);
            bool b2a = canConvert(m2->parent->asAGGSYM(), m1->parent->asAGGSYM(), NULL, CONVERT_NOUDC);
            if (a2b && !b2a) {
                better = 1;
            } else if (b2a && !a2b) {
                better = -1;
            }
        }
    }

    TYPESYM ** p1 = m1->params;
    TYPESYM ** p2 = m2->params;

//    unsigned argCount = __min(m1->cParams, m2->cParams);

    EXPRLOOP(args, arg)

/*        if (!argCount) {
            if (m1->isParamArray && m2->isParamArray) {
                break;
            }
            if (m1->isParamArray) {
                goto BETM2;
            } else {
                goto BETM1;
            }
        }
        argCount--;
*/
        if (*p1 == *p2) goto NEXT;

        sameArgs = false;


        // Check exact mathches:
        if (arg->type == *p1) {
            goto BETM1;
        }
        if (arg->type == *p2) {
            goto BETM2;
        }
        a2b = canConvert(*p1, *p2, NULL);
        b2a = canConvert(*p2, *p1, NULL);

        if (!(a2b ^ b2a) &&
            (*p1)->kind == SK_AGGSYM && (*p2)->kind == SK_AGGSYM &&
            (*p1)->asAGGSYM()->isPredefined && (*p2)->asAGGSYM()->isPredefined &&
            (*p1)->asAGGSYM()->iPredef <= PT_OBJECT && (*p2)->asAGGSYM()->iPredef <= PT_OBJECT) {
            // See CLR section 7.4.1.3
            int c = betterConversionTable[(*p1)->asAGGSYM()->iPredef][(*p2)->asAGGSYM()->iPredef];
            if (c == 1)
                goto BETM1;
            else if (c == 2)
                goto BETM2;
        }

        if (a2b && !b2a) {
BETM1:
            if (better < 0) {
                return 0;
            } else {
                better = 1;
                goto NEXT;
            }
        }
        if (b2a && !a2b) {
BETM2:
            if (better > 0) {
                return 0;
            } else {
                better = -1;
            }
        }
NEXT:
        p1++;
        p2++;
    ENDLOOP;

    if (!better && sameArgs) {
        if (exp1 && !exp2) {
            return -2;
        } else if (exp2 && !exp1) {
            return 2;
        }
    }

    return better;
}


EXPR * FUNCBREC::bindUDBinop(BASENODE * tree, EXPRKIND ek, EXPR * op1, EXPR * op2)
{


    NAME * name = ekName(ek);
    ASSERT(name);

    METHLIST * bestSoFar = NULL;

    TYPESYM * type = op1->type;
    AGGSYM * commonBase = AGGSYM::commonBase(type, op2->type);
    // if the types are the same, we skip op2 and proceed straight to op1...
    int typeCount = 0;
    bool foundInType = false;
    do {
NEXTARG:
        if (!type || type->kind != SK_AGGSYM) {
            if (typeCount) break;

            typeCount = 1;
            foundInType = false;
            type = op2->type;
            goto CHECKNEWTYPE;
        }

        SYM * sym;
        sym = compiler()->symmgr.LookupGlobalSym(name, type, MASK_ALL);
        if (type->kind == SK_AGGSYM) {
            addDependancyOnType(type->asAGGSYM());
        }


        while (true) {
            if (!sym) {
                if (!foundInType) {
                    type = type->asAGGSYM()->baseClass;
                    // if the next type is already covered int he other hierarchy,
                    // then let that other hierarchy deal w/ it...
                    if (typeCount) {
CHECKNEWTYPE:
                        if (type == commonBase) {
                            type = NULL;
                        }
                    }

                } else {
                    type = NULL;
                }
                goto NEXTARG;
            }

            if (sym->kind != SK_METHSYM || !sym->asMETHSYM()->isOperator) {
                if (sym->kind == SK_METHSYM && sym->asMETHSYM()->cParams == 1) {
                    goto LOOKNEXT;
                }
                // oops, this is not what we wanted... 
                type = NULL;
                goto NEXTARG;
            }

            ASSERT(sym->asMETHSYM()->cParams == 2);

            if (canConvert(op1, sym->asMETHSYM()->params[0], NULL) &&
                canConvert(op2, sym->asMETHSYM()->params[1], NULL)) {
                foundInType = true;
                METHLIST * newNode = (METHLIST *) _alloca(sizeof(METHLIST));
                newNode->next = bestSoFar;
                newNode->position = bestSoFar ? bestSoFar->position - 1 : 0;
                newNode->sym = sym->asMETHSYM();
                newNode->relevant = true;
                bestSoFar = newNode;
            }
LOOKNEXT:
            sym = compiler()->symmgr.LookupNextSym(sym, sym->parent, MASK_METHSYM)->asMETHPROPSYM();
        }

    } while (true);

    if (!bestSoFar) return NULL;

    EXPR * argList = newExprBinop(NULL, EK_LIST, NULL, op1, op2);

    PMETHPROPSYM ambig1, ambig2;
    METHSYM * best = findBestMethod(bestSoFar, argList, &ambig1, &ambig2)->asMETHSYM();
    if (!best) {
        // No winner, so its an ambigous call...
        errorSymSym(tree, ERR_AmbigCall, ambig1, ambig2);
        return newError(tree);
    }
    if (checkBogus(best)) {
        compiler()->clsDeclRec.errorNameRefSymbol(best->name, tree, pParent, best, ERR_BindToBogus);
        return newError(tree);
    }

    EXPRCALL * call = newExpr(tree, EK_CALL, best->retType)->asCALL();
    call->object = NULL;
    call->args = argList;
    call->method = best;
    call->flags |= EXF_BOUNDCALL;

    verifyMethodArgs(call);

    return call;

}

// Determine best method for overload resolution. Returns NULL if no best method, in which
// case two tying methods are returned for error reporting.
METHPROPSYM * FUNCBREC::findBestMethod(METHLIST * list, EXPR * args, PMETHPROPSYM * ambigMeth1, PMETHPROPSYM * ambigMeth2)
{
    ASSERT(list && list->sym);

    // select the best method:
    /*
    We run a tournament as follows: picking from the left, we nominate methods
    as candidates.  This method is compared to every other method, also
    moving left and starting w/ the first method.  We call the methods compared to
    contender methods.  At the first loss or tie we stop the comparison
    and note that this candidate method cannot be the best method.
    At which point the contender method becomes the candidate if the
    contender won the comparison and is further to the left.  If the comparison
    was a tie and the contender is further to the left, then the method to the
    left of the contender becomes the candidate.  Otherwise, since the candidate
    was to the left of the contender, then we simply move leftwards and
    appoint the next method.
    */
    METHLIST * candidate;
    PMETHPROPSYM ambig1 = NULL, ambig2 = NULL;  // Record two method that are ambiguous for error reporting.

    bool isInterface = list->sym->parent && list->sym->parent->kind == SK_AGGSYM && list->sym->parent->asAGGSYM()->isInterface;

    SYM * parent = NULL;

    for (candidate = list; candidate;) {
        if (candidate->sym->parent->asAGGSYM()->isPrecluded) {
            candidate = candidate->next;
            continue;
        }

        if (isInterface) {
            if (!parent) {
                parent = candidate->sym->parent;
            } else {
                if (candidate->sym->parent != parent) {
                    *ambigMeth1 = list->sym;
                    *ambigMeth2 = candidate->sym;
                    return NULL;
                }
            }
        }

        for (METHLIST *contender = list; contender; contender = contender->next) {
            if (candidate == contender) continue;
            if (contender->sym->parent->asAGGSYM()->isPrecluded) {
                contender->relevant = false;
                continue;
            }
            int result = whichMethodIsBetter(candidate->sym, contender->sym, args);
            if (result > 0) {
                if (result == 2) {
                    contender->relevant = false;
                }
                continue;  // (meaning m1 is better...)
            }

            // so m2 is better, or a tie...
            if (result == -2) {
                candidate->relevant = false;
            }

            if (candidate->position > contender->position) {
                candidate = candidate->next;
            } else {
                if (!result) {
                    // in case of tie we don't want to bother with the contender who tied...
                    ambig1 = candidate->sym; ambig2 = contender->sym;
                    candidate = contender->next;
                } else {
                    candidate = contender;
                }
            }
            goto NEXTCANDIDATE;
        }
        // Wow, we managed to loop all the way through, so we have a winner:
        return candidate->sym;
NEXTCANDIDATE:;
    }

    // an ambig call. Return two of the ambiguous set.
    if (ambig1 && ambig2) {
        *ambigMeth1 = ambig1; *ambigMeth2 = ambig2;
    }
    else {
        // For some reason, we have an ambiguity but never had a tie. I'm not sure if this can
        // happen, but just use the first two methods in this case.
        *ambigMeth1 = list->sym; *ambigMeth2 = list->next->sym;
    }

    return NULL;
}



EXPANDEDPARAMSSYM * FUNCBREC::getParamsMethod(METHPROPSYM * meth, unsigned count)
{

    AGGSYM * parent = meth->parent->asAGGSYM();

    ASSERT(meth->isParamArray);

    EXPANDEDPARAMSSYM * rval = compiler()->symmgr.LookupGlobalSym(meth->name, parent, MASK_EXPANDEDPARAMSSYM)->asEXPANDEDPARAMSSYM();

    while (rval) {
        if (rval->realMethod == meth && rval->cParams == count) {
            return rval;
        }
        rval = compiler()->symmgr.LookupNextSym(rval, parent, MASK_EXPANDEDPARAMSSYM)->asEXPANDEDPARAMSSYM();
    }

    rval = compiler()->symmgr.CreateGlobalSym(SK_EXPANDEDPARAMSSYM, meth->name, parent)->asEXPANDEDPARAMSSYM();
    meth->copyInto(rval);
    rval->realMethod = meth;
    rval->cParams = count;
    
    TYPESYM ** types = (TYPESYM**) _alloca(sizeof(TYPESYM*) * count);
    memcpy(types, meth->params, (meth->cParams - 1) * sizeof (TYPESYM*));

    TYPESYM * et = meth->params[meth->cParams - 1]->asARRAYSYM()->elementType();

    for (unsigned int cc = meth->cParams - 1; cc < count; cc++) {
        types[cc] = et;
    }

    rval->params = compiler()->symmgr.AllocParams(count, types);

    return rval;
}





AGGSYM * FUNCBREC::pickNextInterface(AGGSYM * current, SYMLISTSTACK ** stack, bool * hideByNameMethodFound, SYMLISTSTACK ** newStackSlot)
{

    SYMLIST * bases = current->ifaceList;

    if (bases && (!*hideByNameMethodFound)) {
        // if we found a HBN method then we do not consider the bases of this interface...
        current = bases->sym->asAGGSYM();
        bases = bases->next;
        if (bases) {
            SYMLISTSTACK * newSlot = *newStackSlot;
            *newStackSlot = NULL;
            newSlot->syms = bases;
            newSlot->next = stack[0];
            stack[0] = newSlot;
        }
    } else {
AGAIN:
        *hideByNameMethodFound = false;
        // since we are starting a new branch, we need to reset the flag to false... 
        // [effectively, we are resetting it to what it was, which we know to be "false"
        // since otherwise we would not have any branches to consider]

        // no candidate, so we need to go to the stack...
        if (stack[0]) {
            bases = stack[0]->syms;
            current = bases->sym->asAGGSYM();
            bases = bases->next;
            if (bases) {
                stack[0]->syms = bases;
            } else {
                stack[0] = stack[0]->next;
            }
        } else {
            return NULL;
        }
    }

    if (current->isPrecluded) goto AGAIN;

    return current;
}


void FUNCBREC::cleanupInterfaces(SYMLIST * precludedList)
{

    FOREACHSYMLIST(precludedList, elem)
        ASSERT(elem->asAGGSYM()->hasCandidateMethod);
        elem->asAGGSYM()->hasCandidateMethod = false;
        FOREACHSYMLIST(elem->asAGGSYM()->allIfaceList, precluded)
            precluded->asAGGSYM()->isPrecluded = false;
        ENDFOREACHSYMLIST
    ENDFOREACHSYMLIST

}

// Is the method/property callable. Not if it's an override or not user-callable.
bool FUNCBREC::isMethPropCallable(METHPROPSYM * sym, bool requireUC)
{
    // The hide-by-name option for binding other languages takes precedence over general
    // rules of not binding to overrides.
    if ((sym->isOverride && !sym->isHideByName) || (requireUC && !sym->isUserCallable()))
        return false;
    else
        return true;
}

// Perform overload resolution on the method to be called
//
// tree     - parse tree location to report errors
// mp       - a method or property on the class in question with the right name
//            that we are looking for
// args     - the argument list we are trying to do overload resolution against
// object   - the expression for the this pointer
// flags
//      EXF_NEWOBJCALL      - this is a new<type> expression
//
METHPROPSYM * FUNCBREC::verifyMethodCall(BASENODE * tree, METHPROPSYM * mp, EXPR * args, EXPR ** object, int flags, METHSYM * invoke)
{
    METHPROPSYM * unaccessAlt = NULL;

    METHPROPSYM * current = mp;
    METHPROPSYM * badArgsAlt = current;
    METHPROPSYM * badArgType = NULL;

    METHLIST * bestSoFar = NULL;
    METHLIST bestListItem;

    SYMLIST * precludedInterfaces = NULL;
    SYMLISTSTACK * traversalStack = NULL;
    SYMLISTSTACK * newStackSlot = NULL;

    TYPESYM * origObject = (object && object[0]) ? object[0]->type : NULL;

    TYPESYM * qualifier = (flags & EXF_BASECALL) ? NULL : origObject;

    int mask = current->mask();
    int iGoodTypes = -1;

    bool isInterface = current->parent->asAGGSYM()->isInterface;
    bool forceUC = current->isUserCallable();

    bool hideByNameMethodFound = false;

    if (isInterface && !*object) {
        // this will do the blowing up for us:
        checkStaticness(tree, mp, object);
        return NULL;
    }

    ASSERT(!isInterface || (*object)->type->asAGGSYM()->isInterface);
    AGGSYM * currentInterface = isInterface ? (*object)->type->asAGGSYM() : NULL;

    bool UseDelegateErrors = false;
    int cActuals;
    if (invoke) {
        cActuals = invoke->cParams;
    } else {
        bool hasErrorArgs = false;
        // Lets count them!
        cActuals = 0;
        EXPR * actualList = args;
        while (actualList) {
            EXPR * actual;
            cActuals++;
            if (actualList->kind == EK_LIST) {
                actual = actualList->asBIN()->p1;
                actualList = actualList->asBIN()->p2;
            } else {
                actual = actualList;
                actualList = NULL;
            }
            hasErrorArgs |= (actual->kind == EK_ERROR);
        }

        // If it has the right number of args, but one is
        // an error, we don't want to cascade it by
        // checking the coversions, so just return a NULL.
        if (hasErrorArgs) goto ERRORL;
    }


    if (isInterface) {
        goto FINDFIRSTINTERFACEMETHOD;
    }

    do {

        // NOTE: If you change this code for checking the args,
        //  be sure to also change the code for reporting errors
        //  down below.

        hideByNameMethodFound |= current->isHideByName;

        int iBestArgCount;
        iBestArgCount = 0;
        int cFormals;
        cFormals = current->cParams;
        // If how many actuals we got is not what this
        // method expectes, bail to the next method...
        if (cFormals != cActuals) goto NEXT;

        if (invoke) {
            // We want a complete match, not implicit convertability...
            if (invoke->params != current->params || invoke->retType != current->retType || invoke->isParamArray != current->isParamArray) {
                goto NEXT;
            }
        } else {
            iBestArgCount = 0;
            EXPR * actualList;
            actualList = args;
            if (args) {
                unsigned paramCount = current->cParams;
                TYPESYM ** params = current->params;
                while (actualList) {
                    paramCount--;
                    EXPR * actual;
                    if (actualList->kind == EK_LIST) {
                        actual = actualList->asBIN()->p1;
                        actualList = actualList->asBIN()->p2;
                    } else {
                        actual = actualList;
                        actualList = NULL;
                    }

                    if (!canConvert(actual,*params, NULL)) {
                        if ( iBestArgCount > iGoodTypes || badArgType == NULL) {
                            iGoodTypes = iBestArgCount;
                            badArgType = current;
                        } else if (iBestArgCount == iGoodTypes) {
                            TYPESYM * strippedActual = actual->type->kind == SK_PARAMMODSYM ? actual->type->asPARAMMODSYM()->paramType() : actual->type;
                            TYPESYM * strippedCurrent = params[0]->kind == SK_PARAMMODSYM ? params[0]->asPARAMMODSYM()->paramType() : params[0];
                            if (strippedActual == strippedCurrent && (strippedActual != actual->type || strippedCurrent != params[0])) {
                                badArgType = current;
                            }
                        }
                        goto NEXT;
                    }

                    params++;
                    iBestArgCount++;
                }
            }
        }

        // We know we have the right number of arguments and they are all convertable
        if (!compiler()->clsDeclRec.checkAccess(current, pParent, NULL, qualifier)) {
            // In case we never get an accesable method, this will allow us to give
            // a better error...
            unaccessAlt = current;
            goto NEXT;
        }

        // ok, this is a plausible method to call

        // if it was found on an interface, then all the base interfaces are precluded
        // also, this interface should also be marked as having a candidate method...

        if (currentInterface) {
            ASSERT(!currentInterface->isPrecluded);
            currentInterface->hasCandidateMethod = true;
            FOREACHSYMLIST(currentInterface->allIfaceList, elem)
                elem->asAGGSYM()->isPrecluded = true;
            ENDFOREACHSYMLIST
            if (!precludedInterfaces || precludedInterfaces->sym != currentInterface) {
                SYMLIST * newList = (SYMLIST *)_alloca(sizeof(SYMLIST));
                newList->sym = currentInterface;
                newList->next = precludedInterfaces;
                precludedInterfaces = newList;
            }
        }

        if (!bestSoFar) {
            // do a quickie for that single no override case...
            bestSoFar = &bestListItem;
            bestListItem.sym = current;

        //
        // don't add the incoming method more than once
        // which can happen when binding to an interface method call
        //
        } else if (current != bestListItem.sym) {


            METHLIST * newNode = (METHLIST *) _alloca(sizeof(METHLIST));
            newNode->next = bestSoFar;
            newNode->sym = current;
            newNode->position = bestSoFar->position - 1;
            newNode->relevant = true;
            bestSoFar = newNode;
        }

NEXT:

        if (current->isParamArray) {
            if (current->kind != SK_EXPANDEDPARAMSSYM) {
				if (!bestSoFar || bestSoFar->sym != current) {
					if (cActuals >= current->cParams - 1) {
						current = getParamsMethod(current, cActuals);
						continue;
					}
				}
            } else {
                current = current->asEXPANDEDPARAMSSYM()->realMethod;
            }
        }

        SYM * next;
        next = current;

        if (current->name) {  
            // get next non-override method or property
            while ((next = compiler()->symmgr.LookupNextSym(next, current->parent, mask)->asMETHPROPSYM())  &&
                   !isMethPropCallable(next->asMETHPROPSYM(), forceUC)) {
                /*nothing */
                ;
            }
        }
        else {
            // current->name can be NULL in the case where we bound to a explicit impl event backing property.
            // don't look further in that case.
            next = NULL;
        }

        // go to the baseclass only if we found no plausible methods in the current
        // class and we aren't looking for a constructor
        if (!next && (!bestSoFar || isInterface) && (mp->kind == SK_PROPSYM || !mp->asMETHSYM()->isCtor) && current->name) {
            AGGSYM * base;
            if (isInterface) {
                while (true) {
                    do  {
                        if (!newStackSlot) {
                            newStackSlot = (SYMLISTSTACK *)_alloca(sizeof(SYMLISTSTACK));
                        }
                        currentInterface = pickNextInterface(currentInterface, &traversalStack, &hideByNameMethodFound, &newStackSlot);
                    } while (currentInterface && currentInterface->hasCandidateMethod);
                    if (currentInterface) {
                        ASSERT(!currentInterface->isPrecluded);
FINDFIRSTINTERFACEMETHOD:
                        addDependancyOnType(currentInterface);
                        next = compiler()->symmgr.LookupGlobalSym(mp->name, currentInterface, MASK_PROPSYM | MASK_METHSYM);
                        if (next) {
                            // first, we need to check if this matches the kind of things we are looking for:
                            if (next->kind != mp->kind) {
                                errorSymSym(tree, ERR_AmbigMethProp, mp, next);
                                goto ERRORL;
                            }
                            goto FOUNDONE;
                        }
                    } else {
                        break;
                    }
                }
                // now, consider methods on object...
                if (!bestSoFar) {
                    base = getPDT(PT_OBJECT)->asAGGSYM();
                    isInterface = false;
                    goto DOBASE;
                }
FOUNDONE:;
            } else if (!hideByNameMethodFound) {
                base = current->parent->asAGGSYM()->baseClass;
DOBASE:
                //
                // find next base member with this name
                // which is not an override or a hidden member of the wrong type
                //
                next = NULL;
                while ((next = compiler()->clsDeclRec.findNextName(current->name, base, next))) {
                    if (!(next->mask() & mask)) {
                        // ignore inaccessible members of wrong type
                        if (compiler()->clsDeclRec.checkAccess(next, pParent, NULL, qualifier)) {
                            // oops, this is not what we wanted... so error out...
                            errorBadSK(tree, next, mask);
                            goto ERRORL;
                        }

                    // if this isn't an override, then continue
                    // NOTE that if this is inaccessible we try anyway!
                    // so that if it is the only match we can report a good error...
                    } else if (isMethPropCallable(next->asMETHPROPSYM(), forceUC)) {
                        
                        break;
                    }

                    // we never include overridden methods in overload resolution
                    // so keep looking
                    base = next->parent->asAGGSYM();
                }
                if (!next) {
                    goto REPORTERROR;
                }
            }
        }
        current = next->asMETHPROPSYM();
    } while (current);

    // Ok, we looked at all the evidence, and we come to render the verdict:

    if (bestSoFar == &bestListItem) {
        // We found the singular best method to call:
        current = bestListItem.sym;
        goto CHECKRETURN;
    }

    if (bestSoFar) {
        PMETHPROPSYM ambig1, ambig2;
        current = findBestMethod(bestSoFar, args, &ambig1, &ambig2);

        if (current) {
            goto CHECKRETURN;
        }

        ambig1 = ambig1->kind == SK_EXPANDEDPARAMSSYM ? ambig1->asEXPANDEDPARAMSSYM()->realMethod : ambig1;
        ambig2 = ambig2->kind == SK_EXPANDEDPARAMSSYM ? ambig2->asEXPANDEDPARAMSSYM()->realMethod : ambig2;

        errorSymSym(tree, ERR_AmbigCall, ambig1 , ambig2);

        goto ERRORL;

    }

REPORTERROR:

    ASSERT(!traversalStack);


    if (unaccessAlt) {
        // We might have called this, but it is unaccesable...
        if ((unaccessAlt->access == ACC_PROTECTED || 
             (unaccessAlt->access == ACC_INTERNALPROTECTED && pParent->getAssemblyIndex() != unaccessAlt->getAssemblyIndex())) && 
            !(flags & EXF_BASECALL) && compiler()->clsDeclRec.checkAccess(unaccessAlt, pParent, NULL, NULL))
        {
            errorSymNameName(tree, ERR_BadProtectedAccess, unaccessAlt, origObject->name, pParent->name);
        }
        else
            errorSym(tree, ERR_BadAccess, unaccessAlt);
        goto ERRORL;
    }
    // Finally, we got a method where the args are just plain wrong:
    if (checkBogus(badArgsAlt)) {
        if (badArgsAlt->kind == SK_PROPSYM && badArgsAlt->useMethInstead) {
            CError  *pError;
            if (badArgsAlt->asPROPSYM()->methGet && badArgsAlt->asPROPSYM()->methSet)
                pError = make_errorStrSymSym( tree, ERR_BindToBogusProp2, compiler()->ErrName(badArgsAlt->name), badArgsAlt->asPROPSYM()->methGet, badArgsAlt->asPROPSYM()->methSet);
            else
                pError = make_errorStrSym( tree, ERR_BindToBogusProp1, compiler()->ErrName(badArgsAlt->name),
                // The casts are here because we can't just bitwise-or the getter and setter (we know one is NULL)
                (METHSYM*)((INT_PTR)badArgsAlt->asPROPSYM()->methGet | (INT_PTR)badArgsAlt->asPROPSYM()->methSet));
            compiler()->clsDeclRec.AddRelatedSymbolLocations (pError, badArgsAlt);
            compiler()->SubmitError (pError);
        } else {
            compiler()->clsDeclRec.errorNameRefSymbol(badArgsAlt->name, tree, pParent, current, ERR_BindToBogus);
        }
        goto ERRORL;
    }

    // Report different errors for different problems
    if (invoke) {
        errorStrSym(tree, ERR_InvalidCall, compiler()->ErrDelegate(invoke->parent->asAGGSYM()), badArgsAlt);
        goto ERRORL;
    }

    if (origObject && origObject->kind == SK_AGGSYM && origObject->asAGGSYM()->isDelegate &&
        mp->name == compiler()->namemgr->GetPredefName(PN_INVOKE)) {
        ASSERT(mp->parent->asAGGSYM()->isDelegate);
        ASSERT(!badArgType || badArgType->parent->asAGGSYM()->isDelegate);
        UseDelegateErrors = true;
    }

    if (badArgType) {
        // Best matching overloaded method 'name' had some invalid arguments:
        if (UseDelegateErrors)
            errorSym(tree, ERR_BadDelArgTypes, badArgType->parent); // Point to the Delegate, not the Invoke method
        else
            errorSym(tree, ERR_BadArgTypes, badArgType);
        // Argument X: cannot convert type 'Y' to type 'Z'
        TYPESYM ** params = badArgType->params;
        EXPR * actualList = args;
        int curActual = 0;
        while (actualList && params) {
            EXPR * actual;
            curActual ++;
            if (actualList->kind == EK_LIST) {
                actual = actualList->asBIN()->p1;
                actualList = actualList->asBIN()->p2;
            } else {
                actual = actualList;
                actualList = NULL;
            }
            if (!canConvert(actual,*params, NULL)) {
                errorIntSymSym(actual->tree, ERR_BadArgType, curActual, actual->type, *params);
            }
            params++;
        }
    } else {
        // The number of arguments must be wrong
        NAME *pName = ((!UseDelegateErrors && (mp->kind == SK_PROPSYM || !mp->asMETHSYM()->isCtor)) ? mp->name : mp->parent->name);
        CError  *pError = make_errorNameInt(tree, UseDelegateErrors ? ERR_BadDelArgCount : ERR_BadArgCount,
            pName, cActuals);
        // Error: No overloaded method 'name' that takes cActuals arguments
        // Report possible matches (same name and is accesible)
        current = mp;

        hideByNameMethodFound = false;

        if (isInterface) {
            goto ERR_FINDFIRSTINTERFACEMETHOD;
        }

        do {

            hideByNameMethodFound |= current->isHideByName;

            // This only affects error reporting
            int cFormals;
            cFormals = current->cParams;

            if (!compiler()->clsDeclRec.checkAccess(current, pParent, NULL, qualifier)) {
                goto ERR_NEXT;
            }

            // ok, this is a plausible method to call
            compiler()->clsDeclRec.AddRelatedSymbolLocations (pError, current);

ERR_NEXT:

            if (current->isParamArray) {
                if (current->kind != SK_EXPANDEDPARAMSSYM) {
                    if (cActuals >= current->cParams - 1) {
                        current = getParamsMethod(current->asMETHSYM(), cActuals);
                        continue;
                    }
                } else {
                    current = current->asEXPANDEDPARAMSSYM()->realMethod;
                }
            }

            SYM * next;
            next = current;
            // get next non-override method or property
            while ((next = compiler()->symmgr.LookupNextSym(next, current->parent, mask)->asMETHPROPSYM())  &&
                   !isMethPropCallable(next->asMETHPROPSYM(), forceUC)) {
                /*nothing */
                ;
            }

            if (!next && (mp->kind == SK_PROPSYM || !mp->asMETHSYM()->isCtor)) {
                AGGSYM * base;
                if (isInterface) {
                    while (true) {
                        if (!newStackSlot) {
                            newStackSlot = (SYMLISTSTACK *)_alloca(sizeof(SYMLISTSTACK));
                        }
                        currentInterface = pickNextInterface(currentInterface, &traversalStack, &hideByNameMethodFound, &newStackSlot);
                        if (currentInterface) {
                            ASSERT(!currentInterface->isPrecluded);
ERR_FINDFIRSTINTERFACEMETHOD:
                            addDependancyOnType(currentInterface);
                            next = compiler()->symmgr.LookupGlobalSym(mp->name, currentInterface, MASK_ALL);
                            if (next) goto ERR_FOUNDONE;
                        } else {
                            break;
                        }
                    }
                    // now, consider methods on object...
                    base = getPDT(PT_OBJECT)->asAGGSYM();
                    isInterface = false;
                    goto ERR_DOBASE;
ERR_FOUNDONE:;
                } else if (!hideByNameMethodFound) {
                    base = current->parent->asAGGSYM()->baseClass;
ERR_DOBASE:
                    //
                    // find next base member with this name
                    // which is not an override or a hidden member of the wrong type
                    //
                    next = NULL;
                    while ((next = compiler()->clsDeclRec.findNextName(current->name, base, next))) {
                        if (!(next->mask() & mask)) {
                            // ignore inaccessible members of wrong type
                            ASSERT(!compiler()->clsDeclRec.checkAccess(next, pParent, NULL, qualifier));

                        } else if (isMethPropCallable(next->asMETHPROPSYM(), forceUC)) {

                            break;
                        }

                        // we never include overridden methods in overload resolution
                        // so keep looking
                        base = next->parent->asAGGSYM();
                    }
                    if (!next) {
                        // break out of the error loop
                        break;
                    }
                }
            }
            current = next->asMETHPROPSYM();
        } while (current);

        compiler()->SubmitError (pError);
    }

ERRORL:

    if (isInterface) {
        cleanupInterfaces(precludedInterfaces);
    }

    return NULL;

CHECKRETURN:

    if (isInterface) {
        cleanupInterfaces(precludedInterfaces);
    }

    if (current->kind == SK_EXPANDEDPARAMSSYM && current->isParamArray) {
        current = current->asEXPANDEDPARAMSSYM()->realMethod;
    }

    {
        // if this is a binding to finalize on object, then complain:

    
        if (current->name == compiler()->namemgr->GetPredefName(PN_DTOR) && current->parent == compiler()->symmgr.GetPredefType(PT_OBJECT, false)) {
            if (flags & EXF_BASECALL)
            {
                errorN(tree, ERR_CallingBaseFinalizeDeprecated);
            }
            else
            {
                errorN(tree, ERR_CallingFinalizeDepracated);
            }
        }
    }

    METHPROPSYM * newCurrent = remapToOverride(current, origObject, flags & EXF_BASECALL);

    checkStaticness(tree, newCurrent, object);

    if (object && *object && object[0]->kind == EK_FIELD) {
        checkFieldRef(object[0]->asFIELD(), false);
    }

    if (checkBogus(newCurrent)) {
        if (newCurrent->kind == SK_PROPSYM && newCurrent->useMethInstead) {
            CError  *pError;
            if (newCurrent->asPROPSYM()->methGet && newCurrent->asPROPSYM()->methSet)
                pError = make_errorStrSymSym( tree, ERR_BindToBogusProp2, newCurrent->name->text, newCurrent->asPROPSYM()->methGet, newCurrent->asPROPSYM()->methSet);
            else
                pError = make_errorStrSym( tree, ERR_BindToBogusProp1, newCurrent->name->text,
                // The casts are here because we can't just bitwise-or the getter and setter (we know one is NULL)
                (METHSYM*)((INT_PTR)newCurrent->asPROPSYM()->methGet | (INT_PTR)newCurrent->asPROPSYM()->methSet));
            compiler()->clsDeclRec.AddRelatedSymbolLocations (pError, newCurrent);
            compiler()->SubmitError (pError);
        } else {
            NAME * name = newCurrent->name;
            if (newCurrent->parent && newCurrent->parent->kind == SK_AGGSYM &&
                name == compiler()->namemgr->GetPredefName(PN_INVOKE) &&
                newCurrent->parent->asAGGSYM()->isDelegate)
                name = newCurrent->parent->name;
            compiler()->clsDeclRec.errorNameRefSymbol(name, tree, pParent, newCurrent, ERR_BindToBogus);
        }
        return NULL;
    }

    ASSERT(newCurrent->isUserCallable());

    if (newCurrent->kind == SK_METHSYM && (flags & EXF_BASECALL) && newCurrent->asMETHSYM()->isAbstract) {
        errorSym(tree, ERR_AbstractBaseCall, newCurrent);
    }

    if (current->isDeprecated) {
        reportDeprecated(tree, current);
    } else if (newCurrent->isDeprecated) {
        reportDeprecated(tree, newCurrent);
    }

    if (newCurrent->retType) {
        compiler()->symmgr.DeclareType (newCurrent->retType);    
        checkUnsafe(tree, newCurrent->retType);
        bool checkParams = false;
        if (newCurrent->kind == SK_METHSYM && newCurrent->asMETHSYM()->isExternal) {
            checkParams = true;

            AGGSYM * temp = newCurrent->retType->underlyingAggregate();
            if (temp && temp->isStruct)
                temp->hasExternReference = true;
        }
        // we need to check unsafe on the parameters as well, since we cannot check in conversion
        for (int pc = 0; pc < newCurrent->cParams; pc++) {
            ASSERT(newCurrent->params[pc]->isUnsafe() == (newCurrent->params[pc]->kind == SK_PTRSYM));
            // this is an optimization: don't call this in the vast majority of cases
            if (newCurrent->params[pc]->kind == SK_PTRSYM) {
                checkUnsafe(tree, newCurrent->params[pc]);
            }
            if (checkParams && newCurrent->params[pc]->kind == SK_PARAMMODSYM) {
                AGGSYM * temp = newCurrent->params[pc]->underlyingAggregate();
                if (temp && temp->isStruct)
                    temp->hasExternReference = true;
            }
        }
    }

    return newCurrent;
}

bool FUNCBREC::checkBogus(SYM * sym, bool bNoError, bool * pbUndeclared)
{
    if (NULL == sym)
        return false;

    if (sym->checkedBogus)
        return sym->isBogus;

    if (sym->isBogus) {
        return (sym->checkedBogus = true);
    }

    bool isBogus = false;
    bool bUndeclared = false;
    switch (sym->kind) {
    case SK_EXPANDEDPARAMSSYM:
    case SK_PROPSYM:
    case SK_METHSYM:
    case SK_FAKEPROPSYM:
    case SK_FAKEMETHSYM:
    case SK_INDEXERSYM:
    case SK_IFACEIMPLMETHSYM:
        if (sym->asMETHPROPSYM()) {
            METHPROPSYM * meth = sym->asMETHPROPSYM();
            if (checkBogus(meth->retType, bNoError, &bUndeclared)) {
                isBogus = true;
                break;
            }
            // we need to check the parameters as well, since we cannot check in conversion
            for (int pc = 0; pc < meth->cParams; pc++) {
                if (isBogus = checkBogus(meth->params[pc], bNoError, &bUndeclared))
                    break;
            }
        }
        break;

    case SK_PARAMMODSYM:
    case SK_PTRSYM:
    case SK_ARRAYSYM:
    case SK_PINNEDSYM:
        isBogus = checkBogus(sym->asTYPESYM()->underlyingAggregate(), bNoError, &bUndeclared);
        break;

    case SK_EVENTSYM:
        isBogus = checkBogus(sym->asEVENTSYM()->type, bNoError, &bUndeclared);
        break;

    case SK_MEMBVARSYM:
        isBogus = checkBogus(sym->asVARSYM()->type, bNoError, &bUndeclared);
        break;

    case SK_ERRORSYM:
        isBogus = false;
        break;

    case SK_SCOPESYM:
    case SK_NSSYM:
    case SK_NSDECLSYM:
    default:
        VSFAIL("FUNCBREC::checkBogus with invalid Symbol kind");
        // Fall-through

    case SK_VOIDSYM:
    case SK_NULLSYM:
    case SK_AGGSYM:
    case SK_LOCVARSYM:
        if (!sym->isPrepared && !bNoError)
            compiler()->symmgr.DeclareType (sym);
        bUndeclared = !sym->isPrepared;
        isBogus = sym->isBogus;
        break;
    }
    if (!bUndeclared || isBogus)
        // Only set this if everything is declared or
        // at least 1 declared thing is bogus
        sym->checkedBogus = true;

    if (pbUndeclared)
        *pbUndeclared |= bUndeclared;
    return sym->isBogus |= isBogus;
}


METHPROPSYM * FUNCBREC::remapToOverride(METHPROPSYM * mp, TYPESYM * object, int baseCallOverride)
{

    if (mp->parent != getPDO() && !baseCallOverride) return mp;

    if (!object || (object->kind == SK_AGGSYM && object->asAGGSYM()->isInterface)) return mp;

    if (mp->kind == SK_METHSYM) {
        if (!mp->asMETHSYM()->isVirtual) return mp;
    } else {
        if ((mp->asPROPSYM()->methGet && !mp->asPROPSYM()->methGet->isVirtual) ||
            (mp->asPROPSYM()->methSet && !mp->asPROPSYM()->methSet->isVirtual)) {

            return mp;
        }
    }

    int mask = mp->mask();

    TYPESYM * qualifier = baseCallOverride ? NULL : object;

    while (object && object != mp->parent) {
        METHPROPSYM * remap = compiler()->symmgr.LookupGlobalSym(mp->name, object, mask)->asMETHPROPSYM();
        while (remap) {
            if (remap->isOverride &&
                remap->retType == mp->retType &&
                remap->params == mp->params &&
                (remap->kind != SK_METHSYM || remap->asMETHSYM()->isUserCallable()) &&
                compiler()->clsDeclRec.checkAccess(remap, pParent, NULL, qualifier)) {
                return remap;
            }
            remap = compiler()->symmgr.LookupNextSym(remap, object, mask)->asMETHPROPSYM();
        }
        if (object->kind != SK_AGGSYM)
            break;
        object = object->asAGGSYM()->baseClass;
    }

    return mp;

}


EXPRBLOCK * FUNCBREC::newExprBlock(BASENODE * tree)
{
    EXPRBLOCK * block = newExpr(tree, EK_BLOCK, NULL)->asBLOCK();
    block->owningBlock = pCurrentBlock;
    block->scopeSymbol = NULL;
    block->exitBitset = NULL;
    block->enterBitset = NULL;
    block->exitReachable = false;
    block->redoBlock = true;
    block->scanning = false;

    return block;
}


EXPRCONSTANT * FUNCBREC::newExprConstant(BASENODE * tree, TYPESYM * type, CONSTVAL cv)
{
    EXPRCONSTANT * rval = newExpr(tree, EK_CONSTANT, type)->asCONSTANT();
    rval->list = NULL;
    rval->pList = &(rval->list);
    rval->getSVal().init = cv.init; // Always big enough for everything

    return rval;
}

// Create an error node
EXPRERROR * FUNCBREC::newError(BASENODE * tree)
{
    EXPRERROR * expr;
    ASSERT(tree != NULL);
    expr = newExpr(tree, EK_ERROR, compiler()->symmgr.GetErrorSym())->asERROR();
    expr->extraInfo = ERROREXTRA_NONE;
    return expr;
}

// Create a generic node...
EXPR * FUNCBREC::newExpr(PBASENODE tree, EXPRKIND kind, TYPESYM * type)
{

    // in debug, allocate through new so that we get a vtable for the debugger
    // else, just allocate enough space and use that...
#if DEBUG
    EXPR * rval;
    switch (kind) {
#define EXPRDEF(e) case EK_##e: rval = new (allocator) EXPR##e; break;
#include "exprkind.h"
#undef EXPRDEF
        default:    ASSERT(!"bad kind passed to newExpr"); rval = new (allocator) EXPR;
    }

#else // debug
    unsigned size;

    switch (kind) {
#define EXPRDEF(e) case EK_##e: size = sizeof (EXPR##e); break;
#include "exprkind.h"
#undef EXPRDEF
        default:    ASSERT(!"bad kind passed to newExpr"); size = sizeof(EXPR);
    }

    EXPR * rval = (EXPR*) allocator->Alloc(size);
#endif // debug

    rval->kind = kind;
    rval->type = type;
    rval->tree = tree;
    rval->flags = (EXPRFLAG) 0;

    return rval;

}

EXPRSTMTAS * FUNCBREC::newExprStmtAs(BASENODE * tree, EXPR * expr)
{
    EXPRSTMTAS * rval = newExpr(tree, EK_STMTAS, NULL)->asSTMTAS();
    rval->expression = expr;
    return rval;
}

EXPRWRAP * FUNCBREC::newExprWrap(EXPR * wrap, TEMP_KIND tempKind)
{
    EXPRWRAP * rval = newExpr(EK_WRAP)->asWRAP();
    rval->asWRAP()->expr = wrap;
    rval->type = wrap->type;
    rval->asWRAP()->slot = NULL;
    rval->asWRAP()->doNotFree = false;
    rval->asWRAP()->needEmptySlot = false;
    rval->asWRAP()->pinned = false;
    rval->flags |= EXF_LVALUE;
    rval->tempKind = tempKind;
    return rval;
}

// Create a binop node...
EXPRBINOP * FUNCBREC::newExprBinop(PBASENODE tree, EXPRKIND kind, TYPESYM *type, EXPR *p1, EXPR *p2)
{
    EXPRBINOP * rval = (EXPRBINOP*) newExpr(tree, EK_BINOP, type);
    rval->p1 = p1;
    rval->p2 = p2;
    rval->kind = kind;
    rval->flags = EXF_BINOP;

    return rval->asBIN();
}

// create a new label expr statment...
EXPRLABEL * FUNCBREC::newExprLabel(bool userDefined)
{
    EXPRLABEL * rval = newExpr(NULL, EK_LABEL, NULL)->asLABEL();
    rval->block = NULL;
    rval->owningBlock = pCurrentBlock;
    rval->scope = pCurrentScope;
    rval->entranceBitset = NULL;
    rval->checkedDupe = false;
    rval->reachedByBreak = false;
    rval->definitelyReachable = false;
    rval->userDefined = userDefined;
    // the second part of this assert checks that pCB is not garbage by dereferencing it...
    ASSERT(pCurrentBlock && !pCurrentBlock->type);
    ASSERT(pCurrentScope && pCurrentScope->parent);

    return rval;
}

EXPRZEROINIT * FUNCBREC::newExprZero(BASENODE * tree, EXPR * op)
{
    EXPRZEROINIT * rval = newExpr(tree, EK_ZEROINIT, op->type)->asZEROINIT();
    rval->p1 = op;
    return rval;
}

EXPRZEROINIT * FUNCBREC::newExprZero(BASENODE * tree, TYPESYM * type)
{
#if DEBUG
    if (type->kind != SK_ERRORSYM) {
        ASSERT(type->kind == SK_AGGSYM && (type->asAGGSYM()->isStruct || type->asAGGSYM()->isEnum));
    }
#endif
    EXPRZEROINIT * rval = newExpr(tree, EK_ZEROINIT, type)->asZEROINIT();
    rval->p1 = NULL;
    return rval;
}

// Concatenate an item onto a list
void FUNCBREC::newList(EXPR * newItem, EXPR *** list) 
{
    ASSERT(list != NULL);
    ASSERT(*list != NULL);

    if (!newItem) return;

    if (**list == NULL) {
        **list = newItem;
        return;
    }
    EXPRBINOP * temp = newExprBinop(
        NULL,
        EK_LIST,
        getVoidType(),
        (**list),
        newItem);
    (**list) = temp;
    (*list) = &(temp->p2);
}

unsigned EXPR::isDefValue()
{

    if (kind == EK_CONSTANT) {
        switch (type->fundType()) {
        case FT_REF:
            return !(asCONSTANT()->getSVal().strVal);
        case FT_I8:
        case FT_U8:
            return *(asCONSTANT()->getVal().longVal) == 0;
        case FT_R4:
        case FT_R8:
            return *(asCONSTANT()->getVal().doubleVal) == 0.0; // ?????? will this ever hit?
        default:
            return !asCONSTANT()->getVal().iVal;
        }
    } else {
        return 0;
    }
}



bool EXPR::hasSideEffects(COMPILER *compiler)
{

    EXPR * object;

    if (flags & (EXF_ASSGOP | EXF_CHECKOVERFLOW)) {
        return true;
    }
    if (flags & EXF_BINOP) {
        if (asBIN()->p1->hasSideEffects( compiler)) {
            return true;
        }
        if (asBIN()->p2)
            return asBIN()->p2->hasSideEffects( compiler);
        else
            return false;
    }

    switch (kind) {
    case EK_ZEROINIT:
        object = asZEROINIT()->p1;
        return object && (object->kind != EK_LOCAL || object->asLOCAL()->local->slot.type);
    case EK_PROP:
    case EK_CONCAT:
    case EK_CALL:
        return true;
    case EK_NOOP:
        return false;
    case EK_ARRINIT:
        if (asARRINIT()->args)
            return asARRINIT()->args->hasSideEffects( compiler);
        else
            return false;
    case EK_FIELD:
        object = asFIELD()->object;
        ASSERT(compiler != NULL);
        if (object && object->type->fundType() == FT_REF && !compiler->funcBRec.isThisPointer(object)) {
            return true;
        }
        if (object)
            return object->hasSideEffects( compiler);
        else
            return true; // a static field has the sideeffect of loading the cctor
	case EK_WRAP:
		return asWRAP()->expr->hasSideEffects(compiler);
    case EK_CAST:
        return (flags & (EXF_BOX | EXF_UNBOX | EXF_REFCHECK | EXF_CHECKOVERFLOW)) ? true : false;
    case EK_LOCAL:
    case EK_CONSTANT:
    case EK_FUNCPTR:
    case EK_TYPEOF:
        return false;
    default:
        ASSERT(!"bad expr");
        return true;
    }
}

LOCSLOTINFO * FUNCBREC::getThisPointerSlot()
{
    if (thisPointer) {
        return &(thisPointer->slot);
    } else {
        return NULL;
    }
}

__forceinline NAME * FUNCBREC::ekName(EXPRKIND ek)
{
    ASSERT(ek >= EK_FIRSTOP && ek <= EK_BITNOT);
    return compiler()->namemgr->GetPredefName(EK2NAME[ek - EK_FIRSTOP]);
}


const EXPRKIND FUNCBREC::OP2EK[] = {
#define OP(n,p,a,stmt,t,pn,e) (EXPRKIND) (e),
#include "ops.h"
#undef OP
};

const BOOL FUNCBREC::opCanBeStatement[] = {
#define OP(n,p,a,stmt,t,pn,e) stmt,
#include "ops.h"
#undef OP
};

const PREDEFNAME FUNCBREC::EK2NAME[] = {

    PN_OPEQUALS,
    PN_OPCOMPARE,

    PN_OPTRUE,
    PN_OPFALSE,

    PN_OPNEGATION,

    PN_OPINCREMENT,
    PN_OPDECREMENT,

    PN_OPEQUALITY,
    PN_OPINEQUALITY,
    PN_OPLESSTHAN,
    PN_OPLESSTHANOREQUAL,
    PN_OPGREATERTHAN,
    PN_OPGREATERTHANOREQUAL,

    PN_OPPLUS,
    PN_OPMINUS,
    PN_OPMULTIPLY,
    PN_OPDIVISION,
    PN_OPMODULUS,

    PN_OPBITWISEAND,
    PN_OPBITWISEOR,
    PN_OPXOR,
    PN_OPLEFTSHIFT,
    PN_OPRIGHTSHIFT,
    PN_OPUNARYMINUS,
    PN_OPUNARYPLUS,
    PN_OPCOMPLEMENT,
};
