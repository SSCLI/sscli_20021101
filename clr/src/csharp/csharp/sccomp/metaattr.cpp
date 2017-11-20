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
// File: metaattr.cpp
//
// Routines for converting the attribute information of an item
// ===========================================================================

#include "stdafx.h"

#include "compiler.h"

//-----------------------------------------------------------------------------

// Return the predefined void type
inline TYPESYM * FUNCBREC::getVoidType()
{
    return compiler()->symmgr.GetVoid();
}


// return the given predefined type (including void)
inline TYPESYM * FUNCBREC::getPDT(PREDEFTYPE tp)
{
    if (tp == PT_VOID) return compiler()->symmgr.GetVoid();
    return compiler()->symmgr.GetPredefType(tp, true);
}

EXPR * FUNCBREC::bindAttributeValue(PARENTSYM *parent, BASENODE * tree)
{
    CHECKEDCONTEXT checked(this, compiler()->GetCheckedMode());
    this->checked.constant = true;

    EXPR * val = bindExpr(tree);

    checked.restore(this);

    if (!val->isOK()) return NULL;

    return val;
}

EXPR * FUNCBREC::bindNamedAttributeValue(PARENTSYM *parent, AGGSYM *attributeClass, ATTRNODE * attr)
{
    //
    // bind name to public non-readonly field or property
    //
    NAME * name = attr->pName->asNAME()->pName;
    SYM * sym = NULL;
    SYM * badSym = NULL;
    MEMBVARSYM * field = NULL;
    PROPSYM * property = NULL;
    TYPESYM * type = NULL;
    AGGSYM *classToSearch = attributeClass;
    while (NULL != (sym = compiler()->clsDeclRec.findNextAccessibleName(name, classToSearch, pParent, sym, false, true))) {
        if (sym->kind == SK_MEMBVARSYM) {
            if (sym->asMEMBVARSYM()->isReadOnly || sym->asMEMBVARSYM()->isStatic || sym->asMEMBVARSYM()->isConst) {
                if (!badSym) {
                    badSym = sym;
                }
            } else {
                if (!compiler()->clsDeclRec.isAttributeType(sym->asMEMBVARSYM()->type)) {
                    if (!badSym) {
                        badSym = sym;
                    }
                } else {
                    field = sym->asMEMBVARSYM();
                    type = field->type;
                    break;
                }
            }
        } else if (sym->kind == SK_PROPSYM) {
            PROPSYM *propertyToCheck = sym->asPROPSYM();
            if (!propertyToCheck->methSet && !(propertyToCheck->parseTree && propertyToCheck->parseTree->asANYPROPERTY()->pSet)) {
                if (!badSym) {
                    badSym = sym;
                }
            } else {
                if (!compiler()->clsDeclRec.isAttributeType(sym->asPROPSYM()->retType)) {
                    if (!badSym) {
                        badSym = sym;
                    }
                } else {
                    property = sym->asPROPSYM();
                    type = property->retType;
                    break;
                }
            }
        }

        classToSearch = sym->parent->asAGGSYM();
    }

    if (!field && !property) {
        //
        // didn't find an accessible field
        // report what we did find ...
        //
        if (badSym && ((badSym->kind == SK_MEMBVARSYM && !compiler()->clsDeclRec.isAttributeType(badSym->asMEMBVARSYM()->type))
                    || (badSym->kind == SK_PROPSYM && !compiler()->clsDeclRec.isAttributeType(badSym->asPROPSYM()->retType)))) {
            // bad type
            compiler()->clsDeclRec.errorNameRefStrSymbol(attr->pName, parent, compiler()->ErrSym(badSym), badSym, ERR_BadNamedAttributeArgumentType);
        } else if (!badSym && !(badSym = compiler()->clsDeclRec.findNextName(name, classToSearch, sym))) {
            // name not found
            compiler()->clsDeclRec.errorNameRefStr(attr->pName, parent, compiler()->ErrSym(attributeClass), ERR_NameNotInContext);
        } else if (badSym->kind == SK_PROPSYM && (!badSym->asPROPSYM()->methSet && !(badSym->asPROPSYM()->parseTree && badSym->asPROPSYM()->parseTree->asANYPROPERTY()->pSet))) {
            // read only property
            compiler()->clsDeclRec.errorStrFile(
                    attr->pName, 
                    inputfile, 
                    ERR_NamedAttrArgIsReadOnlyProperty, 
                    badSym->name->text);
        } else {
            // inaccessible field/property or readonly field
            compiler()->clsDeclRec.errorNameRefStrSymbol(attr->pName, parent, compiler()->ErrSym(badSym), badSym, ERR_BadNamedAttributeArgument);
        }

        return NULL;
    }

    //
    // bind value
    //
    EXPR * value = bindAttributeValue(parent, attr->pArgs);
    if (!value || !value->isOK()) {
        return NULL;
    }
    value = mustConvert(value, type, attr);
    if (!value || !value->isOK()) {
        return NULL;
    }

    //
    // package it up as an assignment
    //
    EXPR * op1;
    if (field) {
        ASSERT(!property);
        op1 = newExpr(attr->pName, EK_FIELD, type);
        op1->asFIELD()->field = field;
    } else {
        ASSERT(property);
        op1 = newExpr(attr->pName, EK_PROP, type);
        op1->asPROP()->prop = property;
    }
    return newExprBinop(attr, EK_ASSG, op1->type, op1, value);
}

EXPR * FUNCBREC::bindAttrArgs(PARENTSYM *parent, AGGSYM *attributeClass, ATTRNODE * attr, EXPR **namedArgs)
{
    EXPR *rval = NULL;
    EXPR ** prval = &rval;
    NODELOOP(attr->pArgs, BASE, node)

        ATTRNODE *argNode = node->asATTRARG();
        if (argNode->pName) {
            //
            // check for duplicate named argument
            //
            NODELOOP(attr->pArgs, BASE, nodeInner)
                ATTRNODE *argNodeInner = nodeInner->asATTRARG();
                if (argNodeInner->pName) {
                    if (argNodeInner->pName->asNAME()->pName == argNode->pName->asNAME()->pName) {
                        if (argNodeInner == argNode) {
                            break;
                        } else {
                            compiler()->clsDeclRec.errorStrFile(argNode, parent->getInputFile(), ERR_DuplicateNamedAttributeArgument, argNode->pName->asNAME()->pName->text);
                        }
                    }
                }
            ENDLOOP

            EXPR * item = bindNamedAttributeValue(parent, attributeClass, argNode);
            if (item) {
                newList(item, &namedArgs);
            }
        } else {
            EXPR * item = bindAttributeValue(parent, argNode->pArgs);
            newList(item, &prval);
        }
    ENDLOOP;

    return rval;
}

EXPR * FUNCBREC::bindNamedAttributeValue(PARENTSYM *parent, ATTRNODE * attr)
{
    //
    // bind name to public non-readonly field or property
    //
    NAME * name = attr->pName->asNAME()->pName;

    //
    // bind value
    //
    EXPR * value = bindAttributeValue(parent, attr->pArgs);
    if (!value || !value->isOK()) {
        return NULL;
    }
    //
    // package it up as a NamedAttrArgument
    //
    EXPRNAMEDATTRARG * e = (EXPRNAMEDATTRARG*) newExpr(attr, EK_NAMEDATTRARG, value->type);
    e->name = name;
    e->value = value;
    return e;
}

EXPR * FUNCBREC::bindAttribute(PARENTSYM *parent, ATTRNODE * attr, EXPR **namedArgs)
{
    SETINPUTFILE(parent->getInputFile());

    pMSym = NULL;
    pFSym = NULL;
    pFOrigSym = NULL;
    if (pParent != parent) {
        pParent = parent;
        initClass();
    }

    // We null out the scope so as not to get any cache hits
    SCOPESYM * scope = pOuterScope;
    pOuterScope = NULL;

    EXPR *rval = NULL;
    EXPR ** prval = &rval;
    NODELOOP(attr->pArgs, BASE, node)

        ATTRNODE *argNode = node->asATTRARG();
        if (argNode->pName) {
            //
            // check for duplicate named argument
            //
            NODELOOP(attr->pArgs, BASE, nodeInner)
                ATTRNODE *argNodeInner = nodeInner->asATTRARG();
                if (argNodeInner->pName) {
                    if (argNodeInner->pName->asNAME()->pName == argNode->pName->asNAME()->pName) {
                        if (argNodeInner == argNode) {
                            break;
                        } else {
                            compiler()->clsDeclRec.errorStrFile(argNode, parent->getInputFile(), ERR_DuplicateNamedAttributeArgument, argNode->pName->asNAME()->pName->text);
                        }
                    }
                }
            ENDLOOP

            EXPR * item = bindNamedAttributeValue(parent, argNode);
            if (item) {
                newList(item, &namedArgs);
            }
        } else {
            EXPR * item = bindAttributeValue(parent, argNode->pArgs);
            newList(item, &prval);
        }
    ENDLOOP;

    // restore it (useful if we are doing param attrs...)
    pOuterScope = scope;

    return rval;
}

EXPRCALL *FUNCBREC::bindAttribute(PARENTSYM * context, AGGSYM * attributeClass, ATTRNODE * attribute, EXPR ** namedArgs)
{
    SETINPUTFILE(context->getInputFile());

    pMSym = NULL;
    pFSym = NULL;
    pFOrigSym = NULL;
    if (pParent != context) {
        pParent = context;
        initClass();
    }

    // We null out the scope so as not to get any cache hits
    SCOPESYM * scope = pOuterScope;
    pOuterScope = NULL;

    *namedArgs = NULL;
    EXPR * args = bindAttrArgs(context, attributeClass, attribute, namedArgs);
    if (args && !args->isOK()) {
        return NULL;
    }
    EXPRCALL *call = createConstructorCall(attribute, attributeClass, NULL, args, true)->asCALL();
    if (!call->method) {
        call = NULL;
    }

    // restore it (useful if we are doing param attrs...)
    pOuterScope = scope;

    return call;
}

bool FUNCBREC::getAttributeValue(PARENTSYM *context, EXPR * val, STRCONST ** rval)
{
    SETINPUTFILE(context->getInputFile());

    pMSym = NULL;
    pFSym = NULL;
    pFOrigSym = NULL;
    if (pParent != context) {
        pParent = context;
        initClass();
    }

    // We null out the scope so as not to get any cache hits
    pOuterScope = NULL;

    val = mustConvert(val, getPDT(PT_STRING), val->tree);

    if (!val || !val->isOK()) return false;

    if (val->kind != EK_CONSTANT) {
        errorN(val->tree, ERR_BadAttributeParam);
        return false;
    }

    *rval = val->asCONSTANT()->getSVal().strVal;
    return true;

}

bool FUNCBREC::getAttributeValue(PARENTSYM *context, EXPR * val, bool * rval)
{
    SETINPUTFILE(context->getInputFile());

    pMSym = NULL;
    pFSym = NULL;
    pFOrigSym = NULL;
    if (pParent != context) {
        pParent = context;
        initClass();
    }

    // We null out the scope so as not to get any cache hits
    pOuterScope = NULL;

    val = mustConvert(val, getPDT(PT_BOOL), val->tree);

    if (!val) return false;

    if (val->kind != EK_CONSTANT) {
        errorN(val->tree, ERR_BadAttributeParam);
        return false;
    }

    *rval = val->asCONSTANT()->getVal().iVal ? true : false;
    return true;
}


bool FUNCBREC::getAttributeValue(PARENTSYM *context, EXPR * val, int * rval)
{
    SETINPUTFILE(context->getInputFile());

    pMSym = NULL;
    pFSym = NULL;
    pFOrigSym = NULL;
    if (pParent != context) {
        pParent = context;
        initClass();
    }

    // We null out the scope so as not to get any cache hits
    pOuterScope = NULL;

    val->type = val->type->underlyingType();
    val = mustConvert(val, getPDT(PT_INT), val->tree);

    if (!val) return false;

    if (val->kind != EK_CONSTANT) {
        errorN(val->tree, ERR_BadAttributeParam);
        return false;
    }

    *rval = val->asCONSTANT()->getVal().iVal;
    return true;
}

bool FUNCBREC::getAttributeValue(PARENTSYM *context, EXPR * val, TYPESYM ** rval)
{
    SETINPUTFILE(context->getInputFile());

    pMSym = NULL;
    pFSym = NULL;
    pFOrigSym = NULL;
    if (pParent != context) {
        pParent = context;
        initClass();
    }

    // We null out the scope so as not to get any cache hits
    pOuterScope = NULL;

    val = mustConvert(val, getPDT(PT_TYPE), val->tree);

    if (!val) return false;

    if (val->kind == EK_CONSTANT) {
        *rval = 0;
    } else if (val->kind == EK_TYPEOF) {
        *rval = val->asTYPEOF()->sourceType;
    } else {
        errorN(val->tree, ERR_BadAttributeParam);
        return false;
    }

    return true;
}

// Bind a parameterless predefined attribute
inline EXPR * FUNCBREC::bindSimplePredefinedAttribute(PARENTSYM * context, PREDEFTYPE pt)
{
    return bindSimplePredefinedAttribute(context, pt, NULL);
}

// Bind a predefined attribute with a single string parameter.
EXPR * FUNCBREC::bindStringPredefinedAttribute(PARENTSYM * context, PREDEFTYPE pt, LPCWSTR arg)
{
    STRCONST * str = (STRCONST*) allocator->Alloc(sizeof(STRCONST));
    CONSTVAL cv;
    int cch = (int)wcslen(arg);

    str->length = cch;
    str->text = (WCHAR*) allocator->Alloc(sizeof(WCHAR) * cch);
    memcpy(str->text, arg, sizeof(WCHAR) * cch);
    cv.strVal = str;

    return bindSimplePredefinedAttribute(context, pt, newExprConstant(NULL, compiler()->symmgr.GetPredefType(PT_STRING), cv));
}


 

// Bind a predefined attribute with a single parameter.
EXPR * FUNCBREC::bindSimplePredefinedAttribute(PARENTSYM * context, PREDEFTYPE pt, EXPR * args)
{
    SETINPUTFILE(context->getInputFile());

    pMSym = NULL;
    pFSym = NULL;
    pFOrigSym = NULL;

    // We null out the scope so as not to get any cache hits
    SCOPESYM * scope = pOuterScope;
    pOuterScope = NULL;

    AGGSYM * cls = compiler()->symmgr.GetPredefType(pt)->asAGGSYM();


    EXPR * expr = createConstructorCall(NULL, cls, NULL, args, true);

    // restore it (useful if we are doing param attrs...)
    pOuterScope = scope;

    return expr;
}

EXPR * FUNCBREC::bindSkipVerifyArgs()
{
    // this is just one arg, the value of SecurityAction.RequestMinumum

    AGGSYM * cls = compiler()->symmgr.GetPredefType(PT_SECURITYACTION);
    
    MEMBVARSYM * member = lookupClassMember(compiler()->namemgr->GetPredefName(PN_REQUESTMINIMUM), cls, false, false, true)->asMEMBVARSYM();
    if (!member) {
        compiler()->clsDeclRec.errorStrStrFile(NULL, cls->getInputFile(), ERR_MissingPredefinedMember, cls->name->text, compiler()->namemgr->GetPredefName(PN_REQUESTMINIMUM)->text);
    }

    CONSTVAL cv;
    cv.iVal = member->constVal.iVal;
    EXPR * rval = newExprConstant(NULL, cls, cv);

    return rval;
}

EXPR * FUNCBREC::bindStructLayoutArgs()
{
    // LayoutKind.Sequential

    AGGSYM * cls = compiler()->symmgr.GetPredefType(PT_LAYOUTKIND);
    
    MEMBVARSYM * member = lookupClassMember(compiler()->namemgr->GetPredefName(PN_SEQUENTIAL), cls, false, false, true)->asMEMBVARSYM();
    if (!member) {
        compiler()->clsDeclRec.errorStrStrFile(NULL, cls->getInputFile(), ERR_MissingPredefinedMember, cls->name->text, compiler()->namemgr->GetPredefName(PN_SEQUENTIAL)->text);
    }


    CONSTVAL cv;
    cv.iVal = member->constVal.iVal;
    EXPR * rval = newExprConstant(NULL, cls, cv);

    return rval;
}

EXPR * FUNCBREC::bindStructLayoutNamedArgs(bool hasNonZeroSize)
{
    // add Size = 1
    if (!hasNonZeroSize) {


        // get Size field
        AGGSYM *structLayout = compiler()->symmgr.GetPredefType(PT_STRUCTLAYOUT);
        MEMBVARSYM * field = lookupClassMember(compiler()->namemgr->GetPredefName(PN_Size), structLayout, false, false, true)->asMEMBVARSYM();
        if (!field) {
            compiler()->clsDeclRec.errorStrStrFile(NULL, structLayout->getInputFile(), ERR_MissingPredefinedMember, structLayout->name->text, compiler()->namemgr->GetPredefName(PN_Size)->text);
        }

        EXPRFIELD * fieldExpr = newExpr(NULL, EK_FIELD, field->type)->asFIELD();
        fieldExpr->field = field;
        fieldExpr->object = NULL;
        fieldExpr->type = field->type;

        // Size = 1
        CONSTVAL cv;
        cv.iVal = 1;
        return newExprBinop(NULL, EK_ASSG, fieldExpr->type, fieldExpr, newExprConstant(NULL, compiler()->symmgr.GetPredefType(PT_INT), cv));

    } else {
        return NULL;
    }
}

EXPR * FUNCBREC::bindDebuggableArgs()
{

    CONSTVAL cv;
    cv.iVal = (compiler()->options.m_fEMITDEBUGINFO && !compiler()->options.pdbOnly) ? 1 : 0;

    EXPR * eDebug = newExprConstant(NULL, compiler()->symmgr.GetPredefType(PT_BOOL), cv);

    cv.iVal = compiler()->options.m_fOPTIMIZATIONS ? 0 : 1;

    EXPR * eDisableOpt = newExprConstant(NULL, compiler()->symmgr.GetPredefType(PT_BOOL), cv);

    return newExprBinop(NULL, EK_LIST, NULL, eDebug, eDisableOpt);
}


EXPR * FUNCBREC::bindSkipVerifyNamedArgs()
{
    // this is SkipVerification = true

    AGGSYM * cls = compiler()->symmgr.GetPredefType(PT_SECURITYPERMATTRIBUTE);

    PROPSYM * prop = lookupClassMember(compiler()->namemgr->GetPredefName(PN_SKIPVERIFICATION), cls, false, false, true)->asPROPSYM();
    if (!prop) {
        compiler()->clsDeclRec.errorStrStrFile(NULL, cls->getInputFile(), ERR_MissingPredefinedMember, cls->name->text, compiler()->namemgr->GetPredefName(PN_SKIPVERIFICATION)->text);
    }

    EXPRPROP * propExpr = newExpr(NULL, EK_PROP, prop->retType)->asPROP();
    propExpr->prop = prop;
    propExpr->args = NULL;
    propExpr->object = NULL;
    propExpr->type = prop->retType;


    CONSTVAL c;
    c.iVal = true;
    EXPR * value = newExprConstant(NULL, compiler()->symmgr.GetPredefType(PT_BOOL), c);

    EXPR * rval = newExprBinop(NULL, EK_ASSG, propExpr->type, propExpr, value);

    return rval;

}


//-----------------------------------------------------------------------------

BOOL CLSDREC::isAttributeClass(TYPESYM *type)
{
    if (type->kind != SK_AGGSYM) {
        return false;
    }

    AGGSYM *cls = type->asAGGSYM();
    compiler()->symmgr.DeclareType(cls);
    return (cls->isAttribute != 0);
}

//
// returns true if a type is an attribute type
//
bool CLSDREC::isAttributeType(TYPESYM *type)
{
    compiler()->symmgr.DeclareType(type);

    switch (type->kind) {
    case SK_AGGSYM:
        if (type->asAGGSYM()->isEnum) {
            return true;
        }
        else if (type->asAGGSYM()->isPredefined) {
            switch (type->asAGGSYM()->iPredef) {

            case PT_BYTE:
            case PT_SHORT:
            case PT_INT:
            case PT_LONG:
            case PT_FLOAT:
            case PT_DOUBLE:
            case PT_CHAR:
            case PT_BOOL:
            case PT_OBJECT:
            case PT_STRING:
            case PT_TYPE:

            case PT_SBYTE:
            case PT_USHORT:
            case PT_UINT:
            case PT_ULONG:

                return true;

            }
        }
        break;

    case SK_ARRAYSYM:
        if (type->asARRAYSYM()->rank == 1 && isAttributeType(type->asARRAYSYM()->elementType()) &&
            type->asARRAYSYM()->elementType()->kind != SK_ARRAYSYM) {

            return true;
        }

    default:
        break;
    }

    return false;
}

//-----------------------------------------------------------------------------


//
// estimate maximum possible size for the UTF8 encoded string
//
__inline static unsigned EstimateUtf8EncodingLength(unsigned size)
{
    return 4 + 3 * size;
}

__inline static unsigned EstimateTypeNameLength(TYPESYM *type, COMPILER *compiler)
{
    if (!type) {
        return 1;
    }

    DWORD maxLength = MAX_FULLNAME_SIZE;
    LPCWSTR assemblyName = compiler->importer.GetAssemblyName(type->getInputFile());
    if (assemblyName) {
        maxLength += 1 + (DWORD)wcslen(assemblyName);
    }
    return EstimateUtf8EncodingLength(maxLength);
}

//
// returns a (conservitive) estimate  of the number of bytes
// needed to encode this type.
//
// NOTE: this could get called with a non-attribute type in some cases
// NOTE: which we will later flag as an error. For these scenarios we'll
// NOTE: just return 1 and report the error later.
//
static unsigned EstimateEncodedSize(TYPESYM *type, COMPILER *compiler)
{
    switch (type->kind) {

    case SK_ARRAYSYM:
 
        return 1 + EstimateEncodedSize(type->asARRAYSYM()->elementType(), compiler);
        break;

    case SK_AGGSYM:
        if (type->asAGGSYM()->isEnum) {
            return 1 + EstimateTypeNameLength(type, compiler);
        }
        break;

    default:
        break;
    }

    //
    // this should be a basic type OR type, variant, string
    //
    return 1;
}

//-----------------------------------------------------------------------------

inline TYPESYM * ATTRBIND::getPDT(PREDEFTYPE tp)
{
    if (tp == PT_VOID) return compiler->symmgr.GetVoid();
    return compiler->symmgr.GetPredefType(tp, true);
}

inline NAME * ATTRBIND::GetPredefName(PREDEFNAME pn)
{
    return compiler->namemgr->GetPredefName(pn);
}

inline bool ATTRBIND::isAttributeType(TYPESYM *type)
{
    return compiler->clsDeclRec.isAttributeType(type);
}

ATTRBIND::ATTRBIND(COMPILER *compiler, bool haveAttributeUsage) : 
    sym(0),
    ek((CorAttributeTargets)0),
    context(0),
    attributeClass(0),
    customAttributeList(0),
    haveAttributeUsage(haveAttributeUsage),
    compiler(compiler)
{
}


void ATTRBIND::Init(SYM * sym)
{
    Init(sym->getElementKind(), sym->containingDeclaration());
    this->sym = sym;
}

void ATTRBIND::Init(CorAttributeTargets ek, PARENTSYM * context)
{
    this->ek = ek;
    this->context = context;

    // set the attrloc
    switch (ek)
    {
    case catReturnValue:
        this->attrloc = AL_RETURN;
        break;
    case catParameter:
        this->attrloc = AL_PARAM;
        break;
    case catEvent:
        this->attrloc = AL_EVENT;
        break;
    case catField:
        this->attrloc = AL_FIELD;
        break;
    case catProperty:
        this->attrloc = AL_PROPERTY;
        break;
    case catConstructor:
    case catMethod:
        this->attrloc = AL_METHOD;
        break;
    case catStruct:
    case catClass:
    case catEnum:
    case catInterface:
    case catDelegate:
        this->attrloc = AL_TYPE;
        break;
    case catModule:
        this->attrloc = AL_MODULE;
        break;
    case catAssembly:
        this->attrloc = AL_ASSEMBLY;
        break;
    default:
        ASSERT(!"Bad CorAttributeTargets");
    }
}

//
// Stores a Packed string in buffer
//
// Returns the total length
//
static int StoreString(BYTE* buffer, BYTE* bufferEnd, LPCWSTR str, int cchLen)
{
    //
    // check for null string
    //
    if (!str) {
        *buffer = 0xFF;
        return 1;
    }

    //
    // convert the string to UTF8. Guess that its length is < 128
    //
    cchLen = UnicodeToUTF8 (str, &cchLen, (char*)(buffer + 1), (int)(bufferEnd - buffer - 1));

    //
    // get the real packed length of the string.
    // may need to move the UTF8 string if the string length
    // doesn't fit in one byte.
    //
    int cb = cchLen;
    int cbPackedLen = CorSigCompressData(cb, (void*)&cb);
    if (cbPackedLen != 1) {
        memmove(buffer + cbPackedLen, buffer + 1, cchLen);
    }

    //
    // copy in the packed string length
    //
    memcpy(buffer, &cb, cbPackedLen);

    return cbPackedLen + cchLen;
}

static int StoreString(BYTE* buffer, BYTE* bufferEnd, NAME *name)
{
    LPCWSTR sz;
    int length;
    if (name) {
        sz = name->text;
        length = (int)wcslen(sz);
    } else {
        sz = NULL;
        length = 0;
    }
    return StoreString(buffer, bufferEnd, sz, length);
}

static int StoreString(BYTE* buffer, BYTE* bufferEnd, STRCONST *constVal)
{
    LPCWSTR sz;
    int length;
    if (constVal) {
        sz = constVal->text;
        length = constVal->length;
    } else {
        sz = NULL;
        length = 0;
    }
    return StoreString(buffer, bufferEnd, sz, length);
}

// appends array and ptr modifiers
static void AddTypeModifiers(TYPESYM *type, WCHAR *sz, int &length)
{
    if (type->kind != SK_AGGSYM) {
        AddTypeModifiers(type->parent->asTYPESYM(), sz, length);
        switch (type->kind)
        {
        case SK_ARRAYSYM:
            sz[length++] = L'[';
            for (int i = 0; i < (type->asARRAYSYM()->rank - 1); i++) {
                sz[length++] = L',';
            }
            sz[length++] = L']';
            break;
        case SK_PTRSYM:
            sz[length++] = L'*';
            break;
        default:
            ASSERT(!"Unknown symbol type");
        }
        sz[length] = 0;
    }
}

//
// Stores a type name into a buffer
//
// Returns byte after the last used byte
//
static BYTE * StoreTypeName(BYTE *buffer, BYTE *bufferEnd, TYPESYM *type, COMPILER * compiler)
{
    WCHAR name[MAX_FULLNAME_SIZE];
    LPWSTR sz;
    int length;

    if (type) {
        //
        // reserve space for assembly name
        //
        LPCWSTR assemblyName = compiler->importer.GetAssemblyName(type->getInputFile());
        if (assemblyName) {
            sz = (WCHAR*)_alloca((wcslen(assemblyName) + 1 + MAX_FULLNAME_SIZE) * sizeof(WCHAR));
        } else {
            sz = name;
        }

        //
        // do type name
        //
        METADATAHELPER::GetFullMetadataName(type->underlyingAggregate(), sz, lengthof(name));
        length = (int)wcslen(sz);

        //
        // add ptr and array modifiers
        //
        if (type->kind != SK_AGGSYM) {
            AddTypeModifiers(type, sz, length);
        }

        //
        // append assembly name
        //
        if (assemblyName) {
            sz[length] = L',';
            length += 1;
            wcscpy(sz + length, assemblyName);
            length += (int)wcslen(sz+length);
        }

    } else {
        sz = NULL;
        length = 0;
    }
    return buffer + StoreString( buffer, bufferEnd, sz, length);
}

#define PREDEFTYPEDEF(id, name, isRequired, isSimple, isNumeric, isclass, isstruct, isiface, isdelegate, ft, et, nicename, zero, qspec, asize, st, attr, keysig) \
    (CorSerializationType)st,
CorSerializationType serializationTypes[] =
{
#include "predeftype.h"
};
#undef PREDEFTYPEDEF

static BYTE *StoreEncodedType(BYTE *buffer, BYTE *bufferEnd, TYPESYM *type, COMPILER *compiler)
{
    switch (type->kind) {
    case SK_ARRAYSYM:

        ASSERT(type->asARRAYSYM()->rank == 1);
        *buffer = SERIALIZATION_TYPE_SZARRAY;
        buffer += 1;
        return StoreEncodedType(buffer, bufferEnd, type->asARRAYSYM()->elementType(), compiler);

    case SK_AGGSYM:

        if (type->asAGGSYM()->isEnum) {
            *buffer = SERIALIZATION_TYPE_ENUM;
            buffer += 1;
            return StoreTypeName(buffer, bufferEnd, type, compiler);
        } else {
            ASSERT(type->asAGGSYM()->isPredefined);
            BYTE b = serializationTypes[type->asAGGSYM()->iPredef];
            ASSERT(b != 0);
            *buffer = b;
            return buffer + 1;
        }

    default:
        ASSERT(!"unrecognized attribute type");
        return buffer;
    }
}

void ATTRBIND::Compile(BASENODE *attributes)
{
    NODELOOP(attributes, ATTRDECL, decl)
        if (decl->location == attrloc)
        {
            NODELOOP(decl->pAttr, ATTR, attr)
                CompileSingleAttribute(attr);
            ENDLOOP;
        }
    ENDLOOP;

    ValidateAttributes();

    // GetToken() forces us to emit a token, even in cases where none is needed
    // like for param tokens.  So if there are no security attributes, don't emit a token!
    if((!sym || sym->kind != SK_GLOBALATTRSYM) && compiler->emitter.HasSecurityAttributes())
        compiler->emitter.EmitSecurityAttributes(attributes, context->getInputFile(), GetToken());
}


void ATTRBIND::CompileSynthetizedAttribute(AGGSYM * attributeClass, EXPR* ctorExpression, EXPR * namedArguments)
{

    this->attributeClass = attributeClass;
    this->ctorExpression = ctorExpression;
    this->namedArguments = namedArguments;

    CompileSingleAttribute(NULL);

}

void ATTRBIND::CompileSingleAttribute(ATTRNODE *attr)
{

    estimatedBlobSize   = NULL;
    predefinedAttribute = NULL;

    if (attr) {

        attributeClass      = NULL;
        ctorExpression      = NULL;
        namedArguments      = NULL;

        //
        // check for predefined attributes first
        //
        BASENODE * name = attr->pName;
        if (name->kind == NK_NAME) {
            predefinedAttribute = compiler->symmgr.GetPredefAttr(name->asNAME()->pName);
            ASSERT(!predefinedAttribute || PT_COUNT == predefinedAttribute->iPredef);
        }

        if (!predefinedAttribute) {
            //
            // need to resolve the attribute as regular class
            //
            TYPESYM *type = compiler->clsDeclRec.bindTypeName(name, context, BTF_ATTRIBUTE, NULL)->asTYPESYM();
            if (!type) {
                return;
            }

            attributeClass = type->asAGGSYM();
            if (attributeClass->isAbstract) {
                errorNameRef(name, context, ERR_AbstractAttributeClass);
            }

            //
            // map from the attribute class back to a possible predefined attribute
            //
            PREDEFATTR pa = attributeClass->getPredefAttr();
            if (pa != PA_COUNT) {
                predefinedAttribute = compiler->symmgr.GetPredefAttr(pa);
            }
        }
    } else {
        ASSERT(attributeClass && ctorExpression);
    }

    //
    // call the (possibly) location specific bind method
    //
    if (BindAttribute(attr)) {
        VerifyAndEmit(attr);
    }
}

bool ATTRBIND::BindAttribute(ATTRNODE *attr)
{
    if (attributeClass) {
        return BindClassAttribute(attr);
    } else {
        ASSERT(predefinedAttribute);
        return BindPredefinedAttribute(attr);
    }
}

bool ATTRBIND::BindPredefinedAttribute(ATTRNODE *attr)
{
    //
    // bind ctor args
    //
    ctorExpression = compiler->funcBRec.bindAttribute(context, attr, &namedArguments);

    //
    // verify that args are constants
    //
    bool badArg = false;
    unsigned size = 0;
    if (ctorExpression) {
        EXPRLOOP(ctorExpression, arg)
            badArg = badArg || verifyAttributeArg(arg, size);
        ENDLOOP;
    }
    return !badArg;
}

bool ATTRBIND::BindClassAttribute(ATTRNODE *attr)
{

    EXPRCALL * call;
    EXPR * namedArgs;

    if (attr) {

        if (!compiler->clsDeclRec.isAttributeClass(attributeClass)) {
            compiler->clsDeclRec.errorFileSymbol(attr->pName, context->getInputFile(), ERR_NotAnAttributeClass, attributeClass);
            return false;
        }

        //
        // do we have an attribute class which can be applied to this symbol
        //
        if (haveAttributeUsage && !(attributeClass->attributeClass & ek)) {
            if (ek == catMethod && !wcscmp(attributeClass->name->text, L"MarshalAsAttribute")) {
                errorStrStrFile(attr, context->getInputFile(), ERR_FeatureDeprecated, L"MarshalAs attribute on method", L"return : location override with MarshalAs attribute");
                return false;
            }
            errorBadSymbolKind(attr->pName);
            return false;
        }

        //
        // check for duplicate user defined attribute
        //
        if (haveAttributeUsage && !attributeClass->isMultipleAttribute) {
            SYMLIST **customAttributeListTail = &customAttributeList;
            SYMLIST * list = customAttributeList;
            while (list) {
                if (list->sym == attributeClass) {
                    compiler->clsDeclRec.errorNameRef(attr->pName, context, ERR_DuplicateAttribute);
                    return false;
                }
                customAttributeListTail = &list->next;
                list = list->next;
            }

            //
            // not found, add it to our list of attributes for this symbol
            //
            compiler->symmgr.AddToLocalSymList(attributeClass, &customAttributeListTail);
        }

        //
        // bind ctor args
        //
        namedArgs = NULL;
        call = compiler->funcBRec.bindAttribute(context, attributeClass, attr, &namedArgs);

    } else {

        call = ctorExpression->asCALL();
        namedArgs = namedArguments;
    }

    //
    // verify that args are constants, and calculate size of buffer
    //
    bool badArg = false;
    unsigned size = 2 + 2;  // prolog & number of named args
    if (call && call->method) {
        EXPRLOOP(call->args, arg)
            badArg = badArg || verifyAttributeArg(arg, size);
        ENDLOOP;
    }

    //
    // verify named args, and calculate named args size
    //
    numberOfNamedArguments = 0;
    EXPRLOOP(namedArgs, arg)
        EXPRBINOP * assg = arg->asBIN();
        NAME *name;
        TYPESYM *type;
        if (assg->p1->kind == EK_FIELD) {
            MEMBVARSYM * field = assg->p1->asFIELD()->field;
            name = field->name;
            type = field->type;
        } else {
            PROPSYM * property = assg->p1->asPROP()->prop;
            name = property->name;
            type = property->retType;
        }

        // 1 byte for FIELD or PROP
        size += 1;

        // field name
        size += EstimateUtf8EncodingLength((unsigned)wcslen(name->text));

        // field type
        size += EstimateEncodedSize(type, compiler);

        // field value
        badArg = badArg || verifyAttributeArg(assg->p2, size);

        numberOfNamedArguments += 1;
    ENDLOOP;

    estimatedBlobSize = size;

    if (badArg || !call || !call->method) {
        return false;
    }

    ctorExpression = call;
    namedArguments = namedArgs;

    return true;
}

void ATTRBIND::VerifyAndEmit(ATTRNODE *attr)
{
    if (predefinedAttribute) {
        VerifyAndEmitPredefinedAttribute(attr);
    } else {
        VerifyAndEmitClassAttribute(attr);
    }
}
    
void ATTRBIND::VerifyAndEmitClassAttribute(ATTRNODE *attr)
{
    //
    // serialize the ctor arguments into the buffer
    //
    BYTE *buffer = (BYTE*) _alloca(estimatedBlobSize);
    BYTE *bufferEnd = buffer + estimatedBlobSize;
    BYTE *current = buffer;
    *current++ = 1;
    *current++ = 0;
    EXPRLOOP(ctorExpression->asCALL()->args, arg)
        current = addAttributeArg(arg, current, bufferEnd);
    ENDLOOP;
    ASSERT(current <= bufferEnd);

    //
    // serialize the named arguments to the buffer
    //
    SET_UNALIGNED_VAL16(current, numberOfNamedArguments);
    current += 2;
    EXPRLOOP(namedArguments, arg)
        EXPRBINOP * assg = arg->asBIN();
        NAME *name;
        TYPESYM *type;

        //
        // get the name, type and store the field/property byte
        //
        if (assg->p1->kind == EK_FIELD) {
            MEMBVARSYM * field = assg->p1->asFIELD()->field;
            name = field->name;
            type = field->type;

            *current++ = SERIALIZATION_TYPE_FIELD;
        } else {
            PROPSYM * property = assg->p1->asPROP()->prop;
            name = property->name;
            type = property->retType;

            *current++ = SERIALIZATION_TYPE_PROPERTY;
        }

        //
        // member type
        //
        current = StoreEncodedType(current, bufferEnd, type, compiler);

        //
        // name
        //
        current += StoreString(current, bufferEnd, name);

        //
        // value
        //
        current = addAttributeArg(assg->p2, current, bufferEnd);
        
    ENDLOOP;
    ASSERT(current <= bufferEnd);

    //
    // write the attribute to the metadata
    //
    if (!compiler->options.m_fNOCODEGEN) {
        compiler->emitter.EmitCustomAttribute(attr, context ? context->getInputFile() : NULL, GetToken(), ctorExpression->asCALL()->method, buffer, (unsigned)(current - buffer));
    }
}

bool ATTRBIND::verifyAttributeArg(EXPR *arg, unsigned &totalSize)
{
    TYPESYM *type = arg->type;

    unsigned size = 0;
REDO:
    switch (type->kind) {
    case SK_AGGSYM:
    {
        AGGSYM *cls = type->asAGGSYM();
        if (cls->isEnum) {
            type = type->underlyingType();
            goto REDO;
        } else if (!cls->isPredefined || compiler->symmgr.GetAttrArgSize((PREDEFTYPE)cls->iPredef) == 0) {
            error(arg->tree, ERR_BadAttributeParam);
            return true;
        }

        if (compiler->symmgr.GetAttrArgSize((PREDEFTYPE)cls->iPredef) < 0) {
            switch (cls->iPredef)
            {
            case PT_STRING:
            {
                if (arg->kind != EK_CONSTANT) {
                    error(arg->tree, ERR_BadAttributeParam);
                    return true;
                }

                // estimate maximum possible size for the UTF8 encoded string
                STRCONST *constVal = arg->asCONSTANT()->getSVal().strVal;
                if (constVal) {
                    size = EstimateUtf8EncodingLength(arg->asCONSTANT()->getSVal().strVal->length);
                } else {
                    size += 1;
                }
                break;
            }

            case PT_OBJECT:
                //
                // must be a conversion from a constant expression to a object
                //
                if (arg->kind != EK_CAST) {
                    error(arg->tree, ERR_BadAttributeParam);
                    return true;
                }
                //
                // implicit cast of something to object
                // need to encode the underlying object(enum, type, string)
                arg = arg->asCAST()->p1;

                totalSize += EstimateEncodedSize(arg->type, compiler);
                return verifyAttributeArg(arg, totalSize);

            case PT_TYPE:
                switch (arg->kind) {
                case EK_TYPEOF:
                    size = EstimateTypeNameLength(arg->asTYPEOF()->sourceType, compiler);
                    break;

                case EK_CONSTANT:
                    size = EstimateTypeNameLength(NULL, compiler);
                    break;

                default:
                    error(arg->tree, ERR_BadAttributeParam);
                    return true;
                }
                break;
                
            default:
                ASSERT(!"bad variable size attribute argument type");
            }
        } else {
            if (arg->kind != EK_CONSTANT) {
                error(arg->tree, ERR_BadAttributeParam);
                return true;
            }
            size += (unsigned) compiler->symmgr.GetAttrArgSize((PREDEFTYPE)cls->iPredef);
        }
        break;
    }

    case SK_ARRAYSYM:
    {
        PARRAYSYM arr = type->asARRAYSYM();

        if (arr->rank != 1 ||                                   // Single Dimension
            arr->elementType()->kind == SK_ARRAYSYM ||          // Of non-array
            !compiler->clsDeclRec.isAttributeType(arr->elementType()) ||             // valid attribute argument type
            (arg->kind != EK_ARRINIT && arg->kind != EK_CONSTANT)) {                    // which is constant

            error(arg->tree, ERR_BadAttributeParam);
            return true;
        }

        if (arg->kind == EK_ARRINIT) {
            EXPR * param, *next = arg->asARRINIT()->args;
            while (next) {
                if (next->kind == EK_LIST) {
                    param = next->asBIN()->p1;
                    next = next->asBIN()->p2;
                } else {
                    param = next;
                    next = NULL;
                }
                if (verifyAttributeArg( param, size))
                    return true;
            }
        } else {
            ASSERT(arg->kind == EK_CONSTANT);
        }
        size += sizeof(DWORD); // number of elements

        // can't use array types in CLS compliant attributes
        if (IsCLSContext())
        {
            error(arg->tree, ERR_CLS_ArrayArgumentToAttribute);
        }
        break;
    }

    default:
        error(arg->tree, ERR_BadAttributeParam);
        return true;
    }

    totalSize += size;
    return false;
}

bool ATTRBIND::IsCLSContext()
{
    return sym->checkForCLS();
}

NAME *   ATTRBIND::getNamedArgumentName(EXPR *expr)
{
    switch (expr->kind) {
    case EK_ASSG:

        expr = expr->asBIN()->p1;

        switch (expr->kind) {                
        case EK_FIELD:
            return expr->asFIELD()->field->name;
        case EK_PROP:
            return expr->asPROP()->prop->name;
        default:
            ASSERT(!"bad expr kind");
            return NULL;
        };

    case EK_NAMEDATTRARG:
        return expr->asNAMEDATTRARG()->name;

    default:
        ASSERT(!"bad expr kind");
        return NULL;
    }
}

EXPR *   ATTRBIND::getNamedArgumentValue(EXPR *expr)
{
    switch (expr->kind) {
    case EK_ASSG:

        return expr->asBIN()->p2;

    case EK_NAMEDATTRARG:
        return expr->asNAMEDATTRARG()->value;

    default:
        ASSERT(!"bad expr kind");
        return NULL;
    }
}

void ATTRBIND::error(BASENODE *tree, int id)
{
    compiler->clsDeclRec.errorFile(tree, context->getInputFile(), id);
}

void ATTRBIND::errorNameRef(BASENODE *tree, PARENTSYM *context, int id)
{
    compiler->clsDeclRec.errorNameRef(tree, context, id);
}

void ATTRBIND::errorNameRef(NAME *name, BASENODE *tree, PARENTSYM *context, int id)
{
    compiler->clsDeclRec.errorNameRef(name, tree, context, id);
}

void ATTRBIND::errorNameRefStr(NAME *name, BASENODE *tree, PARENTSYM *context, LPCWSTR str, int id)
{
    compiler->clsDeclRec.errorNameRefStr(name, tree, context, str, id);
}

void ATTRBIND::errorStrFile(BASENODE *tree, INFILESYM *infile, int id, LPCWSTR str)
{
    compiler->clsDeclRec.errorStrFile(tree, infile, id, str);
}

void ATTRBIND::errorStrStrFile(BASENODE *tree, INFILESYM *infile, int id, LPCWSTR str1, LPCWSTR str2)
{
    compiler->clsDeclRec.errorStrStrFile(tree, infile, id, str1, str2);
}

void ATTRBIND::errorSymbol(SYM *sym, int id)
{
    compiler->clsDeclRec.errorSymbol(sym, id);
}

void ATTRBIND::errorSymbolStr(SYM *symbol, int id, LPCWSTR str)
{
    compiler->clsDeclRec.errorSymbolStr(sym, id, str);
}

void ATTRBIND::errorFile(BASENODE *tree, INFILESYM *infile, int id)
{
    compiler->clsDeclRec.errorFile(tree, infile, id);
}

void ATTRBIND::errorFileSymbol(BASENODE *tree, INFILESYM *infile, int id, SYM *sym)
{
    compiler->clsDeclRec.errorFileSymbol(tree, infile, id, sym);
}

const LPCWSTR attributeTargetStrings[] =
{
    L"assembly",            // catAssembly      = 0x0001, 
    L"module",              // catModule        = 0x0002,
    L"class",               // catClass         = 0x0004,
    L"struct",              // catStruct        = 0x0008,
    L"enum",                // catEnum          = 0x0010,
    L"constructor",         // catConstructor   = 0x0020,
    L"method",              // catMethod        = 0x0040,
    L"property",            // catProperty      = 0x0080,
    L"field",               // catField         = 0x0100,
    L"event",               // catEvent         = 0x0200,
    L"interface",           // catInterface     = 0x0400,
    L"param",               // catParameter     = 0x0800,
    L"delegate",            // catDelegate      = 0x1000,
    L"return",              // catReturn        = 0x2000,
    0
};

CorAttributeTargets targetsByPredefAttr[] =
{
#define PREDEFATTRDEF(id,name,iPredef, validOn) (CorAttributeTargets) (validOn),
#include "predefattr.h"
#undef PREDEFATTRDEF
};

void ATTRBIND::errorBadSymbolKind(BASENODE *tree)
{
    //
    // get valid targets
    //
    CorAttributeTargets validTargets;
    if (this->attributeClass) {
        validTargets = this->attributeClass->attributeClass;
    } else {
        validTargets = targetsByPredefAttr[this->predefinedAttribute->attr];
    }

    //
    // convert valid targets to string
    //
    WCHAR buffer[1024];
    LPWSTR psz = buffer;
    *psz = 0;
    const LPCWSTR *target = attributeTargetStrings;
    while (validTargets && *target)
    {
        if (validTargets & 1) {
            if (*buffer) {  
                *psz++ = L',';
                *psz++ = L' ';
            }
            wcscpy(psz, *target);
            psz += wcslen(psz);
        }

        validTargets = (CorAttributeTargets) (validTargets >> 1);
        target += 1;
    }

    compiler->clsDeclRec.errorNameRefStr(tree, context, buffer, ERR_AttributeOnBadSymbolType);
}

void ATTRBIND::ValidateAttributes()
{
    // default implementation does nothing
    // may be overridden  
}

BYTE * ATTRBIND::addAttributeArg(EXPR *arg, BYTE* buffer, BYTE* bufferEnd)
{
    TYPESYM *type = arg->type;
    unsigned int iPredef;

    switch (type->kind) {
    case SK_AGGSYM:
    {
        AGGSYM *cls = type->asAGGSYM();
        if (cls->isEnum) {
            ASSERT(cls->underlyingType->isPredefined);
            iPredef = cls->underlyingType->iPredef;
        } else {
            ASSERT(cls->isPredefined);
            iPredef = cls->iPredef;
        }

        ASSERT(compiler->symmgr.GetAttrArgSize((PREDEFTYPE)iPredef) != 0);

        switch (iPredef)
        {
        case PT_BYTE:
        case PT_BOOL:
        case PT_SBYTE:
            _ASSERTE(compiler->symmgr.GetAttrArgSize((PREDEFTYPE)iPredef) == 1);
            *buffer = (BYTE)arg->asCONSTANT()->getVal().iVal;
            return buffer + 1;

        case PT_SHORT:  
        case PT_CHAR:   
        case PT_USHORT: 
            _ASSERTE(compiler->symmgr.GetAttrArgSize((PREDEFTYPE)iPredef) == 2);
            SET_UNALIGNED_VAL16(buffer, arg->asCONSTANT()->getVal().iVal);
            return buffer + 2;

        case PT_INT:    
        case PT_UINT:   
            _ASSERTE(compiler->symmgr.GetAttrArgSize((PREDEFTYPE)iPredef) == 4);
            SET_UNALIGNED_VAL32(buffer, arg->asCONSTANT()->getVal().iVal);
            return buffer + 4;

        case PT_FLOAT:  
        {
            _ASSERTE(compiler->symmgr.GetAttrArgSize((PREDEFTYPE)iPredef) == 4);
            float tmpfloat = (float)*arg->asCONSTANT()->getVal().doubleVal;
            SET_UNALIGNED_VAL32(buffer, *reinterpret_cast<const __int32 *>(&tmpfloat));
            return buffer + sizeof(float);
        }

        case PT_LONG:   
        case PT_ULONG:  
            _ASSERTE(compiler->symmgr.GetAttrArgSize((PREDEFTYPE)iPredef) == 8);
            SET_UNALIGNED_VAL64(buffer, *arg->asCONSTANT()->getVal().ulongVal);
            return buffer + 8;

        case PT_DOUBLE: 
        {
            _ASSERTE(compiler->symmgr.GetAttrArgSize((PREDEFTYPE)iPredef) == 8);
            SET_UNALIGNED_VAL64(buffer, *arg->asCONSTANT()->getVal().ulongVal);
            return buffer + 8;
        }

        case PT_STRING:
            {
            // First, check if the string is a valid UTF8 string

            STRCONST * s = arg->asCONSTANT()->getSVal().strVal;
            
            //
            // convert the string to UTF8. Guess that its length is < 128
            //
            return buffer + StoreString( buffer, bufferEnd, s);
            }

        case PT_OBJECT:
            // This must be a constant argument to an Variant.OpImplicit call
            ASSERT(arg->kind == EK_CAST);

            //
            // implicit cast of something to object
            // need to encode the underlying object(enum, type, string)
            arg = arg->asCAST()->p1;

            buffer = StoreEncodedType(buffer, bufferEnd, arg->type, compiler);
            return addAttributeArg(arg, buffer, bufferEnd);

        case PT_TYPE:
        {
            TYPESYM *type = NULL;
            switch (arg->kind) {
            case EK_TYPEOF:
                type = arg->asTYPEOF()->sourceType;
            case EK_CONSTANT:
                // this handles the 'null' type constant
                break;
            default:
                ASSERT(!"Unknown constant of type type");
            }
            return StoreTypeName(buffer, bufferEnd, type, compiler);
            break;
        }

        default:
            ASSERT(!"bad variable size attribute argument type");
        }
        break;
    }

    case SK_ARRAYSYM:
    {
#if DEBUG
        PARRAYSYM arr = type->asARRAYSYM();
#endif

        ASSERT(arr->rank == 1 && arr->elementType()->kind == SK_AGGSYM &&
                    (arr->elementType()->asAGGSYM()->isPredefined || arr->elementType()->asAGGSYM()->isEnum));

        if (arg->kind == EK_CONSTANT) {
            SET_UNALIGNED_32(buffer, 0xFFFFFFFF);
            buffer += sizeof(DWORD);
        } else {
            SET_UNALIGNED_VAL32(buffer, arg->asARRINIT()->dimSize);
            buffer += sizeof(DWORD);
            EXPR * param, *next = arg->asARRINIT()->args;
            while (next) {
                if (next->kind == EK_LIST) {
                    param = next->asBIN()->p1;
                    next = next->asBIN()->p2;
                } else {
                    param = next;
                    next = NULL;
                }
                buffer = addAttributeArg( param, buffer, bufferEnd);
            }
        }
        break;
    }

    default:
        ASSERT(!"unexpected attribute argument type");
        break;
    }

    return buffer;
}

bool ATTRBIND::getValue(EXPR *argument, bool * rval)
{
    return compiler->funcBRec.getAttributeValue(context, argument, rval);
}

bool ATTRBIND::getValue(EXPR *argument, int * rval)
{
    return compiler->funcBRec.getAttributeValue(context, argument, rval);
}

bool ATTRBIND::getValue(EXPR *argument, STRCONST ** rval)
{
    return compiler->funcBRec.getAttributeValue(context, argument, rval);
}

bool ATTRBIND::getValue(EXPR *argument, TYPESYM **type)
{
    return compiler->funcBRec.getAttributeValue(context, argument, type);
}

STRCONST *   ATTRBIND::getKnownString(EXPR *expr)
{
    return expr->asCONSTANT()->getSVal().strVal;
}

bool         ATTRBIND::getKnownBool(EXPR *expr)
{
    return expr->asCONSTANT()->getVal().iVal ? true : false;
}

void ATTRBIND::compileObsolete(ATTRNODE * attr)
{
    //
    // crack the ctor arguments to get the obsolete information
    //
    sym->isDeprecated = true;
    bool foundMessage = false;
    EXPRLOOP(ctorExpression->asCALL()->args, argument)

        if (!foundMessage) {
            STRCONST * message = NULL;
            if (!getValue(argument, &message)) {
                break;
            }
            foundMessage = true;
            if (message) {
                sym->deprecatedMessage = message->text;
            }
        } else {
            bool value = false;
            if (!getValue(argument, &value)) {
                break;
            }
            sym->isDeprecatedError = value;
        }
    ENDLOOP;
}

void ATTRBIND::compileCLS(ATTRNODE * attr)
{
    sym->hasCLSattribute = true;
    sym->isCLS = false;

    //
    // mark it according to the argument
    //
    EXPRLOOP(ctorExpression->asCALL()->args, argument)

        if (argument->type->isPredefType(PT_BOOL)) {
            bool value;
            if (!getValue(argument, &value)) {
                break;
            }
            sym->isCLS = value;
        }
    ENDLOOP;

    if (sym->kind == SK_EVENTSYM) {
        EVENTSYM *event = sym->asEVENTSYM();

        event->methAdd->hasCLSattribute     = true;
        event->methAdd->isCLS               = event->isCLS;
        event->methRemove->hasCLSattribute  = true;
        event->methRemove->isCLS            = event->isCLS;
    }
}

void ATTRBIND::emitCLS(ATTRNODE * attr)
{
	//
	// CLSCompliantAttribute is emitted as a regular custom attribute
	//
	VerifyAndEmitClassAttribute(attr);

    if (sym->isCLS && !(sym->getInputFile()->hasCLSattribute && sym->getInputFile()->isCLS)) {
        // It's illegal to have CLSCompliant(true) when the assembly is not marked as true
        errorStrFile( attr, sym->getInputFile(), ERR_CLS_AssemblyNotCLS, compiler->ErrSym(sym));
    }
}

//-----------------------------------------------------------------------------

void AGGATTRBIND::Compile(COMPILER *compiler, AGGSYM *cls, AGGINFO *info)
{
    NAME * defaultMemberName = NULL;
    // Do we have any indexers? If so, we must emit a default member name.
    if (cls->isStruct || cls->isClass || cls->isInterface) {
        PROPSYM * indexer = compiler->symmgr.LookupGlobalSym(compiler->namemgr->GetPredefName(PN_INDEXERINTERNAL), 
                                                             cls, MASK_PROPSYM)->asPROPSYM();
        while (indexer) {
            NAME * indexerName = indexer->getRealName();

            // All indexers better have the same metadata name. 
            if (defaultMemberName && indexerName != defaultMemberName) 
                compiler->clsDeclRec.errorFile(indexer->getParseTree(), indexer->getInputFile(), ERR_InconsistantIndexerNames);
            defaultMemberName = indexerName;

            indexer = compiler->symmgr.LookupNextSym(indexer, cls, MASK_PROPSYM)->asPROPSYM();
        }
    }
        

    //
    // do we have attributes
    //
    BASENODE *attributes = cls->getAttributesNode();

    AGGATTRBIND attrbind(compiler, cls, info, defaultMemberName);
    attrbind.ATTRBIND::Compile(attributes);
    if (!cls->isComImport && cls->isInterface) {
        cls->underlyingType = NULL;
        cls->comImportCoClass = NULL;
    }

    // If we have any indexers in us, emit the "DefaultMember" attribute with the name of the indexer.
    if (defaultMemberName) {
        attrbind.defaultMemberName = NULL;
        attrbind.ATTRBIND::CompileSynthetizedAttribute(compiler->symmgr.GetPredefType(PT_DEFAULTMEMBER)->asAGGSYM(), 
                                                       compiler->funcBRec.bindStringPredefinedAttribute(cls, PT_DEFAULTMEMBER, defaultMemberName->text), NULL);
    }
}

AGGATTRBIND::AGGATTRBIND(COMPILER *compiler, AGGSYM *cls, AGGINFO *info, NAME * defaultMemberName) :
    ATTRBIND(compiler, true),
    cls(cls),
    info(info),
    defaultMemberName(defaultMemberName)
{
    Init(cls);
}

void AGGATTRBIND::VerifyAndEmitClassAttribute(ATTRNODE *attr)
{
    if (attributeClass->isPredefType(PT_DEFAULTMEMBER) && defaultMemberName) {
        errorFile(attr->pName, cls->getInputFile(), ERR_DefaultMemberOnIndexedType);
    } else {
        ATTRBIND::VerifyAndEmitClassAttribute(attr);
    }
}

void AGGATTRBIND::VerifyAndEmitPredefinedAttribute(ATTRNODE *attr)
{
    BASENODE *name = attr->pName;
    
    switch (predefinedAttribute->attr) {
    case PA_ATTRIBUTEUSAGE:
        if (!cls->isAttribute) {
            errorNameRef(name, cls, ERR_AttributeUsageOnNonAttributeClass);
        } else {
            VerifyAndEmitClassAttribute(attr);
        }
        break;

    case PA_STRUCTLAYOUT:
        compileStructLayout(attr);
        break;

    case PA_COMIMPORT:

        VerifyAndEmitClassAttribute(attr);
        info->isComimport = true;
        cls->isComImport = true;
        if (cls->isClass) {
            // this is a wrapper for a  COM classic coclass

            // can only have a compiler generated constructor
            METHSYM *ctor = compiler->symmgr.LookupGlobalSym(GetPredefName(PN_CTOR), cls, MASK_METHSYM)->asMETHSYM();
            while (ctor)
            {
                if (ctor->isCompilerGeneratedCtor())
                {
                    // whose implementation is supplied by COM+
                    ctor->isExternal = true;
                    ctor->isSysNative = true;

                    //
                    // we need to reset the method's flags
                    //
                    compiler->emitter.ResetMethodFlags(ctor);
                }
                else
                {
                    this->errorSymbol(ctor, ERR_ComImportWithUserCtor);
                }

                ctor = ctor->nextSameName->asMETHSYM();
            }
        }
        break;

    case PA_GUID:
        // just emit this as a regular custom attribute
        info->hasUuid = true;
        VerifyAndEmitClassAttribute(attr);
        break;

    case PA_COCLASS:
        compileCoClass(attr);
        break;

    case PA_OBSOLETE:
        //
        // obsolete is emitted as a regular custom attribute
        //
        VerifyAndEmitClassAttribute(attr);
        break;

    case PA_CLSCOMPLIANT:
        emitCLS(attr);
        break;

    case PA_REQUIRED:
        //
        // predefined attribute which is not allowed in C#
        //
        compiler->clsDeclRec.errorNameRef(name, context, ERR_CantUseRequiredAttribute);
        break;

    default:
        //
        // predefined attribute which is not valid on aggregates
        //
        errorBadSymbolKind(name);
        break;
    }
}

void AGGATTRBIND::compileStructLayout(ATTRNODE * attr)
{
    if (!cls->isClass && !cls->isStruct ) {
        errorBadSymbolKind(attr->pName);
        return;
    }

    info->hasStructLayout = true;

    int layoutKind;
    EXPRLOOP(ctorExpression->asCALL()->args, argument)
        getValue(argument, &layoutKind);
        break;
    ENDLOOP;
    info->hasExplicitLayout = (layoutKind == 2);

    //
    // if the class has structlayout then it is
    // expected to be used in Interop
    //
    // Disable warnings for unassigned members for all interop structs here
    //
    cls->hasExternReference = true;

    VerifyAndEmitClassAttribute(attr);
}

void AGGATTRBIND::compileCoClass(ATTRNODE * attr)
{
    //
    // if the attribute is on an interface, then we do some special processing
    //
    if (cls->isInterface) {
        //
        // set the baseClass to point to the CoClass
        // and fill in the comImportCoClass string
        //
        EXPRLOOP(ctorExpression->asCALL()->args, argument)

            if (argument->type->isPredefType(PT_TYPE)) {
                TYPESYM * value;
                if (!getValue(argument, &value)) {
                    break;
                }
                if (value && value->kind == SK_AGGSYM && value->asAGGSYM()->isClass) {
                    WCHAR buffer[MAX_FULLNAME_SIZE];
                    if (METADATAHELPER::GetFullMetadataName(value->underlyingAggregate(), buffer, lengthof(buffer))) {
                        cls->underlyingType = value->asAGGSYM();
                        cls->comImportCoClass = compiler->globalSymAlloc.AllocStr(buffer);
                    }
                }
            }
        ENDLOOP;
    }
    //
    // Now emit this like a regular attribute
    //
    VerifyAndEmitClassAttribute(attr);
}

void AGGATTRBIND::ValidateAttributes()
{
    if (info->isComimport && !info->hasUuid) {
        errorSymbol(cls, ERR_ComImportWithoutUuidAttribute);
    }

    // struct layout defaults to sequential if the user didn't explicitly specify
    if (!info->hasStructLayout && cls->isStruct)
    {
        // structs with 0 instance fields must be given an explicit size of 0
        bool hasInstanceVar = false;
        FOREACHCHILD(cls, child)
            if (child->kind == SK_MEMBVARSYM && !child->asMEMBVARSYM()->isStatic) {
                hasInstanceVar = true;
                break;
            }
        ENDFOREACHCHILD

        CompileSynthetizedAttribute(compiler->symmgr.GetPredefType(PT_STRUCTLAYOUT)->asAGGSYM(),
                                    compiler->funcBRec.bindSimplePredefinedAttribute(context, 
                                                                                     PT_STRUCTLAYOUT, 
                                                                                     compiler->funcBRec.bindStructLayoutArgs()),
                                    compiler->funcBRec.bindStructLayoutNamedArgs(hasInstanceVar));
    }
}

//-----------------------------------------------------------------------------

void ATTRATTRBIND::Compile(COMPILER *compiler, AGGSYM *cls)
{
    //
    // do we have attributes
    //
    BASENODE *attributes = cls->getAttributesNode();
    if (attributes) {
        ATTRATTRBIND attrbind(compiler, cls);
        attrbind.ATTRBIND::Compile(attributes);
    }
}

ATTRATTRBIND::ATTRATTRBIND(COMPILER *compiler, AGGSYM *cls) :
    ATTRBIND(compiler, false),
    cls(cls)
{
    Init(cls);
}

bool ATTRATTRBIND::BindAttribute(ATTRNODE *attr)
{
    //
    // only do attribute attributes
    //
    if (predefinedAttribute && 
			(predefinedAttribute->attr == PA_ATTRIBUTEUSAGE ||
			 predefinedAttribute->attr == PA_OBSOLETE ||
			 predefinedAttribute->attr == PA_CLSCOMPLIANT)) {
        return ATTRBIND::BindAttribute(attr);
    } else {
        return false;
    }
}

void ATTRATTRBIND::VerifyAndEmitPredefinedAttribute(ATTRNODE *attr)
{
    if (PA_OBSOLETE == predefinedAttribute->attr) {
        compileObsolete(attr);
        return;
    }
	if (PA_CLSCOMPLIANT == predefinedAttribute->attr) {
		compileCLS(attr);
		return;
	}

    ASSERT(PA_ATTRIBUTEUSAGE == predefinedAttribute->attr);

    BASENODE *name = attr->pName;
    if (cls->attributeClass) {
        if (cls->iPredef != PT_ATTRIBUTEUSAGE 
			&& cls->iPredef != PT_CONDITIONAL
			&& cls->iPredef != PT_OBSOLETE
            && cls->iPredef != PT_CLSCOMPLIANT) {
            errorNameRef(name, cls, ERR_DuplicateAttribute);
            return;
        } else {
            //
            // attributeusage, conditional and obsolete attributes are 'special'
            //
            cls->attributeClass = (CorAttributeTargets) 0;
        }
    }
    bool foundAllowMultiple = false;

    EXPRLOOP(ctorExpression->asCALL()->args, argument)
        if (cls->attributeClass) {
            // using a constructor which should be removed
            errorStrStrFile(attr, 
                            cls->getInputFile(), 
                            ERR_DeprecatedSymbolStr, 
                            compiler->ErrSym(ctorExpression->asCALL()->method), 
                            L"Use single argument contsructor instead");
            return;
        }
        if (getValue(argument, (int*) &cls->attributeClass)) {
            if (cls->attributeClass == 0 || (cls->attributeClass & ~catAll)) {
                errorStrFile(argument->tree, context->getInputFile(), ERR_InvalidAttributeArgument, compiler->ErrNameNode(attr->pName));
                cls->attributeClass = catAll;
            }
        }
    ENDLOOP;

    EXPRLOOP(namedArguments, argument)

        NAME *name = getNamedArgumentName(argument);
        EXPR *value = getNamedArgumentValue(argument);
        if (name == GetPredefName(PN_VALIDON)) {

            if (cls->attributeClass != 0) {
                errorNameRef(name, argument->tree, cls, ERR_DuplicateNamedAttributeArgument);
            } else if (!getValue(value, (int*) &cls->attributeClass)) {
                // error already reported
            } else if (cls->attributeClass == 0 || (cls->attributeClass & ~catAll)) {

                errorNameRef(name, argument->tree, cls, ERR_InvalidNamedArgument);
                cls->attributeClass = catAll;
            }
        } else if (name == GetPredefName(PN_ALLOWMULTIPLE)) {

            bool isMultiple = false;

            if (foundAllowMultiple) {
                errorNameRef(name, argument->tree, cls, ERR_DuplicateNamedAttributeArgument);
            } else if (!getValue(value, &isMultiple)) {
                // error already reported
            } else {
                cls->isMultipleAttribute = isMultiple;
                foundAllowMultiple = true;
            }
        } else if (name == GetPredefName(PN_INHERITED)) {
        } else {
            ASSERT(!"unknown named argument to attributeusage attribute");
        }

    ENDLOOP;

    //
    // set default allow on
    //
    if (cls->attributeClass == 0) {
        cls->attributeClass = catAll;
    }
}

//-----------------------------------------------------------------------------

void METHATTRBIND::Compile(COMPILER *compiler, METHSYM *method, METHINFO *info, AGGINFO *agginfo)
{
    //
    // check for compiler generated ctor, cctor, delegate member
    //
    if (method->isPropertyAccessor || method->isEventAccessor) {
        //
        // get marshalling attributes found on property declaration
        //
        ACCESSORATTRBIND::Compile(compiler, method, info, agginfo);
        return;
    }

    BASENODE *attributes = method->getAttributesNode();
    if (attributes) {
        METHATTRBIND attrbind(compiler, method, info, agginfo);
        attrbind.ATTRBIND::Compile(attributes);
    }
}

METHATTRBIND::METHATTRBIND(COMPILER *compiler, METHSYM *method, METHINFO *info, AGGINFO *agginfo) :
    ATTRBIND(compiler, true),
    method(method),
    info(info),
    agginfo(agginfo)
{
    Init(method);
}

void METHATTRBIND::VerifyAndEmitPredefinedAttribute(ATTRNODE *attr)
{
    BASENODE *name = attr->pName;

    switch (predefinedAttribute->attr) {
    case PA_CONDITIONAL:
        VerifyAndEmitClassAttribute(attr);
        break;

    case PA_DLLIMPORT:
        compileMethodAttribDllImport(attr);
        break;

    case PA_OBSOLETE:
        //
        // obsolete is emitted as a regular custom attribute
        //
        VerifyAndEmitClassAttribute(attr);
        break;

    case PA_CLSCOMPLIANT:
        emitCLS(attr);
        break;

    default:
        //
        // predefined attribute which is not valid on aggregates
        //
        errorBadSymbolKind(name);
        break;
    }
}

//
// compiles a dllimport attribute on a method
//
void METHATTRBIND::compileMethodAttribDllImport(ATTRNODE * attr)
{
    if (!method->isStatic || !method->isExternal) {
        errorFile(attr->pName, method->getInputFile(), ERR_DllImportOnInvalidMethod);
        return;
    }

    VerifyAndEmitClassAttribute(attr);
}

//-----------------------------------------------------------------------------

void CONDITIONALATTRBIND::Compile(COMPILER *compiler, METHSYM *method)
{
    if (method->isPropertyAccessor || method->isEventAccessor) {
        // conditional not allowed on accessors
        return;
    }

    BASENODE *attributes = method->getAttributesNode();
    if (!attributes) {
        return;
    }

    CONDITIONALATTRBIND attrbind(compiler, method);
    attrbind.ATTRBIND::Compile(attributes);
}

CONDITIONALATTRBIND::CONDITIONALATTRBIND(COMPILER *compiler, METHSYM *method) :
    ATTRBIND(compiler, false),
    method(method),
    list(&method->conditionalSymbols)
{
    Init(method);
}

bool CONDITIONALATTRBIND::BindAttribute(ATTRNODE *attr)
{
    //
    // only do conditonal attributes
    //
    if (predefinedAttribute && 
			(predefinedAttribute->attr == PA_CONDITIONAL ||
			 predefinedAttribute->attr == PA_OBSOLETE ||
			 predefinedAttribute->attr == PA_CLSCOMPLIANT)) {
        return ATTRBIND::BindAttribute(attr);
    } else {
        return false;
    }
}

void CONDITIONALATTRBIND::VerifyAndEmitPredefinedAttribute(ATTRNODE *attr)
{
    if (PA_OBSOLETE == predefinedAttribute->attr) {
        compileObsolete(attr);
        return;
    }
	if (PA_CLSCOMPLIANT == predefinedAttribute->attr ) {
		compileCLS(attr);
		return;
	}

    ASSERT(predefinedAttribute->attr == PA_CONDITIONAL);

    BASENODE *name = attr->pName;

    if (method->getClass()->isInterface) {
        errorFile(attr, method->getInputFile(), ERR_ConditionalOnInterfaceMethod);
    } else if (!method->isUserCallable() || !method->name || method->isCtor || method->isDtor) {
        errorStrFile(attr, method->getInputFile(), ERR_ConditionalOnSpecialMethod, compiler->ErrSym(method));
    } else if (method->isOverride) {
        errorStrFile(attr, method->getInputFile(), ERR_ConditionalOnOverride, compiler->ErrSym(method));
    } else if (method->retType != compiler->symmgr.GetVoid()) {
        errorStrFile(attr, method->getInputFile(), ERR_ConditionalMustReturnVoid, compiler->ErrSym(method));
    } else if (!attr->pArgs) {
        errorNameRef(name, method->getClass(), ERR_TooFewArgumentsToAttribute);
    } else {
        //
        // validate arguments ...
        //
        STRCONST * conditionalValue = NULL;
        EXPRLOOP(ctorExpression->asCALL()->args, argument)
            if (conditionalValue) {
                errorStrFile(argument->tree, method->getInputFile(), ERR_TooManyArgumentsToAttribute, compiler->ErrNameNode(name));
                break;
            } else if (!getValue(argument, &conditionalValue)) {
                break;
            } else {
                //
                // convert the string to a name and return it
                //
                NAME *name = compiler->namemgr->AddString(conditionalValue->text, conditionalValue->length);
                compiler->symmgr.AddToGlobalNameList(name, &list);
            }
        ENDLOOP;

        EXPRLOOP(this->namedArguments, argument)
            errorNameRef(attr->pName, method->getClass(), ERR_NamedArgumentToAttribute);
            break;
        ENDLOOP;
    }
}

//-----------------------------------------------------------------------------

void ACCESSORATTRBIND::Compile(COMPILER *compiler, METHSYM *method, METHINFO *info, AGGINFO *agginfo)
{
    ASSERT(method->isPropertyAccessor || method->isEventAccessor);

    BASENODE *attributes = method->getAttributesNode();
    if (!attributes) {
        return;
    }

    ACCESSORATTRBIND attrbind(compiler, method, info, agginfo);
    attrbind.ATTRBIND::Compile(attributes);
}

ACCESSORATTRBIND::ACCESSORATTRBIND(COMPILER *compiler, METHSYM *method, METHINFO *info, AGGINFO *agginfo) :
    METHATTRBIND(compiler, method, info, agginfo)
{
}

void ACCESSORATTRBIND::VerifyAndEmitPredefinedAttribute(ATTRNODE *attr)
{
    switch (predefinedAttribute->attr) {

    //
    // need to error on attributes which can go on methods but not on accessors
    //
    case PA_CONDITIONAL:
    case PA_OBSOLETE:
    case PA_CLSCOMPLIANT:
        //
        // predefined attribute which is not valid on accessors
        //
        errorBadSymbolKind(attr->pName);
        break;

    //
    // otherwise its just another attribute on a method
    //
    default:
        METHATTRBIND::VerifyAndEmitPredefinedAttribute(attr);
        break;
    }
}

//-----------------------------------------------------------------------------

void PROPATTRBIND::Compile(COMPILER *compiler, PROPSYM *property, PROPINFO *info)
{
    BASENODE * attributes = property->getAttributesNode();
    if (!attributes) {
        return;
    }

    PROPATTRBIND attrbind(compiler, property, info);
    if (attributes)
        attrbind.ATTRBIND::Compile(attributes);
}

PROPATTRBIND::PROPATTRBIND(COMPILER *compiler, PROPSYM *property, PROPINFO *info) :
    ATTRBIND(compiler, true),
    property(property),
    info(info)
{
    Init(property);
}

void PROPATTRBIND::VerifyAndEmitPredefinedAttribute(ATTRNODE *attr)
{
    BASENODE *name = attr->pName;

    switch (predefinedAttribute->attr) {

    case PA_OBSOLETE:
        //
        // obsolete is emitted as a regular custom attribute
        //
        VerifyAndEmitClassAttribute(attr);
        break;

    case PA_CLSCOMPLIANT:
        emitCLS(attr);
        break;

    case PA_NAME:
        //
        // name attributes are checked in the define stage
        // for non-explicit impl indexers, otherwise they are an error
        //
        if (property->isIndexer() && !property->explicitImpl) {
            break;
        }
        // NOTE: fallthrough to error bad type

    default:
        //
        // predefined attribute which is not valid on aggregates
        //
        errorBadSymbolKind(name);
        break;
    }
}

//-----------------------------------------------------------------------------

void EVENTATTRBIND::Compile(COMPILER *compiler, EVENTSYM *event, EVENTINFO *info)
{
    BASENODE * attributes = event->getAttributesNode();
    if (!attributes) {
        return;
    }

    EVENTATTRBIND attrbind(compiler, event, info);
    attrbind.ATTRBIND::Compile(attributes);
}

EVENTATTRBIND::EVENTATTRBIND(COMPILER *compiler, EVENTSYM *event, EVENTINFO *info) :
    ATTRBIND(compiler, true),
    event(event),
    info(info)
{
    Init(event);
}

void EVENTATTRBIND::VerifyAndEmitPredefinedAttribute(ATTRNODE *attr)
{
    BASENODE *name = attr->pName;

    switch (predefinedAttribute->attr) {
    case PA_OBSOLETE:
        //
        // obsolete is emitted as a regular custom attribute
        //
        VerifyAndEmitClassAttribute(attr);
        break;

    case PA_CLSCOMPLIANT:
        emitCLS(attr);
        break;

    default:
        //
        // predefined attribute which is not valid on events
        //
        errorBadSymbolKind(name);
        break;
    }
}

//-----------------------------------------------------------------------------

void MEMBVARATTRBIND::Compile(COMPILER *compiler, MEMBVARSYM *field, MEMBVARINFO *info, AGGINFO *agginfo)
{
    BASENODE *attributes = field->getAttributesNode();
    if (!attributes) {
        MEMBVARATTRBIND::ValidateAttributes(compiler, field, info, agginfo);
        return;
    }

    MEMBVARATTRBIND attrbind(compiler, field, info, agginfo);
    attrbind.ATTRBIND::Compile(attributes);
}

MEMBVARATTRBIND::MEMBVARATTRBIND(COMPILER *compiler, MEMBVARSYM *field, MEMBVARINFO *info, AGGINFO *agginfo) :
    ATTRBIND(compiler, true),
    field(field),
    info(info),
    agginfo(agginfo)
{
    Init(field);
}

void MEMBVARATTRBIND::VerifyAndEmitPredefinedAttribute(ATTRNODE *attr)
{
    BASENODE *name = attr->pName;

    if (field->getClass()->isEnum) {
        switch (predefinedAttribute->attr) {

        case PA_OBSOLETE:
            //
            // obsolete is emitted as a regular custom attribute
            //
            VerifyAndEmitClassAttribute(attr);
            break;

        default:

            //
            // predefined attribute which is not valid on enumerators
            //
            errorBadSymbolKind(name);
            break;
        }
    } else {
        switch (predefinedAttribute->attr) {

        case PA_CLSCOMPLIANT:
            emitCLS(attr);
            break;

        case PA_STRUCTOFFSET:
            compileStructOffset(attr);
            break;

        case PA_OBSOLETE:
            VerifyAndEmitClassAttribute(attr);
            break;

        default:

            //
            // predefined attribute which is not valid on fields
            //
            errorBadSymbolKind(name);
            break;
        }
    }
}

void MEMBVARATTRBIND::compileStructOffset(ATTRNODE *attr)
{
    //
    // must be explicit layout kind on aggregate
    //
    if (!agginfo->hasExplicitLayout) {
        errorFile(attr->pName, field->getInputFile(), ERR_StructOffsetOnBadStruct);
        return;
    }

    if (field->isConst || field->isStatic) {
        errorFile(attr->pName, field->getInputFile(), ERR_StructOffsetOnBadField);
        return;
    }

    info->foundOffset = true;

    VerifyAndEmitClassAttribute(attr);
}

void MEMBVARATTRBIND::ValidateAttributes()
{
    ValidateAttributes(compiler, field, info, agginfo);
}

void MEMBVARATTRBIND::ValidateAttributes(COMPILER *compiler, MEMBVARSYM *field, MEMBVARINFO *info, AGGINFO *agginfo)
{
    if (agginfo->hasExplicitLayout && !field->isConst && !field->isStatic && !info->foundOffset) {
        compiler->clsDeclRec.errorSymbol(field, ERR_MissingStructOffset);
    }

    // we can't store decimal constants as COM+ literals so we hack them in as custom values
    if (field->isConst && field->type->isPredefType(PT_DECIMAL)) {
		DecimalConstantBuffer buffer;

		buffer.format	= VAL16(1);
		buffer.scale	= DECIMAL_SCALE(*(field->constVal.decVal));
		buffer.sign		= DECIMAL_SIGN(*(field->constVal.decVal));
		buffer.hi		= VAL32(DECIMAL_HI32(*(field->constVal.decVal)));
		buffer.mid		= VAL32(DECIMAL_MID32(*(field->constVal.decVal)));
		buffer.low		= VAL32(DECIMAL_LO32(*(field->constVal.decVal)));
		buffer.cNamedArgs = 0;

		compiler->emitter.EmitCustomAttribute(
			field->getParseTree(), 
			field->getInputFile(), 
			field->tokenEmit, 
			compiler->symmgr.LookupGlobalSym(
				compiler->namemgr->GetPredefName(PN_CTOR), 
				compiler->symmgr.GetPredefType(PT_DECIMALCONSTANT, true), 
				MASK_METHSYM
				)->asMETHSYM(),
			(BYTE*)&buffer,
			sizeof(buffer));
    }
}

//-----------------------------------------------------------------------------

PARAMATTRBIND::PARAMATTRBIND(COMPILER *compiler, METHSYM *method) :
    ATTRBIND(compiler, true),
    method(method)
{
}

// for return values only
PARAMATTRBIND::PARAMATTRBIND(COMPILER *compiler, METHSYM *method, TYPESYM *type, PARAMINFO *info, int index) :
    ATTRBIND(compiler, true ),
    method(method)
{
    Init(type, info, index);
}

void PARAMATTRBIND::Init(TYPESYM *type, PARAMINFO *info, int index)
{
    this->parameterType = type;
    this->paramInfo = info;
    this->index = index;
    this->customAttributeList = 0;
    ATTRBIND::Init((index == 0 ? catReturnValue : catParameter), method->getClass());
    EmitParamProps();
}

mdToken PARAMATTRBIND::GetToken()
{
    if (paramInfo->tokenEmit == mdTokenNil) {
        compiler->emitter.DefineParam(method->tokenEmit, index, &paramInfo->tokenEmit);
    }

    return paramInfo->tokenEmit;
}

bool PARAMATTRBIND::IsCLSContext()
{
    return method->checkForCLS();
}


void PARAMATTRBIND::VerifyAndEmitPredefinedAttribute(ATTRNODE *attr)
{
    BASENODE *name = attr->pName;

    switch (predefinedAttribute->attr) {

    case PA_IN:
        if (parameterType->kind == SK_PARAMMODSYM && parameterType->asPARAMMODSYM()->isOut) {
            // "out" parameter cannot have the "in" attribute.
            errorFile(attr, method->getInputFile(), ERR_InAttrOnOutParam);
        } else {
            paramInfo->isIn = true;
            VerifyAndEmitClassAttribute(attr);
        }
        break;

    case PA_OUT:
        paramInfo->isOut = true;
        VerifyAndEmitClassAttribute(attr);
        break;

    case PA_PARAMARRAY:
        errorFile(attr, method->getInputFile(), ERR_ExplicitParamArray);
        VerifyAndEmitClassAttribute(attr);
        break;

    default:
        //
        // predefined attribute which is not valid on aggregates
        //
        errorBadSymbolKind(name);
        break;
    }
}

void PARAMATTRBIND::ValidateAttributes()
{
    if (paramInfo->isOut && !paramInfo->isIn && parameterType->kind == SK_PARAMMODSYM && parameterType->asPARAMMODSYM()->isRef)
    {
        errorSymbol(method, ERR_OutAttrOnRefParam);
    }
}

void PARAMATTRBIND::EmitPredefinedAttributes()
{
    // emit the synthetized parameter...
    if (paramInfo->isParamArray) {
        ATTRBIND::CompileSynthetizedAttribute(compiler->symmgr.GetPredefType(PT_PARAMS)->asAGGSYM(), 
                                              compiler->funcBRec.bindSimplePredefinedAttribute(method->getClass(), PT_PARAMS), NULL);
    }

    if (parameterType->kind == SK_PARAMMODSYM && parameterType->asPARAMMODSYM()->isOut) {
        ATTRBIND::CompileSynthetizedAttribute(compiler->symmgr.GetPredefType(PT_OUT), 
                                              compiler->funcBRec.bindSimplePredefinedAttribute(method->getClass(), PT_OUT), 
                                              NULL);
    }
}

void PARAMATTRBIND::EmitParamProps()
{
    compiler->emitter.EmitParamProp(method->tokenEmit, index, parameterType, paramInfo);
}

void PARAMATTRBIND::CompileParamList(COMPILER *compiler, METHSYM *method, METHINFO *info)
{
    PARAMATTRBIND attrbind(compiler, method);

    //
    // do return value param 
    //
    BASENODE * tree = method->getParseTree();
    BASENODE * paramTree;
    BASENODE * methodAttrs = NULL;
    if (tree->kind == NK_METHOD || tree->kind == NK_CTOR ||
        tree->kind == NK_OPERATOR || tree->kind == NK_DTOR) {

        methodAttrs = tree->asANYMETHOD()->pAttr;
        paramTree = tree->asANYMETHOD()->pParms;
    } else if (tree->kind == NK_ACCESSOR) {
        //
        // parameter attributes for property accessors, 
        //
        methodAttrs = method->getParseTree()->asACCESSOR()->pAttr;
        paramTree = tree->pParent->asANYPROPERTY()->pParms;
    } else if (method->getClass()->isDelegate) {
        if (method->isCtor) {
            // compiler generated delegate constructors don't have 
            // attributes on their parameters ... but they do have names
            for (int i = 0; i < method->cParams; i += 1) {
                attrbind.Init(method->params[i], &info->paramInfos[i], i + 1);
            }
            return;
        }
        ASSERT(method->name == compiler->namemgr->GetPredefName(PN_INVOKE) || 
               method->name == compiler->namemgr->GetPredefName(PN_BEGININVOKE) || 
               method->name == compiler->namemgr->GetPredefName(PN_ENDINVOKE));
        paramTree = method->getClass()->getParseTree()->asDELEGATE()->pParms;
        if (method->name == compiler->namemgr->GetPredefName(PN_INVOKE) || 
            method->name == compiler->namemgr->GetPredefName(PN_ENDINVOKE)) {

            //
            // emit attributes for invoke, end invoke return value
            //
            methodAttrs = method->getClass()->getParseTree()->asDELEGATE()->pAttr;
        } else {
            methodAttrs = NULL;
        }

    } else if (method->isEventAccessor) {
        // Event accessors don't have explicitly declared parameters.
        paramTree = NULL;
        methodAttrs = NULL;
    }
    else {
        ASSERT(method->isCompilerGeneratedCtor() || method->isIfaceImpl);
        return;
    }
    if (methodAttrs) {
        attrbind.Init(method->retType, &info->returnValueInfo, 0);
        attrbind.Compile(methodAttrs);
    }

    PARAMINFO *paramInfo = info->paramInfos;
    TYPESYM ** parameterType = method->params;
    int index  = 1;

    bool isBeginInvoke = false;
    bool isEndInvoke = false;
    METHSYM * invoke = NULL;

    if (method->parent->asAGGSYM()->isDelegate) {
        isEndInvoke = method->name == compiler->namemgr->GetPredefName(PN_ENDINVOKE);
        isBeginInvoke = method->name == compiler->namemgr->GetPredefName(PN_BEGININVOKE);
        if (isEndInvoke || isBeginInvoke) {
            invoke = compiler->symmgr.LookupGlobalSym(compiler->namemgr->GetPredefName(PN_INVOKE), method->parent, MASK_METHSYM)->asMETHSYM();
        }
    }

    int i = 0;
    NODELOOP(paramTree, PARAMETER, param)

        //
        // skip arguments which were removed from BeginINvoke/EndInvoke methods
        //
        if (!(isEndInvoke && invoke->params[i]->kind != SK_PARAMMODSYM)) {

            attrbind.Init(*parameterType, paramInfo, index);

            //
            // do parameter attributes
            //
            if (param->pAttr) {
                attrbind.Compile(param->pAttr);
            }

            //
            // emit predefined parameter properties(name, marshaling)
            //
            attrbind.EmitPredefinedAttributes();

            paramInfo += 1;
            parameterType += 1;
            index += 1;

        }

        i += 1;

    ENDLOOP;

    //
    // emit extra params for BeginInvoke/EndInvoke
    //
    if (isBeginInvoke || isEndInvoke) {
        attrbind.Init(*parameterType++, paramInfo++, index++);
        attrbind.EmitPredefinedAttributes();
        if (isBeginInvoke) {
            attrbind.Init(*parameterType, paramInfo, index++);
            attrbind.EmitPredefinedAttributes();
        }
    }

    //
    // emit attributes for implicit 'value' parameter on accessor methods.
    //
    unsigned short cParams = method->cParams;
    if (method->isVarargs) cParams--;

    if (index != (cParams + 1)) {

        ASSERT (!isEndInvoke && !isBeginInvoke);

        ASSERT((method->isPropertyAccessor && !method->isGetAccessor()) || method->isEventAccessor);
        ASSERT(paramInfo->name == compiler->namemgr->GetPredefName(PN_VALUE));

        attrbind.Init(*parameterType, paramInfo, index++);

        //
        // attributes for implicit value parameter are on the method
        //
        if (method->isPropertyAccessor && !method->isGetAccessor() || method->isEventAccessor) {
            attrbind.Compile(methodAttrs);
        }
        attrbind.EmitPredefinedAttributes();
    }

    ASSERT(index == (cParams + 1));
}

//-----------------------------------------------------------------------------

void GLOBALATTRBIND::Compile(COMPILER *compiler, PARENTSYM *context, GLOBALATTRSYM *globalAttributes, mdToken tokenEmit)
{
    //
    // do we have attributes
    //
    if (globalAttributes) {

        GLOBALATTRBIND attrbind(compiler, globalAttributes, tokenEmit);
        while (1)
        {
            long ec = compiler->ErrorCount();
            if (!globalAttributes->isBogus)
                attrbind.ATTRBIND::Compile(globalAttributes->parseTree);
            if (ec != compiler->ErrorCount()) {
                // Overload isBogus to indicate that we got an error
                // processing this attribute and we shouldn't process it again
                // NOTE: this interacts with similar loop in CLSGLOBALATTRBIND::Compile()
                ASSERT(!globalAttributes->isBogus);
                globalAttributes->isBogus = true;
            }

            globalAttributes = globalAttributes->nextAttr;
            if (!globalAttributes) 
            {
                break;
            }
            attrbind.Init(globalAttributes);
        } 
    }

    // synthetized attributes
    GLOBALATTRBIND attrbind(compiler, tokenEmit);
    if (compiler->options.m_fUNSAFE) {
        if (TypeFromToken(tokenEmit) == mdtModule) {
            if (compiler->symmgr.GetPredefType(PT_UNVERIFCODEATTRIBUTE, false))
                attrbind.ATTRBIND::CompileSynthetizedAttribute(compiler->symmgr.GetPredefType(PT_UNVERIFCODEATTRIBUTE)->asAGGSYM(),
                                                               compiler->funcBRec.bindSimplePredefinedAttribute(context, PT_UNVERIFCODEATTRIBUTE),
                                                               NULL);
        } else {
            if (compiler->symmgr.GetPredefType(PT_UNVERIFCODEATTRIBUTE, false) &&
                compiler->symmgr.GetPredefType(PT_SECURITYPERMATTRIBUTE, false) &&
                compiler->symmgr.GetPredefType(PT_SECURITYACTION, false))
            {
                attrbind.ATTRBIND::CompileSynthetizedAttribute(compiler->symmgr.GetPredefType(PT_SECURITYPERMATTRIBUTE)->asAGGSYM(),
                                                               compiler->funcBRec.bindSimplePredefinedAttribute(context, PT_SECURITYPERMATTRIBUTE, 
                                                                                                                compiler->funcBRec.bindSkipVerifyArgs()),
                                                               compiler->funcBRec.bindSkipVerifyNamedArgs());
            }
        }
    }

    if (TypeFromToken(tokenEmit) != mdtModule) {
        // "/o+ /debug-" is the default, so don't bother emitting the attribute
        if ((!compiler->options.m_fOPTIMIZATIONS ||
            (compiler->options.m_fEMITDEBUGINFO && !compiler->options.pdbOnly)) &&
            compiler->symmgr.GetPredefType(PT_DEBUGGABLEATTRIBUTE, false)) {
            attrbind.ATTRBIND::CompileSynthetizedAttribute(compiler->symmgr.GetPredefType(PT_DEBUGGABLEATTRIBUTE)->asAGGSYM(),
                                                           compiler->funcBRec.bindSimplePredefinedAttribute(context, PT_DEBUGGABLEATTRIBUTE,
                                                                                                            compiler->funcBRec.bindDebuggableArgs()),
                                                           NULL);
        }
    }
}

GLOBALATTRBIND::GLOBALATTRBIND(COMPILER *compiler, GLOBALATTRSYM *globalAttributes, mdToken tokenEmit) :
    ATTRBIND(compiler, true),
    globalAttribute(globalAttributes),
    tokenEmit(tokenEmit)
{
    Init(globalAttribute);
}

GLOBALATTRBIND::GLOBALATTRBIND(COMPILER *compiler, mdToken tokenEmit) :
    ATTRBIND(compiler, true),
    globalAttribute(NULL),
    tokenEmit(tokenEmit)
{
    ASSERT(TypeFromToken(tokenEmit) == mdtAssembly || TypeFromToken(tokenEmit) == mdtModule);
    Init((TypeFromToken(tokenEmit) == mdtAssembly) ? catAssembly : catModule, NULL);
}

void GLOBALATTRBIND::VerifyAndEmitPredefinedAttribute(ATTRNODE *attr)
{
    BASENODE *name = attr->pName;

    //
    // predefined attribute which is not valid on assemblies or modules -- if
    // it doesn't have an attribute class, it's invalid.
    //
    if (! attributeClass)
        errorBadSymbolKind(name);
    else {
        if (predefinedAttribute->attr == PA_CLSCOMPLIANT && globalAttribute->getElementKind() == catModule && compiler->BuildAssembly())
            // No CLS attributes on modules when building an assembly
            error(name, ERR_WRN_NotOnModules);
        VerifyAndEmitClassAttribute(attr);
    }
}

//-----------------------------------------------------------------------------

void CLSGLOBALATTRBIND::Compile(COMPILER *compiler, GLOBALATTRSYM *globalAttributes)
{
    //
    // do we have attributes
    //
    if (!globalAttributes) {
        return;
    }

    CLSGLOBALATTRBIND attrbind(compiler, globalAttributes);
    while (1)
    {
        long ec = compiler->ErrorCount();
        // Only compile the assembly-level attributes and the ones that don't already have errors
        if (!globalAttributes->isBogus)
            attrbind.ATTRBIND::Compile(globalAttributes->parseTree);
        if (ec != compiler->ErrorCount()) {
            // Overload isBogus to indicate that we got an error
            // processing this attribute and we shouldn't process it again
            ASSERT(!globalAttributes->isBogus);
            globalAttributes->isBogus = true;
        }
        globalAttributes = globalAttributes->nextAttr;
        if (!globalAttributes) 
        {
            break;
        }
        attrbind.Init(globalAttributes);
    } 
}

CLSGLOBALATTRBIND::CLSGLOBALATTRBIND(COMPILER *compiler, GLOBALATTRSYM *globalAttributes) :
    ATTRBIND(compiler, true)
{
    Init(globalAttributes);
}

bool CLSGLOBALATTRBIND::BindAttribute(ATTRNODE *attr)
{
    //
    // only do CLS attributes
    //
    if (predefinedAttribute && predefinedAttribute->attr == PA_CLSCOMPLIANT) {
        return ATTRBIND::BindAttribute(attr);
    } else {
        return false;
    }
}

void CLSGLOBALATTRBIND::VerifyAndEmitPredefinedAttribute(ATTRNODE *attr)
{

    //
    // predefined attribute which is valid on assemblies or modules
    //
    
    ASSERT(predefinedAttribute->attr == PA_CLSCOMPLIANT);
    sym->hasCLSattribute = true;

    EXPRLOOP(ctorExpression->asCALL()->args, argument)

        if (argument->type->isPredefType(PT_BOOL)) {
            bool value;
            if (!getValue(argument, &value)) {
                break;
            }
            sym->isCLS = value;
        }
    ENDLOOP;
}

//-----------------------------------------------------------------------------

void OBSOLETEATTRBIND::Compile(COMPILER *compiler, SYM *sym)
{
    //
    // do we have attributes
    //
    BASENODE *attributes = sym->getAttributesNode();
    if (!attributes) {
        return;
    }

    OBSOLETEATTRBIND attrbind(compiler, sym);
    attrbind.ATTRBIND::Compile(attributes);
}

OBSOLETEATTRBIND::OBSOLETEATTRBIND(COMPILER *compiler, SYM *sym) :
    ATTRBIND(compiler, false)
{
    Init(sym);
}

bool OBSOLETEATTRBIND::BindAttribute(ATTRNODE *attr)
{
    //
    // only do attribute attributes
    //
    if (predefinedAttribute && 
			(predefinedAttribute->attr == PA_OBSOLETE ||
			 predefinedAttribute->attr == PA_CLSCOMPLIANT)) {
        return ATTRBIND::BindAttribute(attr);
    } else {
        return false;
    }
}

void OBSOLETEATTRBIND::VerifyAndEmitPredefinedAttribute(ATTRNODE *attr)
{
    switch (predefinedAttribute->attr) {

    case PA_OBSOLETE:
        compileObsolete(attr);
        break;

    case PA_CLSCOMPLIANT:
        compileCLS(attr);
        break;

    default:
        ASSERT(!"Bad predefined attribute");
    }
}

