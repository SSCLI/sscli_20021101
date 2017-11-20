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
// File: symmgr.cpp
//
// Routines for storing and handling symbols.
// ===========================================================================


#include "stdafx.h"

#include "compiler.h"
#include "srcdata.h"
#include <limits.h>

/*
 * Static array of info about the predefined types.
 */
#define PREDEFTYPEDEF(id, name, isRequired, isSimple, isNumeric, isclass, isstruct, isiface, isdelegate, ft, et, nicenm, zero, qspec, asize, st, attr, keysig) \
                     { L##name, isRequired, isSimple, isNumeric, isclass, isstruct, isiface, isdelegate, qspec, ft, et, nicenm, { (INT_PTR)(zero) }, attr, asize },

const static double doubleZero = 0;
const static __int64 longZero = 0;
#if BIGENDIAN
const static DECIMAL decimalZero = { { { 0 } }, 0 };
#else   // BIGENDIAN
const static DECIMAL decimalZero = { 0, { { 0 } } };
#endif  // BIGENDIAN

const static struct {
    LPWSTR fullName;
    bool isRequired: 1;
    bool isSimple: 1;
    bool isNumeric: 1;
    bool isClass: 1;
    bool isStruct: 1;
    bool isInterface: 1;
    bool isDelegate: 1;
    bool isQSimple: 1;
    FUNDTYPE ft: 8;
    unsigned et: 8;
    LPWSTR niceName;
    CONSTVAL zero;
    PREDEFATTR attr;
    int asize;
}
predefTypeInfo[] =
{
    #include "predeftype.h"
};

#undef PREDEFTYPEDEF

/*
 * Given a symbol, get its parse tree
 */
BASENODE * SYM::getParseTree()
{
    switch (kind) {
    case SK_EXPANDEDPARAMSSYM:
        return asEXPANDEDPARAMSSYM()->realMethod->getParseTree();
    case SK_METHSYM:
    case SK_PROPSYM:
        return asMETHPROPSYM()->parseTree;
    case SK_MEMBVARSYM:
        return asMEMBVARSYM()->parseTree;
    case SK_EVENTSYM:
        return asEVENTSYM()->parseTree;
    case SK_AGGSYM:
        return asAGGSYM()->parseTree;
    case SK_NSDECLSYM:
        return asNSDECLSYM()->parseTree;
    case SK_NSSYM:
        return NULL;
    case SK_ALIASSYM:
        return asALIASSYM()->parseTree;
    case SK_ARRAYSYM:
        return asARRAYSYM()->elementType()->getParseTree();
    case SK_PTRSYM:
        return asPTRSYM()->baseType()->getParseTree();
    case SK_PARAMMODSYM:
        return asPARAMMODSYM()->paramType()->getParseTree();
    case SK_GLOBALATTRSYM:
        return asGLOBALATTRSYM()->parseTree;
    
    default:
        // should never call this with any other type
        ASSERT(FALSE);
    }

    return NULL;
}

/*
 * Given a symbol, returns the address of its metadata token for emitting
 * or NULL if this symbol is not emitted
 */
mdToken *SYM::getTokenEmitPosition()
{
    switch (kind) {
    case SK_METHSYM:
        return &asMETHSYM()->tokenEmit;

    case SK_PROPSYM:
        // explicit impl properties don't have emit tokens
        if (asPROPSYM()->explicitImpl) return NULL;
        return &asPROPSYM()->tokenEmit;

    case SK_MEMBVARSYM:
        return &asMEMBVARSYM()->tokenEmit;

    case SK_AGGSYM:
        return &asAGGSYM()->tokenEmit;

    case SK_EVENTSYM:
        return &asEVENTSYM()->tokenEmit;

    default:
        ;
    }

    return NULL;
}

/*
 * Given a symbol, returns the address of its metadata token for emitting
 * or NULL if this symbol is not emitted
 */
mdToken SYM::getTokenEmit()
{
    switch (kind) {
    case SK_METHSYM:
        return asMETHSYM()->tokenEmit;

    case SK_PROPSYM:
        // explicit impl properties don't have emit tokens
        if (asPROPSYM()->explicitImpl) return mdTokenNil;
        return asPROPSYM()->tokenEmit;

    case SK_MEMBVARSYM:
        return asMEMBVARSYM()->tokenEmit;

    case SK_EVENTSYM:
        return asEVENTSYM()->tokenEmit;

    case SK_AGGSYM:
        return asAGGSYM()->tokenEmit;

    default:
        ASSERT(!"Bad Symbol type");
    }

    return mdTokenNil;
}

BASENODE * SYM::getAttributesNode()
{
    switch (kind) {
    case SK_METHSYM:
        return asMETHSYM()->getAttributesNode();

    case SK_AGGSYM:
        return asAGGSYM()->getAttributesNode();

    case SK_MEMBVARSYM:
        return asMEMBVARSYM()->getAttributesNode();

    case SK_PROPSYM:
        return asPROPSYM()->getAttributesNode();

    case SK_EVENTSYM:
        return asEVENTSYM()->getAttributesNode();

    default:
        ASSERT(!"Bad Symbol Type");
        return 0;
    }
}

bool    SYM::isContainedInDeprecated() const
{
    if (this == 0)
        return false;

    if (this->isDeprecated)
        return true;

    if (parent && (parent->kind == SK_AGGSYM))
        return parent->isContainedInDeprecated();

    return false;
}

BASENODE * MEMBVARSYM::getAttributesNode()
{
    BASENODE *attr;
    BASENODE *parseTree = getParseTree();

    if (parseTree->kind == NK_ENUMMBR) {
        // enumerators currently don't have attributes
        attr = parseTree->asENUMMBR()->pAttr;
    } else {

        BASENODE * fieldTree = parseTree->asVARDECL()->pParent;
        while (fieldTree->kind == NK_LIST) {
            fieldTree = fieldTree->pParent;
        }
        attr = fieldTree->asANYFIELD()->pAttr;
    }

    return attr;
}

BASENODE * PROPSYM::getAttributesNode()
{
    return getParseTree()->asANYPROPERTY()->pAttr;
}

BASENODE * EVENTSYM::getAttributesNode()
{
    BASENODE * parseTree = getParseTree();
    BASENODE * attributes;

    // parseTree could be field or property parse tree.
    if (parseTree->kind == NK_VARDECL) {
        BASENODE * fieldTree = parseTree->asVARDECL()->pParent;
        while (fieldTree->kind == NK_LIST) {
            fieldTree = fieldTree->pParent;
        }
        attributes = fieldTree->asANYFIELD()->pAttr;
    }
    else if (parseTree->kind == NK_PROPERTY) {
        attributes = parseTree->asPROPERTY()->pAttr;
    }
    else {
        ASSERT(!"BadEvent nodes");
        attributes = 0;
    }

    return attributes;
}

BASENODE * METHSYM::getAttributesNode()
{
    BASENODE * tree = getParseTree();
    switch (tree->kind)
    {
    case NK_METHOD:
    case NK_CTOR:
    case NK_OPERATOR:
    case NK_DTOR:
        return tree->asANYMETHOD()->pAttr;

    case NK_ACCESSOR:
        ASSERT(this->isPropertyAccessor || this->isEventAccessor);
        return tree->asACCESSOR()->pAttr;

    case NK_VARDECL:
        {
        // simple event declaration auto-generated accessors.
        ASSERT(this->isEventAccessor);
        BASENODE * fieldTree = parseTree->asVARDECL()->pParent;
        while (fieldTree->kind == NK_LIST) {
            fieldTree = fieldTree->pParent;
        }
        return fieldTree->asANYFIELD()->pAttr;
        }
    default:
        ASSERT(this->isCompilerGeneratedCtor() || this->isIfaceImpl || this->getClass()->isDelegate);
        return 0;
    }
}

/*
 * Given a symbol, returns its element kind for attribute purpose
 */
CorAttributeTargets AGGSYM::getElementKind()
{
    if (this->isInterface)  return catInterface;
    if (this->isEnum)       return catEnum;
    if (this->isClass)      return catClass;
    if (this->isStruct)     return catStruct;
    if (this->isDelegate)   return catDelegate;

    ASSERT(!"Bad aggsym");

    return (CorAttributeTargets) 0;
}

/*
 * Given a symbol, returns its element kind for attribute purpose
 */
CorAttributeTargets SYM::getElementKind()
{
    switch (kind) {
    case SK_METHSYM:
        return (asMETHSYM()->isCtor ? catConstructor : catMethod);

    case SK_PROPSYM:
        return catProperty;

    case SK_MEMBVARSYM:
        return catField;

    case SK_EVENTSYM:
        return catEvent;

    case SK_AGGSYM:
        return asAGGSYM()->getElementKind();

    case SK_GLOBALATTRSYM:
        return this->asGLOBALATTRSYM()->elementKind;

    default:
        ASSERT(!"Bad Symbol type");
    }

    return (CorAttributeTargets) 0;
}

/*
 * returns the corresponding scope for a symbol.
 * for NSDECLSYM's it returns its namespace, for 
 * all others it just returns itself.
 */
PPARENTSYM  PARENTSYM::getScope()
{
    if (kind == SK_NSDECLSYM) {
        return this->asNSDECLSYM()->getScope();
    } else {
        return this;
    }
}

// true for a&b in namespace a.b.c {}
bool NSDECLSYM::isDottedDeclaration()
{
    return  this->parseTree->pName &&
           (this->parseTree->pName->kind == NK_DOT) &&
            this->firstChild && 
           (this->firstChild->kind == SK_NSDECLSYM) &&
           (this->firstChild->asNSDECLSYM()->parseTree == this->parseTree);
}  

/*
 * returns the containing namespace declaration for a type
 */
PNSDECLSYM AGGSYM::getNamespaceDecl() const
{
    PPARENTSYM declaration = this->declaration;
    while (declaration->kind != SK_NSDECLSYM) {
        declaration = declaration->asAGGSYM()->declaration;
    }

    return declaration->asNSDECLSYM();
}

/*
 * returns the assembly id for the declaration of this type
 */
ULONG AGGSYM::getAssemblyIndex() const
{
    return getNamespaceDecl()->getAssemblyIndex();
}

/*
 * returns the assembly id for the declaration of this symbol
 */
ULONG SYM::getAssemblyIndex() const
{
    switch(this->kind) {
    case SK_EXPANDEDPARAMSSYM:
		return const_cast<SYM *>(this)->asEXPANDEDPARAMSSYM()->realMethod->getAssemblyIndex();
    case SK_METHSYM:
    case SK_PROPSYM:
        return const_cast<SYM *>(this)->asMETHPROPSYM()->getAssemblyIndex();
    case SK_MEMBVARSYM:
        return const_cast<SYM *>(this)->asMEMBVARSYM()->getAssemblyIndex();
    case SK_EVENTSYM:
        return const_cast<SYM *>(this)->asEVENTSYM()->getAssemblyIndex();
    case SK_AGGSYM:
        return const_cast<SYM *>(this)->asAGGSYM()->getAssemblyIndex();
    case SK_NSDECLSYM:
        return const_cast<SYM *>(this)->asNSDECLSYM()->getAssemblyIndex();
    case SK_NSSYM:
        // shouldn'e be called with a namespace
    default:
        // should never call this with any other type
        ASSERT(FALSE);
    }

    return 0;
}

/*
 * returns the inputfile where a symbol was declared.
 */
INFILESYM * AGGSYM::getInputFile()
{
    return getNamespaceDecl()->inputfile;
}

/*
 * returns the inputfile scope where a symbol was declared.
 */
DWORD AGGSYM::getImportScope() const
{
    return importScope;
}


/*
 * returns the allowable access modifiers on members of this aggregate symbol
 */
unsigned AGGSYM::allowableMemberAccess()
{
    if (this->isClass) {
        return NF_MOD_ACCESSMODIFIERS;
    } else if (this->isStruct) {
        return NF_MOD_ACCESSMODIFIERS;
    } else {
        ASSERT(this->isInterface);
        return 0;
    }
}

PREDEFATTR      AGGSYM::getPredefAttr() const
{
    if (!this->isPredefined) {
        return PA_COUNT;
    } else {
        return predefTypeInfo[this->iPredef].attr;
    }
}


/*
 * returns the inputfile where a symbol was declared.
 *
 * returns NULL for namespaces because they can be declared
 * in multiple files.
 */
INFILESYM * SYM::getInputFile()
{
    switch (kind)
    {
    case SK_NSSYM:
        // namespaces don't have input files
        // call with a NSDECLSYM instead
        return NULL;

    case SK_NSDECLSYM:
        return this->asNSDECLSYM()->inputfile;

    case SK_AGGSYM:
        return this->asAGGSYM()->getInputFile();

    case SK_MEMBVARSYM:
    case SK_METHSYM:
    case SK_PROPSYM:
    case SK_EVENTSYM:
    case SK_EXPANDEDPARAMSSYM:
        return this->parent->asAGGSYM()->getInputFile();
    case SK_ALIASSYM:
        return this->parent->asNSDECLSYM()->getInputFile();

    case SK_ARRAYSYM:
        return asARRAYSYM()->elementType()->getInputFile();
    case SK_PTRSYM:
        return asPTRSYM()->baseType()->getInputFile();
    case SK_PARAMMODSYM:
        return asPARAMMODSYM()->paramType()->getInputFile();
    case SK_GLOBALATTRSYM:
        return parent->getInputFile();

    case SK_OUTFILESYM:
        return asOUTFILESYM()->firstInfile();

    case SK_NULLSYM:
    case SK_VOIDSYM:
        return NULL;

    default:
        ASSERT(0);
    }

    return NULL;
}

/*
 * returns the inputfile scope where a symbol was declared.
 *
 * returns 0xFFFFFFFF for anything not parented by AGGSYMs (same as above)
 */
DWORD SYM::getImportScope() const
{
    switch (kind)
    {
    case SK_AGGSYM:
        return ((AGGSYM*)this)->getImportScope();

    case SK_MEMBVARSYM:
    case SK_METHSYM:
    case SK_PROPSYM:
    case SK_EVENTSYM:
    case SK_EXPANDEDPARAMSSYM:
        return this->parent->asAGGSYM()->getImportScope();

    default:
        return 0xFFFFFFFF;
    }

    return NULL;
}

/* Returns if the symbol is virtual.
 */
bool SYM::isVirtualSym() 
{
    switch (kind) {
    case SK_METHSYM:
        return this->asMETHSYM()->isVirtual;
    case SK_EVENTSYM:
        return this->asEVENTSYM()->methAdd && this->asEVENTSYM()->methAdd->isVirtual;
    case SK_PROPSYM:
        return (this->asPROPSYM()->methGet && this->asPROPSYM()->methGet->isVirtual) || 
               (this->asPROPSYM()->methSet && this->asPROPSYM()->methSet->isVirtual);
    case SK_EXPANDEDPARAMSSYM:
        return this->asEXPANDEDPARAMSSYM()->realMethod->isVirtualSym();
    default:
        return false;
    }
}

/*
 * returns the containing declaration of a symbol.
 * this will be an AGGSYM for nested types, or
 * a NSDECLSYM for members of a namespace.
 */
PPARENTSYM PARENTSYM::containingDeclaration()
{
    if (kind == SK_AGGSYM) {
        return this->asAGGSYM()->containingDeclaration();
    } else {
        return parent->asPARENTSYM();
    }
}

PPARENTSYM SYM::containingDeclaration()
{
    switch (kind) {
    case SK_NSSYM:
    case SK_NSDECLSYM:
    case SK_AGGSYM:
        return asPARENTSYM()->containingDeclaration();

    case SK_GLOBALATTRSYM:
    case SK_MEMBVARSYM:
    case SK_METHSYM:
    case SK_PROPSYM:
    case SK_EVENTSYM:
    case SK_EXPANDEDPARAMSSYM:
        return parent;

    default:
        ASSERT(!"Bad Sym kind");
    }

    return NULL;
}

/*
 * is this a compiler generated constructor
 */
bool METHSYM::isCompilerGeneratedCtor()
{
    return this->isCtor && (getParseTree()->kind == NK_CLASS || getParseTree()->kind == NK_STRUCT);
}

/*
 * returns property. Only valid to call if isPropertyAccessor is true
 * NOTE: this is only called in an error situation so perf isn't exactly a premium here
 *                                                                                              
 */
PROPSYM *METHSYM::getProperty()
{
    ASSERT(this->isPropertyAccessor);

    //
    // find a property on our containing class with us as an accessor
    //
    FOREACHCHILD(this->getClass(), member)
        if ((member->kind == SK_PROPSYM) && 
            ((member->asPROPSYM()->methGet == this) ||
             (member->asPROPSYM()->methSet == this))) {

            return member->asPROPSYM();
        }

    ENDFOREACHCHILD

    ASSERT(!"property not found");

    return NULL;
}

/*
 * returns true if this property is a get accessor
 */
bool METHSYM::isGetAccessor()
{
    ASSERT(isPropertyAccessor); 
    return (this == getProperty()->methGet); 
}

/*
 * returns event. Only valid to call if isEventAccessor is true
 *                                                                                            
 */
EVENTSYM *METHSYM::getEvent()
{
    ASSERT(this->isEventAccessor);

    //
    // find a property on our containing class with us as an accessor
    //
    FOREACHCHILD(this->getClass(), member)
        if ((member->kind == SK_EVENTSYM) && 
            ((member->asEVENTSYM()->methAdd == this) ||
             (member->asEVENTSYM()->methRemove == this))) {

            return member->asEVENTSYM();
        }

    ENDFOREACHCHILD

    ASSERT(!"event not found");

    return NULL;
}

/*
 * returns event. Only valid to call is isEvent is true.
 */
EVENTSYM *PROPSYM::getEvent(SYMMGR * symmgr)
{
    ASSERT(this->isEvent);

    EVENTSYM * event;
    event = symmgr->LookupGlobalSym(this->name, this->parent, MASK_EVENTSYM)->asEVENTSYM();
    if (event && event->implementation == this)
        return event;
    else 
        return NULL;
}

/*
 * returns event. Only valid to call is isEvent is true.
 */
EVENTSYM *MEMBVARSYM::getEvent(SYMMGR * symmgr)
{
    ASSERT(this->isEvent);

    EVENTSYM * event;
    event = symmgr->LookupGlobalSym(this->name, this->parent, MASK_EVENTSYM)->asEVENTSYM();
    if (event->implementation == this)
        return event;
    else 
        return NULL;
}
/*
 * returns the parse tree for the Ith parameter or NULL
 */
BASENODE* METHSYM::getParamParseTree(int iParam) 
{
    int i = 0;
    ASSERT(iParam >= 0);

    NODELOOP( parseTree->asANYMETHOD()->pParms, PARAMETER, pParam)
        if (i++ >= iParam)
            return pParam;
    ENDLOOP;
    return NULL;
}

/*
 * returns the conditional symbols for the method or what it overrides
 */
NAMELIST * METHSYM::getBaseConditionalSymbols(SYMMGR * symmgr)
{
    if (!isOverride || checkedCondSymbols || conditionalSymbols) {
        return conditionalSymbols;
    }
    checkedCondSymbols = true;
    AGGSYM * cls = this->parent->asAGGSYM()->baseClass;
    while (cls) {
        METHSYM * meth = symmgr->LookupGlobalSym(this->name, cls, MASK_METHSYM)->asMETHSYM();
        while (meth) {
            if (meth->params == this->params && meth->retType == this->retType && !meth->isOverride) {
                return this->conditionalSymbols = meth->conditionalSymbols;
            }
            meth = symmgr->LookupNextSym(meth, cls, MASK_METHSYM)->asMETHSYM();
        }
        cls = cls->baseClass;
    }
    return NULL;
}

/*
 * Given a symbol, return the symbol's first declaration in the sources
 */
SYM * SYM::firstDeclaration()
{
    if (kind == SK_NSSYM) {
        // for namespaces, return their first declaration
        return this->asNSSYM()->firstDeclaration();
    } else {
        //
        // for anything other than namespaces the symbol
        // is the only declaration
        //
        return this;
    }
}


/*
 * Returns the "size" of the aggregate.  The size of non structs is always 1.
 * For structs, the size is the sum of the sizes of every field, with a 
 * minimum size of 1 for a struct of 0 fields.
 */
unsigned short AGGSYM::getFieldsSize()
{
    if (fieldsSize) return fieldsSize;

	if (!isStruct || isEnum || (isPredefined && (predefTypeInfo[iPredef].ft != FT_STRUCT))) {

        fieldsSize = 1; 

    } else {

        unsigned size = 0;
        for (SYM * ps = firstChild; ps; ps = ps->nextChild) {
            if (ps->kind == SK_MEMBVARSYM && !ps->asMEMBVARSYM()->isStatic) {
                if (ps->asMEMBVARSYM()->isBogus || ps->asMEMBVARSYM()->type->kind != SK_AGGSYM) {
                    ps->asMEMBVARSYM()->offset = size ++;
                } else {
                    ps->asMEMBVARSYM()->offset = size;
                    size += ps->asMEMBVARSYM()->type->asAGGSYM()->getFieldsSize();
                }
            }
        }
        // Second part of this if is a highly dubious fix for VS7:118426. If we have > 64K fields, just treat it like
        // an empty struct for definite assignment checking.
        if (!size || (unsigned short) size != size) {
            size = 1;
            isEmptyStruct = true;
        }

        fieldsSize = (unsigned short) size;
    }

    return fieldsSize;
}


void AGGSYM::storeFieldOffsets(unsigned * list)
{
    ASSERT(hasInnerFields());

    unsigned size = 0;
    unsigned count = 0;
    for (SYM * ps = firstChild; ps; ps = ps->nextChild) {
        if (ps->kind == SK_MEMBVARSYM && !ps->asMEMBVARSYM()->isStatic) {
            if (ps->asMEMBVARSYM()->isBogus || ps->asMEMBVARSYM()->type->kind != SK_AGGSYM) {
                list[size++] = count++;
            } else {
                unsigned max = ps->asMEMBVARSYM()->type->asAGGSYM()->getFieldsSize() + size;
                for (;size < max; size ++) {
                    list[size] = count;
                }
                count++;
            }
        }
    }
}

unsigned AGGSYM::hasInnerFields()
{

    if (!isStruct || isEnum) { // || (isPredefined && (predefTypeInfo[iPredef].ft != FT_STRUCT))) {

        return 0;
    }

    getFieldsSize();

    if (!isEmptyStruct) return 1;

    return 0;

}

/*
 * returns the parse node for the type's attributes
 */
BASENODE * AGGSYM::getAttributesNode()
{
    return (isDelegate ? parseTree->asDELEGATE()->pAttr : parseTree->asAGGREGATE()->pAttr);
}


AGGSYM * TYPESYM::commonBase(TYPESYM * type1, TYPESYM * type2)
{
    if (type1->kind != SK_AGGSYM || type2->kind != SK_AGGSYM) return NULL;

    if (type1 == type2) return type1->asAGGSYM();

    int depth1 = 0;
    AGGSYM * base;
    for (base = type1->asAGGSYM(); base; (base = base->baseClass), depth1++);
    int depth2 = 0;
    for (base = type2->asAGGSYM(); base; (base = base->baseClass), depth2++);

    if (depth1 < depth2) {
        depth1 = depth2 - depth1;
        base = type1->asAGGSYM();
        type1 = type2;
        type2 = base;
    } else {
        depth1 -= depth2;
    }

    while (depth1) {
        type1 = type1->asAGGSYM()->baseClass;
        depth1--;
    }

    while (type1) {
        if (type1 == type2) return type1->asAGGSYM();
        type1 = type1->asAGGSYM()->baseClass;
        type2 = type2->asAGGSYM()->baseClass;
    }

    return NULL;
}


unsigned short TYPESYM::getFieldsSize()
{
    if (kind == SK_AGGSYM) return asAGGSYM()->getFieldsSize();
    return 1;
}

/*
 * Given a symbol, determine its fundemental type. This is the type that indicate how the item
 * is stored and what instructions are used to reference if. The fundemental types are:
 *   one of the integral/float types (includes enums with that underlying type)
 *   reference type
 *   struct/value type
 */
FUNDTYPE TYPESYM::fundType()
{

    switch (this->kind) {

    case SK_AGGSYM: {
        AGGSYM * sym = this->asAGGSYM();
    	ASSERT(sym->hasResolvedBaseClasses || (sym->isPredefined && sym->iPredef < PT_OBJECT) );

        // Treat enums like their underlying types.
        if (sym->isEnum) {

            
            sym = sym->underlyingType;
            ASSERT(sym->isStruct);
        }

        if (sym->isStruct) {
            // Struct type could be predefined (int, long, etc.) or some other struct.
            if (sym->isPredefined)
                return predefTypeInfo[sym->iPredef].ft;
            else
                return FT_STRUCT;
        }
        else
            return FT_REF;  // Interfaces, classes, delegates are reference types.
    }

    case SK_ARRAYSYM:
    case SK_NULLSYM:
        return FT_REF;

    case SK_PTRSYM:
        return FT_PTR;

    default:
        return FT_NONE;
    }
}

/*
 * returns true if the type is any struct type
 */
bool     TYPESYM::isAnyStructType()
{
    if (this->kind == SK_AGGSYM) {
        return this->asAGGSYM()->isStruct;
    }
    return false;
}

/* 
 * returns the base of the expression tree for this initializer
 * ie. the entire assignment expression
 */
BASENODE *MEMBVARSYM::getBaseExprTree()
{
    ASSERT(!this->getClass()->isEnum);
    return this->parseTree->asVARDECL()->pArg;
}

/*
 * returns the constant expression tree(after the =) or null
 */
BASENODE *MEMBVARSYM::getConstExprTree()
{
    if (this->getClass()->isEnum) {
        return this->parseTree->asENUMMBR()->pValue;
    } else {
        ASSERT(this->isConst);
        return getBaseExprTree()->asBINOP()->p2;
    }
}

/*
 * returns TRUE if a preprocessor symbol is defined
 */
bool INFILESYM::isSymbolDefined(NAME *symbol)
{
    return !!((CSourceData *)pData)->GetModule ()->IsSymbolDefined(symbol);
}

/*
 * returns true if this symbol is a normal symbol visible to the user
 */
bool SYM::isUserCallable()
{
    switch (kind) {
    case SK_METHSYM:
        return this->asMETHSYM()->isUserCallable();
    case SK_EXPANDEDPARAMSSYM:
        return this->asEXPANDEDPARAMSSYM()->realMethod->isUserCallable();
    default:
        break;
    }

    return true;
}

/*
 * Returns the matching EXF_PARAM flags for a type.  This tells whether this is
 * a ref or an out param...
 */
//int __fastcall TYPESYM::getParamFlags() {
//    if (kind == SK_PARAMMODSYM) {
//        ASSERT(EXF_REFPARAM << 1 == EXF_OUTPARAM);
//        return (EXF_REFPARAM << (int)asPARAMMODSYM()->isOut);
//    } else {
//        return 0;
//    }
//}

/*
 * returns true if this list contains sym
 * NOTE that the this pointer may be NULL
 */
bool SYMLIST::contains(PSYM sym)
{
    FOREACHSYMLIST(this, p)
        if (p == sym) {
            return true;
        }
    ENDFOREACHSYMLIST

    return false;
}


/* 
 * Constructor.
 */
SYMTBL::SYMTBL(COMPILER * comp, unsigned log2Buckets)
{
    compiler = comp;
    buckets = NULL;
    cSyms = cBuckets = 0;
    bucketShift = log2Buckets;
}


/* 
 * Destructor 
 */
SYMTBL::~SYMTBL()
{
    Term();
}

/*
 * Hash a name and parent to get a bucket number and jump amount. The jump
 * amount must be odd.
 */
__forceinline unsigned SYMTBL::Bucket(PNAME name, PPARENTSYM parent, unsigned * pjump)
{
    unsigned hash, iBucket, jump;

    // Create a hash value from the name and parent of the symbol.
    hash = HashUInt((UINT_PTR)name) + HashUInt((UINT_PTR)parent);

    // Get bucket and jump amount. Jump amount must be odd so that
    // all symbols in the table are hit.
    iBucket = hash & bucketMask;
    jump = hash >> bucketShift;
    while (!(jump & 1)) {
        jump >>= 1;
        if (jump == 0)
            jump = 1;
    }

    *pjump = jump;
    return iBucket;
}

/*
 * Helper routine to place a child into the hash table. This hash table is
 * organized to enable quick resolving of mapping a (name, parent) pair
 * to a child symbol. If multiple child symbols of a parent have the same
 * name, they are all chained together, so we need only find the first one.
 * We place children in a hash table with the hash function a hashing of the 
 * NAME and PARENT addresses. We use double hashing to handle collisions
 * (the second hash providing the "jump"), and double the size of the table
 * when it becomes 75% full.
 */
void SYMTBL::InsertChildNoGrow(PSYM child)
{
    PNAME name = child->name;
    PPARENTSYM parent = child->parent;
    unsigned iBucket, jump;
    PSYM sym;

    // Get bucket and jump amount.
    iBucket = Bucket(name, parent, &jump);

    // Search the table until the correct name or an empty bucket is found.
    while ((sym = buckets[iBucket])) {
        if (sym->name == name && sym->parent == parent) {
            // Link onto the end of the symbol chain here.
            while (sym->nextSameName)
                sym = sym->nextSameName;
            sym->nextSameName = child;
            return;
        }

        iBucket = (iBucket + jump) & bucketMask;
    }

    // No bucket has this parent/name pair.
    buckets[iBucket] = child;
}

/* 
 * Add a named symbol to a parent scope, for later lookup.
 */
void SYMTBL::InsertChild(PPARENTSYM parent, PSYM child)
{
    ASSERT(child->nextSameName == NULL);
    child->parent = parent;

    // Is the table more than 3/4 full? Grow the table if so.
    if (++cSyms * 4 > cBuckets * 3) {
        GrowTable();
    }

    // Place the child into the hash table.
    InsertChildNoGrow(child);
}


/* 
 * Look up a symbol by name and parent, filtering by mask.
 */
PSYM SYMTBL::LookupSym(PNAME name, PPARENTSYM parent, unsigned kindmask)
{
    unsigned iBucket, jump;
    PSYM sym;

    if (!buckets)
        return NULL;        // No bucket table yet.

    // Get bucket and jump amount.
    iBucket = Bucket(name, parent, &jump);

    // Search the table until the correct name or an empty bucket is found.
    while ((sym = buckets[iBucket])) {
        if (sym->name == name && sym->parent == parent) {
            do {
                if (kindmask & (1 << sym->kind))
                    return sym;
                sym = sym->nextSameName;
            } while (sym);

            return NULL;
        }

        iBucket = (iBucket + jump) & bucketMask;
    }

    return NULL;
}


/* 
 * Clear the table of symbols.
 */
void SYMTBL::Clear()
{
    // Clear the table.
    memset(buckets, 0, cBuckets * sizeof(PSYM));
    cSyms = 0;
}


/*
 * Terminate the table. Free the bucket array.
 */
void SYMTBL::Term()
{
    // Free the bucket table.
    if (buckets) {
        size_t cb = cBuckets * sizeof(PSYM);
        if (cb < PAGEHEAP::pageSize)
            cb = PAGEHEAP::pageSize;
        compiler->pageheap.FreePages(buckets, cb);
        buckets = NULL;
    }
}

/* 
 * Grow the table. This could either be the initial allocation of
 * the table, or a doubling in size.
 */
void SYMTBL::GrowTable()
{
    PSYM * bucketsPrev;        // the buckets of the old hash table.
    unsigned cBucketsPrev = 0; // number of buckets in old table
    size_t cb;

    // Hold onto the old table.
    bucketsPrev = buckets;
    if (buckets) {
        cBucketsPrev = cBuckets;
        ++bucketShift;              // double size.
    }

    // Allocate a new empty table of size 2^bucketShift
    cBuckets = (1 << bucketShift);
    bucketMask = cBuckets - 1;
    cb = cBuckets * sizeof(PSYM);
    if (cb < PAGEHEAP::pageSize)
        cb = PAGEHEAP::pageSize;
    buckets = (PSYM *) compiler->pageheap.AllocPages(cb);
    if (!buckets)
        compiler->NoMemory();  // won't return.
    memset(buckets, 0, cb);

    // Redistribute the old table into the new table.
    if (bucketsPrev) {
        // Re-add the symbols to the new table.
        for (unsigned i = 0; i < cBucketsPrev; ++i) {
            PSYM sym;
            if ((sym = bucketsPrev[i]))
                InsertChildNoGrow(sym);
        }

        // Free the old table.
        cb = cBucketsPrev * sizeof(PSYM);
        if (cb < PAGEHEAP::pageSize)
            cb = PAGEHEAP::pageSize;
        compiler->pageheap.FreePages(bucketsPrev, cb);
    }
}


/*
 * Constructor.
 */
SYMMGR::SYMMGR(NRHEAP * allocGlobal, NRHEAP * allocLocal)
: tableGlobal(compiler(), 13),  // Initial global size: 8192 buckets.
  tableLocal(compiler(), 7)     // Initial local size: 128 buckets.
{
    this->allocGlobal = allocGlobal;
    this->allocLocal = allocLocal;

    // Init the type array hash table.
    buckets = NULL;
    cBuckets = 0;
    bucketMask = 0;
    cTypeArrays = 0;

    voidSym = NULL;
    nullType = NULL;
    errorSym = NULL;
    arglistSym = NULL;
	naturalIntSym = NULL;
    rootNS = NULL;
    fileroot = NULL;
    mdfileroot = NULL;
    attrroot = NULL;
    deletedtypes = NULL;
    xmlfileroot = NULL;
    for (int i = 0; i < PA_COUNT; i++) {
        predefinedAttributes[i] = NULL;
    }
#ifdef DEBUG
    predefInputFile = NULL;
#endif

#ifdef TRACKMEM
    for (int i = 0; i < SK_COUNT; i++)
        m_iLocalSymCount[i] = m_iGlobalSymCount[i] = 0;
#endif
}


/* 
 * Destructor
 */
SYMMGR::~SYMMGR()
{
    Term();
}

/*
 * returns a constant value of zero for a predefined type
 */
CONSTVAL SYMMGR::GetPredefZero(PREDEFTYPE pt)
{
    ASSERT(pt >= 0 && pt < PT_COUNT);
    return predefTypeInfo[pt].zero;
}

const static struct PREDEFATTRINITIALIZER
{
    PREDEFATTR      pa;
    PREDEFNAME      pn;
    PREDEFTYPE      pt;
} attrInitializers[] = {
#define PREDEFATTRDEF(id,name,iPredef, validOn) {id,name, iPredef},
#include "predefattr.h"
#undef PREDEFATTRDEF
};

/* 
 * Initialize a bunch of pre-defined symbols and such.
 */
void SYMMGR::Init()
{
    // 'void' and 'null' are special types with their own symbol kind.
    voidSym = CreateGlobalSym(SK_VOIDSYM, NULL, NULL)->asVOIDSYM();
    nullType = CreateGlobalSym(SK_NULLSYM, NULL, NULL)->asNULLSYM();

    // Both should be considered public.
    voidSym->access = ACC_PUBLIC;
    nullType->access = ACC_PUBLIC;

    // create a generic error symbol:
    errorSym = CreateGlobalSym(SK_ERRORSYM, NULL, NULL)->asERRORSYM();
    errorSym->isPrepared = true;

    // create the varargs type symbol:
    arglistSym = CreateGlobalSym(SK_AGGSYM, compiler()->namemgr->GetPredefName(PN_ARGLIST), NULL)->asTYPESYM();
    arglistSym->asAGGSYM()->hasResolvedBaseClasses = true;
    arglistSym->isPrepared = true;
    arglistSym->asAGGSYM()->isSealed = true;
    arglistSym->access = ACC_PUBLIC;
    arglistSym->asAGGSYM()->isDefined = true;

    // create the natural int type symbol:
    naturalIntSym = CreateGlobalSym(SK_AGGSYM, compiler()->namemgr->GetPredefName(PN_NATURALINT), NULL)->asTYPESYM();
    naturalIntSym->asAGGSYM()->hasResolvedBaseClasses = true;
    naturalIntSym->isPrepared = true;
    naturalIntSym->asAGGSYM()->isSealed = true;
    naturalIntSym->access = ACC_PUBLIC;
    naturalIntSym->asAGGSYM()->isDefined = true;

	// Some root symbols.
    rootNS = CreateNamespace(compiler()->namemgr->AddString(L""), NULL);  // Root namespace
    fileroot = CreateGlobalSym(SK_SCOPESYM, NULL, NULL)->asSCOPESYM();  // Root of file symbols.
    mdfileroot = CreateOutFile(L"", false, false, NULL, NULL, NULL, false);
    attrroot = CreateGlobalSym(SK_SCOPESYM, NULL, NULL)->asSCOPESYM();  // Root of predefined attribute symbols.
    deletedtypes = CreateGlobalSym(SK_SCOPESYM, NULL, NULL)->asSCOPESYM();
    xmlfileroot = CreateGlobalSym(SK_SCOPESYM, NULL, NULL)->asSCOPESYM();  // Root of predefined included XML files

    // initialize predefined attributes table
    int i = 0;
    for (const PREDEFATTRINITIALIZER *p = attrInitializers; p < (attrInitializers + PA_COUNT); i++, p++) {
        NAME *name;
        if (p->pn !=PN_COUNT) {
            name = compiler()->namemgr->GetPredefName(p->pn);
        } else {
            name = 0;
        }

        PREDEFATTRSYM *attr = CreateGlobalSym(SK_PREDEFATTRSYM, name, attrroot)->asPREDEFATTRSYM();
        attr->attr = p->pa;
        attr->iPredef = p->pt;
        predefinedAttributes[i] = attr;
    }
}

/*
 * Initialized the predefined types. At this point all types have been
 * declared, so if we don't find a declaration or import of a predefined
 * types we aren't going to get very far.
 */
void SYMMGR::InitPredefinedTypes()
{
    PAGGSYM sym;

    predefSyms = (PAGGSYM *) allocGlobal->Alloc(sizeof(PAGGSYM) * PT_COUNT);
    for (int i = 0; i < PT_COUNT; ++i) {

        sym = FindPredefinedType(predefTypeInfo[i].fullName, predefTypeInfo[i].isRequired);
        if (sym) {
            // If the user specified this type in source, make sure he did it correctly.
            if (sym->getInputFile()->isSource) {
                // isClass, isStruct, isInterface should already be set correctly.
                if (sym->isClass != predefTypeInfo[i].isClass)
                    compiler()->clsDeclRec.errorSymbol(sym, ERR_PredefinedTypeBadType);
                if (sym->isStruct != predefTypeInfo[i].isStruct)
                    compiler()->clsDeclRec.errorSymbol(sym, ERR_PredefinedTypeBadType);
                if (sym->isInterface != predefTypeInfo[i].isInterface)
                    compiler()->clsDeclRec.errorSymbol(sym, ERR_PredefinedTypeBadType);
                if (sym->isDelegate != predefTypeInfo[i].isDelegate)
                    compiler()->clsDeclRec.errorSymbol(sym, ERR_PredefinedTypeBadType);
            }
            else {
                sym->isClass     = predefTypeInfo[i].isClass ;
                sym->isStruct    = predefTypeInfo[i].isStruct ;
                sym->isInterface = predefTypeInfo[i].isInterface ;
                sym->isDelegate  = predefTypeInfo[i].isDelegate ;
            }

            sym->isPredefined    = true;
            sym->iPredef         = i;
            ASSERT(sym->iPredef == (unsigned)i);  // Assert that the bitfield is large enough.

        }
        else {
            if (predefTypeInfo[i].isRequired)
                return;     // Don't bother continuing -- we're dead in the water.
        }

        predefSyms[i] = sym;
    }

    // set up the root of the attribute hierarchy
    sym = GetPredefType(PT_ATTRIBUTE, false);
    if (sym) sym->isAttribute = true;

    // need to set this up because the attribute class has the attribute usage attribute on it
    sym = GetPredefType(PT_ATTRIBUTEUSAGE, false);
    if (sym) sym->attributeClass = catClass;
    if (sym) sym->isAttribute = true;
    sym = GetPredefType(PT_ATTRIBUTETARGETS, false);
    if (sym) sym->isEnum = true;

    sym = GetPredefType(PT_CONDITIONAL, false);
    if (sym) sym->isAttribute = true;
    if (sym) sym->attributeClass = catMethod;

    sym = GetPredefType(PT_OBSOLETE, false);
    if (sym) sym->isAttribute = true;
    if (sym) sym->attributeClass = catAll;

    sym = GetPredefType(PT_CLSCOMPLIANT, false);
    if (sym) sym->isAttribute = true;
    if (sym) sym->attributeClass = (CorAttributeTargets)(catAssembly | catClassMembers); // catAll & ~catModule & ~catParameter & ~catReturnValue

    // set up the root of the security attribute hierarchy
    sym = GetPredefType(PT_SECURITYATTRIBUTE, false);
    if (sym) sym->isSecurityAttribute = true;

    // set up the root of for marshalbyref types.
    sym = GetPredefType(PT_MARSHALBYREF, false);
    if (sym) sym->isMarshalByRef = true;
    sym = GetPredefType(PT_CONTEXTBOUND, false);
    if (sym) sym->isMarshalByRef = true;
}

/*
 * finds an existing declaration for a predefined type
 * returns NULL on failure. If isRequired is true, an error message is also given.
 */
AGGSYM * SYMMGR::FindPredefinedType(LPCWSTR typeName, bool isRequired)
{
    LPCWSTR current, p;
    SYM *currentScope = rootNS;
    PNAME name;

    current = typeName;
    while (true) {
        //
        // find next name component
        //
        p = current;
        while (*p && *p != L'.') {
            p++;
        }
        name = compiler()->namemgr->AddString(current, (int)(p - current));

        currentScope = LookupGlobalSym(name, currentScope->asPARENTSYM(), MASK_ALL);
        if (!currentScope) {
            // didn't find a predefined symbol
            if (isRequired)
                compiler()->Error(NULL, ERR_PredefinedTypeNotFound, typeName);
            return NULL;
        } else if (*p) {
            // we have more components to this name
            // we must be looking for a namespace
            if (currentScope->kind != SK_NSSYM) {
                if (isRequired)
                    compiler()->clsDeclRec.errorSymbol(currentScope, ERR_PredefinedTypeBadNamespace);

                return NULL;
            }
        } else {
            // we have the last name component
            // we must be looking for a type
            if (currentScope->kind != SK_AGGSYM) {
                if (isRequired)
                    compiler()->clsDeclRec.errorSymbol(currentScope, ERR_PredefinedTypeBadType);
                return NULL;
            }
        }

        if (!*p) break;

        current = p + 1;
    }

    return currentScope->asAGGSYM();
}

/*
 * Free all memory associated with the symbol manager.
 */
void SYMMGR::Term()
{
    tableLocal.Term();
    tableGlobal.Term();

    // Free the typearray table.
    if (buckets) {
        size_t cb = cBuckets * sizeof(PTYPEARRAY);
        compiler()->pageheap.FreePages(buckets, cb);
        buckets = NULL;
    }
}


/*
 * Clear the local table in preperation for the next fuction
 */
void SYMMGR::DestroyLocalSymbols()
{
    tableLocal.Clear();
}

/*
 * Helper routine to allocator a symbol structor from the given heap,
 * and assign the given name (if appropriate). This code is kind of
 * repetitive, but its hard to do it in a generic way with the new
 * operator. It's fast, which is most important.
 */
PSYM SYMMGR::AllocSym(SYMKIND symkind, PNAME name, NRHEAP * allocator)
{
    PSYM sym;
    switch (symkind) {
    case SK_NSSYM: 
    {
        sym = new(allocator) NSSYM;
        sym->name = name;
        break;
    }
    case SK_NSDECLSYM: 
    {
        sym = new(allocator) NSDECLSYM;
        sym->name = name;
        break;
    }
    case SK_AGGSYM:
    {
        sym = new(allocator) AGGSYM;
        sym->name = name;
        break;
    }
    case SK_ALIASSYM:
    {
        sym = new(allocator) ALIASSYM;
        sym->name = name;
        break;
    }
    case SK_GLOBALATTRSYM:
    {
        sym = new(allocator) GLOBALATTRSYM;
        sym->name = name;
        break;
    }
    case SK_MEMBVARSYM:
    {
        sym = new(allocator) MEMBVARSYM;
        sym->name = name;
        break;
    }
    case SK_LOCVARSYM:
    {
        sym = new(allocator) LOCVARSYM;
        sym->name = name;
        break;
    }
    case SK_FAKEMETHSYM:
    {
        sym = new(allocator) FAKEMETHSYM;
        sym->name = name;
        break;
    }
    case SK_METHSYM:
    {
        sym = new(allocator) METHSYM;
        sym->name = name;
        break;
    }
    case SK_IFACEIMPLMETHSYM:
    {
        sym = new(allocator) IFACEIMPLMETHSYM;
        sym->name = name;
        break;
    }
    case SK_EXPANDEDPARAMSSYM:
    {
        sym = new(allocator) EXPANDEDPARAMSSYM;
        sym->name = name;
        break;
    }
    case SK_FAKEPROPSYM:
    {
        sym = new(allocator) FAKEPROPSYM;
        sym->name = name;
        break;
    }
    case SK_PROPSYM:
    {
        sym = new(allocator) PROPSYM;
        sym->name = name;
        break;
    }
    case SK_SCOPESYM:
    {
        sym = new(allocator) SCOPESYM;
        sym->name = name;
        break;
    }
    case SK_INFILESYM:
    {
        sym = new(allocator) INFILESYM;
        sym->name = name;
        break;
    }
    case SK_RESFILESYM:
    {
        sym = new(allocator) RESFILESYM;
        sym->name = name;
        break;
    }
    case SK_OUTFILESYM:
    {
        sym = new(allocator) OUTFILESYM;
        sym->name = name;
        break;
    }
    case SK_ARRAYSYM:
    {
        sym = new(allocator) ARRAYSYM;
        sym->name = name;
        break;
    }
    case SK_PTRSYM:
    {
        sym = new(allocator) PTRSYM;
        sym->name = name;
        break;
    }
    case SK_PINNEDSYM:
    {
        sym = new(allocator) PINNEDSYM;
        sym->name = name;
        break;
    }
    case SK_PARAMMODSYM:
    {
        sym = new(allocator) PARAMMODSYM;
        sym->name = name;
        break;
    }
    case SK_ERRORSYM:
    {
        sym = new(allocator) ERRORSYM;
        break;
    }
    case SK_NULLSYM:
    {
        sym = new(allocator) NULLSYM;
        break;
    }
    case SK_VOIDSYM:
    {
        sym = new(allocator) VOIDSYM;
        break;
    }
    case SK_CACHESYM:
    {
        sym = new(allocator) CACHESYM;
        sym->name = name;
        break;
    }
    case SK_LABELSYM:
    {
        sym = new(allocator) LABELSYM;
        sym->name = name;
        break;
    }
    case SK_INDEXERSYM:
    {
        sym = new(allocator) INDEXERSYM;
        sym->name = name;
        break;
    }
    case SK_PREDEFATTRSYM:
    {
        sym = new(allocator) PREDEFATTRSYM;
        sym->name = name;
        break;
    }
    case SK_DELETEDTYPESYM:
    {
        sym = new(allocator) DELETEDTYPESYM;
        sym->name = name;
        break;
    }
    case SK_EVENTSYM:
    {
        sym = new(allocator) EVENTSYM;
        sym->name = name;
        break;
    }
    case SK_SYNTHINFILESYM:
    {
        sym = new(allocator) INFILESYM;
        sym->name = name;
        break;
    }

    default:
        // Illegal symbol kind. This shouldn't happen.
        ASSERT(0);
        return NULL;
    }

    sym->kind = symkind;
#ifdef TRACKMEM
    if (allocator == allocLocal)
        m_iLocalSymCount[symkind]++;
    else {
        ASSERT (allocator == allocGlobal);
        m_iGlobalSymCount[symkind]++;
    }
#endif

    return sym;

}


/* 
 * Add a named symbol to a parent scope, for later lookup.
 */
void SYMMGR::AddChild(SYMTBL *table, PPARENTSYM parent, PSYM child)
{
    ASSERT(child->nextSameName == NULL);

    // Link into the parent's child list at the end.
    if (parent->lastChild) {
        parent->lastChild->nextChild = child;
        parent->lastChild = child;
    }
    else {
        parent->firstChild = parent->lastChild = child;
    }

    table->InsertChild(parent, child);
}

/* 
 * The main routine for creating a global symbol and putting it into the
 * symbol table under a particular parent. Either name or parent can
 * be NULL.
 */
PSYM SYMMGR::CreateGlobalSym(SYMKIND symkind, PNAME name, PPARENTSYM parent)
{
    PSYM sym;

    // Only some symbol kinds are valid as global symbols. Validate.
    ASSERT(symkind == SK_MEMBVARSYM || symkind == SK_METHSYM ||
           symkind == SK_AGGSYM     || symkind == SK_NSSYM ||
           symkind == SK_ERRORSYM   || symkind == SK_SCOPESYM ||
           symkind == SK_INFILESYM  || symkind == SK_OUTFILESYM ||
           symkind == SK_VOIDSYM    || symkind == SK_NULLSYM ||
           symkind == SK_ARRAYSYM   || symkind == SK_PTRSYM ||
           symkind == SK_PARAMMODSYM || symkind == SK_PROPSYM ||
           symkind == SK_FAKEPROPSYM || symkind == SK_FAKEMETHSYM ||
           symkind == SK_EXPANDEDPARAMSSYM || symkind == SK_ALIASSYM ||
           symkind == SK_INDEXERSYM || symkind == SK_PREDEFATTRSYM ||
           symkind == SK_RESFILESYM || symkind == SK_EVENTSYM ||
           symkind == SK_GLOBALATTRSYM || symkind == SK_PINNEDSYM ||
           symkind == SK_IFACEIMPLMETHSYM || symkind == SK_XMLFILESYM ||
           symkind == SK_SYNTHINFILESYM);

    // Allocate the symbol from the global allocator and fill in the name member.
    sym = AllocSym(symkind, name, allocGlobal);

    if (parent) {
        // Set the parent element of the child symbol.
        AddChild(&tableGlobal, parent, sym);
    }

    return sym;
}

/* 
 * The main routine for creating a local symbol and putting it into the
 * symbol table under a particular parent. Either name or parent can
 * be NULL.
 */
PSYM SYMMGR::CreateLocalSym(SYMKIND symkind, PNAME name, PPARENTSYM parent)
{
    PSYM sym;

    // Only some symbol kinds are valid as local symbols. Validate.
    ASSERT(symkind == SK_LOCVARSYM || symkind == SK_ERRORSYM ||
           symkind == SK_SCOPESYM || symkind == SK_CACHESYM || symkind == SK_LABELSYM);

    // Allocate the symbol from the local allocator and fill in the name member.
    sym = AllocSym(symkind, name, allocLocal);

    if (parent) {
        // Set the parent element of the child symbol.
        AddChild(&tableLocal, parent, sym);
    }

    return sym;
}


/*
 * The main routine for looking up a global symbol by name. It's possible for
 * there to be more that one symbol with a particular name in a particular
 * parent; if you want to check for more, then use LookupNextSym.
 *
 * kindmask filters the result by particular symbol kinds.
 *
 * returns NULL if no match found.
 */
PSYM SYMMGR::LookupGlobalSym(PNAME name, PPARENTSYM parent, unsigned kindmask)
{
    return tableGlobal.LookupSym(name, parent, kindmask);
}


/*
 * The main routine for looking up a local symbol by name. It's possible for
 * there to be more than one symbol with a particular name in a particular
 * parent; if you want to check for more, then use LookupNextSym.
 *
 * kindmask filters the result by particular symbol kinds.
 *
 * returns NULL if no match found.
 */
PSYM SYMMGR::LookupLocalSym(PNAME name, PPARENTSYM parent, unsigned kindmask)
{
    ASSERT(name);       // name can't be NULL.

    return tableLocal.LookupSym(name, parent, kindmask);
}


/*
 * Look up the next symbol with the same name and parent.
 */
PSYM SYMMGR::LookupNextSym(PSYM sym, PPARENTSYM parent, unsigned kindmask)
{
    ASSERT(sym->parent == parent);

    sym = sym->nextSameName;
    ASSERT(!sym || sym->parent == parent);

    // Keep traversing the list of symbols with same name and parent.
    while (sym) {
        if (kindmask & (1 << sym->kind))
            return sym;

        sym = sym->nextSameName;
        ASSERT(!sym || sym->parent == parent);
    }
    return NULL;
}



/*
 * Specific creation functions.
 * These functions create specific kinds of symbols.
 */


/*
 * Create a namespace symbol.
 */
PNSSYM SYMMGR::CreateNamespace(PNAME name, PNSSYM parent)
{
    //
    // duplicate checking is done elsewhere
    //
    ASSERT(LookupGlobalSym(name, parent, MASK_ALL) == NULL);

    //
    // create and initialize the symbol.
    //
    // NOTE: we don't use CreateGlobalSym() because we don't want
    //       to link the new SYM into its parent's list of children
    //
    NSSYM *ns = AllocSym(SK_NSSYM, name, allocGlobal)->asNSSYM();
    ns->access = ACC_PUBLIC;    // namespaces all have public access
    if (parent) {
        tableGlobal.InsertChild(parent, ns);
    }

    return ns;
}

/*
 * Create a namespace declaration symbol.
 *
 * nspace is the namespace for the declaration to be created.
 * parent is the containing declaration for the new declaration.
 * parseTree is the parseTree fro the new declaration.
 */
NSDECLSYM * SYMMGR::CreateNamespaceDeclaration(PNSSYM nspace, NSDECLSYM * parent, PINFILESYM inputfile, NAMESPACENODE * parseTree)
{
    ASSERT(inputfile);
    // input file must match parent's input file if we have a parent
    ASSERT((parent == NULL) || (inputfile && (parent->inputfile == inputfile)));
    // namespace parent must match declaration's container
    ASSERT((parent && parent->namespaceSymbol && (parent->namespaceSymbol == nspace->parent)) ||
           (!parent && !nspace->parent && (nspace == GetRootNS())));

    //
    // create and initialize the symbol.
    //
    // NOTE: we don't use CreateGlobalSym() because we don't want
    //       to insert the new SYM into the global lookup table.
    //       This requires us to do a bunch of stuff manually.
    //
    NSDECLSYM *declaration = AllocSym(SK_NSDECLSYM, NULL, allocGlobal)->asNSDECLSYM();
    declaration->inputfile = inputfile;
    declaration->namespaceSymbol = nspace;
    declaration->parseTree = parseTree;

    //
    // Link into the parent's child list at the end.
    //
    declaration->parent = parent;
    if (parent) {
        if (parent->lastChild) {
            ASSERT(parent->lastChild);
            ASSERT(!parent->lastChild->nextChild);

            parent->lastChild->nextChild = declaration;
            parent->lastChild = declaration;
        }
        else {
            ASSERT(!parent->lastChild);
            parent->firstChild = parent->lastChild = declaration;
        }
    }

    //
    // link us into our namespace's list of declarations
    //
    if (nspace->firstChild) {
        ASSERT(nspace->lastDeclaration());
        ASSERT(!nspace->lastDeclaration()->nextDeclaration);
        ASSERT(nspace->lastDeclaration()->namespaceSymbol == nspace);

        nspace->lastDeclaration()->nextDeclaration = declaration;
        nspace->lastChild = declaration;
    } else {
        ASSERT(!nspace->lastChild);

        nspace->firstChild = nspace->lastChild = declaration;
    }

    //
    // Set some bits on the namespace
    if (inputfile->isSource)
        nspace->isDefinedInSource = true;

    return declaration;
}


/*
 * Create a symbol for an aggregate type: class, struct, interface, or enum.
 * It is assumed the caller has already checked to make sure there is no
 * conflicting name.
 */
PAGGSYM SYMMGR::CreateAggregate(PNAME name, PPARENTSYM parent)
{

    // Only allow parents to be namespace declarations or
    // classes.
    ASSERT(parent->kind == SK_NSDECLSYM || (parent->kind == SK_AGGSYM));

    PAGGSYM newAggregate;

    if (parent->kind == SK_AGGSYM) {
        // Better not be a conflict.
        ASSERT(LookupGlobalSym(name, parent, MASK_ALL) == NULL);

        // Create the new symbol.
        newAggregate = CreateGlobalSym(SK_AGGSYM, name, parent)->asAGGSYM();

    } else {
        //
        // create and initialize the symbol
        //
        newAggregate = AllocSym(SK_AGGSYM, name, allocGlobal)->asAGGSYM();

        //
        // set the parent correctly for lookups
        //
        PPARENTSYM parentScope = parent->getScope();

        //
        // Link into the parent's child list at the end.
        //
        if (parent->lastChild) {
            parent->lastChild->nextChild = newAggregate;
            parent->lastChild = newAggregate;
        }
        else {
            parent->firstChild = parent->lastChild = newAggregate;
        }

        //
        // link us into the symbol lookup
        //
        // Better not be a conflict.
        ASSERT(LookupGlobalSym(name, parentScope, MASK_ALL) == NULL);
        tableGlobal.InsertChild(parentScope, newAggregate);

    }

    parent->getInputFile()->cTypes++;
    parent->getInputFile()->getOutputFile()->cTypes++;

    newAggregate->declaration = parent;

    return newAggregate;
}

/*
 * Create a symbol for a using alias clause.
 */
PALIASSYM SYMMGR::CreateAlias(PNAME name)
{
    // Create the new symbol.
    return CreateGlobalSym(SK_ALIASSYM, name, NULL)->asALIASSYM();
}


/*
 * Create a symbol for an member variable.
 */
PGLOBALATTRSYM SYMMGR::CreateGlobalAttribute(PNAME name, NSDECLSYM *parent)
{
    // Create the new symbol.
    return CreateGlobalSym(SK_GLOBALATTRSYM, name, parent)->asGLOBALATTRSYM();
}


/*
 * Create a symbol for an member variable.
 */
PMEMBVARSYM SYMMGR::CreateMembVar(PNAME name, PAGGSYM parent)
{
    // Create the new symbol.
    return CreateGlobalSym(SK_MEMBVARSYM, name, parent)->asMEMBVARSYM();
}


/*
 * Create a symbol for an method. Does not check for existing symbols
 * because methods are assumed to be overloadable.
 */
PMETHSYM SYMMGR::CreateMethod(PNAME name, PAGGSYM parent)
{
    // Create the new symbol.
    return CreateGlobalSym(SK_METHSYM, name, parent)->asMETHSYM();
}

PIFACEIMPLMETHSYM SYMMGR::CreateIfaceImplMethod(PAGGSYM parent)
{
    IFACEIMPLMETHSYM *sym = (IFACEIMPLMETHSYM*) CreateGlobalSym(SK_IFACEIMPLMETHSYM, NULL, parent);
    sym->kind = SK_METHSYM; // these syms really want to be methods + a little bit
    sym->isIfaceImpl = true;
    return sym;
}

/* 
 * Create a symbol for an property. Does not check for existing symbols
 * because properties are assumed to be overloadable.
 */
PPROPSYM SYMMGR::CreateProperty(PNAME name, PAGGSYM parent)
{
    // Create the new symbol.
    return CreateGlobalSym(SK_PROPSYM, name, parent)->asPROPSYM();
}


/*
 * Create a symbol for an property. Does not check for existing symbols
 * because properties are assumed to be overloadable.
 */
PINDEXERSYM SYMMGR::CreateIndexer(PNAME name, PAGGSYM parent)
{
    // Create the new symbol.
    PINDEXERSYM indexer = (PINDEXERSYM) CreateGlobalSym(SK_INDEXERSYM, name ? compiler()->namemgr->GetPredefName(PN_INDEXERINTERNAL) : NULL, parent);
    indexer->realName = name;
    indexer->kind = SK_PROPSYM;
    indexer->isOperator = true;

    return indexer;
}

/*
 * Create an event symbol. Does not check for existing symbols.
 */
PEVENTSYM SYMMGR::CreateEvent(PNAME name, PAGGSYM parent)
{
    return CreateGlobalSym(SK_EVENTSYM, name, parent)->asEVENTSYM();
}



/*
 * Handling "derived types".
 *
 * There are a set of types that are "derived" types, in that they
 * are derived from standard types. Pointer types and array types are
 * the obvious examples. We don't want to allocate new symbols for these
 * every time we come across them. We handle these by using the parent
 * lookup mechanism do the work for us, we use the "base" or "element" type of the
 * derived type being the parent type, with a special weird name for looking
 * up the derived type by.
 */


/*
 * Create or return an existing array symbol. We use the lookup mechanism
 * to find unique array symbols efficiently. The parent of an array symbol
 * is the element type, and the name is "[X<n+1>", where the second character
 * has the rank.
 */

PARRAYSYM SYMMGR::GetArray(PTYPESYM elementType, int args)
{
    WCHAR nameString[4];
    PNAME name;
    ARRAYSYM *sym;

    ASSERT(args > 0 && args < 32767);

    switch (args) {
    case 1: case 2:
        name = compiler()->namemgr->GetPredefName((PREDEFNAME)(PN_ARRAY0 + args));
        break;
    default:
        nameString[0] = L'[';
        nameString[1] = L'X';
        nameString[2] = args + 1;
        nameString[3] = L'\0';
        name = compiler()->namemgr->AddString(nameString);
    }

    // See if we already have a array symbol of this element type and rank.
    sym = LookupGlobalSym(name, elementType, MASK_ARRAYSYM)->asARRAYSYM();
    if (! sym) {
        // No existing array symbol. Create a new one.
        sym = CreateGlobalSym(SK_ARRAYSYM, name, elementType)->asARRAYSYM();
        sym->rank = args;
    }

    ASSERT(sym->rank == args);
    ASSERT(sym->elementType() == elementType);

    return sym;
}

/*
 * Create or return an existing pointer symbol. The parent of an array symbol
 * is the base type, and the name is "*"
 */
PPTRSYM SYMMGR::GetPtrType(PTYPESYM baseType)
{
    PPTRSYM sym;
    PNAME namePtr = compiler()->namemgr->GetPredefName(PN_PTR);

    // See if we already have a pointer symbol of this base type.
    sym = LookupGlobalSym(namePtr, baseType, MASK_PTRSYM)->asPTRSYM();
    if (! sym) {
        // No existing array symbol. Create a new one.
        sym = CreateGlobalSym(SK_PTRSYM, namePtr, baseType)->asPTRSYM();
    }

    ASSERT(sym->baseType() == baseType);

    return sym;
}


/*
 * Create or return an existing pinned symbol. The parent of a pinned symbol
 * is the base type, and the name is "@"
 */
PPINNEDSYM SYMMGR::GetPinnedType(PTYPESYM baseType)
{
    PPINNEDSYM sym;
    PNAME namePinned = compiler()->namemgr->GetPredefName(PN_PINNED);

    // See if we already have a pointer symbol of this base type.
    sym = LookupGlobalSym(namePinned, baseType, MASK_PINNEDSYM)->asPINNEDSYM();
    if (! sym) {
        // No existing array symbol. Create a new one.
        sym = CreateGlobalSym(SK_PINNEDSYM, namePinned, baseType)->asPINNEDSYM();
    }

    ASSERT(sym->baseType() == baseType);

    return sym;
}

/*
 * Create or return an param modifier symbol. This symbol represents the
 * type of a ref or out param.
 */
PPARAMMODSYM SYMMGR::GetParamModifier(PTYPESYM paramType, bool isOut)
{
    PNAME name = compiler()->namemgr->GetPredefName(isOut ? PN_OUTPARAM : PN_REFPARAM);
    PPARAMMODSYM sym;

    // See if we already have a parammod symbol of this base type.
    sym = LookupGlobalSym(name, paramType, MASK_PARAMMODSYM)->asPARAMMODSYM();
    if (! sym) {
        // No existing parammod symbol. Create a new one.
        sym = CreateGlobalSym(SK_PARAMMODSYM, name, paramType)->asPARAMMODSYM();
        if (isOut)
            sym->isOut = true;
        else
            sym->isRef = true;
    }

    ASSERT(sym->paramType() == paramType);

    return sym;
}


/*
 * Sets the filename of an output file to that
 * of the given input file
 */
void SYMMGR::SetOutFileName(PINFILESYM in)
{
    WCHAR filename[MAX_PATH], *temp1, *temp2;

    CPEFile::ReplaceFileExtension(wcscpy(filename, in->name->text), L".exe");
    temp1 = wcsrchr(filename, L'\\');
    temp2 = wcsrchr(filename, L'/');
    if (temp1 == NULL)
        temp1 = filename;
    else
        temp1++;
    if (temp2 > temp1)
        temp1 = temp2 + 1;

    in->getOutputFile()->name = compiler()->namemgr->AddString( temp1);
}


/*
 * Create a symbol representing an output file. All output files
 * are placed as children of the "fileroot" symbol.
 */
POUTFILESYM SYMMGR::CreateOutFile(LPCWSTR filename, bool isDll, bool isWinApp, LPCWSTR entryClass, LPCWSTR resource, LPCWSTR icon, bool buildIncrementally)
{
    if (!filename)
        filename = L"*";
    PNAME name = compiler()->namemgr->AddString(filename);
    POUTFILESYM rval = CreateGlobalSym(SK_OUTFILESYM, name, fileroot)->asOUTFILESYM();
    rval->isResource = false; // by default
    rval->isDll = isDll;
    rval->isConsoleApp = !isWinApp;
    rval->idFile = (mdFile)0;
    rval->idModRef = (mdModuleRef)0;
    rval->multiEntryReported = false;   // Always starts off as false
    if (resource) {
        rval->makeResFile = false;
        rval->resourceFile = allocGlobal->AllocStr(resource);
        ASSERT(!icon);
    } else {
        rval->makeResFile = true;
        if (icon)
            rval->iconFile = allocGlobal->AllocStr(icon);
        else
            rval->iconFile = NULL;
    }

    if (entryClass && !isDll)
        rval->entryClassName = allocGlobal->AllocStr(entryClass);
    else
        rval->entryClassName = NULL;

    rval->buildIncrementally = buildIncrementally;
    rval->entrySym = NULL;

    compiler()->cOutputFiles += 1;
    return rval;
}


/*
 * Create a symbol representing a resource file.  If bEmbed is true
 * it becomes a child of the default output file, otherwise a new
 * output file is created for itself
 */
PRESFILESYM SYMMGR::CreateEmbeddedResFile(LPCWSTR filename, LPCWSTR Ident, bool bVisible)
{
    RESFILESYM *resfile = CreateSeperateResFile(filename, mdfileroot, Ident, bVisible);
    resfile->isEmbedded = true;
    return resfile;
}

/*
 * Create a symbol representing a resource file.  If bEmbed is true
 * it becomes a child of the default output file, otherwise a new
 * output file is created for itself
 */
PRESFILESYM SYMMGR::CreateSeperateResFile(LPCWSTR filename, OUTFILESYM *outfileSym, LPCWSTR Ident, bool bVisible)
{
    PNAME name;
    PRESFILESYM resfileSym;
    POUTFILESYM pOutfile;

    name = compiler()->namemgr->AddString(Ident);
    ASSERT(outfileSym);

    // Check for duplicates
    for (pOutfile = fileroot->firstChild->asOUTFILESYM(); pOutfile != NULL; pOutfile = pOutfile->nextOutfile()) {
        if (LookupGlobalSym( name, pOutfile, MASK_RESFILESYM))
            return NULL;
    }

    // Create the input file symbol.
    resfileSym = CreateGlobalSym(SK_RESFILESYM, name, outfileSym)->asRESFILESYM();
    resfileSym->filename = allocGlobal->AllocStr( filename);
    resfileSym->isVis = bVisible;

    compiler()->NotifyHostOfBinaryFile(filename);

    return resfileSym;
}

/*
 * Create a symbol representing an imported metadata input file. 
 * All imported metadata input files are children of the MDFileRoot
 */
PINFILESYM SYMMGR::CreateMDFile(LPCWSTR filename, ULONG assemblyIndex, PARENTSYM *parent)
{
    PNAME name;
    PINFILESYM infileSym;

    name = compiler()->namemgr->AddString(filename);

    infileSym = (INFILESYM*)LookupGlobalSym(name, parent, MASK_INFILESYM);
    if (infileSym && !infileSym->isSource && infileSym->isAddedModule == (assemblyIndex == 0))
        // If we have a match then just return it
        return infileSym;

    // Create the input file symbol.
    infileSym = CreateGlobalSym(SK_INFILESYM, name, parent)->asINFILESYM();
    infileSym->isSource = false;
    infileSym->assemblyIndex = assemblyIndex;
    infileSym->idLocalAssembly = mdTokenNil;
    infileSym->canRefThisFile = true;
    infileSym->isAddedModule = (assemblyIndex == 0);
    infileSym->hasChanged = true;

    if (compiler()->options.m_fINCBUILD) {
        infileSym->timeLastChange = INCBUILD::GetFileChangeTime(infileSym->name->text);
    }

    compiler()->NotifyHostOfMetadataFile(filename);

    infileSym->getOutputFile()->cInputFiles += 1;
    compiler()->cInputFiles += 1;

    return infileSym;
}

/*
 * Create a symbol representing an synthetized input file
 */
PINFILESYM SYMMGR::CreateSynthSourceFile(LPCWSTR filename, OUTFILESYM *outfile)
{
    PNAME name;
    PINFILESYM infileSym;

    name = compiler()->namemgr->AddString(filename);

    // Create the input file symbol.
    infileSym = CreateGlobalSym(SK_SYNTHINFILESYM, name, outfile)->asANYINFILESYM();

    return infileSym;
}

/*
 * Create a symbol representing an input file, which creates
 * a given output file. All input files are placed as children of
 * their output files.
 */
PINFILESYM SYMMGR::CreateSourceFile(LPCWSTR filename, OUTFILESYM *outfile, FILETIME timeLastChange)
{
    PNAME name;
    PINFILESYM infileSym;

    name = compiler()->namemgr->AddString(filename);
    outfile->cInputFiles += 1;
    compiler()->cInputFiles += 1;

    // Create the input file symbol.
    infileSym = CreateGlobalSym(SK_INFILESYM, name, outfile)->asINFILESYM();
    infileSym->isSource = true;
    infileSym->idLocalAssembly = (mdAssembly) mdTokenNil;
    infileSym->canRefThisFile = false;
    infileSym->isAddedModule = false;
    infileSym->hasChanged = true;
    infileSym->assemblyIndex = 0;
    infileSym->timeLastChange = timeLastChange;
    infileSym->metaimport = (IMetaDataImport**)allocGlobal->AllocZero(sizeof(IMetaDataImport*));
    infileSym->dwScopes = 1;

    return infileSym;
}


DELETEDTYPESYM* SYMMGR::GetDeletedType(ULONG32 signatureSize, PCCOR_SIGNATURE signature)
{
    NAME *name = compiler()->namemgr->AddString((LPCWSTR)signature, (signatureSize+1) >> 1);
    DELETEDTYPESYM *sym = LookupGlobalSym(name, this->deletedtypes, SK_DELETEDTYPESYM)->asDELETEDTYPESYM();
    if (!sym) {
        sym = AllocSym(SK_DELETEDTYPESYM, name, allocGlobal)->asDELETEDTYPESYM();
        tableGlobal.InsertChild(this->deletedtypes, sym);
        sym->signatureSize = signatureSize;
        sym->isPrepared = 1;
    }
    return sym;
}

/*
 * Get one of the predefined types. It is forced to be declared if desired.
 *
 * If the predefined type is a require type, a non-NULL value will always be returned.
 * If the predefined type is a non-required type, and the type doesn't exist, then:
 *    if declared is false: NULL is returned, no error is reported.
 *    if declared is true: NULL is returned, and an error is reported. 
 */
PAGGSYM SYMMGR::GetPredefType(PREDEFTYPE pt, bool declared)
{
    PAGGSYM sym;

    ASSERT(pt >= 0 && pt < PT_COUNT);

    sym = predefSyms[pt];
    if (declared) {
        if (sym)
            DeclareType(sym);
        else {
            // This will fail and report an appropriate error.
            FindPredefinedType(predefTypeInfo[pt].fullName, true);
        }
    }
    return sym;
}

/*
 * Is this type a particular predefined type?
 */
bool TYPESYM::isPredefType(PREDEFTYPE pt)
{
    if (this == NULL)
        return false;
    else if (this->kind == SK_AGGSYM && this->asAGGSYM()->isPredefined && this->asAGGSYM()->iPredef == (unsigned) pt)
        return true;
    else if (this->kind == SK_VOIDSYM && pt == PT_VOID)
        return true;
    else
        return false;
}

/*
 *  A few types are considered "simple" types for purposes of conversions and so on. They
 *  are the fundemental types the compiler knows about for operators and conversions.
 */
bool TYPESYM::isSimpleType()
{
    return (this->kind == SK_AGGSYM && this->asAGGSYM()->isPredefined &&
            predefTypeInfo[this->asAGGSYM()->iPredef].isSimple);
}

bool TYPESYM::isQSimpleType()
{
    return (this->kind == SK_AGGSYM && this->asAGGSYM()->isPredefined &&
            predefTypeInfo[this->asAGGSYM()->iPredef].isQSimple);
}


/* 
 *  A few types are considered "numeric" types. They
 *  are the fundemental number types the compiler knows about for 
 *  operators and conversions.
 */
bool TYPESYM::isNumericType()
{
    return (this->kind == SK_AGGSYM && this->asAGGSYM()->isPredefined &&
            predefTypeInfo[this->asAGGSYM()->iPredef].isNumeric);
}

/* 
 *  byte, ushort, uint, ulong, and enums of the above
 */
bool TYPESYM::isUnsigned()
{

    TYPESYM * sym = this;

    if (!sym->isNumericType()) {
        if (sym->kind != SK_AGGSYM || !sym->asAGGSYM()->isEnum) {
            return false;
        }
        sym = sym->asAGGSYM()->underlyingType;
    }

    return (sym->asAGGSYM()->iPredef == PT_BYTE || (sym->asAGGSYM()->iPredef >= PT_USHORT && sym->asAGGSYM()->iPredef <= PT_ULONG));
}

/*
 * True if a type is not listed as a non-CLS type
 * See spec.
 */
bool TYPESYM::isCLS_Type()
{

    if (isQSimpleType() || kind == SK_PTRSYM)
        return false;

    if (kind == SK_AGGSYM && asAGGSYM()->isPredefined)
        return (asAGGSYM()->iPredef != PT_REFANY && asAGGSYM()->iPredef != PT_UINTPTR);

    if (kind == SK_ARRAYSYM) {
        // arrays of arrays are not CLS compliant!
        TYPESYM * elementType = asARRAYSYM()->elementType();
        return elementType->kind != SK_ARRAYSYM && elementType->isCLS_Type();
    }

    if (kind == SK_PARAMMODSYM)
        return asPARAMMODSYM()->paramType()->isCLS_Type();

    return checkForCLS();
}

/*
 * returns the fundamental aggregate symbol at the base of a complex type
 * or NULL if the type is derived from void
 */
AGGSYM * TYPESYM::underlyingAggregate()
{
    switch (this->kind) {
    case SK_AGGSYM:
        return this->asAGGSYM();
    case SK_VOIDSYM:
        return NULL;
    case SK_ARRAYSYM:
        return this->asARRAYSYM()->elementType()->underlyingAggregate();
    case SK_PARAMMODSYM:
        return this->asPARAMMODSYM()->paramType()->underlyingAggregate();
    case SK_PTRSYM:
        return this->asPTRSYM()->baseType()->underlyingAggregate();
    case SK_PINNEDSYM:
        return this->asPINNEDSYM()->baseType()->underlyingAggregate();
    case SK_NULLSYM:
        return NULL;
    case SK_ERRORSYM:
        return NULL;
    default:
        ASSERT(!"Bad TYPESYM kind");
    }

    return NULL;
}

/*
 * Is this type System.TypedReference or System.ArgIterator?
 * (used for errors becase these types can't go certain places)
 */
bool TYPESYM::isSpecialByRefType()
{
    if (this == NULL)
        return false;
    else if (this->kind == SK_AGGSYM && this->asAGGSYM()->isPredefined)
        return this->asAGGSYM()->iPredef == PT_REFANY || this->asAGGSYM()->iPredef == PT_ARGITERATOR;
    else
        return false;
}

/*
 * true iff this symbol should be CLS compliant based on it's attributes
 * and the attributes of it's declaration scope
 */
bool SYM::checkForCLS()
{
    ASSERT(this && kind != SK_NSSYM);
    if (kind == SK_ERRORSYM || kind == SK_VOIDSYM)
        return true;
    
    if (kind == SK_NSDECLSYM) {
        // Since there are no attributes on namespaces
        // skip directly to the 'global' assembly attribute.
        // If there is no assembly level attribute and we are
        // doing CLS checking, assume it should be Compliant
GET_ASSEMBLY:
        INFILESYM *in = getInputFile();
        return in->hasCLSattribute && in->isCLS;
    }

    ASSERT ((kind != SK_AGGSYM) || isPrepared);

    if (hasCLSattribute)
        return isCLS;

    PARENTSYM * temp = containingDeclaration();
    if (temp->kind == SK_NSDECLSYM)
        goto GET_ASSEMBLY;
    return temp->checkForCLS();
}

/*
 * Force a type to be fully declared, so it can be bound against.
 */
void SYMMGR::MakeTypeDeclared(PSYM sym)
{
    ASSERT(! sym->isPrepared);

    switch (sym->kind) {
    case SK_AGGSYM: {
        PAGGSYM aggsym = sym->asAGGSYM();

        if (aggsym->parseTree) {
            return;
        } else if (aggsym->getInputFile()->isSource) {
            compiler()->EnsureTypeIsDefined(aggsym);
            // EnsureTypeIsDefined may force the type to be parsed
            if (aggsym->parseTree) return;
        }

        if (TypeFromToken(aggsym->tokenImport) == mdtTypeDef) {
            // Type was imported; import all children and bases.
            compiler()->clsDeclRec.prepareAggregate(aggsym);
        } else {
            // type was imported, but we have no definition for it

            LOCATION * location = compiler()->location;

            //
            // find last spot which wasn't imported metadata
            //
            BASENODE *node = NULL;
            INFILESYM * file = NULL;
            if (compiler()->location) {
                while (location && location->getFile() && !location->getFile()->isSource) {
                    location = location->getPrevious();
                }

                if (location) {
                    node = location->getNode();
                    file = location->getFile();
                }
            }

            // Get the assembly this would be found in.
            WCHAR assemblyName[MAX_FULLNAME_SIZE];
            assemblyName[0] = '\0';
            compiler()->importer.GetTypeRefAssemblyName(aggsym->getInputFile(), aggsym->importScope, aggsym->tokenImport, assemblyName, lengthof(assemblyName));

            //
            // try and generate a good error message for it
            //
            CError  *pError;
            if (node && file) {
                pError = compiler()->clsDeclRec.make_errorStrStrSymbol(node, file, compiler()->ErrSym(aggsym), assemblyName, aggsym, ERR_NoTypeDef);
            } else {
                pError = compiler()->clsDeclRec.make_errorSymbolStr(aggsym, ERR_NoTypeDef, assemblyName);
            }

            //
            // dump all intervening import symbols on the location stack
            // to show the dependancy chain which caused the importing of the unknown symbol
            //
            LOCATION *currentLocation = compiler()->location;
            SYM *currentSymbol = NULL;
            while (currentLocation != location) {
                SYM *topSymbol = currentLocation->getSymbol();
                if (topSymbol && topSymbol != currentSymbol) {
                    currentSymbol = topSymbol;
                    if (currentSymbol != aggsym) {
                        compiler()->clsDeclRec.AddRelatedSymbolLocations (pError, topSymbol);
                    }
                }
                currentLocation = currentLocation->getPrevious();
            }

            // Submit the error
            compiler()->SubmitError (pError);

            //
            // fake up the type so that it looks reasonable
            // for the rest of the compile
            //
            aggsym->isPrepared = true;
            aggsym->isDefined = true;
            aggsym->isClass = true;
            aggsym->hasResolvedBaseClasses = true;
            if (aggsym != GetObject()) {
                aggsym->baseClass = GetObject();
            }

        }
        break;
    }

    case SK_PARAMMODSYM: {
        DeclareType(sym->asPARAMMODSYM()->paramType());
        sym->isPrepared = 1;
        break;
    }

    case SK_ARRAYSYM: {
        // Arrays should have element type declared.
        ASSERT(sym->kind == SK_ARRAYSYM);
        DeclareType(sym->asARRAYSYM()->elementType());
        sym->isPrepared = 1;
        break;
    }
    case SK_ALIASSYM: {
        // aliases should have their underlying type declared.
        sym->isPrepared = 1;
        break;
    }

    case SK_NSSYM:
    case SK_NULLSYM:
    case SK_VOIDSYM: {
        // Nothing to do.
        sym->isPrepared = 1;
        break;
    }

    case SK_PTRSYM:
        DeclareType(sym->asPTRSYM()->baseType());
        sym->isPrepared = 1;
        break;

    case SK_PINNEDSYM:
        DeclareType(sym->asPINNEDSYM()->baseType());
        sym->isPrepared = 1;
        break;

    default:
        ASSERT(!"unknown type");
        break;
    }
}


/*
 * Add a sym to a symbol list. The memory for the list is allocated from
 * the global symbol area, so this is appropriate only for global symbols.
 *
 * The calls should pass a pointer to a local that's initialized to
 * point to the PSYMLIST that's the head of the list.
 */
void SYMMGR::AddToGlobalSymList(PSYM sym, PSYMLIST * * symLink)
{
    AddToSymList(allocGlobal, sym, symLink);
}


/*
 * Add a sym to a symbol list. The memory for the list is allocated from
 * the local symbol area, so this is appropriate only for local symbols.
 *
 * The calls should pass a pointer to a local that's initialized to
 * point to the PSYMLIST that's the head of the list.
 */
void SYMMGR::AddToLocalSymList(PSYM sym, PSYMLIST * * symLink)
{
    AddToSymList(allocLocal, sym, symLink);
}


/*
 * Add a sym to a symbol list. The memory for the list is allocated from
 * the provided heap.
 *
 * The calls should pass a pointer to a local that's initialized to
 * point to the PSYMLIST that's the head of the list.
 */
void SYMMGR::AddToSymList(NRHEAP *heap, PSYM sym, PSYMLIST * * symLink)
{
    PSYMLIST newList;

    ASSERT(! sym->isLocal);

    // Allocate and fill in new list element.
    newList = (SYMLIST *) heap->Alloc(sizeof(SYMLIST));
    newList->sym = sym;
    newList->next = **symLink;

    // List onto end of list, and update the link value.
    **symLink = newList;
    *symLink = & newList->next;
}


/*
 * Add a name to a symbol list. The memory for the list is allocated from
 * the global symbol area, so this is appropriate only for global symbols.
 *
 * The calls should pass a pointer to a local that's initialized to
 * point to the PNAMELIST that's the head of the list.
 */
void SYMMGR::AddToGlobalNameList(PNAME name, PNAMELIST * * nameLink)
{
    AddToNameList(allocGlobal, name, nameLink);
}


/*
 * Add a name to a symbol list. The memory for the list is allocated from
 * the local symbol area, so this is appropriate only for local symbols.
 *
 * The calls should pass a pointer to a local that's initialized to
 * point to the PNAMELIST that's the head of the list.
 */
void SYMMGR::AddToLocalNameList(PNAME name, PNAMELIST * * nameLink)
{
    AddToNameList(allocLocal, name, nameLink);
}


/*
 * Add a name to a symbol list. The memory for the list is allocated from
 * the provided heap.
 *
 * The calls should pass a pointer to a local that's initialized to
 * point to the PNAMELIST that's the head of the list.
 */
void SYMMGR::AddToNameList(NRHEAP *heap, PNAME name, PNAMELIST * * nameLink)
{
    PNAMELIST newList;

    ASSERT(**nameLink == NULL);

    // Allocate and fill in new list element.
    newList = (NAMELIST *) heap->Alloc(sizeof(NAMELIST));
    newList->name = name;
    newList->next = NULL;

    // List onto end of list, and update the link value.
    **nameLink = newList;
    *nameLink = & newList->next;
}


/*
 * Get the COM+ signature element type from an aggregate type. 
 */
BYTE SYMMGR::GetElementType(PAGGSYM type)
{
    if (type->isPredefined) {
        BYTE et = predefTypeInfo[type->iPredef].et;
        if (et != ELEMENT_TYPE_END)
            return et;
    }

    // Not a special type. Either a value or reference type.
    if (type->fundType() == FT_REF)
        return ELEMENT_TYPE_CLASS;
    else 
        return ELEMENT_TYPE_VALUETYPE;
}

/*
 * Some of the predefined types have built-in names, like "int"
 * or "string" or "object". This return the nice name if one
 * exists; otherwise NULL is returned.
 */
LPWSTR SYMMGR::GetNiceName(PAGGSYM type)
{
    if (type->isPredefined) 
        return GetNiceName((PREDEFTYPE) type->iPredef);
    else
        return NULL;
}

/*
 * Some of the predefined types have built-in names, like "int"
 * or "string" or "object". This return the nice name if one
 * exists; otherwise NULL is returned.
 */
LPWSTR SYMMGR::GetNiceName(PREDEFTYPE pt)
{
    return predefTypeInfo[pt].niceName;
}

int SYMMGR::GetAttrArgSize(PREDEFTYPE pt)
{
    return predefTypeInfo[pt].asize;
}

/*
 */
LPWSTR SYMMGR::GetFullName(PREDEFTYPE pt)
{
    return predefTypeInfo[pt].fullName;
}


/* 
 * Determine if a class/struct/interface or interface (base) is a base of 
 * another class/struct/interface (derived). Object is NOT considered
 * a base of an interface but is considered as base of a struct.
 */
bool SYMMGR::IsBaseType(PAGGSYM derived, PAGGSYM base)
{
    ASSERT(!derived->isEnum && !base->isEnum);

    if (derived == base)
        return true;      // identity.

    if (base->isSealed)
        return false;     // structs and delegates and sealed classes are not bases of anything.

    if (base->isInterface) {
        // Search the direct and indirect interfaces recursively,
        // going up the base chain..

        while (derived) {
            FOREACHSYMLIST(derived->ifaceList, iface)
                if (IsBaseType(iface->asAGGSYM(), base)) {
                    return true;
                }
            ENDFOREACHSYMLIST
            derived = derived->baseClass;
        }

        return false;
    }
    else {
        ASSERT(base->isClass);

        // base is a class. Just go up the base class chain to look for it.

        while (derived->baseClass) {
            derived = derived->baseClass;
            if (derived == base)
                return true;
        }

        return false;
    }
}


/*
 * Allocate a type array; used to represent a parameter list.
 * We use a hash table to make sure that allocating the same type array
 * twice returns the same value. This does two things:
 *  1) Save a lot of memory.
 *  2) Make it so parameter lists can be compared by a simple pointer comparison.
 *  3) Allow us to associated a token with each signature for faster metadata emit.
 */

PTYPESYM * SYMMGR::AllocParams(int cTypes, PTYPESYM * types, mdToken ** ppToken)
{
    unsigned ibucket;
    unsigned hash;
    PTYPEARRAY typeArray;
    PTYPEARRAY * link; // link to place to add new name.
    mdToken * pToken;

    if (cTypes == 0)
        return NULL;        // No type array.

    // Create hash value from all the types in the array.
    hash = NAMEMGR::HashString((WCHAR *) types, cTypes * sizeof(types[0]) / sizeof(WCHAR));

    if (buckets == NULL) {
        // Nothing in the table. Create a new empty table.
        ASSERT(cTypeArrays == 0);

        buckets = (PTYPEARRAY *) compiler()->pageheap.AllocPages(PAGEHEAP::pageSize);
        if (!buckets)
            compiler()->NoMemory();  // won't return.

	ASSERT(PAGEHEAP::pageSize / sizeof(PTYPEARRAY) < UINT_MAX);
        cBuckets = (unsigned)(PAGEHEAP::pageSize / sizeof(PTYPEARRAY));

        memset(buckets, 0, cBuckets * sizeof(PTYPEARRAY));
        bucketMask = cBuckets - 1;
    }

    // Get the bucket.
    ibucket = (hash & bucketMask);

    // Traverse all type arrays in the bucket.
    link = & buckets[ibucket];
    for (;;) {
        typeArray = *link;

        if (!typeArray)
            break;  // End of list.

        if (typeArray->hash == hash) {
            // Hash values match... pretty good sign. Check length
            // and types.
            if (typeArray->cTypes == cTypes && 
                memcmp(typeArray->types, types, cTypes * sizeof(PTYPESYM)) == 0)
            {
                // Yes, we found it!
                if (ppToken)
                    *ppToken = (mdToken *)(& typeArray->types[cTypes]);
                return & typeArray->types[0];
            }
        }
        link = & typeArray->next;
    }

    // Typearray not found; create new and add it to the table.
    typeArray = (PTYPEARRAY) allocGlobal->Alloc(sizeof(TYPEARRAY) + cTypes * sizeof(PTYPESYM) + sizeof(mdToken));
    *link = typeArray;

    // Fill in the new typearray structure and copy in the array.
    typeArray->hash = hash;
    typeArray->next = 0;
    typeArray->cTypes = cTypes;
    memcpy(typeArray->types, types, cTypes * sizeof(PTYPESYM));

    // If the table is full, resize it.
    ++cTypeArrays;
    if (cTypeArrays >= cBuckets)
        GrowTypeArrayTable();

    // The token lives just beyond the array.
    pToken = (mdToken *)(& typeArray->types[cTypes]);
    *pToken = 0;
    if (ppToken)
        *ppToken = pToken;

    // And return the allocated type array.
    return & typeArray->types[0];
}


/*
 * Double the bucket size of the type array hash table.
 */
void SYMMGR::GrowTypeArrayTable()
{
    unsigned newSize;
    size_t cbNew;
    PTYPEARRAY * newBuckets;
    unsigned iBucket;
    PTYPEARRAY * linkLow, * linkHigh;
    PTYPEARRAY typeArray;

    // Allocate the new bucket table.
    newSize = cBuckets * 2;
    cbNew = newSize * sizeof(PTYPEARRAY);
    newBuckets = (PTYPEARRAY *) compiler()->pageheap.AllocPages(cbNew);
    if (!newBuckets)
        compiler()->NoMemory();  // won't return.

    // Go throw each bucket in the old table, and link the typearrays
    // into the new tables. Because the old table is a power of 2 in size,
    // and the new table is twice that size, entries from bucket n in the
    // old table must map to either bucket n or bucket n + oldSize in the new
    // table. Which of these two is determined by (hash & oldSize).
    for (iBucket = 0; iBucket < cBuckets; ++iBucket)
    {
        // Get starting links for the two new buckets.
        linkLow = &newBuckets[iBucket];
        linkHigh = &newBuckets[iBucket + cBuckets];

        // Go through each typeArray in the old bucket, redistributing
        // to the two new buckets.
        typeArray = buckets[iBucket];
        while (typeArray) {
            if (typeArray->hash & cBuckets) {
                *linkHigh = typeArray;
                linkHigh = & typeArray->next;
            }
            else {
                *linkLow = typeArray;
                linkLow = & typeArray->next;
            }

            typeArray = typeArray->next;
        }

        // Terminate both new linked lists.
        *linkLow = *linkHigh = NULL;
    }

    // Free the old bucket table.
    size_t cb = cBuckets * sizeof(PTYPEARRAY);
    compiler()->pageheap.FreePages(buckets, cb);

    // Remember the new bucket table and size.
    buckets = newBuckets;
    cBuckets = newSize;
    bucketMask = cBuckets - 1;
}




#ifdef DEBUG
/****************************************************************************
 * Symbol dumping routines.
 */



/*
 * Print <indent" spaces to stdout.
 */
static void PrintIndent(int indent)
{
    for (int i = 0; i < indent; ++i)
        putchar(' ');
}

/*
 * Dump the children of a symbol to stdout.
 */
void SYMMGR::DumpChildren(PPARENTSYM sym, int indent)
{
    FOREACHCHILD(sym, symChild)
        if (symChild->kind == SK_NSSYM || symChild->kind == SK_MEMBVARSYM ||
            symChild->kind == SK_METHSYM || symChild->kind == SK_PROPSYM ||
            symChild->kind == SK_AGGSYM || symChild->kind == SK_ERRORSYM ||
            symChild->kind == SK_NSDECLSYM)
                DumpSymbol(symChild, indent + 2);
    ENDFOREACHCHILD
}

/*
 * Dump a text representation of an access level to stdout
 */
void SYMMGR::DumpAccess(ACCESS acc)
{
    switch (acc) {
    case ACC_PRIVATE:
        printf("private "); break;
    case ACC_INTERNAL:
        printf("internal "); break;
    case ACC_PROTECTED:
        printf("protected "); break;
    case ACC_INTERNALPROTECTED:
        printf("internal protected "); break;
    case ACC_PUBLIC:
        printf("public "); break;
    default:
        printf("/* unknown access */"); break;
    }
}

/* 
 * Dump a text representation of a constant value
 */
void SYMMGR::DumpConst(PTYPESYM type, CONSTVAL * constVal)
{
    if (type->isPredefType(PT_BOOL)) {
        printf("%s", constVal->iVal ? "true" : "false");
        return;
    }   
    else if (type->isPredefType(PT_STRING)) {
        STRCONST * strConst = constVal->strVal;
        printf("\"");
        for (int i = 0; i < strConst->length; ++i) {
            printf("%lc", strConst->text[i]);
        }
        printf("\"");
        return;
    }

    switch (type->fundType())
    {
    case FT_I1: case FT_I2: case FT_I4:
        printf("%d", constVal->iVal);
        break;
    case FT_U1: case FT_U2: case FT_U4:
        printf("%u", constVal->uiVal);
        break;
    case FT_U8: case FT_I8:
        printf("%I64", *constVal->longVal); 
        break;
    case FT_R4:
    case FT_R8:
        printf("%g", * constVal->doubleVal);
        break;
    case FT_REF:
        ASSERT(constVal->iVal == 0);
        printf("null");
        break;
    default:
        ASSERT(0);
    }
}

/* 
 * Dump a text representation of a symbol to stdout.
 */
void SYMMGR::DumpSymbol(PSYM sym, int indent)
{
    int i;

    if (sym == NULL) {
        printf("*NULL*");
        return;
    }

    switch (sym->kind) {
    case SK_NSSYM:
        {
            PNSDECLSYM declaration;
            for (declaration = sym->asNSSYM()->firstDeclaration();
                 declaration;
                 declaration = declaration->nextDeclaration)
            {
                DumpSymbol(declaration, indent);
            }
        }
        break;
    case SK_NSDECLSYM:
        PrintIndent(indent);
        printf("namespace %ls {\n", sym->asNSDECLSYM()->namespaceSymbol->name->text);

        DumpChildren(sym->asPARENTSYM(), indent);

        PrintIndent(indent);
        printf("}\n");
        break;

    case SK_AGGSYM:
        PrintIndent(indent);

        DumpAccess(sym->asAGGSYM()->access);

        if (sym->asAGGSYM()->isClass)
            printf("class");
        else if (sym->asAGGSYM()->isInterface)
            printf("interface");
        else if (sym->asAGGSYM()->isStruct)
            printf("struct");
        else if (sym->asAGGSYM()->isEnum)
            printf("enum");
        else
            printf("*unknown*");
        printf(" %ls ", sym->name->text);

        // Base class.
        if (sym->asAGGSYM()->baseClass) {
            printf(": ");
            if (sym->asAGGSYM()->isEnum)
                DumpType(sym->asAGGSYM()->underlyingType);
            else
                DumpType(sym->asAGGSYM()->baseClass);
        }
        if (sym->asAGGSYM()->ifaceList) {
            if (sym->asAGGSYM()->baseClass)
                printf(", ");
            else
                printf(": ");

            for (PSYMLIST list = sym->asAGGSYM()->ifaceList; list; list = list->next)
            {
                DumpType(list->sym->asTYPESYM());
                if (list->next)
                    printf(", ");
            }
        }

        printf("\n");
        PrintIndent(indent);
        printf("{\n");

        DumpChildren(sym->asPARENTSYM(), indent);

        PrintIndent(indent);
        printf("}\n\n");
        break;

    case SK_MEMBVARSYM:
        PrintIndent(indent);
        DumpAccess(sym->asMEMBVARSYM()->access);

        if (sym->asMEMBVARSYM()->isConst)
            printf("const ");
        else {
            if (sym->asMEMBVARSYM()->isStatic)
                printf("static ");
            if (sym->asMEMBVARSYM()->isReadOnly)
                printf("readonly ");
        }        

        DumpType(sym->asVARSYM()->type);

        printf(" %ls", sym->asVARSYM()->name->text);

        if (sym->asMEMBVARSYM()->isConst && sym->asVARSYM()->type != GetErrorSym()) {
            printf(" = ");
            DumpConst(sym->asVARSYM()->type, & sym->asMEMBVARSYM()->constVal);
        }

        printf(";");

        if (sym->asMEMBVARSYM()->isBogus)
            printf("  /* UNSUPPORTED or BOGUS */");

        printf("\n");
        break;

    case SK_LOCVARSYM:
        PrintIndent(indent);
        DumpType(sym->asVARSYM()->type);
        printf(" %ls;\n", sym->asVARSYM()->name->text);
        break;

    case SK_METHSYM:
        PrintIndent(indent);
        DumpAccess(sym->asMETHSYM()->access);

        if (sym->asMETHSYM()->isStatic)
            printf("static ");
        if (sym->asMETHSYM()->isAbstract)
            printf("abstract ");
        if (sym->asMETHSYM()->isVirtual)
            printf("virtual ");

        if (sym->asMETHSYM()->isCtor) {
            printf("%ls(", sym->asMETHSYM()->parent->name->text);
        }
        else {
            DumpType(sym->asMETHSYM()->retType);
            printf(" %ls(", sym->asMETHSYM()->name->text);
        }

        for (i = 0; i < sym->asMETHSYM()->cParams; ++i) {
            if (i != 0)
                printf(", ");
            DumpType(sym->asMETHSYM()->params[i]);
        }
        printf(");");

        if (sym->asMETHSYM()->isBogus)
            printf("  /* UNSUPPORTED or BOGUS */");
        printf("\n");

        break;

    case SK_PROPSYM:
        PrintIndent(indent);
        DumpAccess(sym->asPROPSYM()->access);

        if (sym->asPROPSYM()->isStatic)
            printf("static ");

        DumpType(sym->asPROPSYM()->retType);
        printf(" %ls", sym->asPROPSYM()->name->text);

        if (sym->asPROPSYM()->cParams > 0)
            printf("[");

        for (i = 0; i < sym->asPROPSYM()->cParams; ++i) {
            if (i != 0)
                printf(", ");
            DumpType(sym->asPROPSYM()->params[i]);
        }
        if (sym->asPROPSYM()->cParams > 0)
            printf("]");

        if (sym->asPROPSYM()->isBogus)
            printf("  /* UNSUPPORTED or BOGUS */");
        printf("\n");

        PrintIndent(indent);
        printf("{ ");
        if (sym->asPROPSYM()->methGet)
            printf("get { %ls } ", sym->asPROPSYM()->methGet->name->text);
        if (sym->asPROPSYM()->methSet)
            printf("set { %ls } ", sym->asPROPSYM()->methSet->name->text);
        printf("}\n");

        break;

    case SK_ERRORSYM:
        printf("*error*");
        break;

    case SK_INFILESYM:
        if (sym->asINFILESYM()->isSource)
            printf("source file %ls", sym->asINFILESYM()->name->text);
        else
            printf("metadata file %ls", sym->asINFILESYM()->name->text);
        break;

    case SK_OUTFILESYM:
    case SK_SCOPESYM:
    default:
        ASSERT(0);
        break;
    }
}

/* 
 * Dump a text representation of a type to stdout.
 */

void SYMMGR::DumpType(PTYPESYM sym)
{
    int i, rank;

    if (sym == NULL) {
        printf("*NULL*");
        return;
    }

    switch (sym->kind) {
    case SK_AGGSYM:
        if (sym == GetPredefType(PT_OBJECT, false))
            printf("object");
        else if (sym == GetPredefType(PT_STRING, false))
            printf("string");
        else if (sym == GetPredefType(PT_BYTE, false))
            printf("byte");
        else if (sym == GetPredefType(PT_SHORT, false))
            printf("short");
        else if (sym == GetPredefType(PT_INT, false))
            printf("int");
        else if (sym == GetPredefType(PT_LONG, false))
            printf("long");
        else if (sym == GetPredefType(PT_SBYTE, false))
            printf("sbyte");
        else if (sym == GetPredefType(PT_USHORT, false))
            printf("ushort");
        else if (sym == GetPredefType(PT_UINT, false))
            printf("uint");
        else if (sym == GetPredefType(PT_ULONG, false))
            printf("ulong");
        else if (sym == GetPredefType(PT_FLOAT, false))
            printf("float");
        else if (sym == GetPredefType(PT_DOUBLE, false))
            printf("double");
        else if (sym == GetPredefType(PT_CHAR, false))
            printf("char");
        else if (sym == GetPredefType(PT_BOOL, false))
            printf("bool");
        else
            printf("%ls", sym->name->text);
        break;

    case SK_ARRAYSYM:
    {
        // Brackets go the reverse way that would be logical, so we need
        // to get down to the first non-array element type.
        PTYPESYM elementType = sym;
        while (elementType->kind == SK_ARRAYSYM)
            elementType = elementType->asARRAYSYM()->elementType();
        DumpType(elementType);

        do {
            rank = sym->asARRAYSYM()->rank;

            if (rank == 0) {
                printf("[?]");
            }
            else if (rank == 1)
                printf("[]");
            else
            {
                // known rank > 1
                printf("[*");
                for (i = rank; i > 1; --i) {
                    printf(",*");
                }
                printf("]");
            }
        } while ((sym = sym->asARRAYSYM()->elementType())->kind == SK_ARRAYSYM);
        break;
    }

    case SK_PTRSYM:
        DumpType(sym->asPTRSYM()->baseType());
        printf(" *");
        break;

    case SK_PARAMMODSYM:
        if (sym->asPARAMMODSYM()->isOut)
            printf("out ");
        else
            printf("ref ");
        DumpType(sym->asPARAMMODSYM()->paramType());
        break;

    case SK_VOIDSYM:
        printf("void");
        break;

    case SK_NULLSYM:
        printf("null");
        break;

    case SK_ERRORSYM:
        printf("*error*");
        break;

    default:
        ASSERT(0);
        break;
    }




}


PINFILESYM SYMMGR::GetPredefInfile()
{
    if (!predefInputFile) {
        // Create a bogus inputfile
        // we use the bogus assemly id 0

        predefInputFile = CreateSourceFile(L"", mdfileroot, FILETIME());
        predefInputFile->rootDeclaration = CreateNamespaceDeclaration(
                    rootNS, 
                    NULL,                               // no declaration parent
                    predefInputFile, 
                    NULL);                              // no parse tree
    }
    return predefInputFile;
}

#endif //DEBUG
