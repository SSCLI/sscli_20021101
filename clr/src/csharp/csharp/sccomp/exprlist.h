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
// File: exprlist.h
//
// Defines the nodes of the bound expression tree
// ===========================================================================

/*
 * Define the different expr node kinds
 */

#define EXPRDEF(kind) EK_ ## kind,
enum EXPRKIND {
    #include "exprkind.h"
    EK_COUNT,

    EK_LIST,
    EK_ASSG,

    EK_MAKERA,
    EK_VALUERA,
    EK_TYPERA,

    EK_ARGS,

    EK_FIRSTOP,   // this is only used as a parameter, no actual exprs are constructed with it

    //EK_OLDEQUALS = EK_FIRSTOP,
    //EK_OLDCOMPARE,

    EK_EQUALS = EK_FIRSTOP,        // this is only used as a parameter, no actual exprs are constructed with it
    EK_COMPARE,       // this is only used as a parameter, no actual exprs are constructed with it

    //EK_OLDADD,
    //EK_OLDSUB,

    EK_TRUE,
    EK_FALSE,

    EK_LOGNOT,

    EK_INC,       // this is only used as a parameter, no actual exprs are constructed with it
    EK_DEC,       // this is only used as a parameter, no actual exprs are constructed with it

    EK_EQ,          // keep EK_EQ to EK_GE in the same sequence (ILGENREC::genCondBranch)
    EK_NE,          // keep EK_EQ to EK_GE in the same sequence (ILGENREC::genCondBranch)
    EK_LT,          // keep EK_EQ to EK_GE in the same sequence (ILGENREC::genCondBranch)
    EK_LE,          // keep EK_EQ to EK_GE in the same sequence (ILGENREC::genCondBranch)
    EK_GT,          // keep EK_EQ to EK_GE in the same sequence (ILGENREC::genCondBranch)
    EK_GE,          // keep EK_EQ to EK_GE in the same sequence (ILGENREC::genCondBranch)

    EK_ADD,         // keep EK_ADD to EK_ARRLEN in the same sequence (ILGENREC::genBinopExpr)
    EK_SUB,         // keep EK_ADD to EK_ARRLEN in the same sequence (ILGENREC::genBinopExpr)
    EK_MUL,         // keep EK_ADD to EK_ARRLEN in the same sequence (ILGENREC::genBinopExpr)
    EK_DIV,         // keep EK_ADD to EK_ARRLEN in the same sequence (ILGENREC::genBinopExpr)
    EK_MOD,         // keep EK_ADD to EK_ARRLEN in the same sequence (ILGENREC::genBinopExpr)

    EK_BITAND,      // keep EK_ADD to EK_ARRLEN in the same sequence (ILGENREC::genBinopExpr)
    EK_BITOR,       // keep EK_ADD to EK_ARRLEN in the same sequence (ILGENREC::genBinopExpr)
    EK_BITXOR,      // keep EK_ADD to EK_ARRLEN in the same sequence (ILGENREC::genBinopExpr)
    EK_LSHIFT,      // keep EK_ADD to EK_ARRLEN in the same sequence (ILGENREC::genBinopExpr)
    EK_RSHIFT,      // keep EK_ADD to EK_ARRLEN in the same sequence (ILGENREC::genBinopExpr)
    EK_NEG,         // keep EK_ADD to EK_ARRLEN in the same sequence (ILGENREC::genBinopExpr)
    EK_UPLUS,       // keep EK_ADD to EK_ARRLEN in the same sequence (ILGENREC::genBinopExpr)
    EK_BITNOT,      // keep EK_ADD to EK_ARRLEN in the same sequence (ILGENREC::genBinopExpr)
    EK_ARRLEN,      // keep EK_ADD to EK_ARRLEN in the same sequence (ILGENREC::genBinopExpr)

    EK_LOGAND,
    EK_LOGOR,

    EK_IS,
    EK_AS,
    EK_ARRINDEX,
    EK_NEWARRAY,
    EK_QMARK,
    EK_SEQUENCE,  // p1 is sideeffectes, p2 is values
    EK_SAVE,        // p1 is expr, p2 is wrap to be saved into...
    EK_SWAP,

    EK_ARGLIST,

    EK_INDIR,
    EK_ADDR,
    EK_LOCALLOC,

    EK_MULTIOFFSET, // This has to be last!!! To deal /w multiops we add this to the op to obtain the ek in the op table
};
#undef EXPRDEF



enum EXPRFLAG {
    // These are specific to various node types.

    EXF_BINOP = 0x01,           // Only on non stmt exprs
    EXF_COMPARE = 0x4,          // Only on EXPRBINOP, indicates compare op (or &&, ||)
    EXF_ISPOSTOP = 0x8,         // Only on EXPRMULTI, indicates <x>++

    EXF_UNREALIZEDCONCAT = 0x8, // Only on EXPRCONCAT, means that we need to realize the list to a constructor call...

    EXF_LITERALCONST = 0x10,    // Only on EXPRCONSTANT, means this was not a folded constant


    EXF_NEEDSRET = 0x01,        // Only on EXPRBLOCK

    EXF_UNREALIZEDGOTO = 0x01,  // Only on EXPRGOTO, means target unknown
    EXF_GOTOASLEAVE = 0x2,      // Only on EXPRGOTO, means use leave instead of br instruction
    EXF_GOTOCASE = 0x4,         // Only on EXPRGOTO, means goto case or goto default
    EXF_GOTOFORWARD = 0x8,      // Only on EXPRGOTO, means a known forward jump
    EXF_BADGOTO = 0x10,         // Only on EXPRGOTO, indicates an unrealizable goto
    EXF_OPTIMIZABLEGOTO = 0x20, // Only on EXPRGOTO, indicates inserted by compiler and may be removed
    EXF_GOTOASFINALLYLEAVE = 0x40, // Only on EXPRGOTO, means leave through a finally, GOTOASLEAVE must also be set
    EXF_GOTOBLOCKED = 0x80,     // only on EXPRGOTO, means the goto passes through a finally which does not terminate

    EXF_RETURNASLEAVE = 0x1,    // only on EXPRRETURN, means needs intermediate jump out of try block
    EXF_RETURNASFINALLYLEAVE = 0x2, // only on EXPRRETURN, means leave through a finally (return as leave must also be set)
    EXF_RETURNBLOCKED = 0x4,    // only on EXPRRETURN, means that the return is blocked by a non-terminating finally

    EXF_NEWOBJCALL = 0x10,      // only on EXPRCALL, to indicate new <...>(...)
    EXF_BOUNDCALL = 0x20,       // only on EXPRCALL, indicates that bindCall returned this
    EXF_BASECALL = 0x40,        // only on EXPRCALL, EXPRFNCPTR, EXPRPROP, and EXPREVENT, indicateas a "base.XXXX" call
    EXF_HASREFPARAM = 0x80,     // onyl on EXPRCALL, indicates that some of the params are out or ref

    EXF_HASDEFAULT = 0x1,       // only on EXPRSWITCH
    EXF_HASHTABLESWITCH = 0x2,  // only on EXPRSWITCH

    EXF_HASNOCODE = 0x1,        // only on EXPRSWITCHLABEL

    EXF_ISFINALLY = 0x1,        // only on EXPRTRY
    EXF_FINALLYBLOCKED = 0x2,   // only on EXPRTRY, means that FINALLY block end is unreachable
    EXF_REMOVEFINALLY = 0x4,    // only on EXPRTRY, means that the try-finally should be converted to normal code

    EXF_HASRETHROW = 0x1,       // only on EXPRHANDLER, means that an argumentless throw occurs inside it

    EXF_BOX = 0x10,              // only on EXPRCAST, indicates a boxing operation (value type -> object)
    EXF_UNBOX = 0x20,            // only on EXPRCAST, indicates a unboxing operation (object -> value type)
    EXF_REFCHECK = 0x40,         // only on EXPRCAST, indicates an reference checked cast is required
    EXF_INDEXEXPR = 0x80,        // only on EXPRCAST, indicates a special cast for array indexing

    EXF_ARRAYCONST = 0x2,        // only on EXPRARRINIT, indicates that we should init using a memory block
    EXF_ARRAYALLCONST = 0x4,     // only on EXPRARRINIT, indicated that all elems are constant (must also have ARRAYCONST set)

    EXF_MEMBERSET = 0x02,        // only on EXPRFIELD, indicates that the reference is for set purposes

    EXF_IMPLICITTHIS = 0x10,     // only on EXPRLOCAL, indicates a compiler provided this pointer

    EXF_NODEBUGINFO = 0x10,      // only on EXPRSTMTAS, indicates no debug info for this statement.

    EXF_ARRFLAT = 0x10,          // only on EXPRBINOP, w/ kind == EK_ARRINDEX, to indicate a flat array view get call

    // The following are usable on multiple node types.
    EXF_CANTBENULL = 0x0100,      // indicate this expression can't ever be null (e.g., "this").
    EXF_CHECKOVERFLOW = 0x0200,     // indicates that operation should be checked for overflow
    EXF_NEWSTRUCTASSG = 0x0400,     // on any exprs, indicates that this is a constructor call which assigns to object

    EXF_WRAPASTEMP = 0x0800,      // only on expr wrap... indicates that this wrap represents an actual local...
    EXF_VALONSTACK = 0x1000,      // Only on any expr, indicates value on stack already (as an exprwrap)
    EXF_ASSGOP = 0x2000,          // On any non stmt exprs, indicates assignment node...
    EXF_LVALUE = 0x4000,          // On any exprs
    EXF_NAMEEXPR = 0x8000,        // On any exprs, indicates that the expression consists of E.M where E is a simple name
};

// Typedefs for some pointer types up front.
typedef class EXPR * PEXPR;
typedef class EXPRLIST * PEXPRLIST;

/* 
 * EXPR - the base expression node. 
 */
class EXPR {
public:
    EXPRKIND kind: 8;     // the exprnode kind
    int flags : 24;
    
    // We have more bits available here!
    BASENODE * tree;
    TYPESYM * type;


    // We define the casts by hand so that statement completion will be aware of them...
    class EXPRBLOCK * asBLOCK ();
    class EXPRBINOP * asBINOP ();
    class EXPRSTMTAS * asSTMTAS ();
    class EXPRCALL * asCALL ();
    class EXPRCAST * asCAST ();
    class EXPREVENT * asEVENT ();
    class EXPRFIELD * asFIELD ();
    class EXPRLOCAL * asLOCAL ();
    class EXPRRETURN * asRETURN ();
    class EXPRCONSTANT * asCONSTANT ();
    class EXPRCLASS * asCLASS ();
    class EXPRNSPACE * asNSPACE ();
    class EXPRERROR * asERROR();
    class EXPRDECL * asDECL();
    class EXPRLABEL * asLABEL();
    class EXPRGOTO * asGOTO();
    class EXPRGOTOIF * asGOTOIF();
    class EXPRFUNCPTR * asFUNCPTR();
    class EXPRSWITCH * asSWITCH();
    class EXPRSWITCHLABEL * asSWITCHLABEL();
    class EXPRPROP * asPROP();
    class EXPRHANDLER * asHANDLER();
    class EXPRTRY * asTRY();
    class EXPRTHROW * asTHROW();
    class EXPRMULTI * asMULTI();
    class EXPRWRAP * asWRAP();
    class EXPRCONCAT * asCONCAT();
    class EXPRARRINIT * asARRINIT();
    class EXPRASSERT * asASSERT();
    class EXPRTYPEOF * asTYPEOF();
    class EXPRSIZEOF * asSIZEOF();
    class EXPRZEROINIT * asZEROINIT();
    class EXPRNOOP * asNOOP();
    class EXPRNAMEDATTRARG * asNAMEDATTRARG();
    class EXPRUSERLOGOP * asUSERLOGOP();

    EXPR * asEXPR() { return this; };
    EXPRBINOP * asBIN() { ASSERT(!this || (this->flags & EXF_BINOP)); return (EXPRBINOP*)this;};
    EXPRGOTO * asGT() { ASSERT(!this || this->kind == EK_GOTO || this->kind == EK_GOTOIF || this->kind == EK_ASSERT); return (EXPRGOTO*) this; };
    EXPRLABEL * asANYLABEL() { ASSERT(!this || this->kind == EK_LABEL || this->kind == EK_SWITCHLABEL); return (EXPRLABEL*)this; };

    // Allocate from a no-release allocator.
    #undef new
    void * operator new(size_t sz, NRHEAP * allocator)
    {
        return allocator->Alloc(sz);
    };

    int isTrue(bool sense = true);
    int isFalse(bool sense = true);
    int isNull();
    int isZero();

    bool isOK()
    {
        return kind != EK_ERROR;
    };

    unsigned short getOffset();
    unsigned short getOwnerOffset();
    bool hasSideEffects(COMPILER *compiler);

    EXPR * getArgs();
    void setArgs(EXPR * args);
    EXPR ** getArgsPtr();
    EXPR * getObject();
    SYM * getMember();
    unsigned isDefValue();

#if DEBUG
    virtual void zDummy() {};
#endif

};

/*
 * EXPRLIST: a way to string together expressions
 */
class EXPRBINOP: public EXPR {
public:
    PEXPR p1;      // first member
    PEXPR p2;       // last member
};

class EXPRUSERLOGOP: public EXPR {
public:
    PEXPR opX;
    PEXPR callTF;
    PEXPR callOp;
};

class EXPRTYPEOF: public EXPR {
public:
    TYPESYM * sourceType;
    METHSYM * method;
};

class EXPRSIZEOF: public EXPR {
public:
    TYPESYM * sourceType;
};

class EXPRCAST: public EXPR {
public:
    PEXPR p1;      // thing being cast
};

class EXPRZEROINIT: public EXPR {
public:
    PEXPR p1;      // thing being inited, if any...
};

class EXPRBLOCK: public EXPR {
public:
    PEXPR statements;
    EXPRBLOCK * owningBlock; // the block in which this block appears...
    SCOPESYM * scopeSymbol;  // corresponding scope symbol (could be NULL if none).
    class BITSET * enterBitset;  // the bitset the last time this block was entered...
    class BITSET * exitBitset;  // the bitset the last time this block was exited...
    bool redoBlock:1;  // do we need to reanalyze this block?
    bool scanning:1; // are we currently scanning this block?
    bool exitReachable:1; // is the exit of this block reachable?
};

class EXPRSTMTAS: public EXPR {
public:
    PEXPR expression;
};

class EXPRCALL: public EXPR {
public:
    PEXPR object;
    PEXPR args;
    METHSYM * method;
};

class EXPRPROP: public EXPR {
public:
    PEXPR object;
    PEXPR args;
    PROPSYM * prop;
};

class EXPRDECL: public EXPR {
public:
    LOCVARSYM * sym;
    PEXPR init;
};

class EXPREVENT: public EXPR {
public:
    PEXPR object;
    EVENTSYM * event;
};

class EXPRFIELD: public EXPR {
public:
    PEXPR object;
    unsigned offset;
    MEMBVARSYM * field;
    unsigned ownerOffset;
};

class EXPRLOCAL: public EXPR {
public:
    LOCVARSYM * local;
};

class FINALLYSTORE {
public:
    class BITSET * entry;
    class BITSET * exit;
    bool reachableEnd;
};

class EXPRRETURN: public EXPR {
public:
    SCOPESYM * currentScope;
    FINALLYSTORE * finallyStore;
    PEXPR object;
};

class EXPRTHROW: public EXPR {
public:
    PEXPR object;
};

class EXPRCONSTANT: public EXPR {
private:
    CONSTVAL val;
public:

    EXPR ** pList;
    EXPR * list;
    NRHEAP * allocator;

    void realizeStringConstant();

    bool isNull() 
    {
        return (type->fundType() == FT_REF && val.iVal == 0);
    }

    bool isEqual(EXPRCONSTANT * expr)
    {
        FUNDTYPE ft1 = type->fundType();
        FUNDTYPE ft2 = expr->type->fundType();
        if ((ft1 == FT_REF )^(ft2 == FT_REF)) return false;

        if (list) realizeStringConstant();
        if (expr->list) expr->realizeStringConstant();
        
        if (ft1 == FT_REF) {
            if (val.iVal == 0) {
                return expr->val.iVal == 0;
            }
            return (expr->val.iVal != 0) &&
                val.strVal->length == expr->val.strVal->length && 
                !memcmp(val.strVal->text, expr->val.strVal->text, sizeof(WCHAR) * val.strVal->length );
        } else {
            return getI64Value() == expr->getI64Value() ;
        }
    }

    CONSTVALNS & getVal() 
    {
        return (CONSTVALNS&) val;
    }

    CONSTVAL & getSVal() 
    {
        if (list) realizeStringConstant();
        return val;
    }

    __int64 getI64Value() 
    {
        FUNDTYPE ft = type->fundType();

        switch (ft) {
        case FT_I8:
        case FT_U8:
            return *(val.longVal);
        case FT_U4:
            return val.uiVal;
        case FT_I1: case  FT_I2: case FT_I4: case FT_U1: case FT_U2:
            return val.iVal;
        default:
            ASSERT(0);
            return 0;
        }
    }
};

class EXPRCLASS: public EXPR {
public:
};

class EXPRNSPACE: public EXPR {
public:
    NSSYM * nspace;
};

class EXPRLABEL: public EXPR {
public:
    struct BBLOCK * block;   // the emitted block which follows this label
    EXPRBLOCK * owningBlock; // the block in which this block appears...
    SCOPESYM * scope;
    BITSET * entranceBitset;     // the bitset the last time this label was entered...
    bool definitelyReachable:1;
    bool checkedDupe:1;     // did we check that this label does not shadow an outer one?
    bool reachedByBreak:1;     // whether a break hit this label...
    bool userDefined:1;         // not a break nor a continue label...
};

class EXPRGOTO: public EXPR {
public:
    union {
        EXPRLABEL * label;   // if know the code location of the dest
        NAME * labelName; // if we need to realized in the def-use pass.
    };
    SCOPESYM * currentScope;            // the goto's location scope
    SCOPESYM * targetScope;         // the scope of the label (only goto case, and break&cont knows that scope)
    class FINALLYSTORE * finallyStore;
    class EXPRGOTO * prev;
};

class EXPRASSERT: public EXPRGOTO {
public:
    PEXPR expression;
    bool sense;
};

class EXPRGOTOIF: public EXPRGOTO {
public:
    EXPR * condition;
    bool sense;
};

class EXPRFUNCPTR : public EXPR {
public:
    METHSYM * func;
    EXPR * object;
};

class EXPRSWITCH : public EXPR {
public:
    EXPR * arg; // what are we switching on?
    unsigned labelCount; // count of case statements + default statement
    EXPRSWITCHLABEL ** labels;  // NOT A LIST!!! this is an array of label expressions
    EXPR * bodies; // lists of statements
    EXPRLABEL * breakLabel;
    EXPRLABEL * nullLabel; // if string on switch, and we have a "case: null"
    mdToken hashtableToken;
};

class EXPRHANDLER : public EXPR {
public:
    EXPRBLOCK * handlerBlock;
    LOCVARSYM * param;
};

class EXPRTRY : public EXPR {
public:
    EXPRBLOCK * tryblock;
    EXPR * handlers;  // either a block, or a list of EXPRHANDLERs
    FINALLYSTORE * finallyStore;
};

class EXPRSWITCHLABEL : public EXPRLABEL {
public:
    EXPR * key; // the key of the case statement, or null if default
    EXPR * statements; // statements under this label
    bool fellThrough:1; // did we already error about this fallthrough before?
    //EXPRSWITCH * owningSwitch;
};

class EXPRMULTI : public EXPR {
public:
    EXPR * left;
    EXPR * op;
    class EXPRWRAP * wrap;
};

class EXPRWRAP : public EXPR {
public:
    EXPR * expr;
    class LOCSLOTINFO * slot;
    bool doNotFree:1;
    bool needEmptySlot:1;
    bool pinned:1;
    TEMP_KIND   tempKind;
};


class EXPRCONCAT: public EXPR {
public:
    EXPR * list;
    EXPR ** pList;
    unsigned count;
};

class EXPRARRINIT: public EXPR {
public:
    EXPR * args;
    int *dimSizes;
    int dimSize;
};

class EXPRNOOP: public EXPR {
public:
};

class EXPRNAMEDATTRARG: public EXPR {
public:
    NAME * name;
    EXPR * value;
};

/*
 * ERRORSYM - a symbol representing an error that has been reported.
 */
enum ERROREXTRAINFO {
    ERROREXTRA_NONE,
    ERROREXTRA_MAYBECONFUSEDNEGATIVECAST, // bound to type instead of value; user might be trying
                                          // to do (E)(-1) but wrote (E)-1.
};

class EXPRERROR: public EXPR {
public:
    // Desribe the error reported in case that can be used to better
    // report additional information at a higher level.
    ERROREXTRAINFO extraInfo;
};
typedef EXPRERROR * PEXPRERROR;


// Casts from the base type:
#define EXPRDEF(k) \
    __forceinline EXPR ## k * EXPR::as ## k () {   \
        ASSERT(this == NULL || this->kind == EK_ ## k);  \
        return static_cast< EXPR ## k *>(this);     \
    }
#include "exprkind.h"
#undef EXPRDEF

__forceinline int EXPR::isTrue(bool sense) 
{
    return this && kind == EK_CONSTANT && !(asCONSTANT()->getVal().iVal ^ (int) sense);
};

__forceinline int EXPR::isFalse(bool sense) 
{
    return this && kind == EK_CONSTANT && (asCONSTANT()->getVal().iVal ^ (int) sense);
};

__forceinline int EXPR::isNull()
{
    return kind == EK_CONSTANT && (type->fundType() == FT_REF) && !asCONSTANT()->getSVal().strVal;
}

__forceinline int EXPR::isZero()
{
    if (kind == EK_CONSTANT) {
        switch( type->fundType() ) {
        case FT_I1: case FT_U1:
        case FT_I2: case FT_U2:
        case FT_I4: 
            return asCONSTANT()->getVal().iVal == 0;
        case FT_U4:
            return asCONSTANT()->getVal().uiVal == 0;
        case FT_I8: 
            return *asCONSTANT()->getVal().longVal == 0;
        case FT_U8:
            return *asCONSTANT()->getVal().ulongVal == 0;
        case FT_R4: case FT_R8:
            return *asCONSTANT()->getVal().doubleVal == 0.0;
        case FT_STRUCT: // Decimal
            {
            DECIMAL *pdec = asCONSTANT()->getVal().decVal;
            return (DECIMAL_HI32(*pdec) == 0 && DECIMAL_MID32(*pdec) == 0 && DECIMAL_LO32(*pdec) == 0);
            }
        default:
            break;
        }
    }
    return false;
}


__forceinline unsigned short EXPR::getOwnerOffset()
{
    if (kind == EK_LOCAL) {
        ASSERT(!asLOCAL()->local->isConst);
        return asLOCAL()->local->ownerOffset;
    } else if (kind == EK_FIELD) {
        return asFIELD()->ownerOffset;
    } else {
        return 0;
    }
}

__forceinline unsigned short EXPR::getOffset()
{
    if (kind == EK_LOCAL) {
        ASSERT(!asLOCAL()->local->isConst);
        return asLOCAL()->local->slot.ilSlot;
    } else if (kind == EK_FIELD) {
        return asFIELD()->offset;
    } else {
        return 0;
    }
};

__forceinline EXPR ** EXPR::getArgsPtr()
{
    ASSERT(kind == EK_CALL || kind == EK_PROP || kind == EK_FIELD || kind == EK_ARRINDEX);
    if (kind == EK_FIELD) return NULL;
    ASSERT(offsetof(EXPRCALL, args) == offsetof(EXPRPROP, args));
    ASSERT(offsetof(EXPRBINOP, p2) == offsetof(EXPRCALL, args));
    return &((static_cast<EXPRCALL*>(this))->args);
}

__forceinline EXPR * EXPR::getArgs()
{
    ASSERT(kind == EK_CALL || kind == EK_PROP || kind == EK_FIELD || kind == EK_ARRINDEX);
    if (kind == EK_FIELD) return NULL;
    ASSERT(offsetof(EXPRCALL, args) == offsetof(EXPRPROP, args));
    ASSERT(offsetof(EXPRBINOP, p2) == offsetof(EXPRCALL, args));
    return (static_cast<EXPRCALL*>(this))->args;
}

__forceinline void EXPR::setArgs(EXPR * args)
{
    ASSERT(kind == EK_CALL || kind == EK_PROP);
    ASSERT(offsetof(EXPRCALL, args) == offsetof(EXPRPROP, args));
    (static_cast<EXPRCALL*>(this))->args = args;
}

__forceinline EXPR * EXPR::getObject()
{
    ASSERT(kind == EK_CALL || kind == EK_PROP || kind == EK_FIELD);
    ASSERT(offsetof(EXPRCALL, object) == offsetof(EXPRPROP, object));
    ASSERT(offsetof(EXPRPROP, object) == offsetof(EXPRFIELD, object));
    return (static_cast<EXPRCALL*>(this))->object;
}

__forceinline SYM * EXPR::getMember()
{
    ASSERT(kind == EK_CALL || kind == EK_PROP || kind == EK_FIELD);
    ASSERT(offsetof(EXPRCALL, method) == offsetof(EXPRPROP, prop));
    ASSERT(offsetof(EXPRPROP, prop) == offsetof(EXPRFIELD, field));
    return (static_cast<EXPRCALL*>(this))->method;
}

