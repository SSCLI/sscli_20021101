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
// File: ilgen.h
//
// Defines record used to generate il for a method
// ===========================================================================

class Compiler;

/////////////////////////////////////////////////////////////////////////////////////////

enum ILCODE {
#define OPDEF(id, name, pop, push, operand, type, len, b1, b2, cf) id,
#include "opcode.def"
#undef OPDEF
    cee_last,
    cee_next,
    cee_stop,
};

/////////////////////////////////////////////////////////////////////////////////////////

struct REFENCODING {
    BYTE b1;
    BYTE b2;
};

/////////////////////////////////////////////////////////////////////////////////////////

struct DEBUGINFO {
    POSDATA begin;
    POSDATA end;
    BBLOCK *endBlock;    
    BBLOCK *beginBlock;
    unsigned short beginOffset;
    unsigned short endOffset;
    DEBUGINFO *next;
    DEBUGINFO *prev;
};

/////////////////////////////////////////////////////////////////////////////////////////

struct SWITCHDESTGOTO {
    BBLOCK * dest;
    bool jumpIntoTry:1;
};

struct SWITCHDEST {
    DWORD count;
    SWITCHDESTGOTO blocks[0];  // this is actually of size "count"
};

/////////////////////////////////////////////////////////////////////////////////////////

struct BBLOCK {
    union {
        BBLOCK * jumpDest;
        SWITCHDEST * switchDest;
        SYM * sym;
    };
    BBLOCK * next;
    union {
        unsigned startOffset;
        BBLOCK * markedWith;
        SWITCHDESTGOTO * markedWithSwitch;
    };
    BYTE * code;
    DEBUGINFO * debugInfo;

    size_t curLen;
    ILCODE exitIL;
    bool reachable:1; // post op && op && pre op
    union {
        struct { // post op
            bool largeJump:1;  
        } u;
        struct { // pre op && op
            bool gotoBlocked:1;
            bool jumpIntoTry:1;
            bool startsCatchOrFinally:1;
            bool endsFinally:1;
            bool condIsUN:1;
            bool mayRemove:1;
            bool startsTry:1;
        } v;
    };

    bool isNOP() {
        return exitIL == cee_next && !curLen;
    }
    // Size = 8 x 4 == 32 bytes... (on 32 bit machines, on 64 bit its more...)
};
#define BBSIZE 1024
#define BB_TOPOFF 20


/////////////////////////////////////////////////////////////////////////////////////////

struct HANDLERINFO {
    BBLOCK * tryBegin; // beginning of try
    BBLOCK * tryEnd;   // end of try
    BBLOCK * handBegin;  // beginning of handler
    BBLOCK * handEnd;  // end of handler
    TYPESYM * type;
    HANDLERINFO * next;
};

/////////////////////////////////////////////////////////////////////////////////////////

typedef LOCSLOTINFO * PSLOT;

/////////////////////////////////////////////////////////////////////////////////////////

static const int TEMPBUCKETSIZE = 16; 

struct TEMPBUCKET {
    LOCSLOTINFO slots[TEMPBUCKETSIZE] ; 
    TEMPBUCKET * next;
};

/////////////////////////////////////////////////////////////////////////////////////////

enum ADDRTYPE {
    ADDR_NONE       = 0x0,  // no address (could be a local, or have a static address...)
    ADDR_STACK      = 0x1,  // leave address on stack  (can be combined w/ _STORE)
    ADDR_DUP        = 0x2,  // leave address on stack twice
    ADDR_STORE      = 0x4,  // store address (nothing on stack)
    ADDR_STACKSTORE = 0x8,  // leave address on stack and store as well...
    ADDR_NEEDBOX    = 0x10, // address of obj is constant and needs to be boxed...
    ADDR_CONSTANT   = 0x20, // objs is a constant address and needs to be regenerated
    ADDR_LOADONLY   = 0x40, // can optimize for only loads (addr will not be stored into)
};


struct ADDRESS {
    int type;
    unsigned stackCount;
    union {
        PSLOT * argSlots; // if _STORE, this is the array, count is stackCount 
        PSLOT argSlot; // if _STORE and we only need 1 arg...
    };
    PSLOT baseSlot; // Holds object pointer if needs to be saved, or backed object pointer
    EXPR * expr;
    ADDRESS() : type(ADDR_STACK), argSlots(NULL), baseSlot(NULL) { }
    ADDRESS(ADDRTYPE typ) : type(typ), argSlots(NULL), baseSlot(NULL) { }

    void * operator new(size_t sz, ADDRESS * addr)
    {
        return addr;
    }

    // Size is: 6 x 4 = 24 bytes... (which is stack based, so that's not SOOO bad...)
    // Consider compressing it into 20 bytes by smashing type & stackCount...
};
typedef ADDRESS *PADDRESS;


/////////////////////////////////////////////////////////////////////////////////////////

struct VALUSED {

};
typedef VALUSED * PVALUSED;
#define VUYES ((VALUSED*)-1)

/////////////////////////////////////////////////////////////////////////////////////////

struct SWITCHBUCKET {
    unsigned __int64 firstMember;
    unsigned __int64 nextMember; // ie, member after last, so that (next-first) == slots

    unsigned members; // actual members present in slots
    EXPRSWITCHLABEL ** labels; // starts w/ firstMember
    SWITCHBUCKET * prevBucket;

};
typedef SWITCHBUCKET * PSWITCHBUCKET;

/////////////////////////////////////////////////////////////////////////////////////////

typedef bool (*BBMarkFnc)(BBLOCK *, void * );

/////////////////////////////////////////////////////////////////////////////////////////

#define NO_DEBUG_LINE   (0x00FEEFED)    // == 0x00FeeFee - 1

/////////////////////////////////////////////////////////////////////////////////////////

struct ENCLocal
{
    NAME *      name;
    TYPESYM *   type;
    unsigned    slot;
    TEMP_KIND   tempKind;
    ENCLocal *  next;
};

/////////////////////////////////////////////////////////////////////////////////////////
class ILGENREC {
public:

    void compile(METHSYM * method, METHINFO * info, EXPR * tree);
    ILGENREC();

private:
    METHSYM * method;
    METHINFO * info;
    SCOPESYM * localScope;
    AGGSYM * cls;
    HCEEFILE pFile;

    NRHEAP * allocator;

    BBLOCK * firstBB;
    BBLOCK * currentBB;
    BBLOCK   inlineBB;
    
    HANDLERINFO * handlers;
    HANDLERINFO * lastHandler;
    PSLOT origException;
    BBLOCK * returnLocation;
    bool returnHandled;
    unsigned blockedLeave;
    unsigned ehCount;

    DEBUGINFO * curDebugInfo;

    TOKENDATA lastPos;
    BASENODE * lastNode;

    TEMPBUCKET * temporaries;
    unsigned permLocalCount;
    PSLOT retTemp;
    int globalFieldCount;

    int curStack;
    int maxStack;

    void markStackMax() {
        if (maxStack < curStack) maxStack = curStack;
        ASSERT(maxStack >= 0 && curStack >= 0);
    }

    long getOpenIndex();
    long getCloseIndex();
    SCOPESYM * getLocalScope();
    AGGSYM * searchInterveningClasses(METHPROPSYM * sym);
    METHPROPSYM * getBaseMethod(METHPROPSYM * sym);
    METHPROPSYM * getVarArgMethod(METHPROPSYM * sym, EXPR * args);
    TYPESYM ** getLocalTypes(TYPESYM ** array, SCOPESYM * scope);
    unsigned assignLocals(unsigned startWith, SCOPESYM * scope);
    void preInitLocals(SCOPESYM * scope);
    void initLocal(PSLOT slot, TYPESYM * type = NULL);
    void assignEncLocals(ENCLocal *oldLocals);
    void assignEncLocals(ENCLocal *oldLocals, unsigned *newLocalSlot, SCOPESYM * scope);
    void assignParams();
    mdToken computeLocalSignature();
    TOKENDATA __fastcall getPosFromTree(BASENODE * tree);
    TOKENDATA __fastcall getPosFromIndex(long index);

    BBLOCK * createNewBB(bool makeCurrent = false);
    BBLOCK * startNewBB(BBLOCK * next, ILCODE exitIL = cee_next, BBLOCK * jumpDest = NULL);
    void endBB(ILCODE exitIL, BBLOCK * jumpDest);
    void initInlineBB();
    void initFirstBB();
    void flushBB();
    unsigned getCOffset() { return (unsigned)(inlineBB.code - (reusableBuffer)); }
    void closeDebugInfo();
    void emitDebugDataPoint(BASENODE * tree, int flags);
    void emitDebugDataPoint(long index, int flags);
    void setDebugDataPoint();
    void emitDebugLocalUsage(BASENODE * tree, LOCVARSYM * sym);
    void maybeEmitDebugLocalUsage(BASENODE * tree, LOCVARSYM * sym);
    void openDebugInfo(BASENODE * tree, int flags);
    void openDebugInfo(long index, int flags);
    void createNewDebugInfo();
    ILCODE getFlippedBranch(ILCODE br, bool reverseToUN);
    static bool fitsInBucket(PSWITCHBUCKET bucket, unsigned __int64 key, unsigned newMembers);
    static unsigned mergeLastBucket(PSWITCHBUCKET * lastBucket);
    void emitSwitchBuckets(PSWITCHBUCKET * array, unsigned first, unsigned last, PSLOT slot, BBLOCK * defBlock);
    void emitSwitchBucket(PSWITCHBUCKET bucket, PSLOT slot, BBLOCK * defBlock);
    BBLOCK * emitSwitchBucketGuard(EXPR * key, PSLOT slot, bool force);
    void emitDebugInfo(unsigned codeSize);
    void emitDebugScopesAndVars(SCOPESYM * scope, unsigned codeSize);

    void putOpcode(ILCODE opcode);
    void putOpcode(BYTE ** buffer, ILCODE opcode);
    void putDWORD(DWORD dw);
    void putWORD(WORD w);
    void putCHAR(char c);
    void putQWORD(__int64 * qw);

    void genPrologue();
    void genBlock(EXPRBLOCK * tree);
    void genStatement(EXPR * tree);
#if DEBUG
    void genStatementInner(EXPR * tree);
#endif
    void genExpr(EXPR * tree, PVALUSED valUsed = VUYES);
    void genBinopExpr(EXPRBINOP* tree, PVALUSED valUsed = VUYES);
    void genString(CONSTVAL string);
    bool callAsVirtual(METHSYM * meth, EXPR * object, bool isBaseCall);
    void genCall(EXPRCALL * tree, PVALUSED valUsed);
    void emitRefParam(EXPR * arg, PSLOT * curSlot);
    void genSideEffects(EXPR * tree);
    void genAddress(EXPR * tree, PADDRESS addr);
    void storeObjPointerAndArgs(EXPR * obj, EXPR * args, PADDRESS addr);
    bool storeAddress(PADDRESS addr, bool leaveObject = false);
    bool storeAddressMore(PADDRESS addr, bool leaveObject = false);
    unsigned getArgCount(EXPR * args);
    void dumpAddress(PADDRESS addr, bool free);
    void dumpAddressMore(PADDRESS addr, bool free);
    void freeAddress(PADDRESS addr);
    void freeAddressMore(PADDRESS addr);
    void copyAddress(PADDRESS addr, bool free);
    void copyAddressMore(PADDRESS addr, bool free);
    void genFromAddress(PADDRESS addr, bool store, bool free = false);
    void genMemoryAddress(EXPR * tree, PSLOT * pslot, bool ptrAddr = false);
    void genSlotAddress(PSLOT slot, bool ptrAddr = false);
    void genReturn(EXPRRETURN * tree);
    void genGoto(EXPRGOTO * tree);
    void genGotoIf(EXPRGOTOIF * tree);
    void genLabel(EXPRLABEL * tree);
    BBLOCK * genCondBranch(EXPR * condition, BBLOCK * dest, bool sense);
    bool isSimpleExpr(EXPR * condition, bool * sense);
    void genCondExpr(EXPR * condition, bool sense);
    void genNewArray(EXPRBINOP * tree);
    void genSwitch(EXPRSWITCH * tree);
    void genStringSwitch(EXPRSWITCH * tree);
    void genPropInvoke(EXPRPROP * tree, PADDRESS addr, bool store);
    void genTry(EXPRTRY * tree);
    void genThrow(EXPRTHROW * tree);
    HANDLERINFO * createHandler(BBLOCK * tryBegin, BBLOCK * tryEnd, BBLOCK * handBegin, TYPESYM * type);
    void handleReturn(bool addDebugInfo = false);
    void genMultiOp(EXPRMULTI * tree, PVALUSED valUsed = VUYES);
    void genQMark(EXPRBINOP * tree, PVALUSED valUsed);
    bool canSwitchAround(EXPR * e1, EXPR * e2);
    void analyzeExpression(EXPR * expr, BITSET ** read, BITSET ** written, bool * isCall, bool * isFieldDef, bool *isFieldUse);
    void genArrayInit(EXPRARRINIT * tree, PVALUSED valUsed);
    void genArrayInitConstant(EXPRARRINIT * tree, TYPESYM * elemType, PVALUSED valUsed);
    static void writeArrayValues1(BYTE * buffer, EXPR * tree);
    static void writeArrayValues2(BYTE * buffer, EXPR * tree);
    static void writeArrayValues4(BYTE * buffer, EXPR * tree);
    static void writeArrayValues8(BYTE * buffer, EXPR * tree);
    static void writeArrayValuesD(BYTE * buffer, EXPR * tree);
    static void writeArrayValuesF(BYTE * buffer, EXPR * tree);
    void genCast(EXPRCAST * tree, PVALUSED valUsed);
    void genObjectPtr(EXPR * object, SYM * member, PSLOT * pslot);
    void genObjectPtr(EXPR * object, SYM * member, ADDRESS * addr);
    bool hasConstantAddress(EXPR * expr);
    void ensureConstantStructValue(EXPR * arg, PSLOT * pslot);
    bool maybeEnsureConstantStructValue(EXPR * arg, PSLOT * pslot);
    void genIs(EXPRBINOP * tree, PVALUSED valUsed, bool isAs);
    void genZeroInit(EXPRZEROINIT * tree, PVALUSED valUsed);
    void genArrayCall(ARRAYSYM * array, int args, ARRAYMETHOD meth);
    void genFlipParams(TYPESYM * elem, unsigned count);
    void genPtrAddr(EXPR * op, PVALUSED valUsed);
    void genSizeOf(TYPESYM * typ);

    void genIntConstant(int val);
    void genFloatConstant(float val);
    void genDoubleConstant(double *val);
    void genDecimalConstant(DECIMAL * val);
    void genLongConstant(__int64 *val);
    void genZero(BASENODE * tree, TYPESYM * type);
    void genSwap(EXPRBINOP * tree, PVALUSED valUsed);
    
    void emitRefValue(EXPRBINOP * tree);
    void genMakeRefAny(EXPRBINOP * tree, PVALUSED valUsed);
    void genTypeRefAny(EXPRBINOP * tree, PVALUSED valUsed);

    void dumpLocal(PSLOT slot, int store);
    PSLOT storeLocal(PSLOT slot);

    bool isExprOptimizedAway(EXPR *tree);

#if DEBUG
    void dumpAllBlocks();
    void dumpAllBlocksContent();
    unsigned findBlockInList(BBLOCK ** list, BBLOCK * block);
    ILCODE findInstruction(BYTE b1, BYTE b2);

    void  verifyAllTempsFree();
    PSLOT allocTemporary(TYPESYM * type, TEMP_KIND tempKind, char * file, unsigned line);
    PSLOT allocTemporaryRef(TYPESYM * type, TEMP_KIND tempKind, char * file, unsigned line);
#else
    PSLOT allocTemporary(TYPESYM * type, TEMP_KIND tempKind);
    PSLOT allocTemporaryRef(TYPESYM * type, TEMP_KIND tempKind);
#endif
    void freeTemporary(PSLOT slot);

    void emitFieldToken(MEMBVARSYM * sym);
    void emitMethodToken(METHSYM * sym);
    void emitTypeToken(TYPESYM * sym);
    void emitArrayMethodToken(ARRAYSYM * sym, ARRAYMETHOD methodId);

    static unsigned needsBoxing(PARENTSYM * parent, TYPESYM * object);
    static bool markAsVisited(BBLOCK * block, void * data);
    void markAllReachableBB(BBLOCK * start, BBMarkFnc fnc, void * data);
    void __fastcall optimizeBranchesToNOPs();
    void __fastcall optimizeBranchesToNext();
    void __fastcall optimizeBranchesOverBranches();
    void suckUpGotos();
    unsigned getFinalCodeSize();
    unsigned computeSwitchSize(BBLOCK * block);
    BYTE * copySwitchInstruction(BYTE * outBuffer, BBLOCK * block);
    BYTE * copyCode(BYTE * outBuffer, unsigned deltaToCode);
    int    computeJumpOffset(BBLOCK * from, BBLOCK * to, unsigned instrSize);
    void copyHandlers(COR_ILMETHOD_SECT_EH_CLAUSE_FAT* clauses);
    
    ILCODE getShortOpcode(ILCODE longOpcode);

    COMPILER * compiler();
    static const REFENCODING ILcodes[cee_last];
    static const int ILStackOps[cee_last];
    static const BYTE ILcodesSize[cee_last];
    static const ILCODE ILlsTiny[4][6];
    static const ILCODE ILcjInstr[6][6];
    static const ILCODE ILcjInstrUN[6][6];
    static const ILCODE ILarithInstr[EK_ARRLEN - EK_ADD + 1];
    static const ILCODE ILarithInstrUN[EK_ARRLEN - EK_ADD + 1];
    static const ILCODE ILarithInstrOvf[EK_ARRLEN - EK_ADD + 1];
    static const ILCODE ILarithInstrUNOvf[EK_ARRLEN - EK_ADD + 1];
    static const ILCODE ILstackLoad[FT_COUNT];
    static const ILCODE ILstackStore[FT_COUNT];
    static const ILCODE ILarrayLoad[FT_COUNT];
    static const ILCODE ILarrayStore[FT_COUNT];
    static const ILCODE ILaddrLoad[2][2];
    BYTE reusableBuffer[BBSIZE+BB_TOPOFF];
    static const ILCODE simpleTypeConversions[NUM_SIMPLE_TYPES][NUM_SIMPLE_TYPES];
    static const ILCODE simpleTypeConversionsOvf[NUM_SIMPLE_TYPES][NUM_SIMPLE_TYPES];
    static const ILCODE simpleTypeConversionsEx[NUM_SIMPLE_TYPES][NUM_SIMPLE_TYPES];

#if DEBUG
    static const LPSTR ILnames[cee_last];

    bool smallBlock;
#endif
};

