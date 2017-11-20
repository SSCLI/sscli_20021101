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
// File: clsdrec.cpp
//
// Routines for declaring a class
// ===========================================================================
#include "stdafx.h"

#include "compiler.h"
#include "fileiter.h"

// Tokens which correspond to the parser flags for the various item modifiers
// This needs to be kept in ssync with the list in nodes.h
const TOKENID CLSDREC::accessTokens[] = {
    TID_ABSTRACT,
    TID_NEW,
    TID_OVERRIDE,
    TID_PRIVATE,
    TID_PROTECTED,
    TID_INTERNAL,
    TID_PUBLIC,
    TID_SEALED,
    TID_STATIC,
    TID_VIRTUAL,
    TID_EXTERN,
    TID_READONLY,
    TID_VOLATILE,
    TID_UNSAFE,

};

//
// class level init
//
CLSDREC::CLSDREC() 
{
}


CError *CLSDREC::make_errorSymbol(SYM *symbol, int id)
{
    // if this ever hits, find the offender
    // and have them pass in a NSDECLSYM instead
    ASSERT(symbol->kind != SK_NSSYM);

    return make_errorStrFile (symbol->getParseTree(), symbol->getInputFile(), id, compiler()->ErrSym(symbol));
}

void CLSDREC::errorSymbol(SYM *symbol, int id)
{
    compiler()->SubmitError (make_errorSymbol (symbol, id));
}

void CLSDREC::AddRelatedSymbolLocations (CError *pError, SYM *relatedSymbol)
{
    if (relatedSymbol->kind == SK_NSSYM) {
        // we have a namespace declaration
        // dump all declarations of it
        NSDECLSYM *decl = relatedSymbol->asNSSYM()->firstDeclaration();
        while (decl) {
            AddRelatedSymbolLocations (pError, decl);
            decl = decl->nextDeclaration;
        }
    } else {
        compiler()->AddLocationToError (pError, ERRLOC (relatedSymbol->getInputFile(), relatedSymbol->getParseTree()));
    }
}

void CLSDREC::errorSymbolStr(SYM *symbol, int id, LPCWSTR str)
{
    // if this hits then do the same as in errorSymbol()
    ASSERT(symbol->kind != SK_NSSYM);

    errorStrStrFile(symbol->getParseTree(), symbol->getInputFile(), id, compiler()->ErrSym(symbol), str);
}

CError *CLSDREC::make_errorSymbolStr(SYM *symbol, int id, LPCWSTR str1)
{
    return make_errorStrStrFile (symbol->getParseTree(), symbol->getInputFile(), id, compiler()->ErrSym(symbol), str1);
}


CError *CLSDREC::make_errorSymbolStrStr(SYM *symbol, int id, LPCWSTR str1, LPCWSTR str2)
{
    return make_errorStrStrStrFile (symbol->getParseTree(), symbol->getInputFile(), id, compiler()->ErrSym(symbol), str1, str2);
}

void CLSDREC::errorSymbolStrStr(SYM *symbol, int id, LPCWSTR str1, LPCWSTR str2)
{
    // if this hits then do the same as in errorSymbol()
    ASSERT(symbol->kind != SK_NSSYM);

    errorStrStrStrFile(symbol->getParseTree(), symbol->getInputFile(), id, compiler()->ErrSym(symbol), str1, str2);
}


void CLSDREC::errorSymbolSymbolSymbol(SYM *symbol, SYM *relatedSymbol1, SYM *relatedSymbol2, int id)
{
    // if this hits then do the same as in errorSymbol()
    ASSERT(symbol->kind != SK_NSSYM);

    CError  *pError = make_errorStrStrStrFile (symbol->getParseTree(), symbol->getInputFile(), id, compiler()->ErrSym(symbol), compiler()->ErrSym(relatedSymbol1), compiler()->ErrSym(relatedSymbol2));
    AddRelatedSymbolLocations (pError, relatedSymbol1);
    AddRelatedSymbolLocations (pError, relatedSymbol2);
    compiler()->SubmitError (pError);
}

void CLSDREC::errorSymbolAndRelatedSymbol(SYM *symbol, SYM *relatedSymbol, int id)
{
    // if this hits then do the same as in errorSymbol()
    ASSERT(symbol->kind != SK_NSSYM);

    CError  *pError = make_errorStrStrFile (symbol->getParseTree(), symbol->getInputFile(), id, compiler()->ErrSym(symbol), compiler()->ErrSym(relatedSymbol));
    AddRelatedSymbolLocations (pError, relatedSymbol);
    compiler()->SubmitError (pError);
}


void CLSDREC::errorNamespaceAndRelatedNamespace(NSSYM *NSsym, NSSYM *relatedNS, int id)
{
    // if this hits then do the same as in errorSymbol()
    ASSERT(NSsym->kind == SK_NSSYM);

    
    NSDECLSYM *decl = NSsym->asNSSYM()->firstDeclaration();
    CError  *pError = make_errorStrStrFile (decl->getParseTree(), decl->getInputFile(), id, compiler()->ErrSym(decl), compiler()->ErrSym(relatedNS));
    decl = decl->nextDeclaration;
    while (decl) {
        AddRelatedSymbolLocations (pError, decl);
        decl = decl->nextDeclaration;
    }
    AddRelatedSymbolLocations (pError, relatedNS);
    compiler()->SubmitError (pError);
}


void CLSDREC::errorType(TYPENODE *type, PPARENTSYM refContext, int id)
{
    errorStrFile(type, refContext->getInputFile(), id, compiler()->ErrTypeNode(type));
}

void CLSDREC::errorTypeSymbol(TYPENODE *type, PPARENTSYM refContext, SYM *relatedSymbol, int id)
{
    CError  *pError = make_errorStrFile (type, refContext->getInputFile(), id, compiler()->ErrTypeNode(type));
    AddRelatedSymbolLocations (pError, relatedSymbol);
    compiler()->SubmitError (pError);
}

void CLSDREC::errorNameRef(BASENODE *name, PPARENTSYM refContext, int id)
{
    errorStrFile(name, refContext->getInputFile(), id, compiler()->ErrNameNode(name));
}

void CLSDREC::errorNameRef(NAME *name, BASENODE *parseTree, PPARENTSYM refContext, int id)
{
    errorStrFile(parseTree, refContext->getInputFile(), id, compiler()->ErrName(name));
}

void CLSDREC::errorNameRefSymbol(BASENODE *name, PPARENTSYM refContext, SYM *relatedSymbol, int id)
{
    CError  *pError = make_errorStrFile (name, refContext->getInputFile(), id, compiler()->ErrNameNode(name));
    AddRelatedSymbolLocations (pError, relatedSymbol);
    compiler()->SubmitError (pError);
}

void CLSDREC::errorNameRefSymbol(NAME *name, BASENODE *parseTree, PPARENTSYM refContext, SYM *relatedSymbol, int id)
{
    CError  *pError = make_errorStrFile (parseTree, refContext->getInputFile(), id, compiler()->ErrName(name));
    AddRelatedSymbolLocations (pError, relatedSymbol);
    compiler()->SubmitError (pError);
}

void CLSDREC::errorNameRefStr(BASENODE *name, PPARENTSYM refContext, LPCWSTR string, int id)
{
    errorStrStrFile(name, refContext->getInputFile(), id, compiler()->ErrNameNode(name), string);
}

void CLSDREC::errorNameRefStr(NAME *name, BASENODE *parseTree, PPARENTSYM refContext, LPCWSTR string, int id)
{
    errorStrStrFile(parseTree, refContext->getInputFile(), id, compiler()->ErrName(name), string);
}

void CLSDREC::errorNameRefStrSymbol(BASENODE *name, PPARENTSYM refContext, LPCWSTR string, SYM *relatedSymbol, int id)
{
    CError  *pError = make_errorStrStrFile (name, refContext->getInputFile(), id, compiler()->ErrNameNode(name), string);
    AddRelatedSymbolLocations (pError, relatedSymbol);
    compiler()->SubmitError (pError);
}

void CLSDREC::errorNameRefStrSymbol(NAME *name, BASENODE *parseTree, PPARENTSYM refContext, LPCWSTR string, SYM *relatedSymbol, int id)
{
    CError  *pError = make_errorStrStrFile (parseTree, refContext->getInputFile(), id, compiler()->ErrName(name), string);
    AddRelatedSymbolLocations (pError, relatedSymbol);
    compiler()->SubmitError (pError);
}

void CLSDREC::errorNameRefSymbolSymbol(NAME *name, BASENODE *node, PPARENTSYM refContext, SYM *relatedSymbol1, SYM *relatedSymbol2, int id)
{
    CError  *pError = make_errorStrFile (node, refContext->getInputFile(), id, compiler()->ErrName(name));
    AddRelatedSymbolLocations (pError, relatedSymbol1);
    AddRelatedSymbolLocations (pError, relatedSymbol2);
    compiler()->SubmitError (pError);
}

CError *CLSDREC::make_errorStrStrSymbol(BASENODE *location, PINFILESYM inputFile, LPCWSTR str1, LPCWSTR str2, SYM *symbol, int id)
{
    CError  *pError = make_errorStrStrFile (location, inputFile, id, str1, str2);
    AddRelatedSymbolLocations (pError, symbol);
    return pError;
}


CError *CLSDREC::make_errorStrSymbol(BASENODE *location, PINFILESYM inputFile, LPCWSTR str, SYM *symbol, int id)
{
    CError  *pError = make_errorStrFile (location, inputFile, id, str);
    AddRelatedSymbolLocations (pError, symbol);
    return pError;
}

void CLSDREC::errorStrSymbol(BASENODE *location, PINFILESYM inputFile, LPCWSTR str, SYM *symbol, int id)
{
    compiler()->SubmitError (make_errorStrSymbol (location, inputFile, str, symbol, id));
}

void CLSDREC::errorFile(BASENODE *tree, PINFILESYM inputfile, int id)
{
    ASSERT(tree && inputfile && inputfile->pData);
    CError  *pError = compiler()->MakeError (id);
    compiler()->AddLocationToError (pError, ERRLOC(inputfile, tree));
    compiler()->SubmitError (pError);
}

void CLSDREC::errorFileSymbol(BASENODE *tree, PINFILESYM inputfile, int id, SYM *sym)
{
    CError  *pError = make_errorStrFile(tree, inputfile, id, compiler()->ErrSym(sym));
    AddRelatedSymbolLocations (pError, sym);
    compiler()->SubmitError (pError);
}


CError *CLSDREC::make_errorStrFile(BASENODE *tree, PINFILESYM inputfile, int id, LPCWSTR str)
{
    CError  *pError = compiler()->MakeError (id, str);
    compiler()->AddLocationToError (pError, ERRLOC(inputfile, tree));
    return pError;
}

void CLSDREC::errorStrFile(BASENODE *tree, PINFILESYM inputfile, int id, LPCWSTR str)
{
    compiler()->SubmitError (make_errorStrFile (tree, inputfile, id, str));
}


CError *CLSDREC::make_errorStrStrFile(BASENODE *tree, PINFILESYM inputfile, int id, LPCWSTR str1, LPCWSTR str2)
{
    CError  *pError = compiler()->MakeError (id, str1, str2);
    compiler()->AddLocationToError (pError, ERRLOC(inputfile, tree));
    return pError;
}

void CLSDREC::errorStrStrFile(BASENODE *tree, PINFILESYM inputfile, int id, LPCWSTR str1, LPCWSTR str2)
{
    compiler()->SubmitError (make_errorStrStrFile (tree, inputfile, id, str1, str2));
}


CError *CLSDREC::make_errorStrStrStrFile(BASENODE *tree, PINFILESYM inputfile, int id, LPCWSTR str1, LPCWSTR str2, LPCWSTR str3)
{
    CError  *pError = compiler()->MakeError (id, str1, str2, str3);
    compiler()->AddLocationToError (pError, ERRLOC(inputfile, tree));
    return pError;
}

void CLSDREC::errorStrStrStrFile(BASENODE *tree, PINFILESYM inputfile, int id, LPCWSTR str1, LPCWSTR str2, LPCWSTR str3)
{
    compiler()->SubmitError (make_errorStrStrStrFile (tree, inputfile, id, str1, str2, str3));
}

// Issue an error if the given type is unsafe. This function should NOT be called if the type
// was used in an unsafe context -- this function doesn't check that we're in an unsafe context.
void CLSDREC::checkUnsafe(BASENODE * tree, PINFILESYM inputfile, TYPESYM * type)
{
    if (type->isUnsafe()) {
        errorFile(tree, inputfile, ERR_UnsafeNeeded);
    }
}



// return whether target can be accessed from within a given place.
// if not, then save if in badAccess for a better error message later...
//
//  target  - the symbol to check access on
//  place   - the location that target is being referenced from
//            NOTE: could be a class or a namespace
//  badAccess   - save target in baddAccess if lookup fails
//
// never displays an error message
bool CLSDREC::checkAccess(SYM * target, PARENTSYM * place, SYM ** badAccess, SYM * qualifier)
{

TRYAGAIN:
    switch (target->access) {
    case ACC_PUBLIC :
        return true;

    case ACC_PRIVATE :
        //
        // check if place is this class, or a nested class
        //
        if (place->kind == SK_AGGSYM)
        {
            AGGSYM * targetClass = target->parent->asAGGSYM();
            do
            {
                AGGSYM * cls = place->asAGGSYM();
                if (cls == targetClass) {
                    return true;
                }

                place = place->parent;
            } while (place->kind == SK_AGGSYM);
        }
        break;

    case ACC_INTERNAL:
    case ACC_INTERNALPROTECTED:  // Check internal, then protected.
        //
        // ensure we add the inputfile ref to the assembly we're emiting
        //
        target->getInputFile()->hasBeenRefed = true;
        place->getInputFile()->hasBeenRefed = true;
        if (target->getAssemblyIndex() == place->getAssemblyIndex()) {
            return true;
        }
        
        if (target->access == ACC_INTERNAL)
            break;
        // else == ACC_INTERNALPROTECTED:
        //    FALL THROUGH to protected case

    case ACC_PROTECTED :
        //
        // check if place is nested class of target
        // or place is nested class of a more derived class of target
        //
        if (place->kind == SK_AGGSYM)
        {
            bool isStatic;

            switch (target->kind) {
            case SK_MEMBVARSYM:
                isStatic = target->asMEMBVARSYM()->isStatic; break;
            case SK_EVENTSYM:
                isStatic = target->asEVENTSYM()->isStatic; break;
            case SK_AGGSYM:
                isStatic = true; break;
            default:
                isStatic = target->asMETHPROPSYM()->isStatic; break;
            }

            //
            // Static protected access must have qualifier be a the type of the place
            // or a derived class
            // a NULL qualifer means 'this' or 'base' expression which is always OK
            //
            if (qualifier && !isStatic && qualifier->kind == SK_AGGSYM)
            {
                // Target is either this place or out classes of a nested class
                AGGSYM * targetClass = place->asAGGSYM();
                while (true)
                {
                    AGGSYM * cls = qualifier->asAGGSYM();
                    //
                    // check if qualifier is a derived class of target
                    //
                    do
                    {
                        if (cls == targetClass) {
                            goto CHECK_BASES;
                        }
                        cls = cls->baseClass;
                    } while (cls);

                    if (targetClass->parent->kind != SK_AGGSYM)
                        break;
                    targetClass = targetClass->parent->asAGGSYM();
                };
                break;
            }

CHECK_BASES:
            AGGSYM * targetClass = target->parent->asAGGSYM();
            do 
            {
                AGGSYM * cls = place->asAGGSYM();
                //
                // check if place is the target class or a derived class
                //
                do 
                {
                    if (cls == targetClass) {
                        return true;
                    }
                    cls = cls->baseClass;
                } while (cls);

                place = place->parent;
            } while (place->kind == SK_AGGSYM);
        }
        break;

    case ACC_UNKNOWN:
        // If we don't know, declare the type to find out.
        if (target->isPrepared)
            break;
        // target must be an imported type
        resolveInheritanceRec(target->asAGGSYM());
        ASSERT (target->asAGGSYM()->hasResolvedBaseClasses);
        goto TRYAGAIN;

    default:
        ASSERT(!"Bad access level");
    }

    if (badAccess && !*badAccess) {
        *badAccess = target;
    }
    return false;
}

bool CLSDREC::checkForBadMember(NAME *name, BASENODE *parseTree, AGGSYM *cls)
{
    //
    // check for name same as that of parent aggregate
    //
    if (name == cls->name && !cls->isEnum) {
        errorNameRefSymbol(name, parseTree, cls, cls, ERR_MemberNameSameAsType);
        return false;
    }
    
    return checkForDuplicateSymbol(name, parseTree, cls);
}

bool CLSDREC::checkForBadMember(NAME *name, SYMKIND symkind, PTYPESYM *params, BASENODE *parseTree, AGGSYM *cls)
{
    ASSERT(symkind == SK_PROPSYM || symkind == SK_METHSYM);

    //
    // check for name same as that of parent aggregate
    //
    if (name == cls->name && !cls->isInterface) {
        errorNameRefSymbol(name, parseTree, cls, cls, ERR_MemberNameSameAsType);
        return false;
    }
    
    return checkForDuplicateSymbol(name, symkind, params, parseTree, cls);
}

bool CLSDREC::checkForBadMember(NAMENODE *nameNode, SYMKIND symkind, PTYPESYM *params, AGGSYM *cls)
{
    ASSERT(symkind == SK_PROPSYM || symkind == SK_METHSYM);
    return checkForBadMember(nameNode->pName, symkind, params, nameNode, cls);
}

bool CLSDREC::checkForDuplicateSymbol(NAME *name, BASENODE *parseTree, AGGSYM *cls)
{
    if (name) {
        SYM* present = compiler()->symmgr.LookupGlobalSym(name, cls, MASK_ALL);
        if (present) {
            errorNameRefStrSymbol(
                name,
                parseTree, 
                cls, 
                compiler()->ErrSym(cls), 
                present, 
                ERR_DuplicateNameInClass);
            return false;
        }
    }

    return true;
}

bool CLSDREC::checkForDuplicateSymbol(NAME *name, SYMKIND symkind, PTYPESYM *params, BASENODE *parseTree, AGGSYM *cls)
{

    ASSERT(symkind == SK_PROPSYM || symkind == SK_METHSYM);
    if (name) {
        SYM* present = compiler()->symmgr.LookupGlobalSym(name, cls, MASK_ALL);
        if (present) {
            BOOL hasRefArgs = false;
            if (symkind == SK_METHSYM) {
                for (unsigned i = 0; i < SYMMGR::NumberOfParams(params); i += 1) {
                    if (params[i]->kind == SK_PARAMMODSYM) {
                        hasRefArgs = true;
                        break;
                    }
                }
            }
            while (present) {
                METHPROPSYM *mpSym;
                if ((present->kind != symkind) || ((mpSym = present->asMETHPROPSYM())->params == params) ||
                    // check for varargs with matching signature prefix
                    (mpSym->isVarargs && mpSym->cParams < SYMMGR::NumberOfParams(params) &&
                     !memcmp(mpSym->params, params, mpSym->cParams * sizeof(*params)))) {
                    if (name == compiler()->namemgr->GetPredefName(PN_CTOR) ||
                        name == compiler()->namemgr->GetPredefName(PN_STATCTOR)) {

                        name = cls->name;
                    } else if (parseTree->kind == NK_DTOR && present->kind == SK_METHSYM && present->asMETHSYM()->isDtor) {
                        // If we have 2 destructors, report it as a duplicate destructor
                        // otherwise report it as a duplicate Finalize()
                        WCHAR temp[MAX_IDENT_SIZE];
                        temp[0] = L'~';
                        wcscpy(temp + 1, cls->name->text);
                        name = compiler()->namemgr->AddString(temp);
                    }
                    errorNameRefStrSymbol(
                        name,
                        parseTree, 
                        cls, 
                        compiler()->ErrSym(cls), 
                        present, 
                        ((present->kind == symkind) ? ERR_MemberAlreadyExists : ERR_DuplicateNameInClass));
                    return false;
                } else if (present->kind == SK_METHSYM && present->asMETHSYM()->cParams == SYMMGR::NumberOfParams(params)) {
                    // check for overloading on ref and out
                    unsigned i;
                    for (i = 0; i < SYMMGR::NumberOfParams(params); i += 1) {
                        TYPESYM *t1 = params[i];
                        if (t1->kind == SK_PARAMMODSYM && t1->asPARAMMODSYM()->isOut) { 
                            t1 = compiler()->symmgr.GetParamModifier(t1->asPARAMMODSYM()->paramType(), false);
                        }
                        TYPESYM *t2 = present->asMETHSYM()->params[i];
                        if (t2->kind == SK_PARAMMODSYM && t2->asPARAMMODSYM()->isOut) { 
                            t2 = compiler()->symmgr.GetParamModifier(t2->asPARAMMODSYM()->paramType(), false);
                        }
                        if (t1 != t2) {
                            break;
                        }
                    }

                    if (i == SYMMGR::NumberOfParams(params)) {
                        errorNameRefSymbol(
                            name,
                            parseTree, 
                            cls, 
                            present, 
                            ERR_OverloadRefOut);
                        return false;
                    }
                }

                present = compiler()->symmgr.LookupNextSym(present, present->parent, MASK_ALL);
            }
        }
    }

    return true;
}

bool CLSDREC::checkForDuplicateSymbol(NAMENODE *nameNode, SYMKIND symkind, PTYPESYM *params, AGGSYM *cls)
{
    ASSERT(symkind == SK_PROPSYM || symkind == SK_METHSYM);
    return checkForDuplicateSymbol(nameNode->pName, symkind, params, nameNode, cls);
}

// Add a aggregate symbol to the given parent.  Checks for collisions and 
// displays and error if so. Does NOT declare nested types, or other aggregate members.
//
//      aggregateNode   is either a CLASSNODE or a DELEGATENODE
//      nameNode        is the NAMENODE of the aggregate to be declared
//      parent          is the containing declaration for the new aggregate
AGGSYM * CLSDREC::addAggregate(BASENODE *aggregateNode, NAMENODE* nameNode, PARENTSYM* parent)
{
    ASSERT(aggregateNode->kind == NK_CLASS  || aggregateNode->kind == NK_DELEGATE ||
           aggregateNode->kind == NK_STRUCT || aggregateNode->kind == NK_INTERFACE ||
           aggregateNode->kind == NK_ENUM);

    NAME * ident = nameNode->pName;
    PARENTSYM * parentScope = parent->getScope();
    AGGSYM *cls = NULL;

    //
    // check for name same as that of parent and duplicate name
    //
    if (parent->kind == SK_AGGSYM) {
        if (!parent->getInputFile()->hasChanged) {
            // incremental rebuild, matching existing symbols to parse trees
            SYM* present = compiler()->symmgr.LookupGlobalSym(ident, parentScope, MASK_ALL);
            if (present && present->kind == SK_AGGSYM) {
                // the flags may not be fully set yet from the import, so reset them here
                cls = present->asAGGSYM();
                cls->isClass    = false;
                cls->isEnum     = false;
                cls->isStruct   = false;
                cls->isDelegate = false;

                goto SET_FLAGS;
            }
        }
        if (!checkForBadMember(ident, nameNode, parent->asAGGSYM())) {
            return NULL;
        }
    } else {
        SYM* present = compiler()->symmgr.LookupGlobalSym(ident, parentScope, MASK_ALL);
        if (present) {
            if (!parent->getInputFile()->hasChanged) {
                // this is the incremental scenario
                // we're matching symbols to parse trees
                cls = present->asAGGSYM();

                // unlink from old nsdecl
                NSDECLSYM *oldParent = cls->declaration->asNSDECLSYM();
                SYM **oldChildLoc = &oldParent->firstChild;
                while (*oldChildLoc != cls) {
                    oldChildLoc = &((*oldChildLoc)->nextChild);
                }
                *oldChildLoc = (*oldChildLoc)->asAGGSYM()->nextChild;

                // link into new nsdecl
                cls->declaration = parent;
                if (parent->lastChild) {
                    parent->lastChild->nextChild = cls;
                    parent->lastChild = cls;
                }
                else {
                    parent->firstChild = parent->lastChild = cls;
                }
                cls->nextChild = NULL;

                // the flags may not be fully set yet from the import, so reset them here
                cls->isClass    = false;
                cls->isEnum     = false;
                cls->isStruct   = false;
                cls->isDelegate = false;

                goto SET_FLAGS;
            }
            errorNameRefStrSymbol(
                nameNode, 
                parent, 
                compiler()->ErrSym(parent->getScope()), 
                present->firstDeclaration(), 
                ERR_DuplicateNameInNS);
            return NULL;
        }
    }

    //
    // create new aggregate
    //
    cls = compiler()->symmgr.CreateAggregate(ident, parent);

SET_FLAGS:

    cls->parseTree = aggregateNode;


    //
    // check modifiers, set flags
    //
    unsigned allowableFlags = NF_MOD_PUBLIC | NF_MOD_INTERNAL;
    switch (aggregateNode->kind) {
    case NK_CLASS:
        // classes can be sealed or abstract
        allowableFlags |= NF_MOD_SEALED | NF_MOD_ABSTRACT | NF_MOD_UNSAFE;
        cls->isClass = true;
        break;
    case NK_STRUCT:
        // we have a struct
        cls->isStruct = true;
        cls->isSealed = true;
        allowableFlags |= NF_MOD_UNSAFE;
        break;
    case NK_INTERFACE:
        cls->isInterface = true;
        cls->isAbstract = true;
        allowableFlags |= NF_MOD_UNSAFE;
        break;
    case NK_ENUM:
        cls->isEnum = true;
        cls->isSealed = true;
        break;
    case NK_DELEGATE:
        cls->isDelegate = true;
        cls->isSealed = true;
        allowableFlags |= NF_MOD_UNSAFE;
        break;
    default:
        ASSERT(!"Unrecognized aggregate parse node");
    }
    if (parent->kind == SK_AGGSYM) {
        // nested classes can have private access
        // classes in a namespace can only have public or assembly access
        //
        // also nested classes can be marked new
        allowableFlags |= NF_MOD_NEW | NF_MOD_PRIVATE;

        // only class members can be protected
        if (parent->asAGGSYM()->isClass) {
            allowableFlags |= NF_MOD_PROTECTED;
        }
    }
    checkFlags(cls, allowableFlags, aggregateNode->flags, true);
    if (aggregateNode->flags & NF_MOD_SEALED) {
        cls->isSealed = true;
    }
    if (aggregateNode->flags & NF_MOD_ABSTRACT) {
        cls->isAbstract = true;
    }

    return cls;
}

NSDECLSYM * CLSDREC::addNamespaceDeclaration(NAMENODE* name, NAMESPACENODE *parseTree, NSDECLSYM *containingDeclaration)
{
    //
    // create the namespace symbol
    //
    NSSYM *nspace = addNamespace(name, containingDeclaration);
    if (!nspace) {
        return NULL;
    }

    //
    // create the new declaration and link it in
    //
    return compiler()->symmgr.CreateNamespaceDeclaration(
                nspace, 
                containingDeclaration,
                containingDeclaration->inputfile, 
                parseTree);
}

// add a namespace symbol to the given parent.  checks for collisions with
// classes, and displays an error if so.  returns a namespace regardless.
NSSYM * CLSDREC::addNamespace(NAMENODE * name, NSDECLSYM * parent)
{
    NAME * ident = name->pName;

    //
    // check for existing namespace
    //
    SYM *present = compiler()->symmgr.LookupGlobalSym(ident, parent->namespaceSymbol, MASK_ALL);
    if (present){
        if (present->kind != SK_NSSYM) {
            errorNameRefStrSymbol(name, parent, compiler()->ErrSym(parent->namespaceSymbol), present, ERR_DuplicateNameInNS);
            return NULL;
        } else {
            // just add this declaration to existing delcaration
            return present->asNSSYM();
        }
    }

    //
    // create new namespace
    //
    return compiler()->symmgr.CreateNamespace(ident, parent->namespaceSymbol);
}


// declares a class or struct.  This means that this aggregate and any contained aggregates
// have their symbols entered in the symbol table. 
// the access modifier on the type is also checked
void CLSDREC::declareAggregate(CLASSNODE * pClassTree, PARENTSYM * parent)
{
    AGGSYM * cls = addAggregate(pClassTree, pClassTree->pName, parent);
    if (cls) {
    	ASSERT(!(cls->isEnum && (cls->isStruct || cls->isClass || cls->isDelegate)));
        //
        // declare all nested types
        //
        //
        // parser guarantees that enums don't contain nested types
        //
        if (!cls->isEnum && !cls->isDelegate) {
            MEMBERNODE * members = pClassTree->pMembers;
            while (members) {

                SETLOCATIONNODE(members);

                if (members->kind == NK_NESTEDTYPE) {
                    if (!cls->isInterface) {
                        switch(members->asNESTEDTYPE()->pType->kind) {
                        case NK_CLASS:
                        case NK_STRUCT:
                        case NK_INTERFACE:
                        case NK_ENUM:
                            declareAggregate(members->asNESTEDTYPE()->pType->asAGGREGATE(), cls);
                            break;
                        case NK_DELEGATE:
                        {
                            DELEGATENODE *delegateNode = members->asNESTEDTYPE()->pType->asDELEGATE();
                            addAggregate(delegateNode, delegateNode->pName, cls);
                            break;
                        }
                        default:
                            ASSERT(!"Unknown aggregate type");
                        }
                    } else {
                        NAMENODE * name;

                        if (members->asNESTEDTYPE()->pType->kind == NK_DELEGATE)
                            name = members->asNESTEDTYPE()->pType->asDELEGATE()->pName;
                        else
                            name = members->asNESTEDTYPE()->pType->asAGGREGATE()->pName;

                        errorNameRef(name, cls, ERR_InterfacesCannotContainTypes);
                    }
                }

                members = members->pNext;
            }
        }
    }
}


// prepare a namespace for compilation.  this merely involves preparing all
// the namespace elements.  nspace is the symbol for THIS namespace, not its
// parent...
void CLSDREC::prepareNamespace(NSDECLSYM *nsDeclaration)
{
    if (!nsDeclaration) return;

    ASSERT(nsDeclaration->isPrepared);

    SETLOCATIONSYM(nsDeclaration);

    //
    // prepare members
    //
    FOREACHCHILD(nsDeclaration, elem)
        switch (elem->kind) {
        case SK_NSDECLSYM:
            prepareNamespace(elem->asNSDECLSYM());
            break;
        case SK_AGGSYM:
            prepareAggregate(elem->asAGGSYM());
            break;
        case SK_GLOBALATTRSYM:
            break;
        default:
            ASSERT(!"Unknown namespace member");
        }
    ENDFOREACHCHILD
}


// ensures that the using clauses for this namespace declaration
// have been resolved. This can happen between declare and define
// steps.
void CLSDREC::ensureUsingClausesAreResolved(NSDECLSYM *nsDeclaration, AGGSYM *classBeingResolved)
{
    if (!nsDeclaration->usingClausesResolved) {
        nsDeclaration->usingClausesResolved = true;

        ASSERT(!nsDeclaration->usingClauses);

        //
        // ok, now bind the using clauses:
        // Note that we don't add the using clauses as we find them
        // because the bindTypeName will reference the current using clauses.
        //
        SYMLIST * first = NULL;
        SYMLIST ** pointer = &first;

        NODELOOP(nsDeclaration->parseTree->pUsing, USING, usedecl)

            SYM * newUsingClause;
            // check that we got the right thing type or namespace
            if (usedecl->pAlias == NULL) {
                //
                // find the type or namespace for the using clause
                //
                BASENODE * nameNode = usedecl->pName;
                SYM * nspace = bindTypeName(nameNode, nsDeclaration, BTF_NSOK | BTF_NODECLARE, classBeingResolved);
                if (!nspace) {
                    // couldn't find the type/namespace
                    // error already reported
                    continue;
                }
                ASSERT(nspace->kind == SK_AGGSYM || nspace->kind == SK_NSSYM);

                if (nspace->kind != SK_NSSYM) {
                    errorNameRefSymbol(nameNode, nsDeclaration, nspace, ERR_BadUsingNamespace);
                    continue;
                }

                // check for duplicate using clauses
                if (first->contains(nspace)) {
                    errorNameRef(nameNode, nsDeclaration, WRN_DuplicateUsing);
                    continue;
                }

                newUsingClause = nspace;
            } else {
                PNAME name = usedecl->pAlias->pName;

                // check for duplicate name with other namespace elements
                SYM * duplicate = compiler()->symmgr.LookupGlobalSym(name, nsDeclaration->namespaceSymbol, MASK_ALL);
                if (duplicate) {
                    errorNameRefStrSymbol(usedecl->pAlias, nsDeclaration, compiler()->ErrSym(duplicate->parent), duplicate, ERR_ConflictAliasAndMember);
                    continue;
                }

                // check for duplicate name in using alias clause
                FOREACHSYMLIST(first, usingElem)
                    if ((usingElem->kind == SK_ALIASSYM) && (usingElem->asALIASSYM()->name == name)) {
                        duplicate = usingElem;
                        break;
                    }
                ENDFOREACHSYMLIST
                if (duplicate) {
                    errorNameRefSymbol(usedecl->pAlias, nsDeclaration, duplicate, ERR_DuplicateAlias);
                    continue;
                }

                ALIASSYM *alias = compiler()->symmgr.CreateAlias(name);
                alias->parseTree = usedecl;
                alias->parent = nsDeclaration;

                newUsingClause = alias;
            }
            compiler()->symmgr.AddToGlobalSymList(newUsingClause, &pointer);
        ENDLOOP;

        // And set them on the declaration so that they take effect...
        nsDeclaration->usingClauses = first;
    }
}

bool CLSDREC::hasBaseChanged(AGGSYM *cls, AGGSYM *newBaseClass)
{
    if (cls->isInterface) return false;

    //
    // in incremental scenario check that the type is the same
    //
    AGGSYM *oldBase;
    DWORD flags;
    compiler()->importer.GetBaseTokenAndFlags(cls, &oldBase, &flags);
    if (newBaseClass->isInterface)
    {
        newBaseClass = compiler()->symmgr.GetPredefType(PT_OBJECT, false);
    }

    return (oldBase != newBaseClass);
}


bool CLSDREC::setBaseType(AGGSYM *cls, AGGSYM *baseType)
{
    if (!resolveInheritanceRec(baseType)) 
    {
        cls->isResolvingBaseClasses = false;
        return false;
    }

    if (compiler()->IsIncrementalRebuild() && hasBaseChanged(cls, baseType)) 
    {
        cls->isResolvingBaseClasses = false;
        return false;
    }

    cls->baseClass = baseType;

    return true;
}

AGGSYM *CLSDREC::getEnumUnderlyingType(CLASSNODE *parseTree)
{
    //
    // get the underlying type
    //
    PREDEFTYPE baseTypeIndex;
    if (parseTree->pBases) {
        // parser guarantees this
        ASSERT(parseTree->pBases->TypeKind() == TK_PREDEFINED);

        baseTypeIndex = (PREDEFTYPE) parseTree->pBases->asTYPE()->iType;
    } else {
        baseTypeIndex = PT_INT;
    }
    // found a valid underlying type.
    // ensure it is resolved
    AGGSYM *baseType = compiler()->symmgr.GetPredefType(baseTypeIndex, false);
    if (!resolveInheritanceRec(baseType)) {
        // this should never happen
        ASSERT(0);
    }
    return baseType;
}

bool CLSDREC::resolveInheritanceRec(AGGSYM *cls)
{
    // check if we're done already
    if (cls->hasResolvedBaseClasses) {
        ASSERT(!cls->isResolvingBaseClasses);
        return true;
    }

    ASSERT(!(cls->isEnum && (cls->isStruct || cls->isClass || cls->isDelegate)));
    ASSERT(cls != compiler()->symmgr.GetObject());

    // check for cycles
    if (cls->isResolvingBaseClasses) {
        return true;
    }

    SETLOCATIONSYM(cls);

    if (cls->parseTree == NULL && (cls->tokenImport == 0 || TypeFromToken(cls->tokenImport) == mdtTypeRef)) {
        // This class appears to be neither imported nor defined in source
        compiler()->symmgr.DeclareType(cls);
        if (cls->hasResolvedBaseClasses || cls->isResolvingBaseClasses) {
            ASSERT(cls->hasResolvedBaseClasses ^ cls->isResolvingBaseClasses);
            return true;
        }
    }

    //
    // resolve our base classes
    //
    cls->isResolvingBaseClasses = true;

    //
    // if we are a nested class, resolve our enclosing classes
    //
    if (cls->isNested()) {
        //
        // resolve our containing class
        //
        bool ifaceChanged = !resolveInheritanceRec(cls->parent->asAGGSYM());
        if (cls->parent->asAGGSYM()->isResolvingBaseClasses) {
            errorSymbolAndRelatedSymbol(cls, cls->parent, ERR_CircularBase);
        }
        if (ifaceChanged) {
            cls->isResolvingBaseClasses = false;
            return false;
        }
    }

    if (cls->parseTree) {

        if (cls->isEnum) {
            //
            // get the underlying type
            //
            cls->underlyingType = getEnumUnderlyingType(cls->parseTree->asAGGREGATE());

            // enum should derive from "Enum".
            if (!setBaseType(cls, compiler()->symmgr.GetPredefType(PT_ENUM, false)))
            {
                cls->underlyingType = NULL;
                return false;
            }
        } else if (cls->isDelegate) {
            AGGSYM *baseType;

            //
            // all delegates in C# are multicast now
            //
            baseType = compiler()->symmgr.GetPredefType(PT_MULTIDEL, false);

            if (!setBaseType(cls, baseType))
                return false;
        } else {
            //
            // resolve base and implemented interfaces
            //
            SYMLIST ** addToIfaceList = &(cls->ifaceList);

            // only classes can explicitly list a base class
            bool needBase = (cls->isClass);
            BASENODE * bases = cls->parseTree->asAGGREGATE()->pBases;

            //
            // make sure that the base is set for structs
            //
            if (cls->isStruct && !setBaseType(cls, compiler()->symmgr.GetPredefType(PT_VALUE, false)))
                return false;

            NODELOOP(bases, BASE, base)
                ASSERT(base->kind == NK_TYPE);

                AGGSYM * type = bindType(base->asTYPE(), cls->containingDeclaration(), BTF_NODECLARE | BTF_NODEPRECATED, cls)->asAGGSYM();
                if (!type) {
                    // already reported error in bindTypeName
                    if (needBase && !setBaseType(cls, compiler()->symmgr.GetObject()))
                        return false;
                } else {
                    //
                    // in incremental scenario check that the base type is the same
                    //
                    if (!cls->baseClass && compiler()->IsIncrementalRebuild() && hasBaseChanged(cls, type)) {
                        cls->isResolvingBaseClasses = false;
                        return false;
                    }

                    //
                    // need to resolve base aggregate first
                    //
                    if (!resolveInheritanceRec(type)) {
                        cls->isResolvingBaseClasses = false;
                        return false;
                    }
                    compiler()->MakeFileDependOnType(cls, type);
                    if (type->isInterface) {
                        if (needBase)
                            cls->baseClass = compiler()->symmgr.GetObject();

                        // found an interface, check that it wasn't listed twice
                        if (cls->ifaceList->contains(type)) {
                            errorType(base->asTYPE(), cls, ERR_DuplicateInterfaceInBaseList);
                        } else {
                            if (type->isResolvingBaseClasses) {
                                // found a cycle, report error and don't add interface to list
                                errorSymbolAndRelatedSymbol(type, cls, ERR_CycleInInterfaceInheritance);
                            } else {
                                // didn't find a duplicate
                                // everything checks out, add to our list
                                compiler()->symmgr.AddToGlobalSymList(type, &addToIfaceList);

                                // Made sure base interface is at least as visible as derived interface (don't check
                                // for derived CLASS, though.
                                if (cls->isInterface) 
                                    checkConstituentVisibility(cls, type, ERR_BadVisBaseInterface);
                            }
                        }
                    } else if (needBase) {
                        ASSERT(type->kind != SK_ERRORSYM);
                        if (type->isPredefined && 
                            (type->iPredef == PT_ENUM || type->iPredef == PT_VALUE || type->iPredef == PT_DELEGATE  || type->iPredef == PT_MULTIDEL || type->iPredef == PT_ARRAY) && 
                            !cls->isPredefined) 
                        {
                            errorSymbolAndRelatedSymbol(cls, type, ERR_DeriveFromEnumOrValueType);
                        }

                        if (type->isResolvingBaseClasses) {
                            if (!setBaseType(cls, compiler()->symmgr.GetObject()))
                                return false;
                            errorSymbolAndRelatedSymbol(cls, type, ERR_CircularBase);
                        } else {
                            //
                            // found an explicit base
                            // ensure it is resolved
                            //
                            cls->baseClass = type;

                            // make sure base class is at least as visible as derived class.
                            checkConstituentVisibility(cls, type, ERR_BadVisBaseClass);
                        }
                    } else {
                        // already reported error in bindTypeName
                        if (needBase && !setBaseType(cls, compiler()->symmgr.GetObject()))
                            return false;
                        // found non-interface where interface was expected
                        errorTypeSymbol(base->asTYPE(), cls, type, ERR_NonInterfaceInInterfaceList);
                    }
                }
                needBase = false;
            ENDLOOP;

            //
            // make sure that the base is set for classes and structs
            //
            if (!cls->baseClass && cls->isClass) {
                if (!setBaseType(cls, compiler()->symmgr.GetObject()))
                    return false;
            }

            //
            // compute recursive closure of all implemeneted interfaces
            // Need this before we check for iface changes
            //
            combineInterfaceLists(cls);

            if (compiler()->IsIncrementalRebuild() && compiler()->importer.HasIfaceListChanged(cls)) {
                cls->isResolvingBaseClasses = false;
                cls->baseClass = NULL;
                cls->ifaceList = NULL;
                return false;
            }
        }
        cls->isResolvingBaseClasses = false;
        cls->hasResolvedBaseClasses = true;

    } else {
        //
        // read base from metadata (unless it's a bogus token)
        //
        if (!compiler()->importer.ResolveInheritance(cls))
            return false;
    }
    ASSERT(cls->hasResolvedBaseClasses);
    ASSERT(!cls->isResolvingBaseClasses);

    ASSERT(!(cls->isEnum && (cls->isStruct || cls->isClass || cls->isDelegate)));

    // Set inherited bits.
    if (cls->baseClass) {
        AGGSYM * baseClass = cls->baseClass;
        if (baseClass->isAttribute)
            cls->isAttribute = true;
        if (baseClass->isSecurityAttribute)
            cls->isSecurityAttribute = true;
        if (baseClass->isMarshalByRef)
            cls->isMarshalByRef = true;
    }

    return true;
}

bool CLSDREC::resolveInheritance(AGGSYM *cls)
{
    ASSERT(!cls->isResolvingBaseClasses);
    if (!resolveInheritanceRec(cls)) {
        return false;
    }
    ASSERT(!cls->isResolvingBaseClasses);

    //
    // resolve nested types, interfaces will have no members at this point
    //
    FOREACHCHILD(cls, elem)

        SETLOCATIONSYM(elem);

        // should only have types at this point
        switch (elem->kind) {
        case SK_AGGSYM:
            ASSERT (elem->asAGGSYM()->isClass || elem->asAGGSYM()->isStruct || 
                elem->asAGGSYM()->isInterface || elem->asAGGSYM()->isEnum ||
                elem->asAGGSYM()->isDelegate);

            if (!resolveInheritance(elem->asAGGSYM())) {
                return false;
            }
            break;

        case SK_ARRAYSYM:
        case SK_EXPANDEDPARAMSSYM:
        case SK_PARAMMODSYM:
        case SK_PTRSYM:
            break;

        default:
            ASSERT(!"Unknown member");
        }
    ENDFOREACHCHILD

    return true;
}

// resolves the inheritance hierarchy for a namespace's types
// returns false if a change in the hierarchy was detected in 
// an incremental build
bool CLSDREC::resolveInheritance(NSDECLSYM *nsDeclaration)
{
    if (!nsDeclaration) return true;

    SETLOCATIONSYM(nsDeclaration);

    //
    // ensure the using clauses have been done
    // Note that this can happen before we get here
    // if a class in another namespace which is derived
    // from a class in this namespace is defined before us.
    //
    ensureUsingClausesAreResolved(nsDeclaration, NULL);

    //
    // and define contained types and namespaces...
    //
    FOREACHCHILD(nsDeclaration, elem)
        switch (elem->kind) {
        case SK_NSDECLSYM:
            if (!resolveInheritance(elem->asNSDECLSYM()))
                return false;
            break;
        case SK_AGGSYM:
            if (!resolveInheritance(elem->asAGGSYM()))
                return false;
            break;
        case SK_GLOBALATTRSYM:
            break;
        default:
            ASSERT(!"Unknown type");
        }
    ENDFOREACHCHILD

    return true;
}

// define a namespace by binding its name to a symbol and resolving the uses
// clauses in this namespace declaration.  nspace indicates the PARENT namaspace
void CLSDREC::defineNamespace(NSDECLSYM *nsDeclaration)
{
    SETLOCATIONSYM(nsDeclaration);

    ensureUsingClausesAreResolved(nsDeclaration, NULL);
    ASSERT(nsDeclaration->usingClausesResolved);

    //
    // force binding of using aliases. Up to this point they are lazily bound
    //
    FOREACHSYMLIST(nsDeclaration->usingClauses, usingSym)
        if (usingSym->kind == SK_ALIASSYM) {
            SYM * sym;
            bindUsingAlias(usingSym->asALIASSYM(), &sym, NULL, NULL);
        }
    ENDFOREACHSYMLIST

    //
    // handle global attributes
    //
    NODELOOP(nsDeclaration->parseTree->pGlobalAttr, ATTRDECL, attr)
        declareGlobalAttribute(attr, nsDeclaration);
    ENDLOOP;

    //
    // and define contained types and namespaces...
    //
    FOREACHCHILD(nsDeclaration, elem)
        switch (elem->kind) {
        case SK_NSDECLSYM:
            defineNamespace(elem->asNSDECLSYM());
            break;
        case SK_AGGSYM:
            defineAggregate(elem->asAGGSYM());
            break;
        case SK_GLOBALATTRSYM:
            break;
        default:
            ASSERT(!"Unknown type");
        }
    ENDFOREACHCHILD

    nsDeclaration->isPrepared = true;
}


// check the provided flags for item and set the modifiers & access level
// on the item based on the given flags
//
// item - symbol to check, and whose flags to assign
// allowedFlags, actualFlags - provided flags
// error - whether to check for erronous flags & report error
// or to just set the flags
void CLSDREC::checkFlags(SYM * item, unsigned allowedFlags, unsigned actualFlags, bool error)
{
    //
    // if any flags are disallowed, tell which:
    //
    if (error && (actualFlags & ~allowedFlags)) {
        for (unsigned flags = 1, index = 0; flags <= NF_MOD_LAST; flags = flags << 1) {
            if (flags & actualFlags & ~allowedFlags) {
                TOKENID id = accessTokens[index];
                const TOKINFO * info = CParser::GetTokenInfo(id);
                errorSymbolStr(item, ERR_BadMemberFlag, info->pszText);
            }
            index += 1;
        }
        actualFlags &= allowedFlags;
    }


    if (error) {
        if ((actualFlags & allowedFlags & NF_MOD_UNSAFE) && !compiler()->options.m_fUNSAFE) {
            errorSymbol(item, ERR_IllegalUnsafe);
        }
    }

    //
    // set protection level:
    //
    NODEFLAGS protFlags = (NODEFLAGS) (actualFlags & allowedFlags & (NF_MOD_ACCESSMODIFIERS));
    switch (protFlags) {
    case 0:
        // get default protection level from our container
        switch (item->parent->kind) {
        case SK_NSSYM:
            item->access = ACC_INTERNAL;
            break;
        case SK_AGGSYM:
            if (!item->parent->asAGGSYM()->isInterface) {
                item->access = ACC_PRIVATE;
            } else {
                item->access = ACC_PUBLIC;
            }
            break;
        default:
            ASSERT(!"Unknown parent");
        }
        break;
    case NF_MOD_INTERNAL:
        item->access = ACC_INTERNAL;
        break;
    case NF_MOD_PRIVATE:
        item->access = ACC_PRIVATE;
        break;
    case NF_MOD_PROTECTED:
        item->access = ACC_PROTECTED;
        break;
    case NF_MOD_PUBLIC:
        item->access = ACC_PUBLIC;
        break;
    case NF_MOD_PROTECTED | NF_MOD_INTERNAL:
        item->access = ACC_INTERNALPROTECTED;
        break;
    default:
        ASSERT(!"Invalid protection modifiers");
    }

    if (error) {
        // check for conflict with abstract & sealed modifiers on classes
        if ((actualFlags & (NF_MOD_ABSTRACT | NF_MOD_SEALED)) == (NF_MOD_ABSTRACT | NF_MOD_SEALED)) {
            errorSymbol(item, ERR_AbstractAndSealed);
        }

        // Check for conclict between virtual, new, static, override, abstract
        if (error && (allowedFlags & (NF_MOD_VIRTUAL | NF_MOD_OVERRIDE | NF_MOD_NEW | NF_MOD_ABSTRACT))) {
            if ((actualFlags & NF_MOD_STATIC) && (actualFlags & (NF_MOD_VIRTUAL | NF_MOD_OVERRIDE | NF_MOD_ABSTRACT))) {
                errorSymbol(item, ERR_StaticNotVirtual);
            } else if ((actualFlags & NF_MOD_OVERRIDE) && (actualFlags & (NF_MOD_VIRTUAL | NF_MOD_NEW))) {
                errorSymbol(item, ERR_OverrideNotNew);
            } else if ((actualFlags & NF_MOD_ABSTRACT) && (actualFlags & NF_MOD_VIRTUAL)) {
                errorSymbol(item, ERR_AbstractNotVirtual);
            }
        }
    }
}

// prepare a field for compilation by setting its constant field, if present
// and veryfing that field shadowing is explicit...
void CLSDREC::prepareFields(MEMBVARSYM *field)
{
    if (! field->isEvent) {
        reportDeprecatedType(field->parseTree, field, field->type);

        //
        // find a member with the same name in a base class
        // check the hiding flags are correct
        //
        checkSimpleHiding(field, field->parseTree->pParent->flags);
        
    }

    ASSERT(!field->isUnevaled);
}

// checks if we are hiding an abstract method in a bad way
// this happens when the newSymbol is in an abstract class and it
// will prevent the declaration of non-abstract derived classes
void CLSDREC::checkHiddenSymbol(SYM *newSymbol, SYM *hiddenSymbol)
{
    // we are interested in hiding abstract methods in abstract classes
    if (hiddenSymbol->kind == SK_METHSYM && hiddenSymbol->asMETHSYM()->isAbstract &&
        newSymbol->parent->asAGGSYM()->isAbstract) {

        switch (newSymbol->access) {
        case ACC_INTERNAL:
            // derived classes outside this assembly will be OK
            break;
        case ACC_PUBLIC:
        case ACC_PROTECTED:
        case ACC_INTERNALPROTECTED:
            // the new symbol will always hide the abstract symbol
            errorSymbolAndRelatedSymbol(newSymbol, hiddenSymbol, ERR_HidingAbstractMethod);
            break;
        case ACC_PRIVATE:
            // no problem since the new member won't hide the abstract method in derived classes
            break;
        default:
            // bad access
            ASSERT(0);
        }
    } else if (hiddenSymbol->kind == SK_PROPSYM) {
        if (hiddenSymbol->asPROPSYM()->methGet) {
            checkHiddenSymbol(newSymbol, hiddenSymbol->asPROPSYM()->methGet);
        }
        if (hiddenSymbol->asPROPSYM()->methSet) {
            checkHiddenSymbol(newSymbol, hiddenSymbol->asPROPSYM()->methSet);
        }
    }
}

// list of first invalid (overflow) values for enumerator constants by type
// note that byte is unsigned, while short and int are signed
static const int enumeratorOverflowValues[] = {
    0x100,          // byte
    0x8000,         // short
    0x80000000,     // int
    0x80,           // sbyte
    0x10000,        // ushort
    0x00000000      // uint
};

// evaluate a field constant for fieldcurrent, fieldFirst is given only so that you
// know what to display in case of a circular definition error.
// returns true on success
bool CLSDREC::evaluateFieldConstant(MEMBVARSYM * fieldFirst, MEMBVARSYM * fieldCurrent)
{
    if (!fieldFirst) {
        fieldFirst = fieldCurrent;
    }

    ASSERT(fieldCurrent->isUnevaled);
    ASSERT(fieldFirst->isUnevaled);
    if (fieldCurrent->isConst) {
        // Circular field definition.
        ASSERT(fieldFirst->getClass()->isEnum || fieldFirst->parseTree->asVARDECL()->pArg);
        errorSymbol(fieldFirst, ERR_CircConstValue);
        fieldCurrent->isConst = false;
        fieldCurrent->isUnevaled = false;
        return false;
    }
    // Set the flag to mean we are evaling this field...
    fieldCurrent->isConst = true;

    // and compile the parse tree:
    BASENODE *expressionTree = fieldCurrent->getConstExprTree();
    if (!expressionTree) {
        ASSERT(fieldCurrent->getClass()->isEnum);

        //
        // we have an enum member with no expression
        //
        PREDEFTYPE fieldType = (PREDEFTYPE) fieldCurrent->type->asAGGSYM()->underlyingType->iPredef;
        if (fieldCurrent->previousEnumerator && (!fieldCurrent->previousEnumerator->isUnevaled || evaluateFieldConstant(fieldFirst, fieldCurrent->previousEnumerator))) {
            //
            // we've successfully evaluated our previous enumerator
            // add one and check for overflow
            //

            if (fieldType == PT_LONG || fieldType == PT_ULONG) {
                ASSERT(offsetof(CONSTVAL, longVal) == offsetof(CONSTVAL, ulongVal));

                //
                // do long constants special since their values are allocated on the heap
                //
                __int64 *constVal = (__int64*) compiler()->globalSymAlloc.Alloc(sizeof(__int64));
                *constVal = *fieldCurrent->previousEnumerator->constVal.longVal + 1;

                //
                // check for overflow
                //
                if ((unsigned __int64) *constVal == (fieldType == PT_LONG ? I64(0x8000000000000000) : 0x0)) {
                    errorSymbol(fieldCurrent, ERR_EnumeratorOverflow);
                    *constVal = 0;
                }

                fieldCurrent->constVal.longVal = constVal;
            } else {
                //
                // we've got an int, short, or byte constant
                //
                int constVal = fieldCurrent->previousEnumerator->constVal.iVal + 1;

                //
                // check for overflow
                //
                ASSERT(PT_BYTE == 0 && PT_SHORT == 1 && PT_INT == 2);
                ASSERT(fieldType >= PT_BYTE && fieldType <= PT_UINT);
                if (fieldType >= PT_SBYTE) {
                    ASSERT( (PT_SBYTE + 1) == PT_USHORT && (PT_USHORT + 1) == PT_UINT);
                    fieldType = (PREDEFTYPE)(fieldType - PT_SBYTE + 3);
                }
                if (constVal == enumeratorOverflowValues[fieldType]) {
                    errorSymbol(fieldCurrent, ERR_EnumeratorOverflow);
                    constVal = 0;
                }

                fieldCurrent->constVal.iVal = constVal;
            }
        } else {
            //
            // we are the first enumerator, or we failed to evaluate our previos enumerator
            // set constVal to the appropriate type of zero
            //
            fieldCurrent->constVal = compiler()->symmgr.GetPredefZero(fieldType);
        }
    } else {
        EXPR * rval;
        if (fieldCurrent == fieldFirst) {
            rval = compiler()->funcBRec.compileFirstField (fieldCurrent, expressionTree);
        } else {
            rval = compiler()->funcBRec.compileNextField(fieldCurrent, expressionTree);
        }

        //
        // check that we really got a constant value
        //
        if (rval->kind != EK_CONSTANT) {
            if (rval->kind != EK_ERROR) {  // error reported by compile already...
                errorSymbol(fieldCurrent, ERR_NotConstantExpression);
            }
            fieldCurrent->isConst = false;
            fieldCurrent->isUnevaled = false;
            if (fieldCurrent->type->kind == SK_AGGSYM && fieldCurrent->type->asAGGSYM()->isPredefined &&
                (fieldCurrent->type->asAGGSYM()->iPredef == FT_I8 || fieldCurrent->type->asAGGSYM()->iPredef == FT_U8))
                fieldCurrent->constVal = compiler()->symmgr.GetPredefZero(PT_LONG);
            else
                fieldCurrent->constVal = compiler()->symmgr.GetPredefZero(PT_INT);
            return false;
        }

        // We have to copy what rval->getVal() actually points to, as well as val itself, because
        // it may be allocated with a short-lived allocator.
        fieldCurrent->constVal = rval->asCONSTANT()->getSVal();
        switch (rval->type->fundType()) {
        case FT_I8:
        case FT_U8:
            fieldCurrent->constVal.longVal = (__int64 *) compiler()->globalSymAlloc.Alloc(sizeof(__int64));
            * fieldCurrent->constVal.longVal = *(rval->asCONSTANT()->getVal().longVal);
            break;

        case FT_R4:
        case FT_R8:
            fieldCurrent->constVal.doubleVal = (double *) compiler()->globalSymAlloc.Alloc(sizeof(double));
            * fieldCurrent->constVal.doubleVal = *(rval->asCONSTANT()->getVal().doubleVal);
            break;

        case FT_STRUCT:
            fieldCurrent->constVal.decVal = (DECIMAL *) compiler()->globalSymAlloc.Alloc(sizeof(DECIMAL));
            * fieldCurrent->constVal.decVal = *(rval->asCONSTANT()->getVal().decVal);
            break;

        case FT_REF:
            if (rval->asCONSTANT()->getSVal().strVal != NULL) {
                // Allocate memory for the string constant.
                STRCONST * strConst = (STRCONST *) compiler()->globalSymAlloc.Alloc(sizeof(STRCONST));

                int cch = rval->asCONSTANT()->getSVal().strVal->length;
                strConst->length = cch;
                strConst->text = (WCHAR *) compiler()->globalSymAlloc.Alloc(cch * sizeof(WCHAR));
                memcpy(strConst->text, rval->asCONSTANT()->getSVal().strVal->text, cch * sizeof(WCHAR));
                fieldCurrent->constVal.strVal = strConst;
            }
            break;
        default:
            break;
        }
    }

    fieldCurrent->isUnevaled = false;
    return true;
}

void CLSDREC::evaluateConstants(AGGSYM *cls)
{
    ASSERT(cls->isDefined && cls->parseTree);
    FOREACHCHILD(cls, child)
        SETLOCATIONSYM(child);

        switch (child->kind) {
        case SK_MEMBVARSYM:
            //
            // evaluate constants
            //
            if (child->asMEMBVARSYM()->isUnevaled) {
                evaluateFieldConstant(NULL, child->asMEMBVARSYM());
                compiler()->DiscardLocalState();
            }
            ASSERT(!child->asMEMBVARSYM()->isUnevaled);
            
            OBSOLETEATTRBIND::Compile(compiler(), child->asMEMBVARSYM());
            break;
        case SK_AGGSYM:
            // nothing
            break;
        case SK_METHSYM:
            //
            // validate existance or non-existance of conditional methods here
            //
            CONDITIONALATTRBIND::Compile(compiler(), child->asMETHSYM());
            break;
        case SK_EXPANDEDPARAMSSYM:
        case SK_PARAMMODSYM:
        case SK_ARRAYSYM:
        case SK_PTRSYM:
            break;
        case SK_PROPSYM:
            OBSOLETEATTRBIND::Compile(compiler(), child->asPROPSYM());
            break;
        case SK_EVENTSYM:
            {
            EVENTSYM *event = child->asEVENTSYM();
            OBSOLETEATTRBIND::Compile(compiler(), event);
            if (event->isDeprecated) {
                event->methAdd->isDeprecated            = true;
                event->methAdd->isDeprecatedError       = event->isDeprecatedError;
                event->methAdd->deprecatedMessage       = event->deprecatedMessage;
                event->methRemove->isDeprecated         = true;
                event->methRemove->isDeprecatedError    = event->isDeprecatedError;
                event->methRemove->deprecatedMessage    = event->deprecatedMessage;
            }
            }
            break;
        default:
            ASSERT(!"Unknown node type");
        }
    ENDFOREACHCHILD;

    if (cls->isClass || cls->isStruct) {
        //
        // determine if this type is an attribute class
        //
        ATTRATTRBIND::Compile(compiler(), cls);

        if (cls->isAttribute)
        {
            if (!cls->attributeClass) {
                //
                // inherit usage from base class if not present
                //
                cls->attributeClass = cls->baseClass->attributeClass;
                cls->isMultipleAttribute = cls->baseClass->isMultipleAttribute;

                if (cls->baseClass->iPredef == PT_ATTRIBUTE) {
                    cls->attributeClass = catAll;
                }
            }
        }
    } else {
        OBSOLETEATTRBIND::Compile(compiler(), cls);
    }
}


// prepares a method for compilation.  the parsetree is obtained from the
// method symbol.  verify means whether to verify that the right override/new/etc...
// flags were specified on the method...
void CLSDREC::prepareMethod(METHSYM * method)
{
    if (method->isIfaceImpl)
    {
        return;
    }

    reportDeprecatedMethProp(method->parseTree, method);

    //
    // conversions are special
    //
    if (method->isConversionOperator()) {

        prepareConversion(method);

    //
    // operators and dtors just need some simple checking
    //
    } else if (method->isOperator || method->isDtor) {

        prepareOperator(method);
    //
    // for constructors the basic stuff(valid flags) is checked
    // in defineMethod(). The more serious stuff(base constructor calls)
    // is checked when we actually compile the method and we can
    // evaluate the arg list
    //
    // property/event accessors are handled in prepareProperty/prepareEvent by their
    // associated property/event
    //
    } else if (!method->isPropertyAccessor && !method->isEventAccessor) {
        /*
        The rules are as follows:
        Of new, virtual, override, you can't do both override and virtual as well as
        override and new.  So that leaves:
        [new]
            - accept the method w/o any checks, regardless if base exists
        [virtual]
            - mark method as new, if base contains previous method, then complain
            about missing [new] or [override]
        [override]
            - check that base contains a virtual method
        [new virtual]
            - mark method as new, make no checks  (same as [new])
        [nothing]
            - mark method as new, check base class, give warning if previous exists
        */

        // First, find out if a method w/ that name & args exists in the baseclasses:
        AGGSYM * cls = method->getClass();
        BASENODE * tree = method->parseTree;

        //
        // check new, virtual & override flags
        //
        if (method->name) {
            // for explicit interface implementation these flags can't be set

            if (!method->isCtor) {
                //
                // find hidden member in a base class
                //
                if (!(tree->flags & NF_MOD_OVERRIDE)) {
                    //
                    // not an override, just do the simple checks
                    //
                    checkSimpleHiding(method, tree->flags, SK_METHSYM, method->params);

                    // check if this is finalize on object, if so, warn
                    if (method->name == compiler()->namemgr->GetPredefName(PN_DTOR) && method->parent == compiler()->symmgr.GetPredefType(PT_OBJECT, false)) {
                        errorFile(method->parseTree, method->getInputFile(), ERR_OverrideFinalizeDepracated);
                    }

                } else {
                    //
                    // we have an override method
                    //
                    SYM *hiddenSymbol;
                    hiddenSymbol = findHiddenSymbol(method->name, SK_METHSYM, method->params, cls->baseClass, cls);
                    if (hiddenSymbol) {

                        if (hiddenSymbol->kind != SK_METHSYM) {
                            // found a non-method we will hide
                            // we should not have 'override'
                            errorSymbolAndRelatedSymbol(method, hiddenSymbol, ERR_CantOverrideNonFunction);
                            checkHiddenSymbol(method, hiddenSymbol);
                        } else {
                            METHSYM *hiddenMethod = hiddenSymbol->asMETHSYM();
                            ASSERT (hiddenMethod->params == method->params);

                            // if this an override of Finalize on object, then give a warning...
                            if (hiddenMethod->name == compiler()->namemgr->GetPredefName(PN_DTOR) && method->cParams == 0) {
                                METHSYM *sym = hiddenMethod;
                                while (sym && sym->isOverride && sym->getClass()->baseClass) {
                                    sym = findHiddenSymbol(method->name, SK_METHSYM, method->params, sym->getClass()->baseClass, cls)->asMETHSYM();
                                }
                                if (sym->getClass() == compiler()->symmgr.GetPredefType(PT_OBJECT, false)) {
                                    errorFile(method->parseTree, method->getInputFile(), ERR_OverrideFinalizeDepracated);
                                }
                            }

                            // found a method of same signature that we will hide
                            method->isOverride = true;

                            if (hiddenMethod->isOperator || hiddenMethod->isPropertyAccessor || hiddenMethod->isEventAccessor) {
                                errorSymbolAndRelatedSymbol(method, hiddenMethod, ERR_CantOverrideSpecialMethod);
                            }

                            if (hiddenMethod->isBogus) {
                                errorSymbolAndRelatedSymbol(method, hiddenMethod, ERR_CantOverrideBogusMethod);
                            }

                            if (!hiddenMethod->isVirtual) {
                                if (hiddenMethod->isOverride)
                                    errorSymbolAndRelatedSymbol(method, hiddenMethod, ERR_CantOverrideSealed);
                                else
                                    errorSymbolAndRelatedSymbol(method, hiddenMethod, ERR_CantOverrideNonVirtual);
                            }

                            if (hiddenMethod->access != method->access) {
                                CError  *pError = make_errorSymbolStrStr(method, ERR_CantChangeAccessOnOverride,
                                    compiler()->ErrAccess(hiddenMethod->access), compiler()->ErrSym(hiddenMethod));
                                AddRelatedSymbolLocations (pError, hiddenMethod);
                                compiler()->SubmitError (pError);
                            }

                            if (hiddenMethod->retType != method->retType) {
                                errorSymbolAndRelatedSymbol(method, hiddenMethod, ERR_CantChangeReturnTypeOnOverride);
                            }

                            if (hiddenMethod->hasCmodOpt) {
                                method->hasCmodOpt = true;
                            }

                            if (!method->isDeprecated && hiddenMethod->isDeprecated) {
                                errorSymbolAndRelatedSymbol(method, hiddenMethod, WRN_NonObsoleteOverridingObsolete);
                            }

                            checkForMethodImplOnOverride(method, hiddenMethod);
                        }
                    } else {
                        // didn't find hidden base member
                        errorSymbol(method, ERR_OverrideNotExpected);
                    }
                }

                //
                // check that abstract methods are in abstract classes
                //
                if (method->isAbstract && !cls->isAbstract) {
                    errorSymbolAndRelatedSymbol(method, cls, ERR_AbstractInConcreteClass);
                }

                //
                // check that new virtual methods aren't in sealed classes
                //
                if (cls->isSealed && method->isVirtual && !method->isOverride) {
                    errorSymbolAndRelatedSymbol(method, cls, ERR_NewVirtualInSealed);
                }
            }

            //
            // check that the method body existance matches with the abstractness
            // attribute of the method
            //
            if (method->isAbstract || method->isExternal) {
                if ((tree->other & NFEX_METHOD_NOBODY) == 0) {
                    // found an abstract method with a body
                    errorSymbol(method, method->isAbstract ? ERR_AbstractHasBody : ERR_ExternHasBody);
                }
            } else {
                if (tree->other & NFEX_METHOD_NOBODY) {
                    // found non-abstract method without body
                    errorSymbol(method, ERR_ConcreteMissingBody);
                }
            }
            
            //
            // check for internal virtuals
            //
            if (method->isVirtual && method->access == ACC_INTERNAL && !cls->isSealed && cls->hasExternalAccess()) {
                errorSymbol(method, WRN_InternalVirtual);
            }
        } else {
            //
            // method is an explicit interface implementation
            //

            // get the interface
            method->explicitImpl = checkExplicitImpl(
                                method->parseTree->asANYMETHOD()->pName, 
                                method->retType,
                                method->params,
                                method, 
                                cls
                                )->asMETHSYM();

            if (cls->isStruct) {
                cls->hasExplicitImpl = true;
            }

            //
            // must have a body
            //
            if (tree->other & NFEX_METHOD_NOBODY && !method->isExternal) {
                // found non-abstract method without body
                errorSymbol(method, ERR_ConcreteMissingBody);
            } else if (method->isExternal && (tree->other & NFEX_METHOD_NOBODY) == 0)  {
                // found an extern method with a body
                errorSymbol(method, ERR_ExternHasBody);
            }
        }
    }
}

// checks if the runtime will override the method we want to override
void CLSDREC::checkForMethodImplOnOverride(METHSYM *method, METHSYM *overridenMethod)
{
    ASSERT(method->isOverride);

    // Check for requiring an explicit impl for an override of a method in another assembly
    // which is possibly shadowed by an internal method in another assembly                             
    if (method->access != ACC_INTERNAL && method->access != ACC_INTERNALPROTECTED && overridenMethod->getAssemblyIndex() != method->getAssemblyIndex())
    {
        SYM *sym = findAnyAccessHiddenSymbol(method->name, SK_METHSYM, method->params, method->getClass()->baseClass);
        if (sym != overridenMethod)
        {
            // Need an explicit method impl for this override
            method->requiresMethodImplForOverride = true;
            method->explicitImpl = overridenMethod;
        }
    }
}

// list of locally defined and inherited conversion operators.
PSYMLIST CLSDREC::getConversionList(AGGSYM *cls)
{
    if (cls->hasConversion && !cls->conversionOperators) {

        //
        // add all locally defined conversions first
        //
        SYMLIST **addToList = &cls->conversionOperators;
        FOREACHCHILD(cls, sym)
            if ((sym->kind == SK_METHSYM) && sym->asMETHSYM()->isConversionOperator()) {
    			/**                                                                    
                /**/METHSYM *msym = sym->asMETHSYM();
                /**/ASSERT( msym->cParams == 1);
                /**/compiler()->symmgr.DeclareType(msym->params[0]);
                /**/FUNDTYPE ftSrc = msym->params[0]->fundType();
                /**/compiler()->symmgr.DeclareType(msym->retType);
                /**/FUNDTYPE ftDest = msym->retType->fundType();
                /**/if (ftDest == FT_NONE || ftDest > FT_R8 || ftSrc == FT_NONE || ftSrc > FT_R8) {
                    compiler()->symmgr.AddToGlobalSymList(sym, &addToList);
                /**/}
            }
        ENDFOREACHCHILD

        if (cls->baseClass) {

            //
            // append base class conversions to our list
            //
            *addToList = getConversionList(cls->baseClass);
        }
    }

    return cls->conversionOperators;
}

// prepare checks for a user defined conversino operator
// checks that the conversion doesn't override a compiler generated conversion
// checks extern and body don't conflict
void CLSDREC::prepareConversion(METHSYM * conversion)
{
    AGGSYM *cls = conversion->getClass();

    //
    // what are we converting from/to
    //
    PTYPESYM alternateType;
    if (conversion->retType == cls) {
        alternateType = conversion->params[0];
    } else {
        alternateType = conversion->retType;
    }

    //
    // can't convert from/to a base or derived class or interface.
    // we check this here so that the check for hidden members below
    // doesn't report odd errors.
    //
    AGGSYM * classToSearch;
    if (alternateType->kind == SK_AGGSYM) {

        //
        // can't convert to/from an interface
        //
        if (alternateType->asAGGSYM()->isInterface) {
            errorSymbol(conversion, ERR_ConversionWithInterface);
        } else if ((cls->isClass || cls->isStruct) && alternateType->asAGGSYM()->isClass) {

            //
            // check that we aren't converting to/from a base class
            //
            classToSearch = cls->baseClass;
            while (classToSearch) {
                if (classToSearch == alternateType) {
                    errorSymbol(conversion, ERR_ConversionWithBase);
                    return;
                }

                classToSearch = classToSearch->baseClass;
            }

            //
            // check that we aren't converting to/from a derived class
            //
            classToSearch = alternateType->asAGGSYM()->baseClass;
            while (classToSearch) {
                if (classToSearch == cls) {
                    errorSymbol(conversion, ERR_ConversionWithDerived);
                    return;
                }

                classToSearch = classToSearch->baseClass;
            }
        }
    }

    //
    // operators don't hide
    //

    if (conversion->isExternal) {
        if ((conversion->getParseTree()->other & NFEX_METHOD_NOBODY) == 0) {
            // found an abstract method with a body
            errorSymbol(conversion, ERR_ExternHasBody);
        }
    } else {
        if (conversion->getParseTree()->other & NFEX_METHOD_NOBODY) {
            // found non-abstract method without body
            errorSymbol(conversion, ERR_ConcreteMissingBody);
        }
    }
}

// prepare checks for a user defined operator (not conversion operators)
// checks extern and body don't conflict
void CLSDREC::prepareOperator(METHSYM * op)
{
    if (op->isExternal) {
        if ((op->getParseTree()->other & NFEX_METHOD_NOBODY) == 0) {
            // found an abstract method with a body
            errorSymbol(op, ERR_ExternHasBody);
        }
    } else {
        if (op->getParseTree()->other & NFEX_METHOD_NOBODY) {
            // found non-abstract method without body
            errorSymbol(op, ERR_ConcreteMissingBody);
        }
    }
}

// do prepare stage for a property in a class or struct
// verifies new, override, abstract, virtual against
// inherited members.
void CLSDREC::prepareProperty(PROPSYM * property)
{
    reportDeprecatedMethProp(property->parseTree, property);

    definePropertyAccessors(property);

    AGGSYM * cls = property->getClass();
    if (property->isIndexer()) {
        checkForBadMember(property->getRealName(), property->getParseTree(), cls);
    }

    // First, find out if a property w/ that name & args exists in the baseclasses:
    PROPERTYNODE * tree = property->parseTree->asANYPROPERTY();

    //
    // check new, virtual & override flags
    //
    if (property->name) {
        // for explicit interface implementation these flags can't be set

            //
            // find hidden member in a base class
            //
        if (!property->isOverride) {
            checkSimpleHiding( property, tree->flags, SK_PROPSYM, property->params);
        } else {
            SYM *hiddenSymbol = findHiddenSymbol(property->name, SK_PROPSYM, property->params, cls->baseClass, cls);
            if (hiddenSymbol) {
                if (hiddenSymbol->kind != SK_PROPSYM) {
                    // found a non-property we will hide
                    // we should not have 'override'
                    errorSymbolAndRelatedSymbol(property, hiddenSymbol, ERR_CantOverrideNonProperty);

                    checkHiddenSymbol(property, hiddenSymbol);
                } else {

                    //
                    // found a property of same signature that we will override
                    //

                    //
                    // check virtual-ness of overriden accessors
                    // we may have to go to a base class if the hidden property is
                    // also an override and doesn't declare both accessors
                    //
                    bool needGet = (property->methGet != 0);
                    bool needSet = (property->methSet != 0);
                    PROPSYM *lastProperty;
                    PROPSYM *hiddenProperty;
                    do
                    {
                        hiddenProperty = hiddenSymbol->asPROPSYM();

                        //
                        // remember the last base property we found
                        //
                        lastProperty = hiddenProperty;

                        //
                        // can't override invalid members
                        //
                        if (hiddenProperty->isBogus) {
                            errorSymbolAndRelatedSymbol(property, hiddenProperty, ERR_CantOverrideBogusMethod);
                        }

                        //
                        // can't change access on override
                        //
                        if (hiddenProperty->access != property->access) {
                            CError  *pError = make_errorSymbolStrStr(property, ERR_CantChangeAccessOnOverride,
                                compiler()->ErrAccess(hiddenProperty->access), compiler()->ErrSym(hiddenProperty));
                            AddRelatedSymbolLocations (pError, hiddenProperty);
                            compiler()->SubmitError (pError);
                            //errorRelatedSymbol(hiddenProperty);
                            // stop looking for accessors
                            needGet = needSet = false;
                        }

                        //
                        // can't change type on override
                        //
                        if (hiddenProperty->retType != property->retType) {
                            errorSymbolAndRelatedSymbol(property, hiddenProperty, ERR_CantChangeReturnTypeOnOverride);
                            // stop looking for accessors
                            needGet = needSet = false;
                        }


                        if (!property->isDeprecated && hiddenProperty->isDeprecated) {
                            errorSymbolAndRelatedSymbol(property, hiddenProperty, WRN_NonObsoleteOverridingObsolete);
                        }

                        //
                        // check overriden get is virtual
                        //
                        if (needGet && hiddenProperty->methGet) {

                            needGet = false;

                            //
                            // check that accessor hides expected base accessor ...
                            //
                            SYM *hiddenByAccessor = findHiddenSymbol(property->methGet->name, SK_METHSYM, property->methGet->params, cls->baseClass, cls);
                            if (hiddenByAccessor != hiddenProperty->methGet) {
                                //
                                // we have the amusing case of someone introducing a name the same as
                                // our accesssor somewhere between us and our overridden property
                                //
                                errorSymbolSymbolSymbol(property->methGet, hiddenProperty->methGet, hiddenByAccessor, ERR_CantOverrideAccessor);
                            } else {
                                if (!property->methGet->isOverride) {
                                    errorSymbolAndRelatedSymbol(property->methGet, hiddenProperty->methGet, 
                                                                hiddenProperty->methGet->isVirtual ? WRN_NewOrOverrideExpected : WRN_NewRequired);
                                }

                                if (!hiddenProperty->methGet->isVirtual) {
                                    if (hiddenProperty->methGet->isOverride)
                                        errorSymbolAndRelatedSymbol(property->methGet, hiddenProperty->methGet, ERR_CantOverrideSealed);
                                    else
                                        errorSymbolAndRelatedSymbol(property->methGet, hiddenProperty->methGet, ERR_CantOverrideNonVirtual);
                                }

                                checkForMethodImplOnOverride(property->methGet, hiddenProperty->methGet);

                                property->methGet->isOverride = true;
                                property->methGet->hasCmodOpt = hiddenProperty->methGet->hasCmodOpt;
                                property->hasCmodOpt |= property->methGet->hasCmodOpt;
                            }
                        }

                        //
                        // check overridden set is virtual
                        //
                        if (needSet && hiddenProperty->methSet) {
                            needSet = false;

                            //
                            // check that accessor hides expected base accessor ...
                            //
                            SYM *hiddenByAccessor = findHiddenSymbol(property->methSet->name, SK_METHSYM, property->methSet->params, cls->baseClass, cls);
                            if (hiddenByAccessor != hiddenProperty->methSet) {
                                //
                                // we have the amusing case of someone introducing a name the same as
                                // our accesssor somewhere between us and our overridden property
                                //
                                errorSymbolSymbolSymbol(property->methSet, hiddenProperty->methSet, hiddenByAccessor, ERR_CantOverrideAccessor);
                            } else {
                                if (!property->methSet->isOverride) {
                                    errorSymbolAndRelatedSymbol(property->methSet, hiddenProperty->methSet, 
                                                                hiddenProperty->methSet->isVirtual ? WRN_NewOrOverrideExpected : WRN_NewRequired);
                                }

                                if (!hiddenProperty->methSet->isVirtual) {
                                    if (hiddenProperty->methSet->isOverride)
                                        errorSymbolAndRelatedSymbol(property->methSet, hiddenProperty->methSet, ERR_CantOverrideSealed);
                                    else
                                        errorSymbolAndRelatedSymbol(property->methSet, hiddenProperty->methSet, ERR_CantOverrideNonVirtual);
                                }

                                checkForMethodImplOnOverride(property->methSet, hiddenProperty->methSet);

                                property->methSet->isOverride = true;
                                property->methSet->hasCmodOpt = hiddenProperty->methSet->hasCmodOpt;
                                property->hasCmodOpt |= property->methSet->hasCmodOpt;
                            }
                        }

                    //
                    // keep looking as long as we need to find matches for an accessor and
                    // the property we've currently found is an override and there's
                    // another property in a more base class
                    //
                    } while ((needGet || needSet) &&
                             (hiddenProperty->isOverride) &&
                             (hiddenProperty->getClass()->baseClass) &&
                             (hiddenSymbol = findHiddenSymbol(property->name, SK_PROPSYM, property->params, hiddenProperty->getClass()->baseClass, cls)) &&
                             (hiddenSymbol->kind == SK_PROPSYM));

                    //
                    // did we find all base accessors required
                    //
                    if (needGet) {
                        errorSymbolAndRelatedSymbol(property->methGet, lastProperty, ERR_NoGetToOverride);
                    }
                    if (needSet) {
                        errorSymbolAndRelatedSymbol(property->methSet, lastProperty, ERR_NoSetToOverride);
                    }
                }
            } else {
                //
                // didn't find a hidden base symbol to override
                //
                errorSymbol(property, ERR_OverrideNotExpected);
            }
        }

        //
        // NOTE: that inner classes can derive from outer classes
        //       so private virtual properties are OK.
        //

        //
        // check that new virtual accessors aren't in sealed classes
        //
        if (cls->isSealed && !property->isOverride) {
            if (property->methGet && property->methGet->isVirtual) {
                errorSymbolAndRelatedSymbol(property->methGet, cls, ERR_NewVirtualInSealed);
            }
            if (property->methSet && property->methSet->isVirtual) {
                errorSymbolAndRelatedSymbol(property->methSet, cls, ERR_NewVirtualInSealed);
            }
        }

        //
        // check that abstract properties are in abstract classes
        //
        if (!cls->isAbstract) {
            if (property->methGet && property->methGet->isAbstract) {
                errorSymbolAndRelatedSymbol(property->methGet, cls, ERR_AbstractInConcreteClass);
            }
            if (property->methSet && property->methSet->isAbstract) {
                errorSymbolAndRelatedSymbol(property->methSet, cls, ERR_AbstractInConcreteClass);
            }
        }

        //
        // check that the property body existance matches with the abstractness
        // attribute of the property
        //
        if (property->methGet) {
            if (property->methGet->isAbstract || property->methGet->isExternal) {
                if ((property->methGet->parseTree->other & NFEX_METHOD_NOBODY) == 0) {
                    // found an abstract property with a body
                    errorSymbol(property->methGet, property->methGet->isAbstract ? ERR_AbstractHasBody : ERR_ExternHasBody);
                }
            } else {
                if (property->methGet->parseTree->other & NFEX_METHOD_NOBODY) {
                    // found non-abstract property without body
                    errorSymbol(property->methGet, ERR_ConcreteMissingBody);
                }
            }
        }
        if (property->methSet) {
            if (property->methSet->isAbstract || property->methSet->isExternal) {
                if ((property->methSet->parseTree->other & NFEX_METHOD_NOBODY) == 0) {
                    // found an abstract property with a body
                    errorSymbol(property->methSet, property->methSet->isAbstract ? ERR_AbstractHasBody : ERR_ExternHasBody);
                }
            } else {
                if (property->methSet->parseTree->other & NFEX_METHOD_NOBODY) {
                    // found non-abstract property without body
                    errorSymbol(property->methSet, ERR_ConcreteMissingBody);
                }
            }
        }

        //
        // check for internal virtuals
        //
        if (property->methGet && property->methGet->isVirtual && property->methGet->access == ACC_INTERNAL && !cls->isSealed && cls->hasExternalAccess()) {
            errorSymbol(property->methGet, WRN_InternalVirtual);
        }
        if (property->methSet && property->methSet->isVirtual && property->methSet->access == ACC_INTERNAL && !cls->isSealed && cls->hasExternalAccess()) {
            errorSymbol(property->methSet, WRN_InternalVirtual);
        }
    } else {
        //
        // property is an explicit interface implementation
        //
        property->explicitImpl = checkExplicitImpl(
                            property->parseTree->asANYPROPERTY()->pName,
                            property->retType,
                            property->params,
                            property,
                            cls);
        if (property->explicitImpl && property->explicitImpl->kind == SK_PROPSYM) {
            PROPSYM *explicitImpl = property->explicitImpl->asPROPSYM();
            if (property->methGet) {
                property->methGet->isMetadataVirtual = true;

                //
                // must have a body
                //
                if (property->methGet->parseTree->other & NFEX_METHOD_NOBODY && !property->methGet->isExternal) {
                    errorSymbol(property->methGet, ERR_ConcreteMissingBody);
                } else if (property->methGet->isExternal && (property->methGet->parseTree->other & NFEX_METHOD_NOBODY) == 0) {
                    // found an extern method with a body
                    errorSymbol(property->methGet, ERR_ExternHasBody);
                }


                //
                // must have same accessors as interface member
                //
                if (!explicitImpl->asPROPSYM()->methGet) {
                    errorSymbolAndRelatedSymbol(property->methGet, explicitImpl, ERR_ExplicitPropertyAddingAccessor);
                } else {
                    property->methGet->explicitImpl = explicitImpl->asPROPSYM()->methGet;
                }
            } else {
                if (explicitImpl->kind == SK_PROPSYM && explicitImpl->asPROPSYM()->methGet) {
                    errorSymbolAndRelatedSymbol(property, explicitImpl->asPROPSYM()->methGet, ERR_ExplicitPropertyMissingAccessor);
                }
            }
            if (property->methSet) {
                property->methSet->isMetadataVirtual = true;

                //
                // must have a body
                //
                if (property->methSet->parseTree->other & NFEX_METHOD_NOBODY && !property->methSet->isExternal) {
                    errorSymbol(property->methSet, ERR_ConcreteMissingBody);
                } else if (property->methSet->isExternal && (property->methSet->parseTree->other & NFEX_METHOD_NOBODY) == 0) {
                    // found an extern method with a body
                    errorSymbol(property->methSet, ERR_ExternHasBody);
                }

                //
                // must have same accessors as interface member
                //
                if (!explicitImpl->asPROPSYM()->methSet) {
                    errorSymbolAndRelatedSymbol(property->methSet, explicitImpl, ERR_ExplicitPropertyAddingAccessor);
                } else {
                    property->methSet->explicitImpl = explicitImpl->asPROPSYM()->methSet;
                }
            } else {
                if (explicitImpl->kind == SK_PROPSYM && explicitImpl->asPROPSYM()->methSet) {
                    errorSymbolAndRelatedSymbol(property, explicitImpl->asPROPSYM()->methSet, ERR_ExplicitPropertyMissingAccessor);
                }
            }
        }
    }

    if (property->isDeprecated) {
        if (property->methGet) {
            property->methGet->isDeprecated         = true;
            property->methGet->isDeprecatedError    = property->isDeprecatedError;
            property->methGet->deprecatedMessage    = property->deprecatedMessage;
        }
        if (property->methSet) {
            property->methSet->isDeprecated         = true;
            property->methSet->isDeprecatedError    = property->isDeprecatedError;
            property->methSet->deprecatedMessage    = property->deprecatedMessage;
        }
    }
}


// do prepare stage for an event in a class or struct (not interface)
void CLSDREC::prepareEvent(EVENTSYM * event)
{
    BASENODE * tree = event->parseTree;
    AGGSYM * cls = event->getClass();

    unsigned flags = (tree->kind == NK_VARDECL) ? tree->pParent->flags 
                                                : tree->flags;

    reportDeprecatedType(event->parseTree, event, event->type);

    // Issue error if the event type is not a delegate.
    if (event->type->kind != SK_AGGSYM || ! event->type->asAGGSYM()->isDelegate) {
        errorSymbol(event, ERR_EventNotDelegate);
    }

    if (event->name == NULL) 
    {
        // event must be an explicit implementation. 
        EVENTSYM * explicitImpl;

        explicitImpl = checkExplicitImpl(
                            event->parseTree->asANYPROPERTY()->pName,
                            event->type,
                            NULL,
                            event,
                            event->parent->asAGGSYM())->asEVENTSYM();

        event->explicitImpl = explicitImpl;

        if (explicitImpl) {
            // Set up the accessors as explicit interfaces.
            event->methAdd->explicitImpl = explicitImpl->methAdd;
            event->methAdd->isMetadataVirtual = true;
            event->methRemove->explicitImpl = explicitImpl->methRemove;
            event->methRemove->isMetadataVirtual = true;

            // must have a body
            if (event->methAdd->parseTree->other & NFEX_METHOD_NOBODY) {
                errorSymbol(event->methAdd, ERR_ConcreteMissingBody);
            }
            if (event->methRemove->parseTree->other & NFEX_METHOD_NOBODY) {
                errorSymbol(event->methRemove, ERR_ConcreteMissingBody);
            }
        }
    }
    else {
        // not explicit event implementation.
        if (!(flags & NF_MOD_OVERRIDE)) {
            checkSimpleHiding(event, tree->flags);
        }
        else {
            // This is an event marked "override".
            SYM * hiddenSymbol;
            hiddenSymbol = findHiddenSymbol(event->name, cls->baseClass, cls);
            if (hiddenSymbol) {
                if (hiddenSymbol->kind != SK_EVENTSYM) {
                    // Found a non-event. "override" is in error.
                    errorSymbolAndRelatedSymbol(event, hiddenSymbol, ERR_CantOverrideNonEvent);
                }
                else {
                    // Found an event that we will override.
                    EVENTSYM * hiddenEvent = hiddenSymbol->asEVENTSYM();

                    if (hiddenEvent->isBogus) {
                        errorSymbolAndRelatedSymbol(event, hiddenEvent, ERR_CantOverrideBogusMethod);
                    }
                    else {
                        // Check that access is the same.
                        if (hiddenEvent->access != event->access) {
                            CError  *pError = make_errorSymbolStrStr(event, ERR_CantChangeAccessOnOverride,
                                compiler()->ErrAccess(hiddenEvent->access), compiler()->ErrSym(hiddenEvent));
                            AddRelatedSymbolLocations (pError, hiddenEvent);
                            compiler()->SubmitError (pError);
                        }
                        // Make sure both accessors are virtual.
                        if (! hiddenEvent->methAdd->isVirtual || ! hiddenEvent->methRemove->isVirtual) {
                            if (hiddenEvent->methAdd->isOverride || hiddenEvent->methRemove->isOverride)
                                errorSymbolAndRelatedSymbol(event, hiddenEvent, ERR_CantOverrideSealed);
                            else
                                errorSymbolAndRelatedSymbol(event, hiddenEvent, ERR_CantOverrideNonVirtual);
                        }
            
                        if (!event->isDeprecated && hiddenEvent->isDeprecated) {
                            errorSymbolAndRelatedSymbol(event, hiddenEvent, WRN_NonObsoleteOverridingObsolete);
                        }

                        // Make sure type of the event is the same.
                        if (hiddenEvent->type != event->type) {
                            errorSymbolAndRelatedSymbol(event, hiddenEvent, ERR_CantChangeReturnTypeOnOverride);
                        }
                        else {
                            // Check that accessor hides expected base accessor.
                            SYM *hiddenByAccessor = findHiddenSymbol(event->methAdd->name, SK_METHSYM, event->methAdd->params, cls->baseClass, cls);
                            if (hiddenByAccessor != hiddenEvent->methAdd) 
                                errorSymbolSymbolSymbol(event->methAdd, hiddenEvent->methAdd, hiddenByAccessor, ERR_CantOverrideAccessor);
                            else
                                checkForMethodImplOnOverride(event->methAdd, hiddenEvent->methAdd);

                            hiddenByAccessor = findHiddenSymbol(event->methRemove->name, SK_METHSYM, event->methRemove->params, cls->baseClass, cls);
                            if (hiddenByAccessor != hiddenEvent->methRemove) 
                                errorSymbolSymbolSymbol(event->methRemove, hiddenEvent->methRemove, hiddenByAccessor, ERR_CantOverrideAccessor);
                            else
                                checkForMethodImplOnOverride(event->methRemove, hiddenEvent->methRemove);

                            // Set the override bits.
                            event->methAdd->isOverride = true;
                            event->methAdd->hasCmodOpt = hiddenEvent->methAdd->hasCmodOpt;
                            event->methRemove->isOverride = true;
                            event->methRemove->hasCmodOpt = hiddenEvent->methRemove->hasCmodOpt;
                        }
                    }
                }
            }
            else {
                // didn't find base member to override.
                errorSymbol(event, ERR_OverrideNotExpected);
            }
        } 

        // Check that abstract-ness matches with whether have a body for each accessor.
        if (tree->kind != NK_VARDECL) {
            if (event->methAdd->isAbstract || event->methAdd->isExternal) {
                if ((event->methAdd->parseTree->other & NFEX_METHOD_NOBODY) == 0) {
                    // found an abstract event with a body
                    errorSymbol(event->methAdd, event->methAdd->isAbstract ? ERR_AbstractHasBody : ERR_ExternHasBody);
                }
            } else {
                if (event->methAdd->parseTree->other & NFEX_METHOD_NOBODY) {
                    // found non-abstract event without body
                    errorSymbol(event->methAdd, ERR_ConcreteMissingBody);
                }
            }
            if (event->methRemove->isAbstract || event->methRemove->isExternal) {
                if ((event->methRemove->parseTree->other & NFEX_METHOD_NOBODY) == 0) {
                    // found an abstract event with a body
                    errorSymbol(event->methRemove, event->methRemove->isAbstract ? ERR_AbstractHasBody : ERR_ExternHasBody);
                }
            } else {
                if (event->methRemove->parseTree->other & NFEX_METHOD_NOBODY) {
                    // found non-abstract event without body
                    errorSymbol(event->methRemove, ERR_ConcreteMissingBody);
                }
            }
        }
        
        //
        // warn on internal virtuals
        //
        if (event->isVirtualSym() && event->access == ACC_INTERNAL && !cls->isSealed && cls->hasExternalAccess()) {
            errorSymbol(event, WRN_InternalVirtual);
        }
    }

    // Check that virtual/abstractness is OK.
    if (cls->isSealed && event->methAdd->isVirtual && !event->methAdd->isOverride) {
        errorSymbolAndRelatedSymbol(event, cls, ERR_NewVirtualInSealed);
    }

    if (!cls->isAbstract && event->methAdd->isAbstract) {
        errorSymbolAndRelatedSymbol(event, cls, ERR_AbstractInConcreteClass);
    }


}

// define the fields in a given field node, this involves checking that the
// flags for the decls have be specified correctly, and actually entering the
// field into the symbol table...
//
// returns true if any of the fields need initialization in the static
// constructor: i.e., are static, non-const, and have an explicit initializer (or are of struct type).
bool CLSDREC::defineFields(FIELDNODE * pFieldTree, AGGSYM * cls)
{
    bool isEvent;    // Is it an event declaration?

    ASSERT(!cls->isEnum);

    isEvent = !!(pFieldTree->other & NFEX_EVENT);

    if (cls->isInterface && !isEvent) {
        errorFile(pFieldTree, cls->getInputFile(), ERR_InterfacesCantContainFields);
        return false;
    }

    TYPESYM * type = bindType(pFieldTree->pType, cls, BTF_NODECLARE | BTF_NODEPRECATED, NULL);
    if (!type) return false;

    if (type->isSpecialByRefType()) {
        errorStrFile(pFieldTree, cls->getInputFile(), ERR_FieldCantBeRefAny, compiler()->ErrSym(type));
        return false;
    }
    if (type->kind == SK_VOIDSYM) {
        errorFile(pFieldTree->pType, cls->getInputFile(), ERR_FieldCantHaveVoidType);
        return false;
    }

    unsigned flags = pFieldTree->flags;
    bool error = true;
    bool needStaticCtor = false;
    MEMBVARSYM fakeFieldSym;   // used only for abstract events

    NODELOOP(pFieldTree->pDecls, VARDECL, vardecl)

        NAME * name = vardecl->pName->pName;

        //
        // check for name same as that of parent aggregate
        // check for conflicting field...
        //
        if (!checkForBadMember(name, vardecl->pName, cls)) {
            continue;
        }

        MEMBVARSYM * rval;
        if (cls->isInterface || (isEvent && (flags & (NF_MOD_ABSTRACT | NF_MOD_EXTERN)))) {
            // An abstract or extern event does not declare an underlying field. In order
            // to keep the rest of this function simpler, we declare a "fake" field on the 
            // stack and use it.
            ASSERT(isEvent);
            memset(&fakeFieldSym, 0, sizeof(fakeFieldSym));
            rval = &fakeFieldSym;
            rval->name = name;
            rval->parent = cls;
            rval->kind = SK_MEMBVARSYM;
        }
        else {
            // The usual case.
            rval = compiler()->symmgr.CreateMembVar(name, cls);
        }

        rval->type = type;

        // set the value parse tree:
        rval->parseTree = vardecl;
        unsigned allowableFlags = 
            cls->allowableMemberAccess() |
            NF_MOD_UNSAFE |
            NF_MOD_NEW;

        if (cls->isInterface) { 
            // event in interface can't have initializer.
            if (vardecl->pArg) 
                errorSymbol(rval, ERR_InterfaceEventInitializer);
        }
        else {  // ! cls->isInterface
            allowableFlags |= NF_MOD_STATIC;
        }

        // abstract event can't have initializer
        if (isEvent && (flags & NF_MOD_ABSTRACT) && vardecl->pArg)
            errorSymbol(rval, ERR_AbstractEventInitializer);

        if (pFieldTree->kind == NK_CONST) {
            rval->isUnevaled = true;
            rval->isStatic = true; // consts are implicitly static.

            allowableFlags &= ~NF_MOD_UNSAFE; // consts can't be marked unsafe.

            // parser guarnatees this
            ASSERT(rval->parseTree->asVARDECL()->pArg);

            // can't have static modifier on constant delcaration
            if (error && (flags & NF_MOD_STATIC)) {
                errorSymbol(rval, ERR_StaticConstant);
            }

            // constants of decimal type can't be expressed in COM+ as a literal
            // so we need to initialize them in the static constructor
            if (rval->type->isPredefType(PT_DECIMAL)) {
                needStaticCtor = true;
            }
        } 
        else if (isEvent) {
            // events can be virtual, but in interfaces the modifier is implied and can't be specified directly.
            if (! cls->isInterface) {
                allowableFlags |= NF_MOD_VIRTUAL | NF_MOD_ABSTRACT | NF_MOD_EXTERN | NF_MOD_OVERRIDE | NF_MOD_SEALED;   
            }
        }
        else {  
            // events and consts can't be readonly or volatile.
            allowableFlags |= NF_MOD_READONLY | NF_MOD_VOLATILE;
        }

        checkFlags(rval,
            allowableFlags,
            flags,
            error);
        
        if (flags & NF_MOD_STATIC) {
            rval->isStatic = true;

            // If it had an initializer and isn't a const, 
            // then we need a static initializer.
            if (vardecl->pArg && !rval->isUnevaled)
            {
                needStaticCtor = true;
            }
        } else if (!rval->isStatic && cls->isStruct && vardecl->pArg) {
            errorSymbol(rval, ERR_FieldInitializerInStruct);
        }
        if (flags & NF_MOD_READONLY) {
            rval->isReadOnly = true;
        }
        if (flags & NF_MOD_VOLATILE) {
            FUNDTYPE ft = type->fundType();
            if (ft != FT_REF  && ft != FT_PTR && 
                ft != FT_R4 && ft != FT_I4 && ft != FT_U4 &&
                ft != FT_I2 && ft != FT_U2 &&
                ft != FT_I1 && ft != FT_U1) 
            {   
                errorSymbolStr(rval, ERR_VolatileStruct, compiler()->ErrSym(type));
            }
            else if (rval->isReadOnly) 
                errorSymbol(rval, ERR_VolatileAndReadonly);
            else
                rval->isVolatile = true;
        }

        //
        // check for new protected in a sealed class
        //
        if ((rval->access == ACC_PROTECTED || rval->access == ACC_INTERNALPROTECTED) &&
            cls->isSealed) {
            errorSymbol(rval, cls->isStruct ? ERR_ProtectedInStruct : WRN_ProtectedInSealed);
        }

        if (isEvent && (flags & NF_MOD_SEALED) && !(flags & NF_MOD_OVERRIDE))
        {
            errorSymbol(rval, ERR_SealedNonOverride);
        }

        // Check that the field type is as accessible as the field itself.
        checkConstituentVisibility(rval, type, ERR_BadVisFieldType);

        // If this is an event field, define the corresponding event symbol.
        if (isEvent) {
            EVENTSYM * event = defineEvent(rval, vardecl);
            if (cls->isInterface || (isEvent && (flags & (NF_MOD_ABSTRACT | NF_MOD_EXTERN))))
                event->implementation = NULL;  // remove link to "fake" field symbol.
            if (flags & NF_MOD_ABSTRACT && flags & NF_MOD_EXTERN)
                errorSymbol(event, ERR_AbstractAndExtern);
        }

        // don't want to give duplicate errors on subsequent decls...
        error = false;

        ASSERT(rval->parseTree->kind == NK_VARDECL);
    ENDLOOP;

    return needStaticCtor;
}

SYM *CLSDREC::checkExplicitImpl(BASENODE *name, PTYPESYM returnType, PTYPESYM *params, SYM *sym, AGGSYM *cls)
{
    BASENODE *ifaceName;
    NAME *memberName;

    if (sym->kind == SK_PROPSYM && sym->asPROPSYM()->cParams > 0)
    {
        //
        // property is an explicit interface implementation
        //

        //
        // get the interface nameNode and member name
        //
        ifaceName = name;
        memberName = compiler()->namemgr->GetPredefName(PN_INDEXERINTERNAL);
    } else {
        ifaceName = name->asDOT()->p1;
        memberName = name->asDOT()->p2->asNAME()->pName;
    }

    AGGSYM *iface = bindTypeName(ifaceName, cls, BTF_NONE, NULL)->asAGGSYM();

    unsigned mask = 1 << sym->kind;
    if (sym->kind == SK_PROPSYM && sym->asPROPSYM()->isEvent)
        mask = MASK_EVENTSYM;

    if (iface) {
        if (!iface->isInterface) {
            errorNameRefSymbol(ifaceName, cls, iface, ERR_ExplicitInterfaceImplementationNotInterface);
            return NULL;
        } else {

            //
            // check that we implement the interface in question
            //
            if (!cls->allIfaceList->contains(iface)) {
                errorSymbolAndRelatedSymbol(sym, iface, ERR_ClassDoesntImplementInterface);
            }

            //
            // find member on interface. May need to bring the interface
            // up to declared state first.
            //
            prepareAggregate(iface);

            SYM *explicitImpl = compiler()->symmgr.LookupGlobalSym(memberName, iface, mask);
            for (;;) {
                if (!explicitImpl)
                    break;
                if (explicitImpl->kind == SK_EVENTSYM) {
                    if (explicitImpl->asEVENTSYM()->type == returnType)
                        break;
                }
                else {
                    if (explicitImpl->asMETHPROPSYM()->params == params && explicitImpl->asMETHPROPSYM()->retType == returnType)
                        break;
                }

                // Try next one.
                explicitImpl = compiler()->symmgr.LookupNextSym(explicitImpl, explicitImpl->parent, mask);
            }

            if (!explicitImpl) {
                if (ifaceName->pParent->kind == NK_DOT) {
                    errorNameRefSymbol(ifaceName->pParent, cls, iface, ERR_InterfaceMemberNotFound);
                } else {
                    errorNameRefSymbol(memberName, ifaceName, cls, iface, ERR_InterfaceMemberNotFound);
                }
                return NULL;
            } else {
                //
                // check for duplicate explicit implementation
                //
                SYM *duplicateImpl = findExplicitInterfaceImplementation(cls, explicitImpl);
                if (duplicateImpl) {
                    if (ifaceName->pParent->kind == NK_DOT) {
                        errorNameRefStrSymbol(ifaceName->pParent, cls, compiler()->ErrSym(cls), duplicateImpl, ERR_MemberAlreadyExists);
                    } else {
                        errorNameRefStrSymbol(memberName, ifaceName, cls, compiler()->ErrSym(cls), duplicateImpl, ERR_MemberAlreadyExists);
                    }
                    return NULL;
                }

                return explicitImpl;
            }
        }
    } else {
        //
        // interface didn't exist, error already reported
        //
        return NULL;
    }
}

// Check a "params" style signature to make sure the last argument is of correct type.
void CLSDREC::checkParamsArgument(BASENODE * tree, AGGSYM * cls, unsigned cParam, PTYPESYM *paramTypes)
{
    if (!cParam || paramTypes[cParam-1]->kind != SK_ARRAYSYM || paramTypes[cParam-1]->asARRAYSYM()->rank != 1) {
        if (cParam && paramTypes[cParam-1]->kind == SK_PARAMMODSYM)
            errorFile(tree, cls->getInputFile(), ERR_ParamsCantBeRefOut);
        else
            errorFile(tree, cls->getInputFile(), ERR_ParamsMustBeArray);
    }
}

// defines a method by entering it into the symbol table, also checks for
// duplicates, and makes sure that the flags don't conflict.
METHSYM *CLSDREC::defineMethod(METHODNODE *pMethodTree, AGGSYM * cls)
{
    NAME * name;
    BASENODE * nameTree = pMethodTree->pName;

    bool isStaticCtor = false;

    // figure out the name...
    if (pMethodTree->kind == NK_CTOR) {
        if (cls->isInterface) {
            errorFile(pMethodTree, cls->getInputFile(), ERR_InterfacesCantContainConstructors);
            return NULL;
        }

        if (pMethodTree->flags & NF_MOD_STATIC) {
            name = compiler()->namemgr->GetPredefName(PN_STATCTOR);
            isStaticCtor = true;
        } else {
            name = compiler()->namemgr->GetPredefName(PN_CTOR);
        }
    } else if (pMethodTree->kind == NK_DTOR) {
        if (!cls->isClass) {
            errorFile(pMethodTree, cls->getInputFile(), ERR_OnlyClassesCanContainDestructors);
            return NULL;
        }

        name = compiler()->namemgr->GetPredefName(PN_DTOR);
    } else if (nameTree->kind == NK_NAME) {
        name = nameTree->asNAME()->pName;
    } else {
        ASSERT(nameTree->kind == NK_DOT);

        if (!cls->isClass && !cls->isStruct) {
            errorNameRef(nameTree, cls, ERR_ExplicitInterfaceImplementationInNonClassOrStruct);
            return NULL;
        }

        // we handle explicit interface implementations in the prepare stage
        // when we have the complete list of interfaces
        name = NULL;
    }

    TYPESYM * retType;

    // common case: fewer than 8 parameters
    unsigned maxParams = 8;
    unsigned cParam = 0;
    PTYPESYM initTypeArray[8];
    PTYPESYM * paramTypes = &(initTypeArray[0]);
    bool isOldStyleVarargs = !!(pMethodTree->other & NFEX_METHOD_VARARGS);
    bool isParamArray = !!(pMethodTree->other & NFEX_METHOD_PARAMS);

    if (isOldStyleVarargs && isParamArray) {
        errorFile(pMethodTree, cls->getInputFile(), ERR_VarargsAndParams);
    }

    NODELOOP(pMethodTree->pParms, PARAMETER, param)
        // double the param list if we reached the end... (guarantee at least 1 free space at end)
        if (cParam + 1 == maxParams) {
            maxParams *= 2;
            TYPESYM ** newArray = (TYPESYM **) _alloca(maxParams * sizeof(PTYPESYM));
            memcpy(newArray, paramTypes, cParam * sizeof(PTYPESYM));
            paramTypes = newArray;
        }
        // bind the type, and wrap it if the variable is byref
        retType = bindType(param->pType, cls, BTF_NODECLARE | BTF_NODEPRECATED, NULL);
        if (!retType) return NULL;
        if (param->flags & (NF_PARMMOD_REF | NF_PARMMOD_OUT)) {
            retType = compiler()->symmgr.GetParamModifier(retType, param->flags & NF_PARMMOD_OUT ? true : false);
            if (retType->asPARAMMODSYM()->paramType()->isSpecialByRefType()) {
                errorStrFile(param, cls->getInputFile(), ERR_MethodArgCantBeRefAny, compiler()->ErrSym(retType));
                return NULL;
            }
        }
        paramTypes[cParam++] = retType;
    ENDLOOP;

    if (isParamArray) {
        checkParamsArgument(pMethodTree, cls, cParam, paramTypes);
    }

    if (isOldStyleVarargs) {
        paramTypes[cParam++] = compiler()->symmgr.GetArglistSym();
    }

    // get the formal (and unique) param array for the given types...
    TYPESYM ** params = compiler()->symmgr.AllocParams(cParam, paramTypes);

    // bind the return type, or assume void for constructors...
    if (pMethodTree->pType) {
        retType = bindType(pMethodTree->pType, cls, BTF_NODECLARE | BTF_NODEPRECATED, NULL);
        if (!retType) return NULL;
        if (retType->isSpecialByRefType() && !cls->isSpecialByRefType()) { // Allow return type for themselves
            errorStrFile(pMethodTree, cls->getInputFile(), ERR_MethodReturnCantBeRefAny, compiler()->ErrSym(retType));
            return NULL;
        }
    } else {
        ASSERT(pMethodTree->kind == NK_CTOR || pMethodTree->kind == NK_DTOR);
        retType = compiler()->symmgr.GetVoid();
    }

    if (name) {
        if (cls->isStruct && (pMethodTree->kind == NK_CTOR) && (cParam == 0) && !isStaticCtor) {
            errorFile(pMethodTree, cls->getInputFile(), ERR_StructsCantContainDefaultContructor);
            return NULL;
        }

        // for non-explicit interface implementations
        // find another method with the same signature
        if (!checkForBadMember(name, SK_METHSYM, params, pMethodTree, cls)) {
            return NULL;
        }

    } else {
        //
        // method is an explicit interface implementation
        //
    }

    METHSYM * rval = compiler()->symmgr.CreateMethod(name, cls);

    rval->parseTree = pMethodTree;

    rval->retType = retType;
    rval->params = params;
    rval->cParams = cParam;
    rval->isVarargs = isOldStyleVarargs;
    rval->isParamArray = isParamArray;
    
    if (pMethodTree->flags & NF_MOD_STATIC) {
        rval->isStatic = true;
    }

    // constructors have a much simpler set of allowed flags:
    if (pMethodTree->kind == NK_CTOR) {
        rval->isCtor = true;

        //
        // NOTE:
        //
        // NF_CTOR_BASE && NF_CTOR_THIS have the same value as
        // NF_MOD_ABSTRACT && NF_MOD_NEW we mask them out so that
        // we don't get spurious conflicts with NF_MOD_STATIC
        //
        checkFlags(rval,
            cls->allowableMemberAccess() | NF_MOD_STATIC | NF_MOD_EXTERN | NF_MOD_UNSAFE,
            (NODEFLAGS) (pMethodTree->flags));
        if (rval->isStatic) {
            //
            // static constructors can't have explicit constructor calls,
            // access modifiers or parameters
            //
            if (pMethodTree->other & (NFEX_CTOR_BASE | NFEX_CTOR_THIS)) {
                errorSymbol(rval, ERR_StaticConstructorWithExplicitConstructorCall);
            }
            if (pMethodTree->flags & NF_MOD_ACCESSMODIFIERS) {
                errorSymbol(rval, ERR_StaticConstructorWithAccessModifiers);
            }
            if (cParam) {
                errorSymbol(rval, ERR_StaticConstParam);
            }
        }

        if (pMethodTree->flags & NF_MOD_EXTERN) {
            rval->isExternal = true;
        }
    } else if (pMethodTree->kind == NK_DTOR) {

        // no modifiers are allowed on destructors
        checkFlags(rval, NF_MOD_UNSAFE | NF_MOD_EXTERN, (NODEFLAGS) (pMethodTree->flags));

        rval->access = ACC_PROTECTED;
        rval->isDtor = true;
        rval->isVirtual = true;
        rval->isMetadataVirtual = true;
        rval->isOverride = true;
        if (pMethodTree->flags & NF_MOD_EXTERN) {
            rval->isExternal = true;
        }

    } else if (name) {
        // methods a somewhat more complicated one...
        if (pMethodTree->flags & NF_MOD_ABSTRACT || cls->isInterface) {
            rval->isAbstract = true;
        }
        if (pMethodTree->flags & NF_MOD_OVERRIDE) {
            rval->isOverride = true;
        }
        if (pMethodTree->flags & NF_MOD_EXTERN) {
            rval->isExternal = true;
            if (rval->isAbstract) {
                errorSymbol(rval, ERR_AbstractAndExtern);
            }
        }
        if (pMethodTree->flags & NF_MOD_VIRTUAL ||
            rval->isOverride ||
            rval->isAbstract) {
            // abstract implies virtual
            rval->isVirtual = true;
            rval->isMetadataVirtual = true;
        }
        if (pMethodTree->flags & NF_MOD_SEALED) {
            if (!rval->isOverride)
                errorSymbol(rval, ERR_SealedNonOverride);
            else {
                // Note: a sealed override is marked with isOverride=true, isVirtual=false.
                rval->isVirtual = false;
                ASSERT(rval->isMetadataVirtual);
            }
        }

        unsigned allowableFlags = cls->allowableMemberAccess() | NF_MOD_EXTERN;
        if (cls->isClass) {
            allowableFlags |= NF_MOD_ABSTRACT | NF_MOD_VIRTUAL | NF_MOD_NEW | NF_MOD_OVERRIDE | NF_MOD_SEALED | NF_MOD_STATIC | NF_MOD_UNSAFE;
        } else if (cls->isStruct) {
            allowableFlags |= NF_MOD_NEW | NF_MOD_OVERRIDE | NF_MOD_STATIC | NF_MOD_UNSAFE;
        } else {
            ASSERT(cls->isInterface);
            // interface members can only have new
            allowableFlags |= NF_MOD_NEW | NF_MOD_UNSAFE;
        }
        checkFlags(rval,allowableFlags,
            (NODEFLAGS) pMethodTree->flags);

        if (rval->isVirtual && rval->access == ACC_PRIVATE) {
            errorSymbol(rval, ERR_VirtualPrivate);
        }

    } else {
        // explicit interface implementation
        // can't have any flags
        checkFlags(rval, NF_MOD_UNSAFE | NF_MOD_EXTERN, (NODEFLAGS) pMethodTree->flags);
        rval->isMetadataVirtual = true; // explicit impls need to be virtual when emitted
        if (pMethodTree->flags & NF_MOD_EXTERN)
            rval->isExternal = true;
    }

    //
    // check for new protected in a sealed class
    //
    if ((rval->access == ACC_PROTECTED || rval->access == ACC_INTERNALPROTECTED) &&
        !rval->isOverride &&
        cls->isSealed) {
        errorSymbol(rval, cls->isStruct ? ERR_ProtectedInStruct : WRN_ProtectedInSealed);
    }

    // Check return and parameter types for correct visibility.
    checkConstituentVisibility(rval, retType, ERR_BadVisReturnType);
    if (rval->isVarargs) cParam--; // don't count the sentinel
    for (unsigned i = 0; i < cParam; ++i) 
        checkConstituentVisibility(rval, params[i], ERR_BadVisParamType);

    return rval;
}

void CLSDREC::findEntryPoint(AGGSYM* cls)
{
    // We've found the specified class, now let's look for a Main
    METHSYM *method;

    method = compiler()->symmgr.LookupGlobalSym(compiler()->namemgr->GetPredefName(PN_MAIN), cls, MASK_METHSYM)->asMETHSYM();

    while (method != NULL) {
        // Must be public, static, void/int Main ()
        // with no args or String[] args
         // If you change this code also change the code in EMITTER::FindEntryPointInClass (it does basically the same thing)
       if (method->isStatic &&
            !method->isPropertyAccessor) {
            if ((method->retType == compiler()->symmgr.GetVoid() || method->retType->isPredefType(PT_INT)) &&
                 (method->cParams == 0 ||
                   (method->cParams == 1 && method->params[0]->kind == SK_ARRAYSYM &&
                    method->params[0]->asARRAYSYM()->elementType()->isPredefType(PT_STRING))))
            {
                compiler()->emitter.SetEntryPoint(method);
            } else {

                errorSymbol(method, WRN_InvalidMainSig);
            }
        }

        SYM *next = method->nextSameName;
        method = NULL;
        while (next != NULL) {
            if (next->kind == SK_METHSYM) {
                method = next->asMETHSYM();
                break;
            }
            next = next->nextSameName;
        }
    }
}

static BOOL IsValidIdentifier(LPCWSTR str)
{
    WCHAR ch;

    ch = *str++;
    if (!IsIdentifierChar(ch))
    {
        return false;
    }

    while (0 != (ch = *str++))
    {
        if (!IsIdentifierCharOrDigit(ch))
            return false;
    }

    return true;
}

// defines a property by entering it into the symbol table, also checks for
// duplicates, and makes sure that the flags don't conflict.
// also adds methods for the property's accessors
void CLSDREC::defineProperty(PROPERTYNODE *propertyNode, AGGSYM *cls)
{
    unsigned cParam = 0;
    PTYPESYM initTypeArray[8];                      // the initial location of the param types
    PTYPESYM *paramTypes = &(initTypeArray[0]);     // the non-unique param types
    PTYPESYM *params = NULL;                        // the unique param types

    BASENODE *nameTree = propertyNode->pName;
    bool isEvent = !!(propertyNode->other & NFEX_EVENT); // is it really an event?
    bool isOldStyleVarargs = false;
    bool isParamArray = false;

    NAME *name;
    if (propertyNode->kind == NK_INDEXER) {
        //
        // we have an indexer property
        //
        if (nameTree) {
            //
            // explicit interface implementation
            //
            if (!cls->isClass && !cls->isStruct) {
                errorNameRef(nameTree, cls, ERR_ExplicitInterfaceImplementationInNonClassOrStruct);
                return;
            }

            // we handle explicit interface implementations in the prepare stage
            // when we have the complete list of interfaces
            name = NULL;
        } else {
            name = NULL;

            //
            // search for a name override
            //
            NODELOOP(propertyNode->pAttr, ATTRDECL, decl)
                if (decl->location == AL_PROPERTY)
                {
                    NODELOOP(decl->pAttr, ATTR, attr)

                        //
                        // check for predefined attributes first
                        //
                        PREDEFATTRSYM *predefAttr = NULL;
                        BASENODE * nameNode = attr->pName;

                        if (nameNode->kind == NK_NAME)
                            predefAttr = compiler()->symmgr.GetPredefAttr(nameNode->asNAME()->pName);
                        if (predefAttr == NULL) {
                            SYM * type = bindTypeName(nameNode, cls, BTF_ATTRIBUTE | BTF_NOERRORS | BTF_NODECLARE, NULL);
                            if (type && type->kind == SK_AGGSYM) {
                                PREDEFATTR pa = type->asAGGSYM()->getPredefAttr();
                                if (pa != PA_COUNT) 
                                    predefAttr = compiler()->symmgr.GetPredefAttr(pa);
                            }
                        }

                        if (predefAttr) 
                        {
                            switch (predefAttr->attr) {

                            case PA_NAME:
                                //
                                // name attributes are checked in the define stage
                                // for indexers, otherwise they are an error
                                //
                                if (name) {
                                    errorNameRef(nameNode, cls, ERR_DuplicateAttribute);
                                } else if (propertyNode->flags & NF_MOD_OVERRIDE) {
                                    errorFile(nameNode, cls->getInputFile(), ERR_NameAttributeOnOverride);
                                } else {
                                    NODELOOP(attr->pArgs, BASE, node)

                                        ATTRNODE *argNode = node->asATTRARG();

                                        //
                                        // validate arguments ...
                                        //
                                        if (argNode->pName) {
                                            errorNameRef(nameNode, cls, ERR_NamedArgumentToAttribute);
                                            break;
                                        } else if (name) {
                                            errorStrFile(argNode, cls->getInputFile(), ERR_TooManyArgumentsToAttribute, compiler()->ErrNameNode(nameNode));
                                            break;
                                        //
                                        // bind the argument expression and evaluate it
                                        // Note: we only handle string literals because we can't evaluate expressions yet
                                        //
                                        } else if (argNode->pArgs->kind != NK_CONSTVAL || (argNode->pArgs->asCONSTVAL()->PredefType() != PT_STRING)) {
                                            errorFile(argNode->pArgs, cls->getInputFile(), ERR_BadAttributeParam);
                                            break;
                                        }

                                        //
                                        // convert the string to a name
                                        //
                                        STRCONST * nameValue = argNode->pArgs->asCONSTVAL()->val.strVal;
                                        name = compiler()->namemgr->AddString(nameValue->text, nameValue->length);

                                        //
                                        // check that the name is a valid C# Identifier
                                        //
                                        if (!IsValidIdentifier(name->text)) {
                                            errorFile(argNode, cls->getInputFile(), ERR_BadArgumentToNameAttribute);
                                        }

                                        //
                                        // continue checking for duplicate name attributes
                                        //
                                    ENDLOOP;
                                }
			    default:
			        break;
                            }
                        }
                    ENDLOOP;
                }
            ENDLOOP;

            //
            // name defaults to Item
            //
            if (!name) {
                name = compiler()->namemgr->GetPredefName(PN_INDEXER);
            }
        }

        //
        // get param types
        //

        // common case: fewer than 8 parameters
        unsigned maxParams = 8;

        isOldStyleVarargs = !!(propertyNode->other & NFEX_METHOD_VARARGS);
        isParamArray = !!(propertyNode->other & NFEX_METHOD_PARAMS);

        if (isOldStyleVarargs) {
            errorFile(propertyNode, cls->getInputFile(), ERR_VarargIndexer);
        }

        NODELOOP(propertyNode->pParms, PARAMETER, param)
            // double the param list if we reached the end...
            // Note that we leave room for the return type at the end of the parameter list
            if ((cParam + 1)== maxParams) {
                maxParams *= 2;
                TYPESYM ** newArray = (TYPESYM **) _alloca(maxParams * sizeof(PTYPESYM));
                memcpy(newArray, paramTypes, cParam * sizeof(PTYPESYM));
                paramTypes = newArray;
            }
            // bind the type, and wrap it if the variable is byref
            TYPESYM *paramType = bindType(param->pType, cls, BTF_NODECLARE | BTF_NODEPRECATED, NULL);
            if (!paramType) return;
            if (param->flags & (NF_PARMMOD_REF | NF_PARMMOD_OUT)) {
                //
                // indexers can't have ref or out params
                //
                errorFile(propertyNode, cls->getInputFile(), ERR_IndexerWithRefParam);
                return;
            }
            paramTypes[cParam++] = paramType;
        ENDLOOP;

        if (isParamArray) {
            checkParamsArgument(propertyNode, cls, cParam, paramTypes);
        }

        // get the formal (and unique) param array for the given types...
        params = compiler()->symmgr.AllocParams(cParam, paramTypes);

        if (!checkForBadMember(name ? compiler()->namemgr->GetPredefName(PN_INDEXERINTERNAL) : NULL, SK_PROPSYM, params, propertyNode, cls)) {
            return;
        }

    } else if (nameTree->kind == NK_NAME) {
        name = nameTree->asNAME()->pName;
        if (!checkForBadMember(name, nameTree, cls)) {
            return;
        }
        params = NULL;
    } else {
        ASSERT(nameTree->kind == NK_DOT);

        if (!cls->isClass && !cls->isStruct) {
            errorNameRef(nameTree, cls, ERR_ExplicitInterfaceImplementationInNonClassOrStruct);
            return;
        }

        // we handle explicit interface implementations in the prepare stage
        // when we have the complete list of interfaces
        name = NULL;
        params = NULL;
    }

    //
    // get the properties type. It can't be void.
    //
    TYPESYM * type = bindType(propertyNode->pType, cls, BTF_NODECLARE | BTF_NODEPRECATED, NULL);
    if (!type) {
        return;
    }
    if (type->kind == SK_VOIDSYM) {
        if (propertyNode->kind == NK_INDEXER) {
            errorFile(propertyNode, cls->getInputFile(), ERR_IndexerCantHaveVoidType);
        } else {
            if (nameTree) {
                errorNameRef(nameTree, cls, ERR_PropertyCantHaveVoidType);
            } else {
                errorNameRef(name, propertyNode, cls, ERR_PropertyCantHaveVoidType);
            }
        }
        return;
    }
    if (type->isSpecialByRefType()) {
        errorStrFile(propertyNode, cls->getInputFile(), ERR_FieldCantBeRefAny, compiler()->ErrSym(type));
        return;
    }
    // tack the return type on after the argument types
    // we've left room for it above
    paramTypes[cParam] = type;

    //
    // create the symbol for the property
    //
    PROPSYM *property;
    PROPSYM fakePropSym;
    if (isEvent) {
        // A new-style event does not declare an underlying property. In order to keep
        // the rest of this function simpler, we declare a "fake" property on the stack
        // and use it.
        memset(&fakePropSym, 0, sizeof(fakePropSym));
        property = &fakePropSym;
        property->name = name;
        property->parent = cls;
        property->kind = SK_PROPSYM;
    }
    else {
        if (propertyNode->kind == NK_INDEXER) {
            property = compiler()->symmgr.CreateIndexer(name, cls);
        } else {
            property = compiler()->symmgr.CreateProperty(name, cls);
        }
    }
    ASSERT(name || !property->getRealName());
    property->parseTree = propertyNode;
    property->retType = type;
    property->params = params;
    property->cParams = cParam;
    property->isParamArray = isParamArray;

    if ((propertyNode->flags & NF_MOD_STATIC) != 0) {
        property->isStatic = true;
    }
    property->isOverride = !!(propertyNode->flags & NF_MOD_OVERRIDE);
    if ((propertyNode->flags & NF_MOD_SEALED) && !property->isOverride) {
        errorSymbol(property, ERR_SealedNonOverride);
    }

    if (isEvent && cls->isInterface) 
        errorSymbol(property, ERR_EventPropertyInInterface);

    //
    // check the flags. Assigns access.
    //
    unsigned allowableFlags;
    if (property->name && cls->isClass) {
        allowableFlags = NF_MOD_ABSTRACT | NF_MOD_EXTERN | NF_MOD_VIRTUAL | NF_MOD_OVERRIDE | NF_MOD_SEALED | NF_MOD_UNSAFE; 
    } else { // interface member or explicit interface impl
        allowableFlags = NF_MOD_UNSAFE;
    }
    if (name && (cls->isClass || cls->isStruct)) {
        allowableFlags |= cls->allowableMemberAccess() | NF_MOD_NEW | NF_MOD_UNSAFE;

        //
        // indexers can't be static
        //
        if (propertyNode->kind != NK_INDEXER) {
            allowableFlags |= NF_MOD_STATIC;
        }
    } else if (cls->isInterface) { // interface members
        // interface members 
        // have no allowable flags
        allowableFlags |= NF_MOD_NEW;
    } else { // explicit interface impls
        ASSERT(!name);
        allowableFlags |= NF_MOD_UNSAFE | NF_MOD_EXTERN;
    }
    checkFlags(property, allowableFlags, (NODEFLAGS) propertyNode->flags);
    if (property->isOverride)
        propertyNode->flags |= (NF_MOD_OVERRIDE & allowableFlags);

    // Check return and parameter types for correct visibility.
    checkConstituentVisibility(property, type, (propertyNode->kind == NK_INDEXER) ? ERR_BadVisIndexerReturn : ERR_BadVisPropertyType);
    for (unsigned i = 0; i < cParam; ++i) 
        checkConstituentVisibility(property, params[i], ERR_BadVisIndexerParam);

    //
    // must have at least one accessor, and an event must have both.
    //
    if (isEvent) {
        if (!propertyNode->pGet || !propertyNode->pSet) 
            errorSymbol(property, ERR_EventNeedsBothAccessors);
    }
    else {
        if (!propertyNode->pGet && !propertyNode->pSet) 
            errorSymbol(property, ERR_PropertyWithNoAccessors);
    }
    if ((propertyNode->flags & NF_MOD_EXTERN) && (propertyNode->flags & NF_MOD_ABSTRACT))
        errorSymbol(property, ERR_AbstractAndExtern);

    // If this is an event property, define the corresponding event symbol.
    if (isEvent) {
        EVENTSYM * event = defineEvent(property, propertyNode);
        event->implementation = NULL; // remove link to "fake" property symbol.
    }
}

void CLSDREC::definePropertyAccessors(PROPSYM *property)
{
    PROPERTYNODE *propertyNode = property->parseTree->asANYPROPERTY();
    bool isEvent = property->isEvent;
    AGGSYM *cls = property->getClass();

    //
    // set allowable flags for accessors
    // can't have virtual (or override) accessors in a struct, or for any event.
    // can't have any modifiers in an interface
    // can't have any modifiers on an explicit interface implementation (except extern)
    //
    unsigned allowableFlags;
    unsigned propertyFlags = propertyNode->flags;
    if (property->name && cls->isClass && !isEvent) {
        allowableFlags = NF_MOD_ABSTRACT | NF_MOD_EXTERN | NF_MOD_VIRTUAL | NF_MOD_OVERRIDE | NF_MOD_SEALED | NF_MOD_UNSAFE; 
    } else if (!property->name || isEvent) { // explicit interface impl or event
        allowableFlags = NF_MOD_UNSAFE | NF_MOD_EXTERN;
    } else { // interface member
        allowableFlags = 0;
    }

    //
    // overriden properties use inherited names for accessors
    //
    PROPSYM *overridenMember = NULL;
    if (property->isOverride) {
        //
        // find a base member which implements all the properties we need
        //
        AGGSYM *classToSearch = cls->baseClass;
        SYM * overridenSym;
        while (classToSearch &&
               (overridenSym = findHiddenSymbol(property->name, SK_PROPSYM, property->params, classToSearch, cls)) &&
               (overridenSym->kind == SK_PROPSYM)) {

            overridenMember = overridenSym->asPROPSYM();

            if (overridenMember->isBogus || (overridenMember->retType != property->retType) ||
                (overridenMember->access != property->access))
            {
                overridenMember = NULL;
                break;
            }

            if ((!propertyNode->pGet || (overridenMember->methGet && overridenMember->methGet->isVirtual)) && 
                (!propertyNode->pSet || (overridenMember->methSet && overridenMember->methSet->isVirtual))) {

                //
                // found a base member which implements all the accessors we need
                //
                if (property->isIndexer()) {
                    property->asINDEXERSYM()->realName = overridenMember->asINDEXERSYM()->realName;
                }
                break;
            }

            classToSearch = overridenMember->getClass()->baseClass;
            overridenMember = NULL;
        }
    } else {
        //
        // check for new protected in a sealed class
        //
        if ((property->access == ACC_PROTECTED || property->access == ACC_INTERNALPROTECTED) &&
            cls->isSealed) {
            errorSymbol(property, cls->isStruct ? ERR_ProtectedInStruct : WRN_ProtectedInSealed);
        }
    }

    //
    // create get accessor
    //
    if (propertyNode->pGet) {
        PNAME accessorName;
        if (overridenMember && overridenMember->methGet) {
            accessorName = overridenMember->methGet->name;
        } else {
            accessorName = createAccessorName(property->getRealName(), L"get_");
        }
        METHSYM *getAccessor = definePropertyAccessor(
                                    accessorName, 
                                    propertyNode->pGet, 
                                    property->cParams,
                                    property->params,
                                    property->retType,
                                    allowableFlags,
                                    propertyFlags,
                                    &property->methGet,
                                    cls);
        if (getAccessor) {
            if (isEvent) {
                getAccessor->access = ACC_PRIVATE;
            } else {
                getAccessor->access = property->access;
            }
            if (getAccessor->isVirtual && getAccessor->access == ACC_PRIVATE) {
                errorSymbol(getAccessor, ERR_VirtualPrivate);
            }
            getAccessor->isParamArray = property->isParamArray;
            getAccessor->hasCLSattribute = property->hasCLSattribute;
            getAccessor->isCLS = property->isCLS;
        }
    }

    //
    // create set accessor
    //
    if (propertyNode->pSet) {
        PNAME accessorName;
        if (overridenMember && overridenMember->methSet) {
            accessorName = overridenMember->methSet->name;
        } else {
            accessorName = createAccessorName(property->getRealName(), L"set_");
        }

        // build the signature for the set accessor
        PTYPESYM initTypeArray[8];                      // the initial location of the param types
        PTYPESYM *paramTypes = &(initTypeArray[0]);     // the non-unique param types
        if ((property->cParams + 1) > 8) {
            paramTypes = (PTYPESYM*) _alloca((property->cParams + 1) * sizeof(PTYPESYM));
        }
        memcpy(paramTypes, property->params, property->cParams * sizeof(PTYPESYM));
        paramTypes[property->cParams] = property->retType;

        METHSYM *setAccessor = definePropertyAccessor(
                                    accessorName,
                                    propertyNode->pSet,
                                    property->cParams + 1,
                                    compiler()->symmgr.AllocParams(property->cParams + 1, paramTypes),
                                    compiler()->symmgr.GetVoid(),
                                    allowableFlags,
                                    propertyFlags,
                                    &property->methSet,
                                    cls);
        if (setAccessor) {
            if (isEvent) {
                setAccessor->access = ACC_PRIVATE;
            } else {
                setAccessor->access = property->access;
            }
            if (setAccessor->isVirtual && setAccessor->access == ACC_PRIVATE) {
                errorSymbol(setAccessor, ERR_VirtualPrivate);
            }
            setAccessor->isParamArray = property->isParamArray;
            setAccessor->hasCLSattribute = property->hasCLSattribute;
            setAccessor->isCLS = property->isCLS;
        }
    }
}

// creates an internal name for user defined property accessor methods
// just prepends the name with "Get" or "Set"
PNAME CLSDREC::createAccessorName(PNAME propertyName, LPCWSTR prefix)
{
    if (propertyName) {
        size_t prefixLen = wcslen(prefix);

        WCHAR *nameBuffer = (WCHAR*) _alloca((1 + prefixLen + wcslen(propertyName->text)) * sizeof(WCHAR));
        wcscpy(nameBuffer, prefix);
        wcscpy(nameBuffer + prefixLen, propertyName->text);
        return compiler()->namemgr->AddString(nameBuffer);
    } else {
        // explicit interface implementation accessors have no name
        // just like the properties they are contained on.
        return NULL;
    }
}

// creates a METHSYM for a property accessor
METHSYM *CLSDREC::definePropertyAccessor(PNAME name, BASENODE *parseTree, unsigned cParam, PTYPESYM *params, PTYPESYM retType, unsigned allowableFlags, unsigned propertyFlags, METHSYM **accessor, AGGSYM *cls)
{
    ASSERT(!*accessor);

    //
    // check for duplicate member name, or name same as class
    //
    if (!checkForBadMember(name, SK_METHSYM, params, parseTree, cls)) {
        return NULL;
    }

    //
    // munge the parse tree flags from the properties flags
    //
    parseTree->flags |= ((NF_MOD_OVERRIDE | NF_MOD_ABSTRACT | NF_MOD_VIRTUAL | NF_MOD_SEALED | NF_MOD_EXTERN) & propertyFlags);

    //
    // create accessor
    //
    METHSYM *acc = compiler()->symmgr.CreateMethod(name, cls);
    acc->parseTree = parseTree;
    acc->isStatic = (propertyFlags & NF_MOD_STATIC) != 0;
    if (cls->isInterface || propertyFlags & NF_MOD_ABSTRACT) {
        acc->isAbstract = true;
    }
    if (propertyFlags & NF_MOD_OVERRIDE) {
        acc->isOverride = true;
    }
    if (propertyFlags & NF_MOD_EXTERN) {
        acc->isExternal = true;
    }
    if (acc->isAbstract ||
        acc->isOverride ||
        (propertyFlags & NF_MOD_VIRTUAL)) {

        acc->isVirtual = true;
        acc->isMetadataVirtual = true;
    }
    if ((propertyFlags & NF_MOD_SEALED)) {
        // Already checked for non-override at property level.
        acc->isVirtual = false; 
        ASSERT(acc->isMetadataVirtual);
    }
    acc->isPropertyAccessor = true;
    acc->retType = retType;
    acc->params = params;
    acc->cParams = cParam;

    //
    // this needs to happen here so that the correct errors are reported
    *accessor = acc;
    if (acc->isExternal && acc->isAbstract)
        errorSymbol(acc, ERR_AbstractAndExtern);

    return acc;
}

// define an event by entering it into the symbol table.
// the underlying implementation of the event, a field or property, has already been defined,
// and we do not duplicate checks performed there. We do check that the event
// is of a delegate type. The access modifier on the implementation is changed to 
// private and the event inherits the access modifier of the underlying field. The event
// accessor methods are defined.
EVENTSYM * CLSDREC::defineEvent(SYM * implementingSym, BASENODE * pTree)
{
    EVENTSYM * event;
    bool isField;
    bool isStatic;
    AGGSYM * cls;
    bool isPropertyEvent = (pTree->kind == NK_PROPERTY);
    unsigned flags = (pTree->kind == NK_VARDECL) ? pTree->pParent->flags 
                                                 : pTree->flags;


    // An event must be implemented by a property or field symbol.
    ASSERT(implementingSym->kind == SK_PROPSYM || implementingSym->kind == SK_MEMBVARSYM);
    isField = (implementingSym->kind == SK_MEMBVARSYM);
    isStatic = isField ? implementingSym->asMEMBVARSYM()->isStatic : implementingSym->asPROPSYM()->isStatic;

    // Create the event symbol.
    cls = implementingSym->parent->asAGGSYM();
    event = compiler()->symmgr.CreateEvent(implementingSym->name, cls);
    event->implementation = implementingSym;
    event->isStatic = isStatic;
    event->parseTree = pTree;
    if (isField) {
        event->type = implementingSym->asMEMBVARSYM()->type;
        implementingSym->asMEMBVARSYM()->isEvent = true;
    }
    else {
        event->type = implementingSym->asPROPSYM()->retType;
        implementingSym->asPROPSYM()->isEvent = true;
    }

    // The event and accessors get the access modifiers of the implementing symbol; the implementing symbol
    // becomes private.
    event->access = implementingSym->access;
    implementingSym->access = ACC_PRIVATE;


    // Create Accessor for Add.
    defineEventAccessor(createAccessorName(event->name, L"add_"),
                        isPropertyEvent ? pTree->asPROPERTY()->pSet : pTree,
                        event->type,
                        event->access,
                        flags,
                        & event->methAdd,
                        cls);

    // Create accessor for Remove.
    defineEventAccessor(createAccessorName(event->name, L"remove_"),
                        isPropertyEvent ? pTree->asPROPERTY()->pGet : pTree,
                        event->type,
                        event->access,
                        flags,
                        & event->methRemove,
                        cls);

    if (event->methAdd && event->methAdd->isVirtual && event->access == ACC_PRIVATE) {
        errorSymbol(event, ERR_VirtualPrivate);
    }

    return event;
}

// Define an "add" or "remove" accessor for an event. Returns the METHSYM of the accessor.
METHSYM * CLSDREC::defineEventAccessor(PNAME name, BASENODE * parseTree, PTYPESYM eventType, ACCESS access, unsigned flags, METHSYM **accessor, AGGSYM * cls)
{
    if (parseTree == NULL)
        return NULL;  // error must already have been reported.

    //
    // check for duplicate member name, or name same as class
    //
    if (!checkForBadMember(name, parseTree, cls)) {
        return NULL;
    }

    //
    // create accessor
    //
    METHSYM *acc = compiler()->symmgr.CreateMethod(name, cls);
    acc->parseTree = parseTree;
    acc->isStatic = (flags & NF_MOD_STATIC) != 0;
    acc->access = access;

    if (cls->isInterface || flags & NF_MOD_ABSTRACT) {
        acc->isAbstract = true;
    }
    if (flags & NF_MOD_OVERRIDE) {
        acc->isOverride = true;
    }
    if (flags & NF_MOD_EXTERN) {
        acc->isExternal = true;
    }
    if (acc->isAbstract ||
        acc->isOverride ||
        (flags & NF_MOD_VIRTUAL)) 
    {
        acc->isVirtual = true;
        acc->isMetadataVirtual = true;
    }
    if (flags & NF_MOD_SEALED) {
        // Already checked for non-override at event level.
        acc->isVirtual = false; 
        ASSERT(acc->isMetadataVirtual);
    }

    acc->isEventAccessor = true;

    // Accessors always have the signature: void add_XXX(EventType e) or void remove_XXX(EventType e);
    acc->retType = compiler()->symmgr.GetVoid();
    acc->params = compiler()->symmgr.AllocParams(1, &eventType);
    acc->cParams = 1;

    // save accessor away.
    *accessor = acc;

    return acc;
}


// finds a duplicate user defined conversion operator
// or a regular member hidden by a new conversion operator
SYM *CLSDREC::findDuplicateConversion(bool conversionsOnly, PTYPESYM *parameterTypes, PTYPESYM returnType, PNAME name, AGGSYM* cls)
{
    SYM * present = compiler()->symmgr.LookupGlobalSym(name, cls, MASK_ALL);
    while (present) {
        if ((!conversionsOnly && (present->kind != SK_METHSYM)) || 
            ((present->kind == SK_METHSYM) && 
    		 (present->asMETHSYM()->params == parameterTypes) &&
    		 //
    		 // we have a method which matches name and parameters
    		 // we're looking for 2 cases:
    		 //

    		 //
    		 // 1) any conversion operator with matching return type
    		 //
             (((present->asMETHSYM()->retType == returnType) &&
    			present->asMETHSYM()->isConversionOperator()) ||
    		 //
    		 // 2) a non-conversion operator, and we have an exact name match
    		 //
               (!present->asMETHSYM()->isConversionOperator() && 
    			!conversionsOnly)))) {
            return present;
        }

        present = compiler()->symmgr.LookupNextSym(present, present->parent, MASK_ALL);
    }

    return NULL;
}

#define OP(n,p,a,stmt,t,pn,e) pn,
const PREDEFNAME CLSDREC::operatorNames[]  = {
    #include "ops.h"
    };

OPERATOR CLSDREC::operatorOfName(PNAME name)
{
    unsigned i;
    for (i = 0; i < OP_LAST && ((operatorNames[i] == PN_COUNT) || (name != compiler()->namemgr->GetPredefName(operatorNames[i]))); i += 1)
    { /* nothing */ }

    return (OPERATOR) i;
}

// define a user defined conversion operator, this involves checking that the
// flags for the decls have be specified correctly, and actually entering the
// conversion into the symbol table...
//
// returns whether an operator requiring a matching operator has been seen
bool CLSDREC::defineOperator(METHODNODE * operatorNode, AGGSYM * cls)
{

    bool mustMatch = false;

    ASSERT(!cls->isEnum);

    if (cls->isInterface) {
        errorFile(operatorNode, cls->getInputFile(), ERR_InterfacesCantContainOperators);
        return false;
    }

    //
    // get parameter types. Note that the parser guarantees that
    // we will have exactly one or two parameters and the right
    // number for the operator.
    //
    unsigned numberOfParameters = 0;
    PTYPESYM parameterTypes[2];
    NODELOOP(operatorNode->pParms, PARAMETER, parameterNode)
        ASSERT(numberOfParameters < 2);
        parameterTypes[numberOfParameters] = bindType(parameterNode->pType, cls, BTF_NODECLARE | BTF_NODEPRECATED, NULL);
        if (!parameterTypes[numberOfParameters]) {
            return false;
        }
        numberOfParameters += 1;
    ENDLOOP

    //
    // get return type
    //
    TYPESYM *returnType = bindType(operatorNode->pType, cls, BTF_NODECLARE | BTF_NODEPRECATED, NULL);
    if (!returnType) {
        return mustMatch;
    }

    //
    // check argument restrictions. Note that the parser has
    // already checked the number of arguments is correct.
    //
    bool isConversion = false;
    bool isImplicit = false;
    switch (operatorNode->iOp) {

    // conversions
    case OP_IMPLICIT:
        isImplicit = true;
        // fallthrough

    case OP_EXPLICIT:
        //
        // check for the identity conversion here
        //
        if (returnType == cls && parameterTypes[0] == cls) {
            errorFile(operatorNode, cls->getInputFile(), ERR_IdentityConversion);
            return false;
        }
        //
        // either the source or the destination must be the containing type
        //
        else if (returnType != cls && parameterTypes[0] != cls) {
            errorFile(operatorNode, cls->getInputFile(), ERR_ConversionNotInvolvingContainedType);
            return false;
        }
        isConversion = true;
        break;

    // unary operators
    case OP_TRUE:
    case OP_FALSE:

        mustMatch = true;

        if (!returnType->isPredefType(PT_BOOL)) {
            errorFile(operatorNode, cls->getInputFile(), ERR_OpTFRetType);
            return false;
        }

        goto CHECKSOURCE;

    case OP_PREINC:
    case OP_PREDEC:
        //
        // the destination must be the containing type
        //
        if (parameterTypes[0] != cls || returnType != cls) {
            errorFile(operatorNode, cls->getInputFile(), ERR_BadIncDecSignature);
            return false;
        }
        // fallthrough

    case OP_UPLUS:
    case OP_NEG:
    case OP_BITNOT:
        
    case OP_LOGNOT:

CHECKSOURCE:
        //
        // the source must be the containing type
        //
        if (parameterTypes[0] != cls) {
            errorFile(operatorNode, cls->getInputFile(), ERR_BadUnaryOperatorSignature);
            return false;
        }
        break;

    // binary operators
    case OP_EQ:
    case OP_NEQ:
    case OP_GT:
    case OP_LT:
    case OP_GE:
    case OP_LE:
        mustMatch = true;

    case OP_ADD:
    case OP_SUB:
    case OP_MUL:
    case OP_DIV:
    case OP_MOD:
    case OP_BITXOR:
    case OP_BITAND:
    case OP_BITOR:

    // shift operators
    case OP_LSHIFT:
    case OP_RSHIFT:
        //
        // at least one of the parameter types must be the containing type
        //
        if (parameterTypes[0] != cls && parameterTypes[1] != cls) {
            errorFile(operatorNode, cls->getInputFile(), ERR_BadBinaryOperatorSignature);
            return false;
        }
        break;


    default:
        ASSERT(!"Unknown operator parse tree");
        break;
    }

    //
    // return type of user defined operators can't be VOID
    //
    if (returnType == compiler()->symmgr.GetVoid()) {
        errorFile(operatorNode, cls->getInputFile(), ERR_OperatorCantReturnVoid);
        return false;
    }

    //
    //
    // create the unique parameter array
    //
    TYPESYM ** params = compiler()->symmgr.AllocParams(numberOfParameters, parameterTypes);

    //
    // check for duplicate and get name.
    //
    PNAME name;
    if (isConversion) {
        //
        // For operators we check for conflicts with implicit vs. explicit as well
        // create both implicit and explicit names here because they can conflict
        // with each other.
        //
        PNAME implicitName = compiler()->namemgr->GetPredefName(PN_OPIMPLICITMN);
        PNAME explicitName = compiler()->namemgr->GetPredefName(PN_OPEXPLICITMN);
        PNAME otherName;
        if (isImplicit) {
            name = implicitName;
            otherName = explicitName;
        } else {
            name = explicitName;
            otherName = implicitName;
        }

        //
        // check for name same as that of parent aggregate
        //
        if (name == cls->name) {
            errorNameRefSymbol(name, operatorNode, cls, cls, ERR_MemberNameSameAsType);
            return false;
        }

        //
        // check for duplicate conversion and conflicting names
        //
        SYM *duplicateConversion;
        if ((duplicateConversion = findDuplicateConversion(false,   params, returnType, name,      cls)) ||
            (duplicateConversion = findDuplicateConversion(true,    params, returnType, otherName, cls))) {
            if ((duplicateConversion->kind == SK_METHSYM) && duplicateConversion->asMETHSYM()->isConversionOperator()) {
                errorStrSymbol(operatorNode, cls->getInputFile(), compiler()->ErrSym(cls), duplicateConversion, ERR_DuplicateConversionInClass);
            } else {
                errorNameRefStrSymbol(
                    name,
                    operatorNode, 
                    cls, 
                    compiler()->ErrSym(cls), 
                    duplicateConversion, 
                    ERR_DuplicateNameInClass);
            }
            return false;
        }
    } else {
        //
        // get the name for the operator
        //
        PREDEFNAME predefName = operatorNames[operatorNode->iOp];
        ASSERT(predefName > 0 && predefName < PN_COUNT);
        name = compiler()->namemgr->GetPredefName(predefName);

        if (!checkForBadMember(name, SK_METHSYM, params, operatorNode, cls)) {
            return false;
        }
    }

    //
    // create the operator
    //
    METHSYM *operatorSym = compiler()->symmgr.CreateMethod(name, cls);
    operatorSym->parseTree = operatorNode;
    operatorSym->retType = returnType;
    operatorSym->params = params;
    operatorSym->cParams = numberOfParameters;
    operatorSym->isStatic = true;
    operatorSym->isOperator = true;
    operatorSym->isExternal = !!(operatorNode->flags & NF_MOD_EXTERN);
    if (isConversion) {
        //
        // our containing type now has a conversion operator
        //
        cls->hasConversion = true;
        operatorSym->isImplicit = isImplicit;
        operatorSym->isExplicit = !isImplicit;
    }

    //
    // check flags and assign access. Conversions must be public.
    //
    checkFlags(operatorSym, NF_MOD_UNSAFE | NF_MOD_PUBLIC | NF_MOD_STATIC | NF_MOD_EXTERN, operatorNode->flags);

    //
    // operators must be explicitly declared static
    //
    if ((operatorNode->flags & (NF_MOD_PUBLIC | NF_MOD_STATIC)) != (NF_MOD_PUBLIC | NF_MOD_STATIC)) {
        errorSymbol(operatorSym, ERR_OperatorsMustBeStatic);
    }

    // Check constituent types
    checkConstituentVisibility(operatorSym, returnType, ERR_BadVisOpReturn);
    for (unsigned i = 0; i < numberOfParameters; ++i) 
        checkConstituentVisibility(operatorSym, params[i], ERR_BadVisOpParam);

    return mustMatch;
}

// bind a type name in a textual context (nsContext), and a symbolic context
// given by (current).  nsOK tells if its ok to bind to a namespace instead of a class.
// return the symbol, or null if not found, in which case error was already reported.
SYM * CLSDREC::bindTypeName(BASENODE *name, PARENTSYM * symContext, int flags, AGGSYM *classBeingResolved)
{
    SYM * rval;

    if (name->kind == NK_NAME) {
        rval = bindSingleTypeName(name->asNAME(), symContext, flags, classBeingResolved);
    } else {
        rval = bindDottedTypeName(name->asDOT(), symContext, flags, classBeingResolved);
    }

    return rval;
}

// resolves a structure's layout ensuring that there are no
// cycles in its layout
void CLSDREC::resolveStructLayout(AGGSYM *cls)
{
    ASSERT(!cls->isLayoutResolved && !cls->isResolvingLayout);

    //
    // predefined types don't obey the rules
    //
    if (cls->isPredefined) {
        cls->isLayoutResolved = true;
        return;
    }

    cls->isResolvingLayout = true;

    //
    // resolve the layout for all of our non-static fields
    FOREACHCHILD(cls, elem)
        if (elem->kind == SK_MEMBVARSYM && !elem->asMEMBVARSYM()->isStatic &&
            elem->asMEMBVARSYM()->type->kind == SK_AGGSYM) {
            AGGSYM *fieldType = elem->asMEMBVARSYM()->type->asAGGSYM();
            if (fieldType->isStruct) {
                if (fieldType->isResolvingLayout) {
                    // found a cycle
                    errorSymbolAndRelatedSymbol(elem, fieldType, ERR_StructLayoutCycle);
                    goto Error;
                } else if (!fieldType->isLayoutResolved) {
                    resolveStructLayout(fieldType);
                }
            }
        }
    ENDFOREACHCHILD

Error:
    cls->isResolvingLayout = false;
    cls->isLayoutResolved = true;
}

void CLSDREC::prepareInterfaceMember(METHPROPSYM * member)
{
    reportDeprecatedMethProp(member->parseTree, member);

    //
    // check that new members don't hide inherited members
    // we've built up a list of all inherited interfaces
    // search all of them for a member we will hide
    //

    // for all inherited interfaces
    checkIfaceHiding(member, member->getParseTree()->flags, member->kind, member->params);

    //
    // shouldn't have a body on interface members
    //
    if (member->kind == SK_METHSYM) {
        if ((member->asMETHSYM()->getParseTree()->other & NFEX_METHOD_NOBODY) == 0) {
            errorSymbol(member, ERR_InterfaceMemberHasBody);
        }
    } else {
        ASSERT(member->kind == SK_PROPSYM);
        PROPSYM *property = member->asPROPSYM();

        definePropertyAccessors(property);

        //
        // didn't find a conflict between this property and anything else
        // check that the accessors don't have a conflict with other methods
        //
        if (property->methGet) {
            prepareInterfaceMember(property->methGet);
        }
        if (property->methSet) {
            prepareInterfaceMember(property->methSet);
        }

        if (property->isDeprecated) {
            if (property->methGet) {
                property->methGet->isDeprecated         = true;
                property->methGet->isDeprecatedError    = property->isDeprecatedError;
                property->methGet->deprecatedMessage    = property->deprecatedMessage;
            }
            if (property->methSet) {
                property->methSet->isDeprecated         = true;
                property->methSet->isDeprecatedError    = property->isDeprecatedError;
                property->methSet->deprecatedMessage    = property->deprecatedMessage;
            }
        }
    }
}

//
// builds the recursive closure of an interface list
//
void CLSDREC::addUniqueInterfaces(SYMLIST *list, SYMLIST *** addToList, SYMLIST *source)
{
    FOREACHSYMLIST(source, sym)
        addUniqueInterfaces(list, addToList, sym->asAGGSYM());
    ENDFOREACHSYMLIST
}

//
// builds the recursive closure of an interface list
//
void CLSDREC::addUniqueInterfaces(SYMLIST *list, SYMLIST ***addToList, AGGSYM *sym)
{
    if (!list->contains(sym)) {
        compiler()->symmgr.AddToGlobalSymList(sym, addToList);
    }
}

//
// combines a list of interfaces, and checks for conflicting
// members between interfaces in the list, and all their inherited
// interfaces.
//
// It is important that we form the lists in a given order.  Basically, an interface may not be
// preceeded by any interfaces that it descends from.  So, we always add to the front of the list.
void CLSDREC::combineInterfaceLists(AGGSYM *aggregate)
{
    SYMLIST **addToAllIfaceList = &aggregate->allIfaceList;
    SYMLIST **addToAllIfaceListFront = addToAllIfaceList;

    // Because we add to the front of the list, this reverses the list of interfaces.
    // Because we want to the metadata to have the interface list in the order specified in
    // source, we reverse the list first.
    SYMLIST * reversedIfaceList = NULL;
    SYMLIST ** preversedIfaceList = &reversedIfaceList;
    SYMLIST ** preversedIfaceListFront = preversedIfaceList;
    FOREACHSYMLIST(aggregate->ifaceList, next)
        preversedIfaceList = preversedIfaceListFront;
        compiler()->symmgr.AddToGlobalSymList(next, &preversedIfaceList);
    ENDFOREACHSYMLIST

    FOREACHSYMLIST(reversedIfaceList, next)
        if (!aggregate->allIfaceList->contains(next)) {
            AGGSYM *ifaceToAdd = next->asAGGSYM();

            //
            // ensure inherited interface has already merged its interfaces
            //
            ASSERT(ifaceToAdd->hasResolvedBaseClasses);

            //
            // add the inherited interface and its inherited interfaces
            // to our complete list of inheritedInterfaces
            //
            addToAllIfaceList = addToAllIfaceListFront;

            addUniqueInterfaces(aggregate->allIfaceList, &addToAllIfaceList, ifaceToAdd);
            addUniqueInterfaces(aggregate->allIfaceList, &addToAllIfaceList, ifaceToAdd->allIfaceList);
        }
    ENDFOREACHSYMLIST
}

//
// does the prepare stage for an interface.
// This checks for conflicts between members in inherited interfaces
// and checks for conflicts between members in this interface
// and inherited members.
//
void CLSDREC::prepareInterface(AGGSYM *cls)
{
    ASSERT(cls->isDefined);

    //
    // this must be done before preparing our members
    // because they may derive from us, which requires that we
    // are prepared.
    //
    cls->isPrepared = true;

    if (cls->getInputFile()->isSource) {
        //
        // prepare members
        //
        FOREACHCHILD(cls, child)

        SETLOCATIONSYM(child);

            switch (child->kind) {
            case SK_METHSYM:
                if (child->asMETHSYM()->isPropertyAccessor || child->asMETHSYM()->isEventAccessor) {
                    //
                    // accessors are handled by their property/event
                    //
                    break;
                }
                // fallthrough

            case SK_PROPSYM:
                prepareInterfaceMember(child->asMETHPROPSYM());
                break;

            case SK_PTRSYM:
            case SK_ARRAYSYM:
            case SK_EXPANDEDPARAMSSYM:
            case SK_PARAMMODSYM:
                break;  // type based on this type.

            case SK_EVENTSYM:
                checkIfaceHiding(child, child->asEVENTSYM()->parseTree->pParent->flags, SK_COUNT, NULL);
                compiler()->symmgr.DeclareType(child->asEVENTSYM()->type);
                reportDeprecatedType(child->asEVENTSYM()->parseTree, child->asEVENTSYM(), child->asEVENTSYM()->type);
                // Issue error if the event type is not a delegate.
                if (child->asEVENTSYM()->type->kind != SK_AGGSYM || ! child->asEVENTSYM()->type->asAGGSYM()->isDelegate) {
                    errorSymbol(child->asEVENTSYM(), ERR_EventNotDelegate);
                }
                break;

            default:
                ASSERT(!"Unknown node type");
            }
        ENDFOREACHCHILD
    }
}

//
// checks the simple hiding issues for non-override members
//
void CLSDREC::checkSimpleHiding(SYM *sym, unsigned flags)
{
    checkSimpleHiding(sym, flags, SK_COUNT, NULL);
}

//
// reports the results of a symbol hiding another symbol
//
void CLSDREC::reportHiding(SYM *sym, SYM *hiddenSymbol, unsigned flags)
{
    // Pretend like Destructors are not called "Finalize"
    if (hiddenSymbol && hiddenSymbol->kind == SK_METHSYM && hiddenSymbol->asMETHSYM()->isDtor) {
        hiddenSymbol = NULL;
    }

    //
    // check the shadowing flags are correct
    //
    if (!hiddenSymbol) {
        if (flags & NF_MOD_NEW) {
            errorSymbol(sym, WRN_NewNotRequired);
        }
    } else {
        if (!(flags & NF_MOD_NEW)) {
            errorSymbolAndRelatedSymbol(
                sym,
                hiddenSymbol,
                (hiddenSymbol->isVirtualSym() &&
                 (sym->kind == hiddenSymbol->kind) &&
                 !sym->parent->asAGGSYM()->isInterface) ?
                    WRN_NewOrOverrideExpected :
                    WRN_NewRequired);
        }
        if (!sym->parent->asAGGSYM()->isInterface) {
            checkHiddenSymbol(sym, hiddenSymbol);
        }
    }
}

//
// checks the simple hiding issues for non-override members
//
void CLSDREC::checkSimpleHiding(SYM *sym, unsigned flags, SYMKIND symkind, PTYPESYM *params)
{
    //
    // find an accessible member with the same name in
    // a base class of our enclosing class
    //
    AGGSYM *enclosingClass = sym->parent->asAGGSYM();
    SYM *hiddenSymbol = findHiddenSymbol(sym->name, symkind, params, enclosingClass->baseClass, enclosingClass);

    //
    // report the results
    //
    reportHiding(sym, hiddenSymbol, flags);
}

void CLSDREC::checkIfaceHiding(SYM *sym, unsigned flags, SYMKIND symkind, PTYPESYM *params)
{
    // for all inherited interfaces
    AGGSYM *cls = sym->parent->asAGGSYM();
    if (sym->isUserCallable()) {
        BOOL foundHiddenSymbol = false;
        FOREACHSYMLIST(cls->allIfaceList, interfaceToSearch)

            SYM *hiddenSymbol = findHiddenSymbol(sym->name, symkind, params, interfaceToSearch->asAGGSYM(), cls);
            if (hiddenSymbol) {
                // found a method or property we will hide
                reportHiding(sym, hiddenSymbol, flags);
                foundHiddenSymbol = true;
            }
        ENDFOREACHSYMLIST

        // check for gratuitous new
        if (!foundHiddenSymbol) {
            reportHiding(sym, NULL, flags);
        }
    }
}

// Removes ref or out from all parameters in a signature.
PTYPESYM * CLSDREC::RemoveRefOut(int cTypes, PTYPESYM * types)
{
    PTYPESYM * newTypes = (PTYPESYM *) _alloca(cTypes * sizeof(PTYPESYM));
    
    for (int i = 0; i < cTypes; ++i) {
        PTYPESYM type = types[i];
        if (type->kind == SK_PARAMMODSYM) 
            newTypes[i] = type->asPARAMMODSYM()->paramType();
        else
            newTypes[i] = type;
    }

    return compiler()->symmgr.AllocParams(cTypes, newTypes);
}

// checks for case-insensitive collisions in
// public members of any AGGSYM. Also checks that we don't overload solely by
// ref or out.
void CLSDREC::checkCLSnaming(AGGSYM *cls)
{
    ASSERT(cls && cls->hasExternalAccess());
    WCHAR buffer[MAX_IDENT_SIZE];
    PERRORSYM CLSroot;
    PLOCVARSYM temp;
    PAGGSYM base;

    // Create a local symbol table root for this stuff
    CLSroot = (PERRORSYM)compiler()->symmgr.CreateLocalSym( SK_ERRORSYM,
        compiler()->namemgr->AddString( L"$clsnamecheck$"), NULL);

    // notice that for all these 'fake' members we're adding the sym->type 
    // is not the actual type of the 'fake' member, but a pointer back to
    // the orignal member
    
    // add all externally visible members of interfaces
    FOREACHSYMLIST(cls->allIfaceList, base)
        ASSERT(base->asAGGSYM());
       
        FOREACHCHILD(base->asAGGSYM(), member)
            if (member->hasExternalAccess()) {
                // Add a lower-case symbol to our list (we don't care about collisions here)
                ToLowerCase( member->name->text, buffer, (size_t) -1);
                PLOCVARSYM temp = (PLOCVARSYM)compiler()->symmgr.CreateLocalSym( SK_LOCVARSYM,
                    compiler()->namemgr->AddString( buffer), CLSroot);
                temp->type = (PTYPESYM)member;
            }
        ENDFOREACHCHILD
    ENDFOREACHSYMLIST
    // add all the externally visible members of base classes
    for (base = cls->baseClass; base; base = base->baseClass) {
        ASSERT(base->asAGGSYM());
        if (!base->asAGGSYM()->isCLS_Type())
            errorSymbolAndRelatedSymbol( cls, base, ERR_CLS_BadBase);
       
        FOREACHCHILD(base->asAGGSYM(), member)
            if (member->hasExternalAccess()) {
                // Add a lower-case symbol to our list (we don't care about collisions here)
                ToLowerCase( member->name->text, buffer, (size_t) -1);
                PLOCVARSYM temp = (PLOCVARSYM)compiler()->symmgr.CreateLocalSym( SK_LOCVARSYM,
                    compiler()->namemgr->AddString( buffer), CLSroot);
                temp->type = (PTYPESYM)member;
            }
        ENDFOREACHCHILD
    }

    // Also check the underlying type of Enums
    if (cls->isEnum && !cls->underlyingType->isCLS_Type()) {
        errorSymbolAndRelatedSymbol( cls, cls->underlyingType, ERR_CLS_BadBase);
    }


    FOREACHCHILD(cls, member)
        if (member->kind == SK_ARRAYSYM || 
            member->kind == SK_PTRSYM || 
            member->kind == SK_PARAMMODSYM || 
            member->kind == SK_PINNEDSYM ||
            member->kind == SK_EXPANDEDPARAMSSYM ||
            member->kind == SK_FAKEMETHSYM ||
            member->kind == SK_FAKEPROPSYM)
            continue;

        if (!member->checkForCLS()) {
            if (cls->isInterface) {
                // CLS Compliant Interfaces can't have non-compliant members
                errorSymbol(member, ERR_CLS_BadInterfaceMember);
            } else if (member->mask() & (MASK_METHSYM | MASK_EXPANDEDPARAMSSYM) && member->asMETHSYM()->isAbstract) {
                // CLS Compliant types can't have non-compliant abstract members
                errorSymbol(member, ERR_CLS_NoAbstractMembers);
            }
        } else if (member->hasExternalAccess()) {

            ToLowerCase( member->name->text, buffer, (size_t) -1);
            if (buffer[0] == (WCHAR)0x005F || buffer[0] == (WCHAR)0xFF3F)  // According to CLS Spec these are '_'
                errorSymbol( member, ERR_CLS_BadIdentifier);

            PNAME nameLower = compiler()->namemgr->AddString(buffer);

            // Check for colliding names
            temp = (PLOCVARSYM)compiler()->symmgr.LookupLocalSym(nameLower, CLSroot, MASK_LOCVARSYM);
            if (temp) {
                SYM * sym = temp->type;  // Wacky but true!

                // If names are different, then they must differ only in case.
                if (member->name != temp->type->name) 
                    errorSymbolAndRelatedSymbol( member, temp->type, ERR_CLS_BadIdentifierCase);

                if (member->kind == SK_METHSYM) {
                    PTYPESYM * reducedSig = RemoveRefOut(member->asMETHSYM()->cParams, member->asMETHSYM()->params);
                    // Search all symbols with this name for signature collisions in ref/out only.
                    while (temp) {
                        sym = temp->type;
                        if (sym->kind == SK_METHSYM && reducedSig == RemoveRefOut(sym->asMETHSYM()->cParams, sym->asMETHSYM()->params) &&
                            member->asMETHSYM()->params != sym->asMETHSYM()->params)
                        {
                            errorSymbolAndRelatedSymbol( member, sym, ERR_CLS_OverloadRefOut);
                        }

                    
                        temp = (PLOCVARSYM)compiler()->symmgr.LookupNextSym(temp, CLSroot, MASK_LOCVARSYM);
                    }
                }
            } 

            // Add to the table.
            temp = (PLOCVARSYM)compiler()->symmgr.CreateLocalSym( SK_LOCVARSYM, nameLower, CLSroot);
            temp->type = (PTYPESYM)member;
        }
    ENDFOREACHCHILD

    //Cleanup
    compiler()->DiscardLocalState();
}

// checks for case-insensitive collisions in
// public members of any NSSYM
void CLSDREC::checkCLSnaming(NSSYM *ns)
{
    ASSERT(ns && ns->hasExternalAccess());
    WCHAR buffer[MAX_IDENT_SIZE];
    PERRORSYM CLSroot;
    PLOCVARSYM temp;

    // Create a local symbol table root for this stuff
    CLSroot = (PERRORSYM)compiler()->symmgr.CreateLocalSym( SK_ERRORSYM,
        compiler()->namemgr->AddString( L"$clsnamecheck$"), NULL);

    // notice that for all these 'fake' members we're adding the sym->type 
    // is not the actual type of the 'fake' member, but a pointer back to
    // the orignal member
    
    NSDECLSYM *decl = ns->firstDeclaration();
    while (decl) {
        FOREACHCHILD(decl, member)
            if (member->mask() & (MASK_GLOBALATTRSYM | MASK_PREDEFATTRSYM | MASK_ALIASSYM))
                continue;
            if (member->kind != SK_NSDECLSYM) compiler()->symmgr.DeclareType(member);
            if (member->hasExternalAccess() && member->checkForCLS()) {
                if (member->kind == SK_AGGSYM) {

                    ToLowerCase( member->name->text, buffer, (size_t) -1);
                    if (buffer[0] == (WCHAR)0x005F || buffer[0] == (WCHAR)0xFF3F)  // According to CLS Spec these are '_'
                        errorSymbol( member, ERR_CLS_BadIdentifier);

                    // Check for colliding names
                    temp = (PLOCVARSYM)compiler()->symmgr.LookupLocalSym(
                        compiler()->namemgr->AddString( buffer), CLSroot, MASK_LOCVARSYM);
                    if (temp && member->name != temp->type->name) {
                        if (member->getInputFile()->isSource || (temp->type->kind == SK_AGGSYM && temp->type->asAGGSYM()->parseTree) ||
                            (temp->type->kind == SK_NSSYM && temp->type->asNSSYM()->isDefinedInSource)) {
                            errorSymbolAndRelatedSymbol( member, temp->type, ERR_CLS_BadIdentifierCase);
                        }
                    } else {
                        temp = (PLOCVARSYM)compiler()->symmgr.CreateLocalSym( SK_LOCVARSYM,
                            compiler()->namemgr->AddString( buffer), CLSroot);
                        temp->type = (PTYPESYM)member;
                    }
                } else {
                    ASSERT (member->kind == SK_NSDECLSYM);
                    NSSYM * nested = member->asNSDECLSYM()->getScope();

                    // Only check namespaces that we haven't already visited
                    if (!nested->checkingForCLS) {
                        nested->checkingForCLS = true;

                        ToLowerCase( nested->name->text, buffer, (size_t) -1);
                        if (buffer[0] == (WCHAR)0x005F || buffer[0] == (WCHAR)0xFF3F)  // According to CLS Spec these are '_'
                            errorSymbol( member, ERR_CLS_BadIdentifier);

                        // Check for colliding names
                        temp = (PLOCVARSYM)compiler()->symmgr.LookupLocalSym(
                            compiler()->namemgr->AddString( buffer), CLSroot, MASK_LOCVARSYM);
                        if (temp && nested->name != temp->type->name) {
                            if ((temp->type->kind == SK_NSSYM && temp->type->asNSSYM()->isDefinedInSource) ||
                                nested->isDefinedInSource) {
                                errorNamespaceAndRelatedNamespace( (PNSSYM)temp->type, nested, ERR_CLS_BadIdentifierCase);
                            } else if (temp->type->kind == SK_AGGSYM && temp->type->asAGGSYM()->parseTree) {
                                errorSymbolAndRelatedSymbol( temp->type, nested, ERR_CLS_BadIdentifierCase);
                            }
                        } else {
                            temp = (PLOCVARSYM)compiler()->symmgr.CreateLocalSym( SK_LOCVARSYM,
                                compiler()->namemgr->AddString( buffer), CLSroot);
                            temp->type = (PTYPESYM)nested;
                        }
                    }
                }
            }
        ENDFOREACHCHILD
        decl = decl->nextDeclaration;
    }
    ns->checkedForCLS = true;

    //Cleanup
    compiler()->DiscardLocalState();
}

// prepares a class for compilation by preparing all of its elements...
// This should also verify that a class actually implements its interfaces...
void CLSDREC::prepareAggregate(AGGSYM * cls)
{
    ASSERT(!cls->getInputFile()->isSource || ((cls->parseTree  && cls->getInputFile()->hasChanged) || (!cls->parseTree  && !cls->getInputFile()->hasChanged)));
    if (!cls->isPrepared) {

        ASSERT(!cls->isPreparing);
        cls->isPreparing = true;

        // ASSERT(!compiler()->IsIncrementalRebuild() || compiler()->HaveTypesBeenDefined());
        compiler()->EnsureTypeIsDefined(cls);
        ASSERT(!(cls->isEnum && (cls->isStruct || cls->isClass || cls->isDelegate)));

        SETLOCATIONSYM(cls);

        //
        // do base class & interfaces first
        //
        if (cls->baseClass) {
            prepareAggregate(cls->baseClass);
            if (cls->parseTree) {
                reportDeprecatedType(cls->parseTree, cls, cls->baseClass);
            }
        }
        FOREACHSYMLIST(cls->ifaceList, iface)
            prepareAggregate(iface->asAGGSYM());
            if (cls->parseTree) {
                reportDeprecatedType(cls->parseTree, cls, iface->asAGGSYM());
            }
        ENDFOREACHSYMLIST;

        if (cls->isClass) {
            AGGSYM * base = cls->baseClass;
            while (!cls->hasConversion && base) {
                cls->hasConversion |= base->hasConversion;
                base = base->baseClass;
            }
        }

        if (cls->parseTree) {
            //
            // are we a nested class
            //
            if (cls->isNested()) {

                //
                // find an accessible member with the same name in
                // a base class of our enclosing class
                // check the shadowing flags are correct
                //
                checkSimpleHiding(cls, cls->parseTree->flags);
            }

            if (cls->isInterface) {
                prepareInterface(cls);
            } else if (cls->isEnum) {
                prepareEnum(cls);
            } else if (cls->isDelegate) {
                METHSYM *invokeMethod = compiler()->symmgr.LookupGlobalSym(compiler()->namemgr->GetPredefName(PN_INVOKE),
                                                                           cls, MASK_METHSYM)->asMETHSYM();
                reportDeprecatedMethProp(cls->parseTree, invokeMethod);
                // nothing to prepare for delegates
                cls->isPrepared = true;
            } else {
                prepareClassOrStruct(cls);
            }

            ASSERT(cls->isPrepared);

        } else {

            // For imported files we need to set the 'isOverride' bits
            // AFTER defining the base class, due to incremental scenarios
            setOverrideBits(cls);

            cls->isPrepared = true;

            buildAbstractMethodsList(cls);
        }
        cls->isPreparing = false;

        if (cls->getInputFile()->isSource && ((cls->isClass || cls->isStruct) && cls->getInputFile()->getOutputFile()->entryClassName == NULL)) {
            findEntryPoint(cls);
        }

    }

    ASSERT(!(cls->isEnum && (cls->isStruct || cls->isClass || cls->isDelegate)));
}

// finds an explicit interface implementation on a class or struct
// cls          is the type to look in
// explicitImpl is the interface member to look for
SYM *CLSDREC::findExplicitInterfaceImplementation(AGGSYM *cls, SYM *explicitImpl)
{
    if (explicitImpl->kind == SK_EVENTSYM) {
        EVENTSYM *foundImplementation = compiler()->symmgr.LookupGlobalSym(NULL, cls, MASK_EVENTSYM)->asEVENTSYM();
        while (foundImplementation && foundImplementation->explicitImpl != explicitImpl) {
            foundImplementation = compiler()->symmgr.LookupNextSym(foundImplementation, foundImplementation->parent, MASK_EVENTSYM)->asEVENTSYM();
        }

        return foundImplementation;
    }
    else {
        METHPROPSYM *foundImplementation = compiler()->symmgr.LookupGlobalSym(NULL, cls, MASK_METHSYM | MASK_PROPSYM)->asMETHPROPSYM();
        while (foundImplementation && foundImplementation->explicitImpl != explicitImpl) {
            foundImplementation = compiler()->symmgr.LookupNextSym(foundImplementation, foundImplementation->parent, MASK_METHSYM | MASK_PROPSYM)->asMETHPROPSYM();
        }

        return foundImplementation;
    }
}


//
// for each method in a class, set the override bit correctly
//
void CLSDREC::setOverrideBits(AGGSYM * cls)
{
    if (!cls->baseClass)
        return;

    ASSERT( cls->baseClass->isPrepared);

    FOREACHCHILD(cls, sym)
        if (sym->kind == SK_METHSYM && sym->asMETHSYM()->isOverride) {

            METHSYM * method = sym->asMETHSYM();
            method->isOverride = false;
            //
            // this method could be an override
            // NOTE: that we must set the isOverride flag accurately
            // because it affects:
            //      - lookup rules(override methods are ignored)
            //      - list of inherited abstract methods
            //
            SYM *hiddenSymbol;
            hiddenSymbol = findHiddenSymbol(method->name, SK_METHSYM, method->params, cls->baseClass, cls);
            if (hiddenSymbol && hiddenSymbol->kind == SK_METHSYM) {
                METHSYM *hiddenMethod = hiddenSymbol->asMETHSYM();
                if (hiddenMethod->retType == method->retType && hiddenMethod->access == method->access &&
                    hiddenMethod->isMetadataVirtual) {
                    method->isOverride = true;
                }
				
                if (hiddenMethod->isDtor)
                    method->isDtor = true;
            }
        }
    ENDFOREACHCHILD

    // Now set the override bits on the property itself
    FOREACHCHILD(cls, sym)
        if (sym->kind == SK_PROPSYM && !sym->isBogus) {

            PROPSYM * prop = sym->asPROPSYM();
            if (prop->methGet && prop->methSet) {
                if (prop->methGet->isOverride == prop->methSet->isOverride)
                    prop->isOverride = prop->methGet->isOverride;
                else {
                    prop->isOverride = false;
                    prop->isBogus = true;
                    prop->useMethInstead = true;
                }
            } else if (prop->methGet) {
                prop->isOverride = prop->methGet->isOverride;
            } else if (prop->methSet) {
                prop->isOverride = prop->methSet->isOverride;
            } else {
                ASSERT(!"A property without accessors?");
            }
        }
    ENDFOREACHCHILD

}

//
// for abstract classes, build list of abstract methods
//
void CLSDREC::buildAbstractMethodsList(AGGSYM * cls)
{
    ASSERT(!cls->abstractMethods);

    //
    // for abstract classes, build list of abstract methods
    //
    if (cls->isAbstract && !cls->isInterface) {

        ASSERT(cls->isClass);

        PSYMLIST *addToList = &cls->abstractMethods;

        // add all new abstract methods
        FOREACHCHILD(cls, child)
            if (child->kind == SK_METHSYM && child->asMETHSYM()->isAbstract) {
                compiler()->symmgr.AddToGlobalSymList(child, &addToList);
            }
        ENDFOREACHCHILD

        //
        // add inherited abstract methods that we don't locally implement
        // NOTE: this deals with property accessors as well
        //
        if (cls->baseClass) {
            FOREACHSYMLIST(cls->baseClass->abstractMethods, method)
                METHSYM *localMethod = findSameSignature(method->asMETHSYM(), cls);
                if (!localMethod || !localMethod->isOverride ||
                    //
                    // here we are checking for an inherited abstract method
                    // with internal access in another assembly
                    // or a private virtual method and we aren't an inner class
                    // 
                    !checkAccess(method, cls, NULL, cls)) {
                    compiler()->symmgr.AddToGlobalSymList(method, &addToList);
                }
            ENDFOREACHSYMLIST
        }
    }
}

// prepares a class for compilation by preparing all of its elements...
// This should also verify that a class actually implements its interfaces...
void CLSDREC::prepareClassOrStruct(AGGSYM * cls)
{
    ASSERT(cls->isDefined);
    ASSERT(!cls->isPrepared);

    //
    // this must be done before preparing our members
    // because they may derive from us, which requires that we
    // are prepared.
    //
    cls->isPrepared = true;

    ASSERT(cls->getInputFile()->isSource);

    //
    // check that our base class isn't sealed
    //
    if (cls->baseClass && cls->baseClass->isSealed) {
        errorSymbolAndRelatedSymbol(cls, cls->baseClass, ERR_CantDeriveFromSealedClass);
    }

    //
    // check that there isn't a cycle in the members
    // of this struct and other structs
    //
    if (cls->isStruct && !cls->isLayoutResolved) {
        resolveStructLayout(cls);
    }

    //
    // prepare members
    //
    FOREACHCHILD(cls, child)

        SETLOCATIONSYM(child);

        switch (child->kind) {
        case SK_MEMBVARSYM:
            prepareFields(child->asMEMBVARSYM());
            break;
        case SK_AGGSYM:
    		//
            // need to do this after fully preparing members
    		//
            break;
        case SK_METHSYM:
            prepareMethod(child->asMETHSYM());
            break;
        case SK_EXPANDEDPARAMSSYM:
        case SK_PARAMMODSYM:
        case SK_ARRAYSYM:
        case SK_PTRSYM:
            break;
        case SK_PROPSYM:
            prepareProperty(child->asPROPSYM());
            break;
        case SK_EVENTSYM:
            prepareEvent(child->asEVENTSYM());
            break;
        default:
            ASSERT(!"Unknown node type");
        }
    ENDFOREACHCHILD

    //
    // for abstract classes, build list of abstract methods
    // NOTE: must come after preparing members for abstract property accessors
    //       and before preparing nested types so they can properly check themselves
    //
    buildAbstractMethodsList(cls);

    //
    // prepare agggregate members
    //
    FOREACHCHILD(cls, child)

        SETLOCATIONSYM(child);

        switch (child->kind) {
        case SK_AGGSYM:
            prepareAggregate(child->asAGGSYM());
            break;
        default:
            break;
        }
    ENDFOREACHCHILD

    //
    // check that all inherited abstract methods are implemented for non-abstract classes
    //
    if (!cls->isAbstract && cls->baseClass) {
        FOREACHSYMLIST(cls->baseClass->abstractMethods, method)
            METHSYM *implementedMethod = findSameSignature(method->asMETHSYM(), cls);
            if (!implementedMethod || !implementedMethod->isOverride ||
                //
                // here we are checking for an inherited abstract method
                // with internal access in another assembly
                // or a private virtual method and we aren't an inner class
                // 
                !checkAccess(method, cls, NULL, cls)) {
                errorSymbolAndRelatedSymbol(cls, method, ERR_UnimplementedAbstractMethod);
            }
        ENDFOREACHSYMLIST
    }

    //
    // check that all interface methods are implemented
    //
    FOREACHSYMLIST(cls->allIfaceList, iface)
        FOREACHCHILD(iface->asAGGSYM(), member)

            switch (member->kind) {
            case SK_METHSYM:
            {
                if (member->asMETHSYM()->isPropertyAccessor || member->asMETHSYM()->isEventAccessor) {
                    //
                    // property accessor implementations are checked
                    // by their property declaration
                    // 
                    continue;
                }
                BOOL foundBaseImplementation = false;
                AGGSYM *classToSearchIn = cls;
                METHPROPSYM *foundImplementation;
                METHPROPSYM *closeImplementation = NULL;
                do {
                    //
                    // check for explicit interface implementation
                    //
                    foundImplementation = findExplicitInterfaceImplementation(classToSearchIn, member->asMETHSYM())->asMETHSYM();
                    if (foundImplementation) {
                        break;
                    }
                    
                    //
                    // check for imported class which implements this interface
                    //
                    if (!classToSearchIn->parseTree && classToSearchIn->allIfaceList->contains(iface)) {
                        foundBaseImplementation = true;
                        break;
                    }

                    //
                    // check for implicit interface implementation
                    //
                    foundImplementation = findSameSignature(member->asMETHSYM(), classToSearchIn);
                    if (foundImplementation) {
                        if (!foundImplementation->isStatic && 
                            (foundImplementation->access == ACC_PUBLIC) &&
                            (foundImplementation->retType == member->asMETHSYM()->retType)) {
                            // found a match

                            foundImplementation = needExplicitImpl(member->asMETHSYM(), foundImplementation->asMETHSYM(), cls);
                            break;
                        } else {
                            //
                            // found a close match, save it for error reporting
                            // and continue checking
                            //
                            if (!closeImplementation) {
                                closeImplementation = foundImplementation;
                            }
                            foundImplementation = NULL;
                        }
                    }
            
                    classToSearchIn = classToSearchIn->baseClass;
                } while (classToSearchIn);

                if (!foundImplementation && !foundBaseImplementation) {
                    //
                    // didn't find an implementation
                    //
                    if (!closeImplementation) {
                        errorSymbolAndRelatedSymbol(cls, member, ERR_UnimplementedInterfaceMember);
                    } else {
                        errorSymbolSymbolSymbol(cls, member, closeImplementation, ERR_CloseUnimplementedInterfaceMember);
                    }
                } else {
                    if (foundImplementation && foundImplementation->asMETHSYM()->getBaseConditionalSymbols(&compiler()->symmgr)) {
                        errorSymbolAndRelatedSymbol(foundImplementation, member, ERR_InterfaceImplementedByConditional);
                    }

                    // if the implementing method can't be set to isMetadataVirtual
                    // then we should have added a compiler generated explcit impl in needExplicitImpl
                    ASSERT((foundBaseImplementation && !foundImplementation) || foundImplementation->asMETHSYM()->isMetadataVirtual);
                }
                break;
            }
            case SK_PROPSYM:
            {
                if (member->asPROPSYM()->isBogus) {
                    //
                    // don't need implementation of bogus properties
                    // just need implementation of accessors as regular methods
                    //
                    break;
                }
                BOOL foundBaseImplementation = false;
                AGGSYM *classToSearchIn = cls;
                PROPSYM *foundImplementation;
                PROPSYM *closeImplementation = NULL;
                do {
                    //
                    // check for explicit interface implementation
                    //
                    foundImplementation = findExplicitInterfaceImplementation(classToSearchIn, member->asPROPSYM())->asPROPSYM();
                    if (foundImplementation) {
                        break;
                    }

                    //
                    // check for imported class which implements this interface
                    //
                    if (!classToSearchIn->parseTree && classToSearchIn->allIfaceList->contains(iface)) {
                        foundBaseImplementation = true;
                        break;
                    }

                    //
                    // check for implicit interface implementation
                    //
                    foundImplementation = findSameSignature(member->asPROPSYM(), classToSearchIn);
                    if (foundImplementation) {
                        if (!foundImplementation->isStatic && 
                            (foundImplementation->access == ACC_PUBLIC) &&
                            (foundImplementation->retType == member->asPROPSYM()->retType) &&
                            (!foundImplementation->isOverride)) {
                            // found a match
                            break;
                        } else {
                            //
                            // found a close match, save it for error reporting
                            // and continue checking
                            //
                            if (!closeImplementation) {
                                closeImplementation = foundImplementation;
                            }
                            foundImplementation = NULL;
                        }
                    }
            
                    classToSearchIn = classToSearchIn->baseClass;
                } while (classToSearchIn);

                if (!foundImplementation && !foundBaseImplementation) {
                    //
                    // didn't find an implementation
                    //
                    if (!closeImplementation) {
                        errorSymbolAndRelatedSymbol(cls, member, ERR_UnimplementedInterfaceMember);
                    } else {
                        errorSymbolSymbolSymbol(cls, member, closeImplementation, ERR_CloseUnimplementedInterfaceMember);
                    }
                } else {
                    //
                    // check that all accessors are implemented
                    //
                    if (foundImplementation) {

                        if (member->asPROPSYM()->methGet && !foundImplementation->methGet) {
                            errorSymbolAndRelatedSymbol(cls, member->asPROPSYM()->methGet, ERR_UnimplementedInterfaceMember);
                        } else if (member->asPROPSYM()->methGet) {
#ifdef DEBUG
                            METHSYM *methGet =
#endif // DEBUG
                            needExplicitImpl(member->asPROPSYM()->methGet, foundImplementation->methGet, cls);
                            // if the implementing method can't be set to isMetadataVirtual
                            // then we should have added a compiler generated explcit impl in needExplicitImpl
                            ASSERT(methGet->isMetadataVirtual);
                        }
                        if (member->asPROPSYM()->methSet && !foundImplementation->methSet) {
                            errorSymbolAndRelatedSymbol(cls, member->asPROPSYM()->methSet, ERR_UnimplementedInterfaceMember);
                        } else if (member->asPROPSYM()->methSet) {
#ifdef DEBUG
                            METHSYM *methSet =
#endif // DEBUG
                            needExplicitImpl(member->asPROPSYM()->methSet, foundImplementation->methSet, cls);
                            // if the implementing method can't be set to isMetadataVirtual
                            // then we should have added a compiler generated explcit impl in needExplicitImpl
                            ASSERT(methSet->isMetadataVirtual);
                        }
                    }
                }
                break;
            }
            case SK_EVENTSYM:
            {
                if (member->asEVENTSYM()->isBogus) {
                    //
                    // don't need implementation of bogus events
                    // just need implementation of accessors as regular methods
                    //
                    break;
                }
                BOOL foundBaseImplementation = false;
                AGGSYM *classToSearchIn = cls;
                EVENTSYM *foundImplementation;
                SYM *closeImplementation = NULL;
                do {
                    //
                    // check for explicit interface implementation
                    //
                    foundImplementation = findExplicitInterfaceImplementation(classToSearchIn, member)->asEVENTSYM();
                    if (foundImplementation) {
                        break;
                    }

                    //
                    // check for imported base class which implements this interface
                    //
                    if (!classToSearchIn->parseTree && classToSearchIn->allIfaceList->contains(iface)) {
                        foundBaseImplementation = true;
                        break;
                    }

                    //
                    // check for implicit interface implementation
                    //
                    foundImplementation = compiler()->symmgr.LookupGlobalSym(member->name, classToSearchIn, MASK_EVENTSYM)->asEVENTSYM();
                    if (foundImplementation) {
                        if (!foundImplementation->isStatic && 
                            (foundImplementation->access == ACC_PUBLIC) &&
                            (foundImplementation->type == member->asEVENTSYM()->type)) {
                            // found a match
                            break;
                        } else {
                            //
                            // found a close match, save it for error reporting
                            // and continue checking
                            //
                            if (!closeImplementation) {
                                closeImplementation = foundImplementation;
                            }
                            foundImplementation = NULL;
                        }
                    }
            
                    classToSearchIn = classToSearchIn->baseClass;
                } while (classToSearchIn && !foundImplementation);

                if (!foundImplementation && !foundBaseImplementation) {
                    //
                    // didn't find an implementation
                    //
                    if (!closeImplementation) {
                        errorSymbolAndRelatedSymbol(cls, member, ERR_UnimplementedInterfaceMember);
                    } else {
                        errorSymbolSymbolSymbol(cls, member, closeImplementation, ERR_CloseUnimplementedInterfaceMember);
                    }
                }
                else {
                    if (foundImplementation) {
                        needExplicitImpl(member->asEVENTSYM()->methAdd, foundImplementation->asEVENTSYM()->methAdd, cls);
                        needExplicitImpl(member->asEVENTSYM()->methRemove, foundImplementation->asEVENTSYM()->methRemove, cls);
                    }
                }
                break;
            }
            case SK_ARRAYSYM:
            case SK_EXPANDEDPARAMSSYM:
            case SK_PARAMMODSYM:
            case SK_PTRSYM:
                break;
            default:
                ASSERT(!"Unknown interface member");
            }
        ENDFOREACHCHILD
    ENDFOREACHSYMLIST

}

//
// checks if we need a compiler generated explicit method impl
// returns the actual method implementing the interface method
//
METHSYM *CLSDREC::needExplicitImpl(METHSYM *ifaceMethod, METHSYM *implMethod, AGGSYM *cls)
{
    if (!implMethod->explicitImpl &&                            // user defined explicit impl
        (ifaceMethod->hasCmodOpt || implMethod->hasCmodOpt ||   // differing signatures
        (ifaceMethod->name != implMethod->name) ||              // can happen when implementing properties
        !implMethod->isMetadataVirtual && 
            (!implMethod->getInputFile()->isSource ||
             // always add explicit impls when a base class implements a derived class during an incremental build
             // When rebuilding base class only during an incremental build we won't know to set the method to mdVirtual
             ((implMethod->getClass() != cls) && compiler()->options.m_fINCBUILD))           // non-virtual method which we can't artificially mark as virtual
        )) 
    {
        IFACEIMPLMETHSYM *impl = compiler()->symmgr.CreateIfaceImplMethod(cls);

        ifaceMethod->copyInto(impl);
        impl->access = ACC_PRIVATE;
        impl->isOverride = false;
        impl->hasCmodOpt = ifaceMethod->hasCmodOpt;
        impl->explicitImpl = ifaceMethod;
        impl->implMethod = implMethod;
        impl->parseTree = cls->parseTree;
        impl->isAbstract = false;
        impl->isMetadataVirtual = true;

        return impl;
    } else {
        implMethod->isMetadataVirtual = true;
        return implMethod;
    }
}

// prepares an enumerator
void CLSDREC::prepareEnum(AGGSYM *cls)
{
    ASSERT(cls->isDefined);

    cls->isPrepared = true;
}

// define a class.  this means bind its base class and implemented interface
// list as well as define the class elements.
//
// returns false if this type is involved in a cycle in the inheritance chain
//
// NOTE: this does not work for the predefined 'object' type.
void CLSDREC::defineAggregate(AGGSYM *cls)
{
    ASSERT(!(cls->isEnum && (cls->isStruct || cls->isClass || cls->isDelegate)));
    ASSERT(cls->getInputFile()->isSource);
    ASSERT(cls->parseTree);

    // check if we're done already
    if (cls->isDefined) {
        return;
    }
    ASSERT(cls != compiler()->symmgr.GetObject());

#ifdef DEBUG
    compiler()->haveDefinedAnyType = true;
#endif

    ASSERT(!cls->isResolvingBaseClasses);
    resolveInheritanceRec(cls);
    ASSERT(!cls->isResolvingBaseClasses && cls->hasResolvedBaseClasses);

    SETLOCATIONSYM(cls);
    cls->isDefined = true;

    //
    // check for new protected in a sealed class
    //
    if ((cls->access == ACC_PROTECTED || cls->access == ACC_INTERNALPROTECTED) &&
        cls->parent->kind == SK_AGGSYM &&
        cls->parent->asAGGSYM()->isSealed) {

        errorSymbol(cls, cls->isStruct ? ERR_ProtectedInStruct : WRN_ProtectedInSealed);
    }

    //
    // do the members
    //
    if (cls->isEnum) {
        defineEnumMembers(cls);
    } else if (cls->isDelegate) {
        defineDelegateMembers(cls);
    } else {
        defineAggregateMembers(cls);
    }

    ASSERT(cls->isDefined);
    ASSERT(!(cls->isEnum && (cls->isStruct || cls->isClass || cls->isDelegate)));
}

// handles the special case of bringing object up to defined state
void CLSDREC::defineObject()
{
    AGGSYM *object = compiler()->symmgr.GetObject();

    if (object) {
        ASSERT(!object->isDefined && !object->isResolvingBaseClasses);

        SETLOCATIONSYM(object);

        if (object->getInputFile()->isSource) {
            // we are defining object. Better not have a base class.
            NODELOOP(object->parseTree->asAGGREGATE()->pBases, BASE, base)
                errorSymbol(object, ERR_ObjectCantHaveBases);
            ENDLOOP;

            object->isDefined = true;
            defineAggregateMembers(object);
        } else {
            //
            // import all of its members from the
            // metadata file
            //
            compiler()->importer.DefineImportedType(object);
            ASSERT(object->isDefined);
            ASSERT(!object->isResolvingBaseClasses);

            object->getInputFile()->isBCL = true;
        }
		
        SYM * dtor = compiler()->symmgr.LookupGlobalSym(compiler()->namemgr->GetPredefName(PN_DTOR), object, MASK_METHSYM);
        if (dtor)
            dtor->asMETHSYM()->isDtor = true;
    }
}

// define a types members, does not do base classes
// and implemented interfaces
void CLSDREC::defineAggregateMembers(AGGSYM *cls)
{
    ASSERT(cls->isDefined);

    //
    // define nested types, interfaces will have no members at this point
    //
    FOREACHCHILD(cls, elem)

        SETLOCATIONSYM(elem);

        // should only have types at this point
        switch (elem->kind) {
        case SK_AGGSYM:
            ASSERT (elem->asAGGSYM()->isClass || elem->asAGGSYM()->isStruct || 
                elem->asAGGSYM()->isInterface || elem->asAGGSYM()->isEnum ||
                elem->asAGGSYM()->isDelegate);

            defineAggregate(elem->asAGGSYM());
            break;

        case SK_ARRAYSYM:
        case SK_EXPANDEDPARAMSSYM:
        case SK_PARAMMODSYM:
        case SK_PTRSYM:
            break;

        default:
            ASSERT(!"Unknown member");
        }
    ENDFOREACHCHILD

    //
    // define members
    //
    // We never generate a non-static constructor for structs...
    bool seenConstructor = cls->isStruct;
    bool seenStaticConstructor = false;
    bool needStaticConstructor = false;
    bool mustMatch = false;
    
    MEMBERNODE * memb = cls->parseTree->asAGGREGATE()->pMembers;
    while (memb) {
    
        SETLOCATIONNODE(memb);

        switch(memb->kind) {
        case NK_DTOR:
        case NK_METHOD:
            defineMethod(memb->asANYMETHOD(), cls);
            break;
        case NK_CTOR:
        {
            METHSYM * method = defineMethod(memb->asCTOR(), cls);
            if (method) {
                if (method->isStatic) {
                    seenStaticConstructor = true;

                } else {
                    seenConstructor = true;
                }
            }
            break;
        }
        case NK_OPERATOR:
            mustMatch |= defineOperator(memb->asOPERATOR(), cls);
            break;
        case NK_FIELD:
            needStaticConstructor |= defineFields(memb->asFIELD(), cls);
            break;
        case NK_NESTEDTYPE:
            // seperate loop for nested types
            break;
        case NK_PROPERTY:
            defineProperty(memb->asPROPERTY(), cls);
            break;
        case NK_INDEXER:
            defineProperty(memb->asINDEXER(), cls);
            break;
        case NK_CONST:
            needStaticConstructor |= defineFields(memb->asCONST(), cls);
            break;
        case NK_PARTIALMEMBER:
            // Ignore this codesense artifact...
            break;
        default:
            ASSERT(!"Unknown node type");
        }


        memb = memb->pNext;
    }

    METHSYM * equalsMember = NULL;
    METHSYM * gethashcodeMember = NULL;
    if (!cls->isInterface) {
        // find public override bool Equals(object)
        for (equalsMember = compiler()->symmgr.LookupGlobalSym(compiler()->namemgr->GetPredefName(PN_EQUALS), cls, MASK_METHSYM)->asMETHSYM();
            equalsMember;
            equalsMember = compiler()->symmgr.LookupNextSym(equalsMember, cls, MASK_METHSYM)->asMETHSYM()) {

            if (equalsMember->isOverride && 
                equalsMember->access == ACC_PUBLIC && 
                equalsMember->cParams == 1 && equalsMember->params[0] == compiler()->symmgr.GetPredefType(PT_OBJECT, false) &&
                equalsMember->retType == compiler()->symmgr.GetPredefType(PT_BOOL, false)) {
                break;
            }
        }

        // find public override int GetHashCode()
        for (gethashcodeMember = compiler()->symmgr.LookupGlobalSym(compiler()->namemgr->GetPredefName(PN_GETHASHCODE), cls, MASK_METHSYM)->asMETHSYM();
            gethashcodeMember;
            gethashcodeMember = compiler()->symmgr.LookupNextSym(gethashcodeMember, cls, MASK_METHSYM)->asMETHSYM()) {

            if (gethashcodeMember->isOverride && 
                gethashcodeMember->access == ACC_PUBLIC && 
                gethashcodeMember->cParams == 0 &&
                gethashcodeMember->retType == compiler()->symmgr.GetPredefType(PT_INT, false)) {
                break;
            }
        }

        if (equalsMember && !gethashcodeMember) {
            errorSymbol(cls, WRN_EqualsWithoutGetHashCode);
        }
    }

    if (mustMatch) {
        checkMatchingOperator(PN_OPEQUALITY, cls);
        checkMatchingOperator(PN_OPINEQUALITY, cls);
        checkMatchingOperator(PN_OPGREATERTHAN, cls);
        checkMatchingOperator(PN_OPLESSTHAN, cls);
        checkMatchingOperator(PN_OPGREATERTHANOREQUAL, cls);
        checkMatchingOperator(PN_OPLESSTHANOREQUAL, cls);
        checkMatchingOperator(PN_OPTRUE, cls);
        checkMatchingOperator(PN_OPFALSE, cls);

        if (!cls->isInterface && (!equalsMember || !gethashcodeMember)) {
            bool foundEqualityOp = false;
            for (METHSYM * op = compiler()->symmgr.LookupGlobalSym(compiler()->namemgr->GetPredefName(PN_OPEQUALITY), cls, MASK_METHSYM)->asMETHSYM();
                op;
                op = compiler()->symmgr.LookupNextSym(op, cls, MASK_METHSYM)->asMETHSYM()) {

                if (op->isOperator) {
                    foundEqualityOp = true;
                    break;
                }
            }

            if (!foundEqualityOp) {
                for (METHSYM * op = compiler()->symmgr.LookupGlobalSym(compiler()->namemgr->GetPredefName(PN_OPINEQUALITY), cls, MASK_METHSYM)->asMETHSYM();
                    op;
                    op = compiler()->symmgr.LookupNextSym(op, cls, MASK_METHSYM)->asMETHSYM()) {

                    if (op->isOperator) {
                        foundEqualityOp = true;
                        break;
                    }
                }
            }

            if (foundEqualityOp) {
                if (!equalsMember) {
                    errorSymbol(cls, WRN_EqualityOpWithoutEquals);
                }
                if (!gethashcodeMember) {
                    errorSymbol(cls, WRN_EqualityOpWithoutGetHashCode);
                }
            }
        }
    }

    //
    // Synthetize the static and non static default constructors if necessary...
    //
    if (!cls->isInterface) {
        cls->hasStaticCtor = seenStaticConstructor;
SYNTHCONSTRUCTOR:
        if (!seenConstructor || (!seenStaticConstructor && needStaticConstructor)) {
            // first time through we do the static constructor, then the default constructor...
            METHSYM *method = compiler()->symmgr.CreateMethod(compiler()->namemgr->GetPredefName((!seenStaticConstructor && needStaticConstructor) ? PN_STATCTOR : PN_CTOR), cls);
            if (!seenStaticConstructor && needStaticConstructor)
                method->access = ACC_PRIVATE; // static ctor is always private
            else if (cls->isAbstract)
                method->access = ACC_PROTECTED; // default constructor for abstract classes is protected.
            else 
                method->access = ACC_PUBLIC;
            method->isCtor = true;
            method->retType = compiler()->symmgr.GetVoid();
            method->parseTree = cls->parseTree;
            if (!seenStaticConstructor && needStaticConstructor) {
                method->isStatic = true;
                seenStaticConstructor = true;
                goto SYNTHCONSTRUCTOR;
            }
        }
    }
}

// Check that all operators w/ the given name have a match
void CLSDREC::checkMatchingOperator(PREDEFNAME pn, AGGSYM * cls)
{
    PREDEFNAME opposite;
    TOKENID oppToken;
    switch (pn) {
    case PN_OPEQUALITY: opposite = PN_OPINEQUALITY; oppToken = TID_NOTEQUAL; break;
    case PN_OPINEQUALITY: opposite = PN_OPEQUALITY; oppToken = TID_EQUALEQUAL; break;
    case PN_OPGREATERTHAN: opposite = PN_OPLESSTHAN; oppToken = TID_LESS; break;
    case PN_OPGREATERTHANOREQUAL: opposite = PN_OPLESSTHANOREQUAL; oppToken = TID_LESSEQUAL; break;
    case PN_OPLESSTHAN: opposite = PN_OPGREATERTHAN; oppToken = TID_GREATER; break;
    case PN_OPLESSTHANOREQUAL: opposite = PN_OPGREATERTHANOREQUAL; oppToken = TID_GREATEREQUAL; break;
    case PN_OPTRUE: opposite = PN_OPFALSE; oppToken = TID_FALSE; break;
    case PN_OPFALSE: opposite = PN_OPTRUE; oppToken = TID_TRUE; break;
    default:
        ASSERT(!"bad pn in checkMatchingOperator");
	return;
    }

    NAME * name = compiler()->namemgr->GetPredefName(pn);
    NAME * oppositeName = compiler()->namemgr->GetPredefName(opposite);

    for (METHSYM * original = compiler()->symmgr.LookupGlobalSym(name, cls, MASK_METHSYM)->asMETHSYM();
        original;
        original = compiler()->symmgr.LookupNextSym(original, cls, MASK_METHSYM)->asMETHSYM()) {

        if (original->isOperator) {
            for (METHSYM * match = compiler()->symmgr.LookupGlobalSym(oppositeName, cls, MASK_METHSYM)->asMETHSYM();
                match;
                match = compiler()->symmgr.LookupNextSym(match, cls, MASK_METHSYM)->asMETHSYM()) {

                if (match->isOperator) {
                    if (match->retType == original->retType && match->params == original->params) {
                        goto MATCHED;
                    }
                }
            }

            LPCWSTR operatorName = CParser::GetTokenInfo(oppToken)->pszText;
            ASSERT(operatorName[0]);

            errorSymbolStr (original, ERR_OperatorNeedsMatch, operatorName);
            return;
        }

MATCHED:;
    }
}

// define all the enumerators in an enum type
void CLSDREC::defineEnumMembers(AGGSYM *cls)
{
    // we do NOT need default & copy constructors

    //
    // define members
    //
    MEMBVARSYM *previousEnumerator = NULL;
    for (ENUMMBRNODE * member = cls->parseTree->asAGGREGATE()->pMembers->asENUMMBR();
         member;
         member = member->pNext->asENUMMBR()) {

        SETLOCATIONNODE(member);

        NAMENODE *nameNode = member->pName;
        PNAME name = nameNode->pName;

        //
        // check for name same as that of parent aggregate
        // check for conflicting enumerator name
        //
        if (!checkForBadMember(name, nameNode, cls)) {
            continue;
        }

        // Check for same names as the reserved "value" enumerators.
        if (name == compiler()->namemgr->GetPredefName(PN_ENUMVALUE)) {
            errorNameRef(nameNode, cls, ERR_ReservedEnumerator);
            continue;
        }

        //
        // create the symbol
        //
        MEMBVARSYM * rval = compiler()->symmgr.CreateMembVar(name, cls);

        rval->type = cls;
        rval->access = ACC_PUBLIC;

        //
        // set link to previous enumerator
        //
        rval->previousEnumerator = previousEnumerator;
        previousEnumerator = rval;

        // set the value parse tree:
        rval->parseTree = member;

        //
        // set the flags to indicate an unevaluated constant
        //
        rval->isUnevaled = true;
        rval->isStatic = true; // consts are implicitly static.
    }
}

// define all the members in a delegate type
void CLSDREC::defineDelegateMembers(AGGSYM *cls)
{
    //
    // constructor taking an object and an uintptr
    //
    {
        METHSYM *method = compiler()->symmgr.CreateMethod(compiler()->namemgr->GetPredefName(PN_CTOR), cls);
        method->access = ACC_PUBLIC;
        method->isCtor = true;
        method->retType = compiler()->symmgr.GetVoid();
        method->parseTree = cls->parseTree;
        method->cParams = 2;

        PTYPESYM paramTypes[2];
        paramTypes[0] = compiler()->symmgr.GetObject();
        paramTypes[1] = compiler()->symmgr.GetPredefType(PT_INTPTR);
        method->params = compiler()->symmgr.AllocParams(2, paramTypes);
    }

    BASENODE * tree = cls->parseTree;
    bool isOldStyleVarargs = !!(tree->other & NFEX_METHOD_VARARGS);
    bool isParamArray = !!(tree->other & NFEX_METHOD_PARAMS);

    if (isOldStyleVarargs) {
        errorFile(tree, cls->getInputFile(), ERR_VarargDelegate);
    }

    //
    // 'Invoke' method
    //
    METHSYM * invokeMethod;
    TYPESYM * retType;
    {

        //
        // bind return type
        //
        DELEGATENODE * delegateNode = cls->parseTree->asDELEGATE();
        bool unsafeContext = ((delegateNode->flags & NF_MOD_UNSAFE) != 0);
        if (!unsafeContext) {
            SYM *context = cls->parent;
            while (!unsafeContext && context && context->kind == SK_AGGSYM) {
                unsafeContext = ((context->getParseTree()->flags & NF_MOD_UNSAFE) != 0);
                context = context->parent; // The while loop condition makes sure this is still an AGGSYM
            }
        }

        PTYPESYM returnType = bindType(delegateNode->pType, cls->containingDeclaration(), BTF_NODECLARE | BTF_NODEPRECATED, NULL);
        if (!returnType) {
            return;
        }

        if (returnType->isSpecialByRefType()) {
            errorStrFile(delegateNode, cls->getInputFile(), ERR_MethodReturnCantBeRefAny, compiler()->ErrSym(returnType));
            return;
        }
        retType = returnType;

        if (!unsafeContext) 
            checkUnsafe(delegateNode->pType, cls->getInputFile(), returnType);

        //
        // bind parameter types
        //

        // common case: fewer than 8 parameters
        unsigned maxParams = 8;
        unsigned cParam = 0;
        PTYPESYM initTypeArray[8];
        PTYPESYM * paramTypes = &(initTypeArray[0]);

        NODELOOP(delegateNode->pParms, PARAMETER, param)
            // double the param list if we reached the end...
            if (cParam == maxParams) {
                maxParams *= 2;
                TYPESYM ** newArray = (TYPESYM **) _alloca(maxParams * sizeof(PTYPESYM));
                memcpy(newArray, paramTypes, cParam * sizeof(PTYPESYM));
                paramTypes = newArray;
            }
            // bind the type, and wrap it if the variable is byref
            PTYPESYM paramType = bindType(param->pType, cls->containingDeclaration(), BTF_NODECLARE | BTF_NODEPRECATED, NULL);
            if (!paramType) {
                return;
            }
            if (param->flags & (NF_PARMMOD_REF | NF_PARMMOD_OUT)) {
                paramType = compiler()->symmgr.GetParamModifier(paramType, param->flags & NF_PARMMOD_OUT ? true : false);
                if (paramType->asPARAMMODSYM()->paramType()->isSpecialByRefType()) {
                    errorStrFile(param, cls->getInputFile(), ERR_MethodArgCantBeRefAny, compiler()->ErrSym(paramType));
                    return;
                }
            }
            paramTypes[cParam++] = paramType;

            if (!unsafeContext) 
                checkUnsafe(param->pType, cls->getInputFile(), paramType);
        ENDLOOP;

        if (isParamArray) {
            checkParamsArgument(tree, cls, cParam, paramTypes);
        }

        // Check return and parameter types for correct visibility.

        checkConstituentVisibility(cls, returnType, ERR_BadVisDelegateReturn);
        
        for (unsigned i = 0; i < cParam; ++i) {
            checkConstituentVisibility(cls, paramTypes[i], ERR_BadVisDelegateParam);
        }

        //
        // create virtual public 'Invoke' method
        //
        METHSYM *method = compiler()->symmgr.CreateMethod(compiler()->namemgr->GetPredefName(PN_INVOKE), cls);
        method->access = ACC_PUBLIC;
        method->retType = returnType;
        method->isVirtual = true;
        method->isMetadataVirtual = true;
        method->isOverride = true;  // magically prevents lookup to invoke
        method->parseTree = cls->parseTree;
        method->params = compiler()->symmgr.AllocParams(cParam, paramTypes);
        method->cParams = cParam;
        method->isParamArray = isParamArray;

        invokeMethod = method;
    }


    //
    // 'BeginInvoke' method
    //
    {
        // These two types might not exist on some platforms. If they don't, then don't create a BeginInvoke/EndInvoke.
        PTYPESYM asynchResult = compiler()->symmgr.GetPredefType(PT_IASYNCRESULT, false);
        PTYPESYM asynchCallback = compiler()->symmgr.GetPredefType(PT_ASYNCCBDEL, false);

        if (asynchResult && asynchCallback) {
            //
            // bind parameter types
            //

            unsigned maxParams = invokeMethod->cParams + 2;
            unsigned cParam = 0;
            PTYPESYM * paramTypes = (PTYPESYM*) _alloca(sizeof(TYPESYM*) * maxParams);

            for (unsigned i = 0; i < invokeMethod->cParams; i++) {
                paramTypes[cParam++] = invokeMethod->params[i];
            }
            paramTypes[cParam++] = asynchCallback;
            paramTypes[cParam++] = compiler()->symmgr.GetPredefType(PT_OBJECT, false);

            //
            // create virtual public 'Invoke' method
            //
            METHSYM *method = compiler()->symmgr.CreateMethod(compiler()->namemgr->GetPredefName(PN_BEGININVOKE), cls);
            method->access = ACC_PUBLIC;
            method->retType = asynchResult;
            method->isVirtual = true;
            method->parseTree = cls->parseTree;
            method->params = compiler()->symmgr.AllocParams(cParam, paramTypes);
            method->cParams = cParam;
            method->isParamArray = isParamArray;
        }
    }

    //
    // 'EndInvoke' method
    //
    {
        // These two types might not exist on some platforms. If they don't, then don't create a BeginInvoke/EndInvoke.
        PTYPESYM asynchResult = compiler()->symmgr.GetPredefType(PT_IASYNCRESULT, false);
        PTYPESYM asynchCallback = compiler()->symmgr.GetPredefType(PT_ASYNCCBDEL, false);

        if (asynchResult && asynchCallback) {

            //
            // bind parameter types
            //

            // common case: fewer than 8 parameters
            unsigned maxParams = invokeMethod->cParams + 1;
            unsigned cParam = 0;
            PTYPESYM * paramTypes = (PTYPESYM*) _alloca(sizeof(TYPESYM*) * maxParams);

            for (unsigned i = 0; i < invokeMethod->cParams; i++) {
                if (invokeMethod->params[i]->kind == SK_PARAMMODSYM) {
                    paramTypes[cParam++] = invokeMethod->params[i];
                }
            }
            paramTypes[cParam++] = compiler()->symmgr.GetPredefType(PT_IASYNCRESULT, false);

            //
            // create virtual public 'Invoke' method
            //
            METHSYM *method = compiler()->symmgr.CreateMethod(compiler()->namemgr->GetPredefName(PN_ENDINVOKE), cls);
            method->access = ACC_PUBLIC;
            method->retType = retType;
            method->isVirtual = true;
            method->parseTree = cls->parseTree;
            method->params = compiler()->symmgr.AllocParams(cParam, paramTypes);
            method->cParams = cParam;
        }
    }
}

// declare all the types in an input file
void CLSDREC::declareInputfile(NAMESPACENODE * parseTree, PINFILESYM inputfile)
{
    ASSERT(parseTree && inputfile->isSource && (!inputfile->rootDeclaration || !inputfile->rootDeclaration->parseTree));

    SETLOCATIONFILE(inputfile);
    SETLOCATIONNODE(parseTree);

    //
    // create the new delcaration
    //
    inputfile->rootDeclaration = compiler()->symmgr.CreateNamespaceDeclaration(
                compiler()->symmgr.GetRootNS(), 
                NULL,                               // no declaration parent
                inputfile, 
                parseTree);

    //
    // declare everything in the declaration
    //
    declareNamespace(inputfile->rootDeclaration);
}

// declare a namespace by entering it into the symbol table, and recording it in the
// parsetree...  nspace is the parent namespace.
void CLSDREC::declareNamespace(PNSDECLSYM declaration)
{
    //
    // declare members
    // 
    NODELOOP(declaration->parseTree->pElements, BASE, elem)

        SETLOCATIONNODE(elem);

        switch (elem->kind) {
        case NK_NAMESPACE:
        {
            NAMESPACENODE * nsnode = elem->asNAMESPACE();
            NSDECLSYM *newDeclaration;

            // we can not be the global namespace
            ASSERT(nsnode->pName);

            BASENODE *namenode = nsnode->pName;
            // is our name simple, or a qualified one...
            if (namenode->kind == NK_NAME) {
                newDeclaration = addNamespaceDeclaration(namenode->asNAME(), nsnode, declaration);
            } else {
                if (!declaration->isRootDeclaration()) {
                    // if we have a dotted name, our parent must be the root namespace
                    errorNameRef(namenode, declaration, ERR_IllegalQualifiedNamespace);
                    break;
                }

                //
                // get the first component of the dotted name
                //
                BASENODE * first = namenode;
                while (namenode->asDOT()->p1->kind == NK_DOT) {
                    namenode = namenode->asDOT()->p1;
                }

                //
                // add all the namespaces in the dotted name
                //
                newDeclaration = addNamespaceDeclaration(namenode->asDOT()->p1->asNAME(), nsnode, declaration);
                while (newDeclaration) {
                    //
                    // don't add using clauses for the declaration to any but the last 
                    // namespace declaration in the list.
                    //
                    newDeclaration->usingClausesResolved = true;

                    ASSERT(namenode->kind == NK_DOT);
                    newDeclaration = addNamespaceDeclaration(namenode->asDOT()->p2->asNAME(), nsnode, newDeclaration);
                    if (!newDeclaration) {
                        break;
                    }
                    if (namenode == first) {
                        break;
                    }
                    namenode = namenode->pParent;
                }
            }
            if (newDeclaration) {
                declareNamespace(newDeclaration);
            }
            break;
        }
        case NK_CLASS:
        case NK_STRUCT:
        case NK_INTERFACE:
        case NK_ENUM:
            declareAggregate(elem->asAGGREGATE(), declaration);
            break;

        case NK_DELEGATE:
        {
            DELEGATENODE *delegateNode = elem->asDELEGATE();
            addAggregate(delegateNode, delegateNode->pName, declaration);
            break;
        }

        default:
            ASSERT(!"unknown namespace member");
            break;
        }
    ENDLOOP;
}

/*
 * creates a global attribute symbol  for attributes on modules and assemblies
 */
void CLSDREC::declareGlobalAttribute(ATTRDECLNODE *pNode, NSDECLSYM *declaration)
{
    PGLOBALATTRSYM *attributeLocation;
    CorAttributeTargets elementKind;

    // check the global attribute's name
    if (pNode->location == AL_ASSEMBLY) {
        attributeLocation = &compiler()->assemblyAttributes;
        elementKind = catAssembly;
    } else {
        // parser guarantees that the name is correct for global attributes
        ASSERT(pNode->location == AL_MODULE);

        attributeLocation = &declaration->getInputFile()->getOutputFile()->attributes;
        elementKind = catModule;
    }

    declaration->getInputFile()->hasGlobalAttr = true;

    // create the attribute
    GLOBALATTRSYM *attr = compiler()->symmgr.CreateGlobalAttribute(pNode->pNameNode->pName, declaration);
    attr->parseTree = pNode;
    attr->elementKind = elementKind;
    attr->nextAttr = *attributeLocation;

    *attributeLocation = attr;
}

/*
 * Once the "prepare" stage is done, classes must be compiled and metadata emitted
 * in three distinct stages in order to make sure that the metadata emitting is done
 * most efficiently. The stages are:
 *   EmitTypeDefs -- typedefs must be created for each aggregate types
 *   EmitMemberDefs -- memberdefs must be created for each member of aggregate types
 *   Compile -- Compile method bodies.
 * 
 * To conform to the most efficient metadata emitting scheme, each of the three stages
 * must be done in the exact same order. Furthermore, the first stage must be done
 * in an order that does base classes and interfaces before derived classes and interfaces --
 * e.g., a topological sort of the classes. So every stage must go in this same topological
 * order.
 */

// Emit typedefs for all aggregates in this namespace declaration...  
void CLSDREC::emitTypedefsNamespace(NSDECLSYM *nsDeclaration)
{
    SETLOCATIONSYM(nsDeclaration);

    //
    // emit typedefs for each aggregate type.
    //
    FOREACHCHILD(nsDeclaration, elem)
        switch (elem->kind) {
        case SK_NSDECLSYM:
            emitTypedefsNamespace(elem->asNSDECLSYM());
            break;
        case SK_AGGSYM:
            emitTypedefsAggregate(elem->asAGGSYM());
            break;
        case SK_GLOBALATTRSYM:
            break;
        default:
            ASSERT(!"Unknown type");
        }
    ENDFOREACHCHILD
}

// Emit typedefs for this aggregates 
void CLSDREC::emitTypedefsAggregate(AGGSYM * cls)
{
    OUTFILESYM * outputFile;
    AGGSYM * base;

    // If we've already hit this one (because it was a base of someone earlier),
    // then nothing more to do.
    if (cls->isTypeDefEmitted) {
        ASSERT(compiler()->options.m_fNOCODEGEN || cls->tokenEmit);
        return;             // already did it.
    }

    SETLOCATIONSYM(cls);

    ASSERT(!cls->tokenEmit);

    // Do base classes and base interfaces, if they are in the same output scope.
    outputFile = cls->getInputFile()->getOutputFile();

    // Do the base class 
    base = cls->baseClass;
    if (base && base->getInputFile()->getOutputFile() == outputFile)
        emitTypedefsAggregate(base);

    // Iterate the base interfaces.
    FOREACHSYMLIST(cls->ifaceList, baseIface)

        base = baseIface->asAGGSYM();
        if (base->getInputFile()->getOutputFile() == outputFile)
            emitTypedefsAggregate(base);

    ENDFOREACHSYMLIST

    // we need to do outer classes before we do the nested classes
    if (cls->parent->kind == SK_AGGSYM && !cls->parent->asAGGSYM()->isTypeDefEmitted) {
        emitTypedefsAggregate(cls->parent->asAGGSYM());
        ASSERT(cls->isTypeDefEmitted);
        return;
    }

    // Do this aggregate.
    if (!compiler()->options.m_fNOCODEGEN)
        compiler()->emitter.EmitAggregateDef(cls);
    cls->isTypeDefEmitted = true;

    // Do child aggregates.
    FOREACHCHILD(cls, child)
        if (child->kind == SK_AGGSYM) {
            emitTypedefsAggregate(child->asAGGSYM());
        }
    ENDFOREACHCHILD
}

// Emit memberdefs for all aggregates in this namespace...  
void CLSDREC::emitMemberdefsNamespace(NSDECLSYM *nsDeclaration)
{
    SETLOCATIONSYM(nsDeclaration);

    //
    // emit memberdefs for each aggregate type.
    //
    FOREACHCHILD(nsDeclaration, elem)
        switch (elem->kind) {
        case SK_NSDECLSYM:
            emitMemberdefsNamespace(elem->asNSDECLSYM());
            break;
        case SK_AGGSYM:
            emitMemberdefsAggregate(elem->asAGGSYM());
            break;
        case SK_GLOBALATTRSYM:
            break;
        default:
            ASSERT(!"Unknown type");
        }
    ENDFOREACHCHILD
}

// Emit memberdefs for all aggregates in this namespace...  
void CLSDREC::reemitMemberdefsNamespace(NSDECLSYM *nsDeclaration)
{
    SETLOCATIONSYM(nsDeclaration);

    //
    // emit memberdefs for each aggregate type.
    //
    FOREACHCHILD(nsDeclaration, elem)
        switch (elem->kind) {
        case SK_NSDECLSYM:
            reemitMemberdefsNamespace(elem->asNSDECLSYM());
            break;
        case SK_AGGSYM:
            reemitMemberdefsAggregate(elem->asAGGSYM());
            break;
        case SK_GLOBALATTRSYM:
            break;
        default:
            ASSERT(!"Unknown type");
        }
    ENDFOREACHCHILD
}

void CLSDREC::EnumMembersInEmitOrder(AGGSYM *cls, VOID *info, MEMBER_OPERATION doMember)
{
    // Constant fields go first
    FOREACHCHILD(cls, child)
        if (child->kind == SK_MEMBVARSYM && child->asMEMBVARSYM()->isConst)
        {
            (this->*doMember)(child, info);
        }
    ENDFOREACHCHILD

    // Emit the other children.
    FOREACHCHILD(cls, child)

        SETLOCATIONSYM(child);

        switch (child->kind) {
        case SK_MEMBVARSYM:
            if (!child->asMEMBVARSYM()->isConst) {
                (this->*doMember)(child, info);
            }
            break;
        case SK_PROPSYM:
            {
                PROPSYM *property = child->asPROPSYM();
                if (property->methSet && property->methGet) 
                {
                    // emit accessors in declared order
                    if (property->methGet->parseTree->asACCESSOR()->tokidx <
                        property->methSet->parseTree->asACCESSOR()->tokidx)
                    {
                        (this->*doMember)(property->methGet, info);
                        (this->*doMember)(property->methSet, info);
                    }
                    else 
                    {
                        (this->*doMember)(property->methSet, info);
                        (this->*doMember)(property->methGet, info);
                    }
                } 
                else if (property->methGet) 
                {
                    (this->*doMember)(property->methGet, info);
                } 
                else 
                {
                    (this->*doMember)(property->methSet, info);
                }
            }
            break;
        case SK_METHSYM:
            // accessors are done off of the property
            if (!child->asMETHSYM()->isPropertyAccessor)
            {
                (this->*doMember)(child, info);
            }
            break;
	default:
	    break;
        }
    ENDFOREACHCHILD

    // Properties/events must be done after methods.
    FOREACHCHILD(cls, child)
        if (child->kind == SK_PROPSYM || child->kind == SK_EVENTSYM) 
        {
            (this->*doMember)(child, info);
        }
    ENDFOREACHCHILD
}

void CLSDREC::EmitMemberdef(SYM *member, VOID *unused)
{
    SETLOCATIONSYM(member);
    switch (member->kind)
    {
    case SK_MEMBVARSYM:
        compiler()->emitter.EmitMembVarDef  (member->asMEMBVARSYM());
        break;
    case SK_METHSYM:
        compiler()->emitter.EmitMethodDef   (member->asMETHSYM());
        break;
    case SK_PROPSYM:
        compiler()->emitter.EmitPropertyDef (member->asPROPSYM());
        break;
    case SK_EVENTSYM:
        compiler()->emitter.EmitEventDef    (member->asEVENTSYM());
        break;

    default:
        ASSERT(!"Invalid member sym");
    }
}

// Emit memberdefs for this aggregate 
//
// The ordering of items here must EXACTLY match that in compileAggregate.
void CLSDREC::emitMemberdefsAggregate(AGGSYM * cls)
{
    OUTFILESYM * outputFile;
    AGGSYM * base;

    // If we've already hit this one (because it was a base of someone earlier),
    // then nothing more to do.
    if (cls->isMemberDefsEmitted) {
        return;             // already did it.
    }

    // If we couldn't emit the class don't try to emit the members
    // just exit.
    if (TypeFromToken(cls->tokenEmit) != mdtTypeDef || cls->tokenEmit == mdTypeDefNil) {
        return;
    }

    SETLOCATIONSYM(cls);

    ASSERT(!cls->isMemberDefsEmitted);

    // Do base classes and base interfaces, if they are in the same output scope.
    outputFile = cls->getInputFile()->getOutputFile();

    // Do the base class
    base = cls->baseClass;
    if (base && base->getInputFile()->getOutputFile() == outputFile)
        emitMemberdefsAggregate(base);

    // Iterate the base interfaces.
    FOREACHSYMLIST(cls->ifaceList, baseIface)

        base = baseIface->asAGGSYM();
        if (base->getInputFile()->getOutputFile() == outputFile)
            emitMemberdefsAggregate(base);

    ENDFOREACHSYMLIST

    ASSERT(!cls->isMemberDefsEmitted);

    // To do this in the same order as the Aggregate defs
    // we need to do outer classes before we do the nested classes
    if (cls->parent->kind == SK_AGGSYM && !cls->parent->asAGGSYM()->isMemberDefsEmitted) {
        emitMemberdefsAggregate(cls->parent->asAGGSYM());
        ASSERT(cls->isMemberDefsEmitted);
        return;
    }

    // Do any special fields of this aggregate.
    compiler()->emitter.EmitAggregateSpecialFields(cls);

    ASSERT(!cls->isMemberDefsEmitted);

    
    // Emit the children.
    EnumMembersInEmitOrder(cls, 0, &CLSDREC::EmitMemberdef);
    ASSERT(!cls->isMemberDefsEmitted);
    cls->isMemberDefsEmitted = true;

    // Do child aggregates.
    FOREACHCHILD(cls, child)
        if (child->kind == SK_AGGSYM) {
            emitMemberdefsAggregate(child->asAGGSYM());
        }
    ENDFOREACHCHILD
}

// Emit memberdefs for this aggregate 
//
// The ordering of items here must EXACTLY match that in compileAggregate.
void CLSDREC::reemitMemberdefsAggregate(AGGSYM * cls)
{
    OUTFILESYM * outputFile;
    AGGSYM * base;

    // If we've already hit this one (because it was a base of someone earlier),
    // then nothing more to do.
    if (cls->isMemberDefsEmitted) {
        return;             // already did it.
    }

    // If we couldn't emit the class don't try to emit the members
    // just exit.
    if (TypeFromToken(cls->tokenEmit) != mdtTypeDef || cls->tokenEmit == mdTypeDefNil) {
        return;
    }

    SETLOCATIONSYM(cls);

    ASSERT(!cls->isMemberDefsEmitted);

    // Do base classes and base interfaces, if they are in the same output scope.
    outputFile = cls->getInputFile()->getOutputFile();

    // Do the base class
    base = cls->baseClass;
    if (base && base->getInputFile()->getOutputFile() == outputFile)
        reemitMemberdefsAggregate(base);

    // Iterate the base interfaces.
    FOREACHSYMLIST(cls->ifaceList, baseIface)

        base = baseIface->asAGGSYM();
        if (base && base->getInputFile()->getOutputFile() == outputFile)
            reemitMemberdefsAggregate(base);

    ENDFOREACHSYMLIST

    ASSERT(!cls->isMemberDefsEmitted);

    // To do this in the same order as the Aggregate defs
    // we need to do outer classes before we do the nested classes
    if (cls->parent->kind == SK_AGGSYM && !cls->parent->asAGGSYM()->isMemberDefsEmitted) {
        reemitMemberdefsAggregate(cls->parent->asAGGSYM());
        ASSERT(cls->isMemberDefsEmitted);
        return;
    }

    // do the members for this class
    compiler()->importer.ReadExistingMemberTokens(cls);
    cls->isMemberDefsEmitted = true;

    // Do child aggregates.
    FOREACHCHILD(cls, child)
        if (child->kind == SK_AGGSYM) {
            reemitMemberdefsAggregate(child->asAGGSYM());
        }
    ENDFOREACHCHILD
}



// Compile all members of this namespace...  nspace is this namespace...
void CLSDREC::compileNamespace(NSDECLSYM *nsDeclaration)
{
    SETLOCATIONSYM(nsDeclaration);

    if (bSaveDocs)
        DocSym(nsDeclaration);

    //
    // compile members
    //
    FOREACHCHILD(nsDeclaration, elem)
        switch (elem->kind) {
        case SK_NSDECLSYM:
            compileNamespace(elem->asNSDECLSYM());
            break;
        case SK_AGGSYM:
            ASSERT (elem->asAGGSYM()->isClass || elem->asAGGSYM()->isStruct || 
                    elem->asAGGSYM()->isInterface || elem->asAGGSYM()->isEnum ||
                    elem->asAGGSYM()->isDelegate);
            compileAggregate(elem->asAGGSYM(), false);
            break;
        case SK_GLOBALATTRSYM:
            break;
        default:
            ASSERT(!"Unknown type");
        }
    ENDFOREACHCHILD

    // Do CLS name checking, after compiling members so we know who is Compliant
    if (compiler()->CheckForCLS() && nsDeclaration->hasExternalAccess() && !nsDeclaration->getScope()->checkedForCLS)
        checkCLSnaming(nsDeclaration->getScope());
}

// Compile all decls in a field node... all this does is emit the md...
void CLSDREC::compileField(MEMBVARSYM * field, AGGINFO * aggInfo)
{
    ASSERT(field);

    MEMBVARINFO info;
    memset(&info, 0, sizeof(info));
    MEMBVARATTRBIND::Compile(compiler(), field, &info, aggInfo);
    if (compiler()->CheckForCLS() && field->checkForCLS() && field->hasExternalAccess() && !field->type->isCLS_Type())
        errorSymbol( field, ERR_CLS_BadFieldPropType);

    if (!aggInfo->isUnsafe && !field->isStatic && field->getClass()->isStruct &&
        field->type->isUnsafe() && !(field->getParseTree()->pParent->flags & NF_MOD_UNSAFE)) {
        // this field is unsafe, normally this is checked in FUNCBREC::bindInitializers
        // but for instance fields of structs, we have to check it here
        // NOTE: the aggInfo->isUnsafe automatically propigates unsafe context from outer types.
        checkUnsafe(field->getParseTree(), field->getInputFile(), field->type);
    }

    if (bSaveDocs)
        DocSym(field);
}

void CLSDREC::CompileMember(SYM *member, AGGINFO *info)
{
    SETLOCATIONSYM(member);
    switch (member->kind)
    {
    case SK_MEMBVARSYM:
        compileField    (member->asMEMBVARSYM(),info);
        break;
    case SK_METHSYM:
        compileMethod   (member->asMETHSYM(),   info);
        compiler()->DiscardLocalState();
        break;
    case SK_PROPSYM:
        compileProperty (member->asPROPSYM(),   info);
        break;
    case SK_EVENTSYM:
        compileEvent    (member->asEVENTSYM(),  info);
        break;

    default:
        ASSERT(!"Invalid member sym");
    }
}

// compile all memebers of a class, this emits the class md, and loops through its
// children...
//
// The ordering of items here must EXACTLY match that in EmitMemberDefsAggregate.
void CLSDREC::compileAggregate(AGGSYM * cls, bool UnsafeContext)
{
    // If we've already hit this one (because it was a base of someone earlier),
    // then nothing more to do.
    if (cls->isCompiled) {
        return;             // already did it.
    }

    // If we couldn't emit the class don't try to compile it (unless codegen is off, of course, in which case
    // we won't have a token even if everything was good).
    if (!compiler()->options.m_fNOCODEGEN && (TypeFromToken(cls->tokenEmit) != mdtTypeDef || cls->tokenEmit == mdTypeDefNil)) {
        return;
    }

    ASSERT((cls->parseTree  && cls->getInputFile()->hasChanged) || (!cls->parseTree  && !cls->getInputFile()->hasChanged));
    if (!cls->parseTree) {
        // this is an unchanged class
        cls->isCompiled = true;
        return;
    }
    ASSERT(cls->parseTree);
    SETLOCATIONSYM(cls);

    if (bSaveDocs)
        DocSym(cls);

    // Do base classes and base interfaces, if they are in the same output scope.
    OUTFILESYM * outputFile = cls->getInputFile()->getOutputFile();
    AGGSYM * base;

    // Do the base class
    base = cls->baseClass;
    if (base && base->getInputFile()->getOutputFile() == outputFile)
        compileAggregate(base, false);

    // Iterate the base interfaces.
    FOREACHSYMLIST(cls->ifaceList, baseIface)

        base = baseIface->asAGGSYM();
        if (base->getInputFile()->getOutputFile() == outputFile)
            compileAggregate(base, false);

    ENDFOREACHSYMLIST

    // Do outer classes before nested classes
    if (cls->parent->kind == SK_AGGSYM && !cls->parent->asAGGSYM()->isCompiled) {
        compileAggregate(cls->parent->asAGGSYM(), false);
        ASSERT(cls->isCompiled);
        return;
    }

    //
    // emit the class header
    //
    AGGINFO info;
    memset(&info, 0, sizeof(info));

    if (cls->hasInnerFields()) {
        info.offsetList = (unsigned *) _alloca(sizeof(unsigned) * cls->getFieldsSize());

        cls->storeFieldOffsets(info.offsetList);
    }
    info.isUnsafe = UnsafeContext || ((cls->parseTree->flags & NF_MOD_UNSAFE) != 0);

    AGGATTRBIND::Compile(compiler(), cls, &info);

    //
    // compile & emit members
    //
    EnumMembersInEmitOrder(cls, &info, (MEMBER_OPERATION) &CLSDREC::CompileMember);
    ASSERT(!cls->isCompiled);
    cls->isCompiled = true;  // We've compiled all of our members.

    // Nested classes must be done after other members.
    FOREACHCHILD(cls, child)

        SETLOCATIONSYM(child);

        if (child->kind == SK_AGGSYM) {
            compileAggregate(child->asAGGSYM(), info.isUnsafe);
        }
    ENDFOREACHCHILD

    // Do CLS name checking, after compiling members so we know who is Compliant
    if (compiler()->CheckForCLS() && cls->checkForCLS() && cls->hasExternalAccess())
    {
        checkCLSnaming(cls);

        if (cls->isAttribute)
        {
            bool foundCLSCtor = false;
            FOREACHCHILD(cls, child)
                if (child->kind == SK_METHSYM && child->name == compiler()->namemgr->GetPredefName(PN_CTOR)
                    && child->hasExternalAccess()) {
                    METHPROPSYM *member = child->asMETHPROPSYM();
		    int i;
                    for (i = 0; i < member->cParams; i += 1)
                    {
                        if (member->params[i]->kind == SK_ARRAYSYM || !isAttributeType(member->params[i]))
                        {
                            break;
                        }
                    }
                    if (i == member->cParams) {
                        foundCLSCtor = true;
                        break;
                    }
                }
            ENDFOREACHCHILD
            if (!foundCLSCtor) {
                errorSymbol(cls, ERR_CLS_BadAttributeType);
            }
        }
    }
}

// Compile a method...
void CLSDREC::compileMethod(METHSYM * method, AGGINFO * aggInfo)
{

    BASENODE * tree = method->parseTree;

    METHINFO info;
    memset(&info, 0, sizeof(info));

    if (method->isEventAccessor && method->parseTree->kind != NK_ACCESSOR) {
        // This is an auto-generated event accessor, not a user-defined one.
        if (! method->parent->asAGGSYM()->isStruct)
            info.isSynchronized = true; // synchronize this method if not on a struct.
        info.noDebugInfo = true;   // Don't generate debug info for this method, since there is no source code.
    }
    if (method->isPropertyAccessor) {
        if (method->getParseTree()->pParent->flags & NF_MOD_UNSAFE) { 
            info.isUnsafe = true;
        }
    }
    if (method->getParseTree()->flags & NF_MOD_UNSAFE) {
        info.isUnsafe = true;
    }

    info.paramInfos = (PARAMINFO *) _alloca(method->cParams * sizeof (PARAMINFO));
    memset(info.paramInfos, 0, method->cParams * sizeof (PARAMINFO));

    if (method->isParamArray && (!method->getClass()->isDelegate || method->name == compiler()->namemgr->GetPredefName(PN_INVOKE))) {
        int paramIndex;
		if (method->isPropertyAccessor && !method->isGetAccessor()) {
            paramIndex = method->cParams - 2;   // skip the implicit value parameter
        } else {
            paramIndex = method->cParams - 1;
        }
        info.paramInfos[paramIndex].isParamArray = true;
    }

    if (method->getClass()->isDelegate) {
        // all delegate members are implemented by the runtime
        info.isMagicImpl = true;

        // delegate constructors have no names

        // invoke grabs parameter names from the delegate declaration
        if (method->name == compiler()->namemgr->GetPredefName(PN_INVOKE)) {
            DELEGATENODE * delegateNode = method->getClass()->getParseTree()->asDELEGATE();
            int i = 0;
            NODELOOP(delegateNode->pParms, PARAMETER, param)
                NAME * name = param->pName->pName;

                // check for a duplicate parameter name
                for (int j = 0; j < i; j += 1) {
                    if (name == info.paramInfos[j].name) {
                        errorNameRef(param->pName, method->getClass(), ERR_DuplicateParamName);
                        break;
                    }
                }

                info.paramInfos[i].name = name;

                i += 1;
            ENDLOOP
            ASSERT(i == method->cParams);
        } else if (method->name == compiler()->namemgr->GetPredefName(PN_BEGININVOKE)) {
            // we don't check duplicates here, only invoke will do it...
            DELEGATENODE * delegateNode = method->getClass()->getParseTree()->asDELEGATE();
            int i = 0;
            int j = 0;
            NODELOOP(delegateNode->pParms, PARAMETER, param)
                NAME * name = param->pName->pName;

                info.paramInfos[i].name = name;
                
                i += 1;
                j += 1;
            ENDLOOP;

            info.paramInfos[i++].name = compiler()->namemgr->GetPredefName(PN_DELEGATEBIPARAM0);
            info.paramInfos[i++].name = compiler()->namemgr->GetPredefName(PN_DELEGATEBIPARAM1);

            ASSERT(i == method->cParams);

        } else if (method->name == compiler()->namemgr->GetPredefName(PN_ENDINVOKE)) {

            // we need the type of the invoke method:
            METHSYM * invoke = compiler()->symmgr.LookupGlobalSym(compiler()->namemgr->GetPredefName(PN_INVOKE), method->parent, MASK_METHSYM)->asMETHSYM();

            DELEGATENODE * delegateNode = method->getClass()->getParseTree()->asDELEGATE();
            int i = 0;
            int j = 0;
            NODELOOP(delegateNode->pParms, PARAMETER, param)

                if (invoke->params[j]->kind == SK_PARAMMODSYM) {

                    NAME * name = param->pName->pName;

                    info.paramInfos[i].name = name;
                    
                    i += 1;
                }
                j += 1;
            ENDLOOP;

            info.paramInfos[i++].name = compiler()->namemgr->GetPredefName(PN_DELEGATEEIPARAM0);

            ASSERT(i == method->cParams);

        } else if (method->isCtor) {
            // compiler generated delegate constructors don't have 
            // attributes on their parameters ... but they do have names
            info.paramInfos[0].name = compiler()->namemgr->GetPredefName(PN_DELEGATECTORPARAM0);
            info.paramInfos[1].name = compiler()->namemgr->GetPredefName(PN_DELEGATECTORPARAM1);
        }
    } else if (method->isExternal && method->isCtor && aggInfo->isComimport) {

        ASSERT(method->getClass()->isClass && !method->isStatic && method->isSysNative);

        // constructors for COM classic coclasses are magic
        info.isMagicImpl = true;
    }

    int errNow = compiler()->ErrorCount();

    //
    // parse the method body
    //
    ICSInteriorTree *pIntTree = NULL;
    {
        SETLOCATIONSTAGE(INTERIORPARSE);
        if ((tree->InGroup (NG_INTERIOR)) && FAILED (method->getInputFile()->pData->GetInteriorParseTree (tree, &pIntTree)))
            compiler()->Error (ERRLOC(method), FTL_NoMemory);     // The only possible failure
        else if (pIntTree != NULL)
        {
            CComPtr<ICSErrorContainer>  spErrors;

            // See if there were any errors in the interior parse
            if (SUCCEEDED (pIntTree->GetErrors (&spErrors)))
                controller()->ReportErrorsToHost (spErrors);
            else
                compiler()->Error (ERRLOC(method), FTL_NoMemory);  // Again, the only possible failure
        }
    }

    if (errNow != compiler()->ErrorCount()) {
        goto LERROR;
    }


    //
    // emit method info
    //
    compiler()->emitter.EmitMethodInfo(method, &info);

    //
    // get expr tree for method body
    //
    EXPR * body;
    if (!method->getClass()->isDelegate) {
        SETLOCATIONSTAGE(COMPILE);

        METHATTRBIND::Compile(compiler(), method, &info, aggInfo);

        {
            SETLOCATIONSTAGE(BIND);
            body = compiler()->funcBRec.compileMethod(method, tree, &info, aggInfo);
        }
    } else {
        //
        // delegate members are 'magic'. Their implementatino is provided by the VOS.
        //
        body = NULL;
    }

    //
    // get existing param tokens
    //
    if (method->tokenImport) {
        ASSERT(compiler()->IsIncrementalRebuild());
        compiler()->importer.ReadExistingParameterTokens(method, &info);
        compiler()->emitter.EmitParamProp(method->tokenEmit, 0, method->retType, &info.returnValueInfo);
    }

    //
    // emit attributes on parameters
    //
    PARAMATTRBIND::CompileParamList(compiler(), method, &info);

    // generate il only if no errors...
    if (errNow != compiler()->ErrorCount()) goto LERROR;

    if (bSaveDocs && !method->isCompilerGeneratedCtor())
        DocSym(method, method->cParams, info.paramInfos);

    if (!compiler()->options.m_fNOCODEGEN) {
        if (body) {
            ASSERT(!method->isAbstract && !method->isExternal);
            SETLOCATIONSTAGE(CODEGEN);
            compiler()->ilGenRec.compile(method, &info, body);
        } else  {
            ASSERT(method->isAbstract || method->getClass()->isDelegate || method->isExternal);
        }
    }

    if (method->isExternal) {
        BASENODE* tree = method->getAttributesNode();
        if (method->isEventAccessor)
            tree = method->getEvent()->getAttributesNode();
        if (tree == NULL && !info.isMagicImpl) {
            // An extern method, property accessor or event with NO attributes!
            errorSymbol(method, WRN_ExternMethodNoImplementation);
        }
    }

LERROR:

    if (pIntTree)
        pIntTree->Release();

}

void CLSDREC::compileProperty(PROPSYM * property, AGGINFO * aggInfo)
{
    //
    // emit the property
    //
    PROPINFO info;
    memset(&info, 0, sizeof(info));

    PROPATTRBIND::Compile(compiler(), property, &info);

    // Accumulate parameter names for indexed properties, if any.
    if (property->cParams) {
        PARAMINFO *paramInfo = info.paramInfos = (PARAMINFO *) _alloca(sizeof(PARAMINFO) * property->cParams);
        NODELOOP(property->getParseTree()->asANYPROPERTY()->pParms, PARAMETER, param)
            paramInfo->name = param->pName->pName;
            paramInfo++;
        ENDLOOP;
    }

    if (compiler()->CheckForCLS() && property->checkForCLS() && property->hasExternalAccess() && !property->retType->isCLS_Type())
        errorSymbol( property, ERR_CLS_BadFieldPropType);
    
    if (bSaveDocs) {
        DocSym(property, property->cParams, info.paramInfos);
    }
}

void CLSDREC::compileEvent(EVENTSYM * event, AGGINFO * aggInfo)
{
    //
    // emit the event
    //
    EVENTINFO info;
    memset(&info, 0, sizeof(info));

    EVENTATTRBIND::Compile(compiler(), event, &info);

    if (compiler()->CheckForCLS() && event->hasExternalAccess() && !event->type->isCLS_Type())
        errorSymbol( event, ERR_CLS_BadFieldPropType);

    if (bSaveDocs)
        DocSym(event);

}

// bind a dotted type name, or namespace name (if NSok is true)
// symContext gives the symbolic context
SYM * CLSDREC::bindDottedTypeName(BINOPNODE * name, PARENTSYM * symContext, int flags, AGGSYM *classBeingResolved)
{
    SYM * rval;
    ASSERT(name->kind == NK_DOT);
    BINOPNODE *last = name;
    SYM * badAccess = NULL;
    SYM * badKind = NULL;

    // now, find the first name:
    BASENODE *first = name->p1;
    while (first->kind == NK_DOT) first = first->asDOT()->p1;
    // we will use the parent pointer to walk 'forward' in the chain...

    SYM * current;

    // this is a partially qualified name, so bind the name on the left of all
    // the dots:
    if (first->asNAME()->pName == compiler()->namemgr->GetPredefName(PN_EMPTY)) {
        current = compiler()->symmgr.GetRootNS();
    } else {
        current = bindSingleTypeName(first->asNAME(), symContext, (flags | BTF_NSOK) & ~BTF_ATTRIBUTE, classBeingResolved);
        // bSTN already reported the error, if any...
        if (!current) return NULL;
    }

    // so assming we have the left part of the name on which to look:

    NAME * curName;
    for (;;) {
        // loop until we bind all the names before the rightmost one...
        first = first->pParent;
        ASSERT(first->kind == NK_DOT && first->asDOT()->p2->kind == NK_NAME);

        curName = first->asDOT()->p2->asNAME()->pName;
        PARENTSYM *toSearchIn = current->asPARENTSYM();
        do 
        {
            if (toSearchIn->kind == SK_AGGSYM)
            {
                if (!resolveInheritanceRec(toSearchIn->asAGGSYM())) {
                    if (toSearchIn->asAGGSYM()->isResolvingBaseClasses) {
                        ASSERT(classBeingResolved);
                        errorSymbolAndRelatedSymbol(toSearchIn, classBeingResolved, ERR_CircularBase);
                    }
                }
            }
            // at this point we will take either a class, or a namespace...
            rval = lookupIfAccessOk(curName, toSearchIn, MASK_NSSYM | MASK_AGGSYM, symContext, !(flags & BTF_NODECLARE), &badAccess, &badKind);
            if (!rval && (toSearchIn->kind == SK_AGGSYM)) {
                toSearchIn = toSearchIn->asAGGSYM()->baseClass;
            } else {
                toSearchIn = NULL;
            }
        } while (!rval && toSearchIn);
        if (first == last || !rval) break;
        current = rval;
        // is this the rightmost name?
    }

    if ((flags & BTF_ATTRIBUTE) && first == last &&
         (!rval || rval->kind != SK_AGGSYM || !isAttributeClass(rval->asAGGSYM())))
    {
        //
        // we were looking for the last name but failed
        // try again with the "attribute" suffix
        //
        WCHAR nameBuffer[MAX_IDENT_SIZE];
        wcscpy(nameBuffer, curName->text);
        wcscat(nameBuffer, L"Attribute");
        NAME *attribIdent;
        compiler()->namemgr->Lookup(nameBuffer, &attribIdent);
        if (attribIdent) {
            PARENTSYM *toSearchIn = current->asPARENTSYM();
            do 
            {
                rval = lookupIfAccessOk(attribIdent, current->asPARENTSYM(), MASK_NSSYM | MASK_AGGSYM, symContext, !(flags & BTF_NODECLARE), &badAccess, &badKind);
                if (!rval && (toSearchIn->kind == SK_AGGSYM)) {
                    toSearchIn = toSearchIn->asAGGSYM()->baseClass;
                } else {
                    toSearchIn = NULL;
                }
            } while (!rval && toSearchIn);
        }
    }

    if (rval) {
        if (!(flags & BTF_NSOK) && rval->kind == SK_NSSYM) {
            if (! (flags & BTF_NOERRORS))
                errorStrStrStrFile(name, symContext->getInputFile(), ERR_BadSKknown, compiler()->ErrSym(rval), compiler()->ErrSK(rval->kind), compiler()->ErrSK(SK_AGGSYM));
            return NULL;
        }

        CheckSymbol(name, rval, symContext, flags);

        return rval;
    }

    if (badAccess) {
        if (! (flags & BTF_NOERRORS))
            errorStrFile(name, symContext->getInputFile(), ERR_BadAccess, compiler()->ErrSym(badAccess));
    } else if (badKind) {
        // Found a symbol of the wrong kind.
        if (! (flags & BTF_NOERRORS))
            errorStrStrStrFile(name, symContext->getInputFile(), ERR_BadSKknown, compiler()->ErrSym(badKind), compiler()->ErrSK(badKind->kind), compiler()->ErrSK(SK_AGGSYM));
    } else {
        if (! (flags & BTF_NOERRORS))
            errorNameRefStr(first->asDOT()->p2, symContext, compiler()->ErrSym(current), ERR_TypeNameNotFound);
    }

    return NULL;

}

// Shared by bindSingleTypeName and bindDottedTypeName
// Optionally declares the rval and reports isBogus, hasMultipleDefs, and isDeprecated
void CLSDREC::CheckSymbol(BASENODE * name, SYM * rval, PARENTSYM * symContext, int flags)
{
    if (!rval) return;

    if (!(flags & BTF_NODECLARE)) {
        compiler()->symmgr.DeclareType(rval);
    }
    if (rval->isBogus && !(flags & BTF_NOERRORS)) {
        errorFileSymbol(name, symContext->getInputFile(), ERR_BogusType, rval);
    }
    if (rval->kind == SK_AGGSYM && !(flags & BTF_NOERRORS) && rval->asAGGSYM()->hasMultipleDefs) {
        errorStrStrFile(name, symContext->getInputFile(), WRN_MultipleTypeDefs, compiler()->ErrSym(rval), rval->getInputFile()->name->text);
    }
    if (rval->isDeprecated && !(flags & (BTF_NODEPRECATED|BTF_NOERRORS))) {
        reportDeprecated(name, symContext, rval);
    }
    if (rval->kind == SK_AGGSYM && rval->asAGGSYM()->isPredefType(PT_SYSTEMVOID)) {
        errorFile(name, symContext->getInputFile(), ERR_SystemVoid);
    }
}

// Searches for a symbol matching the given name
// Assumes that name is fully qualified (i.e. namespace.namespace.type.field)
// Does not report any errors, returns NULL if not found
SYM * CLSDREC::findSymName(LPCWSTR fullyQualifiedName, size_t cchName)
{
    WCHAR * p;
    WCHAR * start;
    PNAME name;
    WCHAR ch;
    PARENTSYM *ns;

    if (cchName == 0)
        cchName = wcslen(fullyQualifiedName);
    ns = compiler()->symmgr.GetRootNS();  // start at the root namespace.
    start = p = (WCHAR*)fullyQualifiedName;

    while (ns != NULL) {
        // Advance p to the end of the next segment.
        ch = *p;
        while (p < fullyQualifiedName + cchName && ch != L'/' && ch != L'.') {
            ++p;
            ch = *p;
        }

        // Find/Create a sub-namespace with this name.
        if (p != start) {
            name = compiler()->namemgr->LookupString(start, (int)(p - start));
            if (!name) {
                return NULL;
            } else {

                // Find an existing namespace or class
                SYM *NewSym;
                NewSym = compiler()->symmgr.LookupGlobalSym(name, ns, MASK_ALL);
                if (p >= fullyQualifiedName + cchName)
                    return NewSym; // We've reached the end this must be it (even if it is NULL)
                else if (NewSym && (NewSym->kind == SK_AGGSYM || NewSym->kind == SK_NSSYM ||
                       NewSym->kind == SK_ERRORSYM   || NewSym->kind == SK_SCOPESYM ||
                       NewSym->kind == SK_OUTFILESYM || NewSym->kind == SK_ARRAYSYM ||
                       NewSym->kind == SK_NSDECLSYM))
                    ns = NewSym->asPARENTSYM(); // Found a parent that matches, keep searching
                else
                    // We didn't find a PARENTSYM, and we weren't at the end of the
                    // the name, so it obviously doesn't exist (only PARENTSYMs can have children)
                    return NULL;
            }
        }

        // Continue to the next segment.
        ++p;
        start = p;
    }

    return NULL;
}


// searches for a name in a parent and checks access on it
// never raises an error message
// If declareIfNeeded is true, may declare parent if its an aggregate.
SYM * CLSDREC::lookupIfAccessOk(NAME * name, PARENTSYM * parent, int mask, PARENTSYM * current, bool declareIfNeeded, SYM ** badAccess, SYM ** badKind)
{
    SYM * rval = compiler()->symmgr.LookupGlobalSym(name, parent, mask);
    if (rval) {
        if (checkAccess(rval, current, badAccess, current)) 
            return rval;
        else
            return NULL;
    }
    else { 
        if (parent->kind == SK_AGGSYM && declareIfNeeded)
            compiler()->symmgr.DeclareType(parent);

        SYM * badSymKind = compiler()->symmgr.LookupGlobalSym(name, parent, MASK_ALL);
        if (badSymKind)
            *badKind = badSymKind;
        return NULL;
    }
}

//
// Searches a namespace for a name
// searches this namespace, using cluases & parent namespaces
//
// returns false on ambiguity error
// returns true and sets returnValue if found
// returns true and sets returnValue to NULL if not found
//
// nsDeclaration is the namespace declaration to search in
// context is the symbolic context being searched from
//
bool CLSDREC::searchNamespace(
    NAME *          name,
    BASENODE *      node,
    NSDECLSYM *     nsDeclaration,
    PARENTSYM *     context,
    SYM **          returnValue,
    SYM **          badAccess, 
    SYM **          badKind, 
    bool            fUsingAlias,
    AGGSYM *        classBeingResolved
    )
{
    do {
        //
        // check the namespace proper
        //
        *returnValue = lookupIfAccessOk(name, nsDeclaration->namespaceSymbol, MASK_AGGSYM | MASK_NSSYM, context, false, badAccess, badKind);
        if (*returnValue) {
            return true;
        }

        //
        // check the using clauses for this declaration
        //
        if (!fUsingAlias && !searchUsingClauses(name, node, nsDeclaration, context, returnValue, badAccess, badKind, classBeingResolved)) {
            // found an ambiguity, stop search
            return false;
        }
        if (*returnValue) {
            return true;
        }

        nsDeclaration = nsDeclaration->containingDeclaration();
        fUsingAlias = false;
    } while (nsDeclaration);

    // not found, no error
    return true;
}


//
// Searches the using clauses of a namespace for a name
//
// returns false on ambiguity error
// returns true and sets returnValue if found
// returns true and sets returnValue to NULL if not found
//
// nsDeclaration is the namespace declaration to search in 
// context is the symbolic context being searched from
//
bool CLSDREC::bindUsingAlias(ALIASSYM *alias, SYM **returnValue, SYM **badAccess, AGGSYM *classBeingResolved)
{
    if (!alias->boundType) {
        alias->boundType = (PARENTSYM*) bindTypeName(alias->parseTree->asUSING()->pName, alias->parent, BTF_NSOK | BTF_NODECLARE | BTF_USINGALIAS, classBeingResolved);
        if (!alias->boundType) {
            return false;
        }
    }

    // check that we have access to the element
    ASSERT(checkAccess(alias->boundType, alias->parent, badAccess, alias->parent));
    *returnValue = alias->boundType;
    return true;
}

//
// Searches the using clauses of a namespace for a name
//
// returns false on ambiguity error
// returns true and sets returnValue if found
// returns true and sets returnValue to NULL if not found
//
// nsDeclaration is the namespace declaration to search in 
// context is the symbolic context being searched from
//
bool CLSDREC::searchUsingClauses(
    NAME *          name, 
    BASENODE *      node, 
    NSDECLSYM *     nsDeclaration, 
    PARENTSYM *     context, 
    SYM **          returnValue,
    SYM **          badAccess,
    SYM **          badKind,
    AGGSYM *        classBeingResolved
    )
{
    //
    // ensure our using clauses have been dealt with
    // This handles the case of a class in another namespace
    // derived from a class in this namespace which is 
    // defined before this namespace.
    //
    ensureUsingClausesAreResolved(nsDeclaration, classBeingResolved);

    *returnValue = NULL;

    //
    // First check explicit using types:
    //
    FOREACHSYMLIST(nsDeclaration->usingClauses, usingSym)
        if (usingSym->kind == SK_ALIASSYM) {
            if (usingSym->name == name) {
                return bindUsingAlias(usingSym->asALIASSYM(), returnValue, badAccess, classBeingResolved);
            }
        }
    ENDFOREACHSYMLIST

    //
    // didn't find a using type
    // then check for using namespace...
    //
    FOREACHSYMLIST(nsDeclaration->usingClauses, usingSym)
        if (usingSym->kind == SK_NSSYM) {
            // don't want caller to report bad kind error
            // for finding namespaces througha using namespace clause
            SYM * tempBadKind = 0;
            SYM * rvalCurrent = lookupIfAccessOk(name, usingSym->asNSSYM(), MASK_AGGSYM, context, false, badAccess, &tempBadKind);
            if (rvalCurrent) {
                if (*returnValue) {
                    errorNameRefSymbolSymbol(name, node, context, rvalCurrent, *returnValue, ERR_AmbigContext);
                    *returnValue = NULL;
                    return false;
                } else {
                    *returnValue = rvalCurrent;
                }
            }
        }
    ENDFOREACHSYMLIST

    // *returnValue is already set
    return true;
}

//
// Searches a class for a name
// searches this class, base classes, and nested classes
//
// returns false on error
// returns true and sets returnValue if found
// returns true and sets returnValue to NULL if not found
//
// badAccess and badKind return symbols that matched by name but had incorrect access or symbol kind.
//
// context is the symbolic context being searched from
//
bool CLSDREC::searchClass(
    NAME *          name, 
    AGGSYM *        classToSearchIn,
    PARENTSYM *     context, 
    SYM **          returnValue,
    bool            declareIfNeeded, 
    SYM **          badAccess,  
    SYM **          badKind
    )
{
    *returnValue = NULL;

    //
    // check this class and all classes that this class
    // are nested in
    //
    do {
        //
        // check this class and all base classes
        //
        AGGSYM * cls = classToSearchIn;
        do {
            //
            // check this class
            //
            ASSERT(cls->hasResolvedBaseClasses);
            *returnValue = lookupIfAccessOk(name, cls, MASK_AGGSYM | MASK_NSSYM, context, declareIfNeeded, badAccess, badKind);
            if (*returnValue) {
                return true;
            }

            ASSERT(cls->baseClass || cls == compiler()->symmgr.GetObject() || cls->isInterface);
            cls = cls->baseClass;
        } while (cls);

        //
        // is our parent a class ...
        //
        if (classToSearchIn->parent->kind == SK_AGGSYM) {
            //
            // we are a nested class, check our parent's class
            //
            classToSearchIn = classToSearchIn->parent->asAGGSYM();
        } else {
            break;
        }
    } while (1);

    return true;
}

/*
 * Searches a class for a method with the same name and signature.
 * Does not search base classes.
 */
METHSYM *CLSDREC::findSameSignature(METHSYM *method, AGGSYM *classToSearchIn)
{
    SYM *symbol = compiler()->symmgr.LookupGlobalSym(method->name, classToSearchIn, MASK_ALL);
    while (symbol && (symbol->kind != SK_METHSYM || symbol->asMETHSYM()->params != method->params)) {
        symbol = compiler()->symmgr.LookupNextSym(symbol, symbol->parent, MASK_ALL);
    }
    return symbol->asMETHSYM();
}

/*
 * Searches a class for a method with the same name and signature.
 * Does not search base classes.
 */
PROPSYM *CLSDREC::findSameSignature(PROPSYM *prop, AGGSYM *classToSearchIn)
{
    SYM *symbol = compiler()->symmgr.LookupGlobalSym(prop->name, classToSearchIn, MASK_ALL);
    while (symbol && (symbol->kind != SK_PROPSYM || symbol->asPROPSYM()->params != prop->params)) {
        symbol = compiler()->symmgr.LookupNextSym(symbol, symbol->parent, MASK_ALL);
    }
    return symbol->asPROPSYM();
}

/*
 * Searches for an accessible name in a class and its base classes
 *
 * name - the name to search for
 * classToSearchIn - the classToSearchIn
 * context - the class we are searching from
 * current - if specified, then the search looks for members in the class
 *           with the same name after current in the symbol table
 *           if specified current->parent  must equal classToSearchIn
 *
 * This method can be used to iterate over all accessible base members
 * of a given name, by updating current with the previous return value
 * and classToSearchIn with current->parent
 *
 * This method never reports any errors
 *
 * Returns NULL if there are no remaining members in a base class
 * with the given name.
 *
 */
SYM *CLSDREC::findNextAccessibleName(NAME *name, AGGSYM *classToSearchIn, PARENTSYM *context, SYM *current, bool bAllowAllProtected, bool ignoreSpecialMethods)
{
    PAGGSYM qualifier = bAllowAllProtected ? NULL : classToSearchIn;
    while ((current = findNextName(name, classToSearchIn, current)) && 
        (!checkAccess(current, context, NULL,qualifier) || (ignoreSpecialMethods && !current->isUserCallable()))) {
        classToSearchIn = current->parent->asAGGSYM();
    }

    return current;
}

/*
 * Searches for a name in a class and its base classes
 *
 * name - the name to search for
 * classToSearchIn - the classToSearchIn
 * current - if specified, then the search looks for members in the class
 *           with the same name after current in the symbol table
 *           if specified current->parent  must equal classToSearchIn
 *
 * This method can be used to iterate over all base members
 * of a given name, by updating current with the previous return value
 * and classToSearchIn with current->parent
 *
 * This method never reports any errors
 *
 * Returns NULL if there are no remaining members in a base class
 * with the given name.
 *
 */
SYM *CLSDREC::findNextName(NAME *name, AGGSYM *classToSearchIn, SYM *current)
{
    //
    // check for next in same class
    //
    if (current) {
        do {
            ASSERT(current->parent == classToSearchIn);
            current = compiler()->symmgr.LookupNextSym(current, current->parent, MASK_ALL);
            if (current) {
                return current;
            }
        } while (current);

        //
        // didn't find any more in this class
        // start with the base class
        //
        classToSearchIn = classToSearchIn->baseClass;
    }

    //
    // check base class
    //
    while (classToSearchIn) {
        SYM * hiddenSymbol = NULL;
        compiler()->EnsureTypeIsDefined(classToSearchIn);
        hiddenSymbol = compiler()->symmgr.LookupGlobalSym(name, classToSearchIn, MASK_ALL);
        if (hiddenSymbol) {
            return hiddenSymbol;
        }

        classToSearchIn = classToSearchIn->baseClass;
    }

    return NULL;
}

//
// find an inherited member which is hidden by name, kind and params
//  name    - name to find hidden member
//  symkind - kind of member. SK_COUNT for any member
//  params  - signature to match, only valid for symkind SK_METHSYM || SK_PROPSYM
//  classToSearchIn - class to start the search in
//  context - context to search from for access checks
//
SYM *CLSDREC::findHiddenSymbol(NAME *name, AGGSYM *classToSearchIn, AGGSYM *context)
{
    return findHiddenSymbol(name, SK_COUNT, NULL, classToSearchIn, context);
}

//
// find an inherited member which is hidden by name, kind and params
//  name    - name to find hidden member
//  symkind - kind of member. SK_COUNT for any member
//  params  - signature to match, only valid for symkind SK_METHSYM || SK_PROPSYM
//  classToSearchIn - class to start the search in
//  context - context to search from for access checks
//
SYM *CLSDREC::findHiddenSymbol(NAME *name, SYMKIND symkind, PTYPESYM *params, AGGSYM *classToSearchIn, AGGSYM *context)
{
    SYM *hiddenSymbol;
    for (hiddenSymbol = NULL;
         (hiddenSymbol = findNextAccessibleName(name, classToSearchIn, context, hiddenSymbol, true, false));
         /* nothing */) {

        if ((hiddenSymbol->kind != symkind) || 
            (hiddenSymbol->asMETHPROPSYM()->params == params)) {
            break;
        }

        classToSearchIn = hiddenSymbol->parent->asAGGSYM();
    }

    return hiddenSymbol;
}

//
// find an inherited member which is hidden by name, kind and params
//  name    - name to find hidden member
//  symkind - kind of member. SK_COUNT for any member
//  params  - signature to match, only valid for symkind SK_METHSYM || SK_PROPSYM
//  classToSearchIn - class to start the search in
//
SYM *CLSDREC::findAnyAccessHiddenSymbol(NAME *name, SYMKIND symkind, PTYPESYM *params, AGGSYM *classToSearchIn)
{
    SYM *hiddenSymbol;
    for (hiddenSymbol = NULL;
         (hiddenSymbol = findNextName(name, classToSearchIn, hiddenSymbol));
         /* nothing */) {

        if ((hiddenSymbol->kind != symkind) || 
            (hiddenSymbol->asMETHPROPSYM()->params == params)) {
            break;
        }

        classToSearchIn = hiddenSymbol->parent->asAGGSYM();
    }

    return hiddenSymbol;
}

// bind a single (not dotted) type name, or namespace name (if NSok is true)
// nmTree and current give the lexical and symbolic contexts...
// If current is not specified, we start off with the namespace given by nsDecl
SYM * CLSDREC::bindSingleTypeName(NAMENODE * name, PARENTSYM * symContext, int flags, AGGSYM *classBeingResolved)
{

    SYM * rval = NULL;
    NAME * ident = name->pName;
    SYM * badAccess = NULL;
    SYM * badKind = NULL;
    NSDECLSYM *nsDeclaration = NULL;
    NAME *attribIdent = NULL;

    //
    // used for Attribute suffix check
    //
TRYAGAIN:

    //
    // if we want to check classes, and we have a class to look in
    //        
    if (!(flags & BTF_STARTATNS) && (symContext->kind != SK_NSDECLSYM)) {
        //
        // check this class and all classes that this class
        // are nested in
        //
        ASSERT(symContext->asAGGSYM()->hasResolvedBaseClasses);
        if (!searchClass(ident, symContext->asAGGSYM(), symContext->asAGGSYM(), &rval, !(flags & BTF_NODECLARE), &badAccess, &badKind)) {
            //
            // got an error in searchClass(resolving base classes)
            // just bail ...
            //
            return NULL;
        }
    }

    //
    // search namespace if we haven't found it in a class
    //
    if (!rval) {
        //
        // find the enclosing namespace declaration
        //
        PARENTSYM *parent = symContext;
        while (parent->kind != SK_NSDECLSYM) {
            parent = parent->containingDeclaration();
        }
        nsDeclaration = parent->asNSDECLSYM();

        if (!searchNamespace(ident, name, nsDeclaration, symContext, &rval, &badAccess, &badKind, !!(flags & BTF_USINGALIAS), classBeingResolved)) {
            // ambiguity error
            return NULL;
        }
    }


    // If we're searching for an attribute classes, and we didn't succeed (not found or not an attribute class) with the base
    // name, append "Attribute" and try again.
    if ((flags & BTF_ATTRIBUTE) && !attribIdent &&
        (!rval || rval->kind != SK_AGGSYM || !isAttributeClass(rval->asAGGSYM())))
    {
        //
        // look for Attribute suffix on type name
        //
        WCHAR nameBuffer[MAX_IDENT_SIZE];
        wcscpy(nameBuffer, ident->text);
        wcscat(nameBuffer, L"Attribute");
        compiler()->namemgr->Lookup(nameBuffer, &attribIdent);
        if (attribIdent) {
            rval = NULL;
            ident = attribIdent;
            goto TRYAGAIN;
        }
    }

    //
    // report the appropriate errors
    //
    if (!rval) {
        //
        // restore ident to non-attrib version for error reporting
        //
        ident = name->pName;

        if (!badAccess && (flags & BTF_ANYBADACCESS) && symContext->kind == SK_AGGSYM) {
            // didn't find any names at all
            // try and find inaccessible name of any SK
            badAccess = findNextName(ident, symContext->asAGGSYM(), NULL);
        }
        if (badAccess) {
            // found an inaccessible name or an uncallable name
            if (!badAccess->isUserCallable()) {
                if (! (flags & BTF_NOERRORS))
                    errorNameRefSymbol(name, symContext, badAccess, ERR_CantCallSpecialMethod);
            } else {
                if (! (flags & BTF_NOERRORS))
                    errorStrFile(name, symContext->getInputFile(), ERR_BadAccess, compiler()->ErrSym(badAccess));
            }
        } 
        else if (badKind) {
            // Found a symbol of the wrong kind.
            if (! (flags & BTF_NOERRORS))
                errorStrStrStrFile(name, symContext->getInputFile(), ERR_BadSKknown, compiler()->ErrSym(badKind), compiler()->ErrSK(badKind->kind), compiler()->ErrSK(SK_AGGSYM));
        }
        else {
            // didn't find any names at all
            PARENTSYM *errorContext;
            if (flags & BTF_STARTATNS) {
                // we were looking in the namespace
                errorContext = nsDeclaration->namespaceSymbol;
            } else {
                // we were looking in the class, or if no class
                // was provided symContext == the namespace anyways
                errorContext = symContext->getScope();
            }
            if (! (flags & BTF_NOERRORS))
                errorNameRefStr(name, symContext, compiler()->ErrSym(errorContext), ERR_SingleTypeNameNotFound);
        }
    } else if (!(flags & BTF_NSOK) && rval->kind != SK_AGGSYM) {
        // found a namespace but was looking for a class
        if (badAccess && (badAccess->kind == SK_AGGSYM)) {
            // could have found a type, but it was inaccessible
            if (! (flags & BTF_NOERRORS))
                errorStrFile(name, symContext->getInputFile(), ERR_BadAccess, compiler()->ErrSym(badAccess));
        } else {
            if (! (flags & BTF_NOERRORS))
                errorStrStrStrFile(name, symContext->getInputFile(), ERR_BadSKknown, compiler()->ErrSym(rval), compiler()->ErrSK(rval->kind), compiler()->ErrSK(SK_AGGSYM));
        }
        rval = NULL;
    }

    if (rval) {
        CheckSymbol(name, rval, symContext, flags);
    }

    return rval;
}

// Bind a type given a parstree node, and a textual and symbolic context...
TYPESYM * CLSDREC::bindType(TYPENODE * type, PARENTSYM* symContext, int flags, AGGSYM *classBeingResolved)
{
    switch (type->TypeKind()) {
    case TK_PREDEFINED:
        if (type->iType == PT_VOID) return compiler()->symmgr.GetVoid();

        return compiler()->symmgr.GetPredefType((PREDEFTYPE)type->iType, false);
    case TK_ARRAY: {
        TYPESYM * elemType = bindType(type->pElementType, symContext, flags, classBeingResolved);
        // This is a bit of a mess, we need to synchronize better with the parser...
        if (!elemType) {
            return NULL;
        }
        if (elemType->isSpecialByRefType()) {
            if (! (flags & BTF_NOERRORS))
                errorStrFile(type, symContext->getInputFile(), ERR_ArrayElementCantBeRefAny, compiler()->ErrSym(elemType));
            return NULL;
        }
        int rank = type->iDims;
        ASSERT(rank > 0);
        return compiler()->symmgr.GetArray(elemType, rank);
                        }
    case TK_NAMED:
        return bindTypeName(type->pName, symContext, flags, classBeingResolved)->asAGGSYM();
    case TK_POINTER: {
        TYPESYM * innerType = bindType(type->pElementType, symContext, flags, classBeingResolved);
        if (!innerType) return NULL;
        return compiler()->symmgr.GetPtrType(innerType);
                          }
    default:
        ASSERT(!"BAD type flag");
        return NULL;
    }
}


// Report a deprecation error on a symbol.
void CLSDREC::reportDeprecated(BASENODE * tree, PSYM refContext, SYM * sym)
{
    ASSERT(sym->isDeprecated);

    if (refContext->isContainedInDeprecated())
        return;

    if (sym->deprecatedMessage) {
        // A message is associated with this deprecated symbol: use that.
        if (sym->isDeprecatedError)
            errorStrStrFile(tree, refContext->getInputFile(), ERR_DeprecatedSymbolStr, compiler()->ErrSym(sym), sym->deprecatedMessage);
        else
            errorStrStrFile(tree, refContext->getInputFile(), WRN_DeprecatedSymbolStr, compiler()->ErrSym(sym), sym->deprecatedMessage);
    }
    else {
        // No message
        errorStrFile(tree, refContext->getInputFile(), WRN_DeprecatedSymbol, compiler()->ErrSym(sym));
    }
}

// reports deprecation errors on the use of a type. Also checkes the isBogus flag.
void CLSDREC::reportDeprecatedType(BASENODE * tree, PSYM refContext, TYPESYM * type)
{
    AGGSYM *baseType = type->underlyingAggregate();
    if (baseType) {
        compiler()->EnsureTypeIsDefined(baseType);
        if (baseType->isBogus) {
            errorFileSymbol(tree, refContext->getInputFile(), ERR_BogusType, baseType);
        }
        else if (baseType->isDeprecated) {

            // don't report deprecated warning if any of our containing classes are deprecated
            if (!refContext->isContainedInDeprecated()) {
                reportDeprecated(tree, refContext, baseType);
            }
        }
    }
}

// reports deprecation errors on the types used in the signature of a method or property
void CLSDREC::reportDeprecatedMethProp(BASENODE *tree, METHPROPSYM *methprop)
{
    reportDeprecatedType(methprop->parseTree, methprop, methprop->retType);
    for (int i = 0; i < methprop->cParams; i += 1) {
        reportDeprecatedType(methprop->parseTree, methprop, methprop->params[i]);
    }
}

// Check to see if sym1 is "at least as visible" as sym2.
bool CLSDREC::isAtLeastAsVisibleAs(SYM * sym1, SYM * sym2)
{
    SYM * s1parent, * s2parent;

    ASSERT(sym2->kind != SK_ARRAYSYM && sym2->kind != SK_PTRSYM && sym2->kind != SK_PARAMMODSYM);

    // If sym1 is a pointer, array, or byref type, convert to underlying type. 
    for (;;) {
        if (sym1->kind == SK_ARRAYSYM)
            sym1 = sym1->asARRAYSYM()->elementType();
        else if (sym1->kind == SK_PTRSYM)
            sym1 = sym1->asPTRSYM()->baseType();
        else if (sym1->kind == SK_PARAMMODSYM)
            sym1 = sym1->asPARAMMODSYM()->paramType();
        else if (sym1->kind == SK_VOIDSYM)
            return true; // void is completely visible.
        else
            break;
    }

    // quick check -- everything is at least as visible as private 
    if (sym2->access == ACC_PRIVATE)
        return true;

    // Algorithm: The only way that sym1 is NOT at least as visible as sym2
    // is if it has a access restriction that sym2 does not have. So, we simply
    // go up the parent chain on sym1. For each access modifier found, we check
    // to see that the same access modifier, or a more restrictive one, is somewhere
    // is sym2's parent chain.

    for (SYM * s1 = sym1; s1->kind != SK_NSSYM; s1 = s1->parent)
    {
        ACCESS acc1 = s1->access;

        if (acc1 != ACC_PUBLIC) {
            bool asRestrictive = false;

            for (SYM * s2 = sym2; s2->kind != SK_NSSYM; s2 = s2->parent)
            {
                ACCESS acc2 = s2->access;

                switch (acc1) {
                case ACC_INTERNAL:
                    // If s2 is private or internal, and within the same assembly as s1,
                    // then this is at least as restrictive as s1's internal. 
                    if (acc2 == ACC_PRIVATE || acc2 == ACC_INTERNAL) {
                        if (s2->getAssemblyIndex() == s1->getAssemblyIndex())
                            asRestrictive = true;
                    }
                    break;

                case ACC_PROTECTED:
                    s1parent = s1->parent;

                    if (acc2 == ACC_PRIVATE) {
                        // if s2 is private and within s1's parent or within a subclass of s1's parent,
                        // then this is at least as restrictive as s1's protected. 
                        for (s2parent = s2->parent; s2parent->kind != SK_NSSYM; s2parent = s2parent->parent)
                            if (compiler()->symmgr.IsBaseType(s2parent->asAGGSYM(), s1parent->asAGGSYM()))
                                asRestrictive = true;
                    }
                    else if (acc2 == ACC_PROTECTED) {
                        // if s2 is protected, and it's parent is a subclass (or the same as) s1's parent
                        // then this is at least as restrictive as s1's protected
                        if (compiler()->symmgr.IsBaseType(s2->parent->asAGGSYM(), s1parent->asAGGSYM()))
                            asRestrictive = true;
                    }
                    break;
                        
                case ACC_INTERNALPROTECTED:
                    s1parent = s1->parent;

                    if (acc2 == ACC_PRIVATE) {
                        // if s2 is private and within a subclass of s1's parent,
                        // or withing the same assembly as s1
                        // then this is at least as restrictive as s1's internal protected. 
                        if (s2->getAssemblyIndex() == s1->getAssemblyIndex())
                            asRestrictive = true;
                        else {
                            for (s2parent = s2->parent; s2parent->kind != SK_NSSYM; s2parent = s2parent->parent)
                                if (compiler()->symmgr.IsBaseType(s2parent->asAGGSYM(), s1parent->asAGGSYM()))
                                    asRestrictive = true;
                        }
                    }
                    else if (acc2 == ACC_INTERNAL) {
                        // If s2 is in the same assembly as s1, than this is more restrictive
                        // than s1's internal protected.
                        if (s2->getAssemblyIndex() == s1->getAssemblyIndex())
                            asRestrictive = true;
                    }
                    else if (acc2 == ACC_PROTECTED) {
                        // if s2 is protected, and it's parent is a subclass (or the same as) s1's parent
                        // then this is at least as restrictive as s1's internal protected
                        if (compiler()->symmgr.IsBaseType(s2->parent->asAGGSYM(), s1parent->asAGGSYM()))
                            asRestrictive = true;
                    }
                    else if (acc2 == ACC_INTERNALPROTECTED) {
                        // if s2 is internal protected, and it's parent is a subclass (or the same as) s1's parent
                        // and its in the same assembly and s1, then this is at least as restrictive as s1's protected
                        if (s2->getAssemblyIndex() == s1->getAssemblyIndex() &&
                            compiler()->symmgr.IsBaseType(s2->parent->asAGGSYM(), s1parent->asAGGSYM()))
                            asRestrictive = true;
                    }
                    break;


                case ACC_PRIVATE:
                    if (acc2 == ACC_PRIVATE) {
                        // if s2 is private, and it is withing s1's parent, then this is at
                        // least as restrictive of s1's private.
                        s1parent = s1->parent;
                        for (s2parent = s2->parent; s2parent->kind != SK_NSSYM; s2parent = s2parent->parent)
                            if (s2parent == s1parent)
                                asRestrictive = true;
                    }
                    break;

                default:
                    ASSERT(0);
                    break;
                }

            }

            if (! asRestrictive)
                return false;  // no modifier on sym2 was as restrictive as s1
        }
    }

    return true;
}

// Check a symbol and a constituent symbol to make sure the constituent is at least 
// as visible as the main symbol. If not, report an error of the given code.
void CLSDREC::checkConstituentVisibility(SYM * main, SYM * constituent, int errCode)
{
    if (! isAtLeastAsVisibleAs(constituent, main)) {
        errorSymbolAndRelatedSymbol(main, constituent, errCode);
    }
}

void CLSDREC::DocSym(SYM *sym, int cParam, PARAMINFO * params)
{
    // Nothing to do on rotor
}

// Copies pszNew into the string at pInsert, doubling the length of the string
// if needed
LPWSTR CLSDREC::AddToString(LPWSTR *ppStart, LPWSTR pInsert, LPWSTR *ppEnd, LPCWSTR pszNew)
{
    ASSERT(pszNew);
    ASSERT(*ppEnd >= pInsert && pInsert >= *ppStart);
    size_t len = wcslen(pszNew);

    while (pInsert + len > *ppEnd) {
        // double the length
        size_t OldLen = *ppEnd - *ppStart;
        size_t Offset = pInsert - *ppStart;
        *ppStart = (LPWSTR)compiler()->globalHeap.Realloc(*ppStart, sizeof(WCHAR)* 2 * OldLen); 
        *ppEnd = *ppStart + OldLen * 2;
        pInsert = *ppStart + Offset;
    }

    wcscpy(pInsert, pszNew);

    pInsert += len;
    *pInsert = L'\0'; // Make sure verything is NULL terminated

    return pInsert;
}

// Write the given Unicode text to the file as
// a UTF8 string.  This uses stack storage for the UTF8
// version of the text.  Returns the result of WriteFile()
/* static */ HRESULT CLSDREC::WriteFileAsUTF8(HANDLE hFile, LPCWSTR pszText)
{
    PSTR pUTF8 = NULL;
    int cbUTF8, cchUnicode = (int)wcslen(pszText);
    int temp = 0;
    DWORD dwWritten = 0;

    cbUTF8 = UTF8LengthOfUnicode( pszText, cchUnicode);
    ASSERT(cbUTF8 > 0);
    pUTF8 = (PSTR)_alloca(cbUTF8);

    temp = UnicodeToUTF8(pszText, &cchUnicode, pUTF8, cbUTF8);
    ASSERT(temp == cbUTF8);

    if (! WriteFile( hFile, pUTF8, cbUTF8, &dwWritten, NULL))
        return HRESULT_FROM_WIN32(GetLastError());
    else
        return NOERROR;
}

